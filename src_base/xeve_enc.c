/* Copyright (c) 2020, Samsung Electronics Co., Ltd.
   All Rights Reserved. */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

   - Neither the name of the copyright owner, nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

#include "xeve_type.h"
#include "xeve_enc.h"
#include <math.h>

static int xeve_eco_tree(XEVE_CTX * ctx, XEVE_CORE * core, int x0, int y0, int cup, int cuw, int cuh, int cud
                       , int cu_qp_delta_code, TREE_CONS tree_cons, XEVE_BSW * bs)
{
    int ret;
    s8  split_mode;
    s8  suco_flag = 0;
    int bound;

    core->tree_cons = tree_cons;
    xeve_get_split_mode(&split_mode, cud, cup, cuw, cuh, ctx->max_cuwh, ctx->map_cu_data[core->lcu_num].split_mode);

    if(split_mode != NO_SPLIT)
    {
        if(!ctx->sps.sps_btt_flag || ((x0 + cuw <= ctx->w) && (y0 + cuh <= ctx->h)))
        {
            ctx->fn_eco_split_mode(bs, ctx, core, cud, cup, cuw, cuh, ctx->max_cuwh, x0, y0);
        }

        bound = !((x0 + cuw <= ctx->w) && (y0 + cuh <= ctx->h));
        XEVE_SPLIT_STRUCT split_struct;
        xeve_split_get_part_structure(split_mode, x0, y0, cuw, cuh, cup, cud, ctx->log2_culine, &split_struct);
        split_struct.tree_cons = xeve_get_default_tree_cons();

        for(int part_num = 0; part_num < split_struct.part_count; ++part_num)
        {
            int cur_part_num = part_num;
            int sub_cuw = split_struct.width[cur_part_num];
            int sub_cuh = split_struct.height[cur_part_num];
            int x_pos = split_struct.x_pos[cur_part_num];
            int y_pos = split_struct.y_pos[cur_part_num];

            if(x_pos < ctx->w && y_pos < ctx->h)
            {
                ret = xeve_eco_tree(ctx, core, x_pos, y_pos, split_struct.cup[cur_part_num], sub_cuw, sub_cuh, split_struct.cud[cur_part_num], cu_qp_delta_code, split_struct.tree_cons, bs);
                xeve_assert_g(XEVE_SUCCEEDED(ret), ERR);
            }
        }
    }
    else
    {
        xeve_assert(x0 + cuw <= ctx->w && y0 + cuh <= ctx->h);
        if(cuw > MIN_CU_SIZE || cuh > MIN_CU_SIZE)
        {
            ctx->fn_eco_split_mode(bs, ctx, core, cud, cup, cuw, cuh, ctx->max_cuwh, x0, y0);
        }
        core->cu_qp_delta_code = cu_qp_delta_code;
        ret = xeve_eco_unit(ctx, core, x0, y0, cup, cuw, cuh, tree_cons, bs);
        xeve_assert_g(XEVE_SUCCEEDED(ret), ERR);
    }

    return XEVE_OK;
ERR:
    return ret;
}

static int xeve_ctu_mt_core(void * arg)
{
    assert(arg != NULL);

    XEVE_BSW  * bs;
    XEVE_CORE * core = (XEVE_CORE *)arg;
    XEVE_CTX  * ctx = core->ctx;
    bs = &ctx->bs[core->thread_cnt];
    int i = core->tile_num;

    /* CABAC Initialize for each Tile */
    ctx->fn_eco_sbac_reset(GET_SBAC_ENC(bs), ctx->sh->slice_type, ctx->sh->qp, ctx->sps.tool_cm_init);
    ctx->fn_eco_sbac_reset(&core->s_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2], ctx->sh->slice_type, ctx->sh->qp, ctx->sps.tool_cm_init);

    /*Set entry point for each ctu row in the tile*/
    int sp_x_lcu = ctx->tile[core->tile_num].ctba_rs_first % ctx->w_lcu;
    int sp_y_lcu = ctx->tile[core->tile_num].ctba_rs_first / ctx->w_lcu;
    xeve_update_core_loc_param_mt(ctx, core);

    int bef_cu_qp = ctx->tile[i].qp_prev_eco[core->thread_cnt];

    /* LCU encoding loop */
    while (ctx->tile[i].f_ctb > 0)
    {
        if (core->y_lcu != sp_y_lcu && core->x_lcu < (sp_x_lcu + ctx->tile[core->tile_idx].w_ctb - 1))
        {
            /* up-right CTB */
            spinlock_wait(&ctx->sync_flag[core->lcu_num - ctx->w_lcu + 1], THREAD_TERMINATED);
        }

        /* initialize structures *****************************************/
        int ret = ctx->fn_mode_init_lcu(ctx, core);
        xeve_assert_rv(ret == XEVE_OK, ret);

        /* mode decision *************************************************/
        SBAC_LOAD(core->s_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2], *GET_SBAC_ENC(bs));
        core->s_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].is_bitcount = 1;

        ret = ctx->fn_mode_analyze_lcu(ctx, core);
        xeve_assert_rv(ret == XEVE_OK, ret);

        ret = ctx->fn_mode_post_lcu(ctx, core);
        xeve_assert_rv(ret == XEVE_OK, ret)

        ctx->tile[i].qp_prev_eco[core->thread_cnt] = bef_cu_qp;

        /* entropy coding ************************************************/
        ret = xeve_eco_tree(ctx, core, core->x_pel, core->y_pel, 0, ctx->max_cuwh, ctx->max_cuwh, 0, 0, xeve_get_default_tree_cons(), bs);
        bef_cu_qp = ctx->tile[i].qp_prev_eco[core->thread_cnt];

        xeve_assert_rv(ret == XEVE_OK, ret);

        threadsafe_assign(&ctx->sync_flag[core->lcu_num], THREAD_TERMINATED);
        threadsafe_decrement(ctx->sync_block, (volatile s32 *)&ctx->tile[i].f_ctb);

        core->lcu_num = xeve_mt_get_next_ctu_num(ctx, core, ctx->parallel_rows);
        if (core->lcu_num == -1)
            break;
    }
    return XEVE_OK;
}

XEVE_CTX * xeve_ctx_alloc(void)
{
    XEVE_CTX * ctx;

    ctx = (XEVE_CTX*)xeve_malloc_fast(sizeof(XEVE_CTX));
    xeve_assert_rv(ctx, NULL);
    xeve_mset_x64a(ctx, 0, sizeof(XEVE_CTX));
    return ctx;
}

void xeve_ctx_free(XEVE_CTX * ctx)
{
    xeve_mfree_fast(ctx);
}


XEVE_CORE * xeve_core_alloc(int chroma_format_idc)
{
    XEVE_CORE * core;
    int i, j;

    core = (XEVE_CORE *)xeve_malloc_fast(sizeof(XEVE_CORE));

    xeve_assert_rv(core, NULL);
    xeve_mset_x64a(core, 0, sizeof(XEVE_CORE));

    for (i = 0; i < MAX_CU_LOG2; i++)
    {
        for (j = 0; j < MAX_CU_LOG2; j++)
        {
            xeve_create_cu_data(&core->cu_data_best[i][j], i, j, chroma_format_idc);
            xeve_create_cu_data(&core->cu_data_temp[i][j], i, j, chroma_format_idc);
        }
    }

    return core;
}

void xeve_core_free(XEVE_CORE * core)
{
    int i, j;

    for (i = 0; i < MAX_CU_LOG2; i++)
    {
        for (j = 0; j < MAX_CU_LOG2; j++)
        {
            xeve_delete_cu_data(&core->cu_data_best[i][j], i, j);
            xeve_delete_cu_data(&core->cu_data_temp[i][j], i, j);
        }
    }

    xeve_mfree_fast(core);
}

int xeve_pic(XEVE_CTX * ctx, XEVE_BITB * bitb, XEVE_STAT * stat)
{
    XEVE_CORE     * core;
    XEVE_BSW      * bs;
    XEVE_SH       * sh;
    int             ctb_cnt_in_tile = 0;
    int             col_bd = 0;
    int             num_slice_in_pic = ctx->param.num_slice_in_pic;
    u8            * tiles_in_slice;
    u8            * curr_temp = ctx->bs[0].cur;
    int             last_intra_poc = INT_MAX;

    for (ctx->slice_num = 0; ctx->slice_num < num_slice_in_pic; ctx->slice_num++)
    {
        ctx->sh = &ctx->sh_array[ctx->slice_num];
        sh = ctx->sh;
        xeve_set_tile_in_slice(ctx);
        tiles_in_slice = sh->tile_order;

        bs = &ctx->bs[0];
        core = ctx->core[0];
        core->ctx = ctx;
        XEVE_APS* aps = &ctx->aps;

        if ((int)ctx->poc.poc_val > last_intra_poc)
        {
            last_intra_poc = INT_MAX;
        }

        if (ctx->slice_type == SLICE_I)
        {
            last_intra_poc = ctx->poc.poc_val;
            ctx->aps_counter = -1;
            aps->aps_id = -1;

            ctx->sh->aps_signaled = -1; // reset stored aps id in tile group header
            ctx->aps_temp = 0;
        }

        /* Set slice header */
        xeve_set_sh(ctx, sh);

        /* initialize reference pictures */
        int ret = xeve_picman_refp_init(&ctx->rpm, ctx->sps.max_num_ref_pics, ctx->slice_type, ctx->poc.poc_val, ctx->nalu.nuh_temporal_id, ctx->last_intra_poc, ctx->refp);
        xeve_assert_rv(ret == XEVE_OK, ret);

        ctx->fn_mode_analyze_frame(ctx);

        /* slice layer encoding loop */
        core->x_lcu = core->y_lcu = 0;
        core->x_pel = core->y_pel = 0;
        core->lcu_num = 0;
        ctx->lcu_cnt = ctx->f_lcu;

        /* Set nalu header */
        xeve_set_nalu(&ctx->nalu, ctx->pic_cnt == 0 || (ctx->slice_type == SLICE_I && ctx->param.closed_gop) ? XEVE_IDR_NUT : XEVE_NONIDR_NUT, ctx->nalu.nuh_temporal_id);

        core->qp_y = ctx->sh->qp + 6 * ctx->sps.bit_depth_luma_minus8;
        core->qp_u = ctx->qp_chroma_dynamic[0][sh->qp_u] + 6 * ctx->sps.bit_depth_chroma_minus8;
        core->qp_v = ctx->qp_chroma_dynamic[1][sh->qp_v] + 6 * ctx->sps.bit_depth_chroma_minus8;
        core->bs_temp.pdata[1] = &core->s_temp_run;

        /* LCU encoding */
#if TRACE_RDO_EXCLUDE_I
        if(ctx->slice_type != SLICE_I)
        {
#endif
        XEVE_TRACE_SET(0);
#if TRACE_RDO_EXCLUDE_I
        }
#endif
        if (ctx->sps.tool_mmvd && (ctx->slice_type == SLICE_B))
        {
            sh->mmvd_group_enable_flag = !(ctx->refp[0][0].poc == ctx->refp[0][1].poc);
        }
        else
        {
            sh->mmvd_group_enable_flag = 0;
        }

        ctx->sh->qp_prev_eco = ctx->sh->qp;
        ctx->sh->qp_prev_mode = ctx->sh->qp;
        core->dqp_data[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].prev_qp = ctx->sh->qp_prev_mode;
        core->dqp_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].curr_qp = ctx->sh->qp;
        core->dqp_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].prev_qp = ctx->sh->qp;

        /* Tile wise encoding with in a slice */
        u32 k = 0;
        u16 total_tiles_in_slice = sh->num_tiles_in_slice;
        THREAD_CONTROLLER * tc;
        int res;
        u32 i = 0;
        tc = ctx->tc;
        int thread_cnt = 0, thread_cnt1 = 0;;
        int task_completed = 0;
        int tile_cnt = 0;

        //Code for CTU parallel encoding
        while (total_tiles_in_slice)
        {
            //Limiting parallel task to the number of LCU rows
            i = tiles_in_slice[tile_cnt++];
            int temp_store_total_ctb = ctx->tile[i].f_ctb;
            int parallel_task = (ctx->param.threads > ctx->tile[i].h_ctb) ? ctx->tile[i].h_ctb : ctx->param.threads;
            ctx->parallel_rows = parallel_task;
            ctx->tile[i].qp = ctx->sh->qp;

            for (thread_cnt = 1; (thread_cnt < parallel_task); thread_cnt++)
            {
                ctx->tile[i].qp_prev_eco[thread_cnt] = ctx->sh->qp;
                ctx->core[thread_cnt]->tile_idx = i;
                ctx->core[thread_cnt]->x_lcu = ((ctx->tile[core->tile_num].ctba_rs_first) % ctx->w_lcu);               //entry point lcu's x location
                ctx->core[thread_cnt]->y_lcu = ((ctx->tile[core->tile_num].ctba_rs_first) / ctx->w_lcu) + thread_cnt; // entry point lcu's y location
                ctx->core[thread_cnt]->lcu_num = thread_cnt*ctx->tile[i].w_ctb;
                xeve_init_core_mt(ctx, i, core, thread_cnt);

                ctx->core[thread_cnt]->thread_cnt = thread_cnt;
                tc->run(ctx->thread_pool[thread_cnt], xeve_ctu_mt_core, (void*)ctx->core[thread_cnt]);
            }

            ctx->tile[i].qp = ctx->sh->qp;
            ctx->tile[i].qp_prev_eco[0] = ctx->sh->qp;
            ctx->core[0]->tile_idx = i;
            ctx->core[0]->lcu_num = 0;

            xeve_init_core_mt(ctx, i, core, 0);

            ctx->core[0]->thread_cnt = 0;
            xeve_ctu_mt_core((void*)ctx->core[0]);

             for (thread_cnt1 = 1; thread_cnt1 < parallel_task; thread_cnt1++)
            {
                tc->join(ctx->thread_pool[thread_cnt1], &res);
                if (XEVE_FAILED(res))
                {
                    ret = res;
                }
            }

            ctx->tile[i].f_ctb = temp_store_total_ctb;


            /*Set entry point for each Tile in the tile Slice*/
            ctx->core[0]->x_lcu = (ctx->tile[i].ctba_rs_first) % ctx->w_lcu; //entry point lcu's x location
            ctx->core[0]->y_lcu = (ctx->tile[i].ctba_rs_first) / ctx->w_lcu; // entry point lcu's y location
            ctb_cnt_in_tile = ctx->tile[i].f_ctb; //Total LCUs in the current tile
            xeve_update_core_loc_param(ctx, ctx->core[0]);
            ctx->lcu_cnt = ctx->f_lcu;
            while (1)
            {
                /* entropy coding ************************************************/
                ret = xeve_eco_tree(ctx, ctx->core[0], ctx->core[0]->x_pel, ctx->core[0]->y_pel, 0, ctx->max_cuwh, ctx->max_cuwh, 0
                                  , 0, xeve_get_default_tree_cons(), &ctx->bs[0]);
                /* prepare next step *********************************************/
                ctx->core[0]->x_lcu++;
                if (ctx->core[0]->x_lcu >= ctx->tile[i].w_ctb + col_bd)
                {
                    ctx->core[0]->x_lcu = (ctx->tile[i].ctba_rs_first) % ctx->w_lcu;
                    ctx->core[0]->y_lcu++;
                }
                xeve_update_core_loc_param(ctx, ctx->core[0]);
                ctb_cnt_in_tile--;
                ctx->lcu_cnt--;
                if (ctb_cnt_in_tile == 0)
                {
                    xeve_eco_tile_end_flag(bs, 1);
                    xeve_sbac_finish(bs);
                    break;
                }
            }
            total_tiles_in_slice -= 1;
        }

        ctx->sh->qp_prev_eco = ctx->sh->qp;
        ctx->fn_loop_filter(ctx, core);
        core->x_lcu = core->y_lcu = 0;
        core->x_pel = core->y_pel = 0;
        core->lcu_num = 0;
        ctx->lcu_cnt = ctx->f_lcu;
        for (i = 0; i < ctx->f_scu; i++)
        {
            MCU_CLR_COD(ctx->map_scu[i]);
        }

        /* Bit-stream re-writing (START) */
        xeve_bsw_init_slice(&ctx->bs[0], (u8*)curr_temp, bitb->bsize, NULL);

#if TRACE_START_POC
        if (fp_trace_started == 1)
        {
            XEVE_TRACE_SET(1);
        }
        else
        {
            XEVE_TRACE_SET(0);
        }
#else
#if TRACE_RDO_EXCLUDE_I
        if(ctx->slice_type != SLICE_I)
        {
#endif
#if !TRACE_DBF
            XEVE_TRACE_SET(1);
#endif
#if TRACE_RDO_EXCLUDE_I
        }
#endif
#endif

        XEVE_SBAC* t_sbac;
        t_sbac = GET_SBAC_ENC(bs);
        t_sbac->bin_counter = 0;

        unsigned int bin_counts_in_units = 0;
        unsigned int num_bytes_in_units = 0;

        /* Send available APSs */
        int aps_nalu_size = 0;
        u8 * nalu_len_buf = bs->cur;
        u8* cur_tmp = bs->cur;

        /* Encode nalu header */
        ret = xeve_eco_nalu(bs, &ctx->nalu);
        xeve_assert_rv(ret == XEVE_OK, ret);

        /* Encode slice header */
        sh->num_ctb = ctx->f_lcu;
        XEVE_BSW bs_sh;
        xeve_mcpy(&bs_sh, bs, sizeof(XEVE_BSW));
#if TRACE_HLS
        s32 tmp_fp_point = ftell(fp_trace);
#endif
        ret = ctx->fn_eco_sh(bs, &ctx->sps, &ctx->pps, sh, ctx->nalu.nal_unit_type_plus1 - 1);
        xeve_assert_rv(ret == XEVE_OK, ret);

        core->x_lcu = core->y_lcu = 0;
        core->x_pel = core->y_pel = 0;
        core->lcu_num = 0;
        ctx->lcu_cnt = ctx->f_lcu;
        for(i = 0; i < ctx->f_scu; i++)
        {
            MCU_CLR_COD(ctx->map_scu[i]);
        }
        ctx->sh->qp_prev_eco = ctx->sh->qp;

        /* Tile level encoding for a slice */
        /* Tile wise encoding with in a slice */
        k = 0;
        total_tiles_in_slice = sh->num_tiles_in_slice;
        while (total_tiles_in_slice)
        {
            int i = tiles_in_slice[k++];
            ctx->tile[i].qp = ctx->sh->qp;
            ctx->tile[i].qp_prev_eco[0] = ctx->sh->qp;
            core->tile_idx = i;

            /* CABAC Initialize for each Tile */
            ctx->fn_eco_sbac_reset(GET_SBAC_ENC(bs), ctx->sh->slice_type, ctx->sh->qp, ctx->sps.tool_cm_init);

            /*Set entry point for each Tile in the tile Slice*/
            core->x_lcu = (ctx->tile[i].ctba_rs_first) % ctx->w_lcu; //entry point lcu's x location
            core->y_lcu = (ctx->tile[i].ctba_rs_first) / ctx->w_lcu; // entry point lcu's y location
            ctb_cnt_in_tile = ctx->tile[i].f_ctb; //Total LCUs in the current tile
            xeve_update_core_loc_param(ctx, core);

            XEVE_BSW bs_beg;
            bs_beg.cur = bs->cur;
            bs_beg.leftbits = bs->leftbits;

            col_bd = 0;
            if (i% ctx->param.tile_columns)
            {
                int temp = i - 1;
                while (temp >= 0)
                {
                    col_bd += ctx->tile[temp].w_ctb;
                    if (!(temp%ctx->param.tile_columns)) break;
                    temp--;
                }
            }
            else
            {
                col_bd = 0;
            }

            while (1) // LCU level CABAC loop
            {
                ret = xeve_eco_tree(ctx, core, core->x_pel, core->y_pel, 0, ctx->max_cuwh, ctx->max_cuwh, 0, 0, xeve_get_default_tree_cons(), bs);

                xeve_assert_rv(ret == XEVE_OK, ret);
                /* prepare next step *********************************************/
                core->x_lcu++;
                if (core->x_lcu >= ctx->tile[i].w_ctb + col_bd)
                {
                    core->x_lcu = (ctx->tile[i].ctba_rs_first) % ctx->w_lcu;
                    core->y_lcu++;
                }

                xeve_update_core_loc_param(ctx, core);
                ctb_cnt_in_tile--;
                ctx->lcu_cnt--;

                /* end_of_picture_flag */
                if (ctb_cnt_in_tile == 0)
                {
                    xeve_eco_tile_end_flag(bs, 1);
                    xeve_sbac_finish(bs);
                    break;
                }
            } //End of LCU encoding loop in a tile

            XEVE_SBAC* tmp_sbac;
            tmp_sbac = GET_SBAC_ENC(bs);
            bin_counts_in_units += tmp_sbac->bin_counter;
            total_tiles_in_slice--;

            sh->entry_point_offset_minus1[k - 1] = (u32)((bs)->cur - bs_beg.cur - 4 + (4 - (bs->leftbits >> 3)) + (bs_beg.leftbits >> 3) - 1);
        } // End to tile encoding loop in a slice

        num_bytes_in_units = (int)(bs->cur - cur_tmp) - 4;

        int log2_sub_wh_c = 2;
        int min_cu_w = ctx->min_cuwh;
        int min_cu_h = ctx->min_cuwh;
        int padded_w = ((ctx->w + min_cu_w - 1) / min_cu_w) * min_cu_w;
        int padded_h = ((ctx->h + min_cu_h - 1) / min_cu_h) * min_cu_h;
        int raw_bits = padded_w * padded_h * ((ctx->sps.bit_depth_luma_minus8 + 8) + (ctx->sps.chroma_format_idc != 0 ? (2 * ((ctx->sps.bit_depth_chroma_minus8 + 8) >> log2_sub_wh_c)) : 0));
        unsigned int threshold = (CABAC_ZERO_PARAM / 3) * num_bytes_in_units + (raw_bits / 32);

        if (bin_counts_in_units >= threshold)
        {
            unsigned int target_num_bytes_in_units = ((bin_counts_in_units - (raw_bits / 32)) * 3 + (CABAC_ZERO_PARAM - 1)) / CABAC_ZERO_PARAM;
            if (target_num_bytes_in_units > num_bytes_in_units)
            {
                unsigned int num_add_bytes_needed = target_num_bytes_in_units - num_bytes_in_units;
                unsigned int num_add_cabac_zero_words = (num_add_bytes_needed + 2) / 3;
                unsigned int num_add_cabac_zero_bytes = num_add_cabac_zero_words * 3;
                for (unsigned int i = 0; i < num_add_cabac_zero_words; i++)
                {
                    xeve_bsw_write(bs, 0, 16); //2 bytes (=00 00))
                }
            }
        }

        xeve_bsw_deinit(bs);
        xeve_eco_nal_unit_len(nalu_len_buf, (int)(bs->cur - cur_tmp) - 4);

        curr_temp = bs->cur;

        /* slice header re-writing */
#if TRACE_HLS
        s32 tmp_fp_point2 = ftell(fp_trace);
        fseek(fp_trace, tmp_fp_point, SEEK_SET);
#endif
        ret = ctx->fn_eco_sh(&bs_sh, &ctx->sps, &ctx->pps, sh, ctx->nalu.nal_unit_type_plus1 - 1);
        xeve_assert_rv(ret == XEVE_OK, ret);
        xeve_bsw_deinit(&bs_sh);
#if TRACE_HLS
        fseek(fp_trace, tmp_fp_point2, SEEK_SET);
#endif
        /* Bit-stream re-writing (END) */

    }  // End of slice loop

    return XEVE_OK;
}

int xeve_enc(XEVE_CTX * ctx, XEVE_BITB * bitb, XEVE_STAT * stat)
{
    int            ret;
    int            gop_size, pic_cnt;

    pic_cnt = ctx->pic_icnt - ctx->frm_rnum;
    gop_size = ctx->param.gop_size;

    if (ctx->param.keyint == 0)
    {
        ctx->force_slice = ((ctx->pic_ticnt % gop_size >= ctx->pic_ticnt - pic_cnt + 1) && FORCE_OUT(ctx)) ? 1 : 0;
    }
    else
    {
        ctx->force_slice = (((int)(ctx->pic_ticnt % ctx->param.keyint) % gop_size >= (int)(ctx->pic_ticnt % ctx->param.keyint) - pic_cnt + 1) && FORCE_OUT(ctx)) ? 1 : 0;
    }

    xeve_assert_rv(bitb->addr && bitb->bsize > 0, XEVE_ERR_INVALID_ARGUMENT);

    /* initialize variables for a picture encoding */
    ret = ctx->fn_enc_pic_prepare(ctx, bitb, stat);
    xeve_assert_rv(ret == XEVE_OK, ret);

    /* encode parameter set */
    ret = ctx->fn_enc_header(ctx);
    xeve_assert_rv(ret == XEVE_OK, ret);

    /* encode one picture */
    ret = ctx->fn_enc_pic(ctx, bitb, stat);
    xeve_assert_rv(ret == XEVE_OK, ret);

    /* finishing of encoding a picture */
    ctx->fn_enc_pic_finish(ctx, bitb, stat);
    xeve_assert_rv(ret == XEVE_OK, ret);

    return XEVE_OK;
}


int xeve_push_frm(XEVE_CTX * ctx, XEVE_IMGB * img)
{
    XEVE_PIC  * pic;
    XEVE_PICO * pico;
    XEVE_IMGB * imgb;

    int ret;

    ret = ctx->fn_get_inbuf(ctx, &imgb);
    xeve_assert_rv(XEVE_OK == ret, ret);

    imgb->cs = ctx->param.cs;
    xeve_imgb_cpy(imgb, img);

    if (ctx->fn_pic_flt != NULL)
    {
        ctx->fn_pic_flt(ctx, imgb);
    }

    ctx->pic_icnt++;
    ctx->pico_idx = ctx->pic_icnt % ctx->pico_max_cnt;
    pico = ctx->pico_buf[ctx->pico_idx];
    pico->pic_icnt = ctx->pic_icnt;
    pico->is_used = 1;
    pic = &pico->pic;
    ctx->pico = pico;

    PIC_ORIG(ctx) = pic;

    /* set pushed image to current input (original) image */
    xeve_mset(pic, 0, sizeof(XEVE_PIC));

    pic->buf_y = imgb->baddr[0];
    pic->buf_u = imgb->baddr[1];
    pic->buf_v = imgb->baddr[2];

    pic->y = imgb->a[0];
    pic->u = imgb->a[1];
    pic->v = imgb->a[2];

    pic->w_l = imgb->w[0];
    pic->h_l = imgb->h[0];
    pic->w_c = imgb->w[1];
    pic->h_c = imgb->h[1];

    pic->s_l = STRIDE_IMGB2PIC(imgb->s[0]);
    pic->s_c = STRIDE_IMGB2PIC(imgb->s[1]);

    pic->imgb = imgb;
    /* generate sub-picture for RC and Forecast */
    if (ctx->param.use_fcst)
    {
        XEVE_PIC* spic = pico->spic;
        xeve_gen_subpic(pic->y, spic->y, spic->w_l, spic->h_l, pic->s_l, spic->s_l, 10);

        xeve_mset(pico->sinfo.map_pdir, 0, sizeof(u8) * ctx->fcst.f_blk);
        xeve_mset(pico->sinfo.map_pdir_bi, 0, sizeof(u8) * ctx->fcst.f_blk);
        xeve_mset(pico->sinfo.map_mv, 0, sizeof(s16) * ctx->fcst.f_blk * REFP_NUM * MV_D);
        xeve_mset(pico->sinfo.map_mv_bi, 0, sizeof(s16) * ctx->fcst.f_blk * REFP_NUM * MV_D);
        xeve_mset(pico->sinfo.map_mv_pga, 0, sizeof(s16) * ctx->fcst.f_blk * REFP_NUM * MV_D);
        xeve_mset(pico->sinfo.map_uni_lcost, 0, sizeof(s32) * ctx->fcst.f_blk * 4);
        xeve_mset(pico->sinfo.map_bi_lcost, 0, sizeof(s32) * ctx->fcst.f_blk);
        xeve_mset(pico->sinfo.map_qp_blk, 0, sizeof(s32) * ctx->fcst.f_blk);
        xeve_mset(pico->sinfo.map_qp_scu, 0, sizeof(s8) * ctx->f_scu);
        xeve_mset(pico->sinfo.transfer_cost, 0, sizeof(u16) * ctx->fcst.f_blk);
        xeve_picbuf_expand(spic, spic->pad_l, spic->pad_c, ctx->sps.chroma_format_idc);
    }

    if (ctx->ts.frame_delay > 0)
    {
        if (ctx->pic_icnt == 0)
        {
            ctx->ts.frame_first_pts = pic->imgb->ts[XEVE_TS_PTS];
        }
        else if (ctx->pic_icnt == ctx->ts.frame_delay)
        {
            ctx->ts.frame_dealy_time = ctx->ts.frame_first_pts - pic->imgb->ts[XEVE_TS_PTS];
        }
    }

    ctx->ts.frame_ts[ctx->pic_icnt % XEVE_MAX_INBUF_CNT] = pic->imgb->ts[XEVE_TS_PTS];

    return XEVE_OK;
}


void xeve_platform_init_func(XEVE_CTX * ctx)
{
#if ARM_NEON
  if(1)
  {
        xeve_func_sad               = xeve_tbl_sad_16b_neon;
        xeve_func_ssd               = xeve_tbl_ssd_16b_neon;
        xeve_func_diff              = xeve_tbl_diff_16b_neon;
        xeve_func_satd              = xeve_tbl_satd_16b_neon;
        xeve_func_mc_l              = xeve_tbl_mc_l_neon;
        xeve_func_mc_c              = xeve_tbl_mc_c_neon;
        xeve_func_average_no_clip   = &xeve_average_16b_no_clip_neon;
        ctx->fn_itxb                = &xeve_tbl_itxb_neon;
        xeve_func_txb               = &xeve_tbl_txb_neon;
  }
  else
#elif X86_SSE
    int check_cpu, support_sse, support_avx, support_avx2;

    check_cpu = xeve_check_cpu_info();
    support_sse  = (check_cpu >> 1) & 1;
    support_avx2 = (check_cpu >> 2) & 1;

    if (support_avx2)
    {
        xeve_func_sad               = xeve_tbl_sad_16b_avx;
        xeve_func_ssd               = xeve_tbl_ssd_16b_sse;
        xeve_func_diff              = xeve_tbl_diff_16b_sse;
        xeve_func_satd              = xeve_tbl_satd_16b_sse;
        xeve_func_mc_l              = xeve_tbl_mc_l_avx;
        xeve_func_mc_c              = xeve_tbl_mc_c_avx;
        xeve_func_average_no_clip   = &xeve_average_16b_no_clip_sse;
        ctx->fn_itxb                = &xeve_tbl_itxb_avx;
        xeve_func_txb               = &xeve_tbl_txb_avx;
    }
    else if (support_sse)
    {
        xeve_func_sad               = xeve_tbl_sad_16b_sse;
        xeve_func_ssd               = xeve_tbl_ssd_16b_sse;
        xeve_func_diff              = xeve_tbl_diff_16b_sse;
        xeve_func_satd              = xeve_tbl_satd_16b_sse;
        xeve_func_mc_l              = xeve_tbl_mc_l_sse;
        xeve_func_mc_c              = xeve_tbl_mc_c_sse;
        xeve_func_average_no_clip   = &xeve_average_16b_no_clip_sse;
        ctx->fn_itxb                = &xeve_tbl_itxb_sse;
        xeve_func_txb               = &xeve_tbl_txb; /*to be updated*/
    }
    else
#endif
    {
        xeve_func_sad               = xeve_tbl_sad_16b;
        xeve_func_ssd               = xeve_tbl_ssd_16b;
        xeve_func_diff              = xeve_tbl_diff_16b;
        xeve_func_satd              = xeve_tbl_satd_16b;
        xeve_func_mc_l              = xeve_tbl_mc_l;
        xeve_func_mc_c              = xeve_tbl_mc_c;
        xeve_func_average_no_clip   = &xeve_average_16b_no_clip;
        ctx->fn_itxb                = &xeve_tbl_itxb;
        xeve_func_txb               = &xeve_tbl_txb;
    }
}

int xeve_platform_init(XEVE_CTX * ctx)
{
    int ret = XEVE_ERR_UNKNOWN;

    /* create mode decision */
    ret = xeve_mode_create(ctx, 0);
    xeve_assert_rv(XEVE_OK == ret, ret);

    /* create intra prediction analyzer */
    ret = xeve_pintra_create(ctx, 0);
    xeve_assert_rv(XEVE_OK == ret, ret);

    /* create inter prediction analyzer */
    if (ctx->param.profile == XEVE_PROFILE_BASELINE)
    {
        ret = xeve_pinter_create(ctx, 0);
        xeve_assert_rv(XEVE_OK == ret, ret);
    }

    ctx->fn_ready             = xeve_ready;
    ctx->fn_flush             = xeve_flush;
    ctx->fn_enc               = xeve_enc;
    ctx->fn_enc_header        = xeve_header;
    ctx->fn_enc_pic           = xeve_pic;
    ctx->fn_enc_pic_prepare   = xeve_pic_prepare;
    ctx->fn_enc_pic_finish    = xeve_pic_finish;
    ctx->fn_push              = xeve_push_frm;
    ctx->fn_deblock           = xeve_deblock;
    ctx->fn_picbuf_expand     = xeve_pic_expand;
    ctx->fn_get_inbuf         = xeve_picbuf_get_inbuf;
    ctx->fn_loop_filter       = xeve_loop_filter;
    ctx->fn_encode_sps        = xeve_encode_sps;
    ctx->fn_encode_pps        = xeve_encode_pps;
    ctx->fn_encode_sei        = xeve_encode_sei;
    ctx->fn_eco_sh            = xeve_eco_sh;
    ctx->fn_eco_split_mode    = xeve_eco_split_mode;
    ctx->fn_eco_sbac_reset    = xeve_sbac_reset;
    ctx->fn_eco_coef          = xeve_eco_coef;
    ctx->fn_eco_pic_signature = xeve_eco_pic_signature;
    ctx->fn_tq                = xeve_sub_block_tq;
    ctx->fn_rdoq_set_ctx_cc   = xeve_rdoq_set_ctx_cc;
    ctx->fn_itdp              = xeve_itdq;
    ctx->fn_recon             = xeve_recon;
    ctx->fn_deblock_tree      = xeve_deblock_tree;
    ctx->fn_deblock_unit      = xeve_deblock_unit;
    ctx->fn_set_tile_info     = xeve_set_tile_info;
    ctx->fn_rdo_intra_ext     = NULL;
    ctx->fn_rdo_intra_ext_c   = NULL;
    ctx->pic_dbk              = NULL;
    ctx->fn_pocs              = NULL;
    ctx->fn_pic_flt           = NULL;
    ctx->pf                   = NULL;

    xeve_platform_init_func(ctx);

    return XEVE_OK;
}

void xeve_platform_deinit(XEVE_CTX * ctx)
{
    xeve_assert(ctx->pf == NULL);

    ctx->fn_ready = NULL;
    ctx->fn_flush = NULL;
    ctx->fn_enc = NULL;
    ctx->fn_enc_pic = NULL;
    ctx->fn_enc_pic_prepare = NULL;
    ctx->fn_enc_pic_finish = NULL;
    ctx->fn_push = NULL;
    ctx->fn_deblock = NULL;
    ctx->fn_picbuf_expand = NULL;
    ctx->fn_get_inbuf = NULL;
}

int xeve_create_bs_buf(XEVE_CTX  * ctx, int max_bs_buf_size)
{
    u8 * bs_buf, *bs_buf_temp;
    if (ctx->param.threads > 1)
    {
        bs_buf = (u8 *)xeve_malloc(sizeof(u8) * (ctx->param.threads - 1) * max_bs_buf_size);
        for (int task_id = 1; task_id < ctx->param.threads; task_id++)
        {
            bs_buf_temp = bs_buf + ((task_id - 1) * max_bs_buf_size);
            xeve_bsw_init(&ctx->bs[task_id], bs_buf_temp, max_bs_buf_size, NULL);
            ctx->bs[task_id].pdata[1] = &ctx->sbac_enc[task_id];
        }
    }
    return XEVE_OK;
}

int xeve_delete_bs_buf(XEVE_CTX  * ctx)
{
    if (ctx->param.threads > 1)
    {
        u8 * bs_buf_temp = ctx->bs[1].beg;
        if (bs_buf_temp != NULL)
        {
            xeve_mfree(bs_buf_temp);
        }
        bs_buf_temp = NULL;
    }
    return XEVE_OK;
}


int xeve_encode_sps(XEVE_CTX * ctx)
{
    XEVE_BSW * bs = &ctx->bs[0];
    XEVE_SPS * sps = &ctx->sps;
    XEVE_NALU  nalu;

    u8* size_field = bs->cur;
    u8* cur_tmp = bs->cur;

    /* nalu header */
    xeve_set_nalu(&nalu, XEVE_SPS_NUT, 0);
    xeve_eco_nalu(bs, &nalu);

    /* sequence parameter set*/
    xeve_set_sps(ctx, &ctx->sps);
    xeve_assert_rv(xeve_eco_sps(bs, sps) == XEVE_OK, XEVE_ERR_INVALID_ARGUMENT);

    /* de-init BSW */
    xeve_bsw_deinit(bs);

    /* write the bitstream size */
    xeve_eco_nal_unit_len(size_field, (int)(bs->cur - cur_tmp) - 4);

    return XEVE_OK;
}

int xeve_encode_pps(XEVE_CTX * ctx)
{
    XEVE_BSW * bs = &ctx->bs[0];
    XEVE_SPS * sps = &ctx->sps;
    XEVE_PPS * pps = &ctx->pps;
    XEVE_NALU  nalu;
    u8       * size_field = bs->cur;
    u8       * cur_tmp = bs->cur;

    /* nalu header */
    xeve_set_nalu(&nalu, XEVE_PPS_NUT, ctx->nalu.nuh_temporal_id);
    xeve_eco_nalu(bs, &nalu);

    /* sequence parameter set*/
    xeve_set_pps(ctx, &ctx->pps);
    xeve_assert_rv(xeve_eco_pps(bs, sps, pps) == XEVE_OK, XEVE_ERR_INVALID_ARGUMENT);

    /* de-init BSW */
    xeve_bsw_deinit(bs);

    /* write the bitstream size */
    xeve_eco_nal_unit_len(size_field, (int)(bs->cur - cur_tmp) - 4);
    return XEVE_OK;
}

int xeve_encode_sei(XEVE_CTX * ctx)
{
    XEVE_BSW * bs = &ctx->bs[0];
    XEVE_NALU  sei_nalu;
    int ret;

    int* size_field = (int*)(*(&bs->cur));
    u8* cur_tmp = bs->cur;

    /* nalu header */
    xeve_set_nalu(&sei_nalu, XEVE_SEI_NUT, ctx->nalu.nuh_temporal_id);
    xeve_eco_nalu(bs, &sei_nalu);

    /* sei parameter set*/
    ret = xeve_eco_emitsei(ctx, bs);
    xeve_assert_rv(ret == XEVE_OK, XEVE_ERR_INVALID_ARGUMENT);

    /* de-init BSW */
    xeve_bsw_deinit(bs);

    /* write the bitstream size */
    xeve_eco_nal_unit_len(size_field, (int)(bs->cur - cur_tmp) - 4);

    return XEVE_OK;
}


int xeve_check_frame_delay(XEVE_CTX * ctx)
{
    if (ctx->pic_icnt < ctx->frm_rnum)
    {
        return XEVE_OK_OUT_NOT_AVAILABLE;
    }
    return XEVE_OK;
}

int xeve_check_more_frames(XEVE_CTX * ctx)
{
    XEVE_PICO  * pico;

    if(FORCE_OUT(ctx))
    {
        /* pseudo xeve_push() for bumping process ****************/
        ctx->pic_icnt++;
        /**********************************************************/

        for(int i = 0; i < ctx->pico_max_cnt; i++)
        {
            pico = ctx->pico_buf[i];
            if(pico != NULL)
            {
                if(pico->is_used == 1)
                {
                    return XEVE_OK;
                }
            }
        }

        return XEVE_OK_NO_MORE_FRM;
    }

    return XEVE_OK;
}

static void decide_normal_gop(XEVE_CTX * ctx, u32 pic_imcnt)
{
    int i_period, gop_size, pos;
    u32        pic_icnt_b;

    i_period = ctx->param.keyint;
    gop_size = ctx->param.gop_size;

    if (i_period == 0 && pic_imcnt == 0)
    {
        ctx->slice_type = SLICE_I;
        ctx->slice_depth = FRM_DEPTH_0;
        ctx->poc.poc_val = pic_imcnt;
        ctx->poc.prev_doc_offset = 0;
        ctx->poc.prev_poc_val = ctx->poc.poc_val;
        ctx->slice_ref_flag = 1;
    }
    else if ((i_period != 0) && pic_imcnt % i_period == 0 && !ctx->param.closed_gop)
    {
        ctx->slice_type = SLICE_I;
        ctx->slice_depth = FRM_DEPTH_0;
        ctx->poc.poc_val = pic_imcnt;
        ctx->poc.prev_doc_offset = 0;
        ctx->poc.prev_poc_val = ctx->poc.poc_val;
        ctx->slice_ref_flag = 1;
        ctx->ip_cnt += 1;
    }
    else if ((i_period != 0) && ctx->pic_cnt % i_period == 0 && ctx->param.closed_gop)
    {
        ctx->slice_type = SLICE_I;
        ctx->slice_depth = FRM_DEPTH_0;
        ctx->poc.poc_val = ctx->pic_cnt;
        ctx->poc.prev_doc_offset = 0;
        ctx->poc.prev_poc_val = ctx->poc.poc_val;
        ctx->slice_ref_flag = 1;
        ctx->ip_cnt += 1;
    }
    else if (pic_imcnt % gop_size == 0)
    {
        ctx->slice_type = ctx->param.inter_slice_type;
        ctx->slice_ref_flag = 1;
        ctx->slice_depth = FRM_DEPTH_1;
        ctx->poc.poc_val = pic_imcnt;
        ctx->poc.prev_doc_offset = 0;
        ctx->poc.prev_poc_val = ctx->poc.poc_val;
        ctx->slice_ref_flag = 1;
    }
    else
    {
        ctx->slice_type = ctx->param.inter_slice_type;
        if (ctx->param.disable_hgop == 0)
        {
            pos = (pic_imcnt % gop_size) - 1;

            if (ctx->sps.tool_pocs)
            {
                ctx->fn_pocs(ctx, pic_imcnt, gop_size, pos);
            }
            else
            {
                ctx->slice_depth = xeve_tbl_slice_depth[gop_size >> 2][pos];
                int tid = ctx->slice_depth - (ctx->slice_depth > 0);
                xeve_poc_derivation(ctx->sps, tid, &ctx->poc);
            }
            if (!ctx->sps.tool_pocs && gop_size >= 2)
            {
                ctx->slice_ref_flag = (ctx->slice_depth == xeve_tbl_slice_depth[gop_size >> 2][gop_size - 2] ? 0 : 1);
            }
            else
            {
                ctx->slice_ref_flag = 1;
            }
        }
        else
        {
            pos = (pic_imcnt % gop_size) - 1;
            ctx->slice_depth = FRM_DEPTH_2;
            ctx->poc.poc_val = ((pic_imcnt / gop_size) * gop_size) - gop_size + pos + 1;
            ctx->slice_ref_flag = 0;
        }
    }

    ctx->poc.poc_val += ctx->param.closed_gop ? (ctx->ip_cnt - 1) * i_period : 0;

    /* find pico again here */
    ctx->pico_idx = (u8)(ctx->poc.poc_val % ctx->pico_max_cnt);
    ctx->pico = ctx->pico_buf[ctx->pico_idx];
    PIC_ORIG(ctx) = &ctx->pico->pic;
}

static void decide_slice_type(XEVE_CTX * ctx)
{
    u32 pic_imcnt, pic_icnt;
    int i_period, gop_size;
    int force_cnt = 0;
    int ip_pic_cnt, is_aligned_gop;

    ip_pic_cnt = ctx->param.closed_gop && ctx->param.keyint > 0 ? ctx->pic_cnt % ctx->param.keyint : ctx->pic_cnt;
    i_period = ctx->param.keyint;
    gop_size = ctx->param.gop_size;
    pic_icnt = (ip_pic_cnt + ctx->param.bframes);
    pic_imcnt = pic_icnt;
    ctx->pico_idx = pic_icnt % ctx->pico_max_cnt;
    ctx->pico = ctx->pico_buf[ctx->pico_idx];
    PIC_ORIG(ctx) = &ctx->pico->pic;
    is_aligned_gop = ctx->param.closed_gop && i_period > 0 &&
                     ((ip_pic_cnt + gop_size - 1) / gop_size) > ((i_period - 1) / gop_size) ? 0 : 1;

    if(gop_size == 1)
    {
        if (i_period == 1) /* IIII... */
        {
            ctx->slice_type = SLICE_I;
            ctx->slice_depth = FRM_DEPTH_0;
            ctx->poc.poc_val = pic_icnt;
            ctx->slice_ref_flag = 0;
        }
        else /* IPPP... */
        {
            pic_imcnt = (i_period > 0) ? pic_icnt % i_period : pic_icnt;
            if (pic_imcnt == 0)
            {
                ctx->slice_type = SLICE_I;
                ctx->slice_depth = FRM_DEPTH_0;
                ctx->slice_ref_flag = 1;
            }
            else
            {
                ctx->slice_type = ctx->param.inter_slice_type;

                if (ctx->param.disable_hgop == 0)
                {
                    ctx->slice_depth = xeve_tbl_slice_depth_P[ctx->param.ref_pic_gap_length >> 2][(pic_imcnt - 1) % ctx->param.ref_pic_gap_length];
                }
                else
                {
                    ctx->slice_depth = FRM_DEPTH_1;
                }
                ctx->slice_ref_flag = 1;
            }
            ctx->poc.poc_val = (ctx->param.closed_gop && i_period > 0 && (ctx->pic_cnt % i_period) == 0 ?
                                0 : (ctx->param.closed_gop ? ctx->pic_cnt % i_period : ctx->pic_cnt));
        }
    }
    else /* include B Picture (gop_size = 2 or 4 or 8 or 16) */
    {
        if(pic_icnt == gop_size - 1) /* special case when sequence start */
        {
            ctx->slice_type = SLICE_I;
            ctx->slice_depth = FRM_DEPTH_0;
            ctx->poc.poc_val = ctx->param.closed_gop ? ctx->ip_cnt * i_period : 0;
            ctx->poc.prev_doc_offset = 0;
            ctx->poc.prev_poc_val = ctx->poc.poc_val;
            ctx->slice_ref_flag = 1;

            /* find pico again here */
            ctx->pico_idx = (u8)(ctx->poc.poc_val % ctx->pico_max_cnt);
            ctx->pico = ctx->pico_buf[ctx->pico_idx];
            PIC_ORIG(ctx) = &ctx->pico->pic;

            ctx->ip_cnt += 1;
            ctx->force_ignored_cnt = 0;
        }
        else if(ctx->force_slice)
        {
            for(force_cnt = ctx->force_ignored_cnt; force_cnt < gop_size; force_cnt++)
            {
                pic_icnt = (ip_pic_cnt + ctx->param.bframes + force_cnt);
                pic_imcnt = pic_icnt;

                decide_normal_gop(ctx, pic_imcnt);

                if(ctx->poc.poc_val <= (int)ctx->pic_ticnt &&
                   (ctx->param.keyint == 0 || ctx->poc.poc_val < ctx->param.keyint * (ctx->ip_cnt)))
                {
                    break;
                }
            }
            ctx->force_ignored_cnt = force_cnt;
        }
        else if (!is_aligned_gop)
        {
            for (force_cnt = ctx->force_ignored_cnt; force_cnt < gop_size; force_cnt++)
            {
                pic_icnt = (ip_pic_cnt + ctx->param.bframes + force_cnt);
                pic_imcnt = pic_icnt;

                decide_normal_gop(ctx, pic_imcnt);

                if (ctx->poc.poc_val < ctx->param.keyint * (ctx->ip_cnt) &&
                    ctx->poc.poc_val == ctx->pico->pic.imgb->ts[0])
                {
                    break;
                }
            }
            ctx->force_ignored_cnt = force_cnt;
        }
        else /* normal GOP case */
        {
            decide_normal_gop(ctx, pic_imcnt);
        }
    }
    if (ctx->param.disable_hgop == 0 && gop_size > 1)
    {
        ctx->nalu.nuh_temporal_id = ctx->slice_depth - (ctx->slice_depth > 0);
    }
    else
    {
        ctx->nalu.nuh_temporal_id = 0;
    }
    if (ctx->slice_type == SLICE_I && ctx->param.closed_gop)
    {
        ctx->poc.prev_idr_poc = ctx->poc.poc_val;
    }
}

int xeve_pic_prepare(XEVE_CTX * ctx, XEVE_BITB * bitb, XEVE_STAT * stat)
{
    int             ret;
    int             size;

    xeve_assert_rv(PIC_ORIG(ctx) != NULL, XEVE_ERR_UNEXPECTED);

    ctx->qp = (u8)ctx->param.qp;

    PIC_CURR(ctx) = xeve_picman_get_empty_pic(&ctx->rpm, &ret);
    xeve_assert_rv(PIC_CURR(ctx) != NULL, ret);
    ctx->map_refi = PIC_CURR(ctx)->map_refi;
    ctx->map_mv = PIC_CURR(ctx)->map_mv;
    ctx->map_unrefined_mv = PIC_CURR(ctx)->map_unrefined_mv;
    ctx->map_dqp_lah = ctx->pico->sinfo.map_qp_scu;

    PIC_MODE(ctx) = PIC_CURR(ctx);
    if(ctx->pic_dbk == NULL)
    {
        ctx->pic_dbk = xeve_pic_alloc(&ctx->rpm.pa, &ret);
        xeve_assert_rv(ctx->pic_dbk != NULL, ret);
    }

    decide_slice_type(ctx);

    ctx->lcu_cnt = ctx->f_lcu;
    ctx->slice_num = 0;

    if (ctx->tile_cnt == 1 && ctx->param.threads > 1)
    {
        for (u32 i = 0; i < ctx->f_lcu; i++)
        {
            ctx->sync_flag[i] = 0; //Reset the sync flag at the begining of each frame
        }
    }

    if(ctx->slice_type == SLICE_I) ctx->last_intra_poc = ctx->poc.poc_val;

    size = sizeof(s8) * ctx->f_scu * REFP_NUM;
    xeve_mset_x64a(ctx->map_refi, -1, size);
    size = sizeof(s16) * ctx->f_scu * REFP_NUM * MV_D;
    xeve_mset_x64a(ctx->map_mv, 0, size);
    size = sizeof(s16) * ctx->f_scu * REFP_NUM * MV_D;
    xeve_mset_x64a(ctx->map_unrefined_mv, 0, size);

    /* initialize bitstream container */
    xeve_bsw_init(&ctx->bs[0], bitb->addr, bitb->bsize, NULL);
    ctx->bs[0].pdata[1] = &ctx->sbac_enc[0];
    for (int i = 0; i < ctx->param.threads; i++)
    {
        xeve_bsw_init(&ctx->bs[i], ctx->bs[i].beg, bitb->bsize, NULL);
    }

    /* clear map */
    xeve_mset_x64a(ctx->map_scu, 0, sizeof(u32) * ctx->f_scu);
    xeve_mset_x64a(ctx->map_cu_mode, 0, sizeof(u32) * ctx->f_scu);

    xeve_set_active_pps_info(ctx);
    if (ctx->param.rc_type != 0)
    {
        ctx->qp = xeve_rc_get_qp(ctx);
    }

    return XEVE_OK;
}

int xeve_pic_finish(XEVE_CTX *ctx, XEVE_BITB *bitb, XEVE_STAT *stat)
{
    XEVE_IMGB *imgb_o, *imgb_c;
    int        ret;
    int        i, j;

    xeve_mset(stat, 0, sizeof(XEVE_STAT));

    /* adding picture sign */
    if (ctx->param.use_pic_sign)
    {
        XEVE_BSW * bs = &ctx->bs[0];
        XEVE_NALU sei_nalu;
        xeve_set_nalu(&sei_nalu, XEVE_SEI_NUT, ctx->nalu.nuh_temporal_id);

        u8* size_field = bs->cur;
        u8* cur_tmp = bs->cur;

        xeve_eco_nalu(bs, &sei_nalu);

        ret = xeve_eco_sei(ctx, bs);
        xeve_assert_rv(ret == XEVE_OK, ret);

        xeve_bsw_deinit(bs);
        stat->sei_size = (int)(bs->cur - cur_tmp);
        xeve_eco_nal_unit_len(size_field, stat->sei_size - 4);
    }

    /* expand current encoding picture, if needs */
    ctx->fn_picbuf_expand(ctx, PIC_CURR(ctx));

    /* picture buffer management */
    ret = xeve_picman_put_pic(&ctx->rpm, PIC_CURR(ctx), ctx->nalu.nal_unit_type_plus1 - 1 == XEVE_IDR_NUT,
                              ctx->poc.poc_val, ctx->nalu.nuh_temporal_id, 0, ctx->refp,
                              ctx->slice_ref_flag, ctx->sps.tool_rpl, ctx->param.ref_pic_gap_length);

    xeve_assert_rv(ret == XEVE_OK, ret);

    imgb_o = PIC_ORIG(ctx)->imgb;
    xeve_assert(imgb_o != NULL);

    imgb_c = PIC_CURR(ctx)->imgb;
    xeve_assert(imgb_c != NULL);

    /* set stat */
    stat->write = XEVE_BSW_GET_WRITE_BYTE(&ctx->bs[0]);
    stat->nalu_type = (ctx->slice_type == SLICE_I && ctx->param.closed_gop) ? XEVE_IDR_NUT : XEVE_NONIDR_NUT;
    stat->stype = ctx->slice_type;
    stat->fnum = ctx->pic_cnt;
    stat->qp = ctx->sh->qp;
    stat->poc = ctx->poc.poc_val;
    stat->tid = ctx->nalu.nuh_temporal_id;

    for(i = 0; i < 2; i++)
    {
        stat->refpic_num[i] = ctx->rpm.num_refp[i];
        for (j = 0; j < stat->refpic_num[i]; j++)
        {
            stat->refpic[i][j] = ctx->refp[j][i].poc;
        }
    }

    imgb_c->ts[XEVE_TS_PTS] = bitb->ts[XEVE_TS_PTS] = imgb_o->ts[XEVE_TS_PTS];
    if (ctx->ts.frame_delay > 0)
    {
        if (ctx->pic_cnt < ctx->ts.frame_delay)
        {
            imgb_c->ts[XEVE_TS_DTS] = bitb->ts[XEVE_TS_DTS] = ctx->ts.frame_ts[ctx->pic_cnt % XEVE_MAX_INBUF_CNT] + ctx->ts.frame_dealy_time;
        }
        else
        {
            imgb_c->ts[XEVE_TS_DTS] = bitb->ts[XEVE_TS_DTS] = ctx->ts.frame_ts[(ctx->pic_cnt - ctx->ts.frame_delay) % XEVE_MAX_INBUF_CNT];
        }
    }
    else
    {
        imgb_c->ts[XEVE_TS_DTS] = bitb->ts[XEVE_TS_DTS] = ctx->ts.frame_ts[ctx->pic_cnt % XEVE_MAX_INBUF_CNT];
    }

    ctx->pic_cnt++; /* increase picture count */
    ctx->param.f_ifrm = 0; /* clear force-IDR flag */
    ctx->pico->is_used = 0;

    if (ctx->param.rc_type != 0)
    {
        ctx->rcore->real_bits = (stat->write - stat->sei_size) << 3;
    }

    imgb_o->release(imgb_o);
    return XEVE_OK;
}

void xeve_set_nalu(XEVE_NALU * nalu, int nalu_type, int nuh_temporal_id)
{
    nalu->nal_unit_size = 0;
    nalu->forbidden_zero_bit = 0;
    nalu->nal_unit_type_plus1 = nalu_type + 1;
    nalu->nuh_temporal_id = nuh_temporal_id;
    nalu->nuh_reserved_zero_5bits = 0;
    nalu->nuh_extension_flag = 0;
}

void xeve_set_vui(XEVE_CTX * ctx, XEVE_VUI * vui)
{
    vui->aspect_ratio_info_present_flag = ctx->param.aspect_ratio_info_present_flag;
    vui->aspect_ratio_idc = ctx->param.sar;
    vui->sar_width = ctx->param.sar_width;
    vui->sar_height = ctx->param.sar_height;
    vui->overscan_info_present_flag = ctx->param.overscan_info_present_flag;
    vui->overscan_appropriate_flag = ctx->param.overscan_appropriate_flag;
    vui->video_signal_type_present_flag = ctx->param.video_signal_type_present_flag;
    vui->video_format = ctx->param.videoformat;
    vui->video_full_range_flag = ctx->param.range;
    vui->colour_description_present_flag = ctx->param.colour_description_present_flag;
    vui->colour_primaries = ctx->param.colorprim;
    vui->transfer_characteristics = ctx->param.transfer;
    vui->matrix_coefficients = ctx->param.matrix_coefficients;
    vui->chroma_loc_info_present_flag = ctx->param.chroma_loc_info_present_flag;
    vui->chroma_sample_loc_type_top_field = ctx->param.chroma_sample_loc_type_top_field;
    vui->chroma_sample_loc_type_bottom_field = ctx->param.chroma_sample_loc_type_bottom_field;
    vui->neutral_chroma_indication_flag = ctx->param.neutral_chroma_indication_flag;
    vui->field_seq_flag = ctx->param.field_seq_flag;
    vui->timing_info_present_flag = ctx->param.timing_info_present_flag;
    vui->num_units_in_tick = ctx->param.num_units_in_tick;
    vui->time_scale = ctx->param.time_scale;
    vui->fixed_pic_rate_flag = ctx->param.fixed_pic_rate_flag;
    vui->nal_hrd_parameters_present_flag = ctx->param.nal_hrd_parameters_present_flag;
    vui->vcl_hrd_parameters_present_flag = ctx->param.vcl_hrd_parameters_present_flag;
    vui->low_delay_hrd_flag = ctx->param.low_delay_hrd_flag;
    vui->pic_struct_present_flag = ctx->param.pic_struct_present_flag;
    vui->bitstream_restriction_flag = ctx->param.bitstream_restriction_flag;
    vui->motion_vectors_over_pic_boundaries_flag = ctx->param.motion_vectors_over_pic_boundaries_flag;
    vui->max_bytes_per_pic_denom = ctx->param.max_bytes_per_pic_denom;
    vui->max_bits_per_mb_denom = ctx->param.max_bits_per_mb_denom;
    vui->log2_max_mv_length_horizontal = ctx->param.log2_max_mv_length_horizontal;
    vui->log2_max_mv_length_vertical = ctx->param.log2_max_mv_length_vertical;
    vui->num_reorder_pics = ctx->param.num_reorder_pics;
    vui->max_dec_pic_buffering = ctx->param.max_dec_pic_buffering;
    vui->hrd_parameters.cpb_cnt_minus1 = 1;
    vui->hrd_parameters.bit_rate_scale = 1;
    vui->hrd_parameters.cpb_size_scale = 1;
    xeve_mset(&(vui->hrd_parameters.bit_rate_value_minus1), 0, sizeof(int)*NUM_CPB);
    xeve_mset(&(vui->hrd_parameters.cpb_size_value_minus1), 0, sizeof(int)*NUM_CPB);
    xeve_mset(&(vui->hrd_parameters.cbr_flag), 0, sizeof(int)*NUM_CPB);
    vui->hrd_parameters.initial_cpb_removal_delay_length_minus1 = 1;
    vui->hrd_parameters.cpb_removal_delay_length_minus1 = 1;
    vui->hrd_parameters.dpb_output_delay_length_minus1 = 1;
    vui->hrd_parameters.time_offset_length = 1;
}

void xeve_set_sps(XEVE_CTX * ctx, XEVE_SPS * sps)
{
    xeve_mset(sps, 0, sizeof(XEVE_SPS));

    sps->profile_idc = ctx->param.profile;
    sps->level_idc = ctx->param.level_idc * 3;
    sps->pic_width_in_luma_samples = XEVE_ALIGN_VAL(ctx->param.w, 8);
    sps->pic_height_in_luma_samples = XEVE_ALIGN_VAL(ctx->param.h, 8);
    sps->toolset_idc_h = 0;
    sps->toolset_idc_l = 0;
    sps->bit_depth_luma_minus8 = ctx->param.codec_bit_depth - 8;
    sps->bit_depth_chroma_minus8 = ctx->param.codec_bit_depth - 8;
    sps->chroma_format_idc = XEVE_CFI_FROM_CF(XEVE_CS_GET_FORMAT(ctx->param.cs));
    sps->dquant_flag = 0;
    sps->log2_max_pic_order_cnt_lsb_minus4 = POC_LSB_BIT - 4;

    if (ctx->param.bframes > 0)
    {
        sps->max_num_ref_pics = ctx->param.me_ref_num;
    }
    else
    {
        sps->max_num_ref_pics = ctx->param.ref_pic_gap_length;;
    }

    sps->log2_sub_gop_length = (int)(log2(ctx->param.gop_size) + .5);
    sps->sps_max_dec_pic_buffering_minus1 = (int)pow(2.0, sps->log2_sub_gop_length) + sps->max_num_ref_pics - 1;
    sps->log2_ref_pic_gap_length = (int)(log2(ctx->param.ref_pic_gap_length) + .5);
    sps->long_term_ref_pics_flag = 0;
    sps->vui_parameters_present_flag = 0;
    xeve_set_vui(ctx, &(sps->vui_parameters));

    if (ctx->chroma_qp_table_struct.chroma_qp_table_present_flag)
    {
        xeve_copy_chroma_qp_mapping_params(&(sps->chroma_qp_table_struct), &(ctx->chroma_qp_table_struct));
    }

    sps->picture_cropping_flag = ctx->param.picture_cropping_flag;
    if (sps->picture_cropping_flag)
    {
        sps->picture_crop_left_offset = ctx->param.picture_crop_left_offset;
        sps->picture_crop_right_offset = ctx->param.picture_crop_right_offset;
        sps->picture_crop_top_offset = ctx->param.picture_crop_top_offset;
        sps->picture_crop_bottom_offset = ctx->param.picture_crop_bottom_offset;
    }
}

int xeve_set_active_pps_info(XEVE_CTX * ctx)
{
    int active_pps_id = ctx->sh->slice_pic_parameter_set_id;
    xeve_mcpy(&(ctx->pps), &(ctx->pps_array[active_pps_id]), sizeof(XEVE_PPS));

    return XEVE_OK;
}

void xeve_set_pps(XEVE_CTX * ctx, XEVE_PPS * pps)
{
    pps->loop_filter_across_tiles_enabled_flag = 0;
    pps->single_tile_in_pic_flag = 1;
    pps->constrained_intra_pred_flag = ctx->param.constrained_intra_pred;
    pps->cu_qp_delta_enabled_flag = (ctx->param.aq_mode || ctx->param.cutree);

    pps->num_ref_idx_default_active_minus1[REFP_0] = 0;
    pps->num_ref_idx_default_active_minus1[REFP_1] = 0;

    ctx->pps.pps_pic_parameter_set_id = 0;
    xeve_mcpy(&ctx->pps_array[ctx->pps.pps_pic_parameter_set_id], &ctx->pps, sizeof(XEVE_PPS));
}

void xeve_set_sh(XEVE_CTX *ctx, XEVE_SH *sh)
{
    double qp;
    int qp_l_i;
    int qp_c_i;

    const QP_ADAPT_PARAM *qp_adapt_param = ctx->param.bframes == 0 ?
        (ctx->param.keyint == 1 ? xeve_qp_adapt_param_ai : xeve_qp_adapt_param_ld) : xeve_qp_adapt_param_ra;

    sh->slice_type = ctx->slice_type;
    sh->no_output_of_prior_pics_flag = 0;
    sh->deblocking_filter_on = (ctx->param.use_deblock) ? 1 : 0;
    sh->sh_deblock_alpha_offset = ctx->param.deblock_alpha_offset;
    sh->sh_deblock_beta_offset = ctx->param.deblock_beta_offset;
    sh->single_tile_in_slice_flag = 1;
    sh->collocated_from_list_idx = (sh->slice_type == SLICE_P) ? REFP_0 : REFP_1;  // Specifies source (List ID) of the collocated picture, equialent of the collocated_from_l0_flag
    sh->collocated_from_ref_idx = 0;        // Specifies source (RefID_ of the collocated picture, equialent of the collocated_ref_idx
    sh->collocated_mvp_source_list_idx = REFP_0;  // Specifies source (List ID) in collocated pic that provides MV information (Applicability is function of NoBackwardPredFlag)

    /* set lambda */
    qp = XEVE_CLIP3(0, MAX_QUANT, (ctx->param.qp_incread_frame != 0 && (int)(ctx->poc.poc_val) >= ctx->param.qp_incread_frame) ? ctx->qp + 1.0 : ctx->qp);
    sh->dqp = ctx->param.aq_mode != 0;

    if(ctx->param.disable_hgop == 0 && ctx->param.rc_type == 0)
    {
        double dqp_offset;
        int qp_offset;

        qp += qp_adapt_param[ctx->slice_depth].qp_offset_layer;
        dqp_offset = qp * qp_adapt_param[ctx->slice_depth].qp_offset_model_scale + qp_adapt_param[ctx->slice_depth].qp_offset_model_offset + 0.5;

        qp_offset = (int)floor(XEVE_CLIP3(0.0, 3.0, dqp_offset));
        qp += qp_offset;
    }

    sh->qp   = (u8)XEVE_CLIP3(0, MAX_QUANT, qp);
    sh->qp_u_offset = ctx->param.qp_cb_offset;
    sh->qp_v_offset = ctx->param.qp_cr_offset;
    sh->qp_u = (s8)XEVE_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, sh->qp + sh->qp_u_offset);
    sh->qp_v = (s8)XEVE_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, sh->qp + sh->qp_v_offset);

    qp_l_i = sh->qp;
    ctx->lambda[0] = 0.57 * pow(2.0, (qp_l_i - 12.0) / 3.0);
    qp_c_i = ctx->qp_chroma_dynamic[0][sh->qp_u];
    ctx->dist_chroma_weight[0] = pow(2.0, (qp_l_i - qp_c_i) / 3.0);
    qp_c_i = ctx->qp_chroma_dynamic[1][sh->qp_v];
    ctx->dist_chroma_weight[1] = pow(2.0, (qp_l_i - qp_c_i) / 3.0);
    ctx->lambda[1] = ctx->lambda[0] / ctx->dist_chroma_weight[0];
    ctx->lambda[2] = ctx->lambda[0] / ctx->dist_chroma_weight[1];
    ctx->sqrt_lambda[0] = sqrt(ctx->lambda[0]);
    ctx->sqrt_lambda[1] = sqrt(ctx->lambda[1]);
    ctx->sqrt_lambda[2] = sqrt(ctx->lambda[2]);

    ctx->sh->slice_pic_parameter_set_id = 0;
}

int xeve_set_tile_info(XEVE_CTX * ctx)
{
    XEVE_TILE  * tile;
    int          size, f_tile, tidx;

    ctx->tile_cnt = ctx->ts_info.tile_columns * ctx->ts_info.tile_rows;
    f_tile = ctx->ts_info.tile_columns * ctx->ts_info.tile_rows;

    ctx->tile_to_slice_map[0] = 0;
    /* alloc tile information */
    size = sizeof(XEVE_TILE) * f_tile;
    ctx->tile = xeve_malloc(size);
    xeve_assert_rv(ctx->tile, XEVE_ERR_OUT_OF_MEMORY);
    xeve_mset(ctx->tile, 0, size);

    /* update tile information - Tile width, height, First ctb address */
    tidx = 0;
    tile = &ctx->tile[tidx];
    tile->w_ctb = ctx->w_lcu;
    tile->h_ctb = ctx->h_lcu;
    tile->f_ctb = tile->w_ctb * tile->h_ctb;

    return XEVE_OK;
}

int xeve_ready(XEVE_CTX* ctx)
{
    XEVE_CORE* core = NULL;
    int          w, h, ret, i, f_blk;
    s32          size;
    XEVE_FCST* fcst = &ctx->fcst;

    xeve_assert(ctx);
    if (ctx->core[0] == NULL)
    {

        /* set various value */
        for (int i = 0; i < ctx->param.threads; i++)
        {
            core = xeve_core_alloc(ctx->param.chroma_format_idc);
            xeve_assert_gv(core != NULL, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
            ctx->core[i] = core;
        }
    }

    xeve_init_bits_est();

    if (ctx->w == 0)
    {
        w = ctx->w = XEVE_ALIGN_VAL(ctx->param.w, 8);
        h = ctx->h = XEVE_ALIGN_VAL(ctx->param.h, 8);
        ctx->f = w * h;
        if ((ctx->w != ctx->param.w) || (ctx->h != ctx->param.h))
        {
            ctx->param.picture_cropping_flag = 1;
            ctx->param.picture_crop_left_offset = 0;
            ctx->param.picture_crop_right_offset = (ctx->w - ctx->param.w + 1) >> 1;
            ctx->param.picture_crop_top_offset = 0;
            ctx->param.picture_crop_bottom_offset = (ctx->h - ctx->param.h + 1) >> 1;
        }

        ctx->max_cuwh = 64;
        ctx->min_cuwh = 1 << 2;
        ctx->log2_min_cuwh = 2;

        ctx->log2_max_cuwh = XEVE_LOG2(ctx->max_cuwh);
        ctx->max_cud = ctx->log2_max_cuwh - MIN_CU_LOG2;
        ctx->w_lcu = (w + ctx->max_cuwh - 1) >> ctx->log2_max_cuwh;
        ctx->h_lcu = (h + ctx->max_cuwh - 1) >> ctx->log2_max_cuwh;
        ctx->f_lcu = ctx->w_lcu * ctx->h_lcu;
        ctx->w_scu = (w + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
        ctx->h_scu = (h + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
        ctx->f_scu = ctx->w_scu * ctx->h_scu;
        ctx->log2_culine = ctx->log2_max_cuwh - MIN_CU_LOG2;
        ctx->log2_cudim = ctx->log2_culine << 1;
    }

    if (ctx->param.rc_type != 0 || ctx->param.lookahead != 0 || ctx->param.use_fcst != 0)
    {
        xeve_rc_create(ctx);
    }
    else
    {
        ctx->rc = NULL;
        ctx->rcore = NULL;
        ctx->qp = ctx->param.qp;
    }

    //initialize the threads to NULL
    for (int i = 0; i < XEVE_MAX_THREADS; i++)
    {
        ctx->thread_pool[i] = 0;
    }

    //get the context synchronization handle
    ctx->sync_block = get_synchronized_object();
    xeve_assert_gv(ctx->sync_block != NULL, ret, XEVE_ERR_UNKNOWN, ERR);

    if (ctx->param.threads >= 1)
    {
        ctx->tc = xeve_malloc(sizeof(THREAD_CONTROLLER));
        init_thread_controller(ctx->tc, ctx->param.threads);
        for (int i = 0; i < ctx->param.threads; i++)
        {
            ctx->thread_pool[i] = ctx->tc->create(ctx->tc, i);
            xeve_assert_gv(ctx->thread_pool[i] != NULL, ret, XEVE_ERR_UNKNOWN, ERR);
        }
    }

    size = ctx->f_lcu * sizeof(int);
    ctx->sync_flag = (volatile s32 *)xeve_malloc(size);
    xeve_assert_gv( ctx->sync_flag, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    for (int i = 0; i < (int)ctx->f_lcu; i++)
    {
        ctx->sync_flag[i] = 0;
    }

    /*  allocate CU data map*/
    if (ctx->map_cu_data == NULL)
    {
        size = sizeof(XEVE_CU_DATA) * ctx->f_lcu;
        ctx->map_cu_data = (XEVE_CU_DATA*)xeve_malloc_fast(size);
        xeve_assert_gv(ctx->map_cu_data, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset_x64a(ctx->map_cu_data, 0, size);

        for (i = 0; i < (int)ctx->f_lcu; i++)
        {
            xeve_create_cu_data(ctx->map_cu_data + i, ctx->log2_max_cuwh - MIN_CU_LOG2, ctx->log2_max_cuwh - MIN_CU_LOG2, ctx->param.chroma_format_idc);
        }
    }

    /* allocate maps */
    if (ctx->map_scu == NULL)
    {
        size = sizeof(u32) * ctx->f_scu;
        ctx->map_scu = xeve_malloc_fast(size);
        xeve_assert_gv(ctx->map_scu, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset_x64a(ctx->map_scu, 0, size);
    }

    if (ctx->map_ipm == NULL)
    {
        size = sizeof(s8) * ctx->f_scu;
        ctx->map_ipm = xeve_malloc_fast(size);
        xeve_assert_gv(ctx->map_ipm, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset(ctx->map_ipm, -1, size);
    }

    size = sizeof(s8) * ctx->f_scu;
    ctx->map_depth = xeve_malloc_fast(size);
    xeve_assert_gv(ctx->map_depth, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    xeve_mset(ctx->map_depth, -1, size);


    if (ctx->map_cu_mode == NULL)
    {
        size = sizeof(u32) * ctx->f_scu;
        ctx->map_cu_mode = xeve_malloc_fast(size);
        xeve_assert_gv(ctx->map_cu_mode, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset_x64a(ctx->map_cu_mode, 0, size);
    }

    /* initialize reference picture manager */
    ctx->pa.fn_alloc = xeve_pic_alloc;
    ctx->pa.fn_free = xeve_pic_free;
    ctx->pa.w = ctx->w;
    ctx->pa.h = ctx->h;
    ctx->pa.pad_l = PIC_PAD_SIZE_L;
    ctx->pa.pad_c = PIC_PAD_SIZE_L >> ctx->param.cs_h_shift;
    ctx->pa.bit_depth = ctx->param.codec_bit_depth;
    ctx->pic_cnt = 0;
    ctx->pic_icnt = -1;
    ctx->poc.poc_val = 0;
    ctx->pa.chroma_format_idc = ctx->param.chroma_format_idc;

    ret = xeve_picman_init(&ctx->rpm, MAX_PB_SIZE, XEVE_MAX_NUM_REF_PICS, &ctx->pa);
    xeve_assert_g(XEVE_SUCCEEDED(ret), ERR);

    if (ctx->param.gop_size == 1 && ctx->param.keyint != 1) //LD case
    {
        ctx->pico_max_cnt = 2;
    }
    else //RA case
    {
        ctx->pico_max_cnt = XEVE_MAX_INBUF_CNT;
    }

    if (ctx->param.bframes)
    {
        ctx->frm_rnum = ctx->param.use_fcst ? ctx->param.lookahead : ctx->param.bframes + 1;
    }
    else
    {
        ctx->frm_rnum = 0;
    }

    ctx->qp = ctx->param.qp;
    if (ctx->param.use_fcst)
    {
        fcst->log2_fcst_blk_spic = 4; /* 16x16 in half image*/
        fcst->w_blk = (ctx->w/2 + (((1 << (fcst->log2_fcst_blk_spic + 1)) - 1))) >> (fcst->log2_fcst_blk_spic + 1);
        fcst->h_blk = (ctx->h/2 + (((1 << (fcst->log2_fcst_blk_spic + 1)) - 1))) >> (fcst->log2_fcst_blk_spic + 1);
        fcst->f_blk = fcst->w_blk * fcst->h_blk;
    }

    for (i = 0; i < ctx->pico_max_cnt; i++)
    {
        ctx->pico_buf[i] = (XEVE_PICO*)xeve_malloc(sizeof(XEVE_PICO));
        xeve_assert_gv(ctx->pico_buf[i], ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset(ctx->pico_buf[i], 0, sizeof(XEVE_PICO));

        if (ctx->param.use_fcst)
        {
            ctx->pico_buf[i]->spic = xeve_alloc_spic_l(ctx->w, ctx->h);
            xeve_assert_g(ctx->pico_buf[i]->spic != NULL, ERR);

            f_blk = ctx->fcst.f_blk;
            size = sizeof(u8) * f_blk;
            ctx->pico_buf[i]->sinfo.map_pdir = xeve_malloc(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.map_pdir, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

            size = sizeof(u8) * f_blk;
            ctx->pico_buf[i]->sinfo.map_pdir_bi = xeve_malloc(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.map_pdir_bi, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

            size = sizeof(s16) * f_blk * PRED_BI * MV_D;
            ctx->pico_buf[i]->sinfo.map_mv = xeve_malloc(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.map_mv, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

            size = sizeof(s16) * f_blk * PRED_BI * MV_D;
            ctx->pico_buf[i]->sinfo.map_mv_bi = xeve_malloc(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.map_mv_bi, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

            size = sizeof(s16) * f_blk * PRED_BI * MV_D;
            ctx->pico_buf[i]->sinfo.map_mv_pga = xeve_malloc(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.map_mv_pga, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

            size = sizeof(s32) * f_blk * 4;
            ctx->pico_buf[i]->sinfo.map_uni_lcost = xeve_malloc(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.map_uni_lcost, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

            size = sizeof(s32) * f_blk;
            ctx->pico_buf[i]->sinfo.map_bi_lcost = xeve_malloc(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.map_bi_lcost, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

            size = sizeof(s32) * f_blk;
            ctx->pico_buf[i]->sinfo.map_qp_blk = xeve_malloc(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.map_qp_blk, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

            size = sizeof(s8) * ctx->f_scu;
            ctx->pico_buf[i]->sinfo.map_qp_scu = xeve_malloc_fast(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.map_qp_scu, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

            size = sizeof(u16) * f_blk;
            ctx->pico_buf[i]->sinfo.transfer_cost = xeve_malloc(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.transfer_cost, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        }
    }

    /* alloc tile index map in SCU unit */
    if (ctx->map_tidx == NULL)
    {
        size = sizeof(u8) * ctx->f_scu;
        ctx->map_tidx = (u8*)xeve_malloc_fast(size);
        xeve_assert_gv(ctx->map_tidx, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset_x64a(ctx->map_tidx, 0, size);
    }

    if (ctx->tile == NULL)
    {
        ret = ctx->fn_set_tile_info(ctx);
        if (ret != XEVE_OK)
        {
            goto ERR;
        }
    }

    ctx->sh_array = (XEVE_SH*)xeve_malloc(sizeof(XEVE_SH) * ctx->ts_info.num_slice_in_pic);
    xeve_assert_gv(ctx->sh_array, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    xeve_mset(ctx->sh_array, 0, sizeof(XEVE_SH) * ctx->ts_info.num_slice_in_pic);
    ctx->sh = &ctx->sh_array[0];

    ctx->ts.frame_delay = ctx->param.bframes > 0 ? 8 : 0;

    return XEVE_OK;
ERR:
    for (i = 0; i < (int)ctx->f_lcu; i++)
    {
        xeve_delete_cu_data(ctx->map_cu_data + i, ctx->log2_max_cuwh - MIN_CU_LOG2, ctx->log2_max_cuwh - MIN_CU_LOG2);
    }

    xeve_mfree_fast(ctx->map_cu_data);
    xeve_mfree_fast(ctx->map_ipm);
    xeve_mfree_fast(ctx->map_depth);
    xeve_mfree_fast(ctx->map_cu_mode);
    xeve_mfree_fast(ctx->sh_array);
    xeve_mfree(ctx->tile);

    //free the threadpool and created thread if any
    if (ctx->sync_block)
    {
        release_synchornized_object(&ctx->sync_block);
    }

    if (ctx->param.threads >= 1)
    {
        if (ctx->tc)
        {
            //thread controller instance is present
            //terminate the created thread
            for (int i = 0; i < ctx->param.threads; i++)
            {
                if (ctx->thread_pool[i])
                {
                    //valid thread instance
                    ctx->tc->release(&ctx->thread_pool[i]);
                }
            }
            //dinitialize the tc
            dinit_thread_controller(ctx->tc);
            xeve_mfree_fast(ctx->tc);
            ctx->tc = 0;
        }
    }
    xeve_mfree_fast(ctx->map_tidx);
    xeve_mfree_fast((void*)ctx->sync_flag);

    for (i = 0; i < ctx->pico_max_cnt; i++)
    {
        if (ctx->param.use_fcst)
        {
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_pdir);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_pdir_bi);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_mv);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_mv_bi);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_mv_pga);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_uni_lcost);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_bi_lcost);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_qp_blk);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_qp_scu);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.transfer_cost);
            if (ctx->pico_buf[i] != NULL)
                xeve_picbuf_rc_free(ctx->pico_buf[i]->spic);
        }

        xeve_mfree_fast(ctx->pico_buf[i]);
    }

    if (core)
    {
        xeve_core_free(core);
    }

    if (ctx->param.rc_type != 0 || ctx->param.lookahead != 0 || ctx->param.use_fcst != 0)
    {
        xeve_rc_delete(ctx);
    }

    return ret;
}

void xeve_flush(XEVE_CTX * ctx)
{
    int i;
    xeve_assert(ctx);

    xeve_mfree_fast(ctx->map_scu);
    for(i = 0; i < (int)ctx->f_lcu; i++)
    {
        xeve_delete_cu_data(ctx->map_cu_data + i, ctx->log2_max_cuwh - MIN_CU_LOG2, ctx->log2_max_cuwh - MIN_CU_LOG2);
    }
    xeve_mfree_fast(ctx->map_cu_data);
    xeve_mfree_fast(ctx->map_ipm);
    xeve_mfree_fast(ctx->map_depth);
    xeve_mfree_fast(ctx->sh_array);
    xeve_mfree(ctx->tile);
    //release the sync block
    if (ctx->sync_block)
    {
        release_synchornized_object(&ctx->sync_block);
    }

    //Release thread pool controller and created threads
    if (ctx->param.threads >= 1)
    {
        if(ctx->tc)
        {
            //thread controller instance is present
            //terminate the created thread
            for (int i = 0; i < ctx->param.threads; i++)
            {
                if(ctx->thread_pool[i])
                {
                    //valid thread instance
                    ctx->tc->release(&ctx->thread_pool[i]);
                }
            }
            //dinitialize the tc
            dinit_thread_controller(ctx->tc);
            xeve_mfree_fast(ctx->tc);
            ctx->tc = 0;
        }
    }

    xeve_mfree_fast((void*) ctx->sync_flag);

    xeve_mfree_fast(ctx->map_cu_mode);
    xeve_picbuf_free(ctx->pic_dbk);
    xeve_picman_deinit(&ctx->rpm);

    for (int i = 0; i < ctx->param.threads; i++)
    {
        xeve_core_free(ctx->core[i]);
    }

    for (i = 0; i < ctx->pico_max_cnt; i++)
    {
        if (ctx->param.use_fcst)
        {
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_pdir);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_pdir_bi);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_mv);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_mv_bi);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_mv_pga);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_uni_lcost);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_bi_lcost);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_qp_blk);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_qp_scu);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.transfer_cost);
            xeve_picbuf_rc_free(ctx->pico_buf[i]->spic);
        }
        xeve_mfree_fast(ctx->pico_buf[i]);
    }
    xeve_mfree_fast(ctx->map_tidx);

    for(i = 0; i < XEVE_MAX_INBUF_CNT; i++)
    {
        if(ctx->inbuf[i]) ctx->inbuf[i]->release(ctx->inbuf[i]);
    }

    if (ctx->param.rc_type != 0 || ctx->param.lookahead != 0 || ctx->param.use_fcst != 0)
    {
        xeve_rc_delete(ctx);
    }
}

int xeve_picbuf_get_inbuf(XEVE_CTX * ctx, XEVE_IMGB ** imgb)
{
    int i, opt, align[XEVE_IMGB_MAX_PLANE], pad[XEVE_IMGB_MAX_PLANE];

    for(i = 0; i < XEVE_MAX_INBUF_CNT; i++)
    {
        if(ctx->inbuf[i] == NULL)
        {
            opt = XEVE_IMGB_OPT_NONE;

            /* set align value*/
            align[0] = MIN_CU_SIZE;
            align[1] = MIN_CU_SIZE;
            align[2] = MIN_CU_SIZE;

            /* no padding */
            pad[0] = 0;
            pad[1] = 0;
            pad[2] = 0;

            int cs = ctx->param.chroma_format_idc == 0 ? XEVE_CS_YCBCR400_10LE : (ctx->param.chroma_format_idc == 1 ? XEVE_CS_YCBCR420_10LE :
                (ctx->param.chroma_format_idc == 2 ? XEVE_CS_YCBCR422_10LE : XEVE_CS_YCBCR444_10LE));
            *imgb = xeve_imgb_create(ctx->w, ctx->h, cs, opt, pad, align);
            xeve_assert_rv(*imgb != NULL, XEVE_ERR_OUT_OF_MEMORY);

            ctx->inbuf[i] = *imgb;

            (*imgb)->addref(*imgb);
            return XEVE_OK;
        }
        else if(ctx->inbuf[i]->getref(ctx->inbuf[i]) == 1)
        {
            *imgb = ctx->inbuf[i];

            (*imgb)->addref(*imgb);
            return XEVE_OK;
        }
    }

    return XEVE_ERR_UNEXPECTED;
}

int xeve_header(XEVE_CTX * ctx)
{
    int ret = XEVE_OK;

    /* encode parameter sets */
    if (ctx->pic_cnt == 0 || (ctx->slice_type == SLICE_I && ctx->param.closed_gop)) /* if nalu_type is IDR */
    {
        ret = ctx->fn_encode_sps(ctx);
        xeve_assert_rv(ret == XEVE_OK, ret);

        ret = ctx->fn_encode_pps(ctx);
        xeve_assert_rv(ret == XEVE_OK, ret);

        // SEI Command info
        if (ctx->param.sei_cmd_info)
        {
            ret = ctx->fn_encode_sei(ctx);
            xeve_assert_rv(ret == XEVE_OK, ret);
        }
    }

    return ret;
}

static int parse_tile_slice_param(XEVE_CTX* ctx)
{
    XEVE_TS_INFO * ts_info = &ctx->ts_info;
    XEVE_PARAM   * param = &ctx->param;

    ts_info->tile_uniform_spacing_flag = param->tile_uniform_spacing_flag;
    ts_info->tile_columns              = param->tile_columns;
    ts_info->tile_rows                 = param->tile_rows;
    ts_info->num_slice_in_pic          = param->num_slice_in_pic;
    ts_info->arbitrary_slice_flag      = param->arbitrary_slice_flag;

    int num_tiles = ts_info->tile_columns * ts_info->tile_rows;
    if (num_tiles < ts_info->num_slice_in_pic) return XEVE_ERR;
    if (num_tiles > 1)
    {
        if (!ts_info->tile_uniform_spacing_flag)
        {
            ts_info->tile_column_width_array[0] = atoi(strtok(param->tile_column_width_array, " "));
            int j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                ts_info->tile_column_width_array[j++] = atoi(val);
            } while (1);

            ts_info->tile_row_height_array[0] = atoi(strtok(param->tile_row_height_array, " "));
            j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                ts_info->tile_row_height_array[j++] = atoi(val);
            } while (1);
        }

        if (ts_info->num_slice_in_pic == 1)
        {
            ts_info->tile_array_in_slice[0] = 0;
            ts_info->tile_array_in_slice[1] = (ts_info->tile_columns * ts_info->tile_rows) - 1;
            ts_info->num_remaining_tiles_in_slice_minus1[0] = param->num_remaining_tiles_in_slice_minus1[0];
        }
        else /* There are more than one slice in the picture */
        {
            ts_info->tile_array_in_slice[0] = atoi(strtok(param->num_remaining_tiles_in_slice_minus1, " "));
            int j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                ts_info->tile_array_in_slice[j++] = atoi(val);
            } while (1);

            if (ts_info->arbitrary_slice_flag)
            {
                ts_info->num_remaining_tiles_in_slice_minus1[0] = atoi(strtok(param->num_remaining_tiles_in_slice_minus1, " ")) - 1;
                int j = 1;
                do
                {
                    char* val = strtok(NULL, " \r");
                    if (!val)
                        break;
                    ts_info->num_remaining_tiles_in_slice_minus1[j++] = atoi(val) - 1;
                } while (1);
            }
        }
    }

    return XEVE_OK;
}

static void parse_chroma_qp_mapping_table(XEVE_CHROMA_TABLE* chroma_qp_table, XEVE_PARAM * param)
{
    xeve_mset(chroma_qp_table, 0, sizeof(XEVE_CHROMA_TABLE));
    chroma_qp_table->chroma_qp_table_present_flag = param->chroma_qp_table_present_flag;
    if (chroma_qp_table->chroma_qp_table_present_flag)
    {
        chroma_qp_table->num_points_in_qp_table_minus1[0] = atoi(strtok(param->chroma_qp_num_points_in_table, " ")) - 1;
        chroma_qp_table->num_points_in_qp_table_minus1[1] = atoi(strtok(NULL, " \r")) - 1;

        { /* input pivot points */
            chroma_qp_table->delta_qp_in_val_minus1[0][0] = atoi(strtok(param->chroma_qp_delta_in_val_cb, " "));
            int j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                chroma_qp_table->delta_qp_in_val_minus1[0][j++] = atoi(val);
            } while (1);
            if (chroma_qp_table->num_points_in_qp_table_minus1[0] + 1 == j);

            chroma_qp_table->delta_qp_in_val_minus1[1][0] = atoi(strtok(param->chroma_qp_delta_in_val_cr, " "));
            j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                chroma_qp_table->delta_qp_in_val_minus1[1][j++] = atoi(val);
            } while (1);
            assert(chroma_qp_table->num_points_in_qp_table_minus1[1] + 1 == j);
        }
        {/* output pivot points */
            chroma_qp_table->delta_qp_out_val[0][0] = atoi(strtok(param->chroma_qp_delta_out_val_cb, " "));
            int j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                chroma_qp_table->delta_qp_out_val[0][j++] = atoi(val);
            } while (1);
            assert(chroma_qp_table->num_points_in_qp_table_minus1[0] + 1 == j);

            chroma_qp_table->delta_qp_out_val[1][0] = atoi(strtok(param->chroma_qp_delta_out_val_cr, " "));
            j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                chroma_qp_table->delta_qp_out_val[1][j++] = atoi(val);
            } while (1);
            assert(chroma_qp_table->num_points_in_qp_table_minus1[1] + 1 == j);
        }
    }
}

static void parse_chroma_qp_mapping_params(XEVE_CHROMA_TABLE *dst_struct, XEVE_CHROMA_TABLE *src_struct, int bit_depth)
{
    int qp_bd_offset_c = 6 * (bit_depth - 8);
    dst_struct->chroma_qp_table_present_flag = src_struct->chroma_qp_table_present_flag;
    dst_struct->num_points_in_qp_table_minus1[0] = src_struct->num_points_in_qp_table_minus1[0];
    dst_struct->num_points_in_qp_table_minus1[1] = src_struct->num_points_in_qp_table_minus1[1];

    if (dst_struct->chroma_qp_table_present_flag)
    {
        dst_struct->same_qp_table_for_chroma = 1;
        if (src_struct->num_points_in_qp_table_minus1[0] != src_struct->num_points_in_qp_table_minus1[1])
            dst_struct->same_qp_table_for_chroma = 0;
        else
        {
            for (int i = 0; i < src_struct->num_points_in_qp_table_minus1[0]; i++)
            {
                if ((src_struct->delta_qp_in_val_minus1[0][i] != src_struct->delta_qp_in_val_minus1[1][i]) ||
                    (src_struct->delta_qp_out_val[0][i] != src_struct->delta_qp_out_val[1][i]))
                {
                    dst_struct->same_qp_table_for_chroma = 0;
                    break;
                }
            }
        }

        dst_struct->global_offset_flag = (src_struct->delta_qp_in_val_minus1[0][0] > 15 && src_struct->delta_qp_out_val[0][0] > 15) ? 1 : 0;
        if (!dst_struct->same_qp_table_for_chroma)
        {
            dst_struct->global_offset_flag = dst_struct->global_offset_flag && ((src_struct->delta_qp_in_val_minus1[1][0] > 15 && src_struct->delta_qp_out_val[1][0] > 15) ? 1 : 0);
        }

        int start_qp = (dst_struct->global_offset_flag == 1) ? 16 : -qp_bd_offset_c;
        for (int ch = 0; ch < (dst_struct->same_qp_table_for_chroma ? 1 : 2); ch++)
        {
            dst_struct->delta_qp_in_val_minus1[ch][0] = src_struct->delta_qp_in_val_minus1[ch][0] - start_qp;
            dst_struct->delta_qp_out_val[ch][0] = src_struct->delta_qp_out_val[ch][0] - start_qp - dst_struct->delta_qp_in_val_minus1[ch][0];

            for (int k = 1; k <= dst_struct->num_points_in_qp_table_minus1[ch]; k++)
            {
                dst_struct->delta_qp_in_val_minus1[ch][k] = (src_struct->delta_qp_in_val_minus1[ch][k] - src_struct->delta_qp_in_val_minus1[ch][k - 1]) - 1;
                dst_struct->delta_qp_out_val[ch][k] = (src_struct->delta_qp_out_val[ch][k] - src_struct->delta_qp_out_val[ch][k - 1]) - (dst_struct->delta_qp_in_val_minus1[ch][k] + 1);
            }
        }
    }
}

static void tbl_derived_chroma_qp_mapping(XEVE_CTX * ctx, XEVE_CHROMA_TABLE * struct_qp_c, int bit_depth)
{
    int MAX_QP = XEVE_MAX_QP_TABLE_SIZE - 1;
    int qpInVal[XEVE_MAX_QP_TABLE_SIZE_EXT] = { 0 };
    int qpOutVal[XEVE_MAX_QP_TABLE_SIZE_EXT] = { 0 };
    int qp_bd_offset_c = 6 * (bit_depth - 8);
    int startQp = (struct_qp_c->global_offset_flag == 1) ? 16 : -qp_bd_offset_c;

    for (int i = 0; i < (struct_qp_c->same_qp_table_for_chroma ? 1 : 2); i++)
    {
        qpInVal[0] = startQp + struct_qp_c->delta_qp_in_val_minus1[i][0];
        qpOutVal[0] = startQp + struct_qp_c->delta_qp_in_val_minus1[i][0] + struct_qp_c->delta_qp_out_val[i][0];
        for (int j = 1; j <= struct_qp_c->num_points_in_qp_table_minus1[i]; j++)
        {
            qpInVal[j] = qpInVal[j - 1] + struct_qp_c->delta_qp_in_val_minus1[i][j] + 1;
            qpOutVal[j] = qpOutVal[j - 1] + (struct_qp_c->delta_qp_in_val_minus1[i][j] + 1 + struct_qp_c->delta_qp_out_val[i][j]);
        }

        for (int j = 0; j <= struct_qp_c->num_points_in_qp_table_minus1[i]; j++)
        {
            assert(qpInVal[j] >= -qp_bd_offset_c && qpInVal[j]  <= MAX_QP);
            assert(qpOutVal[j] >= -qp_bd_offset_c && qpOutVal[j] <= MAX_QP);
        }

        ctx->qp_chroma_dynamic[i][qpInVal[0]] = XEVE_CLIP3(-qp_bd_offset_c, MAX_QP, qpOutVal[0]);
        for (int k = qpInVal[0] - 1; k >= -qp_bd_offset_c; k--)
        {
            ctx->qp_chroma_dynamic[i][k] = XEVE_CLIP3(-qp_bd_offset_c, MAX_QP, ctx->qp_chroma_dynamic[i][k + 1] - 1);
        }
        for (int j = 0; j < struct_qp_c->num_points_in_qp_table_minus1[i]; j++)
        {
            int sh = (struct_qp_c->delta_qp_in_val_minus1[i][j + 1] + 1) >> 1;
            for (int k = qpInVal[j] + 1, m = 1; k <= qpInVal[j + 1]; k++, m++)
            {
                ctx->qp_chroma_dynamic[i][k] = ctx->qp_chroma_dynamic[i][qpInVal[j]]
                    + ((qpOutVal[j + 1] - qpOutVal[j]) * m + sh) / (struct_qp_c->delta_qp_in_val_minus1[i][j + 1] + 1);
            }
        }
        for (int k = qpInVal[struct_qp_c->num_points_in_qp_table_minus1[i]] + 1; k <= MAX_QP; k++)
        {
            ctx->qp_chroma_dynamic[i][k] = XEVE_CLIP3(-qp_bd_offset_c, MAX_QP, ctx->qp_chroma_dynamic[i][k - 1] + 1);
        }
    }
    if (struct_qp_c->same_qp_table_for_chroma)
    {
        xeve_mcpy(&(ctx->qp_chroma_dynamic[1][-qp_bd_offset_c]), &(ctx->qp_chroma_dynamic[0][-qp_bd_offset_c]), XEVE_MAX_QP_TABLE_SIZE_EXT * sizeof(int));
    }
}


int xeve_set_init_param(XEVE_CTX * ctx, XEVE_PARAM * param)
{
    /* check input parameters */
    xeve_assert_rv(param->w > 0 && param->h > 0, XEVE_ERR_INVALID_ARGUMENT);
    xeve_assert_rv(param->qp >= MIN_QUANT && param->qp <= MAX_QUANT, XEVE_ERR_INVALID_ARGUMENT);
    xeve_assert_rv(param->keyint >= 0 ,XEVE_ERR_INVALID_ARGUMENT);
    xeve_assert_rv(param->threads <= XEVE_MAX_THREADS ,XEVE_ERR_INVALID_ARGUMENT);

    if(param->disable_hgop == 0)
    {
        xeve_assert_rv(param->bframes == 0 || param->bframes == 1 || \
                       param->bframes == 3 || param->bframes == 7 || \
                       param->bframes == 15, XEVE_ERR_INVALID_ARGUMENT);

        if(param->bframes != 0)
        {
            if(!param->closed_gop && param->keyint % (param->bframes + 1) != 0)
            {
                xeve_assert_rv(0, XEVE_ERR_INVALID_ARGUMENT);
            }
        }
    }

    if (param->ref != 0)
    {
        if (param->bframes == 0)
        {
            param->me_ref_num = XEVE_MIN(5, param->ref);
        }
        else
        {
            param->me_ref_num = (param->ref > param->bframes) ? param->bframes : param->ref;
        }
    }

    if (param->ref_pic_gap_length != 0)
    {
        xeve_assert_rv(param->bframes == 0, XEVE_ERR_INVALID_ARGUMENT);
    }

    if (param->bframes == 0)
    {
        if (param->ref_pic_gap_length == 0)
        {
            param->ref_pic_gap_length = 1;
        }
        xeve_assert_rv(param->ref_pic_gap_length == 1 || param->ref_pic_gap_length == 2 || \
                       param->ref_pic_gap_length == 4 || param->ref_pic_gap_length == 8 || \
                       param->ref_pic_gap_length == 16, XEVE_ERR_INVALID_ARGUMENT);
    }

    /* set default encoding parameter */
    param->gop_size          = param->bframes +1;
    param->lookahead         = XEVE_MIN(XEVE_MAX((param->cutree)? param->gop_size : 0, param->lookahead), XEVE_MAX_INBUF_CNT>>1);
    param->use_fcst          = ((param->use_fcst || param->lookahead) && (param->rc_type || param->aq_mode)) ? 1 : 0;
    param->chroma_format_idc = XEVE_CFI_FROM_CF(XEVE_CS_GET_FORMAT(param->cs));
    param->cs_w_shift        = XEVE_GET_CHROMA_W_SHIFT(param->chroma_format_idc);
    param->cs_h_shift        = XEVE_GET_CHROMA_H_SHIFT(param->chroma_format_idc);


    if (param->chroma_qp_table_present_flag)
    {
        XEVE_CHROMA_TABLE tmp_qp_tbl;
        parse_chroma_qp_mapping_table(&tmp_qp_tbl, param);
        parse_chroma_qp_mapping_params(&(ctx->chroma_qp_table_struct), &tmp_qp_tbl, param->codec_bit_depth);
        tbl_derived_chroma_qp_mapping(ctx, &(ctx->chroma_qp_table_struct), param->codec_bit_depth);
    }
    else
    {
        const int * qp_chroma_ajudst = xeve_tbl_qp_chroma_ajudst;
        xeve_mcpy(&(ctx->qp_chroma_dynamic_ext[0][6 *( param->codec_bit_depth - 8)]), qp_chroma_ajudst, XEVE_MAX_QP_TABLE_SIZE * sizeof(int));
        xeve_mcpy(&(ctx->qp_chroma_dynamic_ext[1][6 * (param->codec_bit_depth - 8)]), qp_chroma_ajudst, XEVE_MAX_QP_TABLE_SIZE * sizeof(int));
    }

#if 0
    if (param->vbv_bufsize == 0)
    {
        param->vbv_bufsize = (int)((param->bitrate) * (param->vbv_msec / 1000.0));
    }
#endif

    parse_tile_slice_param(ctx);

    return XEVE_OK;
}

int xeve_param_init(XEVE_PARAM* param)
{
    xeve_mset(param, 0, sizeof(XEVE_PARAM));

    param->profile                    = XEVE_PROFILE_BASELINE;
    param->qp                         = 32;
    param->crf                        = 32;
    param->bframes                    = 15;
    param->codec_bit_depth            = 10;
    param->lookahead                  = 17;
    param->use_deblock                = 1;
    param->threads                    = 1;
    param->rdo_dbk_switch             = 1;
    param->tile_rows                  = 1;
    param->tile_columns               = 1;
    param->num_slice_in_pic           = 1;
    param->use_annexb                 = 1;
    param->qp_max                     = MAX_QUANT;
    param->qp_min                     = MIN_QUANT;

    param->sei_cmd_info               = 1;

    param->sar                        = 0;
    param->videoformat                = 2;
    param->range                      = 0;
    param->colorprim                  = 2;
    param->transfer                   = 2;
    param->matrix_coefficients        = 2;
    param->master_display             = 2;

    param->max_dec_pic_buffering      = 21;
    param->num_reorder_pics           = 21;
    return XEVE_OK;
}

int xeve_init_core_mt(XEVE_CTX * ctx, int tile_num, XEVE_CORE * core, int thread_cnt)
{
    ctx->fn_mode_init_mt(ctx, thread_cnt);

    /********************* Core initialization *****************************/
    ctx->core[thread_cnt]->tile_num = tile_num;
    ctx->core[thread_cnt]->qp_y = core->qp_y;
    ctx->core[thread_cnt]->qp_u = core->qp_u;
    ctx->core[thread_cnt]->qp_v = core->qp_v;
    ctx->sh->qp_prev_eco = ctx->sh->qp;
    ctx->sh->qp_prev_mode = ctx->sh->qp;
    ctx->core[thread_cnt]->dqp_data[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].prev_qp = ctx->sh->qp_prev_mode;
    ctx->core[thread_cnt]->dqp_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].curr_qp = ctx->sh->qp;
    ctx->core[thread_cnt]->dqp_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].prev_qp = ctx->sh->qp;
    ctx->core[thread_cnt]->ctx = ctx;
    ctx->core[thread_cnt]->bs_temp.pdata[1] = &ctx->core[thread_cnt]->s_temp_run;

    return XEVE_OK;
}

int xeve_deblock_mt(void * arg)
{
    XEVE_CORE * core = (XEVE_CORE *)arg;
    XEVE_CTX * ctx = core->ctx;
    int i = core->tile_num;
    ctx->fn_deblock(ctx, PIC_MODE(ctx), i, ctx->pps.loop_filter_across_tiles_enabled_flag, core);
    return XEVE_OK;
}

int xeve_loop_filter(XEVE_CTX * ctx, XEVE_CORE * core)
{
    int ret = XEVE_OK;

    if (ctx->sh->deblocking_filter_on)
    {
#if TRACE_DBF
        XEVE_TRACE_SET(1);
#endif
        for (int is_hor_edge = 0; is_hor_edge <= 1; is_hor_edge++)
        {
            for (u32 i = 0; i < ctx->f_scu; i++)
            {
                MCU_CLR_COD(ctx->map_scu[i]);
            }

            for (ctx->slice_num = 0; ctx->slice_num < ctx->ts_info.num_slice_in_pic; ctx->slice_num++)
            {
                ctx->sh = &ctx->sh_array[ctx->slice_num];
                u16 total_tiles_in_slice = ctx->sh->num_tiles_in_slice;
                THREAD_CONTROLLER * tc;
                int res;
                int i, k = 0;
                tc = ctx->tc;
                int parallel_task = 1;
                int thread_cnt = 0, thread_cnt1 = 0;;
                int task_completed = 0;

                while (total_tiles_in_slice)
                {
                    parallel_task = 1;// (ctx->param.threads > total_tiles_in_slice) ? total_tiles_in_slice : ctx->param.threads;
                    for (thread_cnt = 0; (thread_cnt < parallel_task - 1); thread_cnt++)
                    {
                        i = ctx->sh->tile_order[thread_cnt + task_completed];
                        ctx->core[thread_cnt]->thread_cnt = thread_cnt;
                        ctx->core[thread_cnt]->tile_num = i;
                        ctx->core[thread_cnt]->deblock_is_hor = is_hor_edge;

                        tc->run(ctx->thread_pool[thread_cnt], xeve_deblock_mt, (void*)ctx->core[thread_cnt]);
                    }
                    i = ctx->sh->tile_order[thread_cnt + task_completed];
                    ctx->core[thread_cnt]->thread_cnt = thread_cnt;
                    ctx->core[thread_cnt]->tile_num = i;
                    ctx->core[thread_cnt]->deblock_is_hor = is_hor_edge;

                    xeve_deblock_mt((void*)ctx->core[thread_cnt]);
                    for (thread_cnt1 = 0; thread_cnt1 < parallel_task - 1; thread_cnt1++)
                    {
                        tc->join(ctx->thread_pool[thread_cnt1], &res);
                        if (XEVE_FAILED(res))
                        {
                            ret = res;
                        }
                    }
                    total_tiles_in_slice -= parallel_task;
                    task_completed += parallel_task;
                }
                total_tiles_in_slice = ctx->sh->num_tiles_in_slice;
            }
#if TRACE_DBF
            XEVE_TRACE_SET(0);
#endif
        }
    }

    return ret;
}

void xeve_recon(XEVE_CTX * ctx, XEVE_CORE * core, s16 *coef, pel *pred, int is_coef, int cuw, int cuh, int s_rec, pel *rec, int bit_depth)
{
    xeve_recon_blk(coef, pred, is_coef, cuw, cuh, s_rec, rec, bit_depth);
}

int xeve_param_apply_ppt_baseline(XEVE_PARAM* param, int profile, int preset, int tune)
{
    if (profile != XEVE_PROFILE_BASELINE)
    {
        return XEVE_ERR;
    }

    param->profile = XEVE_PROFILE_BASELINE;

    if (preset == XEVE_PRESET_FAST)
    {
        param->max_cu_intra      = 32;
        param->min_cu_intra      = 4;
        param->max_cu_inter      = 64;
        param->min_cu_inter      = 8;
        param->me_ref_num        = 1;
        param->me_algo           = 1;
        param->me_range          = 32;
        param->me_sub            = 2;
        param->me_sub_pos        = 2;
        param->me_sub_range      = 1;
        param->skip_th           = 0;
        param->merge_num         = 2;
        param->rdoq              = 1;
        param->cabac_refine      = 1;
        param->rdo_dbk_switch    = 0;
    }
    else if (preset == XEVE_PRESET_MEDIUM)
    {
        param->max_cu_intra      = 32;
        param->min_cu_intra      = 4;
        param->max_cu_inter      = 64;
        param->min_cu_inter      = 8;
        param->me_ref_num        = 1;
        param->me_algo           = 1;
        param->me_range          = 64;
        param->me_sub            = 2;
        param->me_sub_pos        = 4;
        param->me_sub_range      = 1;
        param->skip_th           = 0;
        param->merge_num         = 3;
        param->rdoq              = 1;
        param->cabac_refine      = 1;
        param->rdo_dbk_switch    = 0;
    }
    else if (preset == XEVE_PRESET_SLOW)
    {
        param->max_cu_intra      = 32;
        param->min_cu_intra      = 4;
        param->max_cu_inter      = 64;
        param->min_cu_inter      = 8;
        param->me_ref_num        = 1;
        param->me_algo           = 1;
        param->me_range          = 128;
        param->me_sub            = 3;
        param->me_sub_pos        = 4;
        param->me_sub_range      = 2;
        param->skip_th           = 0;
        param->merge_num         = 3;
        param->rdoq              = 1;
        param->cabac_refine      = 1;
        param->rdo_dbk_switch    = 1;
    }
    else if (preset == XEVE_PRESET_PLACEBO)
    {
        param->max_cu_intra      = 64;
        param->min_cu_intra      = 4;
        param->max_cu_inter      = 64;
        param->min_cu_inter      = 4;
        param->me_ref_num        = 2;
        param->me_algo           = 2;
        param->me_range          = 384;
        param->me_sub            = 3;
        param->me_sub_pos        = 8;
        param->me_sub_range      = 3;
        param->skip_th           = 0;
        param->merge_num         = 4;
        param->rdoq              = 1;
        param->cabac_refine      = 1;
        param->rdo_dbk_switch    = 1;
    }
    else
    {
        return XEVE_ERR;
    }

    if (tune != XEVE_TUNE_NONE)
    {
        if (tune == XEVE_TUNE_ZEROLATENCY)
        {
            param->aq_mode = 1;
            param->lookahead = 0;
            param->cutree = 0;
            param->bframes = 0;
            param->me_ref_num = 1;
            param->ref_pic_gap_length = 1;
            param->use_fcst = 1;
            param->inter_slice_type = 1;
        }
        else if (tune == XEVE_TUNE_PSNR)
        {
            param->aq_mode = 0;
        }
        else
        {
            return XEVE_ERR;
        }
    }

    return XEVE_OK;
}


void xeve_param2string(XEVE_PARAM * param, char * sei_buf, int padx, int pady)
{
    int max_n = 200;
    sei_buf += snprintf(sei_buf, max_n, "profile=%d", param->profile);
    sei_buf += snprintf(sei_buf, max_n, " threads=%d", param->threads);
    sei_buf += snprintf(sei_buf, max_n, " input-res=%dx%d", param->w - padx, param->h - pady);
    sei_buf += snprintf(sei_buf, max_n, " fps=%u", param->fps);
    sei_buf += snprintf(sei_buf, max_n, " keyint=%d", param->keyint);
    sei_buf += snprintf(sei_buf, max_n, " color-space=%d", param->cs);
    sei_buf += snprintf(sei_buf, max_n, " rc-type=%s", (param->rc_type == XEVE_RC_ABR) ? "ABR" : (param->rc_type == XEVE_RC_CRF) ? "CRF" : "CQP");

    if (param->rc_type == XEVE_RC_ABR || param->rc_type == XEVE_RC_CRF)
    {
        if (param->rc_type == XEVE_RC_CRF)
            sei_buf += snprintf(sei_buf, max_n, " crf=%df", param->crf);
        else
            sei_buf += snprintf(sei_buf, max_n, " bitrate=%d", param->bitrate);

        if (param->vbv_bufsize)
        {
            sei_buf += snprintf(sei_buf, max_n, "vbv-bufsize=%d", param->vbv_bufsize);
        }
        sei_buf += snprintf(sei_buf, max_n, "use-filler=%d", param->use_filler);
    }
    else if (param->rc_type == XEVE_RC_CQP)
    {
        sei_buf += snprintf(sei_buf, max_n, " qp=%d", param->qp);
        sei_buf += snprintf(sei_buf, max_n, " qp_cb_offset=%d", param->qp_cb_offset);
        sei_buf += snprintf(sei_buf, max_n, " qp_cr_offset=%d", param->qp_cr_offset);
    }

    sei_buf += snprintf(sei_buf, max_n, " info=%d", param->sei_cmd_info);
    sei_buf += snprintf(sei_buf, max_n, " hash=%d", param->use_pic_sign);

    sei_buf += snprintf(sei_buf, max_n, " bframes=%d", param->bframes);
    sei_buf += snprintf(sei_buf, max_n, " aq-mode=%d", param->aq_mode);
    sei_buf += snprintf(sei_buf, max_n, " lookahead=%d", param->lookahead);
    sei_buf += snprintf(sei_buf, max_n, " closed-gop=%d", param->closed_gop);

    sei_buf += snprintf(sei_buf, max_n, " disable-hgop=%d", param->disable_hgop);
    sei_buf += snprintf(sei_buf, max_n, " ref_pic_gap_length=%d", param->ref_pic_gap_length);
    sei_buf += snprintf(sei_buf, max_n, " codec-bit-depth=%d", param->codec_bit_depth);
    sei_buf += snprintf(sei_buf, max_n, " level-idc=%d", param->level_idc);
    sei_buf += snprintf(sei_buf, max_n, " cu-tree=%d", param->cutree);
    sei_buf += snprintf(sei_buf, max_n, " constrained-ip=%d", param->constrained_intra_pred);
    sei_buf += snprintf(sei_buf, max_n, " use-deblock=%d", param->use_deblock);

    sei_buf += snprintf(sei_buf, max_n, " inter-slice-type=%d", param->inter_slice_type);
    sei_buf += snprintf(sei_buf, max_n, " rdo-deblk-switch=%d", param->rdo_dbk_switch);
    sei_buf += snprintf(sei_buf, max_n, " qp-increased-frame=%d", param->qp_incread_frame);
    sei_buf += snprintf(sei_buf, max_n, " forced-idr-frame-flag=%d", param->f_ifrm);
    sei_buf += snprintf(sei_buf, max_n, " qp-increased-frame=%d", param->qp_incread_frame);

    sei_buf += snprintf(sei_buf, max_n, " qp-max=%d qp-min=%d", param->qp_max, param->qp_min);
    sei_buf += snprintf(sei_buf, max_n, " gop-size=%d", param->gop_size);
    sei_buf += snprintf(sei_buf, max_n, " use-fcst=%d", param->use_fcst);
    sei_buf += snprintf(sei_buf, max_n, " chroma-format-idc=%d", param->chroma_format_idc);
    sei_buf += snprintf(sei_buf, max_n, " cs-w-shift=%d cs-h-shift=%d", param->cs_w_shift, param->cs_h_shift);


    sei_buf += snprintf(sei_buf, max_n, " max-cu-intra=%d min-cu-intra=%d max-cu-inter=%d min-cu-inter=%d ", param->max_cu_intra
        , param->min_cu_intra, param->max_cu_inter, param->min_cu_inter);
    sei_buf += snprintf(sei_buf, max_n, " max-num-ref=%d", param->ref);

    sei_buf += snprintf(sei_buf, max_n, " me-ref-num=%d me-algo=%d me-range=%d me-sub=%d me-sub-pos=%d me-sub-range=%d ", param->me_ref_num
        , param->me_algo, param->me_range, param->me_sub, param->me_sub_pos, param->me_sub_range);

    sei_buf += snprintf(sei_buf, max_n, " rdoq=%d", param->rdoq);
    sei_buf += snprintf(sei_buf, max_n, " cabac-refine=%d", param->cabac_refine);
    sei_buf += snprintf(sei_buf, max_n, " intra-block-copy=%d", param->ibc_flag);
    sei_buf += snprintf(sei_buf, max_n, " btt=%d", param->btt);
    sei_buf += snprintf(sei_buf, max_n, " suco=%d", param->suco);
    sei_buf += snprintf(sei_buf, max_n, " amvr=%d", param->tool_amvr);
    sei_buf += snprintf(sei_buf, max_n, " vd=%d", param->tool_mmvd);
    sei_buf += snprintf(sei_buf, max_n, " affine=%d", param->tool_affine);
    sei_buf += snprintf(sei_buf, max_n, " dmvr=%d", param->tool_dmvr);
    sei_buf += snprintf(sei_buf, max_n, " addb=%d", param->tool_addb);
    sei_buf += snprintf(sei_buf, max_n, " alf=%d", param->tool_alf);
    sei_buf += snprintf(sei_buf, max_n, " htdf=%d", param->tool_htdf);
    sei_buf += snprintf(sei_buf, max_n, " admvp=%d", param->tool_admvp);
    sei_buf += snprintf(sei_buf, max_n, " hmvp=%d", param->tool_hmvp);
    sei_buf += snprintf(sei_buf, max_n, " eipd=%d", param->tool_eipd);
    sei_buf += snprintf(sei_buf, max_n, " iqt=%d", param->tool_iqt);
    sei_buf += snprintf(sei_buf, max_n, " cm-init=%d", param->tool_cm_init);
    sei_buf += snprintf(sei_buf, max_n, " adcc=%d", param->tool_adcc);
    sei_buf += snprintf(sei_buf, max_n, " rpl=%d", param->tool_rpl);
    sei_buf += snprintf(sei_buf, max_n, " pocs=%d", param->tool_pocs);
    sei_buf += snprintf(sei_buf, max_n, " ats=%d", param->tool_ats);
    sei_buf += snprintf(sei_buf, max_n, " pocs=%d", param->tool_pocs);
    if (1 == param->use_deblock)
        sei_buf += snprintf(sei_buf, max_n, " deblock-alpha-offset=%d deblock-beta-offset=%d", param->deblock_alpha_offset, param->deblock_beta_offset);
    sei_buf += snprintf(sei_buf, max_n, " dra=%d", param->tool_dra);

    sei_buf += snprintf(sei_buf, max_n, " aspect-ration-info-flag=%d", param->aspect_ratio_info_present_flag);
    if (param->aspect_ratio_info_present_flag)
    {
        sei_buf += snprintf(sei_buf, max_n, " sar=%d", param->sar);
        if (param->sar == EXTENDED_SAR)
            sei_buf += snprintf(sei_buf, max_n, " sar-width : sar-height=%d:%d", param->sar_width, param->sar_height);
    }
    sei_buf += snprintf(sei_buf, max_n, " overscan=%d", param->overscan_info_present_flag);
    if (param->overscan_info_present_flag)
        sei_buf += snprintf(sei_buf, max_n, " overscan-crop=%d", param->overscan_appropriate_flag);
    sei_buf += snprintf(sei_buf, max_n, " videoformat=%d", param->videoformat);
    sei_buf += snprintf(sei_buf, max_n, " range=%d", param->range);
    sei_buf += snprintf(sei_buf, max_n, " colorprim=%d", param->colorprim);
    sei_buf += snprintf(sei_buf, max_n, " transfer=%d", param->transfer);
    sei_buf += snprintf(sei_buf, max_n, " colormatrix=%d", param->matrix_coefficients);
    if (param->master_display)
        sei_buf += snprintf(sei_buf, max_n, " master-display=%d", param->master_display);
    if (param->max_cll)
        sei_buf += snprintf(sei_buf, max_n, " max-content-light-level=%d", param->max_cll);
    sei_buf += snprintf(sei_buf, max_n, " chromaloc=%d", param->chroma_loc_info_present_flag);
    if (param->chroma_loc_info_present_flag)
        sei_buf += snprintf(sei_buf, max_n, " chromaloc-top=%d chromaloc-bottom=%d",
            param->chroma_sample_loc_type_top_field, param->chroma_sample_loc_type_bottom_field);
    sei_buf += snprintf(sei_buf, max_n, " field-seq-flag=%d", param->field_seq_flag);
    sei_buf += snprintf(sei_buf, max_n, " vui-timing-info-flag=%d", param->timing_info_present_flag);
    sei_buf += snprintf(sei_buf, max_n, " fixed-pic-rate-flag=%d", param->fixed_pic_rate_flag);
    sei_buf += snprintf(sei_buf, max_n, " nal-hrd-params-present-flag=%d", param->nal_hrd_parameters_present_flag);
    sei_buf += snprintf(sei_buf, max_n, " vcl-hrd-params-present-flag=%d", param->vcl_hrd_parameters_present_flag);
    sei_buf += snprintf(sei_buf, max_n, " num-reorder-pics=%d", param->num_reorder_pics);

    return;
}


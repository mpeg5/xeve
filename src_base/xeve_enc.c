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

static int ctu_mt_core(void * arg)
{
    assert(arg != NULL);

    XEVE_BSW  * bs;
    XEVE_SH   * sh;
    XEVE_CORE * core = (XEVE_CORE *)arg;
    XEVE_CTX  * ctx = core->ctx;
    int ctb_cnt_in_row, ret;
    bs = &ctx->bs[core->thread_cnt];
    sh = ctx->sh;
    int i = core->tile_num;

    /* CABAC Initialize for each Tile */
    ctx->fn_eco_sbac_reset(GET_SBAC_ENC(bs), ctx->sh->slice_type, ctx->sh->qp, ctx->sps.tool_cm_init);
    ctx->fn_eco_sbac_reset(&core->s_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2], ctx->sh->slice_type, ctx->sh->qp, ctx->sps.tool_cm_init);

    /*Set entry point for each ctu row in the tile*/
    int sp_x_lcu = ctx->tile[core->tile_num].ctba_rs_first % ctx->w_lcu;
    int sp_y_lcu = ctx->tile[core->tile_num].ctba_rs_first / ctx->w_lcu;
    ctb_cnt_in_row = ctx->tile[i].w_ctb; //Total LCUs in the current row
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
        ret = ctx->fn_mode_init_lcu(ctx, core);
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

int xeve_pic(XEVE_CTX * ctx, XEVE_BITB * bitb, XEVE_STAT * stat)
{
    XEVE_CORE     * core;
    XEVE_BSW      * bs;
    XEVE_SH       * sh;
    XEVE_APS      * aps;
    int             ret;
    u32             i;
    int             ctb_cnt_in_tile = 0;
    int             col_bd = 0;
    int             num_slice_in_pic = ctx->param.num_slice_in_pic;
    u8            * tiles_in_slice;
    u16             total_tiles_in_slice;
    int             tile_cnt = 0;
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
        aps = &ctx->aps;

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
        ret = xeve_picman_refp_init(&ctx->rpm, ctx->sps.max_num_ref_pics, ctx->slice_type, ctx->poc.poc_val, ctx->nalu.nuh_temporal_id, ctx->last_intra_poc, ctx->refp);
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
        else if (ctx->sps.tool_mmvd && (ctx->slice_type == SLICE_P))
        {
            sh->mmvd_group_enable_flag = 0;
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
        total_tiles_in_slice = sh->num_tiles_in_slice;
        THREAD_CONTROLLER * tc;
        int res;
        i = 0;
        tc = ctx->tc;
        int parallel_task = 1;
        int thread_cnt = 0, thread_cnt1 = 0;;
        int task_completed = 0;
        int tile_cnt = 0;

        //Code for CTU parallel encoding
        while (total_tiles_in_slice)
        {
            //Limiting parallel task to the number of LCU rows
            i = tiles_in_slice[tile_cnt++];
            int temp_store_total_ctb = ctx->tile[i].f_ctb;
            parallel_task = (ctx->param.threads > ctx->tile[i].h_ctb) ? ctx->tile[i].h_ctb : ctx->param.threads;
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
                tc->run(ctx->thread_pool[thread_cnt], ctu_mt_core, (void*)ctx->core[thread_cnt]);
            }

            ctx->tile[i].qp = ctx->sh->qp;
            ctx->tile[i].qp_prev_eco[0] = ctx->sh->qp;
            ctx->core[0]->tile_idx = i;
            ctx->core[0]->lcu_num = 0;

            xeve_init_core_mt(ctx, i, core, 0);

            ctx->core[0]->thread_cnt = 0;
            ctu_mt_core((void*)ctx->core[0]);

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

    ctx->force_slice = ((ctx->pic_ticnt % gop_size >= ctx->pic_ticnt - pic_cnt + 1) && FORCE_OUT(ctx)) ? 1 : 0;

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
    XEVE_PIC  * pic, * spic;
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
        spic = pico->spic;
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
    return XEVE_OK;
}


void xeve_platform_init_func(XEVE_CTX * ctx)
{
#if X86_SSE
    int check_cpu, support_sse, support_avx, support_avx2;

    check_cpu = xeve_check_cpu_info();
    support_sse  = (check_cpu >> 1) & 1;
    support_avx  = check_cpu & 1;
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
        bs_buf = (u8 *)xeve_malloc(sizeof(u8 *) * (ctx->param.threads - 1) * max_bs_buf_size);
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
    int           i;

    if(FORCE_OUT(ctx))
    {
        /* pseudo xeve_push() for bumping process ****************/
        ctx->pic_icnt++;
        /**********************************************************/

        for(i=0; i<ctx->pico_max_cnt; i++)
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

    return XEVE_OK;
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



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

#include <math.h>
#include "xeve_type.h"

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
    sh = &ctx->sh;
    int i = core->tile_num;

    /* CABAC Initialize for each Tile */
    ctx->fn_eco_sbac_reset(GET_SBAC_ENC(bs), ctx->sh.slice_type, ctx->sh.qp, ctx->sps.tool_cm_init);
    ctx->fn_eco_sbac_reset(&core->s_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2], ctx->sh.slice_type, ctx->sh.qp, ctx->sps.tool_cm_init);

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
    u8            * tiles_in_slice, total_tiles_in_slice, total_tiles_in_slice_copy;
    int             tile_cnt = 0;
    u8            * curr_temp = ctx->bs[0].cur;;
    int             last_intra_poc = INT_MAX;

    for (ctx->slice_num = 0; ctx->slice_num < num_slice_in_pic; ctx->slice_num++)
    {
        if (num_slice_in_pic > 1)
        {
            if (!ctx->param.arbitrary_slice_flag)
            {
                int first_tile_in_slice, last_tile_in_slice, first_tile_col_idx, last_tile_col_idx, delta_tile_idx;
                int w_tile, w_tile_slice, h_tile_slice;

                w_tile = ctx->param.tile_columns;
                first_tile_in_slice = ctx->param.slice_boundary_array[ctx->slice_num * 2];
                last_tile_in_slice = ctx->param.slice_boundary_array[ctx->slice_num * 2 + 1];

                first_tile_col_idx = first_tile_in_slice % w_tile;
                last_tile_col_idx = last_tile_in_slice % w_tile;
                delta_tile_idx = last_tile_in_slice - first_tile_in_slice;

                if (last_tile_in_slice < first_tile_in_slice)
                {
                    if (first_tile_col_idx > last_tile_col_idx)
                    {
                        delta_tile_idx += ctx->tile_cnt + w_tile;
                    }
                    else
                    {
                        delta_tile_idx += ctx->tile_cnt;
                    }
                }
                else if (first_tile_col_idx > last_tile_col_idx)
                {
                    delta_tile_idx += w_tile;
                }

                w_tile_slice = (delta_tile_idx % w_tile) + 1; //Number of tiles in slice width
                h_tile_slice = (delta_tile_idx / w_tile) + 1;
                total_tiles_in_slice = w_tile_slice * h_tile_slice;
                total_tiles_in_slice_copy = total_tiles_in_slice;
                for (u32 k = 0; k < total_tiles_in_slice; k++)
                {
                    ctx->tiles_in_slice[k] = ctx->tile_order[tile_cnt++];
                }
            }
            else
            {
                total_tiles_in_slice = ctx->param.num_remaining_tiles_in_slice_minus1[ctx->slice_num] + 2;
                int bef_tile_num = 0;
                for (int i = 0; i < ctx->slice_num; ++i)
                {
                    bef_tile_num += ctx->param.num_remaining_tiles_in_slice_minus1[i] + 2;
                }
                for (u32 k = 0; k < total_tiles_in_slice; k++)
                {
                    ctx->tiles_in_slice[k] = ctx->param.tile_array_in_slice[bef_tile_num + k];
                }
                total_tiles_in_slice_copy = total_tiles_in_slice;
            }
        }
        else
        {
            if (ctx->param.arbitrary_slice_flag)
            {
                total_tiles_in_slice = ctx->param.num_remaining_tiles_in_slice_minus1[ctx->slice_num] + 2;
                int bef_tile_num = 0;
                for (int i = 0; i < ctx->slice_num; ++i)
                {
                    bef_tile_num += ctx->param.num_remaining_tiles_in_slice_minus1[i] + 2;
                }
                for (u32 k = 0; k < total_tiles_in_slice; k++)
                {
                    ctx->tiles_in_slice[k] = ctx->param.tile_array_in_slice[bef_tile_num + k];
                }
                total_tiles_in_slice_copy = total_tiles_in_slice;
            }
            else
            {
                total_tiles_in_slice = 0;
                for (u32 k = 0; k < ctx->tile_cnt; k++)
                {
                    ctx->tiles_in_slice[total_tiles_in_slice] = k;
                    total_tiles_in_slice++;
                }
                total_tiles_in_slice_copy = total_tiles_in_slice;
            }
        }
        tiles_in_slice = ctx->tiles_in_slice;

        bs = &ctx->bs[0];
        core = ctx->core[0];
        sh = &ctx->sh;
        core->ctx = ctx;

        sh->num_tiles_in_slice = total_tiles_in_slice;
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

            ctx->sh.aps_signaled = -1; // reset stored aps id in tile group header
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
        xeve_set_nalu(&ctx->nalu, ctx->pic_cnt == 0 || (ctx->slice_type == SLICE_I && ctx->param.use_closed_gop) ? XEVE_IDR_NUT : XEVE_NONIDR_NUT, ctx->nalu.nuh_temporal_id);

        core->qp_y = ctx->sh.qp + 6 * ctx->sps.bit_depth_luma_minus8;
        core->qp_u = xeve_qp_chroma_dynamic[0][sh->qp_u] + 6 * ctx->sps.bit_depth_chroma_minus8;
        core->qp_v = xeve_qp_chroma_dynamic[1][sh->qp_v] + 6 * ctx->sps.bit_depth_chroma_minus8;
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

        ctx->sh.qp_prev_eco = ctx->sh.qp;
        ctx->sh.qp_prev_mode = ctx->sh.qp;
        core->dqp_data[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].prev_qp = ctx->sh.qp_prev_mode;
        core->dqp_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].curr_qp = ctx->sh.qp;
        core->dqp_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].prev_qp = ctx->sh.qp;

        /* Tile wise encoding with in a slice */
        u32 k = 0;
        total_tiles_in_slice = total_tiles_in_slice_copy;
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
            parallel_task = (ctx->cdsc.parallel_task_cnt > ctx->tile[i].h_ctb) ? ctx->tile[i].h_ctb : ctx->cdsc.parallel_task_cnt;
            ctx->parallel_rows = parallel_task;
            ctx->tile[i].qp = ctx->sh.qp;

            for (thread_cnt = 1; (thread_cnt < parallel_task); thread_cnt++)
            {
                ctx->tile[i].qp_prev_eco[thread_cnt] = ctx->sh.qp;
                ctx->core[thread_cnt]->tile_idx = i;
                ctx->core[thread_cnt]->x_lcu = ((ctx->tile[core->tile_num].ctba_rs_first) % ctx->w_lcu);               //entry point lcu's x location
                ctx->core[thread_cnt]->y_lcu = ((ctx->tile[core->tile_num].ctba_rs_first) / ctx->w_lcu) + thread_cnt; // entry point lcu's y location
                ctx->core[thread_cnt]->lcu_num = thread_cnt*ctx->tile[i].w_ctb;
                xeve_init_core_mt(ctx, i, core, thread_cnt);

                ctx->core[thread_cnt]->thread_cnt = thread_cnt;
                tc->run(ctx->thread_pool[thread_cnt], ctu_mt_core, (void*)ctx->core[thread_cnt]);
            }

            ctx->tile[i].qp = ctx->sh.qp;
            ctx->tile[i].qp_prev_eco[0] = ctx->sh.qp;
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

        ctx->sh.qp_prev_eco = ctx->sh.qp;

        k = 0;
        total_tiles_in_slice = total_tiles_in_slice_copy;
        while (total_tiles_in_slice)
        {
            int i = tiles_in_slice[k++];
            ctx->tile[i].qp = ctx->sh.qp;
            ctx->tile[i].qp_prev_eco[0] = ctx->sh.qp;
            core->tile_idx = i;
            ctx->fn_eco_sbac_reset(GET_SBAC_ENC(bs), ctx->sh.slice_type, ctx->sh.qp, ctx->sps.tool_cm_init);
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
                core->x_lcu++;
                if (core->x_lcu >= ctx->tile[i].w_ctb + col_bd)
                {
                    core->x_lcu = (ctx->tile[i].ctba_rs_first) % ctx->w_lcu;
                    core->y_lcu++;
                }
                xeve_update_core_loc_param(ctx, core);
                ctb_cnt_in_tile--;
                ctx->lcu_cnt--;
                if (ctb_cnt_in_tile == 0)
                {
                    xeve_eco_tile_end_flag(bs, 1);
                    xeve_sbac_finish(bs);
                    break;
                }
            } //End of LCU encoding loop in a tile
            total_tiles_in_slice--;
            sh->entry_point_offset_minus1[k - 1] = (u32)((bs)->cur - bs_beg.cur - 4 + (4 - (bs->leftbits >> 3)) + (bs_beg.leftbits >> 3) - 1);
        } // End to tile encoding loop in a slice

        total_tiles_in_slice = total_tiles_in_slice_copy;

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
        int* size_field = (int*)(*(&bs->cur));
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
        ctx->sh.qp_prev_eco = ctx->sh.qp;

        /* Tile level encoding for a slice */
        /* Tile wise encoding with in a slice */
        k = 0;
        total_tiles_in_slice = total_tiles_in_slice_copy;
        while (total_tiles_in_slice)
        {
            int i = tiles_in_slice[k++];
            ctx->tile[i].qp = ctx->sh.qp;
            ctx->tile[i].qp_prev_eco[0] = ctx->sh.qp;
            core->tile_idx = i;

            /* CABAC Initialize for each Tile */
            ctx->fn_eco_sbac_reset(GET_SBAC_ENC(bs), ctx->sh.slice_type, ctx->sh.qp, ctx->sps.tool_cm_init);

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

        /* Bit-stream re-writing (END) */
#if TRACE_HLS
        s32 tmp_fp_point2 = ftell(fp_trace);
        fseek(fp_trace, tmp_fp_point, SEEK_SET);
#endif
        ret = ctx->fn_eco_sh(&bs_sh, &ctx->sps, &ctx->pps, sh, ctx->nalu.nal_unit_type_plus1 - 1);
        xeve_assert_rv(ret == XEVE_OK, ret);
#if TRACE_HLS
        fseek(fp_trace, tmp_fp_point2, SEEK_SET);
#endif
        xeve_bsw_deinit(bs);
        *size_field = (int)(bs->cur - cur_tmp) - 4;
        curr_temp = bs->cur;

    }  // End of slice loop
    return XEVE_OK;
}

XEVE xeve_create(XEVE_CDSC * cdsc, int * err)
{
    XEVE_CTX  * ctx;
    int          ret;

#if ENC_DEC_TRACE
#if TRACE_DBF
    fp_trace = fopen("enc_trace_dbf.txt", "w+");
#else
    fp_trace = fopen("enc_trace.txt", "w+");
#endif
#if TRACE_HLS
    XEVE_TRACE_SET(1);
#endif
#endif

    xeve_assert_rv(cdsc->profile == PROFILE_BASELINE, NULL);
    ctx = NULL;

    /* memory allocation for ctx and core structure */
    ctx = (XEVE_CTX*)xeve_ctx_alloc();

    xeve_assert_gv(ctx != NULL, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    xeve_mcpy(&ctx->cdsc, cdsc, sizeof(XEVE_CDSC));

    /* set default value for encoding parameter */
    ret = xeve_set_init_param(cdsc, &ctx->param);
    xeve_assert_g(ret == XEVE_OK, ERR);

    ret = xeve_platform_init(ctx);
    xeve_assert_g(ret == XEVE_OK, ERR);

    ret = xeve_scan_tbl_init();
    xeve_assert_g(ret == XEVE_OK, ERR);

    ret = xeve_create_bs_buf(ctx);
    xeve_assert_g(ret == XEVE_OK, ERR);

    xeve_init_err_scale(cdsc->codec_bit_depth);
    xeve_set_chroma_qp_tbl_loc(cdsc->codec_bit_depth);

    if(ctx->fn_ready != NULL)
    {
        ret = ctx->fn_ready(ctx);
        xeve_assert_g(ret == XEVE_OK, ERR);
    }

    /* set default value for ctx */
    ctx->magic = XEVE_MAGIC_CODE;
    ctx->id = (XEVE)ctx;
    ctx->sh.aps_signaled = -1;

    return (ctx->id);
ERR:
    if(ctx)
    {
        xeve_platform_deinit(ctx);
        xeve_delete_bs_buf(ctx);
        xeve_ctx_free(ctx);
    }
    if(err) *err = ret;
    return NULL;
}

void xeve_delete(XEVE id)
{
    XEVE_CTX * ctx;

    XEVE_ID_TO_CTX_R(id, ctx);

#if ENC_DEC_TRACE
    fclose(fp_trace);
#endif

    if(ctx->fn_flush != NULL)
    {
        ctx->fn_flush(ctx);
    }

    xeve_platform_deinit(ctx);
    xeve_delete_bs_buf(ctx);
    xeve_ctx_free(ctx);

    xeve_scan_tbl_delete();
}

int xeve_encode(XEVE id, XEVE_BITB * bitb, XEVE_STAT * stat)
{
    XEVE_CTX * ctx;

    XEVE_ID_TO_CTX_RV(id, ctx, XEVE_ERR_INVALID_ARGUMENT);
    xeve_assert_rv(ctx->fn_enc, XEVE_ERR_UNEXPECTED);

    /* bumping - check whether input pictures are remaining or not in pico_buf[] */
    if(XEVE_OK_NO_MORE_FRM == xeve_check_more_frames(ctx))
    {
        return XEVE_OK_NO_MORE_FRM;
    }
    if (!FORCE_OUT(ctx))
    {
        if (ctx->param.use_fcst)
        {
            xeve_forecast_fixed_gop(ctx);
        }
    }
    /* store input picture and return if needed */
    if(XEVE_OK_OUT_NOT_AVAILABLE == xeve_check_frame_delay(ctx))
    {
        return XEVE_OK_OUT_NOT_AVAILABLE;
    }

    /* update BSB */
    bitb->err = 0;

    return ctx->fn_enc(ctx, bitb, stat);
}

int xeve_push(XEVE id, XEVE_IMGB * img)
{
    XEVE_CTX * ctx;

    XEVE_ID_TO_CTX_RV(id, ctx, XEVE_ERR_INVALID_ARGUMENT);
    xeve_assert_rv(ctx->fn_push, XEVE_ERR_UNEXPECTED);

    return ctx->fn_push(ctx, img);
}

int xeve_config(XEVE id, int cfg, void * buf, int * size)
{
    XEVE_CTX       * ctx;
    int              t0;
    XEVE_IMGB      * imgb;

    XEVE_ID_TO_CTX_RV(id, ctx, XEVE_ERR_INVALID_ARGUMENT);

    switch(cfg)
    {
        /* set config **********************************************************/
        case XEVE_CFG_SET_FORCE_OUT:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.force_output = (t0) ? 1 : 0;
            /* store total input picture count at this time */
            ctx->pic_ticnt = ctx->pic_icnt;
            break;

        case XEVE_CFG_SET_FINTRA:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.f_ifrm = t0;
            break;
        case XEVE_CFG_SET_QP:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            xeve_assert_rv(t0 >= MIN_QUANT && t0 <= MAX_QUANT, \
                           XEVE_ERR_INVALID_ARGUMENT);
            ctx->param.qp = t0;
            break;
        case XEVE_CFG_SET_FPS:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            xeve_assert_rv(t0 > 0, XEVE_ERR_INVALID_ARGUMENT);
            ctx->param.fps = t0;
            break;
        case XEVE_CFG_SET_I_PERIOD:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            xeve_assert_rv(t0 >= 0, XEVE_ERR_INVALID_ARGUMENT);
            ctx->param.i_period = t0;
            break;
        case XEVE_CFG_SET_QP_MIN:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            xeve_assert_rv(t0 >= MIN_QUANT, XEVE_ERR_INVALID_ARGUMENT);
            ctx->param.qp_min = t0;
            break;
        case XEVE_CFG_SET_QP_MAX:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            xeve_assert_rv(t0 <= MAX_QUANT, XEVE_ERR_INVALID_ARGUMENT);
            ctx->param.qp_max = t0;
            break;
        case XEVE_CFG_SET_USE_DEBLOCK:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.use_deblock = t0;
            break;
        case XEVE_CFG_SET_DEBLOCK_A_OFFSET:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.deblock_alpha_offset = t0;
            break;
        case XEVE_CFG_SET_DEBLOCK_B_OFFSET:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.deblock_beta_offset = t0;
            break;
        case XEVE_CFG_SET_USE_PIC_SIGNATURE:
            ctx->param.use_pic_sign = (*((int *)buf)) ? 1 : 0;
            break;

            /* get config *******************************************************/
        case XEVE_CFG_GET_QP:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.qp;
            break;
        case XEVE_CFG_GET_WIDTH:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.w;
            break;
        case XEVE_CFG_GET_HEIGHT:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.h;
            break;
        case XEVE_CFG_GET_FPS:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.fps;
            break;
        case XEVE_CFG_GET_I_PERIOD:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.i_period;
            break;
        case XEVE_CFG_GET_RECON:
            xeve_assert_rv(*size == sizeof(XEVE_IMGB**), XEVE_ERR_INVALID_ARGUMENT);
            imgb = PIC_CURR(ctx)->imgb;

            if (ctx->sps.picture_cropping_flag)
            {
                int end_comp = ctx->sps.chroma_format_idc ? N_C : Y_C;
                for (int i = 0; i < end_comp; i++)
                {
                    int cs_offset = i == Y_C ? 2 : 1;
                    imgb->x[i] = ctx->sps.picture_crop_left_offset * cs_offset;
                    imgb->y[i] = ctx->sps.picture_crop_top_offset * cs_offset;
                    imgb->h[i] = imgb->ah[i] - (ctx->sps.picture_crop_top_offset + ctx->sps.picture_crop_bottom_offset) * cs_offset;
                    imgb->w[i] = imgb->aw[i] - (ctx->sps.picture_crop_left_offset + ctx->sps.picture_crop_left_offset) * cs_offset;
                }
            }

            *((XEVE_IMGB **)buf) = imgb;
            imgb->addref(imgb);
            break;
        case XEVE_CFG_GET_USE_DEBLOCK:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.use_deblock;
            break;
        case XEVE_CFG_GET_CLOSED_GOP:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.use_closed_gop;
            break;
        case XEVE_CFG_GET_HIERARCHICAL_GOP:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.use_hgop;
            break;
        case XEVE_CFG_GET_DEBLOCK_A_OFFSET:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.deblock_alpha_offset;
            break;
        case XEVE_CFG_GET_DEBLOCK_B_OFFSET:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.deblock_beta_offset;
            break;
        case XEVE_CFG_GET_SUPPORT_PROF:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = PROFILE_BASELINE;
            break;
        default:
            xeve_trace("unknown config value (%d)\n", cfg);
            xeve_assert_rv(0, XEVE_ERR_UNSUPPORTED);
    }

    return XEVE_OK;
}

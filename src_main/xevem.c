/* The copyright in this software is being made available under the BSD
   License, included below. This software may be subject to contributor and
   other third party rights, including patent rights, and no such rights are
   granted under this license.

   Copyright (c) 2020, Samsung Electronics Co., Ltd.
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
#include "xevem_type.h"
#include "xeve_param_parse.h"

static int xevem_eco_tree(XEVE_CTX * ctx, XEVE_CORE * core, int x0, int y0, int cup, int cuw, int cuh, int cud
                       , int next_split, const int parent_split, int* same_layer_split, const int node_idx, const int* parent_split_allow
                       , int qt_depth, int btt_depth, int cu_qp_delta_code, TREE_CONS tree_cons, XEVE_BSW * bs)
{
    int ret;
    s8  split_mode;
    s8  suco_flag = 0;
    int bound;
    int split_mode_child[4] = {NO_SPLIT, NO_SPLIT, NO_SPLIT, NO_SPLIT};
    int split_allow[6];

    core->tree_cons = tree_cons;

    xeve_get_split_mode(&split_mode, cud, cup, cuw, cuh, ctx->max_cuwh, ctx->map_cu_data[core->lcu_num].split_mode);
    xeve_get_suco_flag(&suco_flag, cud, cup, cuw, cuh, ctx->max_cuwh, ctx->map_cu_data[core->lcu_num].suco_flag);

    same_layer_split[node_idx] = split_mode;

    if(ctx->pps.cu_qp_delta_enabled_flag && ctx->sps.dquant_flag)
    {
        if (split_mode == NO_SPLIT && (XEVE_LOG2(cuw) + XEVE_LOG2(cuh) >= ctx->pps.cu_qp_delta_area) && cu_qp_delta_code != 2)
        {
            if (XEVE_LOG2(cuw) == 7 || XEVE_LOG2(cuh) == 7)
            {
                cu_qp_delta_code = 2;
            }
            else
            {
                cu_qp_delta_code = 1;
            }
            core->cu_qp_delta_is_coded = 0;
        }
        else if ((((XEVE_LOG2(cuw) + XEVE_LOG2(cuh) == ctx->pps.cu_qp_delta_area + 1) && (split_mode == SPLIT_TRI_VER || split_mode == SPLIT_TRI_HOR)) ||
            (XEVE_LOG2(cuh) + XEVE_LOG2(cuw) == ctx->pps.cu_qp_delta_area && cu_qp_delta_code != 2)))
        {
            cu_qp_delta_code = 2;
            core->cu_qp_delta_is_coded = 0;
        }
    }

    if(split_mode != NO_SPLIT)
    {
        if(!ctx->sps.sps_btt_flag || ((x0 + cuw <= ctx->w) && (y0 + cuh <= ctx->h)))
        {
            ctx->fn_eco_split_mode(bs, ctx, core, cud, cup, cuw, cuh, ctx->max_cuwh, x0, y0);
        }

        bound = !((x0 + cuw <= ctx->w) && (y0 + cuh <= ctx->h));
        xevem_eco_suco_flag(bs, ctx, core, cud, cup, cuw, cuh, ctx->max_cuwh, split_mode, bound, ctx->log2_max_cuwh);
        XEVE_SPLIT_STRUCT split_struct;
        int suco_order[SPLIT_MAX_PART_COUNT];
        xeve_split_get_part_structure_main(split_mode, x0, y0, cuw, cuh, cup, cud, ctx->log2_culine, &split_struct);

        xeve_split_get_suco_order(suco_flag, split_mode, suco_order);

        split_struct.tree_cons = tree_cons;

        BOOL mode_cons_changed = FALSE;

        if ( ctx->sps.sps_btt_flag && ctx->sps.tool_admvp )
        {
            split_struct.tree_cons.changed = tree_cons.mode_cons == eAll && ctx->sps.chroma_format_idc != 0 && !xeve_is_chroma_split_allowed(cuw, cuh, split_mode);
            mode_cons_changed = xeve_signal_mode_cons(&core->tree_cons ,&split_struct.tree_cons);

            BOOL mode_cons_signal = mode_cons_changed && (ctx->sh->slice_type != SLICE_I) && (xeve_get_mode_cons_by_split(split_mode, cuw, cuh) == eAll) && (ctx->sps.chroma_format_idc == 1);
            if (mode_cons_changed)
            {
                MODE_CONS mode = xeve_derive_mode_cons(ctx, core->lcu_num, cup);
                xeve_set_tree_mode(&split_struct.tree_cons, mode);
            }

            if (mode_cons_signal)
            {

                xeve_get_ctx_some_flags(PEL2SCU(x0), PEL2SCU(y0), cuw, cuh, ctx->w_scu, ctx->map_scu, ctx->map_cu_mode , core->ctx_flags, ctx->sh->slice_type, ctx->sps.tool_cm_init
                                     , ctx->param.ibc_flag, ctx->sps.ibc_log_max_size, ctx->map_tidx);
                xevem_eco_mode_constr(bs, split_struct.tree_cons.mode_cons, core->ctx_flags[CNID_MODE_CONS]);
            }
        }
        else
        {
            split_struct.tree_cons = xeve_get_default_tree_cons();
        }

        for(int part_num = 0; part_num < split_struct.part_count; ++part_num)
        {
            int cur_part_num = suco_order[part_num];
            int sub_cuw = split_struct.width[cur_part_num];
            int sub_cuh = split_struct.height[cur_part_num];
            int x_pos = split_struct.x_pos[cur_part_num];
            int y_pos = split_struct.y_pos[cur_part_num];

            if(x_pos < ctx->w && y_pos < ctx->h)
            {
                ret = xevem_eco_tree(ctx, core, x_pos, y_pos, split_struct.cup[cur_part_num], sub_cuw, sub_cuh, split_struct.cud[cur_part_num], 1, split_mode, split_mode_child
                                      , part_num, split_allow, INC_QT_DEPTH(qt_depth, split_mode), INC_BTT_DEPTH(btt_depth, split_mode, bound), cu_qp_delta_code, split_struct.tree_cons, bs);
                xeve_assert_g(XEVE_SUCCEEDED(ret), ERR);
            }
            core->tree_cons = tree_cons;
        }

        if (mode_cons_changed && !xeve_check_all(split_struct.tree_cons))
        {
            xeve_assert(x0 + cuw <= PIC_ORIG(ctx)->w_l && y0 + cuh <= PIC_ORIG(ctx)->h_l);
            TREE_CONS local_tree_cons = split_struct.tree_cons;
            local_tree_cons.tree_type = TREE_C;
            ret = xevem_eco_unit(ctx, core, x0, y0, cup, cuw, cuh, local_tree_cons, bs);
            core->tree_cons = tree_cons;
        }
    }
    else
    {
        xeve_assert(x0 + cuw <= ctx->w && y0 + cuh <= ctx->h);

        if((cuw > MIN_CU_SIZE || cuh > MIN_CU_SIZE) && next_split && xeve_check_luma(core->tree_cons))
        {
            ctx->fn_eco_split_mode(bs, ctx, core, cud, cup, cuw, cuh, ctx->max_cuwh, x0, y0);
        }
        core->cu_qp_delta_code = cu_qp_delta_code;
        ret = xevem_eco_unit(ctx, core, x0, y0, cup, cuw, cuh, tree_cons, bs);
        xeve_assert_g(XEVE_SUCCEEDED(ret), ERR);
    }

    return XEVE_OK;
ERR:
    return ret;
}

static int xevem_ctu_mt_core(void * arg)
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
        xeve_init_bef_data(core, ctx);

#if GRAB_STAT
        xeve_stat_set_enc_state(TRUE);
#endif

        /* mode decision *************************************************/
        SBAC_LOAD(core->s_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2], *GET_SBAC_ENC(bs));
        core->s_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].is_bitcount = 1;
        ret = ctx->fn_mode_analyze_lcu(ctx, core);
        xeve_assert_rv(ret == XEVE_OK, ret);

        ret = ctx->fn_mode_post_lcu(ctx, core);
        xeve_assert_rv(ret == XEVE_OK, ret)

        ctx->tile[i].qp_prev_eco[core->thread_cnt] = bef_cu_qp;
        if (ctx->param.cabac_refine)
        {
            /* entropy coding ************************************************/
            int split_mode_child[4];
            int split_allow[6] = { 0, 0, 0, 0, 0, 1 };
            ret = xevem_eco_tree(ctx, core, core->x_pel, core->y_pel, 0, ctx->max_cuwh, ctx->max_cuwh, 0, 1, NO_SPLIT
                              , split_mode_child, 0, split_allow, 0, 0, 0, xeve_get_default_tree_cons(), bs);
            bef_cu_qp = ctx->tile[i].qp_prev_eco[core->thread_cnt];
        }
#if GRAB_STAT
        xeve_stat_set_enc_state(FALSE);
        xeve_stat_write_lcu(core->x_pel, core->y_pel, ctx->w, ctx->h, ctx->max_cuwh, ctx->log2_culine, ctx, core, ctx->map_cu_data[core->lcu_num].split_mode, ctx->map_cu_data[core->lcu_num].suco_flag);
#endif
        xeve_assert_rv(ret == XEVE_OK, ret);

        threadsafe_assign(&ctx->sync_flag[core->lcu_num], THREAD_TERMINATED);
        threadsafe_decrement(ctx->sync_block, (volatile s32 *)&ctx->tile[i].f_ctb);

        core->lcu_num = xeve_mt_get_next_ctu_num(ctx, core, ctx->parallel_rows);
        if (core->lcu_num == -1)
            break;
    }
    return XEVE_OK;
}


static int xevem_tile_mt_core(void * arg)
{
    XEVE_CORE * core = (XEVE_CORE *)arg;
    XEVE_CTX  * ctx = core->ctx;
    int i;
    int res, ret;
    int temp_store_total_ctb = ctx->tile[core->tile_idx].f_ctb;
    int parallel_task = ctx->tile_cnt == 1 ? ((ctx->param.threads > ctx->tile[core->tile_idx].h_ctb) ?
                                             ctx->tile[core->tile_idx].h_ctb : ctx->param.threads): 1;
    ctx->parallel_rows = parallel_task;
    ctx->tile[core->tile_idx].qp = ctx->sh->qp;
    for (i = 0; i < ctx->param.threads; i++)
    {
        ctx->tile[core->tile_idx].qp_prev_eco[i] = ctx->sh->qp;
    }

    for (int thread_cnt = 1; thread_cnt < parallel_task; thread_cnt++)
    {
        ctx->core[thread_cnt]->tile_idx = core->tile_idx;
        ctx->core[thread_cnt]->x_lcu = ((ctx->tile[core->tile_num].ctba_rs_first) % ctx->w_lcu);               //entry point lcu's x location
        ctx->core[thread_cnt]->y_lcu = ((ctx->tile[core->tile_num].ctba_rs_first) / ctx->w_lcu) + thread_cnt; // entry point lcu's y location
        ctx->core[thread_cnt]->lcu_num = ctx->core[thread_cnt]->y_lcu * ctx->w_lcu + ctx->core[thread_cnt]->x_lcu;

        xevem_init_core_mt(ctx, core->tile_idx, core, thread_cnt);

        ctx->core[thread_cnt]->thread_cnt = thread_cnt;
        ctx->tc->run(ctx->thread_pool[thread_cnt], xevem_ctu_mt_core, (void*)ctx->core[thread_cnt]);
    }

    core->x_lcu = ((ctx->tile[core->tile_num].ctba_rs_first) % ctx->w_lcu);
    core->y_lcu = ((ctx->tile[core->tile_num].ctba_rs_first) / ctx->w_lcu);
    core->lcu_num = core->y_lcu * ctx->w_lcu + core->x_lcu;

    xevem_ctu_mt_core(arg);

    for (int thread_cnt1 = 1; thread_cnt1 < parallel_task; thread_cnt1++)
    {
        ctx->tc->join(ctx->thread_pool[thread_cnt1], &res);
        if (XEVE_FAILED(res))
        {
            ret = res;
        }
    }

    ctx->tile[core->tile_idx].f_ctb = temp_store_total_ctb;

    return XEVE_OK;
}

int xevem_pic(XEVE_CTX * ctx, XEVE_BITB * bitb, XEVE_STAT * stat)
{
    XEVE_CORE   * core;
    XEVE_BSW     * bs;
    XEVE_SH      * sh;
    XEVE_APS     * aps;
    XEVE_APS_GEN * aps_alf;
    XEVE_APS_GEN * aps_dra;
    int            ret;
    u32            i, j;
    int            split_mode_child[4];
    int            split_allow[6] = { 0, 0, 0, 0, 0, 1 };
    int            ctb_cnt_in_tile = 0;
    int            col_bd = 0;
    int            num_slice_in_pic = ctx->param.num_slice_in_pic;
    u8           * tiles_in_slice;
    u16            total_tiles_in_slice;
    u8           * curr_temp = ctx->bs[0].cur;
    int            tile_cnt = 0;
    int            last_intra_poc = INT_MAX;
    BOOL           aps_counter_reset = FALSE;

    if (ctx->sps.tool_alf)
    {
        aps_alf = &ctx->aps_gen_array[0];
    }
    if (ctx->sps.tool_dra)
    {
        aps_dra = &ctx->aps_gen_array[1];
    }

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
        aps_counter_reset = FALSE;

        if ((int)ctx->poc.poc_val > last_intra_poc)
        {
            last_intra_poc = INT_MAX;
            aps_counter_reset = TRUE;
        }

        if (ctx->slice_type == SLICE_I)
        {
            last_intra_poc = ctx->poc.poc_val;
            ctx->aps_counter = -1;
            aps->aps_id = -1;
            if (ctx->sps.tool_alf)
            {
                aps_alf->aps_id = -1;
            }
            ctx->sh->aps_signaled = -1; // reset stored aps id in tile group header
            ctx->aps_temp = 0;
        }

        if (aps_counter_reset)
        {
            ctx->aps_counter = 0;
        }

        /* Set slice header */
        xevem_set_sh(ctx, sh);

        if (!ctx->sps.tool_rpl)
        {
            /* initialize reference pictures */
            ret = xeve_picman_refp_init(&ctx->rpm, ctx->sps.max_num_ref_pics, ctx->slice_type, ctx->poc.poc_val, ctx->nalu.nuh_temporal_id, ctx->last_intra_poc, ctx->refp);
        }
        else
        {

#if GRAB_STAT
            xeve_stat_set_poc(ctx->poc.poc_val);
#endif
            ret = xeve_picman_rpl_refp_init(ctx, sh);
        }
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
        if (ctx->slice_type != SLICE_I)
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
        total_tiles_in_slice = sh->num_tiles_in_slice;
        THREAD_CONTROLLER * tc;
        int res;
        i = 0;
        tc = ctx->tc;
        int parallel_task = 1;
        int thread_cnt = 0, thread_cnt1 = 0;;
        int task_completed = 0;

        //Code for CTU parallel encoding
        while (total_tiles_in_slice)
        {
            parallel_task = (ctx->param.threads > total_tiles_in_slice) ? total_tiles_in_slice : ctx->param.threads;
            for (thread_cnt = 0; (thread_cnt < parallel_task - 1); thread_cnt++)
            {
                i = tiles_in_slice[thread_cnt + task_completed];

                ctx->tile[i].qp = ctx->sh->qp;
                for (j = 0; j < (u32)ctx->param.threads; j++)
                {
                    ctx->tile[i].qp_prev_eco[j] = ctx->sh->qp;
                }
                ctx->core[thread_cnt]->tile_idx = i;
                xevem_init_core_mt(ctx, i, core, thread_cnt);
                ctx->core[thread_cnt]->thread_cnt = thread_cnt;
                tc->run(ctx->thread_pool[thread_cnt], xevem_tile_mt_core, (void*)ctx->core[thread_cnt]);
            }

            i = tiles_in_slice[thread_cnt + task_completed];
            ctx->tile[i].qp = ctx->sh->qp;
            ctx->core[thread_cnt]->tile_idx = i;

            for (j = 0; j < (u32)ctx->param.threads; j++)
            {
                ctx->tile[i].qp_prev_eco[j] = ctx->sh->qp;
            }

            xevem_init_core_mt(ctx, i, core, thread_cnt);
            ctx->core[thread_cnt]->thread_cnt = thread_cnt;
            xevem_tile_mt_core((void*)ctx->core[thread_cnt]);
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
    }//End of mode decision

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

    core->x_lcu = core->y_lcu = 0;
    core->x_pel = core->y_pel = 0;
    core->lcu_num = 0;
    ctx->lcu_cnt = ctx->f_lcu;
    for (i = 0; i < ctx->f_scu; i++)
    {
        MCU_CLR_COD(ctx->map_scu[i]);
    }

    ctx->fn_loop_filter(ctx, core);

    /* Bit-stream writing (START) */
    for (ctx->slice_num = 0; ctx->slice_num < num_slice_in_pic; ctx->slice_num++)
    {
        ctx->sh = &ctx->sh_array[ctx->slice_num];
        sh = ctx->sh;
        total_tiles_in_slice = sh->num_tiles_in_slice;
        tiles_in_slice = sh->tile_order;

        xeve_bsw_init_slice(&ctx->bs[0], (u8*)curr_temp, bitb->bsize, NULL);

        XEVE_SBAC* t_sbac;
        t_sbac = GET_SBAC_ENC(bs);
        t_sbac->bin_counter = 0;

        unsigned int bin_counts_in_units = 0;
        unsigned int num_bytes_in_units = 0;

        /* Send available APSs */
        int aps_nalu_size = 0;

        /* Encode ALF in APS */
        if ((ctx->sps.tool_alf) && (ctx->sh->alf_on) && (ctx->slice_num == 0))
        {
            if ((aps->alf_aps_param.enable_flag[0]) && (aps->alf_aps_param.temporal_alf_flag == 0))    // ALF is selected, and new ALF was derived for TG
            {
                XEVE_ALF_SLICE_PARAM * aps_data = (XEVE_ALF_SLICE_PARAM *)aps_alf->aps_data;
                aps_alf->aps_id = aps->aps_id;
                xeve_mcpy(aps_data, &(aps->alf_aps_param), sizeof(XEVE_ALF_SLICE_PARAM));

                ret = xevem_encode_aps(ctx, aps_alf);
                xeve_assert_rv(ret == XEVE_OK, ret);
            }
        }

        /* Encode DRA in APS */
        if ((ctx->sps.tool_dra) && aps_dra->signal_flag)
        {
            ret = xevem_encode_aps(ctx, aps_dra);
            xeve_assert_rv(ret == XEVE_OK, ret);

            aps_dra->signal_flag = 0;
        }

        u8* size_field = bs->cur;
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

#if GRAB_STAT
        xeve_stat_set_enc_state(FALSE);
#endif
        /* Tile level encoding for a slice */
        /* Tile wise encoding with in a slice */
        int k = 0;
        total_tiles_in_slice = sh->num_tiles_in_slice;
        while (total_tiles_in_slice)
        {
            int i = tiles_in_slice[k++];
            ctx->tile[i].qp = ctx->sh->qp;
            ctx->tile[i].qp_prev_eco[core->thread_cnt] = ctx->sh->qp;
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
                XEVE_ALF_SLICE_PARAM * alf_slice_param = &(ctx->sh->alf_sh_param);
                if ((alf_slice_param->is_ctb_alf_on) && (sh->alf_on))
                {
                    XEVE_SBAC *sbac;
                    sbac = GET_SBAC_ENC(bs);
                    XEVE_TRACE_COUNTER;
                    XEVE_TRACE_STR("Usage of ALF: ");
                    xeve_sbac_encode_bin((int)(*(alf_slice_param->alf_ctb_flag + core->lcu_num)), sbac, sbac->ctx.alf_ctb_flag, bs);
                    XEVE_TRACE_INT((int)(*(alf_slice_param->alf_ctb_flag + core->lcu_num)));
                    XEVE_TRACE_STR("\n");
                }
                if ((ctx->sh->alfChromaMapSignalled) && (ctx->sh->alf_on))
                {
                    XEVE_SBAC *sbac;
                    sbac = GET_SBAC_ENC(bs);
                    xeve_sbac_encode_bin((int)(*(alf_slice_param->alf_ctb_chroma_flag + core->lcu_num)), sbac, sbac->ctx.alf_ctb_flag, bs);
                }
                if ((ctx->sh->alfChroma2MapSignalled) && (ctx->sh->alf_on))
                {
                    XEVE_SBAC *sbac;
                    sbac = GET_SBAC_ENC(bs);
                    xeve_sbac_encode_bin((int)(*(alf_slice_param->alf_ctb_chroma2_flag + core->lcu_num)), sbac, sbac->ctx.alf_ctb_flag, bs);
                }

                ret = xevem_eco_tree(ctx, core, core->x_pel, core->y_pel, 0, ctx->max_cuwh, ctx->max_cuwh, 0, 1, NO_SPLIT
                                      , split_mode_child, 0, split_allow, 0, 0, 0, xeve_get_default_tree_cons(), bs);
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
                ctx->lcu_cnt--; //To be updated properly in case of multicore

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

        int log2_sub_widthC_subHeightC = 2; // 4:2:0 only, to be updated
        int min_cu_w = ctx->min_cuwh;
        int min_cu_h = ctx->min_cuwh;
        int padded_w = ((ctx->w + min_cu_w - 1) / min_cu_w) * min_cu_w;
        int padded_h = ((ctx->h + min_cu_h - 1) / min_cu_h) * min_cu_h;
        int raw_bits = padded_w * padded_h * ((ctx->sps.bit_depth_luma_minus8 + 8) + (ctx->sps.chroma_format_idc != 0 ? 2 * ((ctx->sps.bit_depth_chroma_minus8 + 8) >> log2_sub_widthC_subHeightC) : 0));
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
        xeve_eco_nal_unit_len(size_field, (int)(bs->cur - cur_tmp) - 4);
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
        /* Bit-stream writing (END) */

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
#if GRAB_STAT
    xeve_stat_init("enc_stat.vtmbmsstats", esu_only_enc, 0, -1, ence_stat_cu);
    enc_stat_header(cdsc->w, cdsc->h);
#endif
    ctx = NULL;

    /* memory allocation for ctx and core structure */
    ctx = (XEVE_CTX*)xevem_ctx_alloc();
    xeve_assert_gv(ctx != NULL, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

    /* set default value for encoding parameter */
    xeve_mcpy(&ctx->param, &(cdsc->param), sizeof(XEVE_PARAM));
    ret = xevem_set_init_param(ctx, &ctx->param);
    xeve_assert_g(ret == XEVE_OK, ERR);

    ret = xevem_platform_init(ctx);
    xeve_assert_g(ret == XEVE_OK, ERR);

    ret = xeve_create_bs_buf(ctx, cdsc->max_bs_buf_size);
    xeve_assert_g(ret == XEVE_OK, ERR);

    xeve_init_err_scale(ctx);

    xeve_split_tbl_init(ctx);
    xeve_set_chroma_qp_tbl_loc(ctx);

    if(ctx->fn_ready != NULL)
    {
        ret = ctx->fn_ready(ctx);
        xeve_assert_g(ret == XEVE_OK, ERR);
    }

    /* set default value for ctx */
    ctx->magic = XEVE_MAGIC_CODE;
    ctx->id = (XEVE)ctx;
    ctx->sh->aps_signaled = -1;
    return (ctx->id);
ERR:
    if(ctx)
    {
        if (cdsc->param.profile)
        {
            xevem_platform_deinit(ctx);
        }
        else
        {
            xeve_platform_deinit(ctx);
        }
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
#if GRAB_STAT
    xeve_stat_finish();
#endif

    if(ctx->fn_flush != NULL)
    {
        ctx->fn_flush(ctx);
    }

    if (ctx->param.profile)
    {
        xevem_platform_deinit(ctx);
    }
    else
    {
        xeve_platform_deinit(ctx);
    }

    xeve_delete_bs_buf(ctx);
    xeve_ctx_free(ctx);
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
    if(!FORCE_OUT(ctx))
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
    XEVE_CTX      * ctx;
    int             t0;
    XEVE_IMGB     * imgb;

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
            xeve_assert_rv(*size == sizeof(char*), XEVE_ERR_INVALID_ARGUMENT);
            if (strpbrk((char*)buf, "/") != NULL)
            {
              sscanf((char*)buf, "%d/%d", &ctx->param.fps.num, &ctx->param.fps.den);
            }
            else if (strpbrk((char*)buf, ".") != NULL)
            {
              float tmp_fps = 0;
              sscanf((char*)buf, "%f", &tmp_fps);
              ctx->param.fps.num = tmp_fps * 10000;
              ctx->param.fps.den = 10000;
            }
            else
            {
              sscanf((char*)buf, "%d", &ctx->param.fps.num);
              ctx->param.fps.den = 1;
            }
            break;
        case XEVE_CFG_SET_BPS:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            xeve_assert_rv(t0 > 0, XEVE_ERR_INVALID_ARGUMENT);
            ctx->param.bitrate = t0;
            break;
        case XEVE_CFG_SET_KEYINT:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            xeve_assert_rv(t0 >= 0, XEVE_ERR_INVALID_ARGUMENT);
            ctx->param.keyint = t0;
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
        case XEVE_CFG_SET_SEI_CMD:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.sei_cmd_info = t0 ? 1 : 0;
            break;
        case XEVE_CFG_SET_USE_PIC_SIGNATURE:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.use_pic_sign = t0? 1 : 0;
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
            xeve_assert_rv(*size == sizeof(char*), XEVE_ERR_INVALID_ARGUMENT);
            sprintf((char*)buf, "%d/%d", ctx->param.fps.num, ctx->param.fps.den);
            break;
        case XEVE_CFG_GET_KEYINT:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.keyint;
            break;
        case XEVE_CFG_GET_RECON:
            xeve_assert_rv(*size == sizeof(XEVE_IMGB**), XEVE_ERR_INVALID_ARGUMENT);
            imgb = PIC_CURR(ctx)->imgb;
            if (ctx->sps.tool_dra)
            {
                XEVE_IMGB * timgb;
                int ret;
                ret = ctx->fn_get_inbuf(ctx, &timgb);
                xeve_assert_rv(XEVE_OK == ret, ret);

                xeve_imgb_cpy(timgb, imgb);
                XEVEM_CTX * mctx = (XEVEM_CTX *)ctx;
                xeve_apply_dra_from_array(ctx, timgb, timgb, mctx->dra_array, ctx->aps_gen_array[1].aps_id, 1);
                imgb = timgb;
                imgb->release(imgb);
            }

            if (ctx->sps.picture_cropping_flag)
            {
                int end_comp = ctx->sps.chroma_format_idc ? N_C : Y_C;
                for (int i = 0; i < end_comp; i++)
                {
                    int cs_offset = i == Y_C ? 2 : 1;
                    imgb->x[i] = ctx->sps.picture_crop_left_offset * cs_offset;
                    imgb->y[i] = ctx->sps.picture_crop_top_offset * cs_offset;
                    imgb->h[i] = imgb->ah[i] - (ctx->sps.picture_crop_top_offset + ctx->sps.picture_crop_bottom_offset) * cs_offset;
                    imgb->w[i] = imgb->aw[i] - (ctx->sps.picture_crop_left_offset + ctx->sps.picture_crop_right_offset) * cs_offset;
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
            *((int *)buf) = ctx->param.closed_gop;
            break;
        case XEVE_CFG_GET_HIERARCHICAL_GOP:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.disable_hgop;
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
            *((int *)buf) = XEVE_PROFILE_MAIN;
            break;
        case XEVE_CFG_GET_BPS:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            if (ctx->rc != NULL)
            {
                *((int*)buf) = (int)ctx->rc->bitrate;
            }
            else
            {
                *((int*)buf) = 0;
            }
            break;
        default:
            xeve_trace("unknown config value (%d)\n", cfg);
            xeve_assert_rv(0, XEVE_ERR_UNSUPPORTED);
    }

    return XEVE_OK;
}

int xeve_param_default(XEVE_PARAM *param)
{
    return xeve_param_init(param);
}

int xeve_param_ppt(XEVE_PARAM* param, int profile, int preset, int tune)
{
    if (preset == XEVE_PRESET_DEFAULT)
    {
        preset = XEVE_PRESET_MEDIUM;
    }

    if (profile == XEVE_PROFILE_BASELINE)
    {
        return xeve_param_apply_ppt_baseline(param, profile, preset, tune);
    }

    if (profile != XEVE_PROFILE_MAIN)
    {
        return XEVE_ERR;
    }

    param->profile = XEVE_PROFILE_MAIN;
    param->ibc_flag = 0;
    param->ibc_search_range_x = 64;
    param->ibc_search_range_y = 64;
    param->ibc_hash_search_flag = 0;
    param->ibc_hash_search_max_cand = 64;
    param->ibc_hash_search_range_4smallblk = 64;
    param->ibc_fast_method = 0x02;
    param->toolset_idc_h = 0x1FFFFF;
    param->toolset_idc_l = 0;
    param->btt = 1;
    param->suco = 1;
    param->framework_cb_max = 7;
    param->framework_cb_min = 2;
    param->framework_cu14_max = 6;
    param->framework_tris_max = 6;
    param->framework_tris_min = 4;
    param->framework_suco_max = 6;
    param->framework_suco_min = 4;
    param->tool_amvr = 1;
    param->tool_mmvd = 1;
    param->tool_affine = 1;
    param->tool_dmvr = 1;
    param->tool_addb = 1;
    param->tool_alf = 1;
    param->tool_htdf = 1;
    param->tool_admvp = 1;
    param->tool_hmvp = 1;
    param->tool_eipd = 1;
    param->tool_iqt = 1;
    param->tool_cm_init = 1;
    param->tool_adcc = 1;
    param->tool_rpl = 1;
    param->tool_pocs = 1;
    param->cu_qp_delta_area = 10;
    param->tool_ats = 1;

    if (preset == XEVE_PRESET_FAST)
    {
        param->max_cu_intra      = 64;
        param->min_cu_intra      = 4;
        param->max_cu_inter      = 64;
        param->min_cu_inter      = 4;
        param->me_ref_num        = 2;
        param->me_algo           = 2;
        param->me_range          = 256;
        param->me_sub            = 3;
        param->me_sub_pos        = 4;
        param->me_sub_range      = 3;
        param->skip_th           = 0;
        param->merge_num         = 4;
        param->rdoq              = 1;
        param->cabac_refine      = 1;
        param->rdo_dbk_switch    = 1;

        param->btt                = 0;
        param->ats_intra_fast     = 1;
        param->me_fast            = 1;

    }
    else if (preset == XEVE_PRESET_MEDIUM)
    {
        param->max_cu_intra      = 128;
        param->min_cu_intra      = 4;
        param->max_cu_inter      = 128;
        param->min_cu_inter      = 4;
        param->me_ref_num        = 2;
        param->me_algo           = 2;
        param->me_range          = 256;
        param->me_sub            = 3;
        param->me_sub_pos        = 4;
        param->me_sub_range      = 3;
        param->skip_th           = 0;
        param->merge_num         = 4;
        param->rdoq              = 1;
        param->cabac_refine      = 1;
        param->rdo_dbk_switch    = 1;

        param->btt                = 1;
        param->framework_cb_max   = 7;
        param->framework_cb_min   = 2;
        param->framework_cu14_max = 0;
        param->framework_tris_max = 4;
        param->framework_tris_min = 5;
        param->ats_intra_fast     = 1;
        param->me_fast            = 0;
    }
    else if (preset == XEVE_PRESET_SLOW)
    {
        param->max_cu_intra      = 128;
        param->min_cu_intra      = 4;
        param->max_cu_inter      = 128;
        param->min_cu_inter      = 4;
        param->me_ref_num        = 2;
        param->me_algo           = 2;
        param->me_range          = 256;
        param->me_sub            = 3;
        param->me_sub_pos        = 4;
        param->me_sub_range      = 3;
        param->skip_th           = 0;
        param->merge_num         = 4;
        param->rdoq              = 1;
        param->cabac_refine      = 1;
        param->rdo_dbk_switch    = 1;

        param->btt                = 1;
        param->framework_cb_max   = 7;
        param->framework_cb_min   = 2;
        param->framework_cu14_max = 5;
        param->framework_tris_max = 5;
        param->framework_tris_min = 4;
        param->ats_intra_fast     = 1;
        param->me_fast            = 0;
    }
    else if (preset == XEVE_PRESET_PLACEBO)
    {
        param->max_cu_intra      = 128;
        param->min_cu_intra      = 4;
        param->max_cu_inter      = 128;
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

        param->btt                = 1;
        param->framework_cb_max   = 7;
        param->framework_cb_min   = 2;
        param->framework_cu14_max = 6;
        param->framework_tris_max = 6;
        param->framework_tris_min = 4;
        param->ats_intra_fast     = 0;
        param->me_fast            = 1;
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
            param->ref_pic_gap_length = 1;
            param->me_ref_num = 1;
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

int xeve_param_check(const XEVE_PARAM* param)
{
    int ret = 0;
    int min_block_size = 4;

    if(param->profile == 0)
    {
        if (param->tool_amvr    == 1) { xeve_trace("AMVR cannot be on in base profile\n"); ret = -1; }
        if (param->tool_mmvd    == 1) { xeve_trace("MMVD cannot be on in base profile\n"); ret = -1; }
        if (param->tool_affine  == 1) { xeve_trace("Affine cannot be on in base profile\n"); ret = -1; }
        if (param->tool_dmvr    == 1) { xeve_trace("DMVR cannot be on in base profile\n"); ret = -1; }
        if (param->tool_admvp   == 1) { xeve_trace("ADMVP cannot be on in base profile\n"); ret = -1; }
        if (param->tool_hmvp    == 1) { xeve_trace("HMVP cannot be on in base profile\n"); ret = -1; }
        if (param->tool_addb    == 1) { xeve_trace("ADDB cannot be on in base profile\n"); ret = -1; }
        if (param->tool_alf     == 1) { xeve_trace("ALF cannot be on in base profile\n"); ret = -1; }
        if (param->tool_htdf    == 1) { xeve_trace("HTDF cannot be on in base profile\n"); ret = -1; }
        if (param->btt          == 1) { xeve_trace("BTT cannot be on in base profile\n"); ret = -1; }
        if (param->suco         == 1) { xeve_trace("SUCO cannot be on in base profile\n"); ret = -1; }
        if (param->tool_eipd    == 1) { xeve_trace("EIPD cannot be on in base profile\n"); ret = -1; }
        if (param->tool_iqt     == 1) { xeve_trace("IQT cannot be on in base profile\n"); ret = -1; }
        if (param->tool_cm_init == 1) { xeve_trace("CM_INIT cannot be on in base profile\n"); ret = -1; }
        if (param->tool_adcc    == 1) { xeve_trace("ADCC cannot be on in base profile\n"); ret = -1; }
        if (param->tool_ats     == 1) { xeve_trace("ATS_INTRA cannot be on in base profile\n"); ret = -1; }
        if (param->ibc_flag     == 1) { xeve_trace("IBC cannot be on in base profile\n"); ret = -1; }
        if (param->tool_rpl     == 1) { xeve_trace("RPL cannot be on in base profile\n"); ret = -1; }
        if (param->tool_pocs    == 1) { xeve_trace("POCS cannot be on in base profile\n"); ret = -1; }
    }
    else
    {
        if (param->tool_admvp   == 0 && param->tool_affine == 1) { xeve_trace("AFFINE cannot be on when ADMVP is off\n"); ret = -1; }
        if (param->tool_admvp   == 0 && param->tool_amvr   == 1) { xeve_trace("AMVR cannot be on when ADMVP is off\n"); ret = -1; }
        if (param->tool_admvp   == 0 && param->tool_dmvr   == 1) { xeve_trace("DMVR cannot be on when ADMVP is off\n"); ret = -1; }
        if (param->tool_admvp   == 0 && param->tool_mmvd   == 1) { xeve_trace("MMVD cannot be on when ADMVP is off\n"); ret = -1; }
        if (param->tool_eipd    == 0 && param->ibc_flag    == 1) { xeve_trace("IBC cannot be on when EIPD is off\n"); ret = -1; }
        if (param->tool_iqt     == 0 && param->tool_ats    == 1) { xeve_trace("ATS cannot be on when IQT is off\n"); ret = -1; }
        if (param->tool_cm_init == 0 && param->tool_adcc   == 1) { xeve_trace("ADCC cannot be on when CM_INIT is off\n"); ret = -1; }
    }

    if (param->btt == 1)
    {
        if (param->framework_cb_max && param->framework_cb_max < 5) { xeve_trace("Maximun Coding Block size cannot be smaller than 5\n"); ret = -1; }
        if (param->framework_cb_max > 7) { xeve_trace("Maximun Coding Block size cannot be greater than 7\n"); ret = -1; }
        if (param->framework_cb_min && param->framework_cb_min < 2) { xeve_trace("Minimum Coding Block size cannot be smaller than 2\n"); ret = -1; }
        if ((param->framework_cb_max || param->framework_cb_min) &&
            param->framework_cb_min > param->framework_cb_max) { xeve_trace("Minimum Coding Block size cannot be greater than Maximum coding Block size\n"); ret = -1; }
        if (param->framework_cu14_max > 6) { xeve_trace("Maximun 1:4 Coding Block size cannot be greater than 6\n"); ret = -1; }
        if ((param->framework_cb_max || param->framework_cu14_max) &&
            param->framework_cu14_max > param->framework_cb_max) { xeve_trace("Maximun 1:4 Coding Block size cannot be greater than Maximum coding Block size\n"); ret = -1; }
        if (param->framework_tris_max > 6) { xeve_trace("Maximun Tri-split Block size be greater than 6\n"); ret = -1; }
        if ((param->framework_tris_max || param->framework_cb_max) &&
            param->framework_tris_max > param->framework_cb_max) { xeve_trace("Maximun Tri-split Block size cannot be greater than Maximum coding Block size\n"); ret = -1; }
        if ((param->framework_tris_min || param->framework_cb_min) &&
            param->framework_tris_min < param->framework_cb_min + 2) { xeve_trace("Maximun Tri-split Block size cannot be smaller than Minimum Coding Block size plus two\n"); ret = -1; }
        if(param->framework_cb_min) min_block_size = 1 << param->framework_cb_min;
        else min_block_size = 8;
    }

    if (param->suco == 1)
    {
        if (param->framework_suco_max > 6) { xeve_trace("Maximun SUCO size cannot be greater than 6\n"); ret = -1; }
        if (param->framework_cb_max && param->framework_suco_max > param->framework_cb_max) { xeve_trace("Maximun SUCO size cannot be greater than Maximum coding Block size\n"); ret = -1; }
        if (param->framework_suco_min < 4) { xeve_trace("Minimun SUCO size cannot be smaller than 4\n"); ret = -1; }
        if (param->framework_cb_min && param->framework_suco_min < param->framework_cb_min) { xeve_trace("Minimun SUCO size cannot be smaller than Minimum coding Block size\n"); ret = -1; }
        if (param->framework_suco_min > param->framework_suco_max) { xeve_trace("Minimum SUCO size cannot be greater than Maximum SUCO size\n"); ret = -1; }
    }

    if (XEVE_CS_GET_FORMAT(param->cs) != XEVE_CF_YCBCR400)
    {
        int pic_m = 2;
        if ((param->w & (pic_m - 1)) != 0) { xeve_trace("Current encoder does not support odd picture width\n"); ret = -1; }
        if ((param->h & (pic_m - 1)) != 0) { xeve_trace("Current encoder does not support odd picture height\n"); ret = -1; }
    }

    return ret;
}

int xeve_param_parse(XEVE_PARAM* param, const char* name, const char* value)
{
    if (!param || !name || !value)
    {
        return XEVE_ERR_INVALID_ARGUMENT;
    }

    int ret = xeve_param_set_val(param, name, value);
    return ret;
}

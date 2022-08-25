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

#include "xevem_type.h"
#include "xevem_mode.h"

typedef int(*LOSSY_ES_FUNC)(XEVE_CU_DATA *, int, double, int, int, int, int, int, int);

void xeve_rdo_bit_cnt_cu_intra_main(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s32 cup, s16 coef[N_C][MAX_CU_DIM])
{
    XEVE_SBAC *sbac = &core->s_temp_run;
    int log2_cuw = core->log2_cuw;
    int log2_cuh = core->log2_cuh;
    int* nnz = core->nnz;

    if(slice_type != SLICE_I && (ctx->sps.tool_admvp == 0 || !(core->log2_cuw <= MIN_CU_LOG2 && core->log2_cuh <= MIN_CU_LOG2))
        && xeve_check_all_preds(core->tree_cons) )
    {
        xeve_sbac_encode_bin(0, sbac, core->s_temp_run.ctx.skip_flag + core->ctx_flags[CNID_SKIP_FLAG], &core->bs_temp); /* skip_flag */
        xeve_eco_pred_mode(&core->bs_temp, MODE_INTRA, core->ctx_flags[CNID_PRED_MODE]);
    }

    if (ctx->fn_rdo_intra_ext != NULL)
    {
        ctx->fn_rdo_intra_ext(ctx, core);
    }

    if (xeve_check_luma(core->tree_cons))
    {
        ctx->fn_mode_rdo_bit_cnt_intra_dir(ctx, core, core->ipm[0]);
    }
    else
    {
        xeve_assert(nnz[Y_C] == 0);
    }

    if (ctx->sps.tool_eipd )
    {
        if (xeve_check_chroma(core->tree_cons) && ctx->sps.chroma_format_idc)
        {
            xevem_eco_intra_dir_c(&core->bs_temp, core->ipm[1], core->ipm[0]);
        }
        else
        {
            xeve_assert(nnz[U_C] == 0);
            xeve_assert(nnz[V_C] == 0);
        }
    }

    if (ctx->pps.cu_qp_delta_enabled_flag)
    {
        core->cu_qp_delta_code = core->dqp_temp_run.cu_qp_delta_code;
        core->cu_qp_delta_is_coded = core->dqp_temp_run.cu_qp_delta_is_coded;
        ctx->tile[core->tile_idx].qp_prev_eco[core->thread_cnt] = core->dqp_temp_run.prev_qp;
    }

    ctx->fn_eco_coef(ctx, core, &core->bs_temp, coef, MODE_INTRA, ctx->pps.cu_qp_delta_enabled_flag, 0, RUN_L | RUN_CB | RUN_CR);

    if (ctx->pps.cu_qp_delta_enabled_flag)
    {
        core->dqp_temp_run.cu_qp_delta_code = core->cu_qp_delta_code;
        core->dqp_temp_run.cu_qp_delta_is_coded = core->cu_qp_delta_is_coded;
        core->dqp_temp_run.prev_qp = ctx->tile[core->tile_idx].qp_prev_eco[core->thread_cnt] ;
        core->dqp_temp_run.curr_qp = core->qp;
    }
}

void xevem_rdo_bit_cnt_intra_ext_c(XEVE_CTX * ctx, XEVE_CORE * core)
{
    if (ctx->sps.tool_eipd)
    {
        xevem_eco_intra_dir_c(&core->bs_temp, core->ipm[1], core->ipm[0]);
    }
}

void xevem_rdo_bit_cnt_intra_ext(XEVE_CTX * ctx, XEVE_CORE * core)
{
    if (((ctx->slice_type == SLICE_I) || xeve_check_only_intra(core->tree_cons))
        && xeve_check_luma(core->tree_cons)
        && ctx->param.ibc_flag && core->log2_cuw <= ctx->sps.ibc_log_max_size && core->log2_cuh <= ctx->sps.ibc_log_max_size)
    {
        xevem_eco_ibc_flag(&core->bs_temp, 0, core->ctx_flags[CNID_IBC_FLAG]);
    }
}

void xeve_rdo_bit_cnt_intra_dir_main(XEVE_CTX * ctx, XEVE_CORE * core, int ipm)
{
    XEVEM_CORE *mcore = (XEVEM_CORE*)core;
    if (ctx->sps.tool_eipd)
    {
        xevem_eco_intra_dir(&core->bs_temp, ipm, core->mpm, mcore->mpm_ext, mcore->pims);
    }
    else
    {
        xeve_eco_intra_dir(&core->bs_temp, ipm, core->mpm_b_list);
    }
}

void xeve_rdo_bit_cnt_cu_skip_main(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s32 cup, int mvp_idx0, int mvp_idx1, int c_num , int tool_mmvd)
{
    XEVEM_CORE *mcore = (XEVEM_CORE*)core;

    if(slice_type != SLICE_I)
    {
        xeve_sbac_encode_bin(1, &core->s_temp_run, core->s_temp_run.ctx.skip_flag + core->ctx_flags[CNID_SKIP_FLAG], &core->bs_temp); /* skip_flag */

        if(tool_mmvd)
        {
            xevem_eco_mmvd_flag(&core->bs_temp, mcore->mmvd_flag);
        }

        if(mcore->mmvd_flag)
        {
            xevem_eco_mmvd_info(&core->bs_temp, c_num, ctx->sh->mmvd_group_enable_flag && !((1 << core->log2_cuw)*(1 << core->log2_cuh) <= NUM_SAMPLES_BLOCK));
        }
        else
        {
            // affine skip mode in rdo
            if(core->cuw >= 8 && core->cuh >= 8 && ctx->sps.tool_affine)
            {
                xeve_sbac_encode_bin(mcore->affine_flag != 0, &core->s_temp_run, core->s_temp_run.ctx.affine_flag + core->ctx_flags[CNID_AFFN_FLAG], &core->bs_temp); /* skip affine_flag */
            }
            if(mcore->affine_flag)
            {
                xevem_eco_affine_mrg_idx(&core->bs_temp, mvp_idx0);
                return;
            }

            if(!ctx->sps.tool_admvp)
            {
                xeve_eco_mvp_idx(&core->bs_temp, mvp_idx0);

                if(slice_type == SLICE_B)
                {
                    xeve_eco_mvp_idx(&core->bs_temp, mvp_idx1);
                }
            }
            else
            {
                xevem_eco_merge_idx(&core->bs_temp, mvp_idx0);
            }
        }
    }
}

void xeve_rdo_bit_cnt_affine_mvp(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s8 refi[REFP_NUM], s16 mvd[REFP_NUM][VER_NUM][MV_D], int pidx, int mvp_idx, int vertex_num)
{
    int refi0, refi1;
    int vertex;
    int b_zero = 1;

    if(pidx != PRED_DIR)
    {
        refi0 = refi[REFP_0];
        refi1 = refi[REFP_1];
        if(IS_INTER_SLICE(slice_type) && REFI_IS_VALID(refi0))
        {
            xevem_eco_affine_mvp_idx( &core->bs_temp, mvp_idx );
            b_zero = 1;
            for(vertex = 0; vertex < vertex_num; vertex++)
            {
                if(mvd[REFP_0][vertex][MV_X] != 0 || mvd[REFP_0][vertex][MV_Y] != 0)
                {
                    b_zero = 0;
                    break;
                }
            }
            xevem_eco_affine_mvd_flag(&core->bs_temp, b_zero, REFP_0);
            if(b_zero == 0)
                for(vertex = 0; vertex < vertex_num; vertex++)
                    xeve_eco_mvd(&core->bs_temp, mvd[REFP_0][vertex]);
        }
        if(slice_type == SLICE_B && REFI_IS_VALID(refi1))
        {
            xevem_eco_affine_mvp_idx( &core->bs_temp, mvp_idx );
            b_zero = 1;
            for(vertex = 0; vertex < vertex_num; vertex++)
            {
                if(mvd[REFP_1][vertex][MV_X] != 0 || mvd[REFP_1][vertex][MV_Y] != 0)
                {
                    b_zero = 0;
                    break;
                }
            }
            xevem_eco_affine_mvd_flag(&core->bs_temp, b_zero, REFP_1);
            if(b_zero == 0)
                for(vertex = 0; vertex < vertex_num; vertex++)
                    xeve_eco_mvd(&core->bs_temp, mvd[REFP_1][vertex]);
        }
    }
}

void xeve_rdo_bit_cnt_cu_ibc(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s32 cup, s16 mvd[MV_D], s16 coef[N_C][MAX_CU_DIM], u8 mvp_idx, u8 ibc_flag)
{
    int b_no_cbf = 0;

    if (slice_type != SLICE_I && xeve_check_all_preds(core->tree_cons) )
    {
        xeve_sbac_encode_bin(0, &core->s_temp_run, core->s_temp_run.ctx.skip_flag + core->ctx_flags[CNID_SKIP_FLAG], &core->bs_temp); /* skip_flag */
        xeve_eco_pred_mode(&core->bs_temp, MODE_INTER, core->ctx_flags[CNID_PRED_MODE]);
    }

    if ((!(core->skip_flag == 1 && slice_type == SLICE_I) || xeve_check_only_intra(core->tree_cons) ) && xeve_check_luma(core->tree_cons) )
    {
        xevem_eco_ibc_flag(&core->bs_temp, ibc_flag, core->ctx_flags[CNID_IBC_FLAG]);
    }

    xeve_eco_mvd(&core->bs_temp, mvd);
    if (ctx->pps.cu_qp_delta_enabled_flag)
    {
        core->cu_qp_delta_code = core->dqp_temp_run.cu_qp_delta_code;
        core->cu_qp_delta_is_coded = core->dqp_temp_run.cu_qp_delta_is_coded;
        ctx->tile[core->tile_idx].qp_prev_eco[core->thread_cnt]  = core->dqp_temp_run.prev_qp;
    }

    ctx->fn_eco_coef(ctx, core, &core->bs_temp, coef, MODE_IBC, ctx->pps.cu_qp_delta_enabled_flag, b_no_cbf, RUN_L | RUN_CB | RUN_CR);

    if (ctx->pps.cu_qp_delta_enabled_flag)
    {
        core->dqp_temp_run.cu_qp_delta_code = core->cu_qp_delta_code;
        core->dqp_temp_run.cu_qp_delta_is_coded = core->cu_qp_delta_is_coded;
        core->dqp_temp_run.prev_qp = ctx->tile[core->tile_idx].qp_prev_eco[core->thread_cnt] ;
        core->dqp_temp_run.curr_qp = core->qp;
    }
}

void xeve_rdo_bit_cnt_cu_inter_main(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s32 cup, s8 refi[REFP_NUM], s16 mvd[REFP_NUM][MV_D], s16 coef[N_C][MAX_CU_DIM]
                                  , int pidx, u8 * mvp_idx, u8 mvr_idx, u8 bi_idx, s16 affine_mvd[REFP_NUM][VER_NUM][MV_D])
{
    XEVEM_CORE *mcore = (XEVEM_CORE*)core;
    int refi0, refi1;
    int vertex = 0;
    int vertex_num = mcore->affine_flag + 1;
    XEVE_PINTER *pi = &ctx->pinter[core->thread_cnt];

    int b_no_cbf = 0;
    b_no_cbf |= pidx == AFF_DIR;
    b_no_cbf |= pidx == PRED_DIR_MMVD;
    b_no_cbf |= pidx == PRED_DIR;

    if(ctx->sps.tool_admvp == 0)
    {
        b_no_cbf = 0;
    }

    if(slice_type != SLICE_I)
    {
        if (ctx->sps.tool_admvp && core->log2_cuw == MIN_CU_LOG2 && core->log2_cuh == MIN_CU_LOG2)
        {
            xeve_assert(0);
        }

        xeve_sbac_encode_bin(0, &core->s_temp_run, core->s_temp_run.ctx.skip_flag + core->ctx_flags[CNID_SKIP_FLAG], &core->bs_temp); /* skip_flag */

        if (xeve_check_all_preds(core->tree_cons))
        {
            xeve_eco_pred_mode(&core->bs_temp, MODE_INTER, core->ctx_flags[CNID_PRED_MODE]);
        }
        if (!xeve_check_only_inter(core->tree_cons) && xeve_check_luma(core->tree_cons) &&
             ctx->param.ibc_flag && core->log2_cuw <= ctx->sps.ibc_log_max_size && core->log2_cuh <= ctx->sps.ibc_log_max_size)
        {
            xevem_eco_ibc_flag(&core->bs_temp, 0, core->ctx_flags[CNID_IBC_FLAG]);
        }

        if(ctx->sps.tool_amvr)
        {
            xevem_eco_mvr_idx(&core->bs_temp, mvr_idx);
        }


        int dir_flag = (pidx == PRED_DIR);
        dir_flag |= (pidx == PRED_DIR_MMVD);
        dir_flag |= (pidx == AFF_DIR);

        if(ctx->sps.tool_admvp == 0)
        {
            xeve_eco_direct_mode_flag(&core->bs_temp, dir_flag);
        }
        else
        {
            if(mvr_idx == 0)
            {
                xevem_eco_merge_mode_flag(&core->bs_temp, dir_flag);
            }
        }

        if(ctx->sps.tool_mmvd)
        {
            if(dir_flag)
            {
                xevem_eco_mmvd_flag(&core->bs_temp, pidx == PRED_DIR_MMVD);
            }

            if((pidx == PRED_DIR_MMVD))
            {
                xevem_eco_mmvd_info(&core->bs_temp, pi->mmvd_idx[pidx], ctx->sh->mmvd_group_enable_flag && !((1 << core->log2_cuw)*(1 << core->log2_cuh) <= NUM_SAMPLES_BLOCK));
            }
        }

        // affine direct in rdo
        if(core->cuw >= 8 && core->cuh >= 8 && ctx->sps.tool_affine && ((pidx == PRED_DIR) || (pidx == AFF_DIR)))
        {
            xeve_sbac_encode_bin(mcore->affine_flag != 0, &core->s_temp_run, core->s_temp_run.ctx.affine_flag + core->ctx_flags[CNID_AFFN_FLAG], &core->bs_temp); /* direct affine_flag */

            if(mcore->affine_flag)
                xevem_eco_affine_mrg_idx(&core->bs_temp, mvp_idx[REFP_0]);
        }

        if (ctx->sps.tool_admvp == 1 && pidx == PRED_DIR && !mcore->affine_flag && mvr_idx == 0)
        {
            xevem_eco_merge_idx(&core->bs_temp, mvp_idx[0]);
        }


        if((((pidx % ORG_PRED_NUM) != PRED_DIR) && ((pidx % ORG_PRED_NUM) != PRED_DIR_MMVD)) || ((pidx >= AFF_L0) && (pidx <= AFF_6_BI) && (pidx != AFF_DIR)) )
        {
            xeve_eco_inter_pred_idc(&core->bs_temp, refi, slice_type, 1 << core->log2_cuw, 1 << core->log2_cuh, ctx->sps.tool_admvp);

            // affine inter in rdo
            if (core->cuw >= 16 && core->cuh >= 16 && ctx->sps.tool_affine && mvr_idx == 0)
            {
                xeve_sbac_encode_bin(mcore->affine_flag != 0, &core->s_temp_run, core->s_temp_run.ctx.affine_flag + core->ctx_flags[CNID_AFFN_FLAG], &core->bs_temp); /* inter affine_flag */
            }

            if(mcore->affine_flag)
            {
                xeve_sbac_encode_bin(mcore->affine_flag - 1, &core->s_temp_run, core->s_temp_run.ctx.affine_mode, &core->bs_temp); /* inter affine_mode */
            }

            if(!mcore->affine_flag)
            {
                if(ctx->sps.tool_admvp == 1 && REFI_IS_VALID(refi[REFP_0]) && REFI_IS_VALID(refi[REFP_1]))
                {
                    xevem_eco_bi_idx(&core->bs_temp, bi_idx - 1);
                }
            }
            refi0 = refi[REFP_0];
            refi1 = refi[REFP_1];
            if(IS_INTER_SLICE(slice_type) && REFI_IS_VALID(refi0))
            {
                if(ctx->sps.tool_admvp == 0)
                {
                    xeve_eco_refi(&core->bs_temp, ctx->rpm.num_refp[REFP_0], refi0);
                    xeve_eco_mvp_idx(&core->bs_temp, mvp_idx[REFP_0]);
                    xeve_eco_mvd(&core->bs_temp, mvd[REFP_0]);
                }
                else
                {
                    if(bi_idx != BI_FL0 && bi_idx != BI_FL1)
                    {
                        xeve_eco_refi(&core->bs_temp, ctx->rpm.num_refp[REFP_0], refi0);
                    }

                    if(mcore->affine_flag)
                    {
                        int b_zero = 1;

                        xevem_eco_affine_mvp_idx(&core->bs_temp, mvp_idx[REFP_0]);

                        for(vertex = 0; vertex < vertex_num; vertex++)
                        {
                            int mvd_x = affine_mvd[REFP_0][vertex][MV_X];
                            int mvd_y = affine_mvd[REFP_0][vertex][MV_Y];
                            if(mvd_x != 0 || mvd_y != 0)
                            {
                                b_zero = 0;
                                break;
                            }
                        }
                        xevem_eco_affine_mvd_flag(&core->bs_temp, b_zero, REFP_0);

                        if(b_zero == 0)
                        {
                            for(vertex = 0; vertex < vertex_num; vertex++)
                            {
                                xeve_eco_mvd(&core->bs_temp, affine_mvd[REFP_0][vertex]);
                            }
                        }
                    }
                    else
                    {
                        if(bi_idx != BI_FL0)
                        {
                            xeve_eco_mvd(&core->bs_temp, mvd[REFP_0]);
                        }
                    }
                }
            }

            if(slice_type == SLICE_B && REFI_IS_VALID(refi1))
            {
                if(ctx->sps.tool_admvp == 0)
                {
                    xeve_eco_refi(&core->bs_temp, ctx->rpm.num_refp[REFP_1], refi1);
                    xeve_eco_mvp_idx(&core->bs_temp, mvp_idx[REFP_1]);
                    xeve_eco_mvd(&core->bs_temp, mvd[REFP_1]);
                }
                else
                {
                    if(bi_idx != BI_FL0 && bi_idx != BI_FL1)
                    {
                        xeve_eco_refi(&core->bs_temp, ctx->rpm.num_refp[REFP_1], refi1);
                    }

                    if(mcore->affine_flag)
                    {
                        int b_zero = 1;

                        xevem_eco_affine_mvp_idx(&core->bs_temp, mvp_idx[REFP_1]);

                        for(vertex = 0; vertex < vertex_num; vertex++)
                        {
                            int mvd_x = affine_mvd[REFP_1][vertex][MV_X];
                            int mvd_y = affine_mvd[REFP_1][vertex][MV_Y];
                            if(mvd_x != 0 || mvd_y != 0)
                            {
                                b_zero = 0;
                                break;
                            }
                        }
                        xevem_eco_affine_mvd_flag(&core->bs_temp, b_zero, REFP_1);

                        if(b_zero == 0)
                        {
                            for(vertex = 0; vertex < vertex_num; vertex++)
                            {
                                xeve_eco_mvd(&core->bs_temp, affine_mvd[REFP_1][vertex]);
                            }
                        }
                    }
                    else
                    {
                        if(bi_idx != BI_FL1)
                        {
                            xeve_eco_mvd(&core->bs_temp, mvd[REFP_1]);
                        }
                    }
                }
            }
        }
    }

    if (ctx->pps.cu_qp_delta_enabled_flag)
    {
        core->cu_qp_delta_code = core->dqp_temp_run.cu_qp_delta_code;
        core->cu_qp_delta_is_coded = core->dqp_temp_run.cu_qp_delta_is_coded;
        ctx->tile[core->tile_idx].qp_prev_eco[core->thread_cnt]  = core->dqp_temp_run.prev_qp;
    }

    ctx->fn_eco_coef(ctx, core, &core->bs_temp, coef, MODE_INTER, ctx->pps.cu_qp_delta_enabled_flag, b_no_cbf, RUN_L | RUN_CB | RUN_CR);

    if (ctx->pps.cu_qp_delta_enabled_flag)
    {
        core->dqp_temp_run.cu_qp_delta_code = core->cu_qp_delta_code;
        core->dqp_temp_run.cu_qp_delta_is_coded = core->cu_qp_delta_is_coded;
        core->dqp_temp_run.prev_qp = ctx->tile[core->tile_idx].qp_prev_eco[core->thread_cnt] ;
        core->dqp_temp_run.curr_qp = core->qp;
    }
}

void mode_reset_intra_main(XEVE_CORE *core)
{
    XEVEM_CORE * mcore = (XEVEM_CORE*)core;
    mcore->ibc_flag = 0;
    mcore->affine_flag = 0;
};

static int mode_cu_init_main(XEVE_CTX * ctx, XEVE_CORE * core, int x, int y, int log2_cuw, int log2_cuh, int cud)
{
    XEVEM_CTX  * mctx = (XEVEM_CTX *)ctx;
    XEVEM_CORE * mcore = (XEVEM_CORE *)core;
    XEVE_PIBC *pibc = &mctx->pibc[core->thread_cnt];

    mode_cu_init(ctx, core, x, y, log2_cuw, log2_cuh, cud);

    mcore->ibc_flag = 0;
    mcore->affine_flag = 0;
    mcore->ats_intra_cu = 0;
    mcore->ats_mode = 0;
    mcore->ats_inter_info = 0;
    mcore->dmvr_flag = 0;

    pibc->qp_y = core->qp_y;
    pibc->qp_u = core->qp_u;
    pibc->qp_v = core->qp_v;

    return XEVE_OK;
}

void update_history_buffer_affine(XEVE_HISTORY_BUFFER *history_buffer, XEVE_MODE *mi, int slice_type, XEVE_CORE *core)
{
    XEVEM_CORE *mcore = (XEVEM_CORE *)core;
    int i;
    if(history_buffer->currCnt == history_buffer->m_maxCnt)
    {
        for(i = 1; i < history_buffer->currCnt; i++)
        {
            xeve_mcpy(history_buffer->history_mv_table[i - 1], history_buffer->history_mv_table[i], REFP_NUM * MV_D * sizeof(s16));
            xeve_mcpy(history_buffer->history_refi_table[i - 1], history_buffer->history_refi_table[i], REFP_NUM * sizeof(s8));
#if TRACE_ENC_CU_DATA
            history_buffer->history_cu_table[i - 1] = history_buffer->history_cu_table[i];
#endif
        }
        if(mi->affine_flag)
        {
            mi->mv_sp[REFP_0][MV_X] = 0;
            mi->mv_sp[REFP_0][MV_Y] = 0;
            mi->refi_sp[REFP_0] = REFI_INVALID;
            mi->mv_sp[REFP_1][MV_X] = 0;
            mi->mv_sp[REFP_1][MV_Y] = 0;
            mi->refi_sp[REFP_1] = REFI_INVALID;
            for (int lidx = 0; lidx < REFP_NUM; lidx++)
            {
                if (mi->refi[lidx] >= 0)
                {
                    s16(*ac_mv)[MV_D] = mi->affine_mv[lidx];
                    int dmv_hor_x, dmv_ver_x, dmv_hor_y, dmv_ver_y;
                    int mv_scale_hor = ac_mv[0][MV_X] << 7;
                    int mv_scale_ver = ac_mv[0][MV_Y] << 7;
                    int mv_y_hor = mv_scale_hor;
                    int mv_y_ver = mv_scale_ver;
                    int mv_scale_tmp_hor, mv_scale_tmp_ver;


                    dmv_hor_x = (ac_mv[1][MV_X] - ac_mv[0][MV_X]) << (7 - core->log2_cuw);
                    dmv_hor_y = (ac_mv[1][MV_Y] - ac_mv[0][MV_Y]) << (7 - core->log2_cuw);

                    if (mcore->affine_flag == 2)
                    {
                        dmv_ver_x = (ac_mv[2][MV_X] - ac_mv[0][MV_X]) << (7 - core->log2_cuh);
                        dmv_ver_y = (ac_mv[2][MV_Y] - ac_mv[0][MV_Y]) << (7 - core->log2_cuh);
                    }
                    else
                    {
                        dmv_ver_x = -dmv_hor_y;
                        dmv_ver_y = dmv_hor_x;
                    }
                    int pos_x = 1 << (core->log2_cuw - 1);
                    int pos_y = 1 << (core->log2_cuh - 1);

                    mv_scale_tmp_hor = mv_scale_hor + dmv_hor_x * pos_x + dmv_ver_x * pos_y;
                    mv_scale_tmp_ver = mv_scale_ver + dmv_hor_y * pos_x + dmv_ver_y * pos_y;

                    xeve_mv_rounding_s32(mv_scale_tmp_hor, mv_scale_tmp_ver, &mv_scale_tmp_hor, &mv_scale_tmp_ver, 7, 0);
                    mv_scale_tmp_hor = XEVE_CLIP3(-(1 << 15), (1 << 15) - 1, mv_scale_tmp_hor);
                    mv_scale_tmp_ver = XEVE_CLIP3(-(1 << 15), (1 << 15) - 1, mv_scale_tmp_ver);

                    mi->mv_sp[lidx][MV_X] = mv_scale_tmp_hor;
                    mi->mv_sp[lidx][MV_Y] = mv_scale_tmp_ver;
                    mi->refi_sp[lidx] = mi->refi[lidx];

                }
            }
            // some spatial neighbor may be unavailable
            if((slice_type == SLICE_P && REFI_IS_VALID(mi->refi_sp[REFP_0])) ||
                (slice_type == SLICE_B && (REFI_IS_VALID(mi->refi_sp[REFP_0]) || REFI_IS_VALID(mi->refi_sp[REFP_1]))))
            {
                xeve_mcpy(history_buffer->history_mv_table[history_buffer->currCnt - 1], mi->mv_sp, REFP_NUM * MV_D * sizeof(s16));
                xeve_mcpy(history_buffer->history_refi_table[history_buffer->currCnt - 1], mi->refi_sp, REFP_NUM * sizeof(s8));
#if TRACE_ENC_CU_DATA
                history_buffer->history_cu_table[history_buffer->currCnt - 1] = mi->trace_cu_idx;
#endif
            }
        }
        else
        {
            xeve_mcpy(history_buffer->history_mv_table[history_buffer->currCnt - 1], mi->mv, REFP_NUM * MV_D * sizeof(s16));
            xeve_mcpy(history_buffer->history_refi_table[history_buffer->currCnt - 1], mi->refi, REFP_NUM * sizeof(s8));
#if TRACE_ENC_CU_DATA
            history_buffer->history_cu_table[history_buffer->currCnt - 1] = mi->trace_cu_idx;
#endif
        }
    }
    else
    {
        if(mi->affine_flag)
        {
            mi->mv_sp[REFP_0][MV_X] = 0;
            mi->mv_sp[REFP_0][MV_Y] = 0;
            mi->refi_sp[REFP_0] = REFI_INVALID;
            mi->mv_sp[REFP_1][MV_X] = 0;
            mi->mv_sp[REFP_1][MV_Y] = 0;
            mi->refi_sp[REFP_1] = REFI_INVALID;
            for (int lidx = 0; lidx < REFP_NUM; lidx++)
            {
                if (mi->refi[lidx] >= 0)
                {
                    s16(*ac_mv)[MV_D] = mi->affine_mv[lidx];
                    int dmv_hor_x, dmv_ver_x, dmv_hor_y, dmv_ver_y;
                    int mv_scale_hor = ac_mv[0][MV_X] << 7;
                    int mv_scale_ver = ac_mv[0][MV_Y] << 7;
                    int mv_y_hor = mv_scale_hor;
                    int mv_y_ver = mv_scale_ver;
                    int mv_scale_tmp_hor, mv_scale_tmp_ver;

                    dmv_hor_x = (ac_mv[1][MV_X] - ac_mv[0][MV_X]) << (7 - core->log2_cuw);
                    dmv_hor_y = (ac_mv[1][MV_Y] - ac_mv[0][MV_Y]) << (7 - core->log2_cuw);

                    if (mcore->affine_flag == 2)
                    {
                        dmv_ver_x = (ac_mv[2][MV_X] - ac_mv[0][MV_X]) << (7 - core->log2_cuh);
                        dmv_ver_y = (ac_mv[2][MV_Y] - ac_mv[0][MV_Y]) << (7 - core->log2_cuh);
                    }
                    else
                    {
                        dmv_ver_x = -dmv_hor_y;
                        dmv_ver_y = dmv_hor_x;
                    }
                    int pos_x = 1 << (core->log2_cuw - 1);
                    int pos_y = 1 << (core->log2_cuh - 1);

                    mv_scale_tmp_hor = mv_scale_hor + dmv_hor_x * pos_x + dmv_ver_x * pos_y;
                    mv_scale_tmp_ver = mv_scale_ver + dmv_hor_y * pos_x + dmv_ver_y * pos_y;

                    xeve_mv_rounding_s32(mv_scale_tmp_hor, mv_scale_tmp_ver, &mv_scale_tmp_hor, &mv_scale_tmp_ver, 7, 0);
                    mv_scale_tmp_hor = XEVE_CLIP3(-(1 << 15), (1 << 15) - 1, mv_scale_tmp_hor);
                    mv_scale_tmp_ver = XEVE_CLIP3(-(1 << 15), (1 << 15) - 1, mv_scale_tmp_ver);

                    mi->mv_sp[lidx][MV_X] = mv_scale_tmp_hor;
                    mi->mv_sp[lidx][MV_Y] = mv_scale_tmp_ver;
                    mi->refi_sp[lidx] = mi->refi[lidx];
                }
            }
            // some spatial neighbor may be unavailable
            if((slice_type == SLICE_P && REFI_IS_VALID(mi->refi_sp[REFP_0])) ||
                (slice_type == SLICE_B && (REFI_IS_VALID(mi->refi_sp[REFP_0]) || REFI_IS_VALID(mi->refi_sp[REFP_1]))))
            {
                xeve_mcpy(history_buffer->history_mv_table[history_buffer->currCnt], mi->mv_sp, REFP_NUM * MV_D * sizeof(s16));
                xeve_mcpy(history_buffer->history_refi_table[history_buffer->currCnt], mi->refi_sp, REFP_NUM * sizeof(s8));
#if TRACE_ENC_CU_DATA
                history_buffer->history_cu_table[history_buffer->currCnt] = mi->trace_cu_idx;
#endif
            }
        }
        else
        {
            xeve_mcpy(history_buffer->history_mv_table[history_buffer->currCnt], mi->mv, REFP_NUM * MV_D * sizeof(s16));
            xeve_mcpy(history_buffer->history_refi_table[history_buffer->currCnt], mi->refi, REFP_NUM * sizeof(s8));
#if TRACE_ENC_CU_DATA
            history_buffer->history_cu_table[history_buffer->currCnt] = mi->trace_cu_idx;
#endif
        }

        history_buffer->currCnt++;
    }
}

void xeve_set_affine_mv(XEVE_CTX *ctx, XEVE_CORE *core, XEVE_MODE *mi)
{
    XEVEM_CORE   *mcore = (XEVEM_CORE*)core;
    XEVE_CU_DATA *cu_data;
    int   log2_cuw, log2_cuh;
    int   w_cu;
    int   h_cu;
    int   i;
    int   lidx;
    int   idx;
    int   vertex_num = mcore->affine_flag + 1;
    int   aff_scup[VER_NUM];

    log2_cuw = XEVE_LOG2(core->cuw);
    log2_cuh = XEVE_LOG2(core->cuh);
    cu_data = &core->cu_data_temp[log2_cuw - 2][log2_cuh - 2];

    w_cu = core->cuw >> MIN_CU_LOG2;
    h_cu = core->cuh >> MIN_CU_LOG2;

    aff_scup[0] = 0;
    aff_scup[1] = (w_cu - 1);
    aff_scup[2] = (h_cu - 1) * w_cu;
    aff_scup[3] = (w_cu - 1) + (h_cu - 1) * w_cu;

    // derive sub-block size
    int sub_w = 4, sub_h = 4;
    derive_affine_subblock_size_bi( mi->affine_mv, mi->refi, core->cuw, core->cuh, &sub_w, &sub_h, vertex_num, NULL);

    int   sub_w_in_scu = PEL2SCU( sub_w );
    int   sub_h_in_scu = PEL2SCU( sub_h );
    int   half_w = sub_w >> 1;
    int   half_h = sub_h >> 1;

    for(lidx = 0; lidx < REFP_NUM; lidx++)
    {
        if(mi->refi[lidx] >= 0)
        {
            s16( *ac_mv )[MV_D] = mi->affine_mv[lidx];
            int dmv_hor_x, dmv_ver_x, dmv_hor_y, dmv_ver_y;
            int mv_scale_hor = ac_mv[0][MV_X] << 7;
            int mv_scale_ver = ac_mv[0][MV_Y] << 7;
            int mv_scale_tmp_hor, mv_scale_tmp_ver;

            // convert to 2^(storeBit + iBit) precision
            dmv_hor_x = (ac_mv[1][MV_X] - ac_mv[0][MV_X]) << (7 - core->log2_cuw);      // deltaMvHor
            dmv_hor_y = (ac_mv[1][MV_Y] - ac_mv[0][MV_Y]) << (7 - core->log2_cuw);
            if ( vertex_num == 3 )
            {
                dmv_ver_x = (ac_mv[2][MV_X] - ac_mv[0][MV_X]) << (7 - core->log2_cuh);  // deltaMvVer
                dmv_ver_y = (ac_mv[2][MV_Y] - ac_mv[0][MV_Y]) << (7 - core->log2_cuh);
            }
            else
            {
                dmv_ver_x = -dmv_hor_y;                                                 // deltaMvVer
                dmv_ver_y = dmv_hor_x;
            }

            idx = 0;
            for ( int h = 0; h < h_cu; h += sub_h_in_scu )
            {
                for ( int w = 0; w < w_cu; w += sub_w_in_scu )
                {
                    if ( w == 0 && h == 0 )
                    {
                        mv_scale_tmp_hor = ac_mv[0][MV_X];
                        mv_scale_tmp_ver = ac_mv[0][MV_Y];
                    }
                    else if ( w + sub_w_in_scu == w_cu && h == 0 )
                    {
                        mv_scale_tmp_hor = ac_mv[1][MV_X];
                        mv_scale_tmp_ver = ac_mv[1][MV_Y];
                    }
                    else if ( w == 0 && h + sub_h_in_scu == h_cu && vertex_num == 3 )
                    {
                        mv_scale_tmp_hor = ac_mv[2][MV_X];
                        mv_scale_tmp_ver = ac_mv[2][MV_Y];
                    }
                    else
                    {
                        int pos_x = (w << MIN_CU_LOG2) + half_w;
                        int pos_y = (h << MIN_CU_LOG2) + half_h;

                        mv_scale_tmp_hor = mv_scale_hor + dmv_hor_x * pos_x + dmv_ver_x * pos_y;
                        mv_scale_tmp_ver = mv_scale_ver + dmv_hor_y * pos_x + dmv_ver_y * pos_y;

                        // 1/16 precision, 18 bits, same as MC
                        xeve_mv_rounding_s32( mv_scale_tmp_hor, mv_scale_tmp_ver, &mv_scale_tmp_hor, &mv_scale_tmp_ver, 5, 0 );

                        mv_scale_tmp_hor = XEVE_CLIP3( -(1 << 17), (1 << 17) - 1, mv_scale_tmp_hor );
                        mv_scale_tmp_ver = XEVE_CLIP3( -(1 << 17), (1 << 17) - 1, mv_scale_tmp_ver );

                        // 1/4 precision, 16 bits for storage
                        mv_scale_tmp_hor >>= 2;
                        mv_scale_tmp_ver >>= 2;
                    }

                    // save MV for each 4x4 block
                    for ( int y = h; y < h + sub_h_in_scu; y++ )
                    {
                        for ( int x = w; x < w + sub_w_in_scu; x++ )
                        {
                            idx = x + y * w_cu;
                            cu_data->mv[idx][lidx][MV_X] = (s16)mv_scale_tmp_hor;
                            cu_data->mv[idx][lidx][MV_Y] = (s16)mv_scale_tmp_ver;
                        }
                    }
                }
            }
            // save mvd for encoding, and reset vertex mv
            for(i = 0; i < vertex_num; i++)
            {
                cu_data->mvd[aff_scup[i]][lidx][MV_X] = mi->affine_mvd[lidx][i][MV_X];
                cu_data->mvd[aff_scup[i]][lidx][MV_Y] = mi->affine_mvd[lidx][i][MV_Y];
            }
        }
    }
}


void copy_to_cu_data_main(XEVE_CTX *ctx, XEVE_CORE *core, XEVE_MODE *mi, s16 coef_src[N_C][MAX_CU_DIM])
{
    XEVEM_CORE   *mcore = (XEVEM_CORE *)core;
    XEVE_CU_DATA *cu_data;
    int i, j, idx;
    u32 size;
    int log2_cuw, log2_cuh;

    log2_cuw = XEVE_LOG2(core->cuw);
    log2_cuh = XEVE_LOG2(core->cuh);

    cu_data = &core->cu_data_temp[log2_cuw - 2][log2_cuh - 2];

    copy_to_cu_data(ctx, core, mi, coef_src);

    if (xeve_check_luma(core->tree_cons))
    {
        /* copy coef */
        size = core->cuw * core->cuh * sizeof(s16);
        xeve_mcpy(cu_data->coef[Y_C], coef_src[Y_C], size);

        /* copy reco */
        size = core->cuw * core->cuh * sizeof(pel);
        xeve_mcpy(cu_data->reco[Y_C], mi->rec[Y_C], size);

        /* copy mode info */
        idx = 0;
        for (j = 0; j < core->cuh >> MIN_CU_LOG2; j++)
        {
            for (i = 0; i < core->cuw >> MIN_CU_LOG2; i++)
            {
                if (ctx->param.ibc_flag)
                {
                    cu_data->ibc_flag[idx + i] = mcore->ibc_flag;
                    if (mcore->ibc_flag)
                    {
                        MCU_SET_IBC(cu_data->map_scu[idx + i]);
                    }
                    else
                    {
                        MCU_CLR_IBC(cu_data->map_scu[idx + i]);
                    }
                }

                cu_data->affine_flag[idx + i] = mcore->affine_flag;
                if (mcore->affine_flag)
                {
                    MCU_SET_AFF(cu_data->map_scu[idx + i], mcore->affine_flag);
                    MCU_SET_AFF_LOGW(cu_data->map_affine[idx + i], log2_cuw);
                    MCU_SET_AFF_LOGH(cu_data->map_affine[idx + i], log2_cuh);
                    MCU_SET_AFF_XOFF(cu_data->map_affine[idx + i], i);
                    MCU_SET_AFF_YOFF(cu_data->map_affine[idx + i], j);
                }
                else
                {
                    MCU_CLR_AFF(cu_data->map_scu[idx + i]);
                }

                cu_data->ats_intra_cu[idx + i]   = mcore->ats_intra_cu;
                cu_data->ats_mode_h[idx + i]     = mcore->ats_mode >> 1;
                cu_data->ats_mode_v[idx + i]     = mcore->ats_mode & 1;
                cu_data->ats_inter_info[idx + i] = mcore->ats_inter_info;

                MCU_CLR_DMVRF(cu_data->map_scu[idx + i]);
                if (core->cu_mode == MODE_SKIP || core->cu_mode == MODE_DIR)
                {
                    cu_data->dmvr_flag[idx + i] = mcore->dmvr_flag;
                    if (cu_data->dmvr_flag[idx + i])
                    {
                        MCU_SET_DMVRF(cu_data->map_scu[idx + i]);
                    }
                }

                if (cu_data->dmvr_flag[idx + i])
                {
                    cu_data->mv[idx + i][REFP_0][MV_X] = mi->dmvr_mv[idx + i][REFP_0][MV_X];
                    cu_data->mv[idx + i][REFP_0][MV_Y] = mi->dmvr_mv[idx + i][REFP_0][MV_Y];
                    cu_data->mv[idx + i][REFP_1][MV_X] = mi->dmvr_mv[idx + i][REFP_1][MV_X];
                    cu_data->mv[idx + i][REFP_1][MV_Y] = mi->dmvr_mv[idx + i][REFP_1][MV_Y];

                    cu_data->unrefined_mv[idx + i][REFP_0][MV_X] = mi->mv[REFP_0][MV_X];
                    cu_data->unrefined_mv[idx + i][REFP_0][MV_Y] = mi->mv[REFP_0][MV_Y];
                    cu_data->unrefined_mv[idx + i][REFP_1][MV_X] = mi->mv[REFP_1][MV_X];
                    cu_data->unrefined_mv[idx + i][REFP_1][MV_Y] = mi->mv[REFP_1][MV_Y];
                }
            }
            idx += core->cuw >> MIN_CU_LOG2;
        }
        if (mcore->affine_flag)
        {
            xeve_set_affine_mv(ctx, core, mi);
        }
    }
}


int xeve_hmvp_init(XEVE_HISTORY_BUFFER *history_buffer)
{
    xeve_mset(history_buffer->history_mv_table, 0, ALLOWED_CHECKED_NUM * REFP_NUM * MV_D * sizeof(s16));
#if TRACE_ENC_CU_DATA
    xeve_mset(history_buffer->history_cu_table, 0, sizeof(history_buffer->history_cu_table[0])* ALLOWED_CHECKED_NUM);
#endif

    for (int i = 0; i < ALLOWED_CHECKED_NUM; i++)
    {
        history_buffer->history_refi_table[i][REFP_0] = REFI_INVALID;
        history_buffer->history_refi_table[i][REFP_1] = REFI_INVALID;
    }

    history_buffer->currCnt = 0;
    history_buffer->m_maxCnt = ALLOWED_CHECKED_NUM;

    return XEVE_OK;
}

static int init_history_buffer(XEVE_HISTORY_BUFFER *history_buffer)
{
    xeve_mset(history_buffer->history_mv_table,   0, ALLOWED_CHECKED_NUM * REFP_NUM * MV_D * sizeof(s16));
#if TRACE_ENC_CU_DATA
    xeve_mset(history_buffer->history_cu_table, 0, sizeof(history_buffer->history_cu_table[0])* ALLOWED_CHECKED_NUM);
#endif


    //xeve_mset(history_buffer->history_refi_table, 0, ALLOWED_CHECKED_NUM * REFP_NUM * sizeof(s8));
    for (int i = 0; i < ALLOWED_CHECKED_NUM; i++)
    {
        history_buffer->history_refi_table[i][REFP_0] = REFI_INVALID;
        history_buffer->history_refi_table[i][REFP_1] = REFI_INVALID;
    }

    history_buffer->currCnt = 0;
    history_buffer->m_maxCnt = ALLOWED_CHECKED_NUM;

    return XEVE_OK;
}

static int copy_history_buffer(XEVE_HISTORY_BUFFER *dst, XEVE_HISTORY_BUFFER *src)
{
    xeve_mcpy(dst->history_mv_table,   src->history_mv_table,   sizeof(s16)* ALLOWED_CHECKED_NUM * REFP_NUM * MV_D);
    xeve_mcpy(dst->history_refi_table, src->history_refi_table, sizeof(s8)* ALLOWED_CHECKED_NUM * REFP_NUM);
#if TRACE_ENC_CU_DATA
    xeve_mcpy(dst->history_cu_table, src->history_cu_table, sizeof(src->history_cu_table[0])* ALLOWED_CHECKED_NUM);
#endif

    dst->currCnt = src->currCnt;
    dst->m_maxCnt = src->m_maxCnt;

    return XEVE_OK;
}


static void update_map_scu_main(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int src_cuw, int src_cuh)
{
    XEVEM_CTX *mctx = (XEVEM_CTX *)ctx;
    int        w, h, i, size32, size8;
    int        log2_src_cuw, log2_src_cuh;
    int        scu_x, scu_y;
    u32       *map_affine = 0, *src_map_affine = 0;
    u8        *map_ats_inter = 0, *src_map_ats_inter = 0;

    scu_x = x >> MIN_CU_LOG2;
    scu_y = y >> MIN_CU_LOG2;
    log2_src_cuw = XEVE_LOG2(src_cuw);
    log2_src_cuh = XEVE_LOG2(src_cuh);

    map_affine = mctx->map_affine + scu_y * ctx->w_scu + scu_x;
    src_map_affine = core->cu_data_best[log2_src_cuw - 2][log2_src_cuh - 2].map_affine;

    map_ats_inter = mctx->map_ats_inter + scu_y * ctx->w_scu + scu_x;
    src_map_ats_inter = core->cu_data_best[log2_src_cuw - 2][log2_src_cuh - 2].ats_inter_info;

    update_map_scu(ctx, core, x, y, src_cuw, src_cuh);

    if(x + src_cuw > ctx->w)
    {
        w = (ctx->w - x) >> MIN_CU_LOG2;
    }
    else
    {
        w = (src_cuw >> MIN_CU_LOG2);
    }

    if(y + src_cuh > ctx->h)
    {
        h = (ctx->h - y) >> MIN_CU_LOG2;
    }
    else
    {
        h = (src_cuh >> MIN_CU_LOG2);
    }

    size32 = sizeof(u32) * w;
    size8 = sizeof(u8) * w;

    for(i = 0; i < h; i++)
    {
        xeve_mcpy(map_affine, src_map_affine, size32);
        map_affine += ctx->w_scu;
        src_map_affine += (src_cuw >> MIN_CU_LOG2);

        xeve_mcpy(map_ats_inter, src_map_ats_inter, size8);
        map_ats_inter += ctx->w_scu;
        src_map_ats_inter += (src_cuw >> MIN_CU_LOG2);
    }
}

static void clear_map_scu_main(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int cuw, int cuh)
{
    XEVEM_CTX *mctx = (XEVEM_CTX *)ctx;
    int w, h, i, size;
    u32 *map_affine    = mctx->map_affine + (y >> MIN_CU_LOG2) * ctx->w_scu + (x >> MIN_CU_LOG2);
    u8  *map_ats_inter = mctx->map_ats_inter + (y >> MIN_CU_LOG2) * ctx->w_scu + (x >> MIN_CU_LOG2);

    clear_map_scu(ctx, core, x, y, cuw, cuh);

    if (x + cuw > ctx->w)
    {
        cuw = ctx->w - x;
    }

    if (y + cuh > ctx->h)
    {
        cuh = ctx->h - y;
    }

    w = (cuw >> MIN_CU_LOG2);
    h = (cuh >> MIN_CU_LOG2);

    size = sizeof(u32) * w;

    for (i = 0; i < h; i++)
    {
        xeve_mset(map_affine, 0, size);
        map_affine += ctx->w_scu;

        xeve_mset(map_ats_inter, 0, sizeof(u8) * w);
        map_ats_inter += ctx->w_scu;
    }
}

static double mode_check_ibc(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh, int cud, XEVE_MODE *mi, double cost_best)
{
    XEVEM_CTX  *mctx = (XEVEM_CTX *)ctx;
    XEVEM_CORE *mcore = (XEVEM_CORE *)core;
    s16      (*coef)[MAX_CU_DIM] = core->ctmp;
    pel       *rec[N_C];
    double     cost = MAX_COST;
    int        start_comp = xeve_check_luma(core->tree_cons) ? Y_C : U_C;
    int        end_comp = xeve_check_chroma(core->tree_cons) ? N_C : U_C;
    int        i, s_rec[N_C];

    if (ctx->param.ibc_flag == 1 && (core->nnz[Y_C] != 0 || core->nnz[U_C] != 0 || core->nnz[V_C] != 0 || cost_best == MAX_COST)
        && (!xeve_check_only_inter(core->tree_cons)) && xeve_check_luma(core->tree_cons))
    {
        if (log2_cuw <= ctx->sps.ibc_log_max_size && log2_cuh <= ctx->sps.ibc_log_max_size)
        {
            core->avail_cu = xeve_get_avail_ibc(core->x_scu, core->y_scu, ctx->w_scu, ctx->h_scu, core->scup, core->cuw, core->cuh, ctx->map_scu, ctx->map_tidx);
            cost = mctx->fn_pibc_analyze_cu(ctx, core, x, y, log2_cuw, log2_cuh, mi, coef, rec, s_rec);

            if (cost < cost_best)
            {
                cost_best = cost;
                core->cu_mode = MODE_IBC;
                mcore->ibc_flag = 1;

                SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
                DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);

                XEVE_PIBC *pibc = &mctx->pibc[core->thread_cnt];
                mi->pred_y_best = pibc->pred[0][Y_C];
                mi->mvp_idx[0] = pibc->mvp_idx;

                mi->mv[0][MV_X] = pibc->mv[0][MV_X];
                mi->mv[0][MV_Y] = pibc->mv[0][MV_Y];

                mi->mvd[0][MV_X] = pibc->mvd[MV_X];
                mi->mvd[0][MV_Y] = pibc->mvd[MV_Y];

                for (i = start_comp; i < end_comp; i++)
                {
                    mi->rec[i] = rec[i];
                    mi->s_rec[i] = s_rec[i];
                }
                core->skip_flag = 0;
                mcore->affine_flag = 0;
                mcore->dmvr_flag = 0;
                if (ctx->pps.cu_qp_delta_enabled_flag)
                {
                    xeve_set_qp(ctx, core, core->dqp_next_best[log2_cuw - 2][log2_cuh - 2].prev_qp);
                }
                ctx->fn_mode_copy_to_cu_data(ctx, core, mi, coef);
            }
        }
    }
    return cost_best;
}

static double mode_coding_unit_main(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh, int cud, XEVE_MODE *mi)
{
    s16(*coef)[MAX_CU_DIM] = core->ctmp;
    double  cost_best;
    xeve_assert(abs(log2_cuw - log2_cuh) <= 2);
    mode_cu_init_main(ctx, core, x, y, log2_cuw, log2_cuh, cud);

    if (ctx->sps.sps_btt_flag && log2_cuw == 2 && log2_cuh == 2 && ctx->sps.tool_admvp)
    {
        // Check only in main profile
        xeve_assert(ctx->sps.chroma_format_idc == 0 || !xeve_check_all(core->tree_cons));
        xeve_assert(xeve_check_only_intra(core->tree_cons));
    }

    if (ctx->sps.chroma_format_idc != 0 && ((log2_cuw + log2_cuh) == 5 && ctx->sps.tool_admvp))
    {
        xeve_assert(!xeve_check_all_preds(core->tree_cons));

        if (xeve_check_only_intra(core->tree_cons))
        {
            xeve_assert(!xeve_check_all(core->tree_cons));
        }
    }

    core->avail_lr = xeve_check_nev_avail(core->x_scu, core->y_scu, (1 << log2_cuw), (1 << log2_cuh), ctx->w_scu, ctx->h_scu, ctx->map_scu, ctx->map_tidx);
    xeve_get_ctx_some_flags(core->x_scu, core->y_scu, 1 << log2_cuw, 1 << log2_cuh, ctx->w_scu, ctx->map_scu, ctx->map_cu_mode, core->ctx_flags, ctx->sh->slice_type, ctx->sps.tool_cm_init
                         , ctx->param.ibc_flag, ctx->sps.ibc_log_max_size, ctx->map_tidx);

    cost_best = MAX_COST;
    core->cost_best = MAX_COST;

    cost_best = mode_check_inter(ctx, core, x, y, log2_cuw, log2_cuh, cud, mi, cost_best);
    cost_best = mode_check_ibc(ctx, core, x, y, log2_cuw, log2_cuh, cud, mi, cost_best);
    cost_best = mode_check_intra(ctx, core, x, y, log2_cuw, log2_cuh, cud, mi, cost_best);

    return cost_best;
}

void xeve_init_bef_data(XEVE_CORE* core, XEVE_CTX* ctx)
{
    XEVEM_CORE * mcore = (XEVEM_CORE * )core;
    xeve_mset(&mcore->bef_data, 0, sizeof(XEVE_BEF_DATA) * NUM_CU_LOG2 * NUM_CU_LOG2 * MAX_CU_CNT_IN_LCU * MAX_BEF_DATA_NUM);
}

static void check_run_split(XEVE_CORE *core, int log2_cuw, int log2_cuh, int cup, int next_split, int do_curr, int do_split, u16 bef_data_idx, int* split_allow, int boundary, TREE_CONS tree_cons)
{
    int i;
    double min_cost = MAX_COST;
    int run_list[MAX_SPLIT_NUM]; //a smaller set of allowed split modes based on a save & load technique
    XEVEM_CORE * mcore = (XEVEM_CORE *)core;

    if(!next_split)
    {
        xeve_mset(split_allow, 0, sizeof(int) * MAX_SPLIT_NUM);
        split_allow[0] = 1;
        return;
    }
    if(mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].split_visit)
    {
        if((mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].nosplit < 1
            && mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].split >= 1))
        {
            run_list[0] = 0;

            for(i = 1; i < MAX_SPLIT_NUM; i++)
            {
                if(mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].split_cost[i] < min_cost && split_allow[i])
                {
                    min_cost = mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].split_cost[i];
                }
            }

            if(min_cost == MAX_COST)
            {
                run_list[0] = 1;
                for(i = 1; i < MAX_SPLIT_NUM; i++)
                {
                    if((mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].remaining_split >> i) & 0x01)
                    {
                        run_list[i] = 1;
                    }
                    else
                    {
                        run_list[i] = 0;
                    }
                }
            }
            else
            {
                for(i = 1; i < MAX_SPLIT_NUM; i++)
                {
                    double th = 1.01;
                    if (core->ctx->param.rdo_dbk_switch)
                    {
                        th = (min_cost < 0) ? 0.99 : 1.02;
                    }

                    if(mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].split_cost[i] <= th * min_cost)
                    {
                        run_list[i] = 1;
                    }
                    else if((mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].remaining_split >> i) & 0x01)
                    {
                        run_list[i] = 1;
                    }
                    else
                    {
                        run_list[i] = 0;
                    }
                }
            }
        }
        else if(mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].nosplit == 0
                && mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].split == 0)
        {
            run_list[0] = 1;
            for(i = 1; i < MAX_SPLIT_NUM; i++)
            {
                if((mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].remaining_split >> i) & 0x01)
                {
                    run_list[i] = 1;
            }
                else
                {
                    run_list[i] = 0;
                }
            }
        }
        else
        {
            xeve_mset(run_list, 0, sizeof(int) * MAX_SPLIT_NUM);
            run_list[0] = 1;
        }
    }
    else
    {
        for(i = 0; i < MAX_SPLIT_NUM; i++)
        {
            run_list[i] = 1;
            mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].split_cost[i] = MAX_COST;
        }

        run_list[0] &= do_curr;

        for(i = 1; i < MAX_SPLIT_NUM; i++)
        {
            run_list[i] &= do_split;
        }
    }

    //modified split_allow by the save & load decision
    int num_run = 0;
    split_allow[0] = run_list[0];
    for(i = 1; i < MAX_SPLIT_NUM; i++)
    {
        split_allow[i] = run_list[i] && split_allow[i];
        num_run += split_allow[i];
    }

    //if all further splitting modes are not tried, at least we need try NO_SPLIT
    if(num_run == 0)
        split_allow[0] = 1;
}

void get_cud_min_max_avg(XEVE_CU_DATA *best_cu_data, int cuw, int cuh, int sub_cuw, int sub_cuh, int cux_offset, int cuy_offset, int *cud_min, int* cud_max, int *cud_avg)
{
    int i, j, idx, depth;
    int x_offset_scu = cux_offset >> MIN_CU_LOG2;
    int y_offset_scu = cuy_offset >> MIN_CU_LOG2;
    int min_depth = MAX_CU_DEPTH;
    int max_depth = 0;
    int sum = 0;

    xeve_assert(cuw >= (1 << MIN_CU_LOG2));
    xeve_assert(cuh >= (1 << MIN_CU_LOG2));
    xeve_assert(sub_cuw >= (1 << MIN_CU_LOG2));
    xeve_assert(sub_cuh >= (1 << MIN_CU_LOG2));
    xeve_assert(sub_cuw <= cuw);
    xeve_assert(sub_cuh <= cuh);
    xeve_assert((cux_offset + sub_cuw) <= cuw);
    xeve_assert((cuy_offset + sub_cuh) <= cuh);

    for (j = 0; j < (sub_cuh >> MIN_CU_LOG2); j++)
    {
        for (i = 0; i < (sub_cuw >> MIN_CU_LOG2); i++)
        {
            idx = (x_offset_scu + i) + ((y_offset_scu + j) * (cuw >> MIN_CU_LOG2));

            depth = best_cu_data->depth[idx];

            if (depth < min_depth)
            {
                min_depth = depth;
            }

            if (depth > max_depth)
            {
                max_depth = depth;
            }

            sum += depth;
        }
    }

    *cud_min = min_depth;
    *cud_max = max_depth;
    *cud_avg = sum / ((sub_cuw >> MIN_CU_LOG2) * (sub_cuh >> MIN_CU_LOG2));
}

static int lossycheck_biver(XEVE_CU_DATA *cu_data, int eval_parent_node_first, double cost_best, int log2_cuw, int log2_cuh, int cuw, int cuh, int cud, int nev_max_depth)
{
    int ans = 0;
    if(!eval_parent_node_first)
    {
        if(cost_best != MAX_COST)
        {
            int cud_min, cud_max, cud_avg;

            get_cud_min_max_avg(cu_data, cuw, cuh, cuw, cuh, 0, 0, &cud_min, &cud_max, &cud_avg);

            if(((cud_min > (cud + 6))) ||
               ((cud_min > (cud + 3)) && (cud_max > cud_min)) ||
               ((cud_max == (cud + 3)) && ((cud + 3) < nev_max_depth)))
            {
                ans = 1;
            }
        }
    }
    return ans;
}

static int lossycheck_bihor(XEVE_CU_DATA *cu_data, int eval_parent_node_first, double cost_best, int log2_cuw, int log2_cuh, int cuw, int cuh, int cud, int nev_max_depth)
{
    int ans = 0;
    int cud_min, cud_max, cud_avg;

    if(!eval_parent_node_first)
    {
        if(cost_best != MAX_COST)
        {
            get_cud_min_max_avg(cu_data, cuw, cuh, cuw, cuh, 0, 0, &cud_min, &cud_max, &cud_avg);

            if(((cud_min > (cud + 6))) ||
               ((cud_min > (cud + 3)) && (cud_max > cud_min)) ||
               ((cud_max == (cud + 3)) && ((cud + 3) < nev_max_depth)))
            {
                ans = 1;
            }
        }
    }
    else if(cuw == cuh)
    {
        if(cost_best != MAX_COST) //TODO: Check once more
        {
            get_cud_min_max_avg(cu_data, cuw, cuh, cuw, cuh, 0, 0, &cud_min, &cud_max, &cud_avg);

            if((cud_min > (cud + 2)) /*&& (cud_max > cud_min) */)
            {
                ans = 1;
            }
        }
    }
    return ans;
}

static int lossycheck_ttver(XEVE_CU_DATA *cu_data, int eval_parent_node_first, double cost_best, int log2_cuw, int log2_cuh, int cuw, int cuh, int cud, int nev_max_depth)
{
    int ans = 0;
    int cud_min, cud_max, cud_avg;

    if(cost_best != MAX_COST)
    {
        get_cud_min_max_avg(cu_data, cuw, cuh, (cuw >> 1), cuh, (cuw >> 2), 0, &cud_min, &cud_max, &cud_avg);

        if((cud_min > (cud + 3)) /*&& (cud_max > cud_min)*/)
        {
            ans = 1;
        }
    }
    return ans;
}

static int lossycheck_tthor(XEVE_CU_DATA *cu_data, int eval_parent_node_first, double cost_best, int log2_cuw, int log2_cuh, int cuw, int cuh, int cud, int nev_max_depth)
{
    int ans = 0;
    int cud_min, cud_max, cud_avg;

    if(cost_best != MAX_COST)
    {
        get_cud_min_max_avg(cu_data, cuw, cuh, cuw, (cuh >> 1), 0, (cuh >> 2), &cud_min, &cud_max, &cud_avg);

        if((cud_min > (cud + 3)) /*&& (cud_max > cud_min)*/)
        {
            ans = 1;
        }
    }
    return ans;
}

static double mode_coding_tree_main(XEVE_CTX *ctx, XEVE_CORE *core, int x0, int y0, int cup, int log2_cuw, int log2_cuh, int cud, XEVE_MODE *mi, int next_split, int parent_suco, u8 qp, TREE_CONS tree_cons)
{
    // x0 = CU's left up corner horizontal index in entrie frame
    // y0 = CU's left up corner vertical index in entire frame
    // cuw = CU width, log2_cuw = CU width in log2
    // cuh = CU height, log2_chu = CU height in log2
    // ctx->w = frame width, ctx->h = frame height
    XEVEM_CORE *mcore = (XEVEM_CORE*)core;
    int cuw = 1 << log2_cuw;
    int cuh = 1 << log2_cuh;
    s8 best_split_mode = NO_SPLIT;
    int bit_cnt;
    double cost_best = MAX_COST;
    double cost_temp = MAX_COST;
    XEVE_SBAC s_temp_depth = {0};
    int boundary = !(x0 + cuw <= ctx->w && y0 + cuh <= ctx->h);
    int split_allow[SPLIT_QUAD + 1]; //allowed split by normative and non-normative selection
    s8 best_suco_flag = 0;
    u16 avail_lr = xeve_check_nev_avail(PEL2SCU(x0), PEL2SCU(y0), cuw, cuh, ctx->w_scu, ctx->h_scu, ctx->map_scu, ctx->map_tidx);
    SPLIT_MODE split_mode = NO_SPLIT;
    int do_split, do_curr;
    double best_split_cost = MAX_COST;
    double best_curr_cost = MAX_COST;
    int split_mode_child[4] = { NO_SPLIT, NO_SPLIT, NO_SPLIT, NO_SPLIT };
    int curr_split_allow[SPLIT_QUAD + 1]; //allowed split by normative selection, used in entropy coding
    u8  remaining_split = 0;
    int num_split_tried = 0;
    int num_split_to_try = 0;
    int bef_data_idx = 0;
#if ET_BY_RDC_CHILD_SPLIT
    double split_cost[6] = { MAX_COST, MAX_COST, MAX_COST, MAX_COST, MAX_COST, MAX_COST };
    int split_mode_child_rdo[6][4];
#endif
    int nev_max_depth = 0;
    int eval_parent_node_first = 1;
    int nbr_map_skip_flag = 0;
    int cud_min = cud;
    int cud_max = cud;
    int cud_avg = cud;
    XEVE_DQP dqp_temp_depth = { 0 };
    u8 best_dqp = qp;
    s8 min_qp, max_qp;
    double cost_temp_dqp;
    double cost_best_dqp = MAX_COST;
    int dqp_coded = 0;
    int loop_counter;
    int dqp_loop;
    int cu_mode_dqp = 0;
    int dist_cu_best_dqp = 0;
    int ibc_flag_dqp = 0;
    core->tree_cons = tree_cons;
    int check_max_cu, check_min_cu;
    if (ctx->slice_type == SLICE_I)
    {
        check_max_cu = ctx->param.max_cu_intra;
        check_min_cu = ctx->param.min_cu_intra;
    }
    else
    {
        check_max_cu = ctx->param.max_cu_inter;
        check_min_cu = ctx->param.min_cu_inter;
    }

    set_lambda(ctx, core, ctx->sh, ctx->tile[core->tile_idx].qp);

    if (ctx->sps.chroma_format_idc != 0 && ctx->sps.sps_btt_flag && log2_cuw == 2 && log2_cuh == 2 &&
        (xeve_check_luma(core->tree_cons) || xeve_check_all(core->tree_cons)) && ctx->sps.tool_admvp)
    {
        // Check only for main profile
        xeve_assert(xeve_check_only_intra(core->tree_cons));
    }

    // stroe the previous stored history MV list to m_pSplitTempMotLUTs, backup
    XEVE_HISTORY_BUFFER org_mot_lut, temp_sub_mot_lut;

    if (ctx->sps.tool_hmvp)
    {
        copy_history_buffer(&org_mot_lut, &mcore->tmp_mot_lut[log2_cuw - 2][log2_cuh - 2]);
    }

    core->avail_lr = avail_lr;
    bef_data_idx = xeve_get_lr(core->avail_lr);
    core->bef_data_idx = bef_data_idx;
    if (ctx->pps.cu_qp_delta_enabled_flag)
    {
        bef_data_idx = (!!(qp - ctx->tile[core->tile_idx].qp) << 2) | bef_data_idx;
        core->bef_data_idx = bef_data_idx;
    }
    SBAC_LOAD(core->s_curr_before_split[log2_cuw - 2][log2_cuh - 2], core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);

    //decide allowed split modes for the current node
    //based on CU size located at boundary
    if (cuw > ctx->min_cuwh || cuh > ctx->min_cuwh)
    {
        /***************************** Step 1: decide normatively allowed split modes ********************************/
        int boundary_b = boundary && (y0 + cuh > ctx->h) && !(x0 + cuw > ctx->w);
        int boundary_r = boundary && (x0 + cuw > ctx->w) && !(y0 + cuh > ctx->h);
        xeve_check_split_mode(ctx, split_allow, log2_cuw, log2_cuh, boundary, boundary_r, ctx->log2_max_cuwh
                            , x0, y0, ctx->w, ctx->h, ctx->sps.sps_btt_flag, core->tree_cons.mode_cons);
        //save normatively allowed split modes, as it will be used in in child nodes for entropy coding of split mode
        xeve_mcpy(curr_split_allow, split_allow, sizeof(int)*MAX_SPLIT_NUM);
        for (int i = 1; i < MAX_SPLIT_NUM; i++)
        {
            num_split_to_try += split_allow[i];
        }

        /***************************** Step 2: reduce split modes by fast algorithm ********************************/
        do_split = do_curr = 1;
        if(!boundary)
        {
            nev_max_depth = check_nev_block(ctx, x0, y0, log2_cuw, log2_cuh, &do_curr, &do_split, cud, &nbr_map_skip_flag, core);
            do_split = do_curr = 1;
        }

        check_run_split(core, log2_cuw, log2_cuh, cup, next_split, do_curr, do_split, bef_data_idx, split_allow, boundary, tree_cons);
    }
    else
    {
        split_allow[0] = 1;
        for(int i = 1; i < MAX_SPLIT_NUM; i++)
        {
            split_allow[i] = 0;
        }
    }

    if(!boundary)
    {
        cost_temp = 0.0;
        init_cu_data(&core->cu_data_temp[log2_cuw - 2][log2_cuh - 2], log2_cuw, log2_cuh, ctx->qp, ctx->qp, ctx->qp);

        // copy previous stored history MV list to current cu
        if (ctx->sps.tool_hmvp)
        {
            copy_history_buffer(&mcore->history_buffer, &org_mot_lut);
        }
        ctx->sh->qp_prev_mode = core->dqp_data[log2_cuw - 2][log2_cuh - 2].prev_qp;
        best_dqp = ctx->sh->qp_prev_mode;
        split_mode = NO_SPLIT;
        if(split_allow[split_mode] && (cuw <= check_max_cu && cuh <= check_max_cu))
        {
            if ((cuw > ctx->min_cuwh || cuh > ctx->min_cuwh) && xeve_check_luma(core->tree_cons))
            {
                /* consider CU split mode */
                SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
                xeve_sbac_bit_reset(&core->s_temp_run);
                xeve_set_split_mode(NO_SPLIT, cud, 0, cuw, cuh, cuw, core->cu_data_temp[log2_cuw - 2][log2_cuh - 2].split_mode);
                ctx->fn_eco_split_mode(&core->bs_temp, ctx, core, cud, 0, cuw, cuh, cuw, x0, y0);

                bit_cnt = xeve_get_bit_number(&core->s_temp_run);
                cost_temp += RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt);
                SBAC_STORE(core->s_curr_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_run);
            }
            core->cup = cup;
            int is_dqp_set = 0;
            get_min_max_qp(ctx, core, &min_qp, &max_qp, &is_dqp_set, split_mode, cuw, cuh, qp, x0, y0);
            for (int dqp = min_qp; dqp <= max_qp; dqp++)
            {
                core->qp = GET_QP((s8)qp, dqp - (s8)qp);

                if (ctx->param.aq_mode != 0 || ctx->param.cutree)
                {
                    set_lambda(ctx, core, ctx->sh, core->qp);
                }

                core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].curr_qp = core->qp;
                if (core->cu_qp_delta_code_mode != 2 || is_dqp_set)
                {
                    core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].cu_qp_delta_code = 1 + is_dqp_set;
                    core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].cu_qp_delta_is_coded = 0;
                }
                cost_temp_dqp = cost_temp;
                init_cu_data(&core->cu_data_temp[log2_cuw - 2][log2_cuh - 2], log2_cuw, log2_cuh, ctx->qp, ctx->qp, ctx->qp);

                // copy previous stored history MV list to current cu
                if (ctx->sps.tool_hmvp)
                {
                    copy_history_buffer(&mcore->history_buffer, &org_mot_lut);
                }

                if(ctx->sps.tool_admvp && log2_cuw == 2 && log2_cuh == 2)
                    core->tree_cons.mode_cons = eOnlyIntra;
                clear_map_scu_main(ctx, core, x0, y0, cuw, cuh);
                cost_temp_dqp += mode_coding_unit_main(ctx, core, x0, y0, log2_cuw, log2_cuh, cud, mi);


                if (cost_best > cost_temp_dqp)
                {
                    ibc_flag_dqp = mcore->ibc_flag;
                    cu_mode_dqp = core->cu_mode;
                    dist_cu_best_dqp = core->dist_cu_best;
                    /* backup the current best data */
                    copy_cu_data(&core->cu_data_best[log2_cuw - 2][log2_cuh - 2], &core->cu_data_temp[log2_cuw - 2][log2_cuh - 2], 0, 0, log2_cuw, log2_cuh, log2_cuw, cud, core->tree_cons, ctx->sps.chroma_format_idc);
                    cost_best = cost_temp_dqp;
                    best_split_mode = NO_SPLIT;
                    SBAC_STORE(s_temp_depth, core->s_next_best[log2_cuw - 2][log2_cuh - 2]);
                    DQP_STORE(dqp_temp_depth, core->dqp_next_best[log2_cuw - 2][log2_cuh - 2]);
                    mode_cpy_rec_to_ref(core, x0, y0, cuw, cuh, PIC_MODE(ctx), core->tree_cons, ctx->sps.chroma_format_idc);

                    if (xeve_check_luma(core->tree_cons))
                    {
                        // update history MV list
                        // in mode_coding_unit, ctx->fn_pinter_analyze_cu will store the best MV in mi
                        // if the cost_temp has been update above, the best MV is in mi
                        get_cu_pred_data(&core->cu_data_best[log2_cuw - 2][log2_cuh - 2], 0, 0, log2_cuw, log2_cuh, log2_cuw, cud, mi, ctx, core);

                        if (mi->cu_mode != MODE_INTRA && mi->cu_mode != MODE_IBC && ctx->sps.tool_hmvp)
                        {
                            update_history_buffer_affine(&mcore->history_buffer, mi, ctx->slice_type, core);
                        }

                        if (ctx->sps.tool_hmvp)
                        {
                            copy_history_buffer(&mcore->best_mot_lut[log2_cuw - 2][log2_cuh - 2], &mcore->history_buffer);
                        }
                    }
                }
            }
            if (is_dqp_set && core->cu_qp_delta_code_mode == 2)
            {
                core->cu_qp_delta_code_mode = 0;
            }

            cost_temp = cost_best;
            mcore->ibc_flag = ibc_flag_dqp;
            core->cu_mode = cu_mode_dqp;
            core->dist_cu_best = dist_cu_best_dqp;


#if TRACE_COSTS
            XEVE_TRACE_COUNTER;
            XEVE_TRACE_STR("Block [");
            XEVE_TRACE_INT(x0);
            XEVE_TRACE_STR(", ");
            XEVE_TRACE_INT(y0);
            XEVE_TRACE_STR("]x(");
            XEVE_TRACE_INT(cuw);
            XEVE_TRACE_STR("x");
            XEVE_TRACE_INT(cuh);
            XEVE_TRACE_STR(") split_type ");
            XEVE_TRACE_INT(NO_SPLIT);
            XEVE_TRACE_STR(" cost is ");
            XEVE_TRACE_DOUBLE(cost_temp);
            XEVE_TRACE_STR("\n");
#endif
        }
        else
        {
            cost_temp = MAX_COST;
        }

        if(!mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].split_visit)
        {
            mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].split_cost[split_mode] = cost_temp;
            best_curr_cost = cost_temp;
        }
#if ET_BY_RDC_CHILD_SPLIT
        split_cost[split_mode] = cost_temp;
#endif
        if(split_allow[split_mode] != 0)
        {
            mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].visit = 1;
        }
    }

#if ENC_ECU_ADAPTIVE
    if(cost_best != MAX_COST && cud >= (ctx->poc.poc_val % 2 ? (ctx->sps.sps_btt_flag ? ENC_ECU_DEPTH - 2 : ENC_ECU_DEPTH_B - 2) : (ctx->sps.sps_btt_flag ? ENC_ECU_DEPTH : ENC_ECU_DEPTH_B))
#else
    if(cost_best != MAX_COST && cud >= ENC_ECU_DEPTH
#endif
       && (core->cu_mode == MODE_SKIP || core->cu_mode == MODE_SKIP_MMVD)
       )
    {
        next_split = 0;
    }

    if(cost_best != MAX_COST && ctx->sh->slice_type == SLICE_I && mcore->ibc_flag != 1)
    {
        int dist_cu = core->dist_cu_best;
        int dist_cu_th = 1 << (log2_cuw + log2_cuh + 7);

        if(dist_cu < dist_cu_th)
        {
            u8 bits_inc_by_split = 0;
            bits_inc_by_split += (log2_cuw + log2_cuh >= 6) ? 2 : 0; //two split flags
            bits_inc_by_split += 8; //one more (intra dir + cbf + edi_flag + mtr info) + 1-bit penalty, approximately 8 bits

            if(dist_cu < core->lambda[0] * bits_inc_by_split)
                next_split = 0;
        }
    }

    if((cuw > MIN_CU_SIZE || cuh > MIN_CU_SIZE) && next_split && (cuw > check_min_cu || cuh > check_min_cu))
    {
        SPLIT_MODE split_mode_order[MAX_SPLIT_NUM];
        int split_mode_num = 0;
        core->tree_cons = tree_cons;
        xeve_split_get_split_rdo_order(cuw, cuh, split_mode_order);

        static LOSSY_ES_FUNC lossy_es[MAX_SPLIT_NUM] = {NULL, lossycheck_biver, lossycheck_bihor, lossycheck_ttver, lossycheck_tthor, NULL};

        for(split_mode_num = 1; split_mode_num < MAX_SPLIT_NUM; ++split_mode_num)
        {
            split_mode = split_mode_order[split_mode_num];
            int is_mode_TT = xeve_split_is_TT(split_mode);
            int TT_not_skiped = is_mode_TT ? (best_split_mode != NO_SPLIT || cost_best == MAX_COST) : 1;

            if(split_allow[split_mode] && TT_not_skiped)
            {
                int suco_flag = 0;
                SPLIT_DIR split_dir = xeve_split_get_direction(split_mode);
                int is_mode_BT = xeve_split_is_BT(split_mode);
                u8  allow_suco = ctx->sps.sps_suco_flag ? xeve_check_suco_cond(cuw, cuh, split_mode, boundary, ctx->log2_max_cuwh, ctx->log2_min_cuwh
                                                                             , ctx->sps.log2_diff_ctu_size_max_suco_cb_size, ctx->sps.log2_diff_max_suco_min_suco_cb_size) : 0;
                int num_suco = (split_dir == SPLIT_VER) ? 2 : 1;
                XEVE_SPLIT_STRUCT split_struct;
                double cost_suco[2] = {MAX_COST, MAX_COST};
                int prev_suco_num = is_mode_TT ? 1 : (is_mode_BT ? 0 : 2);
                int prev_suco = mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].suco[prev_suco_num];

                if(lossy_es[split_mode] &&
                   lossy_es[split_mode](&(core->cu_data_best[log2_cuw - 2][log2_cuh - 2]), eval_parent_node_first, cost_best, log2_cuw, log2_cuh, cuw, cuh, cud, nev_max_depth))
                {
                    split_allow[split_mode] = 0;
                }

                if(split_allow[split_mode])
                {
                    num_split_tried++;
                }

                if(is_mode_TT)
                {
                    if(prev_suco == 0 && mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].suco[0] > 0)
                    {
                        prev_suco = mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].suco[0];
                    }
                }
                else
                {
                    if(!is_mode_BT)
                    {
                        // QT case
                        if(prev_suco == 0 && (mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].suco[0] > 0 || mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].suco[1] > 0))
                        {
                            prev_suco = mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].suco[0] > 0 ? mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].suco[0] : mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].suco[1];
                        }
                    }
                }
                xeve_split_get_part_structure_main( split_mode, x0, y0, cuw, cuh, cup, cud, ctx->log2_culine, &split_struct );

                if(split_allow[split_mode])
                {
                    split_struct.tree_cons = tree_cons;

                    BOOL mode_cons_changed = FALSE;
                    BOOL mode_cons_signal = FALSE;

                    if ( ctx->sps.tool_admvp && ctx->sps.sps_btt_flag )
                    {
                        split_struct.tree_cons.changed = tree_cons.mode_cons == eAll && ctx->sps.chroma_format_idc != 0 && !xeve_is_chroma_split_allowed( cuw, cuh, split_mode );
                        mode_cons_changed = xeve_signal_mode_cons( &core->tree_cons, &split_struct.tree_cons );
                        mode_cons_signal = mode_cons_changed && ( ctx->sh->slice_type != SLICE_I ) && ( xeve_get_mode_cons_by_split( split_mode, cuw, cuh ) == eAll ) && (ctx->sps.chroma_format_idc == 1);
                    }

                    for (int mode_num = 0; mode_num < (mode_cons_signal ? 2 : 1); ++mode_num)
                    {
                        if (mode_cons_changed)
                        {
                            xeve_set_tree_mode(&split_struct.tree_cons, mode_num == 0 ? eOnlyIntra : eOnlyInter);
                        }
                        cost_suco[0] = MAX_COST;
                        cost_suco[1] = MAX_COST;
                        for (suco_flag = 0; suco_flag < num_suco; ++suco_flag)
                        {
                            int suco_order[SPLIT_MAX_PART_COUNT];
                            xeve_split_get_suco_order(suco_flag, split_mode, suco_order);
                            int prev_log2_sub_cuw = split_struct.log_cuw[suco_order[0]];
                            int prev_log2_sub_cuh = split_struct.log_cuh[suco_order[0]];
                            int is_dqp_set = 0;
                            if (num_suco == 2)
                            {
                                if (prev_suco > 0 && suco_flag != (prev_suco - 1) && allow_suco)
                                {
                                    continue;
                                }

                                if (!allow_suco && suco_flag != parent_suco)
                                {
                                    continue;
                                }
                            }

                            init_cu_data(&core->cu_data_temp[log2_cuw - 2][log2_cuh - 2], log2_cuw, log2_cuh, ctx->qp, ctx->qp, ctx->qp);
                            clear_map_scu_main(ctx, core, x0, y0, cuw, cuh);

                            int part_num = 0;

                            cost_temp = 0.0;

                            /* When BTT is disabled, split_cu_flag should always be considered although CU is on the picture boundary */
                            if ((ctx->sps.sps_btt_flag == 0) || (x0 + cuw <= ctx->w && y0 + cuh <= ctx->h))
                            {
                                /* consider CU split flag */
                                SBAC_LOAD(core->s_temp_run, core->s_curr_before_split[log2_cuw - 2][log2_cuh - 2]);
                                xeve_sbac_bit_reset(&core->s_temp_run);
                                xeve_set_split_mode(split_mode, cud, 0, cuw, cuh, cuw, core->cu_data_temp[log2_cuw - 2][log2_cuh - 2].split_mode);
                                ctx->fn_eco_split_mode(&core->bs_temp, ctx, core, cud, 0, cuw, cuh, cuw, x0, y0);

                                if (num_suco == 2)
                                {
                                    xeve_set_suco_flag(suco_flag, cud, 0, cuw, cuh, cuw, core->cu_data_temp[log2_cuw - 2][log2_cuh - 2].suco_flag);
                                    xevem_eco_suco_flag(&core->bs_temp, ctx, core, cud, 0, cuw, cuh, cuw, split_mode, boundary, ctx->log2_max_cuwh);
                                }
                                else
                                {
                                    xeve_set_suco_flag(suco_flag, cud, 0, cuw, cuh, cuw, core->cu_data_temp[log2_cuw - 2][log2_cuh - 2].suco_flag);
                                }

                                if (ctx->sps.tool_admvp && ctx->sps.sps_btt_flag && mode_cons_signal)
                                {
                                    xevem_eco_mode_constr(&core->bs_temp, split_struct.tree_cons.mode_cons, core->ctx_flags[CNID_MODE_CONS]);
                                }
                                bit_cnt = xeve_get_bit_number(&core->s_temp_run);
                                cost_temp += RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt);
                                SBAC_STORE(core->s_curr_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_run);
                            }
                            get_min_max_qp(ctx, core, &min_qp, &max_qp, &is_dqp_set, split_mode, cuw, cuh, qp, x0, y0);
                            loop_counter = 0;
                            if (is_dqp_set)
                            {
                                loop_counter = XEVE_ABS(max_qp - min_qp);
                            }
                            cost_best_dqp = MAX_COST;
                            for (dqp_loop = 0; dqp_loop <= loop_counter; dqp_loop++)
                            {
                                int dqp = min_qp + dqp_loop;
                                core->qp = GET_QP((s8)qp, dqp - (s8)qp);

                                if (ctx->param.aq_mode != 0 || ctx->param.cutree != 0)
                                {
                                    set_lambda(ctx, core, ctx->sh, core->qp);
                                }
                                if (is_dqp_set)
                                {
                                    core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].cu_qp_delta_code = 2;
                                    core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].cu_qp_delta_is_coded = 0;
                                    core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].curr_qp = core->qp;
                                }

                                cost_temp_dqp = cost_temp;
                                init_cu_data(&core->cu_data_temp[log2_cuw - 2][log2_cuh - 2], log2_cuw, log2_cuh, ctx->qp, ctx->qp, ctx->qp);
                                clear_map_scu_main(ctx, core, x0, y0, cuw, cuh);

                                if (ctx->sps.tool_hmvp)
                                {
                                    copy_history_buffer(&temp_sub_mot_lut, &org_mot_lut);
                                }

#if TRACE_ENC_CU_DATA_CHECK
                                static int counter_in[MAX_CU_LOG2 - MIN_CU_LOG2][MAX_CU_LOG2 - MIN_CU_LOG2] = { 0, };
                                counter_in[log2_cuw - MIN_CU_LOG2][log2_cuh - MIN_CU_LOG2]++;
#endif

                                for (part_num = 0; part_num < split_struct.part_count; ++part_num)
                                {
                                    int cur_part_num = suco_order[part_num];
                                    int log2_sub_cuw = split_struct.log_cuw[cur_part_num];
                                    int log2_sub_cuh = split_struct.log_cuh[cur_part_num];
                                    int x_pos = split_struct.x_pos[cur_part_num];
                                    int y_pos = split_struct.y_pos[cur_part_num];
                                    int cur_cuw = split_struct.width[cur_part_num];
                                    int cur_cuh = split_struct.height[cur_part_num];

                                    if (ctx->sps.tool_hmvp)
                                    {
                                        copy_history_buffer(&mcore->tmp_mot_lut[log2_sub_cuw - 2][log2_sub_cuh - 2], &temp_sub_mot_lut);
                                    }

                                    if ((x_pos < ctx->w) && (y_pos < ctx->h))
                                    {
                                        if (part_num == 0)
                                        {
                                            SBAC_LOAD(core->s_curr_best[log2_sub_cuw - 2][log2_sub_cuh - 2], core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
                                            DQP_STORE(core->dqp_curr_best[log2_sub_cuw - 2][log2_sub_cuh - 2], core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);
                                        }
                                        else
                                        {
                                            SBAC_LOAD(core->s_curr_best[log2_sub_cuw - 2][log2_sub_cuh - 2], core->s_next_best[prev_log2_sub_cuw - 2][prev_log2_sub_cuh - 2]);
                                            DQP_STORE(core->dqp_curr_best[log2_sub_cuw - 2][log2_sub_cuh - 2], core->dqp_next_best[prev_log2_sub_cuw - 2][prev_log2_sub_cuh - 2]);
                                        }
                                        cost_temp_dqp += mode_coding_tree_main(ctx, core, x_pos, y_pos, split_struct.cup[cur_part_num], log2_sub_cuw, log2_sub_cuh, split_struct.cud[cur_part_num], mi, 1
                                                                             , (num_suco == 2) ? suco_flag : parent_suco, core->qp, split_struct.tree_cons);

                                        core->qp = GET_QP((s8)qp, dqp - (s8)qp);

                                        copy_cu_data(&core->cu_data_temp[log2_cuw - 2][log2_cuh - 2], &core->cu_data_best[log2_sub_cuw - 2][log2_sub_cuh - 2], x_pos - split_struct.x_pos[0]
                                                   , y_pos - split_struct.y_pos[0], log2_sub_cuw, log2_sub_cuh, log2_cuw, cud, split_struct.tree_cons, ctx->sps.chroma_format_idc);

                                        update_map_scu_main(ctx, core, x_pos, y_pos, cur_cuw, cur_cuh);
                                        prev_log2_sub_cuw = log2_sub_cuw;
                                        prev_log2_sub_cuh = log2_sub_cuh;

                                        if (ctx->sps.tool_hmvp)
                                        {
                                            copy_history_buffer(&temp_sub_mot_lut, &mcore->best_mot_lut[log2_sub_cuw - 2][log2_sub_cuh - 2]);
                                        }
                                    }
                                    core->tree_cons = tree_cons;
                                }

                                if (mode_cons_changed && !xeve_check_all(split_struct.tree_cons))
                                {
                                    xeve_assert(xeve_check_only_intra(split_struct.tree_cons));

                                    core->tree_cons = split_struct.tree_cons;
                                    core->tree_cons.tree_type = TREE_C;

                                    XEVE_TRACE_COUNTER;
                                    XEVE_TRACE_STR("Cost luma: ");
                                    XEVE_TRACE_DOUBLE(cost_temp);
                                    XEVE_TRACE_STR("\n");
                                    double cost_node = mode_coding_unit_main(ctx, core, x0, y0, log2_cuw, log2_cuh, cud, mi);
                                    cost_temp_dqp += cost_node;

                                    XEVE_TRACE_STR("Cost chroma: ");
                                    XEVE_TRACE_DOUBLE(cost_node);
                                    XEVE_TRACE_STR("\n");
                                    update_map_scu_main(ctx, core, x0, y0, cuw, cuh);
                                    core->tree_cons = tree_cons;
                                }
#if TRACE_COSTS
                                XEVE_TRACE_COUNTER;
                                XEVE_TRACE_STR("Block [");
                                XEVE_TRACE_INT(x0);
                                XEVE_TRACE_STR(", ");
                                XEVE_TRACE_INT(y0);
                                XEVE_TRACE_STR("]x(");
                                XEVE_TRACE_INT(cuw);
                                XEVE_TRACE_STR("x");
                                XEVE_TRACE_INT(cuh);
                                XEVE_TRACE_STR(") split_type ");
                                XEVE_TRACE_INT(split_mode);
                                XEVE_TRACE_STR(" cost is ");
                                XEVE_TRACE_DOUBLE(cost_temp);
                                XEVE_TRACE_STR("\n");
#endif
#if TRACE_ENC_CU_DATA_CHECK
                                static int counter_out = 0;
                                counter_out++;
                                {
                                    XEVE_CU_DATA *cu_data = &(core->cu_data_temp[log2_cuw - 2][log2_cuh - 2]);
                                    int cuw = 1 << (log2_cuw - MIN_CU_LOG2);
                                    int cuh = 1 << (log2_cuh - MIN_CU_LOG2);
                                    int cus = cuw;
                                    int idx = 0;
                                    for (int j = 0; j < cuh; ++j)
                                    {
                                        int y_pos = y0 + (j << MIN_CU_LOG2);
                                        for (int i = 0; i < cuw; ++i)
                                        {
                                            int x_pos = x0 + (i << MIN_CU_LOG2);
                                            if ((x_pos < ctx->w) && (y_pos < ctx->h))
                                                xeve_assert(cu_data->trace_idx[idx + i] != 0);
                                        }
                                        idx += cus;
                                    }
                                }
#endif
                                if (cost_suco[suco_flag] > cost_temp_dqp)
                                {
                                    cost_suco[suco_flag] = cost_temp_dqp;
                                }
                                if (cost_best_dqp > cost_temp_dqp)
                                {
                                    cost_best_dqp = cost_temp_dqp;
                                }

                                if (cost_best - 0.0001 > cost_temp_dqp)
                                {
                                    /* backup the current best data */
                                    copy_cu_data(&core->cu_data_best[log2_cuw - 2][log2_cuh - 2], &core->cu_data_temp[log2_cuw - 2][log2_cuh - 2]
                                               , 0, 0, log2_cuw, log2_cuh, log2_cuw, cud, core->tree_cons, ctx->sps.chroma_format_idc);
                                    cost_best = cost_temp_dqp;
                                    best_dqp = core->dqp_data[prev_log2_sub_cuw - 2][prev_log2_sub_cuh - 2].prev_qp;
                                    DQP_STORE(dqp_temp_depth, core->dqp_next_best[prev_log2_sub_cuw - 2][prev_log2_sub_cuh - 2]);
                                    SBAC_STORE(s_temp_depth, core->s_next_best[prev_log2_sub_cuw - 2][prev_log2_sub_cuh - 2]);
                                    best_split_mode = split_mode;
                                    best_suco_flag = suco_flag;

                                    if (ctx->sps.tool_hmvp)
                                    {
                                        copy_history_buffer(&mcore->best_mot_lut[log2_cuw - 2][log2_cuh - 2], &temp_sub_mot_lut);
                                    }
                                }
                            }
                            cost_temp = cost_best_dqp;

                            if (is_dqp_set)
                            {
                                core->cu_qp_delta_code_mode = 0;
                            }
                        }
                    }
                }

                if(num_suco == 2)
                {
                    cost_temp = cost_suco[0] < cost_suco[1] ? cost_suco[0] : cost_suco[1];
                }

                if(split_mode != NO_SPLIT && cost_temp < best_split_cost)
                    best_split_cost = cost_temp;

#if ET_BY_RDC_CHILD_SPLIT
                split_cost[split_mode] = cost_temp;
                xeve_mcpy(split_mode_child_rdo[split_mode], split_mode_child, sizeof(int) * 4);
#endif
                if(!mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].split_visit)
                {
                    cost_temp = cost_suco[0] < cost_suco[1] ? cost_suco[0] : cost_suco[1];
                    mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].split_cost[split_mode] = cost_temp;
                }
                else if((mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].remaining_split >> split_mode) & 0x01)
                {
                    cost_temp = cost_suco[0] < cost_suco[1] ? cost_suco[0] : cost_suco[1];
                    mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].split_cost[split_mode] = cost_temp;
                    mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].remaining_split &= ~(1 << split_mode);
                }

                if(num_suco == 2 && mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].suco[prev_suco_num] == 0 && allow_suco)
                {
                    mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].suco[prev_suco_num] = cost_suco[0] < cost_suco[1] ? 1 : 2;
                }
            }

            if(!mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].split_visit && num_split_tried > 0)
            {
                if((best_curr_cost * (1.10)) < best_split_cost)
                {
                    break;
                }
            }

#if ET_BY_RDC_CHILD_SPLIT
            int rdc_bits_th = 5;
            if(cuw < cuh)
            {
                if(split_cost[NO_SPLIT] != MAX_COST && split_cost[SPLIT_BI_HOR] != MAX_COST)
                {
                    if(split_cost[SPLIT_BI_HOR] < split_cost[NO_SPLIT] + core->lambda[0] * rdc_bits_th && split_cost[SPLIT_BI_HOR] > split_cost[NO_SPLIT]
                       && split_mode_child_rdo[SPLIT_BI_HOR][0] == NO_SPLIT && split_mode_child_rdo[SPLIT_BI_HOR][1] == NO_SPLIT)
                    {
                        break;
                    }
                }
            }
            else
            {
                if(split_cost[NO_SPLIT] != MAX_COST && split_cost[SPLIT_BI_VER] != MAX_COST)
                {
                    if(split_cost[SPLIT_BI_VER] < split_cost[NO_SPLIT] + core->lambda[0] * rdc_bits_th && split_cost[SPLIT_BI_VER] > split_cost[NO_SPLIT]
                       && split_mode_child_rdo[SPLIT_BI_VER][0] == NO_SPLIT && split_mode_child_rdo[SPLIT_BI_VER][1] == NO_SPLIT)
                    {
                        break;
                    }
                }
            }
#endif
        }
    }

    if (ctx->sps.tool_hmvp)
    {
        copy_history_buffer(&mcore->tmp_mot_lut[log2_cuw - 2][log2_cuh - 2], &org_mot_lut);
        copy_history_buffer(&mcore->history_buffer, &org_mot_lut);
    }

    mode_cpy_rec_to_ref(core, x0, y0, cuw, cuh, PIC_MODE(ctx), core->tree_cons, ctx->sps.chroma_format_idc);

    /* restore best data */
    xeve_set_split_mode(best_split_mode, cud, 0, cuw, cuh, cuw, core->cu_data_best[log2_cuw - 2][log2_cuh - 2].split_mode);
    xeve_set_suco_flag(best_suco_flag, cud, 0, cuw, cuh, cuw, core->cu_data_best[log2_cuw - 2][log2_cuh - 2].suco_flag);

    SBAC_LOAD(core->s_next_best[log2_cuw - 2][log2_cuh - 2], s_temp_depth);
    DQP_LOAD(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], dqp_temp_depth);

    if(mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].split_visit != 1)
    {
        mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].remaining_split = remaining_split;
    }

    if(num_split_to_try > 0)
    {
        if(best_split_mode == NO_SPLIT)
        {
            if(mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].remaining_split == 0)
            {
                mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].nosplit += 1;
            }
        }
        else
        {
            mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].split += 1;
        }

        mcore->bef_data[log2_cuw - 2][log2_cuh - 2][cup][bef_data_idx].split_visit = 1;
    }

    xeve_assert(cost_best != MAX_COST);
#if TRACE_ENC_CU_DATA_CHECK
    int i, j, w, h, w_scu;
    w = PEL2SCU(core->cuw);
    h = PEL2SCU(core->cuh);
    w_scu = 1 << (log2_cuw - MIN_CU_LOG2);
    for (j = 0; j < h; ++j)
    {
        int y_pos = core->y_pel + (j << MIN_CU_LOG2);
        for (i = 0; i < w; ++i)
        {
            int x_pos = core->x_pel + (i << MIN_CU_LOG2);
            if (x_pos < ctx->w && y_pos < ctx->h)
                xeve_assert(core->cu_data_best[log2_cuw - 2][log2_cuh - 2].trace_idx[i + j * w_scu] != 0);
        }
    }
#endif

    core->tree_cons = tree_cons;

    return (cost_best > MAX_COST) ? MAX_COST : cost_best;
}

static int xevem_mode_init_mt(XEVE_CTX *ctx, int thread_idx)
{
    XEVEM_CTX * mctx = (XEVEM_CTX *)ctx;
    int ret;

    ret = xeve_mode_init_mt(ctx, thread_idx);
    xeve_assert_rv(ret == XEVE_OK, ret);

    if (ctx->param.ibc_flag)
    {
        /* initialize pibc */
        if (mctx->fn_pibc_init_tile)
        {
            ret = mctx->fn_pibc_init_tile(ctx, thread_idx);
            xeve_assert_rv(ret == XEVE_OK, ret);
        }
        if (ctx->param.ibc_hash_search_flag)
        {
            xeve_ibc_hash_rebuild(mctx->ibc_hash, PIC_ORIG(ctx));
        }
    }

    return XEVE_OK;
}

static int mode_init_lcu_main(XEVE_CTX *ctx, XEVE_CORE *core)
{
    XEVEM_CTX  * mctx = (XEVEM_CTX *)ctx;
    XEVEM_CORE * mcore = (XEVEM_CORE *)core;
    int num_size_idx = MAX_TR_LOG2 - MIN_CU_LOG2 + 1;
    int ret;

    mode_init_lcu(ctx, core);

    xeve_mset(mctx->ats_inter_num_pred[core->thread_cnt], 0, sizeof(u8) * num_size_idx * num_size_idx * (ctx->max_cuwh >> MIN_CU_LOG2) * (ctx->max_cuwh >> MIN_CU_LOG2));

    if (ctx->param.ibc_flag)
    {
        /* initialize pibc */
        if (mctx->fn_pibc_init_lcu)
        {
            ret = mctx->fn_pibc_init_lcu(ctx, core);
            xeve_assert_rv(ret == XEVE_OK, ret);
        }
    }

    if (ctx->sps.tool_hmvp)
    {
        if (core->x_lcu == (ctx->tile[core->tile_num].ctba_rs_first) % ctx->w_lcu)
        {
            ret = xeve_hmvp_init(&(mcore->history_buffer));
            xeve_assert_rv(ret == XEVE_OK, ret);
        }

        xeve_hmvp_init(&mcore->tmp_mot_lut[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2]);
        xeve_hmvp_init(&mcore->best_mot_lut[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2]);

        copy_history_buffer(&mcore->tmp_mot_lut[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2], &mcore->history_buffer);
        copy_history_buffer(&mcore->best_mot_lut[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2], &mcore->history_buffer);
    }

    if (ctx->sps.tool_mmvd)
    {
        for (int i = 0; i < PRED_MAX_REF_FRAMES; i++)
        {
            mcore->mmvd_opt.ref_ctu[i] = 0;
            mcore->mmvd_opt.ref_buf_idx[i] = 0;
        }
        for (int i = 0; i < PRED_MAX_I_PERIOD; i++)
        {
            mcore->mmvd_opt.poc_to_idx[i] = -1;
        }
        mcore->mmvd_opt.enabled = 1;
        if (ctx->param.keyint <= 0 || ctx->param.keyint >= PRED_MAX_REF_FRAMES)
        {
            mcore->mmvd_opt.enabled = 0;
        }
    }

    return XEVE_OK;
}

static int mode_post_lcu_main(XEVE_CTX *ctx, XEVE_CORE *core)
{
    XEVEM_CORE *mcore = (XEVEM_CORE *)core;

    if (ctx->sps.tool_hmvp)
    {
        copy_history_buffer(&mcore->history_buffer, &mcore->best_mot_lut[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2]);
    }

    if (ctx->param.ibc_flag && (ctx->param.ibc_fast_method & IBC_FAST_METHOD_ADAPTIVE_SEARCHRANGE) && ctx->param.ibc_hash_search_flag)
    {
        reset_ibc_search_range(ctx, core->x_pel, core->y_pel, ctx->log2_max_cuwh, ctx->log2_max_cuwh, core);
    }
    return XEVE_OK;
}

static void update_to_ctx_map_main(XEVE_CTX *ctx, XEVE_CORE *core)
{
    XEVEM_CTX    *mctx = (XEVEM_CTX *)ctx;
    XEVE_CU_DATA *cu_data;
    int   cuw, cuh, i, j, w, h;
    int   x, y;
    int   core_idx, ctx_idx;
    u8   *map_ats_intra_cu;
    u8   *map_ats_mode_h;
    u8   *map_ats_mode_v;
    u8   *map_ats_inter;

    update_to_ctx_map(ctx, core);

    cu_data = &core->cu_data_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2];
    cuw = ctx->max_cuwh;
    cuh = ctx->max_cuwh;
    x = core->x_pel;
    y = core->y_pel;

    if(x + cuw > ctx->w)
    {
        cuw = ctx->w - x;
    }

    if(y + cuh > ctx->h)
    {
        cuh = ctx->h - y;
    }

    w = cuw >> MIN_CU_LOG2;
    h = cuh >> MIN_CU_LOG2;

    /* copy mode info */
    core_idx = 0;
    ctx_idx = (y >> MIN_CU_LOG2) * ctx->w_scu + (x >> MIN_CU_LOG2);

    map_ats_intra_cu = mctx->map_ats_intra_cu;
    map_ats_mode_h   = mctx->map_ats_mode_h;
    map_ats_mode_v   = mctx->map_ats_mode_v;
    map_ats_inter    = mctx->map_ats_inter;

    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            map_ats_intra_cu[ctx_idx + j] = cu_data->ats_intra_cu[core_idx + j];
            map_ats_mode_h[ctx_idx + j] = cu_data->ats_mode_h[core_idx + j];
            map_ats_mode_v[ctx_idx + j] = cu_data->ats_mode_v[core_idx + j];

            if (core->cu_mode == MODE_IBC)
            {
                map_ats_inter[ctx_idx + j] = 0;
            }
            else
            {
                map_ats_inter[ctx_idx + j] = cu_data->ats_inter_info[core_idx + j];
            }
        }
        ctx_idx += ctx->w_scu;
        core_idx += (ctx->max_cuwh >> MIN_CU_LOG2);
    }

    update_map_scu_main(ctx, core, core->x_pel, core->y_pel, ctx->max_cuwh, ctx->max_cuwh);
}

static int mode_analyze_lcu_main(XEVE_CTX *ctx, XEVE_CORE *core)
{
    XEVE_MODE *mi;
    u32 *map_scu;
    int  w, h;

    mi = &ctx->mode[core->thread_cnt];

    xeve_mset(mi->mvp_idx, 0, sizeof(u8) * REFP_NUM);
    xeve_mset(mi->mvd, 0, sizeof(s16) * REFP_NUM * MV_D);

    /* decide mode */
    mode_coding_tree_main(ctx, core, core->x_pel, core->y_pel, 0, ctx->log2_max_cuwh, ctx->log2_max_cuwh, 0, mi, 1
                        , 0, ctx->tile[core->tile_idx].qp, xeve_get_default_tree_cons() );

#if TRACE_ENC_CU_DATA_CHECK
    h = w = 1 << (ctx->log2_max_cuwh - MIN_CU_LOG2);
    for(j = 0; j < h; ++j)
    {
        int y_pos = core->y_pel + (j << MIN_CU_LOG2);
        for(i = 0; i < w; ++i)
        {
            int x_pos = core->x_pel + (i << MIN_CU_LOG2);
            if(x_pos < ctx->w && y_pos < ctx->h)
                xeve_assert(core->cu_data_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].trace_idx[i + h * j] != 0);
        }
    }
#endif

    update_to_ctx_map_main(ctx, core);
    copy_cu_data(&ctx->map_cu_data[core->lcu_num], &core->cu_data_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2],
                 0, 0, ctx->log2_max_cuwh, ctx->log2_max_cuwh, ctx->log2_max_cuwh, 0, xeve_get_default_tree_cons(), ctx->sps.chroma_format_idc);

#if TRACE_ENC_CU_DATA_CHECK
    h = w = 1 << (ctx->log2_max_cuwh - MIN_CU_LOG2);
    for(j = 0; j < h; ++j)
    {
        int y_pos = core->y_pel + (j << MIN_CU_LOG2);
        for(i = 0; i < w; ++i)
        {
            int x_pos = core->x_pel + (i << MIN_CU_LOG2);
            if(x_pos < ctx->w && y_pos < ctx->h)
                xeve_assert(core->cu_data_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].trace_idx[i + h * j] != 0);
        }
    }
    for(j = 0; j < h; ++j)
    {
        int y_pos = core->y_pel + (j << MIN_CU_LOG2);
        for(i = 0; i < w; ++i)
        {
            int x_pos = core->x_pel + (i << MIN_CU_LOG2);
            if(x_pos < ctx->w && y_pos < ctx->h)
                xeve_assert(ctx->map_cu_data[core->lcu_num].trace_idx[i + h * j] != 0);
        }
    }
#endif

    /* Reset all coded flag for the current lcu */
    core->x_scu = PEL2SCU(core->x_pel);
    core->y_scu = PEL2SCU(core->y_pel);
    map_scu = ctx->map_scu + ((u32)core->y_scu * ctx->w_scu) + core->x_scu;
    w = XEVE_MIN(1 << (ctx->log2_max_cuwh - MIN_CU_LOG2), ctx->w_scu - core->x_scu);
    h = XEVE_MIN(1 << (ctx->log2_max_cuwh - MIN_CU_LOG2), ctx->h_scu - core->y_scu);
    if (ctx->param.cabac_refine)
    {
        int i, j;
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                MCU_CLR_COD(map_scu[j]);
            }
            map_scu += ctx->w_scu;
        }
    }
    return XEVE_OK;
}

void xeve_set_affine_mvf(XEVE_CTX * ctx, XEVE_CORE * core, int w, int h, s8 refi[REFP_NUM], s16 mv[REFP_NUM][VER_NUM][MV_D], int vertex_num)
{
    s8(*map_refi)[REFP_NUM];
    int w_cu;
    int h_cu;
    int scup;
    int w_scu;
    int i, j;
    int lidx;
    int aff_scup[VER_NUM];
    int log2_cuw = XEVE_LOG2(w);
    int log2_cuh = XEVE_LOG2(h);

    scup = core->scup;
    w_cu = w >> MIN_CU_LOG2;
    h_cu = h >> MIN_CU_LOG2;
    w_scu = ctx->w_scu;

    aff_scup[0] = 0;
    aff_scup[1] = (w_cu - 1);
    aff_scup[2] = (h_cu - 1) * w_scu;
    aff_scup[3] = (w_cu - 1) + (h_cu - 1) * w_scu;

    map_refi = ctx->map_refi + scup;
    for (i = 0; i < h_cu; i++)
    {
        for (j = 0; j < w_cu; j++)
        {
            map_refi[j][REFP_0] = refi[REFP_0];
            map_refi[j][REFP_1] = refi[REFP_1];
        }
        map_refi += w_scu;
    }

    // derive sub-block size
    int sub_w = 4, sub_h = 4;
    derive_affine_subblock_size_bi(mv, refi, core->cuw, core->cuh, &sub_w, &sub_h, vertex_num, NULL);

    int   sub_w_in_scu = PEL2SCU(sub_w);
    int   sub_h_in_scu = PEL2SCU(sub_h);
    int   half_w = sub_w >> 1;
    int   half_h = sub_h >> 1;

    for (lidx = 0; lidx < REFP_NUM; lidx++)
    {
        if (refi[lidx] >= 0)
        {
            s16(*ac_mv)[MV_D] = mv[lidx];

            int dmv_hor_x, dmv_ver_x, dmv_hor_y, dmv_ver_y;
            int mv_scale_hor = ac_mv[0][MV_X] << 7;
            int mv_scale_ver = ac_mv[0][MV_Y] << 7;
            int mv_scale_tmp_hor, mv_scale_tmp_ver;

            // convert to 2^(storeBit + iBit) precision
            dmv_hor_x = (ac_mv[1][MV_X] - ac_mv[0][MV_X]) << (7 - log2_cuw);     // deltaMvHor
            dmv_hor_y = (ac_mv[1][MV_Y] - ac_mv[0][MV_Y]) << (7 - log2_cuw);
            if (vertex_num == 3)
            {
                dmv_ver_x = (ac_mv[2][MV_X] - ac_mv[0][MV_X]) << (7 - log2_cuh); // deltaMvVer
                dmv_ver_y = (ac_mv[2][MV_Y] - ac_mv[0][MV_Y]) << (7 - log2_cuh);
            }
            else
            {
                dmv_ver_x = -dmv_hor_y;                                          // deltaMvVer
                dmv_ver_y = dmv_hor_x;
            }

            for (int h = 0; h < h_cu; h += sub_h_in_scu)
            {
                for (int w = 0; w < w_cu; w += sub_w_in_scu)
                {
                    if (w == 0 && h == 0)
                    {
                        mv_scale_tmp_hor = ac_mv[0][MV_X];
                        mv_scale_tmp_ver = ac_mv[0][MV_Y];
                    }
                    else if (w + sub_w_in_scu == w_cu && h == 0)
                    {
                        mv_scale_tmp_hor = ac_mv[1][MV_X];
                        mv_scale_tmp_ver = ac_mv[1][MV_Y];
                    }
                    else if (w == 0 && h + sub_h_in_scu == h_cu && vertex_num == 3)
                    {
                        mv_scale_tmp_hor = ac_mv[2][MV_X];
                        mv_scale_tmp_ver = ac_mv[2][MV_Y];
                    }
                    else
                    {
                        int pos_x = (w << MIN_CU_LOG2) + half_w;
                        int pos_y = (h << MIN_CU_LOG2) + half_h;

                        mv_scale_tmp_hor = mv_scale_hor + dmv_hor_x * pos_x + dmv_ver_x * pos_y;
                        mv_scale_tmp_ver = mv_scale_ver + dmv_hor_y * pos_x + dmv_ver_y * pos_y;

                        // 1/16 precision, 18 bits, same as MC
                        xeve_mv_rounding_s32(mv_scale_tmp_hor, mv_scale_tmp_ver, &mv_scale_tmp_hor, &mv_scale_tmp_ver, 5, 0);

                        mv_scale_tmp_hor = XEVE_CLIP3(-(1 << 17), (1 << 17) - 1, mv_scale_tmp_hor);
                        mv_scale_tmp_ver = XEVE_CLIP3(-(1 << 17), (1 << 17) - 1, mv_scale_tmp_ver);

                        // 1/4 precision, 16 bits for storage
                        mv_scale_tmp_hor >>= 2;
                        mv_scale_tmp_ver >>= 2;
                    }

                    // save MV for each 4x4 block
                    for (int y = h; y < h + sub_h_in_scu; y++)
                    {
                        for (int x = w; x < w + sub_w_in_scu; x++)
                        {
                            int addr_in_scu = scup + x + y * w_scu;
                            ctx->map_mv[addr_in_scu][lidx][MV_X] = (s16)mv_scale_tmp_hor;
                            ctx->map_mv[addr_in_scu][lidx][MV_Y] = (s16)mv_scale_tmp_ver;
                        }
                    }
                }
            }
        }
    }
}

void xeve_mode_rdo_dbk_map_set(XEVE_CTX * ctx, XEVE_CORE *core, int log2_cuw, int log2_cuh, int cbf_l, int scup)
{
    if (((XEVEM_CORE*)core)->ats_inter_info && cbf_l)
    {
        set_cu_cbf_flags(1, ((XEVEM_CORE*)core)->ats_inter_info, log2_cuw, log2_cuh, ctx->map_scu + scup, ctx->w_scu);
    }
}

void xeve_split_tbl_init(XEVE_CTX *ctx)
{
    ctx->split_check[BLOCK_11][IDX_MAX] = ctx->param.framework_cb_max;
    ctx->split_check[BLOCK_11][IDX_MIN] = ctx->param.framework_cb_min;
    ctx->split_check[BLOCK_12][IDX_MAX] = ctx->split_check[BLOCK_11][IDX_MAX];
    ctx->split_check[BLOCK_12][IDX_MIN] = ctx->split_check[BLOCK_11][IDX_MIN] + 1;
    ctx->split_check[BLOCK_14][IDX_MAX] = ctx->param.framework_cu14_max;
    ctx->split_check[BLOCK_14][IDX_MIN] = ctx->split_check[BLOCK_12][IDX_MIN] + 1;
    ctx->split_check[BLOCK_TT][IDX_MAX] = ctx->param.framework_tris_max;
    ctx->split_check[BLOCK_TT][IDX_MIN] = ctx->param.framework_tris_min;
}

void xeve_mode_create_main(XEVE_CTX *ctx)
{
    /* set function addresses */
    ctx->fn_mode_init_mt = xevem_mode_init_mt;
    ctx->fn_mode_init_lcu = mode_init_lcu_main;
    ctx->fn_mode_copy_to_cu_data = copy_to_cu_data_main;
    ctx->fn_mode_reset_intra = mode_reset_intra_main;
    ctx->fn_mode_post_lcu = mode_post_lcu_main;
    ctx->fn_mode_analyze_lcu = mode_analyze_lcu_main;
    ctx->fn_mode_rdo_dbk_map_set = xeve_mode_rdo_dbk_map_set;
    ctx->fn_mode_rdo_bit_cnt_intra_dir = xeve_rdo_bit_cnt_intra_dir_main;

    return;
}
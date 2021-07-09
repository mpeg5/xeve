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
#include "xeve_mode.h"
#include "xeve_ipred.h"
#include <math.h>

typedef int(*LOSSY_ES_FUNC)(XEVE_CU_DATA *, int, double, int, int, int, int, int, int);
static s32 entropy_bits[1024];

void xeve_sbac_bit_reset(XEVE_SBAC * sbac)
{
    sbac->code           &= 0x7FFFF;
    sbac->code_bits       = 11;
    sbac->pending_byte    = 0;
    sbac->is_pending_byte = 0;
    sbac->stacked_ff      = 0;
    sbac->stacked_zero    = 0;
    sbac->bitcounter      = 0;
    sbac->bin_counter     = 0;
}

u32 xeve_get_bit_number(XEVE_SBAC *sbac)
{
    return sbac->bitcounter + 8 * (sbac->stacked_zero + sbac->stacked_ff) + 8 * (sbac->is_pending_byte ? 1 : 0) + 8 - sbac->code_bits + 3;
}

void xeve_rdo_bit_cnt_mvp(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s8 refi[REFP_NUM], s16 mvd[REFP_NUM][MV_D], int pidx, int mvp_idx)
{
    int refi0, refi1;

    if(pidx != PRED_DIR)
    {
        refi0 = refi[REFP_0];
        refi1 = refi[REFP_1];
        if(IS_INTER_SLICE(slice_type) && REFI_IS_VALID(refi0))
        {
            xeve_eco_mvp_idx(&core->bs_temp, mvp_idx);
            xeve_eco_mvd(&core->bs_temp, mvd[REFP_0]);
        }
        if(slice_type == SLICE_B && REFI_IS_VALID(refi1))
        {
            xeve_eco_mvp_idx(&core->bs_temp, mvp_idx);
            xeve_eco_mvd(&core->bs_temp, mvd[REFP_1]);
        }
    }
}

void xeve_rdo_bit_cnt_cu_intra_luma(XEVE_CTX *ctx, XEVE_CORE *core, s32 slice_type, s32 cup, s16 coef[N_C][MAX_CU_DIM])
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

    ctx->fn_mode_rdo_bit_cnt_intra_dir(ctx, core, core->ipm[0]);

    if (ctx->pps.cu_qp_delta_enabled_flag)
    {
        core->cu_qp_delta_code = core->dqp_temp_run.cu_qp_delta_code;
        core->cu_qp_delta_is_coded = core->dqp_temp_run.cu_qp_delta_is_coded;
        ctx->tile[core->tile_idx].qp_prev_eco[core->thread_cnt] = core->dqp_temp_run.prev_qp;
    }

    ctx->fn_eco_coef(ctx, core, &core->bs_temp, coef, MODE_INTRA, 0, 0, RUN_L);

    if (ctx->pps.cu_qp_delta_enabled_flag)
    {
        core->dqp_temp_run.cu_qp_delta_code = core->cu_qp_delta_code;
        core->dqp_temp_run.cu_qp_delta_is_coded = core->cu_qp_delta_is_coded;
        core->dqp_temp_run.prev_qp = ctx->tile[core->tile_idx].qp_prev_eco[core->thread_cnt];
        core->dqp_temp_run.curr_qp = core->qp;
    }
}

void xeve_rdo_bit_cnt_cu_intra_chroma(XEVE_CTX *ctx, XEVE_CORE *core, s32 slice_type, s32 cup, s16 coef[N_C][MAX_CU_DIM])
{
    XEVE_SBAC *sbac = &core->s_temp_run;
    int log2_cuw = core->log2_cuw;
    int log2_cuh = core->log2_cuh;
    int *nnz = core->nnz;

    if (ctx->fn_rdo_intra_ext_c != NULL)
    {
        ctx->fn_rdo_intra_ext_c(ctx, core);
    }

    ctx->fn_eco_coef(ctx, core, &core->bs_temp, coef, MODE_INTRA, 0, 0, RUN_CB | RUN_CR);
}

void xeve_rdo_bit_cnt_intra_dir(XEVE_CTX * ctx, XEVE_CORE * core, int ipm)
{
    xeve_eco_intra_dir(&core->bs_temp, ipm, core->mpm_b_list);
}

void xeve_rdo_bit_cnt_cu_intra(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s32 cup, s16 coef[N_C][MAX_CU_DIM])
{
    XEVE_SBAC *sbac = &core->s_temp_run;
    int log2_cuw = core->log2_cuw;
    int log2_cuh = core->log2_cuh;
    int* nnz = core->nnz;

    if(slice_type != SLICE_I)
    {
        xeve_sbac_encode_bin(0, sbac, core->s_temp_run.ctx.skip_flag + core->ctx_flags[CNID_SKIP_FLAG], &core->bs_temp); /* skip_flag */
        xeve_eco_pred_mode(&core->bs_temp, MODE_INTRA, core->ctx_flags[CNID_PRED_MODE]);
    }

    ctx->fn_mode_rdo_bit_cnt_intra_dir(ctx, core, core->ipm[0]);

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
        core->dqp_temp_run.prev_qp = ctx->tile[core->tile_idx].qp_prev_eco[core->thread_cnt];
        core->dqp_temp_run.curr_qp = core->qp;
    }
}

void xeve_rdo_bit_cnt_cu_inter_comp(XEVE_CORE * core, s16 coef[N_C][MAX_CU_DIM], int ch_type, int pidx, XEVE_CTX * ctx, TREE_CONS tree_cons)
{
    int* nnz = core->nnz;
    XEVE_SBAC* sbac = &core->s_temp_run;
    int log2_cuw = core->log2_cuw;
    int log2_cuh = core->log2_cuh;
    int b_no_cbf = 0;

    if(ch_type == Y_C)
    {
        ctx->fn_eco_coef(ctx, core, &core->bs_temp, coef, MODE_INTER, 0, b_no_cbf, RUN_L);
    }

    if(ch_type == U_C)
    {
        ctx->fn_eco_coef(ctx, core, &core->bs_temp, coef, MODE_INTER, 0, b_no_cbf, RUN_CB);
    }

    if(ch_type == V_C)
    {
        ctx->fn_eco_coef(ctx, core, &core->bs_temp, coef, MODE_INTER, 0, b_no_cbf, RUN_CR);
    }
}

void xeve_rdo_bit_cnt_cu_inter(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s32 cup, s8 refi[REFP_NUM], s16 mvd[REFP_NUM][MV_D], s16 coef[N_C][MAX_CU_DIM], int pidx, u8 * mvp_idx, u8 mvr_idx, u8 bi_idx, s16 affine_mvd[REFP_NUM][VER_NUM][MV_D])
{
    int refi0, refi1;
    int vertex = 0;
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

        xeve_sbac_encode_bin(0, &core->s_temp_run, core->s_temp_run.ctx.skip_flag + core->ctx_flags[CNID_SKIP_FLAG], &core->bs_temp); /* skip_flag */

        if (xeve_check_all_preds(core->tree_cons))
        {
            xeve_eco_pred_mode(&core->bs_temp, MODE_INTER, core->ctx_flags[CNID_PRED_MODE]);
        }

        int dir_flag = (pidx == PRED_DIR);
        xeve_eco_direct_mode_flag(&core->bs_temp, dir_flag);



        if((((pidx % ORG_PRED_NUM) != PRED_DIR) && ((pidx % ORG_PRED_NUM) != PRED_DIR_MMVD)) || ((pidx >= AFF_L0) && (pidx <= AFF_6_BI) && (pidx != AFF_DIR)) )
        {
            xeve_eco_inter_pred_idc(&core->bs_temp, refi, slice_type, 1 << core->log2_cuw, 1 << core->log2_cuh, ctx->sps.tool_admvp);

            refi0 = refi[REFP_0];
            refi1 = refi[REFP_1];
            if(IS_INTER_SLICE(slice_type) && REFI_IS_VALID(refi0))
            {
                xeve_eco_refi(&core->bs_temp, ctx->rpm.num_refp[REFP_0], refi0);
                xeve_eco_mvp_idx(&core->bs_temp, mvp_idx[REFP_0]);
                xeve_eco_mvd(&core->bs_temp, mvd[REFP_0]);
            }

            if(slice_type == SLICE_B && REFI_IS_VALID(refi1))
            {
                xeve_eco_refi(&core->bs_temp, ctx->rpm.num_refp[REFP_1], refi1);
                xeve_eco_mvp_idx(&core->bs_temp, mvp_idx[REFP_1]);
                xeve_eco_mvd(&core->bs_temp, mvd[REFP_1]);
            }
        }
    }

    if (ctx->pps.cu_qp_delta_enabled_flag)
    {
        core->cu_qp_delta_code = core->dqp_temp_run.cu_qp_delta_code;
        core->cu_qp_delta_is_coded = core->dqp_temp_run.cu_qp_delta_is_coded;
        ctx->tile[core->tile_idx].qp_prev_eco[core->thread_cnt] = core->dqp_temp_run.prev_qp;
    }

    ctx->fn_eco_coef(ctx, core, &core->bs_temp, coef, MODE_INTER, ctx->pps.cu_qp_delta_enabled_flag, b_no_cbf, RUN_L | RUN_CB | RUN_CR);

    if (ctx->pps.cu_qp_delta_enabled_flag)
    {
        core->dqp_temp_run.cu_qp_delta_code = core->cu_qp_delta_code;
        core->dqp_temp_run.cu_qp_delta_is_coded = core->cu_qp_delta_is_coded;
        core->dqp_temp_run.prev_qp = ctx->tile[core->tile_idx].qp_prev_eco[core->thread_cnt];
        core->dqp_temp_run.curr_qp = core->qp;
    }
}

void xeve_rdo_bit_cnt_cu_skip(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s32 cup, int mvp_idx0, int mvp_idx1, int c_num , int tool_mmvd)
{
    if(slice_type != SLICE_I)
    {
        xeve_sbac_encode_bin(1, &core->s_temp_run, core->s_temp_run.ctx.skip_flag + core->ctx_flags[CNID_SKIP_FLAG], &core->bs_temp); /* skip_flag */

        xeve_eco_mvp_idx(&core->bs_temp, mvp_idx0);
        if(slice_type == SLICE_B)
        {
            xeve_eco_mvp_idx(&core->bs_temp, mvp_idx1);
        }
    }
}

void xeve_init_bits_est()
{
    int i = 0;
    double p;

    for(i = 0; i < 1024; i++)
    {
        p = (512 * (i + 0.5)) / 1024;
        entropy_bits[i] = (s32)(-32768 * (log(p) / log(2.0) - 9));
    }
}

static s32 biari_no_bits(int symbol, SBAC_CTX_MODEL* cm)
{
    u16 mps, state;

    mps = (*cm) & 1;
    state = (*cm) >> 1;
    state = ((u16)(symbol != 0) != mps) ? state : (512 - state);

    return entropy_bits[state << 1];
}

static void xeve_rdoq_bit_est(XEVE_SBAC * sbac, XEVE_CORE * core)
{
    int bin, ctx;

    for(bin = 0; bin < 2; bin++)
    {
        core->rdoq_est_cbf_luma[bin] = biari_no_bits(bin, sbac->ctx.cbf_luma);
        core->rdoq_est_cbf_cb[bin] = biari_no_bits(bin, sbac->ctx.cbf_cb);
        core->rdoq_est_cbf_cr[bin] = biari_no_bits(bin, sbac->ctx.cbf_cr);
        core->rdoq_est_cbf_all[bin] = biari_no_bits(bin, sbac->ctx.cbf_all);
    }

    for (ctx = 0; ctx < NUM_CTX_SIG_COEFF_FLAG; ctx++)
    {
        for (bin = 0; bin < 2; bin++)
        {
            core->rdoq_est_sig_coeff[ctx][bin] = biari_no_bits(bin, sbac->ctx.sig_coeff_flag + ctx);
        }
    }

    for (ctx = 0; ctx < NUM_CTX_GTX; ctx++)
    {
        for (bin = 0; bin < 2; bin++)
        {
            core->rdoq_est_gtx[ctx][bin] = biari_no_bits(bin, sbac->ctx.coeff_abs_level_greaterAB_flag + ctx);
        }
    }

    for (ctx = 0; ctx < NUM_CTX_LAST_SIG_COEFF; ctx++)
    {
        for (bin = 0; bin < 2; bin++)
        {
            core->rdoq_est_last_sig_coeff_x[ctx][bin] = biari_no_bits(bin, sbac->ctx.last_sig_coeff_x_prefix + ctx);
            core->rdoq_est_last_sig_coeff_y[ctx][bin] = biari_no_bits(bin, sbac->ctx.last_sig_coeff_y_prefix + ctx);
        }
    }

    for(ctx = 0; ctx < NUM_CTX_CC_RUN; ctx++)
    {
        for(bin = 0; bin < 2; bin++)
        {
            core->rdoq_est_run[ctx][bin] = biari_no_bits(bin, sbac->ctx.run + ctx);
        }
    }

    for(ctx = 0; ctx < NUM_CTX_CC_LEVEL; ctx++)
    {
        for(bin = 0; bin < 2; bin++)
        {
            core->rdoq_est_level[ctx][bin] = biari_no_bits(bin, sbac->ctx.level + ctx);
        }
    }

    for(ctx = 0; ctx < NUM_CTX_CC_LAST; ctx++)
    {
        for(bin = 0; bin < 2; bin++)
        {
            core->rdoq_est_last[ctx][bin] = biari_no_bits(bin, sbac->ctx.last + ctx);
        }
    }
}

int init_cu_data(XEVE_CU_DATA *cu_data, int log2_cuw, int log2_cuh, int qp_y, int qp_u, int qp_v)
{
    int i, j;
    int cuw_scu, cuh_scu;

    cuw_scu = 1 << (log2_cuw - MIN_CU_LOG2);
    cuh_scu = 1 << (log2_cuh - MIN_CU_LOG2);

    for(i = 0; i < NUM_CU_DEPTH; i++)
    {
        for(j = 0; j < NUM_BLOCK_SHAPE; j++)
        {
            xeve_mset(cu_data->split_mode[i][j], 0, cuw_scu * cuh_scu * sizeof(s8));
            xeve_mset(cu_data->suco_flag[i][j], 0, cuw_scu * cuh_scu * sizeof(s8));
        }
    }

    xeve_mset(cu_data->qp_y, qp_y, cuw_scu * cuh_scu * sizeof(u8));
    xeve_mset(cu_data->qp_u, qp_u, cuw_scu * cuh_scu * sizeof(u8));
    xeve_mset(cu_data->qp_v, qp_v, cuw_scu * cuh_scu * sizeof(u8));
    xeve_mset(cu_data->mpm[0], 0, cuw_scu * cuh_scu * sizeof(u8));
    xeve_mset(cu_data->mpm[1], 0, cuw_scu * cuh_scu * sizeof(u8));
    xeve_mset(cu_data->ipm[0], 0, cuw_scu * cuh_scu * sizeof(s8));
    xeve_mset(cu_data->ipm[1], 0, cuw_scu * cuh_scu * sizeof(s8));
    for(i = 0; i < 8; i++)
    {
        xeve_mset(cu_data->mpm_ext[i], 0, cuw_scu * cuh_scu * sizeof(u8));
    }
    xeve_mset(cu_data->dmvr_flag, 0, cuw_scu * cuh_scu * sizeof(s8));
    xeve_mset(cu_data->ats_intra_cu, 0, cuw_scu * cuh_scu * sizeof(u8));
    xeve_mset(cu_data->ats_mode_h, 0, cuw_scu * cuh_scu * sizeof(u8));
    xeve_mset(cu_data->ats_mode_v, 0, cuw_scu * cuh_scu * sizeof(u8));
    xeve_mset(cu_data->ats_inter_info, 0, cuw_scu * cuh_scu * sizeof(s8));

#if TRACE_ENC_CU_DATA
    xeve_mset(cu_data->trace_idx, 0, cuw_scu * cuh_scu * sizeof(cu_data->trace_idx[0]));
#endif
#if TRACE_ENC_HISTORIC
    for (i = 0; i < cuw_scu * cuh_scu; ++i)
    {
        cu_data->history_buf->currCnt = 0;
        cu_data->history_buf->m_maxCnt = ALLOWED_CHECKED_NUM;
#if TRACE_ENC_CU_DATA
        xeve_mset(cu_data->history_buf->history_cu_table, 0x00, ALLOWED_CHECKED_NUM * sizeof(cu_data->history_buf->history_cu_table[0]));
#endif
        xeve_mset(&cu_data->history_buf->history_mv_table[0], 0x00, ALLOWED_CHECKED_NUM * sizeof(cu_data->history_buf->history_mv_table[0]) * REFP_NUM * MV_D);
        xeve_mset(&cu_data->history_buf->history_refi_table[0], 0x00, ALLOWED_CHECKED_NUM * sizeof(cu_data->history_buf->history_refi_table[0]) * REFP_NUM);
    }
#endif

    return XEVE_OK;
}

int copy_cu_data(XEVE_CU_DATA *dst, XEVE_CU_DATA *src, int x, int y, int log2_cuw, int log2_cuh, int log2_cus, int cud, TREE_CONS tree_cons, int chroma_format_idc)
{
    int i, j, k;
    int cuw, cuh, cus;
    int cuw_scu, cuh_scu, cus_scu;
    int cx, cy;
    int size, idx_dst, idx_src;
    int w_shift = (XEVE_GET_CHROMA_W_SHIFT(chroma_format_idc));
    int h_shift = (XEVE_GET_CHROMA_H_SHIFT(chroma_format_idc));

    cx = x >> MIN_CU_LOG2;    //x = position in LCU, cx = 4x4 CU horizontal index
    cy = y >> MIN_CU_LOG2;    //y = position in LCU, cy = 4x4 CU vertical index

    cuw = 1 << log2_cuw;    //current CU width
    cuh = 1 << log2_cuh;    //current CU height
    cus = 1 << log2_cus;    //current CU buffer stride (= current CU width)
    cuw_scu = 1 << (log2_cuw - MIN_CU_LOG2);    //4x4 CU number in width
    cuh_scu = 1 << (log2_cuh - MIN_CU_LOG2);    //4x4 CU number in height
    cus_scu = 1 << (log2_cus - MIN_CU_LOG2);    //4x4 CU number in stride

    // only copy src's first row of 4x4 CUs to dis's all 4x4 CUs
    if (xeve_check_luma(tree_cons))
    {
        for (j = 0; j < cuh_scu; j++)
        {
            idx_dst = (cy + j) * cus_scu + cx;
            idx_src = j * cuw_scu;

            size = cuw_scu * sizeof(s8);
            for (k = cud; k < NUM_CU_DEPTH; k++)
            {
                for (i = 0; i < NUM_BLOCK_SHAPE; i++)
                {
                    xeve_mcpy(dst->split_mode[k][i] + idx_dst, src->split_mode[k][i] + idx_src, size);
                    xeve_mcpy(dst->suco_flag[k][i] + idx_dst, src->suco_flag[k][i] + idx_src, size);
                }
            }

            xeve_mcpy(dst->ats_intra_cu + idx_dst, src->ats_intra_cu + idx_src, size);
            xeve_mcpy(dst->ats_mode_h + idx_dst, src->ats_mode_h + idx_src, size);
            xeve_mcpy(dst->ats_mode_v + idx_dst, src->ats_mode_v + idx_src, size);
            xeve_mcpy(dst->ats_inter_info + idx_dst, src->ats_inter_info + idx_src, size);
            xeve_mcpy(dst->qp_y + idx_dst, src->qp_y + idx_src, size);
            xeve_mcpy(dst->pred_mode + idx_dst, src->pred_mode + idx_src, size);
            xeve_mcpy(dst->mpm[0] + idx_dst, src->mpm[0] + idx_src, size);
            xeve_mcpy(dst->mpm[1] + idx_dst, src->mpm[1] + idx_src, size);
            xeve_mcpy(dst->ipm[0] + idx_dst, src->ipm[0] + idx_src, size);

            for (i = 0; i < 8; i++)
            {
                xeve_mcpy(dst->mpm_ext[i] + idx_dst, src->mpm_ext[i] + idx_src, size);
            }
            xeve_mcpy(dst->skip_flag + idx_dst, src->skip_flag + idx_src, size);
            xeve_mcpy(dst->ibc_flag + idx_dst, src->ibc_flag + idx_src, size);
            xeve_mcpy(dst->dmvr_flag + idx_dst, src->dmvr_flag + idx_src, size);
            xeve_mcpy(dst->mmvd_flag + idx_dst, src->mmvd_flag + idx_src, size);
            xeve_mcpy(dst->affine_flag + idx_dst, src->affine_flag + idx_src, size);
            xeve_mcpy(dst->depth + idx_dst, src->depth + idx_src, size);

            size = cuw_scu * sizeof(u32);
            xeve_mcpy(dst->map_scu + idx_dst, src->map_scu + idx_src, size);
            xeve_mcpy(dst->map_affine + idx_dst, src->map_affine + idx_src, size);
            xeve_mcpy(dst->map_cu_mode + idx_dst, src->map_cu_mode + idx_src, size);

            size = cuw_scu * sizeof(u8) * REFP_NUM;
            xeve_mcpy(*(dst->refi + idx_dst), *(src->refi + idx_src), size);
            xeve_mcpy(*(dst->mvp_idx + idx_dst), *(src->mvp_idx + idx_src), size);

            size = cuw_scu * sizeof(u8);
            xeve_mcpy(dst->mvr_idx + idx_dst, src->mvr_idx + idx_src, size);

            size = cuw_scu * sizeof(u8);
            xeve_mcpy(dst->bi_idx + idx_dst, src->bi_idx + idx_src, size);

            size = cuw_scu * sizeof(s16);
            xeve_mcpy(dst->mmvd_idx + idx_dst, src->mmvd_idx + idx_src, size);

            size = cuw_scu * sizeof(s16) * REFP_NUM * MV_D;
            xeve_mcpy(dst->mv + idx_dst, src->mv + idx_src, size);
            xeve_mcpy(dst->unrefined_mv + idx_dst, src->unrefined_mv + idx_src, size);
            xeve_mcpy(dst->mvd + idx_dst, src->mvd + idx_src, size);

            size = cuw_scu * sizeof(int);
            k = Y_C;
            {
                xeve_mcpy(dst->nnz[k] + idx_dst, src->nnz[k] + idx_src, size);

                for (i = 0; i < MAX_SUB_TB_NUM; i++)
                {
                    xeve_mcpy(dst->nnz_sub[k][i] + idx_dst, src->nnz_sub[k][i] + idx_src, size);
                }
            }

#if TRACE_ENC_CU_DATA
            size = cuw_scu * sizeof(dst->trace_idx[0]);
            xeve_mcpy(dst->trace_idx + idx_dst, src->trace_idx + idx_src, size);
#endif
#if TRACE_ENC_HISTORIC
            size = cuw_scu * sizeof(dst->history_buf[0]);
            xeve_mcpy(dst->history_buf + idx_dst, src->history_buf + idx_src, size);
#endif
        }

        for(j = 0; j < cuh; j++)
        {
            idx_dst = (y + j) * cus + x;
            idx_src = j * cuw;

            size = cuw * sizeof(s16);
            xeve_mcpy(dst->coef[Y_C] + idx_dst, src->coef[Y_C] + idx_src, size);
            size = cuw * sizeof(pel);
            xeve_mcpy(dst->reco[Y_C] + idx_dst, src->reco[Y_C] + idx_src, size);
        }
    }
    if (xeve_check_chroma(tree_cons) && chroma_format_idc)
    {
        for(j = 0; j < cuh >> h_shift; j++)
        {
            idx_dst = ((y >> h_shift) + j) * (cus >> w_shift) + (x >> w_shift);
            idx_src = j * (cuw >> w_shift);

            size = (cuw >> w_shift) * sizeof(s16);
            xeve_mcpy(dst->coef[U_C] + idx_dst, src->coef[U_C] + idx_src, size);
            xeve_mcpy(dst->coef[V_C] + idx_dst, src->coef[V_C] + idx_src, size);
            size = (cuw >> w_shift) * sizeof(pel);
            xeve_mcpy(dst->reco[U_C] + idx_dst, src->reco[U_C] + idx_src, size);
            xeve_mcpy(dst->reco[V_C] + idx_dst, src->reco[V_C] + idx_src, size);
        }

        for (j = 0; j < cuh_scu; j++)
        {
            idx_dst = (cy + j) * cus_scu + cx;
            idx_src = j * cuw_scu;

            size = cuw_scu * sizeof(s8);
            xeve_mcpy(dst->qp_u + idx_dst, src->qp_u + idx_src, size);
            xeve_mcpy(dst->qp_v + idx_dst, src->qp_v + idx_src, size);
            xeve_mcpy(dst->ipm[1] + idx_dst, src->ipm[1] + idx_src, size);
            xeve_mcpy(dst->pred_mode_chroma + idx_dst, src->pred_mode_chroma + idx_src, size);

            size = cuw_scu * sizeof(int);
            for (k = U_C; k < N_C; k++)
            {
                xeve_mcpy(dst->nnz[k] + idx_dst, src->nnz[k] + idx_src, size);

                for (i = 0; i < MAX_SUB_TB_NUM; i++)
                {
                    xeve_mcpy(dst->nnz_sub[k][i] + idx_dst, src->nnz_sub[k][i] + idx_src, size);
                }
            }
        }
    }

    return XEVE_OK;
}

int get_cu_pred_data(XEVE_CU_DATA *src, int x, int y, int log2_cuw, int log2_cuh, int log2_cus, int cud, XEVE_MODE *mi, XEVE_CTX *ctx, XEVE_CORE *core)
{
    int cuw, cuh, cus;
    int cuw_scu, cuh_scu, cus_scu;
    int cx, cy;
    int idx_src;

    cx = x >> MIN_CU_LOG2;    //x = position in LCU, cx = 4x4 CU horizontal index
    cy = y >> MIN_CU_LOG2;    //y = position in LCU, cy = 4x4 CU vertical index

    cuw = 1 << log2_cuw;    //current CU width
    cuh = 1 << log2_cuh;    //current CU height
    cus = 1 << log2_cus;    //current CU buffer stride (= current CU width)
    cuw_scu = 1 << (log2_cuw - MIN_CU_LOG2);    //4x4 CU number in width
    cuh_scu = 1 << (log2_cuh - MIN_CU_LOG2);    //4x4 CU number in height
    cus_scu = 1 << (log2_cus - MIN_CU_LOG2);    //4x4 CU number in stride

    // only copy src's first row of 4x4 CUs to dis's all 4x4 CUs
    idx_src = cy * cus_scu + cx;

    mi->cu_mode = src->pred_mode[idx_src];
    mi->affine_flag = src->affine_flag[idx_src];
    mi->mv[REFP_0][MV_X] = src->mv[idx_src][REFP_0][MV_X];
    mi->mv[REFP_0][MV_Y] = src->mv[idx_src][REFP_0][MV_Y];
    mi->mv[REFP_1][MV_X] = src->mv[idx_src][REFP_1][MV_X];
    mi->mv[REFP_1][MV_Y] = src->mv[idx_src][REFP_1][MV_Y];

    mi->refi[REFP_0] = src->refi[idx_src][REFP_0];
    mi->refi[REFP_1] = src->refi[idx_src][REFP_1];

#if TRACE_ENC_CU_DATA
    mi->trace_cu_idx = src->trace_idx[idx_src];
#endif
#if TRACE_ENC_HISTORIC
    xeve_mcpy(&mi->history_buf, src->history_buf + idx_src, sizeof(mi->history_buf));
#endif

#if TRACE_ENC_CU_DATA_CHECK
    xeve_assert(mi->trace_cu_idx != 0);
#endif
    return XEVE_OK;
}
int get_averaged_qp(s8 * map_dqp, int x_scu, int y_scu,  int w_scu, int h_scu, int cuw, int cuh)
{
    int i, j, cnt, aver_qp;
    int w, h;

    w = cuw >> MIN_CU_LOG2;
    h = cuh >> MIN_CU_LOG2;

    cnt = 0;
    aver_qp = 0;
    for (i = y_scu; i < y_scu + h; i++)
    {
        if (i >= h_scu) continue;
        for (j = x_scu; j < x_scu + w; j++)
        {
            if (j >= w_scu) continue;
            aver_qp += map_dqp[i * w_scu + j];
            cnt++;
        }
    }
    if (cnt)
        return aver_qp / cnt;
    else
        return 0;

}

void set_lambda(XEVE_CTX * ctx, XEVE_CORE * core, XEVE_SH *sh, s8 qp)
{

    int qp_c_i, qp_u, qp_v;

    qp_u = (s8)XEVE_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, qp + sh->qp_u_offset);
    qp_v = (s8)XEVE_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, qp + sh->qp_v_offset);

    core->lambda[0] = 0.57 * pow(2.0, (qp- 12.0) / 3.0);
    qp_c_i = ctx->qp_chroma_dynamic[0][qp_u];
    core->dist_chroma_weight[0] = pow(2.0, (qp- qp_c_i) / 3.0);
    qp_c_i = ctx->qp_chroma_dynamic[1][qp_v];
    core->dist_chroma_weight[1] = pow(2.0, (qp- qp_c_i) / 3.0);
    core->lambda[1] = core->lambda[0] / core->dist_chroma_weight[0];
    core->lambda[2] = core->lambda[0] / core->dist_chroma_weight[1];
    core->sqrt_lambda[0] = sqrt(core->lambda[0]);
    core->sqrt_lambda[1] = sqrt(core->lambda[1]);
    core->sqrt_lambda[2] = sqrt(core->lambda[2]);
}


void get_min_max_qp(XEVE_CTX * ctx, XEVE_CORE *core, s8 * min_qp, s8 * max_qp, int * is_dqp_set, SPLIT_MODE split_mode, int cuw, int cuh, u8 qp, int x0, int y0)
{
    s8  dqp;
    u8  qp0;
    u8  min_dqp, max_dqp;
    u16 x_scu = PEL2SCU(x0);
    u16 y_scu = PEL2SCU(y0);

    *is_dqp_set = 0;
    if (!ctx->pps.cu_qp_delta_enabled_flag)
    {
        *min_qp = ctx->tile[core->tile_idx].qp;
        *max_qp = ctx->tile[core->tile_idx].qp;
    }
    else
    {
        if (ctx->param.aq_mode != 0 || ctx->param.cutree != 0)
        {

            dqp = get_averaged_qp(ctx->map_dqp_lah, x_scu, y_scu, ctx->w_scu, ctx->h_scu, cuw, cuh);
            qp0 = ctx->tile[core->tile_idx].qp;
            max_dqp = min_dqp = qp0 + dqp;
        }
        else
        {
            min_dqp = ctx->tile[core->tile_idx].qp;
            max_dqp = ctx->tile[core->tile_idx].qp + ctx->sh->dqp;
        }

        if (!(ctx->sps.dquant_flag))
        {
            if (split_mode != NO_SPLIT)
            {
                *min_qp = qp;
                *max_qp = qp;
            }
            else
            {
                *min_qp = min_dqp;
                *max_qp = max_dqp;
            }
        }
        else
        {
            *min_qp = qp;
            *max_qp = qp;

            if (split_mode == NO_SPLIT && (XEVE_LOG2(cuw) + XEVE_LOG2(cuh) >= ctx->pps.cu_qp_delta_area) && core->cu_qp_delta_code_mode != 2)
            {
                core->cu_qp_delta_code_mode = 1;
                *min_qp = min_dqp;
                *max_qp = max_dqp;

                if (XEVE_LOG2(cuw) == 7 || XEVE_LOG2(cuh) == 7)
                {
                    *is_dqp_set = 1;
                    core->cu_qp_delta_code_mode = 2;
                }
                else
                {
                    *is_dqp_set = 0;
                }
            }
            else if ((((XEVE_LOG2(cuw) + XEVE_LOG2(cuh) == ctx->pps.cu_qp_delta_area + 1) && (split_mode == SPLIT_TRI_VER || split_mode == SPLIT_TRI_HOR)) ||
                (XEVE_LOG2(cuh) + XEVE_LOG2(cuw) == ctx->pps.cu_qp_delta_area && core->cu_qp_delta_code_mode != 2)))
            {
                core->cu_qp_delta_code_mode = 2;
                *is_dqp_set = 1;
                *min_qp = min_dqp;
                *max_qp = max_dqp;
            }
        }
    }

    *min_qp = XEVE_CLIP3(1, 51, *min_qp);
    *max_qp = XEVE_CLIP3(1, 51, *max_qp);
}

int mode_cu_init(XEVE_CTX * ctx, XEVE_CORE * core, int x, int y, int log2_cuw, int log2_cuh, int cud)
{
#if TRACE_ENC_CU_DATA
    static u64  trace_idx = 1;
    core->trace_idx = trace_idx++;
#endif
    core->cuw = 1 << log2_cuw;
    core->cuh = 1 << log2_cuh;
    core->log2_cuw = log2_cuw;
    core->log2_cuh = log2_cuh;
    core->x_scu = PEL2SCU(x);
    core->y_scu = PEL2SCU(y);
    core->scup = ((u32)core->y_scu * ctx->w_scu) + core->x_scu;
    core->avail_cu = 0;
    core->avail_lr = LR_10;
    core->nnz[Y_C] = core->nnz[U_C] = core->nnz[V_C] = 0;
    xeve_mset(core->nnz_sub, 0, sizeof(int) * N_C * MAX_SUB_TB_NUM);
    core->cud = cud;
    core->cu_mode = MODE_INTRA;

    /* Getting the appropriate QP based on dqp table*/
    int qp_i_cb, qp_i_cr;

    core->qp_y = GET_LUMA_QP(core->qp, ctx->sps.bit_depth_luma_minus8);
    qp_i_cb = XEVE_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, core->qp + ctx->sh->qp_u_offset);
    qp_i_cr = XEVE_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, core->qp + ctx->sh->qp_v_offset);
    core->qp_u = ctx->qp_chroma_dynamic[0][qp_i_cb] + 6 * ctx->sps.bit_depth_chroma_minus8;
    core->qp_v = ctx->qp_chroma_dynamic[1][qp_i_cr] + 6 * ctx->sps.bit_depth_chroma_minus8;

    XEVE_PINTER *pi = &ctx->pinter[core->thread_cnt];

    pi->qp_y = core->qp_y;
    pi->qp_u = core->qp_u;
    pi->qp_v = core->qp_v;

    xeve_rdoq_bit_est(&core->s_curr_best[log2_cuw - 2][log2_cuh - 2],  core);

    return XEVE_OK;
}

void mode_cpy_rec_to_ref(XEVE_CORE *core, int x, int y, int w, int h, XEVE_PIC *pic, TREE_CONS tree_cons, int chroma_format_idc)
{
    XEVE_CU_DATA * cu_data;
    pel          * src, * dst;
    int            j, s_pic, off, size;
    int            log2_w, log2_h;
    int            stride;
    int            w_shift = (XEVE_GET_CHROMA_W_SHIFT(chroma_format_idc));
    int            h_shift = (XEVE_GET_CHROMA_H_SHIFT(chroma_format_idc));

    log2_w = XEVE_LOG2(w);
    log2_h = XEVE_LOG2(h);

    cu_data = &core->cu_data_best[log2_w - 2][log2_h - 2];

    s_pic = pic->s_l;

    stride = w;

    if (x + w > pic->w_l)
    {
        w = pic->w_l - x;
    }

    if (y + h > pic->h_l)
    {
        h = pic->h_l - y;
    }

    if (xeve_check_luma(tree_cons))
    {
        /* luma */
        src = cu_data->reco[Y_C];
        dst = pic->y + x + y * s_pic;
        size = sizeof(pel) * w;

        for (j = 0; j < h; j++)
        {
            xeve_mcpy(dst, src, size);
            src += stride;
            dst += s_pic;
        }

    }

    if (xeve_check_chroma(tree_cons) && chroma_format_idc)
    {
        /* chroma */
        s_pic = pic->s_c;
        off = (x >> w_shift) + (y >> h_shift) * s_pic;
        size = (sizeof(pel) * w) >> w_shift;

        src = cu_data->reco[U_C];
        dst = pic->u + off;
        for (j = 0; j < (h >> h_shift); j++)
        {
            xeve_mcpy(dst, src, size);
            src += (stride >> w_shift);
            dst += s_pic;
        }

        src = cu_data->reco[V_C];
        dst = pic->v + off;
        for (j = 0; j < (h >> h_shift); j++)
        {
            xeve_mcpy(dst, src, size);
            src += (stride >> w_shift);
            dst += s_pic;
        }
    }
}

void copy_to_cu_data(XEVE_CTX *ctx, XEVE_CORE *core, XEVE_MODE *mi, s16 coef_src[N_C][MAX_CU_DIM])
{
    XEVE_CU_DATA *cu_data;
    int i, j, idx;
    u32 size;
    int log2_cuw, log2_cuh;

    log2_cuw = XEVE_LOG2(core->cuw);
    log2_cuh = XEVE_LOG2(core->cuh);

    cu_data = &core->cu_data_temp[log2_cuw - 2][log2_cuh - 2];

    if (xeve_check_luma(core->tree_cons))
    {
    /* copy coef */
    size = core->cuw * core->cuh * sizeof(s16);
    xeve_mcpy(cu_data->coef[Y_C], coef_src[Y_C], size);

    /* copy reco */
    size = core->cuw * core->cuh * sizeof(pel);
    xeve_mcpy(cu_data->reco[Y_C], mi->rec[Y_C], size);

#if TRACE_ENC_CU_DATA_CHECK
    xeve_assert(core->trace_idx == mi->trace_cu_idx);
    xeve_assert(core->trace_idx != 0);
#endif

    /* copy mode info */
    idx = 0;
    for(j = 0; j < core->cuh >> MIN_CU_LOG2; j++)
    {
        for(i = 0; i < core->cuw >> MIN_CU_LOG2; i++)
        {
            cu_data->pred_mode[idx + i] = core->cu_mode;
            cu_data->skip_flag[idx + i] = ((core->cu_mode == MODE_SKIP) || (core->cu_mode == MODE_SKIP_MMVD)) ? 1 : 0;
            cu_data->mmvd_flag[idx + i] = core->cu_mode == MODE_SKIP_MMVD ? 1 : 0;
            cu_data->nnz[Y_C][idx + i] = core->nnz[Y_C];

            for (int sb = 0; sb < MAX_SUB_TB_NUM; sb++)
            {
               cu_data->nnz_sub[Y_C][sb][idx + i] = core->nnz_sub[Y_C][sb];
            }
            cu_data->qp_y[idx + i] = core->qp_y;
            MCU_CLR_QP(cu_data->map_scu[idx + i]);
            if (ctx->pps.cu_qp_delta_enabled_flag)
            {
                MCU_SET_IF_COD_SN_QP(cu_data->map_scu[idx + i], core->cu_mode == MODE_INTRA, ctx->slice_num, core->qp);
            }
            else
            {
                MCU_SET_IF_COD_SN_QP(cu_data->map_scu[idx + i], core->cu_mode == MODE_INTRA, ctx->slice_num, ctx->tile[core->tile_idx].qp);
            }

            if(cu_data->skip_flag[idx + i])
            {
                MCU_SET_SF(cu_data->map_scu[idx + i]);
            }
            else
            {
                MCU_CLR_SF(cu_data->map_scu[idx + i]);
            }

            cu_data->depth[idx + i] = core->cud;

            MCU_SET_LOGW(cu_data->map_cu_mode[idx + i], log2_cuw);
            MCU_SET_LOGH(cu_data->map_cu_mode[idx + i], log2_cuh);

            if(core->cu_mode == MODE_SKIP_MMVD)
            {
                MCU_SET_MMVDS(cu_data->map_cu_mode[idx + i]);
            }
            else
            {
                MCU_CLR_MMVDS(cu_data->map_cu_mode[idx + i]);
            }

            if(core->cu_mode == MODE_INTRA)
            {
                cu_data->ipm[0][idx + i] = core->ipm[0];
                cu_data->mv[idx + i][REFP_0][MV_X] = 0;
                cu_data->mv[idx + i][REFP_0][MV_Y] = 0;
                cu_data->mv[idx + i][REFP_1][MV_X] = 0;
                cu_data->mv[idx + i][REFP_1][MV_Y] = 0;
                cu_data->refi[idx + i][REFP_0] = -1;
                cu_data->refi[idx + i][REFP_1] = -1;
            }
            else if (core->cu_mode == MODE_IBC)
            {
              cu_data->refi[idx + i][REFP_0] = -1;
              cu_data->refi[idx + i][REFP_1] = -1;
              cu_data->mvp_idx[idx + i][REFP_0] = mi->mvp_idx[REFP_0];
              cu_data->mvp_idx[idx + i][REFP_1] = 0;
              cu_data->mv[idx + i][REFP_0][MV_X] = mi->mv[REFP_0][MV_X];
              cu_data->mv[idx + i][REFP_0][MV_Y] = mi->mv[REFP_0][MV_Y];
              cu_data->mv[idx + i][REFP_1][MV_X] = 0;
              cu_data->mv[idx + i][REFP_1][MV_Y] = 0;
              cu_data->mvd[idx + i][REFP_0][MV_X] = mi->mvd[REFP_0][MV_X];
              cu_data->mvd[idx + i][REFP_0][MV_Y] = mi->mvd[REFP_0][MV_Y];
            }
            else
            {
                cu_data->refi[idx + i][REFP_0] = mi->refi[REFP_0];
                cu_data->refi[idx + i][REFP_1] = mi->refi[REFP_1];
                cu_data->mvp_idx[idx + i][REFP_0] = mi->mvp_idx[REFP_0];
                cu_data->mvp_idx[idx + i][REFP_1] = mi->mvp_idx[REFP_1];
                cu_data->mvr_idx[idx + i] = mi->mvr_idx;
                cu_data->bi_idx[idx + i] = mi->bi_idx;
                cu_data->mmvd_idx[idx + i] = mi->mmvd_idx;

                cu_data->mv[idx + i][REFP_0][MV_X] = mi->mv[REFP_0][MV_X];
                cu_data->mv[idx + i][REFP_0][MV_Y] = mi->mv[REFP_0][MV_Y];
                cu_data->mv[idx + i][REFP_1][MV_X] = mi->mv[REFP_1][MV_X];
                cu_data->mv[idx + i][REFP_1][MV_Y] = mi->mv[REFP_1][MV_Y];

                cu_data->mvd[idx + i][REFP_0][MV_X] = mi->mvd[REFP_0][MV_X];
                cu_data->mvd[idx + i][REFP_0][MV_Y] = mi->mvd[REFP_0][MV_Y];
                cu_data->mvd[idx + i][REFP_1][MV_X] = mi->mvd[REFP_1][MV_X];
                cu_data->mvd[idx + i][REFP_1][MV_Y] = mi->mvd[REFP_1][MV_Y];
            }
#if TRACE_ENC_CU_DATA
            cu_data->trace_idx[idx + i] = core->trace_idx;
#endif
#if TRACE_ENC_HISTORIC
            xeve_mcpy(cu_data->history_buf + idx + i, &core->history_buffer, sizeof(core->history_buffer));
#endif
        }

        idx += core->cuw >> MIN_CU_LOG2;
    }
#if TRACE_ENC_CU_DATA_CHECK
    int w = PEL2SCU(core->cuw);
    int h = PEL2SCU(core->cuh);
    idx = 0;
    for (j = 0; j < h; ++j, idx += w)
    {
        for (i = 0; i < w; ++i)
        {
            xeve_assert(cu_data->trace_idx[idx + i] == core->trace_idx);
        }
    }
#endif
    }
    if (xeve_check_chroma(core->tree_cons) && ctx->sps.chroma_format_idc)
    {
        /* copy coef */
        size = (core->cuw * core->cuh * sizeof(s16)) >> (ctx->param.cs_h_shift + ctx->param.cs_w_shift);
        xeve_mcpy(cu_data->coef[U_C], coef_src[U_C], size);
        xeve_mcpy(cu_data->coef[V_C], coef_src[V_C], size);

        /* copy reco */
        size = (core->cuw * core->cuh * sizeof(pel)) >> (ctx->param.cs_h_shift + ctx->param.cs_w_shift);
        xeve_mcpy(cu_data->reco[U_C], mi->rec[U_C], size);
        xeve_mcpy(cu_data->reco[V_C], mi->rec[V_C], size);

        /* copy mode info */
        idx = 0;
        for (j = 0; j < core->cuh >> MIN_CU_LOG2; j++)
        {
            for (i = 0; i < core->cuw >> MIN_CU_LOG2; i++)
            {
                cu_data->pred_mode_chroma[idx + i] = core->cu_mode;
                cu_data->nnz[U_C][idx + i] = core->nnz[U_C];
                cu_data->nnz[V_C][idx + i] = core->nnz[V_C];
                for (int c = U_C; c < N_C; c++)
                {
                    for (int sb = 0; sb < MAX_SUB_TB_NUM; sb++)
                    {
                        cu_data->nnz_sub[c][sb][idx + i] = core->nnz_sub[c][sb];
                    }
                }

                cu_data->qp_u[idx + i] = core->qp_u;
                cu_data->qp_v[idx + i] = core->qp_v;

                if (core->cu_mode == MODE_INTRA)
                {
                    cu_data->ipm[1][idx + i] = core->ipm[1];
                }
            }
            idx += core->cuw >> MIN_CU_LOG2;
        }
    }
}

void update_map_scu(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int src_cuw, int src_cuh)
{
    u32  *map_scu = 0, *src_map_scu = 0;
    s8   *map_ipm = 0, *src_map_ipm = 0;
    s16 (*map_mv)[REFP_NUM][MV_D] = 0, (*src_map_mv)[REFP_NUM][MV_D] = 0;
    s16 (*map_unrefined_mv)[REFP_NUM][MV_D] = 0, (*src_map_unrefined_mv)[REFP_NUM][MV_D] = 0;
    s8  (*map_refi)[REFP_NUM] = 0;
    s8  **src_map_refi = NULL;
    s8   *map_depth = 0, *src_depth = 0;
    int   size_depth;
    int   w, h, i, size, size_ipm, size_mv, size_refi;
    int   log2_src_cuw, log2_src_cuh;
    int   scu_x, scu_y;
    u32  *map_cu_mode = 0, *src_map_cu_mode = 0;

    scu_x = x >> MIN_CU_LOG2;
    scu_y = y >> MIN_CU_LOG2;
    log2_src_cuw = XEVE_LOG2(src_cuw);
    log2_src_cuh = XEVE_LOG2(src_cuh);

    map_scu = ctx->map_scu + scu_y * ctx->w_scu + scu_x;
    src_map_scu = core->cu_data_best[log2_src_cuw - 2][log2_src_cuh - 2].map_scu;

    map_ipm = ctx->map_ipm + scu_y * ctx->w_scu + scu_x;
    src_map_ipm = core->cu_data_best[log2_src_cuw - 2][log2_src_cuh - 2].ipm[0];

    map_mv = ctx->map_mv + scu_y * ctx->w_scu + scu_x;
    src_map_mv = core->cu_data_best[log2_src_cuw - 2][log2_src_cuh - 2].mv;

    map_refi = ctx->map_refi + scu_y * ctx->w_scu + scu_x;
    src_map_refi = core->cu_data_best[log2_src_cuw - 2][log2_src_cuh - 2].refi;

    map_depth = ctx->map_depth + scu_y * ctx->w_scu + scu_x;
    src_depth = core->cu_data_best[log2_src_cuw - 2][log2_src_cuh - 2].depth;

    map_unrefined_mv = ctx->map_unrefined_mv + scu_y * ctx->w_scu + scu_x;
    src_map_unrefined_mv = core->cu_data_best[log2_src_cuw - 2][log2_src_cuh - 2].unrefined_mv;

    map_cu_mode = ctx->map_cu_mode + scu_y * ctx->w_scu + scu_x;
    src_map_cu_mode = core->cu_data_best[log2_src_cuw - 2][log2_src_cuh - 2].map_cu_mode;

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

    size = sizeof(u32) * w;
    size_ipm = sizeof(u8) * w;
    size_mv = sizeof(s16) * w * REFP_NUM * MV_D;
    size_refi = sizeof(s8) * w * REFP_NUM;
    size_depth = sizeof(s8) * w;

    for(i = 0; i < h; i++)
    {
        xeve_mcpy(map_scu, src_map_scu, size);
        xeve_mcpy(map_ipm, src_map_ipm, size_ipm);
        xeve_mcpy(map_mv, src_map_mv, size_mv);
        xeve_mcpy(map_refi, *(src_map_refi), size_refi);
        xeve_mcpy(map_unrefined_mv, src_map_unrefined_mv, size_mv);
        xeve_mcpy(map_depth, src_depth, size_depth);

        map_depth += ctx->w_scu;
        src_depth += (src_cuw >> MIN_CU_LOG2);

        map_scu += ctx->w_scu;
        src_map_scu += (src_cuw >> MIN_CU_LOG2);

        map_ipm += ctx->w_scu;
        src_map_ipm += (src_cuw >> MIN_CU_LOG2);

        map_mv += ctx->w_scu;
        src_map_mv += (src_cuw >> MIN_CU_LOG2);

        map_unrefined_mv += ctx->w_scu;
        src_map_unrefined_mv += (src_cuw >> MIN_CU_LOG2);

        map_refi += ctx->w_scu;
        src_map_refi += (src_cuw >> MIN_CU_LOG2);

        xeve_mcpy(map_cu_mode, src_map_cu_mode, size);
        map_cu_mode += ctx->w_scu;
        src_map_cu_mode += (src_cuw >> MIN_CU_LOG2);
    }
}

void clear_map_scu(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int cuw, int cuh)
{
    int w, h, i, size;
    u32 *map_scu = ctx->map_scu + (y >> MIN_CU_LOG2) * ctx->w_scu + (x >> MIN_CU_LOG2);
    u32 *map_cu_mode = ctx->map_cu_mode + (y >> MIN_CU_LOG2) * ctx->w_scu + (x >> MIN_CU_LOG2);

    if(x + cuw > ctx->w)
    {
        cuw = ctx->w - x;
    }

    if(y + cuh > ctx->h)
    {
        cuh = ctx->h - y;
    }

    w = (cuw >> MIN_CU_LOG2);
    h = (cuh >> MIN_CU_LOG2);

    size = sizeof(u32) * w;

    for(i = 0; i < h; i++)
    {
        xeve_mset(map_scu, 0, size);
        map_scu += ctx->w_scu;

        xeve_mset(map_cu_mode, 0, size);
        map_cu_mode += ctx->w_scu;
    }
}


u16 xeve_get_lr(u16 avail)
{
    u16 avail_lr = avail;
#if ENC_SUCO_FAST_CONFIG == 1
    avail_lr = 0;
#elif ENC_SUCO_FAST_CONFIG == 2
    avail_lr = (avail == LR_10 || avail == LR_00) ? 0 : 1;
#else // ENC_SUCO_FAST_CONFIG == 4
    avail_lr = avail;
#endif
    return avail_lr;
}

double mode_check_inter(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh, int cud, XEVE_MODE *mi, double cost_best)
{
    s16(*coef)[MAX_CU_DIM] = core->ctmp;
    pel    *rec[N_C];
    double  cost = MAX_COST;
    int start_comp = xeve_check_luma(core->tree_cons) ? Y_C : U_C;
    int end_comp = xeve_check_chroma(core->tree_cons) ? N_C : U_C;
    int     i, s_rec[N_C];

    if (ctx->slice_type != SLICE_I && (ctx->sps.tool_admvp == 0 || !(log2_cuw <= MIN_CU_LOG2 && log2_cuh <= MIN_CU_LOG2)) && (!xeve_check_only_intra(core->tree_cons)))
    {
        core->avail_cu = xeve_get_avail_inter(core->x_scu, core->y_scu, ctx->w_scu, ctx->h_scu, core->scup, core->cuw, core->cuh, ctx->map_scu, ctx->map_tidx);
        cost = ctx->fn_pinter_analyze_cu(ctx, core, x, y, log2_cuw, log2_cuh, mi, coef, rec, s_rec);

        if (cost < cost_best)
        {
            cost_best = cost;
#if TRACE_ENC_CU_DATA
            mi->trace_cu_idx = core->trace_idx;
#endif
#if TRACE_ENC_HISTORIC
            xeve_mcpy(&mi->history_buf, &core->history_buffer, sizeof(core->history_buffer));
#endif
#if TRACE_ENC_CU_DATA_CHECK
            xeve_assert(core->trace_idx != 0);
#endif

            for (i = start_comp; i < end_comp; i++)
            {
                mi->rec[i] = rec[i];
                mi->s_rec[i] = s_rec[i];
            }
            if (ctx->pps.cu_qp_delta_enabled_flag)
            {
                xeve_set_qp(ctx, core, core->dqp_next_best[log2_cuw - 2][log2_cuh - 2].prev_qp);
            }
            ctx->fn_mode_copy_to_cu_data(ctx, core, mi, coef);
        }
    }

    return cost_best;
}

double mode_check_intra(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh, int cud, XEVE_MODE *mi, double cost_best)
{
    s16       (*coef)[MAX_CU_DIM] = core->ctmp;
    pel        *rec[N_C];
    double      cost = MAX_COST;
    int         start_comp = xeve_check_luma(core->tree_cons) ? Y_C : U_C;
    int         end_comp = xeve_check_chroma(core->tree_cons) ? N_C : U_C;
    int         i, s_rec[N_C];

    if( (ctx->slice_type == SLICE_I || core->nnz[Y_C] != 0 || core->nnz[U_C] != 0 || core->nnz[V_C] != 0 || cost_best == MAX_COST)
        && (!xeve_check_only_inter(core->tree_cons)))
    {
        core->cost_best = cost_best;
        core->dist_cu_best = XEVE_INT32_MAX;

        if(core->cu_mode != MODE_IBC && core->cost_best != MAX_COST)
        {
            XEVE_PINTRA *pi = &ctx->pintra[core->thread_cnt];
            core->inter_satd = xeve_satd_16b(log2_cuw, log2_cuh, pi->o[Y_C] + (y * pi->s_o[Y_C]) + x, mi->pred_y_best, pi->s_o[Y_C], 1 << log2_cuw, ctx->sps.bit_depth_luma_minus8+8);
        }
        else
        {
            core->inter_satd = XEVE_UINT32_MAX;
        }
        if (ctx->pps.cu_qp_delta_enabled_flag)
        {
            xeve_set_qp(ctx, core, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].curr_qp);
        }

        core->avail_cu = xeve_get_avail_intra(core->x_scu, core->y_scu, ctx->w_scu, ctx->h_scu, core->scup, log2_cuw, log2_cuh, ctx->map_scu, ctx->map_tidx);
        cost = ctx->fn_pintra_analyze_cu(ctx, core, x, y, log2_cuw, log2_cuh, mi, coef, rec, s_rec);


        if(cost < cost_best)
        {
            cost_best = cost;
#if TRACE_ENC_CU_DATA
            mi->trace_cu_idx = core->trace_idx;
#endif
#if TRACE_ENC_HISTORIC
            xeve_mcpy(&mi->history_buf, &core->history_buffer, sizeof(core->history_buffer));
#endif
#if TRACE_ENC_CU_DATA_CHECK
            xeve_assert(core->trace_idx != 0);
#endif
            core->cu_mode = MODE_INTRA;

            SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
            DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);
            core->dist_cu_best = core->dist_cu;

            for (i = start_comp; i < end_comp; i++)
            {
                mi->rec[i] = rec[i];
                mi->s_rec[i] = s_rec[i];
            }

            if (ctx->fn_mode_reset_intra != NULL)
            {
                ctx->fn_mode_reset_intra(core);
            }

            ctx->fn_mode_copy_to_cu_data(ctx, core, mi, coef);
        }
    }
    return cost_best;
}

static double mode_coding_unit(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh, int cud, XEVE_MODE *mi)
{
    s16(*coef)[MAX_CU_DIM] = core->ctmp;
    double  cost_best;

    xeve_assert(abs(log2_cuw - log2_cuh) <= 2);
    mode_cu_init(ctx, core, x, y, log2_cuw, log2_cuh, cud);

    core->avail_lr = xeve_check_nev_avail(core->x_scu, core->y_scu, (1 << log2_cuw), (1 << log2_cuh), ctx->w_scu, ctx->h_scu, ctx->map_scu, ctx->map_tidx);
    xeve_get_ctx_some_flags(core->x_scu, core->y_scu, 1 << log2_cuw, 1 << log2_cuh, ctx->w_scu, ctx->map_scu, ctx->map_cu_mode, core->ctx_flags, ctx->sh->slice_type, ctx->sps.tool_cm_init
                         , ctx->param.ibc_flag, ctx->sps.ibc_log_max_size, ctx->map_tidx);

    cost_best = MAX_COST;
    core->cost_best = MAX_COST;

    cost_best = mode_check_inter(ctx, core, x, y, log2_cuw, log2_cuh, cud, mi, cost_best);
    cost_best = mode_check_intra(ctx, core, x, y, log2_cuw, log2_cuh, cud, mi, cost_best);

    return cost_best;
}

static u16 xeve_get_avail_block(int x_scu, int y_scu, int w_scu, int h_scu, int scup, int log2_cuw, int log2_cuh, u32 *map_scu, u8* map_tidx)
{
    u16 avail = 0;
    int log2_scuw, log2_scuh, scuw, scuh;

    log2_scuw = log2_cuw - MIN_CU_LOG2;
    log2_scuh = log2_cuh - MIN_CU_LOG2;
    scuw = 1 << log2_scuw;
    scuh = 1 << log2_scuh;

    if(x_scu > 0 && MCU_GET_COD(map_scu[scup - 1]) && (map_tidx[scup] == map_tidx[scup - 1]))
    {
        SET_AVAIL(avail, AVAIL_LE);
        if(y_scu + scuh < h_scu && MCU_GET_COD(map_scu[scup + (scuh * w_scu) - 1]) && (map_tidx[scup] == map_tidx[scup + (scuh * w_scu) - 1]))
        {
            SET_AVAIL(avail, AVAIL_LO_LE);
        }
    }

    if(y_scu > 0)
    {
        if (map_tidx[scup] == map_tidx[scup - w_scu])
        {
            SET_AVAIL(avail, AVAIL_UP);
        }
        if (map_tidx[scup] == map_tidx[scup - w_scu + scuw - 1])
        {
            SET_AVAIL(avail, AVAIL_RI_UP);
        }

        if(x_scu > 0 && MCU_GET_COD(map_scu[scup - w_scu - 1]) && (map_tidx[scup] == map_tidx[scup - w_scu - 1]) && (map_tidx[scup] == map_tidx[scup-1]))
        {
            SET_AVAIL(avail, AVAIL_UP_LE);
        }
        if(x_scu + scuw < w_scu && MCU_GET_COD(map_scu[scup - w_scu + scuw]) && (map_tidx[scup] == map_tidx[scup - w_scu + scuw]))
        {
            SET_AVAIL(avail, AVAIL_UP_RI);
        }
    }

    if(x_scu + scuw < w_scu && MCU_GET_COD(map_scu[scup + scuw]) && (map_tidx[scup] == map_tidx[scup + scuw]))
    {
        SET_AVAIL(avail, AVAIL_RI);

        if(y_scu + scuh < h_scu && MCU_GET_COD(map_scu[scup + (w_scu * scuh) + scuw]) && (map_tidx[scup] == map_tidx[scup + (w_scu * scuh) + scuw]))
        {
            SET_AVAIL(avail, AVAIL_LO_RI);
        }
    }

    return avail;
}

int check_nev_block(XEVE_CTX *ctx, int x0, int y0, int log2_cuw, int log2_cuh, int *do_curr, int *do_split, int cud, int *nbr_map_skip_flag, XEVE_CORE * core)
{
    int avail_cu;
    int pos;
    int log2_scuw, log2_scuh, scuw, scuh;
    int tmp;
    int min_depth, max_depth;
    int cup;
    int x_scu, y_scu;
    int w, h;
    int nbr_map_skipcnt = 0;
    int nbr_map_cnt = 0;

    xeve_assert(xeve_check_luma(core->tree_cons));

    x_scu = (x0 >> MIN_CU_LOG2);
    y_scu = (y0 >> MIN_CU_LOG2);
    cup   = y_scu * ctx->w_scu + x_scu;
    log2_scuw = log2_cuw - MIN_CU_LOG2;
    log2_scuh = log2_cuh - MIN_CU_LOG2;
    scuw = 1 << log2_scuw;
    scuh = 1 << log2_scuh;

    *do_curr = 1;
    *do_split = 1;
    avail_cu = xeve_get_avail_block(x_scu, y_scu, ctx->w_scu, ctx->h_scu, cup, log2_cuw, log2_cuh, ctx->map_scu, ctx->map_tidx);

    min_depth = MAX_CU_DEPTH;
    max_depth = 0;

    if(IS_AVAIL(avail_cu, AVAIL_UP))
    {
        for(w = 0; w < scuw; w++)
        {
            pos = cup - ctx->w_scu + w;
            tmp = ctx->map_depth[pos];
            min_depth = tmp < min_depth ? tmp : min_depth;
            max_depth = tmp > max_depth ? tmp : max_depth;
            nbr_map_skipcnt += (1 == (MCU_GET_SF(ctx->map_scu[pos]) || MCU_GET_MMVDS(ctx->map_cu_mode[pos])));
            nbr_map_cnt++;
        }
    }

    if(IS_AVAIL(avail_cu, AVAIL_UP_RI))
    {
        pos = cup - ctx->w_scu + scuw;

        tmp = ctx->map_depth[pos];
        min_depth = tmp < min_depth ? tmp : min_depth;
        max_depth = tmp > max_depth ? tmp : max_depth;
    }

    if(IS_AVAIL(avail_cu, AVAIL_LE))
    {
        for(h = 0; h < scuh; h++)
        {
            pos = cup - 1 + (h * ctx->w_scu);

            tmp = ctx->map_depth[pos];
            min_depth = tmp < min_depth ? tmp : min_depth;
            max_depth = tmp > max_depth ? tmp : max_depth;
            nbr_map_skipcnt += (1 == (MCU_GET_SF(ctx->map_scu[pos])|| MCU_GET_MMVDS(ctx->map_cu_mode[pos])));
            nbr_map_cnt++;
        }
    }

    if(IS_AVAIL(avail_cu, AVAIL_LO_LE))
    {
        pos = cup + (ctx->w_scu * scuh) - 1;
        tmp = ctx->map_depth[pos];
        min_depth = tmp < min_depth ? tmp : min_depth;
        max_depth = tmp > max_depth ? tmp : max_depth;
    }

    if(IS_AVAIL(avail_cu, AVAIL_UP_LE))
    {
        pos = cup - ctx->w_scu - 1;
        tmp = ctx->map_depth[pos];
        min_depth = tmp < min_depth ? tmp : min_depth;
        max_depth = tmp > max_depth ? tmp : max_depth;
    }

    if(IS_AVAIL(avail_cu, AVAIL_RI))
    {
        for(h = 0; h < scuh; h++)
        {
            pos = cup + scuw + (h * ctx->w_scu);
            tmp = ctx->map_depth[pos];
            min_depth = tmp < min_depth ? tmp : min_depth;
            max_depth = tmp > max_depth ? tmp : max_depth;
            nbr_map_skipcnt += (1 == (MCU_GET_SF(ctx->map_scu[pos]) || MCU_GET_MMVDS(ctx->map_cu_mode[pos])));
            nbr_map_cnt++;
        }
    }

    if(IS_AVAIL(avail_cu, AVAIL_LO_RI))
    {
        pos = cup + (ctx->w_scu * scuh) + scuw;
        tmp = ctx->map_depth[pos];
        min_depth = tmp < min_depth ? tmp : min_depth;
        max_depth = tmp > max_depth ? tmp : max_depth;
    }

    if(avail_cu && (max_depth - min_depth < 3))
    {
        if(cud < min_depth - 1)
        {
            if(log2_cuw > MIN_CU_LOG2 && log2_cuh > MIN_CU_LOG2)
                *do_curr = 0;
            else
                *do_curr = 1;
        }

        if(cud > max_depth + 1)
        {
            *do_split = (*do_curr) ? 0 : 1;
        }
    }
    else
    {
        max_depth = MAX_CU_DEPTH;
        min_depth = 0;
    }

    *nbr_map_skip_flag = 0;
    if((ctx->slice_type != SLICE_I) && (nbr_map_skipcnt > (nbr_map_cnt / 2)))
    {
        *nbr_map_skip_flag = 1;
    }

    return (max_depth);
}

void calc_delta_dist_filter_boundary(XEVE_CTX* ctx, XEVE_PIC *pic_rec, XEVE_PIC *pic_org, int cuw, int cuh,
                                     pel(*src)[MAX_CU_DIM], int s_src, int x, int y, u16 avail_lr, u8 intra_flag,
                                     u8 cbf_l, s8 *refi, s16(*mv)[MV_D], u8 is_mv_from_mvf, XEVE_CORE * core)
{
    int i, j;
    int log2_cuw = XEVE_LOG2(cuw);
    int log2_cuh = XEVE_LOG2(cuh);
    int x_offset = 4; //for preparing deblocking filter taps
    int y_offset = 4;
    int x_tm = ctx->sps.tool_addb ? 4 : 2; //for calculating template dist
    int y_tm = ctx->sps.tool_addb ? 4 : 2; //must be the same as x_tm
    int log2_x_tm = XEVE_LOG2(x_tm);
    int log2_y_tm = XEVE_LOG2(y_tm);
    XEVE_PIC * pic_dbk = ctx->pic_dbk;
    int s_l_dbk = pic_dbk->s_l;
    int s_c_dbk = pic_dbk->s_c;
    int s_l_org = pic_org->s_l;
    int s_c_org = pic_org->s_c;
    int w_shift = ctx->param.cs_w_shift;
    int h_shift = ctx->param.cs_h_shift;
    pel * dst_y = pic_dbk->y + y * s_l_dbk + x;
    pel * dst_u = pic_dbk->u + (y >> h_shift) * s_c_dbk + (x >> w_shift);
    pel * dst_v = pic_dbk->v + (y >> h_shift) * s_c_dbk + (x >> w_shift);
    pel * org_y = pic_org->y + y * s_l_org + x;
    pel * org_u = pic_org->u + (y >> h_shift) * s_c_org + (x >> w_shift);
    pel * org_v = pic_org->v + (y >> h_shift) * s_c_org + (x >> w_shift);
    int x_scu = x >> MIN_CU_LOG2;
    int y_scu = y >> MIN_CU_LOG2;
    int t = x_scu + y_scu * ctx->w_scu;
    //cu info to save
    u8 intra_flag_save, cbf_l_save;
    u8 do_filter = 0;
    int y_begin = ((ctx->tile[core->tile_num].ctba_rs_first) / ctx->w_lcu) << ctx->log2_max_cuwh;
    int y_begin_uv = (((ctx->tile[core->tile_num].ctba_rs_first) / ctx->w_lcu) << ctx->log2_max_cuwh)>> h_shift;

    if(ctx->sh->deblocking_filter_on)
    {
        do_filter = 1;
    }

    if(do_filter == 0)
    {
        core->delta_dist[Y_C] = core->delta_dist[U_C] = core->delta_dist[V_C] = 0;
        return; //if no filter is applied, just return delta_dist as 0
    }

    //reset
    for (i = 0; i < N_C; i++)
    {
        core->dist_filter[i] = core->dist_nofilt[i] = 0;
    }

    /********************** prepare pred/rec pixels (not filtered) ****************************/

    //fill src to dst
    for(i = 0; i < cuh; i++)
        xeve_mcpy(dst_y + i*s_l_dbk, src[Y_C] + i*s_src, cuw * sizeof(pel));

    //fill top
    if (y != y_begin)
    {
        for(i = 0; i < y_offset; i++)
            xeve_mcpy(dst_y + (-y_offset + i)*s_l_dbk, pic_rec->y + (y - y_offset + i)*s_l_dbk + x, cuw * sizeof(pel));
    }

    //fill left
    if(avail_lr == LR_10 || avail_lr == LR_11)
    {
        for(i = 0; i < cuh; i++)
            xeve_mcpy(dst_y + i*s_l_dbk - x_offset, pic_rec->y + (y + i)*s_l_dbk + (x - x_offset), x_offset * sizeof(pel));
    }

    //fill right
    if(avail_lr == LR_01 || avail_lr == LR_11)
    {
        for(i = 0; i < cuh; i++)
            xeve_mcpy(dst_y + i*s_l_dbk + cuw, pic_rec->y + (y + i)*s_l_dbk + (x + cuw), x_offset * sizeof(pel));
    }

    //modify parameters from y to uv
    cuw >>= w_shift;  cuh >>= h_shift;  x_offset >>= w_shift;  y_offset >>= h_shift;  s_src >>= w_shift;  x >>= w_shift;  y >>= h_shift;
    x_tm >>= w_shift;  y_tm >>= h_shift;  log2_cuw -= w_shift;  log2_cuh -= h_shift;  log2_x_tm -= w_shift;  log2_y_tm -= h_shift;

    if(ctx->sps.chroma_format_idc)
    {
        //fill src to dst
        for(i = 0; i < cuh; i++)
        {
            xeve_mcpy(dst_u + i * s_c_dbk, src[U_C] + i * s_src, cuw * sizeof(pel));
            xeve_mcpy(dst_v + i * s_c_dbk, src[V_C] + i * s_src, cuw * sizeof(pel));
        }

        //fill top
        if (y != y_begin_uv)
        {
            for(i = 0; i < y_offset; i++)
            {
                xeve_mcpy(dst_u + (-y_offset + i)*s_c_dbk, pic_rec->u + (y - y_offset + i)*s_c_dbk + x, cuw * sizeof(pel));
                xeve_mcpy(dst_v + (-y_offset + i)*s_c_dbk, pic_rec->v + (y - y_offset + i)*s_c_dbk + x, cuw * sizeof(pel));
            }
        }

        //fill left
        if (avail_lr == LR_10 || avail_lr == LR_11)
        {
            for(i = 0; i < cuh; i++)
            {
                xeve_mcpy(dst_u + i * s_c_dbk - x_offset, pic_rec->u + (y + i)*s_c_dbk + (x - x_offset), x_offset * sizeof(pel));
                xeve_mcpy(dst_v + i * s_c_dbk - x_offset, pic_rec->v + (y + i)*s_c_dbk + (x - x_offset), x_offset * sizeof(pel));
            }
        }

        //fill right
        if(avail_lr == LR_01 || avail_lr == LR_11)
        {
            for(i = 0; i < cuh; i++)
            {
                xeve_mcpy(dst_u + i * s_c_dbk + cuw, pic_rec->u + (y + i)*s_c_dbk + (x + cuw), x_offset * sizeof(pel));
                xeve_mcpy(dst_v + i * s_c_dbk + cuw, pic_rec->v + (y + i)*s_c_dbk + (x + cuw), x_offset * sizeof(pel));
            }
        }
    }

    //recover
    cuw <<= w_shift;  cuh <<= h_shift;  x_offset <<= w_shift;  y_offset <<= h_shift;  s_src <<= w_shift;  x <<= w_shift;  y <<= h_shift;
    x_tm <<= w_shift;  y_tm <<= h_shift;  log2_cuw += w_shift;  log2_cuh += h_shift;  log2_x_tm += w_shift;  log2_y_tm += h_shift;

    //add distortion of current
    core->dist_nofilt[Y_C] += xeve_ssd_16b(log2_cuw, log2_cuh, dst_y, org_y, s_l_dbk, s_l_org, ctx->sps.bit_depth_luma_minus8 + 8);

    //add distortion of top
    if (y != y_begin)
    {
        core->dist_nofilt[Y_C] += xeve_ssd_16b(log2_cuw, log2_y_tm, dst_y - y_tm * s_l_dbk, org_y - y_tm * s_l_org, s_l_dbk, s_l_org, ctx->sps.bit_depth_luma_minus8 + 8);
    }
    if(avail_lr == LR_10 || avail_lr == LR_11)
    {
        core->dist_nofilt[Y_C] += xeve_ssd_16b(log2_x_tm, log2_cuh, dst_y - x_tm, org_y - x_tm, s_l_dbk, s_l_org, ctx->sps.bit_depth_luma_minus8 + 8);
    }
    if(avail_lr == LR_01 || avail_lr == LR_11)
    {
        core->dist_nofilt[Y_C] += xeve_ssd_16b(log2_x_tm, log2_cuh, dst_y + cuw, org_y + cuw, s_l_dbk, s_l_org, ctx->sps.bit_depth_luma_minus8 + 8);
    }

    cuw >>= w_shift;  cuh >>= h_shift;  x_offset >>= w_shift;  y_offset >>= h_shift;  s_src >>= w_shift;  x >>= w_shift;  y >>= h_shift;
    x_tm >>= w_shift;  y_tm >>= h_shift;  log2_cuw -= w_shift;  log2_cuh -= h_shift;  log2_x_tm -= w_shift;  log2_y_tm -= h_shift;
    if(ctx->sps.chroma_format_idc)
    {
        core->dist_nofilt[U_C] += xeve_ssd_16b(log2_cuw, log2_cuh, dst_u, org_u, s_c_dbk, s_c_org, ctx->sps.bit_depth_chroma_minus8 + 8);
        core->dist_nofilt[V_C] += xeve_ssd_16b(log2_cuw, log2_cuh, dst_v, org_v, s_c_dbk, s_c_org, ctx->sps.bit_depth_chroma_minus8 + 8);

        if (y != y_begin_uv)
        {
            core->dist_nofilt[U_C] += xeve_ssd_16b(log2_cuw, log2_y_tm, dst_u - y_tm*s_c_dbk, org_u - y_tm*s_c_org, s_c_dbk, s_c_org, ctx->sps.bit_depth_chroma_minus8 + 8);
            core->dist_nofilt[V_C] += xeve_ssd_16b(log2_cuw, log2_y_tm, dst_v - y_tm*s_c_dbk, org_v - y_tm*s_c_org, s_c_dbk, s_c_org, ctx->sps.bit_depth_chroma_minus8 + 8);
        }
        if(avail_lr == LR_10 || avail_lr == LR_11)
        {
            core->dist_nofilt[U_C] += xeve_ssd_16b(log2_x_tm, log2_cuh, dst_u - x_tm, org_u - x_tm, s_c_dbk, s_c_org, ctx->sps.bit_depth_chroma_minus8 + 8);
            core->dist_nofilt[V_C] += xeve_ssd_16b(log2_x_tm, log2_cuh, dst_v - x_tm, org_v - x_tm, s_c_dbk, s_c_org, ctx->sps.bit_depth_chroma_minus8 + 8);
        }
        if(avail_lr == LR_01 || avail_lr == LR_11)
        {
            core->dist_nofilt[U_C] += xeve_ssd_16b(log2_x_tm, log2_cuh, dst_u + cuw, org_u + cuw, s_c_dbk, s_c_org, ctx->sps.bit_depth_chroma_minus8 + 8);
            core->dist_nofilt[V_C] += xeve_ssd_16b(log2_x_tm, log2_cuh, dst_v + cuw, org_v + cuw, s_c_dbk, s_c_org, ctx->sps.bit_depth_chroma_minus8 + 8);
        }
    }

    //recover
    cuw <<= w_shift;  cuh <<= h_shift;  x_offset <<= w_shift;  y_offset <<= h_shift;  s_src <<= w_shift;  x <<= w_shift;  y <<= h_shift;
    x_tm <<= w_shift;  y_tm <<= h_shift;  log2_cuw += w_shift;  log2_cuh += h_shift;  log2_x_tm += w_shift;  log2_y_tm += h_shift;

    /********************************* filter the pred/rec **************************************/
    if(do_filter)
    {
        pic_dbk->pic_deblock_alpha_offset = ctx->param.deblock_alpha_offset;
        pic_dbk->pic_deblock_beta_offset = ctx->param.deblock_beta_offset;
        int w_scu = cuw >> MIN_CU_LOG2;
        int h_scu = cuh >> MIN_CU_LOG2;
        int ind, k;
        //save current best cu info
        intra_flag_save       = MCU_GET_IF(ctx->map_scu[t]);
        cbf_l_save            = MCU_GET_CBFL(ctx->map_scu[t]);
        //set map info of current cu to current mode
        for(j = 0; j < h_scu; j++)
        {
            ind = (y_scu + j) * ctx->w_scu + x_scu;
            for(i = 0; i < w_scu; i++)
            {
                k = ind + i;

                if (xeve_check_luma(core->tree_cons))
                {
                if(intra_flag)
                    MCU_SET_IF(ctx->map_scu[k]);
                else
                    MCU_CLR_IF(ctx->map_scu[k]);
                if(cbf_l)
                    MCU_SET_CBFL(ctx->map_scu[k]);
                else
                    MCU_CLR_CBFL(ctx->map_scu[k]);
                }

                if(refi != NULL && !is_mv_from_mvf)
                {
                    ctx->map_refi[k][REFP_0] = refi[REFP_0];
                    ctx->map_refi[k][REFP_1] = refi[REFP_1];
                    ctx->map_mv[k][REFP_0][MV_X] = mv[REFP_0][MV_X];
                    ctx->map_mv[k][REFP_0][MV_Y] = mv[REFP_0][MV_Y];
                    ctx->map_mv[k][REFP_1][MV_X] = mv[REFP_1][MV_X];
                    ctx->map_mv[k][REFP_1][MV_Y] = mv[REFP_1][MV_Y];

                    ctx->map_unrefined_mv[k][REFP_0][MV_X] = mv[REFP_0][MV_X];
                    ctx->map_unrefined_mv[k][REFP_0][MV_Y] = mv[REFP_0][MV_Y];
                    ctx->map_unrefined_mv[k][REFP_1][MV_X] = mv[REFP_1][MV_X];
                    ctx->map_unrefined_mv[k][REFP_1][MV_Y] = mv[REFP_1][MV_Y];
                }

                if(ctx->pps.cu_qp_delta_enabled_flag)
                {
                    MCU_CLR_QP(ctx->map_scu[k]);
                    MCU_SET_QP(ctx->map_scu[k], ctx->core[core->thread_cnt]->qp);
                }
                else
                {
                    MCU_SET_QP(ctx->map_scu[k], ctx->tile[core->tile_idx].qp);
                }

                //clear coded (necessary)
                MCU_CLR_COD(ctx->map_scu[k]);
            }
        }

        if (ctx->fn_mode_rdo_dbk_map_set != NULL)
        {
            ctx->fn_mode_rdo_dbk_map_set(ctx, core, log2_cuw, log2_cuh, cbf_l, t);
        }

        //first, horizontal filtering
        // As of now filtering across tile boundaries is disabled
        ctx->fn_deblock_unit(ctx, pic_dbk, x, y, cuw, cuh, 1, core, 0);

        //clean coded flag in between two directional filtering (not necessary here)
        for(j = 0; j < h_scu; j++)
        {
            ind = (y_scu + j) * ctx->w_scu + x_scu;
            for(i = 0; i < w_scu; i++)
            {
                k = ind + i;
                MCU_CLR_COD(ctx->map_scu[k]);
            }
        }

        //then, vertical filtering
        ctx->fn_deblock_unit(ctx, pic_dbk, x, y, cuw, cuh, 0, core, 0);

        //recover best cu info
        for(j = 0; j < h_scu; j++)
        {
            ind = (y_scu + j) * ctx->w_scu + x_scu;
            for(i = 0; i < w_scu; i++)
            {
                k = ind + i;

                if (xeve_check_luma(core->tree_cons))
                {
                    if (intra_flag_save)
                    {
                        MCU_SET_IF(ctx->map_scu[k]);
                    }
                    else
                    {
                        MCU_CLR_IF(ctx->map_scu[k]);
                    }

                    if (cbf_l_save)
                    {
                        MCU_SET_CBFL(ctx->map_scu[k]);
                    }
                    else
                    {
                        MCU_CLR_CBFL(ctx->map_scu[k]);
                    }
                }

                MCU_CLR_COD(ctx->map_scu[k]);
            }
        }
    }
    /*********************** calc dist of filtered pixels *******************************/
    //add current
    core->dist_filter[Y_C] += xeve_ssd_16b(log2_cuw, log2_cuh, dst_y, org_y, s_l_dbk, s_l_org, ctx->sps.bit_depth_luma_minus8 + 8);

    //add  top
    if (y != y_begin)
    {
        core->dist_filter[Y_C] += xeve_ssd_16b(log2_cuw, log2_y_tm, dst_y - y_tm * s_l_dbk, org_y - y_tm * s_l_org, s_l_dbk, s_l_org, ctx->sps.bit_depth_luma_minus8 + 8);
    }

    //add left
    if(avail_lr == LR_10 || avail_lr == LR_11)
    {
        core->dist_filter[Y_C] += xeve_ssd_16b(log2_x_tm, log2_cuh, dst_y - x_tm, org_y - x_tm, s_l_dbk, s_l_org, ctx->sps.bit_depth_luma_minus8 + 8);
    }

    //add right
    if(avail_lr == LR_01 || avail_lr == LR_11)
    {
        core->dist_filter[Y_C] += xeve_ssd_16b(log2_x_tm, log2_cuh, dst_y + cuw, org_y + cuw, s_l_dbk, s_l_org, ctx->sps.bit_depth_luma_minus8+8);
    }

    //modify parameters from y to uv
    cuw >>= w_shift;  cuh >>= h_shift;  x_offset >>= w_shift;  y_offset >>= h_shift;  s_src >>= w_shift;  x >>= w_shift;  y >>= h_shift;
    x_tm >>= w_shift;  y_tm >>= h_shift;  log2_cuw -= w_shift;  log2_cuh -= h_shift;  log2_x_tm -= w_shift;  log2_y_tm -= h_shift;

    if(ctx->sps.chroma_format_idc)
    {
        //add current
        core->dist_filter[U_C] += xeve_ssd_16b(log2_cuw, log2_cuh, dst_u, org_u, s_c_dbk, s_c_org, ctx->sps.bit_depth_chroma_minus8+8);
        core->dist_filter[V_C] += xeve_ssd_16b(log2_cuw, log2_cuh, dst_v, org_v, s_c_dbk, s_c_org, ctx->sps.bit_depth_chroma_minus8+8);

        //add top
        if (y != y_begin_uv)
        {
            core->dist_filter[U_C] += xeve_ssd_16b(log2_cuw, log2_y_tm, dst_u - y_tm * s_c_dbk, org_u - y_tm * s_c_org, s_c_dbk, s_c_org, ctx->sps.bit_depth_chroma_minus8+8);
            core->dist_filter[V_C] += xeve_ssd_16b(log2_cuw, log2_y_tm, dst_v - y_tm * s_c_dbk, org_v - y_tm * s_c_org, s_c_dbk, s_c_org, ctx->sps.bit_depth_chroma_minus8+8);
        }

        //add left
        if(avail_lr == LR_10 || avail_lr == LR_11)
        {
            core->dist_filter[U_C] += xeve_ssd_16b(log2_x_tm, log2_cuh, dst_u - x_tm, org_u - x_tm, s_c_dbk, s_c_org, ctx->sps.bit_depth_chroma_minus8+8);
            core->dist_filter[V_C] += xeve_ssd_16b(log2_x_tm, log2_cuh, dst_v - x_tm, org_v - x_tm, s_c_dbk, s_c_org, ctx->sps.bit_depth_chroma_minus8+8);
        }

        //add right
        if(avail_lr == LR_01 || avail_lr == LR_11)
        {
            core->dist_filter[U_C] += xeve_ssd_16b(log2_x_tm, log2_cuh, dst_u + cuw, org_u + cuw, s_c_dbk, s_c_org, ctx->sps.bit_depth_chroma_minus8+8);
            core->dist_filter[V_C] += xeve_ssd_16b(log2_x_tm, log2_cuh, dst_v + cuw, org_v + cuw, s_c_dbk, s_c_org, ctx->sps.bit_depth_chroma_minus8+8);
        }
    }
    //recover
    cuw <<= w_shift;  cuh <<= h_shift;  x_offset <<= w_shift;  y_offset <<= h_shift;  s_src <<= w_shift;  x <<= w_shift;  y <<= h_shift;
    x_tm <<= w_shift;  y_tm <<= h_shift;  log2_cuw += w_shift;  log2_cuh += h_shift;  log2_x_tm += w_shift;  log2_y_tm += h_shift;

    /******************************* derive delta dist ********************************/
    core->delta_dist[Y_C] = core->dist_filter[Y_C] - core->dist_nofilt[Y_C];
    core->delta_dist[U_C] = core->dist_filter[U_C] - core->dist_nofilt[U_C];
    core->delta_dist[V_C] = core->dist_filter[V_C] - core->dist_nofilt[V_C];
}

static double mode_coding_tree(XEVE_CTX *ctx, XEVE_CORE *core, int x0, int y0, int cup, int log2_cuw, int log2_cuh, int cud, XEVE_MODE *mi, int next_split, u8 qp, TREE_CONS tree_cons)
{
    // x0 = CU's left up corner horizontal index in entrie frame
    // y0 = CU's left up corner vertical index in entire frame
    // cuw = CU width, log2_cuw = CU width in log2
    // cuh = CU height, log2_chu = CU height in log2
    // ctx->w = frame width, ctx->h = frame height
    int cuw = 1 << log2_cuw;
    int cuh = 1 << log2_cuh;
    s8 best_split_mode = NO_SPLIT;
    int bit_cnt;
    double cost_best = MAX_COST;
    double cost_temp = MAX_COST;
    XEVE_SBAC s_temp_depth = {0};
    int boundary = !(x0 + cuw <= ctx->w && y0 + cuh <= ctx->h);
    int split_allow[SPLIT_QUAD + 1]; //allowed split by normative and non-normative selection
    u16 avail_lr = xeve_check_nev_avail(PEL2SCU(x0), PEL2SCU(y0), cuw, cuh, ctx->w_scu, ctx->h_scu, ctx->map_scu, ctx->map_tidx);
    SPLIT_MODE split_mode = NO_SPLIT;
    double best_split_cost = MAX_COST;
    double best_curr_cost = MAX_COST;
    XEVE_DQP dqp_temp_depth = { 0 };
    u8 best_dqp = qp;
    s8 min_qp, max_qp;
    double cost_temp_dqp;
    int cu_mode_dqp = 0;
    int dist_cu_best_dqp = 0;
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

    core->tree_cons = tree_cons;
    core->avail_lr = avail_lr;

    SBAC_LOAD(core->s_curr_before_split[log2_cuw - 2][log2_cuh - 2], core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);

    //decide allowed split modes for the current node
    //based on CU size located at boundary
    if (cuw > ctx->min_cuwh || cuh > ctx->min_cuwh)
    {
        split_allow[SPLIT_QUAD] = 1;
        split_allow[NO_SPLIT] = 1;
    }
    else
    {
        split_allow[NO_SPLIT] = 1;
    }

    if(!boundary)
    {
        cost_temp = 0.0;
        init_cu_data(&core->cu_data_temp[log2_cuw - 2][log2_cuh - 2], log2_cuw, log2_cuh, ctx->qp, ctx->qp, ctx->qp);

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

                if (ctx->param.aq_mode != 0 || ctx->param.cutree != 0)
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

                clear_map_scu(ctx, core, x0, y0, cuw, cuh);
                if (ctx->sps.tool_admvp && log2_cuw == 2 && log2_cuh == 2)
                {
                    core->tree_cons.mode_cons = eOnlyIntra;
                }
                cost_temp_dqp += mode_coding_unit(ctx, core, x0, y0, log2_cuw, log2_cuh, cud, mi);

                if (cost_best > cost_temp_dqp)
                {
                    cu_mode_dqp = core->cu_mode;
                    dist_cu_best_dqp = core->dist_cu_best;
                    /* backup the current best data */
                    copy_cu_data(&core->cu_data_best[log2_cuw - 2][log2_cuh - 2], &core->cu_data_temp[log2_cuw - 2][log2_cuh - 2], 0, 0, log2_cuw, log2_cuh, log2_cuw, cud, core->tree_cons, ctx->sps.chroma_format_idc);
                    cost_best = cost_temp_dqp;
                    best_split_mode = NO_SPLIT;
                    SBAC_STORE(s_temp_depth, core->s_next_best[log2_cuw - 2][log2_cuh - 2]);
                    DQP_STORE(dqp_temp_depth, core->dqp_next_best[log2_cuw - 2][log2_cuh - 2]);
                    mode_cpy_rec_to_ref(core, x0, y0, cuw, cuh, PIC_MODE(ctx), core->tree_cons, ctx->sps.chroma_format_idc);
                }
            }
            if (is_dqp_set && core->cu_qp_delta_code_mode == 2)
            {
                core->cu_qp_delta_code_mode = 0;
            }
            cost_temp = cost_best;
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
    }

#if ENC_ECU_ADAPTIVE
    if(cost_best != MAX_COST && cud >= (ctx->poc.poc_val % 2 ? (ENC_ECU_DEPTH_B - 2) : ENC_ECU_DEPTH_B)
#else
    if(cost_best != MAX_COST && cud >= ENC_ECU_DEPTH
#endif
       && core->cu_mode == MODE_SKIP)
    {
        next_split = 0;
    }

    if(cost_best != MAX_COST && ctx->sh->slice_type == SLICE_I)
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
        int split_mode_num = 0;
        core->tree_cons = tree_cons;
        split_mode = SPLIT_QUAD;
        if(split_allow[split_mode])
        {
            XEVE_SPLIT_STRUCT split_struct;
            xeve_split_get_part_structure( split_mode, x0, y0, cuw, cuh, cup, cud, ctx->log2_culine, &split_struct );
            split_struct.tree_cons = tree_cons;

            int prev_log2_sub_cuw = split_struct.log_cuw[0];
            int prev_log2_sub_cuh = split_struct.log_cuh[0];
            int is_dqp_set = 0;

            init_cu_data(&core->cu_data_temp[log2_cuw - 2][log2_cuh - 2], log2_cuw, log2_cuh, ctx->qp, ctx->qp, ctx->qp);
            clear_map_scu(ctx, core, x0, y0, cuw, cuh);
            cost_temp = 0.0;

            /* When BTT is disabled, split_cu_flag should always be considered although CU is on the picture boundary */
            {
                /* consider CU split flag */
                SBAC_LOAD(core->s_temp_run, core->s_curr_before_split[log2_cuw - 2][log2_cuh - 2]);
                xeve_sbac_bit_reset(&core->s_temp_run);
                xeve_set_split_mode(split_mode, cud, 0, cuw, cuh, cuw, core->cu_data_temp[log2_cuw - 2][log2_cuh - 2].split_mode);
                ctx->fn_eco_split_mode(&core->bs_temp, ctx, core, cud, 0, cuw, cuh, cuw, x0, y0);

                bit_cnt = xeve_get_bit_number(&core->s_temp_run);
                cost_temp += RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt);
                SBAC_STORE(core->s_curr_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_run);
            }

            init_cu_data(&core->cu_data_temp[log2_cuw - 2][log2_cuh - 2], log2_cuw, log2_cuh, ctx->qp, ctx->qp, ctx->qp);
            clear_map_scu(ctx, core, x0, y0, cuw, cuh);

#if TRACE_ENC_CU_DATA_CHECK
            static int counter_in[MAX_CU_LOG2 - MIN_CU_LOG2][MAX_CU_LOG2 - MIN_CU_LOG2] = { 0, };
            counter_in[log2_cuw - MIN_CU_LOG2][log2_cuh - MIN_CU_LOG2]++;
#endif

            for (int part_num = 0; part_num < split_struct.part_count; ++part_num)
            {
                int cur_part_num = part_num;
                int log2_sub_cuw = split_struct.log_cuw[cur_part_num];
                int log2_sub_cuh = split_struct.log_cuh[cur_part_num];
                int x_pos = split_struct.x_pos[cur_part_num];
                int y_pos = split_struct.y_pos[cur_part_num];
                int cur_cuw = split_struct.width[cur_part_num];
                int cur_cuh = split_struct.height[cur_part_num];

                if ((x_pos < ctx->w) && (y_pos < ctx->h))
                {
                    if(part_num == 0)
                    {
                        SBAC_LOAD(core->s_curr_best[log2_sub_cuw - 2][log2_sub_cuh - 2], core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
                        DQP_STORE(core->dqp_curr_best[log2_sub_cuw - 2][log2_sub_cuh - 2], core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);
                    }
                    else
                    {
                        SBAC_LOAD(core->s_curr_best[log2_sub_cuw - 2][log2_sub_cuh - 2], core->s_next_best[prev_log2_sub_cuw - 2][prev_log2_sub_cuh - 2]);
                        DQP_STORE(core->dqp_curr_best[log2_sub_cuw - 2][log2_sub_cuh - 2], core->dqp_next_best[prev_log2_sub_cuw - 2][prev_log2_sub_cuh - 2]);
                    }
                    cost_temp += mode_coding_tree(ctx, core, x_pos, y_pos, split_struct.cup[cur_part_num], log2_sub_cuw, log2_sub_cuh, split_struct.cud[cur_part_num], mi, 1
                                                  , core->qp, split_struct.tree_cons);

                    copy_cu_data(&core->cu_data_temp[log2_cuw - 2][log2_cuh - 2], &core->cu_data_best[log2_sub_cuw - 2][log2_sub_cuh - 2], x_pos - split_struct.x_pos[0]
                               , y_pos - split_struct.y_pos[0], log2_sub_cuw, log2_sub_cuh, log2_cuw, cud, split_struct.tree_cons, ctx->sps.chroma_format_idc);

                    update_map_scu(ctx, core, x_pos, y_pos, cur_cuw, cur_cuh);
                    prev_log2_sub_cuw = log2_sub_cuw;
                    prev_log2_sub_cuh = log2_sub_cuh;
                }
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
            if (cost_best - 0.0001 > cost_temp)
            {
                /* backup the current best data */
                copy_cu_data(&core->cu_data_best[log2_cuw - 2][log2_cuh - 2], &core->cu_data_temp[log2_cuw - 2][log2_cuh - 2]
                           , 0, 0, log2_cuw, log2_cuh, log2_cuw, cud, core->tree_cons, ctx->sps.chroma_format_idc);
                cost_best = cost_temp;
                best_dqp = core->dqp_data[prev_log2_sub_cuw - 2][prev_log2_sub_cuh - 2].prev_qp;
                DQP_STORE(dqp_temp_depth, core->dqp_next_best[prev_log2_sub_cuw - 2][prev_log2_sub_cuh - 2]);
                SBAC_STORE(s_temp_depth, core->s_next_best[prev_log2_sub_cuw - 2][prev_log2_sub_cuh - 2]);
                best_split_mode = split_mode;
            }
        }
    }

    mode_cpy_rec_to_ref(core, x0, y0, cuw, cuh, PIC_MODE(ctx), core->tree_cons,ctx->sps.chroma_format_idc);

    /* restore best data */
    xeve_set_split_mode(best_split_mode, cud, 0, cuw, cuh, cuw, core->cu_data_best[log2_cuw - 2][log2_cuh - 2].split_mode);

    SBAC_LOAD(core->s_next_best[log2_cuw - 2][log2_cuh - 2], s_temp_depth);
    DQP_LOAD(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], dqp_temp_depth);

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

int xeve_mode_init_mt(XEVE_CTX *ctx, int thread_idx)
{
    XEVE_MODE *mi;
    int ret;

    mi = &ctx->mode[thread_idx];

    /* set default values to mode information */
    mi->log2_culine = ctx->log2_max_cuwh - MIN_CU_LOG2;

    /* initialize pintra */
    if (ctx->fn_pintra_init_mt)
    {
        ret = ctx->fn_pintra_init_mt(ctx, thread_idx);
        xeve_assert_rv(ret == XEVE_OK, ret);
    }

    /* initialize pinter */
    if (ctx->fn_pinter_init_mt)
    {
        ret = ctx->fn_pinter_init_mt(ctx, thread_idx);
        xeve_assert_rv(ret == XEVE_OK, ret);
    }

    return XEVE_OK;
}

int mode_init_lcu(XEVE_CTX *ctx, XEVE_CORE *core)
{
    int ret;

    /*initialize lambda for lcu */
    set_lambda(ctx, core, ctx->sh, ctx->sh->qp);
    /* initialize pintra */
    if(ctx->fn_pintra_init_lcu)
    {
        ret = ctx->fn_pintra_init_lcu(ctx, core);
        xeve_assert_rv(ret == XEVE_OK, ret);
    }

    /* initialize pinter */
    if(ctx->fn_pinter_init_lcu)
    {
        ret = ctx->fn_pinter_init_lcu(ctx, core);
        xeve_assert_rv(ret == XEVE_OK, ret);
    }

    /* initialize cu data */
    init_cu_data(&core->cu_data_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2], ctx->log2_max_cuwh, ctx->log2_max_cuwh, ctx->qp, ctx->qp, ctx->qp);
    init_cu_data(&core->cu_data_temp[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2], ctx->log2_max_cuwh, ctx->log2_max_cuwh, ctx->qp, ctx->qp, ctx->qp);

    return XEVE_OK;
}

static int mode_post_lcu(XEVE_CTX *ctx, XEVE_CORE *core)
{
    return XEVE_OK;
}

static int mode_analyze_frame(XEVE_CTX *ctx)
{
    return XEVE_OK;
}

void update_to_ctx_map(XEVE_CTX *ctx, XEVE_CORE *core)
{
    XEVE_CU_DATA *cu_data;
    int   cuw, cuh, i, j, w, h;
    int   x, y;
    int   core_idx, ctx_idx;
    s8  (*map_refi)[REFP_NUM];
    s16 (*map_mv)[REFP_NUM][MV_D];
    s16 (*map_unrefined_mv)[REFP_NUM][MV_D];
    s8   *map_ipm;

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

    map_ipm = ctx->map_ipm;
    map_refi = ctx->map_refi;
    map_mv = ctx->map_mv;
    map_unrefined_mv = ctx->map_unrefined_mv;

    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            if(cu_data->pred_mode[core_idx + j] == MODE_INTRA)
            {
                map_ipm[ctx_idx + j] = cu_data->ipm[0][core_idx + j];
                map_mv[ctx_idx + j][REFP_0][MV_X] = 0;
                map_mv[ctx_idx + j][REFP_0][MV_Y] = 0;
                map_mv[ctx_idx + j][REFP_1][MV_X] = 0;
                map_mv[ctx_idx + j][REFP_1][MV_Y] = 0;
            }
            else
            {
                map_refi[ctx_idx + j][REFP_0] = cu_data->refi[core_idx + j][REFP_0];
                map_refi[ctx_idx + j][REFP_1] = cu_data->refi[core_idx + j][REFP_1];
                map_mv[ctx_idx + j][REFP_0][MV_X] = cu_data->mv[core_idx + j][REFP_0][MV_X];
                map_mv[ctx_idx + j][REFP_0][MV_Y] = cu_data->mv[core_idx + j][REFP_0][MV_Y];
                map_mv[ctx_idx + j][REFP_1][MV_X] = cu_data->mv[core_idx + j][REFP_1][MV_X];
                map_mv[ctx_idx + j][REFP_1][MV_Y] = cu_data->mv[core_idx + j][REFP_1][MV_Y];

                if (cu_data->dmvr_flag[core_idx + j])
                {
                    map_unrefined_mv[ctx_idx + j][REFP_0][MV_X] = cu_data->unrefined_mv[core_idx + j][REFP_0][MV_X];
                    map_unrefined_mv[ctx_idx + j][REFP_0][MV_Y] = cu_data->unrefined_mv[core_idx + j][REFP_0][MV_Y];
                    map_unrefined_mv[ctx_idx + j][REFP_1][MV_X] = cu_data->unrefined_mv[core_idx + j][REFP_1][MV_X];
                    map_unrefined_mv[ctx_idx + j][REFP_1][MV_Y] = cu_data->unrefined_mv[core_idx + j][REFP_1][MV_Y];
                }
                else
                {
                    map_unrefined_mv[ctx_idx + j][REFP_0][MV_X] = cu_data->mv[core_idx + j][REFP_0][MV_X];
                    map_unrefined_mv[ctx_idx + j][REFP_0][MV_Y] = cu_data->mv[core_idx + j][REFP_0][MV_Y];
                    map_unrefined_mv[ctx_idx + j][REFP_1][MV_X] = cu_data->mv[core_idx + j][REFP_1][MV_X];
                    map_unrefined_mv[ctx_idx + j][REFP_1][MV_Y] = cu_data->mv[core_idx + j][REFP_1][MV_Y];
                }
            }
        }
        ctx_idx += ctx->w_scu;
        core_idx += (ctx->max_cuwh >> MIN_CU_LOG2);
    }

    update_map_scu(ctx, core, core->x_pel, core->y_pel, ctx->max_cuwh, ctx->max_cuwh);
}

static int mode_analyze_lcu(XEVE_CTX *ctx, XEVE_CORE *core)
{
    XEVE_MODE *mi;
    u32 *map_scu;
    int  w, h;

    mi = &ctx->mode[core->thread_cnt];

    xeve_mset(mi->mvp_idx, 0, sizeof(u8) * REFP_NUM);
    xeve_mset(mi->mvd, 0, sizeof(s16) * REFP_NUM * MV_D);

    /* decide mode */
    mode_coding_tree(ctx, core, core->x_pel, core->y_pel, 0, ctx->log2_max_cuwh, ctx->log2_max_cuwh, 0, mi, 1, ctx->tile[core->tile_idx].qp, xeve_get_default_tree_cons() );

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

    update_to_ctx_map(ctx, core);
    copy_cu_data(&ctx->map_cu_data[core->lcu_num], &core->cu_data_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2]
               , 0, 0, ctx->log2_max_cuwh, ctx->log2_max_cuwh, ctx->log2_max_cuwh, 0, xeve_get_default_tree_cons(), ctx->sps.chroma_format_idc);

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

    int i, j;
    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            MCU_CLR_COD(map_scu[j]);
        }
        map_scu += ctx->w_scu;
    }

    return XEVE_OK;
}

static int mode_set_complexity(XEVE_CTX *ctx, int complexity)
{
    XEVE_MODE  *mi;

    mi = &ctx->mode[0];
    xeve_assert_rv(mi != NULL, XEVE_ERR_UNEXPECTED);

    return XEVE_OK;
}

int xeve_mode_create(XEVE_CTX *ctx, int complexity)
{
    XEVE_MODE *mi;

    mi = &ctx->mode[0];

    /* create mode information structure */
    xeve_assert_rv(mi, XEVE_ERR_OUT_OF_MEMORY);
    xeve_mset(mi, 0, sizeof(XEVE_MODE));

    /* set function addresses */
    ctx->fn_mode_init_mt = xeve_mode_init_mt;
    ctx->fn_mode_init_lcu = mode_init_lcu;
    ctx->fn_mode_analyze_frame = mode_analyze_frame;
    ctx->fn_mode_analyze_lcu = mode_analyze_lcu;
    ctx->fn_mode_set_complexity = mode_set_complexity;
    ctx->fn_mode_copy_to_cu_data = copy_to_cu_data;
    ctx->fn_mode_post_lcu = mode_post_lcu;
    ctx->fn_mode_reset_intra = NULL;
    ctx->fn_mode_rdo_dbk_map_set = NULL;
    ctx->fn_mode_rdo_bit_cnt_intra_dir = xeve_rdo_bit_cnt_intra_dir;;

    return ctx->fn_mode_set_complexity(ctx, complexity);
}

/******************************************************************************
 * picture buffer alloc/free/expand
 ******************************************************************************/

static void picbuf_expand(pel *a, int s, int w, int h, int exp)
{
    int i, j;
    pel pixel;
    pel *src, *dst;

    /* left */
    src = a;
    dst = a - exp;

    for(i = 0; i < h; i++)
    {
        pixel = *src; /* get boundary pixel */
        for(j = 0; j < exp; j++)
        {
            dst[j] = pixel;
        }
        dst += s;
        src += s;
    }

    /* right */
    src = a + (w - 1);
    dst = a + w;

    for(i = 0; i < h; i++)
    {
        pixel = *src; /* get boundary pixel */
        for(j = 0; j < exp; j++)
        {
            dst[j] = pixel;
        }
        dst += s;
        src += s;
    }

    /* upper */
    src = a - exp;
    dst = a - exp - (exp * s);

    for(i = 0; i < exp; i++)
    {
        xeve_mcpy(dst, src, s*sizeof(pel));
        dst += s;
    }

    /* below */
    src = a + ((h - 1)*s) - exp;
    dst = a + ((h - 1)*s) - exp + s;

    for(i = 0; i < exp; i++)
    {
        xeve_mcpy(dst, src, s*sizeof(pel));
        dst += s;
    }
}

void xeve_pic_expand(XEVE_CTX *ctx, XEVE_PIC *pic)
{
    xeve_picbuf_expand(pic, pic->pad_l, pic->pad_c, ctx->sps.chroma_format_idc);
}

XEVE_PIC * xeve_pic_alloc(PICBUF_ALLOCATOR * pa, int * ret)
{
    return xeve_picbuf_alloc(pa->w, pa->h, pa->pad_l, pa->pad_c, pa->bit_depth, ret, pa->chroma_format_idc);
}

void xeve_pic_free(PICBUF_ALLOCATOR *pa, XEVE_PIC *pic)
{
    xeve_picbuf_free(pic);
}

/******************************************************************************
 * implementation of bitstream writer
 ******************************************************************************/
void xeve_bsw_skip_slice_size(XEVE_BSW *bs)
{
    xeve_bsw_write(bs, 0, 32);
}

int xeve_bsw_write_nalu_size(XEVE_BSW *bs)
{
    u32 size;

    size = XEVE_BSW_GET_WRITE_BYTE(bs) - 4;

    bs->beg[0] = size & 0x000000ff;
    bs->beg[1] = (size & 0x0000ff00) >> 8;
    bs->beg[2] = (size & 0x00ff0000) >> 16;
    bs->beg[3] = (size & 0xff000000) >> 24;
    return size;
}

void xeve_diff_pred(int x, int y, int log2_cuw, int log2_cuh, XEVE_PIC *org, pel pred[N_C][MAX_CU_DIM], s16 diff[N_C][MAX_CU_DIM], int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc)
{
    pel * buf;
    int cuw, cuh, stride;
    int w_shift = XEVE_GET_CHROMA_W_SHIFT(chroma_format_idc);
    int h_shift = XEVE_GET_CHROMA_H_SHIFT(chroma_format_idc);

    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;
    stride = org->s_l;

    /* Y */
    buf = org->y + (y * stride) + x;

    xeve_diff_16b(log2_cuw, log2_cuh, buf, pred[Y_C], stride, cuw, cuw, diff[Y_C], bit_depth_luma);

    if(!chroma_format_idc)
        return;

    cuw >>= w_shift;
    cuh >>= h_shift;
    x >>= w_shift;
    y >>= h_shift;
    log2_cuw -= w_shift;
    log2_cuh -= h_shift;

    stride = org->s_c;

    /* U */
    buf = org->u + (y * stride) + x;
    xeve_diff_16b(log2_cuw, log2_cuh, buf, pred[U_C], stride, cuw, cuw, diff[U_C], bit_depth_chroma);

    /* V */
    buf = org->v + (y * stride) + x;
    xeve_diff_16b(log2_cuw, log2_cuh, buf, pred[V_C], stride, cuw, cuw, diff[V_C], bit_depth_chroma);
}

void xeve_set_qp(XEVE_CTX *ctx, XEVE_CORE *core, u8 qp)
{
    u8 qp_i_cb, qp_i_cr;
    core->qp = qp;
    core->qp_y = GET_LUMA_QP(core->qp, ctx->sps.bit_depth_luma_minus8);
    qp_i_cb = XEVE_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, core->qp + ctx->sh->qp_u_offset);
    qp_i_cr = XEVE_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, core->qp + ctx->sh->qp_v_offset);
    core->qp_u = ctx->qp_chroma_dynamic[0][qp_i_cb] + 6 * ctx->sps.bit_depth_chroma_minus8;
    core->qp_v = ctx->qp_chroma_dynamic[1][qp_i_cr] + 6 * ctx->sps.bit_depth_chroma_minus8;
}

MODE_CONS xeve_derive_mode_cons(XEVE_CTX *ctx, int lcu_num, int cup)
{
    return ((ctx->map_cu_data[lcu_num].pred_mode[cup] == MODE_INTRA) || (ctx->map_cu_data[lcu_num].pred_mode[cup] == MODE_IBC)) ? eOnlyIntra : eOnlyInter;
}

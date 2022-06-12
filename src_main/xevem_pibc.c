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
#include "xevem_mc.h"
#if x86_SSE
#include "xevem_mc_sse.h"
#endif
#include <math.h>

#define ENABLE_IBC_CHROMA_REFINE 0

#define SWAP(a, b, t) { (t) = (a); (a) = (b); (b) = (t); }

#define CHROMA_REFINEMENT_CANDIDATES         8  /* 8 candidates BV to choose from */

int is_bv_valid(XEVE_CTX *ctx, int x, int y, int width, int height, int log2_cuw, int log2_cuh
              , int pic_width, int pic_height, int x_bv, int y_bv, int ctu_size, XEVE_CORE * core)
{
    XEVEM_CTX *mctx = (XEVEM_CTX *)ctx;
    int x_scu = 0, y_scu = 0;
    int log2_scuw = 0, log2_scuh = 0;
    int scuw = 0, scuh = 0;

    x_scu = PEL2SCU(x);
    y_scu = PEL2SCU(y);

    log2_scuw = log2_cuw - MIN_CU_LOG2;
    log2_scuh = log2_cuh - MIN_CU_LOG2;
    scuw = 1 << log2_scuw;
    scuh = 1 << log2_scuh;

    const int ctu_size_log2 = mctx->pibc[core->thread_cnt].ctu_log2_tbl[ctu_size];

    int ref_right_x = x + x_bv + width - 1;
    int ref_bottom_y = y + y_bv + height - 1;

    int ref_left_x = x + x_bv;
    int ref_top_y = y + y_bv;
    int x_l = ((ctx->tile[core->tile_num].ctba_rs_first) % ctx->w_lcu) << MAX_CU_LOG2; //entry point lcu's x location
    int y_l = ((ctx->tile[core->tile_num].ctba_rs_first) / ctx->w_lcu) << MAX_CU_LOG2; // entry point lcu's y location
    int x_r = x_l + ((int)(ctx->tile[core->tile_num].w_ctb) << MAX_CU_LOG2);
    int y_r = y_l + ((int)(ctx->tile[core->tile_num].h_ctb) << MAX_CU_LOG2);
    x_r = x_r > pic_width ? pic_width : x_r;
    y_r = y_r > pic_height ? pic_height : y_r;

    if ((x + x_bv) < x_l)
    {
        return 0;
    }
    if (ref_right_x >= x_r)
    {
        return 0;
    }
    if ((y + y_bv) < y_l)
    {
        return 0;
    }
    if (ref_bottom_y >= y_r)
    {
        return 0;
    }
    if ((x_bv + width) > x_l && (y_bv + height) > y_l)
    {
        return 0;
    }
    if ((ref_top_y >> ctu_size_log2) < (y >> ctu_size_log2))
        return 0;
    if ((ref_bottom_y >> ctu_size_log2) > (y >> ctu_size_log2))
    {
        return 0;
    }

    // in the same CTU line
    if (((ref_right_x >> ctu_size_log2) <= (x >> ctu_size_log2)) && ((ref_left_x >> ctu_size_log2) >= (x >> ctu_size_log2) - 1))
    {
        // in the same CTU, or left CTU
        // if part of ref block is in the left CTU, some area can be referred from the not-yet updated local CTU buffer
        if ((ref_left_x >> ctu_size_log2) == ((x >> ctu_size_log2) - 1))
        {
            // top left position of ref block's collocated block in current CTU
            int ref_pos_col_x = x + x_bv + ctu_size;
            int ref_pos_col_y = y + y_bv;
            int offset64x = (ref_pos_col_x >> (ctu_size_log2 - 1)) << (ctu_size_log2 - 1);
            int offset64y = (ref_pos_col_y >> (ctu_size_log2 - 1)) << (ctu_size_log2 - 1);
            int offset_x_scu = PEL2SCU(offset64x);
            int offset_y_scu = PEL2SCU(offset64y);
            int offset_scup = (offset_y_scu * ctx->w_scu) + offset_x_scu;
            int curr_scup = ((y_scu)* ctx->w_scu) + (x_scu);
            int avail_cu = MCU_GET_COD(ctx->map_scu[offset_scup]) && (ctx->map_tidx[curr_scup] == ctx->map_tidx[offset_scup]);

            if (avail_cu)
            {
                return 0;
            }

            //corn case: start coding first block in 64x64 CU, then should disable ref 64x64 CU
            if (offset64x == x && offset64y == y)
            {
                return 0;
            }

            if (ctx->sps.sps_suco_flag)
            {
                // top right position of ref block's collocated block in current CTU
                int offset64_TR_x = offset64x + (1 << (ctu_size_log2 - 1)) - 1;
                if (offset64_TR_x >= pic_width)
                {
                    offset64_TR_x = pic_width - 1;
                }

                int offset64_TR_y = offset64y;
                int offset_TR_x_scu = PEL2SCU(offset64_TR_x);
                int offset_TR_y_scu = PEL2SCU(offset64_TR_y);
                int offset_TR_scup = (offset_TR_y_scu * ctx->w_scu) + offset_TR_x_scu;
                curr_scup = ((y_scu)* ctx->w_scu) + (x_scu);

                int avail_TR_cu = MCU_GET_COD(ctx->map_scu[offset_TR_scup]) && (ctx->map_tidx[curr_scup] == ctx->map_tidx[offset_TR_scup]);
                if (avail_TR_cu)
                {
                    return 0;
                }

                if (offset64_TR_x == (x + (1 << log2_cuw) - 1) && offset64_TR_y == y)
                {
                    return 0;
                }

                //Check the collocated 64x64 region of the reference block's top-right corner is valid for reference or not
                int RT_ref_pos_LT_col_x = x + x_bv + ctu_size + width - 1;
                if (RT_ref_pos_LT_col_x < pic_width)
                {
                    int RT_ref_pos_LT_offset64x = (RT_ref_pos_LT_col_x >> (ctu_size_log2 - 1)) << (ctu_size_log2 - 1);
                    int RT_ref_pos_LT_col_y = y + y_bv;
                    int RT_ref_pos_LT_offset64y = (RT_ref_pos_LT_col_y >> (ctu_size_log2 - 1)) << (ctu_size_log2 - 1);
                    int RT_ref_pos_LT_x_scu = PEL2SCU(RT_ref_pos_LT_offset64x);
                    int RT_ref_pos_LT_y_scu = PEL2SCU(RT_ref_pos_LT_offset64y);
                    int RT_ref_pos_LT_scup = (RT_ref_pos_LT_y_scu * ctx->w_scu) + RT_ref_pos_LT_x_scu;
                    curr_scup = ((y_scu)* ctx->w_scu) + (x_scu);

                    int RT_ref_pos_LT_cu = MCU_GET_COD(ctx->map_scu[RT_ref_pos_LT_scup]) && (ctx->map_tidx[curr_scup] == ctx->map_tidx[RT_ref_pos_LT_scup]);
                    if (RT_ref_pos_LT_cu)
                    {
                        return 0;
                    }

                    if (RT_ref_pos_LT_offset64x == (x + width - 1) && RT_ref_pos_LT_col_y == y)
                    {
                        return 0;
                    }

                    int RT_ref_pos_RT_offset64x = RT_ref_pos_LT_offset64x + (1 << (ctu_size_log2 - 1)) - 1;
                    int RT_ref_pos_RT_col_y = RT_ref_pos_LT_col_y;
                    int RT_ref_pos_RT_offset64y = (RT_ref_pos_RT_col_y >> (ctu_size_log2 - 1)) << (ctu_size_log2 - 1);
                    int RT_ref_pos_RT_x_scu = PEL2SCU(RT_ref_pos_RT_offset64x);
                    int RT_ref_pos_RT_y_scu = PEL2SCU(RT_ref_pos_RT_offset64y);
                    int RT_ref_pos_RT_scup = (RT_ref_pos_RT_y_scu * ctx->w_scu) + RT_ref_pos_RT_x_scu;
                    curr_scup = ((y_scu)* ctx->w_scu) + (x_scu);

                    int RT_ref_pos_RT_cu = MCU_GET_COD(ctx->map_scu[RT_ref_pos_RT_scup]) && (ctx->map_tidx[curr_scup] == ctx->map_tidx[RT_ref_pos_RT_scup]);
                    if (RT_ref_pos_RT_cu)
                    {
                        return 0;
                    }

                    if (RT_ref_pos_RT_offset64x == (x + width - 1) && RT_ref_pos_RT_offset64y == y)
                    {
                        return 0;
                    }
                }
            }
        }
    }
    else
    {
        return 0;
    }

    // in the same CTU, or valid area from left CTU. Check if the reference block is already coded
    int ref_pos_LT_x = x + x_bv;
    int ref_pos_LT_y = y + y_bv;
    int ref_pos_LT_x_scu = PEL2SCU(ref_pos_LT_x);
    int ref_pos_LT_y_scu = PEL2SCU(ref_pos_LT_y);
    int ref_pos_LT_scup = (ref_pos_LT_y_scu * ctx->w_scu) + ref_pos_LT_x_scu;
    int curr_scup = ((y_scu)* ctx->w_scu) + (x_scu);
    int avail_cu = MCU_GET_COD(ctx->map_scu[ref_pos_LT_scup]) && (ctx->map_tidx[curr_scup] == ctx->map_tidx[ref_pos_LT_scup]);
    if (avail_cu == 0)
    {
        return 0;
    }

    int ref_pos_BR_x = x + width - 1 + x_bv;
    int ref_pos_BR_y = y + height - 1 + y_bv;
    int ref_pos_BR_x_scu = PEL2SCU(ref_pos_BR_x);
    int ref_pos_BR_y_scu = PEL2SCU(ref_pos_BR_y);
    int ref_pos_BR_scup = (ref_pos_BR_y_scu * ctx->w_scu) + ref_pos_BR_x_scu;

    curr_scup = ((y_scu)* ctx->w_scu) + (x_scu);
    avail_cu = MCU_GET_COD(ctx->map_scu[ref_pos_BR_scup]) && (ctx->map_tidx[curr_scup] == ctx->map_tidx[ref_pos_BR_scup]);
    if (avail_cu == 0)
    {
        return 0;
    }

    if (ctx->sps.sps_suco_flag)
    {
        // check the availablity of bottom-left corner
        int ref_pos_BL_scup = (ref_pos_BR_y_scu * ctx->w_scu) + ref_pos_LT_x_scu;
        int curr_scup = ((y_scu)* ctx->w_scu) + (x_scu);

        avail_cu = MCU_GET_COD(ctx->map_scu[ref_pos_BL_scup]) && (ctx->map_tidx[curr_scup] == ctx->map_tidx[ref_pos_BL_scup]);
        if (avail_cu == 0)
        {
            return 0;
        }

        // check if the reference block cross the uncoded block
        if (ref_pos_BR_x >= x && ref_pos_BR_y < y)
        {
            int check_point_x = ref_pos_LT_x + width / 2;
            int check_point_y = ref_pos_BR_y;
            int check_point_x_scu = PEL2SCU(check_point_x);
            int check_point_y_scu = PEL2SCU(check_point_y);
            int check_point_scup = (check_point_y_scu * ctx->w_scu) + check_point_x_scu;
            int curr_scup = ((y_scu)* ctx->w_scu) + (x_scu);

            avail_cu = MCU_GET_COD(ctx->map_scu[check_point_scup]) && (ctx->map_tidx[curr_scup] == ctx->map_tidx[check_point_scup]);
            if (avail_cu == 0)
            {
                return 0;
            }
        }
    }
    return 1;
}

__inline u32 get_exp_golomb_bits(u32 abs_mvd)
{
    int bits = 0;
    int len_i, len_c, nn;

    /* abs(mvd) */
    nn = ((abs_mvd + 1) >> 1);
    for (len_i = 0; len_i < 16 && nn != 0; len_i++)
    {
        nn >>= 1;
    }
    len_c = (len_i << 1) + 1;

    bits += len_c;

    /* sign */
    if (abs_mvd)
    {
        bits++;
    }

    return bits;
}

static double pibc_residue_rdo(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh
                             , pel pred[2][N_C][MAX_CU_DIM], s16 coef[N_C][MAX_CU_DIM], u8 mvp_idx, s16 match_pos[MV_D])
{
    XEVEM_CTX  *mctx = (XEVEM_CTX *)ctx;
    XEVEM_CORE *mcore = (XEVEM_CORE *)core;
    XEVE_PIBC  *pi = &mctx->pibc[core->thread_cnt];

    int   *nnz, tnnz, w[N_C], h[N_C], log2_w[N_C], log2_h[N_C];
    int    cuw;
    int    cuh;
    pel  (*rec)[MAX_CU_DIM];
    s64    dist[N_C];
    double cost, cost_best = MAX_COST;
    int    nnz_store[N_C];
    int    bit_cnt;
    int    i;
    pel   *org[N_C];
    double cost_comp_best = MAX_COST;
    int    idx_best[N_C] = { 0, };
    u8     is_from_mv_field = 0;

    mcore->ats_inter_info = 0;

    int start_c = xeve_check_luma(core->tree_cons) ? Y_C : U_C;
    int end_c = xeve_check_chroma(core->tree_cons) ? N_C : U_C;
    end_c = ctx->sps.chroma_format_idc == 0 ? U_C : end_c;
    int bit_depth_tbl[3] = { ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8 };
    int w_shift = ctx->param.cs_w_shift;
    int h_shift = ctx->param.cs_h_shift;

    rec = pi->unfiltered_rec_buf;
    nnz = core->nnz;
    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;
    w[Y_C] = 1 << log2_cuw;
    h[Y_C] = 1 << log2_cuh;
    w[U_C] = w[V_C] = 1 << (log2_cuw - w_shift);
    h[U_C] = h[V_C] = 1 << (log2_cuh - h_shift);
    log2_w[Y_C] = log2_cuw;
    log2_h[Y_C] = log2_cuh;
    log2_w[U_C] = log2_w[V_C] = log2_cuw - w_shift;
    log2_h[U_C] = log2_h[V_C] = log2_cuh - h_shift;
    org[Y_C] = pi->o[Y_C] + (y * pi->s_o[Y_C]) + x;
    org[U_C] = pi->o[U_C] + ((y >> h_shift) * pi->s_o[U_C]) + (x >> w_shift);
    org[V_C] = pi->o[V_C] + ((y >> h_shift) * pi->s_o[V_C]) + (x >> w_shift);

    xeve_IBC_mc(x, y, log2_cuw, log2_cuh, match_pos, pi->pic_m, pred[0], core->tree_cons, ctx->sps.chroma_format_idc);

    /* get residual */
    xeve_diff_pred(x, y, log2_cuw, log2_cuh, pi->pic_o, pred[0], coef, ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc);
    if(ctx->pps.cu_qp_delta_enabled_flag)
    {
        xeve_set_qp(ctx, core, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].curr_qp);
    }
    /* transform and quantization */
    tnnz = ctx->fn_tq(ctx, core, coef, log2_cuw, log2_cuh, pi->slice_type, nnz, 0, RUN_L | RUN_CB | RUN_CR);

    if (tnnz)
    {
        for (i = start_c; i < end_c; i++)
        {
            int size = (cuw * cuh) >> (i == 0 ? 0 : (w_shift + h_shift));
            xeve_mcpy(pi->inv_coef[i], coef[i], sizeof(s16) * size);
            nnz_store[i] = nnz[i];
        }

        ctx->fn_itdp(ctx, core, pi->inv_coef, core->nnz_sub);

        for (i = start_c; i < end_c; i++)
        {
            ctx->fn_recon(ctx, core, pi->inv_coef[i], pred[0][i], nnz[i], w[i], h[i], w[i], rec[i], bit_depth_tbl[i]);
            dist[i] = xeve_ssd_16b(log2_w[i], log2_h[i], rec[i], org[i], w[i], pi->s_o[i], bit_depth_tbl[i]);
        }

        if(ctx->param.rdo_dbk_switch)
        {
            //filter rec and calculate ssd
            calc_delta_dist_filter_boundary(ctx, PIC_MODE(ctx), PIC_ORIG(ctx), cuw, cuh, rec, cuw, x, y, core->avail_lr, 0, nnz[Y_C] != 0, NULL, pi->mv, is_from_mv_field, core);

            for (i = start_c; i < end_c; i++)
            {
                dist[i] += core->delta_dist[i];
            }
        }

        cost = 0.0;
        if (xeve_check_luma(core->tree_cons))
        {
            cost += (double)dist[Y_C];
        }
        if (xeve_check_chroma(core->tree_cons))
        {
            cost += (((double)dist[U_C] * core->dist_chroma_weight[0]) + ((double)dist[V_C] * core->dist_chroma_weight[1]));
        }

        SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
        DQP_LOAD(core->dqp_temp_run, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);

        xeve_sbac_bit_reset(&core->s_temp_run);
        xeve_rdo_bit_cnt_cu_ibc(ctx, core, ctx->sh->slice_type, core->scup, pi->mvd, coef, mvp_idx, pi->ibc_flag);

        bit_cnt = xeve_get_bit_number(&core->s_temp_run);
        cost += RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt);

        if (cost < cost_best)
        {
            cost_best = cost;
            SBAC_STORE(core->s_temp_best, core->s_temp_run);
            DQP_STORE(core->dqp_temp_best, core->dqp_temp_run);
        }

        SBAC_LOAD(core->s_temp_prev_comp_best, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);

        for (i = start_c; i < end_c; i++)
        {
            nnz[i] = nnz_store[i];
            if (nnz[i] == 0 && nnz_store[i] != 0)
            {
                xeve_mset(coef[i], 0, sizeof(s16) * ((cuw * cuh) >> (i == 0 ? 0 : (w_shift + h_shift))));
            }
        }
    }
    else
    {
        if (ctx->pps.cu_qp_delta_enabled_flag)
        {
            if (core->cu_qp_delta_code_mode != 2)
            {
                xeve_set_qp(ctx, core, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].prev_qp);
            }
        }

        for (i = start_c; i < end_c; i++)
        {
            nnz[i] = 0;
        }

        for (i = start_c; i < end_c; i++)
        {
            ctx->fn_recon(ctx, core, coef[i], pred[0][i], nnz[i], w[i], h[i], w[i], rec[i], bit_depth_tbl[i]);
            dist[i] = xeve_ssd_16b(log2_w[i], log2_h[i], rec[i], org[i], w[i], pi->s_o[i], bit_depth_tbl[i]);
        }
        if(ctx->param.rdo_dbk_switch)
        {
            calc_delta_dist_filter_boundary(ctx, PIC_MODE(ctx), PIC_ORIG(ctx), cuw, cuh, rec, cuw, x, y, core->avail_lr, 0, 0, NULL, pi->mv, is_from_mv_field, core);

            for (i = start_c; i < end_c; i++)
            {
                dist[i] += core->delta_dist[i];
            }
        }

        cost_best = 0.0;
        if (xeve_check_luma(core->tree_cons))
        {
            cost_best += (double)dist[Y_C];
        }
        if (xeve_check_chroma(core->tree_cons))
        {
            cost_best += (((double)dist[U_C] * core->dist_chroma_weight[0]) + ((double)dist[V_C] * core->dist_chroma_weight[1]));
        }

        DQP_LOAD(core->dqp_temp_run, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);
        SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);

        xeve_sbac_bit_reset(&core->s_temp_run);
        xeve_rdo_bit_cnt_cu_ibc(ctx, core, ctx->sh->slice_type, core->scup, pi->mvd, coef, mvp_idx, pi->ibc_flag);

        bit_cnt = xeve_get_bit_number(&core->s_temp_run);
        cost_best += RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt);
        SBAC_STORE(core->s_temp_best, core->s_temp_run);
        DQP_STORE(core->dqp_temp_best, core->dqp_temp_run);
    }

    return cost_best;
}

static void clip_ibc_mv(int rc_mv[2], int pic_width, int pic_height, int lcu_width, int lcu_height, int cu_pos_x, int cu_pos_y)
{
    int offset = 8;
    int hor_max = (pic_width + offset - cu_pos_x - 1);
    int hor_min = (-lcu_width - offset - cu_pos_x + 1);

    int ver_max = (pic_height + offset - cu_pos_y - 1);
    int ver_min = (-lcu_height - offset - cu_pos_y + 1);

    rc_mv[0] = XEVE_MIN(hor_max, XEVE_MAX(hor_min, rc_mv[0]));
    rc_mv[1] = XEVE_MIN(ver_max, XEVE_MAX(ver_min, rc_mv[1]));
}

static void ibc_set_search_range(XEVE_CTX *ctx, XEVE_CORE *core, int cu_pel_x, int cu_pel_y, int log2_cuw, int log2_cuh,
                                 const int local_search_range_x, const int local_search_range_y, int mv_search_range_left[2], int mv_search_range_right[2])
{
    int search_left = 0;
    int search_right = 0;
    int search_top = 0;
    int search_bottom = 0;

    const int roi_width = (1 << log2_cuw);
    const int roi_height = (1 << log2_cuh);

    const int pic_width = ctx->w;
    const int pic_height = ctx->h;

    search_left = -XEVE_MIN(cu_pel_x, local_search_range_x);
    search_top = -XEVE_MIN(cu_pel_y, local_search_range_y);

    search_right = XEVE_MIN(pic_width - cu_pel_x - roi_width, local_search_range_x);
    search_bottom = XEVE_MIN(pic_height - cu_pel_y - roi_height, local_search_range_y);

    mv_search_range_left[0] = search_left;
    mv_search_range_left[1] = search_top;
    mv_search_range_right[0] = search_right;
    mv_search_range_right[1] = search_bottom;

    clip_ibc_mv(mv_search_range_left, pic_width, pic_height, ctx->max_cuwh, ctx->max_cuwh,
        cu_pel_x, cu_pel_y);
    clip_ibc_mv(mv_search_range_right, pic_width, pic_height, ctx->max_cuwh, ctx->max_cuwh,
        cu_pel_x, cu_pel_y);
}

static void update_ibc_mv_cand(u32 sad, int x, int y, u32 *sad_best_cand, s16 mv_cand[CHROMA_REFINEMENT_CANDIDATES][MV_D])
{
    int j = CHROMA_REFINEMENT_CANDIDATES - 1;

    if (sad < sad_best_cand[CHROMA_REFINEMENT_CANDIDATES - 1])
    {
        for (int t = CHROMA_REFINEMENT_CANDIDATES - 1; t >= 0; t--)
        {
            if (sad < sad_best_cand[t])
                j = t;
        }

        for (int k = CHROMA_REFINEMENT_CANDIDATES - 1; k > j; k--)
        {
            sad_best_cand[k] = sad_best_cand[k - 1];

            mv_cand[k][0] = mv_cand[k - 1][0];
            mv_cand[k][1] = mv_cand[k - 1][1];
        }
        sad_best_cand[j] = sad;
        mv_cand[j][0] = x;
        mv_cand[j][1] = y;
    }
}

#if ENABLE_IBC_CHROMA_REFINE
static int refine_ibc_chroma_mv(XEVE_CTX *ctx,
    XEVE_CORE *core,
    XEVE_PIBC *pi,
    int cu_x,
    int cu_y,
    int log2_cuw,
    int log2_cuh,
    int pic_width,
    int pic_height,
    u32 *sad_best_cand,
    s16 mv_cand[CHROMA_REFINEMENT_CANDIDATES][MV_D])
{
    int best_cand_idx = 0;
    u32 sad_best = XEVE_UINT32_MAX;

    u32 temp_sad = 0;

    int luma_cuw = 0, luma_cuh = 0;
    int chroma_cuw = 0, chroma_cuh = 0;

    pel pred[N_C][MAX_CU_DIM];

    pel *org = NULL;
    pel *ref = NULL;

    int ref_stride = 0, org_stride = 0;
    int chroma_cu_x = 0, chroma_cu_y = 0;

    XEVE_PIC *ref_pic = NULL;

    luma_cuw = 1 << log2_cuw;
    luma_cuh = 1 << log2_cuh;
    chroma_cuw = luma_cuw >> 1;
    chroma_cuh = luma_cuh >> 1;

    chroma_cu_x = cu_x >> 1;
    chroma_cu_y = cu_y >> 1;
    org_stride = pi->pic_o->s_c;

    ref_pic = pi->pic_m;

    ref_stride = ref_pic->s_c;

    for (int cand = 0; cand < CHROMA_REFINEMENT_CANDIDATES; cand++)
    {
        if ((!mv_cand[cand][0]) && (!mv_cand[cand][1]))
            continue;

        if (((int)(cu_y + mv_cand[cand][1] + luma_cuh) >= pic_height) || ((cu_y + mv_cand[cand][1]) < 0))
            continue;

        if (((int)(cu_x + mv_cand[cand][0] + luma_cuw) >= pic_width) || ((cu_x + mv_cand[cand][0]) < 0))
            continue;

        temp_sad = sad_best_cand[cand];

        xeve_IBC_mc(cu_x, cu_y, log2_cuw, log2_cuh, mv_cand[cand], ref_pic, pred, core->tree_cons, ctx->sps.chroma_format_idc);

        org = pi->pic_o->u + chroma_cu_y * org_stride + chroma_cu_x;
        ref = pred[U_C];
        temp_sad += xeve_sad_16b(log2_cuw - 1, log2_cuh - 1, org, ref, org_stride, chroma_cuw, ctx->sps.bit_depth_chroma_minus8 +8);

        org = pi->pic_o->v + chroma_cu_y * org_stride + chroma_cu_x;
        ref = pred[V_C];
        temp_sad += xeve_sad_16b(log2_cuw - 1, log2_cuh - 1, org, ref, org_stride, chroma_cuw, ctx->sps.bit_depth_chroma_minus8 + 8);

        if (temp_sad < sad_best)
        {
            sad_best = temp_sad;
            best_cand_idx = cand;
        }
    }

    return best_cand_idx;
}
#endif

static u32 get_comp_bits(int val)
{
    if (!val) return 1;

    u32 length = 1;
    u32 temp = (val <= 0) ? (-val << 1) + 1 : (val << 1);

    while (1 != temp)
    {
        temp >>= 1;
        length += 2;
    }

    return length;
}

u32 get_bv_cost_bits(int mv_x, int mv_y)
{
    return get_comp_bits(mv_x) + get_comp_bits(mv_y);
}

static int pibc_search_estimation(XEVE_CTX *ctx, XEVE_CORE *core, XEVE_PIBC *pi, int cu_x, int cu_y, int log2_cuw, int log2_cuh,
    s16 mvp[MV_D], s16 mv[MV_D])
{
    XEVEM_CTX *mctx = (XEVEM_CTX *)ctx;
    int mv_search_range_left[2] = { 0 };
    int mv_search_range_right[2] = { 0 };

    int srch_rng_hor_left = 0;
    int srch_rng_hor_right = 0;
    int srch_rng_ver_top = 0;
    int srch_rng_ver_bottom = 0;

    const unsigned int lcu_width = ctx->max_cuwh;
    const int pu_pel_offset_x = 0;
    const int pu_pel_offset_y = 0;

    const int cu_pel_x = cu_x;
    const int cu_pel_y = cu_y;

    int roi_width = (1 << log2_cuw);
    int roi_height = (1 << log2_cuh);

    //Distortion  sad;
    u32 sad = 0;
    u32 sad_best = XEVE_UINT32_MAX;
    u32 rui_cost = XEVE_UINT32_MAX;
    int bestX = 0;
    int bestY = 0;
    int mv_bits = 0, best_mv_bits = 0;
    XEVE_PIC *ref_pic = mctx->pibc[core->thread_cnt].pic_m;
    pel *org = pi->o[Y_C] + cu_y * pi->s_o[Y_C] + cu_x;
    pel *rec = ref_pic->y + cu_y * ref_pic->s_l + cu_x;
    pel *ref = rec;
    int best_cand_idx = 0;
    u32 sad_best_cand[CHROMA_REFINEMENT_CANDIDATES];
    s16 mv_cand[CHROMA_REFINEMENT_CANDIDATES][MV_D];

    ibc_set_search_range(ctx, core, cu_x, cu_y, log2_cuw, log2_cuh, mctx->pibc[core->thread_cnt].search_range_x,
        mctx->pibc[core->thread_cnt].search_range_y, mv_search_range_left, mv_search_range_right);

    srch_rng_hor_left = mv_search_range_left[0];
    srch_rng_hor_right = mv_search_range_right[0];
    srch_rng_ver_top = mv_search_range_left[1];
    srch_rng_ver_bottom = mv_search_range_right[1];

    mvp[MV_X] = 0;
    mvp[MV_Y] = 0;

    for (int cand = 0; cand < CHROMA_REFINEMENT_CANDIDATES; cand++)
    {
        sad_best_cand[cand] = XEVE_UINT32_MAX;
        mv_cand[cand][0] = 0;
        mv_cand[cand][1] = 0;
    }

    const int pic_width = ctx->w;
    const int pic_height = ctx->h;


    u32 tempSadBest = 0;

    int srLeft = srch_rng_hor_left, srRight = srch_rng_hor_right, srTop = srch_rng_ver_top, srBottom = srch_rng_ver_bottom;

    const int boundY = (0 - roi_height - pu_pel_offset_y);
    for (int y = XEVE_MAX(srch_rng_ver_top, 0 - cu_pel_y); y <= boundY; ++y)
    {
        if (!is_bv_valid(ctx, cu_pel_x, cu_pel_y, roi_width, roi_height, log2_cuw, log2_cuh, pic_width, pic_height, 0, y, lcu_width, core))
        {
            continue;
        }

        mv_bits = get_bv_cost_bits(0, y);
        sad = GET_BV_COST(ctx, mv_bits);

        /* get sad */
        ref = rec + ref_pic->s_l * y;
        sad += xeve_sad_16b(log2_cuw, log2_cuh, org, ref, pi->s_o[Y_C], ref_pic->s_l, ctx->sps.bit_depth_luma_minus8+8);

        update_ibc_mv_cand(sad, 0, y, sad_best_cand, mv_cand);
        tempSadBest = sad_best_cand[0];
        if (sad_best_cand[0] <= 3)
        {
            bestX = mv_cand[0][0];
            bestY = mv_cand[0][1];
            sad_best = sad_best_cand[0];
            best_mv_bits = mv_bits;
            mv[0] = bestX;
            mv[1] = bestY;
            rui_cost = sad_best;
            goto end;
        }
    }

    const int boundX = XEVE_MAX(srch_rng_hor_left, -cu_pel_x);
    for (int x = 0 - roi_width - pu_pel_offset_x; x >= boundX; --x)
    {
        if (!is_bv_valid(ctx, cu_pel_x, cu_pel_y, roi_width, roi_height, log2_cuw, log2_cuh, pic_width, pic_height, x, 0, lcu_width, core))
        {
            continue;
        }

        mv_bits = get_bv_cost_bits(x, 0);
        sad = GET_BV_COST(ctx, mv_bits);

        /* get sad */
        ref = rec + x;
        sad += xeve_sad_16b(log2_cuw, log2_cuh, org, ref, pi->s_o[Y_C], ref_pic->s_l, ctx->sps.bit_depth_luma_minus8 + 8);

        update_ibc_mv_cand(sad, x, 0, sad_best_cand, mv_cand);
        tempSadBest = sad_best_cand[0];
        if (sad_best_cand[0] <= 3)
        {
            bestX = mv_cand[0][0];
            bestY = mv_cand[0][1];
            sad_best = sad_best_cand[0];
            best_mv_bits = mv_bits;
            mv[0] = bestX;
            mv[1] = bestY;
            rui_cost = sad_best;
            goto end;
        }
    }

    bestX = mv_cand[0][0];
    bestY = mv_cand[0][1];
    sad_best = sad_best_cand[0];
    sad = GET_BV_COST(ctx, mv_bits);
    if ((!bestX && !bestY) || (sad_best - sad <= 32))
    {
#if ENABLE_IBC_CHROMA_REFINE
        //chroma refine
        best_cand_idx = refine_ibc_chroma_mv(ctx, core, pi, cu_x, cu_y, log2_cuw, log2_cuh, pic_width, pic_height, sad_best_cand, mv_cand);
#else
        best_cand_idx = 0;
#endif
        bestX = mv_cand[best_cand_idx][0];
        bestY = mv_cand[best_cand_idx][1];
        sad_best = sad_best_cand[best_cand_idx];
        mv[0] = bestX;
        mv[1] = bestY;
        rui_cost = sad_best;
        goto end;
    }

    if ((1 << log2_cuw) < 16 && (1 << log2_cuh) < 16)
    {
        for (int y = XEVE_MAX(srch_rng_ver_top, -cu_pel_y); y <= srch_rng_ver_bottom; y += 2)
        {
            if ((y == 0) || ((int)(cu_pel_y + y + roi_height) >= pic_height))
            {
                continue;
            }

            for (int x = XEVE_MAX(srch_rng_hor_left, -cu_pel_x); x <= srch_rng_hor_right; x++)
            {
                if ((x == 0) || ((int)(cu_pel_x + x + roi_width) >= pic_width))
                {
                    continue;
                }

                if (!is_bv_valid(ctx, cu_pel_x, cu_pel_y, roi_width, roi_height, log2_cuw, log2_cuh, pic_width, pic_height, x, y, lcu_width, core))
                {
                    continue;
                }

                mv_bits = get_bv_cost_bits(x, y);
                sad = GET_BV_COST(ctx, mv_bits);

                /* get sad */
                ref = rec + y * ref_pic->s_l + x;
                sad += xeve_sad_16b(log2_cuw, log2_cuh, org, ref, pi->s_o[Y_C], ref_pic->s_l, ctx->sps.bit_depth_luma_minus8 + 8);

                update_ibc_mv_cand(sad, x, y, sad_best_cand, mv_cand);
            }
        }

        bestX = mv_cand[0][0];
        bestY = mv_cand[0][1];
        sad_best = sad_best_cand[0];

        mv_bits = get_bv_cost_bits(bestX, bestY);
        sad = GET_BV_COST(ctx, mv_bits);

        if (sad_best - sad <= 16)
        {
#if ENABLE_IBC_CHROMA_REFINE
            //chroma refine
            best_cand_idx = refine_ibc_chroma_mv(ctx, core, pi, cu_x, cu_y, log2_cuw, log2_cuh, pic_width, pic_height, sad_best_cand, mv_cand);
#else
            best_cand_idx = 0;
#endif
            bestX = mv_cand[0][0];
            bestY = mv_cand[0][1];
            sad_best = sad_best_cand[best_cand_idx];
            best_mv_bits = mv_bits;
            mv[0] = bestX;
            mv[1] = bestY;
            rui_cost = sad_best;
            goto end;
        }

        for (int y = (XEVE_MAX(srch_rng_ver_top, -cu_pel_y) + 1); y <= srch_rng_ver_bottom; y += 2)
        {
            if ((y == 0) || ((int)(cu_pel_y + y + roi_height) >= pic_height))
            {
                continue;
            }

            for (int x = XEVE_MAX(srch_rng_hor_left, -cu_pel_x); x <= srch_rng_hor_right; x += 2)
            {
                if ((x == 0) || ((int)(cu_pel_x + x + roi_width) >= pic_width))
                {
                    continue;
                }

                if (!is_bv_valid(ctx, cu_pel_x, cu_pel_y, roi_width, roi_height, log2_cuw, log2_cuh, pic_width, pic_height, x, y, lcu_width, core))
                {
                    continue;
                }

                mv_bits = get_bv_cost_bits(x, y);
                sad = GET_BV_COST(ctx, mv_bits);

                /* get sad */
                ref = rec + y * ref_pic->s_l + x;
                sad += xeve_sad_16b(log2_cuw, log2_cuh, org, ref, pi->s_o[Y_C], ref_pic->s_l, ctx->sps.bit_depth_luma_minus8 + 8);

                update_ibc_mv_cand(sad, x, y, sad_best_cand, mv_cand);
                tempSadBest = sad_best_cand[0];
                if (sad_best_cand[0] <= 5)
                {
#if ENABLE_IBC_CHROMA_REFINE
                    //chroma refine & return
                    best_cand_idx = refine_ibc_chroma_mv(ctx, core, pi, cu_x, cu_y, log2_cuw, log2_cuh, pic_width, pic_height, sad_best_cand, mv_cand);
#else
                    best_cand_idx = 0;
#endif
                    bestX = mv_cand[best_cand_idx][0];
                    bestY = mv_cand[best_cand_idx][1];
                    sad_best = sad_best_cand[best_cand_idx];
                    mv[0] = bestX;
                    mv[1] = bestY;
                    rui_cost = sad_best;
                    goto end;
                }
            }
        }

        bestX = mv_cand[0][0];
        bestY = mv_cand[0][1];
        sad_best = sad_best_cand[0];

        mv_bits = get_bv_cost_bits(bestX, bestY);
        sad = GET_BV_COST(ctx, mv_bits);

        if ((sad_best >= tempSadBest) || ((sad_best - sad) <= 32))
        {
#if ENABLE_IBC_CHROMA_REFINE
            //chroma refine
            best_cand_idx = refine_ibc_chroma_mv(ctx, core, pi, cu_x, cu_y, log2_cuw, log2_cuh, pic_width, pic_height, sad_best_cand, mv_cand);
#else
            best_cand_idx = 0;
#endif
            bestX = mv_cand[best_cand_idx][0];
            bestY = mv_cand[best_cand_idx][1];
            sad_best = sad_best_cand[best_cand_idx];
            mv[0] = bestX;
            mv[1] = bestY;
            rui_cost = sad_best;
            goto end;
        }

        tempSadBest = sad_best_cand[0];

        for (int y = (XEVE_MAX(srch_rng_ver_top, -cu_pel_y) + 1); y <= srch_rng_ver_bottom; y += 2)
        {
            if ((y == 0) || ((int)(cu_pel_y + y + roi_height) >= pic_height))
            {
                continue;
            }

            for (int x = (XEVE_MAX(srch_rng_hor_left, -cu_pel_x) + 1); x <= srch_rng_hor_right; x += 2)
            {

                if ((x == 0) || ((int)(cu_pel_x + x + roi_width) >= pic_width))
                {
                    continue;
                }

                if (!is_bv_valid(ctx, cu_pel_x, cu_pel_y, roi_width, roi_height, log2_cuw, log2_cuh, pic_width, pic_height, x, y, lcu_width, core))
                {
                    continue;
                }

                mv_bits = get_bv_cost_bits(x, y);
                sad = GET_BV_COST(ctx, mv_bits);

                /* get sad */
                ref = rec + y * ref_pic->s_l + x;
                sad += xeve_sad_16b(log2_cuw, log2_cuh, org, ref, pi->s_o[Y_C], ref_pic->s_l, ctx->sps.bit_depth_luma_minus8 + 8);

                update_ibc_mv_cand(sad, x, y, sad_best_cand, mv_cand);
                tempSadBest = sad_best_cand[0];
                if (sad_best_cand[0] <= 5)
                {
#if ENABLE_IBC_CHROMA_REFINE
                    //chroma refine & return
                    best_cand_idx = refine_ibc_chroma_mv(ctx, core, pi, cu_x, cu_y, log2_cuw, log2_cuh, pic_width, pic_height, sad_best_cand, mv_cand);
#else
                    best_cand_idx = 0;
#endif
                    bestX = mv_cand[best_cand_idx][0];
                    bestY = mv_cand[best_cand_idx][1];
                    sad_best = sad_best_cand[best_cand_idx];
                    mv[0] = bestX;
                    mv[1] = bestY;
                    rui_cost = sad_best;
                    goto end;
                }
            }
        }
    }


#if ENABLE_IBC_CHROMA_REFINE
    //chroma refine
    best_cand_idx = refine_ibc_chroma_mv(ctx, core, pi, cu_x, cu_y, log2_cuw, log2_cuh, pic_width, pic_height, sad_best_cand, mv_cand);
#else
    best_cand_idx = 0;
#endif

    bestX = mv_cand[best_cand_idx][0];
    bestY = mv_cand[best_cand_idx][1];
    sad_best = sad_best_cand[best_cand_idx];
    mv[0] = bestX;
    mv[1] = bestY;
    rui_cost = sad_best;

end:
    return rui_cost;
}

static u32 pibc_me_search(XEVE_CTX *ctx, XEVE_CORE *core, XEVE_PIBC *pi, int x, int y, int log2_cuw, int log2_cuh,
    s16 mvp[MV_D], s16 mv[MV_D])
{
    XEVEM_CTX *mctx = (XEVEM_CTX *)ctx;
    u32 cost = 0;
    s16 mv_temp[MV_D] = { 0, 0 };
    if (ctx->param.ibc_hash_search_flag && xeve_check_luma(core->tree_cons) )
    {
        cost = xeve_ibc_hash_search(ctx, mctx->ibc_hash, x, y, log2_cuw, log2_cuh, mvp, mv_temp, core);
    }
    if (mv_temp[0] == 0 && mv_temp[1] == 0)
    {
        // if hash search does not work or is not enabled
        cost = pibc_search_estimation(ctx, core, pi, x, y, log2_cuw, log2_cuh, mvp, mv_temp);
    }

    mv[0] = mv_temp[0];
    mv[1] = mv_temp[1];

    if (mv_temp[0] == 0 && mv_temp[1] == 0)
    {
        return XEVE_UINT32_MAX;
    }

    return cost;
}

static double pibc_analyze_cu(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh
                            , XEVE_MODE *mi, s16 coef[N_C][MAX_CU_DIM], pel *rec[N_C], int s_rec[N_C])
{
    XEVEM_CTX  *mctx = (XEVEM_CTX *)ctx;
    XEVEM_CORE *mcore = (XEVEM_CORE *)core;
    XEVE_PIBC *pi;
    u32 mecost, best_mecost;
    s16(*mvp)[MV_D], *mv, *mvd;
    int cuw, cuh, i, j;
    u8 mvp_idx = 0;
    double cost, cost_best = MAX_COST;
    double cost_ibc;
    u8 found_available_ibc = 0;
    mcore->ats_inter_info = 0;

    int start_c = xeve_check_luma(core->tree_cons) ? Y_C : U_C;
    int end_c = xeve_check_chroma(core->tree_cons) ? N_C : U_C;
    end_c = ctx->sps.chroma_format_idc == 0 ? U_C : end_c;

    pi = &mctx->pibc[core->thread_cnt];

    cuw = (1 << log2_cuw);
    cuh = (1 << log2_cuh);

    mv = pi->mv[0];
    mvd = pi->mvd;

    best_mecost = XEVE_UINT32_MAX;

    mvp = pi->mvp;

    mvp_idx = 0;

    /* motion search ********************/
    mecost = pibc_me_search(ctx, core, pi, x, y, log2_cuw, log2_cuh, mvp[mvp_idx], mv);

    if (mv[MV_X] != 0 || mv[MV_Y] != 0)
    {
        found_available_ibc = 1;
        if (mecost < best_mecost)
        {
            best_mecost = mecost;
        }

        pi->mv[1][MV_X] = mv[MV_X];
        pi->mv[1][MV_Y] = mv[MV_Y];

        mvd[MV_X] = mv[MV_X];
        mvd[MV_Y] = mv[MV_Y];

        pi->mvp_idx = mvp_idx;

        pi->pred_mode = MODE_IBC;
        pi->ibc_flag = 1;

        cost = cost_ibc = pibc_residue_rdo(ctx, core, x, y, log2_cuw, log2_cuh, pi->pred, pi->coef, mvp_idx, pi->mv[1]);

        if (cost < cost_best)
        {
            pi->mvp_idx = mvp_idx;
            cost_ibc = cost_best = cost;

            for (j = start_c; j < end_c; j++)
            {
                int size_tmp = (cuw * cuh) >> (j == 0 ? 0 : (ctx->param.cs_w_shift + ctx->param.cs_h_shift));
                pi->nnz_best[j] = core->nnz[j];
            }
        }
    }

    if (found_available_ibc)
    {
        /* reconstruct */

        for (j = start_c; j < end_c; j++)
        {
            int size_tmp = (cuw * cuh) >> (j == 0 ? 0 : (ctx->param.cs_w_shift + ctx->param.cs_h_shift));
            xeve_mcpy(coef[j], pi->coef[j], sizeof(s16) * size_tmp);
        }

        for (i = start_c; i < end_c; i++)
        {
            rec[i] = pi->unfiltered_rec_buf[i];
            s_rec[i] = (i == 0 ? cuw : cuw >> ctx->param.cs_w_shift);
            core->nnz[i] = pi->nnz_best[i];
        }

        return cost_ibc;
    }
    else
    {
        return MAX_COST;
    }
}

static void init_log_lut(XEVE_PIBC *pi)
{
    int size = sizeof(s8) * (MAX_CU_SIZE + 1);
    xeve_mset(pi->ctu_log2_tbl, 0, size);
    int c = 0;
    for (int i = 0, n = 0; i <= MAX_CU_SIZE; i++)
    {
        if (i == (1 << n))
        {
            c = n;
            n++;
        }

        pi->ctu_log2_tbl[i] = c;
    }
}

static int pibc_init_tile(XEVE_CTX *ctx, int tile_idx)
{
    XEVEM_CTX * mctx = (XEVEM_CTX *)ctx;
    XEVE_PIBC * pi = &mctx->pibc[tile_idx];
    XEVE_PIC  * pic;
    int         size;

    pic = pi->pic_o = PIC_ORIG(ctx);
    pi->o[Y_C] = pic->y;
    pi->o[U_C] = pic->u;
    pi->o[V_C] = pic->v;

    pi->s_o[Y_C] = pic->s_l;
    pi->s_o[U_C] = pic->s_c;
    pi->s_o[V_C] = pic->s_c;

    pic = pi->pic_m = PIC_MODE(ctx);
    pi->m[Y_C] = pic->y;
    pi->m[U_C] = pic->u;
    pi->m[V_C] = pic->v;

    pi->s_m[Y_C] = pic->s_l;
    pi->s_m[U_C] = pic->s_c;
    pi->s_m[V_C] = pic->s_c;

    pi->slice_type = ctx->slice_type;

    pi->refi[0] = 0;
    pi->refi[1] = REFI_INVALID;

    pi->w_scu = ctx->w_scu;

    size = sizeof(pel) * N_C * MAX_CU_DIM;
    xeve_mset(pi->unfiltered_rec_buf, 0, size);

    size = sizeof(pel) * REFP_NUM * N_C * MAX_CU_DIM;
    xeve_mset(pi->pred, 0, size);

    /* MV predictor */
    size = sizeof(s16) * MAX_NUM_MVP * MV_D;
    xeve_mset(pi->mvp, 0, size);

    size = sizeof(s16) * MV_D;
    xeve_mset(pi->mv, 0, size);

    size = sizeof(s16) * MV_D;
    xeve_mset(pi->mvd, 0, size);

    init_log_lut(pi);

    return XEVE_OK;
}

void reset_ibc_search_range(XEVE_CTX *ctx, int cu_x, int cu_y, int log2_cuw, int log2_cuh, XEVE_CORE * core)
{
    XEVEM_CTX *mctx = (XEVEM_CTX *)ctx;
    int hashHitRatio = 0;
    mctx->pibc[core->thread_cnt].search_range_x = ctx->param.ibc_search_range_x;
    mctx->pibc[core->thread_cnt].search_range_y = ctx->param.ibc_search_range_y;
    hashHitRatio = xeve_ibc_hash_hit_ratio(ctx, mctx->ibc_hash, cu_x, cu_y, log2_cuw, log2_cuh); // in percent

    if (hashHitRatio < 5) // 5%
    {
        mctx->pibc[core->thread_cnt].search_range_x >>= 1;
        mctx->pibc[core->thread_cnt].search_range_y >>= 1;
    }
}

static int pibc_init_lcu(XEVE_CTX *ctx, XEVE_CORE *core)
{
    XEVE_PIBC *pi;
    XEVEM_CTX *mctx = (XEVEM_CTX *)ctx;
    pi = &mctx->pibc[core->thread_cnt];

    pi->lambda_mv = (u32)floor(65536.0 * core->sqrt_lambda[0]);
    pi->qp_y = core->qp_y;
    pi->qp_u = core->qp_u;
    pi->qp_v = core->qp_v;

    return XEVE_OK;
}

static int pibc_set_complexity(XEVE_CTX *ctx, int complexity)
{
    XEVE_PIBC *pi;
    XEVEM_CTX *mctx = (XEVEM_CTX *)ctx;

    for (int i = 0; i < ctx->param.threads; i++)
    {
        pi = &mctx->pibc[i];
        pi->search_range_x = ctx->param.ibc_search_range_x;
        pi->search_range_y = ctx->param.ibc_search_range_y;
        mctx->fn_pibc_analyze_cu = pibc_analyze_cu;
        pi->complexity = complexity;
    }

    return XEVE_OK;
}

int xevem_pibc_create(XEVE_CTX *ctx, int complexity)
{
    XEVE_PIBC * pi;
    XEVEM_CTX *mctx = (XEVEM_CTX *)ctx;

    /* set function addresses */
    mctx->fn_pibc_init_tile = pibc_init_tile;
    mctx->fn_pibc_init_lcu = pibc_init_lcu;
    mctx->fn_pibc_set_complexity = pibc_set_complexity;

    for (int i = 0; i < ctx->param.threads; i++)
    {
        pi = &mctx->pibc[i];
        pi->min_clip[MV_X] = -MAX_CU_SIZE + 1;
        pi->min_clip[MV_Y] = -MAX_CU_SIZE + 1;
        pi->max_clip[MV_X] = ctx->param.w - 1;
        pi->max_clip[MV_Y] = ctx->param.h - 1;
    }

    return mctx->fn_pibc_set_complexity(ctx, complexity);
}

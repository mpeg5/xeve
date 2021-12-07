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
#include <math.h>

int xeve_pintra_init_mt(XEVE_CTX * ctx, int tile_idx)
{
    XEVE_PINTRA * pi;
    XEVE_PIC    * pic;

    pi = &ctx->pintra[tile_idx];

    pic          = pi->pic_o = PIC_ORIG(ctx);
    pi->o[Y_C]   = pic->y;
    pi->o[U_C]   = pic->u;
    pi->o[V_C]   = pic->v;

    pi->s_o[Y_C] = pic->s_l;
    pi->s_o[U_C] = pic->s_c;
    pi->s_o[V_C] = pic->s_c;

    pic          = pi->pic_m = PIC_MODE(ctx);
    pi->m[Y_C]   = pic->y;
    pi->m[U_C]   = pic->u;
    pi->m[V_C]   = pic->v;

    pi->s_m[Y_C] = pic->s_l;
    pi->s_m[U_C] = pic->s_c;
    pi->s_m[V_C] = pic->s_c;

    pi->slice_type = ctx->slice_type;

    return XEVE_OK;
}

int xeve_pintra_analyze_lcu(XEVE_CTX * ctx, XEVE_CORE * core)
{
    return XEVE_OK;
}

static double pintra_residue_rdo(XEVE_CTX *ctx, XEVE_CORE *core, pel *org_luma, pel *org_cb, pel *org_cr, int s_org, int s_org_c, int log2_cuw
                               , int log2_cuh, s16 coef[N_C][MAX_CU_DIM], s32 *dist, int mode, int x, int y)
{
    XEVE_PINTRA *pi = &ctx->pintra[core->thread_cnt];
    int cuw, cuh, bit_cnt;
    double cost = 0;
    int tmp_cbf_l =0;
    int tmp_cbf_sub_l[MAX_SUB_TB_NUM] = {0,};
    int w_shift = ctx->param.cs_w_shift;
    int h_shift = ctx->param.cs_h_shift;

    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;

    if(mode == 0)
    {
        pel * pred = 0;

        pred = pi->pred_cache[core->ipm[0]];

        xeve_diff_16b(log2_cuw, log2_cuh, org_luma, pred, s_org, cuw, cuw, pi->coef_tmp[Y_C], ctx->sps.bit_depth_luma_minus8+8);

        ctx->fn_tq(ctx, core, pi->coef_tmp, log2_cuw, log2_cuh, pi->slice_type, core->nnz, 1, RUN_L);

        xeve_mcpy(coef[Y_C], pi->coef_tmp[Y_C], sizeof(u16) * (cuw * cuh));

        SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
        DQP_LOAD(core->dqp_temp_run, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);
        xeve_sbac_bit_reset(&core->s_temp_run);
        xeve_rdo_bit_cnt_cu_intra_luma(ctx, core, ctx->sh->slice_type, core->scup, pi->coef_tmp);
        bit_cnt = xeve_get_bit_number(&core->s_temp_run);

        ctx->fn_itdp(ctx, core, pi->coef_tmp, core->nnz_sub);

        ctx->fn_recon(ctx, core, pi->coef_tmp[Y_C], pred, core->nnz[Y_C], cuw, cuh, cuw, pi->rec[Y_C], ctx->sps.bit_depth_luma_minus8 + 8);

        cost += xeve_ssd_16b(log2_cuw, log2_cuh, pi->rec[Y_C], org_luma, cuw, s_org, ctx->sps.bit_depth_luma_minus8+8);

        if(ctx->param.rdo_dbk_switch)
        {
            calc_delta_dist_filter_boundary(ctx, PIC_MODE(ctx), PIC_ORIG(ctx), cuw, cuh, pi->rec, cuw, x, y, core->avail_lr, 1, core->nnz[Y_C] != 0, NULL, NULL, 0, core);
            cost += core->delta_dist[Y_C];
        }
        *dist = (s32)cost;
        cost += RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt);
    }
    else
    {
        xeve_ipred_uv(core->nb[1][0] + 2, core->nb[1][1] + (cuh >> h_shift), core->nb[1][2] + 2, core->avail_lr, pi->pred[U_C], core->ipm[1], core->ipm[0], cuw >> w_shift, cuh >> h_shift);
        xeve_ipred_uv(core->nb[2][0] + 2, core->nb[2][1] + (cuh >> h_shift), core->nb[2][2] + 2, core->avail_lr, pi->pred[V_C], core->ipm[1], core->ipm[0], cuw >> w_shift, cuh >> h_shift);

        xeve_diff_16b(log2_cuw - w_shift, log2_cuh - h_shift, org_cb, pi->pred[U_C], s_org_c, cuw >> w_shift, cuw >> w_shift, pi->coef_tmp[U_C], ctx->sps.bit_depth_chroma_minus8+8);
        xeve_diff_16b(log2_cuw - w_shift, log2_cuh - h_shift, org_cr, pi->pred[V_C], s_org_c, cuw >> w_shift, cuw >> w_shift, pi->coef_tmp[V_C], ctx->sps.bit_depth_chroma_minus8 + 8);

        ctx->fn_tq(ctx, core, pi->coef_tmp, log2_cuw, log2_cuh, pi->slice_type, core->nnz, 1, RUN_CB | RUN_CR);

        xeve_mcpy(coef[U_C], pi->coef_tmp[U_C], sizeof(u16) * (cuw * cuh) >> (w_shift + h_shift));
        xeve_mcpy(coef[V_C], pi->coef_tmp[V_C], sizeof(u16) * (cuw * cuh) >> (w_shift + h_shift));

        ctx->fn_itdp(ctx, core, pi->coef_tmp, core->nnz_sub);

        core->nnz[Y_C] = tmp_cbf_l;
        xeve_mcpy(core->nnz_sub[Y_C], tmp_cbf_sub_l, sizeof(int) * MAX_SUB_TB_NUM);

        ctx->fn_recon(ctx, core, pi->coef_tmp[U_C], pi->pred[U_C], core->nnz[U_C], cuw >> w_shift, cuh >> h_shift, cuw >> w_shift, pi->rec[U_C], ctx->sps.bit_depth_luma_minus8 + 8);
        ctx->fn_recon(ctx, core, pi->coef_tmp[V_C], pi->pred[V_C], core->nnz[V_C], cuw >> w_shift, cuh >> h_shift, cuw >> w_shift, pi->rec[V_C], ctx->sps.bit_depth_luma_minus8 + 8);

        xeve_sbac_bit_reset(&core->s_temp_run);
        xeve_rdo_bit_cnt_cu_intra_chroma(ctx, core, ctx->sh->slice_type, core->scup, coef);
        bit_cnt = xeve_get_bit_number(&core->s_temp_run);

        if(ctx->sps.chroma_format_idc)
        {
            cost += core->dist_chroma_weight[0] * xeve_ssd_16b(log2_cuw - w_shift, log2_cuh - h_shift, pi->rec[U_C], org_cb, cuw >> w_shift, s_org_c, ctx->sps.bit_depth_chroma_minus8 + 8);
            cost += core->dist_chroma_weight[1] * xeve_ssd_16b(log2_cuw - w_shift, log2_cuh - h_shift, pi->rec[V_C], org_cr, cuw >> w_shift, s_org_c, ctx->sps.bit_depth_chroma_minus8 + 8);
        }

        if(ctx->param.rdo_dbk_switch)
        {
            calc_delta_dist_filter_boundary(ctx, PIC_MODE(ctx), PIC_ORIG(ctx), cuw, cuh, pi->rec, cuw, x, y, core->avail_lr, 1,
                                            core->nnz[Y_C] != 0, NULL, NULL, 0, core);
            cost += (core->delta_dist[U_C] * core->dist_chroma_weight[0]) + (core->delta_dist[V_C] * core->dist_chroma_weight[1]);
        }
        *dist = (s32)cost;

        cost += xeve_ssd_16b(log2_cuw, log2_cuh, pi->rec[Y_C], org_luma, cuw, s_org, ctx->sps.bit_depth_luma_minus8+8);
        cost += RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt);
    }

    return cost;
}

static void pintra_ipred(XEVE_CTX * ctx, XEVE_CORE * core, pel * pred_buf,  int ipm, int cuw, int cuh)
{
    xeve_ipred(core->nb[0][0] + 2, core->nb[0][1] + cuh, core->nb[0][2] + 2, core->avail_lr, pred_buf, ipm, cuw, cuh);
}

static double make_ipred_list_simple(XEVE_CTX * ctx, XEVE_CORE * core, int log2_cuw, int log2_cuh, pel * org, int s_org)
{
    XEVE_PINTRA *pi = &ctx->pintra[core->thread_cnt];
    int cuw, cuh, pred_cnt, i;
    double min_cost, cost;

    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;

    min_cost = MAX_COST;
    pred_cnt = IPD_CNT_B;

    for (i = 0; i < pred_cnt; i++)
    {
        pel * pred_buf = NULL;

        pred_buf = pi->pred_cache[i];

        pintra_ipred(ctx, core, pred_buf, i, cuw, cuh);

        cost = xeve_satd_16b(log2_cuw, log2_cuh, org, pred_buf, s_org, cuw, ctx->sps.bit_depth_luma_minus8+8);

        if (cost < min_cost)
        {
            min_cost = cost;

        }

    }


    return min_cost;
}

static int make_ipred_list(XEVE_CTX * ctx, XEVE_CORE * core, int log2_cuw, int log2_cuh, pel * org, int s_org, int * ipred_list)
{
    XEVE_PINTRA *pi = &ctx->pintra[core->thread_cnt];
    int cuw, cuh, pred_cnt, i, j;
    double cost, cand_cost[IPD_RDO_CNT];
    u32 cand_satd_cost[IPD_RDO_CNT];
    u32 cost_satd;
    const int ipd_rdo_cnt = XEVE_ABS(log2_cuw - log2_cuh) >= 2 ? IPD_RDO_CNT - 1 : IPD_RDO_CNT;

    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;

    for(i = 0; i < ipd_rdo_cnt; i++)
    {
        ipred_list[i] = IPD_DC;
        cand_cost[i] = MAX_COST;
        cand_satd_cost[i] = XEVE_UINT32_MAX;
    }

    pred_cnt = IPD_CNT_B;

    for (i = 0; i < pred_cnt; i++)
    {
        int bit_cnt, shift = 0;
        pel * pred_buf = NULL;

        pred_buf = pi->pred_cache[i];

        pintra_ipred(ctx, core, pred_buf, i, cuw, cuh);

        cost_satd = xeve_satd_16b(log2_cuw, log2_cuh, org, pred_buf, s_org, cuw, ctx->sps.bit_depth_luma_minus8+8);
        cost = (double)cost_satd;
        SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
        xeve_sbac_bit_reset(&core->s_temp_run);

        ctx->fn_mode_rdo_bit_cnt_intra_dir(ctx, core, i);

        bit_cnt = xeve_get_bit_number(&core->s_temp_run);
        cost += RATE_TO_COST_SQRT_LAMBDA(core->sqrt_lambda[0], bit_cnt);

        while(shift < ipd_rdo_cnt && cost < cand_cost[ipd_rdo_cnt - 1 - shift])
        {
            shift++;
        }

        if(shift != 0)
        {
            for(j = 1; j < shift; j++)
            {
                ipred_list[ipd_rdo_cnt - j] = ipred_list[ipd_rdo_cnt - 1 - j];
                cand_cost[ipd_rdo_cnt - j] = cand_cost[ipd_rdo_cnt - 1 - j];
                cand_satd_cost[ipd_rdo_cnt - j] = cand_satd_cost[ipd_rdo_cnt - 1 - j];
            }
            ipred_list[ipd_rdo_cnt - shift] = i;
            cand_cost[ipd_rdo_cnt - shift] = cost;
            cand_satd_cost[ipd_rdo_cnt - shift] = cost_satd;
        }
    }

    pred_cnt = ipd_rdo_cnt;
    for(i = ipd_rdo_cnt - 1; i >= 1; i--)
    {
        if(cand_satd_cost[i] > core->inter_satd * (1.2))
        {
            pred_cnt--;
        }
        else
        {
            break;
        }
    }

    return XEVE_MIN(pred_cnt, ipd_rdo_cnt);
}

static void pintra_get_mpm(XEVE_CTX *ctx, XEVE_CORE * core, int cuw, int cuh)
{
    xeve_get_mpm(core->x_scu, core->y_scu, cuw, cuh, ctx->map_scu, ctx->map_ipm, core->scup, ctx->w_scu, &core->mpm_b_list, ctx->map_tidx);
}

static void pintra_get_nbr(XEVE_CTX *ctx, XEVE_CORE * core, int x, int y, int cuw, int cuh)
{
    XEVE_PINTRA *pi = &ctx->pintra[core->thread_cnt];

    pel *mod;
    pel *mod_cb, *mod_cr;
    int  s_mod, s_mod_c;
    int w_shift = ctx->param.cs_w_shift;
    int h_shift = ctx->param.cs_h_shift;

    /* prediction */
    s_mod = pi->s_m[Y_C];
    mod = pi->m[Y_C] + (y * s_mod) + x;

    s_mod_c = pi->s_m[U_C];
    mod_cb = pi->m[U_C] + ((y >> h_shift) * s_mod_c) + (x >> w_shift);
    mod_cr = pi->m[V_C] + ((y >> h_shift) * s_mod_c) + (x >> w_shift);

    xeve_get_nbr(x, y, cuw, cuh, mod, s_mod, core->avail_cu, core->nb, core->scup, ctx->map_scu, ctx->w_scu, ctx->h_scu, Y_C, ctx->pps.constrained_intra_pred_flag, ctx->map_tidx, ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.chroma_format_idc);
    if(ctx->sps.chroma_format_idc)
    {
        xeve_get_nbr(x >> w_shift, y >> h_shift, cuw >> w_shift, cuh >> h_shift, mod_cb, s_mod_c, core->avail_cu, core->nb, core->scup, ctx->map_scu, ctx->w_scu, ctx->h_scu, U_C, ctx->pps.constrained_intra_pred_flag, ctx->map_tidx, ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.chroma_format_idc);
        xeve_get_nbr(x >> w_shift, y >> h_shift, cuw >> w_shift, cuh >> h_shift, mod_cr, s_mod_c, core->avail_cu, core->nb, core->scup, ctx->map_scu, ctx->w_scu, ctx->h_scu, V_C, ctx->pps.constrained_intra_pred_flag, ctx->map_tidx, ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.chroma_format_idc);
    }
}


static void pintra_get_nbr_simple(XEVE_CTX *ctx, XEVE_CORE * core, int x, int y, int cuw, int cuh)
{
    XEVE_PINTRA *pi = &ctx->pintra[core->thread_cnt];

    pel *mod;
    int  s_mod;

    s_mod = ctx->pico->spic->s_l;
    mod = ctx->pico->spic->y + (y * s_mod) + x;

    xeve_get_nbr(x, y, cuw, cuh, mod, s_mod, core->avail_cu, core->nb, core->scup, ctx->map_scu, ctx->w_scu, ctx->h_scu, Y_C, ctx->pps.constrained_intra_pred_flag, ctx->map_tidx, ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.chroma_format_idc);
}

double xeve_pintra_analyze_cu_simple(XEVE_CTX* ctx, XEVE_CORE* core, int x, int y, int log2_cuw, int log2_cuh, s16 coef[N_C][MAX_CU_DIM])
{

    XEVE_PINTRA* pi = &ctx->pintra[0];
    int s_org,  s_mod, cuw, cuh;
    int best_ipd = IPD_INVALID;
    int best_ipd_c = IPD_INVALID;
    s32 best_dist_y = 0, best_dist_c = 0;
    int ipm_l2c = 0;
    int chk_bypass = 0;
    int bit_cnt = 0;
    int pred_cnt = IPD_CNT_B;;
    pel* org, * mod;
    double cost_t, cost = MAX_COST;
    int sec_best_ipd = IPD_INVALID;


    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;

    /* Y */
    xeve_assert(x + cuw <= ctx->w);
    xeve_assert(y + cuh <= ctx->h);

    core->cuw = cuw;
    core->cuh = cuh;
    core->log2_cuw = XEVE_LOG2(cuw);
    core->log2_cuh = XEVE_LOG2(cuh);
    core->x_scu = PEL2SCU(x);
    core->y_scu = PEL2SCU(y);
    core->scup = ((u32)core->y_scu * ctx->w_scu) + core->x_scu;

    /* prediction */
    s_mod = pi->s_m[Y_C];
    mod = pi->m[Y_C] + (y * s_mod) + x;

    s_org = pi->s_o[Y_C];
    org = pi->o[Y_C] + (y * s_org) + x;



    pintra_get_nbr_simple(ctx, core, x, y, cuw, cuh);
    pintra_get_mpm(ctx, core, cuw, cuh);

    cost_t = make_ipred_list_simple(ctx, core, log2_cuw, log2_cuh, org, s_org);

    return cost_t;

}

static double pintra_analyze_cu(XEVE_CTX* ctx, XEVE_CORE* core, int x, int y, int log2_cuw, int log2_cuh, XEVE_MODE* mi, s16 coef[N_C][MAX_CU_DIM], pel* rec[N_C], int s_rec[N_C])
{
    XEVE_PINTRA *pi = &ctx->pintra[core->thread_cnt];
    int i, j, s_org, s_org_c, s_mod, s_mod_c, cuw, cuh;
    int best_ipd = IPD_INVALID;
    int best_ipd_c = IPD_INVALID;
    s32 best_dist_y = 0, best_dist_c = 0;
    int ipm_l2c = 0;
    int chk_bypass = 0;
    int bit_cnt = 0;
    int ipred_list[IPD_CNT];
    int pred_cnt = IPD_CNT_B;;
    pel* org, * mod;
    pel* org_cb, * org_cr;
    pel* mod_cb, * mod_cr;
    double cost_t, cost = MAX_COST;
    int sec_best_ipd = IPD_INVALID;
    int w_shift = ctx->param.cs_w_shift;
    int h_shift = ctx->param.cs_h_shift;

    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;

    /* Y */
    xeve_assert(x + cuw <= ctx->w);
    xeve_assert(y + cuh <= ctx->h);

    /* prediction */
    s_mod = pi->s_m[Y_C];
    mod = pi->m[Y_C] + (y * s_mod) + x;

    s_org = pi->s_o[Y_C];
    org = pi->o[Y_C] + (y * s_org) + x;

    s_mod_c = pi->s_m[U_C];
    mod_cb = pi->m[U_C] + ((y >> h_shift) * s_mod_c) + (x >> w_shift);
    mod_cr = pi->m[V_C] + ((y >> h_shift) * s_mod_c) + (x >> w_shift);

    s_org_c = pi->s_o[U_C];
    org_cb = pi->o[U_C] + ((y >> h_shift) * s_org_c) + (x >> w_shift);
    org_cr = pi->o[V_C] + ((y >> h_shift) * s_org_c) + (x >> w_shift);

    pintra_get_nbr(ctx, core, x, y, cuw, cuh);
    pintra_get_mpm(ctx, core, cuw, cuh);

    pred_cnt = make_ipred_list(ctx, core, log2_cuw, log2_cuh, org, s_org, ipred_list);
    if(pred_cnt == 0)
    {
        return MAX_COST;
    }

    for(j = 0; j < pred_cnt; j++) /* Y */
    {
        s32 dist_t = 0;
        s32 dist_tc = 0;

        i = ipred_list[j];
        core->ipm[0] = i;

        core->ipm[1] = IPD_INVALID;
        cost_t = pintra_residue_rdo(ctx, core, org, org_cb, org_cr, s_org, s_org_c, log2_cuw, log2_cuh, coef, &dist_t, 0, x, y);
#if TRACE_COSTS
        XEVE_TRACE_COUNTER;
        XEVE_TRACE_STR("Luma mode ");
        XEVE_TRACE_INT(i);
        XEVE_TRACE_STR(" cost is ");
        XEVE_TRACE_DOUBLE(cost_t);
        XEVE_TRACE_STR("\n");
#endif
        if(cost_t < cost)
        {
            cost = cost_t;
            best_dist_y = dist_t;
            if(sec_best_ipd != best_ipd)
            {
                sec_best_ipd = best_ipd;
            }
            best_ipd = i;
            xeve_mcpy(pi->coef_best[Y_C], coef[Y_C], (cuw * cuh) * sizeof(s16));
            xeve_mcpy(pi->rec_best[Y_C], pi->rec[Y_C], (cuw * cuh) * sizeof(pel));

            pi->nnz_best[Y_C] = core->nnz[Y_C];
            xeve_mcpy(pi->nnz_sub_best[Y_C], core->nnz_sub[Y_C], sizeof(int) * MAX_SUB_TB_NUM);
            SBAC_STORE(core->s_temp_prev_comp_best, core->s_temp_run);
        }
    }

    s32 dist_tc = 0;
    core->ipm[0] = best_ipd;
    core->ipm[1] = best_ipd;
    cost_t = MAX_COST;
    if(ctx->sps.chroma_format_idc)
    {
        cost_t = pintra_residue_rdo(ctx, core, org, org_cb, org_cr, s_org, s_org_c, log2_cuw, log2_cuh, coef, &dist_tc, 1, x, y);

        best_ipd_c = core->ipm[1];
        best_dist_c = dist_tc;
        for(j = U_C; j < N_C; j++)
        {
            int size_tmp = (cuw * cuh) >> (j == 0 ? 0 : (w_shift + h_shift));
            xeve_mcpy(pi->coef_best[j], coef[j], size_tmp * sizeof(s16));
            xeve_mcpy(pi->rec_best[j], pi->rec[j], size_tmp * sizeof(pel));

            pi->nnz_best[j] = core->nnz[j];
            xeve_mcpy(pi->nnz_sub_best[j], core->nnz_sub[j], sizeof(int) * MAX_SUB_TB_NUM);
        }
    }

    for (j = Y_C; j < N_C; j++)
    {
        if(!ctx->sps.chroma_format_idc && j != Y_C)
            continue;
        int size_tmp = (cuw * cuh) >> (j == 0 ? 0 : (w_shift + h_shift));
        xeve_mcpy(coef[j], pi->coef_best[j], size_tmp * sizeof(u16));
        xeve_mcpy(pi->rec[j], pi->rec_best[j], size_tmp * sizeof(pel));
        core->nnz[j] = pi->nnz_best[j];
        xeve_mcpy(core->nnz_sub[j], pi->nnz_sub_best[j], sizeof(int) * MAX_SUB_TB_NUM);
        rec[j] = pi->rec[j];
        s_rec[j] = cuw >> (j == 0 ? 0 : w_shift);
    }

    core->ipm[0] = best_ipd;
    if(ctx->sps.chroma_format_idc)
    {
        core->ipm[1] = best_ipd_c;
        xeve_assert(best_ipd_c != IPD_INVALID);
    }

    /* cost calculation */
    SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
    DQP_STORE(core->dqp_temp_run, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);

    xeve_sbac_bit_reset(&core->s_temp_run);
    xeve_rdo_bit_cnt_cu_intra(ctx, core, ctx->sh->slice_type, core->scup, coef);

    bit_cnt = xeve_get_bit_number(&core->s_temp_run);
    cost = RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt);

    core->dist_cu = 0;
    cost += best_dist_y;
    core->dist_cu += best_dist_y;
    if(ctx->sps.chroma_format_idc)
    {
        cost += best_dist_c;
        core->dist_cu += best_dist_c;
    }
    SBAC_STORE(core->s_temp_best, core->s_temp_run);
    DQP_STORE(core->dqp_temp_best, core->dqp_temp_run);

    return cost;
}

int xeve_pintra_set_complexity(XEVE_CTX * ctx, int complexity)
{
    XEVE_PINTRA * pi;

   
    for (int i = 0; i < ctx->param.threads; i++)
    {
        pi = &ctx->pintra[i];
        pi->complexity = complexity;
    }
    return XEVE_OK;
}

int xeve_pintra_create(XEVE_CTX * ctx, int complexity)
{
    /* set function addresses */
    ctx->fn_pintra_set_complexity = xeve_pintra_set_complexity;
    ctx->fn_pintra_init_mt = xeve_pintra_init_mt;
    ctx->fn_pintra_init_lcu = xeve_pintra_analyze_lcu;
    ctx->fn_pintra_analyze_cu = pintra_analyze_cu;

    return ctx->fn_pintra_set_complexity(ctx, complexity);
}

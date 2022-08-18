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

#include "xeve_type.h"
#include "xevem_type.h"
#include "xevem_recon.h"
#include "xevem_mc.h"
#if x86_SSE
#include "xevem_mc_sse.h"
#endif
#include <math.h>


/* Define the Search Range for int-pel */
#define SEARCH_RANGE_IPEL_RA               384
#define SEARCH_RANGE_IPEL_LD               64
/* Define the Search Range for sub-pel ME */
#define SEARCH_RANGE_SPEL                  3

#define MV_COST(pi, mv_bits) (u32)(((pi)->lambda_mv * mv_bits + (1 << 15)) >> 16)
#define SWAP(a, b, t) { (t) = (a); (a) = (b); (b) = (t); }

/* q-pel search pattern */
static s8 tbl_search_pattern_qpel_8point[8][2] =
{
    {-1,  0}, { 0,  1}, { 1,  0}, { 0, -1},
    {-1,  1}, { 1,  1}, {-1, -1}, { 1, -1}
};

static const s8 tbl_diapos_partial[2][16][2] =
{
    {
        {-2, 0}, {-1, 1}, {0, 2}, {1, 1}, {2, 0}, {1, -1}, {0, -2}, {-1, -1}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}, {0, 0}
    },
    {
        {-4, 0}, {-3, 1}, {-2, 2}, {-1, 3}, {0, 4}, {1, 3}, {2, 2}, {3, 1}, {4, 0}, {3, -1}, {2, -2}, {1, -3}, {0, -4}, {-1, -3}, {-2, -2}, {-3, -1}
    }
};

static s8 tbl_search_pattern_hpel_partial[8][2] =
{
    {-2, 0}, {-2, 2}, {0, 2}, {2, 2}, {2, 0}, {2, -2}, {0, -2}, {-2, -2}
};


static int pinter_init_mt(XEVE_CTX *ctx, int tile_idx)
{
    XEVE_PINTER * pi = &ctx->pinter[tile_idx];
    XEVE_PIC    * pic;
    int           size;

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

    pi->refp = ctx->refp;
    pi->slice_type = ctx->slice_type;

    pi->map_mv = ctx->map_mv;
    pi->map_unrefined_mv = ctx->map_unrefined_mv;

    pi->w_scu = ctx->w_scu;

    size = sizeof(pel) * MAX_CU_DIM;
    xeve_mset(pi->pred_buf, 0, size);

    size = sizeof(s8) * PRED_NUM * REFP_NUM;
    xeve_mset(pi->refi, 0, size);

    size = sizeof(s8) * REFP_NUM * MAX_NUM_MVP;
    xeve_mset(pi->refi_pred, 0, size);

    size = sizeof(s8) * REFP_NUM * MAX_NUM_MVP;
    xeve_mset(pi->mvp_idx, 0, size);

    size = sizeof(s16) * REFP_NUM * XEVE_MAX_NUM_ACTIVE_REF_FRAME * MAX_NUM_MVP * MV_D;
    xeve_mset(pi->mvp_scale, 0, size);

    size = sizeof(s16) * REFP_NUM * XEVE_MAX_NUM_ACTIVE_REF_FRAME * MV_D;
    xeve_mset(pi->mv_scale, 0, size);

    size = sizeof(u8) * PRED_NUM * REFP_NUM * XEVE_MAX_NUM_ACTIVE_REF_FRAME;
    xeve_mset(pi->mvp_idx_temp_for_bi, 0, size);

    size = sizeof(int) * PRED_NUM * 4;
    xeve_mset(pi->best_index, 0, size);

    size = sizeof(s16) * PRED_NUM;
    xeve_mset(pi->mmvd_idx, 0, size);

    size = sizeof(s8) * PRED_NUM;
    xeve_mset(pi->mvr_idx, 0, size);

    size = sizeof(int) * MV_D;
    xeve_mset(pi->max_imv, 0, size);

    size = sizeof(s8) * PRED_NUM * REFP_NUM;
    xeve_mset(pi->first_refi, 0, size);

    size = sizeof(u8) * PRED_NUM;
    xeve_mset(pi->bi_idx, 0, size);

    size = sizeof(s16) * REFP_NUM * XEVE_MAX_NUM_ACTIVE_REF_FRAME * MAX_NUM_MVP * VER_NUM * MV_D;
    xeve_mset(pi->affine_mvp_scale, 0, size);

    size = sizeof(s16) * REFP_NUM * XEVE_MAX_NUM_ACTIVE_REF_FRAME * VER_NUM * MV_D;
    xeve_mset(pi->affine_mv_scale, 0, size);

    size = sizeof(u8) * REFP_NUM * XEVE_MAX_NUM_ACTIVE_REF_FRAME;
    xeve_mset(pi->mvp_idx_scale, 0, size);

    size = sizeof(u8) * REFP_NUM * MAX_NUM_MVP * VER_NUM * MV_D;
    xeve_mset(pi->affine_mvp, 0, size);

    size = sizeof(s16) * PRED_NUM * REFP_NUM * VER_NUM * MV_D;
    xeve_mset(pi->affine_mv, 0, size);

    size = sizeof(s16) * PRED_NUM * REFP_NUM * VER_NUM * MV_D;
    xeve_mset(pi->affine_mvd, 0, size);

    size = sizeof(pel) * MAX_CU_DIM;
    xeve_mset(pi->p_error, 0, size);

    size = sizeof(int) * 2 * MAX_CU_DIM;
    xeve_mset(pi->i_gradient, 0, size);

    size = sizeof(s16) * N_C * MAX_CU_DIM;
    xeve_mset(pi->resi, 0, size);

    size = sizeof(s16) * N_C * MAX_CU_DIM;
    xeve_mset(pi->coff_save, 0, size);

    size = sizeof(u8) * PRED_NUM;
    xeve_mset(pi->ats_inter_info_mode, 0, size);

    /* MV predictor */
    size = sizeof(s16) * REFP_NUM * MAX_NUM_MVP * MV_D;
    xeve_mset(pi->mvp, 0, size);

    size = sizeof(s16) * PRED_NUM * REFP_NUM * MV_D;
    xeve_mset(pi->mv, 0, size);

    size = sizeof(s16) * MAX_CU_CNT_IN_LCU * PRED_NUM * REFP_NUM * MV_D;
    xeve_mset(pi->dmvr_mv, 0, size);

    size = sizeof(s16) * PRED_NUM * REFP_NUM * MV_D;
    xeve_mset(pi->mvd, 0, size);

    size = sizeof(s16) * MAX_CU_DIM;
    xeve_mset(pi->org_bi, 0, size);

    size = sizeof(s32) * REFP_NUM;
    xeve_mset(pi->mot_bits, 0, size);

    size = sizeof(pel) * (PRED_NUM + 1) * 2 * N_C * MAX_CU_DIM;
    xeve_mset(pi->pred, 0, size);

    size = sizeof(pel) * MAX_CU_DIM;
    xeve_mset(pi->dmvr_template, 0, size);

    size = sizeof(pel) * REFP_NUM * (MAX_CU_SIZE + ((DMVR_NEW_VERSION_ITER_COUNT + 1) * REF_PRED_EXTENTION_PEL_COUNT)) *
        (MAX_CU_SIZE + ((DMVR_NEW_VERSION_ITER_COUNT + 1) * REF_PRED_EXTENTION_PEL_COUNT));
    xeve_mset(pi->dmvr_ref_pred_interpolated, 0, size);

    return XEVE_OK;
}

/* Motion Estimation */
__inline static u32 get_exp_golomb_bits(u32 abs_mvd)
{
    int bits = 0;
    int len_i, len_c, nn;

    /* abs(mvd) */
    nn = ((abs_mvd + 1) >> 1);
    for(len_i = 0; len_i < 16 && nn != 0; len_i++)
    {
        nn >>= 1;
    }
    len_c = (len_i << 1) + 1;

    bits += len_c;

    /* sign */
    if(abs_mvd)
    {
        bits++;
    }

    return bits;
}

static int get_mv_bits(int mvd_x, int mvd_y, int num_refp, int refi, u8 mvr_idx, int sps_amvr_flag)
{
    int bits = 0;
    bits = ((mvd_x >> mvr_idx) > 2048 || (mvd_x >> mvr_idx) <= -2048) ? get_exp_golomb_bits(XEVE_ABS(mvd_x) >> mvr_idx) : xeve_tbl_mv_bits[mvd_x >> mvr_idx];
    bits += ((mvd_y >> mvr_idx) > 2048 || (mvd_y >> mvr_idx) <= -2048) ? get_exp_golomb_bits(XEVE_ABS(mvd_y) >> mvr_idx) : xeve_tbl_mv_bits[mvd_y >> mvr_idx];
    bits += xeve_tbl_refi_bits[num_refp][refi];
    if(sps_amvr_flag)
    {
        bits += mvr_idx + 1;
    }
    return bits;
}

static void get_range_ipel(XEVE_PINTER * pi, s16 mvc[MV_D], s16 range[MV_RANGE_DIM][MV_D], int bi, int ri, int lidx)
{
    if(pi->sps_amvr_flag)
    {
        int offset = pi->gop_size >> 1;
        int max_qpel_sr = pi->max_search_range >> 3;
        int max_hpel_sr = pi->max_search_range >> 2;
        int max_ipel_sr = pi->max_search_range >> 1;
        int max_spel_sr = pi->max_search_range;
        int max_search_range = XEVE_CLIP3(pi->max_search_range >> 2, pi->max_search_range, (pi->max_search_range * XEVE_ABS(pi->poc - (int)pi->refp[ri][lidx].poc) + offset) / pi->gop_size);
        int offset_x, offset_y, rangexy;
        int range_offset = 3 * (1 << (pi->curr_mvr - 1));

        if(pi->curr_mvr == 0)
        {
            rangexy = XEVE_CLIP3(max_qpel_sr >> 2, max_qpel_sr, (max_qpel_sr * XEVE_ABS(pi->poc - (int)pi->refp[ri][lidx].poc) + offset) / pi->gop_size);
        }
        else if(pi->curr_mvr == 1)
        {
            rangexy = XEVE_CLIP3(max_hpel_sr >> 2, max_hpel_sr, (max_hpel_sr * XEVE_ABS(pi->poc - (int)pi->refp[ri][lidx].poc) + offset) / pi->gop_size);
        }
        else if(pi->curr_mvr == 2)
        {
            rangexy = XEVE_CLIP3(max_ipel_sr >> 2, max_ipel_sr, (max_ipel_sr * XEVE_ABS(pi->poc - (int)pi->refp[ri][lidx].poc) + offset) / pi->gop_size);
        }
        else
        {
            rangexy = XEVE_CLIP3(max_spel_sr >> 2, max_spel_sr, (max_spel_sr * XEVE_ABS(pi->poc - (int)pi->refp[ri][lidx].poc) + offset) / pi->gop_size);
        }

        if(rangexy > max_search_range)
        {
            rangexy = max_search_range;
        }

        if(pi->curr_mvr > 0)
        {
            if((abs(mvc[MV_X] - pi->max_imv[MV_X]) + range_offset) > rangexy)
            {
                offset_x = rangexy;
            }
            else
            {
                offset_x = abs(mvc[MV_X] - pi->max_imv[MV_X]) + range_offset;
            }

            if((abs(mvc[MV_Y] - pi->max_imv[MV_Y]) + range_offset) > rangexy)
            {
                offset_y = rangexy;
            }
            else
            {
                offset_y = abs(mvc[MV_Y] - pi->max_imv[MV_Y]) + range_offset;
            }
        }
        else
        {
            offset_x = rangexy;
            offset_y = rangexy;
        }

        /* define search range for int-pel search and clip it if needs */
        range[MV_RANGE_MIN][MV_X] = XEVE_CLIP3(pi->min_clip[MV_X], pi->max_clip[MV_X], mvc[MV_X] - offset_x);
        range[MV_RANGE_MAX][MV_X] = XEVE_CLIP3(pi->min_clip[MV_X], pi->max_clip[MV_X], mvc[MV_X] + offset_x);
        range[MV_RANGE_MIN][MV_Y] = XEVE_CLIP3(pi->min_clip[MV_Y], pi->max_clip[MV_Y], mvc[MV_Y] - offset_y);
        range[MV_RANGE_MAX][MV_Y] = XEVE_CLIP3(pi->min_clip[MV_Y], pi->max_clip[MV_Y], mvc[MV_Y] + offset_y);
    }
    else
    {
        int offset = pi->gop_size >> 1;
        int max_search_range = XEVE_CLIP3(pi->max_search_range >> 2, pi->max_search_range, (pi->max_search_range * XEVE_ABS(pi->poc - (int)pi->refp[ri][lidx].poc) + offset) / pi->gop_size);
        int search_range_x = bi ? BI_STEP : max_search_range;
        int search_range_y = bi ? BI_STEP : max_search_range;

        /* define search range for int-pel search and clip it if needs */
        range[MV_RANGE_MIN][MV_X] = XEVE_CLIP3(pi->min_clip[MV_X], pi->max_clip[MV_X], mvc[MV_X] - search_range_x);
        range[MV_RANGE_MAX][MV_X] = XEVE_CLIP3(pi->min_clip[MV_X], pi->max_clip[MV_X], mvc[MV_X] + search_range_x);
        range[MV_RANGE_MIN][MV_Y] = XEVE_CLIP3(pi->min_clip[MV_Y], pi->max_clip[MV_Y], mvc[MV_Y] - search_range_y);
        range[MV_RANGE_MAX][MV_Y] = XEVE_CLIP3(pi->min_clip[MV_Y], pi->max_clip[MV_Y], mvc[MV_Y] + search_range_y);
    }

    xeve_assert(range[MV_RANGE_MIN][MV_X] <= range[MV_RANGE_MAX][MV_X]);
    xeve_assert(range[MV_RANGE_MIN][MV_Y] <= range[MV_RANGE_MAX][MV_Y]);
}

/* Get original dummy buffer for bi prediction */
static void get_org_bi(pel * org, pel * pred, int s_o, int cuw, int cuh, s16 * org_bi)
{
    int i, j;

    for(j = 0; j < cuh; j++)
    {
        for(i = 0; i < cuw; i++)
        {
            org_bi[i] = ((s16)(org[i]) << 1) - (s16)pred[i];
        }

        org += s_o;
        pred += cuw;
        org_bi += cuw;
    }
}

static u32 me_raster(XEVE_PINTER * pi, int x, int y, int log2_cuw, int log2_cuh, s8 refi, int lidx, s16 range[MV_RANGE_DIM][MV_D], s16 gmvp[MV_D], s16 mv[MV_D], int bit_depth_luma, int cost_init)
{
    XEVE_PIC *ref_pic;
    pel      *org, *ref;
    u8        mv_bits, best_mv_bits;
    u32       cost_best, cost;
    int       i, j;
    s16       mv_x, mv_y;
    s32       search_step_x = XEVE_MAX(RASTER_SEARCH_STEP, (1 << (log2_cuw - 1))); /* Adaptive step size : Half of CU dimension */
    s32       search_step_y = XEVE_MAX(RASTER_SEARCH_STEP, (1 << (log2_cuh - 1))); /* Adaptive step size : Half of CU dimension */
    s16       center_mv[MV_D];
    s32       search_step;
    search_step_x = search_step_y = XEVE_MAX(RASTER_SEARCH_STEP, (1 << (XEVE_MIN(log2_cuh, log2_cuw) - 1)));
    search_step_x = search_step_y = XEVE_MAX(pi->me_opt->raster_search_step_opt, search_step_x);

    org = pi->o[Y_C] + y * pi->s_o[Y_C] + x;
    ref_pic = pi->refp[refi][lidx].pic;
    best_mv_bits = 0;
    cost_best = cost_init;

#if MULTI_REF_ME_STEP
    for(i = range[MV_RANGE_MIN][MV_Y]; i <= range[MV_RANGE_MAX][MV_Y]; i += (search_step_y * (refi + 1)))
    {
        for(j = range[MV_RANGE_MIN][MV_X]; j <= range[MV_RANGE_MAX][MV_X]; j += (search_step_x * (refi + 1)))
#else
    for(i = range[MV_RANGE_MIN][MV_Y]; i <= range[MV_RANGE_MAX][MV_Y]; i += search_step_y)
    {
        for(j = range[MV_RANGE_MIN][MV_X]; j <= range[MV_RANGE_MAX][MV_X]; j += search_step_x)
#endif
        {
            mv_x = j;
            mv_y = i;

            if(pi->curr_mvr > 2)
            {
                int shift = pi->curr_mvr - 2;
                int offset = 1 << (shift - 1);
                mv_x = mv_x >= 0 ? ((mv_x + offset) >> shift) << shift : -(((-mv_x + offset) >> shift) << shift);
                mv_y = mv_y >= 0 ? ((mv_y + offset) >> shift) << shift : -(((-mv_y + offset) >> shift) << shift);
            }

            /* get MVD bits */
            mv_bits = get_mv_bits((mv_x << 2) - gmvp[MV_X], (mv_y << 2) - gmvp[MV_Y], pi->num_refp, refi, pi->curr_mvr, pi->sps_amvr_flag);

            /* get MVD cost_best */
            cost = MV_COST(pi, mv_bits);
            ref = ref_pic->y + mv_x + mv_y * ref_pic->s_l;

            /* get sad */
            cost += xeve_sad_16b(log2_cuw, log2_cuh, org, ref, pi->s_o[Y_C], ref_pic->s_l, bit_depth_luma);

            /* check if motion cost_best is less than minimum cost_best */
            if(cost < cost_best)
            {
                mv[MV_X] = ((mv_x - x) << 2);
                mv[MV_Y] = ((mv_y - y) << 2);
                cost_best = cost;
                best_mv_bits = mv_bits;
            }
        }
    }

    /* Grid search around best mv for all dyadic step sizes till integer pel */
#if MULTI_REF_ME_STEP
    search_step = (refi + 1) * XEVE_MAX(search_step_x, search_step_y) >> 1;
#else
    search_step = XEVE_MAX(search_step_x, search_step_y) >> 1;
#endif

    /* Limit the search steps b/w min and max */
    search_step = XEVE_MIN(pi->me_opt->search_step_max, search_step);
    search_step = XEVE_MAX(pi->me_opt->search_step_min, search_step);
    int new_center_cnt = 0;

    while(search_step > 0)
    {
        center_mv[MV_X] = mv[MV_X];
        center_mv[MV_Y] = mv[MV_Y];

        for(i = -search_step; i <= search_step; i += search_step)
        {
            for(j = -search_step; j <= search_step; j += search_step)
            {
                mv_x = (center_mv[MV_X] >> 2) + x + j;
                mv_y = (center_mv[MV_Y] >> 2) + y + i;

                if((mv_x < range[MV_RANGE_MIN][MV_X]) || (mv_x > range[MV_RANGE_MAX][MV_X]))
                    continue;
                if((mv_y < range[MV_RANGE_MIN][MV_Y]) || (mv_y > range[MV_RANGE_MAX][MV_Y]))
                    continue;

                if(pi->curr_mvr > 2)
                {
                    int rounding = 0;
                    rounding = 1 << (pi->curr_mvr - 3);
                    if(mv_x > 0)
                    {
                        mv_x = ((mv_x + rounding) >> (pi->curr_mvr - 2)) << (pi->curr_mvr - 2);
                    }
                    else
                    {
                        mv_x = ((abs(mv_x) + rounding) >> (pi->curr_mvr - 2)) << (pi->curr_mvr - 2);
                        mv_x = -1 * mv_x;
                    }
                    if(mv_y > 0)
                    {
                        mv_y = ((mv_y + rounding) >> (pi->curr_mvr - 2)) << (pi->curr_mvr - 2);
                    }
                    else
                    {
                        mv_y = ((abs(mv_y) + rounding) >> (pi->curr_mvr - 2)) << (pi->curr_mvr - 2);
                        mv_y = -1 * mv_y;
                    }
                }

                /* get MVD bits */
                mv_bits = get_mv_bits((mv_x << 2) - gmvp[MV_X], (mv_y << 2) - gmvp[MV_Y], pi->num_refp, refi, pi->curr_mvr, pi->sps_amvr_flag);

                /* get MVD cost_best */
                cost = MV_COST(pi, mv_bits);
                ref = ref_pic->y + mv_x + mv_y * ref_pic->s_l;

                /* get sad */
                cost += xeve_sad_16b(log2_cuw, log2_cuh, org, ref, pi->s_o[Y_C], ref_pic->s_l, bit_depth_luma);

                /* check if motion cost_best is less than minimum cost_best */
                if(cost < cost_best)
                {
                    mv[MV_X] = ((mv_x - x) << 2);
                    mv[MV_Y] = ((mv_y - y) << 2);
                    cost_best = cost;
                    best_mv_bits = mv_bits;
                }
            }
        }

        /* check if center is changing */
        if (center_mv[MV_X] != mv[MV_X] || center_mv[MV_Y] != mv[MV_Y])
        {
            /* Refine around the new center with same step size */
            if (new_center_cnt > pi->me_opt->raster_new_center_th)
            {
                /* Halve the step size */
                search_step >>= 1;
            }
            new_center_cnt++;
        }
        else
        {
        /* Halve the step size */
        search_step >>= 1;
        }
    }

    if(best_mv_bits > 0)
    {
        pi->mot_bits[lidx] = best_mv_bits;
    }

    return cost_best;
}

static u32 me_ipel_refinement(XEVE_PINTER *pi, int x, int y, int log2_cuw, int log2_cuh, s8 refi, int lidx, s16 range[MV_RANGE_DIM][MV_D]
                            , s16 gmvp[MV_D], s16 mvi[MV_D], s16 mv[MV_D], int bi, int *beststep, int faststep, int bit_depth_luma)
{
    XEVE_PIC      *ref_pic;
    pel           *org, *ref;
    u32            cost, cost_best = XEVE_UINT32_MAX;
    int            mv_bits, best_mv_bits;
    s16            mv_x, mv_y, mv_best_x, mv_best_y;
    int            lidx_r = (lidx == REFP_0) ? REFP_1 : REFP_0;
    s16           *org_bi = pi->org_bi;
    int            step, i;
    s16            imv_x, imv_y;
    int            mvsize = 1;

    org = pi->o[Y_C] + y * pi->s_o[Y_C] + x;
    ref_pic = pi->refp[refi][lidx].pic;
    mv_best_x = (mvi[MV_X] >> 2);
    mv_best_y = (mvi[MV_Y] >> 2);
    best_mv_bits = 0;
    step = 1;
    mv_best_x = XEVE_CLIP3(pi->min_clip[MV_X], pi->max_clip[MV_X], mv_best_x);
    mv_best_y = XEVE_CLIP3(pi->min_clip[MV_Y], pi->max_clip[MV_Y], mv_best_y);

    //assert that mv is already rounded
    if(pi->curr_mvr > 2)
    {
        int shift = pi->curr_mvr - 2;
        int offset = 1 << (shift - 1);
        mv_best_x = mv_best_x >= 0 ? ((mv_best_x + offset) >> shift) << shift : -(((-mv_best_x + offset) >> shift) << shift);
        mv_best_y = mv_best_y >= 0 ? ((mv_best_y + offset) >> shift) << shift : -(((-mv_best_y + offset) >> shift) << shift);
    }

    imv_x = mv_best_x;
    imv_y = mv_best_y;

    int test_pos[9][2] = {{ 0, 0}, { -1, -1},{ -1, 0},{ -1, 1},{ 0, -1},{ 0, 1},{ 1, -1},{ 1, 0},{ 1, 1}};

    if(pi->curr_mvr > 2)
    {
        step = step * (1 << (pi->curr_mvr - 2));
    }

    for(i = 0; i <= 8; i++)
    {
        mv_x = imv_x + (step * test_pos[i][MV_X]);
        mv_y = imv_y + (step * test_pos[i][MV_Y]);

        if(mv_x > range[MV_RANGE_MAX][MV_X] ||
           mv_x < range[MV_RANGE_MIN][MV_X] ||
           mv_y > range[MV_RANGE_MAX][MV_Y] ||
           mv_y < range[MV_RANGE_MIN][MV_Y])
        {
            cost = XEVE_UINT32_MAX;
        }
        else
        {
            /* get MVD bits */
            mv_bits = get_mv_bits((mv_x << 2) - gmvp[MV_X], (mv_y << 2) - gmvp[MV_Y], pi->num_refp, refi, pi->curr_mvr, pi->sps_amvr_flag);


            if(bi)
            {
                mv_bits += pi->mot_bits[lidx_r];
            }

            /* get MVD cost_best */
            cost = MV_COST(pi, mv_bits);

            ref = ref_pic->y + mv_x + mv_y * ref_pic->s_l;
            if(bi)
            {
                /* get sad */
                cost += xeve_sad_bi_16b(log2_cuw, log2_cuh, org_bi, ref, 1 << log2_cuw, ref_pic->s_l, bit_depth_luma);
            }
            else
            {
                /* get sad */
                cost += xeve_sad_16b(log2_cuw, log2_cuh, org, ref, pi->s_o[Y_C], ref_pic->s_l, bit_depth_luma);
            }

            /* check if motion cost_best is less than minimum cost_best */
            if(cost < cost_best)
            {
                mv_best_x = mv_x;
                mv_best_y = mv_y;
                cost_best = cost;
                best_mv_bits = mv_bits;
            }
        }
    }

    /* set best MV */
    mv[MV_X] = ((mv_best_x - x) << 2);
    mv[MV_Y] = ((mv_best_y - y) << 2);

    if(bi != BI_NORMAL && best_mv_bits > 0)
    {
        pi->mot_bits[lidx] = best_mv_bits;
    }

    return cost_best;
}

static u32 me_ipel_diamond(XEVE_PINTER *pi, int x, int y, int log2_cuw, int log2_cuh, s8 refi, int lidx, s16 range[MV_RANGE_DIM][MV_D]
                         , s16 gmvp[MV_D], s16 mvi[MV_D], s16 mv[MV_D], int bi, int *beststep, int faststep, int bit_depth_luma)
{
    XEVE_PIC      *ref_pic;
    pel           *org, *ref;
    u32            cost, cost_best = XEVE_UINT32_MAX;
    int            mv_bits, best_mv_bits;
    s16            mv_x, mv_y, mv_best_x, mv_best_y;
    int            lidx_r = (lidx == REFP_0) ? REFP_1 : REFP_0;
    s16           *org_bi = pi->org_bi;
    s16            mvc[MV_D];
    int            step, i, j;
    int            min_cmv_x, min_cmv_y, max_cmv_x, max_cmv_y;
    s16            imv_x, imv_y;
    int            mvsize_r = 1, mvsize_c = 1;
    int not_found_best = 0;

    org = pi->o[Y_C] + y * pi->s_o[Y_C] + x;
    ref_pic = pi->refp[refi][lidx].pic;
    mv_best_x = (mvi[MV_X] >> 2);
    mv_best_y = (mvi[MV_Y] >> 2);
    best_mv_bits = 0;
    step = 0;
    mv_best_x = XEVE_CLIP3(pi->min_clip[MV_X], pi->max_clip[MV_X], mv_best_x);
    mv_best_y = XEVE_CLIP3(pi->min_clip[MV_Y], pi->max_clip[MV_Y], mv_best_y);

    if(pi->curr_mvr > 2)
    {
        int shift = pi->curr_mvr - 2;
        int offset = 1 << (shift - 1);
        mv_best_x = mv_best_x >= 0 ? ((mv_best_x + offset) >> shift) << shift : -(((-mv_best_x + offset) >> shift) << shift);
        mv_best_y = mv_best_y >= 0 ? ((mv_best_y + offset) >> shift) << shift : -(((-mv_best_y + offset) >> shift) << shift);
    }

    imv_x = mv_best_x;
    imv_y = mv_best_y;

    while(1)
    {
        not_found_best++;

        if(step <= pi->me_opt->opt_me_diamond_mvr012_step)
        {
            if(pi->curr_mvr > 2)
            {
                min_cmv_x = (mv_best_x <= range[MV_RANGE_MIN][MV_X]) ? mv_best_x : mv_best_x - ((bi == BI_NORMAL ? (BI_STEP - 2) : 1) << (pi->curr_mvr - 1));
                min_cmv_y = (mv_best_y <= range[MV_RANGE_MIN][MV_Y]) ? mv_best_y : mv_best_y - ((bi == BI_NORMAL ? (BI_STEP - 2) : 1) << (pi->curr_mvr - 1));
                max_cmv_x = (mv_best_x >= range[MV_RANGE_MAX][MV_X]) ? mv_best_x : mv_best_x + ((bi == BI_NORMAL ? (BI_STEP - 2) : 1) << (pi->curr_mvr - 1));
                max_cmv_y = (mv_best_y >= range[MV_RANGE_MAX][MV_Y]) ? mv_best_y : mv_best_y + ((bi == BI_NORMAL ? (BI_STEP - 2) : 1) << (pi->curr_mvr - 1));
            }
            else
            {
                min_cmv_x = (mv_best_x <= range[MV_RANGE_MIN][MV_X]) ? mv_best_x : mv_best_x - (bi == BI_NORMAL ? BI_STEP - pi->me_opt->mvr_012_bi_step : pi->me_opt->mvr_012_non_bi_step);
                min_cmv_y = (mv_best_y <= range[MV_RANGE_MIN][MV_Y]) ? mv_best_y : mv_best_y - (bi == BI_NORMAL ? BI_STEP - pi->me_opt->mvr_012_bi_step : pi->me_opt->mvr_012_non_bi_step);
                max_cmv_x = (mv_best_x >= range[MV_RANGE_MAX][MV_X]) ? mv_best_x : mv_best_x + (bi == BI_NORMAL ? BI_STEP - pi->me_opt->mvr_012_bi_step : pi->me_opt->mvr_012_non_bi_step);
                max_cmv_y = (mv_best_y >= range[MV_RANGE_MAX][MV_Y]) ? mv_best_y : mv_best_y + (bi == BI_NORMAL ? BI_STEP - pi->me_opt->mvr_012_bi_step : pi->me_opt->mvr_012_non_bi_step);
            }

            int off = 0, mask = 0;
            if(pi->curr_mvr > 2)
            {
                mvsize_r = 1 << (pi->curr_mvr - 2);
                mvsize_c = 1 << (pi->curr_mvr - 2);
            }
            else
            {
                mvsize_r = 1;
                mvsize_c = 1;
                if (bi == BI_NORMAL)
                {
                    mvsize_c = pi->me_opt->bi_normal_step_c;

                    //to select diagonal points in a 2x2
                    mask = pi->me_opt->bi_normal_mask;
                }
            }

            for(i = min_cmv_y; i <= max_cmv_y; i += mvsize_r)
            {
                for(j = min_cmv_x + off; j <= max_cmv_x; j += mvsize_c)
                {
                    mv_x = j;
                    mv_y = i;

                    if(mv_x > range[MV_RANGE_MAX][MV_X] ||
                       mv_x < range[MV_RANGE_MIN][MV_X] ||
                       mv_y > range[MV_RANGE_MAX][MV_Y] ||
                       mv_y < range[MV_RANGE_MIN][MV_Y])
                    {
                        cost = XEVE_UINT32_MAX;
                    }
                    else
                    {
                        /* get MVD bits */
                        mv_bits = get_mv_bits((mv_x << 2) - gmvp[MV_X], (mv_y << 2) - gmvp[MV_Y], pi->num_refp, refi, pi->curr_mvr, pi->sps_amvr_flag);

                        if(bi)
                        {
                            mv_bits += pi->mot_bits[lidx_r];
                        }

                        /* get MVD cost_best */
                        cost = MV_COST(pi, mv_bits);

                        ref = ref_pic->y + mv_x + mv_y * ref_pic->s_l;

                        if(bi)
                        {
                            /* get sad */
                            cost += xeve_sad_bi_16b(log2_cuw, log2_cuh, org_bi, ref, 1 << log2_cuw, ref_pic->s_l, bit_depth_luma);
                        }
                        else
                        {
                            /* get sad */
                            cost += xeve_sad_16b(log2_cuw, log2_cuh, org, ref, pi->s_o[Y_C], ref_pic->s_l, bit_depth_luma);
                        }

                        /* check if motion cost_best is less than minimum cost_best */
                        if(cost < cost_best)
                        {
                            mv_best_x = mv_x;
                            mv_best_y = mv_y;
                            *beststep = 2;
                            not_found_best = 0;
                            cost_best = cost;
                            best_mv_bits = mv_bits;
                        }
                    }
                }
                off ^= mask;
            }

            mvc[MV_X] = mv_best_x;
            mvc[MV_Y] = mv_best_y;

            get_range_ipel(pi, mvc, range, (bi != BI_NORMAL) ? 0 : 1, refi, lidx);

            step += 2;
            /* Optimization Notes:
            * Cover the diamond points in 5x5 region in next loop by using step=2
            */
            if (pi->curr_mvr <= 2)
            {
                step = pi->me_opt->mvr_02_step_nxt;
            }
        }
        else
        {
            int meidx = step > 8 ? 2 : 1;
            int multi;
            int loop_cnt = 8;
            if (meidx == 2)
            {
                loop_cnt = 16;
            }

            if(pi->curr_mvr > 2)
            {
                multi = step * (1 << (pi->curr_mvr - 2));
            }
            else
            {
                multi = step;
            }

            for(i = 0; i < loop_cnt; i++)
            {
                if (pi->curr_mvr <= 2)
                {
                    /* To allow only 4 diamond positions in 5x5 region for step=2 */
                    if ((step == pi->me_opt->mvr_012_step_th) && (i == 1 || i == 3 || i == 5 || i == 7))
                    {
                    continue;
                }
                }
                else
                if((step == 4) && (i == 1 || i == 3 || i == 5 || i == 7))
                {
                    continue;
                }

                mv_x = imv_x + ((multi >> meidx) * tbl_diapos_partial[meidx - 1][i][MV_X]);
                mv_y = imv_y + ((multi >> meidx) * tbl_diapos_partial[meidx - 1][i][MV_Y]);

                if(mv_x > range[MV_RANGE_MAX][MV_X] ||
                   mv_x < range[MV_RANGE_MIN][MV_X] ||
                   mv_y > range[MV_RANGE_MAX][MV_Y] ||
                   mv_y < range[MV_RANGE_MIN][MV_Y])
                {
                    cost = XEVE_UINT32_MAX;
                }
                else
                {
                    /* get MVD bits */
                    mv_bits = get_mv_bits((mv_x << 2) - gmvp[MV_X], (mv_y << 2) - gmvp[MV_Y], pi->num_refp, refi, pi->curr_mvr, pi->sps_amvr_flag);

                    if(bi)
                    {
                        mv_bits += pi->mot_bits[lidx_r];
                    }

                    /* get MVD cost_best */
                    cost = MV_COST(pi, mv_bits);

                    ref = ref_pic->y + mv_x + mv_y * ref_pic->s_l;
                    if(bi)
                    {
                        /* get sad */
                        cost += xeve_sad_bi_16b(log2_cuw, log2_cuh, org_bi, ref, 1 << log2_cuw, ref_pic->s_l, bit_depth_luma);
                    }
                    else
                    {
                        /* get sad */
                        cost += xeve_sad_16b(log2_cuw, log2_cuh, org, ref, pi->s_o[Y_C], ref_pic->s_l, bit_depth_luma);
                    }

                    /* check if motion cost_best is less than minimum cost_best */
                    if(cost < cost_best)
                    {
                        mv_best_x = mv_x;
                        mv_best_y = mv_y;
                        *beststep = step;
                        cost_best = cost;
                        best_mv_bits = mv_bits;
                        not_found_best = 0;
                    }
                }
            }
        }

        if(not_found_best == faststep)
        {
            break;
        }

        if(bi == BI_NORMAL)
        {
            break;
        }

        step <<= 1;

        if(step > pi->max_search_range || (step << (pi->curr_mvr - 2)) > pi->max_search_range)
        {
            break;
        }
    }

    /* set best MV */
    mv[MV_X] = ((mv_best_x - x) << 2);
    mv[MV_Y] = ((mv_best_y - y) << 2);

    if(bi != BI_NORMAL && best_mv_bits > 0)
    {
        pi->mot_bits[lidx] = best_mv_bits;
    }

    return cost_best;
}

static u32 me_spel_pattern(XEVE_PINTER *pi, int x, int y, int log2_cuw, int log2_cuh, s8 refi, int lidx
                         , s16 gmvp[MV_D], s16 mvi[MV_D], s16 mv[MV_D], int bi, int bit_depth_luma)
{
    pel     *org, *ref, *pred;
    s16     *org_bi;
    u32      cost, cost_best = XEVE_UINT32_MAX;
    s16      mv_x, mv_y, cx, cy;
    int      lidx_r = (lidx == REFP_0) ? REFP_1 : REFP_0;
    int      i, mv_bits, cuw, cuh, s_org, s_ref, best_mv_bits;

    s_org = pi->s_o[Y_C];
    org = pi->o[Y_C] + x + y * pi->s_o[Y_C];
    s_ref = pi->refp[refi][lidx].pic->s_l;
    ref = pi->refp[refi][lidx].pic->y;
    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;
    org_bi = pi->org_bi;
    pred = pi->pred_buf;
    best_mv_bits = 0;

    /* make MV to be global coordinate */
    cx = mvi[MV_X] + (x << 2);
    cy = mvi[MV_Y] + (y << 2);

    /* intial value */
    mv[MV_X] = mvi[MV_X];
    mv[MV_Y] = mvi[MV_Y];

    /* search upto hpel-level from here */
    /* search of large diamond pattern */
    for(i = 0; i < pi->search_pattern_hpel_cnt; i++)
    {
        mv_x = cx + pi->search_pattern_hpel[i][0];
        mv_y = cy + pi->search_pattern_hpel[i][1];

        /* get MVD bits */
        mv_bits = get_mv_bits(mv_x - gmvp[MV_X], mv_y - gmvp[MV_Y], pi->num_refp, refi, pi->curr_mvr, pi->sps_amvr_flag);

        if(bi)
        {
            mv_bits += pi->mot_bits[lidx_r];
        }

        /* get MVD cost_best */
        cost = MV_COST(pi, mv_bits);

        /* get the interpolated(predicted) image */
        xeve_mc_l((mv_x << 2), (mv_y << 2), ref, (mv_x << 2), (mv_y << 2), s_ref, cuw, pred, cuw, cuh, bit_depth_luma, pi->mc_l_coeff);

        if(bi)
        {
            /* get sad */
            cost += xeve_sad_bi_16b(log2_cuw, log2_cuh, org_bi, pred, cuw, cuw, bit_depth_luma);
        }
        else
        {
            /* get sad */
            cost += xeve_sad_16b(log2_cuw, log2_cuh, org, pred, s_org, cuw, bit_depth_luma);
        }

        /* check if motion cost_best is less than minimum cost_best */
        if(cost < cost_best)
        {
            mv[MV_X] = mv_x - (x << 2);
            mv[MV_Y] = mv_y - (y << 2);
            cost_best = cost;
        }
    }

    /* search upto qpel-level from here*/
    /* search of small diamond pattern */
    if(pi->me_level > ME_LEV_HPEL && pi->curr_mvr == 0)
    {
        /* make MV to be absolute coordinate */
        cx = mv[MV_X] + (x << 2);
        cy = mv[MV_Y] + (y << 2);

        for(i = 0; i < pi->search_pattern_qpel_cnt; i++)
        {
            mv_x = cx + pi->search_pattern_qpel[i][0];
            mv_y = cy + pi->search_pattern_qpel[i][1];

            /* get MVD bits */
            mv_bits = get_mv_bits(mv_x - gmvp[MV_X], mv_y - gmvp[MV_Y], pi->num_refp, refi, pi->curr_mvr, pi->sps_amvr_flag);

            if(bi)
            {
                mv_bits += pi->mot_bits[lidx_r];
            }

            /* get MVD cost_best */
            cost = MV_COST(pi, mv_bits);

            /* get the interpolated(predicted) image */
            xeve_mc_l((mv_x << 2), (mv_y << 2), ref, (mv_x << 2), (mv_y << 2), s_ref, cuw, pred, cuw, cuh, bit_depth_luma, pi->mc_l_coeff);

            if(bi)
            {
                /* get sad */
                cost += xeve_sad_bi_16b(log2_cuw, log2_cuh, org_bi, pred, cuw, cuw, bit_depth_luma);
            }
            else
            {
                /* get sad */
                cost += xeve_sad_16b(log2_cuw, log2_cuh, org, pred, s_org, cuw, bit_depth_luma);
            }

            /* check if motion cost_best is less than minimum cost_best */
            if(cost < cost_best)
            {
                mv[MV_X] = mv_x - (x << 2);
                mv[MV_Y] = mv_y - (y << 2);
                cost_best = cost;
                best_mv_bits = mv_bits;
            }
        }
    }

    if(!bi && best_mv_bits > 0)
    {
        pi->mot_bits[lidx] = best_mv_bits;
    }

    return cost_best;
}

static u32 pinter_me_epzs(XEVE_PINTER * pi, int x, int y, int log2_cuw, int log2_cuh, s8 * refi, int lidx, s16 mvp[MV_D], s16 mv[MV_D], int bi, int bit_depth_luma)
{
    s16 mvc[MV_D];  /* MV center for search */
    s16 gmvp[MV_D]; /* MVP in frame cordinate */
    s16 range[MV_RANGE_DIM][MV_D]; /* search range after clipping */
    s16 mvi[MV_D];
    s16 mvt[MV_D];
    u32 cost, cost_best = XEVE_UINT32_MAX;
    s8 ri = 0;  /* reference buffer index */
    int tmpstep = 0;
    int beststep = 0;

    gmvp[MV_X] = mvp[MV_X] + (x << 2);
    gmvp[MV_Y] = mvp[MV_Y] + (y << 2);

    if(bi == BI_NORMAL)
    {
        mvi[MV_X] = mv[MV_X] + (x << 2);
        mvi[MV_Y] = mv[MV_Y] + (y << 2);
        mvc[MV_X] = x + (mv[MV_X] >> 2);
        mvc[MV_Y] = y + (mv[MV_Y] >> 2);
    }
    else
    {
        mvi[MV_X] = mvp[MV_X] + (x << 2);
        mvi[MV_Y] = mvp[MV_Y] + (y << 2);
        mvc[MV_X] = x + (mvp[MV_X] >> 2);
        mvc[MV_Y] = y + (mvp[MV_Y] >> 2);
    }

    ri = *refi;

    mvc[MV_X] = XEVE_CLIP3(pi->min_clip[MV_X], pi->max_clip[MV_X], mvc[MV_X]);
    mvc[MV_Y] = XEVE_CLIP3(pi->min_clip[MV_Y], pi->max_clip[MV_Y], mvc[MV_Y]);

    get_range_ipel(pi, mvc, range, (bi != BI_NORMAL) ? 0 : 1, ri, lidx);
    cost = me_ipel_diamond(pi, x, y, log2_cuw, log2_cuh, ri, lidx, range, gmvp, mvi, mvt, bi, &tmpstep, MAX_FIRST_SEARCH_STEP - pi->me_opt->max_first_search_step_th, bit_depth_luma);

    if(cost < cost_best)
    {
        cost_best = cost;
        mv[MV_X] = mvt[MV_X];
        mv[MV_Y] = mvt[MV_Y];
        if(abs(mvp[MV_X] - mv[MV_X]) < 2 && abs(mvp[MV_Y] - mv[MV_Y]) < 2)
        {
            beststep = 0;
        }
        else
        {
            beststep = tmpstep;
        }
    }

    int cost_init = XEVE_UINT32_MAX;
    /* Do raster search with best cost found so far */
    cost_init = cost_best;
    if(bi == BI_NON && beststep > RASTER_SEARCH_THD && pi->me_complexity > 1)
    {
        cost = me_raster(pi, x, y, log2_cuw, log2_cuh, ri, lidx, range, gmvp, mvt, bit_depth_luma, cost_init);

        if(cost < cost_best)
        {
            beststep = RASTER_SEARCH_THD;

            cost_best = cost;

            mv[MV_X] = mvt[MV_X];
            mv[MV_Y] = mvt[MV_Y];
        }
    }

    while(bi != BI_NORMAL && beststep > REFINE_SEARCH_THD && pi->me_complexity > 0)
    {
        mvc[MV_X] = x + (mv[MV_X] >> 2);
        mvc[MV_Y] = y + (mv[MV_Y] >> 2);

        get_range_ipel(pi, mvc, range, (bi != BI_NORMAL) ? 0 : 1, ri, lidx);

        mvi[MV_X] = mv[MV_X] + (x << 2);
        mvi[MV_Y] = mv[MV_Y] + (y << 2);

        beststep = 0;
        cost = me_ipel_diamond(pi, x, y, log2_cuw, log2_cuh, ri, lidx, range, gmvp, mvi, mvt, bi, &tmpstep, MAX_REFINE_SEARCH_STEP - pi->me_opt->max_refine_search_step_th, bit_depth_luma);

        if(cost < cost_best)
        {
            cost_best = cost;

            mv[MV_X] = mvt[MV_X];
            mv[MV_Y] = mvt[MV_Y];

            if(abs(mvp[MV_X] - mv[MV_X]) < 2 && abs(mvp[MV_Y] - mv[MV_Y]) < 2)
            {
                beststep = 0;
            }
            else
            {
                beststep = tmpstep;
            }

        }
    }

    if(pi->me_level > ME_LEV_IPEL && (pi->curr_mvr == 0 || pi->curr_mvr == 1))
    {
        /* sub-pel ME */
        cost = me_spel_pattern(pi, x, y, log2_cuw, log2_cuh, ri, lidx, gmvp, mv, mvt, bi, bit_depth_luma);

        if(cost < cost_best)
        {
            cost_best = cost;

            mv[MV_X] = mvt[MV_X];
            mv[MV_Y] = mvt[MV_Y];
        }
    }
    else
    {
        mvc[MV_X] = x + (mv[MV_X] >> 2);
        mvc[MV_Y] = y + (mv[MV_Y] >> 2);

        get_range_ipel(pi, mvc, range, (bi != BI_NORMAL) ? 0 : 1, ri, lidx);

        mvi[MV_X] = mv[MV_X] + (x << 2);
        mvi[MV_Y] = mv[MV_Y] + (y << 2);

        cost = me_ipel_refinement(pi, x, y, log2_cuw, log2_cuh, ri, lidx, range, gmvp, mvi, mvt, bi, &tmpstep, MAX_REFINE_SEARCH_STEP, bit_depth_luma);

        if(cost < cost_best)
        {
            cost_best = cost;

            mv[MV_X] = mvt[MV_X];
            mv[MV_Y] = mvt[MV_Y];
        }
    }

    return cost_best;
}

static void xeve_mc_mmvd(int x, int y, int pic_w, int pic_h, int w, int h, s8 refi[REFP_NUM], s16 mv[REFP_NUM][MV_D], XEVE_REFP(*refp)[REFP_NUM]
                       , pel pred[2][N_C][MAX_CU_DIM], int bit_depth_luma,  XEVE_MMVD_OPT *mmvd_opt)
{
    XEVE_PIC *ref_pic;
    int       qpel_gmv_x, qpel_gmv_y;
    int       bidx = 0;
    s16       mv_t[REFP_NUM][MV_D];
    xeve_mv_clip(x, y, pic_w, pic_h, w, h, refi, mv, mv_t);
    if(REFI_IS_VALID(refi[REFP_0]))
    {
        ref_pic = refp[refi[REFP_0]][REFP_0].pic;
        int x_changed, y_changed;
        x_changed = x - 128 * ((int)(x / 128));
        y_changed = y - 128 * ((int)(y / 128));
        int mmvd_opt_switch = 1;
        if(mmvd_opt->enabled)
        {
            int idx_frm_poc = mmvd_opt->poc_to_idx[ref_pic->poc % mmvd_opt->i_period];
            if(idx_frm_poc == -1)
            {
                int i;
                for(i = 0; i < 4; i++)
                {
                    if(mmvd_opt->ref_buf_idx[i] == 0)
                    {
                        break;
                    }
                }
                if(i == 4)
                    mmvd_opt_switch = 0;
            }
        }
        if(mmvd_opt->enabled && mmvd_opt_switch &&
            ((x_changed << 2) + mv_t[REFP_0][MV_X]) >= -(OPT_MC_BI_PAD << 2) &&
            ((x_changed<<2) + mv_t[REFP_0][MV_X]) < (((MAX_CU_SIZE + OPT_MC_BI_PAD) << 2) - (w << 2)) &&
            ((y_changed << 2) + mv_t[REFP_0][MV_Y]) >= -(OPT_MC_BI_PAD << 2) &&
            ((y_changed<<2) + mv_t[REFP_0][MV_Y]) < (((MAX_CU_SIZE + OPT_MC_BI_PAD) << 2) - (h << 2)))
        {
            qpel_gmv_x = (x_changed << 2) + mv_t[REFP_0][MV_X];
            qpel_gmv_y = (y_changed << 2) + mv_t[REFP_0][MV_Y];
            int idx_frm_poc = mmvd_opt->poc_to_idx[ref_pic->poc % mmvd_opt->i_period];
            if(idx_frm_poc==-1)
            {
                int i;
                for(i = 0; i < 4; i++)
                {
                    if(mmvd_opt->ref_buf_idx[i] == 0)
                    {
                        mmvd_opt->ref_buf_idx[i] = 1;
                        mmvd_opt->poc_to_idx[ref_pic->poc % mmvd_opt->i_period] = i;
                        break;
                    }
                }
                idx_frm_poc = mmvd_opt->poc_to_idx[ref_pic->poc % mmvd_opt->i_period];
                int x_buf, y_buf;
                x_buf = 128 * ((int)x / 128);
                y_buf = 128 * ((int)y / 128);
                for(int yy = 0; yy < 4; yy++)
                {
                    for(int xx = 0; xx < 4; xx++)
                    {
                        int qpel_gmv_x_buf = (x_buf << 2) -(OPT_MC_BI_PAD<<2) + xx;
                        int qpel_gmv_y_buf = (y_buf << 2) - (OPT_MC_BI_PAD << 2) + yy;
                        int idx = ((qpel_gmv_x_buf & 0x03) << 2) + (qpel_gmv_y_buf & 0x03);
                        xeve_bl_mc_l(ref_pic->y, (qpel_gmv_x_buf << 2), (qpel_gmv_y_buf << 2), ref_pic->s_l, (MAX_CU_SIZE + OPT_MC_BI_PAD * 2),
                            mmvd_opt->pred_bi[idx_frm_poc][idx], (MAX_CU_SIZE + OPT_MC_BI_PAD * 2), (MAX_CU_SIZE + OPT_MC_BI_PAD * 2), bit_depth_luma);
                    }
                }
            }
            int idx = ((mv_t[REFP_0][MV_X] & 0x03) << 2) + (mv_t[REFP_0][MV_Y] & 0x03);
            qpel_gmv_x += (OPT_MC_BI_PAD << 2);
            qpel_gmv_y += (OPT_MC_BI_PAD << 2);
            (xevem_func_bl_mc_l[0][0])(mmvd_opt->pred_bi[idx_frm_poc][idx], (qpel_gmv_x << 2), (qpel_gmv_y << 2), (MAX_CU_SIZE + OPT_MC_BI_PAD * 2), w, pred[0][Y_C], w, h, bit_depth_luma);
        }
        else
        {
            qpel_gmv_x = (x << 2) + mv_t[REFP_0][MV_X];
            qpel_gmv_y = (y << 2) + mv_t[REFP_0][MV_Y];
            xeve_bl_mc_l(ref_pic->y, (qpel_gmv_x << 2), (qpel_gmv_y << 2), ref_pic->s_l, w, pred[0][Y_C], w, h, bit_depth_luma);
        }
        bidx++;
    }
    if(REFI_IS_VALID(refi[REFP_0]) && REFI_IS_VALID(refi[REFP_1]))
    {
        if(refp[refi[REFP_0]][REFP_0].pic->poc == refp[refi[REFP_1]][REFP_1].pic->poc &&  mv_t[REFP_0][MV_X] == mv_t[REFP_1][MV_X] && mv_t[REFP_0][MV_Y] == mv_t[REFP_1][MV_Y])
        {
            return;
        }
    }
    if(REFI_IS_VALID(refi[REFP_1]))
    {
        ref_pic = refp[refi[REFP_1]][REFP_1].pic;
        int x_changed, y_changed;
        x_changed = x - 128 * ((int)(x / 128));
        y_changed = y - 128 * ((int)(y / 128));
        int mmvd_opt_switch = 1;
        if(mmvd_opt->enabled)
        {
            int idx_frm_poc = mmvd_opt->poc_to_idx[ref_pic->poc % mmvd_opt->i_period];
            if(idx_frm_poc == -1)
            {
                int i;
                for(i = 0; i < 4; i++)
                {
                    if(mmvd_opt->ref_buf_idx[i] == 0)
                    {
                        break;
                    }
                }
                if(i == 4)
                    mmvd_opt_switch = 0;
            }
        }
        if( mmvd_opt->enabled && mmvd_opt_switch &&
            ((x_changed << 2) + mv_t[REFP_1][MV_X]) >= -(OPT_MC_BI_PAD << 2) &&
            ((x_changed << 2) + mv_t[REFP_1][MV_X]) < (((MAX_CU_SIZE + OPT_MC_BI_PAD) << 2) - (w << 2)) &&
            ((y_changed << 2) + mv_t[REFP_1][MV_Y]) >= -(OPT_MC_BI_PAD << 2) &&
            ((y_changed << 2) + mv_t[REFP_1][MV_Y]) < (((MAX_CU_SIZE + OPT_MC_BI_PAD) << 2) - (h << 2)))
        {
            qpel_gmv_x = (x_changed << 2) + mv_t[REFP_1][MV_X];
            qpel_gmv_y = (y_changed << 2) + mv_t[REFP_1][MV_Y];
            int idx_frm_poc = mmvd_opt->poc_to_idx[ref_pic->poc % mmvd_opt->i_period];
            if(idx_frm_poc == -1)
            {
                int i;
                for(i = 0; i < 4; i++)
                {
                    if(mmvd_opt->ref_buf_idx[i] == 0)
                    {
                        mmvd_opt->ref_buf_idx[i] = 1;
                        mmvd_opt->poc_to_idx[ref_pic->poc % mmvd_opt->i_period] = i;
                        break;
                    }
                }
                idx_frm_poc = mmvd_opt->poc_to_idx[ref_pic->poc % mmvd_opt->i_period];
                int x_buf, y_buf;
                x_buf = 128 * ((int)x / 128);
                y_buf = 128 * ((int)y / 128);
                for(int yy = 0; yy < 4; yy++)
                {
                    for(int xx = 0; xx < 4; xx++)
                    {
                        int qpel_gmv_x_buf = (x_buf << 2) - (OPT_MC_BI_PAD << 2) + xx;
                        int qpel_gmv_y_buf = (y_buf << 2) - (OPT_MC_BI_PAD << 2) + yy;
                        int idx = ((qpel_gmv_x_buf & 0x03) << 2) + (qpel_gmv_y_buf & 0x03);
                        xeve_bl_mc_l(ref_pic->y, (qpel_gmv_x_buf << 2), (qpel_gmv_y_buf << 2), ref_pic->s_l, (MAX_CU_SIZE + OPT_MC_BI_PAD * 2),
                            mmvd_opt->pred_bi[idx_frm_poc][idx], (MAX_CU_SIZE + OPT_MC_BI_PAD * 2), (MAX_CU_SIZE + OPT_MC_BI_PAD * 2), bit_depth_luma);
                    }
                }
            }
            int idx = ((mv_t[REFP_1][MV_X] & 0x03) << 2) + (mv_t[REFP_1][MV_Y] & 0x03);
            qpel_gmv_x += (OPT_MC_BI_PAD << 2);
            qpel_gmv_y += (OPT_MC_BI_PAD << 2);
            (xevem_func_bl_mc_l[0][0])(mmvd_opt->pred_bi[idx_frm_poc][idx], (qpel_gmv_x << 2), (qpel_gmv_y << 2), (MAX_CU_SIZE + OPT_MC_BI_PAD * 2), w, pred[bidx][Y_C], w, h, bit_depth_luma);
        }
        else
        {
            qpel_gmv_x = (x << 2) + mv_t[REFP_1][MV_X];
            qpel_gmv_y = (y << 2) + mv_t[REFP_1][MV_Y];
            xeve_bl_mc_l(ref_pic->y, (qpel_gmv_x << 2), (qpel_gmv_y << 2), ref_pic->s_l, w, pred[bidx][Y_C], w, h, bit_depth_luma);
        }
        bidx++;
    }
    if(bidx == 2)
    {
        xeve_func_average_no_clip(pred[0][Y_C], pred[1][Y_C], pred[0][Y_C], w, w, w, w, h);
    }
}

__inline static int mmvd_bit_unary_sym(u32 sym, u32 num_ctx, u32 max_num)
{
    int bits = 0;
    u32 ctx_idx = 0;
    int symbol = 0;

    if(max_num > 1)
    {
        for(ctx_idx = 0; ctx_idx < max_num - 1; ++ctx_idx)
        {
            symbol = (ctx_idx == sym) ? 0 : 1;
            bits++;

            if(symbol == 0)
            {
                break;
            }
        }
    }

    return bits;
}

__inline static int mmvd_info_bit_cost(int mvp_idx, int type)
{
    int bits = 0;
    int var0, var1, var2;
    int dev0 = 0;
    int var;

    if(type == 1)
    {
        if(mvp_idx >= (MMVD_MAX_REFINE_NUM*MMVD_BASE_MV_NUM))
        {
            mvp_idx = mvp_idx - (MMVD_MAX_REFINE_NUM*MMVD_BASE_MV_NUM);
            dev0 = mvp_idx / (MMVD_MAX_REFINE_NUM*MMVD_BASE_MV_NUM);
            mvp_idx = mvp_idx - dev0 * (MMVD_MAX_REFINE_NUM*MMVD_BASE_MV_NUM);
            var = 1;
        }
        else
        {
            var = 0;
        }

        /* mmvd_group_idx */
        bits += 1;
        if(var == 1)
        {
            bits += 1;
        }
    }
    else
    {
        var = 0;
        dev0 = 0;
    }

    var0 = mvp_idx / MMVD_MAX_REFINE_NUM;
    var1 = (mvp_idx - (var0 * MMVD_MAX_REFINE_NUM)) / 4;
    var2 = mvp_idx - (var0 * MMVD_MAX_REFINE_NUM) - var1 * 4;

    /* mmvd_merge_idx */
    bits += mmvd_bit_unary_sym(var0, NUM_CTX_MMVD_MERGE_IDX, MMVD_BASE_MV_NUM);
    /* mmvd_distance_idx */
    bits += mmvd_bit_unary_sym(var1, NUM_CTX_MMVD_DIST_IDX, MMVD_DIST_NUM);
    /* mmvd_direction_idx */
    if(var2 == 0)
    {
        bits += 2;
    }
    else if(var2 == 1)
    {
        bits += 2;
    }
    else if(var2 == 2)
    {
        bits += 2;
    }
    else if(var2 == 3)
    {
        bits += 2;
    }

    return bits;
}

static double pinter_residue_rdo_mmvd(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh, pel pred[2][N_C][MAX_CU_DIM], int pidx)
{
    XEVE_PINTER * pi = &ctx->pinter[core->thread_cnt];
    XEVEM_CORE  * mcore = (XEVEM_CORE *)core;
    int           w, h, log2_w, log2_h;
    int           bit_cnt;
    double        cost = 0.0;
    pel         * y_org;

    w = 1 << log2_cuw;
    h = 1 << log2_cuh;
    log2_w = log2_cuw;
    log2_h = log2_cuh;

    /* prediction */
    xeve_mc_mmvd(x, y, ctx->w, ctx->h, w, h, pi->refi[pidx], pi->mv[pidx], pi->refp, pred, ctx->sps.bit_depth_luma_minus8 + 8, &mcore->mmvd_opt);

    /* get distortion */
    y_org = pi->o[Y_C] + x + y * pi->s_o[Y_C];
    cost = xeve_satd_16b(log2_w, log2_h, pred[0][Y_C], y_org, w, pi->s_o[Y_C], ctx->sps.bit_depth_luma_minus8 + 8);

    /* get bits */
    bit_cnt = mmvd_info_bit_cost(pi->mmvd_idx[pidx], ctx->sh->mmvd_group_enable_flag && !((1 << core->log2_cuw)*(1 << core->log2_cuh) <= NUM_SAMPLES_BLOCK));

    /* get RD cost */
    cost += RATE_TO_COST_SQRT_LAMBDA(core->sqrt_lambda[0], bit_cnt);

    return cost;
}

static void copy_tu_from_cu(s16 tu_resi[N_C][MAX_CU_DIM], s16 cu_resi[N_C][MAX_CU_DIM], int log2_cuw, int log2_cuh, u8 ats_inter_info, int chroma_format_idc)
{
    int j;
    int cuw = 1 << log2_cuw;
    int log2_tuw, log2_tuh;
    int tuw, tuh;
    int tu_offset_x, tu_offset_y;
    int w_shift = XEVE_GET_CHROMA_W_SHIFT(chroma_format_idc);
    int h_shift = XEVE_GET_CHROMA_H_SHIFT(chroma_format_idc);

    get_tu_size(ats_inter_info, log2_cuw, log2_cuh, &log2_tuw, &log2_tuh);
    get_tu_pos_offset(ats_inter_info, log2_cuw, log2_cuh, &tu_offset_x, &tu_offset_y);
    tuw = 1 << log2_tuw;
    tuh = 1 << log2_tuh;

    //Y
    for(j = tu_offset_y; j < tu_offset_y + tuh; j++)
    {
        xeve_mcpy(tu_resi[Y_C] + (j - tu_offset_y) * tuw, cu_resi[Y_C] + tu_offset_x + j * cuw, sizeof(s16)*tuw);
    }

    //UV
    if(chroma_format_idc)
    {
        tu_offset_x >>= w_shift;
        tu_offset_y >>= h_shift;
        tuw >>= w_shift;
        tuh >>= h_shift;
        cuw >>= w_shift;

        for(j = tu_offset_y; j < tu_offset_y + tuh; j++)
        {
            xeve_mcpy(tu_resi[U_C] + (j - tu_offset_y) * tuw, cu_resi[U_C] + tu_offset_x + j * cuw, sizeof(s16)*tuw);
            xeve_mcpy(tu_resi[V_C] + (j - tu_offset_y) * tuw, cu_resi[V_C] + tu_offset_x + j * cuw, sizeof(s16)*tuw);
        }
    }
}

void get_ats_inter_info_rdo_order(XEVE_CORE *core, u8 ats_inter_avail, int* num_rdo, u8* ats_inter_info_list)
{
    int i;
    u8 idx = 0;
    if(ats_inter_avail == 0)
    {
        ats_inter_info_list[idx++] = 0;
    }
    else
    {
        //add non-ats_inter mode
        ats_inter_info_list[idx++] = 0;

        //add ats_inter mode
        for(i = 0; i < 4; i++)
        {
            if((ats_inter_avail >> i) & 0x1)
            {
                ats_inter_info_list[idx++] = get_ats_inter_info(i + 1, 0);
                ats_inter_info_list[idx++] = get_ats_inter_info(i + 1, 1);
            }
        }

        //toDO: add reordering fast algorithm based on estimated RDCost
    }

    *num_rdo = idx;
}

//fast algorithms for ATS_inter
void calc_min_cost_ats_inter(XEVE_CTX *ctx, XEVE_CORE *core, pel pred[N_C][MAX_CU_DIM], pel** org, int* s_pred, int* s_org,
                             u8 ats_inter_avail, s64* dist_no_resi, int* num_rdo, u8* ats_inter_info_list, s64* ats_inter_est_dist)
{
    int cuw = 1 << core->log2_cuw;
    int cuh = 1 << core->log2_cuh;
    int num_part_x = XEVE_MIN(16, cuw) / 4;
    int num_part_y = XEVE_MIN(16, cuh) / 4;
    int log2_length_x[3];
    int log2_length_y[3];
    s64 dist[4][4], dist_blk, dist_temp[9];
    s64 sum_dist = 0;
    u8  ats_inter_info_list_temp[9];
    int comp, i, j, idx;
    int blk_luma_w = cuw / num_part_x;
    int blk_luma_h = cuh / num_part_y;
    int ats_inter_rdo_idx_list[4] = { 0 };
    int ats_inter_rdo_idx_num = 0;
    int num_half_ats_inter = ((ats_inter_avail & 0x1) ? 2 : 0) + ((ats_inter_avail & 0x2) ? 2 : 0);
    int num_quad_ats_inter = ((ats_inter_avail & 0x4) ? 2 : 0) + ((ats_inter_avail & 0x8) ? 2 : 0);
    assert(num_half_ats_inter + num_quad_ats_inter == *num_rdo - 1);

    if(!ats_inter_avail)
        return;

    //ATS_INTER fast algorithm 1.1: not try ATS_INTER if the residual is too small to compensate bits for encoding residual info
    if(dist_no_resi[Y_C] + dist_no_resi[U_C] * core->dist_chroma_weight[0] + dist_no_resi[V_C] * core->dist_chroma_weight[1]
       < RATE_TO_COST_LAMBDA(core->lambda[0], 20)) //20 extra bits for ATS_INTER residual encoding
    {
        *num_rdo = 1;
        return;
    }

    int bit_depth_tbl[3] = {ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8 , ctx->sps.bit_depth_chroma_minus8 + 8};
    //ATS_INTER fast algorithm 1.2: derive estimated minDist of ATS_INTER = zero-residual part distortion + non-zero residual part distortion / 16
    xeve_mset(dist, 0, sizeof(s64) * 16);
    for(comp = Y_C; comp < N_C; comp++)
    {
        int blk_w, blk_h;
        if(comp != Y_C && ctx->sps.chroma_format_idc == 0)
            continue;
        log2_length_x[comp] = xeve_tbl_log2[blk_luma_w] - (comp > 0 ? ctx->param.cs_w_shift : 0);
        log2_length_y[comp] = xeve_tbl_log2[blk_luma_h] - (comp > 0 ? ctx->param.cs_h_shift : 0);
        blk_w = 1 << log2_length_x[comp];
        blk_h = 1 << log2_length_y[comp];

        for(j = 0; j < num_part_y; j++)
        {
            for(i = 0; i < num_part_x; i++)
            {
                int offset_pred = j * blk_h * s_pred[comp] + i * blk_w;
                int offset_org = j * blk_h * s_org[comp] + i * blk_w;

                dist_blk = xeve_ssd_16b(log2_length_x[comp], log2_length_y[comp], pred[comp] + offset_pred, org[comp] + offset_org, s_pred[comp], s_org[comp], bit_depth_tbl[comp]);
                dist_blk = comp > 0 ? (s64)(dist_blk * core->dist_chroma_weight[comp - 1]) : dist_blk;
                dist[j][i] += dist_blk;
                sum_dist += dist_blk;
            }
        }
    }
    assert(abs((int)(sum_dist - (dist_no_resi[Y_C] + dist_no_resi[U_C] * core->dist_chroma_weight[0] + dist_no_resi[V_C] * core->dist_chroma_weight[1]))) < 32);

    //estimate rd cost for each ATS_INTER mode
    ats_inter_est_dist[0] = sum_dist;
    for(idx = 1; idx < 9; idx++)
    {
        ats_inter_est_dist[idx] = UINT_MAX;
    }
    for(idx = 1; idx < *num_rdo; idx++)
    {
        u8 ats_inter_info = ats_inter_info_list[idx];
        int log2_tuw, log2_tuh, tux, tuy, tuw, tuh;
        s64 dist_tu = 0;
        get_tu_size(ats_inter_info, core->log2_cuw, core->log2_cuh, &log2_tuw, &log2_tuh);
        get_tu_pos_offset(ats_inter_info, core->log2_cuw, core->log2_cuh, &tux, &tuy);
        tuw = 1 << log2_tuw;
        tuh = 1 << log2_tuh;
        for(j = tuy / blk_luma_h; j < (tuy + tuh) / blk_luma_h; j++)
        {
            for(i = tux / blk_luma_w; i < (tux + tuw) / blk_luma_w; i++)
            {
                dist_tu += dist[j][i];
            }
        }
        ats_inter_est_dist[idx] = (dist_tu / 16) + (sum_dist - dist_tu);
    }
    //try 2 half ATS_INTER modes with the lowest distortion
    xeve_mcpy(dist_temp, ats_inter_est_dist, sizeof(s64) * 9);
    if(num_half_ats_inter > 0)
    {
        for(i = ats_inter_rdo_idx_num; i < ats_inter_rdo_idx_num + 2; i++)
        {
            s64 min_dist = UINT_MAX;
            for(idx = 1; idx < 1 + num_half_ats_inter; idx++)
            {
                if(dist_temp[idx] < min_dist)
                {
                    min_dist = dist_temp[idx];
                    ats_inter_rdo_idx_list[i] = idx;
                }
            }
            dist_temp[ats_inter_rdo_idx_list[i]] = UINT_MAX;
        }
        ats_inter_rdo_idx_num += 2;
    }
    if(num_quad_ats_inter > 0)
    {
        for(i = ats_inter_rdo_idx_num; i < ats_inter_rdo_idx_num + 2; i++)
        {
            s64 min_dist = UINT_MAX;
            for(idx = 1 + num_half_ats_inter; idx < 1 + num_half_ats_inter + num_quad_ats_inter; idx++)
            {
                if(dist_temp[idx] < min_dist)
                {
                    min_dist = dist_temp[idx];
                    ats_inter_rdo_idx_list[i] = idx;
                }
            }
            dist_temp[ats_inter_rdo_idx_list[i]] = UINT_MAX;
        }
        ats_inter_rdo_idx_num += 2;
    }

    xeve_mcpy(dist_temp, ats_inter_est_dist, sizeof(s64) * 9);
    xeve_mcpy(ats_inter_info_list_temp, ats_inter_info_list, sizeof(u8) * 9);
    for(idx = 1; idx < 1 + ats_inter_rdo_idx_num; idx++)
    {
        ats_inter_info_list[idx] = ats_inter_info_list_temp[ats_inter_rdo_idx_list[idx - 1]];
        ats_inter_est_dist[idx] = dist_temp[ats_inter_rdo_idx_list[idx - 1]];
    }
    for(idx = 1 + ats_inter_rdo_idx_num; idx < *num_rdo; idx++)
    {
        ats_inter_info_list[idx] = 255;
        ats_inter_est_dist[idx] = UINT_MAX;
    }
    *num_rdo = ats_inter_rdo_idx_num + 1;
}

u8 skip_ats_inter_by_rd_cost(XEVE_CTX *ctx, XEVE_CORE * core, s64* ats_inter_est_dist, u8* ats_inter_info_list, int curr_idx, double cost_best, s64 dist_ats_inter0, double cost_ats_inter0, u8 root_cbf_ats_inter0)
{
    //ATS_INTER fast algorithm 2.2 : estimate a minimum RD cost of a ATS_INTER mode based on the luma distortion of uncoded part and coded part (assuming distorted can be reduced to 1/16);
    //                         if this cost is larger than the best cost, no need to try a specific ATS_INTER mode
    double cost_curr_ats_inter = ats_inter_est_dist[curr_idx] + RATE_TO_COST_LAMBDA(core->lambda[0], 11);
    if(cost_curr_ats_inter > cost_best)
    {
        return 1;
    }

    if(cost_ats_inter0 != MAX_COST)
    {
        u8 ats_inter_idx = get_ats_inter_idx(ats_inter_info_list[curr_idx]);
        if(!root_cbf_ats_inter0)
        {
            //ATS_INTER fast algorithm 3: skip ATS_INTER when the residual is too small (estCost is more accurate than fast algorithm 1, counting PU mode bits)
            int weight = is_ats_inter_quad_size(ats_inter_idx) ? 6 : 9;
            s64 dist_resi_part = ((ats_inter_est_dist[0] - ats_inter_est_dist[curr_idx]) * weight) >> 4;
            //prediction info bits cost + minimum residual bits cost + estimate distortion
            double est_cost = (cost_ats_inter0 - dist_ats_inter0) + RATE_TO_COST_LAMBDA(core->lambda[0], 10) + (ats_inter_est_dist[curr_idx] + dist_resi_part);
            if(est_cost > cost_ats_inter0 || est_cost > cost_best)
            {
                return 2;
            }
        }
        else
        {
            //ATS_INTER fast algorithm 4: skip ATS_INTER when an estimated RD cost is larger than the bestCost
            double weight = is_ats_inter_quad_size(ats_inter_idx) ? 0.4 : 0.6;
            double est_cost = (cost_ats_inter0 - dist_ats_inter0) * weight + ats_inter_est_dist[curr_idx];
            if(est_cost > cost_best)
            {
                return 3;
            }
        }
    }
    return 0;
}

//save & load functions for ATS_inter
void search_ats_inter_info_saved(XEVE_CTX *ctx, XEVE_CORE *core, u32 dist_pu, int log2_cuw, int log2_cuh, int x, int y, u8* ats_inter_info_match)
{
    XEVEM_CTX *mctx = (XEVEM_CTX *)ctx;
    int posx = (x - core->x_pel) >> MIN_CU_LOG2;
    int posy = (y - core->y_pel) >> MIN_CU_LOG2;
    int widx = log2_cuw - 2;
    int hidx = log2_cuh - 2;
    int num_route = ATS_INTER_SL_NUM;
    int stride1 = MAX_TR_LOG2 - MIN_CU_LOG2 + 1;
    int stride2 = ctx->max_cuwh >> MIN_CU_LOG2;
    int stridew = stride2 * stride2 * stride1;
    int strideh = stride2 * stride2;
    int stridex = stride2;
    int offset1 = widx * stridew + hidx * strideh + posx * stridex + posy;
    int offset2 = offset1 * num_route;
    int i;
    *ats_inter_info_match = 255;

    u8  *ats_inter_num_pred = mctx->ats_inter_num_pred[core->thread_cnt];
    u32 *ats_inter_pred_dist = mctx->ats_inter_pred_dist[core->thread_cnt];
    u8  *ats_inter_info_pred = mctx->ats_inter_info_pred[core->thread_cnt];
    for(i = 0; i < ats_inter_num_pred[offset1]; i++)
    {
        if(ats_inter_pred_dist[offset2 + i] == dist_pu)
        {
            *ats_inter_info_match = ats_inter_info_pred[offset2 + i];
            break;
        }
    }
}

void save_ats_inter_info_pred(XEVE_CTX *ctx, XEVE_CORE *core, u32 dist_pu, u8 ats_inter_info_pu, int log2_cuw, int log2_cuh, int x, int y)
{
    XEVEM_CTX *mctx = (XEVEM_CTX *)ctx;
    int posx = (x - core->x_pel) >> MIN_CU_LOG2;
    int posy = (y - core->y_pel) >> MIN_CU_LOG2;
    int widx = log2_cuw - 2;
    int hidx = log2_cuh - 2;
    int num_route = ATS_INTER_SL_NUM;
    int stride1 = MAX_TR_LOG2 - MIN_CU_LOG2 + 1;
    int stride2 = ctx->max_cuwh >> MIN_CU_LOG2;
    int stridew = stride2 * stride2 * stride1;
    int strideh = stride2 * stride2;
    int stridex = stride2;
    int offset1 = widx * stridew + hidx * strideh + posx * stridex + posy;
    int offset2 = offset1 * num_route;
    u8  *ats_inter_num_pred = mctx->ats_inter_num_pred[core->thread_cnt];
    u32 *ats_inter_pred_dist = mctx->ats_inter_pred_dist[core->thread_cnt];
    u8  *ats_inter_info_pred = mctx->ats_inter_info_pred[core->thread_cnt];
    int num_data = ats_inter_num_pred[offset1];
    if(num_data < num_route)
    {
        ats_inter_info_pred[offset2 + num_data] = ats_inter_info_pu;
        ats_inter_pred_dist[offset2 + num_data] = dist_pu;
        ats_inter_num_pred[offset1]++;
    }
}


static double pinter_residue_rdo(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh, pel pred[2][N_C][MAX_CU_DIM], s16 coef[N_C][MAX_CU_DIM], int pidx, u8 *mvp_idx, BOOL apply_dmvr)
{
    XEVEM_CORE  *mcore = (XEVEM_CORE*)core;
    XEVE_PINTER *pi = &ctx->pinter[core->thread_cnt];

    s16    coef_t[N_C][MAX_CU_DIM];
    int   *nnz, tnnz, w[N_C], h[N_C], log2_w[N_C], log2_h[N_C];
    int    cuw;
    int    cuh;
    pel(*rec)[MAX_CU_DIM];
    s64    dist[2][N_C];
    double cost, cost_best = MAX_COST;
    int    cbf_idx[N_C], nnz_store[N_C];
    int    nnz_sub_store[N_C][MAX_SUB_TB_NUM] = {{0},};
    int    bit_cnt;
    int    i, idx_y, idx_u, idx_v;
    pel   *org[N_C];
    double cost_comp_best = MAX_COST;
    int    idx_best[N_C] = {0,};
    u8     is_from_mv_field = 0;
    s64    dist_no_resi[N_C];
    int    log2_tuw, log2_tuh;
    u8     ats_inter_info_best = 255;
    u8     ats_inter_info_list[9];
    int    num_rdo;
    int    nnz_best[N_C] = {-1, -1, -1};
    int    ats_inter_mode_idx;
    u8     ats_inter_avail = check_ats_inter_info_coded(1 << log2_cuw, 1 << log2_cuh, MODE_INTER, ctx->sps.tool_ats);
    s64    ats_inter_est_dist[9];
    s64    dist_ats_inter0 = UINT_MAX;
    double cost_ats_inter0 = MAX_COST;
    u8     root_cbf_ats_inter0 = 255;
    u8     ats_inter_info_match = 255;
    u8     num_rdo_tried = 0;
    s64    dist_idx = -1;
    int    w_shift = ctx->param.cs_w_shift;
    int    h_shift = ctx->param.cs_h_shift;

    get_ats_inter_info_rdo_order(core, ats_inter_avail, &num_rdo, ats_inter_info_list);
    mcore->ats_inter_info = 0;

    if(mcore->affine_flag)
    {
        pi->mvr_idx[pidx] = 0;
        pi->bi_idx[pidx] = BI_NON;
    }

    rec = pi->rec[pidx];
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
    if(ctx->sps.chroma_format_idc)
    {
        org[U_C] = pi->o[U_C] + ((y >> h_shift) * pi->s_o[U_C]) + (x >> w_shift);
        org[V_C] = pi->o[V_C] + ((y >> h_shift) * pi->s_o[V_C]) + (x >> w_shift);
    }

    if(ctx->param.rdo_dbk_switch && mcore->affine_flag)
    {
        is_from_mv_field = 1;
    }

    /* prediction */
    if(mcore->affine_flag)
    {
        xeve_affine_mc(x, y, ctx->w, ctx->h, w[0], h[0], pi->refi[pidx], pi->affine_mv[pidx], pi->refp, pred, mcore->affine_flag + 1, mcore->eif_tmp_buffer
                     , ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc);
    }
    else
    {
        pi->fn_mc(ctx, core, x, y, w[0], h[0], pi->refi[pidx], pi->mv[pidx], pi->refp, pred, ctx->poc.poc_val, apply_dmvr, pi->dmvr_mv[pidx]);
    }

    int bit_depth_tbl[3] = {ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8};
    /* get residual */
    xeve_diff_pred(x, y, log2_cuw, log2_cuh, pi->pic_o, pred[0], pi->resi
                 , ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc);
    for(i = 0; i < N_C; i++)
    {
        if(!ctx->sps.chroma_format_idc && i != 0)
            dist[0][i] = 0;
        else
        dist[0][i] = xeve_ssd_16b(log2_w[i], log2_h[i], pred[0][i], org[i], w[i], pi->s_o[i], bit_depth_tbl[i]);
        dist_no_resi[i] = dist[0][i];
    }

    //load best in history
    if(ats_inter_avail)
    {
        int shift_val = XEVE_MIN(log2_cuw + log2_cuh, 9);
        dist_idx = dist_no_resi[Y_C] + dist_no_resi[U_C] + dist_no_resi[V_C];
        dist_idx = (dist_idx + (s64)(1 << (shift_val - 1))) >> shift_val;
        search_ats_inter_info_saved(ctx, core, (u32)dist_idx, log2_cuw, log2_cuh, x, y, &ats_inter_info_match);
    }
    if(ats_inter_avail && ats_inter_info_match == 255)
    {
        calc_min_cost_ats_inter(ctx, core, pred[0], org, w, pi->s_o, ats_inter_avail, dist_no_resi, &num_rdo, ats_inter_info_list, ats_inter_est_dist);
    }

    for(ats_inter_mode_idx = 0; ats_inter_mode_idx < num_rdo; ats_inter_mode_idx++)
    {
        mcore->ats_inter_info = ats_inter_info_list[ats_inter_mode_idx];
        assert(get_ats_inter_idx(mcore->ats_inter_info) >= 0 && get_ats_inter_idx(mcore->ats_inter_info) <= 4);
        assert(get_ats_inter_pos(mcore->ats_inter_info) >= 0 && get_ats_inter_pos(mcore->ats_inter_info) <= 1);

        //early skp fast algorithm here
        if(ats_inter_info_match != 255 && mcore->ats_inter_info != ats_inter_info_match)
        {
            continue;
        }
        if(ats_inter_mode_idx > 0 && ats_inter_info_match == 255)
        {
            assert(pidx == AFF_DIR || pidx == PRED_DIR || pidx == PRED_DIR_MMVD || root_cbf_ats_inter0 != 255);
            if(skip_ats_inter_by_rd_cost(ctx, core, ats_inter_est_dist, ats_inter_info_list, ats_inter_mode_idx, core->cost_best, dist_ats_inter0, cost_ats_inter0, root_cbf_ats_inter0))
            {
                continue;
            }
        }

        //try this ATS_INTER mode
        num_rdo_tried++;

        //prepare tu residual
        copy_tu_from_cu(coef, pi->resi, log2_cuw, log2_cuh, mcore->ats_inter_info,ctx->sps.chroma_format_idc);
        if(ctx->pps.cu_qp_delta_enabled_flag)
        {
            xeve_set_qp(ctx, core, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].curr_qp);
        }

        /* transform and quantization */
        tnnz = ctx->fn_tq(ctx, core, coef, log2_cuw, log2_cuh, pi->slice_type, nnz, 0, RUN_L | RUN_CB | RUN_CR);

        if(tnnz)
        {
            for(i = 0; i < N_C; i++)
            {
                if(i != 0 && !ctx->sps.chroma_format_idc)
                {
                    cbf_idx[i] = 0;
                    nnz_store[i] = nnz[i];
                    xeve_mcpy(nnz_sub_store[i], core->nnz_sub[i], sizeof(int) * MAX_SUB_TB_NUM);
                    continue;
                }
                int size = (cuw * cuh) >> (i == 0 ? 0 : w_shift + h_shift);
                int ats_inter_idx = get_ats_inter_idx(mcore->ats_inter_info);
                size = (mcore->ats_inter_info == 0) ? size : (size >> (is_ats_inter_quad_size(ats_inter_idx) ? 2 : 1));
                xeve_mcpy(coef_t[i], coef[i], sizeof(s16) * size);

                cbf_idx[i] = 0;
                nnz_store[i] = nnz[i];
                xeve_mcpy(nnz_sub_store[i], core->nnz_sub[i], sizeof(int) * MAX_SUB_TB_NUM);
            }

            ctx->fn_itdp(ctx, core, coef_t, core->nnz_sub);

            if(ctx->param.rdo_dbk_switch && mcore->ats_inter_info == 0)
            {
                calc_delta_dist_filter_boundary(ctx, PIC_MODE(ctx), PIC_ORIG(ctx), cuw, cuh, pred[0], cuw, x, y, core->avail_lr, 0, 0, pi->refi[pidx]
                                                , pi->mv[pidx], is_from_mv_field, core);
            }

            for(i = 0; i < N_C; i++)
            {
                if(nnz[i])
                {
                    ctx->fn_recon(ctx, core, coef_t[i], pred[0][i], nnz[i], w[i], h[i], w[i], rec[i], ctx->sps.bit_depth_luma_minus8 + 8);

                    if(ctx->sps.tool_htdf == 1 && i == Y_C)
                    {
                        const int s_mod = pi->s_m[Y_C];
                        u16 avail_cu = xeve_get_avail_intra(core->x_scu, core->y_scu, ctx->w_scu, ctx->h_scu, core->scup, log2_cuw, log2_cuh, ctx->map_scu, ctx->map_tidx);

                        int constrained_intra_flag = 0 && ctx->pps.constrained_intra_pred_flag;
                        xeve_htdf(rec[i], ctx->tile[core->tile_idx].qp, cuw, cuh, cuw, FALSE, pi->m[Y_C] + (y * s_mod) + x, s_mod, avail_cu
                                , core->scup, ctx->w_scu, ctx->h_scu, ctx->map_scu, constrained_intra_flag, ctx->sps.bit_depth_luma_minus8 + 8);

                    }
                    if(!ctx->sps.chroma_format_idc && i != 0)
                        dist[1][i] = 0;
                    else
                    dist[1][i] = xeve_ssd_16b(log2_w[i], log2_h[i], rec[i], org[i], w[i], pi->s_o[i], bit_depth_tbl[i]);
                }
                else
                {
                    dist[1][i] = dist_no_resi[i];
                }
                if(ctx->param.rdo_dbk_switch && mcore->ats_inter_info == 0)
                {
                    dist[0][i] += core->delta_dist[i];
                }
            }

            if(ctx->param.rdo_dbk_switch)
            {
                //complete rec
                for(i = 0; i < N_C; i++)
                {
                    if(nnz[i] == 0)
                    {
                        ctx->fn_recon(ctx, core, coef_t[i], pred[0][i], nnz[i], w[i], h[i], w[i], rec[i], ctx->sps.bit_depth_luma_minus8 + 8);
                    }
                }
                //filter rec and calculate ssd
                calc_delta_dist_filter_boundary(ctx, PIC_MODE(ctx), PIC_ORIG(ctx), cuw, cuh, rec, cuw, x, y, core->avail_lr, 0
                                                , nnz[Y_C] != 0, pi->refi[pidx], pi->mv[pidx], is_from_mv_field, core);
                for(i = 0; i < N_C; i++)
                {
                    dist[1][i] += core->delta_dist[i];
                    if(i != 0 && !ctx->sps.chroma_format_idc)
                        dist[1][i] = 0;
                }
            }

            if(pidx != AFF_DIR && pidx != PRED_DIR_MMVD && pidx != PRED_DIR && mcore->ats_inter_info == 0)
            {
                /* test all zero case */
                idx_y = 0;
                idx_u = 0;
                idx_v = 0;
                nnz[Y_C] = 0;
                nnz[U_C] = 0;
                nnz[V_C] = 0;
                xeve_mset(core->nnz_sub, 0, sizeof(int) * N_C * MAX_SUB_TB_NUM);

                cost = (double)dist[idx_y][Y_C] + (((double)dist[idx_u][U_C] * core->dist_chroma_weight[0]) + ((double)dist[idx_v][V_C] * core->dist_chroma_weight[1]));

                SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
                DQP_LOAD(core->dqp_temp_run, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);

                xeve_sbac_bit_reset(&core->s_temp_run);

                if(IS_INTER_SLICE(ctx->sh->slice_type) && REFI_IS_VALID(pi->refi[pidx][REFP_0]))
                {
                    pi->mvd[pidx][REFP_0][MV_X] >>= pi->mvr_idx[pidx];
                    pi->mvd[pidx][REFP_0][MV_Y] >>= pi->mvr_idx[pidx];
                }

                if(ctx->sh->slice_type == SLICE_B && REFI_IS_VALID(pi->refi[pidx][REFP_1]))
                {
                    pi->mvd[pidx][REFP_1][MV_X] >>= pi->mvr_idx[pidx];
                    pi->mvd[pidx][REFP_1][MV_Y] >>= pi->mvr_idx[pidx];
                }

                xeve_rdo_bit_cnt_cu_inter_main(ctx, core, ctx->sh->slice_type, core->scup, pi->refi[pidx], pi->mvd[pidx], coef, pidx, mvp_idx, pi->mvr_idx[pidx], pi->bi_idx[pidx], pi->affine_mvd[pidx]);

                if(IS_INTER_SLICE(ctx->sh->slice_type) && REFI_IS_VALID(pi->refi[pidx][REFP_0]))
                {
                    pi->mvd[pidx][REFP_0][MV_X] <<= pi->mvr_idx[pidx];
                    pi->mvd[pidx][REFP_0][MV_Y] <<= pi->mvr_idx[pidx];
                }
                if(ctx->sh->slice_type == SLICE_B && REFI_IS_VALID(pi->refi[pidx][REFP_1]))
                {
                    pi->mvd[pidx][REFP_1][MV_X] <<= pi->mvr_idx[pidx];
                    pi->mvd[pidx][REFP_1][MV_Y] <<= pi->mvr_idx[pidx];
                }

                bit_cnt = xeve_get_bit_number(&core->s_temp_run);
                cost += RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt);

                if(cost < cost_best)
                {
                    cost_best = cost;
                    cbf_idx[Y_C] = idx_y;
                    cbf_idx[U_C] = idx_u;
                    cbf_idx[V_C] = idx_v;
                    SBAC_STORE(core->s_temp_best, core->s_temp_run);
                    DQP_STORE(core->dqp_temp_best, core->dqp_temp_run);
                    ats_inter_info_best = mcore->ats_inter_info;
                    core->cost_best = cost < core->cost_best ? cost : core->cost_best;
                    if(ats_inter_mode_idx == 0)
                    {
                        dist_ats_inter0 = (s64)(cost_best - RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt));
                        cost_ats_inter0 = cost_best;
                        root_cbf_ats_inter0 = 0;
                    }
                }
            } // forced zero

            /* test as it is */
            idx_y = nnz_store[Y_C] > 0 ? 1 : 0;
            idx_u = nnz_store[U_C] > 0 ? 1 : 0;
            idx_v = nnz_store[V_C] > 0 ? 1 : 0;
            nnz[Y_C] = nnz_store[Y_C];
            nnz[U_C] = nnz_store[U_C];
            nnz[V_C] = nnz_store[V_C];
            xeve_mcpy(core->nnz_sub, nnz_sub_store, sizeof(int) * N_C * MAX_SUB_TB_NUM);

            cost = (double)dist[idx_y][Y_C] + (((double)dist[idx_u][U_C] * core->dist_chroma_weight[0]) + ((double)dist[idx_v][V_C] * core->dist_chroma_weight[1]));

            SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
            DQP_LOAD(core->dqp_temp_run, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);

            xeve_sbac_bit_reset(&core->s_temp_run);

            if(IS_INTER_SLICE(ctx->sh->slice_type) && REFI_IS_VALID(pi->refi[pidx][REFP_0]))
            {
                pi->mvd[pidx][REFP_0][MV_X] >>= pi->mvr_idx[pidx];
                pi->mvd[pidx][REFP_0][MV_Y] >>= pi->mvr_idx[pidx];
            }
            if(ctx->sh->slice_type == SLICE_B && REFI_IS_VALID(pi->refi[pidx][REFP_1]))
            {
                pi->mvd[pidx][REFP_1][MV_X] >>= pi->mvr_idx[pidx];
                pi->mvd[pidx][REFP_1][MV_Y] >>= pi->mvr_idx[pidx];
            }

            xeve_rdo_bit_cnt_cu_inter_main(ctx, core, ctx->sh->slice_type, core->scup, pi->refi[pidx], pi->mvd[pidx], coef, pidx, mvp_idx, pi->mvr_idx[pidx], pi->bi_idx[pidx], pi->affine_mvd[pidx]);

            if(IS_INTER_SLICE(ctx->sh->slice_type) && REFI_IS_VALID(pi->refi[pidx][REFP_0]))
            {
                pi->mvd[pidx][REFP_0][MV_X] <<= pi->mvr_idx[pidx];
                pi->mvd[pidx][REFP_0][MV_Y] <<= pi->mvr_idx[pidx];
            }
            if(ctx->sh->slice_type == SLICE_B && REFI_IS_VALID(pi->refi[pidx][REFP_1]))
            {
                pi->mvd[pidx][REFP_1][MV_X] <<= pi->mvr_idx[pidx];
                pi->mvd[pidx][REFP_1][MV_Y] <<= pi->mvr_idx[pidx];
            }

            bit_cnt = xeve_get_bit_number(&core->s_temp_run);
            cost += RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt);

            if(cost < cost_best)
            {
                cost_best = cost;
                cbf_idx[Y_C] = idx_y;
                cbf_idx[U_C] = idx_u;
                cbf_idx[V_C] = idx_v;
                SBAC_STORE(core->s_temp_best, core->s_temp_run);
                DQP_STORE(core->dqp_temp_best, core->dqp_temp_run);
                ats_inter_info_best = mcore->ats_inter_info;
                core->cost_best = cost < core->cost_best ? cost : core->cost_best;
                if(ats_inter_mode_idx == 0)
                {
                    dist_ats_inter0 = (s64)(cost_best - RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt));
                    cost_ats_inter0 = cost_best;
                    root_cbf_ats_inter0 = (idx_y + idx_u + idx_v) != 0;
                }
            }

            for (i = 0; i < N_C; i++)
            {
                nnz[i] = (cbf_idx[i] ? nnz_store[i] : 0);
                if (cbf_idx[i])
                {
                    xeve_mcpy(core->nnz_sub[i], nnz_sub_store[i], sizeof(int) * MAX_SUB_TB_NUM);
                }
                else
                {
                    xeve_mset(core->nnz_sub[i], 0, sizeof(int) * MAX_SUB_TB_NUM);
                }
                if (nnz[i] == 0 && nnz_store[i] != 0)
                {
                    xeve_mset(core->nnz_sub[i], 0, sizeof(int) * MAX_SUB_TB_NUM);
                    xeve_mset(coef[i], 0, sizeof(s16) * ((cuw * cuh) >> (i == 0 ? 0 : w_shift + h_shift)));
                }
            }

            //save the best coeff
            if(ats_inter_info_best == mcore->ats_inter_info && ats_inter_avail)
            {
                for(i = 0; i < N_C; i++)
                {
                    nnz_best[i] = nnz[i];
                    if(nnz[i] > 0)
                    {
                        xeve_mcpy(pi->coff_save[i], coef[i], sizeof(s16) * ((cuw * cuh) >> (i == 0 ? 0 : w_shift + h_shift)));
                    }
                }
            }
        }
        else
        {
            if(ctx->pps.cu_qp_delta_enabled_flag)
            {
                if(core->cu_qp_delta_code_mode != 2)
                {
                    xeve_set_qp(ctx, core, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].prev_qp);
                }
            }

            if(ctx->sps.tool_admvp == 1 && (pidx == AFF_DIR || pidx == PRED_DIR_MMVD || pidx == PRED_DIR))
            {
                if(ats_inter_info_match != 0 && ats_inter_info_match != 255 && mcore->ats_inter_info)
                {
                    return MAX_COST;
                }
                continue;
            }

            mcore->ats_inter_info = 0;
            if(cost_best != MAX_COST)
            {
                continue;
            }

            for(i = 0; i < N_C; i++)
            {
                nnz[i] = 0;
                xeve_mset(core->nnz_sub[i], 0, sizeof(int) * MAX_SUB_TB_NUM);
            }
            if(ctx->param.rdo_dbk_switch)
            {
                calc_delta_dist_filter_boundary(ctx, PIC_MODE(ctx), PIC_ORIG(ctx), cuw, cuh, pred[0], cuw, x, y, core->avail_lr, 0, 0
                                                , pi->refi[pidx], pi->mv[pidx], is_from_mv_field, core);
            }
            for(i = 0; i < N_C; i++)
            {
                dist[0][i] = dist_no_resi[i];
                if(ctx->param.rdo_dbk_switch)
                {
                    dist[0][i] += core->delta_dist[i];
                }
                if(i != 0 && !ctx->sps.chroma_format_idc)
                    dist[0][i] = 0;
            }
            cost_best = (double)dist[0][Y_C] + (core->dist_chroma_weight[0] * (double)dist[0][U_C]) + (core->dist_chroma_weight[1] * (double)dist[0][V_C]);

            SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
            DQP_LOAD(core->dqp_temp_run, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);

            xeve_sbac_bit_reset(&core->s_temp_run);

            if(IS_INTER_SLICE(ctx->sh->slice_type) && REFI_IS_VALID(pi->refi[pidx][REFP_0]))
            {
                pi->mvd[pidx][REFP_0][MV_X] >>= pi->mvr_idx[pidx];
                pi->mvd[pidx][REFP_0][MV_Y] >>= pi->mvr_idx[pidx];
            }
            if(ctx->sh->slice_type == SLICE_B && REFI_IS_VALID(pi->refi[pidx][REFP_1]))
            {
                pi->mvd[pidx][REFP_1][MV_X] >>= pi->mvr_idx[pidx];
                pi->mvd[pidx][REFP_1][MV_Y] >>= pi->mvr_idx[pidx];
            }

            xeve_rdo_bit_cnt_cu_inter_main(ctx, core, ctx->sh->slice_type, core->scup, pi->refi[pidx], pi->mvd[pidx], coef, pidx, mvp_idx, pi->mvr_idx[pidx], pi->bi_idx[pidx], pi->affine_mvd[pidx]);

            if(IS_INTER_SLICE(ctx->sh->slice_type) && REFI_IS_VALID(pi->refi[pidx][REFP_0]))
            {
                pi->mvd[pidx][REFP_0][MV_X] <<= pi->mvr_idx[pidx];
                pi->mvd[pidx][REFP_0][MV_Y] <<= pi->mvr_idx[pidx];
            }
            if(ctx->sh->slice_type == SLICE_B && REFI_IS_VALID(pi->refi[pidx][REFP_1]))
            {
                pi->mvd[pidx][REFP_1][MV_X] <<= pi->mvr_idx[pidx];
                pi->mvd[pidx][REFP_1][MV_Y] <<= pi->mvr_idx[pidx];
            }

            bit_cnt = xeve_get_bit_number(&core->s_temp_run);
            cost_best += RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt);
            SBAC_STORE(core->s_temp_best, core->s_temp_run);
            DQP_STORE(core->dqp_temp_best, core->dqp_temp_run);
            assert(mcore->ats_inter_info == 0);
            ats_inter_info_best = mcore->ats_inter_info;
            nnz_best[Y_C] = nnz_best[U_C] = nnz_best[V_C] = 0;
            core->cost_best = cost_best < core->cost_best ? cost_best : core->cost_best;
            if(ats_inter_mode_idx == 0)
            {
                dist_ats_inter0 = (s64)(cost_best - (s64)RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt));
                cost_ats_inter0 = cost_best;
                root_cbf_ats_inter0 = 0;
            }
        }
    }

    if(ats_inter_avail)
    {
        assert(log2_cuw <= MAX_TR_LOG2 && log2_cuh <= MAX_TR_LOG2);

        if(ctx->sps.tool_admvp == 1 && (pidx == AFF_DIR || pidx == PRED_DIR_MMVD || pidx == PRED_DIR))
        {
            if(nnz_best[Y_C] + nnz_best[U_C] + nnz_best[V_C] <= 0)
            {
                mcore->ats_inter_info = 0;
                return MAX_COST;
            }
        }

        //if no residual, the best mode shall not be ATS_INTER mode
        ats_inter_info_best = (nnz_best[Y_C] + nnz_best[U_C] + nnz_best[V_C] == 0) ? 0 : ats_inter_info_best;
        assert(cost_best != MAX_COST);
        assert(ats_inter_info_best != 255);
        mcore->ats_inter_info = ats_inter_info_best;
        get_tu_size(mcore->ats_inter_info, log2_cuw, log2_cuh, &log2_tuw, &log2_tuh);
        for(i = 0; i < N_C; i++)
        {
            int tuw = 1 << log2_tuw;
            int tuh = 1 << log2_tuh;
            assert(nnz_best[i] != -1);
            core->nnz_sub[i][0] = nnz[i] = nnz_best[i];
            if(nnz[i] > 0)
            {
                xeve_mcpy(coef[i], pi->coff_save[i], sizeof(s16) * ((tuw * tuh) >> (i == 0 ? 0 : (i == 0 ? 0 : w_shift + h_shift))));
            }
            else
            {
                xeve_mset(coef[i], 0, sizeof(s16) * ((cuw * cuh) >> (i == 0 ? 0 : (i == 0 ? 0 : w_shift + h_shift)))); //not necessary
            }
        }
        //save the best to history memory
        if(ats_inter_info_match == 255 && num_rdo_tried > 1)
        {
            assert(dist_idx != -1);
            save_ats_inter_info_pred(ctx, core, (u32)dist_idx, ats_inter_info_best, log2_cuw, log2_cuh, x, y);
        }
    }

    return cost_best;
}

static void get_mmvd_mvp_list(s8(*map_refi)[REFP_NUM], XEVE_REFP refp[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], int w_scu, int h_scu, int scup, u16 avail, int log2_cuw, int log2_cuh, int slice_t
                            , int real_mv[][2][3], u32 *map_scu, int REF_SET[][XEVE_MAX_NUM_ACTIVE_REF_FRAME], u16 avail_lr
                            , u32 curr_ptr, u8 num_refp[REFP_NUM]
                            , XEVE_HISTORY_BUFFER *history_buffer, int admvp_flag, XEVE_SH* sh, int log2_max_cuwh, u8* map_tidx)
{
    int ref_mvd = 0;
    int ref_mvd1 = 0;
    int list0_weight;
    int list1_weight;
    int ref_sign = 0;
    int ref_sign1 = 0;
    int ref_mvd_cands[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
    int hor0var[MMVD_MAX_REFINE_NUM] = { 0 };
    int ver0var[MMVD_MAX_REFINE_NUM] = { 0 };
    int hor1var[MMVD_MAX_REFINE_NUM] = { 0 };
    int ver1Var[MMVD_MAX_REFINE_NUM] = { 0 };
    int base_mv_idx = 0;
    int base_mv[25][2][3];
    s16 smvp[REFP_NUM][MAX_NUM_MVP][MV_D];
    s8 srefi[REFP_NUM][MAX_NUM_MVP];
    int base_mv_t[25][2][3];
    int base_type[3][MAX_NUM_MVP];
    int cur_set;
    int total_num = MMVD_BASE_MV_NUM * MMVD_MAX_REFINE_NUM;
    int k;
    int cuw = (1 << log2_cuw);
    int cuh = (1 << log2_cuh);
    int list0_r;
    int list1_r;
    int poc0, poc1, poc_c;

    int base_mv_p[25][3][3];
    int small_cu = (cuw*cuh <= NUM_SAMPLES_BLOCK) ? 1 : 0;

    int base_st = 0;
    int base_ed = MMVD_BASE_MV_NUM;

    int group_st = 0;
    int group_ed =  (small_cu ? 1 : 3);

    int mmvd_v_st = 0;
    int mmvd_v_ed = MMVD_MAX_REFINE_NUM;

    if (admvp_flag == 0)
    {
        xeve_get_motion_skip(slice_t, scup, map_refi, map_mv, refp, cuw, cuh, w_scu, srefi, smvp, avail);
    }
    else
    {
        xevem_get_motion_merge(curr_ptr, slice_t, scup, map_refi, map_mv, refp, cuw, cuh, w_scu, h_scu, srefi, smvp, map_scu, avail_lr
                                 , NULL, history_buffer, 0, (XEVE_REFP(*)[2])refp, sh, log2_max_cuwh, map_tidx);
    }

    if (slice_t == SLICE_B)
    {
        for (k = base_st; k < base_ed; k++)
        {
            base_mv[k][REFP_0][MV_X] = smvp[REFP_0][k][MV_X];
            base_mv[k][REFP_0][MV_Y] = smvp[REFP_0][k][MV_Y];
            base_mv[k][REFP_1][MV_X] = smvp[REFP_1][k][MV_X];
            base_mv[k][REFP_1][MV_Y] = smvp[REFP_1][k][MV_Y];
            base_mv[k][REFP_0][REFI] = srefi[REFP_0][k];
            base_mv[k][REFP_1][REFI] = srefi[REFP_1][k];
        }
    }
    else
    {
        for (k = base_st; k < base_ed; k++)
        {
            base_mv[k][REFP_0][MV_X] = smvp[REFP_0][k][MV_X];
            base_mv[k][REFP_0][MV_Y] = smvp[REFP_0][k][MV_Y];
            base_mv[k][REFP_1][MV_X] = smvp[REFP_1][0][MV_X];
            base_mv[k][REFP_1][MV_Y] = smvp[REFP_1][0][MV_Y];
            base_mv[k][REFP_0][REFI] = srefi[REFP_0][k];
            base_mv[k][REFP_1][REFI] = srefi[REFP_1][0];
        }
    }

    for (k = base_st; k < base_ed; k++)
    {
        ref_sign = 1;
        ref_sign1 = 1;

        base_mv_t[k][REFP_0][MV_X] = base_mv[k][REFP_0][MV_X];
        base_mv_t[k][REFP_0][MV_Y] = base_mv[k][REFP_0][MV_Y];
        base_mv_t[k][REFP_0][REFI] = base_mv[k][REFP_0][REFI];

        base_mv_t[k][REFP_1][MV_X] = base_mv[k][REFP_1][MV_X];
        base_mv_t[k][REFP_1][MV_Y] = base_mv[k][REFP_1][MV_Y];
        base_mv_t[k][REFP_1][REFI] = base_mv[k][REFP_1][REFI];

        list0_r = base_mv_t[k][REFP_0][REFI];
        list1_r = base_mv_t[k][REFP_1][REFI];

        if ((base_mv_t[k][REFP_0][REFI] != REFI_INVALID) && (base_mv_t[k][REFP_1][REFI] != REFI_INVALID))
        {
            base_type[0][k] = 0;
            base_type[1][k] = 1;
            base_type[2][k] = 2;
        }
        else if ((base_mv_t[k][REFP_0][REFI] != REFI_INVALID) && (base_mv_t[k][REFP_1][REFI] == REFI_INVALID))
        {
            if (slice_t == SLICE_P)
            {
                int cur_ref_num = num_refp[REFP_0];
                base_type[0][k] = 1;
                base_type[1][k] = 1;
                base_type[2][k] = 1;

                if (cur_ref_num == 1)
                {
                    base_mv_p[k][0][REFI] = base_mv_t[k][REFP_0][REFI];
                    base_mv_p[k][1][REFI] = base_mv_t[k][REFP_0][REFI];
                    base_mv_p[k][2][REFI] = base_mv_t[k][REFP_0][REFI];
                }
                else
                {
                    base_mv_p[k][0][REFI] = base_mv_t[k][REFP_0][REFI];
                    base_mv_p[k][1][REFI] = !base_mv_t[k][REFP_0][REFI];
                    if (cur_ref_num < 3)
                    {
                        base_mv_p[k][2][REFI] = base_mv_t[k][REFP_0][REFI];
                    }
                    else
                    {
                        base_mv_p[k][2][REFI] = base_mv_t[k][REFP_0][REFI] < 2 ? 2 : 1;
                    }
                }

                if (cur_ref_num == 1)
                {
                    base_mv_p[k][0][MV_X] = base_mv_t[k][REFP_0][MV_X];
                    base_mv_p[k][0][MV_Y] = base_mv_t[k][REFP_0][MV_Y];

                    base_mv_p[k][1][MV_X] = base_mv_t[k][REFP_0][MV_X] + 3;
                    base_mv_p[k][1][MV_Y] = base_mv_t[k][REFP_0][MV_Y];

                    base_mv_p[k][2][MV_X] = base_mv_t[k][REFP_0][MV_X] - 3;
                    base_mv_p[k][2][MV_Y] = base_mv_t[k][REFP_0][MV_Y];
                }
                else if (cur_ref_num == 2)
                {
                    base_mv_p[k][0][MV_X] = base_mv_t[k][REFP_0][MV_X];
                    base_mv_p[k][0][MV_Y] = base_mv_t[k][REFP_0][MV_Y];

                    poc0 = REF_SET[0][base_mv_p[k][0][REFI]];
                    poc_c = curr_ptr;
                    poc1 = REF_SET[0][base_mv_p[k][1][REFI]];

                    list0_weight = ((poc_c - poc0) << MVP_SCALING_PRECISION) / ((poc_c - poc1));
                    ref_sign = 1;
                    base_mv_p[k][1][MV_X] = XEVE_CLIP3(-32768, 32767, ref_sign  * ((XEVE_ABS(list0_weight * base_mv_t[k][REFP_0][MV_X]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));
                    base_mv_p[k][1][MV_Y] = XEVE_CLIP3(-32768, 32767, ref_sign1 * ((XEVE_ABS(list0_weight * base_mv_t[k][REFP_0][MV_Y]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));
                    base_mv_p[k][2][MV_X] = base_mv_t[k][REFP_0][MV_X] - 3;
                    base_mv_p[k][2][MV_Y] = base_mv_t[k][REFP_0][MV_Y];
                }
                else if (cur_ref_num >= 3)
                {
                    base_mv_p[k][0][MV_X] = base_mv_t[k][REFP_0][MV_X];
                    base_mv_p[k][0][MV_Y] = base_mv_t[k][REFP_0][MV_Y];

                    poc0 = REF_SET[0][base_mv_p[k][0][REFI]];
                    poc_c = curr_ptr;
                    poc1 = REF_SET[0][base_mv_p[k][1][REFI]];

                    list0_weight = ((poc_c - poc0) << MVP_SCALING_PRECISION) / ((poc_c - poc1));
                    ref_sign = 1;
                    base_mv_p[k][1][MV_X] = XEVE_CLIP3(-32768, 32767, ref_sign  * ((XEVE_ABS(list0_weight * base_mv_t[k][REFP_0][MV_X]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));
                    base_mv_p[k][1][MV_Y] = XEVE_CLIP3(-32768, 32767, ref_sign1 * ((XEVE_ABS(list0_weight * base_mv_t[k][REFP_0][MV_Y]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));

                    poc0 = REF_SET[0][base_mv_p[k][0][2]];
                    poc_c = curr_ptr;
                    poc1 = REF_SET[0][base_mv_p[k][2][2]];

                    list0_weight = ((poc_c - poc0) << MVP_SCALING_PRECISION) / ((poc_c - poc1));
                    ref_sign = 1;
                    base_mv_p[k][2][MV_X] = XEVE_CLIP3(-32768, 32767, ref_sign  * ((XEVE_ABS(list0_weight * base_mv_t[k][REFP_0][MV_X]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));
                    base_mv_p[k][2][MV_Y] = XEVE_CLIP3(-32768, 32767, ref_sign1 * ((XEVE_ABS(list0_weight * base_mv_t[k][REFP_0][MV_Y]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));
                }
            }
            else
            {
                base_type[0][k] = 1;
                base_type[1][k] = 0;
                base_type[2][k] = 2;

                list0_weight = 1 << MVP_SCALING_PRECISION;
                list1_weight = 1 << MVP_SCALING_PRECISION;
                poc0 = REF_SET[REFP_0][list0_r];
                poc_c = curr_ptr;
                if ((num_refp[REFP_1] > 1) && ((REF_SET[REFP_1][1] - poc_c) == (poc_c - poc0)))
                {
                    base_mv_t[k][REFP_1][REFI] = 1;
                }
                else
                {
                    base_mv_t[k][REFP_1][REFI] = 0;
                }
                poc1 = REF_SET[REFP_1][base_mv_t[k][REFP_1][REFI]];

                list1_weight = ((poc_c - poc1) << MVP_SCALING_PRECISION) / ((poc_c - poc0));
                if ((list1_weight * base_mv_t[k][0][0]) < 0)
                {
                    ref_sign = -1;
                }

                base_mv_t[k][REFP_1][MV_X] = XEVE_CLIP3(-32768, 32767, ref_sign * ((XEVE_ABS(list1_weight * base_mv_t[k][REFP_0][MV_X]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));

                list1_weight = ((poc_c - poc1) << MVP_SCALING_PRECISION) / ((poc_c - poc0));
                if ((list1_weight * base_mv_t[k][0][1]) < 0)
                {
                    ref_sign1 = -1;
                }

                base_mv_t[k][REFP_1][MV_Y] = XEVE_CLIP3(-32768, 32767, ref_sign1 * ((XEVE_ABS(list1_weight * base_mv_t[k][REFP_0][MV_Y]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));
            }
        }
        else if ((base_mv_t[k][REFP_0][REFI] == REFI_INVALID) && (base_mv_t[k][REFP_1][REFI] != REFI_INVALID))
        {
            base_type[0][k] = 2;
            base_type[1][k] = 0;
            base_type[2][k] = 1;

            list0_weight = 1 << MVP_SCALING_PRECISION;
            list1_weight = 1 << MVP_SCALING_PRECISION;
            poc1 = REF_SET[1][list1_r];
            poc_c = curr_ptr;
            if ((num_refp[REFP_0] > 1) && ((REF_SET[REFP_0][1] - poc_c) == (poc_c - poc1)))
            {
                base_mv_t[k][REFP_0][REFI] = 1;
            }
            else
            {
                base_mv_t[k][REFP_0][REFI] = 0;
            }
            poc0 = REF_SET[REFP_0][base_mv_t[k][REFP_0][REFI]];

            list0_weight = ((poc_c - poc0) << MVP_SCALING_PRECISION) / ((poc_c - poc1));
            if ((list0_weight * base_mv_t[k][REFP_1][MV_X]) < 0)
            {
                ref_sign = -1;
            }
            base_mv_t[k][REFP_0][MV_X] = XEVE_CLIP3(-32768, 32767, ref_sign * ((XEVE_ABS(list0_weight * base_mv_t[k][REFP_1][MV_X]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));

            list0_weight = ((poc_c - poc0) << MVP_SCALING_PRECISION) / ((poc_c - poc1));
            if ((list0_weight * base_mv_t[k][REFP_1][MV_Y]) < 0)
            {
                ref_sign1 = -1;
            }
            base_mv_t[k][REFP_0][MV_Y] = XEVE_CLIP3(-32768, 32767, ref_sign1 * ((XEVE_ABS(list0_weight * base_mv_t[k][REFP_1][MV_Y]) + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION));
        }
        else
        {
            base_type[0][k] = 3;
            base_type[1][k] = 3;
            base_type[2][k] = 3;
        }
    }

    for (base_mv_idx = base_st; base_mv_idx < base_ed; base_mv_idx++)
    {
        int list0_r, list1_r;
        int poc0, poc1, poc_c;

        if (small_cu)
        {
            base_type[0][base_mv_idx] = 1;
        }

        for (cur_set = group_st; cur_set < group_ed; cur_set++)
        {
            if (base_type[cur_set][base_mv_idx] == 0)
            {
                base_mv[base_mv_idx][REFP_0][MV_X] = base_mv_t[base_mv_idx][REFP_0][MV_X];
                base_mv[base_mv_idx][REFP_0][MV_Y] = base_mv_t[base_mv_idx][REFP_0][MV_Y];
                base_mv[base_mv_idx][REFP_0][REFI] = base_mv_t[base_mv_idx][REFP_0][REFI];

                base_mv[base_mv_idx][REFP_1][MV_X] = base_mv_t[base_mv_idx][REFP_1][MV_X];
                base_mv[base_mv_idx][REFP_1][MV_Y] = base_mv_t[base_mv_idx][REFP_1][MV_Y];
                base_mv[base_mv_idx][REFP_1][REFI] = base_mv_t[base_mv_idx][REFP_1][REFI];
            }
            else if (base_type[cur_set][base_mv_idx] == 1)
            {
                if (slice_t == SLICE_P)
                {
                    base_mv[base_mv_idx][REFP_0][REFI] = base_mv_p[base_mv_idx][cur_set][REFI];
                    base_mv[base_mv_idx][REFP_1][REFI] = -1;

                    base_mv[base_mv_idx][REFP_0][MV_X] = base_mv_p[base_mv_idx][cur_set][MV_X];
                    base_mv[base_mv_idx][REFP_0][MV_Y] = base_mv_p[base_mv_idx][cur_set][MV_Y];
                }
                else
                {
                    base_mv[base_mv_idx][REFP_0][REFI] = base_mv_t[base_mv_idx][REFP_0][REFI];
                    base_mv[base_mv_idx][REFP_1][REFI] = -1;

                    base_mv[base_mv_idx][REFP_0][MV_X] = base_mv_t[base_mv_idx][REFP_0][MV_X];
                    base_mv[base_mv_idx][REFP_0][MV_Y] = base_mv_t[base_mv_idx][REFP_0][MV_Y];
                }
            }
            else if (base_type[cur_set][base_mv_idx] == 2)
            {
                base_mv[base_mv_idx][REFP_0][REFI] = -1;
                base_mv[base_mv_idx][REFP_1][REFI] = base_mv_t[base_mv_idx][REFP_1][REFI];

                base_mv[base_mv_idx][REFP_1][MV_X] = base_mv_t[base_mv_idx][REFP_1][MV_X];
                base_mv[base_mv_idx][REFP_1][MV_Y] = base_mv_t[base_mv_idx][REFP_1][MV_Y];
            }
            else if (base_type[cur_set][base_mv_idx] == 3)
            {
                base_mv[base_mv_idx][REFP_0][REFI] = -1;
                base_mv[base_mv_idx][REFP_1][REFI] = -1;
            }

            list0_r = base_mv[base_mv_idx][REFP_0][REFI];
            list1_r = base_mv[base_mv_idx][REFP_1][REFI];

            ref_sign = 1;
            if (slice_t == SLICE_B)
            {
                if ((list0_r != -1) && (list1_r != -1))
                {
                    poc0 = REF_SET[0][list0_r];
                    poc1 = REF_SET[1][list1_r];
                    poc_c = curr_ptr;
                    if ((poc0 - poc_c) * (poc_c - poc1) > 0)
                    {
                        ref_sign = -1;
                    }
                }
            }

            for (k = mmvd_v_st; k < mmvd_v_ed; k++)
            {
                list0_weight = 1 << MVP_SCALING_PRECISION;
                list1_weight = 1 << MVP_SCALING_PRECISION;
                ref_mvd = ref_mvd_cands[(int)(k / 4)];
                ref_mvd1 = ref_mvd_cands[(int)(k / 4)];

                if ((list0_r != -1) && (list1_r != -1))
                {
                    poc0 = REF_SET[0][list0_r];
                    poc1 = REF_SET[1][list1_r];
                    poc_c = curr_ptr;

                    if (XEVE_ABS(poc1 - poc_c) >= XEVE_ABS(poc0 - poc_c))
                    {
                        list0_weight = (XEVE_ABS(poc0 - poc_c) << MVP_SCALING_PRECISION) / (XEVE_ABS(poc1 - poc_c));
                        ref_mvd = XEVE_CLIP3(-32768, 32767, (list0_weight * ref_mvd_cands[(int)(k / 4)] + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION);
                    }
                    else
                    {
                        list1_weight = (XEVE_ABS(poc1 - poc_c) << MVP_SCALING_PRECISION) / (XEVE_ABS(poc0 - poc_c));
                        ref_mvd1 = XEVE_CLIP3(-32768, 32767, (list1_weight * ref_mvd_cands[(int)(k / 4)] + (1 << (MVP_SCALING_PRECISION - 1))) >> MVP_SCALING_PRECISION);
                    }

                    ref_mvd = XEVE_CLIP3(-(1 << 15), (1 << 15) - 1, ref_mvd);
                    ref_mvd1 = XEVE_CLIP3(-(1 << 15), (1 << 15) - 1, ref_mvd1);
                }

                if ((k % 4) == 0)
                {
                    hor0var[k] = ref_mvd;
                    hor1var[k] = ref_mvd1 * ref_sign;
                    ver0var[k] = 0;
                    ver1Var[k] = 0;
                }
                else if ((k % 4) == 1)
                {
                    hor0var[k] = ref_mvd * -1;
                    hor1var[k] = ref_mvd1 * -1 * ref_sign;
                    ver0var[k] = 0;
                    ver1Var[k] = 0;
                }
                else if ((k % 4) == 2)
                {
                    hor0var[k] = 0;
                    hor1var[k] = 0;
                    ver0var[k] = ref_mvd;
                    ver1Var[k] = ref_mvd1 * ref_sign;
                }
                else
                {
                    hor0var[k] = 0;
                    hor1var[k] = 0;
                    ver0var[k] = ref_mvd * -1;
                    ver1Var[k] = ref_mvd1 * -1 * ref_sign;
                }

                real_mv[cur_set*total_num + base_mv_idx * MMVD_MAX_REFINE_NUM + k][REFP_0][MV_X] = base_mv[base_mv_idx][REFP_0][MV_X] + hor0var[k];
                real_mv[cur_set*total_num + base_mv_idx * MMVD_MAX_REFINE_NUM + k][REFP_0][MV_Y] = base_mv[base_mv_idx][REFP_0][MV_Y] + ver0var[k];
                real_mv[cur_set*total_num + base_mv_idx * MMVD_MAX_REFINE_NUM + k][REFP_1][MV_X] = base_mv[base_mv_idx][REFP_1][MV_X] + hor1var[k];
                real_mv[cur_set*total_num + base_mv_idx * MMVD_MAX_REFINE_NUM + k][REFP_1][MV_Y] = base_mv[base_mv_idx][REFP_1][MV_Y] + ver1Var[k];

                real_mv[cur_set*total_num + base_mv_idx * MMVD_MAX_REFINE_NUM + k][REFP_0][REFI] = base_mv[base_mv_idx][REFP_0][REFI];
                real_mv[cur_set*total_num + base_mv_idx * MMVD_MAX_REFINE_NUM + k][REFP_1][REFI] = base_mv[base_mv_idx][REFP_1][REFI];
            }
        }
    }
}

static void mmvd_base_skip(XEVE_CTX *ctx, XEVE_CORE *core, int real_mv[][2][3], int log2_cuw, int log2_cuh, int slice_t, int scup
                           , s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], XEVE_REFP refp[REFP_NUM], int w_scu, u16 avail, int REF_SET[][XEVE_MAX_NUM_ACTIVE_REF_FRAME]
                           , int h_scu, u32 *map_scu, u16 avail_lr, XEVE_HISTORY_BUFFER *history_buffer, int admvp_flag, XEVE_SH* sh, int log2_max_cuwh
                           , u32 curr_ptr)
{
    int nn;
    int k;
    int base_skip[MMVD_BASE_MV_NUM];
    int base_mv[25][2][3];
    int cuw = (1 << log2_cuw);
    int cuh = (1 << log2_cuh);
    int small_cu = 0;
    int c_num = 0;
    int c_win = 0;
    s8 srefi[REFP_NUM][MAX_NUM_MVP];
    s16 smvp[REFP_NUM][MAX_NUM_MVP][MV_D];
    int cur_num;
    int dev0;
    int t_base_num = MMVD_MAX_REFINE_NUM * MMVD_BASE_MV_NUM;

    if(cuw*cuh <= NUM_SAMPLES_BLOCK)
        small_cu = 1;

    xeve_mset(base_skip, 0, sizeof(int) * MMVD_BASE_MV_NUM);

    int sld[MMVD_BASE_MV_NUM*MMVD_BASE_MV_NUM][2] = {
    {0, 0}, {1, 1}, {2, 2}, {3, 3},
    {0, 1}, {1, 2}, {2, 3}, {3, 0},
    {0, 2}, {1, 3}, {2, 0}, {3, 1},
    {0, 3}, {1, 0}, {2, 1}, {3, 2}, };

    if(admvp_flag == 0)
    {
        xeve_get_motion_skip(slice_t, scup, map_refi, map_mv, refp, cuw, cuh, w_scu, srefi, smvp, avail);
    }
    else
    {
        xevem_get_motion_merge(curr_ptr, slice_t, scup, map_refi, map_mv, refp, cuw, cuh, w_scu, h_scu, srefi, smvp, map_scu, avail_lr
                                   , NULL, history_buffer, 0, (XEVE_REFP(*)[2])refp, sh, log2_max_cuwh, ctx->map_tidx);
    }

    for (k = 0; k < MMVD_BASE_MV_NUM; k++)
    {
        base_mv[k][REFP_0][MV_X] = smvp[REFP_0][k][MV_X];
        base_mv[k][REFP_0][MV_Y] = smvp[REFP_0][k][MV_Y];
        base_mv[k][REFP_1][MV_X] = smvp[REFP_1][k][MV_X];
        base_mv[k][REFP_1][MV_Y] = smvp[REFP_1][k][MV_Y];
        base_mv[k][REFP_0][2]    = srefi[REFP_0][k];
        base_mv[k][REFP_1][2]    = srefi[REFP_1][k];

    }

    for(k = 0; k < MMVD_BASE_MV_NUM - 1; k++)
    {
        if(base_skip[k] == 0)
        {
            for(nn = k + 1; nn < MMVD_BASE_MV_NUM; nn++)
            {
                if((base_mv[k][0][2] != -1) && (base_mv[nn][0][2] != -1))
                {
                    if((base_mv[k][1][2] != -1) && (base_mv[nn][1][2] != -1))
                    {
                        if((base_mv[k][0][MV_X] == base_mv[nn][0][MV_X])  &&
                           (base_mv[k][0][MV_Y] == base_mv[nn][0][MV_Y])  &&
                           (base_mv[k][0][2]    == base_mv[nn][0][2])     &&
                           (base_mv[k][1][MV_X] == base_mv[nn][1][MV_X])  &&
                           (base_mv[k][1][MV_Y] == base_mv[nn][1][MV_Y])  &&
                           (base_mv[k][1][2]    == base_mv[nn][1][2]))
                        {
                            base_skip[nn] = -1;
                        }
                    }
                    else if((base_mv[k][1][2] == -1) && (base_mv[nn][1][2] == -1))
                    {
                        if((base_mv[k][0][MV_X] == base_mv[nn][0][MV_X]) &&
                           (base_mv[k][0][MV_Y] == base_mv[nn][0][MV_Y]) &&
                           (base_mv[k][0][2]    == base_mv[nn][0][2]))
                        {
                            base_skip[nn] = -1;
                        }
                    }
                }

                if((base_mv[k][0][2] == -1) && (base_mv[nn][0][2] == -1))
                {
                    if((base_mv[k][1][2] != -1) && (base_mv[nn][1][2] != -1))
                    {
                        if((base_mv[k][1][MV_X] == base_mv[nn][1][MV_X]) &&
                           (base_mv[k][1][MV_Y] == base_mv[nn][1][MV_Y]) &&
                           (base_mv[k][1][2] == base_mv[nn][1][2]))
                        {
                            base_skip[nn] = -1;
                        }
                    }
                    else if((base_mv[k][1][2] == -1) && (base_mv[nn][1][2] == -1))
                    {
                        base_skip[nn] = -1;
                    }
                }
            }
        }
    }

    for(c_num = 0; c_num < 3 * t_base_num; c_num++)
    {
        if((c_num >= t_base_num) && (!(ctx->sh->mmvd_group_enable_flag) || (small_cu == 1)))
        {
            continue;
        }

        cur_num = c_num;
        if(cur_num >= t_base_num)
        {
            cur_num = cur_num % t_base_num;
        }
        dev0 = cur_num / (MMVD_MAX_REFINE_NUM);
        if(base_skip[dev0] == -1)
        {
            real_mv[c_num][0][2] = -1;
            real_mv[c_num][1][2] = -1;
        }

    }
}

static double analyze_skip(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh)
{
    XEVEM_CORE      * mcore = (XEVEM_CORE *)core;
    XEVE_PINTER     * pi = &ctx->pinter[core->thread_cnt];
    pel             * y_org, *u_org, *v_org;
    s16               mvp[REFP_NUM][MV_D];
    s16               dmvr_mv[MAX_CU_CNT_IN_LCU][REFP_NUM][MV_D];
    int               best_dmvr = 0;
    s8                refi[REFP_NUM];
    double            cost, cost_best = MAX_COST;
    double            ad_best_costs[MAX_NUM_MVP];
    int               j;
    int               cuw, cuh, idx0, idx1, bit_cnt;
    s64               cy, cu, cv;
    int               w_shift = ctx->param.cs_w_shift;
    int               h_shift = ctx->param.cs_h_shift;

    mcore->ats_inter_info = 0;
    cuw = (1 << log2_cuw);
    cuh = (1 << log2_cuh);
    y_org = pi->o[Y_C] + x + y * pi->s_o[Y_C];
    cu = cv = cy = 0;
    if(ctx->sps.chroma_format_idc)
    {
        u_org = pi->o[U_C] + (x >> w_shift) + ((y >> h_shift) * pi->s_o[U_C]);
        v_org = pi->o[V_C] + (x >> w_shift) + ((y >> h_shift) * pi->s_o[V_C]);
    }
    mcore->mmvd_flag = 0;

    for(j = 0; j < MAX_NUM_MVP; j++)
    {
        ad_best_costs[j] = MAX_COST;
    }

    if(ctx->sps.tool_admvp == 0)
    {
        xeve_get_motion_skip(ctx->sh->slice_type, core->scup, ctx->map_refi, ctx->map_mv, ctx->refp[0], cuw, cuh, ctx->w_scu, pi->refi_pred, pi->mvp, core->avail_cu);
    }
    else
    {
        xevem_get_motion_merge(ctx->poc.poc_val, ctx->slice_type, core->scup, ctx->map_refi, ctx->map_mv, pi->refp[0], cuw, cuh, ctx->w_scu, ctx->h_scu, pi->refi_pred, pi->mvp, ctx->map_scu, core->avail_lr
                                   , ctx->map_unrefined_mv, &mcore->history_buffer, mcore->ibc_flag, (XEVE_REFP(*)[2])ctx->refp[0], ctx->sh, ctx->log2_max_cuwh, ctx->map_tidx);
    }

    if(ctx->pps.cu_qp_delta_enabled_flag)
    {
        if(core->cu_qp_delta_code_mode != 2)
        {
            xeve_set_qp(ctx, core, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].prev_qp);
        }
    }
    pi->mvp_idx[PRED_SKIP][REFP_0] = 0;
    pi->mvp_idx[PRED_SKIP][REFP_1] = 0;

    for(idx0 = 0; idx0 < (cuw*cuh <= NUM_SAMPLES_BLOCK ? MAX_NUM_MVP_SMALL_CU : MAX_NUM_MVP); idx0++)
    {
        idx1 = idx0;
        {

            mvp[REFP_0][MV_X] = pi->mvp[REFP_0][idx0][MV_X];
            mvp[REFP_0][MV_Y] = pi->mvp[REFP_0][idx0][MV_Y];
            mvp[REFP_1][MV_X] = pi->mvp[REFP_1][idx1][MV_X];
            mvp[REFP_1][MV_Y] = pi->mvp[REFP_1][idx1][MV_Y];

            SET_REFI(refi, pi->refi_pred[REFP_0][idx0], ctx->sh->slice_type == SLICE_B ? pi->refi_pred[REFP_1][idx1] : REFI_INVALID);
            if(!REFI_IS_VALID(refi[REFP_0]) && !REFI_IS_VALID(refi[REFP_1]))
            {
                continue;
            }

            pi->fn_mc(ctx, core, x, y, cuw, cuh, refi, mvp, pi->refp, pi->pred[PRED_NUM], ctx->poc.poc_val, TRUE, dmvr_mv);

            cy = xeve_ssd_16b(log2_cuw, log2_cuh, pi->pred[PRED_NUM][0][Y_C], y_org, cuw, pi->s_o[Y_C], ctx->sps.bit_depth_luma_minus8 + 8);
            if(ctx->sps.chroma_format_idc)
            {
                cu = xeve_ssd_16b(log2_cuw - w_shift, log2_cuh - h_shift, pi->pred[PRED_NUM][0][U_C], u_org, cuw >> w_shift, pi->s_o[U_C], ctx->sps.bit_depth_chroma_minus8 + 8);
                cv = xeve_ssd_16b(log2_cuw - w_shift, log2_cuh - h_shift, pi->pred[PRED_NUM][0][V_C], v_org, cuw >> w_shift, pi->s_o[V_C], ctx->sps.bit_depth_chroma_minus8 + 8);
            }

            if(ctx->param.rdo_dbk_switch)
            {
                calc_delta_dist_filter_boundary(ctx, PIC_MODE(ctx), PIC_ORIG(ctx), cuw, cuh, pi->pred[PRED_NUM][0], cuw, x, y, core->avail_lr, 0, 0, refi, mvp, 0, core);
                cy += core->delta_dist[Y_C];
                if(ctx->sps.chroma_format_idc)
                {
                    cu += core->delta_dist[U_C];
                    cv += core->delta_dist[V_C];
                }
            }

            cost = (double)cy + (core->dist_chroma_weight[0] * (double)cu) + (core->dist_chroma_weight[1] * (double)cv);

            SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
            DQP_LOAD(core->dqp_temp_run, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);

            xeve_sbac_bit_reset(&core->s_temp_run);
            xeve_rdo_bit_cnt_cu_skip_main(ctx, core, ctx->sh->slice_type, core->scup, idx0, idx1, 0, ctx->sps.tool_mmvd);

            bit_cnt = xeve_get_bit_number(&core->s_temp_run);
            cost += RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt);

            if(cost < cost_best)
            {
                int j;
                cost_best = cost;
                pi->mvp_idx[PRED_SKIP][REFP_0] = idx0;
                pi->mvp_idx[PRED_SKIP][REFP_1] = idx1;
                pi->mv[PRED_SKIP][REFP_0][MV_X] = mvp[REFP_0][MV_X];
                pi->mv[PRED_SKIP][REFP_0][MV_Y] = mvp[REFP_0][MV_Y];
                pi->mv[PRED_SKIP][REFP_1][MV_X] = mvp[REFP_1][MV_X];
                pi->mv[PRED_SKIP][REFP_1][MV_Y] = mvp[REFP_1][MV_Y];
                core->cost_best = cost < core->cost_best ? cost : core->cost_best;
                best_dmvr = mcore->dmvr_flag;
                mcore->dmvr_flag = 0;

                if(best_dmvr)
                {
                    u16 idx = 0, i, j;
                    for(j = 0; j < core->cuh >> MIN_CU_LOG2; j++)
                    {
                        for(i = 0; i < core->cuw >> MIN_CU_LOG2; i++)
                        {
                            pi->dmvr_mv[PRED_SKIP][idx + i][REFP_0][MV_X] = dmvr_mv[idx + i][REFP_0][MV_X];
                            pi->dmvr_mv[PRED_SKIP][idx + i][REFP_0][MV_Y] = dmvr_mv[idx + i][REFP_0][MV_Y];
                            pi->dmvr_mv[PRED_SKIP][idx + i][REFP_1][MV_X] = dmvr_mv[idx + i][REFP_1][MV_X];
                            pi->dmvr_mv[PRED_SKIP][idx + i][REFP_1][MV_Y] = dmvr_mv[idx + i][REFP_1][MV_Y];
                        }
                        idx += core->cuw >> MIN_CU_LOG2;
                    }
                }

                pi->mvd[PRED_SKIP][REFP_0][MV_X] = 0;
                pi->mvd[PRED_SKIP][REFP_0][MV_Y] = 0;
                pi->mvd[PRED_SKIP][REFP_1][MV_X] = 0;
                pi->mvd[PRED_SKIP][REFP_1][MV_Y] = 0;
                pi->refi[PRED_SKIP][REFP_0] = refi[REFP_0];
                pi->refi[PRED_SKIP][REFP_1] = refi[REFP_1];

                for(j = 0; j < N_C; j++)
                {
                    if(j != 0 && !ctx->sps.chroma_format_idc)
                        continue;
                    int size_tmp = (cuw * cuh) >> (j == 0 ? 0 : (w_shift + h_shift));
                    xeve_mcpy(pi->pred[PRED_SKIP][0][j], pi->pred[PRED_NUM][0][j], size_tmp * sizeof(pel));
                }

                SBAC_STORE(core->s_temp_best, core->s_temp_run);
                DQP_STORE(core->dqp_temp_best, core->dqp_temp_run);
                pi->ats_inter_info_mode[PRED_SKIP] = 0;
            }
            ad_best_costs[idx0] = cost;
        }
    }
    if(ctx->slice_type == SLICE_B)
    {
        assert(ctx->slice_type == SLICE_B);
        /* removes the cost above threshold and remove the duplicates */

        for(idx0 = 0; idx0 < (cuw * cuh <= NUM_SAMPLES_BLOCK ? MAX_NUM_MVP_SMALL_CU : MAX_NUM_MVP); idx0++)
        {
            /* removes the cost above threshold */
            if(ad_best_costs[idx0] > (cost_best * FAST_MERGE_THR))
            {
                mcore->eval_mvp_idx[idx0] = 0;
            }
            else
            {
                mcore->eval_mvp_idx[idx0] = 1;
            }
        }

        /* remove the duplicates and keep the best */
        for(idx0 = 0; idx0 < (cuw * cuh <= NUM_SAMPLES_BLOCK ? MAX_NUM_MVP_SMALL_CU : MAX_NUM_MVP); idx0++)
        {
            if(mcore->eval_mvp_idx[idx0] == 1)
            {
                for(j = idx0 + 1; j < (cuw * cuh <= NUM_SAMPLES_BLOCK ? MAX_NUM_MVP_SMALL_CU : MAX_NUM_MVP); j++)
                {
                    if(mcore->eval_mvp_idx[j] == 1)
                    {
                        u8 is_duplicate = 0;
                        if(pi->refi_pred[REFP_0][idx0] == pi->refi_pred[REFP_0][j])
                        {
                            if((pi->mvp[REFP_0][idx0][MV_X] == pi->mvp[REFP_0][j][MV_X]) &&
                                (pi->mvp[REFP_0][idx0][MV_Y] == pi->mvp[REFP_0][j][MV_Y]))
                            {
                                is_duplicate = 1;
                            }
                        }
                        if(is_duplicate && (pi->refi_pred[REFP_1][idx0] == pi->refi_pred[REFP_1][j]))
                        {
                            if((pi->mvp[REFP_1][idx0][MV_X] == pi->mvp[REFP_1][j][MV_X]) &&
                                (pi->mvp[REFP_1][idx0][MV_Y] == pi->mvp[REFP_1][j][MV_Y]))
                            {
                                if(ad_best_costs[j] < ad_best_costs[idx0])
                                {
                                    mcore->eval_mvp_idx[idx0] = 0;
                                    break;
                                }
                                else
                                {
                                    mcore->eval_mvp_idx[j] = 0;
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    mcore->dmvr_flag = best_dmvr;
    return cost_best;
}

static double analyze_merge(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh)
{
    XEVEM_CORE     * mcore = (XEVEM_CORE *)core;
    XEVE_PINTER    * pi = &ctx->pinter[core->thread_cnt];
    s16              mvp[REFP_NUM][MV_D];
    s8               refi[REFP_NUM];
    double           cost, cost_best = MAX_COST;
    int              cuw, cuh, idx0;
    int              j;
    int              pidx = PRED_DIR, pidx1 = PRED_NUM;
    int              tmp_mvp0 = 0, tmp_mvp1 = 0, tmp_mvx0 = 0, tmp_mvx1 = 0, tmp_mvy0 = 0, tmp_mvy1 = 0, tmp_ref0 = 0, tmp_ref1 = 0;
    int              tmp_dmvr_mv[MAX_CU_CNT_IN_LCU][REFP_NUM][MV_D];
    BOOL             apply_dmvr;
    int              best_dmvr = 0;

    mcore->ats_inter_info = 0;
    cuw = (1 << log2_cuw);
    cuh = (1 << log2_cuh);
    mcore->mmvd_flag = 0;

    if(ctx->sps.tool_admvp == 0)
    {
        xeve_get_motion_skip(ctx->sh->slice_type, core->scup, ctx->map_refi, ctx->map_mv, ctx->refp[0], cuw, cuh, ctx->w_scu, pi->refi_pred, pi->mvp, core->avail_cu);
    }
    else
    {
        xevem_get_motion_merge(ctx->poc.poc_val, ctx->slice_type, core->scup, ctx->map_refi, ctx->map_mv, pi->refp[0], cuw, cuh, ctx->w_scu, ctx->h_scu, pi->refi_pred, pi->mvp, ctx->map_scu, core->avail_lr
                             , ctx->map_unrefined_mv, &mcore->history_buffer, mcore->ibc_flag, (XEVE_REFP(*)[2])ctx->refp[0], ctx->sh, ctx->log2_max_cuwh, ctx->map_tidx);
    }

    for(idx0 = 0; idx0 < (cuw*cuh <= NUM_SAMPLES_BLOCK ? MAX_NUM_MVP_SMALL_CU : MAX_NUM_MVP); idx0++)
    {
        if(ctx->sh->slice_type == SLICE_B && 0 == mcore->eval_mvp_idx[idx0])
        {
            continue;
        }
        mvp[REFP_0][MV_X] = pi->mvp[REFP_0][idx0][MV_X];
        mvp[REFP_0][MV_Y] = pi->mvp[REFP_0][idx0][MV_Y];
        mvp[REFP_1][MV_X] = pi->mvp[REFP_1][idx0][MV_X];
        mvp[REFP_1][MV_Y] = pi->mvp[REFP_1][idx0][MV_Y];

        SET_REFI(refi, pi->refi_pred[REFP_0][idx0], ctx->sh->slice_type == SLICE_B ? pi->refi_pred[REFP_1][idx0] : REFI_INVALID);
        if(!REFI_IS_VALID(refi[REFP_0]) && !REFI_IS_VALID(refi[REFP_1]))
        {
            continue;
        }

        pi->mvp_idx[pidx][REFP_0] = idx0;
        pi->mvp_idx[pidx][REFP_1] = idx0;
        pi->mv[pidx][REFP_0][MV_X] = mvp[REFP_0][MV_X];
        pi->mv[pidx][REFP_0][MV_Y] = mvp[REFP_0][MV_Y];
        pi->mv[pidx][REFP_1][MV_X] = mvp[REFP_1][MV_X];
        pi->mv[pidx][REFP_1][MV_Y] = mvp[REFP_1][MV_Y];
        pi->mvd[pidx][REFP_0][MV_X] = 0;
        pi->mvd[pidx][REFP_0][MV_Y] = 0;
        pi->mvd[pidx][REFP_1][MV_X] = 0;
        pi->mvd[pidx][REFP_1][MV_Y] = 0;
        pi->refi[pidx][REFP_0] = refi[REFP_0];
        pi->refi[pidx][REFP_1] = refi[REFP_1];

        apply_dmvr = TRUE;
        cost = pinter_residue_rdo(ctx, core, x, y, log2_cuw, log2_cuh, pi->pred[pidx], pi->coef[pidx], pidx, pi->mvp_idx[pidx], apply_dmvr);
        if(cost < cost_best)
        {
            tmp_mvp0 = idx0;
            tmp_mvp1 = idx0;
            tmp_mvx0 = pi->mv[pidx][REFP_0][MV_X];
            tmp_mvy0 = pi->mv[pidx][REFP_0][MV_Y];
            tmp_mvx1 = pi->mv[pidx][REFP_1][MV_X];
            tmp_mvy1 = pi->mv[pidx][REFP_1][MV_Y];
            best_dmvr = mcore->dmvr_flag;
            mcore->dmvr_flag = 0;

            if(best_dmvr)
            {
                u16 idx = 0, i, j;
                for(j = 0; j < core->cuh >> MIN_CU_LOG2; j++)
                {
                    for(i = 0; i < core->cuw >> MIN_CU_LOG2; i++)
                    {
                        tmp_dmvr_mv[idx + i][REFP_0][MV_X] = pi->dmvr_mv[pidx][idx + i][REFP_0][MV_X];
                        tmp_dmvr_mv[idx + i][REFP_0][MV_Y] = pi->dmvr_mv[pidx][idx + i][REFP_0][MV_Y];
                        tmp_dmvr_mv[idx + i][REFP_1][MV_X] = pi->dmvr_mv[pidx][idx + i][REFP_1][MV_X];
                        tmp_dmvr_mv[idx + i][REFP_1][MV_Y] = pi->dmvr_mv[pidx][idx + i][REFP_1][MV_Y];
                    }
                    idx += core->cuw >> MIN_CU_LOG2;
                }
            }

            tmp_ref0 = pi->refi[pidx][REFP_0];
            tmp_ref1 = pi->refi[pidx][REFP_1];

            cost_best = cost;

            for(j = 0; j < N_C; j++)
            {
                if(j != 0 && !ctx->sps.chroma_format_idc)
                    continue;
                int size_tmp = (cuw * cuh) >> (j == 0 ? 0 : ctx->param.cs_w_shift + ctx->param.cs_h_shift);
                pi->nnz_best[pidx][j] = core->nnz[j];
                xeve_mcpy(pi->nnz_sub_best[pidx][j], core->nnz_sub[j], MAX_SUB_TB_NUM * sizeof(int));
                xeve_mcpy(pi->pred[pidx1][0][j], pi->pred[pidx][0][j], size_tmp * sizeof(pel));
                xeve_mcpy(pi->coef[pidx1][j], pi->coef[pidx][j], size_tmp * sizeof(s16));
            }
            SBAC_STORE(core->s_temp_best_merge, core->s_temp_best);
            DQP_STORE(core->dqp_temp_best_merge, core->dqp_temp_best);
            pi->ats_inter_info_mode[pidx] = mcore->ats_inter_info;
        }
    }

    pi->mvp_idx[pidx][REFP_0] = tmp_mvp0;
    pi->mvp_idx[pidx][REFP_1] = tmp_mvp1;
    pi->mv[pidx][REFP_0][MV_X] = tmp_mvx0;
    pi->mv[pidx][REFP_0][MV_Y] = tmp_mvy0;
    pi->mv[pidx][REFP_1][MV_X] = tmp_mvx1;
    pi->mv[pidx][REFP_1][MV_Y] = tmp_mvy1;
    mcore->dmvr_flag = best_dmvr;

    if(mcore->dmvr_flag)
    {
        u16 idx = 0, i, j;
        for(j = 0; j < core->cuh >> MIN_CU_LOG2; j++)
        {
            for(i = 0; i < core->cuw >> MIN_CU_LOG2; i++)
            {
                pi->dmvr_mv[pidx][idx + i][REFP_0][MV_X] = tmp_dmvr_mv[idx + i][REFP_0][MV_X];
                pi->dmvr_mv[pidx][idx + i][REFP_0][MV_Y] = tmp_dmvr_mv[idx + i][REFP_0][MV_Y];
                pi->dmvr_mv[pidx][idx + i][REFP_1][MV_X] = tmp_dmvr_mv[idx + i][REFP_1][MV_X];
                pi->dmvr_mv[pidx][idx + i][REFP_1][MV_Y] = tmp_dmvr_mv[idx + i][REFP_1][MV_Y];
            }
            idx += core->cuw >> MIN_CU_LOG2;
        }
    }

    pi->mvd[pidx][REFP_0][MV_X] = 0;
    pi->mvd[pidx][REFP_0][MV_Y] = 0;
    pi->mvd[pidx][REFP_1][MV_X] = 0;
    pi->mvd[pidx][REFP_1][MV_Y] = 0;
    pi->refi[pidx][REFP_0] = tmp_ref0;
    pi->refi[pidx][REFP_1] = tmp_ref1;

    return cost_best;
}

static double analyze_skip_mmvd(XEVE_CTX * ctx, XEVE_CORE * core, int x, int y, int log2_cuw, int log2_cuh, int real_mv[][2][3])
{
    XEVEM_CORE      *mcore = (XEVEM_CORE*)core;
    XEVE_PINTER     *pi = &ctx->pinter[core->thread_cnt];
    pel             *y_org, *u_org, *v_org;
    s16              mvp[REFP_NUM][MV_D];
    s8               refi[REFP_NUM];
    double           cost, cost_best = MAX_COST;
    int              cuw, cuh, bit_cnt;
    s64              cy, cu, cv;
    int              c_num = 0;
    int              t_base_num = 0;
    int              best_idx_num = -1;
    int              i;
    int              w_shift = ctx->param.cs_w_shift;
    int              h_shift = ctx->param.cs_h_shift;
    cy = cu = cv = 0;

    mcore->ats_inter_info = 0;

    if(ctx->pps.cu_qp_delta_enabled_flag)
    {
        if(core->cu_qp_delta_code_mode != 2)
        {
            xeve_set_qp(ctx, core, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].prev_qp);
        }
    }
    mcore->mmvd_flag = 1;

    cuw = (1 << log2_cuw);
    cuh = (1 << log2_cuh);
    y_org = pi->o[Y_C] + x + y * pi->s_o[Y_C];
    if(ctx->sps.chroma_format_idc)
    {
        u_org = pi->o[U_C] + (x >> w_shift) + ((y >> h_shift) * pi->s_o[U_C]);
        v_org = pi->o[V_C] + (x >> w_shift) + ((y >> h_shift) * pi->s_o[V_C]);
    }

    pi->mvp_idx[PRED_SKIP_MMVD][REFP_0] = 0;
    pi->mvp_idx[PRED_SKIP_MMVD][REFP_1] = 0;

    t_base_num = MMVD_MAX_REFINE_NUM * MMVD_BASE_MV_NUM;

    if((pi->slice_type == SLICE_B) || (pi->slice_type == SLICE_P))
    {
        t_base_num = pi->best_index[PRED_DIR_MMVD][MMVD_SKIP_CON_NUM - 1];
    }
    for(i = 0; i < t_base_num; i++)
    {
        if((pi->slice_type == SLICE_B) || (pi->slice_type == SLICE_P))
        {
            c_num = pi->best_index[PRED_DIR_MMVD][i];
        }
        else
        {
            c_num = i;
        }
        if(c_num == -1)
        {
            continue;
        }

        mvp[REFP_0][MV_X] = real_mv[c_num][0][MV_X];
        mvp[REFP_0][MV_Y] = real_mv[c_num][0][MV_Y];
        mvp[REFP_1][MV_X] = real_mv[c_num][1][MV_X];
        mvp[REFP_1][MV_Y] = real_mv[c_num][1][MV_Y];
        if((real_mv[c_num][0][2] == -1) && (real_mv[c_num][1][2] == -1))
        {
            continue;
        }
        pi->refi[PRED_SKIP_MMVD][0] = real_mv[c_num][0][2];
        pi->refi[PRED_SKIP_MMVD][1] = real_mv[c_num][1][2];

        SET_REFI(refi, real_mv[c_num][0][2], ctx->sh->slice_type == SLICE_B ? real_mv[c_num][1][2] : REFI_INVALID);
        if(!REFI_IS_VALID(refi[REFP_0]) && !REFI_IS_VALID(refi[REFP_1]))
        {
            continue;
        }

        pi->fn_mc(ctx, core, x, y, cuw, cuh, refi, mvp, pi->refp, pi->pred[PRED_NUM], ctx->poc.poc_val, FALSE, NULL);

        cy = xeve_ssd_16b(log2_cuw, log2_cuh, pi->pred[PRED_NUM][0][Y_C], y_org, cuw, pi->s_o[Y_C], ctx->sps.bit_depth_luma_minus8 + 8);
        if(ctx->sps.chroma_format_idc)
        {
            cu = xeve_ssd_16b(log2_cuw - w_shift, log2_cuh - h_shift, pi->pred[PRED_NUM][0][U_C], u_org, cuw >> w_shift, pi->s_o[U_C], ctx->sps.bit_depth_chroma_minus8 + 8);
            cv = xeve_ssd_16b(log2_cuw - w_shift, log2_cuh - h_shift, pi->pred[PRED_NUM][0][V_C], v_org, cuw >> w_shift, pi->s_o[V_C], ctx->sps.bit_depth_chroma_minus8 + 8);
        }

        if(ctx->param.rdo_dbk_switch)
        {
            calc_delta_dist_filter_boundary(ctx, PIC_MODE(ctx), PIC_ORIG(ctx), cuw, cuh, pi->pred[PRED_NUM][0], cuw, x, y, core->avail_lr, 0, 0, refi, mvp, 0, core);
            cy += core->delta_dist[Y_C];
            cu += core->delta_dist[U_C];
            cv += core->delta_dist[V_C];
        }

        cost = (double)cy + (core->dist_chroma_weight[0] * (double)cu) + (core->dist_chroma_weight[1] * (double)cv);

        SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
        DQP_LOAD(core->dqp_temp_run, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);

        xeve_sbac_bit_reset(&core->s_temp_run);
        xeve_rdo_bit_cnt_cu_skip_main(ctx, core, ctx->sh->slice_type, core->scup, 0, 0, c_num, ctx->sps.tool_mmvd);
        bit_cnt = xeve_get_bit_number(&core->s_temp_run);
        cost += RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt);

        if(cost < cost_best)
        {
            int j;
            cost_best = cost;
            best_idx_num = c_num;
            pi->mmvd_idx[PRED_SKIP_MMVD] = c_num;

            pi->mv[PRED_SKIP_MMVD][REFP_0][MV_X] = mvp[REFP_0][MV_X];
            pi->mv[PRED_SKIP_MMVD][REFP_0][MV_Y] = mvp[REFP_0][MV_Y];
            pi->mv[PRED_SKIP_MMVD][REFP_1][MV_X] = mvp[REFP_1][MV_X];
            pi->mv[PRED_SKIP_MMVD][REFP_1][MV_Y] = mvp[REFP_1][MV_Y];
            pi->refi[PRED_SKIP_MMVD][REFP_0] = refi[REFP_0];
            pi->refi[PRED_SKIP_MMVD][REFP_1] = refi[REFP_1];

            core->cost_best = cost < core->cost_best ? cost : core->cost_best;

            for(j = 0; j < N_C; j++)
            {
                if(j != 0 && !ctx->sps.chroma_format_idc)
                    continue;
                int size_tmp = (cuw * cuh) >> (j == 0 ? 0 : (w_shift + h_shift));
                xeve_mcpy(pi->pred[PRED_SKIP_MMVD][0][j], pi->pred[PRED_NUM][0][j], size_tmp * sizeof(pel));
            }
            SBAC_STORE(core->s_temp_best, core->s_temp_run);
            DQP_STORE(core->dqp_temp_best, core->dqp_temp_run);

            pi->ats_inter_info_mode[PRED_SKIP_MMVD] = 0;
        }
    }
    mvp[REFP_0][MV_X] = real_mv[best_idx_num][0][MV_X];
    mvp[REFP_0][MV_Y] = real_mv[best_idx_num][0][MV_Y];
    mvp[REFP_1][MV_X] = real_mv[best_idx_num][1][MV_X];
    mvp[REFP_1][MV_Y] = real_mv[best_idx_num][1][MV_Y];
    pi->refi[PRED_SKIP_MMVD][REFP_0] = real_mv[best_idx_num][0][2];
    pi->refi[PRED_SKIP_MMVD][REFP_1] = real_mv[best_idx_num][1][2];

    pi->mvd[PRED_SKIP_MMVD][REFP_0][MV_X] = 0;
    pi->mvd[PRED_SKIP_MMVD][REFP_0][MV_Y] = 0;
    pi->mvd[PRED_SKIP_MMVD][REFP_1][MV_X] = 0;
    pi->mvd[PRED_SKIP_MMVD][REFP_1][MV_Y] = 0;

    return cost_best;
}

static double analyze_merge_mmvd(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh, int real_mv[][2][3])
{
    XEVEM_CORE  *mcore = (XEVEM_CORE*)core;
    XEVE_PINTER *pi = &ctx->pinter[core->thread_cnt];

    s8  refi[REFP_NUM];
    int pidx, i;
    int c_num = 0;
    int t_base_num = 0;
    double direct_cost[10];
    int current_idx = 0;
    double min_cost = MAX_COST;
    double temp_cost = 0.0;
    int moving_index = 0;
    int current_array[MMVD_GRP_NUM * MMVD_MAX_REFINE_NUM * MMVD_BASE_MV_NUM];
    int i1, i2, i3;
    int cur_temp = 0;
    int ttt = 0;
    int consider_num = 0;
    int best_candi = -1;
    double min_cost_temp = MAX_COST;
    double cost_best_save = core->cost_best;
    mcore->ats_inter_info = 0;

    pidx = PRED_DIR_MMVD;
    SET_REFI(pi->refi[pidx], 0, ctx->sh->slice_type == SLICE_B ? 0 : REFI_INVALID);

    for(i = 0; i < MMVD_SKIP_CON_NUM; i++)
    {
        pi->best_index[pidx][i] = -1;
        direct_cost[i] = MAX_COST;
    }
    t_base_num = MMVD_MAX_REFINE_NUM * MMVD_BASE_MV_NUM;

    pi->mvp_idx[pidx][REFP_0] = 0;
    pi->mvp_idx[pidx][REFP_1] = 0;


    for(i1 = 0; i1 < MMVD_DIST_NUM; i1++)
    {
        for(i2 = 0; i2 < MMVD_BASE_MV_NUM; i2++)
        {
            for(i3 = 0; i3 < 4; i3++)
            {
                int idx_tmp = i1 * 4 + i2 * MMVD_MAX_REFINE_NUM + i3;
                current_array[cur_temp] = idx_tmp;
                current_array[cur_temp + t_base_num] = idx_tmp + t_base_num;
                current_array[cur_temp + t_base_num * 2] = idx_tmp + t_base_num * 2;
                cur_temp++;
            }
        }
    }

    int max_dist = 0;
    for(moving_index = 0; moving_index < 3 * t_base_num; moving_index++)
    {
        c_num = moving_index;
        if((moving_index >= t_base_num) && (!(ctx->sh->mmvd_group_enable_flag) || ((1 << core->log2_cuw)*(1 << core->log2_cuh) <= NUM_SAMPLES_BLOCK)))
        {
            continue;
        }

        if (c_num >= 32 && ((c_num & 31) >> 2) > max_dist + 1)
        {
            continue;
        }

        pi->mv[pidx][REFP_0][MV_X] = real_mv[c_num][0][MV_X];
        pi->mv[pidx][REFP_0][MV_Y] = real_mv[c_num][0][MV_Y];
        pi->mv[pidx][REFP_1][MV_X] = real_mv[c_num][1][MV_X];
        pi->mv[pidx][REFP_1][MV_Y] = real_mv[c_num][1][MV_Y];

        if((real_mv[c_num][0][2] == -1) && (real_mv[c_num][1][2] == -1))
        {
            continue;
        }
        pi->refi[pidx][0] = real_mv[c_num][0][2];
        pi->refi[pidx][1] = real_mv[c_num][1][2];

        SET_REFI(refi, real_mv[c_num][0][2], ctx->sh->slice_type == SLICE_B ? real_mv[c_num][1][2] : REFI_INVALID);
        if(!REFI_IS_VALID(refi[REFP_0]) && !REFI_IS_VALID(refi[REFP_1]))
        {
            continue;
        }

        pi->mvd[pidx][REFP_0][MV_X] = 0;
        pi->mvd[pidx][REFP_0][MV_Y] = 0;
        pi->mvd[pidx][REFP_1][MV_X] = 0;
        pi->mvd[pidx][REFP_1][MV_Y] = 0;

        pi->mmvd_idx[pidx] = c_num;

        temp_cost = pinter_residue_rdo_mmvd(ctx, core, x, y, log2_cuw, log2_cuh, pi->pred[pidx], pidx);

        if(temp_cost < direct_cost[current_idx])
        {
            if (c_num < 32 && max_dist < (c_num >> 2))
            {
                max_dist = (c_num >> 2);
            }

            direct_cost[current_idx] = temp_cost;
            pi->best_index[pidx][current_idx] = c_num;

            for(int c = current_idx; c >= 1; c--)
            {
                if(direct_cost[c] < direct_cost[c - 1])
                {
                    int tmp_idx;
                    double tmp_cost;

                    tmp_cost = direct_cost[c];
                    tmp_idx = pi->best_index[pidx][c];

                    direct_cost[c] = direct_cost[c - 1];
                    pi->best_index[pidx][c] = pi->best_index[pidx][c - 1];

                    direct_cost[c - 1] = tmp_cost;
                    pi->best_index[pidx][c - 1] = tmp_idx;
                }
                else
                {
                    break;
                }
            }
            current_idx = XEVE_MIN(current_idx + 1, MMVD_SKIP_CON_NUM - 1);
        }
    }

    min_cost = 0.0;
    consider_num = 1;
    for(ttt = 1; ttt < current_idx; ttt++)
    {
        if((direct_cost[0] * MMVD_THRESHOLD) < direct_cost[ttt])
        {
            break;
        }
        else
        {
            consider_num++;
        }
    }

    pi->best_index[pidx][MMVD_SKIP_CON_NUM - 1] = consider_num;

    min_cost = MAX_COST;
    min_cost_temp = MAX_COST;
    temp_cost = MAX_COST;
    for(ttt = 0; ttt < consider_num; ttt++)
    {
        c_num = pi->best_index[pidx][ttt];

        pi->mv[pidx][REFP_0][MV_X] = real_mv[c_num][0][MV_X];
        pi->mv[pidx][REFP_0][MV_Y] = real_mv[c_num][0][MV_Y];
        pi->mv[pidx][REFP_1][MV_X] = real_mv[c_num][1][MV_X];
        pi->mv[pidx][REFP_1][MV_Y] = real_mv[c_num][1][MV_Y];
        pi->refi[pidx][0] = real_mv[c_num][0][2];
        pi->refi[pidx][1] = real_mv[c_num][1][2];

        SET_REFI(refi, real_mv[c_num][0][2], ctx->sh->slice_type == SLICE_B ? real_mv[c_num][1][2] : REFI_INVALID);
        if(!REFI_IS_VALID(refi[REFP_0]) && !REFI_IS_VALID(refi[REFP_1]))
        {
            continue;
        }

        pi->mvd[pidx][REFP_0][MV_X] = 0;
        pi->mvd[pidx][REFP_0][MV_Y] = 0;
        pi->mvd[pidx][REFP_1][MV_X] = 0;
        pi->mvd[pidx][REFP_1][MV_Y] = 0;

        pi->mmvd_idx[pidx] = c_num;

        temp_cost = pinter_residue_rdo(ctx, core, x, y, log2_cuw, log2_cuh, pi->pred[pidx], pi->coef[pidx], pidx, pi->mvp_idx[pidx], FALSE);
        //temp_cost = pinter_residue_rdo_mmvd(ctx, core, x, y, log2_cuw, log2_cuh, pi->pred[pidx], pidx);

        xeve_mcpy(pi->nnz_best[pidx], core->nnz, sizeof(int) * N_C);
        xeve_mcpy(pi->nnz_sub_best[pidx], core->nnz_sub, sizeof(int) * N_C * MAX_SUB_TB_NUM);

        if(min_cost_temp > temp_cost)
        {
            min_cost_temp = temp_cost;
            best_candi = ttt;
        }
    }

    //Note: temp_cost could be smaller than min_cost
    //I doubt whether the next for loop is needed
    core->cost_best = cost_best_save;

    for(ttt = best_candi; ttt < best_candi + 1; ttt++)
    {
        c_num = pi->best_index[pidx][ttt];

        pi->mv[pidx][REFP_0][MV_X] = real_mv[c_num][0][MV_X];
        pi->mv[pidx][REFP_0][MV_Y] = real_mv[c_num][0][MV_Y];
        pi->mv[pidx][REFP_1][MV_X] = real_mv[c_num][1][MV_X];
        pi->mv[pidx][REFP_1][MV_Y] = real_mv[c_num][1][MV_Y];
        pi->refi[pidx][0] = real_mv[c_num][0][2];
        pi->refi[pidx][1] = real_mv[c_num][1][2];

        SET_REFI(refi, real_mv[c_num][0][2], ctx->sh->slice_type == SLICE_B ? real_mv[c_num][1][2] : REFI_INVALID);
        if(!REFI_IS_VALID(refi[REFP_0]) && !REFI_IS_VALID(refi[REFP_1]))
        {
            continue;
        }

        pi->mvd[pidx][REFP_0][MV_X] = 0;
        pi->mvd[pidx][REFP_0][MV_Y] = 0;
        pi->mvd[pidx][REFP_1][MV_X] = 0;
        pi->mvd[pidx][REFP_1][MV_Y] = 0;

        pi->mmvd_idx[pidx] = c_num;

        min_cost = pinter_residue_rdo(ctx, core, x, y, log2_cuw, log2_cuh, pi->pred[pidx], pi->coef[pidx], pidx, pi->mvp_idx[pidx], FALSE);
        pi->mmvd_idx[pidx] = c_num;
        xeve_mcpy(pi->nnz_best[pidx], core->nnz, sizeof(int) * N_C);
        xeve_mcpy(pi->nnz_sub_best[pidx], core->nnz_sub, sizeof(int) * N_C * MAX_SUB_TB_NUM);
        pi->ats_inter_info_mode[pidx] = mcore->ats_inter_info;
        core->cost_best = min_cost < core->cost_best ? min_cost : core->cost_best;
    }

    return min_cost;
}

static s8 get_first_refi_main(int scup, int lidx, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], int cuw, int cuh, int w_scu, int h_scu, u32 *map_scu, u8 mvr_idx, u16 avail_lr
                            , s16(*map_unrefined_mv)[REFP_NUM][MV_D], XEVE_HISTORY_BUFFER * history_buffer, int hmvp_flag, u8* map_tidx)
{
    int neb_addr[MAX_NUM_POSSIBLE_SCAND], valid_flag[MAX_NUM_POSSIBLE_SCAND];
    s8  refi = 0, default_refi;
    s16 default_mv[MV_D];

    xeve_check_motion_availability(scup, cuw, cuh, w_scu, h_scu, neb_addr, valid_flag, map_scu, avail_lr, 1, 0, map_tidx);
    xeve_get_default_motion_main(neb_addr, valid_flag, 0, lidx, map_refi, map_mv, &default_refi, default_mv, map_scu, map_unrefined_mv, scup, w_scu, history_buffer, hmvp_flag);

    assert(mvr_idx < 5);
    //neb-position is coupled with mvr index
    if(valid_flag[mvr_idx])
    {
        refi = REFI_IS_VALID(map_refi[neb_addr[mvr_idx]][lidx]) ? map_refi[neb_addr[mvr_idx]][lidx] : default_refi;
    }
    else
    {
        refi = default_refi;
    }

    return refi;
}

s8 pinter_get_first_refi_main(XEVE_CTX *ctx, XEVE_CORE *core, int ref_idx, int pidx, int cuw, int cuh)
{
    XEVEM_CORE *mcore = (XEVEM_CORE*)core;
    XEVE_PINTER *pi = &ctx->pinter[core->thread_cnt];

    return get_first_refi_main(core->scup, ref_idx, ctx->map_refi, ctx->map_mv, cuw, cuh, ctx->w_scu, ctx->h_scu, ctx->map_scu, pi->mvr_idx[pidx], core->avail_lr
                                    , ctx->map_unrefined_mv, &mcore->history_buffer, ctx->sps.tool_hmvp, ctx->map_tidx);
}

static double analyze_bi(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh, double * cost_inter)
{
    XEVE_PINTER *pi = &ctx->pinter[core->thread_cnt];

    s8         refi[REFP_NUM] = {REFI_INVALID, REFI_INVALID};
    int        t1;
    u32        best_mecost = XEVE_UINT32_MAX;
    int        refi_best = 0, refi_cur;
    int        changed = 0;
    u32        mecost;
    pel        *org;
    pel(*pred)[N_C][MAX_CU_DIM];
    int         cuw, cuh, t0;
    double      cost;
    int         lidx_ref, lidx_cnd, mvp_idx = 0;
    int         pidx, pidx_ref, pidx_cnd, i;
    const int   mvr_offset = pi->curr_mvr * ORG_PRED_NUM;
    u8          bi_idx = BI_NORMAL + (pi->curr_bi % 3);
    int         bi_start = 0;
    int         bi_end = pi->num_refp;

    cuw = (1 << log2_cuw);
    cuh = (1 << log2_cuh);

    if(bi_idx == BI_FL0 || bi_idx == BI_FL1)
    {
        pi->mot_bits[REFP_0] = 0;
        pi->mot_bits[REFP_1] = 0;

        if(bi_idx == BI_FL0)
        {
            pidx = PRED_FL0_BI + mvr_offset;
        }
        else
        {
            pidx = PRED_FL1_BI + mvr_offset;
        }

        pi->mvr_idx[pidx] = pi->curr_mvr;
        pi->bi_idx[pidx] = bi_idx;
        pi->mvp_idx[pidx][REFP_0] = 0;
        pi->mvp_idx[pidx][REFP_1] = 0;

        lidx_ref = (bi_idx == BI_FL1) ? REFP_0 : REFP_1;
        lidx_cnd = (bi_idx == BI_FL1) ? REFP_1 : REFP_0;

        pi->refi[pidx][lidx_ref] = REFI_INVALID;
        pi->refi[pidx][lidx_cnd] = pi->fn_get_first_refi(ctx, core, lidx_cnd, pidx, cuw, cuh);

        pi->mv[pidx][lidx_ref][MV_X] = pi->mvp_scale[lidx_ref][pi->refi[pidx][lidx_ref]][pi->mvp_idx[pidx][lidx_ref]][MV_X];
        pi->mv[pidx][lidx_ref][MV_Y] = pi->mvp_scale[lidx_ref][pi->refi[pidx][lidx_ref]][pi->mvp_idx[pidx][lidx_ref]][MV_Y];
        pi->mv[pidx][lidx_cnd][MV_X] = pi->mvp_scale[lidx_cnd][pi->refi[pidx][lidx_cnd]][pi->mvp_idx[pidx][lidx_cnd]][MV_X];
        pi->mv[pidx][lidx_cnd][MV_Y] = pi->mvp_scale[lidx_cnd][pi->refi[pidx][lidx_cnd]][pi->mvp_idx[pidx][lidx_cnd]][MV_Y];

        /* get MVP lidx_cnd */
        org = pi->o[Y_C] + x + y * pi->s_o[Y_C];
        pred = pi->pred[pidx];

        refi[REFP_0] = pi->refi[pidx][REFP_0];
        refi[REFP_1] = pi->refi[pidx][REFP_1];

        /* predict reference */
        pi->fn_mc(ctx, core, x, y, cuw, cuh, refi, pi->mv[pidx], pi->refp, pred, 0, FALSE, NULL);

        get_org_bi(org, pred[0][Y_C], pi->s_o[Y_C], cuw, cuh, pi->org_bi);
        refi[lidx_ref] = pi->fn_get_first_refi(ctx, core, lidx_ref, pidx, cuw, cuh);
        refi[lidx_cnd] = REFI_INVALID;

        for(refi_cur = refi[lidx_ref]; refi_cur < refi[lidx_ref] + 1; refi_cur++)
        {
            refi[lidx_ref] = refi_cur;
            mecost = pi->fn_me(pi, x, y, log2_cuw, log2_cuh, &refi[lidx_ref], lidx_ref, pi->mvp_scale[lidx_ref][refi_cur][pi->mvp_idx[pidx][lidx_ref]]
                             , pi->mv_scale[lidx_ref][refi_cur], bi_idx, ctx->sps.bit_depth_luma_minus8 + 8);
            if(mecost < best_mecost)
            {
                refi_best = refi_cur;
                best_mecost = mecost;
                pi->mv[pidx][lidx_ref][MV_X] = pi->mv_scale[lidx_ref][refi_cur][MV_X];
                pi->mv[pidx][lidx_ref][MV_Y] = pi->mv_scale[lidx_ref][refi_cur][MV_Y];
            }
        }

        pi->refi[pidx][lidx_ref] = refi_best;

        pi->mv[pidx][REFP_0][MV_X] = (pi->mv[pidx][REFP_0][MV_X] >> pi->curr_mvr) << pi->curr_mvr;
        pi->mv[pidx][REFP_0][MV_Y] = (pi->mv[pidx][REFP_0][MV_Y] >> pi->curr_mvr) << pi->curr_mvr;
        pi->mv[pidx][REFP_1][MV_X] = (pi->mv[pidx][REFP_1][MV_X] >> pi->curr_mvr) << pi->curr_mvr;
        pi->mv[pidx][REFP_1][MV_Y] = (pi->mv[pidx][REFP_1][MV_Y] >> pi->curr_mvr) << pi->curr_mvr;

        pi->mvd[pidx][REFP_0][MV_X] = pi->mv[pidx][REFP_0][MV_X] - pi->mvp_scale[REFP_0][pi->refi[pidx][REFP_0]][pi->mvp_idx[pidx][REFP_0]][MV_X];
        pi->mvd[pidx][REFP_0][MV_Y] = pi->mv[pidx][REFP_0][MV_Y] - pi->mvp_scale[REFP_0][pi->refi[pidx][REFP_0]][pi->mvp_idx[pidx][REFP_0]][MV_Y];
        pi->mvd[pidx][REFP_1][MV_X] = pi->mv[pidx][REFP_1][MV_X] - pi->mvp_scale[REFP_1][pi->refi[pidx][REFP_1]][pi->mvp_idx[pidx][REFP_1]][MV_X];
        pi->mvd[pidx][REFP_1][MV_Y] = pi->mv[pidx][REFP_1][MV_Y] - pi->mvp_scale[REFP_1][pi->refi[pidx][REFP_1]][pi->mvp_idx[pidx][REFP_1]][MV_Y];
    }
    else
    {
        pidx = (pi->curr_bi == 3 ? PRED_BI_REF : PRED_BI) + mvr_offset;

        if(cost_inter[PRED_L0 + mvr_offset] <= cost_inter[PRED_L1 + mvr_offset])
        {
            lidx_ref = REFP_0;
            lidx_cnd = REFP_1;
            pidx_ref = PRED_L0 + mvr_offset;
            pidx_cnd = PRED_L1 + mvr_offset;
        }
        else
        {
            lidx_ref = REFP_1;
            lidx_cnd = REFP_0;
            pidx_ref = PRED_L1 + mvr_offset;
            pidx_cnd = PRED_L0 + mvr_offset;
        }
        pi->mvr_idx[pidx] = pi->curr_mvr;

        if(ctx->sps.tool_admvp == 1)
        {
            pi->mvp_idx[pidx][REFP_0] = 0;
            pi->mvp_idx[pidx][REFP_1] = 0;
        }
        else
        {
            pi->mvp_idx[pidx][REFP_0] = pi->mvp_idx[PRED_L0][REFP_0];
            pi->mvp_idx[pidx][REFP_1] = pi->mvp_idx[PRED_L1][REFP_1];
        }
        pi->refi[pidx][REFP_0] = pi->refi[PRED_L0 + mvr_offset][REFP_0];
        pi->refi[pidx][REFP_1] = pi->refi[PRED_L1 + mvr_offset][REFP_1];

        pi->bi_idx[pidx] = bi_idx;
        if(pi->curr_bi == 3)
        {
            if(XEVE_ABS(pi->mvp_scale[lidx_ref][pi->refi[pidx][lidx_ref]][pi->mvp_idx[pidx][lidx_ref]][MV_X] - pi->mv[pidx_ref][lidx_ref][MV_X]) < 3 &&
               XEVE_ABS(pi->mvp_scale[lidx_ref][pi->refi[pidx][lidx_ref]][pi->mvp_idx[pidx][lidx_ref]][MV_Y] - pi->mv[pidx_ref][lidx_ref][MV_Y]) < 3 &&
               XEVE_ABS(pi->mvp_scale[lidx_cnd][pi->refi[pidx][lidx_cnd]][pi->mvp_idx[pidx][lidx_cnd]][MV_X] - pi->mv[pidx_cnd][lidx_cnd][MV_X]) < 3 &&
               XEVE_ABS(pi->mvp_scale[lidx_cnd][pi->refi[pidx][lidx_cnd]][pi->mvp_idx[pidx][lidx_cnd]][MV_Y] - pi->mv[pidx_cnd][lidx_cnd][MV_Y]) < 3)
            {
                return MAX_COST;
            }
            pi->mv[pidx][lidx_ref][MV_X] = pi->mvp_scale[lidx_ref][pi->refi[pidx][lidx_ref]][pi->mvp_idx[pidx][lidx_ref]][MV_X];
            pi->mv[pidx][lidx_ref][MV_Y] = pi->mvp_scale[lidx_ref][pi->refi[pidx][lidx_ref]][pi->mvp_idx[pidx][lidx_ref]][MV_Y];
            pi->mv[pidx][lidx_cnd][MV_X] = pi->mvp_scale[lidx_cnd][pi->refi[pidx][lidx_cnd]][pi->mvp_idx[pidx][lidx_cnd]][MV_X];
            pi->mv[pidx][lidx_cnd][MV_Y] = pi->mvp_scale[lidx_cnd][pi->refi[pidx][lidx_cnd]][pi->mvp_idx[pidx][lidx_cnd]][MV_Y];
        }
        else
        {
            pi->mv[pidx][lidx_ref][MV_X] = pi->mv[pidx_ref][lidx_ref][MV_X];
            pi->mv[pidx][lidx_ref][MV_Y] = pi->mv[pidx_ref][lidx_ref][MV_Y];
            pi->mv[pidx][lidx_cnd][MV_X] = pi->mv[pidx_cnd][lidx_cnd][MV_X];
            pi->mv[pidx][lidx_cnd][MV_Y] = pi->mv[pidx_cnd][lidx_cnd][MV_Y];
        }

        /* get MVP lidx_cnd */
        org = pi->o[Y_C] + x + y * pi->s_o[Y_C];
        pred = pi->pred[pidx];

        t0 = (lidx_ref == REFP_0) ? pi->refi[pidx][lidx_ref] : REFI_INVALID;
        t1 = (lidx_ref == REFP_1) ? pi->refi[pidx][lidx_ref] : REFI_INVALID;
        SET_REFI(refi, t0, t1);


        for(i = 0; i < BI_ITER; i++)
        {
            /* predict reference */
            pi->fn_mc(ctx, core, x, y, cuw, cuh, refi, pi->mv[pidx], pi->refp, pred, 0, FALSE, NULL);

            get_org_bi(org, pred[0][Y_C], pi->s_o[Y_C], cuw, cuh, pi->org_bi);

            SWAP(refi[lidx_ref], refi[lidx_cnd], t0);
            SWAP(lidx_ref, lidx_cnd, t0);
            SWAP(pidx_ref, pidx_cnd, t0);

            mvp_idx = pi->mvp_idx[pidx][lidx_ref];
            changed = 0;

            if(pi->curr_bi == 3)
            {
                bi_start = refi[lidx_ref];
                bi_end = refi[lidx_ref] + 1;
            }

            for(refi_cur = bi_start; refi_cur < bi_end; refi_cur++)
            {
                refi[lidx_ref] = refi_cur;
                mecost = pi->fn_me(pi, x, y, log2_cuw, log2_cuh, &refi[lidx_ref], lidx_ref, pi->mvp[lidx_ref][mvp_idx]
                                 , pi->mv_scale[lidx_ref][refi_cur], 1, ctx->sps.bit_depth_luma_minus8 + 8);
                if(mecost < best_mecost)
                {
                    refi_best = refi_cur;
                    best_mecost = mecost;

                    changed = 1;
                    t0 = (lidx_ref == REFP_0) ? refi_best : pi->refi[pidx][lidx_cnd];
                    t1 = (lidx_ref == REFP_1) ? refi_best : pi->refi[pidx][lidx_cnd];
                    SET_REFI(pi->refi[pidx], t0, t1);

                    pi->mv[pidx][lidx_ref][MV_X] = pi->mv_scale[lidx_ref][refi_cur][MV_X];
                    pi->mv[pidx][lidx_ref][MV_Y] = pi->mv_scale[lidx_ref][refi_cur][MV_Y];
                }
            }

            t0 = (lidx_ref == REFP_0) ? refi_best : REFI_INVALID;
            t1 = (lidx_ref == REFP_1) ? refi_best : REFI_INVALID;
            SET_REFI(refi, t0, t1);

            if(!changed)
            {
                break;
            }
        }

        pi->mv[pidx][REFP_0][MV_X] = (pi->mv[pidx][REFP_0][MV_X] >> pi->curr_mvr) << pi->curr_mvr;
        pi->mv[pidx][REFP_0][MV_Y] = (pi->mv[pidx][REFP_0][MV_Y] >> pi->curr_mvr) << pi->curr_mvr;
        pi->mv[pidx][REFP_1][MV_X] = (pi->mv[pidx][REFP_1][MV_X] >> pi->curr_mvr) << pi->curr_mvr;
        pi->mv[pidx][REFP_1][MV_Y] = (pi->mv[pidx][REFP_1][MV_Y] >> pi->curr_mvr) << pi->curr_mvr;

        pi->mvd[pidx][REFP_0][MV_X] = pi->mv[pidx][REFP_0][MV_X] - pi->mvp_scale[REFP_0][pi->refi[pidx][REFP_0]][pi->mvp_idx[pidx][REFP_0]][MV_X];
        pi->mvd[pidx][REFP_0][MV_Y] = pi->mv[pidx][REFP_0][MV_Y] - pi->mvp_scale[REFP_0][pi->refi[pidx][REFP_0]][pi->mvp_idx[pidx][REFP_0]][MV_Y];
        pi->mvd[pidx][REFP_1][MV_X] = pi->mv[pidx][REFP_1][MV_X] - pi->mvp_scale[REFP_1][pi->refi[pidx][REFP_1]][pi->mvp_idx[pidx][REFP_1]][MV_X];
        pi->mvd[pidx][REFP_1][MV_Y] = pi->mv[pidx][REFP_1][MV_Y] - pi->mvp_scale[REFP_1][pi->refi[pidx][REFP_1]][pi->mvp_idx[pidx][REFP_1]][MV_Y];
    }

    cost = pinter_residue_rdo(ctx, core, x, y, log2_cuw, log2_cuh, pi->pred[pidx], pi->coef[pidx], pidx, pi->mvp_idx[pidx], FALSE);

    xeve_mcpy(pi->nnz_best[pidx], core->nnz, sizeof(int) * N_C);
    xeve_mcpy(pi->nnz_sub_best[pidx], core->nnz_sub, sizeof(int) * N_C * MAX_SUB_TB_NUM);

    pi->fn_save_best_info(ctx, core, pidx);

    return cost;
}

void solve_equal(double(*equal_coeff)[7], int order, double* affine_para)
{
    int i, j, k;

    // row echelon
    for(i = 1; i < order; i++)
    {
        // find column max
        double temp = fabs(equal_coeff[i][i - 1]);
        int temp_idx = i;
        for(j = i + 1; j < order + 1; j++)
        {
            if(fabs(equal_coeff[j][i - 1]) > temp)
            {
                temp = fabs(equal_coeff[j][i - 1]);
                temp_idx = j;
            }
        }

        // swap line
        if(temp_idx != i)
        {
            for(j = 0; j < order + 1; j++)
            {
                equal_coeff[0][j] = equal_coeff[i][j];
                equal_coeff[i][j] = equal_coeff[temp_idx][j];
                equal_coeff[temp_idx][j] = equal_coeff[0][j];
            }
        }

        // elimination first column
        for(j = i + 1; j < order + 1; j++)
        {
            for(k = i; k < order + 1; k++)
            {
                equal_coeff[j][k] = equal_coeff[j][k] - equal_coeff[i][k] * equal_coeff[j][i - 1] / equal_coeff[i][i - 1];
            }
        }
    }

    affine_para[order - 1] = equal_coeff[order][order] / equal_coeff[order][order - 1];
    for(i = order - 2; i >= 0; i--)
    {
        double temp = 0;
        for(j = i + 1; j < order; j++)
        {
            temp += equal_coeff[i + 1][j] * affine_para[j];
        }
        affine_para[i] = (equal_coeff[i + 1][order] - temp) / equal_coeff[i + 1][i];
    }
}

static int get_affine_mv_bits(s16 mv[VER_NUM][MV_D], s16 mvp[VER_NUM][MV_D], int num_refp, int refi, int vertex_num)
{
    int bits = 0;
    int vertex;

    int b_zero = 1;
    bits = 1;
    for(vertex = 0; vertex < vertex_num; vertex++)
    {
        int mvd_x = mv[vertex][MV_X] - mvp[vertex][MV_X];
        int mvd_y = mv[vertex][MV_Y] - mvp[vertex][MV_Y];
        if(mvd_x != 0 || mvd_y != 0)
        {
            b_zero = 0;
            break;
        }
    }
    if(b_zero)
    {
        return bits;
    }

    for(vertex = 0; vertex < vertex_num; vertex++)
    {
        int mvd_x = mv[vertex][MV_X] - mvp[vertex][MV_X];
        int mvd_y = mv[vertex][MV_Y] - mvp[vertex][MV_Y];
        if(vertex)
        {
            mvd_x -= (mv[0][MV_X] - mvp[0][MV_X]);
            mvd_y -= (mv[0][MV_Y] - mvp[0][MV_Y]);
        }
        bits += (mvd_x > 2048 || mvd_x <= -2048) ? get_exp_golomb_bits(XEVE_ABS(mvd_x)) : xeve_tbl_mv_bits[mvd_x];
        bits += (mvd_y > 2048 || mvd_y <= -2048) ? get_exp_golomb_bits(XEVE_ABS(mvd_y)) : xeve_tbl_mv_bits[mvd_y];
    }
    bits += xeve_tbl_refi_bits[num_refp][refi];
    return bits;
}

static u32 pinter_affine_me_gradient(XEVE_PINTER * pi, int x, int y, int log2_cuw, int log2_cuh, s8 * refi, int lidx, s16 mvp[VER_NUM][MV_D]
                                   , s16 mv[VER_NUM][MV_D], int bi, int vertex_num, pel *tmp_buffer_for_eif, int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc)
{
    s16 mvt[VER_NUM][MV_D];
    s16 mvd[VER_NUM][MV_D];

    int cuw = 1 << log2_cuw;
    int cuh = 1 << log2_cuh;

    u32 cost, cost_best = XEVE_UINT32_MAX;

    s8 ri = *refi;
    XEVE_PIC* refp = pi->refp[ri][lidx].pic;

    pel *pred = pi->pred_buf;
    pel *org = bi ? pi->org_bi : (pi->o[Y_C] + x + y * pi->s_o[Y_C]);
    pel s_org = bi ? cuw : pi->s_o[Y_C];

    int mv_bits, best_bits;
    int vertex, iter;
    int iter_num = bi ? AF_ITER_BI : AF_ITER_UNI;
    int para_num = (vertex_num << 1) + 1;
    int affine_param_num = para_num - 1;

    double affine_para[6];
    double delta_mv[6];

    s64    equal_coeff_t[7][7];
    double equal_coeff[7][7];

    pel    *error = pi->p_error;
    int    *derivate[2];
    derivate[0] = pi->i_gradient[0];
    derivate[1] = pi->i_gradient[1];

    cuw = 1 << log2_cuw;
    cuh = 1 << log2_cuh;

    /* set start mv */
    for(vertex = 0; vertex < vertex_num; vertex++)
    {
        mvt[vertex][MV_X] = mv[vertex][MV_X];
        mvt[vertex][MV_Y] = mv[vertex][MV_Y];
        mvd[vertex][MV_X] = 0;
        mvd[vertex][MV_Y] = 0;
    }

    /* do motion compensation with start mv */
    xeve_affine_mc_l(x, y, refp->w_l, refp->h_l, cuw, cuh, mvt, refp, pred, vertex_num
                   , tmp_buffer_for_eif, bit_depth_luma, bit_depth_chroma, chroma_format_idc);

    /* get mvd bits*/
    best_bits = get_affine_mv_bits(mvt, mvp, pi->num_refp, ri, vertex_num);
    if(bi)
    {
        best_bits += pi->mot_bits[1 - lidx];
    }
    cost_best = MV_COST(pi, best_bits);

    /* get satd */
    cost_best += xeve_satd_16b(log2_cuw, log2_cuh, org, pred, s_org, cuw, bit_depth_luma) >> bi;

    if(vertex_num == 3)
    {
        iter_num = bi ? (AF_ITER_BI - 2) : (AF_ITER_UNI - 2);
    }

    for(iter = 0; iter < iter_num; iter++)
    {
        int row, col;
        int all_zero = 0;

        xeve_diff_16b(log2_cuw, log2_cuh, org, pred, s_org, cuw, cuw, error, bit_depth_luma);

        // sobel x direction
        // -1 0 1
        // -2 0 2
        // -1 0 1

        xevem_func_aff_h_sobel_flt(pred, cuw, derivate[0], cuw, cuw, cuh);

        // sobel y direction
        // -1 -2 -1
        //  0  0  0
        //  1  2  1

        xevem_func_aff_v_sobel_flt(pred, cuw, derivate[1], cuw, cuw, cuh);

        // solve delta x and y
        for(row = 0; row < para_num; row++)
        {
            xeve_mset(&equal_coeff_t[row][0], 0, para_num * sizeof(s64));
        }

        xevem_func_aff_eq_coef_comp(error, cuw, derivate, cuw, equal_coeff_t, cuw, cuh, vertex_num);
        for(row = 0; row < para_num; row++)
        {
            for(col = 0; col < para_num; col++)
            {
                equal_coeff[row][col] = (double)equal_coeff_t[row][col];
            }
        }
        solve_equal(equal_coeff, affine_param_num, affine_para);

        // convert to delta mv
        if(vertex_num == 3)
        {
            // for MV0
            delta_mv[0] = affine_para[0];
            delta_mv[2] = affine_para[2];
            // for MV1
            delta_mv[1] = affine_para[1] * cuw + affine_para[0];
            delta_mv[3] = affine_para[3] * cuw + affine_para[2];
            // for MV2
            delta_mv[4] = affine_para[4] * cuh + affine_para[0];
            delta_mv[5] = affine_para[5] * cuh + affine_para[2];

            mvd[0][MV_X] = (s16)(delta_mv[0] * 4 + (delta_mv[0] >= 0 ? 0.5 : -0.5));
            mvd[0][MV_Y] = (s16)(delta_mv[2] * 4 + (delta_mv[2] >= 0 ? 0.5 : -0.5));
            mvd[1][MV_X] = (s16)(delta_mv[1] * 4 + (delta_mv[1] >= 0 ? 0.5 : -0.5));
            mvd[1][MV_Y] = (s16)(delta_mv[3] * 4 + (delta_mv[3] >= 0 ? 0.5 : -0.5));
            mvd[2][MV_X] = (s16)(delta_mv[4] * 4 + (delta_mv[4] >= 0 ? 0.5 : -0.5));
            mvd[2][MV_Y] = (s16)(delta_mv[5] * 4 + (delta_mv[5] >= 0 ? 0.5 : -0.5));
        }
        else
        {
            // for MV0
            delta_mv[0] = affine_para[0];
            delta_mv[2] = affine_para[2];
            // for MV1
            delta_mv[1] = affine_para[1] * cuw + affine_para[0];
            delta_mv[3] = -affine_para[3] * cuw + affine_para[2];

            mvd[0][MV_X] = (s16)(delta_mv[0] * 4 + (delta_mv[0] >= 0 ? 0.5 : -0.5));
            mvd[0][MV_Y] = (s16)(delta_mv[2] * 4 + (delta_mv[2] >= 0 ? 0.5 : -0.5));
            mvd[1][MV_X] = (s16)(delta_mv[1] * 4 + (delta_mv[1] >= 0 ? 0.5 : -0.5));
            mvd[1][MV_Y] = (s16)(delta_mv[3] * 4 + (delta_mv[3] >= 0 ? 0.5 : -0.5));
        }

        // check early terminate
        for(vertex = 0; vertex < vertex_num; vertex++)
        {
            if(mvd[vertex][MV_X] != 0 || mvd[vertex][MV_Y] != 0)
            {
                all_zero = 0;
                break;
            }
            all_zero = 1;
        }
        if(all_zero)
        {
            break;
        }

        /* update mv */
        for(vertex = 0; vertex < vertex_num; vertex++)
        {
            mvt[vertex][MV_X] += mvd[vertex][MV_X];
            mvt[vertex][MV_Y] += mvd[vertex][MV_Y];
        }

        /* do motion compensation with updated mv */
        xeve_affine_mc_l(x, y, refp->w_l, refp->h_l, cuw, cuh, mvt, refp, pred, vertex_num, tmp_buffer_for_eif, bit_depth_luma, bit_depth_chroma, chroma_format_idc);

        /* get mvd bits*/
        mv_bits = get_affine_mv_bits(mvt, mvp, pi->num_refp, ri, vertex_num);
        if(bi)
        {
            mv_bits += pi->mot_bits[1 - lidx];
        }
        cost = MV_COST(pi, mv_bits);

        /* get satd */
        cost += xeve_satd_16b(log2_cuw, log2_cuh, org, pred, s_org, cuw, bit_depth_luma) >> bi;

        /* save best mv */
        if(cost < cost_best)
        {
            cost_best = cost;
            best_bits = mv_bits;
            for(vertex = 0; vertex < vertex_num; vertex++)
            {
                mv[vertex][MV_X] = mvt[vertex][MV_X];
                mv[vertex][MV_Y] = mvt[vertex][MV_Y];
            }
        }
    }

    return (cost_best - MV_COST(pi, best_bits));
}

static void check_best_affine_mvp(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s8 refi[REFP_NUM],
                                  int lidx, int pidx, s16(*mvp)[VER_NUM][MV_D], s16(*mv)[MV_D], s16(*mvd)[MV_D], u8* mvp_idx, int vertex_num)
{
    double cost, best_cost;
    int idx, best_idx;
    int vertex;
    u32 bit_cnt;
    s16 mvd_tmp[REFP_NUM][VER_NUM][MV_D];

    SBAC_LOAD(core->s_temp_run, core->s_curr_best[core->log2_cuw - 2][core->log2_cuh - 2]);
    xeve_sbac_bit_reset(&core->s_temp_run);

    for(vertex = 0; vertex < vertex_num; vertex++)
    {
        mvd_tmp[lidx][vertex][MV_X] = mv[vertex][MV_X] - mvp[*mvp_idx][vertex][MV_X];
        mvd_tmp[lidx][vertex][MV_Y] = mv[vertex][MV_Y] - mvp[*mvp_idx][vertex][MV_Y];
        if(vertex)
        {
            mvd_tmp[lidx][vertex][MV_X] -= mvd_tmp[lidx][0][MV_X];
            mvd_tmp[lidx][vertex][MV_Y] -= mvd_tmp[lidx][0][MV_Y];
        }
    }
    xeve_rdo_bit_cnt_affine_mvp(ctx, core, slice_type, refi, mvd_tmp, pidx, *mvp_idx, vertex_num);
    bit_cnt = xeve_get_bit_number(&core->s_temp_run);

    best_cost = RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt);

    best_idx = *mvp_idx;

    for(idx = 0; idx < AFF_MAX_NUM_MVP; idx++)
    {
        if(idx == *mvp_idx)
        {
            continue;
        }

        SBAC_LOAD(core->s_temp_run, core->s_curr_best[core->log2_cuw - 2][core->log2_cuh - 2]);
        xeve_sbac_bit_reset(&core->s_temp_run);

        for(vertex = 0; vertex < vertex_num; vertex++)
        {
            mvd_tmp[lidx][vertex][MV_X] = mv[vertex][MV_X] - mvp[idx][vertex][MV_X];
            mvd_tmp[lidx][vertex][MV_Y] = mv[vertex][MV_Y] - mvp[idx][vertex][MV_Y];
            if(vertex)
            {
                mvd_tmp[lidx][vertex][MV_X] -= mvd_tmp[lidx][0][MV_X];
                mvd_tmp[lidx][vertex][MV_Y] -= mvd_tmp[lidx][0][MV_Y];
            }
        }
        xeve_rdo_bit_cnt_affine_mvp(ctx, core, slice_type, refi, mvd_tmp, pidx, idx, vertex_num);
        bit_cnt = xeve_get_bit_number(&core->s_temp_run);

        cost = RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt);
        if(cost < best_cost)
        {
            best_idx = idx;
        }
    }

    *mvp_idx = best_idx;
    for(vertex = 0; vertex < vertex_num; vertex++)
    {
        mvd[vertex][MV_X] = mv[vertex][MV_X] - mvp[*mvp_idx][vertex][MV_X];
        mvd[vertex][MV_Y] = mv[vertex][MV_Y] - mvp[*mvp_idx][vertex][MV_Y];
    }
}

static double analyze_affine_bi(XEVE_CTX * ctx, XEVE_CORE * core, XEVE_PINTER * pi,
                                int x, int y, int log2_cuw, int log2_cuh, double * cost_inter, int pred_mode, int vertex_num)
{
    XEVEM_CORE *mcore = (XEVEM_CORE*)core;
    s8          refi[REFP_NUM] = {REFI_INVALID, REFI_INVALID};
    int         t1;
    u32         best_mecost = XEVE_UINT32_MAX;
    int         refi_best = 0, refi_cur;
    int         changed = 0;
    u32         mecost;
    pel        *org;
    pel(*pred)[N_C][MAX_CU_DIM];
    int         cuw, cuh, t0;
    double      cost;
    int         lidx_ref, lidx_cnd;
    u8          mvp_idx = 0;
    int         pidx, pidx_ref, pidx_cnd, i;
    int         vertex;
    int         mebits;

    {
        cuw = (1 << log2_cuw);
        cuh = (1 << log2_cuh);

        if(vertex_num == 3)
        {
            pidx = AFF_6_BI;
            if(cost_inter[AFF_6_L0] <= cost_inter[AFF_6_L1])
            {
                lidx_ref = REFP_0;
                lidx_cnd = REFP_1;
                pidx_ref = AFF_6_L0;
                pidx_cnd = AFF_6_L1;
            }
            else
            {
                lidx_ref = REFP_1;
                lidx_cnd = REFP_0;
                pidx_ref = AFF_6_L1;
                pidx_cnd = AFF_6_L0;
            }
            pi->mvp_idx[pidx][REFP_0] = pi->mvp_idx[AFF_6_L0][REFP_0];
            pi->mvp_idx[pidx][REFP_1] = pi->mvp_idx[AFF_6_L1][REFP_1];
            pi->refi[pidx][REFP_0] = pi->refi[AFF_6_L0][REFP_0];
            pi->refi[pidx][REFP_1] = pi->refi[AFF_6_L1][REFP_1];
        }
        else
        {
            pidx = AFF_BI;
            if(cost_inter[AFF_L0] <= cost_inter[AFF_L1])
            {
                lidx_ref = REFP_0;
                lidx_cnd = REFP_1;
                pidx_ref = AFF_L0;
                pidx_cnd = AFF_L1;
            }
            else
            {
                lidx_ref = REFP_1;
                lidx_cnd = REFP_0;
                pidx_ref = AFF_L1;
                pidx_cnd = AFF_L0;
            }
            pi->mvp_idx[pidx][REFP_0] = pi->mvp_idx[AFF_L0][REFP_0];
            pi->mvp_idx[pidx][REFP_1] = pi->mvp_idx[AFF_L1][REFP_1];
            pi->refi[pidx][REFP_0] = pi->refi[AFF_L0][REFP_0];
            pi->refi[pidx][REFP_1] = pi->refi[AFF_L1][REFP_1];
        }

        for(vertex = 0; vertex < vertex_num; vertex++)
        {
            pi->affine_mv[pidx][lidx_ref][vertex][MV_X] = pi->affine_mv[pidx_ref][lidx_ref][vertex][MV_X];
            pi->affine_mv[pidx][lidx_ref][vertex][MV_Y] = pi->affine_mv[pidx_ref][lidx_ref][vertex][MV_Y];
            pi->affine_mv[pidx][lidx_cnd][vertex][MV_X] = pi->affine_mv[pidx_ref][lidx_cnd][vertex][MV_X];
            pi->affine_mv[pidx][lidx_cnd][vertex][MV_Y] = pi->affine_mv[pidx_ref][lidx_cnd][vertex][MV_Y];
        }

        /* get MVP lidx_cnd */
        org = pi->o[Y_C] + x + y * pi->s_o[Y_C];
        pred = pi->pred[pidx];

        t0 = (lidx_ref == REFP_0) ? pi->refi[pidx][lidx_ref] : REFI_INVALID;
        t1 = (lidx_ref == REFP_1) ? pi->refi[pidx][lidx_ref] : REFI_INVALID;
        SET_REFI(refi, t0, t1);

        for(i = 0; i < AFFINE_BI_ITER; i++)
        {
            /* predict reference */
            xeve_affine_mc(x, y, ctx->w, ctx->h, cuw, cuh, refi, pi->affine_mv[pidx], pi->refp, pred, vertex_num, mcore->eif_tmp_buffer
                         , ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc);

            get_org_bi(org, pred[0][Y_C], pi->s_o[Y_C], cuw, cuh, pi->org_bi);

            SWAP(refi[lidx_ref], refi[lidx_cnd], t0);
            SWAP(lidx_ref, lidx_cnd, t0);
            SWAP(pidx_ref, pidx_cnd, t0);

            mvp_idx = pi->mvp_idx[pidx][lidx_ref];
            changed = 0;
            for(refi_cur = 0; refi_cur < pi->num_refp; refi_cur++)
            {
                refi[lidx_ref] = refi_cur;
                mvp_idx = pi->mvp_idx_scale[lidx_ref][refi_cur];
                mecost = pi->fn_affine_me(pi, x, y, log2_cuw, log2_cuh, &refi[lidx_ref], lidx_ref, pi->affine_mvp_scale[lidx_ref][refi_cur][mvp_idx]
                                        , pi->affine_mv_scale[lidx_ref][refi_cur], 1, vertex_num, mcore->eif_tmp_buffer, ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc);

                // update MVP bits
                check_best_affine_mvp(ctx, core, pi->slice_type, refi, lidx_ref, pidx, pi->affine_mvp_scale[lidx_ref][refi_cur], pi->affine_mv_scale[lidx_ref][refi_cur], pi->affine_mvd[pidx][lidx_ref], &mvp_idx, vertex_num);

                mebits = get_affine_mv_bits(pi->affine_mv_scale[lidx_ref][refi_cur], pi->affine_mvp_scale[lidx_ref][refi_cur][mvp_idx], pi->num_refp, refi_cur, vertex_num);
                mebits += xeve_tbl_mvp_idx_bits[AFF_MAX_NUM_MVP][mvp_idx];
                mebits += pi->mot_bits[1 - lidx_ref]; // add opposite bits

                mecost += MV_COST(pi, mebits);
                pi->mvp_idx_scale[lidx_ref][refi_cur] = mvp_idx;

                if(mecost < best_mecost)
                {
                    pi->mot_bits[lidx_ref] = mebits - pi->mot_bits[1 - lidx_ref];
                    pi->mvp_idx[pidx][lidx_ref] = mvp_idx;
                    refi_best = refi_cur;
                    best_mecost = mecost;
                    changed = 1;
                    t0 = (lidx_ref == REFP_0) ? refi_best : pi->refi[pidx][lidx_cnd];
                    t1 = (lidx_ref == REFP_1) ? refi_best : pi->refi[pidx][lidx_cnd];
                    SET_REFI(pi->refi[pidx], t0, t1);

                    for(vertex = 0; vertex < vertex_num; vertex++)
                    {
                        pi->affine_mv[pidx][lidx_ref][vertex][MV_X] = pi->affine_mv_scale[lidx_ref][refi_cur][vertex][MV_X];
                        pi->affine_mv[pidx][lidx_ref][vertex][MV_Y] = pi->affine_mv_scale[lidx_ref][refi_cur][vertex][MV_Y];
                    }
                }
            }

            t0 = (lidx_ref == REFP_0) ? refi_best : REFI_INVALID;
            t1 = (lidx_ref == REFP_1) ? refi_best : REFI_INVALID;
            SET_REFI(refi, t0, t1);

            if(!changed) break;
        }

        for(vertex = 0; vertex < vertex_num; vertex++)
        {
            pi->affine_mvd[pidx][REFP_0][vertex][MV_X] = pi->affine_mv[pidx][REFP_0][vertex][MV_X] - pi->affine_mvp_scale[REFP_0][pi->refi[pidx][REFP_0]][pi->mvp_idx[pidx][REFP_0]][vertex][MV_X];
            pi->affine_mvd[pidx][REFP_0][vertex][MV_Y] = pi->affine_mv[pidx][REFP_0][vertex][MV_Y] - pi->affine_mvp_scale[REFP_0][pi->refi[pidx][REFP_0]][pi->mvp_idx[pidx][REFP_0]][vertex][MV_Y];
            pi->affine_mvd[pidx][REFP_1][vertex][MV_X] = pi->affine_mv[pidx][REFP_1][vertex][MV_X] - pi->affine_mvp_scale[REFP_1][pi->refi[pidx][REFP_1]][pi->mvp_idx[pidx][REFP_1]][vertex][MV_X];
            pi->affine_mvd[pidx][REFP_1][vertex][MV_Y] = pi->affine_mv[pidx][REFP_1][vertex][MV_Y] - pi->affine_mvp_scale[REFP_1][pi->refi[pidx][REFP_1]][pi->mvp_idx[pidx][REFP_1]][vertex][MV_Y];
            if(vertex)
            {
                pi->affine_mvd[pidx][REFP_0][vertex][MV_X] -= pi->affine_mvd[pidx][REFP_0][0][MV_X];
                pi->affine_mvd[pidx][REFP_0][vertex][MV_Y] -= pi->affine_mvd[pidx][REFP_0][0][MV_Y];
                pi->affine_mvd[pidx][REFP_1][vertex][MV_X] -= pi->affine_mvd[pidx][REFP_1][0][MV_X];
                pi->affine_mvd[pidx][REFP_1][vertex][MV_Y] -= pi->affine_mvd[pidx][REFP_1][0][MV_Y];
            }
        }
    }

    for(i = 0; i < REFP_NUM; i++)
    {
        if(vertex_num == 3)
        {
            pi->affine_mv[pidx][i][3][MV_X] = pi->affine_mv[pidx][i][1][MV_X] + pi->affine_mv[pidx][i][2][MV_X] - pi->affine_mv[pidx][i][0][MV_X];
            pi->affine_mv[pidx][i][3][MV_Y] = pi->affine_mv[pidx][i][1][MV_Y] + pi->affine_mv[pidx][i][2][MV_Y] - pi->affine_mv[pidx][i][0][MV_Y];
        }
        else
        {
            pi->affine_mv[pidx][i][2][MV_X] = pi->affine_mv[pidx][i][0][MV_X] - (pi->affine_mv[pidx][i][1][MV_Y] + pi->affine_mv[pidx][i][0][MV_Y]) * cuh / cuh;
            pi->affine_mv[pidx][i][2][MV_Y] = pi->affine_mv[pidx][i][0][MV_Y] + (pi->affine_mv[pidx][i][1][MV_X] + pi->affine_mv[pidx][i][0][MV_X]) * cuh / cuh;
            pi->affine_mv[pidx][i][3][MV_X] = pi->affine_mv[pidx][i][0][MV_X] - (pi->affine_mv[pidx][i][1][MV_Y] + pi->affine_mv[pidx][i][0][MV_Y]) * cuh / cuh;
            pi->affine_mv[pidx][i][3][MV_Y] = pi->affine_mv[pidx][i][0][MV_Y] + (pi->affine_mv[pidx][i][1][MV_X] + pi->affine_mv[pidx][i][0][MV_X]) * cuh / cuh;
        }
    }

    cost = pinter_residue_rdo(ctx, core, x, y, log2_cuw, log2_cuh, pi->pred[pidx], pi->coef[pidx], pidx, pi->mvp_idx[pidx], FALSE);
    xeve_mcpy(pi->nnz_best[pidx], core->nnz, sizeof(int) * N_C);
    xeve_mcpy(pi->nnz_sub_best[pidx], core->nnz_sub, sizeof(int) * N_C * MAX_SUB_TB_NUM);
    pi->ats_inter_info_mode[pidx] = mcore->ats_inter_info;

    return cost;

}

static double analyze_affine_merge(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh, int pidx)
{
    XEVEM_CTX   *mctx = (XEVEM_CTX *)ctx;
    XEVEM_CORE  *mcore = (XEVEM_CORE *)core;
    XEVE_PINTER *pi = &ctx->pinter[core->thread_cnt];
    pel          *y_org, *u_org, *v_org;
    s16          mrg_list_cp_mv[AFF_MAX_CAND][REFP_NUM][VER_NUM][MV_D];
    s8           mrg_list_refi[AFF_MAX_CAND][REFP_NUM];
    int          mrg_list_cp_num[AFF_MAX_CAND];
    double       cost, cost_best = MAX_COST;
    int          cuw, cuh, idx, bit_cnt, mrg_cnt, best_idx = 0;
    s64          cy, cu, cv;
    int          i, j;
    int          w_shift = ctx->param.cs_w_shift;
    int          h_shift = ctx->param.cs_h_shift;
    cy = cu = cv = 0;

    mcore->ats_inter_info = 0;
    if(ctx->pps.cu_qp_delta_enabled_flag)
    {
        if(core->cu_qp_delta_code_mode != 2)
        {
            xeve_set_qp(ctx, core, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].prev_qp);
        }
    }
    cuw = (1 << log2_cuw);
    cuh = (1 << log2_cuh);
    y_org = pi->o[Y_C] + x + y * pi->s_o[Y_C];
    u_org = pi->o[U_C] + (x >> w_shift) + ((y >> h_shift) * pi->s_o[U_C]);
    v_org = pi->o[V_C] + (x >> w_shift) + ((y >> h_shift) * pi->s_o[V_C]);

    mrg_cnt = xeve_get_affine_merge_candidate(ctx->poc.poc_val, ctx->slice_type, core->scup, ctx->map_refi, ctx->map_mv, pi->refp, cuw, cuh, ctx->w_scu, ctx->h_scu
                                            , core->avail_cu, mrg_list_refi, mrg_list_cp_mv, mrg_list_cp_num, ctx->map_scu, mctx->map_affine, ctx->log2_max_cuwh
                                            , ctx->map_unrefined_mv, core->avail_lr, ctx->sh, ctx->map_tidx);

    if(mrg_cnt == 0)
    {
        return MAX_COST;
    }

    for(idx = 0; idx < mrg_cnt; idx++)
    {
        for(i = 0; i < REFP_NUM; i++)
        {
            if(REFI_IS_VALID(mrg_list_refi[idx][i]))
            {
                if(mrg_list_cp_num[idx] == 3) // derive RB
                {
                    mrg_list_cp_mv[idx][i][3][MV_X] = mrg_list_cp_mv[idx][i][1][MV_X] + mrg_list_cp_mv[idx][i][2][MV_X] - mrg_list_cp_mv[idx][i][0][MV_X];
                    mrg_list_cp_mv[idx][i][3][MV_Y] = mrg_list_cp_mv[idx][i][1][MV_Y] + mrg_list_cp_mv[idx][i][2][MV_Y] - mrg_list_cp_mv[idx][i][0][MV_Y];
                }
                else // derive LB, RB
                {
                    mrg_list_cp_mv[idx][i][2][MV_X] = mrg_list_cp_mv[idx][i][0][MV_X] - (mrg_list_cp_mv[idx][i][1][MV_Y] - mrg_list_cp_mv[idx][i][0][MV_Y]) * (s16)cuh / (s16)cuw;
                    mrg_list_cp_mv[idx][i][2][MV_Y] = mrg_list_cp_mv[idx][i][0][MV_Y] + (mrg_list_cp_mv[idx][i][1][MV_X] - mrg_list_cp_mv[idx][i][0][MV_X]) * (s16)cuh / (s16)cuw;
                    mrg_list_cp_mv[idx][i][3][MV_X] = mrg_list_cp_mv[idx][i][1][MV_X] - (mrg_list_cp_mv[idx][i][1][MV_Y] - mrg_list_cp_mv[idx][i][0][MV_Y]) * (s16)cuh / (s16)cuw;
                    mrg_list_cp_mv[idx][i][3][MV_Y] = mrg_list_cp_mv[idx][i][1][MV_Y] + (mrg_list_cp_mv[idx][i][1][MV_X] - mrg_list_cp_mv[idx][i][0][MV_X]) * (s16)cuh / (s16)cuw;
                }
            }
        }

        // set motion information for MC
        mcore->affine_flag = mrg_list_cp_num[idx] - 1;
        pi->mvp_idx[pidx][REFP_0] = idx;
        pi->mvp_idx[pidx][REFP_1] = 0;
        for(j = 0; j < mrg_list_cp_num[idx]; j++)
        {
            pi->affine_mv[pidx][REFP_0][j][MV_X] = mrg_list_cp_mv[idx][REFP_0][j][MV_X];
            pi->affine_mv[pidx][REFP_0][j][MV_Y] = mrg_list_cp_mv[idx][REFP_0][j][MV_Y];
            pi->affine_mv[pidx][REFP_1][j][MV_X] = mrg_list_cp_mv[idx][REFP_1][j][MV_X];
            pi->affine_mv[pidx][REFP_1][j][MV_Y] = mrg_list_cp_mv[idx][REFP_1][j][MV_Y];
        }
        pi->refi[pidx][REFP_0] = mrg_list_refi[idx][REFP_0];
        pi->refi[pidx][REFP_1] = mrg_list_refi[idx][REFP_1];

        if(pidx == AFF_DIR)
        {
            cost = pinter_residue_rdo(ctx, core, x, y, log2_cuw, log2_cuh, pi->pred[PRED_NUM], pi->coef[PRED_NUM], pidx, pi->mvp_idx[pidx], FALSE);
        }
        else
        {
            assert(mcore->ats_inter_info == 0);
            xeve_affine_mc(x, y, ctx->w, ctx->h, cuw, cuh, mrg_list_refi[idx], mrg_list_cp_mv[idx], pi->refp, pi->pred[PRED_NUM], mrg_list_cp_num[idx], mcore->eif_tmp_buffer
                         , ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc);

            cy = xeve_ssd_16b(log2_cuw, log2_cuh, pi->pred[PRED_NUM][0][Y_C], y_org, cuw, pi->s_o[Y_C], ctx->sps.bit_depth_luma_minus8 + 8);
            if(ctx->sps.chroma_format_idc)
            {
                cu = xeve_ssd_16b(log2_cuw - w_shift, log2_cuh - h_shift, pi->pred[PRED_NUM][0][U_C], u_org, cuw >> w_shift, pi->s_o[U_C], ctx->sps.bit_depth_chroma_minus8 + 8);
                cv = xeve_ssd_16b(log2_cuw - w_shift, log2_cuh - h_shift, pi->pred[PRED_NUM][0][V_C], v_org, cuw >> w_shift, pi->s_o[V_C], ctx->sps.bit_depth_chroma_minus8 + 8);
            }

            if(ctx->param.rdo_dbk_switch)
            {
                xeve_set_affine_mvf(ctx, core, cuw, cuh, mrg_list_refi[idx], mrg_list_cp_mv[idx], mrg_list_cp_num[idx]);
                calc_delta_dist_filter_boundary(ctx, PIC_MODE(ctx), PIC_ORIG(ctx), cuw, cuh, pi->pred[PRED_NUM][0], cuw, x, y, core->avail_lr, 0, 0
                                                , mrg_list_refi[idx], pi->mv[pidx], 1, core);
                cy += core->delta_dist[Y_C];
                cu += core->delta_dist[U_C];
                cv += core->delta_dist[V_C];
            }
            cost = (double)cy + (core->dist_chroma_weight[0] * (double)cu) + (core->dist_chroma_weight[1] * (double)cv);

            SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
            DQP_LOAD(core->dqp_temp_run, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);

            xeve_sbac_bit_reset(&core->s_temp_run);
            xeve_rdo_bit_cnt_cu_skip_main(ctx, core, ctx->sh->slice_type, core->scup, idx, 0, 0, ctx->sps.tool_mmvd);

            bit_cnt = xeve_get_bit_number(&core->s_temp_run);
            cost += RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt);
            core->cost_best = cost < core->cost_best ? cost : core->cost_best;
        }

        // store best pred and coeff
        if(cost < cost_best)
        {
            cost_best = cost;
            best_idx = idx;

            xeve_mcpy(pi->nnz_best[pidx], core->nnz, sizeof(int) * N_C);
            xeve_mcpy(pi->nnz_sub_best[pidx], core->nnz_sub, sizeof(int) * N_C * MAX_SUB_TB_NUM);
            pi->ats_inter_info_mode[pidx] = mcore->ats_inter_info;

            for(j = 0; j < N_C; j++)
            {
                if(j != 0 && !ctx->sps.chroma_format_idc)
                    continue;
                int size_tmp = (cuw * cuh) >> (j == 0 ? 0 : (w_shift + h_shift));
                xeve_mcpy(pi->pred[pidx][0][j], pi->pred[PRED_NUM][0][j], size_tmp * sizeof(pel));
                xeve_mcpy(pi->coef[pidx][j], pi->coef[PRED_NUM][j], size_tmp * sizeof(s16));
            }

            SBAC_STORE(core->s_temp_best, core->s_temp_run);
            DQP_STORE(core->dqp_temp_best, core->dqp_temp_run);
        }
    }

    // set best motion information
    if(mrg_cnt >= 1)
    {
        mcore->affine_flag = mrg_list_cp_num[best_idx] - 1;

        pi->mvp_idx[pidx][REFP_0] = best_idx;
        pi->mvp_idx[pidx][REFP_1] = 0;
        for(j = 0; j < mrg_list_cp_num[best_idx]; j++)
        {
            pi->affine_mv[pidx][REFP_0][j][MV_X] = mrg_list_cp_mv[best_idx][REFP_0][j][MV_X];
            pi->affine_mv[pidx][REFP_0][j][MV_Y] = mrg_list_cp_mv[best_idx][REFP_0][j][MV_Y];
            pi->affine_mv[pidx][REFP_1][j][MV_X] = mrg_list_cp_mv[best_idx][REFP_1][j][MV_X];
            pi->affine_mv[pidx][REFP_1][j][MV_Y] = mrg_list_cp_mv[best_idx][REFP_1][j][MV_Y];
        }
        pi->refi[pidx][REFP_0] = mrg_list_refi[best_idx][REFP_0];
        pi->refi[pidx][REFP_1] = mrg_list_refi[best_idx][REFP_1];

        pi->mv[pidx][REFP_0][MV_X] = 0;
        pi->mv[pidx][REFP_0][MV_Y] = 0;
        pi->mv[pidx][REFP_1][MV_X] = 0;
        pi->mv[pidx][REFP_1][MV_Y] = 0;

        pi->mvd[pidx][REFP_0][MV_X] = 0;
        pi->mvd[pidx][REFP_0][MV_Y] = 0;
        pi->mvd[pidx][REFP_1][MV_X] = 0;
        pi->mvd[pidx][REFP_1][MV_Y] = 0;
    }

    return cost_best;
}

static double pinter_analyze_cu(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh, XEVE_MODE *mi, s16 coef[N_C][MAX_CU_DIM], pel *rec[N_C], int s_rec[N_C])
{
    XEVEM_CTX       *mctx = (XEVEM_CTX *)ctx;
    XEVEM_CORE      *mcore = (XEVEM_CORE *)core;
    XEVE_PINTER     *pi;
    s8              *refi;
    s8               refi_temp = 0;
    u32              mecost, best_mecost;
    pel(*pred)[N_C][MAX_CU_DIM];
    s16(*coef_t)[MAX_CU_DIM];
    s16(*mvp)[MV_D], *mv, *mvd;
    int              cuw, cuh, t0, t1, best_idx = PRED_SKIP, i, j;
    u8               mvp_idx[REFP_NUM] = {0, 0};
    s8               refi_cur = 0;
    double           cost, cost_best = MAX_COST;
    double           cost_inter[PRED_NUM];
    int              lidx, pidx;
    int              best_dmvr = 0;
    int              best_affine_mode = 0;
    u8               affine_applicable = 0;
    int              allow_affine = ctx->sps.tool_affine;
    int              mebits, best_bits = 0;
    int              save_translation_mv[REFP_NUM][XEVE_MAX_NUM_ACTIVE_REF_FRAME][MV_D];
    u32              cost_trans[REFP_NUM][XEVE_MAX_NUM_ACTIVE_REF_FRAME];
    s16              mv_trans[XEVE_MAX_NUM_ACTIVE_REF_FRAME][REFP_NUM][MV_D];
    s16              tmp_mv_array[VER_NUM][MV_D];
    int              k;
    int              REF_SET[REFP_NUM][XEVE_MAX_NUM_ACTIVE_REF_FRAME] = {{0,0,},};
    int              real_mv[MMVD_GRP_NUM * MMVD_BASE_MV_NUM * MMVD_MAX_REFINE_NUM][2][3];
    int              num_amvr = MAX_NUM_MVR;
    int              w_shift = ctx->param.cs_w_shift;
    int              h_shift = ctx->param.cs_h_shift;

    if(ctx->sps.tool_amvr)
    {
        if(mcore->bef_data[log2_cuw - 2][log2_cuh - 2][core->cup][core->bef_data_idx].visit)
        {
            num_amvr = mcore->bef_data[log2_cuw - 2][log2_cuh - 2][core->cup][core->bef_data_idx].mvr_idx + 1;

            if(num_amvr > MAX_NUM_MVR)
            {
                num_amvr = MAX_NUM_MVR;
            }
        }
    }
    else
    {
        num_amvr = 1; /* only allow 1/4 pel of resolution */
    }

    pi = &ctx->pinter[core->thread_cnt];

    cuw = (1 << log2_cuw);
    cuh = (1 << log2_cuh);

    mcore->affine_flag = 0;
    if(mcore->bef_data[log2_cuw - 2][log2_cuh - 2][core->cup][core->bef_data_idx].visit)
    {
        if(mcore->bef_data[log2_cuw - 2][log2_cuh - 2][core->cup][core->bef_data_idx].affine_flag == 0)
        {
            allow_affine = 0;
        }
    }

    // init translation mv for affine
    for(i = 0; i < REFP_NUM; i++)
    {
        for(j = 0; j < XEVE_MAX_NUM_ACTIVE_REF_FRAME; j++)
        {
            save_translation_mv[i][j][MV_X] = 0;
            save_translation_mv[i][j][MV_Y] = 0;
        }
    }

    for(i = 0; i < PRED_NUM; i++)
    {
        cost_inter[i] = MAX_COST;
        pi->mvr_idx[i] = 0;
        pi->bi_idx[i] = BI_NON;
    }

    affine_applicable = 1;

    if(ctx->sps.tool_mmvd && ((pi->slice_type == SLICE_B) || (pi->slice_type == SLICE_P)))
    {
        for(k = 0; k < XEVE_MAX_NUM_ACTIVE_REF_FRAME; k++)
        {
            REF_SET[0][k] = ctx->refp[k][0].poc;
            REF_SET[1][k] = ctx->refp[k][1].poc;
        }

        get_mmvd_mvp_list(ctx->map_refi, ctx->refp[0], ctx->map_mv, ctx->w_scu, ctx->h_scu, core->scup, core->avail_cu, log2_cuw, log2_cuh, ctx->slice_type, real_mv, ctx->map_scu, REF_SET, core->avail_lr
                        , ctx->poc.poc_val, ctx->rpm.num_refp
                        , &mcore->history_buffer, ctx->sps.tool_admvp, ctx->sh, ctx->log2_max_cuwh, ctx->map_tidx);

        mmvd_base_skip(ctx, core, real_mv, log2_cuw, log2_cuh, ctx->slice_type, core->scup, ctx->map_refi, ctx->map_mv, ctx->refp[0], ctx->w_scu, core->avail_cu, REF_SET
                     , ctx->h_scu, ctx->map_scu, core->avail_lr, &mcore->history_buffer, ctx->sps.tool_admvp, ctx->sh, ctx->log2_max_cuwh, ctx->poc.poc_val);
    }
    /* skip mode */
    cost = cost_inter[PRED_SKIP] = analyze_skip(ctx, core, x, y, log2_cuw, log2_cuh);
    if(cost < cost_best)
    {
        best_dmvr = mcore->dmvr_flag;
        mcore->dmvr_flag = 0;
        core->cu_mode = MODE_SKIP;
        best_idx = PRED_SKIP;
        cost_inter[best_idx] = cost_best = cost;
        SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
        DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);
        xeve_mset(pi->nnz_best[PRED_SKIP], 0, sizeof(int) * N_C);
        xeve_mcpy(pi->nnz_sub_best[PRED_SKIP], core->nnz_sub, sizeof(int) * N_C * MAX_SUB_TB_NUM);
    }

    cost = cost_inter[PRED_DIR] = analyze_merge(ctx, core, x, y, log2_cuw, log2_cuh);
    if(cost < cost_best)
    {
        core->cu_mode = MODE_DIR;
        best_idx = PRED_DIR;
        cost_inter[best_idx] = cost_best = cost;
        best_dmvr = mcore->dmvr_flag;
        mcore->dmvr_flag = 0;

        for(i = 0; i < N_C; i++)
        {
            if(i != 0 && !ctx->sps.chroma_format_idc)
                continue;
            int size_tmp = (cuw * cuh) >> (i == 0 ? 0 : w_shift + h_shift);
            xeve_mcpy(pi->pred[best_idx][0][i], pi->pred[PRED_NUM][0][i], size_tmp * sizeof(pel));
            xeve_mcpy(pi->coef[best_idx][i], pi->coef[PRED_NUM][i], size_tmp * sizeof(s16));
        }
        SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best_merge);
        DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best_merge);
    }

    if(ctx->sps.tool_mmvd && ((pi->slice_type == SLICE_B) || (pi->slice_type == SLICE_P)))
    {
        /* MMVD mode for merge */
        cost = cost_inter[PRED_DIR_MMVD] = analyze_merge_mmvd(ctx, core, x, y, log2_cuw, log2_cuh, real_mv);
        if(cost < cost_best)
        {
            core->cu_mode = MODE_DIR_MMVD;
            best_idx = PRED_DIR_MMVD;
            cost_inter[best_idx] = cost_best = cost;
            best_dmvr = 0;
            cost_best = cost;
            SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
            DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);
        }

        /* MMVD mode for skip */
        cost = cost_inter[PRED_SKIP_MMVD] = analyze_skip_mmvd(ctx, core, x, y, log2_cuw, log2_cuh, real_mv);
        if(cost < cost_best)
        {
            core->cu_mode = MODE_SKIP_MMVD;
            best_idx = PRED_SKIP_MMVD;
            best_dmvr = 0;
            cost_inter[best_idx] = cost_best = cost;
            SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
            DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);
            xeve_mset(pi->nnz_best[PRED_SKIP_MMVD], 0, sizeof(int) * N_C);
            xeve_mcpy(pi->nnz_sub_best[PRED_SKIP_MMVD], core->nnz_sub, sizeof(int) * N_C * MAX_SUB_TB_NUM);
        }
    }

#if ET_AMVP
    if(best_idx == PRED_SKIP)
    {
        //skip_flag + pred_mode + mrg_flag + ref_list_idx + ref_idx + amvp_idx + mvd_bits + cbf0
        //1           1           1          2              1         1          6          1    = 14
        int penalty = 5;
        int bits_th_uni = 14 + penalty;
        int base_dist = 1 << (log2_cuw + log2_cuh + 2);
        if(cost_best < (bits_th_uni * core->lambda[0] + base_dist))
            mode_skip_curr[MSL_LIS0] = mode_skip_curr[MSL_LIS1] = 1;

        //skip_flag + pred_mode + mrg_flag + ref_list_idx + (ref_idx + amvp_idx + mvd_bits) * 2 + cbf0
        //1           1           1          1              (1         1          6) * 2          1    = 21
        int bits_th_bi = 21 + penalty;
        if(cost_best < (bits_th_bi * core->lambda[0] + base_dist))
            mode_skip_curr[MSL_BI] = 1;
    }
    else if(best_idx == PRED_DIR)
    {
        //skip_flag + pred_mode + mrg_flag + ref_list_idx + ref_idx + amvp_idx + mvd_bits + cbf1 + coeff
        //1           1           1          2              1         1          6          3      5     = 21
        int penalty = 5;
        int bits_th_uni = 21 + penalty;
        int base_dist = 1 << (log2_cuw + log2_cuh + 2);
        if(cost_best < (bits_th_uni * core->lambda[0] + base_dist))
            mode_skip_curr[MSL_LIS0] = mode_skip_curr[MSL_LIS1] = 1;

        //skip_flag + pred_mode + mrg_flag + ref_list_idx + (ref_idx + amvp_idx + mvd_bits) * 2 + cbf1 + coeff
        //1           1           1          1              (1         1          6) * 2          3      5    = 28
        int bits_th_bi = 28 + penalty;
        if(cost_best < (bits_th_bi * core->lambda[0] + base_dist))
            mode_skip_curr[MSL_BI] = 1;
    }
#endif

    if(core->cu_mode != MODE_SKIP)
    {
        for(pi->curr_mvr = 0; pi->curr_mvr < num_amvr; pi->curr_mvr++)
        {
            const int mvr_offset = pi->curr_mvr * ORG_PRED_NUM;

            /* Motion Search *********************************************************/
            for(lidx = 0; lidx <= ((pi->slice_type == SLICE_P) ? PRED_L0 : PRED_L1); lidx++)
            {
                pidx = lidx + mvr_offset;
                pi->mvr_idx[pidx] = pi->curr_mvr;
                refi = pi->refi[pidx];
                mv = pi->mv[pidx][lidx];
                mvd = pi->mvd[pidx][lidx];
                pred = pi->pred[pidx];
                coef_t = pi->coef[pidx];
                pi->num_refp = ctx->rpm.num_refp[lidx];
                best_mecost = XEVE_UINT32_MAX;

                for(refi_cur = 0; refi_cur < pi->num_refp; refi_cur++)
                {
                    mvp = pi->mvp_scale[lidx][refi_cur];
                    xeve_get_motion_from_mvr(pi->curr_mvr, ctx->poc.poc_val, core->scup, lidx, refi_cur, pi->num_refp, ctx->map_mv, ctx->map_refi, pi->refp, core->cuw, core->cuh, ctx->w_scu, ctx->h_scu, core->avail_cu, mvp, pi->refi_pred[lidx], ctx->map_scu, core->avail_lr
                                             , ctx->map_unrefined_mv, &mcore->history_buffer, ctx->sps.tool_hmvp, ctx->map_tidx);
                    mvp_idx[lidx] = 0;

                    /* motion search ********************/
                    u8 skip_me = 0;
#if MODE_SAVE_LOAD_UPDATE
                    if(match_idx != -1)
                    {
                        if(history_data->ref_idx[match_idx][lidx] != refi_cur && history_data->ref_idx[match_idx][lidx] < 255)
                            skip_me = 1;
                    }
#endif
#if ET_ME_REFIDX1
                    int th_mvd = ctx->h >> 6;
                    if(refi_cur > 0 && best_mecost != XEVE_UINT32_MAX && abs(pi->mvd[lidx][0][MV_X] + pi->mvd[lidx][0][MV_Y]) < th_mvd)
                        skip_me = 1;
#endif

                    {
                        mecost = pi->fn_me(pi, x, y, log2_cuw, log2_cuh, &refi_cur, lidx, mvp[mvp_idx[lidx]], mv, 0, ctx->sps.bit_depth_luma_minus8 + 8);
                    }

                    pi->mv_scale[lidx][refi_cur][MV_X] = mv[MV_X];
                    pi->mv_scale[lidx][refi_cur][MV_Y] = mv[MV_Y];
                    if(mecost < best_mecost)
                    {
                        best_mecost = mecost;
                        refi_temp = refi_cur;
                    }

                    if(pi->curr_mvr == 0)
                    {
                        save_translation_mv[lidx][refi_cur][MV_X] = mv[MV_X];
                        save_translation_mv[lidx][refi_cur][MV_Y] = mv[MV_Y];
                    }
                }
#if MODE_SAVE_LOAD_UPDATE
                if(history_data->num_visit_save < NUM_MODE_SL_PATH && match_idx == -1)
                    history_data->ref_idx[history_data->num_visit_save][lidx] = refi_temp;
#endif

                refi_cur = refi_temp;
                mv[MV_X] = pi->mv_scale[lidx][refi_cur][MV_X];
                mv[MV_Y] = pi->mv_scale[lidx][refi_cur][MV_Y];
                mvp = pi->mvp_scale[lidx][refi_cur];

                t0 = (lidx == 0) ? refi_cur : REFI_INVALID;
                t1 = (lidx == 1) ? refi_cur : REFI_INVALID;
                SET_REFI(refi, t0, t1);

                mv[MV_X] = (mv[MV_X] >> pi->curr_mvr) << pi->curr_mvr;
                mv[MV_Y] = (mv[MV_Y] >> pi->curr_mvr) << pi->curr_mvr;

                mvd[MV_X] = mv[MV_X] - mvp[mvp_idx[lidx]][MV_X];
                mvd[MV_Y] = mv[MV_Y] - mvp[mvp_idx[lidx]][MV_Y];

                pi->mvp_idx[pidx][lidx] = mvp_idx[lidx];

                cost = cost_inter[pidx] = pinter_residue_rdo(ctx, core, x, y, log2_cuw, log2_cuh, pi->pred[PRED_NUM], pi->coef[PRED_NUM], pidx, mvp_idx, FALSE);
                if(cost < cost_best)
                {
                    core->cu_mode = MODE_INTER;
                    best_idx = pidx;
                    pi->mvr_idx[best_idx] = pi->curr_mvr;
                    pi->mvp_idx[best_idx][lidx] = mvp_idx[lidx];
                    cost_inter[best_idx] = cost_best = cost;
                    SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
                    DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);
                    best_dmvr = 0;

                    for(j = 0; j < N_C; j++)
                    {
                        if(j != 0 && !ctx->sps.chroma_format_idc)
                            continue;
                        int size_tmp = (cuw * cuh) >> (j == 0 ? 0 : w_shift + h_shift);
                        pi->nnz_best[pidx][j] = core->nnz[j];
                        xeve_mcpy(pi->nnz_sub_best[pidx][j], core->nnz_sub[j], sizeof(int) * MAX_SUB_TB_NUM);
                        xeve_mcpy(pred[0][j], pi->pred[PRED_NUM][0][j], size_tmp * sizeof(pel));
                        xeve_mcpy(coef_t[j], pi->coef[PRED_NUM][j], size_tmp * sizeof(s16));
                    }
                    pi->ats_inter_info_mode[pidx] = mcore->ats_inter_info;
                }
            }

            if(check_bi_applicability(pi->slice_type, cuw, cuh, ctx->sps.tool_admvp))
            {
                int max_num_bi = MAX_NUM_BI;
                int pred_mode = 0;

                if(mcore->bef_data[log2_cuw - 2][log2_cuh - 2][core->cup][core->bef_data_idx].visit)
                {
                    max_num_bi = (mcore->bef_data[log2_cuw - 2][log2_cuh - 2][core->cup][core->bef_data_idx].bi_idx == 2 ||
                                  mcore->bef_data[log2_cuw - 2][log2_cuh - 2][core->cup][core->bef_data_idx].bi_idx == 3) ? MAX_NUM_BI : 1;
                }

                for(pi->curr_bi = 0; pi->curr_bi < max_num_bi; pi->curr_bi++)
                {
                    if(pi->curr_bi > 0 && cost_inter[PRED_BI] > (1.17) * cost_inter[PRED_L0] && cost_inter[PRED_BI] > (1.17) * cost_inter[PRED_L1])
                    {
                        continue;
                    }
                    pred_mode = (pi->curr_bi == 0) ? PRED_BI : ((pi->curr_bi == 1) ? PRED_FL0_BI : (pi->curr_bi == 2) ? PRED_FL1_BI : PRED_BI_REF);
                    pidx = pred_mode + mvr_offset;
                    cost = cost_inter[pidx] = analyze_bi(ctx, core, x, y, log2_cuw, log2_cuh, cost_inter);
                    if(cost < cost_best)
                    {
                        core->cu_mode = MODE_INTER;
                        best_idx = pidx;
                        pi->mvr_idx[best_idx] = pi->curr_mvr;
                        pi->bi_idx[best_idx] = BI_NORMAL + (pi->curr_bi % 3);
                        cost_inter[best_idx] = cost_best = cost;
                        best_dmvr = 0;
                        SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
                        DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);
                    }
                }
            }

            if(pi->curr_mvr >= SKIP_MVR_IDX && ((core->cu_mode == MODE_SKIP) || (core->cu_mode == MODE_SKIP_MMVD)))
            {
                break;
            }

            if(pi->curr_mvr >= FAST_MVR_IDX)
            {
                if(abs(pi->mvd[best_idx][REFP_0][MV_X]) <= 0 &&
                   abs(pi->mvd[best_idx][REFP_0][MV_Y]) <= 0 &&
                   abs(pi->mvd[best_idx][REFP_1][MV_X]) <= 0 &&
                   abs(pi->mvd[best_idx][REFP_1][MV_Y]) <= 0)
                {
                    break;
                }
            }

            if(abs(pi->mv[best_idx][REFP_0][MV_X]) > abs(pi->mv[best_idx][REFP_1][MV_X]))
            {
                pi->max_imv[MV_X] = (abs(pi->mv[best_idx][REFP_0][MV_X]) + 1) >> 2;
                if(pi->mv[best_idx][REFP_0][MV_X] < 0)
                {
                    pi->max_imv[MV_X] = -1 * pi->max_imv[MV_X];
                }
            }
            else
            {
                pi->max_imv[MV_X] = (abs(pi->mv[best_idx][REFP_1][MV_X]) + 1) >> 2;
                if(pi->mv[best_idx][REFP_1][MV_X] < 0)
                {
                    pi->max_imv[MV_X] = -1 * pi->max_imv[MV_X];
                }
            }

            if(abs(pi->mv[best_idx][REFP_0][MV_Y]) > abs(pi->mv[best_idx][REFP_1][MV_Y]))
            {
                pi->max_imv[MV_Y] = (abs(pi->mv[best_idx][REFP_0][MV_Y]) + 1) >> 2;
                if(pi->mv[best_idx][REFP_0][MV_Y] < 0)
                {
                    pi->max_imv[MV_Y] = -1 * pi->max_imv[MV_Y];
                }
            }
            else
            {
                pi->max_imv[MV_Y] = (abs(pi->mv[best_idx][REFP_1][MV_Y]) + 1) >> 2;
                if(pi->mv[best_idx][REFP_1][MV_Y] < 0)
                {
                    pi->max_imv[MV_Y] = -1 * pi->max_imv[MV_Y];
                }
            }
        }
    }

    if(ctx->slice_depth < 4)
    {
        if(allow_affine && cuw >= 8 && cuh >= 8)
        {
            s16(*affine_mvp)[VER_NUM][MV_D], (*affine_mv)[MV_D], (*affine_mvd)[MV_D];
            int vertex = 0;
            int vertex_num;

            /* AFFINE skip mode */
            mcore->mmvd_flag = 0;
            cost = cost_inter[AFF_SKIP] = analyze_affine_merge(ctx, core, x, y, log2_cuw, log2_cuh, AFF_SKIP);

            if(cost < cost_best)
            {
                best_affine_mode = mcore->affine_flag;
                core->cu_mode = MODE_SKIP;
                best_idx = AFF_SKIP;
                best_dmvr = 0;
                cost_inter[best_idx] = cost_best = cost;
                SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
                DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);
                xeve_mset(pi->nnz_best[AFF_SKIP], 0, sizeof(int) * N_C);
                xeve_mset(pi->nnz_sub_best[AFF_SKIP], 0, sizeof(int) * N_C * MAX_SUB_TB_NUM);
            }

            /* AFFINE direct mode */
            cost = cost_inter[AFF_DIR] = analyze_affine_merge(ctx, core, x, y, log2_cuw, log2_cuh, AFF_DIR);

            if(cost < cost_best)
            {
                best_affine_mode = mcore->affine_flag;
                best_dmvr = 0;
                core->cu_mode = MODE_DIR;
                best_idx = AFF_DIR;
                cost_inter[best_idx] = cost_best = cost;
                SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
                DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);
            }

            if(affine_applicable && cuw >= 16 && cuh >= 16)
            {
                if(!(core->cu_mode == MODE_SKIP) && !(core->cu_mode == MODE_SKIP_MMVD)) //fast skip affine
                {
                    /* AFFINE 4 paramters Motion Search *********************************************************/
                    mcore->affine_flag = 1;
                    vertex_num = 2;
                    for(lidx = 0; lidx <= ((pi->slice_type == SLICE_P) ? PRED_L0 : PRED_L1); lidx++)
                    {
                        pidx = lidx + AFF_L0;
                        refi = pi->refi[pidx];
                        affine_mv = pi->affine_mv[pidx][lidx];
                        affine_mvd = pi->affine_mvd[pidx][lidx];

                        pred = pi->pred[pidx];
                        coef_t = pi->coef[pidx];
                        pi->num_refp = ctx->rpm.num_refp[lidx];

                        best_mecost = XEVE_UINT32_MAX;

                        for(refi_cur = 0; refi_cur < pi->num_refp; refi_cur++)
                        {
                            affine_mvp = pi->affine_mvp_scale[lidx][refi_cur];

                            xeve_get_affine_motion_scaling(ctx->poc.poc_val, core->scup, lidx, refi_cur, pi->num_refp, ctx->map_mv, ctx->map_refi, pi->refp
                                                           , core->cuw, core->cuh, ctx->w_scu, ctx->h_scu, core->avail_cu, affine_mvp, pi->refi_pred[lidx]
                                                           , ctx->map_scu, mctx->map_affine, vertex_num, core->avail_lr, ctx->log2_max_cuwh, ctx->map_unrefined_mv, ctx->map_tidx);

                            u32 mvp_best = XEVE_UINT32_MAX;
                            u32 mvp_temp = XEVE_UINT32_MAX;
                            s8  refi_t[REFP_NUM];
                            XEVE_PIC* refp = pi->refp[refi_cur][lidx].pic;
                            pel *pred = pi->pred_buf;
                            pel *org = pi->o[Y_C] + x + y * pi->s_o[Y_C];
                            pel s_org = pi->s_o[Y_C];

                            for(i = 0; i < AFF_MAX_NUM_MVP; i++)
                            {
                                xeve_affine_mc_l(x, y, refp->w_l, refp->h_l, cuw, cuh, affine_mvp[i], refp, pred, vertex_num, mcore->eif_tmp_buffer
                                               , ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc);

                                mvp_temp = xeve_satd_16b(log2_cuw, log2_cuh, org, pred, s_org, cuw, ctx->sps.bit_depth_luma_minus8 + 8);
                                mebits = 1; // zero mvd flag
                                mebits += xeve_tbl_mvp_idx_bits[AFF_MAX_NUM_MVP][i]; // mvp idx
                                mvp_temp += MV_COST(pi, mebits);

                                if(mvp_temp < mvp_best)
                                {
                                    mvp_idx[lidx] = i;
                                    mvp_best = mvp_temp;
                                }
                            }

                            mv_trans[refi_cur][lidx][MV_X] = save_translation_mv[lidx][refi_cur][MV_X];
                            mv_trans[refi_cur][lidx][MV_Y] = save_translation_mv[lidx][refi_cur][MV_Y];

                            refi_t[lidx] = refi_cur;
                            refi_t[1 - lidx] = -1;
                            xeve_mv_clip(x, y, ctx->w, ctx->h, cuw, cuh, refi_t, mv_trans[refi_cur], mv_trans[refi_cur]);

                            for(vertex = 0; vertex < vertex_num; vertex++)
                            {
                                tmp_mv_array[vertex][MV_X] = mv_trans[refi_cur][lidx][MV_X];
                                tmp_mv_array[vertex][MV_Y] = mv_trans[refi_cur][lidx][MV_Y];
                            }

                            xeve_affine_mc_l(x, y, refp->w_l, refp->h_l, cuw, cuh, tmp_mv_array, refp, pred, vertex_num, mcore->eif_tmp_buffer
                                           , ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc);

                            cost_trans[lidx][refi_cur] = xeve_satd_16b(log2_cuw, log2_cuh, org, pred, s_org, cuw, ctx->sps.bit_depth_luma_minus8 + 8);

                            mebits = get_affine_mv_bits(tmp_mv_array, affine_mvp[mvp_idx[lidx]], pi->num_refp, refi_cur, vertex_num);
                            mebits += xeve_tbl_mvp_idx_bits[AFF_MAX_NUM_MVP][mvp_idx[lidx]];
                            mvp_temp = cost_trans[lidx][refi_cur] + MV_COST(pi, mebits);

                            if(mvp_temp < mvp_best)
                            {
                                for(vertex = 0; vertex < vertex_num; vertex++)
                                {
                                    affine_mv[vertex][MV_X] = mv_trans[refi_cur][lidx][MV_X];
                                    affine_mv[vertex][MV_Y] = mv_trans[refi_cur][lidx][MV_Y];
                                }
                            }
                            else
                            {
                                for(vertex = 0; vertex < vertex_num; vertex++)
                                {
                                    affine_mv[vertex][MV_X] = affine_mvp[mvp_idx[lidx]][vertex][MV_X];
                                    affine_mv[vertex][MV_Y] = affine_mvp[mvp_idx[lidx]][vertex][MV_Y];
                                }
                            }

                            /* affine motion search */
                            mecost = pi->fn_affine_me(pi, x, y, log2_cuw, log2_cuh, &refi_cur, lidx, affine_mvp[mvp_idx[lidx]], affine_mv, 0, vertex_num, mcore->eif_tmp_buffer
                                                    , ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc);

                            // update MVP bits
                            t0 = (lidx == 0) ? refi_cur : REFI_INVALID;
                            t1 = (lidx == 1) ? refi_cur : REFI_INVALID;
                            SET_REFI(refi, t0, t1);
                            check_best_affine_mvp(ctx, core, pi->slice_type, refi, lidx, pidx, affine_mvp, affine_mv, affine_mvd, &mvp_idx[lidx], vertex_num);

                            mebits = get_affine_mv_bits(affine_mv, affine_mvp[mvp_idx[lidx]], pi->num_refp, refi_cur, vertex_num);
                            mebits += xeve_tbl_mvp_idx_bits[AFF_MAX_NUM_MVP][mvp_idx[lidx]];
                            mecost += MV_COST(pi, mebits);

                            pi->mvp_idx_scale[lidx][refi_cur] = mvp_idx[lidx];

                            /* save affine per ref me results */
                            for(vertex = 0; vertex < vertex_num; vertex++)
                            {
                                pi->affine_mv_scale[lidx][refi_cur][vertex][MV_X] = affine_mv[vertex][MV_X];
                                pi->affine_mv_scale[lidx][refi_cur][vertex][MV_Y] = affine_mv[vertex][MV_Y];
                            }
                            if(mecost < best_mecost)
                            {
                                best_mecost = mecost;
                                best_bits = mebits;
                                refi_temp = refi_cur;
                            }
                        }

                        /* save affine per list me results */
                        refi_cur = refi_temp;
                        for(vertex = 0; vertex < vertex_num; vertex++)
                        {
                            affine_mv[vertex][MV_X] = pi->affine_mv_scale[lidx][refi_cur][vertex][MV_X];
                            affine_mv[vertex][MV_Y] = pi->affine_mv_scale[lidx][refi_cur][vertex][MV_Y];
                        }

                        affine_mvp = pi->affine_mvp_scale[lidx][refi_cur];
                        t0 = (lidx == 0) ? refi_cur : REFI_INVALID;
                        t1 = (lidx == 1) ? refi_cur : REFI_INVALID;
                        SET_REFI(refi, t0, t1);

                        /* get affine mvd */
                        mvp_idx[lidx] = pi->mvp_idx_scale[lidx][refi_cur];
                        for(vertex = 0; vertex < vertex_num; vertex++)
                        {
                            affine_mvd[vertex][MV_X] = affine_mv[vertex][MV_X] - affine_mvp[mvp_idx[lidx]][vertex][MV_X];
                            affine_mvd[vertex][MV_Y] = affine_mv[vertex][MV_Y] - affine_mvp[mvp_idx[lidx]][vertex][MV_Y];
                            if(vertex)
                            {
                                affine_mvd[vertex][MV_X] -= affine_mvd[0][MV_X];
                                affine_mvd[vertex][MV_Y] -= affine_mvd[0][MV_Y];
                            }
                        }
                        pi->mot_bits[lidx] = best_bits;
                        pi->mvp_idx[pidx][lidx] = mvp_idx[lidx];

                        affine_mv[2][MV_X] = affine_mv[0][MV_X] - (affine_mv[1][MV_Y] - affine_mv[0][MV_Y]) * cuh / cuw;
                        affine_mv[2][MV_Y] = affine_mv[0][MV_Y] + (affine_mv[1][MV_X] - affine_mv[0][MV_X]) * cuh / cuw;
                        affine_mv[3][MV_X] = affine_mv[1][MV_X] - (affine_mv[1][MV_Y] - affine_mv[0][MV_Y]) * cuh / cuw;
                        affine_mv[3][MV_Y] = affine_mv[1][MV_Y] + (affine_mv[1][MV_X] - affine_mv[0][MV_X]) * cuh / cuw;

                        cost = cost_inter[pidx] = pinter_residue_rdo(ctx, core, x, y, log2_cuw, log2_cuh, pi->pred[PRED_NUM], pi->coef[PRED_NUM], pidx, mvp_idx, FALSE);


                        if(cost < cost_best)
                        {
                            best_affine_mode = mcore->affine_flag;
                            best_dmvr = 0;
                            core->cu_mode = MODE_INTER;
                            best_idx = pidx;
                            pi->mvp_idx[best_idx][lidx] = mvp_idx[lidx];
                            cost_inter[best_idx] = cost_best = cost;
                            pi->bi_idx[best_idx] = BI_NON;

                            SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
                            DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);

                            for(j = 0; j < N_C; j++)
                            {
                                if(j != 0 && !ctx->sps.chroma_format_idc)
                                    continue;
                                int size_tmp = (cuw * cuh) >> (j == 0 ? 0 : w_shift + h_shift);
                                pi->nnz_best[pidx][j] = core->nnz[j];
                                xeve_mcpy(pi->nnz_sub_best[pidx][j], core->nnz_sub[j], sizeof(int) * MAX_SUB_TB_NUM);
                                xeve_mcpy(pred[0][j], pi->pred[PRED_NUM][0][j], size_tmp * sizeof(pel));
                                xeve_mcpy(coef_t[j], pi->coef[PRED_NUM][j], size_tmp * sizeof(s16));
                            }
                            pi->ats_inter_info_mode[pidx] = mcore->ats_inter_info;
                        }
                    }

                    if(pi->slice_type == SLICE_B)
                    {
                        pidx = AFF_BI;
                        cost = cost_inter[pidx] = analyze_affine_bi(ctx, core, pi, x, y, log2_cuw, log2_cuh, cost_inter, AFF_BI, vertex_num);

                        if(cost < cost_best)
                        {
                            best_affine_mode = mcore->affine_flag;
                            best_dmvr = 0;
                            core->cu_mode = MODE_INTER;
                            best_idx = pidx;
                            cost_inter[best_idx] = cost_best = cost;
                            pi->bi_idx[best_idx] = BI_NORMAL;

                            SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
                            DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);
                        }
                    }

                    if((best_idx >= AFF_L0) && (best_idx <= AFF_6_BI))
                    {
                        /* AFFINE 6 paramters Motion Search *********************************************************/
                        mcore->affine_flag = 2;
                        vertex_num = 3;
                        for(lidx = 0; lidx <= ((pi->slice_type == SLICE_P) ? PRED_L0 : PRED_L1); lidx++)
                        {
                            pidx = lidx + AFF_6_L0;
                            refi = pi->refi[pidx];
                            affine_mv = pi->affine_mv[pidx][lidx];
                            affine_mvd = pi->affine_mvd[pidx][lidx];

                            pred = pi->pred[pidx];
                            coef_t = pi->coef[pidx];
                            pi->num_refp = ctx->rpm.num_refp[lidx];

                            best_mecost = XEVE_UINT32_MAX;

                            for(refi_cur = 0; refi_cur < pi->num_refp; refi_cur++)
                            {
                                affine_mvp = pi->affine_mvp_scale[lidx][refi_cur];

                                xeve_get_affine_motion_scaling(ctx->poc.poc_val, core->scup, lidx, refi_cur, pi->num_refp, ctx->map_mv, ctx->map_refi, pi->refp
                                                               , core->cuw, core->cuh, ctx->w_scu, ctx->h_scu, core->avail_cu, affine_mvp, pi->refi_pred[lidx]
                                                               , ctx->map_scu, mctx->map_affine, vertex_num, core->avail_lr, ctx->log2_max_cuwh, ctx->map_unrefined_mv, ctx->map_tidx);

                                u32 mvp_best = XEVE_UINT32_MAX;
                                u32 mvp_temp = XEVE_UINT32_MAX;

                                XEVE_PIC* refp = pi->refp[refi_cur][lidx].pic;
                                pel *pred = pi->pred_buf;
                                pel *org = pi->o[Y_C] + x + y * pi->s_o[Y_C];
                                pel s_org = pi->s_o[Y_C];
                                for(i = 0; i < AFF_MAX_NUM_MVP; i++)
                                {
                                    xeve_affine_mc_l(x, y, refp->w_l, refp->h_l, cuw, cuh, affine_mvp[i], refp, pred, vertex_num, mcore->eif_tmp_buffer
                                                   , ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc);

                                    mvp_temp = xeve_satd_16b(log2_cuw, log2_cuh, org, pred, s_org, cuw, ctx->sps.bit_depth_luma_minus8 + 8);

                                    mebits = 1; // zero mvd flag
                                    mebits += xeve_tbl_mvp_idx_bits[AFF_MAX_NUM_MVP][i]; // mvp idx
                                    mvp_temp += MV_COST(pi, mebits);

                                    if(mvp_temp < mvp_best)
                                    {
                                        mvp_idx[lidx] = i;
                                        mvp_best = mvp_temp;
                                    }
                                }

                                affine_mv[0][MV_X] = pi->affine_mv_scale[lidx][refi_cur][0][MV_X];
                                affine_mv[0][MV_Y] = pi->affine_mv_scale[lidx][refi_cur][0][MV_Y];
                                affine_mv[1][MV_X] = pi->affine_mv_scale[lidx][refi_cur][1][MV_X];
                                affine_mv[1][MV_Y] = pi->affine_mv_scale[lidx][refi_cur][1][MV_Y];
                                affine_mv[2][MV_X] = affine_mv[0][MV_X] - (affine_mv[1][MV_Y] - affine_mv[0][MV_Y]) * cuh / cuw;
                                affine_mv[2][MV_Y] = affine_mv[0][MV_Y] + (affine_mv[1][MV_X] - affine_mv[0][MV_X]) * cuh / cuw;
                                xeve_affine_mc_l(x, y, refp->w_l, refp->h_l, cuw, cuh, affine_mv, refp, pred, vertex_num, mcore->eif_tmp_buffer
                                               , ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc);

                                mvp_temp = xeve_satd_16b(log2_cuw, log2_cuh, org, pred, s_org, cuw, ctx->sps.bit_depth_luma_minus8 + 8);

                                // 4 parameter AFFINE MV
                                mebits = get_affine_mv_bits(affine_mv, affine_mvp[mvp_idx[lidx]], pi->num_refp, refi_cur, vertex_num);
                                mebits += xeve_tbl_mvp_idx_bits[AFF_MAX_NUM_MVP][mvp_idx[lidx]]; // mvp idx
                                mvp_temp += MV_COST(pi, mebits);
                                // translation MV
                                for(vertex = 0; vertex < vertex_num; vertex++)
                                {
                                    tmp_mv_array[vertex][MV_X] = mv_trans[refi_cur][lidx][MV_X];
                                    tmp_mv_array[vertex][MV_Y] = mv_trans[refi_cur][lidx][MV_Y];
                                }
                                mebits = get_affine_mv_bits(tmp_mv_array, affine_mvp[mvp_idx[lidx]], pi->num_refp, refi_cur, vertex_num);
                                mebits += xeve_tbl_mvp_idx_bits[AFF_MAX_NUM_MVP][mvp_idx[lidx]];
                                cost_trans[lidx][refi_cur] += MV_COST(pi, mebits);

                                if(mvp_best <= mvp_temp && mvp_best <= cost_trans[lidx][refi_cur])
                                {
                                    for(vertex = 0; vertex < vertex_num; vertex++)
                                    {
                                        affine_mv[vertex][MV_X] = affine_mvp[mvp_idx[lidx]][vertex][MV_X];
                                        affine_mv[vertex][MV_Y] = affine_mvp[mvp_idx[lidx]][vertex][MV_Y];
                                    }
                                }
                                else if(mvp_best <= mvp_temp && cost_trans[lidx][refi_cur] < mvp_best)
                                {
                                    for(vertex = 0; vertex < vertex_num; vertex++)
                                    {
                                        affine_mv[vertex][MV_X] = mv_trans[refi_cur][lidx][MV_X];
                                        affine_mv[vertex][MV_Y] = mv_trans[refi_cur][lidx][MV_Y];
                                    }
                                }

                                /* affine motion search */
                                mecost = pi->fn_affine_me(pi, x, y, log2_cuw, log2_cuh, &refi_cur, lidx, affine_mvp[mvp_idx[lidx]], affine_mv, 0, vertex_num, mcore->eif_tmp_buffer
                                                        , ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc);

                                // update ME bits
                                t0 = (lidx == 0) ? refi_cur : REFI_INVALID;
                                t1 = (lidx == 1) ? refi_cur : REFI_INVALID;
                                SET_REFI(refi, t0, t1);
                                check_best_affine_mvp(ctx, core, pi->slice_type, refi, lidx, pidx, affine_mvp, affine_mv, affine_mvd, &mvp_idx[lidx], vertex_num);
                                mebits = get_affine_mv_bits(affine_mv, affine_mvp[mvp_idx[lidx]], pi->num_refp, refi_cur, vertex_num);
                                mebits += xeve_tbl_mvp_idx_bits[AFF_MAX_NUM_MVP][mvp_idx[lidx]];
                                mecost += MV_COST(pi, mebits);

                                pi->mvp_idx_scale[lidx][refi_cur] = mvp_idx[lidx];

                                /* save affine per ref me results */
                                for(vertex = 0; vertex < vertex_num; vertex++)
                                {
                                    pi->affine_mv_scale[lidx][refi_cur][vertex][MV_X] = affine_mv[vertex][MV_X];
                                    pi->affine_mv_scale[lidx][refi_cur][vertex][MV_Y] = affine_mv[vertex][MV_Y];
                                }
                                if(mecost < best_mecost)
                                {
                                    best_mecost = mecost;
                                    best_bits = mebits;
                                    refi_temp = refi_cur;
                                }
                            }

                            /* save affine per list me results */
                            refi_cur = refi_temp;
                            for(vertex = 0; vertex < vertex_num; vertex++)
                            {
                                affine_mv[vertex][MV_X] = pi->affine_mv_scale[lidx][refi_cur][vertex][MV_X];
                                affine_mv[vertex][MV_Y] = pi->affine_mv_scale[lidx][refi_cur][vertex][MV_Y];
                            }

                            affine_mvp = pi->affine_mvp_scale[lidx][refi_cur];
                            t0 = (lidx == 0) ? refi_cur : REFI_INVALID;
                            t1 = (lidx == 1) ? refi_cur : REFI_INVALID;
                            SET_REFI(refi, t0, t1);

                            /* get affine mvd */
                            mvp_idx[lidx] = pi->mvp_idx_scale[lidx][refi_cur];
                            for(vertex = 0; vertex < vertex_num; vertex++)
                            {
                                affine_mvd[vertex][MV_X] = affine_mv[vertex][MV_X] - affine_mvp[mvp_idx[lidx]][vertex][MV_X];
                                affine_mvd[vertex][MV_Y] = affine_mv[vertex][MV_Y] - affine_mvp[mvp_idx[lidx]][vertex][MV_Y];
                                if(vertex)
                                {
                                    affine_mvd[vertex][MV_X] -= affine_mvd[0][MV_X];
                                    affine_mvd[vertex][MV_Y] -= affine_mvd[0][MV_Y];
                                }
                            }
                            pi->mot_bits[lidx] = best_bits;
                            pi->mvp_idx[pidx][lidx] = mvp_idx[lidx];

                            affine_mv[3][MV_X] = affine_mv[1][MV_X] + affine_mv[2][MV_X] - affine_mv[0][MV_X];
                            affine_mv[3][MV_Y] = affine_mv[1][MV_Y] + affine_mv[2][MV_Y] - affine_mv[0][MV_Y];

                            cost = cost_inter[pidx] = pinter_residue_rdo(ctx, core, x, y, log2_cuw, log2_cuh, pi->pred[PRED_NUM], pi->coef[PRED_NUM], pidx, mvp_idx, FALSE);

                            if(cost < cost_best)
                            {
                                best_affine_mode = mcore->affine_flag;
                                best_dmvr = 0;
                                core->cu_mode = MODE_INTER;
                                best_idx = pidx;
                                pi->mvp_idx[best_idx][lidx] = mvp_idx[lidx];
                                cost_inter[best_idx] = cost_best = cost;
                                pi->bi_idx[best_idx] = BI_NON;

                                SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
                                DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);

                                for(j = 0; j < N_C; j++)
                                {
                                    if(j != 0 && !ctx->sps.chroma_format_idc)
                                        continue;
                                    int size_tmp = (cuw * cuh) >> (j == 0 ? 0 : w_shift + h_shift);
                                    pi->nnz_best[pidx][j] = core->nnz[j];
                                    xeve_mcpy(pi->nnz_sub_best[pidx][j], core->nnz_sub[j], sizeof(int) * MAX_SUB_TB_NUM);
                                    xeve_mcpy(pred[0][j], pi->pred[PRED_NUM][0][j], size_tmp * sizeof(pel));
                                    xeve_mcpy(coef_t[j], pi->coef[PRED_NUM][j], size_tmp * sizeof(s16));
                                }
                                pi->ats_inter_info_mode[pidx] = mcore->ats_inter_info;
                            }
                        }

                        if(pi->slice_type == SLICE_B)
                        {
                            pidx = AFF_6_BI;
                            cost = cost_inter[pidx] = analyze_affine_bi(ctx, core, pi, x, y, log2_cuw, log2_cuh, cost_inter, AFF_6_BI, vertex_num);
                            if(cost < cost_best)
                            {
                                best_affine_mode = mcore->affine_flag;
                                best_dmvr = 0;
                                core->cu_mode = MODE_INTER;
                                best_idx = pidx;
                                cost_inter[best_idx] = cost_best = cost;
                                pi->bi_idx[best_idx] = BI_NORMAL;

                                SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
                                DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);
                            }
                        }
                    }
                }
            }
        }
    }

    /* reconstruct */
    for(j = 0; j < N_C; j++)
    {
        if(j != 0 && !ctx->sps.chroma_format_idc)
            continue;
        int size_tmp = (cuw * cuh) >> (j == 0 ? 0 : w_shift + h_shift);
        xeve_mcpy(coef[j], pi->coef[best_idx][j], sizeof(s16) * size_tmp);
        xeve_mcpy(pi->residue[j], pi->coef[best_idx][j], sizeof(s16) * size_tmp);
    }

    if(ctx->pps.cu_qp_delta_enabled_flag)
    {
        xeve_set_qp(ctx, core, core->dqp_next_best[log2_cuw - 2][log2_cuh - 2].prev_qp);
    }

    mcore->ats_inter_info = pi->ats_inter_info_mode[best_idx];

    ctx->fn_itdp(ctx, core, pi->residue, pi->nnz_sub_best[best_idx]);

    for(i = 0; i < N_C; i++)
    {
        if(i != 0 && !ctx->sps.chroma_format_idc)
            continue;
        rec[i] = pi->rec[best_idx][i];
        s_rec[i] = (i == 0 ? cuw : cuw >> w_shift);
        ctx->fn_recon(ctx, core, pi->residue[i], pi->pred[best_idx][0][i], pi->nnz_best[best_idx][i], s_rec[i], (i == 0 ? cuh : cuh >> h_shift), s_rec[i], rec[i], ctx->sps.bit_depth_luma_minus8 + 8);

        if(ctx->sps.tool_htdf == 1 && i == Y_C && pi->nnz_best[best_idx][i])
        {
            const int s_mod = pi->s_m[Y_C];
            u16 avail_cu = xeve_get_avail_intra(core->x_scu, core->y_scu, ctx->w_scu, ctx->h_scu, core->scup, log2_cuw, log2_cuh, ctx->map_scu, ctx->map_tidx);

            int constrained_intra_flag = 0 && ctx->pps.constrained_intra_pred_flag;
            xeve_htdf(rec[i], ctx->tile[core->tile_idx].qp, cuw, cuh, cuw, FALSE, pi->m[Y_C] + (y * s_mod) + x, s_mod, avail_cu
                    , core->scup, ctx->w_scu, ctx->h_scu, ctx->map_scu, constrained_intra_flag, ctx->sps.bit_depth_luma_minus8 + 8);
        }

        core->nnz[i] = pi->nnz_best[best_idx][i];
        xeve_mcpy(core->nnz_sub[i], pi->nnz_sub_best[best_idx][i], sizeof(int) * MAX_SUB_TB_NUM);
    }

    mi->pred_y_best = pi->pred[best_idx][0][0];

    /* save all cu inforamtion ********************/
    if(best_idx >= AFF_L0 && best_idx <= AFF_6_BI)
    {
        int vertex;
        int vertex_num;

        mcore->affine_flag = best_affine_mode;
        vertex_num = mcore->affine_flag + 1;
        for(lidx = 0; lidx < REFP_NUM; lidx++)
        {
            for(vertex = 0; vertex < vertex_num; vertex++)
            {
                mi->affine_mv[lidx][vertex][MV_X] = pi->affine_mv[best_idx][lidx][vertex][MV_X];
                mi->affine_mv[lidx][vertex][MV_Y] = pi->affine_mv[best_idx][lidx][vertex][MV_Y];
                mi->affine_mvd[lidx][vertex][MV_X] = pi->affine_mvd[best_idx][lidx][vertex][MV_X];
                mi->affine_mvd[lidx][vertex][MV_Y] = pi->affine_mvd[best_idx][lidx][vertex][MV_Y];
            }
        }
    }
    else
    {
        mcore->affine_flag = 0;
    }

    mcore->dmvr_flag = best_dmvr;

    for(lidx = 0; lidx < REFP_NUM; lidx++)
    {
        mi->refi[lidx] = pi->refi[best_idx][lidx];
        mi->mvp_idx[lidx] = pi->mvp_idx[best_idx][lidx];
        if(mcore->dmvr_flag)
        {
            assert(core->cu_mode == MODE_SKIP || core->cu_mode == MODE_DIR);
            u16 idx = 0, i, j;
            for(j = 0; j < core->cuh >> MIN_CU_LOG2; j++)
            {
                for(i = 0; i < core->cuw >> MIN_CU_LOG2; i++)
                {
                    mi->dmvr_mv[idx + i][lidx][MV_X] = pi->dmvr_mv[best_idx][idx + i][lidx][MV_X];
                    mi->dmvr_mv[idx + i][lidx][MV_Y] = pi->dmvr_mv[best_idx][idx + i][lidx][MV_Y];
                }
                idx += core->cuw >> MIN_CU_LOG2;
            }
        }

        mi->mv[lidx][MV_X] = pi->mv[best_idx][lidx][MV_X];
        mi->mv[lidx][MV_Y] = pi->mv[best_idx][lidx][MV_Y];

        mi->mvd[lidx][MV_X] = pi->mvd[best_idx][lidx][MV_X];
        mi->mvd[lidx][MV_Y] = pi->mvd[best_idx][lidx][MV_Y];
    }

    mi->mmvd_idx = pi->mmvd_idx[best_idx];
    mi->mvr_idx = pi->mvr_idx[best_idx];
    mi->bi_idx = pi->bi_idx[best_idx];

    if(!mcore->bef_data[log2_cuw - 2][log2_cuh - 2][core->cup][core->bef_data_idx].visit)
    {
        mcore->bef_data[log2_cuw - 2][log2_cuh - 2][core->cup][core->bef_data_idx].mvr_idx = mi->mvr_idx;
        mcore->bef_data[log2_cuw - 2][log2_cuh - 2][core->cup][core->bef_data_idx].bi_idx = mi->bi_idx;
    }

    if(!mcore->bef_data[log2_cuw - 2][log2_cuh - 2][core->cup][core->bef_data_idx].visit)
    {
        mcore->bef_data[log2_cuw - 2][log2_cuh - 2][core->cup][core->bef_data_idx].affine_flag = best_affine_mode;
    }

#if TRACE_ADDITIONAL_FLAGS
    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("Inter analyze for block [(");
    XEVE_TRACE_INT(x);
    XEVE_TRACE_STR(", ");
    XEVE_TRACE_INT(y);
    XEVE_TRACE_STR("), ");
    XEVE_TRACE_INT(1 << log2_cuw);
    XEVE_TRACE_STR("x");
    XEVE_TRACE_INT(1 << log2_cuh);
    XEVE_TRACE_STR("]Inter costs: ");
    for(int i = 0; i < PRED_NUM; ++i)
    {
        XEVE_TRACE_DOUBLE(cost_inter[i]);
    }
    XEVE_TRACE_STR(". Best idx = ");
    XEVE_TRACE_INT(best_idx);
    XEVE_TRACE_STR("\n");
#endif
    return cost_inter[best_idx];
}

void pinter_mc_main(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int w, int h, s8 refi[REFP_NUM], s16(*mv)[MV_D], XEVE_REFP(*refp)[REFP_NUM]
                    , pel pred[REFP_NUM][N_C][MAX_CU_DIM], int poc_c, int apply_dmvr, s16 dmvr_mv[MAX_CU_CNT_IN_LCU][REFP_NUM][MV_D])
{
    XEVEM_CORE *mcore = (XEVEM_CORE *)core;
    XEVE_PINTER *pi = &ctx->pinter[core->thread_cnt];

    xevem_mc(x, y, ctx->w, ctx->h, w, h, refi, mv, refp, pred, poc_c, pi->dmvr_template, pi->dmvr_ref_pred_interpolated
           , pi->dmvr_half_pred_interpolated, apply_dmvr && ctx->sps.tool_dmvr, pi->dmvr_padding_buf, &(mcore->dmvr_flag)
           , dmvr_mv, ctx->sps.tool_admvp, ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc);
}

static void pinter_save_best_info_main(XEVE_CTX *ctx, XEVE_CORE *core, int pidx)
{
    XEVEM_CORE *mcore = (XEVEM_CORE *)core;
    XEVE_PINTER *pi = &ctx->pinter[core->thread_cnt];

    pi->ats_inter_info_mode[pidx] = mcore->ats_inter_info;
}

static void pinter_load_best_info_main(XEVE_CTX *ctx, XEVE_CORE *core, int best_idx)
{
    XEVEM_CORE *mcore = (XEVEM_CORE *)core;
    XEVE_PINTER *pi = &ctx->pinter[core->thread_cnt];

    mcore->ats_inter_info = pi->ats_inter_info_mode[best_idx];
}

/* For Main profile */
static int pinter_set_complexity(XEVE_CTX *ctx, int complexity)
{
    XEVE_PINTER *pi;

    for(int i = 0; i < ctx->param.threads; i++)
    {
        pi = &ctx->pinter[i];
        pi->max_search_range = ctx->param.bframes == 0 ? SEARCH_RANGE_IPEL_LD : ctx->param.me_range;
        pi->search_range_ipel[MV_X] = pi->max_search_range;
        pi->search_range_ipel[MV_Y] = pi->max_search_range;
        pi->search_range_spel[MV_X] = ctx->param.me_sub_range;
        pi->search_range_spel[MV_Y] = ctx->param.me_sub_range;
        pi->search_pattern_hpel = tbl_search_pattern_hpel_partial;
        pi->search_pattern_hpel_cnt = ctx->param.me_sub_pos;
        pi->search_pattern_qpel = tbl_search_pattern_qpel_8point;
        pi->search_pattern_qpel_cnt = ctx->param.me_sub_pos;
        if(ctx->param.tool_admvp == 0)
        {
            ctx->fn_pinter_analyze_cu = xeve_pinter_analyze_cu;
        }
        else
        {
            ctx->fn_pinter_analyze_cu = pinter_analyze_cu;
        }
        pi->me_level = ctx->param.me_sub;
        pi->fn_me = pinter_me_epzs; /* [To be done] for baseline, pinter_me_epzs should be used */
        pi->fn_affine_me = pinter_affine_me_gradient;
        pi->complexity = complexity;
        pi->sps_amvr_flag = ctx->param.tool_amvr;
        pi->fn_get_first_refi = pinter_get_first_refi_main; /* need to check */
        pi->fn_save_best_info = pinter_save_best_info_main;
        pi->fn_load_best_info = pinter_load_best_info_main;
        pi->fn_mc = pinter_mc_main;
        pi->skip_merge_cand_num = ctx->param.merge_num;
        pi->me_complexity = ctx->param.me_algo;
        pi->me_opt = &tbl_inter_pred_comp[ctx->param.me_fast];
    }
    return XEVE_OK;
}

int xevem_pinter_create(XEVE_CTX *ctx, int complexity)
{
    /* set function addresses */
    ctx->fn_pinter_init_mt = pinter_init_mt;
    ctx->fn_pinter_init_lcu = xeve_pinter_init_lcu;
    ctx->fn_pinter_set_complexity = pinter_set_complexity;

    XEVE_PINTER * pi;
    for(int i = 0; i < ctx->param.threads; i++)
    {
        pi = &ctx->pinter[i];
        /* set maximum/minimum value of search range */
        pi->min_clip[MV_X] = -MAX_CU_SIZE + 1;
        pi->min_clip[MV_Y] = -MAX_CU_SIZE + 1;
        pi->max_clip[MV_X] = ctx->param.w - 1;
        pi->max_clip[MV_Y] = ctx->param.h - 1;

        if (ctx->param.tool_admvp == 0)
        {
            pi->mc_l_coeff = xeve_tbl_mc_l_coeff;
            pi->mc_c_coeff = xeve_tbl_mc_c_coeff;
        }
        else
        {
            pi->mc_l_coeff = xevem_tbl_mc_l_coeff;
            pi->mc_c_coeff = xevem_tbl_mc_c_coeff;
        }
    }

    return ctx->fn_pinter_set_complexity(ctx, complexity);
}
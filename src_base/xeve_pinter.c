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

// clang-format off
const XEVE_PRED_INTER_COMP tbl_inter_pred_comp[2] =
{
    { 12,                 16,  8, 8, 1, 1, 1, 1, 1, 2, 1, 1, 2 },
    { RASTER_SEARCH_STEP, 128, 0, 0, 0, 0, 2, 0, 2, 1, 0, 2, 4}
};

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

// clang-format on

__inline static u32 get_exp_golomb_bits(u32 abs_mvd)
{
    int bits = 0;
    int len_i, len_c, nn;

    /* abs(mvd) */
    nn = (abs_mvd + 1) >> 12;
    for (len_i = 11; len_i < 16 && nn != 0; len_i++)
    {
        nn >>= 1;
    }
    len_c = (len_i << 1) + 1;

    bits += len_c;

    /* sign */
    bits++;

    return bits;
}

__inline static int get_mv_bits(int mvd_x, int mvd_y, int num_refp, int refi)
{
    int bits = 0;

    if (mvd_x > 2048)
    {
        bits = get_exp_golomb_bits(mvd_x);
    }
    else if (mvd_x <= -2048)
    {
        bits = get_exp_golomb_bits(-mvd_x);
    }
    else
    {
        bits = xeve_tbl_mv_bits[mvd_x];
    }

    if (mvd_y > 2048)
    {
        bits += get_exp_golomb_bits(mvd_y);
    }
    else if (mvd_y <= -2048)
    {
        bits += get_exp_golomb_bits(-mvd_y);
    }
    else
    {
        bits += xeve_tbl_mv_bits[mvd_y];
    }

    bits += xeve_tbl_refi_bits[num_refp][refi];
    return bits;
}

static void get_range_ipel(XEVE_PINTER * pi, s16 mvc[MV_D], s16 range[MV_RANGE_DIM][MV_D], int bi, int ri, int lidx)
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

static u32 me_raster(XEVE_PINTER * pi, int x, int y, int log2_cuw, int log2_cuh, s8 refi, int lidx, s16 range[MV_RANGE_DIM][MV_D], s16 gmvp[MV_D], s16 mv[MV_D], int bit_depth_luma)
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

    org = pi->o[Y_C] + y * pi->s_o[Y_C] + x;
    ref_pic = pi->refp[refi][lidx].pic;
    best_mv_bits = 0;
    cost_best = XEVE_UINT32_MAX;

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

            /* get MVD bits */
            mv_bits = get_mv_bits((mv_x << 2) - gmvp[MV_X], (mv_y << 2) - gmvp[MV_Y], pi->num_refp, refi);

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

                mv_bits = get_mv_bits((mv_x << 2) - gmvp[MV_X], (mv_y << 2) - gmvp[MV_Y], pi->num_refp, refi);

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

        /* Halve the step size */
        search_step >>= 1;
    }

    if(best_mv_bits > 0)
    {
        pi->mot_bits[lidx] = best_mv_bits;
    }

    return cost_best;
}

static u32 me_ipel_refinement(XEVE_PINTER *pi, int x, int y, int log2_cuw, int log2_cuh, s8 refi, int lidx, s16 range[MV_RANGE_DIM][MV_D], s16 gmvp[MV_D], s16 mvi[MV_D], s16 mv[MV_D], int bi, int *beststep, int faststep, int bit_depth_luma)
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

    imv_x = mv_best_x;
    imv_y = mv_best_y;

    int test_pos[9][2] = {{ 0, 0}, { -1, -1},{ -1, 0},{ -1, 1},{ 0, -1},{ 0, 1},{ 1, -1},{ 1, 0},{ 1, 1}};

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
            mv_bits = get_mv_bits((mv_x << 2) - gmvp[MV_X], (mv_y << 2) - gmvp[MV_Y], pi->num_refp, refi);

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

static u32 me_ipel_diamond(XEVE_PINTER *pi, int x, int y, int log2_cuw, int log2_cuh, s8 refi, int lidx, s16 range[MV_RANGE_DIM][MV_D], s16 gmvp[MV_D], s16 mvi[MV_D], s16 mv[MV_D], int bi, int *beststep, int faststep, int bit_depth_luma)
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
    int            mvsize = 1;
    int not_found_best = 0;

    org = pi->o[Y_C] + y * pi->s_o[Y_C] + x;
    ref_pic = pi->refp[refi][lidx].pic;
    mv_best_x = (mvi[MV_X] >> 2);
    mv_best_y = (mvi[MV_Y] >> 2);
    best_mv_bits = 0;
    step = 0;
    mv_best_x = XEVE_CLIP3(pi->min_clip[MV_X], pi->max_clip[MV_X], mv_best_x);
    mv_best_y = XEVE_CLIP3(pi->min_clip[MV_Y], pi->max_clip[MV_Y], mv_best_y);

    imv_x = mv_best_x;
    imv_y = mv_best_y;

    while(1)
    {
        not_found_best++;

        if(step <= 2)
        {
            min_cmv_x = (mv_best_x <= range[MV_RANGE_MIN][MV_X]) ? mv_best_x : mv_best_x - (bi == BI_NORMAL ? BI_STEP : 2);
            min_cmv_y = (mv_best_y <= range[MV_RANGE_MIN][MV_Y]) ? mv_best_y : mv_best_y - (bi == BI_NORMAL ? BI_STEP : 2);
            max_cmv_x = (mv_best_x >= range[MV_RANGE_MAX][MV_X]) ? mv_best_x : mv_best_x + (bi == BI_NORMAL ? BI_STEP : 2);
            max_cmv_y = (mv_best_y >= range[MV_RANGE_MAX][MV_Y]) ? mv_best_y : mv_best_y + (bi == BI_NORMAL ? BI_STEP : 2);
            mvsize = 1;

            for(i = min_cmv_y; i <= max_cmv_y; i += mvsize)
            {
                for(j = min_cmv_x; j <= max_cmv_x; j += mvsize)
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
                        mv_bits = get_mv_bits((mv_x << 2) - gmvp[MV_X], (mv_y << 2) - gmvp[MV_Y], pi->num_refp, refi);

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
            }

            mvc[MV_X] = mv_best_x;
            mvc[MV_Y] = mv_best_y;

            get_range_ipel(pi, mvc, range, (bi != BI_NORMAL) ? 0 : 1, refi, lidx);
            step += 2;
        }
        else
        {
            int meidx = step > 8 ? 2 : 1;
            int multi;

            multi = step;

            for(i = 0; i < 16; i++)
            {
                if(meidx == 1 && i > 8)
                {
                    continue;
                }
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
                    mv_bits = get_mv_bits((mv_x << 2) - gmvp[MV_X], (mv_y << 2) - gmvp[MV_Y], pi->num_refp, refi);

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

        if(step > pi->max_search_range)
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

static u32 me_spel_pattern(XEVE_PINTER *pi, int x, int y, int log2_cuw, int log2_cuh, s8 refi, int lidx, s16 gmvp[MV_D], s16 mvi[MV_D], s16 mv[MV_D], int bi, int bit_depth_luma)
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
        mv_bits = get_mv_bits(mv_x - gmvp[MV_X], mv_y - gmvp[MV_Y], pi->num_refp, refi);

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
    if(pi->me_level > ME_LEV_HPEL)
    {
        /* make MV to be absolute coordinate */
        cx = mv[MV_X] + (x << 2);
        cy = mv[MV_Y] + (y << 2);

        for(i = 0; i < pi->search_pattern_qpel_cnt; i++)
        {
            mv_x = cx + pi->search_pattern_qpel[i][0];
            mv_y = cy + pi->search_pattern_qpel[i][1];

            /* get MVD bits */
            mv_bits = get_mv_bits(mv_x - gmvp[MV_X], mv_y - gmvp[MV_Y], pi->num_refp, refi);

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

    cost = me_ipel_diamond(pi, x, y, log2_cuw, log2_cuh, ri, lidx, range, gmvp, mvi, mvt, bi, &tmpstep, MAX_FIRST_SEARCH_STEP, bit_depth_luma);
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

    if(bi == BI_NON && beststep > RASTER_SEARCH_THD  && pi->me_complexity > 1)
    {
        cost = me_raster(pi, x, y, log2_cuw, log2_cuh, ri, lidx, range, gmvp, mvt, bit_depth_luma);

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
        cost = me_ipel_diamond(pi, x, y, log2_cuw, log2_cuh, ri, lidx, range, gmvp, mvi, mvt, bi, &tmpstep, MAX_REFINE_SEARCH_STEP, bit_depth_luma);
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

    if(pi->me_level > ME_LEV_IPEL)
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

static void copy_tu_from_cu(s16 tu_resi[N_C][MAX_CU_DIM], s16 cu_resi[N_C][MAX_CU_DIM], int log2_cuw, int log2_cuh, int chroma_format_idc)
{
    int j;
    int cuw = 1 << log2_cuw;
    int log2_tuw, log2_tuh;
    int tuw, tuh;
    int w_shift = XEVE_GET_CHROMA_W_SHIFT(chroma_format_idc);
    int h_shift = XEVE_GET_CHROMA_H_SHIFT(chroma_format_idc);

    log2_tuw = log2_cuw;
    log2_tuh = log2_cuh;
    tuw = 1 << log2_tuw;
    tuh = 1 << log2_tuh;

    //Y
    for(j = 0; j < tuh; j++)
    {
        xeve_mcpy(tu_resi[Y_C] + j * tuw, cu_resi[Y_C] + j * cuw, sizeof(s16)*tuw);
    }
    if(chroma_format_idc)
    {
    //UV
        tuw >>= w_shift;
        tuh >>= h_shift;
        cuw >>= w_shift;

        for(j = 0; j < tuh; j++)
        {
            xeve_mcpy(tu_resi[U_C] + j * tuw, cu_resi[U_C] + j * cuw, sizeof(s16)*tuw);
            xeve_mcpy(tu_resi[V_C] + j * tuw, cu_resi[V_C] + j * cuw, sizeof(s16)*tuw);
        }
    }
}

static double pinter_residue_rdo(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh, pel pred[2][N_C][MAX_CU_DIM], s16 coef[N_C][MAX_CU_DIM], int pidx, u8 *mvp_idx)
{
    XEVE_PINTER *pi = &ctx->pinter[core->thread_cnt];
    s16     coef_t[N_C][MAX_CU_DIM];
    int   * nnz, tnnz, w[N_C], h[N_C], log2_w[N_C], log2_h[N_C];
    int     cuw;
    int     cuh;
    pel  (* rec)[MAX_CU_DIM];
    s64     dist[2][N_C];
    double  cost, cost_best = MAX_COST;
    int     cbf_idx[N_C], nnz_store[N_C];
    int     nnz_sub_store[N_C][MAX_SUB_TB_NUM] = { {0}, };
    int     bit_cnt;
    int     i, idx_y, idx_u, idx_v;
    pel   * org[N_C];
    double  cost_comp_best = MAX_COST;
    int     idx_best[N_C] = { 0, };
    int     j;
    u8      is_from_mv_field = 0;
    s64     dist_no_resi[N_C];
    int     nnz_best[N_C] = { -1, -1, -1 };
    u8      num_rdo_tried = 0;
    s64     dist_idx = -1;
    int     w_shift = ctx->param.cs_w_shift;
    int     h_shift = ctx->param.cs_h_shift;

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
    org[U_C] = pi->o[U_C] + ((y >> h_shift) * pi->s_o[U_C]) + (x >> w_shift);
    org[V_C] = pi->o[V_C] + ((y >> h_shift) * pi->s_o[V_C]) + (x >> w_shift);

    /* prediction */
    pi->fn_mc(ctx, core, x, y, w[0], h[0], pi->refi[pidx], pi->mv[pidx], pi->refp, pred, 0, 0, NULL);

    int bit_depth_tbl[3] = {ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8 };

    /* get residual */
    xeve_diff_pred(x, y, log2_cuw, log2_cuh, pi->pic_o, pred[0], pi->resi, ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc);

    for (i = 0; i < N_C; i++)
    {
        if (!ctx->sps.chroma_format_idc && i != 0)
        {
            dist[0][i] = 0;
        }
        else
        {
            dist[0][i] = xeve_ssd_16b(log2_w[i], log2_h[i], pred[0][i], org[i], w[i], pi->s_o[i], bit_depth_tbl[i]);
        }
        dist_no_resi[i] = dist[0][i];
    }

    copy_tu_from_cu(coef, pi->resi, log2_cuw, log2_cuh,ctx->sps.chroma_format_idc);

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
            int size = (cuw * cuh) >> (i == 0 ? 0 : (w_shift+h_shift));
            xeve_mcpy(coef_t[i], coef[i], sizeof(s16) * size);

            cbf_idx[i] = 0;
            nnz_store[i] = nnz[i];
            xeve_mcpy(nnz_sub_store[i], core->nnz_sub[i], sizeof(int) * MAX_SUB_TB_NUM);
        }

        ctx->fn_itdp(ctx, core, coef_t, core->nnz_sub);

        if(ctx->param.rdo_dbk_switch)
        {
            calc_delta_dist_filter_boundary(ctx, PIC_MODE(ctx), PIC_ORIG(ctx), cuw, cuh, pred[0], cuw, x, y, core->avail_lr, 0, 0, pi->refi[pidx]
                                          , pi->mv[pidx], is_from_mv_field, core);
        }

        for(i = 0; i < N_C; i++)
        {
            if(nnz[i])
            {
                ctx->fn_recon(ctx, core, coef_t[i], pred[0][i], nnz[i], w[i], h[i], w[i], rec[i], ctx->sps.bit_depth_luma_minus8 + 8);
                if (!ctx->sps.chroma_format_idc && i != 0)
                {
                    dist[1][i] = 0;
                }
                else
                {
                    dist[1][i] = xeve_ssd_16b(log2_w[i], log2_h[i], rec[i], org[i], w[i], pi->s_o[i], bit_depth_tbl[i]);
                }
            }
            else
            {
                dist[1][i] = dist_no_resi[i];
            }
            if(ctx->param.rdo_dbk_switch)
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
                if (i != 0 && !ctx->sps.chroma_format_idc)
                {
                    dist[1][i] = 0;
                }
            }
        }

        if(pidx != PRED_DIR)
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
            xeve_rdo_bit_cnt_cu_inter(ctx, core, ctx->sh->slice_type, core->scup, pi->refi[pidx], pi->mvd[pidx], coef, pidx, mvp_idx, 0, 0, NULL);

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
                core->cost_best = cost < core->cost_best ? cost : core->cost_best;
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
        xeve_rdo_bit_cnt_cu_inter(ctx, core, ctx->sh->slice_type, core->scup, pi->refi[pidx], pi->mvd[pidx], coef, pidx, mvp_idx, 0, 0, NULL);

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
            core->cost_best = cost < core->cost_best ? cost : core->cost_best;
        }

        SBAC_LOAD(core->s_temp_prev_comp_best, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);

        /* cbf test for each component */
        for(i = 0; i < N_C; i++)
        {
            if(nnz_store[i] > 0)
            {
                cost_comp_best = MAX_COST;
                SBAC_LOAD(core->s_temp_prev_comp_run, core->s_temp_prev_comp_best);
                for(j = 0; j < 2; j++)
                {
                    cost = dist[j][i] * (i == 0 ? 1 : core->dist_chroma_weight[i - 1]);
                    nnz[i] = j ? nnz_store[i] : 0;
                    if(j)
                    {
                        xeve_mcpy(core->nnz_sub[i], nnz_sub_store[i], sizeof(int) * MAX_SUB_TB_NUM);
                    }
                    else
                    {
                        xeve_mset(core->nnz_sub[i], 0, sizeof(int) * MAX_SUB_TB_NUM);
                    }

                    SBAC_LOAD(core->s_temp_run, core->s_temp_prev_comp_run);
                    xeve_sbac_bit_reset(&core->s_temp_run);
                    xeve_rdo_bit_cnt_cu_inter_comp(core, coef, i, pidx, ctx
                                                   , core->tree_cons);

                    bit_cnt = xeve_get_bit_number(&core->s_temp_run);
                    cost += RATE_TO_COST_LAMBDA(core->lambda[i], bit_cnt);
                    if(cost < cost_comp_best)
                    {
                        cost_comp_best = cost;
                        idx_best[i] = j;
                        SBAC_STORE(core->s_temp_prev_comp_best, core->s_temp_run);
                    }
                }
            }
            else
            {
                idx_best[i] = 0;
            }
        }

        if(idx_best[Y_C] != 0 || idx_best[U_C] != 0 || idx_best[V_C] != 0)
        {
            idx_y = idx_best[Y_C];
            idx_u = idx_best[U_C];
            idx_v = idx_best[V_C];
            nnz[Y_C] = idx_y ? nnz_store[Y_C] : 0;
            nnz[U_C] = idx_u ? nnz_store[U_C] : 0;
            nnz[V_C] = idx_v ? nnz_store[V_C] : 0;
            for(i = 0; i < N_C; i++)
            {
                if(idx_best[i])
                {
                    xeve_mcpy(core->nnz_sub[i], nnz_sub_store[i], sizeof(int) * MAX_SUB_TB_NUM);
                }
                else
                {
                    xeve_mset(core->nnz_sub[i], 0, sizeof(int) * MAX_SUB_TB_NUM);
                }
            }
        }

        if(nnz[Y_C] != nnz_store[Y_C] || nnz[U_C] != nnz_store[U_C] || nnz[V_C] != nnz_store[V_C])
        {
            cost = (double)dist[idx_y][Y_C] + (((double)dist[idx_u][U_C] * core->dist_chroma_weight[0]) + ((double)dist[idx_v][V_C] * core->dist_chroma_weight[1]));

            SBAC_LOAD(core->s_temp_run, core->s_curr_best[log2_cuw - 2][log2_cuh - 2]);
            DQP_LOAD(core->dqp_temp_run, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2]);

            xeve_sbac_bit_reset(&core->s_temp_run);
            xeve_rdo_bit_cnt_cu_inter(ctx, core, ctx->sh->slice_type, core->scup, pi->refi[pidx], pi->mvd[pidx], coef, pidx, mvp_idx, 0, 0, NULL);

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
                core->cost_best = cost < core->cost_best ? cost : core->cost_best;
            }
        }

        for(i = 0; i < N_C; i++)
        {
            nnz[i] = (cbf_idx[i] ? nnz_store[i] : 0);
            if(cbf_idx[i])
            {
                xeve_mcpy(core->nnz_sub[i], nnz_sub_store[i], sizeof(int) * MAX_SUB_TB_NUM);
            }
            else
            {
                xeve_mset(core->nnz_sub[i], 0, sizeof(int) * MAX_SUB_TB_NUM);
            }
            if(nnz[i] == 0 && nnz_store[i] != 0)
            {
                xeve_mset(core->nnz_sub[i], 0, sizeof(int) * MAX_SUB_TB_NUM);
                xeve_mset(coef[i], 0, sizeof(s16) * ((cuw * cuh) >> (i == 0 ? 0 : (w_shift + h_shift))));
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
        xeve_rdo_bit_cnt_cu_inter(ctx, core, ctx->sh->slice_type, core->scup, pi->refi[pidx], pi->mvd[pidx], coef, pidx, mvp_idx, 0, 0, NULL);

        bit_cnt = xeve_get_bit_number(&core->s_temp_run);
        cost_best += RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt);
        SBAC_STORE(core->s_temp_best, core->s_temp_run);
        DQP_STORE(core->dqp_temp_best, core->dqp_temp_run);
        nnz_best[Y_C] = nnz_best[U_C] = nnz_best[V_C] = 0;
        core->cost_best = cost_best < core->cost_best ? cost_best : core->cost_best;
    }

    return cost_best;
}

static double xeve_analyze_skip(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh)
{
    XEVE_PINTER * pi = &ctx->pinter[core->thread_cnt];
    pel         * y_org, *u_org, *v_org;
    s16           mvp[REFP_NUM][MV_D];
    s8            refi[REFP_NUM];
    double        cost, cost_best = MAX_COST;
    int           cuw, cuh, idx0, idx1, cnt, bit_cnt;
    s64           cy, cu, cv;
    s64           temp_ssd = 0;
    int           w_shift = ctx->param.cs_w_shift;
    int           h_shift = ctx->param.cs_h_shift;

    pi->best_ssd = (s64)1 << (log2_cuw + log2_cuh + 16);

    if(ctx->pps.cu_qp_delta_enabled_flag)
    {
        if(core->cu_qp_delta_code_mode != 2)
        {
            xeve_set_qp(ctx, core, core->dqp_curr_best[log2_cuw - 2][log2_cuh - 2].prev_qp);
        }
    }
    cu = cv = cy = 0;
    cuw = (1 << log2_cuw);
    cuh = (1 << log2_cuh);
    y_org = pi->o[Y_C] + x + y * pi->s_o[Y_C];
    u_org = pi->o[U_C] + (x >> w_shift) + ((y >> h_shift) * pi->s_o[U_C]);
    v_org = pi->o[V_C] + (x >> w_shift) + ((y >> h_shift) * pi->s_o[V_C]);

    xeve_get_motion(core->scup, REFP_0, ctx->map_refi, ctx->map_mv, pi->refp, cuw, cuh, ctx->w_scu, core->avail_cu, pi->refi_pred[REFP_0], pi->mvp[REFP_0]);
    if(ctx->slice_type == SLICE_B)
    {
        xeve_get_motion(core->scup, REFP_1, ctx->map_refi, ctx->map_mv, pi->refp, cuw, cuh, ctx->w_scu, core->avail_cu, pi->refi_pred[REFP_1], pi->mvp[REFP_1]);
    }

    pi->mvp_idx[PRED_SKIP][REFP_0] = 0;
    pi->mvp_idx[PRED_SKIP][REFP_1] = 0;

    for (idx0 = 0; idx0 < pi->skip_merge_cand_num; idx0++)
    {
        if (idx0)
        {
            /* encoder side pruning */
            int found_same_mvp = 0;
            for(int tmp_idx = idx0 - 1; tmp_idx >= 0; tmp_idx--)
            {
                if (pi->mvp[REFP_0][tmp_idx][MV_X] == pi->mvp[REFP_0][idx0][MV_X] &&
                    pi->mvp[REFP_0][tmp_idx][MV_Y] == pi->mvp[REFP_0][idx0][MV_Y])
                {
                    found_same_mvp = 1;
                    break;
                }
            }
            if(found_same_mvp)
            {
                continue;
            }
        }
        cnt = (ctx->slice_type == SLICE_B ? pi->skip_merge_cand_num : 1);
        for(idx1 = 0; idx1 < cnt; idx1++)
        {
            if (idx1)
            {
                /* encoder side pruning */
                int found_same_mvp = 0;
                for (int tmp_idx = idx1 - 1; tmp_idx >= 0; tmp_idx--)
                {
                    if(pi->mvp[REFP_1][tmp_idx][MV_X] == pi->mvp[REFP_1][idx1][MV_X] &&
                       pi->mvp[REFP_1][tmp_idx][MV_Y] == pi->mvp[REFP_1][idx1][MV_Y])
                    {
                        found_same_mvp = 1;
                        break;
                    }
                }
                if (found_same_mvp)
                {
                    continue;
                }
            }
            mvp[REFP_0][MV_X] = pi->mvp[REFP_0][idx0][MV_X];
            mvp[REFP_0][MV_Y] = pi->mvp[REFP_0][idx0][MV_Y];
            mvp[REFP_1][MV_X] = pi->mvp[REFP_1][idx1][MV_X];
            mvp[REFP_1][MV_Y] = pi->mvp[REFP_1][idx1][MV_Y];

            SET_REFI(refi, pi->refi_pred[REFP_0][idx0], ctx->sh->slice_type == SLICE_B ? pi->refi_pred[REFP_1][idx1] : REFI_INVALID);
            if(!REFI_IS_VALID(refi[REFP_0]) && !REFI_IS_VALID(refi[REFP_1]))
            {
                continue;
            }

            pi->fn_mc(ctx, core, x, y, cuw, cuh, refi, mvp, pi->refp, pi->pred[PRED_NUM], 0, 0, NULL);

            cy = xeve_ssd_16b(log2_cuw, log2_cuh, pi->pred[PRED_NUM][0][Y_C], y_org, cuw, pi->s_o[Y_C], ctx->sps.bit_depth_luma_minus8 + 8);
            if(ctx->sps.chroma_format_idc)
            {
                cu = xeve_ssd_16b(log2_cuw - w_shift, log2_cuh - h_shift, pi->pred[PRED_NUM][0][U_C], u_org, cuw >> w_shift, pi->s_o[U_C], ctx->sps.bit_depth_chroma_minus8 + 8);
                cv = xeve_ssd_16b(log2_cuw - w_shift, log2_cuh - h_shift, pi->pred[PRED_NUM][0][V_C], v_org, cuw >> w_shift, pi->s_o[V_C], ctx->sps.bit_depth_chroma_minus8 + 8);
            }

            temp_ssd = cy + cu + cv;

            if(ctx->param.rdo_dbk_switch)
            {
                calc_delta_dist_filter_boundary(ctx, PIC_MODE(ctx), PIC_ORIG(ctx), cuw, cuh, pi->pred[PRED_NUM][0], cuw, x, y, core->avail_lr, 0, 0, refi, mvp, 0,  core);
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

            xeve_rdo_bit_cnt_cu_skip(ctx, core, ctx->sh->slice_type, core->scup, idx0, idx1, 0, 0);

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
                pi->mvd[PRED_SKIP][REFP_0][MV_X] = 0;
                pi->mvd[PRED_SKIP][REFP_0][MV_Y] = 0;
                pi->mvd[PRED_SKIP][REFP_1][MV_X] = 0;
                pi->mvd[PRED_SKIP][REFP_1][MV_Y] = 0;
                pi->refi[PRED_SKIP][REFP_0] = refi[REFP_0];
                pi->refi[PRED_SKIP][REFP_1] = refi[REFP_1];

                core->cost_best = cost < core->cost_best ? cost : core->cost_best;
                pi->best_ssd = temp_ssd;

                for(j = 0; j < N_C; j++)
                {
                    if(j != 0 && !ctx->sps.chroma_format_idc)
                        continue;
                    int size_tmp = (cuw * cuh) >> (j == 0 ? 0 : (w_shift + h_shift));
                    xeve_mcpy(pi->pred[PRED_SKIP][0][j], pi->pred[PRED_NUM][0][j], size_tmp * sizeof(pel));
                }

                SBAC_STORE(core->s_temp_best, core->s_temp_run);
                DQP_STORE(core->dqp_temp_best, core->dqp_temp_run);
            }
        }
    }

    return cost_best;
}

static double analyze_t_direct(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh)
{
    XEVE_PINTER *pi = &ctx->pinter[core->thread_cnt];
    double           cost;
    int              pidx;
    s8               refidx = 0;

    pidx = PRED_DIR;
    xeve_get_mv_dir(pi->refp[0], ctx->poc.poc_val, core->scup + ((1 << (log2_cuw - MIN_CU_LOG2)) - 1) + ((1 << (log2_cuh - MIN_CU_LOG2)) - 1) * ctx->w_scu
                    , core->scup, ctx->w_scu, ctx->h_scu, pi->mv[pidx], 0);

    pi->mvd[pidx][REFP_0][MV_X] = 0;
    pi->mvd[pidx][REFP_0][MV_Y] = 0;
    pi->mvd[pidx][REFP_1][MV_X] = 0;
    pi->mvd[pidx][REFP_1][MV_Y] = 0;

    SET_REFI(pi->refi[pidx], 0, 0);

    cost = pinter_residue_rdo(ctx, core, x, y, log2_cuw, log2_cuh, pi->pred[pidx], pi->coef[pidx], pidx, pi->mvp_idx[pidx]);

    xeve_mcpy(pi->nnz_best[pidx], core->nnz, sizeof(int) * N_C);

    return cost;
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
    pel       (*pred)[N_C][MAX_CU_DIM];
    int         cuw, cuh, t0;
    double      cost;
    int         lidx_ref, lidx_cnd, mvp_idx = 0;
    int         pidx, pidx_ref, pidx_cnd, i;

    cuw = (1 << log2_cuw);
    cuh = (1 << log2_cuh);

    pidx = PRED_BI;

    if(cost_inter[PRED_L0] <= cost_inter[PRED_L1])
    {
        lidx_ref = REFP_0;
        lidx_cnd = REFP_1;
        pidx_ref = PRED_L0;
        pidx_cnd = PRED_L1;
    }
    else
    {
        lidx_ref = REFP_1;
        lidx_cnd = REFP_0;
        pidx_ref = PRED_L1;
        pidx_cnd = PRED_L0;
    }

    pi->mvp_idx[pidx][REFP_0] = pi->mvp_idx[PRED_L0][REFP_0];
    pi->mvp_idx[pidx][REFP_1] = pi->mvp_idx[PRED_L1][REFP_1];
    pi->refi[pidx][REFP_0] = pi->refi[PRED_L0][REFP_0];
    pi->refi[pidx][REFP_1] = pi->refi[PRED_L1][REFP_1];
    pi->mv[pidx][lidx_ref][MV_X] = pi->mv[pidx_ref][lidx_ref][MV_X];
    pi->mv[pidx][lidx_ref][MV_Y] = pi->mv[pidx_ref][lidx_ref][MV_Y];
    pi->mv[pidx][lidx_cnd][MV_X] = pi->mv[pidx_cnd][lidx_cnd][MV_X];
    pi->mv[pidx][lidx_cnd][MV_Y] = pi->mv[pidx_cnd][lidx_cnd][MV_Y];

    /* get MVP lidx_cnd */
    org = pi->o[Y_C] + x + y * pi->s_o[Y_C];
    pred = pi->pred[pidx];

    t0 = (lidx_ref == REFP_0) ? pi->refi[pidx][lidx_ref] : REFI_INVALID;
    t1 = (lidx_ref == REFP_1) ? pi->refi[pidx][lidx_ref] : REFI_INVALID;
    SET_REFI(refi, t0, t1);

    for(i = 0; i < BI_ITER; i++)
    {
        /* predict reference */
        pi->fn_mc(ctx, core, x, y, cuw, cuh, refi, pi->mv[pidx], pi->refp, pred, 0, 0, NULL);

        get_org_bi(org, pred[0][Y_C], pi->s_o[Y_C], cuw, cuh, pi->org_bi);

        SWAP(refi[lidx_ref], refi[lidx_cnd], t0);
        SWAP(lidx_ref, lidx_cnd, t0);
        SWAP(pidx_ref, pidx_cnd, t0);

        mvp_idx = pi->mvp_idx[pidx][lidx_ref];
        changed = 0;

        for(refi_cur = 0; refi_cur < pi->num_refp; refi_cur++)
        {
            refi[lidx_ref] = refi_cur;
            mecost = pi->fn_me(pi, x, y, log2_cuw, log2_cuh, &refi[lidx_ref], lidx_ref, pi->mvp[lidx_ref][mvp_idx], pi->mv_scale[lidx_ref][refi_cur], 1, ctx->sps.bit_depth_luma_minus8 + 8);
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

    pi->mvd[pidx][REFP_0][MV_X] = pi->mv[pidx][REFP_0][MV_X] - pi->mvp_scale[REFP_0][pi->refi[pidx][REFP_0]][pi->mvp_idx[pidx][REFP_0]][MV_X];
    pi->mvd[pidx][REFP_0][MV_Y] = pi->mv[pidx][REFP_0][MV_Y] - pi->mvp_scale[REFP_0][pi->refi[pidx][REFP_0]][pi->mvp_idx[pidx][REFP_0]][MV_Y];
    pi->mvd[pidx][REFP_1][MV_X] = pi->mv[pidx][REFP_1][MV_X] - pi->mvp_scale[REFP_1][pi->refi[pidx][REFP_1]][pi->mvp_idx[pidx][REFP_1]][MV_X];
    pi->mvd[pidx][REFP_1][MV_Y] = pi->mv[pidx][REFP_1][MV_Y] - pi->mvp_scale[REFP_1][pi->refi[pidx][REFP_1]][pi->mvp_idx[pidx][REFP_1]][MV_Y];

    cost = pinter_residue_rdo(ctx, core, x, y, log2_cuw, log2_cuh, pi->pred[pidx], pi->coef[pidx], pidx, pi->mvp_idx[pidx]);

    xeve_mcpy(pi->nnz_best[pidx], core->nnz, sizeof(int) * N_C);
    xeve_mcpy(pi->nnz_sub_best[pidx], core->nnz_sub, sizeof(int) * N_C * MAX_SUB_TB_NUM);

    return cost;
}

static int pinter_init_mt(XEVE_CTX *ctx, int thread_idx)
{
    XEVE_PIC    * pic;
    XEVE_PINTER * pi = &ctx->pinter[thread_idx];
    int           size;

    pic            = pi->pic_o = PIC_ORIG(ctx);
    pi->o[Y_C]     = pic->y;
    pi->o[U_C]     = pic->u;
    pi->o[V_C]     = pic->v;

    pi->s_o[Y_C]   = pic->s_l;
    pi->s_o[U_C]   = pic->s_c;
    pi->s_o[V_C]   = pic->s_c;

    pic            = pi->pic_m = PIC_MODE(ctx);
    pi->m[Y_C]     = pic->y;
    pi->m[U_C]     = pic->u;
    pi->m[V_C]     = pic->v;

    pi->s_m[Y_C]   = pic->s_l;
    pi->s_m[U_C]   = pic->s_c;
    pi->s_m[V_C]   = pic->s_c;

    pi->refp       = ctx->refp;
    pi->slice_type = ctx->slice_type;

    pi->map_mv     = ctx->map_mv;

    pi->w_scu      = ctx->w_scu;

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

    size = sizeof(s16) * N_C * MAX_CU_DIM;
    xeve_mset(pi->resi, 0, size);

    /* MV predictor */
    size = sizeof(s16) * REFP_NUM * MAX_NUM_MVP * MV_D;
    xeve_mset(pi->mvp, 0, size);

    size = sizeof(s16) * PRED_NUM * REFP_NUM * MV_D;
    xeve_mset(pi->mv, 0, size);

    size = sizeof(s16) * PRED_NUM * REFP_NUM * MV_D;
    xeve_mset(pi->mvd, 0, size);

    size = sizeof(s16) * MAX_CU_DIM;
    xeve_mset(pi->org_bi, 0, size);

    size = sizeof(s32) * REFP_NUM;
    xeve_mset(pi->mot_bits, 0, size);

    size = sizeof(pel) * (PRED_NUM + 1) * 2 * N_C * MAX_CU_DIM;
    xeve_mset(pi->pred, 0, size);

    return XEVE_OK;
}

int xeve_pinter_init_lcu(XEVE_CTX *ctx, XEVE_CORE *core)
{
    XEVE_PINTER *pi = &ctx->pinter[core->thread_cnt];

    pi->lambda_mv = (u32)floor(65536.0 * core->sqrt_lambda[0]);
    pi->qp_y      = core->qp_y;
    pi->qp_u      = core->qp_u;
    pi->qp_v      = core->qp_v;
    pi->poc       = ctx->poc.poc_val;
    pi->gop_size  = ctx->param.gop_size;

    return XEVE_OK;
}

static void check_best_mvp(XEVE_CTX *ctx, XEVE_CORE *core, s32 slice_type, s8 refi[REFP_NUM],
                           int lidx, int pidx, s16(*mvp)[2], s16 *mv, s16 *mvd, u8*mvp_idx)
{
    double cost, best_cost;
    int idx, best_idx;
    u32 bit_cnt;
    s16 mvd_tmp[REFP_NUM][MV_D];

    SBAC_LOAD(core->s_temp_run, core->s_curr_best[core->log2_cuw - 2][core->log2_cuh - 2]);

    xeve_sbac_bit_reset(&core->s_temp_run);

    mvd_tmp[lidx][MV_X] = mv[MV_X] - mvp[*mvp_idx][MV_X];
    mvd_tmp[lidx][MV_Y] = mv[MV_Y] - mvp[*mvp_idx][MV_Y];

    xeve_rdo_bit_cnt_mvp(ctx, core, slice_type, refi, mvd_tmp, pidx, *mvp_idx);
    bit_cnt = xeve_get_bit_number(&core->s_temp_run);

    best_cost = RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt);

    best_idx = *mvp_idx;

    for(idx = 0; idx < ORG_MAX_NUM_MVP; idx++)
    {
        if(idx)
        {
            int found_same_mvp = 0;
            for (int tmp_idx = idx - 1; tmp_idx >= 0; tmp_idx--)
            {
                /* encoder side pruning */
                if (mvp[idx][MV_X] == mvp[tmp_idx][MV_X] &&
                    mvp[idx][MV_Y] == mvp[tmp_idx][MV_Y])
                {
                    found_same_mvp = 1;
                    break;
                }
            }
            if (found_same_mvp)
            {
                continue;
            }
        }

        SBAC_LOAD(core->s_temp_run, core->s_curr_best[core->log2_cuw - 2][core->log2_cuh - 2]);

        xeve_sbac_bit_reset(&core->s_temp_run);

        mvd_tmp[lidx][MV_X] = mv[MV_X] - mvp[idx][MV_X];
        mvd_tmp[lidx][MV_Y] = mv[MV_Y] - mvp[idx][MV_Y];

        xeve_rdo_bit_cnt_mvp(ctx, core, slice_type, refi, mvd_tmp, pidx, idx);
        bit_cnt = xeve_get_bit_number(&core->s_temp_run);

        cost = RATE_TO_COST_LAMBDA(core->lambda[0], bit_cnt);
        if(cost < best_cost)
        {
            best_idx = idx;
        }
    }

    *mvp_idx = best_idx;
    mvd[MV_X] = mv[MV_X] - mvp[*mvp_idx][MV_X];
    mvd[MV_Y] = mv[MV_Y] - mvp[*mvp_idx][MV_Y];
}

double xeve_pinter_analyze_cu(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh, XEVE_MODE *mi, s16 coef[N_C][MAX_CU_DIM], pel *rec[N_C], int s_rec[N_C])
{
    s8 *refi;
    s8 refi_temp = 0;
    u32 mecost, best_mecost;
    pel(*pred)[N_C][MAX_CU_DIM];
    s16(*coef_t)[MAX_CU_DIM];
    s16(*mvp)[MV_D], *mv, *mvd;
    int cuw, cuh, t0, t1, best_idx = PRED_SKIP, i, j;
    u8 mvp_idx[REFP_NUM] = {0, 0};
    s8 refi_cur = 0;
    double cost, cost_best = MAX_COST;
    double cost_inter[PRED_NUM];
    int lidx, pidx;
    XEVE_PINTER *pi = &ctx->pinter[core->thread_cnt];

    cuw = (1 << log2_cuw);
    cuh = (1 << log2_cuh);

    for(i = 0; i < PRED_NUM; i++)
    {
        cost_inter[i] = MAX_COST;
    }

    /* skip mode */
    cost = cost_inter[PRED_SKIP] = xeve_analyze_skip(ctx, core, x, y, log2_cuw, log2_cuh);

    if(cost < cost_best)
    {
        core->cu_mode = MODE_SKIP;
        best_idx = PRED_SKIP;
        cost_inter[best_idx] = cost_best = cost;
        SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
        DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);

        xeve_mset(pi->nnz_best[PRED_SKIP], 0, sizeof(int) * N_C);
        xeve_mcpy(pi->nnz_sub_best[PRED_SKIP], core->nnz_sub, sizeof(int) * N_C * MAX_SUB_TB_NUM);
    }

    if (core->cu_mode == MODE_SKIP && pi->best_ssd >
        ((s64)1 << (log2_cuw + log2_cuh + ctx->sps.bit_depth_luma_minus8 + ctx->sps.bit_depth_luma_minus8)) * ctx->param.skip_th)
    {
        if(pi->slice_type == SLICE_B)
        {
            cost = cost_inter[PRED_DIR] = analyze_t_direct(ctx, core, x, y, log2_cuw, log2_cuh);
            if(cost < cost_best)
            {
                core->cu_mode = MODE_DIR;
                best_idx = PRED_DIR;
                cost_inter[best_idx] = cost_best = cost;

                SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
                DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);

                xeve_mcpy(pi->nnz_sub_best[PRED_DIR], core->nnz_sub, sizeof(int) * N_C * MAX_SUB_TB_NUM);
            }
        }

        /* Motion Search *********************************************************/
        for(lidx = 0; lidx <= ((pi->slice_type == SLICE_P) ? PRED_L0 : PRED_L1); lidx++)
        {
            pidx = lidx;
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
                xeve_get_motion(core->scup, lidx, ctx->map_refi, ctx->map_mv, pi->refp, core->cuw, core->cuh, ctx->w_scu, core->avail_cu, pi->refi_pred[lidx], mvp);
                mvp_idx[lidx] = pi->mvp_idx[PRED_SKIP][lidx];

                /* motion search ********************/
                mecost = pi->fn_me(pi, x, y, log2_cuw, log2_cuh, &refi_cur, lidx, mvp[mvp_idx[lidx]], mv, 0, ctx->sps.bit_depth_luma_minus8 + 8);

                pi->mv_scale[lidx][refi_cur][MV_X] = mv[MV_X];
                pi->mv_scale[lidx][refi_cur][MV_Y] = mv[MV_Y];
                if(mecost < best_mecost)
                {
                    best_mecost = mecost;
                    refi_temp = refi_cur;
                }
            }

            refi_cur = refi_temp;
            mv[MV_X] = pi->mv_scale[lidx][refi_cur][MV_X];
            mv[MV_Y] = pi->mv_scale[lidx][refi_cur][MV_Y];
            mvp = pi->mvp_scale[lidx][refi_cur];

            t0 = (lidx == 0) ? refi_cur : REFI_INVALID;
            t1 = (lidx == 1) ? refi_cur : REFI_INVALID;
            SET_REFI(refi, t0, t1);

            mvd[MV_X] = mv[MV_X] - mvp[mvp_idx[lidx]][MV_X];
            mvd[MV_Y] = mv[MV_Y] - mvp[mvp_idx[lidx]][MV_Y];

            check_best_mvp(ctx, core, pi->slice_type, refi, lidx, pidx, mvp, mv, mvd, &mvp_idx[lidx]);

            pi->mvp_idx[pidx][lidx] = mvp_idx[lidx];

            cost = cost_inter[pidx] = pinter_residue_rdo(ctx, core, x, y, log2_cuw, log2_cuh, pi->pred[PRED_NUM], pi->coef[PRED_NUM], pidx, mvp_idx);

            if(cost < cost_best)
            {
                core->cu_mode = MODE_INTER;
                best_idx = pidx;

                pi->mvp_idx[best_idx][lidx] = mvp_idx[lidx];
                cost_inter[best_idx] = cost_best = cost;
                SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
                DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);

                for(j = 0; j < N_C; j++)
                {
                    if(j != 0 && !ctx->sps.chroma_format_idc)
                        continue;
                    int size_tmp = (cuw * cuh) >> (j == 0 ? 0 : (ctx->param.cs_w_shift + ctx->param.cs_h_shift));
                    pi->nnz_best[pidx][j] = core->nnz[j];
                    xeve_mcpy(pi->nnz_sub_best[pidx][j], core->nnz_sub[j], sizeof(int) * MAX_SUB_TB_NUM);
                    xeve_mcpy(pred[0][j], pi->pred[PRED_NUM][0][j], size_tmp * sizeof(pel));
                    xeve_mcpy(coef_t[j], pi->coef[PRED_NUM][j], size_tmp * sizeof(s16));
                }
            }
        }

        if(pi->slice_type == SLICE_B)
        {
            pidx = PRED_BI;
            cost = cost_inter[pidx] = analyze_bi(ctx, core, x, y, log2_cuw, log2_cuh, cost_inter);

            if(cost < cost_best)
            {
                core->cu_mode = MODE_INTER;
                best_idx = pidx;
                cost_inter[best_idx] = cost_best = cost;
                SBAC_STORE(core->s_next_best[log2_cuw - 2][log2_cuh - 2], core->s_temp_best);
                DQP_STORE(core->dqp_next_best[log2_cuw - 2][log2_cuh - 2], core->dqp_temp_best);
                xeve_mcpy(pi->nnz_sub_best[best_idx], core->nnz_sub, sizeof(int) * N_C * MAX_SUB_TB_NUM);
            }
        }
    }




    /* reconstruct */
    for(j = 0; j < N_C; j++)
    {
        if(j != 0 && !ctx->sps.chroma_format_idc)
            continue;
        int size_tmp = (cuw * cuh) >> (j == 0 ? 0 : (ctx->param.cs_w_shift + ctx->param.cs_h_shift));
        xeve_mcpy(coef[j], pi->coef[best_idx][j], sizeof(s16) * size_tmp);
        xeve_mcpy(pi->residue[j], pi->coef[best_idx][j], sizeof(s16) * size_tmp);
    }

    if(ctx->pps.cu_qp_delta_enabled_flag)
    {
        xeve_set_qp(ctx, core, core->dqp_next_best[log2_cuw - 2][log2_cuh - 2].prev_qp);
    }

    ctx->fn_itdp(ctx, core, pi->residue, pi->nnz_sub_best[best_idx]);

    for(i = 0; i < N_C; i++)
    {
        if(i != 0 && !ctx->sps.chroma_format_idc)
            continue;
        rec[i] = pi->rec[best_idx][i];
        s_rec[i] = (i == 0 ? cuw : cuw >> ctx->param.cs_w_shift);
        ctx->fn_recon(ctx, core, pi->residue[i], pi->pred[best_idx][0][i], pi->nnz_best[best_idx][i], s_rec[i]
                    , (i == 0 ? cuh : cuh >> ctx->param.cs_h_shift), s_rec[i], rec[i], ctx->sps.bit_depth_chroma_minus8 + 8);
        core->nnz[i] = pi->nnz_best[best_idx][i];
        xeve_mcpy(core->nnz_sub[i], pi->nnz_sub_best[best_idx][i], sizeof(int) * MAX_SUB_TB_NUM);
    }

    mi->pred_y_best = pi->pred[best_idx][0][0];

    /* save all cu inforamtion ********************/
    for(lidx = 0; lidx < REFP_NUM; lidx++)
    {
        mi->refi[lidx] = pi->refi[best_idx][lidx];
        mi->mvp_idx[lidx] = pi->mvp_idx[best_idx][lidx];
        {
            mi->mv[lidx][MV_X] = pi->mv[best_idx][lidx][MV_X];
            mi->mv[lidx][MV_Y] = pi->mv[best_idx][lidx][MV_Y];
        }

        mi->mvd[lidx][MV_X] = pi->mvd[best_idx][lidx][MV_X];
        mi->mvd[lidx][MV_Y] = pi->mvd[best_idx][lidx][MV_Y];
    }

    return cost_inter[best_idx];
}

static void pinter_mc(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int w, int h, s8 refi[REFP_NUM], s16(*mv)[MV_D], XEVE_REFP(*refp)[REFP_NUM]
                    , pel pred[REFP_NUM][N_C][MAX_CU_DIM], int tmp_val1, int tmp_val2, s16 tmp_buf[MAX_CU_CNT_IN_LCU][REFP_NUM][MV_D])
{
    xeve_mc(x, y, ctx->w, ctx->h, w, h, refi, mv, refp, pred, ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc);
}

static int pinter_set_complexity(XEVE_CTX *ctx, int complexity)
{
    XEVE_PINTER *pi;

    for (int i = 0; i < ctx->param.threads; i++)
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
        ctx->fn_pinter_analyze_cu = xeve_pinter_analyze_cu;
        pi->me_level = ctx->param.me_sub;
        pi->fn_me = pinter_me_epzs;
        pi->complexity = complexity;
        pi->fn_mc = pinter_mc;
        pi->skip_merge_cand_num = ctx->param.merge_num;
        pi->me_complexity = ctx->param.me_algo;
    }
    return XEVE_OK;
}

int xeve_pinter_create(XEVE_CTX *ctx, int complexity)
{
    /* set function addresses */
    ctx->fn_pinter_init_mt = pinter_init_mt;
    ctx->fn_pinter_init_lcu = xeve_pinter_init_lcu;
    ctx->fn_pinter_set_complexity = pinter_set_complexity;

    XEVE_PINTER * pi;
    for (int i = 0; i < ctx->param.threads; i++)
    {
        pi = &ctx->pinter[i];
        /* set maximum/minimum value of search range */
        pi->min_clip[MV_X] = -MAX_CU_SIZE + 1;
        pi->min_clip[MV_Y] = -MAX_CU_SIZE + 1;
        pi->max_clip[MV_X] = ctx->param.w - 1;
        pi->max_clip[MV_Y] = ctx->param.h - 1;
        pi->mc_l_coeff = xeve_tbl_mc_l_coeff;
        pi->mc_c_coeff = xeve_tbl_mc_c_coeff;
    }

    return ctx->fn_pinter_set_complexity(ctx, complexity);
}

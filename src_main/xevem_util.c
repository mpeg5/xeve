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

#include "xevem_util.h"

#if GRAB_STAT
#include "xevem_stat.h"
#endif

void xeve_check_split_mode(int *split_allow, int log2_cuw, int log2_cuh, int boundary, int boundary_r, int log2_max_cuwh
                         , int x, int y, int im_w, int im_h, int sps_btt_flag, MODE_CONS mode_cons)
{
    if(!sps_btt_flag)
    {
        xeve_mset(split_allow, 0, sizeof(int) * SPLIT_CHECK_NUM);
        split_allow[SPLIT_QUAD] = 1;
        return;
    }

    int log2_sub_cuw, log2_sub_cuh;
    int long_side, ratio;
    int cu_max, from_boundary_b;
    cu_max = 1 << (log2_max_cuwh - 1);
    from_boundary_b = (y >= im_h - im_h%cu_max) && !(x >= im_w - im_w % cu_max);

    xeve_mset(split_allow, 0, sizeof(int) * SPLIT_CHECK_NUM);
    {
        split_allow[SPLIT_QUAD] = 0;

        if(log2_cuw == log2_cuh)
        {
            split_allow[SPLIT_BI_HOR] = ALLOW_SPLIT_RATIO(log2_cuw, 1);
            split_allow[SPLIT_BI_VER] = ALLOW_SPLIT_RATIO(log2_cuw, 1);
            split_allow[SPLIT_TRI_VER] = ALLOW_SPLIT_TRI(log2_cuw) && (log2_cuw > log2_cuh || (log2_cuw == log2_cuh && ALLOW_SPLIT_RATIO(log2_cuw, 2)));
            split_allow[SPLIT_TRI_HOR] = ALLOW_SPLIT_TRI(log2_cuh) && (log2_cuh > log2_cuw || (log2_cuw == log2_cuh && ALLOW_SPLIT_RATIO(log2_cuh, 2)));
        }
        else
        {
            if(log2_cuw > log2_cuh)
            {
                {
                    split_allow[SPLIT_BI_HOR] = ALLOW_SPLIT_RATIO(log2_cuw, log2_cuw - log2_cuh + 1);

                    log2_sub_cuw = log2_cuw - 1;
                    log2_sub_cuh = log2_cuh;
                    long_side = log2_sub_cuw > log2_sub_cuh ? log2_sub_cuw : log2_sub_cuh;
                    ratio = XEVE_ABS(log2_sub_cuw - log2_sub_cuh);

                    split_allow[SPLIT_BI_VER] = ALLOW_SPLIT_RATIO(long_side, ratio);
                    if (from_boundary_b && (ratio == 3 || ratio == 4))
                    {
                        split_allow[SPLIT_BI_VER] = 1;
                    }

                    split_allow[SPLIT_TRI_VER] = ALLOW_SPLIT_TRI(log2_cuw) && (log2_cuw > log2_cuh || (log2_cuw == log2_cuh && ALLOW_SPLIT_RATIO(log2_cuw, 2)));
                    split_allow[SPLIT_TRI_HOR] = ALLOW_SPLIT_TRI(log2_cuh) && (log2_cuh > log2_cuw || (log2_cuw == log2_cuh && ALLOW_SPLIT_RATIO(log2_cuh, 2)));
                }
            }
            else
            {
                log2_sub_cuh = log2_cuh - 1;
                log2_sub_cuw = log2_cuw;
                long_side = log2_sub_cuw > log2_sub_cuh ? log2_sub_cuw : log2_sub_cuh;
                ratio = XEVE_ABS(log2_sub_cuw - log2_sub_cuh);

                split_allow[SPLIT_BI_HOR] = ALLOW_SPLIT_RATIO(long_side, ratio);
                split_allow[SPLIT_BI_VER] = ALLOW_SPLIT_RATIO(log2_cuh, log2_cuh - log2_cuw + 1);
                split_allow[SPLIT_TRI_VER] = ALLOW_SPLIT_TRI(log2_cuw) && (log2_cuw > log2_cuh || (log2_cuw == log2_cuh && ALLOW_SPLIT_RATIO(log2_cuw, 2)));
                split_allow[SPLIT_TRI_HOR] = ALLOW_SPLIT_TRI(log2_cuh) && (log2_cuh > log2_cuw || (log2_cuw == log2_cuh && ALLOW_SPLIT_RATIO(log2_cuh, 2)));

            }
        }
    }

    if (boundary)
    {
        split_allow[NO_SPLIT] = 0;
        split_allow[SPLIT_TRI_VER] = 0;
        split_allow[SPLIT_TRI_HOR] = 0;
        split_allow[SPLIT_QUAD] = 0;
        if (boundary_r)
        {
            if (split_allow[SPLIT_BI_VER])
            {
                split_allow[SPLIT_BI_HOR] = 0;
            }
            else
            {
                split_allow[SPLIT_BI_HOR] = 1;
            }
        }
        else
        {
            if (split_allow[SPLIT_BI_HOR])
            {
                split_allow[SPLIT_BI_VER] = 0;
            }
            else
            {
                split_allow[SPLIT_BI_VER] = 1;
            }
        }
    }

    if (mode_cons == eOnlyInter)
    {
        int cuw = 1 << log2_cuw;
        int cuh = 1 << log2_cuh;
        for (int mode = SPLIT_BI_VER; mode < SPLIT_QUAD; ++mode)
            split_allow[mode] &= xeve_get_mode_cons_by_split(mode, cuw, cuh) == eAll;
    }
}

u16 xeve_get_avail_ibc(int x_scu, int y_scu, int w_scu, int h_scu, int scup, int cuw, int cuh, u32 * map_scu, u8* map_tidx)
{
    u16 avail = 0;
    int scuw = cuw >> MIN_CU_LOG2;
    int scuh = cuh >> MIN_CU_LOG2;

    if (x_scu > 0 && MCU_GET_IBC(map_scu[scup - 1]) && MCU_GET_COD(map_scu[scup - 1]) && (map_tidx[scup] == map_tidx[scup - 1]))
    {
        SET_AVAIL(avail, AVAIL_LE);

        if (y_scu + scuh < h_scu  && MCU_GET_COD(map_scu[scup + (scuh * w_scu) - 1]) && MCU_GET_IBC(map_scu[scup + (scuh * w_scu) - 1]) &&
            (map_tidx[scup] == map_tidx[scup + (scuh * w_scu) - 1]))
        {
            SET_AVAIL(avail, AVAIL_LO_LE);
        }
    }

    if (y_scu > 0)
    {
        if (MCU_GET_IBC(map_scu[scup - w_scu]) && (map_tidx[scup] == map_tidx[scup - w_scu]))
        {
            SET_AVAIL(avail, AVAIL_UP);
        }

        if (MCU_GET_IBC(map_scu[scup - w_scu + scuw - 1]) && (map_tidx[scup] == map_tidx[scup - w_scu + scuw - 1]))
        {
            SET_AVAIL(avail, AVAIL_RI_UP);
        }

        if (x_scu > 0 && MCU_GET_IBC(map_scu[scup - w_scu - 1]) && MCU_GET_COD(map_scu[scup - w_scu - 1]) && (map_tidx[scup] == map_tidx[scup - w_scu - 1]))
        {
            SET_AVAIL(avail, AVAIL_UP_LE);
        }

        if (x_scu + scuw < w_scu  && MCU_IS_COD_NIF(map_scu[scup - w_scu + scuw]) && MCU_GET_COD(map_scu[scup - w_scu + scuw]) && (map_tidx[scup] == map_tidx[scup - w_scu + scuw]))
        {
            SET_AVAIL(avail, AVAIL_UP_RI);
        }
    }

    if (x_scu + scuw < w_scu && MCU_GET_IBC(map_scu[scup + scuw]) && MCU_GET_COD(map_scu[scup + scuw]) && (map_tidx[scup] == map_tidx[scup + scuw]))
    {
        SET_AVAIL(avail, AVAIL_RI);

        if (y_scu + scuh < h_scu  && MCU_GET_COD(map_scu[scup + (scuh * w_scu) + scuw]) && MCU_GET_IBC(map_scu[scup + (scuh * w_scu) + scuw]) &&
            (map_tidx[scup] == map_tidx[scup + (scuh * w_scu) + scuw]))
        {
            SET_AVAIL(avail, AVAIL_LO_RI);
        }
    }

    return avail;
}


void xeve_get_mmvd_mvp_list(s8(*map_refi)[REFP_NUM], XEVE_REFP refp[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], int w_scu, int h_scu, int scup, u16 avail, int log2_cuw, int log2_cuh, int slice_t
    , int real_mv[][2][3], u32 *map_scu, int REF_SET[][MAX_NUM_ACTIVE_REF_FRAME], u16 avail_lr
    , u32 curr_ptr, u8 num_refp[REFP_NUM]
    , XEVE_HISTORY_BUFFER history_buffer, int admvp_flag, XEVE_SH* sh, int log2_max_cuwh, u8* map_tidx, int mmvd_idx)
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

    int base_st = mmvd_idx == -1 ? 0 : (mmvd_idx & 127) >> 5;
    int base_ed = mmvd_idx == -1 ? MMVD_BASE_MV_NUM : base_st + 1;

    int group_st = mmvd_idx == -1 ? 0 : mmvd_idx >> 7;
    int group_ed = mmvd_idx == -1 ? (small_cu ? 1 : 3) : group_st + 1;

    int mmvd_v_st = mmvd_idx == -1 ? 0 : (mmvd_idx & 31);
    int mmvd_v_ed = mmvd_idx == -1 ? MMVD_MAX_REFINE_NUM : mmvd_v_st + 1;

    if (admvp_flag == 0)
    {
        xeve_get_motion_skip_baseline(slice_t, scup, map_refi, map_mv, refp, cuw, cuh, w_scu, srefi, smvp, avail);
    }
    else
    {
        xeve_get_motion_merge_main(curr_ptr, slice_t, scup, map_refi, map_mv, refp, cuw, cuh, w_scu, h_scu, srefi, smvp, map_scu, avail_lr
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
                    base_mv_p[k][0][REFI] = base_mv_p[k][REFP_0][REFI];

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

s8 xeve_get_first_refi_main(int scup, int lidx, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], int cuw, int cuh, int w_scu, int h_scu, u32 *map_scu, u8 mvr_idx, u16 avail_lr
                     , s16(*map_unrefined_mv)[REFP_NUM][MV_D], XEVE_HISTORY_BUFFER history_buffer, int hmvp_flag, u8* map_tidx)
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


void xeve_get_default_motion_main(int neb_addr[MAX_NUM_POSSIBLE_SCAND], int valid_flag[MAX_NUM_POSSIBLE_SCAND], s8 cur_refi, int lidx, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], s8 *refi, s16 mv[MV_D]
    , u32 *map_scu, s16(*map_unrefined_mv)[REFP_NUM][MV_D], int scup, int w_scu, XEVE_HISTORY_BUFFER history_buffer, int hmvp_flag)
{
    int k;
    int found = 0;
    s8  tmp_refi = 0;

    *refi = 0;
    mv[MV_X] = 0;
    mv[MV_Y] = 0;

    found = xeve_get_default_motion(neb_addr, valid_flag, cur_refi, lidx, map_refi, map_mv, refi, mv, map_scu, map_unrefined_mv, scup, w_scu);

    if (hmvp_flag)
    {
        if (!found)
        {
            for (k = 1; k <= min(history_buffer.currCnt, ALLOWED_CHECKED_AMVP_NUM); k++)
            {
                tmp_refi = REFI_IS_VALID(history_buffer.history_refi_table[history_buffer.currCnt - k][lidx]) ? history_buffer.history_refi_table[history_buffer.currCnt - k][lidx] : REFI_INVALID;
                if (tmp_refi == cur_refi)
                {
                    found = 1;
                    *refi = tmp_refi;
                    mv[MV_X] = history_buffer.history_mv_table[history_buffer.currCnt - k][lidx][MV_X];
                    mv[MV_Y] = history_buffer.history_mv_table[history_buffer.currCnt - k][lidx][MV_Y];
                    break;
                }
            }
        }

        if (!found)
        {
            for (k = 1; k <= min(history_buffer.currCnt, ALLOWED_CHECKED_AMVP_NUM); k++)
            {
                tmp_refi = REFI_IS_VALID(history_buffer.history_refi_table[history_buffer.currCnt - k][lidx]) ? history_buffer.history_refi_table[history_buffer.currCnt - k][lidx] : REFI_INVALID;
                if (tmp_refi != REFI_INVALID)
                {
                    found = 1;
                    *refi = tmp_refi;
                    mv[MV_X] = history_buffer.history_mv_table[history_buffer.currCnt - k][lidx][MV_X];
                    mv[MV_Y] = history_buffer.history_mv_table[history_buffer.currCnt - k][lidx][MV_Y];
                    break;
                }
            }
        }
    }
}

__inline static void check_redundancy(int slice_type, s16 mvp[REFP_NUM][MAX_NUM_MVP][MV_D], s8 refi[REFP_NUM][MAX_NUM_MVP], int *count)
{
    int i;
    int cnt = *count;

    if(cnt > 0)
    {
        if(refi != NULL)
        {
            for(i = (cnt)-1; i >= 0; i--)
            {
                if(refi[REFP_0][cnt] == refi[REFP_0][i] && mvp[REFP_0][cnt][MV_X] == mvp[REFP_0][i][MV_X] && mvp[REFP_0][cnt][MV_Y] == mvp[REFP_0][i][MV_Y])
                {
                    if(slice_type != SLICE_B || (refi[REFP_1][cnt] == refi[REFP_1][i] && mvp[REFP_1][cnt][MV_X] == mvp[REFP_1][i][MV_X] && mvp[REFP_1][cnt][MV_Y] == mvp[REFP_1][i][MV_Y]))
                    {
                        cnt--;
                        break;
                    }
                }
            }
        }
        else
        {
            for(i = cnt - 1; i >= 0; i--)
            {
                if(mvp[REFP_0][cnt][MV_X] == mvp[REFP_0][i][MV_X] && mvp[REFP_0][cnt][MV_Y] == mvp[REFP_0][i][MV_Y])
                {
                    if(slice_type != SLICE_B || (mvp[REFP_1][cnt][MV_X] == mvp[REFP_1][i][MV_X] && mvp[REFP_1][cnt][MV_Y] == mvp[REFP_1][i][MV_Y]))
                    {
                        cnt--;
                        break;
                    }
                }
            }
        }
        *count = cnt;
    }
}


static int xeve_get_right_below_scup_merge(int scup, int cuw, int cuh, int w_scu, int h_scu, int bottom_right, int log2_max_cuwh)
{
    int scuw = cuw >> MIN_CU_LOG2;
    int scuh = cuh >> MIN_CU_LOG2;

    int x_scu = scup % w_scu + scuw - 1;
    int y_scu = scup / w_scu + scuh - 1;

    if (bottom_right == 0)            // fetch bottom sample
    {
        if (y_scu + 1 >= h_scu)
            return -1;
        else if ( ((y_scu + 1) << MIN_CU_LOG2 >> log2_max_cuwh) != (y_scu << MIN_CU_LOG2 >> log2_max_cuwh) )
            return -1; // check same CTU row, align to spec
        else
            return ((y_scu + 1) >> 1 << 1) * w_scu + (x_scu >> 1 << 1);
    }
    else if (bottom_right == 1)        // fetch bottom-to-right sample
    {
        if (x_scu + 1 >= w_scu)
            return -1;
        else if ( ((x_scu + 1) << MIN_CU_LOG2 >> log2_max_cuwh) != (x_scu << MIN_CU_LOG2 >> log2_max_cuwh) )
            return -1; // check same CTU column, align to spec
        else
            return (y_scu >> 1 << 1) * w_scu + ((x_scu + 1) >> 1 << 1);
    }
    return -1;
}

static int xeve_get_right_below_scup_merge_suco(int scup, int cuw, int cuh, int w_scu, int h_scu, int bottom_right, int log2_max_cuwh)
{
    int scuw = cuw >> MIN_CU_LOG2;
    int scuh = cuh >> MIN_CU_LOG2;

    int x_scu = scup % w_scu - 1;
    int y_scu = scup / w_scu + scuh - 1;

    if (bottom_right == 0)            // fetch bottom sample
    {
        if ( y_scu + 1 >= h_scu )
            return -1;
        else if ( ((y_scu + 1) << MIN_CU_LOG2 >> log2_max_cuwh) != (y_scu << MIN_CU_LOG2 >> log2_max_cuwh) )
            return -1; // check same CTU row, align to spec
        else
            return ((y_scu + 1) >> 1 << 1) * w_scu + ((x_scu + 1) >> 1 << 1);  // bottom sample
    }
    else if (bottom_right == 1)        // fetch bottom-to-left sample
    {
        if (x_scu < 0)
            return -1;
        else if ( ((x_scu + 1) << MIN_CU_LOG2 >> log2_max_cuwh) != (x_scu << MIN_CU_LOG2 >> log2_max_cuwh) )
            return -1; // check same CTU column, align to spec
        else
            return (y_scu >> 1 << 1) * w_scu + (x_scu >> 1 << 1);
    }
    return -1;
}

static int xeve_get_right_below_scup(int scup, int cuw, int cuh, int w_scu, int h_scu)
{
    int scuw = cuw >> MIN_CU_LOG2;
    int scuh = cuh >> MIN_CU_LOG2;

    int x_scu = scup % w_scu + scuw - 1;
    int y_scu = scup / w_scu + scuh - 1;

    if (x_scu + 1 < w_scu && y_scu + 1 < h_scu)
    {
        return (y_scu + 1)*w_scu + (x_scu + 1);
    }
    else if (x_scu + 1 < w_scu)
    {
        return y_scu*w_scu + (x_scu + 1);
    }
    else if (y_scu + 1 < h_scu)
    {
        return (y_scu + 1)*w_scu + x_scu;
    }
    else
    {
        return y_scu*w_scu + x_scu;
    }
}

void xeve_clip_mv_pic(int x, int y, int maxX, int maxY, s16 mvp[REFP_NUM][MV_D])
{
    int minXY = -PIC_PAD_SIZE_L;
    mvp[REFP_0][MV_X] = (x + mvp[REFP_0][MV_X]) < minXY ? -(x + minXY) : mvp[REFP_0][MV_X];
    mvp[REFP_1][MV_X] = (x + mvp[REFP_1][MV_X]) < minXY ? -(x + minXY) : mvp[REFP_1][MV_X];
    mvp[REFP_0][MV_Y] = (y + mvp[REFP_0][MV_Y]) < minXY ? -(y + minXY) : mvp[REFP_0][MV_Y];
    mvp[REFP_1][MV_Y] = (y + mvp[REFP_1][MV_Y]) < minXY ? -(y + minXY) : mvp[REFP_1][MV_Y];

    mvp[REFP_0][MV_X] = (x + mvp[REFP_0][MV_X]) > maxX ? (maxX - x) : mvp[REFP_0][MV_X];
    mvp[REFP_1][MV_X] = (x + mvp[REFP_1][MV_X]) > maxX ? (maxX - x) : mvp[REFP_1][MV_X];
    mvp[REFP_0][MV_Y] = (y + mvp[REFP_0][MV_Y]) > maxY ? (maxY - y) : mvp[REFP_0][MV_Y];
    mvp[REFP_1][MV_Y] = (y + mvp[REFP_1][MV_Y]) > maxY ? (maxY - y) : mvp[REFP_1][MV_Y];
}

void xeve_get_mv_collocated(XEVE_REFP(*refp)[REFP_NUM], u32 poc, int scup, int c_scu, u16 w_scu, u16 h_scu, s16 mvp[REFP_NUM][MV_D], s8 *available_pred_idx, XEVE_SH* sh)
{
    *available_pred_idx = 0;

    int temporal_mvp_asigned_flag = sh->temporal_mvp_asigned_flag;
    int collocated_from_list_idx = (sh->slice_type == SLICE_P) ? REFP_0 : REFP_1;  // Specifies source (List ID) of the collocated picture, equialent of the collocated_from_l0_flag
    int collocated_from_ref_idx = 0;        // Specifies source (RefID_ of the collocated picture, equialent of the collocated_ref_idx
    int collocated_mvp_source_list_idx = REFP_0;  // Specifies source (List ID) in collocated pic that provides MV information (Applicability is function of NoBackwardPredFlag)

    if(sh->temporal_mvp_asigned_flag)
    {
        collocated_from_list_idx = sh->collocated_from_list_idx;
        collocated_from_ref_idx = sh->collocated_from_ref_idx;
        collocated_mvp_source_list_idx = sh->collocated_mvp_source_list_idx;
    }

    XEVE_REFP colPic = (refp[collocated_from_ref_idx][collocated_from_list_idx]);  // col picture is ref idx 0 and list 1

    int neb_addr_coll = scup;     // Col 
    int dpoc_co[REFP_NUM] = {0, 0};
    int dpoc[REFP_NUM] = {0, 0};
    int ver_refi[REFP_NUM] = {-1, -1};
    memset(mvp, 0, sizeof(s16) * REFP_NUM * MV_D);

    s8(*map_refi_co)[REFP_NUM] = colPic.map_refi;
    dpoc[REFP_0] = poc - refp[0][REFP_0].poc;
    dpoc[REFP_1] = poc - refp[0][REFP_1].poc;

    if(!temporal_mvp_asigned_flag)
    {
        dpoc_co[REFP_0] = colPic.poc - colPic.list_poc[map_refi_co[neb_addr_coll][REFP_0]]; //POC1
        dpoc_co[REFP_1] = colPic.poc - colPic.list_poc[map_refi_co[neb_addr_coll][REFP_1]]; //POC2

        for(int lidx = 0; lidx < REFP_NUM; lidx++)
        {
            s8 refidx = map_refi_co[neb_addr_coll][lidx];
            if(dpoc_co[lidx] != 0 && REFI_IS_VALID(refidx))
            {
                int ratio_tmvp = ((dpoc[lidx]) << MVP_SCALING_PRECISION) / dpoc_co[lidx];
                ver_refi[lidx] = 0; // ref idx
                s16 *mvc = colPic.map_mv[neb_addr_coll][lidx];
                scaling_mv(ratio_tmvp, mvc, mvp[lidx]);
            }
            else
            {
                mvp[lidx][MV_X] = 0;
                mvp[lidx][MV_Y] = 0;
            }
        }
    }
    else
    {
        // collocated_mvp_source_list_idx = REFP_0; // specified above
        s8 refidx = map_refi_co[neb_addr_coll][collocated_mvp_source_list_idx];
        dpoc_co[REFP_0] = colPic.poc - colPic.list_poc[refidx];
        {
            if(dpoc_co[REFP_0] != 0 && REFI_IS_VALID(refidx))
            {
                ver_refi[REFP_0] = 0;
                ver_refi[REFP_1] = 0;
                s16 *mvc = colPic.map_mv[neb_addr_coll][collocated_mvp_source_list_idx]; //  collocated_mvp_source_list_idx == 0 for RA
                int ratio_tmvp = ((dpoc[REFP_0]) << MVP_SCALING_PRECISION) / dpoc_co[REFP_0];
                scaling_mv(ratio_tmvp, mvc, mvp[REFP_0]);

                ratio_tmvp = ((dpoc[REFP_1]) << MVP_SCALING_PRECISION) / dpoc_co[REFP_0];
                scaling_mv(ratio_tmvp, mvc, mvp[REFP_1]);
            }
            else
            {
                mvp[REFP_0][MV_X] = 0;
                mvp[REFP_0][MV_Y] = 0;
                mvp[REFP_1][MV_X] = 0;
                mvp[REFP_1][MV_Y] = 0;
            }
        }
    }

    {
        int maxX = PIC_PAD_SIZE_L + (w_scu << MIN_CU_LOG2) - 1;
        int maxY = PIC_PAD_SIZE_L + (h_scu << MIN_CU_LOG2) - 1;
        int x = (c_scu % w_scu) << MIN_CU_LOG2;
        int y = (c_scu / w_scu) << MIN_CU_LOG2;
        xeve_clip_mv_pic(x, y, maxX, maxY, mvp);
    }

    int flag = REFI_IS_VALID(ver_refi[REFP_0]) + (REFI_IS_VALID(ver_refi[REFP_1]) << 1);
    *available_pred_idx = flag; // combines flag and indication on what type of prediction is ( 0 - not available, 1 = uniL0, 2 = uniL1, 3 = Bi)
}

void xeve_get_motion_merge_main(int ptr, int slice_type, int scup, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], XEVE_REFP refp[REFP_NUM]
                              , int cuw, int cuh, int w_scu, int h_scu, s8 refi[REFP_NUM][MAX_NUM_MVP], s16 mvp[REFP_NUM][MAX_NUM_MVP][MV_D], u32 *map_scu, u16 avail_lr
                              , s16(*map_unrefined_mv)[REFP_NUM][MV_D], XEVE_HISTORY_BUFFER history_buffer, u8 ibc_flag, XEVE_REFP(*refplx)[REFP_NUM]
                              , XEVE_SH* sh, int log2_max_cuwh, u8* map_tidx)
{
    BOOL tmp_bottom_right = 0; // Bottom first
    int is_sps_admvp = 1;
    int small_cu = 0;

    if (cuw*cuh <= NUM_SAMPLES_BLOCK)
    {
        small_cu = 1;
    }

    int k, cnt = 0;
    int neb_addr[MAX_NUM_POSSIBLE_SCAND], valid_flag[MAX_NUM_POSSIBLE_SCAND];
    s16 tmvp[REFP_NUM][MV_D];
    int scup_tmp;
    int cur_num, i, idx0, idx1;
    int c_win = 0;
    xeve_mset(mvp, 0, MAX_NUM_MVP * REFP_NUM * MV_D * sizeof(s16));
    xeve_mset(refi, REFI_INVALID, MAX_NUM_MVP * REFP_NUM * sizeof(s8));
    s8  refidx = REFI_INVALID;
    s8  *ref_dst = NULL;
    s16 *map_mv_dst_L0 = NULL;
    s16 *map_mv_dst_L1 = NULL;
    s8  *ref_src = NULL;
    s16 *map_mv_src = NULL;
    for (k = 0; k < MAX_NUM_POSSIBLE_SCAND; k++)
    {
        valid_flag[k] = 0;
    }
    xeve_check_motion_availability(scup, cuw, cuh, w_scu, h_scu, neb_addr, valid_flag, map_scu, avail_lr, 1, ibc_flag, map_tidx);

    for (k = 0; k < 5; k++)
    {
        ref_dst = &(refi[0][cnt]);
        map_mv_dst_L0 = mvp[REFP_0][cnt];
        map_mv_dst_L1 = mvp[REFP_1][cnt];
        ref_src = map_refi[neb_addr[k]];
        map_mv_src = &(map_mv[neb_addr[k]][0][0]);

        if (valid_flag[k])
        {
            if ((NULL != map_unrefined_mv) && MCU_GET_DMVRF(map_scu[neb_addr[k]]))
            {
                ref_src = map_refi[neb_addr[k]];
                map_mv_src = &(map_unrefined_mv[neb_addr[k]][0][0]);
            }
            xeve_get_merge_insert_mv(ref_dst, map_mv_dst_L0, map_mv_dst_L1, ref_src, map_mv_src, slice_type, cuw, cuh, is_sps_admvp);
            check_redundancy(slice_type, mvp, refi, &cnt);
            cnt++;
        }
        if (cnt == (small_cu ? MAX_NUM_MVP_SMALL_CU - 1 : MAX_NUM_MVP - 1))
        {
            break;
        }
    }

    int tmvp_cnt_pos0 = 0, tmvp_cnt_pos1 = 0;
    int tmvp_added = 0;

    if (!tmvp_added)
    {// TMVP-central
        s8 available_pred_idx = 0;

        int x_scu = (scup % w_scu);
        int y_scu = (scup / w_scu);
        int scu_col = ((x_scu + (cuw >> 1 >> MIN_CU_LOG2)) >> 1 << 1) + ((y_scu + (cuh >> 1 >> MIN_CU_LOG2)) >> 1 << 1) * w_scu; // 8x8 grid
        xeve_get_mv_collocated(refplx,ptr, scu_col, scup, w_scu, h_scu, tmvp, &available_pred_idx, sh);

        tmvp_cnt_pos0 = cnt;
        if (available_pred_idx != 0)
        {
            ref_dst = &(refi[0][cnt]);
            map_mv_dst_L0 = mvp[REFP_0][cnt];
            map_mv_dst_L1 = mvp[REFP_1][cnt];
            s8 refs[2] = { -1, -1 };
            refs[0] = (available_pred_idx == 1 || available_pred_idx == 3) ? 0 : -1;
            refs[1] = (available_pred_idx == 2 || available_pred_idx == 3) ? 0 : -1;
            ref_src = refs;
            map_mv_src = &(tmvp[0][0]);
            xeve_get_merge_insert_mv(ref_dst, map_mv_dst_L0, map_mv_dst_L1, ref_src, map_mv_src, slice_type, cuw, cuh, is_sps_admvp);

            check_redundancy(slice_type, mvp, refi, &cnt);
            cnt++;
            tmvp_cnt_pos1 = cnt;
            if (tmvp_cnt_pos1 == tmvp_cnt_pos0 + 1)
                tmvp_added = 1;
            if (cnt >= (small_cu ? MAX_NUM_MVP_SMALL_CU : MAX_NUM_MVP))
            {
                return;
            }
        }
    } // TMVP-central
    if (!tmvp_added)
    {// Bottom first
        s8 available_pred_idx = 0;
        tmp_bottom_right = 0;
        if (avail_lr == LR_01)
            scup_tmp = xeve_get_right_below_scup_merge_suco(scup, cuw, cuh, w_scu, h_scu, tmp_bottom_right, log2_max_cuwh);
        else
            scup_tmp = xeve_get_right_below_scup_merge(scup, cuw, cuh, w_scu, h_scu, tmp_bottom_right, log2_max_cuwh);
        if (scup_tmp != -1)  // if available, add it to candidate list
        {
            xeve_get_mv_collocated(refplx, ptr, scup_tmp, scup, w_scu, h_scu, tmvp, &available_pred_idx, sh);
            tmvp_cnt_pos0 = cnt;
            if (available_pred_idx != 0)
            {
                ref_dst = &(refi[0][cnt]);
                map_mv_dst_L0 = mvp[REFP_0][cnt];
                map_mv_dst_L1 = mvp[REFP_1][cnt];
                s8 refs[2] = { -1, -1 };
                refs[0] = (available_pred_idx == 1 || available_pred_idx == 3) ? 0 : -1;
                refs[1] = (available_pred_idx == 2 || available_pred_idx == 3) ? 0 : -1;
                ref_src = refs;
                map_mv_src = &(tmvp[0][0]);
                xeve_get_merge_insert_mv(ref_dst, map_mv_dst_L0, map_mv_dst_L1, ref_src, map_mv_src, slice_type, cuw, cuh, is_sps_admvp);
                check_redundancy(slice_type, mvp, refi, &cnt);
                cnt++;
                tmvp_cnt_pos1 = cnt;
                if (tmvp_cnt_pos1 == tmvp_cnt_pos0 + 1)
                    tmvp_added = 1;
                if (cnt >= (small_cu ? MAX_NUM_MVP_SMALL_CU : MAX_NUM_MVP))
                {
                    return;
                }
            }
        }
    }
    if (!tmvp_added)
    {
        s8 available_pred_idx = 0;
        if (avail_lr == LR_01)
            scup_tmp = xeve_get_right_below_scup_merge_suco(scup, cuw, cuh, w_scu, h_scu, !tmp_bottom_right, log2_max_cuwh);
        else
            scup_tmp = xeve_get_right_below_scup_merge(scup, cuw, cuh, w_scu, h_scu, !tmp_bottom_right, log2_max_cuwh);
        if (scup_tmp != -1)  // if available, add it to candidate list
        {
            xeve_get_mv_collocated(refplx, ptr, scup_tmp, scup, w_scu, h_scu, tmvp, &available_pred_idx, sh);

            tmvp_cnt_pos0 = cnt;
            if (available_pred_idx != 0)
            {
                ref_dst = &(refi[0][cnt]);
                map_mv_dst_L0 = mvp[REFP_0][cnt];
                map_mv_dst_L1 = mvp[REFP_1][cnt];
                s8 refs[2] = { -1, -1 };
                refs[0] = (available_pred_idx == 1 || available_pred_idx == 3) ? 0 : -1;
                refs[1] = (available_pred_idx == 2 || available_pred_idx == 3) ? 0 : -1;
                ref_src = refs;
                map_mv_src = &(tmvp[0][0]);
                xeve_get_merge_insert_mv(ref_dst, map_mv_dst_L0, map_mv_dst_L1, ref_src, map_mv_src, slice_type, cuw, cuh, is_sps_admvp);
                check_redundancy(slice_type, mvp, refi, &cnt);
                cnt++;
                tmvp_cnt_pos1 = cnt;
                if (tmvp_cnt_pos1 == tmvp_cnt_pos0 + 1)
                    tmvp_added = 1;
                if (cnt >= (small_cu ? MAX_NUM_MVP_SMALL_CU : MAX_NUM_MVP))
                {
                    return;
                }
            }
        }
    }

    if (cnt < (small_cu ? MAX_NUM_MVP_SMALL_CU : MAX_NUM_MVP))
    {

        for (k = 3; k <= min(history_buffer.currCnt, small_cu ? ALLOWED_CHECKED_NUM_SMALL_CU : ALLOWED_CHECKED_NUM); k += 4)
        {
            ref_dst = &(refi[0][cnt]);
            map_mv_dst_L0 = mvp[REFP_0][cnt];
            map_mv_dst_L1 = mvp[REFP_1][cnt];
            ref_src = history_buffer.history_refi_table[history_buffer.currCnt - k];
            map_mv_src = &(history_buffer.history_mv_table[history_buffer.currCnt - k][0][0]);
            xeve_get_merge_insert_mv(ref_dst, map_mv_dst_L0, map_mv_dst_L1, ref_src, map_mv_src, slice_type, cuw, cuh, is_sps_admvp);
            check_redundancy(slice_type, mvp, refi, &cnt);
            cnt++;
            if (cnt >= (small_cu ? MAX_NUM_MVP_SMALL_CU : MAX_NUM_MVP))
            {
                return;
            }

        }
    }
    // B slice mv combination
    if(check_bi_applicability(slice_type, cuw, cuh, is_sps_admvp))
    {
        int priority_list0[MAX_NUM_MVP*MAX_NUM_MVP] = { 0, 1, 0, 2, 1, 2, 0, 3, 1, 3, 2, 3, 0, 4, 1, 4, 2, 4, 3, 4 };
        int priority_list1[MAX_NUM_MVP*MAX_NUM_MVP] = { 1, 0, 2, 0, 2, 1, 3, 0, 3, 1, 3, 2, 4, 0, 4, 1, 4, 2, 4, 3 };
        cur_num = cnt;
        for (i = 0; i < cur_num*(cur_num - 1) && cnt != (small_cu ? MAX_NUM_MVP_SMALL_CU : MAX_NUM_MVP); i++)
        {
            idx0 = priority_list0[i];
            idx1 = priority_list1[i];

            if (REFI_IS_VALID(refi[REFP_0][idx0]) && REFI_IS_VALID(refi[REFP_1][idx1]))
            {
                refi[REFP_0][cnt] = refi[REFP_0][idx0];
                mvp[REFP_0][cnt][MV_X] = mvp[REFP_0][idx0][MV_X];
                mvp[REFP_0][cnt][MV_Y] = mvp[REFP_0][idx0][MV_Y];

                refi[REFP_1][cnt] = refi[REFP_1][idx1];
                mvp[REFP_1][cnt][MV_X] = mvp[REFP_1][idx1][MV_X];
                mvp[REFP_1][cnt][MV_Y] = mvp[REFP_1][idx1][MV_Y];
                cnt++;
            }
        }
        if (cnt == (small_cu ? MAX_NUM_MVP_SMALL_CU : MAX_NUM_MVP))
        {
            return;
        }
    }

    for (k = cnt; k < (small_cu ? MAX_NUM_MVP_SMALL_CU : MAX_NUM_MVP); k++)
    {
        refi[REFP_0][k] = 0;
        mvp[REFP_0][k][MV_X] = 0;
        mvp[REFP_0][k][MV_Y] = 0;
        if(!check_bi_applicability(slice_type, cuw, cuh, is_sps_admvp))
        {
            refi[REFP_1][k] = REFI_INVALID;
            mvp[REFP_1][k][MV_X] = 0;
            mvp[REFP_1][k][MV_Y] = 0;
        }
        else
        {
            refi[REFP_1][k] = 0;
            mvp[REFP_1][k][MV_X] = 0;
            mvp[REFP_1][k][MV_Y] = 0;
        }
    }
}

void xeve_get_merge_insert_mv(s8* refi_dst, s16 *mvp_dst_L0, s16 *mvp_dst_L1, s8* map_refi_src, s16* map_mv_src, int slice_type, int cuw, int cuh, int is_sps_admvp)
{
    refi_dst[REFP_0 * MAX_NUM_MVP] = REFI_IS_VALID(map_refi_src[REFP_0]) ? map_refi_src[REFP_0] : REFI_INVALID;
    mvp_dst_L0[MV_X] = map_mv_src[REFP_0 * REFP_NUM + MV_X];
    mvp_dst_L0[MV_Y] = map_mv_src[REFP_0 * REFP_NUM + MV_Y];

    if (slice_type == SLICE_B)
    {
        if (!REFI_IS_VALID(map_refi_src[REFP_0]))
        {
            refi_dst[REFP_1 * MAX_NUM_MVP] = REFI_IS_VALID(map_refi_src[REFP_1]) ? map_refi_src[REFP_1] : REFI_INVALID;
            mvp_dst_L1[MV_X] = map_mv_src[REFP_1 * REFP_NUM + MV_X];
            mvp_dst_L1[MV_Y] = map_mv_src[REFP_1 * REFP_NUM + MV_Y];
        }
        else if (!check_bi_applicability(slice_type, cuw, cuh, is_sps_admvp))
        {
            refi_dst[REFP_1 * MAX_NUM_MVP] = REFI_INVALID; // TBD: gcc10 triggers stringop-overflow at this line
            mvp_dst_L1[MV_X] = 0;
            mvp_dst_L1[MV_Y] = 0;
        }
        else
        {
            refi_dst[REFP_1 * MAX_NUM_MVP] = REFI_IS_VALID(map_refi_src[REFP_1]) ? map_refi_src[REFP_1] : REFI_INVALID;
            mvp_dst_L1[MV_X] = map_mv_src[REFP_1 * REFP_NUM + MV_X];
            mvp_dst_L1[MV_Y] = map_mv_src[REFP_1 * REFP_NUM + MV_Y];
        }
    }
}

void xeve_get_motion_from_mvr(u8 mvr_idx, int poc, int scup, int lidx, s8 cur_refi, int num_refp
                            , s16(*map_mv)[REFP_NUM][MV_D], s8(*map_refi)[REFP_NUM], XEVE_REFP(*refp)[REFP_NUM]
                            , int cuw, int cuh, int w_scu, int h_scu, u16 avail, s16 mvp[MAX_NUM_MVP][MV_D], s8 refi[MAX_NUM_MVP], u32* map_scu, u16 avail_lr
                            , s16(*map_unrefined_mv)[REFP_NUM][MV_D], XEVE_HISTORY_BUFFER history_buffer, int hmvp_flag, u8* map_tidx)
{
    int i, t0, poc_refi_cur;
    int ratio[MAX_NUM_REF_PICS];
    int neb_addr[MAX_NUM_POSSIBLE_SCAND], valid_flag[MAX_NUM_POSSIBLE_SCAND];
    int rounding = mvr_idx > 0 ? 1 << (mvr_idx - 1) : 0;
    s8 default_refi;
    s16 default_mv[MV_D];
    s16 mvp_temp[MV_D];

    xeve_check_motion_availability(scup, cuw, cuh, w_scu, h_scu, neb_addr, valid_flag, map_scu, avail_lr, 1, 0, map_tidx);
    xeve_get_default_motion_main(neb_addr, valid_flag, cur_refi, lidx, map_refi, map_mv, &default_refi, default_mv, map_scu
                               , map_unrefined_mv, scup, w_scu, history_buffer, hmvp_flag);

    poc_refi_cur = refp[cur_refi][lidx].poc;
    for(i = 0; i < num_refp; i++)
    {
        t0 = poc - refp[i][lidx].poc;
        ratio[i] = ((poc - poc_refi_cur) << MVP_SCALING_PRECISION) / t0;

    }

    assert(mvr_idx < 5);

    if(valid_flag[mvr_idx])
    {
        refi[0] = REFI_IS_VALID(map_refi[neb_addr[mvr_idx]][lidx]) ? map_refi[neb_addr[mvr_idx]][lidx] : REFI_INVALID;
        if(refi[0] == cur_refi)
        {
            if(MCU_GET_DMVRF(map_scu[neb_addr[mvr_idx]]))
            {
                mvp_temp[MV_X] = map_unrefined_mv[neb_addr[mvr_idx]][lidx][MV_X];
                mvp_temp[MV_Y] = map_unrefined_mv[neb_addr[mvr_idx]][lidx][MV_Y];
            }
            else
            {
                mvp_temp[MV_X] = map_mv[neb_addr[mvr_idx]][lidx][MV_X];
                mvp_temp[MV_Y] = map_mv[neb_addr[mvr_idx]][lidx][MV_Y];
            }
        }
        else if(refi[0] == REFI_INVALID)
        {
            refi[0] = default_refi;
            if(refi[0] == cur_refi)
            {
                mvp_temp[MV_X] = default_mv[MV_X];
                mvp_temp[MV_Y] = default_mv[MV_Y];
            }
            else
            {
                scaling_mv(ratio[refi[0]], default_mv, mvp_temp);
            }
        }
        else
        {
            if(MCU_GET_DMVRF(map_scu[neb_addr[mvr_idx]]))
            {
                scaling_mv(ratio[refi[0]], map_unrefined_mv[neb_addr[mvr_idx]][lidx], mvp_temp);
            }
            else
            {
                scaling_mv(ratio[refi[0]], map_mv[neb_addr[mvr_idx]][lidx], mvp_temp);
            }
        }
    }
    else
    {
        refi[0] = default_refi;

        if(refi[0] == cur_refi)
        {
            mvp_temp[MV_X] = default_mv[MV_X];
            mvp_temp[MV_Y] = default_mv[MV_Y];
        }
        else
        {
            scaling_mv(ratio[refi[0]], default_mv, mvp_temp);
        }

    }
    mvp[0][MV_X] = (mvp_temp[MV_X] >= 0) ? (((mvp_temp[MV_X] + rounding) >> mvr_idx) << mvr_idx) : -(((-mvp_temp[MV_X] + rounding) >> mvr_idx) << mvr_idx);
    mvp[0][MV_Y] = (mvp_temp[MV_Y] >= 0) ? (((mvp_temp[MV_Y] + rounding) >> mvr_idx) << mvr_idx) : -(((-mvp_temp[MV_Y] + rounding) >> mvr_idx) << mvr_idx);
}

void xeve_split_get_split_rdo_order(int cuw, int cuh, SPLIT_MODE splits[MAX_SPLIT_NUM])
{
    if (cuw < cuh)
    {
        splits[1] = SPLIT_BI_HOR;
        splits[2] = SPLIT_BI_VER;
    }
    else
    {
        splits[1] = SPLIT_BI_VER;
        splits[2] = SPLIT_BI_HOR;
    }
    splits[3] = SPLIT_TRI_VER;
    splits[4] = SPLIT_TRI_HOR;
    splits[5] = SPLIT_QUAD;
    splits[0] = NO_SPLIT;
}

SPLIT_DIR xeve_split_get_direction(SPLIT_MODE mode)
{
    switch (mode)
    {
    case SPLIT_BI_HOR:
    case SPLIT_TRI_HOR:
        return SPLIT_HOR;
    default:
        return SPLIT_VER;
    }
}

int  xeve_split_is_TT(SPLIT_MODE mode)
{
    return (mode == SPLIT_TRI_HOR) || (mode == SPLIT_TRI_VER) ? 1 : 0;
}

int  xeve_split_is_BT(SPLIT_MODE mode)
{
    return (mode == SPLIT_BI_HOR) || (mode == SPLIT_BI_VER) ? 1 : 0;
}


int xeve_split_part_count(int split_mode)
{
    switch(split_mode)
    {
        case SPLIT_BI_VER:
        case SPLIT_BI_HOR:
            return 2;
        case SPLIT_TRI_VER:
        case SPLIT_TRI_HOR:
            return 3;
        case SPLIT_QUAD:
            return 4;
        default:
            // NO_SPLIT
            return 0;
    }
}

int xeve_split_get_part_size(int split_mode, int part_num, int length)
{
    int ans = length;
    switch(split_mode)
    {
        case SPLIT_QUAD:
        case SPLIT_BI_HOR:
        case SPLIT_BI_VER:
            ans = length >> 1;
            break;
        case SPLIT_TRI_HOR:
        case SPLIT_TRI_VER:
            if(part_num == 1)
                ans = length >> 1;
            else
                ans = length >> 2;
            break;
    }
    return ans;
}

int xeve_split_get_part_size_idx(int split_mode, int part_num, int length_idx)
{
    int ans = length_idx;
    switch(split_mode)
    {
        case SPLIT_QUAD:
        case SPLIT_BI_HOR:
        case SPLIT_BI_VER:
            ans = length_idx - 1;
            break;
        case SPLIT_TRI_HOR:
        case SPLIT_TRI_VER:
            if(part_num == 1)
                ans = length_idx - 1;
            else
                ans = length_idx - 2;
            break;
    }
    return ans;
}

int xeve_split_is_vertical(SPLIT_MODE mode)
{
    return xeve_split_get_direction(mode) == SPLIT_VER ? 1 : 0;
}

int xeve_split_is_horizontal(SPLIT_MODE mode)
{
    return xeve_split_get_direction(mode) == SPLIT_HOR ? 1 : 0;
}

void xeve_split_get_part_structure_main(int split_mode, int x0, int y0, int cuw, int cuh, int cup, int cud, int log2_culine, XEVE_SPLIT_STRUCT* split_struct)
{
    int i;
    int log_cuw, log_cuh;
    int cup_w, cup_h;

    split_struct->part_count = xeve_split_part_count(split_mode);
    log_cuw = CONV_LOG2(cuw);
    log_cuh = CONV_LOG2(cuh);
    split_struct->x_pos[0] = x0;
    split_struct->y_pos[0] = y0;
    split_struct->cup[0] = cup;

    switch(split_mode)
    {
        case NO_SPLIT:
        {
            split_struct->width[0] = cuw;
            split_struct->height[0] = cuh;
            split_struct->log_cuw[0] = log_cuw;
            split_struct->log_cuh[0] = log_cuh;
        }
        break;

        case SPLIT_QUAD:
        {
            split_struct->width[0] = cuw >> 1;
            split_struct->height[0] = cuh >> 1;
            split_struct->log_cuw[0] = log_cuw - 1;
            split_struct->log_cuh[0] = log_cuh - 1;
            for(i = 1; i < split_struct->part_count; ++i)
            {
                split_struct->width[i] = split_struct->width[0];
                split_struct->height[i] = split_struct->height[0];
                split_struct->log_cuw[i] = split_struct->log_cuw[0];
                split_struct->log_cuh[i] = split_struct->log_cuh[0];
            }
            split_struct->x_pos[1] = x0 + split_struct->width[0];
            split_struct->y_pos[1] = y0;
            split_struct->x_pos[2] = x0;
            split_struct->y_pos[2] = y0 + split_struct->height[0];
            split_struct->x_pos[3] = split_struct->x_pos[1];
            split_struct->y_pos[3] = split_struct->y_pos[2];
            cup_w = (split_struct->width[0] >> MIN_CU_LOG2);
            cup_h = ((split_struct->height[0] >> MIN_CU_LOG2) << log2_culine);
            split_struct->cup[1] = cup + cup_w;
            split_struct->cup[2] = cup + cup_h;
            split_struct->cup[3] = split_struct->cup[1] + cup_h;
            split_struct->cud[0] = cud + 2;
            split_struct->cud[1] = cud + 2;
            split_struct->cud[2] = cud + 2;
            split_struct->cud[3] = cud + 2;
        }
        break;

        default:
        {
            if(xeve_split_is_vertical(split_mode))
            {
                for(i = 0; i < split_struct->part_count; ++i)
                {
                    split_struct->width[i] = xeve_split_get_part_size(split_mode, i, cuw);
                    split_struct->log_cuw[i] = xeve_split_get_part_size_idx(split_mode, i, log_cuw);
                    split_struct->height[i] = cuh;
                    split_struct->log_cuh[i] = log_cuh;
                    if(i)
                    {
                        split_struct->x_pos[i] = split_struct->x_pos[i - 1] + split_struct->width[i - 1];
                        split_struct->y_pos[i] = split_struct->y_pos[i - 1];
                        split_struct->cup[i] = split_struct->cup[i - 1] + (split_struct->width[i - 1] >> MIN_CU_LOG2);
                    }
                }
            }
            else
            {
                for(i = 0; i < split_struct->part_count; ++i)
                {
                    split_struct->width[i] = cuw;
                    split_struct->log_cuw[i] = log_cuw;
                    split_struct->height[i] = xeve_split_get_part_size(split_mode, i, cuh);
                    split_struct->log_cuh[i] = xeve_split_get_part_size_idx(split_mode, i, log_cuh);
                    if(i)
                    {
                        split_struct->y_pos[i] = split_struct->y_pos[i - 1] + split_struct->height[i - 1];
                        split_struct->x_pos[i] = split_struct->x_pos[i - 1];
                        split_struct->cup[i] = split_struct->cup[i - 1] + ((split_struct->height[i - 1] >> MIN_CU_LOG2) << log2_culine);
                    }
                }
            }
            switch(split_mode)
            {
                case SPLIT_BI_VER:
                    split_struct->cud[0] = cud + 1;
                    split_struct->cud[1] = cud + 1;
                    break;
                case SPLIT_BI_HOR:
                    split_struct->cud[0] = cud + 1;
                    split_struct->cud[1] = cud + 1;
                    break;
                default:
                    // Triple tree case
                    split_struct->cud[0] = cud + 2;
                    split_struct->cud[1] = cud + 1;
                    split_struct->cud[2] = cud + 2;
                    break;
            }
        }
        break;
    }
}

void xeve_split_get_suco_order(int suco_flag, SPLIT_MODE mode, int suco_order[SPLIT_MAX_PART_COUNT])
{
    int i, i2;
    if(suco_flag)
    {
        // Reverse order of partitions
        switch(mode)
        {
            case SPLIT_QUAD:
                suco_order[0] = 1;
                suco_order[1] = 0;
                suco_order[2] = 3;
                suco_order[3] = 2;
                break;
            default:
                i2 = 0;
                for(i = xeve_split_part_count(mode); i > 0; --i)
                {
                    suco_order[i2++] = i - 1;
                }
        }
    }
    else
    {
        // Direct order of partitions
        for(i = 0; i < xeve_split_part_count(mode); ++i)
        {
            suco_order[i] = i;
        }
    }
}

int xeve_get_suco_flag(s8* suco_flag, int cud, int cup, int cuw, int cuh, int lcu_s, s8(*suco_flag_buf)[NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU])
{
    int ret = XEVE_OK;
    int pos = cup + (((cuh >> 1) >> MIN_CU_LOG2) * (lcu_s >> MIN_CU_LOG2) + ((cuw >> 1) >> MIN_CU_LOG2));
    int shape = SQUARE + (CONV_LOG2(cuw) - CONV_LOG2(cuh));
    *suco_flag = suco_flag_buf[cud][shape][pos];
    return ret;
}

void xeve_set_suco_flag(s8  suco_flag, int cud, int cup, int cuw, int cuh, int lcu_s, s8(*suco_flag_buf)[NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU])
{
    int pos = cup + (((cuh >> 1) >> MIN_CU_LOG2) * (lcu_s >> MIN_CU_LOG2) + ((cuw >> 1) >> MIN_CU_LOG2));
    int shape = SQUARE + (CONV_LOG2(cuw) - CONV_LOG2(cuh));
    suco_flag_buf[cud][shape][pos] = suco_flag;
}

u8 xeve_check_suco_cond(int cuw, int cuh, s8 split_mode, int boundary, u8 log2_max_cuwh, u8 suco_max_depth, u8 suco_depth)
{
    int suco_log2_maxsize = min((log2_max_cuwh - suco_max_depth), 6);
    int suco_log2_minsize = max((suco_log2_maxsize - suco_depth), max(4, MIN_CU_LOG2));
    if (XEVE_MIN(cuw, cuh) < (1 << suco_log2_minsize) || XEVE_MAX(cuw, cuh) > (1 << suco_log2_maxsize))
    {
        return 0;
    }

    if(boundary)
    {
        return 0;
    }

    if(split_mode==NO_SPLIT || split_mode==SPLIT_BI_HOR || split_mode==SPLIT_TRI_HOR)
    {
        return 0;
    }

    if(split_mode != SPLIT_QUAD && cuw <= cuh)
    {
        return 0;
    }

    return 1;
}


void xeve_mv_rounding_s32( s32 hor, int ver, s32 * rounded_hor, s32 * rounded_ver, s32 right_shift, int left_shift )
{
    int offset = (right_shift > 0) ? (1 << (right_shift - 1)) : 0;
    *rounded_hor = ((hor + offset - (hor >= 0)) >> right_shift) << left_shift;
    *rounded_ver = ((ver + offset - (ver >= 0)) >> right_shift) << left_shift;
}

void xeve_rounding_s32(s32 comp, s32 *rounded_comp, int right_shift, int left_shift)
{
  int offset = (right_shift > 0) ? (1 << (right_shift - 1)) : 0;
  *rounded_comp = ((comp + offset - (comp >= 0)) >> right_shift) << left_shift;
}

void derive_affine_subblock_size_bi( s16 ac_mv[REFP_NUM][VER_NUM][MV_D], s8 refi[REFP_NUM], int cuw, int cuh, int *sub_w, int *sub_h, int vertex_num, BOOL* mem_band_conditions_for_eif_are_satisfied)
{
    int w = cuw;
    int h = cuh;
    int mc_prec_add = 2;
    int mv_wx, mv_wy;
    int l = 0;

    *sub_w = cuw;
    *sub_h = cuh;

    for ( l = 0; l < REFP_NUM; l++ )
    {
        if ( REFI_IS_VALID( refi[l] ) )
        {
            int dmv_hor_x, dmv_ver_x, dmv_hor_y, dmv_ver_y;

            // convert to 2^(storeBit + bit) precision
            dmv_hor_x = ((ac_mv[l][1][MV_X] - ac_mv[l][0][MV_X]) << 7) >> xeve_tbl_log2[cuw];     // deltaMvHor
            dmv_hor_y = ((ac_mv[l][1][MV_Y] - ac_mv[l][0][MV_Y]) << 7) >> xeve_tbl_log2[cuw];
            if (vertex_num == 3)
            {
              dmv_ver_x = ((ac_mv[l][2][MV_X] - ac_mv[l][0][MV_X]) << 7) >> xeve_tbl_log2[cuh]; // deltaMvVer
              dmv_ver_y = ((ac_mv[l][2][MV_Y] - ac_mv[l][0][MV_Y]) << 7) >> xeve_tbl_log2[cuh];
            }
            else
            {
              dmv_ver_x = -dmv_hor_y;                                                    // deltaMvVer
              dmv_ver_y = dmv_hor_x;
            }

            mv_wx = max(abs(dmv_hor_x), abs(dmv_hor_y)), mv_wy = max(abs(dmv_ver_x), abs(dmv_ver_y));
            int sub_lut[4] = { 32, 16, 8, 8 };
            if (mv_wx > 4)
            {
              w = 4;
            }
            else if (mv_wx == 0)
            {
              w = cuw;
            }
            else
            {
              w = sub_lut[mv_wx - 1];
            }

            if (mv_wy > 4)
            {
              h = 4;
            }
            else if (mv_wy == 0)
            {
              h = cuh;
            }
            else
            {
              h = sub_lut[mv_wy - 1];
            }

            *sub_w = min( *sub_w, w );
            *sub_h = min( *sub_h, h );
        }
    }

    int apply_eif = check_eif_applicability_bi( ac_mv, refi, cuw, cuh, vertex_num, mem_band_conditions_for_eif_are_satisfied);

    if ( !apply_eif )
    {
      *sub_w = max( *sub_w, AFFINE_ADAPT_EIF_SIZE );
      *sub_h = max( *sub_h, AFFINE_ADAPT_EIF_SIZE );
    }
}

void derive_affine_subblock_size( s16 ac_mv[VER_NUM][MV_D], int cuw, int cuh, int *sub_w, int *sub_h, int vertex_num, BOOL* mem_band_conditions_for_eif_are_satisfied)
{
    int w = cuw;
    int h = cuh;
    int mc_prec_add = 2;
    int mv_wx, mv_wy;
    int dmv_hor_x, dmv_ver_x, dmv_hor_y, dmv_ver_y;

    // convert to 2^(storeBit + bit) precision
    dmv_hor_x = ((ac_mv[1][MV_X] - ac_mv[0][MV_X]) << 7) >> xeve_tbl_log2[cuw];     // deltaMvHor
    dmv_hor_y = ((ac_mv[1][MV_Y] - ac_mv[0][MV_Y]) << 7) >> xeve_tbl_log2[cuw];
    if (vertex_num == 3)
    {
      dmv_ver_x = ((ac_mv[2][MV_X] - ac_mv[0][MV_X]) << 7) >> xeve_tbl_log2[cuh]; // deltaMvVer
      dmv_ver_y = ((ac_mv[2][MV_Y] - ac_mv[0][MV_Y]) << 7) >> xeve_tbl_log2[cuh];
    }
    else
    {
      dmv_ver_x = -dmv_hor_y;                                                    // deltaMvVer
      dmv_ver_y = dmv_hor_x;
    }

    mv_wx = max(abs(dmv_hor_x), abs(dmv_hor_y)), mv_wy = max(abs(dmv_ver_x), abs(dmv_ver_y));
    int sub_lut[4] = { 32, 16, 8, 8 };
    if (mv_wx > 4)
    {
      w = 4;
    }
    else if (mv_wx == 0)
    {
      w = cuw;
    }
    else
    {
      w = sub_lut[mv_wx - 1];
    }

    if (mv_wy > 4)
    {
      h = 4;
    }
    else if (mv_wy == 0)
    {
      h = cuh;
    }
    else
    {
      h = sub_lut[mv_wy - 1];
    }

    *sub_w = w;
    *sub_h = h;

    int apply_eif = check_eif_applicability_uni( ac_mv, cuw, cuh, vertex_num, mem_band_conditions_for_eif_are_satisfied);

    if ( !apply_eif )
    {
      *sub_w = max( *sub_w, AFFINE_ADAPT_EIF_SIZE );
      *sub_h = max( *sub_h, AFFINE_ADAPT_EIF_SIZE );
    }
}

void calculate_affine_motion_model_parameters( s16 ac_mv[VER_NUM][MV_D], int cuw, int cuh, int vertex_num, int d_hor[MV_D], int d_ver[MV_D], int mv_additional_precision )
{
  assert( MV_X == 0 && MV_Y == 1 );
  assert( vertex_num == 2 || vertex_num == 3 );

  // convert to 2^(storeBit + bit) precision

  for ( int comp = MV_X; comp < MV_D; ++comp )
    d_hor[comp] = ( ( ac_mv[1][comp] - ac_mv[0][comp] ) << mv_additional_precision ) >> xeve_tbl_log2[cuw];

  for ( int comp = MV_X; comp < MV_D; ++comp )
  {
    if ( vertex_num == 3 )
      d_ver[comp] = ( ( ac_mv[2][comp] - ac_mv[0][comp] ) << mv_additional_precision ) >> xeve_tbl_log2[cuh]; // deltaMvVer
    else
      d_ver[comp] = comp == MV_X ? -d_hor[1 - comp] : d_hor[1 - comp];
  }
}


void calculate_bounding_box_size( int w, int h, s16 ac_mv[VER_NUM][MV_D], int d_hor[MV_D], int d_ver[MV_D], int mv_precision, int *b_box_w, int *b_box_h )
{
  int corners[MV_D][4] = { 0, };

  corners[MV_X][0] = 0;
  corners[MV_X][1] = corners[MV_X][0] + ( w + 1 ) * ( d_hor[MV_X] + ( 1 << mv_precision) ) ;
  corners[MV_X][2] = corners[MV_X][0] + ( h + 1 ) * d_ver[MV_X];
  corners[MV_X][3] = corners[MV_X][1] + corners[MV_X][2] - corners[MV_X][0];

  corners[MV_Y][0] = 0;
  corners[MV_Y][1] = corners[MV_Y][0] + ( w + 1 ) * d_hor[MV_Y];
  corners[MV_Y][2] = corners[MV_Y][0] + ( h + 1 ) * ( d_ver[MV_Y] + ( 1 << mv_precision ) );
  corners[MV_Y][3] = corners[MV_Y][1] + corners[MV_Y][2] - corners[MV_Y][0];

  int max[MV_D] = { 0, }, min[MV_D] = { 0, }, diff[MV_D] = { 0, };

  for ( int coord = MV_X; coord < MV_D; ++coord )
  {
    max[coord] = max( max( corners[coord][0], corners[coord][1] ), max( corners[coord][2], corners[coord][3] ) );

    min[coord] = min( min( corners[coord][0], corners[coord][1] ), min( corners[coord][2], corners[coord][3] ) );

    diff[coord] = ( max[coord] - min[coord] + ( 1 << mv_precision ) - 1 ) >> mv_precision; //ceil
  }

  *b_box_w = diff[MV_X] + 1 + 1;
  *b_box_h = diff[MV_Y] + 1 + 1;
}

BOOL check_eif_num_fetched_lines_restrictions( s16 ac_mv[VER_NUM][MV_D], int d_hor[MV_D], int d_ver[MV_D], int mv_precision )
{
  if ( d_ver[MV_Y] < -1 << mv_precision )
    return FALSE;

  if( ( max( 0, d_ver[MV_Y] ) + abs( d_hor[MV_Y] ) ) * ( 1 + EIF_SUBBLOCK_SIZE ) > ( EIF_NUM_ALLOWED_FETCHED_LINES_FOR_THE_FIRST_LINE - 2 ) << mv_precision )
    return FALSE;

  return TRUE;
}

BOOL check_eif_applicability_uni( s16 ac_mv[VER_NUM][MV_D], int cuw, int cuh, int vertex_num, BOOL* mem_band_conditions_are_satisfied)
{
  assert( mem_band_conditions_are_satisfied );

  int mv_additional_precision = MAX_CU_LOG2;
  int mv_precision = 2 + mv_additional_precision;

  int d_hor[MV_D] = { 0, 0 }, d_ver[MV_D] = { 0, 0 };

  calculate_affine_motion_model_parameters( ac_mv, cuw, cuh, vertex_num, d_hor, d_ver, mv_additional_precision );

  *mem_band_conditions_are_satisfied = FALSE;

  int bounding_box_w = 0, bounding_box_h = 0;
  calculate_bounding_box_size( EIF_SUBBLOCK_SIZE, EIF_SUBBLOCK_SIZE, ac_mv, d_hor, d_ver, mv_precision, &bounding_box_w, &bounding_box_h );

  *mem_band_conditions_are_satisfied = bounding_box_w * bounding_box_h <= MAX_MEMORY_ACCESS_BI;

  if (!check_eif_num_fetched_lines_restrictions(ac_mv, d_hor, d_ver, mv_precision))
  {
      return FALSE;
  }

  return TRUE;
}

BOOL check_eif_applicability_bi(s16 ac_mv[REFP_NUM][VER_NUM][MV_D], s8 refi[REFP_NUM], int cuw, int cuh, int vertex_num, BOOL* mem_band_conditions_are_satisfied)
{
    if (mem_band_conditions_are_satisfied)
    {
        *mem_band_conditions_are_satisfied = TRUE;
    }

    int mv_additional_precision = MAX_CU_LOG2;
    int mv_precision = 2 + mv_additional_precision;

    for (int lidx = 0; lidx <= PRED_L1; lidx++)
    {
        if (REFI_IS_VALID(refi[lidx]))
        {
            BOOL mem_band_conditions_are_satisfied_lx = FALSE;
            BOOL is_eif_applicable = check_eif_applicability_uni(ac_mv[lidx], cuw, cuh, vertex_num, &mem_band_conditions_are_satisfied_lx);

            if (mem_band_conditions_are_satisfied)
                *mem_band_conditions_are_satisfied &= mem_band_conditions_are_satisfied_lx;

            if (!is_eif_applicable)
                return FALSE;
        }
    }

    return TRUE;
}

/*******************************************/
/* Neighbor location: Graphical indication */
/*                                         */
/*          B2 UP___________B1 B0          */
/*          LE|               |            */
/*            |               |            */
/*            |               |            */
/*            |      cu    cuh|            */
/*            |               |            */
/*            |               |            */
/*          A1|_____cuw_______|            */
/*          A0                             */
/*                                         */
/*******************************************/

#define SAME_MV(MV0, MV1) ((MV0[MV_X] == MV1[MV_X]) && (MV0[MV_Y] == MV1[MV_Y]))
#define SAME_MVF(refi0, vx0, vy0, refi1, vx1, vy1)   ((refi0 == refi1) && (vx0 == vx1) && (vy0 == vy1))

int xeve_derive_affine_constructed_candidate(int poc, XEVE_REFP (*refp)[REFP_NUM], int cuw, int cuh, int cp_valid[VER_NUM], s16 cp_mv[REFP_NUM][VER_NUM][MV_D], int cp_refi[REFP_NUM][VER_NUM], int cp_idx[VER_NUM], int model_idx, int ver_num, s16 mrg_list_cp_mv[AFF_MAX_CAND][REFP_NUM][VER_NUM][MV_D], s8 mrg_list_refi[AFF_MAX_CAND][REFP_NUM], int *mrg_idx, int mrg_list_cp_num[AFF_MAX_CAND])
{
    int lidx, i;
    int valid_model[2] = {0, 0};
    s32 cpmv_tmp[REFP_NUM][VER_NUM][MV_D];
    int tmp_hor, tmp_ver;
    int shiftHtoW = 7 + xeve_tbl_log2[cuw] - xeve_tbl_log2[cuh]; // x * cuWidth / cuHeight

    // early terminate
    if(*mrg_idx >= AFF_MAX_CAND)
    {
        return 0;
    }

    // check valid model and decide ref idx
    if(ver_num == 2)
    {
        int idx0 = cp_idx[0], idx1 = cp_idx[1];

        if(!cp_valid[idx0] || !cp_valid[idx1])
        {
            return 0;
        }

        for(lidx = 0; lidx < REFP_NUM; lidx++)
        {
            if(REFI_IS_VALID(cp_refi[lidx][idx0]) && REFI_IS_VALID(cp_refi[lidx][idx1]) && cp_refi[lidx][idx0] == cp_refi[lidx][idx1])
            {
                valid_model[lidx] = 1;
            }
        }
    }
    else if(ver_num == 3)
    {
        int idx0 = cp_idx[0], idx1 = cp_idx[1], idx2 = cp_idx[2];

        if(!cp_valid[idx0] || !cp_valid[idx1] || !cp_valid[idx2])
        {
            return 0;
        }

        for(lidx = 0; lidx < REFP_NUM; lidx++)
        {
            if(REFI_IS_VALID(cp_refi[lidx][idx0]) && REFI_IS_VALID(cp_refi[lidx][idx1]) && REFI_IS_VALID(cp_refi[lidx][idx2]) && cp_refi[lidx][idx0] == cp_refi[lidx][idx1] && cp_refi[lidx][idx0] == cp_refi[lidx][idx2])
            {
                valid_model[lidx] = 1;
            }
        }
    }
    else
    {
        xeve_assert( 0 );
    }

    // set merge index and vertex num for valid model
    if(valid_model[0] || valid_model[1])
    {
        mrg_list_cp_num[*mrg_idx] = ver_num;
    }
    else
    {
        return 0;
    }
    
    for(lidx = 0; lidx < REFP_NUM; lidx++)
    {
        if(valid_model[lidx])
        {
            mrg_list_refi[*mrg_idx][lidx] = cp_refi[lidx][cp_idx[0]];
            for ( i = 0; i < ver_num; i++ )
            {
                cpmv_tmp[lidx][cp_idx[i]][MV_X] = (s32)cp_mv[lidx][cp_idx[i]][MV_X];
                cpmv_tmp[lidx][cp_idx[i]][MV_Y] = (s32)cp_mv[lidx][cp_idx[i]][MV_Y];
            }

            // convert to LT, RT[, [LB], [RB]]
            switch(model_idx)
            {
            case 0: // 0 : LT, RT, LB
                break;
            case 1: // 1 : LT, RT, RB
                cpmv_tmp[lidx][2][MV_X] = cpmv_tmp[lidx][3][MV_X] + cpmv_tmp[lidx][0][MV_X] - cpmv_tmp[lidx][1][MV_X];
                cpmv_tmp[lidx][2][MV_Y] = cpmv_tmp[lidx][3][MV_Y] + cpmv_tmp[lidx][0][MV_Y] - cpmv_tmp[lidx][1][MV_Y];
                break;
            case 2: // 1 : LT, LB, RB
                cpmv_tmp[lidx][1][MV_X] = cpmv_tmp[lidx][3][MV_X] + cpmv_tmp[lidx][0][MV_X] - cpmv_tmp[lidx][2][MV_X];
                cpmv_tmp[lidx][1][MV_Y] = cpmv_tmp[lidx][3][MV_Y] + cpmv_tmp[lidx][0][MV_Y] - cpmv_tmp[lidx][2][MV_Y];
                break;
            case 3: // 4 : RT, LB, RB
                cpmv_tmp[lidx][0][MV_X] = cpmv_tmp[lidx][1][MV_X] + cpmv_tmp[lidx][2][MV_X] - cpmv_tmp[lidx][3][MV_X];
                cpmv_tmp[lidx][0][MV_Y] = cpmv_tmp[lidx][1][MV_Y] + cpmv_tmp[lidx][2][MV_Y] - cpmv_tmp[lidx][3][MV_Y];
                break;
            case 4: // 5 : LT, RT
                break;
            case 5: // 6 : LT, LB
                tmp_hor = +((cpmv_tmp[lidx][2][MV_Y] - cpmv_tmp[lidx][0][MV_Y]) << shiftHtoW) + (cpmv_tmp[lidx][0][MV_X] << 7);
                tmp_ver = -((cpmv_tmp[lidx][2][MV_X] - cpmv_tmp[lidx][0][MV_X]) << shiftHtoW) + (cpmv_tmp[lidx][0][MV_Y] << 7);
                xeve_mv_rounding_s32( tmp_hor, tmp_ver, &cpmv_tmp[lidx][1][MV_X], &cpmv_tmp[lidx][1][MV_Y], 7, 0 );
                break;
            default:
                xeve_assert( 0 );
            }

            for ( i = 0; i < ver_num; i++ )
            {
                mrg_list_cp_mv[*mrg_idx][lidx][i][MV_X] = (s16)XEVE_CLIP3( XEVE_INT16_MIN, XEVE_INT16_MAX, cpmv_tmp[lidx][i][MV_X] );
                mrg_list_cp_mv[*mrg_idx][lidx][i][MV_Y] = (s16)XEVE_CLIP3( XEVE_INT16_MIN, XEVE_INT16_MAX, cpmv_tmp[lidx][i][MV_Y] );
            }
        }
        else
        {
            mrg_list_refi[*mrg_idx][lidx] = -1;
            for (i = 0; i < ver_num; i++)
            {
                mrg_list_cp_mv[*mrg_idx][lidx][i][MV_X] = 0;
                mrg_list_cp_mv[*mrg_idx][lidx][i][MV_Y] = 0;
            }
        }
    }

    (*mrg_idx)++;

    return 1;
}

void xeve_derive_affine_model_mv(int scup, int scun, int lidx, s16(*map_mv)[REFP_NUM][MV_D], int cuw, int cuh, int w_scu, int h_scu, s16 mvp[VER_NUM][MV_D]
                               , u32 *map_affine, int cur_cp_num, int log2_max_cuwh, u32 *map_scu, s16(*map_unrefined_mv)[REFP_NUM][MV_D])
{
    s16 neb_mv[VER_NUM][MV_D] = {{0, }, };
    int i;
    int neb_addr[VER_NUM];
    int neb_log_w = MCU_GET_AFF_LOGW(map_affine[scun]);
    int neb_log_h = MCU_GET_AFF_LOGH(map_affine[scun]);
    int neb_w = 1 << neb_log_w;
    int neb_h = 1 << neb_log_h;
    int neb_x, neb_y;
    int cur_x, cur_y;
    int max_bit = 7;
    int diff_w = max_bit - neb_log_w;
    int diff_h = max_bit - neb_log_h;
    int dmv_hor_x, dmv_hor_y, dmv_ver_x, dmv_ver_y, hor_base, ver_base;
    s32 tmp_hor, tmp_ver;
    int neb_cp_num = (MCU_GET_AFF( map_scu[scun] ) == 1) ? 2 : 3;

    neb_addr[0] = scun - MCU_GET_AFF_XOFF(map_affine[scun]) - w_scu * MCU_GET_AFF_YOFF(map_affine[scun]);
    neb_addr[1] = neb_addr[0] + ((neb_w >> MIN_CU_LOG2) - 1);
    neb_addr[2] = neb_addr[0] + ((neb_h >> MIN_CU_LOG2) - 1) * w_scu;
    neb_addr[3] = neb_addr[2] + ((neb_w >> MIN_CU_LOG2) - 1);

    neb_x = (neb_addr[0] % w_scu) << MIN_CU_LOG2;
    neb_y = (neb_addr[0] / w_scu) << MIN_CU_LOG2;
    cur_x = (scup % w_scu) << MIN_CU_LOG2;
    cur_y = (scup / w_scu) << MIN_CU_LOG2;

    for ( i = 0; i < VER_NUM; i++ )
    {
        if (MCU_GET_DMVRF(map_scu[neb_addr[i]]) )
        {
            neb_mv[i][MV_X] = map_unrefined_mv[neb_addr[i]][lidx][MV_X];
            neb_mv[i][MV_Y] = map_unrefined_mv[neb_addr[i]][lidx][MV_Y];
        }
        else
        {
            neb_mv[i][MV_X] = map_mv[neb_addr[i]][lidx][MV_X];
            neb_mv[i][MV_Y] = map_mv[neb_addr[i]][lidx][MV_Y];
        }
    }

    int is_top_ctu_boundary = FALSE;
    if ( (neb_y + neb_h) % (1 << log2_max_cuwh) == 0 && (neb_y + neb_h) == cur_y )
    {
        is_top_ctu_boundary = TRUE;
        neb_y += neb_h;

        neb_mv[0][MV_X] = neb_mv[2][MV_X];
        neb_mv[0][MV_Y] = neb_mv[2][MV_Y];
        neb_mv[1][MV_X] = neb_mv[3][MV_X];
        neb_mv[1][MV_Y] = neb_mv[3][MV_Y];
    }

    dmv_hor_x = (neb_mv[1][MV_X] - neb_mv[0][MV_X]) << diff_w;    // deltaMvHor
    dmv_hor_y = (neb_mv[1][MV_Y] - neb_mv[0][MV_Y]) << diff_w;

    if (cur_cp_num == 3 && !is_top_ctu_boundary )
    {
        dmv_ver_x = (neb_mv[2][MV_X] - neb_mv[0][MV_X]) << diff_h;  // deltaMvVer
        dmv_ver_y = (neb_mv[2][MV_Y] - neb_mv[0][MV_Y]) << diff_h;
    }
    else
    {
        dmv_ver_x = -dmv_hor_y;                                      // deltaMvVer
        dmv_ver_y = dmv_hor_x;
    }
    hor_base = neb_mv[0][MV_X] << max_bit;
    ver_base = neb_mv[0][MV_Y] << max_bit;

    // derive CPMV 0
    tmp_hor = dmv_hor_x * (cur_x - neb_x) + dmv_ver_x * (cur_y - neb_y) + hor_base;
    tmp_ver = dmv_hor_y * (cur_x - neb_x) + dmv_ver_y * (cur_y - neb_y) + ver_base;
    xeve_mv_rounding_s32( tmp_hor, tmp_ver, &tmp_hor, &tmp_ver, max_bit, 0 );
    mvp[0][MV_X] = (s16)XEVE_CLIP3( XEVE_INT16_MIN, XEVE_INT16_MAX, tmp_hor );
    mvp[0][MV_Y] = (s16)XEVE_CLIP3( XEVE_INT16_MIN, XEVE_INT16_MAX, tmp_ver );

    // derive CPMV 1
    tmp_hor = dmv_hor_x * (cur_x - neb_x + cuw) + dmv_ver_x * (cur_y - neb_y) + hor_base;
    tmp_ver = dmv_hor_y * (cur_x - neb_x + cuw) + dmv_ver_y * (cur_y - neb_y) + ver_base;
    xeve_mv_rounding_s32( tmp_hor, tmp_ver, &tmp_hor, &tmp_ver, max_bit, 0 );
    mvp[1][MV_X] = (s16)XEVE_CLIP3( XEVE_INT16_MIN, XEVE_INT16_MAX, tmp_hor );
    mvp[1][MV_Y] = (s16)XEVE_CLIP3( XEVE_INT16_MIN, XEVE_INT16_MAX, tmp_ver );

    // derive CPMV 2
    if ( cur_cp_num == 3 )
    {
        tmp_hor = dmv_hor_x * (cur_x - neb_x) + dmv_ver_x * (cur_y - neb_y + cuh) + hor_base;
        tmp_ver = dmv_hor_y * (cur_x - neb_x) + dmv_ver_y * (cur_y - neb_y + cuh) + ver_base;
        xeve_mv_rounding_s32( tmp_hor, tmp_ver, &tmp_hor, &tmp_ver, max_bit, 0 );
        mvp[2][MV_X] = (s16)XEVE_CLIP3( XEVE_INT16_MIN, XEVE_INT16_MAX, tmp_hor );
        mvp[2][MV_Y] = (s16)XEVE_CLIP3( XEVE_INT16_MIN, XEVE_INT16_MAX, tmp_ver );
    }
}

/* inter affine mode */
void xeve_get_affine_motion_scaling(int poc, int scup, int lidx, s8 cur_refi, int num_refp, s16(*map_mv)[REFP_NUM][MV_D], s8(*map_refi)[REFP_NUM]
                                  , XEVE_REFP(*refp)[REFP_NUM], int cuw, int cuh, int w_scu, int h_scu, u16 avail, s16 mvp[MAX_NUM_MVP][VER_NUM][MV_D]
                                  , s8 refi[MAX_NUM_MVP], u32* map_scu, u32* map_affine, int vertex_num, u16 avail_lr, int log2_max_cuwh
                                  , s16(*map_unrefined_mv)[REFP_NUM][MV_D], u8* map_tidx)
{
    int x_scu = scup % w_scu;
    int y_scu = scup / w_scu;
    int scuw = cuw >> MIN_CU_LOG2;
    int scuh = cuh >> MIN_CU_LOG2;
    int cnt_lt = 0, cnt_rt = 0, cnt_lb = 0;
    int i, j, k;
    s16 mvp_tmp[VER_NUM][MV_D];
    int neb_addr[3];
    int valid_flag[3];
    int cnt_tmp = 0;
    s16 mvp_cand_lt[AFFINE_MAX_NUM_LT][MV_D];
    int neb_addr_lt[AFFINE_MAX_NUM_LT];
    int valid_flag_lt[AFFINE_MAX_NUM_LT];
    s16 mvp_cand_rt[AFFINE_MAX_NUM_RT][MV_D];
    int neb_addr_rt[AFFINE_MAX_NUM_RT];
    int valid_flag_rt[AFFINE_MAX_NUM_RT];
    s16 mvp_cand_lb[AFFINE_MAX_NUM_LB][MV_D];
    int neb_addr_lb[AFFINE_MAX_NUM_LB];
    int valid_flag_lb[AFFINE_MAX_NUM_LB];
    int cnt_rb = 0;
    s16 mvp_cand_rb[AFFINE_MAX_NUM_RB][MV_D];
    int neb_addr_rb[AFFINE_MAX_NUM_RB];
    int valid_flag_rb[AFFINE_MAX_NUM_RB];
    //-------------------  INIT  -------------------//
    for(i = 0; i < ORG_MAX_NUM_MVP; i++)
    {
        for(j = 0; j < VER_NUM; j++)
        {
            mvp[i][j][MV_X] = 0;
            mvp[i][j][MV_Y] = 0;
        }
        refi[i] = 0;
    }

    //-------------------  Model based affine MVP  -------------------//
    
    // left inherited affine MVP, first of {A0, A1}
    neb_addr[0] = scup + w_scu * scuh - 1;       // A0
    neb_addr[1] = scup + w_scu * (scuh - 1) - 1; // A1
    valid_flag[0] = x_scu > 0 && y_scu + scuh < h_scu && MCU_GET_COD(map_scu[neb_addr[0]]) && !MCU_GET_IF(map_scu[neb_addr[0]]) && MCU_GET_AFF(map_scu[neb_addr[0]]) &&
                    (map_tidx[scup] == map_tidx[neb_addr[0]]);
    valid_flag[1] = x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[1]]) && !MCU_GET_IF(map_scu[neb_addr[1]]) && MCU_GET_AFF(map_scu[neb_addr[1]]) &&
                    (map_tidx[scup] == map_tidx[neb_addr[1]]);

    for(k = 0; k < 2; k++)
    {
        if(valid_flag[k] && REFI_IS_VALID(map_refi[neb_addr[k]][lidx])
           && map_refi[neb_addr[k]][lidx] == cur_refi)
        {
            refi[cnt_tmp] = map_refi[neb_addr[k]][lidx];
            xeve_derive_affine_model_mv(scup, neb_addr[k], lidx, map_mv, cuw, cuh, w_scu, h_scu, mvp_tmp, map_affine, vertex_num, log2_max_cuwh, map_scu, map_unrefined_mv);
            mvp[cnt_tmp][0][MV_X] = mvp_tmp[0][MV_X];
            mvp[cnt_tmp][0][MV_Y] = mvp_tmp[0][MV_Y];
            mvp[cnt_tmp][1][MV_X] = mvp_tmp[1][MV_X];
            mvp[cnt_tmp][1][MV_Y] = mvp_tmp[1][MV_Y];
            mvp[cnt_tmp][2][MV_X] = mvp_tmp[2][MV_X];
            mvp[cnt_tmp][2][MV_Y] = mvp_tmp[2][MV_Y];
            cnt_tmp++;
            break;
        }
    }
    if(cnt_tmp >= AFF_MAX_NUM_MVP)
    {
        return;
    }

    // above inherited affine MVP, first of {B0, B1, B2}
    neb_addr[0] = scup - w_scu + scuw;           // B0
    neb_addr[1] = scup - w_scu + scuw - 1;       // B1
    neb_addr[2] = scup - w_scu - 1;              // B2
    valid_flag[0] = y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[0]]) && !MCU_GET_IF(map_scu[neb_addr[0]]) && MCU_GET_AFF(map_scu[neb_addr[0]]) &&
                    (map_tidx[scup] == map_tidx[neb_addr[0]]);
    valid_flag[1] = y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[1]]) && !MCU_GET_IF(map_scu[neb_addr[1]]) && MCU_GET_AFF(map_scu[neb_addr[1]]) &&
                    (map_tidx[scup] == map_tidx[neb_addr[1]]);
    valid_flag[2] = x_scu > 0 && y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[2]]) && !MCU_GET_IF(map_scu[neb_addr[2]]) && MCU_GET_AFF(map_scu[neb_addr[2]]) &&
                    (map_tidx[scup] == map_tidx[neb_addr[2]]);
    for(k = 0; k < 3; k++)
    {
        if(valid_flag[k] && REFI_IS_VALID(map_refi[neb_addr[k]][lidx])
           && map_refi[neb_addr[k]][lidx] == cur_refi)
        {
            refi[cnt_tmp] = map_refi[neb_addr[k]][lidx];
            xeve_derive_affine_model_mv(scup, neb_addr[k], lidx, map_mv, cuw, cuh, w_scu, h_scu, mvp_tmp, map_affine, vertex_num, log2_max_cuwh, map_scu, map_unrefined_mv);
            mvp[cnt_tmp][0][MV_X] = mvp_tmp[0][MV_X];
            mvp[cnt_tmp][0][MV_Y] = mvp_tmp[0][MV_Y];
            mvp[cnt_tmp][1][MV_X] = mvp_tmp[1][MV_X];
            mvp[cnt_tmp][1][MV_Y] = mvp_tmp[1][MV_Y];
            mvp[cnt_tmp][2][MV_X] = mvp_tmp[2][MV_X];
            mvp[cnt_tmp][2][MV_Y] = mvp_tmp[2][MV_Y];
            cnt_tmp++;
            break;
        }
    }
    if(cnt_tmp >= AFF_MAX_NUM_MVP)
    {
        return;
    }

    // right inherited affine MVP, first of {C0, C1}
    neb_addr[0] = scup + w_scu * scuh + scuw;       // C0
    neb_addr[1] = scup + w_scu * (scuh - 1) + scuw; // C1
    valid_flag[0] = x_scu + scuw < w_scu && y_scu + scuh < h_scu && MCU_GET_COD(map_scu[neb_addr[0]]) && !MCU_GET_IF(map_scu[neb_addr[0]]) && MCU_GET_AFF(map_scu[neb_addr[0]]) &&
                    (map_tidx[scup] == map_tidx[neb_addr[0]]);
    valid_flag[1] = x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[1]]) && !MCU_GET_IF(map_scu[neb_addr[1]]) && MCU_GET_AFF(map_scu[neb_addr[1]]) &&
                    (map_tidx[scup] == map_tidx[neb_addr[1]]);

    for (k = 0; k < 2; k++)
    {
        if (valid_flag[k] && REFI_IS_VALID(map_refi[neb_addr[k]][lidx])
            && map_refi[neb_addr[k]][lidx] == cur_refi)
        {
            refi[cnt_tmp] = map_refi[neb_addr[k]][lidx];
            xeve_derive_affine_model_mv(scup, neb_addr[k], lidx, map_mv, cuw, cuh, w_scu, h_scu, mvp_tmp, map_affine, vertex_num, log2_max_cuwh, map_scu, map_unrefined_mv);
            mvp[cnt_tmp][0][MV_X] = mvp_tmp[0][MV_X];
            mvp[cnt_tmp][0][MV_Y] = mvp_tmp[0][MV_Y];
            mvp[cnt_tmp][1][MV_X] = mvp_tmp[1][MV_X];
            mvp[cnt_tmp][1][MV_Y] = mvp_tmp[1][MV_Y];
            mvp[cnt_tmp][2][MV_X] = mvp_tmp[2][MV_X];
            mvp[cnt_tmp][2][MV_Y] = mvp_tmp[2][MV_Y];
            cnt_tmp++;
            break;
        }
    }
    if (cnt_tmp >= AFF_MAX_NUM_MVP)
    {
        return;
    }

    //-------------------  LT  -------------------//
    for(i = 0; i < AFFINE_MAX_NUM_LT; i++)
    {
        mvp_cand_lt[i][MV_X] = 0;
        mvp_cand_lt[i][MV_Y] = 0;
    }

    neb_addr_lt[0] = scup - w_scu - 1;
    neb_addr_lt[1] = scup - w_scu;
    neb_addr_lt[2] = scup - 1;

    valid_flag_lt[0] = x_scu > 0 && y_scu > 0 && MCU_GET_COD(map_scu[neb_addr_lt[0]]) && !MCU_GET_IF(map_scu[neb_addr_lt[0]]) && !MCU_GET_IBC(map_scu[neb_addr_lt[0]]) &&
                       (map_tidx[scup] == map_tidx[neb_addr_lt[0]]);
    valid_flag_lt[1] = y_scu > 0 && MCU_GET_COD(map_scu[neb_addr_lt[1]]) && !MCU_GET_IF(map_scu[neb_addr_lt[1]]) && !MCU_GET_IBC(map_scu[neb_addr_lt[1]]) &&
                       (map_tidx[scup] == map_tidx[neb_addr_lt[1]]);
    valid_flag_lt[2] = x_scu > 0 && MCU_GET_COD(map_scu[neb_addr_lt[2]]) && !MCU_GET_IF(map_scu[neb_addr_lt[2]]) && !MCU_GET_IBC(map_scu[neb_addr_lt[2]]) &&
                       (map_tidx[scup] == map_tidx[neb_addr_lt[2]]);

    for(k = 0; k < AFFINE_MAX_NUM_LT; k++)
    {
        if(valid_flag_lt[k] && REFI_IS_VALID(map_refi[neb_addr_lt[k]][lidx]))
        {
            refi[cnt_lt] = map_refi[neb_addr_lt[k]][lidx];
            if(refi[cnt_lt] == cur_refi)
            {
                if(MCU_GET_DMVRF(map_scu[neb_addr_lt[k]]))
                {
                    mvp_cand_lt[cnt_lt][MV_X] = map_unrefined_mv[neb_addr_lt[k]][lidx][MV_X];
                    mvp_cand_lt[cnt_lt][MV_Y] = map_unrefined_mv[neb_addr_lt[k]][lidx][MV_Y];
                }
                else
                {
                    mvp_cand_lt[cnt_lt][MV_X] = map_mv[neb_addr_lt[k]][lidx][MV_X];
                    mvp_cand_lt[cnt_lt][MV_Y] = map_mv[neb_addr_lt[k]][lidx][MV_Y];
                }
                cnt_lt++;
                break;
            }
        }
    }

    //-------------------  RT  -------------------//
    for(i = 0; i < AFFINE_MAX_NUM_RT; i++)
    {
        mvp_cand_rt[i][MV_X] = 0;
        mvp_cand_rt[i][MV_Y] = 0;
    }

    neb_addr_rt[0] = scup - w_scu + scuw;
    neb_addr_rt[1] = scup - w_scu + scuw - 1;
    neb_addr_rt[2] = scup + scuw;

    valid_flag_rt[0] = y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr_rt[0]]) && !MCU_GET_IF(map_scu[neb_addr_rt[0]]) && !MCU_GET_IBC(map_scu[neb_addr_rt[0]]) &&
                       (map_tidx[scup] == map_tidx[neb_addr_rt[0]]);
    valid_flag_rt[1] = y_scu > 0 && MCU_GET_COD(map_scu[neb_addr_rt[1]]) && !MCU_GET_IF(map_scu[neb_addr_rt[1]]) && !MCU_GET_IBC(map_scu[neb_addr_rt[1]]) &&
                       (map_tidx[scup] == map_tidx[neb_addr_rt[1]]);            
    valid_flag_rt[2] = x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr_rt[2]]) && !MCU_GET_IF(map_scu[neb_addr_rt[2]]) && !MCU_GET_IBC(map_scu[neb_addr_rt[2]]) &&
                       (map_tidx[scup] == map_tidx[neb_addr_rt[2]]);

    for(k = 0; k < AFFINE_MAX_NUM_RT; k++)
    {
        if(valid_flag_rt[k] && REFI_IS_VALID(map_refi[neb_addr_rt[k]][lidx]))
        {
            refi[cnt_rt] = map_refi[neb_addr_rt[k]][lidx];
            if(refi[cnt_rt] == cur_refi)
            {
                if(MCU_GET_DMVRF(map_scu[neb_addr_rt[k]]))
                {
                    mvp_cand_rt[cnt_rt][MV_X] = map_unrefined_mv[neb_addr_rt[k]][lidx][MV_X];
                    mvp_cand_rt[cnt_rt][MV_Y] = map_unrefined_mv[neb_addr_rt[k]][lidx][MV_Y];
                }
                else
                {
                    mvp_cand_rt[cnt_rt][MV_X] = map_mv[neb_addr_rt[k]][lidx][MV_X];
                    mvp_cand_rt[cnt_rt][MV_Y] = map_mv[neb_addr_rt[k]][lidx][MV_Y];
                }
                cnt_rt++;
                break;
            }
        }
    }

    //-------------------  LB  -------------------//
    for(i = 0; i < AFFINE_MAX_NUM_LB; i++)
    {
        mvp_cand_lb[i][MV_X] = 0;
        mvp_cand_lb[i][MV_Y] = 0;
    }

    neb_addr_lb[0] = scup + w_scu * scuh - 1;        // A0
    neb_addr_lb[1] = scup + w_scu * (scuh - 1) - 1;  // A1

    valid_flag_lb[0] = x_scu > 0 && y_scu + scuh < h_scu && MCU_GET_COD(map_scu[neb_addr_lb[0]]) && !MCU_GET_IF(map_scu[neb_addr_lb[0]]) && !MCU_GET_IBC(map_scu[neb_addr_lb[0]]) &&
                       (map_tidx[scup] == map_tidx[neb_addr_lb[0]]);
    valid_flag_lb[1] = x_scu > 0 && MCU_GET_COD(map_scu[neb_addr_lb[1]]) && !MCU_GET_IF(map_scu[neb_addr_lb[1]]) && !MCU_GET_IBC(map_scu[neb_addr_lb[1]]) &&
                       (map_tidx[scup] == map_tidx[neb_addr_lb[1]]);

    for(k = 0; k < AFFINE_MAX_NUM_LB; k++)
    {
        if(valid_flag_lb[k] && REFI_IS_VALID(map_refi[neb_addr_lb[k]][lidx]))
        {
            refi[cnt_lb] = map_refi[neb_addr_lb[k]][lidx];
            if(refi[cnt_lb] == cur_refi)
            {
                if (MCU_GET_DMVRF(map_scu[neb_addr_lb[k]]))
                {
                    mvp_cand_lb[cnt_lb][MV_X] = map_unrefined_mv[neb_addr_lb[k]][lidx][MV_X];
                    mvp_cand_lb[cnt_lb][MV_Y] = map_unrefined_mv[neb_addr_lb[k]][lidx][MV_Y];
                }
                else
                {
                    mvp_cand_lb[cnt_lb][MV_X] = map_mv[neb_addr_lb[k]][lidx][MV_X];
                    mvp_cand_lb[cnt_lb][MV_Y] = map_mv[neb_addr_lb[k]][lidx][MV_Y];
                }
                cnt_lb++;
                break;
            }
        }
    }

    //-------------------  RB  -------------------//
    for (i = 0; i < AFFINE_MAX_NUM_RB; i++)
    {
        mvp_cand_rb[i][MV_X] = 0;
        mvp_cand_rb[i][MV_Y] = 0;
    }

    neb_addr_rb[0] = scup + w_scu * scuh + scuw;
    neb_addr_rb[1] = scup + w_scu * (scuh - 1) + scuw;

    valid_flag_rb[0] = x_scu + scuw < w_scu && y_scu + scuh < h_scu && MCU_GET_COD(map_scu[neb_addr_rb[0]]) && !MCU_GET_IF(map_scu[neb_addr_rb[0]]) && !MCU_GET_IBC(map_scu[neb_addr_rb[0]]) &&
                       (map_tidx[scup] == map_tidx[neb_addr_rb[0]]);
    valid_flag_rb[1] = x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr_rb[1]]) && !MCU_GET_IF(map_scu[neb_addr_rb[1]]) && !MCU_GET_IBC(map_scu[neb_addr_rb[1]]) &&
                       (map_tidx[scup] == map_tidx[neb_addr_rb[1]]);

    for (k = 0; k < AFFINE_MAX_NUM_RB; k++)
    {
        if (valid_flag_rb[k] && REFI_IS_VALID(map_refi[neb_addr_rb[k]][lidx]))
        {
            refi[cnt_rb] = map_refi[neb_addr_rb[k]][lidx];
            if (refi[cnt_rb] == cur_refi)
            {
              if (MCU_GET_DMVRF(map_scu[neb_addr_rb[k]]))
              {
                mvp_cand_rb[cnt_rb][MV_X] = map_unrefined_mv[neb_addr_rb[k]][lidx][MV_X];
                mvp_cand_rb[cnt_rb][MV_Y] = map_unrefined_mv[neb_addr_rb[k]][lidx][MV_Y];
              }
              else
              {
                mvp_cand_rb[cnt_rb][MV_X] = map_mv[neb_addr_rb[k]][lidx][MV_X];
                mvp_cand_rb[cnt_rb][MV_Y] = map_mv[neb_addr_rb[k]][lidx][MV_Y];
              }
                cnt_rb++;
                break;
            }
        }
    }

    //-------------------  organize  -------------------//
    {
        if (cnt_lt && cnt_rt && (vertex_num == 2 || (cnt_lb || cnt_rb)))
        {
            mvp[cnt_tmp][0][MV_X] = mvp_cand_lt[0][MV_X];
            mvp[cnt_tmp][0][MV_Y] = mvp_cand_lt[0][MV_Y];
            mvp[cnt_tmp][1][MV_X] = mvp_cand_rt[0][MV_X];
            mvp[cnt_tmp][1][MV_Y] = mvp_cand_rt[0][MV_Y];
            mvp[cnt_tmp][2][MV_X] = mvp_cand_lb[0][MV_X];
            mvp[cnt_tmp][2][MV_Y] = mvp_cand_lb[0][MV_Y];

            if (cnt_lb == 0 && cnt_rb > 0)
            {
                mvp[cnt_tmp][2][MV_X] = (s16)XEVE_CLIP3(XEVE_INT16_MIN, XEVE_INT16_MAX, mvp_cand_rb[0][MV_X] + mvp_cand_lt[0][MV_X] - mvp_cand_rt[0][MV_X]);
                mvp[cnt_tmp][2][MV_Y] = (s16)XEVE_CLIP3(XEVE_INT16_MIN, XEVE_INT16_MAX, mvp_cand_rb[0][MV_Y] + mvp_cand_lt[0][MV_Y] - mvp_cand_rt[0][MV_Y]);
            }
            cnt_tmp++;
        }
        if(cnt_tmp == AFF_MAX_NUM_MVP)
        {
            return;
        }

        // Add translation mv, left
        if(cnt_lb)
        {
            mvp[cnt_tmp][0][MV_X] = mvp_cand_lb[0][MV_X];
            mvp[cnt_tmp][0][MV_Y] = mvp_cand_lb[0][MV_Y];
            mvp[cnt_tmp][1][MV_X] = mvp_cand_lb[0][MV_X];
            mvp[cnt_tmp][1][MV_Y] = mvp_cand_lb[0][MV_Y];
            mvp[cnt_tmp][2][MV_X] = mvp_cand_lb[0][MV_X];
            mvp[cnt_tmp][2][MV_Y] = mvp_cand_lb[0][MV_Y];
            cnt_tmp++;
        }

        // Add translation mv, right
        else if (cnt_rb)
        {
            mvp[cnt_tmp][0][MV_X] = mvp_cand_rb[0][MV_X];
            mvp[cnt_tmp][0][MV_Y] = mvp_cand_rb[0][MV_Y];
            mvp[cnt_tmp][1][MV_X] = mvp_cand_rb[0][MV_X];
            mvp[cnt_tmp][1][MV_Y] = mvp_cand_rb[0][MV_Y];
            mvp[cnt_tmp][2][MV_X] = mvp_cand_rb[0][MV_X];
            mvp[cnt_tmp][2][MV_Y] = mvp_cand_rb[0][MV_Y];
            cnt_tmp++;
        }

        if(cnt_tmp == AFF_MAX_NUM_MVP)
        {
            return;
        }

        // Add translation mv, above
        if(cnt_rt)
        {
            mvp[cnt_tmp][0][MV_X] = mvp_cand_rt[0][MV_X];
            mvp[cnt_tmp][0][MV_Y] = mvp_cand_rt[0][MV_Y];
            mvp[cnt_tmp][1][MV_X] = mvp_cand_rt[0][MV_X];
            mvp[cnt_tmp][1][MV_Y] = mvp_cand_rt[0][MV_Y];
            mvp[cnt_tmp][2][MV_X] = mvp_cand_rt[0][MV_X];
            mvp[cnt_tmp][2][MV_Y] = mvp_cand_rt[0][MV_Y];
            cnt_tmp++;
        }
        if(cnt_tmp == AFF_MAX_NUM_MVP)
        {
            return;
        }

        // Add translation mv, above left
        if(cnt_lt)
        {
            mvp[cnt_tmp][0][MV_X] = mvp_cand_lt[0][MV_X];
            mvp[cnt_tmp][0][MV_Y] = mvp_cand_lt[0][MV_Y];
            mvp[cnt_tmp][1][MV_X] = mvp_cand_lt[0][MV_X];
            mvp[cnt_tmp][1][MV_Y] = mvp_cand_lt[0][MV_Y];
            mvp[cnt_tmp][2][MV_X] = mvp_cand_lt[0][MV_X];
            mvp[cnt_tmp][2][MV_Y] = mvp_cand_lt[0][MV_Y];
            cnt_tmp++;
        }
        if(cnt_tmp == AFF_MAX_NUM_MVP)
        {
            return;
        }

        // Add zero MVP
        for(; cnt_tmp < AFF_MAX_NUM_MVP; cnt_tmp++)
        {
            mvp[cnt_tmp][0][MV_X] = 0;
            mvp[cnt_tmp][0][MV_Y] = 0;
            mvp[cnt_tmp][1][MV_X] = 0;
            mvp[cnt_tmp][1][MV_Y] = 0;
            mvp[cnt_tmp][2][MV_X] = 0;
            mvp[cnt_tmp][2][MV_Y] = 0;
            cnt_tmp++;
        }
    }
}

/* merge affine mode */
int xeve_get_affine_merge_candidate(int poc, int slice_type, int scup, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], XEVE_REFP(*refp)[REFP_NUM]
                                  , int cuw, int cuh, int w_scu, int h_scu, u16 avail, s8 mrg_list_refi[AFF_MAX_CAND][REFP_NUM], s16 mrg_list_cpmv[AFF_MAX_CAND][REFP_NUM][VER_NUM][MV_D]
                                  , int mrg_list_cp_num[AFF_MAX_CAND], u32* map_scu, u32* map_affine, int log2_max_cuwh
                                  , s16(*map_unrefined_mv)[REFP_NUM][MV_D], u16 avail_lr, XEVE_SH * sh, u8* map_tidx)
{
    int lidx, i, k;
    int x_scu = scup % w_scu;
    int y_scu = scup / w_scu;
    int scuw = cuw >> MIN_CU_LOG2;
    int scuh = cuh >> MIN_CU_LOG2;
    int cnt = 0;
    s16 tmvp[REFP_NUM][MV_D];
    s8  available_pred_idx = 0;
    //-------------------  Model based affine MVP  -------------------//
    {
        int neb_addr[5];
        int valid_flag[5];
        int top_left[7];

        if (avail_lr == LR_01)
        {
            neb_addr[0] = scup + w_scu * (scuh - 1) + scuw; // A1
            neb_addr[1] = scup - w_scu;                     // B1
            neb_addr[2] = scup - w_scu - 1;                 // B0
            neb_addr[3] = scup + w_scu * scuh + scuw;       // A0
            neb_addr[4] = scup - w_scu + scuw;              // B2

            valid_flag[0] = x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[0]]) && !MCU_GET_IF(map_scu[neb_addr[0]]) && MCU_GET_AFF(map_scu[neb_addr[0]]);
            valid_flag[1] = y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[1]]) && !MCU_GET_IF(map_scu[neb_addr[1]]) && MCU_GET_AFF(map_scu[neb_addr[1]]);
            valid_flag[2] = x_scu > 0 && y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[2]]) && !MCU_GET_IF(map_scu[neb_addr[2]]) && MCU_GET_AFF(map_scu[neb_addr[2]]);
            valid_flag[3] = x_scu + scuw < w_scu && y_scu + scuh < h_scu && MCU_GET_COD(map_scu[neb_addr[3]]) && !MCU_GET_IF(map_scu[neb_addr[3]]) && MCU_GET_AFF(map_scu[neb_addr[3]]);
            valid_flag[4] = y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[4]]) && !MCU_GET_IF(map_scu[neb_addr[4]]) && MCU_GET_AFF(map_scu[neb_addr[4]]);
        }
        else
        {
            neb_addr[0] = scup + w_scu * (scuh - 1) - 1; // A1
            neb_addr[1] = scup - w_scu + scuw - 1;       // B1
            neb_addr[2] = scup - w_scu + scuw;           // B0
            neb_addr[3] = scup + w_scu * scuh - 1;       // A0
            neb_addr[4] = scup - w_scu - 1;              // B2

            valid_flag[0] = x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[0]]) && !MCU_GET_IF(map_scu[neb_addr[0]]) && MCU_GET_AFF(map_scu[neb_addr[0]]);
            valid_flag[1] = y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[1]]) && !MCU_GET_IF(map_scu[neb_addr[1]]) && MCU_GET_AFF(map_scu[neb_addr[1]]);
            valid_flag[2] = y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[2]]) && !MCU_GET_IF(map_scu[neb_addr[2]]) && MCU_GET_AFF(map_scu[neb_addr[2]]);
            valid_flag[3] = x_scu > 0 && y_scu + scuh < h_scu && MCU_GET_COD(map_scu[neb_addr[3]]) && !MCU_GET_IF(map_scu[neb_addr[3]]) && MCU_GET_AFF(map_scu[neb_addr[3]]);
            valid_flag[4] = x_scu > 0 && y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[4]]) && !MCU_GET_IF(map_scu[neb_addr[4]]) && MCU_GET_AFF(map_scu[neb_addr[4]]);
        }

        valid_flag[0] = valid_flag[0] && (map_tidx[scup] == map_tidx[neb_addr[0]]);
        valid_flag[1] = valid_flag[1] && (map_tidx[scup] == map_tidx[neb_addr[1]]);
        valid_flag[2] = valid_flag[2] && (map_tidx[scup] == map_tidx[neb_addr[2]]);
        valid_flag[3] = valid_flag[3] && (map_tidx[scup] == map_tidx[neb_addr[3]]);
        valid_flag[4] = valid_flag[4] && (map_tidx[scup] == map_tidx[neb_addr[4]]);

        for(k = 0; k < 5; k++)
        {
            if(valid_flag[k])
            {
                top_left[k] = neb_addr[k] - MCU_GET_AFF_XOFF( map_affine[neb_addr[k]] ) - w_scu * MCU_GET_AFF_YOFF( map_affine[neb_addr[k]] );
            }
        }

        if ( valid_flag[2] && valid_flag[1] && top_left[1] == top_left[2] )
        {
            valid_flag[2] = 0;
        }

        if(valid_flag[3] && valid_flag[0] && top_left[0] == top_left[3])
        {
            valid_flag[3] = 0;
        }

        if((valid_flag[4] && valid_flag[0] && top_left[4] == top_left[0]) || (valid_flag[4] && valid_flag[1] && top_left[4] == top_left[1]))
        {
            valid_flag[4] = 0;
        }

        for(k = 0; k < 5; k++)
        {
            if(valid_flag[k])
            {
                // set vertex number: affine flag == 1, set to 2 vertex, otherwise, set to 3 vertex
                mrg_list_cp_num[cnt] = (MCU_GET_AFF(map_scu[neb_addr[k]]) == 1) ? 2 : 3;

                for(lidx = 0; lidx < REFP_NUM; lidx++)
                {
                    if(REFI_IS_VALID(map_refi[neb_addr[k]][lidx]))
                    {
                        mrg_list_refi[cnt][lidx] = map_refi[neb_addr[k]][lidx];
                        xeve_derive_affine_model_mv(scup, neb_addr[k], lidx, map_mv, cuw, cuh, w_scu, h_scu, mrg_list_cpmv[cnt][lidx], map_affine
                                                  , mrg_list_cp_num[cnt], log2_max_cuwh, map_scu, map_unrefined_mv);
                    }
                    else // set to default value
                    {
                        mrg_list_refi[cnt][lidx] = -1;
                        for(i = 0; i < VER_NUM; i++)
                        {
                            mrg_list_cpmv[cnt][lidx][i][MV_X] = 0;
                            mrg_list_cpmv[cnt][lidx][i][MV_Y] = 0;
                        }
                    }
                }
                cnt++;
            }

            if(cnt >= AFF_MODEL_CAND) // one candidate in current stage
            {
                break;
            }
        }
    }

    //-------------------  control point based affine MVP  -------------------//
    {
        s16 cp_mv[REFP_NUM][VER_NUM][MV_D];
        int cp_refi[REFP_NUM][VER_NUM];
        int cp_valid[VER_NUM];

        int neb_addr_lt[AFFINE_MAX_NUM_LT];
        int neb_addr_rt[AFFINE_MAX_NUM_RT];
        int neb_addr_lb[AFFINE_MAX_NUM_LB];
        int neb_addr_rb[AFFINE_MAX_NUM_RB];

        int valid_flag_lt[AFFINE_MAX_NUM_LT];
        int valid_flag_rt[AFFINE_MAX_NUM_RT];
        int valid_flag_lb[AFFINE_MAX_NUM_LB];
        int valid_flag_rb[AFFINE_MAX_NUM_RB];

        //------------------  INIT  ------------------//
        for(i = 0; i < VER_NUM; i++)
        {
            for(lidx = 0; lidx < REFP_NUM; lidx++)
            {
                cp_mv[lidx][i][MV_X] = 0;
                cp_mv[lidx][i][MV_Y] = 0;
                cp_refi[lidx][i] = -1;
            }
            cp_valid[i] = 0;
        }

        //-------------------  LT  -------------------//
        neb_addr_lt[0] = scup - w_scu - 1;
        neb_addr_lt[1] = scup - w_scu;
        neb_addr_lt[2] = scup - 1;

        valid_flag_lt[0] = x_scu > 0 && y_scu > 0 && MCU_GET_COD(map_scu[neb_addr_lt[0]]) && !MCU_GET_IF(map_scu[neb_addr_lt[0]]) && !MCU_GET_IBC(map_scu[neb_addr_lt[0]]);
        valid_flag_lt[1] = y_scu > 0 && MCU_GET_COD(map_scu[neb_addr_lt[1]]) && !MCU_GET_IF(map_scu[neb_addr_lt[1]]) && !MCU_GET_IBC(map_scu[neb_addr_lt[1]]);
        valid_flag_lt[2] = x_scu > 0 && MCU_GET_COD(map_scu[neb_addr_lt[2]]) && !MCU_GET_IF(map_scu[neb_addr_lt[2]]) && !MCU_GET_IBC(map_scu[neb_addr_lt[2]]);
        
        valid_flag_lt[0] = valid_flag_lt[0] && (map_tidx[scup] == map_tidx[neb_addr_lt[0]]);
        valid_flag_lt[1] = valid_flag_lt[1] && (map_tidx[scup] == map_tidx[neb_addr_lt[1]]);
        valid_flag_lt[2] = valid_flag_lt[2] && (map_tidx[scup] == map_tidx[neb_addr_lt[2]]);

        for (k = 0; k < AFFINE_MAX_NUM_LT; k++)
        {
            if (valid_flag_lt[k])
            {
                for (lidx = 0; lidx < REFP_NUM; lidx++)
                {
                    cp_refi[lidx][0] = map_refi[neb_addr_lt[k]][lidx];
                    if (MCU_GET_DMVRF(map_scu[neb_addr_lt[k]]))
                    {
                        cp_mv[lidx][0][MV_X] = map_unrefined_mv[neb_addr_lt[k]][lidx][MV_X];
                        cp_mv[lidx][0][MV_Y] = map_unrefined_mv[neb_addr_lt[k]][lidx][MV_Y];
                    }
                    else
                    {
                        cp_mv[lidx][0][MV_X] = map_mv[neb_addr_lt[k]][lidx][MV_X];
                        cp_mv[lidx][0][MV_Y] = map_mv[neb_addr_lt[k]][lidx][MV_Y];
                    }
                }
                cp_valid[0] = 1;
                break;
            }
        }

        //-------------------  RT  -------------------//
        neb_addr_rt[0] = scup - w_scu + scuw;
        neb_addr_rt[1] = scup - w_scu + scuw - 1;

        valid_flag_rt[0] = y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr_rt[0]]) && !MCU_GET_IF(map_scu[neb_addr_rt[0]]) && !MCU_GET_IBC(map_scu[neb_addr_rt[0]]);
        valid_flag_rt[1] = y_scu > 0 && MCU_GET_COD(map_scu[neb_addr_rt[1]]) && !MCU_GET_IF(map_scu[neb_addr_rt[1]]) && !MCU_GET_IBC(map_scu[neb_addr_rt[1]]);

        neb_addr_rt[2] = scup + scuw;                 // RIGHT
        valid_flag_rt[2] = x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr_rt[2]]) && !MCU_GET_IF(map_scu[neb_addr_rt[2]]) && !MCU_GET_IBC(map_scu[neb_addr_rt[2]]);

        valid_flag_rt[0] = valid_flag_rt[0] && (map_tidx[scup] == map_tidx[neb_addr_rt[0]]);
        valid_flag_rt[1] = valid_flag_rt[1] && (map_tidx[scup] == map_tidx[neb_addr_rt[1]]);
        valid_flag_rt[2] = valid_flag_rt[2] && (map_tidx[scup] == map_tidx[neb_addr_rt[2]]);

        for (k = 0; k < AFFINE_MAX_NUM_RT; k++)
        {
            if (valid_flag_rt[k])
            {
                for (lidx = 0; lidx < REFP_NUM; lidx++)
                {
                    cp_refi[lidx][1] = map_refi[neb_addr_rt[k]][lidx];
                    if (MCU_GET_DMVRF(map_scu[neb_addr_rt[k]]))
                    {
                        cp_mv[lidx][1][MV_X] = map_unrefined_mv[neb_addr_rt[k]][lidx][MV_X];
                        cp_mv[lidx][1][MV_Y] = map_unrefined_mv[neb_addr_rt[k]][lidx][MV_Y];
                    }
                    else
                    {
                        cp_mv[lidx][1][MV_X] = map_mv[neb_addr_rt[k]][lidx][MV_X];
                        cp_mv[lidx][1][MV_Y] = map_mv[neb_addr_rt[k]][lidx][MV_Y];
                    }
                }
                cp_valid[1] = 1;
                break;
            }
        }

        //-------------------  LB  -------------------//
        if (avail_lr == LR_10 || avail_lr == LR_11)
        {
            neb_addr_lb[0] = scup + w_scu * scuh - 1;        // A0
            neb_addr_lb[1] = scup + w_scu * (scuh - 1) - 1;  // A1

            valid_flag_lb[0] = x_scu > 0 && y_scu + scuh < h_scu && MCU_GET_COD(map_scu[neb_addr_lb[0]]) && !MCU_GET_IF(map_scu[neb_addr_lb[0]]) && !MCU_GET_IBC(map_scu[neb_addr_lb[0]]);
            valid_flag_lb[1] = x_scu > 0 && MCU_GET_COD(map_scu[neb_addr_lb[1]]) && !MCU_GET_IF(map_scu[neb_addr_lb[1]]) && !MCU_GET_IBC(map_scu[neb_addr_lb[1]]);

            valid_flag_lb[0] = valid_flag_lb[0] && (map_tidx[scup] == map_tidx[neb_addr_lb[0]]);
            valid_flag_lb[1] = valid_flag_lb[1] && (map_tidx[scup] == map_tidx[neb_addr_lb[1]]);

            for (k = 0; k < AFFINE_MAX_NUM_LB; k++)
            {
                if (valid_flag_lb[k])
                {
                    for (lidx = 0; lidx < REFP_NUM; lidx++)
                    {
                        cp_refi[lidx][2] = map_refi[neb_addr_lb[k]][lidx];
                        if (MCU_GET_DMVRF(map_scu[neb_addr_lb[k]]))
                        {
                            cp_mv[lidx][2][MV_X] = map_unrefined_mv[neb_addr_lb[k]][lidx][MV_X];
                            cp_mv[lidx][2][MV_Y] = map_unrefined_mv[neb_addr_lb[k]][lidx][MV_Y];
                        }
                        else
                        {
                            cp_mv[lidx][2][MV_X] = map_mv[neb_addr_lb[k]][lidx][MV_X];
                            cp_mv[lidx][2][MV_Y] = map_mv[neb_addr_lb[k]][lidx][MV_Y];
                        }
                    }
                    cp_valid[2] = 1;
                    break;
                }
            }
        }
        else
        {
            neb_addr_lb[0] = scup + w_scu * scuh - 1;
            s32 SameCtuRow = ((y_scu + scuh) << MIN_CU_LOG2 >> log2_max_cuwh) == (y_scu << MIN_CU_LOG2 >> log2_max_cuwh);
            valid_flag_lb[0] = x_scu > 0 && (y_scu + scuh < h_scu) && SameCtuRow;


            valid_flag_lb[0] = valid_flag_lb[0] && (map_tidx[scup] == map_tidx[neb_addr_lb[0]]) && (map_tidx[scup] == map_tidx[scup - 1]);
            if (valid_flag_lb[0])
            {
                neb_addr_lb[0] = ((x_scu - 1) >> 1 << 1) + ((y_scu + scuh) >> 1 << 1) * w_scu; // 8x8 grid
                xeve_get_mv_collocated(refp, poc, neb_addr_lb[0], scup, w_scu, h_scu, tmvp, &available_pred_idx, sh);

                if ((available_pred_idx == 1) || (available_pred_idx == 3))
                {
                    cp_refi[REFP_0][2] = 0;
                    cp_mv[REFP_0][2][MV_X] = tmvp[REFP_0][MV_X];
                    cp_mv[REFP_0][2][MV_Y] = tmvp[REFP_0][MV_Y];
                }
                else
                {
                    cp_refi[0][2] = REFI_INVALID;
                    cp_mv[REFP_0][2][MV_X] = 0;
                    cp_mv[REFP_0][2][MV_Y] = 0;
                }
                if (((available_pred_idx == 2) || (available_pred_idx == 3)) && slice_type == SLICE_B)
                {
                    cp_refi[REFP_1][2] = 0;
                    cp_mv[REFP_1][2][MV_X] = tmvp[REFP_1][MV_X];
                    cp_mv[REFP_1][2][MV_Y] = tmvp[REFP_1][MV_Y];
                }
                else
                {
                    cp_refi[REFP_1][2] = REFI_INVALID;
                    cp_mv[REFP_1][2][MV_X] = 0;
                    cp_mv[REFP_1][2][MV_Y] = 0;
                }
            }
            if (REFI_IS_VALID(cp_refi[REFP_0][2]) || REFI_IS_VALID(cp_refi[REFP_1][2]))
            {
                cp_valid[2] = 1;
            }
        }

        //-------------------  RB  -------------------//
        if (avail_lr == LR_01 || avail_lr == LR_11)
        {
            neb_addr_rb[0] = scup + w_scu * scuh + scuw;
            valid_flag_rb[0] = x_scu + scuw < w_scu && y_scu + scuh < h_scu && MCU_GET_COD(map_scu[neb_addr_rb[0]]) && !MCU_GET_IF(map_scu[neb_addr_rb[0]]) && !MCU_GET_IBC(map_scu[neb_addr_rb[0]]);

            neb_addr_rb[1] = scup + w_scu * (scuh - 1) + scuw;
            valid_flag_rb[1] = x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr_rb[1]]) && !MCU_GET_IF(map_scu[neb_addr_rb[1]]) && !MCU_GET_IBC(map_scu[neb_addr_rb[1]]);

            valid_flag_rb[0] = valid_flag_rb[0] && (map_tidx[scup] == map_tidx[neb_addr_rb[0]]);
            valid_flag_rb[1] = valid_flag_rb[1] && (map_tidx[scup] == map_tidx[neb_addr_rb[1]]);

            for (k = 0; k < AFFINE_MAX_NUM_RB; k++)
            {
                if (valid_flag_rb[k])
                {
                    for (lidx = 0; lidx < REFP_NUM; lidx++)
                    {

                        cp_refi[lidx][3] = map_refi[neb_addr_rb[k]][lidx];

                        if (MCU_GET_DMVRF(map_scu[neb_addr_rb[k]]))
                        {
                          cp_mv[lidx][3][MV_X] = map_unrefined_mv[neb_addr_rb[k]][lidx][MV_X];
                          cp_mv[lidx][3][MV_Y] = map_unrefined_mv[neb_addr_rb[k]][lidx][MV_Y];
                        }
                        else
                        {
                          cp_mv[lidx][3][MV_X] = map_mv[neb_addr_rb[k]][lidx][MV_X];
                          cp_mv[lidx][3][MV_Y] = map_mv[neb_addr_rb[k]][lidx][MV_Y];
                        }
                    }
                    break;
                }
            }
        }
        else
        {
            s32 isSameCtuLine = ((y_scu + scuh) << MIN_CU_LOG2 >> log2_max_cuwh) == (y_scu << MIN_CU_LOG2 >> log2_max_cuwh);
            valid_flag_rb[0] = x_scu + scuw < w_scu && y_scu + scuh < h_scu && isSameCtuLine;
            
            neb_addr_rb[0] = ((x_scu + scuw) >> 1 << 1) + ((y_scu + scuh) >> 1 << 1) * w_scu; // 8x8 grid
            valid_flag_rb[0] = valid_flag_rb[0] && (map_tidx[scup] == map_tidx[neb_addr_rb[0]]);

            if (valid_flag_rb[0])
            {
                s16 tmvp[REFP_NUM][MV_D];
                s8 available_pred_idx = 0;

                neb_addr_rb[0] = ((x_scu + scuw) >> 1 << 1) + ((y_scu + scuh) >> 1 << 1) * w_scu; // 8x8 grid
                xeve_get_mv_collocated(refp, poc, neb_addr_rb[0], scup, w_scu, h_scu, tmvp, &available_pred_idx, sh);

                if ((available_pred_idx == 1) || (available_pred_idx == 3))
                {
                    cp_refi[0][3] = 0;
                    cp_mv[0][3][MV_X] = tmvp[REFP_0][MV_X];
                    cp_mv[0][3][MV_Y] = tmvp[REFP_0][MV_Y];
                }
                else
                {
                    cp_refi[0][3] = REFI_INVALID;
                    cp_mv[0][3][MV_X] = 0;
                    cp_mv[0][3][MV_Y] = 0;
                }

                if (((available_pred_idx == 2) || (available_pred_idx == 3)) && slice_type == SLICE_B)
                {
                    cp_refi[1][3] = 0;
                    cp_mv[1][3][MV_X] = tmvp[REFP_1][MV_X];
                    cp_mv[1][3][MV_Y] = tmvp[REFP_1][MV_Y];
                }
                else
                {
                    cp_refi[1][3] = REFI_INVALID;
                    cp_mv[1][3][MV_X] = 0;
                    cp_mv[1][3][MV_Y] = 0;
                }
            }
        }

        if (REFI_IS_VALID(cp_refi[REFP_0][3]) || REFI_IS_VALID(cp_refi[REFP_1][3]))
        {
            cp_valid[3] = 1;
        }

        //-------------------  insert model  -------------------//
        int const_order[6] = { 0, 1, 2, 3, 4, 5 };
        int const_num = 6;

        int idx = 0;
        int const_model[6][VER_NUM] =
        {
            { 0, 1, 2 },          // 0: LT, RT, LB
            { 0, 1, 3 },          // 1: LT, RT, RB
            { 0, 2, 3 },          // 2: LT, LB, RB
            { 1, 2, 3 },          // 3: RT, LB, RB
            { 0, 1 },             // 4: LT, RT
            { 0, 2 },             // 5: LT, LB
        };

        int cp_num[6] = { 3, 3, 3, 3, 2, 2 };
        for ( idx = 0; idx < const_num; idx++ )
        {
            int const_idx = const_order[idx];
            xeve_derive_affine_constructed_candidate( poc, refp, cuw, cuh, cp_valid, cp_mv, cp_refi, const_model[const_idx], const_idx, cp_num[const_idx], mrg_list_cpmv, mrg_list_refi, &cnt, mrg_list_cp_num );
        }
    }

    // Zero padding
    int cnt_wo_padding = cnt;
    {
        int cp_idx;
        for ( ; cnt < AFF_MAX_CAND; cnt++ )
        {
            mrg_list_cp_num[cnt] = 2;
            for ( lidx = 0; lidx < REFP_NUM; lidx++ )
            {
                for ( cp_idx = 0; cp_idx < 2; cp_idx++ )
                {
                    mrg_list_cpmv[cnt][lidx][cp_idx][MV_X] = 0;
                    mrg_list_cpmv[cnt][lidx][cp_idx][MV_Y] = 0;
                }
            }
            mrg_list_refi[cnt][REFP_0] = 0;
            mrg_list_refi[cnt][REFP_1] = (slice_type == SLICE_B) ? 0 : REFI_INVALID;
        }
    }

    return cnt_wo_padding;
}

void xeve_get_ctx_last_pos_xy_para(int ch_type, int width, int height, int *result_offset_x, int *result_offset_y, int *result_shift_x, int *result_shift_y)
{
    int convertedWidth = CONV_LOG2(width) - 2;
    int convertedHeight = CONV_LOG2(height) - 2;
    convertedWidth = (convertedWidth < 0) ? 0 : convertedWidth;
    convertedHeight = (convertedHeight < 0) ? 0 : convertedHeight;

    *result_offset_x = (ch_type != Y_C) ? 0 : ((convertedWidth * 3) + ((convertedWidth + 1) >> 2));
    *result_offset_y = (ch_type != Y_C) ? 0 : ((convertedHeight * 3) + ((convertedHeight + 1) >> 2));
    *result_shift_x = (ch_type != Y_C) ? convertedWidth - CONV_LOG2(width >> 4) : ((convertedWidth + 3) >> 2);
    *result_shift_y = (ch_type != Y_C) ? convertedHeight - CONV_LOG2(height >> 4) : ((convertedHeight + 3) >> 2);

    if (ch_type == Y_C)
    {
        if (convertedWidth >= 4)
        {
            *result_offset_x += ((width >> 6) << 1) + (width >> 7);
            *result_shift_x = 2;
        }
        if (convertedHeight >= 4)
        {
            *result_offset_y += ((height >> 6) << 1) + (height >> 7);
            *result_shift_y = 2;
        }
    }
}

int xeve_get_ctx_sig_coeff_inc(s16 *pcoeff, int blkpos, int width, int height, int ch_type)
{
    const s16 *pdata = pcoeff + blkpos;
    const int width_m1 = width - 1;
    const int height_m1 = height - 1;
    const int log2_w = CONV_LOG2(width);
    const int pos_y = blkpos >> log2_w;
    const int pos_x = blkpos - (pos_y << log2_w);
    int diag = pos_x + pos_y;
    int num_sig_coeff = 0;
    int ctx_idx;
    int ctx_ofs;

    if(pos_x < width_m1)
    {
        num_sig_coeff += pdata[1] != 0;
        if(pos_x < width_m1 - 1)
        {
            num_sig_coeff += pdata[2] != 0;
        }
        if(pos_y < height_m1)
        {
            num_sig_coeff += pdata[width + 1] != 0;
        }
    }

    if(pos_y < height_m1)
    {
        num_sig_coeff += pdata[width] != 0;
        if(pos_y < height_m1 - 1)
        {
            num_sig_coeff += pdata[2 * width] != 0;
        }
    }

    ctx_idx = XEVE_MIN(num_sig_coeff, 4) + 1;

    if(diag < 2)
    {
        ctx_idx = XEVE_MIN(ctx_idx, 2);
    }

    if(ch_type == Y_C)
    {
        ctx_ofs = diag < 2 ? 0 : (diag < 5 ? 2 : 7);
    }
    else
    {
        ctx_ofs = diag < 2 ? 0 : 2;
    }

    return ctx_ofs + ctx_idx;
}

int xeve_get_ctx_gtA_inc(s16 *pcoeff, int blkpos, int width, int height, int ch_type)
{
    const s16 *pdata = pcoeff + blkpos;
    const int width_m1 = width - 1;
    const int height_m1 = height - 1;
    const int log2_w = CONV_LOG2(width);
    const int pos_y = blkpos >> log2_w;
    const int pos_x = blkpos - (pos_y << log2_w);
    int num_gtA = 0;
    int diag = pos_x + pos_y;

    if(pos_x < width_m1)
    {
        num_gtA += XEVE_ABS16(pdata[1]) > 1;
        if(pos_x < width_m1 - 1)
        {
            num_gtA += XEVE_ABS16(pdata[2]) > 1;
        }
        if(pos_y < height_m1)
        {
            num_gtA += XEVE_ABS16(pdata[width + 1]) > 1;
        }
    }

    if(pos_y < height_m1)
    {
        num_gtA += XEVE_ABS16(pdata[width]) > 1;
        if(pos_y < height_m1 - 1)
        {
            num_gtA += XEVE_ABS16(pdata[2 * width]) > 1;
        }
    }

    num_gtA = XEVE_MIN(num_gtA, 3) + 1;
    if(ch_type == Y_C)
    {
        num_gtA += (diag < 3) ? 0 : ((diag < 10) ? 4 : 8);
    }
    return num_gtA;
}

int xeve_get_ctx_gtB_inc(s16 *pcoeff, int blkpos, int width, int height, int ch_type)
{
    const s16 *pdata = pcoeff + blkpos;
    const int width_m1 = width - 1;
    const int height_m1 = height - 1;
    const int log2_w = CONV_LOG2(width);
    const int pos_y = blkpos >> log2_w;
    const int pos_x = blkpos - (pos_y << log2_w);
    int diag = pos_x + pos_y;
    int num_gtB = 0;

    if(pos_x < width_m1)
    {
        num_gtB += XEVE_ABS16(pdata[1]) > 2;
        if(pos_x < width_m1 - 1)
        {
            num_gtB += XEVE_ABS16(pdata[2]) > 2;
        }
        if(pos_y < height_m1)
        {
            num_gtB += XEVE_ABS16(pdata[width + 1]) > 2;
        }
    }

    if(pos_y < height_m1)
    {
        num_gtB += XEVE_ABS16(pdata[width]) > 2;
        if(pos_y < height_m1 - 1)
        {
            num_gtB += XEVE_ABS16(pdata[2 * width]) > 2;
        }
    }

    num_gtB = XEVE_MIN(num_gtB, 3) + 1;
    if(ch_type == Y_C)
    {
        num_gtB += (diag < 3) ? 0 : ((diag < 10) ? 4 : 8);
    }
    return num_gtB;
}

int get_rice_para(s16 *pcoeff, int blkpos, int width, int height, int base_level)
{
    const s16 *pdata = pcoeff + blkpos;
    const int width_m1 = width - 1;
    const int height_m1 = height - 1;
    const int log2_w = CONV_LOG2(width);
    const int pos_y = blkpos >> log2_w;
    const int pos_x = blkpos - (pos_y << log2_w);
    int sum_abs = 0;

    if (pos_x < width_m1)
    {
        sum_abs += XEVE_ABS16(pdata[1]);
        if (pos_x < width_m1 - 1)
        {
            sum_abs += XEVE_ABS16(pdata[2]);
        }
        if (pos_y < height_m1)
        {
            sum_abs += XEVE_ABS16(pdata[width + 1]);
        }
    }

    if (pos_y < height_m1)
    {
        sum_abs += XEVE_ABS16(pdata[width]);
        if (pos_y < height_m1 - 1)
        {
            sum_abs += XEVE_ABS16(pdata[2 * width]);
        }
    }
    sum_abs = XEVE_MAX(XEVE_MIN(sum_abs - 5 * base_level, 31), 0);
    return go_rice_para_coeff[sum_abs];
}


void xeve_eco_sbac_ctx_initialize(SBAC_CTX_MODEL *model, s16 *ctx_init_model, u16 num_ctx, u8 slice_type, u8 slice_qp)
{    
    s32 i, slope, offset;
    u16 mps, state;
    const int qp = XEVE_CLIP3(0, 51, slice_qp);
    const int is_inter_slice = (slice_type == SLICE_B || slice_type == SLICE_P);

    ctx_init_model += (is_inter_slice * num_ctx);

    for(i = 0; i < num_ctx; i++)
    {
        const int init_value = *(ctx_init_model);
        slope = (init_value & 14) << 4;
        slope = (init_value & 1) ? -slope : slope;
        offset = ((init_value >> 4) & 62) << 7;
        offset = ((init_value >> 4) & 1) ? -offset : offset;
        offset += 4096;

        state = XEVE_CLIP3(1, 511, (slope * qp + offset) >> 4);
        if(state > 256)
        {
            state = 512 - state;
            mps = 0;
        }
        else
        {
            mps = 1;
        }
        model[i] = (state << 1) + mps;

        ctx_init_model++;
    }
}

u8 check_ats_inter_info_coded(int cuw, int cuh, int pred_mode, int tool_ats)
{
    int min_size = 8;
    int max_size = 1 << MAX_TR_LOG2;
    u8  mode_hori, mode_vert, mode_hori_quad, mode_vert_quad;
    if (!tool_ats || pred_mode == MODE_INTRA || cuw > max_size || cuh > max_size || pred_mode == MODE_IBC)
    {
        mode_hori = mode_vert = mode_hori_quad = mode_vert_quad = 0;
    }
    else
    {
        //vertical mode
        mode_vert = cuw >= min_size ? 1 : 0;
        mode_vert_quad = cuw >= min_size * 2 ? 1 : 0;
        mode_hori = cuh >= min_size ? 1 : 0;
        mode_hori_quad = cuh >= min_size * 2 ? 1 : 0;
    }
    return (mode_vert << 0) + (mode_hori << 1) + (mode_vert_quad << 2) + (mode_hori_quad << 3);
}


void get_ats_inter_trs(u8 ats_inter_info, int log2_cuw, int log2_cuh, u8* ats_cu, u8* ats_mode)
{
    if (ats_inter_info == 0)
    {
        return;
    }

    if (log2_cuw > 5 || log2_cuh > 5)
    {
        *ats_cu = 0;
        *ats_mode = 0;
    }
    else
    {
        u8 ats_inter_idx = get_ats_inter_idx(ats_inter_info);
        u8 ats_inter_pos = get_ats_inter_pos(ats_inter_info);
        u8 t_idx_h, t_idx_v;

        //Note: 1 is DCT8 and 0 is DST7
        if (is_ats_inter_horizontal(ats_inter_idx))
        {
            t_idx_h = 0;
            t_idx_v = ats_inter_pos == 0 ? 1 : 0;
        }
        else
        {
            t_idx_v = 0;
            t_idx_h = ats_inter_pos == 0 ? 1 : 0;
        }
        *ats_cu = 1;
        *ats_mode = (t_idx_h << 1) | t_idx_v;
    }
}

u8 xeve_check_chroma_split_allowed(int luma_width, int luma_height)
{
    return (luma_width * luma_height) >= (16 * 4)  ? 1 : 0;
}

u8 xeve_is_chroma_split_allowed(int w, int h, SPLIT_MODE split)
{
    switch (split)
    {
    case SPLIT_BI_VER:
        return xeve_check_chroma_split_allowed(w >> 1, h);
    case SPLIT_BI_HOR:
        return xeve_check_chroma_split_allowed(w, h >> 1);
    case SPLIT_TRI_VER:
        return xeve_check_chroma_split_allowed(w >> 2, h);
    case SPLIT_TRI_HOR:
        return xeve_check_chroma_split_allowed(w, h >> 2);
    default:
        xeve_assert(!"This check is for BTT only");
        return 0;
    }
}

enum TQC_RUN xeve_get_run(enum TQC_RUN run_list, TREE_CONS tree_cons)
{
    enum TQC_RUN ans = 0;
    if (xeve_check_luma(tree_cons))
    {
        ans |= run_list & RUN_L;
    }

    if (xeve_check_chroma(tree_cons))
    {
        ans |= run_list & RUN_CB;
        ans |= run_list & RUN_CR;
    }
    return ans;
}

void get_tu_pos_offset(u8 ats_inter_info, int log2_cuw, int log2_cuh, int* x_offset, int* y_offset)
{
    u8 ats_inter_idx = get_ats_inter_idx(ats_inter_info);
    u8 ats_inter_pos = get_ats_inter_pos(ats_inter_info);
    int cuw = 1 << log2_cuw;
    int cuh = 1 << log2_cuh;

    if (ats_inter_idx == 0)
    {
        *x_offset = 0;
        *y_offset = 0;
        return;
    }

    if (is_ats_inter_horizontal(ats_inter_idx))
    {
        *x_offset = 0;
        *y_offset = ats_inter_pos == 0 ? 0 : cuh - (is_ats_inter_quad_size(ats_inter_idx) ? cuh / 4 : cuh / 2);
    }
    else
    {
        *x_offset = ats_inter_pos == 0 ? 0 : cuw - (is_ats_inter_quad_size(ats_inter_idx) ? cuw / 4 : cuw / 2);
        *y_offset = 0;
    }
}

void get_tu_size(u8 ats_inter_info, int log2_cuw, int log2_cuh, int* log2_tuw, int* log2_tuh)
{
    u8 ats_inter_idx = get_ats_inter_idx(ats_inter_info);
    if (ats_inter_idx == 0)
    {
        *log2_tuw = log2_cuw;
        *log2_tuh = log2_cuh;
        return;
    }

    assert(ats_inter_idx <= 4);
    if (is_ats_inter_horizontal(ats_inter_idx))
    {
        *log2_tuw = log2_cuw;
        *log2_tuh = is_ats_inter_quad_size(ats_inter_idx) ? log2_cuh - 2 : log2_cuh - 1;
    }
    else
    {
        *log2_tuw = is_ats_inter_quad_size(ats_inter_idx) ? log2_cuw - 2 : log2_cuw - 1;
        *log2_tuh = log2_cuh;
    }
}

void set_cu_cbf_flags(u8 cbf_y, u8 ats_inter_info, int log2_cuw, int log2_cuh, u32 *map_scu, int w_scu)
{
    u8 ats_inter_idx = get_ats_inter_idx(ats_inter_info);
    u8 ats_inter_pos = get_ats_inter_pos(ats_inter_info);
    int x_offset, y_offset, log2_tuw, log2_tuh;
    int x, y, w, h;
    int w_cus = 1 << (log2_cuw - MIN_CU_LOG2);
    int h_cus = 1 << (log2_cuh - MIN_CU_LOG2);
    u32 *cur_map;
    if (ats_inter_info)
    {
        get_tu_pos_offset(ats_inter_info, log2_cuw, log2_cuh, &x_offset, &y_offset);
        get_tu_size(ats_inter_info, log2_cuw, log2_cuh, &log2_tuw, &log2_tuh);
        x_offset >>= MIN_CU_LOG2;
        y_offset >>= MIN_CU_LOG2;
        w = 1 << (log2_tuw - MIN_CU_LOG2);
        h = 1 << (log2_tuh - MIN_CU_LOG2);

        // Clear CbF of CU
        cur_map = map_scu;
        for (y = 0; y < h_cus; ++y, cur_map += w_scu)
        {
            for (x = 0; x < w_cus; ++x)
            {
                MCU_CLR_CBFL(cur_map[x]);
            }
        }

        if (cbf_y)
        {
            // Set CbF only on coded part
            cur_map = map_scu + y_offset * w_scu + x_offset;
            for (y = 0; y < h; ++y, cur_map += w_scu)
            {
                for (x = 0; x < w; ++x)
                {
                    MCU_SET_CBFL(cur_map[x]);
                }
            }
        }
    }
    else
    {
        assert(0);
    }
}

#if GRAB_STAT
void enc_stat_header(int pic_w, int pic_h)
{
    xeve_stat_write_comment("VTMBMS Block Statistics");
    xeve_stat_write_comment("Sequence size: [%dx%4d]", pic_w, pic_h);
    xeve_stat_write_type("PredMode", "Flag", NULL);
    xeve_stat_write_type("AffineFlag", "Flag", NULL);
    xeve_stat_write_type("MMVDFlag", "Flag", NULL);
    xeve_stat_write_type("MV0", "Vector", "Scale: 4");
    xeve_stat_write_type("REF0", "Flag", NULL);
    xeve_stat_write_type("MV1", "Vector", "Scale: 4");
    xeve_stat_write_type("REF1", "Flag", NULL);
    xeve_stat_write_type("BiBlock", "Flag", NULL);
    xeve_stat_write_type("ats_intra_cu", "Flag", NULL);
    xeve_stat_write_type("ats_inter_info", "Flag", NULL);
    xeve_stat_write_type("CBF_luma", "Flag", NULL);
    xeve_stat_write_type("Tile_ID", "Flag", NULL);
    xeve_stat_write_type("Slice_IDX", "Flag", NULL);
}
#endif
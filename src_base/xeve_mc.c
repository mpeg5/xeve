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

#include "xeve_def.h"
#include <assert.h>

const XEVE_MC_L (*xeve_func_mc_l)[2];
const XEVE_MC_C (*xeve_func_mc_c)[2];
XEVE_AVG_NO_CLIP xeve_func_average_no_clip;

// clang-format off
const s16 xeve_tbl_mc_l_coeff[16][8] =
{
    {  0, 0,   0, 64,  0,   0,  0,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 1,  -5, 52, 20,  -5,  1,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 2, -10, 40, 40, -10,  2,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 1,  -5, 20, 52,  -5,  1,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
    {  0, 0,   0,  0,  0,   0,  0,  0 },
};

const s16 xeve_tbl_mc_c_coeff[32][4] =
{
    {  0, 64,  0,  0 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    { -2, 58, 10, -2 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    { -4, 52, 20, -4 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    { -6, 46, 30, -6 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    { -8, 40, 40, -8 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    { -6, 30, 46, -6 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    { -4, 20, 52, -4 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    { -2, 10, 58, -2 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
    {  0,  0,  0,  0 },
};
// clang-format on

/****************************************************************************
 * motion compensation for luma
 ****************************************************************************/
void xeve_mc_l_00(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth, const s16(*mc_l_coeff)[8])
{
    int size = sizeof(pel) * w;
    gmv_x >>= 4;
    gmv_y >>= 4;
    ref += gmv_y * s_ref + gmv_x;
       
    for (int i = 0; i < h; i++)
    {
        xeve_mcpy(pred, ref, size);
        pred += s_pred;
        ref += s_ref;
    }
}

void xeve_mc_l_n0(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth, const s16(*mc_l_coeff)[8])
{
    int i, j, dx;
    s32 pt;
    dx = gmv_x & 15;
    ref += (gmv_y >> 4) * s_ref + (gmv_x >> 4) - 3;

    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
        {
            pt = MAC_8TAP_N0(mc_l_coeff[dx], ref[j], ref[j + 1], ref[j + 2], ref[j + 3], ref[j + 4], ref[j + 5], ref[j + 6], ref[j + 7]);
            pred[j] = XEVE_CLIP3(0, (1 << bit_depth) - 1, pt);
        }
        ref += s_ref;
        pred += s_pred;
    }
}

void xeve_mc_l_0n(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth, const s16(*mc_l_coeff)[8])
{
    int i, j, dy;
    s32 pt;

    dy = gmv_y & 15;
    ref += ((gmv_y >> 4) - 3) * s_ref + (gmv_x >> 4);

    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
        {
            pt = MAC_8TAP_0N(mc_l_coeff[dy], ref[j], ref[s_ref + j], ref[s_ref * 2 + j], ref[s_ref * 3 + j], ref[s_ref * 4 + j], ref[s_ref * 5 + j], ref[s_ref * 6 + j], ref[s_ref * 7 + j]);
            pred[j] = XEVE_CLIP3(0, (1 << bit_depth) - 1, pt);
        }
        ref += s_ref;
        pred += s_pred;
    }
}

void xeve_mc_l_nn(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth, const s16(*mc_l_coeff)[8])
{
    s16         buf[(MAX_CU_SIZE + MC_IBUF_PAD_L)*(MAX_CU_SIZE + MC_IBUF_PAD_L)];
    s16        *b;
    int         i, j, dx, dy;
    s32         pt;

    dx = gmv_x & 15;
    dy = gmv_y & 15;
    ref += ((gmv_y >> 4) - 3) * s_ref + (gmv_x >> 4) - 3;

    int shift1 = XEVE_MIN(4, bit_depth - 8);
    int shift2 = XEVE_MAX(8, 20 - bit_depth);
    int offset1 = 0;
    int offset2 = (1 << (shift2 - 1));

    b = buf;
    for (i = 0; i < h + 7; i++)
    {
        for (j = 0; j < w; j++)
        {
            b[j] = MAC_8TAP_NN_S1(mc_l_coeff[dx], ref[j], ref[j + 1], ref[j + 2], ref[j + 3], ref[j + 4], ref[j + 5], ref[j + 6], ref[j + 7],offset1, shift1);
        }
        ref += s_ref;
        b += w;
    }

    b = buf;
    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
        {
            pt = MAC_8TAP_NN_S2(mc_l_coeff[dy], b[j], b[j + w], b[j + w * 2], b[j + w * 3], b[j + w * 4], b[j + w * 5], b[j + w * 6], b[j + w * 7], offset2, shift2);
            pred[j] = XEVE_CLIP3(0, (1 << bit_depth) - 1, pt);
        }
        pred += s_pred;
        b += w;
    }
}


/****************************************************************************
 * motion compensation for chroma
 ****************************************************************************/
void xeve_mc_c_00(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth, const s16(*mc_c_coeff)[4])
{
    int size = sizeof(pel) * w;

    gmv_x >>= 5;
    gmv_y >>= 5;
    ref += gmv_y * s_ref + gmv_x;

    for (int i = 0; i < h; i++)
    {
        xeve_mcpy(pred, ref, size);
        pred += s_pred;
        ref += s_ref;
    }
}

void xeve_mc_c_n0(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth, const s16(*mc_c_coeff)[4])
{
    int       i, j, dx;
    s32       pt;

    dx = gmv_x & 31;
    ref += (gmv_y >> 5) * s_ref + (gmv_x >> 5) - 1;

    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
        {
            pt = MAC_4TAP_N0(mc_c_coeff[dx], ref[j], ref[j + 1], ref[j + 2], ref[j + 3]);
            pred[j] = XEVE_CLIP3(0, (1 << bit_depth) - 1, pt);
        }
        pred += s_pred;
        ref += s_ref;
    }
}

void xeve_mc_c_0n(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth, const s16(*mc_c_coeff)[4])
{
    int i, j, dy;
    s32       pt;

    dy = gmv_y & 31;
    ref += ((gmv_y >> 5) - 1) * s_ref + (gmv_x >> 5);

    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
        {
            pt = MAC_4TAP_0N(mc_c_coeff[dy], ref[j], ref[s_ref + j], ref[s_ref * 2 + j], ref[s_ref * 3 + j]);
            pred[j] = XEVE_CLIP3(0, (1 << bit_depth) - 1, pt);
        }
        pred += s_pred;
        ref += s_ref;
    }
}

void xeve_mc_c_nn(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth, const s16(*mc_c_coeff)[4])
{
    s16         buf[(MAX_CU_SIZE + MC_IBUF_PAD_C)*MAX_CU_SIZE];
    s16        *b;
    int         i, j;
    s32         pt;
    int         dx, dy;

    dx = gmv_x & 31;
    dy = gmv_y & 31;
    ref += ((gmv_y >> 5) - 1) * s_ref + (gmv_x >> 5) - 1;

    int shift1 = XEVE_MIN(4, bit_depth - 8);
    int shift2 = XEVE_MAX(8, 20 - bit_depth);
    int offset1 = 0;
    int offset2 = (1 << (shift2 - 1));

    b = buf;
    for (i = 0; i < h + 3; i++)
    {
        for (j = 0; j < w; j++)
        {
            b[j] = MAC_4TAP_NN_S1(mc_c_coeff[dx], ref[j], ref[j + 1], ref[j + 2], ref[j + 3], offset1, shift1);
        }
        ref += s_ref;
        b += w;
    }

    b = buf;
    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
        {
            pt = MAC_4TAP_NN_S2(mc_c_coeff[dy], b[j], b[j + w], b[j + 2 * w], b[j + 3 * w], offset2, shift2);
            pred[j] = XEVE_CLIP3(0, (1 << bit_depth) - 1, pt);
        }
        pred += s_pred;
        b += w;
    }
}

const XEVE_MC_L xeve_tbl_mc_l[2][2] =
{
    {
        xeve_mc_l_00, /* dx == 0 && dy == 0 */
        xeve_mc_l_0n  /* dx == 0 && dy != 0 */
    },
    {
        xeve_mc_l_n0, /* dx != 0 && dy == 0 */
        xeve_mc_l_nn  /* dx != 0 && dy != 0 */
    }
};

const XEVE_MC_C xeve_tbl_mc_c[2][2] =
{
    {
        xeve_mc_c_00, /* dx == 0 && dy == 0 */
        xeve_mc_c_0n  /* dx == 0 && dy != 0 */
    },
    {
        xeve_mc_c_n0, /* dx != 0 && dy == 0 */
        xeve_mc_c_nn  /* dx != 0 && dy != 0 */
    }
};

void xeve_mv_clip(int x, int y, int pic_w, int pic_h, int w, int h, s8 refi[REFP_NUM], s16 mv[REFP_NUM][MV_D], s16(*mv_t)[MV_D])
{
    int min_clip[MV_D], max_clip[MV_D];

    x <<= 2;
    y <<= 2;
    w <<= 2;
    h <<= 2;
    min_clip[MV_X] = -(MAX_CU_SIZE << 2);
    min_clip[MV_Y] = -(MAX_CU_SIZE << 2);
    max_clip[MV_X] = (pic_w - 1 + MAX_CU_SIZE) << 2;
    max_clip[MV_Y] = (pic_h - 1 + MAX_CU_SIZE) << 2;

    mv_t[REFP_0][MV_X] = mv[REFP_0][MV_X];
    mv_t[REFP_0][MV_Y] = mv[REFP_0][MV_Y];
    mv_t[REFP_1][MV_X] = mv[REFP_1][MV_X];
    mv_t[REFP_1][MV_Y] = mv[REFP_1][MV_Y];

    if (REFI_IS_VALID(refi[REFP_0]))
    {
        if (x + mv[REFP_0][MV_X] < min_clip[MV_X]) mv_t[REFP_0][MV_X] = min_clip[MV_X] - x;
        if (y + mv[REFP_0][MV_Y] < min_clip[MV_Y]) mv_t[REFP_0][MV_Y] = min_clip[MV_Y] - y;
        if (x + mv[REFP_0][MV_X] + w - 4 > max_clip[MV_X]) mv_t[REFP_0][MV_X] = max_clip[MV_X] - x - w + 4;
        if (y + mv[REFP_0][MV_Y] + h - 4 > max_clip[MV_Y]) mv_t[REFP_0][MV_Y] = max_clip[MV_Y] - y - h + 4;
    }
    if (REFI_IS_VALID(refi[REFP_1]))
    {
        if (x + mv[REFP_1][MV_X] < min_clip[MV_X]) mv_t[REFP_1][MV_X] = min_clip[MV_X] - x;
        if (y + mv[REFP_1][MV_Y] < min_clip[MV_Y]) mv_t[REFP_1][MV_Y] = min_clip[MV_Y] - y;
        if (x + mv[REFP_1][MV_X] + w - 4 > max_clip[MV_X]) mv_t[REFP_1][MV_X] = max_clip[MV_X] - x - w + 4;
        if (y + mv[REFP_1][MV_Y] + h - 4 > max_clip[MV_Y]) mv_t[REFP_1][MV_Y] = max_clip[MV_Y] - y - h + 4;
    }
}

void xeve_average_16b_no_clip(s16 *src, s16 *ref, s16 *dst, int s_src, int s_ref, int s_dst, int wd, int ht)
{
    pel* s = src;
    pel* r = ref;
    pel* d = dst;

    for (int j = 0; j < ht; j++)
    {
        for (int i = 0; i < wd; i++)
        {
            d[i] = (s[i] + r[i] + 1) >> 1;
        }
        s += s_src;
        r += s_ref;
        d += s_dst;
    }
}

void xeve_mc(int x, int y, int pic_w, int pic_h, int w, int h, s8 refi[REFP_NUM], s16(*mv)[MV_D], XEVE_REFP(*refp)[REFP_NUM], pel pred[REFP_NUM][N_C][MAX_CU_DIM], int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc)
{
    XEVE_PIC   * ref_pic;
    int          qpel_gmv_x, qpel_gmv_y;
    int          bidx = 0;
    s16          mv_t[REFP_NUM][MV_D];
    s16          mv_before_clipping[REFP_NUM][MV_D]; //store it to pass it to interpolation function for deriving correct interpolation filter
    int          w_shift = XEVE_GET_CHROMA_W_SHIFT(chroma_format_idc);
    int          h_shift = XEVE_GET_CHROMA_H_SHIFT(chroma_format_idc);
    int          chroma_w_fac = 2 / (w_shift + 1);
    int          chroma_h_fac = 2 / (h_shift + 1);

    mv_before_clipping[REFP_0][MV_X] = mv[REFP_0][MV_X];
    mv_before_clipping[REFP_0][MV_Y] = mv[REFP_0][MV_Y];
    mv_before_clipping[REFP_1][MV_X] = mv[REFP_1][MV_X];
    mv_before_clipping[REFP_1][MV_Y] = mv[REFP_1][MV_Y];

    xeve_mv_clip(x, y, pic_w, pic_h, w, h, refi, mv, mv_t);

    if(REFI_IS_VALID(refi[REFP_0]))
    {
        /* forward */
        ref_pic = refp[refi[REFP_0]][REFP_0].pic;
        qpel_gmv_x = (x << 2) + mv_t[REFP_0][MV_X];
        qpel_gmv_y = (y << 2) + mv_t[REFP_0][MV_Y];

        xeve_mc_l(mv_before_clipping[REFP_0][MV_X] << 2, mv_before_clipping[REFP_0][MV_Y] << 2, ref_pic->y, (qpel_gmv_x << 2), (qpel_gmv_y << 2), ref_pic->s_l, w, pred[0][Y_C], w, h, bit_depth_luma, xeve_tbl_mc_l_coeff);
        if(chroma_format_idc)
        {
            xeve_mc_c(mv_before_clipping[REFP_0][MV_X] << 2, mv_before_clipping[REFP_0][MV_Y] << 2, ref_pic->u, (qpel_gmv_x << 2)*chroma_w_fac, (qpel_gmv_y << 2)*chroma_h_fac, ref_pic->s_c, w >> w_shift
                    , pred[0][U_C], w >> w_shift, h >> h_shift, bit_depth_chroma, xeve_tbl_mc_c_coeff);
            xeve_mc_c(mv_before_clipping[REFP_0][MV_X] << 2, mv_before_clipping[REFP_0][MV_Y] << 2, ref_pic->v, (qpel_gmv_x << 2)*chroma_w_fac, (qpel_gmv_y << 2)*chroma_h_fac, ref_pic->s_c, w >> w_shift
                    , pred[0][V_C], w >> w_shift, h >> h_shift, bit_depth_chroma, xeve_tbl_mc_c_coeff);
        }

        bidx++;
    }

    /* check identical motion */
    if(REFI_IS_VALID(refi[REFP_0]) && REFI_IS_VALID(refi[REFP_1]))
    {
        if(refp[refi[REFP_0]][REFP_0].pic->poc == refp[refi[REFP_1]][REFP_1].pic->poc &&  mv_t[REFP_0][MV_X] == mv_t[REFP_1][MV_X] && mv_t[REFP_0][MV_Y] == mv_t[REFP_1][MV_Y])
        {
            return;
        }
    }

    if(REFI_IS_VALID(refi[REFP_1]))
    {
        /* backward */
        ref_pic = refp[refi[REFP_1]][REFP_1].pic;
        qpel_gmv_x = (x << 2) + mv_t[REFP_1][MV_X];
        qpel_gmv_y = (y << 2) + mv_t[REFP_1][MV_Y];

        xeve_mc_l(mv_before_clipping[REFP_1][MV_X] << 2, mv_before_clipping[REFP_1][MV_Y] << 2, ref_pic->y, (qpel_gmv_x << 2), (qpel_gmv_y << 2), ref_pic->s_l, w, pred[bidx][Y_C], w, h, bit_depth_luma, xeve_tbl_mc_l_coeff);
        if(chroma_format_idc)
        {
            xeve_mc_c(mv_before_clipping[REFP_1][MV_X] << 2, mv_before_clipping[REFP_1][MV_Y] << 2, ref_pic->u, (qpel_gmv_x << 2)*chroma_w_fac, (qpel_gmv_y << 2)*chroma_h_fac, ref_pic->s_c, w >> w_shift
                    , pred[bidx][U_C], w >> w_shift, h >> h_shift, bit_depth_chroma, xeve_tbl_mc_c_coeff);
            xeve_mc_c(mv_before_clipping[REFP_1][MV_X] << 2, mv_before_clipping[REFP_1][MV_Y] << 2, ref_pic->v, (qpel_gmv_x << 2)*chroma_w_fac, (qpel_gmv_y << 2)*chroma_h_fac, ref_pic->s_c, w >> w_shift
                    , pred[bidx][V_C], w >> w_shift, h >> h_shift, bit_depth_chroma, xeve_tbl_mc_c_coeff);
        }

        bidx++;
    }

    if(bidx == 2)
    {
        xeve_func_average_no_clip(pred[0][Y_C], pred[1][Y_C], pred[0][Y_C], w, w, w, w, h);
        w >>= w_shift;
        h >>= h_shift;
        if(chroma_format_idc)
        {
            xeve_func_average_no_clip(pred[0][U_C], pred[1][U_C], pred[0][U_C], w, w, w, w, h);
            xeve_func_average_no_clip(pred[0][V_C], pred[1][V_C], pred[0][V_C], w, w, w, w, h);
        }
    }
}

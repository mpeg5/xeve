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


const XEVE_INTRA_PRED_ANG (*xeve_func_intra_pred_ang)[2];

void xevem_get_nbr(int x, int y, int cuw, int cuh, pel *src, int s_src, u16 avail_cu, pel nb[N_C][N_REF][MAX_CU_SIZE * 3], int scup, u32 * map_scu
                 , int w_scu, int h_scu, int ch_type, int constrained_intra_pred, u8 * map_tidx, int bit_depth, int chroma_format_idc)
{
    int  i, j;
    int w_shift = (XEVE_GET_CHROMA_W_SHIFT(chroma_format_idc));
    int h_shift = (XEVE_GET_CHROMA_H_SHIFT(chroma_format_idc));
    int  scuw = (ch_type == Y_C) ? (cuw >> MIN_CU_LOG2) : (cuw >> (MIN_CU_LOG2 - w_shift));
    int  scuh = (ch_type == Y_C) ? (cuh >> MIN_CU_LOG2) : (cuh >> (MIN_CU_LOG2 - h_shift));
    int  unit_size = (ch_type == Y_C) ? MIN_CU_SIZE : (MIN_CU_SIZE >> 1);
    int  x_scu = PEL2SCU(ch_type == Y_C ? x : x << w_shift);
    int  y_scu = PEL2SCU(ch_type == Y_C ? y : y << h_shift);
    pel *tmp = src;
    pel *left = nb[ch_type][0] + 2;
    pel *up = nb[ch_type][1] + cuh;
    pel *right = nb[ch_type][2] + 2;
    scuh = ((ch_type != Y_C) && (chroma_format_idc == 2)) ? scuh * 2 : scuh;
    unit_size = ((ch_type != Y_C) && (chroma_format_idc == 3)) ? unit_size * 2 : unit_size;

    if (IS_AVAIL(avail_cu, AVAIL_UP_LE) && (!constrained_intra_pred || MCU_GET_IF(map_scu[scup - w_scu - 1])) &&
        (map_tidx[scup] == map_tidx[scup - w_scu - 1])  )
    {
        xeve_mcpy(up - 1, src - s_src - 1, cuw * sizeof(pel));
    }
    else
    {
        up[-1] = 1 << (bit_depth - 1);
    }

    for (i = 0; i < (scuw + scuh); i++)
    {
        int is_avail = (y_scu > 0) && (x_scu + i < w_scu);
        if (is_avail && MCU_GET_COD(map_scu[scup - w_scu + i]) && (!constrained_intra_pred || MCU_GET_IF(map_scu[scup - w_scu + i])) &&
            (map_tidx[scup] == map_tidx[scup - w_scu + i]))
        {
            xeve_mcpy(up + i * unit_size, src - s_src + i * unit_size, unit_size * sizeof(pel));
        }
        else
        {
            xeve_mset_16b(up + i * unit_size, up[i * unit_size - 1], unit_size);
        }
    }

    if (x_scu > 0)
    {
        for (i = 0; i < scuh; i++)
        {
            if (scup > 0 && y_scu > 0 && (x_scu - 1 - i >= 0) && MCU_GET_COD(map_scu[scup - w_scu - 1 - i]) &&
                (!constrained_intra_pred || MCU_GET_IF(map_scu[scup - w_scu - 1 - i])) && (map_tidx[scup] == map_tidx[scup - w_scu - 1 - i]))
            {
                xeve_mcpy(up - (i + 1) * unit_size, src - s_src - (i + 1) * unit_size, unit_size * sizeof(pel));
            }
            else
            {
                xeve_mset_16b(up - (i + 1) * unit_size, up[-i * unit_size], unit_size);
            }
        }
    }
    else
    {
        xeve_mset_16b(up - cuh, up[0], cuh);
    }

    src--;
    left[-1] = up[-1];

    for (i = 0; i < (scuh + scuw); ++i)
    {
        int is_avail = (x_scu > 0) && (y_scu + i < h_scu);
        if (is_avail && MCU_GET_COD(map_scu[scup - 1 + i * w_scu]) && (!constrained_intra_pred || MCU_GET_IF(map_scu[scup - 1 + i * w_scu])) &&
            (map_tidx[scup] == map_tidx[scup - 1 + i * w_scu]))
        {
            for (j = 0; j < unit_size; ++j)
            {
                left[i * unit_size + j] = *src;
                src += s_src;
            }
        }
        else
        {
            xeve_mset_16b(left + i * unit_size, left[i * unit_size - 1], unit_size);
            src += (s_src * unit_size);
        }
    }

    left[-2] = left[-1];

    src = tmp;

    src += cuw;
    right[-1] = up[cuw];

    for (i = 0; i < (scuh + scuw); i++)
    {
        /*Check if right neighbours are available */
        int is_avail = (x_scu + scuw < w_scu) && (y_scu + i < h_scu);
        if (is_avail && MCU_GET_COD(map_scu[scup + scuw + i * w_scu]) && (!constrained_intra_pred || MCU_GET_IF(map_scu[scup + scuw + i * w_scu])) &&
            (map_tidx[scup] == map_tidx[scup + scuw + i * w_scu]))
        {
            for (j = 0; j < unit_size; ++j)
            {
                right[i * unit_size + j] = *src;
                src += s_src;
            }
        }
        else
        {
            xeve_mset_16b(right + i * unit_size, right[i * unit_size - 1], unit_size);
            src += (s_src * unit_size);
        }
    }

    right[-2] = right[-1];
}

static const int lut_size_plus1[MAX_CU_LOG2 + 1] = { 2048, 1365, 819, 455, 241, 124, 63, 32 }; // 1/(w+1) = k >> 12

static void ipred_hor(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h)
{
    int i, j;

    if(avail_lr == LR_11)
    {
        const int multi_w = lut_size_plus1[xeve_tbl_log2[w]];
        const int shift_w = 12;
        for(i = 0; i < h; i++)
        {
            for(j = 0; j < w; j++)
            {
                pel vle, vri;

                vle = src_le[0];
                vri = src_ri[0];
                dst[j] = ((vle * (w - j) + vri * (j + 1) + (w >> 1)) * multi_w) >> shift_w;
            }
            dst += w; src_le++; src_ri++;
        }
    }
    else if(avail_lr == LR_01)
    {
        for(i = 0; i < h; i++)
        {
            for(j = 0; j < w; j++)
            {
                dst[j] = src_ri[0];
            }
            dst += w; src_ri++;
        }
    }
    else
    {
        for(i = 0; i < h; i++)
        {
            for(j = 0; j < w; j++)
            {
                dst[j] = src_le[0];
            }
            dst += w; src_le++;
        }
    }
}

static void ipred_vert(pel *src_le, pel *src_up, pel * src_ri, u16 avail_lr, pel *dst, int w, int h)
{
    int i, j;

    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            dst[j] = src_up[j];
        }
        dst += w;
    }
}

static int xeve_get_dc(const int numerator, const int w, const int h)
{
    const int log2_w = xeve_tbl_log2[w];
    const int log2_h = xeve_tbl_log2[h];
    const int shift_w = 12;

    int basic_shift = log2_w, log2_asp_ratio = 0;

    if (log2_w > log2_h)
    {
        basic_shift = log2_h;
        log2_asp_ratio = log2_w - log2_h;
    }
    else if (log2_w < log2_h)
    {
        basic_shift = log2_w;
        log2_asp_ratio = log2_h - log2_w;
    }

  return (numerator * lut_size_plus1[log2_asp_ratio]) >> (basic_shift + shift_w);
}

static void ipred_dc(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h)
{
    int dc = 0;
    int wh, i, j;

    if (avail_lr == LR_11)
    {
        for (i = 0; i < h; i++) dc += src_le[i];
        for (i = 0; i < h; i++) dc += src_ri[i];
        for (j = 0; j < w; j++) dc += src_up[j];
        dc = xeve_get_dc(dc + ((w + h + h) >> 1), w, h << 1);
    }
    else if (avail_lr == LR_01)
    {
        for (i = 0; i < h; i++) dc += src_ri[i];
        for (j = 0; j < w; j++) dc += src_up[j];
        dc = xeve_get_dc(dc + ((w + h) >> 1), w, h);
    }
    else
    {
        for (i = 0; i < h; i++) dc += src_le[i];
        for (j = 0; j < w; j++) dc += src_up[j];
        dc = xeve_get_dc(dc + ((w + h) >> 1), w, h);
    }

    wh = w * h;

    for (i = 0; i < wh; i++)
    {
        dst[i] = (pel)dc;
    }
}

static void ipred_plane(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h, int bit_depth)
{
    pel *rsrc;
    int  coef_h = 0, coef_v = 0;
    int  a, b, c, x, y;
    int  w2 = w >> 1;
    int  h2 = h >> 1;
    int  ib_mult[6] = {13, 17, 5, 11, 23, 47};
    int  ib_shift[6] = {7, 10, 11, 15, 19, 23};
    int  idx_w = xeve_tbl_log2[w] < 2 ? 0 : xeve_tbl_log2[w] - 2;
    int  idx_h = xeve_tbl_log2[h] < 2 ? 0 : xeve_tbl_log2[h] - 2;
    int  im_h, is_h, im_v, is_v, temp, temp2;

    im_h = ib_mult[idx_w];
    is_h = ib_shift[idx_w];
    im_v = ib_mult[idx_h];
    is_v = ib_shift[idx_h];

    if(avail_lr == LR_01 || avail_lr == LR_11)
    {
        rsrc = src_up + w2;
        for(x = 1; x < w2 + 1; x++)
        {
            coef_h += x * (rsrc[-x] - rsrc[x]);
        }

        rsrc = src_ri + (h2 - 1);
        for(y = 1; y < h2 + 1; y++)
        {
            coef_v += y * (rsrc[y] - rsrc[-y]);
        }

        a = (src_ri[h - 1] + src_up[0]) << 4;
        b = ((coef_h << 5) * im_h + (1 << (is_h - 1))) >> is_h;
        c = ((coef_v << 5) * im_v + (1 << (is_v - 1))) >> is_v;

        temp = a - (h2 - 1) * c - (w2 - 1) * b + 16;

        for(y = 0; y < h; y++)
        {
            temp2 = temp;
            for(x = w - 1; x >= 0; x--)
            {
                dst[x] = XEVE_CLIP3(0, (1 << bit_depth) - 1, temp2 >> 5);
                temp2 += b;
            }
            temp += c; dst += w;
        }
    }
    else
    {
        rsrc = src_up + (w2 - 1);
        for(x = 1; x < w2 + 1; x++)
        {
            coef_h += x * (rsrc[x] - rsrc[-x]);
        }

        rsrc = src_le + (h2 - 1);
        for(y = 1; y < h2 + 1; y++)
        {
            coef_v += y * (rsrc[y] - rsrc[-y]);
        }

        a = (src_le[h - 1] + src_up[w - 1]) << 4;
        b = ((coef_h << 5) * im_h + (1 << (is_h - 1))) >> is_h;
        c = ((coef_v << 5) * im_v + (1 << (is_v - 1))) >> is_v;

        temp = a - (h2 - 1) * c - (w2 - 1) * b + 16;

        for(y = 0; y < h; y++)
        {
            temp2 = temp;
            for(x = 0; x < w; x++)
            {
                dst[x] = XEVE_CLIP3(0, (1 << bit_depth) - 1, temp2 >> 5);
                temp2 += b;
            }
            temp += c; dst += w;
        }
    }
}

static void ipred_bi(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h, int bit_depth)
{
    int x, y;
    int ishift_x = xeve_tbl_log2[w];
    int ishift_y = xeve_tbl_log2[h];
    int ishift = XEVE_MIN(ishift_x, ishift_y);
    int ishift_xy = ishift_x + ishift_y + 1;
    int offset = 1 << (ishift_x + ishift_y);
    int a, b, c, wt, wxy, tmp;
    int predx;
    int ref_up[MAX_CU_SIZE], ref_le[MAX_CU_SIZE], up[MAX_CU_SIZE], le[MAX_CU_SIZE], wy[MAX_CU_SIZE];
    int ref_ri[MAX_CU_SIZE], ri[MAX_CU_SIZE];
    int dst_tmp[MAX_CU_SIZE][MAX_CU_SIZE];
    int wc, tbl_wc[6] = {-1, 341, 205, 114, 60, 31};
    int log2_w = xeve_tbl_log2[w];
    int log2_h = xeve_tbl_log2[h];
    int multi_w = lut_size_plus1[log2_w];
    int shift_w = 12;

    wc = ishift_x > ishift_y ? ishift_x - ishift_y : ishift_y - ishift_x;
    xeve_assert(wc <= 5);
    wc = tbl_wc[wc];

    for(x = 0; x < w; x++) ref_up[x] = src_up[x];
    for(y = 0; y < h; y++) ref_le[y] = src_le[y];
    for(y = 0; y < h; y++) ref_ri[y] = src_ri[y];

    if(avail_lr == LR_11)
    {
        for(y = 0; y < h; y++)
        {
            for(x = 0; x < w; x++)
            {
                dst_tmp[y][x] = (ref_le[y] * (w - x) + ref_ri[y] * (x + 1) + (w >> 1)) * multi_w >> shift_w;
            }
        }

        for(x = 0; x < w; x++)
        {
            for(y = 0; y < h; y++)
            {
                tmp = (ref_up[x] * (h - 1 - y) + dst_tmp[h - 1][x] * (y + 1) + (h >> 1)) >> log2_h;
                dst[y * w + x] = (dst_tmp[y][x] + tmp + 1) >> 1;
            }
        }
    }
    else if(avail_lr == LR_01)
    {
        a = src_up[-1];
        b = src_ri[h];
        c = (w == h) ? (a + b + 1) >> 1 : (((a << ishift_x) + (b << ishift_y)) * wc + (1 << (ishift + 9))) >> (ishift + 10);
        wt = (c << 1) - a - b;

        for(x = w - 1; x >= 0; x--)
        {
            up[x] = b - ref_up[x];
            ref_up[x] <<= ishift_y;
        }
        tmp = 0;
        for(y = 0; y < h; y++)
        {
            ri[y] = a - ref_ri[y];
            ref_ri[y] <<= ishift_x;
            wy[y] = tmp;
            tmp += wt;
        }

        for(y = 0; y < h; y++)
        {
            predx = ref_ri[y];
            wxy = 0;
            for(x = w - 1; x >= 0; x--)
            {
                predx += ri[y];
                ref_up[x] += up[x];
                dst[x] = ((predx << ishift_y) + (ref_up[x] << ishift_x) + wxy + offset) >> ishift_xy;
                dst[x] = XEVE_CLIP3(0, (1 << bit_depth) - 1, dst[x]);
                wxy += wy[y];
            }
            dst += w;
        }
    }
    else
    {
        a = src_up[w];
        b = src_le[h];
        c = (w == h) ? (a + b + 1) >> 1 : (((a << ishift_x) + (b << ishift_y)) * wc + (1 << (ishift + 9))) >> (ishift + 10);
        wt = (c << 1) - a - b;

        for(x = 0; x < w; x++)
        {
            up[x] = b - ref_up[x];
            ref_up[x] <<= ishift_y;
        }
        tmp = 0;
        for(y = 0; y < h; y++)
        {
            le[y] = a - ref_le[y];
            ref_le[y] <<= ishift_x;
            wy[y] = tmp;
            tmp += wt;
        }

        for(y = 0; y < h; y++)
        {
            predx = ref_le[y];
            wxy = 0;
            for(x = 0; x < w; x++)
            {
                predx += le[y];
                ref_up[x] += up[x];
                dst[x] = XEVE_CLIP3(0, (1 << bit_depth) - 1, (((predx << ishift_y) + (ref_up[x] << ishift_x) + wxy + offset) >> ishift_xy));
                wxy += wy[y];
            }
            dst += w;
        }
    }
}

#define GET_REF_POS(mt,d_in,d_out,offset) \
    (d_out) = ((d_in) * (mt)) >> 10;\
    (offset) = (((d_in) * (mt)) >> 5) - ((d_out) << 5);

#define ADI_4T_FILTER_BITS                 7
#define ADI_4T_FILTER_OFFSET              (1<<(ADI_4T_FILTER_BITS-1))

void ipred_ang_less_ver_no_right(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h, int ipm, int bit_depth)
{
    int i, j;
    const int(*tbl_filt)[4];
    const int * mt = xevem_tbl_ipred_dxdy[ipm];
    const int pos_max = w + h - 1;
    const int pos_min = -1;

    tbl_filt = xevem_tbl_ipred_adi;
    pel*  src_ch = src_up;

    for (j = 0; j < h; j++)
    {
        int t_dx = ((j + 1) * (mt[0])) >> 10;
        int offset = (((j + 1) * (mt[0])) >> 5) - ((t_dx) << 5);
        const int* filter = (tbl_filt + offset)[0];

        for (i = 0; i < w; i++)
        {
            int x = i + t_dx;
            int pn_n1 = x - 1;
            int p     = x;
            int pn    = x + 1;
            int pn_p2 = x + 2;

            pn_n1 = XEVE_CLIP3(pos_min, pos_max, pn_n1);
            p     = XEVE_CLIP3(pos_min, pos_max, p);
            pn    = XEVE_CLIP3(pos_min, pos_max, pn);
            pn_p2 = XEVE_CLIP3(pos_min, pos_max, pn_p2);

            pel temp_pel = (src_ch[pn_n1] * filter[0] + src_ch[p] * filter[1] + src_ch[pn] * filter[2] + src_ch[pn_p2] * filter[3] + ADI_4T_FILTER_OFFSET) >> ADI_4T_FILTER_BITS;
            dst[i] = XEVE_CLIP3(0, (1 << bit_depth) - 1, temp_pel);

        }
        dst += w;
    }
}

void ipred_ang_less_ver_on_right(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h, int ipm, int bit_depth)
{
    int i, j;
    const int(*tbl_filt)[4];
    const int * mt = xevem_tbl_ipred_dxdy[ipm];
    const int pos_max = w + h - 1;
    const int pos_min = -1;

    tbl_filt = xevem_tbl_ipred_adi;

    for (j = 0; j < h; j++)
    {
        int t_dx = ((j + 1) * (mt[0])) >> 10;
        int offset = (((j + 1) * (mt[0])) >> 5) - ((t_dx) << 5);
        const int*  filter = (tbl_filt + offset)[0];

        pel* src_ch = src_up;
        for (i = 0; i < w - t_dx; i++)
        {
            int x = i + t_dx;
            int pn_n1 = x - 1;
            int p     = x;
            int pn    = x + 1;
            int pn_p2 = x + 2;

            pn_n1 = XEVE_CLIP3(pos_min, pos_max, pn_n1);
            p     = XEVE_CLIP3(pos_min, pos_max, p);
            pn    = XEVE_CLIP3(pos_min, pos_max, pn);
            pn_p2 = XEVE_CLIP3(pos_min, pos_max, pn_p2);

            pel temp_pel = (src_ch[pn_n1] * filter[0] + src_ch[p] * filter[1] + src_ch[pn] * filter[2] + src_ch[pn_p2] * filter[3] + ADI_4T_FILTER_OFFSET) >> ADI_4T_FILTER_BITS;
            dst[i] = XEVE_CLIP3(0, (1 << bit_depth) - 1, temp_pel);
        }

        src_ch = src_ri;
        for (i = XEVE_MAX(w - t_dx, 0) ; i < w; i++)
        {
            int t_dy = ((w - i) * (mt[1])) >> 10;
            int offset = (((w - i) * (mt[1])) >> 5) - ((t_dy) << 5);
            int y = j - t_dy;
            int pn_n1 = y + 1;
            int p     = y;
            int pn    = y - 1;
            int pn_p2 = y - 2;

            pn_n1 = XEVE_CLIP3(pos_min, pos_max, pn_n1);
            p = XEVE_CLIP3(pos_min, pos_max, p);
            pn = XEVE_CLIP3(pos_min, pos_max, pn);
            pn_p2 = XEVE_CLIP3(pos_min, pos_max, pn_p2);

            const int* filter = (tbl_filt + offset)[0];
            pel temp_pel = (src_ch[pn_n1] * filter[0] + src_ch[p] * filter[1] + src_ch[pn] * filter[2] + src_ch[pn_p2] * filter[3] + ADI_4T_FILTER_OFFSET) >> ADI_4T_FILTER_BITS;
            dst[i] = XEVE_CLIP3(0, (1 << bit_depth) - 1, temp_pel);
        }

        dst += w;
    }
}

void ipred_ang_gt_hor_no_right(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h, int ipm, int bit_depth)
{
    const int(*tbl_filt)[4];
    const int * mt = xevem_tbl_ipred_dxdy[ipm];
    int i, j;
    const int pos_max = w + h - 1;
    const int pos_min = -1;

    tbl_filt = xevem_tbl_ipred_adi;

    pel* src_ch = src_le;

    for (j = 0; j < h; j++)
    {
        for (i = 0; i < w; i++)
        {
            int t_dy = ((i + 1) * (mt[1])) >> 10;
            int offset = (((i + 1) * (mt[1])) >> 5) - ((t_dy) << 5);
            int y = j + t_dy;

            int pn_n1 = y - 1;
            int p     = y;
            int pn    = y + 1;
            int pn_p2 = y + 2;

            pn_n1 = XEVE_CLIP3(pos_min, pos_max, pn_n1);
            p     = XEVE_CLIP3(pos_min, pos_max, p);
            pn    = XEVE_CLIP3(pos_min, pos_max, pn);
            pn_p2 = XEVE_CLIP3(pos_min, pos_max, pn_p2);

            const int* filter = (tbl_filt + offset)[0];
            pel temp_pel = (src_ch[pn_n1] * filter[0] + src_ch[p] * filter[1] + src_ch[pn] * filter[2] + src_ch[pn_p2] * filter[3] + ADI_4T_FILTER_OFFSET) >> ADI_4T_FILTER_BITS;
            dst[i] = XEVE_CLIP3(0, (1 << bit_depth) - 1, temp_pel);
        }
        dst += w;
    }
}

void ipred_ang_gt_hor_on_right(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h, int ipm, int bit_depth)
{
    const int(*tbl_filt)[4];
    const int * mt = xevem_tbl_ipred_dxdy[ipm];
    pel * src_ch = NULL;
    int p, pn, pn_n1, pn_p2;

    int i, j;
    const int pos_max = w + h - 1;
    const int pos_min = -1;

    tbl_filt = xevem_tbl_ipred_adi;

    for (j = 0; j < h; j++)
    {
        for (i = 0; i < w; i++)
        {
            int t_dy = ((w - i) * (mt[1])) >> 10;
            int offset = (((w - i) * (mt[1])) >> 5) - ((t_dy) << 5);

            if (j < t_dy)
            {
                int t_dx = ((w - i) * (mt[0])) >> 10;
                offset = (((w - i) * (mt[0])) >> 5) - ((t_dx) << 5);

                int x = i + t_dx;

                src_ch = src_up;
                pn_n1 = x - 1;
                p     = x;
                pn    = x + 1;
                pn_p2 = x + 2;
            }
            else
            {
                int y = j - t_dy;

                src_ch = src_ri;
                pn_n1 = y + 1;
                p     = y;
                pn    = y - 1;
                pn_p2 = y - 2;
            }

            pn_n1 = XEVE_CLIP3(pos_min, pos_max, pn_n1);
            p     = XEVE_CLIP3(pos_min, pos_max, p);
            pn    = XEVE_CLIP3(pos_min, pos_max, pn);
            pn_p2 = XEVE_CLIP3(pos_min, pos_max, pn_p2);

            const int* filter = (tbl_filt + offset)[0];
            pel temp_pel = (src_ch[pn_n1] * filter[0] + src_ch[p] * filter[1] + src_ch[pn] * filter[2] + src_ch[pn_p2] * filter[3] + ADI_4T_FILTER_OFFSET) >> ADI_4T_FILTER_BITS;
            dst[i] = XEVE_CLIP3(0, (1 << bit_depth) - 1, temp_pel);
        }
        dst += w;
    }
}

void ipred_ang_no_right(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h, int ipm, int bit_depth)
{
    const int(*tbl_filt)[4];
    const int * mt = xevem_tbl_ipred_dxdy[ipm];
    pel * src_ch = NULL;
    int p, pn, pn_n1, pn_p2;
    int i, j;
    const int pos_max = w + h - 1;
    const int pos_min = -1;

    tbl_filt = xevem_tbl_ipred_adi;

    for (j = 0; j < h; j++)
    {
        for (i = 0; i < w; i++)
        {
            int t_dy = ((i + 1) * (mt[1])) >> 10;
            int offset = (((i + 1) * (mt[1])) >> 5) - ((t_dy) << 5);

            if (j < t_dy)
            {
                int t_dx = ((j + 1) * (mt[0])) >> 10;
                offset = (((j + 1) * (mt[0])) >> 5) - ((t_dx) << 5);

                int x = i - t_dx;

                pn_n1 = x + 1;
                p     = x;
                pn    = x - 1;
                pn_p2 = x - 2;

                src_ch = src_up;
            }
            else
            {
                int y = j - t_dy;

                pn_n1 = y + 1;
                p     = y;
                pn    = y - 1;
                pn_p2 = y - 2;

                src_ch = src_le;
            }

            pn_n1 = XEVE_CLIP3(pos_min, pos_max, pn_n1);
            p     = XEVE_CLIP3(pos_min, pos_max, p);
            pn    = XEVE_CLIP3(pos_min, pos_max, pn);
            pn_p2 = XEVE_CLIP3(pos_min, pos_max, pn_p2);

            const int* filter = (tbl_filt + offset)[0];
            pel temp_pel = (src_ch[pn_n1] * filter[0] + src_ch[p] * filter[1] + src_ch[pn] * filter[2] + src_ch[pn_p2] * filter[3] + ADI_4T_FILTER_OFFSET) >> ADI_4T_FILTER_BITS;
            dst[i] = XEVE_CLIP3(0, (1 << bit_depth) - 1, temp_pel);
        }
        dst += w;
    }
}

void ipred_ang_only_right(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h, int ipm, int bit_depth)
{
    const int(*tbl_filt)[4];
    const int * mt = xevem_tbl_ipred_dxdy[ipm];
    pel * src_ch = NULL;
    int p, pn, pn_n1, pn_p2;

    int i, j;
    const int pos_max = w + h - 1;
    const int pos_min = -1;

    tbl_filt = xevem_tbl_ipred_adi;

    for (j = 0; j < h; j++)
    {
        for (i = 0; i < w; i++)
        {
            int t_dy = ((i + 1) * (mt[1])) >> 10;
            int offset = (((i + 1) * (mt[1])) >> 5) - ((t_dy) << 5);

            if (j < t_dy)
            {
                int t_dx = ((j + 1) * (mt[0])) >> 10;
                offset = (((j + 1) * (mt[0])) >> 5) - ((t_dx) << 5);

                int x = i - t_dx;

                pn_n1 = x + 1;
                p     = x;
                pn    = x - 1;
                pn_p2 = x - 2;

                src_ch = src_up;
            }
            else
            {
                t_dy = ((w - i) * (mt[1])) >> 10;
                offset = (((w - i) * (mt[1])) >> 5) - ((t_dy) << 5);

                int y = j + t_dy;

                pn_n1 = y - 1;
                p     = y;
                pn    = y + 1;
                pn_p2 = y + 2;

                src_ch = src_ri;
            }

            pn_n1 = XEVE_CLIP3(pos_min, pos_max, pn_n1);
            p     = XEVE_CLIP3(pos_min, pos_max, p);
            pn    = XEVE_CLIP3(pos_min, pos_max, pn);
            pn_p2 = XEVE_CLIP3(pos_min, pos_max, pn_p2);

            const int* filter = (tbl_filt + offset)[0];
            pel temp_pel = (src_ch[pn_n1] * filter[0] + src_ch[p] * filter[1] + src_ch[pn] * filter[2] + src_ch[pn_p2] * filter[3] + ADI_4T_FILTER_OFFSET) >> ADI_4T_FILTER_BITS;
            dst[i] = XEVE_CLIP3(0, (1 << bit_depth) - 1, temp_pel);
        }
        dst += w;
    }
}

const XEVE_INTRA_PRED_ANG xeve_tbl_intra_pred_ang[3][2] =
{
    {ipred_ang_less_ver_no_right, ipred_ang_less_ver_on_right},
    {ipred_ang_gt_hor_no_right, ipred_ang_gt_hor_on_right},
    {ipred_ang_no_right, ipred_ang_only_right},
};

void xevem_ipred(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int ipm, int w, int h, int bit_depth)
{
    int func_ipm, func_lr;

    switch(ipm)
    {
        case IPD_VER:
            ipred_vert(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        case IPD_HOR:
            ipred_hor(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        case IPD_DC:
            ipred_dc(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        case IPD_PLN:
            ipred_plane(src_le, src_up, src_ri, avail_lr, dst, w, h, bit_depth);
            break;
        case IPD_BI:
            ipred_bi(src_le, src_up, src_ri, avail_lr, dst, w, h, bit_depth);
            break;
        default:
            func_ipm = (ipm < IPD_VER ? 0 : (ipm > IPD_HOR ? 1 : 2));
            func_lr = func_ipm < 2 ? (((avail_lr >> 1) & 1)) : (avail_lr == LR_01);
            xeve_func_intra_pred_ang[func_ipm][func_lr](src_le, src_up, src_ri, avail_lr, dst, w, h, ipm, bit_depth);
            break;
    }
}

void xevem_ipred_uv(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int ipm_c, int ipm, int w, int h, int bit_depth)
{
    int func_ipm, func_lr;

    if(ipm_c == IPD_DM_C && XEVE_IPRED_CHK_CONV(ipm))
    {
        ipm_c = XEVE_IPRED_CONV_L2C(ipm);
    }

    switch(ipm_c)
    {
        case IPD_DM_C:
            switch(ipm)
            {
                case IPD_PLN:
                    ipred_plane(src_le, src_up, src_ri, avail_lr, dst, w, h, bit_depth);
                    break;
                default:
                    func_ipm = (ipm < IPD_VER ? 0 : (ipm > IPD_HOR ? 1 : 2));
                    func_lr = func_ipm < 2 ? (((avail_lr >> 1) & 1)) : (avail_lr == LR_01);
                    xeve_func_intra_pred_ang[func_ipm][func_lr](src_le, src_up, src_ri, avail_lr, dst, w, h, ipm, bit_depth);
                    break;
            }
            break;

        case IPD_DC_C:
            ipred_dc(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        case IPD_HOR_C:
            ipred_hor(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        case IPD_VER_C:
            ipred_vert(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;

        case IPD_BI_C:
            ipred_bi(src_le, src_up, src_ri, avail_lr, dst, w, h, bit_depth);
            break;
        default:
            xeve_trace("\n illegal chroma intra prediction mode\n");
            break;
    }
}

int intra_mode_list[IPD_CNT] = {
    IPD_DC, IPD_BI, IPD_VER, IPD_PLN, IPD_HOR,
    IPD_VER - 1, IPD_VER + 1, IPD_VER - 2, IPD_VER + 2, IPD_VER - 3, IPD_VER + 3,
    IPD_HOR - 1,IPD_HOR + 1, IPD_HOR - 2, IPD_HOR + 2, IPD_HOR - 3, IPD_HOR + 3,
    IPD_DIA_R,
    IPD_DIA_L, IPD_DIA_L - 3, IPD_DIA_L - 2, IPD_DIA_L - 1,
    IPD_DIA_U, IPD_DIA_U + 1, IPD_DIA_U + 2,
    IPD_VER + 5, IPD_VER + 4,
    IPD_HOR - 4, IPD_HOR - 5,
    IPD_VER - 5, IPD_VER - 4,
    IPD_HOR + 5, IPD_HOR + 4,
};

void xevem_get_mpm(int x_scu, int y_scu, int cuw, int cuh, u32 *map_scu, s8 *map_ipm, int scup, int w_scu,
                   u8 mpm[2], u16 avail_lr, u8 mpm_ext[8], u8 pms[IPD_CNT] /* 10 third MPM */, u8 * map_tidx)
{
    u8 ipm_l = IPD_DC, ipm_u = IPD_DC;
    u8 ipm_r = IPD_DC;
    int scuw = cuw >> MIN_CU_LOG2;
    int valid_l = 0, valid_u = 0;
    int valid_r = 0;
    int i;
    int mode_idx = 0;
    int check = 8;
    int included_mode[IPD_CNT];
    int *default_mode_list = intra_mode_list;

    xeve_mset(included_mode, 0, sizeof(included_mode));

    if(x_scu > 0 && MCU_GET_IF(map_scu[scup - 1]) && MCU_GET_COD(map_scu[scup - 1]) && (map_tidx[scup] == map_tidx[scup - 1]))
    {
        ipm_l = map_ipm[scup - 1];
        valid_l = 1;
    }

    if(y_scu > 0 && MCU_GET_IF(map_scu[scup - w_scu]) && MCU_GET_COD(map_scu[scup - w_scu]) && (map_tidx[scup] == map_tidx[scup - w_scu]))
    {
        ipm_u = map_ipm[scup - w_scu];
        valid_u = 1;
    }

    if(x_scu + scuw < w_scu && MCU_GET_IF(map_scu[scup + scuw]) && MCU_GET_COD(map_scu[scup + scuw]) && (map_tidx[scup] == map_tidx[scup + scuw]))
    {
        ipm_r = map_ipm[scup + scuw];

        if(valid_l && valid_u)
        {
            if(ipm_l == ipm_u)
            {
                ipm_u = ipm_r;
            }
            else
            {
                valid_r = 1;
            }
        }
        else if(valid_l == 0)
        {
            ipm_l = ipm_r;
        }
        else if(valid_u == 0)
        {
            ipm_u = ipm_r;
        }
        if(valid_r)
        {
            if((ipm_l == ipm_r) || (ipm_u == ipm_r))
            {
                valid_r = 0;
            }
        }
    }

    mpm[0] = XEVE_MIN(ipm_l, ipm_u);
    mpm[1] = XEVE_MAX(ipm_l, ipm_u);

    if(mpm[0] == mpm[1])
    {
        mpm[0] = IPD_DC;
        mpm[1] = (mpm[1] == IPD_DC) ? IPD_BI : mpm[1];
    }

    if(valid_r)
    {
        int j;
        if(mpm[0] < 3 && mpm[1] < 3)
        {
            if(ipm_r < 3)
            {
                if(mpm[0] == IPD_DC)
                {
                    mpm_ext[0] = ((mpm[1] == IPD_BI) ? IPD_PLN : IPD_BI);
                }
                else if(mpm[0] == IPD_PLN)
                {
                    mpm_ext[0] = IPD_DC;
                }
                mpm_ext[1] = IPD_VER;
                mpm_ext[2] = IPD_HOR;
                mpm_ext[3] = IPD_DIA_R;
                mpm_ext[4] = IPD_DIA_L;
                mpm_ext[5] = IPD_DIA_U;
                mpm_ext[6] = IPD_VER + 4;
                mpm_ext[7] = IPD_HOR - 4;
            }
            else
            {
                int list[10] = {IPD_VER, IPD_HOR, IPD_DIA_R, IPD_PLN, IPD_DIA_L, IPD_DIA_U, IPD_VER + 4, IPD_HOR - 4, IPD_VER - 4, IPD_HOR + 4};
                if(mpm[0] == IPD_DC)
                {
                    mpm_ext[0] = ((mpm[1] == IPD_BI) ? IPD_PLN : IPD_BI);
                }
                else if(mpm[0] == IPD_PLN)
                {
                    mpm_ext[0] = IPD_DC;
                }
                mpm_ext[1] = ipm_r;
                mpm_ext[2] = ((ipm_r == 3 || ipm_r == 4) ? ipm_r + 1 : ipm_r - 2);
                mpm_ext[3] = ((ipm_r == IPD_CNT - 1 || ipm_r == IPD_CNT - 2) ? ipm_r - 1 : ipm_r + 2);
                int cnt_cand = 4;
                for (i = 0; i < 10; i++)
                {
                    for (j = 0; j < cnt_cand; j++)
                    {
                        if (list[i] == mpm_ext[j] || list[i] == mpm[0] || list[i] == mpm[1])
                        {
                            break;
                        }
                        if (j == cnt_cand - 1)
                        {
                            mpm_ext[cnt_cand] = list[i];
                            cnt_cand++;
                            break;
                        }
                    }
                    if (cnt_cand > 7)
                    {
                        break;
                    }
                }
            }
        }
        else if(mpm[0] < 3)
        {
            if(ipm_r < 3)
            {
                if(mpm[0] == IPD_PLN)
                {
                    mpm_ext[0] = IPD_BI;
                    mpm_ext[1] = IPD_DC;
                }
                else
                {
                    mpm_ext[0] = (mpm[0] == IPD_BI ? IPD_DC : IPD_BI);
                    mpm_ext[1] = IPD_PLN;
                }
                if(mpm[1] > IPD_CNT - 3)
                {
                    mpm_ext[2] = (mpm[1] == IPD_CNT - 1 ? IPD_CNT - 2 : IPD_CNT - 1);
                    mpm_ext[3] = IPD_CNT - 3;
                    mpm_ext[4] = IPD_CNT - 4;
                    mpm_ext[5] = IPD_CNT - 5;
                    mpm_ext[6] = IPD_HOR;
                    mpm_ext[7] = IPD_DIA_R;
                }
                else if(mpm[1] < 5)
                {
                    mpm_ext[2] = (mpm[1] == 3 ? 4 : 3);
                    mpm_ext[3] = 5;
                    mpm_ext[4] = 6;
                    mpm_ext[5] = 7;
                    mpm_ext[6] = IPD_VER;
                    mpm_ext[7] = IPD_DIA_R;
                }
                else
                {
                    mpm_ext[2] = mpm[1] + 2;
                    mpm_ext[3] = mpm[1] - 2;
                    mpm_ext[4] = mpm[1] + 1;
                    mpm_ext[5] = mpm[1] - 1;
                    if(mpm[1] <= 23 && mpm[1] >= 13)
                    {
                        mpm_ext[6] = mpm[1] - 5;
                        mpm_ext[7] = mpm[1] + 5;
                    }
                    else
                    {
                        mpm_ext[6] = (mpm[1] > 23) ? mpm[1] - 5 : mpm[1] + 5;
                        mpm_ext[7] = (mpm[1] > 23) ? mpm[1] - 10 : mpm[1] + 10;
                    }
                }
            }
            else
            {
                int list[15] = { 0, 0, 0, 0, 0, 0, 0, IPD_VER, IPD_HOR, IPD_DIA_R, IPD_PLN, IPD_DIA_L, IPD_DIA_U, IPD_VER + 4, IPD_HOR - 4};
                int cnt_cand = 0;
                list[0] = ((ipm_r == 3 || ipm_r == 4) ? ipm_r + 1 : ipm_r - 2);
                list[1] = ((ipm_r == IPD_CNT - 1 || ipm_r == IPD_CNT - 2) ? ipm_r - 1 : ipm_r + 2);
                list[2] = ((mpm[1] == 3 || mpm[1] == 4) ? mpm[1] + 1 : mpm[1] - 2);
                list[3] = ((mpm[1] == IPD_CNT - 1 || mpm[1] == IPD_CNT - 2) ? mpm[1] - 1 : mpm[1] + 2);
                list[4] = (ipm_r + mpm[1] + 1) >> 1;
                list[5] = (list[4] + ipm_r + 1) >> 1;
                list[6] = (list[4] + mpm[1] + 1) >> 1;

                if(mpm[0] == IPD_PLN)
                {
                    mpm_ext[0] = IPD_BI;
                    mpm_ext[1] = IPD_DC;
                }
                else
                {
                    mpm_ext[0] = (mpm[0] == IPD_BI ? IPD_DC : IPD_BI);
                    mpm_ext[1] = IPD_PLN;
                }
                mpm_ext[2] = ipm_r;

                cnt_cand = 3;
                for(i = 0; i < 15; i++)
                {
                    for(j = 0; j < cnt_cand; j++)
                    {
                        if(list[i] == mpm_ext[j] || list[i] == mpm[0] || list[i] == mpm[1])
                        {
                            break;
                        }
                        if(j == cnt_cand - 1)
                        {
                            mpm_ext[cnt_cand] = list[i];
                            cnt_cand++;
                            break;
                        }
                    }
                    if(cnt_cand > 7)
                    {
                        break;
                    }
                }
            }
        }
        else
        {
            if(ipm_r < 3)
            {
                int list[15] = {0, 0, 0, 0, 0, 0, 0, IPD_VER, IPD_HOR, IPD_DIA_R, IPD_PLN, IPD_DIA_L, IPD_DIA_U, IPD_VER + 4, IPD_HOR - 4};
                int cnt_cand = 0;
                list[0] = ((mpm[0] == 3 || mpm[0] == 4) ? mpm[0] + 1 : mpm[0] - 2);
                list[1] = ((mpm[0] == IPD_CNT - 2) ? mpm[0] - 1 : mpm[0] + 2);
                list[2] = ((mpm[1] == 4) ? mpm[1] + 1 : mpm[1] - 2);
                list[3] = ((mpm[1] == IPD_CNT - 1 || mpm[1] == IPD_CNT - 2) ? mpm[1] - 1 : mpm[1] + 2);
                list[4] = (mpm[0] + mpm[1] + 1) >> 1;
                list[5] = (list[4] + mpm[0] + 1) >> 1;
                list[6] = (list[4] + mpm[1] + 1) >> 1;

                mpm_ext[0] = ipm_r;
                mpm_ext[1] = (ipm_r == IPD_BI) ? IPD_DC : IPD_BI;

                cnt_cand = 2;
                for(i = 0; i < 15; i++)
                {
                    for(j = 0; j < cnt_cand; j++)
                    {
                        if(list[i] == mpm_ext[j] || list[i] == mpm[0] || list[i] == mpm[1])
                        {
                            break;
                        }
                        if(j == cnt_cand - 1)
                        {
                            mpm_ext[cnt_cand] = list[i];
                            cnt_cand++;
                            break;
                        }
                    }
                    if(cnt_cand > 7)
                    {
                        break;
                    }
                }
            }
            else
            {
                int list[16] = {0, 0, 0, 0, 0, 0, 0, 0, IPD_VER, IPD_HOR, IPD_DIA_R, IPD_PLN, IPD_DIA_L, IPD_DIA_U, IPD_VER + 4, IPD_HOR - 4};
                int cnt_cand = 0;
                list[0] = ((mpm[0] == 3 || mpm[0] == 4) ? mpm[0] + 1 : mpm[0] - 2);
                list[1] = ((mpm[0] == IPD_CNT - 2) ? mpm[0] - 1 : mpm[0] + 2);
                list[2] = ((mpm[1] == 4) ? mpm[1] + 1 : mpm[1] - 2);
                list[3] = ((mpm[1] == IPD_CNT - 1 || mpm[1] == IPD_CNT - 2) ? mpm[1] - 1 : mpm[1] + 2);
                list[4] = ((ipm_r == 3 || ipm_r == 4) ? ipm_r + 1 : ipm_r - 2);
                list[5] = ((ipm_r == IPD_CNT - 1 || ipm_r == IPD_CNT - 2) ? ipm_r - 1 : ipm_r + 2);
                list[6] = ((ipm_r < mpm[1]) ? (mpm[0] + ipm_r + 1) >> 1 : (mpm[0] + mpm[1] + 1) >> 1);
                list[7] = ((ipm_r < mpm[0]) ? (mpm[0] + mpm[1] + 1) >> 1 : (mpm[1] + ipm_r + 1) >> 1);

                mpm_ext[0] = IPD_BI;
                mpm_ext[1] = IPD_DC;
                mpm_ext[2] = ipm_r;

                cnt_cand = 3;
                for(i = 0; i < 16; i++)
                {
                    for(j = 0; j < cnt_cand; j++)
                    {
                        if(list[i] == mpm_ext[j] || list[i] == mpm[0] || list[i] == mpm[1])
                        {
                            break;
                        }
                        if(j == cnt_cand - 1)
                        {
                            mpm_ext[cnt_cand] = list[i];
                            cnt_cand++;
                            break;
                        }
                    }
                    if(cnt_cand > 7)
                    {
                        break;
                    }
                }
            }
        }
    }
    else
    {
        int j;
        if(mpm[0] < 3 && mpm[1] < 3)
        {
            int cnt_cand = 4;

            if(mpm[0] == IPD_DC)
            {
                mpm_ext[0] = ((mpm[1] == IPD_BI) ? IPD_PLN : IPD_BI);
            }
            else if(mpm[0] == IPD_PLN)
            {
                mpm_ext[0] = IPD_DC;
            }
            mpm_ext[1] = IPD_VER;
            mpm_ext[2] = IPD_HOR;
            mpm_ext[3] = IPD_DIA_R;
            mpm_ext[4] = IPD_DIA_L;
            mpm_ext[5] = IPD_DIA_U;
            mpm_ext[6] = IPD_VER + 4;
            mpm_ext[7] = IPD_HOR - 4;
        }
        else if(mpm[0] < 3)
        {
            if(mpm[0] == IPD_PLN)
            {
                mpm_ext[0] = IPD_BI;
                mpm_ext[1] = IPD_DC;
            }
            else
            {
                mpm_ext[0] = (mpm[0] == IPD_BI ? IPD_DC : IPD_BI);
                mpm_ext[1] = IPD_PLN;
            }

            if(mpm[1] > IPD_CNT - 3)
            {
                mpm_ext[2] = (mpm[1] == IPD_CNT - 1 ? IPD_CNT - 2 : IPD_CNT - 1);
                mpm_ext[3] = IPD_CNT - 3;
                mpm_ext[4] = IPD_CNT - 4;
                mpm_ext[5] = IPD_CNT - 5;
                mpm_ext[6] = IPD_HOR;
                mpm_ext[7] = IPD_DIA_R;
            }

            else if(mpm[1] < 5)
            {
                mpm_ext[2] = (mpm[1] == 3 ? 4 : 3);
                mpm_ext[3] = 5;
                mpm_ext[4] = 6;
                mpm_ext[5] = 7;
                mpm_ext[6] = IPD_VER;
                mpm_ext[7] = IPD_DIA_R;
            }
            else
            {
                mpm_ext[2] = mpm[1] + 2;
                mpm_ext[3] = mpm[1] - 2;
                mpm_ext[4] = mpm[1] + 1;
                mpm_ext[5] = mpm[1] - 1;

                if(mpm[1] <= 23 && mpm[1] >= 13)
                {
                    mpm_ext[6] = mpm[1] - 5;
                    mpm_ext[7] = mpm[1] + 5;
                }
                else
                {
                    mpm_ext[6] = (mpm[1] > 23) ? mpm[1] - 5 : mpm[1] + 5;
                    mpm_ext[7] = (mpm[1] > 23) ? mpm[1] - 10 : mpm[1] + 10;
                }
            }
        }
        else
        {
            int list[15] = {0, 0, 0, 0, 0, 0, 0, IPD_VER, IPD_HOR, IPD_DIA_R, IPD_PLN, IPD_DIA_L, IPD_DIA_U, IPD_VER + 4, IPD_HOR - 4};
            int cnt_cand = 0;
            list[0] = ((mpm[0] == 3 || mpm[0] == 4) ? mpm[0] + 1 : mpm[0] - 2);
            list[1] = ((mpm[0] == IPD_CNT - 2) ? mpm[0] - 1 : mpm[0] + 2);
            list[2] = ((mpm[1] == 4) ? mpm[1] + 1 : mpm[1] - 2);
            list[3] = ((mpm[1] == IPD_CNT - 1 || mpm[1] == IPD_CNT - 2) ? mpm[1] - 1 : mpm[1] + 2);
            list[4] = (mpm[0] + mpm[1] + 1) >> 1;
            list[5] = (list[4] + mpm[0] + 1) >> 1;
            list[6] = (list[4] + mpm[1] + 1) >> 1;

            mpm_ext[0] = IPD_BI;
            mpm_ext[1] = IPD_DC;

            cnt_cand = 2;
            for(i = 0; i < 15; i++)
            {
                for(j = 0; j < cnt_cand; j++)
                {
                    if(list[i] == mpm_ext[j] || list[i] == mpm[0] || list[i] == mpm[1])
                    {
                        break;
                    }
                    if(j == cnt_cand - 1)
                    {
                        mpm_ext[cnt_cand] = list[i];
                        cnt_cand++;
                        break;
                    }
                }
                if(cnt_cand > 7)
                {
                    break;
                }
            }
        }
    }

    for(i = 0; i < 2; i++)
    {
        if(!included_mode[mpm[i]])
        {
            included_mode[mpm[i]] = 1;
            pms[mode_idx] = mpm[i];
            mode_idx++;
        }
    }

    for(i = 0; i < check; i++)
    {
        if(!included_mode[mpm_ext[i]])
        {
            included_mode[mpm_ext[i]] = 1;
            pms[mode_idx] = mpm_ext[i];
            mode_idx++;
        }
    }

    for(i = 0; i < IPD_CNT; i++)
    {
        if(!included_mode[default_mode_list[i]])
        {
            included_mode[default_mode_list[i]] = 1;
            pms[mode_idx] = default_mode_list[i];
            mode_idx++;
        }
    }
    assert(mode_idx == IPD_CNT);
}

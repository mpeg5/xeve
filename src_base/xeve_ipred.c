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


void xeve_get_nbr(int x, int y, int cuw, int cuh, pel *src, int s_src, u16 avail_cu, pel nb[N_C][N_REF][MAX_CU_SIZE * 3], int scup, u32 * map_scu
                , int w_scu, int h_scu, int ch_type, int constrained_intra_pred, u8* map_tidx, int bit_depth, int chroma_format_idc)
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
    scuh = ((ch_type != Y_C) && (chroma_format_idc == 2)) ? scuh * 2 : scuh;
    unit_size = ((ch_type != Y_C) && (chroma_format_idc == 3)) ? unit_size * 2 : unit_size;

    if (IS_AVAIL(avail_cu, AVAIL_UP_LE) && (!constrained_intra_pred || MCU_GET_IF(map_scu[scup - w_scu - 1])) &&
        (map_tidx[scup] == map_tidx[scup - w_scu - 1]))
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
            xeve_mset_16b(up + i * unit_size, 1 << (bit_depth - 1), unit_size);
        }
    }

    src--;
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
            xeve_mset_16b(left + i * unit_size, 1 << (bit_depth - 1), unit_size);
            src += (s_src * unit_size);
        }
    }
    left[-1] = up[-1];
}

static void ipred_hor(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h)
{
    int i, j;
    for (i = 0; i < h; i++)
    {
        for (j = 0; j < w; j++)
        {
            dst[j] = src_le[0];
        }
        dst += w; src_le++;
    }
}

static const int lut_size_plus1[MAX_CU_LOG2 + 1] = { 2048, 1365, 819, 455, 241, 124, 63, 32 }; // 1/(w+1) = k >> 12

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

static void ipred_dc(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int w, int h)
{
    int dc = 0;
    int wh, i, j;

    for (i = 0; i < h; i++) dc += src_le[i];
    for (j = 0; j < w; j++) dc += src_up[j];
    dc = (dc + w) >> (xeve_tbl_log2[w] + 1);

    wh = w * h;

    for(i = 0; i < wh; i++)
    {
        dst[i] = (pel)dc;
    }
}

static void ipred_ul(pel *src_le, pel *src_up, pel * src_ri, u16 avail_lr, pel *dst, int w, int h)
{
    int i, j;
    for (i = 0; i<h; i++)
    {
        for (j = 0; j<w; j++)
        {
            int diag = i - j;
            if (diag > 0) {
                dst[j] = src_le[diag - 1];
            }
            else if (diag == 0)
            {
                dst[j] = src_up[-1];
            }
            else
            {
                dst[j] = src_up[-diag - 1];
            }
        }
        dst += w;
    }
}

static void ipred_ur(pel *src_le, pel *src_up, pel * src_ri, u16 avail_lr, pel *dst, int w, int h)
{
    int i, j;
    for (i = 0; i<h; i++)
    {
        for (j = 0; j<w; j++)
        {
            dst[j] = (src_up[i + j + 1] + src_le[i + j + 1]) >> 1;
        }
        dst += w;
    }
}

/* intra prediction for baseline profile */
void xeve_ipred(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int ipm, int w, int h)
{
    switch(ipm)
    {
        case IPD_VER_B:
            ipred_vert(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        case IPD_HOR_B:
            ipred_hor(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        case IPD_DC_B:
            ipred_dc(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        case IPD_UL_B:
            ipred_ul(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        case IPD_UR_B:
            ipred_ur(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        default:
            xeve_assert(0);
            xeve_trace("\n illegal intra prediction mode\n");
            break;
    }
}

void xeve_ipred_uv(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int ipm_c, int ipm, int w, int h)
{
    switch(ipm_c)
    {
        case IPD_DC_C_B:
            ipred_dc(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        case IPD_HOR_C_B:
            ipred_hor(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        case IPD_VER_C_B:
            ipred_vert(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        case IPD_UL_C_B:
            ipred_ul(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        case IPD_UR_C_B:
            ipred_ur(src_le, src_up, src_ri, avail_lr, dst, w, h);
            break;
        default:
            xeve_assert(0);
            xeve_trace("\n illegal chroma intra prediction mode\n");
            break;
    }
}

void xeve_get_mpm(int x_scu, int y_scu, int cuw, int cuh, u32 *map_scu, s8 *map_ipm, int scup, int w_scu, u8 ** mpm, u8 * map_tidx)
{
    u8 ipm_l = IPD_DC, ipm_u = IPD_DC;

    if(x_scu > 0 && MCU_GET_IF(map_scu[scup - 1]) && MCU_GET_COD(map_scu[scup - 1]) && (map_tidx[scup] == map_tidx[scup - 1]))
    {
        ipm_l = map_ipm[scup - 1] + 1;
    }
    if(y_scu > 0 && MCU_GET_IF(map_scu[scup - w_scu]) && MCU_GET_COD(map_scu[scup - w_scu]) && (map_tidx[scup] == map_tidx[scup - w_scu]))
    {
        ipm_u = map_ipm[scup - w_scu] + 1;
    }
    *mpm = (u8*)&xeve_tbl_mpm[ipm_l][ipm_u];
}

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
#include <math.h>

#define QUANT(c, scale, offset, shift) ((s16)((((c)*(scale)) + (offset)) >> (shift)))

const XEVE_TX(*xeve_func_tx)[MAX_TR_LOG2];

void xeve_trans_DST7_B4(s16* block, s16* coeff, s32 shift, s32 line, int skip_line, int skip_line_2);
void xeve_trans_DST7_B8(s16* block, s16* coeff, s32 shift, s32 line, int skip_line, int skip_line_2);
void xeve_trans_DST7_B16(s16* block, s16* coeff, s32 shift, s32 line, int skip_line, int skip_line_2);
void xeve_trans_DST7_B32(s16* block, s16* coeff, s32 shift, s32 line, int skip_line, int skip_line_2);
void xeve_trans_DCT8_B4(s16* block, s16* coeff, s32 shift, s32 line, int skip_line, int skip_line_2);
void xeve_trans_DCT8_B8(s16* block, s16* coeff, s32 shift, s32 line, int skip_line, int skip_line_2);
void xeve_trans_DCT8_B16(s16* block, s16* coeff, s32 shift, s32 line, int skip_line, int skip_line_2);
void xeve_trans_DCT8_B32(s16* block, s16* coeff, s32 shift, s32 line, int skip_line, int skip_line_2);
typedef void Trans(s16*, s16*, s32, s32, int, int);

Trans* xeve_trans_map_tbl[16][5] =
{
    { NULL, xeve_trans_DCT8_B4, xeve_trans_DCT8_B8, xeve_trans_DCT8_B16, xeve_trans_DCT8_B32 },
    { NULL, xeve_trans_DST7_B4, xeve_trans_DST7_B8, xeve_trans_DST7_B16, xeve_trans_DST7_B32 },
};

void tx_pb2(s16 * src, s16 * dst, int shift, int line)
{
    int j;
    int E, O;
    int add = shift == 0 ? 0 : 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        /* E and O */
        E = src[j * 2 + 0] + src[j * 2 + 1];
        O = src[j * 2 + 0] - src[j * 2 + 1];

        dst[0 * line + j] = (xeve_tbl_tm2[0][0] * E + add) >> shift;
        dst[1 * line + j] = (xeve_tbl_tm2[1][0] * O + add) >> shift;
    }
}

void tx_pb4(s16 * src, s16 * dst, int shift, int line)
{
    int j;
    int E[2], O[2];
    int add = 1 << (shift - 1);
    for (j = 0; j < line; j++)
    {
        /* E and O */
        E[0] = src[j * 4 + 0] + src[j * 4 + 3];
        O[0] = src[j * 4 + 0] - src[j * 4 + 3];
        E[1] = src[j * 4 + 1] + src[j * 4 + 2];
        O[1] = src[j * 4 + 1] - src[j * 4 + 2];

        dst[0 * line + j] = (xeve_tbl_tm4[0][0] * E[0] + xeve_tbl_tm4[0][1] * E[1] + add) >> shift;
        dst[2 * line + j] = (xeve_tbl_tm4[2][0] * E[0] + xeve_tbl_tm4[2][1] * E[1] + add) >> shift;
        dst[1 * line + j] = (xeve_tbl_tm4[1][0] * O[0] + xeve_tbl_tm4[1][1] * O[1] + add) >> shift;
        dst[3 * line + j] = (xeve_tbl_tm4[3][0] * O[0] + xeve_tbl_tm4[3][1] * O[1] + add) >> shift;
    }
}

void tx_pb8(s16 * src, s16 * dst, int shift, int line)
{
    int j, k;
    int E[4], O[4];
    int EE[2], EO[2];
    int add = 1 << (shift - 1);
    for (j = 0; j < line; j++)
    {
        /* E and O*/
        for (k = 0; k < 4; k++)
        {
            E[k] = src[j * 8 + k] + src[j * 8 + 7 - k];
            O[k] = src[j * 8 + k] - src[j * 8 + 7 - k];
        }
        /* EE and EO */
        EE[0] = E[0] + E[3];
        EO[0] = E[0] - E[3];
        EE[1] = E[1] + E[2];
        EO[1] = E[1] - E[2];

        dst[0 * line + j] = (xeve_tbl_tm8[0][0] * EE[0] + xeve_tbl_tm8[0][1] * EE[1] + add) >> shift;
        dst[4 * line + j] = (xeve_tbl_tm8[4][0] * EE[0] + xeve_tbl_tm8[4][1] * EE[1] + add) >> shift;
        dst[2 * line + j] = (xeve_tbl_tm8[2][0] * EO[0] + xeve_tbl_tm8[2][1] * EO[1] + add) >> shift;
        dst[6 * line + j] = (xeve_tbl_tm8[6][0] * EO[0] + xeve_tbl_tm8[6][1] * EO[1] + add) >> shift;

        dst[1 * line + j] = (xeve_tbl_tm8[1][0] * O[0] + xeve_tbl_tm8[1][1] * O[1] + xeve_tbl_tm8[1][2] * O[2] + xeve_tbl_tm8[1][3] * O[3] + add) >> shift;
        dst[3 * line + j] = (xeve_tbl_tm8[3][0] * O[0] + xeve_tbl_tm8[3][1] * O[1] + xeve_tbl_tm8[3][2] * O[2] + xeve_tbl_tm8[3][3] * O[3] + add) >> shift;
        dst[5 * line + j] = (xeve_tbl_tm8[5][0] * O[0] + xeve_tbl_tm8[5][1] * O[1] + xeve_tbl_tm8[5][2] * O[2] + xeve_tbl_tm8[5][3] * O[3] + add) >> shift;
        dst[7 * line + j] = (xeve_tbl_tm8[7][0] * O[0] + xeve_tbl_tm8[7][1] * O[1] + xeve_tbl_tm8[7][2] * O[2] + xeve_tbl_tm8[7][3] * O[3] + add) >> shift;
    }
}

void tx_pb16(s16 * src, s16 * dst, int shift, int line)
{
    int j, k;
    int E[8], O[8];
    int EE[4], EO[4];
    int EEE[2], EEO[2];
    int add = 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        /* E and O*/
        for (k = 0; k < 8; k++)
        {
            E[k] = src[j * 16 + k] + src[j * 16 + 15 - k];
            O[k] = src[j * 16 + k] - src[j * 16 + 15 - k];
        }
        /* EE and EO */
        for (k = 0; k < 4; k++)
        {
            EE[k] = E[k] + E[7 - k];
            EO[k] = E[k] - E[7 - k];
        }
        /* EEE and EEO */
        EEE[0] = EE[0] + EE[3];
        EEO[0] = EE[0] - EE[3];
        EEE[1] = EE[1] + EE[2];
        EEO[1] = EE[1] - EE[2];

        dst[0 * line + j] = (xeve_tbl_tm16[0][0] * EEE[0] + xeve_tbl_tm16[0][1] * EEE[1] + add) >> shift;
        dst[8 * line + j] = (xeve_tbl_tm16[8][0] * EEE[0] + xeve_tbl_tm16[8][1] * EEE[1] + add) >> shift;
        dst[4 * line + j] = (xeve_tbl_tm16[4][0] * EEO[0] + xeve_tbl_tm16[4][1] * EEO[1] + add) >> shift;
        dst[12 * line + j] = (xeve_tbl_tm16[12][0] * EEO[0] + xeve_tbl_tm16[12][1] * EEO[1] + add) >> shift;

        for (k = 2; k < 16; k += 4)
        {
            dst[k*line + j] = (xeve_tbl_tm16[k][0] * EO[0] + xeve_tbl_tm16[k][1] * EO[1] + xeve_tbl_tm16[k][2] * EO[2] + xeve_tbl_tm16[k][3] * EO[3] + add) >> shift;
        }

        for (k = 1; k < 16; k += 2)
        {
            dst[k*line + j] = (xeve_tbl_tm16[k][0] * O[0] + xeve_tbl_tm16[k][1] * O[1] + xeve_tbl_tm16[k][2] * O[2] + xeve_tbl_tm16[k][3] * O[3] +
                xeve_tbl_tm16[k][4] * O[4] + xeve_tbl_tm16[k][5] * O[5] + xeve_tbl_tm16[k][6] * O[6] + xeve_tbl_tm16[k][7] * O[7] + add) >> shift;
        }
    }
}

void tx_pb32(s16 * src, s16 * dst, int shift, int line)
{
    int j, k;
    int E[16], O[16];
    int EE[8], EO[8];
    int EEE[4], EEO[4];
    int EEEE[2], EEEO[2];
    int add = 1 << (shift - 1);
    for (j = 0; j < line; j++)
    {
        /* E and O*/
        for (k = 0; k < 16; k++)
        {
            E[k] = src[j * 32 + k] + src[j * 32 + 31 - k];
            O[k] = src[j * 32 + k] - src[j * 32 + 31 - k];
        }
        /* EE and EO */
        for (k = 0; k < 8; k++)
        {
            EE[k] = E[k] + E[15 - k];
            EO[k] = E[k] - E[15 - k];
        }
        /* EEE and EEO */
        for (k = 0; k < 4; k++)
        {
            EEE[k] = EE[k] + EE[7 - k];
            EEO[k] = EE[k] - EE[7 - k];
        }
        /* EEEE and EEEO */
        EEEE[0] = EEE[0] + EEE[3];
        EEEO[0] = EEE[0] - EEE[3];
        EEEE[1] = EEE[1] + EEE[2];
        EEEO[1] = EEE[1] - EEE[2];

        dst[0 * line + j] = (xeve_tbl_tm32[0][0] * EEEE[0] + xeve_tbl_tm32[0][1] * EEEE[1] + add) >> shift;
        dst[16 * line + j] = (xeve_tbl_tm32[16][0] * EEEE[0] + xeve_tbl_tm32[16][1] * EEEE[1] + add) >> shift;
        dst[8 * line + j] = (xeve_tbl_tm32[8][0] * EEEO[0] + xeve_tbl_tm32[8][1] * EEEO[1] + add) >> shift;
        dst[24 * line + j] = (xeve_tbl_tm32[24][0] * EEEO[0] + xeve_tbl_tm32[24][1] * EEEO[1] + add) >> shift;
        for (k = 4; k < 32; k += 8)
        {
            dst[k*line + j] = (xeve_tbl_tm32[k][0] * EEO[0] + xeve_tbl_tm32[k][1] * EEO[1] + xeve_tbl_tm32[k][2] * EEO[2] + xeve_tbl_tm32[k][3] * EEO[3] + add) >> shift;
        }
        for (k = 2; k < 32; k += 4)
        {
            dst[k*line + j] = (xeve_tbl_tm32[k][0] * EO[0] + xeve_tbl_tm32[k][1] * EO[1] + xeve_tbl_tm32[k][2] * EO[2] + xeve_tbl_tm32[k][3] * EO[3] +
                xeve_tbl_tm32[k][4] * EO[4] + xeve_tbl_tm32[k][5] * EO[5] + xeve_tbl_tm32[k][6] * EO[6] + xeve_tbl_tm32[k][7] * EO[7] + add) >> shift;
        }
        for (k = 1; k < 32; k += 2)
        {
            dst[k*line + j] = (xeve_tbl_tm32[k][0] * O[0] + xeve_tbl_tm32[k][1] * O[1] + xeve_tbl_tm32[k][2] * O[2] + xeve_tbl_tm32[k][3] * O[3] +
                xeve_tbl_tm32[k][4] * O[4] + xeve_tbl_tm32[k][5] * O[5] + xeve_tbl_tm32[k][6] * O[6] + xeve_tbl_tm32[k][7] * O[7] +
                xeve_tbl_tm32[k][8] * O[8] + xeve_tbl_tm32[k][9] * O[9] + xeve_tbl_tm32[k][10] * O[10] + xeve_tbl_tm32[k][11] * O[11] +
                xeve_tbl_tm32[k][12] * O[12] + xeve_tbl_tm32[k][13] * O[13] + xeve_tbl_tm32[k][14] * O[14] + xeve_tbl_tm32[k][15] * O[15] + add) >> shift;
        }
    }
}

void tx_pb64(s16 *src, s16 *dst, int shift, int line)
{
    const int tx_size = 64;
    const s8 * tm = xeve_tbl_tm64[0];

    int j, k;
    int E[32], O[32];
    int EE[16], EO[16];
    int EEE[8], EEO[8];
    int EEEE[4], EEEO[4];
    int EEEEE[2], EEEEO[2];
    int add = 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        for (k = 0; k < 32; k++)
        {
            E[k] = src[k] + src[63 - k];
            O[k] = src[k] - src[63 - k];
        }
        for (k = 0; k<16; k++)
        {
            EE[k] = E[k] + E[31 - k];
            EO[k] = E[k] - E[31 - k];
        }
        for (k = 0; k<8; k++)
        {
            EEE[k] = EE[k] + EE[15 - k];
            EEO[k] = EE[k] - EE[15 - k];
        }
        for (k = 0; k<4; k++)
        {
            EEEE[k] = EEE[k] + EEE[7 - k];
            EEEO[k] = EEE[k] - EEE[7 - k];
        }
        EEEEE[0] = EEEE[0] + EEEE[3];
        EEEEO[0] = EEEE[0] - EEEE[3];
        EEEEE[1] = EEEE[1] + EEEE[2];
        EEEEO[1] = EEEE[1] - EEEE[2];

        dst[0] = (tm[0 * 64 + 0] * EEEEE[0] + tm[0 * 64 + 1] * EEEEE[1] + add) >> shift;
        dst[16 * line] = (tm[16 * 64 + 0] * EEEEO[0] + tm[16 * 64 + 1] * EEEEO[1] + add) >> shift;

        dst[32 * line] = 0;
        dst[48 * line] = 0;

        for (k = 8; k<64; k += 16)
        {
            if (k > 31)
            {
                dst[k*line] = 0;
            }
            else
            {
                dst[k*line] = (tm[k * 64 + 0] * EEEO[0] + tm[k * 64 + 1] * EEEO[1] + tm[k * 64 + 2] * EEEO[2] + tm[k * 64 + 3] * EEEO[3] + add) >> shift;
            }
        }
        for (k = 4; k<64; k += 8)
        {
            if (k > 31)
            {
                dst[k*line] = 0;
            }
            else
            {
                dst[k*line] = (tm[k * 64 + 0] * EEO[0] + tm[k * 64 + 1] * EEO[1] + tm[k * 64 + 2] * EEO[2] + tm[k * 64 + 3] * EEO[3] +
                    tm[k * 64 + 4] * EEO[4] + tm[k * 64 + 5] * EEO[5] + tm[k * 64 + 6] * EEO[6] + tm[k * 64 + 7] * EEO[7] + add) >> shift;
            }
        }
        for (k = 2; k<64; k += 4)
        {
            if (k > 31)
            {
                dst[k*line] = 0;
            }
            else
            {
                dst[k*line] = (tm[k * 64 + 0] * EO[0] + tm[k * 64 + 1] * EO[1] + tm[k * 64 + 2] * EO[2] + tm[k * 64 + 3] * EO[3] +
                    tm[k * 64 + 4] * EO[4] + tm[k * 64 + 5] * EO[5] + tm[k * 64 + 6] * EO[6] + tm[k * 64 + 7] * EO[7] +
                    tm[k * 64 + 8] * EO[8] + tm[k * 64 + 9] * EO[9] + tm[k * 64 + 10] * EO[10] + tm[k * 64 + 11] * EO[11] +
                    tm[k * 64 + 12] * EO[12] + tm[k * 64 + 13] * EO[13] + tm[k * 64 + 14] * EO[14] + tm[k * 64 + 15] * EO[15] + add) >> shift;
            }
        }
        for (k = 1; k<64; k += 2)
        {
            if (k > 31)
            {
                dst[k*line] = 0;
            }
            else
            {
                dst[k*line] = (tm[k * 64 + 0] * O[0] + tm[k * 64 + 1] * O[1] + tm[k * 64 + 2] * O[2] + tm[k * 64 + 3] * O[3] +
                    tm[k * 64 + 4] * O[4] + tm[k * 64 + 5] * O[5] + tm[k * 64 + 6] * O[6] + tm[k * 64 + 7] * O[7] +
                    tm[k * 64 + 8] * O[8] + tm[k * 64 + 9] * O[9] + tm[k * 64 + 10] * O[10] + tm[k * 64 + 11] * O[11] +
                    tm[k * 64 + 12] * O[12] + tm[k * 64 + 13] * O[13] + tm[k * 64 + 14] * O[14] + tm[k * 64 + 15] * O[15] +
                    tm[k * 64 + 16] * O[16] + tm[k * 64 + 17] * O[17] + tm[k * 64 + 18] * O[18] + tm[k * 64 + 19] * O[19] +
                    tm[k * 64 + 20] * O[20] + tm[k * 64 + 21] * O[21] + tm[k * 64 + 22] * O[22] + tm[k * 64 + 23] * O[23] +
                    tm[k * 64 + 24] * O[24] + tm[k * 64 + 25] * O[25] + tm[k * 64 + 26] * O[26] + tm[k * 64 + 27] * O[27] +
                    tm[k * 64 + 28] * O[28] + tm[k * 64 + 29] * O[29] + tm[k * 64 + 30] * O[30] + tm[k * 64 + 31] * O[31] + add) >> shift;
            }
        }
        src += tx_size;
        dst++;
    }
}

/********************************** DST-VII **********************************/
void xeve_trans_DST7_B4(s16 *block, s16 *coef, s32 shift, s32 line, int skip_line, int skip_line_2)  /* input block, output coef */
{
    int i;
    int rnd_factor = 1 << (shift - 1);
    const s8 *tm = xevem_tbl_tr[DST7][0];
    int c[4];
    s16 *tmp = coef;
    const int reduced_line = line - skip_line;

    for (i = 0; i < reduced_line; i++)
    {
        /* Intermediate Variables */

        c[0] = block[0] + block[3];
        c[1] = block[1] + block[3];
        c[2] = block[0] - block[1];
        c[3] = tm[2] * block[2];

        coef[0] = (tm[0] * c[0] + tm[1] * c[1] + c[3] + rnd_factor) >> shift;
        coef[line] = (tm[2] * (block[0] + block[1] - block[3]) + rnd_factor) >> shift;
        coef[2 * line] = (tm[0] * c[2] + tm[1] * c[0] - c[3] + rnd_factor) >> shift;
        coef[3 * line] = (tm[1] * c[2] - tm[0] * c[1] + c[3] + rnd_factor) >> shift;
        block += 4;
        coef++;
    }

    if (skip_line)
    {
        coef = tmp + reduced_line;
        for (i = 0; i < 4; i++)
        {
            xeve_mset(coef, 0, sizeof(s16)* skip_line);
            coef += line;
        }
    }
}

void xeve_trans_DST7_B8(s16 *block, s16 *coef, s32 shift, s32 line, int skip_line, int skip_line_2)  /* input block, output coef */
{
    int i, j, k, sum;
    int rnd_factor = 1 << (shift - 1);
    const int tr_size = 8;
    const s8 *tm;
    s16 *coef_tmp;
    const int reduced_line = line - skip_line;
    const int cut_off = tr_size - skip_line_2;

    for (i = 0; i < reduced_line; i++)
    {
        coef_tmp = coef;
        tm = xevem_tbl_tr[DST7][1];

        for (j = 0; j < cut_off; j++)
        {
            sum = 0;
            for (k = 0; k < tr_size; k++)
            {
                sum += tm[k] * block[k];
            }
            coef_tmp[i] = (sum + rnd_factor) >> shift;
            coef_tmp += line;
            tm += tr_size;
        }
        block += tr_size;
    }

    if (skip_line)
    {
        coef_tmp = coef + reduced_line;
        for (j = 0; j < cut_off; j++)
        {
            xeve_mset(coef_tmp, 0, sizeof(s16)* skip_line);
            coef_tmp += line;
        }
    }

    if (skip_line_2)
    {
        coef_tmp = coef + line * cut_off;
        xeve_mset(coef_tmp, 0, sizeof(s16)* line * skip_line_2);
    }
}

void xeve_trans_DST7_B16(s16 *block, s16 *coef, s32 shift, s32 line, int skip_line, int skip_line_2)  /* input block, output coef */
{
    int i, j, k, sum;
    int rnd_factor = 1 << (shift - 1);
    const int tr_size = 16;
    const s8 *tm;
    s16 *coef_tmp;
    const int reduced_line = line - skip_line;
    const int cut_off = tr_size - skip_line_2;

    for (i = 0; i < reduced_line; i++)
    {
        coef_tmp = coef;
        tm = xevem_tbl_tr[DST7][2];

        for (j = 0; j < cut_off; j++)
        {
            sum = 0;
            for (k = 0; k < tr_size; k++)
            {
                sum += tm[k] * block[k];
            }
            coef_tmp[i] = (sum + rnd_factor) >> shift;
            coef_tmp += line;
            tm += tr_size;
        }
        block += tr_size;
    }

    if (skip_line)
    {
        coef_tmp = coef + reduced_line;
        for (j = 0; j < cut_off; j++)
        {
            xeve_mset(coef_tmp, 0, sizeof(s16)* skip_line);
            coef_tmp += line;
        }
    }

    if (skip_line_2)
    {
        coef_tmp = coef + line * cut_off;
        xeve_mset(coef_tmp, 0, sizeof(s16)* line * skip_line_2);
    }
}

void xeve_trans_DST7_B32(s16 *block, s16 *coef, s32 shift, s32 line, int skip_line, int skip_line_2)  /* input block, output coef */
{
    int i, j, k, sum;
    int rnd_factor = 1 << (shift - 1);
    const int tr_size = 32;
    const s8 *tm;
    s16 *coef_tmp;
    const int reduced_line = line - skip_line;
    const int cut_off = tr_size - skip_line_2;

    for (i = 0; i < reduced_line; i++)
    {
        coef_tmp = coef;
        tm = xevem_tbl_tr[DST7][3];

        for (j = 0; j < cut_off; j++)
        {
            sum = 0;
            for (k = 0; k < tr_size; k++)
            {
                sum += block[k] * tm[k];
            }
            coef_tmp[i] = (sum + rnd_factor) >> shift;
            tm += tr_size;
            coef_tmp += line;
        }
        block += tr_size;
    }

    if (skip_line)
    {
        coef_tmp = coef + reduced_line;
        for (j = 0; j < cut_off; j++)
        {
            xeve_mset(coef_tmp, 0, sizeof(s16)* skip_line);
            coef_tmp += line;
        }
    }

    if (skip_line_2)
    {
        coef_tmp = coef + line * cut_off;
        xeve_mset(coef_tmp, 0, sizeof(s16)* line * skip_line_2);
    }
}

/********************************** DCT-VIII **********************************/
void xeve_trans_DCT8_B4(s16 *block, s16 *coef, s32 shift, s32 line, int skip_line, int skip_line_2)  /* input block, output coef */
{
    int i;
    int rnd_factor = 1 << (shift - 1);
    const s8 *tm = xevem_tbl_tr[DCT8][0];
    int c[4];
    s16 *tmp = coef;
    const int reduced_line = line - skip_line;

    for (i = 0; i < reduced_line; i++)
    {
        /* Intermediate Variables */
        c[0] = block[0] + block[3];
        c[1] = block[2] + block[0];
        c[2] = block[3] - block[2];
        c[3] = tm[1] * block[1];

        coef[0] = (tm[3] * c[0] + tm[2] * c[1] + c[3] + rnd_factor) >> shift;
        coef[line] = (tm[1] * (block[0] - block[2] - block[3]) + rnd_factor) >> shift;
        coef[2 * line] = (tm[3] * c[2] + tm[2] * c[0] - c[3] + rnd_factor) >> shift;
        coef[3 * line] = (tm[3] * c[1] - tm[2] * c[2] - c[3] + rnd_factor) >> shift;
        block += 4;
        coef++;
    }

    if (skip_line)
    {
        coef = tmp + reduced_line;
        for (i = 0; i < 4; i++)
        {
            xeve_mset(coef, 0, sizeof(s16)* skip_line);
            coef += line;
        }
    }
}

void xeve_trans_DCT8_B8(s16 *block, s16 *coef, s32 shift, s32 line, int skip_line, int skip_line_2)  /* input block, output coef */
{
    int i, j, k, sum;
    int rnd_factor = 1 << (shift - 1);
    const int tr_size = 8;
    const s8 *tm;
    s16 *coef_tmp;
    const int reduced_line = line - skip_line;
    const int cut_off = tr_size - skip_line_2;

    for (i = 0; i < reduced_line; i++)
    {
        coef_tmp = coef;
        tm = xevem_tbl_tr[DCT8][1];

        for (j = 0; j < cut_off; j++)
        {
            sum = 0;
            for (k = 0; k < tr_size; k++)
            {
                sum += tm[k] * block[k];
            }
            coef_tmp[i] = (sum + rnd_factor) >> shift;
            coef_tmp += line;
            tm += tr_size;
        }
        block += tr_size;
    }

    if (skip_line)
    {
        coef_tmp = coef + reduced_line;
        for (j = 0; j < cut_off; j++)
        {
            xeve_mset(coef_tmp, 0, sizeof(s16)* skip_line);
            coef_tmp += line;
        }
    }

    if (skip_line_2)
    {
        coef_tmp = coef + line * cut_off;
        xeve_mset(coef_tmp, 0, sizeof(s16)* line * skip_line_2);
    }
}

void xeve_trans_DCT8_B16(s16 *block, s16 *coef, s32 shift, s32 line, int skip_line, int skip_line_2)  /* input block, output coef */
{
    int i, j, k, sum;
    int rnd_factor = 1 << (shift - 1);
    const int tr_size = 16;
    const s8 *tm;
    s16 *coef_tmp;
    const int reduced_line = line - skip_line;
    const int cut_off = tr_size - skip_line_2;

    for (i = 0; i < reduced_line; i++)
    {
        coef_tmp = coef;
        tm = xevem_tbl_tr[DCT8][2];

        for (j = 0; j < tr_size; j++)
        {
            sum = 0;
            for (k = 0; k < tr_size; k++)
            {
                sum += tm[k] * block[k];
            }
            coef_tmp[i] = (sum + rnd_factor) >> shift;
            coef_tmp += line;
            tm += tr_size;
        }
        block += tr_size;
    }

    if (skip_line)
    {
        coef_tmp = coef + reduced_line;
        for (j = 0; j < cut_off; j++)
        {
            xeve_mset(coef_tmp, 0, sizeof(s16)* skip_line);
            coef_tmp += line;
        }
    }

    if (skip_line_2)
    {
        coef_tmp = coef + line * cut_off;
        xeve_mset(coef_tmp, 0, sizeof(s16)* line * skip_line_2);
    }
}

void xeve_trans_DCT8_B32(s16 *block, s16 *coef, s32 shift, s32 line, int skip_line, int skip_line_2)  /* input block, output coef */
{
    int i, j, k, sum;
    int rnd_factor = 1 << (shift - 1);
    const int tr_size = 32;
    const s8 *tm;
    s16 *coef_tmp;
    const int reduced_line = line - skip_line;
    const int cut_off = tr_size - skip_line_2;

    for (i = 0; i < reduced_line; i++)
    {
        coef_tmp = coef;
        tm = xevem_tbl_tr[DCT8][3];

        for (j = 0; j < cut_off; j++)
        {
            sum = 0;
            for (k = 0; k < tr_size; k++)
            {
                sum += block[k] * tm[k];
            }
            coef_tmp[i] = (sum + rnd_factor) >> shift;
            tm += tr_size;
            coef_tmp += line;
        }
        block += tr_size;
    }

    if (skip_line)
    {
        coef_tmp = coef + reduced_line;
        for (j = 0; j < cut_off; j++)
        {
            xeve_mset(coef_tmp, 0, sizeof(s16)* skip_line);
            coef_tmp += line;
        }
    }

    if (skip_line_2)
    {
        coef_tmp = coef + line * cut_off;
        xeve_mset(coef_tmp, 0, sizeof(s16)* line * skip_line_2);
    }
}

void xeve_t_MxN_ats_intra(s16 *coef, int tuw, int tuh, int bit_depth, u8 ats_intra_mode, u8 ats_intra_tridx)
{
    const int shift_1st = XEVE_LOG2(tuw) - 1 + bit_depth - 8;
    const int shift_2nd = XEVE_LOG2(tuh) + 6;
    const u8 log2_minus1_w = XEVE_LOG2(tuw) - 1;
    const u8 log2_minus1_h = XEVE_LOG2(tuh) - 1;
    s16 t[MAX_TR_DIM]; /* temp buffer */
    u8  t_idx_h = 0, t_idx_v = 0;
    int skip_w = 0;
    int skip_h = 0;

    t_idx_h = xevem_tbl_tr_subset_intra[ats_intra_tridx >> 1];
    t_idx_v = xevem_tbl_tr_subset_intra[ats_intra_tridx & 1];

    xeve_trans_map_tbl[t_idx_h][log2_minus1_w](coef, t, shift_1st, tuh, 0, skip_w);
    xeve_trans_map_tbl[t_idx_v][log2_minus1_h](t, coef, shift_2nd, tuw, skip_w, skip_h);
}


const XEVE_TX xeve_tbl_tx[MAX_TR_LOG2] =
{
    tx_pb2,
    tx_pb4,
    tx_pb8,
    tx_pb16,
    tx_pb32,
    tx_pb64
};

void xeve_trans_ats_intra(s16* coef, int log2_cuw, int log2_cuh, u8 ats_intra_cu, u8 ats_mode, int bit_depth)
{
    xeve_t_MxN_ats_intra(coef, (1 << log2_cuw), (1 << log2_cuh), bit_depth, ats_intra_cu, ats_mode);
}

static void xeve_trans(s16 * coef, int log2_cuw, int log2_cuh, int iqt_flag, int bit_depth)
{    
    int shift1 = xeve_get_transform_shift(log2_cuw, 0,  bit_depth);
    int shift2 = xeve_get_transform_shift(log2_cuh, 1,  bit_depth);

    if (iqt_flag == 1)
    {
        ALIGNED_128(s16 t[MAX_TR_DIM]); /* temp buffer */
        (*xeve_func_tx)[log2_cuw - 1](coef, t, shift1, 1 << log2_cuh);
        (*xeve_func_tx)[log2_cuh - 1](t, coef, shift2, 1 << log2_cuw);
    }
    else
    {
        s32 tb[MAX_TR_DIM]; /* temp buffer */
        (*xeve_func_txb)[log2_cuw - 1](coef, tb, 0, 1 << log2_cuh, 0);
        (*xeve_func_txb)[log2_cuh - 1](tb, coef, (shift1 + shift2), 1 << log2_cuw, 1);
    }
}

__inline static s64 get_rate_positionLastXY(int pos_x, int pos_y, int width, int height, int ch_type, s64 lambda, int sps_cm_init_flag, XEVE_CORE * core)
{
    int group_idx_x;
    int group_idx_y;
    int blk_offset_x, blk_offset_y, shift_x, shift_y;
    int bin, cnt;
    int rate = 0;
    int offset_x = (ch_type == Y_C ? 0 : (sps_cm_init_flag == 1 ? NUM_CTX_LAST_SIG_COEFF_LUMA : 11));
    int offset_y = (ch_type == Y_C ? 0 : (sps_cm_init_flag == 1 ? NUM_CTX_LAST_SIG_COEFF_LUMA : 11));

    group_idx_x = xeve_group_idx[pos_x];
    group_idx_y = xeve_group_idx[pos_y];
    if (sps_cm_init_flag == 1)
    {
        xeve_get_ctx_last_pos_xy_para(ch_type, width, height, &blk_offset_x, &blk_offset_y, &shift_x, &shift_y);
    }
    else
    {
        blk_offset_x = 0;
        blk_offset_y = 0;
        shift_x = 0;
        shift_y = 0;
    }
    //------------------

    // pos_x

    for (bin = 0; bin < group_idx_x; bin++)
    {
        rate += core->rdoq_est_last_sig_coeff_x[offset_x + blk_offset_x + (bin >> shift_x)][1];
    }
    if (group_idx_x < xeve_group_idx[width - 1])
    {
        rate += core->rdoq_est_last_sig_coeff_x[offset_x + blk_offset_x + (bin >> shift_x)][0];
    }

    // pos_y

    for (bin = 0; bin < group_idx_y; bin++)
    {
        rate += core->rdoq_est_last_sig_coeff_y[offset_y + blk_offset_y + (bin >> shift_y)][1];
    }
    if (group_idx_y < xeve_group_idx[height - 1])
    {
        rate += core->rdoq_est_last_sig_coeff_y[offset_y + blk_offset_y + (bin >> shift_y)][0];
    }

    // EP-coded part

    if (group_idx_x > 3)
    {
        cnt = (group_idx_x - 2) >> 1;
        pos_x = pos_x - xeve_min_in_group[group_idx_x];
        rate += (cnt * GET_IEP_RATE);
    }
    if (group_idx_y > 3)
    {
        cnt = (group_idx_y - 2) >> 1;
        pos_y = pos_y - xeve_min_in_group[group_idx_y];
        rate += (cnt * GET_IEP_RATE);
    }

    return GET_I_COST(rate, lambda);
}

__inline static s64 get_rate_sig_coeff(int significance, int ctx_sig_coeff, s64 lambda, XEVE_CORE * core
)
{    s64 rate = core->rdoq_est_sig_coeff[ctx_sig_coeff][significance];
    return GET_I_COST(rate, lambda);
}


__inline static int get_ic_rate(int abs_level, int ctx_gtA, int ctx_gtB, int rparam, int c1_idx, int c2_idx, int num_gtA, int num_gtB, XEVE_CORE * core)
{
    int rate = GET_IEP_RATE; // cost of sign bit
    int base_level = (c1_idx < num_gtA) ? (2 + (c2_idx < num_gtB ? 1 : 0)) : 1;

    if (abs_level >= base_level)
    {
        int symbol = abs_level - base_level;
        int length;

        if (symbol < (xeve_go_rice_range[rparam] << rparam))
        {
            length = symbol >> rparam;
            rate += (length + 1 + rparam) << 15;
        }
        else
        {
            length = rparam;
            symbol = symbol - (xeve_go_rice_range[rparam] << rparam);
            while (symbol >= (1 << length))
            {
                symbol -= (1 << (length++));
            }
            rate += (xeve_go_rice_range[rparam] + length + 1 - rparam + length) << 15;
        }

        if (c1_idx < num_gtA)
        {
            rate += core->rdoq_est_gtx[ctx_gtA][1];

            if (c2_idx < num_gtB)
            {
                rate += core->rdoq_est_gtx[ctx_gtB][1];
            }
        }
    }
    else if (abs_level == 1)
    {
        rate += core->rdoq_est_gtx[ctx_gtA][0];
    }
    else if (abs_level == 2)
    {
        rate += core->rdoq_est_gtx[ctx_gtA][1];
        rate += core->rdoq_est_gtx[ctx_gtB][0];
    }
    else
    {
        rate = 0;
    }

    return  rate;
}

__inline static int get_coded_level(
    s64*    rd_coded_cost,          //< reference to coded cost
    s64*    rd_coded_cost0,         //< reference to cost when coefficient is 0
    s64*    rd_coded_cost_sig,       //< rd_coded_cost_sig reference to cost of significant coefficient
    s64     level_double,           //< reference to unscaled quantized level
    int     max_abs_level,          //< scaled quantized level
    int     ctx_sig_coeff,          //< current ctxInc for coeff_abs_significant_flag
    int     ctx_gtA,          //< current ctxInc for coeff_abs_level_greater1 
    int     ctx_gtB,          //< current ctxInc for coeff_abs_level_greater2 
    int     rparam,          //< current Rice parameter for coeff_abs_level_minus3
    int     c1_idx,                  //< 
    int     c2_idx,                  //< 
    int     num_gtA,
    int     num_gtB,
    int     qbits,                 //< quantization step size
    s64     error_scale,             //< 
    s64     lambda,
    int     bypass_sigmap,
    XEVE_CORE * core
)
{
    s64 curr_cost_sig = 0;
    s64 curr_cost;
    int best_abs_level = 0;
    int min_abs_level;
    int abs_level;
    int rate_best = 0;
    int rate_max = 0;
    int rate = 0;

    if (bypass_sigmap == 0 && max_abs_level < 3)
    {
        *rd_coded_cost_sig = get_rate_sig_coeff(0, ctx_sig_coeff, lambda, core);
        *rd_coded_cost = *rd_coded_cost0 + *rd_coded_cost_sig;

        if (max_abs_level == 0)
        {
            return best_abs_level;
        }
    }
    else
    {
        *rd_coded_cost = XEVE_INT64_MAX;
    }

    if (bypass_sigmap == 0)
    {
        curr_cost_sig = get_rate_sig_coeff(1, ctx_sig_coeff, lambda, core);
    }

    min_abs_level = (max_abs_level > 1 ? max_abs_level - 1 : 1);
    for (abs_level = max_abs_level; abs_level >= min_abs_level; abs_level--)
    {
        s64 err = (s64)(level_double - ((s64)abs_level << qbits));
        rate = get_ic_rate(abs_level, ctx_gtA, ctx_gtB, rparam, c1_idx, c2_idx, num_gtA, num_gtB, core);
        err = (err * error_scale) >> ERR_SCALE_PRECISION_BITS;
        curr_cost = err * err + GET_I_COST(rate, lambda);
        curr_cost += curr_cost_sig;

        if (curr_cost < *rd_coded_cost)
        {
            best_abs_level = abs_level;
            *rd_coded_cost = curr_cost;
            *rd_coded_cost_sig = curr_cost_sig;
            rate_best = rate;
        }
        if (abs_level == max_abs_level)
        {
            rate_max = rate;
        }
    }
    return best_abs_level;
}

__inline static int get_ctx_sig_coeff_inc_rdoq(s16 *pcoeff, int blkpos, int width, int height, int ch_type, int *num1, int *num2)
{
    const s16 *pdata = pcoeff + blkpos;
    const int width_m1 = width - 1;
    const int height_m1 = height - 1;
    const int log2_w = XEVE_LOG2(width);
    const int pos_y = blkpos >> log2_w;
    const int pos_x = blkpos - (pos_y << log2_w);
    const int diag = pos_x + pos_y;
    int num_sig_coeff = 0;
    int num_gtA = 0;
    int num_gtB = 0;
    int ctx_idx;
    int ctx_ofs;
    s16 tmp;

    if (pos_x < width_m1)
    {
        tmp = XEVE_ABS16(pdata[1]);
        num_sig_coeff += !!(tmp);
        num_gtA += (tmp > 1 ? 1 : 0);
        num_gtB += (tmp > 2 ? 1 : 0);
        if (pos_x < width_m1 - 1)
        {
            tmp = XEVE_ABS16(pdata[2]);
            num_sig_coeff += !!(tmp);
            num_gtA += (tmp > 1 ? 1 : 0);
            num_gtB += (tmp > 2 ? 1 : 0);
        }
        if (pos_y < height_m1)
        {
            tmp = XEVE_ABS16(pdata[width + 1]);
            num_sig_coeff += !!(tmp);
            num_gtA += (tmp > 1 ? 1 : 0);
            num_gtB += (tmp > 2 ? 1 : 0);
        }
    }
    if (pos_y < height_m1)
    {
        tmp = XEVE_ABS16(pdata[width]);
        num_sig_coeff += !!(tmp);
        num_gtA += (tmp > 1 ? 1 : 0);
        num_gtB += (tmp > 2 ? 1 : 0);
        if (pos_y < height_m1 - 1)
        {
            tmp = XEVE_ABS16(pdata[2 * width]);
            num_sig_coeff += !!(tmp);
            num_gtA += (tmp > 1 ? 1 : 0);
            num_gtB += (tmp > 2 ? 1 : 0);
        }
    }

    ctx_idx = XEVE_MIN(num_sig_coeff, 4) + 1;

    if (diag < 2)
    {
        ctx_idx = XEVE_MIN(ctx_idx, 2);
    }

    if (ch_type == Y_C)
    {
        ctx_ofs = diag < 2 ? 0 : (diag < 5 ? 2 : 7);
    }
    else
    {
        ctx_ofs = diag < 2 ? 0 : 2;
    }

    *num1 = XEVE_MIN(num_gtA, 3) + 1;
    *num2 = XEVE_MIN(num_gtB, 3) + 1;
    if (ch_type == Y_C)
    {
        *num1 += (diag < 3) ? 0 : ((diag < 10) ? 4 : 8);
        *num2 += (diag < 3) ? 0 : ((diag < 10) ? 4 : 8);
    }

    return ctx_ofs + ctx_idx;
}

int xeve_rdoq_method_adcc(u8 qp, double d_lambda, u8 is_intra, s16 *src_coef, s16 *dst_tmp, int log2_cuw, int log2_cuh, int ch_type, int sps_cm_init_flag, XEVE_CORE * core, int bit_depth)
{
    const int ns_shift = ((log2_cuw + log2_cuh) & 1) ? 7 : 0;
    const int ns_scale = ((log2_cuw + log2_cuh) & 1) ? 181 : 1;
    const int qp_rem = qp % 6;
    const int q_value = (xeve_quant_scale[core->ctx->param.tool_iqt][qp_rem] * ns_scale) >> ns_shift;
    const int log2_size = (log2_cuw + log2_cuh) >> 1;
    const int tr_shift = MAX_TX_DYNAMIC_RANGE - bit_depth - (log2_size);

    s64 err_scale = core->ctx->err_scale[qp_rem][log2_size - 1];
    s64 lambda = (s64)(d_lambda * (double)(1 << SCALE_BITS) + 0.5);
    int q_bits;
    const int width = (1 << log2_cuw);
    const int height = (1 << log2_cuh);
    const int max_num_coef = width * height;
    int scan_type = COEF_SCAN_ZIGZAG;
    int log2_block_size = XEVE_MIN(log2_cuw, log2_cuh);
    const u16 *scan;
    int scan_pos_last = -1;
    int ipos;
    int cg_log2_size = LOG2_CG_SIZE;
    int cg_size = 1 << cg_log2_size;
    int last_scan_set;
    int sub_set;

    int offset1 = (sps_cm_init_flag == 1) ? ((ch_type == Y_C) ? 0 : NUM_CTX_GTX_LUMA) : ((ch_type == Y_C) ? 0 : 1);
    int offset0 = (sps_cm_init_flag == 1) ? ((ch_type == Y_C) ? (log2_block_size <= 2 ? 0 : NUM_CTX_SIG_COEFF_LUMA_TU << (XEVE_MIN(1, (log2_block_size - 3)))) : NUM_CTX_SIG_COEFF_LUMA) : (ch_type == Y_C ? 0 : 1);
    int c1_idx = 0;
    int c2_idx = 0;
    s64 cost_base = 0;
    s64 cost_best = 0;
    int best_last_idx_p1 = 0;
    int found_last = 0;
    s64 cbf_cost = 0;
    int nnz = 0;
    int rice_param = 0;
    s64 dcost_block_uncoded = 0;
    s64 pdcost_coeff[MAX_TR_DIM];
    s64 pdcost_sig[MAX_TR_DIM];
    s64 pdcost_coeff0[MAX_TR_DIM];
    int sig_rate_delta[MAX_TR_DIM];
    int delta_u[MAX_TR_DIM];
    s16 coef_dst[MAX_TR_DIM];
    
    int sum_all = 0;
    int blk_pos;
    s64 tmp_level_double[MAX_TR_DIM];

    int num_nz = 0;
    int is_last_x = 0;
    int is_last_y = 0;
    int is_last_nz = 0;
    int num_gtA, num_gtB;

    s64 sig_last_cost[MAX_TR_DIM];
    s64 sig_last_cost0[MAX_TR_DIM];
    s64 sig_cost_delta[MAX_TR_DIM];
    int last_pos_in_scan = -1;
    int numNonZeroCoefs = 0;
    int last_pos_in_raster_from_scan = -1;
    int scan_pos = 0;
    q_bits = QUANT_SHIFT + tr_shift + (qp / 6);
    scan = xeve_tbl_scan[log2_cuw - 1][log2_cuh - 1];

    for (scan_pos = 0; scan_pos < max_num_coef; scan_pos++)
    {
        int max_abs_level;
        s64 err;
        s64 temp_level;
        int level_double;
        blk_pos = scan[scan_pos];
        temp_level = ((s64)XEVE_ABS(src_coef[blk_pos]) * (s64)q_value);
        level_double = (int)XEVE_MIN(((s64)temp_level), (s64)XEVE_INT32_MAX - (s64)(1 << (q_bits - 1)));
        tmp_level_double[blk_pos] = (s64)level_double;
        max_abs_level = XEVE_MIN(MAX_TX_VAL, ((level_double + ((int)1 << (q_bits - 1))) >> q_bits));
        err = (s64)level_double;
        err = (err * err_scale) >> ERR_SCALE_PRECISION_BITS;
        pdcost_coeff0[blk_pos] = err * err;
        dcost_block_uncoded += pdcost_coeff0[blk_pos];
        coef_dst[blk_pos] = (s16)max_abs_level;
        sum_all += max_abs_level;

        if(max_abs_level != 0)
        {
            num_nz++;
            last_pos_in_scan = scan_pos;
            last_pos_in_raster_from_scan = blk_pos;
        }
    }
    if(sum_all == 0)
    {
        xeve_mset(dst_tmp, 0, sizeof(s16) * max_num_coef);
        return 0;
    }
    
    last_scan_set = last_pos_in_scan >> cg_log2_size;
    scan_pos_last = last_pos_in_raster_from_scan;
    num_gtA = CAFLAG_NUMBER;
    num_gtB = CBFLAG_NUMBER;
    rice_param = 0;

    ipos = last_pos_in_scan;

    cost_base = dcost_block_uncoded;

    for(sub_set = last_scan_set; sub_set >= 0; sub_set--)
    {
        int sub_pos = sub_set << cg_log2_size;

        c1_idx = 0;
        c2_idx = 0;

        for(; ipos >= sub_pos; ipos--)
        {
            //===== coefficient level estimation =====
            int  level;
            int  ctx_sig_coeff = 0;
            int  ctx_gtA = 0;
            int  ctx_gtB = 0;

            blk_pos = scan[ipos];
            {
                s64 level_double = tmp_level_double[blk_pos];
                int max_abs_level = coef_dst[blk_pos];
                int bypass_sigmap = blk_pos == scan_pos_last ? 1 : 0;
                int base_level = (c1_idx < num_gtA) ? (2 + (c2_idx < num_gtB ? 1 : 0)) : 1;
                if (sps_cm_init_flag == 1)
                {
                    ctx_sig_coeff = get_ctx_sig_coeff_inc_rdoq(coef_dst, blk_pos, width, height, ch_type, &ctx_gtA, &ctx_gtB);
                }

                ctx_sig_coeff += offset0;
                if (max_abs_level != 0 && is_last_nz == 0)
                {
                    ctx_gtA = 0;
                    ctx_gtB = 0;
                }
                ctx_gtA += offset1;
                ctx_gtB += offset1;
                rice_param = get_rice_para(coef_dst, blk_pos, width, height, base_level);
                level = get_coded_level(&pdcost_coeff[blk_pos], &pdcost_coeff0[blk_pos], &pdcost_sig[blk_pos], level_double, max_abs_level, ctx_sig_coeff, ctx_gtA, ctx_gtB, rice_param,
                                        c1_idx, c2_idx, num_gtA, num_gtB, q_bits, err_scale, lambda, bypass_sigmap, core);
                
                sig_rate_delta[blk_pos] = core->rdoq_est_sig_coeff[ctx_sig_coeff][1] - core->rdoq_est_sig_coeff[ctx_sig_coeff][0];
                delta_u[blk_pos] = (int)((level_double - (((s64)level) << q_bits)) >> (q_bits - 8));
                sig_cost_delta[blk_pos] = GET_I_COST(sig_rate_delta[blk_pos], lambda);
                sig_last_cost[blk_pos] = GET_I_COST(core->rdoq_est_sig_coeff[offset0][!!(level)], lambda);
                sig_last_cost0[blk_pos] = GET_I_COST(core->rdoq_est_sig_coeff[offset0][0], lambda);
                coef_dst[blk_pos] = (s16)level;
                
                if(level > 0)
                {
                    if (is_last_nz == 0)
                    {
                        is_last_nz = 1;
                    }

                    c1_idx++;
                    if(level > 1)
                    {
                        c2_idx++;
                    }
                }
                else if(max_abs_level)
                {
                    num_nz--;
                    if(num_nz == 0)
                    {
                        xeve_mset(dst_tmp, 0, sizeof(s16) * max_num_coef);
                        return 0;
                    }
                }
            }
        }
    }

    if(num_nz == 0)
    {
        xeve_mset(dst_tmp, 0, sizeof(s16) * max_num_coef);
        return 0;
    }

    {
        s64 in_sr_cost0 = 0;
        s64 in_sr_cost = 0;

        cost_base = 0;

        for (ipos = last_pos_in_scan; ipos >= 0; ipos--)
        {
            blk_pos = scan[ipos];
            in_sr_cost += pdcost_coeff[blk_pos];
            in_sr_cost0 += pdcost_coeff0[blk_pos];
        }

        cost_base = dcost_block_uncoded - in_sr_cost0 + in_sr_cost;
    }

    cost_best = 0;

    if(is_intra == 0 && ch_type == Y_C)
    {
        cost_best = dcost_block_uncoded + GET_I_COST(core->rdoq_est_cbf_all[0], lambda);
        cbf_cost = GET_I_COST(core->rdoq_est_cbf_all[1], lambda);
        cost_base += cbf_cost;
    }
    else
    {
        if(ch_type == Y_C)
        {
            cost_best = dcost_block_uncoded + GET_I_COST(core->rdoq_est_cbf_luma[0], lambda);
            cbf_cost = GET_I_COST(core->rdoq_est_cbf_luma[1], lambda);
        }
        else if (ch_type == U_C)
        {
            cost_best = dcost_block_uncoded + GET_I_COST(core->rdoq_est_cbf_cb[0], lambda);
            cbf_cost = GET_I_COST(core->rdoq_est_cbf_cb[1], lambda);
        }
        else if (ch_type == V_C)
        {
            cost_best = dcost_block_uncoded + GET_I_COST(core->rdoq_est_cbf_cr[0], lambda);
            cbf_cost = GET_I_COST(core->rdoq_est_cbf_cr[1], lambda);
        }
        cost_base += cbf_cost;
    }
    
    best_last_idx_p1 = 0;
    found_last = 0;
    for (ipos = last_pos_in_scan; ipos >= 0; ipos--)
    {
        blk_pos = scan[ipos];
        if (coef_dst[blk_pos] > 0)
        {
            u32 pos_y = blk_pos >> log2_cuw;
            u32 pos_x = blk_pos - (pos_y << log2_cuw);

            s64 cost_last = get_rate_positionLastXY(pos_x, pos_y, width, height, ch_type, lambda, sps_cm_init_flag, core);
            s64 total_cost = cost_base + cost_last - pdcost_sig[blk_pos];

            if (total_cost < cost_best)
            {
                best_last_idx_p1 = ipos + 1;
                cost_best = total_cost;
            }
            if (coef_dst[blk_pos] > 1)
            {
                found_last = 1;
                break;
            }
            cost_base -= pdcost_coeff[blk_pos];
            cost_base += pdcost_coeff0[blk_pos];
        }
        else
        {
            cost_base -= pdcost_sig[blk_pos];
        }
    }

    nnz = 0;
    for (ipos = 0; ipos < best_last_idx_p1; ipos++)
    {
        u32 blk_pos = scan[ipos];
        s16 level = coef_dst[blk_pos];
        dst_tmp[blk_pos] = (src_coef[blk_pos] < 0) ? -level : level;
        nnz += !!(level);
    }

    //===== clean uncoded coefficients =====
    for (ipos = best_last_idx_p1; ipos < max_num_coef; ipos++)
    {
        dst_tmp[scan[ipos]] = 0;
    }
    return nnz;
}

static int xeve_quant_nnz(u8 qp, double lambda, int is_intra, s16 * coef, int log2_cuw, int log2_cuh, u16 scale, int ch_type
                        , int slice_type, int sps_cm_init_flag, int tool_adcc, XEVE_CORE * core, int bit_depth, int use_rdoq)
{
    int nnz = 0;

    if(use_rdoq)
    {
        s64 lev;
        s64 offset;
        int i;
        int shift;
        int tr_shift;
        int log2_size = (log2_cuw + log2_cuh) >> 1;
        const int ns_shift = ((log2_cuw + log2_cuh) & 1) ? 7 : 0;
        const int ns_scale = ((log2_cuw + log2_cuh) & 1) ? 181 : 1;
        s64 zero_coeff_threshold;
        BOOL is_coded = 0;

        tr_shift = MAX_TX_DYNAMIC_RANGE - bit_depth - log2_size + ns_shift;
        shift = QUANT_SHIFT + tr_shift + (qp / 6);

#define FAST_RDOQ_INTRA_RND_OFST  201 //171
#define FAST_RDOQ_INTER_RND_OFST  153 //85
        offset = (s64)((slice_type == SLICE_I) ? FAST_RDOQ_INTRA_RND_OFST : FAST_RDOQ_INTER_RND_OFST) << (s64)(shift - 9);
        zero_coeff_threshold = ((s64)1 << (s64)shift) - offset;

        for(i = 0; i < (1 << (log2_cuw + log2_cuh)); i++)
        {
            lev = (s64)XEVE_ABS(coef[i]) * (s64)scale * ns_scale;
            if(lev >= zero_coeff_threshold)
            {
                is_coded = 1;
                break;
            }
        }

        if(!is_coded)
        {
            xeve_mset(coef, 0, sizeof(coef[0])*((s64)1 << (log2_cuw + log2_cuh)));
            return nnz;
        }
    }

    if(use_rdoq)
    {
        if (tool_adcc)
        {
            nnz = xeve_rdoq_method_adcc(qp, lambda, is_intra, coef, coef, log2_cuw, log2_cuh, ch_type, sps_cm_init_flag, core,  bit_depth);
        }
        else
        {
            nnz = xeve_rdoq_run_length_cc(qp, lambda, is_intra, coef, coef, log2_cuw, log2_cuh, ch_type, core, bit_depth);
        }
    }
    else
    {
        s64 lev;
        s64 offset;
        int sign;
        int i;
        int shift;
        int tr_shift;
        int log2_size = (log2_cuw + log2_cuh) >> 1;
        const int ns_shift = ((log2_cuw + log2_cuh) & 1) ? 7 : 0;
        const int ns_scale = ((log2_cuw + log2_cuh) & 1) ? 181 : 1;

        tr_shift = MAX_TX_DYNAMIC_RANGE - bit_depth - log2_size + ns_shift;
        shift = QUANT_SHIFT + tr_shift + (qp / 6);
        offset = (s64)((slice_type == SLICE_I) ? 171 : 85) << (s64)(shift - 9);

        for(i = 0; i < (1 << (log2_cuw + log2_cuh)); i++)
        {
            sign = XEVE_SIGN_GET(coef[i]);
            lev = (s64)XEVE_ABS(coef[i]) * (s64)scale;
            lev = (s16)(((s64)lev * ns_scale + offset) >> shift);
            coef[i] = (s16)XEVE_SIGN_SET(lev, sign);
            nnz += !!(coef[i]);
        }
    }

    return nnz;
}


static int xeve_tq_nnz(u8 qp, double lambda, s16 * coef, int log2_cuw, int log2_cuh, u16 scale, int slice_type, int ch_type, int is_intra, int sps_cm_init_flag, int iqt_flag
                     , u8 ats_intra_cu, u8 ats_mode, int tool_adcc, XEVE_CORE * core, int bit_depth, int rdoq)
{
    if (ats_intra_cu)
    {
        xeve_trans_ats_intra(coef, log2_cuw, log2_cuh, ats_intra_cu, ats_mode, bit_depth);
    }
    else
    {
        xeve_trans(coef, log2_cuw, log2_cuh, iqt_flag,  bit_depth);
    }

    return xeve_quant_nnz(qp, lambda, is_intra, coef, log2_cuw, log2_cuh, scale, ch_type, slice_type, sps_cm_init_flag, tool_adcc, core, bit_depth, rdoq);
}

int xevem_rdoq_set_ctx_cc(XEVE_CORE * core, int ch_type, int prev_level)
{
    return core->ctx->sps.tool_cm_init == 1 ? ((XEVE_MIN(prev_level - 1, 5)) << 1) + (ch_type == Y_C ? 0 : 12) : (ch_type == Y_C ? 0 : 2);
}

int xevem_sub_block_tq(XEVE_CTX * ctx, XEVE_CORE * core, s16 coef[N_C][MAX_CU_DIM], int log2_cuw, int log2_cuh, int slice_type, int nnz[N_C], int is_intra, int run_stats)
{
    XEVEM_CORE *mcore = (XEVEM_CORE*)core;
    run_stats = xeve_get_run(run_stats, core->tree_cons);
    int run[N_C] = {run_stats & 1, (run_stats >> 1) & 1, (run_stats >> 2) & 1};
    s16 *coef_temp[N_C];
    s16 coef_temp_buf[N_C][MAX_TR_DIM];
    int i, j, c;
    int log2_w_sub = (log2_cuw > MAX_TR_LOG2) ? MAX_TR_LOG2 : log2_cuw;
    int log2_h_sub = (log2_cuh > MAX_TR_LOG2) ? MAX_TR_LOG2 : log2_cuh;
    int loop_w     = (log2_cuw > MAX_TR_LOG2) ? (1 << (log2_cuw - MAX_TR_LOG2)) : 1;
    int loop_h     = (log2_cuh > MAX_TR_LOG2) ? (1 << (log2_cuh - MAX_TR_LOG2)) : 1;
    int w_shift    = ctx->param.cs_w_shift;
    int h_shift    = ctx->param.cs_h_shift;
    int stride     = (1 << log2_cuw);
    int sub_stride = (1 << log2_w_sub);
    u8  qp[N_C]    = { core->qp_y, core->qp_u, core->qp_v };
    double lambda[N_C] = { core->lambda[0], core->lambda[1], core->lambda[2] };
    int nnz_temp[N_C]  = {0};
    u8 ats_intra_cu_on = 0;
    u8 ats_mode_idx    = 0;

    xeve_mset(core->nnz_sub, 0, sizeof(int) * N_C * MAX_SUB_TB_NUM);

    if (mcore->ats_inter_info)
    {
        get_tu_size(mcore->ats_inter_info, log2_cuw, log2_cuh, &log2_w_sub, &log2_h_sub);
        sub_stride = (1 << log2_w_sub);
    }

    for(j = 0; j < loop_h; j++)
    {
        for(i = 0; i < loop_w; i++)
        {
            for(c = 0; c < N_C; c++)
            {
                ats_intra_cu_on = (c == 0)? mcore->ats_intra_cu : 0;
                ats_mode_idx = (c == 0) ? mcore->ats_mode : 0;

                if (c == 0)
                {
                    get_ats_inter_trs(mcore->ats_inter_info, log2_cuw, log2_cuh, &ats_intra_cu_on, &ats_mode_idx);
                }

                if(run[c])
                {
                    int pos_sub_x = c == 0 ? (i * (1 << (log2_w_sub))) : (i * (1 << (log2_w_sub - w_shift)));
                    int pos_sub_y = c == 0 ? j * (1 << (log2_h_sub)) * (stride) : j * (1 << (log2_h_sub - h_shift)) * (stride >> w_shift);

                    if(loop_h + loop_w > 2)
                    {
                        if(c == 0)
                            xeve_block_copy(coef[c] + pos_sub_x + pos_sub_y, stride, coef_temp_buf[c], sub_stride, log2_w_sub, log2_h_sub);
                        else
                            xeve_block_copy(coef[c] + pos_sub_x + pos_sub_y, stride >> w_shift, coef_temp_buf[c], sub_stride >> w_shift, log2_w_sub - w_shift, log2_h_sub - h_shift);
                        coef_temp[c] = coef_temp_buf[c];
                    }
                    else
                    {
                        coef_temp[c] = coef[c];
                    }

                    int scale = xeve_quant_scale[ctx->param.tool_iqt][qp[c] % 6];
                    if(c == 0)
                    {
                    core->nnz_sub[c][(j << 1) | i] = xeve_tq_nnz(qp[c], lambda[c], coef_temp[c], log2_w_sub - !!c, log2_h_sub - !!c, scale, slice_type, c, is_intra, ctx->sps.tool_cm_init, ctx->sps.tool_iqt
                                                               , ats_intra_cu_on, ats_mode_idx, ctx->sps.tool_adcc, core, ctx->sps.bit_depth_luma_minus8 + 8, ctx->param.rdoq);
                    }
                    else
                    {
                        core->nnz_sub[c][(j << 1) | i] = xeve_tq_nnz(qp[c], lambda[c], coef_temp[c], log2_w_sub - w_shift, log2_h_sub - h_shift, scale, slice_type, c, is_intra, ctx->sps.tool_cm_init, ctx->sps.tool_iqt
                            , ats_intra_cu_on, ats_mode_idx, ctx->sps.tool_adcc, core, ctx->sps.bit_depth_luma_minus8 + 8, ctx->param.rdoq);
                    }
                    nnz_temp[c] += core->nnz_sub[c][(j << 1) | i];

                    if(loop_h + loop_w > 2)
                    {
                        if(c == 0)
                        {
                        xeve_block_copy(coef_temp_buf[c], sub_stride >> (!!c), coef[c] + pos_sub_x + pos_sub_y, stride >> (!!c), log2_w_sub - (!!c), log2_h_sub - (!!c));
                        }
                        else
                        {
                            xeve_block_copy(coef_temp_buf[c], sub_stride >> w_shift, coef[c] + pos_sub_x + pos_sub_y, stride >> w_shift, log2_w_sub - w_shift, log2_h_sub - h_shift);
                        }
                    }
                }
            }
        }
    }

    for(c = 0; c < N_C; c++)
    {
        nnz[c] = run[c] ? nnz_temp[c] : 0;
    }

    return (nnz[Y_C] + nnz[U_C] + nnz[V_C]);
}

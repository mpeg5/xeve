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

#include "xeve_tq.h"
#include <math.h>

#define QUANT(c, scale, offset, shift) ((s16)((((c)*(scale)) + (offset)) >> (shift)))

const XEVE_TXB(*xeve_func_txb)[MAX_TR_LOG2];
const int xeve_quant_scale[2][6] = { {26214, 23302, 20560, 18396, 16384, 14764},
                                     {26214, 23302, 20560, 18396, 16384, 14564} };

void tx_pb2b(void * src, void * dst, int shift, int line, int step)
{
    int j;
    s64 E, O;
    int add = shift == 0 ? 0 : 1 << (shift - 1);

#define RUN_TX_PB2(src, dst, type_src, type_dst) \
    for (j = 0; j < line; j++)\
    {\
        /* E and O */\
        E = *((type_src * )src + j * 2 + 0) + *((type_src *)src + j * 2 + 1);\
        O = *((type_src * )src + j * 2 + 0) - *((type_src *)src + j * 2 + 1);\
        \
        *((type_dst * )dst + 0 * line + j) = (type_dst)((xeve_tbl_tm2[0][0] * E + add) >> shift);\
        *((type_dst * )dst + 1 * line + j) = (type_dst)((xeve_tbl_tm2[1][0] * O + add) >> shift);\
    }

    if(step == 0)
    {
        RUN_TX_PB2(src, dst, s16, s32);
    }
    else
    {
        RUN_TX_PB2(src, dst, s32, s16);
    }
}

void tx_pb4b(void * src, void * dst, int shift, int line, int step)
{
    int j;
    s64 E[2], O[2];
    int add = shift == 0 ? 0 : 1 << (shift - 1);

#define RUN_TX_PB4(src, dst, type_src, type_dst) \
    for (j = 0; j < line; j++)\
    {\
        /* E and O */\
        E[0] = *((type_src * )src + j * 4 + 0) + *((type_src * )src + j * 4 + 3);\
        O[0] = *((type_src * )src + j * 4 + 0) - *((type_src * )src + j * 4 + 3);\
        E[1] = *((type_src * )src + j * 4 + 1) + *((type_src * )src + j * 4 + 2);\
        O[1] = *((type_src * )src + j * 4 + 1) - *((type_src * )src + j * 4 + 2);\
        \
        *((type_dst * )dst + 0 * line + j) = (type_dst)((xeve_tbl_tm4[0][0] * E[0] + xeve_tbl_tm4[0][1] * E[1] + add) >> shift);\
        *((type_dst * )dst + 2 * line + j) = (type_dst)((xeve_tbl_tm4[2][0] * E[0] + xeve_tbl_tm4[2][1] * E[1] + add) >> shift);\
        *((type_dst * )dst + 1 * line + j) = (type_dst)((xeve_tbl_tm4[1][0] * O[0] + xeve_tbl_tm4[1][1] * O[1] + add) >> shift);\
        *((type_dst * )dst + 3 * line + j) = (type_dst)((xeve_tbl_tm4[3][0] * O[0] + xeve_tbl_tm4[3][1] * O[1] + add) >> shift);\
    }

    if(step == 0)
    {
        RUN_TX_PB4(src, dst, s16, s32);
    }
    else
    {
        RUN_TX_PB4(src, dst, s32, s16);
    }
}

void tx_pb8b(void * src, void * dst, int shift, int line, int step)
{
    int j, k;
    s64 E[4], O[4];
    s64 EE[2], EO[2];
    int add = shift == 0 ? 0 : 1 << (shift - 1);

#define RUN_TX_PB8(src, dst, type_src, type_dst) \
    for (j = 0; j < line; j++)\
    {\
        /* E and O*/\
        for (k = 0; k < 4; k++)\
        {\
            E[k] = *((type_src * )src + j * 8 + k) + *((type_src * )src + j * 8 + 7 - k);\
            O[k] = *((type_src * )src + j * 8 + k) - *((type_src * )src + j * 8 + 7 - k);\
        }\
        /* EE and EO */\
        EE[0] = E[0] + E[3];\
        EO[0] = E[0] - E[3];\
        EE[1] = E[1] + E[2];\
        EO[1] = E[1] - E[2];\
        \
        *((type_dst * )dst + 0 * line + j) = (type_dst)((xeve_tbl_tm8[0][0] * EE[0] + xeve_tbl_tm8[0][1] * EE[1] + add) >> shift);\
        *((type_dst * )dst + 4 * line + j) = (type_dst)((xeve_tbl_tm8[4][0] * EE[0] + xeve_tbl_tm8[4][1] * EE[1] + add) >> shift);\
        *((type_dst * )dst + 2 * line + j) = (type_dst)((xeve_tbl_tm8[2][0] * EO[0] + xeve_tbl_tm8[2][1] * EO[1] + add) >> shift);\
        *((type_dst * )dst + 6 * line + j) = (type_dst)((xeve_tbl_tm8[6][0] * EO[0] + xeve_tbl_tm8[6][1] * EO[1] + add) >> shift);\
        \
        *((type_dst * )dst + 1 * line + j) = (type_dst)((xeve_tbl_tm8[1][0] * O[0] + xeve_tbl_tm8[1][1] * O[1] + xeve_tbl_tm8[1][2] * O[2] + xeve_tbl_tm8[1][3] * O[3] + add) >> shift);\
        *((type_dst * )dst + 3 * line + j) = (type_dst)((xeve_tbl_tm8[3][0] * O[0] + xeve_tbl_tm8[3][1] * O[1] + xeve_tbl_tm8[3][2] * O[2] + xeve_tbl_tm8[3][3] * O[3] + add) >> shift);\
        *((type_dst * )dst + 5 * line + j) = (type_dst)((xeve_tbl_tm8[5][0] * O[0] + xeve_tbl_tm8[5][1] * O[1] + xeve_tbl_tm8[5][2] * O[2] + xeve_tbl_tm8[5][3] * O[3] + add) >> shift);\
        *((type_dst * )dst + 7 * line + j) = (type_dst)((xeve_tbl_tm8[7][0] * O[0] + xeve_tbl_tm8[7][1] * O[1] + xeve_tbl_tm8[7][2] * O[2] + xeve_tbl_tm8[7][3] * O[3] + add) >> shift);\
    }

    if(step == 0)
    {
        RUN_TX_PB8(src, dst, s16, s32);
    }
    else
    {
        RUN_TX_PB8(src, dst, s32, s16);
    }
}

void tx_pb16b(void * src, void * dst, int shift, int line, int step)
{
    int j, k;
    s64 E[8], O[8];
    s64 EE[4], EO[4];
    s64 EEE[2], EEO[2];
    int add = shift == 0 ? 0 : 1 << (shift - 1);

#define RUN_TX_PB16(src, dst, type_src, type_dst) \
    for (j = 0; j < line; j++)\
    {\
        /* E and O*/\
        for (k = 0; k < 8; k++)\
        {\
            E[k] = *((type_src * )src + j * 16 + k) + *((type_src * )src + j * 16 + 15 - k);\
            O[k] = *((type_src * )src + j * 16 + k) - *((type_src * )src + j * 16 + 15 - k);\
        }\
        /* EE and EO */\
        for (k = 0; k < 4; k++)\
        {\
            EE[k] = E[k] + E[7 - k];\
            EO[k] = E[k] - E[7 - k];\
        }\
        /* EEE and EEO */\
        EEE[0] = EE[0] + EE[3];\
        EEO[0] = EE[0] - EE[3];\
        EEE[1] = EE[1] + EE[2];\
        EEO[1] = EE[1] - EE[2];\
        \
        *((type_dst * )dst + 0  * line + j) = (type_dst)((xeve_tbl_tm16[0][0]  * EEE[0] + xeve_tbl_tm16[0][1]  * EEE[1] + add) >> shift);\
        *((type_dst * )dst + 8  * line + j) = (type_dst)((xeve_tbl_tm16[8][0]  * EEE[0] + xeve_tbl_tm16[8][1]  * EEE[1] + add) >> shift);\
        *((type_dst * )dst + 4  * line + j) = (type_dst)((xeve_tbl_tm16[4][0]  * EEO[0] + xeve_tbl_tm16[4][1]  * EEO[1] + add) >> shift);\
        *((type_dst * )dst + 12 * line + j) = (type_dst)((xeve_tbl_tm16[12][0] * EEO[0] + xeve_tbl_tm16[12][1] * EEO[1] + add) >> shift);\
        \
        for (k = 2; k < 16; k += 4)\
        {\
            *((type_dst * )dst + k * line + j) = (type_dst)((xeve_tbl_tm16[k][0] * EO[0] + xeve_tbl_tm16[k][1] * EO[1] + xeve_tbl_tm16[k][2] * EO[2] + xeve_tbl_tm16[k][3] * EO[3] + add) >> shift);\
        }\
        \
        for (k = 1; k < 16; k += 2)\
        {\
            *((type_dst * )dst + k * line + j) = (type_dst)((xeve_tbl_tm16[k][0] * O[0] + xeve_tbl_tm16[k][1] * O[1] + xeve_tbl_tm16[k][2] * O[2] + xeve_tbl_tm16[k][3] * O[3] + \
                xeve_tbl_tm16[k][4] * O[4] + xeve_tbl_tm16[k][5] * O[5] + xeve_tbl_tm16[k][6] * O[6] + xeve_tbl_tm16[k][7] * O[7] + add) >> shift);\
        }\
    }

    if(step == 0)
    {
        RUN_TX_PB16(src, dst, s16, s32);
    }
    else
    {
        RUN_TX_PB16(src, dst, s32, s16);
    }
}

void tx_pb32b(void * src, void * dst, int shift, int line, int step)
{
    int j, k;
    s64 E[16], O[16];
    s64 EE[8], EO[8];
    s64 EEE[4], EEO[4];
    s64 EEEE[2], EEEO[2];
    int add = shift == 0 ? 0 : 1 << (shift - 1);

#define RUN_TX_PB32(src, dst, type_src, type_dst) \
    for (j = 0; j < line; j++)\
    {\
        /* E and O*/\
        for (k = 0; k < 16; k++)\
        {\
            E[k] = *((type_src *)src + j * 32 + k) + *((type_src *)src + j * 32 + 31 - k);\
            O[k] = *((type_src *)src + j * 32 + k) - *((type_src *)src + j * 32 + 31 - k);\
        }\
        /* EE and EO */\
        for (k = 0; k < 8; k++)\
        {\
            EE[k] = E[k] + E[15 - k];\
            EO[k] = E[k] - E[15 - k];\
        }\
        /* EEE and EEO */\
        for (k = 0; k < 4; k++)\
        {\
            EEE[k] = EE[k] + EE[7 - k];\
            EEO[k] = EE[k] - EE[7 - k];\
        }\
        /* EEEE and EEEO */\
        EEEE[0] = EEE[0] + EEE[3];\
        EEEO[0] = EEE[0] - EEE[3];\
        EEEE[1] = EEE[1] + EEE[2];\
        EEEO[1] = EEE[1] - EEE[2];\
        \
        *((type_dst *)dst + 0  * line + j) = (type_dst)((xeve_tbl_tm32[0][0]  * EEEE[0] + xeve_tbl_tm32[0][1]  * EEEE[1] + add) >> shift);\
        *((type_dst *)dst + 16 * line + j) = (type_dst)((xeve_tbl_tm32[16][0] * EEEE[0] + xeve_tbl_tm32[16][1] * EEEE[1] + add) >> shift);\
        *((type_dst *)dst + 8  * line + j) = (type_dst)((xeve_tbl_tm32[8][0]  * EEEO[0] + xeve_tbl_tm32[8][1]  * EEEO[1] + add) >> shift);\
        *((type_dst *)dst + 24 * line + j) = (type_dst)((xeve_tbl_tm32[24][0] * EEEO[0] + xeve_tbl_tm32[24][1] * EEEO[1] + add) >> shift);\
        for (k = 4; k < 32; k += 8)\
        {\
            *((type_dst *)dst + k*line + j) = (type_dst)((xeve_tbl_tm32[k][0] * EEO[0] + xeve_tbl_tm32[k][1] * EEO[1] + xeve_tbl_tm32[k][2] * EEO[2] + xeve_tbl_tm32[k][3] * EEO[3] + add) >> shift);\
        }\
        for (k = 2; k < 32; k += 4)\
        {\
            *((type_dst *)dst + k*line + j) = (type_dst)((xeve_tbl_tm32[k][0] * EO[0] + xeve_tbl_tm32[k][1] * EO[1] + xeve_tbl_tm32[k][2] * EO[2] + xeve_tbl_tm32[k][3] * EO[3] +\
                xeve_tbl_tm32[k][4] * EO[4] + xeve_tbl_tm32[k][5] * EO[5] + xeve_tbl_tm32[k][6] * EO[6] + xeve_tbl_tm32[k][7] * EO[7] + add) >> shift);\
        }\
        for (k = 1; k < 32; k += 2)\
        {\
            *((type_dst *)dst + k*line + j) = (type_dst)((xeve_tbl_tm32[k][0] * O[0] + xeve_tbl_tm32[k][1] * O[1] + xeve_tbl_tm32[k][2] * O[2] + xeve_tbl_tm32[k][3] * O[3] +\
                xeve_tbl_tm32[k][4] * O[4] + xeve_tbl_tm32[k][5] * O[5] + xeve_tbl_tm32[k][6] * O[6] + xeve_tbl_tm32[k][7] * O[7] +\
                xeve_tbl_tm32[k][8] * O[8] + xeve_tbl_tm32[k][9] * O[9] + xeve_tbl_tm32[k][10] * O[10] + xeve_tbl_tm32[k][11] * O[11] +\
                xeve_tbl_tm32[k][12] * O[12] + xeve_tbl_tm32[k][13] * O[13] + xeve_tbl_tm32[k][14] * O[14] + xeve_tbl_tm32[k][15] * O[15] + add) >> shift);\
        }\
    }

    if(step == 0)
    {
        RUN_TX_PB32(src, dst, s16, s32);
    }
    else
    {
        RUN_TX_PB32(src, dst, s32, s16);
    }
}

void tx_pb64b(void *src, void *dst, int shift, int line, int step)
{
    const int tx_size = 64;
    const s8 * tm = xeve_tbl_tm64[0];
    int j, k;
    s64 E[32], O[32];
    s64 EE[16], EO[16];
    s64 EEE[8], EEO[8];
    s64 EEEE[4], EEEO[4];
    s64 EEEEE[2], EEEEO[2];
    int add = shift == 0 ? 0 : 1 << (shift - 1);

#define RUN_TX_PB64(src, dst, type_src, type_dst) \
    for (j = 0; j < line; j++)\
    {\
        for (k = 0; k < 32; k++)\
        {\
            E[k] =  *((type_src *)src + k) + *((type_src *)src + 63 - k);\
            O[k] =  *((type_src *)src + k) - *((type_src *)src + 63 - k);\
        }\
        for (k = 0; k<16; k++)\
        {\
            EE[k] = E[k] + E[31 - k];\
            EO[k] = E[k] - E[31 - k];\
        }\
        for (k = 0; k<8; k++)\
        {\
            EEE[k] = EE[k] + EE[15 - k];\
            EEO[k] = EE[k] - EE[15 - k];\
        }\
        for (k = 0; k<4; k++)\
        {\
            EEEE[k] = EEE[k] + EEE[7 - k];\
            EEEO[k] = EEE[k] - EEE[7 - k];\
        }\
        EEEEE[0] = EEEE[0] + EEEE[3];\
        EEEEO[0] = EEEE[0] - EEEE[3];\
        EEEEE[1] = EEEE[1] + EEEE[2];\
        EEEEO[1] = EEEE[1] - EEEE[2];\
        \
        *((type_dst *)dst + 0        ) = (type_dst)((tm[0 * 64 + 0]  * EEEEE[0] + tm[0 * 64 + 1]  * EEEEE[1] + add) >> shift);\
        *((type_dst *)dst + 16 * line) = (type_dst)((tm[16 * 64 + 0] * EEEEO[0] + tm[16 * 64 + 1] * EEEEO[1] + add) >> shift);\
        *((type_dst *)dst + 32 * line) = 0;\
        *((type_dst *)dst + 48 * line) = 0;\
        \
        for (k = 8; k<64; k += 16)\
        {\
            if(k > 31)\
            {\
                *((type_dst *)dst + k*line) = 0;\
            }\
            else\
            {\
                *((type_dst *)dst + k*line) = (type_dst)((tm[k * 64 + 0] * EEEO[0] + tm[k * 64 + 1] * EEEO[1] + tm[k * 64 + 2] * EEEO[2] + tm[k * 64 + 3] * EEEO[3] + add) >> shift);\
            }\
        }\
        for (k = 4; k<64; k += 8)\
        {\
            if(k > 31)\
            {\
                *((type_dst *)dst + k*line) = 0;\
            }\
            else\
            {\
                *((type_dst *)dst + k * line) = (type_dst)((tm[k * 64 + 0] * EEO[0] + tm[k * 64 + 1] * EEO[1] + tm[k * 64 + 2] * EEO[2] + tm[k * 64 + 3] * EEO[3] + \
                    tm[k * 64 + 4] * EEO[4] + tm[k * 64 + 5] * EEO[5] + tm[k * 64 + 6] * EEO[6] + tm[k * 64 + 7] * EEO[7] + add) >> shift); \
            }\
        }\
        for (k = 2; k<64; k += 4)\
        {\
            if (k > 31)\
            {\
                *((type_dst *)dst + k * line) = 0; \
            }\
            else\
            {\
                *((type_dst *)dst + k * line) = (type_dst)((tm[k * 64 + 0] * EO[0] + tm[k * 64 + 1] * EO[1] + tm[k * 64 + 2] * EO[2] + tm[k * 64 + 3] * EO[3] + \
                    tm[k * 64 + 4] * EO[4] + tm[k * 64 + 5] * EO[5] + tm[k * 64 + 6] * EO[6] + tm[k * 64 + 7] * EO[7] + \
                    tm[k * 64 + 8] * EO[8] + tm[k * 64 + 9] * EO[9] + tm[k * 64 + 10] * EO[10] + tm[k * 64 + 11] * EO[11] + \
                    tm[k * 64 + 12] * EO[12] + tm[k * 64 + 13] * EO[13] + tm[k * 64 + 14] * EO[14] + tm[k * 64 + 15] * EO[15] + add) >> shift); \
            }\
        }\
        for (k = 1; k<64; k += 2)\
        {\
            if (k > 31)\
            {\
                *((type_dst *)dst + k * line) = 0; \
            }\
            else\
            {\
                *((type_dst *)dst + k * line) = (type_dst)((tm[k * 64 + 0] * O[0] + tm[k * 64 + 1] * O[1] + tm[k * 64 + 2] * O[2] + tm[k * 64 + 3] * O[3] + \
                    tm[k * 64 + 4] * O[4] + tm[k * 64 + 5] * O[5] + tm[k * 64 + 6] * O[6] + tm[k * 64 + 7] * O[7] + \
                    tm[k * 64 + 8] * O[8] + tm[k * 64 + 9] * O[9] + tm[k * 64 + 10] * O[10] + tm[k * 64 + 11] * O[11] + \
                    tm[k * 64 + 12] * O[12] + tm[k * 64 + 13] * O[13] + tm[k * 64 + 14] * O[14] + tm[k * 64 + 15] * O[15] + \
                    tm[k * 64 + 16] * O[16] + tm[k * 64 + 17] * O[17] + tm[k * 64 + 18] * O[18] + tm[k * 64 + 19] * O[19] + \
                    tm[k * 64 + 20] * O[20] + tm[k * 64 + 21] * O[21] + tm[k * 64 + 22] * O[22] + tm[k * 64 + 23] * O[23] + \
                    tm[k * 64 + 24] * O[24] + tm[k * 64 + 25] * O[25] + tm[k * 64 + 26] * O[26] + tm[k * 64 + 27] * O[27] + \
                    tm[k * 64 + 28] * O[28] + tm[k * 64 + 29] * O[29] + tm[k * 64 + 30] * O[30] + tm[k * 64 + 31] * O[31] + add) >> shift); \
            }\
        }\
        src = (type_src *)src + tx_size;\
        dst = (type_dst *)dst + 1;\
    }

    if(step == 0)
    {
        RUN_TX_PB64(src, dst, s16, s32);
    }
    else
    {
        RUN_TX_PB64(src, dst, s32, s16);
    }
}

const XEVE_TXB xeve_tbl_txb[MAX_TR_LOG2] =
{
    tx_pb2b,
    tx_pb4b,
    tx_pb8b,
    tx_pb16b,
    tx_pb32b,
    tx_pb64b
};

static void xeve_trans(s16 * coef, int log2_cuw, int log2_cuh, int bit_depth)
{    
    int shift1 = xeve_get_transform_shift(log2_cuw, 0, bit_depth);
    int shift2 = xeve_get_transform_shift(log2_cuh, 1, bit_depth);

    s32 tb[MAX_TR_DIM]; /* temp buffer */
    (*xeve_func_txb)[log2_cuw - 1](coef, tb, 0, 1 << log2_cuh, 0);
    (*xeve_func_txb)[log2_cuh - 1](tb, coef, (shift1 + shift2), 1 << log2_cuw, 1);
}

void xeve_init_err_scale(XEVE_CTX * ctx)
{
    double err_scale;
    int qp;
    int i;

    for (qp = 0; qp < 6; qp++)
    {
        int q_value = xeve_quant_scale[ctx->param.tool_iqt][qp];

        for (i = 0; i < NUM_CU_LOG2 + 1; i++)
        {
            int tr_shift = MAX_TX_DYNAMIC_RANGE - ctx->param.codec_bit_depth - (i + 1);

            err_scale = (double)(1 << SCALE_BITS) * pow(2.0, -tr_shift);
            err_scale = err_scale / q_value / (1 << ((ctx->param.codec_bit_depth - 8)));
            ctx->err_scale[qp][i] = (s64)(err_scale * (double)(1 << ERR_SCALE_PRECISION_BITS));
        }
    }
}

static __inline s64 get_ic_rate_cost_rl(u32 abs_level, u32 run, s32 ctx_run, u32 ctx_level, s64 lambda, XEVE_CORE * core)
{
    s32 rate;
    if(abs_level == 0)
    {
        rate = 0;
        if(run == 0)
        {
            rate += core->rdoq_est_run[ctx_run][1];
        }
        else
        {
            rate += core->rdoq_est_run[ctx_run + 1][1];
        }
    }
    else
    {
        rate = GET_IEP_RATE;
        if(run == 0)
        {
            rate += core->rdoq_est_run[ctx_run][0];
        }
        else
        {
            rate += core->rdoq_est_run[ctx_run + 1][0];
        }

        if(abs_level == 1)
        {
            rate += core->rdoq_est_level[ctx_level][0];
        }
        else
        {
            rate += core->rdoq_est_level[ctx_level][1];
            rate += core->rdoq_est_level[ctx_level + 1][1] * (s32)(abs_level - 2);
            rate += core->rdoq_est_level[ctx_level + 1][0];
        }
    }
    return (s64)GET_I_COST(rate, lambda);
}

static __inline u32 get_coded_level_rl(s64* rd64_uncoded_cost, s64* rd64_coded_cost, s64 level_double, u32 max_abs_level,
                                       u32 run, u16 ctx_run, u16 ctx_level, s32 q_bits, s64 err_scale, s64 lambda, XEVE_CORE * core)
{
    u32 best_abs_level = 0;
    s64 err1 = (level_double * err_scale) >> ERR_SCALE_PRECISION_BITS;
    u32 min_abs_level;
    u32 abs_level;

    *rd64_uncoded_cost = err1 * err1;
    *rd64_coded_cost = *rd64_uncoded_cost + get_ic_rate_cost_rl(0, run, ctx_run, ctx_level, lambda, core);

    min_abs_level = (max_abs_level > 1 ? max_abs_level - 1 : 1);
    for(abs_level = max_abs_level; abs_level >= min_abs_level; abs_level--)
    {
        s64 i64Delta = level_double - ((s64)abs_level << q_bits);
        s64 err = (i64Delta * err_scale) >> ERR_SCALE_PRECISION_BITS;
        s64 dCurrCost = err * err + get_ic_rate_cost_rl(abs_level, run, ctx_run, ctx_level, lambda, core);

        if(dCurrCost < *rd64_coded_cost)
        {
            best_abs_level = abs_level;
            *rd64_coded_cost = dCurrCost;
        }
    }
    return best_abs_level;
}

int xeve_rdoq_set_ctx_cc(XEVE_CORE * core, int ch_type, int prev_level)
{
    return (ch_type == Y_C ? 0 : 2);
}

int xeve_rdoq_run_length_cc(u8 qp, double d_lambda, u8 is_intra, s16 *src_coef, s16 *dst_tmp, int log2_cuw, int log2_cuh, int ch_type, XEVE_CORE * core, int bit_depth)
{
    const int qp_rem = qp % 6;
    const int ns_shift = ((log2_cuw + log2_cuh) & 1) ? 7 : 0;
    const int ns_scale = ((log2_cuw + log2_cuh) & 1) ? 181 : 1;
    const int ns_offset = ((log2_cuw + log2_cuh) & 1) ? (1 << (ns_shift - 1)) : 0;
    const int q_value = (xeve_quant_scale[core->ctx->param.tool_iqt][qp_rem] * ns_scale + ns_offset) >> ns_shift;
    const int log2_size = (log2_cuw + log2_cuh) >> 1;
    const int tr_shift = MAX_TX_DYNAMIC_RANGE - bit_depth - (log2_size);
    const u32 max_num_coef = 1 << (log2_cuw + log2_cuh);
    const u16 *scan = xeve_tbl_scan[log2_cuw - 1][log2_cuh - 1];
    const int ctx_last = (ch_type == Y_C) ? 0 : 1;
    const int q_bits = QUANT_SHIFT + tr_shift + (qp / 6);
    int nnz = 0;
    int sum_all = 0;
    u32 scan_pos;
    u32 run;
    u32 prev_level;
    u32 best_last_idx_p1 = 0;
    s16 tmp_coef[MAX_TR_DIM];
    s64 tmp_level_double[MAX_TR_DIM];
    s16 tmp_dst_coef[MAX_TR_DIM];
    const s64 lambda = (s64)(d_lambda * (double)(1 << SCALE_BITS) + 0.5);
    s64 err_scale = core->ctx->err_scale[qp_rem][log2_size - 1];
    s64 d64_best_cost = 0;
    s64 d64_base_cost = 0;
    s64 d64_coded_cost = 0;
    s64 d64_uncoded_cost = 0;       
    s64 d64_block_uncoded_cost = 0;
    s64 err;

    /* ===== quantization ===== */
    for (scan_pos = 0; scan_pos < max_num_coef; scan_pos++)
    {
        u32 blk_pos = scan[scan_pos];
        s64 level_double = src_coef[blk_pos];
        u32 max_abs_level;
        s8 lower_int;
        s64 temp_level;

        temp_level = ((s64)XEVE_ABS(src_coef[blk_pos]) * (s64)q_value);

        level_double = (int)XEVE_MIN(((s64)temp_level), (s64)XEVE_INT32_MAX - (s64)(1 << (q_bits - 1)));
        tmp_level_double[blk_pos] = level_double;
        max_abs_level = (u32)(level_double >> q_bits);
        lower_int = ((level_double - ((s64)max_abs_level << q_bits)) < (s64)(1 << (q_bits - 1))) ? 1 : 0;

        if (!lower_int)
        {
            max_abs_level++;
        }

        err = (level_double * err_scale) >> ERR_SCALE_PRECISION_BITS;
        d64_block_uncoded_cost += err * err;
        tmp_coef[blk_pos] = src_coef[blk_pos] > 0 ? (s16)max_abs_level : -(s16)(max_abs_level);
        sum_all += max_abs_level;
    }

    xeve_mset(dst_tmp, 0, sizeof(s16)*max_num_coef);

    if (sum_all == 0)
    {       
        return nnz;
    }

    if (!is_intra && ch_type == Y_C)
    {
        d64_best_cost = d64_block_uncoded_cost + GET_I_COST(core->rdoq_est_cbf_all[0], lambda);
        d64_base_cost = d64_block_uncoded_cost + GET_I_COST(core->rdoq_est_cbf_all[1], lambda);
    }
    else
    {
        if (ch_type == Y_C)
        {
            d64_best_cost = d64_block_uncoded_cost + GET_I_COST(core->rdoq_est_cbf_luma[0], lambda);
            d64_base_cost = d64_block_uncoded_cost + GET_I_COST(core->rdoq_est_cbf_luma[1], lambda);
        }
        else if (ch_type == U_C)
        {
            d64_best_cost = d64_block_uncoded_cost + GET_I_COST(core->rdoq_est_cbf_cb[0], lambda);
            d64_base_cost = d64_block_uncoded_cost + GET_I_COST(core->rdoq_est_cbf_cb[1], lambda);
        }
        else //if (ch_type == U_C)
        {
            d64_best_cost = d64_block_uncoded_cost + GET_I_COST(core->rdoq_est_cbf_cr[0], lambda);
            d64_base_cost = d64_block_uncoded_cost + GET_I_COST(core->rdoq_est_cbf_cr[1], lambda);
        }     
    }

    run = 0;
    prev_level = 6;

    for (scan_pos = 0; scan_pos < max_num_coef; scan_pos++)
    {
        u32 blk_pos = scan[scan_pos];
        u32 level;
        int ctx_run = core->ctx->fn_rdoq_set_ctx_cc(core, ch_type, prev_level);
        int ctx_level = ctx_run;

        level = get_coded_level_rl(&d64_uncoded_cost, &d64_coded_cost, tmp_level_double[blk_pos], XEVE_ABS(tmp_coef[blk_pos]), run, ctx_run, ctx_level, q_bits, err_scale, lambda,  core);
        tmp_dst_coef[blk_pos] = tmp_coef[blk_pos] < 0 ? -(s32)(level) : level;
        d64_base_cost -= d64_uncoded_cost;
        d64_base_cost += d64_coded_cost;

        if (level)
        {
            /* ----- check for last flag ----- */
            s64 d64_cost_last_zero = GET_I_COST(core->rdoq_est_last[ctx_last][0], lambda);
            s64 d64_cost_last_one = GET_I_COST(core->rdoq_est_last[ctx_last][1], lambda);
            s64 d64_cur_is_last_cost = d64_base_cost + d64_cost_last_one;

            d64_base_cost += d64_cost_last_zero;

            if (d64_cur_is_last_cost < d64_best_cost)
            {
                d64_best_cost = d64_cur_is_last_cost;
                best_last_idx_p1 = scan_pos + 1;
            }
            run = 0;
            prev_level = level;
        }
        else
        {
            run++;
        }
    }

    /* ===== clean uncoded coeficients ===== */
    for (scan_pos = 0; scan_pos < max_num_coef; scan_pos++)
    {
        u32 blk_pos = scan[scan_pos];

        if (scan_pos < best_last_idx_p1)
        {
            if (tmp_dst_coef[blk_pos])
            {
                nnz++;
            }
        }
        else
        {
            tmp_dst_coef[blk_pos] = 0;
        }

        dst_tmp[blk_pos] = tmp_dst_coef[blk_pos];
    }

    return nnz;
}

static int xeve_quant_nnz(u8 qp, double lambda, int is_intra, s16 * coef, int log2_cuw, int log2_cuh, u16 scale, int ch_type
                        , int slice_type, XEVE_CORE * core, int bit_depth, int use_rdoq)
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

    if (use_rdoq)
    {
        nnz = xeve_rdoq_run_length_cc(qp, lambda, is_intra, coef, coef, log2_cuw, log2_cuh, ch_type, core, bit_depth);
    }
    else
    {
        s32 lev;
        s32 offset;
        int sign;
        int i;
        int shift;
        int tr_shift;
        int log2cuwh_sum = log2_cuw + log2_cuh;
        int log2_size = log2cuwh_sum >> 1;
        int cuwh = (1 << (log2cuwh_sum));

        tr_shift = MAX_TX_DYNAMIC_RANGE - bit_depth - log2_size;
        shift = QUANT_SHIFT + tr_shift + (qp / 6);
        offset = (s32)((slice_type == SLICE_I) ? 171 : 85) << (s32)(shift - 9);

        for (i = 0; i < cuwh; i++)
        {
            sign = XEVE_SIGN_GET(coef[i]);
            lev = (s32)XEVE_ABS(coef[i]) * (s32)scale; // coeff is in 10 bit and scale is in 16 bit, so product should fit in 32 bit precision
            lev = (s16)((lev + offset) >> shift);
            coef[i] = (s16)XEVE_SIGN_SET(lev, sign);
            nnz += !!(coef[i]);
        }
    }

    return nnz;
}

static int xeve_tq_nnz(u8 qp, double lambda, s16 * coef, int log2_cuw, int log2_cuh, u16 scale, int slice_type, int ch_type, int is_intra, XEVE_CORE * core, int bit_depth, int rdoq)
{
    xeve_trans(coef, log2_cuw, log2_cuh, bit_depth);
    return xeve_quant_nnz(qp, lambda, is_intra, coef, log2_cuw, log2_cuh, scale, ch_type, slice_type, core, bit_depth, rdoq);
}

int xeve_sub_block_tq(XEVE_CTX * ctx, XEVE_CORE * core, s16 coef[N_C][MAX_CU_DIM], int log2_cuw, int log2_cuh, int slice_type, int nnz[N_C], int is_intra, int run_stats)
{
    int run[N_C] = {run_stats & 1, (run_stats >> 1) & 1, (run_stats >> 2) & 1};
    s16 *coef_temp[N_C];
    s16 coef_temp_buf[N_C][MAX_TR_DIM];
    int i, j, c;
    int log2_w_sub = (log2_cuw > MAX_TR_LOG2) ? MAX_TR_LOG2 : log2_cuw;
    int log2_h_sub = (log2_cuh > MAX_TR_LOG2) ? MAX_TR_LOG2 : log2_cuh;
    int loop_w = (log2_cuw > MAX_TR_LOG2) ? (1 << (log2_cuw - MAX_TR_LOG2)) : 1;
    int loop_h = (log2_cuh > MAX_TR_LOG2) ? (1 << (log2_cuh - MAX_TR_LOG2)) : 1;
    int w_shift = ctx->param.cs_w_shift;
    int h_shift = ctx->param.cs_h_shift;
    int stride = (1 << log2_cuw);
    int sub_stride = (1 << log2_w_sub);
    u8 qp[N_C] = { core->qp_y, core->qp_u, core->qp_v };
    double lambda[N_C] = { core->lambda[0], core->lambda[1], core->lambda[2] };
    int nnz_temp[N_C] = {0};
    xeve_mset(core->nnz_sub, 0, sizeof(int) * N_C * MAX_SUB_TB_NUM);
    if(!ctx->sps.chroma_format_idc)
    {
        run[1] = run[2] = 0;
    }

    for(j = 0; j < loop_h; j++)
    {
        for(i = 0; i < loop_w; i++)
        {
            for(c = 0; c < N_C; c++)
            {
                if(run[c])
                {
                    int pos_sub_x = c == 0 ? (i * (1 << (log2_w_sub))) : (i * (1 << (log2_w_sub - w_shift)));
                    int pos_sub_y = c == 0 ? j * (1 << (log2_h_sub)) * (stride) : j * (1 << (log2_h_sub - h_shift)) * (stride >> w_shift);

                    if(loop_h + loop_w > 2)
                    {
                        if(c==0)
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
                    if(c==0)
                        core->nnz_sub[c][(j << 1) | i] = xeve_tq_nnz(qp[c], lambda[c], coef_temp[c], log2_w_sub, log2_h_sub, scale, slice_type, c, is_intra, core, ctx->sps.bit_depth_luma_minus8 + 8, ctx->param.rdoq);
                    else
                        core->nnz_sub[c][(j << 1) | i] = xeve_tq_nnz(qp[c], lambda[c], coef_temp[c], log2_w_sub - w_shift, log2_h_sub - h_shift, scale, slice_type, c, is_intra, core, ctx->sps.bit_depth_luma_minus8 + 8, ctx->param.rdoq);
                    nnz_temp[c] += core->nnz_sub[c][(j << 1) | i];

                    if(loop_h + loop_w > 2)
                    {
                        if(c==0)
                            xeve_block_copy(coef_temp_buf[c], sub_stride, coef[c] + pos_sub_x + pos_sub_y, stride, log2_w_sub, log2_h_sub);
                        else
                            xeve_block_copy(coef_temp_buf[c], sub_stride >> w_shift, coef[c] + pos_sub_x + pos_sub_y, stride >> w_shift, log2_w_sub - w_shift, log2_h_sub - h_shift);
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

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

#include <math.h>
#include "xeve_type.h"

void xeve_itx_pb2b(void *src, void *dst, int shift, int line, int step)
{
    int j;
    s64 E, O;
    int add = shift == 0 ? 0 : 1 << (shift - 1);
#define RUN_ITX_PB2(src, dst, type_src, type_dst) \
    for (j = 0; j < line; j++)\
    {\
        /* E and O */\
        E = *((type_src *)src + 0 * line + j) + *((type_src *)src + 1 * line + j);\
        O = *((type_src *)src + 0 * line + j) - *((type_src *)src + 1 * line + j);\
        \
        if(step == 0)\
        {\
            *((type_dst *)dst + j * 2 + 0) = ITX_CLIP_32((xeve_tbl_tm2[0][0] * E + add) >> shift); \
            *((type_dst *)dst + j * 2 + 1) = ITX_CLIP_32((xeve_tbl_tm2[1][0] * O + add) >> shift); \
        }\
        else\
        {\
            *((type_dst *)dst + j * 2 + 0) = ITX_CLIP((xeve_tbl_tm2[0][0] * E + add) >> shift); \
            *((type_dst *)dst + j * 2 + 1) = ITX_CLIP((xeve_tbl_tm2[1][0] * O + add) >> shift); \
        }\
    }
    if (step == 0)
    {
        RUN_ITX_PB2(src, dst, s16, s32);
    }
    else
    {
        RUN_ITX_PB2(src, dst, s32, s16);
    }
}

void xeve_itx_pb4b(void *src, void *dst, int shift, int line, int step)
{
    int j;
    s64 E[2], O[2];
    int add = shift == 0 ? 0 : 1 << (shift - 1);

#define RUN_ITX_PB4(src, dst, type_src, type_dst) \
    for (j = 0; j < line; j++)\
    {\
        /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */\
        O[0] = xeve_tbl_tm4[1][0] * *((type_src * )src + 1 * line + j) + xeve_tbl_tm4[3][0] * *((type_src * )src + 3 * line + j);\
        O[1] = xeve_tbl_tm4[1][1] * *((type_src * )src + 1 * line + j) + xeve_tbl_tm4[3][1] * *((type_src * )src + 3 * line + j);\
        E[0] = xeve_tbl_tm4[0][0] * *((type_src * )src + 0 * line + j) + xeve_tbl_tm4[2][0] * *((type_src * )src + 2 * line + j);\
        E[1] = xeve_tbl_tm4[0][1] * *((type_src * )src + 0 * line + j) + xeve_tbl_tm4[2][1] * *((type_src * )src + 2 * line + j);\
        \
        /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */\
        if (step == 0)\
        {\
            *((type_dst * )dst + j * 4 + 0) = ITX_CLIP_32((E[0] + O[0] + add) >> shift);\
            *((type_dst * )dst + j * 4 + 1) = ITX_CLIP_32((E[1] + O[1] + add) >> shift);\
            *((type_dst * )dst + j * 4 + 2) = ITX_CLIP_32((E[1] - O[1] + add) >> shift);\
            *((type_dst * )dst + j * 4 + 3) = ITX_CLIP_32((E[0] - O[0] + add) >> shift);\
        }\
        else\
        {\
            *((type_dst * )dst + j * 4 + 0) = ITX_CLIP((E[0] + O[0] + add) >> shift);\
            *((type_dst * )dst + j * 4 + 1) = ITX_CLIP((E[1] + O[1] + add) >> shift);\
            *((type_dst * )dst + j * 4 + 2) = ITX_CLIP((E[1] - O[1] + add) >> shift);\
            *((type_dst * )dst + j * 4 + 3) = ITX_CLIP((E[0] - O[0] + add) >> shift);\
        }\
    }

    if (step == 0)
    {
        RUN_ITX_PB4(src, dst, s16, s32);
    }
    else
    {
        RUN_ITX_PB4(src, dst, s32, s16);
    }
}

void xeve_itx_pb8b(void *src, void *dst, int shift, int line, int step)
{
    int j, k;
    s64 E[4], O[4];
    s64 EE[2], EO[2];
    int add = shift == 0 ? 0 : 1 << (shift - 1);
#define RUN_ITX_PB8(src, dst, type_src, type_dst) \
    for (j = 0; j < line; j++)\
    {\
        /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */\
        for (k = 0; k < 4; k++)\
        {\
            O[k] = xeve_tbl_tm8[1][k] * *((type_src * )src + 1 * line + j) + xeve_tbl_tm8[3][k] * *((type_src * )src + 3 * line + j)\
                 + xeve_tbl_tm8[5][k] * *((type_src * )src + 5 * line + j) + xeve_tbl_tm8[7][k] * *((type_src * )src + 7 * line + j);\
        }\
        \
        EO[0] = xeve_tbl_tm8[2][0] * *((type_src * )src + 2 * line + j) + xeve_tbl_tm8[6][0] * *((type_src * )src + 6 * line + j);\
        EO[1] = xeve_tbl_tm8[2][1] * *((type_src * )src + 2 * line + j) + xeve_tbl_tm8[6][1] * *((type_src * )src + 6 * line + j);\
        EE[0] = xeve_tbl_tm8[0][0] * *((type_src * )src + 0 * line + j) + xeve_tbl_tm8[4][0] * *((type_src * )src + 4 * line + j);\
        EE[1] = xeve_tbl_tm8[0][1] * *((type_src * )src + 0 * line + j) + xeve_tbl_tm8[4][1] * *((type_src * )src + 4 * line + j);\
        \
        /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */\
        E[0] = EE[0] + EO[0];\
        E[3] = EE[0] - EO[0];\
        E[1] = EE[1] + EO[1];\
        E[2] = EE[1] - EO[1];\
        \
        if(step == 0)\
        {\
            for (k = 0; k < 4; k++)\
            {\
                *((type_dst * )dst + j * 8 + k    ) = ITX_CLIP_32((E[k] + O[k] + add) >> shift);\
                *((type_dst * )dst + j * 8 + k + 4) = ITX_CLIP_32((E[3 - k] - O[3 - k] + add) >> shift);\
            }\
        }\
        else\
        {\
            for (k = 0; k < 4; k++)\
            {\
                *((type_dst * )dst + j * 8 + k    ) = ITX_CLIP((E[k] + O[k] + add) >> shift);\
                *((type_dst * )dst + j * 8 + k + 4) = ITX_CLIP((E[3 - k] - O[3 - k] + add) >> shift);\
            }\
        }\
    }

    if (step == 0)
    {
        RUN_ITX_PB8(src, dst, s16, s32);
    }
    else
    {
        RUN_ITX_PB8(src, dst, s32, s16);
    }
}

void xeve_itx_pb16b(void *src, void *dst, int shift, int line, int step)
{
    int j, k;
    s64 E[8], O[8];
    s64 EE[4], EO[4];
    s64 EEE[2], EEO[2];
    int add = shift == 0 ? 0 : 1 << (shift - 1);
#define RUN_ITX_PB16(src, dst, type_src, type_dst) \
    for (j = 0; j < line; j++)\
    {\
        /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */\
        for (k = 0; k < 8; k++)\
        {\
            O[k] = xeve_tbl_tm16[1][k]  * *((type_src * )src + 1  * line + j) + xeve_tbl_tm16[3][k]  * *((type_src * )src + 3  * line + j) +\
                   xeve_tbl_tm16[5][k]  * *((type_src * )src + 5  * line + j) + xeve_tbl_tm16[7][k]  * *((type_src * )src + 7  * line + j) +\
                   xeve_tbl_tm16[9][k]  * *((type_src * )src + 9  * line + j) + xeve_tbl_tm16[11][k] * *((type_src * )src + 11 * line + j) +\
                   xeve_tbl_tm16[13][k] * *((type_src * )src + 13 * line + j) + xeve_tbl_tm16[15][k] * *((type_src * )src + 15 * line + j);\
        }\
        \
        for (k = 0; k < 4; k++)\
        {\
            EO[k] = xeve_tbl_tm16[2][k]  * *((type_src * )src + 2  * line + j) + xeve_tbl_tm16[6][k]  * *((type_src * )src + 6  * line + j) +\
                    xeve_tbl_tm16[10][k] * *((type_src * )src + 10 * line + j) + xeve_tbl_tm16[14][k] * *((type_src * )src + 14 * line + j);\
        }\
        \
        EEO[0] = xeve_tbl_tm16[4][0] * *((type_src * )src + 4 * line + j) + xeve_tbl_tm16[12][0] * *((type_src * )src + 12 * line + j);\
        EEE[0] = xeve_tbl_tm16[0][0] * *((type_src * )src + 0 * line + j) + xeve_tbl_tm16[8][0]  * *((type_src * )src + 8  * line + j);\
        EEO[1] = xeve_tbl_tm16[4][1] * *((type_src * )src + 4 * line + j) + xeve_tbl_tm16[12][1] * *((type_src * )src + 12 * line + j);\
        EEE[1] = xeve_tbl_tm16[0][1] * *((type_src * )src + 0 * line + j) + xeve_tbl_tm16[8][1]  * *((type_src * )src + 8  * line + j);\
        \
        /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */\
        for (k = 0; k < 2; k++)\
        {\
            EE[k] = EEE[k] + EEO[k];\
            EE[k + 2] = EEE[1 - k] - EEO[1 - k];\
        }\
        for (k = 0; k < 4; k++)\
        {\
            E[k] = EE[k] + EO[k];\
            E[k + 4] = EE[3 - k] - EO[3 - k];\
        }\
        if(step == 0)\
        {\
            for (k = 0; k < 8; k++)\
            {\
                *((type_dst * )dst + j * 16 + k    ) = ITX_CLIP_32((E[k] + O[k] + add) >> shift); \
                *((type_dst * )dst + j * 16 + k + 8) = ITX_CLIP_32((E[7 - k] - O[7 - k] + add) >> shift); \
            }\
        }\
        else\
        {\
            for (k = 0; k < 8; k++)\
            {\
                *((type_dst * )dst + j * 16 + k    ) = ITX_CLIP((E[k] + O[k] + add) >> shift); \
                *((type_dst * )dst + j * 16 + k + 8) = ITX_CLIP((E[7 - k] - O[7 - k] + add) >> shift); \
            }\
        }\
    }

    if (step == 0)
    {
        RUN_ITX_PB16(src, dst, s16, s32);
    }
    else
    {
        RUN_ITX_PB16(src, dst, s32, s16);    
    }
}

void xeve_itx_pb32b(void *src, void *dst, int shift, int line, int step)
{
    int j, k;
    s64 E[16], O[16];
    s64 EE[8], EO[8];
    s64 EEE[4], EEO[4];
    s64 EEEE[2], EEEO[2];
    int add = shift == 0 ? 0 : 1 << (shift - 1);
#define RUN_ITX_PB32(src, dst, type_src, type_dst) \
    for (j = 0; j < line; j++)\
    {\
        for (k = 0; k < 16; k++) \
        {\
            O[k] = xeve_tbl_tm32[1][k]  * *((type_src * )src + 1  * line + j)  + \
                   xeve_tbl_tm32[3][k]  * *((type_src * )src + 3  * line + j)  + \
                   xeve_tbl_tm32[5][k]  * *((type_src * )src + 5  * line + j)  + \
                   xeve_tbl_tm32[7][k]  * *((type_src * )src + 7  * line + j)  + \
                   xeve_tbl_tm32[9][k]  * *((type_src * )src + 9  * line + j)  + \
                   xeve_tbl_tm32[11][k] * *((type_src * )src + 11 * line + j) + \
                   xeve_tbl_tm32[13][k] * *((type_src * )src + 13 * line + j) + \
                   xeve_tbl_tm32[15][k] * *((type_src * )src + 15 * line + j) + \
                   xeve_tbl_tm32[17][k] * *((type_src * )src + 17 * line + j) + \
                   xeve_tbl_tm32[19][k] * *((type_src * )src + 19 * line + j) + \
                   xeve_tbl_tm32[21][k] * *((type_src * )src + 21 * line + j) + \
                   xeve_tbl_tm32[23][k] * *((type_src * )src + 23 * line + j) + \
                   xeve_tbl_tm32[25][k] * *((type_src * )src + 25 * line + j) + \
                   xeve_tbl_tm32[27][k] * *((type_src * )src + 27 * line + j) + \
                   xeve_tbl_tm32[29][k] * *((type_src * )src + 29 * line + j) + \
                   xeve_tbl_tm32[31][k] * *((type_src * )src + 31 * line + j);\
        }\
        \
        for (k = 0; k < 8; k++)\
        {\
            EO[k] = xeve_tbl_tm32[2][k]  * *((type_src * )src + 2  * line + j) + \
                    xeve_tbl_tm32[6][k]  * *((type_src * )src + 6  * line + j) + \
                    xeve_tbl_tm32[10][k] * *((type_src * )src + 10 * line + j) + \
                    xeve_tbl_tm32[14][k] * *((type_src * )src + 14 * line + j) + \
                    xeve_tbl_tm32[18][k] * *((type_src * )src + 18 * line + j) + \
                    xeve_tbl_tm32[22][k] * *((type_src * )src + 22 * line + j) + \
                    xeve_tbl_tm32[26][k] * *((type_src * )src + 26 * line + j) + \
                    xeve_tbl_tm32[30][k] * *((type_src * )src + 30 * line + j);\
        }\
        \
        for (k = 0; k < 4; k++)\
        {\
            EEO[k] = xeve_tbl_tm32[4][k]  * *((type_src * )src + 4  * line + j) + \
                     xeve_tbl_tm32[12][k] * *((type_src * )src + 12 * line + j) + \
                     xeve_tbl_tm32[20][k] * *((type_src * )src + 20 * line + j) + \
                     xeve_tbl_tm32[28][k] * *((type_src * )src + 28 * line + j);\
        }\
        \
        EEEO[0] = xeve_tbl_tm32[8][0] * *((type_src * )src + 8 * line + j) + xeve_tbl_tm32[24][0] * *((type_src * )src + 24 * line + j);\
        EEEO[1] = xeve_tbl_tm32[8][1] * *((type_src * )src + 8 * line + j) + xeve_tbl_tm32[24][1] * *((type_src * )src + 24 * line + j);\
        EEEE[0] = xeve_tbl_tm32[0][0] * *((type_src * )src + 0 * line + j) + xeve_tbl_tm32[16][0] * *((type_src * )src + 16 * line + j);\
        EEEE[1] = xeve_tbl_tm32[0][1] * *((type_src * )src + 0 * line + j) + xeve_tbl_tm32[16][1] * *((type_src * )src + 16 * line + j);\
        \
        EEE[0] = EEEE[0] + EEEO[0];\
        EEE[3] = EEEE[0] - EEEO[0];\
        EEE[1] = EEEE[1] + EEEO[1];\
        EEE[2] = EEEE[1] - EEEO[1];\
        for (k = 0; k<4; k++)\
        {\
            EE[k] = EEE[k] + EEO[k];\
            EE[k + 4] = EEE[3 - k] - EEO[3 - k];\
        }\
        for (k = 0; k<8; k++)\
        {\
            E[k] = EE[k] + EO[k];\
            E[k + 8] = EE[7 - k] - EO[7 - k];\
        }\
        if (step == 0)\
        {\
            for (k = 0; k < 16; k++)\
            {\
                *((type_dst * )dst + j * 32 + k     ) = ITX_CLIP_32((E[k] + O[k] + add) >> shift);\
                *((type_dst * )dst + j * 32 + k + 16) = ITX_CLIP_32((E[15 - k] - O[15 - k] + add) >> shift);\
            }\
        }\
        else\
        {\
            for (k = 0; k < 16; k++)\
            {\
                *((type_dst * )dst + j * 32 + k     ) = ITX_CLIP((E[k] + O[k] + add) >> shift);\
                *((type_dst * )dst + j * 32 + k + 16) = ITX_CLIP((E[15 - k] - O[15 - k] + add) >> shift);\
            }\
        }\
    }

    if (step == 0)
    {
        RUN_ITX_PB32(src, dst, s16, s32);
    }
    else
    {
        RUN_ITX_PB32(src, dst, s32, s16);
    }
}

void xeve_itx_pb64b(void *src, void *dst, int shift, int line, int step)
{
    const int tx_size = 64;
    const s8 *tm = xeve_tbl_tm64[0];
    int j, k;
    s64 E[32], O[32];
    s64 EE[16], EO[16];
    s64 EEE[8], EEO[8];
    s64 EEEE[4], EEEO[4];
    s64 EEEEE[2], EEEEO[2];
    int add = shift == 0 ? 0 : 1 << (shift - 1);
#define RUN_ITX_PB64(src, dst, type_src, type_dst) \
    for (j = 0; j < line; j++) \
    { \
        for (k = 0; k < 32; k++) \
        { \
            O[k] = tm[1  * 64 + k] * *((type_src * )src +      line) + tm[3  * 64 + k] * *((type_src * )src + 3  * line) + \
                   tm[5  * 64 + k] * *((type_src * )src + 5  * line) + tm[7  * 64 + k] * *((type_src * )src + 7  * line) + \
                   tm[9  * 64 + k] * *((type_src * )src + 9  * line) + tm[11 * 64 + k] * *((type_src * )src + 11 * line) + \
                   tm[13 * 64 + k] * *((type_src * )src + 13 * line) + tm[15 * 64 + k] * *((type_src * )src + 15 * line) + \
                   tm[17 * 64 + k] * *((type_src * )src + 17 * line) + tm[19 * 64 + k] * *((type_src * )src + 19 * line) + \
                   tm[21 * 64 + k] * *((type_src * )src + 21 * line) + tm[23 * 64 + k] * *((type_src * )src + 23 * line) + \
                   tm[25 * 64 + k] * *((type_src * )src + 25 * line) + tm[27 * 64 + k] * *((type_src * )src + 27 * line) + \
                   tm[29 * 64 + k] * *((type_src * )src + 29 * line) + tm[31 * 64 + k] * *((type_src * )src + 31 * line) + \
                   tm[33 * 64 + k] * *((type_src * )src + 33 * line) + tm[35 * 64 + k] * *((type_src * )src + 35 * line) + \
                   tm[37 * 64 + k] * *((type_src * )src + 37 * line) + tm[39 * 64 + k] * *((type_src * )src + 39 * line) + \
                   tm[41 * 64 + k] * *((type_src * )src + 41 * line) + tm[43 * 64 + k] * *((type_src * )src + 43 * line) + \
                   tm[45 * 64 + k] * *((type_src * )src + 45 * line) + tm[47 * 64 + k] * *((type_src * )src + 47 * line) + \
                   tm[49 * 64 + k] * *((type_src * )src + 49 * line) + tm[51 * 64 + k] * *((type_src * )src + 51 * line) + \
                   tm[53 * 64 + k] * *((type_src * )src + 53 * line) + tm[55 * 64 + k] * *((type_src * )src + 55 * line) + \
                   tm[57 * 64 + k] * *((type_src * )src + 57 * line) + tm[59 * 64 + k] * *((type_src * )src + 59 * line) + \
                   tm[61 * 64 + k] * *((type_src * )src + 61 * line) + tm[63 * 64 + k] * *((type_src * )src + 63 * line);  \
        } \
        \
        for (k = 0; k < 16; k++) \
        { \
            EO[k] = tm[2  * 64 + k] * *((type_src * )src + 2  * line) + tm[6  * 64 + k] * *((type_src * )src + 6  * line) + \
                    tm[10 * 64 + k] * *((type_src * )src + 10 * line) + tm[14 * 64 + k] * *((type_src * )src + 14 * line) + \
                    tm[18 * 64 + k] * *((type_src * )src + 18 * line) + tm[22 * 64 + k] * *((type_src * )src + 22 * line) + \
                    tm[26 * 64 + k] * *((type_src * )src + 26 * line) + tm[30 * 64 + k] * *((type_src * )src + 30 * line) + \
                    tm[34 * 64 + k] * *((type_src * )src + 34 * line) + tm[38 * 64 + k] * *((type_src * )src + 38 * line) + \
                    tm[42 * 64 + k] * *((type_src * )src + 42 * line) + tm[46 * 64 + k] * *((type_src * )src + 46 * line) + \
                    tm[50 * 64 + k] * *((type_src * )src + 50 * line) + tm[54 * 64 + k] * *((type_src * )src + 54 * line) + \
                    tm[58 * 64 + k] * *((type_src * )src + 58 * line) + tm[62 * 64 + k] * *((type_src * )src + 62 * line);  \
        } \
        \
        for (k = 0; k < 8; k++) \
        {\
            EEO[k] = tm[4  * 64 + k] * *((type_src * )src + 4  * line) + tm[12 * 64 + k] * *((type_src * )src + 12 * line) + \
                     tm[20 * 64 + k] * *((type_src * )src + 20 * line) + tm[28 * 64 + k] * *((type_src * )src + 28 * line) + \
                     tm[36 * 64 + k] * *((type_src * )src + 36 * line) + tm[44 * 64 + k] * *((type_src * )src + 44 * line) + \
                     tm[52 * 64 + k] * *((type_src * )src + 52 * line) + tm[60 * 64 + k] * *((type_src * )src + 60 * line);  \
        } \
        \
        for (k = 0; k<4; k++)\
        {\
            EEEO[k] = tm[8  * 64 + k] * *((type_src * )src + 8  * line) + tm[24 * 64 + k] * *((type_src * )src + 24 * line) + \
                      tm[40 * 64 + k] * *((type_src * )src + 40 * line) + tm[56 * 64 + k] * *((type_src * )src + 56 * line);  \
        }\
        EEEEO[0] = tm[16 * 64 + 0] * *((type_src * )src + 16 * line) + tm[48 * 64 + 0] * *((type_src * )src + 48 * line);\
        EEEEO[1] = tm[16 * 64 + 1] * *((type_src * )src + 16 * line) + tm[48 * 64 + 1] * *((type_src * )src + 48 * line);\
        EEEEE[0] = tm[0  * 64 + 0] * *((type_src * )src + 0        ) + tm[32 * 64 + 0] * *((type_src * )src + 32 * line);\
        EEEEE[1] = tm[0  * 64 + 1] * *((type_src * )src + 0        ) + tm[32 * 64 + 1] * *((type_src * )src + 32 * line);\
        \
        for (k = 0; k < 2; k++)\
        {\
            EEEE[k] = EEEEE[k] + EEEEO[k];\
            EEEE[k + 2] = EEEEE[1 - k] - EEEEO[1 - k];\
        }\
        for (k = 0; k < 4; k++)\
        {\
            EEE[k] = EEEE[k] + EEEO[k];\
            EEE[k + 4] = EEEE[3 - k] - EEEO[3 - k];\
        }\
        for (k = 0; k < 8; k++)\
        {\
            EE[k] = EEE[k] + EEO[k];\
            EE[k + 8] = EEE[7 - k] - EEO[7 - k];\
        }\
        for (k = 0; k < 16; k++)\
        {\
            E[k] = EE[k] + EO[k];\
            E[k + 16] = EE[15 - k] - EO[15 - k];\
        }\
        if (step == 0)\
        {\
            for (k = 0; k < 32; k++)\
            {\
                *((type_dst * )dst + k     ) = ITX_CLIP_32((E[k] + O[k] + add) >> shift);\
                *((type_dst * )dst + k + 32) = ITX_CLIP_32((E[31 - k] - O[31 - k] + add) >> shift);\
            }\
        }\
        else\
        {\
            for (k = 0; k < 32; k++)\
            {\
                *((type_dst * )dst + k     ) = ITX_CLIP((E[k] + O[k] + add) >> shift);\
                *((type_dst * )dst + k + 32) = ITX_CLIP((E[31 - k] - O[31 - k] + add) >> shift);\
            }\
        }\
        src = (type_src * )src + 1;\
        dst = (type_dst * )dst + tx_size;\
    }

    if (step == 0)
    {
        RUN_ITX_PB64(src, dst, s16, s32);
    }
    else
    {
        RUN_ITX_PB64(src, dst, s32, s16);
    }    
}

const XEVE_ITXB xeve_tbl_itxb[MAX_TR_LOG2] =
{
    xeve_itx_pb2b,
    xeve_itx_pb4b,
    xeve_itx_pb8b,
    xeve_itx_pb16b,
    xeve_itx_pb32b,
    xeve_itx_pb64b
};

static void xeve_itrans(XEVE_CTX * ctx, s16 *coef, int log2_cuw, int log2_cuh, int bit_depth)
{
    s32 tb[MAX_TR_DIM]; /* temp buffer */
    (*ctx->fn_itxb)[log2_cuh - 1](coef, tb, 0, 1 << log2_cuw, 0);
    (*ctx->fn_itxb)[log2_cuw - 1](tb, coef, (ITX_SHIFT1 + ITX_SHIFT2(bit_depth)), 1 << log2_cuh, 1);
}

static void xeve_dquant(s16 *coef, int log2_w, int log2_h, int scale, s32 offset, u8 shift)
{
    int i;
    s64 lev;

    const int ns_scale = ((log2_w + log2_h) & 1) ? 181 : 1;
    for(i = 0; i < (1 << (log2_w + log2_h)); i++)
    {
        lev = (coef[i] * (scale * (s64)ns_scale) + offset) >> shift;
        coef[i] = (s16)XEVE_CLIP3(-32768, 32767, lev);
    }
}

static void itdq_cu(XEVE_CTX * ctx, s16 *coef, int log2_w, int log2_h, int scale)
{
    s32 offset;
    u8 shift;
    s8 tr_shift;
    int log2_size = (log2_w + log2_h) >> 1;
    const int ns_shift = ((log2_w + log2_h) & 1) ? 8 : 0;

    int skip_w = 1 << log2_w;
    int skip_h = 1 << log2_h;
    int max_x = 0;
    int max_y = 0;
    s16* coef_tmp = coef;
    int i, j;
    int cuw = 1 << log2_w;
    int cuh = 1 << log2_h;
    int bit_depth = ctx->sps.bit_depth_luma_minus8 + 8;

    tr_shift = MAX_TX_DYNAMIC_RANGE - bit_depth - log2_size;
    shift = QUANT_IQUANT_SHIFT - QUANT_SHIFT - tr_shift;
    shift += ns_shift;
    offset = (shift == 0) ? 0 : (1 << (shift - 1));

    xeve_dquant(coef, log2_w, log2_h, scale, offset, shift);
    
    for(j = 0; j < cuh; j++)
    {
        for(i = 0; i < cuw; i++)
        {
            if(coef_tmp[i] != 0)
            {
                if(i > max_x)
                {
                    max_x = i;
                }
                if(j > max_y)
                {
                    max_y = j;
                }
            }
        }
        coef_tmp += cuw;
    }

    skip_w = cuw - 1 - max_x;
    skip_h = cuh - 1 - max_y;
    
    xeve_itrans(ctx, coef, log2_w, log2_h, bit_depth);
}

void xeve_itdq(XEVE_CTX* ctx, XEVE_CORE* core, s16 coef[N_C][MAX_CU_DIM], int nnz_sub[N_C][MAX_SUB_TB_NUM])
{
    s16 *coef_temp[N_C];
    s16 coef_temp_buf[N_C][MAX_TR_DIM];
    int i, j, c;
    int log2_w_sub = (core->log2_cuw > MAX_TR_LOG2) ? MAX_TR_LOG2 : core->log2_cuw;
    int log2_h_sub = (core->log2_cuh > MAX_TR_LOG2) ? MAX_TR_LOG2 : core->log2_cuh;
    int loop_w = (core->log2_cuw > MAX_TR_LOG2) ? (1 << (core->log2_cuw - MAX_TR_LOG2)) : 1;
    int loop_h = (core->log2_cuh > MAX_TR_LOG2) ? (1 << (core->log2_cuh - MAX_TR_LOG2)) : 1;
    int stride = (1 << core->log2_cuw);
    int sub_stride = (1 << log2_w_sub);
    u8  qp[N_C] = { core->qp_y, core->qp_u, core->qp_v };
    int scale = 0;
    int w_shift = (XEVE_GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc));
    int h_shift = (XEVE_GET_CHROMA_H_SHIFT(ctx->sps.chroma_format_idc));

    for(j = 0; j < loop_h; j++)
    {
        for(i = 0; i < loop_w; i++)
        {
            for(c = 0; c < N_C; c++)
            {
                if((c != 0) && !ctx->sps.chroma_format_idc)
                {
                    continue;
                }
                if(nnz_sub[c][(j << 1) | i])
                {
                    int pos_sub_x = c == 0 ? (i * (1 << (log2_w_sub))) : (i * (1 << (log2_w_sub - w_shift)));
                    int pos_sub_y = c == 0 ? j * (1 << (log2_h_sub)) * (stride) : j * (1 << (log2_h_sub - h_shift)) * (stride >> w_shift);

                    if(loop_h + loop_w > 2)
                    {
                        if(c == 0)
                        {
                            xeve_block_copy(coef[c] + pos_sub_x + pos_sub_y, stride, coef_temp_buf[c], sub_stride, log2_w_sub, log2_h_sub);
                        }
                        else
                        {
                            xeve_block_copy(coef[c] + pos_sub_x + pos_sub_y, stride >> w_shift, coef_temp_buf[c], sub_stride >> w_shift, log2_w_sub - w_shift, log2_h_sub - h_shift);
                        }
                        coef_temp[c] = coef_temp_buf[c];
                    }
                    else
                    {
                        coef_temp[c] = coef[c];
                    }

                    scale = xeve_tbl_dq_scale_b[qp[c] % 6] << (qp[c] / 6);

                    if(c == 0)
                    {
                        itdq_cu(ctx, coef_temp[c], log2_w_sub, log2_h_sub, scale);
                    }
                    else
                    {
                        itdq_cu(ctx, coef_temp[c], log2_w_sub - w_shift, log2_h_sub - h_shift, scale);
                    }

                    if(loop_h + loop_w > 2)
                    {
                        if(c == 0)
                        {
                            xeve_block_copy(coef_temp_buf[c], sub_stride, coef[c] + pos_sub_x + pos_sub_y, stride, log2_w_sub, log2_h_sub);
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
}

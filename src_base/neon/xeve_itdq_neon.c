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
#include "sse2neon.h"
#include "xeve_def.h"
#include "xeve_tbl.h"

#define MAX_TX_DYNAMIC_RANGE_32               31
#define MAX_TX_VAL_32                       2147483647
#define MIN_TX_VAL_32                      (-2147483647-1)

#define XEVE_ITX_CLIP_NEON(X, min, max)\
X = _mm_max_epi32(X, min_val);\
X = _mm_min_epi32(X, max_val);

#define XEVE_ITX_SHIFT_CLIP_NEON(dst, offset, shift, min, max)\
dst = _mm_add_epi32( dst, offset);\
dst = _mm_srai_epi32(dst, shift);\
dst = _mm_max_epi32( dst, min);\
dst = _mm_min_epi32( dst, max);




void xeve_itx_pb4b_neon(void *src, void *dst, int shift, int line, int step)
{
    int add = shift == 0 ? 0 : 1 << (shift - 1);

    if (step == 0)
    {
        if (line > 2)
        {
            s16 * pel_src = src;
            s32 * pel_dst = dst;
            __m128i r0, r1, r2, r3;
            __m128i a0, a1;
            __m128i e0, e1, o0, o1;
            __m128i v0, v1, v2, v3;
            __m128i t0, t1, t2, t3;
            const __m128i coef_0_13 = _mm_set1_epi32((xeve_tbl_tm4[3][0] << 16) | xeve_tbl_tm4[1][0]);
            const __m128i coef_1_13 = _mm_set1_epi32((xeve_tbl_tm4[3][1] << 16) | xeve_tbl_tm4[1][1]);
            const __m128i coef_1_02 = _mm_set1_epi32((xeve_tbl_tm4[2][1] << 16) | xeve_tbl_tm4[0][1]);
            const __m128i coef_0_02 = _mm_set1_epi32((xeve_tbl_tm4[0][0] << 16) | xeve_tbl_tm4[2][0]);
            __m128i max_val = _mm_set1_epi32(MAX_TX_VAL_32);
            __m128i min_val = _mm_set1_epi32(MIN_TX_VAL_32);
            int i_src1 = line;
            int i_src2 = i_src1 + i_src1;
            int i_src3 = i_src2 + i_src1;

            for (int j = 0; j < line; j += 4)
            {
                r0 = _mm_loadl_epi64((__m128i*)(pel_src + j));
                r1 = _mm_loadl_epi64((__m128i*)(pel_src + i_src1 + j));
                r2 = _mm_loadl_epi64((__m128i*)(pel_src + i_src2 + j));
                r3 = _mm_loadl_epi64((__m128i*)(pel_src + i_src3 + j));

                a0 = _mm_unpacklo_epi16(r0, r2);
                a1 = _mm_unpacklo_epi16(r1, r3);

                e0 = _mm_madd_epi16(a0, coef_0_02);
                e1 = _mm_madd_epi16(a0, coef_1_02);
                o0 = _mm_madd_epi16(a1, coef_0_13);
                o1 = _mm_madd_epi16(a1, coef_1_13);

                v0 = _mm_add_epi32(e0, o0);
                v3 = _mm_sub_epi32(e0, o0);
                v1 = _mm_add_epi32(e1, o1);
                v2 = _mm_sub_epi32(e1, o1);

                v0 = _mm_max_epi32(v0, min_val);
                v1 = _mm_max_epi32(v1, min_val);
                v2 = _mm_max_epi32(v2, min_val);
                v3 = _mm_max_epi32(v3, min_val);

                v0 = _mm_min_epi32(v0, max_val);
                v1 = _mm_min_epi32(v1, max_val);
                v2 = _mm_min_epi32(v2, max_val);
                v3 = _mm_min_epi32(v3, max_val);

                t0 = _mm_unpacklo_epi32(v0, v1);
                t2 = _mm_unpackhi_epi32(v0, v1);
                t1 = _mm_unpacklo_epi32(v2, v3);
                t3 = _mm_unpackhi_epi32(v2, v3);

                v0 = _mm_unpacklo_epi64(t0, t1);
                v1 = _mm_unpackhi_epi64(t0, t1);
                v2 = _mm_unpacklo_epi64(t2, t3);
                v3 = _mm_unpackhi_epi64(t2, t3);

                _mm_storeu_si128((__m128i*) pel_dst,       v0);
                _mm_storeu_si128((__m128i*)(pel_dst + 4),  v1);
                _mm_storeu_si128((__m128i*)(pel_dst + 8),  v2);
                _mm_storeu_si128((__m128i*)(pel_dst + 12), v3);

                pel_dst += 16;
            }
        }
        else
        {
            xeve_itx_pb4b(src, dst, shift, line, step);
        }
    }
    else
    { 
        if (line > 2)
        {
            s32* pel_src = src;
            s16* pel_dst = dst;
            __m128i r0, r1, r2, r3;
            __m128i a0, a1, b0, b1;
            __m128i e0, e1, o0, o1;
            __m128i v0, v1, v2, v3;
            __m128i t0, t1;
            const __m128i coef_0_13 = _mm_set1_epi64x(((s64)xeve_tbl_tm4[3][0] << 32) | xeve_tbl_tm4[1][0]);
            const __m128i coef_1_13 = _mm_set1_epi64x(((s64)xeve_tbl_tm4[3][1] << 32) | xeve_tbl_tm4[1][1]);
            const __m128i coef_1_02 = _mm_set1_epi64x(((s64)xeve_tbl_tm4[2][1] << 32) | xeve_tbl_tm4[0][1]);
            const __m128i coef_0_02 = _mm_set1_epi64x(((s64)xeve_tbl_tm4[0][0] << 32) | xeve_tbl_tm4[2][0]);
            const __m128i add_s2   = _mm_set1_epi32(add);
            __m128i max_val = _mm_set1_epi32(MAX_TX_VAL);
            __m128i min_val = _mm_set1_epi32(MIN_TX_VAL);
            int i_src1 = line;
            int i_src2 = i_src1 + i_src1;
            int i_src3 = i_src2 + i_src1;

            for (int j = 0; j < line; j += 4)
            {
                r0 = _mm_loadu_si128((__m128i*)(pel_src + j));
                r1 = _mm_loadu_si128((__m128i*)(pel_src + i_src1 + j));
                r2 = _mm_loadu_si128((__m128i*)(pel_src + i_src2 + j));
                r3 = _mm_loadu_si128((__m128i*)(pel_src + i_src3 + j));
                a0 = _mm_unpacklo_epi32(r0, r2);
                b0 = _mm_unpackhi_epi32(r0, r2);
                a1 = _mm_unpacklo_epi32(r1, r3);
                b1 = _mm_unpackhi_epi32(r1, r3);

                t0 = _mm_mullo_epi32(a0, coef_0_02);
                t1 = _mm_mullo_epi32(b0, coef_0_02);
                e0 = _mm_hadd_epi32(t0, t1);

                t0 = _mm_mullo_epi32(a0, coef_1_02);
                t1 = _mm_mullo_epi32(b0, coef_1_02);
                e1 = _mm_hadd_epi32(t0, t1);

                t0 = _mm_mullo_epi32(a1, coef_0_13);
                t1 = _mm_mullo_epi32(b1, coef_0_13);
                o0 = _mm_hadd_epi32(t0, t1);

                t0 = _mm_mullo_epi32(a1, coef_1_13);
                t1 = _mm_mullo_epi32(b1, coef_1_13);
                o1 = _mm_hadd_epi32(t0, t1);

                v0 = _mm_add_epi32(e0, o0);
                v3 = _mm_sub_epi32(e0, o0);
                v1 = _mm_add_epi32(e1, o1);
                v2 = _mm_sub_epi32(e1, o1);

                v0 = _mm_add_epi32(v0, add_s2);
                v1 = _mm_add_epi32(v1, add_s2);
                v2 = _mm_add_epi32(v2, add_s2);
                v3 = _mm_add_epi32(v3, add_s2);

                v0 = _mm_srai_epi32(v0, shift);
                v1 = _mm_srai_epi32(v1, shift);
                v2 = _mm_srai_epi32(v2, shift);
                v3 = _mm_srai_epi32(v3, shift);

                v0 = _mm_max_epi32(v0, min_val);
                v1 = _mm_max_epi32(v1, min_val);
                v2 = _mm_max_epi32(v2, min_val);
                v3 = _mm_max_epi32(v3, min_val);

                v0 = _mm_min_epi32(v0, max_val);
                v1 = _mm_min_epi32(v1, max_val);
                v2 = _mm_min_epi32(v2, max_val);
                v3 = _mm_min_epi32(v3, max_val);

                t0 = _mm_packs_epi32(v0, v2);
                t1 = _mm_packs_epi32(v1, v3);
                
                v0 = _mm_unpacklo_epi16(t0, t1);
                v1 = _mm_unpackhi_epi16(t0, t1);

                t0 = _mm_unpacklo_epi32(v0, v1);
                t1 = _mm_unpackhi_epi32(v0, v1);

                _mm_storeu_si128((__m128i*) pel_dst,      t0);
                _mm_storeu_si128((__m128i*)(pel_dst + 8), t1);

                pel_dst += 16;
            }
        }
        else
        {
            xeve_itx_pb4b(src, dst, shift, line, step);
        }
    }
}

void xeve_itx_pb8b_neon(void *src, void *dst, int shift, int line, int step)
{
    int add = shift == 0 ? 0 : 1 << (shift - 1);

    if (step == 0)
    {
        if (line > 2)
        {
            s16* pel_src = src;
            s32* pel_dst = dst;
            __m128i r0, r1, r2, r3, r4, r5, r6, r7;
            __m128i a0, a1, a2, a3;
            __m128i e0, e1, e2, e3, o0, o1, o2, o3, eo0, eo1, ee0, ee1;
            __m128i v0, v1, v2, v3, v4, v5, v6, v7;
            __m128i t0, t1, t2, t3;
            __m128i max_val = _mm_set1_epi32(MAX_TX_VAL_32);
            __m128i min_val = _mm_set1_epi32(MIN_TX_VAL_32);
            __m128i coef[4][4]; 

            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    coef[i][j] = _mm_set1_epi32(((s32)(xeve_tbl_tm8[j + 4][i]) << 16) | (xeve_tbl_tm8[j][i] & 0xFFFF));
                }
            }

            int i_src1 = line;
            int i_src2 = i_src1 + i_src1;
            int i_src3 = i_src2 + i_src1;
            int i_src4 = i_src3 + i_src1;
            int i_src5 = i_src4 + i_src1;
            int i_src6 = i_src5 + i_src1;
            int i_src7 = i_src6 + i_src1;

            for (int j = 0; j < line; j += 4)
            {
                r0 = _mm_loadl_epi64((__m128i*)(pel_src + j));
                r1 = _mm_loadl_epi64((__m128i*)(pel_src + i_src1 + j));
                r2 = _mm_loadl_epi64((__m128i*)(pel_src + i_src2 + j));
                r3 = _mm_loadl_epi64((__m128i*)(pel_src + i_src3 + j));
                r4 = _mm_loadl_epi64((__m128i*)(pel_src + i_src4 + j));
                r5 = _mm_loadl_epi64((__m128i*)(pel_src + i_src5 + j));
                r6 = _mm_loadl_epi64((__m128i*)(pel_src + i_src6 + j));
                r7 = _mm_loadl_epi64((__m128i*)(pel_src + i_src7 + j));

                a1 = _mm_unpacklo_epi16(r1, r5);
                a3 = _mm_unpacklo_epi16(r3, r7);

                t0 = _mm_madd_epi16(a1, coef[0][1]);
                t1 = _mm_madd_epi16(a3, coef[0][3]);
                o0 = _mm_add_epi32(t0, t1);

                t0 = _mm_madd_epi16(a1, coef[1][1]);
                t1 = _mm_madd_epi16(a3, coef[1][3]);
                o1 = _mm_add_epi32(t0, t1);

                t0 = _mm_madd_epi16(a1, coef[2][1]);
                t1 = _mm_madd_epi16(a3, coef[2][3]);
                o2 = _mm_add_epi32(t0, t1);

                t0 = _mm_madd_epi16(a1, coef[3][1]);
                t1 = _mm_madd_epi16(a3, coef[3][3]);
                o3 = _mm_add_epi32(t0, t1);

                a0 = _mm_unpacklo_epi16(r0, r4);
                a2 = _mm_unpacklo_epi16(r2, r6);

                eo0 = _mm_madd_epi16(a2, coef[0][2]);
                eo1 = _mm_madd_epi16(a2, coef[1][2]);
                ee0 = _mm_madd_epi16(a0, coef[0][0]);
                ee1 = _mm_madd_epi16(a0, coef[1][0]);

                e0 = _mm_add_epi32(ee0, eo0);
                e3 = _mm_sub_epi32(ee0, eo0);
                e1 = _mm_add_epi32(ee1, eo1);
                e2 = _mm_sub_epi32(ee1, eo1);

                v0 = _mm_add_epi32(e0, o0);
                v7 = _mm_sub_epi32(e0, o0);
                v1 = _mm_add_epi32(e1, o1);
                v6 = _mm_sub_epi32(e1, o1);
                v2 = _mm_add_epi32(e2, o2);
                v5 = _mm_sub_epi32(e2, o2);
                v3 = _mm_add_epi32(e3, o3);
                v4 = _mm_sub_epi32(e3, o3);

                XEVE_ITX_CLIP_NEON(v0, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v1, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v2, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v3, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v4, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v5, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v6, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v7, min_val, max_val);

                t0 = _mm_unpacklo_epi32(v0, v1);
                t2 = _mm_unpackhi_epi32(v0, v1);
                t1 = _mm_unpacklo_epi32(v2, v3);
                t3 = _mm_unpackhi_epi32(v2, v3);

                v0 = _mm_unpacklo_epi64(t0, t1);
                v1 = _mm_unpackhi_epi64(t0, t1);
                v2 = _mm_unpacklo_epi64(t2, t3);
                v3 = _mm_unpackhi_epi64(t2, t3);

                t0 = _mm_unpacklo_epi32(v4, v5);
                t2 = _mm_unpackhi_epi32(v4, v5);
                t1 = _mm_unpacklo_epi32(v6, v7);
                t3 = _mm_unpackhi_epi32(v6, v7);

                v4 = _mm_unpacklo_epi64(t0, t1);
                v5 = _mm_unpackhi_epi64(t0, t1);
                v6 = _mm_unpacklo_epi64(t2, t3);
                v7 = _mm_unpackhi_epi64(t2, t3);

                _mm_storeu_si128((__m128i*)(pel_dst     ), v0);
                _mm_storeu_si128((__m128i*)(pel_dst + 4 ), v4);
                _mm_storeu_si128((__m128i*)(pel_dst + 8),  v1);
                _mm_storeu_si128((__m128i*)(pel_dst + 12), v5);
                _mm_storeu_si128((__m128i*)(pel_dst + 16), v2);
                _mm_storeu_si128((__m128i*)(pel_dst + 20), v6);
                _mm_storeu_si128((__m128i*)(pel_dst + 24), v3);
                _mm_storeu_si128((__m128i*)(pel_dst + 28), v7);

                pel_dst += 32;
            }
        }
        else
        {
            xeve_itx_pb8b(src, dst, shift, line, step);
        }
    }
    else
    {
        if (line > 2)
        {
            s32 * pel_src = src;
            s16 * pel_dst = dst;
            __m128i r0, r1, r2, r3, r4, r5, r6, r7;
            __m128i a0, a1, a2, a3, b0, b1, b2, b3;
            __m128i e0, e1, e2, e3, o0, o1, o2, o3, eo0, eo1, ee0, ee1;
            __m128i v0, v1, v2, v3, v4, v5, v6, v7;
            __m128i t0, t1, t2, t3;
            const __m128i max_val = _mm_set1_epi32(MAX_TX_VAL_32);
            const __m128i min_val = _mm_set1_epi32(MIN_TX_VAL_32);
            const __m128i add_s2 = _mm_set1_epi32(add);
            __m128i coef[4][4];

            for (int i = 0; i < 4; i++)
            {
                for (int j = 0; j < 4; j++)
                {
                    coef[i][j] = _mm_set1_epi64x(((s64)(xeve_tbl_tm8[j + 4][i]) << 32) | (xeve_tbl_tm8[j][i] & 0xFFFFFFFF));
                }
            }

            int i_src1 = line;
            int i_src2 = i_src1 + i_src1;
            int i_src3 = i_src2 + i_src1;
            int i_src4 = i_src3 + i_src1;
            int i_src5 = i_src4 + i_src1;
            int i_src6 = i_src5 + i_src1;
            int i_src7 = i_src6 + i_src1;

            for (int j = 0; j < line; j += 4)
            {
                r0 = _mm_loadu_si128((__m128i*)(pel_src + j));
                r1 = _mm_loadu_si128((__m128i*)(pel_src + i_src1 + j));
                r2 = _mm_loadu_si128((__m128i*)(pel_src + i_src2 + j));
                r3 = _mm_loadu_si128((__m128i*)(pel_src + i_src3 + j));
                r4 = _mm_loadu_si128((__m128i*)(pel_src + i_src4 + j));
                r5 = _mm_loadu_si128((__m128i*)(pel_src + i_src5 + j));
                r6 = _mm_loadu_si128((__m128i*)(pel_src + i_src6 + j));
                r7 = _mm_loadu_si128((__m128i*)(pel_src + i_src7 + j));

                a1 = _mm_unpacklo_epi32(r1, r5);
                b1 = _mm_unpackhi_epi32(r1, r5);
                a3 = _mm_unpacklo_epi32(r3, r7);
                b3 = _mm_unpackhi_epi32(r3, r7);

                t0 = _mm_mullo_epi32(a1, coef[0][1]);
                t1 = _mm_mullo_epi32(b1, coef[0][1]);
                t2 = _mm_hadd_epi32(t0, t1);
                t0 = _mm_mullo_epi32(a3, coef[0][3]);
                t1 = _mm_mullo_epi32(b3, coef[0][3]);
                t3 = _mm_hadd_epi32(t0, t1);
                o0 = _mm_add_epi32(t2, t3);

                t0 = _mm_mullo_epi32(a1, coef[1][1]);
                t1 = _mm_mullo_epi32(b1, coef[1][1]);
                t2 = _mm_hadd_epi32(t0, t1);
                t0 = _mm_mullo_epi32(a3, coef[1][3]);
                t1 = _mm_mullo_epi32(b3, coef[1][3]);
                t3 = _mm_hadd_epi32(t0, t1);
                o1 = _mm_add_epi32(t2, t3);

                t0 = _mm_mullo_epi32(a1, coef[2][1]);
                t1 = _mm_mullo_epi32(b1, coef[2][1]);
                t2 = _mm_hadd_epi32(t0, t1);
                t0 = _mm_mullo_epi32(a3, coef[2][3]);
                t1 = _mm_mullo_epi32(b3, coef[2][3]);
                t3 = _mm_hadd_epi32(t0, t1);
                o2 = _mm_add_epi32(t2, t3);

                t0 = _mm_mullo_epi32(a1, coef[3][1]);
                t1 = _mm_mullo_epi32(b1, coef[3][1]);
                t2 = _mm_hadd_epi32(t0, t1);
                t0 = _mm_mullo_epi32(a3, coef[3][3]);
                t1 = _mm_mullo_epi32(b3, coef[3][3]);
                t3 = _mm_hadd_epi32(t0, t1);
                o3 = _mm_add_epi32(t2, t3);

                a0 = _mm_unpacklo_epi32(r0, r4);
                b0 = _mm_unpackhi_epi32(r0, r4);
                a2 = _mm_unpacklo_epi32(r2, r6);
                b2 = _mm_unpackhi_epi32(r2, r6);

                t0 = _mm_mullo_epi32(a2, coef[0][2]);
                t1 = _mm_mullo_epi32(b2, coef[0][2]);
                eo0 = _mm_hadd_epi32(t0, t1);

                t0 = _mm_mullo_epi32(a2, coef[1][2]);
                t1 = _mm_mullo_epi32(b2, coef[1][2]);
                eo1 = _mm_hadd_epi32(t0, t1);

                t0 = _mm_mullo_epi32(a0, coef[0][0]);
                t1 = _mm_mullo_epi32(b0, coef[0][0]);
                ee0 = _mm_hadd_epi32(t0, t1);

                t0 = _mm_mullo_epi32(a0, coef[1][0]);
                t1 = _mm_mullo_epi32(b0, coef[1][0]);
                ee1 = _mm_hadd_epi32(t0, t1);

                e0 = _mm_add_epi32(ee0, eo0);
                e3 = _mm_sub_epi32(ee0, eo0);
                e1 = _mm_add_epi32(ee1, eo1);
                e2 = _mm_sub_epi32(ee1, eo1);

                v0 = _mm_add_epi32(e0, o0);
                v7 = _mm_sub_epi32(e0, o0);
                v1 = _mm_add_epi32(e1, o1);
                v6 = _mm_sub_epi32(e1, o1);
                v2 = _mm_add_epi32(e2, o2);
                v5 = _mm_sub_epi32(e2, o2);
                v3 = _mm_add_epi32(e3, o3);
                v4 = _mm_sub_epi32(e3, o3);

                XEVE_ITX_SHIFT_CLIP_NEON(v0, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v1, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v2, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v3, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v4, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v5, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v6, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v7, add_s2, shift, min_val, max_val);

                t0 = _mm_packs_epi32(v0, v4);
                t1 = _mm_packs_epi32(v1, v5);
                t2 = _mm_packs_epi32(v2, v6);
                t3 = _mm_packs_epi32(v3, v7);

                v0 = _mm_unpacklo_epi16(t0, t1);
                v1 = _mm_unpacklo_epi16(t2, t3);
                v2 = _mm_unpackhi_epi16(t0, t1);
                v3 = _mm_unpackhi_epi16(t2, t3);

                t0 = _mm_unpacklo_epi32(v0, v1);
                t1 = _mm_unpacklo_epi32(v2, v3);
                t2 = _mm_unpackhi_epi32(v0, v1);
                t3 = _mm_unpackhi_epi32(v2, v3);

                v0 = _mm_unpacklo_epi64(t0, t1);
                v1 = _mm_unpackhi_epi64(t0, t1);
                v2 = _mm_unpacklo_epi64(t2, t3);
                v3 = _mm_unpackhi_epi64(t2, t3);

                _mm_storeu_si128((__m128i*) pel_dst,       v0);
                _mm_storeu_si128((__m128i*)(pel_dst + 8),  v1);
                _mm_storeu_si128((__m128i*)(pel_dst + 16), v2);
                _mm_storeu_si128((__m128i*)(pel_dst + 24), v3);
                pel_dst += 32;
            }
        }
        else
        {
            xeve_itx_pb8b(src, dst, shift, line, step);
        }
    }
}

void xeve_itx_pb16b_neon(void *src, void *dst, int shift, int line, int step)
{
    int add = shift == 0 ? 0 : 1 << (shift - 1);

    if (step == 0)
    {
        if (line > 2)
        {
            s16* pel_src = src;
            s32* pel_dst = dst;
            __m128i r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15 ;
            __m128i a0, a1, a2, a3, a4, a5, a6, a7;
            __m128i o0, o1, o2, o3, o4, o5, o6, o7;
            __m128i e0, e1, e2, e3, e4, e5, e6, e7;
            __m128i eo0, eo1, eo2, eo3, ee0, ee1, ee2, ee3;
            __m128i eeo0, eeo1, eee0, eee1;
            __m128i v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15;
            __m128i t0, t1, t2, t3, t5, t6, t7;
            __m128i max_val = _mm_set1_epi32(MAX_TX_VAL_32);
            __m128i min_val = _mm_set1_epi32(MIN_TX_VAL_32);
            __m128i coef[8][8];

            for (int i = 0; i < 8; i++)
            {
                for (int j = 0; j < 8; j++)
                {
                    coef[i][j] = _mm_set1_epi32(((s32)(xeve_tbl_tm16[j + 8][i]) << 16) | (xeve_tbl_tm16[j][i] & 0xFFFF));
                }
            }

            int i_src1  = line;
            int i_src2  = i_src1  + i_src1;
            int i_src3  = i_src2  + i_src1;
            int i_src4  = i_src3  + i_src1;
            int i_src5  = i_src4  + i_src1;
            int i_src6  = i_src5  + i_src1;
            int i_src7  = i_src6  + i_src1;
            int i_src8  = i_src7  + i_src1;
            int i_src9  = i_src8  + i_src1;
            int i_src10 = i_src9  + i_src1;
            int i_src11 = i_src10 + i_src1;
            int i_src12 = i_src11 + i_src1;
            int i_src13 = i_src12 + i_src1;
            int i_src14 = i_src13 + i_src1;
            int i_src15 = i_src14 + i_src1;


            for (int j = 0; j < line; j += 4)
            {
                r0  = _mm_loadl_epi64((__m128i*)(pel_src + j));
                r1  = _mm_loadl_epi64((__m128i*)(pel_src + i_src1  + j));
                r2  = _mm_loadl_epi64((__m128i*)(pel_src + i_src2  + j));
                r3  = _mm_loadl_epi64((__m128i*)(pel_src + i_src3  + j));
                r4  = _mm_loadl_epi64((__m128i*)(pel_src + i_src4  + j));
                r5  = _mm_loadl_epi64((__m128i*)(pel_src + i_src5  + j));
                r6  = _mm_loadl_epi64((__m128i*)(pel_src + i_src6  + j));
                r7  = _mm_loadl_epi64((__m128i*)(pel_src + i_src7  + j));
                r8  = _mm_loadl_epi64((__m128i*)(pel_src + i_src8  + j));
                r9  = _mm_loadl_epi64((__m128i*)(pel_src + i_src9  + j));
                r10 = _mm_loadl_epi64((__m128i*)(pel_src + i_src10 + j));
                r11 = _mm_loadl_epi64((__m128i*)(pel_src + i_src11 + j));
                r12 = _mm_loadl_epi64((__m128i*)(pel_src + i_src12 + j));
                r13 = _mm_loadl_epi64((__m128i*)(pel_src + i_src13 + j));
                r14 = _mm_loadl_epi64((__m128i*)(pel_src + i_src14 + j));
                r15 = _mm_loadl_epi64((__m128i*)(pel_src + i_src15 + j));

                a1 = _mm_unpacklo_epi16(r1, r9);
                a3 = _mm_unpacklo_epi16(r3, r11);
                a5 = _mm_unpacklo_epi16(r5, r13);
                a7 = _mm_unpacklo_epi16(r7, r15);

#define XEVE_ITX16_O(dst, idx) \
t1 = _mm_madd_epi16(a1, coef[idx][1]);\
t3 = _mm_madd_epi16(a3, coef[idx][3]);\
t5 = _mm_madd_epi16(a5, coef[idx][5]);\
t7 = _mm_madd_epi16(a7, coef[idx][7]);\
v0 = _mm_add_epi32(t1, t3);\
v1 = _mm_add_epi32(t5, t7);\
dst = _mm_add_epi32(v0, v1);

                XEVE_ITX16_O(o0, 0);
                XEVE_ITX16_O(o1, 1);
                XEVE_ITX16_O(o2, 2);
                XEVE_ITX16_O(o3, 3);
                XEVE_ITX16_O(o4, 4);
                XEVE_ITX16_O(o5, 5);
                XEVE_ITX16_O(o6, 6);
                XEVE_ITX16_O(o7, 7);
#undef XEVE_ITX16_O


                a2 = _mm_unpacklo_epi16(r2, r10);
                a6 = _mm_unpacklo_epi16(r6, r14);

#define XEVE_ITX16_EO(dst, idx) \
t2  = _mm_madd_epi16(a2, coef[idx][2]);\
t6  = _mm_madd_epi16(a6, coef[idx][6]);\
dst = _mm_add_epi32(t2, t6);

                XEVE_ITX16_EO(eo0, 0);
                XEVE_ITX16_EO(eo1, 1);
                XEVE_ITX16_EO(eo2, 2);
                XEVE_ITX16_EO(eo3, 3);

#undef XEVE_ITX16_EO

                a4 = _mm_unpacklo_epi16(r4, r12);
                a0 = _mm_unpacklo_epi16(r0, r8);

                eeo0 = _mm_madd_epi16(a4, coef[0][4]);
                eeo1 = _mm_madd_epi16(a4, coef[1][4]);
                eee0 = _mm_madd_epi16(a0, coef[0][0]);
                eee1 = _mm_madd_epi16(a0, coef[1][0]);

                ee0 = _mm_add_epi32(eee0, eeo0);
                ee1 = _mm_add_epi32(eee1, eeo1);
                ee2 = _mm_sub_epi32(eee1, eeo1);
                ee3 = _mm_sub_epi32(eee0, eeo0);

                e0 = _mm_add_epi32(ee0, eo0);
                e1 = _mm_add_epi32(ee1, eo1);
                e2 = _mm_add_epi32(ee2, eo2);
                e3 = _mm_add_epi32(ee3, eo3);
                e4 = _mm_sub_epi32(ee3, eo3);
                e5 = _mm_sub_epi32(ee2, eo2);
                e6 = _mm_sub_epi32(ee1, eo1);
                e7 = _mm_sub_epi32(ee0, eo0);

                v0  = _mm_add_epi32(e0, o0);
                v1  = _mm_add_epi32(e1, o1);
                v2  = _mm_add_epi32(e2, o2);
                v3  = _mm_add_epi32(e3, o3);
                v4  = _mm_add_epi32(e4, o4);
                v5  = _mm_add_epi32(e5, o5);
                v6  = _mm_add_epi32(e6, o6);
                v7  = _mm_add_epi32(e7, o7);
                v8  = _mm_sub_epi32(e7, o7);
                v9  = _mm_sub_epi32(e6, o6);
                v10 = _mm_sub_epi32(e5, o5);
                v11 = _mm_sub_epi32(e4, o4);
                v12 = _mm_sub_epi32(e3, o3);
                v13 = _mm_sub_epi32(e2, o2);
                v14 = _mm_sub_epi32(e1, o1);
                v15 = _mm_sub_epi32(e0, o0);

                XEVE_ITX_CLIP_NEON(v0 , min_val, max_val);
                XEVE_ITX_CLIP_NEON(v1 , min_val, max_val);
                XEVE_ITX_CLIP_NEON(v2 , min_val, max_val);
                XEVE_ITX_CLIP_NEON(v3 , min_val, max_val);
                XEVE_ITX_CLIP_NEON(v4 , min_val, max_val);
                XEVE_ITX_CLIP_NEON(v5 , min_val, max_val);
                XEVE_ITX_CLIP_NEON(v6 , min_val, max_val);
                XEVE_ITX_CLIP_NEON(v7 , min_val, max_val);
                XEVE_ITX_CLIP_NEON(v8 , min_val, max_val);
                XEVE_ITX_CLIP_NEON(v9 , min_val, max_val);
                XEVE_ITX_CLIP_NEON(v10, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v11, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v12, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v13, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v14, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v15, min_val, max_val);

#define XEVE_ITDQ_TRANSPOS_NEON(s0, s1, s2, s3, t0, t1, t2, t3)\
t0 = _mm_unpacklo_epi32(s0, s1);\
t2 = _mm_unpackhi_epi32(s0, s1);\
t1 = _mm_unpacklo_epi32(s2, s3);\
t3 = _mm_unpackhi_epi32(s2, s3);\
\
s0 = _mm_unpacklo_epi64(t0, t1);\
s1 = _mm_unpackhi_epi64(t0, t1);\
s2 = _mm_unpacklo_epi64(t2, t3);\
s3 = _mm_unpackhi_epi64(t2, t3);
                XEVE_ITDQ_TRANSPOS_NEON(v0,  v1,  v2,  v3,  t0, t1, t2, t3);
                XEVE_ITDQ_TRANSPOS_NEON(v4,  v5,  v6,  v7,  t0, t1, t2, t3);
                XEVE_ITDQ_TRANSPOS_NEON(v8,  v9,  v10, v11, t0, t1, t2, t3);
                XEVE_ITDQ_TRANSPOS_NEON(v12, v13, v14, v15, t0, t1, t2, t3);
#undef XEVE_ITDQ_TRANSPOS_NEON

                _mm_storeu_si128((__m128i*)(pel_dst),      v0 );
                _mm_storeu_si128((__m128i*)(pel_dst + 4),  v4 );
                _mm_storeu_si128((__m128i*)(pel_dst + 8),  v8 );
                _mm_storeu_si128((__m128i*)(pel_dst + 12), v12);
                _mm_storeu_si128((__m128i*)(pel_dst + 16), v1 );
                _mm_storeu_si128((__m128i*)(pel_dst + 20), v5 );
                _mm_storeu_si128((__m128i*)(pel_dst + 24), v9 );
                _mm_storeu_si128((__m128i*)(pel_dst + 28), v13);
                _mm_storeu_si128((__m128i*)(pel_dst + 32), v2 );
                _mm_storeu_si128((__m128i*)(pel_dst + 36), v6 );
                _mm_storeu_si128((__m128i*)(pel_dst + 40), v10);
                _mm_storeu_si128((__m128i*)(pel_dst + 44), v14);
                _mm_storeu_si128((__m128i*)(pel_dst + 48), v3 );
                _mm_storeu_si128((__m128i*)(pel_dst + 52), v7 );
                _mm_storeu_si128((__m128i*)(pel_dst + 56), v11);
                _mm_storeu_si128((__m128i*)(pel_dst + 60), v15);

                pel_dst += 64;
            }
        }
        else
        {
            xeve_itx_pb16b(src, dst, shift, line, step);
        }
    }
    else
    {
        if (line > 2)
        {
            s32 * pel_src = src;
            s16 * pel_dst = dst;
            __m128i r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15;
            __m128i a0, a1, a2, a3, a4, a5, a6, a7;
            __m128i b0, b1, b2, b3, b4, b5, b6, b7;
            __m128i o0, o1, o2, o3, o4, o5, o6, o7;
            __m128i e0, e1, e2, e3, e4, e5, e6, e7;
            __m128i eo0, eo1, eo2, eo3, ee0, ee1, ee2, ee3;
            __m128i eeo0, eeo1, eee0, eee1;
            __m128i v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15;
            __m128i t0, t1, t2, t3, t4, t5, t6, t7;
            const __m128i max_val = _mm_set1_epi32(MAX_TX_VAL_32);
            const __m128i min_val = _mm_set1_epi32(MIN_TX_VAL_32);
            const __m128i add_s2 = _mm_set1_epi32(add);
            __m128i coef[8][8];

            for (int i = 0; i < 8; i++)
            {
                for (int j = 0; j < 8; j++)
                {
                    coef[i][j] = _mm_set1_epi64x(((s64)(xeve_tbl_tm16[j + 8][i]) << 32) | (xeve_tbl_tm16[j][i] & 0xFFFFFFFF));
                }
            }

            int i_src1  = line;
            int i_src2  = i_src1  + i_src1;
            int i_src3  = i_src2  + i_src1;
            int i_src4  = i_src3  + i_src1;
            int i_src5  = i_src4  + i_src1;
            int i_src6  = i_src5  + i_src1;
            int i_src7  = i_src6  + i_src1;
            int i_src8  = i_src7  + i_src1;
            int i_src9  = i_src8  + i_src1;
            int i_src10 = i_src9  + i_src1;
            int i_src11 = i_src10 + i_src1;
            int i_src12 = i_src11 + i_src1;
            int i_src13 = i_src12 + i_src1;
            int i_src14 = i_src13 + i_src1;
            int i_src15 = i_src14 + i_src1;

            for (int j = 0; j < line; j += 4)
            {
                r0  = _mm_loadu_si128((__m128i*)(pel_src +           j));
                r1  = _mm_loadu_si128((__m128i*)(pel_src + i_src1  + j));
                r2  = _mm_loadu_si128((__m128i*)(pel_src + i_src2  + j));
                r3  = _mm_loadu_si128((__m128i*)(pel_src + i_src3  + j));
                r4  = _mm_loadu_si128((__m128i*)(pel_src + i_src4  + j));
                r5  = _mm_loadu_si128((__m128i*)(pel_src + i_src5  + j));
                r6  = _mm_loadu_si128((__m128i*)(pel_src + i_src6  + j));
                r7  = _mm_loadu_si128((__m128i*)(pel_src + i_src7  + j));
                r8  = _mm_loadu_si128((__m128i*)(pel_src + i_src8  + j));
                r9  = _mm_loadu_si128((__m128i*)(pel_src + i_src9  + j));
                r10 = _mm_loadu_si128((__m128i*)(pel_src + i_src10 + j));
                r11 = _mm_loadu_si128((__m128i*)(pel_src + i_src11 + j));
                r12 = _mm_loadu_si128((__m128i*)(pel_src + i_src12 + j));
                r13 = _mm_loadu_si128((__m128i*)(pel_src + i_src13 + j));
                r14 = _mm_loadu_si128((__m128i*)(pel_src + i_src14 + j));
                r15 = _mm_loadu_si128((__m128i*)(pel_src + i_src15 + j));

                a1 = _mm_unpacklo_epi32(r1, r9);
                b1 = _mm_unpackhi_epi32(r1, r9);
                a3 = _mm_unpacklo_epi32(r3, r11);
                b3 = _mm_unpackhi_epi32(r3, r11);
                a5 = _mm_unpacklo_epi32(r5, r13);
                b5 = _mm_unpackhi_epi32(r5, r13);
                a7 = _mm_unpacklo_epi32(r7, r15);
                b7 = _mm_unpackhi_epi32(r7, r15);

#define XEVE_ITX16_0_32B(dst, idx)\
t0 = _mm_mullo_epi32(a1, coef[idx][1]);\
t1 = _mm_mullo_epi32(b1, coef[idx][1]);\
v0 = _mm_hadd_epi32(t0, t1);\
t0 = _mm_mullo_epi32(a3, coef[idx][3]);\
t1 = _mm_mullo_epi32(b3, coef[idx][3]);\
v1 = _mm_hadd_epi32(t0, t1);\
t0 = _mm_mullo_epi32(a5, coef[idx][5]);\
t1 = _mm_mullo_epi32(b5, coef[idx][5]);\
v2 = _mm_hadd_epi32(t0, t1);\
t0 = _mm_mullo_epi32(a7, coef[idx][7]);\
t1 = _mm_mullo_epi32(b7, coef[idx][7]);\
v3 = _mm_hadd_epi32(t0, t1);\
t0 = _mm_add_epi32(v0, v1);\
t1 = _mm_add_epi32(v2, v3);\
dst = _mm_add_epi32(t0, t1);

                XEVE_ITX16_0_32B(o0, 0);
                XEVE_ITX16_0_32B(o1, 1);
                XEVE_ITX16_0_32B(o2, 2);
                XEVE_ITX16_0_32B(o3, 3);
                XEVE_ITX16_0_32B(o4, 4);
                XEVE_ITX16_0_32B(o5, 5);
                XEVE_ITX16_0_32B(o6, 6);
                XEVE_ITX16_0_32B(o7, 7);
#undef XEVE_ITX16_0_32B

                a2 = _mm_unpacklo_epi32(r2, r10);
                b2 = _mm_unpackhi_epi32(r2, r10);
                a6 = _mm_unpacklo_epi32(r6, r14);
                b6 = _mm_unpackhi_epi32(r6, r14);

#define XEVE_ITX16_EO_32B(dst, idx)\
t0 = _mm_mullo_epi32(a2, coef[idx][2]);\
t1 = _mm_mullo_epi32(b2, coef[idx][2]);\
v0 = _mm_hadd_epi32(t0, t1);\
t0 = _mm_mullo_epi32(a6, coef[idx][6]);\
t1 = _mm_mullo_epi32(b6, coef[idx][6]);\
v1 = _mm_hadd_epi32(t0, t1);\
dst = _mm_add_epi32(v0, v1);

                t0 = _mm_mullo_epi32(a2, coef[0][2]);
                t1 = _mm_mullo_epi32(b2, coef[0][2]);
                v0 = _mm_hadd_epi32(t0, t1);
                t0 = _mm_mullo_epi32(a6, coef[0][6]);
                t1 = _mm_mullo_epi32(b6, coef[0][6]);
                v1 = _mm_hadd_epi32(t0, t1);
                eo0 = _mm_add_epi32(v0, v1);

                XEVE_ITX16_EO_32B(eo1, 1);
                XEVE_ITX16_EO_32B(eo2, 2);
                XEVE_ITX16_EO_32B(eo3, 3);
#undef XEVE_ITX16_EO_32B

                a0 = _mm_unpacklo_epi32(r0, r8);
                b0 = _mm_unpackhi_epi32(r0, r8);
                a4 = _mm_unpacklo_epi32(r4, r12);
                b4 = _mm_unpackhi_epi32(r4, r12);

                t0   = _mm_mullo_epi32(a4, coef[0][4]);
                t1   = _mm_mullo_epi32(b4, coef[0][4]);
                eeo0 = _mm_hadd_epi32(t0, t1);
                t0   = _mm_mullo_epi32(a4, coef[1][4]);
                t1   = _mm_mullo_epi32(b4, coef[1][4]);
                eeo1 = _mm_hadd_epi32(t0, t1);

                t0   = _mm_mullo_epi32(a0, coef[0][0]);
                t1   = _mm_mullo_epi32(b0, coef[0][0]);
                eee0 = _mm_hadd_epi32(t0, t1);
                t0   = _mm_mullo_epi32(a0, coef[1][0]);
                t1   = _mm_mullo_epi32(b0, coef[1][0]);
                eee1 = _mm_hadd_epi32(t0, t1);

                ee0 = _mm_add_epi32(eee0, eeo0);
                ee1 = _mm_add_epi32(eee1, eeo1);
                ee2 = _mm_sub_epi32(eee1, eeo1);
                ee3 = _mm_sub_epi32(eee0, eeo0);

                e0 = _mm_add_epi32(ee0, eo0);
                e1 = _mm_add_epi32(ee1, eo1);
                e2 = _mm_add_epi32(ee2, eo2);
                e3 = _mm_add_epi32(ee3, eo3);
                e4 = _mm_sub_epi32(ee3, eo3);
                e5 = _mm_sub_epi32(ee2, eo2);
                e6 = _mm_sub_epi32(ee1, eo1);
                e7 = _mm_sub_epi32(ee0, eo0);

                v0 = _mm_add_epi32(e0, o0);
                v1 = _mm_add_epi32(e1, o1);
                v2 = _mm_add_epi32(e2, o2);
                v3 = _mm_add_epi32(e3, o3);
                v4 = _mm_add_epi32(e4, o4);
                v5 = _mm_add_epi32(e5, o5);
                v6 = _mm_add_epi32(e6, o6);
                v7 = _mm_add_epi32(e7, o7);
                v8 = _mm_sub_epi32(e7, o7);
                v9 = _mm_sub_epi32(e6, o6);
                v10 = _mm_sub_epi32(e5, o5);
                v11 = _mm_sub_epi32(e4, o4);
                v12 = _mm_sub_epi32(e3, o3);
                v13 = _mm_sub_epi32(e2, o2);
                v14 = _mm_sub_epi32(e1, o1);
                v15 = _mm_sub_epi32(e0, o0);

                XEVE_ITX_SHIFT_CLIP_NEON(v0 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v1 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v2 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v3 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v4 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v5 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v6 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v7 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v8 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v9 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v10, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v11, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v12, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v13, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v14, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v15, add_s2, shift, min_val, max_val);

                t0 = _mm_packs_epi32(v0, v8 );
                t1 = _mm_packs_epi32(v1, v9 );
                t2 = _mm_packs_epi32(v2, v10);
                t3 = _mm_packs_epi32(v3, v11);
                t4 = _mm_packs_epi32(v4, v12);
                t5 = _mm_packs_epi32(v5, v13);
                t6 = _mm_packs_epi32(v6, v14);
                t7 = _mm_packs_epi32(v7, v15);

                v0 = _mm_unpacklo_epi16(t0, t1);
                v1 = _mm_unpacklo_epi16(t2, t3);
                v2 = _mm_unpacklo_epi16(t4, t5);
                v3 = _mm_unpacklo_epi16(t6, t7);
                v4 = _mm_unpackhi_epi16(t0, t1);
                v5 = _mm_unpackhi_epi16(t2, t3);
                v6 = _mm_unpackhi_epi16(t4, t5);
                v7 = _mm_unpackhi_epi16(t6, t7);

                t0 = _mm_unpacklo_epi32(v0, v1);
                t1 = _mm_unpacklo_epi32(v2, v3);
                t2 = _mm_unpacklo_epi32(v4, v5);
                t3 = _mm_unpacklo_epi32(v6, v7);
                t4 = _mm_unpackhi_epi32(v0, v1);
                t5 = _mm_unpackhi_epi32(v2, v3);
                t6 = _mm_unpackhi_epi32(v4, v5);
                t7 = _mm_unpackhi_epi32(v6, v7);

                v0 = _mm_unpacklo_epi64(t0, t1);
                v1 = _mm_unpacklo_epi64(t2, t3);
                v2 = _mm_unpackhi_epi64(t0, t1);
                v3 = _mm_unpackhi_epi64(t2, t3);
                v4 = _mm_unpacklo_epi64(t4, t5);
                v5 = _mm_unpacklo_epi64(t6, t7);
                v6 = _mm_unpackhi_epi64(t4, t5);
                v7 = _mm_unpackhi_epi64(t6, t7);

                _mm_storeu_si128((__m128i*) pel_dst,       v0);
                _mm_storeu_si128((__m128i*)(pel_dst + 8),  v1);
                _mm_storeu_si128((__m128i*)(pel_dst + 16), v2);
                _mm_storeu_si128((__m128i*)(pel_dst + 24), v3);
                _mm_storeu_si128((__m128i*)(pel_dst + 32), v4);
                _mm_storeu_si128((__m128i*)(pel_dst + 40), v5);
                _mm_storeu_si128((__m128i*)(pel_dst + 48), v6);
                _mm_storeu_si128((__m128i*)(pel_dst + 56), v7);
                pel_dst += 64;
            }
        }
        else
        {
            xeve_itx_pb16b(src, dst, shift, line, step);
        }
    }
}

void xeve_itx_pb32b_neon(void *src, void *dst, int shift, int line, int step)
{
  xeve_itx_pb32b(src, dst, shift, line, step);
}

void xeve_itx_pb64b_neon(void *src, void *dst, int shift, int line, int step)
{
  xeve_itx_pb64b(src, dst, shift, line, step);
}


const XEVE_ITXB xeve_tbl_itxb_neon[MAX_TR_LOG2] =
{
    xeve_itx_pb2b,
    xeve_itx_pb4b_neon,
    xeve_itx_pb8b_neon,
    xeve_itx_pb16b_neon,
    xeve_itx_pb32b_neon,
    xeve_itx_pb64b_neon
};

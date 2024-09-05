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
#include "xeve_def.h"
#include "xeve_tbl.h"

// clang-format off

#define MAX_TX_DYNAMIC_RANGE_32               31
#define MAX_TX_VAL_32                       2147483647
#define MIN_TX_VAL_32                      (-2147483647-1)

#define XEVE_ITX_CLIP_SSE(X, min, max)\
X = _mm_max_epi32(X, min_val);\
X = _mm_min_epi32(X, max_val);

#define XEVE_ITX_SHIFT_CLIP_SSE(dst, offset, shift, min, max)\
dst = _mm_add_epi32( dst, offset);\
dst = _mm_srai_epi32(dst, shift);\
dst = _mm_max_epi32( dst, min);\
dst = _mm_min_epi32( dst, max);

void xeve_itx_pb4b_sse(void *src, void *dst, int shift, int line, int step)
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

void xeve_itx_pb8b_sse(void *src, void *dst, int shift, int line, int step)
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

                XEVE_ITX_CLIP_SSE(v0, min_val, max_val);
                XEVE_ITX_CLIP_SSE(v1, min_val, max_val);
                XEVE_ITX_CLIP_SSE(v2, min_val, max_val);
                XEVE_ITX_CLIP_SSE(v3, min_val, max_val);
                XEVE_ITX_CLIP_SSE(v4, min_val, max_val);
                XEVE_ITX_CLIP_SSE(v5, min_val, max_val);
                XEVE_ITX_CLIP_SSE(v6, min_val, max_val);
                XEVE_ITX_CLIP_SSE(v7, min_val, max_val);

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

                XEVE_ITX_SHIFT_CLIP_SSE(v0, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v1, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v2, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v3, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v4, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v5, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v6, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v7, add_s2, shift, min_val, max_val);

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

void xeve_itx_pb16b_sse(void *src, void *dst, int shift, int line, int step)
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

                XEVE_ITX_CLIP_SSE(v0 , min_val, max_val);
                XEVE_ITX_CLIP_SSE(v1 , min_val, max_val);
                XEVE_ITX_CLIP_SSE(v2 , min_val, max_val);
                XEVE_ITX_CLIP_SSE(v3 , min_val, max_val);
                XEVE_ITX_CLIP_SSE(v4 , min_val, max_val);
                XEVE_ITX_CLIP_SSE(v5 , min_val, max_val);
                XEVE_ITX_CLIP_SSE(v6 , min_val, max_val);
                XEVE_ITX_CLIP_SSE(v7 , min_val, max_val);
                XEVE_ITX_CLIP_SSE(v8 , min_val, max_val);
                XEVE_ITX_CLIP_SSE(v9 , min_val, max_val);
                XEVE_ITX_CLIP_SSE(v10, min_val, max_val);
                XEVE_ITX_CLIP_SSE(v11, min_val, max_val);
                XEVE_ITX_CLIP_SSE(v12, min_val, max_val);
                XEVE_ITX_CLIP_SSE(v13, min_val, max_val);
                XEVE_ITX_CLIP_SSE(v14, min_val, max_val);
                XEVE_ITX_CLIP_SSE(v15, min_val, max_val);

#define XEVE_ITDQ_TRANSPOS_SSE(s0, s1, s2, s3, t0, t1, t2, t3)\
t0 = _mm_unpacklo_epi32(s0, s1);\
t2 = _mm_unpackhi_epi32(s0, s1);\
t1 = _mm_unpacklo_epi32(s2, s3);\
t3 = _mm_unpackhi_epi32(s2, s3);\
\
s0 = _mm_unpacklo_epi64(t0, t1);\
s1 = _mm_unpackhi_epi64(t0, t1);\
s2 = _mm_unpacklo_epi64(t2, t3);\
s3 = _mm_unpackhi_epi64(t2, t3);
                XEVE_ITDQ_TRANSPOS_SSE(v0,  v1,  v2,  v3,  t0, t1, t2, t3);
                XEVE_ITDQ_TRANSPOS_SSE(v4,  v5,  v6,  v7,  t0, t1, t2, t3);
                XEVE_ITDQ_TRANSPOS_SSE(v8,  v9,  v10, v11, t0, t1, t2, t3);
                XEVE_ITDQ_TRANSPOS_SSE(v12, v13, v14, v15, t0, t1, t2, t3);
#undef XEVE_ITDQ_TRANSPOS_SSE

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

                XEVE_ITX_SHIFT_CLIP_SSE(v0 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v1 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v2 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v3 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v4 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v5 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v6 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v7 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v8 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v9 , add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v10, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v11, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v12, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v13, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v14, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v15, add_s2, shift, min_val, max_val);

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

void xeve_itx_pb32b_sse(void *src, void *dst, int shift, int line, int step)
{
    int add = shift == 0 ? 0 : 1 << (shift - 1);

    if (step == 0)
    {
        if (line > 2)
        {
            s16* pel_src = src;
            s32* pel_dst = dst;
            __m128i r[32] ;
            __m128i a[32];
            __m128i o[16], e[16], eo[8], ee[8], eeo[4], eee[4], eeeo[2], eeee[2];
            __m128i v[32], t[16], d[32];
            __m128i max_val = _mm_set1_epi32(MAX_TX_VAL_32);
            __m128i min_val = _mm_set1_epi32(MIN_TX_VAL_32);
            __m128i coef[16][16];

            for (int i = 0; i < 16; i++)
            {
                for (int j = 0; j < 16; j++)
                {
                    coef[i][j] = _mm_set1_epi32(((s32)(xeve_tbl_tm32[j + 16][i]) << 16) | (xeve_tbl_tm32[j][i] & 0xFFFF));
                }
            }

            int i, j, i_src[32];
            i_src[0] = 0;
            for (i = 1; i < 32; i++)
            {
                i_src[i] = i_src[i - 1] + line;
            }

            for (j = 0; j < line; j += 4)
            {

                for (i = 0; i < 32; i++)
                {
                    r[i] = _mm_loadl_epi64((__m128i*)(pel_src + i_src[i] + j));
                }

                for (i = 0; i < 16; i++)
                {
                    a[i] = _mm_unpacklo_epi16(r[i], r[i + 16]);
                }

#define XEVE_ITX32_O(dst, idx) \
t[ 1] = _mm_madd_epi16(a[ 1], coef[idx][ 1]);\
t[ 3] = _mm_madd_epi16(a[ 3], coef[idx][ 3]);\
t[ 5] = _mm_madd_epi16(a[ 5], coef[idx][ 5]);\
t[ 7] = _mm_madd_epi16(a[ 7], coef[idx][ 7]);\
t[ 9] = _mm_madd_epi16(a[ 9], coef[idx][ 9]);\
t[11] = _mm_madd_epi16(a[11], coef[idx][11]);\
t[13] = _mm_madd_epi16(a[13], coef[idx][13]);\
t[15] = _mm_madd_epi16(a[15], coef[idx][15]);\
d[0] = _mm_add_epi32(t[ 1], t[ 3]);\
d[1] = _mm_add_epi32(t[ 5], t[ 7]);\
d[2] = _mm_add_epi32(t[ 9], t[11]);\
d[3] = _mm_add_epi32(t[13], t[15]);\
t[0] = _mm_add_epi32(d[0], d[1]);\
t[1] = _mm_add_epi32(d[2], d[3]);\
dst = _mm_add_epi32(t[0], t[1]);

                for (i = 0; i < 16; i++)
                {
                    XEVE_ITX32_O(o[i], i);
                }
#undef XEVE_ITX32_O


#define XEVE_ITX32_EO(dst, idx) \
t[ 2] = _mm_madd_epi16(a[ 2], coef[idx][ 2]);\
t[ 6] = _mm_madd_epi16(a[ 6], coef[idx][ 6]);\
t[10] = _mm_madd_epi16(a[10], coef[idx][10]);\
t[14] = _mm_madd_epi16(a[14], coef[idx][14]);\
d[0] = _mm_add_epi32(t[ 2], t[ 6]);\
d[1] = _mm_add_epi32(t[10], t[14]);\
dst = _mm_add_epi32(d[0], d[1]);

                for (int i = 0; i < 8; i++)
                {
                    XEVE_ITX32_EO(eo[i], i);
                }
#undef XEVE_ITX32_EO

#define XEVE_ITX32_EEO(dst, idx) \
t[ 4] = _mm_madd_epi16(a[ 4], coef[idx][ 4]);\
t[12] = _mm_madd_epi16(a[12], coef[idx][12]);\
dst = _mm_add_epi32(t[4], t[12]);


                for (int i = 0; i < 4; i++)
                {
                    XEVE_ITX32_EEO(eeo[i], i);
                }
#undef XEVE_ITX32_EEO


                eeeo[0] = _mm_madd_epi16(a[8], coef[0][8]);
                eeeo[1] = _mm_madd_epi16(a[8], coef[1][8]);
                eeee[0] = _mm_madd_epi16(a[0], coef[0][0]);
                eeee[1] = _mm_madd_epi16(a[0], coef[1][0]);

                eee[0] = _mm_add_epi32(eeee[0], eeeo[0]);
                eee[1] = _mm_add_epi32(eeee[1], eeeo[1]);
                eee[2] = _mm_sub_epi32(eeee[1], eeeo[1]);
                eee[3] = _mm_sub_epi32(eeee[0], eeeo[0]);

                ee[0] = _mm_add_epi32(eee[0], eeo[0]);
                ee[1] = _mm_add_epi32(eee[1], eeo[1]);
                ee[2] = _mm_add_epi32(eee[2], eeo[2]);
                ee[3] = _mm_add_epi32(eee[3], eeo[3]);
                ee[4] = _mm_sub_epi32(eee[3], eeo[3]);
                ee[5] = _mm_sub_epi32(eee[2], eeo[2]);
                ee[6] = _mm_sub_epi32(eee[1], eeo[1]);
                ee[7] = _mm_sub_epi32(eee[0], eeo[0]);

                e[ 0] = _mm_add_epi32(ee[0], eo[0]);
                e[ 1] = _mm_add_epi32(ee[1], eo[1]);
                e[ 2] = _mm_add_epi32(ee[2], eo[2]);
                e[ 3] = _mm_add_epi32(ee[3], eo[3]);
                e[ 4] = _mm_add_epi32(ee[4], eo[4]);
                e[ 5] = _mm_add_epi32(ee[5], eo[5]);
                e[ 6] = _mm_add_epi32(ee[6], eo[6]);
                e[ 7] = _mm_add_epi32(ee[7], eo[7]);
                e[ 8] = _mm_sub_epi32(ee[7], eo[7]);
                e[ 9] = _mm_sub_epi32(ee[6], eo[6]);
                e[10] = _mm_sub_epi32(ee[5], eo[5]);
                e[11] = _mm_sub_epi32(ee[4], eo[4]);
                e[12] = _mm_sub_epi32(ee[3], eo[3]);
                e[13] = _mm_sub_epi32(ee[2], eo[2]);
                e[14] = _mm_sub_epi32(ee[1], eo[1]);
                e[15] = _mm_sub_epi32(ee[0], eo[0]);

                v[ 0] = _mm_add_epi32(e[ 0], o[ 0]);
                v[ 1] = _mm_add_epi32(e[ 1], o[ 1]);
                v[ 2] = _mm_add_epi32(e[ 2], o[ 2]);
                v[ 3] = _mm_add_epi32(e[ 3], o[ 3]);
                v[ 4] = _mm_add_epi32(e[ 4], o[ 4]);
                v[ 5] = _mm_add_epi32(e[ 5], o[ 5]);
                v[ 6] = _mm_add_epi32(e[ 6], o[ 6]);
                v[ 7] = _mm_add_epi32(e[ 7], o[ 7]);
                v[ 8] = _mm_add_epi32(e[ 8], o[ 8]);
                v[ 9] = _mm_add_epi32(e[ 9], o[ 9]);
                v[10] = _mm_add_epi32(e[10], o[10]);
                v[11] = _mm_add_epi32(e[11], o[11]);
                v[12] = _mm_add_epi32(e[12], o[12]);
                v[13] = _mm_add_epi32(e[13], o[13]);
                v[14] = _mm_add_epi32(e[14], o[14]);
                v[15] = _mm_add_epi32(e[15], o[15]);
                v[16] = _mm_sub_epi32(e[15], o[15]);
                v[17] = _mm_sub_epi32(e[14], o[14]);
                v[18] = _mm_sub_epi32(e[13], o[13]);
                v[19] = _mm_sub_epi32(e[12], o[12]);
                v[20] = _mm_sub_epi32(e[11], o[11]);
                v[21] = _mm_sub_epi32(e[10], o[10]);
                v[22] = _mm_sub_epi32(e[ 9], o[ 9]);
                v[23] = _mm_sub_epi32(e[ 8], o[ 8]);
                v[24] = _mm_sub_epi32(e[ 7], o[ 7]);
                v[25] = _mm_sub_epi32(e[ 6], o[ 6]);
                v[26] = _mm_sub_epi32(e[ 5], o[ 5]);
                v[27] = _mm_sub_epi32(e[ 4], o[ 4]);
                v[28] = _mm_sub_epi32(e[ 3], o[ 3]);
                v[29] = _mm_sub_epi32(e[ 2], o[ 2]);
                v[30] = _mm_sub_epi32(e[ 1], o[ 1]);
                v[31] = _mm_sub_epi32(e[ 0], o[ 0]);

                for (int i = 0; i < 32; i++)
                {
                    XEVE_ITX_CLIP_SSE(v[i], min_val, max_val);
                }

#define XEVE_ITDQ_TRANSPOS_SSE(s0, s1, s2, s3, t0, t1, t2, t3)\
t0 = _mm_unpacklo_epi32(s0, s1);\
t2 = _mm_unpackhi_epi32(s0, s1);\
t1 = _mm_unpacklo_epi32(s2, s3);\
t3 = _mm_unpackhi_epi32(s2, s3);\
\
s0 = _mm_unpacklo_epi64(t0, t1);\
s1 = _mm_unpackhi_epi64(t0, t1);\
s2 = _mm_unpacklo_epi64(t2, t3);\
s3 = _mm_unpackhi_epi64(t2, t3);
                XEVE_ITDQ_TRANSPOS_SSE(v[ 0], v[ 1], v[ 2], v[ 3],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[ 4], v[ 5], v[ 6], v[ 7],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[ 8], v[ 9], v[10], v[11],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[12], v[13], v[14], v[15],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[16], v[17], v[18], v[19],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[20], v[21], v[22], v[23],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[24], v[25], v[26], v[27],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[28], v[29], v[30], v[31],  t[0], t[1], t[2], t[3]);
#undef XEVE_ITDQ_TRANSPOS_SSE

                _mm_storeu_si128((__m128i*)(pel_dst),       v[ 0]);
                _mm_storeu_si128((__m128i*)(pel_dst +   4), v[ 4]);
                _mm_storeu_si128((__m128i*)(pel_dst +   8), v[ 8]);
                _mm_storeu_si128((__m128i*)(pel_dst +  12), v[12]);
                _mm_storeu_si128((__m128i*)(pel_dst +  16), v[16]);
                _mm_storeu_si128((__m128i*)(pel_dst +  20), v[20]);
                _mm_storeu_si128((__m128i*)(pel_dst +  24), v[24]);
                _mm_storeu_si128((__m128i*)(pel_dst +  28), v[28]);
                _mm_storeu_si128((__m128i*)(pel_dst +  32), v[ 1]);
                _mm_storeu_si128((__m128i*)(pel_dst +  36), v[ 5]);
                _mm_storeu_si128((__m128i*)(pel_dst +  40), v[ 9]);
                _mm_storeu_si128((__m128i*)(pel_dst +  44), v[13]);
                _mm_storeu_si128((__m128i*)(pel_dst +  48), v[17]);
                _mm_storeu_si128((__m128i*)(pel_dst +  52), v[21]);
                _mm_storeu_si128((__m128i*)(pel_dst +  56), v[25]);
                _mm_storeu_si128((__m128i*)(pel_dst +  60), v[29]);
                _mm_storeu_si128((__m128i*)(pel_dst +  64), v[ 2]);
                _mm_storeu_si128((__m128i*)(pel_dst +  68), v[ 6]);
                _mm_storeu_si128((__m128i*)(pel_dst +  72), v[10]);
                _mm_storeu_si128((__m128i*)(pel_dst +  76), v[14]);
                _mm_storeu_si128((__m128i*)(pel_dst +  80), v[18]);
                _mm_storeu_si128((__m128i*)(pel_dst +  84), v[22]);
                _mm_storeu_si128((__m128i*)(pel_dst +  88), v[26]);
                _mm_storeu_si128((__m128i*)(pel_dst +  92), v[30]);
                _mm_storeu_si128((__m128i*)(pel_dst +  96), v[ 3]);
                _mm_storeu_si128((__m128i*)(pel_dst + 100), v[ 7]);
                _mm_storeu_si128((__m128i*)(pel_dst + 104), v[11]);
                _mm_storeu_si128((__m128i*)(pel_dst + 108), v[15]);
                _mm_storeu_si128((__m128i*)(pel_dst + 112), v[19]);
                _mm_storeu_si128((__m128i*)(pel_dst + 116), v[23]);
                _mm_storeu_si128((__m128i*)(pel_dst + 120), v[27]);
                _mm_storeu_si128((__m128i*)(pel_dst + 124), v[31]);

                pel_dst += 128;
            }
        }
        else
        {
            xeve_itx_pb32b(src, dst, shift, line, step);
        }
    }
    else
    {
        if (line > 2)
        {
            s32 * pel_src = src;
            s16 * pel_dst = dst;
            __m128i r[32], a[16], b[16], e[16], o[16];
            __m128i eo[8], ee[8], eeo[4], eee[4], eeeo[2], eeee[2];
            __m128i v[32];
            __m128i t[16];
            const __m128i max_val = _mm_set1_epi32(MAX_TX_VAL_32);
            const __m128i min_val = _mm_set1_epi32(MIN_TX_VAL_32);
            const __m128i add_s2 = _mm_set1_epi32(add);
            __m128i coef[16][16];
            int i, j, i_src[32];

            for (i = 0; i < 16; i++)
            {
                for (j = 0; j < 16; j++)
                {
                    coef[i][j] = _mm_set1_epi64x(((s64)(xeve_tbl_tm32[j + 16][i]) << 32) | (xeve_tbl_tm32[j][i] & 0xFFFFFFFF));
                }
            }

            i_src[0] = 0;
            for (i = 1; i < 32; i++)
            {
                i_src[i] = i_src[i - 1] + line;
            }


            for (j = 0; j < line; j += 4)
            {
                for (i = 0; i < 32; i++)
                {
                    r[i] = _mm_loadu_si128((__m128i*)(pel_src + i_src[i] + j));
                }

                for (i = 0; i < 16; i++)
                {
                    a[i] = _mm_unpacklo_epi32(r[i], r[i + 16]);
                    b[i] = _mm_unpackhi_epi32(r[i], r[i + 16]);
                }

#define XEVE_ITX_MADD(dst, r_idx, c_idx)\
t[0] = _mm_mullo_epi32(a[r_idx], coef[c_idx][r_idx]);\
t[1] = _mm_mullo_epi32(b[r_idx], coef[c_idx][r_idx]);\
dst = _mm_hadd_epi32(t[0], t[1]);

#define XEVE_ITX32_0_32B(dst, idx)\
XEVE_ITX_MADD(v[0],  1, idx)\
XEVE_ITX_MADD(v[1],  3, idx)\
XEVE_ITX_MADD(v[2],  5, idx)\
XEVE_ITX_MADD(v[3],  7, idx)\
XEVE_ITX_MADD(v[4],  9, idx)\
XEVE_ITX_MADD(v[5], 11, idx)\
XEVE_ITX_MADD(v[6], 13, idx)\
XEVE_ITX_MADD(v[7], 15, idx)\
t[0] = _mm_add_epi32(v[0], v[1]);\
t[1] = _mm_add_epi32(v[2], v[3]);\
t[2] = _mm_add_epi32(v[4], v[5]);\
t[3] = _mm_add_epi32(v[6], v[7]);\
v[0] = _mm_add_epi32(t[0], t[1]);\
v[1] = _mm_add_epi32(t[2], t[3]);\
dst = _mm_add_epi32(v[0], v[1]);

                for (i = 0; i < 16; i++)
                {
                    XEVE_ITX32_0_32B(o[i], i);
                }
#undef XEVE_ITX32_0_32B

#define XEVE_ITX32_E0_32B(dst, idx)\
XEVE_ITX_MADD(v[0],   2, idx)\
XEVE_ITX_MADD(v[1],   6, idx)\
XEVE_ITX_MADD(v[2],  10, idx)\
XEVE_ITX_MADD(v[3],  14, idx)\
t[0] = _mm_add_epi32(v[0], v[1]);\
t[1] = _mm_add_epi32(v[2], v[3]);\
dst = _mm_add_epi32(t[0], t[1]);

                for (i = 0; i < 8; i++)
                {
                    XEVE_ITX32_E0_32B(eo[i], i);
                }
#undef XEVE_ITX32_E0_32B

#define XEVE_ITX32_EEO_32B(dst, idx)\
XEVE_ITX_MADD(v[0],   4, idx)\
XEVE_ITX_MADD(v[1],  12, idx)\
dst = _mm_add_epi32(v[0], v[1]);

                for (i = 0; i < 4; i++)
                {
                    XEVE_ITX32_EEO_32B(eeo[i], i);
                }
#undef XEVE_ITX32_EEO_32B

                XEVE_ITX_MADD(eeeo[0], 8, 0);
                XEVE_ITX_MADD(eeeo[1], 8, 1);
                XEVE_ITX_MADD(eeee[0], 0, 0);
                XEVE_ITX_MADD(eeee[1], 0, 1);
#undef XEVE_ITX_MADD
                eee[0] = _mm_add_epi32(eeee[0], eeeo[0]);
                eee[1] = _mm_add_epi32(eeee[1], eeeo[1]);
                eee[2] = _mm_sub_epi32(eeee[1], eeeo[1]);
                eee[3] = _mm_sub_epi32(eeee[0], eeeo[0]);

                ee[0] = _mm_add_epi32(eee[0], eeo[0]);
                ee[1] = _mm_add_epi32(eee[1], eeo[1]);
                ee[2] = _mm_add_epi32(eee[2], eeo[2]);
                ee[3] = _mm_add_epi32(eee[3], eeo[3]);
                ee[4] = _mm_sub_epi32(eee[3], eeo[3]);
                ee[5] = _mm_sub_epi32(eee[2], eeo[2]);
                ee[6] = _mm_sub_epi32(eee[1], eeo[1]);
                ee[7] = _mm_sub_epi32(eee[0], eeo[0]);

                e[ 0] = _mm_add_epi32(ee[0], eo[0]);
                e[ 1] = _mm_add_epi32(ee[1], eo[1]);
                e[ 2] = _mm_add_epi32(ee[2], eo[2]);
                e[ 3] = _mm_add_epi32(ee[3], eo[3]);
                e[ 4] = _mm_add_epi32(ee[4], eo[4]);
                e[ 5] = _mm_add_epi32(ee[5], eo[5]);
                e[ 6] = _mm_add_epi32(ee[6], eo[6]);
                e[ 7] = _mm_add_epi32(ee[7], eo[7]);
                e[ 8] = _mm_sub_epi32(ee[7], eo[7]);
                e[ 9] = _mm_sub_epi32(ee[6], eo[6]);
                e[10] = _mm_sub_epi32(ee[5], eo[5]);
                e[11] = _mm_sub_epi32(ee[4], eo[4]);
                e[12] = _mm_sub_epi32(ee[3], eo[3]);
                e[13] = _mm_sub_epi32(ee[2], eo[2]);
                e[14] = _mm_sub_epi32(ee[1], eo[1]);
                e[15] = _mm_sub_epi32(ee[0], eo[0]);

                v[ 0] = _mm_add_epi32(e[ 0], o[ 0]);
                v[ 1] = _mm_add_epi32(e[ 1], o[ 1]);
                v[ 2] = _mm_add_epi32(e[ 2], o[ 2]);
                v[ 3] = _mm_add_epi32(e[ 3], o[ 3]);
                v[ 4] = _mm_add_epi32(e[ 4], o[ 4]);
                v[ 5] = _mm_add_epi32(e[ 5], o[ 5]);
                v[ 6] = _mm_add_epi32(e[ 6], o[ 6]);
                v[ 7] = _mm_add_epi32(e[ 7], o[ 7]);
                v[ 8] = _mm_add_epi32(e[ 8], o[ 8]);
                v[ 9] = _mm_add_epi32(e[ 9], o[ 9]);
                v[10] = _mm_add_epi32(e[10], o[10]);
                v[11] = _mm_add_epi32(e[11], o[11]);
                v[12] = _mm_add_epi32(e[12], o[12]);
                v[13] = _mm_add_epi32(e[13], o[13]);
                v[14] = _mm_add_epi32(e[14], o[14]);
                v[15] = _mm_add_epi32(e[15], o[15]);
                v[16] = _mm_sub_epi32(e[15], o[15]);
                v[17] = _mm_sub_epi32(e[14], o[14]);
                v[18] = _mm_sub_epi32(e[13], o[13]);
                v[19] = _mm_sub_epi32(e[12], o[12]);
                v[20] = _mm_sub_epi32(e[11], o[11]);
                v[21] = _mm_sub_epi32(e[10], o[10]);
                v[22] = _mm_sub_epi32(e[ 9], o[ 9]);
                v[23] = _mm_sub_epi32(e[ 8], o[ 8]);
                v[24] = _mm_sub_epi32(e[ 7], o[ 7]);
                v[25] = _mm_sub_epi32(e[ 6], o[ 6]);
                v[26] = _mm_sub_epi32(e[ 5], o[ 5]);
                v[27] = _mm_sub_epi32(e[ 4], o[ 4]);
                v[28] = _mm_sub_epi32(e[ 3], o[ 3]);
                v[29] = _mm_sub_epi32(e[ 2], o[ 2]);
                v[30] = _mm_sub_epi32(e[ 1], o[ 1]);
                v[31] = _mm_sub_epi32(e[ 0], o[ 0]);

                XEVE_ITX_SHIFT_CLIP_SSE(v[ 0], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[ 1], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[ 2], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[ 3], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[ 4], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[ 5], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[ 6], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[ 7], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[ 8], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[ 9], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[10], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[11], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[12], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[13], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[14], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[15], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[16], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[17], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[18], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[19], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[20], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[21], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[22], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[23], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[24], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[25], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[26], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[27], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[28], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[29], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[30], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[31], add_s2, shift, min_val, max_val);

                t[ 0] = _mm_packs_epi32(v[ 0], v[16]);
                t[ 1] = _mm_packs_epi32(v[ 1], v[17]);
                t[ 2] = _mm_packs_epi32(v[ 2], v[18]);
                t[ 3] = _mm_packs_epi32(v[ 3], v[19]);
                t[ 4] = _mm_packs_epi32(v[ 4], v[20]);
                t[ 5] = _mm_packs_epi32(v[ 5], v[21]);
                t[ 6] = _mm_packs_epi32(v[ 6], v[22]);
                t[ 7] = _mm_packs_epi32(v[ 7], v[23]);
                t[ 8] = _mm_packs_epi32(v[ 8], v[24]);
                t[ 9] = _mm_packs_epi32(v[ 9], v[25]);
                t[10] = _mm_packs_epi32(v[10], v[26]);
                t[11] = _mm_packs_epi32(v[11], v[27]);
                t[12] = _mm_packs_epi32(v[12], v[28]);
                t[13] = _mm_packs_epi32(v[13], v[29]);
                t[14] = _mm_packs_epi32(v[14], v[30]);
                t[15] = _mm_packs_epi32(v[15], v[31]);

                v[ 0] = _mm_unpacklo_epi16(t[ 0], t[ 1]);
                v[ 1] = _mm_unpacklo_epi16(t[ 2], t[ 3]);
                v[ 2] = _mm_unpacklo_epi16(t[ 4], t[ 5]);
                v[ 3] = _mm_unpacklo_epi16(t[ 6], t[ 7]);
                v[ 4] = _mm_unpacklo_epi16(t[ 8], t[ 9]);
                v[ 5] = _mm_unpacklo_epi16(t[10], t[11]);
                v[ 6] = _mm_unpacklo_epi16(t[12], t[13]);
                v[ 7] = _mm_unpacklo_epi16(t[14], t[15]);
                v[ 8] = _mm_unpackhi_epi16(t[ 0], t[ 1]);
                v[ 9] = _mm_unpackhi_epi16(t[ 2], t[ 3]);
                v[10] = _mm_unpackhi_epi16(t[ 4], t[ 5]);
                v[11] = _mm_unpackhi_epi16(t[ 6], t[ 7]);
                v[12] = _mm_unpackhi_epi16(t[ 8], t[ 9]);
                v[13] = _mm_unpackhi_epi16(t[10], t[11]);
                v[14] = _mm_unpackhi_epi16(t[12], t[13]);
                v[15] = _mm_unpackhi_epi16(t[14], t[15]);

                t[ 0] = _mm_unpacklo_epi32(v[ 0], v[ 1]);
                t[ 1] = _mm_unpacklo_epi32(v[ 2], v[ 3]);
                t[ 2] = _mm_unpacklo_epi32(v[ 4], v[ 5]);
                t[ 3] = _mm_unpacklo_epi32(v[ 6], v[ 7]);
                t[ 4] = _mm_unpacklo_epi32(v[ 8], v[ 9]);
                t[ 5] = _mm_unpacklo_epi32(v[10], v[11]);
                t[ 6] = _mm_unpacklo_epi32(v[12], v[13]);
                t[ 7] = _mm_unpacklo_epi32(v[14], v[15]);
                t[ 8] = _mm_unpackhi_epi32(v[ 0], v[ 1]);
                t[ 9] = _mm_unpackhi_epi32(v[ 2], v[ 3]);
                t[10] = _mm_unpackhi_epi32(v[ 4], v[ 5]);
                t[11] = _mm_unpackhi_epi32(v[ 6], v[ 7]);
                t[12] = _mm_unpackhi_epi32(v[ 8], v[ 9]);
                t[13] = _mm_unpackhi_epi32(v[10], v[11]);
                t[14] = _mm_unpackhi_epi32(v[12], v[13]);
                t[15] = _mm_unpackhi_epi32(v[14], v[15]);

                v[ 0] = _mm_unpacklo_epi64(t[ 0], t[ 1]);
                v[ 1] = _mm_unpacklo_epi64(t[ 2], t[ 3]);
                v[ 2] = _mm_unpacklo_epi64(t[ 4], t[ 5]);
                v[ 3] = _mm_unpacklo_epi64(t[ 6], t[ 7]);
                v[ 4] = _mm_unpackhi_epi64(t[ 0], t[ 1]);
                v[ 5] = _mm_unpackhi_epi64(t[ 2], t[ 3]);
                v[ 6] = _mm_unpackhi_epi64(t[ 4], t[ 5]);
                v[ 7] = _mm_unpackhi_epi64(t[ 6], t[ 7]);
                v[ 8] = _mm_unpacklo_epi64(t[ 8], t[ 9]);
                v[ 9] = _mm_unpacklo_epi64(t[10], t[11]);
                v[10] = _mm_unpacklo_epi64(t[12], t[13]);
                v[11] = _mm_unpacklo_epi64(t[14], t[15]);
                v[12] = _mm_unpackhi_epi64(t[ 8], t[ 9]);
                v[13] = _mm_unpackhi_epi64(t[10], t[11]);
                v[14] = _mm_unpackhi_epi64(t[12], t[13]);
                v[15] = _mm_unpackhi_epi64(t[14], t[15]);

                for (i = 0; i < 16; i++)
                {
                    _mm_storeu_si128((__m128i*)(pel_dst), v[i]);
                    pel_dst += 8;
                }
            }
        }
        else
    
        {
            xeve_itx_pb32b(src, dst, shift, line, step);
        }
    }

}

void xeve_itx_pb64b_sse(void *src, void *dst, int shift, int line, int step)
{
    int add = shift == 0 ? 0 : 1 << (shift - 1);
    if (step == 0)
    {
        if (line > 2)
        {
            s16* pel_src = src;
            s32* pel_dst = dst;
            __m128i r[64] ;
            __m128i a[64];
            __m128i o[32], e[32], eo[16], ee[16], eeo[8], eee[8], eeeo[4], eeee[4], eeeeo[2], eeeee[2];
            __m128i v[64], t[16], d[64];
            __m128i max_val = _mm_set1_epi32(MAX_TX_VAL_32);
            __m128i min_val = _mm_set1_epi32(MIN_TX_VAL_32);
            __m128i coef[32][32];

            for (int i = 0; i < 32; i++)
            {
                for (int j = 0; j < 32; j++)
                {
                    coef[i][j] = _mm_set1_epi32(((s32)(xeve_tbl_tm64[j + 32][i]) << 16) | (xeve_tbl_tm64[j][i] & 0xFFFF));
                }
            }

            int i, j, i_src[64];
            i_src[0] = 0;
            for (int i = 1; i < 64; i++)
            {
                i_src[i] = i_src[i - 1] + line;
            }

            for (j = 0; j < line; j += 4)
            {

                for (i = 0; i < 64; i++)
                {
                    r[i] = _mm_loadl_epi64((__m128i*)(pel_src + i_src[i] + j));
                }

                for (i = 0; i < 32; i++)
                {
                    a[i] = _mm_unpacklo_epi16(r[i], r[i + 32]);
                }


#define XEVE_ITX64_O(dst, idx) \
t[ 0] = _mm_madd_epi16(a[ 1], coef[idx][ 1]);\
t[ 1] = _mm_madd_epi16(a[ 3], coef[idx][ 3]);\
t[ 2] = _mm_madd_epi16(a[ 5], coef[idx][ 5]);\
t[ 3] = _mm_madd_epi16(a[ 7], coef[idx][ 7]);\
t[ 4] = _mm_madd_epi16(a[ 9], coef[idx][ 9]);\
t[ 5] = _mm_madd_epi16(a[11], coef[idx][11]);\
t[ 6] = _mm_madd_epi16(a[13], coef[idx][13]);\
t[ 7] = _mm_madd_epi16(a[15], coef[idx][15]);\
t[ 8] = _mm_madd_epi16(a[17], coef[idx][17]);\
t[ 9] = _mm_madd_epi16(a[19], coef[idx][19]);\
t[10] = _mm_madd_epi16(a[21], coef[idx][21]);\
t[11] = _mm_madd_epi16(a[23], coef[idx][23]);\
t[12] = _mm_madd_epi16(a[25], coef[idx][25]);\
t[13] = _mm_madd_epi16(a[27], coef[idx][27]);\
t[14] = _mm_madd_epi16(a[29], coef[idx][29]);\
t[15] = _mm_madd_epi16(a[31], coef[idx][31]);\
d[0] = _mm_add_epi32(t[ 0], t[ 1]);\
d[1] = _mm_add_epi32(t[ 2], t[ 3]);\
d[2] = _mm_add_epi32(t[ 4], t[ 5]);\
d[3] = _mm_add_epi32(t[ 6], t[ 7]);\
d[4] = _mm_add_epi32(t[ 8], t[ 9]);\
d[5] = _mm_add_epi32(t[10], t[11]);\
d[6] = _mm_add_epi32(t[12], t[13]);\
d[7] = _mm_add_epi32(t[14], t[15]);\
t[0] = _mm_add_epi32(d[0], d[1]);\
t[1] = _mm_add_epi32(d[2], d[3]);\
t[2] = _mm_add_epi32(d[4], d[5]);\
t[3] = _mm_add_epi32(d[6], d[7]);\
d[0] = _mm_add_epi32(t[0], t[1]);\
d[1] = _mm_add_epi32(t[2], t[3]);\
dst = _mm_add_epi32(d[0], d[1]);

                for (int i = 0; i < 32; i++)
                {
                    XEVE_ITX64_O(o[i], i);
                }
#undef XEVE_ITX64_O


#define XEVE_ITX64_EO(dst, idx) \
t[0] = _mm_madd_epi16(a[ 2], coef[idx][ 2]);\
t[1] = _mm_madd_epi16(a[ 6], coef[idx][ 6]);\
t[2] = _mm_madd_epi16(a[10], coef[idx][10]);\
t[3] = _mm_madd_epi16(a[14], coef[idx][14]);\
t[4] = _mm_madd_epi16(a[18], coef[idx][18]);\
t[5] = _mm_madd_epi16(a[22], coef[idx][22]);\
t[6] = _mm_madd_epi16(a[26], coef[idx][26]);\
t[7] = _mm_madd_epi16(a[30], coef[idx][30]);\
d[0] = _mm_add_epi32(t[ 0], t[ 1]);\
d[1] = _mm_add_epi32(t[ 2], t[ 3]);\
d[2] = _mm_add_epi32(t[ 4], t[ 5]);\
d[3] = _mm_add_epi32(t[ 6], t[ 7]);\
t[0] = _mm_add_epi32(d[0], d[1]);\
t[1] = _mm_add_epi32(d[2], d[3]);\
dst = _mm_add_epi32(t[0], t[1]);

                for (int i = 0; i < 16; i++)
                {
                    XEVE_ITX64_EO(eo[i], i);
                }
#undef XEVE_ITX64_EO


#define XEVE_ITX64_EEO(dst, idx) \
t[0] = _mm_madd_epi16(a[ 4], coef[idx][ 4]);\
t[1] = _mm_madd_epi16(a[12], coef[idx][12]);\
t[2] = _mm_madd_epi16(a[20], coef[idx][20]);\
t[3] = _mm_madd_epi16(a[28], coef[idx][28]);\
d[0] = _mm_add_epi32(t[ 0], t[ 1]);\
d[1] = _mm_add_epi32(t[ 2], t[ 3]);\
dst = _mm_add_epi32(d[0], d[1]);

                for (int i = 0; i < 8; i++)
                {
                    XEVE_ITX64_EEO(eeo[i], i);
                }
#undef XEVE_ITX64_EEO

#define XEVE_ITX64_EEEO(dst, idx) \
t[0] = _mm_madd_epi16(a[ 8], coef[idx][ 8]);\
t[1] = _mm_madd_epi16(a[24], coef[idx][24]);\
dst = _mm_add_epi32(t[0], t[1]);


                for (int i = 0; i < 4; i++)
                {
                    XEVE_ITX64_EEEO(eeeo[i], i);
                }
#undef XEVE_ITX64_EEEO

                eeeeo[0] = _mm_madd_epi16(a[16], coef[0][16]);
                eeeeo[1] = _mm_madd_epi16(a[16], coef[1][16]);
                eeeee[0] = _mm_madd_epi16(a[0], coef[0][0]);
                eeeee[1] = _mm_madd_epi16(a[0], coef[1][0]);

                eeee[0] = _mm_add_epi32(eeeee[0], eeeeo[0]);
                eeee[1] = _mm_add_epi32(eeeee[1], eeeeo[1]);
                eeee[2] = _mm_sub_epi32(eeeee[1], eeeeo[1]);
                eeee[3] = _mm_sub_epi32(eeeee[0], eeeeo[0]);

                eee[0] = _mm_add_epi32(eeee[0], eeeo[0]);
                eee[1] = _mm_add_epi32(eeee[1], eeeo[1]);
                eee[2] = _mm_add_epi32(eeee[2], eeeo[2]);
                eee[3] = _mm_add_epi32(eeee[3], eeeo[3]);
                eee[4] = _mm_sub_epi32(eeee[3], eeeo[3]);
                eee[5] = _mm_sub_epi32(eeee[2], eeeo[2]);
                eee[6] = _mm_sub_epi32(eeee[1], eeeo[1]);
                eee[7] = _mm_sub_epi32(eeee[0], eeeo[0]);

                ee[ 0] = _mm_add_epi32(eee[0], eeo[0]);
                ee[ 1] = _mm_add_epi32(eee[1], eeo[1]);
                ee[ 2] = _mm_add_epi32(eee[2], eeo[2]);
                ee[ 3] = _mm_add_epi32(eee[3], eeo[3]);
                ee[ 4] = _mm_add_epi32(eee[4], eeo[4]);
                ee[ 5] = _mm_add_epi32(eee[5], eeo[5]);
                ee[ 6] = _mm_add_epi32(eee[6], eeo[6]);
                ee[ 7] = _mm_add_epi32(eee[7], eeo[7]);
                ee[ 8] = _mm_sub_epi32(eee[7], eeo[7]);
                ee[ 9] = _mm_sub_epi32(eee[6], eeo[6]);
                ee[10] = _mm_sub_epi32(eee[5], eeo[5]);
                ee[11] = _mm_sub_epi32(eee[4], eeo[4]);
                ee[12] = _mm_sub_epi32(eee[3], eeo[3]);
                ee[13] = _mm_sub_epi32(eee[2], eeo[2]);
                ee[14] = _mm_sub_epi32(eee[1], eeo[1]);
                ee[15] = _mm_sub_epi32(eee[0], eeo[0]);

                e[ 0] = _mm_add_epi32(ee[ 0], eo[ 0]);
                e[ 1] = _mm_add_epi32(ee[ 1], eo[ 1]);
                e[ 2] = _mm_add_epi32(ee[ 2], eo[ 2]);
                e[ 3] = _mm_add_epi32(ee[ 3], eo[ 3]);
                e[ 4] = _mm_add_epi32(ee[ 4], eo[ 4]);
                e[ 5] = _mm_add_epi32(ee[ 5], eo[ 5]);
                e[ 6] = _mm_add_epi32(ee[ 6], eo[ 6]);
                e[ 7] = _mm_add_epi32(ee[ 7], eo[ 7]);
                e[ 8] = _mm_add_epi32(ee[ 8], eo[ 8]);
                e[ 9] = _mm_add_epi32(ee[ 9], eo[ 9]);
                e[10] = _mm_add_epi32(ee[10], eo[10]);
                e[11] = _mm_add_epi32(ee[11], eo[11]);
                e[12] = _mm_add_epi32(ee[12], eo[12]);
                e[13] = _mm_add_epi32(ee[13], eo[13]);
                e[14] = _mm_add_epi32(ee[14], eo[14]);
                e[15] = _mm_add_epi32(ee[15], eo[15]);
                e[16] = _mm_sub_epi32(ee[15], eo[15]);
                e[17] = _mm_sub_epi32(ee[14], eo[14]);
                e[18] = _mm_sub_epi32(ee[13], eo[13]);
                e[19] = _mm_sub_epi32(ee[12], eo[12]);
                e[20] = _mm_sub_epi32(ee[11], eo[11]);
                e[21] = _mm_sub_epi32(ee[10], eo[10]);
                e[22] = _mm_sub_epi32(ee[ 9], eo[ 9]);
                e[23] = _mm_sub_epi32(ee[ 8], eo[ 8]);
                e[24] = _mm_sub_epi32(ee[ 7], eo[ 7]);
                e[25] = _mm_sub_epi32(ee[ 6], eo[ 6]);
                e[26] = _mm_sub_epi32(ee[ 5], eo[ 5]);
                e[27] = _mm_sub_epi32(ee[ 4], eo[ 4]);
                e[28] = _mm_sub_epi32(ee[ 3], eo[ 3]);
                e[29] = _mm_sub_epi32(ee[ 2], eo[ 2]);
                e[30] = _mm_sub_epi32(ee[ 1], eo[ 1]);
                e[31] = _mm_sub_epi32(ee[ 0], eo[ 0]);

                v[ 0] = _mm_add_epi32(e[ 0], o[ 0]);
                v[ 1] = _mm_add_epi32(e[ 1], o[ 1]);
                v[ 2] = _mm_add_epi32(e[ 2], o[ 2]);
                v[ 3] = _mm_add_epi32(e[ 3], o[ 3]);
                v[ 4] = _mm_add_epi32(e[ 4], o[ 4]);
                v[ 5] = _mm_add_epi32(e[ 5], o[ 5]);
                v[ 6] = _mm_add_epi32(e[ 6], o[ 6]);
                v[ 7] = _mm_add_epi32(e[ 7], o[ 7]);
                v[ 8] = _mm_add_epi32(e[ 8], o[ 8]);
                v[ 9] = _mm_add_epi32(e[ 9], o[ 9]);
                v[10] = _mm_add_epi32(e[10], o[10]);
                v[11] = _mm_add_epi32(e[11], o[11]);
                v[12] = _mm_add_epi32(e[12], o[12]);
                v[13] = _mm_add_epi32(e[13], o[13]);
                v[14] = _mm_add_epi32(e[14], o[14]);
                v[15] = _mm_add_epi32(e[15], o[15]);
                v[16] = _mm_add_epi32(e[16], o[16]);
                v[17] = _mm_add_epi32(e[17], o[17]);
                v[18] = _mm_add_epi32(e[18], o[18]);
                v[19] = _mm_add_epi32(e[19], o[19]);
                v[20] = _mm_add_epi32(e[20], o[20]);
                v[21] = _mm_add_epi32(e[21], o[21]);
                v[22] = _mm_add_epi32(e[22], o[22]);
                v[23] = _mm_add_epi32(e[23], o[23]);
                v[24] = _mm_add_epi32(e[24], o[24]);
                v[25] = _mm_add_epi32(e[25], o[25]);
                v[26] = _mm_add_epi32(e[26], o[26]);
                v[27] = _mm_add_epi32(e[27], o[27]);
                v[28] = _mm_add_epi32(e[28], o[28]);
                v[29] = _mm_add_epi32(e[29], o[29]);
                v[30] = _mm_add_epi32(e[30], o[30]);
                v[31] = _mm_add_epi32(e[31], o[31]);
                v[32] = _mm_sub_epi32(e[31], o[31]);
                v[33] = _mm_sub_epi32(e[30], o[30]);
                v[34] = _mm_sub_epi32(e[29], o[29]);
                v[35] = _mm_sub_epi32(e[28], o[28]);
                v[36] = _mm_sub_epi32(e[27], o[27]);
                v[37] = _mm_sub_epi32(e[26], o[26]);
                v[38] = _mm_sub_epi32(e[25], o[25]);
                v[39] = _mm_sub_epi32(e[24], o[24]);
                v[40] = _mm_sub_epi32(e[23], o[23]);
                v[41] = _mm_sub_epi32(e[22], o[22]);
                v[42] = _mm_sub_epi32(e[21], o[21]);
                v[43] = _mm_sub_epi32(e[20], o[20]);
                v[44] = _mm_sub_epi32(e[19], o[19]);
                v[45] = _mm_sub_epi32(e[18], o[18]);
                v[46] = _mm_sub_epi32(e[17], o[17]);
                v[47] = _mm_sub_epi32(e[16], o[16]);
                v[48] = _mm_sub_epi32(e[15], o[15]);
                v[49] = _mm_sub_epi32(e[14], o[14]);
                v[50] = _mm_sub_epi32(e[13], o[13]);
                v[51] = _mm_sub_epi32(e[12], o[12]);
                v[52] = _mm_sub_epi32(e[11], o[11]);
                v[53] = _mm_sub_epi32(e[10], o[10]);
                v[54] = _mm_sub_epi32(e[ 9], o[ 9]);
                v[55] = _mm_sub_epi32(e[ 8], o[ 8]);
                v[56] = _mm_sub_epi32(e[ 7], o[ 7]);
                v[57] = _mm_sub_epi32(e[ 6], o[ 6]);
                v[58] = _mm_sub_epi32(e[ 5], o[ 5]);
                v[59] = _mm_sub_epi32(e[ 4], o[ 4]);
                v[60] = _mm_sub_epi32(e[ 3], o[ 3]);
                v[61] = _mm_sub_epi32(e[ 2], o[ 2]);
                v[62] = _mm_sub_epi32(e[ 1], o[ 1]);
                v[63] = _mm_sub_epi32(e[ 0], o[ 0]);

                for (int i = 0; i < 64; i++)
                {
                    XEVE_ITX_CLIP_SSE(v[i], min_val, max_val);
                }

#define XEVE_ITDQ_TRANSPOS_SSE(s0, s1, s2, s3, t0, t1, t2, t3)\
t0 = _mm_unpacklo_epi32(s0, s1);\
t2 = _mm_unpackhi_epi32(s0, s1);\
t1 = _mm_unpacklo_epi32(s2, s3);\
t3 = _mm_unpackhi_epi32(s2, s3);\
\
s0 = _mm_unpacklo_epi64(t0, t1);\
s1 = _mm_unpackhi_epi64(t0, t1);\
s2 = _mm_unpacklo_epi64(t2, t3);\
s3 = _mm_unpackhi_epi64(t2, t3);
                XEVE_ITDQ_TRANSPOS_SSE(v[ 0], v[ 1], v[ 2], v[ 3],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[ 4], v[ 5], v[ 6], v[ 7],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[ 8], v[ 9], v[10], v[11],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[12], v[13], v[14], v[15],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[16], v[17], v[18], v[19],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[20], v[21], v[22], v[23],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[24], v[25], v[26], v[27],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[28], v[29], v[30], v[31],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[32], v[33], v[34], v[35],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[36], v[37], v[38], v[39],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[40], v[41], v[42], v[43],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[44], v[45], v[46], v[47],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[48], v[49], v[50], v[51],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[52], v[53], v[54], v[55],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[56], v[57], v[58], v[59],  t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_SSE(v[60], v[61], v[62], v[63],  t[0], t[1], t[2], t[3]);
#undef XEVE_ITDQ_TRANSPOS_SSE

                _mm_storeu_si128((__m128i*)(pel_dst),       v[ 0]);
                _mm_storeu_si128((__m128i*)(pel_dst +   4), v[ 4]);
                _mm_storeu_si128((__m128i*)(pel_dst +   8), v[ 8]);
                _mm_storeu_si128((__m128i*)(pel_dst +  12), v[12]);
                _mm_storeu_si128((__m128i*)(pel_dst +  16), v[16]);
                _mm_storeu_si128((__m128i*)(pel_dst +  20), v[20]);
                _mm_storeu_si128((__m128i*)(pel_dst +  24), v[24]);
                _mm_storeu_si128((__m128i*)(pel_dst +  28), v[28]);
                _mm_storeu_si128((__m128i*)(pel_dst +  32), v[32]);
                _mm_storeu_si128((__m128i*)(pel_dst +  36), v[36]);
                _mm_storeu_si128((__m128i*)(pel_dst +  40), v[40]);
                _mm_storeu_si128((__m128i*)(pel_dst +  44), v[44]);
                _mm_storeu_si128((__m128i*)(pel_dst +  48), v[48]);
                _mm_storeu_si128((__m128i*)(pel_dst +  52), v[52]);
                _mm_storeu_si128((__m128i*)(pel_dst +  56), v[56]);
                _mm_storeu_si128((__m128i*)(pel_dst +  60), v[60]);
                _mm_storeu_si128((__m128i*)(pel_dst +  64), v[ 1]);
                _mm_storeu_si128((__m128i*)(pel_dst +  68), v[ 5]);
                _mm_storeu_si128((__m128i*)(pel_dst +  72), v[ 9]);
                _mm_storeu_si128((__m128i*)(pel_dst +  76), v[13]);
                _mm_storeu_si128((__m128i*)(pel_dst +  80), v[17]);
                _mm_storeu_si128((__m128i*)(pel_dst +  84), v[21]);
                _mm_storeu_si128((__m128i*)(pel_dst +  88), v[25]);
                _mm_storeu_si128((__m128i*)(pel_dst +  92), v[29]);
                _mm_storeu_si128((__m128i*)(pel_dst +  96), v[33]);
                _mm_storeu_si128((__m128i*)(pel_dst + 100), v[37]);
                _mm_storeu_si128((__m128i*)(pel_dst + 104), v[41]);
                _mm_storeu_si128((__m128i*)(pel_dst + 108), v[45]);
                _mm_storeu_si128((__m128i*)(pel_dst + 112), v[49]);
                _mm_storeu_si128((__m128i*)(pel_dst + 116), v[53]);
                _mm_storeu_si128((__m128i*)(pel_dst + 120), v[57]);
                _mm_storeu_si128((__m128i*)(pel_dst + 124), v[61]);
                _mm_storeu_si128((__m128i*)(pel_dst + 128), v[ 2]);
                _mm_storeu_si128((__m128i*)(pel_dst + 132), v[ 6]);
                _mm_storeu_si128((__m128i*)(pel_dst + 136), v[10]);
                _mm_storeu_si128((__m128i*)(pel_dst + 140), v[14]);
                _mm_storeu_si128((__m128i*)(pel_dst + 144), v[18]);
                _mm_storeu_si128((__m128i*)(pel_dst + 148), v[22]);
                _mm_storeu_si128((__m128i*)(pel_dst + 152), v[26]);
                _mm_storeu_si128((__m128i*)(pel_dst + 156), v[30]);
                _mm_storeu_si128((__m128i*)(pel_dst + 160), v[34]);
                _mm_storeu_si128((__m128i*)(pel_dst + 164), v[38]);
                _mm_storeu_si128((__m128i*)(pel_dst + 168), v[42]);
                _mm_storeu_si128((__m128i*)(pel_dst + 172), v[46]);
                _mm_storeu_si128((__m128i*)(pel_dst + 176), v[50]);
                _mm_storeu_si128((__m128i*)(pel_dst + 180), v[54]);
                _mm_storeu_si128((__m128i*)(pel_dst + 184), v[58]);
                _mm_storeu_si128((__m128i*)(pel_dst + 188), v[62]);
                _mm_storeu_si128((__m128i*)(pel_dst + 192), v[ 3]);
                _mm_storeu_si128((__m128i*)(pel_dst + 196), v[ 7]);
                _mm_storeu_si128((__m128i*)(pel_dst + 200), v[11]);
                _mm_storeu_si128((__m128i*)(pel_dst + 204), v[15]);
                _mm_storeu_si128((__m128i*)(pel_dst + 208), v[19]);
                _mm_storeu_si128((__m128i*)(pel_dst + 212), v[23]);
                _mm_storeu_si128((__m128i*)(pel_dst + 216), v[27]);
                _mm_storeu_si128((__m128i*)(pel_dst + 220), v[31]);
                _mm_storeu_si128((__m128i*)(pel_dst + 224), v[35]);
                _mm_storeu_si128((__m128i*)(pel_dst + 228), v[39]);
                _mm_storeu_si128((__m128i*)(pel_dst + 232), v[43]);
                _mm_storeu_si128((__m128i*)(pel_dst + 236), v[47]);
                _mm_storeu_si128((__m128i*)(pel_dst + 240), v[51]);
                _mm_storeu_si128((__m128i*)(pel_dst + 244), v[55]);
                _mm_storeu_si128((__m128i*)(pel_dst + 248), v[59]);
                _mm_storeu_si128((__m128i*)(pel_dst + 252), v[63]);

                pel_dst += 256;
            }
        }
        else
        {
            xeve_itx_pb64b(src, dst, shift, line, step);
        }
    }
    else
    {
        if (line > 2)
        {
            s32 * pel_src = src;
            s16 * pel_dst = dst;
            __m128i r[64], a[32], b[32], e[32], o[32];
            __m128i eo[16], ee[16], eeo[8], eee[8], eeeo[4], eeee[4], eeeeo[2], eeeee[2];
            __m128i v[64];
            __m128i t[32];
            const __m128i max_val = _mm_set1_epi32(MAX_TX_VAL_32);
            const __m128i min_val = _mm_set1_epi32(MIN_TX_VAL_32);
            const __m128i add_s2 = _mm_set1_epi32(add);
            __m128i coef[32][32];
            int i, j, i_src[64];

            for (i = 0; i < 32; i++)
            {
                for (j = 0; j < 32; j++)
                {
                    coef[i][j] = _mm_set1_epi64x(((s64)(xeve_tbl_tm64[j + 32][i]) << 32) | (xeve_tbl_tm64[j][i] & 0xFFFFFFFF));
                }
            }

            i_src[0] = 0;
            for (i = 1; i < 64; i++)
            {
                i_src[i] = i_src[i - 1] + line;
            }


            for (j = 0; j < line; j += 4)
            {
                for (i = 0; i < 64; i++)
                {
                    r[i] = _mm_loadu_si128((__m128i*)(pel_src + i_src[i] + j));
                }

                for (i = 0; i < 32; i++)
                {
                    a[i] = _mm_unpacklo_epi32(r[i], r[i + 32]);
                    b[i] = _mm_unpackhi_epi32(r[i], r[i + 32]);
                }

#define XEVE_ITX_MADD(dst, r_idx, c_idx)\
t[0] = _mm_mullo_epi32(a[r_idx], coef[c_idx][r_idx]);\
t[1] = _mm_mullo_epi32(b[r_idx], coef[c_idx][r_idx]);\
dst = _mm_hadd_epi32(t[0], t[1]);

#define XEVE_ITX64_0_32B(dst, idx)\
XEVE_ITX_MADD(v[ 0],  1, idx)\
XEVE_ITX_MADD(v[ 1],  3, idx)\
XEVE_ITX_MADD(v[ 2],  5, idx)\
XEVE_ITX_MADD(v[ 3],  7, idx)\
XEVE_ITX_MADD(v[ 4],  9, idx)\
XEVE_ITX_MADD(v[ 5], 11, idx)\
XEVE_ITX_MADD(v[ 6], 13, idx)\
XEVE_ITX_MADD(v[ 7], 15, idx)\
XEVE_ITX_MADD(v[ 8], 17, idx)\
XEVE_ITX_MADD(v[ 9], 19, idx)\
XEVE_ITX_MADD(v[10], 21, idx)\
XEVE_ITX_MADD(v[11], 23, idx)\
XEVE_ITX_MADD(v[12], 25, idx)\
XEVE_ITX_MADD(v[13], 27, idx)\
XEVE_ITX_MADD(v[14], 29, idx)\
XEVE_ITX_MADD(v[15], 31, idx)\
t[0] = _mm_add_epi32(v[ 0], v[ 1]);\
t[1] = _mm_add_epi32(v[ 2], v[ 3]);\
t[2] = _mm_add_epi32(v[ 4], v[ 5]);\
t[3] = _mm_add_epi32(v[ 6], v[ 7]);\
t[4] = _mm_add_epi32(v[ 8], v[ 9]);\
t[5] = _mm_add_epi32(v[10], v[11]);\
t[6] = _mm_add_epi32(v[12], v[13]);\
t[7] = _mm_add_epi32(v[14], v[15]);\
\
v[0] = _mm_add_epi32(t[0], t[1]);\
v[1] = _mm_add_epi32(t[2], t[3]);\
v[2] = _mm_add_epi32(t[4], t[5]);\
v[3] = _mm_add_epi32(t[6], t[7]);\
\
t[0] = _mm_add_epi32(v[0], v[1]);\
t[1] = _mm_add_epi32(v[2], v[3]);\
\
dst = _mm_add_epi32(t[0], t[1]);


                for (i = 0; i < 32; i++)
                {
                    XEVE_ITX64_0_32B(o[i], i);
                }
#undef XEVE_ITX64_0_32B

#define XEVE_ITX64_E0_32B(dst, idx)\
XEVE_ITX_MADD(v[0],  2, idx)\
XEVE_ITX_MADD(v[1],  6, idx)\
XEVE_ITX_MADD(v[2], 10, idx)\
XEVE_ITX_MADD(v[3], 14, idx)\
XEVE_ITX_MADD(v[4], 18, idx)\
XEVE_ITX_MADD(v[5], 22, idx)\
XEVE_ITX_MADD(v[6], 26, idx)\
XEVE_ITX_MADD(v[7], 30, idx)\
t[0] = _mm_add_epi32(v[ 0], v[ 1]);\
t[1] = _mm_add_epi32(v[ 2], v[ 3]);\
t[2] = _mm_add_epi32(v[ 4], v[ 5]);\
t[3] = _mm_add_epi32(v[ 6], v[ 7]);\
\
v[0] = _mm_add_epi32(t[0], t[1]);\
v[1] = _mm_add_epi32(t[2], t[3]);\
\
dst = _mm_add_epi32(v[0], v[1]);

                for (i = 0; i < 16; i++)
                {
                    XEVE_ITX64_E0_32B(eo[i], i);
                }
#undef XEVE_ITX64_E0_32B

#define XEVE_ITX64_EE0_32B(dst, idx)\
XEVE_ITX_MADD(v[0],  4, idx)\
XEVE_ITX_MADD(v[1], 12, idx)\
XEVE_ITX_MADD(v[2], 20, idx)\
XEVE_ITX_MADD(v[3], 28, idx)\
t[0] = _mm_add_epi32(v[0], v[1]);\
t[1] = _mm_add_epi32(v[2], v[3]);\
dst = _mm_add_epi32(t[0], t[1]);

                for (i = 0; i < 8; i++)
                {
                    XEVE_ITX64_EE0_32B(eeo[i], i);
                }
#undef XEVE_ITX64_EE0_32B

#define XEVE_ITX64_EEEO_32B(dst, idx)\
XEVE_ITX_MADD(v[0],   8, idx)\
XEVE_ITX_MADD(v[1],  24, idx)\
dst = _mm_add_epi32(v[0], v[1]);

                for (i = 0; i < 4; i++)
                {
                    XEVE_ITX64_EEEO_32B(eeeo[i], i);
                }
#undef XEVE_ITX64_EEEO_32B

                XEVE_ITX_MADD(eeeeo[0], 16, 0);
                XEVE_ITX_MADD(eeeeo[1], 16, 1);
                XEVE_ITX_MADD(eeeee[0],  0, 0);
                XEVE_ITX_MADD(eeeee[1],  0, 1);

                eeee[0] = _mm_add_epi32(eeeee[0], eeeeo[0]);
                eeee[1] = _mm_add_epi32(eeeee[1], eeeeo[1]);
                eeee[2] = _mm_sub_epi32(eeeee[1], eeeeo[1]);
                eeee[3] = _mm_sub_epi32(eeeee[0], eeeeo[0]);

                eee[0] = _mm_add_epi32(eeee[0], eeeo[0]);
                eee[1] = _mm_add_epi32(eeee[1], eeeo[1]);
                eee[2] = _mm_add_epi32(eeee[2], eeeo[2]);
                eee[3] = _mm_add_epi32(eeee[3], eeeo[3]);
                eee[4] = _mm_sub_epi32(eeee[3], eeeo[3]);
                eee[5] = _mm_sub_epi32(eeee[2], eeeo[2]);
                eee[6] = _mm_sub_epi32(eeee[1], eeeo[1]);
                eee[7] = _mm_sub_epi32(eeee[0], eeeo[0]);

                ee[ 0] = _mm_add_epi32(eee[0], eeo[0]);
                ee[ 1] = _mm_add_epi32(eee[1], eeo[1]);
                ee[ 2] = _mm_add_epi32(eee[2], eeo[2]);
                ee[ 3] = _mm_add_epi32(eee[3], eeo[3]);
                ee[ 4] = _mm_add_epi32(eee[4], eeo[4]);
                ee[ 5] = _mm_add_epi32(eee[5], eeo[5]);
                ee[ 6] = _mm_add_epi32(eee[6], eeo[6]);
                ee[ 7] = _mm_add_epi32(eee[7], eeo[7]);
                ee[ 8] = _mm_sub_epi32(eee[7], eeo[7]);
                ee[ 9] = _mm_sub_epi32(eee[6], eeo[6]);
                ee[10] = _mm_sub_epi32(eee[5], eeo[5]);
                ee[11] = _mm_sub_epi32(eee[4], eeo[4]);
                ee[12] = _mm_sub_epi32(eee[3], eeo[3]);
                ee[13] = _mm_sub_epi32(eee[2], eeo[2]);
                ee[14] = _mm_sub_epi32(eee[1], eeo[1]);
                ee[15] = _mm_sub_epi32(eee[0], eeo[0]);

                e[ 0] = _mm_add_epi32(ee[ 0], eo[ 0]);
                e[ 1] = _mm_add_epi32(ee[ 1], eo[ 1]);
                e[ 2] = _mm_add_epi32(ee[ 2], eo[ 2]);
                e[ 3] = _mm_add_epi32(ee[ 3], eo[ 3]);
                e[ 4] = _mm_add_epi32(ee[ 4], eo[ 4]);
                e[ 5] = _mm_add_epi32(ee[ 5], eo[ 5]);
                e[ 6] = _mm_add_epi32(ee[ 6], eo[ 6]);
                e[ 7] = _mm_add_epi32(ee[ 7], eo[ 7]);
                e[ 8] = _mm_add_epi32(ee[ 8], eo[ 8]);
                e[ 9] = _mm_add_epi32(ee[ 9], eo[ 9]);
                e[10] = _mm_add_epi32(ee[10], eo[10]);
                e[11] = _mm_add_epi32(ee[11], eo[11]);
                e[12] = _mm_add_epi32(ee[12], eo[12]);
                e[13] = _mm_add_epi32(ee[13], eo[13]);
                e[14] = _mm_add_epi32(ee[14], eo[14]);
                e[15] = _mm_add_epi32(ee[15], eo[15]);
                e[16] = _mm_sub_epi32(ee[15], eo[15]);
                e[17] = _mm_sub_epi32(ee[14], eo[14]);
                e[18] = _mm_sub_epi32(ee[13], eo[13]);
                e[19] = _mm_sub_epi32(ee[12], eo[12]);
                e[20] = _mm_sub_epi32(ee[11], eo[11]);
                e[21] = _mm_sub_epi32(ee[10], eo[10]);
                e[22] = _mm_sub_epi32(ee[ 9], eo[ 9]);
                e[23] = _mm_sub_epi32(ee[ 8], eo[ 8]);
                e[24] = _mm_sub_epi32(ee[ 7], eo[ 7]);
                e[25] = _mm_sub_epi32(ee[ 6], eo[ 6]);
                e[26] = _mm_sub_epi32(ee[ 5], eo[ 5]);
                e[27] = _mm_sub_epi32(ee[ 4], eo[ 4]);
                e[28] = _mm_sub_epi32(ee[ 3], eo[ 3]);
                e[29] = _mm_sub_epi32(ee[ 2], eo[ 2]);
                e[30] = _mm_sub_epi32(ee[ 1], eo[ 1]);
                e[31] = _mm_sub_epi32(ee[ 0], eo[ 0]);

                v[ 0] = _mm_add_epi32(e[ 0], o[ 0]);
                v[ 1] = _mm_add_epi32(e[ 1], o[ 1]);
                v[ 2] = _mm_add_epi32(e[ 2], o[ 2]);
                v[ 3] = _mm_add_epi32(e[ 3], o[ 3]);
                v[ 4] = _mm_add_epi32(e[ 4], o[ 4]);
                v[ 5] = _mm_add_epi32(e[ 5], o[ 5]);
                v[ 6] = _mm_add_epi32(e[ 6], o[ 6]);
                v[ 7] = _mm_add_epi32(e[ 7], o[ 7]);
                v[ 8] = _mm_add_epi32(e[ 8], o[ 8]);
                v[ 9] = _mm_add_epi32(e[ 9], o[ 9]);
                v[10] = _mm_add_epi32(e[10], o[10]);
                v[11] = _mm_add_epi32(e[11], o[11]);
                v[12] = _mm_add_epi32(e[12], o[12]);
                v[13] = _mm_add_epi32(e[13], o[13]);
                v[14] = _mm_add_epi32(e[14], o[14]);
                v[15] = _mm_add_epi32(e[15], o[15]);
                v[16] = _mm_add_epi32(e[16], o[16]);
                v[17] = _mm_add_epi32(e[17], o[17]);
                v[18] = _mm_add_epi32(e[18], o[18]);
                v[19] = _mm_add_epi32(e[19], o[19]);
                v[20] = _mm_add_epi32(e[20], o[20]);
                v[21] = _mm_add_epi32(e[21], o[21]);
                v[22] = _mm_add_epi32(e[22], o[22]);
                v[23] = _mm_add_epi32(e[23], o[23]);
                v[24] = _mm_add_epi32(e[24], o[24]);
                v[25] = _mm_add_epi32(e[25], o[25]);
                v[26] = _mm_add_epi32(e[26], o[26]);
                v[27] = _mm_add_epi32(e[27], o[27]);
                v[28] = _mm_add_epi32(e[28], o[28]);
                v[29] = _mm_add_epi32(e[29], o[29]);
                v[30] = _mm_add_epi32(e[30], o[30]);
                v[31] = _mm_add_epi32(e[31], o[31]);
                v[32] = _mm_sub_epi32(e[31], o[31]);
                v[33] = _mm_sub_epi32(e[30], o[30]);
                v[34] = _mm_sub_epi32(e[29], o[29]);
                v[35] = _mm_sub_epi32(e[28], o[28]);
                v[36] = _mm_sub_epi32(e[27], o[27]);
                v[37] = _mm_sub_epi32(e[26], o[26]);
                v[38] = _mm_sub_epi32(e[25], o[25]);
                v[39] = _mm_sub_epi32(e[24], o[24]);
                v[40] = _mm_sub_epi32(e[23], o[23]);
                v[41] = _mm_sub_epi32(e[22], o[22]);
                v[42] = _mm_sub_epi32(e[21], o[21]);
                v[43] = _mm_sub_epi32(e[20], o[20]);
                v[44] = _mm_sub_epi32(e[19], o[19]);
                v[45] = _mm_sub_epi32(e[18], o[18]);
                v[46] = _mm_sub_epi32(e[17], o[17]);
                v[47] = _mm_sub_epi32(e[16], o[16]);
                v[48] = _mm_sub_epi32(e[15], o[15]);
                v[49] = _mm_sub_epi32(e[14], o[14]);
                v[50] = _mm_sub_epi32(e[13], o[13]);
                v[51] = _mm_sub_epi32(e[12], o[12]);
                v[52] = _mm_sub_epi32(e[11], o[11]);
                v[53] = _mm_sub_epi32(e[10], o[10]);
                v[54] = _mm_sub_epi32(e[ 9], o[ 9]);
                v[55] = _mm_sub_epi32(e[ 8], o[ 8]);
                v[56] = _mm_sub_epi32(e[ 7], o[ 7]);
                v[57] = _mm_sub_epi32(e[ 6], o[ 6]);
                v[58] = _mm_sub_epi32(e[ 5], o[ 5]);
                v[59] = _mm_sub_epi32(e[ 4], o[ 4]);
                v[60] = _mm_sub_epi32(e[ 3], o[ 3]);
                v[61] = _mm_sub_epi32(e[ 2], o[ 2]);
                v[62] = _mm_sub_epi32(e[ 1], o[ 1]);
                v[63] = _mm_sub_epi32(e[ 0], o[ 0]);

                XEVE_ITX_SHIFT_CLIP_SSE(v[ 0], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[ 1], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[ 2], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[ 3], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[ 4], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[ 5], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[ 6], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[ 7], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[ 8], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[ 9], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[10], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[11], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[12], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[13], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[14], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[15], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[16], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[17], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[18], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[19], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[20], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[21], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[22], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[23], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[24], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[25], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[26], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[27], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[28], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[29], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[30], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[31], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[32], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[33], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[34], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[35], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[36], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[37], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[38], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[39], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[40], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[41], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[42], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[43], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[44], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[45], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[46], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[47], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[48], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[49], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[50], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[51], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[52], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[53], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[54], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[55], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[56], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[57], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[58], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[59], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[60], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[61], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[62], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_SSE(v[63], add_s2, shift, min_val, max_val);

                t[ 0] = _mm_packs_epi32(v[ 0], v[32]);
                t[ 1] = _mm_packs_epi32(v[ 1], v[33]);
                t[ 2] = _mm_packs_epi32(v[ 2], v[34]);
                t[ 3] = _mm_packs_epi32(v[ 3], v[35]);
                t[ 4] = _mm_packs_epi32(v[ 4], v[36]);
                t[ 5] = _mm_packs_epi32(v[ 5], v[37]);
                t[ 6] = _mm_packs_epi32(v[ 6], v[38]);
                t[ 7] = _mm_packs_epi32(v[ 7], v[39]);
                t[ 8] = _mm_packs_epi32(v[ 8], v[40]);
                t[ 9] = _mm_packs_epi32(v[ 9], v[41]);
                t[10] = _mm_packs_epi32(v[10], v[42]);
                t[11] = _mm_packs_epi32(v[11], v[43]);
                t[12] = _mm_packs_epi32(v[12], v[44]);
                t[13] = _mm_packs_epi32(v[13], v[45]);
                t[14] = _mm_packs_epi32(v[14], v[46]);
                t[15] = _mm_packs_epi32(v[15], v[47]);
                t[16] = _mm_packs_epi32(v[16], v[48]);
                t[17] = _mm_packs_epi32(v[17], v[49]);
                t[18] = _mm_packs_epi32(v[18], v[50]);
                t[19] = _mm_packs_epi32(v[19], v[51]);
                t[20] = _mm_packs_epi32(v[20], v[52]);
                t[21] = _mm_packs_epi32(v[21], v[53]);
                t[22] = _mm_packs_epi32(v[22], v[54]);
                t[23] = _mm_packs_epi32(v[23], v[55]);
                t[24] = _mm_packs_epi32(v[24], v[56]);
                t[25] = _mm_packs_epi32(v[25], v[57]);
                t[26] = _mm_packs_epi32(v[26], v[58]);
                t[27] = _mm_packs_epi32(v[27], v[59]);
                t[28] = _mm_packs_epi32(v[28], v[60]);
                t[29] = _mm_packs_epi32(v[29], v[61]);
                t[30] = _mm_packs_epi32(v[30], v[62]);
                t[31] = _mm_packs_epi32(v[31], v[63]);

                v[ 0] = _mm_unpacklo_epi16(t[ 0], t[ 1]);
                v[ 1] = _mm_unpacklo_epi16(t[ 2], t[ 3]);
                v[ 2] = _mm_unpacklo_epi16(t[ 4], t[ 5]);
                v[ 3] = _mm_unpacklo_epi16(t[ 6], t[ 7]);
                v[ 4] = _mm_unpacklo_epi16(t[ 8], t[ 9]);
                v[ 5] = _mm_unpacklo_epi16(t[10], t[11]);
                v[ 6] = _mm_unpacklo_epi16(t[12], t[13]);
                v[ 7] = _mm_unpacklo_epi16(t[14], t[15]);
                v[ 8] = _mm_unpacklo_epi16(t[16], t[17]);
                v[ 9] = _mm_unpacklo_epi16(t[18], t[19]);
                v[10] = _mm_unpacklo_epi16(t[20], t[21]);
                v[11] = _mm_unpacklo_epi16(t[22], t[23]);
                v[12] = _mm_unpacklo_epi16(t[24], t[25]);
                v[13] = _mm_unpacklo_epi16(t[26], t[27]);
                v[14] = _mm_unpacklo_epi16(t[28], t[29]);
                v[15] = _mm_unpacklo_epi16(t[30], t[31]);
                v[16] = _mm_unpackhi_epi16(t[ 0], t[ 1]);
                v[17] = _mm_unpackhi_epi16(t[ 2], t[ 3]);
                v[18] = _mm_unpackhi_epi16(t[ 4], t[ 5]);
                v[19] = _mm_unpackhi_epi16(t[ 6], t[ 7]);
                v[20] = _mm_unpackhi_epi16(t[ 8], t[ 9]);
                v[21] = _mm_unpackhi_epi16(t[10], t[11]);
                v[22] = _mm_unpackhi_epi16(t[12], t[13]);
                v[23] = _mm_unpackhi_epi16(t[14], t[15]);
                v[24] = _mm_unpackhi_epi16(t[16], t[17]);
                v[25] = _mm_unpackhi_epi16(t[18], t[19]);
                v[26] = _mm_unpackhi_epi16(t[20], t[21]);
                v[27] = _mm_unpackhi_epi16(t[22], t[23]);
                v[28] = _mm_unpackhi_epi16(t[24], t[25]);
                v[29] = _mm_unpackhi_epi16(t[26], t[27]);
                v[30] = _mm_unpackhi_epi16(t[28], t[29]);
                v[31] = _mm_unpackhi_epi16(t[30], t[31]);

                t[ 0] = _mm_unpacklo_epi32(v[ 0], v[ 1]);
                t[ 1] = _mm_unpacklo_epi32(v[ 2], v[ 3]);
                t[ 2] = _mm_unpacklo_epi32(v[ 4], v[ 5]);
                t[ 3] = _mm_unpacklo_epi32(v[ 6], v[ 7]);
                t[ 4] = _mm_unpacklo_epi32(v[ 8], v[ 9]);
                t[ 5] = _mm_unpacklo_epi32(v[10], v[11]);
                t[ 6] = _mm_unpacklo_epi32(v[12], v[13]);
                t[ 7] = _mm_unpacklo_epi32(v[14], v[15]);
                t[ 8] = _mm_unpacklo_epi32(v[16], v[17]);
                t[ 9] = _mm_unpacklo_epi32(v[18], v[19]);
                t[10] = _mm_unpacklo_epi32(v[20], v[21]);
                t[11] = _mm_unpacklo_epi32(v[22], v[23]);
                t[12] = _mm_unpacklo_epi32(v[24], v[25]);
                t[13] = _mm_unpacklo_epi32(v[26], v[27]);
                t[14] = _mm_unpacklo_epi32(v[28], v[29]);
                t[15] = _mm_unpacklo_epi32(v[30], v[31]);
                t[16] = _mm_unpackhi_epi32(v[ 0], v[ 1]);
                t[17] = _mm_unpackhi_epi32(v[ 2], v[ 3]);
                t[18] = _mm_unpackhi_epi32(v[ 4], v[ 5]);
                t[19] = _mm_unpackhi_epi32(v[ 6], v[ 7]);
                t[20] = _mm_unpackhi_epi32(v[ 8], v[ 9]);
                t[21] = _mm_unpackhi_epi32(v[10], v[11]);
                t[22] = _mm_unpackhi_epi32(v[12], v[13]);
                t[23] = _mm_unpackhi_epi32(v[14], v[15]);
                t[24] = _mm_unpackhi_epi32(v[16], v[17]);
                t[25] = _mm_unpackhi_epi32(v[18], v[19]);
                t[26] = _mm_unpackhi_epi32(v[20], v[21]);
                t[27] = _mm_unpackhi_epi32(v[22], v[23]);
                t[28] = _mm_unpackhi_epi32(v[24], v[25]);
                t[29] = _mm_unpackhi_epi32(v[26], v[27]);
                t[30] = _mm_unpackhi_epi32(v[28], v[29]);
                t[31] = _mm_unpackhi_epi32(v[30], v[31]);

                v[ 0] = _mm_unpacklo_epi64(t[ 0], t[ 1]);
                v[ 1] = _mm_unpacklo_epi64(t[ 2], t[ 3]);
                v[ 2] = _mm_unpacklo_epi64(t[ 4], t[ 5]);
                v[ 3] = _mm_unpacklo_epi64(t[ 6], t[ 7]);
                v[ 4] = _mm_unpacklo_epi64(t[ 8], t[ 9]);
                v[ 5] = _mm_unpacklo_epi64(t[10], t[11]);
                v[ 6] = _mm_unpacklo_epi64(t[12], t[13]);
                v[ 7] = _mm_unpacklo_epi64(t[14], t[15]);
                v[ 8] = _mm_unpackhi_epi64(t[ 0], t[ 1]);
                v[ 9] = _mm_unpackhi_epi64(t[ 2], t[ 3]);
                v[10] = _mm_unpackhi_epi64(t[ 4], t[ 5]);
                v[11] = _mm_unpackhi_epi64(t[ 6], t[ 7]);
                v[12] = _mm_unpackhi_epi64(t[ 8], t[ 9]);
                v[13] = _mm_unpackhi_epi64(t[10], t[11]);
                v[14] = _mm_unpackhi_epi64(t[12], t[13]);
                v[15] = _mm_unpackhi_epi64(t[14], t[15]);
                v[16] = _mm_unpacklo_epi64(t[16], t[17]);
                v[17] = _mm_unpacklo_epi64(t[18], t[19]);
                v[18] = _mm_unpacklo_epi64(t[20], t[21]);
                v[19] = _mm_unpacklo_epi64(t[22], t[23]);
                v[20] = _mm_unpacklo_epi64(t[24], t[25]);
                v[21] = _mm_unpacklo_epi64(t[26], t[27]);
                v[22] = _mm_unpacklo_epi64(t[28], t[29]);
                v[23] = _mm_unpacklo_epi64(t[30], t[31]);
                v[24] = _mm_unpackhi_epi64(t[16], t[17]);
                v[25] = _mm_unpackhi_epi64(t[18], t[19]);
                v[26] = _mm_unpackhi_epi64(t[20], t[21]);
                v[27] = _mm_unpackhi_epi64(t[22], t[23]);
                v[28] = _mm_unpackhi_epi64(t[24], t[25]);
                v[29] = _mm_unpackhi_epi64(t[26], t[27]);
                v[30] = _mm_unpackhi_epi64(t[28], t[29]);
                v[31] = _mm_unpackhi_epi64(t[30], t[31]);

                for (i = 0; i < 32; i++)
                {
                    _mm_storeu_si128((__m128i*)(pel_dst), v[i]);
                    pel_dst += 8;
                }
            }
        }
        else
        {
            xeve_itx_pb64b(src, dst, shift, line, step);
        }
    }
}

const XEVE_ITXB xeve_tbl_itxb_sse[MAX_TR_LOG2] =
{
    xeve_itx_pb2b,
    xeve_itx_pb4b_sse,
    xeve_itx_pb8b_sse,
    xeve_itx_pb16b_sse,
    xeve_itx_pb32b_sse,
    xeve_itx_pb64b_sse
};
// clang-format on

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
  xeve_itx_pb8b(src, dst, shift, line, step);
}

void xeve_itx_pb16b_neon(void *src, void *dst, int shift, int line, int step)
{
  xeve_itx_pb16b(src, dst, shift, line, step);
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

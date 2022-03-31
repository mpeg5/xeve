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

#define MAX_TX_DYNAMIC_RANGE_32               31
#define MAX_TX_VAL_32                       2147483647
#define MIN_TX_VAL_32                      (-2147483647-1)

#define XEVE_ITX_CLIP_NEON(X, min, max)\
X = vmaxq_s32(X, min_val);\
X = vminq_s32(X, max_val);

#define XEVE_ITX_SHIFT_CLIP_NEON(dst, offset, shift, min, max)\
dst = vaddq_s32( dst, offset);\
dst = vshr_n_s32(dst, shift);\
dst = vmaxq_s32( dst, min);\
dst = vminq_s32( dst, max);

int32x4_t mm_madd(int16x8_t a, int16x8_t b)
{
    int32x4_t pl = vmull_s16(vget_low_s16(a), vget_low_s16(b));
    int32x4_t ph = vmull_s16(vget_high_s16(a), vget_high_s16(b));
    int32x2_t rl = vpadd_s32(vget_low_s32(pl), vget_high_s32(pl));
    int32x2_t rh = vpadd_s32(vget_low_s32(ph), vget_high_s32(ph));
    return vcombine_s32(rl, rh);
}



void xeve_itx_pb4b_neon(void *src, void *dst, int shift, int line, int step)
{
    int add = shift == 0 ? 0 : 1 << (shift - 1);
    if (step == 0)
    {
        if (line > 2)
        {
            int16_t* pel_src = src;
            int32_t* pel_dst = dst;
            int64x2_t  r0, r1, r2, r3;
            int16x8_t  a0, a1;
            int32x4_t  e0, e1, o0, o1;
            int32x4_t v0, v1, v2, v3;
            int32x4_t t0, t1, t2, t3;

            int coef[] = {
           (xeve_tbl_tm4[3][0] << 16) | xeve_tbl_tm4[1][0],
            (xeve_tbl_tm4[3][1] << 16) | xeve_tbl_tm4[1][1],
            (xeve_tbl_tm4[2][1] << 16) | xeve_tbl_tm4[0][1],
            (xeve_tbl_tm4[0][0] << 16) | xeve_tbl_tm4[2][0]
            };

            const int32x4_t coef_0_13 = vld1q_dup_s32(coef);
            const int32x4_t coef_1_13 = vld1q_dup_s32(coef + 1);
            const int32x4_t coef_1_02 = vld1q_dup_s32(coef + 2);
            const int32x4_t coef_0_02 = vld1q_dup_s32(coef + 3);
            int val[] = {
              MAX_TX_VAL_32,
              MIN_TX_VAL_32
            };


            int32x4_t max_val = vld1q_dup_s32(val);
            int32x4_t min_val = vld1q_dup_s32(val + 1);
            int i_src1 = line;
            int i_src2 = i_src1 + i_src1;
            int i_src3 = i_src2 + i_src1;

            for (int j = 0; j < line; j += 4)
            {
                r0 = vld1q_s64((int64_t*)pel_src + j);
                r1 = vld1q_s64((int64_t*)pel_src + i_src1 + j);
                r2 = vld1q_s64((int64_t*)pel_src + i_src2 + j);
                r3 = vld1q_s64((int64_t*)pel_src + i_src3 + j);
                
                int16x4x2_t a0_ = vzip_s16(vget_low_s16(vreinterpretq_s16_s64(r0)), vget_low_s16(vreinterpretq_s16_s64(r2)));
                a0 = vcombine_s16(a0_.val[0], a0_.val[1]);
                int16x4x2_t a1_ = vzip_s16(vget_low_s16(vreinterpretq_s16_s64(r1)), vget_low_s16(vreinterpretq_s16_s64(r3)));
                a1 = vcombine_s16(a1_.val[0], a1_.val[1]);

                e0 = mm_madd(a0, vreinterpretq_s16_s32(coef_0_02));
                e1 = mm_madd(a0, vreinterpretq_s16_s32(coef_1_02));
                o0 = mm_madd(a1, vreinterpretq_s16_s32(coef_0_13));
                o1 = mm_madd(a1, vreinterpretq_s16_s32(coef_1_13));

                v0 = vaddq_s32(e0, o0);
                v3 = vsubq_s32(e0, o0);
                v1 = vaddq_s32(e1, o1);
                v2 = vsubq_s32(e1, o1);
                v0 = vmaxq_s32(v0, min_val);
                v1 = vmaxq_s32(v1, min_val);
                v2 = vmaxq_s32(v2, min_val);
                v3 = vmaxq_s32(v3, min_val);

                v0 = vminq_s32(v0, max_val);
                v1 = vminq_s32(v1, max_val);
                v2 = vminq_s32(v2, max_val);
                v3 = vminq_s32(v3, max_val);
               
               
               
                int32x2x2_t t0_ = vzip_s32(vget_low_s32(v0), vget_low_s32(v1));
                t0 = vcombine_s32(t0_.val[0], t0_.val[1]);

                int32x2x2_t t2_ = vzip_s32(vget_high_s32(v0), vget_high_s32(v1));
                t2 = vcombine_s32(t2_.val[0], t2_.val[1]);


                int32x2x2_t t1_ = vzip_s32(vget_low_s32(v2), vget_low_s32(v3));
                t1 = vcombine_s32(t1_.val[0], t1_.val[1]);


                int32x2x2_t t3_ = vzip_s32(vget_high_s32(v2), vget_high_s32(v3));
                t3 = vcombine_s32(t3_.val[0], t3_.val[1]);



                int32x2x2_t v0_ = vzip_s32(vget_low_s32(t0), vget_low_s32(t1));

                v0 = vcombine_s32(v0_.val[0], v0_.val[1]);


                int32x2x2_t v1_ = vzip_s32(vget_high_s32(t0), vget_high_s32(t1));
                v1 = vcombine_s32(v1_.val[0], v1_.val[1]);

                int32x2x2_t v2_ = vzip_s32(vget_low_s32(t2), vget_low_s32(t3));
                v2 = vcombine_s32(v2_.val[0], v2_.val[1]);

                int32x2x2_t v3_ = vzip_s32(vget_high_s32(t2), vget_high_s32(t3));
                v3 = vcombine_s32(v3_.val[0], v3_.val[1]);
                

                vst1q_s32(pel_dst, v0);
                vst1q_s32(pel_dst + 4, v1);
                vst1q_s32(pel_dst + 8, v2);
                vst1q_s32(pel_dst + 12, v3);

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
            int32_t* pel_src = src;
            int16_t* pel_dst = dst;



            int32x4_t  r0, r1, r2, r3;
            int32x4_t  a0, a1, b0, b1;
            int32x4_t  e0, e1, o0, o1;
            int32x4_t v0, v1, v2, v3;
            int32x4_t t0, t1;
            long coef[] = {
          ((long long)xeve_tbl_tm4[3][0] << 32) | xeve_tbl_tm4[1][0],
          ((long long)xeve_tbl_tm4[3][1] << 32) | xeve_tbl_tm4[1][1],
          ((long long)xeve_tbl_tm4[2][1] << 32) | xeve_tbl_tm4[0][1],
          ((long long)xeve_tbl_tm4[0][0] << 32) | xeve_tbl_tm4[2][0]
            };


            const int64x2_t coef_0_13 = vld1q_dup_s64(coef);
            const int64x2_t coef_1_13 = vld1q_dup_s64(coef + 1);
            const int64x2_t coef_1_02 = vld1q_dup_s64(coef + 2);
            const int64x2_t coef_0_02 = vld1q_dup_s64(coef + 3);

            const int32x4_t add_s2 = vld1q_dup_s32(&add);

            int val[] = {
              MAX_TX_VAL_32,
              MIN_TX_VAL_32
            };
            int32x4_t max_val = vld1q_dup_s32(val);
            int32x4_t min_val = vld1q_dup_s32(val + 1);
            int i_src1 = line;
            int i_src2 = i_src1 + i_src1;
            int i_src3 = i_src2 + i_src1;

            for (int j = 0; j < line; j += 4)
            {
                r0 = vld1q_s32(pel_src + j);
                r1 = vld1q_s32(pel_src + i_src1 + j);
                r2 = vld1q_s32(pel_src + i_src2 + j);
                r3 = vld1q_s32(pel_src + i_src3 + j);

                int32x2x2_t a0_ = vzip_s32(vget_low_s32(r0), vget_low_s32(r2));
                a0 = vcombine_s32(a0_.val[0], a0_.val[1]);

                int32x2x2_t b0_ = vzip_s32(vget_high_s32(r0), vget_high_s32(r2));
                b0 = vcombine_s32(b0_.val[0], b0_.val[1]);


                int32x2x2_t a1_ = vzip_s32(vget_low_s32(r1), vget_low_s32(r3));
                a1 = vcombine_s32(a1_.val[0], a1_.val[1]);

                int32x2x2_t b1_ = vzip_s32(vget_high_s32(r1), vget_high_s32(r3));
                b1 = vcombine_s32(b1_.val[0], b1_.val[1]);

                t0 = vmulq_s32(a0, vreinterpretq_s32_s64(coef_0_02));
                t1 = vmulq_s32(b0, vreinterpretq_s32_s64(coef_0_02));
                e0 = vpaddq_s32(t0, t1);

                t0 = vmulq_s32(a0, vreinterpretq_s32_s64(coef_1_02));
                t1 = vmulq_s32(b0, vreinterpretq_s32_s64(coef_1_02));
                e1 = vpaddq_s32(t0, t1);

                t0 = vmulq_s32(a1, vreinterpretq_s32_s64(coef_0_13));
                t1 = vmulq_s32(b1, vreinterpretq_s32_s64(coef_0_13));
                o0 = vpaddq_s32(t0, t1);

                t0 = vmulq_s32(a1, vreinterpretq_s32_s64(coef_1_13));
                t1 = vmulq_s32(b1, vreinterpretq_s32_s64(coef_1_13));
                o1 = vpaddq_s32(t0, t1);

                v0 = vaddq_s32(e0, o0);
                v3 = vsubq_s32(e0, o0);
                v1 = vaddq_s32(e1, o1);
                v2 = vsubq_s32(e1, o1);

                v0 = vaddq_s32(v0, add_s2);
                v1 = vaddq_s32(v1, add_s2);
                v2 = vaddq_s32(v2, add_s2);
                v3 = vaddq_s32(v3, add_s2);

                v0 = vshrq_n_s32(v0, shift);
                v1 = vshrq_n_s32(v1, shift);
                v2 = vshrq_n_s32(v2, shift);
                v3 = vshrq_n_s32(v3, shift);

                v0 = vpmaxq_s32(v0, min_val);
                v1 = vpmaxq_s32(v1, min_val);
                v2 = vpmaxq_s32(v2, min_val);
                v3 = vpmaxq_s32(v3, min_val);

                v0 = vpminq_s32(v0, max_val);
                v1 = vpminq_s32(v1, max_val);
                v2 = vpminq_s32(v2, max_val);
                v3 = vpminq_s32(v3, max_val);

                int32x2x2_t t0_ = vuzp_s32(vreinterpret_s32_s16(vqmovn_s32(v0)), vreinterpret_s32_s16(vqmovn_s32(v2)));
                t0 = vcombine_s32(t0_.val[0], t0_.val[1]);
                int32x2x2_t t1_ = vuzp_s32(vreinterpret_s32_s16(vqmovn_s32(v1)), vreinterpret_s32_s16(vqmovn_s32(v3)));
                t1 = vcombine_s32(t1_.val[0], t1_.val[1]);


                int16x4x2_t v0_ = vzip_s16(vget_low_s16(vreinterpretq_s16_s32(t0)), vget_low_s16(vreinterpretq_s16_s32(t1)));
                v0 = vreinterpretq_s32_s16(vcombine_s16(v0_.val[0], v0_.val[1]));

                int16x4x2_t v1_ = vzip_s16(vget_high_s16(vreinterpretq_s16_s32(t0)), vget_high_s16(vreinterpretq_s16_s32(t1)));
                v1 = vreinterpretq_s32_s16(vcombine_s16(v1_.val[0], v1_.val[1]));

                t0_ = vzip_s32(vget_low_s32(v0), vget_low_s32(v1));
                t0 = vcombine_s32(t0_.val[0], t0_.val[1]);

                t1_ = vzip_s32(vget_high_s32(v0), vget_high_s32(v1));
                t1 = vcombine_s32(t1_.val[0], t1_.val[1]);

                vst1q_s32((int32_t*)pel_dst, t0);
                vst1q_s32((int32_t*)pel_dst + 8, t1);

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
             xeve_itx_pb8b(src, dst, shift, line, step);
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
           xeve_itx_pb8b(src, dst, shift, line, step);
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
            xeve_itx_pb16b(src, dst, shift, line, step);
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
            xeve_itx_pb16b(src, dst, shift, line, step);
        }
        else
        {
            xeve_itx_pb16b(src, dst, shift, line, step);
        }
    }
}

void xeve_itx_pb32b_neon(void *src, void *dst, int shift, int line, int step)
{
    int add = shift == 0 ? 0 : 1 << (shift - 1);
    if (step == 0)
    {
        if (line > 2)
        {
            xeve_itx_pb32b(src, dst, shift, line, step);
        }
        else
    
        {
            xeve_itx_pb32b(src, dst, shift, line, step);
        }
    }

}

void xeve_itx_pb64b_neon(void *src, void *dst, int shift, int line, int step)
{
    int add = shift == 0 ? 0 : 1 << (shift - 1);
    if (step == 0)
    {
        if (line > 2)
        {
             xeve_itx_pb64b(src, dst, shift, line, step);
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
             xeve_itx_pb64b(src, dst, shift, line, step);
        }
        else
        {
            xeve_itx_pb64b(src, dst, shift, line, step);
        }
    }
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

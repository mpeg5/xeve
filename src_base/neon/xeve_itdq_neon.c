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


#define XEVE_ITX_CLIP_NEON(X, min, max)\
X = vmaxq_s32(X, min_val);\
X = vminq_s32(X, max_val);

#define XEVE_ITX_SHIFT_CLIP_NEON(dst, offset, shift, min, max)\
dst = vaddq_s32( dst, offset);\
dst = vshlq_s32(dst, vdupq_n_s32(-shift));\
dst = vmaxq_s32( dst, min);\
dst = vminq_s32( dst, max);

// the macro stores the multiply & pair-wise add value in 3rd register i.e t2 in this case
#define XEVE_MADD_S32(t0, t1, t2, a, b, coef)\
t0 = vmulq_s32(a, coef);\
t1 = vmulq_s32(b, coef);\
t2 = vpaddq_s32(t0, t1);

#define vmadd_s16(a, coef)\
    vpaddq_s32(vmull_s16(a.val[0], vget_low_s16(coef)), vmull_s16(a.val[1], vget_high_s16(coef)));

// clang-format on

void xeve_itx_pb4b_neon(void* src, void* dst, int shift, int line, int step)
{
    int add = shift == 0 ? 0 : 1 << (shift - 1);

    if(step == 0) {
        if(line > 2) {
            s16* pel_src = src;
            s32* pel_dst = dst;

            int16x4_t   r0, r1, r2, r3;
            int16x4x2_t a0, a1;
            int32x4_t   e0, e1, o0, o1;
            int32x4_t   v0, v1, v2, v3, t0, t1, t2, t3;

            const int16x8_t coef_0_13 = vdupq_n_s32((xeve_tbl_tm4[3][0] << 16) | xeve_tbl_tm4[1][0]);
            const int16x8_t coef_1_13 = vdupq_n_s32((xeve_tbl_tm4[3][1] << 16) | xeve_tbl_tm4[1][1]);
            const int16x8_t coef_1_02 = vdupq_n_s32((xeve_tbl_tm4[2][1] << 16) | xeve_tbl_tm4[0][1]);
            const int16x8_t coef_0_02 = vdupq_n_s32((xeve_tbl_tm4[0][0] << 16) | xeve_tbl_tm4[2][0]);

            int32x4_t max_val = vdupq_n_s32(MAX_TX_VAL_32);
            int32x4_t min_val = vdupq_n_s32(MIN_TX_VAL_32);

            int i_src1 = line;
            int i_src2 = i_src1 + i_src1;
            int i_src3 = i_src2 + i_src1;

            for(int j = 0; j < line; j += 4) {
                r0 = vld1_s16((pel_src + j));
                r1 = vld1_s16((pel_src + i_src1 + j));
                r2 = vld1_s16((pel_src + i_src2 + j));
                r3 = vld1_s16((pel_src + i_src3 + j));

                a0 = vzip_s16(r0, r2);
                a1 = vzip_s16(r1, r3);

                e0 = vmadd_s16(a0, coef_0_02);
                e1 = vmadd_s16(a0, coef_1_02);
                o0 = vmadd_s16(a1, coef_0_13);
                o1 = vmadd_s16(a1, coef_1_13);

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

                t0 = vzip1q_s32(v0, v1);
                t2 = vzip2q_s32(v0, v1);
                t1 = vzip1q_s32(v2, v3);
                t3 = vzip2q_s32(v2, v3);

                v0 = vzip1q_s64(t0, t1);
                v1 = vzip2q_s64(t0, t1);
                v2 = vzip1q_s64(t2, t3);
                v3 = vzip2q_s64(t2, t3);

                vst1q_s32((int32_t*)pel_dst, (v0));
                vst1q_s32((int32_t*)(pel_dst + 4), (v1));
                vst1q_s32((int32_t*)(pel_dst + 8), (v2));
                vst1q_s32((int32_t*)(pel_dst + 12), (v3));

                pel_dst += 16;
            }
        }
        else {
            xeve_itx_pb4b(src, dst, shift, line, step);
        }
    }
    else {
        if(line > 2) {
            s32* pel_src = src;
            s16* pel_dst = dst;

            int32x4_t r0, r1, r2, r3, a0, a1, b0, b1, t0, t1, e0, o0, e1, o1;
            int32x4_t v0, v1, v2, v3;

            const int32x4_t coef_0_13 = vdupq_n_s64(((s64)xeve_tbl_tm4[3][0] << 32) | xeve_tbl_tm4[1][0]);
            const int32x4_t coef_1_13 = vdupq_n_s64(((s64)xeve_tbl_tm4[3][1] << 32) | xeve_tbl_tm4[1][1]);
            const int32x4_t coef_1_02 = vdupq_n_s64(((s64)xeve_tbl_tm4[2][1] << 32) | xeve_tbl_tm4[0][1]);
            const int32x4_t coef_0_02 = vdupq_n_s64(((s64)xeve_tbl_tm4[0][0] << 32) | xeve_tbl_tm4[2][0]);

            const int32x4_t add_s2  = vdupq_n_s32(add);
            int32x4_t       max_val = vdupq_n_s32(MAX_TX_VAL_32);
            int32x4_t       min_val = vdupq_n_s32(MIN_TX_VAL_32);

            int i_src1 = line;
            int i_src2 = i_src1 + i_src1;
            int i_src3 = i_src2 + i_src1;

            for(int j = 0; j < line; j += 4) {
                r0 = vld1q_s32((pel_src + j));
                r1 = vld1q_s32((pel_src + i_src1 + j));
                r2 = vld1q_s32((pel_src + i_src2 + j));
                r3 = vld1q_s32((pel_src + i_src3 + j));

                a0 = vzip1q_s32(r0, r2);
                b0 = vzip2q_s32(r0, r2);

                a1 = vzip1q_s32(r1, r3);
                b1 = vzip2q_s32(r1, r3);

                XEVE_MADD_S32(t0, t1, e0, a0, b0, coef_0_02);
                XEVE_MADD_S32(t0, t1, e1, a0, b0, coef_1_02);
                XEVE_MADD_S32(t0, t1, o0, a1, b1, coef_0_13);
                XEVE_MADD_S32(t0, t1, o1, a1, b1, coef_1_13);

                v0 = vaddq_s32(e0, o0);
                v3 = vsubq_s32(e0, o0);
                v1 = vaddq_s32(e1, o1);
                v2 = vsubq_s32(e1, o1);
                v0 = vaddq_s32(v0, add_s2);
                v1 = vaddq_s32(v1, add_s2);
                v2 = vaddq_s32(v2, add_s2);
                v3 = vaddq_s32(v3, add_s2);

                v0 = vshlq_s32(v0, vdupq_n_s32(-shift));
                v1 = vshlq_s32(v1, vdupq_n_s32(-shift));
                v2 = vshlq_s32(v2, vdupq_n_s32(-shift));
                v3 = vshlq_s32(v3, vdupq_n_s32(-shift));

                // CLIPPING
                v0 = vmaxq_s32(v0, min_val);
                v1 = vmaxq_s32(v1, min_val);
                v2 = vmaxq_s32(v2, min_val);
                v3 = vmaxq_s32(v3, min_val);

                v0 = vminq_s32(v0, max_val);
                v1 = vminq_s32(v1, max_val);
                v2 = vminq_s32(v2, max_val);
                v3 = vminq_s32(v3, max_val);

                // Pack to 16 bits
                t0 = vcombine_s16(vqmovn_s32(v0), vqmovn_s32(v2));
                t1 = vcombine_s16(vqmovn_s32(v1), vqmovn_s32(v3));

                v0 = vzip1q_s16(t0, t1);
                v1 = vzip2q_s16(t0, t1);
                t0 = vzip1q_s32(v0, v1);
                t1 = vzip2q_s32(v0, v1);

                vst1q_s16(pel_dst, t0);
                vst1q_s16((pel_dst + 8), t1);

                pel_dst += 16;
            }
        }
        else {
            xeve_itx_pb4b(src, dst, shift, line, step);
        }
    }
}

void xeve_itx_pb8b_neon(void* src, void* dst, int shift, int line, int step)
{
    int add = shift == 0 ? 0 : 1 << (shift - 1);

    if(step == 0) {
        if(line > 2) {
            s16* pel_src = src;
            s32* pel_dst = dst;

            int16x4_t   r0, r1, r2, r3, r4, r5, r6, r7;
            int16x4x2_t a0, a1, a2, a3;
            int32x4_t   e0, e1, e2, e3, o0, o1, o2, o3, eo0, eo1, ee0, ee1;
            int32x4_t   v0, v1, v2, v3, v4, v5, v6, v7;
            int32x4_t   t0, t1, t2, t3;

            int32x4_t max_val = vdupq_n_s32(MAX_TX_VAL_32);
            int32x4_t min_val = vdupq_n_s32(MIN_TX_VAL_32);

            int32x4_t coef[4][4];

            for(int i = 0; i < 4; i++) {
                for(int j = 0; j < 4; j++) {
                    coef[i][j] = vdupq_n_s32(((s32)(xeve_tbl_tm8[j + 4][i]) << 16) | (xeve_tbl_tm8[j][i] & 0xFFFF));
                }
            }

            int i_src1 = line;
            int i_src2 = i_src1 + i_src1;
            int i_src3 = i_src2 + i_src1;
            int i_src4 = i_src3 + i_src1;
            int i_src5 = i_src4 + i_src1;
            int i_src6 = i_src5 + i_src1;
            int i_src7 = i_src6 + i_src1;

            for(int j = 0; j < line; j += 4) {
                r0 = vld1_s16((pel_src + j));
                r1 = vld1_s16((pel_src + i_src1 + j));
                r2 = vld1_s16((pel_src + i_src2 + j));
                r3 = vld1_s16((pel_src + i_src3 + j));
                r4 = vld1_s16((pel_src + i_src4 + j));
                r5 = vld1_s16((pel_src + i_src5 + j));
                r6 = vld1_s16((pel_src + i_src6 + j));
                r7 = vld1_s16((pel_src + i_src7 + j));

                a1 = vzip_s16(r1, r5);
                a3 = vzip_s16(r3, r7);

                t0 = vmadd_s16(a1, coef[0][1]);
                t1 = vmadd_s16(a3, coef[0][3]);
                o0 = vaddq_s32(t0, t1);

                t0 = vmadd_s16(a1, coef[1][1]);
                t1 = vmadd_s16(a3, coef[1][3]);
                o1 = vaddq_s32(t0, t1);

                t0 = vmadd_s16(a1, coef[2][1]);
                t1 = vmadd_s16(a3, coef[2][3]);
                o2 = vaddq_s32(t0, t1);

                t0 = vmadd_s16(a1, coef[3][1]);
                t1 = vmadd_s16(a3, coef[3][3]);
                o3 = vaddq_s32(t0, t1);

                a0 = vzip_s16(r0, r4);
                a2 = vzip_s16(r2, r6);

                eo0 = vmadd_s16(a2, coef[0][2]);
                eo1 = vmadd_s16(a2, coef[1][2]);
                ee0 = vmadd_s16(a0, coef[0][0]);
                ee1 = vmadd_s16(a0, coef[1][0]);

                e0 = vaddq_s32(ee0, eo0);
                e3 = vsubq_s32(ee0, eo0);
                e1 = vaddq_s32(ee1, eo1);
                e2 = vsubq_s32(ee1, eo1);

                v0 = vaddq_s32(e0, o0);
                v7 = vsubq_s32(e0, o0);
                v1 = vaddq_s32(e1, o1);
                v6 = vsubq_s32(e1, o1);
                v2 = vaddq_s32(e2, o2);
                v5 = vsubq_s32(e2, o2);
                v3 = vaddq_s32(e3, o3);
                v4 = vsubq_s32(e3, o3);

                // CLIPPING
                XEVE_ITX_CLIP_NEON(v0, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v1, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v2, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v3, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v4, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v5, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v6, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v7, min_val, max_val);

                t0 = vzip1q_s32(v0, v1);
                t2 = vzip2q_s32(v0, v1);
                t1 = vzip1q_s32(v2, v3);
                t3 = vzip2q_s32(v2, v3);

                v0 = vzip1q_s64(t0, t1);
                v1 = vzip2q_s64(t0, t1);
                v2 = vzip1q_s64(t2, t3);
                v3 = vzip2q_s64(t2, t3);

                t0 = vzip1q_s32(v4, v5);
                t2 = vzip2q_s32(v4, v5);
                t1 = vzip1q_s32(v6, v7);
                t3 = vzip2q_s32(v6, v7);

                v4 = vzip1q_s64(t0, t1);
                v5 = vzip2q_s64(t0, t1);
                v6 = vzip1q_s64(t2, t3);
                v7 = vzip2q_s64(t2, t3);

                vst1q_s32((pel_dst), v0);
                vst1q_s32((pel_dst + 4), v4);
                vst1q_s32((pel_dst + 8), v1);
                vst1q_s32((pel_dst + 12), v5);
                vst1q_s32((pel_dst + 16), v2);
                vst1q_s32((pel_dst + 20), v6);
                vst1q_s32((pel_dst + 24), v3);
                vst1q_s32((pel_dst + 28), v7);

                pel_dst += 32;
            }
        }
        else {
            xeve_itx_pb8b(src, dst, shift, line, step);
        }
    }
    else {
        if(line > 2) {
            s32* pel_src = src;
            s16* pel_dst = dst;

            int32x4_t r0, r1, r2, r3, r4, r5, r6, r7;
            int32x4_t a0, a1, a2, a3, b0, b1, b2, b3;
            int32x4_t e0, e1, e2, e3, o0, o1, o2, o3, eo0, eo1, ee0, ee1;
            int32x4_t v0, v1, v2, v3, v4, v5, v6, v7;
            int32x4_t t0, t1, t2, t3;

            int32x4_t max_val = vdupq_n_s32(MAX_TX_VAL_32);
            int32x4_t min_val = vdupq_n_s32(MIN_TX_VAL_32);

            int32x4_t add_s2 = vdupq_n_s32(add);
            int32x4_t coef[4][4];

            for(int i = 0; i < 4; i++) {
                for(int j = 0; j < 4; j++) {
                    coef[i][j] = vdupq_n_s64(((s64)(xeve_tbl_tm8[j + 4][i]) << 32) | (xeve_tbl_tm8[j][i] & 0xFFFFFFFF));
                }
            }

            int i_src1 = line;
            int i_src2 = i_src1 + i_src1;
            int i_src3 = i_src2 + i_src1;
            int i_src4 = i_src3 + i_src1;
            int i_src5 = i_src4 + i_src1;
            int i_src6 = i_src5 + i_src1;
            int i_src7 = i_src6 + i_src1;

            for(int j = 0; j < line; j += 4) {
                r0 = vld1q_s32((pel_src + j));
                r1 = vld1q_s32((pel_src + i_src1 + j));
                r2 = vld1q_s32((pel_src + i_src2 + j));
                r3 = vld1q_s32((pel_src + i_src3 + j));
                r4 = vld1q_s32((pel_src + i_src4 + j));
                r5 = vld1q_s32((pel_src + i_src5 + j));
                r6 = vld1q_s32((pel_src + i_src6 + j));
                r7 = vld1q_s32((pel_src + i_src7 + j));

                a1 = vzip1q_s32(r1, r5);
                b1 = vzip2q_s32(r1, r5);
                a3 = vzip1q_s32(r3, r7);
                b3 = vzip2q_s32(r3, r7);

                XEVE_MADD_S32(t0,
                              t1,
                              t2,
                              a1,
                              b1,
                              coef[0][1]);  // the macro stores the madd value in 3rd register i.e t2 in this case
                XEVE_MADD_S32(t0, t1, t3, a3, b3, coef[0][3]);
                o0 = vaddq_s32(t2, t3);

                XEVE_MADD_S32(t0, t1, t2, a1, b1, coef[1][1]);
                XEVE_MADD_S32(t0, t1, t3, a3, b3, coef[1][3]);
                o1 = vaddq_s32(t2, t3);

                XEVE_MADD_S32(t0, t1, t2, a1, b1, coef[2][1]);
                XEVE_MADD_S32(t0, t1, t3, a3, b3, coef[2][3]);
                o2 = vaddq_s32(t2, t3);

                XEVE_MADD_S32(t0, t1, t2, a1, b1, coef[3][1]);
                XEVE_MADD_S32(t0, t1, t3, a3, b3, coef[3][3]);
                o3 = vaddq_s32(t2, t3);

                a0 = vzip1q_s32(r0, r4);
                b0 = vzip2q_s32(r0, r4);
                a2 = vzip1q_s32(r2, r6);
                b2 = vzip2q_s32(r2, r6);

                XEVE_MADD_S32(t0, t1, eo0, a2, b2, coef[0][2]);
                XEVE_MADD_S32(t0, t1, eo1, a2, b2, coef[1][2]);
                XEVE_MADD_S32(t0, t1, ee0, a0, b0, coef[0][0]);
                XEVE_MADD_S32(t0, t1, ee1, a0, b0, coef[1][0]);

                e0 = vaddq_s32(ee0, eo0);
                e3 = vsubq_s32(ee0, eo0);
                e1 = vaddq_s32(ee1, eo1);
                e2 = vsubq_s32(ee1, eo1);

                v0 = vaddq_s32(e0, o0);
                v7 = vsubq_s32(e0, o0);
                v1 = vaddq_s32(e1, o1);
                v6 = vsubq_s32(e1, o1);
                v2 = vaddq_s32(e2, o2);
                v5 = vsubq_s32(e2, o2);
                v3 = vaddq_s32(e3, o3);
                v4 = vsubq_s32(e3, o3);

                // CLIPPING
                XEVE_ITX_SHIFT_CLIP_NEON(v0, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v1, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v2, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v3, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v4, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v5, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v6, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v7, add_s2, shift, min_val, max_val);

                // Pack to 16 bits
                t0 = vcombine_s16(vqmovn_s32(v0), vqmovn_s32(v4));
                t1 = vcombine_s16(vqmovn_s32(v1), vqmovn_s32(v5));
                t2 = vcombine_s16(vqmovn_s32(v2), vqmovn_s32(v6));
                t3 = vcombine_s16(vqmovn_s32(v3), vqmovn_s32(v7));

                v0 = vzip1q_s16(t0, t1);
                v1 = vzip1q_s16(t2, t3);
                v2 = vzip2q_s16(t0, t1);
                v3 = vzip2q_s16(t2, t3);

                t0 = vzip1q_s32(v0, v1);
                t1 = vzip1q_s32(v2, v3);
                t2 = vzip2q_s32(v0, v1);
                t3 = vzip2q_s32(v2, v3);

                v0 = vzip1q_s64(t0, t1);
                v1 = vzip2q_s64(t0, t1);
                v2 = vzip1q_s64(t2, t3);
                v3 = vzip2q_s64(t2, t3);

                // store
                vst1q_s16(pel_dst, v0);
                vst1q_s16((pel_dst + 8), v1);
                vst1q_s16((pel_dst + 16), v2);
                vst1q_s16((pel_dst + 24), v3);
                pel_dst += 32;
            }
        }
        else {
            xeve_itx_pb8b(src, dst, shift, line, step);
        }
    }
}

void xeve_itx_pb16b_neon(void* src, void* dst, int shift, int line, int step)
{
    int add = shift == 0 ? 0 : 1 << (shift - 1);

    if(step == 0) {
        if(line > 2) {
            s16* pel_src = src;
            s32* pel_dst = dst;

            int16x4_t   r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15;
            int16x4x2_t a0, a1, a2, a3, a4, a5, a6, a7;
            int32x4_t   o0, o1, o2, o3, o4, o5, o6, o7;
            int32x4_t   e0, e1, e2, e3, e4, e5, e6, e7;
            int32x4_t   eo0, eo1, eo2, eo3, ee0, ee1, ee2, ee3;
            int32x4_t   eeo0, eeo1, eee0, eee1;
            int32x4_t   v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15;
            int32x4_t   t0, t1, t2, t3, t5, t6, t7;

            int32x4_t max_val = vdupq_n_s32(MAX_TX_VAL_32);
            int32x4_t min_val = vdupq_n_s32(MIN_TX_VAL_32);

            int32x4_t coef[8][8];

            for(int i = 0; i < 8; i++) {
                for(int j = 0; j < 8; j++) {
                    coef[i][j] = vdupq_n_s32(((s32)(xeve_tbl_tm16[j + 8][i]) << 16) | (xeve_tbl_tm16[j][i] & 0xFFFF));
                }
            }

            int i_src1  = line;
            int i_src2  = i_src1 + i_src1;
            int i_src3  = i_src2 + i_src1;
            int i_src4  = i_src3 + i_src1;
            int i_src5  = i_src4 + i_src1;
            int i_src6  = i_src5 + i_src1;
            int i_src7  = i_src6 + i_src1;
            int i_src8  = i_src7 + i_src1;
            int i_src9  = i_src8 + i_src1;
            int i_src10 = i_src9 + i_src1;
            int i_src11 = i_src10 + i_src1;
            int i_src12 = i_src11 + i_src1;
            int i_src13 = i_src12 + i_src1;
            int i_src14 = i_src13 + i_src1;
            int i_src15 = i_src14 + i_src1;

            for(int j = 0; j < line; j += 4) {
                r0  = vld1_s16((pel_src + j));
                r1  = vld1_s16((pel_src + i_src1 + j));
                r2  = vld1_s16((pel_src + i_src2 + j));
                r3  = vld1_s16((pel_src + i_src3 + j));
                r4  = vld1_s16((pel_src + i_src4 + j));
                r5  = vld1_s16((pel_src + i_src5 + j));
                r6  = vld1_s16((pel_src + i_src6 + j));
                r7  = vld1_s16((pel_src + i_src7 + j));
                r8  = vld1_s16((pel_src + i_src8 + j));
                r9  = vld1_s16((pel_src + i_src9 + j));
                r10 = vld1_s16((pel_src + i_src10 + j));
                r11 = vld1_s16((pel_src + i_src11 + j));
                r12 = vld1_s16((pel_src + i_src12 + j));
                r13 = vld1_s16((pel_src + i_src13 + j));
                r14 = vld1_s16((pel_src + i_src14 + j));
                r15 = vld1_s16((pel_src + i_src15 + j));

                a1 = vzip_s16(r1, r9);
                a3 = vzip_s16(r3, r11);
                a5 = vzip_s16(r5, r13);
                a7 = vzip_s16(r7, r15);

#define XEVE_ITX16_O_NEON(dst, idx)    \
    t1  = vmadd_s16(a1, coef[idx][1]); \
    t3  = vmadd_s16(a3, coef[idx][3]); \
    t5  = vmadd_s16(a5, coef[idx][5]); \
    t7  = vmadd_s16(a7, coef[idx][7]); \
    v0  = vaddq_s32(t1, t3);           \
    v1  = vaddq_s32(t5, t7);           \
    dst = vaddq_s32(v0, v1);

                XEVE_ITX16_O_NEON(o0, 0);
                XEVE_ITX16_O_NEON(o1, 1);
                XEVE_ITX16_O_NEON(o2, 2);
                XEVE_ITX16_O_NEON(o3, 3);
                XEVE_ITX16_O_NEON(o4, 4);
                XEVE_ITX16_O_NEON(o5, 5);
                XEVE_ITX16_O_NEON(o6, 6);
                XEVE_ITX16_O_NEON(o7, 7);

                a2 = vzip_s16(r2, r10);
                a6 = vzip_s16(r6, r14);

#define XEVE_ITX16_EO_NEON(dst, idx)   \
    t2  = vmadd_s16(a2, coef[idx][2]); \
    t6  = vmadd_s16(a6, coef[idx][6]); \
    dst = vaddq_s32(t2, t6);

                XEVE_ITX16_EO_NEON(eo0, 0);
                XEVE_ITX16_EO_NEON(eo1, 1);
                XEVE_ITX16_EO_NEON(eo2, 2);
                XEVE_ITX16_EO_NEON(eo3, 3);

#undef XEVE_ITX16_EO_NEON

                a4 = vzip_s16(r4, r12);
                a0 = vzip_s16(r0, r8);

                eeo0 = vmadd_s16(a4, coef[0][4]);
                eeo1 = vmadd_s16(a4, coef[1][4]);
                eee0 = vmadd_s16(a0, coef[0][0]);
                eee1 = vmadd_s16(a0, coef[1][0]);

                ee0 = vaddq_s32(eee0, eeo0);
                ee1 = vaddq_s32(eee1, eeo1);
                ee2 = vsubq_s32(eee1, eeo1);
                ee3 = vsubq_s32(eee0, eeo0);

                e0 = vaddq_s32(ee0, eo0);
                e1 = vaddq_s32(ee1, eo1);
                e2 = vaddq_s32(ee2, eo2);
                e3 = vaddq_s32(ee3, eo3);
                e4 = vsubq_s32(ee3, eo3);
                e5 = vsubq_s32(ee2, eo2);
                e6 = vsubq_s32(ee1, eo1);
                e7 = vsubq_s32(ee0, eo0);

                v0  = vaddq_s32(e0, o0);
                v1  = vaddq_s32(e1, o1);
                v2  = vaddq_s32(e2, o2);
                v3  = vaddq_s32(e3, o3);
                v4  = vaddq_s32(e4, o4);
                v5  = vaddq_s32(e5, o5);
                v6  = vaddq_s32(e6, o6);
                v7  = vaddq_s32(e7, o7);
                v8  = vsubq_s32(e7, o7);
                v9  = vsubq_s32(e6, o6);
                v10 = vsubq_s32(e5, o5);
                v11 = vsubq_s32(e4, o4);
                v12 = vsubq_s32(e3, o3);
                v13 = vsubq_s32(e2, o2);
                v14 = vsubq_s32(e1, o1);
                v15 = vsubq_s32(e0, o0);

                // CLIPPING
                XEVE_ITX_CLIP_NEON(v0, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v1, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v2, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v3, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v4, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v5, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v6, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v7, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v8, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v9, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v10, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v11, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v12, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v13, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v14, min_val, max_val);
                XEVE_ITX_CLIP_NEON(v15, min_val, max_val);

#define XEVE_ITDQ_TRANSPOS_NEON(s0, s1, s2, s3, t0, t1, t2, t3) \
    t0 = vzip1q_s32(s0, s1);                                    \
    t2 = vzip2q_s32(s0, s1);                                    \
    t1 = vzip1q_s32(s2, s3);                                    \
    t3 = vzip2q_s32(s2, s3);                                    \
                                                                \
    s0 = vzip1q_s64(t0, t1);                                    \
    s1 = vzip2q_s64(t0, t1);                                    \
    s2 = vzip1q_s64(t2, t3);                                    \
    s3 = vzip2q_s64(t2, t3);

                XEVE_ITDQ_TRANSPOS_NEON(v0, v1, v2, v3, t0, t1, t2, t3);
                XEVE_ITDQ_TRANSPOS_NEON(v4, v5, v6, v7, t0, t1, t2, t3);
                XEVE_ITDQ_TRANSPOS_NEON(v8, v9, v10, v11, t0, t1, t2, t3);
                XEVE_ITDQ_TRANSPOS_NEON(v12, v13, v14, v15, t0, t1, t2, t3);

                // STORE
                vst1q_s32((pel_dst), v0);
                vst1q_s32((pel_dst + 4), v4);
                vst1q_s32((pel_dst + 8), v8);
                vst1q_s32((pel_dst + 12), v12);
                vst1q_s32((pel_dst + 16), v1);
                vst1q_s32((pel_dst + 20), v5);
                vst1q_s32((pel_dst + 24), v9);
                vst1q_s32((pel_dst + 28), v13);
                vst1q_s32((pel_dst + 32), v2);
                vst1q_s32((pel_dst + 36), v6);
                vst1q_s32((pel_dst + 40), v10);
                vst1q_s32((pel_dst + 44), v14);
                vst1q_s32((pel_dst + 48), v3);
                vst1q_s32((pel_dst + 52), v7);
                vst1q_s32((pel_dst + 56), v11);
                vst1q_s32((pel_dst + 60), v15);

                pel_dst += 64;
            }
        }
        else {
            xeve_itx_pb16b(src, dst, shift, line, step);
        }
    }
    else {
        if(line > 2) {
            s32* pel_src = src;
            s16* pel_dst = dst;

            int32x4_t r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12, r13, r14, r15;
            int32x4_t a0, a1, a2, a3, a4, a5, a6, a7;
            int32x4_t b0, b1, b2, b3, b4, b5, b6, b7;
            int32x4_t o0, o1, o2, o3, o4, o5, o6, o7;
            int32x4_t e0, e1, e2, e3, e4, e5, e6, e7;
            int32x4_t eo0, eo1, eo2, eo3, ee0, ee1, ee2, ee3;
            int32x4_t eeo0, eeo1, eee0, eee1;
            int32x4_t v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12, v13, v14, v15;
            int32x4_t t0, t1, t2, t3, t4, t5, t6, t7;

            const int32x4_t max_val = vdupq_n_s32(MAX_TX_VAL_32);
            const int32x4_t min_val = vdupq_n_s32(MIN_TX_VAL_32);
            const int32x4_t add_s2  = vdupq_n_s32(add);
            int32x4_t       coef[8][8];

            for(int i = 0; i < 8; i++) {
                for(int j = 0; j < 8; j++) {
                    coef[i][j] =
                        vdupq_n_s64(((s64)(xeve_tbl_tm16[j + 8][i]) << 32) | (xeve_tbl_tm16[j][i] & 0xFFFFFFFF));
                }
            }

            int i_src1  = line;
            int i_src2  = i_src1 + i_src1;
            int i_src3  = i_src2 + i_src1;
            int i_src4  = i_src3 + i_src1;
            int i_src5  = i_src4 + i_src1;
            int i_src6  = i_src5 + i_src1;
            int i_src7  = i_src6 + i_src1;
            int i_src8  = i_src7 + i_src1;
            int i_src9  = i_src8 + i_src1;
            int i_src10 = i_src9 + i_src1;
            int i_src11 = i_src10 + i_src1;
            int i_src12 = i_src11 + i_src1;
            int i_src13 = i_src12 + i_src1;
            int i_src14 = i_src13 + i_src1;
            int i_src15 = i_src14 + i_src1;

            for(int j = 0; j < line; j += 4) {
                r0  = vld1q_s32((pel_src + j));
                r1  = vld1q_s32((pel_src + i_src1 + j));
                r2  = vld1q_s32((pel_src + i_src2 + j));
                r3  = vld1q_s32((pel_src + i_src3 + j));
                r4  = vld1q_s32((pel_src + i_src4 + j));
                r5  = vld1q_s32((pel_src + i_src5 + j));
                r6  = vld1q_s32((pel_src + i_src6 + j));
                r7  = vld1q_s32((pel_src + i_src7 + j));
                r8  = vld1q_s32((pel_src + i_src8 + j));
                r9  = vld1q_s32((pel_src + i_src9 + j));
                r10 = vld1q_s32((pel_src + i_src10 + j));
                r11 = vld1q_s32((pel_src + i_src11 + j));
                r12 = vld1q_s32((pel_src + i_src12 + j));
                r13 = vld1q_s32((pel_src + i_src13 + j));
                r14 = vld1q_s32((pel_src + i_src14 + j));
                r15 = vld1q_s32((pel_src + i_src15 + j));

                a1 = vzip1q_s32(r1, r9);
                b1 = vzip2q_s32(r1, r9);

                a3 = vzip1q_s32(r3, r11);
                b3 = vzip2q_s32(r3, r11);

                a5 = vzip1q_s32(r5, r13);
                b5 = vzip2q_s32(r5, r13);

                a7 = vzip1q_s32(r7, r15);
                b7 = vzip2q_s32(r7, r15);

#define XEVE_ITX16_0_32B_NEON(dst, idx)              \
    XEVE_MADD_S32(t0, t1, v0, a1, b1, coef[idx][1]); \
    XEVE_MADD_S32(t0, t1, v1, a3, b3, coef[idx][3]); \
    XEVE_MADD_S32(t0, t1, v2, a5, b5, coef[idx][5]); \
    XEVE_MADD_S32(t0, t1, v3, a7, b7, coef[idx][7]); \
    t0  = vaddq_s32(v0, v1);                         \
    t1  = vaddq_s32(v2, v3);                         \
    dst = vaddq_s32(t0, t1);

                XEVE_ITX16_0_32B_NEON(o0, 0);
                XEVE_ITX16_0_32B_NEON(o1, 1);
                XEVE_ITX16_0_32B_NEON(o2, 2);
                XEVE_ITX16_0_32B_NEON(o3, 3);
                XEVE_ITX16_0_32B_NEON(o4, 4);
                XEVE_ITX16_0_32B_NEON(o5, 5);
                XEVE_ITX16_0_32B_NEON(o6, 6);
                XEVE_ITX16_0_32B_NEON(o7, 7);

#undef XEVE_ITX16_0_32B_NEON

                a2 = vzip1q_s32(r2, r10);
                b2 = vzip2q_s32(r2, r10);
                a6 = vzip1q_s32(r6, r14);
                b6 = vzip2q_s32(r6, r14);

#define XEVE_ITX16_EO_32B_NEON(dst, idx)             \
    XEVE_MADD_S32(t0, t1, v0, a2, b2, coef[idx][2]); \
    XEVE_MADD_S32(t0, t1, v1, a6, b6, coef[idx][6]); \
    dst = vaddq_s32(v0, v1);

                XEVE_ITX16_EO_32B_NEON(eo0, 0);
                XEVE_ITX16_EO_32B_NEON(eo1, 1);
                XEVE_ITX16_EO_32B_NEON(eo2, 2);
                XEVE_ITX16_EO_32B_NEON(eo3, 3);

#undef XEVE_ITX16_EO_32B_NEON

                a0 = vzip1q_s32(r0, r8);
                b0 = vzip2q_s32(r0, r8);
                a4 = vzip1q_s32(r4, r12);
                b4 = vzip2q_s32(r4, r12);

                XEVE_MADD_S32(t0, t1, eeo0, a4, b4, coef[0][4]);
                XEVE_MADD_S32(t0, t1, eeo1, a4, b4, coef[1][4]);
                XEVE_MADD_S32(t0, t1, eee0, a0, b0, coef[0][0]);
                XEVE_MADD_S32(t0, t1, eee1, a0, b0, coef[1][0]);

                ee0 = vaddq_s32(eee0, eeo0);
                ee1 = vaddq_s32(eee1, eeo1);
                ee2 = vsubq_s32(eee1, eeo1);
                ee3 = vsubq_s32(eee0, eeo0);

                e0 = vaddq_s32(ee0, eo0);
                e1 = vaddq_s32(ee1, eo1);
                e2 = vaddq_s32(ee2, eo2);
                e3 = vaddq_s32(ee3, eo3);
                e4 = vsubq_s32(ee3, eo3);
                e5 = vsubq_s32(ee2, eo2);
                e6 = vsubq_s32(ee1, eo1);
                e7 = vsubq_s32(ee0, eo0);

                v0  = vaddq_s32(e0, o0);
                v1  = vaddq_s32(e1, o1);
                v2  = vaddq_s32(e2, o2);
                v3  = vaddq_s32(e3, o3);
                v4  = vaddq_s32(e4, o4);
                v5  = vaddq_s32(e5, o5);
                v6  = vaddq_s32(e6, o6);
                v7  = vaddq_s32(e7, o7);
                v8  = vsubq_s32(e7, o7);
                v9  = vsubq_s32(e6, o6);
                v10 = vsubq_s32(e5, o5);
                v11 = vsubq_s32(e4, o4);
                v12 = vsubq_s32(e3, o3);
                v13 = vsubq_s32(e2, o2);
                v14 = vsubq_s32(e1, o1);
                v15 = vsubq_s32(e0, o0);

                XEVE_ITX_SHIFT_CLIP_NEON(v0, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v1, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v2, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v3, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v4, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v5, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v6, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v7, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v8, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v9, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v10, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v11, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v12, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v13, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v14, add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v15, add_s2, shift, min_val, max_val);

                t0 = vcombine_s16(vqmovn_s32(v0), vqmovn_s32(v8));
                t1 = vcombine_s16(vqmovn_s32(v1), vqmovn_s32(v9));
                t2 = vcombine_s16(vqmovn_s32(v2), vqmovn_s32(v10));
                t3 = vcombine_s16(vqmovn_s32(v3), vqmovn_s32(v11));
                t4 = vcombine_s16(vqmovn_s32(v4), vqmovn_s32(v12));
                t5 = vcombine_s16(vqmovn_s32(v5), vqmovn_s32(v13));
                t6 = vcombine_s16(vqmovn_s32(v6), vqmovn_s32(v14));
                t7 = vcombine_s16(vqmovn_s32(v7), vqmovn_s32(v15));

                v0 = vzip1q_s16(t0, t1);
                v1 = vzip1q_s16(t2, t3);
                v2 = vzip1q_s16(t4, t5);
                v3 = vzip1q_s16(t6, t7);
                v4 = vzip2q_s16(t0, t1);
                v5 = vzip2q_s16(t2, t3);
                v6 = vzip2q_s16(t4, t5);
                v7 = vzip2q_s16(t6, t7);

                t0 = vzip1q_s32(v0, v1);
                t1 = vzip1q_s32(v2, v3);
                t2 = vzip1q_s32(v4, v5);
                t3 = vzip1q_s32(v6, v7);
                t4 = vzip2q_s32(v0, v1);
                t5 = vzip2q_s32(v2, v3);
                t6 = vzip2q_s32(v4, v5);
                t7 = vzip2q_s32(v6, v7);

                v0 = vzip1q_s64(t0, t1);
                v1 = vzip1q_s64(t2, t3);
                v2 = vzip2q_s64(t0, t1);
                v3 = vzip2q_s64(t2, t3);
                v4 = vzip1q_s64(t4, t5);
                v5 = vzip1q_s64(t6, t7);
                v6 = vzip2q_s64(t4, t5);
                v7 = vzip2q_s64(t6, t7);

                vst1q_s16(pel_dst, v0);
                vst1q_s16((pel_dst + 8), v1);
                vst1q_s16((pel_dst + 16), v2);
                vst1q_s16((pel_dst + 24), v3);
                vst1q_s16((pel_dst + 32), v4);
                vst1q_s16((pel_dst + 40), v5);
                vst1q_s16((pel_dst + 48), v6);
                vst1q_s16((pel_dst + 56), v7);

                pel_dst += 64;
            }
        }
        else {
            xeve_itx_pb16b(src, dst, shift, line, step);
        }
    }
}

void xeve_itx_pb32b_neon(void* src, void* dst, int shift, int line, int step)
{
    int add = shift == 0 ? 0 : 1 << (shift - 1);

    if(step == 0) {
        if(line > 2) {
            s16* pel_src = src;
            s32* pel_dst = dst;

            int16x4_t   r[32];
            int16x4x2_t a[32];
            int32x4_t   o[16], e[16], eo[8], ee[8], eeo[4], eee[4], eeeo[2], eeee[2];
            int32x4_t   v[32], t[16], d[32];

            int32x4_t max_val = vdupq_n_s32(MAX_TX_VAL_32);
            int32x4_t min_val = vdupq_n_s32(MIN_TX_VAL_32);
            int32x4_t coef[16][16];

            for(int i = 0; i < 16; i++) {
                for(int j = 0; j < 16; j++) {
                    coef[i][j] = vdupq_n_s32(((s32)(xeve_tbl_tm32[j + 16][i]) << 16) | (xeve_tbl_tm32[j][i] & 0xFFFF));
                }
            }

            int i, j, i_src[32];
            i_src[0] = 0;

            for(i = 1; i < 32; i++) {
                i_src[i] = i_src[i - 1] + line;
            }

            for(j = 0; j < line; j += 4) {
                for(i = 0; i < 32; i++) {
                    r[i] = vld1_s16((pel_src + i_src[i] + j));
                }

                for(i = 0; i < 16; i++) {
                    a[i] = vzip_s16(r[i], r[i + 16]);
                }

#define XEVE_ITX32_O_NEON(dst, idx)          \
    t[1]  = vmadd_s16(a[1], coef[idx][1]);   \
    t[3]  = vmadd_s16(a[3], coef[idx][3]);   \
    t[5]  = vmadd_s16(a[5], coef[idx][5]);   \
    t[7]  = vmadd_s16(a[7], coef[idx][7]);   \
    t[9]  = vmadd_s16(a[9], coef[idx][9]);   \
    t[11] = vmadd_s16(a[11], coef[idx][11]); \
    t[13] = vmadd_s16(a[13], coef[idx][13]); \
    t[15] = vmadd_s16(a[15], coef[idx][15]); \
    d[0]  = vaddq_s32(t[1], t[3]);           \
    d[1]  = vaddq_s32(t[5], t[7]);           \
    d[2]  = vaddq_s32(t[9], t[11]);          \
    d[3]  = vaddq_s32(t[13], t[15]);         \
    t[0]  = vaddq_s32(d[0], d[1]);           \
    t[1]  = vaddq_s32(d[2], d[3]);           \
    dst   = vaddq_s32(t[0], t[1]);

                for(i = 0; i < 16; i++) {
                    XEVE_ITX32_O_NEON(o[i], i);
                }

#undef XEVE_ITX32_O_NEON

#define XEVE_ITX32_EO_NEON(dst, idx)         \
    t[2]  = vmadd_s16(a[2], coef[idx][2]);   \
    t[6]  = vmadd_s16(a[6], coef[idx][6]);   \
    t[10] = vmadd_s16(a[10], coef[idx][10]); \
    t[14] = vmadd_s16(a[14], coef[idx][14]); \
    d[0]  = vaddq_s32(t[2], t[6]);           \
    d[1]  = vaddq_s32(t[10], t[14]);         \
    dst   = vaddq_s32(d[0], d[1]);

                for(int i = 0; i < 8; i++) {
                    XEVE_ITX32_EO_NEON(eo[i], i);
                }
#undef XEVE_ITX32_EO

#define XEVE_ITX32_EEO_NEON(dst, idx)        \
    t[4]  = vmadd_s16(a[4], coef[idx][4]);   \
    t[12] = vmadd_s16(a[12], coef[idx][12]); \
    dst   = vaddq_s32(t[4], t[12]);

                for(int i = 0; i < 4; i++) {
                    XEVE_ITX32_EEO_NEON(eeo[i], i);
                }
#undef XEVE_ITX32_EEO

                eeeo[0] = vmadd_s16(a[8], coef[0][8]);
                eeeo[1] = vmadd_s16(a[8], coef[1][8]);
                eeee[0] = vmadd_s16(a[0], coef[0][0]);
                eeee[1] = vmadd_s16(a[0], coef[1][0]);

                eee[0] = vaddq_s32(eeee[0], eeeo[0]);
                eee[1] = vaddq_s32(eeee[1], eeeo[1]);
                eee[2] = vsubq_s32(eeee[1], eeeo[1]);
                eee[3] = vsubq_s32(eeee[0], eeeo[0]);

                ee[0] = vaddq_s32(eee[0], eeo[0]);
                ee[1] = vaddq_s32(eee[1], eeo[1]);
                ee[2] = vaddq_s32(eee[2], eeo[2]);
                ee[3] = vaddq_s32(eee[3], eeo[3]);
                ee[4] = vsubq_s32(eee[3], eeo[3]);
                ee[5] = vsubq_s32(eee[2], eeo[2]);
                ee[6] = vsubq_s32(eee[1], eeo[1]);
                ee[7] = vsubq_s32(eee[0], eeo[0]);

                e[0] = vaddq_s32(ee[0], eo[0]);
                e[1] = vaddq_s32(ee[1], eo[1]);
                e[2] = vaddq_s32(ee[2], eo[2]);
                e[3] = vaddq_s32(ee[3], eo[3]);
                e[4] = vaddq_s32(ee[4], eo[4]);
                e[5] = vaddq_s32(ee[5], eo[5]);
                e[6] = vaddq_s32(ee[6], eo[6]);
                e[7] = vaddq_s32(ee[7], eo[7]);

                e[8]  = vsubq_s32(ee[7], eo[7]);
                e[9]  = vsubq_s32(ee[6], eo[6]);
                e[10] = vsubq_s32(ee[5], eo[5]);
                e[11] = vsubq_s32(ee[4], eo[4]);
                e[12] = vsubq_s32(ee[3], eo[3]);
                e[13] = vsubq_s32(ee[2], eo[2]);
                e[14] = vsubq_s32(ee[1], eo[1]);
                e[15] = vsubq_s32(ee[0], eo[0]);

                v[0]  = vaddq_s32(e[0], o[0]);
                v[1]  = vaddq_s32(e[1], o[1]);
                v[2]  = vaddq_s32(e[2], o[2]);
                v[3]  = vaddq_s32(e[3], o[3]);
                v[4]  = vaddq_s32(e[4], o[4]);
                v[5]  = vaddq_s32(e[5], o[5]);
                v[6]  = vaddq_s32(e[6], o[6]);
                v[7]  = vaddq_s32(e[7], o[7]);
                v[8]  = vaddq_s32(e[8], o[8]);
                v[9]  = vaddq_s32(e[9], o[9]);
                v[10] = vaddq_s32(e[10], o[10]);
                v[11] = vaddq_s32(e[11], o[11]);
                v[12] = vaddq_s32(e[12], o[12]);
                v[13] = vaddq_s32(e[13], o[13]);
                v[14] = vaddq_s32(e[14], o[14]);
                v[15] = vaddq_s32(e[15], o[15]);

                v[16] = vsubq_s32(e[15], o[15]);
                v[17] = vsubq_s32(e[14], o[14]);
                v[18] = vsubq_s32(e[13], o[13]);
                v[19] = vsubq_s32(e[12], o[12]);
                v[20] = vsubq_s32(e[11], o[11]);
                v[21] = vsubq_s32(e[10], o[10]);
                v[22] = vsubq_s32(e[9], o[9]);
                v[23] = vsubq_s32(e[8], o[8]);
                v[24] = vsubq_s32(e[7], o[7]);
                v[25] = vsubq_s32(e[6], o[6]);
                v[26] = vsubq_s32(e[5], o[5]);
                v[27] = vsubq_s32(e[4], o[4]);
                v[28] = vsubq_s32(e[3], o[3]);
                v[29] = vsubq_s32(e[2], o[2]);
                v[30] = vsubq_s32(e[1], o[1]);
                v[31] = vsubq_s32(e[0], o[0]);

                for(int i = 0; i < 32; i++) {
                    XEVE_ITX_CLIP_NEON(v[i], min_val, max_val);
                }

#define XEVE_ITDQ_TRANSPOS_NEON(s0, s1, s2, s3, t0, t1, t2, t3) \
    t0 = vzip1q_s32(s0, s1);                                    \
    t2 = vzip2q_s32(s0, s1);                                    \
    t1 = vzip1q_s32(s2, s3);                                    \
    t3 = vzip2q_s32(s2, s3);                                    \
                                                                \
    s0 = vzip1q_s64(t0, t1);                                    \
    s1 = vzip2q_s64(t0, t1);                                    \
    s2 = vzip1q_s64(t2, t3);                                    \
    s3 = vzip2q_s64(t2, t3);
                XEVE_ITDQ_TRANSPOS_NEON(v[0], v[1], v[2], v[3], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[4], v[5], v[6], v[7], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[8], v[9], v[10], v[11], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[12], v[13], v[14], v[15], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[16], v[17], v[18], v[19], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[20], v[21], v[22], v[23], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[24], v[25], v[26], v[27], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[28], v[29], v[30], v[31], t[0], t[1], t[2], t[3]);
#undef XEVE_ITDQ_TRANSPOS_NEON

                // Store
                vst1q_s32((pel_dst), v[0]);
                vst1q_s32((pel_dst + 4), v[4]);
                vst1q_s32((pel_dst + 8), v[8]);
                vst1q_s32((pel_dst + 12), v[12]);
                vst1q_s32((pel_dst + 16), v[16]);
                vst1q_s32((pel_dst + 20), v[20]);
                vst1q_s32((pel_dst + 24), v[24]);
                vst1q_s32((pel_dst + 28), v[28]);
                vst1q_s32((pel_dst + 32), v[1]);
                vst1q_s32((pel_dst + 36), v[5]);
                vst1q_s32((pel_dst + 40), v[9]);
                vst1q_s32((pel_dst + 44), v[13]);
                vst1q_s32((pel_dst + 48), v[17]);
                vst1q_s32((pel_dst + 52), v[21]);
                vst1q_s32((pel_dst + 56), v[25]);
                vst1q_s32((pel_dst + 60), v[29]);
                vst1q_s32((pel_dst + 64), v[2]);
                vst1q_s32((pel_dst + 68), v[6]);
                vst1q_s32((pel_dst + 72), v[10]);
                vst1q_s32((pel_dst + 76), v[14]);
                vst1q_s32((pel_dst + 80), v[18]);
                vst1q_s32((pel_dst + 84), v[22]);
                vst1q_s32((pel_dst + 88), v[26]);
                vst1q_s32((pel_dst + 92), v[30]);
                vst1q_s32((pel_dst + 96), v[3]);
                vst1q_s32((pel_dst + 100), v[7]);
                vst1q_s32((pel_dst + 104), v[11]);
                vst1q_s32((pel_dst + 108), v[15]);
                vst1q_s32((pel_dst + 112), v[19]);
                vst1q_s32((pel_dst + 116), v[23]);
                vst1q_s32((pel_dst + 120), v[27]);
                vst1q_s32((pel_dst + 124), v[31]);

                pel_dst += 128;
            }
        }
        else {
            xeve_itx_pb32b(src, dst, shift, line, step);
        }
    }
    else {
        if(line > 2) {
            s32*      pel_src = src;
            s16*      pel_dst = dst;
            /*
                Variables are named in a way
                similar to SSE code for ITX
                for better understanding
            */
            int32x4_t r[32], a[16], b[16], e[16], o[16];
            int32x4_t eo[8], ee[8], eeo[4], eee[4], eeeo[2], eeee[2];
            int32x4_t v[32];
            int32x4_t t[16];

            const int32x4_t max_val = vdupq_n_s32(MAX_TX_VAL_32);
            const int32x4_t min_val = vdupq_n_s32(MIN_TX_VAL_32);
            const int32x4_t add_s2  = vdupq_n_s32(add);
            int32x4_t       coef[16][16];

            int i, j, i_src[32];

            for(i = 0; i < 16; i++) {
                for(j = 0; j < 16; j++) {
                    coef[i][j] =
                        vdupq_n_s64(((s64)(xeve_tbl_tm32[j + 16][i]) << 32) | (xeve_tbl_tm32[j][i] & 0xFFFFFFFF));
                }
            }

            i_src[0] = 0;
            for(i = 1; i < 32; i++) {
                i_src[i] = i_src[i - 1] + line;
            }

            for(j = 0; j < line; j += 4) {
                for(i = 0; i < 32; i++) {
                    r[i] = vld1q_s32((pel_src + i_src[i] + j));
                }

                for(i = 0; i < 16; i++) {
                    a[i] = vzip1q_s32(r[i], r[i + 16]);
                    b[i] = vzip2q_s32(r[i], r[i + 16]);
                }

#define XEVE_ITX_MADD(dst, r_idx, c_idx) XEVE_MADD_S32(t[0], t[1], dst, a[r_idx], b[r_idx], coef[c_idx][r_idx]);

#define XEVE_ITX32_0_32B_NEON(dst, idx) \
    XEVE_ITX_MADD(v[0], 1, idx)         \
    XEVE_ITX_MADD(v[1], 3, idx)         \
    XEVE_ITX_MADD(v[2], 5, idx)         \
    XEVE_ITX_MADD(v[3], 7, idx)         \
    XEVE_ITX_MADD(v[4], 9, idx)         \
    XEVE_ITX_MADD(v[5], 11, idx)        \
    XEVE_ITX_MADD(v[6], 13, idx)        \
    XEVE_ITX_MADD(v[7], 15, idx)        \
    t[0] = vaddq_s32(v[0], v[1]);       \
    t[1] = vaddq_s32(v[2], v[3]);       \
    t[2] = vaddq_s32(v[4], v[5]);       \
    t[3] = vaddq_s32(v[6], v[7]);       \
    v[0] = vaddq_s32(t[0], t[1]);       \
    v[1] = vaddq_s32(t[2], t[3]);       \
    dst  = vaddq_s32(v[0], v[1]);

                for(i = 0; i < 16; i++) {
                    XEVE_ITX32_0_32B_NEON(o[i], i);
                }
#undef XEVE_ITX32_0_32B_NEON

#define XEVE_ITX32_E0_32B_NEON(dst, idx) \
    XEVE_ITX_MADD(v[0], 2, idx)          \
    XEVE_ITX_MADD(v[1], 6, idx)          \
    XEVE_ITX_MADD(v[2], 10, idx)         \
    XEVE_ITX_MADD(v[3], 14, idx)         \
    t[0] = vaddq_s32(v[0], v[1]);        \
    t[1] = vaddq_s32(v[2], v[3]);        \
    dst  = vaddq_s32(t[0], t[1]);

                for(i = 0; i < 8; i++) {
                    XEVE_ITX32_E0_32B_NEON(eo[i], i);
                }
#undef XEVE_ITX32_E0_32B_NEON

#define XEVE_ITX32_EEO_32B_NEON(dst, idx) \
    XEVE_ITX_MADD(v[0], 4, idx)           \
    XEVE_ITX_MADD(v[1], 12, idx)          \
    dst = vaddq_s32(v[0], v[1]);

                for(i = 0; i < 4; i++) {
                    XEVE_ITX32_EEO_32B_NEON(eeo[i], i);
                }
#undef XEVE_ITX32_EEO_32B_NEON

                XEVE_ITX_MADD(eeeo[0], 8, 0);
                XEVE_ITX_MADD(eeeo[1], 8, 1);
                XEVE_ITX_MADD(eeee[0], 0, 0);
                XEVE_ITX_MADD(eeee[1], 0, 1);
#undef XEVE_ITX_MADD

                eee[0] = vaddq_s32(eeee[0], eeeo[0]);
                eee[1] = vaddq_s32(eeee[1], eeeo[1]);
                eee[2] = vsubq_s32(eeee[1], eeeo[1]);
                eee[3] = vsubq_s32(eeee[0], eeeo[0]);

                ee[0] = vaddq_s32(eee[0], eeo[0]);
                ee[1] = vaddq_s32(eee[1], eeo[1]);
                ee[2] = vaddq_s32(eee[2], eeo[2]);
                ee[3] = vaddq_s32(eee[3], eeo[3]);
                ee[4] = vsubq_s32(eee[3], eeo[3]);
                ee[5] = vsubq_s32(eee[2], eeo[2]);
                ee[6] = vsubq_s32(eee[1], eeo[1]);
                ee[7] = vsubq_s32(eee[0], eeo[0]);

                e[0]  = vaddq_s32(ee[0], eo[0]);
                e[1]  = vaddq_s32(ee[1], eo[1]);
                e[2]  = vaddq_s32(ee[2], eo[2]);
                e[3]  = vaddq_s32(ee[3], eo[3]);
                e[4]  = vaddq_s32(ee[4], eo[4]);
                e[5]  = vaddq_s32(ee[5], eo[5]);
                e[6]  = vaddq_s32(ee[6], eo[6]);
                e[7]  = vaddq_s32(ee[7], eo[7]);
                e[8]  = vsubq_s32(ee[7], eo[7]);
                e[9]  = vsubq_s32(ee[6], eo[6]);
                e[10] = vsubq_s32(ee[5], eo[5]);
                e[11] = vsubq_s32(ee[4], eo[4]);
                e[12] = vsubq_s32(ee[3], eo[3]);
                e[13] = vsubq_s32(ee[2], eo[2]);
                e[14] = vsubq_s32(ee[1], eo[1]);
                e[15] = vsubq_s32(ee[0], eo[0]);

                v[0]  = vaddq_s32(e[0], o[0]);
                v[1]  = vaddq_s32(e[1], o[1]);
                v[2]  = vaddq_s32(e[2], o[2]);
                v[3]  = vaddq_s32(e[3], o[3]);
                v[4]  = vaddq_s32(e[4], o[4]);
                v[5]  = vaddq_s32(e[5], o[5]);
                v[6]  = vaddq_s32(e[6], o[6]);
                v[7]  = vaddq_s32(e[7], o[7]);
                v[8]  = vaddq_s32(e[8], o[8]);
                v[9]  = vaddq_s32(e[9], o[9]);
                v[10] = vaddq_s32(e[10], o[10]);
                v[11] = vaddq_s32(e[11], o[11]);
                v[12] = vaddq_s32(e[12], o[12]);
                v[13] = vaddq_s32(e[13], o[13]);
                v[14] = vaddq_s32(e[14], o[14]);
                v[15] = vaddq_s32(e[15], o[15]);

                v[16] = vsubq_s32(e[15], o[15]);
                v[17] = vsubq_s32(e[14], o[14]);
                v[18] = vsubq_s32(e[13], o[13]);
                v[19] = vsubq_s32(e[12], o[12]);
                v[20] = vsubq_s32(e[11], o[11]);
                v[21] = vsubq_s32(e[10], o[10]);
                v[22] = vsubq_s32(e[9], o[9]);
                v[23] = vsubq_s32(e[8], o[8]);
                v[24] = vsubq_s32(e[7], o[7]);
                v[25] = vsubq_s32(e[6], o[6]);
                v[26] = vsubq_s32(e[5], o[5]);
                v[27] = vsubq_s32(e[4], o[4]);
                v[28] = vsubq_s32(e[3], o[3]);
                v[29] = vsubq_s32(e[2], o[2]);
                v[30] = vsubq_s32(e[1], o[1]);
                v[31] = vsubq_s32(e[0], o[0]);

                // CLIPPING
                XEVE_ITX_SHIFT_CLIP_NEON(v[0], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[1], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[2], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[3], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[4], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[5], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[6], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[7], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[8], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[9], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[10], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[11], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[12], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[13], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[14], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[15], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[16], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[17], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[18], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[19], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[20], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[21], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[22], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[23], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[24], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[25], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[26], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[27], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[28], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[29], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[30], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[31], add_s2, shift, min_val, max_val);

                // Pack to 16 bits
                t[0]  = vcombine_s16(vqmovn_s32(v[0]), vqmovn_s32(v[16]));
                t[1]  = vcombine_s16(vqmovn_s32(v[1]), vqmovn_s32(v[17]));
                t[2]  = vcombine_s16(vqmovn_s32(v[2]), vqmovn_s32(v[18]));
                t[3]  = vcombine_s16(vqmovn_s32(v[3]), vqmovn_s32(v[19]));
                t[4]  = vcombine_s16(vqmovn_s32(v[4]), vqmovn_s32(v[20]));
                t[5]  = vcombine_s16(vqmovn_s32(v[5]), vqmovn_s32(v[21]));
                t[6]  = vcombine_s16(vqmovn_s32(v[6]), vqmovn_s32(v[22]));
                t[7]  = vcombine_s16(vqmovn_s32(v[7]), vqmovn_s32(v[23]));
                t[8]  = vcombine_s16(vqmovn_s32(v[8]), vqmovn_s32(v[24]));
                t[9]  = vcombine_s16(vqmovn_s32(v[9]), vqmovn_s32(v[25]));
                t[10] = vcombine_s16(vqmovn_s32(v[10]), vqmovn_s32(v[26]));
                t[11] = vcombine_s16(vqmovn_s32(v[11]), vqmovn_s32(v[27]));
                t[12] = vcombine_s16(vqmovn_s32(v[12]), vqmovn_s32(v[28]));
                t[13] = vcombine_s16(vqmovn_s32(v[13]), vqmovn_s32(v[29]));
                t[14] = vcombine_s16(vqmovn_s32(v[14]), vqmovn_s32(v[30]));
                t[15] = vcombine_s16(vqmovn_s32(v[15]), vqmovn_s32(v[31]));

                v[0]  = vzip1q_s16(t[0], t[1]);
                v[1]  = vzip1q_s16(t[2], t[3]);
                v[2]  = vzip1q_s16(t[4], t[5]);
                v[3]  = vzip1q_s16(t[6], t[7]);
                v[4]  = vzip1q_s16(t[8], t[9]);
                v[5]  = vzip1q_s16(t[10], t[11]);
                v[6]  = vzip1q_s16(t[12], t[13]);
                v[7]  = vzip1q_s16(t[14], t[15]);
                v[8]  = vzip2q_s16(t[0], t[1]);
                v[9]  = vzip2q_s16(t[2], t[3]);
                v[10] = vzip2q_s16(t[4], t[5]);
                v[11] = vzip2q_s16(t[6], t[7]);
                v[12] = vzip2q_s16(t[8], t[9]);
                v[13] = vzip2q_s16(t[10], t[11]);
                v[14] = vzip2q_s16(t[12], t[13]);
                v[15] = vzip2q_s16(t[14], t[15]);

                t[0]  = vzip1q_s32(v[0], v[1]);
                t[1]  = vzip1q_s32(v[2], v[3]);
                t[2]  = vzip1q_s32(v[4], v[5]);
                t[3]  = vzip1q_s32(v[6], v[7]);
                t[4]  = vzip1q_s32(v[8], v[9]);
                t[5]  = vzip1q_s32(v[10], v[11]);
                t[6]  = vzip1q_s32(v[12], v[13]);
                t[7]  = vzip1q_s32(v[14], v[15]);
                t[8]  = vzip2q_s32(v[0], v[1]);
                t[9]  = vzip2q_s32(v[2], v[3]);
                t[10] = vzip2q_s32(v[4], v[5]);
                t[11] = vzip2q_s32(v[6], v[7]);
                t[12] = vzip2q_s32(v[8], v[9]);
                t[13] = vzip2q_s32(v[10], v[11]);
                t[14] = vzip2q_s32(v[12], v[13]);
                t[15] = vzip2q_s32(v[14], v[15]);

                v[0]  = vzip1q_s64(t[0], t[1]);
                v[1]  = vzip1q_s64(t[2], t[3]);
                v[2]  = vzip1q_s64(t[4], t[5]);
                v[3]  = vzip1q_s64(t[6], t[7]);
                v[4]  = vzip2q_s64(t[0], t[1]);
                v[5]  = vzip2q_s64(t[2], t[3]);
                v[6]  = vzip2q_s64(t[4], t[5]);
                v[7]  = vzip2q_s64(t[6], t[7]);
                v[8]  = vzip1q_s64(t[8], t[9]);
                v[9]  = vzip1q_s64(t[10], t[11]);
                v[10] = vzip1q_s64(t[12], t[13]);
                v[11] = vzip1q_s64(t[14], t[15]);
                v[12] = vzip2q_s64(t[8], t[9]);
                v[13] = vzip2q_s64(t[10], t[11]);
                v[14] = vzip2q_s64(t[12], t[13]);
                v[15] = vzip2q_s64(t[14], t[15]);

                // store
                for(i = 0; i < 16; i++) {
                    vst1q_s16((pel_dst), v[i]);
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

void xeve_itx_pb64b_neon(void* src, void* dst, int shift, int line, int step)
{
    int add = shift == 0 ? 0 : 1 << (shift - 1);
    if(step == 0) {
        if(line > 2) {
            s16* pel_src = src;
            s32* pel_dst = dst;

            int16x4_t   r[64];
            int16x4x2_t a[64];
            int32x4_t   o[32], e[32], eo[16], ee[16], eeo[8], eee[8], eeeo[4], eeee[4], eeeeo[2], eeeee[2];
            int32x4_t   v[64], t[16], d[64];

            int32x4_t max_val = vdupq_n_s32(MAX_TX_VAL_32);
            int32x4_t min_val = vdupq_n_s32(MIN_TX_VAL_32);

            int32x4_t coef[32][32];

            for(int i = 0; i < 32; i++) {
                for(int j = 0; j < 32; j++) {
                    coef[i][j] = vdupq_n_s32(((s32)(xeve_tbl_tm64[j + 32][i]) << 16) | (xeve_tbl_tm64[j][i] & 0xFFFF));
                }
            }

            int i, j, i_src[64];
            i_src[0] = 0;

            for(int i = 1; i < 64; i++) {
                i_src[i] = i_src[i - 1] + line;
            }

            for(j = 0; j < line; j += 4) {
                for(i = 0; i < 64; i++) {
                    r[i] = vld1_s16((pel_src + i_src[i] + j));
                }

                for(i = 0; i < 32; i++) {
                    a[i] = vzip_s16(r[i], r[i + 32]);
                }

#define XEVE_ITX64_O(dst, idx)               \
    t[0]  = vmadd_s16(a[1], coef[idx][1]);   \
    t[1]  = vmadd_s16(a[3], coef[idx][3]);   \
    t[2]  = vmadd_s16(a[5], coef[idx][5]);   \
    t[3]  = vmadd_s16(a[7], coef[idx][7]);   \
    t[4]  = vmadd_s16(a[9], coef[idx][9]);   \
    t[5]  = vmadd_s16(a[11], coef[idx][11]); \
    t[6]  = vmadd_s16(a[13], coef[idx][13]); \
    t[7]  = vmadd_s16(a[15], coef[idx][15]); \
    t[8]  = vmadd_s16(a[17], coef[idx][17]); \
    t[9]  = vmadd_s16(a[19], coef[idx][19]); \
    t[10] = vmadd_s16(a[21], coef[idx][21]); \
    t[11] = vmadd_s16(a[23], coef[idx][23]); \
    t[12] = vmadd_s16(a[25], coef[idx][25]); \
    t[13] = vmadd_s16(a[27], coef[idx][27]); \
    t[14] = vmadd_s16(a[29], coef[idx][29]); \
    t[15] = vmadd_s16(a[31], coef[idx][31]); \
    d[0]  = vaddq_s32(t[0], t[1]);           \
    d[1]  = vaddq_s32(t[2], t[3]);           \
    d[2]  = vaddq_s32(t[4], t[5]);           \
    d[3]  = vaddq_s32(t[6], t[7]);           \
    d[4]  = vaddq_s32(t[8], t[9]);           \
    d[5]  = vaddq_s32(t[10], t[11]);         \
    d[6]  = vaddq_s32(t[12], t[13]);         \
    d[7]  = vaddq_s32(t[14], t[15]);         \
    t[0]  = vaddq_s32(d[0], d[1]);           \
    t[1]  = vaddq_s32(d[2], d[3]);           \
    t[2]  = vaddq_s32(d[4], d[5]);           \
    t[3]  = vaddq_s32(d[6], d[7]);           \
    d[0]  = vaddq_s32(t[0], t[1]);           \
    d[1]  = vaddq_s32(t[2], t[3]);           \
    dst   = vaddq_s32(d[0], d[1]);

                for(int i = 0; i < 32; i++) {
                    XEVE_ITX64_O(o[i], i);
                }
#undef XEVE_ITX64_O

#define XEVE_ITX64_EO(dst, idx)             \
    t[0] = vmadd_s16(a[2], coef[idx][2]);   \
    t[1] = vmadd_s16(a[6], coef[idx][6]);   \
    t[2] = vmadd_s16(a[10], coef[idx][10]); \
    t[3] = vmadd_s16(a[14], coef[idx][14]); \
    t[4] = vmadd_s16(a[18], coef[idx][18]); \
    t[5] = vmadd_s16(a[22], coef[idx][22]); \
    t[6] = vmadd_s16(a[26], coef[idx][26]); \
    t[7] = vmadd_s16(a[30], coef[idx][30]); \
    d[0] = vaddq_s32(t[0], t[1]);           \
    d[1] = vaddq_s32(t[2], t[3]);           \
    d[2] = vaddq_s32(t[4], t[5]);           \
    d[3] = vaddq_s32(t[6], t[7]);           \
    t[0] = vaddq_s32(d[0], d[1]);           \
    t[1] = vaddq_s32(d[2], d[3]);           \
    dst  = vaddq_s32(t[0], t[1]);

                for(int i = 0; i < 16; i++) {
                    XEVE_ITX64_EO(eo[i], i);
                }
#undef XEVE_ITX64_EO

#define XEVE_ITX64_EEO(dst, idx)            \
    t[0] = vmadd_s16(a[4], coef[idx][4]);   \
    t[1] = vmadd_s16(a[12], coef[idx][12]); \
    t[2] = vmadd_s16(a[20], coef[idx][20]); \
    t[3] = vmadd_s16(a[28], coef[idx][28]); \
    d[0] = vaddq_s32(t[0], t[1]);           \
    d[1] = vaddq_s32(t[2], t[3]);           \
    dst  = vaddq_s32(d[0], d[1]);

                for(int i = 0; i < 8; i++) {
                    XEVE_ITX64_EEO(eeo[i], i);
                }
#undef XEVE_ITX64_EEO

#define XEVE_ITX64_EEEO(dst, idx)           \
    t[0] = vmadd_s16(a[8], coef[idx][8]);   \
    t[1] = vmadd_s16(a[24], coef[idx][24]); \
    dst  = vaddq_s32(t[0], t[1]);

                for(int i = 0; i < 4; i++) {
                    XEVE_ITX64_EEEO(eeeo[i], i);
                }
#undef XEVE_ITX64_EEEO

                eeeeo[0] = vmadd_s16(a[16], coef[0][16]);
                eeeeo[1] = vmadd_s16(a[16], coef[1][16]);
                eeeee[0] = vmadd_s16(a[0], coef[0][0]);
                eeeee[1] = vmadd_s16(a[0], coef[1][0]);

                eeee[0] = vaddq_s32(eeeee[0], eeeeo[0]);
                eeee[1] = vaddq_s32(eeeee[1], eeeeo[1]);
                eeee[2] = vsubq_s32(eeeee[1], eeeeo[1]);
                eeee[3] = vsubq_s32(eeeee[0], eeeeo[0]);

                eee[0] = vaddq_s32(eeee[0], eeeo[0]);
                eee[1] = vaddq_s32(eeee[1], eeeo[1]);
                eee[2] = vaddq_s32(eeee[2], eeeo[2]);
                eee[3] = vaddq_s32(eeee[3], eeeo[3]);
                eee[4] = vsubq_s32(eeee[3], eeeo[3]);
                eee[5] = vsubq_s32(eeee[2], eeeo[2]);
                eee[6] = vsubq_s32(eeee[1], eeeo[1]);
                eee[7] = vsubq_s32(eeee[0], eeeo[0]);

                ee[0]  = vaddq_s32(eee[0], eeo[0]);
                ee[1]  = vaddq_s32(eee[1], eeo[1]);
                ee[2]  = vaddq_s32(eee[2], eeo[2]);
                ee[3]  = vaddq_s32(eee[3], eeo[3]);
                ee[4]  = vaddq_s32(eee[4], eeo[4]);
                ee[5]  = vaddq_s32(eee[5], eeo[5]);
                ee[6]  = vaddq_s32(eee[6], eeo[6]);
                ee[7]  = vaddq_s32(eee[7], eeo[7]);
                ee[8]  = vsubq_s32(eee[7], eeo[7]);
                ee[9]  = vsubq_s32(eee[6], eeo[6]);
                ee[10] = vsubq_s32(eee[5], eeo[5]);
                ee[11] = vsubq_s32(eee[4], eeo[4]);
                ee[12] = vsubq_s32(eee[3], eeo[3]);
                ee[13] = vsubq_s32(eee[2], eeo[2]);
                ee[14] = vsubq_s32(eee[1], eeo[1]);
                ee[15] = vsubq_s32(eee[0], eeo[0]);

                e[0]  = vaddq_s32(ee[0], eo[0]);
                e[1]  = vaddq_s32(ee[1], eo[1]);
                e[2]  = vaddq_s32(ee[2], eo[2]);
                e[3]  = vaddq_s32(ee[3], eo[3]);
                e[4]  = vaddq_s32(ee[4], eo[4]);
                e[5]  = vaddq_s32(ee[5], eo[5]);
                e[6]  = vaddq_s32(ee[6], eo[6]);
                e[7]  = vaddq_s32(ee[7], eo[7]);
                e[8]  = vaddq_s32(ee[8], eo[8]);
                e[9]  = vaddq_s32(ee[9], eo[9]);
                e[10] = vaddq_s32(ee[10], eo[10]);
                e[11] = vaddq_s32(ee[11], eo[11]);
                e[12] = vaddq_s32(ee[12], eo[12]);
                e[13] = vaddq_s32(ee[13], eo[13]);
                e[14] = vaddq_s32(ee[14], eo[14]);
                e[15] = vaddq_s32(ee[15], eo[15]);

                e[16] = vsubq_s32(ee[15], eo[15]);
                e[17] = vsubq_s32(ee[14], eo[14]);
                e[18] = vsubq_s32(ee[13], eo[13]);
                e[19] = vsubq_s32(ee[12], eo[12]);
                e[20] = vsubq_s32(ee[11], eo[11]);
                e[21] = vsubq_s32(ee[10], eo[10]);
                e[22] = vsubq_s32(ee[9], eo[9]);
                e[23] = vsubq_s32(ee[8], eo[8]);
                e[24] = vsubq_s32(ee[7], eo[7]);
                e[25] = vsubq_s32(ee[6], eo[6]);
                e[26] = vsubq_s32(ee[5], eo[5]);
                e[27] = vsubq_s32(ee[4], eo[4]);
                e[28] = vsubq_s32(ee[3], eo[3]);
                e[29] = vsubq_s32(ee[2], eo[2]);
                e[30] = vsubq_s32(ee[1], eo[1]);
                e[31] = vsubq_s32(ee[0], eo[0]);

                v[0]  = vaddq_s32(e[0], o[0]);
                v[1]  = vaddq_s32(e[1], o[1]);
                v[2]  = vaddq_s32(e[2], o[2]);
                v[3]  = vaddq_s32(e[3], o[3]);
                v[4]  = vaddq_s32(e[4], o[4]);
                v[5]  = vaddq_s32(e[5], o[5]);
                v[6]  = vaddq_s32(e[6], o[6]);
                v[7]  = vaddq_s32(e[7], o[7]);
                v[8]  = vaddq_s32(e[8], o[8]);
                v[9]  = vaddq_s32(e[9], o[9]);
                v[10] = vaddq_s32(e[10], o[10]);
                v[11] = vaddq_s32(e[11], o[11]);
                v[12] = vaddq_s32(e[12], o[12]);
                v[13] = vaddq_s32(e[13], o[13]);
                v[14] = vaddq_s32(e[14], o[14]);
                v[15] = vaddq_s32(e[15], o[15]);
                v[16] = vaddq_s32(e[16], o[16]);
                v[17] = vaddq_s32(e[17], o[17]);
                v[18] = vaddq_s32(e[18], o[18]);
                v[19] = vaddq_s32(e[19], o[19]);
                v[20] = vaddq_s32(e[20], o[20]);
                v[21] = vaddq_s32(e[21], o[21]);
                v[22] = vaddq_s32(e[22], o[22]);
                v[23] = vaddq_s32(e[23], o[23]);
                v[24] = vaddq_s32(e[24], o[24]);
                v[25] = vaddq_s32(e[25], o[25]);
                v[26] = vaddq_s32(e[26], o[26]);
                v[27] = vaddq_s32(e[27], o[27]);
                v[28] = vaddq_s32(e[28], o[28]);
                v[29] = vaddq_s32(e[29], o[29]);
                v[30] = vaddq_s32(e[30], o[30]);
                v[31] = vaddq_s32(e[31], o[31]);

                v[32] = vsubq_s32(e[31], o[31]);
                v[33] = vsubq_s32(e[30], o[30]);
                v[34] = vsubq_s32(e[29], o[29]);
                v[35] = vsubq_s32(e[28], o[28]);
                v[36] = vsubq_s32(e[27], o[27]);
                v[37] = vsubq_s32(e[26], o[26]);
                v[38] = vsubq_s32(e[25], o[25]);
                v[39] = vsubq_s32(e[24], o[24]);
                v[40] = vsubq_s32(e[23], o[23]);
                v[41] = vsubq_s32(e[22], o[22]);
                v[42] = vsubq_s32(e[21], o[21]);
                v[43] = vsubq_s32(e[20], o[20]);
                v[44] = vsubq_s32(e[19], o[19]);
                v[45] = vsubq_s32(e[18], o[18]);
                v[46] = vsubq_s32(e[17], o[17]);
                v[47] = vsubq_s32(e[16], o[16]);
                v[48] = vsubq_s32(e[15], o[15]);
                v[49] = vsubq_s32(e[14], o[14]);
                v[50] = vsubq_s32(e[13], o[13]);
                v[51] = vsubq_s32(e[12], o[12]);
                v[52] = vsubq_s32(e[11], o[11]);
                v[53] = vsubq_s32(e[10], o[10]);
                v[54] = vsubq_s32(e[9], o[9]);
                v[55] = vsubq_s32(e[8], o[8]);
                v[56] = vsubq_s32(e[7], o[7]);
                v[57] = vsubq_s32(e[6], o[6]);
                v[58] = vsubq_s32(e[5], o[5]);
                v[59] = vsubq_s32(e[4], o[4]);
                v[60] = vsubq_s32(e[3], o[3]);
                v[61] = vsubq_s32(e[2], o[2]);
                v[62] = vsubq_s32(e[1], o[1]);
                v[63] = vsubq_s32(e[0], o[0]);

                for(int i = 0; i < 64; i++) {
                    XEVE_ITX_CLIP_NEON(v[i], min_val, max_val);
                }

#define XEVE_ITDQ_TRANSPOS_NEON(s0, s1, s2, s3, t0, t1, t2, t3) \
    t0 = vzip1q_s32(s0, s1);                                    \
    t2 = vzip2q_s32(s0, s1);                                    \
    t1 = vzip1q_s32(s2, s3);                                    \
    t3 = vzip2q_s32(s2, s3);                                    \
                                                                \
    s0 = vzip1q_s64(t0, t1);                                    \
    s1 = vzip2q_s64(t0, t1);                                    \
    s2 = vzip1q_s64(t2, t3);                                    \
    s3 = vzip2q_s64(t2, t3);

                XEVE_ITDQ_TRANSPOS_NEON(v[0], v[1], v[2], v[3], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[4], v[5], v[6], v[7], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[8], v[9], v[10], v[11], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[12], v[13], v[14], v[15], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[16], v[17], v[18], v[19], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[20], v[21], v[22], v[23], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[24], v[25], v[26], v[27], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[28], v[29], v[30], v[31], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[32], v[33], v[34], v[35], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[36], v[37], v[38], v[39], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[40], v[41], v[42], v[43], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[44], v[45], v[46], v[47], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[48], v[49], v[50], v[51], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[52], v[53], v[54], v[55], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[56], v[57], v[58], v[59], t[0], t[1], t[2], t[3]);
                XEVE_ITDQ_TRANSPOS_NEON(v[60], v[61], v[62], v[63], t[0], t[1], t[2], t[3]);
#undef XEVE_ITDQ_TRANSPOS_NEON

                // Store
                vst1q_s32((pel_dst), v[0]);
                vst1q_s32((pel_dst + 4), v[4]);
                vst1q_s32((pel_dst + 8), v[8]);
                vst1q_s32((pel_dst + 12), v[12]);
                vst1q_s32((pel_dst + 16), v[16]);
                vst1q_s32((pel_dst + 20), v[20]);
                vst1q_s32((pel_dst + 24), v[24]);
                vst1q_s32((pel_dst + 28), v[28]);
                vst1q_s32((pel_dst + 32), v[32]);
                vst1q_s32((pel_dst + 36), v[36]);
                vst1q_s32((pel_dst + 40), v[40]);
                vst1q_s32((pel_dst + 44), v[44]);
                vst1q_s32((pel_dst + 48), v[48]);
                vst1q_s32((pel_dst + 52), v[52]);
                vst1q_s32((pel_dst + 56), v[56]);
                vst1q_s32((pel_dst + 60), v[60]);
                vst1q_s32((pel_dst + 64), v[1]);
                vst1q_s32((pel_dst + 68), v[5]);
                vst1q_s32((pel_dst + 72), v[9]);
                vst1q_s32((pel_dst + 76), v[13]);
                vst1q_s32((pel_dst + 80), v[17]);
                vst1q_s32((pel_dst + 84), v[21]);
                vst1q_s32((pel_dst + 88), v[25]);
                vst1q_s32((pel_dst + 92), v[29]);
                vst1q_s32((pel_dst + 96), v[33]);
                vst1q_s32((pel_dst + 100), v[37]);
                vst1q_s32((pel_dst + 104), v[41]);
                vst1q_s32((pel_dst + 108), v[45]);
                vst1q_s32((pel_dst + 112), v[49]);
                vst1q_s32((pel_dst + 116), v[53]);
                vst1q_s32((pel_dst + 120), v[57]);
                vst1q_s32((pel_dst + 124), v[61]);
                vst1q_s32((pel_dst + 128), v[2]);
                vst1q_s32((pel_dst + 132), v[6]);
                vst1q_s32((pel_dst + 136), v[10]);
                vst1q_s32((pel_dst + 140), v[14]);
                vst1q_s32((pel_dst + 144), v[18]);
                vst1q_s32((pel_dst + 148), v[22]);
                vst1q_s32((pel_dst + 152), v[26]);
                vst1q_s32((pel_dst + 156), v[30]);
                vst1q_s32((pel_dst + 160), v[34]);
                vst1q_s32((pel_dst + 164), v[38]);
                vst1q_s32((pel_dst + 168), v[42]);
                vst1q_s32((pel_dst + 172), v[46]);
                vst1q_s32((pel_dst + 176), v[50]);
                vst1q_s32((pel_dst + 180), v[54]);
                vst1q_s32((pel_dst + 184), v[58]);
                vst1q_s32((pel_dst + 188), v[62]);
                vst1q_s32((pel_dst + 192), v[3]);
                vst1q_s32((pel_dst + 196), v[7]);
                vst1q_s32((pel_dst + 200), v[11]);
                vst1q_s32((pel_dst + 204), v[15]);
                vst1q_s32((pel_dst + 208), v[19]);
                vst1q_s32((pel_dst + 212), v[23]);
                vst1q_s32((pel_dst + 216), v[27]);
                vst1q_s32((pel_dst + 220), v[31]);
                vst1q_s32((pel_dst + 224), v[35]);
                vst1q_s32((pel_dst + 228), v[39]);
                vst1q_s32((pel_dst + 232), v[43]);
                vst1q_s32((pel_dst + 236), v[47]);
                vst1q_s32((pel_dst + 240), v[51]);
                vst1q_s32((pel_dst + 244), v[55]);
                vst1q_s32((pel_dst + 248), v[59]);
                vst1q_s32((pel_dst + 252), v[63]);

                pel_dst += 256;
            }
        }
        else {
            xeve_itx_pb64b(src, dst, shift, line, step);
        }
    }
    else {
        if(line > 2) {
            s32*      pel_src = src;
            s16*      pel_dst = dst;
            /*
                Variables are named in a way
                similar to SSE code for ITX
                for better understanding
            */
            int32x4_t r[64], a[32], b[32], e[32], o[32];
            int32x4_t eo[16], ee[16], eeo[8], eee[8], eeeo[4], eeee[4], eeeeo[2], eeeee[2];
            int32x4_t v[64];
            int32x4_t t[32];

            const int32x4_t max_val = vdupq_n_s32(MAX_TX_VAL_32);
            const int32x4_t min_val = vdupq_n_s32(MIN_TX_VAL_32);
            const int32x4_t add_s2  = vdupq_n_s32(add);
            int32x4_t       coef[32][32];

            int i, j, i_src[64];

            for(i = 0; i < 32; i++) {
                for(j = 0; j < 32; j++) {
                    coef[i][j] =
                        vdupq_n_s64(((s64)(xeve_tbl_tm64[j + 32][i]) << 32) | (xeve_tbl_tm64[j][i] & 0xFFFFFFFF));
                }
            }

            i_src[0] = 0;

            for(i = 1; i < 64; i++) {
                i_src[i] = i_src[i - 1] + line;
            }

            for(j = 0; j < line; j += 4) {
                for(i = 0; i < 64; i++) {
                    r[i] = vld1q_s32((pel_src + i_src[i] + j));
                }

                for(i = 0; i < 32; i++) {
                    a[i] = vzip1q_s32(r[i], r[i + 32]);
                    b[i] = vzip2q_s32(r[i], r[i + 32]);
                }

#define XEVE_ITX_MADD(dst, r_idx, c_idx) XEVE_MADD_S32(t[0], t[1], dst, a[r_idx], b[r_idx], coef[c_idx][r_idx]);

#define XEVE_ITX64_0_32B(dst, idx)  \
    XEVE_ITX_MADD(v[0], 1, idx)     \
    XEVE_ITX_MADD(v[1], 3, idx)     \
    XEVE_ITX_MADD(v[2], 5, idx)     \
    XEVE_ITX_MADD(v[3], 7, idx)     \
    XEVE_ITX_MADD(v[4], 9, idx)     \
    XEVE_ITX_MADD(v[5], 11, idx)    \
    XEVE_ITX_MADD(v[6], 13, idx)    \
    XEVE_ITX_MADD(v[7], 15, idx)    \
    XEVE_ITX_MADD(v[8], 17, idx)    \
    XEVE_ITX_MADD(v[9], 19, idx)    \
    XEVE_ITX_MADD(v[10], 21, idx)   \
    XEVE_ITX_MADD(v[11], 23, idx)   \
    XEVE_ITX_MADD(v[12], 25, idx)   \
    XEVE_ITX_MADD(v[13], 27, idx)   \
    XEVE_ITX_MADD(v[14], 29, idx)   \
    XEVE_ITX_MADD(v[15], 31, idx)   \
    t[0] = vaddq_s32(v[0], v[1]);   \
    t[1] = vaddq_s32(v[2], v[3]);   \
    t[2] = vaddq_s32(v[4], v[5]);   \
    t[3] = vaddq_s32(v[6], v[7]);   \
    t[4] = vaddq_s32(v[8], v[9]);   \
    t[5] = vaddq_s32(v[10], v[11]); \
    t[6] = vaddq_s32(v[12], v[13]); \
    t[7] = vaddq_s32(v[14], v[15]); \
                                    \
    v[0] = vaddq_s32(t[0], t[1]);   \
    v[1] = vaddq_s32(t[2], t[3]);   \
    v[2] = vaddq_s32(t[4], t[5]);   \
    v[3] = vaddq_s32(t[6], t[7]);   \
                                    \
    t[0] = vaddq_s32(v[0], v[1]);   \
    t[1] = vaddq_s32(v[2], v[3]);   \
                                    \
    dst = vaddq_s32(t[0], t[1]);

                for(i = 0; i < 32; i++) {
                    XEVE_ITX64_0_32B(o[i], i);
                }
#undef XEVE_ITX64_0_32B

#define XEVE_ITX64_E0_32B(dst, idx) \
    XEVE_ITX_MADD(v[0], 2, idx)     \
    XEVE_ITX_MADD(v[1], 6, idx)     \
    XEVE_ITX_MADD(v[2], 10, idx)    \
    XEVE_ITX_MADD(v[3], 14, idx)    \
    XEVE_ITX_MADD(v[4], 18, idx)    \
    XEVE_ITX_MADD(v[5], 22, idx)    \
    XEVE_ITX_MADD(v[6], 26, idx)    \
    XEVE_ITX_MADD(v[7], 30, idx)    \
    t[0] = vaddq_s32(v[0], v[1]);   \
    t[1] = vaddq_s32(v[2], v[3]);   \
    t[2] = vaddq_s32(v[4], v[5]);   \
    t[3] = vaddq_s32(v[6], v[7]);   \
                                    \
    v[0] = vaddq_s32(t[0], t[1]);   \
    v[1] = vaddq_s32(t[2], t[3]);   \
                                    \
    dst = vaddq_s32(v[0], v[1]);

                for(i = 0; i < 16; i++) {
                    XEVE_ITX64_E0_32B(eo[i], i);
                }
#undef XEVE_ITX64_E0_32B

#define XEVE_ITX64_EE0_32B(dst, idx) \
    XEVE_ITX_MADD(v[0], 4, idx)      \
    XEVE_ITX_MADD(v[1], 12, idx)     \
    XEVE_ITX_MADD(v[2], 20, idx)     \
    XEVE_ITX_MADD(v[3], 28, idx)     \
    t[0] = vaddq_s32(v[0], v[1]);    \
    t[1] = vaddq_s32(v[2], v[3]);    \
    dst  = vaddq_s32(t[0], t[1]);

                for(i = 0; i < 8; i++) {
                    XEVE_ITX64_EE0_32B(eeo[i], i);
                }
#undef XEVE_ITX64_EE0_32B

#define XEVE_ITX64_EEEO_32B(dst, idx) \
    XEVE_ITX_MADD(v[0], 8, idx)       \
    XEVE_ITX_MADD(v[1], 24, idx)      \
    dst = vaddq_s32(v[0], v[1]);

                for(i = 0; i < 4; i++) {
                    XEVE_ITX64_EEEO_32B(eeeo[i], i);
                }
#undef XEVE_ITX64_EEEO_32B

                XEVE_ITX_MADD(eeeeo[0], 16, 0);
                XEVE_ITX_MADD(eeeeo[1], 16, 1);
                XEVE_ITX_MADD(eeeee[0], 0, 0);
                XEVE_ITX_MADD(eeeee[1], 0, 1);

                eeee[0] = vaddq_s32(eeeee[0], eeeeo[0]);
                eeee[1] = vaddq_s32(eeeee[1], eeeeo[1]);
                eeee[2] = vsubq_s32(eeeee[1], eeeeo[1]);
                eeee[3] = vsubq_s32(eeeee[0], eeeeo[0]);

                eee[0] = vaddq_s32(eeee[0], eeeo[0]);
                eee[1] = vaddq_s32(eeee[1], eeeo[1]);
                eee[2] = vaddq_s32(eeee[2], eeeo[2]);
                eee[3] = vaddq_s32(eeee[3], eeeo[3]);
                eee[4] = vsubq_s32(eeee[3], eeeo[3]);
                eee[5] = vsubq_s32(eeee[2], eeeo[2]);
                eee[6] = vsubq_s32(eeee[1], eeeo[1]);
                eee[7] = vsubq_s32(eeee[0], eeeo[0]);

                ee[0]  = vaddq_s32(eee[0], eeo[0]);
                ee[1]  = vaddq_s32(eee[1], eeo[1]);
                ee[2]  = vaddq_s32(eee[2], eeo[2]);
                ee[3]  = vaddq_s32(eee[3], eeo[3]);
                ee[4]  = vaddq_s32(eee[4], eeo[4]);
                ee[5]  = vaddq_s32(eee[5], eeo[5]);
                ee[6]  = vaddq_s32(eee[6], eeo[6]);
                ee[7]  = vaddq_s32(eee[7], eeo[7]);
                ee[8]  = vsubq_s32(eee[7], eeo[7]);
                ee[9]  = vsubq_s32(eee[6], eeo[6]);
                ee[10] = vsubq_s32(eee[5], eeo[5]);
                ee[11] = vsubq_s32(eee[4], eeo[4]);
                ee[12] = vsubq_s32(eee[3], eeo[3]);
                ee[13] = vsubq_s32(eee[2], eeo[2]);
                ee[14] = vsubq_s32(eee[1], eeo[1]);
                ee[15] = vsubq_s32(eee[0], eeo[0]);

                e[0]  = vaddq_s32(ee[0], eo[0]);
                e[1]  = vaddq_s32(ee[1], eo[1]);
                e[2]  = vaddq_s32(ee[2], eo[2]);
                e[3]  = vaddq_s32(ee[3], eo[3]);
                e[4]  = vaddq_s32(ee[4], eo[4]);
                e[5]  = vaddq_s32(ee[5], eo[5]);
                e[6]  = vaddq_s32(ee[6], eo[6]);
                e[7]  = vaddq_s32(ee[7], eo[7]);
                e[8]  = vaddq_s32(ee[8], eo[8]);
                e[9]  = vaddq_s32(ee[9], eo[9]);
                e[10] = vaddq_s32(ee[10], eo[10]);
                e[11] = vaddq_s32(ee[11], eo[11]);
                e[12] = vaddq_s32(ee[12], eo[12]);
                e[13] = vaddq_s32(ee[13], eo[13]);
                e[14] = vaddq_s32(ee[14], eo[14]);
                e[15] = vaddq_s32(ee[15], eo[15]);

                e[16] = vsubq_s32(ee[15], eo[15]);
                e[17] = vsubq_s32(ee[14], eo[14]);
                e[18] = vsubq_s32(ee[13], eo[13]);
                e[19] = vsubq_s32(ee[12], eo[12]);
                e[20] = vsubq_s32(ee[11], eo[11]);
                e[21] = vsubq_s32(ee[10], eo[10]);
                e[22] = vsubq_s32(ee[9], eo[9]);
                e[23] = vsubq_s32(ee[8], eo[8]);
                e[24] = vsubq_s32(ee[7], eo[7]);
                e[25] = vsubq_s32(ee[6], eo[6]);
                e[26] = vsubq_s32(ee[5], eo[5]);
                e[27] = vsubq_s32(ee[4], eo[4]);
                e[28] = vsubq_s32(ee[3], eo[3]);
                e[29] = vsubq_s32(ee[2], eo[2]);
                e[30] = vsubq_s32(ee[1], eo[1]);
                e[31] = vsubq_s32(ee[0], eo[0]);

                v[0]  = vaddq_s32(e[0], o[0]);
                v[1]  = vaddq_s32(e[1], o[1]);
                v[2]  = vaddq_s32(e[2], o[2]);
                v[3]  = vaddq_s32(e[3], o[3]);
                v[4]  = vaddq_s32(e[4], o[4]);
                v[5]  = vaddq_s32(e[5], o[5]);
                v[6]  = vaddq_s32(e[6], o[6]);
                v[7]  = vaddq_s32(e[7], o[7]);
                v[8]  = vaddq_s32(e[8], o[8]);
                v[9]  = vaddq_s32(e[9], o[9]);
                v[10] = vaddq_s32(e[10], o[10]);
                v[11] = vaddq_s32(e[11], o[11]);
                v[12] = vaddq_s32(e[12], o[12]);
                v[13] = vaddq_s32(e[13], o[13]);
                v[14] = vaddq_s32(e[14], o[14]);
                v[15] = vaddq_s32(e[15], o[15]);
                v[16] = vaddq_s32(e[16], o[16]);
                v[17] = vaddq_s32(e[17], o[17]);
                v[18] = vaddq_s32(e[18], o[18]);
                v[19] = vaddq_s32(e[19], o[19]);
                v[20] = vaddq_s32(e[20], o[20]);
                v[21] = vaddq_s32(e[21], o[21]);
                v[22] = vaddq_s32(e[22], o[22]);
                v[23] = vaddq_s32(e[23], o[23]);
                v[24] = vaddq_s32(e[24], o[24]);
                v[25] = vaddq_s32(e[25], o[25]);
                v[26] = vaddq_s32(e[26], o[26]);
                v[27] = vaddq_s32(e[27], o[27]);
                v[28] = vaddq_s32(e[28], o[28]);
                v[29] = vaddq_s32(e[29], o[29]);
                v[30] = vaddq_s32(e[30], o[30]);
                v[31] = vaddq_s32(e[31], o[31]);

                v[32] = vsubq_s32(e[31], o[31]);
                v[33] = vsubq_s32(e[30], o[30]);
                v[34] = vsubq_s32(e[29], o[29]);
                v[35] = vsubq_s32(e[28], o[28]);
                v[36] = vsubq_s32(e[27], o[27]);
                v[37] = vsubq_s32(e[26], o[26]);
                v[38] = vsubq_s32(e[25], o[25]);
                v[39] = vsubq_s32(e[24], o[24]);
                v[40] = vsubq_s32(e[23], o[23]);
                v[41] = vsubq_s32(e[22], o[22]);
                v[42] = vsubq_s32(e[21], o[21]);
                v[43] = vsubq_s32(e[20], o[20]);
                v[44] = vsubq_s32(e[19], o[19]);
                v[45] = vsubq_s32(e[18], o[18]);
                v[46] = vsubq_s32(e[17], o[17]);
                v[47] = vsubq_s32(e[16], o[16]);
                v[48] = vsubq_s32(e[15], o[15]);
                v[49] = vsubq_s32(e[14], o[14]);
                v[50] = vsubq_s32(e[13], o[13]);
                v[51] = vsubq_s32(e[12], o[12]);
                v[52] = vsubq_s32(e[11], o[11]);
                v[53] = vsubq_s32(e[10], o[10]);
                v[54] = vsubq_s32(e[9], o[9]);
                v[55] = vsubq_s32(e[8], o[8]);
                v[56] = vsubq_s32(e[7], o[7]);
                v[57] = vsubq_s32(e[6], o[6]);
                v[58] = vsubq_s32(e[5], o[5]);
                v[59] = vsubq_s32(e[4], o[4]);
                v[60] = vsubq_s32(e[3], o[3]);
                v[61] = vsubq_s32(e[2], o[2]);
                v[62] = vsubq_s32(e[1], o[1]);
                v[63] = vsubq_s32(e[0], o[0]);

                // CLIPPING
                XEVE_ITX_SHIFT_CLIP_NEON(v[0], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[1], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[2], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[3], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[4], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[5], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[6], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[7], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[8], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[9], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[10], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[11], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[12], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[13], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[14], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[15], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[16], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[17], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[18], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[19], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[20], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[21], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[22], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[23], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[24], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[25], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[26], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[27], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[28], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[29], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[30], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[31], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[32], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[33], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[34], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[35], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[36], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[37], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[38], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[39], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[40], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[41], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[42], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[43], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[44], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[45], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[46], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[47], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[48], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[49], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[50], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[51], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[52], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[53], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[54], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[55], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[56], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[57], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[58], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[59], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[60], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[61], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[62], add_s2, shift, min_val, max_val);
                XEVE_ITX_SHIFT_CLIP_NEON(v[63], add_s2, shift, min_val, max_val);

                // Pack to 16 bits
                t[0]  = vcombine_s16(vqmovn_s32(v[0]), vqmovn_s32(v[32]));
                t[1]  = vcombine_s16(vqmovn_s32(v[1]), vqmovn_s32(v[33]));
                t[2]  = vcombine_s16(vqmovn_s32(v[2]), vqmovn_s32(v[34]));
                t[3]  = vcombine_s16(vqmovn_s32(v[3]), vqmovn_s32(v[35]));
                t[4]  = vcombine_s16(vqmovn_s32(v[4]), vqmovn_s32(v[36]));
                t[5]  = vcombine_s16(vqmovn_s32(v[5]), vqmovn_s32(v[37]));
                t[6]  = vcombine_s16(vqmovn_s32(v[6]), vqmovn_s32(v[38]));
                t[7]  = vcombine_s16(vqmovn_s32(v[7]), vqmovn_s32(v[39]));
                t[8]  = vcombine_s16(vqmovn_s32(v[8]), vqmovn_s32(v[40]));
                t[9]  = vcombine_s16(vqmovn_s32(v[9]), vqmovn_s32(v[41]));
                t[10] = vcombine_s16(vqmovn_s32(v[10]), vqmovn_s32(v[42]));
                t[11] = vcombine_s16(vqmovn_s32(v[11]), vqmovn_s32(v[43]));
                t[12] = vcombine_s16(vqmovn_s32(v[12]), vqmovn_s32(v[44]));
                t[13] = vcombine_s16(vqmovn_s32(v[13]), vqmovn_s32(v[45]));
                t[14] = vcombine_s16(vqmovn_s32(v[14]), vqmovn_s32(v[46]));
                t[15] = vcombine_s16(vqmovn_s32(v[15]), vqmovn_s32(v[47]));
                t[16] = vcombine_s16(vqmovn_s32(v[16]), vqmovn_s32(v[48]));
                t[17] = vcombine_s16(vqmovn_s32(v[17]), vqmovn_s32(v[49]));
                t[18] = vcombine_s16(vqmovn_s32(v[18]), vqmovn_s32(v[50]));
                t[19] = vcombine_s16(vqmovn_s32(v[19]), vqmovn_s32(v[51]));
                t[20] = vcombine_s16(vqmovn_s32(v[20]), vqmovn_s32(v[52]));
                t[21] = vcombine_s16(vqmovn_s32(v[21]), vqmovn_s32(v[53]));
                t[22] = vcombine_s16(vqmovn_s32(v[22]), vqmovn_s32(v[54]));
                t[23] = vcombine_s16(vqmovn_s32(v[23]), vqmovn_s32(v[55]));
                t[24] = vcombine_s16(vqmovn_s32(v[24]), vqmovn_s32(v[56]));
                t[25] = vcombine_s16(vqmovn_s32(v[25]), vqmovn_s32(v[57]));
                t[26] = vcombine_s16(vqmovn_s32(v[26]), vqmovn_s32(v[58]));
                t[27] = vcombine_s16(vqmovn_s32(v[27]), vqmovn_s32(v[59]));
                t[28] = vcombine_s16(vqmovn_s32(v[28]), vqmovn_s32(v[60]));
                t[29] = vcombine_s16(vqmovn_s32(v[29]), vqmovn_s32(v[61]));
                t[30] = vcombine_s16(vqmovn_s32(v[30]), vqmovn_s32(v[62]));
                t[31] = vcombine_s16(vqmovn_s32(v[31]), vqmovn_s32(v[63]));

                v[0]  = vzip1q_s16(t[0], t[1]);
                v[1]  = vzip1q_s16(t[2], t[3]);
                v[2]  = vzip1q_s16(t[4], t[5]);
                v[3]  = vzip1q_s16(t[6], t[7]);
                v[4]  = vzip1q_s16(t[8], t[9]);
                v[5]  = vzip1q_s16(t[10], t[11]);
                v[6]  = vzip1q_s16(t[12], t[13]);
                v[7]  = vzip1q_s16(t[14], t[15]);
                v[8]  = vzip1q_s16(t[16], t[17]);
                v[9]  = vzip1q_s16(t[18], t[19]);
                v[10] = vzip1q_s16(t[20], t[21]);
                v[11] = vzip1q_s16(t[22], t[23]);
                v[12] = vzip1q_s16(t[24], t[25]);
                v[13] = vzip1q_s16(t[26], t[27]);
                v[14] = vzip1q_s16(t[28], t[29]);
                v[15] = vzip1q_s16(t[30], t[31]);

                v[16] = vzip2q_s16(t[0], t[1]);
                v[17] = vzip2q_s16(t[2], t[3]);
                v[18] = vzip2q_s16(t[4], t[5]);
                v[19] = vzip2q_s16(t[6], t[7]);
                v[20] = vzip2q_s16(t[8], t[9]);
                v[21] = vzip2q_s16(t[10], t[11]);
                v[22] = vzip2q_s16(t[12], t[13]);
                v[23] = vzip2q_s16(t[14], t[15]);
                v[24] = vzip2q_s16(t[16], t[17]);
                v[25] = vzip2q_s16(t[18], t[19]);
                v[26] = vzip2q_s16(t[20], t[21]);
                v[27] = vzip2q_s16(t[22], t[23]);
                v[28] = vzip2q_s16(t[24], t[25]);
                v[29] = vzip2q_s16(t[26], t[27]);
                v[30] = vzip2q_s16(t[28], t[29]);
                v[31] = vzip2q_s16(t[30], t[31]);

                t[0]  = vzip1q_s32(v[0], v[1]);
                t[1]  = vzip1q_s32(v[2], v[3]);
                t[2]  = vzip1q_s32(v[4], v[5]);
                t[3]  = vzip1q_s32(v[6], v[7]);
                t[4]  = vzip1q_s32(v[8], v[9]);
                t[5]  = vzip1q_s32(v[10], v[11]);
                t[6]  = vzip1q_s32(v[12], v[13]);
                t[7]  = vzip1q_s32(v[14], v[15]);
                t[8]  = vzip1q_s32(v[16], v[17]);
                t[9]  = vzip1q_s32(v[18], v[19]);
                t[10] = vzip1q_s32(v[20], v[21]);
                t[11] = vzip1q_s32(v[22], v[23]);
                t[12] = vzip1q_s32(v[24], v[25]);
                t[13] = vzip1q_s32(v[26], v[27]);
                t[14] = vzip1q_s32(v[28], v[29]);
                t[15] = vzip1q_s32(v[30], v[31]);

                t[16] = vzip2q_s32(v[0], v[1]);
                t[17] = vzip2q_s32(v[2], v[3]);
                t[18] = vzip2q_s32(v[4], v[5]);
                t[19] = vzip2q_s32(v[6], v[7]);
                t[20] = vzip2q_s32(v[8], v[9]);
                t[21] = vzip2q_s32(v[10], v[11]);
                t[22] = vzip2q_s32(v[12], v[13]);
                t[23] = vzip2q_s32(v[14], v[15]);
                t[24] = vzip2q_s32(v[16], v[17]);
                t[25] = vzip2q_s32(v[18], v[19]);
                t[26] = vzip2q_s32(v[20], v[21]);
                t[27] = vzip2q_s32(v[22], v[23]);
                t[28] = vzip2q_s32(v[24], v[25]);
                t[29] = vzip2q_s32(v[26], v[27]);
                t[30] = vzip2q_s32(v[28], v[29]);
                t[31] = vzip2q_s32(v[30], v[31]);

                v[0] = vzip1q_s64(t[0], t[1]);
                v[1] = vzip1q_s64(t[2], t[3]);
                v[2] = vzip1q_s64(t[4], t[5]);
                v[3] = vzip1q_s64(t[6], t[7]);
                v[4] = vzip1q_s64(t[8], t[9]);
                v[5] = vzip1q_s64(t[10], t[11]);
                v[6] = vzip1q_s64(t[12], t[13]);
                v[7] = vzip1q_s64(t[14], t[15]);

                v[8]  = vzip2q_s64(t[0], t[1]);
                v[9]  = vzip2q_s64(t[2], t[3]);
                v[10] = vzip2q_s64(t[4], t[5]);
                v[11] = vzip2q_s64(t[6], t[7]);
                v[12] = vzip2q_s64(t[8], t[9]);
                v[13] = vzip2q_s64(t[10], t[11]);
                v[14] = vzip2q_s64(t[12], t[13]);
                v[15] = vzip2q_s64(t[14], t[15]);

                v[16] = vzip1q_s64(t[16], t[17]);
                v[17] = vzip1q_s64(t[18], t[19]);
                v[18] = vzip1q_s64(t[20], t[21]);
                v[19] = vzip1q_s64(t[22], t[23]);
                v[20] = vzip1q_s64(t[24], t[25]);
                v[21] = vzip1q_s64(t[26], t[27]);
                v[22] = vzip1q_s64(t[28], t[29]);
                v[23] = vzip1q_s64(t[30], t[31]);

                v[24] = vzip2q_s64(t[16], t[17]);
                v[25] = vzip2q_s64(t[18], t[19]);
                v[26] = vzip2q_s64(t[20], t[21]);
                v[27] = vzip2q_s64(t[22], t[23]);
                v[28] = vzip2q_s64(t[24], t[25]);
                v[29] = vzip2q_s64(t[26], t[27]);
                v[30] = vzip2q_s64(t[28], t[29]);
                v[31] = vzip2q_s64(t[30], t[31]);

                for(i = 0; i < 32; i++) {
                    vst1q_s16((pel_dst), v[i]);
                    pel_dst += 8;
                }
            }
        }
        else {
            xeve_itx_pb64b(src, dst, shift, line, step);
        }
    }
}

#undef XEVE_MADD_S32
#undef vmadd_s16

const XEVE_ITXB xeve_tbl_itxb_neon[MAX_TR_LOG2] = {xeve_itx_pb2b,
                                                   xeve_itx_pb4b_neon,
                                                   xeve_itx_pb8b_neon,
                                                   xeve_itx_pb16b_neon,
                                                   xeve_itx_pb32b_neon,
                                                   xeve_itx_pb64b_neon};

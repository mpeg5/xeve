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
#include "xeve_tq_neon.h"

/* The first transform pass is called with shift == 0, so its s32 output can
   grow large enough that the second pass must accumulate coefficient products
   in 64 bits, like the plain C version does. The butterfly stages themselves
   stay within s32. */

/* load a 4x4 block of s16 at src (row stride w) transposed into 4 s32 column vectors */
static inline void tr4x4_s16(const s16 *src, int w, int32x4_t *c0, int32x4_t *c1, int32x4_t *c2, int32x4_t *c3)
{
    int16x4_t   r0  = vld1_s16(src);
    int16x4_t   r1  = vld1_s16(src + w);
    int16x4_t   r2  = vld1_s16(src + 2 * w);
    int16x4_t   r3  = vld1_s16(src + 3 * w);
    int16x4x2_t t01 = vtrn_s16(r0, r1);
    int16x4x2_t t23 = vtrn_s16(r2, r3);
    int32x2x2_t u02 = vtrn_s32(vreinterpret_s32_s16(t01.val[0]), vreinterpret_s32_s16(t23.val[0]));
    int32x2x2_t u13 = vtrn_s32(vreinterpret_s32_s16(t01.val[1]), vreinterpret_s32_s16(t23.val[1]));

    *c0 = vmovl_s16(vreinterpret_s16_s32(u02.val[0]));
    *c1 = vmovl_s16(vreinterpret_s16_s32(u13.val[0]));
    *c2 = vmovl_s16(vreinterpret_s16_s32(u02.val[1]));
    *c3 = vmovl_s16(vreinterpret_s16_s32(u13.val[1]));
}

/* load a 4x4 block of s32 at src (row stride w) transposed into 4 column vectors */
static inline void tr4x4_s32(const s32 *src, int w, int32x4_t *c0, int32x4_t *c1, int32x4_t *c2, int32x4_t *c3)
{
    int32x4_t   r0  = vld1q_s32(src);
    int32x4_t   r1  = vld1q_s32(src + w);
    int32x4_t   r2  = vld1q_s32(src + 2 * w);
    int32x4_t   r3  = vld1q_s32(src + 3 * w);
    int32x4x2_t t01 = vtrnq_s32(r0, r1);
    int32x4x2_t t23 = vtrnq_s32(r2, r3);

    *c0 = vcombine_s32(vget_low_s32(t01.val[0]), vget_low_s32(t23.val[0]));
    *c1 = vcombine_s32(vget_low_s32(t01.val[1]), vget_low_s32(t23.val[1]));
    *c2 = vcombine_s32(vget_high_s32(t01.val[0]), vget_high_s32(t23.val[0]));
    *c3 = vcombine_s32(vget_high_s32(t01.val[1]), vget_high_s32(t23.val[1]));
}

/* remaining lines (line % 4 != 0), same arithmetic as the plain C version */
static void tx_tail(void *src, void *dst, int shift, int line, int step, int size, const s8 *tm, int j0)
{
    int j, k, t;
    s64 sum;
    int add = shift == 0 ? 0 : 1 << (shift - 1);

    for(j = j0; j < line; j++) {
        for(k = 0; k < size; k++) {
            if(size == 64 && k > 31) {
                if(step == 0)
                    *((s32 *)dst + k * line + j) = 0;
                else
                    *((s16 *)dst + k * line + j) = 0;
                continue;
            }
            sum = 0;
            if(step == 0) {
                for(t = 0; t < size; t++)
                    sum += (s64)tm[k * size + t] * *((s16 *)src + j * size + t);
                *((s32 *)dst + k * line + j) = (s32)((sum + add) >> shift);
            }
            else {
                for(t = 0; t < size; t++)
                    sum += (s64)tm[k * size + t] * *((s32 *)src + j * size + t);
                *((s16 *)dst + k * line + j) = (s16)((sum + add) >> shift);
            }
        }
    }
}

#define LOAD_S16(N)                                                                             \
    for(c = 0; c < (N); c += 4) {                                                               \
        tr4x4_s16((const s16 *)src + j * (N) + c, (N), &X[c], &X[c + 1], &X[c + 2], &X[c + 3]); \
    }

#define LOAD_S32(N)                                                                             \
    for(c = 0; c < (N); c += 4) {                                                               \
        tr4x4_s32((const s32 *)src + j * (N) + c, (N), &X[c], &X[c + 1], &X[c + 2], &X[c + 3]); \
    }

/* step 0: s16 input, s32 accumulation, s32 output */
#define OUT_BEG0()      d = v_add
#define OUT_ACC0(v, cf) d = vmlaq_n_s32(d, v, (s32)(cf))
#define OUT_END0(k)     vst1q_s32((s32 *)dst + (k) * line + j, vshlq_s32(d, v_shift))
#define OUT_ZERO0(k)    vst1q_s32((s32 *)dst + (k) * line + j, vdupq_n_s32(0))

/* step 1: s32 input, s64 accumulation, s16 output */
#define OUT_BEG1() \
    lo = v_add64;  \
    hi = v_add64
#define OUT_ACC1(v, cf)                                    \
    lo = vmlal_n_s32(lo, vget_low_s32(v), (s32)(cf));      \
    hi = vmlal_n_s32(hi, vget_high_s32(v), (s32)(cf))
#define OUT_END1(k)                                                             \
    vst1_s16((s16 *)dst + (k) * line + j,                                       \
             vmovn_s32(vcombine_s32(vmovn_s64(vshlq_s64(lo, v_shift64)),        \
                                    vmovn_s64(vshlq_s64(hi, v_shift64)))))
#define OUT_ZERO1(k) vst1_s16((s16 *)dst + (k) * line + j, vdup_n_s16(0))

#define TX_PB8_CORE(LOAD, OB, OA, OE)                 \
    for(j = 0; j + 4 <= line; j += 4) {               \
        LOAD(8);                                      \
        for(k = 0; k < 4; k++) {                      \
            E[k] = vaddq_s32(X[k], X[7 - k]);         \
            O[k] = vsubq_s32(X[k], X[7 - k]);         \
        }                                             \
        EE[0] = vaddq_s32(E[0], E[3]);                \
        EO[0] = vsubq_s32(E[0], E[3]);                \
        EE[1] = vaddq_s32(E[1], E[2]);                \
        EO[1] = vsubq_s32(E[1], E[2]);                \
                                                      \
        OB();                                         \
        OA(EE[0], xeve_tbl_tm8[0][0]);                \
        OA(EE[1], xeve_tbl_tm8[0][1]);                \
        OE(0);                                        \
        OB();                                         \
        OA(EE[0], xeve_tbl_tm8[4][0]);                \
        OA(EE[1], xeve_tbl_tm8[4][1]);                \
        OE(4);                                        \
        OB();                                         \
        OA(EO[0], xeve_tbl_tm8[2][0]);                \
        OA(EO[1], xeve_tbl_tm8[2][1]);                \
        OE(2);                                        \
        OB();                                         \
        OA(EO[0], xeve_tbl_tm8[6][0]);                \
        OA(EO[1], xeve_tbl_tm8[6][1]);                \
        OE(6);                                        \
        for(k = 1; k < 8; k += 2) {                   \
            OB();                                     \
            for(t = 0; t < 4; t++) {                  \
                OA(O[t], xeve_tbl_tm8[k][t]);         \
            }                                         \
            OE(k);                                    \
        }                                             \
    }

static void tx_pb8b_neon(void *src, void *dst, int shift, int line, int step)
{
    int             j, k, t, c;
    int32x4_t       X[8], E[4], O[4], EE[2], EO[2], d;
    int64x2_t       lo, hi;
    const int32x4_t v_shift   = vdupq_n_s32(-shift);
    const int64x2_t v_shift64 = vdupq_n_s64(-shift);
    const int32x4_t v_add     = vdupq_n_s32(shift == 0 ? 0 : 1 << (shift - 1));
    const int64x2_t v_add64   = vdupq_n_s64(shift == 0 ? 0 : 1 << (shift - 1));

    if(step == 0) {
        TX_PB8_CORE(LOAD_S16, OUT_BEG0, OUT_ACC0, OUT_END0);
    }
    else {
        TX_PB8_CORE(LOAD_S32, OUT_BEG1, OUT_ACC1, OUT_END1);
    }
    if(line & 3) {
        tx_tail(src, dst, shift, line, step, 8, xeve_tbl_tm8[0], line & ~3);
    }
}

#define TX_PB16_CORE(LOAD, OB, OA, OE)                \
    for(j = 0; j + 4 <= line; j += 4) {               \
        LOAD(16);                                     \
        for(k = 0; k < 8; k++) {                      \
            E[k] = vaddq_s32(X[k], X[15 - k]);        \
            O[k] = vsubq_s32(X[k], X[15 - k]);        \
        }                                             \
        for(k = 0; k < 4; k++) {                      \
            EE[k] = vaddq_s32(E[k], E[7 - k]);        \
            EO[k] = vsubq_s32(E[k], E[7 - k]);        \
        }                                             \
        EEE[0] = vaddq_s32(EE[0], EE[3]);             \
        EEO[0] = vsubq_s32(EE[0], EE[3]);             \
        EEE[1] = vaddq_s32(EE[1], EE[2]);             \
        EEO[1] = vsubq_s32(EE[1], EE[2]);             \
                                                      \
        OB();                                         \
        OA(EEE[0], xeve_tbl_tm16[0][0]);              \
        OA(EEE[1], xeve_tbl_tm16[0][1]);              \
        OE(0);                                        \
        OB();                                         \
        OA(EEE[0], xeve_tbl_tm16[8][0]);              \
        OA(EEE[1], xeve_tbl_tm16[8][1]);              \
        OE(8);                                        \
        OB();                                         \
        OA(EEO[0], xeve_tbl_tm16[4][0]);              \
        OA(EEO[1], xeve_tbl_tm16[4][1]);              \
        OE(4);                                        \
        OB();                                         \
        OA(EEO[0], xeve_tbl_tm16[12][0]);             \
        OA(EEO[1], xeve_tbl_tm16[12][1]);             \
        OE(12);                                       \
        for(k = 2; k < 16; k += 4) {                  \
            OB();                                     \
            for(t = 0; t < 4; t++) {                  \
                OA(EO[t], xeve_tbl_tm16[k][t]);       \
            }                                         \
            OE(k);                                    \
        }                                             \
        for(k = 1; k < 16; k += 2) {                  \
            OB();                                     \
            for(t = 0; t < 8; t++) {                  \
                OA(O[t], xeve_tbl_tm16[k][t]);        \
            }                                         \
            OE(k);                                    \
        }                                             \
    }

static void tx_pb16b_neon(void *src, void *dst, int shift, int line, int step)
{
    int             j, k, t, c;
    int32x4_t       X[16], E[8], O[8], EE[4], EO[4], EEE[2], EEO[2], d;
    int64x2_t       lo, hi;
    const int32x4_t v_shift   = vdupq_n_s32(-shift);
    const int64x2_t v_shift64 = vdupq_n_s64(-shift);
    const int32x4_t v_add     = vdupq_n_s32(shift == 0 ? 0 : 1 << (shift - 1));
    const int64x2_t v_add64   = vdupq_n_s64(shift == 0 ? 0 : 1 << (shift - 1));

    if(step == 0) {
        TX_PB16_CORE(LOAD_S16, OUT_BEG0, OUT_ACC0, OUT_END0);
    }
    else {
        TX_PB16_CORE(LOAD_S32, OUT_BEG1, OUT_ACC1, OUT_END1);
    }
    if(line & 3) {
        tx_tail(src, dst, shift, line, step, 16, xeve_tbl_tm16[0], line & ~3);
    }
}

#define TX_PB32_CORE(LOAD, OB, OA, OE)                \
    for(j = 0; j + 4 <= line; j += 4) {               \
        LOAD(32);                                     \
        for(k = 0; k < 16; k++) {                     \
            E[k] = vaddq_s32(X[k], X[31 - k]);        \
            O[k] = vsubq_s32(X[k], X[31 - k]);        \
        }                                             \
        for(k = 0; k < 8; k++) {                      \
            EE[k] = vaddq_s32(E[k], E[15 - k]);       \
            EO[k] = vsubq_s32(E[k], E[15 - k]);       \
        }                                             \
        for(k = 0; k < 4; k++) {                      \
            EEE[k] = vaddq_s32(EE[k], EE[7 - k]);     \
            EEO[k] = vsubq_s32(EE[k], EE[7 - k]);     \
        }                                             \
        EEEE[0] = vaddq_s32(EEE[0], EEE[3]);          \
        EEEO[0] = vsubq_s32(EEE[0], EEE[3]);          \
        EEEE[1] = vaddq_s32(EEE[1], EEE[2]);          \
        EEEO[1] = vsubq_s32(EEE[1], EEE[2]);          \
                                                      \
        OB();                                         \
        OA(EEEE[0], xeve_tbl_tm32[0][0]);             \
        OA(EEEE[1], xeve_tbl_tm32[0][1]);             \
        OE(0);                                        \
        OB();                                         \
        OA(EEEE[0], xeve_tbl_tm32[16][0]);            \
        OA(EEEE[1], xeve_tbl_tm32[16][1]);            \
        OE(16);                                       \
        OB();                                         \
        OA(EEEO[0], xeve_tbl_tm32[8][0]);             \
        OA(EEEO[1], xeve_tbl_tm32[8][1]);             \
        OE(8);                                        \
        OB();                                         \
        OA(EEEO[0], xeve_tbl_tm32[24][0]);            \
        OA(EEEO[1], xeve_tbl_tm32[24][1]);            \
        OE(24);                                       \
        for(k = 4; k < 32; k += 8) {                  \
            OB();                                     \
            for(t = 0; t < 4; t++) {                  \
                OA(EEO[t], xeve_tbl_tm32[k][t]);      \
            }                                         \
            OE(k);                                    \
        }                                             \
        for(k = 2; k < 32; k += 4) {                  \
            OB();                                     \
            for(t = 0; t < 8; t++) {                  \
                OA(EO[t], xeve_tbl_tm32[k][t]);       \
            }                                         \
            OE(k);                                    \
        }                                             \
        for(k = 1; k < 32; k += 2) {                  \
            OB();                                     \
            for(t = 0; t < 16; t++) {                 \
                OA(O[t], xeve_tbl_tm32[k][t]);        \
            }                                         \
            OE(k);                                    \
        }                                             \
    }

static void tx_pb32b_neon(void *src, void *dst, int shift, int line, int step)
{
    int             j, k, t, c;
    int32x4_t       X[32], E[16], O[16], EE[8], EO[8], EEE[4], EEO[4], EEEE[2], EEEO[2], d;
    int64x2_t       lo, hi;
    const int32x4_t v_shift   = vdupq_n_s32(-shift);
    const int64x2_t v_shift64 = vdupq_n_s64(-shift);
    const int32x4_t v_add     = vdupq_n_s32(shift == 0 ? 0 : 1 << (shift - 1));
    const int64x2_t v_add64   = vdupq_n_s64(shift == 0 ? 0 : 1 << (shift - 1));

    if(step == 0) {
        TX_PB32_CORE(LOAD_S16, OUT_BEG0, OUT_ACC0, OUT_END0);
    }
    else {
        TX_PB32_CORE(LOAD_S32, OUT_BEG1, OUT_ACC1, OUT_END1);
    }
    if(line & 3) {
        tx_tail(src, dst, shift, line, step, 32, xeve_tbl_tm32[0], line & ~3);
    }
}

#define TX_PB64_CORE(LOAD, OB, OA, OE, OZ)            \
    for(j = 0; j + 4 <= line; j += 4) {               \
        LOAD(64);                                     \
        for(k = 0; k < 32; k++) {                     \
            E[k] = vaddq_s32(X[k], X[63 - k]);        \
            O[k] = vsubq_s32(X[k], X[63 - k]);        \
        }                                             \
        for(k = 0; k < 16; k++) {                     \
            EE[k] = vaddq_s32(E[k], E[31 - k]);       \
            EO[k] = vsubq_s32(E[k], E[31 - k]);       \
        }                                             \
        for(k = 0; k < 8; k++) {                      \
            EEE[k] = vaddq_s32(EE[k], EE[15 - k]);    \
            EEO[k] = vsubq_s32(EE[k], EE[15 - k]);    \
        }                                             \
        for(k = 0; k < 4; k++) {                      \
            EEEE[k] = vaddq_s32(EEE[k], EEE[7 - k]);  \
            EEEO[k] = vsubq_s32(EEE[k], EEE[7 - k]);  \
        }                                             \
        EEEEE[0] = vaddq_s32(EEEE[0], EEEE[3]);       \
        EEEEO[0] = vsubq_s32(EEEE[0], EEEE[3]);       \
        EEEEE[1] = vaddq_s32(EEEE[1], EEEE[2]);       \
        EEEEO[1] = vsubq_s32(EEEE[1], EEEE[2]);       \
                                                      \
        OB();                                         \
        OA(EEEEE[0], tm[0 * 64 + 0]);                 \
        OA(EEEEE[1], tm[0 * 64 + 1]);                 \
        OE(0);                                        \
        OB();                                         \
        OA(EEEEO[0], tm[16 * 64 + 0]);                \
        OA(EEEEO[1], tm[16 * 64 + 1]);                \
        OE(16);                                       \
        OZ(32);                                       \
        OZ(48);                                       \
        for(k = 8; k < 32; k += 16) {                 \
            OB();                                     \
            for(t = 0; t < 4; t++) {                  \
                OA(EEEO[t], tm[k * 64 + t]);          \
            }                                         \
            OE(k);                                    \
        }                                             \
        for(k = 40; k < 64; k += 16) {                \
            OZ(k);                                    \
        }                                             \
        for(k = 4; k < 32; k += 8) {                  \
            OB();                                     \
            for(t = 0; t < 8; t++) {                  \
                OA(EEO[t], tm[k * 64 + t]);           \
            }                                         \
            OE(k);                                    \
        }                                             \
        for(k = 36; k < 64; k += 8) {                 \
            OZ(k);                                    \
        }                                             \
        for(k = 2; k < 32; k += 4) {                  \
            OB();                                     \
            for(t = 0; t < 16; t++) {                 \
                OA(EO[t], tm[k * 64 + t]);            \
            }                                         \
            OE(k);                                    \
        }                                             \
        for(k = 34; k < 64; k += 4) {                 \
            OZ(k);                                    \
        }                                             \
        for(k = 1; k < 32; k += 2) {                  \
            OB();                                     \
            for(t = 0; t < 32; t++) {                 \
                OA(O[t], tm[k * 64 + t]);             \
            }                                         \
            OE(k);                                    \
        }                                             \
        for(k = 33; k < 64; k += 2) {                 \
            OZ(k);                                    \
        }                                             \
    }

static void tx_pb64b_neon(void *src, void *dst, int shift, int line, int step)
{
    const s8       *tm = xeve_tbl_tm64[0];
    int             j, k, t, c;
    int32x4_t       X[64], E[32], O[32], EE[16], EO[16], EEE[8], EEO[8], EEEE[4], EEEO[4], EEEEE[2], EEEEO[2], d;
    int64x2_t       lo, hi;
    const int32x4_t v_shift   = vdupq_n_s32(-shift);
    const int64x2_t v_shift64 = vdupq_n_s64(-shift);
    const int32x4_t v_add     = vdupq_n_s32(shift == 0 ? 0 : 1 << (shift - 1));
    const int64x2_t v_add64   = vdupq_n_s64(shift == 0 ? 0 : 1 << (shift - 1));

    if(step == 0) {
        TX_PB64_CORE(LOAD_S16, OUT_BEG0, OUT_ACC0, OUT_END0, OUT_ZERO0);
    }
    else {
        TX_PB64_CORE(LOAD_S32, OUT_BEG1, OUT_ACC1, OUT_END1, OUT_ZERO1);
    }
    if(line & 3) {
        tx_tail(src, dst, shift, line, step, 64, tm, line & ~3);
    }
}

const XEVE_TXB xeve_tbl_txb_neon[MAX_TR_LOG2] =
    {tx_pb2b, tx_pb4b, tx_pb8b_neon, tx_pb16b_neon, tx_pb32b_neon, tx_pb64b_neon};

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

#include "xeve_type.h"
#include <math.h>

#if ARM_NEON

/* SAD for 16bit **************************************************************/
int sad_16b_neon_4x2(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int bit_depth)
{
    int            sad;
    int16_t const* s1           = src1;
    int16_t const* s2           = src2;
    /*
        -- Variable naming conventions are similar as SSE code.
        -- 4x16b translates to 4 datapoints each stored in 16bit
    */
    int16x4_t      src_4x16b    = vld1_s16((s1));
    int16x4_t      pred_4x16b   = vld1_s16((s2));
    int16x4_t      src_4x16b_1  = vld1_s16((s1 + s_src1));
    int16x4_t      pred_4x16b_1 = vld1_s16((s2 + s_src2));

    int16x4_t abs_diff_4x16b   = vabd_s16(src_4x16b, pred_4x16b);
    int16x4_t abs_diff_4x16b_1 = vabd_s16(src_4x16b_1, pred_4x16b_1);

    sad = vaddv_s16(abs_diff_4x16b);
    sad += vaddv_s16(abs_diff_4x16b_1);

    return (sad >> (bit_depth - 8));
}

int sad_16b_neon_4x2n(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int bit_depth)
{
    int            sad;
    int16_t const* s1 = src1;
    int16_t const* s2 = src2;
    int            i;
    /*
        -- Variable naming conventions are similar as SSE code.
        -- 4x16b translates to 4 datapoints each stored in 16bit
    */
    int16x4_t      src_4x16b, pred_4x16b, abs_diff_4x16b;
    int16x4_t      src_4x16b_1, pred_4x16b_1, abs_diff_4x16b_1;

    h = h >> 1;
    for(i = 0; i != h; ++i) {
        src_4x16b    = vld1_s16((s1));
        pred_4x16b   = vld1_s16((s2));
        src_4x16b_1  = vld1_s16((s1 + s_src1));
        pred_4x16b_1 = vld1_s16((s2 + s_src2));

        abs_diff_4x16b   = vabd_s16(src_4x16b, pred_4x16b);
        abs_diff_4x16b_1 = vabd_s16(src_4x16b_1, pred_4x16b_1);

        sad = vaddv_s16(abs_diff_4x16b);
        sad += vaddv_s16(abs_diff_4x16b_1);

        s1 += s_src1 << 1;
        s2 += s_src2 << 1;
    }

    return (sad >> (bit_depth - 8));
}

int sad_16b_neon_4x4(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int bit_depth)
{
    int            sad;
    int16_t const* s1 = src1;
    int16_t const* s2 = src2;
    /*
        -- Variable naming conventions are same as SSE code.
        -- 4x16b translates to 4 datapoints each stored in 16bit
    */
    int16x4_t      src_4x16b, pred_4x16b, abs_diff_4x16b;
    int16x4_t      src_4x16b_1, pred_4x16b_1, abs_diff_4x16b_1;
    int16x4_t      src_4x16b_2, pred_4x16b_2, abs_diff_4x16b_2;
    int16x4_t      src_4x16b_3, pred_4x16b_3, abs_diff_4x16b_3;

    src_4x16b  = vld1_s16((s1));
    pred_4x16b = vld1_s16((s2));

    src_4x16b_1  = vld1_s16((s1 + s_src1));
    pred_4x16b_1 = vld1_s16((s2 + s_src2));

    src_4x16b_2  = vld1_s16((s1 + s_src1 * 2));
    pred_4x16b_2 = vld1_s16((s2 + s_src2 * 2));

    src_4x16b_3  = vld1_s16((s1 + s_src1 * 3));
    pred_4x16b_3 = vld1_s16((s2 + s_src2 * 3));

    abs_diff_4x16b   = vabd_s16(src_4x16b, pred_4x16b);
    abs_diff_4x16b_1 = vabd_s16(src_4x16b_1, pred_4x16b_1);
    abs_diff_4x16b_2 = vabd_s16(src_4x16b_2, pred_4x16b_2);
    abs_diff_4x16b_3 = vabd_s16(src_4x16b_3, pred_4x16b_3);

    sad = vaddv_s16(abs_diff_4x16b);
    sad += vaddv_s16(abs_diff_4x16b_1);
    sad += vaddv_s16(abs_diff_4x16b_2);
    sad += vaddv_s16(abs_diff_4x16b_3);

    return (sad >> (bit_depth - 8));
}

int sad_16b_neon_8x2n(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int bit_depth)
{
    int16_t const *pu2_inp, *pu2_inp2;
    int16_t const *pu2_ref, *pu2_ref2;

    pu2_inp       = src1;
    pu2_ref       = src2;
    pu2_inp2      = (s16*)src1 + s_src1;
    pu2_ref2      = (s16*)src2 + s_src2;
    int s_src1_t2 = s_src1 * 2;
    int s_src2_t2 = s_src2 * 2;

    int       i, j;
    u32       sad = 0;
    /*
        -- Variable naming conventions are same as SSE code.
        -- 8x16b translates to 8 datapoints each stored in 16bit
    */
    int16x8_t src_8x16b, pred_8x16b, abs_diff_8x16b;
    int16x8_t src_8x16b_1, pred_8x16b_1, abs_diff_8x16b_1;

    h = h >> 1;
    for(i = 0; i != h; ++i) {
        for(j = 0; j < w; j += 8) {
            src_8x16b  = vld1q_s16(&pu2_inp[j]);
            pred_8x16b = vld1q_s16(&pu2_ref[j]);

            src_8x16b_1  = vld1q_s16(&pu2_inp2[j]);
            pred_8x16b_1 = vld1q_s16(&pu2_ref2[j]);

            abs_diff_8x16b   = vabdq_s16(src_8x16b, pred_8x16b);
            abs_diff_8x16b_1 = vabdq_s16(src_8x16b_1, pred_8x16b_1);

            sad += vaddvq_s16(abs_diff_8x16b);
            sad += vaddvq_s16(abs_diff_8x16b_1);
        }
        pu2_inp += s_src1_t2;
        pu2_ref += s_src2_t2;
        pu2_inp2 += s_src1_t2;
        pu2_ref2 += s_src2_t2;
    }
    return (sad >> (bit_depth - 8));
}

int sad_16b_neon_16nx1n(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int bit_depth)
{
    int16_t const* pu2_inp = src1;
    int16_t const* pu2_ref = src2;

    u32       sad = 0;
    int16x8_t src_8x16b, pred_8x16b, abs_diff_8x16b;
    int16x8_t src_8x16b_1, pred_8x16b_1, abs_diff_8x16b_1;
    for(int i = 0; i != h; ++i) {
        for(int j = 0; j < w; j += 16) {
            src_8x16b    = vld1q_s16(&pu2_inp[j]);
            pred_8x16b   = vld1q_s16(&pu2_ref[j]);
            src_8x16b_1  = vld1q_s16(&pu2_inp[j + 8]);
            pred_8x16b_1 = vld1q_s16(&pu2_ref[j + 8]);

            abs_diff_8x16b   = vabdq_s16(src_8x16b, pred_8x16b);
            abs_diff_8x16b_1 = vabdq_s16(src_8x16b_1, pred_8x16b_1);
            sad += vaddvq_s16(abs_diff_8x16b);
            sad += vaddvq_s16(abs_diff_8x16b_1);
        }
        pu2_inp += s_src1;
        pu2_ref += s_src2;
    }
    return (sad >> (bit_depth - 8));
}

/* index: [log2 of width][log2 of height] */
const XEVE_FN_SAD xeve_tbl_sad_16b_neon[8][8] = {
    /* width == 1 */
    {
        sad_16b, /* height == 1 */
        sad_16b, /* height == 2 */
        sad_16b, /* height == 4 */
        sad_16b, /* height == 8 */
        sad_16b, /* height == 16 */
        sad_16b, /* height == 32 */
        sad_16b, /* height == 64 */
        sad_16b, /* height == 128 */
    },
    /* width == 2 */
    {
        sad_16b, /* height == 1 */
        sad_16b, /* height == 2 */
        sad_16b, /* height == 4 */
        sad_16b, /* height == 8 */
        sad_16b, /* height == 16 */
        sad_16b, /* height == 32 */
        sad_16b, /* height == 64 */
        sad_16b, /* height == 128 */
    },
    /* width == 4 */
    {
        sad_16b,           /* height == 1 */
        sad_16b_neon_4x2,  /* height == 2 */
        sad_16b_neon_4x4,  /* height == 4 */
        sad_16b_neon_4x2n, /* height == 8 */
        sad_16b_neon_4x2n, /* height == 16 */
        sad_16b_neon_4x2n, /* height == 32 */
        sad_16b_neon_4x2n, /* height == 64 */
        sad_16b_neon_4x2n, /* height == 128 */
    },
    /* width == 8 */
    {
        sad_16b,           /* height == 1 */
        sad_16b_neon_8x2n, /* height == 2 */
        sad_16b_neon_8x2n, /* height == 4 */
        sad_16b_neon_8x2n, /* height == 8 */
        sad_16b_neon_8x2n, /* height == 16 */
        sad_16b_neon_8x2n, /* height == 32 */
        sad_16b_neon_8x2n, /* height == 64 */
        sad_16b_neon_8x2n, /* height == 128 */
    },
    /* width == 16 */
    {
        sad_16b_neon_16nx1n, /* height == 1 */
        sad_16b_neon_16nx1n, /* height == 2 */
        sad_16b_neon_16nx1n, /* height == 4 */
        sad_16b_neon_16nx1n, /* height == 8 */
        sad_16b_neon_16nx1n, /* height == 16 */
        sad_16b_neon_16nx1n, /* height == 32 */
        sad_16b_neon_16nx1n, /* height == 64 */
        sad_16b_neon_16nx1n, /* height == 128 */
    },
    /* width == 32 */
    {
        sad_16b_neon_16nx1n, /* height == 1 */
        sad_16b_neon_16nx1n, /* height == 2 */
        sad_16b_neon_16nx1n, /* height == 4 */
        sad_16b_neon_16nx1n, /* height == 8 */
        sad_16b_neon_16nx1n, /* height == 16 */
        sad_16b_neon_16nx1n, /* height == 32 */
        sad_16b_neon_16nx1n, /* height == 64 */
        sad_16b_neon_16nx1n, /* height == 128 */
    },
    /* width == 64 */
    {
        sad_16b_neon_16nx1n, /* height == 1 */
        sad_16b_neon_16nx1n, /* height == 2 */
        sad_16b_neon_16nx1n, /* height == 4 */
        sad_16b_neon_16nx1n, /* height == 8 */
        sad_16b_neon_16nx1n, /* height == 16 */
        sad_16b_neon_16nx1n, /* height == 32 */
        sad_16b_neon_16nx1n, /* height == 64 */
        sad_16b_neon_16nx1n, /* height == 128 */
    },
    /* width == 128 */
    {
        sad_16b_neon_16nx1n, /* height == 1 */
        sad_16b_neon_16nx1n, /* height == 2 */
        sad_16b_neon_16nx1n, /* height == 4 */
        sad_16b_neon_16nx1n, /* height == 8 */
        sad_16b_neon_16nx1n, /* height == 16 */
        sad_16b_neon_16nx1n, /* height == 32 */
        sad_16b_neon_16nx1n, /* height == 64 */
        sad_16b_neon_16nx1n, /* height == 128 */
    }};

/* DIFF **********************************************************************/
#define NEON_DIFF_16B_4PEL(src1, src2, diff, m00, m01, m02) \
    m00 = vld1_s16((src1));                                 \
    m01 = vld1_s16((src2));                                 \
    m02 = vsub_s16(m00, m01);                               \
    vst1_s16((int16_t*)(diff), m02);

#define NEON_DIFF_16B_8PEL(src1, src2, diff, m00, m01, m02) \
    m00 = vld1q_s16((src1));                                \
    m01 = vld1q_s16((src2));                                \
    m02 = vsubq_s16(m00, m01);                              \
    vst1q_s16((int16_t*)(diff), m02);

static void
diff_16b_neon_4x2(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int s_diff, s16* diff, int bit_depth)
{
    int16_t const* s1;
    int16_t const* s2;
    int16x4_t      m01, m02, m03, m04, m05, m06;

    s1 = src1;
    s2 = src2;

    NEON_DIFF_16B_4PEL(s1, s2, diff, m01, m02, m03);
    NEON_DIFF_16B_4PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
}

static void
diff_16b_neon_4x4(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int s_diff, s16* diff, int bit_depth)
{
    int16_t const* s1;
    int16_t const* s2;
    int16x4_t      m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12;

    s1 = src1;
    s2 = src2;

    NEON_DIFF_16B_4PEL(s1, s2, diff, m01, m02, m03);
    NEON_DIFF_16B_4PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
    NEON_DIFF_16B_4PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, diff + s_diff * 2, m07, m08, m09);
    NEON_DIFF_16B_4PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, diff + s_diff * 3, m10, m11, m12);
}

static void
diff_16b_neon_8x8(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int s_diff, s16* diff, int bit_depth)
{
    int16_t const* s1;
    int16_t const* s2;
    int16x8_t      m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12;

    s1 = src1;
    s2 = src2;

    NEON_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
    NEON_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
    NEON_DIFF_16B_8PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, diff + s_diff * 2, m07, m08, m09);
    NEON_DIFF_16B_8PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, diff + s_diff * 3, m10, m11, m12);
    NEON_DIFF_16B_8PEL(s1 + s_src1 * 4, s2 + s_src2 * 4, diff + s_diff * 4, m01, m02, m03);
    NEON_DIFF_16B_8PEL(s1 + s_src1 * 5, s2 + s_src2 * 5, diff + s_diff * 5, m04, m05, m06);
    NEON_DIFF_16B_8PEL(s1 + s_src1 * 6, s2 + s_src2 * 6, diff + s_diff * 6, m07, m08, m09);
    NEON_DIFF_16B_8PEL(s1 + s_src1 * 7, s2 + s_src2 * 7, diff + s_diff * 7, m10, m11, m12);
}

static void
diff_16b_neon_8nx2n(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int s_diff, s16* diff, int bit_depth)
{
    int16_t const* s1;
    int16_t const* s2;
    int            i, j;
    int16x8_t      m01, m02, m03, m04, m05, m06;

    s1 = src1;
    s2 = src2;

    h = h >> 1;
    w = w >> 3;
    for(i = 0; i != h; ++i) {
        for(j = 0; j != w; ++j) {
            NEON_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            NEON_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            s1 += 8;
            s2 += 8;
            diff += 8;
        }

        s1 += ((s_src1 << 1) - (w << 3));
        s2 += ((s_src2 << 1) - (w << 3));
        diff += ((s_diff << 1) - (w << 3));
    }
}

static void
diff_16b_neon_16nx2n(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int s_diff, s16* diff, int bit_depth)
{
    int16_t const* s1;
    int16_t const* s2;
    int            i, j;
    int16x8_t      m01, m02, m03, m04, m05, m06;

    s1 = src1;
    s2 = src2;

    h = h >> 1;
    w = w >> 4;

    for(i = 0; i != h; ++i) {
        for(j = 0; j != w; ++j) {
            NEON_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            NEON_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            s1 += 8;
            s2 += 8;
            diff += 8;

            NEON_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            NEON_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            s1 += 8;
            s2 += 8;
            diff += 8;
        }

        s2 += ((s_src2 << 1) - (w << 4));
        s1 += ((s_src1 << 1) - (w << 4));
        diff += ((s_diff << 1) - (w << 4));
    }
}

static void
diff_16b_neon_32nx4n(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int s_diff, s16* diff, int bit_depth)
{
    int16_t const* s1;
    int16_t const* s2;
    int            i, j;
    int16x8_t      m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12;

    s1 = src1;
    s2 = src2;

    h = h >> 2;
    w = w >> 5;

    for(i = 0; i != h; ++i) {
        for(j = 0; j != w; ++j) {
            NEON_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            NEON_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            NEON_DIFF_16B_8PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, diff + s_diff * 2, m07, m08, m09);
            NEON_DIFF_16B_8PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, diff + s_diff * 3, m10, m11, m12);
            s1 += 8;
            s2 += 8;
            diff += 8;

            NEON_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            NEON_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            NEON_DIFF_16B_8PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, diff + s_diff * 2, m07, m08, m09);
            NEON_DIFF_16B_8PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, diff + s_diff * 3, m10, m11, m12);
            s1 += 8;
            s2 += 8;
            diff += 8;

            NEON_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            NEON_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            NEON_DIFF_16B_8PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, diff + s_diff * 2, m07, m08, m09);
            NEON_DIFF_16B_8PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, diff + s_diff * 3, m10, m11, m12);
            s1 += 8;
            s2 += 8;
            diff += 8;

            NEON_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            NEON_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            NEON_DIFF_16B_8PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, diff + s_diff * 2, m07, m08, m09);
            NEON_DIFF_16B_8PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, diff + s_diff * 3, m10, m11, m12);
            s1 += 8;
            s2 += 8;
            diff += 8;
        }

        s1 += ((s_src1 << 2) - (w << 5));
        s2 += ((s_src2 << 2) - (w << 5));
        diff += ((s_diff << 2) - (w << 5));
    }
}

// clang-format off

const XEVE_FN_DIFF xeve_tbl_diff_16b_neon[8][8] =
{
    /* width == 1 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 2 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 4 */
    {
        diff_16b, /* height == 1 */
        diff_16b_neon_4x2,  /* height == 2 */
        diff_16b_neon_4x4,  /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 8 */
    {
        diff_16b,  /* height == 1 */
        diff_16b_neon_8nx2n, /* height == 2 */
        diff_16b_neon_8nx2n, /* height == 4 */
        diff_16b_neon_8x8,   /* height == 8 */
        diff_16b_neon_8nx2n, /* height == 16 */
        diff_16b_neon_8nx2n, /* height == 32 */
        diff_16b_neon_8nx2n, /* height == 64 */
        diff_16b_neon_8nx2n, /* height == 128 */
    },
    /* width == 16 */
    {
        diff_16b,   /* height == 1 */
        diff_16b_neon_16nx2n, /* height == 2 */
        diff_16b_neon_16nx2n, /* height == 4 */
        diff_16b_neon_16nx2n, /* height == 8 */
        diff_16b_neon_16nx2n, /* height == 16 */
        diff_16b_neon_16nx2n, /* height == 32 */
        diff_16b_neon_16nx2n, /* height == 64 */
        diff_16b_neon_16nx2n, /* height == 128 */
    },
    /* width == 32 */
    {
        diff_16b,   /* height == 1 */
        diff_16b_neon_16nx2n, /* height == 2 */
        diff_16b_neon_32nx4n, /* height == 4 */
        diff_16b_neon_32nx4n, /* height == 8 */
        diff_16b_neon_32nx4n, /* height == 16 */
        diff_16b_neon_32nx4n, /* height == 32 */
        diff_16b_neon_32nx4n, /* height == 64 */
        diff_16b_neon_32nx4n, /* height == 128 */
    },
    /* width == 64 */
    {
        diff_16b,   /* height == 1 */
        diff_16b_neon_16nx2n, /* height == 2 */
        diff_16b_neon_32nx4n, /* height == 4 */
        diff_16b_neon_32nx4n, /* height == 8 */
        diff_16b_neon_32nx4n, /* height == 16 */
        diff_16b_neon_32nx4n, /* height == 32 */
        diff_16b_neon_32nx4n, /* height == 64 */
        diff_16b_neon_32nx4n, /* height == 128 */
    },
    /* width == 128 */
    {
        diff_16b,   /* height == 1 */
        diff_16b_neon_16nx2n, /* height == 2 */
        diff_16b_neon_32nx4n, /* height == 4 */
        diff_16b_neon_32nx4n, /* height == 8 */
        diff_16b_neon_32nx4n, /* height == 16 */
        diff_16b_neon_32nx4n, /* height == 32 */
        diff_16b_neon_32nx4n, /* height == 64 */
        diff_16b_neon_32nx4n, /* height == 128 */
    }
};

/* SSD ***********************************************************************/
#define NEON_SSD_16B_4PEL(src1, src2, shift, s00, s01, s02, ssd) \
    s00 = vld1_s16((src1)); \
    s01 = vld1_s16((src2)); \
    s00 = vsub_s16(s00, s01); \
    s02 = vmovl_s16(s00); \
    s02 = vmulq_s32(s02, s02); \
    s02 = vshrq_n_s32(s02, 4); \
    ssd += vaddvq_s32(s02);


#define NEON_SSD_16B_8PEL(src1, src2, shift, s00, s01, s02, s00a, s00b, ssd) \
    s00 = vld1q_s16((src1)); \
    s01 = vld1q_s16((src2)); \
    s02 = vsubq_s16(s00, s01); \
    s00a = vmovl_s16(vget_high_s16(s02)); \
    s00b = vmovl_s16(vget_low_s16(s02)); \
    s00a = vmulq_s32(s00a, s00a); \
    s00b = vmulq_s32(s00b, s00b); \
    s00a = vshrq_n_s32(s00a, 4); \
    s00b = vshrq_n_s32(s00b, 4); \
    ssd += vaddvq_s32(s00a); \
    ssd += vaddvq_s32(s00b);

// clang-format on

static s64 ssd_16b_neon_4x2(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int bit_depth)
{
    s64            ssd = 0;
    int16_t const* s1;
    int16_t const* s2;

    const int shift = (bit_depth - 8) << 1;
    int16x4_t s00, s01;
    int32x4_t s02;

    s1 = (s16*)src1;
    s2 = (s16*)src2;

    NEON_SSD_16B_4PEL(s1, s2, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s02, ssd);

    return ssd;
}

static s64 ssd_16b_neon_4x4(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int bit_depth)
{
    s64            ssd = 0;
    int16_t const* s1;
    int16_t const* s2;

    const int shift = (bit_depth - 8) << 1;
    int16x4_t s00, s01;
    int32x4_t s02;

    s1 = (s16*)src1;
    s2 = (s16*)src2;

    NEON_SSD_16B_4PEL(s1, s2, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, shift, s00, s01, s02, ssd);

    return ssd;
}

static s64 ssd_16b_neon_4x8(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int bit_depth)
{
    s64            ssd = 0;
    int16_t const* s1;
    int16_t const* s2;

    const int shift = (bit_depth - 8) << 1;
    int16x4_t s00, s01;
    int32x4_t s02;

    s1 = (s16*)src1;
    s2 = (s16*)src2;

    NEON_SSD_16B_4PEL(s1, s2, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 4, s2 + s_src2 * 4, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 5, s2 + s_src2 * 5, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 6, s2 + s_src2 * 6, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 7, s2 + s_src2 * 7, shift, s00, s01, s02, ssd);

    return ssd;
}

static s64 ssd_16b_neon_4x16(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int bit_depth)
{
    s64            ssd = 0;
    int16_t const* s1;
    int16_t const* s2;

    const int shift = (bit_depth - 8) << 1;
    int16x4_t s00, s01;
    int32x4_t s02;

    s1 = (s16*)src1;
    s2 = (s16*)src2;

    NEON_SSD_16B_4PEL(s1, s2, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 4, s2 + s_src2 * 4, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 5, s2 + s_src2 * 5, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 6, s2 + s_src2 * 6, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 7, s2 + s_src2 * 7, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 8, s2 + s_src2 * 8, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 9, s2 + s_src2 * 9, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 10, s2 + s_src2 * 10, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 11, s2 + s_src2 * 11, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 12, s2 + s_src2 * 12, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 13, s2 + s_src2 * 13, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 14, s2 + s_src2 * 14, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 15, s2 + s_src2 * 15, shift, s00, s01, s02, ssd);

    return ssd;
}

static s64 ssd_16b_neon_4x32(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int bit_depth)
{
    s64            ssd = 0;
    int16_t const* s1;
    int16_t const* s2;

    const int shift = (bit_depth - 8) << 1;
    int16x4_t s00, s01;
    int32x4_t s02;

    s1 = (s16*)src1;
    s2 = (s16*)src2;

    NEON_SSD_16B_4PEL(s1, s2, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 4, s2 + s_src2 * 4, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 5, s2 + s_src2 * 5, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 6, s2 + s_src2 * 6, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 7, s2 + s_src2 * 7, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 8, s2 + s_src2 * 8, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 9, s2 + s_src2 * 9, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 10, s2 + s_src2 * 10, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 11, s2 + s_src2 * 11, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 12, s2 + s_src2 * 12, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 13, s2 + s_src2 * 13, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 14, s2 + s_src2 * 14, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 15, s2 + s_src2 * 15, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 16, s2 + s_src2 * 16, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 17, s2 + s_src2 * 17, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 18, s2 + s_src2 * 18, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 19, s2 + s_src2 * 19, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 20, s2 + s_src2 * 20, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 21, s2 + s_src2 * 21, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 22, s2 + s_src2 * 22, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 23, s2 + s_src2 * 23, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 24, s2 + s_src2 * 24, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 25, s2 + s_src2 * 25, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 26, s2 + s_src2 * 26, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 27, s2 + s_src2 * 27, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 28, s2 + s_src2 * 28, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 29, s2 + s_src2 * 29, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 30, s2 + s_src2 * 30, shift, s00, s01, s02, ssd);
    NEON_SSD_16B_4PEL(s1 + s_src1 * 31, s2 + s_src2 * 31, shift, s00, s01, s02, ssd);

    return ssd;
}
static s64 ssd_16b_neon_8x2(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int bit_depth)
{
    s64            ssd = 0;
    int16_t const* s1;
    int16_t const* s2;

    const int shift = (bit_depth - 8) << 1;
    int16x8_t s00, s01, s02;
    int32x4_t s00a, s00b;

    s1 = (s16*)src1;
    s2 = (s16*)src2;

    NEON_SSD_16B_8PEL(s1, s2, shift, s00, s01, s02, s00a, s00b, ssd);
    NEON_SSD_16B_8PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s02, s00a, s00b, ssd);

    return ssd;
}

static s64 ssd_16b_neon_8x4(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int bit_depth)
{
    s64            ssd = 0;
    int16_t const* s1;
    int16_t const* s2;

    const int shift = (bit_depth - 8) << 1;
    int16x8_t s00, s01, s02;
    int32x4_t s00a, s00b;

    s1 = (s16*)src1;
    s2 = (s16*)src2;

    NEON_SSD_16B_8PEL(s1, s2, shift, s00, s01, s02, s00a, s00b, ssd);
    NEON_SSD_16B_8PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s02, s00a, s00b, ssd);
    NEON_SSD_16B_8PEL(s1 + 2 * s_src1, s2 + 2 * s_src2, shift, s00, s01, s02, s00a, s00b, ssd);
    NEON_SSD_16B_8PEL(s1 + 3 * s_src1, s2 + 3 * s_src2, shift, s00, s01, s02, s00a, s00b, ssd);

    return ssd;
}

static s64 ssd_16b_neon_8x8(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int bit_depth)
{
    s64            ssd = 0;
    int16_t const* s1;
    int16_t const* s2;

    const int shift = (bit_depth - 8) << 1;
    int16x8_t s00, s01, s02;
    int32x4_t s00a, s00b;

    s1 = (s16*)src1;
    s2 = (s16*)src2;

    NEON_SSD_16B_8PEL(s1, s2, shift, s00, s01, s02, s00a, s00b, ssd);
    NEON_SSD_16B_8PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s02, s00a, s00b, ssd);
    NEON_SSD_16B_8PEL(s1 + 2 * s_src1, s2 + 2 * s_src2, shift, s00, s01, s02, s00a, s00b, ssd);
    NEON_SSD_16B_8PEL(s1 + 3 * s_src1, s2 + 3 * s_src2, shift, s00, s01, s02, s00a, s00b, ssd);
    NEON_SSD_16B_8PEL(s1 + 4 * s_src1, s2 + 4 * s_src2, shift, s00, s01, s02, s00a, s00b, ssd);
    NEON_SSD_16B_8PEL(s1 + 5 * s_src1, s2 + 5 * s_src2, shift, s00, s01, s02, s00a, s00b, ssd);
    NEON_SSD_16B_8PEL(s1 + 6 * s_src1, s2 + 6 * s_src2, shift, s00, s01, s02, s00a, s00b, ssd);
    NEON_SSD_16B_8PEL(s1 + 7 * s_src1, s2 + 7 * s_src2, shift, s00, s01, s02, s00a, s00b, ssd);

    return ssd;
}

static s64 ssd_16b_neon_8nx2n(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int bit_depth)
{
    s64            ssd = 0;
    int16_t const* s1;
    int16_t const* s2;

    const int shift = (bit_depth - 8) << 1;
    int16x8_t s00, s01, s02;
    int32x4_t s00a, s00b;

    s1 = (s16*)src1;
    s2 = (s16*)src2;
    int i, j;
    h = h >> 1;
    w = w >> 3;
    for(i = 0; i != h; ++i) {
        for(j = 0; j != w; ++j) {
            NEON_SSD_16B_8PEL(s1, s2, shift, s00, s01, s02, s00a, s00b, ssd);
            NEON_SSD_16B_8PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s02, s00a, s00b, ssd);

            s1 += 8;
            s2 += 8;
        }
        s1 += (s_src1 << 1) - (w << 8);
        s2 += (s_src2 << 1) - (w << 8);
    }

    return ssd;
}

static s64 ssd_16b_neon_8nx4n(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int bit_depth)
{
    s64            ssd = 0;
    int16_t const* s1;
    int16_t const* s2;

    const int shift = (bit_depth - 8) << 1;
    int16x8_t s00, s01, s02;
    int32x4_t s00a, s00b;

    s1 = (s16*)src1;
    s2 = (s16*)src2;
    int i, j;

    h = h >> 2;
    w = w >> 3;

    for(i = 0; i != h; ++i) {
        for(j = 0; j != w; ++j) {
            NEON_SSD_16B_8PEL(s1, s2, shift, s00, s01, s02, s00a, s00b, ssd);
            NEON_SSD_16B_8PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s02, s00a, s00b, ssd);
            NEON_SSD_16B_8PEL(s1 + 2 * s_src1, s2 + 2 * s_src2, shift, s00, s01, s02, s00a, s00b, ssd);
            NEON_SSD_16B_8PEL(s1 + 3 * s_src1, s2 + 3 * s_src2, shift, s00, s01, s02, s00a, s00b, ssd);

            s1 += 8;
            s2 += 8;
        }
        s1 += (s_src1 << 2) - (w << 3);
        s2 += (s_src2 << 2) - (w << 3);
    }

    return ssd;
}

static s64 ssd_16b_neon_8nx8n(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int bit_depth)
{
    s64            ssd = 0;
    int16_t const* s1;
    int16_t const* s2;

    const int shift = (bit_depth - 8) << 1;
    int16x8_t s00, s01, s02;
    int32x4_t s00a, s00b;

    s1 = (s16*)src1;
    s2 = (s16*)src2;
    int i, j;
    h = h >> 3;
    w = w >> 3;
    for(i = 0; i != h; ++i) {
        for(j = 0; j != w; ++j) {
            NEON_SSD_16B_8PEL(s1, s2, shift, s00, s01, s02, s00a, s00b, ssd);
            NEON_SSD_16B_8PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s02, s00a, s00b, ssd);
            NEON_SSD_16B_8PEL(s1 + 2 * s_src1, s2 + 2 * s_src2, shift, s00, s01, s02, s00a, s00b, ssd);
            NEON_SSD_16B_8PEL(s1 + 3 * s_src1, s2 + 3 * s_src2, shift, s00, s01, s02, s00a, s00b, ssd);
            NEON_SSD_16B_8PEL(s1 + 4 * s_src1, s2 + 4 * s_src2, shift, s00, s01, s02, s00a, s00b, ssd);
            NEON_SSD_16B_8PEL(s1 + 5 * s_src1, s2 + 5 * s_src2, shift, s00, s01, s02, s00a, s00b, ssd);
            NEON_SSD_16B_8PEL(s1 + 6 * s_src1, s2 + 6 * s_src2, shift, s00, s01, s02, s00a, s00b, ssd);
            NEON_SSD_16B_8PEL(s1 + 7 * s_src1, s2 + 7 * s_src2, shift, s00, s01, s02, s00a, s00b, ssd);
            s1 += 8;
            s2 += 8;
        }
        s1 += (s_src1 << 3) - (w << 3);
        s2 += (s_src2 << 3) - (w << 3);
    }

    return ssd;
}

// clang-format off

const XEVE_FN_SSD xeve_tbl_ssd_16b_neon[8][8] =
{
    /* width == 1 */
    {
        ssd_16b, /* height == 1 */
        ssd_16b, /* height == 2 */
        ssd_16b, /* height == 4 */
        ssd_16b, /* height == 8 */
        ssd_16b, /* height == 16 */
        ssd_16b, /* height == 32 */
        ssd_16b, /* height == 64 */
        ssd_16b, /* height == 128 */
    },
    /* width == 2 */
    {
        ssd_16b, /* height == 1 */
        ssd_16b, /* height == 2 */
        ssd_16b, /* height == 4 */
        ssd_16b, /* height == 8 */
        ssd_16b, /* height == 16 */
        ssd_16b, /* height == 32 */
        ssd_16b, /* height == 64 */
        ssd_16b, /* height == 128 */
    },
    /* width == 4 */
    {
        ssd_16b, /* height == 1 */
        ssd_16b_neon_4x2,  /* height == 2 */
        ssd_16b_neon_4x4,  /* height == 4 */
        ssd_16b_neon_4x8,  /* height == 8 */
        ssd_16b_neon_4x16, /* height == 16 */
        ssd_16b_neon_4x32, /* height == 32 */
        ssd_16b, /* height == 64 */
        ssd_16b, /* height == 128 */
    },
    /* width == 8 */
    {
        ssd_16b,  /* height == 1 */
        ssd_16b_neon_8x2,   /* height == 2 */
        ssd_16b_neon_8x4,   /* height == 4 */
        ssd_16b_neon_8x8,   /* height == 8 */
        ssd_16b_neon_8nx8n, /* height == 16 */
        ssd_16b_neon_8nx8n, /* height == 32 */
        ssd_16b_neon_8nx8n, /* height == 64 */
        ssd_16b_neon_8nx8n, /* height == 128 */
    },
    /* width == 16 */
    {
        ssd_16b,  /* height == 1 */
        ssd_16b_neon_8nx2n, /* height == 2 */
        ssd_16b_neon_8nx4n, /* height == 4 */
        ssd_16b_neon_8nx8n, /* height == 8 */
        ssd_16b_neon_8nx8n, /* height == 16 */
        ssd_16b_neon_8nx8n, /* height == 32 */
        ssd_16b_neon_8nx8n, /* height == 64 */
        ssd_16b_neon_8nx8n, /* height == 128 */
    },
    /* width == 32 */
    {
        ssd_16b,  /* height == 1 */
        ssd_16b_neon_8nx2n, /* height == 2 */
        ssd_16b_neon_8nx4n, /* height == 4 */
        ssd_16b_neon_8nx8n, /* height == 8 */
        ssd_16b_neon_8nx8n, /* height == 16 */
        ssd_16b_neon_8nx8n, /* height == 32 */
        ssd_16b_neon_8nx8n, /* height == 64 */
        ssd_16b_neon_8nx8n, /* height == 128 */
    },
    /* width == 64 */
    {
        ssd_16b,  /* height == 1 */
        ssd_16b,  /* height == 2 */
        ssd_16b_neon_8nx4n, /* height == 4 */
        ssd_16b_neon_8nx8n, /* height == 8 */
        ssd_16b_neon_8nx8n, /* height == 16 */
        ssd_16b_neon_8nx8n, /* height == 32 */
        ssd_16b_neon_8nx8n, /* height == 64 */
        ssd_16b_neon_8nx8n, /* height == 128 */
    },
    /* width == 128 */
    {
        ssd_16b,  /* height == 1 */
        ssd_16b_neon_8nx2n, /* height == 2 */
        ssd_16b_neon_8nx4n, /* height == 4 */
        ssd_16b_neon_8nx8n, /* height == 8 */
        ssd_16b_neon_8nx8n, /* height == 16 */
        ssd_16b_neon_8nx8n, /* height == 32 */
        ssd_16b_neon_8nx8n, /* height == 64 */
        ssd_16b_neon_8nx8n, /* height == 128 */
    }
};

// clang-format on

/* SATD **********************************************************************/
int xeve_had_4x4_neon(pel* org, pel* cur, int s_org, int s_cur, int step, int bit_depth)
{
    if(bit_depth == 10) {
        int         satd = 0;
        int16x4_t   r0_16x4, r1_16x4, r2_16x4, r3_16x4, r4_16x4, r5_16x4, r6_16x4, r7_16x4;
        int16x4x2_t r0_16x4x2, r2_16x4x2;
        int16x8_t   r0_16x8, r1_16x8, r2_16x8, r3_16x8, r4_16x8, r5_16x8;

        // load 16x4
        r0_16x4 = (vld1_s16(&org[0]));
        r1_16x4 = (vld1_s16(&org[s_org]));
        r2_16x4 = (vld1_s16(&org[2 * s_org]));
        r3_16x4 = (vld1_s16(&org[3 * s_org]));
        r4_16x4 = (vld1_s16(&cur[0]));
        r5_16x4 = (vld1_s16(&cur[s_cur]));
        r6_16x4 = (vld1_s16(&cur[2 * s_cur]));
        r7_16x4 = (vld1_s16(&cur[3 * s_cur]));

        r0_16x4 = vsub_s16(r0_16x4, r4_16x4);
        r1_16x4 = vsub_s16(r1_16x4, r5_16x4);
        r2_16x4 = vsub_s16(r2_16x4, r6_16x4);
        r3_16x4 = vsub_s16(r3_16x4, r7_16x4);

        r4_16x4 = r0_16x4;
        r5_16x4 = r1_16x4;

        r0_16x4 = vadd_s16(r0_16x4, r3_16x4);
        r1_16x4 = vadd_s16(r1_16x4, r2_16x4);
        r4_16x4 = vsub_s16(r4_16x4, r3_16x4);
        r5_16x4 = vsub_s16(r5_16x4, r2_16x4);

        r2_16x4 = r0_16x4;
        r3_16x4 = r4_16x4;

        r0_16x4 = vadd_s16(r0_16x4, r1_16x4);
        r2_16x4 = vsub_s16(r2_16x4, r1_16x4);
        r3_16x4 = vsub_s16(r3_16x4, r5_16x4);
        r5_16x4 = vadd_s16(r5_16x4, r4_16x4);

        r0_16x4x2 = vzip_s16(r0_16x4, r5_16x4);
        r0_16x8   = vcombine_s16(r0_16x4x2.val[0], r0_16x4x2.val[1]);

        r2_16x4x2 = vzip_s16(r2_16x4, r3_16x4);
        r2_16x8   = vcombine_s16(r2_16x4x2.val[0], r2_16x4x2.val[1]);

        r3_16x8 = r0_16x8;
        r0_16x8 = vreinterpretq_s16_s32(vzip1q_s32(vreinterpretq_s32_s16(r0_16x8), vreinterpretq_s32_s16(r2_16x8)));
        r3_16x8 = vreinterpretq_s16_s32(vzip2q_s32(vreinterpretq_s32_s16(r3_16x8), vreinterpretq_s32_s16(r2_16x8)));

        r1_16x8 = r0_16x8;
        r2_16x8 = r3_16x8;

        r1_16x8 = vcombine_s16(vget_high_s16(r1_16x8), vcreate_s32(0));
        r3_16x8 = vcombine_s16(vget_high_s16(r3_16x8), vcreate_s32(0));

        // second stage
        r4_16x8 = r0_16x8;
        r5_16x8 = r1_16x8;

        r0_16x8 = vaddq_s16(r0_16x8, r3_16x8);
        r1_16x8 = vaddq_s16(r1_16x8, r2_16x8);

        r4_16x8 = vsubq_s16(r4_16x8, r3_16x8);
        r5_16x8 = vsubq_s16(r5_16x8, r2_16x8);

        r2_16x8 = r0_16x8;
        r3_16x8 = r4_16x8;

        r0_16x8 = vaddq_s16(r0_16x8, r1_16x8);
        r2_16x8 = vsubq_s16(r2_16x8, r1_16x8);
        r3_16x8 = vsubq_s16(r3_16x8, r5_16x8);
        r5_16x8 = vaddq_s16(r5_16x8, r4_16x8);
        // abs
        r0_16x8 = vabsq_s16(r0_16x8);

        s16* p = (s16*)&r0_16x8;
        p[0]   = p[0] >> 2;

        r2_16x8 = vabsq_s16(r2_16x8);
        r3_16x8 = vabsq_s16(r3_16x8);
        r5_16x8 = vabsq_s16(r5_16x8);

        r0_16x8 = vaddq_s16(r0_16x8, r2_16x8);
        r0_16x8 = vaddq_s16(r0_16x8, r3_16x8);
        r0_16x8 = vaddq_s16(r0_16x8, r5_16x8);

        satd = vaddv_s16(vget_low_s16(r0_16x8));
        satd = ((satd + 1) >> 1);

        return satd;
    }
    else {
        int  k;
        int  satd = 0;
        int  subs[16], interm1[16], interm2[16];
        pel* orgn = org;
        pel* curn = cur;

        for(k = 0; k < 16; k += 4) {
            subs[k + 0] = orgn[0] - curn[0];
            subs[k + 1] = orgn[1] - curn[1];
            subs[k + 2] = orgn[2] - curn[2];
            subs[k + 3] = orgn[3] - curn[3];
            curn += s_cur;
            orgn += s_org;
        }

        interm1[0]  = subs[0] + subs[12];
        interm1[1]  = subs[1] + subs[13];
        interm1[2]  = subs[2] + subs[14];
        interm1[3]  = subs[3] + subs[15];
        interm1[4]  = subs[4] + subs[8];
        interm1[5]  = subs[5] + subs[9];
        interm1[6]  = subs[6] + subs[10];
        interm1[7]  = subs[7] + subs[11];
        interm1[8]  = subs[4] - subs[8];
        interm1[9]  = subs[5] - subs[9];
        interm1[10] = subs[6] - subs[10];
        interm1[11] = subs[7] - subs[11];
        interm1[12] = subs[0] - subs[12];
        interm1[13] = subs[1] - subs[13];
        interm1[14] = subs[2] - subs[14];
        interm1[15] = subs[3] - subs[15];

        interm2[0]  = interm1[0] + interm1[4];
        interm2[1]  = interm1[1] + interm1[5];
        interm2[2]  = interm1[2] + interm1[6];
        interm2[3]  = interm1[3] + interm1[7];
        interm2[4]  = interm1[8] + interm1[12];
        interm2[5]  = interm1[9] + interm1[13];
        interm2[6]  = interm1[10] + interm1[14];
        interm2[7]  = interm1[11] + interm1[15];
        interm2[8]  = interm1[0] - interm1[4];
        interm2[9]  = interm1[1] - interm1[5];
        interm2[10] = interm1[2] - interm1[6];
        interm2[11] = interm1[3] - interm1[7];
        interm2[12] = interm1[12] - interm1[8];
        interm2[13] = interm1[13] - interm1[9];
        interm2[14] = interm1[14] - interm1[10];
        interm2[15] = interm1[15] - interm1[11];

        interm1[0]  = interm2[0] + interm2[3];
        interm1[1]  = interm2[1] + interm2[2];
        interm1[2]  = interm2[1] - interm2[2];
        interm1[3]  = interm2[0] - interm2[3];
        interm1[4]  = interm2[4] + interm2[7];
        interm1[5]  = interm2[5] + interm2[6];
        interm1[6]  = interm2[5] - interm2[6];
        interm1[7]  = interm2[4] - interm2[7];
        interm1[8]  = interm2[8] + interm2[11];
        interm1[9]  = interm2[9] + interm2[10];
        interm1[10] = interm2[9] - interm2[10];
        interm1[11] = interm2[8] - interm2[11];
        interm1[12] = interm2[12] + interm2[15];
        interm1[13] = interm2[13] + interm2[14];
        interm1[14] = interm2[13] - interm2[14];
        interm1[15] = interm2[12] - interm2[15];

        interm2[0]  = XEVE_ABS(interm1[0] + interm1[1]);
        interm2[1]  = XEVE_ABS(interm1[0] - interm1[1]);
        interm2[2]  = XEVE_ABS(interm1[2] + interm1[3]);
        interm2[3]  = XEVE_ABS(interm1[3] - interm1[2]);
        interm2[4]  = XEVE_ABS(interm1[4] + interm1[5]);
        interm2[5]  = XEVE_ABS(interm1[4] - interm1[5]);
        interm2[6]  = XEVE_ABS(interm1[6] + interm1[7]);
        interm2[7]  = XEVE_ABS(interm1[7] - interm1[6]);
        interm2[8]  = XEVE_ABS(interm1[8] + interm1[9]);
        interm2[9]  = XEVE_ABS(interm1[8] - interm1[9]);
        interm2[10] = XEVE_ABS(interm1[10] + interm1[11]);
        interm2[11] = XEVE_ABS(interm1[11] - interm1[10]);
        interm2[12] = XEVE_ABS(interm1[12] + interm1[13]);
        interm2[13] = XEVE_ABS(interm1[12] - interm1[13]);
        interm2[14] = XEVE_ABS(interm1[14] + interm1[15]);
        interm2[15] = XEVE_ABS(interm1[15] - interm1[14]);

        satd = (interm2[0] >> 2);
        for(k = 1; k < 16; k++) {
            satd += interm2[k];
        }
        satd = ((satd + 1) >> 1);
        return satd;
    }
}

int xeve_had_8x8_neon(pel* org, pel* cur, int s_org, int s_cur, int step, int bit_depth)
{
    if(bit_depth == 10) {
        int satd = 0;
        /* all 128 bit registers are named with a suffix mxnb, where m is the */
        /* number of n bits packed in the register                            */

        int16x8_t   src0_8x16b, src1_8x16b, src2_8x16b, src3_8x16b;
        int16x8_t   src4_8x16b, src5_8x16b, src6_8x16b, src7_8x16b;
        int16x8_t   pred0_8x16b, pred1_8x16b, pred2_8x16b, pred3_8x16b;
        int16x8_t   pred4_8x16b, pred5_8x16b, pred6_8x16b, pred7_8x16b;
        int16x8_t   out0_8x16b, out1_8x16b, out2_8x16b, out3_8x16b;
        int16x8_t   out4_8x16b, out5_8x16b, out6_8x16b, out7_8x16b;
        int16x8x2_t out0_8x16bx2, out1_8x16bx2, out2_8x16bx2, out3_8x16bx2;

        /**********************Residue Calculation********************************/

        src0_8x16b = (vld1q_s16(&org[0]));
        org        = org + s_org;
        src1_8x16b = (vld1q_s16(&org[0]));
        org        = org + s_org;
        src2_8x16b = (vld1q_s16(&org[0]));
        org        = org + s_org;
        src3_8x16b = (vld1q_s16(&org[0]));
        org        = org + s_org;
        src4_8x16b = (vld1q_s16(&org[0]));
        org        = org + s_org;
        src5_8x16b = (vld1q_s16(&org[0]));
        org        = org + s_org;
        src6_8x16b = (vld1q_s16(&org[0]));
        org        = org + s_org;
        src7_8x16b = (vld1q_s16(&org[0]));
        org        = org + s_org;

        pred0_8x16b = (vld1q_s16(&cur[0]));
        cur         = cur + s_cur;
        pred1_8x16b = (vld1q_s16(&cur[0]));
        cur         = cur + s_cur;
        pred2_8x16b = (vld1q_s16(&cur[0]));
        cur         = cur + s_cur;
        pred3_8x16b = (vld1q_s16(&cur[0]));
        cur         = cur + s_cur;
        pred4_8x16b = (vld1q_s16(&cur[0]));
        cur         = cur + s_cur;
        pred5_8x16b = (vld1q_s16(&cur[0]));
        cur         = cur + s_cur;
        pred6_8x16b = (vld1q_s16(&cur[0]));
        cur         = cur + s_cur;
        pred7_8x16b = (vld1q_s16(&cur[0]));
        cur         = cur + s_cur;

        src0_8x16b = vsubq_s16(src0_8x16b, pred0_8x16b);
        src1_8x16b = vsubq_s16(src1_8x16b, pred1_8x16b);
        src2_8x16b = vsubq_s16(src2_8x16b, pred2_8x16b);
        src3_8x16b = vsubq_s16(src3_8x16b, pred3_8x16b);
        src4_8x16b = vsubq_s16(src4_8x16b, pred4_8x16b);
        src5_8x16b = vsubq_s16(src5_8x16b, pred5_8x16b);
        src6_8x16b = vsubq_s16(src6_8x16b, pred6_8x16b);
        src7_8x16b = vsubq_s16(src7_8x16b, pred7_8x16b);

        /**********************Residue Calculation********************************/

        /**************** 8x8 horizontal transform *******************************/
        /***********************    8x8 16 bit Transpose  ************************/

        out3_8x16b = vcombine_s16(vget_low_s16(src0_8x16b), vget_low_s16(src1_8x16b));
        out7_8x16b = vcombine_s16(vget_high_s16(src0_8x16b), vget_high_s16(src1_8x16b));

        pred0_8x16b = vcombine_s16(vget_low_s16(src2_8x16b), vget_low_s16(src3_8x16b));
        src2_8x16b  = vcombine_s16(vget_high_s16(src2_8x16b), vget_high_s16(src3_8x16b));

        out2_8x16b  = vcombine_s16(vget_low_s16(src4_8x16b), vget_low_s16(src5_8x16b));
        pred7_8x16b = vcombine_s16(vget_high_s16(src4_8x16b), vget_high_s16(src5_8x16b));

        pred3_8x16b = vcombine_s16(vget_low_s16(src6_8x16b), vget_low_s16(src7_8x16b));
        src6_8x16b  = vcombine_s16(vget_high_s16(src6_8x16b), vget_high_s16(src7_8x16b));

        out1_8x16b = vzip1q_s32(out3_8x16b, pred0_8x16b);
        out3_8x16b = vzip2q_s32(out3_8x16b, pred0_8x16b);

        pred1_8x16b = vzip1q_s32(out2_8x16b, pred3_8x16b);
        pred3_8x16b = vzip2q_s32(out2_8x16b, pred3_8x16b);

        out5_8x16b = vzip1q_s32(out7_8x16b, src2_8x16b);
        out7_8x16b = vzip2q_s32(out7_8x16b, src2_8x16b);

        pred5_8x16b = vzip1q_s32(pred7_8x16b, src6_8x16b);
        pred7_8x16b = vzip2q_s32(pred7_8x16b, src6_8x16b);

        out0_8x16b = vzip1q_s64(out1_8x16b, pred1_8x16b);
        out1_8x16b = vzip2q_s64(out1_8x16b, pred1_8x16b);
        out2_8x16b = vzip1q_s64(out3_8x16b, pred3_8x16b);
        out3_8x16b = vzip2q_s64(out3_8x16b, pred3_8x16b);
        out4_8x16b = vzip1q_s64(out5_8x16b, pred5_8x16b);
        out5_8x16b = vzip2q_s64(out5_8x16b, pred5_8x16b);
        out6_8x16b = vzip1q_s64(out7_8x16b, pred7_8x16b);
        out7_8x16b = vzip2q_s64(out7_8x16b, pred7_8x16b);

        /**********************   8x8 16 bit Transpose End   *********************/

        /* r0 + r1 */
        pred0_8x16b = vaddq_s16(out0_8x16b, out1_8x16b);
        /* r2 + r3 */
        pred2_8x16b = vaddq_s16(out2_8x16b, out3_8x16b);
        /* r4 + r5 */
        pred4_8x16b = vaddq_s16(out4_8x16b, out5_8x16b);
        /* r6 + r7 */
        pred6_8x16b = vaddq_s16(out6_8x16b, out7_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = vaddq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = vaddq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        src0_8x16b  = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        src4_8x16b  = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b = vsubq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b = vsubq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        src2_8x16b  = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        src6_8x16b  = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = vsubq_s16(out0_8x16b, out1_8x16b);
        /* r2 - r3 */
        pred2_8x16b = vsubq_s16(out2_8x16b, out3_8x16b);
        /* r4 - r5 */
        pred4_8x16b = vsubq_s16(out4_8x16b, out5_8x16b);
        /* r6 - r7 */
        pred6_8x16b = vsubq_s16(out6_8x16b, out7_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b = vaddq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b = vaddq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        src1_8x16b  = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        src5_8x16b  = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b = vsubq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b = vsubq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        src3_8x16b  = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        src7_8x16b  = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /***********************    8x8 16 bit Transpose  ************************/
        out3_8x16b  = vzip1q_s16(src0_8x16b, src1_8x16b);
        pred0_8x16b = vzip1q_s16(src2_8x16b, src3_8x16b);
        out2_8x16b  = vzip1q_s16(src4_8x16b, src5_8x16b);
        pred3_8x16b = vzip1q_s16(src6_8x16b, src7_8x16b);
        out7_8x16b  = vzip2q_s16(src0_8x16b, src1_8x16b);
        src2_8x16b  = vzip2q_s16(src2_8x16b, src3_8x16b);
        pred7_8x16b = vzip2q_s16(src4_8x16b, src5_8x16b);
        src6_8x16b  = vzip2q_s16(src6_8x16b, src7_8x16b);

        out1_8x16b = vzip1q_s32(out3_8x16b, pred0_8x16b);
        out3_8x16b = vzip2q_s32(out3_8x16b, pred0_8x16b);

        pred1_8x16b = vzip1q_s32(out2_8x16b, pred3_8x16b);
        pred3_8x16b = vzip2q_s32(out2_8x16b, pred3_8x16b);

        out5_8x16b = vzip1q_s32(out7_8x16b, src2_8x16b);
        out7_8x16b = vzip2q_s32(out7_8x16b, src2_8x16b);

        pred5_8x16b = vzip1q_s32(pred7_8x16b, src6_8x16b);
        pred7_8x16b = vzip2q_s32(pred7_8x16b, src6_8x16b);

        src0_8x16b = vzip1q_s64(out1_8x16b, pred1_8x16b);
        src1_8x16b = vzip2q_s64(out1_8x16b, pred1_8x16b);
        src2_8x16b = vzip1q_s64(out3_8x16b, pred3_8x16b);
        src3_8x16b = vzip2q_s64(out3_8x16b, pred3_8x16b);
        src4_8x16b = vzip1q_s64(out5_8x16b, pred5_8x16b);
        src5_8x16b = vzip2q_s64(out5_8x16b, pred5_8x16b);
        src6_8x16b = vzip1q_s64(out7_8x16b, pred7_8x16b);
        src7_8x16b = vzip2q_s64(out7_8x16b, pred7_8x16b);

        /**********************   8x8 16 bit Transpose End   *********************/
        /**************** 8x8 horizontal transform *******************************/
        int16x8_t out0a_8x16b, out1a_8x16b, out2a_8x16b, out3a_8x16b;
        int16x8_t out4a_8x16b, out5a_8x16b, out6a_8x16b, out7a_8x16b;
        int16x8_t tmp0_8x16b, tmp1_8x16b, tmp2_8x16b, tmp3_8x16b;
        int16x8_t tmp4_8x16b, tmp5_8x16b, tmp6_8x16b, tmp7_8x16b;

        /************************* 8x8 Vertical Transform*************************/
        tmp0_8x16b = vcombine_s16(vget_high_s16(src0_8x16b), vcreate_s32(0));
        tmp1_8x16b = vcombine_s16(vget_high_s16(src1_8x16b), vcreate_s32(0));
        tmp2_8x16b = vcombine_s16(vget_high_s16(src2_8x16b), vcreate_s32(0));
        tmp3_8x16b = vcombine_s16(vget_high_s16(src3_8x16b), vcreate_s32(0));
        tmp4_8x16b = vcombine_s16(vget_high_s16(src4_8x16b), vcreate_s32(0));
        tmp5_8x16b = vcombine_s16(vget_high_s16(src5_8x16b), vcreate_s32(0));
        tmp6_8x16b = vcombine_s16(vget_high_s16(src6_8x16b), vcreate_s32(0));
        tmp7_8x16b = vcombine_s16(vget_high_s16(src7_8x16b), vcreate_s32(0));

        /*************************First 4 pixels ********************************/

        src0_8x16b = vmovl_s16(vget_low_s16(src0_8x16b));
        src1_8x16b = vmovl_s16(vget_low_s16(src1_8x16b));
        src2_8x16b = vmovl_s16(vget_low_s16(src2_8x16b));
        src3_8x16b = vmovl_s16(vget_low_s16(src3_8x16b));
        src4_8x16b = vmovl_s16(vget_low_s16(src4_8x16b));
        src5_8x16b = vmovl_s16(vget_low_s16(src5_8x16b));
        src6_8x16b = vmovl_s16(vget_low_s16(src6_8x16b));
        src7_8x16b = vmovl_s16(vget_low_s16(src7_8x16b));

        /* r0 + r1 */
        pred0_8x16b = vaddq_s32(src0_8x16b, src1_8x16b);
        /* r2 + r3 */
        pred2_8x16b = vaddq_s32(src2_8x16b, src3_8x16b);
        /* r4 + r5 */
        pred4_8x16b = vaddq_s32(src4_8x16b, src5_8x16b);
        /* r6 + r7 */
        pred6_8x16b = vaddq_s32(src6_8x16b, src7_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        out0_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        out4_8x16b  = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        out2_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        out6_8x16b  = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = vsubq_s32(src0_8x16b, src1_8x16b);
        /* r2 - r3 */
        pred2_8x16b = vsubq_s32(src2_8x16b, src3_8x16b);
        /* r4 - r5 */
        pred4_8x16b = vsubq_s32(src4_8x16b, src5_8x16b);
        /* r6 - r7 */
        pred6_8x16b = vsubq_s32(src6_8x16b, src7_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        out1_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        out5_8x16b  = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        out3_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        out7_8x16b  = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /*************************First 4 pixels ********************************/

        /**************************Next 4 pixels *******************************/
        src0_8x16b = vmovl_s16(vget_low_s16(tmp0_8x16b));
        src1_8x16b = vmovl_s16(vget_low_s16(tmp1_8x16b));
        src2_8x16b = vmovl_s16(vget_low_s16(tmp2_8x16b));
        src3_8x16b = vmovl_s16(vget_low_s16(tmp3_8x16b));
        src4_8x16b = vmovl_s16(vget_low_s16(tmp4_8x16b));
        src5_8x16b = vmovl_s16(vget_low_s16(tmp5_8x16b));
        src6_8x16b = vmovl_s16(vget_low_s16(tmp6_8x16b));
        src7_8x16b = vmovl_s16(vget_low_s16(tmp7_8x16b));

        /* r0 + r1 */
        pred0_8x16b = vaddq_s32(src0_8x16b, src1_8x16b);
        /* r2 + r3 */
        pred2_8x16b = vaddq_s32(src2_8x16b, src3_8x16b);
        /* r4 + r5 */
        pred4_8x16b = vaddq_s32(src4_8x16b, src5_8x16b);
        /* r6 + r7 */
        pred6_8x16b = vaddq_s32(src6_8x16b, src7_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        out0a_8x16b = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        out4a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        out2a_8x16b = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        out6a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = vsubq_s32(src0_8x16b, src1_8x16b);
        /* r2 - r3 */
        pred2_8x16b = vsubq_s32(src2_8x16b, src3_8x16b);
        /* r4 - r5 */
        pred4_8x16b = vsubq_s32(src4_8x16b, src5_8x16b);
        /* r6 - r7 */
        pred6_8x16b = vsubq_s32(src6_8x16b, src7_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        out1a_8x16b = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        out5a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        out3a_8x16b = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        out7a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /**************************Next 4 pixels *******************************/
        /************************* 8x8 Vertical Transform*************************/

        /****************************SATD calculation ****************************/
        src0_8x16b = vabsq_s32(out0_8x16b);
        src1_8x16b = vabsq_s32(out1_8x16b);
        src2_8x16b = vabsq_s32(out2_8x16b);
        src3_8x16b = vabsq_s32(out3_8x16b);
        src4_8x16b = vabsq_s32(out4_8x16b);
        src5_8x16b = vabsq_s32(out5_8x16b);
        src6_8x16b = vabsq_s32(out6_8x16b);
        src7_8x16b = vabsq_s32(out7_8x16b);
        s32* p     = (s32*)&src0_8x16b;
        p[0]       = p[0] >> 2;

        satd = vaddvq_s32(src0_8x16b);
        satd += vaddvq_s32(src1_8x16b);
        satd += vaddvq_s32(src2_8x16b);
        satd += vaddvq_s32(src3_8x16b);
        satd += vaddvq_s32(src4_8x16b);
        satd += vaddvq_s32(src5_8x16b);
        satd += vaddvq_s32(src6_8x16b);
        satd += vaddvq_s32(src7_8x16b);

        src0_8x16b = vabsq_s32(out0a_8x16b);
        src1_8x16b = vabsq_s32(out1a_8x16b);
        src2_8x16b = vabsq_s32(out2a_8x16b);
        src3_8x16b = vabsq_s32(out3a_8x16b);
        src4_8x16b = vabsq_s32(out4a_8x16b);
        src5_8x16b = vabsq_s32(out5a_8x16b);
        src6_8x16b = vabsq_s32(out6a_8x16b);
        src7_8x16b = vabsq_s32(out7a_8x16b);

        satd += vaddvq_s32(src0_8x16b);
        satd += vaddvq_s32(src1_8x16b);
        satd += vaddvq_s32(src2_8x16b);
        satd += vaddvq_s32(src3_8x16b);
        satd += vaddvq_s32(src4_8x16b);
        satd += vaddvq_s32(src5_8x16b);
        satd += vaddvq_s32(src6_8x16b);
        satd += vaddvq_s32(src7_8x16b);

        satd = (satd + 2) >> 2;
        return satd;
    }
    else {
        int  k, i, j, jj;
        int  satd = 0;
        int  sub[64], interm1[8][8], interm2[8][8], interm3[8][8];
        pel *orgn = org, *curn = cur;

        for(k = 0; k < 64; k += 8) {
            sub[k + 0] = orgn[0] - curn[0];
            sub[k + 1] = orgn[1] - curn[1];
            sub[k + 2] = orgn[2] - curn[2];
            sub[k + 3] = orgn[3] - curn[3];
            sub[k + 4] = orgn[4] - curn[4];
            sub[k + 5] = orgn[5] - curn[5];
            sub[k + 6] = orgn[6] - curn[6];
            sub[k + 7] = orgn[7] - curn[7];

            curn += s_cur;
            orgn += s_org;
        }

        /* horizontal */
        for(j = 0; j < 8; j++) {
            jj            = j << 3;
            interm2[j][0] = sub[jj] + sub[jj + 4];
            interm2[j][1] = sub[jj + 1] + sub[jj + 5];
            interm2[j][2] = sub[jj + 2] + sub[jj + 6];
            interm2[j][3] = sub[jj + 3] + sub[jj + 7];
            interm2[j][4] = sub[jj] - sub[jj + 4];
            interm2[j][5] = sub[jj + 1] - sub[jj + 5];
            interm2[j][6] = sub[jj + 2] - sub[jj + 6];
            interm2[j][7] = sub[jj + 3] - sub[jj + 7];

            interm1[j][0] = interm2[j][0] + interm2[j][2];
            interm1[j][1] = interm2[j][1] + interm2[j][3];
            interm1[j][2] = interm2[j][0] - interm2[j][2];
            interm1[j][3] = interm2[j][1] - interm2[j][3];
            interm1[j][4] = interm2[j][4] + interm2[j][6];
            interm1[j][5] = interm2[j][5] + interm2[j][7];
            interm1[j][6] = interm2[j][4] - interm2[j][6];
            interm1[j][7] = interm2[j][5] - interm2[j][7];

            interm2[j][0] = interm1[j][0] + interm1[j][1];
            interm2[j][1] = interm1[j][0] - interm1[j][1];
            interm2[j][2] = interm1[j][2] + interm1[j][3];
            interm2[j][3] = interm1[j][2] - interm1[j][3];
            interm2[j][4] = interm1[j][4] + interm1[j][5];
            interm2[j][5] = interm1[j][4] - interm1[j][5];
            interm2[j][6] = interm1[j][6] + interm1[j][7];
            interm2[j][7] = interm1[j][6] - interm1[j][7];
        }

        /* vertical */
        for(i = 0; i < 8; i++) {
            interm3[0][i] = interm2[0][i] + interm2[4][i];
            interm3[1][i] = interm2[1][i] + interm2[5][i];
            interm3[2][i] = interm2[2][i] + interm2[6][i];
            interm3[3][i] = interm2[3][i] + interm2[7][i];
            interm3[4][i] = interm2[0][i] - interm2[4][i];
            interm3[5][i] = interm2[1][i] - interm2[5][i];
            interm3[6][i] = interm2[2][i] - interm2[6][i];
            interm3[7][i] = interm2[3][i] - interm2[7][i];

            interm1[0][i] = interm3[0][i] + interm3[2][i];
            interm1[1][i] = interm3[1][i] + interm3[3][i];
            interm1[2][i] = interm3[0][i] - interm3[2][i];
            interm1[3][i] = interm3[1][i] - interm3[3][i];
            interm1[4][i] = interm3[4][i] + interm3[6][i];
            interm1[5][i] = interm3[5][i] + interm3[7][i];
            interm1[6][i] = interm3[4][i] - interm3[6][i];
            interm1[7][i] = interm3[5][i] - interm3[7][i];

            interm2[0][i] = XEVE_ABS(interm1[0][i] + interm1[1][i]);
            interm2[1][i] = XEVE_ABS(interm1[0][i] - interm1[1][i]);
            interm2[2][i] = XEVE_ABS(interm1[2][i] + interm1[3][i]);
            interm2[3][i] = XEVE_ABS(interm1[2][i] - interm1[3][i]);
            interm2[4][i] = XEVE_ABS(interm1[4][i] + interm1[5][i]);
            interm2[5][i] = XEVE_ABS(interm1[4][i] - interm1[5][i]);
            interm2[6][i] = XEVE_ABS(interm1[6][i] + interm1[7][i]);
            interm2[7][i] = XEVE_ABS(interm1[6][i] - interm1[7][i]);
        }

        satd = interm2[0][0] >> 2;
        for(j = 1; j < 8; j++) {
            satd += interm2[0][j];
        }
        for(i = 1; i < 8; i++) {
            for(j = 0; j < 8; j++) {
                satd += interm2[i][j];
            }
        }

        satd = ((satd + 2) >> 2);
        return satd;
    }
}

int xeve_had_16x8_neon(pel* org, pel* cur, int s_org, int s_cur, int step, int bit_depth)
{
    if(bit_depth == 10) {
        int         satd = 0;
        int16x8x2_t out0_8x16bx2, out1_8x16bx2, out2_8x16bx2, out3_8x16bx2;

        /* all 128 bit registers are named with a suffix mxnb, where m is the */
        /* number of n bits packed in the register                            */
        int16x8_t src0_8x16b, src1_8x16b, src2_8x16b, src3_8x16b;
        int16x8_t src4_8x16b, src5_8x16b, src6_8x16b, src7_8x16b;
        int16x8_t src8_8x16b, src9_8x16b, src10_8x16b, src11_8x16b;
        int16x8_t src12_8x16b, src13_8x16b, src14_8x16b, src15_8x16b;
        int16x8_t pred0_8x16b, pred1_8x16b, pred2_8x16b, pred3_8x16b;
        int16x8_t pred4_8x16b, pred5_8x16b, pred6_8x16b, pred7_8x16b;
        int16x8_t pred8_8x16b, pred9_8x16b, pred10_8x16b, pred11_8x16b;
        int16x8_t pred12_8x16b, pred13_8x16b, pred14_8x16b, pred15_8x16b;
        int16x8_t out0_8x16b, out1_8x16b, out2_8x16b, out3_8x16b;
        int16x8_t out4_8x16b, out5_8x16b, out6_8x16b, out7_8x16b;
        int16x8_t out8_8x16b, out9_8x16b, out10_8x16b, out11_8x16b;
        int16x8_t out12_8x16b, out13_8x16b, out14_8x16b, out15_8x16b;

        /**********************Residue Calculation********************************/
        src0_8x16b = (vld1q_s16(&org[0]));
        src1_8x16b = (vld1q_s16(&org[8]));
        org        = org + s_org;
        src2_8x16b = (vld1q_s16(&org[0]));
        src3_8x16b = (vld1q_s16(&org[8]));
        org        = org + s_org;
        src4_8x16b = (vld1q_s16(&org[0]));
        src5_8x16b = (vld1q_s16(&org[8]));
        org        = org + s_org;
        src6_8x16b = (vld1q_s16(&org[0]));
        src7_8x16b = (vld1q_s16(&org[8]));
        org        = org + s_org;

        pred0_8x16b = (vld1q_s16(&cur[0]));
        pred1_8x16b = (vld1q_s16(&cur[8]));
        cur         = cur + s_cur;
        pred2_8x16b = (vld1q_s16(&cur[0]));
        pred3_8x16b = (vld1q_s16(&cur[8]));
        cur         = cur + s_cur;
        pred4_8x16b = (vld1q_s16(&cur[0]));
        pred5_8x16b = (vld1q_s16(&cur[8]));
        cur         = cur + s_cur;
        pred6_8x16b = (vld1q_s16(&cur[0]));
        pred7_8x16b = (vld1q_s16(&cur[8]));
        cur         = cur + s_cur;

        src0_8x16b = vsubq_s16(src0_8x16b, pred0_8x16b);
        src1_8x16b = vsubq_s16(src1_8x16b, pred1_8x16b);
        src2_8x16b = vsubq_s16(src2_8x16b, pred2_8x16b);
        src3_8x16b = vsubq_s16(src3_8x16b, pred3_8x16b);
        src4_8x16b = vsubq_s16(src4_8x16b, pred4_8x16b);
        src5_8x16b = vsubq_s16(src5_8x16b, pred5_8x16b);
        src6_8x16b = vsubq_s16(src6_8x16b, pred6_8x16b);
        src7_8x16b = vsubq_s16(src7_8x16b, pred7_8x16b);

        src8_8x16b  = (vld1q_s16(&org[0]));
        src9_8x16b  = (vld1q_s16(&org[8]));
        org         = org + s_org;
        src10_8x16b = (vld1q_s16(&org[0]));
        src11_8x16b = (vld1q_s16(&org[8]));
        org         = org + s_org;
        src12_8x16b = (vld1q_s16(&org[0]));
        src13_8x16b = (vld1q_s16(&org[8]));
        org         = org + s_org;
        src14_8x16b = (vld1q_s16(&org[0]));
        src15_8x16b = (vld1q_s16(&org[8]));
        org         = org + s_org;

        pred8_8x16b  = (vld1q_s16(&cur[0]));
        pred9_8x16b  = (vld1q_s16(&cur[8]));
        cur          = cur + s_cur;
        pred10_8x16b = (vld1q_s16(&cur[0]));
        pred11_8x16b = (vld1q_s16(&cur[8]));
        cur          = cur + s_cur;
        pred12_8x16b = (vld1q_s16(&cur[0]));
        pred13_8x16b = (vld1q_s16(&cur[8]));
        cur          = cur + s_cur;
        pred14_8x16b = (vld1q_s16(&cur[0]));
        pred15_8x16b = (vld1q_s16(&cur[8]));
        cur          = cur + s_cur;

        src8_8x16b  = vsubq_s16(src8_8x16b, pred8_8x16b);
        src9_8x16b  = vsubq_s16(src9_8x16b, pred9_8x16b);
        src10_8x16b = vsubq_s16(src10_8x16b, pred10_8x16b);
        src11_8x16b = vsubq_s16(src11_8x16b, pred11_8x16b);
        src12_8x16b = vsubq_s16(src12_8x16b, pred12_8x16b);
        src13_8x16b = vsubq_s16(src13_8x16b, pred13_8x16b);
        src14_8x16b = vsubq_s16(src14_8x16b, pred14_8x16b);
        src15_8x16b = vsubq_s16(src15_8x16b, pred15_8x16b);

        /**********************Residue Calculation********************************/

        /**************** 8x8 horizontal transform *******************************/
        /***********************    8x8 16 bit Transpose  ************************/

        out3_8x16b  = vzip1q_s16(src0_8x16b, src1_8x16b);
        pred0_8x16b = vzip1q_s16(src2_8x16b, src3_8x16b);
        out2_8x16b  = vzip1q_s16(src4_8x16b, src5_8x16b);
        pred3_8x16b = vzip1q_s16(src6_8x16b, src7_8x16b);
        out7_8x16b  = vzip2q_s16(src0_8x16b, src1_8x16b);
        src2_8x16b  = vzip2q_s16(src2_8x16b, src3_8x16b);
        pred7_8x16b = vzip2q_s16(src4_8x16b, src5_8x16b);
        src6_8x16b  = vzip2q_s16(src6_8x16b, src7_8x16b);

        out1_8x16b  = vzip1q_s32(out3_8x16b, pred0_8x16b);
        out3_8x16b  = vzip2q_s32(out3_8x16b, pred0_8x16b);
        pred1_8x16b = vzip1q_s32(out2_8x16b, pred3_8x16b);
        pred3_8x16b = vzip2q_s32(out2_8x16b, pred3_8x16b);
        out5_8x16b  = vzip1q_s32(out7_8x16b, src2_8x16b);
        out7_8x16b  = vzip2q_s32(out7_8x16b, src2_8x16b);
        pred5_8x16b = vzip1q_s32(pred7_8x16b, src6_8x16b);
        pred7_8x16b = vzip2q_s32(pred7_8x16b, src6_8x16b);

        out0_8x16b = vzip1q_s64(out1_8x16b, pred1_8x16b);
        out1_8x16b = vzip2q_s64(out1_8x16b, pred1_8x16b);
        out2_8x16b = vzip1q_s64(out3_8x16b, pred3_8x16b);
        out3_8x16b = vzip2q_s64(out3_8x16b, pred3_8x16b);
        out4_8x16b = vzip1q_s64(out5_8x16b, pred5_8x16b);
        out5_8x16b = vzip2q_s64(out5_8x16b, pred5_8x16b);
        out6_8x16b = vzip1q_s64(out7_8x16b, pred7_8x16b);
        out7_8x16b = vzip2q_s64(out7_8x16b, pred7_8x16b);

        /**********************   8x8 16 bit Transpose End   *********************/

        /* r0 + r1 */
        pred0_8x16b = vaddq_s16(out0_8x16b, out1_8x16b);
        /* r2 + r3 */
        pred2_8x16b = vaddq_s16(out2_8x16b, out3_8x16b);
        /* r4 + r5 */
        pred4_8x16b = vaddq_s16(out4_8x16b, out5_8x16b);
        /* r6 + r7 */
        pred6_8x16b = vaddq_s16(out6_8x16b, out7_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = vaddq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = vaddq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        src0_8x16b  = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        src4_8x16b  = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b = vsubq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b = vsubq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        src2_8x16b  = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        src6_8x16b  = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = vsubq_s16(out0_8x16b, out1_8x16b);
        /* r2 - r3 */
        pred2_8x16b = vsubq_s16(out2_8x16b, out3_8x16b);
        /* r4 - r5 */
        pred4_8x16b = vsubq_s16(out4_8x16b, out5_8x16b);
        /* r6 - r7 */
        pred6_8x16b = vsubq_s16(out6_8x16b, out7_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b = vaddq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b = vaddq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        src1_8x16b  = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        src5_8x16b  = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b = vsubq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b = vsubq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        src3_8x16b  = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        src7_8x16b  = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /***********************    8x8 16 bit Transpose  ************************/
        out3_8x16b  = vzip1q_s16(src0_8x16b, src1_8x16b);
        pred0_8x16b = vzip1q_s16(src2_8x16b, src3_8x16b);
        out2_8x16b  = vzip1q_s16(src4_8x16b, src5_8x16b);
        pred3_8x16b = vzip1q_s16(src6_8x16b, src7_8x16b);
        out7_8x16b  = vzip2q_s16(src0_8x16b, src1_8x16b);
        src2_8x16b  = vzip2q_s16(src2_8x16b, src3_8x16b);
        pred7_8x16b = vzip2q_s16(src4_8x16b, src5_8x16b);
        src6_8x16b  = vzip2q_s16(src6_8x16b, src7_8x16b);

        out1_8x16b  = vzip1q_s32(out3_8x16b, pred0_8x16b);
        out3_8x16b  = vzip2q_s32(out3_8x16b, pred0_8x16b);
        pred1_8x16b = vzip1q_s32(out2_8x16b, pred3_8x16b);
        pred3_8x16b = vzip2q_s32(out2_8x16b, pred3_8x16b);
        out5_8x16b  = vzip1q_s32(out7_8x16b, src2_8x16b);
        out7_8x16b  = vzip2q_s32(out7_8x16b, src2_8x16b);
        pred5_8x16b = vzip1q_s32(pred7_8x16b, src6_8x16b);
        pred7_8x16b = vzip2q_s32(pred7_8x16b, src6_8x16b);

        src0_8x16b = vzip1q_s64(out1_8x16b, pred1_8x16b);
        src1_8x16b = vzip2q_s64(out1_8x16b, pred1_8x16b);
        src2_8x16b = vzip1q_s64(out3_8x16b, pred3_8x16b);
        src3_8x16b = vzip2q_s64(out3_8x16b, pred3_8x16b);
        src4_8x16b = vzip1q_s64(out5_8x16b, pred5_8x16b);
        src5_8x16b = vzip2q_s64(out5_8x16b, pred5_8x16b);
        src6_8x16b = vzip1q_s64(out7_8x16b, pred7_8x16b);
        src7_8x16b = vzip2q_s64(out7_8x16b, pred7_8x16b);

        /**********************   8x8 16 bit Transpose End   *********************/
        /**************** 8x8 horizontal transform *******************************/

        /**************** 8x8 horizontal transform *******************************/
        /***********************    8x8 16 bit Transpose  ************************/
        out3_8x16b  = vzip1q_s16(src8_8x16b, src9_8x16b);
        pred0_8x16b = vzip1q_s16(src10_8x16b, src11_8x16b);
        out2_8x16b  = vzip1q_s16(src12_8x16b, src13_8x16b);
        pred3_8x16b = vzip1q_s16(src14_8x16b, src15_8x16b);
        out7_8x16b  = vzip2q_s16(src8_8x16b, src9_8x16b);
        src10_8x16b = vzip2q_s16(src10_8x16b, src11_8x16b);
        pred7_8x16b = vzip2q_s16(src12_8x16b, src13_8x16b);
        src14_8x16b = vzip2q_s16(src14_8x16b, src15_8x16b);

        out1_8x16b  = vzip1q_s32(out3_8x16b, pred0_8x16b);
        out3_8x16b  = vzip2q_s32(out3_8x16b, pred0_8x16b);
        pred1_8x16b = vzip1q_s32(out2_8x16b, pred3_8x16b);
        pred3_8x16b = vzip2q_s32(out2_8x16b, pred3_8x16b);
        out5_8x16b  = vzip1q_s32(out7_8x16b, src10_8x16b);
        out7_8x16b  = vzip2q_s32(out7_8x16b, src10_8x16b);
        pred5_8x16b = vzip1q_s32(pred7_8x16b, src14_8x16b);
        pred7_8x16b = vzip2q_s32(pred7_8x16b, src14_8x16b);

        out0_8x16b = vzip1q_s64(out1_8x16b, pred1_8x16b);
        out1_8x16b = vzip2q_s64(out1_8x16b, pred1_8x16b);
        out2_8x16b = vzip1q_s64(out3_8x16b, pred3_8x16b);
        out3_8x16b = vzip2q_s64(out3_8x16b, pred3_8x16b);
        out4_8x16b = vzip1q_s64(out5_8x16b, pred5_8x16b);
        out5_8x16b = vzip2q_s64(out5_8x16b, pred5_8x16b);
        out6_8x16b = vzip1q_s64(out7_8x16b, pred7_8x16b);
        out7_8x16b = vzip2q_s64(out7_8x16b, pred7_8x16b);

        /**********************   8x8 16 bit Transpose End   *********************/

        /* r0 + r1 */
        pred0_8x16b = vaddq_s16(out0_8x16b, out1_8x16b);
        /* r2 + r3 */
        pred2_8x16b = vaddq_s16(out2_8x16b, out3_8x16b);
        /* r4 + r5 */
        pred4_8x16b = vaddq_s16(out4_8x16b, out5_8x16b);
        /* r6 + r7 */
        pred6_8x16b = vaddq_s16(out6_8x16b, out7_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = vaddq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = vaddq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        src8_8x16b  = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        src12_8x16b = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b = vsubq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b = vsubq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        src10_8x16b = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        src14_8x16b = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = vsubq_s16(out0_8x16b, out1_8x16b);
        /* r2 - r3 */
        pred2_8x16b = vsubq_s16(out2_8x16b, out3_8x16b);
        /* r4 - r5 */
        pred4_8x16b = vsubq_s16(out4_8x16b, out5_8x16b);
        /* r6 - r7 */
        pred6_8x16b = vsubq_s16(out6_8x16b, out7_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b = vaddq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b = vaddq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        src9_8x16b  = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        src13_8x16b = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b = vsubq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b = vsubq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        src11_8x16b = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        src15_8x16b = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /***********************    8x8 16 bit Transpose  ************************/
        out3_8x16b  = vzip1q_s16(src8_8x16b, src9_8x16b);
        pred0_8x16b = vzip1q_s16(src10_8x16b, src11_8x16b);
        out2_8x16b  = vzip1q_s16(src12_8x16b, src13_8x16b);
        pred3_8x16b = vzip1q_s16(src14_8x16b, src15_8x16b);
        out7_8x16b  = vzip2q_s16(src8_8x16b, src9_8x16b);
        src10_8x16b = vzip2q_s16(src10_8x16b, src11_8x16b);
        pred7_8x16b = vzip2q_s16(src12_8x16b, src13_8x16b);
        src14_8x16b = vzip2q_s16(src14_8x16b, src15_8x16b);

        out1_8x16b  = vzip1q_s32(out3_8x16b, pred0_8x16b);
        out3_8x16b  = vzip2q_s32(out3_8x16b, pred0_8x16b);
        pred1_8x16b = vzip1q_s32(out2_8x16b, pred3_8x16b);
        pred3_8x16b = vzip2q_s32(out2_8x16b, pred3_8x16b);
        out5_8x16b  = vzip1q_s32(out7_8x16b, src10_8x16b);
        out7_8x16b  = vzip2q_s32(out7_8x16b, src10_8x16b);
        pred5_8x16b = vzip1q_s32(pred7_8x16b, src14_8x16b);
        pred7_8x16b = vzip2q_s32(pred7_8x16b, src14_8x16b);

        src8_8x16b  = vzip1q_s64(out1_8x16b, pred1_8x16b);
        src9_8x16b  = vzip2q_s64(out1_8x16b, pred1_8x16b);
        src10_8x16b = vzip1q_s64(out3_8x16b, pred3_8x16b);
        src11_8x16b = vzip2q_s64(out3_8x16b, pred3_8x16b);
        src12_8x16b = vzip1q_s64(out5_8x16b, pred5_8x16b);
        src13_8x16b = vzip2q_s64(out5_8x16b, pred5_8x16b);
        src14_8x16b = vzip1q_s64(out7_8x16b, pred7_8x16b);
        src15_8x16b = vzip2q_s64(out7_8x16b, pred7_8x16b);

        /**********************   8x8 16 bit Transpose End   *********************/
        /**************** 8x8 horizontal transform *******************************/

        /****************Horizontal Transform Addition****************************/
        out0_8x16b = vaddq_s16(src0_8x16b, src1_8x16b);
        out1_8x16b = vsubq_s16(src0_8x16b, src1_8x16b);

        out2_8x16b = vaddq_s16(src2_8x16b, src3_8x16b);
        out3_8x16b = vsubq_s16(src2_8x16b, src3_8x16b);

        out4_8x16b = vaddq_s16(src4_8x16b, src5_8x16b);
        out5_8x16b = vsubq_s16(src4_8x16b, src5_8x16b);

        out6_8x16b = vaddq_s16(src6_8x16b, src7_8x16b);
        out7_8x16b = vsubq_s16(src6_8x16b, src7_8x16b);

        out8_8x16b = vaddq_s16(src8_8x16b, src9_8x16b);
        out9_8x16b = vsubq_s16(src8_8x16b, src9_8x16b);

        out10_8x16b = vaddq_s16(src10_8x16b, src11_8x16b);
        out11_8x16b = vsubq_s16(src10_8x16b, src11_8x16b);

        out12_8x16b = vaddq_s16(src12_8x16b, src13_8x16b);
        out13_8x16b = vsubq_s16(src12_8x16b, src13_8x16b);

        out14_8x16b = vaddq_s16(src14_8x16b, src15_8x16b);
        out15_8x16b = vsubq_s16(src14_8x16b, src15_8x16b);
        /****************Horizontal Transform Addition****************************/

        src0_8x16b  = out0_8x16b;
        src1_8x16b  = out1_8x16b;
        src2_8x16b  = out2_8x16b;
        src3_8x16b  = out3_8x16b;
        src4_8x16b  = out4_8x16b;
        src5_8x16b  = out5_8x16b;
        src6_8x16b  = out6_8x16b;
        src7_8x16b  = out7_8x16b;
        src8_8x16b  = out8_8x16b;
        src9_8x16b  = out9_8x16b;
        src10_8x16b = out10_8x16b;
        src11_8x16b = out11_8x16b;
        src12_8x16b = out12_8x16b;
        src13_8x16b = out13_8x16b;
        src14_8x16b = out14_8x16b;
        src15_8x16b = out15_8x16b;

        int16x8_t out0a_8x16b, out1a_8x16b, out2a_8x16b, out3a_8x16b;
        int16x8_t out4a_8x16b, out5a_8x16b, out6a_8x16b, out7a_8x16b;
        int16x8_t out8a_8x16b, out9a_8x16b, out10a_8x16b, out11a_8x16b;
        int16x8_t out12a_8x16b, out13a_8x16b, out14a_8x16b, out15a_8x16b;
        int16x8_t tmp0_8x16b, tmp1_8x16b, tmp2_8x16b, tmp3_8x16b;
        int16x8_t tmp4_8x16b, tmp5_8x16b, tmp6_8x16b, tmp7_8x16b;
        int16x8_t tmp8_8x16b, tmp9_8x16b, tmp10_8x16b, tmp11_8x16b;
        int16x8_t tmp12_8x16b, tmp13_8x16b, tmp14_8x16b, tmp15_8x16b;

        /************************* 8x8 Vertical Transform*************************/
        tmp0_8x16b  = vcombine_s16(vget_high_s16(src0_8x16b), vcreate_s32(0));
        tmp2_8x16b  = vcombine_s16(vget_high_s16(src2_8x16b), vcreate_s32(0));
        tmp4_8x16b  = vcombine_s16(vget_high_s16(src4_8x16b), vcreate_s32(0));
        tmp6_8x16b  = vcombine_s16(vget_high_s16(src6_8x16b), vcreate_s32(0));
        tmp8_8x16b  = vcombine_s16(vget_high_s16(src8_8x16b), vcreate_s32(0));
        tmp10_8x16b = vcombine_s16(vget_high_s16(src10_8x16b), vcreate_s32(0));
        tmp12_8x16b = vcombine_s16(vget_high_s16(src12_8x16b), vcreate_s32(0));
        tmp14_8x16b = vcombine_s16(vget_high_s16(src14_8x16b), vcreate_s32(0));

        /*************************First 4 pixels ********************************/
        src0_8x16b  = vmovl_s16(vget_low_s16(src0_8x16b));
        src2_8x16b  = vmovl_s16(vget_low_s16(src2_8x16b));
        src4_8x16b  = vmovl_s16(vget_low_s16(src4_8x16b));
        src6_8x16b  = vmovl_s16(vget_low_s16(src6_8x16b));
        src8_8x16b  = vmovl_s16(vget_low_s16(src8_8x16b));
        src10_8x16b = vmovl_s16(vget_low_s16(src10_8x16b));
        src12_8x16b = vmovl_s16(vget_low_s16(src12_8x16b));
        src14_8x16b = vmovl_s16(vget_low_s16(src14_8x16b));

        /* r0 + r1 */
        pred0_8x16b = vaddq_s32(src0_8x16b, src2_8x16b);
        /* r2 + r3 */
        pred2_8x16b = vaddq_s32(src4_8x16b, src6_8x16b);
        /* r4 + r5 */
        pred4_8x16b = vaddq_s32(src8_8x16b, src10_8x16b);
        /* r6 + r7 */
        pred6_8x16b = vaddq_s32(src12_8x16b, src14_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        out0_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        out8_8x16b  = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        out4_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        out12_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = vsubq_s32(src0_8x16b, src2_8x16b);
        /* r2 - r3 */
        pred2_8x16b = vsubq_s32(src4_8x16b, src6_8x16b);
        /* r4 - r5 */
        pred4_8x16b = vsubq_s32(src8_8x16b, src10_8x16b);
        /* r6 - r7 */
        pred6_8x16b = vsubq_s32(src12_8x16b, src14_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        out2_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        out10_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        out6_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        out14_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);
        /*************************First 4 pixels ********************************/

        /**************************Next 4 pixels *******************************/
        src0_8x16b  = vmovl_s16(vget_low_s16(tmp0_8x16b));
        src2_8x16b  = vmovl_s16(vget_low_s16(tmp2_8x16b));
        src4_8x16b  = vmovl_s16(vget_low_s16(tmp4_8x16b));
        src6_8x16b  = vmovl_s16(vget_low_s16(tmp6_8x16b));
        src8_8x16b  = vmovl_s16(vget_low_s16(tmp8_8x16b));
        src10_8x16b = vmovl_s16(vget_low_s16(tmp10_8x16b));
        src12_8x16b = vmovl_s16(vget_low_s16(tmp12_8x16b));
        src14_8x16b = vmovl_s16(vget_low_s16(tmp14_8x16b));

        /* r0 + r1 */
        pred0_8x16b = vaddq_s32(src0_8x16b, src2_8x16b);
        /* r2 + r3 */
        pred2_8x16b = vaddq_s32(src4_8x16b, src6_8x16b);
        /* r4 + r5 */
        pred4_8x16b = vaddq_s32(src8_8x16b, src10_8x16b);
        /* r6 + r7 */
        pred6_8x16b = vaddq_s32(src12_8x16b, src14_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        out0a_8x16b = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        out8a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b  = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b  = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        out4a_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        out12a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = vsubq_s32(src0_8x16b, src2_8x16b);
        /* r2 - r3 */
        pred2_8x16b = vsubq_s32(src4_8x16b, src6_8x16b);
        /* r4 - r5 */
        pred4_8x16b = vsubq_s32(src8_8x16b, src10_8x16b);
        /* r6 - r7 */
        pred6_8x16b = vsubq_s32(src12_8x16b, src14_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b  = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b  = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        out2a_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        out10a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b  = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b  = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        out6a_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        out14a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);
        /**************************Next 4 pixels *******************************/
        /************************* 8x8 Vertical Transform*************************/

        /************************* 8x8 Vertical Transform*************************/
        tmp1_8x16b  = vcombine_s16(vget_high_s16(src1_8x16b), vcreate_s32(0));
        tmp3_8x16b  = vcombine_s16(vget_high_s16(src3_8x16b), vcreate_s32(0));
        tmp5_8x16b  = vcombine_s16(vget_high_s16(src5_8x16b), vcreate_s32(0));
        tmp7_8x16b  = vcombine_s16(vget_high_s16(src7_8x16b), vcreate_s32(0));
        tmp9_8x16b  = vcombine_s16(vget_high_s16(src9_8x16b), vcreate_s32(0));
        tmp11_8x16b = vcombine_s16(vget_high_s16(src11_8x16b), vcreate_s32(0));
        tmp13_8x16b = vcombine_s16(vget_high_s16(src13_8x16b), vcreate_s32(0));
        tmp15_8x16b = vcombine_s16(vget_high_s16(src15_8x16b), vcreate_s32(0));

        /*************************First 4 pixels ********************************/
        src1_8x16b  = vmovl_s16(vget_low_s16(src1_8x16b));
        src3_8x16b  = vmovl_s16(vget_low_s16(src3_8x16b));
        src5_8x16b  = vmovl_s16(vget_low_s16(src5_8x16b));
        src7_8x16b  = vmovl_s16(vget_low_s16(src7_8x16b));
        src9_8x16b  = vmovl_s16(vget_low_s16(src9_8x16b));
        src11_8x16b = vmovl_s16(vget_low_s16(src11_8x16b));
        src13_8x16b = vmovl_s16(vget_low_s16(src13_8x16b));
        src15_8x16b = vmovl_s16(vget_low_s16(src15_8x16b));

        /* r0 + r1 */
        pred0_8x16b = vaddq_s32(src1_8x16b, src3_8x16b);
        /* r2 + r3 */
        pred2_8x16b = vaddq_s32(src5_8x16b, src7_8x16b);
        /* r4 + r5 */
        pred4_8x16b = vaddq_s32(src9_8x16b, src11_8x16b);
        /* r6 + r7 */
        pred6_8x16b = vaddq_s32(src13_8x16b, src15_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        out1_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        out9_8x16b  = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        out5_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        out13_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = vsubq_s32(src1_8x16b, src3_8x16b);
        /* r2 - r3 */
        pred2_8x16b = vsubq_s32(src5_8x16b, src7_8x16b);
        /* r4 - r5 */
        pred4_8x16b = vsubq_s32(src9_8x16b, src11_8x16b);
        /* r6 - r7 */
        pred6_8x16b = vsubq_s32(src13_8x16b, src15_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        out3_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        out11_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        out7_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        out15_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);
        /*************************First 4 pixels ********************************/

        /*************************Next 4 pixels ********************************/
        src1_8x16b  = vmovl_s16(vget_low_s16(tmp1_8x16b));
        src3_8x16b  = vmovl_s16(vget_low_s16(tmp3_8x16b));
        src5_8x16b  = vmovl_s16(vget_low_s16(tmp5_8x16b));
        src7_8x16b  = vmovl_s16(vget_low_s16(tmp7_8x16b));
        src9_8x16b  = vmovl_s16(vget_low_s16(tmp9_8x16b));
        src11_8x16b = vmovl_s16(vget_low_s16(tmp11_8x16b));
        src13_8x16b = vmovl_s16(vget_low_s16(tmp13_8x16b));
        src15_8x16b = vmovl_s16(vget_low_s16(tmp15_8x16b));

        /* r0 + r1 */
        pred0_8x16b = vaddq_s32(src1_8x16b, src3_8x16b);
        /* r2 + r3 */
        pred2_8x16b = vaddq_s32(src5_8x16b, src7_8x16b);
        /* r4 + r5 */
        pred4_8x16b = vaddq_s32(src9_8x16b, src11_8x16b);
        /* r6 + r7 */
        pred6_8x16b = vaddq_s32(src13_8x16b, src15_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        out1a_8x16b = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        out9a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b  = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b  = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        out5a_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        out13a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = vsubq_s32(src1_8x16b, src3_8x16b);
        /* r2 - r3 */
        pred2_8x16b = vsubq_s32(src5_8x16b, src7_8x16b);
        /* r4 - r5 */
        pred4_8x16b = vsubq_s32(src9_8x16b, src11_8x16b);
        /* r6 - r7 */
        pred6_8x16b = vsubq_s32(src13_8x16b, src15_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b  = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b  = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        out3a_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        out11a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b  = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b  = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        out7a_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        out15a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);
        /************************* 8x8 Vertical Transform*************************/

        /****************************SATD calculation ****************************/
        src0_8x16b = vabsq_s32(out0_8x16b);
        src1_8x16b = vabsq_s32(out1_8x16b);
        src2_8x16b = vabsq_s32(out2_8x16b);
        src3_8x16b = vabsq_s32(out3_8x16b);
        src4_8x16b = vabsq_s32(out4_8x16b);
        src5_8x16b = vabsq_s32(out5_8x16b);
        src6_8x16b = vabsq_s32(out6_8x16b);
        src7_8x16b = vabsq_s32(out7_8x16b);
        s32* p     = (s32*)&src0_8x16b;
        p[0]       = p[0] >> 2;

        satd = vaddvq_s32(src0_8x16b);
        satd += vaddvq_s32(src1_8x16b);
        satd += vaddvq_s32(src2_8x16b);
        satd += vaddvq_s32(src3_8x16b);
        satd += vaddvq_s32(src4_8x16b);
        satd += vaddvq_s32(src5_8x16b);
        satd += vaddvq_s32(src6_8x16b);
        satd += vaddvq_s32(src7_8x16b);

        src0_8x16b = vabsq_s32(out8_8x16b);
        src1_8x16b = vabsq_s32(out9_8x16b);
        src2_8x16b = vabsq_s32(out10_8x16b);
        src3_8x16b = vabsq_s32(out11_8x16b);
        src4_8x16b = vabsq_s32(out12_8x16b);
        src5_8x16b = vabsq_s32(out13_8x16b);
        src6_8x16b = vabsq_s32(out14_8x16b);
        src7_8x16b = vabsq_s32(out15_8x16b);

        satd += vaddvq_s32(src0_8x16b);
        satd += vaddvq_s32(src1_8x16b);
        satd += vaddvq_s32(src2_8x16b);
        satd += vaddvq_s32(src3_8x16b);
        satd += vaddvq_s32(src4_8x16b);
        satd += vaddvq_s32(src5_8x16b);
        satd += vaddvq_s32(src6_8x16b);
        satd += vaddvq_s32(src7_8x16b);

        src0_8x16b = vabsq_s32(out0a_8x16b);
        src1_8x16b = vabsq_s32(out1a_8x16b);
        src2_8x16b = vabsq_s32(out2a_8x16b);
        src3_8x16b = vabsq_s32(out3a_8x16b);
        src4_8x16b = vabsq_s32(out4a_8x16b);
        src5_8x16b = vabsq_s32(out5a_8x16b);
        src6_8x16b = vabsq_s32(out6a_8x16b);
        src7_8x16b = vabsq_s32(out7a_8x16b);

        satd += vaddvq_s32(src0_8x16b);
        satd += vaddvq_s32(src1_8x16b);
        satd += vaddvq_s32(src2_8x16b);
        satd += vaddvq_s32(src3_8x16b);
        satd += vaddvq_s32(src4_8x16b);
        satd += vaddvq_s32(src5_8x16b);
        satd += vaddvq_s32(src6_8x16b);
        satd += vaddvq_s32(src7_8x16b);

        src0_8x16b = vabsq_s32(out8a_8x16b);
        src1_8x16b = vabsq_s32(out9a_8x16b);
        src2_8x16b = vabsq_s32(out10a_8x16b);
        src3_8x16b = vabsq_s32(out11a_8x16b);
        src4_8x16b = vabsq_s32(out12a_8x16b);
        src5_8x16b = vabsq_s32(out13a_8x16b);
        src6_8x16b = vabsq_s32(out14a_8x16b);
        src7_8x16b = vabsq_s32(out15a_8x16b);

        satd += vaddvq_s32(src0_8x16b);
        satd += vaddvq_s32(src1_8x16b);
        satd += vaddvq_s32(src2_8x16b);
        satd += vaddvq_s32(src3_8x16b);
        satd += vaddvq_s32(src4_8x16b);
        satd += vaddvq_s32(src5_8x16b);
        satd += vaddvq_s32(src6_8x16b);
        satd += vaddvq_s32(src7_8x16b);

        satd = (int)(satd / sqrt(16.0 * 8) * 2);
        return satd;
    }
    else {
        int  k, i, j, jj;
        int  satd = 0;
        int  sub[128], interm1[8][16], interm2[8][16];
        pel *curn = cur, *orgn = org;

        for(k = 0; k < 128; k += 16) {
            sub[k + 0] = orgn[0] - curn[0];
            sub[k + 1] = orgn[1] - curn[1];
            sub[k + 2] = orgn[2] - curn[2];
            sub[k + 3] = orgn[3] - curn[3];
            sub[k + 4] = orgn[4] - curn[4];
            sub[k + 5] = orgn[5] - curn[5];
            sub[k + 6] = orgn[6] - curn[6];
            sub[k + 7] = orgn[7] - curn[7];

            sub[k + 8]  = orgn[8] - curn[8];
            sub[k + 9]  = orgn[9] - curn[9];
            sub[k + 10] = orgn[10] - curn[10];
            sub[k + 11] = orgn[11] - curn[11];
            sub[k + 12] = orgn[12] - curn[12];
            sub[k + 13] = orgn[13] - curn[13];
            sub[k + 14] = orgn[14] - curn[14];
            sub[k + 15] = orgn[15] - curn[15];

            curn += s_cur;
            orgn += s_org;
        }

        for(j = 0; j < 8; j++) {
            jj = j << 4;

            interm2[j][0]  = sub[jj] + sub[jj + 8];
            interm2[j][1]  = sub[jj + 1] + sub[jj + 9];
            interm2[j][2]  = sub[jj + 2] + sub[jj + 10];
            interm2[j][3]  = sub[jj + 3] + sub[jj + 11];
            interm2[j][4]  = sub[jj + 4] + sub[jj + 12];
            interm2[j][5]  = sub[jj + 5] + sub[jj + 13];
            interm2[j][6]  = sub[jj + 6] + sub[jj + 14];
            interm2[j][7]  = sub[jj + 7] + sub[jj + 15];
            interm2[j][8]  = sub[jj] - sub[jj + 8];
            interm2[j][9]  = sub[jj + 1] - sub[jj + 9];
            interm2[j][10] = sub[jj + 2] - sub[jj + 10];
            interm2[j][11] = sub[jj + 3] - sub[jj + 11];
            interm2[j][12] = sub[jj + 4] - sub[jj + 12];
            interm2[j][13] = sub[jj + 5] - sub[jj + 13];
            interm2[j][14] = sub[jj + 6] - sub[jj + 14];
            interm2[j][15] = sub[jj + 7] - sub[jj + 15];

            interm1[j][0]  = interm2[j][0] + interm2[j][4];
            interm1[j][1]  = interm2[j][1] + interm2[j][5];
            interm1[j][2]  = interm2[j][2] + interm2[j][6];
            interm1[j][3]  = interm2[j][3] + interm2[j][7];
            interm1[j][4]  = interm2[j][0] - interm2[j][4];
            interm1[j][5]  = interm2[j][1] - interm2[j][5];
            interm1[j][6]  = interm2[j][2] - interm2[j][6];
            interm1[j][7]  = interm2[j][3] - interm2[j][7];
            interm1[j][8]  = interm2[j][8] + interm2[j][12];
            interm1[j][9]  = interm2[j][9] + interm2[j][13];
            interm1[j][10] = interm2[j][10] + interm2[j][14];
            interm1[j][11] = interm2[j][11] + interm2[j][15];
            interm1[j][12] = interm2[j][8] - interm2[j][12];
            interm1[j][13] = interm2[j][9] - interm2[j][13];
            interm1[j][14] = interm2[j][10] - interm2[j][14];
            interm1[j][15] = interm2[j][11] - interm2[j][15];

            interm2[j][0]  = interm1[j][0] + interm1[j][2];
            interm2[j][1]  = interm1[j][1] + interm1[j][3];
            interm2[j][2]  = interm1[j][0] - interm1[j][2];
            interm2[j][3]  = interm1[j][1] - interm1[j][3];
            interm2[j][4]  = interm1[j][4] + interm1[j][6];
            interm2[j][5]  = interm1[j][5] + interm1[j][7];
            interm2[j][6]  = interm1[j][4] - interm1[j][6];
            interm2[j][7]  = interm1[j][5] - interm1[j][7];
            interm2[j][8]  = interm1[j][8] + interm1[j][10];
            interm2[j][9]  = interm1[j][9] + interm1[j][11];
            interm2[j][10] = interm1[j][8] - interm1[j][10];
            interm2[j][11] = interm1[j][9] - interm1[j][11];
            interm2[j][12] = interm1[j][12] + interm1[j][14];
            interm2[j][13] = interm1[j][13] + interm1[j][15];
            interm2[j][14] = interm1[j][12] - interm1[j][14];
            interm2[j][15] = interm1[j][13] - interm1[j][15];

            interm1[j][0]  = interm2[j][0] + interm2[j][1];
            interm1[j][1]  = interm2[j][0] - interm2[j][1];
            interm1[j][2]  = interm2[j][2] + interm2[j][3];
            interm1[j][3]  = interm2[j][2] - interm2[j][3];
            interm1[j][4]  = interm2[j][4] + interm2[j][5];
            interm1[j][5]  = interm2[j][4] - interm2[j][5];
            interm1[j][6]  = interm2[j][6] + interm2[j][7];
            interm1[j][7]  = interm2[j][6] - interm2[j][7];
            interm1[j][8]  = interm2[j][8] + interm2[j][9];
            interm1[j][9]  = interm2[j][8] - interm2[j][9];
            interm1[j][10] = interm2[j][10] + interm2[j][11];
            interm1[j][11] = interm2[j][10] - interm2[j][11];
            interm1[j][12] = interm2[j][12] + interm2[j][13];
            interm1[j][13] = interm2[j][12] - interm2[j][13];
            interm1[j][14] = interm2[j][14] + interm2[j][15];
            interm1[j][15] = interm2[j][14] - interm2[j][15];
        }

        for(i = 0; i < 16; i++) {
            interm2[0][i] = interm1[0][i] + interm1[4][i];
            interm2[1][i] = interm1[1][i] + interm1[5][i];
            interm2[2][i] = interm1[2][i] + interm1[6][i];
            interm2[3][i] = interm1[3][i] + interm1[7][i];
            interm2[4][i] = interm1[0][i] - interm1[4][i];
            interm2[5][i] = interm1[1][i] - interm1[5][i];
            interm2[6][i] = interm1[2][i] - interm1[6][i];
            interm2[7][i] = interm1[3][i] - interm1[7][i];

            interm1[0][i] = interm2[0][i] + interm2[2][i];
            interm1[1][i] = interm2[1][i] + interm2[3][i];
            interm1[2][i] = interm2[0][i] - interm2[2][i];
            interm1[3][i] = interm2[1][i] - interm2[3][i];
            interm1[4][i] = interm2[4][i] + interm2[6][i];
            interm1[5][i] = interm2[5][i] + interm2[7][i];
            interm1[6][i] = interm2[4][i] - interm2[6][i];
            interm1[7][i] = interm2[5][i] - interm2[7][i];

            interm2[0][i] = XEVE_ABS(interm1[0][i] + interm1[1][i]);
            interm2[1][i] = XEVE_ABS(interm1[0][i] - interm1[1][i]);
            interm2[2][i] = XEVE_ABS(interm1[2][i] + interm1[3][i]);
            interm2[3][i] = XEVE_ABS(interm1[2][i] - interm1[3][i]);
            interm2[4][i] = XEVE_ABS(interm1[4][i] + interm1[5][i]);
            interm2[5][i] = XEVE_ABS(interm1[4][i] - interm1[5][i]);
            interm2[6][i] = XEVE_ABS(interm1[6][i] + interm1[7][i]);
            interm2[7][i] = XEVE_ABS(interm1[6][i] - interm1[7][i]);
        }

        satd = interm2[0][0] >> 2;
        for(j = 1; j < 16; j++) {
            satd += interm2[0][j];
        }
        for(i = 1; i < 8; i++) {
            for(j = 0; j < 16; j++) {
                satd += interm2[i][j];
            }
        }

        satd = (int)(satd / (2.0 * sqrt(8.0)));

        return satd;
    }
}

int xeve_had_8x16_neon(pel* org, pel* cur, int s_org, int s_cur, int step, int bit_depth)
{
    if(bit_depth == 10) {
        int       satd = 0;
        /* all 128 bit registers are named with a suffix mxnb, where m is the */
        /* number of n bits packed in the register                            */
        int16x8_t src0_8x16b, src1_8x16b, src2_8x16b, src3_8x16b;
        int16x8_t src4_8x16b, src5_8x16b, src6_8x16b, src7_8x16b;
        int16x8_t src8_8x16b, src9_8x16b, src10_8x16b, src11_8x16b;
        int16x8_t src12_8x16b, src13_8x16b, src14_8x16b, src15_8x16b;
        int16x8_t pred0_8x16b, pred1_8x16b, pred2_8x16b, pred3_8x16b;
        int16x8_t pred4_8x16b, pred5_8x16b, pred6_8x16b, pred7_8x16b;
        int16x8_t pred8_8x16b, pred9_8x16b, pred10_8x16b, pred11_8x16b;
        int16x8_t pred12_8x16b, pred13_8x16b, pred14_8x16b, pred15_8x16b;
        int16x8_t out0_8x16b, out1_8x16b, out2_8x16b, out3_8x16b;
        int16x8_t out4_8x16b, out5_8x16b, out6_8x16b, out7_8x16b;
        int16x8_t out8_8x16b, out9_8x16b, out10_8x16b, out11_8x16b;
        int16x8_t out12_8x16b, out13_8x16b, out14_8x16b, out15_8x16b;

        /**********************Residue Calculation********************************/
        src0_8x16b = (vld1q_s16(&org[0]));
        org        = org + s_org;
        src1_8x16b = (vld1q_s16(&org[0]));
        org        = org + s_org;
        src2_8x16b = (vld1q_s16(&org[0]));
        org        = org + s_org;
        src3_8x16b = (vld1q_s16(&org[0]));
        org        = org + s_org;
        src4_8x16b = (vld1q_s16(&org[0]));
        org        = org + s_org;
        src5_8x16b = (vld1q_s16(&org[0]));
        org        = org + s_org;
        src6_8x16b = (vld1q_s16(&org[0]));
        org        = org + s_org;
        src7_8x16b = (vld1q_s16(&org[0]));
        org        = org + s_org;

        pred0_8x16b = (vld1q_s16(&cur[0]));
        cur         = cur + s_cur;
        pred1_8x16b = (vld1q_s16(&cur[0]));
        cur         = cur + s_cur;
        pred2_8x16b = (vld1q_s16(&cur[0]));
        cur         = cur + s_cur;
        pred3_8x16b = (vld1q_s16(&cur[0]));
        cur         = cur + s_cur;
        pred4_8x16b = (vld1q_s16(&cur[0]));
        cur         = cur + s_cur;
        pred5_8x16b = (vld1q_s16(&cur[0]));
        cur         = cur + s_cur;
        pred6_8x16b = (vld1q_s16(&cur[0]));
        cur         = cur + s_cur;
        pred7_8x16b = (vld1q_s16(&cur[0]));
        cur         = cur + s_cur;

        src0_8x16b = vsubq_s16(src0_8x16b, pred0_8x16b);
        src1_8x16b = vsubq_s16(src1_8x16b, pred1_8x16b);
        src2_8x16b = vsubq_s16(src2_8x16b, pred2_8x16b);
        src3_8x16b = vsubq_s16(src3_8x16b, pred3_8x16b);
        src4_8x16b = vsubq_s16(src4_8x16b, pred4_8x16b);
        src5_8x16b = vsubq_s16(src5_8x16b, pred5_8x16b);
        src6_8x16b = vsubq_s16(src6_8x16b, pred6_8x16b);
        src7_8x16b = vsubq_s16(src7_8x16b, pred7_8x16b);

        src8_8x16b  = (vld1q_s16(&org[0]));
        org         = org + s_org;
        src9_8x16b  = (vld1q_s16(&org[0]));
        org         = org + s_org;
        src10_8x16b = (vld1q_s16(&org[0]));
        org         = org + s_org;
        src11_8x16b = (vld1q_s16(&org[0]));
        org         = org + s_org;
        src12_8x16b = (vld1q_s16(&org[0]));
        org         = org + s_org;
        src13_8x16b = (vld1q_s16(&org[0]));
        org         = org + s_org;
        src14_8x16b = (vld1q_s16(&org[0]));
        org         = org + s_org;
        src15_8x16b = (vld1q_s16(&org[0]));
        org         = org + s_org;

        pred8_8x16b  = (vld1q_s16(&cur[0]));
        cur          = cur + s_cur;
        pred9_8x16b  = (vld1q_s16(&cur[0]));
        cur          = cur + s_cur;
        pred10_8x16b = (vld1q_s16(&cur[0]));
        cur          = cur + s_cur;
        pred11_8x16b = (vld1q_s16(&cur[0]));
        cur          = cur + s_cur;
        pred12_8x16b = (vld1q_s16(&cur[0]));
        cur          = cur + s_cur;
        pred13_8x16b = (vld1q_s16(&cur[0]));
        cur          = cur + s_cur;
        pred14_8x16b = (vld1q_s16(&cur[0]));
        cur          = cur + s_cur;
        pred15_8x16b = (vld1q_s16(&cur[0]));
        cur          = cur + s_cur;

        src8_8x16b  = vsubq_s16(src8_8x16b, pred8_8x16b);
        src9_8x16b  = vsubq_s16(src9_8x16b, pred9_8x16b);
        src10_8x16b = vsubq_s16(src10_8x16b, pred10_8x16b);
        src11_8x16b = vsubq_s16(src11_8x16b, pred11_8x16b);
        src12_8x16b = vsubq_s16(src12_8x16b, pred12_8x16b);
        src13_8x16b = vsubq_s16(src13_8x16b, pred13_8x16b);
        src14_8x16b = vsubq_s16(src14_8x16b, pred14_8x16b);
        src15_8x16b = vsubq_s16(src15_8x16b, pred15_8x16b);
        /**********************Residue Calculation********************************/

        /**************** 8x8 horizontal transform *******************************/
        /***********************    8x8 16 bit Transpose  ************************/
        out3_8x16b  = vzip1q_s16(src0_8x16b, src1_8x16b);
        pred0_8x16b = vzip1q_s16(src2_8x16b, src3_8x16b);
        out2_8x16b  = vzip1q_s16(src4_8x16b, src5_8x16b);
        pred3_8x16b = vzip1q_s16(src6_8x16b, src7_8x16b);
        out7_8x16b  = vzip2q_s16(src0_8x16b, src1_8x16b);
        src2_8x16b  = vzip2q_s16(src2_8x16b, src3_8x16b);
        pred7_8x16b = vzip2q_s16(src4_8x16b, src5_8x16b);
        src6_8x16b  = vzip2q_s16(src6_8x16b, src7_8x16b);

        out1_8x16b  = vzip1q_s32(out3_8x16b, pred0_8x16b);
        out3_8x16b  = vzip2q_s32(out3_8x16b, pred0_8x16b);
        pred1_8x16b = vzip1q_s32(out2_8x16b, pred3_8x16b);
        pred3_8x16b = vzip2q_s32(out2_8x16b, pred3_8x16b);
        out5_8x16b  = vzip1q_s32(out7_8x16b, src2_8x16b);
        out7_8x16b  = vzip2q_s32(out7_8x16b, src2_8x16b);
        pred5_8x16b = vzip1q_s32(pred7_8x16b, src6_8x16b);
        pred7_8x16b = vzip2q_s32(pred7_8x16b, src6_8x16b);

        out0_8x16b = vzip1q_s64(out1_8x16b, pred1_8x16b);
        out1_8x16b = vzip2q_s64(out1_8x16b, pred1_8x16b);
        out2_8x16b = vzip1q_s64(out3_8x16b, pred3_8x16b);
        out3_8x16b = vzip2q_s64(out3_8x16b, pred3_8x16b);
        out4_8x16b = vzip1q_s64(out5_8x16b, pred5_8x16b);
        out5_8x16b = vzip2q_s64(out5_8x16b, pred5_8x16b);
        out6_8x16b = vzip1q_s64(out7_8x16b, pred7_8x16b);
        out7_8x16b = vzip2q_s64(out7_8x16b, pred7_8x16b);

        /**********************   8x8 16 bit Transpose End   *********************/

        /* r0 + r1 */
        pred0_8x16b = vaddq_s16(out0_8x16b, out1_8x16b);
        /* r2 + r3 */
        pred2_8x16b = vaddq_s16(out2_8x16b, out3_8x16b);
        /* r4 + r5 */
        pred4_8x16b = vaddq_s16(out4_8x16b, out5_8x16b);
        /* r6 + r7 */
        pred6_8x16b = vaddq_s16(out6_8x16b, out7_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = vaddq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = vaddq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        src0_8x16b  = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        src4_8x16b  = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b = vsubq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b = vsubq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        src2_8x16b  = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        src6_8x16b  = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = vsubq_s16(out0_8x16b, out1_8x16b);
        /* r2 - r3 */
        pred2_8x16b = vsubq_s16(out2_8x16b, out3_8x16b);
        /* r4 - r5 */
        pred4_8x16b = vsubq_s16(out4_8x16b, out5_8x16b);
        /* r6 - r7 */
        pred6_8x16b = vsubq_s16(out6_8x16b, out7_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b = vaddq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b = vaddq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        src1_8x16b  = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        src5_8x16b  = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b = vsubq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b = vsubq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        src3_8x16b  = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        src7_8x16b  = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /***********************    8x8 16 bit Transpose  ************************/
        out3_8x16b  = vzip1q_s16(src0_8x16b, src1_8x16b);
        pred0_8x16b = vzip1q_s16(src2_8x16b, src3_8x16b);
        out2_8x16b  = vzip1q_s16(src4_8x16b, src5_8x16b);
        pred3_8x16b = vzip1q_s16(src6_8x16b, src7_8x16b);
        out7_8x16b  = vzip2q_s16(src0_8x16b, src1_8x16b);
        src2_8x16b  = vzip2q_s16(src2_8x16b, src3_8x16b);
        pred7_8x16b = vzip2q_s16(src4_8x16b, src5_8x16b);
        src6_8x16b  = vzip2q_s16(src6_8x16b, src7_8x16b);

        out1_8x16b  = vzip1q_s32(out3_8x16b, pred0_8x16b);
        out3_8x16b  = vzip2q_s32(out3_8x16b, pred0_8x16b);
        pred1_8x16b = vzip1q_s32(out2_8x16b, pred3_8x16b);
        pred3_8x16b = vzip2q_s32(out2_8x16b, pred3_8x16b);
        out5_8x16b  = vzip1q_s32(out7_8x16b, src2_8x16b);
        out7_8x16b  = vzip2q_s32(out7_8x16b, src2_8x16b);
        pred5_8x16b = vzip1q_s32(pred7_8x16b, src6_8x16b);
        pred7_8x16b = vzip2q_s32(pred7_8x16b, src6_8x16b);

        src0_8x16b = vzip1q_s64(out1_8x16b, pred1_8x16b);
        src1_8x16b = vzip2q_s64(out1_8x16b, pred1_8x16b);
        src2_8x16b = vzip1q_s64(out3_8x16b, pred3_8x16b);
        src3_8x16b = vzip2q_s64(out3_8x16b, pred3_8x16b);
        src4_8x16b = vzip1q_s64(out5_8x16b, pred5_8x16b);
        src5_8x16b = vzip2q_s64(out5_8x16b, pred5_8x16b);
        src6_8x16b = vzip1q_s64(out7_8x16b, pred7_8x16b);
        src7_8x16b = vzip2q_s64(out7_8x16b, pred7_8x16b);

        /**********************   8x8 16 bit Transpose End   *********************/
        /**************** 8x8 horizontal transform *******************************/

        /**************** 8x8 horizontal transform *******************************/
        /***********************    8x8 16 bit Transpose  ************************/
        out3_8x16b  = vzip1q_s16(src8_8x16b, src9_8x16b);
        pred0_8x16b = vzip1q_s16(src10_8x16b, src11_8x16b);
        out2_8x16b  = vzip1q_s16(src12_8x16b, src13_8x16b);
        pred3_8x16b = vzip1q_s16(src14_8x16b, src15_8x16b);
        out7_8x16b  = vzip2q_s16(src8_8x16b, src9_8x16b);
        src10_8x16b = vzip2q_s16(src10_8x16b, src11_8x16b);
        pred7_8x16b = vzip2q_s16(src12_8x16b, src13_8x16b);
        src14_8x16b = vzip2q_s16(src14_8x16b, src15_8x16b);

        out1_8x16b  = vzip1q_s32(out3_8x16b, pred0_8x16b);
        out3_8x16b  = vzip2q_s32(out3_8x16b, pred0_8x16b);
        pred1_8x16b = vzip1q_s32(out2_8x16b, pred3_8x16b);
        pred3_8x16b = vzip2q_s32(out2_8x16b, pred3_8x16b);
        out5_8x16b  = vzip1q_s32(out7_8x16b, src10_8x16b);
        out7_8x16b  = vzip2q_s32(out7_8x16b, src10_8x16b);
        pred5_8x16b = vzip1q_s32(pred7_8x16b, src14_8x16b);
        pred7_8x16b = vzip2q_s32(pred7_8x16b, src14_8x16b);

        out0_8x16b = vzip1q_s64(out1_8x16b, pred1_8x16b);
        out1_8x16b = vzip2q_s64(out1_8x16b, pred1_8x16b);
        out2_8x16b = vzip1q_s64(out3_8x16b, pred3_8x16b);
        out3_8x16b = vzip2q_s64(out3_8x16b, pred3_8x16b);
        out4_8x16b = vzip1q_s64(out5_8x16b, pred5_8x16b);
        out5_8x16b = vzip2q_s64(out5_8x16b, pred5_8x16b);
        out6_8x16b = vzip1q_s64(out7_8x16b, pred7_8x16b);
        out7_8x16b = vzip2q_s64(out7_8x16b, pred7_8x16b);

        /**********************   8x8 16 bit Transpose End   *********************/

        /* r0 + r1 */
        pred0_8x16b = vaddq_s16(out0_8x16b, out1_8x16b);
        /* r2 + r3 */
        pred2_8x16b = vaddq_s16(out2_8x16b, out3_8x16b);
        /* r4 + r5 */
        pred4_8x16b = vaddq_s16(out4_8x16b, out5_8x16b);
        /* r6 + r7 */
        pred6_8x16b = vaddq_s16(out6_8x16b, out7_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = vaddq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = vaddq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        src8_8x16b  = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        src12_8x16b = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b = vsubq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b = vsubq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        src10_8x16b = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        src14_8x16b = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = vsubq_s16(out0_8x16b, out1_8x16b);
        /* r2 - r3 */
        pred2_8x16b = vsubq_s16(out2_8x16b, out3_8x16b);
        /* r4 - r5 */
        pred4_8x16b = vsubq_s16(out4_8x16b, out5_8x16b);
        /* r6 - r7 */
        pred6_8x16b = vsubq_s16(out6_8x16b, out7_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b = vaddq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b = vaddq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        src9_8x16b  = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        src13_8x16b = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b = vsubq_s16(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b = vsubq_s16(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        src11_8x16b = vaddq_s16(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        src15_8x16b = vsubq_s16(pred1_8x16b, pred5_8x16b);

        /***********************    8x8 16 bit Transpose  ************************/
        out3_8x16b  = vzip1q_s16(src8_8x16b, src9_8x16b);
        pred0_8x16b = vzip1q_s16(src10_8x16b, src11_8x16b);
        out2_8x16b  = vzip1q_s16(src12_8x16b, src13_8x16b);
        pred3_8x16b = vzip1q_s16(src14_8x16b, src15_8x16b);
        out7_8x16b  = vzip2q_s16(src8_8x16b, src9_8x16b);
        src10_8x16b = vzip2q_s16(src10_8x16b, src11_8x16b);
        pred7_8x16b = vzip2q_s16(src12_8x16b, src13_8x16b);
        src14_8x16b = vzip2q_s16(src14_8x16b, src15_8x16b);

        out1_8x16b  = vzip1q_s32(out3_8x16b, pred0_8x16b);
        out3_8x16b  = vzip2q_s32(out3_8x16b, pred0_8x16b);
        pred1_8x16b = vzip1q_s32(out2_8x16b, pred3_8x16b);
        pred3_8x16b = vzip2q_s32(out2_8x16b, pred3_8x16b);
        out5_8x16b  = vzip1q_s32(out7_8x16b, src10_8x16b);
        out7_8x16b  = vzip2q_s32(out7_8x16b, src10_8x16b);
        pred5_8x16b = vzip1q_s32(pred7_8x16b, src14_8x16b);
        pred7_8x16b = vzip2q_s32(pred7_8x16b, src14_8x16b);

        src8_8x16b  = vzip1q_s64(out1_8x16b, pred1_8x16b);
        src9_8x16b  = vzip2q_s64(out1_8x16b, pred1_8x16b);
        src10_8x16b = vzip1q_s64(out3_8x16b, pred3_8x16b);
        src11_8x16b = vzip2q_s64(out3_8x16b, pred3_8x16b);
        src12_8x16b = vzip1q_s64(out5_8x16b, pred5_8x16b);
        src13_8x16b = vzip2q_s64(out5_8x16b, pred5_8x16b);
        src14_8x16b = vzip1q_s64(out7_8x16b, pred7_8x16b);
        src15_8x16b = vzip2q_s64(out7_8x16b, pred7_8x16b);

        /**********************   8x8 16 bit Transpose End   *********************/
        /**************** 8x8 horizontal transform *******************************/

        /****************Horizontal Transform Addition****************************/
        out0_8x16b = vaddq_s16(src0_8x16b, src1_8x16b);
        out1_8x16b = vsubq_s16(src0_8x16b, src1_8x16b);

        out2_8x16b = vaddq_s16(src2_8x16b, src3_8x16b);
        out3_8x16b = vsubq_s16(src2_8x16b, src3_8x16b);

        out4_8x16b = vaddq_s16(src4_8x16b, src5_8x16b);
        out5_8x16b = vsubq_s16(src4_8x16b, src5_8x16b);

        out6_8x16b = vaddq_s16(src6_8x16b, src7_8x16b);
        out7_8x16b = vsubq_s16(src6_8x16b, src7_8x16b);

        out8_8x16b = vaddq_s16(src8_8x16b, src9_8x16b);
        out9_8x16b = vsubq_s16(src8_8x16b, src9_8x16b);

        out10_8x16b = vaddq_s16(src10_8x16b, src11_8x16b);
        out11_8x16b = vsubq_s16(src10_8x16b, src11_8x16b);

        out12_8x16b = vaddq_s16(src12_8x16b, src13_8x16b);
        out13_8x16b = vsubq_s16(src12_8x16b, src13_8x16b);

        out14_8x16b = vaddq_s16(src14_8x16b, src15_8x16b);
        out15_8x16b = vsubq_s16(src14_8x16b, src15_8x16b);
        /****************Horizontal Transform Addition****************************/

        src0_8x16b  = out0_8x16b;
        src1_8x16b  = out1_8x16b;
        src2_8x16b  = out2_8x16b;
        src3_8x16b  = out3_8x16b;
        src4_8x16b  = out4_8x16b;
        src5_8x16b  = out5_8x16b;
        src6_8x16b  = out6_8x16b;
        src7_8x16b  = out7_8x16b;
        src8_8x16b  = out8_8x16b;
        src9_8x16b  = out9_8x16b;
        src10_8x16b = out10_8x16b;
        src11_8x16b = out11_8x16b;
        src12_8x16b = out12_8x16b;
        src13_8x16b = out13_8x16b;
        src14_8x16b = out14_8x16b;
        src15_8x16b = out15_8x16b;

        int16x8_t out0a_8x16b, out1a_8x16b, out2a_8x16b, out3a_8x16b;
        int16x8_t out4a_8x16b, out5a_8x16b, out6a_8x16b, out7a_8x16b;
        int16x8_t out8a_8x16b, out9a_8x16b, out10a_8x16b, out11a_8x16b;
        int16x8_t out12a_8x16b, out13a_8x16b, out14a_8x16b, out15a_8x16b;
        int16x8_t tmp0_8x16b, tmp1_8x16b, tmp2_8x16b, tmp3_8x16b;
        int16x8_t tmp4_8x16b, tmp5_8x16b, tmp6_8x16b, tmp7_8x16b;
        int16x8_t tmp8_8x16b, tmp9_8x16b, tmp10_8x16b, tmp11_8x16b;
        int16x8_t tmp12_8x16b, tmp13_8x16b, tmp14_8x16b, tmp15_8x16b;

        /************************* 8x8 Vertical Transform*************************/
        tmp0_8x16b  = vcombine_s16(vget_high_s16(src0_8x16b), vcreate_s32(0));
        tmp2_8x16b  = vcombine_s16(vget_high_s16(src2_8x16b), vcreate_s32(0));
        tmp4_8x16b  = vcombine_s16(vget_high_s16(src4_8x16b), vcreate_s32(0));
        tmp6_8x16b  = vcombine_s16(vget_high_s16(src6_8x16b), vcreate_s32(0));
        tmp8_8x16b  = vcombine_s16(vget_high_s16(src8_8x16b), vcreate_s32(0));
        tmp10_8x16b = vcombine_s16(vget_high_s16(src10_8x16b), vcreate_s32(0));
        tmp12_8x16b = vcombine_s16(vget_high_s16(src12_8x16b), vcreate_s32(0));
        tmp14_8x16b = vcombine_s16(vget_high_s16(src14_8x16b), vcreate_s32(0));

        /*************************First 4 pixels ********************************/
        src0_8x16b  = vmovl_s16(vget_low_s16(src0_8x16b));
        src2_8x16b  = vmovl_s16(vget_low_s16(src2_8x16b));
        src4_8x16b  = vmovl_s16(vget_low_s16(src4_8x16b));
        src6_8x16b  = vmovl_s16(vget_low_s16(src6_8x16b));
        src8_8x16b  = vmovl_s16(vget_low_s16(src8_8x16b));
        src10_8x16b = vmovl_s16(vget_low_s16(src10_8x16b));
        src12_8x16b = vmovl_s16(vget_low_s16(src12_8x16b));
        src14_8x16b = vmovl_s16(vget_low_s16(src14_8x16b));

        /* r0 + r1 */
        pred0_8x16b = vaddq_s32(src0_8x16b, src2_8x16b);
        /* r2 + r3 */
        pred2_8x16b = vaddq_s32(src4_8x16b, src6_8x16b);
        /* r4 + r5 */
        pred4_8x16b = vaddq_s32(src8_8x16b, src10_8x16b);
        /* r6 + r7 */
        pred6_8x16b = vaddq_s32(src12_8x16b, src14_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        out0_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        out8_8x16b  = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        out4_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        out12_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = vsubq_s32(src0_8x16b, src2_8x16b);
        /* r2 - r3 */
        pred2_8x16b = vsubq_s32(src4_8x16b, src6_8x16b);
        /* r4 - r5 */
        pred4_8x16b = vsubq_s32(src8_8x16b, src10_8x16b);
        /* r6 - r7 */
        pred6_8x16b = vsubq_s32(src12_8x16b, src14_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        out2_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        out10_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        out6_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        out14_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);
        /*************************First 4 pixels ********************************/

        /**************************Next 4 pixels *******************************/
        src0_8x16b  = vmovl_s16(vget_low_s16(tmp0_8x16b));
        src2_8x16b  = vmovl_s16(vget_low_s16(tmp2_8x16b));
        src4_8x16b  = vmovl_s16(vget_low_s16(tmp4_8x16b));
        src6_8x16b  = vmovl_s16(vget_low_s16(tmp6_8x16b));
        src8_8x16b  = vmovl_s16(vget_low_s16(tmp8_8x16b));
        src10_8x16b = vmovl_s16(vget_low_s16(tmp10_8x16b));
        src12_8x16b = vmovl_s16(vget_low_s16(tmp12_8x16b));
        src14_8x16b = vmovl_s16(vget_low_s16(tmp14_8x16b));

        /* r0 + r1 */
        pred0_8x16b = vaddq_s32(src0_8x16b, src2_8x16b);
        /* r2 + r3 */
        pred2_8x16b = vaddq_s32(src4_8x16b, src6_8x16b);
        /* r4 + r5 */
        pred4_8x16b = vaddq_s32(src8_8x16b, src10_8x16b);
        /* r6 + r7 */
        pred6_8x16b = vaddq_s32(src12_8x16b, src14_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        out0a_8x16b = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        out8a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b  = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b  = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        out4a_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        out12a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = vsubq_s32(src0_8x16b, src2_8x16b);
        /* r2 - r3 */
        pred2_8x16b = vsubq_s32(src4_8x16b, src6_8x16b);
        /* r4 - r5 */
        pred4_8x16b = vsubq_s32(src8_8x16b, src10_8x16b);
        /* r6 - r7 */
        pred6_8x16b = vsubq_s32(src12_8x16b, src14_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b  = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b  = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        out2a_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        out10a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b  = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b  = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        out6a_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        out14a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);
        /**************************Next 4 pixels *******************************/
        /************************* 8x8 Vertical Transform*************************/

        /************************* 8x8 Vertical Transform*************************/
        tmp1_8x16b  = vcombine_s16(vget_high_s16(src1_8x16b), vcreate_s32(0));
        tmp3_8x16b  = vcombine_s16(vget_high_s16(src3_8x16b), vcreate_s32(0));
        tmp5_8x16b  = vcombine_s16(vget_high_s16(src5_8x16b), vcreate_s32(0));
        tmp7_8x16b  = vcombine_s16(vget_high_s16(src7_8x16b), vcreate_s32(0));
        tmp9_8x16b  = vcombine_s16(vget_high_s16(src9_8x16b), vcreate_s32(0));
        tmp11_8x16b = vcombine_s16(vget_high_s16(src11_8x16b), vcreate_s32(0));
        tmp13_8x16b = vcombine_s16(vget_high_s16(src13_8x16b), vcreate_s32(0));
        tmp15_8x16b = vcombine_s16(vget_high_s16(src15_8x16b), vcreate_s32(0));

        /*************************First 4 pixels ********************************/
        src1_8x16b  = vmovl_s16(vget_low_s16(src1_8x16b));
        src3_8x16b  = vmovl_s16(vget_low_s16(src3_8x16b));
        src5_8x16b  = vmovl_s16(vget_low_s16(src5_8x16b));
        src7_8x16b  = vmovl_s16(vget_low_s16(src7_8x16b));
        src9_8x16b  = vmovl_s16(vget_low_s16(src9_8x16b));
        src11_8x16b = vmovl_s16(vget_low_s16(src11_8x16b));
        src13_8x16b = vmovl_s16(vget_low_s16(src13_8x16b));
        src15_8x16b = vmovl_s16(vget_low_s16(src15_8x16b));

        /* r0 + r1 */
        pred0_8x16b = vaddq_s32(src1_8x16b, src3_8x16b);
        /* r2 + r3 */
        pred2_8x16b = vaddq_s32(src5_8x16b, src7_8x16b);
        /* r4 + r5 */
        pred4_8x16b = vaddq_s32(src9_8x16b, src11_8x16b);
        /* r6 + r7 */
        pred6_8x16b = vaddq_s32(src13_8x16b, src15_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        out1_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        out9_8x16b  = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        out5_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        out13_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = vsubq_s32(src1_8x16b, src3_8x16b);
        /* r2 - r3 */
        pred2_8x16b = vsubq_s32(src5_8x16b, src7_8x16b);
        /* r4 - r5 */
        pred4_8x16b = vsubq_s32(src9_8x16b, src11_8x16b);
        /* r6 - r7 */
        pred6_8x16b = vsubq_s32(src13_8x16b, src15_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        out3_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        out11_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        out7_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        out15_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);
        /*************************First 4 pixels ********************************/

        /*************************Next 4 pixels ********************************/
        src1_8x16b  = vmovl_s16(vget_low_s16(tmp1_8x16b));
        src3_8x16b  = vmovl_s16(vget_low_s16(tmp3_8x16b));
        src5_8x16b  = vmovl_s16(vget_low_s16(tmp5_8x16b));
        src7_8x16b  = vmovl_s16(vget_low_s16(tmp7_8x16b));
        src9_8x16b  = vmovl_s16(vget_low_s16(tmp9_8x16b));
        src11_8x16b = vmovl_s16(vget_low_s16(tmp11_8x16b));
        src13_8x16b = vmovl_s16(vget_low_s16(tmp13_8x16b));
        src15_8x16b = vmovl_s16(vget_low_s16(tmp15_8x16b));

        /* r0 + r1 */
        pred0_8x16b = vaddq_s32(src1_8x16b, src3_8x16b);
        /* r2 + r3 */
        pred2_8x16b = vaddq_s32(src5_8x16b, src7_8x16b);
        /* r4 + r5 */
        pred4_8x16b = vaddq_s32(src9_8x16b, src11_8x16b);
        /* r6 + r7 */
        pred6_8x16b = vaddq_s32(src13_8x16b, src15_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        out1a_8x16b = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        out9a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b  = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b  = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        out5a_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        out13a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = vsubq_s32(src1_8x16b, src3_8x16b);
        /* r2 - r3 */
        pred2_8x16b = vsubq_s32(src5_8x16b, src7_8x16b);
        /* r4 - r5 */
        pred4_8x16b = vsubq_s32(src9_8x16b, src11_8x16b);
        /* r6 - r7 */
        pred6_8x16b = vsubq_s32(src13_8x16b, src15_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b  = vaddq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b  = vaddq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        out3a_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        out11a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b  = vsubq_s32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b  = vsubq_s32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        out7a_8x16b  = vaddq_s32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        out15a_8x16b = vsubq_s32(pred1_8x16b, pred5_8x16b);
        /************************* 8x8 Vertical Transform*************************/

        /****************************SATD calculation ****************************/
        src0_8x16b = vabsq_s32(out0_8x16b);
        src1_8x16b = vabsq_s32(out1_8x16b);
        src2_8x16b = vabsq_s32(out2_8x16b);
        src3_8x16b = vabsq_s32(out3_8x16b);
        src4_8x16b = vabsq_s32(out4_8x16b);
        src5_8x16b = vabsq_s32(out5_8x16b);
        src6_8x16b = vabsq_s32(out6_8x16b);
        src7_8x16b = vabsq_s32(out7_8x16b);

        s32* p = (s32*)&src0_8x16b;
        p[0]   = p[0] >> 2;

        satd = vaddvq_s32(src0_8x16b);
        satd += vaddvq_s32(src1_8x16b);
        satd += vaddvq_s32(src2_8x16b);
        satd += vaddvq_s32(src3_8x16b);
        satd += vaddvq_s32(src4_8x16b);
        satd += vaddvq_s32(src5_8x16b);
        satd += vaddvq_s32(src6_8x16b);
        satd += vaddvq_s32(src7_8x16b);

        src0_8x16b = vabsq_s32(out8_8x16b);
        src1_8x16b = vabsq_s32(out9_8x16b);
        src2_8x16b = vabsq_s32(out10_8x16b);
        src3_8x16b = vabsq_s32(out11_8x16b);
        src4_8x16b = vabsq_s32(out12_8x16b);
        src5_8x16b = vabsq_s32(out13_8x16b);
        src6_8x16b = vabsq_s32(out14_8x16b);
        src7_8x16b = vabsq_s32(out15_8x16b);

        satd += vaddvq_s32(src0_8x16b);
        satd += vaddvq_s32(src1_8x16b);
        satd += vaddvq_s32(src2_8x16b);
        satd += vaddvq_s32(src3_8x16b);
        satd += vaddvq_s32(src4_8x16b);
        satd += vaddvq_s32(src5_8x16b);
        satd += vaddvq_s32(src6_8x16b);
        satd += vaddvq_s32(src7_8x16b);

        src0_8x16b = vabsq_s32(out0a_8x16b);
        src1_8x16b = vabsq_s32(out1a_8x16b);
        src2_8x16b = vabsq_s32(out2a_8x16b);
        src3_8x16b = vabsq_s32(out3a_8x16b);
        src4_8x16b = vabsq_s32(out4a_8x16b);
        src5_8x16b = vabsq_s32(out5a_8x16b);
        src6_8x16b = vabsq_s32(out6a_8x16b);
        src7_8x16b = vabsq_s32(out7a_8x16b);

        satd += vaddvq_s32(src0_8x16b);
        satd += vaddvq_s32(src1_8x16b);
        satd += vaddvq_s32(src2_8x16b);
        satd += vaddvq_s32(src3_8x16b);
        satd += vaddvq_s32(src4_8x16b);
        satd += vaddvq_s32(src5_8x16b);
        satd += vaddvq_s32(src6_8x16b);
        satd += vaddvq_s32(src7_8x16b);

        src0_8x16b = vabsq_s32(out8a_8x16b);
        src1_8x16b = vabsq_s32(out9a_8x16b);
        src2_8x16b = vabsq_s32(out10a_8x16b);
        src3_8x16b = vabsq_s32(out11a_8x16b);
        src4_8x16b = vabsq_s32(out12a_8x16b);
        src5_8x16b = vabsq_s32(out13a_8x16b);
        src6_8x16b = vabsq_s32(out14a_8x16b);
        src7_8x16b = vabsq_s32(out15a_8x16b);

        satd += vaddvq_s32(src0_8x16b);
        satd += vaddvq_s32(src1_8x16b);
        satd += vaddvq_s32(src2_8x16b);
        satd += vaddvq_s32(src3_8x16b);
        satd += vaddvq_s32(src4_8x16b);
        satd += vaddvq_s32(src5_8x16b);
        satd += vaddvq_s32(src6_8x16b);
        satd += vaddvq_s32(src7_8x16b);

        satd = (int)(satd / sqrt(16.0 * 8) * 2);
        return satd;
    }
    else {
        int  k, i, j, jj;
        int  satd = 0;
        int  sub[128], interm1[16][8], interm2[16][8];
        pel *orgn = org, *curn = cur;

        for(k = 0; k < 128; k += 8) {
            sub[k + 0] = orgn[0] - curn[0];
            sub[k + 1] = orgn[1] - curn[1];
            sub[k + 2] = orgn[2] - curn[2];
            sub[k + 3] = orgn[3] - curn[3];
            sub[k + 4] = orgn[4] - curn[4];
            sub[k + 5] = orgn[5] - curn[5];
            sub[k + 6] = orgn[6] - curn[6];
            sub[k + 7] = orgn[7] - curn[7];

            curn += s_cur;
            orgn += s_org;
        }

        for(j = 0; j < 16; j++) {
            jj = j << 3;

            interm2[j][0] = sub[jj] + sub[jj + 4];
            interm2[j][1] = sub[jj + 1] + sub[jj + 5];
            interm2[j][2] = sub[jj + 2] + sub[jj + 6];
            interm2[j][3] = sub[jj + 3] + sub[jj + 7];
            interm2[j][4] = sub[jj] - sub[jj + 4];
            interm2[j][5] = sub[jj + 1] - sub[jj + 5];
            interm2[j][6] = sub[jj + 2] - sub[jj + 6];
            interm2[j][7] = sub[jj + 3] - sub[jj + 7];

            interm1[j][0] = interm2[j][0] + interm2[j][2];
            interm1[j][1] = interm2[j][1] + interm2[j][3];
            interm1[j][2] = interm2[j][0] - interm2[j][2];
            interm1[j][3] = interm2[j][1] - interm2[j][3];
            interm1[j][4] = interm2[j][4] + interm2[j][6];
            interm1[j][5] = interm2[j][5] + interm2[j][7];
            interm1[j][6] = interm2[j][4] - interm2[j][6];
            interm1[j][7] = interm2[j][5] - interm2[j][7];

            interm2[j][0] = interm1[j][0] + interm1[j][1];
            interm2[j][1] = interm1[j][0] - interm1[j][1];
            interm2[j][2] = interm1[j][2] + interm1[j][3];
            interm2[j][3] = interm1[j][2] - interm1[j][3];
            interm2[j][4] = interm1[j][4] + interm1[j][5];
            interm2[j][5] = interm1[j][4] - interm1[j][5];
            interm2[j][6] = interm1[j][6] + interm1[j][7];
            interm2[j][7] = interm1[j][6] - interm1[j][7];
        }

        for(i = 0; i < 8; i++) {
            interm1[0][i]  = interm2[0][i] + interm2[8][i];
            interm1[1][i]  = interm2[1][i] + interm2[9][i];
            interm1[2][i]  = interm2[2][i] + interm2[10][i];
            interm1[3][i]  = interm2[3][i] + interm2[11][i];
            interm1[4][i]  = interm2[4][i] + interm2[12][i];
            interm1[5][i]  = interm2[5][i] + interm2[13][i];
            interm1[6][i]  = interm2[6][i] + interm2[14][i];
            interm1[7][i]  = interm2[7][i] + interm2[15][i];
            interm1[8][i]  = interm2[0][i] - interm2[8][i];
            interm1[9][i]  = interm2[1][i] - interm2[9][i];
            interm1[10][i] = interm2[2][i] - interm2[10][i];
            interm1[11][i] = interm2[3][i] - interm2[11][i];
            interm1[12][i] = interm2[4][i] - interm2[12][i];
            interm1[13][i] = interm2[5][i] - interm2[13][i];
            interm1[14][i] = interm2[6][i] - interm2[14][i];
            interm1[15][i] = interm2[7][i] - interm2[15][i];

            interm2[0][i]  = interm1[0][i] + interm1[4][i];
            interm2[1][i]  = interm1[1][i] + interm1[5][i];
            interm2[2][i]  = interm1[2][i] + interm1[6][i];
            interm2[3][i]  = interm1[3][i] + interm1[7][i];
            interm2[4][i]  = interm1[0][i] - interm1[4][i];
            interm2[5][i]  = interm1[1][i] - interm1[5][i];
            interm2[6][i]  = interm1[2][i] - interm1[6][i];
            interm2[7][i]  = interm1[3][i] - interm1[7][i];
            interm2[8][i]  = interm1[8][i] + interm1[12][i];
            interm2[9][i]  = interm1[9][i] + interm1[13][i];
            interm2[10][i] = interm1[10][i] + interm1[14][i];
            interm2[11][i] = interm1[11][i] + interm1[15][i];
            interm2[12][i] = interm1[8][i] - interm1[12][i];
            interm2[13][i] = interm1[9][i] - interm1[13][i];
            interm2[14][i] = interm1[10][i] - interm1[14][i];
            interm2[15][i] = interm1[11][i] - interm1[15][i];

            interm1[0][i]  = interm2[0][i] + interm2[2][i];
            interm1[1][i]  = interm2[1][i] + interm2[3][i];
            interm1[2][i]  = interm2[0][i] - interm2[2][i];
            interm1[3][i]  = interm2[1][i] - interm2[3][i];
            interm1[4][i]  = interm2[4][i] + interm2[6][i];
            interm1[5][i]  = interm2[5][i] + interm2[7][i];
            interm1[6][i]  = interm2[4][i] - interm2[6][i];
            interm1[7][i]  = interm2[5][i] - interm2[7][i];
            interm1[8][i]  = interm2[8][i] + interm2[10][i];
            interm1[9][i]  = interm2[9][i] + interm2[11][i];
            interm1[10][i] = interm2[8][i] - interm2[10][i];
            interm1[11][i] = interm2[9][i] - interm2[11][i];
            interm1[12][i] = interm2[12][i] + interm2[14][i];
            interm1[13][i] = interm2[13][i] + interm2[15][i];
            interm1[14][i] = interm2[12][i] - interm2[14][i];
            interm1[15][i] = interm2[13][i] - interm2[15][i];

            interm2[0][i]  = XEVE_ABS(interm1[0][i] + interm1[1][i]);
            interm2[1][i]  = XEVE_ABS(interm1[0][i] - interm1[1][i]);
            interm2[2][i]  = XEVE_ABS(interm1[2][i] + interm1[3][i]);
            interm2[3][i]  = XEVE_ABS(interm1[2][i] - interm1[3][i]);
            interm2[4][i]  = XEVE_ABS(interm1[4][i] + interm1[5][i]);
            interm2[5][i]  = XEVE_ABS(interm1[4][i] - interm1[5][i]);
            interm2[6][i]  = XEVE_ABS(interm1[6][i] + interm1[7][i]);
            interm2[7][i]  = XEVE_ABS(interm1[6][i] - interm1[7][i]);
            interm2[8][i]  = XEVE_ABS(interm1[8][i] + interm1[9][i]);
            interm2[9][i]  = XEVE_ABS(interm1[8][i] - interm1[9][i]);
            interm2[10][i] = XEVE_ABS(interm1[10][i] + interm1[11][i]);
            interm2[11][i] = XEVE_ABS(interm1[10][i] - interm1[11][i]);
            interm2[12][i] = XEVE_ABS(interm1[12][i] + interm1[13][i]);
            interm2[13][i] = XEVE_ABS(interm1[12][i] - interm1[13][i]);
            interm2[14][i] = XEVE_ABS(interm1[14][i] + interm1[15][i]);
            interm2[15][i] = XEVE_ABS(interm1[14][i] - interm1[15][i]);
        }

        satd = interm2[0][0] >> 2;
        for(j = 1; j < 8; j++) {
            satd += interm2[0][j];
        }
        for(i = 1; i < 16; i++) {
            for(j = 0; j < 8; j++) {
                satd += interm2[i][j];
            }
        }

        satd = (int)(satd / (2.0 * sqrt(8.0)));
        return satd;
    }
}

int xeve_had_8x4_neon(pel* org, pel* cur, int s_org, int s_cur, int step, int bit_depth)
{
    if(bit_depth == 10) {
        int       k, i;
        int       satd = 0;
        int16x8_t m1[8], m2[8];
        int16x8_t vzero   = vdupq_n_s16(0);
        int16x4_t vzero_4 = vdup_n_s16(0);
        int16x8_t sum;

        for(k = 0; k < 4; k++) {
            int16x8_t r0 = (vld1q_s16(&org[0]));
            int16x8_t r1 = (vld1q_s16(&cur[0]));
            m1[k]        = vsubq_s16(r0, r1);
            org += s_org;
            cur += s_cur;
        }

        // vertical
        m2[0] = vaddq_s16(m1[0], m1[2]);
        m2[1] = vaddq_s16(m1[1], m1[3]);
        m2[2] = vsubq_s16(m1[0], m1[2]);
        m2[3] = vsubq_s16(m1[1], m1[3]);

        m1[0] = vaddq_s16(m2[0], m2[1]);
        m1[1] = vsubq_s16(m2[0], m2[1]);
        m1[2] = vaddq_s16(m2[2], m2[3]);
        m1[3] = vsubq_s16(m2[2], m2[3]);

        // transpose, partially
        m2[0] = vzip1q_s16(m1[0], m1[1]);
        m2[1] = vzip1q_s16(m1[2], m1[3]);
        m2[2] = vzip2q_s16(m1[0], m1[1]);
        m2[3] = vzip2q_s16(m1[2], m1[3]);

        m1[0] = vzip1q_s32(m2[0], m2[1]);
        m1[1] = vzip2q_s32(m2[0], m2[1]);
        m1[2] = vzip1q_s32(m2[2], m2[3]);
        m1[3] = vzip2q_s32(m2[2], m2[3]);

        // horizontal
        // finish transpose
        m2[0] = vcombine_s64(vget_low_s64(m1[0]), vzero_4);
        m2[1] = vcombine_s64(vget_high_s64(m1[0]), vzero_4);
        m2[2] = vcombine_s64(vget_low_s64(m1[1]), vzero_4);
        m2[3] = vcombine_s64(vget_high_s64(m1[1]), vzero_4);
        m2[4] = vcombine_s64(vget_low_s64(m1[2]), vzero_4);
        m2[5] = vcombine_s64(vget_high_s64(m1[2]), vzero_4);
        m2[6] = vcombine_s64(vget_low_s64(m1[3]), vzero_4);
        m2[7] = vcombine_s64(vget_high_s64(m1[3]), vzero_4);

        for(i = 0; i < 8; i++) {
            m2[i] = vmovl_s16(vget_low_s16(m2[i]));
        }

        m1[0] = vaddq_s32(m2[0], m2[4]);
        m1[1] = vaddq_s32(m2[1], m2[5]);
        m1[2] = vaddq_s32(m2[2], m2[6]);
        m1[3] = vaddq_s32(m2[3], m2[7]);
        m1[4] = vsubq_s32(m2[0], m2[4]);
        m1[5] = vsubq_s32(m2[1], m2[5]);
        m1[6] = vsubq_s32(m2[2], m2[6]);
        m1[7] = vsubq_s32(m2[3], m2[7]);

        m2[0] = vaddq_s32(m1[0], m1[2]);
        m2[1] = vaddq_s32(m1[1], m1[3]);
        m2[2] = vsubq_s32(m1[0], m1[2]);
        m2[3] = vsubq_s32(m1[1], m1[3]);
        m2[4] = vaddq_s32(m1[4], m1[6]);
        m2[5] = vaddq_s32(m1[5], m1[7]);
        m2[6] = vsubq_s32(m1[4], m1[6]);
        m2[7] = vsubq_s32(m1[5], m1[7]);

        m1[0] = vabsq_s32(vaddq_s32(m2[0], m2[1]));
        m1[1] = vabsq_s32(vsubq_s32(m2[0], m2[1]));
        m1[2] = vabsq_s32(vaddq_s32(m2[2], m2[3]));
        m1[3] = vabsq_s32(vsubq_s32(m2[2], m2[3]));
        m1[4] = vabsq_s32(vaddq_s32(m2[4], m2[5]));
        m1[5] = vabsq_s32(vsubq_s32(m2[4], m2[5]));
        m1[6] = vabsq_s32(vaddq_s32(m2[6], m2[7]));
        m1[7] = vabsq_s32(vsubq_s32(m2[6], m2[7]));

        s32* p = (s32*)&m1[0];
        p[0]   = p[0] >> 2;

        satd = vaddvq_s32(m1[0]);
        satd += vaddvq_s32(m1[1]);
        satd += vaddvq_s32(m1[2]);
        satd += vaddvq_s32(m1[3]);
        satd += vaddvq_s32(m1[4]);
        satd += vaddvq_s32(m1[5]);
        satd += vaddvq_s32(m1[6]);
        satd += vaddvq_s32(m1[7]);

        satd = (int)(satd / sqrt(4.0 * 8) * 2);

        return satd;
    }
    else {
        int  k, i, j, jj;
        int  satd = 0;
        int  sub[32], interm1[4][8], interm2[4][8];
        pel *orgn = org, *curn = cur;

        for(k = 0; k < 32; k += 8) {
            sub[k + 0] = orgn[0] - curn[0];
            sub[k + 1] = orgn[1] - curn[1];
            sub[k + 2] = orgn[2] - curn[2];
            sub[k + 3] = orgn[3] - curn[3];
            sub[k + 4] = orgn[4] - curn[4];
            sub[k + 5] = orgn[5] - curn[5];
            sub[k + 6] = orgn[6] - curn[6];
            sub[k + 7] = orgn[7] - curn[7];

            curn += s_cur;
            orgn += s_org;
        }

        for(j = 0; j < 4; j++) {
            jj = j << 3;

            interm2[j][0] = sub[jj] + sub[jj + 4];
            interm2[j][1] = sub[jj + 1] + sub[jj + 5];
            interm2[j][2] = sub[jj + 2] + sub[jj + 6];
            interm2[j][3] = sub[jj + 3] + sub[jj + 7];
            interm2[j][4] = sub[jj] - sub[jj + 4];
            interm2[j][5] = sub[jj + 1] - sub[jj + 5];
            interm2[j][6] = sub[jj + 2] - sub[jj + 6];
            interm2[j][7] = sub[jj + 3] - sub[jj + 7];

            interm1[j][0] = interm2[j][0] + interm2[j][2];
            interm1[j][1] = interm2[j][1] + interm2[j][3];
            interm1[j][2] = interm2[j][0] - interm2[j][2];
            interm1[j][3] = interm2[j][1] - interm2[j][3];
            interm1[j][4] = interm2[j][4] + interm2[j][6];
            interm1[j][5] = interm2[j][5] + interm2[j][7];
            interm1[j][6] = interm2[j][4] - interm2[j][6];
            interm1[j][7] = interm2[j][5] - interm2[j][7];

            interm2[j][0] = interm1[j][0] + interm1[j][1];
            interm2[j][1] = interm1[j][0] - interm1[j][1];
            interm2[j][2] = interm1[j][2] + interm1[j][3];
            interm2[j][3] = interm1[j][2] - interm1[j][3];
            interm2[j][4] = interm1[j][4] + interm1[j][5];
            interm2[j][5] = interm1[j][4] - interm1[j][5];
            interm2[j][6] = interm1[j][6] + interm1[j][7];
            interm2[j][7] = interm1[j][6] - interm1[j][7];
        }

        for(i = 0; i < 8; i++) {
            interm1[0][i] = interm2[0][i] + interm2[2][i];
            interm1[1][i] = interm2[1][i] + interm2[3][i];
            interm1[2][i] = interm2[0][i] - interm2[2][i];
            interm1[3][i] = interm2[1][i] - interm2[3][i];

            interm2[0][i] = XEVE_ABS(interm1[0][i] + interm1[1][i]);
            interm2[1][i] = XEVE_ABS(interm1[0][i] - interm1[1][i]);
            interm2[2][i] = XEVE_ABS(interm1[2][i] + interm1[3][i]);
            interm2[3][i] = XEVE_ABS(interm1[2][i] - interm1[3][i]);
        }

        satd = interm2[0][0] >> 2;
        for(j = 1; j < 8; j++) {
            satd += interm2[0][j];
        }
        for(i = 1; i < 4; i++) {
            for(j = 0; j < 8; j++) {
                satd += interm2[i][j];
            }
        }

        satd = (int)(satd / sqrt(8.0));
        return satd;
    }
}

int xeve_had_4x8_neon(pel* org, pel* cur, int s_org, int s_cur, int step, int bit_depth)
{
    if(bit_depth == 10) {
        int       k, i;
        int16x8_t m1[8], m2[8];
        int16x4_t m1_4[8], m2_4[8];
        int16x8_t n1[4][2];
        int16x8_t n2[4][2];
        int16x8_t sum;
        int       satd  = 0;
        int16x4_t vzero = vdup_n_s16(0);

        for(k = 0; k < 8; k++) {
            int16x4_t r0 = (vld1_s16(&org[0]));
            int16x4_t r1 = (vld1_s16(&cur[0]));
            m2_4[k]      = (vsub_s16(r0, r1));
            org += s_org;
            cur += s_cur;
        }

        // vertical
        m1_4[0] = vadd_s16(m2_4[0], m2_4[4]);
        m1_4[1] = vadd_s16(m2_4[1], m2_4[5]);
        m1_4[2] = vadd_s16(m2_4[2], m2_4[6]);
        m1_4[3] = vadd_s16(m2_4[3], m2_4[7]);
        m1_4[4] = vsub_s16(m2_4[0], m2_4[4]);
        m1_4[5] = vsub_s16(m2_4[1], m2_4[5]);
        m1_4[6] = vsub_s16(m2_4[2], m2_4[6]);
        m1_4[7] = vsub_s16(m2_4[3], m2_4[7]);

        m2_4[0] = vadd_s16(m1_4[0], m1_4[2]);
        m2_4[1] = vadd_s16(m1_4[1], m1_4[3]);
        m2_4[2] = vsub_s16(m1_4[0], m1_4[2]);
        m2_4[3] = vsub_s16(m1_4[1], m1_4[3]);
        m2_4[4] = vadd_s16(m1_4[4], m1_4[6]);
        m2_4[5] = vadd_s16(m1_4[5], m1_4[7]);
        m2_4[6] = vsub_s16(m1_4[4], m1_4[6]);
        m2_4[7] = vsub_s16(m1_4[5], m1_4[7]);

        m1_4[0] = vadd_s16(m2_4[0], m2_4[1]);
        m1_4[1] = vsub_s16(m2_4[0], m2_4[1]);
        m1_4[2] = vadd_s16(m2_4[2], m2_4[3]);
        m1_4[3] = vsub_s16(m2_4[2], m2_4[3]);
        m1_4[4] = vadd_s16(m2_4[4], m2_4[5]);
        m1_4[5] = vsub_s16(m2_4[4], m2_4[5]);
        m1_4[6] = vadd_s16(m2_4[6], m2_4[7]);
        m1_4[7] = vsub_s16(m2_4[6], m2_4[7]);

        // horizontal
        // transpose

        int16x4x2_t temp = vzip_s16(m1_4[0], m1_4[1]);
        m2[0]            = vcombine_s16(temp.val[0], temp.val[1]);
        temp             = vzip_s16(m1_4[2], m1_4[3]);
        m2[1]            = vcombine_s16(temp.val[0], temp.val[1]);
        temp             = vzip_s16(m1_4[4], m1_4[5]);
        m2[2]            = vcombine_s16(temp.val[0], temp.val[1]);
        temp             = vzip_s16(m1_4[6], m1_4[7]);
        m2[3]            = vcombine_s16(temp.val[0], temp.val[1]);

        m1[0] = vzip1q_s32(m2[0], m2[1]);
        m1[1] = vzip2q_s32(m2[0], m2[1]);
        m1[2] = vzip1q_s32(m2[2], m2[3]);
        m1[3] = vzip2q_s32(m2[2], m2[3]);

        m2[0] = vzip1q_s32(m1[0], m1[2]);
        m2[1] = vzip2q_s32(m1[0], m1[2]);
        m2[2] = vzip1q_s32(m1[1], m1[3]);
        m2[3] = vzip2q_s32(m1[1], m1[3]);

        for(i = 0; i < 4; i++) {
            n1[i][0] = vmovl_s16(vget_low_s16(m2[i]));
            n1[i][1] = vmovl_s16(vget_high_s16(m2[i]));
        }

        for(i = 0; i < 2; i++) {
            n2[0][i] = vaddq_s32(n1[0][i], n1[2][i]);
            n2[1][i] = vaddq_s32(n1[1][i], n1[3][i]);
            n2[2][i] = vsubq_s32(n1[0][i], n1[2][i]);
            n2[3][i] = vsubq_s32(n1[1][i], n1[3][i]);

            n1[0][i] = vabsq_s32(vaddq_s32(n2[0][i], n2[1][i]));
            n1[1][i] = vabsq_s32(vsubq_s32(n2[0][i], n2[1][i]));
            n1[2][i] = vabsq_s32(vaddq_s32(n2[2][i], n2[3][i]));
            n1[3][i] = vabsq_s32(vsubq_s32(n2[2][i], n2[3][i]));
        }

        s32* p = (s32*)&n1[0][0];
        p[0]   = p[0] >> 2;

        satd = vaddvq_s32(n1[0][0]);
        satd += vaddvq_s32(n1[0][1]);
        satd += vaddvq_s32(n1[1][0]);
        satd += vaddvq_s32(n1[1][1]);
        satd += vaddvq_s32(n1[2][0]);
        satd += vaddvq_s32(n1[2][1]);
        satd += vaddvq_s32(n1[3][0]);
        satd += vaddvq_s32(n1[3][1]);

        satd = (int)(satd / sqrt(4.0 * 8) * 2);

        return satd;
    }
    else {
        int  k, i, j, jj;
        int  satd = 0;
        int  sub[32], interm1[8][4], interm2[8][4];
        pel *curn = cur, *orgn = org;

        for(k = 0; k < 32; k += 4) {
            sub[k + 0] = orgn[0] - curn[0];
            sub[k + 1] = orgn[1] - curn[1];
            sub[k + 2] = orgn[2] - curn[2];
            sub[k + 3] = orgn[3] - curn[3];

            curn += s_cur;
            orgn += s_org;
        }

        for(j = 0; j < 8; j++) {
            jj            = j << 2;
            interm2[j][0] = sub[jj] + sub[jj + 2];
            interm2[j][1] = sub[jj + 1] + sub[jj + 3];
            interm2[j][2] = sub[jj] - sub[jj + 2];
            interm2[j][3] = sub[jj + 1] - sub[jj + 3];

            interm1[j][0] = interm2[j][0] + interm2[j][1];
            interm1[j][1] = interm2[j][0] - interm2[j][1];
            interm1[j][2] = interm2[j][2] + interm2[j][3];
            interm1[j][3] = interm2[j][2] - interm2[j][3];
        }

        for(i = 0; i < 4; i++) {
            interm2[0][i] = interm1[0][i] + interm1[4][i];
            interm2[1][i] = interm1[1][i] + interm1[5][i];
            interm2[2][i] = interm1[2][i] + interm1[6][i];
            interm2[3][i] = interm1[3][i] + interm1[7][i];
            interm2[4][i] = interm1[0][i] - interm1[4][i];
            interm2[5][i] = interm1[1][i] - interm1[5][i];
            interm2[6][i] = interm1[2][i] - interm1[6][i];
            interm2[7][i] = interm1[3][i] - interm1[7][i];

            interm1[0][i] = interm2[0][i] + interm2[2][i];
            interm1[1][i] = interm2[1][i] + interm2[3][i];
            interm1[2][i] = interm2[0][i] - interm2[2][i];
            interm1[3][i] = interm2[1][i] - interm2[3][i];
            interm1[4][i] = interm2[4][i] + interm2[6][i];
            interm1[5][i] = interm2[5][i] + interm2[7][i];
            interm1[6][i] = interm2[4][i] - interm2[6][i];
            interm1[7][i] = interm2[5][i] - interm2[7][i];

            interm2[0][i] = XEVE_ABS(interm1[0][i] + interm1[1][i]);
            interm2[1][i] = XEVE_ABS(interm1[0][i] - interm1[1][i]);
            interm2[2][i] = XEVE_ABS(interm1[2][i] + interm1[3][i]);
            interm2[3][i] = XEVE_ABS(interm1[2][i] - interm1[3][i]);
            interm2[4][i] = XEVE_ABS(interm1[4][i] + interm1[5][i]);
            interm2[5][i] = XEVE_ABS(interm1[4][i] - interm1[5][i]);
            interm2[6][i] = XEVE_ABS(interm1[6][i] + interm1[7][i]);
            interm2[7][i] = XEVE_ABS(interm1[6][i] - interm1[7][i]);
        }

        satd = interm2[0][0] >> 2;
        for(j = 1; j < 4; j++) {
            satd += interm2[0][j];
        }
        for(i = 1; i < 8; i++) {
            for(j = 0; j < 4; j++) {
                satd += interm2[i][j];
            }
        }

        satd = (int)(satd / sqrt(8.0));
        return satd;
    }
}

int xeve_had_neon(int w, int h, void* o, void* c, int s_org, int s_cur, int bit_depth)
{
    pel* org = o;
    pel* cur = c;
    int  x, y;
    int  sum  = 0;
    int  step = 1;

    if(w > h && (h & 7) == 0 && (w & 15) == 0) {
        int offset_org = s_org << 3;
        int offset_cur = s_cur << 3;

        for(y = 0; y < h; y += 8) {
            for(x = 0; x < w; x += 16) {
                sum += xeve_had_16x8_neon(&org[x], &cur[x], s_org, s_cur, step, bit_depth);
            }
            org += offset_org;
            cur += offset_cur;
        }
    }
    else if(w < h && (w & 7) == 0 && (h & 15) == 0) {
        int offset_org = s_org << 4;
        int offset_cur = s_cur << 4;

        for(y = 0; y < h; y += 16) {
            for(x = 0; x < w; x += 8) {
                sum += xeve_had_8x16_neon(&org[x], &cur[x], s_org, s_cur, step, bit_depth);
            }
            org += offset_org;
            cur += offset_cur;
        }
    }
    else if(w > h && (h & 3) == 0 && (w & 7) == 0) {
        int offset_org = s_org << 2;
        int offset_cur = s_cur << 2;

        for(y = 0; y < h; y += 4) {
            for(x = 0; x < w; x += 8) {
                sum += xeve_had_8x4_neon(&org[x], &cur[x], s_org, s_cur, step, bit_depth);
            }
            org += offset_org;
            cur += offset_cur;
        }
    }
    else if(w < h && (w & 3) == 0 && (h & 7) == 0) {
        int offset_org = s_org << 3;
        int offset_cur = s_cur << 3;

        for(y = 0; y < h; y += 8) {
            for(x = 0; x < w; x += 4) {
                sum += xeve_had_4x8_neon(&org[x], &cur[x], s_org, s_cur, step, bit_depth);
            }
            org += offset_org;
            cur += offset_cur;
        }
    }
    else if((w % 8 == 0) && (h % 8 == 0)) {
        int offset_org = s_org << 3;
        int offset_cur = s_cur << 3;

        for(y = 0; y < h; y += 8) {
            for(x = 0; x < w; x += 8) {
                sum += xeve_had_8x8_neon(&org[x], &cur[x * step], s_org, s_cur, step, bit_depth);
            }
            org += offset_org;
            cur += offset_cur;
        }
    }
    else if((w % 4 == 0) && (h % 4 == 0)) {
        int offset_org = s_org << 2;
        int offset_cur = s_cur << 2;

        for(y = 0; y < h; y += 4) {
            for(x = 0; x < w; x += 4) {
                sum += xeve_had_4x4_neon(&org[x], &cur[x * step], s_org, s_cur, step, bit_depth);
            }
            org += offset_org;
            cur += offset_cur;
        }
    }
    else if((w % 2 == 0) && (h % 2 == 0)) {
        int offset_org = s_org << 1;
        int offset_cur = s_cur << 1;

        for(y = 0; y < h; y += 2) {
            for(x = 0; x < w; x += 2) {
                sum += xeve_had_2x2(&org[x], &cur[x * step], s_org, s_cur, step);
            }
            org += offset_org;
            cur += offset_cur;
        }
    }
    else {
        xeve_assert(0);
    }

    return (sum >> (bit_depth - 8));
}

const XEVE_FN_SATD xeve_tbl_satd_16b_neon[1] = {
    xeve_had_neon,
};

#endif
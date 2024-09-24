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

/* SAD for 16bit **************************************************************/
#define SSE_SAD_16B_4PEL(src1, src2, s00, s01, sac0) \
    s00 = _mm_loadl_epi64((__m128i *)(src1));        \
    s01 = _mm_loadl_epi64((__m128i *)(src2));        \
    s00 = _mm_sub_epi16(s00, s01);                   \
    s00 = _mm_abs_epi16(s00);                        \
    s00 = _mm_cvtepi16_epi32(s00);                   \
                                                     \
    sac0 = _mm_add_epi32(sac0, s00);

int sad_16b_sse_4x2(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    int     sad;
    s16    *s1;
    s16    *s2;
    __m128i s00, s01, sac0;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    sac0 = _mm_setzero_si128();

    SSE_SAD_16B_4PEL(s1, s2, s00, s01, sac0);
    SSE_SAD_16B_4PEL(s1 + s_src1, s2 + s_src2, s00, s01, sac0);

    sac0 = _mm_hadd_epi32(sac0, sac0);
    sac0 = _mm_hadd_epi32(sac0, sac0);
    sad  = _mm_extract_epi32(sac0, 0);

    return (sad >> (bit_depth - 8));
}

int sad_16b_sse_4x2n(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    int     sad;
    s16    *s1;
    s16    *s2;
    __m128i s00, s01, sac0;
    int     i;
    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    sac0 = _mm_setzero_si128();

    for(i = 0; i < h >> 1; i++) {
        SSE_SAD_16B_4PEL(s1, s2, s00, s01, sac0);
        SSE_SAD_16B_4PEL(s1 + s_src1, s2 + s_src2, s00, s01, sac0);
        s1 += s_src1 << 1;
        s2 += s_src2 << 1;
    }

    sac0 = _mm_hadd_epi32(sac0, sac0);
    sac0 = _mm_hadd_epi32(sac0, sac0);
    sad  = _mm_extract_epi32(sac0, 0);

    return (sad >> (bit_depth - 8));
}

int sad_16b_sse_4x4(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    int     sad;
    s16    *s1;
    s16    *s2;
    __m128i s00, s01, sac0;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    sac0 = _mm_setzero_si128();

    SSE_SAD_16B_4PEL(s1, s2, s00, s01, sac0);
    SSE_SAD_16B_4PEL(s1 + s_src1, s2 + s_src2, s00, s01, sac0);
    SSE_SAD_16B_4PEL(s1 + (s_src1 * 2), s2 + (s_src2 * 2), s00, s01, sac0);
    SSE_SAD_16B_4PEL(s1 + (s_src1 * 3), s2 + (s_src2 * 3), s00, s01, sac0);

    sac0 = _mm_hadd_epi32(sac0, sac0);
    sac0 = _mm_hadd_epi32(sac0, sac0);
    sad  = _mm_extract_epi32(sac0, 0);

    return (sad >> (bit_depth - 8));
}

int sad_16b_sse_8x2n(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    __m128i src_8x16b;
    __m128i src_8x16b_1;

    __m128i pred_8x16b;
    __m128i pred_8x16b_1;

    __m128i temp;
    __m128i temp_1;
    __m128i temp_2;

    __m128i temp_dummy;
    __m128i result;

    short *pu2_inp, *pu2_inp2;
    short *pu2_ref, *pu2_ref2;

    int i, j;
    int sad       = 0;
    int s_src1_t2 = s_src1 * 2;
    int s_src2_t2 = s_src2 * 2;

    assert(bit_depth <= 14);
    assert(!(w & 7)); /* width has to be multiple of 4  */
    assert(!(h & 3)); /* height has to be multiple of 4 */

    pu2_inp  = src1;
    pu2_ref  = src2;
    pu2_inp2 = (short *)src1 + s_src1;
    pu2_ref2 = (short *)src2 + s_src2;

    temp_dummy = _mm_setzero_si128();
    result     = _mm_setzero_si128();

    for(i = 0; i < h >> 1; i++) {
        for(j = 0; j < w; j += 8) {
            src_8x16b   = _mm_loadu_si128((__m128i *)(&pu2_inp[j]));
            src_8x16b_1 = _mm_loadu_si128((__m128i *)(&pu2_inp2[j]));

            pred_8x16b   = _mm_loadu_si128((__m128i *)(&pu2_ref[j]));
            pred_8x16b_1 = _mm_loadu_si128((__m128i *)(&pu2_ref2[j]));

            temp   = _mm_sub_epi16(src_8x16b, pred_8x16b);
            temp_1 = _mm_sub_epi16(src_8x16b_1, pred_8x16b_1);

            temp   = _mm_abs_epi16(temp);
            temp_1 = _mm_abs_epi16(temp_1);

            temp = _mm_add_epi16(temp, temp_1);

            temp_1 = _mm_unpackhi_epi16(temp, temp_dummy);
            temp_2 = _mm_unpacklo_epi16(temp, temp_dummy);

            temp   = _mm_add_epi32(temp_1, temp_2);
            result = _mm_add_epi32(result, temp);
        }

        pu2_inp += s_src1_t2;
        pu2_ref += s_src2_t2;
        pu2_inp2 += s_src1_t2;
        pu2_ref2 += s_src2_t2;
    }

    result = _mm_hadd_epi32(result, result);
    result = _mm_hadd_epi32(result, result);
    sad    = _mm_extract_epi32(result, 0);

    return (sad >> (bit_depth - 8));
}

int sad_16b_sse_16nx1n(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    __m128i src_8x16b;
    __m128i src_8x16b_1;

    __m128i pred_8x16b;
    __m128i pred_8x16b_1;

    __m128i temp;
    __m128i temp_1;
    __m128i temp_2;

    __m128i temp_dummy;
    __m128i result;

    short *pu2_inp;
    short *pu2_ref;

    int i, j;
    int sad = 0;

    assert(bit_depth <= 14);
    assert(!(w & 15)); /*fun used only for multiple of 16, but internal assumption is only 8 */

    pu2_inp = src1;
    pu2_ref = src2;

    temp_dummy = _mm_setzero_si128();
    result     = _mm_setzero_si128();

    for(i = 0; i < h; i++) {
        for(j = 0; j < w; j += 16) {
            src_8x16b   = _mm_loadu_si128((__m128i *)(&pu2_inp[j]));
            src_8x16b_1 = _mm_loadu_si128((__m128i *)(&pu2_inp[j + 8]));

            pred_8x16b   = _mm_loadu_si128((__m128i *)(&pu2_ref[j]));
            pred_8x16b_1 = _mm_loadu_si128((__m128i *)(&pu2_ref[j + 8]));

            temp   = _mm_sub_epi16(src_8x16b, pred_8x16b);
            temp_1 = _mm_sub_epi16(src_8x16b_1, pred_8x16b_1);

            temp   = _mm_abs_epi16(temp);
            temp_1 = _mm_abs_epi16(temp_1);

            temp = _mm_add_epi16(temp, temp_1);

            temp_1 = _mm_unpackhi_epi16(temp, temp_dummy);
            temp_2 = _mm_unpacklo_epi16(temp, temp_dummy);

            temp   = _mm_add_epi32(temp_1, temp_2);
            result = _mm_add_epi32(result, temp);
        }

        pu2_inp += s_src1;
        pu2_ref += s_src2;
    }

    result = _mm_hadd_epi32(result, result);
    result = _mm_hadd_epi32(result, result);
    sad    = _mm_extract_epi32(result, 0);

    return (sad >> (bit_depth - 8));
}

// clang-format off

/* index: [log2 of width][log2 of height] */
const XEVE_FN_SAD xeve_tbl_sad_16b_sse[8][8] =
{
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
        sad_16b, /* height == 1 */
        sad_16b_sse_4x2,  /* height == 2 */
        sad_16b_sse_4x4,  /* height == 4 */
        sad_16b_sse_4x2n, /* height == 8 */
        sad_16b_sse_4x2n, /* height == 16 */
        sad_16b_sse_4x2n, /* height == 32 */
        sad_16b_sse_4x2n, /* height == 64 */
        sad_16b_sse_4x2n, /* height == 128 */
    },
    /* width == 8 */
    {
        sad_16b,          /* height == 1 */
        sad_16b_sse_8x2n, /* height == 2 */
        sad_16b_sse_8x2n, /* height == 4 */
        sad_16b_sse_8x2n, /* height == 8 */
        sad_16b_sse_8x2n, /* height == 16 */
        sad_16b_sse_8x2n, /* height == 32 */
        sad_16b_sse_8x2n, /* height == 64 */
        sad_16b_sse_8x2n, /* height == 128 */
    },
    /* width == 16 */
    {
        sad_16b_sse_16nx1n,  /* height == 1 */
        sad_16b_sse_16nx1n,  /* height == 2 */
        sad_16b_sse_16nx1n,  /* height == 4 */
        sad_16b_sse_16nx1n,  /* height == 8 */
        sad_16b_sse_16nx1n,  /* height == 16 */
        sad_16b_sse_16nx1n,  /* height == 32 */
        sad_16b_sse_16nx1n,  /* height == 64 */
        sad_16b_sse_16nx1n,  /* height == 128 */
    },
    /* width == 32 */
    {
        sad_16b_sse_16nx1n,  /* height == 1 */
        sad_16b_sse_16nx1n,  /* height == 2 */
        sad_16b_sse_16nx1n,  /* height == 4 */
        sad_16b_sse_16nx1n,  /* height == 8 */
        sad_16b_sse_16nx1n,  /* height == 16 */
        sad_16b_sse_16nx1n,  /* height == 32 */
        sad_16b_sse_16nx1n,  /* height == 64 */
        sad_16b_sse_16nx1n,  /* height == 128 */
    },
    /* width == 64 */
    {
        sad_16b_sse_16nx1n,  /* height == 1 */
        sad_16b_sse_16nx1n,  /* height == 2 */
        sad_16b_sse_16nx1n,  /* height == 4 */
        sad_16b_sse_16nx1n,  /* height == 8 */
        sad_16b_sse_16nx1n,  /* height == 16 */
        sad_16b_sse_16nx1n,  /* height == 32 */
        sad_16b_sse_16nx1n,  /* height == 64 */
        sad_16b_sse_16nx1n,  /* height == 128 */
    },
    /* width == 128 */
    {
        sad_16b_sse_16nx1n,  /* height == 1 */
        sad_16b_sse_16nx1n,  /* height == 2 */
        sad_16b_sse_16nx1n,  /* height == 4 */
        sad_16b_sse_16nx1n,  /* height == 8 */
        sad_16b_sse_16nx1n,  /* height == 16 */
        sad_16b_sse_16nx1n,  /* height == 32 */
        sad_16b_sse_16nx1n,  /* height == 64 */
        sad_16b_sse_16nx1n,  /* height == 128 */
    }
};


/* DIFF **********************************************************************/
#define SSE_DIFF_16B_4PEL(src1, src2, diff, m00, m01, m02) \
    m00 = _mm_loadl_epi64((__m128i*)(src1)); \
    m01 = _mm_loadl_epi64((__m128i*)(src2)); \
    m02 = _mm_sub_epi16(m00, m01); \
    _mm_storel_epi64((__m128i*)(diff), m02);

#define SSE_DIFF_16B_8PEL(src1, src2, diff, m00, m01, m02) \
    m00 = _mm_loadu_si128((__m128i*)(src1)); \
    m01 = _mm_loadu_si128((__m128i*)(src2)); \
    m02 = _mm_sub_epi16(m00, m01); \
    _mm_storeu_si128((__m128i*)(diff), m02);

// clang-format on

static void
diff_16b_sse_4x2(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int s_diff, s16 *diff, int bit_depth)
{
    s16    *s1;
    s16    *s2;
    __m128i m01, m02, m03, m04, m05, m06;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    SSE_DIFF_16B_4PEL(s1, s2, diff, m01, m02, m03);
    SSE_DIFF_16B_4PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
}

static void
diff_16b_sse_4x4(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int s_diff, s16 *diff, int bit_depth)
{
    s16    *s1;
    s16    *s2;
    __m128i m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    SSE_DIFF_16B_4PEL(s1, s2, diff, m01, m02, m03);
    SSE_DIFF_16B_4PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
    SSE_DIFF_16B_4PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, diff + s_diff * 2, m07, m08, m09);
    SSE_DIFF_16B_4PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, diff + s_diff * 3, m10, m11, m12);
}

static void
diff_16b_sse_8x8(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int s_diff, s16 *diff, int bit_depth)
{
    s16    *s1;
    s16    *s2;
    __m128i m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    SSE_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
    SSE_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
    SSE_DIFF_16B_8PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, diff + s_diff * 2, m07, m08, m09);
    SSE_DIFF_16B_8PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, diff + s_diff * 3, m10, m11, m12);
    SSE_DIFF_16B_8PEL(s1 + s_src1 * 4, s2 + s_src2 * 4, diff + s_diff * 4, m01, m02, m03);
    SSE_DIFF_16B_8PEL(s1 + s_src1 * 5, s2 + s_src2 * 5, diff + s_diff * 5, m04, m05, m06);
    SSE_DIFF_16B_8PEL(s1 + s_src1 * 6, s2 + s_src2 * 6, diff + s_diff * 6, m07, m08, m09);
    SSE_DIFF_16B_8PEL(s1 + s_src1 * 7, s2 + s_src2 * 7, diff + s_diff * 7, m10, m11, m12);
}

static void
diff_16b_sse_8nx2n(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int s_diff, s16 *diff, int bit_depth)
{
    s16    *s1;
    s16    *s2;
    int     i, j;
    __m128i m01, m02, m03, m04, m05, m06;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    for(i = 0; i < h >> 1; i++) {
        for(j = 0; j < (w >> 3); j++) {
            SSE_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            SSE_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            s1 += 8;
            s2 += 8;
            diff += 8;
        }

        s1 += ((s_src1 << 1) - ((w >> 3) << 3));
        s2 += ((s_src2 << 1) - ((w >> 3) << 3));
        diff += ((s_diff << 1) - ((w >> 3) << 3));
    }
}

static void
diff_16b_sse_16nx2n(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int s_diff, s16 *diff, int bit_depth)
{
    s16    *s1;
    s16    *s2;
    int     i, j;
    __m128i m01, m02, m03, m04, m05, m06;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    for(i = 0; i < h >> 1; i++) {
        for(j = 0; j < (w >> 4); j++) {
            SSE_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            SSE_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            s1 += 8;
            s2 += 8;
            diff += 8;

            SSE_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            SSE_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            s1 += 8;
            s2 += 8;
            diff += 8;
        }

        s2 += ((s_src2 << 1) - ((w >> 4) << 4));
        s1 += ((s_src1 << 1) - ((w >> 4) << 4));
        diff += ((s_diff << 1) - ((w >> 4) << 4));
    }
}

static void
diff_16b_sse_32nx4n(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int s_diff, s16 *diff, int bit_depth)
{
    s16    *s1;
    s16    *s2;
    int     i, j;
    __m128i m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    for(i = 0; i < (h >> 2); i++) {
        for(j = 0; j < (w >> 5); j++) {
            SSE_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            SSE_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            SSE_DIFF_16B_8PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, diff + s_diff * 2, m07, m08, m09);
            SSE_DIFF_16B_8PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, diff + s_diff * 3, m10, m11, m12);
            s1 += 8;
            s2 += 8;
            diff += 8;

            SSE_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            SSE_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            SSE_DIFF_16B_8PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, diff + s_diff * 2, m07, m08, m09);
            SSE_DIFF_16B_8PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, diff + s_diff * 3, m10, m11, m12);
            s1 += 8;
            s2 += 8;
            diff += 8;

            SSE_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            SSE_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            SSE_DIFF_16B_8PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, diff + s_diff * 2, m07, m08, m09);
            SSE_DIFF_16B_8PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, diff + s_diff * 3, m10, m11, m12);
            s1 += 8;
            s2 += 8;
            diff += 8;

            SSE_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            SSE_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            SSE_DIFF_16B_8PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, diff + s_diff * 2, m07, m08, m09);
            SSE_DIFF_16B_8PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, diff + s_diff * 3, m10, m11, m12);
            s1 += 8;
            s2 += 8;
            diff += 8;
        }

        s1 += ((s_src1 << 2) - ((w >> 5) << 5));
        s2 += ((s_src2 << 2) - ((w >> 5) << 5));
        diff += ((s_diff << 2) - ((w >> 5) << 5));
    }
}

// clang-format off
const XEVE_FN_DIFF xeve_tbl_diff_16b_sse[8][8] =
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
        diff_16b_sse_4x2,  /* height == 2 */
        diff_16b_sse_4x4,  /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 8 */
    {
        diff_16b,  /* height == 1 */
        diff_16b_sse_8nx2n, /* height == 2 */
        diff_16b_sse_8nx2n, /* height == 4 */
        diff_16b_sse_8x8,   /* height == 8 */
        diff_16b_sse_8nx2n, /* height == 16 */
        diff_16b_sse_8nx2n, /* height == 32 */
        diff_16b_sse_8nx2n, /* height == 64 */
        diff_16b_sse_8nx2n, /* height == 128 */
    },
    /* width == 16 */
    {
        diff_16b,   /* height == 1 */
        diff_16b_sse_16nx2n, /* height == 2 */
        diff_16b_sse_16nx2n, /* height == 4 */
        diff_16b_sse_16nx2n, /* height == 8 */
        diff_16b_sse_16nx2n, /* height == 16 */
        diff_16b_sse_16nx2n, /* height == 32 */
        diff_16b_sse_16nx2n, /* height == 64 */
        diff_16b_sse_16nx2n, /* height == 128 */
    },
    /* width == 32 */
    {
        diff_16b,   /* height == 1 */
        diff_16b_sse_16nx2n, /* height == 2 */
        diff_16b_sse_32nx4n, /* height == 4 */
        diff_16b_sse_32nx4n, /* height == 8 */
        diff_16b_sse_32nx4n, /* height == 16 */
        diff_16b_sse_32nx4n, /* height == 32 */
        diff_16b_sse_32nx4n, /* height == 64 */
        diff_16b_sse_32nx4n, /* height == 128 */
    },
    /* width == 64 */
    {
        diff_16b,   /* height == 1 */
        diff_16b_sse_16nx2n, /* height == 2 */
        diff_16b_sse_32nx4n, /* height == 4 */
        diff_16b_sse_32nx4n, /* height == 8 */
        diff_16b_sse_32nx4n, /* height == 16 */
        diff_16b_sse_32nx4n, /* height == 32 */
        diff_16b_sse_32nx4n, /* height == 64 */
        diff_16b_sse_32nx4n, /* height == 128 */
    },
    /* width == 128 */
    {
        diff_16b,   /* height == 1 */
        diff_16b_sse_16nx2n, /* height == 2 */
        diff_16b_sse_32nx4n, /* height == 4 */
        diff_16b_sse_32nx4n, /* height == 8 */
        diff_16b_sse_32nx4n, /* height == 16 */
        diff_16b_sse_32nx4n, /* height == 32 */
        diff_16b_sse_32nx4n, /* height == 64 */
        diff_16b_sse_32nx4n, /* height == 128 */
    }
};

/* SSD ***********************************************************************/
#define SSE_SSD_16B_4PEL(src1, src2, shift, s00, s01, s00a) \
    s00 = _mm_loadl_epi64((__m128i*)(src1)); \
    s01 = _mm_loadl_epi64((__m128i*)(src2));\
    s00 = _mm_sub_epi16(s00, s01); \
    s00 = _mm_cvtepi16_epi32(s00); \
    s01 = _mm_mullo_epi32(s00, s00); \
    s00 = _mm_srli_epi32(s01, shift); \
    s00a = _mm_add_epi32(s00a, s00);

#define SSE_SSD_16B_8PEL(src1, src2, shift, s00, s01, s02, s00a) \
    s00 = _mm_loadu_si128((__m128i*)(src1)); \
    s01 = _mm_loadu_si128((__m128i*)(src2)); \
    s02 = _mm_sub_epi16(s00, s01); \
    \
    s00 = _mm_cvtepi16_epi32(s02); \
    s00 = _mm_mullo_epi32(s00, s00); \
    \
    s01 = _mm_srli_si128(s02, 8); \
    s01 = _mm_cvtepi16_epi32(s01); \
    s01 = _mm_mullo_epi32(s01, s01); \
    \
    s00 = _mm_srli_epi32(s00, shift); \
    s01 = _mm_srli_epi32(s01, shift); \
    s00a = _mm_add_epi32(s00a, s00); \
    s00a = _mm_add_epi32(s00a, s01);

// clang-format on

static s64 ssd_16b_sse_4x2(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    s64       ssd;
    s16      *s1;
    s16      *s2;
    const int shift = (bit_depth - 8) << 1;
    __m128i   s00, s01, s00a;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    s00a = _mm_setzero_si128();

    SSE_SSD_16B_4PEL(s1, s2, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s00a);

    ssd = _mm_extract_epi32(s00a, 0);
    ssd += _mm_extract_epi32(s00a, 1);
    ssd += _mm_extract_epi32(s00a, 2);
    ssd += _mm_extract_epi32(s00a, 3);

    return ssd;
}

static s64 ssd_16b_sse_4x4(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    s64       ssd;
    s16      *s1;
    s16      *s2;
    const int shift = (bit_depth - 8) << 1;
    __m128i   s00, s01, s00a;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    s00a = _mm_setzero_si128();

    SSE_SSD_16B_4PEL(s1, s2, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, shift, s00, s01, s00a);

    ssd = _mm_extract_epi32(s00a, 0);
    ssd += _mm_extract_epi32(s00a, 1);
    ssd += _mm_extract_epi32(s00a, 2);
    ssd += _mm_extract_epi32(s00a, 3);

    return ssd;
}

static s64 ssd_16b_sse_4x8(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    s64       ssd;
    s16      *s1;
    s16      *s2;
    const int shift = (bit_depth - 8) << 1;
    __m128i   s00, s01, s00a;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    s00a = _mm_setzero_si128();

    SSE_SSD_16B_4PEL(s1, s2, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 4, s2 + s_src2 * 4, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 5, s2 + s_src2 * 5, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 6, s2 + s_src2 * 6, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 7, s2 + s_src2 * 7, shift, s00, s01, s00a);

    ssd = _mm_extract_epi32(s00a, 0);
    ssd += _mm_extract_epi32(s00a, 1);
    ssd += _mm_extract_epi32(s00a, 2);
    ssd += _mm_extract_epi32(s00a, 3);

    return ssd;
}

static s64 ssd_16b_sse_4x16(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    s64       ssd;
    s16      *s1;
    s16      *s2;
    const int shift = (bit_depth - 8) << 1;
    __m128i   s00, s01, s00a;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    s00a = _mm_setzero_si128();

    SSE_SSD_16B_4PEL(s1, s2, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 4, s2 + s_src2 * 4, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 5, s2 + s_src2 * 5, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 6, s2 + s_src2 * 6, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 7, s2 + s_src2 * 7, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 8, s2 + s_src2 * 8, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 9, s2 + s_src2 * 9, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 10, s2 + s_src2 * 10, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 11, s2 + s_src2 * 11, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 12, s2 + s_src2 * 12, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 13, s2 + s_src2 * 13, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 14, s2 + s_src2 * 14, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 15, s2 + s_src2 * 15, shift, s00, s01, s00a);

    ssd = _mm_extract_epi32(s00a, 0);
    ssd += _mm_extract_epi32(s00a, 1);
    ssd += _mm_extract_epi32(s00a, 2);
    ssd += _mm_extract_epi32(s00a, 3);

    return ssd;
}

static s64 ssd_16b_sse_4x32(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    s64       ssd;
    s16      *s1;
    s16      *s2;
    const int shift = (bit_depth - 8) << 1;
    __m128i   s00, s01, s00a;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    s00a = _mm_setzero_si128();

    SSE_SSD_16B_4PEL(s1, s2, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 4, s2 + s_src2 * 4, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 5, s2 + s_src2 * 5, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 6, s2 + s_src2 * 6, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 7, s2 + s_src2 * 7, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 8, s2 + s_src2 * 8, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 9, s2 + s_src2 * 9, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 10, s2 + s_src2 * 10, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 11, s2 + s_src2 * 11, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 12, s2 + s_src2 * 12, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 13, s2 + s_src2 * 13, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 14, s2 + s_src2 * 14, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 15, s2 + s_src2 * 15, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 16, s2 + s_src2 * 16, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 17, s2 + s_src2 * 17, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 18, s2 + s_src2 * 18, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 19, s2 + s_src2 * 19, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 20, s2 + s_src2 * 20, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 21, s2 + s_src2 * 21, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 22, s2 + s_src2 * 22, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 23, s2 + s_src2 * 23, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 24, s2 + s_src2 * 24, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 25, s2 + s_src2 * 25, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 26, s2 + s_src2 * 26, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 27, s2 + s_src2 * 27, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 28, s2 + s_src2 * 28, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 29, s2 + s_src2 * 29, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 30, s2 + s_src2 * 30, shift, s00, s01, s00a);
    SSE_SSD_16B_4PEL(s1 + s_src1 * 31, s2 + s_src2 * 31, shift, s00, s01, s00a);

    ssd = _mm_extract_epi32(s00a, 0);
    ssd += _mm_extract_epi32(s00a, 1);
    ssd += _mm_extract_epi32(s00a, 2);
    ssd += _mm_extract_epi32(s00a, 3);

    return ssd;
}

static s64 ssd_16b_sse_8x2(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    s64       ssd;
    s16      *s1;
    s16      *s2;
    const int shift = (bit_depth - 8) << 1;
    __m128i   s00, s01, s02, s00a;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    s00a = _mm_setzero_si128();

    SSE_SSD_16B_8PEL(s1, s2, shift, s00, s01, s02, s00a);
    SSE_SSD_16B_8PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s02, s00a);

    ssd = _mm_extract_epi32(s00a, 0);
    ssd += _mm_extract_epi32(s00a, 1);
    ssd += _mm_extract_epi32(s00a, 2);
    ssd += _mm_extract_epi32(s00a, 3);

    return ssd;
}

static s64 ssd_16b_sse_8x4(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    s64       ssd;
    s16      *s1;
    s16      *s2;
    const int shift = (bit_depth - 8) << 1;
    __m128i   s00, s01, s02, s00a;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    s00a = _mm_setzero_si128();

    SSE_SSD_16B_8PEL(s1, s2, shift, s00, s01, s02, s00a);
    SSE_SSD_16B_8PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s02, s00a);
    SSE_SSD_16B_8PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, shift, s00, s01, s02, s00a);
    SSE_SSD_16B_8PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, shift, s00, s01, s02, s00a);

    ssd = _mm_extract_epi32(s00a, 0);
    ssd += _mm_extract_epi32(s00a, 1);
    ssd += _mm_extract_epi32(s00a, 2);
    ssd += _mm_extract_epi32(s00a, 3);

    return ssd;
}

static s64 ssd_16b_sse_8x8(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    s64       ssd;
    s16      *s1;
    s16      *s2;
    const int shift = (bit_depth - 8) << 1;
    __m128i   s00, s01, s02, s00a;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    s00a = _mm_setzero_si128();

    SSE_SSD_16B_8PEL(s1, s2, shift, s00, s01, s02, s00a);
    SSE_SSD_16B_8PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s02, s00a);
    SSE_SSD_16B_8PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, shift, s00, s01, s02, s00a);
    SSE_SSD_16B_8PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, shift, s00, s01, s02, s00a);
    SSE_SSD_16B_8PEL(s1 + s_src1 * 4, s2 + s_src2 * 4, shift, s00, s01, s02, s00a);
    SSE_SSD_16B_8PEL(s1 + s_src1 * 5, s2 + s_src2 * 5, shift, s00, s01, s02, s00a);
    SSE_SSD_16B_8PEL(s1 + s_src1 * 6, s2 + s_src2 * 6, shift, s00, s01, s02, s00a);
    SSE_SSD_16B_8PEL(s1 + s_src1 * 7, s2 + s_src2 * 7, shift, s00, s01, s02, s00a);

    ssd = _mm_extract_epi32(s00a, 0);
    ssd += _mm_extract_epi32(s00a, 1);
    ssd += _mm_extract_epi32(s00a, 2);
    ssd += _mm_extract_epi32(s00a, 3);

    return ssd;
}

static s64 ssd_16b_sse_8nx2n(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    s64       ssd;
    s16      *s1;
    s16      *s2;
    int       i, j;
    const int shift = (bit_depth - 8) << 1;
    __m128i   s00, s01, s02, s00a;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    s00a = _mm_setzero_si128();

    for(i = 0; i < (h >> 1); i++) {
        for(j = 0; j < (w >> 3); j++) {
            SSE_SSD_16B_8PEL(s1, s2, shift, s00, s01, s02, s00a);
            SSE_SSD_16B_8PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s02, s00a);

            s1 += 8;
            s2 += 8;
        }
        s1 += (s_src1 << 1) - ((w >> 3) << 8);
        s2 += (s_src2 << 1) - ((w >> 3) << 8);
    }
    ssd = _mm_extract_epi32(s00a, 0);
    ssd += _mm_extract_epi32(s00a, 1);
    ssd += _mm_extract_epi32(s00a, 2);
    ssd += _mm_extract_epi32(s00a, 3);

    return ssd;
}

static s64 ssd_16b_sse_8nx4n(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    s64       ssd;
    s16      *s1;
    s16      *s2;
    int       i, j;
    const int shift = (bit_depth - 8) << 1;
    __m128i   s00, s01, s02, s00a;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    s00a = _mm_setzero_si128();

    for(i = 0; i < (h >> 2); i++) {
        for(j = 0; j < (w >> 3); j++) {
            SSE_SSD_16B_8PEL(s1, s2, shift, s00, s01, s02, s00a);
            SSE_SSD_16B_8PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s02, s00a);
            SSE_SSD_16B_8PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, shift, s00, s01, s02, s00a);
            SSE_SSD_16B_8PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, shift, s00, s01, s02, s00a);

            s1 += 8;
            s2 += 8;
        }
        s1 += (s_src1 << 2) - ((w >> 3) << 3);
        s2 += (s_src2 << 2) - ((w >> 3) << 3);
    }
    ssd = _mm_extract_epi32(s00a, 0);
    ssd += _mm_extract_epi32(s00a, 1);
    ssd += _mm_extract_epi32(s00a, 2);
    ssd += _mm_extract_epi32(s00a, 3);

    return ssd;
}

static s64 ssd_16b_sse_8nx8n(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    s64       ssd;
    s16      *s1;
    s16      *s2;
    int       i, j;
    const int shift = (bit_depth - 8) << 1;
    __m128i   s00, s01, s02, s00a;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    s00a = _mm_setzero_si128();

    for(i = 0; i < (h >> 3); i++) {
        for(j = 0; j < (w >> 3); j++) {
            SSE_SSD_16B_8PEL(s1, s2, shift, s00, s01, s02, s00a);
            SSE_SSD_16B_8PEL(s1 + s_src1, s2 + s_src2, shift, s00, s01, s02, s00a);
            SSE_SSD_16B_8PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, shift, s00, s01, s02, s00a);
            SSE_SSD_16B_8PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, shift, s00, s01, s02, s00a);
            SSE_SSD_16B_8PEL(s1 + s_src1 * 4, s2 + s_src2 * 4, shift, s00, s01, s02, s00a);
            SSE_SSD_16B_8PEL(s1 + s_src1 * 5, s2 + s_src2 * 5, shift, s00, s01, s02, s00a);
            SSE_SSD_16B_8PEL(s1 + s_src1 * 6, s2 + s_src2 * 6, shift, s00, s01, s02, s00a);
            SSE_SSD_16B_8PEL(s1 + s_src1 * 7, s2 + s_src2 * 7, shift, s00, s01, s02, s00a);
            s1 += 8;
            s2 += 8;
        }
        s1 += (s_src1 << 3) - ((w >> 3) << 3);
        s2 += (s_src2 << 3) - ((w >> 3) << 3);
    }
    ssd = _mm_extract_epi32(s00a, 0);
    ssd += _mm_extract_epi32(s00a, 1);
    ssd += _mm_extract_epi32(s00a, 2);
    ssd += _mm_extract_epi32(s00a, 3);

    return ssd;
}

// clang-format off
const XEVE_FN_SSD xeve_tbl_ssd_16b_sse[8][8] =
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
        ssd_16b_sse_4x2,  /* height == 2 */
        ssd_16b_sse_4x4,  /* height == 4 */
        ssd_16b_sse_4x8,  /* height == 8 */
        ssd_16b_sse_4x16, /* height == 16 */
        ssd_16b_sse_4x32, /* height == 32 */
        ssd_16b, /* height == 64 */
        ssd_16b, /* height == 128 */
    },
    /* width == 8 */
    {
        ssd_16b,  /* height == 1 */
        ssd_16b_sse_8x2,   /* height == 2 */
        ssd_16b_sse_8x4,   /* height == 4 */
        ssd_16b_sse_8x8,   /* height == 8 */
        ssd_16b_sse_8nx8n, /* height == 16 */
        ssd_16b_sse_8nx8n, /* height == 32 */
        ssd_16b_sse_8nx8n, /* height == 64 */
        ssd_16b_sse_8nx8n, /* height == 128 */
    },
    /* width == 16 */
    {
        ssd_16b,  /* height == 1 */
        ssd_16b_sse_8nx2n, /* height == 2 */
        ssd_16b_sse_8nx4n, /* height == 4 */
        ssd_16b_sse_8nx8n, /* height == 8 */
        ssd_16b_sse_8nx8n, /* height == 16 */
        ssd_16b_sse_8nx8n, /* height == 32 */
        ssd_16b_sse_8nx8n, /* height == 64 */
        ssd_16b_sse_8nx8n, /* height == 128 */
    },
    /* width == 32 */
    {
        ssd_16b,  /* height == 1 */
        ssd_16b_sse_8nx2n, /* height == 2 */
        ssd_16b_sse_8nx4n, /* height == 4 */
        ssd_16b_sse_8nx8n, /* height == 8 */
        ssd_16b_sse_8nx8n, /* height == 16 */
        ssd_16b_sse_8nx8n, /* height == 32 */
        ssd_16b_sse_8nx8n, /* height == 64 */
        ssd_16b_sse_8nx8n, /* height == 128 */
    },
    /* width == 64 */
    {
        ssd_16b,  /* height == 1 */
        ssd_16b,  /* height == 2 */
        ssd_16b_sse_8nx4n, /* height == 4 */
        ssd_16b_sse_8nx8n, /* height == 8 */
        ssd_16b_sse_8nx8n, /* height == 16 */
        ssd_16b_sse_8nx8n, /* height == 32 */
        ssd_16b_sse_8nx8n, /* height == 64 */
        ssd_16b_sse_8nx8n, /* height == 128 */
    },
    /* width == 128 */
    {
        ssd_16b,  /* height == 1 */
        ssd_16b_sse_8nx2n, /* height == 2 */
        ssd_16b_sse_8nx4n, /* height == 4 */
        ssd_16b_sse_8nx8n, /* height == 8 */
        ssd_16b_sse_8nx8n, /* height == 16 */
        ssd_16b_sse_8nx8n, /* height == 32 */
        ssd_16b_sse_8nx8n, /* height == 64 */
        ssd_16b_sse_8nx8n, /* height == 128 */
    }
};
// clang-format on

/* SATD **********************************************************************/
int xeve_had_4x4_sse(pel *org, pel *cur, int s_org, int s_cur, int step, int bit_depth)
{
    xeve_assert(bit_depth == 10);
    int     satd = 0;
    __m128i r0   = (_mm_loadl_epi64((const __m128i *)&org[0]));
    __m128i r1   = (_mm_loadl_epi64((const __m128i *)&org[s_org]));
    __m128i r2   = (_mm_loadl_epi64((const __m128i *)&org[2 * s_org]));
    __m128i r3   = (_mm_loadl_epi64((const __m128i *)&org[3 * s_org]));
    __m128i r4   = (_mm_loadl_epi64((const __m128i *)&cur[0]));
    __m128i r5   = (_mm_loadl_epi64((const __m128i *)&cur[s_cur]));
    __m128i r6   = (_mm_loadl_epi64((const __m128i *)&cur[2 * s_cur]));
    __m128i r7   = (_mm_loadl_epi64((const __m128i *)&cur[3 * s_cur]));
    __m128i sum;
    __m128i zero;

    r0 = _mm_sub_epi16(r0, r4);
    r1 = _mm_sub_epi16(r1, r5);
    r2 = _mm_sub_epi16(r2, r6);
    r3 = _mm_sub_epi16(r3, r7);

    // first stage
    r4 = r0;
    r5 = r1;

    r0 = _mm_add_epi16(r0, r3);
    r1 = _mm_add_epi16(r1, r2);

    r4 = _mm_sub_epi16(r4, r3);
    r5 = _mm_sub_epi16(r5, r2);

    r2 = r0;
    r3 = r4;

    r0 = _mm_add_epi16(r0, r1);
    r2 = _mm_sub_epi16(r2, r1);
    r3 = _mm_sub_epi16(r3, r5);
    r5 = _mm_add_epi16(r5, r4);

    // shuffle - flip matrix for vertical transform
    r0 = _mm_unpacklo_epi16(r0, r5);
    r2 = _mm_unpacklo_epi16(r2, r3);

    r3 = r0;
    r0 = _mm_unpacklo_epi32(r0, r2);
    r3 = _mm_unpackhi_epi32(r3, r2);

    r1 = r0;
    r2 = r3;
    r1 = _mm_srli_si128(r1, 8);
    r3 = _mm_srli_si128(r3, 8);

    // second stage
    r4 = r0;
    r5 = r1;

    r0 = _mm_add_epi16(r0, r3);
    r1 = _mm_add_epi16(r1, r2);

    r4 = _mm_sub_epi16(r4, r3);
    r5 = _mm_sub_epi16(r5, r2);

    r2 = r0;
    r3 = r4;

    r0 = _mm_add_epi16(r0, r1);
    r2 = _mm_sub_epi16(r2, r1);
    r3 = _mm_sub_epi16(r3, r5);
    r5 = _mm_add_epi16(r5, r4);

    // abs
    sum = _mm_abs_epi16(r0);

    s16 *p = (s16 *)&sum;
    p[0]   = p[0] >> 2;

    sum = _mm_add_epi16(sum, _mm_abs_epi16(r2));
    sum = _mm_add_epi16(sum, _mm_abs_epi16(r3));
    sum = _mm_add_epi16(sum, _mm_abs_epi16(r5));

    zero = _mm_set1_epi16(0);
    sum  = _mm_unpacklo_epi16(sum, zero);
    sum  = _mm_hadd_epi32(sum, sum);
    sum  = _mm_hadd_epi32(sum, sum);

    satd = _mm_cvtsi128_si32(sum);

    satd = ((satd + 1) >> 1);

    return satd;
}

int xeve_had_8x8_sse(pel *org, pel *cur, int s_org, int s_cur, int step, int bit_depth)
{
    xeve_assert(bit_depth == 10);
    int     sad = 0;
    /* all 128 bit registers are named with a suffix mxnb, where m is the */
    /* number of n bits packed in the register                            */
    __m128i src0_8x16b, src1_8x16b, src2_8x16b, src3_8x16b;
    __m128i src4_8x16b, src5_8x16b, src6_8x16b, src7_8x16b;
    __m128i pred0_8x16b, pred1_8x16b, pred2_8x16b, pred3_8x16b;
    __m128i pred4_8x16b, pred5_8x16b, pred6_8x16b, pred7_8x16b;
    __m128i out0_8x16b, out1_8x16b, out2_8x16b, out3_8x16b;
    __m128i out4_8x16b, out5_8x16b, out6_8x16b, out7_8x16b;

    /**********************Residue Calculation********************************/
    src0_8x16b = _mm_loadu_si128((__m128i *)org);
    org        = org + s_org;
    src1_8x16b = _mm_loadu_si128((__m128i *)org);
    org        = org + s_org;
    src2_8x16b = _mm_loadu_si128((__m128i *)org);
    org        = org + s_org;
    src3_8x16b = _mm_loadu_si128((__m128i *)org);
    org        = org + s_org;
    src4_8x16b = _mm_loadu_si128((__m128i *)org);
    org        = org + s_org;
    src5_8x16b = _mm_loadu_si128((__m128i *)org);
    org        = org + s_org;
    src6_8x16b = _mm_loadu_si128((__m128i *)org);
    org        = org + s_org;
    src7_8x16b = _mm_loadu_si128((__m128i *)org);
    org        = org + s_org;

    pred0_8x16b = _mm_loadu_si128((__m128i *)cur);
    cur         = cur + s_cur;
    pred1_8x16b = _mm_loadu_si128((__m128i *)cur);
    cur         = cur + s_cur;
    pred2_8x16b = _mm_loadu_si128((__m128i *)cur);
    cur         = cur + s_cur;
    pred3_8x16b = _mm_loadu_si128((__m128i *)cur);
    cur         = cur + s_cur;
    pred4_8x16b = _mm_loadu_si128((__m128i *)cur);
    cur         = cur + s_cur;
    pred5_8x16b = _mm_loadu_si128((__m128i *)cur);
    cur         = cur + s_cur;
    pred6_8x16b = _mm_loadu_si128((__m128i *)cur);
    cur         = cur + s_cur;
    pred7_8x16b = _mm_loadu_si128((__m128i *)cur);
    cur         = cur + s_cur;

    src0_8x16b = _mm_sub_epi16(src0_8x16b, pred0_8x16b);
    src1_8x16b = _mm_sub_epi16(src1_8x16b, pred1_8x16b);
    src2_8x16b = _mm_sub_epi16(src2_8x16b, pred2_8x16b);
    src3_8x16b = _mm_sub_epi16(src3_8x16b, pred3_8x16b);
    src4_8x16b = _mm_sub_epi16(src4_8x16b, pred4_8x16b);
    src5_8x16b = _mm_sub_epi16(src5_8x16b, pred5_8x16b);
    src6_8x16b = _mm_sub_epi16(src6_8x16b, pred6_8x16b);
    src7_8x16b = _mm_sub_epi16(src7_8x16b, pred7_8x16b);
    /**********************Residue Calculation********************************/

    /**************** 8x8 horizontal transform *******************************/
    /***********************    8x8 16 bit Transpose  ************************/
    out3_8x16b  = _mm_unpacklo_epi16(src0_8x16b, src1_8x16b);
    pred0_8x16b = _mm_unpacklo_epi16(src2_8x16b, src3_8x16b);
    out2_8x16b  = _mm_unpacklo_epi16(src4_8x16b, src5_8x16b);
    pred3_8x16b = _mm_unpacklo_epi16(src6_8x16b, src7_8x16b);
    out7_8x16b  = _mm_unpackhi_epi16(src0_8x16b, src1_8x16b);
    src2_8x16b  = _mm_unpackhi_epi16(src2_8x16b, src3_8x16b);
    pred7_8x16b = _mm_unpackhi_epi16(src4_8x16b, src5_8x16b);
    src6_8x16b  = _mm_unpackhi_epi16(src6_8x16b, src7_8x16b);

    out1_8x16b  = _mm_unpacklo_epi32(out3_8x16b, pred0_8x16b);
    out3_8x16b  = _mm_unpackhi_epi32(out3_8x16b, pred0_8x16b);
    pred1_8x16b = _mm_unpacklo_epi32(out2_8x16b, pred3_8x16b);
    pred3_8x16b = _mm_unpackhi_epi32(out2_8x16b, pred3_8x16b);
    out5_8x16b  = _mm_unpacklo_epi32(out7_8x16b, src2_8x16b);
    out7_8x16b  = _mm_unpackhi_epi32(out7_8x16b, src2_8x16b);
    pred5_8x16b = _mm_unpacklo_epi32(pred7_8x16b, src6_8x16b);
    pred7_8x16b = _mm_unpackhi_epi32(pred7_8x16b, src6_8x16b);

    out0_8x16b = _mm_unpacklo_epi64(out1_8x16b, pred1_8x16b);
    out1_8x16b = _mm_unpackhi_epi64(out1_8x16b, pred1_8x16b);
    out2_8x16b = _mm_unpacklo_epi64(out3_8x16b, pred3_8x16b);
    out3_8x16b = _mm_unpackhi_epi64(out3_8x16b, pred3_8x16b);
    out4_8x16b = _mm_unpacklo_epi64(out5_8x16b, pred5_8x16b);
    out5_8x16b = _mm_unpackhi_epi64(out5_8x16b, pred5_8x16b);
    out6_8x16b = _mm_unpacklo_epi64(out7_8x16b, pred7_8x16b);
    out7_8x16b = _mm_unpackhi_epi64(out7_8x16b, pred7_8x16b);
    /**********************   8x8 16 bit Transpose End   *********************/

    /* r0 + r1 */
    pred0_8x16b = _mm_add_epi16(out0_8x16b, out1_8x16b);
    /* r2 + r3 */
    pred2_8x16b = _mm_add_epi16(out2_8x16b, out3_8x16b);
    /* r4 + r5 */
    pred4_8x16b = _mm_add_epi16(out4_8x16b, out5_8x16b);
    /* r6 + r7 */
    pred6_8x16b = _mm_add_epi16(out6_8x16b, out7_8x16b);

    /* r0 + r1 + r2 + r3 */
    pred1_8x16b = _mm_add_epi16(pred0_8x16b, pred2_8x16b);
    /* r4 + r5 + r6 + r7 */
    pred5_8x16b = _mm_add_epi16(pred4_8x16b, pred6_8x16b);
    /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
    src0_8x16b  = _mm_add_epi16(pred1_8x16b, pred5_8x16b);
    /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
    src4_8x16b  = _mm_sub_epi16(pred1_8x16b, pred5_8x16b);

    /* r0 + r1 - r2 - r3 */
    pred1_8x16b = _mm_sub_epi16(pred0_8x16b, pred2_8x16b);
    /* r4 + r5 - r6 - r7 */
    pred5_8x16b = _mm_sub_epi16(pred4_8x16b, pred6_8x16b);
    /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
    src2_8x16b  = _mm_add_epi16(pred1_8x16b, pred5_8x16b);
    /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
    src6_8x16b  = _mm_sub_epi16(pred1_8x16b, pred5_8x16b);

    /* r0 - r1 */
    pred0_8x16b = _mm_sub_epi16(out0_8x16b, out1_8x16b);
    /* r2 - r3 */
    pred2_8x16b = _mm_sub_epi16(out2_8x16b, out3_8x16b);
    /* r4 - r5 */
    pred4_8x16b = _mm_sub_epi16(out4_8x16b, out5_8x16b);
    /* r6 - r7 */
    pred6_8x16b = _mm_sub_epi16(out6_8x16b, out7_8x16b);

    /* r0 - r1 + r2 - r3 */
    pred1_8x16b = _mm_add_epi16(pred0_8x16b, pred2_8x16b);
    /* r4 - r5 + r6 - r7 */
    pred5_8x16b = _mm_add_epi16(pred4_8x16b, pred6_8x16b);
    /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
    src1_8x16b  = _mm_add_epi16(pred1_8x16b, pred5_8x16b);
    /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
    src5_8x16b  = _mm_sub_epi16(pred1_8x16b, pred5_8x16b);

    /* r0 - r1 - r2 + r3 */
    pred1_8x16b = _mm_sub_epi16(pred0_8x16b, pred2_8x16b);
    /* r4 - r5 - r6 + r7 */
    pred5_8x16b = _mm_sub_epi16(pred4_8x16b, pred6_8x16b);
    /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
    src3_8x16b  = _mm_add_epi16(pred1_8x16b, pred5_8x16b);
    /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
    src7_8x16b  = _mm_sub_epi16(pred1_8x16b, pred5_8x16b);

    /***********************    8x8 16 bit Transpose  ************************/
    out3_8x16b  = _mm_unpacklo_epi16(src0_8x16b, src1_8x16b);
    pred0_8x16b = _mm_unpacklo_epi16(src2_8x16b, src3_8x16b);
    out2_8x16b  = _mm_unpacklo_epi16(src4_8x16b, src5_8x16b);
    pred3_8x16b = _mm_unpacklo_epi16(src6_8x16b, src7_8x16b);
    out7_8x16b  = _mm_unpackhi_epi16(src0_8x16b, src1_8x16b);
    src2_8x16b  = _mm_unpackhi_epi16(src2_8x16b, src3_8x16b);
    pred7_8x16b = _mm_unpackhi_epi16(src4_8x16b, src5_8x16b);
    src6_8x16b  = _mm_unpackhi_epi16(src6_8x16b, src7_8x16b);

    out1_8x16b  = _mm_unpacklo_epi32(out3_8x16b, pred0_8x16b);
    out3_8x16b  = _mm_unpackhi_epi32(out3_8x16b, pred0_8x16b);
    pred1_8x16b = _mm_unpacklo_epi32(out2_8x16b, pred3_8x16b);
    pred3_8x16b = _mm_unpackhi_epi32(out2_8x16b, pred3_8x16b);
    out5_8x16b  = _mm_unpacklo_epi32(out7_8x16b, src2_8x16b);
    out7_8x16b  = _mm_unpackhi_epi32(out7_8x16b, src2_8x16b);
    pred5_8x16b = _mm_unpacklo_epi32(pred7_8x16b, src6_8x16b);
    pred7_8x16b = _mm_unpackhi_epi32(pred7_8x16b, src6_8x16b);

    src0_8x16b = _mm_unpacklo_epi64(out1_8x16b, pred1_8x16b);
    src1_8x16b = _mm_unpackhi_epi64(out1_8x16b, pred1_8x16b);
    src2_8x16b = _mm_unpacklo_epi64(out3_8x16b, pred3_8x16b);
    src3_8x16b = _mm_unpackhi_epi64(out3_8x16b, pred3_8x16b);
    src4_8x16b = _mm_unpacklo_epi64(out5_8x16b, pred5_8x16b);
    src5_8x16b = _mm_unpackhi_epi64(out5_8x16b, pred5_8x16b);
    src6_8x16b = _mm_unpacklo_epi64(out7_8x16b, pred7_8x16b);
    src7_8x16b = _mm_unpackhi_epi64(out7_8x16b, pred7_8x16b);
    /**********************   8x8 16 bit Transpose End   *********************/
    /**************** 8x8 horizontal transform *******************************/

    {
        __m128i out0a_8x16b, out1a_8x16b, out2a_8x16b, out3a_8x16b;
        __m128i out4a_8x16b, out5a_8x16b, out6a_8x16b, out7a_8x16b;
        __m128i tmp0_8x16b, tmp1_8x16b, tmp2_8x16b, tmp3_8x16b;
        __m128i tmp4_8x16b, tmp5_8x16b, tmp6_8x16b, tmp7_8x16b;

        /************************* 8x8 Vertical Transform*************************/
        tmp0_8x16b = _mm_srli_si128(src0_8x16b, 8);
        tmp1_8x16b = _mm_srli_si128(src1_8x16b, 8);
        tmp2_8x16b = _mm_srli_si128(src2_8x16b, 8);
        tmp3_8x16b = _mm_srli_si128(src3_8x16b, 8);
        tmp4_8x16b = _mm_srli_si128(src4_8x16b, 8);
        tmp5_8x16b = _mm_srli_si128(src5_8x16b, 8);
        tmp6_8x16b = _mm_srli_si128(src6_8x16b, 8);
        tmp7_8x16b = _mm_srli_si128(src7_8x16b, 8);

        /*************************First 4 pixels ********************************/
        src0_8x16b = _mm_cvtepi16_epi32(src0_8x16b);
        src1_8x16b = _mm_cvtepi16_epi32(src1_8x16b);
        src2_8x16b = _mm_cvtepi16_epi32(src2_8x16b);
        src3_8x16b = _mm_cvtepi16_epi32(src3_8x16b);
        src4_8x16b = _mm_cvtepi16_epi32(src4_8x16b);
        src5_8x16b = _mm_cvtepi16_epi32(src5_8x16b);
        src6_8x16b = _mm_cvtepi16_epi32(src6_8x16b);
        src7_8x16b = _mm_cvtepi16_epi32(src7_8x16b);

        /* r0 + r1 */
        pred0_8x16b = _mm_add_epi32(src0_8x16b, src1_8x16b);
        /* r2 + r3 */
        pred2_8x16b = _mm_add_epi32(src2_8x16b, src3_8x16b);
        /* r4 + r5 */
        pred4_8x16b = _mm_add_epi32(src4_8x16b, src5_8x16b);
        /* r6 + r7 */
        pred6_8x16b = _mm_add_epi32(src6_8x16b, src7_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = _mm_add_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = _mm_add_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        out0_8x16b  = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        out4_8x16b  = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b = _mm_sub_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b = _mm_sub_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        out2_8x16b  = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        out6_8x16b  = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = _mm_sub_epi32(src0_8x16b, src1_8x16b);
        /* r2 - r3 */
        pred2_8x16b = _mm_sub_epi32(src2_8x16b, src3_8x16b);
        /* r4 - r5 */
        pred4_8x16b = _mm_sub_epi32(src4_8x16b, src5_8x16b);
        /* r6 - r7 */
        pred6_8x16b = _mm_sub_epi32(src6_8x16b, src7_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b = _mm_add_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b = _mm_add_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        out1_8x16b  = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        out5_8x16b  = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b = _mm_sub_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b = _mm_sub_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        out3_8x16b  = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        out7_8x16b  = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);
        /*************************First 4 pixels ********************************/

        /**************************Next 4 pixels *******************************/
        src0_8x16b = _mm_cvtepi16_epi32(tmp0_8x16b);
        src1_8x16b = _mm_cvtepi16_epi32(tmp1_8x16b);
        src2_8x16b = _mm_cvtepi16_epi32(tmp2_8x16b);
        src3_8x16b = _mm_cvtepi16_epi32(tmp3_8x16b);
        src4_8x16b = _mm_cvtepi16_epi32(tmp4_8x16b);
        src5_8x16b = _mm_cvtepi16_epi32(tmp5_8x16b);
        src6_8x16b = _mm_cvtepi16_epi32(tmp6_8x16b);
        src7_8x16b = _mm_cvtepi16_epi32(tmp7_8x16b);

        /* r0 + r1 */
        pred0_8x16b = _mm_add_epi32(src0_8x16b, src1_8x16b);
        /* r2 + r3 */
        pred2_8x16b = _mm_add_epi32(src2_8x16b, src3_8x16b);
        /* r4 + r5 */
        pred4_8x16b = _mm_add_epi32(src4_8x16b, src5_8x16b);
        /* r6 + r7 */
        pred6_8x16b = _mm_add_epi32(src6_8x16b, src7_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = _mm_add_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = _mm_add_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        out0a_8x16b = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        out4a_8x16b = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b = _mm_sub_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b = _mm_sub_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        out2a_8x16b = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        out6a_8x16b = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = _mm_sub_epi32(src0_8x16b, src1_8x16b);
        /* r2 - r3 */
        pred2_8x16b = _mm_sub_epi32(src2_8x16b, src3_8x16b);
        /* r4 - r5 */
        pred4_8x16b = _mm_sub_epi32(src4_8x16b, src5_8x16b);
        /* r6 - r7 */
        pred6_8x16b = _mm_sub_epi32(src6_8x16b, src7_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b = _mm_add_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b = _mm_add_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        out1a_8x16b = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        out5a_8x16b = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b = _mm_sub_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b = _mm_sub_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        out3a_8x16b = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        out7a_8x16b = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);
        /**************************Next 4 pixels *******************************/
        /************************* 8x8 Vertical Transform*************************/

        /****************************SATD calculation ****************************/
        src0_8x16b = _mm_abs_epi32(out0_8x16b);
        src1_8x16b = _mm_abs_epi32(out1_8x16b);
        src2_8x16b = _mm_abs_epi32(out2_8x16b);
        src3_8x16b = _mm_abs_epi32(out3_8x16b);
        src4_8x16b = _mm_abs_epi32(out4_8x16b);
        src5_8x16b = _mm_abs_epi32(out5_8x16b);
        src6_8x16b = _mm_abs_epi32(out6_8x16b);
        src7_8x16b = _mm_abs_epi32(out7_8x16b);

        s32 *p = (s32 *)&src0_8x16b;
        p[0]   = p[0] >> 2;

        src0_8x16b = _mm_add_epi32(src0_8x16b, src1_8x16b);
        src2_8x16b = _mm_add_epi32(src2_8x16b, src3_8x16b);
        src4_8x16b = _mm_add_epi32(src4_8x16b, src5_8x16b);
        src6_8x16b = _mm_add_epi32(src6_8x16b, src7_8x16b);

        src0_8x16b = _mm_add_epi32(src0_8x16b, src2_8x16b);
        src4_8x16b = _mm_add_epi32(src4_8x16b, src6_8x16b);

        src0_8x16b = _mm_add_epi32(src0_8x16b, src4_8x16b);

        src0_8x16b = _mm_hadd_epi32(src0_8x16b, src0_8x16b);
        src0_8x16b = _mm_hadd_epi32(src0_8x16b, src0_8x16b);

        sad += _mm_cvtsi128_si32(src0_8x16b);

        src0_8x16b = _mm_abs_epi32(out0a_8x16b);
        src1_8x16b = _mm_abs_epi32(out1a_8x16b);
        src2_8x16b = _mm_abs_epi32(out2a_8x16b);
        src3_8x16b = _mm_abs_epi32(out3a_8x16b);
        src4_8x16b = _mm_abs_epi32(out4a_8x16b);
        src5_8x16b = _mm_abs_epi32(out5a_8x16b);
        src6_8x16b = _mm_abs_epi32(out6a_8x16b);
        src7_8x16b = _mm_abs_epi32(out7a_8x16b);

        src0_8x16b = _mm_add_epi32(src0_8x16b, src1_8x16b);
        src2_8x16b = _mm_add_epi32(src2_8x16b, src3_8x16b);
        src4_8x16b = _mm_add_epi32(src4_8x16b, src5_8x16b);
        src6_8x16b = _mm_add_epi32(src6_8x16b, src7_8x16b);

        src0_8x16b = _mm_add_epi32(src0_8x16b, src2_8x16b);
        src4_8x16b = _mm_add_epi32(src4_8x16b, src6_8x16b);

        src0_8x16b = _mm_add_epi32(src0_8x16b, src4_8x16b);

        src0_8x16b = _mm_hadd_epi32(src0_8x16b, src0_8x16b);
        src0_8x16b = _mm_hadd_epi32(src0_8x16b, src0_8x16b);

        sad += _mm_cvtsi128_si32(src0_8x16b);

        sad = (sad + 2) >> 2;

        return sad;
    }
}

int xeve_had_16x8_8x16_util(pel *org, pel *cur, int s_org, int s_cur, int step, int bit_depth, int mode)
{
    int sad = 0;

    /* all 128 bit registers are named with a suffix mxnb, where m is the */
    /* number of n bits packed in the register                            */
    __m128i src0_8x16b, src1_8x16b, src2_8x16b, src3_8x16b;
    __m128i src4_8x16b, src5_8x16b, src6_8x16b, src7_8x16b;
    __m128i src8_8x16b, src9_8x16b, src10_8x16b, src11_8x16b;
    __m128i src12_8x16b, src13_8x16b, src14_8x16b, src15_8x16b;
    __m128i pred0_8x16b, pred1_8x16b, pred2_8x16b, pred3_8x16b;
    __m128i pred4_8x16b, pred5_8x16b, pred6_8x16b, pred7_8x16b;
    __m128i pred8_8x16b, pred9_8x16b, pred10_8x16b, pred11_8x16b;
    __m128i pred12_8x16b, pred13_8x16b, pred14_8x16b, pred15_8x16b;
    __m128i out0_8x16b, out1_8x16b, out2_8x16b, out3_8x16b;
    __m128i out4_8x16b, out5_8x16b, out6_8x16b, out7_8x16b;
    __m128i out8_8x16b, out9_8x16b, out10_8x16b, out11_8x16b;
    __m128i out12_8x16b, out13_8x16b, out14_8x16b, out15_8x16b;

    /**********************Residue Calculation********************************/

    if(mode == 0) {
        src0_8x16b = _mm_loadu_si128((__m128i *)org);
        src1_8x16b = _mm_loadu_si128((__m128i *)(org + 8));
        org        = org + s_org;
        src2_8x16b = _mm_loadu_si128((__m128i *)org);
        src3_8x16b = _mm_loadu_si128((__m128i *)(org + 8));
        org        = org + s_org;
        src4_8x16b = _mm_loadu_si128((__m128i *)org);
        src5_8x16b = _mm_loadu_si128((__m128i *)(org + 8));
        org        = org + s_org;
        src6_8x16b = _mm_loadu_si128((__m128i *)org);
        src7_8x16b = _mm_loadu_si128((__m128i *)(org + 8));
        org        = org + s_org;

        pred0_8x16b = _mm_loadu_si128((__m128i *)cur);
        pred1_8x16b = _mm_loadu_si128((__m128i *)(cur + 8));
        cur         = cur + s_cur;
        pred2_8x16b = _mm_loadu_si128((__m128i *)cur);
        pred3_8x16b = _mm_loadu_si128((__m128i *)(cur + 8));
        cur         = cur + s_cur;
        pred4_8x16b = _mm_loadu_si128((__m128i *)cur);
        pred5_8x16b = _mm_loadu_si128((__m128i *)(cur + 8));
        cur         = cur + s_cur;
        pred6_8x16b = _mm_loadu_si128((__m128i *)cur);
        pred7_8x16b = _mm_loadu_si128((__m128i *)(cur + 8));
        cur         = cur + s_cur;

        src0_8x16b = _mm_sub_epi16(src0_8x16b, pred0_8x16b);
        src1_8x16b = _mm_sub_epi16(src1_8x16b, pred1_8x16b);
        src2_8x16b = _mm_sub_epi16(src2_8x16b, pred2_8x16b);
        src3_8x16b = _mm_sub_epi16(src3_8x16b, pred3_8x16b);
        src4_8x16b = _mm_sub_epi16(src4_8x16b, pred4_8x16b);
        src5_8x16b = _mm_sub_epi16(src5_8x16b, pred5_8x16b);
        src6_8x16b = _mm_sub_epi16(src6_8x16b, pred6_8x16b);
        src7_8x16b = _mm_sub_epi16(src7_8x16b, pred7_8x16b);

        src8_8x16b  = _mm_loadu_si128((__m128i *)org);
        src9_8x16b  = _mm_loadu_si128((__m128i *)(org + 8));
        org         = org + s_org;
        src10_8x16b = _mm_loadu_si128((__m128i *)org);
        src11_8x16b = _mm_loadu_si128((__m128i *)(org + 8));
        org         = org + s_org;
        src12_8x16b = _mm_loadu_si128((__m128i *)org);
        src13_8x16b = _mm_loadu_si128((__m128i *)(org + 8));
        org         = org + s_org;
        src14_8x16b = _mm_loadu_si128((__m128i *)org);
        src15_8x16b = _mm_loadu_si128((__m128i *)(org + 8));
        org         = org + s_org;

        pred8_8x16b  = _mm_loadu_si128((__m128i *)cur);
        pred9_8x16b  = _mm_loadu_si128((__m128i *)(cur + 8));
        cur          = cur + s_cur;
        pred10_8x16b = _mm_loadu_si128((__m128i *)cur);
        pred11_8x16b = _mm_loadu_si128((__m128i *)(cur + 8));
        cur          = cur + s_cur;
        pred12_8x16b = _mm_loadu_si128((__m128i *)cur);
        pred13_8x16b = _mm_loadu_si128((__m128i *)(cur + 8));
        cur          = cur + s_cur;
        pred14_8x16b = _mm_loadu_si128((__m128i *)cur);
        pred15_8x16b = _mm_loadu_si128((__m128i *)(cur + 8));
        cur          = cur + s_cur;
    }
    else {
        src0_8x16b = _mm_loadu_si128((__m128i *)org);
        org        = org + s_org;
        src1_8x16b = _mm_loadu_si128((__m128i *)org);
        org        = org + s_org;
        src2_8x16b = _mm_loadu_si128((__m128i *)org);
        org        = org + s_org;
        src3_8x16b = _mm_loadu_si128((__m128i *)org);
        org        = org + s_org;
        src4_8x16b = _mm_loadu_si128((__m128i *)org);
        org        = org + s_org;
        src5_8x16b = _mm_loadu_si128((__m128i *)org);
        org        = org + s_org;
        src6_8x16b = _mm_loadu_si128((__m128i *)org);
        org        = org + s_org;
        src7_8x16b = _mm_loadu_si128((__m128i *)org);
        org        = org + s_org;

        pred0_8x16b = _mm_loadu_si128((__m128i *)cur);
        cur         = cur + s_cur;
        pred1_8x16b = _mm_loadu_si128((__m128i *)cur);
        cur         = cur + s_cur;
        pred2_8x16b = _mm_loadu_si128((__m128i *)cur);
        cur         = cur + s_cur;
        pred3_8x16b = _mm_loadu_si128((__m128i *)cur);
        cur         = cur + s_cur;
        pred4_8x16b = _mm_loadu_si128((__m128i *)cur);
        cur         = cur + s_cur;
        pred5_8x16b = _mm_loadu_si128((__m128i *)cur);
        cur         = cur + s_cur;
        pred6_8x16b = _mm_loadu_si128((__m128i *)cur);
        cur         = cur + s_cur;
        pred7_8x16b = _mm_loadu_si128((__m128i *)cur);
        cur         = cur + s_cur;

        src0_8x16b = _mm_sub_epi16(src0_8x16b, pred0_8x16b);
        src1_8x16b = _mm_sub_epi16(src1_8x16b, pred1_8x16b);
        src2_8x16b = _mm_sub_epi16(src2_8x16b, pred2_8x16b);
        src3_8x16b = _mm_sub_epi16(src3_8x16b, pred3_8x16b);
        src4_8x16b = _mm_sub_epi16(src4_8x16b, pred4_8x16b);
        src5_8x16b = _mm_sub_epi16(src5_8x16b, pred5_8x16b);
        src6_8x16b = _mm_sub_epi16(src6_8x16b, pred6_8x16b);
        src7_8x16b = _mm_sub_epi16(src7_8x16b, pred7_8x16b);

        src8_8x16b  = _mm_loadu_si128((__m128i *)org);
        org         = org + s_org;
        src9_8x16b  = _mm_loadu_si128((__m128i *)org);
        org         = org + s_org;
        src10_8x16b = _mm_loadu_si128((__m128i *)org);
        org         = org + s_org;
        src11_8x16b = _mm_loadu_si128((__m128i *)org);
        org         = org + s_org;
        src12_8x16b = _mm_loadu_si128((__m128i *)org);
        org         = org + s_org;
        src13_8x16b = _mm_loadu_si128((__m128i *)org);
        org         = org + s_org;
        src14_8x16b = _mm_loadu_si128((__m128i *)org);
        org         = org + s_org;
        src15_8x16b = _mm_loadu_si128((__m128i *)org);
        org         = org + s_org;

        pred8_8x16b  = _mm_loadu_si128((__m128i *)cur);
        cur          = cur + s_cur;
        pred9_8x16b  = _mm_loadu_si128((__m128i *)cur);
        cur          = cur + s_cur;
        pred10_8x16b = _mm_loadu_si128((__m128i *)cur);
        cur          = cur + s_cur;
        pred11_8x16b = _mm_loadu_si128((__m128i *)cur);
        cur          = cur + s_cur;
        pred12_8x16b = _mm_loadu_si128((__m128i *)cur);
        cur          = cur + s_cur;
        pred13_8x16b = _mm_loadu_si128((__m128i *)cur);
        cur          = cur + s_cur;
        pred14_8x16b = _mm_loadu_si128((__m128i *)cur);
        cur          = cur + s_cur;
        pred15_8x16b = _mm_loadu_si128((__m128i *)cur);
        cur          = cur + s_cur;
    }

    src8_8x16b  = _mm_sub_epi16(src8_8x16b, pred8_8x16b);
    src9_8x16b  = _mm_sub_epi16(src9_8x16b, pred9_8x16b);
    src10_8x16b = _mm_sub_epi16(src10_8x16b, pred10_8x16b);
    src11_8x16b = _mm_sub_epi16(src11_8x16b, pred11_8x16b);
    src12_8x16b = _mm_sub_epi16(src12_8x16b, pred12_8x16b);
    src13_8x16b = _mm_sub_epi16(src13_8x16b, pred13_8x16b);
    src14_8x16b = _mm_sub_epi16(src14_8x16b, pred14_8x16b);
    src15_8x16b = _mm_sub_epi16(src15_8x16b, pred15_8x16b);
    /**********************Residue Calculation********************************/

    /**************** 8x8 horizontal transform *******************************/
    /***********************    8x8 16 bit Transpose  ************************/
    out3_8x16b  = _mm_unpacklo_epi16(src0_8x16b, src1_8x16b);
    pred0_8x16b = _mm_unpacklo_epi16(src2_8x16b, src3_8x16b);
    out2_8x16b  = _mm_unpacklo_epi16(src4_8x16b, src5_8x16b);
    pred3_8x16b = _mm_unpacklo_epi16(src6_8x16b, src7_8x16b);
    out7_8x16b  = _mm_unpackhi_epi16(src0_8x16b, src1_8x16b);
    src2_8x16b  = _mm_unpackhi_epi16(src2_8x16b, src3_8x16b);
    pred7_8x16b = _mm_unpackhi_epi16(src4_8x16b, src5_8x16b);
    src6_8x16b  = _mm_unpackhi_epi16(src6_8x16b, src7_8x16b);

    out1_8x16b  = _mm_unpacklo_epi32(out3_8x16b, pred0_8x16b);
    out3_8x16b  = _mm_unpackhi_epi32(out3_8x16b, pred0_8x16b);
    pred1_8x16b = _mm_unpacklo_epi32(out2_8x16b, pred3_8x16b);
    pred3_8x16b = _mm_unpackhi_epi32(out2_8x16b, pred3_8x16b);
    out5_8x16b  = _mm_unpacklo_epi32(out7_8x16b, src2_8x16b);
    out7_8x16b  = _mm_unpackhi_epi32(out7_8x16b, src2_8x16b);
    pred5_8x16b = _mm_unpacklo_epi32(pred7_8x16b, src6_8x16b);
    pred7_8x16b = _mm_unpackhi_epi32(pred7_8x16b, src6_8x16b);

    out0_8x16b = _mm_unpacklo_epi64(out1_8x16b, pred1_8x16b);
    out1_8x16b = _mm_unpackhi_epi64(out1_8x16b, pred1_8x16b);
    out2_8x16b = _mm_unpacklo_epi64(out3_8x16b, pred3_8x16b);
    out3_8x16b = _mm_unpackhi_epi64(out3_8x16b, pred3_8x16b);
    out4_8x16b = _mm_unpacklo_epi64(out5_8x16b, pred5_8x16b);
    out5_8x16b = _mm_unpackhi_epi64(out5_8x16b, pred5_8x16b);
    out6_8x16b = _mm_unpacklo_epi64(out7_8x16b, pred7_8x16b);
    out7_8x16b = _mm_unpackhi_epi64(out7_8x16b, pred7_8x16b);
    /**********************   8x8 16 bit Transpose End   *********************/

    /* r0 + r1 */
    pred0_8x16b = _mm_add_epi16(out0_8x16b, out1_8x16b);
    /* r2 + r3 */
    pred2_8x16b = _mm_add_epi16(out2_8x16b, out3_8x16b);
    /* r4 + r5 */
    pred4_8x16b = _mm_add_epi16(out4_8x16b, out5_8x16b);
    /* r6 + r7 */
    pred6_8x16b = _mm_add_epi16(out6_8x16b, out7_8x16b);

    /* r0 + r1 + r2 + r3 */
    pred1_8x16b = _mm_add_epi16(pred0_8x16b, pred2_8x16b);
    /* r4 + r5 + r6 + r7 */
    pred5_8x16b = _mm_add_epi16(pred4_8x16b, pred6_8x16b);
    /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
    src0_8x16b  = _mm_add_epi16(pred1_8x16b, pred5_8x16b);
    /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
    src4_8x16b  = _mm_sub_epi16(pred1_8x16b, pred5_8x16b);

    /* r0 + r1 - r2 - r3 */
    pred1_8x16b = _mm_sub_epi16(pred0_8x16b, pred2_8x16b);
    /* r4 + r5 - r6 - r7 */
    pred5_8x16b = _mm_sub_epi16(pred4_8x16b, pred6_8x16b);
    /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
    src2_8x16b  = _mm_add_epi16(pred1_8x16b, pred5_8x16b);
    /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
    src6_8x16b  = _mm_sub_epi16(pred1_8x16b, pred5_8x16b);

    /* r0 - r1 */
    pred0_8x16b = _mm_sub_epi16(out0_8x16b, out1_8x16b);
    /* r2 - r3 */
    pred2_8x16b = _mm_sub_epi16(out2_8x16b, out3_8x16b);
    /* r4 - r5 */
    pred4_8x16b = _mm_sub_epi16(out4_8x16b, out5_8x16b);
    /* r6 - r7 */
    pred6_8x16b = _mm_sub_epi16(out6_8x16b, out7_8x16b);

    /* r0 - r1 + r2 - r3 */
    pred1_8x16b = _mm_add_epi16(pred0_8x16b, pred2_8x16b);
    /* r4 - r5 + r6 - r7 */
    pred5_8x16b = _mm_add_epi16(pred4_8x16b, pred6_8x16b);
    /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
    src1_8x16b  = _mm_add_epi16(pred1_8x16b, pred5_8x16b);
    /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
    src5_8x16b  = _mm_sub_epi16(pred1_8x16b, pred5_8x16b);

    /* r0 - r1 - r2 + r3 */
    pred1_8x16b = _mm_sub_epi16(pred0_8x16b, pred2_8x16b);
    /* r4 - r5 - r6 + r7 */
    pred5_8x16b = _mm_sub_epi16(pred4_8x16b, pred6_8x16b);
    /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
    src3_8x16b  = _mm_add_epi16(pred1_8x16b, pred5_8x16b);
    /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
    src7_8x16b  = _mm_sub_epi16(pred1_8x16b, pred5_8x16b);

    /***********************    8x8 16 bit Transpose  ************************/
    out3_8x16b  = _mm_unpacklo_epi16(src0_8x16b, src1_8x16b);
    pred0_8x16b = _mm_unpacklo_epi16(src2_8x16b, src3_8x16b);
    out2_8x16b  = _mm_unpacklo_epi16(src4_8x16b, src5_8x16b);
    pred3_8x16b = _mm_unpacklo_epi16(src6_8x16b, src7_8x16b);
    out7_8x16b  = _mm_unpackhi_epi16(src0_8x16b, src1_8x16b);
    src2_8x16b  = _mm_unpackhi_epi16(src2_8x16b, src3_8x16b);
    pred7_8x16b = _mm_unpackhi_epi16(src4_8x16b, src5_8x16b);
    src6_8x16b  = _mm_unpackhi_epi16(src6_8x16b, src7_8x16b);

    out1_8x16b  = _mm_unpacklo_epi32(out3_8x16b, pred0_8x16b);
    out3_8x16b  = _mm_unpackhi_epi32(out3_8x16b, pred0_8x16b);
    pred1_8x16b = _mm_unpacklo_epi32(out2_8x16b, pred3_8x16b);
    pred3_8x16b = _mm_unpackhi_epi32(out2_8x16b, pred3_8x16b);
    out5_8x16b  = _mm_unpacklo_epi32(out7_8x16b, src2_8x16b);
    out7_8x16b  = _mm_unpackhi_epi32(out7_8x16b, src2_8x16b);
    pred5_8x16b = _mm_unpacklo_epi32(pred7_8x16b, src6_8x16b);
    pred7_8x16b = _mm_unpackhi_epi32(pred7_8x16b, src6_8x16b);

    src0_8x16b = _mm_unpacklo_epi64(out1_8x16b, pred1_8x16b);
    src1_8x16b = _mm_unpackhi_epi64(out1_8x16b, pred1_8x16b);
    src2_8x16b = _mm_unpacklo_epi64(out3_8x16b, pred3_8x16b);
    src3_8x16b = _mm_unpackhi_epi64(out3_8x16b, pred3_8x16b);
    src4_8x16b = _mm_unpacklo_epi64(out5_8x16b, pred5_8x16b);
    src5_8x16b = _mm_unpackhi_epi64(out5_8x16b, pred5_8x16b);
    src6_8x16b = _mm_unpacklo_epi64(out7_8x16b, pred7_8x16b);
    src7_8x16b = _mm_unpackhi_epi64(out7_8x16b, pred7_8x16b);
    /**********************   8x8 16 bit Transpose End   *********************/
    /**************** 8x8 horizontal transform *******************************/

    /**************** 8x8 horizontal transform *******************************/
    /***********************    8x8 16 bit Transpose  ************************/
    out3_8x16b  = _mm_unpacklo_epi16(src8_8x16b, src9_8x16b);
    pred0_8x16b = _mm_unpacklo_epi16(src10_8x16b, src11_8x16b);
    out2_8x16b  = _mm_unpacklo_epi16(src12_8x16b, src13_8x16b);
    pred3_8x16b = _mm_unpacklo_epi16(src14_8x16b, src15_8x16b);
    out7_8x16b  = _mm_unpackhi_epi16(src8_8x16b, src9_8x16b);
    src10_8x16b = _mm_unpackhi_epi16(src10_8x16b, src11_8x16b);
    pred7_8x16b = _mm_unpackhi_epi16(src12_8x16b, src13_8x16b);
    src14_8x16b = _mm_unpackhi_epi16(src14_8x16b, src15_8x16b);

    out1_8x16b  = _mm_unpacklo_epi32(out3_8x16b, pred0_8x16b);
    out3_8x16b  = _mm_unpackhi_epi32(out3_8x16b, pred0_8x16b);
    pred1_8x16b = _mm_unpacklo_epi32(out2_8x16b, pred3_8x16b);
    pred3_8x16b = _mm_unpackhi_epi32(out2_8x16b, pred3_8x16b);
    out5_8x16b  = _mm_unpacklo_epi32(out7_8x16b, src10_8x16b);
    out7_8x16b  = _mm_unpackhi_epi32(out7_8x16b, src10_8x16b);
    pred5_8x16b = _mm_unpacklo_epi32(pred7_8x16b, src14_8x16b);
    pred7_8x16b = _mm_unpackhi_epi32(pred7_8x16b, src14_8x16b);

    out0_8x16b = _mm_unpacklo_epi64(out1_8x16b, pred1_8x16b);
    out1_8x16b = _mm_unpackhi_epi64(out1_8x16b, pred1_8x16b);
    out2_8x16b = _mm_unpacklo_epi64(out3_8x16b, pred3_8x16b);
    out3_8x16b = _mm_unpackhi_epi64(out3_8x16b, pred3_8x16b);
    out4_8x16b = _mm_unpacklo_epi64(out5_8x16b, pred5_8x16b);
    out5_8x16b = _mm_unpackhi_epi64(out5_8x16b, pred5_8x16b);
    out6_8x16b = _mm_unpacklo_epi64(out7_8x16b, pred7_8x16b);
    out7_8x16b = _mm_unpackhi_epi64(out7_8x16b, pred7_8x16b);
    /**********************   8x8 16 bit Transpose End   *********************/

    /* r0 + r1 */
    pred0_8x16b = _mm_add_epi16(out0_8x16b, out1_8x16b);
    /* r2 + r3 */
    pred2_8x16b = _mm_add_epi16(out2_8x16b, out3_8x16b);
    /* r4 + r5 */
    pred4_8x16b = _mm_add_epi16(out4_8x16b, out5_8x16b);
    /* r6 + r7 */
    pred6_8x16b = _mm_add_epi16(out6_8x16b, out7_8x16b);

    /* r0 + r1 + r2 + r3 */
    pred1_8x16b = _mm_add_epi16(pred0_8x16b, pred2_8x16b);
    /* r4 + r5 + r6 + r7 */
    pred5_8x16b = _mm_add_epi16(pred4_8x16b, pred6_8x16b);
    /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
    src8_8x16b  = _mm_add_epi16(pred1_8x16b, pred5_8x16b);
    /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
    src12_8x16b = _mm_sub_epi16(pred1_8x16b, pred5_8x16b);

    /* r0 + r1 - r2 - r3 */
    pred1_8x16b = _mm_sub_epi16(pred0_8x16b, pred2_8x16b);
    /* r4 + r5 - r6 - r7 */
    pred5_8x16b = _mm_sub_epi16(pred4_8x16b, pred6_8x16b);
    /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
    src10_8x16b = _mm_add_epi16(pred1_8x16b, pred5_8x16b);
    /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
    src14_8x16b = _mm_sub_epi16(pred1_8x16b, pred5_8x16b);

    /* r0 - r1 */
    pred0_8x16b = _mm_sub_epi16(out0_8x16b, out1_8x16b);
    /* r2 - r3 */
    pred2_8x16b = _mm_sub_epi16(out2_8x16b, out3_8x16b);
    /* r4 - r5 */
    pred4_8x16b = _mm_sub_epi16(out4_8x16b, out5_8x16b);
    /* r6 - r7 */
    pred6_8x16b = _mm_sub_epi16(out6_8x16b, out7_8x16b);

    /* r0 - r1 + r2 - r3 */
    pred1_8x16b = _mm_add_epi16(pred0_8x16b, pred2_8x16b);
    /* r4 - r5 + r6 - r7 */
    pred5_8x16b = _mm_add_epi16(pred4_8x16b, pred6_8x16b);
    /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
    src9_8x16b  = _mm_add_epi16(pred1_8x16b, pred5_8x16b);
    /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
    src13_8x16b = _mm_sub_epi16(pred1_8x16b, pred5_8x16b);

    /* r0 - r1 - r2 + r3 */
    pred1_8x16b = _mm_sub_epi16(pred0_8x16b, pred2_8x16b);
    /* r4 - r5 - r6 + r7 */
    pred5_8x16b = _mm_sub_epi16(pred4_8x16b, pred6_8x16b);
    /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
    src11_8x16b = _mm_add_epi16(pred1_8x16b, pred5_8x16b);
    /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
    src15_8x16b = _mm_sub_epi16(pred1_8x16b, pred5_8x16b);

    /***********************    8x8 16 bit Transpose  ************************/
    out3_8x16b  = _mm_unpacklo_epi16(src8_8x16b, src9_8x16b);
    pred0_8x16b = _mm_unpacklo_epi16(src10_8x16b, src11_8x16b);
    out2_8x16b  = _mm_unpacklo_epi16(src12_8x16b, src13_8x16b);
    pred3_8x16b = _mm_unpacklo_epi16(src14_8x16b, src15_8x16b);
    out7_8x16b  = _mm_unpackhi_epi16(src8_8x16b, src9_8x16b);
    src10_8x16b = _mm_unpackhi_epi16(src10_8x16b, src11_8x16b);
    pred7_8x16b = _mm_unpackhi_epi16(src12_8x16b, src13_8x16b);
    src14_8x16b = _mm_unpackhi_epi16(src14_8x16b, src15_8x16b);

    out1_8x16b  = _mm_unpacklo_epi32(out3_8x16b, pred0_8x16b);
    out3_8x16b  = _mm_unpackhi_epi32(out3_8x16b, pred0_8x16b);
    pred1_8x16b = _mm_unpacklo_epi32(out2_8x16b, pred3_8x16b);
    pred3_8x16b = _mm_unpackhi_epi32(out2_8x16b, pred3_8x16b);
    out5_8x16b  = _mm_unpacklo_epi32(out7_8x16b, src10_8x16b);
    out7_8x16b  = _mm_unpackhi_epi32(out7_8x16b, src10_8x16b);
    pred5_8x16b = _mm_unpacklo_epi32(pred7_8x16b, src14_8x16b);
    pred7_8x16b = _mm_unpackhi_epi32(pred7_8x16b, src14_8x16b);

    src8_8x16b  = _mm_unpacklo_epi64(out1_8x16b, pred1_8x16b);
    src9_8x16b  = _mm_unpackhi_epi64(out1_8x16b, pred1_8x16b);
    src10_8x16b = _mm_unpacklo_epi64(out3_8x16b, pred3_8x16b);
    src11_8x16b = _mm_unpackhi_epi64(out3_8x16b, pred3_8x16b);
    src12_8x16b = _mm_unpacklo_epi64(out5_8x16b, pred5_8x16b);
    src13_8x16b = _mm_unpackhi_epi64(out5_8x16b, pred5_8x16b);
    src14_8x16b = _mm_unpacklo_epi64(out7_8x16b, pred7_8x16b);
    src15_8x16b = _mm_unpackhi_epi64(out7_8x16b, pred7_8x16b);
    /**********************   8x8 16 bit Transpose End   *********************/
    /**************** 8x8 horizontal transform *******************************/

    /****************Horizontal Transform Addition****************************/
    out0_8x16b = _mm_add_epi16(src0_8x16b, src1_8x16b);
    out1_8x16b = _mm_sub_epi16(src0_8x16b, src1_8x16b);

    out2_8x16b = _mm_add_epi16(src2_8x16b, src3_8x16b);
    out3_8x16b = _mm_sub_epi16(src2_8x16b, src3_8x16b);

    out4_8x16b = _mm_add_epi16(src4_8x16b, src5_8x16b);
    out5_8x16b = _mm_sub_epi16(src4_8x16b, src5_8x16b);

    out6_8x16b = _mm_add_epi16(src6_8x16b, src7_8x16b);
    out7_8x16b = _mm_sub_epi16(src6_8x16b, src7_8x16b);

    out8_8x16b = _mm_add_epi16(src8_8x16b, src9_8x16b);
    out9_8x16b = _mm_sub_epi16(src8_8x16b, src9_8x16b);

    out10_8x16b = _mm_add_epi16(src10_8x16b, src11_8x16b);
    out11_8x16b = _mm_sub_epi16(src10_8x16b, src11_8x16b);

    out12_8x16b = _mm_add_epi16(src12_8x16b, src13_8x16b);
    out13_8x16b = _mm_sub_epi16(src12_8x16b, src13_8x16b);

    out14_8x16b = _mm_add_epi16(src14_8x16b, src15_8x16b);
    out15_8x16b = _mm_sub_epi16(src14_8x16b, src15_8x16b);
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

    {
        __m128i out0a_8x16b, out1a_8x16b, out2a_8x16b, out3a_8x16b;
        __m128i out4a_8x16b, out5a_8x16b, out6a_8x16b, out7a_8x16b;
        __m128i out8a_8x16b, out9a_8x16b, out10a_8x16b, out11a_8x16b;
        __m128i out12a_8x16b, out13a_8x16b, out14a_8x16b, out15a_8x16b;
        __m128i tmp0_8x16b, tmp1_8x16b, tmp2_8x16b, tmp3_8x16b;
        __m128i tmp4_8x16b, tmp5_8x16b, tmp6_8x16b, tmp7_8x16b;
        __m128i tmp8_8x16b, tmp9_8x16b, tmp10_8x16b, tmp11_8x16b;
        __m128i tmp12_8x16b, tmp13_8x16b, tmp14_8x16b, tmp15_8x16b;

        /************************* 8x8 Vertical Transform*************************/
        tmp0_8x16b  = _mm_srli_si128(src0_8x16b, 8);
        tmp2_8x16b  = _mm_srli_si128(src2_8x16b, 8);
        tmp4_8x16b  = _mm_srli_si128(src4_8x16b, 8);
        tmp6_8x16b  = _mm_srli_si128(src6_8x16b, 8);
        tmp8_8x16b  = _mm_srli_si128(src8_8x16b, 8);
        tmp10_8x16b = _mm_srli_si128(src10_8x16b, 8);
        tmp12_8x16b = _mm_srli_si128(src12_8x16b, 8);
        tmp14_8x16b = _mm_srli_si128(src14_8x16b, 8);

        /*************************First 4 pixels ********************************/
        src0_8x16b  = _mm_cvtepi16_epi32(src0_8x16b);
        src2_8x16b  = _mm_cvtepi16_epi32(src2_8x16b);
        src4_8x16b  = _mm_cvtepi16_epi32(src4_8x16b);
        src6_8x16b  = _mm_cvtepi16_epi32(src6_8x16b);
        src8_8x16b  = _mm_cvtepi16_epi32(src8_8x16b);
        src10_8x16b = _mm_cvtepi16_epi32(src10_8x16b);
        src12_8x16b = _mm_cvtepi16_epi32(src12_8x16b);
        src14_8x16b = _mm_cvtepi16_epi32(src14_8x16b);

        /* r0 + r1 */
        pred0_8x16b = _mm_add_epi32(src0_8x16b, src2_8x16b);
        /* r2 + r3 */
        pred2_8x16b = _mm_add_epi32(src4_8x16b, src6_8x16b);
        /* r4 + r5 */
        pred4_8x16b = _mm_add_epi32(src8_8x16b, src10_8x16b);
        /* r6 + r7 */
        pred6_8x16b = _mm_add_epi32(src12_8x16b, src14_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = _mm_add_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = _mm_add_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        out0_8x16b  = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        out8_8x16b  = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b = _mm_sub_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b = _mm_sub_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        out4_8x16b  = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        out12_8x16b = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = _mm_sub_epi32(src0_8x16b, src2_8x16b);
        /* r2 - r3 */
        pred2_8x16b = _mm_sub_epi32(src4_8x16b, src6_8x16b);
        /* r4 - r5 */
        pred4_8x16b = _mm_sub_epi32(src8_8x16b, src10_8x16b);
        /* r6 - r7 */
        pred6_8x16b = _mm_sub_epi32(src12_8x16b, src14_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b = _mm_add_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b = _mm_add_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        out2_8x16b  = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        out10_8x16b = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b = _mm_sub_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b = _mm_sub_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        out6_8x16b  = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        out14_8x16b = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);
        /*************************First 4 pixels ********************************/

        /**************************Next 4 pixels *******************************/
        src0_8x16b  = _mm_cvtepi16_epi32(tmp0_8x16b);
        src2_8x16b  = _mm_cvtepi16_epi32(tmp2_8x16b);
        src4_8x16b  = _mm_cvtepi16_epi32(tmp4_8x16b);
        src6_8x16b  = _mm_cvtepi16_epi32(tmp6_8x16b);
        src8_8x16b  = _mm_cvtepi16_epi32(tmp8_8x16b);
        src10_8x16b = _mm_cvtepi16_epi32(tmp10_8x16b);
        src12_8x16b = _mm_cvtepi16_epi32(tmp12_8x16b);
        src14_8x16b = _mm_cvtepi16_epi32(tmp14_8x16b);

        /* r0 + r1 */
        pred0_8x16b = _mm_add_epi32(src0_8x16b, src2_8x16b);
        /* r2 + r3 */
        pred2_8x16b = _mm_add_epi32(src4_8x16b, src6_8x16b);
        /* r4 + r5 */
        pred4_8x16b = _mm_add_epi32(src8_8x16b, src10_8x16b);
        /* r6 + r7 */
        pred6_8x16b = _mm_add_epi32(src12_8x16b, src14_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = _mm_add_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = _mm_add_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        out0a_8x16b = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        out8a_8x16b = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b  = _mm_sub_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b  = _mm_sub_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        out4a_8x16b  = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        out12a_8x16b = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = _mm_sub_epi32(src0_8x16b, src2_8x16b);
        /* r2 - r3 */
        pred2_8x16b = _mm_sub_epi32(src4_8x16b, src6_8x16b);
        /* r4 - r5 */
        pred4_8x16b = _mm_sub_epi32(src8_8x16b, src10_8x16b);
        /* r6 - r7 */
        pred6_8x16b = _mm_sub_epi32(src12_8x16b, src14_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b  = _mm_add_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b  = _mm_add_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        out2a_8x16b  = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        out10a_8x16b = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b  = _mm_sub_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b  = _mm_sub_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        out6a_8x16b  = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        out14a_8x16b = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);
        /**************************Next 4 pixels *******************************/
        /************************* 8x8 Vertical Transform*************************/

        /************************* 8x8 Vertical Transform*************************/
        tmp1_8x16b  = _mm_srli_si128(src1_8x16b, 8);
        tmp3_8x16b  = _mm_srli_si128(src3_8x16b, 8);
        tmp5_8x16b  = _mm_srli_si128(src5_8x16b, 8);
        tmp7_8x16b  = _mm_srli_si128(src7_8x16b, 8);
        tmp9_8x16b  = _mm_srli_si128(src9_8x16b, 8);
        tmp11_8x16b = _mm_srli_si128(src11_8x16b, 8);
        tmp13_8x16b = _mm_srli_si128(src13_8x16b, 8);
        tmp15_8x16b = _mm_srli_si128(src15_8x16b, 8);

        /*************************First 4 pixels ********************************/
        src1_8x16b  = _mm_cvtepi16_epi32(src1_8x16b);
        src3_8x16b  = _mm_cvtepi16_epi32(src3_8x16b);
        src5_8x16b  = _mm_cvtepi16_epi32(src5_8x16b);
        src7_8x16b  = _mm_cvtepi16_epi32(src7_8x16b);
        src9_8x16b  = _mm_cvtepi16_epi32(src9_8x16b);
        src11_8x16b = _mm_cvtepi16_epi32(src11_8x16b);
        src13_8x16b = _mm_cvtepi16_epi32(src13_8x16b);
        src15_8x16b = _mm_cvtepi16_epi32(src15_8x16b);

        /* r0 + r1 */
        pred0_8x16b = _mm_add_epi32(src1_8x16b, src3_8x16b);
        /* r2 + r3 */
        pred2_8x16b = _mm_add_epi32(src5_8x16b, src7_8x16b);
        /* r4 + r5 */
        pred4_8x16b = _mm_add_epi32(src9_8x16b, src11_8x16b);
        /* r6 + r7 */
        pred6_8x16b = _mm_add_epi32(src13_8x16b, src15_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = _mm_add_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = _mm_add_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        out1_8x16b  = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        out9_8x16b  = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b = _mm_sub_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b = _mm_sub_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        out5_8x16b  = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        out13_8x16b = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = _mm_sub_epi32(src1_8x16b, src3_8x16b);
        /* r2 - r3 */
        pred2_8x16b = _mm_sub_epi32(src5_8x16b, src7_8x16b);
        /* r4 - r5 */
        pred4_8x16b = _mm_sub_epi32(src9_8x16b, src11_8x16b);
        /* r6 - r7 */
        pred6_8x16b = _mm_sub_epi32(src13_8x16b, src15_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b = _mm_add_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b = _mm_add_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        out3_8x16b  = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        out11_8x16b = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b = _mm_sub_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b = _mm_sub_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        out7_8x16b  = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        out15_8x16b = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);
        /*************************First 4 pixels ********************************/

        /*************************Next 4 pixels ********************************/
        src1_8x16b  = _mm_cvtepi16_epi32(tmp1_8x16b);
        src3_8x16b  = _mm_cvtepi16_epi32(tmp3_8x16b);
        src5_8x16b  = _mm_cvtepi16_epi32(tmp5_8x16b);
        src7_8x16b  = _mm_cvtepi16_epi32(tmp7_8x16b);
        src9_8x16b  = _mm_cvtepi16_epi32(tmp9_8x16b);
        src11_8x16b = _mm_cvtepi16_epi32(tmp11_8x16b);
        src13_8x16b = _mm_cvtepi16_epi32(tmp13_8x16b);
        src15_8x16b = _mm_cvtepi16_epi32(tmp15_8x16b);

        /* r0 + r1 */
        pred0_8x16b = _mm_add_epi32(src1_8x16b, src3_8x16b);
        /* r2 + r3 */
        pred2_8x16b = _mm_add_epi32(src5_8x16b, src7_8x16b);
        /* r4 + r5 */
        pred4_8x16b = _mm_add_epi32(src9_8x16b, src11_8x16b);
        /* r6 + r7 */
        pred6_8x16b = _mm_add_epi32(src13_8x16b, src15_8x16b);

        /* r0 + r1 + r2 + r3 */
        pred1_8x16b = _mm_add_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 + r6 + r7 */
        pred5_8x16b = _mm_add_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 + r2 + r3 + r4 + r5 + r6 + r7 */
        out1a_8x16b = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 + r2 + r3 - r4 - r5 - r6 - r7 */
        out9a_8x16b = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);

        /* r0 + r1 - r2 - r3 */
        pred1_8x16b  = _mm_sub_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 + r5 - r6 - r7 */
        pred5_8x16b  = _mm_sub_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 + r1 - r2 - r3 + r4 + r5 - r6 - r7 */
        out5a_8x16b  = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 + r1 - r2 - r3 - r4 - r5 + r6 + r7 */
        out13a_8x16b = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 */
        pred0_8x16b = _mm_sub_epi32(src1_8x16b, src3_8x16b);
        /* r2 - r3 */
        pred2_8x16b = _mm_sub_epi32(src5_8x16b, src7_8x16b);
        /* r4 - r5 */
        pred4_8x16b = _mm_sub_epi32(src9_8x16b, src11_8x16b);
        /* r6 - r7 */
        pred6_8x16b = _mm_sub_epi32(src13_8x16b, src15_8x16b);

        /* r0 - r1 + r2 - r3 */
        pred1_8x16b  = _mm_add_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 + r6 - r7 */
        pred5_8x16b  = _mm_add_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 + r2 - r3 + r4 - r5 + r6 - r7 */
        out3a_8x16b  = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 + r2 - r3 - r4 + r5 - r6 + r7 */
        out11a_8x16b = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);

        /* r0 - r1 - r2 + r3 */
        pred1_8x16b  = _mm_sub_epi32(pred0_8x16b, pred2_8x16b);
        /* r4 - r5 - r6 + r7 */
        pred5_8x16b  = _mm_sub_epi32(pred4_8x16b, pred6_8x16b);
        /* r0 - r1 - r2 + r3 + r4 - r5 - r6 + r7 */
        out7a_8x16b  = _mm_add_epi32(pred1_8x16b, pred5_8x16b);
        /* r0 - r1 - r2 + r3 - r4 + r5 + r6 - r7 */
        out15a_8x16b = _mm_sub_epi32(pred1_8x16b, pred5_8x16b);
        /************************* 8x8 Vertical Transform*************************/

        /****************************SATD calculation ****************************/
        src0_8x16b = _mm_abs_epi32(out0_8x16b);
        src1_8x16b = _mm_abs_epi32(out1_8x16b);
        src2_8x16b = _mm_abs_epi32(out2_8x16b);
        src3_8x16b = _mm_abs_epi32(out3_8x16b);
        src4_8x16b = _mm_abs_epi32(out4_8x16b);
        src5_8x16b = _mm_abs_epi32(out5_8x16b);
        src6_8x16b = _mm_abs_epi32(out6_8x16b);
        src7_8x16b = _mm_abs_epi32(out7_8x16b);

        s32 *p = (s32 *)&src0_8x16b;
        p[0]   = p[0] >> 2;

        src0_8x16b = _mm_add_epi32(src0_8x16b, src1_8x16b);
        src2_8x16b = _mm_add_epi32(src2_8x16b, src3_8x16b);
        src4_8x16b = _mm_add_epi32(src4_8x16b, src5_8x16b);
        src6_8x16b = _mm_add_epi32(src6_8x16b, src7_8x16b);

        src0_8x16b = _mm_add_epi32(src0_8x16b, src2_8x16b);
        src4_8x16b = _mm_add_epi32(src4_8x16b, src6_8x16b);

        src0_8x16b = _mm_add_epi32(src0_8x16b, src4_8x16b);

        src0_8x16b = _mm_hadd_epi32(src0_8x16b, src0_8x16b);
        src0_8x16b = _mm_hadd_epi32(src0_8x16b, src0_8x16b);

        sad += _mm_cvtsi128_si32(src0_8x16b);

        src0_8x16b = _mm_abs_epi32(out8_8x16b);
        src1_8x16b = _mm_abs_epi32(out9_8x16b);
        src2_8x16b = _mm_abs_epi32(out10_8x16b);
        src3_8x16b = _mm_abs_epi32(out11_8x16b);
        src4_8x16b = _mm_abs_epi32(out12_8x16b);
        src5_8x16b = _mm_abs_epi32(out13_8x16b);
        src6_8x16b = _mm_abs_epi32(out14_8x16b);
        src7_8x16b = _mm_abs_epi32(out15_8x16b);

        src0_8x16b = _mm_add_epi32(src0_8x16b, src1_8x16b);
        src2_8x16b = _mm_add_epi32(src2_8x16b, src3_8x16b);
        src4_8x16b = _mm_add_epi32(src4_8x16b, src5_8x16b);
        src6_8x16b = _mm_add_epi32(src6_8x16b, src7_8x16b);

        src0_8x16b = _mm_add_epi32(src0_8x16b, src2_8x16b);
        src4_8x16b = _mm_add_epi32(src4_8x16b, src6_8x16b);

        src0_8x16b = _mm_add_epi32(src0_8x16b, src4_8x16b);

        src0_8x16b = _mm_hadd_epi32(src0_8x16b, src0_8x16b);
        src0_8x16b = _mm_hadd_epi32(src0_8x16b, src0_8x16b);

        sad += _mm_cvtsi128_si32(src0_8x16b);

        src0_8x16b = _mm_abs_epi32(out0a_8x16b);
        src1_8x16b = _mm_abs_epi32(out1a_8x16b);
        src2_8x16b = _mm_abs_epi32(out2a_8x16b);
        src3_8x16b = _mm_abs_epi32(out3a_8x16b);
        src4_8x16b = _mm_abs_epi32(out4a_8x16b);
        src5_8x16b = _mm_abs_epi32(out5a_8x16b);
        src6_8x16b = _mm_abs_epi32(out6a_8x16b);
        src7_8x16b = _mm_abs_epi32(out7a_8x16b);

        src0_8x16b = _mm_add_epi32(src0_8x16b, src1_8x16b);
        src2_8x16b = _mm_add_epi32(src2_8x16b, src3_8x16b);
        src4_8x16b = _mm_add_epi32(src4_8x16b, src5_8x16b);
        src6_8x16b = _mm_add_epi32(src6_8x16b, src7_8x16b);

        src0_8x16b = _mm_add_epi32(src0_8x16b, src2_8x16b);
        src4_8x16b = _mm_add_epi32(src4_8x16b, src6_8x16b);

        src0_8x16b = _mm_add_epi32(src0_8x16b, src4_8x16b);

        src0_8x16b = _mm_hadd_epi32(src0_8x16b, src0_8x16b);
        src0_8x16b = _mm_hadd_epi32(src0_8x16b, src0_8x16b);

        sad += _mm_cvtsi128_si32(src0_8x16b);

        src0_8x16b = _mm_abs_epi32(out8a_8x16b);
        src1_8x16b = _mm_abs_epi32(out9a_8x16b);
        src2_8x16b = _mm_abs_epi32(out10a_8x16b);
        src3_8x16b = _mm_abs_epi32(out11a_8x16b);
        src4_8x16b = _mm_abs_epi32(out12a_8x16b);
        src5_8x16b = _mm_abs_epi32(out13a_8x16b);
        src6_8x16b = _mm_abs_epi32(out14a_8x16b);
        src7_8x16b = _mm_abs_epi32(out15a_8x16b);

        src0_8x16b = _mm_add_epi32(src0_8x16b, src1_8x16b);
        src2_8x16b = _mm_add_epi32(src2_8x16b, src3_8x16b);
        src4_8x16b = _mm_add_epi32(src4_8x16b, src5_8x16b);
        src6_8x16b = _mm_add_epi32(src6_8x16b, src7_8x16b);

        src0_8x16b = _mm_add_epi32(src0_8x16b, src2_8x16b);
        src4_8x16b = _mm_add_epi32(src4_8x16b, src6_8x16b);

        src0_8x16b = _mm_add_epi32(src0_8x16b, src4_8x16b);

        src0_8x16b = _mm_hadd_epi32(src0_8x16b, src0_8x16b);
        src0_8x16b = _mm_hadd_epi32(src0_8x16b, src0_8x16b);

        sad += _mm_cvtsi128_si32(src0_8x16b);

        sad = (int)(sad / sqrt(16.0 * 8) * 2);

        return sad;
    }
}

int xeve_had_16x8_sse(pel *org, pel *cur, int s_org, int s_cur, int step, int bit_depth)
{
    xeve_assert(bit_depth == 10);
    return xeve_had_16x8_8x16_util(org, cur, s_org, s_cur, step, bit_depth, 0);
}

int xeve_had_8x16_sse(pel *org, pel *cur, int s_org, int s_cur, int step, int bit_depth)
{
    xeve_assert(bit_depth == 10);
    return xeve_had_16x8_8x16_util(org, cur, s_org, s_cur, step, bit_depth, 1);
}

int xeve_had_8x4_sse(pel *org, pel *cur, int s_org, int s_cur, int step, int bit_depth)
{
    xeve_assert(bit_depth == 10);
    int     k, i;
    int     satd = 0;
    __m128i m1[8], m2[8];
    __m128i vzero = _mm_setzero_si128();
    __m128i sum;

    for(k = 0; k < 4; k++) {
        __m128i r0 = (_mm_loadu_si128((__m128i *)org));
        __m128i r1 = (_mm_lddqu_si128((__m128i *)cur));
        m1[k]      = _mm_sub_epi16(r0, r1);
        org += s_org;
        cur += s_cur;
    }

    // vertical
    m2[0] = _mm_add_epi16(m1[0], m1[2]);
    m2[1] = _mm_add_epi16(m1[1], m1[3]);
    m2[2] = _mm_sub_epi16(m1[0], m1[2]);
    m2[3] = _mm_sub_epi16(m1[1], m1[3]);

    m1[0] = _mm_add_epi16(m2[0], m2[1]);
    m1[1] = _mm_sub_epi16(m2[0], m2[1]);
    m1[2] = _mm_add_epi16(m2[2], m2[3]);
    m1[3] = _mm_sub_epi16(m2[2], m2[3]);

    // transpose, partially
    m2[0] = _mm_unpacklo_epi16(m1[0], m1[1]);
    m2[1] = _mm_unpacklo_epi16(m1[2], m1[3]);
    m2[2] = _mm_unpackhi_epi16(m1[0], m1[1]);
    m2[3] = _mm_unpackhi_epi16(m1[2], m1[3]);

    m1[0] = _mm_unpacklo_epi32(m2[0], m2[1]);
    m1[1] = _mm_unpackhi_epi32(m2[0], m2[1]);
    m1[2] = _mm_unpacklo_epi32(m2[2], m2[3]);
    m1[3] = _mm_unpackhi_epi32(m2[2], m2[3]);

    // horizontal
    // finish transpose
    m2[0] = _mm_unpacklo_epi64(m1[0], vzero);
    m2[1] = _mm_unpackhi_epi64(m1[0], vzero);
    m2[2] = _mm_unpacklo_epi64(m1[1], vzero);
    m2[3] = _mm_unpackhi_epi64(m1[1], vzero);
    m2[4] = _mm_unpacklo_epi64(m1[2], vzero);
    m2[5] = _mm_unpackhi_epi64(m1[2], vzero);
    m2[6] = _mm_unpacklo_epi64(m1[3], vzero);
    m2[7] = _mm_unpackhi_epi64(m1[3], vzero);

    for(i = 0; i < 8; i++) {
        m2[i] = _mm_cvtepi16_epi32(m2[i]);
    }

    m1[0] = _mm_add_epi32(m2[0], m2[4]);
    m1[1] = _mm_add_epi32(m2[1], m2[5]);
    m1[2] = _mm_add_epi32(m2[2], m2[6]);
    m1[3] = _mm_add_epi32(m2[3], m2[7]);
    m1[4] = _mm_sub_epi32(m2[0], m2[4]);
    m1[5] = _mm_sub_epi32(m2[1], m2[5]);
    m1[6] = _mm_sub_epi32(m2[2], m2[6]);
    m1[7] = _mm_sub_epi32(m2[3], m2[7]);

    m2[0] = _mm_add_epi32(m1[0], m1[2]);
    m2[1] = _mm_add_epi32(m1[1], m1[3]);
    m2[2] = _mm_sub_epi32(m1[0], m1[2]);
    m2[3] = _mm_sub_epi32(m1[1], m1[3]);
    m2[4] = _mm_add_epi32(m1[4], m1[6]);
    m2[5] = _mm_add_epi32(m1[5], m1[7]);
    m2[6] = _mm_sub_epi32(m1[4], m1[6]);
    m2[7] = _mm_sub_epi32(m1[5], m1[7]);

    m1[0] = _mm_abs_epi32(_mm_add_epi32(m2[0], m2[1]));
    m1[1] = _mm_abs_epi32(_mm_sub_epi32(m2[0], m2[1]));
    m1[2] = _mm_abs_epi32(_mm_add_epi32(m2[2], m2[3]));
    m1[3] = _mm_abs_epi32(_mm_sub_epi32(m2[2], m2[3]));
    m1[4] = _mm_abs_epi32(_mm_add_epi32(m2[4], m2[5]));
    m1[5] = _mm_abs_epi32(_mm_sub_epi32(m2[4], m2[5]));
    m1[6] = _mm_abs_epi32(_mm_add_epi32(m2[6], m2[7]));
    m1[7] = _mm_abs_epi32(_mm_sub_epi32(m2[6], m2[7]));

    s32 *p = (s32 *)&m1[0];
    p[0]   = p[0] >> 2;

    m1[0] = _mm_add_epi32(m1[0], m1[1]);
    m1[1] = _mm_add_epi32(m1[2], m1[3]);
    m1[2] = _mm_add_epi32(m1[4], m1[5]);
    m1[3] = _mm_add_epi32(m1[6], m1[7]);

    m1[0] = _mm_add_epi32(m1[0], m1[1]);
    m1[1] = _mm_add_epi32(m1[2], m1[3]);

    sum = _mm_add_epi32(m1[0], m1[1]);

    sum = _mm_hadd_epi32(sum, sum);
    sum = _mm_hadd_epi32(sum, sum);

    satd = _mm_cvtsi128_si32(sum);
    satd = (int)(satd / sqrt(4.0 * 8) * 2);

    return satd;
}

int xeve_had_4x8_sse(pel *org, pel *cur, int s_org, int s_cur, int step, int bit_depth)
{
    xeve_assert(bit_depth == 10);
    int     k, i;
    __m128i m1[8], m2[8];
    __m128i n1[4][2];
    __m128i n2[4][2];
    __m128i sum;
    int     satd = 0;

    for(k = 0; k < 8; k++) {
        __m128i r0 = (_mm_loadl_epi64((__m128i *)org));
        __m128i r1 = (_mm_loadl_epi64((__m128i *)cur));
        m2[k]      = _mm_sub_epi16(r0, r1);
        org += s_org;
        cur += s_cur;
    }

    // vertical

    m1[0] = _mm_add_epi16(m2[0], m2[4]);
    m1[1] = _mm_add_epi16(m2[1], m2[5]);
    m1[2] = _mm_add_epi16(m2[2], m2[6]);
    m1[3] = _mm_add_epi16(m2[3], m2[7]);
    m1[4] = _mm_sub_epi16(m2[0], m2[4]);
    m1[5] = _mm_sub_epi16(m2[1], m2[5]);
    m1[6] = _mm_sub_epi16(m2[2], m2[6]);
    m1[7] = _mm_sub_epi16(m2[3], m2[7]);

    m2[0] = _mm_add_epi16(m1[0], m1[2]);
    m2[1] = _mm_add_epi16(m1[1], m1[3]);
    m2[2] = _mm_sub_epi16(m1[0], m1[2]);
    m2[3] = _mm_sub_epi16(m1[1], m1[3]);
    m2[4] = _mm_add_epi16(m1[4], m1[6]);
    m2[5] = _mm_add_epi16(m1[5], m1[7]);
    m2[6] = _mm_sub_epi16(m1[4], m1[6]);
    m2[7] = _mm_sub_epi16(m1[5], m1[7]);

    m1[0] = _mm_add_epi16(m2[0], m2[1]);
    m1[1] = _mm_sub_epi16(m2[0], m2[1]);
    m1[2] = _mm_add_epi16(m2[2], m2[3]);
    m1[3] = _mm_sub_epi16(m2[2], m2[3]);
    m1[4] = _mm_add_epi16(m2[4], m2[5]);
    m1[5] = _mm_sub_epi16(m2[4], m2[5]);
    m1[6] = _mm_add_epi16(m2[6], m2[7]);
    m1[7] = _mm_sub_epi16(m2[6], m2[7]);

    // horizontal
    // transpose

    m2[0] = _mm_unpacklo_epi16(m1[0], m1[1]);
    m2[1] = _mm_unpacklo_epi16(m1[2], m1[3]);
    m2[2] = _mm_unpacklo_epi16(m1[4], m1[5]);
    m2[3] = _mm_unpacklo_epi16(m1[6], m1[7]);

    m1[0] = _mm_unpacklo_epi32(m2[0], m2[1]);
    m1[1] = _mm_unpackhi_epi32(m2[0], m2[1]);
    m1[2] = _mm_unpacklo_epi32(m2[2], m2[3]);
    m1[3] = _mm_unpackhi_epi32(m2[2], m2[3]);

    m2[0] = _mm_unpacklo_epi64(m1[0], m1[2]);
    m2[1] = _mm_unpackhi_epi64(m1[0], m1[2]);
    m2[2] = _mm_unpacklo_epi64(m1[1], m1[3]);
    m2[3] = _mm_unpackhi_epi64(m1[1], m1[3]);

    for(i = 0; i < 4; i++) {
        n1[i][0] = _mm_cvtepi16_epi32(m2[i]);
        n1[i][1] = _mm_cvtepi16_epi32(_mm_shuffle_epi32(m2[i], 0xEE));
    }

    for(i = 0; i < 2; i++) {
        n2[0][i] = _mm_add_epi32(n1[0][i], n1[2][i]);
        n2[1][i] = _mm_add_epi32(n1[1][i], n1[3][i]);
        n2[2][i] = _mm_sub_epi32(n1[0][i], n1[2][i]);
        n2[3][i] = _mm_sub_epi32(n1[1][i], n1[3][i]);

        n1[0][i] = _mm_abs_epi32(_mm_add_epi32(n2[0][i], n2[1][i]));
        n1[1][i] = _mm_abs_epi32(_mm_sub_epi32(n2[0][i], n2[1][i]));
        n1[2][i] = _mm_abs_epi32(_mm_add_epi32(n2[2][i], n2[3][i]));
        n1[3][i] = _mm_abs_epi32(_mm_sub_epi32(n2[2][i], n2[3][i]));
    }

    s32 *p = (s32 *)&n1[0][0];
    p[0]   = p[0] >> 2;

    for(i = 0; i < 4; i++) {
        m1[i] = _mm_add_epi32(n1[i][0], n1[i][1]);
    }

    m1[0] = _mm_add_epi32(m1[0], m1[1]);
    m1[2] = _mm_add_epi32(m1[2], m1[3]);

    sum = _mm_add_epi32(m1[0], m1[2]);

    sum = _mm_hadd_epi32(sum, sum);
    sum = _mm_hadd_epi32(sum, sum);

    satd = _mm_cvtsi128_si32(sum);
    satd = (int)(satd / sqrt(4.0 * 8) * 2);

    return satd;
}

int xeve_had_sse(int w, int h, void *o, void *c, int s_org, int s_cur, int bit_depth)
{
    pel *org = o;
    pel *cur = c;
    int  x, y;
    int  sum  = 0;
    int  step = 1;

    if(w > h && (h & 7) == 0 && (w & 15) == 0) {
        int offset_org = s_org << 3;
        int offset_cur = s_cur << 3;

        for(y = 0; y < h; y += 8) {
            for(x = 0; x < w; x += 16) {
                sum += xeve_had_16x8_sse(&org[x], &cur[x], s_org, s_cur, step, bit_depth);
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
                sum += xeve_had_8x16_sse(&org[x], &cur[x], s_org, s_cur, step, bit_depth);
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
                sum += xeve_had_8x4_sse(&org[x], &cur[x], s_org, s_cur, step, bit_depth);
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
                sum += xeve_had_4x8_sse(&org[x], &cur[x], s_org, s_cur, step, bit_depth);
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
                sum += xeve_had_8x8_sse(&org[x], &cur[x * step], s_org, s_cur, step, bit_depth);
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
                sum += xeve_had_4x4_sse(&org[x], &cur[x * step], s_org, s_cur, step, bit_depth);
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

const XEVE_FN_SATD xeve_tbl_satd_16b_sse[1] = {
    xeve_had_sse,
};

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

#include "xeve_sad_avx.h"

#if X86_SSE
static int sad_16b_avx_16nx2n(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int bit_depth)
{
    __m256i src_16x16b;
    __m256i src_16x16b_1;

    __m256i pred_16x16b;
    __m256i pred_16x16b_1;

    __m256i temp;
    __m256i temp_1;
    __m256i temp_3;

    __m256i temp_dummy;
    __m256i result;

    short *pu2_inp, *pu2_inp2;
    short *pu2_ref, *pu2_ref2;

    int  i, j;
    int sad = 0;
    int s_src1_t2 = s_src1 * 2;
    int s_src2_t2 = s_src2 * 2;
    assert(bit_depth <= 14);
    assert(!(w & 15)); /*fun used only for multiple of 16, but internal assumption is only 8 */

    pu2_inp = src1;
    pu2_ref = src2;
    pu2_inp2 = (short*)src1 + s_src1;
    pu2_ref2 = (short*)src2 + s_src2;

    temp_dummy = _mm256_setzero_si256();
    result = _mm256_setzero_si256();


    for (i = 0; i < h >> 1; i++)
    {
        int count = 0;

        for (j = 0; j < w; j += 16)
        {
            src_16x16b = _mm256_loadu_si256((__m256i *) (&pu2_inp[j]));
            src_16x16b_1 = _mm256_loadu_si256((__m256i *) (&pu2_inp2[j]));

            pred_16x16b = _mm256_loadu_si256((__m256i *) (&pu2_ref[j]));
            pred_16x16b_1 = _mm256_loadu_si256((__m256i *) (&pu2_ref2[j]));

            temp = _mm256_sub_epi16(src_16x16b, pred_16x16b);
            temp_1 = _mm256_sub_epi16(src_16x16b_1, pred_16x16b_1);

            temp = _mm256_abs_epi16(temp);
            temp_1 = _mm256_abs_epi16(temp_1);

            temp = _mm256_add_epi16(temp, temp_1);

            temp_1 = _mm256_unpackhi_epi16(temp, temp_dummy);
            temp_3 = _mm256_unpacklo_epi16(temp, temp_dummy);

            temp = _mm256_add_epi32(temp_1, temp_3);
            result = _mm256_add_epi32(result, temp);
        }

        pu2_inp += s_src1_t2;
        pu2_ref += s_src2_t2;
        pu2_inp2 += s_src1_t2;
        pu2_ref2 += s_src2_t2;
    }
    result = _mm256_hadd_epi32(result, result);
    result = _mm256_hadd_epi32(result, result);
    int *val = (int*)&result;
    sad = val[0] + val[4];

    return (sad >> (bit_depth - 8));
}

// clang-format off

/* index: [log2 of width][log2 of height] */
const XEVE_FN_SAD xeve_tbl_sad_16b_avx[8][8] =
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
        sad_16b_avx_16nx2n,  /* height == 2 */
        sad_16b_avx_16nx2n,  /* height == 4 */
        sad_16b_avx_16nx2n,  /* height == 8 */
        sad_16b_avx_16nx2n,  /* height == 16 */
        sad_16b_avx_16nx2n,  /* height == 32 */
        sad_16b_avx_16nx2n,  /* height == 64 */
        sad_16b_avx_16nx2n,  /* height == 128 */
    },
    /* width == 32 */
    {
        sad_16b_sse_16nx1n,  /* height == 1 */
        sad_16b_avx_16nx2n,  /* height == 2 */
        sad_16b_avx_16nx2n,  /* height == 4 */
        sad_16b_avx_16nx2n,  /* height == 8 */
        sad_16b_avx_16nx2n,  /* height == 16 */
        sad_16b_avx_16nx2n,  /* height == 32 */
        sad_16b_avx_16nx2n,  /* height == 64 */
        sad_16b_avx_16nx2n,  /* height == 128 */
    },
    /* width == 64 */
    {
        sad_16b_sse_16nx1n,  /* height == 1 */
        sad_16b_avx_16nx2n,  /* height == 2 */
        sad_16b_avx_16nx2n,  /* height == 4 */
        sad_16b_avx_16nx2n,  /* height == 8 */
        sad_16b_avx_16nx2n,  /* height == 16 */
        sad_16b_avx_16nx2n,  /* height == 32 */
        sad_16b_avx_16nx2n,  /* height == 64 */
        sad_16b_avx_16nx2n,  /* height == 128 */
    },
    /* width == 128 */
    {
        sad_16b_sse_16nx1n,  /* height == 1 */
        sad_16b_avx_16nx2n,  /* height == 2 */
        sad_16b_avx_16nx2n,  /* height == 4 */
        sad_16b_avx_16nx2n,  /* height == 8 */
        sad_16b_avx_16nx2n,  /* height == 16 */
        sad_16b_avx_16nx2n,  /* height == 32 */
        sad_16b_avx_16nx2n,  /* height == 64 */
        sad_16b_avx_16nx2n,  /* height == 128 */
    }
};
// clang-format on

#endif
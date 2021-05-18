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

#include "xeve_mc_avx.h"

#if X86_SSE

static void mc_filter_l_8pel_horz_clip_avx(s16 *ref, int src_stride, s16 *pred, int dst_stride, const s16 *coeff
                                       , int width, int height, int min_val, int max_val, int offset, int shift)
{
    int row, col, rem_w;
    s16 const *src_tmp;
    s16 const *inp_copy;
    s16 *dst_copy;

    src_tmp = ref;
    rem_w = width;
    inp_copy = src_tmp;
    dst_copy = pred;

    if (rem_w > 15)
    {
        __m256i offset_8x16b = _mm256_set1_epi32(offset);
        __m256i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b, src_temp5_16x8b, src_temp6_16x8b;
        __m256i src_temp7_16x8b, src_temp8_16x8b, src_temp9_16x8b, src_temp0_16x8b;
        __m256i res_temp1_8x16b, res_temp2_8x16b, res_temp3_8x16b, res_temp4_8x16b, res_temp5_8x16b, res_temp6_8x16b, res_temp7_8x16b, res_temp8_8x16b;
        __m256i res_temp9_8x16b, res_temp0_8x16b;
        __m256i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b, coeff6_7_8x16b;
        __m256i mm_min = _mm256_set1_epi16(min_val);
        __m256i mm_max = _mm256_set1_epi16(max_val);
        short tmp_buf[16] = { 0 };
        xeve_mcpy(tmp_buf, coeff, 16);
        xeve_mcpy(tmp_buf + 8, coeff, 16);
        coeff0_1_8x16b = _mm256_loadu_si256((__m256i*)(tmp_buf));

        coeff2_3_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff4_5_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0xaa);
        coeff6_7_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0xff);
        coeff0_1_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0);

        for (row = 0; row < height; row += 1)
        {
            for (col = 0; col < width; col += 16)
            {
                /*load 8 pixel values from row 0*/
                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 1]));

                src_temp3_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp7_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp1_8x16b = _mm256_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);
                res_temp7_8x16b = _mm256_madd_epi16(src_temp7_16x8b, coeff0_1_8x16b);
                /* row = 0 */
                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 2]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 3]));

                src_temp4_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp8_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp2_8x16b = _mm256_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);
                res_temp8_8x16b = _mm256_madd_epi16(src_temp8_16x8b, coeff2_3_8x16b);

                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 4]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 5]));

                src_temp5_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp9_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp3_8x16b = _mm256_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);
                res_temp9_8x16b = _mm256_madd_epi16(src_temp9_16x8b, coeff4_5_8x16b);

                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 6]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 7]));

                src_temp6_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp0_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp4_8x16b = _mm256_madd_epi16(src_temp6_16x8b, coeff6_7_8x16b);
                res_temp0_8x16b = _mm256_madd_epi16(src_temp0_16x8b, coeff6_7_8x16b);

                res_temp5_8x16b = _mm256_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
                res_temp6_8x16b = _mm256_add_epi32(res_temp3_8x16b, res_temp4_8x16b);
                res_temp5_8x16b = _mm256_add_epi32(res_temp5_8x16b, res_temp6_8x16b);

                res_temp6_8x16b = _mm256_add_epi32(res_temp7_8x16b, res_temp8_8x16b);
                res_temp7_8x16b = _mm256_add_epi32(res_temp9_8x16b, res_temp0_8x16b);
                res_temp8_8x16b = _mm256_add_epi32(res_temp6_8x16b, res_temp7_8x16b);

                res_temp6_8x16b = _mm256_add_epi32(res_temp5_8x16b, offset_8x16b);
                res_temp7_8x16b = _mm256_add_epi32(res_temp8_8x16b, offset_8x16b);
                res_temp6_8x16b = _mm256_srai_epi32(res_temp6_8x16b, shift);
                res_temp7_8x16b = _mm256_srai_epi32(res_temp7_8x16b, shift);
                res_temp5_8x16b = _mm256_packs_epi32(res_temp6_8x16b, res_temp7_8x16b);
                
                res_temp5_8x16b = _mm256_min_epi16(res_temp5_8x16b, mm_max);
                res_temp5_8x16b = _mm256_max_epi16(res_temp5_8x16b, mm_min);

                /* to store the 8 pixels res. */
                _mm256_storeu_si256((__m256i *)(dst_copy + col), res_temp5_8x16b);
            }

            inp_copy += src_stride; /* pointer updates*/
            dst_copy += dst_stride; /* pointer updates*/
        }
    }
    else if (rem_w > 7)
    {
        __m128i offset_8x16b = _mm_set1_epi32(offset);
        __m128i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b, src_temp5_16x8b, src_temp6_16x8b;
        __m128i src_temp7_16x8b, src_temp8_16x8b, src_temp9_16x8b, src_temp0_16x8b;
        __m128i res_temp1_8x16b, res_temp2_8x16b, res_temp3_8x16b, res_temp4_8x16b, res_temp5_8x16b, res_temp6_8x16b, res_temp7_8x16b, res_temp8_8x16b;
        __m128i res_temp9_8x16b, res_temp0_8x16b;
        __m128i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b, coeff6_7_8x16b;
        __m128i mm_min = _mm_set1_epi16(min_val);
        __m128i mm_max = _mm_set1_epi16(max_val);
        /* load 8 8-bit coefficients and convert 8-bit into 16-bit  */
        coeff0_1_8x16b = _mm_loadu_si128((__m128i*)coeff);

        coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff4_5_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xaa);
        coeff6_7_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xff);
        coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);

        for (row = 0; row < height; row += 1)
        {
            /*load 8 pixel values from row 0*/
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[0]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[1]));

            src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp7_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);
            res_temp7_8x16b = _mm_madd_epi16(src_temp7_16x8b, coeff0_1_8x16b);
            /* row = 0 */
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[2]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[3]));

            src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp8_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);
            res_temp8_8x16b = _mm_madd_epi16(src_temp8_16x8b, coeff2_3_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[4]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[5]));

            src_temp5_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp9_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp3_8x16b = _mm_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);
            res_temp9_8x16b = _mm_madd_epi16(src_temp9_16x8b, coeff4_5_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[6]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[7]));

            src_temp6_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp0_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp4_8x16b = _mm_madd_epi16(src_temp6_16x8b, coeff6_7_8x16b);
            res_temp0_8x16b = _mm_madd_epi16(src_temp0_16x8b, coeff6_7_8x16b);

            res_temp5_8x16b = _mm_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
            res_temp6_8x16b = _mm_add_epi32(res_temp3_8x16b, res_temp4_8x16b);
            res_temp5_8x16b = _mm_add_epi32(res_temp5_8x16b, res_temp6_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp7_8x16b, res_temp8_8x16b);
            res_temp7_8x16b = _mm_add_epi32(res_temp9_8x16b, res_temp0_8x16b);
            res_temp8_8x16b = _mm_add_epi32(res_temp6_8x16b, res_temp7_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp5_8x16b, offset_8x16b);
            res_temp7_8x16b = _mm_add_epi32(res_temp8_8x16b, offset_8x16b);
            res_temp6_8x16b = _mm_srai_epi32(res_temp6_8x16b, shift);
            res_temp7_8x16b = _mm_srai_epi32(res_temp7_8x16b, shift);
            res_temp5_8x16b = _mm_packs_epi32(res_temp6_8x16b, res_temp7_8x16b);

            res_temp5_8x16b = _mm_min_epi16(res_temp5_8x16b, mm_max);
            res_temp5_8x16b = _mm_max_epi16(res_temp5_8x16b, mm_min);

            /* to store the 8 pixels res. */
            _mm_storeu_si128((__m128i *)(dst_copy), res_temp5_8x16b);

            inp_copy += src_stride; /* pointer updates*/
            dst_copy += dst_stride; /* pointer updates*/
        }
    }
    else if (rem_w > 3)
    {
        __m128i offset_8x16b = _mm_set1_epi32(offset);
        __m128i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b, src_temp5_16x8b, src_temp6_16x8b;
        __m128i res_temp1_8x16b, res_temp2_8x16b, res_temp3_8x16b, res_temp4_8x16b, res_temp5_8x16b, res_temp6_8x16b;
        __m128i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b, coeff6_7_8x16b;
        __m128i mm_min = _mm_set1_epi16(min_val);
        __m128i mm_max = _mm_set1_epi16(max_val);
        coeff0_1_8x16b = _mm_loadu_si128((__m128i*)coeff);
        coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff4_5_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xaa);
        coeff6_7_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xff);
        coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);

        inp_copy = src_tmp + ((width / 8) * 8);
        dst_copy = pred + ((width / 8) * 8);

        for (row = 0; row < height; row += 1)
        {
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[0]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[1]));

            src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[2]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[3]));

            src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[4]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[5]));

            src_temp5_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp3_8x16b = _mm_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[6]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[7]));

            src_temp6_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp4_8x16b = _mm_madd_epi16(src_temp6_16x8b, coeff6_7_8x16b);

            res_temp5_8x16b = _mm_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
            res_temp6_8x16b = _mm_add_epi32(res_temp3_8x16b, res_temp4_8x16b);
            res_temp5_8x16b = _mm_add_epi32(res_temp5_8x16b, res_temp6_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp5_8x16b, offset_8x16b);
            res_temp6_8x16b = _mm_srai_epi32(res_temp6_8x16b, shift);
            res_temp5_8x16b = _mm_packs_epi32(res_temp6_8x16b, res_temp6_8x16b);

            res_temp5_8x16b = _mm_min_epi16(res_temp5_8x16b, mm_max);
            res_temp5_8x16b = _mm_max_epi16(res_temp5_8x16b, mm_min);

            /* to store the 1st 4 pixels res. */
            _mm_storel_epi64((__m128i *)(dst_copy), res_temp5_8x16b);
            inp_copy += src_stride;
            dst_copy += dst_stride;
        }
    }
}

void xeve_mc_filter_l_6pel_horz_clip_avx(s16 *ref, int src_stride, s16 *pred, int dst_stride, const s16 *coeff
                                       , int width, int height, int min_val, int max_val, int offset, int shift)
{
    int row, col, rem_w;
    s16 const *src_tmp;
    s16 const *inp_copy;
    s16 *dst_copy;

    src_tmp = ref;
    rem_w = width;
    inp_copy = src_tmp;
    dst_copy = pred;

    if (rem_w > 15)
    {
        __m256i offset_8x16b = _mm256_set1_epi32(offset);
        __m256i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b, src_temp5_16x8b;
        __m256i src_temp7_16x8b, src_temp8_16x8b, src_temp9_16x8b;
        __m256i res_temp1_8x16b, res_temp2_8x16b, res_temp3_8x16b, res_temp5_8x16b, res_temp6_8x16b, res_temp7_8x16b, res_temp8_8x16b;
        __m256i res_temp9_8x16b;
        __m256i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b, coeff6_7_8x16b;
        __m256i mm_min = _mm256_set1_epi16(min_val);
        __m256i mm_max = _mm256_set1_epi16(max_val);
        short tmp_buf[16] = { 0 };
        xeve_mcpy(tmp_buf, coeff + 1, 12);
        xeve_mcpy(tmp_buf + 8, coeff + 1, 12);
        coeff0_1_8x16b = _mm256_loadu_si256((__m256i*)(tmp_buf));

        coeff2_3_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff4_5_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0xaa);
        coeff6_7_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0xff);
        coeff0_1_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0);

        for (row = 0; row < height; row += 1)
        {
            for (col = 0; col < width; col += 16)
            {
                /*load 8 pixel values from row 0*/
                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 1]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 2]));

                src_temp3_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp7_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp1_8x16b = _mm256_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);
                res_temp7_8x16b = _mm256_madd_epi16(src_temp7_16x8b, coeff0_1_8x16b);
                /* row = 0 */
                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 3]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 4]));

                src_temp4_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp8_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp2_8x16b = _mm256_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);
                res_temp8_8x16b = _mm256_madd_epi16(src_temp8_16x8b, coeff2_3_8x16b);

                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 5]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 6]));

                src_temp5_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp9_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp3_8x16b = _mm256_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);
                res_temp9_8x16b = _mm256_madd_epi16(src_temp9_16x8b, coeff4_5_8x16b);

                res_temp5_8x16b = _mm256_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
                res_temp5_8x16b = _mm256_add_epi32(res_temp5_8x16b, res_temp3_8x16b);

                res_temp6_8x16b = _mm256_add_epi32(res_temp7_8x16b, res_temp8_8x16b);
                res_temp8_8x16b = _mm256_add_epi32(res_temp6_8x16b, res_temp9_8x16b);

                res_temp6_8x16b = _mm256_add_epi32(res_temp5_8x16b, offset_8x16b);
                res_temp7_8x16b = _mm256_add_epi32(res_temp8_8x16b, offset_8x16b);
                res_temp6_8x16b = _mm256_srai_epi32(res_temp6_8x16b, shift);
                res_temp7_8x16b = _mm256_srai_epi32(res_temp7_8x16b, shift);
                res_temp5_8x16b = _mm256_packs_epi32(res_temp6_8x16b, res_temp7_8x16b);
                
                res_temp5_8x16b = _mm256_min_epi16(res_temp5_8x16b, mm_max);
                res_temp5_8x16b = _mm256_max_epi16(res_temp5_8x16b, mm_min);

                /* to store the 8 pixels res. */
                _mm256_storeu_si256((__m256i *)(dst_copy + col), res_temp5_8x16b);
            }

            inp_copy += src_stride; /* pointer updates*/
            dst_copy += dst_stride; /* pointer updates*/
        }
    }
    else if (rem_w > 7)
    {
        __m128i offset_8x16b = _mm_set1_epi32(offset);
        __m128i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b, src_temp5_16x8b;
        __m128i src_temp7_16x8b, src_temp8_16x8b, src_temp9_16x8b;
        __m128i res_temp1_8x16b, res_temp2_8x16b, res_temp3_8x16b, res_temp5_8x16b, res_temp6_8x16b, res_temp7_8x16b, res_temp8_8x16b;
        __m128i res_temp9_8x16b;
        __m128i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b;
        __m128i mm_min = _mm_set1_epi16(min_val);
        __m128i mm_max = _mm_set1_epi16(max_val);
        /* load 8 8-bit coefficients and convert 8-bit into 16-bit  */
        coeff0_1_8x16b = _mm_loadu_si128((__m128i*)(coeff + 1));

        coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff4_5_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xaa);        
        coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);

        for (row = 0; row < height; row += 1)
        {
            /*load 8 pixel values from row 0*/
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[1]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[2]));

            src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp7_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);
            res_temp7_8x16b = _mm_madd_epi16(src_temp7_16x8b, coeff0_1_8x16b);
            /* row = 0 */
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[3]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[4]));

            src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp8_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);
            res_temp8_8x16b = _mm_madd_epi16(src_temp8_16x8b, coeff2_3_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[5]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[6]));

            src_temp5_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp9_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp3_8x16b = _mm_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);
            res_temp9_8x16b = _mm_madd_epi16(src_temp9_16x8b, coeff4_5_8x16b);

            res_temp5_8x16b = _mm_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
            res_temp5_8x16b = _mm_add_epi32(res_temp5_8x16b, res_temp3_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp7_8x16b, res_temp8_8x16b);
            res_temp8_8x16b = _mm_add_epi32(res_temp6_8x16b, res_temp9_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp5_8x16b, offset_8x16b);
            res_temp7_8x16b = _mm_add_epi32(res_temp8_8x16b, offset_8x16b);
            res_temp6_8x16b = _mm_srai_epi32(res_temp6_8x16b, shift);
            res_temp7_8x16b = _mm_srai_epi32(res_temp7_8x16b, shift);
            res_temp5_8x16b = _mm_packs_epi32(res_temp6_8x16b, res_temp7_8x16b);

            res_temp5_8x16b = _mm_min_epi16(res_temp5_8x16b, mm_max);
            res_temp5_8x16b = _mm_max_epi16(res_temp5_8x16b, mm_min);

            /* to store the 8 pixels res. */
            _mm_storeu_si128((__m128i *)(dst_copy), res_temp5_8x16b);

            inp_copy += src_stride; /* pointer updates*/
            dst_copy += dst_stride; /* pointer updates*/
        }
    }
    else if (rem_w > 3)
    {
        __m128i offset_8x16b = _mm_set1_epi32(offset);
        __m128i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b, src_temp5_16x8b;
        __m128i res_temp1_8x16b, res_temp2_8x16b, res_temp3_8x16b, res_temp5_8x16b, res_temp6_8x16b;
        __m128i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b;
        __m128i mm_min = _mm_set1_epi16(min_val);
        __m128i mm_max = _mm_set1_epi16(max_val);
        coeff0_1_8x16b = _mm_loadu_si128((__m128i*)(coeff + 1));
        coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff4_5_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xaa);
        coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);

        inp_copy = src_tmp + ((width / 8) * 8);
        dst_copy = pred + ((width / 8) * 8);

        for (row = 0; row < height; row += 1)
        {
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[1]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[2]));

            src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[3]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[4]));

            src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[5]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[6]));

            src_temp5_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp3_8x16b = _mm_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);

            res_temp5_8x16b = _mm_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
            res_temp5_8x16b = _mm_add_epi32(res_temp5_8x16b, res_temp3_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp5_8x16b, offset_8x16b);
            res_temp6_8x16b = _mm_srai_epi32(res_temp6_8x16b, shift);
            res_temp5_8x16b = _mm_packs_epi32(res_temp6_8x16b, res_temp6_8x16b);

            res_temp5_8x16b = _mm_min_epi16(res_temp5_8x16b, mm_max);
            res_temp5_8x16b = _mm_max_epi16(res_temp5_8x16b, mm_min);

            /* to store the 1st 4 pixels res. */
            _mm_storel_epi64((__m128i *)(dst_copy), res_temp5_8x16b);
            inp_copy += src_stride;
            dst_copy += dst_stride;
        }
    }
}

static void mc_filter_l_8pel_horz_no_clip_avx(s16 *ref,
    int src_stride,
    s16 *pred,
    int dst_stride,
    const s16 *coeff,
    int width,
    int height,
    int offset,
    int shift)
{
    int row, col, rem_w;
    s16 const *src_tmp;
    s16 const *inp_copy;
    s16 *dst_copy;

    /* all 128 bit registers are named with a suffix mxnb, where m is the */
    /* number of n bits packed in the register                            */

    src_tmp = ref;
    rem_w = width;
    inp_copy = src_tmp;
    dst_copy = pred;

    if (rem_w > 15)
    {
        __m256i offset_8x16b = _mm256_set1_epi32(offset);
        __m256i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b, src_temp5_16x8b, src_temp6_16x8b;
        __m256i src_temp7_16x8b, src_temp8_16x8b, src_temp9_16x8b, src_temp0_16x8b;
        __m256i res_temp1_8x16b, res_temp2_8x16b, res_temp3_8x16b, res_temp4_8x16b, res_temp5_8x16b, res_temp6_8x16b, res_temp7_8x16b, res_temp8_8x16b;
        __m256i res_temp9_8x16b, res_temp0_8x16b;
        __m256i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b, coeff6_7_8x16b;
        short tmp_buf[16] = { 0 };
        xeve_mcpy(tmp_buf, coeff, 16);
        xeve_mcpy(tmp_buf + 8, coeff, 16);
        coeff0_1_8x16b = _mm256_loadu_si256((__m256i*)(tmp_buf));

        coeff2_3_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff4_5_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0xaa);
        coeff6_7_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0xff);
        coeff0_1_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0);

        for (row = 0; row < height; row += 1)
        {
            for (col = 0; col < width; col += 16)
            {
                /*load 8 pixel values from row 0*/
                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 1]));

                src_temp3_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp7_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp1_8x16b = _mm256_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);
                res_temp7_8x16b = _mm256_madd_epi16(src_temp7_16x8b, coeff0_1_8x16b);
                /* row = 0 */
                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 2]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 3]));

                src_temp4_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp8_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp2_8x16b = _mm256_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);
                res_temp8_8x16b = _mm256_madd_epi16(src_temp8_16x8b, coeff2_3_8x16b);

                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 4]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 5]));

                src_temp5_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp9_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp3_8x16b = _mm256_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);
                res_temp9_8x16b = _mm256_madd_epi16(src_temp9_16x8b, coeff4_5_8x16b);

                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 6]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 7]));

                src_temp6_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp0_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp4_8x16b = _mm256_madd_epi16(src_temp6_16x8b, coeff6_7_8x16b);
                res_temp0_8x16b = _mm256_madd_epi16(src_temp0_16x8b, coeff6_7_8x16b);

                res_temp5_8x16b = _mm256_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
                res_temp6_8x16b = _mm256_add_epi32(res_temp3_8x16b, res_temp4_8x16b);
                res_temp5_8x16b = _mm256_add_epi32(res_temp5_8x16b, res_temp6_8x16b);

                res_temp6_8x16b = _mm256_add_epi32(res_temp7_8x16b, res_temp8_8x16b);
                res_temp7_8x16b = _mm256_add_epi32(res_temp9_8x16b, res_temp0_8x16b);
                res_temp8_8x16b = _mm256_add_epi32(res_temp6_8x16b, res_temp7_8x16b);

                res_temp6_8x16b = _mm256_add_epi32(res_temp5_8x16b, offset_8x16b);
                res_temp7_8x16b = _mm256_add_epi32(res_temp8_8x16b, offset_8x16b);
                res_temp6_8x16b = _mm256_srai_epi32(res_temp6_8x16b, shift);
                res_temp7_8x16b = _mm256_srai_epi32(res_temp7_8x16b, shift);
                res_temp5_8x16b = _mm256_packs_epi32(res_temp6_8x16b, res_temp7_8x16b);

                /* to store the 8 pixels res. */
                _mm256_storeu_si256((__m256i *)(dst_copy + col), res_temp5_8x16b);
            }

            inp_copy += src_stride; /* pointer updates*/
            dst_copy += dst_stride; /* pointer updates*/
        }
    }
    else if (rem_w > 7)
    {
        __m128i offset_8x16b = _mm_set1_epi32(offset);
        __m128i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b, src_temp5_16x8b, src_temp6_16x8b;
        __m128i src_temp7_16x8b, src_temp8_16x8b, src_temp9_16x8b, src_temp0_16x8b;
        __m128i res_temp1_8x16b, res_temp2_8x16b, res_temp3_8x16b, res_temp4_8x16b, res_temp5_8x16b, res_temp6_8x16b, res_temp7_8x16b, res_temp8_8x16b;
        __m128i res_temp9_8x16b, res_temp0_8x16b;
        __m128i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b, coeff6_7_8x16b;

        /* load 8 8-bit coefficients and convert 8-bit into 16-bit  */
        coeff0_1_8x16b = _mm_loadu_si128((__m128i*)coeff);

        coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff4_5_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xaa);
        coeff6_7_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xff);
        coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);

        for (row = 0; row < height; row += 1)
        {
            /*load 8 pixel values from row 0*/
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[0]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[1]));

            src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp7_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);
            res_temp7_8x16b = _mm_madd_epi16(src_temp7_16x8b, coeff0_1_8x16b);
            /* row = 0 */
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[2]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[3]));

            src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp8_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);
            res_temp8_8x16b = _mm_madd_epi16(src_temp8_16x8b, coeff2_3_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[4]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[5]));

            src_temp5_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp9_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp3_8x16b = _mm_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);
            res_temp9_8x16b = _mm_madd_epi16(src_temp9_16x8b, coeff4_5_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[6]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[7]));

            src_temp6_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp0_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp4_8x16b = _mm_madd_epi16(src_temp6_16x8b, coeff6_7_8x16b);
            res_temp0_8x16b = _mm_madd_epi16(src_temp0_16x8b, coeff6_7_8x16b);

            res_temp5_8x16b = _mm_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
            res_temp6_8x16b = _mm_add_epi32(res_temp3_8x16b, res_temp4_8x16b);
            res_temp5_8x16b = _mm_add_epi32(res_temp5_8x16b, res_temp6_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp7_8x16b, res_temp8_8x16b);
            res_temp7_8x16b = _mm_add_epi32(res_temp9_8x16b, res_temp0_8x16b);
            res_temp8_8x16b = _mm_add_epi32(res_temp6_8x16b, res_temp7_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp5_8x16b, offset_8x16b);
            res_temp7_8x16b = _mm_add_epi32(res_temp8_8x16b, offset_8x16b);
            res_temp6_8x16b = _mm_srai_epi32(res_temp6_8x16b, shift);
            res_temp7_8x16b = _mm_srai_epi32(res_temp7_8x16b, shift);
            res_temp5_8x16b = _mm_packs_epi32(res_temp6_8x16b, res_temp7_8x16b);

            /* to store the 8 pixels res. */
            _mm_storeu_si128((__m128i *)(dst_copy), res_temp5_8x16b);

            inp_copy += src_stride; /* pointer updates*/
            dst_copy += dst_stride; /* pointer updates*/
        }
    }
    else if (rem_w > 3)
    {
        __m128i offset_8x16b = _mm_set1_epi32(offset);
        __m128i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b, src_temp5_16x8b, src_temp6_16x8b;
        __m128i res_temp1_8x16b, res_temp2_8x16b, res_temp3_8x16b, res_temp4_8x16b, res_temp5_8x16b, res_temp6_8x16b;
        __m128i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b, coeff6_7_8x16b;
        coeff0_1_8x16b = _mm_loadu_si128((__m128i*)coeff);
        coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff4_5_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xaa);
        coeff6_7_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xff);
        coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);

        inp_copy = src_tmp + ((width / 8) * 8);
        dst_copy = pred + ((width / 8) * 8);

        for (row = 0; row < height; row += 1)
        {
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[0]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[1]));

            src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[2]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[3]));

            src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[4]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[5]));

            src_temp5_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp3_8x16b = _mm_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[6]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[7]));

            src_temp6_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp4_8x16b = _mm_madd_epi16(src_temp6_16x8b, coeff6_7_8x16b);

            res_temp5_8x16b = _mm_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
            res_temp6_8x16b = _mm_add_epi32(res_temp3_8x16b, res_temp4_8x16b);
            res_temp5_8x16b = _mm_add_epi32(res_temp5_8x16b, res_temp6_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp5_8x16b, offset_8x16b);
            res_temp6_8x16b = _mm_srai_epi32(res_temp6_8x16b, shift);
            res_temp5_8x16b = _mm_packs_epi32(res_temp6_8x16b, res_temp6_8x16b);

            /* to store the 1st 4 pixels res. */
            _mm_storel_epi64((__m128i *)(dst_copy), res_temp5_8x16b);
            inp_copy += src_stride;
            dst_copy += dst_stride;
        }
    }
}

static void mc_filter_l_6pel_horz_no_clip_avx(s16 *ref,
    int src_stride,
    s16 *pred,
    int dst_stride,
    const s16 *coeff,
    int width,
    int height,
    int offset,
    int shift)
{
    int row, col, rem_w;
    s16 const *src_tmp;
    s16 const *inp_copy;
    s16 *dst_copy;

    src_tmp = ref;
    rem_w = width;
    inp_copy = src_tmp;
    dst_copy = pred;

    if (rem_w > 15)
    {
        __m256i offset_8x16b = _mm256_set1_epi32(offset);
        __m256i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b, src_temp5_16x8b;
        __m256i src_temp7_16x8b, src_temp8_16x8b, src_temp9_16x8b;
        __m256i res_temp1_8x16b, res_temp2_8x16b, res_temp3_8x16b, res_temp5_8x16b, res_temp6_8x16b, res_temp7_8x16b, res_temp8_8x16b;
        __m256i res_temp9_8x16b;
        __m256i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b, coeff6_7_8x16b;
        short tmp_buf[16] = { 0 };
        xeve_mcpy(tmp_buf, coeff + 1, 12);
        xeve_mcpy(tmp_buf + 8, coeff + 1, 12);
        coeff0_1_8x16b = _mm256_loadu_si256((__m256i*)(tmp_buf));

        coeff2_3_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff4_5_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0xaa);
        coeff6_7_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0xff);
        coeff0_1_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0);

        for (row = 0; row < height; row += 1)
        {
            for (col = 0; col < width; col += 16)
            {
                /*load 8 pixel values from row 0*/
                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 1]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 2]));

                src_temp3_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp7_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp1_8x16b = _mm256_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);
                res_temp7_8x16b = _mm256_madd_epi16(src_temp7_16x8b, coeff0_1_8x16b);
                /* row = 0 */
                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 3]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 4]));

                src_temp4_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp8_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp2_8x16b = _mm256_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);
                res_temp8_8x16b = _mm256_madd_epi16(src_temp8_16x8b, coeff2_3_8x16b);

                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 5]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 6]));

                src_temp5_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp9_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp3_8x16b = _mm256_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);
                res_temp9_8x16b = _mm256_madd_epi16(src_temp9_16x8b, coeff4_5_8x16b);

                res_temp5_8x16b = _mm256_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
                res_temp5_8x16b = _mm256_add_epi32(res_temp5_8x16b, res_temp3_8x16b);

                res_temp6_8x16b = _mm256_add_epi32(res_temp7_8x16b, res_temp8_8x16b);
                res_temp8_8x16b = _mm256_add_epi32(res_temp6_8x16b, res_temp9_8x16b);

                res_temp6_8x16b = _mm256_add_epi32(res_temp5_8x16b, offset_8x16b);
                res_temp7_8x16b = _mm256_add_epi32(res_temp8_8x16b, offset_8x16b);
                res_temp6_8x16b = _mm256_srai_epi32(res_temp6_8x16b, shift);
                res_temp7_8x16b = _mm256_srai_epi32(res_temp7_8x16b, shift);
                res_temp5_8x16b = _mm256_packs_epi32(res_temp6_8x16b, res_temp7_8x16b);

                /* to store the 8 pixels res. */
                _mm256_storeu_si256((__m256i *)(dst_copy + col), res_temp5_8x16b);
            }

            inp_copy += src_stride; /* pointer updates*/
            dst_copy += dst_stride; /* pointer updates*/
        }
    }
    else if (rem_w > 7)
    {
        __m128i offset_8x16b = _mm_set1_epi32(offset);
        __m128i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b, src_temp5_16x8b;
        __m128i src_temp7_16x8b, src_temp8_16x8b, src_temp9_16x8b;
        __m128i res_temp1_8x16b, res_temp2_8x16b, res_temp3_8x16b, res_temp5_8x16b, res_temp6_8x16b, res_temp7_8x16b, res_temp8_8x16b;
        __m128i res_temp9_8x16b;
        __m128i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b;
        /* load 8 8-bit coefficients and convert 8-bit into 16-bit  */
        coeff0_1_8x16b = _mm_loadu_si128((__m128i*)(coeff + 1));

        coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff4_5_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xaa);
        coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);

        for (row = 0; row < height; row += 1)
        {
            /*load 8 pixel values from row 0*/
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[1]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[2]));

            src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp7_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);
            res_temp7_8x16b = _mm_madd_epi16(src_temp7_16x8b, coeff0_1_8x16b);
            /* row = 0 */
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[3]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[4]));

            src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp8_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);
            res_temp8_8x16b = _mm_madd_epi16(src_temp8_16x8b, coeff2_3_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[5]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[6]));

            src_temp5_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp9_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp3_8x16b = _mm_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);
            res_temp9_8x16b = _mm_madd_epi16(src_temp9_16x8b, coeff4_5_8x16b);

            res_temp5_8x16b = _mm_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
            res_temp5_8x16b = _mm_add_epi32(res_temp5_8x16b, res_temp3_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp7_8x16b, res_temp8_8x16b);
            res_temp8_8x16b = _mm_add_epi32(res_temp6_8x16b, res_temp9_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp5_8x16b, offset_8x16b);
            res_temp7_8x16b = _mm_add_epi32(res_temp8_8x16b, offset_8x16b);
            res_temp6_8x16b = _mm_srai_epi32(res_temp6_8x16b, shift);
            res_temp7_8x16b = _mm_srai_epi32(res_temp7_8x16b, shift);
            res_temp5_8x16b = _mm_packs_epi32(res_temp6_8x16b, res_temp7_8x16b);

            /* to store the 8 pixels res. */
            _mm_storeu_si128((__m128i *)(dst_copy), res_temp5_8x16b);

            inp_copy += src_stride; /* pointer updates*/
            dst_copy += dst_stride; /* pointer updates*/
        }
    }
    else if (rem_w > 3)
    {
        __m128i offset_8x16b = _mm_set1_epi32(offset);
        __m128i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b, src_temp5_16x8b;
        __m128i res_temp1_8x16b, res_temp2_8x16b, res_temp3_8x16b, res_temp5_8x16b, res_temp6_8x16b;
        __m128i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b;
        coeff0_1_8x16b = _mm_loadu_si128((__m128i*)(coeff + 1));
        coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff4_5_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xaa);
        coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);

        inp_copy = src_tmp + ((width / 8) * 8);
        dst_copy = pred + ((width / 8) * 8);

        for (row = 0; row < height; row += 1)
        {
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[1]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[2]));

            src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[3]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[4]));

            src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[5]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[6]));

            src_temp5_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp3_8x16b = _mm_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);

            res_temp5_8x16b = _mm_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
            res_temp5_8x16b = _mm_add_epi32(res_temp5_8x16b, res_temp3_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp5_8x16b, offset_8x16b);
            res_temp6_8x16b = _mm_srai_epi32(res_temp6_8x16b, shift);
            res_temp5_8x16b = _mm_packs_epi32(res_temp6_8x16b, res_temp6_8x16b);

            /* to store the 1st 4 pixels res. */
            _mm_storel_epi64((__m128i *)(dst_copy), res_temp5_8x16b);
            inp_copy += src_stride;
            dst_copy += dst_stride;
        }
    }
}

static void mc_filter_l_8pel_vert_clip_avx(s16 *ref,
    int src_stride,
    s16 *pred,
    int dst_stride,
    const s16 *coeff,
    int width,
    int height,
    int min_val,
    int max_val,
    int offset,
    int shift)
{
    int row, col, rem_w;
    s16 const *src_tmp;
    s16 const *inp_copy;
    s16 *dst_copy;

    src_tmp = ref;
    rem_w = width;
    inp_copy = src_tmp;
    dst_copy = pred;

    if (rem_w > 15)
    {
        __m256i offset_8x16b = _mm256_set1_epi32(offset);
        __m256i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b, src_temp5_16x8b, src_temp6_16x8b;
        __m256i src_temp7_16x8b, src_temp8_16x8b, src_temp9_16x8b, src_temp0_16x8b;
        __m256i res_temp1_8x16b, res_temp2_8x16b, res_temp3_8x16b, res_temp4_8x16b, res_temp5_8x16b, res_temp6_8x16b, res_temp7_8x16b, res_temp8_8x16b;
        __m256i res_temp9_8x16b, res_temp0_8x16b;
        __m256i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b, coeff6_7_8x16b;
        __m256i mm_min = _mm256_set1_epi16(min_val);
        __m256i mm_max = _mm256_set1_epi16(max_val);
        short tmp_buf[16] = { 0 };
        xeve_mcpy(tmp_buf, coeff, 16);
        xeve_mcpy(tmp_buf + 8, coeff, 16);
        coeff0_1_8x16b = _mm256_loadu_si256((__m256i*)(tmp_buf));

        coeff2_3_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff4_5_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0xaa);
        coeff6_7_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0xff);
        coeff0_1_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0);

        for (row = 0; row < height; row += 1)
        {
            for (col = 0; col < width; col += 16)
            {
                /*load 8 pixel values from row 0*/
                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + src_stride]));

                src_temp3_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp7_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp1_8x16b = _mm256_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);
                res_temp7_8x16b = _mm256_madd_epi16(src_temp7_16x8b, coeff0_1_8x16b);
                /* row = 0 */
                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + src_stride * 2]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + src_stride * 3]));

                src_temp4_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp8_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp2_8x16b = _mm256_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);
                res_temp8_8x16b = _mm256_madd_epi16(src_temp8_16x8b, coeff2_3_8x16b);

                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + src_stride * 4]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + src_stride * 5]));

                src_temp5_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp9_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp3_8x16b = _mm256_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);
                res_temp9_8x16b = _mm256_madd_epi16(src_temp9_16x8b, coeff4_5_8x16b);

                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + src_stride * 6]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + src_stride * 7]));

                src_temp6_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp0_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp4_8x16b = _mm256_madd_epi16(src_temp6_16x8b, coeff6_7_8x16b);
                res_temp0_8x16b = _mm256_madd_epi16(src_temp0_16x8b, coeff6_7_8x16b);

                res_temp5_8x16b = _mm256_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
                res_temp6_8x16b = _mm256_add_epi32(res_temp3_8x16b, res_temp4_8x16b);
                res_temp5_8x16b = _mm256_add_epi32(res_temp5_8x16b, res_temp6_8x16b);

                res_temp6_8x16b = _mm256_add_epi32(res_temp7_8x16b, res_temp8_8x16b);
                res_temp7_8x16b = _mm256_add_epi32(res_temp9_8x16b, res_temp0_8x16b);
                res_temp8_8x16b = _mm256_add_epi32(res_temp6_8x16b, res_temp7_8x16b);

                res_temp6_8x16b = _mm256_add_epi32(res_temp5_8x16b, offset_8x16b);
                res_temp7_8x16b = _mm256_add_epi32(res_temp8_8x16b, offset_8x16b);
                res_temp6_8x16b = _mm256_srai_epi32(res_temp6_8x16b, shift);
                res_temp7_8x16b = _mm256_srai_epi32(res_temp7_8x16b, shift);
                res_temp5_8x16b = _mm256_packs_epi32(res_temp6_8x16b, res_temp7_8x16b);

                res_temp5_8x16b = _mm256_min_epi16(res_temp5_8x16b, mm_max);
                res_temp5_8x16b = _mm256_max_epi16(res_temp5_8x16b, mm_min);

                /* to store the 8 pixels res. */
                _mm256_storeu_si256((__m256i *)(dst_copy + col), res_temp5_8x16b);
            }

            inp_copy += src_stride; /* pointer updates*/
            dst_copy += dst_stride; /* pointer updates*/
        }
    }
    else if (rem_w > 7)
    {
        __m128i offset_8x16b = _mm_set1_epi32(offset);
        __m128i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b, src_temp5_16x8b, src_temp6_16x8b;
        __m128i src_temp7_16x8b, src_temp8_16x8b, src_temp9_16x8b, src_temp0_16x8b;
        __m128i res_temp1_8x16b, res_temp2_8x16b, res_temp3_8x16b, res_temp4_8x16b, res_temp5_8x16b, res_temp6_8x16b, res_temp7_8x16b, res_temp8_8x16b;
        __m128i res_temp9_8x16b, res_temp0_8x16b;
        __m128i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b, coeff6_7_8x16b;
        __m128i mm_min = _mm_set1_epi16(min_val);
        __m128i mm_max = _mm_set1_epi16(max_val);
        /* load 8 8-bit coefficients and convert 8-bit into 16-bit  */
        coeff0_1_8x16b = _mm_loadu_si128((__m128i*)coeff);

        coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff4_5_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xaa);
        coeff6_7_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xff);
        coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);

        for (row = 0; row < height; row += 1)
        {
            /*load 8 pixel values from row 0*/
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[0]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride]));

            src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp7_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);
            res_temp7_8x16b = _mm_madd_epi16(src_temp7_16x8b, coeff0_1_8x16b);
            /* row = 0 */
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 2]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 3]));

            src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp8_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);
            res_temp8_8x16b = _mm_madd_epi16(src_temp8_16x8b, coeff2_3_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 4]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 5]));

            src_temp5_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp9_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp3_8x16b = _mm_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);
            res_temp9_8x16b = _mm_madd_epi16(src_temp9_16x8b, coeff4_5_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 6]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 7]));

            src_temp6_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp0_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp4_8x16b = _mm_madd_epi16(src_temp6_16x8b, coeff6_7_8x16b);
            res_temp0_8x16b = _mm_madd_epi16(src_temp0_16x8b, coeff6_7_8x16b);

            res_temp5_8x16b = _mm_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
            res_temp6_8x16b = _mm_add_epi32(res_temp3_8x16b, res_temp4_8x16b);
            res_temp5_8x16b = _mm_add_epi32(res_temp5_8x16b, res_temp6_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp7_8x16b, res_temp8_8x16b);
            res_temp7_8x16b = _mm_add_epi32(res_temp9_8x16b, res_temp0_8x16b);
            res_temp8_8x16b = _mm_add_epi32(res_temp6_8x16b, res_temp7_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp5_8x16b, offset_8x16b);
            res_temp7_8x16b = _mm_add_epi32(res_temp8_8x16b, offset_8x16b);
            res_temp6_8x16b = _mm_srai_epi32(res_temp6_8x16b, shift);
            res_temp7_8x16b = _mm_srai_epi32(res_temp7_8x16b, shift);
            res_temp5_8x16b = _mm_packs_epi32(res_temp6_8x16b, res_temp7_8x16b);

            res_temp5_8x16b = _mm_min_epi16(res_temp5_8x16b, mm_max);
            res_temp5_8x16b = _mm_max_epi16(res_temp5_8x16b, mm_min);

            /* to store the 8 pixels res. */
            _mm_storeu_si128((__m128i *)(dst_copy), res_temp5_8x16b);

            inp_copy += src_stride; /* pointer updates*/
            dst_copy += dst_stride; /* pointer updates*/
        }
    }
    else if (rem_w > 3)
    {
        __m128i offset_8x16b = _mm_set1_epi32(offset);
        __m128i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b, src_temp5_16x8b, src_temp6_16x8b;
        __m128i res_temp1_8x16b, res_temp2_8x16b, res_temp3_8x16b, res_temp4_8x16b, res_temp5_8x16b, res_temp6_8x16b;
        __m128i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b, coeff6_7_8x16b;
        __m128i mm_min = _mm_set1_epi16(min_val);
        __m128i mm_max = _mm_set1_epi16(max_val);
        coeff0_1_8x16b = _mm_loadu_si128((__m128i*)coeff);
        coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff4_5_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xaa);
        coeff6_7_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xff);
        coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);

        inp_copy = src_tmp + ((width / 8) * 8);
        dst_copy = pred + ((width / 8) * 8);

        for (row = 0; row < height; row += 1)
        {
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[0]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride]));

            src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 2]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 3]));

            src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 4]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 5]));

            src_temp5_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp3_8x16b = _mm_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 6]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 7]));

            src_temp6_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp4_8x16b = _mm_madd_epi16(src_temp6_16x8b, coeff6_7_8x16b);

            res_temp5_8x16b = _mm_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
            res_temp6_8x16b = _mm_add_epi32(res_temp3_8x16b, res_temp4_8x16b);
            res_temp5_8x16b = _mm_add_epi32(res_temp5_8x16b, res_temp6_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp5_8x16b, offset_8x16b);
            res_temp6_8x16b = _mm_srai_epi32(res_temp6_8x16b, shift);
            res_temp5_8x16b = _mm_packs_epi32(res_temp6_8x16b, res_temp6_8x16b);

            res_temp5_8x16b = _mm_min_epi16(res_temp5_8x16b, mm_max);
            res_temp5_8x16b = _mm_max_epi16(res_temp5_8x16b, mm_min);

            /* to store the 1st 4 pixels res. */
            _mm_storel_epi64((__m128i *)(dst_copy), res_temp5_8x16b);
            inp_copy += src_stride;
            dst_copy += dst_stride;
        }
    }
}


void mc_filter_l_6pel_vert_clip_avx(s16 *ref,
    int src_stride,
    s16 *pred,
    int dst_stride,
    const s16 *coeff,
    int width,
    int height,
    int min_val,
    int max_val,
    int offset,
    int shift)
{
    int row, col, rem_w;
    s16 const *src_tmp;
    s16 const *inp_copy;
    s16 *dst_copy;

    src_tmp = ref;
    rem_w = width;
    inp_copy = src_tmp;
    dst_copy = pred;

    if (rem_w > 15)
    {
        __m256i offset_8x16b = _mm256_set1_epi32(offset);
        __m256i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b, src_temp5_16x8b;
        __m256i src_temp7_16x8b, src_temp8_16x8b, src_temp9_16x8b;
        __m256i res_temp1_8x16b, res_temp2_8x16b, res_temp3_8x16b, res_temp5_8x16b, res_temp6_8x16b, res_temp7_8x16b, res_temp8_8x16b;
        __m256i res_temp9_8x16b;
        __m256i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b, coeff6_7_8x16b;
        __m256i mm_min = _mm256_set1_epi16(min_val);
        __m256i mm_max = _mm256_set1_epi16(max_val);
        short tmp_buf[16] = { 0 };
        xeve_mcpy(tmp_buf, coeff + 1, 12);
        xeve_mcpy(tmp_buf + 8, coeff + 1, 12);
        coeff0_1_8x16b = _mm256_loadu_si256((__m256i*)(tmp_buf));

        coeff2_3_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff4_5_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0xaa);
        coeff6_7_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0xff);
        coeff0_1_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0);

        for (row = 0; row < height; row += 1)
        {
            for (col = 0; col < width; col += 16)
            {
                /*load 8 pixel values from row 0*/
                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + src_stride]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + src_stride * 2]));

                src_temp3_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp7_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp1_8x16b = _mm256_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);
                res_temp7_8x16b = _mm256_madd_epi16(src_temp7_16x8b, coeff0_1_8x16b);
                /* row = 0 */
                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + src_stride * 3]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + src_stride * 4]));

                src_temp4_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp8_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp2_8x16b = _mm256_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);
                res_temp8_8x16b = _mm256_madd_epi16(src_temp8_16x8b, coeff2_3_8x16b);

                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + src_stride * 5]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + src_stride * 6]));

                src_temp5_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp9_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp3_8x16b = _mm256_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);
                res_temp9_8x16b = _mm256_madd_epi16(src_temp9_16x8b, coeff4_5_8x16b);

                res_temp5_8x16b = _mm256_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
                res_temp5_8x16b = _mm256_add_epi32(res_temp5_8x16b, res_temp3_8x16b);

                res_temp6_8x16b = _mm256_add_epi32(res_temp7_8x16b, res_temp8_8x16b);
                res_temp8_8x16b = _mm256_add_epi32(res_temp6_8x16b, res_temp9_8x16b);

                res_temp6_8x16b = _mm256_add_epi32(res_temp5_8x16b, offset_8x16b);
                res_temp7_8x16b = _mm256_add_epi32(res_temp8_8x16b, offset_8x16b);
                res_temp6_8x16b = _mm256_srai_epi32(res_temp6_8x16b, shift);
                res_temp7_8x16b = _mm256_srai_epi32(res_temp7_8x16b, shift);
                res_temp5_8x16b = _mm256_packs_epi32(res_temp6_8x16b, res_temp7_8x16b);
                
                res_temp5_8x16b = _mm256_min_epi16(res_temp5_8x16b, mm_max);
                res_temp5_8x16b = _mm256_max_epi16(res_temp5_8x16b, mm_min);

                /* to store the 8 pixels res. */
                _mm256_storeu_si256((__m256i *)(dst_copy + col), res_temp5_8x16b);
            }

            inp_copy += src_stride; /* pointer updates*/
            dst_copy += dst_stride; /* pointer updates*/
        }
    }
    else if (rem_w > 7)
    {
        __m128i offset_8x16b = _mm_set1_epi32(offset);
        __m128i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b, src_temp5_16x8b;
        __m128i src_temp7_16x8b, src_temp8_16x8b, src_temp9_16x8b;
        __m128i res_temp1_8x16b, res_temp2_8x16b, res_temp3_8x16b, res_temp5_8x16b, res_temp6_8x16b, res_temp7_8x16b, res_temp8_8x16b;
        __m128i res_temp9_8x16b;
        __m128i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b;
        __m128i mm_min = _mm_set1_epi16(min_val);
        __m128i mm_max = _mm_set1_epi16(max_val);
        /* load 8 8-bit coefficients and convert 8-bit into 16-bit  */
        coeff0_1_8x16b = _mm_loadu_si128((__m128i*)(coeff + 1));

        coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff4_5_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xaa);        
        coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);

        for (row = 0; row < height; row += 1)
        {
            /*load 8 pixel values from row 0*/
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 2]));

            src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp7_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);
            res_temp7_8x16b = _mm_madd_epi16(src_temp7_16x8b, coeff0_1_8x16b);
            /* row = 0 */
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 3]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 4]));

            src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp8_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);
            res_temp8_8x16b = _mm_madd_epi16(src_temp8_16x8b, coeff2_3_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 5]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 6]));

            src_temp5_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp9_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp3_8x16b = _mm_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);
            res_temp9_8x16b = _mm_madd_epi16(src_temp9_16x8b, coeff4_5_8x16b);

            res_temp5_8x16b = _mm_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
            res_temp5_8x16b = _mm_add_epi32(res_temp5_8x16b, res_temp3_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp7_8x16b, res_temp8_8x16b);
            res_temp8_8x16b = _mm_add_epi32(res_temp6_8x16b, res_temp9_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp5_8x16b, offset_8x16b);
            res_temp7_8x16b = _mm_add_epi32(res_temp8_8x16b, offset_8x16b);
            res_temp6_8x16b = _mm_srai_epi32(res_temp6_8x16b, shift);
            res_temp7_8x16b = _mm_srai_epi32(res_temp7_8x16b, shift);
            res_temp5_8x16b = _mm_packs_epi32(res_temp6_8x16b, res_temp7_8x16b);

            res_temp5_8x16b = _mm_min_epi16(res_temp5_8x16b, mm_max);
            res_temp5_8x16b = _mm_max_epi16(res_temp5_8x16b, mm_min);

            /* to store the 8 pixels res. */
            _mm_storeu_si128((__m128i *)(dst_copy), res_temp5_8x16b);

            inp_copy += src_stride; /* pointer updates*/
            dst_copy += dst_stride; /* pointer updates*/
        }
    }
    else if (rem_w > 3)
    {
        __m128i offset_8x16b = _mm_set1_epi32(offset);
        __m128i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b, src_temp5_16x8b;
        __m128i res_temp1_8x16b, res_temp2_8x16b, res_temp3_8x16b, res_temp5_8x16b, res_temp6_8x16b;
        __m128i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b;
        __m128i mm_min = _mm_set1_epi16(min_val);
        __m128i mm_max = _mm_set1_epi16(max_val);
        coeff0_1_8x16b = _mm_loadu_si128((__m128i*)(coeff + 1));
        coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff4_5_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xaa);
        coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);

        inp_copy = src_tmp + ((width / 8) * 8);
        dst_copy = pred + ((width / 8) * 8);

        for (row = 0; row < height; row += 1)
        {
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 2]));

            src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride* 3]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride* 4]));

            src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 5]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 6]));

            src_temp5_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp3_8x16b = _mm_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);

            res_temp5_8x16b = _mm_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
            res_temp5_8x16b = _mm_add_epi32(res_temp5_8x16b, res_temp3_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp5_8x16b, offset_8x16b);
            res_temp6_8x16b = _mm_srai_epi32(res_temp6_8x16b, shift);
            res_temp5_8x16b = _mm_packs_epi32(res_temp6_8x16b, res_temp6_8x16b);

            res_temp5_8x16b = _mm_min_epi16(res_temp5_8x16b, mm_max);
            res_temp5_8x16b = _mm_max_epi16(res_temp5_8x16b, mm_min);

            /* to store the 1st 4 pixels res. */
            _mm_storel_epi64((__m128i *)(dst_copy), res_temp5_8x16b);
            inp_copy += src_stride;
            dst_copy += dst_stride;
        }
    }
}

void xeve_mc_l_n0_avx(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int dx = gmv_x & 15;
    ref += (gmv_y >> 4) * s_ref + (gmv_x >> 4) - 3;

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    mc_filter_l_8pel_horz_clip_avx(ref, s_ref, pred, s_pred, xeve_mc_l_coeff[dx], w, h, min, max, MAC_ADD_N0, MAC_SFT_N0);
}

void xeve_mc_l_0n_avx(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int dy = gmv_y & 15;
    ref += ((gmv_y >> 4) - 3) * s_ref + (gmv_x >> 4);

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    mc_filter_l_8pel_vert_clip_avx(ref, s_ref, pred, s_pred, xeve_mc_l_coeff[dy], w, h, min, max, MAC_ADD_0N, MAC_SFT_0N);
}

void xeve_mc_l_nn_avx(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    s16         buf[(MAX_CU_SIZE + MC_IBUF_PAD_L)*(MAX_CU_SIZE + MC_IBUF_PAD_L)];
    int         dx, dy;

    dx = gmv_x & 15;
    dy = gmv_y & 15;
    ref += ((gmv_y >> 4) - 3) * s_ref + (gmv_x >> 4) - 3;

    int shift1 = XEVE_MIN(4, bit_depth - 8);
    int shift2 = XEVE_MAX(8, 20 - bit_depth);
    int offset1 = 0;
    int offset2 = (1 << (shift2 - 1));
    int max = ((1 << bit_depth) - 1);
    int min = 0;

    mc_filter_l_8pel_horz_no_clip_avx(ref, s_ref, buf, w, xeve_mc_l_coeff[dx], w, (h + 7), offset1, shift1);
    mc_filter_l_8pel_vert_clip_avx(buf, w, pred, s_pred, xeve_mc_l_coeff[dy], w, h, min, max, offset2, shift2);
}


/****************************************************************************
 * motion compensation for chroma
 ****************************************************************************/

void mc_filter_c_4pel_horz_avx(s16 *ref, int src_stride, s16 *pred, int dst_stride, const s16 *coeff
                             , int width, int height, int min_val, int max_val, int offset, int shift, s8  is_last)
{
    int row, col, rem_w;
    s16 const *src_tmp;
    s16 const *inp_copy;
    s16 *dst_copy;

    src_tmp = ref;
    rem_w = width;
    inp_copy = src_tmp;
    dst_copy = pred;

    if (rem_w > 15)
    {
        __m256i offset_8x16b = _mm256_set1_epi32(offset);
        __m256i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b;
        __m256i src_temp7_16x8b, src_temp8_16x8b;
        __m256i res_temp1_8x16b, res_temp2_8x16b, res_temp5_8x16b, res_temp6_8x16b, res_temp7_8x16b, res_temp8_8x16b;
        __m256i coeff0_1_8x16b, coeff2_3_8x16b;
        __m256i mm_min = _mm256_set1_epi16(min_val);
        __m256i mm_max = _mm256_set1_epi16(max_val);
        short tmp_buf[16] = { 0 };
        xeve_mcpy(tmp_buf, coeff, 8);
        xeve_mcpy(tmp_buf + 8, coeff, 8);
        coeff0_1_8x16b = _mm256_loadu_si256((__m256i*)(tmp_buf));

        coeff2_3_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff0_1_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0);

        for (row = 0; row < height; row += 1)
        {
            for (col = 0; col < width; col += 16)
            {
                /*load 8 pixel values from row 0*/
                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 1]));

                src_temp3_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp7_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp1_8x16b = _mm256_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);
                res_temp7_8x16b = _mm256_madd_epi16(src_temp7_16x8b, coeff0_1_8x16b);
                /* row = 0 */
                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 2]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + 3]));

                src_temp4_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp8_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp2_8x16b = _mm256_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);
                res_temp8_8x16b = _mm256_madd_epi16(src_temp8_16x8b, coeff2_3_8x16b);

                res_temp5_8x16b = _mm256_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
                res_temp8_8x16b = _mm256_add_epi32(res_temp7_8x16b, res_temp8_8x16b);

                res_temp6_8x16b = _mm256_add_epi32(res_temp5_8x16b, offset_8x16b);
                res_temp7_8x16b = _mm256_add_epi32(res_temp8_8x16b, offset_8x16b);
                res_temp6_8x16b = _mm256_srai_epi32(res_temp6_8x16b, shift);
                res_temp7_8x16b = _mm256_srai_epi32(res_temp7_8x16b, shift);
                res_temp5_8x16b = _mm256_packs_epi32(res_temp6_8x16b, res_temp7_8x16b);

                if (is_last)
                {
                    res_temp5_8x16b = _mm256_min_epi16(res_temp5_8x16b, mm_max);
                    res_temp5_8x16b = _mm256_max_epi16(res_temp5_8x16b, mm_min);
                }

                /* to store the 8 pixels res. */
                _mm256_storeu_si256((__m256i *)(dst_copy + col), res_temp5_8x16b);
            }

            inp_copy += src_stride; /* pointer updates*/
            dst_copy += dst_stride; /* pointer updates*/
        }
    }
    else if (rem_w > 7)
    {
        __m128i offset_8x16b = _mm_set1_epi32(offset);
        __m128i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b;
        __m128i src_temp7_16x8b, src_temp8_16x8b;
        __m128i res_temp1_8x16b, res_temp2_8x16b, res_temp5_8x16b, res_temp6_8x16b, res_temp7_8x16b, res_temp8_8x16b;
        __m128i coeff0_1_8x16b, coeff2_3_8x16b;
        __m128i mm_min = _mm_set1_epi16(min_val);
        __m128i mm_max = _mm_set1_epi16(max_val);
        /* load 8 8-bit coefficients and convert 8-bit into 16-bit  */
        coeff0_1_8x16b = _mm_loadu_si128((__m128i*)coeff);

        coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);

        for (row = 0; row < height; row += 1)
        {
            /*load 8 pixel values from row 0*/
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[0]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[1]));

            src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp7_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);
            res_temp7_8x16b = _mm_madd_epi16(src_temp7_16x8b, coeff0_1_8x16b);
            /* row = 0 */
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[2]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[3]));

            src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp8_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);
            res_temp8_8x16b = _mm_madd_epi16(src_temp8_16x8b, coeff2_3_8x16b);

            res_temp5_8x16b = _mm_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
            res_temp8_8x16b = _mm_add_epi32(res_temp7_8x16b, res_temp8_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp5_8x16b, offset_8x16b);
            res_temp7_8x16b = _mm_add_epi32(res_temp8_8x16b, offset_8x16b);
            res_temp6_8x16b = _mm_srai_epi32(res_temp6_8x16b, shift);
            res_temp7_8x16b = _mm_srai_epi32(res_temp7_8x16b, shift);
            res_temp5_8x16b = _mm_packs_epi32(res_temp6_8x16b, res_temp7_8x16b);

            if (is_last)
            {
                res_temp5_8x16b = _mm_min_epi16(res_temp5_8x16b, mm_max);
                res_temp5_8x16b = _mm_max_epi16(res_temp5_8x16b, mm_min);
            }

            /* to store the 8 pixels res. */
            _mm_storeu_si128((__m128i *)(dst_copy), res_temp5_8x16b);

            inp_copy += src_stride; /* pointer updates*/
            dst_copy += dst_stride; /* pointer updates*/
        }
    }
    else if (rem_w > 3)
    {
        __m128i offset_8x16b = _mm_set1_epi32(offset);
        __m128i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b;
        __m128i res_temp1_8x16b, res_temp2_8x16b, res_temp5_8x16b, res_temp6_8x16b;
        __m128i coeff0_1_8x16b, coeff2_3_8x16b;
        __m128i mm_min = _mm_set1_epi16(min_val);
        __m128i mm_max = _mm_set1_epi16(max_val);
        coeff0_1_8x16b = _mm_loadu_si128((__m128i*)coeff);
        coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);

        inp_copy = src_tmp + ((width / 8) * 8);
        dst_copy = pred + ((width / 8) * 8);

        for (row = 0; row < height; row += 1)
        {
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[0]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[1]));

            src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[2]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[3]));

            src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);

            res_temp5_8x16b = _mm_add_epi32(res_temp1_8x16b, res_temp2_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp5_8x16b, offset_8x16b);
            res_temp6_8x16b = _mm_srai_epi32(res_temp6_8x16b, shift);
            res_temp5_8x16b = _mm_packs_epi32(res_temp6_8x16b, res_temp6_8x16b);

            if (is_last)
            {
                res_temp5_8x16b = _mm_min_epi16(res_temp5_8x16b, mm_max);
                res_temp5_8x16b = _mm_max_epi16(res_temp5_8x16b, mm_min);
            }

            /* to store the 1st 4 pixels res. */
            _mm_storel_epi64((__m128i *)(dst_copy), res_temp5_8x16b);
            inp_copy += src_stride;
            dst_copy += dst_stride;
        }
    }
    else if (rem_w)
    {
        inp_copy = ref + ((width / 4) * 4);
        dst_copy = pred + ((width / 4) * 4);

        for (row = 0; row < height; row++)
        {
            for (col = 0; col < rem_w; col++)
            {
                s16 val;
                int sum;

                sum = inp_copy[col + 0] * coeff[0];
                sum += inp_copy[col + 1] * coeff[1];
                sum += inp_copy[col + 2] * coeff[2];
                sum += inp_copy[col + 3] * coeff[3];

                val = (sum + offset) >> shift;
                dst_copy[col] = (is_last ? (XEVE_CLIP3(min_val, max_val, val)) : val);
            }
            inp_copy += (src_stride); /* pointer updates*/
            dst_copy += (dst_stride); /* pointer updates*/
        }
    }
}

void mc_filter_c_4pel_vert_avx(s16 *ref,
    int src_stride,
    s16 *pred,
    int dst_stride,
    const s16 *coeff,
    int width,
    int height,
    int min_val,
    int max_val,
    int offset,
    int shift,
    s8  is_last)
{
    int row, col, rem_w;
    s16 const *src_tmp;
    s16 const *inp_copy;
    s16 *dst_copy;

    src_tmp = ref;
    rem_w = width;
    inp_copy = src_tmp;
    dst_copy = pred;

    if (rem_w > 15)
    {
        __m256i offset_8x16b = _mm256_set1_epi32(offset);
        __m256i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b;
        __m256i src_temp7_16x8b, src_temp8_16x8b;
        __m256i res_temp1_8x16b, res_temp2_8x16b, res_temp5_8x16b, res_temp6_8x16b, res_temp7_8x16b, res_temp8_8x16b;
        __m256i coeff0_1_8x16b, coeff2_3_8x16b;
        __m256i mm_min = _mm256_set1_epi16(min_val);
        __m256i mm_max = _mm256_set1_epi16(max_val);
        short tmp_buf[16] = { 0 };
        xeve_mcpy(tmp_buf, coeff, 8);
        xeve_mcpy(tmp_buf + 8, coeff, 8);
        coeff0_1_8x16b = _mm256_loadu_si256((__m256i*)(tmp_buf));

        coeff2_3_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff0_1_8x16b = _mm256_shuffle_epi32(coeff0_1_8x16b, 0);

        for (row = 0; row < height; row += 1)
        {
            for (col = 0; col < width; col += 16)
            {
                /*load 8 pixel values from row 0*/
                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + src_stride]));

                src_temp3_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp7_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp1_8x16b = _mm256_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);
                res_temp7_8x16b = _mm256_madd_epi16(src_temp7_16x8b, coeff0_1_8x16b);
                /* row = 0 */
                src_temp1_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + src_stride * 2]));
                src_temp2_16x8b = _mm256_loadu_si256((__m256i*)(&inp_copy[col + src_stride * 3]));

                src_temp4_16x8b = _mm256_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp8_16x8b = _mm256_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp2_8x16b = _mm256_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);
                res_temp8_8x16b = _mm256_madd_epi16(src_temp8_16x8b, coeff2_3_8x16b);

                res_temp5_8x16b = _mm256_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
                res_temp8_8x16b = _mm256_add_epi32(res_temp7_8x16b, res_temp8_8x16b);

                res_temp6_8x16b = _mm256_add_epi32(res_temp5_8x16b, offset_8x16b);
                res_temp7_8x16b = _mm256_add_epi32(res_temp8_8x16b, offset_8x16b);
                res_temp6_8x16b = _mm256_srai_epi32(res_temp6_8x16b, shift);
                res_temp7_8x16b = _mm256_srai_epi32(res_temp7_8x16b, shift);
                res_temp5_8x16b = _mm256_packs_epi32(res_temp6_8x16b, res_temp7_8x16b);

                res_temp5_8x16b = _mm256_min_epi16(res_temp5_8x16b, mm_max);
                res_temp5_8x16b = _mm256_max_epi16(res_temp5_8x16b, mm_min);

                /* to store the 8 pixels res. */
                _mm256_storeu_si256((__m256i *)(dst_copy + col), res_temp5_8x16b);
            }

            inp_copy += src_stride; /* pointer updates*/
            dst_copy += dst_stride; /* pointer updates*/
        }
    }
    else if (rem_w > 7)
    {
        __m128i offset_8x16b = _mm_set1_epi32(offset);
        __m128i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b;
        __m128i src_temp7_16x8b, src_temp8_16x8b;
        __m128i res_temp1_8x16b, res_temp2_8x16b, res_temp5_8x16b, res_temp6_8x16b, res_temp7_8x16b, res_temp8_8x16b;
        __m128i coeff0_1_8x16b, coeff2_3_8x16b;
        __m128i mm_min = _mm_set1_epi16(min_val);
        __m128i mm_max = _mm_set1_epi16(max_val);
        /* load 8 8-bit coefficients and convert 8-bit into 16-bit  */
        coeff0_1_8x16b = _mm_loadu_si128((__m128i*)coeff);

        coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);

        for (row = 0; row < height; row += 1)
        {
            /*load 8 pixel values from row 0*/
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[0]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride]));

            src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp7_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);
            res_temp7_8x16b = _mm_madd_epi16(src_temp7_16x8b, coeff0_1_8x16b);
            /* row = 0 */
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 2]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 3]));

            src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            src_temp8_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);
            res_temp8_8x16b = _mm_madd_epi16(src_temp8_16x8b, coeff2_3_8x16b);

            res_temp5_8x16b = _mm_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
            res_temp8_8x16b = _mm_add_epi32(res_temp7_8x16b, res_temp8_8x16b);

            res_temp6_8x16b = _mm_add_epi32(res_temp5_8x16b, offset_8x16b);
            res_temp7_8x16b = _mm_add_epi32(res_temp8_8x16b, offset_8x16b);
            res_temp6_8x16b = _mm_srai_epi32(res_temp6_8x16b, shift);
            res_temp7_8x16b = _mm_srai_epi32(res_temp7_8x16b, shift);
            res_temp5_8x16b = _mm_packs_epi32(res_temp6_8x16b, res_temp7_8x16b);

            res_temp5_8x16b = _mm_min_epi16(res_temp5_8x16b, mm_max);
            res_temp5_8x16b = _mm_max_epi16(res_temp5_8x16b, mm_min);

            /* to store the 8 pixels res. */
            _mm_storeu_si128((__m128i *)(dst_copy), res_temp5_8x16b);

            inp_copy += src_stride; /* pointer updates*/
            dst_copy += dst_stride; /* pointer updates*/
        }
    }
    else if (rem_w > 3)
    {
        __m128i offset_8x16b = _mm_set1_epi32(offset);
        __m128i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b;
        __m128i res_temp1_8x16b, res_temp2_8x16b, res_temp5_8x16b, res_temp6_8x16b;
        __m128i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b, coeff6_7_8x16b;
        __m128i mm_min = _mm_set1_epi16(min_val);
        __m128i mm_max = _mm_set1_epi16(max_val);
        coeff0_1_8x16b = _mm_loadu_si128((__m128i*)coeff);
        coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);
        coeff4_5_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xaa);
        coeff6_7_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xff);
        coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);

        inp_copy = src_tmp + ((width / 8) * 8);
        dst_copy = pred + ((width / 8) * 8);

        for (row = 0; row < height; row += 1)
        {
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[0]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride]));

            src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 2]));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[src_stride * 3]));

            src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);

            res_temp5_8x16b = _mm_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
            res_temp6_8x16b = _mm_add_epi32(res_temp5_8x16b, offset_8x16b);
            res_temp6_8x16b = _mm_srai_epi32(res_temp6_8x16b, shift);
            res_temp5_8x16b = _mm_packs_epi32(res_temp6_8x16b, res_temp6_8x16b);

            res_temp5_8x16b = _mm_min_epi16(res_temp5_8x16b, mm_max);
            res_temp5_8x16b = _mm_max_epi16(res_temp5_8x16b, mm_min);

            /* to store the 1st 4 pixels res. */
            _mm_storel_epi64((__m128i *)(dst_copy), res_temp5_8x16b);
            inp_copy += src_stride;
            dst_copy += dst_stride;
        }
    }
    else if (rem_w)
    {
        inp_copy = src_tmp + ((width / 4) * 4);
        dst_copy = pred + ((width / 4) * 4);

        for (row = 0; row < height; row++)
        {
            for (col = 0; col < rem_w; col++)
            {
                s16 val;
                int sum;

                sum = inp_copy[col + 0 * src_stride] * coeff[0];
                sum += inp_copy[col + 1 * src_stride] * coeff[1];
                sum += inp_copy[col + 2 * src_stride] * coeff[2];
                sum += inp_copy[col + 3 * src_stride] * coeff[3];

                val = (sum + offset) >> shift;
                dst_copy[col] = (is_last ? (XEVE_CLIP3(min_val, max_val, val)) : val);
            }

            inp_copy += src_stride;
            dst_copy += dst_stride;
        }
    }
}

void xeve_mc_c_n0_avx(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    int  dx = gmv_x & 31;
    ref += (gmv_y >> 5) * s_ref + (gmv_x >> 5) - 1;

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    mc_filter_c_4pel_horz_avx(ref, s_ref, pred, s_pred, xeve_mc_c_coeff[dx], w, h, min, max, MAC_ADD_N0, MAC_SFT_N0, 1);
}

void xeve_mc_c_0n_avx(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    int dy = gmv_y & 31;
    ref += ((gmv_y >> 5) - 1) * s_ref + (gmv_x >> 5);

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    mc_filter_c_4pel_vert_avx(ref, s_ref, pred, s_pred, xeve_mc_c_coeff[dy], w, h, min, max, MAC_ADD_0N, MAC_SFT_0N, 1);
}

void xeve_mc_c_nn_avx(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    s16 buf[(MAX_CU_SIZE + MC_IBUF_PAD_C)*MAX_CU_SIZE];
    int dx, dy;

    dx = gmv_x & 31;
    dy = gmv_y & 31;
    ref += ((gmv_y >> 5) - 1) * s_ref + (gmv_x >> 5) - 1;

    int shift1 = XEVE_MIN(4, bit_depth - 8);
    int shift2 = XEVE_MAX(8, 20 - bit_depth);
    int offset1 = 0;
    int offset2 = (1 << (shift2 - 1));
    int max = ((1 << bit_depth) - 1);
    int min = 0;

    mc_filter_c_4pel_horz_avx(ref, s_ref, buf, w, xeve_mc_c_coeff[dx], w, (h + 3), min, max, offset1, shift1, 0);
    mc_filter_c_4pel_vert_avx(buf, w, pred, s_pred, xeve_mc_c_coeff[dy], w, h, min, max, offset2, shift2, 1);
}

XEVE_MC_L xeve_tbl_mc_l_avx[2][2] =
{
    {
        xeve_mc_l_00, /* dx == 0 && dy == 0 */
        xeve_mc_l_0n_avx  /* dx == 0 && dy != 0 */
    },
    {
        xeve_mc_l_n0_avx, /* dx != 0 && dy == 0 */
        xeve_mc_l_nn_avx  /* dx != 0 && dy != 0 */
    }
};

XEVE_MC_C xeve_tbl_mc_c_avx[2][2] =
{
    {
        xeve_mc_c_00, /* dx == 0 && dy == 0 */
        xeve_mc_c_0n_avx  /* dx == 0 && dy != 0 */
    },
    {
        xeve_mc_c_n0_avx, /* dx != 0 && dy == 0 */
        xeve_mc_c_nn_avx  /* dx != 0 && dy != 0 */
    }
};

#endif /* X86_SSE */

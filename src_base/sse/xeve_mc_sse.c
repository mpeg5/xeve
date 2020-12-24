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

#include "xeve_tbl.h"
#include "xeve_util.h"
#include "xeve_mc_sse.h"
#include <assert.h>

#if X86_SSE
/****************************************************************************
 * motion compensation for luma
 ****************************************************************************/
void mc_filter_l_8pel_horz_clip_sse(s16 *ref,
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

    /* all 128 bit registers are named with a suffix mxnb, where m is the */
    /* number of n bits packed in the register                            */
    __m128i offset_8x16b = _mm_set1_epi32(offset);
    __m128i    mm_min = _mm_set1_epi16(min_val);
    __m128i mm_max = _mm_set1_epi16(max_val);
    __m128i src_temp1_16x8b, src_temp2_16x8b, src_temp3_16x8b, src_temp4_16x8b, src_temp5_16x8b, src_temp6_16x8b;
    __m128i src_temp7_16x8b, src_temp8_16x8b, src_temp9_16x8b, src_temp0_16x8b;
    __m128i src_temp11_16x8b, src_temp12_16x8b, src_temp13_16x8b, src_temp14_16x8b, src_temp15_16x8b, src_temp16_16x8b;
    __m128i res_temp1_8x16b, res_temp2_8x16b, res_temp3_8x16b, res_temp4_8x16b, res_temp5_8x16b, res_temp6_8x16b, res_temp7_8x16b, res_temp8_8x16b;
    __m128i res_temp9_8x16b, res_temp0_8x16b;
    __m128i res_temp11_8x16b, res_temp12_8x16b, res_temp13_8x16b, res_temp14_8x16b, res_temp15_8x16b, res_temp16_8x16b;
    __m128i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b, coeff6_7_8x16b;

    src_tmp = ref;
    rem_w = width;
    inp_copy = src_tmp;
    dst_copy = pred;

    /* load 8 8-bit coefficients and convert 8-bit into 16-bit  */
    coeff0_1_8x16b = _mm_loadu_si128((__m128i*)coeff);

    coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);
    coeff4_5_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xaa);
    coeff6_7_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xff);
    coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);

    if (!(height & 1))    /*even height*/
    {
        if (rem_w > 7)
        {
            for (row = 0; row < height; row += 1)
            {
                int cnt = 0;
                for (col = width; col > 7; col -= 8)
                {
                    /*load 8 pixel values from row 0*/
                    src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + cnt));
                    src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 1));

                    src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                    src_temp7_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                    res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);
                    res_temp7_8x16b = _mm_madd_epi16(src_temp7_16x8b, coeff0_1_8x16b);

                    src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 2));
                    src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 3));

                    src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                    src_temp8_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                    res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);
                    res_temp8_8x16b = _mm_madd_epi16(src_temp8_16x8b, coeff2_3_8x16b);

                    src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 4));
                    src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 5));

                    src_temp5_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                    src_temp9_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                    res_temp3_8x16b = _mm_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);
                    res_temp9_8x16b = _mm_madd_epi16(src_temp9_16x8b, coeff4_5_8x16b);

                    src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 6));
                    src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 7));

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

                    //if (is_last)
                    {
                        res_temp5_8x16b = _mm_min_epi16(res_temp5_8x16b, mm_max);
                        res_temp5_8x16b = _mm_max_epi16(res_temp5_8x16b, mm_min);
                    }

                    /* to store the 8 pixels res. */
                    _mm_storeu_si128((__m128i *)(dst_copy + cnt), res_temp5_8x16b);

                    cnt += 8; /* To pointer updates*/
                }

                inp_copy += src_stride; /* pointer updates*/
                dst_copy += dst_stride; /* pointer updates*/
            }
        }

        rem_w &= 0x7;

        if (rem_w > 3)
        {
            inp_copy = src_tmp + ((width / 8) * 8);
            dst_copy = pred + ((width / 8) * 8);

            for (row = 0; row < height; row += 2)
            {
                /*load 8 pixel values */
                src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy));                /* row = 0 */
                src_temp11_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride));    /* row = 1 */

                src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 1));
                src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);

                src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 2));
                src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 3));

                src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);

                src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 4));
                src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 5));

                src_temp5_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp3_8x16b = _mm_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);

                src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 6));
                src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 7));

                src_temp6_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp4_8x16b = _mm_madd_epi16(src_temp6_16x8b, coeff6_7_8x16b);

                res_temp5_8x16b = _mm_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
                res_temp6_8x16b = _mm_add_epi32(res_temp3_8x16b, res_temp4_8x16b);
                res_temp5_8x16b = _mm_add_epi32(res_temp5_8x16b, res_temp6_8x16b);

                res_temp6_8x16b = _mm_add_epi32(res_temp5_8x16b, offset_8x16b);
                res_temp6_8x16b = _mm_srai_epi32(res_temp6_8x16b, shift);
                res_temp5_8x16b = _mm_packs_epi32(res_temp6_8x16b, res_temp6_8x16b);

                src_temp12_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + 1));

                src_temp13_16x8b = _mm_unpacklo_epi16(src_temp11_16x8b, src_temp12_16x8b);
                res_temp11_8x16b = _mm_madd_epi16(src_temp13_16x8b, coeff0_1_8x16b);

                src_temp11_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + 2));
                src_temp12_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + 3));

                src_temp14_16x8b = _mm_unpacklo_epi16(src_temp11_16x8b, src_temp12_16x8b);
                res_temp12_8x16b = _mm_madd_epi16(src_temp14_16x8b, coeff2_3_8x16b);

                src_temp11_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + 4));
                src_temp12_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + 5));

                src_temp15_16x8b = _mm_unpacklo_epi16(src_temp11_16x8b, src_temp12_16x8b);
                res_temp13_8x16b = _mm_madd_epi16(src_temp15_16x8b, coeff4_5_8x16b);

                src_temp11_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + 6));
                src_temp12_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + 7));

                src_temp16_16x8b = _mm_unpacklo_epi16(src_temp11_16x8b, src_temp12_16x8b);
                res_temp14_8x16b = _mm_madd_epi16(src_temp16_16x8b, coeff6_7_8x16b);

                res_temp15_8x16b = _mm_add_epi32(res_temp11_8x16b, res_temp12_8x16b);
                res_temp16_8x16b = _mm_add_epi32(res_temp13_8x16b, res_temp14_8x16b);
                res_temp15_8x16b = _mm_add_epi32(res_temp15_8x16b, res_temp16_8x16b);

                res_temp16_8x16b = _mm_add_epi32(res_temp15_8x16b, offset_8x16b);
                res_temp16_8x16b = _mm_srai_epi32(res_temp16_8x16b, shift);
                res_temp15_8x16b = _mm_packs_epi32(res_temp16_8x16b, res_temp16_8x16b);

                //if (is_last)
                {
                    res_temp5_8x16b = _mm_min_epi16(res_temp5_8x16b, mm_max);
                    res_temp15_8x16b = _mm_min_epi16(res_temp15_8x16b, mm_max);

                    res_temp5_8x16b = _mm_max_epi16(res_temp5_8x16b, mm_min);
                    res_temp15_8x16b = _mm_max_epi16(res_temp15_8x16b, mm_min);
                }

                /* to store the 1st 4 pixels res. */
                _mm_storel_epi64((__m128i *)(dst_copy), res_temp5_8x16b);
                _mm_storel_epi64((__m128i *)(dst_copy + dst_stride), res_temp15_8x16b);
                inp_copy += (src_stride << 1);  /* Pointer update */
                dst_copy += (dst_stride << 1);  /* Pointer update */
            }
        }

        rem_w &= 0x3;

        if (rem_w)
        {
            __m128i filt_coef;
            s16 sum, sum1;

            inp_copy = src_tmp + ((width / 4) * 4);
            dst_copy = pred + ((width / 4) * 4);

            filt_coef = _mm_loadu_si128((__m128i*)coeff);   //w0 w1 w2 w3 w4 w5 w6 w7

            for (row = 0; row < height; row += 2)
            {
                for (col = 0; col < rem_w; col++)
                {
                    src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + col));
                    src_temp5_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + col));

                    src_temp1_16x8b = _mm_madd_epi16(src_temp1_16x8b, filt_coef);
                    src_temp5_16x8b = _mm_madd_epi16(src_temp5_16x8b, filt_coef);

                    src_temp1_16x8b = _mm_hadd_epi32(src_temp1_16x8b, src_temp5_16x8b);
                    src_temp1_16x8b = _mm_hadd_epi32(src_temp1_16x8b, src_temp1_16x8b);

                    src_temp1_16x8b = _mm_add_epi32(src_temp1_16x8b, offset_8x16b);
                    src_temp1_16x8b = _mm_srai_epi32(src_temp1_16x8b, shift);
                    src_temp1_16x8b = _mm_packs_epi32(src_temp1_16x8b, src_temp1_16x8b);

                    sum = _mm_extract_epi16(src_temp1_16x8b, 0);
                    sum1 = _mm_extract_epi16(src_temp1_16x8b, 1);

                    //if (is_last)
                    {
                        sum = (sum < min_val) ? min_val : (sum > max_val ? max_val : sum);
                        sum1 = (sum1 < min_val) ? min_val : (sum1 > max_val ? max_val : sum1);
                    }
                    dst_copy[col] = (sum);
                    dst_copy[col + dst_stride] = (sum1);
                }
                inp_copy += (src_stride << 1);
                dst_copy += (dst_stride << 1);
            }
        }
    }
    else
    {
        if (rem_w > 7)
        {
            for (row = 0; row < height; row += 1)
            {
                int cnt = 0;
                for (col = width; col > 7; col -= 8)
                {
                    /*load 8 pixel values from row 0*/
                    src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + cnt));
                    src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 1));

                    src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                    src_temp7_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                    res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);
                    res_temp7_8x16b = _mm_madd_epi16(src_temp7_16x8b, coeff0_1_8x16b);
                    /* row = 0 */
                    src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 2));
                    src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 3));

                    src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                    src_temp8_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                    res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);
                    res_temp8_8x16b = _mm_madd_epi16(src_temp8_16x8b, coeff2_3_8x16b);

                    src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 4));
                    src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 5));

                    src_temp5_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                    src_temp9_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                    res_temp3_8x16b = _mm_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);
                    res_temp9_8x16b = _mm_madd_epi16(src_temp9_16x8b, coeff4_5_8x16b);

                    src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 6));
                    src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 7));

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

                    //if (is_last)
                    {
                        res_temp5_8x16b = _mm_min_epi16(res_temp5_8x16b, mm_max);
                        res_temp5_8x16b = _mm_max_epi16(res_temp5_8x16b, mm_min);
                    }

                    /* to store the 8 pixels res. */
                    _mm_storeu_si128((__m128i *)(dst_copy + cnt), res_temp5_8x16b);

                    cnt += 8; /* To pointer updates*/
                }

                inp_copy += src_stride; /* pointer updates*/
                dst_copy += dst_stride; /* pointer updates*/
            }
        }

        rem_w &= 0x7;

        if (rem_w > 3)
        {
            inp_copy = src_tmp + ((width / 8) * 8);
            dst_copy = pred + ((width / 8) * 8);

            for (row = 0; row < (height - 1); row += 2)
            {

                src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy));
                src_temp11_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride));

                src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 1));

                src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);

                src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 2));
                src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 3));

                src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);

                src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 4));
                src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 5));

                src_temp5_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp3_8x16b = _mm_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);

                src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 6));
                src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 7));

                src_temp6_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp4_8x16b = _mm_madd_epi16(src_temp6_16x8b, coeff6_7_8x16b);

                res_temp5_8x16b = _mm_add_epi32(res_temp1_8x16b, res_temp2_8x16b);
                res_temp6_8x16b = _mm_add_epi32(res_temp3_8x16b, res_temp4_8x16b);
                res_temp5_8x16b = _mm_add_epi32(res_temp5_8x16b, res_temp6_8x16b);

                res_temp6_8x16b = _mm_add_epi32(res_temp5_8x16b, offset_8x16b);
                res_temp6_8x16b = _mm_srai_epi32(res_temp6_8x16b, shift);
                res_temp5_8x16b = _mm_packs_epi32(res_temp6_8x16b, res_temp6_8x16b);

                src_temp12_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + 1));

                src_temp13_16x8b = _mm_unpacklo_epi16(src_temp11_16x8b, src_temp12_16x8b);
                res_temp11_8x16b = _mm_madd_epi16(src_temp13_16x8b, coeff0_1_8x16b);
                /* row =1 */
                src_temp11_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + 2));
                src_temp12_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + 3));

                src_temp14_16x8b = _mm_unpacklo_epi16(src_temp11_16x8b, src_temp12_16x8b);
                res_temp12_8x16b = _mm_madd_epi16(src_temp14_16x8b, coeff2_3_8x16b);

                src_temp11_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + 4));
                src_temp12_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + 5));

                src_temp15_16x8b = _mm_unpacklo_epi16(src_temp11_16x8b, src_temp12_16x8b);
                res_temp13_8x16b = _mm_madd_epi16(src_temp15_16x8b, coeff4_5_8x16b);

                src_temp11_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + 6));
                src_temp12_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + 7));

                src_temp16_16x8b = _mm_unpacklo_epi16(src_temp11_16x8b, src_temp12_16x8b);
                res_temp14_8x16b = _mm_madd_epi16(src_temp16_16x8b, coeff6_7_8x16b);

                res_temp15_8x16b = _mm_add_epi32(res_temp11_8x16b, res_temp12_8x16b);
                res_temp16_8x16b = _mm_add_epi32(res_temp13_8x16b, res_temp14_8x16b);
                res_temp15_8x16b = _mm_add_epi32(res_temp15_8x16b, res_temp16_8x16b);

                res_temp16_8x16b = _mm_add_epi32(res_temp15_8x16b, offset_8x16b);
                res_temp16_8x16b = _mm_srai_epi32(res_temp16_8x16b, shift);
                res_temp15_8x16b = _mm_packs_epi32(res_temp16_8x16b, res_temp16_8x16b);

                res_temp5_8x16b = _mm_min_epi16(res_temp5_8x16b, mm_max);
                res_temp15_8x16b = _mm_min_epi16(res_temp15_8x16b, mm_max);
                res_temp5_8x16b = _mm_max_epi16(res_temp5_8x16b, mm_min);
                res_temp15_8x16b = _mm_max_epi16(res_temp15_8x16b, mm_min);

                /* to store the 1st 4 pixels res. */
                _mm_storel_epi64((__m128i *)(dst_copy), res_temp5_8x16b);
                _mm_storel_epi64((__m128i *)(dst_copy + dst_stride), res_temp15_8x16b);
                inp_copy += (src_stride << 1);  /* Pointer update */
                dst_copy += (dst_stride << 1);  /* Pointer update */
            }

            /*extra one height to be done*/
            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy));

            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 1));

            src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 2));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 3));

            src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 4));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 5));

            src_temp5_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
            res_temp3_8x16b = _mm_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);

            src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 6));
            src_temp2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + 7));

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

            _mm_storel_epi64((__m128i *)(dst_copy), res_temp5_8x16b);
        }

        rem_w &= 0x3;

        if (rem_w)
        {
            __m128i filt_coef;
            s16 sum, sum1;

            inp_copy = src_tmp + ((width / 4) * 4);
            dst_copy = pred + ((width / 4) * 4);

            filt_coef = _mm_loadu_si128((__m128i*)coeff);   //w0 w1 w2 w3 w4 w5 w6 w7

            for (row = 0; row < (height - 1); row += 2)
            {
                for (col = 0; col < rem_w; col++)
                {
                    src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + col));
                    src_temp5_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + col));

                    src_temp1_16x8b = _mm_madd_epi16(src_temp1_16x8b, filt_coef);
                    src_temp5_16x8b = _mm_madd_epi16(src_temp5_16x8b, filt_coef);

                    src_temp1_16x8b = _mm_hadd_epi32(src_temp1_16x8b, src_temp5_16x8b);
                    src_temp1_16x8b = _mm_hadd_epi32(src_temp1_16x8b, src_temp1_16x8b);

                    src_temp1_16x8b = _mm_add_epi32(src_temp1_16x8b, offset_8x16b);
                    src_temp1_16x8b = _mm_srai_epi32(src_temp1_16x8b, shift);
                    src_temp1_16x8b = _mm_packs_epi32(src_temp1_16x8b, src_temp1_16x8b);

                    sum = _mm_extract_epi16(src_temp1_16x8b, 0);
                    sum1 = _mm_extract_epi16(src_temp1_16x8b, 1);

                    //if (is_last)
                    {
                        sum = (sum < min_val) ? min_val : (sum > max_val ? max_val : sum);
                        sum1 = (sum1 < min_val) ? min_val : (sum1 > max_val ? max_val : sum1);
                    }
                    dst_copy[col] = (sum);
                    dst_copy[col + dst_stride] = (sum1);
                }
                inp_copy += (src_stride << 1);
                dst_copy += (dst_stride << 1);
            }

            for (col = 0; col < rem_w; col++)
            {
                src_temp1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + col));

                src_temp1_16x8b = _mm_madd_epi16(src_temp1_16x8b, filt_coef);

                src_temp1_16x8b = _mm_hadd_epi32(src_temp1_16x8b, src_temp1_16x8b);
                src_temp2_16x8b = _mm_srli_si128(src_temp1_16x8b, 4);
                src_temp1_16x8b = _mm_add_epi32(src_temp1_16x8b, src_temp2_16x8b);

                src_temp1_16x8b = _mm_add_epi32(src_temp1_16x8b, offset_8x16b);
                src_temp1_16x8b = _mm_srai_epi32(src_temp1_16x8b, shift);
                src_temp1_16x8b = _mm_packs_epi32(src_temp1_16x8b, src_temp1_16x8b);

                sum = (s16)_mm_extract_epi16(src_temp1_16x8b, 0);

                //if (is_last)
                {
                    sum = (sum < min_val) ? min_val : (sum > max_val ? max_val : sum);
                }
                dst_copy[col] = (sum);
            }
        }
    }
}

void mc_filter_l_8pel_horz_no_clip_sse(s16 *ref,
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

    if (rem_w > 7)
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
            for (col = 0; col < width; col += 8)
            {
                /*load 8 pixel values from row 0*/
                src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[col]));
                src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[col + 1]));

                src_temp3_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp7_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp1_8x16b = _mm_madd_epi16(src_temp3_16x8b, coeff0_1_8x16b);
                res_temp7_8x16b = _mm_madd_epi16(src_temp7_16x8b, coeff0_1_8x16b);
                /* row = 0 */
                src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[col + 2]));
                src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[col + 3]));

                src_temp4_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp8_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp2_8x16b = _mm_madd_epi16(src_temp4_16x8b, coeff2_3_8x16b);
                res_temp8_8x16b = _mm_madd_epi16(src_temp8_16x8b, coeff2_3_8x16b);

                src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[col + 4]));
                src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[col + 5]));

                src_temp5_16x8b = _mm_unpacklo_epi16(src_temp1_16x8b, src_temp2_16x8b);
                src_temp9_16x8b = _mm_unpackhi_epi16(src_temp1_16x8b, src_temp2_16x8b);
                res_temp3_8x16b = _mm_madd_epi16(src_temp5_16x8b, coeff4_5_8x16b);
                res_temp9_8x16b = _mm_madd_epi16(src_temp9_16x8b, coeff4_5_8x16b);

                src_temp1_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[col + 6]));
                src_temp2_16x8b = _mm_loadu_si128((__m128i*)(&inp_copy[col + 7]));

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
                _mm_storeu_si128((__m128i *)(dst_copy + col), res_temp5_8x16b);
            }
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

void mc_filter_l_8pel_vert_clip_sse(s16 *ref,
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

    __m128i coeff0_1_8x16b, coeff2_3_8x16b, coeff4_5_8x16b, coeff6_7_8x16b;
    __m128i r0_8x16b, r1_8x16b, r2_8x16b, r3_8x16b, r4_8x16b, r5_8x16b, r6_8x16b, r7_8x16b, r8_8x16b, r9_8x16b;
    __m128i r2_1r_16x8b, r2_2r_16x8b, r2_3r_16x8b, r2_4r_16x8b, r2_5r_16x8b, r2_6r_16x8b, r2_7r_16x8b, r2_8r_16x8b;
    __m128i r3_1r_16x8b, r3_2r_16x8b, r3_3r_16x8b, r3_4r_16x8b, r3_5r_16x8b, r3_6r_16x8b, r3_7r_16x8b, r3_8r_16x8b;

    __m128i mm_min = _mm_set1_epi16(min_val);
    __m128i mm_max = _mm_set1_epi16(max_val);
    __m128i offset_8x16b = _mm_set1_epi32(offset); /* for offset addition */

    src_tmp = ref;
    rem_w = width;
    inp_copy = ref;
    dst_copy = pred;

    /* load 8 8-bit coefficients and convert 8-bit into 16-bit  */
    coeff0_1_8x16b = _mm_loadu_si128((__m128i*)coeff);

    coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);
    coeff4_5_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xaa);
    coeff6_7_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0xff);
    coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);

    if (rem_w > 7)
    {
        for (row = 0; row < height; row++)
        {
            int cnt = 0;
            for (col = width; col > 7; col -= 8)
            {
                /*load 8 pixel values.*/
                r2_1r_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + cnt));

                /*load 8 pixel values*/
                r2_2r_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + cnt));

                r3_1r_16x8b = _mm_unpacklo_epi16(r2_1r_16x8b, r2_2r_16x8b);
                r3_5r_16x8b = _mm_unpackhi_epi16(r2_1r_16x8b, r2_2r_16x8b);

                r0_8x16b = _mm_madd_epi16(r3_1r_16x8b, coeff0_1_8x16b);
                r4_8x16b = _mm_madd_epi16(r3_5r_16x8b, coeff0_1_8x16b);
                /*load 8 pixel values*/
                r2_3r_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + (src_stride << 1) + cnt));

                /*load 8 pixel values*/
                r2_4r_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + (src_stride * 3) + cnt));

                r3_2r_16x8b = _mm_unpacklo_epi16(r2_3r_16x8b, r2_4r_16x8b);
                r3_6r_16x8b = _mm_unpackhi_epi16(r2_3r_16x8b, r2_4r_16x8b);

                r1_8x16b = _mm_madd_epi16(r3_2r_16x8b, coeff2_3_8x16b);
                r5_8x16b = _mm_madd_epi16(r3_6r_16x8b, coeff2_3_8x16b);

                /*load 8 pixel values*/
                r2_5r_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + (src_stride << 2) + cnt));

                /*load 8 pixel values*/
                r2_6r_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + (src_stride * 5) + cnt));

                r3_3r_16x8b = _mm_unpacklo_epi16(r2_5r_16x8b, r2_6r_16x8b);
                r3_7r_16x8b = _mm_unpackhi_epi16(r2_5r_16x8b, r2_6r_16x8b);

                r2_8x16b = _mm_madd_epi16(r3_3r_16x8b, coeff4_5_8x16b);
                r6_8x16b = _mm_madd_epi16(r3_7r_16x8b, coeff4_5_8x16b);

                /*load 8 pixel values*/
                r2_7r_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + (src_stride * 6) + cnt));

                /*load 8 pixel values*/
                r2_8r_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + (src_stride * 7) + cnt));

                r3_4r_16x8b = _mm_unpacklo_epi16(r2_7r_16x8b, r2_8r_16x8b);
                r3_8r_16x8b = _mm_unpackhi_epi16(r2_7r_16x8b, r2_8r_16x8b);

                r3_8x16b = _mm_madd_epi16(r3_4r_16x8b, coeff6_7_8x16b);
                r7_8x16b = _mm_madd_epi16(r3_8r_16x8b, coeff6_7_8x16b);

                r0_8x16b = _mm_add_epi32(r0_8x16b, r1_8x16b);
                r2_8x16b = _mm_add_epi32(r2_8x16b, r3_8x16b);
                r4_8x16b = _mm_add_epi32(r4_8x16b, r5_8x16b);
                r6_8x16b = _mm_add_epi32(r6_8x16b, r7_8x16b);
                r0_8x16b = _mm_add_epi32(r0_8x16b, r2_8x16b);
                r4_8x16b = _mm_add_epi32(r4_8x16b, r6_8x16b);

                r0_8x16b = _mm_add_epi32(r0_8x16b, offset_8x16b);
                r4_8x16b = _mm_add_epi32(r4_8x16b, offset_8x16b);

                r7_8x16b = _mm_srai_epi32(r0_8x16b, shift);
                r8_8x16b = _mm_srai_epi32(r4_8x16b, shift);

                /* i2_tmp = CLIP_U8(i2_tmp);*/
                r9_8x16b = _mm_packs_epi32(r7_8x16b, r8_8x16b);

                
                r9_8x16b = _mm_min_epi16(r9_8x16b, mm_max);
                r9_8x16b = _mm_max_epi16(r9_8x16b, mm_min);
                

                _mm_storeu_si128((__m128i*)(dst_copy + cnt), r9_8x16b);

                cnt += 8;
            }
            inp_copy += (src_stride);
            dst_copy += (dst_stride);
        }
    }

    rem_w &= 0x7;

    if (rem_w > 3)
    {
        inp_copy = src_tmp + ((width / 8) * 8);
        dst_copy = pred + ((width / 8) * 8);

        for (row = 0; row < height; row++)
        {
            /*load 4 pixel values */
            r2_1r_16x8b = _mm_loadl_epi64((__m128i*)(inp_copy));

            /*load 4 pixel values */
            r2_2r_16x8b = _mm_loadl_epi64((__m128i*)(inp_copy + (src_stride)));
			
			/*load 4 pixel values*/
            r2_3r_16x8b = _mm_loadl_epi64((__m128i*)(inp_copy + (src_stride << 1)));


            r3_1r_16x8b = _mm_unpacklo_epi16(r2_1r_16x8b, r2_2r_16x8b);

            r0_8x16b = _mm_madd_epi16(r3_1r_16x8b, coeff0_1_8x16b);
            
            /*load 4 pixel values*/
            r2_4r_16x8b = _mm_loadl_epi64((__m128i*)(inp_copy + (3 * src_stride)));

            r3_2r_16x8b = _mm_unpacklo_epi16(r2_3r_16x8b, r2_4r_16x8b);

            r1_8x16b = _mm_madd_epi16(r3_2r_16x8b, coeff2_3_8x16b);
            /*load 4 pixel values*/
            r2_5r_16x8b = _mm_loadl_epi64((__m128i*)(inp_copy + (src_stride << 2)));

            /*load 4 pixel values*/
            r2_6r_16x8b = _mm_loadl_epi64((__m128i*)(inp_copy + (5 * src_stride)));

            r3_3r_16x8b = _mm_unpacklo_epi16(r2_5r_16x8b, r2_6r_16x8b);

            r2_8x16b = _mm_madd_epi16(r3_3r_16x8b, coeff4_5_8x16b);
            /*load 4 pixel values*/
            r2_7r_16x8b = _mm_loadl_epi64((__m128i*)(inp_copy + (6 * src_stride)));

            /*load 4 pixel values*/
            r2_8r_16x8b = _mm_loadl_epi64((__m128i*)(inp_copy + (7 * src_stride)));

            r3_4r_16x8b = _mm_unpacklo_epi16(r2_7r_16x8b, r2_8r_16x8b);

            r3_8x16b = _mm_madd_epi16(r3_4r_16x8b, coeff6_7_8x16b);

            r4_8x16b = _mm_add_epi32(r0_8x16b, r1_8x16b);
            r5_8x16b = _mm_add_epi32(r2_8x16b, r3_8x16b);
            r6_8x16b = _mm_add_epi32(r4_8x16b, r5_8x16b);

            r7_8x16b = _mm_add_epi32(r6_8x16b, offset_8x16b);

            /*(i2_tmp + OFFSET_14_MINUS_BIT_DEPTH) >> SHIFT_14_MINUS_BIT_DEPTH */
            r8_8x16b = _mm_srai_epi32(r7_8x16b, shift);

            /* i2_tmp = CLIP_U8(i2_tmp);*/
            r9_8x16b = _mm_packs_epi32(r8_8x16b, r8_8x16b);

            
            r9_8x16b = _mm_min_epi16(r9_8x16b, mm_max);
            r9_8x16b = _mm_max_epi16(r9_8x16b, mm_min);
           
            _mm_storel_epi64((__m128i*)(dst_copy), r9_8x16b);

            inp_copy += (src_stride);
            dst_copy += (dst_stride);
        }
    }

    rem_w &= 0x3;

    if (rem_w)
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
                sum += inp_copy[col + 4 * src_stride] * coeff[4];
                sum += inp_copy[col + 5 * src_stride] * coeff[5];
                sum += inp_copy[col + 6 * src_stride] * coeff[6];
                sum += inp_copy[col + 7 * src_stride] * coeff[7];

                val = (sum + offset) >> shift;
                val = XEVE_CLIP3(min_val, max_val, val);
                
                dst_copy[col] = val;
            }

            inp_copy += src_stride;
            dst_copy += dst_stride;
        }
    }
}


void mc_filter_c_4pel_horz_sse(s16 *ref, int src_stride, s16 *pred, int dst_stride, const s16 *coeff
                             , int width, int height, int min_val, int max_val, int offset, int shift, s8  is_last)
{
    int row, col, rem_w, rem_h, cnt;
    int src_stride2, src_stride3;
    s16 *inp_copy;
    s16 *dst_copy;

    /* all 128 bit registers are named with a suffix mxnb, where m is the */
    /* number of n bits packed in the register                            */

    __m128i offset_4x32b = _mm_set1_epi32(offset);
    __m128i    mm_min = _mm_set1_epi16(min_val);
    __m128i mm_max = _mm_set1_epi16(max_val);
    __m128i coeff0_1_8x16b, coeff2_3_8x16b, mm_mask;

    __m128i res0, res1, res2, res3;
    __m128i row11, row12, row13, row14, row21, row22, row23, row24;
    __m128i row31, row32, row33, row34, row41, row42, row43, row44;

    src_stride2 = (src_stride << 1);
    src_stride3 = (src_stride * 3);

    rem_w = width;
    inp_copy = ref;
    dst_copy = pred;

    /* load 8 8-bit coefficients and convert 8-bit into 16-bit  */
    coeff0_1_8x16b = _mm_loadu_si128((__m128i*)coeff);

    {
        rem_h = height & 0x3;
        rem_w = width;

        coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);  /*w2 w3 w2 w3 w2 w3 w2 w3*/
        coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);        /*w0 w1 w0 w1 w0 w1 w0 w1*/

        /* 8 pixels at a time */
        if (rem_w > 7)
        {
            cnt = 0;
            for (row = 0; row < height; row += 4)
            {
                for (col = width; col > 7; col -= 8)
                {
                    /*load pixel values from row 1*/
                    row11 = _mm_loadu_si128((__m128i*)(inp_copy + cnt));            /*a0 a1 a2 a3 a4 a5 a6 a7*/
                    row12 = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 1));        /*a1 a2 a3 a4 a5 a6 a7 a8*/
                    row13 = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 2));       /*a2 a3 a4 a5 a6 a7 a8 a9*/
                    row14 = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 3));        /*a3 a4 a5 a6 a7 a8 a9 a10*/
                                                                                    /*load pixel values from row 2*/
                    row21 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + cnt));
                    row22 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + cnt + 1));
                    row23 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + cnt + 2));
                    row24 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + cnt + 3));

                    /*load pixel values from row 3*/
                    row31 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride2 + cnt));
                    row32 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride2 + cnt + 1));
                    row33 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride2 + cnt + 2));
                    row34 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride2 + cnt + 3));
                    /*load pixel values from row 4*/
                    row41 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride3 + cnt));
                    row42 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride3 + cnt + 1));
                    row43 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride3 + cnt + 2));
                    row44 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride3 + cnt + 3));

                    row11 = _mm_madd_epi16(row11, coeff0_1_8x16b);    /*a0+a1 a2+a3 a4+a5 a6+a7*/
                    row12 = _mm_madd_epi16(row12, coeff0_1_8x16b);       /*a1+a2 a3+a4 a5+a6 a7+a8*/
                    row13 = _mm_madd_epi16(row13, coeff2_3_8x16b);       /*a2+a3 a4+a5 a6+a7 a8+a9*/
                    row14 = _mm_madd_epi16(row14, coeff2_3_8x16b);       /*a3+a4 a5+a6 a7+a8 a9+a10*/
                    row21 = _mm_madd_epi16(row21, coeff0_1_8x16b);
                    row22 = _mm_madd_epi16(row22, coeff0_1_8x16b);
                    row23 = _mm_madd_epi16(row23, coeff2_3_8x16b);
                    row24 = _mm_madd_epi16(row24, coeff2_3_8x16b);
                    row31 = _mm_madd_epi16(row31, coeff0_1_8x16b);
                    row32 = _mm_madd_epi16(row32, coeff0_1_8x16b);
                    row33 = _mm_madd_epi16(row33, coeff2_3_8x16b);
                    row34 = _mm_madd_epi16(row34, coeff2_3_8x16b);
                    row41 = _mm_madd_epi16(row41, coeff0_1_8x16b);
                    row42 = _mm_madd_epi16(row42, coeff0_1_8x16b);
                    row43 = _mm_madd_epi16(row43, coeff2_3_8x16b);
                    row44 = _mm_madd_epi16(row44, coeff2_3_8x16b);

                    row11 = _mm_add_epi32(row11, row13);
                    row12 = _mm_add_epi32(row12, row14);
                    row21 = _mm_add_epi32(row21, row23);
                    row22 = _mm_add_epi32(row22, row24);
                    row31 = _mm_add_epi32(row31, row33);
                    row32 = _mm_add_epi32(row32, row34);
                    row41 = _mm_add_epi32(row41, row43);
                    row42 = _mm_add_epi32(row42, row44);

                    row11 = _mm_add_epi32(row11, offset_4x32b);
                    row12 = _mm_add_epi32(row12, offset_4x32b);
                    row21 = _mm_add_epi32(row21, offset_4x32b);
                    row22 = _mm_add_epi32(row22, offset_4x32b);
                    row31 = _mm_add_epi32(row31, offset_4x32b);
                    row32 = _mm_add_epi32(row32, offset_4x32b);
                    row41 = _mm_add_epi32(row41, offset_4x32b);
                    row42 = _mm_add_epi32(row42, offset_4x32b);

                    row11 = _mm_srai_epi32(row11, shift);
                    row12 = _mm_srai_epi32(row12, shift);
                    row21 = _mm_srai_epi32(row21, shift);
                    row22 = _mm_srai_epi32(row22, shift);
                    row31 = _mm_srai_epi32(row31, shift);
                    row32 = _mm_srai_epi32(row32, shift);
                    row41 = _mm_srai_epi32(row41, shift);
                    row42 = _mm_srai_epi32(row42, shift);

                    row11 = _mm_packs_epi32(row11, row21);
                    row12 = _mm_packs_epi32(row12, row22);
                    row31 = _mm_packs_epi32(row31, row41);
                    row32 = _mm_packs_epi32(row32, row42);

                    res0 = _mm_unpacklo_epi16(row11, row12);
                    res1 = _mm_unpackhi_epi16(row11, row12);
                    res2 = _mm_unpacklo_epi16(row31, row32);
                    res3 = _mm_unpackhi_epi16(row31, row32);

                    if (is_last)
                    {
                        mm_mask = _mm_cmpgt_epi16(res0, mm_min);  /*if gt = -1...  -1 -1 0 0 -1 */
                        res0 = _mm_or_si128(_mm_and_si128(mm_mask, res0), _mm_andnot_si128(mm_mask, mm_min));
                        mm_mask = _mm_cmplt_epi16(res0, mm_max);
                        res0 = _mm_or_si128(_mm_and_si128(mm_mask, res0), _mm_andnot_si128(mm_mask, mm_max));

                        mm_mask = _mm_cmpgt_epi16(res1, mm_min);  /*if gt = -1...  -1 -1 0 0 -1 */
                        res1 = _mm_or_si128(_mm_and_si128(mm_mask, res1), _mm_andnot_si128(mm_mask, mm_min));
                        mm_mask = _mm_cmplt_epi16(res1, mm_max);
                        res1 = _mm_or_si128(_mm_and_si128(mm_mask, res1), _mm_andnot_si128(mm_mask, mm_max));

                        mm_mask = _mm_cmpgt_epi16(res2, mm_min);  /*if gt = -1...  -1 -1 0 0 -1 */
                        res2 = _mm_or_si128(_mm_and_si128(mm_mask, res2), _mm_andnot_si128(mm_mask, mm_min));
                        mm_mask = _mm_cmplt_epi16(res2, mm_max);
                        res2 = _mm_or_si128(_mm_and_si128(mm_mask, res2), _mm_andnot_si128(mm_mask, mm_max));

                        mm_mask = _mm_cmpgt_epi16(res3, mm_min);  /*if gt = -1...  -1 -1 0 0 -1 */
                        res3 = _mm_or_si128(_mm_and_si128(mm_mask, res3), _mm_andnot_si128(mm_mask, mm_min));
                        mm_mask = _mm_cmplt_epi16(res3, mm_max);
                        res3 = _mm_or_si128(_mm_and_si128(mm_mask, res3), _mm_andnot_si128(mm_mask, mm_max));
                    }
                    /* to store the 8 pixels res. */
                    _mm_storeu_si128((__m128i *)(dst_copy + cnt), res0);
                    _mm_storeu_si128((__m128i *)(dst_copy + dst_stride + cnt), res1);
                    _mm_storeu_si128((__m128i *)(dst_copy + (dst_stride << 1) + cnt), res2);
                    _mm_storeu_si128((__m128i *)(dst_copy + (dst_stride * 3) + cnt), res3);

                    cnt += 8;
                }

                cnt = 0;
                inp_copy += (src_stride << 2); /* pointer updates*/
                dst_copy += (dst_stride << 2); /* pointer updates*/
            }

            /*remaining ht */
            for (row = 0; row < rem_h; row++)
            {
                cnt = 0;
                for (col = width; col > 7; col -= 8)
                {
                    /*load pixel values from row 1*/
                    row11 = _mm_loadu_si128((__m128i*)(inp_copy + cnt));            /*a0 a1 a2 a3 a4 a5 a6 a7*/
                    row12 = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 1));        /*a1 a2 a3 a4 a5 a6 a7 a8*/
                    row13 = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 2));       /*a2 a3 a4 a5 a6 a7 a8 a9*/
                    row14 = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 3));        /*a3 a4 a5 a6 a7 a8 a9 a10*/

                    row11 = _mm_madd_epi16(row11, coeff0_1_8x16b);    /*a0+a1 a2+a3 a4+a5 a6+a7*/
                    row12 = _mm_madd_epi16(row12, coeff0_1_8x16b);       /*a1+a2 a3+a4 a5+a6 a7+a8*/
                    row13 = _mm_madd_epi16(row13, coeff2_3_8x16b);       /*a2+a3 a4+a5 a6+a7 a8+a9*/
                    row14 = _mm_madd_epi16(row14, coeff2_3_8x16b);       /*a3+a4 a5+a6 a7+a8 a9+a10*/

                    row11 = _mm_add_epi32(row11, row13); /*a0+a1+a2+a3 a2+a3+a4+a5 a4+a5+a6+a7 a6+a7+a8+a9*/
                    row12 = _mm_add_epi32(row12, row14); /*a1+a2+a3+a4 a3+a4+a5+a6 a5+a6+a7+a8 a7+a8+a9+a10*/

                    row11 = _mm_add_epi32(row11, offset_4x32b);
                    row12 = _mm_add_epi32(row12, offset_4x32b);

                    row11 = _mm_srai_epi32(row11, shift);
                    row12 = _mm_srai_epi32(row12, shift);

                    row11 = _mm_packs_epi32(row11, row12);

                    res3 = _mm_unpackhi_epi64(row11, row11);
                    res0 = _mm_unpacklo_epi16(row11, res3);

                    if (is_last)
                    {
                        mm_mask = _mm_cmpgt_epi16(res0, mm_min);  /*if gt = -1...  -1 -1 0 0 -1 */
                        res0 = _mm_or_si128(_mm_and_si128(mm_mask, res0), _mm_andnot_si128(mm_mask, mm_min));
                        mm_mask = _mm_cmplt_epi16(res0, mm_max);
                        res0 = _mm_or_si128(_mm_and_si128(mm_mask, res0), _mm_andnot_si128(mm_mask, mm_max));
                    }

                    /* to store the 8 pixels res. */
                    _mm_storeu_si128((__m128i *)(dst_copy + cnt), res0);

                    cnt += 8;
                }
                inp_copy += (src_stride); /* pointer updates*/
                dst_copy += (dst_stride); /* pointer updates*/
            }
        }

        rem_w &= 0x7;

        /* one 4 pixel wd for multiple rows */
        if (rem_w > 3)
        {
            inp_copy = ref + ((width / 8) * 8);
            dst_copy = pred + ((width / 8) * 8);

            for (row = 0; row < height; row += 4)
            {
                /*load pixel values from row 1*/
                row11 = _mm_loadl_epi64((__m128i*)(inp_copy));            /*a0 a1 a2 a3 a4 a5 a6 a7*/
                row12 = _mm_loadl_epi64((__m128i*)(inp_copy + 1));        /*a1 a2 a3 a4 a5 a6 a7 a8*/
                row13 = _mm_loadl_epi64((__m128i*)(inp_copy + 2));       /*a2 a3 a4 a5 a6 a7 a8 a9*/
                row14 = _mm_loadl_epi64((__m128i*)(inp_copy + 3));        /*a3 a4 a5 a6 a7 a8 a9 a10*/
                                                                        /*load pixel values from row 2*/
                row21 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride));
                row22 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride + 1));
                row23 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride + 2));
                row24 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride + 3));

                /*load pixel values from row 3*/
                row31 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride2));
                row32 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride2 + 1));
                row33 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride2 + 2));
                row34 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride2 + 3));
                /*load pixel values from row 4*/
                row41 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride3));
                row42 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride3 + 1));
                row43 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride3 + 2));
                row44 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride3 + 3));

                row11 = _mm_unpacklo_epi32(row11, row12);
                row13 = _mm_unpacklo_epi32(row13, row14);
                row21 = _mm_unpacklo_epi32(row21, row22);
                row23 = _mm_unpacklo_epi32(row23, row24);
                row31 = _mm_unpacklo_epi32(row31, row32);
                row33 = _mm_unpacklo_epi32(row33, row34);
                row41 = _mm_unpacklo_epi32(row41, row42);
                row43 = _mm_unpacklo_epi32(row43, row44);

                row11 = _mm_madd_epi16(row11, coeff0_1_8x16b);
                row13 = _mm_madd_epi16(row13, coeff2_3_8x16b);
                row21 = _mm_madd_epi16(row21, coeff0_1_8x16b);
                row23 = _mm_madd_epi16(row23, coeff2_3_8x16b);
                row31 = _mm_madd_epi16(row31, coeff0_1_8x16b);
                row33 = _mm_madd_epi16(row33, coeff2_3_8x16b);
                row41 = _mm_madd_epi16(row41, coeff0_1_8x16b);
                row43 = _mm_madd_epi16(row43, coeff2_3_8x16b);

                row11 = _mm_add_epi32(row11, row13);
                row21 = _mm_add_epi32(row21, row23);
                row31 = _mm_add_epi32(row31, row33);
                row41 = _mm_add_epi32(row41, row43);

                row11 = _mm_add_epi32(row11, offset_4x32b);
                row21 = _mm_add_epi32(row21, offset_4x32b);
                row31 = _mm_add_epi32(row31, offset_4x32b);
                row41 = _mm_add_epi32(row41, offset_4x32b);

                row11 = _mm_srai_epi32(row11, shift);
                row21 = _mm_srai_epi32(row21, shift);
                row31 = _mm_srai_epi32(row31, shift);
                row41 = _mm_srai_epi32(row41, shift);

                res0 = _mm_packs_epi32(row11, row21);
                res1 = _mm_packs_epi32(row31, row41);

                if (is_last)
                {
                    mm_mask = _mm_cmpgt_epi16(res0, mm_min);  /*if gt = -1...  -1 -1 0 0 -1 */
                    res0 = _mm_or_si128(_mm_and_si128(mm_mask, res0), _mm_andnot_si128(mm_mask, mm_min));
                    mm_mask = _mm_cmplt_epi16(res0, mm_max);
                    res0 = _mm_or_si128(_mm_and_si128(mm_mask, res0), _mm_andnot_si128(mm_mask, mm_max));

                    mm_mask = _mm_cmpgt_epi16(res1, mm_min);  /*if gt = -1...  -1 -1 0 0 -1 */
                    res1 = _mm_or_si128(_mm_and_si128(mm_mask, res1), _mm_andnot_si128(mm_mask, mm_min));
                    mm_mask = _mm_cmplt_epi16(res1, mm_max);
                    res1 = _mm_or_si128(_mm_and_si128(mm_mask, res1), _mm_andnot_si128(mm_mask, mm_max));
                }
                /* to store the 8 pixels res. */
                _mm_storel_epi64((__m128i *)(dst_copy), res0);
                _mm_storel_epi64((__m128i *)(dst_copy + dst_stride), _mm_unpackhi_epi64(res0, res0));
                _mm_storel_epi64((__m128i *)(dst_copy + (dst_stride << 1)), res1);
                _mm_storel_epi64((__m128i *)(dst_copy + (dst_stride * 3)), _mm_unpackhi_epi64(res1, res1));

                inp_copy += (src_stride << 2); /* pointer updates*/
                dst_copy += (dst_stride << 2); /* pointer updates*/
            }

            for (row = 0; row < rem_h; row++)
            {
                /*load pixel values from row 1*/
                row11 = _mm_loadl_epi64((__m128i*)(inp_copy));            /*a0 a1 a2 a3 a4 a5 a6 a7*/
                row12 = _mm_loadl_epi64((__m128i*)(inp_copy + 1));        /*a1 a2 a3 a4 a5 a6 a7 a8*/
                row13 = _mm_loadl_epi64((__m128i*)(inp_copy + 2));       /*a2 a3 a4 a5 a6 a7 a8 a9*/
                row14 = _mm_loadl_epi64((__m128i*)(inp_copy + 3));        /*a3 a4 a5 a6 a7 a8 a9 a10*/

                row11 = _mm_unpacklo_epi32(row11, row12);        /*a0 a1 a1 a2 a2 a3 a3 a4*/
                row13 = _mm_unpacklo_epi32(row13, row14);        /*a2 a3 a3 a4 a4 a5 a5 a6*/

                row11 = _mm_madd_epi16(row11, coeff0_1_8x16b);    /*a0+a1 a1+a2 a2+a3 a3+a4*/
                row13 = _mm_madd_epi16(row13, coeff2_3_8x16b);       /*a2+a3 a3+a4 a4+a5 a5+a6*/

                row11 = _mm_add_epi32(row11, row13);    /*r00 r01  r02  r03*/

                row11 = _mm_add_epi32(row11, offset_4x32b);

                row11 = _mm_srai_epi32(row11, shift);

                res1 = _mm_packs_epi32(row11, row11);

                if (is_last)
                {
                    mm_mask = _mm_cmpgt_epi16(res1, mm_min);  /*if gt = -1...  -1 -1 0 0 -1 */
                    res1 = _mm_or_si128(_mm_and_si128(mm_mask, res1), _mm_andnot_si128(mm_mask, mm_min));
                    mm_mask = _mm_cmplt_epi16(res1, mm_max);
                    res1 = _mm_or_si128(_mm_and_si128(mm_mask, res1), _mm_andnot_si128(mm_mask, mm_max));
                }
                /* to store the 8 pixels res. */
                _mm_storel_epi64((__m128i *)(dst_copy), res1);

                inp_copy += (src_stride); /* pointer updates*/
                dst_copy += (dst_stride); /* pointer updates*/
            }
        }

        rem_w &= 0x3;
        if (rem_w)
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
}

void mc_filter_c_4pel_vert_sse(s16 *ref,
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

    __m128i coeff0_1_8x16b, coeff2_3_8x16b, mm_mask;
    __m128i s0_8x16b, s1_8x16b, s4_8x16b, s5_8x16b, s7_8x16b, s8_8x16b, s9_8x16b;
    __m128i s2_0_16x8b, s2_1_16x8b, s2_2_16x8b, s2_3_16x8b;
    __m128i s3_0_16x8b, s3_1_16x8b, s3_4_16x8b, s3_5_16x8b;

    __m128i mm_min = _mm_set1_epi16(min_val);
    __m128i mm_max = _mm_set1_epi16(max_val);
    __m128i offset_8x16b = _mm_set1_epi32(offset); /* for offset addition */

    src_tmp = ref;
    rem_w = width;
    inp_copy = ref;
    dst_copy = pred;

    /* load 8 8-bit coefficients and convert 8-bit into 16-bit  */
    coeff0_1_8x16b = _mm_loadu_si128((__m128i*)coeff);
    coeff2_3_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0x55);
    coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);

    if (rem_w > 7)
    {
        for (row = 0; row < height; row++)
        {
            int cnt = 0;
            for (col = width; col > 7; col -= 8)
            {
                /* a0 a1 a2 a3 a4 a5 a6 a7 */
                s2_0_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + cnt));
                /* b0 b1 b2 b3 b4 b5 b6 b7 */
                s2_1_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + cnt));
                /* a0 b0 a1 b1 a2 b2 a3 b3 */
                s3_0_16x8b = _mm_unpacklo_epi16(s2_0_16x8b, s2_1_16x8b);
                /* a4 b4 ... a7 b7 */
                s3_4_16x8b = _mm_unpackhi_epi16(s2_0_16x8b, s2_1_16x8b);
                /* a0+b0 a1+b1 a2+b2 a3+b3*/
                s0_8x16b = _mm_madd_epi16(s3_0_16x8b, coeff0_1_8x16b);
                s4_8x16b = _mm_madd_epi16(s3_4_16x8b, coeff0_1_8x16b);

                /* c0 c1 c2 c3 c4 c5 c6 c7 */
                s2_2_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + (src_stride << 1) + cnt));
                /* d0 d1 d2 d3 d4 d5 d6 d7 */
                s2_3_16x8b = _mm_loadu_si128((__m128i*)(inp_copy + (src_stride * 3) + cnt));
                /* c0 d0 c1 d1 c2 d2 c3 d3 */
                s3_1_16x8b = _mm_unpacklo_epi16(s2_2_16x8b, s2_3_16x8b);
                s3_5_16x8b = _mm_unpackhi_epi16(s2_2_16x8b, s2_3_16x8b);
                /* c0+d0 c1+d1 c2+d2 c3+d3*/
                s1_8x16b = _mm_madd_epi16(s3_1_16x8b, coeff2_3_8x16b);
                s5_8x16b = _mm_madd_epi16(s3_5_16x8b, coeff2_3_8x16b);

                /* a0+b0+c0+d0 ... a3+b3+c3+d3 */
                s0_8x16b = _mm_add_epi32(s0_8x16b, s1_8x16b);
                /* a4+b4+c4+d4 ... a7+b7+c7+d7 */
                s4_8x16b = _mm_add_epi32(s4_8x16b, s5_8x16b);

                s0_8x16b = _mm_add_epi32(s0_8x16b, offset_8x16b);
                s4_8x16b = _mm_add_epi32(s4_8x16b, offset_8x16b);

                s7_8x16b = _mm_srai_epi32(s0_8x16b, shift);
                s8_8x16b = _mm_srai_epi32(s4_8x16b, shift);

                s9_8x16b = _mm_packs_epi32(s7_8x16b, s8_8x16b);

                if (is_last)
                {
                    mm_mask = _mm_cmpgt_epi16(s9_8x16b, mm_min);  /*if gt = -1...  -1 -1 0 0 -1 */
                    s9_8x16b = _mm_or_si128(_mm_and_si128(mm_mask, s9_8x16b), _mm_andnot_si128(mm_mask, mm_min));
                    mm_mask = _mm_cmplt_epi16(s9_8x16b, mm_max);
                    s9_8x16b = _mm_or_si128(_mm_and_si128(mm_mask, s9_8x16b), _mm_andnot_si128(mm_mask, mm_max));
                }

                _mm_storeu_si128((__m128i*)(dst_copy + cnt), s9_8x16b);

                cnt += 8;
            }
            inp_copy += (src_stride);
            dst_copy += (dst_stride);
        }
    }

    rem_w &= 0x7;

    if (rem_w > 3)
    {
        inp_copy = src_tmp + ((width / 8) * 8);
        dst_copy = pred + ((width / 8) * 8);

        for (row = 0; row < height; row++)
        {
            /*load 4 pixel values */
            s2_0_16x8b = _mm_loadl_epi64((__m128i*)(inp_copy));
            /*load 4 pixel values */
            s2_1_16x8b = _mm_loadl_epi64((__m128i*)(inp_copy + (src_stride)));

            s3_0_16x8b = _mm_unpacklo_epi16(s2_0_16x8b, s2_1_16x8b);
            s0_8x16b = _mm_madd_epi16(s3_0_16x8b, coeff0_1_8x16b);

            /*load 4 pixel values*/
            s2_2_16x8b = _mm_loadl_epi64((__m128i*)(inp_copy + (2 * src_stride)));
            /*load 4 pixel values*/
            s2_3_16x8b = _mm_loadl_epi64((__m128i*)(inp_copy + (3 * src_stride)));

            s3_1_16x8b = _mm_unpacklo_epi16(s2_2_16x8b, s2_3_16x8b);
            s1_8x16b = _mm_madd_epi16(s3_1_16x8b, coeff2_3_8x16b);

            s4_8x16b = _mm_add_epi32(s0_8x16b, s1_8x16b);

            s7_8x16b = _mm_add_epi32(s4_8x16b, offset_8x16b);
            s8_8x16b = _mm_srai_epi32(s7_8x16b, shift);

            s9_8x16b = _mm_packs_epi32(s8_8x16b, s8_8x16b);

            if (is_last)
            {
                mm_mask = _mm_cmpgt_epi16(s9_8x16b, mm_min);  /*if gt = -1...  -1 -1 0 0 -1 */
                s9_8x16b = _mm_or_si128(_mm_and_si128(mm_mask, s9_8x16b), _mm_andnot_si128(mm_mask, mm_min));
                mm_mask = _mm_cmplt_epi16(s9_8x16b, mm_max);
                s9_8x16b = _mm_or_si128(_mm_and_si128(mm_mask, s9_8x16b), _mm_andnot_si128(mm_mask, mm_max));
            }
            _mm_storel_epi64((__m128i*)(dst_copy), s9_8x16b);

            inp_copy += (src_stride);
            dst_copy += (dst_stride);
        }
    }

    rem_w &= 0x3;

    if (rem_w)
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

void xeve_mc_l_00_sse(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int i, j;
    gmv_x >>= 4;
    gmv_y >>= 4;
    ref += gmv_y * s_ref + gmv_x;

    if (((w & 0x7) == 0) && ((h & 1) == 0))
    {
        __m128i m00, m01;

        for (i = 0; i < h; i += 2)
        {
            for (j = 0; j < w; j += 8)
            {
                m00 = _mm_loadu_si128((__m128i*)(ref + j));
                m01 = _mm_loadu_si128((__m128i*)(ref + j + s_ref));

                _mm_storeu_si128((__m128i*)(pred + j), m00);
                _mm_storeu_si128((__m128i*)(pred + j + s_pred), m01);
            }
            pred += s_pred * 2;
            ref += s_ref * 2;
        }
    }
    else if ((w & 0x3) == 0)
    {
        __m128i m00;

        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j += 4)
            {
                m00 = _mm_loadl_epi64((__m128i*)(ref + j));
                _mm_storel_epi64((__m128i*)(pred + j), m00);
            }
            pred += s_pred;
            ref += s_ref;
        }
    }
    else
    {
        for (i = 0; i < h; i++)
        {
            for (j = 0; j < w; j++)
            {
                pred[j] = ref[j];
            }
            pred += s_pred;
            ref += s_ref;
        }
    }
}

void xeve_mc_l_n0_sse(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int dx = gmv_x & 15;
    ref += (gmv_y >> 4) * s_ref + (gmv_x >> 4) - 3;

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    mc_filter_l_8pel_horz_clip_sse(ref, s_ref, pred, s_pred, tbl_mc_l_coeff[dx], w, h, min, max, MAC_ADD_N0, MAC_SFT_N0);
}

void xeve_mc_l_0n_sse(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int dy = gmv_y & 15;
    ref += ((gmv_y >> 4) - 3) * s_ref + (gmv_x >> 4);

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    mc_filter_l_8pel_vert_clip_sse(ref, s_ref, pred, s_pred, tbl_mc_l_coeff[dy], w, h, min, max, MAC_ADD_0N, MAC_SFT_0N);
}

void xeve_mc_l_nn_sse(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
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

    mc_filter_l_8pel_horz_no_clip_sse(ref, s_ref, buf, w, tbl_mc_l_coeff[dx], w, (h + 7), offset1, shift1);
    mc_filter_l_8pel_vert_clip_sse(buf, w, pred, s_pred, tbl_mc_l_coeff[dy], w, h, min, max, offset2, shift2);    
}


/****************************************************************************
 * motion compensation for chroma
 ****************************************************************************/
void xeve_mc_c_n0_sse(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    int  dx = gmv_x & 31;
    ref += (gmv_y >> 5) * s_ref + (gmv_x >> 5) - 1;

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    mc_filter_c_4pel_horz_sse(ref, s_ref, pred, s_pred, tbl_mc_c_coeff[dx], w, h, min, max, MAC_ADD_N0, MAC_SFT_N0, 1);
}

void xeve_mc_c_0n_sse(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    int dy = gmv_y & 31;
    ref += ((gmv_y >> 5) - 1) * s_ref + (gmv_x >> 5);
    
    int max = ((1 << bit_depth) - 1);
    int min = 0;

    mc_filter_c_4pel_vert_sse(ref, s_ref, pred, s_pred, tbl_mc_c_coeff[dy], w, h, min, max, MAC_ADD_0N, MAC_SFT_0N, 1);
}

void xeve_mc_c_nn_sse(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
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

    mc_filter_c_4pel_horz_sse(ref, s_ref, buf, w, tbl_mc_c_coeff[dx], w, (h + 3), min, max, offset1, shift1, 0);
    mc_filter_c_4pel_vert_sse(buf, w, pred, s_pred, tbl_mc_c_coeff[dy], w, h, min, max, offset2, shift2, 1);
}

XEVE_MC_L xeve_tbl_mc_l_sse[2][2] =
{
    {
        xeve_mc_l_00_sse, /* dx == 0 && dy == 0 */
        xeve_mc_l_0n_sse  /* dx == 0 && dy != 0 */
    },
    {
        xeve_mc_l_n0_sse, /* dx != 0 && dy == 0 */
        xeve_mc_l_nn_sse  /* dx != 0 && dy != 0 */
    }
};

XEVE_MC_C xeve_tbl_mc_c_sse[2][2] =
{
    {
        xeve_mc_c_00, /* dx == 0 && dy == 0 */
        xeve_mc_c_0n_sse  /* dx == 0 && dy != 0 */
    },
    {
        xeve_mc_c_n0_sse, /* dx != 0 && dy == 0 */
        xeve_mc_c_nn_sse  /* dx != 0 && dy != 0 */
    }
};


void xeve_average_16b_no_clip_sse(s16 *src, s16 *ref, s16 *dst, int s_src, int s_ref, int s_dst, int wd, int ht)
{
    s16 *p0, *p1, *p2;
    int rem_h = ht;
    int rem_w;
    int i, j;

    __m128i src_8x16b, src_8x16b_1, src_8x16b_2, src_8x16b_3;
    __m128i pred_8x16b, pred_8x16b_1, pred_8x16b_2, pred_8x16b_3;
    __m128i temp_0, temp_1, temp_2, temp_3;
    __m128i offset_8x16b;

    /* Can be changed for a generic avg fun. or taken as an argument! */
    int offset = 1;
    int shift = 1;

    p0 = src;
    p1 = ref;
    p2 = dst;

    offset_8x16b = _mm_set1_epi16(offset);

    /* Mult. of 4 Loop */
    if (rem_h >= 4)
    {
        for (i = 0; i < rem_h; i += 4)
        {
            p0 = src + (i * s_src);
            p1 = ref + (i * s_ref);
            p2 = dst + (i * s_dst);

            rem_w = wd;

            /* Mult. of 8 Loop */
            if (rem_w >= 8)
            {
                for (j = 0; j < rem_w; j += 8)
                {
                    src_8x16b = _mm_loadu_si128((__m128i *) (p0));
                    src_8x16b_1 = _mm_loadu_si128((__m128i *) (p0 + s_src));
                    src_8x16b_2 = _mm_loadu_si128((__m128i *) (p0 + (s_src * 2)));
                    src_8x16b_3 = _mm_loadu_si128((__m128i *) (p0 + (s_src * 3)));

                    pred_8x16b = _mm_loadu_si128((__m128i *) (p1));
                    pred_8x16b_1 = _mm_loadu_si128((__m128i *) (p1 + s_ref));
                    pred_8x16b_2 = _mm_loadu_si128((__m128i *) (p1 + (s_ref * 2)));
                    pred_8x16b_3 = _mm_loadu_si128((__m128i *) (p1 + (s_ref * 3)));

                    temp_0 = _mm_add_epi16(src_8x16b, pred_8x16b);
                    temp_1 = _mm_add_epi16(src_8x16b_1, pred_8x16b_1);
                    temp_2 = _mm_add_epi16(src_8x16b_2, pred_8x16b_2);
                    temp_3 = _mm_add_epi16(src_8x16b_3, pred_8x16b_3);

                    temp_0 = _mm_add_epi16(temp_0, offset_8x16b);
                    temp_1 = _mm_add_epi16(temp_1, offset_8x16b);
                    temp_2 = _mm_add_epi16(temp_2, offset_8x16b);
                    temp_3 = _mm_add_epi16(temp_3, offset_8x16b);

                    temp_0 = _mm_srai_epi16(temp_0, shift);
                    temp_1 = _mm_srai_epi16(temp_1, shift);
                    temp_2 = _mm_srai_epi16(temp_2, shift);
                    temp_3 = _mm_srai_epi16(temp_3, shift);

                    _mm_storeu_si128((__m128i *)(p2 + 0 * s_dst), temp_0);
                    _mm_storeu_si128((__m128i *)(p2 + 1 * s_dst), temp_1);
                    _mm_storeu_si128((__m128i *)(p2 + 2 * s_dst), temp_2);
                    _mm_storeu_si128((__m128i *)(p2 + 3 * s_dst), temp_3);

                    p0 += 8;
                    p1 += 8;
                    p2 += 8;
                }
            }

            rem_w &= 0x7;
            /* One 4 case */
            if (rem_w >= 4)
            {
                src_8x16b = _mm_loadl_epi64((__m128i *) (p0));
                src_8x16b_1 = _mm_loadl_epi64((__m128i *) (p0 + s_src));
                src_8x16b_2 = _mm_loadl_epi64((__m128i *) (p0 + (s_src * 2)));
                src_8x16b_3 = _mm_loadl_epi64((__m128i *) (p0 + (s_src * 3)));

                pred_8x16b = _mm_loadl_epi64((__m128i *) (p1));
                pred_8x16b_1 = _mm_loadl_epi64((__m128i *) (p1 + s_ref));
                pred_8x16b_2 = _mm_loadl_epi64((__m128i *) (p1 + (s_ref * 2)));
                pred_8x16b_3 = _mm_loadl_epi64((__m128i *) (p1 + (s_ref * 3)));

                temp_0 = _mm_add_epi16(src_8x16b, pred_8x16b);
                temp_1 = _mm_add_epi16(src_8x16b_1, pred_8x16b_1);
                temp_2 = _mm_add_epi16(src_8x16b_2, pred_8x16b_2);
                temp_3 = _mm_add_epi16(src_8x16b_3, pred_8x16b_3);

                temp_0 = _mm_add_epi16(temp_0, offset_8x16b);
                temp_1 = _mm_add_epi16(temp_1, offset_8x16b);
                temp_2 = _mm_add_epi16(temp_2, offset_8x16b);
                temp_3 = _mm_add_epi16(temp_3, offset_8x16b);

                temp_0 = _mm_srai_epi16(temp_0, shift);
                temp_1 = _mm_srai_epi16(temp_1, shift);
                temp_2 = _mm_srai_epi16(temp_2, shift);
                temp_3 = _mm_srai_epi16(temp_3, shift);

                _mm_storel_epi64((__m128i *)(p2 + 0 * s_dst), temp_0);
                _mm_storel_epi64((__m128i *)(p2 + 1 * s_dst), temp_1);
                _mm_storel_epi64((__m128i *)(p2 + 2 * s_dst), temp_2);
                _mm_storel_epi64((__m128i *)(p2 + 3 * s_dst), temp_3);

                p0 += 4;
                p1 += 4;
                p2 += 4;
            }

            /* Remaining */
            rem_w &= 0x3;
            if (rem_w)
            {
                for (j = 0; j < rem_w; j++)
                {
                    p2[j + 0 * s_dst] = (p0[j + 0 * s_src] + p1[j + 0 * s_ref] + offset) >> shift;
                    p2[j + 1 * s_dst] = (p0[j + 1 * s_src] + p1[j + 1 * s_ref] + offset) >> shift;
                    p2[j + 2 * s_dst] = (p0[j + 2 * s_src] + p1[j + 2 * s_ref] + offset) >> shift;
                    p2[j + 3 * s_dst] = (p0[j + 3 * s_src] + p1[j + 3 * s_ref] + offset) >> shift;
                }
            }
        }
    }

    /* Remaining rows */
    rem_h &= 0x3;

    /* One 2 row case */
    if (rem_h >= 2)
    {
        p0 = src + ((ht >> 2) << 2) * s_src;
        p1 = ref + ((ht >> 2) << 2) * s_ref;
        p2 = dst + ((ht >> 2) << 2) * s_dst;

        /* One 2 row case */
        {
            rem_w = wd;

            /* Mult. of 8 Loop */
            if (rem_w >= 8)
            {
                for (j = 0; j < rem_w; j += 8)
                {
                    src_8x16b = _mm_loadu_si128((__m128i *) (p0));
                    src_8x16b_1 = _mm_loadu_si128((__m128i *) (p0 + s_src));

                    pred_8x16b = _mm_loadu_si128((__m128i *) (p1));
                    pred_8x16b_1 = _mm_loadu_si128((__m128i *) (p1 + s_ref));

                    temp_0 = _mm_add_epi16(src_8x16b, pred_8x16b);
                    temp_1 = _mm_add_epi16(src_8x16b_1, pred_8x16b_1);

                    temp_0 = _mm_add_epi16(temp_0, offset_8x16b);
                    temp_1 = _mm_add_epi16(temp_1, offset_8x16b);

                    temp_0 = _mm_srai_epi16(temp_0, shift);
                    temp_1 = _mm_srai_epi16(temp_1, shift);

                    _mm_storeu_si128((__m128i *)(p2 + 0 * s_dst), temp_0);
                    _mm_storeu_si128((__m128i *)(p2 + 1 * s_dst), temp_1);

                    p0 += 8;
                    p1 += 8;
                    p2 += 8;
                }
            }

            rem_w &= 0x7;
            /* One 4 case */
            if (rem_w >= 4)
            {
                src_8x16b = _mm_loadl_epi64((__m128i *) (p0));
                src_8x16b_1 = _mm_loadl_epi64((__m128i *) (p0 + s_src));

                pred_8x16b = _mm_loadl_epi64((__m128i *) (p1));
                pred_8x16b_1 = _mm_loadl_epi64((__m128i *) (p1 + s_ref));

                temp_0 = _mm_add_epi16(src_8x16b, pred_8x16b);
                temp_1 = _mm_add_epi16(src_8x16b_1, pred_8x16b_1);

                temp_0 = _mm_add_epi16(temp_0, offset_8x16b);
                temp_1 = _mm_add_epi16(temp_1, offset_8x16b);

                temp_0 = _mm_srai_epi16(temp_0, shift);
                temp_1 = _mm_srai_epi16(temp_1, shift);

                _mm_storel_epi64((__m128i *)(p2 + 0 * s_dst), temp_0);
                _mm_storel_epi64((__m128i *)(p2 + 1 * s_dst), temp_1);

                p0 += 4;
                p1 += 4;
                p2 += 4;
            }

            /* Remaining */
            rem_w &= 0x3;
            if (rem_w)
            {
                for (j = 0; j < rem_w; j++)
                {
                    p2[j + 0 * s_dst] = (p0[j + 0 * s_src] + p1[j + 0 * s_ref] + offset) >> shift;
                    p2[j + 1 * s_dst] = (p0[j + 1 * s_src] + p1[j + 1 * s_ref] + offset) >> shift;
                }
            }
        }
    }

    /* Remaining 1 row */
    if (rem_h &= 0x1)
    {
        p0 = src + ((ht >> 1) << 1) * s_src;
        p1 = ref + ((ht >> 1) << 1) * s_ref;
        p2 = dst + ((ht >> 1) << 1) * s_dst;

        /* One 1 row case */
        {
            rem_w = wd;

            /* Mult. of 8 Loop */
            if (rem_w >= 8)
            {
                for (j = 0; j < rem_w; j += 8)
                {
                    src_8x16b = _mm_loadu_si128((__m128i *) (p0));

                    pred_8x16b = _mm_loadu_si128((__m128i *) (p1));

                    temp_0 = _mm_add_epi16(src_8x16b, pred_8x16b);

                    temp_0 = _mm_add_epi16(temp_0, offset_8x16b);

                    temp_0 = _mm_srai_epi16(temp_0, shift);

                    _mm_storeu_si128((__m128i *)(p2 + 0 * s_dst), temp_0);

                    p0 += 8;
                    p1 += 8;
                    p2 += 8;
                }
            }

            rem_w &= 0x7;
            /* One 4 case */
            if (rem_w >= 4)
            {
                src_8x16b = _mm_loadl_epi64((__m128i *) (p0));

                pred_8x16b = _mm_loadl_epi64((__m128i *) (p1));

                temp_0 = _mm_add_epi16(src_8x16b, pred_8x16b);

                temp_0 = _mm_add_epi16(temp_0, offset_8x16b);

                temp_0 = _mm_srai_epi16(temp_0, shift);

                _mm_storel_epi64((__m128i *)(p2 + 0 * s_dst), temp_0);

                p0 += 4;
                p1 += 4;
                p2 += 4;
            }

            /* Remaining */
            rem_w &= 0x3;
            if (rem_w)
            {
                for (j = 0; j < rem_w; j++)
                {
                    p2[j] = (p0[j] + p1[j] + offset) >> shift;
                }
            }
        }
    }
}
#endif /* X86_SSE */

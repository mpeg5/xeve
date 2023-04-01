/* The copyright in this software is being made available under the BSD
   License, included below. This software may be subject to contributor and
   other third party rights, including patent rights, and no such rights are
   granted under this license.

   Copyright (c) 2020, Samsung Electronics Co., Ltd.
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

#include "../xeve_def.h"
#include "../xevem_mc.h"
#include "../xevem_mc_sse.h"
#include <assert.h>

/****************************************************************************
 * motion compensation for luma
 ****************************************************************************/
static const s8 shuffle_2tap[16] = { 0, 1, 2, 3, 2, 3, 4, 5, 4, 5, 6, 7, 6, 7, 8, 9 };

void mc_filter_bilin_horz_sse(s16 const *ref,
    int src_stride,
    s16 *pred,
    int dst_stride,
    const short *coeff,
    int width,
    int height,
    int min_val,
    int max_val,
    int offset,
    int shift,
    s8  is_last)
{
    int row, col, rem_w, rem_h;
    int src_stride2, src_stride3;
    s16 const *inp_copy;
    s16 *dst_copy;

    __m128i offset_4x32b = _mm_set1_epi32(offset);
    __m128i mm_min = _mm_set1_epi16(min_val);
    __m128i mm_max = _mm_set1_epi16(max_val);

    __m128i row1, row11, row2, row22, row3, row33, row4, row44;
    __m128i res0, res1, res2, res3;
    __m128i coeff0_1_8x16b, shuffle;

    rem_w = width;
    inp_copy = ref;
    dst_copy = pred;

    src_stride2 = (src_stride << 1);
    src_stride3 = (src_stride * 3);

    /* load 8 8-bit coefficients and convert 8-bit into 16-bit  */
    coeff0_1_8x16b = _mm_loadl_epi64((__m128i*)coeff);      /*w0 w1 x x x x x x*/
    coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);  /*w0 w1 w0 w1 w0 w1 w0 w1*/

    shuffle = _mm_loadu_si128((__m128i*)shuffle_2tap);

    rem_h = (height & 0x3);

    if (rem_w > 7)
    {
        for (row = height; row > 3; row -= 4)
        {
            int cnt = 0;
            for (col = rem_w; col > 7; col -= 8)
            {
                /*load 8 pixel values from row 0*/
                row1  = _mm_loadu_si128((__m128i*)(inp_copy + cnt));                   /*a0 a1 a2 a3 a4 a5 a6 a7*/
                row11 = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 1));               /*a1 a2 a3 a4 a5 a6 a7 a8*/
                row2  = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + cnt));      /*b0 b1 b2 b3 b4 b5 b6 b7*/
                row22 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + cnt + 1));  /*b1 b2 b3 b4 b5 b6 b7 b8*/
                row3  = _mm_loadu_si128((__m128i*)(inp_copy + src_stride2 + cnt));
                row33 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride2 + cnt + 1));
                row4  = _mm_loadu_si128((__m128i*)(inp_copy + src_stride3 + cnt));
                row44 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride3 + cnt + 1));

                row1  = _mm_madd_epi16(row1,  coeff0_1_8x16b);          /*a0+a1 a2+a3 a4+a5 a6+a7*/
                row11 = _mm_madd_epi16(row11, coeff0_1_8x16b);          /*a1+a2 a3+a4 a5+a6 a7+a8*/
                row2  = _mm_madd_epi16(row2,  coeff0_1_8x16b);
                row22 = _mm_madd_epi16(row22, coeff0_1_8x16b);
                row3  = _mm_madd_epi16(row3,  coeff0_1_8x16b);
                row33 = _mm_madd_epi16(row33, coeff0_1_8x16b);
                row4  = _mm_madd_epi16(row4,  coeff0_1_8x16b);
                row44 = _mm_madd_epi16(row44, coeff0_1_8x16b);

                row1  = _mm_add_epi32(row1,  offset_4x32b);
                row11 = _mm_add_epi32(row11, offset_4x32b);
                row2  = _mm_add_epi32(row2,  offset_4x32b);
                row22 = _mm_add_epi32(row22, offset_4x32b);
                row3  = _mm_add_epi32(row3,  offset_4x32b);
                row33 = _mm_add_epi32(row33, offset_4x32b);
                row4  = _mm_add_epi32(row4,  offset_4x32b);
                row44 = _mm_add_epi32(row44, offset_4x32b);

                row1 = _mm_srai_epi32(row1, shift);
                row11 = _mm_srai_epi32(row11, shift);
                row2 = _mm_srai_epi32(row2, shift);
                row22 = _mm_srai_epi32(row22, shift);
                row3 = _mm_srai_epi32(row3, shift);
                row33 = _mm_srai_epi32(row33, shift);
                row4 = _mm_srai_epi32(row4, shift);
                row44 = _mm_srai_epi32(row44, shift);

                row1 = _mm_packs_epi32(row1, row2);
                row11 = _mm_packs_epi32(row11, row22);
                row3 = _mm_packs_epi32(row3, row4);
                row33 = _mm_packs_epi32(row33, row44);

                res0 = _mm_unpacklo_epi16(row1, row11);
                res1 = _mm_unpackhi_epi16(row1, row11);
                res2 = _mm_unpacklo_epi16(row3, row33);
                res3 = _mm_unpackhi_epi16(row3, row33);

                if (is_last)
                {
                    res0 = _mm_min_epi16(res0, mm_max);
                    res1 = _mm_min_epi16(res1, mm_max);
                    res2 = _mm_min_epi16(res2, mm_max);
                    res3 = _mm_min_epi16(res3, mm_max);

                    res0 = _mm_max_epi16(res0, mm_min);
                    res1 = _mm_max_epi16(res1, mm_min);
                    res2 = _mm_max_epi16(res2, mm_min);
                    res3 = _mm_max_epi16(res3, mm_min);
                }

                /* to store the 8 pixels res. */
                _mm_storeu_si128((__m128i *)(dst_copy + cnt), res0);
                _mm_storeu_si128((__m128i *)(dst_copy + dst_stride + cnt), res1);
                _mm_storeu_si128((__m128i *)(dst_copy + dst_stride * 2 + cnt), res2);
                _mm_storeu_si128((__m128i *)(dst_copy + dst_stride * 3 + cnt), res3);

                cnt += 8; /* To pointer updates*/
            }

            inp_copy += (src_stride << 2);
            dst_copy += (dst_stride << 2);
        }

        /*extra height to be done --- one row at a time*/
        for (row = 0; row < rem_h; row++)
        {
            int cnt = 0;
            for (col = rem_w; col > 7; col -= 8)
            {
                /*load 8 pixel values from row 0*/
                row1 = _mm_loadu_si128((__m128i*)(inp_copy + cnt));       /*a0 a1 a2 a3 a4 a5 a6 a7*/
                row11 = _mm_loadu_si128((__m128i*)(inp_copy + cnt + 1));  /*a1 a2 a3 a4 a5 a6 a7 a8*/

                row1 = _mm_madd_epi16(row1, coeff0_1_8x16b);              /*a0+a1 a2+a3 a4+a5 a6+a7*/
                row11 = _mm_madd_epi16(row11, coeff0_1_8x16b);            /*a1+a2 a3+a4 a5+a6 a7+a8*/

                row1 = _mm_add_epi32(row1, offset_4x32b);
                row11 = _mm_add_epi32(row11, offset_4x32b);

                row1 = _mm_srai_epi32(row1, shift);
                row11 = _mm_srai_epi32(row11, shift);

                row1 = _mm_packs_epi32(row1, row11);    /*a0 a2 a4 a6 a1 a3 a5 a7*/

                res0 = _mm_unpackhi_epi64(row1, row1);  /*a1 a3 a5 a7*/
                res1 = _mm_unpacklo_epi16(row1, res0);  /*a0 a1 a2 a3 a4 a5 a6 a7*/

                if (is_last)
                {
                    res1 = _mm_min_epi16(res1, mm_max);
                    res1 = _mm_max_epi16(res1, mm_min);
                }

                /* to store the 8 pixels res. */
                _mm_storeu_si128((__m128i *)(dst_copy + cnt), res1);

                cnt += 8;
            }

            inp_copy += (src_stride);
            dst_copy += (dst_stride);
        }
    }

    rem_w &= 0x7;

    if (rem_w > 3)
    {
        inp_copy = ref + ((width / 8) * 8);
        dst_copy = pred + ((width / 8) * 8);

        for (row = height; row > 3; row -= 4)
        {
            /*load 8 pixel values from row 0*/
            row1 = _mm_loadu_si128((__m128i*)(inp_copy));               /*a0 a1 a2 a3 a4 a5 a6 a7*/
            row2 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride));  /*a1 a2 a3 a4 a5 a6 a7 a8*/
            row3 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride2));
            row4 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride3));

            row1 = _mm_shuffle_epi8(row1, shuffle);  /*a0 a1 a1 a2 a2 a3 a3 a4 */
            row2 = _mm_shuffle_epi8(row2, shuffle);
            row3 = _mm_shuffle_epi8(row3, shuffle);
            row4 = _mm_shuffle_epi8(row4, shuffle);

            row1 = _mm_madd_epi16(row1, coeff0_1_8x16b);  /*a0+a1 a1+a2 a2+a3 a3+a4*/
            row2 = _mm_madd_epi16(row2, coeff0_1_8x16b);
            row3 = _mm_madd_epi16(row3, coeff0_1_8x16b);
            row4 = _mm_madd_epi16(row4, coeff0_1_8x16b);

            row1 = _mm_add_epi32(row1, offset_4x32b);
            row2 = _mm_add_epi32(row2, offset_4x32b);
            row3 = _mm_add_epi32(row3, offset_4x32b);
            row4 = _mm_add_epi32(row4, offset_4x32b);

            row1 = _mm_srai_epi32(row1, shift);
            row2 = _mm_srai_epi32(row2, shift);
            row3 = _mm_srai_epi32(row3, shift);
            row4 = _mm_srai_epi32(row4, shift);

            res0 = _mm_packs_epi32(row1, row2);
            res1 = _mm_packs_epi32(row3, row4);

            if (is_last)
            {
                res0 = _mm_min_epi16(res0, mm_max);
                res1 = _mm_min_epi16(res1, mm_max);

                res0 = _mm_max_epi16(res0, mm_min);
                res1 = _mm_max_epi16(res1, mm_min);
            }

            /* to store the 8 pixels res. */
            _mm_storel_epi64((__m128i *)(dst_copy), res0);
            _mm_storel_epi64((__m128i *)(dst_copy + dst_stride * 2), res1);
            _mm_storel_epi64((__m128i *)(dst_copy + dst_stride), _mm_unpackhi_epi64(res0, res0));
            _mm_storel_epi64((__m128i *)(dst_copy + dst_stride * 3), _mm_unpackhi_epi64(res1, res1));

            inp_copy += (src_stride << 2);
            dst_copy += (dst_stride << 2);
        }

        for (row = 0; row < rem_h; row++)
        {
            /*load 8 pixel values from row 0*/
            row1 = _mm_loadu_si128((__m128i*)(inp_copy));  /*a0 a1 a2 a3 a4 a5 a6 a7*/

            res0 = _mm_shuffle_epi8(row1, shuffle);        /*a0 a1 a1 a2 a2 a3 a3 a4 */
            res0 = _mm_madd_epi16(res0, coeff0_1_8x16b);   /*a0+a1 a1+a2 a2+a3 a3+a4*/
            res0 = _mm_add_epi32(res0, offset_4x32b);
            res0 = _mm_srai_epi32(res0, shift);
            res0 = _mm_packs_epi32(res0, res0);

            if (is_last)
            {
                res0 = _mm_min_epi16(res0, mm_max);
                res0 = _mm_max_epi16(res0, mm_min);
            }

            _mm_storel_epi64((__m128i *)(dst_copy), res0);

            inp_copy += (src_stride);
            dst_copy += (dst_stride);
        }
    }

    rem_w &= 0x3;

    if (rem_w)
    {
        int sum, sum1;

        inp_copy = ref + ((width / 4) * 4);
        dst_copy = pred + ((width / 4) * 4);

        for (row = height; row > 3; row -= 4)
        {
            for (col = 0; col < rem_w; col++)
            {
                row1 = _mm_loadu_si128((__m128i*)(inp_copy + col));               /*a0 a1 x x x x x x*/
                row2 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + col));  /*b0 b1 x x x x x x*/
                row3 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride2 + col));
                row4 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride3 + col));

                row1 = _mm_unpacklo_epi32(row1, row2);  /*a0 a1 b0 b1*/
                row3 = _mm_unpacklo_epi32(row3, row4);  /*c0 c1 d0 d1*/
                row1 = _mm_unpacklo_epi64(row1, row3);  /*a0 a1 b0 b1 c0 c1 d0 d1*/

                row1 = _mm_madd_epi16(row1, coeff0_1_8x16b);  /*a0+a1 b0+b1 c0+c1 d0+d1*/

                row1 = _mm_add_epi32(row1, offset_4x32b);
                row1 = _mm_srai_epi32(row1, shift);
                res0 = _mm_packs_epi32(row1, row1);

                if (is_last)
                {
                    res0 = _mm_min_epi16(res0, mm_max);
                    res0 = _mm_max_epi16(res0, mm_min);
                }

                /*extract 32 bit integer form register and store it in dst_copy*/
                sum = _mm_extract_epi32(res0, 0);
                sum1 = _mm_extract_epi32(res0, 1);

                dst_copy[col] = (s16)(sum & 0xffff);
                dst_copy[col + dst_stride] = (s16)(sum >> 16);
                dst_copy[col + (dst_stride << 1)] = (s16)(sum1 & 0xffff);
                dst_copy[col + (dst_stride * 3)] = (s16)(sum1 >> 16);
            }
            inp_copy += (src_stride << 2);
            dst_copy += (dst_stride << 2);
        }

        for (row = 0; row < rem_h; row++)
        {
            for (col = 0; col < rem_w; col++)
            {
                s16 val;
                int sum;

                sum = inp_copy[col + 0] * coeff[0];
                sum += inp_copy[col + 1] * coeff[1];

                val = (sum + offset) >> shift;
                dst_copy[col] = (is_last ? (XEVE_CLIP3(min_val, max_val, val)) : val);
            }
            inp_copy += src_stride;
            dst_copy += dst_stride;
        }
    }
}

void mc_filter_bilin_vert_sse(s16 const *ref,
    int src_stride,
    s16 *pred,
    int dst_stride,
    const short *coeff,
    int width,
    int height,
    int min_val,
    int max_val,
    int offset,
    int shift,
    s8  is_last)
{
    int row, col, rem_w, rem_h;
    int src_stride2, src_stride3, src_stride4;
    s16 const *inp_copy;
    s16 *dst_copy;

    __m128i offset_4x32b = _mm_set1_epi32(offset);
    __m128i mm_min = _mm_set1_epi16(min_val);
    __m128i mm_max = _mm_set1_epi16(max_val);

    __m128i row1, row11, row2, row22, row3, row33, row4, row44, row5;
    __m128i res0, res1, res2, res3;
    __m128i coeff0_1_8x16b;

    rem_w = width;
    inp_copy = ref;
    dst_copy = pred;

    src_stride2 = (src_stride << 1);
    src_stride3 = (src_stride * 3);
    src_stride4 = (src_stride << 2);

    coeff0_1_8x16b = _mm_loadl_epi64((__m128i*)coeff);      /*w0 w1 x x x x x x*/
    coeff0_1_8x16b = _mm_shuffle_epi32(coeff0_1_8x16b, 0);  /*w0 w1 w0 w1 w0 w1 w0 w1*/

    rem_h = height & 0x3;

    if (rem_w > 7)
    {
        for (row = height; row > 3; row -= 4)
        {
            int cnt = 0;
            for (col = rem_w; col > 7; col -= 8)
            {
                /*load 8 pixel values from row 0*/
                row1 = _mm_loadu_si128((__m128i*)(inp_copy + cnt));                        /*a0 a1 a2 a3 a4 a5 a6 a7*/
                row2 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + cnt));  /*b0 b1 b2 b3 b4 b5 b6 b7*/
                row3 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride2 + cnt));
                row4 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride3 + cnt));
                row5 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride4 + cnt));

                row11 = _mm_unpacklo_epi16(row1, row2);   /*a0 b0 a1 b1 a2 b2 a3 b3*/
                row1 = _mm_unpackhi_epi16(row1, row2);    /*a4 b4 a5 b5 a6 b6 a7 b7*/
                row22 = _mm_unpacklo_epi16(row2, row3);
                row2 = _mm_unpackhi_epi16(row2, row3);
                row33 = _mm_unpacklo_epi16(row3, row4);
                row3 = _mm_unpackhi_epi16(row3, row4);
                row44 = _mm_unpacklo_epi16(row4, row5);
                row4 = _mm_unpackhi_epi16(row4, row5);

                row11 = _mm_madd_epi16(row11, coeff0_1_8x16b);  /*a0+a1 a2+a3 a4+a5 a6+a7*/
                row1 = _mm_madd_epi16(row1, coeff0_1_8x16b);    /*a1+a2 a3+a4 a5+a6 a7+a8*/
                row22 = _mm_madd_epi16(row22, coeff0_1_8x16b);
                row2 = _mm_madd_epi16(row2, coeff0_1_8x16b);
                row33 = _mm_madd_epi16(row33, coeff0_1_8x16b);
                row3 = _mm_madd_epi16(row3, coeff0_1_8x16b);
                row44 = _mm_madd_epi16(row44, coeff0_1_8x16b);
                row4 = _mm_madd_epi16(row4, coeff0_1_8x16b);

                row11 = _mm_add_epi32(row11, offset_4x32b);
                row1 = _mm_add_epi32(row1, offset_4x32b);
                row22 = _mm_add_epi32(row22, offset_4x32b);
                row2 = _mm_add_epi32(row2, offset_4x32b);
                row33 = _mm_add_epi32(row33, offset_4x32b);
                row3 = _mm_add_epi32(row3, offset_4x32b);
                row44 = _mm_add_epi32(row44, offset_4x32b);
                row4 = _mm_add_epi32(row4, offset_4x32b);

                row11 = _mm_srai_epi32(row11, shift);
                row1 = _mm_srai_epi32(row1, shift);
                row22 = _mm_srai_epi32(row22, shift);
                row2 = _mm_srai_epi32(row2, shift);
                row33 = _mm_srai_epi32(row33, shift);
                row3 = _mm_srai_epi32(row3, shift);
                row44 = _mm_srai_epi32(row44, shift);
                row4 = _mm_srai_epi32(row4, shift);

                res0 = _mm_packs_epi32(row11, row1);
                res1 = _mm_packs_epi32(row22, row2);
                res2 = _mm_packs_epi32(row33, row3);
                res3 = _mm_packs_epi32(row44, row4);

                if (is_last)
                {
                    res0 = _mm_min_epi16(res0, mm_max);
                    res1 = _mm_min_epi16(res1, mm_max);
                    res2 = _mm_min_epi16(res2, mm_max);
                    res3 = _mm_min_epi16(res3, mm_max);

                    res0 = _mm_max_epi16(res0, mm_min);
                    res1 = _mm_max_epi16(res1, mm_min);
                    res2 = _mm_max_epi16(res2, mm_min);
                    res3 = _mm_max_epi16(res3, mm_min);
                }

                /* to store the 8 pixels res. */
                _mm_storeu_si128((__m128i *)(dst_copy + cnt), res0);
                _mm_storeu_si128((__m128i *)(dst_copy + dst_stride + cnt), res1);
                _mm_storeu_si128((__m128i *)(dst_copy + dst_stride * 2 + cnt), res2);
                _mm_storeu_si128((__m128i *)(dst_copy + dst_stride * 3 + cnt), res3);

                cnt += 8;  /* To pointer updates*/
            }

            inp_copy += (src_stride << 2);
            dst_copy += (dst_stride << 2);
        }

        /*extra height to be done --- one row at a time*/
        for (row = 0; row < rem_h; row++)
        {
            int cnt = 0;
            for (col = rem_w; col > 7; col -= 8)
            {
                /*load 8 pixel values from row 0*/
                row1 = _mm_loadu_si128((__m128i*)(inp_copy + cnt));                        /*a0 a1 a2 a3 a4 a5 a6 a7*/
                row2 = _mm_loadu_si128((__m128i*)(inp_copy + src_stride + cnt));  /*b0 b1 b2 b3 b4 b5 b6 b7*/

                row11 = _mm_unpacklo_epi16(row1, row2);  /*a0 b0 a1 b1 a2 b2 a3 b3*/
                row1 = _mm_unpackhi_epi16(row1, row2);   /*a4 b4 a5 b5 a6 b6 a7 b7*/

                row1 = _mm_madd_epi16(row1, coeff0_1_8x16b);    /*a0+a1 a2+a3 a4+a5 a6+a7*/
                row11 = _mm_madd_epi16(row11, coeff0_1_8x16b);  /*a1+a2 a3+a4 a5+a6 a7+a8*/

                row1 = _mm_add_epi32(row1, offset_4x32b);
                row11 = _mm_add_epi32(row11, offset_4x32b);

                row1 = _mm_srai_epi32(row1, shift);
                row11 = _mm_srai_epi32(row11, shift);

                res1 = _mm_packs_epi32(row11, row1);

                if (is_last)
                {
                    res1 = _mm_min_epi16(res1, mm_max);
                    res1 = _mm_max_epi16(res1, mm_min);
                }

                /* to store the 8 pixels res. */
                _mm_storeu_si128((__m128i *)(dst_copy + cnt), res1);

                cnt += 8;
            }

            inp_copy += (src_stride);
            dst_copy += (dst_stride);
        }
    }

    rem_w &= 0x7;

    if (rem_w > 3)
    {
        inp_copy = ref + ((width / 8) * 8);
        dst_copy = pred + ((width / 8) * 8);

        for (row = height; row > 3; row -= 4)
        {
            /*load 4 pixel values */
            row1 = _mm_loadl_epi64((__m128i*)(inp_copy));                        /*a0 a1 a2 a3 x x x x*/
            row2 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride));  /*b0 b1 b2 b3 x x x x*/
            row3 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride2));
            row4 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride3));
            row5 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride4));

            row11 = _mm_unpacklo_epi16(row1, row2);  /*a0 b0 a1 b1 a2 b2 a3 b3*/
            row22 = _mm_unpacklo_epi16(row2, row3);
            row33 = _mm_unpacklo_epi16(row3, row4);
            row44 = _mm_unpacklo_epi16(row4, row5);

            row11 = _mm_madd_epi16(row11, coeff0_1_8x16b);  /*a0+a1 a1+a2 a2+a3 a3+a4*/
            row22 = _mm_madd_epi16(row22, coeff0_1_8x16b);
            row33 = _mm_madd_epi16(row33, coeff0_1_8x16b);
            row44 = _mm_madd_epi16(row44, coeff0_1_8x16b);

            row11 = _mm_add_epi32(row11, offset_4x32b);
            row22 = _mm_add_epi32(row22, offset_4x32b);
            row33 = _mm_add_epi32(row33, offset_4x32b);
            row44 = _mm_add_epi32(row44, offset_4x32b);

            row11 = _mm_srai_epi32(row11, shift);
            row22 = _mm_srai_epi32(row22, shift);
            row33 = _mm_srai_epi32(row33, shift);
            row44 = _mm_srai_epi32(row44, shift);

            res0 = _mm_packs_epi32(row11, row22);
            res1 = _mm_packs_epi32(row33, row44);

            if (is_last)
            {
                res0 = _mm_min_epi16(res0, mm_max);
                res1 = _mm_min_epi16(res1, mm_max);
                res0 = _mm_max_epi16(res0, mm_min);
                res1 = _mm_max_epi16(res1, mm_min);
            }

            /* to store the 8 pixels res. */
            _mm_storel_epi64((__m128i *)(dst_copy), res0);
            _mm_storel_epi64((__m128i *)(dst_copy + dst_stride), _mm_unpackhi_epi64(res0, res0));
            _mm_storel_epi64((__m128i *)(dst_copy + dst_stride * 2), res1);
            _mm_storel_epi64((__m128i *)(dst_copy + dst_stride * 3), _mm_unpackhi_epi64(res1, res1));

            inp_copy += (src_stride << 2);
            dst_copy += (dst_stride << 2);
        }

        for (row = 0; row < rem_h; row++)
        {
            /*load 8 pixel values from row 0*/
            row1 = _mm_loadl_epi64((__m128i*)(inp_copy));                        /*a0 a1 a2 a3 x x x x*/
            row2 = _mm_loadl_epi64((__m128i*)(inp_copy + src_stride));  /*b0 b1 b2 b3 x x x x*/

            row11 = _mm_unpacklo_epi16(row1, row2);         /*a0 b0 a1 b1 a2 b2 a3 b3*/
            row11 = _mm_madd_epi16(row11, coeff0_1_8x16b);  /*a0+a1 a1+a2 a2+a3 a3+a4*/
            row11 = _mm_add_epi32(row11, offset_4x32b);
            row11 = _mm_srai_epi32(row11, shift);
            row11 = _mm_packs_epi32(row11, row11);

            if (is_last)
            {
                row11 = _mm_min_epi16(row11, mm_max);
                row11 = _mm_max_epi16(row11, mm_min);
            }

            _mm_storel_epi64((__m128i *)(dst_copy), row11);

            inp_copy += (src_stride);
            dst_copy += (dst_stride);
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

                sum = inp_copy[col + 0 * src_stride] * coeff[0];
                sum += inp_copy[col + 1 * src_stride] * coeff[1];

                val = (sum + offset) >> shift;
                dst_copy[col] = (is_last ? (XEVE_CLIP3(min_val, max_val, val)) : val);
            }

            inp_copy += src_stride;
            dst_copy += dst_stride;
        }
    }
}

void xeve_mc_dmvr_l_00_sse(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int i, j;

    gmv_x >>= 4;
    gmv_y >>= 4;

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

void xeve_mc_dmvr_l_n0_sse(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int dx = gmv_x & 15;
    ref = ref - 3;

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    xeve_mc_filter_l_8pel_horz_clip_sse(ref, s_ref, pred, s_pred, xevem_tbl_mc_l_coeff[dx], w, h, min, max, MAC_ADD_N0, MAC_SFT_N0);
}

void xeve_mc_dmvr_l_0n_sse(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int dy = gmv_y & 15;
    ref = ref - (3 * s_ref);

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    xeve_mc_filter_l_8pel_vert_clip_sse(ref, s_ref, pred, s_pred, xevem_tbl_mc_l_coeff[dy], w, h, min, max, MAC_ADD_0N, MAC_SFT_0N);
}

void xeve_mc_dmvr_l_nn_sse(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    s16         buf[(MAX_CU_SIZE + MC_IBUF_PAD_L)*MAX_CU_SIZE];
    int         dx, dy;

    dx = gmv_x & 15;
    dy = gmv_y & 15;
    ref = ref - (3 * s_ref + 3);

    int shift1 = XEVE_MIN(4, bit_depth - 8);
    int shift2 = XEVE_MAX(8, 20 - bit_depth);
    int offset1 = 0;
    int offset2 = (1 << (shift2 - 1));
    int max = ((1 << bit_depth) - 1);

    int min = 0;

    xeve_mc_filter_l_8pel_horz_no_clip_sse(ref, s_ref, buf, w, xevem_tbl_mc_l_coeff[dx], w, (h + 7), offset1, shift1);
    xeve_mc_filter_l_8pel_vert_clip_sse(buf, w, pred, s_pred, xevem_tbl_mc_l_coeff[dy], w, h, min, max, offset2, shift2);
}

void xeve_bl_mc_l_00_sse(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
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

void xeve_bl_mc_l_n0_sse(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int dx = gmv_x & 15;
    ref += (gmv_y >> 4) * s_ref + (gmv_x >> 4);

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    mc_filter_bilin_horz_sse(ref, s_ref, pred, s_pred, xeve_tbl_bl_mc_l_coeff[dx], w, h, min, max, MAC_ADD_N0, MAC_SFT_N0, 1);
}

void xeve_bl_mc_l_0n_sse(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth)
{
    int dy = gmv_y & 15;
    ref += (gmv_y >> 4) * s_ref + (gmv_x >> 4);

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    mc_filter_bilin_vert_sse(ref, s_ref, pred, s_pred, xeve_tbl_bl_mc_l_coeff[dy], w, h, min, max, MAC_ADD_0N, MAC_SFT_0N, 1);
}

void xeve_bl_mc_l_nn_sse(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    s16 buf[(MAX_CU_SIZE + OPT_MC_BI_PAD * 2 + MC_IBUF_PAD_L)*(MAX_CU_SIZE + OPT_MC_BI_PAD * 2 + MC_IBUF_PAD_L)];

    int dx, dy;

    dx = gmv_x & 15;
    dy = gmv_y & 15;
    ref += (gmv_y >> 4) * s_ref + (gmv_x >> 4);

    int shift1 = XEVE_MIN(4, bit_depth - 8);
    int shift2 = XEVE_MAX(8, 20 - bit_depth);
    int offset1 = 0;
    int offset2 = (1 << (shift2 - 1));
    int max = ((1 << bit_depth) - 1);
    int min = 0;

    mc_filter_bilin_horz_sse(ref, s_ref, buf, w, xeve_tbl_bl_mc_l_coeff[dx], w, (h + 1), min, max, offset1, shift1, 0);
    mc_filter_bilin_vert_sse(buf, w, pred, s_pred, xeve_tbl_bl_mc_l_coeff[dy], w, h, min, max, offset2, shift2, 1);
}

/****************************************************************************
 * motion compensation for chroma
 ****************************************************************************/
void xeve_mc_dmvr_c_00_sse(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    int i, j;

    gmv_x >>= 5;
    gmv_y >>= 5;

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
    else if (((w & 0x3) == 0))
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

void xeve_mc_dmvr_c_n0_sse(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    int dx = gmv_x & 31;
    ref -= 1;

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    xeve_mc_filter_c_4pel_horz_sse(ref, s_ref, pred, s_pred, xevem_tbl_mc_c_coeff[dx], w, h, min, max, MAC_ADD_N0, MAC_SFT_N0, 1);
}

void xeve_mc_dmvr_c_0n_sse(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    int dy = gmv_y & 31;
    ref -= 1 * s_ref;

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    xeve_mc_filter_c_4pel_vert_sse(ref, s_ref, pred, s_pred, xevem_tbl_mc_c_coeff[dy], w, h, min, max, MAC_ADD_0N, MAC_SFT_0N, 1);
}

void xeve_mc_dmvr_c_nn_sse(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth)
{
    s16 buf[(MAX_CU_SIZE + MC_IBUF_PAD_C)*MAX_CU_SIZE];
    int dx, dy;

    dx = gmv_x & 31;
    dy = gmv_y & 31;
    ref -= (1 * s_ref + 1);

    int shift1 = XEVE_MIN(4, bit_depth - 8);
    int shift2 = XEVE_MAX(8, 20 - bit_depth);
    int offset1 = 0;
    int offset2 = (1 << (shift2 - 1));
    int max = ((1 << bit_depth) - 1);
    int min = 0;

    xeve_mc_filter_c_4pel_horz_sse(ref, s_ref, buf, w, xevem_tbl_mc_c_coeff[dx], w, (h + 3), min, max, offset1, shift1, 0);
    xeve_mc_filter_c_4pel_vert_sse(buf, w, pred, s_pred, xevem_tbl_mc_c_coeff[dy], w, h, min, max, offset2, shift2, 1);
}

const XEVEM_MC xeve_tbl_dmvr_mc_l_sse[2][2] =

{
  {
    xeve_mc_dmvr_l_00_sse, /* dx == 0 && dy == 0 */
    xeve_mc_dmvr_l_0n_sse  /* dx == 0 && dy != 0 */
  },
  {
    xeve_mc_dmvr_l_n0_sse, /* dx != 0 && dy == 0 */
    xeve_mc_dmvr_l_nn_sse  /* dx != 0 && dy != 0 */
  }
};

const XEVEM_MC xeve_tbl_dmvr_mc_c_sse[2][2] =
{
  {
    xeve_mc_dmvr_c_00_sse, /* dx == 0 && dy == 0 */
    xeve_mc_dmvr_c_0n_sse  /* dx == 0 && dy != 0 */
  },
  {
    xeve_mc_dmvr_c_n0_sse, /* dx != 0 && dy == 0 */
    xeve_mc_dmvr_c_nn_sse  /* dx != 0 && dy != 0 */
  }
};

/* luma and chroma will remain the same */
const XEVEM_MC xeve_tbl_bl_mc_l_sse[2][2] =
{
    {
        xeve_bl_mc_l_00_sse,
        xeve_bl_mc_l_0n_sse
    },
    {
        xeve_bl_mc_l_n0_sse,
        xeve_bl_mc_l_nn_sse
    }
};

#define CALC_EQUAL_COEFF_8PXLS(x1,x2,y1,y2,tmp0,tmp1,tmp2,tmp3,inter0,inter1,inter2,inter3,load_location)      \
{                                                                                                              \
inter0 = _mm_mul_epi32(x1, y1);                                                                                \
inter1 = _mm_mul_epi32(tmp0, tmp2);                                                                            \
inter2 = _mm_mul_epi32(x2, y2);                                                                                \
inter3 = _mm_mul_epi32(tmp1, tmp3);                                                                            \
inter2 = _mm_add_epi64(inter0, inter2);                                                                        \
inter3 = _mm_add_epi64(inter1, inter3);                                                                        \
inter0 = _mm_loadl_epi64(load_location);                                                                       \
inter3 = _mm_add_epi64(inter2, inter3);                                                                        \
inter1 = _mm_srli_si128(inter3, 8);                                                                            \
inter3 = _mm_add_epi64(inter1, inter3);                                                                        \
inter3 = _mm_add_epi64(inter0, inter3);                                                                        \
}

void xevem_scaled_horizontal_sobel_filter_sse(pel *pred, int pred_stride, int *derivate, int derivate_buf_stride, int width, int height)
{
    int j, col, row;

    __m128i mm_pred[4];
    __m128i mm2x_pred[2];
    __m128i mm_intermediates[4];
    __m128i mm_derivate[2];

    assert(!(height % 2));
    assert(!(width % 4));

    /* Derivates of the rows and columns at the boundary are done at the end of this function */
    /* The value of col and row indicate the columns and rows for which the derivates have already been computed */
    for(col = 1; (col + 2) < width; col += 2)
    {
        mm_pred[0] = _mm_loadl_epi64((const __m128i *)(&pred[0 * pred_stride + col - 1]));
        mm_pred[1] = _mm_loadl_epi64((const __m128i *)(&pred[1 * pred_stride + col - 1]));

        mm_pred[0] = _mm_cvtepi16_epi32(mm_pred[0]);
        mm_pred[1] = _mm_cvtepi16_epi32(mm_pred[1]);

        for(row = 1; row < (height - 1); row += 2)
        {
            mm_pred[2] = _mm_loadl_epi64((const __m128i *)(&pred[(row + 1) * pred_stride + col - 1]));
            mm_pred[3] = _mm_loadl_epi64((const __m128i *)(&pred[(row + 2) * pred_stride + col - 1]));

            mm_pred[2] = _mm_cvtepi16_epi32(mm_pred[2]);
            mm_pred[3] = _mm_cvtepi16_epi32(mm_pred[3]);

            mm2x_pred[0] = _mm_slli_epi32(mm_pred[1], 1);
            mm2x_pred[1] = _mm_slli_epi32(mm_pred[2], 1);

            mm_intermediates[0] = _mm_add_epi32(mm2x_pred[0], mm_pred[0]);
            mm_intermediates[2] = _mm_add_epi32(mm2x_pred[1], mm_pred[1]);

            mm_intermediates[0] = _mm_add_epi32(mm_intermediates[0], mm_pred[2]);
            mm_intermediates[2] = _mm_add_epi32(mm_intermediates[2], mm_pred[3]);

            mm_pred[0] = mm_pred[2];
            mm_pred[1] = mm_pred[3];

            mm_intermediates[1] = _mm_srli_si128(mm_intermediates[0], 8);
            mm_intermediates[3] = _mm_srli_si128(mm_intermediates[2], 8);

            mm_derivate[0] = _mm_sub_epi32(mm_intermediates[1], mm_intermediates[0]);
            mm_derivate[1] = _mm_sub_epi32(mm_intermediates[3], mm_intermediates[2]);

            _mm_storel_epi64((__m128i *)(&derivate[col + (row + 0) * derivate_buf_stride]), mm_derivate[0]);
            _mm_storel_epi64((__m128i *)(&derivate[col + (row + 1) * derivate_buf_stride]), mm_derivate[1]);
        }
    }

    for(j = 1; j < (height - 1); j++)
    {
        derivate[j * derivate_buf_stride] = derivate[j * derivate_buf_stride + 1];
        derivate[j * derivate_buf_stride + (width - 1)] = derivate[j * derivate_buf_stride + (width - 2)];
    }

    xeve_mcpy
    (
        derivate,
        derivate + derivate_buf_stride,
        width * sizeof(derivate[0])
    );

    xeve_mcpy
    (
        derivate + (height - 1) * derivate_buf_stride,
        derivate + (height - 2) * derivate_buf_stride,
        width * sizeof(derivate[0])
    );
}

void xevem_scaled_vertical_sobel_filter_sse(pel *pred, int pred_stride, int *derivate, int derivate_buf_stride, int width, int height)
{
    int j, col, row;

    __m128i mm_pred[4];
    __m128i mm_intermediates[6];
    __m128i mm_derivate[2];

    assert(!(height % 2));
    assert(!(width % 4));

    /* Derivates of the rows and columns at the boundary are done at the end of this function */
    /* The value of col and row indicate the columns and rows for which the derivates have already been computed */
    for(col = 1; col < (width - 1); col += 2)
    {
        mm_pred[0] = _mm_loadl_epi64((const __m128i *)(&pred[0 * pred_stride + col - 1]));
        mm_pred[1] = _mm_loadl_epi64((const __m128i *)(&pred[1 * pred_stride + col - 1]));

        mm_pred[0] = _mm_cvtepi16_epi32(mm_pred[0]);
        mm_pred[1] = _mm_cvtepi16_epi32(mm_pred[1]);

        for(row = 1; row < (height - 1); row += 2)
        {
            mm_pred[2] = _mm_loadl_epi64((const __m128i *)(&pred[(row + 1) * pred_stride + col - 1]));
            mm_pred[3] = _mm_loadl_epi64((const __m128i *)(&pred[(row + 2) * pred_stride + col - 1]));

            mm_pred[2] = _mm_cvtepi16_epi32(mm_pred[2]);
            mm_pred[3] = _mm_cvtepi16_epi32(mm_pred[3]);

            mm_intermediates[0] = _mm_sub_epi32(mm_pred[2], mm_pred[0]);
            mm_intermediates[3] = _mm_sub_epi32(mm_pred[3], mm_pred[1]);

            mm_pred[0] = mm_pred[2];
            mm_pred[1] = mm_pred[3];

            mm_intermediates[1] = _mm_srli_si128(mm_intermediates[0], 4);
            mm_intermediates[4] = _mm_srli_si128(mm_intermediates[3], 4);
            mm_intermediates[2] = _mm_srli_si128(mm_intermediates[0], 8);
            mm_intermediates[5] = _mm_srli_si128(mm_intermediates[3], 8);

            mm_intermediates[1] = _mm_slli_epi32(mm_intermediates[1], 1);
            mm_intermediates[4] = _mm_slli_epi32(mm_intermediates[4], 1);

            mm_intermediates[0] = _mm_add_epi32(mm_intermediates[0], mm_intermediates[2]);
            mm_intermediates[3] = _mm_add_epi32(mm_intermediates[3], mm_intermediates[5]);

            mm_derivate[0] = _mm_add_epi32(mm_intermediates[0], mm_intermediates[1]);
            mm_derivate[1] = _mm_add_epi32(mm_intermediates[3], mm_intermediates[4]);

            _mm_storel_epi64((__m128i *) (&derivate[col + (row + 0) * derivate_buf_stride]), mm_derivate[0]);
            _mm_storel_epi64((__m128i *) (&derivate[col + (row + 1) * derivate_buf_stride]), mm_derivate[1]);
        }
    }

    for(j = 1; j < (height - 1); j++)
    {
        derivate[j * derivate_buf_stride] = derivate[j * derivate_buf_stride + 1];
        derivate[j * derivate_buf_stride + (width - 1)] = derivate[j * derivate_buf_stride + (width - 2)];
    }

    xeve_mcpy
    (
        derivate,
        derivate + derivate_buf_stride,
        width * sizeof(derivate[0])
    );

    xeve_mcpy
    (
        derivate + (height - 1) * derivate_buf_stride,
        derivate + (height - 2) * derivate_buf_stride,
        width * sizeof(derivate[0])
    );
}

void xevem_equal_coeff_computer_sse(pel *residue, int residue_stride, int **derivate, int derivate_buf_stride, s64(*equal_coeff)[7], int width, int height, int vertex_num)
{
    int j, k;
    int idx1 = 0, idx2 = 0;

    __m128i mm_two, mm_four;
    __m128i mm_tmp[4];
    __m128i mm_intermediate[4];
    __m128i mm_idx_k, mm_idx_j[2];
    __m128i mm_residue[2];

    // Add directly to indexes to get new index
    mm_two = _mm_set1_epi32(2);
    mm_four = _mm_set1_epi32(4);

    if(vertex_num == 3)
    {
        __m128i mm_c[12];

        idx1 = -2 * derivate_buf_stride - 4;
        idx2 = -derivate_buf_stride - 4;
        mm_idx_j[0] = _mm_set1_epi32(-2);
        mm_idx_j[1] = _mm_set1_epi32(-1);

        for(j = 0; j < height; j += 2)
        {
            mm_idx_j[0] = _mm_add_epi32(mm_idx_j[0], mm_two);
            mm_idx_j[1] = _mm_add_epi32(mm_idx_j[1], mm_two);
            mm_idx_k = _mm_set_epi32(-1, -2, -3, -4);
            idx1 += (derivate_buf_stride << 1);
            idx2 += (derivate_buf_stride << 1);

            for(k = 0; k < width; k += 4)
            {
                idx1 += 4;
                idx2 += 4;

                mm_idx_k = _mm_add_epi32(mm_idx_k, mm_four);

                // 1st row
                mm_c[0] = _mm_loadu_si128((const __m128i*)&derivate[0][idx1]);
                mm_c[2] = _mm_loadu_si128((const __m128i*)&derivate[1][idx1]);
                // 2nd row
                mm_c[6] = _mm_loadu_si128((const __m128i*)&derivate[0][idx2]);
                mm_c[8] = _mm_loadu_si128((const __m128i*)&derivate[1][idx2]);

                // 1st row
                mm_c[1] = _mm_mullo_epi32(mm_idx_k, mm_c[0]);
                mm_c[3] = _mm_mullo_epi32(mm_idx_k, mm_c[2]);
                mm_c[4] = _mm_mullo_epi32(mm_idx_j[0], mm_c[0]);
                mm_c[5] = _mm_mullo_epi32(mm_idx_j[0], mm_c[2]);

                // 2nd row
                mm_c[7] = _mm_mullo_epi32(mm_idx_k, mm_c[6]);
                mm_c[9] = _mm_mullo_epi32(mm_idx_k, mm_c[8]);
                mm_c[10] = _mm_mullo_epi32(mm_idx_j[1], mm_c[6]);
                mm_c[11] = _mm_mullo_epi32(mm_idx_j[1], mm_c[8]);

                // Residue
                mm_residue[0] = _mm_loadl_epi64((const __m128i*)&residue[idx1]);
                mm_residue[1] = _mm_loadl_epi64((const __m128i*)&residue[idx2]);

                mm_residue[0] = _mm_cvtepi16_epi32(mm_residue[0]);
                mm_residue[1] = _mm_cvtepi16_epi32(mm_residue[1]);

                mm_residue[0] = _mm_slli_epi32(mm_residue[0], 3);
                mm_residue[1] = _mm_slli_epi32(mm_residue[1], 3);

                // Calculate residue coefficients first
                mm_tmp[2] = _mm_srli_si128(mm_residue[0], 4);
                mm_tmp[3] = _mm_srli_si128(mm_residue[1], 4);

                // 1st row
                mm_tmp[0] = _mm_srli_si128(mm_c[0], 4);
                mm_tmp[1] = _mm_srli_si128(mm_c[6], 4);
                // 7th col of row
                CALC_EQUAL_COEFF_8PXLS(mm_c[0], mm_c[6], mm_residue[0], mm_residue[1], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[1][6]);
                _mm_storel_epi64((__m128i*)&equal_coeff[1][6], mm_intermediate[3]);

                // 2nd row
                mm_tmp[0] = _mm_srli_si128(mm_c[1], 4);
                mm_tmp[1] = _mm_srli_si128(mm_c[7], 4);
                // 7th col of row
                CALC_EQUAL_COEFF_8PXLS(mm_c[1], mm_c[7], mm_residue[0], mm_residue[1], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[2][6]);
                _mm_storel_epi64((__m128i*)&equal_coeff[2][6], mm_intermediate[3]);

                // 3rd row
                mm_tmp[0] = _mm_srli_si128(mm_c[2], 4);
                mm_tmp[1] = _mm_srli_si128(mm_c[8], 4);
                // 7th col of row
                CALC_EQUAL_COEFF_8PXLS(mm_c[2], mm_c[8], mm_residue[0], mm_residue[1], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[3][6]);
                _mm_storel_epi64((__m128i*)&equal_coeff[3][6], mm_intermediate[3]);

                // 4th row
                mm_tmp[0] = _mm_srli_si128(mm_c[3], 4);
                mm_tmp[1] = _mm_srli_si128(mm_c[9], 4);
                // 7th col of row
                CALC_EQUAL_COEFF_8PXLS(mm_c[3], mm_c[9], mm_residue[0], mm_residue[1], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[4][6]);
                _mm_storel_epi64((__m128i*)&equal_coeff[4][6], mm_intermediate[3]);

                // 5th row
                mm_tmp[0] = _mm_srli_si128(mm_c[4], 4);
                mm_tmp[1] = _mm_srli_si128(mm_c[10], 4);
                // 7th col of row
                CALC_EQUAL_COEFF_8PXLS(mm_c[4], mm_c[10], mm_residue[0], mm_residue[1], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[5][6]);
                _mm_storel_epi64((__m128i*)&equal_coeff[5][6], mm_intermediate[3]);

                // 6th row
                mm_tmp[0] = _mm_srli_si128(mm_c[5], 4);
                mm_tmp[1] = _mm_srli_si128(mm_c[11], 4);
                // 7th col of row
                CALC_EQUAL_COEFF_8PXLS(mm_c[5], mm_c[11], mm_residue[0], mm_residue[1], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[6][6]);
                _mm_storel_epi64((__m128i*)&equal_coeff[6][6], mm_intermediate[3]);

                //Start calculation of coefficient matrix
                // 1st row
                mm_tmp[0] = _mm_srli_si128(mm_c[0], 4);
                mm_tmp[1] = _mm_srli_si128(mm_c[6], 4);

                // 1st col of row
                CALC_EQUAL_COEFF_8PXLS(mm_c[0], mm_c[6], mm_c[0], mm_c[6], mm_tmp[0], mm_tmp[1], mm_tmp[0], mm_tmp[1], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[1][0]);
                _mm_storel_epi64((__m128i*)&equal_coeff[1][0], mm_intermediate[3]);
                // 2nd col of row and 1st col of 2nd row
                mm_tmp[2] = _mm_srli_si128(mm_c[1], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[7], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[0], mm_c[6], mm_c[1], mm_c[7], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[1][1]);
                _mm_storel_epi64((__m128i*)&equal_coeff[1][1], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[2][0], mm_intermediate[3]);
                // 3rd col of row and 1st col of 3rd row
                mm_tmp[2] = _mm_srli_si128(mm_c[2], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[8], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[0], mm_c[6], mm_c[2], mm_c[8], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[1][2]);
                _mm_storel_epi64((__m128i*)&equal_coeff[1][2], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[3][0], mm_intermediate[3]);
                // 4th col of row and 1st col of 4th row
                mm_tmp[2] = _mm_srli_si128(mm_c[3], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[9], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[0], mm_c[6], mm_c[3], mm_c[9], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[1][3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[1][3], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[4][0], mm_intermediate[3]);
                // 5th col of row and 1st col of the 5th row
                mm_tmp[2] = _mm_srli_si128(mm_c[4], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[10], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[0], mm_c[6], mm_c[4], mm_c[10], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[1][4]);
                _mm_storel_epi64((__m128i*)&equal_coeff[1][4], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[5][0], mm_intermediate[3]);
                // 6th col of row and 1st col of the 6th row
                mm_tmp[2] = _mm_srli_si128(mm_c[5], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[11], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[0], mm_c[6], mm_c[5], mm_c[11], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[1][5]);
                _mm_storel_epi64((__m128i*)&equal_coeff[1][5], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[6][0], mm_intermediate[3]);

                // 2nd row
                mm_tmp[0] = _mm_srli_si128(mm_c[1], 4);
                mm_tmp[1] = _mm_srli_si128(mm_c[7], 4);

                // 2nd col of row
                CALC_EQUAL_COEFF_8PXLS(mm_c[1], mm_c[7], mm_c[1], mm_c[7], mm_tmp[0], mm_tmp[1], mm_tmp[0], mm_tmp[1], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[2][1]);
                _mm_storel_epi64((__m128i*)&equal_coeff[2][1], mm_intermediate[3]);
                // 3rd col of row and 2nd col of 3rd row
                mm_tmp[2] = _mm_srli_si128(mm_c[2], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[8], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[1], mm_c[7], mm_c[2], mm_c[8], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[2][2]);
                _mm_storel_epi64((__m128i*)&equal_coeff[2][2], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[3][1], mm_intermediate[3]);
                // 4th col of row and 2nd col of 4th row
                mm_tmp[2] = _mm_srli_si128(mm_c[3], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[9], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[1], mm_c[7], mm_c[3], mm_c[9], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[2][3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[2][3], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[4][1], mm_intermediate[3]);
                // 5th col of row and 1st col of the 5th row
                mm_tmp[2] = _mm_srli_si128(mm_c[4], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[10], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[1], mm_c[7], mm_c[4], mm_c[10], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[2][4]);
                _mm_storel_epi64((__m128i*)&equal_coeff[2][4], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[5][1], mm_intermediate[3]);
                // 6th col of row and 1st col of the 6th row
                mm_tmp[2] = _mm_srli_si128(mm_c[5], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[11], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[1], mm_c[7], mm_c[5], mm_c[11], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[2][5]);
                _mm_storel_epi64((__m128i*)&equal_coeff[2][5], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[6][1], mm_intermediate[3]);

                // 3rd row
                mm_tmp[0] = _mm_srli_si128(mm_c[2], 4);
                mm_tmp[1] = _mm_srli_si128(mm_c[8], 4);

                //3rd Col of row
                CALC_EQUAL_COEFF_8PXLS(mm_c[2], mm_c[8], mm_c[2], mm_c[8], mm_tmp[0], mm_tmp[1], mm_tmp[0], mm_tmp[1], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[3][2]);
                _mm_storel_epi64((__m128i*)&equal_coeff[3][2], mm_intermediate[3]);
                // 4th col of row and 3rd col of 4th row
                mm_tmp[2] = _mm_srli_si128(mm_c[3], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[9], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[2], mm_c[8], mm_c[3], mm_c[9], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[3][3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[3][3], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[4][2], mm_intermediate[3]);
                // 5th col of row and 1st col of the 5th row
                mm_tmp[2] = _mm_srli_si128(mm_c[4], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[10], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[2], mm_c[8], mm_c[4], mm_c[10], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[3][4]);
                _mm_storel_epi64((__m128i*)&equal_coeff[3][4], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[5][2], mm_intermediate[3]);
                // 6th col of row and 1st col of the 6th row
                mm_tmp[2] = _mm_srli_si128(mm_c[5], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[11], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[2], mm_c[8], mm_c[5], mm_c[11], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[3][5]);
                _mm_storel_epi64((__m128i*)&equal_coeff[3][5], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[6][2], mm_intermediate[3]);

                // 4th row
                mm_tmp[0] = _mm_srli_si128(mm_c[3], 4);
                mm_tmp[1] = _mm_srli_si128(mm_c[9], 4);

                // 4th col of row
                CALC_EQUAL_COEFF_8PXLS(mm_c[3], mm_c[9], mm_c[3], mm_c[9], mm_tmp[0], mm_tmp[1], mm_tmp[0], mm_tmp[1], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[4][3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[4][3], mm_intermediate[3]);
                // 5th col of row and 1st col of the 5th row
                mm_tmp[2] = _mm_srli_si128(mm_c[4], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[10], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[3], mm_c[9], mm_c[4], mm_c[10], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[4][4]);
                _mm_storel_epi64((__m128i*)&equal_coeff[4][4], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[5][3], mm_intermediate[3]);
                // 6th col of row and 1st col of the 6th row
                mm_tmp[2] = _mm_srli_si128(mm_c[5], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[11], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[3], mm_c[9], mm_c[5], mm_c[11], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[4][5]);
                _mm_storel_epi64((__m128i*)&equal_coeff[4][5], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[6][3], mm_intermediate[3]);

                // 5th row
                mm_tmp[0] = _mm_srli_si128(mm_c[4], 4);
                mm_tmp[1] = _mm_srli_si128(mm_c[10], 4);
                // 5th col of row and 1st col of the 5th row
                CALC_EQUAL_COEFF_8PXLS(mm_c[4], mm_c[10], mm_c[4], mm_c[10], mm_tmp[0], mm_tmp[1], mm_tmp[0], mm_tmp[1], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[5][4]);
                _mm_storel_epi64((__m128i*)&equal_coeff[5][4], mm_intermediate[3]);
                // 6th col of row and 1st col of the 6th row
                mm_tmp[2] = _mm_srli_si128(mm_c[5], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[11], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[4], mm_c[10], mm_c[5], mm_c[11], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[5][5]);
                _mm_storel_epi64((__m128i*)&equal_coeff[5][5], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[6][4], mm_intermediate[3]);

                // 6th row
                mm_tmp[0] = _mm_srli_si128(mm_c[5], 4);
                mm_tmp[1] = _mm_srli_si128(mm_c[11], 4);
                // 5th col of row and 1st col of the 5th row
                CALC_EQUAL_COEFF_8PXLS(mm_c[5], mm_c[11], mm_c[5], mm_c[11], mm_tmp[0], mm_tmp[1], mm_tmp[0], mm_tmp[1], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[6][5]);
                _mm_storel_epi64((__m128i*)&equal_coeff[6][5], mm_intermediate[3]);
            }

            idx1 -= (width);
            idx2 -= (width);
        }
    }
    else
    {
        __m128i mm_c[8];

        idx1 = -2 * derivate_buf_stride - 4;
        idx2 = -derivate_buf_stride - 4;
        mm_idx_j[0] = _mm_set1_epi32(-2);
        mm_idx_j[1] = _mm_set1_epi32(-1);

        for(j = 0; j < height; j += 2)
        {
            mm_idx_j[0] = _mm_add_epi32(mm_idx_j[0], mm_two);
            mm_idx_j[1] = _mm_add_epi32(mm_idx_j[1], mm_two);
            mm_idx_k = _mm_set_epi32(-1, -2, -3, -4);
            idx1 += (derivate_buf_stride << 1);
            idx2 += (derivate_buf_stride << 1);

            for(k = 0; k < width; k += 4)
            {
                idx1 += 4;
                idx2 += 4;

                mm_idx_k = _mm_add_epi32(mm_idx_k, mm_four);

                mm_c[0] = _mm_loadu_si128((const __m128i*)&derivate[0][idx1]);
                mm_c[2] = _mm_loadu_si128((const __m128i*)&derivate[1][idx1]);
                mm_c[4] = _mm_loadu_si128((const __m128i*)&derivate[0][idx2]);
                mm_c[6] = _mm_loadu_si128((const __m128i*)&derivate[1][idx2]);

                mm_c[1] = _mm_mullo_epi32(mm_idx_k, mm_c[0]);
                mm_c[3] = _mm_mullo_epi32(mm_idx_j[0], mm_c[0]);
                mm_c[5] = _mm_mullo_epi32(mm_idx_k, mm_c[4]);
                mm_c[7] = _mm_mullo_epi32(mm_idx_j[1], mm_c[4]);

                mm_residue[0] = _mm_loadl_epi64((const __m128i*)&residue[idx1]);
                mm_residue[1] = _mm_loadl_epi64((const __m128i*)&residue[idx2]);

                mm_tmp[0] = _mm_mullo_epi32(mm_idx_j[0], mm_c[2]);
                mm_tmp[1] = _mm_mullo_epi32(mm_idx_k, mm_c[2]);
                mm_tmp[2] = _mm_mullo_epi32(mm_idx_j[1], mm_c[6]);
                mm_tmp[3] = _mm_mullo_epi32(mm_idx_k, mm_c[6]);

                mm_residue[0] = _mm_cvtepi16_epi32(mm_residue[0]);
                mm_residue[1] = _mm_cvtepi16_epi32(mm_residue[1]);

                mm_c[1] = _mm_add_epi32(mm_c[1], mm_tmp[0]);
                mm_c[3] = _mm_sub_epi32(mm_c[3], mm_tmp[1]);
                mm_c[5] = _mm_add_epi32(mm_c[5], mm_tmp[2]);
                mm_c[7] = _mm_sub_epi32(mm_c[7], mm_tmp[3]);

                mm_residue[0] = _mm_slli_epi32(mm_residue[0], 3);
                mm_residue[1] = _mm_slli_epi32(mm_residue[1], 3);

                //Start calculation of coefficient matrix
                // 1st row
                mm_tmp[0] = _mm_srli_si128(mm_c[0], 4);
                mm_tmp[1] = _mm_srli_si128(mm_c[4], 4);

                // 1st col of row
                CALC_EQUAL_COEFF_8PXLS(mm_c[0], mm_c[4], mm_c[0], mm_c[4], mm_tmp[0], mm_tmp[1], mm_tmp[0], mm_tmp[1], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[1][0]);
                _mm_storel_epi64((__m128i*)&equal_coeff[1][0], mm_intermediate[3]);
                // 2nd col of row and 1st col of 2nd row
                mm_tmp[2] = _mm_srli_si128(mm_c[1], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[5], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[0], mm_c[4], mm_c[1], mm_c[5], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[1][1]);
                _mm_storel_epi64((__m128i*)&equal_coeff[1][1], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[2][0], mm_intermediate[3]);
                // 3rd col of row and 1st col of 3rd row
                mm_tmp[2] = _mm_srli_si128(mm_c[2], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[6], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[0], mm_c[4], mm_c[2], mm_c[6], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[1][2]);
                _mm_storel_epi64((__m128i*)&equal_coeff[1][2], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[3][0], mm_intermediate[3]);
                // 4th col of row and 1st col of 4th row
                mm_tmp[2] = _mm_srli_si128(mm_c[3], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[7], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[0], mm_c[4], mm_c[3], mm_c[7], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[1][3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[1][3], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[4][0], mm_intermediate[3]);
                // 5th col of row
                mm_tmp[2] = _mm_srli_si128(mm_residue[0], 4);
                mm_tmp[3] = _mm_srli_si128(mm_residue[1], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[0], mm_c[4], mm_residue[0], mm_residue[1], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[1][4]);
                _mm_storel_epi64((__m128i*)&equal_coeff[1][4], mm_intermediate[3]);

                // 2nd row
                mm_tmp[0] = _mm_srli_si128(mm_c[1], 4);
                mm_tmp[1] = _mm_srli_si128(mm_c[5], 4);

                // 2nd col of row
                CALC_EQUAL_COEFF_8PXLS(mm_c[1], mm_c[5], mm_c[1], mm_c[5], mm_tmp[0], mm_tmp[1], mm_tmp[0], mm_tmp[1], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[2][1]);
                _mm_storel_epi64((__m128i*)&equal_coeff[2][1], mm_intermediate[3]);
                // 3rd col of row and 2nd col of 3rd row
                mm_tmp[2] = _mm_srli_si128(mm_c[2], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[6], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[1], mm_c[5], mm_c[2], mm_c[6], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[2][2]);
                _mm_storel_epi64((__m128i*)&equal_coeff[2][2], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[3][1], mm_intermediate[3]);
                // 4th col of row and 2nd col of 4th row
                mm_tmp[2] = _mm_srli_si128(mm_c[3], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[7], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[1], mm_c[5], mm_c[3], mm_c[7], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[2][3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[2][3], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[4][1], mm_intermediate[3]);
                // 5th col of row
                mm_tmp[2] = _mm_srli_si128(mm_residue[0], 4);
                mm_tmp[3] = _mm_srli_si128(mm_residue[1], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[1], mm_c[5], mm_residue[0], mm_residue[1], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[2][4]);
                _mm_storel_epi64((__m128i*)&equal_coeff[2][4], mm_intermediate[3]);

                // 3rd row
                mm_tmp[0] = _mm_srli_si128(mm_c[2], 4);
                mm_tmp[1] = _mm_srli_si128(mm_c[6], 4);

                //3rd Col of row
                CALC_EQUAL_COEFF_8PXLS(mm_c[2], mm_c[6], mm_c[2], mm_c[6], mm_tmp[0], mm_tmp[1], mm_tmp[0], mm_tmp[1], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[3][2]);
                _mm_storel_epi64((__m128i*)&equal_coeff[3][2], mm_intermediate[3]);
                // 4th col of row and 3rd col of 4th row
                mm_tmp[2] = _mm_srli_si128(mm_c[3], 4);
                mm_tmp[3] = _mm_srli_si128(mm_c[7], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[2], mm_c[6], mm_c[3], mm_c[7], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[3][3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[3][3], mm_intermediate[3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[4][2], mm_intermediate[3]);
                // 5th col of row
                mm_tmp[2] = _mm_srli_si128(mm_residue[0], 4);
                mm_tmp[3] = _mm_srli_si128(mm_residue[1], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[2], mm_c[6], mm_residue[0], mm_residue[1], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[3][4]);
                _mm_storel_epi64((__m128i*)&equal_coeff[3][4], mm_intermediate[3]);

                // 4th row
                mm_tmp[0] = _mm_srli_si128(mm_c[3], 4);
                mm_tmp[1] = _mm_srli_si128(mm_c[7], 4);

                // 4th col of row
                CALC_EQUAL_COEFF_8PXLS(mm_c[3], mm_c[7], mm_c[3], mm_c[7], mm_tmp[0], mm_tmp[1], mm_tmp[0], mm_tmp[1], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[4][3]);
                _mm_storel_epi64((__m128i*)&equal_coeff[4][3], mm_intermediate[3]);
                // 5th col of row
                mm_tmp[2] = _mm_srli_si128(mm_residue[0], 4);
                mm_tmp[3] = _mm_srli_si128(mm_residue[1], 4);
                CALC_EQUAL_COEFF_8PXLS(mm_c[3], mm_c[7], mm_residue[0], mm_residue[1], mm_tmp[0], mm_tmp[1], mm_tmp[2], mm_tmp[3], mm_intermediate[0], mm_intermediate[1], mm_intermediate[2], mm_intermediate[3], (const __m128i*)&equal_coeff[4][4]);
                _mm_storel_epi64((__m128i*)&equal_coeff[4][4], mm_intermediate[3]);
            }

            idx1 -= (width);
            idx2 -= (width);
        }
    }
}

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
#include "xeve_mc_neon.h"
#include <assert.h>

#if ARM_NEON

#define vmadd_s16(a, b)\
    vpaddq_s32(vmull_s16(vget_low_s16(a), vget_low_s16(b)), vmull_s16(vget_high_s16(a), vget_high_s16(b)));

#define vmadd1_s16(a, coef)\
    vpaddq_s32(vmull_s16(a.val[0], vget_low_s16(coef)), vmull_s16(a.val[1], vget_high_s16(coef)));

void xeve_mc_filter_l_8pel_horz_clip_neon(s16* ref,
    int src_stride,
    s16* pred,
    int dst_stride,
    const s16* coeff,
    int width,
    int height,
    int min_val,
    int max_val,
    int offset,
    int shift)
{
    int row, col, rem_w;
    s16 const* src_tmp;
    s16 const* inp_copy;
    s16* dst_copy;

    src_tmp = ref;
    rem_w = width;
    inp_copy = src_tmp;
    dst_copy = pred;


    int16x8_t src1_neon, src2_neon, src3_neon, src4_neon, src5_neon, src6_neon, src7_neon, src8_neon, result_16x8;
    int16x8_t src_temp1_neon, src_temp2_neon, src_temp3_neon, src_temp4_neon, src_temp5_neon, src_temp6_neon;
    int16x8_t src_temp7_neon, src_temp8_neon, src_temp9_neon, src_temp0_neon;
    int16x8_t src_temp11_neon, src_temp12_neon, src_temp13_neon, src_temp14_neon, src_temp15_neon, src_temp16_neon;
    int16x8_t src_temp11a_neon, src_temp12a_neon, src_temp11b_neon, src_temp12b_neon, src_temp11c_neon, src_temp12c_neon;
    int16x8_t res_temp1_neon, res_temp2_neon, res_temp3_neon, res_temp4_neon, res_temp5_neon, res_temp6_neon, res_temp7_neon, res_temp8_neon;
    int16x8_t res_temp9_neon, res_temp0_neon;
    int16x8_t res_temp11_neon, res_temp12_neon, res_temp13_neon, res_temp14_neon, res_temp15_neon, res_temp16_neon;
    int16x8_t coeff0_1_neon, coeff2_3_neon, coeff4_5_neon, coeff6_7_neon;

    int16x8_t min = vdupq_n_s16(min_val);
    int16x8_t max = vdupq_n_s16(max_val);
    int32x4_t offset_neon = vdupq_n_s32(offset);

    coeff0_1_neon = vld1q_s16(coeff);
    coeff2_3_neon = vdupq_n_s32(vgetq_lane_s32((coeff0_1_neon), (1)));
    coeff4_5_neon = vdupq_n_s32(vgetq_lane_s32((coeff0_1_neon), (2)));
    coeff6_7_neon = vdupq_n_s32(vgetq_lane_s32((coeff0_1_neon), (3)));
    coeff0_1_neon = vdupq_n_s32(vgetq_lane_s32((coeff0_1_neon), (0)));

    int32x4_t shift_neon = vdupq_n_s32(-shift);

    if (!(height & 1))    /*even height*/
    {
        if (rem_w > 7)
        {
            for (row = 0; row < height; ++row)
            {
                int cnt = 0;
                for (col = width; col > 7; col -= 8)
                {
                    src1_neon = vld1q_s16((inp_copy + cnt));
                    src2_neon = vld1q_s16((inp_copy + cnt + 1));
                    src3_neon = vld1q_s16((inp_copy + cnt + 2));
                    src4_neon = vld1q_s16((inp_copy + cnt + 3));
                    src5_neon = vld1q_s16((inp_copy + cnt + 4));
                    src6_neon = vld1q_s16((inp_copy + cnt + 5));
                    src7_neon = vld1q_s16((inp_copy + cnt + 6));
                    src8_neon = vld1q_s16((inp_copy + cnt + 7));

                    src_temp1_neon = vzip1q_s16(src1_neon, src2_neon);
                    src_temp2_neon = vzip2q_s16(src1_neon, src2_neon);
                    res_temp1_neon = vmadd_s16(src_temp1_neon, coeff0_1_neon);
                    res_temp7_neon = vmadd_s16(src_temp2_neon, coeff0_1_neon);

                    src_temp3_neon = vzip1q_s16(src3_neon, src4_neon);
                    src_temp4_neon = vzip2q_s16(src3_neon, src4_neon);
                    res_temp2_neon = vmadd_s16(src_temp3_neon, coeff2_3_neon);
                    res_temp8_neon = vmadd_s16(src_temp4_neon, coeff2_3_neon);

                    src_temp5_neon = vzip1q_s16(src5_neon, src6_neon);
                    src_temp6_neon = vzip2q_s16(src5_neon, src6_neon);
                    res_temp3_neon = vmadd_s16(src_temp5_neon, coeff4_5_neon);
                    res_temp9_neon = vmadd_s16(src_temp6_neon, coeff4_5_neon);

                    src_temp7_neon = vzip1q_s16(src7_neon, src8_neon);
                    src_temp8_neon = vzip2q_s16(src7_neon, src8_neon);
                    res_temp4_neon = vmadd_s16(src_temp7_neon, coeff6_7_neon);
                    res_temp0_neon = vmadd_s16(src_temp8_neon, coeff6_7_neon);

                    res_temp5_neon = vaddq_s32(res_temp1_neon, res_temp2_neon);
                    res_temp6_neon = vaddq_s32(res_temp3_neon, res_temp4_neon);
                    res_temp5_neon = vaddq_s32(res_temp5_neon, res_temp6_neon);
                    res_temp6_neon = vaddq_s32(res_temp7_neon, res_temp8_neon);
                    res_temp7_neon = vaddq_s32(res_temp9_neon, res_temp0_neon);
                    res_temp8_neon = vaddq_s32(res_temp6_neon, res_temp7_neon);
                    
                    /* Add offset */
                    res_temp6_neon = vaddq_s32(res_temp5_neon, offset_neon);
                    res_temp7_neon = vaddq_s32(res_temp8_neon, offset_neon);

                    /* Shift */
                    res_temp6_neon = vshlq_s32(res_temp6_neon, shift_neon);
                    res_temp7_neon = vshlq_s32(res_temp7_neon, shift_neon);

                    /* pack to 16 bits */
                    result_16x8 = vcombine_s16(vqmovn_s32(res_temp6_neon), vqmovn_s32(res_temp7_neon));

                    /* Clipping */
                    result_16x8 = vminq_s16(result_16x8, max);
                    result_16x8 = vmaxq_s16(result_16x8, min);

                    /* to store the 8 pixels res. */
                    vst1q_s16((dst_copy + cnt), result_16x8);
                    cnt += 8;
                }

                inp_copy += src_stride;
                dst_copy += dst_stride;
            }
        }

        rem_w &= 0x7;

        if (rem_w > 3)
        {
            inp_copy = src_tmp + ((width >> 3) << 3);
            dst_copy = pred + ((width >> 3) << 3);

            for (row = 0; row < height; row += 2)
            {
                src1_neon = vld1q_s16((inp_copy));
                src2_neon = vld1q_s16((inp_copy + 1));
                src3_neon = vld1q_s16((inp_copy + 2));
                src4_neon = vld1q_s16((inp_copy + 3));
                src5_neon = vld1q_s16((inp_copy + 4));
                src6_neon = vld1q_s16((inp_copy + 5));
                src7_neon = vld1q_s16((inp_copy + 6));
                src8_neon = vld1q_s16((inp_copy + 7));

                src_temp11_neon = vld1q_s16((inp_copy + src_stride));
                src_temp12_neon = vld1q_s16((inp_copy + src_stride + 1));
                src_temp11a_neon = vld1q_s16((inp_copy + src_stride + 2));
                src_temp12a_neon = vld1q_s16((inp_copy + src_stride + 3));
                src_temp11b_neon = vld1q_s16((inp_copy + src_stride + 4));
                src_temp12b_neon = vld1q_s16((inp_copy + src_stride + 5));
                src_temp11c_neon = vld1q_s16((inp_copy + src_stride + 6));
                src_temp12c_neon = vld1q_s16((inp_copy + src_stride + 7));

                src_temp3_neon = vzip1q_s16(src1_neon, src2_neon);
                res_temp1_neon = vmadd_s16(src_temp3_neon, coeff0_1_neon);

                src_temp4_neon = vzip1q_s16(src3_neon, src4_neon);
                res_temp2_neon = vmadd_s16(src_temp4_neon, coeff2_3_neon);

                src_temp5_neon = vzip1q_s16(src5_neon, src6_neon);
                res_temp3_neon = vmadd_s16(src_temp5_neon, coeff4_5_neon);

                src_temp6_neon = vzip1q_s16(src7_neon, src8_neon);
                res_temp4_neon = vmadd_s16(src_temp6_neon, coeff6_7_neon);

                res_temp5_neon = vaddq_s32(res_temp1_neon, res_temp2_neon);
                res_temp6_neon = vaddq_s32(res_temp3_neon, res_temp4_neon);
                res_temp5_neon = vaddq_s32(res_temp5_neon, res_temp6_neon);

                /* Add offset, shift & pack */
                res_temp6_neon = vaddq_s32(res_temp5_neon, offset_neon);
                res_temp6_neon = vshlq_s32(res_temp6_neon, shift_neon);
                res_temp5_neon = vcombine_s16(vqmovn_s32(res_temp6_neon), vqmovn_s32(res_temp6_neon));

                src_temp13_neon = vzip1q_s16(src_temp11_neon, src_temp12_neon);
                res_temp11_neon = vmadd_s16(src_temp13_neon, coeff0_1_neon);

                src_temp14_neon = vzip1q_s16(src_temp11a_neon, src_temp12a_neon);
                res_temp12_neon = vmadd_s16(src_temp14_neon, coeff2_3_neon);

                src_temp15_neon = vzip1q_s16(src_temp11b_neon, src_temp12b_neon);
                res_temp13_neon = vmadd_s16(src_temp15_neon, coeff4_5_neon);

                src_temp16_neon = vzip1q_s16(src_temp11c_neon, src_temp12c_neon);
                res_temp14_neon = vmadd_s16(src_temp16_neon, coeff6_7_neon);

                res_temp15_neon = vaddq_s32(res_temp11_neon, res_temp12_neon);
                res_temp16_neon = vaddq_s32(res_temp13_neon, res_temp14_neon);
                res_temp15_neon = vaddq_s32(res_temp15_neon, res_temp16_neon);
                
                /* Add offset, shift & pack */
                res_temp16_neon = vaddq_s32(res_temp15_neon, offset_neon);
                res_temp16_neon = vshlq_s32(res_temp16_neon, shift_neon);
                res_temp15_neon = vcombine_s16(vqmovn_s32(res_temp16_neon), vqmovn_s32(res_temp16_neon));
                
                /* Clip */
                res_temp5_neon = vminq_s16(res_temp5_neon, max);
                res_temp15_neon = vminq_s16(res_temp15_neon, max);
                res_temp5_neon = vmaxq_s16(res_temp5_neon, min);
                res_temp15_neon = vmaxq_s16(res_temp15_neon, min);
                
                /* store */
                vst1_s16((dst_copy), vget_low_s16(res_temp5_neon));
                vst1_s16((dst_copy + dst_stride), vget_low_s16(res_temp15_neon));

                inp_copy += (src_stride << 1);
                dst_copy += (dst_stride << 1);
            }
        }

        rem_w &= 0x3;

        if (rem_w)
        {
            int16x8_t filt_coef;
            s16 sum, sum1;

            inp_copy = src_tmp + ((width >> 2) << 2);
            dst_copy = pred + ((width >> 2) << 2);

            filt_coef = vld1q_s16(coeff);

            for (row = 0; row < height; row += 2)
            {
                for (col = 0; col < rem_w; ++col)
                {
                    src_temp1_neon = vld1q_s16((inp_copy + col));
                    src_temp5_neon = vld1q_s16((inp_copy + src_stride + col));

                    src_temp1_neon = vmadd_s16(src_temp1_neon, filt_coef);
                    src_temp5_neon = vmadd_s16(src_temp5_neon, filt_coef);
                    
                    /* offset & shift */
                    sum = (s16)((vaddvq_s32(src_temp1_neon) + offset) >> shift);
                    sum1 = (s16)((vaddvq_s32(src_temp5_neon) + offset) >> shift);

                    /* clip and store */
                    dst_copy[col] = (sum < min_val) ? min_val : (sum > max_val ? max_val : sum);
                    dst_copy[col + dst_stride] = (sum1 < min_val) ? min_val : (sum1 > max_val ? max_val : sum1);
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
            for (row = 0; row < height; ++row)
            {
                int cnt = 0;
                for (col = width; col > 7; col -= 8)
                {
                    src1_neon = vld1q_s16((inp_copy + cnt));
                    src2_neon = vld1q_s16((inp_copy + cnt + 1));
                    src3_neon = vld1q_s16((inp_copy + cnt + 2));
                    src4_neon = vld1q_s16((inp_copy + cnt + 3));
                    src5_neon = vld1q_s16((inp_copy + cnt + 4));
                    src6_neon = vld1q_s16((inp_copy + cnt + 5));
                    src7_neon = vld1q_s16((inp_copy + cnt + 6));
                    src8_neon = vld1q_s16((inp_copy + cnt + 7));

                    src_temp3_neon = vzip1q_s16(src1_neon, src2_neon);
                    res_temp1_neon = vmadd_s16(src_temp3_neon, coeff0_1_neon);
                    src_temp7_neon = vzip2q_s16(src1_neon, src2_neon);
                    res_temp7_neon = vmadd_s16(src_temp7_neon, coeff0_1_neon);

                    src_temp4_neon = vzip1q_s16(src3_neon, src4_neon);
                    res_temp2_neon = vmadd_s16(src_temp4_neon, coeff2_3_neon);
                    src_temp8_neon = vzip2q_s16(src3_neon, src4_neon);
                    res_temp8_neon = vmadd_s16(src_temp8_neon, coeff2_3_neon);

                    src_temp5_neon = vzip1q_s16(src5_neon, src6_neon);
                    res_temp3_neon = vmadd_s16(src_temp5_neon, coeff4_5_neon);
                    src_temp9_neon = vzip2q_s16(src5_neon, src6_neon);
                    res_temp9_neon = vmadd_s16(src_temp9_neon, coeff4_5_neon);

                    src_temp6_neon = vzip1q_s16(src7_neon, src8_neon);
                    res_temp4_neon = vmadd_s16(src_temp6_neon, coeff6_7_neon);
                    src_temp0_neon = vzip2q_s16(src7_neon, src8_neon);
                    res_temp0_neon = vmadd_s16(src_temp0_neon, coeff6_7_neon);

                    res_temp5_neon = vaddq_s32(res_temp1_neon, res_temp2_neon);
                    res_temp6_neon = vaddq_s32(res_temp3_neon, res_temp4_neon);
                    res_temp5_neon = vaddq_s32(res_temp5_neon, res_temp6_neon);

                    res_temp6_neon = vaddq_s32(res_temp7_neon, res_temp8_neon);
                    res_temp7_neon = vaddq_s32(res_temp9_neon, res_temp0_neon);
                    res_temp8_neon = vaddq_s32(res_temp6_neon, res_temp7_neon);
                    
                    /* Add offset */
                    res_temp6_neon = vaddq_s32(res_temp5_neon, offset_neon);
                    res_temp7_neon = vaddq_s32(res_temp8_neon, offset_neon);

                    /* shift */
                    res_temp6_neon = vshlq_s32(res_temp6_neon, shift_neon);
                    res_temp7_neon = vshlq_s32(res_temp7_neon, shift_neon);

                    /* pack to 16 bits */
                    res_temp5_neon = vcombine_s16(vqmovn_s32(res_temp6_neon), vqmovn_s32(res_temp7_neon));

                    /* clip */
                    res_temp5_neon = vminq_s16(res_temp5_neon, max);
                    res_temp5_neon = vmaxq_s16(res_temp5_neon, min);

                    /* to store the 8 pixels res. */
                    vst1q_s16((dst_copy + cnt), res_temp5_neon);
                    cnt += 8;
                }

                inp_copy += src_stride;
                dst_copy += dst_stride;
            }
        }

        rem_w &= 0x7;

        if (rem_w > 3)
        {
            inp_copy = src_tmp + ((width >> 3) << 3);
            dst_copy = pred + ((width >> 3) << 3);

            for (row = 0; row < (height - 1); row += 2)
            {
                /* load the pixels */
                src1_neon = vld1q_s16((inp_copy));
                src2_neon = vld1q_s16((inp_copy + 1));
                src3_neon = vld1q_s16((inp_copy + 2));
                src4_neon = vld1q_s16((inp_copy + 3));
                src5_neon = vld1q_s16((inp_copy + 4));
                src6_neon = vld1q_s16((inp_copy + 5));
                src7_neon = vld1q_s16((inp_copy + 6));
                src8_neon = vld1q_s16((inp_copy + 7));

                src_temp11_neon = vld1q_s16((inp_copy + src_stride));
                src_temp12_neon = vld1q_s16((inp_copy + src_stride + 1));
                src_temp11a_neon = vld1q_s16((inp_copy + src_stride + 2));
                src_temp12a_neon = vld1q_s16((inp_copy + src_stride + 3));
                src_temp11b_neon = vld1q_s16((inp_copy + src_stride + 4));
                src_temp12b_neon = vld1q_s16((inp_copy + src_stride + 5));
                src_temp11c_neon = vld1q_s16((inp_copy + src_stride + 6));
                src_temp12c_neon = vld1q_s16((inp_copy + src_stride + 7));

                src_temp3_neon = vzip1q_s16(src1_neon, src2_neon);
                res_temp1_neon = vmadd_s16(src_temp3_neon, coeff0_1_neon);

                src_temp4_neon = vzip1q_s16(src3_neon, src4_neon);
                res_temp2_neon = vmadd_s16(src_temp4_neon, coeff2_3_neon);

                src_temp5_neon = vzip1q_s16(src5_neon, src6_neon);
                res_temp3_neon = vmadd_s16(src_temp5_neon, coeff4_5_neon);

                src_temp6_neon = vzip1q_s16(src7_neon, src8_neon);
                res_temp4_neon = vmadd_s16(src_temp6_neon, coeff6_7_neon);

                res_temp5_neon = vaddq_s32(res_temp1_neon, res_temp2_neon);
                res_temp6_neon = vaddq_s32(res_temp3_neon, res_temp4_neon);
                res_temp5_neon = vaddq_s32(res_temp5_neon, res_temp6_neon);

                res_temp6_neon = vaddq_s32(res_temp5_neon, offset_neon);
                res_temp6_neon = vshlq_s32(res_temp6_neon, shift_neon);

                /* pack to 16 bits */
                res_temp5_neon = vcombine_s16(vqmovn_s32(res_temp6_neon), vqmovn_s32(res_temp6_neon));

                src_temp13_neon = vzip1q_s16(src_temp11_neon, src_temp12_neon);
                res_temp11_neon = vmadd_s16(src_temp13_neon, coeff0_1_neon);

                src_temp14_neon = vzip1q_s16(src_temp11a_neon, src_temp12a_neon);
                res_temp12_neon = vmadd_s16(src_temp14_neon, coeff2_3_neon);

                src_temp15_neon = vzip1q_s16(src_temp11b_neon, src_temp12b_neon);
                res_temp13_neon = vmadd_s16(src_temp15_neon, coeff4_5_neon);

                src_temp16_neon = vzip1q_s16(src_temp11c_neon, src_temp12c_neon);
                res_temp14_neon = vmadd_s16(src_temp16_neon, coeff6_7_neon);

                res_temp15_neon = vaddq_s32(res_temp11_neon, res_temp12_neon);
                res_temp16_neon = vaddq_s32(res_temp13_neon, res_temp14_neon);
                res_temp15_neon = vaddq_s32(res_temp15_neon, res_temp16_neon);
                
                /* Add offset */
                res_temp16_neon = vaddq_s32(res_temp15_neon, offset_neon);
                
                /* Shift */
                res_temp16_neon = vshlq_s32(res_temp16_neon, shift_neon);
                
                /* Pack to 16 bits */
                res_temp15_neon = vcombine_s16(vqmovn_s32(res_temp16_neon), vqmovn_s32(res_temp16_neon));

                res_temp5_neon = vminq_s16(res_temp5_neon, max);
                res_temp5_neon = vmaxq_s16(res_temp5_neon, min);

                res_temp15_neon = vminq_s16(res_temp15_neon, max);
                res_temp15_neon = vmaxq_s16(res_temp15_neon, min);

                vst1_s16((dst_copy), vget_low_s16(res_temp5_neon));
                vst1_s16((dst_copy + dst_stride), vget_low_s16(res_temp15_neon));

                inp_copy += (src_stride << 1);
                dst_copy += (dst_stride << 1);
            }

            /*extra one height to be done*/
            src1_neon = vld1q_s16((inp_copy));
            src2_neon = vld1q_s16((inp_copy + 1));
            src3_neon = vld1q_s16((inp_copy + 2));
            src4_neon = vld1q_s16((inp_copy + 3));
            src5_neon = vld1q_s16((inp_copy + 4));
            src6_neon = vld1q_s16((inp_copy + 5));
            src7_neon = vld1q_s16((inp_copy + 6));
            src8_neon = vld1q_s16((inp_copy + 7));

            src_temp3_neon = vzip1q_s16(src1_neon, src2_neon);
            res_temp1_neon = vmadd_s16(src_temp3_neon, coeff0_1_neon);

            src_temp4_neon = vzip1q_s16(src3_neon, src4_neon);
            res_temp2_neon = vmadd_s16(src_temp4_neon, coeff2_3_neon);

            src_temp5_neon = vzip1q_s16(src5_neon, src6_neon);
            res_temp3_neon = vmadd_s16(src_temp5_neon, coeff4_5_neon);

            src_temp6_neon = vzip1q_s16(src7_neon, src8_neon);
            res_temp4_neon = vmadd_s16(src_temp6_neon, coeff6_7_neon);

            res_temp5_neon = vaddq_s32(res_temp1_neon, res_temp2_neon);
            res_temp6_neon = vaddq_s32(res_temp3_neon, res_temp4_neon);
            res_temp5_neon = vaddq_s32(res_temp5_neon, res_temp6_neon);

            /* Add offset to the result */
            res_temp6_neon = vaddq_s32(res_temp5_neon, offset_neon);

            /* shift */
            res_temp6_neon = vshlq_s32(res_temp6_neon, shift_neon);

            /* pack to 16 bits */
            res_temp5_neon = vcombine_s16(vqmovn_s32(res_temp6_neon), vqmovn_s32(res_temp6_neon));

            res_temp5_neon = vminq_s16(res_temp5_neon, max);
            res_temp5_neon = vmaxq_s16(res_temp5_neon, min);

            vst1_s16((dst_copy), vget_low_s16(res_temp5_neon));
        }

        rem_w &= 0x3;

        if (rem_w)
        {
            int16x8_t filt_coef;
            s16 sum, sum1;

            inp_copy = src_tmp + ((width >> 2) << 2);
            dst_copy = pred + ((width >> 2) << 2);

            filt_coef = vld1q_s16(coeff);

            for (row = 0; row < (height - 1); row += 2)
            {
                for (col = 0; col < rem_w; ++col)
                {
                    src_temp1_neon = vld1q_s16((inp_copy + col));
                    src_temp5_neon = vld1q_s16((inp_copy + src_stride + col));

                    src_temp1_neon = vmadd_s16(src_temp1_neon, filt_coef);
                    src_temp5_neon = vmadd_s16(src_temp5_neon, filt_coef);

                    sum = (s16)((vaddvq_s32(src_temp1_neon) + offset) >> shift); // offset & shift
                    sum1 = (s16)((vaddvq_s32(src_temp5_neon) + offset) >> shift);

                    dst_copy[col] = (sum < min_val) ? min_val : (sum > max_val ? max_val : sum); // clip and store
                    dst_copy[col + dst_stride] = (sum1 < min_val) ? min_val : (sum1 > max_val ? max_val : sum1);
                }
                inp_copy += (src_stride << 1);
                dst_copy += (dst_stride << 1);
            }

            for (col = 0; col < rem_w; ++col)
            {
                src_temp1_neon = vld1q_s16((inp_copy + col));
                src_temp1_neon = vmadd_s16(src_temp1_neon, filt_coef);

                sum = (s16)((vaddvq_s32(src_temp1_neon) + offset) >> shift); // offset & shift
                dst_copy[col] = (sum < min_val) ? min_val : (sum > max_val ? max_val : sum); // clip and store
            }
        }
    }
}

void xeve_mc_filter_l_8pel_horz_no_clip_neon(s16* ref,
    int src_stride,
    s16* pred,
    int dst_stride,
    const s16* coeff,
    int width,
    int height,
    int offset,
    int shift)
{
    int row, col, rem_w;
    s16 const* src_tmp;
    s16 const* inp_copy;
    s16* dst_copy;

    src_tmp = ref;
    rem_w = width;
    inp_copy = src_tmp;
    dst_copy = pred;


    int16x8_t src1_neon, src2_neon, src3_neon, src4_neon;
    int16x8_t src5_neon, src6_neon, src7_neon, src8_neon;

    int16x8_t src_temp3_neon, src_temp4_neon, src_temp5_neon, src_temp6_neon;
    int16x8_t src_temp7_neon, src_temp8_neon, src_temp9_neon, src_temp0_neon;
    int16x8_t res_temp1_neon, res_temp2_neon, res_temp3_neon, res_temp4_neon, res_temp5_neon, res_temp6_neon, res_temp7_neon, res_temp8_neon;
    int16x8_t res_temp9_neon, res_temp0_neon;
    int16x8_t coeff0_1_neon, coeff2_3_neon, coeff4_5_neon, coeff6_7_neon;

    /* shift and offset */
    int32x4_t offset_neon = vdupq_n_s32(offset);
    int32x4_t shift_neon = vdupq_n_s32(-shift);

    coeff0_1_neon = vld1q_s16(coeff);
    coeff2_3_neon = vdupq_n_s32(vgetq_lane_s32(coeff0_1_neon, (1)));
    coeff4_5_neon = vdupq_n_s32(vgetq_lane_s32(coeff0_1_neon, (2)));
    coeff6_7_neon = vdupq_n_s32(vgetq_lane_s32(coeff0_1_neon, (3)));
    coeff0_1_neon = vdupq_n_s32(vgetq_lane_s32(coeff0_1_neon, (0)));



    if (rem_w > 7)
    {
        for (row = 0; row < height; ++row)
        {
            for (col = 0; col < width; col += 8)
            {
                /* load pixel values */
                src1_neon = vld1q_s16((&inp_copy[col]));
                src2_neon = vld1q_s16((&inp_copy[col + 1]));
                src3_neon = vld1q_s16((&inp_copy[col + 2]));
                src4_neon = vld1q_s16((&inp_copy[col + 3]));
                src5_neon = vld1q_s16((&inp_copy[col + 4]));
                src6_neon = vld1q_s16((&inp_copy[col + 5]));
                src7_neon = vld1q_s16((&inp_copy[col + 6]));
                src8_neon = vld1q_s16((&inp_copy[col + 7]));

                src_temp3_neon = vzip1q_s16(src1_neon, src2_neon);
                res_temp1_neon = vmadd_s16(src_temp3_neon, coeff0_1_neon);
                src_temp7_neon = vzip2q_s16(src1_neon, src2_neon);
                res_temp7_neon = vmadd_s16(src_temp7_neon, coeff0_1_neon);

                src_temp4_neon = vzip1q_s16(src3_neon, src4_neon);
                res_temp2_neon = vmadd_s16(src_temp4_neon, coeff2_3_neon);
                src_temp8_neon = vzip2q_s16(src3_neon, src4_neon);
                res_temp8_neon = vmadd_s16(src_temp8_neon, coeff2_3_neon);

                src_temp5_neon = vzip1q_s16(src5_neon, src6_neon);
                res_temp3_neon = vmadd_s16(src_temp5_neon, coeff4_5_neon);
                src_temp9_neon = vzip2q_s16(src5_neon, src6_neon);
                res_temp9_neon = vmadd_s16(src_temp9_neon, coeff4_5_neon);

                src_temp6_neon = vzip1q_s16(src7_neon, src8_neon);
                res_temp4_neon = vmadd_s16(src_temp6_neon, coeff6_7_neon);
                src_temp0_neon = vzip2q_s16(src7_neon, src8_neon);
                res_temp0_neon = vmadd_s16(src_temp0_neon, coeff6_7_neon);

                res_temp5_neon = vaddq_s32(res_temp1_neon, res_temp2_neon);
                res_temp6_neon = vaddq_s32(res_temp3_neon, res_temp4_neon);
                res_temp5_neon = vaddq_s32(res_temp5_neon, res_temp6_neon);

                res_temp6_neon = vaddq_s32(res_temp7_neon, res_temp8_neon);
                res_temp7_neon = vaddq_s32(res_temp9_neon, res_temp0_neon);
                res_temp8_neon = vaddq_s32(res_temp6_neon, res_temp7_neon);

                /* Add offset */
                res_temp6_neon = vaddq_s32(res_temp5_neon, offset_neon);
                res_temp7_neon = vaddq_s32(res_temp8_neon, offset_neon);

                /* Shift */
                res_temp6_neon = vshlq_s32(res_temp6_neon, shift_neon);
                res_temp7_neon = vshlq_s32(res_temp7_neon, shift_neon);

                /* Pack result to 16 bits */
                res_temp5_neon = vcombine_s16(vqmovn_s32(res_temp6_neon), vqmovn_s32(res_temp7_neon));

                /* to store the 8 pixels res. */
                vst1q_s16((dst_copy + col), res_temp5_neon);
            }
            inp_copy += src_stride;
            dst_copy += dst_stride;
        }
    }
    else if (rem_w > 3)
    {
        inp_copy = src_tmp + ((width >> 3) << 3);
        dst_copy = pred + ((width >> 3) << 3);

        for (row = 0; row < height; ++row)
        {
            /* load pixel values */
            src1_neon = vld1q_s16((&inp_copy[0]));
            src2_neon = vld1q_s16((&inp_copy[1]));
            src3_neon = vld1q_s16((&inp_copy[2]));
            src4_neon = vld1q_s16((&inp_copy[3]));
            src5_neon = vld1q_s16((&inp_copy[4]));
            src6_neon = vld1q_s16((&inp_copy[5]));
            src7_neon = vld1q_s16((&inp_copy[6]));
            src8_neon = vld1q_s16((&inp_copy[7]));

            src_temp3_neon = vzip1q_s16(src1_neon, src2_neon);
            res_temp1_neon = vmadd_s16(src_temp3_neon, coeff0_1_neon);

            src_temp4_neon = vzip1q_s16(src3_neon, src4_neon);
            res_temp2_neon = vmadd_s16(src_temp4_neon, coeff2_3_neon);

            src_temp5_neon = vzip1q_s16(src5_neon, src6_neon);
            res_temp3_neon = vmadd_s16(src_temp5_neon, coeff4_5_neon);

            src_temp6_neon = vzip1q_s16(src7_neon, src8_neon);
            res_temp4_neon = vmadd_s16(src_temp6_neon, coeff6_7_neon);

            res_temp5_neon = vaddq_s32(res_temp1_neon, res_temp2_neon);
            res_temp6_neon = vaddq_s32(res_temp3_neon, res_temp4_neon);
            res_temp5_neon = vaddq_s32(res_temp5_neon, res_temp6_neon);
            /* Add offset */
            res_temp6_neon = vaddq_s32(res_temp5_neon, offset_neon);
            /* Shift */
            res_temp6_neon = vshlq_s32(res_temp6_neon, shift_neon);
            /* Pack to 16 bits */
            res_temp5_neon = vcombine_s16(vqmovn_s32(res_temp6_neon), vqmovn_s32(res_temp6_neon));

            /* to store the 1st 4 pixels res. */
            vst1_s16((dst_copy), vget_low_s16(res_temp5_neon));
            inp_copy += src_stride;
            dst_copy += dst_stride;
        }
    }
}

void xeve_mc_filter_l_8pel_vert_clip_neon(s16* ref,
    int src_stride,
    s16* pred,
    int dst_stride,
    const s16* coeff,
    int width,
    int height,
    int min_val,
    int max_val,
    int offset,
    int shift)
{
    int row, col, rem_w;
    s16 const* src_tmp;
    s16 const* inp_copy;
    s16* dst_copy;

    int16x8_t coeff0_1_neon, coeff2_3_neon, coeff4_5_neon, coeff6_7_neon;
    int16x8_t r0_neon, r1_neon, r2_neon, r3_neon, r4_neon, r5_neon, r6_neon, r7_neon, r8_neon, r9_neon;
    int16x8_t r2_1r_neon, r2_2r_neon, r2_3r_neon, r2_4r_neon, r2_5r_neon, r2_6r_neon, r2_7r_neon, r2_8r_neon;
    int16x8_t r3_1r_neon, r3_2r_neon, r3_3r_neon, r3_4r_neon, r3_5r_neon, r3_6r_neon, r3_7r_neon, r3_8r_neon;

    int16x8_t min = vdupq_n_s16(min_val);
    int16x8_t max = vdupq_n_s16(max_val);
    int32x4_t offset_neon = vdupq_n_s32(offset); /* for offset addition */
    int32x4_t shift_neon = vdupq_n_s32(-shift);

    src_tmp = ref;
    rem_w = width;
    inp_copy = ref;
    dst_copy = pred;

    coeff0_1_neon = vld1q_s16(coeff);
    coeff2_3_neon = vdupq_n_s32(vgetq_lane_s32((coeff0_1_neon), (1)));
    coeff4_5_neon = vdupq_n_s32(vgetq_lane_s32((coeff0_1_neon), (2)));
    coeff6_7_neon = vdupq_n_s32(vgetq_lane_s32((coeff0_1_neon), (3)));
    coeff0_1_neon = vdupq_n_s32(vgetq_lane_s32((coeff0_1_neon), (0)));

    if (rem_w > 7)
    {
        for (row = 0; row < height; ++row)
        {
            int cnt = 0;
            for (col = width; col > 7; col -= 8)
            {
                r2_1r_neon = vld1q_s16((inp_copy + cnt));
                r2_2r_neon = vld1q_s16((inp_copy + src_stride + cnt));
                r2_3r_neon = vld1q_s16((inp_copy + (src_stride << 1) + cnt));
                r2_4r_neon = vld1q_s16((inp_copy + (src_stride * 3) + cnt));
                r2_5r_neon = vld1q_s16((inp_copy + (src_stride << 2) + cnt));
                r2_6r_neon = vld1q_s16((inp_copy + (src_stride * 5) + cnt));
                r2_7r_neon = vld1q_s16((inp_copy + (src_stride * 6) + cnt));
                r2_8r_neon = vld1q_s16((inp_copy + (src_stride * 7) + cnt));

                r3_1r_neon = vzip1q_s16(r2_1r_neon, r2_2r_neon);
                r0_neon = vmadd_s16(r3_1r_neon, coeff0_1_neon);
                r3_2r_neon = vzip1q_s16(r2_3r_neon, r2_4r_neon);
                r1_neon = vmadd_s16(r3_2r_neon, coeff2_3_neon);
                r0_neon = vaddq_s32(r0_neon, r1_neon);

                r3_5r_neon = vzip2q_s16(r2_1r_neon, r2_2r_neon);
                r4_neon = vmadd_s16(r3_5r_neon, coeff0_1_neon);
                r3_6r_neon = vzip2q_s16(r2_3r_neon, r2_4r_neon);
                r5_neon = vmadd_s16(r3_6r_neon, coeff2_3_neon);
                r4_neon = vaddq_s32(r4_neon, r5_neon);

                r3_3r_neon = vzip1q_s16(r2_5r_neon, r2_6r_neon);
                r2_neon = vmadd_s16(r3_3r_neon, coeff4_5_neon);
                r3_4r_neon = vzip1q_s16(r2_7r_neon, r2_8r_neon);
                r3_neon = vmadd_s16(r3_4r_neon, coeff6_7_neon);
                r2_neon = vaddq_s32(r2_neon, r3_neon);

                r3_7r_neon = vzip2q_s16(r2_5r_neon, r2_6r_neon);
                r6_neon = vmadd_s16(r3_7r_neon, coeff4_5_neon);
                r3_8r_neon = vzip2q_s16(r2_7r_neon, r2_8r_neon);
                r7_neon = vmadd_s16(r3_8r_neon, coeff6_7_neon);
                r6_neon = vaddq_s32(r6_neon, r7_neon);
                                
                r0_neon = vaddq_s32(r0_neon, r2_neon);
                r4_neon = vaddq_s32(r4_neon, r6_neon);

                /* Add offset */
                r0_neon = vaddq_s32(r0_neon, offset_neon);
                r4_neon = vaddq_s32(r4_neon, offset_neon);
                /* Shift */
                r7_neon = vshlq_s32(r0_neon, shift_neon);
                r8_neon = vshlq_s32(r4_neon, shift_neon);
                /* Pack to 16 bits */
                r9_neon = vcombine_s16(vqmovn_s32(r7_neon), vqmovn_s32(r8_neon));
                /* Clip */
                r9_neon = vminq_s16(r9_neon, max);
                r9_neon = vmaxq_s16(r9_neon, min);

                vst1q_s16((dst_copy + cnt), r9_neon);
                cnt += 8;
            }
            inp_copy += (src_stride);
            dst_copy += (dst_stride);
        }
    }

    rem_w &= 0x7;

    if (rem_w > 3)
    {
        inp_copy = src_tmp + ((width >> 3) << 3);
        dst_copy = pred + ((width >> 3) << 3);

        for (row = 0; row < height; ++row)
        {
            r2_1r_neon = vcombine_s16(vld1_s16(inp_copy), vcreate_s16(0));
            r2_2r_neon = vcombine_s16(vld1_s16(inp_copy + (src_stride)), vcreate_s16(0));
            r2_3r_neon = vcombine_s16(vld1_s16(inp_copy + (src_stride << 1)), vcreate_s16(0));
            r2_4r_neon = vcombine_s16(vld1_s16(inp_copy + (3 * src_stride)), vcreate_s16(0));
            r2_5r_neon = vcombine_s16(vld1_s16(inp_copy + (src_stride << 2)), vcreate_s16(0));
            r2_6r_neon = vcombine_s16(vld1_s16(inp_copy + (5 * src_stride)), vcreate_s16(0));
            r2_7r_neon = vcombine_s16(vld1_s16(inp_copy + (6 * src_stride)), vcreate_s16(0));
            r2_8r_neon = vcombine_s16(vld1_s16(inp_copy + (7 * src_stride)), vcreate_s16(0));

            r3_1r_neon = vzip1q_s16(r2_1r_neon, r2_2r_neon);
            r0_neon = vmadd_s16(r3_1r_neon, coeff0_1_neon);
            r3_2r_neon = vzip1q_s16(r2_3r_neon, r2_4r_neon);
            r1_neon = vmadd_s16(r3_2r_neon, coeff2_3_neon);
            r4_neon = vaddq_s32(r0_neon, r1_neon);

            r3_3r_neon = vzip1q_s16(r2_5r_neon, r2_6r_neon);
            r2_neon = vmadd_s16(r3_3r_neon, coeff4_5_neon);
            r3_4r_neon = vzip1q_s16(r2_7r_neon, r2_8r_neon);
            r3_neon = vmadd_s16(r3_4r_neon, coeff6_7_neon);            
            r5_neon = vaddq_s32(r2_neon, r3_neon);

            r6_neon = vaddq_s32(r4_neon, r5_neon);

            /* Add offset */
            r7_neon = vaddq_s32(r6_neon, offset_neon);
            /* Shift */
            r8_neon = vshlq_s32(r7_neon, shift_neon);
            /* Pack to 16 bits */
            r9_neon = vcombine_s16(vqmovn_s32(r8_neon), vqmovn_s32(r8_neon));
            /* Clip */
            r9_neon = vminq_s16(r9_neon, max);
            r9_neon = vmaxq_s16(r9_neon, min);
            /* Store */
            vst1_s16((dst_copy), vget_low_s16(r9_neon));
            
            inp_copy += (src_stride);
            dst_copy += (dst_stride);
        }
    }

    rem_w &= 0x3;

    if (rem_w)
    {
        inp_copy = src_tmp + ((width >> 2) << 2);
        dst_copy = pred + ((width >> 2) << 2);

        for (row = 0; row < height; ++row)
        {
            for (col = 0; col < rem_w; ++col)
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

void xeve_mc_filter_c_4pel_horz_neon(s16* ref, int src_stride, s16* pred, int dst_stride, const s16* coeff
    , int width, int height, int min_val, int max_val, int offset, int shift, s8  is_last)
{
    int row, col, rem_w, rem_h, cnt;
    int src_stride2, src_stride3;
    s16* inp_copy;
    s16* dst_copy;

    int16x8_t offset_neon = vdupq_n_s32(offset);
    int16x8_t min_neon = vdupq_n_s16(min_val);
    int16x8_t max_neon = vdupq_n_s16(max_val);
    int16x8_t coeff0_1_neon, coeff2_3_neon, mask;
    int16x8_t res0, res1, res2, res3;
    int16x8_t row11, row12, row13, row14, row21, row22, row23, row24;
    int16x8_t row31, row32, row33, row34, row41, row42, row43, row44;
    int32x4_t shift_neon;

    src_stride2 = (src_stride << 1);
    src_stride3 = (src_stride * 3);

    rem_w = width;
    inp_copy = ref;
    dst_copy = pred;
    shift_neon = vdupq_n_s32(-shift);
    coeff0_1_neon = vld1q_s16(coeff);
    {
        rem_h = height & 0x3;
        rem_w = width;
        coeff2_3_neon = vdupq_n_s32(vgetq_lane_s32((coeff0_1_neon), (1)));
        coeff0_1_neon = vdupq_n_s32(vgetq_lane_s32((coeff0_1_neon), (0)));
 
        if (rem_w > 7)
        {
            cnt = 0;
            for (row = 0; row < height; row += 4)
            {
                for (col = width; col > 7; col -= 8)
                {
                    row11 = vld1q_s16((inp_copy + cnt));
                    row12 = vld1q_s16((inp_copy + cnt + 1));
                    row13 = vld1q_s16((inp_copy + cnt + 2));
                    row14 = vld1q_s16((inp_copy + cnt + 3));

                    row21 = vld1q_s16((inp_copy + src_stride + cnt));
                    row22 = vld1q_s16((inp_copy + src_stride + cnt + 1));
                    row23 = vld1q_s16((inp_copy + src_stride + cnt + 2));
                    row24 = vld1q_s16((inp_copy + src_stride + cnt + 3)); 

                    row31 = vld1q_s16((inp_copy + src_stride2 + cnt));
                    row32 = vld1q_s16((inp_copy + src_stride2 + cnt + 1));
                    row33 = vld1q_s16((inp_copy + src_stride2 + cnt + 2));
                    row34 = vld1q_s16((inp_copy + src_stride2 + cnt + 3));

                    row41 = vld1q_s16((inp_copy + src_stride3 + cnt));
                    row42 = vld1q_s16((inp_copy + src_stride3 + cnt + 1));
                    row43 = vld1q_s16((inp_copy + src_stride3 + cnt + 2));
                    row44 = vld1q_s16((inp_copy + src_stride3 + cnt + 3));

                    row11 = vmadd_s16(row11, coeff0_1_neon);
                    row13 = vmadd_s16(row13, coeff2_3_neon);
                    row11 = vaddq_s32(row11, row13);

                    /* Offset and shift */
                    row11 = vaddq_s32(row11, offset_neon);
                    row11 = vshlq_s32(row11, shift_neon);

                    row12 = vmadd_s16(row12, coeff0_1_neon);
                    row14 = vmadd_s16(row14, coeff2_3_neon);
                    row12 = vaddq_s32(row12, row14);

                    /* Offset and shift */
                    row12 = vaddq_s32(row12, offset_neon);
                    row12 = vshlq_s32(row12, shift_neon);

                    row21 = vmadd_s16(row21, coeff0_1_neon);
                    row23 = vmadd_s16(row23, coeff2_3_neon);
                    row21 = vaddq_s32(row21, row23);

                    /* Offset and shift */
                    row21 = vaddq_s32(row21, offset_neon);
                    row21 = vshlq_s32(row21, shift_neon);

                    row22 = vmadd_s16(row22, coeff0_1_neon);
                    row24 = vmadd_s16(row24, coeff2_3_neon);
                    row22 = vaddq_s32(row22, row24);
                    
                    /* Offset and shift */
                    row22 = vaddq_s32(row22, offset_neon);
                    row22 = vshlq_s32(row22, shift_neon);

                    row31 = vmadd_s16(row31, coeff0_1_neon);
                    row33 = vmadd_s16(row33, coeff2_3_neon);
                    row31 = vaddq_s32(row31, row33);

                    /* Offset and shift */
                    row31 = vaddq_s32(row31, offset_neon);
                    row31 = vshlq_s32(row31, shift_neon);

                    row32 = vmadd_s16(row32, coeff0_1_neon);
                    row34 = vmadd_s16(row34, coeff2_3_neon);
                    row32 = vaddq_s32(row32, row34);

                    /* Offset and shift */
                    row32 = vaddq_s32(row32, offset_neon);
                    row32 = vshlq_s32(row32, shift_neon);

                    row41 = vmadd_s16(row41, coeff0_1_neon);
                    row43 = vmadd_s16(row43, coeff2_3_neon);
                    row41 = vaddq_s32(row41, row43);

                    /* Offset and shift */
                    row41 = vaddq_s32(row41, offset_neon);
                    row41 = vshlq_s32(row41, shift_neon);

                    row42 = vmadd_s16(row42, coeff0_1_neon);
                    row44 = vmadd_s16(row44, coeff2_3_neon);
                    row42 = vaddq_s32(row42, row44);
                    row42 = vaddq_s32(row42, offset_neon);
                    row42 = vshlq_s32(row42, shift_neon);
                    
                    /* Pack to 16 bits */
                    row11 = vcombine_s16(vqmovn_s32(row11), vqmovn_s32(row21));
                    row12 = vcombine_s16(vqmovn_s32(row12), vqmovn_s32(row22));
                    row31 = vcombine_s16(vqmovn_s32(row31), vqmovn_s32(row41));
                    row32 = vcombine_s16(vqmovn_s32(row32), vqmovn_s32(row42));

                    res0 = vzip1q_s16(row11, row12);
                    res1 = vzip2q_s16(row11, row12);
                    res2 = vzip1q_s16(row31, row32);
                    res3 = vzip2q_s16(row31, row32);
                    
                    /* Clip */
                    if (is_last)
                    {
                        mask = vcgtq_s16(res0, min_neon);
                        res0 = vorrq_s32(vandq_s16(mask, res0), vbicq_s32(min_neon, mask));
                        mask = vcltq_s16(res0, max_neon);
                        res0 = vorrq_s32(vandq_s16(mask, res0), vbicq_s32(max_neon, mask));

                        mask = vcgtq_s16(res1, min_neon);
                        res1 = vorrq_s32(vandq_s16(mask, res1), vbicq_s32(min_neon, mask));
                        mask = vcltq_s16(res1, max_neon);
                        res1 = vorrq_s32(vandq_s16(mask, res1), vbicq_s32(max_neon, mask));

                        mask = vcgtq_s16(res2, min_neon);
                        res2 = vorrq_s32(vandq_s16(mask, res2), vbicq_s32(min_neon, mask));
                        mask = vcltq_s16(res2, max_neon);
                        res2 = vorrq_s32(vandq_s16(mask, res2), vbicq_s32(max_neon, mask));

                        mask = vcgtq_s16(res3, min_neon);
                        res3 = vorrq_s32(vandq_s16(mask, res3), vbicq_s32(min_neon, mask));
                        mask = vcltq_s16(res3, max_neon);
                        res3 = vorrq_s32(vandq_s16(mask, res3), vbicq_s32(max_neon, mask));
                    }
                    /* Store */
                    vst1q_s16((dst_copy + cnt), res0);
                    vst1q_s16((dst_copy + dst_stride + cnt), res1);
                    vst1q_s16((dst_copy + (dst_stride << 1) + cnt), res2);
                    vst1q_s16((dst_copy + (dst_stride * 3) + cnt), res3);

                    cnt += 8;
                }

                cnt = 0;
                inp_copy += (src_stride << 2);
                dst_copy += (dst_stride << 2);
            }

 
            for (row = 0; row < rem_h; ++row)
            {
                cnt = 0;
                for (col = width; col > 7; col -= 8)
                {
                    /* Load the data */
                    row11 = vld1q_s16((inp_copy + cnt));
                    row12 = vld1q_s16((inp_copy + cnt + 1));
                    row13 = vld1q_s16((inp_copy + cnt + 2));
                    row14 = vld1q_s16((inp_copy + cnt + 3));

                    row11 = vmadd_s16(row11, coeff0_1_neon);
                    row13 = vmadd_s16(row13, coeff2_3_neon);
                    row11 = vaddq_s32(row11, row13);
                    
                    /* Offset and shift */
                    row11 = vaddq_s32(row11, offset_neon);
                    row11 = vshlq_s32(row11, shift_neon);

                    row12 = vmadd_s16(row12, coeff0_1_neon);
                    row14 = vmadd_s16(row14, coeff2_3_neon);
                    row12 = vaddq_s32(row12, row14);
                    
                    /* Offset and shift */
                    row12 = vaddq_s32(row12, offset_neon);
                    row12 = vshlq_s32(row12, shift_neon);
                    
                    /* Pack to 16 bits */
                    row11 = vcombine_s16(vqmovn_s32(row11), vqmovn_s32(row12));

                    res3 = vcombine_s64(vget_high_s64(row11), vget_high_s64(row11));
                    res0 = vzip1q_s16(row11, res3);

                    /* clip */
                    if (is_last)
                    {
                        mask = vcgtq_s16(res0, min_neon);
                        res0 = vorrq_s32(vandq_s16(mask, res0), vbicq_s32(min_neon, mask));
                        mask = vcltq_s16(res0, max_neon);
                        res0 = vorrq_s32(vandq_s16(mask, res0), vbicq_s32(max_neon, mask));
                    }

                    vst1q_s16((dst_copy + cnt), res0);
                    cnt += 8;
                }
                inp_copy += (src_stride);
                dst_copy += (dst_stride);
            }
        }

        rem_w &= 0x7;

 
        if (rem_w > 3)
        {
            inp_copy = ref + ((width >> 3) << 3);
            dst_copy = pred + ((width >> 3) << 3);

            for (row = 0; row < height; row += 4)
            {
                /* Load the data */
                row11 = vcombine_s16(vld1_s16(inp_copy), vcreate_s16(0));
                row12 = vcombine_s16(vld1_s16(inp_copy + 1), vcreate_s16(0));
                row13 = vcombine_s16(vld1_s16(inp_copy + 2), vcreate_s16(0));
                row14 = vcombine_s16(vld1_s16(inp_copy + 3), vcreate_s16(0));
 
                row21 = vcombine_s16(vld1_s16(inp_copy + src_stride), vcreate_s16(0));
                row22 = vcombine_s16(vld1_s16(inp_copy + src_stride + 1), vcreate_s16(0));
                row23 = vcombine_s16(vld1_s16(inp_copy + src_stride + 2), vcreate_s16(0));
                row24 = vcombine_s16(vld1_s16(inp_copy + src_stride + 3), vcreate_s16(0));

                row31 = vcombine_s16(vld1_s16(inp_copy + src_stride2), vcreate_s16(0));
                row32 = vcombine_s16(vld1_s16(inp_copy + src_stride2 + 1), vcreate_s16(0));
                row33 = vcombine_s16(vld1_s16(inp_copy + src_stride2 + 2), vcreate_s16(0));
                row34 = vcombine_s16(vld1_s16(inp_copy + src_stride2 + 3), vcreate_s16(0));
 
                row41 = vcombine_s16(vld1_s16(inp_copy + src_stride3), vcreate_s16(0));
                row42 = vcombine_s16(vld1_s16(inp_copy + src_stride3 + 1), vcreate_s16(0));
                row43 = vcombine_s16(vld1_s16(inp_copy + src_stride3 + 2), vcreate_s16(0));
                row44 = vcombine_s16(vld1_s16(inp_copy + src_stride3 + 3), vcreate_s16(0));

                row11 = vzip1q_s32(row11, row12);
                row13 = vzip1q_s32(row13, row14);
                row21 = vzip1q_s32(row21, row22);
                row23 = vzip1q_s32(row23, row24);
                row31 = vzip1q_s32(row31, row32);
                row33 = vzip1q_s32(row33, row34);
                row41 = vzip1q_s32(row41, row42);
                row43 = vzip1q_s32(row43, row44);

                row11 = vmadd_s16(row11, coeff0_1_neon);
                row13 = vmadd_s16(row13, coeff2_3_neon);
                row11 = vaddq_s32(row11, row13);

                /* Offset and shift */
                row11 = vaddq_s32(row11, offset_neon);
                row11 = vshlq_s32(row11, shift_neon);

                row21 = vmadd_s16(row21, coeff0_1_neon);
                row23 = vmadd_s16(row23, coeff2_3_neon);
                row21 = vaddq_s32(row21, row23);

                /* Offset and shift */
                row21 = vaddq_s32(row21, offset_neon);
                row21 = vshlq_s32(row21, shift_neon);

                row31 = vmadd_s16(row31, coeff0_1_neon);
                row33 = vmadd_s16(row33, coeff2_3_neon);
                row31 = vaddq_s32(row31, row33);

                /* Offset and shift */
                row31 = vaddq_s32(row31, offset_neon);
                row31 = vshlq_s32(row31, shift_neon);

                row41 = vmadd_s16(row41, coeff0_1_neon);
                row43 = vmadd_s16(row43, coeff2_3_neon);
                row41 = vaddq_s32(row41, row43);
                
                /* Offset and shift */
                row41 = vaddq_s32(row41, offset_neon);
                row41 = vshlq_s32(row41, shift_neon);
                
                /* Pack to 16 bits */
                res0 = vcombine_s16(vqmovn_s32(row11), vqmovn_s32(row21));
                res1 = vcombine_s16(vqmovn_s32(row31), vqmovn_s32(row41));
                
                /* Clip */
                if (is_last)
                {
                    mask = vcgtq_s16(res0, min_neon);
                    res0 = vorrq_s32(vandq_s16(mask, res0), vbicq_s32(min_neon, mask));
                    mask = vcltq_s16(res0, max_neon);
                    res0 = vorrq_s32(vandq_s16(mask, res0), vbicq_s32(max_neon, mask));

                    mask = vcgtq_s16(res1, min_neon);
                    res1 = vorrq_s32(vandq_s16(mask, res1), vbicq_s32(min_neon, mask));
                    mask = vcltq_s16(res1, max_neon);
                    res1 = vorrq_s32(vandq_s16(mask, res1), vbicq_s32(max_neon, mask));
                }
                /* Store */
                vst1_s16((dst_copy), vget_low_s16(res0));
                vst1_s16((dst_copy + dst_stride), vget_high_s64(res0));
                vst1_s16((dst_copy + (dst_stride << 1)), vget_low_s16(res1));
                vst1_s16((dst_copy + (dst_stride * 3)), vget_high_s64(res1));

                inp_copy += (src_stride << 2);
                dst_copy += (dst_stride << 2);
            }

            for (row = 0; row < rem_h; ++row)
            {
                row11 = vcombine_s16(vld1_s16(inp_copy), vcreate_s16(0));
                row12 = vcombine_s16(vld1_s16(inp_copy + 1), vcreate_s16(0));
                row13 = vcombine_s16(vld1_s16(inp_copy + 2), vcreate_s16(0));
                row14 = vcombine_s16(vld1_s16(inp_copy + 3), vcreate_s16(0));

                row11 = vzip1q_s32(row11, row12);
                row11 = vmadd_s16(row11, coeff0_1_neon);

                row13 = vzip1q_s32(row13, row14);
                row13 = vmadd_s16(row13, coeff2_3_neon);

                row11 = vaddq_s32(row11, row13);
                
                /* Add offset */
                row11 = vaddq_s32(row11, offset_neon);
                
                /* shift */
                row11 = vshlq_s32(row11, shift_neon);
                
                /* pack to 16 bits */
                res1 = vcombine_s16(vqmovn_s32(row11), vqmovn_s32(row11));
                
                /* clip */
                if (is_last)
                {
                    mask = vcgtq_s16(res1, min_neon);
                    res1 = vorrq_s32(vandq_s16(mask, res1), vbicq_s32(min_neon, mask));
                    mask = vcltq_s16(res1, max_neon);
                    res1 = vorrq_s32(vandq_s16(mask, res1), vbicq_s32(max_neon, mask));
                }
 
                vst1_s16((dst_copy), vget_low_s16(res1));

                inp_copy += (src_stride);
                dst_copy += (dst_stride);
            }
        }

        rem_w &= 0x3;
        if (rem_w)
        {
            inp_copy = ref + ((width >> 2) << 2);
            dst_copy = pred + ((width >> 2) << 2);

            for (row = 0; row < height; ++row)
            {
                for (col = 0; col < rem_w; ++col)
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
                inp_copy += (src_stride);
                dst_copy += (dst_stride);
            }
        }
    }
}

void xeve_mc_filter_c_4pel_vert_neon(s16* ref,
    int src_stride,
    s16* pred,
    int dst_stride,
    const s16* coeff,
    int width,
    int height,
    int min_val,
    int max_val,
    int offset,
    int shift,
    s8  is_last)
{
    int row, col, rem_w;
    s16 const* src_tmp;
    s16 const* inp_copy;
    s16* dst_copy;

    int16x8_t coeff0_1_neon, coeff2_3_neon, mask;
    int16x8_t s0_neon, s1_neon, s4_neon, s5_neon, s7_neon, s8_neon, s9_neon;
    int16x8_t src_0_neon, src_1_neon, src_2_neon, src_3_neon;
    int16x8_t src1_0_neon, src1_1_neon, src1_4_neon, src1_5_neon;
    int32x4_t shift_neon;

    int16x8_t min_neon = vdupq_n_s16(min_val);
    int16x8_t max_neon = vdupq_n_s16(max_val);
    int16x8_t offset_neon = vdupq_n_s32(offset);

    src_tmp = ref;
    rem_w = width;
    inp_copy = ref;
    dst_copy = pred;

    coeff0_1_neon = vld1q_s16(coeff);
    coeff2_3_neon = vdupq_n_s32(vgetq_lane_s32((coeff0_1_neon), (1)));
    coeff0_1_neon = vdupq_n_s32(vgetq_lane_s32((coeff0_1_neon), (0)));

    shift_neon = vdupq_n_s32(-shift);

    if (rem_w > 7)
    {
        for (row = 0; row < height; ++row)
        {
            int cnt = 0;
            for (col = width; col > 7; col -= 8)
            { 
                src_0_neon = vld1q_s16((inp_copy + cnt));
                src_1_neon = vld1q_s16((inp_copy + src_stride + cnt));
                src_2_neon = vld1q_s16((inp_copy + (src_stride << 1) + cnt));
                src_3_neon = vld1q_s16((inp_copy + (src_stride * 3) + cnt));

                src1_0_neon = vzip1q_s16(src_0_neon, src_1_neon); 
                s0_neon = vmadd_s16(src1_0_neon, coeff0_1_neon);
                
                src1_1_neon = vzip1q_s16(src_2_neon, src_3_neon);
                s1_neon = vmadd_s16(src1_1_neon, coeff2_3_neon);
                
                s0_neon = vaddq_s32(s0_neon, s1_neon);

                /* Offset and shift */
                s0_neon = vaddq_s32(s0_neon, offset_neon);
                s0_neon = vshlq_s32(s0_neon, shift_neon);

                src1_4_neon = vzip2q_s16(src_0_neon, src_1_neon);
                s4_neon = vmadd_s16(src1_4_neon, coeff0_1_neon);
                
                src1_5_neon = vzip2q_s16(src_2_neon, src_3_neon);
                s5_neon = vmadd_s16(src1_5_neon, coeff2_3_neon);
                
                s4_neon = vaddq_s32(s4_neon, s5_neon);

                /* Offset and shift */
                s4_neon = vaddq_s32(s4_neon, offset_neon);
                s4_neon = vshlq_s32(s4_neon, shift_neon);
                
                /* Pack to 16 bits */
                s9_neon = vcombine_s16(vqmovn_s32(s0_neon), vqmovn_s32(s4_neon));
                
                /* Clip */
                if (is_last)
                {
                    mask = vcgtq_s16(s9_neon, min_neon);
                    s9_neon = vorrq_s32(vandq_s16(mask, s9_neon), vbicq_s32(min_neon, mask));

                    mask = vcltq_s16(s9_neon, max_neon);
                    s9_neon = vorrq_s32(vandq_s16(mask, s9_neon), vbicq_s32(max_neon, mask));
                }
                /* Store */
                vst1q_s16((dst_copy + cnt), s9_neon);

                cnt += 8;
            }
            inp_copy += (src_stride);
            dst_copy += (dst_stride);
        }
    }

    rem_w &= 0x7;

    if (rem_w > 3)
    {
        inp_copy = src_tmp + ((width >> 3) << 3);
        dst_copy = pred + ((width >> 3) << 3);

        for (row = 0; row < height; ++row)
        {
 
            src_0_neon = vcombine_s16(vld1_s16(inp_copy), vcreate_s16(0));
            src_1_neon = vcombine_s16(vld1_s16(inp_copy + (src_stride)), vcreate_s16(0));
            src_2_neon = vcombine_s16(vld1_s16(inp_copy + (2 * src_stride)), vcreate_s16(0));
            src_3_neon = vcombine_s16(vld1_s16(inp_copy + (3 * src_stride)), vcreate_s16(0));

            src1_0_neon = vzip1q_s16(src_0_neon, src_1_neon);
            s0_neon = vmadd_s16(src1_0_neon, coeff0_1_neon);
 
            src1_1_neon = vzip1q_s16(src_2_neon, src_3_neon);
            s1_neon = vmadd_s16(src1_1_neon, coeff2_3_neon);

            s4_neon = vaddq_s32(s0_neon, s1_neon);

            /* Add offset */
            s7_neon = vaddq_s32(s4_neon, offset_neon);
            
            /* Shift */
            s8_neon = vshlq_s32(s7_neon, shift_neon);

            /* Pack to 16 bits*/
            s9_neon = vcombine_s16(vqmovn_s32(s8_neon), vqmovn_s32(s8_neon));
            
            /* Clip */
            if (is_last)
            {
                mask = vcgtq_s16(s9_neon, min_neon);
                s9_neon = vorrq_s32(vandq_s16(mask, s9_neon), vbicq_s32(min_neon, mask));
                mask = vcltq_s16(s9_neon, max_neon);
                s9_neon = vorrq_s32(vandq_s16(mask, s9_neon), vbicq_s32(max_neon, mask));
            }
            /* store */
            vst1_s16((dst_copy), vget_low_s16(s9_neon));

            inp_copy += (src_stride);
            dst_copy += (dst_stride);
        }
    }

    rem_w &= 0x3;

    if (rem_w)
    {
        inp_copy = src_tmp + ((width >> 2) << 2);
        dst_copy = pred + ((width >> 2) << 2);

        for (row = 0; row < height; ++row)
        {
            for (col = 0; col < rem_w; ++col)
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


void xeve_mc_l_00_neon(pel* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel* pred, int w, int h, int bit_depth, const s16(*mc_l_coeff)[8])
{
    int i, j;
    gmv_x >>= 4;
    gmv_y >>= 4;
    ref += gmv_y * s_ref + gmv_x;

    if (((w & 0x7) == 0) && ((h & 1) == 0))
    {
        int16x8_t m00, m01;

        for (i = 0; i < h; i += 2)
        {
            for (j = 0; j < w; j += 8)
            {
                m00 = vld1q_s16((ref + j));
                m01 = vld1q_s16((ref + j + s_ref));

                vst1q_s16((pred + j), m00);
                vst1q_s16((pred + j + s_pred), m01);
            }
            pred += s_pred << 1;
            ref += s_ref << 1;
        }
    }
    else if ((w & 0x3) == 0)
    {
        int16x4_t m00;

        for (i = 0; i < h; ++i)
        {
            for (j = 0; j < w; j += 4)
            {
                m00 = vld1_s16((ref + j));
                vst1_s16((pred + j), m00);
            }
            pred += s_pred;
            ref += s_ref;
        }
    }
    else
    {
        int size = sizeof(pel) * w;
           
        for (int i = 0; i < h; i++)
        {
            xeve_mcpy(pred, ref, size);
            pred += s_pred;
            ref += s_ref;
        }
    }
}

void xeve_mc_c_00_neon(pel* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel* pred, int w, int h, int bit_depth, const s16(*mc_c_coeff)[4])
{
    int i, j;
    
    gmv_x >>= 5;
    gmv_y >>= 5;
    ref += gmv_y * s_ref + gmv_x;

    if (((w & 0x7) == 0) && ((h & 1) == 0))
    {
        int16x8_t m00, m01;

        for (i = 0; i < h; i += 2)
        {
            for (j = 0; j < w; j += 8)
            {
                m00 = vld1q_s16((ref + j));
                m01 = vld1q_s16((ref + j + s_ref));

                vst1q_s16((pred + j), m00);
                vst1q_s16((pred + j + s_pred), m01);
            }
            pred += s_pred << 1;
            ref += s_ref << 1;
        }
    }
    else if ((w & 0x3) == 0)
    {
        int16x4_t m00;

        for (i = 0; i < h; ++i)
        {
            for (j = 0; j < w; j += 4)
            {
                m00 = vld1_s16((ref + j));
                vst1_s16((pred + j), m00);
            }
            pred += s_pred;
            ref += s_ref;
        }
    }
    else
    {
        int size = sizeof(pel) * w;
        for (int i = 0; i < h; i++)
        {
            xeve_mcpy(pred, ref, size);
            pred += s_pred;
            ref += s_ref;
        }
    }
}

void xeve_mc_l_n0_neon(pel* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel* pred, int w, int h, int bit_depth, const s16(*mc_l_coeff)[8])
{
    int dx = gmv_x & 15;
    ref += (gmv_y >> 4) * s_ref + (gmv_x >> 4) - 3;

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    xeve_mc_filter_l_8pel_horz_clip_neon(ref, s_ref, pred, s_pred, mc_l_coeff[dx], w, h, min, max, MAC_ADD_N0, MAC_SFT_N0);
}

void xeve_mc_l_0n_neon(pel* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel* pred, int w, int h, int bit_depth, const s16(*mc_l_coeff)[8])
{
    int dy = gmv_y & 15;
    ref += ((gmv_y >> 4) - 3) * s_ref + (gmv_x >> 4);

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    xeve_mc_filter_l_8pel_vert_clip_neon(ref, s_ref, pred, s_pred, mc_l_coeff[dy], w, h, min, max, MAC_ADD_0N, MAC_SFT_0N);
}

void xeve_mc_l_nn_neon(s16* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16* pred, int w, int h, int bit_depth, const s16(*mc_l_coeff)[8])
{
    s16         buf[(MAX_CU_SIZE + MC_IBUF_PAD_L) * (MAX_CU_SIZE + MC_IBUF_PAD_L)];
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

    xeve_mc_filter_l_8pel_horz_no_clip_neon(ref, s_ref, buf, w, mc_l_coeff[dx], w, (h + 7), offset1, shift1);
    xeve_mc_filter_l_8pel_vert_clip_neon(buf, w, pred, s_pred, mc_l_coeff[dy], w, h, min, max, offset2, shift2);
}

/****************************************************************************
 * motion compensation for chroma
 ****************************************************************************/
void xeve_mc_c_n0_neon(s16* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16* pred, int w, int h, int bit_depth, const s16(*mc_c_coeff)[4])
{
    int  dx = gmv_x & 31;
    ref += (gmv_y >> 5) * s_ref + (gmv_x >> 5) - 1;

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    xeve_mc_filter_c_4pel_horz_neon(ref, s_ref, pred, s_pred, mc_c_coeff[dx], w, h, min, max, MAC_ADD_N0, MAC_SFT_N0, 1);
}

void xeve_mc_c_0n_neon(s16* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16* pred, int w, int h, int bit_depth, const s16(*mc_c_coeff)[4])
{
    int dy = gmv_y & 31;
    ref += ((gmv_y >> 5) - 1) * s_ref + (gmv_x >> 5);

    int max = ((1 << bit_depth) - 1);
    int min = 0;

    xeve_mc_filter_c_4pel_vert_neon(ref, s_ref, pred, s_pred, mc_c_coeff[dy], w, h, min, max, MAC_ADD_0N, MAC_SFT_0N, 1);
}

void xeve_mc_c_nn_neon(s16* ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16* pred, int w, int h, int bit_depth, const s16(*mc_c_coeff)[4])
{
    s16 buf[(MAX_CU_SIZE + MC_IBUF_PAD_C) * MAX_CU_SIZE];
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

    xeve_mc_filter_c_4pel_horz_neon(ref, s_ref, buf, w, mc_c_coeff[dx], w, (h + 3), min, max, offset1, shift1, 0);
    xeve_mc_filter_c_4pel_vert_neon(buf, w, pred, s_pred, mc_c_coeff[dy], w, h, min, max, offset2, shift2, 1);
}

const XEVE_MC_L xeve_tbl_mc_l_neon[2][2] =
{
    {
        xeve_mc_l_00_neon, /* dx == 0 && dy == 0 */
        xeve_mc_l_0n_neon  /* dx == 0 && dy != 0 */
    },
    {
        xeve_mc_l_n0_neon, /* dx != 0 && dy == 0 */
        xeve_mc_l_nn_neon  /* dx != 0 && dy != 0 */
    }
};

const XEVE_MC_C xeve_tbl_mc_c_neon[2][2] =
{
    {
        xeve_mc_c_00_neon, /* dx == 0 && dy == 0 */
        xeve_mc_c_0n_neon  /* dx == 0 && dy != 0 */
    },
    {
        xeve_mc_c_n0_neon, /* dx != 0 && dy == 0 */
        xeve_mc_c_nn_neon  /* dx != 0 && dy != 0 */
    }
};

void xeve_average_16b_no_clip_neon(s16* src, s16* ref, s16* dst, int s_src, int s_ref, int s_dst, int wd, int ht)
{
    s16* p0, * p1, * p2;
    int rem_h = ht;
    int rem_w;
    int i, j;

    int16x8_t src_neon, src_neon_1, src_neon_2, src_neon_3;
    int16x8_t pred_neon, pred_neon_1, pred_neon_2, pred_neon_3;
    int16x8_t temp_0, temp_1, temp_2, temp_3;
    int16x8_t offset_neon_x8, shift_neon_x8;
    
    int16x4_t src_neon_x4, src_neon_1_x4, src_neon_2_x4, src_neon_3_x4, pred_neon_x4, pred_neon_1_x4;
    int16x4_t pred_neon_2_x4, pred_neon_3_x4, temp_0_x4, temp_1_x4, temp_2_x4, temp_3_x4;
    int16x4_t offset_neon_x4, shift_neon_x4;

    p0 = src;
    p1 = ref;
    p2 = dst;

    int offset = 1;
    int shift = 1;

    offset_neon_x8 = vdupq_n_s16(offset); // for 8 16 bit buffers
    offset_neon_x4 = vdup_n_s16(offset); // for 4 16 bit buffers

    shift_neon_x8 = vdupq_n_s16(-shift);
    shift_neon_x4 = vdup_n_s16(-shift);

    if (rem_h >= 4)
    {
        for (i = 0; i < rem_h; i += 4)
        {
            p0 = src + (i * s_src);
            p1 = ref + (i * s_ref);
            p2 = dst + (i * s_dst);
            rem_w = wd;

            if (rem_w >= 8)
            {
                for (j = 0; j < rem_w; j += 8)
                {
                    src_neon = vld1q_s16((p0));
                    src_neon_1 = vld1q_s16((p0 + s_src));
                    src_neon_2 = vld1q_s16((p0 + (s_src << 1)));
                    src_neon_3 = vld1q_s16((p0 + (s_src * 3)));

                    pred_neon = vld1q_s16((p1));
                    pred_neon_1 = vld1q_s16((p1 + s_ref));
                    pred_neon_2 = vld1q_s16((p1 + (s_ref << 1)));
                    pred_neon_3 = vld1q_s16((p1 + (s_ref * 3)));

                    temp_0 = vaddq_s16(src_neon, pred_neon);
                    temp_1 = vaddq_s16(src_neon_1, pred_neon_1);
                    temp_2 = vaddq_s16(src_neon_2, pred_neon_2);
                    temp_3 = vaddq_s16(src_neon_3, pred_neon_3);

                    temp_0 = vaddq_s16(temp_0, offset_neon_x8);
                    temp_1 = vaddq_s16(temp_1, offset_neon_x8);
                    temp_2 = vaddq_s16(temp_2, offset_neon_x8);
                    temp_3 = vaddq_s16(temp_3, offset_neon_x8);

                    temp_0 = vshlq_s16(temp_0, shift_neon_x8);
                    temp_1 = vshlq_s16(temp_1, shift_neon_x8);
                    temp_2 = vshlq_s16(temp_2, shift_neon_x8);
                    temp_3 = vshlq_s16(temp_3, shift_neon_x8);

                    vst1q_s16((p2 + 0 * s_dst), temp_0);
                    vst1q_s16((p2 + 1 * s_dst), temp_1);
                    vst1q_s16((p2 + 2 * s_dst), temp_2);
                    vst1q_s16((p2 + 3 * s_dst), temp_3);

                    p0 += 8;
                    p1 += 8;
                    p2 += 8;
                }
            }

            rem_w &= 0x7;
            /* One 4 case */
            if (rem_w >= 4)
            {
                src_neon_x4 = vld1_s16((p0));
                src_neon_1_x4 = vld1_s16((p0 + s_src));
                src_neon_2_x4 = vld1_s16((p0 + (s_src << 1)));
                src_neon_3_x4 = vld1_s16((p0 + (s_src * 3)));

                pred_neon_x4 = vld1_s16((p1));
                pred_neon_1_x4 = vld1_s16((p1 + s_ref));
                pred_neon_2_x4 = vld1_s16((p1 + (s_ref << 1)));
                pred_neon_3_x4 = vld1_s16((p1 + (s_ref * 3)));

                temp_0_x4 = vadd_s16(src_neon_x4, pred_neon_x4);
                temp_1_x4 = vadd_s16(src_neon_1_x4, pred_neon_1_x4);
                temp_2_x4 = vadd_s16(src_neon_2_x4, pred_neon_2_x4);
                temp_3_x4 = vadd_s16(src_neon_3_x4, pred_neon_3_x4);

                temp_0_x4 = vadd_s16(temp_0_x4, offset_neon_x4);
                temp_1_x4 = vadd_s16(temp_1_x4, offset_neon_x4);
                temp_2_x4 = vadd_s16(temp_2_x4, offset_neon_x4);
                temp_3_x4 = vadd_s16(temp_3_x4, offset_neon_x4);


                temp_0_x4 = vshl_s16(temp_0_x4, shift_neon_x4);
                temp_1_x4 = vshl_s16(temp_1_x4, shift_neon_x4);
                temp_2_x4 = vshl_s16(temp_2_x4, shift_neon_x4);
                temp_3_x4 = vshl_s16(temp_3_x4, shift_neon_x4);

                vst1_s16((p2 + 0 * s_dst), temp_0_x4);
                vst1_s16((p2 + 1 * s_dst), temp_1_x4);
                vst1_s16((p2 + 2 * s_dst), temp_2_x4);
                vst1_s16((p2 + 3 * s_dst), temp_3_x4);

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

    if (rem_h >= 2)
    {
        p0 = src + ((ht >> 2) << 2) * s_src;
        p1 = ref + ((ht >> 2) << 2) * s_ref;
        p2 = dst + ((ht >> 2) << 2) * s_dst;
        {
            rem_w = wd;

            if (rem_w >= 8)
            {
                for (j = 0; j < rem_w; j += 8)
                {
                    src_neon = vld1q_s16((p0));
                    src_neon_1 = vld1q_s16((p0 + s_src));

                    pred_neon = vld1q_s16((p1));
                    pred_neon_1 = vld1q_s16((p1 + s_ref));

                    temp_0 = vaddq_s16(src_neon, pred_neon);
                    temp_1 = vaddq_s16(src_neon_1, pred_neon_1);

                    temp_0 = vaddq_s16(temp_0, offset_neon_x8);
                    temp_1 = vaddq_s16(temp_1, offset_neon_x8);

                    temp_0 = vshlq_s16(temp_0, shift_neon_x8);
                    temp_1 = vshlq_s16(temp_1, shift_neon_x8);

                    vst1q_s16((p2 + 0 * s_dst), temp_0);
                    vst1q_s16((p2 + 1 * s_dst), temp_1);

                    p0 += 8;
                    p1 += 8;
                    p2 += 8;
                }
            }

            rem_w &= 0x7;
            
            if (rem_w >= 4)
            {
                src_neon_x4 = vld1_s16((p0));
                src_neon_1_x4 = vld1_s16((p0 + s_src));

                pred_neon_x4 = vld1_s16((p1));
                pred_neon_1_x4 = vld1_s16((p1 + s_ref));

                temp_0_x4 = vadd_s16(src_neon_x4, pred_neon_x4);
                temp_1_x4 = vadd_s16(src_neon_1_x4, pred_neon_1_x4);

                temp_0_x4 = vadd_s16(temp_0_x4, offset_neon_x4);
                temp_1_x4 = vadd_s16(temp_1_x4, offset_neon_x4);

                temp_0_x4 = vshl_s16(temp_0_x4, shift_neon_x4);
                temp_1_x4 = vshl_s16(temp_1_x4, shift_neon_x4);

                vst1_s16((p2 + 0 * s_dst), temp_0_x4);
                vst1_s16((p2 + 1 * s_dst), temp_1_x4);

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
                    src_neon = vld1q_s16((p0));
                    pred_neon = vld1q_s16((p1));

                    temp_0 = vaddq_s16(src_neon, pred_neon);
                    temp_0 = vaddq_s16(temp_0, offset_neon_x8);

                    temp_0 = vshlq_s16(temp_0, shift_neon_x8);
                    vst1q_s16((p2 + 0 * s_dst), temp_0);

                    p0 += 8;
                    p1 += 8;
                    p2 += 8;
                }
            }

            rem_w &= 0x7;
            
            if (rem_w >= 4)
            {
                src_neon_x4 = vld1_s16((p0));
                pred_neon_x4 = vld1_s16((p1));

                temp_0_x4 = vadd_s16(src_neon_x4, pred_neon_x4);
                temp_0_x4 = vadd_s16(temp_0_x4, offset_neon_x4);
                temp_0_x4 = vshl_s16(temp_0_x4, shift_neon_x4);

                vst1_s16((p2 + 0 * s_dst), temp_0_x4);

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

#undef vmadd_s16
#undef vmadd1_s16
#endif /* X86_neon */

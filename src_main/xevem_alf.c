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

#include "xevem_alf.h"

void alf_init(ADAPTIVE_LOOP_FILTER * alf, int bit_depth)
{
    alf->clip_ranges.comp[0] = (CLIP_RANGE) { .min = 0, .max = (1 << bit_depth) - 1, .bd = bit_depth, .n = 0 };
    alf->clip_ranges.comp[1] = (CLIP_RANGE) { .min = 0, .max = (1 << bit_depth) - 1, .bd = bit_depth, .n = 0 };
    alf->clip_ranges.comp[2] = (CLIP_RANGE) { .min = 0, .max = (1 << bit_depth) - 1, .bd = bit_depth, .n = 0 };
    alf->clip_ranges.used = FALSE;
    alf->clip_ranges.chroma = FALSE;

    for (int compIdx = 0; compIdx < N_C; compIdx++)
    {
        alf->ctu_enable_flag[compIdx] = NULL;
    }

    alf->derive_classification_blk = alf_derive_classification_blk;
    alf->filter_5x5_blk = alf_filter_blk_5;
    alf->filter_7x7_blk = alf_filter_blk_7;
}

void alf_init_filter_shape(ALF_FILTER_SHAPE* filter_shape, int size)
{
    filter_shape->filterLength = size;
    filter_shape->num_coef = size * size / 4 + 1;
    filter_shape->filter_size = size * size / 2 + 1;

    if (size == 5)
    {
        xeve_mcpy(filter_shape->pattern, pattern5, sizeof(pattern5));
        xeve_mcpy(filter_shape->weights, weights5, sizeof(weights5));
        xeve_mcpy(filter_shape->golombIdx, golombIdx5, sizeof(golombIdx5));
        xeve_mcpy(filter_shape->pattern_to_large_filter, pattern_to_large_filter5, sizeof(pattern_to_large_filter5));
        filter_shape->filter_type = ALF_FILTER_5;
    }
    else if (size == 7)
    {
        xeve_mcpy(filter_shape->pattern, pattern7, sizeof(pattern7));
        xeve_mcpy(filter_shape->weights, weights7, sizeof(weights7));
        xeve_mcpy(filter_shape->golombIdx, golombIdx7, sizeof(golombIdx7));
        xeve_mcpy(filter_shape->pattern_to_large_filter, pattern_to_large_filter7, sizeof(pattern_to_large_filter7));
        filter_shape->filter_type = ALF_FILTER_7;
    }
    else
    {
        filter_shape->filter_type = ALF_NUM_OF_FILTER_TYPES;
        CHECK(0, "Wrong ALF filter shape");
    }
}

/*
* tmp_yuv -  destination, temporary buffer
* pointer tmp_yuv is assumed to point to interior point inside margins
* s - its stride
* rec - source, recovered buffer
* s2 - its stride
* w - width
* h - height
* m - margin size
*/
void alf_copy_and_extend_tile(pel* tmp_yuv, const int s, const pel* rec, const int s2, const int w, const int h, const int m)
{
    //copy
    for (int j = 0; j < h; j++)
    {
        xeve_mcpy(tmp_yuv + j * s, rec + j * s2, sizeof(pel) * w);
    }

    //extend
    pel * p = tmp_yuv;
    // do left and right margins
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < m; x++)
        {
            *(p - m + x) = p[0];
            p[w + x] = p[w - 1];
        }
        p += s;
    }

    // p is now the (0,height) (bottom left of image within bigger picture
    p -= (s + m);
    // p is now the (-margin, height-1)
    for (int y = 0; y < m; y++)
    {
        xeve_mcpy(p + (y + 1) * s, p, sizeof(pel) * (w + (m << 1)));
    }

    // pi is still (-marginX, height-1)
    p -= ((h - 1) * s);
    // pi is now (-marginX, 0)
    for (int y = 0; y < m; y++)
    {
        xeve_mcpy(p - (y + 1) * s, p, sizeof(pel) * (w + (m << 1)));
    }
}

/*
 * tmp_yuv -  destination, temporary buffer
 * pointer tmp_yuv is assumed to point to interior point inside margins
 * s - its stride
 * rec - source, recovered buffer
 * s2 - its stride
 * w - width
 * h - height
 * m - margin size
 */
void alf_copy_and_extend( pel* tmp_yuv, const int s, const pel* rec, const int s2, const int w, const int h, const int m )
{

//copy
    for (int j = 0; j < h; j++)
    {
        xeve_mcpy(tmp_yuv + j * s, rec + j * s2, sizeof(pel) * w);
    }

//extend

    pel * p = tmp_yuv;
// do left and right margins
    for (int y = 0; y < h; y++)
    {
        for (int x = 0; x < m; x++)
        {
            *(p - m + x) = p[0];
            p[w + x] = p[w - 1];
        }
        p += s;
    }

// p is now the (0,height) (bottom left of image within bigger picture
    p -= (s + m);
// p is now the (-margin, height-1)
    for (int y = 0; y < m; y++)
    {
        xeve_mcpy(p + (y + 1) * s, p, sizeof(pel) * (w + (m << 1)));
    }

// pi is still (-marginX, height-1)
    p -= ((h - 1) * s);
// pi is now (-marginX, 0)
    for (int y = 0; y < 3; y++)
    {
        xeve_mcpy(p - (y + 1) * s, p, sizeof(pel) * (w + (m << 1)));
    }

} // <-- end of copy and extend

int alf_get_max_golomb_idx( ALF_FILTER_TYPE filter_type )
{
  return filter_type == ALF_FILTER_5 ? 2 : 3;
}

// clang-format off
const int alf_fixed_filter_coef[FIXED_FILTER_NUM][13] =
{
  {   0,   2,   7, -12,  -4, -11,  -2,  31,  -9,   6,  -4,  30, 444 - (1 << (NUM_BITS - 1)) },
  { -26,   4,  17,  22,  -7,  19,  40,  47,  49, -28,  35,  48,  72 - (1 << (NUM_BITS - 1)) },
  { -24,  -8,  30,  64, -13,  18,  18,  27,  80,   0,  31,  19,  28 - (1 << (NUM_BITS - 1)) },
  {  -4, -14,  44, 100,  -7,   6,  -4,   8,  90,  26,  26, -12,  -6 - (1 << (NUM_BITS - 1)) },
  { -17,  -9,  23,  -3, -15,  20,  53,  48,  16, -25,  42,  66, 114 - (1 << (NUM_BITS - 1)) },
  { -12,  -2,   1, -19,  -5,   8,  66,  80,  -2, -25,  20,  78, 136 - (1 << (NUM_BITS - 1)) },
  {   2,   8, -23, -14,  -3, -23,  64,  86,  35, -17,  -4,  79, 132 - (1 << (NUM_BITS - 1)) },
  {  12,   4, -39,  -7,   1, -20,  78,  13,  -8,  11, -42,  98, 310 - (1 << (NUM_BITS - 1)) },
  {   0,   3,  -4,   0,   2,  -7,   6,   0,   0,   3,  -8,  11, 500 - (1 << (NUM_BITS - 1)) },
  {   4,  -7, -25, -19,  -9,   8,  86,  65, -14,  -7,  -7,  97, 168 - (1 << (NUM_BITS - 1)) },
  {   3,   3,   2, -30,   6, -34,  43,  71, -10,   4, -23,  77, 288 - (1 << (NUM_BITS - 1)) },
  {  12,  -3, -34, -14,  -5, -14,  88,  28, -12,   8, -34, 112, 248 - (1 << (NUM_BITS - 1)) },
  {  -1,   6,   8, -29,   7, -27,  15,  60,  -4,   6, -21,  39, 394 - (1 << (NUM_BITS - 1)) },
  {   8,  -1,  -7, -22,   5, -41,  63,  40, -13,   7, -28, 105, 280 - (1 << (NUM_BITS - 1)) },
  {   1,   3,  -5,  -1,   1, -10,  12,  -1,   0,   3,  -9,  19, 486 - (1 << (NUM_BITS - 1)) },
  {  10,  -1, -23, -14,  -3, -27,  78,  24, -14,   8, -28, 102, 288 - (1 << (NUM_BITS - 1)) },
  {   0,   0,  -1,   0,   0,  -1,   1,   0,   0,   0,   0,   1, 512 - (1 << (NUM_BITS - 1)) },
  {   7,   3, -19,  -7,   2, -27,  51,   8,  -6,   7, -24,  64, 394 - (1 << (NUM_BITS - 1)) },
  {  11, -10, -22, -22, -11, -12,  87,  49, -20,   4, -16, 108, 220 - (1 << (NUM_BITS - 1)) },
  {  17,  -2, -69,  -4,  -4,  22, 106,  31,  -7,  13, -63, 121, 190 - (1 << (NUM_BITS - 1)) },
  {   1,   4,  -1,  -7,   5, -26,  24,   0,   1,   3, -18,  51, 438 - (1 << (NUM_BITS - 1)) },
  {   3,   5, -10,  -2,   4, -17,  17,   1,  -2,   6, -16,  27, 480 - (1 << (NUM_BITS - 1)) },
  {   9,   2, -23,  -5,   6, -45,  90, -22,   1,   7, -39, 121, 308 - (1 << (NUM_BITS - 1)) },
  {   4,   5, -15,  -2,   4, -22,  34,  -2,  -2,   7, -22,  48, 438 - (1 << (NUM_BITS - 1)) },
  {   6,   8, -22,  -3,   4, -32,  57,  -3,  -4,  11, -43, 102, 350 - (1 << (NUM_BITS - 1)) },
  {   2,   5, -11,   1,  12, -46,  64, -32,   7,   4, -31,  85, 392 - (1 << (NUM_BITS - 1)) },
  {   5,   5, -12,  -8,   6, -48,  74, -13,  -1,   7, -41, 129, 306 - (1 << (NUM_BITS - 1)) },
  {   0,   1,  -1,   0,   1,  -3,   2,   0,   0,   1,  -3,   4, 508 - (1 << (NUM_BITS - 1)) },
  {  -1,   3,  16, -42,   6, -16,   2, 105,   6,   6, -31,  43, 318 - (1 << (NUM_BITS - 1)) },
  {   7,   8, -27,  -4,  -4, -23,  46,  79,  64,  -8, -13,  68, 126 - (1 << (NUM_BITS - 1)) },
  {  -3,  12,  -4, -34,  14,  -6, -24, 179,  56,   2, -48,  15, 194 - (1 << (NUM_BITS - 1)) },
  {   8,   0, -16, -25,  -1, -29,  68,  84,   3,  -3, -18,  94, 182 - (1 << (NUM_BITS - 1)) },
  {  -3,  -1,  22, -32,   2, -20,   5,  89,   0,   9, -18,  40, 326 - (1 << (NUM_BITS - 1)) },
  {  14,   6, -51,  22, -10, -22,  36,  75, 106,  -4, -11,  56,  78 - (1 << (NUM_BITS - 1)) },
  {   1,  38, -59,  14,   8, -44, -18, 156,  80,  -1, -42,  29, 188 - (1 << (NUM_BITS - 1)) },
  {  -1,   2,   4,  -9,   3, -13,   7,  17,  -4,   2,  -6,  17, 474 - (1 << (NUM_BITS - 1)) },
  {  11,  -2, -15, -36,   2, -32,  67,  89, -19,  -1, -14, 103, 206 - (1 << (NUM_BITS - 1)) },
  {  -1,  10,   3, -28,   7, -27,   7, 117,  34,   1, -35,  51, 234 - (1 << (NUM_BITS - 1)) },
  {   3,   3,   4, -18,   6, -40,  36,  18,  -8,   7, -25,  86, 368 - (1 << (NUM_BITS - 1)) },
  {  -1,   3,   9, -18,   5, -26,  12,  37, -11,   3,  -7,  32, 436 - (1 << (NUM_BITS - 1)) },
  {   0,  17, -38,  -9, -28, -17,  25,  48, 103,   2,  40,  69,  88 - (1 << (NUM_BITS - 1)) },
  {   6,   4, -11, -20,   5, -32,  51,  77,  17,   0, -25,  84, 200 - (1 << (NUM_BITS - 1)) },
  {   0,  -5,  28, -24,  -1, -22,  18,  -9,  17,  -1, -12, 107, 320 - (1 << (NUM_BITS - 1)) },
  { -10,  -4,  17, -30, -29,  31,  40,  49,  44, -26,  67,  67,  80 - (1 << (NUM_BITS - 1)) },
  { -30, -12,  39,  15, -21,  32,  29,  26,  71,  20,  43,  28,  32 - (1 << (NUM_BITS - 1)) },
  {   6,  -7,  -7, -34, -21,  15,  53,  60,  12, -26,  45,  89, 142 - (1 << (NUM_BITS - 1)) },
  {  -1,  -5,  59, -58,  -8, -30,   2,  17,  34,  -7,  25, 111, 234 - (1 << (NUM_BITS - 1)) },
  {   7,   1,  -7, -20,  -9, -22,  48,  27,  -4,  -6,   0, 107, 268 - (1 << (NUM_BITS - 1)) },
  {  -2,  22,  29, -70,  -4, -28,   2,  19,  94, -40,  14, 110, 220 - (1 << (NUM_BITS - 1)) },
  {  13,   0, -22, -27, -11, -15,  66,  44,  -7,  -5, -10, 121, 218 - (1 << (NUM_BITS - 1)) },
  {  10,   6, -22, -14,  -2, -33,  68,  15,  -9,   5, -35, 135, 264 - (1 << (NUM_BITS - 1)) },
  {   2,  11,   4, -32,  -3, -20,  23,  18,  17,  -1, -28,  88, 354 - (1 << (NUM_BITS - 1)) },
  {   0,   3,  -2,  -1,   3, -16,  16,  -3,   0,   2, -12,  35, 462 - (1 << (NUM_BITS - 1)) },
  {   1,   6,  -6,  -3,  10, -51,  70, -31,   5,   6, -42, 125, 332 - (1 << (NUM_BITS - 1)) },
  {   5,  -7,  61, -71, -36,  -6,  -2,  15,  57,  18,  14, 108, 200 - (1 << (NUM_BITS - 1)) },
  {   9,   1,  35, -70, -73,  28,  13,   1,  96,  40,  36,  80, 120 - (1 << (NUM_BITS - 1)) },
  {  11,  -7,  33, -72, -78,  48,  33,  37,  35,   7,  85,  76,  96 - (1 << (NUM_BITS - 1)) },
  {   4,  15,   1, -26, -24, -19,  32,  29,  -8,  -6,  21, 125, 224 - (1 << (NUM_BITS - 1)) },
  {  11,   8,  14, -57, -63,  21,  34,  51,   7,  -3,  69,  89, 150 - (1 << (NUM_BITS - 1)) },
  {   7,  16,  -7, -31, -38,  -5,  41,  44, -11, -10,  45, 109, 192 - (1 << (NUM_BITS - 1)) },
  {   5,  16,  16, -46, -55,   3,  22,  32,  13,   0,  48, 107, 190 - (1 << (NUM_BITS - 1)) },
  {   2,  10,  -3, -14,  -9, -28,  39,  15, -10,  -5,  -1, 123, 274 - (1 << (NUM_BITS - 1)) },
  {   3,  11,  11, -27, -17, -24,  18,  22,   2,   4,   3, 100, 300 - (1 << (NUM_BITS - 1)) },
  {   0,   1,   7,  -9,   3, -20,  16,   3,  -2,   0,  -9,  61, 410 - (1 << (NUM_BITS - 1)) },
};
const int alf_class_to_filter_mapping[MAX_NUM_ALF_CLASSES][ALF_FIXED_FILTER_NUM] =
{
  { 0,   1,   2,   3,   4,   5,   6,   7,   9,  19,  32,  41,  42,  44,  46,  63 },
  { 0,   1,   2,   4,   5,   6,   7,   9,  11,  16,  25,  27,  28,  31,  32,  47 },
  { 5,   7,   9,  11,  12,  14,  15,  16,  17,  18,  19,  21,  22,  27,  31,  35 },
  { 7,   8,   9,  11,  14,  15,  16,  17,  18,  19,  22,  23,  24,  25,  35,  36 },
  { 7,   8,  11,  13,  14,  15,  16,  17,  19,  20,  21,  22,  23,  24,  25,  27 },
  { 1,   2,   3,   4,   6,  19,  29,  30,  33,  34,  37,  41,  42,  44,  47,  54 },
  { 1,   2,   3,   4,   6,  11,  28,  29,  30,  31,  32,  33,  34,  37,  47,  63 },
  { 0,   1,   4,   6,  10,  12,  13,  19,  28,  29,  31,  32,  34,  35,  36,  37 },
  { 6,   9,  10,  12,  13,  16,  19,  20,  28,  31,  35,  36,  37,  38,  39,  52 },
  { 7,   8,  10,  11,  12,  13,  19,  23,  25,  27,  28,  31,  35,  36,  38,  39 },
  { 1,   2,   3,   5,  29,  30,  33,  34,  40,  43,  44,  46,  54,  55,  59,  62 },
  { 1,   2,   3,   4,  29,  30,  31,  33,  34,  37,  40,  41,  43,  44,  59,  61 },
  { 0,   1,   3,   6,  19,  28,  29,  30,  31,  32,  33,  34,  37,  41,  44,  61 },
  { 1,   6,  10,  13,  19,  28,  29,  30,  32,  33,  34,  35,  37,  41,  48,  52 },
  { 0,   5,   6,  10,  19,  27,  28,  29,  32,  37,  38,  40,  41,  47,  49,  58 },
  { 1,   2,   3,   4,  11,  29,  33,  42,  43,  44,  45,  46,  48,  55,  56,  59 },
  { 0,   1,   2,   5,   7,   9,  29,  40,  43,  44,  45,  47,  48,  56,  59,  63 },
  { 0,   4,   5,   9,  14,  19,  26,  35,  36,  43,  45,  47,  48,  49,  50,  51 },
  { 9,  11,  12,  14,  16,  19,  20,  24,  26,  36,  38,  47,  49,  50,  51,  53 },
  { 7,   8,  13,  14,  20,  21,  24,  25,  26,  27,  35,  38,  47,  50,  52,  53 },
  { 1,   2,   4,  29,  33,  40,  41,  42,  43,  44,  45,  46,  54,  55,  56,  58 },
  { 2,   4,  32,  40,  42,  43,  44,  45,  46,  54,  55,  56,  58,  59,  60,  62 },
  { 0,  19,  42,  43,  45,  46,  48,  54,  55,  56,  57,  58,  59,  60,  61,  62 },
  { 8,  13,  36,  42,  45,  46,  51,  53,  54,  57,  58,  59,  60,  61,  62,  63 },
  { 8,  13,  20,  27,  36,  38,  42,  46,  52,  53,  56,  57,  59,  61,  62,  63 },
};
// clang-format on

void alf_recon_coef(ADAPTIVE_LOOP_FILTER * alf, ALF_SLICE_PARAM* alf_slice_param, int channel, const BOOL is_rdo, const BOOL is_re_do)
{
    int factor = is_rdo ? 0 : (1 << (NUM_BITS - 1));
    ALF_FILTER_TYPE filter_type = channel == LUMA_CH ? alf_slice_param->luma_filter_type : ALF_FILTER_5;
    int num_classes = channel == LUMA_CH ? MAX_NUM_ALF_CLASSES : 1;
    int num_coef = filter_type == ALF_FILTER_5 ? 7 : 13;
    int num_coef_minus1 = num_coef - 1;
    int num_filters = channel == LUMA_CH ? alf_slice_param->num_luma_filters : 1;
    short* coeff = channel == LUMA_CH ? alf_slice_param->luma_coef : alf_slice_param->chroma_coef;
    if (channel == LUMA_CH)
    {
        if (alf_slice_param->coef_delta_pred_mode_flag)
        {
            for (int i = 1; i < num_filters; i++)
            {
                for (int j = 0; j < num_coef_minus1; j++)
                {
                    coeff[i * MAX_NUM_ALF_LUMA_COEFF + j] += coeff[(i - 1) * MAX_NUM_ALF_LUMA_COEFF + j];
                }
            }
        }

        xeve_mset(alf->coef_final, 0, sizeof(alf->coef_final));
        int num_coef_large_minus1 = MAX_NUM_ALF_LUMA_COEFF - 1;
        for (int class_idx = 0; class_idx < num_classes; class_idx++)
        {
            int filter_idx = alf_slice_param->filter_coef_delta_idx[class_idx];
            int fixed_filter_idx = alf_slice_param->fixed_filter_idx[class_idx];
            u8  fixed_filter_usage_flag = alf_slice_param->fixed_filter_usage_flag[class_idx];
            int fixed_filter_used = fixed_filter_usage_flag;
            int fixed_filter_map_idx = fixed_filter_idx;
            if (fixed_filter_used)
            {
                fixed_filter_idx = alf_class_to_filter_mapping[class_idx][fixed_filter_map_idx];
            }

            for (int i = 0; i < num_coef_large_minus1; i++)
            {
                int cur_coef = 0;
                //fixed filter
                if (fixed_filter_usage_flag > 0)
                {
                    cur_coef = alf_fixed_filter_coef[fixed_filter_idx][i];
                }
                //add coded coeff
                if (alf->filter_shapes[LUMA_CH][filter_type].pattern_to_large_filter[i] > 0)
                {
                    int coeffIdx = alf->filter_shapes[LUMA_CH][filter_type].pattern_to_large_filter[i] - 1;
                    cur_coef += coeff[filter_idx * MAX_NUM_ALF_LUMA_COEFF + coeffIdx];
                }
                if (is_rdo == 0)
                    xeve_assert(cur_coef >= -(1 << 9) && cur_coef <= (1 << 9) - 1);
                alf->coef_final[class_idx* MAX_NUM_ALF_LUMA_COEFF + i] = cur_coef;
            }

            //last coeff
            int sum = 0;
            for (int i = 0; i < num_coef_large_minus1; i++)
            {
                sum += (alf->coef_final[class_idx* MAX_NUM_ALF_LUMA_COEFF + i] << 1);
            }
            alf->coef_final[class_idx* MAX_NUM_ALF_LUMA_COEFF + num_coef_large_minus1] = factor - sum;
            if (is_rdo == 0)
                xeve_assert(alf->coef_final[class_idx* MAX_NUM_ALF_LUMA_COEFF + num_coef_large_minus1] >= -(1 << 10) && alf->coef_final[class_idx* MAX_NUM_ALF_LUMA_COEFF + num_coef_large_minus1] <= (1 << 10) - 1);
        }

        if (is_re_do && alf_slice_param->coef_delta_pred_mode_flag)
        {
            for (int i = num_filters - 1; i > 0; i--)
            {
                for (int j = 0; j < num_coef_minus1; j++)
                {
                    coeff[i * MAX_NUM_ALF_LUMA_COEFF + j] = coeff[i * MAX_NUM_ALF_LUMA_COEFF + j] - coeff[(i - 1) * MAX_NUM_ALF_LUMA_COEFF + j];
                }
            }
        }
    }
    else
    {
        for (int filter_idx = 0; filter_idx < num_filters; filter_idx++)
        {
            int sum = 0;
            for (int i = 0; i < num_coef_minus1; i++)
            {
                sum += (coeff[filter_idx* MAX_NUM_ALF_LUMA_COEFF + i] << 1);
                if (is_rdo == 0)
                    xeve_assert(coeff[filter_idx* MAX_NUM_ALF_LUMA_COEFF + i] >= -(1 << 9) && coeff[filter_idx* MAX_NUM_ALF_LUMA_COEFF + i] <= (1 << 9) - 1);
            }
            coeff[filter_idx* MAX_NUM_ALF_LUMA_COEFF + num_coef_minus1] = factor - sum;
            if (is_rdo == 0)
                xeve_assert(coeff[filter_idx* MAX_NUM_ALF_LUMA_COEFF + num_coef_minus1] >= -(1 << 10) && coeff[filter_idx* MAX_NUM_ALF_LUMA_COEFF + num_coef_minus1] <= (1 << 10) - 1);
        }
        return;
    }
}

int alf_create(ADAPTIVE_LOOP_FILTER * alf, const int pic_width, const int pic_height, const int max_cu_width, const int max_cu_height, const int max_cu_depth,  const int chroma_format_idc, int bit_depth)
{
    int ret;

    const int input_bit_depth[NUM_CH] = { bit_depth, bit_depth };

    xeve_mset(alf->alf_idx_in_scan_order, 0, sizeof(u8) * APS_MAX_NUM);
    alf->next_free_alf_idx_in_buf = 0;
    alf->first_idx_poc = INT_MAX;
    alf->last_idr_poc = INT_MAX;
    alf->curr_poc = INT_MAX;
    alf->curr_temp_layer = INT_MAX;
    alf->alf_present_idr = 0;
    alf->alf_idx_idr = INT_MAX;
    alf->ac_alf_line_buf_curr_size = 0;
    alf->last_ras_poc = INT_MAX;
    alf->pending_ras_init = FALSE;

    xeve_mcpy(alf->input_bit_depth, input_bit_depth, sizeof(alf->input_bit_depth));
    alf->pic_width = pic_width;
    alf->pic_height = pic_height;
    alf->max_cu_width = max_cu_width;
    alf->max_cu_height = max_cu_height;
    alf->max_cu_depth = max_cu_depth;
    alf->chroma_format = chroma_format_idc;

    alf->num_ctu_in_widht = (alf->pic_width / alf->max_cu_width) + ((alf->pic_width % alf->max_cu_width) ? 1 : 0);
    alf->num_ctu_in_height = (alf->pic_height / alf->max_cu_height) + ((alf->pic_height % alf->max_cu_height) ? 1 : 0);
    alf->num_ctu_in_pic = alf->num_ctu_in_height * alf->num_ctu_in_widht;

    alf_init_filter_shape(&alf->filter_shapes[LUMA_CH][0], 5);
    alf_init_filter_shape(&alf->filter_shapes[LUMA_CH][1], 7);
    alf_init_filter_shape(&alf->filter_shapes[CHROMA_CH][0], 5);

    alf->temp_buf = (pel*)malloc((pic_width + (7 * alf->num_ctu_in_widht))*(pic_height + (7 * alf->num_ctu_in_height)) * sizeof(pel)); // +7 is of filter diameter //todo: check this
    if(alf->chroma_format)
    {
    alf->temp_buf1 = (pel*)malloc(((pic_width >> 1) + (7 * alf->num_ctu_in_widht))*((pic_height >> 1) + (7 * alf->num_ctu_in_height)) * sizeof(pel)); // for chroma just left for unification
    alf->temp_buf2 = (pel*)malloc(((pic_width >> 1) + (7 * alf->num_ctu_in_widht))*((pic_height >> 1) + (7 * alf->num_ctu_in_height)) * sizeof(pel));
    }
    alf->classifier_mt = (ALF_CLASSIFIER**)malloc(MAX_CU_SIZE * XEVE_MAX_THREADS * sizeof(ALF_CLASSIFIER*));
    if (alf->classifier_mt)
    {
        for (int i = 0; i < MAX_CU_SIZE * XEVE_MAX_THREADS; i++)
        {
            alf->classifier_mt[i] = (ALF_CLASSIFIER*)malloc(MAX_CU_SIZE * sizeof(ALF_CLASSIFIER));
            xeve_mset(alf->classifier_mt[i], 0, MAX_CU_SIZE * sizeof(ALF_CLASSIFIER));
        }
    }

    // Classification
    alf->classifier = (ALF_CLASSIFIER**)malloc(pic_height * sizeof(ALF_CLASSIFIER*));
    xeve_assert_gv(alf->classifier, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    for (int i = 0; i < pic_height; i++)
    {
        alf->classifier[i] = (ALF_CLASSIFIER*)malloc(pic_width * sizeof(ALF_CLASSIFIER));
        xeve_assert_gv( alf->classifier[i], ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset(alf->classifier[i], 0, pic_width * sizeof(ALF_CLASSIFIER));
    }
ERR:
    return -1;
}

void alf_destroy(ADAPTIVE_LOOP_FILTER * alf)
{
    free(alf->temp_buf);
    free(alf->temp_buf1);
    free(alf->temp_buf2);

    if (alf->classifier)
    {
        for (int i = 0; i < alf->pic_height; i++)
        {
            free(alf->classifier[i]);
            alf->classifier[i] = NULL;
        }

        free(alf->classifier);
        alf->classifier = NULL;
    }
    if (alf->classifier_mt)
    {
        for (int i = 0; i < MAX_CU_SIZE * XEVE_MAX_THREADS; i++)
        {
            free(alf->classifier_mt[i]);
            alf->classifier_mt[i] = NULL;
        }
        free(alf->classifier_mt);
        alf->classifier_mt = NULL;
    }
}

void alf_derive_classification(ADAPTIVE_LOOP_FILTER * alf, ALF_CLASSIFIER** classifier, const pel * src_luma, const int src_luma_stride, const AREA * blk)
{
    int height = blk->y + blk->height;
    int width = blk->x + blk->width;

    for (int i = blk->y; i < height; i += CLASSIFICATION_BLK_SIZE)
    {
        int h = XEVE_MIN(i + CLASSIFICATION_BLK_SIZE, height) - i;

        for (int j = blk->x; j < width; j += CLASSIFICATION_BLK_SIZE)
        {
            int w = XEVE_MIN(j + CLASSIFICATION_BLK_SIZE, width) - j;
            AREA area = { j, i, w, h };
            alf_derive_classification_blk(classifier, src_luma, src_luma_stride, &area, alf->input_bit_depth[LUMA_CH] + 4, alf->input_bit_depth[LUMA_CH]);
        }
    }
}

void alf_derive_classification_blk(ALF_CLASSIFIER ** classifier, const pel * src_luma, const int src_stride,  const AREA * blk, const int shift, int bit_depth)
{
    static const int th[16] = { 0, 1, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4 };
    const int stride = src_stride;
    const pel * src = src_luma;
    const int max_act = 15;

    int fl = 2;
    int flP1 = fl + 1;
    int fl2 = 2 * fl;

    int main_dir, sec_dir, dir_temp_hv, dir_temp_d;

    int pix_y;
    int height = blk->height + fl2;
    int width = blk->width + fl2;
    int pos_x = blk->x;
    int pos_y = blk->y;
    int start_h = pos_y - flP1;
    int laplacian[NUM_DIRECTIONS][CLASSIFICATION_BLK_SIZE + 5][CLASSIFICATION_BLK_SIZE + 5];

    for (int i = 0; i < height; i += 2)
    {
        int y_offset = (i + 1 + start_h) * stride - flP1;
        const pel * src0 = &src[y_offset - stride];
        const pel * src1 = &src[y_offset];
        const pel * src2 = &src[y_offset + stride];
        const pel * src3 = &src[y_offset + stride * 2];

        int * y_ver = laplacian[VER][i];
        int * y_hor = laplacian[HOR][i];
        int * y_dig0 = laplacian[DIAG0][i];
        int * y_dig1 = laplacian[DIAG1][i];

        for (int j = 0; j < width; j += 2)
        {
            pix_y = j + 1 + pos_x;
            const pel * y = src1 + pix_y;
            const pel * y_down = src0 + pix_y;
            const pel * y_up = src2 + pix_y;
            const pel * y_up2 = src3 + pix_y;

            const pel y0 = y[0] << 1;
            const pel y1 = y[1] << 1;
            const pel y_up0 = y_up[0] << 1;
            const pel y_up1 = y_up[1] << 1;

            y_ver[j] = abs(y0 - y_down[0] - y_up[0]) + abs(y1 - y_down[1] - y_up[1]) + abs(y_up0 - y[0] - y_up2[0]) + abs(y_up1 - y[1] - y_up2[1]);
            y_hor[j] = abs(y0 - y[1] - y[-1]) + abs(y1 - y[2] - y[0]) + abs(y_up0 - y_up[1] - y_up[-1]) + abs(y_up1 - y_up[2] - y_up[0]);
            y_dig0[j] = abs(y0 - y_down[-1] - y_up[1]) + abs(y1 - y_down[0] - y_up[2]) + abs(y_up0 - y[-1] - y_up2[1]) + abs(y_up1 - y[0] - y_up2[2]);
            y_dig1[j] = abs(y0 - y_up[-1] - y_down[1]) + abs(y1 - y_up[0] - y_down[2]) + abs(y_up0 - y_up2[-1] - y[1]) + abs(y_up1 - y_up2[0] - y[2]);

            if (j > 4 && (j - 6) % 4 == 0)
            {
                int jM6 = j - 6;
                int jM4 = j - 4;
                int jM2 = j - 2;

                y_ver[jM6] += y_ver[jM4] + y_ver[jM2] + y_ver[j];
                y_hor[jM6] += y_hor[jM4] + y_hor[jM2] + y_hor[j];
                y_dig0[jM6] += y_dig0[jM4] + y_dig0[jM2] + y_dig0[j];
                y_dig1[jM6] += y_dig1[jM4] + y_dig1[jM2] + y_dig1[j];
            }
        }
    }

    // classification block size
    const int cls_size_y = 4;
    const int cls_size_x = 4;

    for (int i = 0; i < blk->height; i += cls_size_y)
    {
        int * y_ver  = laplacian[VER][i];
        int * y_ver2 = laplacian[VER][i + 2];
        int * y_ver4 = laplacian[VER][i + 4];
        int * y_ver6 = laplacian[VER][i + 6];

        int * y_hor  = laplacian[HOR][i];
        int * y_hor2 = laplacian[HOR][i + 2];
        int * y_hor4 = laplacian[HOR][i + 4];
        int * y_hor6 = laplacian[HOR][i + 6];

        int * y_dig0 = laplacian[DIAG0][i];
        int * y_dig02 = laplacian[DIAG0][i + 2];
        int * y_dig04 = laplacian[DIAG0][i + 4];
        int * y_dig06 = laplacian[DIAG0][i + 6];

        int * y_dig1 = laplacian[DIAG1][i];
        int * y_dig12 = laplacian[DIAG1][i + 2];
        int * y_dig14 = laplacian[DIAG1][i + 4];
        int * y_dig16 = laplacian[DIAG1][i + 6];

        for (int j = 0; j < blk->width; j += cls_size_x)
        {
            int sum_v = y_ver[j] + y_ver2[j] + y_ver4[j] + y_ver6[j];
            int sum_h = y_hor[j] + y_hor2[j] + y_hor4[j] + y_hor6[j];
            int sum_d0 = y_dig0[j] + y_dig02[j] + y_dig04[j] + y_dig06[j];
            int sum_d1 = y_dig1[j] + y_dig12[j] + y_dig14[j] + y_dig16[j];
            int temp_act = sum_v + sum_h;
            int activity = (pel)XEVE_CLIP3(0, max_act, temp_act >> (bit_depth - 2));
            int class_idx = th[activity];
            int hv1, hv0, d1, d0, hvd1, hvd0;

            if (sum_v > sum_h)
            {
                hv1 = sum_v;
                hv0 = sum_h;
                dir_temp_hv = 1;
            }
            else
            {
                hv1 = sum_h;
                hv0 = sum_v;
                dir_temp_hv = 3;
            }
            if (sum_d0 > sum_d1)
            {
                d1 = sum_d0;
                d0 = sum_d1;
                dir_temp_d = 0;
            }
            else
            {
                d1 = sum_d1;
                d0 = sum_d0;
                dir_temp_d = 2;
            }
            if (d1*hv0 > hv1*d0)
            {
                hvd1 = d1;
                hvd0 = d0;
                main_dir = dir_temp_d;
                sec_dir = dir_temp_hv;
            }
            else
            {
                hvd1 = hv1;
                hvd0 = hv0;
                main_dir = dir_temp_hv;
                sec_dir = dir_temp_d;
            }

            int directionStrength = 0;
            if (hvd1 > 2 * hvd0)
            {
                directionStrength = 1;
            }
            if (hvd1 * 2 > 9 * hvd0)
            {
                directionStrength = 2;
            }

            if (directionStrength)
            {
                class_idx += (((main_dir & 0x1) << 1) + directionStrength) * 5;
            }

            static const int trans_tbl[8] = { 0, 1, 0, 2, 2, 3, 1, 3 };
            int trans_idx = trans_tbl[main_dir * 2 + (sec_dir >> 1)];

            int y_offset = i + pos_y;
            int x_offset = j + pos_x;

            ALF_CLASSIFIER *cl0 = classifier[y_offset] + x_offset;
            ALF_CLASSIFIER *cl1 = classifier[y_offset + 1] + x_offset;
            ALF_CLASSIFIER *cl2 = classifier[y_offset + 2] + x_offset;
            ALF_CLASSIFIER *cl3 = classifier[y_offset + 3] + x_offset;
            cl0[0] = cl0[1] = cl0[2] = cl0[3] = cl1[0] = cl1[1] = cl1[2] = cl1[3] = cl2[0] = cl2[1] = cl2[2] = cl2[3] = cl3[0] = cl3[1] = cl3[2] = cl3[3] = ((class_idx << 2) + trans_idx) & 0xFF;
        }
    }
}

void alf_filter_blk_7(ALF_CLASSIFIER** classifier, pel * rec_dst, const int dst_stride, const pel* rec_src, const int src_stride, const AREA* blk, const u8 comp_id, short* filter_set, const CLIP_RANGE* clip_range)
{
    const BOOL is_chroma = FALSE;

    const int start_h = blk->y;
    const int end_h = blk->y + blk->height;
    const int start_w = blk->x;
    const int end_w = blk->x + blk->width;

    const pel* src = rec_src;
    pel* dst = rec_dst;

    const pel *img_y_pad0, *img_y_pad1, *img_y_pad2, *img_y_pad3, *img_y_pad4, *img_y_pad5, *img_y_pad6;
    const pel *img0, *img1, *img2, *img3, *img4, *img5, *img6;

    short *coef = filter_set;

    const int shift = 9;
    const int offset = 1 << (shift - 1);

    int trans_idx = 0;
    const int cls_size_y = 4;
    const int cls_size_x = 4;

    CHECK(start_h % cls_size_y, "Wrong start_h in filtering");
    CHECK(start_w % cls_size_x, "Wrong start_w in filtering");
    CHECK((end_h - start_h) % cls_size_y, "Wrong end_h in filtering");
    CHECK((end_w - start_w) % cls_size_x, "Wrong end_w in filtering");

    ALF_CLASSIFIER *alf_class = NULL;

    int dst_stride2 = dst_stride * cls_size_y;
    int src_stride2 = src_stride * cls_size_y;

    pel filter_coef[MAX_NUM_ALF_LUMA_COEFF];
    img_y_pad0 = src;
    img_y_pad1 = img_y_pad0 + src_stride;
    img_y_pad2 = img_y_pad0 - src_stride;
    img_y_pad3 = img_y_pad1 + src_stride;
    img_y_pad4 = img_y_pad2 - src_stride;
    img_y_pad5 = img_y_pad3 + src_stride;
    img_y_pad6 = img_y_pad4 - src_stride;
    pel * rec0 = dst;
    pel * rec1 = rec0 + dst_stride;

    for (int i = 0; i < end_h - start_h; i += cls_size_y)
    {
        if (!is_chroma)
        {
            alf_class = classifier[start_h + i] + start_w;
        }

        for (int j = 0; j < end_w - start_w; j += cls_size_x)
        {
            ALF_CLASSIFIER cl = alf_class[j];
            trans_idx = cl & 0x03;
            coef = filter_set + ((cl >> 2) & 0x1F) * MAX_NUM_ALF_LUMA_COEFF;

// clang-format off
            const int l[4][MAX_NUM_ALF_LUMA_COEFF] = {
                { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12 },
                { 9, 4, 10, 8, 1, 5, 11, 7, 3, 0, 2, 6, 12 },
                { 0, 3, 2, 1, 8, 7, 6, 5, 4, 9, 10, 11, 12 },
                { 9, 8, 10, 4, 3, 7, 11, 5, 1, 0, 2, 6, 12 }
            };
// clang-format on

            for (int i = 0; i < MAX_NUM_ALF_LUMA_COEFF; i++)
            {
                filter_coef[i] = coef[l[trans_idx][i]];
            }

            for (int ii = 0; ii < cls_size_y; ii++)
            {
                img0 = img_y_pad0 + j + ii * src_stride;
                img1 = img_y_pad1 + j + ii * src_stride;
                img2 = img_y_pad2 + j + ii * src_stride;
                img3 = img_y_pad3 + j + ii * src_stride;
                img4 = img_y_pad4 + j + ii * src_stride;
                img5 = img_y_pad5 + j + ii * src_stride;
                img6 = img_y_pad6 + j + ii * src_stride;

                rec1 = rec0 + j + ii * dst_stride;

                for (int jj = 0; jj < cls_size_x; jj++)
                {
                    int sum = 0;
                    sum += filter_coef[0] * (img5[0] + img6[0]);

                    sum += filter_coef[1] * (img3[+1] + img4[-1]);
                    sum += filter_coef[2] * (img3[+0] + img4[+0]);
                    sum += filter_coef[3] * (img3[-1] + img4[+1]);

                    sum += filter_coef[4] * (img1[+2] + img2[-2]);
                    sum += filter_coef[5] * (img1[+1] + img2[-1]);
                    sum += filter_coef[6] * (img1[+0] + img2[+0]);
                    sum += filter_coef[7] * (img1[-1] + img2[+1]);
                    sum += filter_coef[8] * (img1[-2] + img2[+2]);

                    sum += filter_coef[9] * (img0[+3] + img0[-3]);
                    sum += filter_coef[10] * (img0[+2] + img0[-2]);
                    sum += filter_coef[11] * (img0[+1] + img0[-1]);
                    sum += filter_coef[12] * (img0[+0]);

                    sum = (sum + offset) >> shift;
                    rec1[jj] = clip_pel(sum, *clip_range);

                    img0++;
                    img1++;
                    img2++;
                    img3++;
                    img4++;
                    img5++;
                    img6++;
                }
            }
        }

        rec0 += dst_stride2;
        rec1 += dst_stride2;

        img_y_pad0 += src_stride2;
        img_y_pad1 += src_stride2;
        img_y_pad2 += src_stride2;
        img_y_pad3 += src_stride2;
        img_y_pad4 += src_stride2;
        img_y_pad5 += src_stride2;
        img_y_pad6 += src_stride2;
    }
}

void alf_filter_blk_5(ALF_CLASSIFIER** classifier, pel * rec_dst, const int dst_stride, const pel* rec_src, const int src_stride, const AREA* blk, const u8 comp_id, short* filter_set, const CLIP_RANGE* clip_range)
{
    const int start_h = blk->y;
    const int end_h = blk->y + blk->height;
    const int start_w = blk->x;
    const int end_w = blk->x + blk->width;

    const pel* src = rec_src;
    pel* dst = rec_dst;

    const pel *img_y_pad0, *img_y_pad1, *img_y_pad2, *img_y_pad3, *img_y_pad4;
    const pel *img0, *img1, *img2, *img3, *img4;

    short *coef = filter_set;

    const int shift = 9;
    const int offset = 1 << (shift - 1);

    int trans_idx = 0;
    const int cls_size_y = 1;
    const int cls_size_x = 1;

    ALF_CLASSIFIER *alf_class = NULL;

    int dst_stride2 = dst_stride * cls_size_y;
    int src_stride2 = src_stride * cls_size_y;

    pel filter_coef[MAX_NUM_ALF_LUMA_COEFF];
    img_y_pad0 = src;
    img_y_pad1 = img_y_pad0 + src_stride;
    img_y_pad2 = img_y_pad0 - src_stride;
    img_y_pad3 = img_y_pad1 + src_stride;
    img_y_pad4 = img_y_pad2 - src_stride;
    pel* rec0 = dst;
    pel* rec1 = rec0 + dst_stride;

    for (int i = 0; i < end_h - start_h; i += cls_size_y)
    {
        for (int j = 0; j < end_w - start_w; j += cls_size_x)
        {
            for (int i = 0; i < MAX_NUM_ALF_CHROMA_COEFF; i++)
            {
                filter_coef[i] = coef[i];
            }

            for (int ii = 0; ii < cls_size_y; ii++)
            {
                img0 = img_y_pad0 + j + ii * src_stride;
                img1 = img_y_pad1 + j + ii * src_stride;
                img2 = img_y_pad2 + j + ii * src_stride;
                img3 = img_y_pad3 + j + ii * src_stride;
                img4 = img_y_pad4 + j + ii * src_stride;

                rec1 = rec0 + j + ii * dst_stride;

                for (int jj = 0; jj < cls_size_x; jj++)
                {
                    int sum = 0;

                    sum += filter_coef[0] * (img3[+0] + img4[+0]);

                    sum += filter_coef[1] * (img1[+1] + img2[-1]);
                    sum += filter_coef[2] * (img1[+0] + img2[+0]);
                    sum += filter_coef[3] * (img1[-1] + img2[+1]);

                    sum += filter_coef[4] * (img0[+2] + img0[-2]);
                    sum += filter_coef[5] * (img0[+1] + img0[-1]);
                    sum += filter_coef[6] * (img0[+0]);

                    sum = (sum + offset) >> shift;
                    rec1[jj] = clip_pel(sum, *clip_range);

                    img0++;
                    img1++;
                    img2++;
                    img3++;
                    img4++;
                }
            }
        }

        rec0 += dst_stride2;
        rec1 += dst_stride2;

        img_y_pad0 += src_stride2;
        img_y_pad1 += src_stride2;
        img_y_pad2 += src_stride2;
        img_y_pad3 += src_stride2;
        img_y_pad4 += src_stride2;
    }
}

void alf_param_chroma(ALF_SLICE_PARAM* dst, ALF_SLICE_PARAM* src)
{
    xeve_mcpy(dst->chroma_coef, src->chroma_coef, sizeof(short)*MAX_NUM_ALF_CHROMA_COEFF);
    dst->chroma_filter_present = src->chroma_filter_present;
    dst->chroma_ctb_present_flag = src->chroma_ctb_present_flag;
    dst->enable_flag[1] = src->enable_flag[1];
    dst->enable_flag[2] = src->enable_flag[2];

}

void alf_copy_param(ALF_SLICE_PARAM* dst, ALF_SLICE_PARAM* src)
{
    xeve_mcpy(dst->enable_flag, src->enable_flag, sizeof(BOOL)*N_C);
    dst->chroma_filter_present = src->chroma_filter_present;
    xeve_mcpy(dst->luma_coef, src->luma_coef, sizeof(short)*MAX_NUM_ALF_CLASSES * MAX_NUM_ALF_LUMA_COEFF);
    xeve_mcpy(dst->chroma_coef, src->chroma_coef, sizeof(short)*MAX_NUM_ALF_CHROMA_COEFF);
    xeve_mcpy(dst->filter_coef_delta_idx, src->filter_coef_delta_idx, sizeof(short)*MAX_NUM_ALF_CLASSES);
    xeve_mcpy(dst->filter_coef_flag, src->filter_coef_flag, sizeof(BOOL)*MAX_NUM_ALF_CLASSES);
    xeve_mcpy(dst->fixed_filter_idx, src->fixed_filter_idx, sizeof(int)*MAX_NUM_ALF_CLASSES);
    xeve_mcpy(dst->fixed_filter_usage_flag, src->fixed_filter_usage_flag, sizeof(u8)*MAX_NUM_ALF_CLASSES);

    dst->luma_filter_type = src->luma_filter_type;
    dst->num_luma_filters = src->num_luma_filters;
    dst->coef_delta_flag = src->coef_delta_flag;
    dst->coef_delta_pred_mode_flag = src->coef_delta_pred_mode_flag;
    dst->filterShapes = src->filterShapes;
    dst->chroma_ctb_present_flag = src->chroma_ctb_present_flag;
    dst->fixed_filter_pattern = src->fixed_filter_pattern;
    dst->temporal_alf_flag = src->temporal_alf_flag;
    dst->prev_idx = src->prev_idx;
    dst->prev_idx_comp[0] = src->prev_idx_comp[0];
    dst->prev_idx_comp[1] = src->prev_idx_comp[1];
    dst->t_layer = src->t_layer;

    dst->filter_poc = src->filter_poc;
    dst->min_idr_poc = src->min_idr_poc;
    dst->max_idr_poc = src->max_idr_poc;
}
void alf_reset_param(ALF_SLICE_PARAM* dst)
{
    //Reset destination
    dst->is_ctb_alf_on = FALSE;
    xeve_mset(dst->enable_flag, 0, sizeof(dst->enable_flag)); //false is still 0
    dst->luma_filter_type = ALF_FILTER_5;
    xeve_mset(dst->luma_coef, 0, sizeof(dst->luma_coef));
    xeve_mset(dst->chroma_coef, 0, sizeof(dst->chroma_coef));
    xeve_mset(dst->filter_coef_delta_idx, 0, sizeof(dst->filter_coef_delta_idx));
    for (int i = 0; i < MAX_NUM_ALF_CLASSES; i++)
        dst->filter_coef_flag[i] = TRUE;
    dst->num_luma_filters = 1;
    dst->coef_delta_flag = FALSE;
    dst->coef_delta_pred_mode_flag = FALSE;
    dst->chroma_ctb_present_flag = FALSE;
    dst->fixed_filter_pattern = 0;
    xeve_mset(dst->fixed_filter_idx, 0, sizeof(dst->fixed_filter_idx));
    xeve_mset(dst->fixed_filter_usage_flag, 0, sizeof(dst->fixed_filter_usage_flag));
    dst->temporal_alf_flag = FALSE;
    dst->prev_idx = 0;
    dst->prev_idx_comp[0] = 0;
    dst->prev_idx_comp[1] = 0;
    dst->t_layer = 0;
    dst->reset_alf_buf_flag = FALSE;
    dst->store2_alf_buf_flag = FALSE;

    dst->filter_poc = INT_MAX;  // store POC value for which filter was produced
    dst->min_idr_poc = INT_MAX;  // Minimal of 2 IDR POC available for current coded nalu  (to identify availability of this filter for temp prediction)
    dst->max_idr_poc = INT_MAX;  // Max of 2 IDR POC available for current coded nalu  (to identify availability of this filter for temp prediction)
}

void alf_reset_idr_idx_list_buf_aps(ADAPTIVE_LOOP_FILTER * alf)
{
    if (alf->alf_present_idr)
    {
        alf->alf_idx_in_scan_order[0] = alf->alf_idx_idr;
        alf->ac_alf_line_buf_curr_size = 1;
        alf->next_free_alf_idx_in_buf = (alf->alf_idx_idr + 1) % APS_MAX_NUM;
        alf->alf_present_idr = 0;
    }
    else
    {
        alf->alf_idx_in_scan_order[0] = 0;
        alf->ac_alf_line_buf_curr_size = 0;
        alf->next_free_alf_idx_in_buf = 0;
    }
}

int  alf_get_protect_idx_from_list(ADAPTIVE_LOOP_FILTER * alf, int idx)
{
    u8 i_slice_idx = 0;
    int protect_entry = 0;

    if (alf->i_period == 0)
    {
        return protect_entry;
    }

    // check if current idx is protected (e.g. idr filter idx)
    if (alf->ac_alf_line_buf[idx].filter_poc == alf->ac_alf_line_buf[idx].max_idr_poc)
    {
        protect_entry = 1; // previent overwrite of the protected ALF id (e.g. id of IDR pic)
    }
    if (alf->curr_poc > alf->ac_alf_line_buf[idx].max_idr_poc + alf->i_period)
    {
        protect_entry = 0;
    }

    if ((alf->curr_poc > alf->last_idr_poc) // current POC is after 2nd IDR
        && (alf->ac_alf_line_buf[idx].filter_poc < alf->last_idr_poc)) // POC of checked ALF is before 2nd IDR
    {
        protect_entry = 0;
    }

    if ((alf->curr_poc > alf->ac_alf_line_buf[idx].max_idr_poc) // current POC is after 2nd IDR
        && (alf->ac_alf_line_buf[idx].filter_poc < alf->ac_alf_line_buf[idx].max_idr_poc)) // POC of checked ALF is before 2nd IDR
    {
        protect_entry = 0;
    }

    return protect_entry;
}

void alf_store_enc_alf_param_line_aps(ADAPTIVE_LOOP_FILTER * alf, ALF_SLICE_PARAM* pAlfParam, unsigned t_layer)
{
    alf->ac_alf_line_buf_curr_size++; // There is new filter, increment computed ALF buffer size
    if (alf->ac_alf_line_buf_curr_size > APS_MAX_NUM)
    { // new filter to be stored in occupied location, check if this location is not protected
        while (alf_get_protect_idx_from_list(alf, alf->next_free_alf_idx_in_buf) && alf->next_free_alf_idx_in_buf < APS_MAX_NUM)
        {
            alf->next_free_alf_idx_in_buf = (alf->next_free_alf_idx_in_buf + 1) % APS_MAX_NUM;  // Compute next availble ALF circular buffer index
        }
    }
    u8 idx = alf->next_free_alf_idx_in_buf;  // Take in use next availble ALF circular buffer index
    pAlfParam->filter_poc = alf->curr_poc;
    pAlfParam->min_idr_poc = alf->first_idx_poc;
    pAlfParam->max_idr_poc = alf->last_idr_poc;
    pAlfParam->temporal_alf_flag = FALSE;
    pAlfParam->t_layer = t_layer;
    pAlfParam->chroma_ctb_present_flag = FALSE;

    if (alf->ac_alf_line_buf_curr_size > APS_MAX_NUM)
    {
        // New ALF beyond ALF buffer capacity, index list is shifted left, by removing the most old  index (preserving protected indexes) from alf_idx_in_scan_order
        for (int i = 1; i < APS_MAX_NUM; i++)
        {
            int idx_to_check = i - 1;
            if (alf_get_protect_idx_from_list(alf, alf->alf_idx_in_scan_order[idx_to_check]))
            {
                continue;
            }
            alf->alf_idx_in_scan_order[idx_to_check] = alf->alf_idx_in_scan_order[i];
        }
    }

    alf_reset_param(&(alf->ac_alf_line_buf[idx]));
    alf_copy_param(&(alf->ac_alf_line_buf[idx]), pAlfParam);

    alf->ac_alf_line_buf_curr_size = alf->ac_alf_line_buf_curr_size > APS_MAX_NUM ? APS_MAX_NUM : alf->ac_alf_line_buf_curr_size;  // Increment size of the circular buffer  (there are 2 buffers - ALF and indexes)
    alf->alf_idx_in_scan_order[alf->ac_alf_line_buf_curr_size - 1] = alf->next_free_alf_idx_in_buf;                                       // store new alf idx in the indexes circular buffer
    alf->next_free_alf_idx_in_buf = (alf->next_free_alf_idx_in_buf + 1) % APS_MAX_NUM;  // Compute next availble ALF circular buffer index
}

void alf_store_paramline_from_aps(ADAPTIVE_LOOP_FILTER * alf, ALF_SLICE_PARAM* pAlfParam, u8 idx)
{
    assert(idx < APS_MAX_NUM);
    alf_copy_param(&(alf->ac_alf_line_buf[idx]), pAlfParam);
    alf->ac_alf_line_buf_curr_size++;
    alf->ac_alf_line_buf_curr_size = alf->ac_alf_line_buf_curr_size > APS_MAX_NUM ? APS_MAX_NUM : alf->ac_alf_line_buf_curr_size;  // Increment used ALF circular buffer size
}

void alf_load_paramline_from_aps_buffer2(ADAPTIVE_LOOP_FILTER * alf, ALF_SLICE_PARAM* pAlfParam, u8 idxY, u8 idxUV, u8 alf_chroma_idc)
{
    alf_copy_param(pAlfParam, &(alf->ac_alf_line_buf[idxY]));
    assert(pAlfParam->enable_flag[0] == 1);
    if (alf_chroma_idc)
    {
        alf_param_chroma(pAlfParam, &(alf->ac_alf_line_buf[idxUV]));
        assert(pAlfParam->chroma_filter_present == 1);
        pAlfParam->enable_flag[1] = alf_chroma_idc & 1;
        pAlfParam->enable_flag[2] = (alf_chroma_idc >> 1) & 1;
    }
    else
    {
        pAlfParam->enable_flag[1] = 0;
        pAlfParam->enable_flag[2] = 0;
    }
}

void alf_load_paramline_from_aps_buffer(ADAPTIVE_LOOP_FILTER * alf, ALF_SLICE_PARAM* pAlfParam, u8 idx)
{
    alf_copy_param(pAlfParam, &(alf->ac_alf_line_buf[idx]) );
}


int alf_cov_create(ALF_COVARIANCE* alf_cov, int size)
{
    int ret;
    alf_cov->num_coef = size;

    alf_cov->y = (double*)xeve_malloc(sizeof(double) * alf_cov->num_coef);
    xeve_assert_gv(alf_cov->y, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    alf_cov->E = (double**)xeve_malloc(sizeof(double*) * alf_cov->num_coef);
    xeve_assert_gv(alf_cov->E, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

    xeve_mset(alf_cov->y, 0, sizeof(double) * alf_cov->num_coef);
    xeve_mset(alf_cov->E, 0, sizeof(double*) * alf_cov->num_coef);

    for (int i = 0; i < alf_cov->num_coef; i++)
    {
        alf_cov->E[i] = (double*)xeve_malloc(sizeof(double) * alf_cov->num_coef);
        xeve_assert_gv(alf_cov->E[i], ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset(alf_cov->E[i], 0, sizeof(double) * alf_cov->num_coef);
    }
ERR:
    return -1;
}

void alf_cov_destroy(ALF_COVARIANCE* alf_cov)
{
    for (int i = 0; i < alf_cov->num_coef; i++)
    {
        xeve_mfree(alf_cov->E[i]);
        alf_cov->E[i] = NULL;
    }

    xeve_mfree(alf_cov->E);
    alf_cov->E = NULL;

    xeve_mfree(alf_cov->y);
    alf_cov->y = NULL;
}

void alf_cov_reset(ALF_COVARIANCE* alf_cov)
{
    alf_cov->pix_acc = 0;
    xeve_mset(alf_cov->y, 0, sizeof(double) * alf_cov->num_coef);
    for (int i = 0; i < alf_cov->num_coef; i++)
    {
        xeve_mset(alf_cov->E[i], 0, sizeof(double) * alf_cov->num_coef);
    }
}

void alf_cov_copy(ALF_COVARIANCE* dst, ALF_COVARIANCE* src)
{
    dst->num_coef = src->num_coef;
    dst->pix_acc = src->pix_acc;

    for (int i = 0; i < src->num_coef; i++)
    {
        xeve_mcpy(dst->E[i], src->E[i], sizeof(src->E[i]) * src->num_coef);
    }
    xeve_mcpy(dst->y, src->y, sizeof(src->y) * src->num_coef);
}

void alf_cov_add_to(ALF_COVARIANCE* dst, const ALF_COVARIANCE* lhs, const ALF_COVARIANCE* rhs)
{
    for (int j = 0; j < dst->num_coef; j++)
    {
        for (int i = 0; i < dst->num_coef; i++)
        {
            dst->E[j][i] = lhs->E[j][i] + rhs->E[j][i];
        }
        dst->y[j] = lhs->y[j] + rhs->y[j];
    }
    dst->pix_acc = lhs->pix_acc + rhs->pix_acc;
}

void alf_cov_add(ALF_COVARIANCE* dst, const ALF_COVARIANCE* src)
{
    for (int j = 0; j < src->num_coef; j++)
    {
        for (int i = 0; i < src->num_coef; i++)
        {
            dst->E[j][i] += src->E[j][i];
        }
        dst->y[j] += src->y[j];
    }
    dst->pix_acc += src->pix_acc;
}

void alf_cov_minus(ALF_COVARIANCE * dst, const ALF_COVARIANCE * src)
{
    for (int j = 0; j < src->num_coef; j++)
    {
        for (int i = 0; i < src->num_coef; i++)
        {
            dst->E[j][i] -= src->E[j][i];
        }
        dst->y[j] -= src->y[j];
    }
    dst->pix_acc -= src->pix_acc;
}


void xeve_alf_set_reset_alf_buf_flag(XEVE_ALF * enc_alf, int flag)
{
    enc_alf->alf.reset_alf_buf_flag = flag;
}

void set_store2ALFBufferFlag(XEVE_ALF * enc_alf, int flag)
{
    enc_alf->alf.strore2_alf_buf_flag = flag;
}

void xeve_alf_delete_buf(XEVE_ALF * enc_alf)
{
    xeve_mfree(enc_alf);
}

XEVE_ALF * xeve_alf_create_buf(int bit_depth)
{
    XEVE_ALF * enc_alf = (XEVE_ALF *)xeve_malloc(sizeof(XEVE_ALF));
    xeve_mset(enc_alf, 0, sizeof(XEVE_ALF));
    alf_init(&(enc_alf->alf), bit_depth);
    return enc_alf;
}

int xeve_alf_aps_enc_opt_process(XEVE_ALF * enc_alf, const double* lambdas, XEVE_CTX * ctx, XEVE_PIC * pic, XEVE_ALF_SLICE_PARAM * input_alf_slice_param)
{
    CODING_STRUCTURE cs;
    cs.ctx = (void*)ctx;
    cs.pic = pic;

    if (enc_alf->alf.reset_alf_buf_flag)
    {
        input_alf_slice_param->reset_alf_buf_flag = TRUE;
    }
    // Initialize ALF module for current POC
    enc_alf->alf.curr_poc = ctx->poc.poc_val;
    enc_alf->alf.curr_temp_layer = ctx->nalu.nuh_temporal_id;
    if (enc_alf->alf.reset_alf_buf_flag)
    {
        // initialize firstIdrPoc
        if (enc_alf->alf.last_idr_poc != INT_MAX)  // LastIdr value was initialized
        {
            enc_alf->alf.first_idx_poc = enc_alf->alf.last_idr_poc;
        }
        else {
            enc_alf->alf.first_idx_poc = ctx->poc.poc_val;
        }
        enc_alf->alf.last_idr_poc = ctx->poc.poc_val;  // store current pointer of the reset poc
        enc_alf->alf.i_period = ctx->param.keyint; // store i-period for current pic.
    }

    enc_alf->alf.pending_ras_init = FALSE;
    if (ctx->poc.poc_val > enc_alf->alf.last_ras_poc)
    {
        enc_alf->alf.last_ras_poc = INT_MAX;
        enc_alf->alf.pending_ras_init = TRUE;
    }
    if (ctx->sh->slice_type == SLICE_I)
    {
        enc_alf->alf.last_ras_poc = ctx->poc.poc_val;
    }

    if (enc_alf->alf.pending_ras_init)
    {
        alf_reset_idr_idx_list_buf_aps(&enc_alf->alf);
    }

    ALF_SLICE_PARAM alf_slice_param;
    s32 size = sizeof(u8) * ctx->f_scu * N_C;
    alf_slice_param.alf_ctb_flag = (u8 *)malloc(size);
    if (alf_slice_param.alf_ctb_flag == NULL)
        return XEVE_ERR;
    xeve_mset(alf_slice_param.alf_ctb_flag, 0, N_C * ctx->f_lcu * sizeof(u8));
    xeve_mset(input_alf_slice_param->alf_ctb_flag, 0, N_C * ctx->f_lcu * sizeof(u8));
    xeve_alf_process(enc_alf, &cs, lambdas, &alf_slice_param);

    if (alf_slice_param.enable_flag[0] && enc_alf->alf.strore2_alf_buf_flag)
    {
        const unsigned tidxMAX = MAX_NUM_TLAYER - 1u;
        const unsigned tidx = ctx->nalu.nuh_temporal_id;
        assert(tidx <= tidxMAX);
        alf_store_enc_alf_param_line_aps(&enc_alf->alf, &alf_slice_param, tidx);
        alf_slice_param.store2_alf_buf_flag = enc_alf->alf.strore2_alf_buf_flag;
    }
    if (ctx->sh->slice_type == SLICE_I)
    {
        if (alf_slice_param.enable_flag[0] && enc_alf->alf.strore2_alf_buf_flag)
        {
            enc_alf->alf.alf_present_idr = 1;
            enc_alf->alf.alf_idx_idr = xeve_alf_aps_get_current_alf_idx(enc_alf);
        }
        else
        {
            enc_alf->alf.alf_present_idr = 0;
            enc_alf->alf.alf_idx_idr = 0;
        }
    }

    input_alf_slice_param->is_ctb_alf_on = (BOOL)alf_slice_param.is_ctb_alf_on ? 1 : 0;
    xeve_mcpy(input_alf_slice_param->alf_ctb_flag, alf_slice_param.alf_ctb_flag, N_C * ctx->f_lcu * sizeof(u8));
    input_alf_slice_param->enable_flag[0] = (BOOL)alf_slice_param.enable_flag[Y_C];
    input_alf_slice_param->enable_flag[1] = (BOOL)alf_slice_param.enable_flag[U_C];
    input_alf_slice_param->enable_flag[2] = (BOOL)alf_slice_param.enable_flag[V_C];

    input_alf_slice_param->num_luma_filters = alf_slice_param.num_luma_filters;
    input_alf_slice_param->luma_filter_type = (int)alf_slice_param.luma_filter_type;

    xeve_mcpy(input_alf_slice_param->filter_coef_delta_idx, alf_slice_param.filter_coef_delta_idx, MAX_NUM_ALF_CLASSES * sizeof(short));
    xeve_mcpy(input_alf_slice_param->luma_coef, alf_slice_param.luma_coef, sizeof(short)*MAX_NUM_ALF_CLASSES*MAX_NUM_ALF_LUMA_COEFF);
    xeve_mcpy(input_alf_slice_param->chroma_coef, alf_slice_param.chroma_coef, sizeof(short)*MAX_NUM_ALF_CHROMA_COEFF);
    xeve_mcpy(input_alf_slice_param->fixed_filter_idx, alf_slice_param.fixed_filter_idx, MAX_NUM_ALF_CLASSES * sizeof(int));
    xeve_mcpy(input_alf_slice_param->fixed_filter_usage_flag, alf_slice_param.fixed_filter_usage_flag, MAX_NUM_ALF_CLASSES * sizeof(u8));
    input_alf_slice_param->fixed_filter_pattern = alf_slice_param.fixed_filter_pattern;
    input_alf_slice_param->coef_delta_flag = (BOOL)alf_slice_param.coef_delta_flag;
    input_alf_slice_param->coef_delta_pred_mode_flag = (BOOL)alf_slice_param.coef_delta_pred_mode_flag;

    //BOOL is not a BOOL
    for (int i = 0; i < MAX_NUM_ALF_CLASSES; i++)
    {
        input_alf_slice_param->filter_coef_flag[i] = (BOOL)alf_slice_param.filter_coef_flag[i];
    }

    input_alf_slice_param->prev_idx = alf_slice_param.prev_idx;
    input_alf_slice_param->prev_idx_comp[0] = alf_slice_param.prev_idx_comp[0];
    input_alf_slice_param->prev_idx_comp[1] = alf_slice_param.prev_idx_comp[1];
    input_alf_slice_param->t_layer = alf_slice_param.t_layer;
    input_alf_slice_param->temporal_alf_flag = (BOOL)alf_slice_param.temporal_alf_flag;
    input_alf_slice_param->reset_alf_buf_flag = (BOOL)alf_slice_param.reset_alf_buf_flag;
    input_alf_slice_param->store2_alf_buf_flag = (BOOL)alf_slice_param.store2_alf_buf_flag;
    xeve_mfree(alf_slice_param.alf_ctb_flag);
	
	return XEVE_OK;
}

u8 xeve_alf_aps_get_current_alf_idx(XEVE_ALF * enc_alf)
{
    return (enc_alf->alf.next_free_alf_idx_in_buf - 1) < 0 ? APS_MAX_NUM - 1 : (enc_alf->alf.next_free_alf_idx_in_buf - 1);
}

void AlfSliceParam_reset(ADAPTIVE_LOOP_FILTER * alf, ALF_SLICE_PARAM* alf_param)
{
    alf_param->is_ctb_alf_on = FALSE;
    xeve_mset(alf_param->alf_ctb_flag, 1, alf->num_ctu_in_pic * sizeof(u8));
    xeve_mset(alf_param->enable_flag, 0, sizeof(alf_param->enable_flag)); //FALSE is still 0
    alf_param->luma_filter_type = ALF_FILTER_5;
    xeve_mset(alf_param->luma_coef, 0, sizeof(alf_param->luma_coef));
    xeve_mset(alf_param->chroma_coef, 0, sizeof(alf_param->chroma_coef));
    xeve_mset(alf_param->filter_coef_delta_idx, 0, sizeof(alf_param->filter_coef_delta_idx));
    for (int i = 0; i < MAX_NUM_ALF_CLASSES; i++)
    {
        alf_param->filter_coef_flag[i] = TRUE;
    }
    alf_param->num_luma_filters = 1;
    alf_param->coef_delta_flag = FALSE;
    alf_param->coef_delta_pred_mode_flag = FALSE;
    alf_param->chroma_ctb_present_flag = FALSE;
    alf_param->fixed_filter_pattern = 0;
    xeve_mset(alf_param->fixed_filter_idx, 0, sizeof(alf_param->fixed_filter_idx));
    xeve_mset(alf_param->fixed_filter_usage_flag, 0, sizeof(alf_param->fixed_filter_usage_flag));
    alf_param->temporal_alf_flag = FALSE;
    alf_param->prev_idx = 0;
    alf_param->prev_idx_comp[0] = 0;
    alf_param->prev_idx_comp[1] = 0;
    alf_param->t_layer = 0;
    alf_param->reset_alf_buf_flag = FALSE;
    alf_param->store2_alf_buf_flag = FALSE;
    alf_param->filter_poc = INT_MAX;  // store POC value for which filter was produced
    alf_param->min_idr_poc = INT_MAX;  // Minimal of 2 IDR POC available for current coded nalu  (to identify availability of this filter for temp prediction)
    alf_param->max_idr_poc = INT_MAX;  // Max of 2 IDR POC available for current coded nalu  (to identify availability of this filter for temp prediction)
}

int xeve_alf_create(XEVE_ALF * enc_alf, const int pic_widht, const int pic_height, const int max_cu_width, const int max_cu_height, const int max_cu_depth, const int chroma_format_idc, int bit_depth)
{
    int ret;
    ADAPTIVE_LOOP_FILTER * alf = &enc_alf->alf;

    enc_alf->frac_bits_scale = 1.0 / (double)(1 << SCALE_BITS);

    for (int i = 0; i < N_C; i++)
    {
        enc_alf->alf_cov[i] = NULL;
    }
    for (int i = 0; i < N_C; i++)
    {
        enc_alf->alf_cov_frame[i] = NULL;
    }

    enc_alf->filter_coef_quant = NULL;
    enc_alf->filter_coef_set = NULL;
    enc_alf->dif_filter_coef = NULL;

    alf_create(alf, pic_widht, pic_height, max_cu_width, max_cu_height, max_cu_depth, chroma_format_idc, bit_depth);
    for (u8 comp_id = 0; comp_id < N_C; comp_id++)
    {
        u8 ch_type = comp_id == Y_C ? LUMA_CH : CHROMA_CH;

        const int size = comp_id == Y_C ? 2 : 1;
        int num_classes = comp_id ? 1 : MAX_NUM_ALF_CLASSES;
        enc_alf->alf_cov_frame[comp_id] = (ALF_COVARIANCE**)xeve_malloc(sizeof(ALF_COVARIANCE*) * size);
        for (int i = 0; i != size; i++)
        {
            enc_alf->alf_cov_frame[comp_id][i] = (ALF_COVARIANCE*)xeve_malloc(sizeof(ALF_COVARIANCE) * num_classes);
            for (int k = 0; k < num_classes; k++)
            {
                alf_cov_create(&enc_alf->alf_cov_frame[comp_id][i][k], alf->filter_shapes[ch_type][i].num_coef);
            }
        }
    }
    enc_alf->alf_cov_frame[N_C] = (ALF_COVARIANCE**)xeve_malloc(sizeof(ALF_COVARIANCE*));
    enc_alf->alf_cov_frame[N_C][0] = (ALF_COVARIANCE*)xeve_malloc(sizeof(ALF_COVARIANCE));
    for (int k = 0; k < 1; k++)
    {
        alf_cov_create(&enc_alf->alf_cov_frame[N_C][0][k], alf->filter_shapes[1][0].num_coef);
    }

    enc_alf->alf_slice_param_temp.alf_ctb_flag = (u8 *)malloc(N_C * alf->num_ctu_in_pic * sizeof(u8));
    xeve_assert_gv(enc_alf->alf_slice_param_temp.alf_ctb_flag, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    xeve_mset(enc_alf->alf_slice_param_temp.alf_ctb_flag, 0, N_C * alf->num_ctu_in_pic * sizeof(u8));

    enc_alf->ctu_enable_flag_temp_luma = (u8 *)malloc(N_C * alf->num_ctu_in_pic * sizeof(u8));
    xeve_mset(enc_alf->ctu_enable_flag_temp_luma, 0, N_C * alf->num_ctu_in_pic * sizeof(u8));

    for (int comp_id = 0; comp_id < N_C; comp_id++)
    {
        enc_alf->ctu_enable_flag_temp[comp_id] = (u8*)xeve_malloc(sizeof(u8)*alf->num_ctu_in_pic);
        xeve_mset(enc_alf->ctu_enable_flag_temp[comp_id], 0, sizeof(u8)*alf->num_ctu_in_pic);

        u8 ch_type = (comp_id == Y_C) ? LUMA_CH : CHROMA_CH;
        int num_classes = (comp_id == Y_C) ? MAX_NUM_ALF_CLASSES : 1;
        const int size = (ch_type == LUMA_CH) ? 2 : 1;

        enc_alf->alf_cov[comp_id] = (ALF_COVARIANCE***)xeve_malloc(sizeof(ALF_COVARIANCE**) * size);
        for (int i = 0; i != size; i++)
        {
            enc_alf->alf_cov[comp_id][i] = (ALF_COVARIANCE**)xeve_malloc(sizeof(ALF_COVARIANCE*) * alf->num_ctu_in_pic);
            for (int j = 0; j < alf->num_ctu_in_pic; j++)
            {
                enc_alf->alf_cov[comp_id][i][j] = (ALF_COVARIANCE*)xeve_malloc(sizeof(ALF_COVARIANCE) * num_classes);
                xeve_assert_gv(enc_alf->alf_cov[comp_id][i][j], ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
                for (int k = 0; k < num_classes; k++)
                {
                    alf_cov_create(&enc_alf->alf_cov[comp_id][i][j][k], alf->filter_shapes[ch_type][i].num_coef);
                }
            }
        }
    }

    for (int i = 0; i != 2; i++)
    {
        for (int j = 0; j <= MAX_NUM_ALF_CLASSES; j++)
        {
            alf_cov_create(&enc_alf->alf_cov_merged[i][j], alf->filter_shapes[Y_C][i].num_coef);
        }
    }

    enc_alf->filter_coef_quant = (int*)xeve_malloc(sizeof(int) * MAX_NUM_ALF_LUMA_COEFF);
    xeve_mset(enc_alf->filter_coef_quant, 0, sizeof(int) * MAX_NUM_ALF_LUMA_COEFF);

    enc_alf->filter_coef_set  = (int**)xeve_malloc(sizeof(int*) * MAX_NUM_ALF_CLASSES);
    enc_alf->dif_filter_coef = (int**)xeve_malloc(sizeof(int*) * MAX_NUM_ALF_CLASSES);

    for (int i = 0; i < MAX_NUM_ALF_CLASSES; i++)
    {
        enc_alf->filter_coef_set[i]  = (int*)xeve_malloc(sizeof(int) * MAX_NUM_ALF_LUMA_COEFF);
        enc_alf->dif_filter_coef[i] = (int*)xeve_malloc(sizeof(int) * MAX_NUM_ALF_LUMA_COEFF);
        xeve_mset(enc_alf->filter_coef_set[i], 0, sizeof(int) * MAX_NUM_ALF_LUMA_COEFF);
        xeve_mset(enc_alf->dif_filter_coef[i], 0, sizeof(int) * MAX_NUM_ALF_LUMA_COEFF);
    }
    return XEVE_OK;
ERR:
    return XEVE_ERR;
}

void xeve_alf_destroy(XEVE_ALF * enc_alf)
{
    ADAPTIVE_LOOP_FILTER * alf = &enc_alf->alf;

    for (int comp_id = 0; comp_id < N_C; comp_id++)
    {
        if (enc_alf->alf_cov_frame[comp_id])
        {
            int num_classes = comp_id == Y_C ? MAX_NUM_ALF_CLASSES : 1;
            const int size = comp_id == Y_C ? 2 : 1;
            for (int i = 0; i != size; i++)
            {
                for (int k = 0; k < num_classes; k++)
                {
                    alf_cov_destroy(&enc_alf->alf_cov_frame[comp_id][i][k]);
                }
                xeve_mfree(enc_alf->alf_cov_frame[comp_id][i]);
                enc_alf->alf_cov_frame[comp_id][i] = NULL;
            }
            xeve_mfree(enc_alf->alf_cov_frame[comp_id]);
            enc_alf->alf_cov_frame[comp_id] = NULL;
        }
    }
    alf_cov_destroy(&enc_alf->alf_cov_frame[N_C][0][0]);
    xeve_mfree(enc_alf->alf_cov_frame[N_C][0]);
    enc_alf->alf_cov_frame[N_C][0] = NULL;
    xeve_mfree(enc_alf->alf_cov_frame[N_C]);
    enc_alf->alf_cov_frame[N_C] = NULL;
    xeve_mfree(enc_alf->alf_slice_param_temp.alf_ctb_flag);
    xeve_mfree(enc_alf->ctu_enable_flag_temp_luma);

    enc_alf->ctu_enable_flag_temp_luma = NULL;

    for (int comp_id = 0; comp_id < N_C; comp_id++)
    {
        if (enc_alf->ctu_enable_flag_temp[comp_id])
        {
            xeve_mfree(enc_alf->ctu_enable_flag_temp[comp_id]);
            enc_alf->ctu_enable_flag_temp[comp_id] = NULL;
        }
        if (enc_alf->alf_cov[comp_id])
        {
            const int size = comp_id == Y_C ? 2 : 1;
            int num_classes = comp_id == Y_C ? MAX_NUM_ALF_CLASSES : 1;

            for (int i = 0; i != size; i++)
            {
                for (int j = 0; j < alf->num_ctu_in_pic; j++)
                {
                    for (int k = 0; k < num_classes; k++)
                    {
                        alf_cov_destroy(&enc_alf->alf_cov[comp_id][i][j][k]);
                    }
                    xeve_mfree(enc_alf->alf_cov[comp_id][i][j]);
                    enc_alf->alf_cov[comp_id][i][j] = NULL;

                }
                xeve_mfree( enc_alf->alf_cov[comp_id][i]);
                enc_alf->alf_cov[comp_id][i] = NULL;

            }
            xeve_mfree(enc_alf->alf_cov[comp_id]);
            enc_alf->alf_cov[comp_id] = NULL;
        }
    }

    for (int i = 0; i != 2 /* filter_shapes[Y_C].size() */; i++)
    {
        for (int j = 0; j <= MAX_NUM_ALF_CLASSES; j++)
        {
            alf_cov_destroy(&enc_alf->alf_cov_merged[i][j]);
        }
    }

    if (enc_alf->filter_coef_set)
    {
        for (int i = 0; i < MAX_NUM_ALF_CLASSES; i++)
        {
            xeve_mfree(enc_alf->filter_coef_set[i]);
            enc_alf->filter_coef_set[i] = NULL;
        }
        xeve_mfree(enc_alf->filter_coef_set);
        enc_alf->filter_coef_set = NULL;
    }

    if (enc_alf->dif_filter_coef)
    {
        for (int i = 0; i < MAX_NUM_ALF_CLASSES; i++)
        {
            xeve_mfree(enc_alf->dif_filter_coef[i]);
            enc_alf->dif_filter_coef[i] = NULL;
        }
        xeve_mfree(enc_alf->dif_filter_coef);
        enc_alf->dif_filter_coef = NULL;
    }

    xeve_mfree( enc_alf->filter_coef_quant);
    enc_alf->filter_coef_quant = NULL;

    alf_destroy(alf);
}


void xeve_alf_process(XEVE_ALF * enc_alf, CODING_STRUCTURE * cs, const double *lambdas, ALF_SLICE_PARAM* alf_slice_param)
{
    XEVE_CTX* ctx = (XEVE_CTX*)(cs->ctx);
    ADAPTIVE_LOOP_FILTER * alf = &enc_alf->alf;
    for (int comp_id = 0; comp_id < N_C; comp_id++)
    {
        alf->ctu_enable_flag[comp_id] = alf_slice_param->alf_ctb_flag + ctx->f_lcu * comp_id;
    }

    // reset ALF parameters
    AlfSliceParam_reset(alf, alf_slice_param);

    // set available filter shapes
    alf_slice_param->filterShapes = alf->filter_shapes;

    int shift_luma = 2 * DISTORTION_PRECISION_ADJUSTMENT(input_bit_depth[LUMA_CH] - 8);
    int shift_chroma = 2 * DISTORTION_PRECISION_ADJUSTMENT(input_bit_depth[CHROMA_CH] - 8);
    enc_alf->lambda[Y_C] = lambdas[Y_C] * (double)(1 << shift_luma);
    enc_alf->lambda[U_C] = lambdas[U_C] * (double)(1 << shift_chroma);
    enc_alf->lambda[V_C] = lambdas[V_C] * (double)(1 << shift_chroma);

    const int h = cs->pic->h_l;
    const int w = cs->pic->w_l;
    const int m = MAX_ALF_FILTER_LENGTH >> 1;
    const int s = w + m + m;

    XEVE_PIC* pic_org = PIC_ORIG(ctx);
    XEVE_PIC* pir_rec = PIC_MODE(ctx);

    pel * org_y = pic_org->y;
    pel * rec_y = pir_rec->y;

    int org_stride = pic_org->s_l;
    int rec_stride = pir_rec->s_l;
    pel * rec_tmp_y = alf->temp_buf + s * m + m;

    //chroma (for 4:2:0 only)
    const int s1 = (w >> 1) + m + m;
    pel * rec_tmp_u = alf->temp_buf1 + s1 * m + m;
    pel * ref_tmp_v = alf->temp_buf2 + s1 * m + m;
    pel * rec_u = pir_rec->u;
    pel * rec_v = pir_rec->v;
    const int rec_stride_c = pir_rec->s_c;
    pel * org_u = pic_org->u;
    pel * org_v = pic_org->v;
    const int org_stride_c = pic_org->s_c;

    YUV org_yuv, rec_temp, rec_yuv;
    org_yuv.yuv[0] = org_y;  org_yuv.s[0] = org_stride;
    org_yuv.yuv[1] = org_u;  org_yuv.s[1] = org_stride_c;
    org_yuv.yuv[2] = org_v;  org_yuv.s[2] = org_stride_c;
    rec_yuv.yuv[0] = rec_y;  rec_yuv.s[0] = rec_stride;
    rec_yuv.yuv[1] = rec_u;  rec_yuv.s[1] = pir_rec->s_c;
    rec_yuv.yuv[2] = rec_v;  rec_yuv.s[2] = pir_rec->s_c;
    rec_temp.yuv[0] = rec_tmp_y; rec_temp.s[0] = s;
    rec_temp.yuv[1] = rec_tmp_u; rec_temp.s[1] = s1;
    rec_temp.yuv[2] = ref_tmp_v; rec_temp.s[2] = s1;

    int  x_l, x_r, y_l, y_r, w_tile, h_tile;
    int col_bd = 0;

    for (int slice_num = 0; slice_num < ctx->param.num_slice_in_pic; slice_num++)
    {
        ctx->sh = &ctx->sh_array[slice_num];

        u32 k = 0;
        int tile_idx = 0;
        int total_tiles_in_slice = ctx->sh->num_tiles_in_slice;
        while (total_tiles_in_slice)
        {
            tile_idx = ctx->sh->tile_order[k++];
            int x_loc = ((ctx->tile[tile_idx].ctba_rs_first) % ctx->w_lcu);
            int y_loc = ((ctx->tile[tile_idx].ctba_rs_first) / ctx->w_lcu);
            x_l = x_loc << ctx->log2_max_cuwh; //entry point CTB's x location
            y_l = y_loc << ctx->log2_max_cuwh; //entry point CTB's y location
            x_r = x_l + ((int)(ctx->tile[tile_idx].w_ctb) << ctx->log2_max_cuwh);
            y_r = y_l + ((int)(ctx->tile[tile_idx].h_ctb) << ctx->log2_max_cuwh);
            w_tile = x_r > ((int)ctx->w_scu << MIN_CU_LOG2) ? ((int)ctx->w_scu << MIN_CU_LOG2) - x_l : x_r - x_l;
            h_tile = y_r > ((int)ctx->h_scu << MIN_CU_LOG2) ? ((int)ctx->h_scu << MIN_CU_LOG2) - y_l : y_r - y_l;
            pel * rec_temp_y_tile = rec_tmp_y + x_l + y_l * s;
            pel * rec_y_tile = rec_y + x_l + y_l * rec_stride;
            alf_copy_and_extend_tile(rec_temp_y_tile, s, rec_y_tile, rec_stride, w_tile, h_tile, m);
            AREA blk = { x_l, y_l, w_tile, h_tile };
            alf_derive_classification(alf, alf->classifier, rec_tmp_y, s, &blk);
            total_tiles_in_slice--;
        }
    }
    alf_copy_and_extend(rec_tmp_y, s, rec_y, rec_stride, w, h, m);
    if(ctx->sps.chroma_format_idc)
    {
        alf_copy_and_extend(rec_tmp_u, s1, rec_u, pir_rec->s_c, (w >> 1), (h >> 1), m);
        alf_copy_and_extend(ref_tmp_v, s1, rec_v, pir_rec->s_c, (w >> 1), (h >> 1), m);
    }

    // get CTB stats for filtering
    xeve_alf_derive_stats_filtering(enc_alf, &org_yuv, &rec_temp);

    // derive filter (luma)
    xeve_alf_encode(enc_alf, cs, alf_slice_param, LUMA_CH);

    // derive filter (chroma)
    if (alf_slice_param->enable_flag[Y_C])
    {
        xeve_alf_encode(enc_alf, cs, alf_slice_param, CHROMA_CH);
    }

    // temporal prediction
    if (ctx->slice_type != SLICE_I)
    {
        xeve_alf_derive_stats_filtering(enc_alf, &org_yuv, &rec_temp);
        xeve_alf_temporal_enc_aps_comp(enc_alf, cs, alf_slice_param);

        alf->reset_alf_buf_flag = FALSE;
        alf_slice_param->reset_alf_buf_flag = FALSE;
        if (alf_slice_param->temporal_alf_flag) {
            alf->strore2_alf_buf_flag = FALSE;
            alf_slice_param->store2_alf_buf_flag = FALSE;
        }
        else
        {
            alf->strore2_alf_buf_flag = TRUE;
            alf_slice_param->store2_alf_buf_flag = TRUE;
        }
    }
    else
    {
        alf_slice_param->store2_alf_buf_flag = TRUE;
        alf->strore2_alf_buf_flag = TRUE;
        alf_slice_param->reset_alf_buf_flag = TRUE;
        alf->reset_alf_buf_flag = TRUE;
    }
    for (int slice_num = 0; slice_num < ctx->param.num_slice_in_pic; slice_num++)
    {
        ctx->sh = &ctx->sh_array[slice_num];

        u32 k = 0;
        int tile_idx = 0;
        int total_tiles_in_slice = ctx->sh->num_tiles_in_slice;
        while (total_tiles_in_slice)
        {
            int tile_idx = ctx->sh->tile_order[k++];
            int x_loc = ((ctx->tile[tile_idx].ctba_rs_first) % ctx->w_lcu);
            int y_loc = ((ctx->tile[tile_idx].ctba_rs_first) / ctx->w_lcu);

            col_bd = 0;
            if (tile_idx% ctx->param.tile_columns)
            {
                int temp = tile_idx - 1;
                while (temp >= 0)
                {
                    col_bd += ctx->tile[temp].w_ctb;
                    if (!(temp % ctx->param.tile_columns)) break;
                    temp--;
                }
            }
            else
            {
                col_bd = 0;
            }

            x_l = x_loc << ctx->log2_max_cuwh; //entry point CTB's x location
            y_l = y_loc << ctx->log2_max_cuwh; //entry point CTB's y location
            x_r = x_l + ((int)(ctx->tile[tile_idx].w_ctb) << ctx->log2_max_cuwh);
            y_r = y_l + ((int)(ctx->tile[tile_idx].h_ctb) << ctx->log2_max_cuwh);
            w_tile = x_r > ((int)ctx->w_scu << MIN_CU_LOG2) ? ((int)ctx->w_scu << MIN_CU_LOG2) - x_l : x_r - x_l;
            h_tile = y_r > ((int)ctx->h_scu << MIN_CU_LOG2) ? ((int)ctx->h_scu << MIN_CU_LOG2) - y_l : y_r - y_l;
            //This is for YUV420 only
            pel * rec_temp_y_tile = rec_tmp_y + x_l + y_l * s;
            pel * rec_temp_u_tile = rec_tmp_u + (x_l >> 1) + (y_l >> 1) * (s1);
            pel * rec_temp_v_tile = ref_tmp_v + (x_l >> 1) + (y_l >> 1) * (s1);
            pel * rec_y_tile = rec_y + x_l + y_l * rec_stride;
            pel * rec_u_tile = rec_u + (x_l >> 1) + (y_l >> 1) * pir_rec->s_c;
            pel * rec_v_tile = rec_v + (x_l >> 1) + (y_l >> 1) * pir_rec->s_c;

            alf_copy_and_extend_tile(rec_temp_y_tile, s, rec_y_tile, rec_stride, w_tile, h_tile, m);
            if (ctx->sps.chroma_format_idc)
            {
                alf_copy_and_extend_tile(rec_temp_u_tile, s1, rec_u_tile, pir_rec->s_c, (w_tile >> 1), (h_tile >> 1), m);
                alf_copy_and_extend_tile(rec_temp_v_tile, s1, rec_v_tile, pir_rec->s_c, (w_tile >> 1), (h_tile >> 1), m);
            }

            // reconstruct
            if (alf_slice_param->enable_flag[Y_C])
            {
                xeve_alf_recon(enc_alf, cs, alf_slice_param, org_yuv.yuv[0], org_yuv.s[0], rec_temp.yuv[0], rec_temp.s[0], Y_C, tile_idx, col_bd);
            }
            if (alf_slice_param->enable_flag[U_C] && ctx->sps.chroma_format_idc)
            {
                xeve_alf_recon(enc_alf, cs, alf_slice_param, org_yuv.yuv[1], org_yuv.s[1], rec_temp.yuv[1], rec_temp.s[1], U_C, tile_idx, col_bd);
            }
            if (alf_slice_param->enable_flag[V_C] && ctx->sps.chroma_format_idc)
            {
                xeve_alf_recon(enc_alf, cs, alf_slice_param, org_yuv.yuv[2], org_yuv.s[2], rec_temp.yuv[2], rec_temp.s[2], V_C, tile_idx, col_bd);
            }
            total_tiles_in_slice--;
        }
    }

    for (int i = 0; i < (int)ctx->f_lcu; i++)
    {
        if (*(alf_slice_param->alf_ctb_flag + i) == 0)
        {
            alf_slice_param->is_ctb_alf_on = TRUE;
            break;
        }
        else
        {
            alf_slice_param->is_ctb_alf_on = FALSE;
        }
    }
}

double xeve_alf_derive_ctb_enable_flags(XEVE_ALF * enc_alf, CODING_STRUCTURE * cs, const int input_shape_idx, u8 comp_id, const int num_classes, const int num_coef, double* dist_unfilter, BOOL rec_coef)
{

    ADAPTIVE_LOOP_FILTER * alf = &enc_alf->alf;
    u8 channel = comp_id > Y_C ? CHROMA_CH : LUMA_CH;
    u8 is_luma = comp_id  == Y_C ? 1 : 0;
    u8 is_chroma = !is_luma;

    double cost = 0;
    *dist_unfilter = 0;
    xeve_alf_set_enable_flag(&enc_alf->alf_slice_param_temp, comp_id, TRUE);

    if (is_chroma)
    {
        enc_alf->alf_slice_param_temp.chroma_ctb_present_flag = FALSE;
    }
    if (rec_coef)
    {
        alf_recon_coef(alf, &enc_alf->alf_slice_param_temp, channel, TRUE, is_luma);
        for (int class_idx = 0; class_idx < (is_luma ? MAX_NUM_ALF_CLASSES : 1); class_idx++)
        {
            for (int i = 0; i < (is_luma ? MAX_NUM_ALF_LUMA_COEFF : MAX_NUM_ALF_CHROMA_COEFF); i++)
            {
                enc_alf->filter_coef_set[class_idx][i] = is_luma ? alf->coef_final[class_idx* MAX_NUM_ALF_LUMA_COEFF + i] : enc_alf->alf_slice_param_temp.chroma_coef[i];
            }
        }
    }

    for (int ctu_idx = 0; ctu_idx < alf->num_ctu_in_pic; ctu_idx++)
    {
        double dist_unfilter_ctu = xeve_alf_get_unfiltered_dist(enc_alf->alf_cov[comp_id][input_shape_idx][ctu_idx], num_classes);
        double cost_on = 0;
        cost_on = dist_unfilter_ctu + xeve_alf_get_filtered_dist(enc_alf, enc_alf->alf_cov[comp_id][input_shape_idx][ctu_idx], num_classes, enc_alf->alf_slice_param_temp.num_luma_filters - 1, num_coef);
        alf->ctu_enable_flag[comp_id][ctu_idx] = 0;
        double costOff = dist_unfilter_ctu;

        if (cost_on < costOff)
        {
            cost += cost_on;
            alf->ctu_enable_flag[comp_id][ctu_idx] = 1;
        }
        else
        {
            cost += costOff;
            alf->ctu_enable_flag[comp_id][ctu_idx] = 0;
            *dist_unfilter += dist_unfilter_ctu;
        }
    }
    if (is_chroma)
    {
        xeve_alf_set_enable_ctb_flag(enc_alf, &enc_alf->alf_slice_param_temp, comp_id, alf->ctu_enable_flag);
        const int alf_chroma_idc = enc_alf->alf_slice_param_temp.enable_flag[U_C] * 2 + enc_alf->alf_slice_param_temp.enable_flag[V_C];
        cost += xeve_alf_lenth_truncated_unary(alf_chroma_idc, 3) * enc_alf->lambda[comp_id];
    }
    return cost;
}


void xeve_alf_encode(XEVE_ALF * enc_alf, CODING_STRUCTURE * cs, ALF_SLICE_PARAM* alf_slice_param, const int channel)
{
    u8 filter_conformance_flag = 0;
    double cost_min = DBL_MAX;
    double cost_min_cb = DBL_MAX;
    double cost_min_cr = DBL_MAX;

    u8 is_luma = channel == LUMA_CH ? 1 : 0;
    u8 is_chroma = !is_luma;

    XEVE_CTX* ctx = (XEVE_CTX*)cs->ctx;
    ALF_FILTER_SHAPE* alf_filter_shape = alf_slice_param->filterShapes[channel];
    ADAPTIVE_LOOP_FILTER * alf = &enc_alf->alf;

    const int num_classes = is_luma ? MAX_NUM_ALF_CLASSES : 1;
    int input_coef_bits = 0;

    const int size = channel == LUMA_CH ? 2 : 1;

    int covLrgIdx = size - 1;
    for (int input_shape_idx = 0; input_shape_idx < size; input_shape_idx++)
    {
        alf_copy_param(&enc_alf->alf_slice_param_temp, alf_slice_param);
        if (is_luma)
        {
            enc_alf->alf_slice_param_temp.luma_filter_type = (ALF_FILTER_TYPE)(alf_filter_shape[input_shape_idx].filter_type);
        }
        double cost = cost_min;
        double cost_cb = cost_min_cb;
        double cost_cr = cost_min_cr;
        //1. get unfiltered distortion
        if (is_luma)
        {
            xeve_alf_set_ctb_enable_flag(enc_alf, alf->ctu_enable_flag, Y_C, 1);
            xeve_alf_get_frame_stats(enc_alf, Y_C, input_shape_idx);
        }
        if (is_chroma)
        {
            cost_cb = xeve_alf_get_unfiltered_dist_ch(enc_alf->alf_cov_frame[U_C][covLrgIdx], channel);
            cost_cb = cost_cb / 1.001;
            cost_cr = xeve_alf_get_unfiltered_dist_ch(enc_alf->alf_cov_frame[V_C][covLrgIdx], channel);
            cost_cr = cost_cr / 1.001;
            if (cost_cb < cost_min_cb)
            {
                cost_min_cb = cost_cb;
                xeve_alf_set_enable_flag(alf_slice_param, U_C, FALSE);
                xeve_alf_set_ctb_enable_flag(enc_alf, enc_alf->ctu_enable_flag_temp, U_C, 0);
                alf_slice_param->chroma_ctb_present_flag = FALSE;
            }
            if (cost_cr < cost_min_cr)
            {
                cost_min_cr = cost_cr;
                xeve_alf_set_enable_flag(alf_slice_param, V_C, FALSE);
                xeve_alf_set_ctb_enable_flag(enc_alf, enc_alf->ctu_enable_flag_temp, V_C, 0);
                alf_slice_param->chroma_ctb_present_flag = FALSE;
            }
        }
        else
        {
            cost = xeve_alf_get_unfiltered_dist_ch(enc_alf->alf_cov_frame[channel][input_shape_idx], channel);
            cost /= 1.001; // slight preference for unfiltered choice
            if (cost < cost_min)
            {
                cost_min = cost;
                xeve_alf_set_enable_flag(alf_slice_param, Y_C, FALSE);
                xeve_alf_set_ctb_enable_flag(enc_alf, enc_alf->ctu_enable_flag_temp, Y_C, 0);
            }
        }

        //2. all CTUs are on
        if (is_chroma)
        {
            enc_alf->alf_slice_param_temp.chroma_ctb_present_flag = TRUE;
        }

        if (is_luma)
        {
            xeve_alf_set_enable_flag(&enc_alf->alf_slice_param_temp, Y_C, TRUE);
            xeve_alf_set_ctb_enable_flag(enc_alf, alf->ctu_enable_flag, Y_C, 1);
            cost = xeve_alf_get_filter_coef_cost(enc_alf, cs, 0, Y_C, is_luma, input_shape_idx, &input_coef_bits, &filter_conformance_flag);

            if (filter_conformance_flag)
            {
                xeve_alf_set_enable_flag(&enc_alf->alf_slice_param_temp, Y_C, FALSE);
            }

            cost += enc_alf->lambda[channel];
            if (cost < cost_min)
            {
                cost_min = cost;
                xeve_alf_copy_slice_param(enc_alf, alf_slice_param, &enc_alf->alf_slice_param_temp, channel);
                xeve_alf_set_ctb_enable_flag(enc_alf, enc_alf->ctu_enable_flag_temp, Y_C, 1);
            }
        }
        else
        {
            xeve_alf_set_enable_flag(&enc_alf->alf_slice_param_temp, U_C, TRUE);
            xeve_alf_set_ctb_enable_flag(enc_alf, alf->ctu_enable_flag, U_C, 1);
            xeve_alf_set_enable_flag(&enc_alf->alf_slice_param_temp, V_C, TRUE);
            xeve_alf_set_ctb_enable_flag(enc_alf, alf->ctu_enable_flag, V_C, 1);
            double filter_cost[3] = { DBL_MAX, DBL_MAX, DBL_MAX };
            xeve_alf_get_filter_coef_cost_ch(enc_alf, cs, 0, U_C, input_shape_idx, &input_coef_bits, filter_cost);

            filter_cost[0] += enc_alf->lambda[U_C];
            filter_cost[1] += enc_alf->lambda[U_C];
            filter_cost[2] += enc_alf->lambda[U_C];

            if (filter_cost[2] < cost_cb + cost_cr)
            {
                cost_min_cb = filter_cost[2] / 2;
                cost_min_cr = filter_cost[2] / 2;
                xeve_alf_set_ctb_enable_flag(enc_alf, enc_alf->ctu_enable_flag_temp, U_C, 1);
                xeve_alf_set_ctb_enable_flag(enc_alf, enc_alf->ctu_enable_flag_temp, V_C, 1);
                xeve_alf_set_enable_flag(&enc_alf->alf_slice_param_temp, U_C, TRUE);
                xeve_alf_set_enable_flag(&enc_alf->alf_slice_param_temp, V_C, TRUE);
                xeve_alf_copy_slice_param(enc_alf, alf_slice_param, &enc_alf->alf_slice_param_temp, channel);
            }
            else if (filter_cost[0] < cost_cb)
            {
                cost_min_cb = filter_cost[0];
                xeve_alf_set_ctb_enable_flag(enc_alf, enc_alf->ctu_enable_flag_temp, U_C, 1);
                xeve_alf_set_ctb_enable_flag(enc_alf, enc_alf->ctu_enable_flag_temp, V_C, 0);
                xeve_alf_set_enable_flag(&enc_alf->alf_slice_param_temp, U_C, TRUE);
                xeve_alf_set_enable_flag(&enc_alf->alf_slice_param_temp, V_C, FALSE);
                xeve_alf_copy_slice_param(enc_alf, alf_slice_param, &enc_alf->alf_slice_param_temp, channel);
            }
            else if (filter_cost[1] < cost_cr)
            {
                cost_min_cr = filter_cost[1];
                xeve_alf_set_ctb_enable_flag(enc_alf, enc_alf->ctu_enable_flag_temp, U_C, 0);
                xeve_alf_set_ctb_enable_flag(enc_alf, enc_alf->ctu_enable_flag_temp, V_C, 1);
                xeve_alf_set_enable_flag(&enc_alf->alf_slice_param_temp, U_C, FALSE);
                xeve_alf_set_enable_flag(&enc_alf->alf_slice_param_temp, V_C, TRUE);
                xeve_alf_copy_slice_param(enc_alf, alf_slice_param, &enc_alf->alf_slice_param_temp, channel);
            }
            else
            {
                xeve_alf_set_ctb_enable_flag(enc_alf, enc_alf->ctu_enable_flag_temp, U_C, 0);
                xeve_alf_set_ctb_enable_flag(enc_alf, enc_alf->ctu_enable_flag_temp, V_C, 0);
                xeve_alf_set_enable_flag(&enc_alf->alf_slice_param_temp, U_C, FALSE);
                xeve_alf_set_enable_flag(&enc_alf->alf_slice_param_temp, V_C, FALSE);
                xeve_alf_copy_slice_param(enc_alf, alf_slice_param, &enc_alf->alf_slice_param_temp, channel);
            }
        }

        //3. CTU decision
        if (channel != CHROMA_CH && !filter_conformance_flag)
        {
            double dist_unfilter = 0;
            const int iter_num = 2 * 2 + 1;

            for (int iter = 0; iter < iter_num; iter++)
            {
                if ((iter & 0x01) == 0)
                {
                    if (!filter_conformance_flag)
                    {
                        cost = enc_alf->lambda[channel] * input_coef_bits;
                        cost += xeve_alf_derive_ctb_enable_flags(enc_alf, cs, input_shape_idx, Y_C, num_classes, (input_shape_idx ? 13 : 7), &dist_unfilter, TRUE);
                        cost += enc_alf->lambda[channel] * (alf->num_ctu_in_pic);

                        if (cost < cost_min)
                        {
                            cost_min = cost;
                            xeve_alf_copy_ctb_enable_flag(enc_alf, enc_alf->ctu_enable_flag_temp, alf->ctu_enable_flag, Y_C);
                            xeve_alf_copy_slice_param(enc_alf, alf_slice_param, &enc_alf->alf_slice_param_temp, channel);
                            alf_slice_param->is_ctb_alf_on = TRUE;
                        }
                    }
                }
                else
                {
                    xeve_alf_set_enable_flag(&enc_alf->alf_slice_param_temp, Y_C, TRUE);
                    cost = xeve_alf_get_filter_coef_cost(enc_alf, cs, dist_unfilter, Y_C, TRUE, input_shape_idx, &input_coef_bits, &filter_conformance_flag);
                    if (filter_conformance_flag)
                    {
                        xeve_alf_set_enable_flag(&enc_alf->alf_slice_param_temp, Y_C, FALSE);
                    }
                    else
                    {
                        xeve_alf_set_enable_flag(&enc_alf->alf_slice_param_temp, Y_C, TRUE);
                    }
                }
            }//for iter
        }
    }//for shapeIdx

    if (is_luma)
    {
        enc_alf->cost_alf_encoder[channel] = cost_min;
        xeve_alf_copy_ctb_enable_flag(enc_alf, alf->ctu_enable_flag, enc_alf->ctu_enable_flag_temp, Y_C);
    }
    else
    {
        enc_alf->cost_alf_encoder[U_C] = cost_min_cb;
        enc_alf->cost_alf_encoder[V_C] = cost_min_cr;
        xeve_alf_copy_ctb_enable_flag(enc_alf, alf->ctu_enable_flag, enc_alf->ctu_enable_flag_temp, U_C);
        xeve_alf_copy_ctb_enable_flag(enc_alf, alf->ctu_enable_flag, enc_alf->ctu_enable_flag_temp, V_C);
    }
}

void tile_boundary_check(int* avail_left, int* avail_right, int* avail_top, int* avail_bottom, const int width, const int height, int x_pos, int y_pos, int x_l, int x_r, int y_l, int y_r)
{
    if (x_pos == x_l)
    {
        *avail_left = 0;
    }
    else
    {
        *avail_left = 1;
    }

    if (x_pos + width == x_r)
    {
        *avail_right = 0;
    }
    else
    {
        *avail_right = 1;
    }

    if (y_pos == y_l)
    {
        *avail_top = 0;
    }
    else
    {
        *avail_top = 1;
    }

    if (y_pos + height == y_r)
    {
        *avail_bottom = 0;
    }
    else
    {
        *avail_bottom = 1;
    }
}

int xeve_alf_recon(XEVE_ALF * enc_alf, CODING_STRUCTURE * cs, ALF_SLICE_PARAM* alf_slice_param, const pel * org_unit_buf, const int org_stride
                  , pel * rec_ext_buf, const int rec_stride, const u8 comp_id, int tile_idx, int col_bd)
{
    int ret;
    ADAPTIVE_LOOP_FILTER * alf = &enc_alf->alf;
    int x_l, x_r, y_l, y_r;
    pel* buffer_l = NULL;
    pel* buffer_cb = NULL;
    pel* buffer_cr = NULL;

    const u8 channel = comp_id == Y_C ? LUMA_CH : CHROMA_CH;
    u8 is_luma = channel == LUMA_CH ? 1 : 0;

    alf_recon_coef(alf, alf_slice_param, channel, FALSE, is_luma);
    XEVE_CTX* ctx = (XEVE_CTX*)(cs->ctx);
    XEVE_PIC* rec_pic = PIC_MODE(ctx);
    pel * rec_buf = NULL;

    int x_loc = ((ctx->tile[tile_idx].ctba_rs_first) % ctx->w_lcu);
    int y_loc = ((ctx->tile[tile_idx].ctba_rs_first) / ctx->w_lcu);
    x_l = x_loc << ctx->log2_max_cuwh; //entry point lcu's x location
    y_l = y_loc << ctx->log2_max_cuwh; // entry point lcu's y location
    x_r = x_l + ((int)(ctx->tile[tile_idx].w_ctb) << ctx->log2_max_cuwh);
    y_r = y_l + ((int)(ctx->tile[tile_idx].h_ctb) << ctx->log2_max_cuwh);
    x_r = x_r > ((int)ctx->w_scu << MIN_CU_LOG2) ? ((int)ctx->w_scu << MIN_CU_LOG2) : x_r;
    y_r = y_r > ((int)ctx->h_scu << MIN_CU_LOG2) ? ((int)ctx->h_scu << MIN_CU_LOG2) : y_r;

    switch (comp_id)
    {
    case Y_C:
        rec_buf = rec_pic->y;
        break;
    case U_C:
        rec_buf = rec_pic->u;
        break;
    case V_C:
        rec_buf = rec_pic->v;
        break;
    default:
        assert(0);
    }

    const int m = MAX_ALF_FILTER_LENGTH >> 1;
    int l_zero_offset = (MAX_CU_SIZE + m + m) * m + m;
    int l_stride = MAX_CU_SIZE + 2 * m;
    buffer_l = (pel*)xeve_malloc(sizeof(pel) * (MAX_CU_SIZE + 2 * m) * (MAX_CU_SIZE + 2 * m));
    xeve_assert_gv(buffer_l, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    xeve_mset(buffer_l, 0, sizeof(pel) * (MAX_CU_SIZE + 2 * m) * (MAX_CU_SIZE + 2 * m));
    pel *tmp_buffer = buffer_l + l_zero_offset;
    int l_zero_offset_chroma = ((MAX_CU_SIZE >> 1) + m + m) * m + m;
    int l_stride_chroma = (MAX_CU_SIZE >> 1) + m + m;
    buffer_cb = (pel*)xeve_malloc(sizeof(pel) * ((MAX_CU_SIZE >> 1) + 2 * m) *((MAX_CU_SIZE >> 1) + 2 * m));
    xeve_assert_gv(buffer_cb, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    buffer_cr = (pel*)xeve_malloc(sizeof(pel) * ((MAX_CU_SIZE >> 1) + 2 * m) *((MAX_CU_SIZE >> 1) + 2 * m));
    xeve_assert_gv(buffer_cr, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    xeve_mset(buffer_cb, 0, sizeof(pel) * ((MAX_CU_SIZE >> 1) + 2 * m) *((MAX_CU_SIZE >> 1) + 2 * m));
    xeve_mset(buffer_cr, 0, sizeof(pel) * ((MAX_CU_SIZE >> 1) + 2 * m) *((MAX_CU_SIZE >> 1) + 2 * m));
    pel *tmp_buffer_cb = buffer_cb + l_zero_offset_chroma;
    pel *tmp_buffer_cr = buffer_cr + l_zero_offset_chroma;

    if (alf_slice_param->enable_flag[comp_id])
    {
        const int chroma_scale_x = is_luma ? 0 : 1;
        const int chroma_scale_y = is_luma ? 0 : 1; //getComponentScaleY(comp_id, rec_buf.chromaFormat);
        int ctu_idx = (x_loc)+(y_loc)* ctx->w_lcu;

        ALF_FILTER_TYPE filter_type = comp_id == Y_C ? ALF_FILTER_7 : ALF_FILTER_5;
        short* coeff = comp_id == Y_C ? alf->coef_final : alf_slice_param->chroma_coef;
        for (int y_pos = y_l; y_pos < y_r; y_pos += ctx->max_cuwh)
        {
            for (int x_pos = x_l; x_pos < x_r; x_pos += ctx->max_cuwh)
            {
                const int width = (x_pos + ctx->max_cuwh > rec_pic->w_l) ? (rec_pic->w_l - x_pos) : ctx->max_cuwh;
                const int height = (y_pos + ctx->max_cuwh > rec_pic->h_l) ? (rec_pic->h_l - y_pos) : ctx->max_cuwh;

                int avail_left, avail_right, avail_top, avail_bottom;
                avail_left = avail_right = avail_top = avail_bottom = 1;
                if (!(ctx->pps.loop_filter_across_tiles_enabled_flag))
                {
                    tile_boundary_check(&avail_left, &avail_right, &avail_top, &avail_bottom, width, height, x_pos, y_pos, x_l, x_r, y_l, y_r);
                }
                else
                {
                    tile_boundary_check(&avail_left, &avail_right, &avail_top, &avail_bottom, width, height, x_pos, y_pos,
                                        0, ctx->sps.pic_width_in_luma_samples - 1, 0, ctx->sps.pic_height_in_luma_samples - 1);
                }
                if (comp_id == Y_C)
                {
                    for (int i = m; i < height + m; i++)
                    {
                        int dst_pos = i * l_stride - l_zero_offset;
                        int src_pos_offset = x_pos + y_pos * rec_stride;
                        int stride = (width == ctx->max_cuwh ? l_stride : width + m + m);
                        xeve_mcpy(tmp_buffer + dst_pos + m, rec_ext_buf + src_pos_offset + (i - m) * rec_stride, sizeof(pel) * (stride - 2 * m));
                        for (int j = 0; j < m; j++)
                        {
                            if (avail_left)
                                tmp_buffer[dst_pos + j] = rec_ext_buf[src_pos_offset + (i - m) * rec_stride - m + j];
                            else
                                tmp_buffer[dst_pos + j] = rec_ext_buf[src_pos_offset + (i - m) * rec_stride + m - j];
                            if (avail_right)
                                tmp_buffer[dst_pos + j + width + m] = rec_ext_buf[src_pos_offset + (i - m) * rec_stride + width + j];
                            else
                                tmp_buffer[dst_pos + j + width + m] = rec_ext_buf[src_pos_offset + (i - m) * rec_stride + width - j - 2];
                        }
                    }
                    for (int i = 0; i < m; i++)
                    {
                        int dst_pos = i * l_stride - l_zero_offset;
                        int src_pos_offset = x_pos + y_pos * rec_stride;
                        int stride = (width == ctx->max_cuwh ? l_stride : width + m + m);
                        if (avail_top)
                            xeve_mcpy(tmp_buffer + dst_pos, rec_ext_buf + src_pos_offset - (m - i) * rec_stride - m, sizeof(pel) * stride);
                        else
                            xeve_mcpy(tmp_buffer + dst_pos, tmp_buffer + dst_pos + (2 * m - 2 * i) * l_stride, sizeof(pel) * stride);
                    }
                    for (int i = height + m; i < height + m + m; i++)
                    {
                        int dst_pos = i * l_stride - l_zero_offset;
                        int src_pos_offset = x_pos + y_pos * rec_stride;
                        int stride = (width == ctx->max_cuwh ? l_stride : width + m + m);
                        if (avail_bottom)
                            xeve_mcpy(tmp_buffer + dst_pos, rec_ext_buf + src_pos_offset + (i - m) * rec_stride - m, sizeof(pel) * stride);
                        else
                            xeve_mcpy(tmp_buffer + dst_pos, tmp_buffer + dst_pos - (2 * (i - height - m) + 2) * l_stride, sizeof(pel) * stride);
                    }
                }
                else if (comp_id == U_C && ctx->sps.chroma_format_idc)
                {
                    for (int i = m; i < ((height >> 1) + m); i++)
                    {
                        int dst_pos = i * l_stride_chroma - l_zero_offset_chroma;
                        int src_pos_offset = (x_pos >> 1) + (y_pos >> 1) * rec_stride;
                        int stride = (width == ctx->max_cuwh ? l_stride_chroma : (width >> 1) + m + m);
                        xeve_mcpy(tmp_buffer_cb + dst_pos + m, rec_ext_buf + src_pos_offset + (i - m) * rec_stride, sizeof(pel) * (stride - 2 * m));
                        for (int j = 0; j < m; j++)
                        {
                            if (avail_left)
                                tmp_buffer_cb[dst_pos + j] = rec_ext_buf[src_pos_offset + (i - m) * rec_stride - m + j];
                            else
                                tmp_buffer_cb[dst_pos + j] = rec_ext_buf[src_pos_offset + (i - m) * rec_stride + m - j];
                            if (avail_right)
                                tmp_buffer_cb[dst_pos + j + (width >> 1) + m] = rec_ext_buf[src_pos_offset + (i - m) * rec_stride + (width >> 1) + j];
                            else
                                tmp_buffer_cb[dst_pos + j + (width >> 1) + m] = rec_ext_buf[src_pos_offset + (i - m) * rec_stride + (width >> 1) - j - 2];
                        }
                    }

                    for (int i = 0; i < m; i++)
                    {
                        int dst_pos = i * l_stride_chroma - l_zero_offset_chroma;
                        int src_pos_offset = (x_pos >> 1) + (y_pos >> 1) * rec_stride;
                        int stride = (width == ctx->max_cuwh ? l_stride_chroma : (width >> 1) + m + m);
                        if (avail_top)
                            xeve_mcpy(tmp_buffer_cb + dst_pos, rec_ext_buf + src_pos_offset - (m - i) * rec_stride - m, sizeof(pel) * stride);
                        else
                            xeve_mcpy(tmp_buffer_cb + dst_pos, tmp_buffer_cb + dst_pos + (2 * m - 2 * i) * l_stride_chroma, sizeof(pel) * stride);
                    }

                    for (int i = ((height >> 1) + m); i < ((height >> 1) + m + m); i++)
                    {
                        int dst_pos = i * l_stride_chroma - l_zero_offset_chroma;
                        int src_pos_offset = (x_pos >> 1) + (y_pos >> 1) * rec_stride;
                        int stride = (width == ctx->max_cuwh ? l_stride_chroma : (width >> 1) + m + m);
                        if (avail_bottom)
                            xeve_mcpy(tmp_buffer_cb + dst_pos, rec_ext_buf + src_pos_offset + (i - m) * rec_stride - m, sizeof(pel) * stride);
                        else
                            xeve_mcpy(tmp_buffer_cb + dst_pos, tmp_buffer_cb + dst_pos - (2 * (i - (height >> 1) - m) + 2) * l_stride_chroma, sizeof(pel) * stride);
                    }
                }
                else if(ctx->sps.chroma_format_idc)
                {
                    for (int i = m; i < ((height >> 1) + m); i++)
                    {
                        int dst_pos = i * l_stride_chroma - l_zero_offset_chroma;
                        int src_pos_offset = (x_pos >> 1) + (y_pos >> 1) * rec_stride;
                        int stride = (width == ctx->max_cuwh ? l_stride_chroma : (width >> 1) + m + m);
                        xeve_mcpy(tmp_buffer_cr + dst_pos + m, rec_ext_buf + src_pos_offset + (i - m) * rec_stride, sizeof(pel) * (stride - 2 * m));
                        for (int j = 0; j < m; j++)
                        {
                            if (avail_left)
                                tmp_buffer_cr[dst_pos + j] = rec_ext_buf[src_pos_offset + (i - m) * rec_stride - m + j];
                            else
                                tmp_buffer_cr[dst_pos + j] = rec_ext_buf[src_pos_offset + (i - m) * rec_stride + m - j];
                            if (avail_right)
                                tmp_buffer_cr[dst_pos + j + (width >> 1) + m] = rec_ext_buf[src_pos_offset + (i - m) * rec_stride + (width >> 1) + j];
                            else
                                tmp_buffer_cr[dst_pos + j + (width >> 1) + m] = rec_ext_buf[src_pos_offset + (i - m) * rec_stride + (width >> 1) - j - 2];
                        }
                    }

                    for (int i = 0; i < m; i++)
                    {
                        int dst_pos = i * l_stride_chroma - l_zero_offset_chroma;
                        int src_pos_offset = (x_pos >> 1) + (y_pos >> 1) * rec_stride;
                        int stride = (width == ctx->max_cuwh ? l_stride_chroma : (width >> 1) + m + m);
                        if (avail_top)
                            xeve_mcpy(tmp_buffer_cr + dst_pos, rec_ext_buf + src_pos_offset - (m - i) * rec_stride - m, sizeof(pel) * stride);
                        else
                            xeve_mcpy(tmp_buffer_cr + dst_pos, tmp_buffer_cr + dst_pos + (2 * m - 2 * i) * l_stride_chroma, sizeof(pel) * stride);
                    }

                    for (int i = ((height >> 1) + m); i < ((height >> 1) + m + m); i++)
                    {
                        int dst_pos = i * l_stride_chroma - l_zero_offset_chroma;
                        int src_pos_offset = (x_pos >> 1) + (y_pos >> 1) * rec_stride;
                        int stride = (width == ctx->max_cuwh ? l_stride_chroma : (width >> 1) + m + m);
                        if (avail_bottom)
                            xeve_mcpy(tmp_buffer_cr + dst_pos, rec_ext_buf + src_pos_offset + (i - m) * rec_stride - m, sizeof(pel) * stride);
                        else
                            xeve_mcpy(tmp_buffer_cr + dst_pos, tmp_buffer_cr + dst_pos - (2 * (i - (height >> 1) - m) + 2) * l_stride_chroma, sizeof(pel) * stride);
                    }
                }
                AREA blk = { 0, 0, width >> chroma_scale_x, height >> chroma_scale_y };

                if (alf->ctu_enable_flag[comp_id][ctu_idx])
                {
                    int stride = is_luma ? rec_pic->s_l : rec_pic->s_c;

                    if (filter_type == ALF_FILTER_5)
                    {
                        if (comp_id == U_C)
                        {
                            enc_alf->alf.filter_5x5_blk(alf->classifier, rec_buf + (x_pos >> 1) + (y_pos >> 1) * rec_pic->s_c, rec_pic->s_c, tmp_buffer_cb, l_stride_chroma, &blk, comp_id, coeff, &(alf->clip_ranges.comp[(int)comp_id]));
                        }
                        else
                        {
                            enc_alf->alf.filter_5x5_blk(alf->classifier, rec_buf + (x_pos >> 1) + (y_pos >> 1) * rec_pic->s_c, rec_pic->s_c, tmp_buffer_cr, l_stride_chroma, &blk, comp_id, coeff, &(alf->clip_ranges.comp[(int)comp_id]));
                        }
                    }
                    else if (filter_type == ALF_FILTER_7)
                    {
                        alf_derive_classification(alf, alf->classifier, tmp_buffer, l_stride, &blk);
                        enc_alf->alf.filter_7x7_blk(alf->classifier, rec_buf + x_pos + y_pos * (rec_pic->s_l), rec_pic->s_l, tmp_buffer, l_stride, &blk, comp_id, coeff, &(alf->clip_ranges.comp[(int)comp_id]));
                    }
                    else
                    {
                        CHECK(0, "Wrong ALF filter type");
                    }
                }

                x_loc++;

                if (x_loc >= ctx->tile[tile_idx].w_ctb + col_bd)
                {
                    x_loc = ((ctx->tile[tile_idx].ctba_rs_first) % ctx->w_lcu);
                    y_loc++;
                }
                ctu_idx = x_loc + y_loc * ctx->w_lcu;
            }
        }
    }
    xeve_mfree(buffer_l);
    xeve_mfree(buffer_cb);
    xeve_mfree(buffer_cr);
    return 0;
ERR:
    xeve_mfree(buffer_l);
    xeve_mfree(buffer_cb);
    xeve_mfree(buffer_cr);
    return -1;
}

void xeve_alf_temporal_enc_aps_comp(XEVE_ALF * enc_alf, CODING_STRUCTURE * cs, ALF_SLICE_PARAM* alf_slice_param)
{
    XEVE_CTX* ctx = (XEVE_CTX*)cs->ctx;
    ADAPTIVE_LOOP_FILTER * alf = &enc_alf->alf;
    const int temp_layer_id = ctx->nalu.nuh_temporal_id;
    int prev_idx_comp[NUM_CH] = { -1, -1 };
    int talf_comp_enable[N_C] = { 0, 0, 0 };
    double unfilterd_cost_cb = DBL_MAX;
    double unfilterd_cost_cr = DBL_MAX;
    double unfilterd_cost_joint = DBL_MAX;
    ALF_SLICE_PARAM *stored_alf_param = ctx->slice_type == SLICE_I ? NULL : alf->ac_alf_line_buf;
    u8 channel, is_luma;

    alf_copy_param(&enc_alf->alf_slice_param_temp, alf_slice_param);
    xeve_alf_copy_ctb_enable_flag(enc_alf, enc_alf->ctu_enable_flag_temp, alf->ctu_enable_flag, Y_C);
    xeve_alf_copy_ctb_enable_flag(enc_alf, enc_alf->ctu_enable_flag_temp, alf->ctu_enable_flag, U_C);
    xeve_alf_copy_ctb_enable_flag(enc_alf, enc_alf->ctu_enable_flag_temp, alf->ctu_enable_flag, V_C);

     if (stored_alf_param != NULL && alf->ac_alf_line_buf_curr_size > 0)
    {
        double cost_best[N_C] = { DBL_MAX, DBL_MAX, DBL_MAX };
        for (int buf_idx2 = 0; buf_idx2 < alf->ac_alf_line_buf_curr_size && buf_idx2 < APS_MAX_NUM; buf_idx2++)
        {
            double cost[N_C + 1] = { DBL_MAX, DBL_MAX, DBL_MAX, DBL_MAX };
            int buf_idx = buf_idx2;
            buf_idx = alf->alf_idx_in_scan_order[buf_idx2];
            {
                if ((stored_alf_param[buf_idx].t_layer > temp_layer_id) && (ctx->param.keyint != 0))
                {
                    continue;
                }
                if ((alf->curr_poc > stored_alf_param[buf_idx].max_idr_poc + ctx->param.keyint) && (ctx->param.keyint != 0))
                {
                    continue;
                }

                if ((alf->curr_poc > alf->last_idr_poc) && (stored_alf_param[buf_idx].filter_poc < alf->last_idr_poc))
                {
                    continue;
                }

                if ((alf->curr_poc > stored_alf_param[buf_idx].max_idr_poc) && (stored_alf_param[buf_idx].filter_poc < stored_alf_param[buf_idx].max_idr_poc))
                {
                    continue;
                }
            }

            alf_copy_param(&enc_alf->alf_slice_param_temp, &(stored_alf_param[buf_idx]));

            for (u8 ch = 0; ch < N_C; ch++)
            {
                channel = ch > Y_C ? CHROMA_CH : LUMA_CH;
                is_luma = channel == LUMA_CH ? 1 : 0;
                {
                    int filter_avail = (ch == Y_C) ? enc_alf->alf_slice_param_temp.enable_flag[Y_C] :
                                            (ch == U_C ? enc_alf->alf_slice_param_temp.enable_flag[U_C] : enc_alf->alf_slice_param_temp.enable_flag[V_C]);
                    if (filter_avail)
                    {
                        int input_shape_idx = enc_alf->alf_slice_param_temp.luma_filter_type;
                        if (ch == Y_C)
                        {
                            double dist_unfilter;
                            cost[ch] = xeve_alf_derive_ctb_enable_flags(enc_alf, cs, input_shape_idx, Y_C, is_luma ? MAX_NUM_ALF_CLASSES : 1, alf->filter_shapes[channel][input_shape_idx].num_coef, &dist_unfilter, TRUE);
                            cost[ch] += enc_alf->lambda[Y_C] * APS_MAX_NUM_IN_BITS;
                            for (int i = 0; i < (int)ctx->f_lcu; i++)
                            {
                                if (alf->ctu_enable_flag[Y_C][i] == 0)
                                {
                                    enc_alf->alf_slice_param_temp.is_ctb_alf_on = TRUE;
                                    break;
                                }
                                else
                                {
                                    enc_alf->alf_slice_param_temp.is_ctb_alf_on = FALSE;
                                }
                            }
                            if (enc_alf->alf_slice_param_temp.is_ctb_alf_on)
                            {
                                cost[ch] += enc_alf->lambda[ch] * (ctx->f_lcu);
                            }
                        }
                        else if (ch == U_C)
                        {
                            double cost_ctb_enable = DBL_MAX;
                            xeve_alf_set_ctb_enable_flag(enc_alf, alf->ctu_enable_flag, U_C, TRUE);
                            xeve_alf_get_frame_stats(enc_alf, U_C, 0);
                            cost_ctb_enable = xeve_alf_get_unfiltered_dist_ch(enc_alf->alf_cov_frame[U_C][0], channel);
                            unfilterd_cost_cb = cost_ctb_enable;
                            alf_recon_coef(alf, &enc_alf->alf_slice_param_temp, channel, TRUE, is_luma);
                            for (int class_idx = 0; class_idx < (is_luma ? MAX_NUM_ALF_CLASSES : 1); class_idx++)
                            {
                                for (int i = 0; i < (is_luma ? MAX_NUM_ALF_LUMA_COEFF : MAX_NUM_ALF_CHROMA_COEFF); i++)
                                {
                                    enc_alf->filter_coef_set[class_idx][i] = is_luma ? alf->coef_final[class_idx* MAX_NUM_ALF_LUMA_COEFF + i] : enc_alf->alf_slice_param_temp.chroma_coef[i];
                                }
                            }
                            cost_ctb_enable += xeve_alf_get_filtered_dist(enc_alf, enc_alf->alf_cov_frame[U_C][0], 1, 0, MAX_NUM_ALF_CHROMA_COEFF);
                            cost[ch] = cost_ctb_enable;
                            cost[ch] += enc_alf->lambda[ch] * APS_MAX_NUM_IN_BITS;
                        }
                        else if (ch == V_C)
                        {
                            double cost_ctb_enable = DBL_MAX;
                            xeve_alf_set_ctb_enable_flag(enc_alf, alf->ctu_enable_flag, V_C, TRUE);
                            xeve_alf_get_frame_stats(enc_alf, V_C, 0);
                            cost_ctb_enable = xeve_alf_get_unfiltered_dist_ch(enc_alf->alf_cov_frame[V_C][0], channel);
                            unfilterd_cost_cr = cost_ctb_enable;
                            alf_recon_coef(alf, &enc_alf->alf_slice_param_temp, channel, TRUE, is_luma);
                            for (int class_idx = 0; class_idx < (is_luma ? MAX_NUM_ALF_CLASSES : 1); class_idx++)
                            {
                                for (int i = 0; i < (is_luma ? MAX_NUM_ALF_LUMA_COEFF : MAX_NUM_ALF_CHROMA_COEFF); i++)
                                {
                                    enc_alf->filter_coef_set[class_idx][i] = is_luma ? alf->coef_final[class_idx* MAX_NUM_ALF_LUMA_COEFF + i] : enc_alf->alf_slice_param_temp.chroma_coef[i];
                                }
                            }
                            cost_ctb_enable += xeve_alf_get_filtered_dist(enc_alf, enc_alf->alf_cov_frame[V_C][0], 1, 0, MAX_NUM_ALF_CHROMA_COEFF);
                            cost[ch] = cost_ctb_enable;
                            cost[ch] += enc_alf->lambda[ch] * APS_MAX_NUM_IN_BITS;
                            if (enc_alf->alf_slice_param_temp.enable_flag[U_C] && enc_alf->alf_slice_param_temp.enable_flag[V_C])
                            {
                                cost_ctb_enable = 0;
                                xeve_alf_set_ctb_enable_flag(enc_alf, alf->ctu_enable_flag, V_C, TRUE);
                                xeve_alf_set_ctb_enable_flag(enc_alf, alf->ctu_enable_flag, U_C, TRUE);
                                xeve_alf_get_frame_stats(enc_alf, V_C, 0);
                                xeve_alf_get_frame_stats(enc_alf, U_C, 0);
                                alf_cov_reset(&enc_alf->alf_cov_frame[N_C][0][0]);
                                alf_cov_add(&enc_alf->alf_cov_frame[N_C][0][0], &enc_alf->alf_cov_frame[U_C][0][0]);
                                alf_cov_add(&enc_alf->alf_cov_frame[N_C][0][0], &enc_alf->alf_cov_frame[V_C][0][0]);
                                cost_ctb_enable = xeve_alf_get_unfiltered_dist_ch(enc_alf->alf_cov_frame[N_C][0], channel);
                                unfilterd_cost_joint = cost_ctb_enable;
                                alf_recon_coef(alf, &enc_alf->alf_slice_param_temp, channel, TRUE, is_luma);
                                for (int class_idx = 0; class_idx < (is_luma ? MAX_NUM_ALF_CLASSES : 1); class_idx++)
                                {
                                    for (int i = 0; i < (is_luma ? MAX_NUM_ALF_LUMA_COEFF : MAX_NUM_ALF_CHROMA_COEFF); i++)
                                    {
                                        enc_alf->filter_coef_set[class_idx][i] = is_luma ? alf->coef_final[class_idx* MAX_NUM_ALF_LUMA_COEFF + i] : enc_alf->alf_slice_param_temp.chroma_coef[i];
                                    }
                                }
                                cost_ctb_enable += xeve_alf_get_filtered_dist(enc_alf, enc_alf->alf_cov_frame[N_C][0], 1, 0, MAX_NUM_ALF_CHROMA_COEFF);
                                cost[N_C] = cost_ctb_enable;
                                cost[N_C] += enc_alf->lambda[ch] * APS_MAX_NUM_IN_BITS;
                            }
                        }
                    }
                    else
                    {
                        if (channel == CHROMA_CH)
                        {
                            if (ch == U_C)
                            {
                                xeve_alf_set_ctb_enable_flag(enc_alf, alf->ctu_enable_flag, U_C, TRUE);
                                xeve_alf_get_frame_stats(enc_alf, U_C, 0);
                                cost[ch] = xeve_alf_get_unfiltered_dist_ch(enc_alf->alf_cov_frame[U_C][0], channel);
                                unfilterd_cost_cb = cost[ch];
                            }
                            else if (ch == V_C)
                            {
                                xeve_alf_set_ctb_enable_flag(enc_alf, alf->ctu_enable_flag, V_C, TRUE);
                                xeve_alf_get_frame_stats(enc_alf, V_C, 0);
                                cost[ch] = xeve_alf_get_unfiltered_dist_ch(enc_alf->alf_cov_frame[V_C][0], channel);
                                unfilterd_cost_cr = cost[ch];
                            }
                        }
                        else
                        {
                            xeve_trace("Error: temporal ALF checked, but enableFlag for luma is OFF\n");
                        }
                    }
                }
                if (ch == Y_C)
                {
                    BOOL is_curr_better_local = cost[ch] < cost_best[ch];
                    if (is_curr_better_local)
                    {
                        talf_comp_enable[ch] = 1;
                        cost_best[ch] = cost[ch];
                        prev_idx_comp[ch] = buf_idx;
                        xeve_alf_copy_ctb_enable_flag(enc_alf, &enc_alf->ctu_enable_flag_temp_luma, alf->ctu_enable_flag, Y_C);
                    }
                }
                else if (ch == V_C)
                {
                    if (enc_alf->alf_slice_param_temp.enable_flag[U_C] && enc_alf->alf_slice_param_temp.enable_flag[V_C])
                    {
                        if (cost[N_C] < cost_best[U_C] + cost_best[V_C])
                        {
                            cost_best[U_C] = cost[N_C] / 2;
                            cost_best[V_C] = cost[N_C] / 2;
                            prev_idx_comp[channel] = buf_idx;
                            talf_comp_enable[U_C] = 1;
                            talf_comp_enable[V_C] = 1;
                        }
                    }
                    else if (enc_alf->alf_slice_param_temp.enable_flag[U_C])
                    {
                        if (cost[U_C] + cost[V_C] < cost_best[U_C] + cost_best[V_C])
                        {
                            cost_best[U_C] = cost[U_C];
                            cost_best[V_C] = cost[V_C];
                            prev_idx_comp[channel] = buf_idx;
                            talf_comp_enable[U_C] = 1;
                            talf_comp_enable[V_C] = 0;
                        }
                    }
                    else if (enc_alf->alf_slice_param_temp.enable_flag[V_C])
                    {
                        if (cost[U_C] + cost[V_C] < cost_best[U_C] + cost_best[V_C])
                        {
                            cost_best[U_C] = cost[U_C];
                            cost_best[V_C] = cost[V_C];
                            prev_idx_comp[channel] = buf_idx;
                            talf_comp_enable[U_C] = 0;
                            talf_comp_enable[V_C] = 1;
                        }
                    }
                    else
                    {
                        if (cost[U_C] + cost[V_C] < cost_best[U_C] + cost_best[V_C])
                        {
                            cost_best[U_C] = cost[U_C];
                            cost_best[V_C] = cost[V_C];
                            prev_idx_comp[channel] = buf_idx;
                            talf_comp_enable[U_C] = 0;
                            talf_comp_enable[V_C] = 1;
                        }
                    }
                }
            }
        }
        BOOL is_curr_better_global = (cost_best[Y_C] + cost_best[U_C] + cost_best[V_C]) < (enc_alf->cost_alf_encoder[Y_C] + enc_alf->cost_alf_encoder[U_C] + enc_alf->cost_alf_encoder[V_C]);

        if (is_curr_better_global)
        {
            if (talf_comp_enable[Y_C])
            {
                enc_alf->cost_alf_encoder[LUMA_CH] = cost_best[LUMA_CH];
                alf_copy_param(alf_slice_param, &(stored_alf_param[prev_idx_comp[LUMA_CH]]));
                alf_slice_param->prev_idx = prev_idx_comp[LUMA_CH];
                alf_slice_param->prev_idx_comp[LUMA_CH] = prev_idx_comp[LUMA_CH];
                xeve_alf_copy_ctb_enable_flag(enc_alf, enc_alf->ctu_enable_flag_temp, &enc_alf->ctu_enable_flag_temp_luma, Y_C);
                alf_slice_param->enable_flag[0] = 1;
            }
            else
            {
                alf_slice_param->enable_flag[0] = 0;
                alf_slice_param->prev_idx_comp[0] = -1;
            }
            if (talf_comp_enable[U_C] || talf_comp_enable[V_C])
            {
                enc_alf->cost_alf_encoder[U_C] = cost_best[U_C];
                enc_alf->cost_alf_encoder[V_C] = cost_best[V_C];
                alf_param_chroma(alf_slice_param, &(stored_alf_param[prev_idx_comp[CHROMA_CH]]));
                alf_slice_param->prev_idx_comp[CHROMA_CH] = prev_idx_comp[CHROMA_CH];
                xeve_alf_set_ctb_enable_flag(enc_alf, enc_alf->ctu_enable_flag_temp, U_C, stored_alf_param[prev_idx_comp[CHROMA_CH]].enable_flag[1]);
                xeve_alf_set_ctb_enable_flag(enc_alf, enc_alf->ctu_enable_flag_temp, V_C, stored_alf_param[prev_idx_comp[CHROMA_CH]].enable_flag[2]);
                alf_slice_param->enable_flag[1] = (stored_alf_param[prev_idx_comp[CHROMA_CH]].enable_flag[1]);
                alf_slice_param->enable_flag[2] = (stored_alf_param[prev_idx_comp[CHROMA_CH]].enable_flag[2]);
            }
            else
            {
                alf_slice_param->enable_flag[1] = 0;
                alf_slice_param->enable_flag[2] = 0;
                alf_slice_param->prev_idx_comp[1] = -1;
            }
            alf_slice_param->temporal_alf_flag = TRUE;
            alf_slice_param->chroma_ctb_present_flag = FALSE;
        }
        else
        {
            alf_slice_param->temporal_alf_flag = FALSE;
            alf_slice_param->prev_idx_comp[0] = -1;
            alf_slice_param->prev_idx_comp[1] = -1;
        }
    }

    xeve_alf_copy_ctb_enable_flag(enc_alf, alf->ctu_enable_flag, enc_alf->ctu_enable_flag_temp, Y_C);
    xeve_alf_copy_ctb_enable_flag(enc_alf, alf->ctu_enable_flag, enc_alf->ctu_enable_flag_temp, U_C);
    xeve_alf_copy_ctb_enable_flag(enc_alf, alf->ctu_enable_flag, enc_alf->ctu_enable_flag_temp, V_C);
}

void xeve_alf_derive_cov_from_ltap_filter(ALF_COVARIANCE* cov_large, ALF_COVARIANCE* cov_small, int* pattern_small, ALF_FILTER_TYPE luma_filter_type)
{
    cov_small->pix_acc = cov_large->pix_acc;
    for (int i = 0; i < (luma_filter_type ? 13 : 7); i++)
    {
        if (pattern_small[i] > 0)
        {
            cov_small->y[pattern_small[i] - 1] = cov_large->y[i];
            for (int j = 0; j < (luma_filter_type ? 13 : 7); j++)
            {
                if (pattern_small[j] > 0)
                {
                    cov_small->E[pattern_small[i] - 1][pattern_small[j] - 1] = cov_large->E[i][j];
                }
            }
        }
    }
}

void xeve_alf_copy_slice_param(XEVE_ALF * enc_alf, ALF_SLICE_PARAM* alf_slice_param_dst, ALF_SLICE_PARAM* alf_slice_param_src, int channel)
{
    ADAPTIVE_LOOP_FILTER * alf = &enc_alf->alf;
    if (channel == LUMA_CH)
    {
        u8* temp = alf_slice_param_dst->alf_ctb_flag;
        xeve_mcpy(alf_slice_param_dst, alf_slice_param_src, sizeof(ALF_SLICE_PARAM));
        alf_slice_param_dst->alf_ctb_flag = temp;
        xeve_mcpy(alf_slice_param_dst->alf_ctb_flag, alf_slice_param_src->alf_ctb_flag, alf->num_ctu_in_pic * sizeof(u8));
    }
    else
    {
        alf_slice_param_dst->enable_flag[U_C] = alf_slice_param_src->enable_flag[U_C];
        alf_slice_param_dst->enable_flag[V_C] = alf_slice_param_src->enable_flag[V_C];
        alf_slice_param_dst->chroma_ctb_present_flag = alf_slice_param_src->chroma_ctb_present_flag;
        xeve_mcpy(alf_slice_param_dst->chroma_coef, alf_slice_param_src->chroma_coef, sizeof(short)*MAX_NUM_ALF_CHROMA_COEFF);
    }
}

double xeve_alf_get_filter_coef_cost(XEVE_ALF * enc_alf, CODING_STRUCTURE * cs, double dist_unfilter, u8 comp_id, BOOL is_re_collect_stat
    , int input_shape_idx, int* input_coef_bits, u8* filter_conformance_flag)
{
    u8 channel = comp_id > Y_C ? 1 : 0;
    u8 is_luma = comp_id == Y_C ? 1 : 0;
    u8 is_chroma = !is_luma;
    const int size = comp_id == Y_C ? 2 : 1;

    if (is_re_collect_stat)
    {
        xeve_alf_get_frame_stats(enc_alf, comp_id, input_shape_idx);
    }

    double dist = dist_unfilter;
    *input_coef_bits = 0;
    int uiSliceFlag = 0;
    ALF_FILTER_SHAPE alf_filter_shape = enc_alf->alf_slice_param_temp.filterShapes[channel][input_shape_idx];

    if (comp_id == Y_C)
    {
        dist += xeve_alf_merge_filters_cost(enc_alf, &enc_alf->alf_slice_param_temp, &alf_filter_shape, enc_alf->alf_cov_frame[channel][input_shape_idx]
                                  , enc_alf->alf_cov_merged[input_shape_idx], input_coef_bits, filter_conformance_flag);
    }
    else if (comp_id == U_C || comp_id == V_C)
    {
        dist += enc_alf->alf_cov_frame[comp_id][input_shape_idx][0].pix_acc +
                xeve_alf_derive_coef_quant(enc_alf->filter_coef_quant, enc_alf->alf_cov_frame[comp_id][input_shape_idx][0].E
                              , enc_alf->alf_cov_frame[comp_id][input_shape_idx][0].y, alf_filter_shape.num_coef, alf_filter_shape.weights, NUM_BITS, TRUE);
        xeve_mcpy(enc_alf->filter_coef_set[0], enc_alf->filter_coef_quant, sizeof(*enc_alf->filter_coef_quant) * alf_filter_shape.num_coef);
        const int alf_chroma_idc = enc_alf->alf_slice_param_temp.enable_flag[U_C] * 2 + enc_alf->alf_slice_param_temp.enable_flag[V_C];
        for (int i = 0; i < MAX_NUM_ALF_CHROMA_COEFF; i++)
        {
            enc_alf->alf_slice_param_temp.chroma_coef[i] = enc_alf->filter_coef_quant[i];
        }
        *input_coef_bits += xeve_alf_get_coef_rate(enc_alf, &enc_alf->alf_slice_param_temp, TRUE);
        uiSliceFlag = xeve_alf_lenth_truncated_unary(alf_chroma_idc, 3);
    }

    double rate = *input_coef_bits + uiSliceFlag;
    if (is_luma || (!enc_alf->alf_slice_param_temp.chroma_ctb_present_flag))
    {
        if (is_luma)
        {
            CHECK(enc_alf->alf_slice_param_temp.chroma_ctb_present_flag, "chromaCTB is on");
        }
        else
        {
            CHECK(!enc_alf->alf_slice_param_temp.enable_flag[Y_C], "Slice Y is off");
        }
    }

    if (comp_id == Y_C)
    {
        return dist + enc_alf->lambda[Y_C] * rate;
    }
    else if (comp_id == U_C)
    {
        return dist + enc_alf->lambda[U_C] * rate;
    }
    else
    {
        return dist + enc_alf->lambda[V_C] * rate;
    }
}

void xeve_alf_get_filter_coef_cost_ch(XEVE_ALF * enc_alf, CODING_STRUCTURE * cs, double dist_unfilter, u8 comp_id, int input_shape_idx, int* input_coef_bits, double* filter_cost)
{
    u8 channel = comp_id == Y_C ? LUMA_CH : CHROMA_CH;
    ALF_FILTER_SHAPE alf_filter_shape = enc_alf->alf_slice_param_temp.filterShapes[channel][input_shape_idx];
    double dist = 0;
    int slice_flag = 0;
    *input_coef_bits = 0;
    double rate = 0;
    int alf_chroma_idc = 0;
    alf_cov_reset(&enc_alf->alf_cov_frame[N_C][input_shape_idx][0]);
    alf_cov_add(&enc_alf->alf_cov_frame[N_C][input_shape_idx][0], &enc_alf->alf_cov_frame[1][input_shape_idx][0]);
    alf_cov_add(&enc_alf->alf_cov_frame[N_C][input_shape_idx][0], &enc_alf->alf_cov_frame[2][input_shape_idx][0]);
    dist += enc_alf->alf_cov_frame[N_C][input_shape_idx][0].pix_acc +
            xeve_alf_derive_coef_quant(enc_alf->filter_coef_quant, enc_alf->alf_cov_frame[N_C][0][0].E
                           , enc_alf->alf_cov_frame[N_C][0][0].y, alf_filter_shape.num_coef, alf_filter_shape.weights, NUM_BITS, TRUE);
    xeve_mcpy(enc_alf->filter_coef_set[0], enc_alf->filter_coef_quant, sizeof(*enc_alf->filter_coef_quant) * alf_filter_shape.num_coef);
    u8 filter_conformance_flag = 0;
    int sum = 0;
    int factor = (1 << (NUM_BITS - 1));
    for (int i = 0; i < MAX_NUM_ALF_CHROMA_COEFF - 1; i++)
    {
        enc_alf->alf_slice_param_temp.chroma_coef[i] = enc_alf->filter_coef_quant[i];
        if (enc_alf->filter_coef_quant[i] < -(1 << 9) || enc_alf->filter_coef_quant[i] > (1 << 9) - 1)
        {
            filter_conformance_flag = 1;
        }
        sum += enc_alf->alf_slice_param_temp.chroma_coef[i] << 1;
    }
    int last_coeff = factor - sum;
    if (last_coeff < -(1 << 10) || last_coeff >(1 << 10) - 1)
    {
        filter_conformance_flag = 1;
    }
    *input_coef_bits += xeve_alf_get_coef_rate(enc_alf, &enc_alf->alf_slice_param_temp, TRUE);
    alf_chroma_idc = 3;
    slice_flag = xeve_alf_lenth_truncated_unary(alf_chroma_idc, 3);
    rate = *input_coef_bits + slice_flag;
    filter_cost[2] = dist + enc_alf->lambda[U_C] * rate;
    dist = enc_alf->alf_cov_frame[U_C][input_shape_idx][0].pix_acc +
           xeve_alf_calc_err_coef(enc_alf->alf_cov_frame[U_C][input_shape_idx][0].E
                            , enc_alf->alf_cov_frame[U_C][input_shape_idx][0].y, enc_alf->filter_coef_quant, 7, 10);
    alf_chroma_idc = 2;
    slice_flag = xeve_alf_lenth_truncated_unary(alf_chroma_idc, 3);
    rate = *input_coef_bits + slice_flag;
    filter_cost[0] = dist + enc_alf->lambda[U_C] * rate;
    dist = enc_alf->alf_cov_frame[V_C][input_shape_idx][0].pix_acc +
           xeve_alf_calc_err_coef(enc_alf->alf_cov_frame[V_C][input_shape_idx][0].E
                            , enc_alf->alf_cov_frame[V_C][input_shape_idx][0].y, enc_alf->filter_coef_quant, 7, 10);
    alf_chroma_idc = 1;
    slice_flag = xeve_alf_lenth_truncated_unary(alf_chroma_idc, 3);
    rate = *input_coef_bits + slice_flag;
    filter_cost[1] = dist + enc_alf->lambda[V_C] * rate;
    if (filter_conformance_flag)
    {
        filter_cost[0] = filter_cost[1] = filter_cost[2] = MAX_COST;
    }
}

int xeve_alf_get_coef_rate(XEVE_ALF * enc_alf, ALF_SLICE_PARAM* alf_slice_param, BOOL is_chroma)
{
    int bits = 0;
    if (!is_chroma)
    {
        bits++;                                               // alf_coefficients_delta_flag
        if (!alf_slice_param->coef_delta_flag)
        {
            if (alf_slice_param->num_luma_filters > 1)
            {
                bits++;                                           // coeff_delta_pred_mode_flag
            }
        }
    }

    xeve_mset(enc_alf->bits_coef_scan, 0, sizeof(enc_alf->bits_coef_scan));
    ALF_FILTER_SHAPE alf_shape;
    alf_init_filter_shape(&alf_shape, is_chroma ? 5 : (alf_slice_param->luma_filter_type == ALF_FILTER_5 ? 5 : 7));
    const int max_golomb_idx = alf_get_max_golomb_idx((ALF_FILTER_TYPE)alf_shape.filter_type);
    const short* coeff = is_chroma ? alf_slice_param->chroma_coef : alf_slice_param->luma_coef;
    const int num_filters = is_chroma ? 1 : alf_slice_param->num_luma_filters;

    // vlc for all
    for (int ind = 0; ind < num_filters; ++ind)
    {
        if (is_chroma || !alf_slice_param->coef_delta_flag || alf_slice_param->filter_coef_flag[ind])
        {
            for (int i = 0; i < alf_shape.num_coef - 1; i++)
            {
                int coef_val = abs(coeff[ind * MAX_NUM_ALF_LUMA_COEFF + i]);

                for (int k = 1; k < 15; k++)
                {
                    enc_alf->bits_coef_scan[alf_shape.golombIdx[i]][k] += xeve_alf_length_golomb(coef_val, k, TRUE);
                }
            }
        }
    }

    int k_min = xeve_alf_get_golomb_k_min(&alf_shape, num_filters, enc_alf->k_min_tab, enc_alf->bits_coef_scan);

    // Golomb parameters
    bits += xeve_alf_lenght_uvlc(k_min - 1);  // "min_golomb_order"
    int golomb_order_inc_flag = 0;

    for (int idx = 0; idx < max_golomb_idx; idx++)
    {
        golomb_order_inc_flag = (enc_alf->k_min_tab[idx] != k_min) ? 1 : 0;
        CHECK(!(enc_alf->k_min_tab[idx] <= k_min + 1), "ALF Golomb parameter not consistent");
        bits += golomb_order_inc_flag;                           //golomb_order_increase_flag
        k_min = enc_alf->k_min_tab[idx];
    }

    if (!is_chroma)
    {
        if (alf_slice_param->coef_delta_flag)
        {
            bits += num_filters;             //filter_coefficient_flag[i]
        }
    }

    // Filter coefficients
    for (int ind = 0; ind < num_filters; ++ind)
    {
        if (!is_chroma && !alf_slice_param->filter_coef_flag[ind] && alf_slice_param->coef_delta_flag)
        {
            continue;
        }

        for (int i = 0; i < alf_shape.num_coef - 1; i++)
        {
            bits += xeve_alf_length_golomb(coeff[ind* MAX_NUM_ALF_LUMA_COEFF + i], enc_alf->k_min_tab[alf_shape.golombIdx[i]], TRUE);  // alf_coeff_chroma[i], alf_coeff_luma_delta[i][j]
        }
    }
    return bits;
}

double xeve_alf_get_unfiltered_dist_ch(ALF_COVARIANCE* cov, int channel)
{
    double dist = 0;
    if (channel == LUMA_CH)
    {
        dist = xeve_alf_get_unfiltered_dist(cov, MAX_NUM_ALF_CLASSES);
    }
    else
    {
        dist = xeve_alf_get_unfiltered_dist(cov, 1);
    }
    return dist;
}

double xeve_alf_get_unfiltered_dist(ALF_COVARIANCE* cov, const int num_classes)
{
    double dist = 0;

    for (int class_idx = 0; class_idx < num_classes; class_idx++)
    {
        dist += cov[class_idx].pix_acc;
    }

    return dist;
}


double xeve_alf_get_filtered_dist(XEVE_ALF * enc_alf, ALF_COVARIANCE* cov, const int num_classes, const int num_filters_minus1, const int num_coef)
{
  double dist = 0;

  for (int class_idx = 0; class_idx < num_classes; class_idx++)
  {

    dist += xeve_alf_calc_err_coef(cov[class_idx].E, cov[class_idx].y, enc_alf->filter_coef_set[class_idx], num_coef, NUM_BITS);
  }

  return dist;
}
void xeve_alf_conformance_check(XEVE_ALF * enc_alf, ALF_SLICE_PARAM* alf_slice_param, u8* filter_conformance_flag)
{
    ADAPTIVE_LOOP_FILTER * alf = &enc_alf->alf;
    int factor = (1 << (NUM_BITS - 1));
    int num_coef = alf_slice_param->luma_filter_type == ALF_FILTER_5 ? 7 : 13;
    short luma_coef[MAX_NUM_ALF_CLASSES * MAX_NUM_ALF_LUMA_COEFF] = { 0, };
    short coef_rec[MAX_NUM_ALF_CLASSES * MAX_NUM_ALF_LUMA_COEFF] = { 0, };
    int num_filters = alf_slice_param->num_luma_filters;
    if (alf_slice_param->coef_delta_pred_mode_flag)
    {
        for (int j = 0; j < num_coef - 1; j++)
        {
            luma_coef[j] = alf_slice_param->luma_coef[j];
        }
        for (int i = 1; i < num_filters; i++)
        {
            for (int j = 0; j < num_coef - 1; j++)
            {
                luma_coef[i * MAX_NUM_ALF_LUMA_COEFF + j] = alf_slice_param->luma_coef[i * MAX_NUM_ALF_LUMA_COEFF + j] + alf_slice_param->luma_coef[(i - 1) * MAX_NUM_ALF_LUMA_COEFF + j];
            }
        }
    }
    else
    {
        for (int j = 0; j < MAX_NUM_ALF_CLASSES * MAX_NUM_ALF_LUMA_COEFF; j++)
        {
            luma_coef[j] = alf_slice_param->luma_coef[j];
        }
    }
    int num_coef_large_minus1 = MAX_NUM_ALF_LUMA_COEFF - 1;
    for (int class_idx = 0; class_idx < MAX_NUM_ALF_CLASSES; class_idx++)
    {
        int filter_idx = alf_slice_param->filter_coef_delta_idx[class_idx];
        int fixed_filter_idx = alf_slice_param->fixed_filter_idx[class_idx];
        u8  fixed_filter_usage_flag = alf_slice_param->fixed_filter_usage_flag[class_idx];
        int fixed_filter_used = fixed_filter_usage_flag;
        int fixed_filter_map_idx = fixed_filter_idx;
        if (fixed_filter_used)
        {
            fixed_filter_idx = alf_class_to_filter_mapping[class_idx][fixed_filter_map_idx];
        }
        int sum = 0;
        for (int i = 0; i < num_coef_large_minus1; i++)
        {
            int cur_coef = 0;
            //fixed filter
            if (fixed_filter_usage_flag > 0)
            {
                cur_coef = alf_fixed_filter_coef[fixed_filter_idx][i];
            }
            //add coded coeff
            if (alf->filter_shapes[LUMA_CH][alf_slice_param->luma_filter_type].pattern_to_large_filter[i] > 0)
            {
                int coeffIdx = alf->filter_shapes[LUMA_CH][alf_slice_param->luma_filter_type].pattern_to_large_filter[i] - 1;
                cur_coef += luma_coef[filter_idx * MAX_NUM_ALF_LUMA_COEFF + coeffIdx];
            }
            coef_rec[class_idx* MAX_NUM_ALF_LUMA_COEFF + i] = cur_coef;
            if (coef_rec[class_idx* MAX_NUM_ALF_LUMA_COEFF + i] < -(1 << 9) || coef_rec[class_idx* MAX_NUM_ALF_LUMA_COEFF + i] > (1 << 9) - 1)
            {
                *filter_conformance_flag = 1;
                break;
            }
            sum += (coef_rec[class_idx* MAX_NUM_ALF_LUMA_COEFF + i] << 1);
        }
        if (*filter_conformance_flag)
            break;
        int last_coeff = factor - sum;
        if (last_coeff < -(1 << 10) || last_coeff >(1 << 10) - 1)
        {
            *filter_conformance_flag = 1;
            break;
        }
    }
}

double xeve_alf_merge_filters_cost(XEVE_ALF * enc_alf, ALF_SLICE_PARAM* alf_slice_param, ALF_FILTER_SHAPE* alf_shape, ALF_COVARIANCE* cov_frame
                                  , ALF_COVARIANCE* cov_merged, int* input_coef_bits, u8* filter_conformance_flag)
{
    int num_filter_best = 0;
    int num_filters = MAX_NUM_ALF_CLASSES;
    static BOOL coded_var_bins[MAX_NUM_ALF_CLASSES];
    static double err_force0_coef_tab[MAX_NUM_ALF_CLASSES][2];

    double cost, cost0, dist, dist_force0, cost_min = DBL_MAX;
    int pred_mode = 0, best_pred_mode = 0, coef_bits, coef_bits_force0;

    xeve_alf_find_best_fixed_filter(alf_slice_param, cov_frame);

    if (alf_shape->filter_type == ALF_FILTER_5)
    {
        for (int class_idx = 0; class_idx < MAX_NUM_ALF_CLASSES; class_idx++)
        {
            xeve_alf_derive_cov_from_ltap_filter(&cov_frame[class_idx], &enc_alf->alf_cov_frame[LUMA_CH][ALF_FILTER_5][class_idx]
                                     , alf_shape->pattern_to_large_filter, alf_slice_param->luma_filter_type);
        }
        cov_frame = enc_alf->alf_cov_frame[LUMA_CH][ALF_FILTER_5];
    }
    xeve_alf_merge_classes(cov_frame, cov_merged, MAX_NUM_ALF_CLASSES, enc_alf->filter_indices);

    while (num_filters >= 1)
    {
        dist = xeve_alf_derive_filter_coef(enc_alf, cov_frame, cov_merged, alf_shape, enc_alf->filter_indices[num_filters - 1], num_filters, err_force0_coef_tab);
        dist_force0 = xeve_alf_get_dist_force0(enc_alf, alf_shape, num_filters, err_force0_coef_tab, coded_var_bins);
        coef_bits = xeve_alf_derive_filter_coef_pred_mode(enc_alf, alf_shape, enc_alf->filter_coef_set, enc_alf->dif_filter_coef, num_filters, &pred_mode);
        coef_bits_force0 = xeve_alf_get_cost_filter_coef_force0(enc_alf, alf_shape, enc_alf->filter_coef_set, num_filters, coded_var_bins);
        cost = dist + enc_alf->lambda[Y_C] * coef_bits;
        cost0 = dist_force0 + enc_alf->lambda[Y_C] * coef_bits_force0;

        if (cost0 < cost)
        {
            cost = cost0;
        }

        if (cost <= cost_min)
        {
            cost_min = cost;
            num_filter_best = num_filters;
            best_pred_mode = pred_mode;
        }
        num_filters--;
    }

    dist = xeve_alf_derive_filter_coef(enc_alf, cov_frame, cov_merged, alf_shape, enc_alf->filter_indices[num_filter_best - 1], num_filter_best, err_force0_coef_tab);

    coef_bits = xeve_alf_derive_filter_coef_pred_mode(enc_alf, alf_shape, enc_alf->filter_coef_set, enc_alf->dif_filter_coef, num_filter_best, &pred_mode);
    dist_force0 = xeve_alf_get_dist_force0(enc_alf, alf_shape, num_filter_best, err_force0_coef_tab, coded_var_bins);
    coef_bits_force0 = xeve_alf_get_cost_filter_coef_force0(enc_alf, alf_shape, enc_alf->filter_coef_set, num_filter_best, coded_var_bins);
    cost = dist + enc_alf->lambda[Y_C] * coef_bits;
    cost0 = dist_force0 + enc_alf->lambda[Y_C] * coef_bits_force0;

    alf_slice_param->num_luma_filters = num_filter_best;

    double dist_return;
    if (cost <= cost0)
    {
        dist_return = dist;
        alf_slice_param->coef_delta_flag = 0;
        *input_coef_bits = coef_bits;
        alf_slice_param->coef_delta_pred_mode_flag = best_pred_mode;
    }
    else
    {
        dist_return = dist_force0;
        alf_slice_param->coef_delta_flag = 1;
        *input_coef_bits = coef_bits_force0;
        for (int i = 0; i < MAX_NUM_ALF_CLASSES; i++)
            alf_slice_param->filter_coef_flag[i] = (BOOL)coded_var_bins[i];
        alf_slice_param->coef_delta_pred_mode_flag = 0;

        for (int varInd = 0; varInd < num_filter_best; varInd++)
        {
            if (coded_var_bins[varInd] == 0)
            {
                xeve_mset(enc_alf->filter_coef_set[varInd], 0, sizeof(int)*MAX_NUM_ALF_LUMA_COEFF);
            }
        }
    }
    for (int ind = 0; ind < alf_slice_param->num_luma_filters; ++ind)
    {
        for (int i = 0; i < alf_shape->num_coef; i++)
        {
            if (alf_slice_param->coef_delta_pred_mode_flag)
            {
                alf_slice_param->luma_coef[ind * MAX_NUM_ALF_LUMA_COEFF + i] = enc_alf->dif_filter_coef[ind][i];
            }
            else
            {
                alf_slice_param->luma_coef[ind * MAX_NUM_ALF_LUMA_COEFF + i] = enc_alf->filter_coef_set[ind][i];
            }
        }
    }

    xeve_mcpy(alf_slice_param->filter_coef_delta_idx, enc_alf->filter_indices[num_filter_best - 1], sizeof(short) * MAX_NUM_ALF_CLASSES);
    const int num_fixed_filter_per_class = ALF_FIXED_FILTER_NUM;
    if (num_fixed_filter_per_class > 0)
    {
        int fixed_filter_pattern = alf_slice_param->fixed_filter_usage_flag[0] ? 1 : 0;

        for (int class_idx = 1; class_idx < MAX_NUM_ALF_CLASSES; class_idx++)
        {
            int curr_fixed_filter_pattern = alf_slice_param->fixed_filter_usage_flag[class_idx] ? 1 : 0;

            if (curr_fixed_filter_pattern != fixed_filter_pattern)
            {
                fixed_filter_pattern = 2;
                break;
            }
        }
        alf_slice_param->fixed_filter_pattern = fixed_filter_pattern;
    }
    *filter_conformance_flag = 0;
    xeve_alf_conformance_check(enc_alf, alf_slice_param, filter_conformance_flag);
    *input_coef_bits = *input_coef_bits + xeve_alf_get_non_filter_coef_rate(alf_slice_param);
    if (*filter_conformance_flag)
    {
        dist_return = MAX_COST;
    }
    return dist_return;
}

int xeve_alf_get_non_filter_coef_rate(ALF_SLICE_PARAM* alf_slice_param)
{
    int len = 1   // filter_type
        + 1   // alf_coefficients_delta_flag
        + xeve_alf_lenth_truncated_unary(0, 3)    // chroma_idc = 0, it is signalled when ALF is enabled for luma
        + 5;   //num_luma_filters

    const int num_fixed_filter_per_class = ALF_FIXED_FILTER_NUM;
    if (num_fixed_filter_per_class > 0)
    {
        len += xeve_alf_length_golomb(alf_slice_param->fixed_filter_pattern, 0, FALSE);
        if (alf_slice_param->fixed_filter_pattern == 2)
        {
            len += MAX_NUM_ALF_CLASSES;  //"fixed_filter_flag" for each class
        }
        if (alf_slice_param->fixed_filter_pattern > 0 && num_fixed_filter_per_class > 1)
        {
            for (int class_idx = 0; class_idx < MAX_NUM_ALF_CLASSES; class_idx++)
            {
                if (alf_slice_param->fixed_filter_usage_flag[class_idx] > 0)
                {
                    len += xeve_tbl_log2[num_fixed_filter_per_class - 1] + 1;
                }
            }
        }
    }

    if (alf_slice_param->num_luma_filters > 1)
    {
        for (int i = 0; i < MAX_NUM_ALF_CLASSES; i++)
        {
            len += xeve_tbl_log2[alf_slice_param->num_luma_filters - 1] + 1;
        }
    }
    return len;
}

int xeve_alf_lenth_truncated_unary(int symbol, int max_symbol)
{
    if (max_symbol == 0)
    {
        return 0;
    }

    BOOL code_last = (max_symbol > symbol);
    int bins = 0;
    int num_bins = 0;
    while (symbol--)
    {
        bins <<= 1;
        bins++;
        num_bins++;
    }
    if (code_last)
    {
        bins <<= 1;
        num_bins++;
    }

    return num_bins;
}


int xeve_alf_get_cost_filter_coef_force0(XEVE_ALF * enc_alf, ALF_FILTER_SHAPE* alf_shape, int **diff_q_filter_coef, const int num_filters, BOOL* coded_var_bins)
{
    const int max_golomb_idx = alf_get_max_golomb_idx((ALF_FILTER_TYPE)alf_shape->filter_type);
    xeve_mset(enc_alf->bits_coef_scan, 0, sizeof(enc_alf->bits_coef_scan));

    for (int ind = 0; ind < num_filters; ++ind)
    {
        if (!coded_var_bins[ind])
        {
            continue;
        }
        for (int i = 0; i < alf_shape->num_coef - 1; i++)
        {
            int coef_val = abs(diff_q_filter_coef[ind][i]);
            for (int k = 1; k < 15; k++)
            {
                enc_alf->bits_coef_scan[alf_shape->golombIdx[i]][k] += xeve_alf_length_golomb(coef_val, k, TRUE);
            }
        }
    }

    int k_min = xeve_alf_get_golomb_k_min(alf_shape, num_filters, enc_alf->k_min_tab, enc_alf->bits_coef_scan);

    // Coding parameters
    int len = k_min           // min_golomb_order
            + max_golomb_idx  // golomb_order_increase_flag
            + num_filters;    // filter_coefficient_flag[i]
                              // Filter coefficients
    for (int ind = 0; ind < num_filters; ++ind)
    {
        if (coded_var_bins[ind])
        {
            for (int i = 0; i < alf_shape->num_coef - 1; i++)
            {
                len += xeve_alf_length_golomb(abs(diff_q_filter_coef[ind][i]), enc_alf->k_min_tab[alf_shape->golombIdx[i]], TRUE); // alf_coeff_luma_delta[i][j]
            }
        }
    }

    return len;
}

int xeve_alf_derive_filter_coef_pred_mode(XEVE_ALF * enc_alf, ALF_FILTER_SHAPE* alf_shape, int **filter_set, int** filterCoeffDiff, const int num_filters, int* predMode)
{
    int rate_pred_mode0 = xeve_alf_get_cost_filter_coef(enc_alf, alf_shape, filter_set, num_filters);

    for (int ind = 0; ind < num_filters; ++ind)
    {
        if (ind == 0)
        {
            xeve_mcpy(filterCoeffDiff[ind], filter_set[ind], sizeof(int) * alf_shape->num_coef);
        }
        else
        {
            for (int i = 0; i < alf_shape->num_coef; i++)
            {
                filterCoeffDiff[ind][i] = filter_set[ind][i] - filter_set[ind - 1][i];
            }
        }
    }

    int rate_pred_mode1 = xeve_alf_get_cost_filter_coef(enc_alf, alf_shape, filterCoeffDiff, num_filters);

    *predMode = (rate_pred_mode1 < rate_pred_mode0 && num_filters > 1) ? 1 : 0;

    return (num_filters > 1 ? 1 : 0)        // coeff_delta_pred_mode_flag
         + (*predMode ? rate_pred_mode1 : rate_pred_mode0); // min_golomb_order, golomb_order_increase_flag, alf_coeff_luma_delta
}

int xeve_alf_get_cost_filter_coef(XEVE_ALF * enc_alf, ALF_FILTER_SHAPE* alf_shape, int **diff_q_filter_coef, const int num_filters)
{
    const int max_golomb_idx = alf_get_max_golomb_idx((ALF_FILTER_TYPE)alf_shape->filter_type);

    xeve_mset(enc_alf->bits_coef_scan, 0, sizeof(enc_alf->bits_coef_scan));

    for (int ind = 0; ind < num_filters; ++ind)
    {
        for (int i = 0; i < alf_shape->num_coef - 1; i++)
        {
            int coef_val = abs(diff_q_filter_coef[ind][i]);
            for (int k = 1; k < 15; k++)
            {
                enc_alf->bits_coef_scan[alf_shape->golombIdx[i]][k] += xeve_alf_length_golomb(coef_val, k, TRUE);
            }
        }
    }

    int k_min = xeve_alf_get_golomb_k_min(alf_shape, num_filters, enc_alf->k_min_tab, enc_alf->bits_coef_scan);

    int len = k_min           //min_golomb_order
        + max_golomb_idx;  //golomb_order_increase_flag

    len += xeve_alf_length_filter_coef(alf_shape, num_filters, diff_q_filter_coef, enc_alf->k_min_tab);  // alf_coeff_luma_delta[i][j]

    return len;
}

int xeve_alf_length_filter_coef(ALF_FILTER_SHAPE* alf_shape, const int num_filters, int **filter_coef, int* k_min_tab)
{
  int bit_cnt = 0;

  for (int ind = 0; ind < num_filters; ++ind)
  {
    for (int i = 0; i < alf_shape->num_coef - 1; i++)
    {
      bit_cnt += xeve_alf_length_golomb(abs(filter_coef[ind][i]), k_min_tab[alf_shape->golombIdx[i]], TRUE);
    }
  }
  return bit_cnt;
}

double xeve_alf_get_dist_force0(XEVE_ALF * enc_alf, ALF_FILTER_SHAPE* alf_shape, const int num_filters, double err_tab_force0_coef[MAX_NUM_ALF_CLASSES][2], BOOL* coded_var_bins)
{
    static int bits_var_bin[MAX_NUM_ALF_CLASSES];

    xeve_mset(enc_alf->bits_coef_scan, 0, sizeof(enc_alf->bits_coef_scan));
    for (int ind = 0; ind < num_filters; ++ind)
    {
        for (int i = 0; i < alf_shape->num_coef - 1; i++)
        {
            int coef_val = abs(enc_alf->filter_coef_set[ind][i]);
            for (int k = 1; k < 15; k++)
            {
                enc_alf->bits_coef_scan[alf_shape->golombIdx[i]][k] += xeve_alf_length_golomb(coef_val, k, TRUE);
            }
        }
    }

    xeve_alf_get_golomb_k_min(alf_shape, num_filters, enc_alf->k_min_tab, enc_alf->bits_coef_scan);

    for (int ind = 0; ind < num_filters; ++ind)
    {
        bits_var_bin[ind] = 0;
        for (int i = 0; i < alf_shape->num_coef - 1; i++)
        {
            bits_var_bin[ind] += xeve_alf_length_golomb(abs(enc_alf->filter_coef_set[ind][i]), enc_alf->k_min_tab[alf_shape->golombIdx[i]], TRUE);
        }
    }

    double dist_force0 = xeve_alf_get_dist_coef_force0(enc_alf, coded_var_bins, err_tab_force0_coef, bits_var_bin, num_filters);

    return dist_force0;
}

int xeve_alf_get_golomb_k_min(ALF_FILTER_SHAPE* alf_shape, const int num_filters, int k_min_tab[MAX_NUM_ALF_LUMA_COEFF], int bits_coef_scan[MAX_SCAN_VAL][MAX_EXP_GOLOMB])
{
    int k_start;
    const int max_golomb_idx = alf_get_max_golomb_idx((ALF_FILTER_TYPE)alf_shape->filter_type);

    int min_bits_k_start = INT_MAX;
    int min_k_start = -1;

    for (int k = 1; k < 8; k++)
    {
        int bits_k_start = 0; k_start = k;
        for (int scan_pos = 0; scan_pos < max_golomb_idx; scan_pos++)
        {
            int k_min = k_start;
            int min_bits = bits_coef_scan[scan_pos][k_min];

            if (bits_coef_scan[scan_pos][k_start + 1] < min_bits)
            {
                k_min = k_start + 1;
                min_bits = bits_coef_scan[scan_pos][k_min];
            }
            k_start = k_min;
            bits_k_start += min_bits;
        }
        if (bits_k_start < min_bits_k_start)
        {
            min_bits_k_start = bits_k_start;
            min_k_start = k;
        }
    }

    k_start = min_k_start;
    for (int scan_pos = 0; scan_pos < max_golomb_idx; scan_pos++)
    {
        int k_min = k_start;
        int min_bits = bits_coef_scan[scan_pos][k_min];

        if (bits_coef_scan[scan_pos][k_start + 1] < min_bits)
        {
            k_min = k_start + 1;
            min_bits = bits_coef_scan[scan_pos][k_min];
        }

        k_min_tab[scan_pos] = k_min;
        k_start = k_min;
    }

    return min_k_start;
}

double xeve_alf_get_dist_coef_force0(XEVE_ALF * enc_alf, BOOL* coded_var_bins, double err_force0_coef_tab[MAX_NUM_ALF_CLASSES][2], int* bits_var_bin, const int num_filters)
{
    double dist_force0 = 0;
    xeve_mset(coded_var_bins, 0, sizeof(*coded_var_bins) * MAX_NUM_ALF_CLASSES);

    for (int filt_idx = 0; filt_idx < num_filters; filt_idx++)
    {
        double costDiff = err_force0_coef_tab[filt_idx][0] - (err_force0_coef_tab[filt_idx][1] + enc_alf->lambda[Y_C] * bits_var_bin[filt_idx]);
        coded_var_bins[filt_idx] = costDiff > 0 ? TRUE : FALSE;
        dist_force0 += err_force0_coef_tab[filt_idx][coded_var_bins[filt_idx] ? 1 : 0];
    }
    return dist_force0;
}

int xeve_alf_lenght_uvlc(int code)
{
    int length = 1;
    int temp = ++code;

    CHECK(!temp, "Integer overflow");

    while (1 != temp)
    {
        temp >>= 1;
        length += 2;
    }
    return (length >> 1) + ((length + 1) >> 1);
}

int xeve_alf_length_golomb(int coef_val, int k, BOOL signed_coeff)
{
    int num_bins = 0;
    unsigned int symbol = abs(coef_val);
    while (symbol >= (unsigned int)(1 << k))
    {
        num_bins++;
        symbol -= 1 << k;
        k++;
    }
    num_bins += (k + 1);
    if (signed_coeff && coef_val != 0)
    {
        num_bins++;
    }
    return num_bins;
}

double xeve_alf_derive_filter_coef(XEVE_ALF * enc_alf, ALF_COVARIANCE* cov, ALF_COVARIANCE* cov_merged, ALF_FILTER_SHAPE* alf_shape, short* filter_indices, int num_filters, double err_tab_force0_coef[MAX_NUM_ALF_CLASSES][2])
{
    double error = 0.0;
    ALF_COVARIANCE* temp_cov = &cov_merged[MAX_NUM_ALF_CLASSES];
    for (int filt_idx = 0; filt_idx < num_filters; filt_idx++)
    {
        alf_cov_reset(temp_cov);
        for (int class_idx = 0; class_idx < MAX_NUM_ALF_CLASSES; class_idx++)
        {
            if (filter_indices[class_idx] == filt_idx)
            {
                alf_cov_add(temp_cov, &cov[class_idx]);
            }
        }

        // Find coeffcients
        err_tab_force0_coef[filt_idx][1] = temp_cov->pix_acc + xeve_alf_derive_coef_quant(enc_alf->filter_coef_quant, temp_cov->E, temp_cov->y, alf_shape->num_coef, alf_shape->weights, NUM_BITS, FALSE);
        err_tab_force0_coef[filt_idx][0] = temp_cov->pix_acc;
        error += err_tab_force0_coef[filt_idx][1];

        // store coeff
        xeve_mcpy(enc_alf->filter_coef_set[filt_idx], enc_alf->filter_coef_quant, sizeof(int)*alf_shape->num_coef);
    }
    return error;
}

double xeve_alf_derive_coef_quant(int *filter_coef_quant, double **E, double *y, const int num_coef, int* weights, const int bit_depth, const BOOL is_chroma)
{
    const int factor = 1 << (bit_depth - 1);
    static int filter_coef_quant_mod[MAX_NUM_ALF_LUMA_COEFF];
    static double filter_coef[MAX_NUM_ALF_LUMA_COEFF];

    xeve_alf_gns_solve_chol(E, y, filter_coef, num_coef);
    xeve_alf_round_filt_coef(filter_coef_quant, filter_coef, num_coef, factor);
    const int target_coef_sum_int = 0;
    int quant_coef_sum = 0;
    for (int i = 0; i < num_coef; i++)
    {
        quant_coef_sum += weights[i] * filter_coef_quant[i];
    }

    int count = 0;
    while (quant_coef_sum != target_coef_sum_int && count < 10)
    {
        int sign = quant_coef_sum > target_coef_sum_int ? 1 : -1;
        int diff = (quant_coef_sum - target_coef_sum_int) * sign;

        double err_min = DBL_MAX;
        int min_ind = -1;

        for (int k = 0; k < num_coef; k++)
        {
            if (weights[k] <= diff)
            {
                xeve_mcpy(filter_coef_quant_mod, filter_coef_quant, sizeof(int) * num_coef);

                filter_coef_quant_mod[k] -= sign;
                double error = xeve_alf_calc_err_coef(E, y, filter_coef_quant_mod, num_coef, bit_depth);

                if (error < err_min)
                {
                    err_min = error;
                    min_ind = k;
                }
            }
        }

        if (min_ind != -1)
        {
            filter_coef_quant[min_ind] -= sign;
        }

        quant_coef_sum = 0;
        for (int i = 0; i < num_coef; i++)
        {
            quant_coef_sum += weights[i] * filter_coef_quant[i];
        }
        ++count;
    }
    if (count == 10)
    {
        xeve_mset(filter_coef_quant, 0, sizeof(int) * num_coef);
    }

    int max_value = 512 + 64 + 32 + 4 + 2;
    int min_value = -max_value;
    for (int i = 0; i < num_coef - 1; i++)
    {
        filter_coef_quant[i] = XEVE_CLIP3(min_value, max_value, filter_coef_quant[i]);
        filter_coef[i] = filter_coef_quant[i] / (double)factor;
    }

    quant_coef_sum = 0;
    for (int i = 0; i < num_coef - 1; i++)
    {
        quant_coef_sum += weights[i] * filter_coef_quant[i];
        filter_coef[i] = filter_coef_quant[i] / (double)factor;
    }
    filter_coef_quant[num_coef - 1] = -quant_coef_sum;
    filter_coef[num_coef - 1] = filter_coef_quant[num_coef - 1] / (double)factor;

    double error = xeve_alf_calc_err_coef(E, y, filter_coef_quant, num_coef, bit_depth);
    return error;
}

double xeve_alf_calc_err_coef(double **E, double *y,  const  int *coeff, const int num_coef, const int bit_depth)
{
    double factor = 1 << (bit_depth - 1);
    double error = 0;

    for (int i = 0; i < num_coef; i++)   //diagonal
    {
        double sum = 0;
        for (int j = i + 1; j < num_coef; j++)
        {
            sum += E[i][j] * coeff[j];
        }
        error += ((E[i][i] * coeff[i] + sum * 2) / factor - 2 * y[i]) * coeff[i];
    }

    return error / factor;
}

void xeve_alf_round_filt_coef(int *filter_coef_quant, double *filter_coef, const int num_coef, const int factor)
{
    for (int i = 0; i < num_coef; i++)
    {
        int sign = filter_coef[i] > 0 ? 1 : -1;
        filter_coef_quant[i] = (int)(filter_coef[i] * sign * factor + 0.5) * sign;
    }
}

void xeve_alf_find_best_fixed_filter(ALF_SLICE_PARAM* alf_slice_param, ALF_COVARIANCE* cov)
{
    double factor = 1 << (NUM_BITS - 1);
    for (int class_idx = 0; class_idx < MAX_NUM_ALF_CLASSES; class_idx++)
    {
        double err_min = cov[class_idx].pix_acc;
        alf_slice_param->fixed_filter_idx[class_idx] = 0;
        for (int filter_idx = 0; filter_idx < ALF_FIXED_FILTER_NUM; filter_idx++)
        {
            int fixed_filter_idx = alf_class_to_filter_mapping[class_idx][filter_idx];
            double errorFilter = cov[class_idx].pix_acc + xeve_alf_calc_err_coef(cov[class_idx].E, cov[class_idx].y, alf_fixed_filter_coef[fixed_filter_idx], (alf_slice_param->luma_filter_type ? 13 : 7), NUM_BITS);

            if (errorFilter < err_min)
            {
                err_min = errorFilter;
                alf_slice_param->fixed_filter_idx[class_idx] = filter_idx;
                alf_slice_param->fixed_filter_usage_flag[class_idx] = 1;
            }
        }
        //update stat
        int final_filter_idx = alf_slice_param->fixed_filter_idx[class_idx];
        u8 final_filter_usage_flag = alf_slice_param->fixed_filter_usage_flag[class_idx];
        if (final_filter_usage_flag > 0)
        {
            int fixed_filter_idx = alf_class_to_filter_mapping[class_idx][final_filter_idx];

            cov[class_idx].pix_acc = err_min;
            //update y
            for (int i = 0; i < (alf_slice_param->luma_filter_type ? 13 : 7); i++)
            {
                double sum = 0;
                for (int j = 0; j < (alf_slice_param->luma_filter_type ? 13 : 7); j++)
                {
                    sum += cov[class_idx].E[i][j] * alf_fixed_filter_coef[fixed_filter_idx][j];
                }
                sum /= factor;
                cov[class_idx].y[i] -= sum;
            }
        }
    }
}

void xeve_alf_merge_classes(ALF_COVARIANCE* cov, ALF_COVARIANCE* cov_merged, const int num_classes, short filter_indices[MAX_NUM_ALF_CLASSES][MAX_NUM_ALF_CLASSES])
{
    static BOOL avail_class[MAX_NUM_ALF_CLASSES];
    static u8 index_list[MAX_NUM_ALF_CLASSES];
    static u8 index_list_temp[MAX_NUM_ALF_CLASSES];
    int num_remaining = num_classes;

    xeve_mset(filter_indices, 0, sizeof(short) * MAX_NUM_ALF_CLASSES * MAX_NUM_ALF_CLASSES);

    for (int i = 0; i < num_classes; i++)
    {
        filter_indices[num_remaining - 1][i] = i;
        index_list[i] = i;
        avail_class[i] = TRUE;
        alf_cov_copy(&cov_merged[i], &cov[i]);
    }

    // Try merging different covariance matrices

    // temporal ALF_COVARIANCE structure is allocated as the last element in cov_merged array, the size of cov_merged is MAX_NUM_ALF_CLASSES + 1
    ALF_COVARIANCE* temp_cov = &cov_merged[MAX_NUM_ALF_CLASSES];

    while (num_remaining > 2)
    {
        double err_min = DBL_MAX;
        int best_to_merge_idx1 = 0, best_to_merge_idx2 = 1;

        for (int i = 0; i < num_classes - 1; i++)
        {
            if (avail_class[i])
            {
                for (int j = i + 1; j < num_classes; j++)
                {
                    if (avail_class[j])
                    {
                        double error1 = xeve_alf_clac_err(&cov_merged[i]);
                        double error2 = xeve_alf_clac_err(&cov_merged[j]);

                        alf_cov_add_to(temp_cov, &cov_merged[i], &cov_merged[j]);
                        double error = xeve_alf_clac_err(temp_cov) - error1 - error2;

                        if (error < err_min)
                        {
                            err_min = error;
                            best_to_merge_idx1 = i;
                            best_to_merge_idx2 = j;
                        }
                    }
                }
            }
        }

        alf_cov_add(&cov_merged[best_to_merge_idx1], &cov_merged[best_to_merge_idx2]);
        avail_class[best_to_merge_idx2] = FALSE;

        for (int i = 0; i < num_classes; i++)
        {
            if (index_list[i] == best_to_merge_idx2)
            {
                index_list[i] = best_to_merge_idx1;
            }
        }

        num_remaining--;
        if (num_remaining <= num_classes)
        {
            xeve_mcpy(index_list_temp, index_list, sizeof(u8) * num_classes);

            BOOL exist = FALSE;
            int ind = 0;

            for (int j = 0; j < num_classes; j++)
            {
                exist = FALSE;
                for (int i = 0; i < num_classes; i++)
                {
                    if (index_list_temp[i] == j)
                    {
                        exist = TRUE;
                        break;
                    }
                }

                if (exist)
                {
                    for (int i = 0; i < num_classes; i++)
                    {
                        if (index_list_temp[i] == j)
                        {
                            filter_indices[num_remaining - 1][i] = ind;
                            index_list_temp[i] = -1;
                        }
                    }
                    ind++;
                }
            }
        }
    }
}

void xeve_alf_get_frame_stats(XEVE_ALF * enc_alf, u8 comp_id, int input_shape_idx)
{
    ADAPTIVE_LOOP_FILTER * alf = &enc_alf->alf;
    int channel = comp_id == Y_C ? LUMA_CH : CHROMA_CH;
    int num_classes = channel == LUMA_CH ? MAX_NUM_ALF_CLASSES : 1;
    for (int i = 0; i < num_classes; i++)
    {
        alf_cov_reset(&enc_alf->alf_cov_frame[comp_id][input_shape_idx][i]);
    }
    if (comp_id == Y_C)
    {
        xeve_alf_get_frame_stat(enc_alf, enc_alf->alf_cov_frame[Y_C][input_shape_idx], enc_alf->alf_cov[Y_C][input_shape_idx], alf->ctu_enable_flag[Y_C], num_classes);
    }
    else if (comp_id == U_C)
    {
        xeve_alf_get_frame_stat(enc_alf, enc_alf->alf_cov_frame[U_C][input_shape_idx], enc_alf->alf_cov[U_C][input_shape_idx], alf->ctu_enable_flag[U_C], num_classes);
    }
    else if (comp_id == V_C)
    {
        xeve_alf_get_frame_stat(enc_alf, enc_alf->alf_cov_frame[V_C][input_shape_idx], enc_alf->alf_cov[V_C][input_shape_idx], alf->ctu_enable_flag[V_C], num_classes);
    }
}

void xeve_alf_get_frame_stat(XEVE_ALF * enc_alf, ALF_COVARIANCE* frame_cov, ALF_COVARIANCE** ctb_cov, u8* ctb_enable_flags, const int num_classes)
{
    ADAPTIVE_LOOP_FILTER * alf = &enc_alf->alf;
    for (int i = 0; i < alf->num_ctu_in_pic; i++)
    {
        if (ctb_enable_flags[i])
        {
            for (int j = 0; j < num_classes; j++)
            {
                alf_cov_add(&frame_cov[j], &ctb_cov[i][j]);
            }
        }
    }
}

void xeve_alf_derive_stats_filtering(XEVE_ALF * enc_alf, YUV * org_yuv, YUV * rec_yuv)
{
    ADAPTIVE_LOOP_FILTER * alf = &enc_alf->alf;
    int ctu_rs_addr = 0;
    const int num_comp = (alf->chroma_format == 1) ? N_C : 1;
    // init CTU stats buffers
    for (u8 comp_id = 0; comp_id < num_comp; comp_id++)
    {
        const int num_classes = comp_id == Y_C ? MAX_NUM_ALF_CLASSES : 1;
        const u8 channel = comp_id == Y_C ? LUMA_CH : CHROMA_CH;
        const int size = channel == LUMA_CH ? 2 : 1;

        for (int shape = 0; shape != size; shape++)
        {
            for (int class_idx = 0; class_idx < num_classes; class_idx++)
            {
                for (int ctu_idx = 0; ctu_idx < alf->num_ctu_in_pic; ctu_idx++)
                {
                    alf_cov_reset(&enc_alf->alf_cov[comp_id][shape][ctu_idx][class_idx]);
                }
            }
        }
    }

    // init Frame stats buffers
    for (u8 comp_id = 0; comp_id < num_comp; comp_id++)
    {
        const int num_classes = (comp_id == LUMA_CH) ? MAX_NUM_ALF_CLASSES : 1;
        const int size = (comp_id == LUMA_CH) ? 2 : 1;

        for (int shape = 0; shape != size; shape++)
        {
            for (int class_idx = 0; class_idx < num_classes; class_idx++)
            {
                alf_cov_reset(&enc_alf->alf_cov_frame[comp_id][shape][class_idx]);
            }
        }
    }

    for (int y_pos = 0; y_pos < alf->pic_height; y_pos += alf->max_cu_height)
    {
        for (int x_pos = 0; x_pos < alf->pic_width; x_pos += alf->max_cu_width)
        {
            const int width = (x_pos + alf->max_cu_width > alf->pic_width) ? (alf->pic_width - x_pos) : alf->max_cu_width;
            const int height = (y_pos + alf->max_cu_height > alf->pic_height) ? (alf->pic_height - y_pos) : alf->max_cu_height;

            for (u8 comp_id = 0; comp_id < num_comp; comp_id++)
            {
                //for 4:2:0 only
                int width2 = 0, height2 = 0, x_pos2 = 0, y_pos2 = 0;
                if (comp_id > 0) {
                    width2 = width >> 1;
                    height2 = height >> 1;
                    x_pos2 = x_pos >> 1;
                    y_pos2 = y_pos >> 1;
                }
                else
                {
                    width2 = width;
                    height2 = height;
                    x_pos2 = x_pos;
                    y_pos2 = y_pos;
                }

                int  rec_stride = rec_yuv->s[comp_id];
                pel* rec = rec_yuv->yuv[comp_id];

                int  org_stride = org_yuv->s[comp_id];
                pel* org = org_yuv->yuv[comp_id];

                u8 ch_type = (comp_id == Y_C) ? LUMA_CH : CHROMA_CH;
                const int size = (ch_type == LUMA_CH) ? 2 : 1;

                for (int shape = 0; shape != size; shape++)
                {
                    xeve_alf_get_blk_stats((int)ch_type, enc_alf->alf_cov[comp_id][shape][ctu_rs_addr], &alf->filter_shapes[ch_type][shape]
                              , comp_id ? NULL : alf->classifier, org, org_stride, rec, rec_stride, x_pos2, y_pos2, width2, height2);

                    const int num_classes = comp_id == Y_C ? MAX_NUM_ALF_CLASSES : 1;

                    for (int class_idx = 0; class_idx < num_classes; class_idx++)
                    {
                        alf_cov_add(&enc_alf->alf_cov_frame[comp_id][shape][class_idx], &enc_alf->alf_cov[comp_id][shape][ctu_rs_addr][class_idx]);
                    }
                }
            }
            ctu_rs_addr++;
        }
    }
}

void xeve_alf_get_blk_stats(int ch, ALF_COVARIANCE* alf_cov, const ALF_FILTER_SHAPE* shape, ALF_CLASSIFIER** classifier, pel* org0
                          , const int org_stride, pel* rec0, const int rec_stride, const int x, const int y, const int width, const int height)
{
    static int E_local[MAX_NUM_ALF_LUMA_COEFF];
    int trans_idx = 0;
    int class_idx = 0;
    pel * rec = rec0 + y * rec_stride + x;
    pel * org = org0 + y * org_stride + x;

    for (int i = 0; i < height; i++)
    {
        for (int j = 0; j < width; j++)
        {
            xeve_mset(E_local, 0, shape->num_coef * sizeof(int));
            if (classifier )
            {
                int x2 = ch ? (x << 1) : x;
                int y2 = ch ? (y << 1) : y;
                ALF_CLASSIFIER cl = classifier[y2 + i][x2 + j];
                trans_idx = cl & 0x03;
                class_idx = (cl >> 2) & 0x1F;
            }

            int yLocal = org[j] - rec[j];
            xeve_alf_clac_covariance(E_local, rec + j, rec_stride, shape->pattern, shape->filterLength >> 1, trans_idx);
            for (int k = 0; k < shape->num_coef; k++)
            {
                for (int l = k; l < shape->num_coef; l++)
                {
                    alf_cov[class_idx].E[k][l] += E_local[k] * E_local[l];
                }
                alf_cov[class_idx].y[k] += E_local[k] * yLocal;
            }
            alf_cov[class_idx].pix_acc += yLocal * yLocal;
        }
        org += org_stride;
        rec += rec_stride;
    }

    int num_classes = classifier ? MAX_NUM_ALF_CLASSES : 1;
    for (class_idx = 0; class_idx < num_classes; class_idx++)
    {
        for (int k = 1; k < shape->num_coef; k++)
        {
            for (int l = 0; l < k; l++)
            {
                alf_cov[class_idx].E[k][l] = alf_cov[class_idx].E[l][k];
            }
        }
    }
}

void xeve_alf_clac_covariance(int *E_local, const pel *rec, const int stride, const int *filter_pattern, const int half_filter_length, const int trans_idx)
{
    int k = 0;

    if (trans_idx == 0)
    {
        for (int i = -half_filter_length; i < 0; i++)
        {
            const pel * rec0 = rec + i * stride;
            const pel * rec1 = rec - i * stride;

            for (int j = -half_filter_length - i; j <= half_filter_length + i; j++)
            {
                E_local[filter_pattern[k++]] += rec0[j] + rec1[-j];
            }
        }
        for (int j = -half_filter_length; j < 0; j++)
        {
            E_local[filter_pattern[k++]] += rec[j] + rec[-j];
        }
    }
    else if (trans_idx == 1)
    {
        for (int j = -half_filter_length; j < 0; j++)
        {
            const pel * rec0 = rec + j;
            const pel * rec1 = rec - j;

            for (int i = -half_filter_length - j; i <= half_filter_length + j; i++)
            {
                E_local[filter_pattern[k++]] += rec0[i * stride] + rec1[-i * stride];
            }
        }
        for (int i = -half_filter_length; i < 0; i++)
        {
            E_local[filter_pattern[k++]] += rec[i*stride] + rec[-i * stride];
        }
    }
    else if (trans_idx == 2)
    {
        for (int i = -half_filter_length; i < 0; i++)
        {
            const pel * rec0 = rec + i * stride;
            const pel * rec1 = rec - i * stride;

            for (int j = half_filter_length + i; j >= -half_filter_length - i; j--)
            {
                E_local[filter_pattern[k++]] += rec0[j] + rec1[-j];
            }
        }
        for (int j = -half_filter_length; j < 0; j++)
        {
            E_local[filter_pattern[k++]] += rec[j] + rec[-j];
        }
    }
    else
    {
        for (int j = -half_filter_length; j < 0; j++)
        {
            const pel * rec0 = rec + j;
            const pel * rec1 = rec - j;

            for (int i = half_filter_length + j; i >= -half_filter_length - j; i--)
            {
                E_local[filter_pattern[k++]] += rec0[i * stride] + rec1[-i * stride];
            }
        }
        for (int i = -half_filter_length; i < 0; i++)
        {
            E_local[filter_pattern[k++]] += rec[i*stride] + rec[-i * stride];
        }
    }
    E_local[filter_pattern[k++]] += rec[0];
}


double xeve_alf_clac_err(ALF_COVARIANCE* cov)
{
    static double c[MAX_NUM_ALF_COEFF];

    xeve_alf_gns_solve_chol(cov->E, cov->y, c, cov->num_coef);

    double sum = 0;
    for (int i = 0; i < cov->num_coef; i++)
    {
        sum += c[i] * cov->y[i];
    }

    return cov->pix_acc - sum;
}

//********************************
// Cholesky decomposition
//********************************
#define ROUND(a)  (((a) < 0)? (int)((a) - 0.5) : (int)((a) + 0.5))
#define REG              0.0001
#define REG_SQR          0.0000001

//Find filter coeff related
int xeve_alf_gns_cholesky_dec(double **input_matr, double out_matr[MAX_NUM_ALF_COEFF][MAX_NUM_ALF_COEFF], int num_eq)
{
    static double inv_diag[MAX_NUM_ALF_COEFF];  /* Vector of the inverse of diagonal entries of out_matr */

    for (int i = 0; i < num_eq; i++)
    {
        for (int j = i; j < num_eq; j++)
        {
            /* Compute the scaling factor */
            double scale = input_matr[i][j];
            if (i > 0)
            {
                for (int k = i - 1; k >= 0; k--)
                {
                    scale -= out_matr[k][j] * out_matr[k][i];
                }
            }

            /* Compute i'th row of out_matr */
            if (i == j)
            {
                if (scale <= REG_SQR) // if(scale <= 0 )  /* If input_matr is singular */
                {
                    return 0;
                }
                else              /* Normal operation */
                    inv_diag[i] = 1.0 / (out_matr[i][i] = sqrt(scale));
            }
            else
            {
                out_matr[i][j] = scale * inv_diag[i]; /* Upper triangular part          */
                out_matr[j][i] = 0.0;              /* Lower triangular part set to 0 */
            }
        }
    }
    return 1; /* Signal that Cholesky factorization is successfully performed */
}

void xeve_alf_gns_transpose_back_substitution(double U[MAX_NUM_ALF_COEFF][MAX_NUM_ALF_COEFF], double* rhs, double* x, int order)
{
    /* Backsubstitution starts */
    x[0] = rhs[0] / U[0][0];               /* First row of U'                   */
    for (int i = 1; i < order; i++)
    {         /* For the rows 1..order-1           */

        double sum = 0; //Holds backsubstitution from already handled rows

        for (int j = 0; j < i; j++) /* Backsubst already solved unknowns */
        {
            sum += x[j] * U[j][i];
        }

        x[i] = (rhs[i] - sum) / U[i][i];       /* i'th component of solution vect.  */
    }
}

void xeve_alf_gns_back_substitution(double R[MAX_NUM_ALF_COEFF][MAX_NUM_ALF_COEFF], double* z, int size, double* A)
{
    size--;
    A[size] = z[size] / R[size][size];

    for (int i = size - 1; i >= 0; i--)
    {
        double sum = 0;

        for (int j = i + 1; j <= size; j++)
        {
            sum += R[i][j] * A[j];
        }

        A[i] = (z[i] - sum) / R[i][i];
    }
}

int xeve_alf_gns_solve_chol(double **LHS, double *rhs, double *x, int num_eq)
{
    static double aux[MAX_NUM_ALF_COEFF];     /* Auxiliary vector */
    static double U[MAX_NUM_ALF_COEFF][MAX_NUM_ALF_COEFF];    /* Upper triangular Cholesky factor of LHS */
    int res = 1;  // Signal that Cholesky factorization is successfully performed
                  /* The equation to be solved is LHSx = rhs */
                  /* Compute upper triangular U such that U'*U = LHS */
    if (xeve_alf_gns_cholesky_dec(LHS, U, num_eq)) /* If Cholesky decomposition has been successful */
    {
        /* Now, the equation is  U'*U*x = rhs, where U is upper triangular
        * Solve U'*aux = rhs for aux
        */
        xeve_alf_gns_transpose_back_substitution(U, rhs, aux, num_eq);

        /* The equation is now U*x = aux, solve it for x (new motion coefficients) */
        xeve_alf_gns_back_substitution(U, aux, num_eq, x);
    }
    else /* LHS was singular */
    {
        res = 0;

        /* Regularize LHS */
        for (int i = 0; i < num_eq; i++)
        {
            LHS[i][i] += REG;
        }

        /* Compute upper triangular U such that U'*U = regularized LHS */
        res = xeve_alf_gns_cholesky_dec(LHS, U, num_eq);

        if (!res)
        {
            xeve_mset(x, 0, sizeof(double)*num_eq);
            return 0;
        }

        /* Solve  U'*aux = rhs for aux */
        xeve_alf_gns_transpose_back_substitution(U, rhs, aux, num_eq);

        /* Solve U*x = aux for x */
        xeve_alf_gns_back_substitution(U, aux, num_eq, x);
    }
    return res;
}
//////////////////////////////////////////////////////////////////////////////////////////
void xeve_alf_set_enable_flag(ALF_SLICE_PARAM* alf_slice_param, u8 comp_id, BOOL val)
{
  if (comp_id == Y_C)
  {
    alf_slice_param->enable_flag[Y_C] = val;
  }
  else if (comp_id == U_C)
  {
    alf_slice_param->enable_flag[U_C] = val;
  }
  else if (comp_id == V_C)
  {
    alf_slice_param->enable_flag[V_C] = val;
  }
}

void xeve_alf_set_enable_ctb_flag(XEVE_ALF * enc_alf, ALF_SLICE_PARAM* alf_slice_param, u8 comp_id, u8** ctu_flags)
{
    ADAPTIVE_LOOP_FILTER * alf = &enc_alf->alf;
    alf_slice_param->enable_flag[comp_id] = FALSE;
    for (int i = 0; i < alf->num_ctu_in_pic; i++)
    {
        if (ctu_flags[comp_id][i])
        {
            alf_slice_param->enable_flag[comp_id] = TRUE;
            break;
        }
    }
}

void xeve_alf_copy_ctb_enable_flag(XEVE_ALF * enc_alf, u8** ctu_flags_dst, u8** ctu_flags_src, u8 comp_id)
{
    ADAPTIVE_LOOP_FILTER * alf = &enc_alf->alf;
    if (comp_id == Y_C)
    {
        xeve_mcpy(ctu_flags_dst[Y_C], ctu_flags_src[Y_C], sizeof(u8) * alf->num_ctu_in_pic);
    }
    else if (comp_id == U_C)
    {
        xeve_mcpy(ctu_flags_dst[U_C], ctu_flags_src[U_C], sizeof(u8) * alf->num_ctu_in_pic);
    }
    else if (comp_id == V_C)
    {
        xeve_mcpy(ctu_flags_dst[V_C], ctu_flags_src[V_C], sizeof(u8) * alf->num_ctu_in_pic);
    }
}

void xeve_alf_set_ctb_enable_flag(XEVE_ALF * enc_alf, u8** ctu_flags, u8 comp_id, u8 val)
{
    ADAPTIVE_LOOP_FILTER * alf = &enc_alf->alf;
    if (comp_id == Y_C)
    {
        xeve_mset(ctu_flags[Y_C], val, sizeof(u8) * alf->num_ctu_in_pic);
    }
    else if (comp_id == U_C)
    {
        xeve_mset(ctu_flags[U_C], val, sizeof(u8) * alf->num_ctu_in_pic);
    }
    else if (comp_id == V_C)
    {
        xeve_mset(ctu_flags[V_C], val, sizeof(u8) * alf->num_ctu_in_pic);
    }
}

int xevem_alf_aps(XEVE_CTX * ctx, XEVE_PIC * pic, XEVE_SH* sh, XEVE_APS* aps)
{
    XEVEM_CTX *mctx = (XEVEM_CTX *)ctx;
    XEVE_ALF  * enc_anf = (XEVE_ALF *)(mctx->enc_alf);
    int ret = XEVE_OK;
    double lambdas[3];
    for (int i = 0; i < 3; i++)
        lambdas[i] = (ctx->lambda[i]) * ALF_LAMBDA_SCALE; //this is for appr match of different lambda sets


    xeve_alf_set_reset_alf_buf_flag(enc_anf, sh->slice_type == SLICE_I ? 1 : 0);
    ret = xeve_alf_aps_enc_opt_process(enc_anf, lambdas, ctx, pic, &(sh->alf_sh_param));

    aps->alf_aps_param = sh->alf_sh_param;
    if (sh->alf_sh_param.reset_alf_buf_flag) // reset aps index counter (buffer) if ALF flag reset is present
    {
        ctx->aps_counter = -1;
    }
    sh->alf_on = sh->alf_sh_param.enable_flag[0];
    if (sh->alf_on == 0)
    {
        sh->alf_sh_param.is_ctb_alf_on = 0;
    }
    if (sh->alf_on)
    {
        if (aps->alf_aps_param.temporal_alf_flag)
        {
            aps->aps_id = sh->alf_sh_param.prev_idx;
            sh->aps_id_y = sh->alf_sh_param.prev_idx_comp[0];
            sh->aps_id_ch = sh->alf_sh_param.prev_idx_comp[1];
            sh->aps_signaled = aps->aps_id;
        }
        else
        {
            aps->aps_id = xeve_alf_aps_get_current_alf_idx(enc_anf);
            sh->aps_id_y = aps->aps_id;
            sh->aps_id_ch = aps->aps_id;
            sh->aps_signaled = aps->aps_id;
        }
    }
    return ret;
}

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
    int sad;
    int16_t const* s1 = src1;
    int16_t const* s2 = src2;
    /*
        -- Variable naming conventions are similar as SSE code.
        -- 4x16b translates to 4 datapoints each stored in 16bit 
    */
    int16x4_t src_4x16b = vld1_s16((s1));
    int16x4_t pred_4x16b = vld1_s16((s2));
	  int16x4_t src_4x16b_1 = vld1_s16((s1 + s_src1));
    int16x4_t pred_4x16b_1 = vld1_s16((s2 + s_src2));
	
    int16x4_t abs_diff_4x16b = vabd_s16(src_4x16b, pred_4x16b);
    int16x4_t abs_diff_4x16b_1 = vabd_s16(src_4x16b_1, pred_4x16b_1);
    
  	sad = vaddv_s16(abs_diff_4x16b);
  	sad += vaddv_s16(abs_diff_4x16b_1);

    return (sad >> (bit_depth - 8));
}

int sad_16b_neon_4x2n(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int bit_depth)
{
    int sad;
    int16_t const* s1 = src1;
    int16_t const* s2 = src2;
    int i;
    /*
        -- Variable naming conventions are similar as SSE code.
        -- 4x16b translates to 4 datapoints each stored in 16bit 
    */
    int16x4_t src_4x16b, pred_4x16b, abs_diff_4x16b;
	  int16x4_t src_4x16b_1, pred_4x16b_1, abs_diff_4x16b_1;
	
    h = h >> 1;
    for (i = 0; i != h; ++i)
    {
        src_4x16b = vld1_s16((s1));
        pred_4x16b = vld1_s16((s2));
        src_4x16b_1 = vld1_s16((s1 + s_src1));
        pred_4x16b_1 = vld1_s16((s2 + s_src2));
		
		    abs_diff_4x16b = vabd_s16(src_4x16b, pred_4x16b);
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
    int sad;
    int16_t const* s1 = src1;
    int16_t const* s2 = src2;
    /*
        -- Variable naming conventions are same as SSE code.
        -- 4x16b translates to 4 datapoints each stored in 16bit 
    */
    int16x4_t src_4x16b, pred_4x16b, abs_diff_4x16b;
  	int16x4_t src_4x16b_1, pred_4x16b_1, abs_diff_4x16b_1;
  	int16x4_t src_4x16b_2, pred_4x16b_2, abs_diff_4x16b_2;
  	int16x4_t src_4x16b_3, pred_4x16b_3, abs_diff_4x16b_3;
    
    src_4x16b = vld1_s16((s1));
    pred_4x16b = vld1_s16((s2));
    
	  src_4x16b_1 = vld1_s16((s1 + s_src1));
    pred_4x16b_1 = vld1_s16((s2 + s_src2));
	  
    src_4x16b_2 = vld1_s16((s1 + s_src1 * 2));
    pred_4x16b_2 = vld1_s16((s2 + s_src2 * 2));
    
    src_4x16b_3 = vld1_s16((s1 + s_src1 * 3));
    pred_4x16b_3 = vld1_s16((s2 + s_src2 * 3));    
    
    abs_diff_4x16b = vabd_s16(src_4x16b, pred_4x16b);
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
    int16_t const* pu2_inp, * pu2_inp2;
    int16_t const* pu2_ref, * pu2_ref2;

    pu2_inp = src1;
    pu2_ref = src2;
    pu2_inp2 = (s16*)src1 + s_src1;
    pu2_ref2 = (s16*)src2 + s_src2;
    int s_src1_t2 = s_src1 * 2;
    int s_src2_t2 = s_src2 * 2;

    int i, j;
    u32 sad = 0;
    /*
        -- Variable naming conventions are same as SSE code.
        -- 8x16b translates to 8 datapoints each stored in 16bit 
    */
    int16x8_t src_8x16b, pred_8x16b, abs_diff_8x16b;
    int16x8_t src_8x16b_1, pred_8x16b_1, abs_diff_8x16b_1;
    
    h = h >> 1;
    for (i = 0; i != h; ++i)
    {
        for (j = 0; j < w; j += 8)
        {
            src_8x16b = vld1q_s16(&pu2_inp[j]);
            pred_8x16b = vld1q_s16(&pu2_ref[j]);
            
            src_8x16b_1 = vld1q_s16(&pu2_inp2[j]);
            pred_8x16b_1 = vld1q_s16(&pu2_ref2[j]);
            
            abs_diff_8x16b = vabdq_s16(src_8x16b, pred_8x16b);
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
    
    u32 sad = 0;
    int16x8_t src_8x16b, pred_8x16b, abs_diff_8x16b;
    int16x8_t src_8x16b_1, pred_8x16b_1, abs_diff_8x16b_1;
    for (int i = 0; i != h; ++i)
    {
        for (int j = 0; j < w; j += 16)
        {
            src_8x16b = vld1q_s16(&pu2_inp[j]);
            pred_8x16b = vld1q_s16(&pu2_ref[j]);
            src_8x16b_1 = vld1q_s16(&pu2_inp[j + 8]);
            pred_8x16b_1 = vld1q_s16(&pu2_ref[j + 8]);
            
            abs_diff_8x16b = vabdq_s16(src_8x16b, pred_8x16b);
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
const XEVE_FN_SAD xeve_tbl_sad_16b_neon[8][8] =
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
        sad_16b,          /* height == 1 */
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
        sad_16b_neon_16nx1n,  /* height == 1 */
        sad_16b_neon_16nx1n,  /* height == 2 */
        sad_16b_neon_16nx1n,  /* height == 4 */
        sad_16b_neon_16nx1n,  /* height == 8 */
        sad_16b_neon_16nx1n,  /* height == 16 */
        sad_16b_neon_16nx1n,  /* height == 32 */
        sad_16b_neon_16nx1n,  /* height == 64 */
        sad_16b_neon_16nx1n,  /* height == 128 */
    },
    /* width == 32 */
    {
        sad_16b_neon_16nx1n,  /* height == 1 */
        sad_16b_neon_16nx1n,  /* height == 2 */
        sad_16b_neon_16nx1n,  /* height == 4 */
        sad_16b_neon_16nx1n,  /* height == 8 */
        sad_16b_neon_16nx1n,  /* height == 16 */
        sad_16b_neon_16nx1n,  /* height == 32 */
        sad_16b_neon_16nx1n,  /* height == 64 */
        sad_16b_neon_16nx1n,  /* height == 128 */
    },
    /* width == 64 */
    {
        sad_16b_neon_16nx1n,  /* height == 1 */
        sad_16b_neon_16nx1n,  /* height == 2 */
        sad_16b_neon_16nx1n,  /* height == 4 */
        sad_16b_neon_16nx1n,  /* height == 8 */
        sad_16b_neon_16nx1n,  /* height == 16 */
        sad_16b_neon_16nx1n,  /* height == 32 */
        sad_16b_neon_16nx1n,  /* height == 64 */
        sad_16b_neon_16nx1n,  /* height == 128 */
    },
    /* width == 128 */
    {
        sad_16b_neon_16nx1n,  /* height == 1 */
        sad_16b_neon_16nx1n,  /* height == 2 */
        sad_16b_neon_16nx1n,  /* height == 4 */
        sad_16b_neon_16nx1n,  /* height == 8 */
        sad_16b_neon_16nx1n,  /* height == 16 */
        sad_16b_neon_16nx1n,  /* height == 32 */
        sad_16b_neon_16nx1n,  /* height == 64 */
        sad_16b_neon_16nx1n,  /* height == 128 */
    }
};


/* DIFF **********************************************************************/
#define NEON_DIFF_16B_4PEL(src1, src2, diff, m00, m01, m02) \
    m00 = vld1_s16((src1)); \
    m01 = vld1_s16((src2)); \
    m02 = vsub_s16(m00, m01); \
    vst1_s16((int16_t*)(diff), m02);

#define NEON_DIFF_16B_8PEL(src1, src2, diff, m00, m01, m02) \
    m00 = vld1q_s16((src1)); \
    m01 = vld1q_s16((src2)); \
    m02 = vsubq_s16(m00, m01); \
    vst1q_s16((int16_t*)(diff), m02);

static void diff_16b_neon_4x2(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int s_diff, s16 * diff, int bit_depth)
{
    int16_t const* s1;
    int16_t const* s2;
    int16x4_t m01, m02, m03, m04, m05, m06;

    s1 = src1;
    s2 = src2;

    NEON_DIFF_16B_4PEL(s1, s2, diff, m01, m02, m03);
    NEON_DIFF_16B_4PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
}

static void diff_16b_neon_4x4(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int s_diff, s16 * diff, int bit_depth)
{
    int16_t const* s1;
    int16_t const* s2;
    int16x4_t m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12;

    s1 = src1;
    s2 = src2;

    NEON_DIFF_16B_4PEL(s1, s2, diff, m01, m02, m03);
    NEON_DIFF_16B_4PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
    NEON_DIFF_16B_4PEL(s1 + s_src1*2, s2 + s_src2*2, diff + s_diff*2, m07, m08, m09);
    NEON_DIFF_16B_4PEL(s1 + s_src1*3, s2 + s_src2*3, diff + s_diff*3, m10, m11, m12);
}

static void diff_16b_neon_8x8(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int s_diff, s16 * diff, int bit_depth)
{
    int16_t const* s1;
    int16_t const* s2;
    int16x8_t m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12;

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

static void diff_16b_neon_8nx2n(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int s_diff, s16 * diff, int bit_depth)
{
    int16_t const* s1;
    int16_t const* s2;
    int i, j;
    int16x8_t m01, m02, m03, m04, m05, m06;

    s1 = src1;
    s2 = src2;
    
    h = h >> 1;
    w = w >> 3;
    for(i = 0; i != h; ++i)
    {
        for(j = 0; j != w; ++j)
        {
            NEON_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            NEON_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            s1 += 8; s2 += 8; diff += 8;
        }

        s1   += ((s_src1 << 1) - (w << 3));
        s2   += ((s_src2 << 1) - (w << 3));
        diff += ((s_diff << 1) - (w << 3));
    }
}

static void diff_16b_neon_16nx2n(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int s_diff, s16 * diff, int bit_depth)
{
    int16_t const* s1;
    int16_t const* s2;
    int i, j;
    int16x8_t m01, m02, m03, m04, m05, m06;

    s1 = src1;
    s2 = src2;
    
    h = h >> 1;
    w = w >> 4;
    
    for(i = 0; i != h; ++i)
    {
        for(j = 0; j != w; ++j)
        {
            NEON_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            NEON_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            s1 += 8; s2 += 8; diff += 8;

            NEON_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            NEON_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            s1 += 8; s2 += 8; diff += 8;
        }

        s2   += ((s_src2<<1) - (w << 4));
        s1   += ((s_src1<<1) - (w << 4));
        diff += ((s_diff<<1) - (w << 4));
    }
}

static void diff_16b_neon_32nx4n(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int s_diff, s16 * diff, int bit_depth)
{
    int16_t const* s1;
    int16_t const* s2;
    int i, j;
    int16x8_t m01, m02, m03, m04, m05, m06, m07, m08, m09, m10, m11, m12;

    s1 = src1;
    s2 = src2;
    
    h = h >> 2;
    w = w >> 5;
    
    for(i = 0; i != h; ++i)
    {
        for(j = 0; j != w; ++j)
        {
            NEON_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            NEON_DIFF_16B_8PEL(s1+s_src1, s2+s_src2, diff+s_diff, m04, m05, m06);
            NEON_DIFF_16B_8PEL(s1+s_src1 * 2, s2 + s_src2 * 2, diff +s_diff * 2, m07, m08, m09);
            NEON_DIFF_16B_8PEL(s1+s_src1 * 3, s2 + s_src2 * 3, diff +s_diff * 3, m10, m11, m12);
            s1 += 8; s2 += 8; diff+= 8;

            NEON_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            NEON_DIFF_16B_8PEL(s1 + s_src1, s2 + s_src2, diff + s_diff, m04, m05, m06);
            NEON_DIFF_16B_8PEL(s1 + s_src1 * 2, s2 + s_src2*2, diff + s_diff * 2, m07, m08, m09);
            NEON_DIFF_16B_8PEL(s1 + s_src1 * 3, s2 + s_src2*3, diff + s_diff * 3, m10, m11, m12);
            s1 += 8; s2 += 8; diff+= 8;

            NEON_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            NEON_DIFF_16B_8PEL(s1 + s_src1, s2+s_src2, diff+s_diff, m04, m05, m06);
            NEON_DIFF_16B_8PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, diff + s_diff * 2, m07, m08, m09);
            NEON_DIFF_16B_8PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, diff + s_diff * 3, m10, m11, m12);
            s1 += 8; s2 += 8; diff+= 8;

            NEON_DIFF_16B_8PEL(s1, s2, diff, m01, m02, m03);
            NEON_DIFF_16B_8PEL(s1 + s_src1, s2+s_src2, diff+s_diff, m04, m05, m06);
            NEON_DIFF_16B_8PEL(s1 + s_src1 * 2, s2 + s_src2 * 2, diff + s_diff * 2, m07, m08, m09);
            NEON_DIFF_16B_8PEL(s1 + s_src1 * 3, s2 + s_src2 * 3, diff + s_diff * 3, m10, m11, m12);
            s1 += 8; s2 += 8; diff+= 8;
        }

        s1   += ((s_src1<<2) - (w << 5));
        s2   += ((s_src2<<2) - (w << 5));
        diff += ((s_diff<<2) - (w << 5));
    }
}

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
    ssd += vaddvq_s32(s00b); \

static s64 ssd_16b_neon_4x2(int w, int h, void* src1, void* src2, int s_src1, int s_src2, int bit_depth)
{
    s64   ssd = 0;
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
    s64   ssd = 0;
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
    s64   ssd = 0;
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
    s64   ssd = 0;
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
    s64   ssd = 0;
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
static s64 ssd_16b_neon_8x2(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int bit_depth)
{
    s64   ssd = 0;
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

static s64 ssd_16b_neon_8x4(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int bit_depth)
{
    s64   ssd = 0;
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

static s64 ssd_16b_neon_8x8(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int bit_depth)
{
    s64   ssd = 0;
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

static s64 ssd_16b_neon_8nx2n(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int bit_depth)
{
    s64   ssd = 0;
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
    for (i = 0; i != h; ++i)
    {
        for (j = 0; j != w; ++j)
        {
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

static s64 ssd_16b_neon_8nx4n(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int bit_depth)
{
    s64   ssd = 0;
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
    
    for (i = 0; i != h; ++i)
    {
        for (j = 0; j != w; ++j)
        {
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

static s64 ssd_16b_neon_8nx8n(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int bit_depth)
{
    s64   ssd = 0;
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
    for (i = 0; i != h; ++i)
    {
        for (j = 0; j != w; ++j)
        {
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
#endif
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

#ifndef _XEVE_SAD_H_
#define _XEVE_SAD_H_

#include "xeve_port.h"

int sad_16b(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth);
void diff_16b(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int s_diff, s16 * diff, int bit_depth);
s64 ssd_16b(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth);
int xeve_had_2x2(pel *org, pel *cur, int s_org, int s_cur, int step);

typedef int  (*XEVE_FN_SAD)  (int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth);
typedef int  (*XEVE_FN_SATD) (int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth);
typedef s64  (*XEVE_FN_SSD)  (int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth);
typedef void (*XEVE_FN_DIFF) (int w, int h, void *src1, void *src2, int s_src1, int s_src2, int s_diff, s16 *diff, int bit_depth);

extern const XEVE_FN_SAD  xeve_tbl_sad_16b[8][8];
extern const XEVE_FN_SSD  xeve_tbl_ssd_16b[8][8];
extern const XEVE_FN_DIFF xeve_tbl_diff_16b[8][8];
extern const XEVE_FN_SATD xeve_tbl_satd_16b[1];

extern const XEVE_FN_SAD  (* xeve_func_sad)[8];
extern const XEVE_FN_SSD  (* xeve_func_ssd)[8];
extern const XEVE_FN_DIFF (* xeve_func_diff)[8];
extern const XEVE_FN_SATD (* xeve_func_satd);

#define xeve_sad_16b(log2w, log2h, src1, src2, s_src1, s_src2, bit_depth)\
        xeve_func_sad[log2w][log2h](1<<(log2w), 1<<(log2h), src1, src2, s_src1, s_src2, bit_depth)
#define xeve_sad_bi_16b(log2w, log2h, src1, src2, s_src1, s_src2, bit_depth)\
       (xeve_func_sad[log2w][log2h](1<<(log2w), 1<<(log2h), src1, src2, s_src1, s_src2, bit_depth) >> 1)
#define xeve_satd_16b(log2w, log2h, src1, src2, s_src1, s_src2, bit_depth)\
        xeve_func_satd[0](1<<(log2w), 1<<(log2h), src1, src2, s_src1, s_src2, bit_depth)
#define xeve_satd_bi_16b(log2w, log2h, src1, src2, s_src1, s_src2, bit_depth)\
       (xeve_func_satd[0](1<<(log2w), 1<<(log2h), src1, src2, s_src1, s_src2, bit_depth) >> 1)
#define xeve_ssd_16b(log2w, log2h, src1, src2, s_src1, s_src2, bit_depth)\
        xeve_func_ssd[log2w][log2h](1<<(log2w), 1<<(log2h), src1, src2, s_src1, s_src2, bit_depth)
#define xeve_diff_16b(log2w, log2h, src1, src2, s_src1, s_src2, s_diff, diff, bit_depth) \
        xeve_func_diff[log2w][log2h](1<<(log2w), 1<<(log2h), src1, src2, s_src1, s_src2, s_diff, diff, bit_depth)

#endif /* _XEVE_SAD_H_ */

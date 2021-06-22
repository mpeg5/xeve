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

#ifndef _XEVE_SAD_SSE_H_
#define _XEVE_SAD_SSE_H_

#include "xeve_port.h"
#include "xeve_sad.h"
#if X86_SSE
extern const XEVE_FN_SAD xeve_tbl_sad_16b_sse[8][8];
extern const XEVE_FN_SSD xeve_tbl_ssd_16b_sse[8][8];
extern const XEVE_FN_DIFF xeve_tbl_diff_16b_sse[8][8];
extern const XEVE_FN_SATD xeve_tbl_satd_16b_sse[1];

int sad_16b_sse_4x2(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int bit_depth);
int sad_16b_sse_4x2n(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int bit_depth);
int sad_16b_sse_4x4(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int bit_depth);
int sad_16b_sse_8x2n(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int bit_depth);
int sad_16b_sse_16nx1n(int w, int h, void * src1, void * src2, int s_src1, int s_src2, int bit_depth);

#endif /* X86_SSE */
#endif /* _XEVE_SAD_SSE_H_ */

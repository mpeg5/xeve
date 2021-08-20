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

#ifndef _XEVEM_ITDQ_SSE_H_
#define _XEVEM_ITDQ_SSE_H_

#include "xeve_def.h"
#if X86_SSE
extern const XEVE_INV_TRANS xeve_itrans_map_tbl_sse[16][5];

void xeve_itrans_ats_intra_DST7_B8_sse(s16 *coeff, s16 *block, int shift, int line, int skip_line, int skip_line_2);
void xeve_itrans_ats_intra_DST7_B16_sse(s16 *coeff, s16 *block, int shift, int line, int skip_line, int skip_line_2);
void xeve_itrans_ats_intra_DST7_B32_sse(s16 *coeff, s16 *block, int shift, int line, int skip_line, int skip_line_2);
void xeve_itrans_ats_intra_DCT8_B8_sse(s16 *coeff, s16 *block, int shift, int line, int skip_line, int skip_line_2);
void xeve_itrans_ats_intra_DCT8_B16_sse(s16 *coeff, s16 *block, int shift, int line, int skip_line, int skip_line_2);
void xeve_itrans_ats_intra_DCT8_B32_sse(s16 *coeff, s16 *block, int shift, int line, int skip_line, int skip_line_2);
#endif /* X86_SSE */
#endif /* _XEVE_ITDQ_SSE_H_ */

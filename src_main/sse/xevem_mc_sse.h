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

#ifndef _XEVEM_MC_SSE_H_
#define _XEVEM_MC_SSE_H_

#if X86_SSE
extern const XEVEM_MC xeve_tbl_dmvr_mc_l_sse[2][2];
extern const XEVEM_MC xeve_tbl_dmvr_mc_c_sse[2][2];
extern const XEVEM_MC xeve_tbl_bl_mc_l_sse[2][2];


void xevem_scaled_horizontal_sobel_filter_sse(pel *pred, int pred_stride, int *derivate, int derivate_buf_stride, int width, int height);
void xevem_scaled_vertical_sobel_filter_sse(pel *pred, int pred_stride, int *derivate, int derivate_buf_stride, int width, int height);
void xevem_equal_coeff_computer_sse(pel *residue, int residue_stride, int **derivate, int derivate_buf_stride, s64(*equal_coeff)[7], int width, int height, int vertex_num);
#endif /* X86_SSE */

#endif /* _XEVEM_MC_SSE_H_ */

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

#ifndef _XEVE_IPRED_H_
#define _XEVE_IPRED_H_

#include "xeve_def.h"

void xeve_get_nbr(int  x,
                  int  y,
                  int  cuw,
                  int  cuh,
                  pel *src,
                  int  s_src,
                  u16  avail_cu,
                  pel  nb[N_C][N_REF][MAX_CU_SIZE * 3],
                  int  scup,
                  u32 *map_scu,
                  int  w_scu,
                  int  h_scu,
                  int  ch_type,
                  int  constrained_intra_pred,
                  u8  *map_tidx,
                  int  bit_depth,
                  int  chroma_format_idc);
void xeve_ipred(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int ipm, int w, int h);
void xeve_ipred_uv(pel *src_le, pel *src_up, pel *src_ri, u16 avail_lr, pel *dst, int ipm_c, int ipm, int w, int h);
void xeve_get_mpm(int  x_scu,
                  int  y_scu,
                  int  cuw,
                  int  cuh,
                  u32 *map_scu,
                  s8  *map_ipm,
                  int  scup,
                  int  w_scu,
                  u8 **mpm,
                  u8  *map_tidx);

#endif /* _XEVE_IPRED_H_ */

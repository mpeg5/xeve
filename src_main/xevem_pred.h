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

#ifndef _XEVEM_PRED_H_
#define _XEVEM_PRED_H_

#include "xevem_type.h"

/* Intra prediction */
int xevem_pintra_create(XEVE_CTX * ctx, int complexity);

/* Inter prediction */
int xevem_pinter_create(XEVE_CTX * ctx, int complexity);

/* IBC prediction */
#define GET_BV_COST(ctx, mv_bits)  ((u32)(core->sqrt_lambda[0] * mv_bits / 65536.0))

u32 get_bv_cost_bits(int mv_x, int mv_y);
int xevem_pibc_create(XEVE_CTX * ctx, int complexity);

void reset_ibc_search_range(XEVE_CTX *ctx, int cu_x, int cu_y, int log2_cuw, int log2_cuh, XEVE_CORE * core);
int is_bv_valid(XEVE_CTX *ctx, int x, int y, int width, int height, int log2_cuw, int log2_cuh
                , int pic_width, int pic_height, int x_bv, int y_bv, int ctu_size, XEVE_CORE * core);

#endif /* _XEVEM_PRED_H_ */

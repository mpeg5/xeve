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

#ifndef _XEVE_ITDQ_H_
#define _XEVE_ITDQ_H_

#include "xeve_def.h"

// clang-format off

#define ITX_SHIFT1                            (7)                     /* shift after 1st IT stage */
#define ITX_SHIFT2(bit_depth)                 (12 - (bit_depth - 8))  /* shift after 2nd IT stage */

#define ITX_CLIP(x) \
    (s16)(((x)<MIN_TX_VAL)? MIN_TX_VAL: (((x)>MAX_TX_VAL)? MAX_TX_VAL: (x)))

#define MAX_TX_DYNAMIC_RANGE_32               31
#define MAX_TX_VAL_32                       2147483647
#define MIN_TX_VAL_32                      (-2147483647-1)
#define ITX_CLIP_32(x) \
    (s32)(((x)<=MIN_TX_VAL_32)? MIN_TX_VAL_32: (((x)>=MAX_TX_VAL_32)? MAX_TX_VAL_32: (x)))

// clang-format on

void xeve_itdq(XEVE_CTX* ctx, XEVE_CORE* core, s16 coef[N_C][MAX_CU_DIM], int nnz_sub[N_C][MAX_SUB_TB_NUM]);
void xeve_itx_pb2b (void * src, void * dst, int shift, int line, int step);
void xeve_itx_pb4b (void * src, void * dst, int shift, int line, int step);
void xeve_itx_pb8b (void * src, void * dst, int shift, int line, int step);
void xeve_itx_pb16b(void * src, void * dst, int shift, int line, int step);
void xeve_itx_pb32b(void * src, void * dst, int shift, int line, int step);
void xeve_itx_pb64b(void * src, void * dst, int shift, int line, int step);

extern const XEVE_ITXB xeve_tbl_itxb[MAX_TR_LOG2];

#endif /* _XEVE_ITDQ_H_ */

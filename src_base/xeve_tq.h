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

#ifndef _XEVE_TQ_H_
#define _XEVE_TQ_H_

#include "xeve_type.h"

#define GET_I_COST(rate, lamba)  (rate*lamba)
#define GET_IEP_RATE             (32768)

extern const XEVE_TXB xeve_tbl_txb[MAX_TR_LOG2];
extern const int xeve_quant_scale[2][6];

int xeve_rdoq_set_ctx_cc(XEVE_CORE * core, int ch_type, int prev_level);
int xeve_sub_block_tq(XEVE_CTX * ctx, XEVE_CORE * core, s16 coef[N_C][MAX_CU_DIM], int log2_cuw, int log2_cuh, int slice_type, int nnz[N_C], int is_intra, int run_stats);
int xeve_rdoq_run_length_cc(u8 qp, double d_lambda, u8 is_intra, s16 *src_coef, s16 *dst_tmp, int log2_cuw, int log2_cuh, int ch_type, XEVE_CORE * core, int bit_depth);
void xeve_init_err_scale(XEVE_CTX * ctx);
extern const XEVE_TXB(*xeve_func_txb)[MAX_TR_LOG2];
void tx_pb2b(void* src, void* dst, int shift, int line, int step);
void tx_pb4b(void* src, void* dst, int shift, int line, int step);
void tx_pb8b(void* src, void* dst, int shift, int line, int step);
void tx_pb16b(void* src, void* dst, int shift, int line, int step);
void tx_pb32b(void* src, void* dst, int shift, int line, int step);
void tx_pb64b(void* src, void* dst, int shift, int line, int step);
#endif /* _XEVE_TQ_H_ */

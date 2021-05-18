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

#ifndef _XEVEM_MODE_H_
#define _XEVEM_MODE_H_

#include "xeve_mode.h"

void xeve_rdo_bit_cnt_cu_skip_main(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s32 cup, int mvp_idx0, int mvp_idx1, int c_num, int tool_mmvd);
void xeve_rdo_bit_cnt_affine_mvp(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s8 refi[REFP_NUM], s16 mvd[REFP_NUM][VER_NUM][MV_D], int pidx, int mvp_idx, int vertex_num);
void xeve_rdo_bit_cnt_cu_ibc(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s32 cup, s16 mvd[MV_D], s16 coef[N_C][MAX_CU_DIM], u8 mvp_idx, u8 pred_mode);
void xeve_rdo_bit_cnt_cu_inter_main(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s32 cup, s8 refi[REFP_NUM], s16 mvd[REFP_NUM][MV_D], s16 coef[N_C][MAX_CU_DIM], int pidx, u8 * mvp_idx, u8 mvr_idx, u8 bi_idx, s16 affine_mvd[REFP_NUM][VER_NUM][MV_D]);
void xeve_rdo_bit_cnt_cu_intra_main(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s32 cup, s16 coef[N_C][MAX_CU_DIM]);
void xeve_rdo_bit_cnt_intra_dir_main(XEVE_CTX * ctx, XEVE_CORE * core, int ipm);
void xevem_rdo_bit_cnt_intra_ext(XEVE_CTX * ctx, XEVE_CORE * core);
void xevem_rdo_bit_cnt_intra_ext_c(XEVE_CTX * ctx, XEVE_CORE * core);
void update_history_buffer_affine(XEVE_HISTORY_BUFFER *history_buffer, XEVE_MODE *mi, int slice_type, XEVE_CORE *core);
void mode_reset_intra_main(XEVE_CORE *core);
void xeve_mode_create_main(XEVE_CTX * ctx);
void copy_to_cu_data_main(XEVE_CTX *ctx, XEVE_CORE *core, XEVE_MODE *mi, s16 coef_src[N_C][MAX_CU_DIM]);
void xeve_split_tbl_init(XEVE_CTX *ctx);
void xeve_set_affine_mvf(XEVE_CTX * ctx, XEVE_CORE * core, int w, int h, s8 refi[REFP_NUM], s16 mv[REFP_NUM][VER_NUM][MV_D], int vertex_num);
int  xeve_hmvp_init(XEVE_HISTORY_BUFFER *history_buffer);
void xeve_init_bef_data(XEVE_CORE * core, XEVE_CTX * ctx);
#endif /* _XEVE_MODE_H_ */

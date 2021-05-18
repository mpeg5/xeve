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

#ifndef _XEVE_TBL_H_
#define _XEVE_TBL_H_

#include "xeve_def.h"

extern const u8   xeve_tbl_mpm[6][6][5];
extern const u8   xeve_tbl_log2[257];
extern const s8   xeve_tbl_tm2[2][2];
extern const s8   xeve_tbl_tm4[4][4];
extern const s8   xeve_tbl_tm8[8][8];
extern const s8   xeve_tbl_tm16[16][16];
extern const s8   xeve_tbl_tm32[32][32];
extern const s8   xeve_tbl_tm64[64][64];
extern u16      * xeve_scan_tbl[COEF_SCAN_TYPE_NUM][MAX_CU_LOG2 - 1][MAX_CU_LOG2 - 1];
extern u16      * xeve_inv_scan_tbl[COEF_SCAN_TYPE_NUM][MAX_CU_LOG2 - 1][MAX_CU_LOG2 - 1];

extern const int  xeve_tbl_dq_scale_b[6];
extern const u8   xeve_tbl_df_st[4][52];
extern int        xeve_tbl_qp_chroma_ajudst[MAX_QP_TABLE_SIZE];
extern int        xeve_tbl_qp_chroma_dynamic_ext[2][MAX_QP_TABLE_SIZE_EXT];
extern int      * xeve_qp_chroma_dynamic_ext[2];// pointer to [0th position in xeve_tbl_qp_chroma_dynamic_ext]
extern int      * xeve_qp_chroma_dynamic[2];    // pointer to [12th position in xeve_tbl_qp_chroma_dynamic_ext]
void              xeve_tbl_derived_chroma_qp_mapping(XEVE_CHROMA_TABLE *structChromaQP, int bit_depth);
void              xeve_set_chroma_qp_tbl_loc(int bit_depth);

extern const int  xeve_min_in_group[LAST_SIGNIFICANT_GROUPS];
extern const int  xeve_group_idx[MAX_TR_SIZE];
extern const int  xeve_go_rice_range[MAX_GR_ORDER_RESIDUAL];
extern const int  xeve_go_rice_para_coeff[32];
extern const u8 * xeve_tbl_mv_bits;
extern const u8   xeve_tbl_refi_bits[17][16];
extern const u8   xeve_tbl_mvp_idx_bits[5][4];
extern const int  xeve_quant_scale[6];

#define RATE_TO_COST_LAMBDA(l, r)       ((double)r * l)
#define RATE_TO_COST_SQRT_LAMBDA(l, r)  ((double)r * l)

extern const XEVE_PRESET xeve_tbl_preset[XEVE_PRESET_MAX];

extern const s8 xeve_tbl_slice_depth_P[5][16];
extern const s8 xeve_tbl_slice_depth[5][15];

extern QP_ADAPT_PARAM xeve_qp_adapt_param_ra[8];
extern QP_ADAPT_PARAM xeve_qp_adapt_param_ld[8];
extern QP_ADAPT_PARAM xeve_qp_adapt_param_ai[8];

#endif /* _XEVE_TBL_H_ */

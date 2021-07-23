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

#ifndef _XEVEM_TBL_H_
#define _XEVEM_TBL_H_

#include "xeve_def.h"

extern const u8  xevem_tbl_split_flag_ctx[6][6];
extern const int xevem_tbl_dq_scale[6];
extern const int xevem_tbl_ipred_adi[32][4];
extern const int xevem_tbl_ipred_dxdy[IPD_CNT][2];
extern const int xevem_tbl_qp_chroma_ajudst[XEVE_MAX_QP_TABLE_SIZE];

extern const s16 xevem_init_cbf_luma[2][NUM_CTX_CBF_LUMA];
extern const s16 xevem_init_cbf_cb[2][NUM_CTX_CBF_CR];
extern const s16 xevem_init_cbf_cr[2][NUM_CTX_CBF_CB];
extern const s16 xevem_init_cbf_all[2][NUM_CTX_CBF_ALL];
extern const s16 xevem_init_dqp[2][NUM_CTX_DELTA_QP];
extern const s16 xevem_init_pred_mode[2][NUM_CTX_PRED_MODE];
extern const s16 xevem_init_mode_cons[2][NUM_CTX_MODE_CONS];
extern const s16 xevem_init_direct_mode_flag[2][NUM_CTX_DIRECT_MODE_FLAG];
extern const s16 xevem_init_merge_mode_flag[2][NUM_CTX_MERGE_MODE_FLAG];
extern const s16 xevem_init_inter_dir[2][NUM_CTX_INTER_PRED_IDC];
extern const s16 xevem_init_intra_dir[2][NUM_CTX_INTRA_PRED_MODE];
extern const s16 xevem_init_intra_luma_pred_mpm_flag[2][NUM_CTX_INTRA_LUMA_PRED_MPM_FLAG];
extern const s16 xevem_init_intra_luma_pred_mpm_idx[2][NUM_CTX_INTRA_LUMA_PRED_MPM_IDX];
extern const s16 xevem_init_intra_chroma_pred_mode[2][NUM_CTX_INTRA_CHROMA_PRED_MODE];
extern const s16 xevem_init_mmvd_flag[2][NUM_CTX_MMVD_FLAG];
extern const s16 xevem_init_mmvd_merge_idx[2][NUM_CTX_MMVD_MERGE_IDX];
extern const s16 xevem_init_mmvd_distance_idx[2][NUM_CTX_MMVD_DIST_IDX];
extern const s16 xevem_init_mmvd_direction_idx[2][NUM_CTX_MMVD_DIRECTION_IDX];
extern const s16 xevem_init_mmvd_group_idx[2][NUM_CTX_MMVD_GROUP_IDX];
extern const s16 xevem_init_merge_idx[2][NUM_CTX_MERGE_IDX];
extern const s16 xevem_init_mvp_idx[2][NUM_CTX_MVP_IDX];
extern const s16 xevem_init_affine_mvp_idx[2][NUM_CTX_AFFINE_MVP_IDX];
extern const s16 xevem_init_mvr_idx[2][NUM_CTX_AMVR_IDX];
extern const s16 xevem_init_bi_idx[2][NUM_CTX_BI_PRED_IDX];
extern const s16 xevem_init_mvd[2][NUM_CTX_MVD];
extern const s16 xevem_init_refi[2][NUM_CTX_REF_IDX];
extern const s16 xevem_init_btt_split_flag[2][NUM_CTX_BTT_SPLIT_FLAG];
extern const s16 xevem_init_btt_split_dir[2][NUM_CTX_BTT_SPLIT_DIR];
extern const s16 xevem_init_btt_split_type[2][NUM_CTX_BTT_SPLIT_TYPE];
extern const s16 xevem_init_run[2][NUM_CTX_CC_RUN];
extern const s16 xevem_init_last[2][NUM_CTX_CC_LAST];
extern const s16 xevem_init_level[2][NUM_CTX_CC_LEVEL];
extern const s16 xevem_init_suco_flag[2][NUM_CTX_SUCO_FLAG];
extern const s16 xevem_init_alf_ctb_flag[2][NUM_CTX_ALF_CTB_FLAG];
extern const s16 xevem_init_split_cu_flag[2][NUM_CTX_SPLIT_CU_FLAG];
extern const s16 xevem_init_sig_coeff_flag[2][NUM_CTX_SIG_COEFF_FLAG];
extern const s16 xevem_init_coeff_abs_level_greaterAB_flag[2][NUM_CTX_GTX];
extern const s16 xevem_init_last_sig_coeff_x_prefix[2][NUM_CTX_LAST_SIG_COEFF];
extern const s16 xevem_init_last_sig_coeff_y_prefix[2][NUM_CTX_LAST_SIG_COEFF];
extern const s16 xevem_init_affine_flag[2][NUM_CTX_AFFINE_FLAG];
extern const s16 xevem_init_affine_mode[2][NUM_CTX_AFFINE_MODE];
extern const s16 xevem_init_affine_mrg[2][NUM_CTX_AFFINE_MRG];
extern const s16 xevem_init_affine_mvd_flag[2][NUM_CTX_AFFINE_MVD_FLAG];
extern const s16 xevem_init_skip_flag[2][NUM_CTX_SKIP_FLAG];
extern const s16 xevem_init_ats_intra_cu[2][NUM_CTX_ATS_INTRA_CU_FLAG];
extern const s16 xevem_init_ibc_flag[2][NUM_CTX_IBC_FLAG];
extern const s16 xevem_init_ats_mode[2][NUM_CTX_ATS_MODE_FLAG];
extern const s16 xevem_init_ats_cu_inter_flag[2][NUM_CTX_ATS_INTER_FLAG];
extern const s16 xevem_init_ats_cu_inter_quad_flag[2][NUM_CTX_ATS_INTER_QUAD_FLAG];
extern const s16 xevem_init_ats_cu_inter_hor_flag[2][NUM_CTX_ATS_INTER_HOR_FLAG];
extern const s16 xevem_init_ats_cu_inter_pos_flag[2][NUM_CTX_ATS_INTER_POS_FLAG];

extern const int xevem_tbl_tr_subset_intra[4];

/* Range of intra ATS is 4 to 32 */
extern const s8 xevem_tbl_tr[NUM_TRANS_TYPE][4][1024];
extern const s8 xevem_tbl_inv_tr[NUM_TRANS_TYPE][4][1024];

extern const u8 xevem_addb_alpha_tbl[52];
extern const u8 xevem_addb_beta_tbl[52];
extern const u8 xevem_addb_clip_tbl[52][5];

/* HDR */
extern const int xevem_dra_chroma_qp_offset_tbl[NUM_CHROMA_QP_OFFSET_LOG];
extern const int xevem_dra_exp_nom_v2[NUM_CHROMA_QP_SCALE_EXP];

extern const s8 xevem_tbl_poc_gop_offset[5][15];
extern const s8 xevem_tbl_slice_depth_orig[5][15];

#endif /* _XEVE_TBL_H_ */

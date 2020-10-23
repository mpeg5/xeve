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

#ifndef _XEVEM_ECO_H_
#define _XEVEM_ECO_H_

#include "xeve_def.h"
#include "xevem_type.h"

void xevem_sbac_reset(XEVE_SBAC *sbac, u8 slice_type, u8 slice_qp, int sps_cm_init_flag);
int  xevem_eco_aps_gen(XEVE_BSW * bs, XEVE_APS_GEN * aps, int bit_depth);
int  xevem_eco_sps(XEVE_BSW * bs, XEVE_SPS * sps);
int  xevem_eco_pps(XEVE_BSW * bs, XEVE_SPS * sps, XEVE_PPS * pps);
int  xevem_eco_sh(XEVE_BSW * bs, XEVE_SPS * sps, XEVE_PPS * pps, XEVE_SH * sh, int nut);
int  xevem_eco_split_mode(XEVE_BSW *bs, XEVE_CTX *c, XEVE_CORE *core, int cud, int cup, int cuw, int cuh, int lcu_s, int x, int y);
int  xevem_eco_unit(XEVE_CTX * ctx, XEVE_CORE * core, int x, int y, int cup, int cuw, int cuh, TREE_CONS tree_cons, XEVE_BSW * bs);
int  xevem_eco_intra_dir(XEVE_BSW *bs, u8 ipm, u8 mpm[2], u8 mpm_ext[8], u8 pims[IPD_CNT]);
int  xevem_eco_intra_dir_c(XEVE_BSW *bs, u8 ipm, u8 ipm_l);
void xevem_intra_mode_write_trunc_binary(int symbol, int max_symbol, XEVE_SBAC *sbac, XEVE_BSW *bs);
void xevem_eco_ibc_flag(XEVE_BSW * bs, int flag, int ctx);
int  xevem_eco_mode_constr(XEVE_BSW * bs, MODE_CONS mode_cons, int ctx);
int  xevem_eco_suco_flag(XEVE_BSW *bs, XEVE_CTX *c, XEVE_CORE *core, int cud, int cup, int cuw, int cuh, int lcu_s, s8 split_mode, int boundary, u8 log2_max_cuwh);
int  xevem_eco_mvr_idx(XEVE_BSW *bs, u8 mvr_idx);
int  xevem_eco_merge_idx(XEVE_BSW *bs, int merge_idx);
void xevem_eco_merge_mode_flag(XEVE_BSW *bs, int merge_mode_flag);
int  xevem_eco_bi_idx(XEVE_BSW * bs, u8 bi_idx);
void xevem_eco_mmvd_flag(XEVE_BSW * bs, int flag);
int  xevem_eco_mmvd_info(XEVE_BSW *bs, int mvp_idx, int type);
int  xevem_eco_affine_mvp_idx(XEVE_BSW *bs, int mvp_idx);
void xevem_eco_affine_flag(XEVE_BSW * bs, int flag, int ctx);
void xevem_eco_affine_mode(XEVE_BSW * bs, int flag);
int  xevem_eco_affine_mrg_idx(XEVE_BSW *bs, s16 affine_mrg_idx);
void xevem_eco_affine_mvd_flag(XEVE_BSW *bs, int flag, int refi);
int  xevem_eco_coef_main(XEVE_CTX * ctx, XEVE_CORE * core, XEVE_BSW * bs, s16 coef[N_C][MAX_CU_DIM], u8 pred_mode, int enc_dqp, int b_no_cbf, int run_stats);
void xevem_eco_alf_golomb(XEVE_BSW * bs, int coeff, int k_min_tab, const BOOL signed_coeff);
int  xevem_eco_alf_aps_param(XEVE_BSW * bs, XEVE_APS_GEN * aps);
int  xevem_eco_alf_sh_param(XEVE_BSW * bs, XEVE_SH * sh);
int  xevem_eco_dra_aps_param(XEVE_BSW * bs, XEVE_APS_GEN * aps, int bit_depth);
int  xeve_eco_udata_hdr(XEVE_CTX * ctx, XEVE_BSW * bs, u8 pic_sign[N_C][16]);
int  xeve_eco_pic_signature_main(XEVE_CTX * ctx, XEVE_BSW * bs, u8 pic_sign[N_C][16]);
#if GRAB_STAT
void ence_stat_cu(int x, int y, int cuw, int cuh, int cup, void *ctx, void *core, TREE_CONS tree_cons);
#endif
#endif /* _XEVE_ECO_H_ */

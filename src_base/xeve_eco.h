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

#ifndef _XEVE_ECO_H_
#define _XEVE_ECO_H_

#include "xeve_def.h"
#include "xeve_type.h"

#define GET_SBAC_ENC(bs)   ((XEVE_SBAC *)(bs)->pdata[1])

void sbac_encode_bin_ep(u32 bin, XEVE_SBAC *sbac, XEVE_BSW *bs);
void sbac_encode_bins_ep(u32 value, int num_bin, XEVE_SBAC *sbac, XEVE_BSW *bs);
void sbac_write_truncate_unary_sym(u32 sym, u32 num_ctx, u32 max_num, XEVE_SBAC *sbac, SBAC_CTX_MODEL *model, XEVE_BSW *bs);
void xeve_sbac_reset(XEVE_SBAC * sbac, u8 slice_type, u8 slice_qp, int sps_cm_init_flag);
void xeve_sbac_finish(XEVE_BSW *bs);
void xeve_sbac_encode_bin(u32 bin, XEVE_SBAC *sbac, SBAC_CTX_MODEL *ctx_model, XEVE_BSW *bs);
void xeve_sbac_encode_bin_trm(u32 bin, XEVE_SBAC *sbac, XEVE_BSW *bs);
int xeve_eco_nal_unit_len(void * buf, int size);
int  xeve_eco_nalu(XEVE_BSW * bs, XEVE_NALU * nalu);
int  xeve_eco_sps(XEVE_BSW * bs, XEVE_SPS * sps);
int  xeve_eco_pps(XEVE_BSW * bs, XEVE_SPS * sps, XEVE_PPS * pps);
int  xeve_eco_sh(XEVE_BSW * bs, XEVE_SPS * sps, XEVE_PPS * pps, XEVE_SH * sh, int nut);
int  xeve_eco_sei(XEVE_CTX * ctx, XEVE_BSW * bs);
int  xeve_eco_emitsei(XEVE_CTX * ctx, XEVE_BSW * bs);
int  xeve_eco_vui(XEVE_BSW * bs, XEVE_VUI * vui);
int  xeve_eco_signature(XEVE_CTX * ctx, XEVE_BSW * bs);
int  xeve_eco_pic_signature(XEVE_CTX * ctx, XEVE_BSW * bs, u8 pic_sign[N_C][16]);
int  xeve_eco_pred_mode(XEVE_BSW * bs, u8 pred_mode, int ctx);
int  xeve_eco_intra_dir(XEVE_BSW *bs, u8 ipm, u8 * mpm);
void xeve_eco_direct_mode_flag(XEVE_BSW *bs, int direct_mode_flag);
void xeve_eco_skip_flag(XEVE_BSW * bs, int flag, int ctx);
int  xeve_eco_mvp_idx(XEVE_BSW *bs, int mvp_idx);
void xeve_eco_inter_pred_idc(XEVE_BSW * bs, s8 refi[REFP_NUM], int slice_type, int cuw, int cuh, int is_sps_admvp);
int  xeve_eco_mvd(XEVE_BSW * bs, s16 mvd[MV_D]);
int  xeve_eco_refi(XEVE_BSW * bs, int num_refp, int refi);
int  xeve_eco_dqp(XEVE_BSW * bs, int ref_qp, int cur_qp);
int  xeve_eco_split_mode(XEVE_BSW *bs, XEVE_CTX *c, XEVE_CORE *core, int cud, int cup, int cuw, int cuh, int lcu_s, int x, int y);
void xeve_eco_tile_end_flag(XEVE_BSW * bs, int flag);
int  cu_init(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int cup, int cuw, int cuh);
void coef_rect_to_series(XEVE_CTX * ctx, s16 *coef_src[N_C], int x, int y, int cuw, int cuh, s16 coef_dst[N_C][MAX_CU_DIM], XEVE_CORE * core);
int  xeve_eco_coef(XEVE_CTX * ctx, XEVE_CORE * core, XEVE_BSW * bs, s16 coef[N_C][MAX_CU_DIM], u8 pred_mode, int enc_dqp, int b_no_cbf, int run_stats);
void xeve_eco_run_length_cc(XEVE_CTX * ctx, XEVE_BSW *bs, s16 *coef, int log2_w, int log2_h, int num_sig, int ch_type);
int  xeve_eco_cbf(XEVE_BSW * bs, int cbf_y, int cbf_u, int cbf_v, u8 pred_mode, int b_no_cbf, int is_sub, int sub_pos, int cbf_all, int run[N_C], TREE_CONS tree_cons, int chroma_format_idc);
int  xeve_eco_unit(XEVE_CTX * ctx, XEVE_CORE * core, int x, int y, int cup, int cuw, int cuh, TREE_CONS tree_cons, XEVE_BSW * bs);

#endif /* _XEVE_ECO_H_ */

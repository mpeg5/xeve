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

#ifndef __XEVEM_UTIL_H__
#define __XEVEM_UTIL_H__

#include "xevem_type.h"

#define ALLOW_SPLIT_RATIO(long_side, block_ratio) (block_ratio <= BLOCK_14 && (long_side <= ctx->split_check[block_ratio][IDX_MAX] && long_side >= ctx->split_check[block_ratio][IDX_MIN]) ? 1 : 0)
#define ALLOW_SPLIT_TRI(long_side) ((long_side <= ctx->split_check[BLOCK_TT][IDX_MAX] && long_side >= ctx->split_check[BLOCK_TT][IDX_MIN]) ? 1 : 0)
void xeve_check_split_mode(XEVE_CTX * ctx, int *split_allow, int log2_cuw, int log2_cuh, int boundary, int boundary_r, int log2_max_cuwh
                         , int x, int y, int im_w, int im_h, int sps_btt_flag, MODE_CONS mode_cons);
u16  xeve_get_avail_ibc(int x_scu, int y_scu, int w_scu, int h_scu, int scup, int cuw, int cuh, u32 * map_scu, u8* map_tidx);
void xeve_get_default_motion_main(int neb_addr[MAX_NUM_POSSIBLE_SCAND], int valid_flag[MAX_NUM_POSSIBLE_SCAND], s8 cur_refi, int lidx, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], s8 *refi, s16 mv[MV_D]
                                , u32 *map_scu, s16(*map_unrefined_mv)[REFP_NUM][MV_D], int scup, int w_scu, XEVE_HISTORY_BUFFER * history_buffer, int hmvp_flag);
void xevem_get_motion_merge(int poc, int slice_type, int scup, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], XEVE_REFP refp[REFP_NUM], int cuw, int cuh, int w_scu, int h_scu
                              , s8 refi[REFP_NUM][MAX_NUM_MVP], s16 mvp[REFP_NUM][MAX_NUM_MVP][MV_D], u32 *map_scu, u16 avail_lr, s16(*map_unrefined_mv)[REFP_NUM][MV_D]
                              , XEVE_HISTORY_BUFFER *history_buffer, u8 ibc_flag, XEVE_REFP(*refplx)[REFP_NUM], XEVE_SH* sh, int log2_max_cuwh, u8 *map_tidx);
void xeve_get_motion_from_mvr(u8 mvr_idx, int poc, int scup, int lidx, s8 cur_refi, int num_refp, s16(*map_mv)[REFP_NUM][MV_D], s8(*map_refi)[REFP_NUM], XEVE_REFP(*refp)[REFP_NUM]
                            , int cuw, int cuh, int w_scu, int h_scu, u16 avail, s16 mvp[MAX_NUM_MVP][MV_D], s8 refi_pred[MAX_NUM_MVP], u32* map_scu, u16 avail_lr, s16(*map_unrefined_mv)[REFP_NUM][MV_D]
                            , XEVE_HISTORY_BUFFER * history_buffer, int hmvp_flag, u8* map_tidx);

//! Get array of split modes tried sequentially in RDO
void xeve_split_get_split_rdo_order(int cuw, int cuh, SPLIT_MODE splits[MAX_SPLIT_NUM]);
//! Get SUCO partition order
void xeve_split_get_suco_order(int suco_flag, SPLIT_MODE mode, int suco_order[SPLIT_MAX_PART_COUNT]);
//! Count of partitions, correspond to split_mode
int  xeve_split_part_count(int split_mode);
//! Get partition size
int  xeve_split_get_part_size(int split_mode, int part_num, int length);
//! Get partition size log
int  xeve_split_get_part_size_idx(int split_mode, int part_num, int length_idx);
//! Get partition split structure
void xeve_split_get_part_structure_main(int split_mode, int x0, int y0, int cuw, int cuh, int cup, int cud, int log2_culine, XEVE_SPLIT_STRUCT* split_struct);
//! Get split direction. Quad will return vertical direction.
SPLIT_DIR xeve_split_get_direction(SPLIT_MODE mode);
//! Check that mode is vertical
int  xeve_split_is_vertical(SPLIT_MODE mode);
//! Check that mode is horizontal
int  xeve_split_is_horizontal(SPLIT_MODE mode);
//! Is mode triple tree?
int  xeve_split_is_TT(SPLIT_MODE mode);
//! Is mode BT?
int  xeve_split_is_BT(SPLIT_MODE mode);

int  xeve_get_suco_flag(s8* suco_flag, int cud, int cup, int cuw, int cuh, int lcu_s, s8(*suco_flag_buf)[NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU]);
void xeve_set_suco_flag(s8  suco_flag, int cud, int cup, int cuw, int cuh, int lcu_s, s8(*suco_flag_buf)[NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU]);
u8   xeve_check_suco_cond(int cuw, int cuh, s8 split_mode, int boundary, u8 log2_max_cuwh, u8 log2_min_cuwh, u8 suco_max_depth, u8 suco_depth);
void xeve_mv_rounding_s32( s32 hor, int ver, s32 * rounded_hor, s32 * rounded_ver, s32 right_shift, int left_shift );
void xeve_rounding_s32(s32 comp, s32 *rounded_comp, int right_shift, int left_shift);
void derive_affine_subblock_size_bi( s16 ac_mv[REFP_NUM][VER_NUM][MV_D], s8 refi[REFP_NUM], int cuw, int cuh, int *sub_w, int *sub_h, int vertex_num, BOOL*mem_band_conditions_for_eif_are_satisfied);
void derive_affine_subblock_size( s16 ac_mv[VER_NUM][MV_D], int cuw, int cuh, int *sub_w, int *sub_h, int vertex_num, BOOL*mem_band_conditions_for_eif_are_satisfied);
BOOL check_eif_applicability_bi( s16 ac_mv[REFP_NUM][VER_NUM][MV_D], s8 refi[REFP_NUM], int cuw, int cuh, int vertex_num, BOOL* mem_band_conditions_are_satisfied);
BOOL check_eif_applicability_uni( s16 ac_mv[VER_NUM][MV_D], int cuw, int cuh, int vertex_num, BOOL* mem_band_conditions_are_satisfied);
void xeve_get_affine_motion_scaling(int poc, int scup, int lidx, s8 cur_refi, int num_refp,s16(*map_mv)[REFP_NUM][MV_D], s8(*map_refi)[REFP_NUM], XEVE_REFP(*refp)[REFP_NUM]
                                  , int cuw, int cuh, int w_scu, int h_scu, u16 avail, s16 mvp[MAX_NUM_MVP][VER_NUM][MV_D], s8 refi[MAX_NUM_MVP]
                                  , u32* map_scu, u32* map_affine, int vertex_num, u16 avail_lr, int log2_max_cuwh, s16(*map_unrefined_mv)[REFP_NUM][MV_D], u8* map_tidx);
int  xeve_get_affine_merge_candidate(int poc, int slice_type, int scup, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], XEVE_REFP(*refp)[REFP_NUM], int cuw, int cuh, int w_scu, int h_scu
                                   , u16 avail, s8 mrg_list_refi[AFF_MAX_CAND][REFP_NUM], s16 mrg_list_cp_mv[AFF_MAX_CAND][REFP_NUM][VER_NUM][MV_D], int mrg_list_cp_num[AFF_MAX_CAND]
                                   , u32* map_scu, u32* map_affine, int log2_max_cuwh, s16(*map_unrefined_mv)[REFP_NUM][MV_D], u16 avail_lr, XEVE_SH * sh, u8 * map_tidx);
void xeve_get_ctx_last_pos_xy_para(int ch_type, int width, int height, int *result_offset_x, int *result_offset_y, int *result_shift_x, int *result_shift_y);
int  xeve_get_ctx_sig_coeff_inc(s16 *pcoeff, int blkpos, int width, int height, int ch_type);
int  xeve_get_ctx_gtA_inc(s16 *pcoeff, int blkpos, int width, int height, int ch_type);
int  xeve_get_ctx_gtB_inc(s16 *pcoeff, int blkpos, int width, int height, int ch_type);
int  get_rice_para(s16 *pcoeff, int blkpos, int width, int height, int base_level);
void xeve_eco_sbac_ctx_initialize(SBAC_CTX_MODEL *ctx, s16 *ctx_init_model, u16 num_ctx, u8 slice_type, u8 slice_qp);
u8   check_ats_inter_info_coded(int cuw, int cuh, int pred_mode, int tool_ats);
void get_ats_inter_trs(u8 ats_inter_info, int log2_cuw, int log2_cuh, u8* ats_cu, u8* ats_mode);
u8   xeve_check_chroma_split_allowed(int luma_width, int luma_height);
u8   xeve_is_chroma_split_allowed(int w, int h, SPLIT_MODE split);
enum TQC_RUN xeve_get_run(enum TQC_RUN run_list, TREE_CONS tree_cons);
void get_tu_pos_offset(u8 ats_inter_info, int log2_cuw, int log2_cuh, int* x_offset, int* y_offset);
void get_tu_size(u8 ats_inter_info, int log2_cuw, int log2_cuh, int* log2_tuw, int* log2_tuh);
void set_cu_cbf_flags(u8 cbf_y, u8 ats_inter_info, int log2_cuw, int log2_cuh, u32 *map_scu, int w_scu);

XEVEM_CTX  * xevem_ctx_alloc(void);
XEVEM_CORE * xevem_core_alloc(int chroma_format_idc);
int  xevem_set_init_param(XEVE_CTX * ctx, XEVE_PARAM * param);
void xevem_set_sps(XEVE_CTX * ctx, XEVE_SPS * sps);
void xevem_set_pps(XEVE_CTX * ctx, XEVE_PPS * pps);
void xevem_set_sh(XEVE_CTX *ctx, XEVE_SH *sh);
void xevem_pocs(XEVE_CTX * ctx, u32 pic_imcnt, int gop_size, int pos);
int  xevem_set_tile_info(XEVE_CTX * ctx);
int  xevem_ready(XEVE_CTX * ctx);
void xevem_flush(XEVE_CTX * ctx);
int xevem_pic(XEVE_CTX * ctx, XEVE_BITB * bitb, XEVE_STAT * stat);
int  xevem_header(XEVE_CTX * ctx);
int  xevem_pic_prepare(XEVE_CTX * ctx, XEVE_BITB * bitb, XEVE_STAT * stat);
int  xevem_init_core_mt(XEVE_CTX * ctx, int tile_num, XEVE_CORE * core, int thread_cnt);
int  xevem_loop_filter(XEVE_CTX * ctx, XEVE_CORE * core);
void xevem_recon(XEVE_CTX * ctx, XEVE_CORE * core, s16 *coef, pel *pred, int is_coef, int cuw, int cuh, int s_rec, pel *rec, int bit_depth);
void xevem_pic_filt(XEVE_CTX * ctx, XEVE_IMGB * img);
void xevem_platform_init_func();
int  xevem_platform_init(XEVE_CTX * ctx);
void xevem_platform_deinit(XEVE_CTX * ctx);
int  xevem_encode_sps(XEVE_CTX * ctx);
int  xevem_encode_pps(XEVE_CTX * ctx);
int  xevem_encode_sei(XEVE_CTX * ctx);
int  xevem_encode_aps(XEVE_CTX * ctx, XEVE_APS_GEN * aps);
#if GRAB_STAT
void enc_stat_header(int pic_w, int pic_h);
#endif
#endif /* __XEVE_UTIL_H__ */


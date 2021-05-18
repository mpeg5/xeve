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

#ifndef _XEVE_MODE_H_
#define _XEVE_MODE_H_

void       xeve_pic_expand(XEVE_CTX *ctx, XEVE_PIC *pic);
XEVE_PIC * xeve_pic_alloc(PICBUF_ALLOCATOR *pa, int *ret);
void       xeve_pic_free(PICBUF_ALLOCATOR *pa, XEVE_PIC *pic);

void xeve_bsw_skip_slice_size(XEVE_BSW *bs);
int  xeve_bsw_write_nalu_size(XEVE_BSW *bs);

void xeve_diff_pred(int x, int y, int log2_cuw, int log2_cuh, XEVE_PIC *org, pel pred[N_C][MAX_CU_DIM], s16 diff[N_C][MAX_CU_DIM], int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc);

#define SBAC_STORE(dst, src) xeve_mcpy(&dst, &src, sizeof(XEVE_SBAC))
#define SBAC_LOAD(dst, src)  xeve_mcpy(&dst, &src, sizeof(XEVE_SBAC))
#define DQP_STORE(dst, src) xeve_mcpy(&dst, &src, sizeof(XEVE_DQP))
#define DQP_LOAD(dst, src)  xeve_mcpy(&dst, &src, sizeof(XEVE_DQP))
void xeve_set_qp(XEVE_CTX *ctx, XEVE_CORE *core, u8 qp);

MODE_CONS xeve_derive_mode_cons(XEVE_CTX *ctx, int luc_num, int cup);

int  xeve_mode_create(XEVE_CTX * ctx, int complexity);
void xeve_rdo_bit_cnt_cu_intra(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s32 cup, s16 coef[N_C][MAX_CU_DIM]);
void xeve_rdo_bit_cnt_cu_intra_luma(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s32 cup, s16 coef[N_C][MAX_CU_DIM]);
void xeve_rdo_bit_cnt_cu_intra_chroma(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s32 cup, s16 coef[N_C][MAX_CU_DIM]);
void xeve_rdo_bit_cnt_cu_inter(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s32 cup, s8 refi[REFP_NUM], s16 mvd[REFP_NUM][MV_D], s16 coef[N_C][MAX_CU_DIM], int pidx, u8 * mvp_idx, u8 mvr_idx, u8 bi_idx, s16 affine_mvd[REFP_NUM][VER_NUM][MV_D]);
void xeve_rdo_bit_cnt_cu_inter_comp(XEVE_CORE * core, s16 coef[N_C][MAX_CU_DIM], int ch_type, int pidx, XEVE_CTX * ctx, TREE_CONS tree_cons);
void xeve_rdo_bit_cnt_cu_skip(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s32 cup, int mvp_idx0, int mvp_idx1, int c_num, int tool_mmvd);
void xeve_rdo_bit_cnt_mvp(XEVE_CTX * ctx, XEVE_CORE * core, s32 slice_type, s8 refi[REFP_NUM], s16 mvd[REFP_NUM][MV_D], int pidx, int mvp_idx);
void xeve_rdo_bit_cnt_intra_dir(XEVE_CTX * ctx, XEVE_CORE * core, int ipm);

void xeve_sbac_bit_reset(XEVE_SBAC * sbac);
u32  xeve_get_bit_number(XEVE_SBAC * sbac);
void xeve_init_bits_est();
u16  xeve_get_lr(u16 avail_lr);
void calc_delta_dist_filter_boundary(XEVE_CTX* ctx, XEVE_PIC *pic_rec, XEVE_PIC *pic_org, int cuw, int cuh, pel(*src)[MAX_CU_DIM], int s_src, int x, int y, u16 avail_lr
                                   , u8 intra_flag, u8 cbf_l, s8 *refi, s16(*mv)[MV_D], u8 is_mv_from_mvf, XEVE_CORE * core);
void copy_to_cu_data(XEVE_CTX *ctx, XEVE_CORE *core, XEVE_MODE *mi, s16 coef_src[N_C][MAX_CU_DIM]);
int mode_cu_init(XEVE_CTX * ctx, XEVE_CORE * core, int x, int y, int log2_cuw, int log2_cuh, int cud);
void update_map_scu(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int src_cuw, int src_cuh);
void clear_map_scu(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int cuw, int cuh);
double mode_check_inter(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh, int cud, XEVE_MODE *mi, double cost_best);
double mode_check_intra(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh, int cud, XEVE_MODE *mi, double cost_best);

int check_nev_block(XEVE_CTX *ctx, int x0, int y0, int log2_cuw, int log2_cuh, int *do_curr, int *do_split, int cud, int *nbr_map_skip_flag, XEVE_CORE * core);
int init_cu_data(XEVE_CU_DATA *cu_data, int log2_cuw, int log2_cuh, int qp_y, int qp_u, int qp_v);
void get_min_max_qp(XEVE_CTX * ctx, XEVE_CORE *core, s8 * min_qp, s8 * max_qp, int * is_dqp_set, SPLIT_MODE split_mode, int cuw, int cuh, u8 qp, int x0, int y0);
void set_lambda(XEVE_CTX* ctx, XEVE_CORE * core, XEVE_SH* sh, s8 qp);
int copy_cu_data(XEVE_CU_DATA *dst, XEVE_CU_DATA *src, int x, int y, int log2_cuw, int log2_cuh, int log2_cus, int cud, TREE_CONS tree_cons, int chroma_format_idc);
void mode_cpy_rec_to_ref(XEVE_CORE *core, int x, int y, int w, int h, XEVE_PIC *pic, TREE_CONS tree_cons, int chroma_format_idc);
int get_cu_pred_data(XEVE_CU_DATA *src, int x, int y, int log2_cuw, int log2_cuh, int log2_cus, int cud, XEVE_MODE *mi, XEVE_CTX *ctx, XEVE_CORE *core);

int xeve_mode_init_mt(XEVE_CTX *ctx, int tile_idx);
int mode_init_lcu(XEVE_CTX *ctx, XEVE_CORE *core);
void update_to_ctx_map(XEVE_CTX *ctx, XEVE_CORE *core);

#endif /* _XEVE_MODE_H_ */

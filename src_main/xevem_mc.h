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

#ifndef _XEVEM_MC_H_
#define _XEVEM_MC_H_


extern const s16 xevem_tbl_mc_l_coeff[16][8];
extern const s16 xevem_tbl_mc_c_coeff[32][4];

extern const s16 xeve_tbl_bl_mc_l_coeff[16][2];
extern const s16 tbl_bl_eif_32_phases_mc_l_coeff[32][2];

typedef void (*XEVEM_MC) (pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth);
typedef int  (*XEVE_DMVR_SAD_MR)(int w, int h, void * src1, void * src2, int s_src1, int s_src2, s16 delta);

extern const XEVEM_MC xevem_tbl_dmvr_mc_l[2][2];
extern const XEVEM_MC xevem_tbl_dmvr_mc_c[2][2];
extern const XEVEM_MC xevem_tbl_bl_mc_l[2][2];

extern const XEVEM_MC (*xevem_func_dmvr_mc_l)[2];
extern const XEVEM_MC (*xevem_func_dmvr_mc_c)[2];
extern const XEVEM_MC (*xevem_func_bl_mc_l)[2];

#define xeve_dmvr_mc_l(ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth) \
       (xevem_func_dmvr_mc_l[((gmv_x) | ((gmv_x)>>1) | ((gmv_x)>>2) | ((gmv_x)>>3)) & 0x1])\
        [((gmv_y) | ((gmv_y)>>1) | ((gmv_y)>>2) | ((gmv_y)>>3)) & 0x1]\
        (ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth)

#define xeve_dmvr_mc_c(ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth) \
       (xevem_func_dmvr_mc_c[((gmv_x) | ((gmv_x)>>1) | ((gmv_x)>>2)| ((gmv_x)>>3) | ((gmv_x)>>4)) & 0x1]\
        [((gmv_y) | ((gmv_y)>>1) | ((gmv_y)>>2) | ((gmv_y)>>3) | ((gmv_y)>>4)) & 0x1])\
        (ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth)

#define xeve_bl_mc_l(ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth) \
       (xevem_func_bl_mc_l[((gmv_x) | ((gmv_x)>>1) | ((gmv_x)>>2) | ((gmv_x)>>3)) & 0x1])\
        [((gmv_y) | ((gmv_y)>>1) | ((gmv_y)>>2) | ((gmv_y)>>3)) & 0x1]\
        (ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth)

void xevem_mc(int x, int y, int pic_w, int pic_h, int w, int h, s8 refi[REFP_NUM], s16(*mv)[MV_D], XEVE_REFP(*refp)[REFP_NUM], pel pred[REFP_NUM][N_C][MAX_CU_DIM]
                , int poc_c, pel *dmvr_ref_pred_template, pel dmvr_ref_pred_interpolated[REFP_NUM][(MAX_CU_SIZE + ((DMVR_NEW_VERSION_ITER_COUNT + 1) * REF_PRED_EXTENTION_PEL_COUNT)) * (MAX_CU_SIZE + ((DMVR_NEW_VERSION_ITER_COUNT + 1) * REF_PRED_EXTENTION_PEL_COUNT))]
                , pel dmvr_half_pred_interpolated[REFP_NUM][(MAX_CU_SIZE + 1) * (MAX_CU_SIZE + 1)], BOOL apply_DMVR, pel dmvr_padding_buf[REFP_NUM][N_C][PAD_BUFFER_STRIDE * PAD_BUFFER_STRIDE]
                , u8 *cu_dmvr_flag, s16 dmvr_mv[MAX_CU_CNT_IN_LCU][REFP_NUM][MV_D], int sps_admvp_flag, int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc);

void xeve_IBC_mc(int x, int y, int log2_cuw, int log2_cuh, s16 mv[MV_D], XEVE_PIC *ref_pic, pel pred[N_C][MAX_CU_DIM], TREE_CONS tree_cons, int chroma_format_idc);
void xeve_affine_mc(int x, int y, int pic_w, int pic_h, int w, int h, s8 refi[REFP_NUM], s16 mv[REFP_NUM][VER_NUM][MV_D], XEVE_REFP(*refp)[REFP_NUM]
                  , pel pred[2][N_C][MAX_CU_DIM], int vertex_num, pel* tmp_buffer, int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc);
void xeve_affine_mc_l(int x, int y, int pic_w, int pic_h, int cuw, int cuh, s16 ac_mv[VER_NUM][MV_D], XEVE_PIC* ref_pic, pel pred[MAX_CU_DIM]
                    , int vertex_num, pel* tmp_buffer, int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc);

void xevem_scaled_horizontal_sobel_filter(pel *pred, int pred_stride, int *derivate, int derivate_buf_stride, int width, int height);
void xevem_scaled_vertical_sobel_filter(pel *pred, int pred_stride, int *derivate, int derivate_buf_stride, int width, int height);
void xevem_equal_coeff_computer(pel *residue, int residue_stride, int **derivate, int derivate_buf_stride, s64(*equal_coeff)[7], int width, int height, int vertex_num);

typedef void (*XEVE_AFFINE_H_SOBEL_FLT)(pel *pred, int pred_stride, int *derivate, int derivate_buf_stride, int width, int height);
typedef void (*XEVE_AFFINE_V_SOBEL_FLT)(pel *pred, int pred_stride, int *derivate, int derivate_buf_stride, int width, int height);
typedef void (*XEVE_AFFINE_EQUAL_COEF)(pel *residue, int residue_stride, int **derivate, int derivate_buf_stride, s64(*equal_coeff)[7], int width, int height, int vertex_num);

extern XEVE_AFFINE_H_SOBEL_FLT xevem_func_aff_h_sobel_flt;
extern XEVE_AFFINE_V_SOBEL_FLT xevem_func_aff_v_sobel_flt;
extern XEVE_AFFINE_EQUAL_COEF  xevem_func_aff_eq_coef_comp;

#endif /* _XEVEM_MC_H_ */

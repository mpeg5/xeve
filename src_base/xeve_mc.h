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

#ifndef _XEVE_MC_H_
#define _XEVE_MC_H_

// clang-format off

#define MAC_SFT_N0            (6)
#define MAC_ADD_N0            0

#define MAC_SFT_0N            MAC_SFT_N0
#define MAC_ADD_0N            MAC_ADD_N0

#define MAC_8TAP(c, r0, r1, r2, r3, r4, r5, r6, r7) \
    ((c)[0]*(r0)+(c)[1]*(r1)+(c)[2]*(r2)+(c)[3]*(r3)+(c)[4]*(r4)+\
    (c)[5]*(r5)+(c)[6]*(r6)+(c)[7]*(r7))
#define MAC_8TAP_N0(c, r0, r1, r2, r3, r4, r5, r6, r7) \
    ((MAC_8TAP(c, r0, r1, r2, r3, r4, r5, r6, r7) + MAC_ADD_N0) >> MAC_SFT_N0)
#define MAC_8TAP_0N(c, r0, r1, r2, r3, r4, r5, r6, r7) \
    ((MAC_8TAP(c, r0, r1, r2, r3, r4, r5, r6, r7) + MAC_ADD_0N) >> MAC_SFT_0N)
#define MAC_8TAP_NN_S1(c, r0, r1, r2, r3, r4, r5, r6, r7, offset, shift) \
    ((MAC_8TAP(c,r0,r1,r2,r3,r4,r5,r6,r7) + offset) >> shift)
#define MAC_8TAP_NN_S2(c, r0, r1, r2, r3, r4, r5, r6, r7, offset, shift) \
    ((MAC_8TAP(c,r0,r1,r2,r3,r4,r5,r6,r7) + offset) >> shift)
#define MAC_4TAP(c, r0, r1, r2, r3) \
    ((c)[0]*(r0)+(c)[1]*(r1)+(c)[2]*(r2)+(c)[3]*(r3))
#define MAC_4TAP_N0(c, r0, r1, r2, r3) \
    ((MAC_4TAP(c, r0, r1, r2, r3) + MAC_ADD_N0) >> MAC_SFT_N0)
#define MAC_4TAP_0N(c, r0, r1, r2, r3) \
    ((MAC_4TAP(c, r0, r1, r2, r3) + MAC_ADD_0N) >> MAC_SFT_0N)
#define MAC_4TAP_NN_S1(c, r0, r1, r2, r3, offset, shift) \
    ((MAC_4TAP(c, r0, r1, r2, r3) + offset) >> shift)
#define MAC_4TAP_NN_S2(c, r0, r1, r2, r3, offset, shift) \
    ((MAC_4TAP(c, r0, r1, r2, r3) + offset) >> shift)

#define MAC_BL(c, r0, r1) \
    ((c)[0]*(r0)+(c)[1]*(r1))
#define MAC_BL_N0(c, r0, r1) \
    ((MAC_BL(c, r0, r1) + MAC_ADD_N0) >> MAC_SFT_N0)
#define MAC_BL_0N(c, r0, r1) \
    ((MAC_BL(c, r0, r1) + MAC_ADD_0N) >> MAC_SFT_0N)
#define MAC_BL_NN_S1(c, r0, r1, offset, shift) \
    ((MAC_BL(c, r0, r1) + offset) >> shift)
#define MAC_BL_NN_S2(c, r0, r1, offset, shift) \
    ((MAC_BL(c, r0, r1) + offset) >> shift)


/* padding for store intermediate values, which should be larger than
1+ half of filter tap */
#define MC_IBUF_PAD_C          4
#define MC_IBUF_PAD_L          8
#define MC_IBUF_PAD_BL         2

extern const s16 xeve_tbl_mc_l_coeff[16][8];
extern const s16 xeve_tbl_mc_c_coeff[32][4];

typedef void (*XEVE_MC_L) (pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth, const s16(*mc_l_coeff)[8]);
typedef void (*XEVE_MC_C) (pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth, const s16(*mc_c_coeff)[4]);
typedef void (*XEVE_AVG_NO_CLIP)(s16 *src, s16 *ref, s16 *dst, int s_src, int s_ref, int s_dst, int wd, int ht);

extern const XEVE_MC_L xeve_tbl_mc_l[2][2];
extern const XEVE_MC_C xeve_tbl_mc_c[2][2];

extern const XEVE_MC_L (*xeve_func_mc_l)[2];
extern const XEVE_MC_C (*xeve_func_mc_c)[2];
extern XEVE_AVG_NO_CLIP xeve_func_average_no_clip;

#define xeve_mc_l(ori_mv_x, ori_mv_y, ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth, mc_l_coeff) \
       (xeve_func_mc_l[((ori_mv_x) | ((ori_mv_x)>>1) | ((ori_mv_x)>>2) | ((ori_mv_x)>>3)) & 0x1])\
        [((ori_mv_y) | ((ori_mv_y)>>1) | ((ori_mv_y)>>2) | ((ori_mv_y)>>3)) & 0x1]\
        (ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth, mc_l_coeff)

#define xeve_mc_c(ori_mv_x, ori_mv_y, ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth, mc_c_coeff) \
       (xeve_func_mc_c[((ori_mv_x) | ((ori_mv_x)>>1) | ((ori_mv_x)>>2)| ((ori_mv_x)>>3) | ((ori_mv_x)>>4)) & 0x1]\
        [((ori_mv_y) | ((ori_mv_y)>>1) | ((ori_mv_y)>>2) | ((ori_mv_y)>>3) | ((ori_mv_y)>>4)) & 0x1])\
        (ref, gmv_x, gmv_y, s_ref, s_pred, pred, w, h, bit_depth, mc_c_coeff)

// clang-format on

void xeve_mc(int x, int y, int pic_w, int pic_h, int w, int h, s8 refi[REFP_NUM], s16(*mv)[MV_D], XEVE_REFP(*refp)[REFP_NUM], pel pred[REFP_NUM][N_C][MAX_CU_DIM], int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc);
void xeve_mv_clip(int x, int y, int pic_w, int pic_h, int w, int h, s8 refi[REFP_NUM], s16 mv[REFP_NUM][MV_D], s16(*mv_t)[MV_D]);
void xeve_average_16b_no_clip(s16 *src, s16 *ref, s16 *dst, int s_src, int s_ref, int s_dst, int wd, int ht);
void xeve_mc_l_00(pel *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, pel *pred, int w, int h, int bit_depth, const s16(*mc_l_coeff)[8]);
void xeve_mc_c_00(s16 *ref, int gmv_x, int gmv_y, int s_ref, int s_pred, s16 *pred, int w, int h, int bit_depth, const s16(*mc_c_coeff)[4]);
#endif /* _XEVE_MC_H_ */

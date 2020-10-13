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

#include "xevem_type.h"

#define NA 255 //never split
#define NB 14  //not reach in current setting of max AR 1:4
#define NC 15  //not reach in current setting of max AR 1:4
const u8 xeve_tbl_split_flag_ctx[6][6] = 
{
    { NA,  4,  4, NB, NC, NC },
    { 4,   4,  3,  3,  2,  2 },
    { 4,   3,  3,  2,  2,  1 },
    { NB,  3,  2,  2,  1,  1 },
    { NC,  2,  2,  1,  1,  0 },
    { NC,  2,  1,  1,  0,  0 },
};

const int xeve_tbl_dq_scale[6] = {40, 45, 51, 57, 64, 72};
const int xeve_tbl_ipred_adi[32][4]=
{
    { 32, 64, 32,  0 },
    { 31, 63, 33,  1 },
    { 30, 62, 34,  2 },
    { 29, 61, 35,  3 },
    { 28, 60, 36,  4 },
    { 27, 59, 37,  5 },
    { 26, 58, 38,  6 },
    { 25, 57, 39,  7 },
    { 24, 56, 40,  8 },
    { 23, 55, 41,  9 },
    { 22, 54, 42, 10 },
    { 21, 53, 43, 11 },
    { 20, 52, 44, 12 },
    { 19, 51, 45, 13 },
    { 18, 50, 46, 14 },
    { 17, 49, 47, 15 },
    { 16, 48, 48, 16 },
    { 15, 47, 49, 17 },
    { 14, 46, 50, 18 },
    { 13, 45, 51, 19 },
    { 12, 44, 52, 20 },
    { 11, 43, 53, 21 },
    { 10, 42, 54, 22 },
    {  9, 41, 55, 23 },
    {  8, 40, 56, 24 },
    {  7, 39, 57, 25 },
    {  6, 38, 58, 26 },
    {  5, 37, 59, 27 },
    {  4, 36, 60, 28 },
    {  3, 35, 61, 29 },
    {  2, 34, 62, 30 },
    {  1, 33, 63, 31 },
};

const int xeve_tbl_ipred_dxdy[IPD_CNT][2] = /* {dx/dy, dy/dx} */
{
    { 0,0 },
    { 0,0 },{ 0,0 },{ 2816,372 },{ 2048,512 },{ 1408,744 },
    { 1024,1024 },{ 744,1408 },{ 512,2048 },{ 372,2816 },{ 256,4096 },
    { 128,8192 },{ 0,0 },{ 128,8192 },{ 256,4096 },{ 372,2816 },
    { 512,2048 },{ 744,1408 },{ 1024,1024 },{ 1408,744 },{ 2048,512 },
    { 2816,372 },{ 4096,256 },{ 8192,128 },{ 0,0 },{ 8192,128 },
    { 4096,256 },{ 2816,372 },{ 2048,512 },{ 1408,744 },{ 1024,1024 },
    { 744,1408 },{ 512,2048 },
};

u16 xeve_tbl_split[SPLIT_CHECK_NUM][2];

int xeve_tbl_qp_chroma_ajudst_main[MAX_QP_TABLE_SIZE] =
{
    0,  1,  2,  3,  4,  5,  6,  7,  8,  9,
    10, 11, 12, 13, 14, 15, 16, 17, 18, 19,
    20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
    29, 30, 31, 32, 33, 34, 35, 36, 37, 37,
    38, 39, 40, 40, 41, 42, 43, 44, 45, 46,
    47, 48, 49, 50, 51, 52, 53, 54
};

const s16 init_skip_flag[2][NUM_CTX_SKIP_FLAG] =
{
    {    0,    0, },
    {  711,  233, },
};

const s16 init_ibc_flag[2][NUM_CTX_IBC_FLAG] =
{
    {    0,    0, },
    {  711,  233, },
};

const s16 init_mmvd_flag[2][NUM_CTX_MMVD_FLAG] =
{
    {    0, },
    {  194, },
};

const s16 init_mmvd_merge_idx[2][NUM_CTX_MMVD_MERGE_IDX] =
{
    {    0,    0,    0, },
    {   49,  129,   82, },
};

const s16 init_mmvd_distance_idx[2][NUM_CTX_MMVD_DIST_IDX] =
{
    {    0,    0,    0,    0,    0,    0,    0, },
    {  179,    5,  133,  131,  227,   64,  128, },
};

const s16 init_mmvd_direction_idx[2][NUM_CTX_MMVD_DIRECTION_IDX] =
{
    {    0,    0, },
    {  161,   33, },
};

const s16 init_mmvd_group_idx[2][NUM_CTX_MMVD_GROUP_IDX] =
{
    {    0,    0, },
    {  453,   48, },
};

const s16 init_direct_mode_flag[2][NUM_CTX_DIRECT_MODE_FLAG] =
{
    {    0, },
    {    0, },
};

const s16 init_merge_mode_flag[2][NUM_CTX_MERGE_MODE_FLAG] =
{
    {    0, },
    {  464, },
};

const s16 init_inter_dir[2][NUM_CTX_INTER_PRED_IDC] =
{
    {    0,    0, },
    {  242,   80, },
};

const s16 init_intra_luma_pred_mpm_flag[2][NUM_CTX_INTRA_LUMA_PRED_MPM_FLAG] =
{
    {  263, },
    {  225, },
};

const s16 init_intra_luma_pred_mpm_idx[2][NUM_CTX_INTRA_LUMA_PRED_MPM_IDX] =
{
    {  436, },
    {  724, },
};

const s16 init_intra_chroma_pred_mode[2][NUM_CTX_INTRA_CHROMA_PRED_MODE] =
{
    {  465, },
    {  560, },
};

const s16 init_intra_dir[2][NUM_CTX_INTRA_PRED_MODE] =
{
    {    0,    0, },
    {    0,    0, },
};

const s16 init_pred_mode[2][NUM_CTX_PRED_MODE] =
{
    {   64,    0,    0, },
    {  481,   16,  368, },
};

const s16 init_mode_cons[2][NUM_CTX_MODE_CONS] =
{
    {   64,    0,    0, },
    {  481,   16,  368, },
};

const s16 init_refi[2][NUM_CTX_REF_IDX] =
{
    {    0,    0, },
    {  288,    0, },
};

const s16 init_merge_idx[2][NUM_CTX_MERGE_IDX] =
{
    {    0,    0,    0,  496,  496, },
    {   18,  128,  146,   37,   69, },
};

const s16 init_mvp_idx[2][NUM_CTX_MVP_IDX] =
{
    {    0,    0,    0, },
    {    0,    0,    0, },
};

const s16 init_affine_mvp_idx[2][NUM_CTX_AFFINE_MVP_IDX] =
{
    {    0, },
    {  161, },
};

const s16 init_mvr_idx[2][NUM_CTX_AMVR_IDX] =
{
    {    0,    0,    0,  496, },
    {  773,  101,  421,  199, },
};

const s16 init_bi_idx[2][NUM_CTX_BI_PRED_IDX] =
{
    {    0,    0, },
    {   49,   17, },
};

const s16 init_mvd[2][NUM_CTX_MVD] =
{
    {    0, },
    {   18, },
};

const s16 init_cbf_all[2][NUM_CTX_CBF_ALL] =
{
    {    0, },
    {  794, },
};

const s16 init_cbf_luma[2][NUM_CTX_CBF_LUMA] =
{
    {  664, },
    {  368, },
};

const s16 init_cbf_cb[2][NUM_CTX_CBF_CB] =
{
    {  384, },
    {  416, },
};

const s16 init_cbf_cr[2][NUM_CTX_CBF_CR] =
{
    {  320, },
    {  288, },
};

const s16 init_dqp[2][NUM_CTX_DELTA_QP] =
{
    {    4, },
    {    4, },
};

const s16 init_sig_coeff_flag[2][NUM_CTX_SIG_COEFF_FLAG] =
{
    {  387,   98,  233,  346,  717,  306,  233,   37,  321,  293,  244,   37,  329,  645,  408,  493,  164,  781,  101,  179,  369,  871,  585,  244,  361,  147,  416,  408,  628,  352,  406,  502,  566,  466,   54,   97,  521,  113,  147,  519,   36,  297,  132,  457,  308,  231,  534, },
    {   66,   34,  241,  321,  293,  113,   35,   83,  226,  519,  553,  229,  751,  224,  129,  133,  162,  227,  178,  165,  532,  417,  357,   33,  489,  199,  387,  939,  133,  515,   32,  131,    3,  305,  579,  323,   65,   99,  425,  453,  291,  329,  679,  683,  391,  751,   51, },
};

const s16 init_coeff_abs_level_greaterAB_flag[2][NUM_CTX_GTX] =
{
    {   40,  225,  306,  272,   85,  120,  389,  664,  209,  322,  291,  536,  338,  709,   54,  244,   19,  566, },
    {   38,  352,  340,   19,  305,  258,   18,   33,  209,  773,  517,  406,  719,  741,  613,  295,   37,  498, },
};

const s16 init_last_sig_coeff_x_prefix[2][NUM_CTX_LAST_SIG_COEFF] =
{
    {  762,  310,  288,  828,  342,  451,  502,   51,   97,  416,  662,  890,  340,  146,   20,  337,  468,  975,  216,   66,   54, },
    {  892,   84,  581,  600,  278,  419,  372,  568,  408,  485,  338,  632,  666,  732,   17,  178,  180,  585,  581,   34,  257, },
};

const s16 init_last_sig_coeff_y_prefix[2][NUM_CTX_LAST_SIG_COEFF] =
{
    {   81,  440,    4,  534,  406,  226,  370,  370,  259,   38,  598,  792,  860,  312,   88,  662,  924,  161,  248,   20,   54, },
    {  470,  376,  323,  276,  602,   52,  340,  600,  376,  378,  598,  502,  730,  538,   17,  195,  504,  378,  320,  160,  572, },
};

const s16 init_run[2][NUM_CTX_CC_RUN] =
{
    {   48,  112,  128,    0,  321,   82,  419,  160,  385,  323,  353,  129,  225,  193,  387,  389,  453,  227,  453,  161,  421,  161,  481,  225, },
    {  129,  178,  453,   97,  583,  259,  517,  259,  453,  227,  871,  355,  291,  227,  195,   97,  161,   65,   97,   33,   65,    1, 1003,  227, },
};

const s16 init_last[2][NUM_CTX_CC_LAST] =
{
    {  421,  337, },
    {   33,  790, },
};

const s16 init_level[2][NUM_CTX_CC_LEVEL] =
{
    {  416,   98,  128,   66,   32,   82,   17,   48,  272,  112,   52,   50,  448,  419,  385,  355,  161,  225,   82,   97,  210,    0,  416,  224, },
    {  805,  775,  775,  581,  355,  389,   65,  195,   48,   33,  224,  225,  775,  227,  355,  161,  129,   97,   33,   65,   16,    1,  841,  355, },
};

const s16 init_btt_split_flag[2][NUM_CTX_BTT_SPLIT_FLAG] =
{
    {  145,  560,  528,  308,  594,  560,  180,  500,  626,   84,  406,  662,  320,   36,  340, },
    {  536,  726,  594,   66,  338,  528,  258,  404,  464,   98,  342,  370,  384,  256,   65, },
};

const s16 init_btt_split_dir[2][NUM_CTX_BTT_SPLIT_DIR] =
{
    {    0,  417,  389,   99,    0, },
    {    0,  128,   81,   49,    0, },
};

const s16 init_btt_split_type[2][NUM_CTX_BTT_SPLIT_TYPE] =
{
    {  257, },
    {  225, },
};

const s16 init_affine_flag[2][NUM_CTX_AFFINE_FLAG] =
{
    {    0,    0, },
    {  320,  210, },
};

const s16 init_affine_mode[2][NUM_CTX_AFFINE_MODE] =
{
    {    0, },
    {  225, },
};

const s16 init_affine_mrg[2][NUM_CTX_AFFINE_MRG] =
{
    {    0,    0,    0,    0,    0, },
    {  193,  129,   32,  323,    0, },
};

const s16 init_affine_mvd_flag[2][NUM_CTX_AFFINE_MVD_FLAG] =
{
    {    0,    0, },
    {  547,  645, },
};

const s16 init_suco_flag[2][NUM_CTX_SUCO_FLAG] =
{
    {    0,    0,    0,    0,    0,    0,  545,    0,  481,  515,    0,   32,    0,    0, },
    {    0,    0,    0,    0,    0,    0,  577,    0,  481,    2,    0,   97,    0,    0, },
};

const s16 init_alf_ctb_flag[2][NUM_CTX_ALF_CTB_FLAG] =
{
    {    0, },
    {    0, },
};

const s16 init_split_cu_flag[2][NUM_CTX_SPLIT_CU_FLAG] =
{
    {    0, },
    {    0, },
};

const s16 init_ats_intra_cu[2][NUM_CTX_ATS_INTRA_CU_FLAG] =
{
    {  999, },
    { 1003, },
};

const s16 init_ats_mode[2][NUM_CTX_ATS_MODE_FLAG] =
{
    {  512, },
    {  673, },
};

const s16 init_ats_cu_inter_flag[2][NUM_CTX_ATS_INTER_FLAG] =
{
    {    0,    0, },
    {    0,    0, },
};

const s16 init_ats_cu_inter_quad_flag[2][NUM_CTX_ATS_INTER_QUAD_FLAG] =
{
    {    0, },
    {    0, },
};

const s16 init_ats_cu_inter_hor_flag[2][NUM_CTX_ATS_INTER_HOR_FLAG] =
{
    {    0,    0,    0, },
    {    0,    0,    0, },
};

const s16 init_ats_cu_inter_pos_flag[2][NUM_CTX_ATS_INTER_POS_FLAG] =
{
    {    0, },
    {    0, },
};

s16 xeve_tbl_tr2[NUM_TRANS_TYPE][2][2];
s16 xeve_tbl_tr4[NUM_TRANS_TYPE][4][4];
s16 xeve_tbl_tr8[NUM_TRANS_TYPE][8][8];
s16 xeve_tbl_tr16[NUM_TRANS_TYPE][16][16];
s16 xeve_tbl_tr32[NUM_TRANS_TYPE][32][32];
s16 xeve_tbl_tr64[NUM_TRANS_TYPE][64][64];
s16 xeve_tbl_tr128[NUM_TRANS_TYPE][128][128];

int xeve_tbl_tr_subset_intra[4] = { DST7, DCT8 };

s16 xeve_tbl_inv_tr2[NUM_TRANS_TYPE][2][2];
s16 xeve_tbl_inv_tr4[NUM_TRANS_TYPE][4][4];
s16 xeve_tbl_inv_tr8[NUM_TRANS_TYPE][8][8];
s16 xeve_tbl_inv_tr16[NUM_TRANS_TYPE][16][16];
s16 xeve_tbl_inv_tr32[NUM_TRANS_TYPE][32][32];
s16 xeve_tbl_inv_tr64[NUM_TRANS_TYPE][64][64];
s16 xeve_tbl_inv_tr128[NUM_TRANS_TYPE][128][128];

const u8 addb_alpha_tbl[52] = { 0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,4,4,5,6,  7,8,9,10,12,13,15,17,  20,22,25,28,32,36,40,45,  50,56,63,71,80,90,101,113,  127,144,162,182,203,226,255,255 };
const u8 addb_beta_tbl[52]  = { 0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,2,2,2,3,  3,3,3, 4, 4, 4, 6, 6,   7, 7, 8, 8, 9, 9,10,10,  11,11,12,12,13,13, 14, 14,   15, 15, 16, 16, 17, 17, 18, 18 };
const u8 addb_clip_tbl[52][5] =
{
    { 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },{ 0, 0, 0, 0, 0 },
    { 0, 0, 0, 0, 0 },{ 0, 0, 0, 1, 1 },{ 0, 0, 0, 1, 1 },{ 0, 0, 0, 1, 1 },{ 0, 0, 0, 1, 1 },{ 0, 0, 1, 1, 1 },{ 0, 0, 1, 1, 1 },{ 0, 1, 1, 1, 1 },
    { 0, 1, 1, 1, 1 },{ 0, 1, 1, 1, 1 },{ 0, 1, 1, 1, 1 },{ 0, 1, 1, 2, 2 },{ 0, 1, 1, 2, 2 },{ 0, 1, 1, 2, 2 },{ 0, 1, 1, 2, 2 },{ 0, 1, 2, 3, 3 },
    { 0, 1, 2, 3, 3 },{ 0, 2, 2, 3, 3 },{ 0, 2, 2, 4, 4 },{ 0, 2, 3, 4, 4 },{ 0, 2, 3, 4, 4 },{ 0, 3, 3, 5, 5 },{ 0, 3, 4, 6, 6 },{ 0, 3, 4, 6, 6 },
    { 0, 4, 5, 7, 7 },{ 0, 4, 5, 8, 8 },{ 0, 4, 6, 9, 9 },{ 0, 5, 7,10,10 },{ 0, 6, 8,11,11 },{ 0, 6, 8,13,13 },{ 0, 7,10,14,14 },{ 0, 8,11,16,16 },
    { 0, 9,12,18,18 },{ 0,10,13,20,20 },{ 0,11,15,23,23 },{ 0,13,17,25,25 }
};

int luma_inv_scale_lut[DRA_LUT_MAXSIZE];               // LUT for luma and correspionding QP offset
double chroma_inv_scale_lut[2][DRA_LUT_MAXSIZE];               // LUT for chroma scales 
int int_chroma_inv_scale_lut[2][DRA_LUT_MAXSIZE];               // LUT for chroma scales 

// input to table is in the range 0<input<256, as a result of multiplication of 2 scales with max value of <16.
const int dra_chroma_qp_offset_tbl[NUM_CHROMA_QP_OFFSET_LOG] =  // Approximation of Log function at accuracy 1<<9 bits
{
  0, 1, 1, 1, 1, 1, 2, 2, 3, 4, 4, 6, 7, 9, 11, 14, 18, 23, 29, 36, 45, 
  57, 72, 91, 114, 144, 181, 228, 287, 362, 456, 575, 724, 912, 1149, 1448, 1825, 2299,
  2896, 3649, 4598, 5793, 7298, 9195, 11585, 14596, 18390, 23170, 29193, 36781, 46341, 58386, 73562, 92682, 116772
};

// input to this table is deltaQP introduced to QPi (lumaQP+chromaQPoffset) by the chromaQPOffset table. Currently max offset 6 is supported, increase to 12 (?). 
const int dra_exp_nom_v2[NUM_CHROMA_QP_SCALE_EXP] =   // Approximation of exp function at accuracy 1 << 9 bits
{
    128, 144, 161, 181, 203, 228, 256, 287, 322, 362, 406, 456, 512, 574, 645, 724, 812, 912, 1024, 1149, 1290, 1448, 1625, 1825, 2048
};

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

#ifndef __ADAPTIVELOOPFILTER__
#define __ADAPTIVELOOPFILTER__

#include "xevem_type.h"
#include <float.h>
#include <math.h>

#define CHECK(a,b) assert((!(a)) && (b));
#define DISTORTION_PRECISION_ADJUSTMENT(x)   0
#define ALF_TEMPORAL_WITH_LINE_BUFFER        6 // temporal buffer size

#define NUM_BITS                             10
#define CLASSIFICATION_BLK_SIZE              32  //non-normative, local buffer size
#define FIXED_FILTER_NUM                     64

#define MAX_NUM_ALF_CLASSES                  25
#define MAX_NUM_ALF_LUMA_COEFF               13
#define MAX_NUM_ALF_CHROMA_COEFF             7
#define MAX_ALF_FILTER_LENGTH                7
#define MAX_NUM_ALF_COEFF                    (MAX_ALF_FILTER_LENGTH * MAX_ALF_FILTER_LENGTH / 2 + 1)
#define ALF_FIXED_FILTER_NUM                 16

typedef u8 ALF_CLASSIFIER;

extern const int alf_fixed_filter_coef[FIXED_FILTER_NUM][13];
extern const int alf_class_to_filter_mapping[MAX_NUM_ALF_CLASSES][ALF_FIXED_FILTER_NUM];

typedef struct AREA
{
    int x;
    int y;
    int width;
    int height;
} AREA;

typedef enum _ALF_FILTER_TYPE
{
    ALF_FILTER_5,
    ALF_FILTER_7,
    ALF_NUM_OF_FILTER_TYPES
} ALF_FILTER_TYPE;

enum DIRECTION
{
    HOR,
    VER,
    DIAG0,
    DIAG1,
    NUM_DIRECTIONS
};

typedef struct CLIP_RANGE
{
  int min;
  int max;
  int bd;
  int n;
} CLIP_RANGE;

typedef struct CLIP_RNAGES
{
  CLIP_RANGE comp[N_C]; ///< the bit depth as indicated in the SPS
  BOOL used;
  BOOL chroma;
} CLIP_RNAGES;

static __inline int clip_pel (const int a, const CLIP_RANGE clip_range)
{ 
    return XEVE_CLIP3(clip_range.min, clip_range.max, a);
}

typedef struct CODING_STRUCTURE
{
    void    * ctx;
    XEVE_PIC * pic;

    int temp_stride; //to pass strides easily
    int pic_stride;
} CODING_STRUCTURE;

static const int pattern5[25] =
{
          0,
          1,  2,  3,
          4,  5,  6,  5,  4,
          3,  2,  1,
          0
};

static const int pattern7[25] =
{
      0,
      1,  2,  3,
      4,  5,  6,  7,  8,
      9, 10, 11, 12, 11, 10, 9,
      8,  7,  6,  5,  4,
      3,  2,  1,
      0
};

static const int weights5[14] =
{
      2,
      2, 2, 2,
      2, 2, 1, 1
};

static const int weights7[14] =
{
  2,
  2,  2,  2,
  2,  2,  2,  2,  2,
  2,  2,  2,  1,  1
};

static const int golombIdx5[14] =
{
  0,
  0, 1, 0,
  0, 1
};

static const int golombIdx7[14] =
{
  0,
  0, 1, 0,
  0, 1, 2, 1, 0,
  0, 1, 2
};

static const int pattern_to_large_filter5[13] =
{
  0,
  0, 1, 0,
  0, 2, 3, 4, 0,
  0, 5, 6, 7
};

static const int pattern_to_large_filter7[13] =
{
  1,
  2, 3, 4,
  5, 6, 7, 8, 9,
  10,11,12,13
};

typedef struct _ALF_FILTER_SHAPE
{
    int filter_type;
    int filterLength;
    int num_coef;
    int filter_size;
    int pattern[25];
    int weights[14];
    int golombIdx[14];
    int pattern_to_large_filter[13];

} ALF_FILTER_SHAPE;

struct _ALF_SLICE_PARAM
{
    BOOL                is_ctb_alf_on;
    u8                * alf_ctb_flag;
    BOOL                enable_flag[N_C];                                        // alf_slice_enable_flag, alf_chroma_idc
    ALF_FILTER_TYPE     luma_filter_type;                                        // filter_type_flag
    BOOL                chroma_ctb_present_flag;                                 // alf_chroma_ctb_present_flag
    short               luma_coef[MAX_NUM_ALF_CLASSES * MAX_NUM_ALF_LUMA_COEFF]; // alf_coeff_luma_delta[i][j]
    short               chroma_coef[MAX_NUM_ALF_CHROMA_COEFF];                   // alf_coeff_chroma[i]
    short               filter_coef_delta_idx[MAX_NUM_ALF_CLASSES];              // filter_coeff_delta[i]
    BOOL                filter_coef_flag[MAX_NUM_ALF_CLASSES];                   // filter_coefficient_flag[i]
    int                 num_luma_filters;                                        // number_of_filters_minus1 + 1
    BOOL                coef_delta_flag;                                         // alf_coefficients_delta_flag
    BOOL                coef_delta_pred_mode_flag;                               // coeff_delta_pred_mode_flag
    ALF_FILTER_SHAPE (* filterShapes)[2];

    int                 fixed_filter_pattern;                                    // 0: no pred from pre-defined filters; 1: all are predicted but could be different values; 2: some predicted and some not
                                                                                 // when ALF_LOWDELAY is 1, fixed_filter_pattern 0: all are predected, fixed_filter_pattern 1: some predicted and some not
    int                 fixed_filter_idx[MAX_NUM_ALF_CLASSES];
    u8                  fixed_filter_usage_flag[MAX_NUM_ALF_CLASSES];
    int                 t_layer;
    BOOL                temporal_alf_flag;                                       // indicate whether reuse previous ALF coefficients
    int                 prev_idx;                                                // index of the reused ALF coefficients
    int                 prev_idx_comp[NUM_CH];
    BOOL                reset_alf_buf_flag;
    BOOL                store2_alf_buf_flag; 
    u32                 filter_poc;                                              // store POC value for which filter was produced
    u32                 min_idr_poc;                                             // Minimal of 2 IDR POC available for current coded nalu  (to identify availability of this filter for temp prediction)
    u32                 max_idr_poc;                                             // Max of 2 IDR POC available for current coded nalu  (to identify availability of this filter for temp prediction)
    BOOL                chroma_filter_present;
};

struct _ADAPTIVE_LOOP_FILTER
{
    short               coef_final[MAX_NUM_ALF_CLASSES * MAX_NUM_ALF_LUMA_COEFF];
    int                 input_bit_depth[NUM_CH];
    ALF_SLICE_PARAM     ac_alf_line_buf[APS_MAX_NUM];
    u8                  alf_idx_in_scan_order[APS_MAX_NUM];
    u8                  next_free_alf_idx_in_buf;
    u32                 first_idx_poc;
    u32                 last_idr_poc;
    u32                 curr_poc;
    u32                 curr_temp_layer;
    u32                 i_period;
    int                 alf_present_idr;
    int                 alf_idx_idr;
    u8                  ac_alf_line_buf_curr_size;
    pel               * temp_buf, * temp_buf1, * temp_buf2;
    int                 pic_width;
    int                 pic_height;
    int                 max_cu_width;
    int                 max_cu_height;
    int                 max_cu_depth;
    int                 num_ctu_in_widht;
    int                 num_ctu_in_height;
    int                 num_ctu_in_pic;
    ALF_CLASSIFIER   ** classifier;
    ALF_CLASSIFIER   ** classifier_mt;
    int                 chroma_format;
    int                 last_ras_poc;
    BOOL                pending_ras_init;
    u8                * ctu_enable_flag[N_C];
    CLIP_RNAGES         clip_ranges;
    BOOL                strore2_alf_buf_flag;
    BOOL                reset_alf_buf_flag;
    ALF_FILTER_SHAPE    filter_shapes[NUM_CH][2];

    void              (* derive_classification_blk)( ALF_CLASSIFIER** classifier, const pel * src_luma, const int src_stride, const AREA * blk, const int shift, int bit_depth );
    void              (* filter_5x5_blk)( ALF_CLASSIFIER** classifier, pel * rec_dst, const int dst_stride, const pel * rec_src, const int src_stride, const AREA* blk, const u8 comp_id, short* filter_set, const CLIP_RANGE* clip_range );
    void              (* filter_7x7_blk)( ALF_CLASSIFIER** classifier, pel * rec_dst, const int dst_stride, const pel * rec_src, const int src_stride, const AREA* blk, const u8 comp_id, short* filter_set, const CLIP_RANGE* clip_range );
};

int alf_create(ADAPTIVE_LOOP_FILTER * alf, const int pic_width, const int pic_height, const int max_cu_width, const int max_cu_height, const int max_cu_depth, const int chroma_format_idc, int bit_depth);
void alf_destroy(ADAPTIVE_LOOP_FILTER * alf);
void alf_init(ADAPTIVE_LOOP_FILTER * alf, int bit_depth);
void alf_copy_and_extend_tile(pel* tmp_yuv, const int s, const pel* rec_yuv, const int s2, const int w, const int h, const int m);
void alf_copy_and_extend(pel* tmp_yuv, const int s, const pel* rec_yuv, const int s2, const int w, const int h, const int m);
void alf_init_filter_shape(ALF_FILTER_SHAPE * filter_shape, int size);
int  alf_get_max_golomb_idx(ALF_FILTER_TYPE filter_type);
void alf_recon_coef(ADAPTIVE_LOOP_FILTER * alf, ALF_SLICE_PARAM* alf_slice_param, int channel, const BOOL is_rdo, const BOOL is_re_do);
void alf_derive_classification(ADAPTIVE_LOOP_FILTER * alf, ALF_CLASSIFIER** classifier, const pel * src_luma, const int src_luma_stride, const AREA * blk);
void alf_copy_param(ALF_SLICE_PARAM* dst, ALF_SLICE_PARAM* src);
void alf_param_chroma(ALF_SLICE_PARAM* dst, ALF_SLICE_PARAM* src);
void alf_store_paramline_from_aps(ADAPTIVE_LOOP_FILTER * alf, ALF_SLICE_PARAM* pAlfParam, u8 idx);
void alf_load_paramline_from_aps_buffer(ADAPTIVE_LOOP_FILTER * alf, ALF_SLICE_PARAM* pAlfParam, u8 idx);
void alf_load_paramline_from_aps_buffer2(ADAPTIVE_LOOP_FILTER * alf, ALF_SLICE_PARAM* pAlfParam, u8 idxY, u8 idxUV, u8 alf_chroma_idc);
void alf_reset_param(ALF_SLICE_PARAM* dst);
void alf_reset_idr_idx_list_buf_aps(ADAPTIVE_LOOP_FILTER * alf);
int  alf_get_protect_idx_from_list(ADAPTIVE_LOOP_FILTER * alf, int idx);
void alf_store_enc_alf_param_line_aps(ADAPTIVE_LOOP_FILTER * alf, ALF_SLICE_PARAM* pAlfParam, unsigned t_layer);
void alf_derive_classification_blk(ALF_CLASSIFIER** classifier, const pel * src_luma, const int src_stride, const AREA * blk, const int shift, int bit_depth);
void alf_filter_blk_7(ALF_CLASSIFIER** classifier, pel * rec_dst, const int dst_stride, const pel * rec_src, const int src_stride, const AREA* blk, const u8 comp_id, short* filter_set, const CLIP_RANGE* clip_range);
void alf_filter_blk_5(ALF_CLASSIFIER** classifier, pel * rec_dst, const int dst_stride, const pel * rec_src, const int src_stride, const AREA* blk, const u8 comp_id, short* filter_set, const CLIP_RANGE* clip_range);

typedef struct _ALF_COVARIANCE
{
    int num_coef;
    double *y;
    double **E;
    double pix_acc;
} ALF_COVARIANCE;

int alf_cov_create(ALF_COVARIANCE* alf_cov, int size);
void alf_cov_destroy(ALF_COVARIANCE* alf_cov);
void alf_cov_reset(ALF_COVARIANCE* alf_cov);
void alf_cov_copy(ALF_COVARIANCE* dst, ALF_COVARIANCE* src);
void alf_cov_add_to(ALF_COVARIANCE* dst, const ALF_COVARIANCE* lhs, const ALF_COVARIANCE* rhs);
void alf_cov_add(ALF_COVARIANCE* dst, const ALF_COVARIANCE* src);
void alf_cov_minus(ALF_COVARIANCE* dst, const ALF_COVARIANCE* src);

//for 4:2:0 only
typedef struct _YUV {
    pel* yuv[3];
    int s[3];
} YUV;

struct _XEVE_ALF
{
    ADAPTIVE_LOOP_FILTER   alf;
    ALF_COVARIANCE     *** alf_cov[N_C];          // [compIdx][shapeIdx][ctbAddr][class_idx]
    ALF_COVARIANCE      ** alf_cov_frame[N_C + 1];   // [CHANNEL][shapeIdx][class_idx]
    u8*                    ctu_enable_flag_temp[N_C];
    u8*                    ctu_enable_flag_temp_luma;

    ALF_SLICE_PARAM        alf_slice_param_temp;
    ALF_COVARIANCE         alf_cov_merged[ALF_NUM_OF_FILTER_TYPES][MAX_NUM_ALF_CLASSES + 1];
    XEVE_CORE            * core;

    double                 lambda[N_C];
    double                 frac_bits_scale;
    double                 cost_alf_encoder[N_C];

    int                  * filter_coef_quant;
    int                 ** filter_coef_set;
    int                 ** dif_filter_coef;
    int                    k_min_tab[MAX_NUM_ALF_LUMA_COEFF];
    int                    bits_coef_scan[MAX_SCAN_VAL][MAX_EXP_GOLOMB];
    short                  filter_indices[MAX_NUM_ALF_CLASSES][MAX_NUM_ALF_CLASSES];
};

int        xevem_alf_aps(XEVE_CTX * ctx, XEVE_PIC * pic, XEVE_SH* sh, XEVE_APS* aps);
XEVE_ALF * xeve_alf_create_buf(int bit_depth);
void       xeve_alf_delete_buf(XEVE_ALF * enc_alf);
void       xeve_alf_set_reset_alf_buf_flag(XEVE_ALF * enc_anf, int flag);
u8         xeve_alf_aps_get_current_alf_idx(XEVE_ALF * enc_anf);
int       xeve_alf_aps_enc_opt_process(XEVE_ALF * enc_anf, const double* lambdas, XEVE_CTX * ctx, XEVE_PIC * pic, XEVE_ALF_SLICE_PARAM * input_alf_slice_param);
int        xeve_alf_create(XEVE_ALF * enc_alf, const int pic_width, const int pic_height, const int max_cu_width, const int max_cu_height, const int max_cu_depth, const int chroma_format_idc, int bit_depth);
void       xeve_alf_destroy(XEVE_ALF * enc_alf);
void       xeve_alf_process(XEVE_ALF * enc_alf, CODING_STRUCTURE * cs, const double *lambdas, ALF_SLICE_PARAM* alf_slice_param);
double     xeve_alf_derive_ctb_enable_flags(XEVE_ALF * enc_alf, CODING_STRUCTURE * cs, const int input_shape_idx, u8 channel, const int num_classes, const int num_coef, double* dist_unfilter, BOOL rec_coef);
void       xeve_alf_encode(XEVE_ALF * enc_alf, CODING_STRUCTURE * cs, ALF_SLICE_PARAM* alf_slice_param, const int channel);
int        xeve_alf_recon(XEVE_ALF * enc_alf, CODING_STRUCTURE * cs, ALF_SLICE_PARAM* alf_slice_param, const pel * org_unit_buf, const int org_stride, pel * rec_ext_buf, const int rec_stride, const u8 comp_id, int tile_idx, int col_bd2);
void       xeve_alf_temporal_enc_aps_comp(XEVE_ALF * enc_alf, CODING_STRUCTURE * cs, ALF_SLICE_PARAM* alf_slice_param);
void       xeve_alf_derive_cov_from_ltap_filter(ALF_COVARIANCE* cov_large, ALF_COVARIANCE* cov_small, int * pattern_small, ALF_FILTER_TYPE luma_filter_type);
void       xeve_alf_copy_slice_param(XEVE_ALF * enc_alf, ALF_SLICE_PARAM* alf_slice_param_dst, ALF_SLICE_PARAM* alf_slice_param_src, int channel);
double     xeve_alf_get_filter_coef_cost(XEVE_ALF * enc_alf, CODING_STRUCTURE * cs, double dist_unfilter, u8 channel, BOOL is_re_collect_stat, int input_shape_idx, int* input_coef_bits, u8* filter_conformance_flag);
void       xeve_alf_get_filter_coef_cost_ch(XEVE_ALF * enc_alf, CODING_STRUCTURE * cs, double dist_unfilter, u8 channel, int input_shape_idx, int* input_coef_bits, double* filter_cost);
int        xeve_alf_get_coef_rate(XEVE_ALF * enc_alf, ALF_SLICE_PARAM* alf_slice_param, BOOL is_chroma);
double     xeve_alf_get_unfiltered_dist(ALF_COVARIANCE* cov, const int num_classes);
double     xeve_alf_get_unfiltered_dist_ch(ALF_COVARIANCE* cov, int channel);
double     xeve_alf_get_filtered_dist(XEVE_ALF * enc_alf, ALF_COVARIANCE* cov, const int num_classes, const int num_filters_minus1, const int num_coef);
void       xeve_alf_conformance_check(XEVE_ALF * enc_alf, ALF_SLICE_PARAM* alf_slice_param, u8* filter_conformance_flag);
double     xeve_alf_merge_filters_cost(XEVE_ALF * enc_alf, ALF_SLICE_PARAM* alf_slice_param, ALF_FILTER_SHAPE* alf_shape, ALF_COVARIANCE* cov_frame, ALF_COVARIANCE* cov_merged, int* input_coef_bits, u8* filter_conformance_flag);
int        xeve_alf_get_non_filter_coef_rate(ALF_SLICE_PARAM* alf_slice_param);
int        xeve_alf_lenth_truncated_unary(int symbol, int max_symbol);
int        xeve_alf_get_cost_filter_coef_force0(XEVE_ALF * enc_alf, ALF_FILTER_SHAPE* alf_shape, int **diff_q_filter_coef, const int num_filters, BOOL* coded_var_bins);
int        xeve_alf_derive_filter_coef_pred_mode(XEVE_ALF * enc_alf, ALF_FILTER_SHAPE* alf_shape, int **filter_set, int** filterCoeffDiff, const int num_filters, int* predMode);
int        xeve_alf_get_cost_filter_coef(XEVE_ALF * enc_alf, ALF_FILTER_SHAPE* alf_shape, int **diff_q_filter_coef, const int num_filters);
int        xeve_alf_length_filter_coef(ALF_FILTER_SHAPE* alf_shape, const int num_filters, int **filter_coef, int* k_min_tab);
double     xeve_alf_get_dist_force0(XEVE_ALF * enc_alf, ALF_FILTER_SHAPE* alf_shape, const int num_filters, double err_tab_force0_coef[MAX_NUM_ALF_CLASSES][2], BOOL* coded_var_bins);
double     xeve_alf_get_dist_coef_force0(XEVE_ALF * enc_alf, BOOL* coded_var_bins, double err_force0_coef_tab[MAX_NUM_ALF_CLASSES][2], int* bits_var_bin, const int num_filters);
int        xeve_alf_lenght_uvlc(int code);
int        xeve_alf_get_golomb_k_min(ALF_FILTER_SHAPE* alf_shape, const int num_filters, int k_min_tab[MAX_NUM_ALF_LUMA_COEFF], int bits_coef_scan[MAX_SCAN_VAL][MAX_EXP_GOLOMB]);
int        xeve_alf_length_golomb(int coef_val, int k, BOOL signed_coeff);
double     xeve_alf_derive_filter_coef(XEVE_ALF * enc_alf, ALF_COVARIANCE* cov, ALF_COVARIANCE* cov_merged, ALF_FILTER_SHAPE* alf_shape, short* filter_indices, int num_filters, double err_tab_force0_coef[MAX_NUM_ALF_CLASSES][2]);
double     xeve_alf_derive_coef_quant(int *filter_coef_quant, double **E, double *y, const int num_coef, int* weights, const int bit_depth, const BOOL is_chroma /*= FALSE*/);
double     xeve_alf_calc_err_coef(double **E, double *y, const int *coeff, const int num_coef, const int bit_depth);
void       xeve_alf_round_filt_coef(int *filter_coef_quant, double *filterCoeff, const int num_coef, const int factor);
void       xeve_alf_find_best_fixed_filter(ALF_SLICE_PARAM* alf_slice_param, ALF_COVARIANCE* cov);
void       xeve_alf_merge_classes(ALF_COVARIANCE* cov, ALF_COVARIANCE* cov_merged, const int num_classes, short filter_indices[MAX_NUM_ALF_CLASSES][MAX_NUM_ALF_CLASSES]);
void       xeve_alf_get_frame_stats(XEVE_ALF * enc_alf, u8 channel, int input_shape_idx);
void       xeve_alf_get_frame_stat(XEVE_ALF * enc_alf, ALF_COVARIANCE* frame_cov, ALF_COVARIANCE** ctb_cov, u8* ctb_enable_flags, const int num_classes);
void       xeve_alf_derive_stats_filtering(XEVE_ALF * enc_alf, YUV * orgYuv, YUV * rec);
void       xeve_alf_get_blk_stats(int ch, ALF_COVARIANCE* alf_cov, const ALF_FILTER_SHAPE* shape, ALF_CLASSIFIER** classifier, pel* org, const int org_stride, pel* rec, const int rec_stride, const int x, const int y, const int width, const int height);
void       xeve_alf_clac_covariance(int *ELocal, const pel *rec, const int stride, const int *filter_pattern, const int half_filter_length, const int trans_idx);
double     xeve_alf_clac_err(ALF_COVARIANCE* cov);
void       xeve_alf_set_enable_flag(ALF_SLICE_PARAM* alf_slice_param, u8 comp_id, BOOL val);
void       xeve_alf_set_enable_ctb_flag(XEVE_ALF * enc_alf, ALF_SLICE_PARAM* alf_slice_param, u8 comp_id, u8** ctu_flags);
void       xeve_alf_copy_ctb_enable_flag(XEVE_ALF * enc_alf, u8** ctu_flags_dst, u8** ctu_flags_src, u8 comp_id);
void       xeve_alf_set_ctb_enable_flag(XEVE_ALF * enc_alf, u8** ctu_flags, u8 comp_id, u8 val);
// Cholesky decomposition
int        xeve_alf_gns_cholesky_dec(double **input_matr, double out_matr[MAX_NUM_ALF_COEFF][MAX_NUM_ALF_COEFF], int num_eq);
void       xeve_alf_gns_transpose_back_substitution(double U[MAX_NUM_ALF_COEFF][MAX_NUM_ALF_COEFF], double* rhs, double* x, int order);
void       xeve_alf_gns_back_substitution(double R[MAX_NUM_ALF_COEFF][MAX_NUM_ALF_COEFF], double* z, int size, double* A);
int        xeve_alf_gns_solve_chol(double **LHS, double *rhs, double *x, int num_eq);
void       tile_boundary_check(int* avail_left, int* avail_right, int* avail_top, int* avail_bottom, const int width, const int height, int x_pos, int y_pos, int x_l, int x_r, int y_l, int y_r);
#endif

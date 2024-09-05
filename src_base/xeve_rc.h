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

#include "xeve_type.h"
#include "xeve_mc.h"

#ifndef _XEVE_RC_H_
#define _XEVE_RC_H_

// clang-format off

/*****************************************************************************
* rate control structure for RC parameter
*****************************************************************************/
struct _XEVE_RC_PARAM
{
    int   rc_blk_wh;
    int   lcu_depth;
    int   intra_depth;
    int   init_qp;
    float intra_rate_ratio;
    float inter_rate_ratio;
    float pow_cplx;
    float qf_diff_lim_frm;
    float df_qp_sum;
    float df_cplx_sum;
    float max_frm_bits_per_br;
    float vbv_buf_of_rate;
    float vbv_buf_of_rate_fcst;
    float vbv_buf_uf_rate;
    float vbv_buf_uf_rate_fcst;
    float max_vbv_rate_frm;
    float qf_limit_vbv;
    float cpx_thd_resolution;
    float thd_sc;
    float blank_sc_cplx_ftr;
    float intra_qf_thd;
    float prev_q_factor;
    int   sub_pic_penalty;
    float aq_strength;
    int   aq_log2_blk_size;
    float aq_mode_str;
    float lcu_tree_str;
};

/*****************************************************************************
* rate control structure for encoding
*****************************************************************************/
struct _XEVE_RCORE
{
    u16        * pred;

    /* qf value limitation parameter */
    double       qf_limit;
    /* offset btw I and P frame */
    double       offset_ip;
    /* minimum qfactor by frame type */
    double       qf_min[RC_NUM_SLICE_TYPE];
    /* maximum qfactor by frame type */
    double       qf_max[RC_NUM_SLICE_TYPE];
    /* current frame scene_type which is inherited from frame analysis */
    int          scene_type;
    /* current frame qp */
    double       qp;
    /* complexity for current frame (mad) */
    s32          cpx_frm;
    /* complexity for rc model update */
    double       cpx_pow;
    /* estimated bits (restore for update) */
    double       est_bits;
    /* real bits (restore for update) */
    double       real_bits;
    /* slice type    (restore for update) */
    int          stype;
    /* slice dpeth   (restore for update) */
    int          sdepth;
    int          avg_dqp;
    /* use filler for write extra byte */
    int          filler_byte;

    /* Bits amortization after I slice and scenecuts */
    int          amortize_flag;
    int          amortized_frames;
    int          residue_cost;
};

/*****************************************************************************
*rate control model structure
*****************************************************************************/
typedef struct _XEVE_RCM
{
    /* bit per second */
    double       bitrate;
    /* sum of k_param (bits*qfactor/rc_avg_cpx) */
    double       k_param;
    /* accumulated target bitrate * window */
    double       target_bits;
    /* sum of qp to get I frame qfactor */
    double       qp_sum;
    /* count of qp to get I frame qfactor */
    double       qp_cnt;
    /* sum of complexity */
    double       cpx_sum;
    /* count of complexity */
    double       cpx_cnt;
    /* bpf decayed weight factor */
    double       bpf_decayed;
}XEVE_RCM;

/*****************************************************************************
* rate control structure
*****************************************************************************/
struct _XEVE_RC
{
    /* frame per second */
    double       fps;
    /* bit per second */
    double       bitrate;
    /* allocated bits per frame (bitrate/fps)*/
    double       bpf;
    /* allocated bits per frame as TID (bitrate/fps)*/
    double       bpf_tid[10];
    /* maximum bit size for one frame encoding */
    double       max_frm_bits;
    /* vbv enabled flag */
    int          vbv_enabled;
    /* total vbv buffer size */
    double       vbv_buf_size;
    double       lambda[4];
    /* accumulated frame size for each slice type */
    s64          frame_bits;
    XEVE_RCM   * rcm;
    XEVE_RCM     rc_model[RC_NUM_SLICE_TYPE];
    /* Rate Control Bits Predictor structure */
    XEVE_RCBE    bit_estimator[RC_NUM_SLICE_TYPE];
    /* amount of vbv buffer fullness */
    double       vbv_buf_fullness;
    /* store slice type of last and previous of last picture I, P slice type
    0 : last picture
    1 : previous of last picture                                           */
    int          prev_st[2];
    /* store qf of last and previous of last picture forI, P slice type
    0 : last picture
    1 : previous of last picture                                           */
    double       prev_qf[2][RC_NUM_SLICE_TYPE];
    /* store poc of last and previous of last picture for I, P slice type
    0 : last picture
    1 : previous of last picture                                           */
    int          prev_picnt[2][RC_NUM_SLICE_TYPE];

    s64          total_frames;
    int          fps_idx;
    double       prev_bpf;
    int          st_idx;
    int          prev_adpt;
    /* access type */
    int          encoding_mode;
    int          scene_cut;
    double       basecplx;

    const XEVE_RC_PARAM * param;
};

enum ACCESS_TYPE 
{
    XEVE_RA,
    XEVE_AI,
    XEVE_LD
};

enum RC_TYPE
{
    RC_OFF,
    RC_CBR_FIXED_HIERARCHY,
    RC_CRF,
    RC_CBR_EQUAL,
};

/* Define the Search Range for int-pel */
#define SEARCH_RANGE_IPEL       64
/* Define the Search Range for int-pel of bi-prediction */
#define SEARCH_RANGE_BIPEL      4
/* Define the Search Range for sub-pel ME */
#define SEARCH_RANGE_SPEL       3
/* initial direction of diamond searhc pattern */
#define NEXT_POS                2
/* max sub block count in CU */
#define MAX_SUB_CNT             4
#define FIRST_SEARCH_NUM        4
#define NEXT_SEARCH_NUM         3
#define MAX_COST_RC             1<<30
#define MAX_INTRA_PERIOD_RC     1<<30

/* Max. and min. QP for Rate control clipping */
#define RC_QP_MAX                   (MAX_QUANT - 1)
#define RC_QP_MIN                   (MIN_QUANT + 1)

// clang-format on

int  xeve_rc_create(XEVE_CTX * ctx);
int  xeve_rc_delete(XEVE_CTX * ctx);
s32  xeve_rc_set(XEVE_CTX *ctx);
s32  xeve_rc_rcore_set(XEVE_CTX * ctx);
void xeve_rc_update_frame(XEVE_CTX *ctx, XEVE_RC * rc, XEVE_RCORE * rcore);
s32  xeve_rc_get_frame_qp(XEVE_CTX *ctx);
int  xeve_rc_get_qp(XEVE_CTX *ctx);
#endif
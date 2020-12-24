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

/*****************************************************************************
* rate control structure for RC parameter
*****************************************************************************/
struct _XEVE_RC_PARAM
{
    float rc_blk_wh;
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

extern XEVE_RC_PARAM tbl_rc_param;

enum SCENE_TYPE
{
    SCENE_NORMAL,
    SCENE_HIGH,
    SCENE_LOW,
    SCENE_EX_LOW,
};

enum PREV_PIC
{
    PREV0,
    PREV1
};

enum PRED_TYPE
{
    INTRA,
    INTER_UNI0,
    INTER_UNI1,
    INTER_UNI2,
    INTER_L0 = 1,
    INTER_L1 = 2,
    INTER_BI = 3,
};

/* modulo pico idx in pico_buf */
#define MOD_IDX(num, mod)  (((num) + (mod)) % (mod))

/* clipping transfer cost */
#define CLIP_ADD(a,b)                  (XEVE_MIN((a)+(b),0xffff))

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

int  xeve_rc_create(XEVE_CTX * ctx);
int  xeve_rc_delete(XEVE_CTX * ctx);
s32  xeve_rc_set(XEVE_CTX *ctx);
s32  xeve_rc_rcore_set(XEVE_CTX * ctx);
s32  xeve_rc_frame_est(XEVE_CTX * ctx);
void xeve_rc_update_frame(XEVE_CTX *ctx, XEVE_RC * rc, XEVE_RCORE * rcore);
s32  xeve_rc_get_frame_qp(XEVE_CTX *ctx);
void xeve_rc_gen_subpic(pel * src_y, pel * dst_y, int w, int h, int s_s, int d_s, int bit_depth);
int  xeve_rc_get_qp(XEVE_CTX *ctx);
#endif

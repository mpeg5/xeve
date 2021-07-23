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

#ifndef _XEVE_FCST_H_
#define _XEVE_FCST_H_

#include "xeve_def.h"
#include "xeve_type.h"

#define LOG2_AQ_BLK_SIZE               4

/* constant for AQ strength */
#define AQ_STR_CONST                   0.75
#define AQ_STRENGTH                    0.5
/* blk-tree strength */
#define LCU_STRENGTH                   0.75

#define SEARCH_RANGE_IPEL            64
#define INIT_SDS_PTS                 4
/* initial direction of diamond searhc pattern */
#define RC_INIT_QP                   28

/* foracast calculation unit depth
0: same as lcu (depth 0)
1: 1/4 size of lcu (depth 1)
*/

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

enum SCENE_TYPE
{
    SCENE_NORMAL,
    SCENE_HIGH,
    SCENE_LOW,
    SCENE_EX_LOW,
};

enum QPA_TYPE
{
    QPA_OFF,
    QPA_AQ_TREE, /* turn on adaptive qantization  + block tree */
    QPA_AQ,      /* turn on adaptive qantization only */
    QPA_TREE,    /* turn on block tree only */
};


/* check whether B picture could be exist or not */
#define B_PIC_ENABLED(ctx)           (ctx->param.bframes > 0)
/* complexity threthold */

int  xeve_forecast_fixed_gop(XEVE_CTX* ctx);
void xeve_gen_subpic(pel* src_y, pel* dst_y, int w, int h, int s_s, int d_s, int bit_depth);
s32  xeve_fcst_get_scene_type(XEVE_CTX * ctx, XEVE_PICO * pico);

#endif /* _XEVE_FCST_H_ */

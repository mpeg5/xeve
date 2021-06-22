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

#ifndef _XEVE_DRA_H_
#define _XEVE_DRA_H_

#include "xeve_def.h"
#include <stdlib.h>

#define DRA_SCALE_NUMFBITS              9   // # frac. bits for scale (Y/Cb/Cr)
#define DRA_INVSCALE_NUMFBITS           9   // # frac. bits for inv. scale (Y/Cb/Cr)
#define DRA_OFFSET_NUMFBITS             7   // # frac. bits for offset (Y/Cb/Cr)
#define DRA_LUT_MAXSIZE                 1024

#define NUM_CHROMA_QP_OFFSET_LOG        55
#define NUM_CHROMA_QP_SCALE_EXP         25

typedef struct _QUANT_PARAM_DRA {

    int value;  // Currently 32 bit is considered sufficient
    int num_frac_bits;
    int num_tot_bits;
}QUANT_PARAM_DRA;

typedef struct _DRA_CHROMA_OFF_CONTROL
{
    BOOL   enabled;           ///< Enabled flag (0:default)
    double cb_qp_scale;       ///< Chroma Cb QP Scale (1.0:default)
    double cr_qp_scale;       ///< Chroma Cr QP Scale (1.0:default)
    double chroma_qp_scale;   ///< Chroma QP Scale (0.0:default)
    double chroma_qp_offset;  ///< Chroma QP Offset (0.0:default)
    int    dra_table_idx;
    int    dra_cb_qp_offset;
    int    dra_cr_qp_offset;
}DRA_CHROMA_OFF_CONTROL;

typedef struct _DRA_SCALE_MAPPING
{
    double dra_scale_map_y[256][2];          ///< first=luma level, second=delta QP.
} DRA_SCALE_MAPPING;

typedef struct _SIG_PARAM_DRA
{
    int  signal_dra_flag; // flag has 3 positions at encoder: -1: not initialized, 0: initialized and sent, 1: initialized, to be sent
    int  dra_table_idx;
    BOOL equal_ranges_flag;
    int  delta_val;
    int  num_ranges;
    int  in_ranges[33];
    int  dra_descriptor1;
    int  dra_descriptor2;
    int  dra_cb_scale_value;
    int  dra_cr_scale_value;
    int  dra_scale_value[33 - 1];
}SIG_PARAM_DRA;

typedef struct _DRA_CONTROL
{
    BOOL                   flag_enabled;
    DRA_SCALE_MAPPING      dra_scale_map;
    DRA_CHROMA_OFF_CONTROL chroma_qp_model;
    
    //------ Signalled DRA Params ------//
    int           dra_descriptor1;
    int           dra_descriptor2;
    SIG_PARAM_DRA signalled_dra;

    //------ DRA Model ------//
    int    num_ranges;
    int    in_ranges[33];
    double out_ranges[33];
    double dra_scales[33 - 1];
    double dra_offets[33 - 1];

    int    dra_cb_scale_value;
    int    dra_cr_scale_value;
    int    out_ranges_s32[33];
    int    dra_scales_s32[33 - 1];
    int    inv_dra_scales_s32[33 - 1];
    int    inv_dra_offsets_s32[33 - 1];
    int    chroma_dra_scales_s32[2][33 - 1];
    int    chroma_inv_dra_scales_s32[2][33 - 1];

    //------ DRA LUT ------//
    int    luma_scale_lut[DRA_LUT_MAXSIZE];               // LUT for luma and correspionding QP offset
    int    xevem_luma_inv_scale_lut[DRA_LUT_MAXSIZE];               // LUT for luma and correspionding QP offset
    int    int_chroma_scale_lut[2][DRA_LUT_MAXSIZE];               // LUT for chroma scales 
    int    xevem_int_chroma_inv_scale_lut[2][DRA_LUT_MAXSIZE];               // LUT for chroma scales 
                                                                  //------ Gammut mapping ------//
    //------ Adaptive mapping ------//
    double dra_hist_norm;
    int    global_offset;
    int    global_end;

} DRA_CONTROL;

void xeve_init_dra(DRA_CONTROL *dra_mapping, int total_change_points, int *luma_change_points, int* qps, int bit_depth);
int  xeve_analyze_input_pic(XEVE_CTX * ctx, DRA_CONTROL *dra_mapping, int bit_depth);
int  xeve_generate_dra_array(XEVE_CTX * ctx, SIG_PARAM_DRA * dra_control_array, DRA_CONTROL * tmp_dra_control, int num_aps, int bit_depth);

/* DRA APS buffer functions are listed below: */
void xeve_reset_aps_gen_read_buffer(XEVE_APS_GEN *tmp_aps_gen_array);
void xeve_apply_dra_from_array(XEVE_CTX * ctx, XEVE_IMGB * dst, XEVE_IMGB * src, SIG_PARAM_DRA * dra_control_array, int dra_id, int backward_map);

int  xevem_set_active_dra_info(XEVE_CTX * ctx);

#endif 
/* _XEVE_DRA_H_ */

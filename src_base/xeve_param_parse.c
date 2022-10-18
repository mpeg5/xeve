/* Copyright (c) 2022, Samsung Electronics Co., Ltd.
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

#include "xeve_param_parse.h"

#include "xeve.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>

#define PARAMS_END_KEY                      (0)
#define PARAM_STR_MAX_LEN                   (256)

#define OFFSET(x) offsetof(XEVE_PARAM, x)

#define SET_XEVE_PARAM_METADATA( param, data_type) \
    { .name=#param, .type=data_type, .offset=OFFSET(param) }

/* Type of data stored by a given param from XEVE_PARAM stuct */
typedef enum DATA_TYPE {
    DT_INTEGER    = (1 << 0), /* integer type value */
    DT_DOUBLE     = (1 << 1), /* double type value  */
    DT_STRING     = (1 << 2)  /* string type value  */
} DATA_TYPE;

/* Structure for storing metadata for XEVE_PARAM */
typedef struct XEVE_PARAM_METADATA {
    const char* name;   /* text string conneced to param of a given name */
    DATA_TYPE   type;   /* data type for a given param */
    int         offset; /* the offset relative to the XEVE_PARAM structure where the param value is stored */
} XEVE_PARAM_METADATA;

/* Define various command line options as a table */
static const XEVE_PARAM_METADATA xeve_params_metadata[] = {
    SET_XEVE_PARAM_METADATA( profile,                                   DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( threads,                                   DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( w,                                         DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( h,                                         DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( fps,                                       DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( keyint,                                    DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( cs,                                        DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( rc_type,                                   DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( qp,                                        DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( qp_cb_offset,                              DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( qp_cr_offset,                              DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( bitrate,                                   DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( vbv_bufsize,                               DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( crf,                                       DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( bframes,                                   DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( aq_mode,                                   DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( lookahead,                                 DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( closed_gop,                                DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( use_annexb,                                DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( use_filler,                                DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( chroma_qp_table_present_flag,              DT_INTEGER ),

    SET_XEVE_PARAM_METADATA( chroma_qp_num_points_in_table,             DT_STRING ),
    SET_XEVE_PARAM_METADATA( chroma_qp_delta_in_val_cb,                 DT_STRING ),
    SET_XEVE_PARAM_METADATA( chroma_qp_delta_out_val_cb,                DT_STRING ),
    SET_XEVE_PARAM_METADATA( chroma_qp_delta_in_val_cr,                 DT_STRING ),
    SET_XEVE_PARAM_METADATA( chroma_qp_delta_out_val_cr,                DT_STRING ),

    SET_XEVE_PARAM_METADATA( disable_hgop,                              DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( ref_pic_gap_length,                        DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( codec_bit_depth,                           DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( level_idc,                                 DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( cutree,                                    DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( constrained_intra_pred,                    DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( use_deblock,                               DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( inter_slice_type,                          DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( picture_cropping_flag,                     DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( picture_crop_left_offset,                  DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( picture_crop_right_offset,                 DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( picture_crop_top_offset,                   DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( picture_crop_bottom_offset,                DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( rdo_dbk_switch,                            DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( qp_incread_frame,                          DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( sei_cmd_info,                              DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( use_pic_sign,                              DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( f_ifrm,                                    DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( qp_max,                                    DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( qp_min,                                    DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( gop_size,                                  DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( force_output,                              DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( use_fcst,                                  DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( chroma_format_idc,                         DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( cs_w_shift,                                DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( cs_h_shift,                                DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( max_cu_intra,                              DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( min_cu_intra,                              DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( max_cu_inter,                              DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( min_cu_inter,                              DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( ref,                                       DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( me_ref_num,                                DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( me_algo,                                   DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( me_range,                                  DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( me_sub,                                    DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( me_sub_pos,                                DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( me_sub_range,                              DT_INTEGER ),

    SET_XEVE_PARAM_METADATA( skip_th,                                   DT_DOUBLE ),

    SET_XEVE_PARAM_METADATA( merge_num,                                 DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( rdoq,                                      DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( cabac_refine,                              DT_INTEGER ),

/*****************************************************************************
* Main Profile Parameters
*****************************************************************************/
    SET_XEVE_PARAM_METADATA( ibc_flag,                                  DT_INTEGER ),

    SET_XEVE_PARAM_METADATA( ibc_search_range_x,                        DT_INTEGER ),
    SET_XEVE_PARAM_METADATA(  ibc_search_range_y,                       DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( ibc_hash_search_flag,                      DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( ibc_hash_search_max_cand,                  DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( ibc_hash_search_range_4smallblk,           DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( ibc_fast_method,                           DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( toolset_idc_h,                             DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( toolset_idc_l,                             DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( btt,                                       DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( suco,                                      DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( framework_cb_max,                          DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( framework_cb_min,                          DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( framework_cu14_max,                        DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( framework_tris_max,                        DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( framework_tris_min,                        DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( framework_suco_max,                        DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( framework_suco_min,                        DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tool_amvr,                                 DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tool_mmvd,                                 DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tool_affine,                               DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tool_dmvr,                                 DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tool_addb,                                 DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tool_alf,                                  DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tool_htdf,                                 DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tool_admvp,                                DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tool_hmvp,                                 DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tool_eipd,                                 DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tool_iqt,                                  DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tool_cm_init,                              DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tool_adcc,                                 DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tool_rpl,                                  DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tool_pocs,                                 DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( cu_qp_delta_area,                          DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tool_ats,                                  DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( deblock_alpha_offset,                      DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( deblock_beta_offset,                       DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( loop_filter_across_tiles_enabled_flag,     DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tool_dra,                                  DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( dra_enable_flag,                           DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( dra_number_ranges,                         DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( dra_range,                                 DT_STRING ),
    SET_XEVE_PARAM_METADATA( dra_scale,                                 DT_STRING ),
    SET_XEVE_PARAM_METADATA( dra_chroma_qp_scale,                       DT_STRING ),
    SET_XEVE_PARAM_METADATA( dra_chroma_qp_offset,                      DT_STRING ),
    SET_XEVE_PARAM_METADATA( dra_chroma_cb_scale,                       DT_STRING ),
    SET_XEVE_PARAM_METADATA( dra_chroma_cr_scale,                       DT_STRING ),
    SET_XEVE_PARAM_METADATA( dra_hist_norm,                             DT_STRING ),
    SET_XEVE_PARAM_METADATA( tile_uniform_spacing_flag,                 DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tile_columns,                              DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tile_rows,                                 DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tile_column_width_array,                   DT_STRING ),
    SET_XEVE_PARAM_METADATA( tile_row_height_array,                     DT_STRING ),
    SET_XEVE_PARAM_METADATA( num_slice_in_pic,                          DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( tile_array_in_slice,                       DT_STRING ),
    SET_XEVE_PARAM_METADATA( arbitrary_slice_flag,                      DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( num_remaining_tiles_in_slice_minus1,       DT_STRING),
    SET_XEVE_PARAM_METADATA( rpl_extern,                                DT_INTEGER ),

    /* max num of RPL is 32 */
    SET_XEVE_PARAM_METADATA( rpl0,                                      DT_STRING), // char rpl0[32][256];
    SET_XEVE_PARAM_METADATA( rpl1,                                      DT_STRING), // char rpl1[32][256];

    SET_XEVE_PARAM_METADATA( rpls_l0_cfg_num,                           DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( rpls_l1_cfg_num,                           DT_INTEGER ),

    /* preset parameter */
    SET_XEVE_PARAM_METADATA( ats_intra_fast,                            DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( me_fast,                                   DT_INTEGER ),

    /* VUI options*/
    SET_XEVE_PARAM_METADATA( sar,                                       DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( sar_width,                                 DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( sar_height,                                DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( videoformat,                               DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( range,                                     DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( colorprim,                                 DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( transfer,                                  DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( matrix_coefficients,                       DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( overscan_info_present_flag,                DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( overscan_appropriate_flag,                 DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( chroma_loc_info_present_flag,              DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( chroma_sample_loc_type_top_field,          DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( chroma_sample_loc_type_bottom_field,       DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( neutral_chroma_indication_flag,            DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( field_seq_flag,                            DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( timing_info_present_flag,                  DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( num_units_in_tick,                         DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( time_scale,                                DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( fixed_pic_rate_flag,                       DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( nal_hrd_parameters_present_flag,           DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( vcl_hrd_parameters_present_flag,           DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( low_delay_hrd_flag,                        DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( pic_struct_present_flag,                   DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( bitstream_restriction_flag,                DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( motion_vectors_over_pic_boundaries_flag,   DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( max_bytes_per_pic_denom,                   DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( max_bits_per_mb_denom,                     DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( log2_max_mv_length_horizontal,             DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( log2_max_mv_length_vertical,               DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( num_reorder_pics,                          DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( max_dec_pic_buffering,                     DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( aspect_ratio_info_present_flag,            DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( video_signal_type_present_flag,            DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( colour_description_present_flag,           DT_INTEGER ),

    /* SEI options*/
    SET_XEVE_PARAM_METADATA( master_display,                            DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( max_cll,                                   DT_INTEGER ),
    SET_XEVE_PARAM_METADATA( max_fall,                                  DT_INTEGER ),

    /* termination */
    { .name = PARAMS_END_KEY }
};

static int xeve_param_search_name(const char * name)
{
    int idx = 0;
    const XEVE_PARAM_METADATA* param_meta = xeve_params_metadata;

    while(param_meta->name != PARAMS_END_KEY)
    {
        if(!strcmp(name, param_meta->name))
        {
            return idx;
        }
        idx++;
        param_meta++;
    }
    return XEVE_ERR;
}

int xeve_param_set_val(XEVE_PARAM* params, const char* name,  const char* value)
{
    int ival;
    double dval;
    char *endptr;
    const XEVE_PARAM_METADATA* param_meta = xeve_params_metadata;

    int idx = xeve_param_search_name(name);
    if( idx < 0 )
        return XEVE_ERR_INVALID_ARGUMENT;

    param_meta = xeve_params_metadata + idx;

    switch(param_meta->type)
    {
        case DT_INTEGER:
            ival = strtol(value, &endptr, 10);
            if (*endptr != '\0')
                return XEVE_ERR_INVALID_ARGUMENT;

            *((int*)((char*)params + param_meta->offset)) = ival;

            break;
        case DT_DOUBLE:
            dval = strtod(value, &endptr);
            if (*endptr != '\0')
                return XEVE_ERR_INVALID_ARGUMENT;

            *((double*)((char*)params + param_meta->offset)) = dval;

            break;
        case DT_STRING:

            strncpy((char*)((char*)params + param_meta->offset), value, PARAM_STR_MAX_LEN);

            // If PARAM_STR_MAX_LEN is less than or equal to the length of val,
            // a null character (\0) is not appended to the copied string (char*)(args->opts[idx].opt_storage)
            // The line below prevents truncation of destination string to not-null terminated string
            ((char*)((char*)params + param_meta->offset))[PARAM_STR_MAX_LEN-1] = 0;

            break;
        default:
            return XEVE_ERR;
    }

    return XEVE_OK;
}
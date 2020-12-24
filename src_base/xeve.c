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

#include <math.h>
#include "xeve_type.h"
#include "xeve_rc.h"

int last_intra_poc = INT_MAX;
BOOL aps_counter_reset = FALSE;

int  xeve_encode_sps(XEVE_CTX * ctx);
int  xeve_encode_pps(XEVE_CTX * ctx);

/* Convert XEVE into XEVE_CTX */
#define XEVE_ID_TO_CTX_R(id, ctx) \
    xeve_assert_r((id)); \
    (ctx) = (XEVE_CTX *)id; \
    xeve_assert_r((ctx)->magic == XEVE_MAGIC_CODE);

/* Convert XEVE into XEVE_CTX with return value if assert on */
#define XEVE_ID_TO_CTX_RV(id, ctx, ret) \
    xeve_assert_rv((id), (ret)); \
    (ctx) = (XEVE_CTX *)id; \
    xeve_assert_rv((ctx)->magic == XEVE_MAGIC_CODE, (ret));

static const s8 tbl_poc_gop_offset[5][15] =
{
    { -1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  /* gop_size = 2 */
    { -2,   -3,   -1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  /* gop_size = 4 */
    { -4,   -6,   -7,   -5,   -2,   -3,   -1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  /* gop_size = 8 */
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},  /* gop_size = 12 */
    { -8,   -12, -14,  -15,  -13,  -10,  -11,   -9,   -4,   -6,   -7,   -5,   -2,   -3,   -1}   /* gop_size = 16 */
};

static const s8 tbl_slice_depth_P[5][16] =
{
    /* gop_size = 2 */
    { FRM_DEPTH_2, FRM_DEPTH_1, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, \
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    /* gop_size = 4 */
    { FRM_DEPTH_3, FRM_DEPTH_2, FRM_DEPTH_3, FRM_DEPTH_1, 0xFF, 0xFF, 0xFF, 0xFF, \
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    /* gop_size = 8 */
    { FRM_DEPTH_4, FRM_DEPTH_3, FRM_DEPTH_4, FRM_DEPTH_2, FRM_DEPTH_4, FRM_DEPTH_3, FRM_DEPTH_4, FRM_DEPTH_1,\
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    /* gop_size = 12 */
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF},
    /* gop_size = 16 */
    { FRM_DEPTH_5, FRM_DEPTH_4, FRM_DEPTH_5, FRM_DEPTH_3, FRM_DEPTH_5, FRM_DEPTH_4, FRM_DEPTH_5, FRM_DEPTH_2, \
      FRM_DEPTH_5, FRM_DEPTH_4, FRM_DEPTH_5, FRM_DEPTH_3, FRM_DEPTH_5, FRM_DEPTH_4, FRM_DEPTH_5, FRM_DEPTH_1 }
};

static const s8 tbl_slice_depth[5][15] =
{
    /* gop_size = 2 */
    { FRM_DEPTH_2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, \
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    /* gop_size = 4 */
    { FRM_DEPTH_2, FRM_DEPTH_3, FRM_DEPTH_3, 0xFF, 0xFF, 0xFF, 0xFF, \
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    /* gop_size = 8 */
    { FRM_DEPTH_2, FRM_DEPTH_3, FRM_DEPTH_3, FRM_DEPTH_4, FRM_DEPTH_4, FRM_DEPTH_4, FRM_DEPTH_4,\
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    /* gop_size = 12 */
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    /* gop_size = 16 */
    { FRM_DEPTH_2, FRM_DEPTH_3, FRM_DEPTH_3, FRM_DEPTH_4, FRM_DEPTH_4, FRM_DEPTH_4, FRM_DEPTH_4, FRM_DEPTH_5, \
      FRM_DEPTH_5,  FRM_DEPTH_5, FRM_DEPTH_5, FRM_DEPTH_5, FRM_DEPTH_5, FRM_DEPTH_5, FRM_DEPTH_5 }
};

static const s8 tbl_slice_depth_orig[5][15] =
{
    /* gop_size = 2 */
    { FRM_DEPTH_2, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, \
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    /* gop_size = 4 */
    { FRM_DEPTH_2, FRM_DEPTH_3, FRM_DEPTH_3, 0xFF, 0xFF, 0xFF, 0xFF, \
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    /* gop_size = 8 */
    { FRM_DEPTH_2, FRM_DEPTH_3, FRM_DEPTH_4, FRM_DEPTH_4, FRM_DEPTH_3, FRM_DEPTH_4, FRM_DEPTH_4,\
    0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    /* gop_size = 12 */
    {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF },
    /* gop_size = 16 */
    { FRM_DEPTH_2, FRM_DEPTH_3, FRM_DEPTH_4, FRM_DEPTH_5, FRM_DEPTH_5, FRM_DEPTH_4, FRM_DEPTH_5, FRM_DEPTH_5, \
      FRM_DEPTH_3,  FRM_DEPTH_4, FRM_DEPTH_5, FRM_DEPTH_5, FRM_DEPTH_4, FRM_DEPTH_5, FRM_DEPTH_5 }
};

static XEVE_CTX * ctx_alloc(void)
{
    XEVE_CTX * ctx;

    ctx = (XEVE_CTX*)xeve_malloc_fast(sizeof(XEVE_CTX));
    xeve_assert_rv(ctx, NULL);
    xeve_mset_x64a(ctx, 0, sizeof(XEVE_CTX));
    return ctx;
}

static void ctx_free(XEVE_CTX * ctx)
{
    xeve_mfree_fast(ctx);
}

static XEVE_CORE * core_alloc(void)
{
    XEVE_CORE * core;
    int i, j;

    core = (XEVE_CORE *)xeve_malloc_fast(sizeof(XEVE_CORE));

    xeve_assert_rv(core, NULL);
    xeve_mset_x64a(core, 0, sizeof(XEVE_CORE));

    for(i = 0; i < MAX_CU_LOG2; i++)
    {
        for(j = 0; j < MAX_CU_LOG2; j++)
        {
            xeve_create_cu_data(&core->cu_data_best[i][j], i, j);
            xeve_create_cu_data(&core->cu_data_temp[i][j], i, j);
        }
    }

    return core;
}

static void core_free(XEVE_CORE * core)
{
    int i, j;

    for(i = 0; i < MAX_CU_LOG2; i++)
    {
        for(j = 0; j < MAX_CU_LOG2; j++)
        {
            xeve_delete_cu_data(&core->cu_data_best[i][j], i, j);
            xeve_delete_cu_data(&core->cu_data_temp[i][j], i, j);
        }
    }

    xeve_mfree_fast(core);
}

void xeve_copy_chroma_qp_mapping_params(XEVE_CHROMA_TABLE *dst, XEVE_CHROMA_TABLE *src)
{
    dst->chroma_qp_table_present_flag = src->chroma_qp_table_present_flag;
    dst->same_qp_table_for_chroma = src->same_qp_table_for_chroma;
    dst->global_offset_flag = src->global_offset_flag;
    dst->num_points_in_qp_table_minus1[0] = src->num_points_in_qp_table_minus1[0];
    dst->num_points_in_qp_table_minus1[1] = src->num_points_in_qp_table_minus1[1];
    memcpy(&(dst->delta_qp_in_val_minus1), &(src->delta_qp_in_val_minus1), sizeof(int) * 2 * MAX_QP_TABLE_SIZE);
    memcpy(&(dst->delta_qp_out_val), &(src->delta_qp_out_val), sizeof(int) * 2 * MAX_QP_TABLE_SIZE);
}

static void xevea_parse_chroma_qp_mapping_params(XEVE_CHROMA_TABLE *dst_struct, XEVE_CHROMA_TABLE *src_struct, int bit_depth)
{
    int qp_bd_offset_c = 6 * (bit_depth - 8);
    XEVE_CHROMA_TABLE *chroma_qp_table = dst_struct;
    chroma_qp_table->chroma_qp_table_present_flag = src_struct->chroma_qp_table_present_flag;
    chroma_qp_table->num_points_in_qp_table_minus1[0] = src_struct->num_points_in_qp_table_minus1[0];
    chroma_qp_table->num_points_in_qp_table_minus1[1] = src_struct->num_points_in_qp_table_minus1[1];

    if (chroma_qp_table->chroma_qp_table_present_flag)
    {
        chroma_qp_table->same_qp_table_for_chroma = 1;
        if (src_struct->num_points_in_qp_table_minus1[0] != src_struct->num_points_in_qp_table_minus1[1])
            chroma_qp_table->same_qp_table_for_chroma = 0;
        else
        {
            for (int i = 0; i < src_struct->num_points_in_qp_table_minus1[0]; i++)
            {
                if ((src_struct->delta_qp_in_val_minus1[0][i] != src_struct->delta_qp_in_val_minus1[1][i]) ||
                    (src_struct->delta_qp_out_val[0][i] != src_struct->delta_qp_out_val[1][i]))
                {
                    chroma_qp_table->same_qp_table_for_chroma = 0;
                    break;
                }
            }
        }

        chroma_qp_table->global_offset_flag = (src_struct->delta_qp_in_val_minus1[0][0] > 15 && src_struct->delta_qp_out_val[0][0] > 15) ? 1 : 0;
        if (!chroma_qp_table->same_qp_table_for_chroma)
        {
            chroma_qp_table->global_offset_flag = chroma_qp_table->global_offset_flag && ((src_struct->delta_qp_in_val_minus1[1][0] > 15 && src_struct->delta_qp_out_val[1][0] > 15) ? 1 : 0);
        }

        int start_qp = (chroma_qp_table->global_offset_flag == 1) ? 16 : -qp_bd_offset_c;
        for (int ch = 0; ch < (chroma_qp_table->same_qp_table_for_chroma ? 1 : 2); ch++) {
            chroma_qp_table->delta_qp_in_val_minus1[ch][0] = src_struct->delta_qp_in_val_minus1[ch][0] - start_qp;
            chroma_qp_table->delta_qp_out_val[ch][0] = src_struct->delta_qp_out_val[ch][0] - start_qp - chroma_qp_table->delta_qp_in_val_minus1[ch][0];

            for (int k = 1; k <= chroma_qp_table->num_points_in_qp_table_minus1[ch]; k++)
            {
                chroma_qp_table->delta_qp_in_val_minus1[ch][k] = (src_struct->delta_qp_in_val_minus1[ch][k] - src_struct->delta_qp_in_val_minus1[ch][k - 1]) - 1;
                chroma_qp_table->delta_qp_out_val[ch][k] = (src_struct->delta_qp_out_val[ch][k] - src_struct->delta_qp_out_val[ch][k - 1]) - (chroma_qp_table->delta_qp_in_val_minus1[ch][k] + 1);
            }
        }
    }
}

static int set_init_param(XEVE_CDSC * cdsc, XEVE_PARAM * param)
{
    param->preset = (&xeve_tbl_preset[cdsc->preset]);

    xeve_assert_rv(cdsc->profile == PROFILE_BASELINE, XEVE_ERR_INVALID_ARGUMENT);
    /* check input parameters */
    int pic_m = 8;
    xeve_assert_rv(cdsc->w > 0 && cdsc->h > 0, XEVE_ERR_INVALID_ARGUMENT);
    xeve_assert_rv((cdsc->w & (pic_m -1)) == 0,XEVE_ERR_INVALID_ARGUMENT);
    xeve_assert_rv((cdsc->h & (pic_m -1)) == 0,XEVE_ERR_INVALID_ARGUMENT);
    xeve_assert_rv(cdsc->qp >= MIN_QUANT && cdsc->qp <= MAX_QUANT, XEVE_ERR_INVALID_ARGUMENT);
    xeve_assert_rv(cdsc->iperiod >= 0 ,XEVE_ERR_INVALID_ARGUMENT);

    if(cdsc->disable_hgop == 0)
    {
        xeve_assert_rv(cdsc->max_b_frames == 0 || cdsc->max_b_frames == 1 || \
                       cdsc->max_b_frames == 3 || cdsc->max_b_frames == 7 || \
                       cdsc->max_b_frames == 15, XEVE_ERR_INVALID_ARGUMENT);

        if(cdsc->max_b_frames != 0)
        {
            if(cdsc->iperiod % (cdsc->max_b_frames + 1) != 0)
            {
                xeve_assert_rv(0, XEVE_ERR_INVALID_ARGUMENT);
            }
        }
    }

    if (cdsc->ref_pic_gap_length != 0)
    {
        xeve_assert_rv(cdsc->max_b_frames == 0, XEVE_ERR_INVALID_ARGUMENT);
    }


    if (cdsc->max_b_frames == 0)
    {
        if (cdsc->ref_pic_gap_length == 0)
        {
            cdsc->ref_pic_gap_length = 1;
        }
        xeve_assert_rv(cdsc->ref_pic_gap_length == 1 || cdsc->ref_pic_gap_length == 2 || \
                      cdsc->ref_pic_gap_length == 4 || cdsc->ref_pic_gap_length == 8 || \
                      cdsc->ref_pic_gap_length == 16, XEVE_ERR_INVALID_ARGUMENT);
    }

    /* set default encoding parameter */
    param->w              = cdsc->w;
    param->h              = cdsc->h;
    param->bit_depth      = cdsc->out_bit_depth;
    param->qp             = cdsc->qp;
    param->fps            = cdsc->fps;
    param->i_period       = cdsc->iperiod;
    param->f_ifrm         = 0;
    param->use_deblock    = cdsc->use_deblock;

    param->qp_max         = MAX_QUANT;
    param->qp_min         = MIN_QUANT;
    param->use_pic_sign   = 0;
    param->max_b_frames   = cdsc->max_b_frames;
    param->ref_pic_gap_length = cdsc->ref_pic_gap_length;
    param->gop_size       = param->max_b_frames +1;
    param->use_closed_gop = (cdsc->closed_gop)? 1: 0;
    param->use_dqp = cdsc->use_dqp;
    param->use_hgop = (cdsc->disable_hgop) ? 0 : 1;
    param->qp_incread_frame = cdsc->add_qp_frame;
    param->rc_type = cdsc->rc_type; //rc_off =0 , rc_cbr = 1
    param->bps = cdsc->bps;
    param->vbv_msec = cdsc->vbv_msec;
    param->use_filler_flag = cdsc->use_filler_flag;
    param->num_pre_analysis_frames = cdsc->num_pre_analysis_frames;
    param->vbv_enabled = 1;
    param->vbv_buffer_size = (int)((param->bps) *(param->vbv_msec / 1000.0));
    xeve_qp_chroma_ajudst = xeve_tbl_qp_chroma_ajudst;

    if (cdsc->chroma_qp_table_struct.chroma_qp_table_present_flag)
    {
        XEVE_CHROMA_TABLE tmp_qp_tbl;
        xeve_mcpy(&tmp_qp_tbl, &(cdsc->chroma_qp_table_struct), sizeof(XEVE_CHROMA_TABLE));
        xevea_parse_chroma_qp_mapping_params(&(cdsc->chroma_qp_table_struct), &tmp_qp_tbl, cdsc->codec_bit_depth);
        xeve_tbl_derived_chroma_qp_mapping(&(cdsc->chroma_qp_table_struct), cdsc->codec_bit_depth);
    }
    else
    {
        memcpy(&(xeve_tbl_qp_chroma_dynamic_ext[0][6 * (cdsc->codec_bit_depth - 8)]), xeve_tbl_qp_chroma_ajudst, MAX_QP_TABLE_SIZE * sizeof(int));
        memcpy(&(xeve_tbl_qp_chroma_dynamic_ext[1][6 * (cdsc->codec_bit_depth - 8)]), xeve_tbl_qp_chroma_ajudst, MAX_QP_TABLE_SIZE * sizeof(int));
    }

    param->deblock_alpha_offset = 0;
    param->deblock_beta_offset  = 0;
    param->use_ibc_flag         = 0;
    param->ibc_search_range_x   = 0;
    param->ibc_hash_search_flag = 0;
    param->ibc_search_range_y   = 0;
    param->ibc_hash_search_max_cand        = 0;
    param->ibc_hash_search_range_4smallblk = 0;
    param->ibc_fast_method  = 0;
    param->cu_qp_delta_area = 0;

    param->tile_rows        = 1;
    param->tile_columns     = 1;
    param->num_slice_in_pic = 1;
    param->uniform_spacing_tiles = 0;;
    param->arbitrary_slice_flag  = 0;

    for (int i = 0; i < (2 * param->num_slice_in_pic); i++)
    {
        param->slice_boundary_array[i] = 0;
    }

    param->rdo_dbk_switch = param->preset->rdo_dbk;

    return XEVE_OK;
}

static int set_enc_param(XEVE_CTX * ctx, XEVE_PARAM * param)
{
    int ret = XEVE_OK;
    ctx->qp = (u8)param->qp;
    return ret;
}

static void set_nalu(XEVE_NALU * nalu, int nalu_type, int nuh_temporal_id)
{
    nalu->nal_unit_size = 0;
    nalu->forbidden_zero_bit = 0;
    nalu->nal_unit_type_plus1 = nalu_type + 1;
    nalu->nuh_temporal_id = nuh_temporal_id;
    nalu->nuh_reserved_zero_5bits = 0;
    nalu->nuh_extension_flag = 0;
}

// Dummy VUI initialization
static void set_vui(XEVE_CTX * ctx, XEVE_VUI * vui)
{
    vui->aspect_ratio_info_present_flag = 1;
    vui->aspect_ratio_idc = 1;
    vui->sar_width = 1;
    vui->sar_height = 1;
    vui->overscan_info_present_flag = 1;
    vui->overscan_appropriate_flag = 1;
    vui->video_signal_type_present_flag = 1;
    vui->video_format = 1;
    vui->video_full_range_flag = 1;
    vui->colour_description_present_flag = 1;
    vui->colour_primaries = 1;
    vui->transfer_characteristics = 1;
    vui->matrix_coefficients = 1;
    vui->chroma_loc_info_present_flag = 1;
    vui->chroma_sample_loc_type_top_field = 1;
    vui->chroma_sample_loc_type_bottom_field = 1;
    vui->neutral_chroma_indication_flag = 1;
    vui->field_seq_flag = 1;
    vui->timing_info_present_flag = 1;
    vui->num_units_in_tick = 1;
    vui->time_scale = 1;
    vui->fixed_pic_rate_flag = 1;
    vui->nal_hrd_parameters_present_flag = 1;
    vui->vcl_hrd_parameters_present_flag = 1;
    vui->low_delay_hrd_flag = 1;
    vui->pic_struct_present_flag = 1;
    vui->bitstream_restriction_flag = 1;
    vui->motion_vectors_over_pic_boundaries_flag = 1;
    vui->max_bytes_per_pic_denom = 1;
    vui->max_bits_per_mb_denom = 1;
    vui->log2_max_mv_length_horizontal = 1;
    vui->log2_max_mv_length_vertical = 1;
    vui->num_reorder_pics = 1;
    vui->max_dec_pic_buffering = 1;
    vui->hrd_parameters.cpb_cnt_minus1 = 1;
    vui->hrd_parameters.bit_rate_scale = 1;
    vui->hrd_parameters.cpb_size_scale = 1;
    memset(&(vui->hrd_parameters.bit_rate_value_minus1), 0, sizeof(int)*NUM_CPB);
    memset(&(vui->hrd_parameters.cpb_size_value_minus1), 0, sizeof(int)*NUM_CPB);
    memset(&(vui->hrd_parameters.cbr_flag), 0, sizeof(int)*NUM_CPB);
    vui->hrd_parameters.initial_cpb_removal_delay_length_minus1 = 1;
    vui->hrd_parameters.cpb_removal_delay_length_minus1 = 1;
    vui->hrd_parameters.dpb_output_delay_length_minus1 = 1;
    vui->hrd_parameters.time_offset_length = 1;
}


static void set_sps(XEVE_CTX * ctx, XEVE_SPS * sps)
{
    sps->profile_idc = ctx->cdsc.profile;
    sps->level_idc = ctx->cdsc.level * 3;
    sps->pic_width_in_luma_samples = ctx->param.w;
    sps->pic_height_in_luma_samples = ctx->param.h;
    sps->toolset_idc_h = 0;
    sps->toolset_idc_l = 0;
    sps->bit_depth_luma_minus8 = ctx->cdsc.codec_bit_depth - 8;
    sps->bit_depth_chroma_minus8 = ctx->cdsc.codec_bit_depth - 8;
    sps->chroma_format_idc = 1;
    sps->ibc_flag = 0;
    sps->ibc_log_max_size = IBC_MAX_CU_LOG2;
    sps->log2_max_pic_order_cnt_lsb_minus4 = POC_LSB_BIT - 4;
    sps->sps_max_dec_pic_buffering_minus1 = 0; //[TBF]

    if(ctx->param.max_b_frames > 0)
    {
        sps->max_num_ref_pics = ctx->param.preset->me_ref_num;
    }
    else
    {
        sps->max_num_ref_pics = MAX_NUM_ACTIVE_REF_FRAME_LDB;
    }

    sps->sps_btt_flag = 0;
    sps->sps_suco_flag = 0;

    sps->tool_amvr    = 0;
    sps->tool_mmvd    = 0;
    sps->tool_affine  = 0;
    sps->tool_dmvr    = 0;
    sps->tool_addb    = 0;
    sps->tool_dra     = 0;
    sps->tool_alf     = 0;
    sps->tool_htdf    = 0;
    sps->tool_admvp   = 0;
    sps->tool_hmvp    = 0;
    sps->tool_eipd    = 0;
    sps->tool_iqt     = 0;
    sps->tool_adcc    = 0;
    sps->tool_cm_init = 0;
    sps->tool_ats     = 0;
    sps->tool_rpl     = 0;
    sps->tool_pocs    = 0;

    sps->log2_sub_gop_length = (int)(log2(ctx->param.gop_size) + .5);
    ctx->ref_pic_gap_length = ctx->param.ref_pic_gap_length;
    sps->log2_ref_pic_gap_length = (int)(log2(ctx->param.ref_pic_gap_length) + .5);
    sps->long_term_ref_pics_flag = 0;

    sps->num_ref_pic_lists_in_sps0 = 0;
    sps->num_ref_pic_lists_in_sps1 = 0;
    sps->rpl1_same_as_rpl0_flag = 0;

    sps->vui_parameters_present_flag = 0;
    set_vui(ctx, &(sps->vui_parameters));
    sps->dquant_flag = ctx->cdsc.profile == 0 ? 0 : 1;                 /*Baseline : Active SPSs shall have sps_dquant_flag equal to 0 only*/

    if (ctx->cdsc.chroma_qp_table_struct.chroma_qp_table_present_flag)
    {
        xeve_copy_chroma_qp_mapping_params(&(sps->chroma_qp_table_struct), &(ctx->cdsc.chroma_qp_table_struct));
    }

    sps->picture_cropping_flag = ctx->cdsc.picture_cropping_flag;
    if (sps->picture_cropping_flag)
    {
        sps->picture_crop_left_offset = ctx->cdsc.picture_crop_left_offset;
        sps->picture_crop_right_offset = ctx->cdsc.picture_crop_right_offset;
        sps->picture_crop_top_offset = ctx->cdsc.picture_crop_top_offset;
        sps->picture_crop_bottom_offset = ctx->cdsc.picture_crop_bottom_offset;
    }
}

static void set_pps(XEVE_CTX * ctx, XEVE_PPS * pps)
{
    pps->loop_filter_across_tiles_enabled_flag = 0;
    pps->single_tile_in_pic_flag = 1;
    pps->constrained_intra_pred_flag = ctx->cdsc.constrained_intra_pred;
    pps->cu_qp_delta_enabled_flag = XEVE_ABS(ctx->cdsc.use_dqp);

    pps->num_ref_idx_default_active_minus1[REFP_0] = 0;
    pps->num_ref_idx_default_active_minus1[REFP_1] = 0;

    ctx->pps.pps_pic_parameter_set_id = 0;
    xeve_mcpy(&ctx->pps_array[ctx->pps.pps_pic_parameter_set_id], &ctx->pps, sizeof(XEVE_PPS));
}

typedef struct _QP_ADAPT_PARAM
{
    int qp_offset_layer;
    double qp_offset_model_offset;
    double qp_offset_model_scale;
} QP_ADAPT_PARAM;

QP_ADAPT_PARAM qp_adapt_param_ra[8] =
{
    {-3,  0.0000, 0.0000},
    { 1,  0.0000, 0.0000},
    { 1, -4.8848, 0.2061},
    { 4, -5.7476, 0.2286},
    { 5, -5.9000, 0.2333},
    { 6, -7.1444, 0.3000},
    { 7, -7.1444, 0.3000},
    { 8, -7.1444, 0.3000},
};

QP_ADAPT_PARAM qp_adapt_param_ld[8] =
{
    {-1,  0.0000, 0.0000 },
    { 1,  0.0000, 0.0000 },
    { 4, -6.5000, 0.2590 },
    { 4, -6.5000, 0.2590 },
    { 5, -6.5000, 0.2590 },
    { 5, -6.5000, 0.2590 },
    { 5, -6.5000, 0.2590 },
    { 5, -6.5000, 0.2590 },
};

QP_ADAPT_PARAM qp_adapt_param_ai[8] =
{
    { 0,  0.0000, 0.0000},
    { 0,  0.0000, 0.0000},
    { 0,  0.0000, 0.0000},
    { 0,  0.0000, 0.0000},
    { 0,  0.0000, 0.0000},
    { 0,  0.0000, 0.0000},
    { 0,  0.0000, 0.0000},
    { 0,  0.0000, 0.0000},
};

static void set_sh(XEVE_CTX *ctx, XEVE_SH *sh)
{
    double qp;
    int qp_l_i;
    int qp_c_i;

    QP_ADAPT_PARAM *qp_adapt_param = ctx->param.max_b_frames == 0 ?
        (ctx->param.i_period == 1 ? qp_adapt_param_ai : qp_adapt_param_ld) : qp_adapt_param_ra;

    sh->slice_type = ctx->slice_type;
    sh->no_output_of_prior_pics_flag = 0;
    sh->deblocking_filter_on = (ctx->param.use_deblock) ? 1 : 0;
    sh->sh_deblock_alpha_offset = ctx->param.deblock_alpha_offset;
    sh->sh_deblock_beta_offset = ctx->param.deblock_beta_offset;
    sh->single_tile_in_slice_flag = 1;
    sh->collocated_from_list_idx = (sh->slice_type == SLICE_P) ? REFP_0 : REFP_1;  // Specifies source (List ID) of the collocated picture, equialent of the collocated_from_l0_flag
    sh->collocated_from_ref_idx = 0;        // Specifies source (RefID_ of the collocated picture, equialent of the collocated_ref_idx
    sh->collocated_mvp_source_list_idx = REFP_0;  // Specifies source (List ID) in collocated pic that provides MV information (Applicability is function of NoBackwardPredFlag)

    /* set lambda */
    qp = XEVE_CLIP3(0, MAX_QUANT, (ctx->param.qp_incread_frame != 0 && (int)(ctx->poc.poc_val) >= ctx->param.qp_incread_frame) ? ctx->qp + 1.0 : ctx->qp);
    sh->dqp = XEVE_ABS(ctx->param.use_dqp);

    if(ctx->param.use_hgop)
    {
        double dqp_offset;
        int qp_offset;

        qp += qp_adapt_param[ctx->slice_depth].qp_offset_layer;
        dqp_offset = qp * qp_adapt_param[ctx->slice_depth].qp_offset_model_scale + qp_adapt_param[ctx->slice_depth].qp_offset_model_offset + 0.5;

        qp_offset = (int)floor(XEVE_CLIP3(0.0, 3.0, dqp_offset));
        qp += qp_offset;
    }

    sh->qp   = (u8)XEVE_CLIP3(0, MAX_QUANT, qp);
    sh->qp_u_offset = ctx->cdsc.cb_qp_offset;
    sh->qp_v_offset = ctx->cdsc.cr_qp_offset;
    sh->qp_u = (s8)XEVE_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, sh->qp + sh->qp_u_offset);
    sh->qp_v = (s8)XEVE_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, sh->qp + sh->qp_v_offset);

    qp_l_i = sh->qp;
    ctx->lambda[0] = 0.57 * pow(2.0, (qp_l_i - 12.0) / 3.0);
    qp_c_i = xeve_qp_chroma_dynamic[0][sh->qp_u];
    ctx->dist_chroma_weight[0] = pow(2.0, (qp_l_i - qp_c_i) / 3.0);
    qp_c_i = xeve_qp_chroma_dynamic[1][sh->qp_v];
    ctx->dist_chroma_weight[1] = pow(2.0, (qp_l_i - qp_c_i) / 3.0);
    ctx->lambda[1] = ctx->lambda[0] / ctx->dist_chroma_weight[0];
    ctx->lambda[2] = ctx->lambda[0] / ctx->dist_chroma_weight[1];
    ctx->sqrt_lambda[0] = sqrt(ctx->lambda[0]);
    ctx->sqrt_lambda[1] = sqrt(ctx->lambda[1]);
    ctx->sqrt_lambda[2] = sqrt(ctx->lambda[2]);

    ctx->sh.slice_pic_parameter_set_id = 0;
}

static int set_active_pps_info(XEVE_CTX * ctx)
{
    int active_pps_id = ctx->sh.slice_pic_parameter_set_id;
    memcpy(&(ctx->pps), &(ctx->pps_array[active_pps_id]), sizeof(XEVE_PPS));

    return XEVE_OK;
}

static int set_active_dra_info(XEVE_CTX * ctx)
{
    return XEVE_OK;
}

static int set_tile_info(XEVE_CTX * ctx)
{
    XEVE_TILE     * tile;
    int          i, j, size, x, y, w, h, w_tile, h_tile, w_lcu, h_lcu, tidx, t0;
    int          col_w[MAX_NUM_TILES_COL], row_h[MAX_NUM_TILES_ROW], f_tile;
    u8         * map_tidx;
    u32        * map_scu;
    u8         * tile_to_slice_map = ctx->tile_to_slice_map;
    u8         * tile_order = ctx->tile_order;
    int          num_slice_in_pic;
    int          slice_num = 0;

    ctx->tile_cnt = ctx->param.tile_columns * ctx->param.tile_rows;
    w_tile = ctx->param.tile_columns;
    h_tile = ctx->param.tile_rows;
    f_tile = w_tile * h_tile;
    w_lcu = ctx->w_lcu;
    h_lcu = ctx->h_lcu;
    num_slice_in_pic = ctx->param.num_slice_in_pic;


    tile_to_slice_map[0] = 0;
    tile_order[0] = 0;
    /* alloc tile information */
    size = sizeof(XEVE_TILE) * f_tile;
    ctx->tile = xeve_malloc(size);
    xeve_assert_rv(ctx->tile, XEVE_ERR_OUT_OF_MEMORY);
    xeve_mset(ctx->tile, 0, size);

    col_w[0] = w_lcu;
    row_h[0] = h_lcu;

    /* update tile information - Tile width, height, First ctb address */
    tidx = 0;
    for (y = 0; y<h_tile; y++)
    {
        for (x = 0; x<w_tile; x++)
        {
            tile = &ctx->tile[tidx];
            tile->w_ctb = col_w[x];
            tile->h_ctb = row_h[y];
            tile->f_ctb = tile->w_ctb * tile->h_ctb;
            tile->ctba_rs_first = 0;

            for (i = 0; i<x; i++)
            {
                tile->ctba_rs_first += col_w[i];
            }
            for (j = 0; j<y; j++)
            {
                tile->ctba_rs_first += w_lcu * row_h[j];
            }
            tidx++;
        }
    }

    /* set tile map - SCU level mapping to tile index */
    for (tidx = 0; tidx<(w_tile * h_tile); tidx++)
    {
        slice_num = tile_to_slice_map[tidx];
        tile = ctx->tile + tidx;
        x = PEL2SCU((tile->ctba_rs_first % w_lcu) << ctx->log2_max_cuwh);
        y = PEL2SCU((tile->ctba_rs_first / w_lcu) << ctx->log2_max_cuwh);
        t0 = PEL2SCU(tile->w_ctb << ctx->log2_max_cuwh);
        w = XEVE_MIN((ctx->w_scu - x), t0);
        t0 = PEL2SCU(tile->h_ctb << ctx->log2_max_cuwh);
        h = XEVE_MIN((ctx->h_scu - y), t0);

        map_tidx = ctx->map_tidx + x + y * ctx->w_scu;
        map_scu = ctx->map_scu + x + y * ctx->w_scu;
        for (j = 0; j<h; j++)
        {
            for (i = 0; i<w; i++)
            {
                map_tidx[i] = tidx;
                MCU_SET_SN(map_scu[i], slice_num);  //Mapping CUs to the slices
            }
            map_tidx += ctx->w_scu;
            map_scu += ctx->w_scu;
        }
    }
    return XEVE_OK;
}

static int xeve_eco_tree(XEVE_CTX * ctx, XEVE_CORE * core, int x0, int y0, int cup, int cuw, int cuh, int cud
                       , int cu_qp_delta_code, TREE_CONS tree_cons, XEVE_BSW * bs)
{
    int ret;
    s8  split_mode;
    s8  suco_flag = 0;
    int bound;

    core->tree_cons = tree_cons;
    xeve_get_split_mode(&split_mode, cud, cup, cuw, cuh, ctx->max_cuwh, ctx->map_cu_data[core->lcu_num].split_mode);

    if(split_mode != NO_SPLIT)
    {
        if(!ctx->sps.sps_btt_flag || ((x0 + cuw <= ctx->w) && (y0 + cuh <= ctx->h)))
        {
            ctx->fn_eco_split_mode(bs, ctx, core, cud, cup, cuw, cuh, ctx->max_cuwh, x0, y0);
        }

        bound = !((x0 + cuw <= ctx->w) && (y0 + cuh <= ctx->h));
        XEVE_SPLIT_STRUCT split_struct;
        xeve_split_get_part_structure(split_mode, x0, y0, cuw, cuh, cup, cud, ctx->log2_culine, &split_struct);
        split_struct.tree_cons = xeve_get_default_tree_cons();

        for(int part_num = 0; part_num < split_struct.part_count; ++part_num)
        {
            int cur_part_num = part_num;
            int sub_cuw = split_struct.width[cur_part_num];
            int sub_cuh = split_struct.height[cur_part_num];
            int x_pos = split_struct.x_pos[cur_part_num];
            int y_pos = split_struct.y_pos[cur_part_num];

            if(x_pos < ctx->w && y_pos < ctx->h)
            {
                ret = xeve_eco_tree(ctx, core, x_pos, y_pos, split_struct.cup[cur_part_num], sub_cuw, sub_cuh, split_struct.cud[cur_part_num], cu_qp_delta_code, split_struct.tree_cons, bs);
                xeve_assert_g(XEVE_SUCCEEDED(ret), ERR);
            }
        }
    }
    else
    {
        xeve_assert(x0 + cuw <= ctx->w && y0 + cuh <= ctx->h);
        if(cuw > MIN_CU_SIZE || cuh > MIN_CU_SIZE)
        {
            ctx->fn_eco_split_mode(bs, ctx, core, cud, cup, cuw, cuh, ctx->max_cuwh, x0, y0);
        }
        core->cu_qp_delta_code = cu_qp_delta_code;
        ret = xeve_eco_unit(ctx, core, x0, y0, cup, cuw, cuh, tree_cons, bs);
        xeve_assert_g(XEVE_SUCCEEDED(ret), ERR);
    }

    return XEVE_OK;
ERR:
    return ret;
}

int xeve_ready(XEVE_CTX * ctx)
{
    XEVE_CORE * core = NULL;
    int          w, h, ret, i;
    s32          size;

    xeve_assert(ctx);
    if(ctx->core[0] == NULL)
    {
        core = core_alloc();
        xeve_assert_gv(core != NULL, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

        /* set various value */
        for (int i = 0; i < ctx->cdsc.parallel_task_cnt; i++)
        {
            core = core_alloc();
            xeve_assert_gv(core != NULL, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
            ctx->core[i] = core;
        }
    }

    xeve_init_bits_est();

    if (ctx->w == 0)
    {
        w = ctx->w = ctx->param.w;
        h = ctx->h = ctx->param.h;
        ctx->f = w * h;

        ctx->max_cuwh = 64;
        ctx->min_cuwh = 1 << 2;
        ctx->log2_min_cuwh = 2;

        ctx->log2_max_cuwh = XEVE_LOG2(ctx->max_cuwh);
        ctx->max_cud = ctx->log2_max_cuwh - MIN_CU_LOG2;
        ctx->w_lcu = (w + ctx->max_cuwh - 1) >> ctx->log2_max_cuwh;
        ctx->h_lcu = (h + ctx->max_cuwh - 1) >> ctx->log2_max_cuwh;
        ctx->f_lcu = ctx->w_lcu * ctx->h_lcu;
        ctx->w_scu = (w + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
        ctx->h_scu = (h + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
        ctx->f_scu = ctx->w_scu * ctx->h_scu;
        ctx->log2_culine = ctx->log2_max_cuwh - MIN_CU_LOG2;
        ctx->log2_cudim = ctx->log2_culine << 1;
    }

    if (ctx->cdsc.rc_type != 0)
    {
        xeve_rc_create(ctx);
    }
    else
    {
        ctx->rc = NULL;
        ctx->rcore = NULL;
        ctx->qp = ctx->param.qp;
    }

    //initialize the threads to NULL
    for (int i = 0; i < XEVE_MAX_TASK_CNT; i++)
    {
        ctx->thread_pool[i] = 0;
    }

    //get the context synchronization handle
    ctx->sync_block = get_synchronized_object();
    xeve_assert_gv(ctx->sync_block != NULL, ret, XEVE_ERR_UNKNOWN, ERR);

    if (ctx->cdsc.parallel_task_cnt >= 1)
    {
        ctx->tc = xeve_malloc(sizeof(THREAD_CONTROLLER));
        init_thread_controller(ctx->tc, ctx->cdsc.parallel_task_cnt);
        for (int i = 0; i < ctx->cdsc.parallel_task_cnt; i++)
        {
            ctx->thread_pool[i] = ctx->tc->create(ctx->tc, i);
            xeve_assert_gv(ctx->thread_pool[i] != NULL, ret, XEVE_ERR_UNKNOWN, ERR);
        }
    }
    
    /*  allocate CU data map*/
    if(ctx->map_cu_data == NULL)
    {
        size = sizeof(XEVE_CU_DATA) * ctx->f_lcu;
        ctx->map_cu_data = (XEVE_CU_DATA*)xeve_malloc_fast(size);
        xeve_assert_gv(ctx->map_cu_data, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset_x64a(ctx->map_cu_data, 0, size);

        for(i = 0; i < (int)ctx->f_lcu; i++)
        {
            xeve_create_cu_data(ctx->map_cu_data + i, ctx->log2_max_cuwh - MIN_CU_LOG2, ctx->log2_max_cuwh - MIN_CU_LOG2);
        }
    }

    /* allocate maps */
    if(ctx->map_scu == NULL)
    {
        size = sizeof(u32) * ctx->f_scu;
        ctx->map_scu = xeve_malloc_fast(size);
        xeve_assert_gv(ctx->map_scu, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset_x64a(ctx->map_scu, 0, size);
    }

    if(ctx->map_ipm == NULL)
    {
        size = sizeof(s8) * ctx->f_scu;
        ctx->map_ipm = xeve_malloc_fast(size);
        xeve_assert_gv(ctx->map_ipm, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset(ctx->map_ipm, -1, size);
    }

    size = sizeof(s8) * ctx->f_scu;
    ctx->map_depth = xeve_malloc_fast(size);
    xeve_assert_gv(ctx->map_depth, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    xeve_mset(ctx->map_depth, -1, size);


    if(ctx->map_cu_mode == NULL)
    {
        size = sizeof(u32) * ctx->f_scu;
        ctx->map_cu_mode = xeve_malloc_fast(size);
        xeve_assert_gv(ctx->map_cu_mode, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset_x64a(ctx->map_cu_mode, 0, size);
    }

    /* initialize reference picture manager */
    ctx->pa.fn_alloc  = xeve_pic_alloc;
    ctx->pa.fn_free   = xeve_pic_free;
    ctx->pa.w         = ctx->w;
    ctx->pa.h         = ctx->h;
    ctx->pa.pad_l     = PIC_PAD_SIZE_L;
    ctx->pa.pad_c     = PIC_PAD_SIZE_C;
    ctx->pa.bit_depth = ctx->cdsc.codec_bit_depth;
    ctx->pic_cnt      = 0;
    ctx->pic_icnt     = -1;
    ctx->poc.poc_val  = 0;

    ret = xeve_picman_init(&ctx->rpm, MAX_PB_SIZE, MAX_NUM_REF_PICS, &ctx->pa);
    xeve_assert_g(XEVE_SUCCEEDED(ret), ERR);

    ctx->pico_max_cnt = 1 + (ctx->param.max_b_frames << 1) ;
    ctx->frm_rnum = ctx->param.max_b_frames;
    ctx->qp = ctx->param.qp;

    for(i = 0; i < ctx->pico_max_cnt; i++)
    {
        ctx->pico_buf[i] = (XEVE_PICO*)xeve_malloc(sizeof(XEVE_PICO));
        xeve_assert_gv(ctx->pico_buf[i], ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset(ctx->pico_buf[i], 0, sizeof(XEVE_PICO));
        XEVE_PICO *pico;
        pico = ctx->pico_buf[i];

        pico->spic = xeve_alloc_spic_l(ctx->w, ctx->h, 32, 0, &ret, 10);
        xeve_assert_g(pico->spic != NULL, ERR);

        size = sizeof(u8) * ctx->f_lcu;
        pico->map_pdir = xeve_malloc(size);
        xeve_assert_g(pico->map_pdir != NULL, ERR);

        size = sizeof(s16) * ctx->f_lcu * REFP_NUM * MV_D;
        pico->map_mv = xeve_malloc(size);
        xeve_assert_g(pico->map_mv != NULL, ERR);

        size = sizeof(s16) * ctx->f_lcu * REFP_NUM * MV_D;
        pico->map_mv_pga = xeve_malloc(size);
        xeve_assert_g(pico->map_mv_pga != NULL, ERR);

        size = sizeof(s32) * ctx->f_lcu * 4;
        pico->map_lcu_cost_uni = xeve_malloc(size);
        xeve_assert_g(pico->map_lcu_cost_uni != NULL, ERR);

        size = sizeof(s32) * ctx->f_lcu;
        pico->map_lcu_cost_bi = xeve_malloc(size);
        xeve_assert_g(pico->map_lcu_cost_bi != NULL, ERR);

        if (ctx->param.use_dqp)
        {
            pico->map_qp_offset = (s32 *)xeve_malloc(sizeof(s32) * ctx->f_lcu);
            xeve_assert_g(pico->map_qp_offset != NULL, ERR);

            size = sizeof(u16) * ctx->f_lcu;
            pico->transfer_cost = xeve_malloc(size);
            xeve_assert_g(pico->transfer_cost != NULL, ERR);
        }
    }

    /* alloc tile index map in SCU unit */
    if (ctx->map_tidx == NULL)
    {
        size = sizeof(u8) * ctx->f_scu;
        ctx->map_tidx = (u8*)xeve_malloc_fast(size);
        xeve_assert_gv(ctx->map_tidx, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset_x64a(ctx->map_tidx, 0, size);
    }

    if (ctx->tile == NULL)
    {
        ret = set_tile_info(ctx);
        if (ret != XEVE_OK)
        {
            goto ERR;
        }
    }

    size = ctx->f_lcu * sizeof(int);
    ctx->sync_flag = (volatile s32 *)xeve_malloc(size);
    for (int i = 0; i < (int)ctx->f_lcu; i++)
    {
        ctx->sync_flag[i] = 0;
    }

    return XEVE_OK;
ERR:
    for (i = 0; i < (int)ctx->f_lcu; i++)
    {
        xeve_delete_cu_data(ctx->map_cu_data + i, ctx->log2_max_cuwh - MIN_CU_LOG2, ctx->log2_max_cuwh - MIN_CU_LOG2);
    }

    xeve_mfree_fast(ctx->map_cu_data);
    xeve_mfree_fast(ctx->map_ipm);
    xeve_mfree_fast(ctx->map_depth);
    xeve_mfree_fast(ctx->map_cu_mode);

//free the threadpool and created thread if any
    if (ctx->sync_block)
    {
        release_synchornized_object(&ctx->sync_block);
    }

    if (ctx->cdsc.parallel_task_cnt >= 1)
    {
        if(ctx->tc)
        {
            //thread controller instance is present
            //terminate the created thread
            for (int i = 0; i < ctx->cdsc.parallel_task_cnt; i++)
            {
                if(ctx->thread_pool[i])
                {
                    //valid thread instance
                    ctx->tc->release(&ctx->thread_pool[i]);
                }
            }
            //dinitialize the tc
            dinit_thread_controller(ctx->tc);
            xeve_mfree_fast(ctx->tc);
            ctx->tc = 0;
        }
    }

    xeve_mfree_fast((void*) ctx->sync_flag);

    for (i = 0; i < ctx->pico_max_cnt; i++)
    {
        xeve_picbuf_rc_free(ctx->pico_buf[i]->spic);
        xeve_mfree_fast(ctx->pico_buf[i]->map_pdir);
        xeve_mfree_fast(ctx->pico_buf[i]->map_mv);
        xeve_mfree_fast(ctx->pico_buf[i]->map_mv_pga);
        xeve_mfree_fast(ctx->pico_buf[i]->map_lcu_cost_uni);
        xeve_mfree_fast(ctx->pico_buf[i]->map_lcu_cost_bi);
        if (ctx->param.use_dqp) {
            xeve_mfree_fast(ctx->pico_buf[i]->map_qp_offset);
            xeve_mfree_fast(ctx->pico_buf[i]->transfer_cost);
        }
        xeve_mfree_fast(ctx->pico_buf[i]);
    }

    if(core)
    {
        core_free(core);
    }

    return ret;
}

void xeve_flush(XEVE_CTX * ctx)
{
    int i;
    xeve_assert(ctx);

    xeve_mfree_fast(ctx->map_scu);
    for(i = 0; i < (int)ctx->f_lcu; i++)
    {
        xeve_delete_cu_data(ctx->map_cu_data + i, ctx->log2_max_cuwh - MIN_CU_LOG2, ctx->log2_max_cuwh - MIN_CU_LOG2);
    }
    xeve_mfree_fast(ctx->map_cu_data);
    xeve_mfree_fast(ctx->map_ipm);
    xeve_mfree_fast(ctx->map_depth);

    //release the sync block
    if (ctx->sync_block)
    {
        release_synchornized_object(&ctx->sync_block);
    }

    //Release thread pool controller and created threads
    if (ctx->cdsc.parallel_task_cnt >= 1)
    {
        if(ctx->tc)
        {
            //thread controller instance is present
            //terminate the created thread
            for (int i = 0; i < ctx->cdsc.parallel_task_cnt; i++)
            {
                if(ctx->thread_pool[i])
                {
                    //valid thread instance
                    ctx->tc->release(&ctx->thread_pool[i]);
                }
            }
            //dinitialize the tc
            dinit_thread_controller(ctx->tc);
            xeve_mfree_fast(ctx->tc);
            ctx->tc = 0;
        }
    }

        xeve_mfree_fast((void*) ctx->sync_flag);

    xeve_mfree_fast(ctx->map_cu_mode);
    xeve_picbuf_free(ctx->pic_dbk);
    xeve_picman_deinit(&ctx->rpm);

    for (int i = 0; i < ctx->cdsc.parallel_task_cnt; i++)
    {
        core_free(ctx->core[i]);
    }
    for (i = 0; i < ctx->pico_max_cnt; i++)
    {
        xeve_picbuf_rc_free(ctx->pico_buf[i]->spic);
        xeve_mfree_fast(ctx->pico_buf[i]->map_pdir);
        xeve_mfree_fast(ctx->pico_buf[i]->map_mv);
        xeve_mfree_fast(ctx->pico_buf[i]->map_mv_pga);
        xeve_mfree_fast(ctx->pico_buf[i]->map_lcu_cost_uni);
        xeve_mfree_fast(ctx->pico_buf[i]->map_lcu_cost_bi);
        if (ctx->param.use_dqp) {
            xeve_mfree_fast(ctx->pico_buf[i]->map_qp_offset);
            xeve_mfree_fast(ctx->pico_buf[i]->transfer_cost);
        }
        xeve_mfree_fast(ctx->pico_buf[i]);
    }
    for(i = 0; i < XEVE_MAX_INBUF_CNT; i++)
    {
        if(ctx->inbuf[i]) ctx->inbuf[i]->release(ctx->inbuf[i]);
    }
}

/* \todo: Remove Main codes */
void xeve_deblock_unit(XEVE_CTX * ctx, XEVE_PIC * pic, int x, int y, int cuw, int cuh, int is_hor_edge, XEVE_CORE * core, int boundary_filtering)
{
    int t = (x >> MIN_CU_LOG2) + (y >> MIN_CU_LOG2) * ctx->w_scu;

    if(is_hor_edge)
    {
        xeve_deblock_cu_hor(pic, x, y, cuw, cuh, ctx->map_scu, ctx->map_refi, ctx->map_unrefined_mv, ctx->w_scu, ctx->refp
                            , core->tree_cons, ctx->map_tidx, boundary_filtering
                            , ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8);
    }
    else
    {
        xeve_deblock_cu_ver(pic, x, y, cuw, cuh, ctx->map_scu, ctx->map_refi, ctx->map_unrefined_mv, ctx->w_scu
                            , ctx->map_cu_mode, ctx->refp, core->tree_cons, ctx->map_tidx, boundary_filtering
                            , ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8);
    }
}

static void deblock_tree(XEVE_CTX * ctx, XEVE_PIC * pic, int x, int y, int cuw, int cuh, int cud, int cup, int is_hor_edge
                       , TREE_CONS tree_cons, XEVE_CORE * core, int boundary_filtering)
{
    s8  split_mode;
    int lcu_num;

    core->tree_cons = tree_cons;
    pic->pic_deblock_alpha_offset = ctx->sh.sh_deblock_alpha_offset;
    pic->pic_deblock_beta_offset = ctx->sh.sh_deblock_beta_offset;
    pic->pic_qp_u_offset = ctx->sh.qp_u_offset;
    pic->pic_qp_v_offset = ctx->sh.qp_v_offset;

    lcu_num = (x >> ctx->log2_max_cuwh) + (y >> ctx->log2_max_cuwh) * ctx->w_lcu;
    xeve_get_split_mode(&split_mode, cud, cup, cuw, cuh, ctx->max_cuwh, ctx->map_cu_data[lcu_num].split_mode);

    if(split_mode != NO_SPLIT)
    {
        XEVE_SPLIT_STRUCT split_struct;
        xeve_split_get_part_structure( split_mode, x, y, cuw, cuh, cup, cud, ctx->log2_culine, &split_struct );

        split_struct.tree_cons = tree_cons;

        BOOL mode_cons_changed = FALSE;
        split_struct.tree_cons = xeve_get_default_tree_cons();

        for(int part_num = 0; part_num < split_struct.part_count; ++part_num)
        {
            int cur_part_num = part_num;
            int sub_cuw = split_struct.width[cur_part_num];
            int sub_cuh = split_struct.height[cur_part_num];
            int x_pos = split_struct.x_pos[cur_part_num];
            int y_pos = split_struct.y_pos[cur_part_num];

            if(x_pos < ctx->w && y_pos < ctx->h)
            {
                deblock_tree(ctx, pic, x_pos, y_pos, sub_cuw, sub_cuh, split_struct.cud[cur_part_num], split_struct.cup[cur_part_num], is_hor_edge
                    , split_struct.tree_cons, core, boundary_filtering);
            }

            core->tree_cons = tree_cons;
        }

    }

    if (split_mode == NO_SPLIT)
    {
        ctx->fn_deblock_unit(ctx, pic, x, y, cuw, cuh, is_hor_edge, core, boundary_filtering);
    }

    core->tree_cons = tree_cons;
}

int xeve_deblock(XEVE_CTX * ctx, XEVE_PIC * pic, int tile_idx, int filter_across_boundary , XEVE_CORE * core)
{
    int i, j;
    int x_l, x_r, y_l, y_r, l_scu, r_scu, t_scu, b_scu;
    u32 k1;
    int scu_in_lcu_wh = 1 << (ctx->log2_max_cuwh - MIN_CU_LOG2);
    int boundary_filtering = 0;
    x_l = (ctx->tile[tile_idx].ctba_rs_first) % ctx->w_lcu; //entry point lcu's x location
    y_l = (ctx->tile[tile_idx].ctba_rs_first) / ctx->w_lcu; // entry point lcu's y location
    x_r = x_l + ctx->tile[tile_idx].w_ctb;
    y_r = y_l + ctx->tile[tile_idx].h_ctb;
    l_scu = x_l * scu_in_lcu_wh;
    r_scu = XEVE_CLIP3(0, ctx->w_scu, x_r*scu_in_lcu_wh);
    t_scu = y_l * scu_in_lcu_wh;
    b_scu = XEVE_CLIP3(0, ctx->h_scu, y_r*scu_in_lcu_wh);

    /* Filtering tile boundaries in case of loop filter is enabled across tiles*/
    if (filter_across_boundary)
    {
        int boundary_filtering = 1;
        /*Horizontal filtering only at the top of the tile */
        j = t_scu;
        for (i = l_scu; i < r_scu; i++)
        {
            k1 = i + j * ctx->w_scu;
            MCU_CLR_COD(ctx->map_scu[k1]);
            if (!MCU_GET_DMVRF(ctx->map_scu[k1]))
            {
                ctx->map_unrefined_mv[k1][REFP_0][MV_X] = ctx->map_mv[k1][REFP_0][MV_X];
                ctx->map_unrefined_mv[k1][REFP_0][MV_Y] = ctx->map_mv[k1][REFP_0][MV_Y];
                ctx->map_unrefined_mv[k1][REFP_1][MV_X] = ctx->map_mv[k1][REFP_1][MV_X];
                ctx->map_unrefined_mv[k1][REFP_1][MV_Y] = ctx->map_mv[k1][REFP_1][MV_Y];
            }
        }

        /* horizontal filtering */
        j = y_l;
        for (i = x_l; i < x_r; i++)
        {
            deblock_tree(ctx, pic, (i << ctx->log2_max_cuwh), (j << ctx->log2_max_cuwh), ctx->max_cuwh, ctx->max_cuwh, 0, 0, 0/*0 - horizontal filtering of vertical edge*/
                       , xeve_get_default_tree_cons(), core, boundary_filtering);
        }
        /*Vertical filtering only at the left boundary of the tile */
        i = l_scu;
        for (j = t_scu; j < b_scu; j++)
        {
            MCU_CLR_COD(ctx->map_scu[i + j * ctx->w_scu]);
        }
        /* vertical filtering */
        i = x_l;
        for (j = y_l; j < y_r; j++)
        {
            deblock_tree(ctx, pic, (i << ctx->log2_max_cuwh), (j << ctx->log2_max_cuwh), ctx->max_cuwh, ctx->max_cuwh, 0, 0, 1/*1 - vertical filtering of horizontal edge*/
                       , xeve_get_default_tree_cons(), core, boundary_filtering);
        }
    }

    else
    /* Applying deblocking on the image except tile boundaries*/
    {

        for (j = t_scu; j < b_scu; j++)
        {
            for (i = l_scu; i < r_scu; i++)
            {
                k1 = i + j * ctx->w_scu;
                MCU_CLR_COD(ctx->map_scu[k1]);

                if (!MCU_GET_DMVRF(ctx->map_scu[k1]))
                {
                    ctx->map_unrefined_mv[k1][REFP_0][MV_X] = ctx->map_mv[k1][REFP_0][MV_X];
                    ctx->map_unrefined_mv[k1][REFP_0][MV_Y] = ctx->map_mv[k1][REFP_0][MV_Y];
                    ctx->map_unrefined_mv[k1][REFP_1][MV_X] = ctx->map_mv[k1][REFP_1][MV_X];
                    ctx->map_unrefined_mv[k1][REFP_1][MV_Y] = ctx->map_mv[k1][REFP_1][MV_Y];
                }
            }
        }

        /* horizontal filtering */
        for (j = y_l; j < y_r; j++)
        {
            for (i = x_l; i < x_r; i++)
            {
                deblock_tree(ctx, pic, (i << ctx->log2_max_cuwh), (j << ctx->log2_max_cuwh), ctx->max_cuwh, ctx->max_cuwh, 0, 0, 0/*0 - horizontal filtering of vertical edge*/
                    , xeve_get_default_tree_cons(), core, boundary_filtering);
            }
        }

        for (j = t_scu; j < b_scu; j++)
        {
            for (i = l_scu; i < r_scu; i++)
            {
                MCU_CLR_COD(ctx->map_scu[i + j * ctx->w_scu]);
            }
        }

        /* vertical filtering */
        for (j = y_l; j < y_r; j++)
        {
            for (i = x_l; i < x_r; i++)
            {

                deblock_tree(ctx, pic, (i << ctx->log2_max_cuwh), (j << ctx->log2_max_cuwh), ctx->max_cuwh, ctx->max_cuwh, 0, 0, 1/*1 - vertical filtering of horizontal edge*/
                    , xeve_get_default_tree_cons(), core, boundary_filtering);
            }
        }
    }
    return XEVE_OK;
}

int xeve_picbuf_get_inbuf(XEVE_CTX * ctx, XEVE_IMGB ** imgb)
{
    int i, opt, align[XEVE_IMGB_MAX_PLANE], pad[XEVE_IMGB_MAX_PLANE];

    for(i = 0; i < XEVE_MAX_INBUF_CNT; i++)
    {
        if(ctx->inbuf[i] == NULL)
        {
            opt = XEVE_IMGB_OPT_NONE;

            /* set align value*/
            align[0] = MIN_CU_SIZE;
            align[1] = MIN_CU_SIZE >> 1;
            align[2] = MIN_CU_SIZE >> 1;

            /* no padding */
            pad[0] = 0;
            pad[1] = 0;
            pad[2] = 0;

            *imgb = xeve_imgb_create(ctx->param.w, ctx->param.h, XEVE_CS_YCBCR420_10LE, opt, pad, align);
            xeve_assert_rv(*imgb != NULL, XEVE_ERR_OUT_OF_MEMORY);

            ctx->inbuf[i] = *imgb;

            (*imgb)->addref(*imgb);
            return XEVE_OK;
        }
        else if(ctx->inbuf[i]->getref(ctx->inbuf[i]) == 1)
        {
            *imgb = ctx->inbuf[i];

            (*imgb)->addref(*imgb);
            return XEVE_OK;
        }
    }

    return XEVE_ERR_UNEXPECTED;
}

int xeve_header(XEVE_CTX * ctx)
{
    int ret = XEVE_OK;

    /* encode parameter sets */
    if (ctx->pic_cnt == 0 || (ctx->slice_type == SLICE_I && ctx->param.use_closed_gop)) /* if nalu_type is IDR */
    {
        ret = xeve_encode_sps(ctx);
        xeve_assert_rv(ret == XEVE_OK, ret);

        ret = xeve_encode_pps(ctx);
        xeve_assert_rv(ret == XEVE_OK, ret);
    }

    return ret;
}

static void decide_normal_gop(XEVE_CTX * ctx, u32 pic_imcnt)
{
    int i_period, gop_size, pos;
    u32        pic_icnt_b;

    i_period = ctx->param.i_period;
    gop_size = ctx->param.gop_size;

    if(i_period == 0 && pic_imcnt == 0)
    {
        ctx->slice_type = SLICE_I;
        ctx->slice_depth = FRM_DEPTH_0;
        ctx->poc.poc_val = pic_imcnt;
        ctx->poc.prev_doc_offset = 0;
        ctx->poc.prev_poc_val = ctx->poc.poc_val;
        ctx->slice_ref_flag = 1;
    }
    else if((i_period != 0) && pic_imcnt % i_period == 0)
    {
        ctx->slice_type = SLICE_I;
        ctx->slice_depth = FRM_DEPTH_0;
        ctx->poc.poc_val = pic_imcnt;
        ctx->poc.prev_doc_offset = 0;
        ctx->poc.prev_poc_val = ctx->poc.poc_val;
        ctx->slice_ref_flag = 1;
    }
    else if(pic_imcnt % gop_size == 0)
    {
        ctx->slice_type = ctx->cdsc.inter_slice_type;
        ctx->slice_ref_flag = 1;
        ctx->slice_depth = FRM_DEPTH_1;
        ctx->poc.poc_val = pic_imcnt;
        ctx->poc.prev_doc_offset = 0;
        ctx->poc.prev_poc_val = ctx->poc.poc_val;
        ctx->slice_ref_flag = 1;
    }
    else
    {
        ctx->slice_type = ctx->cdsc.inter_slice_type;
        if(ctx->param.use_hgop)
        {
            pos = (pic_imcnt % gop_size) - 1;

            ctx->slice_depth = tbl_slice_depth[gop_size >> 2][pos];
            int tid = ctx->slice_depth - (ctx->slice_depth > 0);
            xeve_poc_derivation(ctx->sps, tid, &ctx->poc);
            ctx->poc.poc_val = ctx->poc.poc_val;

            if (!ctx->sps.tool_pocs && gop_size >= 2)
            {
                ctx->slice_ref_flag = (ctx->slice_depth == tbl_slice_depth[gop_size >> 2][gop_size - 2] ? 0 : 1);
            }
            else
            {
                ctx->slice_ref_flag = 1;
            }
        }
        else
        {
            pos = (pic_imcnt % gop_size) - 1;
            ctx->slice_depth = FRM_DEPTH_2;
            ctx->poc.poc_val = ((pic_imcnt / gop_size) * gop_size) - gop_size + pos + 1;
            ctx->slice_ref_flag = 0;
        }
        /* find current encoding picture's(B picture) pic_icnt */
        pic_icnt_b = ctx->poc.poc_val;

        /* find pico again here */
        ctx->pico_idx = (u8)(pic_icnt_b % ctx->pico_max_cnt);
        ctx->pico = ctx->pico_buf[ctx->pico_idx];

        PIC_ORIG(ctx) = &ctx->pico->pic;
    }
}

/* slice_type / slice_depth / poc / PIC_ORIG setting */
static void decide_slice_type(XEVE_CTX * ctx)
{
    u32 pic_imcnt, pic_icnt;
    int i_period, gop_size;
    int force_cnt = 0;

    i_period = ctx->param.i_period;
    gop_size = ctx->param.gop_size;
    pic_icnt = (ctx->pic_cnt + ctx->param.max_b_frames);
    pic_imcnt = pic_icnt;
    ctx->pico_idx = pic_icnt % ctx->pico_max_cnt;
    ctx->pico = ctx->pico_buf[ctx->pico_idx];
    PIC_ORIG(ctx) = &ctx->pico->pic;

    if(gop_size == 1)
    {
        if (i_period == 1) /* IIII... */
        {
            ctx->slice_type = SLICE_I;
            ctx->slice_depth = FRM_DEPTH_0;
            ctx->poc.poc_val = pic_icnt;
            ctx->slice_ref_flag = 0;
        }
        else /* IPPP... */
        {
            pic_imcnt = (i_period > 0) ? pic_icnt % i_period : pic_icnt;
            if (pic_imcnt == 0)
            {
                ctx->slice_type = SLICE_I;
                ctx->slice_depth = FRM_DEPTH_0;
                ctx->poc.poc_val = 0;
                ctx->slice_ref_flag = 1;
            }
            else
            {
                ctx->slice_type = ctx->cdsc.inter_slice_type;

                if (ctx->param.use_hgop)
                {
                    ctx->slice_depth = tbl_slice_depth_P[ctx->param.ref_pic_gap_length >> 2][(pic_imcnt - 1) % ctx->param.ref_pic_gap_length];
                }
                else
                {
                    ctx->slice_depth = FRM_DEPTH_1;
                }
                ctx->poc.poc_val = (i_period > 0) ? ctx->pic_cnt % i_period : ctx->pic_cnt;
                ctx->slice_ref_flag = 1;
            }
        }
    }
    else /* include B Picture (gop_size = 2 or 4 or 8 or 16) */
    {
        if(pic_icnt == gop_size - 1) /* special case when sequence start */
        {
            ctx->slice_type = SLICE_I;
            ctx->slice_depth = FRM_DEPTH_0;
            ctx->poc.poc_val = 0;
            ctx->poc.prev_doc_offset = 0;
            ctx->poc.prev_poc_val = ctx->poc.poc_val;
            ctx->slice_ref_flag = 1;

            /* flush the first IDR picture */
            PIC_ORIG(ctx) = &ctx->pico_buf[0]->pic;
            ctx->pico = ctx->pico_buf[0];
        }
        else if(ctx->force_slice)
        {
            for(force_cnt = ctx->force_ignored_cnt; force_cnt < gop_size; force_cnt++)
            {
                pic_icnt = (ctx->pic_cnt + ctx->param.max_b_frames + force_cnt);
                pic_imcnt = pic_icnt;

                decide_normal_gop(ctx, pic_imcnt);

                if(ctx->poc.poc_val <= (int)ctx->pic_ticnt)
                {
                    break;
                }
            }
            ctx->force_ignored_cnt = force_cnt;
        }
        else /* normal GOP case */
        {
            decide_normal_gop(ctx, pic_imcnt);
        }
    }
    if (ctx->param.use_hgop && gop_size > 1)
    {
        ctx->nalu.nuh_temporal_id = ctx->slice_depth - (ctx->slice_depth > 0);
    }
    else
    {
        ctx->nalu.nuh_temporal_id = 0;
    }
}

int xeve_pic_prepare(XEVE_CTX * ctx, XEVE_BITB * bitb, XEVE_STAT * stat)
{
    XEVE_PARAM   * param;
    int             ret;
    int             size;

    xeve_assert_rv(PIC_ORIG(ctx) != NULL, XEVE_ERR_UNEXPECTED);

    param = &ctx->param;
    ret = set_enc_param(ctx, param);
    xeve_assert_rv(ret == XEVE_OK, ret);

    PIC_CURR(ctx) = xeve_picman_get_empty_pic(&ctx->rpm, &ret);
    xeve_assert_rv(PIC_CURR(ctx) != NULL, ret);
    ctx->map_refi = PIC_CURR(ctx)->map_refi;
    ctx->map_mv = PIC_CURR(ctx)->map_mv;
    ctx->map_unrefined_mv = PIC_CURR(ctx)->map_unrefined_mv;

    PIC_MODE(ctx) = PIC_CURR(ctx);
    if(ctx->pic_dbk == NULL)
    {
        ctx->pic_dbk = xeve_pic_alloc(&ctx->rpm.pa, &ret);
        xeve_assert_rv(ctx->pic_dbk != NULL, ret);
    }

    decide_slice_type(ctx);

    ctx->lcu_cnt = ctx->f_lcu;
    ctx->slice_num = 0;

    if (ctx->tile_cnt == 1 && ctx->cdsc.parallel_task_cnt > 1)
    {
        for (u32 i = 0; i < ctx->f_lcu; i++)
        {
            ctx->sync_flag[i] = 0; //Reset the sync flag at the begining of each frame
        }
    }

    if(ctx->slice_type == SLICE_I) ctx->last_intra_poc = ctx->poc.poc_val;

    size = sizeof(s8) * ctx->f_scu * REFP_NUM;
    xeve_mset_x64a(ctx->map_refi, -1, size);
    size = sizeof(s16) * ctx->f_scu * REFP_NUM * MV_D;
    xeve_mset_x64a(ctx->map_mv, 0, size);
    size = sizeof(s16) * ctx->f_scu * REFP_NUM * MV_D;
    xeve_mset_x64a(ctx->map_unrefined_mv, 0, size);

    /* initialize bitstream container */
    xeve_bsw_init(&ctx->bs[0], bitb->addr, bitb->bsize, NULL);
    ctx->bs[0].pdata[1] = &ctx->sbac_enc[0];
    for (int i = 0; i < ctx->cdsc.parallel_task_cnt; i++)
    {
        xeve_bsw_init(&ctx->bs[i], ctx->bs[i].beg, bitb->bsize, NULL);
    }

    /* clear map */
    xeve_mset_x64a(ctx->map_scu, 0, sizeof(u32) * ctx->f_scu);
    xeve_mset_x64a(ctx->map_cu_mode, 0, sizeof(u32) * ctx->f_scu);

    set_active_pps_info(ctx);
    if (ctx->param.rc_type != 0)
    {
        xeve_rc_frame_est(ctx);
        ctx->qp = xeve_rc_get_qp(ctx);
    }

    return XEVE_OK;
}

int xeve_pic_finish(XEVE_CTX *ctx, XEVE_BITB *bitb, XEVE_STAT *stat)
{
    XEVE_IMGB *imgb_o, *imgb_c;
    int        ret;
    int        i, j;

    xeve_mset(stat, 0, sizeof(XEVE_STAT));

    /* adding picture sign */
    if (ctx->param.use_pic_sign)
    {
        XEVE_BSW * bs = &ctx->bs[0];
        XEVE_NALU sei_nalu;
        set_nalu(&sei_nalu, XEVE_SEI_NUT, ctx->nalu.nuh_temporal_id);

        int* size_field = (int*)(*(&bs->cur));
        u8* cur_tmp = bs->cur;

        xeve_eco_nalu(bs, &sei_nalu);

        ret = xeve_eco_sei(ctx, bs);
        xeve_assert_rv(ret == XEVE_OK, ret);

        xeve_bsw_deinit(bs);
        stat->sei_size = (int)(bs->cur - cur_tmp);
        *size_field = stat->sei_size - 4;
    }

    /* expand current encoding picture, if needs */
    ctx->fn_picbuf_expand(ctx, PIC_CURR(ctx));

    /* picture buffer management */
    ret = xeve_picman_put_pic(&ctx->rpm, PIC_CURR(ctx), ctx->nalu.nal_unit_type_plus1 - 1 == XEVE_IDR_NUT,
                              ctx->poc.poc_val, ctx->nalu.nuh_temporal_id, 0, ctx->refp,
                              ctx->slice_ref_flag, ctx->sps.tool_rpl, ctx->ref_pic_gap_length);

    xeve_assert_rv(ret == XEVE_OK, ret);

    imgb_o = PIC_ORIG(ctx)->imgb;
    xeve_assert(imgb_o != NULL);

    imgb_c = PIC_CURR(ctx)->imgb;
    xeve_assert(imgb_c != NULL);

    /* set stat */
    stat->write = XEVE_BSW_GET_WRITE_BYTE(&ctx->bs[0]);
    stat->nalu_type = ctx->slice_type == SLICE_I ? XEVE_IDR_NUT : XEVE_NONIDR_NUT;
    stat->stype = ctx->slice_type;
    stat->fnum = ctx->pic_cnt;
    stat->qp = ctx->sh.qp;
    stat->poc = ctx->poc.poc_val;
    stat->tid = ctx->nalu.nuh_temporal_id;

    for(i = 0; i < 2; i++)
    {
        stat->refpic_num[i] = ctx->rpm.num_refp[i];
        for (j = 0; j < stat->refpic_num[i]; j++)
        {
            stat->refpic[i][j] = ctx->refp[j][i].poc;
        }
    }

    ctx->pic_cnt++; /* increase picture count */
    ctx->param.f_ifrm = 0; /* clear force-IDR flag */
    ctx->pico->is_used = 0;

    imgb_c->ts[0] = bitb->ts[0] = imgb_o->ts[0];
    imgb_c->ts[1] = bitb->ts[1] = imgb_o->ts[1];
    imgb_c->ts[2] = bitb->ts[2] = imgb_o->ts[2];
    imgb_c->ts[3] = bitb->ts[3] = imgb_o->ts[3];

    if (ctx->cdsc.rc_type != 0)
    {
        ctx->rcore->real_bits = stat->write << 3;
    }

    if(imgb_o) imgb_o->release(imgb_o);

    return XEVE_OK;
}

static void update_core_loc_param(XEVE_CTX * ctx, XEVE_CORE * core)
{
    core->x_pel = core->x_lcu << ctx->log2_max_cuwh;  // entry point's x location in pixel
    core->y_pel = core->y_lcu << ctx->log2_max_cuwh;  // entry point's y location in pixel
    core->x_scu = core->x_lcu << (MAX_CU_LOG2 - MIN_CU_LOG2); // set x_scu location
    core->y_scu = core->y_lcu << (MAX_CU_LOG2 - MIN_CU_LOG2); // set y_scu location
    core->lcu_num = core->x_lcu + core->y_lcu*ctx->w_lcu; // Init the first lcu_num in tile
}

/* updating core location parameters for CTU parallel encoding case*/
static void update_core_loc_param1(XEVE_CTX * ctx, XEVE_CORE * core)
{
    core->x_pel = core->x_lcu << ctx->log2_max_cuwh;  // entry point's x location in pixel
    core->y_pel = core->y_lcu << ctx->log2_max_cuwh;  // entry point's y location in pixel
    core->x_scu = core->x_lcu << (MAX_CU_LOG2 - MIN_CU_LOG2); // set x_scu location
    core->y_scu = core->y_lcu << (MAX_CU_LOG2 - MIN_CU_LOG2); // set y_scu location
}

static int mt_get_next_ctu_num(XEVE_CTX * ctx, XEVE_CORE * core, int skip_ctb_line_cnt)
{
    core->lcu_num++;
    core->x_lcu = (core->lcu_num) % ctx->w_lcu; //entry point lcu's x location

    /* check to move next ctb line */
    if (core->x_lcu == 0)
    {
        core->lcu_num += ctx->w_lcu * skip_ctb_line_cnt;
    }

    core->y_lcu = core->lcu_num / ctx->w_lcu;
    /* check to exceed height of ctb line */
    if (core->y_lcu >= ctx->h_lcu)
    {
        return -1;
    }
    update_core_loc_param1(ctx, core);

    return core->lcu_num;
}

static int ctu_mt_core(void * arg)
{
    XEVE_BSW   * bs;
    XEVE_SH    * sh;
    XEVE_CORE * core;

    assert(arg != NULL);
    core = (XEVE_CORE *)arg;

    XEVE_CTX * ctx = core->ctx;
    int idx;
    int ctb_cnt_in_row, ret;
    bs = &ctx->bs[core->thread_cnt];
    sh = &ctx->sh;
    int i = core->tile_num;

    /* CABAC Initialize for each Tile */
    ctx->fn_eco_sbac_reset(GET_SBAC_ENC(bs), ctx->sh.slice_type, ctx->sh.qp, ctx->sps.tool_cm_init);
    ctx->fn_eco_sbac_reset(&core->s_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2], ctx->sh.slice_type, ctx->sh.qp, ctx->sps.tool_cm_init);

    /*Set entry point for each ctu row in the tile*/
    core->x_lcu = (core->lcu_num) % ctx->w_lcu; //entry point lcu's x location
    core->y_lcu = (core->lcu_num) / ctx->w_lcu; // entry point lcu's y location
    ctb_cnt_in_row = ctx->tile[i].w_ctb; //Total LCUs in the current row
    update_core_loc_param1(ctx, core);

    int bef_cu_qp = ctx->tile[i].qp_prev_eco;

    /* LCU encoding loop */
    while (ctx->tile[i].f_ctb > 0)
    {
        if (core->y_lcu != 0 && core->x_lcu < (ctb_cnt_in_row - 1))
        {
            idx = core->lcu_num - ctx->w_lcu + 1; /* up-right CTB */
            spinlock_wait(&ctx->sync_flag[idx], THREAD_TERMINATED);
        }

        /* initialize structures *****************************************/
        ret = ctx->fn_mode_init_lcu(ctx, core);
        xeve_assert_rv(ret == XEVE_OK, ret);

        /* mode decision *************************************************/
        SBAC_LOAD(core->s_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2], *GET_SBAC_ENC(bs));
        core->s_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].is_bitcount = 1;

        ret = ctx->fn_mode_analyze_lcu(ctx, core);
        xeve_assert_rv(ret == XEVE_OK, ret);

        ret = ctx->fn_mode_post_lcu(ctx, core);
        xeve_assert_rv(ret == XEVE_OK, ret)

        ctx->tile[i].qp_prev_eco = bef_cu_qp;

        /* entropy coding ************************************************/
        ret = xeve_eco_tree(ctx, core, core->x_pel, core->y_pel, 0, ctx->max_cuwh, ctx->max_cuwh, 0, 0, xeve_get_default_tree_cons(), bs);
        bef_cu_qp = ctx->tile[i].qp_prev_eco;

        xeve_assert_rv(ret == XEVE_OK, ret);

        threadsafe_assign(&ctx->sync_flag[core->lcu_num], THREAD_TERMINATED);
        threadsafe_decrement(ctx->sync_block, (volatile s32 *)&ctx->tile[i].f_ctb);

        core->lcu_num = mt_get_next_ctu_num(ctx, core, ctx->parallel_rows - 1);
        if (core->lcu_num == -1)
            break;
    }
    return XEVE_OK;
}

static int tile_mt_core(void * arg)
{
    XEVE_BSW   * bs;
    XEVE_SH    * sh;
    XEVE_CORE * core;

    assert(arg != NULL);
    core = (XEVE_CORE *)arg;

    XEVE_CTX * ctx = core->ctx;
    int ctb_cnt_in_tile, col_bd, ret;
    bs = &ctx->bs[core->thread_cnt];
    sh = &ctx->sh;
    int i = core->tile_num;

    /* CABAC Initialize for each Tile */
    ctx->fn_eco_sbac_reset(GET_SBAC_ENC(bs), ctx->sh.slice_type, ctx->sh.qp, ctx->sps.tool_cm_init);
    ctx->fn_eco_sbac_reset(&core->s_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2], ctx->sh.slice_type, ctx->sh.qp, ctx->sps.tool_cm_init);

    /*Set entry point for each Tile in the tile Slice*/
    core->x_lcu = (ctx->tile[i].ctba_rs_first) % ctx->w_lcu; //entry point lcu's x location
    core->y_lcu = (ctx->tile[i].ctba_rs_first) / ctx->w_lcu; // entry point lcu's y location
    ctb_cnt_in_tile = ctx->tile[i].f_ctb; //Total LCUs in the current tile
    update_core_loc_param(ctx, core);

    int bef_cu_qp = ctx->tile[i].qp_prev_eco;

    col_bd = 0;
    if (i% ctx->param.tile_columns)
    {
        int temp = i - 1;
        while (temp >= 0)
        {
            col_bd += ctx->tile[temp].w_ctb;
            if (!(temp%ctx->param.tile_columns)) break;
            temp--;
        }
    }
    else
    {
        col_bd = 0;
    }

    /* LCU decoding loop */
    while (1)
    {
        /* initialize structures *****************************************/
        ret = ctx->fn_mode_init_lcu(ctx, core);
        xeve_assert_rv(ret == XEVE_OK, ret);

        /* mode decision *************************************************/
        SBAC_LOAD(core->s_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2], *GET_SBAC_ENC(bs));
        core->s_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].is_bitcount = 1;

        ret = ctx->fn_mode_analyze_lcu(ctx, core);
        xeve_assert_rv(ret == XEVE_OK, ret);

        ret = ctx->fn_mode_post_lcu(ctx, core);
        xeve_assert_rv(ret == XEVE_OK, ret);

        ctx->tile[i].qp_prev_eco = bef_cu_qp;

        /* entropy coding ************************************************/
        ret = xeve_eco_tree(ctx, core, core->x_pel, core->y_pel, 0, ctx->max_cuwh, ctx->max_cuwh, 0, 0, xeve_get_default_tree_cons(), bs);
        bef_cu_qp = ctx->tile[i].qp_prev_eco;

        xeve_assert_rv(ret == XEVE_OK, ret);
        /* prepare next step *********************************************/

        core->x_lcu++;
        if (core->x_lcu >= ctx->tile[i].w_ctb + col_bd)
        {
            core->x_lcu = (ctx->tile[i].ctba_rs_first) % ctx->w_lcu;
            core->y_lcu++;
        }

        update_core_loc_param(ctx, core);
        ctb_cnt_in_tile--;

        if (ctb_cnt_in_tile == 0)
        {
            xeve_eco_tile_end_flag(bs, 1);
            xeve_sbac_finish(bs);
            break;
        }
    }
    return XEVE_OK;
}

static void init_log_lut(XEVE_PIBC *pi)
{
    int size = sizeof(s8) * (MAX_CU_SIZE + 1);
    xeve_mset(pi->ctu_log2_tbl, 0, size);
    int c = 0;
    for (int i = 0, n = 0; i <= MAX_CU_SIZE; i++)
    {
        if (i == (1 << n))
        {
            c = n;
            n++;
        }

        pi->ctu_log2_tbl[i] = c;
    }
}

static int initialize_tile(XEVE_CTX * ctx, int tile_num, XEVE_CORE * core, int thread_cnt)
{
    ctx->fn_mode_init_tile(ctx, thread_cnt);

    /********************* Core initialization *****************************/
    ctx->core[thread_cnt]->tile_num = tile_num;
    ctx->core[thread_cnt]->qp_y = core->qp_y;
    ctx->core[thread_cnt]->qp_u = core->qp_u;
    ctx->core[thread_cnt]->qp_v = core->qp_v;
    ctx->sh.qp_prev_eco = ctx->sh.qp;
    ctx->sh.qp_prev_mode = ctx->sh.qp;
    ctx->core[thread_cnt]->dqp_data[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].prev_qp = ctx->sh.qp_prev_mode;
    ctx->core[thread_cnt]->dqp_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].curr_qp = ctx->sh.qp;
    ctx->core[thread_cnt]->dqp_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].prev_qp = ctx->sh.qp;
    ctx->core[thread_cnt]->ctx = ctx;
    ctx->core[thread_cnt]->bs_temp.pdata[1] = &ctx->core[thread_cnt]->s_temp_run;

    return XEVE_OK;
}

static int xeve_deblock_mt(void * arg)
{
    int filter_across_boundary = 0;
    XEVE_CORE * core = (XEVE_CORE *)arg;
    XEVE_CTX * ctx = core->ctx;
    int i = core->tile_num;
    ctx->fn_deblock(ctx, PIC_MODE(ctx), i, filter_across_boundary, core);
    return XEVE_OK;
}

int xeve_loop_filter(XEVE_CTX * ctx, XEVE_CORE * core)
{
    int ret = XEVE_OK;

    if (ctx->sh.deblocking_filter_on)
    {
#if TRACE_DBF
        XEVE_TRACE_SET(1);
#endif
        u16 total_tiles_in_slice = ctx->sh.num_tiles_in_slice;
        THREAD_CONTROLLER * tc;
        int res;
        int i, k = 0;
        tc = ctx->tc;
        int parallel_task = 1;
        int thread_cnt = 0, thread_cnt1 = 0;;
        int task_completed = 0;

        while (total_tiles_in_slice)
        {
            parallel_task = (ctx->cdsc.parallel_task_cnt > total_tiles_in_slice) ? total_tiles_in_slice : ctx->cdsc.parallel_task_cnt;
            for (thread_cnt = 0; (thread_cnt < parallel_task - 1); thread_cnt++)
            {
                i = ctx->tiles_in_slice[thread_cnt + task_completed];
                ctx->core[thread_cnt]->thread_cnt = thread_cnt;
                ctx->core[thread_cnt]->tile_num = i;

                tc->run(ctx->thread_pool[thread_cnt], xeve_deblock_mt, (void*)ctx->core[thread_cnt]);
            }
            i = ctx->tiles_in_slice[thread_cnt + task_completed];
            ctx->core[thread_cnt]->thread_cnt = thread_cnt;
            ctx->core[thread_cnt]->tile_num = i;

            xeve_deblock_mt((void*)ctx->core[thread_cnt]);
            for (thread_cnt1 = 0; thread_cnt1 < parallel_task - 1; thread_cnt1++)
            {
                tc->join(ctx->thread_pool[thread_cnt1], &res);
                if (XEVE_FAILED(res))
                {
                    ret = res;
                }
            }
            total_tiles_in_slice -= parallel_task;
            task_completed += parallel_task;
        }
        total_tiles_in_slice = ctx->sh.num_tiles_in_slice;

        if (ctx->pps.loop_filter_across_tiles_enabled_flag)
        {
            /* Peform deblocking across tile boundaries*/
            k = 0;
            int filter_across_boundary = 1;
            total_tiles_in_slice = ctx->sh.num_tiles_in_slice;
            while (total_tiles_in_slice)
            {
                int i = ctx->tiles_in_slice[k++];
                ret = ctx->fn_deblock(ctx, PIC_MODE(ctx), i, filter_across_boundary, core);
                xeve_assert_rv(ret == XEVE_OK, ret);
                total_tiles_in_slice--;
            }
        }
#if TRACE_DBF
        XEVE_TRACE_SET(0);
#endif
    }

    return ret;
}

int xeve_pic(XEVE_CTX * ctx, XEVE_BITB * bitb, XEVE_STAT * stat)
{
    XEVE_CORE     * core;
    XEVE_BSW      * bs;
    XEVE_SH       * sh;
    XEVE_APS      * aps;
    int             ret;
    u32             i;
    int             ctb_cnt_in_tile = 0;
    int             col_bd = 0;
    int             num_slice_in_pic = ctx->param.num_slice_in_pic;
    u8            * tiles_in_slice, total_tiles_in_slice, total_tiles_in_slice_copy;
    int             tile_cnt = 0;
    u8            * curr_temp = ctx->bs[0].cur;;

    for (ctx->slice_num = 0; ctx->slice_num < num_slice_in_pic; ctx->slice_num++)
    {
        if (num_slice_in_pic > 1)
        {
            if (!ctx->param.arbitrary_slice_flag)
            {
                int first_tile_in_slice, last_tile_in_slice, first_tile_col_idx, last_tile_col_idx, delta_tile_idx;
                int w_tile, w_tile_slice, h_tile_slice;

                w_tile = ctx->param.tile_columns;
                first_tile_in_slice = ctx->param.slice_boundary_array[ctx->slice_num * 2];
                last_tile_in_slice = ctx->param.slice_boundary_array[ctx->slice_num * 2 + 1];

                first_tile_col_idx = first_tile_in_slice % w_tile;
                last_tile_col_idx = last_tile_in_slice % w_tile;
                delta_tile_idx = last_tile_in_slice - first_tile_in_slice;

                if (last_tile_in_slice < first_tile_in_slice)
                {
                    if (first_tile_col_idx > last_tile_col_idx)
                    {
                        delta_tile_idx += ctx->tile_cnt + w_tile;
                    }
                    else
                    {
                        delta_tile_idx += ctx->tile_cnt;
                    }
                }
                else if (first_tile_col_idx > last_tile_col_idx)
                {
                    delta_tile_idx += w_tile;
                }

                w_tile_slice = (delta_tile_idx % w_tile) + 1; //Number of tiles in slice width
                h_tile_slice = (delta_tile_idx / w_tile) + 1;
                total_tiles_in_slice = w_tile_slice * h_tile_slice;
                total_tiles_in_slice_copy = total_tiles_in_slice;
                for (u32 k = 0; k < total_tiles_in_slice; k++)
                {
                    ctx->tiles_in_slice[k] = ctx->tile_order[tile_cnt++];
                }
            }
            else
            {
                total_tiles_in_slice = ctx->param.num_remaining_tiles_in_slice_minus1[ctx->slice_num] + 2;
                int bef_tile_num = 0;
                for (int i = 0; i < ctx->slice_num; ++i)
                {
                    bef_tile_num += ctx->param.num_remaining_tiles_in_slice_minus1[i] + 2;
                }
                for (u32 k = 0; k < total_tiles_in_slice; k++)
                {
                    ctx->tiles_in_slice[k] = ctx->param.tile_array_in_slice[bef_tile_num + k];
                }
                total_tiles_in_slice_copy = total_tiles_in_slice;
            }
        }
        else
        {
            if (ctx->param.arbitrary_slice_flag)
            {
                total_tiles_in_slice = ctx->param.num_remaining_tiles_in_slice_minus1[ctx->slice_num] + 2;
                int bef_tile_num = 0;
                for (int i = 0; i < ctx->slice_num; ++i)
                {
                    bef_tile_num += ctx->param.num_remaining_tiles_in_slice_minus1[i] + 2;
                }
                for (u32 k = 0; k < total_tiles_in_slice; k++)
                {
                    ctx->tiles_in_slice[k] = ctx->param.tile_array_in_slice[bef_tile_num + k];
                }
                total_tiles_in_slice_copy = total_tiles_in_slice;
            }
            else
            {
                total_tiles_in_slice = 0;
                for (u32 k = 0; k < ctx->tile_cnt; k++)
                {
                    ctx->tiles_in_slice[total_tiles_in_slice] = k;
                    total_tiles_in_slice++;
                }
                total_tiles_in_slice_copy = total_tiles_in_slice;
            }
        }
        tiles_in_slice = ctx->tiles_in_slice;

        bs = &ctx->bs[0];
        core = ctx->core[0];
        sh = &ctx->sh;
        core->ctx = ctx;

        sh->num_tiles_in_slice = total_tiles_in_slice;
        aps = &ctx->aps;
        aps_counter_reset = FALSE;

        if ((int)ctx->poc.poc_val > last_intra_poc)
        {
            last_intra_poc = INT_MAX;
            aps_counter_reset = TRUE;
        }

        if (ctx->slice_type == SLICE_I)
        {
            last_intra_poc = ctx->poc.poc_val;
            ctx->aps_counter = -1;
            aps->aps_id = -1;

            ctx->sh.aps_signaled = -1; // reset stored aps id in tile group header
            ctx->aps_temp = 0;
        }

        if (aps_counter_reset)
        {
            ctx->aps_counter = 0;
        }

        /* Set slice header */
        set_sh(ctx, sh);

        /* initialize reference pictures */
        ret = xeve_picman_refp_init(&ctx->rpm, ctx->sps.max_num_ref_pics, ctx->slice_type, ctx->poc.poc_val, ctx->nalu.nuh_temporal_id, ctx->last_intra_poc, ctx->refp);
        xeve_assert_rv(ret == XEVE_OK, ret);

        ctx->fn_mode_analyze_frame(ctx);

        /* slice layer encoding loop */
        core->x_lcu = core->y_lcu = 0;
        core->x_pel = core->y_pel = 0;
        core->lcu_num = 0;
        ctx->lcu_cnt = ctx->f_lcu;

        /* Set nalu header */
        set_nalu(&ctx->nalu, ctx->pic_cnt == 0 || (ctx->slice_type == SLICE_I && ctx->param.use_closed_gop) ? XEVE_IDR_NUT : XEVE_NONIDR_NUT, ctx->nalu.nuh_temporal_id);

        core->qp_y = ctx->sh.qp + 6 * ctx->sps.bit_depth_luma_minus8;
        core->qp_u = xeve_qp_chroma_dynamic[0][sh->qp_u] + 6 * ctx->sps.bit_depth_chroma_minus8;
        core->qp_v = xeve_qp_chroma_dynamic[1][sh->qp_v] + 6 * ctx->sps.bit_depth_chroma_minus8;
        core->bs_temp.pdata[1] = &core->s_temp_run;

        /* LCU encoding */
#if TRACE_RDO_EXCLUDE_I
        if(ctx->slice_type != SLICE_I)
        {
#endif
        XEVE_TRACE_SET(0);
#if TRACE_RDO_EXCLUDE_I
        }
#endif
        if (ctx->sps.tool_mmvd && (ctx->slice_type == SLICE_B))
        {
            sh->mmvd_group_enable_flag = !(ctx->refp[0][0].poc == ctx->refp[0][1].poc);
        }
        else if (ctx->sps.tool_mmvd && (ctx->slice_type == SLICE_P))
        {
            sh->mmvd_group_enable_flag = 0;
        }
        else
        {
            sh->mmvd_group_enable_flag = 0;
        }

        ctx->sh.qp_prev_eco = ctx->sh.qp;
        ctx->sh.qp_prev_mode = ctx->sh.qp;
        core->dqp_data[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].prev_qp = ctx->sh.qp_prev_mode;
        core->dqp_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].curr_qp = ctx->sh.qp;
        core->dqp_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].prev_qp = ctx->sh.qp;

        /* Tile wise encoding with in a slice */
        u32 k = 0;
        total_tiles_in_slice = total_tiles_in_slice_copy;
        THREAD_CONTROLLER * tc;
        int res;
        i = 0;
        tc = ctx->tc;
        int parallel_task = 1;
        int thread_cnt = 0, thread_cnt1 = 0;;
        int task_completed = 0;
        int tile_cnt = 0;

        //Code for CTU parallel encoding
        while (total_tiles_in_slice)
        {
            //Limiting parallel task to the number of LCU rows
            i = tiles_in_slice[tile_cnt++];
            int temp_store_total_ctb = ctx->tile[i].f_ctb;
            parallel_task = (ctx->cdsc.parallel_task_cnt > ctx->tile[i].h_ctb) ? ctx->tile[i].h_ctb : ctx->cdsc.parallel_task_cnt;
            ctx->parallel_rows = parallel_task;
            ctx->tile[i].qp = ctx->sh.qp;
            ctx->tile[i].qp_prev_eco = ctx->sh.qp;

            for (thread_cnt = 1; (thread_cnt < parallel_task); thread_cnt++)
            {
                ctx->core[thread_cnt]->tile_idx = i;
                ctx->core[thread_cnt]->lcu_num = thread_cnt*ctx->tile[i].w_ctb;
                initialize_tile(ctx, i, core, thread_cnt);

                ctx->core[thread_cnt]->thread_cnt = thread_cnt;
                tc->run(ctx->thread_pool[thread_cnt], ctu_mt_core, (void*)ctx->core[thread_cnt]);
            }


            ctx->tile[i].qp = ctx->sh.qp;
            ctx->tile[i].qp_prev_eco = ctx->sh.qp;
            ctx->core[0]->tile_idx = i;
            ctx->core[0]->lcu_num = 0;

            initialize_tile(ctx, i, core, 0);

            ctx->core[0]->thread_cnt = 0;
            ctu_mt_core((void*)ctx->core[0]);

             for (thread_cnt1 = 1; thread_cnt1 < parallel_task; thread_cnt1++)
            {
                tc->join(ctx->thread_pool[thread_cnt1], &res);
                if (XEVE_FAILED(res))
                {
                    ret = res;
                }
            }

            ctx->tile[i].f_ctb = temp_store_total_ctb;


            /*Set entry point for each Tile in the tile Slice*/
            ctx->core[0]->x_lcu = (ctx->tile[i].ctba_rs_first) % ctx->w_lcu; //entry point lcu's x location
            ctx->core[0]->y_lcu = (ctx->tile[i].ctba_rs_first) / ctx->w_lcu; // entry point lcu's y location
            ctb_cnt_in_tile = ctx->tile[i].f_ctb; //Total LCUs in the current tile
            update_core_loc_param(ctx, ctx->core[0]);
            ctx->lcu_cnt = ctx->f_lcu;
            while (1)
            {
                /* entropy coding ************************************************/
                ret = xeve_eco_tree(ctx, ctx->core[0], ctx->core[0]->x_pel, ctx->core[0]->y_pel, 0, ctx->max_cuwh, ctx->max_cuwh, 0
                                  , 0, xeve_get_default_tree_cons(), &ctx->bs[0]);
                /* prepare next step *********************************************/
                ctx->core[0]->x_lcu++;
                if (ctx->core[0]->x_lcu >= ctx->tile[i].w_ctb + col_bd)
                {
                    ctx->core[0]->x_lcu = (ctx->tile[i].ctba_rs_first) % ctx->w_lcu;
                    ctx->core[0]->y_lcu++;
                }
                update_core_loc_param(ctx, ctx->core[0]);
                ctb_cnt_in_tile--;
                ctx->lcu_cnt--;
                if (ctb_cnt_in_tile == 0)
                {
                    xeve_eco_tile_end_flag(bs, 1);
                    xeve_sbac_finish(bs);
                    break;
                }
            }
            total_tiles_in_slice -= 1;
        }
        total_tiles_in_slice = total_tiles_in_slice_copy;

        ctx->fn_loop_filter(ctx, core);

        core->x_lcu = core->y_lcu = 0;
        core->x_pel = core->y_pel = 0;
        core->lcu_num = 0;
        ctx->lcu_cnt = ctx->f_lcu;
        for (i = 0; i < ctx->f_scu; i++)
        {
            MCU_CLR_COD(ctx->map_scu[i]);
        }

        ctx->sh.qp_prev_eco = ctx->sh.qp;

        k = 0;
        total_tiles_in_slice = total_tiles_in_slice_copy;
        while (total_tiles_in_slice)
        {
            int i = tiles_in_slice[k++];
            ctx->tile[i].qp = ctx->sh.qp;
            ctx->tile[i].qp_prev_eco = ctx->sh.qp;
            core->tile_idx = i;
            ctx->fn_eco_sbac_reset(GET_SBAC_ENC(bs), ctx->sh.slice_type, ctx->sh.qp, ctx->sps.tool_cm_init);
            core->x_lcu = (ctx->tile[i].ctba_rs_first) % ctx->w_lcu; //entry point lcu's x location
            core->y_lcu = (ctx->tile[i].ctba_rs_first) / ctx->w_lcu; // entry point lcu's y location
            ctb_cnt_in_tile = ctx->tile[i].f_ctb; //Total LCUs in the current tile
            update_core_loc_param(ctx, core);
            XEVE_BSW bs_beg;
            bs_beg.cur = bs->cur;
            bs_beg.leftbits = bs->leftbits;
            col_bd = 0;
            if (i% ctx->param.tile_columns)
            {
                int temp = i - 1;
                while (temp >= 0)
                {
                    col_bd += ctx->tile[temp].w_ctb;
                    if (!(temp%ctx->param.tile_columns)) break;
                    temp--;
                }
            }
            else
            {
                col_bd = 0;
            }
            while (1) // LCU level CABAC loop
            {
                ret = xeve_eco_tree(ctx, core, core->x_pel, core->y_pel, 0, ctx->max_cuwh, ctx->max_cuwh, 0, 0, xeve_get_default_tree_cons(), bs);
                xeve_assert_rv(ret == XEVE_OK, ret);
                core->x_lcu++;
                if (core->x_lcu >= ctx->tile[i].w_ctb + col_bd)
                {
                    core->x_lcu = (ctx->tile[i].ctba_rs_first) % ctx->w_lcu;
                    core->y_lcu++;
                }
                update_core_loc_param(ctx, core);
                ctb_cnt_in_tile--;
                ctx->lcu_cnt--;
                if (ctb_cnt_in_tile == 0)
                {
                    xeve_eco_tile_end_flag(bs, 1);
                    xeve_sbac_finish(bs);
                    break;
                }
            } //End of LCU encoding loop in a tile
            total_tiles_in_slice--;
            sh->entry_point_offset_minus1[k - 1] = (u32)((bs)->cur - bs_beg.cur - 4 + (4 - (bs->leftbits >> 3)) + (bs_beg.leftbits >> 3) - 1);
        } // End to tile encoding loop in a slice

        /* Bit-stream re-writing (START) */
        xeve_bsw_init_slice(&ctx->bs[0], (u8*)curr_temp, bitb->bsize, NULL);

#if TRACE_START_POC
        if (fp_trace_started == 1)
        {
            XEVE_TRACE_SET(1);
        }
        else
        {
            XEVE_TRACE_SET(0);
        }
#else
#if TRACE_RDO_EXCLUDE_I
        if(ctx->slice_type != SLICE_I)
        {
#endif
#if !TRACE_DBF
            XEVE_TRACE_SET(1);
#endif
#if TRACE_RDO_EXCLUDE_I
        }
#endif
#endif

        XEVE_SBAC* t_sbac;
        t_sbac = GET_SBAC_ENC(bs);
        t_sbac->bin_counter = 0;

        unsigned int bin_counts_in_units = 0;
        unsigned int num_bytes_in_units = 0;

        /* Send available APSs */
        int aps_nalu_size = 0;
        int* size_field = (int*)(*(&bs->cur));
        u8* cur_tmp = bs->cur;

        /* Encode nalu header */
        ret = xeve_eco_nalu(bs, &ctx->nalu);
        xeve_assert_rv(ret == XEVE_OK, ret);

        /* Encode slice header */
        sh->num_ctb = ctx->f_lcu;
        XEVE_BSW bs_sh;
        xeve_mcpy(&bs_sh, bs, sizeof(XEVE_BSW));
#if TRACE_HLS
        s32 tmp_fp_point = ftell(fp_trace);
#endif
        ret = ctx->fn_eco_sh(bs, &ctx->sps, &ctx->pps, sh, ctx->nalu.nal_unit_type_plus1 - 1);
        xeve_assert_rv(ret == XEVE_OK, ret);

        core->x_lcu = core->y_lcu = 0;
        core->x_pel = core->y_pel = 0;
        core->lcu_num = 0;
        ctx->lcu_cnt = ctx->f_lcu;
        for(i = 0; i < ctx->f_scu; i++)
        {
            MCU_CLR_COD(ctx->map_scu[i]);
        }
        ctx->sh.qp_prev_eco = ctx->sh.qp;

        /* Tile level encoding for a slice */
        /* Tile wise encoding with in a slice */
        k = 0;
        total_tiles_in_slice = total_tiles_in_slice_copy;
        while (total_tiles_in_slice)
        {
            int i = tiles_in_slice[k++];
            ctx->tile[i].qp = ctx->sh.qp;
            ctx->tile[i].qp_prev_eco = ctx->sh.qp;
            core->tile_idx = i;

            /* CABAC Initialize for each Tile */
            ctx->fn_eco_sbac_reset(GET_SBAC_ENC(bs), ctx->sh.slice_type, ctx->sh.qp, ctx->sps.tool_cm_init);

            /*Set entry point for each Tile in the tile Slice*/
            core->x_lcu = (ctx->tile[i].ctba_rs_first) % ctx->w_lcu; //entry point lcu's x location
            core->y_lcu = (ctx->tile[i].ctba_rs_first) / ctx->w_lcu; // entry point lcu's y location
            ctb_cnt_in_tile = ctx->tile[i].f_ctb; //Total LCUs in the current tile
            update_core_loc_param(ctx, core);

            XEVE_BSW bs_beg;
            bs_beg.cur = bs->cur;
            bs_beg.leftbits = bs->leftbits;

            col_bd = 0;
            if (i% ctx->param.tile_columns)
            {
                int temp = i - 1;
                while (temp >= 0)
                {
                    col_bd += ctx->tile[temp].w_ctb;
                    if (!(temp%ctx->param.tile_columns)) break;
                    temp--;
                }
            }
            else
            {
                col_bd = 0;
            }

            while (1) // LCU level CABAC loop
            {
                ret = xeve_eco_tree(ctx, core, core->x_pel, core->y_pel, 0, ctx->max_cuwh, ctx->max_cuwh, 0, 0, xeve_get_default_tree_cons(), bs);

                xeve_assert_rv(ret == XEVE_OK, ret);
                /* prepare next step *********************************************/
                core->x_lcu++;
                if (core->x_lcu >= ctx->tile[i].w_ctb + col_bd)
                {
                    core->x_lcu = (ctx->tile[i].ctba_rs_first) % ctx->w_lcu;
                    core->y_lcu++;
                }

                update_core_loc_param(ctx, core);
                ctb_cnt_in_tile--;
                ctx->lcu_cnt--;

                /* end_of_picture_flag */
                if (ctb_cnt_in_tile == 0)
                {
                    xeve_eco_tile_end_flag(bs, 1);
                    xeve_sbac_finish(bs);
                    break;
                }
            } //End of LCU encoding loop in a tile

            XEVE_SBAC* tmp_sbac;
            tmp_sbac = GET_SBAC_ENC(bs);
            bin_counts_in_units += tmp_sbac->bin_counter;
            total_tiles_in_slice--;

            sh->entry_point_offset_minus1[k - 1] = (u32)((bs)->cur - bs_beg.cur - 4 + (4 - (bs->leftbits >> 3)) + (bs_beg.leftbits >> 3) - 1);
        } // End to tile encoding loop in a slice

        num_bytes_in_units = (int)(bs->cur - cur_tmp) - 4;

        int log2_sub_wh_c = 2;
        int min_cu_w = ctx->min_cuwh;
        int min_cu_h = ctx->min_cuwh;
        int padded_w = ((ctx->w + min_cu_w - 1) / min_cu_w) * min_cu_w;
        int padded_h = ((ctx->h + min_cu_h - 1) / min_cu_h) * min_cu_h;
        int raw_bits = padded_w * padded_h * ((ctx->sps.bit_depth_luma_minus8 + 8) + 2 * ((ctx->sps.bit_depth_chroma_minus8 + 8) >> log2_sub_wh_c));
        unsigned int threshold = (CABAC_ZERO_PARAM / 3) * num_bytes_in_units + (raw_bits / 32);

        if (bin_counts_in_units >= threshold)
        {
            unsigned int target_num_bytes_in_units = ((bin_counts_in_units - (raw_bits / 32)) * 3 + (CABAC_ZERO_PARAM - 1)) / CABAC_ZERO_PARAM;
            if (target_num_bytes_in_units > num_bytes_in_units)
            {
                unsigned int num_add_bytes_needed = target_num_bytes_in_units - num_bytes_in_units;
                unsigned int num_add_cabac_zero_words = (num_add_bytes_needed + 2) / 3;
                unsigned int num_add_cabac_zero_bytes = num_add_cabac_zero_words * 3;
                for (unsigned int i = 0; i < num_add_cabac_zero_words; i++)
                {
                    xeve_bsw_write(bs, 0, 16); //2 bytes (=00 00))
                }
            }
        }

        /* Bit-stream re-writing (END) */
#if TRACE_HLS
        s32 tmp_fp_point2 = ftell(fp_trace);
        fseek(fp_trace, tmp_fp_point, SEEK_SET);
#endif
        ret = ctx->fn_eco_sh(&bs_sh, &ctx->sps, &ctx->pps, sh, ctx->nalu.nal_unit_type_plus1 - 1);
        xeve_assert_rv(ret == XEVE_OK, ret);
#if TRACE_HLS
        fseek(fp_trace, tmp_fp_point2, SEEK_SET);
#endif
        xeve_bsw_deinit(bs);
        *size_field = (int)(bs->cur - cur_tmp) - 4;
        curr_temp = bs->cur;

    }  // End of slice loop
    return XEVE_OK;
}


void xeve_itdp(XEVE_CTX * ctx, XEVE_CORE * core, s16 coef[N_C][MAX_CU_DIM], int nnz_sub[N_C][MAX_SUB_TB_NUM])
{
    xeve_sub_block_itdq(coef, core->log2_cuw, core->log2_cuh, core->qp_y, core->qp_u, core->qp_v, core->nnz, nnz_sub, ctx->sps.bit_depth_luma_minus8 + 8);
}

void xeve_recon(XEVE_CTX * ctx, XEVE_CORE * core, s16 *coef, pel *pred, int is_coef, int cuw, int cuh, int s_rec, pel *rec, int bit_depth)
{
    xeve_recon_blk(coef, pred, is_coef, cuw, cuh, s_rec, rec, bit_depth);
}

int xeve_enc(XEVE_CTX * ctx, XEVE_BITB * bitb, XEVE_STAT * stat)
{
    int            ret;
    int            gop_size, pic_cnt;

    pic_cnt = ctx->pic_icnt - ctx->frm_rnum;
    gop_size = ctx->param.gop_size;

    ctx->force_slice = ((ctx->pic_ticnt % gop_size >= ctx->pic_ticnt - pic_cnt + 1) && FORCE_OUT(ctx)) ? 1 : 0;

    xeve_assert_rv(bitb->addr && bitb->bsize > 0, XEVE_ERR_INVALID_ARGUMENT);

    /* initialize variables for a picture encoding */
    ret = ctx->fn_enc_pic_prepare(ctx, bitb, stat);
    xeve_assert_rv(ret == XEVE_OK, ret);

    /* encode parameter set */
    ret = ctx->fn_enc_header(ctx);
    xeve_assert_rv(ret == XEVE_OK, ret);

    /* encode one picture */
    ret = ctx->fn_enc_pic(ctx, bitb, stat);
    xeve_assert_rv(ret == XEVE_OK, ret);

    /* finishing of encoding a picture */
    ctx->fn_enc_pic_finish(ctx, bitb, stat);
    xeve_assert_rv(ret == XEVE_OK, ret);

    return XEVE_OK;
}

int xeve_push_frm(XEVE_CTX * ctx, XEVE_IMGB * img)
{
    XEVE_PIC  * pic;
    XEVE_PICO * pico;
    XEVE_IMGB * imgb;
    int ret;

    ret = ctx->fn_get_inbuf(ctx, &imgb);
    xeve_assert_rv(XEVE_OK == ret, ret);

    imgb->cs = XEVE_CS_SET(XEVE_CF_YCBCR420, ctx->cdsc.codec_bit_depth, 0);
    xeve_imgb_cpy(imgb, img);

    ctx->pic_icnt++;
    ctx->pico_idx = ctx->pic_icnt % ctx->pico_max_cnt;

    pico = ctx->pico_buf[ctx->pico_idx];
    pico->pic_icnt = ctx->pic_icnt;
    pico->is_used = 1;
    pic = &pico->pic;
    ctx->pico = pico;

    PIC_ORIG(ctx) = pic;

    /* set pushed image to current input (original) image */
    xeve_mset(pic, 0, sizeof(XEVE_PIC));

    pic->buf_y = imgb->baddr[0];
    pic->buf_u = imgb->baddr[1];
    pic->buf_v = imgb->baddr[2];

    pic->y = imgb->a[0];
    pic->u = imgb->a[1];
    pic->v = imgb->a[2];

    pic->w_l = imgb->w[0];
    pic->h_l = imgb->h[0];
    pic->w_c = imgb->w[1];
    pic->h_c = imgb->h[1];

    pic->s_l = STRIDE_IMGB2PIC(imgb->s[0]);
    pic->s_c = STRIDE_IMGB2PIC(imgb->s[1]);

    pic->imgb = imgb;
    XEVE_PIC *spic = NULL;

    /* generate sub-picture for RC and Forecast */
    spic = pico->spic;
    xeve_rc_gen_subpic(pic->y, spic->y, spic->w_l, spic->h_l, pic->s_l, spic->s_l, 10);
    
    xeve_mset(pico->map_pdir, 0, sizeof(u8) * ctx->f_lcu);
    xeve_mset(pico->map_mv, 0, sizeof(s16) * ctx->f_lcu * REFP_NUM * MV_D);
    xeve_mset(pico->map_mv_pga, 0, sizeof(s16) * ctx->f_lcu * REFP_NUM * MV_D);
    xeve_mset(pico->map_lcu_cost_uni, 0, sizeof(s32) * ctx->f_lcu * 4);
    xeve_mset(pico->map_lcu_cost_bi, 0, sizeof(s32) * ctx->f_lcu);
    if (ctx->param.use_dqp)
    {
        xeve_mset(pico->map_qp_offset, 0, sizeof(s32) * ctx->f_lcu);
        xeve_mset(pico->transfer_cost, 0, sizeof(u16) * ctx->f_lcu);
    }
    /*********************************************************************/
    xeve_picbuf_expand(spic, spic->pad_l, spic->pad_c);

    return XEVE_OK;
}

void xeve_platform_init_func()
{
#if X86_SSE
    int support_cpu, support_sse, support_avx;

    support_cpu = xeve_check_cpu_info();
    support_sse = (support_cpu >> 1) & 1;
    support_avx = support_cpu & 1;

    if (support_avx)
    {
        xeve_func_sad             = xeve_tbl_sad_16b_avx;
        xeve_func_ssd             = xeve_tbl_ssd_16b_sse;
        xeve_func_diff            = xeve_tbl_diff_16b_sse;
        xeve_func_satd            = xeve_tbl_satd_16b_sse;
        xeve_func_mc_l            = xeve_tbl_mc_l_avx;
        xeve_func_mc_c            = xeve_tbl_mc_c_avx;
        xeve_func_average_no_clip = &xeve_average_16b_no_clip_sse;
    }
    else if (support_sse)
    {
        xeve_func_sad             = xeve_tbl_sad_16b_sse;
        xeve_func_ssd             = xeve_tbl_ssd_16b_sse;
        xeve_func_diff            = xeve_tbl_diff_16b_sse;
        xeve_func_satd            = xeve_tbl_satd_16b_sse;
        xeve_func_mc_l            = xeve_tbl_mc_l_sse;
        xeve_func_mc_c            = xeve_tbl_mc_c_sse;
        xeve_func_average_no_clip = &xeve_average_16b_no_clip_sse;
    }
    else
#endif
    {
        xeve_func_sad             = xeve_tbl_sad_16b;
        xeve_func_ssd             = xeve_tbl_ssd_16b;
        xeve_func_diff            = xeve_tbl_diff_16b;
        xeve_func_satd            = xeve_tbl_satd_16b;
        xeve_func_mc_l            = xeve_tbl_mc_l;
        xeve_func_mc_c            = xeve_tbl_mc_c;
        xeve_func_average_no_clip = &xeve_average_16b_no_clip;
    }
}

int xeve_platform_init(XEVE_CTX * ctx)
{
    int ret = XEVE_ERR_UNKNOWN;

    /* create mode decision */
    ret = xeve_mode_create(ctx, 0);
    xeve_assert_rv(XEVE_OK == ret, ret);

    /* create intra prediction analyzer */
    {
        ret = xeve_pintra_create(ctx, 0);
    }
    xeve_assert_rv(XEVE_OK == ret, ret);

    /* create inter prediction analyzer */
    {
        ret = xeve_pinter_create(ctx, 0);
    }
    xeve_assert_rv(XEVE_OK == ret, ret);


    ctx->pic_dbk = NULL;
    ctx->fn_ready = xeve_ready;
    ctx->fn_flush = xeve_flush;
    ctx->fn_enc = xeve_enc;
    ctx->fn_enc_header = xeve_header;

        ctx->fn_enc_pic = xeve_pic;
  

    ctx->fn_enc_pic_prepare = xeve_pic_prepare;
    ctx->fn_enc_pic_finish = xeve_pic_finish;
    ctx->fn_push = xeve_push_frm;
    ctx->fn_deblock = xeve_deblock;
    ctx->fn_picbuf_expand = xeve_pic_expand;
    ctx->fn_get_inbuf = xeve_picbuf_get_inbuf;
    ctx->fn_loop_filter = xeve_loop_filter;
    ctx->fn_eco_sps = xeve_eco_sps;
    ctx->fn_eco_pps = xeve_eco_pps;
    ctx->fn_eco_sh = xeve_eco_sh;
    ctx->fn_eco_split_mode = xeve_eco_split_mode;
    ctx->fn_eco_sbac_reset = xeve_sbac_reset;
    ctx->fn_rdo_intra_ext = NULL;
    ctx->fn_rdo_intra_ext_c = NULL;
    ctx->fn_eco_coef = xeve_eco_coef;
    ctx->fn_eco_pic_signature = xeve_eco_pic_signature;
    ctx->fn_tq = xeve_sub_block_tq;
    ctx->fn_rdoq_set_ctx_cc = xeve_rdoq_set_ctx_cc;
    ctx->fn_itdp = xeve_itdp;
    ctx->fn_recon = xeve_recon;
    ctx->pf = NULL;
    ctx->fn_deblock_unit = xeve_deblock_unit;

    xeve_platform_init_func();

    return XEVE_OK;
}

void xeve_platform_deinit(XEVE_CTX * ctx)
{
    xeve_assert(ctx->pf == NULL);

    ctx->fn_ready = NULL;
    ctx->fn_flush = NULL;
    ctx->fn_enc = NULL;
    ctx->fn_enc_pic = NULL;
    ctx->fn_enc_pic_prepare = NULL;
    ctx->fn_enc_pic_finish = NULL;
    ctx->fn_push = NULL;
    ctx->fn_deblock = NULL;
    ctx->fn_picbuf_expand = NULL;
    ctx->fn_get_inbuf = NULL;
}

int xeve_create_bs_buf(XEVE_CTX  * ctx)
{
    u8 * bs_buf, *bs_buf_temp;
    if (ctx->cdsc.parallel_task_cnt > 1)
    {
        bs_buf = (u8 *)xeve_malloc(sizeof(u8 *) * (ctx->cdsc.parallel_task_cnt - 1) * ctx->cdsc.bitstream_buf_size);
        for (int task_id = 1; task_id < ctx->cdsc.parallel_task_cnt; task_id++)
        {
            bs_buf_temp = bs_buf + ((task_id - 1) * ctx->cdsc.bitstream_buf_size);
            xeve_bsw_init(&ctx->bs[task_id], bs_buf_temp, ctx->cdsc.bitstream_buf_size, NULL);
            ctx->bs[task_id].pdata[1] = &ctx->sbac_enc[task_id];
        }
    }
    return XEVE_OK;
}

int xeve_delete_bs_buf(XEVE_CTX  * ctx)
{
    if (ctx->cdsc.parallel_task_cnt > 1)
    {
        u8 * bs_buf_temp = ctx->bs[1].beg;
        if (bs_buf_temp != NULL)
        {
            xeve_mfree(bs_buf_temp);
        }
        bs_buf_temp = NULL;
    }
    return XEVE_OK;
}

XEVE xeve_create(XEVE_CDSC * cdsc, int * err)
{
    XEVE_CTX  * ctx;
    int          ret;

#if ENC_DEC_TRACE
#if TRACE_DBF
    fp_trace = fopen("enc_trace_dbf.txt", "w+");
#else
    fp_trace = fopen("enc_trace.txt", "w+");
#endif
#if TRACE_HLS
    XEVE_TRACE_SET(1);
#endif
#endif

    ctx = NULL;

    /* memory allocation for ctx and core structure */
    ctx = (XEVE_CTX*)ctx_alloc();

    xeve_assert_gv(ctx != NULL, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    xeve_mcpy(&ctx->cdsc, cdsc, sizeof(XEVE_CDSC));

    /* set default value for encoding parameter */
    ret = set_init_param(cdsc, &ctx->param);
    xeve_assert_g(ret == XEVE_OK, ERR);

    ret = xeve_platform_init(ctx);
    xeve_assert_g(ret == XEVE_OK, ERR);

    ret = xeve_scan_tbl_init();
    xeve_assert_g(ret == XEVE_OK, ERR);

    ret = xeve_create_bs_buf(ctx);
    xeve_assert_g(ret == XEVE_OK, ERR);

    xeve_init_err_scale(cdsc->codec_bit_depth);
    xeve_set_chroma_qp_tbl_loc(cdsc->codec_bit_depth);

    if(ctx->fn_ready != NULL)
    {
        ret = ctx->fn_ready(ctx);
        xeve_assert_g(ret == XEVE_OK, ERR);
    }

    /* set default value for ctx */
    ctx->magic = XEVE_MAGIC_CODE;
    ctx->id = (XEVE)ctx;
    ctx->sh.aps_signaled = -1;

    return (ctx->id);
ERR:
    if(ctx)
    {
        xeve_platform_deinit(ctx);
        xeve_delete_bs_buf(ctx);
        ctx_free(ctx);
    }
    if(err) *err = ret;
    return NULL;
}

void xeve_delete(XEVE id)
{
    XEVE_CTX * ctx;

    XEVE_ID_TO_CTX_R(id, ctx);

#if ENC_DEC_TRACE
    fclose(fp_trace);
#endif

    if(ctx->fn_flush != NULL)
    {
        ctx->fn_flush(ctx);
    }

    xeve_platform_deinit(ctx);
    xeve_delete_bs_buf(ctx);
    ctx_free(ctx);

    xeve_scan_tbl_delete();
}

int xeve_encode_sps(XEVE_CTX * ctx)
{
    XEVE_BSW * bs = &ctx->bs[0];
    XEVE_SPS * sps = &ctx->sps;
    XEVE_NALU  nalu;

    int* size_field = (int*)(*(&bs->cur));
    u8* cur_tmp = bs->cur;

    /* nalu header */
    set_nalu(&nalu, XEVE_SPS_NUT, 0);
    xeve_eco_nalu(bs, &nalu);

    /* sequence parameter set*/
    set_sps(ctx, &ctx->sps);
    xeve_assert_rv(ctx->fn_eco_sps(bs, sps) == XEVE_OK, XEVE_ERR_INVALID_ARGUMENT);

    /* de-init BSW */
    xeve_bsw_deinit(bs);

    /* write the bitstream size */
    *size_field = (int)(bs->cur - cur_tmp) - 4;

    return XEVE_OK;
}

int xeve_encode_pps(XEVE_CTX * ctx)
{
    XEVE_BSW * bs = &ctx->bs[0];
    XEVE_SPS * sps = &ctx->sps;
    XEVE_PPS * pps = &ctx->pps;
    XEVE_NALU  nalu;
    int      * size_field = (int*)(*(&bs->cur));
    u8       * cur_tmp = bs->cur;

    /* nalu header */
    set_nalu(&nalu, XEVE_PPS_NUT, ctx->nalu.nuh_temporal_id);
    xeve_eco_nalu(bs, &nalu);

    /* sequence parameter set*/
    set_pps(ctx, &ctx->pps);
    xeve_assert_rv(ctx->fn_eco_pps(bs, sps, pps) == XEVE_OK, XEVE_ERR_INVALID_ARGUMENT);

    /* de-init BSW */
    xeve_bsw_deinit(bs);

    /* write the bitstream size */
    *size_field = (int)(bs->cur - cur_tmp) - 4;

    return XEVE_OK;
}

static int check_frame_delay(XEVE_CTX * ctx)
{
    if(ctx->pic_icnt < ctx->frm_rnum)
    {
        return XEVE_OK_OUT_NOT_AVAILABLE;
    }
    return XEVE_OK;
}

static int check_more_frames(XEVE_CTX * ctx)
{
    XEVE_PICO  * pico;
    int           i;

    if(FORCE_OUT(ctx))
    {
        /* pseudo xeve_push() for bumping process ****************/
        ctx->pic_icnt++;
        /**********************************************************/

        for(i=0; i<ctx->pico_max_cnt; i++)
        {
            pico = ctx->pico_buf[i];
            if(pico != NULL)
            {
                if(pico->is_used == 1)
                {
                    return XEVE_OK;
                }
            }
        }

        return XEVE_OK_NO_MORE_FRM;
    }

    return XEVE_OK;
}

int xeve_encode(XEVE id, XEVE_BITB * bitb, XEVE_STAT * stat)
{
    XEVE_CTX * ctx;

    XEVE_ID_TO_CTX_RV(id, ctx, XEVE_ERR_INVALID_ARGUMENT);
    xeve_assert_rv(ctx->fn_enc, XEVE_ERR_UNEXPECTED);

    /* bumping - check whether input pictures are remaining or not in pico_buf[] */
    if(XEVE_OK_NO_MORE_FRM == check_more_frames(ctx))
    {
        return XEVE_OK_NO_MORE_FRM;
    }

    /* store input picture and return if needed */
    if(XEVE_OK_OUT_NOT_AVAILABLE == check_frame_delay(ctx))
    {
        return XEVE_OK_OUT_NOT_AVAILABLE;
    }

    /* update BSB */
    bitb->err = 0;

    return ctx->fn_enc(ctx, bitb, stat);
}

int xeve_push(XEVE id, XEVE_IMGB * img)
{
    XEVE_CTX * ctx;

    XEVE_ID_TO_CTX_RV(id, ctx, XEVE_ERR_INVALID_ARGUMENT);
    xeve_assert_rv(ctx->fn_push, XEVE_ERR_UNEXPECTED);

    return ctx->fn_push(ctx, img);
}

int xeve_config(XEVE id, int cfg, void * buf, int * size)
{
    XEVE_CTX       * ctx;
    int              t0;
    XEVE_IMGB      * imgb;

    XEVE_ID_TO_CTX_RV(id, ctx, XEVE_ERR_INVALID_ARGUMENT);

    switch(cfg)
    {
        /* set config **********************************************************/
        case XEVE_CFG_SET_FORCE_OUT:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.force_output = (t0) ? 1 : 0;
            /* store total input picture count at this time */
            ctx->pic_ticnt = ctx->pic_icnt;
            break;

        case XEVE_CFG_SET_FINTRA:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.f_ifrm = t0;
            break;
        case XEVE_CFG_SET_QP:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            xeve_assert_rv(t0 >= MIN_QUANT && t0 <= MAX_QUANT, \
                           XEVE_ERR_INVALID_ARGUMENT);
            ctx->param.qp = t0;
            break;
        case XEVE_CFG_SET_FPS:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            xeve_assert_rv(t0 > 0, XEVE_ERR_INVALID_ARGUMENT);
            ctx->param.fps = t0;
            break;
        case XEVE_CFG_SET_I_PERIOD:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            xeve_assert_rv(t0 >= 0, XEVE_ERR_INVALID_ARGUMENT);
            ctx->param.i_period = t0;
            break;
        case XEVE_CFG_SET_QP_MIN:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            xeve_assert_rv(t0 >= MIN_QUANT, XEVE_ERR_INVALID_ARGUMENT);
            ctx->param.qp_min = t0;
            break;
        case XEVE_CFG_SET_QP_MAX:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            xeve_assert_rv(t0 <= MAX_QUANT, XEVE_ERR_INVALID_ARGUMENT);
            ctx->param.qp_max = t0;
            break;
        case XEVE_CFG_SET_USE_DEBLOCK:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.use_deblock = t0;
            break;
        case XEVE_CFG_SET_DEBLOCK_A_OFFSET:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.deblock_alpha_offset = t0;
            break;
        case XEVE_CFG_SET_DEBLOCK_B_OFFSET:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.deblock_beta_offset = t0;
            break;
        case XEVE_CFG_SET_USE_PIC_SIGNATURE:
            ctx->param.use_pic_sign = (*((int *)buf)) ? 1 : 0;
            break;

            /* get config *******************************************************/
        case XEVE_CFG_GET_QP:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.qp;
            break;
        case XEVE_CFG_GET_WIDTH:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.w;
            break;
        case XEVE_CFG_GET_HEIGHT:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.h;
            break;
        case XEVE_CFG_GET_FPS:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.fps;
            break;
        case XEVE_CFG_GET_I_PERIOD:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.i_period;
            break;
        case XEVE_CFG_GET_RECON:
            xeve_assert_rv(*size == sizeof(XEVE_IMGB**), XEVE_ERR_INVALID_ARGUMENT);
            imgb = PIC_CURR(ctx)->imgb;

            if (ctx->sps.picture_cropping_flag)
            {
                for (int i = 0; i < N_C; i++)
                {
                    int cs_offset = i == Y_C ? 2 : 1;
                    imgb->x[i] = ctx->sps.picture_crop_left_offset * cs_offset;
                    imgb->y[i] = ctx->sps.picture_crop_top_offset * cs_offset;
                    imgb->h[i] = imgb->ah[i] - (ctx->sps.picture_crop_top_offset + ctx->sps.picture_crop_bottom_offset) * cs_offset;
                    imgb->w[i] = imgb->aw[i] - (ctx->sps.picture_crop_left_offset + ctx->sps.picture_crop_left_offset) * cs_offset;
                }
            }

            *((XEVE_IMGB **)buf) = imgb;
            imgb->addref(imgb);
            break;
        case XEVE_CFG_GET_USE_DEBLOCK:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.use_deblock;
            break;
        case XEVE_CFG_GET_CLOSED_GOP:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.use_closed_gop;
            break;
        case XEVE_CFG_GET_HIERARCHICAL_GOP:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.use_hgop;
            break;
        case XEVE_CFG_GET_DEBLOCK_A_OFFSET:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.deblock_alpha_offset;
            break;
        case XEVE_CFG_GET_DEBLOCK_B_OFFSET:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.deblock_beta_offset;
            break;
        case XEVE_CFG_GET_SUPPORT_PROF:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = PROFILE_BASELINE;
            break;
        default:
            xeve_trace("unknown config value (%d)\n", cfg);
            xeve_assert_rv(0, XEVE_ERR_UNSUPPORTED);
    }

    return XEVE_OK;
}

void xeve_malloc_1d(void** dst, int size)
{
    if(*dst == NULL)
    {
        *dst = xeve_malloc_fast(size);
        xeve_mset(*dst, 0, size);
    }
}

void xeve_malloc_2d(s8*** dst, int size_1d, int size_2d, int type_size)
{
    int i;

    if(*dst == NULL)
    {
        *dst = xeve_malloc_fast(size_1d * sizeof(s8*));
        xeve_mset(*dst, 0, size_1d * sizeof(s8*));


        (*dst)[0] = xeve_malloc_fast(size_1d * size_2d * type_size);
        xeve_mset((*dst)[0], 0, size_1d * size_2d * type_size);

        for(i = 1; i < size_1d; i++)
        {
            (*dst)[i] = (*dst)[i - 1] + size_2d * type_size;
        }
    }
}

int xeve_create_cu_data(XEVE_CU_DATA *cu_data, int log2_cuw, int log2_cuh)
{
    int i, j;
    int cuw_scu, cuh_scu;
    int size_8b, size_16b, size_32b, cu_cnt, pixel_cnt;

    cuw_scu = 1 << log2_cuw;
    cuh_scu = 1 << log2_cuh;

    size_8b = cuw_scu * cuh_scu * sizeof(s8);
    size_16b = cuw_scu * cuh_scu * sizeof(s16);
    size_32b = cuw_scu * cuh_scu * sizeof(s32);
    cu_cnt = cuw_scu * cuh_scu;
    pixel_cnt = cu_cnt << 4;

    xeve_malloc_1d((void**)&cu_data->qp_y, size_8b);
    xeve_malloc_1d((void**)&cu_data->qp_u, size_8b);
    xeve_malloc_1d((void**)&cu_data->qp_v, size_8b);
    xeve_malloc_1d((void**)&cu_data->pred_mode, size_8b);
    xeve_malloc_1d((void**)&cu_data->pred_mode_chroma, size_8b);
    xeve_malloc_2d((s8***)&cu_data->mpm, 2, cu_cnt, sizeof(u8));
    xeve_malloc_2d((s8***)&cu_data->ipm, 2, cu_cnt, sizeof(u8));
    xeve_malloc_2d((s8***)&cu_data->mpm_ext, 8, cu_cnt, sizeof(u8));
    xeve_malloc_1d((void**)&cu_data->skip_flag, size_8b);
    xeve_malloc_1d((void**)&cu_data->ibc_flag, size_8b);
    xeve_malloc_1d((void**)&cu_data->dmvr_flag, size_8b);
    xeve_malloc_2d((s8***)&cu_data->refi, cu_cnt, REFP_NUM, sizeof(u8));
    xeve_malloc_2d((s8***)&cu_data->mvp_idx, cu_cnt, REFP_NUM, sizeof(u8));
    xeve_malloc_1d((void**)&cu_data->mvr_idx, size_8b);
    xeve_malloc_1d((void**)&cu_data->bi_idx, size_8b);
    xeve_malloc_1d((void**)&cu_data->mmvd_idx, size_16b);
    xeve_malloc_1d((void**)&cu_data->mmvd_flag, size_8b);
    xeve_malloc_1d((void**)& cu_data->ats_intra_cu, size_8b);
    xeve_malloc_1d((void**)& cu_data->ats_mode_h, size_8b);
    xeve_malloc_1d((void**)& cu_data->ats_mode_v, size_8b);
    xeve_malloc_1d((void**)&cu_data->ats_inter_info, size_8b);

    for(i = 0; i < N_C; i++)
    {
        xeve_malloc_1d((void**)&cu_data->nnz[i], size_32b);
    }
    for (i = 0; i < N_C; i++)
    {
        for (j = 0; j < 4; j++)
        {
            xeve_malloc_1d((void**)&cu_data->nnz_sub[i][j], size_32b);
        }
    }
    xeve_malloc_1d((void**)&cu_data->map_scu, size_32b);
    xeve_malloc_1d((void**)&cu_data->affine_flag, size_8b);
    xeve_malloc_1d((void**)&cu_data->map_affine, size_32b);
    xeve_malloc_1d((void**)&cu_data->map_cu_mode, size_32b);
    xeve_malloc_1d((void**)&cu_data->depth, size_8b);

    for(i = 0; i < N_C; i++)
    {
        xeve_malloc_1d((void**)&cu_data->coef[i], (pixel_cnt >> (!!(i)* 2)) * sizeof(s16));
        xeve_malloc_1d((void**)&cu_data->reco[i], (pixel_cnt >> (!!(i)* 2)) * sizeof(pel));
    }

    return XEVE_OK;
}

void xeve_free_1d(void* dst)
{
    if(dst != NULL)
    {
        xeve_mfree_fast(dst);
    }
}

void xeve_free_2d(void** dst)
{
    if (dst)
    {
        if (dst[0])
        {
            xeve_mfree_fast(dst[0]);
        }
        xeve_mfree_fast(dst);
    }
}

int xeve_delete_cu_data(XEVE_CU_DATA *cu_data, int log2_cuw, int log2_cuh)
{
    int i, j;

    xeve_free_1d((void*)cu_data->qp_y);
    xeve_free_1d((void*)cu_data->qp_u);
    xeve_free_1d((void*)cu_data->qp_v);
    xeve_free_1d((void*)cu_data->pred_mode);
    xeve_free_1d((void*)cu_data->pred_mode_chroma);
    xeve_free_2d((void**)cu_data->mpm);
    xeve_free_2d((void**)cu_data->ipm);
    xeve_free_2d((void**)cu_data->mpm_ext);
    xeve_free_1d((void*)cu_data->skip_flag);
    xeve_free_1d((void*)cu_data->ibc_flag);
    xeve_free_1d((void*)cu_data->dmvr_flag);
    xeve_free_2d((void**)cu_data->refi);
    xeve_free_2d((void**)cu_data->mvp_idx);
    xeve_free_1d(cu_data->mvr_idx);
    xeve_free_1d(cu_data->bi_idx);
    xeve_free_1d((void*)cu_data->mmvd_idx);
    xeve_free_1d((void*)cu_data->mmvd_flag);

    for (i = 0; i < N_C; i++)
    {
        xeve_free_1d((void*)cu_data->nnz[i]);
    }
    for (i = 0; i < N_C; i++)
    {
        for (j = 0; j < 4; j++)
        {
            xeve_free_1d((void*)cu_data->nnz_sub[i][j]);
        }
    }
    xeve_free_1d((void*)cu_data->map_scu);
    xeve_free_1d((void*)cu_data->affine_flag);
    xeve_free_1d((void*)cu_data->map_affine);
    xeve_free_1d((void*)cu_data->ats_intra_cu);
    xeve_free_1d((void*)cu_data->ats_mode_h);
    xeve_free_1d((void*)cu_data->ats_mode_v);
    xeve_free_1d((void*)cu_data->ats_inter_info);
    xeve_free_1d((void*)cu_data->map_cu_mode);
    xeve_free_1d((void*)cu_data->depth);

    for (i = 0; i < N_C; i++)
    {
        xeve_free_1d((void*)cu_data->coef[i]);
        xeve_free_1d((void*)cu_data->reco[i]);
    }

    return XEVE_OK;
}

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
#include <limits.h>
#include <math.h>

int xeve_eco_nal_unit_len(void * buf, int size)
{
    int i;
    u8 * p = buf;
    for(i=0; i<4; i++) {
        p[i] = (size >> (24 - (i*8))) & 0xFF;
    }
    return 0;
}

int xeve_eco_nalu(XEVE_BSW * bs, XEVE_NALU * nalu)
{
#if TRACE_HLS
    xeve_bsw_write_trace(bs, nalu->nal_unit_size, 0, 32);
#else
    xeve_bsw_write(bs, nalu->nal_unit_size, 32);
#endif
    xeve_bsw_write(bs, nalu->forbidden_zero_bit, 1);
    xeve_bsw_write(bs, nalu->nal_unit_type_plus1, 6);
    xeve_bsw_write(bs, nalu->nuh_temporal_id, 3);
    xeve_bsw_write(bs, nalu->nuh_reserved_zero_5bits, 5);
    xeve_bsw_write(bs, nalu->nuh_extension_flag, 1);
    return XEVE_OK;
}

int xeve_eco_hrd_parameters(XEVE_BSW * bs, XEVE_HRD * hrd) {
    xeve_bsw_write_ue(bs, hrd->cpb_cnt_minus1);
    xeve_bsw_write(bs, hrd->bit_rate_scale, 4);
    xeve_bsw_write(bs, hrd->cpb_size_scale, 4);
    for (int SchedSelIdx = 0; SchedSelIdx <= hrd->cpb_cnt_minus1; SchedSelIdx++) {
        xeve_bsw_write_ue(bs, hrd->bit_rate_value_minus1[SchedSelIdx]);
        xeve_bsw_write_ue(bs, hrd->cpb_size_value_minus1[SchedSelIdx]);
        xeve_bsw_write1(bs, hrd->cbr_flag[SchedSelIdx]);
    }
    xeve_bsw_write(bs, hrd->initial_cpb_removal_delay_length_minus1, 5);
    xeve_bsw_write(bs, hrd->cpb_removal_delay_length_minus1, 5);
    xeve_bsw_write(bs, hrd->dpb_output_delay_length_minus1, 5);
    xeve_bsw_write(bs, hrd->time_offset_length, 5);

    return XEVE_OK;
}

int xeve_eco_vui(XEVE_BSW * bs, XEVE_VUI * vui)
{
    xeve_bsw_write1(bs, vui->aspect_ratio_info_present_flag);
    if (vui->aspect_ratio_info_present_flag) {
        xeve_bsw_write(bs, vui->aspect_ratio_idc, 8);
        if (vui->aspect_ratio_idc == EXTENDED_SAR) {
            xeve_bsw_write(bs, vui->sar_width, 16);
            xeve_bsw_write(bs, vui->sar_height, 16);
        }
    }
    xeve_bsw_write1(bs, vui->overscan_info_present_flag);
    if (vui->overscan_info_present_flag)
        xeve_bsw_write1(bs, vui->overscan_appropriate_flag);
    xeve_bsw_write1(bs, vui->video_signal_type_present_flag);
    if (vui->video_signal_type_present_flag) {
        xeve_bsw_write(bs, vui->video_format, 3);
        xeve_bsw_write1(bs, vui->video_full_range_flag);
        xeve_bsw_write1(bs, vui->colour_description_present_flag);
        if (vui->colour_description_present_flag) {
            xeve_bsw_write(bs, vui->colour_primaries, 8);
            xeve_bsw_write(bs, vui->transfer_characteristics, 8);
            xeve_bsw_write(bs, vui->matrix_coefficients, 8);
        }
    }
    xeve_bsw_write1(bs, vui->chroma_loc_info_present_flag);
    if (vui->chroma_loc_info_present_flag) {
        xeve_bsw_write_ue(bs, vui->chroma_sample_loc_type_top_field);
        xeve_bsw_write_ue(bs, vui->chroma_sample_loc_type_bottom_field);
    }
    xeve_bsw_write1(bs, vui->neutral_chroma_indication_flag);

    xeve_bsw_write1(bs, vui->field_seq_flag);

    xeve_bsw_write1(bs, vui->timing_info_present_flag);
    if (vui->timing_info_present_flag) {
        xeve_bsw_write(bs, vui->num_units_in_tick, 32);
        xeve_bsw_write(bs, vui->time_scale, 32);
        xeve_bsw_write1(bs, vui->fixed_pic_rate_flag);
    }
    xeve_bsw_write1(bs, vui->nal_hrd_parameters_present_flag);
    if (vui->nal_hrd_parameters_present_flag)
        xeve_eco_hrd_parameters(bs, &(vui->hrd_parameters));
    xeve_bsw_write1(bs, vui->vcl_hrd_parameters_present_flag);
    if (vui->vcl_hrd_parameters_present_flag)
        xeve_eco_hrd_parameters(bs, &(vui->hrd_parameters));
    if (vui->nal_hrd_parameters_present_flag || vui->vcl_hrd_parameters_present_flag)
        xeve_bsw_write1(bs, vui->low_delay_hrd_flag);
    xeve_bsw_write1(bs, vui->pic_struct_present_flag);
    xeve_bsw_write1(bs, vui->bitstream_restriction_flag);
    if (vui->bitstream_restriction_flag) {
        xeve_bsw_write1(bs, vui->motion_vectors_over_pic_boundaries_flag);
        xeve_bsw_write_ue(bs, vui->max_bytes_per_pic_denom);
        xeve_bsw_write_ue(bs, vui->max_bits_per_mb_denom);
        xeve_bsw_write_ue(bs, vui->log2_max_mv_length_horizontal);
        xeve_bsw_write_ue(bs, vui->log2_max_mv_length_vertical);
        xeve_bsw_write_ue(bs, vui->num_reorder_pics);
        xeve_bsw_write_ue(bs, vui->max_dec_pic_buffering);
    }

    return XEVE_OK;
}

int xeve_eco_sps(XEVE_BSW * bs, XEVE_SPS * sps)
{
#if TRACE_HLS
    XEVE_TRACE_STR("***********************************\n");
    XEVE_TRACE_STR("************ SPS Start ************\n");
#endif
    xeve_bsw_write_ue(bs, sps->sps_seq_parameter_set_id);
    xeve_bsw_write(bs, sps->profile_idc, 8);
    xeve_bsw_write(bs, sps->level_idc, 8);
    xeve_bsw_write(bs, sps->toolset_idc_h, 32);
    xeve_bsw_write(bs, sps->toolset_idc_l, 32);
    xeve_bsw_write_ue(bs, sps->chroma_format_idc);
    xeve_bsw_write_ue(bs, sps->pic_width_in_luma_samples);
    xeve_bsw_write_ue(bs, sps->pic_height_in_luma_samples);
    xeve_bsw_write_ue(bs, sps->bit_depth_luma_minus8);
    xeve_bsw_write_ue(bs, sps->bit_depth_chroma_minus8);
    xeve_bsw_write1(bs, sps->sps_btt_flag);
    xeve_bsw_write1(bs, sps->sps_suco_flag);
    xeve_bsw_write1(bs, sps->tool_admvp);
    xeve_bsw_write1(bs, sps->tool_eipd);
    xeve_bsw_write1(bs, sps->tool_cm_init);
    xeve_bsw_write1(bs, sps->tool_iqt);
    xeve_bsw_write1(bs, sps->tool_addb);
    xeve_bsw_write1(bs, sps->tool_alf);
    xeve_bsw_write1(bs, sps->tool_htdf);
    xeve_bsw_write1(bs, sps->tool_rpl);
    xeve_bsw_write1(bs, sps->tool_pocs);
    xeve_bsw_write1(bs, sps->dquant_flag);
    xeve_bsw_write1(bs, sps->tool_dra);

    xeve_bsw_write_ue(bs, sps->log2_sub_gop_length);
    if (sps->log2_sub_gop_length == 0)
    {
        xeve_bsw_write_ue(bs, sps->log2_ref_pic_gap_length);
    }

    xeve_bsw_write_ue(bs, sps->max_num_ref_pics);
    xeve_bsw_write1(bs, sps->picture_cropping_flag);
    if (sps->picture_cropping_flag)
    {
        xeve_bsw_write_ue(bs, sps->picture_crop_left_offset);
        xeve_bsw_write_ue(bs, sps->picture_crop_right_offset);
        xeve_bsw_write_ue(bs, sps->picture_crop_top_offset);
        xeve_bsw_write_ue(bs, sps->picture_crop_bottom_offset);
    }

    if (sps->chroma_format_idc != 0)
    {
        xeve_bsw_write1(bs, sps->chroma_qp_table_struct.chroma_qp_table_present_flag);
        if (sps->chroma_qp_table_struct.chroma_qp_table_present_flag)
        {
            xeve_bsw_write1(bs, sps->chroma_qp_table_struct.same_qp_table_for_chroma);
            xeve_bsw_write1(bs, sps->chroma_qp_table_struct.global_offset_flag);
            for (int i = 0; i < (sps->chroma_qp_table_struct.same_qp_table_for_chroma ? 1 : 2); i++)
            {
                xeve_bsw_write_ue(bs, (u32)sps->chroma_qp_table_struct.num_points_in_qp_table_minus1[i]);
                for (int j = 0; j <= sps->chroma_qp_table_struct.num_points_in_qp_table_minus1[i]; j++)
                {
                    xeve_bsw_write(bs, sps->chroma_qp_table_struct.delta_qp_in_val_minus1[i][j], 6);
                    xeve_bsw_write_se(bs, (u32)sps->chroma_qp_table_struct.delta_qp_out_val[i][j]);
                }
            }
        }
    }

    xeve_bsw_write1(bs, sps->vui_parameters_present_flag);
    if (sps->vui_parameters_present_flag)
    {
        xeve_eco_vui(bs, &(sps->vui_parameters));
    }

    u32 t0 = 0;
    while(!XEVE_BSW_IS_BYTE_ALIGN(bs))
    {
        xeve_bsw_write1(bs, t0);
    }
#if TRACE_HLS
    XEVE_TRACE_STR("************ SPS End   ************\n");
    XEVE_TRACE_STR("***********************************\n");
#endif
    return XEVE_OK;
}

int xeve_eco_pps(XEVE_BSW * bs, XEVE_SPS * sps, XEVE_PPS * pps)
{
#if TRACE_HLS
    XEVE_TRACE_STR("***********************************\n");
    XEVE_TRACE_STR("************ PPS Start ************\n");
#endif
    xeve_bsw_write_ue(bs, pps->pps_pic_parameter_set_id);
    xeve_bsw_write_ue(bs, pps->pps_seq_parameter_set_id);
    xeve_bsw_write_ue(bs, pps->num_ref_idx_default_active_minus1[0]);
    xeve_bsw_write_ue(bs, pps->num_ref_idx_default_active_minus1[1]);
    xeve_bsw_write_ue(bs, pps->additional_lt_poc_lsb_len);
    xeve_bsw_write1(bs, pps->rpl1_idx_present_flag);
    xeve_bsw_write1(bs, pps->single_tile_in_pic_flag);
    xeve_bsw_write_ue(bs, pps->tile_id_len_minus1);
    xeve_bsw_write1(bs, pps->explicit_tile_id_flag);
    xeve_bsw_write1(bs, pps->pic_dra_enabled_flag);
    xeve_bsw_write1(bs, pps->arbitrary_slice_present_flag);
    xeve_bsw_write1(bs, pps->constrained_intra_pred_flag);
    xeve_bsw_write1(bs, pps->cu_qp_delta_enabled_flag);
    if (pps->cu_qp_delta_enabled_flag)
    {
        xeve_bsw_write_ue(bs, pps->cu_qp_delta_area - 6);
    }
    u32 t0 = 0;
    while (!XEVE_BSW_IS_BYTE_ALIGN(bs))
    {
        xeve_bsw_write1(bs, t0);
    }
#if TRACE_HLS
    XEVE_TRACE_STR("************ PPS End   ************\n");
    XEVE_TRACE_STR("***********************************\n");
#endif
    return XEVE_OK;
}

int xeve_eco_sh(XEVE_BSW * bs, XEVE_SPS * sps, XEVE_PPS * pps, XEVE_SH * sh, int nut)
{
#if TRACE_HLS
    XEVE_TRACE_STR("***********************************\n");
    XEVE_TRACE_STR("************ SH  Start ************\n");
#endif

    xeve_bsw_write_ue(bs, sh->slice_pic_parameter_set_id);
    xeve_bsw_write_ue(bs, sh->slice_type);

    if (nut == XEVE_IDR_NUT)
    {
        xeve_bsw_write1(bs, sh->no_output_of_prior_pics_flag);
    }

    if (sh->slice_type != SLICE_I)
    {
        xeve_bsw_write1(bs, sh->num_ref_idx_active_override_flag);
        if (sh->num_ref_idx_active_override_flag)
        {
            u32 num_ref_idx_active_minus1 = sh->rpl_l0.ref_pic_active_num - 1;
            xeve_bsw_write_ue(bs, num_ref_idx_active_minus1);
            if (sh->slice_type == SLICE_B)
            {
                num_ref_idx_active_minus1 = sh->rpl_l1.ref_pic_active_num - 1;
                xeve_bsw_write_ue(bs, num_ref_idx_active_minus1);
            }
        }
    }

    xeve_bsw_write1(bs, sh->deblocking_filter_on);

    xeve_bsw_write(bs, sh->qp, 6);
    xeve_bsw_write_se(bs, (u32)sh->qp_u_offset);
    xeve_bsw_write_se(bs, (u32)sh->qp_v_offset);

    /* byte align */
    u32 t0 = 0;
    while(!XEVE_BSW_IS_BYTE_ALIGN(bs))
    {
        xeve_bsw_write1(bs, t0);
    }
#if TRACE_HLS
    XEVE_TRACE_STR("************ SH  End   ************\n");
    XEVE_TRACE_STR("***********************************\n");
#endif
    return XEVE_OK;
}

int xeve_eco_pic_signature(XEVE_CTX * ctx, XEVE_BSW * bs, u8 pic_sign[N_C][16])
{
    int ret;
    ret = xeve_picbuf_signature(PIC_CURR(ctx), pic_sign);
    xeve_assert_rv(ret == XEVE_OK, ret);
    return ret;
}

int xeve_eco_signature(XEVE_CTX * ctx, XEVE_BSW * bs)
{
    if (ctx->param.use_pic_sign)
    {
        u8 pic_sign[N_C][16] = { {0} };

        /* get picture signature */
        ctx->fn_eco_pic_signature(ctx, bs, pic_sign);

        u32 payload_type = XEVE_UD_PIC_SIGNATURE;
        u32 payload_size = 16;

        xeve_bsw_write(bs, payload_type, 8);
        xeve_bsw_write(bs, payload_size, 8);

        for (int i = 0; i < ctx->pic[0]->imgb->np; ++i)
        {
            for (int j = 0; j < payload_size; j++)
            {
                xeve_bsw_write(bs, pic_sign[i][j], 8);
            }
        }
    }

    return XEVE_OK;
}

static void write_sei_userdata_unregistered(XEVE_SEI_PAYLOAD * sei_userdata, XEVE_BSW * bs)
{
    const u8 m_uuid_iso_iec_11578[ISO_IEC_11578_LEN] = {
        0x2C, 0xA2, 0xDE, 0x09, 0xB5, 0x17, 0x47, 0xDB,
        0xBB, 0x55, 0xA4, 0xFE, 0x7F, 0xC2, 0xFC, 0x4E
    };

    u32 payload_type = sei_userdata->payload_type;
    for (; payload_type >= 0xff; payload_type -= 0xff)
        xeve_bsw_write(bs, 0xff, 8);
    xeve_bsw_write(bs, payload_type, 8);

    u32 payload_size = (ISO_IEC_11578_LEN + sei_userdata->payload_size) << 3;
    for (; payload_size >= 0xff; payload_size -= 0xff)
        xeve_bsw_write(bs, 0xff, 8);
    xeve_bsw_write(bs, payload_size, 8);

    for (u32 i = 0; i < ISO_IEC_11578_LEN; i++)
    {
        xeve_bsw_write(bs, m_uuid_iso_iec_11578[i], 8);
    }
    for (u32 i = 0; i < sei_userdata->payload_size; i++)
    {
        xeve_bsw_write(bs, sei_userdata->payload[i], 8);
    }
}

int xeve_eco_emitsei(XEVE_CTX * ctx, XEVE_BSW * bs)
{
    xeve_assert_rv(XEVE_BSW_IS_BYTE_ALIGN(bs), XEVE_ERR_UNKNOWN);

    if (ctx->param.sei_cmd_info)
    {
        char sei_embed_msg[4000];
        char *sei_msg_ptr = sei_embed_msg;

        char *sei_xeve_msg = " xeve - MPEG-5 EVC codec - "
            "ESSENTIAL VIDEO CODING https://github.com/mpeg5/xeve - options: ";

        sei_msg_ptr += sprintf(sei_msg_ptr, "%s", sei_xeve_msg);

        xeve_param2string(&ctx->param, sei_msg_ptr, ctx->sps.picture_crop_right_offset, ctx->sps.picture_crop_bottom_offset);

        XEVE_SEI_PAYLOAD sei_userdata_unregistered;
        sei_userdata_unregistered.payload_type = USER_DATA_UNREGISTERED;
        sei_userdata_unregistered.payload_size = (u32)strlen(sei_embed_msg);
        sei_userdata_unregistered.payload = (u8*)sei_embed_msg;
        write_sei_userdata_unregistered(&sei_userdata_unregistered, bs);
    }

    while (!XEVE_BSW_IS_BYTE_ALIGN(bs))
    {
        xeve_bsw_write1(bs, 0);
    }

    return XEVE_OK;
}
int xeve_eco_sei(XEVE_CTX * ctx, XEVE_BSW * bs)
{
    xeve_assert_rv(XEVE_BSW_IS_BYTE_ALIGN(bs), XEVE_ERR_UNKNOWN);

    if (ctx->param.use_pic_sign)
    {
        xeve_eco_signature(ctx, bs);
    }

    while (!XEVE_BSW_IS_BYTE_ALIGN(bs))
    {
        xeve_bsw_write1(bs, 0);
    }

    return XEVE_OK;
}

static void xeve_bsw_write_est(XEVE_SBAC *sbac, u32 byte, int len)
{
    sbac->bitcounter += len;
}

static void sbac_put_byte(u8 writing_byte, XEVE_SBAC *sbac, XEVE_BSW *bs)
{
    if(sbac->is_pending_byte)
    {
        if(sbac->pending_byte == 0)
        {
            sbac->stacked_zero++;
        }
        else
        {
            while(sbac->stacked_zero > 0)
            {
                if(sbac->is_bitcount)
                    xeve_bsw_write_est(sbac, 0x00, 8);
                else
#if TRACE_HLS
                    xeve_bsw_write_trace(bs, 0x00, 0, 8);
#else
                    xeve_bsw_write(bs, 0x00, 8);
#endif
                sbac->stacked_zero--;
            }
            if(sbac->is_bitcount)
                xeve_bsw_write_est(sbac, sbac->pending_byte, 8);
            else
#if TRACE_HLS
                xeve_bsw_write_trace(bs, sbac->pending_byte, 0, 8);
#else
                xeve_bsw_write(bs, sbac->pending_byte, 8);
#endif
        }
    }
    sbac->pending_byte = writing_byte;
    sbac->is_pending_byte = 1;
}

static void sbac_carry_propagate(XEVE_SBAC *sbac, XEVE_BSW *bs)
{
    u32 out_bits = sbac->code >> 17;

    sbac->code &= (1 << 17) - 1;

    if(out_bits < 0xFF)
    {
        while(sbac->stacked_ff != 0)
        {
            sbac_put_byte(0xFF, sbac, bs);
            sbac->stacked_ff--;
        }
        sbac_put_byte(out_bits, sbac, bs);
    }
    else if(out_bits > 0xFF)
    {
        sbac->pending_byte++;
        while(sbac->stacked_ff != 0)
        {
            sbac_put_byte(0x00, sbac, bs);
            sbac->stacked_ff--;
        }
        sbac_put_byte(out_bits & 0xFF, sbac, bs);
    }
    else
    {
        sbac->stacked_ff++;
    }
}

void sbac_encode_bin_ep(u32 bin, XEVE_SBAC *sbac, XEVE_BSW *bs)
{
    sbac->bin_counter++;

    (sbac->range) >>= 1;

    if(bin != 0)
    {
        (sbac->code) += (sbac->range);
    }

    (sbac->range) <<= 1;
    (sbac->code) <<= 1;

    if(--(sbac->code_bits) == 0)
    {
        sbac_carry_propagate(sbac, bs);
        sbac->code_bits = 8;
    }
}

static void sbac_write_unary_sym(u32 sym, u32 num_ctx, XEVE_SBAC *sbac, SBAC_CTX_MODEL *model, XEVE_BSW *bs)
{
    u32 ctx_idx = 0;

    xeve_sbac_encode_bin(sym ? 1 : 0, sbac, model, bs);

    if(sym == 0)
    {
        return;
    }

    while(sym--)
    {
        if(ctx_idx < num_ctx - 1)
        {
            ctx_idx++;
        }
        xeve_sbac_encode_bin(sym ? 1 : 0, sbac, &model[ctx_idx], bs);
    }
}

void sbac_write_truncate_unary_sym(u32 sym, u32 num_ctx, u32 max_num, XEVE_SBAC *sbac, SBAC_CTX_MODEL *model, XEVE_BSW *bs)
{
    u32 ctx_idx = 0;
    int symbol = 0;

    if(max_num > 1)
    {
        for(ctx_idx = 0; ctx_idx < max_num - 1; ++ctx_idx)
        {
            symbol = (ctx_idx == sym) ? 0 : 1;
            xeve_sbac_encode_bin(symbol, sbac, model + (ctx_idx > max_num - 1 ? max_num - 1 : ctx_idx), bs);

            if(symbol == 0)
                break;
        }
    }
}

void sbac_encode_bins_ep(u32 value, int num_bin, XEVE_SBAC *sbac, XEVE_BSW *bs)
{
    int bin = 0;
    for(bin = num_bin - 1; bin >= 0; bin--)
    {
        sbac_encode_bin_ep(value & (1 << (u32)bin), sbac, bs);
    }
}

void xeve_sbac_encode_bin(u32 bin, XEVE_SBAC *sbac, SBAC_CTX_MODEL *model, XEVE_BSW *bs)
{
    u32 lps;
    u16 mps, state;

    sbac->bin_counter++;

    state = (*model) >> 1;
    mps = (*model) & 1;

    lps = (state * (sbac->range)) >> 9;
    lps = lps < 437 ? 437 : lps;

    sbac->range -= lps;

#if TRACE_BIN
    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("model ");
    XEVE_TRACE_INT(*model);
    XEVE_TRACE_STR("range ");
    XEVE_TRACE_INT(sbac->range);
    XEVE_TRACE_STR("lps ");
    XEVE_TRACE_INT(lps);
    XEVE_TRACE_STR("\n");
#endif

    if(bin != mps)
    {
        if(sbac->range >= lps)
        {
            sbac->code += sbac->range;
            sbac->range = lps;
        }

        state = state + ((512 - state + 16) >> 5);
        if(state > 256)
        {
            mps = 1 - mps;
            state = 512 - state;
        }
        *model = (state << 1) + mps;
    }
    else
    {
        state = state - ((state + 16) >> 5);
        *model = (state << 1) + mps;
    }

    while(sbac->range < 8192)
    {
        sbac->range <<= 1;
        sbac->code <<= 1;
        sbac->code_bits--;

        if(sbac->code_bits == 0)
        {
            sbac_carry_propagate(sbac, bs);
            sbac->code_bits = 8;
        }
    }
}

void xeve_sbac_encode_bin_trm(u32 bin, XEVE_SBAC *sbac, XEVE_BSW *bs)
{
    sbac->bin_counter++;
    sbac->range--;

    if(bin)
    {
        sbac->code += sbac->range;
        sbac->range = 1;
    }

    while(sbac->range < 8192)
    {
        sbac->range <<= 1;
        sbac->code <<= 1;
        if(--(sbac->code_bits) == 0)
        {
            sbac_carry_propagate(sbac, bs);
            sbac->code_bits = 8;
        }
    }
}

void xeve_sbac_reset(XEVE_SBAC *sbac, u8 slice_type, u8 slice_qp, int sps_cm_init_flag)
{
    XEVE_SBAC_CTX *sbac_ctx;
    sbac_ctx = &sbac->ctx;

    /* Initialization of the internal variables */
    sbac->range = 16384;
    sbac->code = 0;
    sbac->code_bits = 11;
    sbac->pending_byte = 0;
    sbac->is_pending_byte = 0;
    sbac->stacked_ff = 0;
    sbac->stacked_zero = 0;
    sbac->bin_counter = 0;

    xeve_mset(sbac_ctx, 0x00, sizeof(*sbac_ctx));

    SBAC_CTX_MODEL* tmp = (SBAC_CTX_MODEL*)sbac_ctx;
    for (int i = 0; i < sizeof(*sbac_ctx) / 2; ++i) {
        *tmp = PROB_INIT;
        tmp++;
    }
    sbac_ctx->sps_cm_init_flag = sps_cm_init_flag;

}

void xeve_sbac_finish(XEVE_BSW *bs)
{
    XEVE_SBAC *sbac;
    u32 tmp;

    sbac = GET_SBAC_ENC(bs);

    tmp = (sbac->code + sbac->range - 1) & (0xFFFFFFFF << 14);
    if(tmp < sbac->code)
    {
        tmp += 8192;
    }

    sbac->code = tmp << sbac->code_bits;
    sbac_carry_propagate(sbac, bs);

    sbac->code <<= 8;
    sbac_carry_propagate(sbac, bs);

    while(sbac->stacked_zero > 0)
    {
#if TRACE_HLS
        xeve_bsw_write_trace(bs, 0x00, 0, 8);
#else
        xeve_bsw_write(bs, 0x00, 8);
#endif
        sbac->stacked_zero--;
    }
    if(sbac->pending_byte != 0)
    {
#if TRACE_HLS
        xeve_bsw_write_trace(bs, sbac->pending_byte, 0, 8);
#else
        xeve_bsw_write(bs, sbac->pending_byte, 8);
#endif
    }
    else
    {
        if(sbac->code_bits < 4)
        {
#if TRACE_HLS
            xeve_bsw_write_trace(bs, 0, 0, 4 - sbac->code_bits);
#else
            xeve_bsw_write(bs, 0, 4 - sbac->code_bits);
#endif

            while(!XEVE_BSW_IS_BYTE_ALIGN(bs))
            {
#if TRACE_HLS
                xeve_bsw_write1_trace(bs, 0, 0);
#else
                xeve_bsw_write1(bs, 0);
#endif
            }
        }
    }
}

void xeve_eco_skip_flag(XEVE_BSW * bs, int flag, int ctx)
{
    XEVE_SBAC *sbac;
    sbac = GET_SBAC_ENC(bs);
    xeve_sbac_encode_bin(flag, sbac, sbac->ctx.skip_flag + ctx, bs);

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("skip flag ");
    XEVE_TRACE_INT(flag);
    XEVE_TRACE_STR("ctx ");
    XEVE_TRACE_INT(ctx);
    XEVE_TRACE_STR("\n");
}

void xeve_eco_direct_mode_flag(XEVE_BSW *bs, int direct_mode_flag)
{
    XEVE_SBAC *sbac;
    sbac = GET_SBAC_ENC(bs);
    xeve_sbac_encode_bin(direct_mode_flag, sbac, sbac->ctx.direct_mode_flag, bs);

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("direct_mode_flag ");
    XEVE_TRACE_INT(direct_mode_flag ? PRED_DIR : 0);
    XEVE_TRACE_STR("\n");
}

void xeve_eco_tile_end_flag(XEVE_BSW * bs, int flag)
{
    XEVE_SBAC *sbac;
    sbac = GET_SBAC_ENC(bs);
    xeve_sbac_encode_bin_trm(flag, sbac, bs);
}

void xeve_eco_run_length_cc(XEVE_CTX * ctx, XEVE_BSW *bs, s16 *coef, int log2_w, int log2_h, int num_sig, int ch_type)
{
    XEVE_SBAC     * sbac;
    XEVE_SBAC_CTX * sbac_ctx;
    u32             num_coeff, scan_pos;
    u32             sign, level, prev_level, run, last_flag;
    s32             t0;
    const u16     * scanp;
    s16             coef_cur;
    int             ctx_last = 0;

    sbac = GET_SBAC_ENC(bs);
    sbac_ctx = &sbac->ctx;
    scanp = xeve_tbl_scan[log2_w - 1][log2_h - 1];
    num_coeff = 1 << (log2_w + log2_h);
    run = 0;
    prev_level = 6;

    for(scan_pos = 0; scan_pos < num_coeff; scan_pos++)
    {
        coef_cur = coef[scanp[scan_pos]];
        if(coef_cur)
        {
            level = XEVE_ABS16(coef_cur);
            sign = (coef_cur > 0) ? 0 : 1;
            t0 = sbac->ctx.sps_cm_init_flag == 1 ? ((XEVE_MIN(prev_level - 1, 5)) << 1) + (ch_type == Y_C ? 0 : 12) : (ch_type == Y_C ? 0 : 2);

            /* Run coding */
            sbac_write_unary_sym(run, 2, sbac, sbac_ctx->run + t0, bs);

            /* Level coding */
            sbac_write_unary_sym(level - 1, 2, sbac, sbac_ctx->level + t0, bs);

            /* Sign coding */
            sbac_encode_bin_ep(sign, sbac, bs);

            if(scan_pos == num_coeff - 1)
            {
                break;
            }

            run = 0;
            prev_level = level;
            num_sig--;

            /* Last flag coding */
            last_flag = (num_sig == 0) ? 1 : 0;
            ctx_last = (ch_type == Y_C) ? 0 : 1;
            xeve_sbac_encode_bin(last_flag, sbac, sbac_ctx->last + ctx_last, bs);

            if(last_flag)
            {
                break;
            }
        }
        else
        {
            run++;
        }
    }

#if ENC_DEC_TRACE
    XEVE_TRACE_STR("coef luma ");
    for (scan_pos = 0; scan_pos < num_coeff; scan_pos++)
    {
        XEVE_TRACE_INT(coef[scan_pos]);
    }
    XEVE_TRACE_STR("\n");
#endif
}


static void xeve_eco_xcoef(XEVE_CTX * ctx, XEVE_BSW *bs, s16 *coef, int log2_w, int log2_h, int num_sig, int ch_type)
{
    xeve_eco_run_length_cc(ctx, bs, coef, log2_w, log2_h, num_sig, (ch_type == Y_C ? 0 : 1));

#if TRACE_COEFFS
    int cuw = 1 << log2_w;
    int cuh = 1 << log2_h;
    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("Coeff for ");
    XEVE_TRACE_INT(ch_type);
    XEVE_TRACE_STR(": ");
    for (int i = 0; i < (cuw * cuh); ++i)
    {
        if (i != 0)
            XEVE_TRACE_STR(", ");
        XEVE_TRACE_INT(coef[i]);
    }
    XEVE_TRACE_STR("\n");
#endif
}

int xeve_eco_cbf(XEVE_BSW * bs, int cbf_y, int cbf_u, int cbf_v, u8 pred_mode, int b_no_cbf, int is_sub,int sub_pos, int cbf_all
               , int run[N_C], TREE_CONS tree_cons, int chroma_format_idc)
{
    XEVE_SBAC    *sbac;
    XEVE_SBAC_CTX *sbac_ctx;

    sbac = GET_SBAC_ENC(bs);
    sbac_ctx = &sbac->ctx;

    /* code allcbf */
    if(pred_mode != MODE_INTRA && !xeve_check_only_intra(tree_cons) )
    {
        if (!cbf_all && sub_pos)
        {
            return 0;
        }
        if(b_no_cbf == 1)
        {
            xeve_assert(cbf_all != 0);
        }
        else if(sub_pos == 0 && (run[Y_C] + run[U_C] + run[V_C]) == 3) // not count bits of root_cbf when checking each component
        {
            if(cbf_all == 0)
            {
                xeve_sbac_encode_bin(0, sbac, sbac_ctx->cbf_all, bs);

                XEVE_TRACE_COUNTER;
                XEVE_TRACE_STR("all_cbf ");
                XEVE_TRACE_INT(0);
                XEVE_TRACE_STR("\n");

                return 1;
            }
            else
            {
                xeve_sbac_encode_bin(1, sbac, sbac_ctx->cbf_all, bs);

                XEVE_TRACE_COUNTER;
                XEVE_TRACE_STR("all_cbf ");
                XEVE_TRACE_INT(1);
                XEVE_TRACE_STR("\n");
            }
        }

        if (run[U_C] && chroma_format_idc)
        {
            xeve_sbac_encode_bin(cbf_u, sbac, sbac_ctx->cbf_cb, bs);
            XEVE_TRACE_COUNTER;
            XEVE_TRACE_STR("cbf U ");
            XEVE_TRACE_INT(cbf_u);
            XEVE_TRACE_STR("\n");
        }
        if (run[V_C] && chroma_format_idc)
        {
            xeve_sbac_encode_bin(cbf_v, sbac, sbac_ctx->cbf_cr, bs);
            XEVE_TRACE_COUNTER;
            XEVE_TRACE_STR("cbf V ");
            XEVE_TRACE_INT(cbf_v);
            XEVE_TRACE_STR("\n");
        }

        if (run[Y_C] && (cbf_u + cbf_v != 0 || is_sub))
        {
            xeve_sbac_encode_bin(cbf_y, sbac, sbac_ctx->cbf_luma, bs);
            XEVE_TRACE_COUNTER;
            XEVE_TRACE_STR("cbf Y ");
            XEVE_TRACE_INT(cbf_y);
            XEVE_TRACE_STR("\n");
        }
    }
    else
    {
        if (run[U_C] && chroma_format_idc)
        {
            xeve_assert(xeve_check_chroma(tree_cons));
            xeve_sbac_encode_bin(cbf_u, sbac, sbac_ctx->cbf_cb, bs);
            XEVE_TRACE_COUNTER;
            XEVE_TRACE_STR("cbf U ");
            XEVE_TRACE_INT(cbf_u);
            XEVE_TRACE_STR("\n");
        }
        if (run[V_C] && chroma_format_idc)
        {
            xeve_assert(xeve_check_chroma(tree_cons));
            xeve_sbac_encode_bin(cbf_v, sbac, sbac_ctx->cbf_cr, bs);
            XEVE_TRACE_COUNTER;
            XEVE_TRACE_STR("cbf V ");
            XEVE_TRACE_INT(cbf_v);
            XEVE_TRACE_STR("\n");
        }
        if (run[Y_C])
        {
            xeve_assert(xeve_check_luma(tree_cons));
            xeve_sbac_encode_bin(cbf_y, sbac, sbac_ctx->cbf_luma, bs);
            XEVE_TRACE_COUNTER;
            XEVE_TRACE_STR("cbf Y ");
            XEVE_TRACE_INT(cbf_y);
            XEVE_TRACE_STR("\n");
        }
    }

    return 0;
}

int xeve_eco_dqp(XEVE_BSW * bs, int ref_qp, int cur_qp)
{
    int abs_dqp, dqp, t0;
    u32 sign;
    XEVE_SBAC    *sbac;
    XEVE_SBAC_CTX *sbac_ctx;

    sbac = GET_SBAC_ENC(bs);
    sbac_ctx = &sbac->ctx;

    dqp = cur_qp - ref_qp;
    abs_dqp = XEVE_ABS(dqp);
    t0 = abs_dqp;

    sbac_write_unary_sym(t0, NUM_CTX_DELTA_QP, sbac, sbac_ctx->delta_qp, bs);

    if(abs_dqp > 0)
    {
        sign = dqp > 0 ? 0 : 1;
        sbac_encode_bin_ep(sign, sbac, bs);
    }

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("dqp ");
    XEVE_TRACE_INT(dqp);
    XEVE_TRACE_STR("\n");

    return XEVE_OK;
}

static int xeve_eco_coefficient(XEVE_BSW * bs, s16 coef[N_C][MAX_CU_DIM], int log2_cuw, int log2_cuh, u8 pred_mode, int nnz_sub[N_C][MAX_SUB_TB_NUM], int b_no_cbf, int run_stats
                              , XEVE_CTX * ctx, XEVE_CORE * core, int enc_dqp, u8 cur_qp, TREE_CONS tree_cons)
{
    int run[N_C] = { run_stats & 1, (run_stats >> 1) & 1, (run_stats >> 2) & 1 };
    s16 *coef_temp[N_C];
    s16 coef_temp_buf[N_C][MAX_TR_DIM];
    int i, j, c;
    int log2_w_sub = (log2_cuw > MAX_TR_LOG2) ? MAX_TR_LOG2 : log2_cuw;
    int log2_h_sub = (log2_cuh > MAX_TR_LOG2) ? MAX_TR_LOG2 : log2_cuh;
    int loop_w = (log2_cuw > MAX_TR_LOG2) ? (1 << (log2_cuw - MAX_TR_LOG2)) : 1;
    int loop_h = (log2_cuh > MAX_TR_LOG2) ? (1 << (log2_cuh - MAX_TR_LOG2)) : 1;
    int stride = (1 << log2_cuw);
    int sub_stride = (1 << log2_w_sub);
    int is_sub = loop_h + loop_w > 2 ? 1 : 0;
    int w_shift = ctx->param.cs_w_shift;
    int h_shift = ctx->param.cs_h_shift;

    if (!xeve_check_luma(tree_cons))
    {
        xeve_assert(run[0] == 0);
    }
    if (!xeve_check_chroma(tree_cons))
    {
        xeve_assert((run[1] == 0) && (run[2] == 0));
    }
    xeve_assert(run_stats != 0);

    int cbf_all = 0;
    u8 is_intra = (pred_mode == MODE_INTRA) ? 1 : 0;
    XEVE_SBAC    * sbac = GET_SBAC_ENC(bs);

    for (j = 0; j < loop_h; j++)
    {
        for (i = 0; i < loop_w; i++)
        {
            for (c = 0; c < N_C; c++)
            {
                if (run[c])
                {
                    cbf_all += !!nnz_sub[c][(j << 1) | i];
                }
            }
        }
    }

    for (j = 0; j < loop_h; j++)
    {
        for (i = 0; i < loop_w; i++)
        {
            int is_cbf_all_coded_zero = xeve_eco_cbf(bs, !!nnz_sub[Y_C][(j << 1) | i], !!nnz_sub[U_C][(j << 1) | i], !!nnz_sub[V_C][(j << 1) | i], pred_mode, b_no_cbf, is_sub, j + i, cbf_all, run, tree_cons, ctx->sps.chroma_format_idc);

            if (is_cbf_all_coded_zero)
            {
                return XEVE_OK;
            }

            if(ctx->pps.cu_qp_delta_enabled_flag)
            {
                if(enc_dqp == 1)
                {
                    int cbf_for_dqp = (!!nnz_sub[Y_C][(j << 1) | i]) || (!!nnz_sub[U_C][(j << 1) | i]) || (!!nnz_sub[V_C][(j << 1) | i]);
                    if(cbf_for_dqp)
                    {
                        xeve_eco_dqp(bs, ctx->tile[core->tile_idx].qp_prev_eco[core->thread_cnt], cur_qp);
                        core->cu_qp_delta_is_coded = 1;
                        ctx->tile[core->tile_idx].qp_prev_eco[core->thread_cnt] = cur_qp;
                    }
                }
            }

            for (c = 0; c < N_C; c++)
            {
                if (nnz_sub[c][(j << 1) | i] && run[c])
                {
                    int pos_sub_x = c == 0 ? i * (1 << (log2_w_sub)) : (i * (1 << (log2_w_sub - w_shift)));
                    int pos_sub_y = c == 0 ? j * (1 << (log2_h_sub)) * (stride) : j * (1 << (log2_h_sub - h_shift)) * (stride >> w_shift);

                    if (is_sub)
                    {
                        if(c == 0)
                        xeve_block_copy(coef[c] + pos_sub_x + pos_sub_y, stride >> (!!c), coef_temp_buf[c], sub_stride >> (!!c), log2_w_sub - (!!c), log2_h_sub - (!!c));
                        else
                            xeve_block_copy(coef[c] + pos_sub_x + pos_sub_y, stride >> w_shift, coef_temp_buf[c], sub_stride >> w_shift, log2_w_sub - w_shift, log2_h_sub - h_shift);
                        coef_temp[c] = coef_temp_buf[c];
                    }
                    else
                    {
                        coef_temp[c] = coef[c];
                    }
                    if(c == 0)
                    xeve_eco_xcoef(ctx, bs, coef_temp[c], log2_w_sub - (!!c), log2_h_sub - (!!c), nnz_sub[c][(j << 1) | i], c);
                    else
                        xeve_eco_xcoef(ctx, bs, coef_temp[c], log2_w_sub - w_shift, log2_h_sub - h_shift, nnz_sub[c][(j << 1) | i], c);

                    if (is_sub)
                    {
                        if(c==0)
                        xeve_block_copy(coef_temp_buf[c], sub_stride >> (!!c), coef[c] + pos_sub_x + pos_sub_y, stride >> (!!c), log2_w_sub - (!!c), log2_h_sub - (!!c));
                        else
                            xeve_block_copy(coef_temp_buf[c], sub_stride >> w_shift, coef[c] + pos_sub_x + pos_sub_y, stride >> w_shift, log2_w_sub - w_shift, log2_h_sub - h_shift);
                    }
                }
            }
        }
    }
    return XEVE_OK;
}

int xeve_eco_coef(XEVE_CTX * ctx, XEVE_CORE * core, XEVE_BSW * bs, s16 coef[N_C][MAX_CU_DIM], u8 pred_mode, int enc_dqp, int b_no_cbf, int run_stats)
{
    return xeve_eco_coefficient(bs, coef, core->log2_cuw, core->log2_cuh, pred_mode, core->nnz_sub, b_no_cbf, run_stats
                              , ctx, core, enc_dqp, core->qp, core->tree_cons);
}

int xeve_eco_pred_mode(XEVE_BSW * bs, u8 pred_mode, int ctx)
{
    XEVE_SBAC * sbac = GET_SBAC_ENC(bs);

    xeve_sbac_encode_bin(pred_mode == MODE_INTRA, sbac, sbac->ctx.pred_mode + ctx, bs);
    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("pred mode ");
    XEVE_TRACE_INT(pred_mode == MODE_INTRA ? MODE_INTRA : MODE_INTER);
    XEVE_TRACE_STR("\n");

    return XEVE_OK;
}

int xeve_eco_intra_dir(XEVE_BSW *bs, u8 ipm, u8  * mpm)
{
    XEVE_SBAC *sbac;

    sbac = GET_SBAC_ENC(bs);
    sbac_write_unary_sym(mpm[ipm], 2, sbac, sbac->ctx.intra_dir, bs);
    XEVE_TRACE_COUNTER;
#if TRACE_ADDITIONAL_FLAGS
    XEVE_TRACE_STR("mpm list: ");
    for (int i = 0; i < IPD_CNT_B; i++)
    {
        XEVE_TRACE_INT(mpm[i]);
    }
#endif
    XEVE_TRACE_STR("ipm Y ");
    XEVE_TRACE_INT(ipm);
    XEVE_TRACE_STR("\n");
    return XEVE_OK;
}

void xeve_eco_inter_pred_idc(XEVE_BSW *bs, s8 refi[REFP_NUM], int slice_type, int cuw, int cuh, int is_sps_admvp)
{
    XEVE_SBAC *sbac;
    sbac = GET_SBAC_ENC(bs);

    if(REFI_IS_VALID(refi[REFP_0]) && REFI_IS_VALID(refi[REFP_1])) /* PRED_BI */
    {
        assert(check_bi_applicability(slice_type, cuw, cuh, is_sps_admvp));
        xeve_sbac_encode_bin(0, sbac, sbac->ctx.inter_dir, bs);
    }
    else
    {
        if (check_bi_applicability(slice_type, cuw, cuh, is_sps_admvp))
        {
            xeve_sbac_encode_bin(1, sbac, sbac->ctx.inter_dir, bs);
        }

        if(REFI_IS_VALID(refi[REFP_0])) /* PRED_L0 */
        {
            xeve_sbac_encode_bin(0, sbac, sbac->ctx.inter_dir + 1, bs);
        }
        else /* PRED_L1 */
        {
            xeve_sbac_encode_bin(1, sbac, sbac->ctx.inter_dir + 1, bs);
        }
    }
#if ENC_DEC_TRACE
    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("inter dir ");
    XEVE_TRACE_INT(REFI_IS_VALID(refi[REFP_0]) && REFI_IS_VALID(refi[REFP_1])? PRED_BI : (REFI_IS_VALID(refi[REFP_0]) ? PRED_L0 : PRED_L1));
    XEVE_TRACE_STR("\n");
#endif
    return;
}

int xeve_eco_refi(XEVE_BSW *bs, int num_refp, int refi)
{
    XEVE_SBAC    *sbac = GET_SBAC_ENC(bs);
    XEVE_SBAC_CTX *sbac_ctx = &sbac->ctx;
    int            i, bin;

    if(num_refp > 1)
    {
        if(refi == 0)
        {
            xeve_sbac_encode_bin(0, sbac, sbac_ctx->refi, bs);
        }
        else
        {
            xeve_sbac_encode_bin(1, sbac, sbac_ctx->refi, bs);
            if(num_refp > 2)
            {
                for(i = 2; i < num_refp; i++)
                {
                    bin = (i == refi + 1) ? 0 : 1;
                    if(i == 2)
                    {
                        xeve_sbac_encode_bin(bin, sbac, sbac_ctx->refi + 1, bs);
                    }
                    else
                    {
                        sbac_encode_bin_ep(bin, sbac, bs);
                    }
                    if(bin == 0)
                    {
                        break;
                    }
                }
            }
        }
    }

    return XEVE_OK;
}

int xeve_eco_mvp_idx(XEVE_BSW *bs, int mvp_idx)
{
    XEVE_SBAC *sbac = GET_SBAC_ENC(bs);
    XEVE_SBAC_CTX *sbac_ctx = &sbac->ctx;

    sbac_write_truncate_unary_sym(mvp_idx, 3, 4, sbac, sbac_ctx->mvp_idx, bs);

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("mvp idx ");
    XEVE_TRACE_INT(mvp_idx);
    XEVE_TRACE_STR("\n");

    return XEVE_OK;
}

static int xeve_eco_abs_mvd(u32 sym, XEVE_SBAC *sbac, SBAC_CTX_MODEL *model, XEVE_BSW *bs)
{
    u32 val = sym;
    s32 len_i, len_c, info, nn;
    u32  code;
    s32 i;

    nn = ((val + 1) >> 1);
    for(len_i = 0; len_i < 16 && nn != 0; len_i++)
    {
        nn >>= 1;
    }

    info = val + 1 - (1 << len_i);
    code = (1 << len_i) | ((info)& ((1 << len_i) - 1));

    len_c = (len_i << 1) + 1;

    for(i = 0; i < len_c; i++)
    {
        int bin = (code >> (len_c - 1 - i)) & 0x01;
        if(i <= 1)
        {
            xeve_sbac_encode_bin(bin, sbac, model, bs); /* use one context model for two bins */
        }
        else
        {
            sbac_encode_bin_ep(bin, sbac, bs);
        }
    }

    return XEVE_OK;
}

int xeve_eco_mvd(XEVE_BSW *bs, s16 mvd[MV_D])
{
    XEVE_SBAC    *sbac;
    XEVE_SBAC_CTX *sbac_ctx;
    int            t0;
    s32            mv;

    sbac     = GET_SBAC_ENC(bs);
    sbac_ctx = &sbac->ctx;

    t0 = 0;

    mv = mvd[MV_X];
    if(mvd[MV_X] < 0)
    {
        t0 = 1;
        mv = -mvd[MV_X];
    }
    xeve_eco_abs_mvd(mv, sbac, sbac_ctx->mvd, bs);

    if(mv)
    {
        sbac_encode_bin_ep(t0, sbac, bs);
    }

    t0 = 0;
    mv = mvd[MV_Y];
    if(mvd[MV_Y] < 0)
    {
        t0 = 1;
        mv = -mvd[MV_Y];
    }

    xeve_eco_abs_mvd(mv, sbac, sbac_ctx->mvd, bs);

    if(mv)
    {
        sbac_encode_bin_ep(t0, sbac, bs);
    }

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("mvd x ");
    XEVE_TRACE_INT(mvd[MV_X]);
    XEVE_TRACE_STR("mvd y ");
    XEVE_TRACE_INT(mvd[MV_Y]);
    XEVE_TRACE_STR("\n");

    return XEVE_OK;
}

int cu_init(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int cup, int cuw, int cuh)
{
    XEVE_CU_DATA *cu_data = &ctx->map_cu_data[core->lcu_num];

    core->cuw = cuw;
    core->cuh = cuh;
    core->log2_cuw = XEVE_LOG2(cuw);
    core->log2_cuh = XEVE_LOG2(cuh);
    core->x_scu = PEL2SCU(x);
    core->y_scu = PEL2SCU(y);
    core->scup = ((u32)core->y_scu * ctx->w_scu) + core->x_scu;
    core->avail_cu = 0;
    core->skip_flag = 0;
    core->nnz[Y_C] = core->nnz[U_C] = core->nnz[V_C] = 0;
    core->cu_mode = xeve_check_luma(core->tree_cons) ? cu_data->pred_mode[cup] : cu_data->pred_mode_chroma[cup];
    core->qp = cu_data->qp_y[cup] - 6 * ctx->sps.bit_depth_luma_minus8;

    if (core->cu_mode == MODE_INTRA)
    {
        core->avail_cu = xeve_get_avail_intra(core->x_scu, core->y_scu, ctx->w_scu, ctx->h_scu, core->scup, core->log2_cuw, core->log2_cuh, ctx->map_scu, ctx->map_tidx);
    }
    else if(core->cu_mode <= MODE_DIR)
    {
        xeve_assert(xeve_check_luma(core->tree_cons));

        if(cu_data->pred_mode[cup] == MODE_SKIP)
        {
            core->skip_flag = 1;
        }

        core->avail_cu = xeve_get_avail_inter(core->x_scu, core->y_scu, ctx->w_scu, ctx->h_scu, core->scup, core->cuw, core->cuh, ctx->map_scu, ctx->map_tidx);
    }

    core->avail_lr = xeve_check_nev_avail(core->x_scu, core->y_scu, cuw, cuh, ctx->w_scu, ctx->h_scu, ctx->map_scu, ctx->map_tidx);

    return XEVE_OK;
}

void coef_rect_to_series(XEVE_CTX * ctx, s16 *coef_src[N_C], int x, int y, int cuw, int cuh, s16 coef_dst[N_C][MAX_CU_DIM], XEVE_CORE * core)
{
    int i, j, sidx, didx;
    int w_shift = ctx->param.cs_w_shift;
    int h_shift = ctx->param.cs_h_shift;

    sidx = (x&(ctx->max_cuwh - 1)) + ((y&(ctx->max_cuwh - 1)) << ctx->log2_max_cuwh);
    didx = 0;

    if (xeve_check_luma(core->tree_cons))
    {
        for(j = 0; j < cuh; j++)
        {
            for(i = 0; i < cuw; i++)
            {
                coef_dst[Y_C][didx++] = coef_src[Y_C][sidx + i];
            }
            sidx += ctx->max_cuwh;
        }
    }
    if (xeve_check_chroma(core->tree_cons) && ctx->sps.chroma_format_idc)
    {
        x >>= w_shift;
        y >>= h_shift;
        cuw >>= w_shift;
        cuh >>= h_shift;
        sidx = (x&((ctx->max_cuwh >> w_shift) - 1)) + ((y&((ctx->max_cuwh >> h_shift) - 1)) << (ctx->log2_max_cuwh - w_shift));
        didx = 0;

        for(j = 0; j < cuh; j++)
        {
            for(i = 0; i < cuw; i++)
            {
                coef_dst[U_C][didx] = coef_src[U_C][sidx + i];
                coef_dst[V_C][didx] = coef_src[V_C][sidx + i];
                didx++;
            }
            sidx += (ctx->max_cuwh >> w_shift);
        }
    }
}

int xeve_eco_split_mode(XEVE_BSW *bs, XEVE_CTX *c, XEVE_CORE *core, int cud, int cup, int cuw, int cuh, int lcu_s, int x, int y)
{
    XEVE_SBAC *sbac;
    int ret = XEVE_OK;
    s8 split_mode;

    if(cuw < 8 && cuh < 8)
    {
        return ret;
    }

    sbac = GET_SBAC_ENC(bs);

    if(sbac->is_bitcount)
    {
        xeve_get_split_mode(&split_mode, cud, cup, cuw, cuh, lcu_s, core->cu_data_temp[XEVE_LOG2(cuw) - 2][XEVE_LOG2(cuh) - 2].split_mode);
    }
    else
    {
        xeve_get_split_mode(&split_mode, cud, cup, cuw, cuh, lcu_s, c->map_cu_data[core->lcu_num].split_mode);
    }

    xeve_sbac_encode_bin(split_mode != NO_SPLIT, sbac, sbac->ctx.split_cu_flag, bs); /* split_cu_flag */

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("x pos ");
    XEVE_TRACE_INT(core->x_pel + ((cup % (c->max_cuwh >> MIN_CU_LOG2)) << MIN_CU_LOG2));
    XEVE_TRACE_STR("y pos ");
    XEVE_TRACE_INT(core->y_pel + ((cup / (c->max_cuwh >> MIN_CU_LOG2)) << MIN_CU_LOG2));
    XEVE_TRACE_STR("width ");
    XEVE_TRACE_INT(cuw);
    XEVE_TRACE_STR("height ");
    XEVE_TRACE_INT(cuh);
    XEVE_TRACE_STR("depth ");
    XEVE_TRACE_INT(cud);
    XEVE_TRACE_STR("split mode ");
    XEVE_TRACE_INT(split_mode);
    XEVE_TRACE_STR("\n");

    return ret;
}

int xeve_eco_unit(XEVE_CTX * ctx, XEVE_CORE * core, int x, int y, int cup, int cuw, int cuh , TREE_CONS tree_cons, XEVE_BSW * bs)
{
    core->tree_cons = tree_cons;
    s16(*coef)[MAX_CU_DIM] = core->ctmp;

    u32 *map_scu;
    int slice_type, refi0, refi1;
    int i, j, w, h;
    XEVE_CU_DATA *cu_data = &ctx->map_cu_data[core->lcu_num];
    u32 *map_cu_mode;

#if TRACE_ENC_CU_DATA
    core->trace_idx = cu_data->trace_idx[cup];
#endif

#if TRACE_ENC_CU_DATA_CHECK
    xeve_assert(core->trace_idx != 0);
#endif
    int enc_dqp;
    slice_type = ctx->slice_type;
    cu_init(ctx, core, x, y, cup, cuw, cuh);

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("poc: ");
    XEVE_TRACE_INT(ctx->poc.poc_val);
    XEVE_TRACE_STR("x pos ");
    XEVE_TRACE_INT(core->x_pel + ((cup % (ctx->max_cuwh >> MIN_CU_LOG2)) << MIN_CU_LOG2));
    XEVE_TRACE_STR("y pos ");
    XEVE_TRACE_INT(core->y_pel + ((cup / (ctx->max_cuwh >> MIN_CU_LOG2)) << MIN_CU_LOG2));
    XEVE_TRACE_STR("width ");
    XEVE_TRACE_INT(cuw);
    XEVE_TRACE_STR("height ");
    XEVE_TRACE_INT(cuh);
    XEVE_TRACE_STR("\n");

    xeve_get_ctx_some_flags(core->x_scu, core->y_scu, cuw, cuh, ctx->w_scu, ctx->map_scu, ctx->map_cu_mode, core->ctx_flags
                            , ctx->sh->slice_type, ctx->sps.tool_cm_init, ctx->param.ibc_flag, ctx->sps.ibc_log_max_size, ctx->map_tidx);

    if (core->skip_flag == 0)
    {
        /* get coefficients and tq */
        coef_rect_to_series(ctx, cu_data->coef, x, y, cuw, cuh, coef, core);

        for(i = 0; i < N_C; i++)
        {
            core->nnz[i] = cu_data->nnz[i][cup];

            for (j = 0; j < MAX_SUB_TB_NUM; j++)
            {
                core->nnz_sub[i][j] = cu_data->nnz_sub[i][j][cup];
            }
        }
    }
    else
    {
        xeve_mset(core->nnz, 0, sizeof(int) * N_C);
        xeve_mset(core->nnz_sub, 0, sizeof(int) * N_C * MAX_SUB_TB_NUM);
    }

    /* entropy coding a CU */
    if(slice_type != SLICE_I )
    {
        xeve_eco_skip_flag(bs, core->skip_flag,core->ctx_flags[CNID_SKIP_FLAG]);

        if(core->skip_flag)
        {
            xeve_eco_mvp_idx(bs, cu_data->mvp_idx[cup][REFP_0]);

            if(slice_type == SLICE_B)
            {
                xeve_eco_mvp_idx(bs, cu_data->mvp_idx[cup][REFP_1]);
            }
        }
        else
        {
            xeve_eco_pred_mode(bs, core->cu_mode, core->ctx_flags[CNID_PRED_MODE]);

            if(core->cu_mode != MODE_INTRA)
            {
                if (slice_type == SLICE_B)
                {
                    xeve_eco_direct_mode_flag(bs, cu_data->pred_mode[cup] == MODE_DIR);
                }

                if((cu_data->pred_mode[cup] % ORG_PRED_NUM) != MODE_DIR)
                {
                    if (slice_type == SLICE_B)
                    {
                        xeve_eco_inter_pred_idc(bs, cu_data->refi[cup], slice_type, cuw, cuh, ctx->sps.tool_admvp);
                    }

                    refi0 = cu_data->refi[cup][REFP_0];
                    refi1 = cu_data->refi[cup][REFP_1];
                    if(IS_INTER_SLICE(slice_type) && REFI_IS_VALID(refi0))
                    {
                        xeve_eco_refi(bs, ctx->rpm.num_refp[REFP_0], refi0);
                        xeve_eco_mvp_idx(bs, cu_data->mvp_idx[cup][REFP_0]);
                        xeve_eco_mvd(bs, cu_data->mvd[cup][REFP_0]);
                    }
                    if(slice_type == SLICE_B && REFI_IS_VALID(refi1))
                    {
                        xeve_eco_refi(bs, ctx->rpm.num_refp[REFP_1], refi1);
                        xeve_eco_mvp_idx(bs, cu_data->mvp_idx[cup][REFP_1]);
                        xeve_eco_mvd(bs, cu_data->mvd[cup][REFP_1]);
                    }
                }
            }
        }
    }

    if(core->cu_mode == MODE_INTRA)
    {
        xeve_assert(cu_data->ipm[0][cup] != IPD_INVALID);
        xeve_assert(cu_data->ipm[1][cup] != IPD_INVALID);

        xeve_get_mpm(core->x_scu, core->y_scu, cuw, cuh, ctx->map_scu, ctx->map_ipm, core->scup, ctx->w_scu, &core->mpm_b_list, ctx->map_tidx);
        xeve_eco_intra_dir(bs, cu_data->ipm[0][cup], core->mpm_b_list);
    }

    if(core->skip_flag == 0)
    {
        int b_no_cbf = 0;
        b_no_cbf |= cu_data->affine_flag[cup] && core->cu_mode == MODE_DIR;
        b_no_cbf |= core->cu_mode == MODE_DIR_MMVD;
        b_no_cbf |= core->cu_mode == MODE_DIR;
        if (ctx->sps.tool_admvp == 0)
        {
            b_no_cbf = 0;
        }
        enc_dqp = 1;

        ctx->fn_eco_coef(ctx, core, bs, coef, core->cu_mode, enc_dqp, b_no_cbf, RUN_L | RUN_CB | RUN_CR);
    }

    map_scu = ctx->map_scu + core->scup;
    w = (core->cuw >> MIN_CU_LOG2);
    h = (core->cuh >> MIN_CU_LOG2);
    map_cu_mode = ctx->map_cu_mode + core->scup;


    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            if(core->skip_flag)
            {
                MCU_SET_SF(map_scu[j]);
            }
            else
            {
                MCU_CLR_SF(map_scu[j]);
            }
            int sub_idx = ((!!(i & 32)) << 1) | (!!(j & 32));
            if (core->nnz_sub[Y_C][sub_idx] > 0)
            {
                MCU_SET_CBFL(map_scu[j]);
            }
            else
            {
                MCU_CLR_CBFL(map_scu[j]);
            }

            MCU_SET_COD(map_scu[j]);
            if(ctx->pps.cu_qp_delta_enabled_flag)
            {
                MCU_CLR_QP(map_scu[j]);
                MCU_SET_QP(map_scu[j], ctx->tile[core->tile_idx].qp_prev_eco[core->thread_cnt]);
            }

            MCU_SET_LOGW(map_cu_mode[j], core->log2_cuw);
            MCU_SET_LOGH(map_cu_mode[j], core->log2_cuh);
        }
        map_scu += ctx->w_scu;
        map_cu_mode += ctx->w_scu;
    }

#if TRACE_ENC_CU_DATA
    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("RDO check id ");
    XEVE_TRACE_INT((int)core->trace_idx);
    XEVE_TRACE_STR("\n");
    xeve_assert(core->trace_idx != 0);
#endif
#if MVF_TRACE
    // Trace MVF
    {
        s8(*map_refi)[REFP_NUM];
        s16(*map_mv)[REFP_NUM][MV_D];

        map_refi = ctx->map_refi + core->scup;
        map_mv = ctx->map_mv + core->scup;

        for(i = 0; i < h; i++)
        {
            for(j = 0; j < w; j++)
            {
                XEVE_TRACE_COUNTER;
                XEVE_TRACE_STR(" x: ");
                XEVE_TRACE_INT(j);
                XEVE_TRACE_STR(" y: ");
                XEVE_TRACE_INT(i);

                XEVE_TRACE_STR(" ref0: ");
                XEVE_TRACE_INT(map_refi[j][REFP_0]);
                XEVE_TRACE_STR(" mv: ");
                XEVE_TRACE_MV(map_mv[j][REFP_0][MV_X], map_mv[j][REFP_0][MV_Y]);

                XEVE_TRACE_STR(" ref1: ");
                XEVE_TRACE_INT(map_refi[j][REFP_1]);
                XEVE_TRACE_STR(" mv: ");
                XEVE_TRACE_MV(map_mv[j][REFP_1][MV_X], map_mv[j][REFP_1][MV_Y]);

                XEVE_TRACE_STR("\n");
            }

            map_refi += ctx->w_scu;
            map_mv += ctx->w_scu;
        }
    }
#endif

    return XEVE_OK;
}

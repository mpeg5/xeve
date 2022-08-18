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
#include <limits.h>

#include <math.h>

#if GRAB_STAT
#include "xevem_stat.h"
#endif
#pragma warning(disable:4018)


static void sbac_write_unary_sym_ep(u32 sym, XEVE_SBAC *sbac, XEVE_BSW *bs, u32 max_val)
{
    u32 icounter = 0;

    sbac_encode_bin_ep(sym ? 1 : 0, sbac, bs); icounter++;

    if(sym == 0)
    {
        return;
    }

    while(sym--)
    {
        if(icounter < max_val)
        {
            sbac_encode_bin_ep(sym ? 1 : 0, sbac, bs); icounter++;
        }
    }
}

void xevem_sbac_reset(XEVE_SBAC *sbac, u8 slice_type, u8 slice_qp, int sps_cm_init_flag)
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

    sbac_ctx->sps_cm_init_flag = sps_cm_init_flag;

    /* Initialization of the context models */
    if(sps_cm_init_flag == 1)
    {
        xeve_eco_sbac_ctx_initialize(sbac_ctx->cbf_luma, (s16*)xevem_init_cbf_luma, NUM_CTX_CBF_LUMA, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->cbf_cb, (s16*)xevem_init_cbf_cb, NUM_CTX_CBF_CB, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->cbf_cr, (s16*)xevem_init_cbf_cr, NUM_CTX_CBF_CR, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->cbf_all, (s16*)xevem_init_cbf_all, NUM_CTX_CBF_ALL, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->delta_qp, (s16*)xevem_init_dqp, NUM_CTX_DELTA_QP, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->sig_coeff_flag, (s16*)xevem_init_sig_coeff_flag, NUM_CTX_SIG_COEFF_FLAG, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->coeff_abs_level_greaterAB_flag, (s16*)xevem_init_coeff_abs_level_greaterAB_flag, NUM_CTX_GTX, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->last_sig_coeff_x_prefix, (s16*)xevem_init_last_sig_coeff_x_prefix, NUM_CTX_LAST_SIG_COEFF, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->last_sig_coeff_y_prefix, (s16*)xevem_init_last_sig_coeff_y_prefix, NUM_CTX_LAST_SIG_COEFF, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->pred_mode, (s16*)xevem_init_pred_mode, NUM_CTX_PRED_MODE, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->mode_cons, (s16*)xevem_init_mode_cons, NUM_CTX_MODE_CONS, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->direct_mode_flag, (s16*)xevem_init_direct_mode_flag, NUM_CTX_DIRECT_MODE_FLAG, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->merge_mode_flag, (s16*)xevem_init_merge_mode_flag, NUM_CTX_MERGE_MODE_FLAG, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->inter_dir, (s16*)xevem_init_inter_dir, NUM_CTX_INTER_PRED_IDC, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->intra_dir, (s16*)xevem_init_intra_dir, NUM_CTX_INTRA_PRED_MODE, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->intra_luma_pred_mpm_flag, (s16*)xevem_init_intra_luma_pred_mpm_flag, NUM_CTX_INTRA_LUMA_PRED_MPM_FLAG, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->intra_luma_pred_mpm_idx, (s16*)xevem_init_intra_luma_pred_mpm_idx, NUM_CTX_INTRA_LUMA_PRED_MPM_IDX, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->intra_chroma_pred_mode, (s16*)xevem_init_intra_chroma_pred_mode, NUM_CTX_INTRA_CHROMA_PRED_MODE, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->run, (s16*)xevem_init_run, NUM_CTX_CC_RUN, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->last, (s16*)xevem_init_last, NUM_CTX_CC_LAST, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->level, (s16*)xevem_init_level, NUM_CTX_CC_LEVEL, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->mmvd_flag, (s16*)xevem_init_mmvd_flag, NUM_CTX_MMVD_FLAG, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->mmvd_merge_idx, (s16*)xevem_init_mmvd_merge_idx, NUM_CTX_MMVD_MERGE_IDX, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->mmvd_distance_idx, (s16*)xevem_init_mmvd_distance_idx, NUM_CTX_MMVD_DIST_IDX, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->mmvd_direction_idx, (s16*)xevem_init_mmvd_direction_idx, NUM_CTX_MMVD_DIRECTION_IDX, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->mmvd_group_idx, (s16*)xevem_init_mmvd_group_idx, NUM_CTX_MMVD_GROUP_IDX, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->merge_idx, (s16*)xevem_init_merge_idx, NUM_CTX_MERGE_IDX, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->mvp_idx, (s16*)xevem_init_mvp_idx, NUM_CTX_MVP_IDX, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->affine_mvp_idx, (s16*)xevem_init_affine_mvp_idx, NUM_CTX_AFFINE_MVP_IDX, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->mvr_idx, (s16*)xevem_init_mvr_idx, NUM_CTX_AMVR_IDX, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->bi_idx, (s16*)xevem_init_bi_idx, NUM_CTX_BI_PRED_IDX, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->mvd, (s16*)xevem_init_mvd, NUM_CTX_MVD, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->refi, (s16*)xevem_init_refi, NUM_CTX_REF_IDX, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->btt_split_flag, (s16*)xevem_init_btt_split_flag, NUM_CTX_BTT_SPLIT_FLAG, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->btt_split_dir, (s16*)xevem_init_btt_split_dir, NUM_CTX_BTT_SPLIT_DIR, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->btt_split_type, (s16*)xevem_init_btt_split_type, NUM_CTX_BTT_SPLIT_TYPE, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->suco_flag, (s16*)xevem_init_suco_flag, NUM_CTX_SUCO_FLAG, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->alf_ctb_flag, (s16*)xevem_init_alf_ctb_flag, NUM_CTX_ALF_CTB_FLAG, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->split_cu_flag, (s16*)xevem_init_split_cu_flag, NUM_CTX_SPLIT_CU_FLAG, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->affine_flag, (s16*)xevem_init_affine_flag, NUM_CTX_AFFINE_FLAG, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->affine_mode, (s16*)xevem_init_affine_mode, NUM_CTX_AFFINE_MODE, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->affine_mrg, (s16*)xevem_init_affine_mrg, NUM_CTX_AFFINE_MRG, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->affine_mvd_flag, (s16*)xevem_init_affine_mvd_flag, NUM_CTX_AFFINE_MVD_FLAG, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->skip_flag, (s16*)xevem_init_skip_flag, NUM_CTX_SKIP_FLAG, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->ibc_flag, (s16*)xevem_init_ibc_flag, NUM_CTX_IBC_FLAG, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->ats_mode, (s16*)xevem_init_ats_mode, NUM_CTX_ATS_MODE_FLAG, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->ats_cu_inter_flag, (s16*)xevem_init_ats_cu_inter_flag, NUM_CTX_ATS_INTER_FLAG, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->ats_cu_inter_quad_flag, (s16*)xevem_init_ats_cu_inter_quad_flag, NUM_CTX_ATS_INTER_QUAD_FLAG, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->ats_cu_inter_hor_flag, (s16*)xevem_init_ats_cu_inter_hor_flag, NUM_CTX_ATS_INTER_HOR_FLAG, slice_type, slice_qp);
        xeve_eco_sbac_ctx_initialize(sbac_ctx->ats_cu_inter_pos_flag, (s16*)xevem_init_ats_cu_inter_pos_flag, NUM_CTX_ATS_INTER_POS_FLAG, slice_type, slice_qp);
    }
    else // (sps_cm_init_flag == 0)
    {
        int here = 0;
        SBAC_CTX_MODEL* tmp = (SBAC_CTX_MODEL *) sbac_ctx;
        for (int i = 0; i < sizeof(*sbac_ctx ) / 2; ++i) {
            *tmp = PROB_INIT;
            tmp++;
        }
        sbac_ctx->sps_cm_init_flag = sps_cm_init_flag;
    }
}

int xevem_eco_aps_gen(XEVE_BSW * bs, XEVE_APS_GEN * aps, int bit_depth)
{
#if TRACE_HLS
    XEVE_TRACE_STR("***********************************\n");
    XEVE_TRACE_STR("************ APS Start ************\n");
    u32 aps_id = aps->aps_id;
    u32 aps_type_id = aps->aps_type_id;
    xeve_bsw_write(bs, aps_id, APS_MAX_NUM_IN_BITS); // signal APS ID
    xeve_bsw_write(bs, aps_type_id, APS_TYPE_ID_BITS); // signal APS TYPE ID
#else
    xeve_bsw_write(bs, aps->aps_id, APS_MAX_NUM_IN_BITS); // signal APS ID
    xeve_bsw_write(bs, aps->aps_type_id, APS_TYPE_ID_BITS); // signal APS TYPE ID
#endif
    if (aps->aps_type_id == 0)
    {
        XEVE_APS local_aps;
        XEVE_ALF_SLICE_PARAM * p_aps_dataDst = (XEVE_ALF_SLICE_PARAM *)aps->aps_data;
        xeve_mcpy(&(local_aps.alf_aps_param), p_aps_dataDst, sizeof(XEVE_ALF_SLICE_PARAM));
        xevem_eco_alf_aps_param(bs, aps); // signal ALF filter parameter except ALF map
    }
    else if (aps->aps_type_id == 1)
    {
        xevem_eco_dra_aps_param(bs, aps, bit_depth); // signal ALF filter parameter except ALF map
    }
    else
    {
        xeve_trace("This version of XEVE doesnot support this APS type: %d\n", aps->aps_type_id);
    }

    u8 aps_extension_flag = 0;
    xeve_bsw_write1(bs, aps_extension_flag);
    assert(aps_extension_flag == 0);

    u32 t0 = 0;
    while (!XEVE_BSW_IS_BYTE_ALIGN(bs))
    {
        xeve_bsw_write1(bs, t0);
    }
#if TRACE_HLS
    XEVE_TRACE_STR("************ APS End   ************\n");
    XEVE_TRACE_STR("***********************************\n");
#endif
    return XEVE_OK;
}

int xeve_eco_rlp(XEVE_BSW * bs, XEVE_RPL * rpl)
{
    u32 delta_poc_st, strp_entry_sign_flag;
    xeve_bsw_write_ue(bs, rpl->ref_pic_num);
    if (rpl->ref_pic_num > 0)
    {
        delta_poc_st = (u32)abs(rpl->ref_pics[0]);
        xeve_bsw_write_ue(bs, delta_poc_st);
        if (rpl->ref_pics[0] != 0)
        {
            strp_entry_sign_flag = rpl->ref_pics[0] < 0;
            xeve_bsw_write1(bs, strp_entry_sign_flag);
        }

        for (int i = 1; i < rpl->ref_pic_num; ++i)
        {
            delta_poc_st = (u32)abs(rpl->ref_pics[i] - rpl->ref_pics[i - 1]);
            strp_entry_sign_flag = rpl->ref_pics[i - 1] > rpl->ref_pics[i];

            xeve_bsw_write_ue(bs, delta_poc_st);
            if (delta_poc_st != 0)
            {
                xeve_bsw_write1(bs, strp_entry_sign_flag);
            }
        }
    }

    return XEVE_OK;
}

int xevem_eco_sps(XEVE_BSW * bs, XEVE_SPS * sps)
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
    if (sps->sps_btt_flag)
    {
        xeve_bsw_write_ue(bs, sps->log2_ctu_size_minus5);
        xeve_bsw_write_ue(bs, sps->log2_min_cb_size_minus2);
        xeve_bsw_write_ue(bs, sps->log2_diff_ctu_max_14_cb_size);
        xeve_bsw_write_ue(bs, sps->log2_diff_ctu_max_tt_cb_size);
        xeve_bsw_write_ue(bs, sps->log2_diff_min_cb_min_tt_cb_size_minus2);
    }
    xeve_bsw_write1(bs, sps->sps_suco_flag);
    if (sps->sps_suco_flag)
    {
        xeve_bsw_write_ue(bs, sps->log2_diff_ctu_size_max_suco_cb_size);
        xeve_bsw_write_ue(bs, sps->log2_diff_max_suco_min_suco_cb_size);
    }

    xeve_bsw_write1(bs, sps->tool_admvp);
    if (sps->tool_admvp)
    {
        xeve_bsw_write1(bs, sps->tool_affine);
        xeve_bsw_write1(bs, sps->tool_amvr);
        xeve_bsw_write1(bs, sps->tool_dmvr);
        xeve_bsw_write1(bs, sps->tool_mmvd);
        xeve_bsw_write1(bs, sps->tool_hmvp);
    }

    xeve_bsw_write1(bs, sps->tool_eipd);
    if (sps->tool_eipd)
    {
        xeve_bsw_write1(bs, sps->ibc_flag);
        if (sps->ibc_flag)
        {
            xeve_bsw_write_ue(bs, (sps->ibc_log_max_size - 2));
        }
    }

    xeve_bsw_write1(bs, sps->tool_cm_init);
    if (sps->tool_cm_init)
    {
        xeve_bsw_write1(bs, sps->tool_adcc);
    }

    xeve_bsw_write1(bs, sps->tool_iqt);
    if (sps->tool_iqt)
    {
        xeve_bsw_write1(bs, sps->tool_ats);
    }

    xeve_bsw_write1(bs, sps->tool_addb);
    xeve_bsw_write1(bs, sps->tool_alf);
    xeve_bsw_write1(bs, sps->tool_htdf);
    xeve_bsw_write1(bs, sps->tool_rpl);
    xeve_bsw_write1(bs, sps->tool_pocs);
    xeve_bsw_write1(bs, sps->dquant_flag);
    xeve_bsw_write1(bs, sps->tool_dra);

    if (sps->tool_pocs)
    {
        xeve_bsw_write_ue(bs, sps->log2_max_pic_order_cnt_lsb_minus4);
    }
    if (!sps->tool_rpl || !sps->tool_pocs)
    {
        xeve_bsw_write_ue(bs, sps->log2_sub_gop_length);
        if (sps->log2_sub_gop_length == 0)
        {
            xeve_bsw_write_ue(bs, sps->log2_ref_pic_gap_length);
        }
    }

    if (!sps->tool_rpl)
    {
        xeve_bsw_write_ue(bs, sps->max_num_ref_pics);
    }
    else
    {
        xeve_bsw_write_ue(bs, sps->sps_max_dec_pic_buffering_minus1);
        xeve_bsw_write1(bs, sps->long_term_ref_pics_flag);
        xeve_bsw_write1(bs, sps->rpl1_same_as_rpl0_flag);
        xeve_bsw_write_ue(bs, sps->num_ref_pic_lists_in_sps0);

        for (int i = 0; i < sps->num_ref_pic_lists_in_sps0; ++i)
        {
            xeve_eco_rlp(bs, &sps->rpls_l0[i]);
        }

        if (!sps->rpl1_same_as_rpl0_flag)
        {
                xeve_bsw_write_ue(bs, sps->num_ref_pic_lists_in_sps1);
                for (int i = 0; i < sps->num_ref_pic_lists_in_sps1; ++i)
                xeve_eco_rlp(bs, &sps->rpls_l1[i]);
        }
    }

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

int xevem_eco_pps(XEVE_BSW * bs, XEVE_SPS * sps, XEVE_PPS * pps)
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

    if (!pps->single_tile_in_pic_flag)
    {
        xeve_bsw_write_ue(bs, pps->num_tile_columns_minus1);
        xeve_bsw_write_ue(bs, pps->num_tile_rows_minus1);
        xeve_bsw_write1(bs, pps->uniform_tile_spacing_flag);
        if (!pps->uniform_tile_spacing_flag)
        {
            for (int i = 0; i < pps->num_tile_columns_minus1; ++i)
            {
                xeve_bsw_write_ue(bs, pps->tile_column_width_minus1[i]);
            }
            for (int i = 0; i < pps->num_tile_rows_minus1; ++i)
            {
                xeve_bsw_write_ue(bs, pps->tile_row_height_minus1[i]);
            }
        }
        xeve_bsw_write1(bs, pps->loop_filter_across_tiles_enabled_flag);
        xeve_bsw_write_ue(bs, pps->tile_offset_lens_minus1);
    }

    xeve_bsw_write_ue(bs, pps->tile_id_len_minus1);
    xeve_bsw_write1(bs, pps->explicit_tile_id_flag);
    if (pps->explicit_tile_id_flag)
    {
        for (int i = 0; i <= pps->num_tile_rows_minus1; ++i)
        {
            for (int j = 0; j <= pps->num_tile_columns_minus1; ++j)
            {
                xeve_bsw_write(bs, pps->tile_id_val[i][j], pps->tile_id_len_minus1 + 1);
            }
        }
    }


    xeve_bsw_write1(bs, pps->pic_dra_enabled_flag);

    if (pps->pic_dra_enabled_flag)
    {
        xeve_bsw_write(bs, pps->pic_dra_aps_id, APS_MAX_NUM_IN_BITS);
    }

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

int xevem_eco_sh(XEVE_BSW * bs, XEVE_SPS * sps, XEVE_PPS * pps, XEVE_SH * sh, int nut)
{
#if TRACE_HLS
    XEVE_TRACE_STR("***********************************\n");
    XEVE_TRACE_STR("************ SH  Start ************\n");
#endif
    int num_tiles_in_slice;
    if (!sh->arbitrary_slice_flag)
    {
        num_tiles_in_slice = sh->num_tiles_in_slice;
    }
    else
    {
        num_tiles_in_slice = sh->num_remaining_tiles_in_slice_minus1 + 2;
    }

    xeve_bsw_write_ue(bs, sh->slice_pic_parameter_set_id);

    if (!pps->single_tile_in_pic_flag)
    {
        xeve_bsw_write1(bs, sh->single_tile_in_slice_flag);
        xeve_bsw_write(bs, sh->first_tile_id, pps->tile_id_len_minus1 + 1);
    }

    if (!sh->single_tile_in_slice_flag)
    {
        if (pps->arbitrary_slice_present_flag)
        {
            xeve_bsw_write1(bs, sh->arbitrary_slice_flag);
        }
        if (!sh->arbitrary_slice_flag)
        {
            xeve_bsw_write(bs, sh->last_tile_id, pps->tile_id_len_minus1 + 1);
        }
        else
        {
            xeve_bsw_write_ue(bs, sh->num_remaining_tiles_in_slice_minus1);
            for (int i = 0; i < num_tiles_in_slice - 1; ++i)
            {
                xeve_bsw_write_ue(bs, sh->delta_tile_id_minus1[i]);
            }
        }
    }

    xeve_bsw_write_ue(bs, sh->slice_type);

    if (nut == XEVE_IDR_NUT)
    {
        xeve_bsw_write1(bs, sh->no_output_of_prior_pics_flag);
    }

    if (sps->tool_mmvd && (sh->slice_type == SLICE_B))
    {
        xeve_bsw_write1(bs, sh->mmvd_group_enable_flag);
    }
    else if (sps->tool_mmvd && (sh->slice_type == SLICE_P))
    {
        xeve_bsw_write1(bs, sh->mmvd_group_enable_flag);
    }

    if (sps->tool_alf)
    {
        xeve_bsw_write1(bs, sh->alf_on);
        if (sh->alf_on)
        {
            xeve_bsw_write(bs, sh->aps_id_y, APS_MAX_NUM_IN_BITS);
            xevem_eco_alf_sh_param(bs, sh); // signaling ALF map

            sh->alf_chroma_idc = ((sh->alf_sh_param.enable_flag[2]) << 1) + sh->alf_sh_param.enable_flag[1];
            xeve_bsw_write(bs, sh->alf_chroma_idc, 2);
            if (sh->alf_chroma_idc == 1)
            {
                sh->ChromaAlfEnabledFlag = 1;
                sh->ChromaAlfEnabled2Flag = 0;
            }
            else if (sh->alf_chroma_idc == 2)
            {
                sh->ChromaAlfEnabledFlag = 0;
                sh->ChromaAlfEnabled2Flag = 1;
            }
            else if (sh->alf_chroma_idc == 3)
            {
                sh->ChromaAlfEnabledFlag = 1;
                sh->ChromaAlfEnabled2Flag = 1;
            }
            else
            {
                sh->ChromaAlfEnabledFlag = 0;
                sh->ChromaAlfEnabled2Flag = 0;
            }


            if (sh->alf_chroma_idc && (sps->chroma_format_idc == 1 || sps->chroma_format_idc == 2))
            {

                xeve_bsw_write(bs, sh->aps_id_ch, APS_MAX_NUM_IN_BITS);

            }

        }

        if (sps->chroma_format_idc == 3 && sh->ChromaAlfEnabledFlag)
        {
            xeve_bsw_write(bs, sh->aps_id_ch, APS_MAX_NUM_IN_BITS);
            xeve_bsw_write1(bs, sh->alfChromaMapSignalled);
        }
        if (sps->chroma_format_idc == 3 && sh->ChromaAlfEnabled2Flag)
        {
            xeve_bsw_write(bs, sh->aps_id_ch2, APS_MAX_NUM_IN_BITS);
            xeve_bsw_write1(bs, sh->alfChroma2MapSignalled);
        }

    }

    if (nut != XEVE_IDR_NUT)
    {
        if (sps->tool_pocs)
        {
            xeve_bsw_write(bs, sh->poc_lsb, sps->log2_max_pic_order_cnt_lsb_minus4 + 4);
        }
        if (sps->tool_rpl)
        {
            //L0 candidates signaling
            if (sps->num_ref_pic_lists_in_sps0 > 0)
            {
                xeve_bsw_write1(bs, sh->ref_pic_list_sps_flag[0]);
            }
            if (sh->ref_pic_list_sps_flag[0])
            {
                if (sps->num_ref_pic_lists_in_sps0 > 1)
                {
                    xeve_bsw_write_ue(bs, sh->rpl_l0_idx);
                }
            }
            else
            {
                xeve_eco_rlp(bs, &sh->rpl_l0);
            }

            //L1 candidates signaling
            if (sps->num_ref_pic_lists_in_sps1 > 0 && pps->rpl1_idx_present_flag)
            {
                xeve_bsw_write1(bs, sh->ref_pic_list_sps_flag[1]);
            }

            if (sh->ref_pic_list_sps_flag[1])
            {
                if (sps->num_ref_pic_lists_in_sps1 > 1 && pps->rpl1_idx_present_flag)
                {
                    xeve_bsw_write_ue(bs, sh->rpl_l1_idx);
                }
            }
            else
            {
                xeve_eco_rlp(bs, &sh->rpl_l1);
            }
        }
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

        if (sps->tool_admvp)
        {
            xeve_bsw_write1(bs, sh->temporal_mvp_asigned_flag);
            if (sh->temporal_mvp_asigned_flag)
            {
                if (sh->slice_type == SLICE_B)
                {
                    xeve_bsw_write1(bs, sh->collocated_from_list_idx);
                    xeve_bsw_write1(bs, sh->collocated_mvp_source_list_idx);
                }
                xeve_bsw_write1(bs, sh->collocated_from_ref_idx);
            }
        }
    }
    xeve_bsw_write1(bs, sh->deblocking_filter_on);

    if(sh->deblocking_filter_on && sps->tool_addb)
    {
        xeve_bsw_write_se(bs, sh->sh_deblock_alpha_offset);
        xeve_bsw_write_se(bs, sh->sh_deblock_beta_offset);
    }

    xeve_bsw_write(bs, sh->qp, 6);
    xeve_bsw_write_se(bs, sh->qp_u_offset);
    xeve_bsw_write_se(bs, sh->qp_v_offset);

    if (!sh->single_tile_in_slice_flag)
    {
        for (int i = 0; i < num_tiles_in_slice - 1; ++i)
        {
            xeve_bsw_write(bs, sh->entry_point_offset_minus1[i], pps->tile_offset_lens_minus1 + 1);
        }
    }

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

int xevem_eco_split_mode(XEVE_BSW *bs, XEVE_CTX *c, XEVE_CORE *core, int cud, int cup, int cuw, int cuh, int lcu_s, int x, int y)
{
    XEVE_SBAC *sbac;
    int sps_cm_init_flag;
    int ret = XEVE_OK;
    s8 split_mode;
    int ctx = 0;

    int i, split_mode_sum;
    int split_allow[SPLIT_CHECK_NUM];

    if(cuw < 8 && cuh < 8)
    {
        return ret;
    }

    xeve_assert(xeve_check_luma(core->tree_cons));

    sbac = GET_SBAC_ENC(bs);
    sps_cm_init_flag = sbac->ctx.sps_cm_init_flag;

    if(sbac->is_bitcount)
    {
        xeve_get_split_mode(&split_mode, cud, cup, cuw, cuh, lcu_s, core->cu_data_temp[XEVE_LOG2(cuw) - 2][XEVE_LOG2(cuh) - 2].split_mode);
    }
    else
    {
        xeve_get_split_mode(&split_mode, cud, cup, cuw, cuh, lcu_s, c->map_cu_data[core->lcu_num].split_mode);
    }

    if (!c->sps.sps_btt_flag)
    {
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

    xeve_check_split_mode(c, split_allow, XEVE_LOG2(cuw), XEVE_LOG2(cuh), 0, 0, c->log2_max_cuwh
                        , x, y, c->w, c->h, c->sps.sps_btt_flag, core->tree_cons.mode_cons);

    split_mode_sum = 1;

    for(i = 1; i < SPLIT_CHECK_NUM; i++)
    {
        split_mode_sum += split_allow[i];
    }

    if (split_mode_sum == 1)
    {
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

    {
        int log2_cuw = XEVE_LOG2(cuw);
        int log2_cuh = XEVE_LOG2(cuh);

        if(sps_cm_init_flag == 1)
        {
            int i;
            u16 x_scu = x >> MIN_CU_LOG2;
            u16 y_scu = y >> MIN_CU_LOG2;
            u16 scuw = cuw >> MIN_CU_LOG2;
            u16 w_scu = c->w >> MIN_CU_LOG2;
            u8  smaller[3] = {0, 0, 0};
            u8  avail[3] = {0, 0, 0};
            int scun[3];
            int w[3], h[3];
            int scup = x_scu + y_scu * w_scu;

            avail[0] = y_scu > 0 && (c->map_tidx[scup] == c->map_tidx[scup - w_scu]);    //up
            if (x_scu > 0)
            {
                avail[1] = MCU_GET_COD(c->map_scu[scup - 1]) && (c->map_tidx[scup] == c->map_tidx[scup - 1]); //left
            }
            if (x_scu + scuw < w_scu)
            {
                avail[2] = MCU_GET_COD(c->map_scu[scup + scuw]) && (c->map_tidx[scup] == c->map_tidx[scup + scuw]); //right
            }
            scun[0] = scup - w_scu;
            scun[1] = scup - 1;
            scun[2] = scup + scuw;
            for(i = 0; i < 3; i++)
            {
                if(avail[i])
                {
                    w[i] = 1 << MCU_GET_LOGW(c->map_cu_mode[scun[i]]);
                    h[i] = 1 << MCU_GET_LOGH(c->map_cu_mode[scun[i]]);
                    if(i == 0)
                        smaller[i] = w[i] < cuw;
                    else
                        smaller[i] = h[i] < cuh;
                }
            }
            ctx = XEVE_MIN(smaller[0] + smaller[1] + smaller[2], 2);
            ctx = ctx + 3 * xevem_tbl_split_flag_ctx[log2_cuw - 2][log2_cuh - 2];
        }
        else
        {
            ctx = 0;
        }

        xeve_sbac_encode_bin(split_mode != NO_SPLIT, sbac, sbac->ctx.btt_split_flag + ctx, bs); /* btt_split_flag */
        if(split_mode != NO_SPLIT)
        {
            u8 HBT = split_allow[SPLIT_BI_HOR];
            u8 VBT = split_allow[SPLIT_BI_VER];
            u8 HTT = split_allow[SPLIT_TRI_HOR];
            u8 VTT = split_allow[SPLIT_TRI_VER];
            u8 sum = HBT + VBT + HTT + VTT;
            u8 ctx_dir = sps_cm_init_flag == 1 ? (log2_cuw - log2_cuh + 2) : 0;
            u8 ctx_typ = 0;
            u8 split_dir = (split_mode == SPLIT_BI_VER) || (split_mode == SPLIT_TRI_VER);
            u8 split_typ = (split_mode == SPLIT_TRI_HOR) || (split_mode == SPLIT_TRI_VER);

            if(sum == 4)
            {
                xeve_sbac_encode_bin(split_dir, sbac, sbac->ctx.btt_split_dir + ctx_dir, bs); /* btt_split_dir */
                xeve_sbac_encode_bin(split_typ, sbac, sbac->ctx.btt_split_type + ctx_typ, bs); /* btt_split_type */
            }
            else if(sum == 3)
            {
                xeve_sbac_encode_bin(split_dir, sbac, sbac->ctx.btt_split_dir + ctx_dir, bs); /* btt_split_dir */
                if(!HBT || !HTT)
                {
                    if(split_dir)
                        xeve_sbac_encode_bin(split_typ, sbac, sbac->ctx.btt_split_type + ctx_typ, bs); /* btt_split_type */
                    else
                        assert(split_typ == !HBT);
                }
                else// if(!VBT || !VTT)
                {
                    if(!split_dir)
                        xeve_sbac_encode_bin(split_typ, sbac, sbac->ctx.btt_split_type + ctx_typ, bs); /* btt_split_type */
                    else
                        assert(split_typ == !VBT);
                }
            }
            else if(sum == 2)
            {
                if((HBT && HTT) || (VBT && VTT))
                {
                    assert(split_dir == !HBT);
                    xeve_sbac_encode_bin(split_typ, sbac, sbac->ctx.btt_split_type + ctx_typ, bs); /* btt_split_type */
                }
                else
                {
                    xeve_sbac_encode_bin(split_dir, sbac, sbac->ctx.btt_split_dir + ctx_dir, bs); /* btt_split_dir */

                    if(!HTT && !VTT)
                        assert(split_typ == 0);
                    else if(HBT && VTT)
                        assert(split_typ == split_dir);
                    else if(VBT && HTT)
                        assert(split_typ == !split_dir);
                    else
                        assert(0);
                }
            }
            else // if(sum==1)
            {
                assert(split_dir == (VBT || VTT));
                assert(split_typ == (HTT || VTT));
            }
        }
    }
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

int xevem_eco_mode_constr(XEVE_BSW * bs, MODE_CONS mode_cons, int ctx)
{
    XEVE_SBAC * sbac = GET_SBAC_ENC(bs);
    u8 bit = mode_cons == eOnlyIntra;
    xeve_sbac_encode_bin(bit, sbac, sbac->ctx.mode_cons + ctx, bs);
    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("mode_constr ");
    XEVE_TRACE_INT(bit);
    XEVE_TRACE_STR("\n");

    return XEVE_OK;
}

static int xeve_eco_ats_intra_cu(XEVE_BSW *bs, u8 ats_intra_cu)
{
    XEVE_SBAC *sbac;
    sbac = GET_SBAC_ENC(bs);
    sbac_encode_bin_ep(ats_intra_cu, sbac, bs);

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("ats intra CU ");
    XEVE_TRACE_INT(ats_intra_cu);
    XEVE_TRACE_STR("\n");

    return XEVE_OK;
}

static int xeve_eco_ats_mode_h(XEVE_BSW *bs, u8 ats_mode_h)
{
    XEVE_SBAC *sbac;
    sbac = GET_SBAC_ENC(bs);
    xeve_sbac_encode_bin(ats_mode_h, sbac, sbac->ctx.ats_mode, bs);

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("ats intra tuH ");
    XEVE_TRACE_INT(ats_mode_h);
    XEVE_TRACE_STR("\n");

    return XEVE_OK;
}

static int xeve_eco_ats_mode_v(XEVE_BSW *bs, u8 ats_mode_v)
{
    XEVE_SBAC *sbac;
    sbac = GET_SBAC_ENC(bs);
    xeve_sbac_encode_bin(ats_mode_v, sbac, sbac->ctx.ats_mode, bs);

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("ats intra tuV ");
    XEVE_TRACE_INT(ats_mode_v);
    XEVE_TRACE_STR("\n");

    return XEVE_OK;
}

int xeve_eco_ats_inter_info(XEVE_BSW * bs, int log2_cuw, int log2_cuh, int ats_inter_info, u8 ats_inter_avail)
{
    u8 mode_vert = (ats_inter_avail >> 0) & 0x1;
    u8 mode_hori = (ats_inter_avail >> 1) & 0x1;
    u8 mode_vert_quad = (ats_inter_avail >> 2) & 0x1;
    u8 mode_hori_quad = (ats_inter_avail >> 3) & 0x1;
    u8 num_ats_inter_mode_avail = mode_vert + mode_hori + mode_vert_quad + mode_hori_quad;

    if (num_ats_inter_mode_avail == 0)
    {
        assert(ats_inter_info == 0);
        return XEVE_OK;
    }
    else
    {
        u8 ats_inter_idx = get_ats_inter_idx(ats_inter_info);
        u8 ats_inter_flag = ats_inter_idx != 0;
        u8 ats_inter_hor = is_ats_inter_horizontal(ats_inter_idx);
        u8 ats_inter_quad = is_ats_inter_quad_size(ats_inter_idx);
        u8 ats_inter_pos = get_ats_inter_pos(ats_inter_info);
        int size = 1 << (log2_cuw + log2_cuh);

        XEVE_SBAC    *sbac;
        XEVE_SBAC_CTX *sbac_ctx;
        sbac = GET_SBAC_ENC(bs);
        sbac_ctx = &sbac->ctx;

        u8 ctx_ats_inter = sbac->ctx.sps_cm_init_flag == 1 ? ((log2_cuw + log2_cuh >= 8) ? 0 : 1) : 0;
        u8 ctx_ats_inter_hor = sbac->ctx.sps_cm_init_flag == 1 ? ((log2_cuw == log2_cuh) ? 0 : (log2_cuw < log2_cuh ? 1 : 2)) : 0;

        if (ats_inter_idx == 0)
            assert(ats_inter_pos == 0);

        xeve_sbac_encode_bin(ats_inter_flag, sbac, sbac_ctx->ats_cu_inter_flag + ctx_ats_inter, bs);
        XEVE_TRACE_STR("ats_inter_flag ");
        XEVE_TRACE_INT(ats_inter_flag);
        XEVE_TRACE_STR("\n");

        if (ats_inter_flag)
        {
            if ((mode_vert_quad || mode_hori_quad) && (mode_vert || mode_hori))
            {
                xeve_sbac_encode_bin(ats_inter_quad, sbac, sbac_ctx->ats_cu_inter_quad_flag, bs);
                XEVE_TRACE_STR("ats_inter_quad ");
                XEVE_TRACE_INT(ats_inter_quad);
                XEVE_TRACE_STR("\n");
            }
            else
            {
                assert(ats_inter_quad == 0);
            }

            if ((ats_inter_quad && mode_vert_quad && mode_hori_quad) || (!ats_inter_quad && mode_vert && mode_hori))
            {
                xeve_sbac_encode_bin(ats_inter_hor, sbac, sbac_ctx->ats_cu_inter_hor_flag + ctx_ats_inter_hor, bs);
                XEVE_TRACE_STR("ats_inter_hor ");
                XEVE_TRACE_INT(ats_inter_hor);
                XEVE_TRACE_STR("\n");
            }
            else
            {
                assert(ats_inter_hor == ((ats_inter_quad && mode_hori_quad) || (!ats_inter_quad && mode_hori)));
            }

            xeve_sbac_encode_bin(ats_inter_pos, sbac, sbac_ctx->ats_cu_inter_pos_flag, bs);
            XEVE_TRACE_STR("ats_inter_pos ");
            XEVE_TRACE_INT(ats_inter_pos);
            XEVE_TRACE_STR("\n");
        }

        return XEVE_OK;
    }
}

static void code_coef_remain_exgolomb(XEVE_BSW *bs, int symbol, int rparam)
{
    XEVE_SBAC    * sbac = GET_SBAC_ENC(bs);
    int code_number = symbol;
    int length;
    if (code_number < (xeve_go_rice_range[rparam] << rparam))
    {
        length = code_number >> rparam;
        sbac_encode_bins_ep((1 << (length + 1)) - 2, length + 1, sbac, bs);
        sbac_encode_bins_ep((code_number % (1 << rparam)), rparam, sbac, bs);
    }
    else
    {
        length = rparam;
        code_number = code_number - (xeve_go_rice_range[rparam] << rparam);
        while (code_number >= (1 << length))
        {
            code_number -= (1 << (length++));
        }
        sbac_encode_bins_ep((1 << (xeve_go_rice_range[rparam] + length + 1 - rparam)) - 2, xeve_go_rice_range[rparam] + length + 1 - rparam, sbac, bs);
        sbac_encode_bins_ep(code_number, length, sbac, bs);
    }
}


static void code_positionLastXY(XEVE_BSW *bs, int last_x, int last_y, int width, int height, int ch_type)
{
    XEVE_SBAC *sbac = GET_SBAC_ENC(bs);
    SBAC_CTX_MODEL* cm_x = sbac->ctx.last_sig_coeff_x_prefix + (ch_type == Y_C ? 0 : (sbac->ctx.sps_cm_init_flag == 1 ? NUM_CTX_LAST_SIG_COEFF_LUMA : 11));
    SBAC_CTX_MODEL* cm_y = sbac->ctx.last_sig_coeff_y_prefix + (ch_type == Y_C ? 0 : (sbac->ctx.sps_cm_init_flag == 1 ? NUM_CTX_LAST_SIG_COEFF_LUMA : 11));

    int bin;
    int group_idx_x;
    int group_idx_y;
    int blk_offset_x, blk_offset_y, shift_x, shift_y;
    int i, cnt;

    group_idx_x = xeve_group_idx[last_x];
    group_idx_y = xeve_group_idx[last_y];
    if (sbac->ctx.sps_cm_init_flag == 1)
    {
        xeve_get_ctx_last_pos_xy_para(ch_type, width, height, &blk_offset_x, &blk_offset_y, &shift_x, &shift_y);
    }
    else
    {
        blk_offset_x = 0;
        blk_offset_y = 0;
        shift_x = 0;
        shift_y = 0;
    }
    //------------------

    // last_sig_coeff_x_prefix
    for (bin = 0; bin < group_idx_x; bin++)
    {
        xeve_sbac_encode_bin(1, sbac, &cm_x[blk_offset_x + (bin >> shift_x)], bs);
    }
    if (group_idx_x < xeve_group_idx[width - 1])
    {
        xeve_sbac_encode_bin(0, sbac, &cm_x[blk_offset_x + (bin >> shift_x)], bs);
    }

    // last_sig_coeff_y_prefix
    for (bin = 0; bin < group_idx_y; bin++)
    {
        xeve_sbac_encode_bin(1, sbac, &cm_y[blk_offset_y + (bin >> shift_y)], bs);
    }
    if (group_idx_y < xeve_group_idx[height - 1])
    {
        xeve_sbac_encode_bin(0, sbac, &cm_y[blk_offset_y + (bin >> shift_y)], bs);
    }

    // last_sig_coeff_x_suffix
    if (group_idx_x > 3)
    {
        cnt = (group_idx_x - 2) >> 1;
        last_x = last_x - xeve_min_in_group[group_idx_x];
        for (i = cnt - 1; i >= 0; i--)
        {
            sbac_encode_bin_ep((last_x >> i) & 1, sbac, bs);
        }
    }
    // last_sig_coeff_y_suffix
    if (group_idx_y > 3)
    {
        cnt = (group_idx_y - 2) >> 1;
        last_y = last_y - xeve_min_in_group[group_idx_y];
        for (i = cnt - 1; i >= 0; i--)
        {
            sbac_encode_bin_ep((last_y >> i) & 1, sbac, bs);
        }
    }
}

static void xeve_eco_adcc(XEVE_CTX * ctx, XEVE_BSW *bs, s16 *coef, int log2_w, int log2_h, int num_sig, int ch_type)
{
    int width = 1 << log2_w;
    int height = 1 << log2_h;
    int offset0;
    XEVE_SBAC    * sbac = GET_SBAC_ENC(bs);
    SBAC_CTX_MODEL* cm_sig_coeff;
    SBAC_CTX_MODEL* cm_gtx;
    int scan_type = COEF_SCAN_ZIGZAG;
    int log2_block_size = XEVE_MIN(log2_w, log2_h);
    const u16 *scan;
    int scan_pos_last = -1;
    int last_x = 0, last_y = 0;
    int ipos;
    int last_scan_set;
    int rice_param;
    int sub_set;
    int ctx_sig_coeff = 0;
    int cg_log2_size = LOG2_CG_SIZE;
    int is_last_x = 0;
    int is_last_y = 0;
    int is_last_nz = 0;
    int pos_last = 0;
    int ctx_gtA = 0;
    int ctx_gtB = 0;
    int escape_data_present_ingroup = 0;
    int cnt_nz = 0;
    int blkpos, sx, sy;
    int sig_coeff_flag;
    int max_num_coef = width * height;
    scan = xeve_tbl_scan[log2_w - 1][log2_h - 1];

    int last_pos_in_scan = 0;
    int numNonZeroCoefs = 0;

    last_pos_in_scan = -1;
    int last_pos_in_raster_from_scan = -1;

    for (int blk_pos = 0; blk_pos < max_num_coef; blk_pos++)
    {
        int scan_pos = scan[blk_pos];

        if (coef[scan_pos] != 0)
        {
            last_y = scan_pos >> log2_w;
            last_x = scan_pos - (last_y << log2_w);

            numNonZeroCoefs++;
            last_pos_in_scan = blk_pos;
            last_pos_in_raster_from_scan = scan_pos;
        }
    }
    code_positionLastXY(bs, last_x, last_y, width, height, ch_type);

    //===== code significance flag =====
    last_scan_set = last_pos_in_scan >> cg_log2_size;
    if (sbac->ctx.sps_cm_init_flag == 1)
    {
        offset0 = log2_block_size <= 2 ? 0 : NUM_CTX_SIG_COEFF_LUMA_TU << (XEVE_MIN(1, (log2_block_size - 3)));
        cm_sig_coeff = (ch_type == Y_C) ? sbac->ctx.sig_coeff_flag + offset0 : sbac->ctx.sig_coeff_flag + NUM_CTX_SIG_COEFF_LUMA;
        cm_gtx = (ch_type == Y_C) ? sbac->ctx.coeff_abs_level_greaterAB_flag : sbac->ctx.coeff_abs_level_greaterAB_flag + NUM_CTX_GTX_LUMA;
    }
    else
    {
        cm_sig_coeff = (ch_type == Y_C) ? sbac->ctx.sig_coeff_flag : sbac->ctx.sig_coeff_flag + 1;
        cm_gtx = (ch_type == Y_C) ? sbac->ctx.coeff_abs_level_greaterAB_flag : sbac->ctx.coeff_abs_level_greaterAB_flag + 1;
    }
    rice_param = 0;
    ipos = last_pos_in_scan;

    for (sub_set = last_scan_set; sub_set >= 0; sub_set--)
    {
        int num_nz = 0;
        int sub_pos = sub_set << cg_log2_size;
        int coef_signs_group = 0;
        int abs_coef[1 << LOG2_CG_SIZE];  // array size of CG
        int pos[1 << LOG2_CG_SIZE];  // array size of CG
        int last_nz_pos_in_cg = -1;
        int first_nz_pos_in_cg = 1 << cg_log2_size;

        {
            for (; ipos >= sub_pos; ipos--)
            {
                blkpos = scan[ipos];
                sy = blkpos >> log2_w;
                sx = blkpos - (sy << log2_w);

                // sigmap
                sig_coeff_flag = (coef[blkpos] != 0 ? 1 : 0);
                if (ipos == last_pos_in_scan)
                {
                    ctx_sig_coeff = 0;
                }
                else
                {
                    ctx_sig_coeff = sbac->ctx.sps_cm_init_flag == 1 ? xeve_get_ctx_sig_coeff_inc(coef, blkpos, width, height, ch_type) : 0;
                }

                if (!(ipos == last_pos_in_scan))
                {
                    xeve_sbac_encode_bin((u32)sig_coeff_flag, sbac, &cm_sig_coeff[ctx_sig_coeff], bs);
                }

                if (sig_coeff_flag)
                {
                    pos[num_nz] = blkpos;
                    abs_coef[num_nz] = (int)(XEVE_ABS(coef[blkpos]));
                    coef_signs_group = 2 * coef_signs_group + (coef[blkpos] < 0 ? 1 : 0);
                    num_nz++;

                    if (last_nz_pos_in_cg == -1)
                    {
                        last_nz_pos_in_cg = ipos;
                    }
                    first_nz_pos_in_cg = ipos;
                    if (is_last_nz == 0)
                    {
                        pos_last = blkpos;
                        is_last_nz = 1;
                    }
                }
            }

            if (num_nz > 0)
            {
                int numC1Flag = XEVE_MIN(num_nz, CAFLAG_NUMBER);

                int firstC2FlagIdx = -1;
                escape_data_present_ingroup = 0;

                for (int idx = 0; idx < numC1Flag; idx++)  //
                {
                    u32 coeff_abs_level_greaterA_flag = abs_coef[idx] > 1 ? 1 : 0;
                    if (pos[idx] != pos_last)
                    {
                        ctx_gtA = sbac->ctx.sps_cm_init_flag == 1 ? xeve_get_ctx_gtA_inc(coef, pos[idx], width, height, ch_type) : 0;
                    }
                    xeve_sbac_encode_bin(coeff_abs_level_greaterA_flag, sbac, &cm_gtx[ctx_gtA], bs);
                    if (coeff_abs_level_greaterA_flag)
                    {
                        if (firstC2FlagIdx == -1)
                        {
                            firstC2FlagIdx = idx;
                        }
                        else
                        {
                            escape_data_present_ingroup = TRUE;
                        }
                    }
                }
                if (firstC2FlagIdx != -1)
                {
                    u32 coeff_abs_level_greaterB_flag = abs_coef[firstC2FlagIdx] > 2 ? 1 : 0;
                    if (pos[firstC2FlagIdx] != pos_last)
                    {
                        ctx_gtB = sbac->ctx.sps_cm_init_flag == 1 ? xeve_get_ctx_gtB_inc(coef, pos[firstC2FlagIdx], width, height, ch_type) : 0;
                    }
                    xeve_sbac_encode_bin(coeff_abs_level_greaterB_flag, sbac, &cm_gtx[ctx_gtB], bs);

                    if (coeff_abs_level_greaterB_flag != 0)
                    {
                        escape_data_present_ingroup = 1;
                    }
                }
                escape_data_present_ingroup = escape_data_present_ingroup || (num_nz > CAFLAG_NUMBER);

                int iFirstCoeff2 = 1;
                if (escape_data_present_ingroup)
                {
                    for (int idx = 0; idx < num_nz; idx++)
                    {
                        int base_level = (idx < CAFLAG_NUMBER) ? (2 + iFirstCoeff2) : 1;
                        if (abs_coef[idx] >= base_level)
                        {
                            int coeff_abs_level_remaining = abs_coef[idx] - base_level;
                            rice_param = get_rice_para(coef, pos[idx], width, height, base_level);
                            code_coef_remain_exgolomb(bs, coeff_abs_level_remaining, rice_param);
                        }
                        if (abs_coef[idx] >= 2)
                        {
                            iFirstCoeff2 = 0;
                        }
                    }
                }
                sbac_encode_bins_ep(coef_signs_group, num_nz, sbac, bs);
            }
        }
    }
}

static void xeve_eco_xcoef(XEVE_CTX * ctx, XEVE_BSW *bs, s16 *coef, int log2_w, int log2_h, int num_sig, int ch_type, int tool_adcc)
{
    if (tool_adcc)
    {
        xeve_eco_adcc(ctx, bs, coef, log2_w, log2_h, num_sig, (ch_type == Y_C ? 0 : 1));
    }
    else
    {
        xeve_eco_run_length_cc(ctx, bs, coef, log2_w, log2_h, num_sig, (ch_type == Y_C ? 0 : 1));
    }

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

static int xeve_eco_coefficient(XEVE_BSW * bs, s16 coef[N_C][MAX_CU_DIM], int log2_cuw, int log2_cuh, u8 pred_mode, int nnz_sub[N_C][MAX_SUB_TB_NUM], int b_no_cbf, int run_stats
                  , int tool_ats, u8 ats_intra_cu, u8 ats_mode, u8 ats_inter_info, XEVE_CTX * ctx, XEVE_CORE * core, int enc_dqp, u8 cur_qp, TREE_CONS tree_cons)
{
    run_stats = xeve_get_run(run_stats, tree_cons);
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
    int w_shift = (XEVE_GET_CHROMA_W_SHIFT(ctx->sps.chroma_format_idc));
    int h_shift = (XEVE_GET_CHROMA_H_SHIFT(ctx->sps.chroma_format_idc));

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

    u8 ats_inter_avail = check_ats_inter_info_coded(1 << log2_cuw, 1 << log2_cuh, pred_mode, tool_ats);
    if( ats_inter_avail )
    {
        get_tu_size( ats_inter_info, log2_cuw, log2_cuh, &log2_w_sub, &log2_h_sub );
        sub_stride = (1 << log2_w_sub);
    }

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
                    if((((!(ctx->sps.dquant_flag) || (core->cu_qp_delta_code == 1 && !core->cu_qp_delta_is_coded)) && (cbf_for_dqp))
                        || (core->cu_qp_delta_code == 2 && !core->cu_qp_delta_is_coded)))
                    {
                        xeve_eco_dqp(bs, ctx->tile[core->tile_idx].qp_prev_eco[core->thread_cnt], cur_qp);
                        core->cu_qp_delta_is_coded = 1;
                        ctx->tile[core->tile_idx].qp_prev_eco[core->thread_cnt] = cur_qp;
                    }
                }
            }

            if (tool_ats && (!!nnz_sub[Y_C][(j << 1) | i]) && (log2_cuw <= 5 && log2_cuh <= 5) && is_intra && run[Y_C])
            {
                xeve_eco_ats_intra_cu(bs, ats_intra_cu);

                if (ats_intra_cu)
                {
                    xeve_eco_ats_mode_h(bs, (ats_mode >> 1));
                    xeve_eco_ats_mode_v(bs, (ats_mode & 1));
                }
            }

            if (pred_mode != MODE_INTRA && pred_mode != MODE_IBC && run[Y_C] && run[U_C] && run[V_C])
            {
                if (ats_inter_avail && cbf_all)
                {
                    assert(loop_w == 1 && loop_h == 1);
                    xeve_eco_ats_inter_info(bs, log2_cuw, log2_cuh, ats_inter_info, ats_inter_avail);
                }
                else
                {
                    assert(ats_inter_info == 0);
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
                    xeve_eco_xcoef(ctx, bs, coef_temp[c], log2_w_sub - (!!c), log2_h_sub - (!!c), nnz_sub[c][(j << 1) | i], c, ctx->sps.tool_adcc);
                    else
                        xeve_eco_xcoef(ctx, bs, coef_temp[c], log2_w_sub - w_shift, log2_h_sub - h_shift, nnz_sub[c][(j << 1) | i], c, ctx->sps.tool_adcc);

                    if (is_sub)
                    {
                        if(c == 0)
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

int xevem_eco_coef_main(XEVE_CTX * ctx, XEVE_CORE * core, XEVE_BSW * bs, s16 coef[N_C][MAX_CU_DIM], u8 pred_mode, int enc_dqp, int b_no_cbf, int run_stats)
{
    XEVEM_CORE *mcore = (XEVEM_CORE*)core;
    return xeve_eco_coefficient(bs, coef, core->log2_cuw, core->log2_cuh, pred_mode, core->nnz_sub, b_no_cbf, run_stats
                              , ctx->sps.tool_ats, mcore->ats_intra_cu, mcore->ats_mode, mcore->ats_inter_info, ctx, core, enc_dqp, core->qp, core->tree_cons);
}

void xevem_intra_mode_write_trunc_binary(int symbol, int max_symbol, XEVE_SBAC *sbac, XEVE_BSW *bs)
{
    int threshold = 4; /* we use 5 bits to signal the default mode */
    int val = 1 << threshold;
    int b;

    if(val > max_symbol)
    {
        xeve_trace("val =%d max_symbol= %d", val, max_symbol);
    }
    assert(val <= max_symbol);
    assert((val << 1) > max_symbol);
    assert(symbol < max_symbol);

    b = max_symbol - val;
    assert(b < val);

    if(symbol < val - b)
    {
        sbac_encode_bins_ep(symbol, threshold, sbac, bs);
    }
    else
    {
        symbol += val - b;
        assert(symbol < (val << 1));
        assert((symbol >> 1) >= val - b);
        sbac_encode_bins_ep(symbol, threshold + 1, sbac, bs);
    }
}

int xevem_eco_intra_dir(XEVE_BSW *bs, u8 ipm, u8 mpm[2], u8 mpm_ext[8], u8 pims[IPD_CNT])
{
    XEVE_SBAC *sbac;

    int t0;
    sbac = GET_SBAC_ENC(bs);
    if(ipm == mpm[0] || ipm == mpm[1])
    {
        xeve_sbac_encode_bin(1, sbac, sbac->ctx.intra_luma_pred_mpm_flag, bs);
        t0 = ipm == mpm[0] ? 0 : 1;
        xeve_sbac_encode_bin(t0, sbac, sbac->ctx.intra_luma_pred_mpm_idx, bs);
    }
    else
    {
        int i;
        int pms_cnt = -1;
        int flag = 0;
        int check = 8;

        xeve_sbac_encode_bin(0, sbac, sbac->ctx.intra_luma_pred_mpm_flag, bs);

        for(i = 0; i < check; i++)
        {
            if(ipm == mpm_ext[i])
            {
                flag = i + 1;
                break;
            }
        }

        if(flag)
        {
            sbac_encode_bin_ep(1, sbac, bs);
            flag = flag - 1;
            {
                sbac_encode_bin_ep((flag >> 2) & 1, sbac, bs);
                sbac_encode_bin_ep((flag >> 1) & 1, sbac, bs);
                sbac_encode_bin_ep(flag & 1, sbac, bs);
            }
        }
        else
        {
            sbac_encode_bin_ep(0, sbac, bs);

            for(pms_cnt = 0; pms_cnt < IPD_CNT; pms_cnt++)
            {
                if(ipm == pims[pms_cnt])
                {
                    break;
                }
            }
            pms_cnt -= check + 2;
            xevem_intra_mode_write_trunc_binary(pms_cnt, IPD_CNT - (check + 2), sbac, bs);
        }
    }

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("ipm Y ");
    XEVE_TRACE_INT(ipm);
    XEVE_TRACE_STR("\n");

    return XEVE_OK;
}

int xevem_eco_intra_dir_c(XEVE_BSW *bs, u8 ipm, u8 ipm_l)
{
    XEVE_SBAC *sbac;
    u8         chk_bypass;
    int        remain;
#if TRACE_ADDITIONAL_FLAGS
    u8 ipm_l_saved = ipm_l;
#endif
    sbac = GET_SBAC_ENC(bs);

    XEVE_IPRED_CONV_L2C_CHK(ipm_l, chk_bypass);

    if(ipm == 0)
    {
        xeve_sbac_encode_bin(1, sbac, sbac->ctx.intra_chroma_pred_mode, bs);
    }
    else
    {
        xeve_sbac_encode_bin(0, sbac, sbac->ctx.intra_chroma_pred_mode, bs);
        remain = (chk_bypass && ipm > ipm_l) ? ipm - 2 : ipm - 1;
        sbac_write_unary_sym_ep(remain, sbac, bs, IPD_CHROMA_CNT - 1);
    }

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("ipm UV ");
    XEVE_TRACE_INT(ipm);
#if TRACE_ADDITIONAL_FLAGS
    XEVE_TRACE_STR("ipm L ");
    XEVE_TRACE_INT(ipm_l_saved);
#endif
    XEVE_TRACE_STR("\n");

    return XEVE_OK;
}

void xevem_eco_ibc_flag(XEVE_BSW * bs, int flag, int ctx)
{
    XEVE_SBAC *sbac;
    sbac = GET_SBAC_ENC(bs);
    xeve_sbac_encode_bin(flag, sbac, sbac->ctx.ibc_flag + ctx, bs);
#if TRACE_ADDITIONAL_FLAGS
    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("ibc pred mode ");
    XEVE_TRACE_INT(!!flag);
    XEVE_TRACE_STR("ctx ");
    XEVE_TRACE_INT(ctx);
    XEVE_TRACE_STR("\n");
#endif
}

void xevem_eco_mmvd_flag(XEVE_BSW * bs, int flag)
{
    XEVE_SBAC *sbac;
    sbac = GET_SBAC_ENC(bs);

    xeve_sbac_encode_bin(flag, sbac, sbac->ctx.mmvd_flag, bs); /* mmvd_flag */

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("mmvd_flag ");
    XEVE_TRACE_INT(flag);
    XEVE_TRACE_STR("\n");
}

int xevem_eco_mmvd_info(XEVE_BSW *bs, int mvp_idx, int type)
{
    XEVE_SBAC *sbac = GET_SBAC_ENC(bs);
    int var0, var1, var2;
    int dev0 = 0;
    int var;
    int t_idx = mvp_idx;

    if(type == 1)
    {
        if(t_idx >= (MMVD_MAX_REFINE_NUM*MMVD_BASE_MV_NUM))
        {
            t_idx = t_idx - (MMVD_MAX_REFINE_NUM*MMVD_BASE_MV_NUM);
            dev0 = t_idx / (MMVD_MAX_REFINE_NUM*MMVD_BASE_MV_NUM);
            t_idx = t_idx - dev0 * (MMVD_MAX_REFINE_NUM*MMVD_BASE_MV_NUM);
            var = 1;
        }
        else
        {
            var = 0;
        }

        /* mmvd_group_idx */
        xeve_sbac_encode_bin(var, sbac, sbac->ctx.mmvd_group_idx + 0, bs);
        if(var == 1)
        {
            xeve_sbac_encode_bin(dev0, sbac, sbac->ctx.mmvd_group_idx + 1, bs);
        }
    }
    else
    {
        var = 0;
        dev0 = 0;
    }

    var0 = t_idx / MMVD_MAX_REFINE_NUM;
    var1 = (t_idx - (var0 * MMVD_MAX_REFINE_NUM)) / 4;
    var2 = t_idx - (var0 * MMVD_MAX_REFINE_NUM) - var1 * 4;

    sbac_write_truncate_unary_sym(var0, NUM_CTX_MMVD_MERGE_IDX, MMVD_BASE_MV_NUM, sbac, sbac->ctx.mmvd_merge_idx, bs); /* mmvd_merge_idx */
    sbac_write_truncate_unary_sym(var1, NUM_CTX_MMVD_DIST_IDX, MMVD_DIST_NUM, sbac, sbac->ctx.mmvd_distance_idx, bs); /* mmvd_distance_idx */

    /* mmvd_direction_idx */
    if(var2 == 0)
    {
        xeve_sbac_encode_bin(0, sbac, sbac->ctx.mmvd_direction_idx, bs);
        xeve_sbac_encode_bin(0, sbac, sbac->ctx.mmvd_direction_idx + 1, bs);
    }
    else if(var2 == 1)
    {
        xeve_sbac_encode_bin(0, sbac, sbac->ctx.mmvd_direction_idx, bs);
        xeve_sbac_encode_bin(1, sbac, sbac->ctx.mmvd_direction_idx + 1, bs);
    }
    else if(var2 == 2)
    {
        xeve_sbac_encode_bin(1, sbac, sbac->ctx.mmvd_direction_idx, bs);
        xeve_sbac_encode_bin(0, sbac, sbac->ctx.mmvd_direction_idx + 1, bs);
    }
    else if(var2 == 3)
    {
        xeve_sbac_encode_bin(1, sbac, sbac->ctx.mmvd_direction_idx, bs);
        xeve_sbac_encode_bin(1, sbac, sbac->ctx.mmvd_direction_idx + 1, bs);
    }

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("mmvd_idx ");
    XEVE_TRACE_INT(mvp_idx);
    XEVE_TRACE_STR("\n");

    return XEVE_OK;
}

int xevem_eco_affine_mvp_idx( XEVE_BSW *bs, int mvp_idx )
{
#if AFF_MAX_NUM_MVP > 1
    XEVE_SBAC *sbac = GET_SBAC_ENC( bs );
    XEVE_SBAC_CTX *sbac_ctx = &sbac->ctx;

    sbac_write_truncate_unary_sym( mvp_idx, NUM_CTX_AFFINE_MVP_IDX, AFF_MAX_NUM_MVP, sbac, sbac_ctx->affine_mvp_idx, bs );

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR( "affine mvp idx " );
    XEVE_TRACE_INT( mvp_idx );
    XEVE_TRACE_STR( "\n" );
#endif

    return XEVE_OK;
}

void xevem_eco_affine_flag(XEVE_BSW * bs, int flag, int ctx)
{
    XEVE_SBAC *sbac;
    sbac = GET_SBAC_ENC(bs);
    xeve_sbac_encode_bin(flag, sbac, sbac->ctx.affine_flag + ctx, bs);

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("affine flag ");
    XEVE_TRACE_INT(flag);
    XEVE_TRACE_STR("\n");
}

void xevem_eco_affine_mode(XEVE_BSW * bs, int flag)
{
    XEVE_SBAC *sbac;
    sbac = GET_SBAC_ENC(bs);
    xeve_sbac_encode_bin(flag, sbac, sbac->ctx.affine_mode, bs);
#if TRACE_ADDITIONAL_FLAGS
    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("affine mode ");
    XEVE_TRACE_INT(flag);
    XEVE_TRACE_STR("\n");
#endif
}

int xevem_eco_affine_mrg_idx(XEVE_BSW * bs, s16 affine_mrg)
{
    XEVE_SBAC * sbac = GET_SBAC_ENC(bs);
    XEVE_SBAC_CTX * sbac_ctx = &sbac->ctx;

    sbac_write_truncate_unary_sym(affine_mrg, AFF_MAX_CAND, AFF_MAX_CAND, sbac, sbac_ctx->affine_mrg, bs);

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("merge affine idx ");
    XEVE_TRACE_INT(affine_mrg);
    XEVE_TRACE_STR("\n");

    return XEVE_OK;
}

void xevem_eco_affine_mvd_flag(XEVE_BSW * bs, int flag, int refi)
{
    XEVE_SBAC *sbac;
    sbac = GET_SBAC_ENC(bs);
    xeve_sbac_encode_bin(flag, sbac, &sbac->ctx.affine_mvd_flag[refi], bs);
}

int xevem_eco_suco_flag(XEVE_BSW *bs, XEVE_CTX *c, XEVE_CORE *core, int cud, int cup, int cuw, int cuh, int lcu_s, s8 split_mode, int boundary, u8 log2_max_cuwh)
{
    XEVE_SBAC *sbac;
    int ret = XEVE_OK;
    s8 suco_flag;
    int ctx;
    u8 allow_suco = c->sps.sps_suco_flag ? xeve_check_suco_cond(cuw, cuh, split_mode, boundary, log2_max_cuwh, c->log2_min_cuwh
                                                              , c->sps.log2_diff_ctu_size_max_suco_cb_size, c->sps.log2_diff_max_suco_min_suco_cb_size) : 0;

    if(!allow_suco)
    {
        return ret;
    }

    sbac = GET_SBAC_ENC(bs);

    if(sbac->is_bitcount)
        xeve_get_suco_flag(&suco_flag, cud, cup, cuw, cuh, lcu_s, core->cu_data_temp[XEVE_LOG2(cuw) - 2][XEVE_LOG2(cuh) - 2].suco_flag);
    else
        xeve_get_suco_flag(&suco_flag, cud, cup, cuw, cuh, lcu_s, c->map_cu_data[core->lcu_num].suco_flag);

    if(sbac->ctx.sps_cm_init_flag == 1)
    {
        ctx = XEVE_LOG2(XEVE_MAX(cuw, cuh)) - 2;
        ctx = (cuw == cuh) ? ctx * 2 : ctx * 2 + 1;
    }
    else
    {
        ctx = 0;
    }
    xeve_sbac_encode_bin(suco_flag, sbac, sbac->ctx.suco_flag + ctx, bs);

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("suco flag ");
    XEVE_TRACE_INT(suco_flag);
    XEVE_TRACE_STR("\n");

    return ret;
}

int xevem_eco_mvr_idx(XEVE_BSW * bs, u8 mvr_idx)
{
    XEVE_SBAC * sbac = GET_SBAC_ENC(bs);
    XEVE_SBAC_CTX * sbac_ctx = &sbac->ctx;

    sbac_write_truncate_unary_sym(mvr_idx, MAX_NUM_MVR, MAX_NUM_MVR, sbac, sbac_ctx->mvr_idx, bs);
#if TRACE_ADDITIONAL_FLAGS
    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("mvr_idx ");
    XEVE_TRACE_INT(mvr_idx);
    XEVE_TRACE_STR("\n");
#endif

    return XEVE_OK;
}

int xevem_eco_merge_idx(XEVE_BSW *bs, int merge_idx)
{
    XEVE_SBAC *sbac = GET_SBAC_ENC(bs);
    XEVE_SBAC_CTX *sbac_ctx = &sbac->ctx;

    sbac_write_truncate_unary_sym(merge_idx, NUM_CTX_MERGE_IDX, MAX_NUM_MVP, sbac, sbac_ctx->merge_idx, bs);

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("merge idx ");
    XEVE_TRACE_INT(merge_idx);
    XEVE_TRACE_STR("\n");

    return XEVE_OK;
}

void xevem_eco_merge_mode_flag(XEVE_BSW *bs, int merge_mode_flag)
{
    XEVE_SBAC *sbac;
    sbac = GET_SBAC_ENC(bs);
    xeve_sbac_encode_bin(merge_mode_flag, sbac, sbac->ctx.merge_mode_flag, bs);

    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("merge_mode_flag ");
    XEVE_TRACE_INT(merge_mode_flag ? PRED_DIR : 0);
    XEVE_TRACE_STR("\n");
}

int xevem_eco_bi_idx(XEVE_BSW *bs, u8 bi_idx)
{
    XEVE_SBAC * sbac = GET_SBAC_ENC(bs);
    XEVE_SBAC_CTX * sbac_ctx = &sbac->ctx;

    if(bi_idx == 0)
    {
        xeve_sbac_encode_bin(1, sbac, sbac_ctx->bi_idx, bs);
    }
    else
    {
        xeve_sbac_encode_bin(0, sbac, sbac_ctx->bi_idx, bs);
        if(bi_idx == 1)
        {
            xeve_sbac_encode_bin(1, sbac, sbac_ctx->bi_idx + 1, bs);
        }
        else
        {
            xeve_sbac_encode_bin(0, sbac, sbac_ctx->bi_idx + 1, bs);
        }
    }
#if TRACE_ADDITIONAL_FLAGS
    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("bi_idx ");
    XEVE_TRACE_INT(bi_idx);
    XEVE_TRACE_STR("\n");
#endif
    return XEVE_OK;
}

static void imgb_free1(XEVE_IMGB * imgb)
{
    int i;
    for (i = 0; i < XEVE_IMGB_MAX_PLANE; i++)
    {
        if (imgb->baddr[i]) free(imgb->baddr[i]);
    }
    free(imgb);
}

XEVE_IMGB * imgb_alloc1(int w, int h, int cs)
{
    int i;
    XEVE_IMGB * imgb;

    imgb = (XEVE_IMGB *)xeve_malloc(sizeof(XEVE_IMGB));
    xeve_assert_g(imgb != NULL, ERR);
    xeve_mset(imgb, 0, sizeof(XEVE_IMGB));

    if (cs == XEVE_CS_YCBCR420)
    {
        for (i = 0; i < 3; i++)
        {
            imgb->w[i] = imgb->aw[i] = imgb->s[i] = w;
            imgb->h[i] = imgb->ah[i] = imgb->e[i] = h;
            imgb->bsize[i] = imgb->s[i] * imgb->e[i];

            imgb->a[i] = imgb->baddr[i] = xeve_malloc(imgb->bsize[i]);
            xeve_assert_g(imgb->a[i] != NULL, ERR);
            if (i == 0)
            {
                w = (w + 1) >> 1; h = (h + 1) >> 1;
            }
        }
        imgb->np = 3;
    }
    else if (cs == XEVE_CS_YCBCR420_10LE)
    {
        for (i = 0; i < 3; i++)
        {
            imgb->w[i] = imgb->aw[i] = w;
            imgb->s[i] = w * sizeof(short);
            imgb->h[i] = imgb->ah[i] = imgb->e[i] = h;
            imgb->bsize[i] = imgb->s[i] * imgb->e[i];

            imgb->a[i] = imgb->baddr[i] = xeve_malloc(imgb->bsize[i]);
            xeve_assert_g(imgb->a[i] != NULL, ERR);
            if (i == 0)
            {
                w = (w + 1) >> 1; h = (h + 1) >> 1;
            }
        }
        imgb->np = 3;
    }
    else if (cs == XEVE_CS_YCBCR444_10LE)
    {
        for (i = 0; i < 3; i++)
        {
            imgb->w[i] = imgb->aw[i] = w;
            imgb->s[i] = w * sizeof(float);
            imgb->h[i] = imgb->ah[i] = imgb->e[i] = h;
            imgb->bsize[i] = imgb->s[i] * imgb->e[i];

            imgb->a[i] = imgb->baddr[i] = xeve_malloc(imgb->bsize[i]);
            xeve_assert_g(imgb->a[i] != NULL, ERR);
        }
        imgb->np = 3;
    }
    else
    {/* "unsupported color space\n"*/
        xeve_assert_g(0, ERR);
    }
    imgb->cs = cs;
    return imgb;

ERR:
    if(imgb) xeve_imgb_garbage_free(imgb);
    return NULL;
}

int xeve_eco_udata_hdr(XEVE_CTX * ctx, XEVE_BSW * bs, u8 pic_sign[N_C][16])
{
    int ret;
    XEVE_IMGB *imgb_hdr_md5 = NULL;
    imgb_hdr_md5 = imgb_alloc1(PIC_CURR(ctx)->imgb->w[0], PIC_CURR(ctx)->imgb->h[0],
        XEVE_CS_YCBCR420_10LE);

    xeve_imgb_cpy(imgb_hdr_md5, PIC_CURR(ctx)->imgb);  // store copy of the reconstructed picture in DPB

    SIG_PARAM_DRA *pps_dra_params = (SIG_PARAM_DRA *)((XEVEM_CTX*)ctx)->dra_array;
    xeve_apply_dra_from_array(ctx, imgb_hdr_md5, imgb_hdr_md5, &(pps_dra_params[0]), ctx->aps_gen_array[1].aps_id, TRUE);

    /* should be aligned before adding user data */
    xeve_assert_rv(XEVE_BSW_IS_BYTE_ALIGN(bs), XEVE_ERR_UNKNOWN);

    /* picture signature */
    if (ctx->param.use_pic_sign)
    {
        /* get picture signature */
        ret = xeve_md5_imgb(imgb_hdr_md5, pic_sign);
        xeve_assert_rv(ret == XEVE_OK, ret);
    }
    imgb_free1(imgb_hdr_md5);
    return XEVE_OK;
}

int xeve_eco_pic_signature_main(XEVE_CTX * ctx, XEVE_BSW * bs, u8 pic_sign[N_C][16])
{
    int ret;

    if (ctx->pps.pic_dra_enabled_flag == 0)
    {
        ret = xeve_picbuf_signature(PIC_CURR(ctx), pic_sign);
        xeve_assert_rv(ret == XEVE_OK, ret);
    }
    else
    {
        ret = xeve_eco_udata_hdr(ctx, bs, pic_sign);
        xeve_assert_rv(ret == XEVE_OK, ret);
    }

    return ret;
}

void xevem_eco_alf_golomb(XEVE_BSW * bs, int coeff, int k, const BOOL signed_coeff)
{
    unsigned int symbol = abs(coeff);
    while (symbol >= (unsigned int)(1 << k))
    {
        symbol -= 1 << k;
        k++;
#if TRACE_HLS
        xeve_bsw_write1_trace(bs, 0, 0);
#else
        xeve_bsw_write1(bs, 0);
#endif
    }
#if TRACE_HLS
    xeve_bsw_write1_trace(bs, 1, 0);
#else
    xeve_bsw_write1(bs, 1);
#endif

    if (k > 0)
    {
#if TRACE_HLS
        xeve_bsw_write_trace(bs, symbol, "bins", k);
#else
        xeve_bsw_write(bs, symbol, k);
#endif
    }

    if (signed_coeff && coeff != 0)
    {
#if TRACE_HLS
        xeve_bsw_write1_trace(bs, (coeff < 0) ? 0 : 1, 0);
#else
        xeve_bsw_write1(bs, (coeff < 0) ? 0 : 1);
#endif
    }
}

void xeve_eco_alf_filter(XEVE_BSW * bs, XEVE_ALF_SLICE_PARAM asp, const BOOL is_chroma)
{
    const XEVE_ALF_SLICE_PARAM * alf_slice_param = &asp;
    if (!is_chroma)
    {
        xeve_bsw_write1(bs, alf_slice_param->coef_delta_flag); // "alf_coefficients_delta_flag"
        if (!alf_slice_param->coef_delta_flag)
        {
            if (alf_slice_param->num_luma_filters > 1)
            {
                xeve_bsw_write1(bs, alf_slice_param->coef_delta_pred_mode_flag); // "coeff_delta_pred_mode_flag"
            }
        }
    }

    // this logic need to be moved to ALF files
    ALF_FILTER_SHAPE alf_shape;
    alf_init_filter_shape( &alf_shape, is_chroma ? 5 : ( alf_slice_param->luma_filter_type == ALF_FILTER_5 ? 5 : 7 ) );

    int bits_coef_scan[MAX_SCAN_VAL][MAX_EXP_GOLOMB];
    xeve_mset(bits_coef_scan, 0, MAX_SCAN_VAL*MAX_EXP_GOLOMB * sizeof(int));

    const int maxGolombIdx = alf_shape.filter_type == 0 ? 2 : 3;
    const short* coeff = is_chroma ? alf_slice_param->chroma_coef : alf_slice_param->luma_coef;
    const int num_filters = is_chroma ? 1 : alf_slice_param->num_luma_filters;

    // vlc for all
    for (int ind = 0; ind < num_filters; ++ind)
    {
        if (is_chroma || !alf_slice_param->coef_delta_flag || alf_slice_param->filter_coef_flag[ind])
        {
            for (int i = 0; i < alf_shape.num_coef - 1; i++)
            {
                int coef_val = abs(coeff[ind * MAX_NUM_ALF_LUMA_COEFF + i]);

                for (int k = 1; k < 15; k++)
                {
                    bits_coef_scan[alf_shape.golombIdx[i]][k] += xeve_alf_length_golomb(coef_val, k, TRUE);
                }
            }
        }
    }

    int k_min_tab[MAX_NUM_ALF_COEFF];
    int k_min = xeve_alf_get_golomb_k_min(&alf_shape, num_filters, k_min_tab, bits_coef_scan);

    // Golomb parameters
    u32 alf_luma_min_eg_order_minus1 = k_min - 1;
    xeve_bsw_write_ue(bs, alf_luma_min_eg_order_minus1);

    for (int idx = 0; idx < maxGolombIdx; idx++)
    {
        BOOL alf_eg_order_increase_flag = (k_min_tab[idx] != k_min) ? TRUE : FALSE;
        xeve_bsw_write1(bs, alf_eg_order_increase_flag);
        k_min = k_min_tab[idx];
    }

    if (!is_chroma)
    {
        if (alf_slice_param->coef_delta_flag)
        {
            for (int ind = 0; ind < num_filters; ++ind)
            {
                xeve_bsw_write1(bs, alf_slice_param->filter_coef_flag[ind]);
            }
        }
    }

    // Filter coefficients
    for (int ind = 0; ind < num_filters; ++ind)
    {
        if (!is_chroma && !alf_slice_param->filter_coef_flag[ind] && alf_slice_param->coef_delta_flag)
        {
            continue;
        }

        for (int i = 0; i < alf_shape.num_coef - 1; i++)
        {
            xevem_eco_alf_golomb(bs, coeff[ind* MAX_NUM_ALF_LUMA_COEFF + i], k_min_tab[alf_shape.golombIdx[i]], TRUE);
        }
    }
}

int xevem_eco_dra_aps_param(XEVE_BSW * bs, XEVE_APS_GEN * aps, int bit_depth)
{
    SIG_PARAM_DRA *p_dra_param = (SIG_PARAM_DRA *)aps->aps_data;
    xeve_bsw_write(bs, (u32)p_dra_param->dra_descriptor1, 4);
    xeve_bsw_write(bs, (u32)p_dra_param->dra_descriptor2, 4);
    xeve_bsw_write_ue(bs, (u32)p_dra_param->num_ranges - 1);
    xeve_bsw_write1(bs, p_dra_param->equal_ranges_flag);
    xeve_bsw_write(bs, (u32)p_dra_param->in_ranges[0], bit_depth); // delta_luma_dqp_change_point

    if (p_dra_param->equal_ranges_flag == TRUE)
    {
        xeve_bsw_write(bs, (u32)p_dra_param->delta_val, bit_depth);
    }
    else
    {
        for (int i = 1; i <= p_dra_param->num_ranges; i++)
        {
            xeve_bsw_write(bs, (u32)(p_dra_param->in_ranges[i] - p_dra_param->in_ranges[i - 1]), bit_depth);
        }
    }

    int numBits = p_dra_param->dra_descriptor1 + p_dra_param->dra_descriptor2;
    for (int i = 0; i < p_dra_param->num_ranges; i++)
    {
        xeve_bsw_write(bs, p_dra_param->dra_scale_value[i], numBits);
    }

    xeve_bsw_write(bs, p_dra_param->dra_cb_scale_value, numBits);
    xeve_bsw_write(bs, p_dra_param->dra_cr_scale_value, numBits);
    xeve_bsw_write_ue(bs, (u32)p_dra_param->dra_table_idx);
    p_dra_param->signal_dra_flag = 0; // dra was sent
    return XEVE_OK;
}

int xevem_eco_alf_aps_param(XEVE_BSW * bs, XEVE_APS_GEN * aps)
{
    XEVE_ALF_SLICE_PARAM *p_alfSliceParam = (XEVE_ALF_SLICE_PARAM *)aps->aps_data;
    XEVE_ALF_SLICE_PARAM alf_slice_param = *p_alfSliceParam;

    u32 alf_luma_filter_signal_flag = alf_slice_param.enable_flag[0];
    u8  alf_chroma_filter_signal_flag = (alf_slice_param.enable_flag[1] || alf_slice_param.enable_flag[2]);

    xeve_bsw_write1(bs, alf_luma_filter_signal_flag);
    xeve_bsw_write1(bs, alf_chroma_filter_signal_flag);

    if (alf_slice_param.enable_flag[0])
    {
        u32 alf_luma_num_filters_signalled_minus1 = alf_slice_param.num_luma_filters - 1;
        u32 alf_luma_type_flag = alf_slice_param.luma_filter_type;
        xeve_bsw_write_ue(bs, alf_luma_num_filters_signalled_minus1);
        xeve_bsw_write1(bs, alf_luma_type_flag); //  "filter_type_flag"

        if (alf_slice_param.num_luma_filters > 1)
        {
            s16 * alf_luma_coeff_delta_idx = alf_slice_param.filter_coef_delta_idx;
            for (int i = 0; i < MAX_NUM_ALF_CLASSES; i++)
            {
                xeve_bsw_write(bs, alf_luma_coeff_delta_idx[i], xeve_tbl_log2[alf_slice_param.num_luma_filters - 1] + 1);
            }
        }
        const int num_fixed_filter_per_class = 16;
        {
            xevem_eco_alf_golomb(bs, alf_slice_param.fixed_filter_pattern, 0, FALSE);

            if (alf_slice_param.fixed_filter_pattern == 2)
            {
                u8 * alf_luma_fixed_filter_usage_flag = alf_slice_param.fixed_filter_usage_flag;
                for (int class_idx = 0; class_idx < MAX_NUM_ALF_CLASSES; class_idx++)
                {
                    xeve_bsw_write1(bs, alf_luma_fixed_filter_usage_flag[class_idx]);
                }
            }
            if (alf_slice_param.fixed_filter_pattern > 0)
            {
                s32 * alf_luma_fixed_filter_set_idx = alf_slice_param.fixed_filter_idx;
                for (int class_idx = 0; class_idx < MAX_NUM_ALF_CLASSES; class_idx++)
                {
                    if (alf_slice_param.fixed_filter_usage_flag[class_idx] > 0)
                    {
                        xeve_bsw_write(bs, alf_luma_fixed_filter_set_idx[class_idx], xeve_tbl_log2[num_fixed_filter_per_class - 1] + 1);
                    }
                }
            }
        }

        xeve_eco_alf_filter(bs, alf_slice_param, FALSE);
    }
    if (alf_chroma_filter_signal_flag)
    {
        {
            xeve_eco_alf_filter(bs, alf_slice_param, TRUE);
        }
    }
    return XEVE_OK;
}


int xevem_eco_alf_sh_param(XEVE_BSW * bs, XEVE_SH * sh)
{
    XEVE_ALF_SLICE_PARAM * alf_slice_param = &sh->alf_sh_param;

    xeve_bsw_write1(bs, alf_slice_param->is_ctb_alf_on);
    return XEVE_OK;
}

static int cu_init_main(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int cup, int cuw, int cuh)
{
    XEVE_CU_DATA *cu_data = &ctx->map_cu_data[core->lcu_num];
    XEVEM_CORE   *mcore = (XEVEM_CORE *)core;

    mcore->ibc_flag = 0;
    mcore->mmvd_flag = 0;
    mcore->affine_flag = cu_data->affine_flag[cup];
    mcore->ats_inter_info = cu_data->ats_inter_info[cup];
    mcore->ats_intra_cu = cu_data->ats_intra_cu[cup];
    mcore->ats_mode = (cu_data->ats_mode_h[cup] << 1 | cu_data->ats_mode_v[cup]);
    mcore->dmvr_flag = 0;

    cu_init(ctx, core, x, y, cup, cuw, cuh);

    if (core->cu_mode == MODE_IBC)
    {
        mcore->ibc_flag = 1;

        if (!xeve_check_luma(core->tree_cons))
        {
            xeve_assert(0);
        }
        mcore->mmvd_flag = 0; // core->new_skip_flag = 0;
        mcore->affine_flag = 0;
        core->avail_cu = xeve_get_avail_ibc(core->x_scu, core->y_scu, ctx->w_scu, ctx->h_scu, core->scup, core->cuw, core->cuh, ctx->map_scu, ctx->map_tidx);
    }
    else if (core->cu_mode >= MODE_SKIP_MMVD)
    {
        if(cu_data->pred_mode[cup] == MODE_SKIP_MMVD)
        {
            core->skip_flag = 1;
            mcore->mmvd_flag = 1;
        }
    }

    return XEVE_OK;
}

int xevem_eco_unit(XEVE_CTX * ctx, XEVE_CORE * core, int x, int y, int cup, int cuw, int cuh , TREE_CONS tree_cons, XEVE_BSW * bs)
{
    XEVEM_CTX  *mctx = (XEVEM_CTX *)ctx;
    XEVEM_CORE *mcore = (XEVEM_CORE *)core;

    core->tree_cons = tree_cons;
    s16(*coef)[MAX_CU_DIM] = core->ctmp;

    u32 *map_scu;
    int slice_type, refi0, refi1;
    int i, j, w, h;
    XEVE_CU_DATA *cu_data = &ctx->map_cu_data[core->lcu_num];
    u32 *map_cu_mode;
    u32 *map_affine;
#if TRACE_ENC_CU_DATA
    core->trace_idx = cu_data->trace_idx[cup];
#endif
#if TRACE_ENC_HISTORIC
    xeve_mcpy(&core->history_buffer, &(cu_data->history_buf[cup]), sizeof(core->history_buffer));
#endif
#if TRACE_ENC_CU_DATA_CHECK
    xeve_assert(core->trace_idx != 0);
#endif
    int enc_dqp;
    slice_type = ctx->slice_type;
    cu_init_main(ctx, core, x, y, cup, cuw, cuh);

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

#if ENC_DEC_TRACE
    if (ctx->sh->slice_type != SLICE_I && ctx->sps.sps_btt_flag)
    {
        XEVE_TRACE_STR("tree status ");
        XEVE_TRACE_INT(core->tree_cons.tree_type);
        XEVE_TRACE_STR("mode status ");
        XEVE_TRACE_INT(core->tree_cons.mode_cons);
    }
#endif  
    XEVE_TRACE_STR("\n");

    xeve_get_ctx_some_flags(core->x_scu, core->y_scu, cuw, cuh, ctx->w_scu, ctx->map_scu, ctx->map_cu_mode, core->ctx_flags
                         , ctx->sh->slice_type, ctx->sps.tool_cm_init , ctx->param.ibc_flag, ctx->sps.ibc_log_max_size, ctx->map_tidx);

    if (ctx->sps.tool_admvp && core->log2_cuw == MIN_CU_LOG2 && core->log2_cuh == MIN_CU_LOG2)
    {
        xeve_assert(cu_data->pred_mode[cup] == MODE_INTRA || cu_data->pred_mode[cup] == MODE_IBC);
    }

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
    if(slice_type != SLICE_I && (ctx->sps.tool_admvp == 0 || !(core->log2_cuw <= MIN_CU_LOG2 && core->log2_cuh <= MIN_CU_LOG2) || ctx->param.ibc_flag) && !xeve_check_only_intra(core->tree_cons) )
    {
        if(!(ctx->sps.tool_admvp && core->log2_cuw == MIN_CU_LOG2 && core->log2_cuh == MIN_CU_LOG2))
        {
            xeve_eco_skip_flag(bs, core->skip_flag,core->ctx_flags[CNID_SKIP_FLAG]);
        }

        if(core->skip_flag)
        {
            if(ctx->sps.tool_mmvd)
            {
                xevem_eco_mmvd_flag(bs, mcore->mmvd_flag);
            }

            if(mcore->mmvd_flag)
            {
                xevem_eco_mmvd_info(bs, cu_data->mmvd_idx[cup], ctx->sh->mmvd_group_enable_flag && !(cuw*cuh <= NUM_SAMPLES_BLOCK));
            }
            else
            {
                if(cuw >= 8 && cuh >= 8 && ctx->sps.tool_affine)
                {
                    xevem_eco_affine_flag(bs, mcore->affine_flag != 0, core->ctx_flags[CNID_AFFN_FLAG]); /* skip affine_flag */
                }

                if(mcore->affine_flag)
                {
                    xevem_eco_affine_mrg_idx(bs, cu_data->mvp_idx[cup][REFP_0]);
                }
                else
                {
                    if(!ctx->sps.tool_admvp)
                    {
                        xeve_eco_mvp_idx(bs, cu_data->mvp_idx[cup][REFP_0]);

                        if(slice_type == SLICE_B)
                        {
                            xeve_eco_mvp_idx(bs, cu_data->mvp_idx[cup][REFP_1]);
                        }
                    }
                    else
                    {
                        xevem_eco_merge_idx(bs, cu_data->mvp_idx[cup][REFP_0]);
                    }
                }
            }
        }
        else
        {
            if (xeve_check_all_preds(core->tree_cons))
                if (!(ctx->sps.tool_admvp && core->log2_cuw == MIN_CU_LOG2 && core->log2_cuh == MIN_CU_LOG2))
                {
                    xeve_eco_pred_mode(bs, core->cu_mode, core->ctx_flags[CNID_PRED_MODE]);
                }

            if (((( core->cu_mode
                != MODE_INTRA) || (ctx->sps.tool_admvp && core->log2_cuw == MIN_CU_LOG2 && core->log2_cuh == MIN_CU_LOG2))
                && !xeve_check_only_inter(core->tree_cons) )
                && xeve_check_luma(core->tree_cons)
                && ctx->param.ibc_flag && core->log2_cuw <= ctx->sps.ibc_log_max_size && core->log2_cuh <= ctx->sps.ibc_log_max_size)
            {
                xevem_eco_ibc_flag(bs, mcore->ibc_flag, core->ctx_flags[CNID_IBC_FLAG]);
            }

            if(core->cu_mode != MODE_INTRA && core->cu_mode != MODE_IBC)
            {
                if(ctx->sps.tool_amvr)
                {
                    xevem_eco_mvr_idx(bs, cu_data->mvr_idx[cup]);
                }

                {
                    if(slice_type == SLICE_B && ctx->sps.tool_admvp == 0)
                    {
                        xeve_eco_direct_mode_flag(bs, cu_data->pred_mode[cup] == MODE_DIR);
                    }
                    else if(ctx->sps.tool_admvp && cu_data->mvr_idx[cup] == 0 )
                    {
                        xevem_eco_merge_mode_flag(bs, cu_data->pred_mode[cup] == MODE_DIR || cu_data->pred_mode[cup] == MODE_DIR_MMVD);
                    }

                    if(ctx->sps.tool_mmvd)
                    {
                        if((cu_data->pred_mode[cup] == MODE_DIR) || (cu_data->pred_mode[cup] == MODE_DIR_MMVD))
                        {
                            xevem_eco_mmvd_flag(bs, cu_data->pred_mode[cup] == MODE_DIR_MMVD);
                        }

                        if((cu_data->pred_mode[cup] == MODE_DIR_MMVD))
                        {
                            xevem_eco_mmvd_info(bs, cu_data->mmvd_idx[cup], ctx->sh->mmvd_group_enable_flag && !(cuw*cuh <= NUM_SAMPLES_BLOCK));
                        }
                    }

                    if(cu_data->pred_mode[cup] == MODE_DIR && cuw >= 8 && cuh >= 8 && ctx->sps.tool_affine)
                    {
                        xevem_eco_affine_flag(bs, mcore->affine_flag != 0, core->ctx_flags[CNID_AFFN_FLAG]); /* direct affine_flag */
                        if(mcore->affine_flag)
                        {
                            xevem_eco_affine_mrg_idx(bs, cu_data->mvp_idx[cup][REFP_0]);
                        }
                    }
                    if(ctx->sps.tool_admvp == 1 && cu_data->pred_mode[cup] == MODE_DIR && !mcore->affine_flag && cu_data->mvr_idx[cup] == 0 )
                    {
                        xevem_eco_merge_idx(bs, cu_data->mvp_idx[cup][REFP_0]);
                    }
                }

                if(((cu_data->pred_mode[cup] % ORG_PRED_NUM) != MODE_DIR) && ((cu_data->pred_mode[cup] % ORG_PRED_NUM) != MODE_DIR_MMVD))
                {
                    if (slice_type == SLICE_B)
                    {
                        xeve_eco_inter_pred_idc(bs, cu_data->refi[cup], slice_type, cuw, cuh, ctx->sps.tool_admvp);
                    }

                    // affine inter mode
                    if(cuw >= 16 && cuh >= 16 && cu_data->mvr_idx[cup] == 0 && ctx->sps.tool_affine)
                    {
                        xevem_eco_affine_flag(bs, mcore->affine_flag != 0, core->ctx_flags[CNID_AFFN_FLAG]); /* inter affine_flag */
                    }

                    if(mcore->affine_flag)
                    {
                        xevem_eco_affine_mode(bs, mcore->affine_flag - 1); /* inter affine_mode */
                    }

                    if(mcore->affine_flag)
                    {
                        int vertex;
                        int vertex_num = mcore->affine_flag + 1;
                        int aff_scup[VER_NUM] = { 0 };

                        aff_scup[0] = cup;
                        aff_scup[1] = cup + ((cuw >> MIN_CU_LOG2) - 1);
                        aff_scup[2] = cup + (((cuh >> MIN_CU_LOG2) - 1) << ctx->log2_culine);

                        refi0 = cu_data->refi[cup][REFP_0];
                        refi1 = cu_data->refi[cup][REFP_1];

                        if(IS_INTER_SLICE(slice_type) && REFI_IS_VALID(refi0))
                        {
                            int b_zero = 1;

                            xeve_eco_refi(bs, ctx->rpm.num_refp[REFP_0], refi0);
                            xevem_eco_affine_mvp_idx( bs, cu_data->mvp_idx[cup][REFP_0] );

                            for(vertex = 0; vertex < vertex_num; vertex++)
                            {
                                int mvd_x = cu_data->mvd[aff_scup[vertex]][REFP_0][MV_X];
                                int mvd_y = cu_data->mvd[aff_scup[vertex]][REFP_0][MV_Y];
                                if(mvd_x != 0 || mvd_y != 0)
                                {
                                    b_zero = 0;
                                    break;
                                }
                            }
                            xevem_eco_affine_mvd_flag(bs, b_zero, REFP_0);

                            if(b_zero == 0)
                            {
                                for(vertex = 0; vertex < vertex_num; vertex++)
                                {
                                    xeve_eco_mvd(bs, cu_data->mvd[aff_scup[vertex]][REFP_0]);
                                }
                            }
                        }

                        if(slice_type == SLICE_B && REFI_IS_VALID(refi1))
                        {
                            int b_zero = 1;

                            xeve_eco_refi(bs, ctx->rpm.num_refp[REFP_1], refi1);
                            xevem_eco_affine_mvp_idx(bs, cu_data->mvp_idx[cup][REFP_1]);

                            for(vertex = 0; vertex < vertex_num; vertex++)
                            {
                                int mvd_x = cu_data->mvd[aff_scup[vertex]][REFP_1][MV_X];
                                int mvd_y = cu_data->mvd[aff_scup[vertex]][REFP_1][MV_Y];
                                if(mvd_x != 0 || mvd_y != 0)
                                {
                                    b_zero = 0;
                                    break;
                                }
                            }
                            xevem_eco_affine_mvd_flag(bs, b_zero, REFP_1);

                            if(b_zero == 0)
                                for(vertex = 0; vertex < vertex_num; vertex++)
                                {
                                    xeve_eco_mvd(bs, cu_data->mvd[aff_scup[vertex]][REFP_1]);
                                }
                        }
                    }
                    else
                    {
                        if(ctx->sps.tool_admvp == 1 && REFI_IS_VALID(cu_data->refi[cup][REFP_0]) && REFI_IS_VALID(cu_data->refi[cup][REFP_1]))
                        {
                            xevem_eco_bi_idx(bs, cu_data->bi_idx[cup] - 1);
                        }

                        refi0 = cu_data->refi[cup][REFP_0];
                        refi1 = cu_data->refi[cup][REFP_1];
                        if(IS_INTER_SLICE(slice_type) && REFI_IS_VALID(refi0))
                        {
                            if(ctx->sps.tool_admvp == 0)
                            {
                                xeve_eco_refi(bs, ctx->rpm.num_refp[REFP_0], refi0);
                                xeve_eco_mvp_idx(bs, cu_data->mvp_idx[cup][REFP_0]);
                                xeve_eco_mvd(bs, cu_data->mvd[cup][REFP_0]);
                            }
                            else
                            {
                                if(cu_data->bi_idx[cup] != BI_FL0 && cu_data->bi_idx[cup] != BI_FL1)
                                {
                                    xeve_eco_refi(bs, ctx->rpm.num_refp[REFP_0], refi0);
                                }

                                cu_data->mvd[cup][REFP_0][MV_Y] >>= cu_data->mvr_idx[cup];
                                cu_data->mvd[cup][REFP_0][MV_X] >>= cu_data->mvr_idx[cup];

                                if(cu_data->bi_idx[cup] != BI_FL0)
                                {
                                    xeve_eco_mvd(bs, cu_data->mvd[cup][REFP_0]);
                                }

                                cu_data->mvd[cup][REFP_0][MV_Y] <<= cu_data->mvr_idx[cup];
                                cu_data->mvd[cup][REFP_0][MV_X] <<= cu_data->mvr_idx[cup];
                            }
                        }

                        if(slice_type == SLICE_B && REFI_IS_VALID(refi1))
                        {
                            if(ctx->sps.tool_admvp == 0)
                            {
                                xeve_eco_refi(bs, ctx->rpm.num_refp[REFP_1], refi1);
                                xeve_eco_mvp_idx(bs, cu_data->mvp_idx[cup][REFP_1]);
                                xeve_eco_mvd(bs, cu_data->mvd[cup][REFP_1]);
                            }
                            else
                            {
                                if(cu_data->bi_idx[cup] != BI_FL0 && cu_data->bi_idx[cup] != BI_FL1)
                                {
                                    xeve_eco_refi(bs, ctx->rpm.num_refp[REFP_1], refi1);
                                }

                                cu_data->mvd[cup][REFP_1][MV_Y] >>= cu_data->mvr_idx[cup];
                                cu_data->mvd[cup][REFP_1][MV_X] >>= cu_data->mvr_idx[cup];

                                if(cu_data->bi_idx[cup] != BI_FL1)
                                {
                                    xeve_eco_mvd(bs, cu_data->mvd[cup][REFP_1]);
                                }

                                cu_data->mvd[cup][REFP_1][MV_Y] <<= cu_data->mvr_idx[cup];
                                cu_data->mvd[cup][REFP_1][MV_X] <<= cu_data->mvr_idx[cup];
                            }
                        }
                    }
                }
            }
        }
    }
    else if (((ctx->sh->slice_type == SLICE_I || xeve_check_only_intra(core->tree_cons)) && ctx->param.ibc_flag))
    {
        if (core->skip_flag == 0 && xeve_check_luma(core->tree_cons))
        {
            if (core->log2_cuw <= ctx->sps.ibc_log_max_size && core->log2_cuh <= ctx->sps.ibc_log_max_size)
            {
                xevem_eco_ibc_flag(bs, mcore->ibc_flag, core->ctx_flags[CNID_IBC_FLAG]);
            }
        }
    }

    if(core->cu_mode == MODE_INTRA)
    {
        xeve_assert(cu_data->ipm[0][cup] != IPD_INVALID);
        xeve_assert(cu_data->ipm[1][cup] != IPD_INVALID);

        if(ctx->sps.tool_eipd)
        {
            xevem_get_mpm(core->x_scu, core->y_scu, cuw, cuh, ctx->map_scu, ctx->map_ipm, core->scup, ctx->w_scu
                            , core->mpm, core->avail_lr, mcore->mpm_ext, mcore->pims, ctx->map_tidx);
            if (xeve_check_luma(core->tree_cons))
            {
                xevem_eco_intra_dir(bs, cu_data->ipm[0][cup], core->mpm, mcore->mpm_ext, mcore->pims);
            }
            if (xeve_check_chroma(core->tree_cons) && ctx->sps.chroma_format_idc)
            {
                int luma_ipm = cu_data->ipm[0][cup];
                if (!xeve_check_luma(core->tree_cons))
                {
                    int luma_cup = xeve_get_luma_cup(core->x_scu - PEL2SCU(core->x_pel), core->y_scu - PEL2SCU(core->y_pel), PEL2SCU(cuw), PEL2SCU(cuh), 1 << ctx->log2_culine);
                    if (cu_data->pred_mode[luma_cup] == MODE_INTRA)
                    {
                        luma_ipm = cu_data->ipm[0][luma_cup];
                    }
                    else
                    {
                        luma_ipm = IPD_DC;
                    }
                }
                xeve_assert(cu_data->ipm[1][cup] != IPD_INVALID);
                xevem_eco_intra_dir_c(bs, cu_data->ipm[1][cup], luma_ipm);
            }
        }
        else
        {
            xeve_get_mpm(core->x_scu, core->y_scu, cuw, cuh, ctx->map_scu, ctx->map_ipm, core->scup, ctx->w_scu, &core->mpm_b_list, ctx->map_tidx);

            if (xeve_check_luma(core->tree_cons))
            {
                xeve_eco_intra_dir(bs, cu_data->ipm[0][cup], core->mpm_b_list);
            }
        }
    }
    else if (mcore->ibc_flag)
    {
      if (core->skip_flag == 0)
      {
        if (core->cu_mode == MODE_IBC)        // Does this condition required?
        {
            if (!xeve_check_luma(core->tree_cons))
            {
                xeve_assert(0);
            }
            else
            {
                xeve_eco_mvd(bs, cu_data->mvd[cup][REFP_0]);
            }
        }
      }
    }

    if((core->skip_flag == 0) && (mcore->mmvd_flag == 0))
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
    map_affine = mctx->map_affine + core->scup;
    map_cu_mode = ctx->map_cu_mode + core->scup;

    if (xeve_check_luma(core->tree_cons))
    {
    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            if((core->skip_flag) || (mcore->mmvd_flag))
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

            if(mcore->affine_flag)
            {
                MCU_SET_AFF(map_scu[j], mcore->affine_flag);

                MCU_SET_AFF_LOGW(map_affine[j], core->log2_cuw);
                MCU_SET_AFF_LOGH(map_affine[j], core->log2_cuh);
                MCU_SET_AFF_XOFF(map_affine[j], j);
                MCU_SET_AFF_YOFF(map_affine[j], i);
            }
            else
            {
                MCU_CLR_AFF(map_scu[j]);
            }

            if (mcore->ibc_flag)
            {
              MCU_SET_IBC(map_scu[j]);
            }
            else
            {
              MCU_CLR_IBC(map_scu[j]);
            }

            MCU_SET_LOGW(map_cu_mode[j], core->log2_cuw);
            MCU_SET_LOGH(map_cu_mode[j], core->log2_cuh);

            if (mcore->mmvd_flag)
            {
                MCU_SET_MMVDS(map_cu_mode[j]);
            }
            else
            {
                MCU_CLR_MMVDS(map_cu_mode[j]);
            }
        }
        map_scu += ctx->w_scu;
        map_affine += ctx->w_scu;
        map_cu_mode += ctx->w_scu;
    }
    if (mcore->ats_inter_info)
    {
        assert(core->nnz_sub[Y_C][0] == core->nnz[Y_C]);
        assert(core->nnz_sub[U_C][0] == core->nnz[U_C]);
        assert(core->nnz_sub[V_C][0] == core->nnz[V_C]);
        set_cu_cbf_flags(core->nnz[Y_C], mcore->ats_inter_info, core->log2_cuw, core->log2_cuh, ctx->map_scu + core->scup, ctx->w_scu);
    }

    }
    if (xeve_check_chroma(core->tree_cons))
    {
        if (!xeve_check_luma(core->tree_cons))
        {
            xeve_assert((core->cu_mode == MODE_INTRA) || (core->cu_mode == MODE_IBC));
        }
    }

#if TRACE_ENC_CU_DATA
    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("RDO check id ");
    XEVE_TRACE_INT((int)core->trace_idx);
    XEVE_TRACE_STR("\n");
    xeve_assert(core->trace_idx != 0);
#endif
#if TRACE_ENC_HISTORIC
    //if (core->cu_mode != MODE_INTRA)
    {
        XEVE_TRACE_COUNTER;
        XEVE_TRACE_STR("Historic (");
        XEVE_TRACE_INT((int)core->history_buffer.currCnt);
        XEVE_TRACE_STR("): ");
        for (int i = 0; i < core->history_buffer.currCnt; ++i)
        {
            XEVE_TRACE_STR("(");
            XEVE_TRACE_INT((int)core->history_buffer.history_mv_table[i][REFP_0][MV_X]);
            XEVE_TRACE_STR(", ");
            XEVE_TRACE_INT((int)core->history_buffer.history_mv_table[i][REFP_0][MV_Y]);
            XEVE_TRACE_STR("; ");
            XEVE_TRACE_INT((int)core->history_buffer.history_refi_table[i][REFP_0]);
            XEVE_TRACE_STR("), (");
            XEVE_TRACE_INT((int)core->history_buffer.history_mv_table[i][REFP_1][MV_X]);
            XEVE_TRACE_STR(", ");
            XEVE_TRACE_INT((int)core->history_buffer.history_mv_table[i][REFP_1][MV_Y]);
            XEVE_TRACE_STR("; ");
            XEVE_TRACE_INT((int)core->history_buffer.history_refi_table[i][REFP_1]);
            XEVE_TRACE_STR("); ");
        }
        XEVE_TRACE_STR("\n");
    }
#endif

#if MVF_TRACE
    // Trace MVF
    {
        s8(*map_refi)[REFP_NUM];
        s16(*map_mv)[REFP_NUM][MV_D];
        s16(*map_unrefined_mv)[REFP_NUM][MV_D];
        u32  *map_scu;
        XEVEM_CTX *mctx = (XEVEM_CTX *)ctx;

        map_affine = mctx->map_affine + core->scup;
        map_refi = ctx->map_refi + core->scup;
        map_scu = ctx->map_scu + core->scup;
        map_mv = ctx->map_mv + core->scup;
        map_unrefined_mv = ctx->map_unrefined_mv + core->scup;

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

                XEVE_TRACE_STR(" affine: ");
                XEVE_TRACE_INT(MCU_GET_AFF(map_scu[j]));
                if(MCU_GET_AFF(map_scu[j]))
                {
                    XEVE_TRACE_STR(" logw: ");
                    XEVE_TRACE_INT(MCU_GET_AFF_LOGW(map_affine[j]));
                    XEVE_TRACE_STR(" logh: ");
                    XEVE_TRACE_INT(MCU_GET_AFF_LOGH(map_affine[j]));
                    XEVE_TRACE_STR(" xoff: ");
                    XEVE_TRACE_INT(MCU_GET_AFF_XOFF(map_affine[j]));
                    XEVE_TRACE_STR(" yoff: ");
                    XEVE_TRACE_INT(MCU_GET_AFF_YOFF(map_affine[j]));
                }
                if (MCU_GET_DMVRF(map_scu[j]))
                {
                    //map_unrefined_mv += ctx->w_scu;
                    XEVE_TRACE_STR("; DMVR: ref0: ");
                    XEVE_TRACE_INT(map_refi[j][REFP_0]);
                    XEVE_TRACE_STR(" mv: ");
                    XEVE_TRACE_MV(map_unrefined_mv[j][REFP_0][MV_X], map_unrefined_mv[j][REFP_0][MV_Y]);

                    XEVE_TRACE_STR(" ref1: ");
                    XEVE_TRACE_INT(map_refi[j][REFP_1]);
                    XEVE_TRACE_STR(" mv: ");
                    XEVE_TRACE_MV(map_unrefined_mv[j][REFP_1][MV_X], map_unrefined_mv[j][REFP_1][MV_Y]);
                }
                XEVE_TRACE_STR("\n");
            }

            map_refi += ctx->w_scu;
            map_mv += ctx->w_scu;
            map_scu += ctx->w_scu;
            map_affine += ctx->w_scu;
            map_unrefined_mv += ctx->w_scu;
        }
    }
#endif

    return XEVE_OK;
}

#if GRAB_STAT
void ence_stat_cu(int x, int y, int cuw, int cuh, int cup, void *ctx, void *core, TREE_CONS tree_cons)
{
    XEVE_CTX *enc_ctx = (XEVE_CTX *)ctx;
    XEVE_CORE *enc_core = (XEVE_CORE *)core;
    XEVE_CU_DATA *cu_data = &enc_ctx->map_cu_data[enc_core->lcu_num];
    int scup = PEL2SCU(y) * enc_ctx->w_scu + PEL2SCU(x);

    int pred_mode = cu_data->pred_mode[cup];
    int mmvd_flag = 0;

    if (pred_mode > MODE_DIR && pred_mode < MODE_IBC)
    {
        pred_mode -= 2;
        mmvd_flag = 1;
    }

    if (xeve_check_only_inter(tree_cons))
    {
        xeve_assert(pred_mode == MODE_INTER);
    }
    if (xeve_check_only_intra(tree_cons))
    {
        xeve_assert( (pred_mode == MODE_IBC) || (pred_mode == MODE_INTRA) );
    }

    xeve_stat_write_cu_str(x, y, cuw, cuh, "PredMode", pred_mode);
    xeve_stat_write_cu_str(x, y, cuw, cuh, "AffineFlag", cu_data->affine_flag[cup]);
    xeve_stat_write_cu_str(x, y, cuw, cuh, "MMVDFlag", mmvd_flag);
    xeve_stat_write_cu_vec(x, y, cuw, cuh, "MV0", cu_data->mv[cup][0][0], cu_data->mv[cup][0][1]);
    xeve_stat_write_cu_str(x, y, cuw, cuh, "REF0", cu_data->refi[cup][0]);
    xeve_stat_write_cu_vec(x, y, cuw, cuh, "MV1", cu_data->mv[cup][1][0], cu_data->mv[cup][1][1]);
    xeve_stat_write_cu_str(x, y, cuw, cuh, "REF1", cu_data->refi[cup][1]);
    xeve_stat_write_cu_str(x, y, cuw, cuh, "ats_intra_cu", cu_data->ats_intra_cu[cup]);
    xeve_stat_write_cu_str(x, y, cuw, cuh, "ats_inter_info", cu_data->ats_inter_info[cup]);
    if (xeve_check_luma(tree_cons))
    {
        xeve_stat_write_cu_str(x, y, cuw, cuh, "CBF_luma", cu_data->nnz[Y_C][cup] > 0);
        xeve_stat_write_cu_str(x, y, cuw, cuh, "Tile_ID", enc_core->tile_num);
        xeve_stat_write_cu_str(x, y, cuw, cuh, "Slice_IDX", enc_ctx->slice_num);
    }
}
#endif
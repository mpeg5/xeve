
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

#include "xeve.h"
#include "xeve_app_util.h"
#include "xeve_app_args.h"

#if LINUX
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#endif

#define VERBOSE_NONE               VERBOSE_0
#define VERBOSE_SIMPLE             VERBOSE_1
#define VERBOSE_FRAME              VERBOSE_2

#define MAX_BS_BUF                 (16*1024*1024)

typedef enum _STATES {
    STATE_ENCODING,
    STATE_BUMPING,
    STATE_SKIPPING
} STATES;


static void print_usage(void)
{
    int i;
    char str[1024];

    logv0("< Usage >\n");

    for(i=0; i<NUM_ARG_OPTION; i++)
    {
        if(args_get_help(options, i, str) < 0) return;
        logv0("%s\n", str);
    }
}

static char get_pic_type(char * in)
{
    int len = (int)strlen(in);
    char type = 0;
    for (int i = 0; i < len; i++) {
        if (in[i] == 'P'){
            type = 'P';
            break;
        } else if (in[i] == 'B') {
            type = 'B';
            break;
        }
    }
    if (type == 0){
        return 0;
    }
    return type;
}

static int get_conf(XEVE_CDSC * cdsc)
{
    int result = 0;
    memset(cdsc, 0, sizeof(XEVE_CDSC));

    cdsc->w = op_w;
    cdsc->h = op_h;
    cdsc->qp = op_qp;
    cdsc->cb_qp_offset = op_cb_qp_offset;
    cdsc->cr_qp_offset = op_cr_qp_offset;
    cdsc->fps = op_fps;
    cdsc->iperiod = op_iperiod;
    cdsc->max_b_frames = op_max_b_frames;
    cdsc->profile = op_profile;
    cdsc->level = op_level;
    cdsc->use_dqp = op_use_dqp;
    cdsc->ref_pic_gap_length = op_ref_pic_gap_length;
    cdsc->inp_bit_depth = op_inp_bit_depth;
    cdsc->codec_bit_depth = op_codec_bit_depth;
    cdsc->constrained_intra_pred = op_constrained_intra_pred;
    cdsc->use_deblock = op_tool_deblocking;

    if (op_out_bit_depth == 0)
    {
        op_out_bit_depth = op_codec_bit_depth;
    }
    cdsc->out_bit_depth = op_out_bit_depth;

    if(op_disable_hgop)
    {
        cdsc->disable_hgop = 1;
    }
    if(op_closed_gop)
    {
        cdsc->closed_gop = 1;
    }

    cdsc->parallel_task_cnt = (op_parallel_task > XEVE_MAX_TASK_CNT) ? XEVE_MAX_TASK_CNT : op_parallel_task;
    cdsc->rdo_dbk_switch = op_rdo_dbk_switch;
    cdsc->inter_slice_type = op_inter_slice_type == 0 ? 0/*SLICE_B*/ : 1/*SLICE_P*/;
    cdsc->add_qp_frame = op_add_qp_frames;
    cdsc->rc_type = op_rc_type; //rc_off =0 , rc_cbr = 1
    cdsc->bps = op_bps;
    cdsc->vbv_msec = op_vbv_msec;
    cdsc->use_filler_flag = 0;
    cdsc->num_pre_analysis_frames = 0;
    cdsc->picture_cropping_flag = op_picture_cropping_flag;
    cdsc->picture_crop_left_offset = op_picture_crop_left_offset;
    cdsc->picture_crop_right_offset = op_picture_crop_right_offset;
    cdsc->picture_crop_top_offset = op_picture_crop_top_offset;
    cdsc->picture_crop_bottom_offset = op_picture_crop_bottom_offset;
    cdsc->bitstream_buf_size = MAX_BS_BUF;

    if (strcmp(op_preset, "fast") == 0)
    {
        cdsc->preset = FAST;
    }
    else if (strcmp(op_preset, "medium") == 0)
    {
        cdsc->preset = MEDIUM;
    }
    else if (strcmp(op_preset, "slow") == 0)
    {
        cdsc->preset = SLOW;
    }
    else if (strcmp(op_preset, "reference") == 0)
    {
        cdsc->preset = REFERENCE;
    }
    else
    {
        return XEVE_ERR_INVALID_ARGUMENT;
    }

    XEVE_CDSC_EXT * cdsc_ext = (XEVE_CDSC_EXT*)malloc(sizeof(XEVE_CDSC_EXT));
    memset(cdsc_ext, 0, sizeof(XEVE_CDSC_EXT));
    cdsc->ext = cdsc_ext;

    if (cdsc->profile == 0)
    {
        cdsc_ext->tile_columns = op_num_tile_columns;
        cdsc_ext->tile_rows = op_num_tile_rows;

        cdsc_ext->tile_array_in_slice[0] = 0;
        cdsc_ext->tile_array_in_slice[1] = (cdsc_ext->tile_columns * cdsc_ext->tile_rows) - 1;
        cdsc_ext->num_remaining_tiles_in_slice_minus1[0] = op_num_remaining_tiles_in_slice[0] - 1;
        cdsc_ext->num_slice_in_pic = op_num_slice_in_pic;
    }
    else
    {
        if (op_enable_ibc)
        {
            cdsc_ext->ibc_flag = 1;
        }

        if (cdsc_ext->ibc_flag)
        {
            cdsc_ext->ibc_search_range_x = op_ibc_search_range_x;
            cdsc_ext->ibc_search_range_y = op_ibc_search_range_y;
            cdsc_ext->ibc_hash_search_flag = op_ibc_hash_search_flag;
            cdsc_ext->ibc_hash_search_max_cand = op_ibc_hash_search_max_cand;
            cdsc_ext->ibc_hash_search_range_4smallblk = op_ibc_hash_search_range_4smallblk;
            cdsc_ext->ibc_fast_method = op_ibc_fast_method;
        }

        cdsc_ext->btt = op_btt;
        cdsc_ext->suco = op_suco;
        cdsc_ext->cu_qp_delta_area = op_cu_qp_delta_area;

        cdsc_ext->framework_cb_max = op_framework_cb_max;
        cdsc_ext->framework_cb_min = op_framework_cb_min;
        cdsc_ext->framework_cu14_max = op_framework_cu14_max;
        cdsc_ext->framework_tris_max = op_framework_tris_max;
        cdsc_ext->framework_tris_min = op_framework_tris_min;
        cdsc_ext->framework_suco_max = op_framework_suco_max;
        cdsc_ext->framework_suco_min = op_framework_suco_min;
        cdsc_ext->tool_amvr = op_tool_amvr;
        cdsc_ext->tool_mmvd = op_tool_mmvd;
        cdsc_ext->tool_affine = op_tool_affine;
        cdsc_ext->tool_dmvr = op_tool_dmvr;
        cdsc_ext->tool_addb = op_tool_addb;
        cdsc_ext->tool_alf = op_tool_alf;
        cdsc_ext->tool_admvp = op_tool_admvp;
        cdsc_ext->tool_hmvp = op_tool_hmvp;
        cdsc_ext->tool_htdf = op_tool_htdf;
        cdsc_ext->tool_eipd = op_tool_eipd;
        cdsc_ext->tool_iqt = op_tool_iqt;
        cdsc_ext->tool_cm_init = op_tool_cm_init;
        cdsc_ext->tool_adcc = op_tool_adcc;
        cdsc_ext->tool_rpl = op_tool_rpl;
        cdsc_ext->tool_pocs = op_tool_pocs;
        cdsc_ext->tool_ats = op_tool_ats;
        cdsc_ext->deblock_aplha_offset = op_deblock_alpha_offset;
        cdsc_ext->deblock_beta_offset = op_deblock_beta_offset;
        cdsc_ext->tool_dra = op_dra_enable_flag;
        cdsc_ext->tile_uniform_spacing_flag = op_tile_uniform_spacing;
        cdsc_ext->tile_columns = op_num_tile_columns;
        cdsc_ext->tile_rows = op_num_tile_rows;
        cdsc_ext->num_slice_in_pic = op_num_slice_in_pic;
        cdsc_ext->arbitrary_slice_flag = op_arbitrary_slice_flag;
        cdsc_ext->loop_filter_across_tiles_enabled_flag = op_loop_filter_across_tiles_enabled_flag;

        if (!cdsc_ext->tile_uniform_spacing_flag)
        {
            cdsc_ext->tile_column_width_array[0] = atoi(strtok(op_tile_column_width_array, " "));
            int j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                cdsc_ext->tile_column_width_array[j++] = atoi(val);
            } while (1);

            cdsc_ext->tile_row_height_array[0] = atoi(strtok(op_tile_row_height_array, " "));
            j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                cdsc_ext->tile_row_height_array[j++] = atoi(val);
            } while (1);
        }

        if (cdsc_ext->num_slice_in_pic == 1)
        {
            cdsc_ext->tile_array_in_slice[0] = 0;
            cdsc_ext->tile_array_in_slice[1] = (cdsc_ext->tile_columns * cdsc_ext->tile_rows) - 1;
            cdsc_ext->num_remaining_tiles_in_slice_minus1[0] = op_num_remaining_tiles_in_slice[0] - 1;
        }
        else /* There are more than one slice in the picture */
        {
            cdsc_ext->tile_array_in_slice[0] = atoi(strtok(op_tile_array_in_slice, " "));
            int j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                cdsc_ext->tile_array_in_slice[j++] = atoi(val);
            } while (1);

            if (cdsc_ext->arbitrary_slice_flag)
            {
                cdsc_ext->num_remaining_tiles_in_slice_minus1[0] = atoi(strtok(op_num_remaining_tiles_in_slice, " ")) - 1;
                int j = 1;
                do
                {
                    char* val = strtok(NULL, " \r");
                    if (!val)
                        break;
                    cdsc_ext->num_remaining_tiles_in_slice_minus1[j++] = atoi(val) - 1;
                } while (1);
            }
        }
        int num_tiles = cdsc_ext->tile_columns * cdsc_ext->tile_rows;
        if (num_tiles < cdsc_ext->num_slice_in_pic) result = -1;

        XEVE_CHROMA_TABLE l_chroma_qp_table;
        memset(&l_chroma_qp_table, 0, sizeof(XEVE_CHROMA_TABLE));

        l_chroma_qp_table.chroma_qp_table_present_flag = op_chroma_qp_table_present_flag;
        if (l_chroma_qp_table.chroma_qp_table_present_flag)
        {
            l_chroma_qp_table.num_points_in_qp_table_minus1[0] = atoi(strtok(op_chroma_qp_num_points_in_table, " ")) - 1;
            l_chroma_qp_table.num_points_in_qp_table_minus1[1] = atoi(strtok(NULL, " \r")) - 1;

            { /* input pivot points */
                l_chroma_qp_table.delta_qp_in_val_minus1[0][0] = atoi(strtok(op_chroma_qp_delta_in_val_cb, " "));
                int j = 1;
                do
                {
                    char* val = strtok(NULL, " \r");
                    if (!val)
                        break;
                    l_chroma_qp_table.delta_qp_in_val_minus1[0][j++] = atoi(val);
                } while (1);
                if (l_chroma_qp_table.num_points_in_qp_table_minus1[0] + 1 == j);

                l_chroma_qp_table.delta_qp_in_val_minus1[1][0] = atoi(strtok(op_chroma_qp_delta_in_val_cr, " "));
                j = 1;
                do
                {
                    char* val = strtok(NULL, " \r");
                    if (!val)
                        break;
                    l_chroma_qp_table.delta_qp_in_val_minus1[1][j++] = atoi(val);
                } while (1);
                assert(l_chroma_qp_table.num_points_in_qp_table_minus1[1] + 1 == j);
            }
            {/* output pivot points */
                l_chroma_qp_table.delta_qp_out_val[0][0] = atoi(strtok(op_chroma_qp_delta_out_val_cb, " "));
                int j = 1;
                do
                {
                    char* val = strtok(NULL, " \r");
                    if (!val)
                        break;
                    l_chroma_qp_table.delta_qp_out_val[0][j++] = atoi(val);
                } while (1);
                assert(l_chroma_qp_table.num_points_in_qp_table_minus1[0] + 1 == j);

                l_chroma_qp_table.delta_qp_out_val[1][0] = atoi(strtok(op_chroma_qp_delta_out_val_cr, " "));
                j = 1;
                do
                {
                    char* val = strtok(NULL, " \r");
                    if (!val)
                        break;
                    l_chroma_qp_table.delta_qp_out_val[1][j++] = atoi(val);
                } while (1);
                assert(l_chroma_qp_table.num_points_in_qp_table_minus1[1] + 1 == j);
            }

            memcpy(&(cdsc->chroma_qp_table_struct), &l_chroma_qp_table, sizeof(XEVE_CHROMA_TABLE));
        }

        cdsc_ext->rpl_extern = op_rpl_extern;
        if(cdsc_ext->rpl_extern)
        {
            for (int i = 0; i < MAX_NUM_RPLS && op_rpl0[i][0] != 0; ++i)
            {
                cdsc_ext->rpls_l0[i].pic_type = get_pic_type(strtok(op_rpl0[i], " "));
                cdsc_ext->rpls_l0[i].poc = atoi(strtok(NULL, " "));
                cdsc_ext->rpls_l0[i].tid = atoi(strtok(NULL, " "));
                cdsc_ext->rpls_l0[i].ref_pic_active_num = atoi(strtok(NULL, " "));

                int j = 0;
                do
                {
                    char* val = strtok(NULL, " \r");
                    if (!val)
                        break;
                    cdsc_ext->rpls_l0[i].ref_pics[j++] = atoi(val);
                } while (1);

                cdsc_ext->rpls_l0[i].ref_pic_num = j;
                ++cdsc_ext->rpls_l0_cfg_num;
            }

            for (int i = 0; i < MAX_NUM_RPLS && op_rpl1[i][0] != 0; ++i)
            {
                cdsc_ext->rpls_l0[i].pic_type = get_pic_type(strtok(op_rpl1[i], " "));
                cdsc_ext->rpls_l1[i].poc = atoi(strtok(NULL, " "));
                cdsc_ext->rpls_l1[i].tid = atoi(strtok(NULL, " "));
                cdsc_ext->rpls_l1[i].ref_pic_active_num = atoi(strtok(NULL, " "));

                int j = 0;
                do
                {
                    char* val = strtok(NULL, " ");
                    if (!val)
                        break;
                    cdsc_ext->rpls_l1[i].ref_pics[j++] = atoi(val);
                } while (1);

                cdsc_ext->rpls_l1[i].ref_pic_num = j;
                ++cdsc_ext->rpls_l1_cfg_num;
            }
        }

        if (op_dra_enable_flag)
        {
            cdsc_ext->dra_hist_norm = atof(strtok(op_dra_hist_norm, " "));
            cdsc_ext->dra_num_ranges = op_dra_number_ranges;
            cdsc_ext->dra_scale_map_y[0][0] = atoi(strtok(op_dra_range, " "));
            int j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                cdsc_ext->dra_scale_map_y[j++][0] = atoi(val);
            } while (1);
            assert(cdsc_ext->dra_num_ranges == j);

            cdsc_ext->dra_scale_map_y[0][1] = atof(strtok(op_dra_scale, " "));
            j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                cdsc_ext->dra_scale_map_y[j++][1] = atof(val);
            } while (1);
            assert(cdsc_ext->dra_num_ranges == j);

            cdsc_ext->dra_scale_map_y[cdsc_ext->dra_num_ranges][0] = 1024;
            cdsc_ext->dra_scale_map_y[cdsc_ext->dra_num_ranges][1] = cdsc_ext->dra_scale_map_y[cdsc_ext->dra_num_ranges - 1][1];

            cdsc_ext->dra_cb_qp_scale = atof(op_dra_chroma_cb_scale);
            cdsc_ext->dra_cr_qp_scale = atof(op_dra_chroma_cr_scale);
            cdsc_ext->dra_chroma_qp_scale = atof(op_dra_chroma_qp_scale);
            cdsc_ext->dra_chroma_qp_offset = atof(op_dra_chroma_qp_offset);
        }
    }

    return XEVE_OK;
}

static void print_enc_conf(XEVE_CDSC * cdsc)
{
    logv2("AMVR: %d, ",        cdsc->ext->tool_amvr);
    logv2("MMVD: %d, ",        cdsc->ext->tool_mmvd);
    logv2("AFFINE: %d, ",      cdsc->ext->tool_affine);
    logv2("DMVR: %d, ",        cdsc->ext->tool_dmvr);
    logv2("DBF.ADDB: %d.%d, ", cdsc->use_deblock, cdsc->ext->tool_addb);
    logv2("ALF: %d, ",         cdsc->ext->tool_alf);
    logv2("ADMVP: %d, ",       cdsc->ext->tool_admvp);
    logv2("HMVP: %d, ",        cdsc->ext->tool_hmvp);
    logv2("HTDF: %d ",         cdsc->ext->tool_htdf);
    logv2("EIPD: %d, ",        cdsc->ext->tool_eipd);
    logv2("IQT: %d, ",         cdsc->ext->tool_iqt);
    logv2("CM_INIT: %d, ",     cdsc->ext->tool_cm_init);
    logv2("ADCC: %d, ",        cdsc->ext->tool_adcc);
    logv2("IBC: %d, ",         cdsc->ext->ibc_flag);
    logv2("ATS: %d, ",         cdsc->ext->tool_ats);
    logv2("RPL: %d, ",         cdsc->ext->tool_rpl);
    logv2("POCS: %d, ",        cdsc->ext->tool_pocs);
    logv2("CONSTRAINED_INTRA_PRED: %d, ", cdsc->constrained_intra_pred);
    logv2("Uniform Tile Spacing: %d, ",   cdsc->ext->tile_uniform_spacing_flag);
    logv2("Number of Tile Columns: %d, ", cdsc->ext->tile_columns);
    logv2("Number of Tile  Rows: %d, ",   cdsc->ext->tile_rows);
    logv2("Number of Slices: %d, ",       cdsc->ext->num_slice_in_pic);
    logv2("Loop Filter Across Tile Enabled: %d, ", cdsc->ext->loop_filter_across_tiles_enabled_flag);
    logv2("ChromaQPTable: %d, ",                   cdsc->chroma_qp_table_struct.chroma_qp_table_present_flag);
    logv2("DRA: %d ",                              cdsc->ext->tool_dra);
    logv2("\n");
}

int check_conf(XEVE_CDSC* cdsc)
{
    int success = 1;
    int min_block_size = 4;
    if(cdsc->profile == 0 /*PROFILE_BASELINE*/ && cdsc->ext != NULL)
    {
        if (cdsc->ext->tool_amvr    == 1) { logv0("AMVR cannot be on in base profile\n"); success = 0; }
        if (cdsc->ext->tool_mmvd    == 1) { logv0("MMVD cannot be on in base profile\n"); success = 0; }
        if (cdsc->ext->tool_affine  == 1) { logv0("Affine cannot be on in base profile\n"); success = 0; }
        if (cdsc->ext->tool_dmvr    == 1) { logv0("DMVR cannot be on in base profile\n"); success = 0; }
        if (cdsc->ext->tool_admvp   == 1) { logv0("ADMVP cannot be on in base profile\n"); success = 0; }
        if (cdsc->ext->tool_hmvp    == 1) { logv0("HMVP cannot be on in base profile\n"); success = 0; }
        if (cdsc->ext->tool_addb    == 1) { logv0("ADDB cannot be on in base profile\n"); success = 0; }
        if (cdsc->ext->tool_alf     == 1) { logv0("ALF cannot be on in base profile\n"); success = 0; }
        if (cdsc->ext->tool_htdf    == 1) { logv0("HTDF cannot be on in base profile\n"); success = 0; }
        if (cdsc->ext->btt          == 1) { logv0("BTT cannot be on in base profile\n"); success = 0; }
        if (cdsc->ext->suco         == 1) { logv0("SUCO cannot be on in base profile\n"); success = 0; }
        if (cdsc->ext->tool_eipd    == 1) { logv0("EIPD cannot be on in base profile\n"); success = 0; }
        if (cdsc->ext->tool_iqt     == 1) { logv0("IQT cannot be on in base profile\n"); success = 0; }
        if (cdsc->ext->tool_cm_init == 1) { logv0("CM_INIT cannot be on in base profile\n"); success = 0; }
        if (cdsc->ext->tool_adcc    == 1) { logv0("ADCC cannot be on in base profile\n"); success = 0; }
        if (cdsc->ext->tool_ats     == 1) { logv0("ATS_INTRA cannot be on in base profile\n"); success = 0; }
        if (cdsc->ext->ibc_flag     == 1) { logv0("IBC cannot be on in base profile\n"); success = 0; }
        if (cdsc->ext->tool_rpl     == 1) { logv0("RPL cannot be on in base profile\n"); success = 0; }
        if (cdsc->ext->tool_pocs    == 1) { logv0("POCS cannot be on in base profile\n"); success = 0; }
    }
    else
    {
        if (cdsc->ext->tool_admvp   == 0 && cdsc->ext->tool_affine == 1) { logv0("AFFINE cannot be on when ADMVP is off\n"); success = 0; }
        if (cdsc->ext->tool_admvp   == 0 && cdsc->ext->tool_amvr   == 1) { logv0("AMVR cannot be on when ADMVP is off\n"); success = 0; }
        if (cdsc->ext->tool_admvp   == 0 && cdsc->ext->tool_dmvr   == 1) { logv0("DMVR cannot be on when ADMVP is off\n"); success = 0; }
        if (cdsc->ext->tool_admvp   == 0 && cdsc->ext->tool_mmvd   == 1) { logv0("MMVD cannot be on when ADMVP is off\n"); success = 0; }
        if (cdsc->ext->tool_eipd    == 0 && cdsc->ext->ibc_flag    == 1) { logv0("IBC cannot be on when EIPD is off\n"); success = 0; }
        if (cdsc->ext->tool_iqt     == 0 && cdsc->ext->tool_ats    == 1) { logv0("ATS cannot be on when IQT is off\n"); success = 0; }
        if (cdsc->ext->tool_cm_init == 0 && cdsc->ext->tool_adcc   == 1) { logv0("ADCC cannot be on when CM_INIT is off\n"); success = 0; }
        if (cdsc->ext->tool_eipd    == 0) { logv0("EIPD cannot be off in main profile\n"); success = 0; }
        if (cdsc->ext->tool_iqt     == 0) { logv0("IQT cannot be off in main profile\n"); success = 0; }
        if (cdsc->ext->tool_cm_init == 0) { logv0("CM_INIT cannot be off in main profile\n"); success = 0; }
    }

    if (cdsc->ext->btt == 1)
    {
        if (cdsc->ext->framework_cb_max && cdsc->ext->framework_cb_max < 5) { logv0("Maximun Coding Block size cannot be smaller than 5\n"); success = 0; }
        if (cdsc->ext->framework_cb_max > 7) { logv0("Maximun Coding Block size cannot be greater than 7\n"); success = 0; }
        if (cdsc->ext->framework_cb_min && cdsc->ext->framework_cb_min < 2) { logv0("Minimum Coding Block size cannot be smaller than 2\n"); success = 0; }
        if ((cdsc->ext->framework_cb_max || cdsc->ext->framework_cb_min) &&
            cdsc->ext->framework_cb_min > cdsc->ext->framework_cb_max) { logv0("Minimum Coding Block size cannot be greater than Maximum coding Block size\n"); success = 0; }
        if (cdsc->ext->framework_cu14_max > 6) { logv0("Maximun 1:4 Coding Block size cannot be greater than 6\n"); success = 0; }
        if ((cdsc->ext->framework_cb_max || cdsc->ext->framework_cu14_max) &&
            cdsc->ext->framework_cu14_max > cdsc->ext->framework_cb_max) { logv0("Maximun 1:4 Coding Block size cannot be greater than Maximum coding Block size\n"); success = 0; }
        if (cdsc->ext->framework_tris_max > 6) { logv0("Maximun Tri-split Block size be greater than 6\n"); success = 0; }
        if ((cdsc->ext->framework_tris_max || cdsc->ext->framework_cb_max) &&
            cdsc->ext->framework_tris_max > cdsc->ext->framework_cb_max) { logv0("Maximun Tri-split Block size cannot be greater than Maximum coding Block size\n"); success = 0; }
        if ((cdsc->ext->framework_tris_min || cdsc->ext->framework_cb_min) &&
            cdsc->ext->framework_tris_min < cdsc->ext->framework_cb_min + 2) { logv0("Maximun Tri-split Block size cannot be smaller than Minimum Coding Block size plus two\n"); success = 0; }
        if(cdsc->ext->framework_cb_min) min_block_size = 1 << cdsc->ext->framework_cb_min;
        else min_block_size = 8;
    }

    if (cdsc->ext->suco == 1)
    {
        if (cdsc->ext->framework_suco_max > 6) { logv0("Maximun SUCO size cannot be greater than 6\n"); success = 0; }
        if (cdsc->ext->framework_cb_max && cdsc->ext->framework_suco_max > cdsc->ext->framework_cb_max) { logv0("Maximun SUCO size cannot be greater than Maximum coding Block size\n"); success = 0; }
        if (cdsc->ext->framework_suco_min < 4) { logv0("Minimun SUCO size cannot be smaller than 4\n"); success = 0; }
        if (cdsc->ext->framework_cb_min && cdsc->ext->framework_suco_min < cdsc->ext->framework_cb_min) { logv0("Minimun SUCO size cannot be smaller than Minimum coding Block size\n"); success = 0; }
        if (cdsc->ext->framework_suco_min > cdsc->ext->framework_suco_max) { logv0("Minimum SUCO size cannot be greater than Maximum SUCO size\n"); success = 0; }
    }

    int pic_m = (8 > min_block_size) ? min_block_size : 8;
    if ((cdsc->w & (pic_m - 1)) != 0) { logv0("Current encoder does not support picture width, not multiple of max(8, minimum CU size)\n"); success = 0; }
    if ((cdsc->h & (pic_m - 1)) != 0) { logv0("Current encoder does not support picture height, not multiple of max(8, minimum CU size)\n"); success = 0; }

    return success;
}

static int set_extra_config(XEVE id)
{
    int  ret, size, value;

    if(op_use_pic_signature)
    {
        value = 1;
        size = 4;
        ret = xeve_config(id, XEVE_CFG_SET_USE_PIC_SIGNATURE, &value, &size);
        if(XEVE_FAILED(ret))
        {
            logerr("failed to set config for picture signature\n");
            return -1;
        }
    }

    return 0;
}

static void print_stat_init(void)
{
    if(op_verbose < VERBOSE_FRAME) return;

    logv2("---------------------------------------------------------------------------------------\n");
    logv2("  Input YUV file          : %s \n", op_fname_inp);
    if(op_flag[OP_FLAG_FNAME_OUT])
    {
        logv2("  Output XEVE bitstream   : %s \n", op_fname_out);
    }
    if(op_flag[OP_FLAG_FNAME_REC])
    {
        logv2("  Output YUV file         : %s \n", op_fname_rec);
    }
    if (op_inp_bit_depth == 8 && op_out_bit_depth != 8)
    {
        logv2("  PSNR is calculated as 10-bit (Input YUV bitdepth: %d)\n", op_inp_bit_depth);
    }
    logv2("---------------------------------------------------------------------------------------\n");

    logv2("POC   Tid   Ftype   QP   PSNR-Y    PSNR-U    PSNR-V    Bits      EncT(ms)  ");
    logv2("Ref. List\n");

    logv2("---------------------------------------------------------------------------------------\n");
}

static void print_config(XEVE id)
{
    int s, v;

    if(op_verbose < VERBOSE_FRAME) return;

    logv2("---------------------------------------------------------------------------------------\n");
    logv2("< Configurations >\n");
    if(op_flag[OP_FLAG_FNAME_CFG])
    {
    logv2("\tconfig file name         = %s\n", op_fname_cfg);
    }
    s = sizeof(int);
    xeve_config(id, XEVE_CFG_GET_WIDTH, (void *)(&v), &s);
    logv2("\twidth                    = %d\n", v);
    xeve_config(id, XEVE_CFG_GET_HEIGHT, (void *)(&v), &s);
    logv2("\theight                   = %d\n", v);
    xeve_config(id, XEVE_CFG_GET_FPS, (void *)(&v), &s);
    logv2("\tFPS                      = %d\n", v);
    xeve_config(id, XEVE_CFG_GET_I_PERIOD, (void *)(&v), &s);
    logv2("\tintra picture period     = %d\n", v);
    xeve_config(id, XEVE_CFG_GET_QP, (void *)(&v), &s);
    logv2("\tQP                       = %d\n", v);

    logv2("\tframes                   = %d\n", op_max_frm_num);
    xeve_config(id, XEVE_CFG_GET_USE_DEBLOCK, (void *)(&v), &s);
    logv2("\tdeblocking filter        = %s\n", v? "enabled": "disabled");
    xeve_config(id, XEVE_CFG_GET_CLOSED_GOP, (void *)(&v), &s);
    logv2("\tGOP type                 = %s\n", v? "closed": "open");

    xeve_config(id, XEVE_CFG_GET_HIERARCHICAL_GOP, (void *)(&v), &s);
    logv2("\thierarchical GOP         = %s\n", v? "enabled": "disabled");

    if(op_flag[OP_RC_TYPE])
    {
    logv2("\tBit_Rate                 = %d\n", op_bps);
    }
}

static int write_rec(IMGB_LIST *list, XEVE_MTIME ts)
{
    int i;

    for(i=0; i<MAX_BUMP_FRM_CNT; i++)
    {
        if(list[i].ts == ts && list[i].used == 1)
        {
            if(op_flag[OP_FLAG_FNAME_REC])
            {
                if(imgb_write(op_fname_rec, list[i].imgb))
                {
                    logerr("cannot write reconstruction image\n");
                    return XEVE_ERR;
                }
            }
            return XEVE_OK;
        }
    }
    return XEVE_OK_FRM_DELAYED;
}

void print_psnr(XEVE_STAT * stat, double * psnr, int bitrate, XEVE_CLK clk_end)
{
    char  stype;
    int i, j;
    switch(stat->stype)
    {
    case XEVE_ST_I :
        stype = 'I';
        break;

    case XEVE_ST_P :
        stype = 'P';
        break;

    case XEVE_ST_B :
        stype = 'B';
        break;

    case XEVE_ST_UNKNOWN :
    default :
        stype = 'U';
        break;
    }

    logv2("%-7d%-5d(%c)     %-5d%-10.4f%-10.4f%-10.4f%-10d%-10d", \
        stat->poc, stat->tid, stype, stat->qp, psnr[0], psnr[1], psnr[2], \
        bitrate, xeve_clk_msec(clk_end));

    for(i=0; i < 2; i++)
    {
        logv2("[L%d ", i);
        for(j=0; j < stat->refpic_num[i]; j++) logv2("%d ",stat->refpic[i][j]);
        logv2("] ");
    }

    logv2("\n");

    fflush(stdout);
    fflush(stderr);
}

int setup_bumping(XEVE id)
{
    int val, size;

    logv2("Entering bumping process...\n");
    val  = 1;
    size = sizeof(int);
    if(XEVE_FAILED(xeve_config(id, XEVE_CFG_SET_FORCE_OUT, (void *)(&val), &size)))
    {
        logv0("failed to fource output\n");
        return -1;
    }
    return 0;
}

int main(int argc, const char **argv)
{
    STATES             state = STATE_ENCODING;
    unsigned char    * bs_buf = NULL;
    FILE             * fp_inp = NULL;
    XEVE               id;
    XEVE_CDSC          cdsc;
    XEVE_BITB          bitb;
    XEVE_IMGB        * imgb_rec = NULL;
    XEVE_STAT          stat;
    int                i, ret, size;
    XEVE_CLK           clk_beg, clk_end, clk_tot;
    XEVE_MTIME         pic_icnt, pic_ocnt, pic_skip;
    double             bitrate;
    double             psnr[3] = { 0, };
    double             psnr_avg[3] = { 0, };
    int                encod_frames = 0;
    IMGB_LIST          ilist_org[MAX_BUMP_FRM_CNT];
    IMGB_LIST          ilist_rec[MAX_BUMP_FRM_CNT];
    IMGB_LIST        * ilist_t = NULL;
    static int         is_first_enc = 1;

    /* parse options */
    ret = args_parse_all(argc, argv, options);
    if(ret != 0)
    {
        if(ret > 0) logerr("-%c argument should be set\n", ret);
        if(ret < 0) logerr("config error\n");
        print_usage();
        return -1;
    }
    logv1("eXtra-fast Essential Video Encoder\n");

    if(op_flag[OP_FLAG_FNAME_OUT])
    {
        /* bitstream file - remove contents and close */
        FILE * fp;
        fp = fopen(op_fname_out, "wb");
        if(fp == NULL)
        {
            logerr("cannot open bitstream file (%s)\n", op_fname_out);
            return -1;
        }
        fclose(fp);
    }

    if(op_flag[OP_FLAG_FNAME_REC])
    {
        /* reconstruction file - remove contents and close */
        FILE * fp;
        fp = fopen(op_fname_rec, "wb");
        if(fp == NULL)
        {
            logerr("cannot open reconstruction file (%s)\n", op_fname_rec);
            return -1;
        }
        fclose(fp);
    }

    /* open input file */
    fp_inp = fopen(op_fname_inp, "rb");
    if(fp_inp == NULL)
    {
        logerr("cannot open original file (%s)\n", op_fname_inp);
        print_usage();
        return -1;
    }

    /* allocate bitstream buffer */
    bs_buf = (unsigned char*)malloc(MAX_BS_BUF);
    if(bs_buf == NULL)
    {
        logerr("cannot allocate bitstream buffer, size=%d", MAX_BS_BUF);
        return -1;
    }

    /* read configurations and set values for create descriptor */
    int val = get_conf(&cdsc);
    if (val != XEVE_OK)
    {
        print_usage();
        return -1;
    }

    print_enc_conf(&cdsc);

    if (!check_conf(&cdsc))
    {
        logv0("invalid configuration\n");
        return -1;
    }

    /* create encoder */
    id = xeve_create(&cdsc, NULL);
    if(id == NULL)
    {
        logv0("cannot create XEVE encoder\n");
        return -1;
    }

    if(set_extra_config(id))
    {
        logv0("cannot set extra configurations\n");
        return -1;
    }

    /* create image lists */
    if(imgb_list_alloc(ilist_org, cdsc.w, cdsc.h, op_inp_bit_depth))
    {
        logv0("cannot allocate image list for original image\n");
        return -1;
    }
    if(imgb_list_alloc(ilist_rec, cdsc.w, cdsc.h, op_out_bit_depth))
    {
        logv0("cannot allocate image list for reconstructed image\n");
        return -1;
    }

    print_config(id);
    print_stat_init();

    bitrate = 0;
    bitb.addr = bs_buf;
    bitb.bsize = MAX_BS_BUF;

    if(op_flag[OP_FLAG_SKIP_FRAMES] && op_skip_frames > 0)
    {
        state = STATE_SKIPPING;
    }

    clk_tot = 0;
    pic_icnt = 0;
    pic_ocnt = 0;
    pic_skip = 0;

    /* encode pictures *******************************************************/
    while(1)
    {
        if(state == STATE_SKIPPING)
        {
            if(pic_skip < op_skip_frames)
            {
                ilist_t = imgb_list_get_empty(ilist_org);
                if(ilist_t == NULL)
                {
                    logerr("cannot get empty orignal buffer\n");
                    goto ERR;
                }
                if(imgb_read(fp_inp, ilist_t->imgb))
                {
                    logv2("reached end of original file (or reading error)\n");
                    goto ERR;
                }
            }
            else
            {
                state = STATE_ENCODING;
            }

            pic_skip++;
            continue;
        }

        if(state == STATE_ENCODING)
        {
            ilist_t = imgb_list_get_empty(ilist_org);
            if(ilist_t == NULL)
            {
                logerr("cannot get empty orignal buffer\n");
                return -1;
            }

            /* read original image */
            if ((op_max_frm_num && pic_icnt >= op_max_frm_num) || imgb_read(fp_inp, ilist_t->imgb))
            {
                logv2("reached end of original file (or reading error)\n");
                state = STATE_BUMPING;
                setup_bumping(id);
                continue;
            }
            imgb_list_make_used(ilist_t, pic_icnt);

            /* push image to encoder */
            ret = xeve_push(id, ilist_t->imgb);
            if(XEVE_FAILED(ret))
            {
                logv0("xeve_push() failed\n");
                return -1;
            }
            pic_icnt++;
        }
        /* encoding */
        clk_beg = xeve_clk_get();

        ret = xeve_encode(id, &bitb, &stat);
        if(XEVE_FAILED(ret))
        {
            logv0("xeve_encode() failed\n");
            return -1;
        }

        clk_end = xeve_clk_from(clk_beg);
        clk_tot += clk_end;

        /* store bitstream */
        if (ret == XEVE_OK_OUT_NOT_AVAILABLE)
        {
            /* logv2("--> RETURN OK BUT PICTURE IS NOT AVAILABLE YET\n"); */
            continue;
        }
        else if(ret == XEVE_OK)
        {
            if(op_flag[OP_FLAG_FNAME_OUT] && stat.write > 0)
            {
                if(write_data(op_fname_out, bs_buf, stat.write))
                {
                    logv0("cannot write bitstream\n");
                    return -1;
                }
            }

            /* get reconstructed image */
            size = sizeof(XEVE_IMGB**);
            ret = xeve_config(id, XEVE_CFG_GET_RECON, (void *)&imgb_rec, &size);
            if(XEVE_FAILED(ret))
            {
                logv0("failed to get reconstruction image\n");
                return -1;
            }

            /* store reconstructed image to list */
            ilist_t = imgb_list_put(ilist_rec, imgb_rec, imgb_rec->ts[0]);
            if(ilist_t == NULL)
            {
                logv0("cannot put reconstructed image to list\n");
                return -1;
            }

            /* calculate PSNR */
            if (op_verbose == VERBOSE_FRAME)
            {
                if(cal_psnr(ilist_org, ilist_t->imgb, ilist_t->ts,
                    op_inp_bit_depth, op_out_bit_depth, psnr))
                {
                    logv0("cannot calculate PSNR\n");
                    return -1;
                }
                if (is_first_enc)
                {
                    print_psnr(&stat, psnr, (stat.write - stat.sei_size + (int)bitrate) << 3, clk_end);
                    is_first_enc = 0;
                }
                else
                {
                    print_psnr(&stat, psnr, (stat.write - stat.sei_size) << 3, clk_end);
                }
                for (i = 0; i < 3; i++) psnr_avg[i] += psnr[i];
            }
            /* release original image */
            imgb_list_make_unused(ilist_org, ilist_t->ts);
            ret = write_rec(ilist_rec, pic_ocnt);
            if (ret == XEVE_ERR)
            {
                logv0("cannot write reconstruction image\n");
                return -1;
            }
            else if (ret == XEVE_OK)
            {
            /* release recon image */
            imgb_list_make_unused(ilist_rec, pic_ocnt);
            pic_ocnt++;
            }
            bitrate += (stat.write - stat.sei_size);

            if (op_verbose >= VERBOSE_SIMPLE) {
                int total_time = ((int)xeve_clk_msec(clk_tot) / 1000);
                int h = total_time / 3600;
                total_time = total_time % 3600;
                int m = total_time / 60;
                total_time = total_time % 60;
                int s = total_time;
                double curr_bitrate = bitrate;
                curr_bitrate *= (cdsc.fps * 8);
                curr_bitrate /= (encod_frames + 1);
                curr_bitrate /= 1000;
                logv1("[ %d / %d frames ] [ %.2f frame/sec ] [ %.4f kbps ] [ %2dh %2dm %2ds ] \r"
                       , encod_frames, op_max_frm_num, ((float)(encod_frames + 1) * 1000) / ((float)xeve_clk_msec(clk_tot))
                       , curr_bitrate, h, m, s);
                fflush(stdout);
                encod_frames++;
            }

            /* release recon buffer */
            if (imgb_rec)
            {
                imgb_rec->release(imgb_rec);
            }
        }
        else if (ret == XEVE_OK_NO_MORE_FRM)
        {
            break;
        }
        else
        {
            logv2("invaild return value (%d)\n", ret);
            return -1;
        }

        if(op_flag[OP_FLAG_MAX_FRM_NUM] && pic_icnt >= op_max_frm_num
            && state == STATE_ENCODING)
        {
            state = STATE_BUMPING;
            setup_bumping(id);
        }
    }

    /* store remained reconstructed pictures in output list */
    while(pic_icnt - pic_ocnt > 0)
    {
        ret = write_rec(ilist_rec, pic_ocnt);
        if (ret == XEVE_ERR)
        {
            logv0("cannot write reconstruction image\n");
            return -1;
        }
        else if (ret == XEVE_OK)
        {
        /* release recon image */
        imgb_list_make_unused(ilist_rec, pic_ocnt);
        pic_ocnt++;
        }
    }
    if(pic_icnt != pic_ocnt)
    {
        logv2("number of input(=%d) and output(=%d) is not matched\n", (int)pic_icnt, (int)pic_ocnt);
    }

    logv1_line("Summary");
    psnr_avg[0] /= pic_ocnt;
    psnr_avg[1] /= pic_ocnt;
    psnr_avg[2] /= pic_ocnt;

    logv2("  PSNR Y(dB)       : %-5.4f\n", psnr_avg[0]);
    logv2("  PSNR U(dB)       : %-5.4f\n", psnr_avg[1]);
    logv2("  PSNR V(dB)       : %-5.4f\n", psnr_avg[2]);
    logv2("  Total bits(bits) : %.0f\n", bitrate * 8);
    bitrate *= (cdsc.fps * 8);
    bitrate /= pic_ocnt;
    bitrate /= 1000;

    logv2("  Labeles          : br,kbps\tPSNR,Y\tPSNR,U\tPSNR,V\t\n");
    logv2("  Summary          : %-5.4f\t%-5.4f\t%-5.4f\t%-5.4f\n", bitrate, psnr_avg[0], psnr_avg[1], psnr_avg[2]);

    logv1("Bitrate                           = %.4f kbps\n", bitrate);
    logv1("Encoded frame count               = %d\n", (int)pic_ocnt);
    logv1("Total encoding time               = %.3f msec,",
        (float)xeve_clk_msec(clk_tot));
    logv1(" %.3f sec\n", (float)(xeve_clk_msec(clk_tot)/1000.0));

    logv1("Average encoding time for a frame = %.3f msec\n",
        (float)xeve_clk_msec(clk_tot)/pic_ocnt);
    logv1("Average encoding speed            = %.3f frames/sec\n",
        ((float)pic_ocnt * 1000) / ((float)xeve_clk_msec(clk_tot)));
    logv1_line(NULL);

    if (pic_ocnt != op_max_frm_num)
    {
        logv2("Wrong frames count: should be %d was %d\n", op_max_frm_num, (int)pic_ocnt);
    }

ERR:
    xeve_delete(id);

    imgb_list_free(ilist_org);
    imgb_list_free(ilist_rec);

    if(fp_inp) fclose(fp_inp);
    if(bs_buf) free(bs_buf); /* release bitstream buffer */
    return 0;
}


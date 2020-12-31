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

#ifndef _XEVE_APP_ARGS_H_
#define _XEVE_APP_ARGS_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ARGS_VAL_TYPE_MANDATORY       (1<<0) /* mandatory or not */
#define ARGS_VAL_TYPE_NONE            (0<<1) /* no value */
#define ARGS_VAL_TYPE_INTEGER         (10<<1) /* integer type value */
#define ARGS_VAL_TYPE_STRING          (20<<1) /* string type value */
#define ARGS_GET_CMD_OPT_VAL_TYPE(x)  ((x) & ~ARGS_VAL_TYPE_MANDATORY)
#define ARGS_NO_KEY                   (127)
#define ARGS_KEY_LONG_CONFIG          "config"
#define ARGS_MAX_NUM_CONF_FILES       (16)

typedef struct _ARGS_OPTION
{
    char   key; /* option keyword. ex) -f */
    char   key_long[32]; /* option long keyword, ex) --file */
    int    val_type; /* value type */
    int  * flag; /* flag to setting or not */
    void * val; /* actual value */
    char   desc[512]; /* description of option */
} ARGS_OPTION;

static int args_search_long_arg(ARGS_OPTION * opts, const char * argv)
{
    int oidx = 0;
    ARGS_OPTION * o;

    o = opts;

    while(o->key != 0)
    {
        if(!strcmp(argv, o->key_long))
        {
            return oidx;
        }
        oidx++;
        o++;
    }
    return -1;
}


static int args_search_short_arg(ARGS_OPTION * ops, const char argv)
{
    int oidx = 0;
    ARGS_OPTION * o;

    o = ops;

    while(o->key != 0)
    {
        if(o->key != ARGS_NO_KEY && o->key == argv)
        {
            return oidx;
        }
        oidx++;
        o++;
    }
    return -1;
}

static int args_read_value(ARGS_OPTION * ops, const char * argv)
{
    if(argv == NULL) return -1;
    if(argv[0] == '-' && (argv[1] < '0' || argv[1] > '9')) return -1;

    switch(ARGS_GET_CMD_OPT_VAL_TYPE(ops->val_type))
    {
        case ARGS_VAL_TYPE_INTEGER:
            *((int*)ops->val) = atoi(argv);
            break;

        case ARGS_VAL_TYPE_STRING:
            strcpy((char*)ops->val, argv);
            break;

        default:
            return -1;
    }
    return 0;
}

static int args_get_help(ARGS_OPTION * ops, int idx, char * help)
{
    int optional = 0;
    char vtype[32];
    ARGS_OPTION * o = ops + idx;

    switch(ARGS_GET_CMD_OPT_VAL_TYPE(o->val_type))
    {
        case ARGS_VAL_TYPE_INTEGER:
            strcpy(vtype, "INTEGER");
            break;
        case ARGS_VAL_TYPE_STRING:
            strcpy(vtype, "STRING");
            break;
        case ARGS_VAL_TYPE_NONE:
        default:
            strcpy(vtype, "FLAG");
            break;
    }
    optional = !(o->val_type & ARGS_VAL_TYPE_MANDATORY);

    if(o->key != ARGS_NO_KEY)
    {
        sprintf(help, "  -%c, --%s [%s]%s\n    : %s", o->key, o->key_long,
                vtype, (optional) ? " (optional)" : "", o->desc);
    }
    else
    {
        sprintf(help, "  --%s [%s]%s\n    : %s", o->key_long,
                vtype, (optional) ? " (optional)" : "", o->desc);
    }
    return 0;
}

static int args_get_arg(ARGS_OPTION * ops, int idx, char * result)
{
    char vtype[32];
    char value[512];
    ARGS_OPTION * o = ops + idx;

    switch(ARGS_GET_CMD_OPT_VAL_TYPE(o->val_type))
    {
        case ARGS_VAL_TYPE_INTEGER:
            strcpy(vtype, "INTEGER");
            sprintf(value, "%d", *((int*)o->val));
            break;

        case ARGS_VAL_TYPE_STRING:
            strcpy(vtype, "STRING");
            sprintf(value, "%s", (char*)o->val);
            break;

        case ARGS_VAL_TYPE_NONE:
        default:
            strcpy(vtype, "FLAG");
            sprintf(value, "%d", *((int*)o->val));
            break;
    }

    if(o->flag != NULL && (*o->flag))
    {
        strcat(value, " (SET)");
    }
    else
    {
        strcat(value, " (DEFAULT)");
    }

    sprintf(result, "  -%c(--%s) = %s\n    : %s", o->key, o->key_long,
            value, o->desc);

    return 0;

}

static int args_parse_cfg(FILE * fp, ARGS_OPTION * ops)
{
    char * parser;
    char line[256] = "", tag[50] = "", val[256] = "";
    int oidx;

    while(fgets(line, sizeof(line), fp))
    {
        parser = strchr(line, '#');
        if(parser != NULL) *parser = '\0';

        parser = strtok(line, "= \t");
        if(parser == NULL) continue;
        strcpy(tag, parser);

        parser = strtok(NULL, "=\n");
        if(parser == NULL) continue;
        strcpy(val, parser);

        oidx = args_search_long_arg(ops, tag);
        if(oidx < 0) continue;

        if(ARGS_GET_CMD_OPT_VAL_TYPE(ops[oidx].val_type) !=
           ARGS_VAL_TYPE_NONE)
        {
            if(args_read_value(ops + oidx, val)) continue;
        }
        else
        {
            *((int*)ops[oidx].val) = 1;
        }
        *ops[oidx].flag = 1;
    }
    return 0;
}


static int args_parse_cmd(int argc, const char * argv[], ARGS_OPTION * ops,
                          int * idx)
{
    int    aidx; /* arg index */
    int    oidx; /* option index */

    aidx = *idx + 1;

    if(aidx >= argc || argv[aidx] == NULL) goto NO_MORE;
    if(argv[aidx][0] != '-') goto ERR;

    if(argv[aidx][1] == '-')
    {
        /* long option */
        oidx = args_search_long_arg(ops, argv[aidx] + 2);
        if(oidx < 0) goto ERR;
    }
    else if(strlen(argv[aidx]) == 2)
    {
        /* short option */
        oidx = args_search_short_arg(ops, argv[aidx][1]);
        if(oidx < 0) goto ERR;
    }
    else
    {
        goto ERR;
    }

    if(ARGS_GET_CMD_OPT_VAL_TYPE(ops[oidx].val_type) !=
       ARGS_VAL_TYPE_NONE)
    {
        if(aidx + 1 >= argc) goto ERR;
        if(args_read_value(ops + oidx, argv[aidx + 1])) goto ERR;
        *idx = *idx + 1;
    }
    else
    {
        *((int*)ops[oidx].val) = 1;
    }
    *ops[oidx].flag = 1;
    *idx = *idx + 1;

    return ops[oidx].key;


NO_MORE:
    return 0;

ERR:
    return -1;
}

static int args_parse_all(int argc, const char * argv[],
                               ARGS_OPTION * ops)
{
    int i, ret = 0, idx = 0;
    ARGS_OPTION *o;
    const char *fname_cfg = NULL;
    FILE *fp;

    int num_configs = 0;
    int pos_conf_files[ARGS_MAX_NUM_CONF_FILES];
    memset(&pos_conf_files, -1, sizeof(int) * ARGS_MAX_NUM_CONF_FILES);
    /* config file parsing */
    for(i = 1; i < argc; i++)
    {
        if(!strcmp(argv[i], "--"ARGS_KEY_LONG_CONFIG))
        {
            if(i + 1 < argc)
            {
                num_configs++;
                pos_conf_files[num_configs - 1] = i + 1;
            }
        }
    }
    for (int i = 0; i < num_configs; i++)
    {
        fname_cfg = argv[pos_conf_files[i]];
        if(fname_cfg)
        {
            fp = fopen(fname_cfg, "r");
            if(fp == NULL) return -1; /* config file error */

            if(args_parse_cfg(fp, ops))
            {
                fclose(fp);
                return -1; /* config file error */
            }
            fclose(fp);
        }
    }
    /* command line parsing */
    while(1)
    {
        ret = args_parse_cmd(argc, argv, ops, &idx);
        if(ret <= 0) break;
    }

    /* check mandatory argument */
    o = ops;

    while(o->key != 0)
    {
        if(o->val_type & ARGS_VAL_TYPE_MANDATORY)
        {
            if(*o->flag == 0)
            {
                /* not filled all mandatory argument */
                return o->key;
            }
        }
        o++;
    }
    return ret;
}

static int args_parse_int_x_int(char * str, int * num0, int * num1)
{
    char str0_t[64];
    int i, cnt0, cnt1;
    char * str0, *str1 = NULL;

    str0 = str;
    cnt1 = (int)strlen(str);

    /* find 'x' */
    for(i = 0; i < (int)strlen(str); i++)
    {
        if(str[i] == 'x' || str[i] == 'X')
        {
            str1 = str + i + 1;
            cnt0 = i;
            cnt1 = cnt1 - cnt0 - 1;
            break;
        }
    }

    /* check malformed data */
    if(str1 == NULL || cnt0 == 0 || cnt1 == 0) return -1;

    for(i = 0; i < cnt0; i++)
    {
        if(str0[i] < 0x30 || str0[i] > 0x39) return -1; /* not a number */
    }
    for(i = 0; i < cnt1; i++)
    {
        if(str1[i] < 0x30 || str1[i] > 0x39) return -1; /* not a number */
    }


    strncpy(str0_t, str0, cnt0);
    str0_t[cnt0] = '\0';

    *num0 = atoi(str0_t);
    *num1 = atoi(str1);

    return 0;
}

/*
 * Define various command line options for application
 */

static char op_fname_cfg[256]     = "\0"; /* config file path name */
static char op_fname_inp[256]     = "\0"; /* input original video */
static char op_fname_out[256]     = "\0"; /* output bitstream */
static char op_fname_rec[256]     = "\0"; /* reconstructed video */
static int  op_max_frm_num        = 0;
static int  op_use_pic_signature  = 0;
static int  op_w                  = 0;
static int  op_h                  = 0;
static int  op_qp                 = 32;
static int  op_fps                = 0;
static int  op_iperiod            = 0;
static int  op_max_b_frames       = 15;
static int  op_ref_pic_gap_length = 0;
static int  op_closed_gop         = 0;
static int  op_enable_ibc         = 0;
static int  op_ibc_search_range_x = 64;
static int  op_ibc_search_range_y = 64;
static int  op_ibc_hash_search_flag = 0;
static int  op_ibc_hash_search_max_cand = 64;
static int  op_ibc_hash_search_range_4smallblk = 64;
static int  op_ibc_fast_method    = 0X02;
static int  op_disable_hgop       = 0;
static int  op_skip_frames        = 0;
static int  op_inp_bit_depth      = 8;
static int  op_out_bit_depth      = 0; /* same as input bit depth */
static int  op_codec_bit_depth    = 10;
static int  op_rdo_dbk_switch     = 1;
static char op_profile[16]        = "baseline";
static int  op_level              = 0;
static int  op_btt                = 1;
static int  op_suco               = 1;
static int  op_add_qp_frames      = 0;
static int  op_framework_cb_max   = 0;
static int  op_framework_cb_min   = 0;
static int  op_framework_cu14_max = 0;
static int  op_framework_tris_max = 0;
static int  op_framework_tris_min = 0;
static int  op_framework_suco_max = 6;
static int  op_framework_suco_min = 4;
static int  op_tool_amvr          = 1; /* default on */
static int  op_tool_mmvd          = 1; /* default on */
static int  op_tool_affine        = 1; /* default on */
static int  op_tool_dmvr          = 1; /* default on */
static int  op_tool_addb          = 1; /* default on */
static int  op_tool_alf           = 1; /* default on */
static int  op_tool_admvp         = 1; /* default on */
static int  op_tool_hmvp = 1; /* default on */
static int  op_tool_htdf          = 1; /* default on */
static int  op_tool_eipd          = 1; /* default on */
static int  op_tool_iqt           = 1; /* default on */
static int  op_tool_cm_init       = 1; /* default on */
static int  op_tool_adcc          = 1; /* default on */
static int  op_tool_rpl           = 1; /* default on */
static int  op_tool_pocs          = 1; /* default on */
static int  op_cb_qp_offset       = 0;
static int  op_cr_qp_offset       = 0;
static int  op_tool_ats           = 1; /* default on */
static int  op_constrained_intra_pred = 0;
static int  op_tool_deblocking        = 1; /* default on */
static int  op_deblock_alpha_offset   = 0; /* default offset 0 */
static int  op_deblock_beta_offset    = 0; /* default offset 0 */

static int  op_tile_uniform_spacing = 1;
static int  op_num_tile_columns     = 1;          /* default 1 */
static int  op_num_tile_rows        = 1;          /* default 1 */
static char op_tile_column_width_array[MAX_NUM_TILES_COL];
static char op_tile_row_height_array[MAX_NUM_TILES_ROW];
static int  op_num_slice_in_pic     = 1;                      // default 1
static char op_tile_array_in_slice[2 * 600];                  // Max. slices can be 600 for the highest level 6.2
static int  op_arbitrary_slice_flag                  = 0;     // default  0
static char op_num_remaining_tiles_in_slice[600]     = { 0 }; // only in case of arbitrary slices
static int  op_loop_filter_across_tiles_enabled_flag = 0;     // by default disabled
static int  op_parallel_task                         = 1;     // default single task
static int  op_rc_type                               = 0;     // rc_off = 0 , rc_cbr = 1
static int  op_bps                                   = 100000;// Default 100Kbps
static int  op_vbv_msec                              = 2000;  // Default value 2000ms
static int  op_use_filler_flag                       = 0;     // Default value 0
static int  op_num_pre_analysis_frames               = 0;     // Default value 0
static int  op_chroma_qp_table_present_flag       = 0;
static char op_chroma_qp_num_points_in_table[256] = {0};
static char op_chroma_qp_delta_in_val_cb[256]     = {0};
static char op_chroma_qp_delta_out_val_cb[256]    = { 0 };
static char op_chroma_qp_delta_in_val_cr[256]     = { 0 };
static char op_chroma_qp_delta_out_val_cr[256]    = { 0 };

static int  op_dra_enable_flag   = 0;
static int  op_dra_number_ranges = 0;
static char op_dra_range[256]    = { 0 };
static char op_dra_scale[256]    = { 0 };
static char op_dra_chroma_qp_scale[256]  = "1.0";
static char op_dra_chroma_qp_offset[256] = "0.0";
static char op_dra_chroma_cb_scale[256]  = "1.0";
static char op_dra_chroma_cr_scale[256]  = "1.0";
static char op_dra_hist_norm[256]        = "1.0";

static int  op_rpl_extern = 0;
static char op_rpl0[MAX_NUM_RPLS][256];
static char op_rpl1[MAX_NUM_RPLS][256];

static int  op_use_dqp             = 0;  /* default cu_delta_qp is off */
static int  op_cu_qp_delta_area    = 10; /* default cu_delta_qp_area is 10 */

static int  op_inter_slice_type     = 0;

static int  op_picture_cropping_flag      = 0;
static int  op_picture_crop_left_offset   = 0;
static int  op_picture_crop_right_offset  = 0;
static int  op_picture_crop_top_offset    = 0;
static int  op_picture_crop_bottom_offset = 0;

static char  op_preset[16] = "reference"; /* default - reference SW level */

typedef enum _OP_FLAGS
{
    OP_FLAG_FNAME_CFG,
    OP_FLAG_FNAME_INP,
    OP_FLAG_FNAME_OUT,
    OP_FLAG_FNAME_REC,
    OP_FLAG_WIDTH_INP,
    OP_FLAG_HEIGHT_INP,
    OP_FLAG_QP,
    OP_FLAG_USE_DQP,
    OP_FLAG_CU_QP_DELTA_AREA,
    OP_FLAG_FPS,
    OP_FLAG_IPERIOD,
    OP_FLAG_MAX_FRM_NUM,
    OP_FLAG_USE_PIC_SIGN,
    OP_FLAG_VERBOSE,
    OP_FLAG_MAX_B_FRAMES,
    OP_FLAG_CLOSED_GOP,
    OP_FLAG_IBC,
    OP_IBC_SEARCH_RNG_X,
    OP_IBC_SEARCH_RND_Y,
    OP_IBC_HASH_FLAG,
    OP_IBC_HASH_SEARCH_MAX_CAND,
    OP_IBC_HASH_SEARCH_RANGE_4SMALLBLK,
    OP_IBC_FAST_METHOD,
    OP_FLAG_DISABLE_HGOP,
    OP_FLAG_OUT_BIT_DEPTH,
    OP_FLAG_IN_BIT_DEPTH,
    OP_FLAG_CODEC_BIT_DEPTH,
    OP_FLAG_RDO_DBK_SWITCH,
    OP_FLAG_SKIP_FRAMES,
    OP_PROFILE,
    OP_LEVEL,
    OP_BTT,
    OP_SUCO,
    OP_FLAG_ADD_QP_FRAME,
    OP_FRAMEWORK_CB_MAX,
    OP_FRAMEWORK_CB_MIN,
    OP_FRAMEWORK_CU14_MAX,
    OP_FRAMEWORK_TRIS_MAX,
    OP_FRAMEWORK_TRIS_MIN,
    OP_FRAMEWORK_SUCO_MAX,
    OP_FRAMEWORK_SUCO_MIN,
    OP_TOOL_AMVR,
    OP_TOOL_MMVD,
    OP_TOOL_AFFINE,
    OP_TOOL_DMVR,
    OP_TOOL_ADDB,
    OP_TOOL_ALF,
    OP_TOOL_RPL,
    OP_TOOL_POCS,
    OP_TOOL_HTDF,
    OP_TOOL_ADMVP,
    OP_TOOL_HMVP,
    OP_TOOL_EIPD,
    OP_TOOL_IQT,
    OP_TOOL_CM_INIT,
    OP_TOOL_ADCC,
    OP_CB_QP_OFFSET,
    OP_CR_QP_OFFSET,
    OP_TOOL_ATS,
    OP_CONSTRAINED_INTRA_PRED,
    OP_TOOL_DBF,
    OP_TOOL_DBFOFFSET,
    OP_TILE_UNIFORM_SPACING,
    OP_NUM_TILE_COLUMNS,
    OP_NUM_TILE_ROWS,
    OP_TILE_COLUMN_WIDTH_ARRAY,
    OP_TILE_ROW_HEIGHT_ARRAY,
    OP_NUM_SLICE_IN_PIC,
    OP_SLICE_BOUNDARY_ARRAY,
    OP_ARBITRAY_SLICE_FLAG,
    OP_NUM_REMAINING_TILES_IN_SLICE,
    OP_LOOP_FILTER_ACROSS_TILES_ENABLED_FLAG,
    OP_PARALLEL_TASK,
    OP_RC_TYPE,
    OP_BPS,
    OP_VBV_MSEC,
    OP_USE_FILLER_FLAG,
    OP_NUM_PRE_ANALYSIS_FRAMES,
    OP_CHROMA_QP_TABLE_PRESENT_FLAG,
    OP_CHROMA_QP_NUM_POINTS_IN_TABLE,
    OP_CHROMA_QP_DELTA_IN_VAL_CB,
    OP_CHROMA_QP_DELTA_OUT_VAL_CB,
    OP_CHROMA_QP_DELTA_IN_VAL_CR,
    OP_CHROMA_QP_DELTA_OUT_VAL_CR,
    OP_DRA_ENABLE_FLAG,
    OP_DRA_NUMBER_RANGES,
    OP_DRA_RANGE,
    OP_DRA_SCALE,
    OP_DRA_CHROMA_QP_SCALE,
    OP_DRA_CHROMA_QP_OFFSET,
    OP_DRA_CHROMA_CB_SCALE,
    OP_DRA_CHROMA_CR_SCALE,
    OP_DRA_HIST_NORM,
    OP_FLAG_RPL_EXTERN,
    OP_FLAG_RPL0_0,
    OP_FLAG_RPL0_1,
    OP_FLAG_RPL0_2,
    OP_FLAG_RPL0_3,
    OP_FLAG_RPL0_4,
    OP_FLAG_RPL0_5,
    OP_FLAG_RPL0_6,
    OP_FLAG_RPL0_7,
    OP_FLAG_RPL0_8,
    OP_FLAG_RPL0_9,
    OP_FLAG_RPL0_10,
    OP_FLAG_RPL0_11,
    OP_FLAG_RPL0_12,
    OP_FLAG_RPL0_13,
    OP_FLAG_RPL0_14,
    OP_FLAG_RPL0_15,
    OP_FLAG_RPL0_16,
    OP_FLAG_RPL0_17,
    OP_FLAG_RPL0_18,
    OP_FLAG_RPL0_19,
    OP_FLAG_RPL0_20,
    OP_FLAG_RPL0_21,
    OP_FLAG_RPL0_22,
    OP_FLAG_RPL0_23,
    OP_FLAG_RPL0_24,
    OP_FLAG_RPL0_25,

    //...
    OP_FLAG_RPL0_31,

    OP_FLAG_RPL1_0,
    OP_FLAG_RPL1_1,
    OP_FLAG_RPL1_2,
    OP_FLAG_RPL1_3,
    OP_FLAG_RPL1_4,
    OP_FLAG_RPL1_5,
    OP_FLAG_RPL1_6,
    OP_FLAG_RPL1_7,
    OP_FLAG_RPL1_8,
    OP_FLAG_RPL1_9,
    OP_FLAG_RPL1_10,
    OP_FLAG_RPL1_11,
    OP_FLAG_RPL1_12,
    OP_FLAG_RPL1_13,
    OP_FLAG_RPL1_14,
    OP_FLAG_RPL1_15,
    OP_FLAG_RPL1_16,
    OP_FLAG_RPL1_17,
    OP_FLAG_RPL1_18,
    OP_FLAG_RPL1_19,
    OP_FLAG_RPL1_20,
    OP_FLAG_RPL1_21,
    OP_FLAG_RPL1_22,
    OP_FLAG_RPL1_23,
    OP_FLAG_RPL1_24,
    OP_FLAG_RPL1_25,

    //...
    OP_FLAG_RPL1_31,
    OP_INTER_SLICE_TYPE,
    OP_PIC_CROP_FLAG,
    OP_PIC_CROP_LEFT,
    OP_PIC_CROP_RIGHT,
    OP_PIC_CROP_TOP,
    OP_PIC_CROP_BOTTOM,
    OP_PRESET,

    OP_FLAG_MAX
} OP_FLAGS;

static int op_flag[OP_FLAG_MAX] = {0};

static ARGS_OPTION options[] = \
{
    {
        ARGS_NO_KEY, ARGS_KEY_LONG_CONFIG, ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_FNAME_CFG], op_fname_cfg,
        "file name of configuration"
    },
    {
        'i', "input", ARGS_VAL_TYPE_STRING|ARGS_VAL_TYPE_MANDATORY,
        &op_flag[OP_FLAG_FNAME_INP], op_fname_inp,
        "file name of input video"
    },
    {
        'o', "output", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_FNAME_OUT], op_fname_out,
        "file name of output bitstream"
    },
    {
        'r', "recon", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_FNAME_REC], op_fname_rec,
        "file name of reconstructed video"
    },
    {
        'w',  "width", ARGS_VAL_TYPE_INTEGER|ARGS_VAL_TYPE_MANDATORY,
        &op_flag[OP_FLAG_WIDTH_INP], &op_w,
        "pixel width of input video"
    },
    {
        'h',  "height", ARGS_VAL_TYPE_INTEGER|ARGS_VAL_TYPE_MANDATORY,
        &op_flag[OP_FLAG_HEIGHT_INP], &op_h,
        "pixel height of input video"
    },
    {
        'q',  "op_qp", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_QP], &op_qp,
        "QP value (0~51)"
    },
    {
        ARGS_NO_KEY,  "use_dqp", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_USE_DQP], &op_use_dqp,
        "use_dqp ({0,..,25})(default: 0) "
    },
    {
        ARGS_NO_KEY,  "cu_qp_delta_area", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_CU_QP_DELTA_AREA], &op_cu_qp_delta_area,
        "cu_qp_delta_area (>= 6)(default: 6) "
    },
    {
        'z',  "hz", ARGS_VAL_TYPE_INTEGER|ARGS_VAL_TYPE_MANDATORY,
        &op_flag[OP_FLAG_FPS], &op_fps,
        "frame rate (Hz)"
    },
    {
        'p',  "iperiod", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_IPERIOD], &op_iperiod,
        "I-picture period"
    },
    {
        'g',  "max_b_frames", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_MAX_B_FRAMES], &op_max_b_frames,
        "Number of maximum B frames (1,3,7,15)\n"
    },
    {
        'm',  "parallel_task", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_PARALLEL_TASK], &op_parallel_task,
        "Number of threads to be created"
    },
    {
        'f',  "frames", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_MAX_FRM_NUM], &op_max_frm_num,
        "maximum number of frames to be encoded"
    },
    {
        's',  "signature", ARGS_VAL_TYPE_NONE,
        &op_flag[OP_FLAG_USE_PIC_SIGN], &op_use_pic_signature,
        "embed picture signature (HASH) for conformance checking in decoding"
    },
    {
        'v',  "verbose", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_VERBOSE], &op_verbose,
        "verbose level\n"
        "\t 0: no message\n"
        "\t 1: simple messages (default)\n"
        "\t 2: frame-level messages\n"
    },
    {
        'd',  "input_bit_depth", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_IN_BIT_DEPTH], &op_inp_bit_depth,
        "input bitdepth (8(default), 10) "
    },
    {
        ARGS_NO_KEY,  "codec_bit_depth", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_CODEC_BIT_DEPTH], &op_codec_bit_depth,
        "codec internal bitdepth (10(default), 8, 12, 14) "
    },
    {
        ARGS_NO_KEY,  "rdo_dbk_switch", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_RDO_DBK_SWITCH], &op_rdo_dbk_switch,
        "switch to on/off rdo_dbk (1(default), 0) "
    },
    {
        ARGS_NO_KEY,  "output_bit_depth", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_OUT_BIT_DEPTH], &op_out_bit_depth,
        "output bitdepth (8, 10)(default: same as input bitdpeth) "
    },
    {
        ARGS_NO_KEY,  "ref_pic_gap_length", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_OUT_BIT_DEPTH], &op_ref_pic_gap_length,
        "reference picture gap length (1, 2, 4, 8, 16) only available when -g is 0"
    },
    {
        ARGS_NO_KEY,  "closed_gop", ARGS_VAL_TYPE_NONE,
        &op_flag[OP_FLAG_CLOSED_GOP], &op_closed_gop,
        "use closed GOP structure. if not set, open GOP is used"
    },
    {
        ARGS_NO_KEY,  "ibc", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_IBC], &op_enable_ibc,
        "use IBC feature. if not set, IBC feature is disabled"
    },
    {
        ARGS_NO_KEY,  "ibc_search_range_x", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_IBC_SEARCH_RNG_X], &op_ibc_search_range_x,
        "set ibc search range in horizontal direction (default 64)"
    },
    {
        ARGS_NO_KEY,  "ibc_search_range_y", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_IBC_SEARCH_RND_Y], &op_ibc_search_range_y,
        "set ibc search range in vertical direction (default 64)"
    },
    {
        ARGS_NO_KEY,  "ibc_hash_search_flag", ARGS_VAL_TYPE_NONE,
        &op_flag[OP_IBC_HASH_FLAG], &op_ibc_hash_search_flag,
        "use IBC hash based block matching search feature. if not set, it is disable"
    },
    {
        ARGS_NO_KEY,  "ibc_hash_search_max_cand", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_IBC_HASH_SEARCH_MAX_CAND], &op_ibc_hash_search_max_cand,
        "Max candidates for hash based IBC search (default 64)"
    },
    {
        ARGS_NO_KEY,  "ibc_hash_search_range_4smallblk", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_IBC_HASH_SEARCH_RANGE_4SMALLBLK], &op_ibc_hash_search_range_4smallblk,
        "Small block search range in IBC based search. (default 64)"
    },
    {
        ARGS_NO_KEY,  "ibc_fast_method", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_IBC_FAST_METHOD], &op_ibc_fast_method,
        "Fast methods for IBC\n"
        "\t 1: Buffer IBC block vector (current not support)\n"
            "\t 2: Adaptive search range (default)\n"
    },
    {
        ARGS_NO_KEY,  "disable_hgop", ARGS_VAL_TYPE_NONE,
        &op_flag[OP_FLAG_DISABLE_HGOP], &op_disable_hgop,
        "disable hierarchical GOP. if not set, hierarchical GOP is used"
    },
    {
        ARGS_NO_KEY,  "skip_frames", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_SKIP_FRAMES], &op_skip_frames,
        "number of skipped frames before encoding. default 0"
    },
    {
        ARGS_NO_KEY,  "profile", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_PROFILE], &op_profile,
        "profile setting flag  main, baseline (default: main) "
    },
    {
        ARGS_NO_KEY,  "level", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_LEVEL], &op_level,
        "level setting "
    },
    {
        ARGS_NO_KEY,  "btt", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_BTT], &op_btt,
        "binary and ternary splits on/off flag"
    },
    {
        ARGS_NO_KEY,  "suco", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_SUCO], &op_suco,
        "split unit coding ordering on/off flag"
    },
    {
        'a',  "qp_add_frm", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_ADD_QP_FRAME], &op_add_qp_frames,
        "one more qp are added after this number of frames, disable:0 (default)"
    },
    {
        ARGS_NO_KEY,  "cb_max", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FRAMEWORK_CB_MAX], &op_framework_cb_max,
        "Max size of Coding Block (log scale)"
    },
    {
        ARGS_NO_KEY,  "cb_min", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FRAMEWORK_CB_MIN], &op_framework_cb_min,
        "MIN size of Coding Block (log scale)"
    },
    {
        ARGS_NO_KEY,  "cu14_max", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FRAMEWORK_CU14_MAX], &op_framework_cu14_max,
        "Max size of 4N in 4NxN or Nx4N block (log scale)"
    },
    {
        ARGS_NO_KEY,  "tris_max", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FRAMEWORK_TRIS_MAX], &op_framework_tris_max,
        "Max size of Tri-split allowed"
    },
    {
        ARGS_NO_KEY,  "tris_min", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FRAMEWORK_TRIS_MIN], &op_framework_tris_min,
        "Min size of Tri-split allowed"
    },
    {
        ARGS_NO_KEY,  "suco_max", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FRAMEWORK_SUCO_MAX], &op_framework_suco_max,
        "Max size of suco allowed from top"
    },
    {
        ARGS_NO_KEY,  "suco_min", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FRAMEWORK_SUCO_MIN], &op_framework_suco_min,
        "Min size of suco allowed from top"
    },
    {
        ARGS_NO_KEY,  "amvr", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_AMVR], &op_tool_amvr,
        "amvr on/off flag"
    },
    {
        ARGS_NO_KEY,  "mmvd", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_MMVD], &op_tool_mmvd,
        "mmvd on/off flag"
    },
    {
        ARGS_NO_KEY,  "affine", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_AFFINE], &op_tool_affine,
        "affine on/off flag"
    },
    {
        ARGS_NO_KEY,  "dmvr", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_DMVR], &op_tool_dmvr,
        "dmvr on/off flag"
    },
    {
        ARGS_NO_KEY,  "addb", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_ADDB], &op_tool_addb,
        "addb on/off flag"
    },
    {
        ARGS_NO_KEY,  "alf", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_ALF], &op_tool_alf,
        "alf on/off flag"
    },
    {
        ARGS_NO_KEY,  "htdf", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_HTDF], &op_tool_htdf,
        "htdf on/off flag"
    },
    {
        ARGS_NO_KEY,  "admvp", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_ADMVP], &op_tool_admvp,
        "admvp on/off flag"
    },
    {
        ARGS_NO_KEY,  "hmvp", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_HMVP], &op_tool_hmvp,
        "hmvp on/off flag"
    },

    {
        ARGS_NO_KEY,  "eipd", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_EIPD], &op_tool_eipd,
        "eipd on/off flag"
    },
    {
        ARGS_NO_KEY,  "iqt", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_IQT], &op_tool_iqt,
        "iqt on/off flag"
    },
    {
        ARGS_NO_KEY,  "cm_init", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_CM_INIT], &op_tool_cm_init,
        "cm_init on/off flag"
    },
    {
        ARGS_NO_KEY,  "adcc", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_ADCC], &op_tool_adcc,
        "adcc on/off flag"
    },
    {
        ARGS_NO_KEY,  "rpl", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_RPL], &op_tool_rpl,
        "rpl on/off flag"
    },
    {
        ARGS_NO_KEY,  "pocs", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_POCS], &op_tool_pocs,
        "pocs on/off flag"
    },
    {
        ARGS_NO_KEY,  "cb_qp_offset", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_CB_QP_OFFSET], &op_cb_qp_offset,
        "cb qp offset"
    },
    {
        ARGS_NO_KEY,  "cr_qp_offset", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_CR_QP_OFFSET], &op_cr_qp_offset,
        "cr qp offset"
    },
    {
        ARGS_NO_KEY, "ats", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_ATS], &op_tool_ats,
        "ats on/off flag"
    },
    {
        ARGS_NO_KEY,  "constrained_intra_pred", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_CONSTRAINED_INTRA_PRED], &op_constrained_intra_pred,
        "constrained intra pred"
    },
    {
        ARGS_NO_KEY,  "dbf", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_DBF], &op_tool_deblocking,
        "Deblocking filter on/off flag"
    },
    {
        ARGS_NO_KEY,  "dbfoffsetA", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_DBFOFFSET], &op_deblock_alpha_offset,
        "ADDB Deblocking filter offset for alpha"
    },
    {
        ARGS_NO_KEY,  "dbfoffsetB", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_DBFOFFSET], &op_deblock_beta_offset,
        "ADDB Deblocking filter offset for beta"
    },
    {
        ARGS_NO_KEY,  "tile_uniform_spacing", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TILE_UNIFORM_SPACING], &op_tile_uniform_spacing,
        "uniform or non-uniform tile spacing"
    },
    {
        ARGS_NO_KEY,  "num_tile_columns", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_NUM_TILE_COLUMNS], &op_num_tile_columns,
        "Number of tile columns"
    },
    {
        ARGS_NO_KEY,  "num_tile_rows", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_NUM_TILE_ROWS], &op_num_tile_rows,
        "Number of tile rows"
    },
    {
        ARGS_NO_KEY,  "tile_column_width_array", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_TILE_COLUMN_WIDTH_ARRAY], &op_tile_column_width_array,
        "Array of Tile Column Width"
    },
    {
        ARGS_NO_KEY,  "tile_row_height_array", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_TILE_ROW_HEIGHT_ARRAY], &op_tile_row_height_array,
        "Array of Tile Row Height"
    },
    {
        ARGS_NO_KEY,  "num_slices_in_pic", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_NUM_SLICE_IN_PIC], &op_num_slice_in_pic,
        "Number of slices in the pic"
    },
    {
        ARGS_NO_KEY,  "tile_array_in_slice", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_SLICE_BOUNDARY_ARRAY], &op_tile_array_in_slice,
        "Array of Slice Boundaries"
    },
    {
        ARGS_NO_KEY,  "arbitrary_slice_flag", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_ARBITRAY_SLICE_FLAG], &op_arbitrary_slice_flag,
        "Array of Slice Boundaries"
    },
    {
        ARGS_NO_KEY,  "num_remaining_tiles_in_slice", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_NUM_REMAINING_TILES_IN_SLICE], &op_num_remaining_tiles_in_slice,
        "Array of Slice Boundaries"
    },
    {
        ARGS_NO_KEY,  "lp_filter_across_tiles_en_flag", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_LOOP_FILTER_ACROSS_TILES_ENABLED_FLAG], &op_loop_filter_across_tiles_enabled_flag,
        "Loop filter across tiles enabled or disabled"
    },
    {
        ARGS_NO_KEY,  "rc_type", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_RC_TYPE], &op_rc_type,
        "Rate control type: OFF, CBR"
    },
    {
        ARGS_NO_KEY,  "bps", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_BPS], &op_bps,
        "Bits per second"
    },
    {
        ARGS_NO_KEY,  "vbv_msec", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_VBV_MSEC], &op_vbv_msec,
        "VBV size in msec"
    },
    {
        ARGS_NO_KEY,  "use_filler", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_USE_FILLER_FLAG], &op_use_filler_flag,
        "user filler flag"
    },
    {
        ARGS_NO_KEY,  "pre_analysis_frames", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_NUM_PRE_ANALYSIS_FRAMES], &op_num_pre_analysis_frames,
        "number of pre analysis frames"
    },
    {
        ARGS_NO_KEY,  "chroma_qp_table_present_flag", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_CHROMA_QP_TABLE_PRESENT_FLAG], &op_chroma_qp_table_present_flag,
        "chroma_qp_table_present_flag"
    },
    {
        ARGS_NO_KEY,  "chroma_qp_num_points_in_table", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_CHROMA_QP_NUM_POINTS_IN_TABLE], &op_chroma_qp_num_points_in_table,
        "Number of pivot points for Cb and Cr channels"
    },
    {
        ARGS_NO_KEY,  "chroma_qp_delta_in_val_cb", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_CHROMA_QP_DELTA_IN_VAL_CB], &op_chroma_qp_delta_in_val_cb,
        "Array of input pivot points for Cb"
    },
    {
        ARGS_NO_KEY,  "chroma_qp_delta_out_val_cb", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_CHROMA_QP_DELTA_OUT_VAL_CB], &op_chroma_qp_delta_out_val_cb,
        "Array of input pivot points for Cb"
    },
    {
        ARGS_NO_KEY,  "chroma_qp_delta_in_val_cr", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_CHROMA_QP_DELTA_IN_VAL_CR], &op_chroma_qp_delta_in_val_cr,
        "Array of input pivot points for Cr"
    },
    {
        ARGS_NO_KEY,  "chroma_qp_delta_out_val_cr", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_CHROMA_QP_DELTA_OUT_VAL_CR], &op_chroma_qp_delta_out_val_cr,
        "Array of input pivot points for Cr"
    },

    {
        ARGS_NO_KEY,  "dra_enable_flag", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_DRA_ENABLE_FLAG], &op_dra_enable_flag,
        "op_dra_enable_flag"
    },
    {
        ARGS_NO_KEY,  "dra_number_ranges", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_DRA_NUMBER_RANGES], &op_dra_number_ranges,
        "Number of DRA ranges"
    },
    {
        ARGS_NO_KEY,  "dra_range", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_DRA_RANGE], &op_dra_range,
        "Array of dra ranges"
    },
    {
        ARGS_NO_KEY,  "dra_scale", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_DRA_SCALE], &op_dra_scale,
        "Array of input dra ranges"
    },
    {
        ARGS_NO_KEY,  "dra_chroma_qp_scale", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_DRA_CHROMA_QP_SCALE], &op_dra_chroma_qp_scale,
        "op_dra_chroma_qp_scale value"
    },
    {
        ARGS_NO_KEY,  "dra_chroma_qp_offset", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_DRA_CHROMA_QP_OFFSET], &op_dra_chroma_qp_offset ,
        "op_dra_chroma_qp_offset"
    },
    {
        ARGS_NO_KEY,  "dra_chroma_cb_scale", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_DRA_CHROMA_CB_SCALE], &op_dra_chroma_cb_scale,
        "op_dra_chroma_cb_scale"
    },
    {
        ARGS_NO_KEY,  "dra_chroma_cr_scale", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_DRA_CHROMA_CR_SCALE], &op_dra_chroma_cr_scale,
        "op_dra_chroma_cr_scale"
    },
    {
        ARGS_NO_KEY,  "dra_hist_norm", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_DRA_HIST_NORM], &op_dra_hist_norm,
        "op_dra_hist_norm"
    },
    {
        ARGS_NO_KEY,  "rpl_extern", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_RPL_EXTERN], &op_rpl_extern,
        "Whether to input external RPL"
    },
    {
        ARGS_NO_KEY,  "RPL0_0", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_0], &op_rpl0[0],
        "RPL0_0"
    },
    {
        ARGS_NO_KEY,  "RPL0_1", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_1], &op_rpl0[1],
        "RPL0_1"
    },
    {
        ARGS_NO_KEY,  "RPL0_2", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_2], &op_rpl0[2],
        "RPL0_2"
    },
    {
        ARGS_NO_KEY,  "RPL0_3", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_3], &op_rpl0[3],
        "RPL0_3"
    },
    {
        ARGS_NO_KEY,  "RPL0_4", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_4], &op_rpl0[4],
        "RPL0_4"
    },
    {
        ARGS_NO_KEY,  "RPL0_5", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_5], &op_rpl0[5],
        "RPL0_5"
    },
    {
        ARGS_NO_KEY,  "RPL0_6", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_6], &op_rpl0[6],
        "RPL0_6"
    },
    {
        ARGS_NO_KEY,  "RPL0_7", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_7], &op_rpl0[7],
        "RPL0_7"
    },
    {
        ARGS_NO_KEY,  "RPL0_8", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_8], &op_rpl0[8],
        "RPL0_8"
    },
    {
        ARGS_NO_KEY,  "RPL0_9", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_9], &op_rpl0[9],
        "RPL0_9"
    },
    {
        ARGS_NO_KEY,  "RPL0_10", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_10], &op_rpl0[10],
        "RPL0_10"
    },
    {
        ARGS_NO_KEY,  "RPL0_11", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_11], &op_rpl0[11],
        "RPL0_11"
    },
    {
        ARGS_NO_KEY,  "RPL0_12", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_12], &op_rpl0[12],
        "RPL0_12"
    },
    {
        ARGS_NO_KEY,  "RPL0_13", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_13], &op_rpl0[13],
        "RPL0_13"
    },
    {
        ARGS_NO_KEY,  "RPL0_14", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_14], &op_rpl0[14],
        "RPL0_14"
    },
    {
        ARGS_NO_KEY,  "RPL0_15", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_15], &op_rpl0[15],
        "RPL0_15"
    },
    {
        ARGS_NO_KEY,  "RPL0_16", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_16], &op_rpl0[16],
        "RPL0_16"
    },
    {
        ARGS_NO_KEY,  "RPL0_17", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_17], &op_rpl0[17],
        "RPL0_17"
    },
    {
        ARGS_NO_KEY,  "RPL0_18", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_18], &op_rpl0[18],
        "RPL0_18"
    },
    {
        ARGS_NO_KEY,  "RPL0_19", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_19], &op_rpl0[19],
        "RPL0_19"
    },
    {
        ARGS_NO_KEY,  "RPL0_20", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_20], &op_rpl0[20],
        "RPL0_20"
    },
    {
        ARGS_NO_KEY,  "RPL0_21", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_21], &op_rpl0[21],
        "RPL0_21"
    },
    {
        ARGS_NO_KEY,  "RPL0_22", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_22], &op_rpl0[22],
        "RPL0_22"
    },
    {
        ARGS_NO_KEY,  "RPL0_23", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_23], &op_rpl0[23],
        "RPL0_23"
    },
    {
        ARGS_NO_KEY,  "RPL0_24", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_24], &op_rpl0[24],
        "RPL0_24"
    },
    {
        ARGS_NO_KEY,  "RPL0_25", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_25], &op_rpl0[25],
        "RPL0_25"
    },
    {
        ARGS_NO_KEY,  "RPL1_0", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_0], &op_rpl1[0],
        "RPL1_0"
    },
    {
        ARGS_NO_KEY,  "RPL1_1", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_1], &op_rpl1[1],
        "RPL1_1"
    },
    {
        ARGS_NO_KEY,  "RPL1_2", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_2], &op_rpl1[2],
        "RPL1_2"
    },
    {
        ARGS_NO_KEY,  "RPL1_3", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_3], &op_rpl1[3],
        "RPL1_3"
    },
    {
        ARGS_NO_KEY,  "RPL1_4", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_4], &op_rpl1[4],
        "RPL1_4"
    },
    {
        ARGS_NO_KEY,  "RPL1_5", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_5], &op_rpl1[5],
        "RPL1_5"
    },
    {
        ARGS_NO_KEY,  "RPL1_6", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_6], &op_rpl1[6],
        "RPL1_6"
    },
    {
        ARGS_NO_KEY,  "RPL1_7", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_7], &op_rpl1[7],
        "RPL1_7"
    },
    {
        ARGS_NO_KEY,  "RPL1_8", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_8], &op_rpl1[8],
        "RPL1_8"
    },
    {
        ARGS_NO_KEY,  "RPL1_9", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_9], &op_rpl1[9],
        "RPL1_9"
    },
    {
        ARGS_NO_KEY,  "RPL1_10", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_10], &op_rpl1[10],
        "RPL1_10"
    },
    {
        ARGS_NO_KEY,  "RPL1_11", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_11], &op_rpl1[11],
        "RPL1_11"
    },
    {
        ARGS_NO_KEY,  "RPL1_12", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_12], &op_rpl1[12],
        "RPL1_12"
    },
    {
        ARGS_NO_KEY,  "RPL1_13", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_13], &op_rpl1[13],
        "RPL1_13"
    },
    {
        ARGS_NO_KEY,  "RPL1_14", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_14], &op_rpl1[14],
        "RPL1_14"
    },
    {
        ARGS_NO_KEY,  "RPL1_15", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_15], &op_rpl1[15],
        "RPL1_15"
    },
    {
        ARGS_NO_KEY,  "RPL1_16", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_16], &op_rpl1[16],
        "RPL1_16"
    },
    {
        ARGS_NO_KEY,  "RPL1_17", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_17], &op_rpl1[17],
        "RPL1_17"
    },
    {
        ARGS_NO_KEY,  "RPL1_18", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_18], &op_rpl1[18],
        "RPL1_18"
    },
    {
        ARGS_NO_KEY,  "RPL1_19", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_19], &op_rpl1[19],
        "RPL1_19"
    },
    {
        ARGS_NO_KEY,  "RPL1_20", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_20], &op_rpl1[20],
        "RPL1_20"
    },
    {
        ARGS_NO_KEY,  "RPL1_21", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_21], &op_rpl1[21],
        "RPL1_21"
    },
    {
        ARGS_NO_KEY,  "RPL1_22", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_22], &op_rpl1[22],
        "RPL1_22"
    },
    {
        ARGS_NO_KEY,  "RPL1_23", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_23], &op_rpl1[23],
        "RPL1_23"
    },
    {
        ARGS_NO_KEY,  "RPL1_24", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_24], &op_rpl1[24],
        "RPL1_24"
    },
    {
        ARGS_NO_KEY,  "RPL1_25", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_25], &op_rpl1[25],
        "RPL1_25"
    },
    {
        ARGS_NO_KEY,  "inter_slice_type", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_INTER_SLICE_TYPE], &op_inter_slice_type,
        "INTER_SLICE_TYPE"
    },
    {
        ARGS_NO_KEY,  "picture_cropping_flag", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_PIC_CROP_FLAG], &op_picture_cropping_flag,
        "INTER_SLICE_TYPE"
    },
    {
        ARGS_NO_KEY,  "picture_crop_left", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_PIC_CROP_LEFT], &op_picture_crop_left_offset,
        "INTER_SLICE_TYPE"
    },
    {
        ARGS_NO_KEY,  "picture_crop_right", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_PIC_CROP_RIGHT], &op_picture_crop_right_offset,
        "INTER_SLICE_TYPE"
    },
    {
        ARGS_NO_KEY,  "picture_crop_top", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_PIC_CROP_TOP], &op_picture_crop_top_offset,
        "INTER_SLICE_TYPE"
    },
    {
        ARGS_NO_KEY,  "picture_crop_bottom", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_PIC_CROP_BOTTOM], &op_picture_crop_bottom_offset,
        "INTER_SLICE_TYPE"
    },
    {
        ARGS_NO_KEY,  "preset", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_PRESET], &op_preset,
        "Encoder PRESET"
        "\t [fast, medium, slow, reference]"
    },
    {0, "", ARGS_VAL_TYPE_NONE, NULL, NULL, ""} /* termination */
};

#define NUM_ARG_OPTION   ((int)(sizeof(options)/sizeof(options[0]))-1)

#endif /*_XEVE_APP_ARGS_H_ */


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
#include "xeve.h"

#define ARGS_VAL_TYPE_MANDATORY       ( 1 << 0) /* mandatory or not */
#define ARGS_VAL_TYPE_NONE            ( 1 << 2) /* no value */
#define ARGS_VAL_TYPE_INTEGER         ( 2 << 2) /* integer type value */
#define ARGS_VAL_TYPE_STRING          ( 3 << 2) /* string type value */
#define ARGS_GET_CMD_OPT_VAL_TYPE(x)  ((x) & 0x0C)
#define ARGS_GET_IS_OPT_TYPE_PPT(x)   (((x) >> 1) & 0x01)

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
    char default_value[256] = { 0 };

    switch(ARGS_GET_CMD_OPT_VAL_TYPE(o->val_type))
    {
        case ARGS_VAL_TYPE_INTEGER:
            strcpy(vtype, "INTEGER");
            sprintf(default_value, " [%d]", *(int*)(o->val));
            break;
        case ARGS_VAL_TYPE_STRING:
            strcpy(vtype, "STRING");
            sprintf(default_value, " [%s]", strlen((char*)(o->val)) == 0 ? "None" : (char*)(o->val));
            break;
        case ARGS_VAL_TYPE_NONE:
        default:
            strcpy(vtype, "FLAG");
            sprintf(default_value, " [%s]", *(int*)(o->val) ? "On" : "Off");
            break;
    }
    optional = !(o->val_type & ARGS_VAL_TYPE_MANDATORY);

    if(o->key != ARGS_NO_KEY)
    {
        sprintf(help, "  -%c, --%s [%s]%s%s\n    : %s", o->key, o->key_long,
                vtype, (optional) ? " (optional)" : "", (optional) ? default_value : "", o->desc);
    }
    else
    {
        sprintf(help, "  --%s [%s]%s%s\n    : %s", o->key_long,
                vtype, (optional) ? " (optional)" : "", (optional) ? default_value : "", o->desc);
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
static char op_fname_cfg[256]    = "\0"; /* config file path name */
static char op_fname_inp[256]    = "\0"; /* input original video */
static char op_fname_out[256]    = "\0"; /* output bitstream */
static char op_fname_rec[256]    = "\0"; /* reconstructed video */
static int  op_max_frm_num       = 0;
static int  op_skip_frames       = 0;
static int  op_use_pic_signature = 0;
static int  op_inp_bit_depth     = 8;
static int  op_codec_bit_depth   = 10;
static int  op_out_bit_depth     = 10; /* same as codec bit-depth */
static int  op_chroma_format_idc = 1;
static char op_profile[16]       = "baseline";
static char op_preset[16]        = "medium";
static char op_tune[16]          = "\0";

typedef enum _OP_FLAGS
{
    OP_FLAG_FNAME_CFG,
    OP_FLAG_FNAME_INP,
    OP_FLAG_FNAME_OUT,
    OP_FLAG_FNAME_REC,
    OP_FLAG_WIDTH_INP,
    OP_FLAG_HEIGHT_INP,
    OP_FLAG_QP,
    OP_FLAG_AQMODE,
    OP_FLAG_CUTREE,
    OP_FLAG_CU_QP_DELTA_AREA,
    OP_FLAG_FPS,
    OP_FLAG_IPERIOD,
    OP_FLAG_MAX_B_FRAMES,
    OP_THREADS,
    OP_FLAG_MAX_FRM_NUM,
    OP_FLAG_USE_PIC_SIGN,
    OP_FLAG_VERBOSE,
    OP_FLAG_IN_BIT_DEPTH,
    OP_FLAG_CODEC_BIT_DEPTH,
    OP_FLAG_CHROMA_FORMAT_IDC,
    OP_FLAG_RDO_DBK_SWITCH,
    OP_FLAG_OUT_BIT_DEPTH,
    OP_REF_PIC_GAP,
    OP_FLAG_CLOSED_GOP,
    OP_FLAG_IBC,
    OP_IBC_SEARCH_RNG_X,
    OP_IBC_SEARCH_RND_Y,
    OP_IBC_HASH_FLAG,
    OP_IBC_HASH_SEARCH_MAX_CAND,
    OP_IBC_HASH_SEARCH_RANGE_4SMALLBLK,
    OP_IBC_FAST_METHOD,
    OP_FLAG_DISABLE_HGOP,
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
    OP_TOOL_HTDF,
    OP_TOOL_ADMVP,
    OP_TOOL_HMVP,
    OP_TOOL_EIPD,
    OP_TOOL_IQT,
    OP_TOOL_CM_INIT,
    OP_TOOL_ADCC,
    OP_TOOL_RPL,
    OP_TOOL_POCS,
    OP_QP_CB_OFFSET,
    OP_QP_CR_OFFSET,
    OP_TOOL_ATS,
    OP_CONSTRAINED_INTRA_PRED,
    OP_TOOL_DBF,
    OP_TOOL_DBFOFFSET_A,
    OP_TOOL_DBFOFFSET_B,
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
    OP_RC_TYPE,
    OP_BPS,
    OP_VBV_MSEC,
    OP_VBV_BUF_SIZE,
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
    //..OP_FLAG_RPL0_31,

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
    //..OP_FLAG_RPL1_31,

    OP_INTER_SLICE_TYPE,
    OP_PIC_CROP_FLAG,
    OP_PIC_CROP_LEFT,
    OP_PIC_CROP_RIGHT,
    OP_PIC_CROP_TOP,
    OP_PIC_CROP_BOTTOM,
    OP_PRESET,
    OP_TUNE,

    OP_FLAG_MAX
} OP_FLAGS;

static int op_flag[OP_FLAG_MAX] = {0};

static ARGS_OPTION options[] = \
{
    {
        ARGS_NO_KEY, ARGS_KEY_LONG_CONFIG, ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_FNAME_CFG], NULL,
        "file name of configuration"
    },
    {
        'i', "input", ARGS_VAL_TYPE_STRING | ARGS_VAL_TYPE_MANDATORY,
        &op_flag[OP_FLAG_FNAME_INP], NULL,
        "file name of input video"
    },
    {
        'o', "output", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_FNAME_OUT], NULL,
        "file name of output bitstream"
    },
    {
        'r', "recon", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_FNAME_REC], NULL,
        "file name of reconstructed video"
    },
    {
        'w',  "width", ARGS_VAL_TYPE_INTEGER | ARGS_VAL_TYPE_MANDATORY,
        &op_flag[OP_FLAG_WIDTH_INP], NULL,
        "pixel width of input video"
    },
    {
        'h',  "height", ARGS_VAL_TYPE_INTEGER | ARGS_VAL_TYPE_MANDATORY,
        &op_flag[OP_FLAG_HEIGHT_INP], NULL,
        "pixel height of input video"
    },
    {
        'q',  "qp", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_QP], NULL,
        "QP value (0~51)"
    },
    {
        ARGS_NO_KEY,  "aq-mode", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_AQMODE], NULL,
        "use adaptive quantization block qp adaptation\n"
        "      - 0: off\n"
        "      - 1: adaptive quantization"
    },
    {
        ARGS_NO_KEY,  "cutree", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_CUTREE], NULL,
        "use cutree block qp adaptation\n"
        "      - 0: off\n"
        "      - 1: cutree"
    },
    {
        ARGS_NO_KEY,  "cu_qp_delta_area", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_CU_QP_DELTA_AREA], NULL,
        "cu_qp_delta_area (>= 6)"
    },
    {
        'z',  "fps", ARGS_VAL_TYPE_INTEGER | ARGS_VAL_TYPE_MANDATORY,
        &op_flag[OP_FLAG_FPS], NULL,
        "frame rate (frame per second)"
    },
    {
        'I',  "keyint", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_IPERIOD], NULL,
        "I-picture period"
    },
    {
        'b',  "bframes", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_MAX_B_FRAMES], NULL,
        "maximum number of B frames (1,3,7,15)"
    },
    {
        'm',  "threads", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_THREADS], NULL,
        "force to use a specific number of threads"
    },
    {
        'f',  "frames", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_MAX_FRM_NUM], NULL,
        "maximum number of frames to be encoded"
    },
    {
        's',  "hash", ARGS_VAL_TYPE_NONE,
        &op_flag[OP_FLAG_USE_PIC_SIGN], NULL,
        "embed picture signature (HASH) for conformance checking in decoding"
    },
    {
        'v',  "log-level", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_VERBOSE], NULL,
        "verbose level\n"
        "      - 0: no message\n"
        "      - 1: simple messages\n"
        "      - 2: frame-level messages"
    },
    {
        'd',  "input-depth", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_IN_BIT_DEPTH], NULL,
        "input bit depth (8, 10) "
    },
    {
        ARGS_NO_KEY,  "codec_bit_depth", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_CODEC_BIT_DEPTH], NULL,
        "codec internal bit depth (10, 12) "
    },
    {
        ARGS_NO_KEY,  "input-csp", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_CHROMA_FORMAT_IDC], NULL,
        "input color space(chroma format idc)\n"
        "      - 0: YUV400\n"
        "      - 1: YUV420"
    },
    {
        ARGS_NO_KEY,  "rdo_dbk_switch", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_RDO_DBK_SWITCH], NULL,
        "switch to on/off rdo_dbk (0, 1) "
    },
    {
        ARGS_NO_KEY,  "output-depth", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_OUT_BIT_DEPTH], NULL,
        "output bit depth (8, 10)"
    },
    {
        ARGS_NO_KEY,  "ref_pic_gap_length", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_REF_PIC_GAP], NULL,
        "reference picture gap length (1, 2, 4, 8, 16) only available when -b is 0"
    },
    {
        ARGS_NO_KEY,  "closed_gop", ARGS_VAL_TYPE_NONE,
        &op_flag[OP_FLAG_CLOSED_GOP], NULL,
        "use closed GOP structure. if not set, open GOP is used"
    },
    {
        ARGS_NO_KEY,  "ibc", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_IBC], NULL,
        "use IBC feature. if not set, IBC feature is disabled"
    },
    {
        ARGS_NO_KEY,  "ibc_search_range_x", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_IBC_SEARCH_RNG_X], NULL,
        "set ibc search range in horizontal direction"
    },
    {
        ARGS_NO_KEY,  "ibc_search_range_y", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_IBC_SEARCH_RND_Y], NULL,
        "set ibc search range in vertical direction"
    },
    {
        ARGS_NO_KEY,  "ibc_hash_search_flag", ARGS_VAL_TYPE_NONE,
        &op_flag[OP_IBC_HASH_FLAG], NULL,
        "use IBC hash based block matching search feature. if not set, it is disable"
    },
    {
        ARGS_NO_KEY,  "ibc_hash_search_max_cand", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_IBC_HASH_SEARCH_MAX_CAND], NULL,
        "Max candidates for hash based IBC search"
    },
    {
        ARGS_NO_KEY,  "ibc_hash_search_range_4smallblk", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_IBC_HASH_SEARCH_RANGE_4SMALLBLK], NULL,
        "Small block search range in IBC based search"
    },
    {
        ARGS_NO_KEY,  "ibc_fast_method", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_IBC_FAST_METHOD], NULL,
        "Fast methods for IBC\n"
        "      - 1: Buffer IBC block vector (current not support)\n"
        "      - 2: Adaptive search range"
    },
    {
        ARGS_NO_KEY,  "disable_hgop", ARGS_VAL_TYPE_NONE,
        &op_flag[OP_FLAG_DISABLE_HGOP], NULL,
        "disable hierarchical GOP. if not set, hierarchical GOP is used"
    },
    {
        ARGS_NO_KEY,  "seek", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_SKIP_FRAMES], NULL,
        "number of skipped frames before encoding"
    },
    {
        ARGS_NO_KEY,  "profile", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_PROFILE], NULL,
        "profile setting flag  (main, baseline)"
    },
    {
        ARGS_NO_KEY,  "level-idc", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_LEVEL], NULL,
        "level setting "
    },
    {
        ARGS_NO_KEY,  "btt", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_BTT], NULL,
        "binary and ternary splits on/off flag"
    },
    {
        ARGS_NO_KEY,  "suco", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_SUCO], NULL,
        "split unit coding ordering on/off flag"
    },
    {
        'a',  "qp_add_frm", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_ADD_QP_FRAME], NULL,
        "one more qp are added after this number of frames, disable:0"
    },
    {
        ARGS_NO_KEY,  "ctu", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FRAMEWORK_CB_MAX], NULL,
        "Max size of Coding Block (log scale)"
    },
    {
        ARGS_NO_KEY,  "min-cu-size", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FRAMEWORK_CB_MIN], NULL,
        "MIN size of Coding Block (log scale)"
    },
    {
        ARGS_NO_KEY,  "cu14_max", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FRAMEWORK_CU14_MAX], NULL,
        "Max size of 4N in 4NxN or Nx4N block (log scale)"
    },
    {
        ARGS_NO_KEY,  "tris_max", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FRAMEWORK_TRIS_MAX], NULL,
        "Max size of Tri-split allowed"
    },
    {
        ARGS_NO_KEY,  "tris_min", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FRAMEWORK_TRIS_MIN], NULL,
        "Min size of Tri-split allowed"
    },
    {
        ARGS_NO_KEY,  "suco_max", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FRAMEWORK_SUCO_MAX], NULL,
        "Max size of suco allowed from top"
    },
    {
        ARGS_NO_KEY,  "suco_min", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FRAMEWORK_SUCO_MIN], NULL,
        "Min size of suco allowed from top"
    },
    {
        ARGS_NO_KEY,  "amvr", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_AMVR], NULL,
        "amvr on/off flag"
    },
    {
        ARGS_NO_KEY,  "mmvd", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_MMVD], NULL,
        "mmvd on/off flag"
    },
    {
        ARGS_NO_KEY,  "affine", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_AFFINE], NULL,
        "affine on/off flag"
    },
    {
        ARGS_NO_KEY,  "dmvr", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_DMVR], NULL,
        "dmvr on/off flag"
    },
    {
        ARGS_NO_KEY,  "addb", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_ADDB], NULL,
        "addb on/off flag"
    },
    {
        ARGS_NO_KEY,  "alf", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_ALF], NULL,
        "alf on/off flag"
    },
    {
        ARGS_NO_KEY,  "htdf", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_HTDF], NULL,
        "htdf on/off flag"
    },
    {
        ARGS_NO_KEY,  "admvp", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_ADMVP], NULL,
        "admvp on/off flag"
    },
    {
        ARGS_NO_KEY,  "hmvp", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_HMVP], NULL,
        "hmvp on/off flag"
    },

    {
        ARGS_NO_KEY,  "eipd", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_EIPD], NULL,
        "eipd on/off flag"
    },
    {
        ARGS_NO_KEY,  "iqt", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_IQT], NULL,
        "iqt on/off flag"
    },
    {
        ARGS_NO_KEY,  "cm_init", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_CM_INIT], NULL,
        "cm_init on/off flag"
    },
    {
        ARGS_NO_KEY,  "adcc", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_ADCC], NULL,
        "adcc on/off flag"
    },
    {
        ARGS_NO_KEY,  "rpl", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_RPL], NULL,
        "rpl on/off flag"
    },
    {
        ARGS_NO_KEY,  "pocs", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_POCS], NULL,
        "pocs on/off flag"
    },
    {
        ARGS_NO_KEY,  "qp_cb_offset", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_QP_CB_OFFSET], NULL,
        "cb qp offset"
    },
    {
        ARGS_NO_KEY,  "qp_cr_offset", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_QP_CR_OFFSET], NULL,
        "cr qp offset"
    },
    {
        ARGS_NO_KEY, "ats", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_ATS], NULL,
        "ats on/off flag"
    },
    {
        ARGS_NO_KEY,  "constrained_intra_pred", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_CONSTRAINED_INTRA_PRED], NULL,
        "constrained intra pred"
    },
    {
        ARGS_NO_KEY,  "deblock", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_DBF], NULL,
        "Deblocking filter on/off flag"
    },
    {
        ARGS_NO_KEY,  "dbfoffsetA", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_DBFOFFSET_A], NULL,
        "ADDB Deblocking filter offset for alpha"
    },
    {
        ARGS_NO_KEY,  "dbfoffsetB", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TOOL_DBFOFFSET_B], NULL,
        "ADDB Deblocking filter offset for beta"
    },
    {
        ARGS_NO_KEY,  "tile_uniform_spacing", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_TILE_UNIFORM_SPACING], NULL,
        "uniform or non-uniform tile spacing"
    },
    {
        ARGS_NO_KEY,  "num_tile_columns", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_NUM_TILE_COLUMNS], NULL,
        "Number of tile columns"
    },
    {
        ARGS_NO_KEY,  "num_tile_rows", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_NUM_TILE_ROWS], NULL,
        "Number of tile rows"
    },
    {
        ARGS_NO_KEY,  "tile_column_width_array", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_TILE_COLUMN_WIDTH_ARRAY], NULL,
        "Array of Tile Column Width"
    },
    {
        ARGS_NO_KEY,  "tile_row_height_array", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_TILE_ROW_HEIGHT_ARRAY], NULL,
        "Array of Tile Row Height"
    },
    {
        ARGS_NO_KEY,  "num_slices_in_pic", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_NUM_SLICE_IN_PIC], NULL,
        "Number of slices in the pic"
    },
    {
        ARGS_NO_KEY,  "tile_array_in_slice", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_SLICE_BOUNDARY_ARRAY], NULL,
        "Array of Slice Boundaries"
    },
    {
        ARGS_NO_KEY,  "arbitrary_slice_flag", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_ARBITRAY_SLICE_FLAG], NULL,
        "Array of Slice Boundaries"
    },
    {
        ARGS_NO_KEY,  "num_remaining_tiles_in_slice", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_NUM_REMAINING_TILES_IN_SLICE], NULL,
        "Array of Slice Boundaries"
    },
    {
        ARGS_NO_KEY,  "lp_filter_across_tiles_en_flag", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_LOOP_FILTER_ACROSS_TILES_ENABLED_FLAG], NULL,
        "Loop filter across tiles enabled or disabled"
    },
    {
        ARGS_NO_KEY,  "rc-type", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_RC_TYPE], NULL,
        "Rate control type, (0: OFF, 1: CBR)"
    },
    {
        ARGS_NO_KEY,  "bitrate", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_BPS], NULL,
        "Bitrate in terms of Bits per second: Kbps(none,K,k), Mbps(M,m)\n"
        "      ex) 100 / 100K / 0.1M"
    },
    {
        ARGS_NO_KEY,  "vbv-msec", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_VBV_MSEC], NULL,
        "VBV buffer size in msec"
    },
    {
        ARGS_NO_KEY,  "vbv-bufsize", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_VBV_BUF_SIZE], NULL,
        "VBV buffer size: Kbits(none,K,k), Mbits(M,m)\n"
        "      ex) 100 / 100K / 0.1M"
    },
    {
        ARGS_NO_KEY,  "use_filler", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_USE_FILLER_FLAG], NULL,
        "user filler flag"
    },
    {
        ARGS_NO_KEY,  "lookahead", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_NUM_PRE_ANALYSIS_FRAMES], NULL,
        "number of pre analysis frames for rate control and cutree, disable:0"
    },
    {
        ARGS_NO_KEY,  "chroma_qp_table_present_flag", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_CHROMA_QP_TABLE_PRESENT_FLAG], NULL,
        "chroma_qp_table_present_flag"
    },
    {
        ARGS_NO_KEY,  "chroma_qp_num_points_in_table", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_CHROMA_QP_NUM_POINTS_IN_TABLE], NULL,
        "Number of pivot points for Cb and Cr channels"
    },
    {
        ARGS_NO_KEY,  "chroma_qp_delta_in_val_cb", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_CHROMA_QP_DELTA_IN_VAL_CB], NULL,
        "Array of input pivot points for Cb"
    },
    {
        ARGS_NO_KEY,  "chroma_qp_delta_out_val_cb", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_CHROMA_QP_DELTA_OUT_VAL_CB], NULL,
        "Array of input pivot points for Cb"
    },
    {
        ARGS_NO_KEY,  "chroma_qp_delta_in_val_cr", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_CHROMA_QP_DELTA_IN_VAL_CR], NULL,
        "Array of input pivot points for Cr"
    },
    {
        ARGS_NO_KEY,  "chroma_qp_delta_out_val_cr", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_CHROMA_QP_DELTA_OUT_VAL_CR], NULL,
        "Array of input pivot points for Cr"
    },

    {
        ARGS_NO_KEY,  "dra_enable_flag", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_DRA_ENABLE_FLAG], NULL,
        "DRA enable flag"
    },
    {
        ARGS_NO_KEY,  "dra_number_ranges", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_DRA_NUMBER_RANGES], NULL,
        "Number of DRA ranges"
    },
    {
        ARGS_NO_KEY,  "dra_range", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_DRA_RANGE], NULL,
        "Array of dra ranges"
    },
    {
        ARGS_NO_KEY,  "dra_scale", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_DRA_SCALE], NULL,
        "Array of input dra ranges"
    },
    {
        ARGS_NO_KEY,  "dra_chroma_qp_scale", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_DRA_CHROMA_QP_SCALE], NULL,
        "DRA chroma qp scale value"
    },
    {
        ARGS_NO_KEY,  "dra_chroma_qp_offset", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_DRA_CHROMA_QP_OFFSET], NULL ,
        "DRA chroma qp offset"
    },
    {
        ARGS_NO_KEY,  "dra_chroma_cb_scale", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_DRA_CHROMA_CB_SCALE], NULL,
        "DRA chroma cb scale"
    },
    {
        ARGS_NO_KEY,  "dra_chroma_cr_scale", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_DRA_CHROMA_CR_SCALE], NULL,
        "DRA chroma cr scale"
    },
    {
        ARGS_NO_KEY,  "dra_hist_norm", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_DRA_HIST_NORM], NULL,
        "DRA hist norm"
    },
    {
        ARGS_NO_KEY,  "rpl_extern", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_FLAG_RPL_EXTERN], NULL,
        "Whether to input external RPL"
    },
    {
        ARGS_NO_KEY,  "RPL0_0", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_0], NULL,
        "RPL0_0"
    },
    {
        ARGS_NO_KEY,  "RPL0_1", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_1], NULL,
        "RPL0_1"
    },
    {
        ARGS_NO_KEY,  "RPL0_2", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_2], NULL,
        "RPL0_2"
    },
    {
        ARGS_NO_KEY,  "RPL0_3", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_3], NULL,
        "RPL0_3"
    },
    {
        ARGS_NO_KEY,  "RPL0_4", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_4], NULL,
        "RPL0_4"
    },
    {
        ARGS_NO_KEY,  "RPL0_5", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_5], NULL,
        "RPL0_5"
    },
    {
        ARGS_NO_KEY,  "RPL0_6", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_6], NULL,
        "RPL0_6"
    },
    {
        ARGS_NO_KEY,  "RPL0_7", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_7], NULL,
        "RPL0_7"
    },
    {
        ARGS_NO_KEY,  "RPL0_8", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_8], NULL,
        "RPL0_8"
    },
    {
        ARGS_NO_KEY,  "RPL0_9", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_9], NULL,
        "RPL0_9"
    },
    {
        ARGS_NO_KEY,  "RPL0_10", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_10], NULL,
        "RPL0_10"
    },
    {
        ARGS_NO_KEY,  "RPL0_11", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_11], NULL,
        "RPL0_11"
    },
    {
        ARGS_NO_KEY,  "RPL0_12", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_12], NULL,
        "RPL0_12"
    },
    {
        ARGS_NO_KEY,  "RPL0_13", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_13], NULL,
        "RPL0_13"
    },
    {
        ARGS_NO_KEY,  "RPL0_14", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_14], NULL,
        "RPL0_14"
    },
    {
        ARGS_NO_KEY,  "RPL0_15", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_15], NULL,
        "RPL0_15"
    },
    {
        ARGS_NO_KEY,  "RPL0_16", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_16], NULL,
        "RPL0_16"
    },
    {
        ARGS_NO_KEY,  "RPL0_17", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_17], NULL,
        "RPL0_17"
    },
    {
        ARGS_NO_KEY,  "RPL0_18", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_18], NULL,
        "RPL0_18"
    },
    {
        ARGS_NO_KEY,  "RPL0_19", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_19], NULL,
        "RPL0_19"
    },
    {
        ARGS_NO_KEY,  "RPL0_20", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_20], NULL,
        "RPL0_20"
    },
    {
        ARGS_NO_KEY,  "RPL0_21", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_21], NULL,
        "RPL0_21"
    },
    {
        ARGS_NO_KEY,  "RPL0_22", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_22], NULL,
        "RPL0_22"
    },
    {
        ARGS_NO_KEY,  "RPL0_23", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_23], NULL,
        "RPL0_23"
    },
    {
        ARGS_NO_KEY,  "RPL0_24", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_24], NULL,
        "RPL0_24"
    },
    {
        ARGS_NO_KEY,  "RPL0_25", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL0_25], NULL,
        "RPL0_25"
    },
    {
        ARGS_NO_KEY,  "RPL1_0", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_0], NULL,
        "RPL1_0"
    },
    {
        ARGS_NO_KEY,  "RPL1_1", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_1], NULL,
        "RPL1_1"
    },
    {
        ARGS_NO_KEY,  "RPL1_2", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_2], NULL,
        "RPL1_2"
    },
    {
        ARGS_NO_KEY,  "RPL1_3", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_3], NULL,
        "RPL1_3"
    },
    {
        ARGS_NO_KEY,  "RPL1_4", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_4], NULL,
        "RPL1_4"
    },
    {
        ARGS_NO_KEY,  "RPL1_5", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_5], NULL,
        "RPL1_5"
    },
    {
        ARGS_NO_KEY,  "RPL1_6", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_6], NULL,
        "RPL1_6"
    },
    {
        ARGS_NO_KEY,  "RPL1_7", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_7], NULL,
        "RPL1_7"
    },
    {
        ARGS_NO_KEY,  "RPL1_8", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_8], NULL,
        "RPL1_8"
    },
    {
        ARGS_NO_KEY,  "RPL1_9", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_9], NULL,
        "RPL1_9"
    },
    {
        ARGS_NO_KEY,  "RPL1_10", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_10], NULL,
        "RPL1_10"
    },
    {
        ARGS_NO_KEY,  "RPL1_11", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_11], NULL,
        "RPL1_11"
    },
    {
        ARGS_NO_KEY,  "RPL1_12", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_12], NULL,
        "RPL1_12"
    },
    {
        ARGS_NO_KEY,  "RPL1_13", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_13], NULL,
        "RPL1_13"
    },
    {
        ARGS_NO_KEY,  "RPL1_14", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_14], NULL,
        "RPL1_14"
    },
    {
        ARGS_NO_KEY,  "RPL1_15", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_15], NULL,
        "RPL1_15"
    },
    {
        ARGS_NO_KEY,  "RPL1_16", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_16], NULL,
        "RPL1_16"
    },
    {
        ARGS_NO_KEY,  "RPL1_17", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_17], NULL,
        "RPL1_17"
    },
    {
        ARGS_NO_KEY,  "RPL1_18", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_18], NULL,
        "RPL1_18"
    },
    {
        ARGS_NO_KEY,  "RPL1_19", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_19], NULL,
        "RPL1_19"
    },
    {
        ARGS_NO_KEY,  "RPL1_20", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_20], NULL,
        "RPL1_20"
    },
    {
        ARGS_NO_KEY,  "RPL1_21", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_21], NULL,
        "RPL1_21"
    },
    {
        ARGS_NO_KEY,  "RPL1_22", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_22], NULL,
        "RPL1_22"
    },
    {
        ARGS_NO_KEY,  "RPL1_23", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_23], NULL,
        "RPL1_23"
    },
    {
        ARGS_NO_KEY,  "RPL1_24", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_24], NULL,
        "RPL1_24"
    },
    {
        ARGS_NO_KEY,  "RPL1_25", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_FLAG_RPL1_25], NULL,
        "RPL1_25"
    },
    {
        ARGS_NO_KEY,  "inter_slice_type", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_INTER_SLICE_TYPE], NULL,
        "INTER_SLICE_TYPE"
    },
    {
        ARGS_NO_KEY,  "picture_cropping_flag", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_PIC_CROP_FLAG], NULL,
        "picture crop flag"
    },
    {
        ARGS_NO_KEY,  "picture_crop_left", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_PIC_CROP_LEFT], NULL,
        "left offset of picture crop"
    },
    {
        ARGS_NO_KEY,  "picture_crop_right", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_PIC_CROP_RIGHT], NULL,
        "right offset of picture crop"
    },
    {
        ARGS_NO_KEY,  "picture_crop_top", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_PIC_CROP_TOP], NULL,
        "top offset of picture crop"
    },
    {
        ARGS_NO_KEY,  "picture_crop_bottom", ARGS_VAL_TYPE_INTEGER,
        &op_flag[OP_PIC_CROP_BOTTOM], NULL,
        "bottom offset of picture crop"
    },
    {
        ARGS_NO_KEY,  "preset", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_PRESET], NULL,
        "Encoder PRESET"
        "\t [fast, medium, slow, placebo]"
    },
    {
        ARGS_NO_KEY,  "tune", ARGS_VAL_TYPE_STRING,
        &op_flag[OP_TUNE], NULL,
        "Encoder TUNE"
        "\t [psnr, zerolatency]"
    },
    {0, "", ARGS_VAL_TYPE_NONE, NULL, NULL, ""} /* termination */
};

#define ARGS_SET_VARIABLE(opt, opt_idx, var) (opt)[opt_idx].val = (var)
#define ARGS_SET_FLAG(opt, opt_idx, b) (*((opt)[opt_idx].flag)) = (b)
#define ARGS_SET_VAL_INT(opt, opt_idx, v) {(*((opt)[opt_idx].v)) = (v); ARGS_SET_FLAG(opt, opt_idx, 1);}
#define ARGS_GET_VAL_INT(opt, opt_idx, v) (*((opt)[opt_idx].v))

static int set_variables_to_parse_val(XEVE_PARAM* param)
{
    ARGS_SET_VARIABLE(options, OP_FLAG_FNAME_CFG, op_fname_cfg);
    ARGS_SET_VARIABLE(options, OP_FLAG_FNAME_INP, op_fname_inp);
    ARGS_SET_VARIABLE(options, OP_FLAG_FNAME_OUT, op_fname_out);
    ARGS_SET_VARIABLE(options, OP_FLAG_FNAME_REC, op_fname_rec);
    ARGS_SET_VARIABLE(options, OP_FLAG_WIDTH_INP, &param->w);
    ARGS_SET_VARIABLE(options, OP_FLAG_HEIGHT_INP, &param->h);
    ARGS_SET_VARIABLE(options, OP_FLAG_QP, &param->qp);
    ARGS_SET_VARIABLE(options, OP_FLAG_AQMODE, &param->aq_mode);
    ARGS_SET_VARIABLE(options, OP_FLAG_CUTREE, &param->cutree);
    ARGS_SET_VARIABLE(options, OP_FLAG_CU_QP_DELTA_AREA, &param->cu_qp_delta_area);
    ARGS_SET_VARIABLE(options, OP_FLAG_FPS, &param->fps);
    ARGS_SET_VARIABLE(options, OP_FLAG_IPERIOD, &param->iperiod);
    ARGS_SET_VARIABLE(options, OP_FLAG_MAX_B_FRAMES, &param->max_b_frames);
    ARGS_SET_VARIABLE(options, OP_THREADS, &param->threads);
    ARGS_SET_VARIABLE(options, OP_FLAG_MAX_FRM_NUM, &op_max_frm_num);
    ARGS_SET_VARIABLE(options, OP_FLAG_USE_PIC_SIGN, &op_use_pic_signature);
    ARGS_SET_VARIABLE(options, OP_FLAG_VERBOSE, &op_verbose);
    ARGS_SET_VARIABLE(options, OP_FLAG_IN_BIT_DEPTH, &op_inp_bit_depth);
    ARGS_SET_VARIABLE(options, OP_FLAG_CODEC_BIT_DEPTH, &param->codec_bit_depth);
    ARGS_SET_VARIABLE(options, OP_FLAG_CHROMA_FORMAT_IDC, &op_chroma_format_idc);
    ARGS_SET_VARIABLE(options, OP_FLAG_RDO_DBK_SWITCH, &param->rdo_dbk_switch);
    ARGS_SET_VARIABLE(options, OP_FLAG_OUT_BIT_DEPTH, &op_out_bit_depth);
    ARGS_SET_VARIABLE(options, OP_REF_PIC_GAP, &param->ref_pic_gap_length);
    ARGS_SET_VARIABLE(options, OP_FLAG_CLOSED_GOP, &param->closed_gop);
    ARGS_SET_VARIABLE(options, OP_FLAG_IBC, &param->ibc_flag);
    ARGS_SET_VARIABLE(options, OP_IBC_SEARCH_RNG_X, &param->ibc_search_range_x);
    ARGS_SET_VARIABLE(options, OP_IBC_SEARCH_RND_Y, &param->ibc_search_range_y);
    ARGS_SET_VARIABLE(options, OP_IBC_HASH_FLAG, &param->ibc_hash_search_flag);
    ARGS_SET_VARIABLE(options, OP_IBC_HASH_SEARCH_MAX_CAND, &param->ibc_hash_search_max_cand);
    ARGS_SET_VARIABLE(options, OP_IBC_HASH_SEARCH_RANGE_4SMALLBLK, &param->ibc_hash_search_range_4smallblk);
    ARGS_SET_VARIABLE(options, OP_IBC_FAST_METHOD, &param->ibc_fast_method);
    ARGS_SET_VARIABLE(options, OP_FLAG_DISABLE_HGOP, &param->disable_hgop);
    ARGS_SET_VARIABLE(options, OP_FLAG_SKIP_FRAMES, &op_skip_frames);
    ARGS_SET_VARIABLE(options, OP_PROFILE, op_profile);
    ARGS_SET_VARIABLE(options, OP_LEVEL, &param->level);
    ARGS_SET_VARIABLE(options, OP_BTT, &param->btt);
    ARGS_SET_VARIABLE(options, OP_SUCO, &param->suco);
    ARGS_SET_VARIABLE(options, OP_FLAG_ADD_QP_FRAME, &param->qp_incread_frame);
    ARGS_SET_VARIABLE(options, OP_FRAMEWORK_CB_MAX, &param->framework_cb_max);
    ARGS_SET_VARIABLE(options, OP_FRAMEWORK_CB_MIN, &param->framework_cb_min);
    ARGS_SET_VARIABLE(options, OP_FRAMEWORK_CU14_MAX, &param->framework_cu14_max);
    ARGS_SET_VARIABLE(options, OP_FRAMEWORK_TRIS_MAX, &param->framework_tris_max);
    ARGS_SET_VARIABLE(options, OP_FRAMEWORK_TRIS_MIN, &param->framework_tris_min);
    ARGS_SET_VARIABLE(options, OP_FRAMEWORK_SUCO_MAX, &param->framework_suco_max);
    ARGS_SET_VARIABLE(options, OP_FRAMEWORK_SUCO_MIN, &param->framework_suco_min);
    ARGS_SET_VARIABLE(options, OP_TOOL_AMVR, &param->tool_amvr);
    ARGS_SET_VARIABLE(options, OP_TOOL_MMVD, &param->tool_mmvd);
    ARGS_SET_VARIABLE(options, OP_TOOL_AFFINE, &param->tool_affine);
    ARGS_SET_VARIABLE(options, OP_TOOL_DMVR, &param->tool_dmvr);
    ARGS_SET_VARIABLE(options, OP_TOOL_ADDB, &param->tool_addb);
    ARGS_SET_VARIABLE(options, OP_TOOL_ALF, &param->tool_alf);
    ARGS_SET_VARIABLE(options, OP_TOOL_HTDF, &param->tool_htdf);
    ARGS_SET_VARIABLE(options, OP_TOOL_ADMVP, &param->tool_admvp);
    ARGS_SET_VARIABLE(options, OP_TOOL_HMVP, &param->tool_hmvp);
    ARGS_SET_VARIABLE(options, OP_TOOL_EIPD, &param->tool_eipd);
    ARGS_SET_VARIABLE(options, OP_TOOL_IQT, &param->tool_iqt);
    ARGS_SET_VARIABLE(options, OP_TOOL_CM_INIT, &param->tool_cm_init);
    ARGS_SET_VARIABLE(options, OP_TOOL_ADCC, &param->tool_adcc);
    ARGS_SET_VARIABLE(options, OP_TOOL_RPL, &param->tool_rpl);
    ARGS_SET_VARIABLE(options, OP_TOOL_POCS, &param->tool_pocs);
    ARGS_SET_VARIABLE(options, OP_QP_CB_OFFSET, &param->qp_cb_offset);
    ARGS_SET_VARIABLE(options, OP_QP_CR_OFFSET, &param->qp_cr_offset);
    ARGS_SET_VARIABLE(options, OP_TOOL_ATS, &param->tool_ats);
    ARGS_SET_VARIABLE(options, OP_CONSTRAINED_INTRA_PRED, &param->constrained_intra_pred);
    ARGS_SET_VARIABLE(options, OP_TOOL_DBF, &param->tool_addb);
    ARGS_SET_VARIABLE(options, OP_TOOL_DBFOFFSET_A, &param->deblock_alpha_offset);
    ARGS_SET_VARIABLE(options, OP_TOOL_DBFOFFSET_B, &param->deblock_beta_offset);
    ARGS_SET_VARIABLE(options, OP_TILE_UNIFORM_SPACING, &param->tile_uniform_spacing_flag);
    ARGS_SET_VARIABLE(options, OP_NUM_TILE_COLUMNS, &param->tile_columns);
    ARGS_SET_VARIABLE(options, OP_NUM_TILE_ROWS, &param->tile_rows);
    ARGS_SET_VARIABLE(options, OP_TILE_COLUMN_WIDTH_ARRAY, &param->tile_column_width_array);
    ARGS_SET_VARIABLE(options, OP_TILE_ROW_HEIGHT_ARRAY, &param->tile_row_height_array);
    ARGS_SET_VARIABLE(options, OP_NUM_SLICE_IN_PIC, &param->num_slice_in_pic);
    ARGS_SET_VARIABLE(options, OP_SLICE_BOUNDARY_ARRAY, &param->tile_array_in_slice);
    ARGS_SET_VARIABLE(options, OP_ARBITRAY_SLICE_FLAG, &param->arbitrary_slice_flag);
    ARGS_SET_VARIABLE(options, OP_NUM_REMAINING_TILES_IN_SLICE, &param->num_remaining_tiles_in_slice_minus1);
    ARGS_SET_VARIABLE(options, OP_LOOP_FILTER_ACROSS_TILES_ENABLED_FLAG, &param->loop_filter_across_tiles_enabled_flag);
    ARGS_SET_VARIABLE(options, OP_RC_TYPE, &param->rc_type);
    ARGS_SET_VARIABLE(options, OP_BPS, &param->bps);
    ARGS_SET_VARIABLE(options, OP_VBV_MSEC, &param->vbv_buf_msec);
    ARGS_SET_VARIABLE(options, OP_VBV_BUF_SIZE, &param->vbv_buf_size);
    ARGS_SET_VARIABLE(options, OP_USE_FILLER_FLAG, &param->use_filler_flag);
    ARGS_SET_VARIABLE(options, OP_NUM_PRE_ANALYSIS_FRAMES, &param->lookahead);
    ARGS_SET_VARIABLE(options, OP_CHROMA_QP_TABLE_PRESENT_FLAG, &param->chroma_qp_table_present_flag);
    ARGS_SET_VARIABLE(options, OP_CHROMA_QP_NUM_POINTS_IN_TABLE, param->chroma_qp_num_points_in_table);
    ARGS_SET_VARIABLE(options, OP_CHROMA_QP_DELTA_IN_VAL_CB, param->chroma_qp_delta_in_val_cb);
    ARGS_SET_VARIABLE(options, OP_CHROMA_QP_DELTA_OUT_VAL_CB, param->chroma_qp_delta_out_val_cb);
    ARGS_SET_VARIABLE(options, OP_CHROMA_QP_DELTA_IN_VAL_CR, param->chroma_qp_delta_in_val_cr);
    ARGS_SET_VARIABLE(options, OP_CHROMA_QP_DELTA_OUT_VAL_CR, param->chroma_qp_delta_out_val_cr);
    ARGS_SET_VARIABLE(options, OP_DRA_ENABLE_FLAG, &param->tool_dra);
    ARGS_SET_VARIABLE(options, OP_DRA_NUMBER_RANGES, &param->dra_number_ranges);
    ARGS_SET_VARIABLE(options, OP_DRA_RANGE, param->dra_range);
    ARGS_SET_VARIABLE(options, OP_DRA_SCALE, param->dra_scale);
    ARGS_SET_VARIABLE(options, OP_DRA_CHROMA_QP_SCALE, param->dra_chroma_qp_scale);
    ARGS_SET_VARIABLE(options, OP_DRA_CHROMA_QP_OFFSET, param->dra_chroma_qp_offset);
    ARGS_SET_VARIABLE(options, OP_DRA_CHROMA_CB_SCALE, param->dra_chroma_cb_scale);
    ARGS_SET_VARIABLE(options, OP_DRA_CHROMA_CR_SCALE, param->dra_chroma_cr_scale);
    ARGS_SET_VARIABLE(options, OP_DRA_HIST_NORM, param->dra_hist_norm);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL_EXTERN, &param->rpl_extern);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_0, param->rpl0[0]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_1, param->rpl0[1]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_2, param->rpl0[2]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_3, param->rpl0[3]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_4, param->rpl0[4]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_5, param->rpl0[5]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_6, param->rpl0[6]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_7, param->rpl0[7]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_8, param->rpl0[8]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_9, param->rpl0[9]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_10, param->rpl0[10]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_11, param->rpl0[11]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_12, param->rpl0[12]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_13, param->rpl0[13]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_14, param->rpl0[14]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_15, param->rpl0[15]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_16, param->rpl0[16]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_17, param->rpl0[17]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_18, param->rpl0[18]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_19, param->rpl0[19]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_20, param->rpl0[20]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_21, param->rpl0[21]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_22, param->rpl0[22]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_23, param->rpl0[23]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_24, param->rpl0[24]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL0_25, param->rpl0[25]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_0, param->rpl1[0]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_1, param->rpl1[1]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_2, param->rpl1[2]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_3, param->rpl1[3]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_4, param->rpl1[4]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_5, param->rpl1[5]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_6, param->rpl1[6]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_7, param->rpl1[7]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_8, param->rpl1[8]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_9, param->rpl1[9]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_10, param->rpl1[10]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_11, param->rpl1[11]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_12, param->rpl1[12]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_13, param->rpl1[13]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_14, param->rpl1[14]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_15, param->rpl1[15]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_16, param->rpl1[16]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_17, param->rpl1[17]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_18, param->rpl1[18]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_19, param->rpl1[19]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_20, param->rpl1[20]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_21, param->rpl1[21]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_22, param->rpl1[22]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_23, param->rpl1[23]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_24, param->rpl1[24]);
    ARGS_SET_VARIABLE(options, OP_FLAG_RPL1_25, param->rpl1[25]);
    ARGS_SET_VARIABLE(options, OP_INTER_SLICE_TYPE, &param->inter_slice_type);
    ARGS_SET_VARIABLE(options, OP_PIC_CROP_FLAG, &param->picture_cropping_flag);
    ARGS_SET_VARIABLE(options, OP_PIC_CROP_LEFT, &param->picture_crop_left_offset);
    ARGS_SET_VARIABLE(options, OP_PIC_CROP_RIGHT, &param->picture_crop_right_offset);
    ARGS_SET_VARIABLE(options, OP_PIC_CROP_TOP, &param->picture_crop_top_offset);
    ARGS_SET_VARIABLE(options, OP_PIC_CROP_BOTTOM, &param->picture_crop_bottom_offset);
    ARGS_SET_VARIABLE(options, OP_PRESET, op_preset);
    ARGS_SET_VARIABLE(options, OP_TUNE, op_tune);

    return 0;
}

static int args_parse_cfg(FILE* fp, ARGS_OPTION* ops, int is_type_ppt)
{
    char* parser;
    char line[256] = "", tag[50] = "", val[256] = "";
    int oidx;

    while (fgets(line, sizeof(line), fp))
    {
        parser = strchr(line, '#');
        if (parser != NULL) *parser = '\0';

        parser = strtok(line, "= \t");
        if (parser == NULL) continue;
        strcpy(tag, parser);

        parser = strtok(NULL, "=\n");
        if (parser == NULL) continue;
        strcpy(val, parser);

        oidx = args_search_long_arg(ops, tag);
        if (oidx < 0) continue;

        if (ops[oidx].val == NULL)
        {
            return -1;
        }

        if (ARGS_GET_IS_OPT_TYPE_PPT(ops[oidx].val_type) == is_type_ppt)
        {
            if (ARGS_GET_CMD_OPT_VAL_TYPE(ops[oidx].val_type) != ARGS_VAL_TYPE_NONE)
            {
                if (args_read_value(ops + oidx, val)) continue;
            }
            else
            {
                *((int*)ops[oidx].val) = 1;
            }
            *ops[oidx].flag = 1;
        }
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

static int args_parse_cmd2(int argc, const char* argv[], ARGS_OPTION* ops, int* idx, int is_type_ppt)
{
    int    aidx; /* arg index */
    int    oidx; /* option index */

    aidx = *idx + 1;

    if (aidx >= argc || argv[aidx] == NULL) goto NO_MORE;
    if (argv[aidx][0] != '-') goto ERR;

    if (argv[aidx][1] == '-')
    {
        /* long option */
        oidx = args_search_long_arg(ops, argv[aidx] + 2);
        if (oidx < 0) goto ERR;
    }
    else if (strlen(argv[aidx]) == 2)
    {
        /* short option */
        oidx = args_search_short_arg(ops, argv[aidx][1]);
        if (oidx < 0) goto ERR;
    }
    else
    {
        goto ERR;
    }

    if (ops[oidx].val == NULL)
    {
        goto ERR;
    }

    if (ARGS_GET_IS_OPT_TYPE_PPT(ops[oidx].val_type) != is_type_ppt)
    {
        if (ARGS_GET_CMD_OPT_VAL_TYPE(ops[oidx].val_type) != ARGS_VAL_TYPE_NONE)
        {
            *idx = *idx + 1;
        }
        goto END;
    }

    if (ARGS_GET_CMD_OPT_VAL_TYPE(ops[oidx].val_type) != ARGS_VAL_TYPE_NONE)
    {
        if (aidx + 1 >= argc) goto ERR;
        if (args_read_value(ops + oidx, argv[aidx + 1])) goto ERR;
        *idx = *idx + 1;
    }
    else
    {
        *((int*)ops[oidx].val) = 1;
    }
    *ops[oidx].flag = 1;
END:
    *idx = *idx + 1;
    return ops[oidx].key;

NO_MORE:
    return 0;

ERR:
    return -1;
}

static int args_check_mandatory_param(ARGS_OPTION* ops, char ** err_arg)
{
    ARGS_OPTION * o = ops;

    while (o->key != 0)
    {
        if (o->val_type & ARGS_VAL_TYPE_MANDATORY)
        {
            if (*o->flag == 0)
            {
                /* not filled all mandatory argument */
                *err_arg = o->key_long;
                return -1;
            }
        }
        o++;
    }
    return 0;
}

static int args_parse_all(int argc, const char* argv[], ARGS_OPTION* ops, XEVE_PARAM* param)
{
    int i, ret = 0, idx = 0;
    const char* fname_cfg = NULL;
    FILE* fp;

    int num_configs = 0;
    int pos_conf_files[ARGS_MAX_NUM_CONF_FILES];
    memset(&pos_conf_files, -1, sizeof(int) * ARGS_MAX_NUM_CONF_FILES);

    set_variables_to_parse_val(param);

    /* config file parsing */
    for (i = 1; i < argc; i++)
    {
        if (!strcmp(argv[i], "--"ARGS_KEY_LONG_CONFIG))
        {
            if (i + 1 < argc)
            {
                num_configs++;
                pos_conf_files[num_configs - 1] = i + 1;
            }
        }
    }
    for (int i = 0; i < num_configs; i++)
    {
        fname_cfg = argv[pos_conf_files[i]];
        if (fname_cfg)
        {
            fp = fopen(fname_cfg, "r");
            if (fp == NULL) return -1; /* config file error */

            if (args_parse_cfg(fp, ops, 1))
            {
                fclose(fp);
                return -1; /* config file error */
            }
            fclose(fp);
        }
    }
    /* command line parsing */
    while (1)
    {
        ret = args_parse_cmd(argc, argv, ops, &idx);
        if (ret <= 0) break;
    }
    return ret;
}

int args_get_profile_preset_tune(int * profile, int * preset, int *tune)
{
    int tprofile, tpreset, ttune;

    if (!strcmp(op_profile, "baseline")) tprofile = XEVE_PROFILE_BASELINE;
    else if (!strcmp(op_profile, "main")) tprofile = XEVE_PROFILE_MAIN;
    else return -1;

    if (!strcmp(op_preset, "fast")) tpreset = XEVE_PRESET_FAST;
    else if (!strcmp(op_preset, "medium")) tpreset = XEVE_PRESET_MEDIUM;
    else if (!strcmp(op_preset, "slow")) tpreset = XEVE_PRESET_SLOW;
    else if (!strcmp(op_preset, "placebo")) tpreset = XEVE_PRESET_PLACEBO;
    else return -1;

    if (op_flag[OP_TUNE]) {
        if (!strcmp(op_tune, "zerolatency")) ttune = XEVE_TUNE_ZEROLATENCY;
        else if (!strcmp(op_tune, "psnr")) ttune = XEVE_TUNE_PSNR;
        else return -1;
    }
    else ttune = XEVE_TUNE_NONE;

    *profile = tprofile;
    *preset = tpreset;
    *tune = ttune;

    return 0;
}

#define NUM_ARG_OPTION   ((int)(sizeof(options)/sizeof(options[0]))-1)

#endif /*_XEVE_APP_ARGS_H_ */


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

#include "xeve_args.h"

#include "xeve.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <limits.h>

#define ARGS_END_KEY                    (0)
#define ARGS_NO_KEY                     (127)
#define ARGS_KEY_LONG_CONFIG            "config"
#define ARGS_MAX_NUM_CONF_FILES         (16)

#define ARGS_MAX_KEY_LONG               (64)

#define ARGS_OPT_DEFAULT_VALUE_MAXLEN   (256)
#define  ARGS_OPT_ALLOWED_VALUES_MAXLEN (512)

#define ARGS_OPT_HELP_STRING_MAXLEN     (1024)

typedef struct Range {
    int min;
    int max;
} Range;

#define MAX_LIST_ELEMENTS 20

#define INT_LIST_END { INT_MIN, NULL }
#define STR_LIST_END { NULL, NULL }

typedef enum ValueType {
    VAL_TYPE_NONE   = (1 << 2),
    VAL_TYPE_INT    = (2 << 2), /* integer type value */
    VAL_TYPE_STRING = (3 << 2)  /* string type value */
} ValueType;

typedef enum ValuesRageType {
    AVT_ALLOWED_VALUES_LIST,
    AVT_MIN_MAX_RANGE,
 } ValuesRageType;

typedef struct IntOption {
    int value;
    char* descr;
} IntOption;

typedef struct StrOption {
    char* value;
    char* descr;
} StrOption;

typedef struct List {
    union {
        IntOption integers[MAX_LIST_ELEMENTS];
        StrOption strings[MAX_LIST_ELEMENTS];
    };
    size_t size;
} List;

/* Structure for storing command line option data */
typedef struct _ARGS_OPT {
    char   key;                             /* option keyword. ex) -f */
    char   key_long[ARGS_MAX_KEY_LONG];     /* option long keyword, ex) --file */
    ValueType value_type;
    int has_default;
    union {
        IntOption integer;
        StrOption string;
    } default_value;                        /* default value */

    ValuesRageType values_rage_type;        /* range or list */
    union {
        Range range;
        List list;
    } allowed_values;                       /* allowed values */

    int is_mandatory;                       /* flag: 0 - option is optional; 1 - option is mandatory */
    int is_set;                             /* flag: 0 - option is not set; 1 - option set */
    char* desc;                             /* option description */
    void* opt_storage;                      /* pointer to option storage */
} ARGS_OPT;

/* Define various command line options as a table */
ARGS_OPT args_opt_table[] = {
        // verbose
        { .key = 'v', .key_long = "verbose", .value_type = VAL_TYPE_INT,
                                            .has_default = 1, .default_value = { .integer = {0, "no message"} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 4, .integers = { {0, "no message"}, {1, "only error message"}, {2, "simple messages"}, {3, "frame-level messages"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "verbose (log) level",
                                            .opt_storage = NULL },

        // input
        { .key = 'i', .key_long = "input", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = STR_LIST_END },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings =  {{NULL,NULL}} } },
                                            .is_mandatory = 1, .is_set = 0,
                                            .desc = "file name of input video (raw YUV or Y4M), `stdin` for standard input instead of regular file",
                                            .opt_storage = NULL },

        // output
        { .key = 'o', .key_long = "output", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"", NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings =  {{NULL,NULL}} } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "file name of output bitstream",
                                            .opt_storage = NULL },

        // recon
        { .key = 'r', .key_long = "recon", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"", NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings =  {{NULL,NULL}} } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "file name of reconstructed video",
                                            .opt_storage = NULL },

        // width
        { .key = 'w', .key_long = "width", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range = {128,3840} }, // SQCIF	128 × 96 / 4K 3840 x 2160 / 8K 7680 x 4320
                                            .is_mandatory = 1, .is_set = 0,
                                            .desc = "pixel width of input video",
                                            .opt_storage = NULL },

        // height
        { .key = 'h', .key_long = "height", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range = {96,2160} }, // SQCIF	128 × 96 / 4K 3840 x 2160 / 8K 7680 x 4320
                                            .is_mandatory = 1, .is_set = 0,
                                            .desc = "pixel height of input video",
                                            .opt_storage = NULL },

        // qp
        { .key = 'q', .key_long = "qp", .value_type = VAL_TYPE_INT,
                                            .has_default = 1, .default_value = { .integer = {17, NULL} },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range = {0,51} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "QP value",
                                            .opt_storage = NULL },

        // fps
        { .key = 'z', .key_long = "fps", .value_type = VAL_TYPE_INT,
                                            .has_default = 1, .default_value = { .integer = {30, NULL} },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, 60} },
                                            .is_mandatory = 1, .is_set = 0,
                                            .desc = "frame rate (frame per second)",
                                            .opt_storage = NULL },

        // keyint
        { .key = 'I', .key_long = "keyint", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range = {0,INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "I-picture period",
                                            .opt_storage = NULL },

        // bframes
        { .key = 'b', .key_long = "bframes", .value_type = VAL_TYPE_INT,
                                            .has_default = 1, .default_value = { .integer = {15 , NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 4, .integers = { {1}, {3}, {7}, {15}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "maximum number of B frames",
                                            .opt_storage = NULL },

        // threads
        { .key = 'm', .key_long = "threads", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range = {0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "force to use a specific number of threads",
                                            . opt_storage = NULL },

        // input-depth
        { .key = 'd', .key_long = "input-depth", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = {8, NULL } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers = { {8,NULL}, {10,NULL}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "input bit depth (8, 10)",
                                            .opt_storage = NULL },

        // codec-bit-depth
        { .key = ARGS_NO_KEY, .key_long = "codec-bit-depth", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = {10, NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers = { {10,NULL}, {12,NULL}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "codec internal bit depth",
                                            .opt_storage = NULL },

        // input-csp
        { .key = ARGS_NO_KEY, .key_long = "input-csp", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = {1, "YUV420"}} ,
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers = {{0, "YUV400"}, {1,"YUV420"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "input color space (chroma format)",
                                            .opt_storage = NULL },

        // profile
        { .key = ARGS_NO_KEY, .key_long = "profile", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"baseline", NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .strings = {{"main"},{"baseline"}, STR_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "profile setting flag  (main, baseline)",
                                            .opt_storage = NULL },

        // level-idc
        { .key = ARGS_NO_KEY, .key_long = "level-idc", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range = {0,INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "level setting",
                                            .opt_storage = NULL },

        // preset
        { .key = ARGS_NO_KEY, .key_long = "preset", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"fast", NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 4, .strings = {{"fast", NULL}, {"medium", NULL}, {"slow", NULL}, {"placebo", NULL} } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Encoder PRESET",
                                            .opt_storage = NULL },

        // tune
        { .key = ARGS_NO_KEY, .key_long = "tune", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"none"} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 3, .strings = {{"none",NULL}, {"psnr",NULL},{"zerolatency", NULL}, {NULL,NULL} } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Encoder TUNE",
                                            .opt_storage = NULL },

        // aq-mode
        { .key = ARGS_NO_KEY, .key_long = "aq-mode", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = {0, "off"} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers = {{0, "off"},{1,"adaptive quantization"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "use adaptive quantization block qp adaptation",
                                            .opt_storage = NULL },

        // frames
        { .key = ARGS_NO_KEY, .key_long = "frames", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range = {1,INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "maximum number of frames to be encoded",
                                            .opt_storage = NULL },

        // seek
        { .key = ARGS_NO_KEY, .key_long = "seek", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range = {1,INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "number of skipped frames before encoding",
                                            .opt_storage = NULL },

        // info
        { .key = ARGS_NO_KEY, .key_long = "info", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = {0, "off"} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers = {{0, "off"},{1, "emit sei info"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "embed SEI messages identifying encoder parameters and command line arguments",
                                            .opt_storage = NULL },

        // hash
        { .key = ARGS_NO_KEY, .key_long = "hash", .value_type = VAL_TYPE_INT,

                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"embed picture signature"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "embed picture signature (HASH) for conformance checking in decoding",
                                            .opt_storage = NULL },

        // cutree
        { .key = ARGS_NO_KEY, .key_long = "cutree", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = {0, "off"} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers = { {0, "off"}, {1, "on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "use cutree block qp adaptation",
                                            .opt_storage = NULL },

        // cu-qp-delta-area
        { .key = ARGS_NO_KEY, .key_long = "cu-qp-delta-area", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range = {6, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "cu-qp-delta-area (>= 6)",
                                            .opt_storage = NULL },

        // rdo-dbk-switch
        { .key = ARGS_NO_KEY, .key_long = "rdo-dbk-switch", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = {0, "off"} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers = { {0, "off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "switch to on/off rdo-dbk (0, 1)",
                                            .opt_storage = NULL },

        // ref-pic-gap-length
        { .key = ARGS_NO_KEY, .key_long = "ref-pic-gap-length", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = {1,NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 5, .integers = { {1,NULL}, {2,NULL}, {4,NULL}, {8,NULL}, {16,NULL}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "reference picture gap length available when -b is 0",
                                            .opt_storage = NULL },

        // closed-gop
        { .key = ARGS_NO_KEY, .key_long = "closed-gop", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "use closed GOP structure. if not set, open GOP is used",
                                            .opt_storage = NULL },

        // ibc
        { .key = ARGS_NO_KEY, .key_long = "ibc", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "use IBC feature. if not set, IBC feature is disabled",
                                            .opt_storage = NULL },

        // ibc-search-range-x
        { .key = ARGS_NO_KEY, .key_long = "ibc-search-range-x", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "set ibc search range in horizontal direction",
                                            .opt_storage = NULL },

        // ibc-search-range-y
        { .key = ARGS_NO_KEY, .key_long = "ibc-search-range-y", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "set ibc search range in vertical direction",
                                            .opt_storage = NULL },

        // ibc-hash-search-flag
        { .key = ARGS_NO_KEY, .key_long = "ibc-hash-search-flag", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "use IBC hash based block matching search feature. if not set, it is disable",
                                            .opt_storage = NULL },

        // ibc-hash-search-max-cand
        { .key = ARGS_NO_KEY, .key_long = "ibc-hash-search-max-cand", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Max candidates for hash based IBC search",
                                            .opt_storage = NULL },

        // ibc-hash-search-range-4smallblk
        { .key = ARGS_NO_KEY, .key_long = "ibc-hash-search-range-4smallblk", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Small block search range in IBC based search",
                                            .opt_storage = NULL },

        // ibc-fast-method
        { .key = ARGS_NO_KEY, .key_long = "ibc-fast-method", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = {1, "Buffer IBC block vector (current not support)"}},
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"Buffer IBC block vector (current not support)"}, {1,"Adaptive search range"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Fast methods for IBC",
                                            .opt_storage = NULL },

        // disable-hgop
        { .key = ARGS_NO_KEY, .key_long = "disable-hgop", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } } ,
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "disable hierarchical GOP. if not set, hierarchical GOP is used",
                                            .opt_storage = NULL },

        // btt
        { .key = ARGS_NO_KEY, .key_long = "bbt", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "binary and ternary splits on/off flag",
                                            .opt_storage = NULL },

        // suco
        { .key = ARGS_NO_KEY, .key_long = "suco", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, 1} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "split unit coding ordering on/off flag",
                                            .opt_storage = NULL },

        // qp-add-frm
        { .key = ARGS_NO_KEY, .key_long = "qp-add-frm", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "one more qp are added after this number of frames, disable:0",
                                            .opt_storage = NULL },

        // ctu
        { .key = ARGS_NO_KEY, .key_long = "ctu", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Max size of Coding Block (log scale)",
                                            .opt_storage = NULL },

        // min-cu-size
        { .key = ARGS_NO_KEY, .key_long = "min-cu-size", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "MIN size of Coding Block (log scale)",
                                            .opt_storage = NULL },

        // cu14-max
        { .key = ARGS_NO_KEY, .key_long = "cu14-max", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Max size of 4N in 4NxN or Nx4N block (log scale)",
                                            .opt_storage = NULL },

        // tris-max
        { .key = ARGS_NO_KEY, .key_long = "tris-max", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Max size of Tri-split allowed",
                                            .opt_storage = NULL },

        // tris-min
        { .key = ARGS_NO_KEY, .key_long = "tris-min", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Min size of Tri-split allowed",
                                            .opt_storage = NULL },

        // suco-max
        { .key = ARGS_NO_KEY, .key_long = "suco-max", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Max size of suco allowed from top",
                                            .opt_storage = NULL },

        // suco-min
        { .key = ARGS_NO_KEY, .key_long = "suco-min", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Min size of suco allowed from top",
                                            .opt_storage = NULL },

        // amvr
        { .key = ARGS_NO_KEY, .key_long = "amvr", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "amvr on/off flag",
                                            .opt_storage = NULL },

        // mmvd
        { .key = ARGS_NO_KEY, .key_long = "mmvd", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "mmvd on/off flag",
                                            .opt_storage = NULL },

        // affine
        { .key = ARGS_NO_KEY, .key_long = "affine", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "affine on/off flag",
                                            .opt_storage = NULL },

        // dmvr
        { .key = ARGS_NO_KEY, .key_long = "dmvr", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "dmvr on/off flag",
                                            .opt_storage = NULL },

        // addb
        { .key = ARGS_NO_KEY, .key_long = "addb", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "addb on/off flag",
                                            .opt_storage = NULL },

        // alf
        { .key = ARGS_NO_KEY, .key_long = "alf", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "alf on/off flag",
                                            .opt_storage = NULL },

        // htdf
        { .key = ARGS_NO_KEY, .key_long = "htdf", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "htdf on/off flag",
                                            .opt_storage = NULL },

        // admvp
        { .key = ARGS_NO_KEY, .key_long = "admvp", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "admvp on/off flag",
                                            .opt_storage = NULL },

        // hmvp
        { .key = ARGS_NO_KEY, .key_long = "hmvp", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "hmvp on/off flag",
                                            .opt_storage = NULL },

        // eipd
        { .key = ARGS_NO_KEY, .key_long = "eipd", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "eipd on/off flag",
                                            .opt_storage = NULL },

        // iqt
        { .key = ARGS_NO_KEY, .key_long = "iqt", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "iqt on/off flag",
                                            .opt_storage = NULL },

        // cm-init
        { .key = ARGS_NO_KEY, .key_long = "cm-init", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "cm-init on/off flag",
                                            .opt_storage = NULL },

        // adcc
        { .key = ARGS_NO_KEY, .key_long = "adcc", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "adcc on/off flag",
                                            .opt_storage = NULL },

        // rpl
        { .key = ARGS_NO_KEY, .key_long = "rpl", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "rpl on/off flag",
                                            .opt_storage = NULL },

        // pocs
        { .key = ARGS_NO_KEY, .key_long = "pocs", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "pocs on/off flag",
                                            .opt_storage = NULL },

        // qp-cb-offset
        { .key = ARGS_NO_KEY, .key_long = "qp-cb-offset", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = " cb qp offset",
                                            .opt_storage = NULL },

        // qp-cr-offset
        { .key = ARGS_NO_KEY, .key_long = "qp-cr-offset", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "cr qp offset",
                                            .opt_storage = NULL },

        // ats
        { .key = ARGS_NO_KEY, .key_long = "ats", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "ats on/off flag",
                                            .opt_storage = NULL },

        // constrained-intra-pred
        { .key = ARGS_NO_KEY, .key_long = "constrained-intra-pred", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "constrained intra pred",
                                            .opt_storage = NULL },

        // deblock
        { .key = ARGS_NO_KEY, .key_long = "deblock", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Deblocking filter on/off flag",
                                            .opt_storage = NULL },

        // dbfoffsetA
        { .key = ARGS_NO_KEY, .key_long = "dbfoffsetA", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "ADDB Deblocking filter offset for alpha",
                                            .opt_storage = NULL },

        // dbfoffsetB
        { .key = ARGS_NO_KEY, .key_long = "dbfoffsetB", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "ADDB Deblocking filter offset for beta",
                                            .opt_storage = NULL },

        // tile-uniform-spacing
        { .key = ARGS_NO_KEY, .key_long = "tile-uniform-spacing", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "uniform or non-uniform tile spacing",
                                            .opt_storage = NULL },

        // num-tile-columns
        { .key = ARGS_NO_KEY, .key_long = "num-tile-columns", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Number of tile columns",
                                            .opt_storage = NULL },

        // num-tile-rows
        { .key = ARGS_NO_KEY, .key_long = "num-tile-rows", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Number of tile rows",
                                            .opt_storage = NULL },

        // tile-column-width-array
        { .key = ARGS_NO_KEY, .key_long = "tile-column-width-array", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings={ {NULL,NULL} } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Array of Tile Column Width",
                                            .opt_storage = NULL },

        // tile-row-height-array
        { .key = ARGS_NO_KEY, .key_long = "tile-row-height-array", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings={ {NULL,NULL} } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Array of Tile Row Height",
                                            .opt_storage = NULL },

        // num-slices-in-pic
        { .key = ARGS_NO_KEY, .key_long = "num-slices-in-pic", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Number of slices in the pic",
                                            .opt_storage = NULL },

        // tile-array-in-slice
        { .key = ARGS_NO_KEY, .key_long = "tile-array-in-slice", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings={ {NULL,NULL} } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Array of Slice Boundaries",
                                            .opt_storage = NULL },

        // arbitrary-slice-flag
        { .key = ARGS_NO_KEY, .key_long = "arbitrary-slice-flag", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Array of Slice Boundaries",
                                            .opt_storage = NULL },

        // num-remaining-tiles-in-slice
        { .key = ARGS_NO_KEY, .key_long = "num-remaining-tiles-in-slice", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Array of Slice Boundaries",
                                            .opt_storage = NULL },

        // lp-filter-across-tiles-en-flag
        { .key = ARGS_NO_KEY, .key_long = "lp-filter-across-tiles-en-flag", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Loop filter across tiles enabled or disabled",
                                            .opt_storage = NULL },

        // rc-type
        { .key = ARGS_NO_KEY, .key_long = "rc-type", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = {1,"OFF"} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .integers = { {0,"OFF"}, {1,"ABR"}, {2,"CRF"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Rate control type, (0: OFF, 1: ABR, 2: CRF)",
                                            .opt_storage = NULL },

        // bitrate
        { .key = ARGS_NO_KEY, .key_long = "bitrate", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings={STR_LIST_END} } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Bitrate in terms of kilo-bits per second: Kbps(none,K,k), Mbps(M,m) ex) 100 = 100K = 0.1M",
                                            .opt_storage = NULL },

        // crf
        { .key = ARGS_NO_KEY, .key_long = "crf", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={10, 49} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Constant Rate Factor CRF-value [10-49]",
                                            .opt_storage = NULL },

        // vbv-bufsize
        { .key = ARGS_NO_KEY, .key_long = "vbv-bufsize", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings={ {NULL,NULL} } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "VBV buffer size: Kbits(none,K,k), Mbits(M,m) ex) 100 / 100K / 0.1M",
                                            .opt_storage = NULL },

        // use-filler
        { .key = ARGS_NO_KEY, .key_long = "use-filler", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "user filler flag",
                                            .opt_storage = NULL },

        // lookahead
        { .key = ARGS_NO_KEY, .key_long = "lookahead", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "number of pre analysis frames for rate control and cutree, disable:0",
                                            .opt_storage = NULL },

        // chroma-qp-table-present-flag
        { .key = ARGS_NO_KEY, .key_long = "chroma-qp-table-present-flag", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "chroma-qp-table-present-flag",
                                            .opt_storage = NULL },

        // chroma-qp-num-points-in-table
        { .key = ARGS_NO_KEY, .key_long = "chroma-qp-num-points-in-table", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings={ STR_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Number of pivot points for Cb and Cr channels",
                                            .opt_storage = NULL },

        // chroma-qp-delta-in-val-cb
        { .key = ARGS_NO_KEY, .key_long = "chroma-qp-delta-in-val-cb", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings =  {{NULL,NULL}} } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Array of input pivot points for Cb",
                                            .opt_storage = NULL },

        // chroma-qp-delta-out-val-cb
        { .key = ARGS_NO_KEY, .key_long = "chroma-qp-delta-out-val-cb", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings =  {{NULL,NULL}} } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Array of input pivot points for Cb",
                                            .opt_storage = NULL },

        // chroma-qp-delta-in-val-cr
        { .key = ARGS_NO_KEY, .key_long = "chroma-qp-delta-in-val-cr", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings =  {{NULL,NULL}} } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Array of input pivot points for Cr",
                                            .opt_storage = NULL },

        // chroma-qp-delta-out-val-cr
        { .key = ARGS_NO_KEY, .key_long = "chroma-qp-delta-out-val-cr", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings =  {{NULL,NULL}} } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Array of input pivot points for Cr",
                                            .opt_storage = NULL },

        // dra-enable-flag
        { .key = ARGS_NO_KEY, .key_long = "dra-enable-flag", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "DRA enable flag",
                                            .opt_storage = NULL },

        // dra-number-ranges
        { .key = ARGS_NO_KEY, .key_long = "dra-number-ranges", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Number of DRA ranges",
                                            .opt_storage = NULL },

        // dra-range
        { .key = ARGS_NO_KEY, .key_long = "dra-range", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings =  {{NULL,NULL}} } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Array of dra ranges",
                                            .opt_storage = NULL },

        // dra-scale
        { .key = ARGS_NO_KEY, .key_long = "dra-scale", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings =  {{NULL,NULL}} } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Array of input dra ranges",
                                            .opt_storage = NULL },

        // dra-chroma-qp-scale
        { .key = ARGS_NO_KEY, .key_long = "dra-chroma-qp-scale", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings =  {{NULL,NULL}} } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "DRA chroma qp scale value",
                                            .opt_storage = NULL },

        // dra-chroma-qp-offset
            { .key = ARGS_NO_KEY, .key_long = "dra-chroma-qp-offset", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings =  {{NULL,NULL}} } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "DRA chroma qp offset",
                                            .opt_storage = NULL },

        // dra-chroma-cb-scale
        { .key = ARGS_NO_KEY, .key_long = "dra-chroma-cb-scale", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings =  {{NULL,NULL}} } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "DRA chroma cb scale",
                                            .opt_storage = NULL },

        // dra-chroma-cr-scale
        { .key = ARGS_NO_KEY, .key_long = "dra-chroma-cr-scale", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings =  {{NULL,NULL}} } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "DRA chroma cr scale",
                                            .opt_storage = NULL },

        // dra-hist-norm
        { .key = ARGS_NO_KEY, .key_long = "dra-hist-norm", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings =  {{NULL,NULL}} } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "DRA hist norm",
                                            .opt_storage = NULL },

        // rpl-extern
        { .key = ARGS_NO_KEY, .key_long = "rpl-extern", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Whether to input external RPL",
                                            .opt_storage = NULL },

        // inter-slice-type
        { .key = ARGS_NO_KEY, .key_long = "inter-slice-type", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "INTER-SLICE-TYPE",
                                            .opt_storage = NULL },

        // picture-cropping-flag
        { .key = ARGS_NO_KEY, .key_long = "picture-cropping-flag", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "picture crop flag",
                                            .opt_storage = NULL },

        // picture-crop-left
        { .key = ARGS_NO_KEY, .key_long = "picture-crop-left", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "left offset of picture crop",
                                            .opt_storage = NULL },

        // picture-crop-right
        { .key = ARGS_NO_KEY, .key_long = "picture-crop-right", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "right offset of picture crop",
                                            .opt_storage = NULL },

        // picture-crop-top
        { .key = ARGS_NO_KEY, .key_long = "picture-crop-top", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "top offset of picture crop",
                                            .opt_storage = NULL },

        // picture-crop-bottom
        { .key = ARGS_NO_KEY, .key_long = "picture-crop-bottom", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "bottom offset of picture crop",
                                            .opt_storage = NULL },

        // ref
        { .key = ARGS_NO_KEY, .key_long = "ref", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Number of reference pictures",
                                            .opt_storage = NULL },

        // sar
        { .key = ARGS_NO_KEY, .key_long = "sar", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings =  {{NULL,NULL}} } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "sar <width:height|int> possible values 1 to 16 and 255",
                                            .opt_storage = NULL },

        // sar-width
        { .key = ARGS_NO_KEY, .key_long = "sar-width", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "sar <width:height|int>",
                                            .opt_storage = NULL },

        // sar-height
        { .key = ARGS_NO_KEY, .key_long = "sar-heigh", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "sar <width:height|int>",
                                            .opt_storage = NULL },

        // videoformat
        { .key = ARGS_NO_KEY, .key_long = "videoformat", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 6, .strings={ {"component",NULL}, {"pal",NULL}, {"ntsc",NULL}, {"secam",NULL}, {"mac",NULL}, {"unspecified",NULL}, {NULL,NULL} } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "",
                                            .opt_storage = NULL },

        // range
        { .key = ARGS_NO_KEY, .key_long = "range", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings =  {{NULL,NULL}} } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "black level and range of luma and chroma signals as 1- full or 0- limited",
                                            .opt_storage = NULL },

        // colorprim
        { .key = ARGS_NO_KEY, .key_long = "colorprim", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"", NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = {  .size = 13, .strings={{"reserved",NULL}, {"bt709",NULL}, {"unknown",NULL}, {"reserved",NULL}, {"bt470m",NULL}, {"bt470bg",NULL}, {"smpte170m",NULL}, {"smpte240m",NULL}, {"film",NULL}, {"bt2020",NULL}, {"smpte428",NULL}, {"smpte431",NULL}, {"smpte432",NULL}, {NULL,NULL} } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "",
                                            .opt_storage = NULL },

        // transfer
        { .key = ARGS_NO_KEY, .key_long = "transfer", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"", NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 19, .strings={{"reserved",NULL}, {"bt709",NULL}, {"unknown",NULL}, {"reserved",NULL}, {"bt470m",NULL}, {"bt470bg",NULL}, {"smpte170m",NULL}, {"smpte240m",NULL}, {"linear",NULL}, {"log100",NULL}, {"log316",NULL}, {"iec61966-2-4",NULL}, {"bt1361e",NULL}, {"iec61966-2-1",NULL}, {"bt2020-10",NULL}, {"bt2020-12",NULL}, {"smpte2084",NULL}, {"smpte428",NULL}, {"arib-std-b67",NULL}, STR_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "transfer characteristics",
                                            .opt_storage = NULL },

        // matrix-coefficients
        { .key = ARGS_NO_KEY, .key_long = "matrix-coefficients", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"", NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = {  .size = 14, .strings={{"gbr",NULL}, {"bt709",NULL}, {"unknown",NULL}, {"fcc",NULL}, {"bt470bg",NULL}, {"smpte170m",NULL}, {"smpte240m",NULL}, {"ycgco",NULL}, {"bt2020nc",NULL}, {"bt2020c",NULL}, {"smpte2085",NULL}, {"chroma-derived-nc",NULL}, {"chroma-derived-c",NULL}, {"ictcp",NULL}, {NULL,NULL} } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "",
                                            .opt_storage = NULL },

        // master-display
        { .key = ARGS_NO_KEY, .key_long = "master-display", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = {  .size = 0, .strings =  {{NULL,NULL}} } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "SMPTE ST 2086 master display color volume info SEI (HDR) format: G(x,y)B(x,y)R(x,y)WP(x,y)L(max,min)",
                                            .opt_storage = NULL },

        // max-content-light-level
        { .key = ARGS_NO_KEY, .key_long = "max-content-light-level", .value_type = VAL_TYPE_STRING,
                                            .has_default = 0, .default_value = { .string = {"",NULL} },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 0, .strings =  {{NULL,NULL}} } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Specify content light level info SEI as (cll,fall) (HDR)",
                                            .opt_storage = NULL },

        // chromaloc-tf
        { .key = ARGS_NO_KEY, .key_long = "chromaloc-tf", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, 5} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Chroma location for Top field - Range from 0 to 5",
                                            .opt_storage = NULL },

        // chromaloc-bf
        { .key = ARGS_NO_KEY, .key_long = "chromaloc-bf", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, 5} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Chroma location for Bottom field - Range from 0 to 5",
                                            .opt_storage = NULL },

        // neutral-chroma-flag
        { .key = ARGS_NO_KEY, .key_long = "neutral-chroma-flag", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Value can be 0 or 1",
                                            .opt_storage = NULL },

        // frame-field-flag
        { .key = ARGS_NO_KEY, .key_long = "frame-field-flag", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, "off" } },
                                            .values_rage_type = AVT_ALLOWED_VALUES_LIST,
                                            .allowed_values =  { .list = { .size = 2, .integers={{0,"off"}, {1,"on"}, INT_LIST_END } } },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "1 indicates fields and 0 indicates frames",
                                            .opt_storage = NULL },

        // units-in-tick
        { .key = ARGS_NO_KEY, .key_long = "units-in-tick", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = {1} },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={1, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Number of units in tick, value should be > 0",
                                            .opt_storage = NULL },

        // time-scale
        { .key = ARGS_NO_KEY, .key_long = "time-scale", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={1, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Time Scale, value should be > 0",
                                            .opt_storage = NULL },

        // fixed-pic-rate-flag
        { .key = ARGS_NO_KEY, .key_long = "fixed-pic-rate-flag", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Fixed picture rate flag, default 0",
                                            .opt_storage = NULL },

        // pic-struct
        { .key = ARGS_NO_KEY, .key_long = "pic-struct", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "Fixed picture rate flag, default 0",
                                            .opt_storage = NULL },

        // mv-over-pic-boundaries
        { .key = ARGS_NO_KEY, .key_long = "mv-over-pic-boundaries", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = {1} },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "mvs over picture boundaries flag",
                                            .opt_storage = NULL },

        // max-bytes-per-pic-denom
        { .key = ARGS_NO_KEY, .key_long = "max-bytes-per-pic-denom", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, 16} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "max bytes per picture denom, valid range 0 to 16",
                                            .opt_storage = NULL },

        // max-bits-per-cu-denom
        { .key = ARGS_NO_KEY, .key_long = "max-bits-per-cu-denom", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, 16} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "max bits per cu denom, valid range 0 to 16",
                                            .opt_storage = NULL },

        // log2-max-mv-len-hor
        { .key = ARGS_NO_KEY, .key_long = "log2-max-mv-len-hor", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, 16} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "max mv length horizontal log2, valid range 0 to 16",
                                            .opt_storage = NULL },

        // log2-max-mv-len-ver
        { .key = ARGS_NO_KEY, .key_long = "log2-max-mv-len-ver", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, 16} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "max mv length vertical log2, valid range o to 16",
                                            .opt_storage = NULL },

        // num-reorder-pics
        { .key = ARGS_NO_KEY, .key_long = "num-reorder-pics", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "# of reorder pics, valid range 0 to max_dec_pic_buffering default = max_dec_pic_buffering",
                                            .opt_storage = NULL },

        // max-dec-pic-buffering
        { .key = ARGS_NO_KEY, .key_long = "max-dec-pic-buffering", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "max picture buffering in decoder, valid range 0 to num-reorder-pic default num-reorder-pic",
                                            .opt_storage = NULL },

        /* termination */
        { .key = ARGS_END_KEY, .key_long = "", .value_type = VAL_TYPE_INT,
                                            .has_default = 0, .default_value = { .integer = { 0, NULL } },
                                            .values_rage_type = AVT_MIN_MAX_RANGE,
                                            .allowed_values =  { .range={0, INT_MAX} },
                                            .is_mandatory = 0, .is_set = 0,
                                            .desc = "",
                                            .opt_storage = NULL }
};


struct _ARGS_PARSER
{
    ARGS_OPT * opts;
    int  num_option;
    int initialized;

    /* variables for options */
    char fname_cfg[ARGS_OPT_VALUE_MAXLEN];
    char fname_inp[ARGS_OPT_VALUE_MAXLEN];
    char fname_out[ARGS_OPT_VALUE_MAXLEN];
    char fname_rec[ARGS_OPT_VALUE_MAXLEN];
    int frames;
    int info;
    int hash;
    int input_depth;
    int input_csp;
    int seek;
    char profile[ARGS_OPT_VALUE_MAXLEN];
    char preset[ARGS_OPT_VALUE_MAXLEN];
    char tune[ARGS_OPT_VALUE_MAXLEN];
    char bitrate[ARGS_OPT_VALUE_MAXLEN];
    char vbv_bufsize[ARGS_OPT_VALUE_MAXLEN];

    /* VUI options*/
    char sar[ARGS_OPT_VALUE_MAXLEN];
    int sar_width, sar_height;
    char videoformat[ARGS_OPT_VALUE_MAXLEN];
    char range[ARGS_OPT_VALUE_MAXLEN];
    char colorprim[ARGS_OPT_VALUE_MAXLEN];
    char transfer[ARGS_OPT_VALUE_MAXLEN];
    char master_display[ARGS_OPT_VALUE_MAXLEN];
    char max_cll[ARGS_OPT_VALUE_MAXLEN];
    char matrix_coefficients[ARGS_OPT_VALUE_MAXLEN];
    int overscan_info_present_flag;
    int overscan_appropriate_flag;
    int chroma_loc_info_present_flag;
    int chroma_sample_loc_type_top_field;
    int chroma_sample_loc_type_bottom_field;
    int neutral_chroma_indication_flag;
    int field_seq_flag;
    int timing_info_present_flag;
    int num_units_in_tick;
    int time_scale;
    int fixed_pic_rate_flag;
    int nal_hrd_parameters_present_flag;
    int vcl_hrd_parameters_present_flag;
    int low_delay_hrd_flag;
    int pic_struct_present_flag;
    int bitstream_restriction_flag;
    int motion_vectors_over_pic_boundaries_flag;
    int max_bytes_per_pic_denom;
    int max_bits_per_mb_denom;
    int log2_max_mv_length_horizontal;
    int log2_max_mv_length_vertical;
    int num_reorder_pics;
    int max_dec_pic_buffering;
};

///////////////////////////////////////////////////////////////////////////////
// Internal function declarations
///////////////////////////////////////////////////////////////////////////////
//
static int args_search_long_key(ARGS_OPT * opts, const char * key);
static int args_search_short_arg(ARGS_OPT * ops, const char key);
static int args_read_value(ARGS_OPT * ops, const char * argv);
static int args_parse_cfg(FILE* fp, ARGS_OPT* ops, int is_type_ppt);
static int args_parse_cmd(int argc, const char * argv[], ARGS_OPT * ops, int * idx, char ** errstr);

static int args_set_storage_for_option_by_long_key(ARGS_OPT * opts, char * key_long, void * var);
static int args_set_storage_for_option_by_short_key(ARGS_OPT * opts, char * key, void * var);

#define ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, key_long) \
    args_set_storage_for_option_by_long_key(opts, #key_long, (void*)&((param)->key_long))

#define ARGS_SET_PARAM_VAR_KEY(opts, param, key) \
    args_set_storage_for_option_by_short_key(opts, #key, (void*)&((param)->key))


///////////////////////////////////////////////////////////////////////////////
// Public API function implementations
///////////////////////////////////////////////////////////////////////////////
//
ARGS_PARSER * xeve_args_create(void)
{
    ARGS_PARSER * args = NULL;
    ARGS_OPT * opts = NULL;

    args = malloc(sizeof(ARGS_PARSER));
    if(args == NULL) goto ERR;
    memset(args, 0, sizeof(ARGS_PARSER));

    opts = malloc(sizeof(args_opt_table));
    if (opts == NULL) goto ERR;
    memcpy(opts, args_opt_table, sizeof(args_opt_table));
    args->opts = opts;
    args->num_option = ((int)(sizeof(args_opt_table)/sizeof(args_opt_table[0]))-1);
    args->initialized = 0;

    return args;

ERR:
    if(opts) {
        free(opts);
        opts = NULL;
    }

    if(args) {
        free(args);
        args = NULL;
    }

    return NULL;
}

void xeve_args_release(ARGS_PARSER * args)
{
    if (args != NULL)
    {
        if(args->opts != NULL) {
            free(args->opts);
            args->opts = NULL;
        }
        free(args);
        args = NULL;
    }
}

int xeve_args_init(ARGS_PARSER * args, XEVE_PARAM* param)
{
    ARGS_OPT * opts;
    char* val;

    // do some checks
    if(!args)
        return XEVE_ERR_INVALID_ARGUMENT;

    opts = args->opts;

    /*args_set_storage_for_option_by_long_key(opts, "config", args->fname_cfg);*/
    args_set_storage_for_option_by_long_key(opts, "input", args->fname_inp);
    args_set_storage_for_option_by_long_key(opts, "output", args->fname_out);
    args_set_storage_for_option_by_long_key(opts, "recon", args->fname_rec);

    args_set_storage_for_option_by_long_key(opts, "frames", &args->frames);

    args_set_storage_for_option_by_long_key(opts, "info", &args->info);
    xeve_args_set_option_val(args, "info", "1");
    assert(args->info == 1);

    args_set_storage_for_option_by_long_key(opts, "hash", &args->hash);

//    args_set_storage_for_option_by_long_key(opts, "verbose", &op_verbose);
//    op_verbose = VERBOSE_SIMPLE; /* default */

    args_set_storage_for_option_by_long_key(opts, "input-depth", &args->input_depth);
    xeve_args_set_option_val(args, "input-depth", "8");
    assert(args->input_depth ==8);

    args_set_storage_for_option_by_long_key(opts, "input-csp", &args->input_csp);
    xeve_args_set_option_val(args, "input-csp", "1");
    assert(args->input_csp == 1);

    args_set_storage_for_option_by_long_key(opts, "seek", &args->seek);
    args_set_storage_for_option_by_long_key(opts, "profile", args->profile);
    xeve_args_set_option_val(args, "profile", "baseline"); /* default */
    assert(strcmp(args->profile, "baseline") == 0);

    args_set_storage_for_option_by_long_key(opts, "preset", args->preset);
    xeve_args_set_option_val(args, "preset", "medium");  /* default */
    assert(strcmp(args->preset, "medium") == 0);

    args_set_storage_for_option_by_long_key(opts, "tune", args->tune);
    xeve_args_set_option_val(args, "tune", "none");  /* default */
    assert(strcmp(args->tune, "none") == 0);

    args_set_storage_for_option_by_long_key(opts, "bitrate", args->bitrate);
    xeve_args_set_option_val(args, "bitrate", "");  /* default */
    assert(strcmp(args->bitrate, "") == 0);

    args_set_storage_for_option_by_long_key(opts, "vbv-bufsize", args->vbv_bufsize);
    xeve_args_set_option_val(args, "vbv-bufsize", "");  /* default */
    assert(strcmp(args->vbv_bufsize, "") == 0);

    args_set_storage_for_option_by_long_key(opts, "sar", args->sar);
    xeve_args_set_option_val(args, "sar", "");  /* default */
    assert(strcmp(args->sar, "") == 0);

    args_set_storage_for_option_by_long_key(opts, "sar-width", &args->sar_width);
    xeve_args_set_option_val(args, "sar-width", "0");  /* default */
    assert(args->sar_width == 0);

    args_set_storage_for_option_by_long_key(opts, "sar-height", &args->sar_height);
    xeve_args_set_option_val(args, "sar-height", "0");  /* default */
    assert(args->sar_height == 0);

    args_set_storage_for_option_by_long_key(opts, "videoformat", args->videoformat);
    xeve_args_set_option_val(args, "videoformat", "");  /* default */
    assert(strcmp(args->videoformat, "") == 0);

    args_set_storage_for_option_by_long_key(opts, "range", args->range);
    xeve_args_set_option_val(args, "range", "");  /* default */
    assert(strcmp(args->range, "") == 0);

    args_set_storage_for_option_by_long_key(opts, "colorprim", args->colorprim);
    xeve_args_set_option_val(args, "colorprim", "");  /* default */
    assert(strcmp(args->colorprim, "") == 0);

    args_set_storage_for_option_by_long_key(opts, "transfer", args->transfer);
    xeve_args_set_option_val(args, "transfer", "");  /* default */
    assert(strcmp(args->transfer, "") == 0);

    args_set_storage_for_option_by_long_key(opts, "master-display", args->master_display);
    xeve_args_set_option_val(args, "master-display", ""); /* default */
    assert(strcmp(args->master_display, "") == 0);

    args_set_storage_for_option_by_long_key(opts, "max-content-light-level", args->max_cll);
    xeve_args_set_option_val(args, "max-content-light-level", "");  /* default */
    assert(strcmp(args->max_cll, "") == 0);

    args_set_storage_for_option_by_long_key(opts, "matrix-coefficients", args->matrix_coefficients);
    xeve_args_set_option_val(args, "matrix-coefficients", "");  /* default */
    assert(strcmp(args->matrix_coefficients, "") == 0);

    args_set_storage_for_option_by_long_key(opts, "chromaloc-tf", &args->chroma_sample_loc_type_top_field);
    xeve_args_set_option_val(args, "chromaloc-tf", "0");  /* default */
    assert(args->chroma_sample_loc_type_top_field == 0);

    args_set_storage_for_option_by_long_key(opts, "chromaloc-bf", &args->chroma_sample_loc_type_bottom_field);
    xeve_args_set_option_val(args, "chromaloc-bf", "0");  /* default */
    assert(args->chroma_sample_loc_type_bottom_field == 0);

    args_set_storage_for_option_by_long_key(opts, "neutral-chroma-flag", &args->neutral_chroma_indication_flag);
    xeve_args_set_option_val(args, "neutral-chroma-flag", "0");  /* default */
    assert(args->neutral_chroma_indication_flag == 0);

    args_set_storage_for_option_by_long_key(opts, "frame-field-flag", &args->field_seq_flag);
    xeve_args_set_option_val(args, "rame-field-flag", "0");  /* default */
    assert(args->field_seq_flag == 0);

    args_set_storage_for_option_by_long_key(opts, "units-in-tick", &args->num_units_in_tick);
    xeve_args_set_option_val(args, "units-in-tick", "0");  /* default */
    assert(args->num_units_in_tick == 0);

    args_set_storage_for_option_by_long_key(opts, "time-scale", &args->time_scale);
    xeve_args_set_option_val(args, "time-scale", "0");  /* default */
    assert(args->time_scale == 0);

    args_set_storage_for_option_by_long_key(opts, "fixed-pic-rate-flag", &args->fixed_pic_rate_flag);
    xeve_args_set_option_val(args, "fixed-pic-rate-flag", "0");  /* default */
    assert(args->fixed_pic_rate_flag == 0);

    args_set_storage_for_option_by_long_key(opts, "pic-struct", &args->pic_struct_present_flag);
    xeve_args_set_option_val(args, "pic-struct", "0");  /* default */
    assert(args->pic_struct_present_flag == 0);

    args_set_storage_for_option_by_long_key(opts, "mv-over-pic-boundaries", &args->motion_vectors_over_pic_boundaries_flag);
    xeve_args_set_option_val(args, "mv-over-pic-boundaries", "1");  /* default */
    assert(args->motion_vectors_over_pic_boundaries_flag == 1);

    args_set_storage_for_option_by_long_key(opts, "max-bytes-per-pic-denom", &args->max_bytes_per_pic_denom);
    xeve_args_set_option_val(args, "max-bytes-per-pic-denom", "2");  /* default */
    assert(args->max_bytes_per_pic_denom  == 2);

    args_set_storage_for_option_by_long_key(opts, "max-bits-per-cu-denom", &args->max_bits_per_mb_denom);
    xeve_args_set_option_val(args, "max-bits-per-cu-denom", "1");  /* default */
    assert(args->max_bits_per_mb_denom == 1);

    args_set_storage_for_option_by_long_key(opts, "log2-max-mv-len-hor", &args->log2_max_mv_length_horizontal);
    xeve_args_set_option_val(args, "log2-max-mv-len-hor", "16");  /* default */
    assert(args->log2_max_mv_length_horizontal == 16);

    args_set_storage_for_option_by_long_key(opts, "log2-max-mv-len-ver", &args->log2_max_mv_length_vertical);
    xeve_args_set_option_val(args, "log2-max-mv-len-ver", "16");  /* default */
    assert(args->log2_max_mv_length_vertical == 16);

    args_set_storage_for_option_by_long_key(opts, "max-dec-pic-buffering", &args->max_dec_pic_buffering);
    xeve_args_set_option_val(args, "max-dec-pic-buffering", "21");  /* default */
    assert(args->max_dec_pic_buffering == 21);

    args_set_storage_for_option_by_long_key(opts, "num-reorder-pics", &args->num_reorder_pics);
    xeve_args_set_option_val(args, "num-reorder-pics", "21");  /* default */
    assert(args->num_reorder_pics == args->max_dec_pic_buffering);

    ARGS_SET_PARAM_VAR_KEY(opts, param, w);
    ARGS_SET_PARAM_VAR_KEY(opts, param, h);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, qp);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, crf);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, aq_mode);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, fps);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, keyint);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, bframes);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, threads);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, codec_bit_depth);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, closed_gop);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, disable_hgop);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, level_idc);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, rc_type);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, use_filler);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, lookahead);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ref);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, sar_width);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, sar_height);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, chroma_sample_loc_type_top_field);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, chroma_sample_loc_type_bottom_field);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, neutral_chroma_indication_flag);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, field_seq_flag);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, num_units_in_tick);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, time_scale);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, fixed_pic_rate_flag);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, pic_struct_present_flag);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, motion_vectors_over_pic_boundaries_flag);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, max_bytes_per_pic_denom);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, max_bits_per_mb_denom);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, log2_max_mv_length_horizontal);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, log2_max_mv_length_vertical);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, num_reorder_pics);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, max_dec_pic_buffering);

#if 0
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_BTT, &param->btt);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_SUCO, &param->suco);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_ADD_QP_FRAME, &param->qp_incread_frame);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_FRAMEWORK_CB_MAX, &param->framework_cb_max);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_FRAMEWORK_CB_MIN, &param->framework_cb_min);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_FRAMEWORK_CU14_MAX, &param->framework_cu14_max);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_FRAMEWORK_TRIS_MAX, &param->framework_tris_max);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_FRAMEWORK_TRIS_MIN, &param->framework_tris_min);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_FRAMEWORK_SUCO_MAX, &param->framework_suco_max);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_FRAMEWORK_SUCO_MIN, &param->framework_suco_min);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TOOL_AMVR, &param->tool_amvr);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TOOL_MMVD, &param->tool_mmvd);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TOOL_AFFINE, &param->tool_affine);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TOOL_DMVR, &param->tool_dmvr);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TOOL_ADDB, &param->tool_addb);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TOOL_ALF, &param->tool_alf);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TOOL_HTDF, &param->tool_htdf);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TOOL_ADMVP, &param->tool_admvp);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TOOL_HMVP, &param->tool_hmvp);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TOOL_EIPD, &param->tool_eipd);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TOOL_IQT, &param->tool_iqt);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TOOL_CM_INIT, &param->tool_cm_init);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TOOL_ADCC, &param->tool_adcc);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TOOL_RPL, &param->tool_rpl);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TOOL_POCS, &param->tool_pocs);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_QP_CB_OFFSET, &param->qp_cb_offset);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_QP_CR_OFFSET, &param->qp_cr_offset);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TOOL_ATS, &param->tool_ats);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_CONSTRAINED_INTRA_PRED, &param->constrained_intra_pred);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TOOL_DBF, &param->tool_addb);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TOOL_DBFOFFSET_A, &param->deblock_alpha_offset);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TOOL_DBFOFFSET_B, &param->deblock_beta_offset);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TILE_UNIFORM_SPACING, &param->tile_uniform_spacing_flag);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_NUM_TILE_COLUMNS, &param->tile_columns);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_NUM_TILE_ROWS, &param->tile_rows);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TILE_COLUMN_WIDTH_ARRAY, &param->tile_column_width_array);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_TILE_ROW_HEIGHT_ARRAY, &param->tile_row_height_array);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_NUM_SLICE_IN_PIC, &param->num_slice_in_pic);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_SLICE_BOUNDARY_ARRAY, &param->tile_array_in_slice);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_ARBITRAY_SLICE_FLAG, &param->arbitrary_slice_flag);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_NUM_REMAINING_TILES_IN_SLICE, &param->num_remaining_tiles_in_slice_minus1);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_LOARG_FILTER_ACROSS_TILES_ENABLED_FLAG, &param->loop_filter_across_tiles_enabled_flag);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_CHROMA_QP_TABLE_PRESENT_FLAG, &param->chroma_qp_table_present_flag);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_CHROMA_QP_NUM_POINTS_IN_TABLE, param->chroma_qp_num_points_in_table);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_CHROMA_QP_DELTA_IN_VAL_CB, param->chroma_qp_delta_in_val_cb);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_CHROMA_QP_DELTA_OUT_VAL_CB, param->chroma_qp_delta_out_val_cb);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_CHROMA_QP_DELTA_IN_VAL_CR, param->chroma_qp_delta_in_val_cr);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_CHROMA_QP_DELTA_OUT_VAL_CR, param->chroma_qp_delta_out_val_cr);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_DRA_ENABLE_FLAG, &param->tool_dra);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_DRA_NUMBER_RANGES, &param->dra_number_ranges);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_DRA_RANGE, param->dra_range);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_DRA_SCALE, param->dra_scale);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_DRA_CHROMA_QP_SCALE, param->dra_chroma_qp_scale);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_DRA_CHROMA_QP_OFFSET, param->dra_chroma_qp_offset);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_DRA_CHROMA_CB_SCALE, param->dra_chroma_cb_scale);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_DRA_CHROMA_CR_SCALE, param->dra_chroma_cr_scale);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_DRA_HIST_NORM, param->dra_hist_norm);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL_EXTERN, &param->rpl_extern);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_0, param->rpl0[0]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_1, param->rpl0[1]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_2, param->rpl0[2]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_3, param->rpl0[3]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_4, param->rpl0[4]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_5, param->rpl0[5]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_6, param->rpl0[6]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_7, param->rpl0[7]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_8, param->rpl0[8]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_9, param->rpl0[9]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_10, param->rpl0[10]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_11, param->rpl0[11]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_12, param->rpl0[12]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_13, param->rpl0[13]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_14, param->rpl0[14]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_15, param->rpl0[15]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_16, param->rpl0[16]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_17, param->rpl0[17]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_18, param->rpl0[18]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_19, param->rpl0[19]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_20, param->rpl0[20]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_21, param->rpl0[21]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_22, param->rpl0[22]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_23, param->rpl0[23]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_24, param->rpl0[24]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL0_25, param->rpl0[25]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_0, param->rpl1[0]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_1, param->rpl1[1]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_2, param->rpl1[2]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_3, param->rpl1[3]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_4, param->rpl1[4]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_5, param->rpl1[5]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_6, param->rpl1[6]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_7, param->rpl1[7]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_8, param->rpl1[8]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_9, param->rpl1[9]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_10, param->rpl1[10]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_11, param->rpl1[11]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_12, param->rpl1[12]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_13, param->rpl1[13]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_14, param->rpl1[14]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_15, param->rpl1[15]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_16, param->rpl1[16]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_17, param->rpl1[17]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_18, param->rpl1[18]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_19, param->rpl1[19]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_20, param->rpl1[20]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_21, param->rpl1[21]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_22, param->rpl1[22]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_23, param->rpl1[23]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_24, param->rpl1[24]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_RPL1_25, param->rpl1[25]);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_INTER_SLICE_TYPE, &param->inter_slice_type);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_PIC_CRARG_FLAG, &param->picture_cropping_flag);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_PIC_CRARG_LEFT, &param->picture_crop_left_offset);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_PIC_CRARG_RIGHT, &param->picture_crop_right_offset);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_PIC_CRARG_TOP, &param->picture_crop_top_offset);
    ARGS_SET_PARAM_VAR_KEY_LONG(opts, param, ARG_PIC_CRARG_BOTTOM, &param->picture_crop_bottom_offset);
#endif
    args->initialized = 1;

    return XEVE_OK;
}

int xeve_args_parse(ARGS_PARSER * args, int argc, const char* argv[], char ** errstr)
{
    int i, ret = 0, idx = 0;
    const char* fname_cfg = NULL;
    FILE* fp;

    int num_configs = 0;
    int pos_conf_files[ARGS_MAX_NUM_CONF_FILES];

     // do some checks
    if(!args || argc<=0 || !argv)
        return XEVE_ERR_INVALID_ARGUMENT;

    if( !args->initialized )
        return XEVE_ERR;

    memset(&pos_conf_files, -1, sizeof(int) * ARGS_MAX_NUM_CONF_FILES);

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

            if (args_parse_cfg(fp, args->opts, 1))
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
        ret = args_parse_cmd(argc, argv, args->opts, &idx, errstr);
        if (ret <= 0) break;
    }
    return ret;
}

int xeve_args_check_mandatory(ARGS_PARSER * args, char ** err_arg)
{
    ARGS_OPT * o = args->opts;

    // do some checks
    if(!args)
        return XEVE_ERR_INVALID_ARGUMENT;

    while (o->key != 0)
    {
        if (o->is_mandatory)
        {
            if (o->is_set == 0)
            {
                /* not filled all mandatory argument */
                *err_arg = o->key_long;
                return XEVE_ERR;
            }
        }
        o++;
    }
    return XEVE_OK;
}

int XEVE_EXPORT xeve_args_get_option_val(ARGS_PARSER * args, char* keyl,  char** val) {

    char buf[ARGS_OPT_VALUE_MAXLEN];
    size_t len;
    char *str;
    ARGS_OPT *opt = NULL;

    *val = NULL;

    // do some checks
    if( !args || !keyl )
        return XEVE_ERR_INVALID_ARGUMENT;

    int idx = args_search_long_key(args->opts, keyl);
    if( idx  < 0 )
        return XEVE_ERR_INVALID_ARGUMENT;

    //if( !args->initialized )
    //    return XEVE_ERR;

    if(!args->opts[idx].opt_storage)
        return XEVE_ERR;

    if( args->opts[idx].is_set != 1 )
        return XEVE_ERR;

    opt = args->opts + idx;

    switch(opt->value_type)
    {
        case VAL_TYPE_INT:
            snprintf(buf, ARGS_OPT_VALUE_MAXLEN, "%d", *((int*)(args->opts[idx].opt_storage)));

            len = strlen(buf);
            *val = (char*)malloc(len+1);
            strncpy(*val, buf, len+1);
            // *val[len-1] = '\0';

            break;
        case VAL_TYPE_STRING:

            str = (char*)(args->opts[idx].opt_storage);
            if(!str) return XEVE_ERR;

            len = strlen(str);
            *val = (char*)malloc(len+1);
            strncpy(*val, str, len+1);

            break;
        default:
            return XEVE_ERR;
    }

    return XEVE_OK;
}

int XEVE_EXPORT xeve_args_set_option_val(ARGS_PARSER * args, char* keyl,  char* val) {

    char *endptr;
    int ival;
    ARGS_OPT *opt = NULL;

     // do some checks
    if( !args || !keyl || !val )
        return XEVE_ERR_INVALID_ARGUMENT;

    //if( !args->initialized )
    //    return XEVE_ERR;

    int idx =  args_search_long_key(args->opts,keyl);
    if( idx<0 )
        return XEVE_ERR_INVALID_ARGUMENT;

    if(!args->opts[idx].opt_storage)
        return XEVE_ERR;

    opt = args->opts + idx;

    switch(opt->value_type)
    {
        case VAL_TYPE_INT:
            ival = strtol(val, &endptr, 10);
            if (*endptr != '\0')
                return XEVE_ERR_INVALID_ARGUMENT;

            *((int*)(args->opts[idx].opt_storage)) = ival;
            args->opts[idx].is_set = 1;

            break;
        case VAL_TYPE_STRING:

            strncpy((char*)(args->opts[idx].opt_storage), val, ARGS_OPT_VALUE_MAXLEN-1);
            args->opts[idx].is_set = 1;

            break;
        default:
            return XEVE_ERR;
    }

    return XEVE_OK;
}

int XEVE_EXPORT xeve_args_get_option_description(ARGS_PARSER * args, char* keyl, char ** help)
{
    int optional = 0;
    char vtype[32];
    ARGS_OPT * opt = NULL;
    int i = 0;
    int n = 0;

    char default_value[ARGS_OPT_DEFAULT_VALUE_MAXLEN] = { 0 };
    char allowed_values[ ARGS_OPT_ALLOWED_VALUES_MAXLEN] = { 0 };

    *help = NULL;

    // do some checks
    if( !args || !keyl )
        return XEVE_ERR_INVALID_ARGUMENT;

    if( !args->initialized )
        return XEVE_ERR;

    int idx =  args_search_long_key(args->opts,keyl);
    if( idx<0 )
        return XEVE_ERR_INVALID_ARGUMENT;

    opt = args->opts + idx;

    switch(opt->value_type)
    {
        case VAL_TYPE_INT:
            strcpy(vtype, "INTEGER");
            if(opt->opt_storage != NULL) snprintf(default_value, ARGS_OPT_DEFAULT_VALUE_MAXLEN, " [default: %d]", *(int*)(opt->opt_storage));
            break;
        case VAL_TYPE_STRING:
            strcpy(vtype, "STRING");
            if(opt->opt_storage != NULL) snprintf(default_value, ARGS_OPT_DEFAULT_VALUE_MAXLEN, " [default: %s]", strlen((char*)(opt->opt_storage)) == 0 ? "None" : (char*)(opt->opt_storage));
            break;
        default:
            break;
    }

    optional = !(opt->is_mandatory);

    // allocate memory for help string
    *help = (char*)malloc(ARGS_OPT_HELP_STRING_MAXLEN);


   // NamedConstant* constants = NULL;
    //Enum* enums = NULL;
    StrOption *strings;
    IntOption *integers;

    switch(opt->value_type)
    {
        case VAL_TYPE_INT:
            if(opt->values_rage_type == AVT_ALLOWED_VALUES_LIST) {
                integers = args_opt_table[idx].allowed_values.list.integers;
                int size = args_opt_table[idx].allowed_values.list.size;
                while(integers[i].value != INT_MIN) {
                    if(integers[i].descr) {
                        n += snprintf(allowed_values + n,  ARGS_OPT_ALLOWED_VALUES_MAXLEN, "\n          - %d: %s",args_opt_table[idx].allowed_values.list.integers[i].value, args_opt_table[idx].allowed_values.list.integers[i].descr);
                    } else {
                        // n += snprintf(allowed_values+n,  ARGS_OPT_ALLOWED_VALUES_MAXLEN, "%d, ", args_opt_table[idx].allowed_values.list.integers[i].value);
                        n += snprintf(allowed_values + n,  ARGS_OPT_ALLOWED_VALUES_MAXLEN, "\n            %d",args_opt_table[idx].allowed_values.list.integers[i].value);
                    }
                    i++;
                }
            } else if(opt->values_rage_type == AVT_MIN_MAX_RANGE) {
                int min  = args_opt_table[idx].allowed_values.range.min;
                int max =  args_opt_table[idx].allowed_values.range.max;
                n += snprintf(allowed_values,  ARGS_OPT_ALLOWED_VALUES_MAXLEN, "\n          - range [%d,  %d]", min, max);
            }

            break;
        // case OPT_TYPE_ENUM:
        //     enums = args_opt_table[idx].allowed_values.enums;

        //     n += snprintf(allowed_values+n,  ARGS_OPT_ALLOWED_VALUES_MAXLEN, "\n          allowed values: [");
        //     while(enums[i]!=ENUM_END) {
        //         n += snprintf(allowed_values+n,  ARGS_OPT_ALLOWED_VALUES_MAXLEN, "%d, ", args_opt_table[idx].allowed_values.enums[i]);
        //         i++;
        //     }
        //     n += snprintf(allowed_values+n-2,  ARGS_OPT_ALLOWED_VALUES_MAXLEN, "]");
        //     break;
        // case OPT_TYPE_CONST:
        //     constants = args_opt_table[idx].allowed_values.constants;
        //     while(constants[i].name) {
        //         n += snprintf(allowed_values + n,  ARGS_OPT_ALLOWED_VALUES_MAXLEN, "\n          - %d: %s",args_opt_table[idx].allowed_values.constants[i].value, args_opt_table[idx].allowed_values.constants[i].name);
        //         i++;
        //     }
        //     break;
        case VAL_TYPE_STRING:
            strings = args_opt_table[idx].allowed_values.list.strings;
            while(strings[i].value) {
                n += snprintf(allowed_values + n,  ARGS_OPT_ALLOWED_VALUES_MAXLEN, "\n          - %s",args_opt_table[idx].allowed_values.list.strings[i].value);
                i++;
            }
            break;
        default:
            break;
    }

    if(opt->key != ARGS_NO_KEY)
    {
        snprintf(*help, ARGS_OPT_HELP_STRING_MAXLEN, "  -%c, --%s [%s]%s%s\n    : %s %s", opt->key, opt->key_long,
                vtype, (optional) ? " (optional)" : "", (optional) ? default_value : "", opt->desc, allowed_values);
    }
    else
    {
        snprintf(*help, ARGS_OPT_HELP_STRING_MAXLEN, "  --%s [%s]%s%s\n    : %s %s", opt->key_long,
                vtype, (optional) ? " (optional)" : "", (optional) ? default_value : "", opt->desc, allowed_values);
    }



    return XEVE_OK;
}

int xeve_args_num_options(ARGS_PARSER * args)
{
    if(!args) {
        return 0;
    }
    return args->num_option;
}

/* Three-part iterator */
char* xeve_args_first_key(ARGS_PARSER * args)             /* returns smallest element in n */
{
    ARGS_OPT *opt = NULL;

    // do some checks
    if( !args )
        return NULL;

    if(!args->num_option) return NULL;

    opt = args->opts;
    return opt->key_long;
}

// int nums_done(ARGS_PARSER * args, char* key);        /* returns 1 if key is past end */
char* xeve_args_next_key(ARGS_PARSER * args, char* keyl)   /* returns next value after key */
{
    ARGS_OPT *opt = NULL;
    int idx = 0;
    int next_idx = 0;

    // do some checks
    if( !args)
        return NULL;

    if(keyl)
    {
        idx = args_search_long_key(args->opts, keyl);
        if( idx  < 0 ) return NULL;

        next_idx = idx + 1;

        // if next idx is out of bound
        if( next_idx > args->num_option-1) return NULL;


    } else {
        next_idx = 0;
    }

    opt = args->opts + next_idx;

    return opt->key_long;
}

//////////////////////////////////////////////////////////////////////////////
// Internal function implementations
///////////////////////////////////////////////////////////////////////////////

static int args_search_long_key(ARGS_OPT * opts, const char * key)
{
    int oidx = 0;
    ARGS_OPT * opt = NULL;

    opt = opts;
    while(opt->key != ARGS_END_KEY)
    {
        if(!strcmp(key, opt->key_long))
        {
            return oidx;
        }
        oidx++;
        opt++;
    }
    return -1;
}

static int args_search_short_arg(ARGS_OPT * ops, const char key)
{
    int oidx = 0;
    ARGS_OPT * o;

    o = ops;

    while(o->key != ARGS_END_KEY)
    {
        if(o->key != ARGS_NO_KEY && o->key == key)
        {
            return oidx;
        }
        oidx++;
        o++;
    }
    return -1;
}

int args_read_value(ARGS_OPT * ops, const char * argv)
{
    if(argv == NULL || ops->opt_storage == NULL)
    {
        return -1;
    }
    if(argv[0] == '-' && (argv[1] < '0' || argv[1] > '9')) return -1;

    switch(ops->value_type)
    {
        case VAL_TYPE_INT:
            *((int*)ops->opt_storage) = atoi(argv);
            break;

        case VAL_TYPE_STRING:
            strcpy((char*)ops->opt_storage, argv);
            break;

        default:
            return -1;
    }
    return 0;
}

static int args_parse_cfg(FILE* fp, ARGS_OPT* ops, int is_type_ppt)
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

        oidx = args_search_long_key(ops, tag);
        if (oidx < 0) continue;

        if (ops[oidx].opt_storage == NULL)
        {
            return -1;
        }

        //if (ARGS_GET_IS_OPT_TYPE_PPT(ops[oidx].value_type) == is_type_ppt)
        //{
            if (ops[oidx].value_type == VAL_TYPE_INT || ops[oidx].value_type == VAL_TYPE_STRING)
            {
                if (args_read_value(ops + oidx, val)) continue;
            }
            else
            {
                *((int*)ops[oidx].opt_storage) = 1;
            }
            ops[oidx].is_set = 1;
        //}
    }
    return 0;
}

static int args_parse_cmd(int argc, const char * argv[], ARGS_OPT * ops, int * idx, char ** errstr)
{
    int    aidx; /* arg index */
    int    oidx; /* option index */

    aidx = *idx + 1;

    if(aidx >= argc || argv[aidx] == NULL) goto NO_MORE;
    if(argv[aidx][0] != '-') goto ERR;

    if(argv[aidx][1] == '-')
    {
        /* long option */
        oidx = args_search_long_key(ops, argv[aidx] + 2);
        if(oidx < 0)
        {
            *errstr = (char*)argv[aidx];
            goto ERR;
        }
    }
    else if(strlen(argv[aidx]) == 2)
    {
        /* short option */
        oidx = args_search_short_arg(ops, argv[aidx][1]);
        if(oidx < 0)
        {
            *errstr = (char*)argv[aidx];
            goto ERR;
        }
    }
    else
    {
        goto ERR;
    }

    if(ops[oidx].value_type == VAL_TYPE_INT || ops[oidx].value_type == VAL_TYPE_STRING)
    {
        if(aidx + 1 >= argc) {
            *errstr = (char*)argv[aidx];
            goto ERR;
        }
        if(args_read_value(ops + oidx, argv[aidx + 1])) {
            *errstr = (char*)argv[aidx];
            goto ERR;
        }
        *idx = *idx + 1;
    }
    else
    {
        *((int*)ops[oidx].opt_storage) = 1;
    }
    ops[oidx].is_set = 1;
    *idx = *idx + 1;

    return ops[oidx].key;


NO_MORE:
    return 0;

ERR:
    return -1;
}

static int args_set_storage_for_option_by_long_key(ARGS_OPT * opts, char * key_long, void * var)
{
    int idx;
    char buf[ARGS_MAX_KEY_LONG];
    char * ko = key_long;
    char * kt = buf;

    /* if long key has "_", convert to "-". */
    while(*ko != '\0')
    {
        if(*ko == '_') *kt = '-';
        else *kt = *ko;

        ko++;
        kt++;
    }
    *kt = '\0';

    idx = args_search_long_key(opts, buf);
    if (idx < 0) return -1;
    opts[idx].opt_storage = var;
    return 0;
}

static int args_set_storage_for_option_by_short_key(ARGS_OPT * opts, char * key, void * var)
{
    int idx;
    idx = args_search_short_arg(opts, key[0]);
    if (idx < 0) return -1;
    opts[idx].opt_storage = var;
    return 0;
}

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

#define ARGS_GET_CMD_OPT_VAL_TYPE(x)  ((x) & 0x0C)
#define ARGS_GET_IS_OPT_TYPE_PPT(x)   (((x) >> 1) & 0x01)

#define ARGS_END_KEY                  (0)
#define ARGS_NO_KEY                   (127)
#define ARGS_KEY_LONG_CONFIG          "config"
#define ARGS_MAX_NUM_CONF_FILES       (16)

#define ARGS_MAX_KEY_LONG             (64)

#define ARGS_OPT_DEFAULT_VALUE_MAXLEN (256)

#define ARGS_OPT_HELP_STRING_MAXLEN   (1024)

#define ARGS_VAL_TYPE_MANDATORY       ( 1 << 0) /* mandatory or not */
#define ARGS_VAL_TYPE_NONE            ( 1 << 2) /* no value */
#define ARGS_VAL_TYPE_INTEGER         ( 2 << 2) /* integer type value */
#define ARGS_VAL_TYPE_STRING          ( 3 << 2) /* string type value */

// typedef union _VALUE {
//     int i;
//     char* str;
// } VALUE;

typedef struct _ARGS_OPT
{
    char   key; /* option keyword. ex) -f */
    char   key_long[ARGS_MAX_KEY_LONG]; /* option long keyword, ex) --file */
    int    val_type; /* value type */
    union {
        int i;
        const char *str;
    } default_val;
    int    flag; /* flag to setting or not */
    void * val; /* actual value */
    char   desc[512]; /* description of option */
} ARGS_OPT;

/* Define various command line options as a table */
const ARGS_OPT args_opt_table[] = {
    {
        'v',  "verbose", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "verbose (log) level\n"
        "      - 0: no message\n"
        "      - 1: only error message\n"
        "      - 2: simple messages\n"
        "      - 3: frame-level messages"
    },
    /*
    {
        ARGS_NO_KEY, ARGS_KEY_LONG_CONFIG, ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "file name of configuration"
    },
    */
    {
        'i', "input", ARGS_VAL_TYPE_STRING | ARGS_VAL_TYPE_MANDATORY, { .str = " " }, 0, NULL,
        "file name of input video (raw YUV or Y4M), `stdin` for standard input instead of regular file "
    },
    {
        'o', "output", ARGS_VAL_TYPE_STRING, { .str = " " }, 0, NULL,
        "file name of output bitstream"
    },
    {
        'r', "recon", ARGS_VAL_TYPE_STRING, { .str = " " }, 0, NULL,
        "file name of reconstructed video"
    },
    {
        'w',  "width", ARGS_VAL_TYPE_INTEGER | ARGS_VAL_TYPE_MANDATORY, { .i = 0 }, 0, NULL,
        "pixel width of input video"
    },
    {
        'h',  "height", ARGS_VAL_TYPE_INTEGER | ARGS_VAL_TYPE_MANDATORY, { .i = 0 },  0, NULL,
        "pixel height of input video"
    },
    {
        'q',  "qp", ARGS_VAL_TYPE_INTEGER, { .i = 17 },  0, NULL,
        "QP value (0~51)"
    },
    {
        'z',  "fps", ARGS_VAL_TYPE_INTEGER | ARGS_VAL_TYPE_MANDATORY, { .i = 30 },  0, NULL,
        "frame rate (frame per second)"
    },
    {
        'I',  "keyint", ARGS_VAL_TYPE_INTEGER, { .i = 200 },  0, NULL,
        "I-picture period"
    },
    {
        'b',  "bframes", ARGS_VAL_TYPE_INTEGER, { .i = 15 },  0, NULL,
        "maximum number of B frames (1,3,7,15)"
    },
    {
        'm',  "threads", ARGS_VAL_TYPE_INTEGER, { .i = 0 },  0, NULL,
        "force to use a specific number of threads"
    },
    {
        'd',  "input-depth", ARGS_VAL_TYPE_INTEGER, { .i = 8 },  0, NULL,
        "input bit depth (8, 10) "
    },
    {
        ARGS_NO_KEY,  "codec-bit-depth", ARGS_VAL_TYPE_INTEGER, { .i = 10 },  0, NULL,
        "codec internal bit depth (10, 12) "
    },
    {
        ARGS_NO_KEY,  "input-csp", ARGS_VAL_TYPE_INTEGER, { .i = 1 },  0, NULL,
        "input color space (chroma format)\n"
        "      - 0: YUV400\n"
        "      - 1: YUV420"
    },
    {
        ARGS_NO_KEY,  "profile", ARGS_VAL_TYPE_STRING, { .str = "baseline" },  0, NULL,
        "profile setting flag  (main, baseline)"
    },
    {
        ARGS_NO_KEY,  "level-idc", ARGS_VAL_TYPE_INTEGER, { .i = 0 },  0, NULL,
        "level setting "
    },
    {
        ARGS_NO_KEY,  "preset", ARGS_VAL_TYPE_STRING, { .str = "fast" },  0, NULL,
        "Encoder PRESET"
        "\t [fast, medium, slow, placebo]"
    },
    {
        ARGS_NO_KEY,  "tune", ARGS_VAL_TYPE_STRING, { .str = "psnr" },  0, NULL,
        "Encoder TUNE"
        "\t [psnr, zerolatency]"
    },
    {
        ARGS_NO_KEY,  "aq-mode", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "use adaptive quantization block qp adaptation\n"
        "      - 0: off\n"
        "      - 1: adaptive quantization"
    },
    {
        ARGS_NO_KEY,  "frames", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "maximum number of frames to be encoded"
    },
    {
        ARGS_NO_KEY,  "seek", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "number of skipped frames before encoding"
    },
    {
        ARGS_NO_KEY,  "info", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "embed SEI messages identifying encoder parameters and command line arguments"
        "      - 0: off\n"
        "      - 1: emit sei info"
    },
    {
        ARGS_NO_KEY,  "hash", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "embed picture signature (HASH) for conformance checking in decoding"
    },
    {
        ARGS_NO_KEY,  "cutree", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "use cutree block qp adaptation\n"
        "      - 0: off\n"
        "      - 1: cutree"
    },
    {
        ARGS_NO_KEY,  "cu-qp-delta-area", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "cu-qp-delta-area (>= 6)"
    },
    {
        ARGS_NO_KEY,  "rdo-dbk-switch", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "switch to on/off rdo-dbk (0, 1) "
    },
    {
        ARGS_NO_KEY,  "ref-pic-gap-length", ARGS_VAL_TYPE_INTEGER, { .i = 1 }, 0, NULL,
        "reference picture gap length (1, 2, 4, 8, 16) only available when -b is 0"
    },
    {
        ARGS_NO_KEY,  "closed-gop", ARGS_VAL_TYPE_NONE, { .i = 0 }, 0, NULL,
        "use closed GOP structure. if not set, open GOP is used"
    },
    {
        ARGS_NO_KEY,  "ibc", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "use IBC feature. if not set, IBC feature is disabled"
    },
    {
        ARGS_NO_KEY,  "ibc-search-range-x", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "set ibc search range in horizontal direction"
    },
    {
        ARGS_NO_KEY,  "ibc-search-range-y", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "set ibc search range in vertical direction"
    },
    {
        ARGS_NO_KEY,  "ibc-hash-search-flag", ARGS_VAL_TYPE_NONE, { .i = 0 }, 0, NULL,
        "use IBC hash based block matching search feature. if not set, it is disable"
    },
    {
        ARGS_NO_KEY,  "ibc-hash-search-max-cand", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Max candidates for hash based IBC search"
    },
    {
        ARGS_NO_KEY,  "ibc-hash-search-range-4smallblk", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Small block search range in IBC based search"
    },
    {
        ARGS_NO_KEY,  "ibc-fast-method", ARGS_VAL_TYPE_INTEGER, { .i = 1 }, 0, NULL,
        "Fast methods for IBC\n"
        "      - 1: Buffer IBC block vector (current not support)\n"
        "      - 2: Adaptive search range"
    },
    {
        ARGS_NO_KEY,  "disable-hgop", ARGS_VAL_TYPE_NONE, { .i = 0 }, 0, NULL,
        "disable hierarchical GOP. if not set, hierarchical GOP is used"
    },
    {
        ARGS_NO_KEY,  "btt", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "binary and ternary splits on/off flag"
    },
    {
        ARGS_NO_KEY,  "suco", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "split unit coding ordering on/off flag"
    },
    {
        ARGS_NO_KEY,  "qp-add-frm", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "one more qp are added after this number of frames, disable:0"
    },
    {
        ARGS_NO_KEY,  "ctu", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Max size of Coding Block (log scale)"
    },
    {
        ARGS_NO_KEY,  "min-cu-size", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "MIN size of Coding Block (log scale)"
    },
    {
        ARGS_NO_KEY,  "cu14-max", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Max size of 4N in 4NxN or Nx4N block (log scale)"
    },
    {
        ARGS_NO_KEY,  "tris-max", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Max size of Tri-split allowed"
    },
    {
        ARGS_NO_KEY,  "tris-min", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Min size of Tri-split allowed"
    },
    {
        ARGS_NO_KEY,  "suco-max", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Max size of suco allowed from top"
    },
    {
        ARGS_NO_KEY,  "suco-min", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Min size of suco allowed from top"
    }
    ,
    {
        ARGS_NO_KEY,  "amvr", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "amvr on/off flag"
    },
    {
        ARGS_NO_KEY,  "mmvd", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "mmvd on/off flag"
    },
    {
        ARGS_NO_KEY,  "affine", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "affine on/off flag"
    },
    {
        ARGS_NO_KEY,  "dmvr", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "dmvr on/off flag"
    },
    {
        ARGS_NO_KEY,  "addb", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "addb on/off flag"
    },
    {
        ARGS_NO_KEY,  "alf", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "alf on/off flag"
    },
    {
        ARGS_NO_KEY,  "htdf", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "htdf on/off flag"
    },
    {
        ARGS_NO_KEY,  "admvp", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "admvp on/off flag"
    },
    {
        ARGS_NO_KEY,  "hmvp", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "hmvp on/off flag"
    },

    {
        ARGS_NO_KEY,  "eipd", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "eipd on/off flag"
    },
    {
        ARGS_NO_KEY,  "iqt", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "iqt on/off flag"
    },
    {
        ARGS_NO_KEY,  "cm-init", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "cm-init on/off flag"
    },
    {
        ARGS_NO_KEY,  "adcc", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "adcc on/off flag"
    },
    {
        ARGS_NO_KEY,  "rpl", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "rpl on/off flag"
    },
    {
        ARGS_NO_KEY,  "pocs", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "pocs on/off flag"
    },
    {
        ARGS_NO_KEY,  "qp-cb-offset", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "cb qp offset"
    },
    {
        ARGS_NO_KEY,  "qp-cr-offset", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "cr qp offset"
    },
    {
        ARGS_NO_KEY, "ats", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "ats on/off flag"
    },
    {
        ARGS_NO_KEY,  "constrained-intra-pred", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "constrained intra pred"
    },
    {
        ARGS_NO_KEY,  "deblock", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Deblocking filter on/off flag"
    },
    {
        ARGS_NO_KEY,  "dbfoffsetA", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "ADDB Deblocking filter offset for alpha"
    },
    {
        ARGS_NO_KEY,  "dbfoffsetB", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "ADDB Deblocking filter offset for beta"
    },
    {
        ARGS_NO_KEY,  "tile-uniform-spacing", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "uniform or non-uniform tile spacing"
    },
    {
        ARGS_NO_KEY,  "num-tile-columns", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Number of tile columns"
    },
    {
        ARGS_NO_KEY,  "num-tile-rows", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Number of tile rows"
    },
    {
        ARGS_NO_KEY,  "tile-column-width-array", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "Array of Tile Column Width"
    },
    {
        ARGS_NO_KEY,  "tile-row-height-array", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "Array of Tile Row Height"
    },
    {
        ARGS_NO_KEY,  "num-slices-in-pic", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Number of slices in the pic"
    },
    {
        ARGS_NO_KEY,  "tile-array-in-slice", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "Array of Slice Boundaries"
    },
    {
        ARGS_NO_KEY,  "arbitrary-slice-flag", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Array of Slice Boundaries"
    },
    {
        ARGS_NO_KEY,  "num-remaining-tiles-in-slice", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "Array of Slice Boundaries"
    },
    {
        ARGS_NO_KEY,  "lp-filter-across-tiles-en-flag", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Loop filter across tiles enabled or disabled"
    },
    {
        ARGS_NO_KEY,  "rc-type", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Rate control type, (0: OFF, 1: ABR, 2: CRF)"
    },
    {
        ARGS_NO_KEY,  "bitrate", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "Bitrate in terms of kilo-bits per second: Kbps(none,K,k), Mbps(M,m)\n"
        "      ex) 100 = 100K = 0.1M"
    },
    {
        ARGS_NO_KEY,  "crf", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Constant Rate Factor CRF-value [10-49]"
    },
    {
        ARGS_NO_KEY,  "vbv-bufsize", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "VBV buffer size: Kbits(none,K,k), Mbits(M,m)\n"
        "      ex) 100 / 100K / 0.1M"
    },
    {
        ARGS_NO_KEY,  "use-filler", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "user filler flag"
    },
    {
        ARGS_NO_KEY,  "lookahead", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "number of pre analysis frames for rate control and cutree, disable:0"
    },
    {
        ARGS_NO_KEY,  "chroma-qp-table-present-flag", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "chroma-qp-table-present-flag"
    },
    {
        ARGS_NO_KEY,  "chroma-qp-num-points-in-table", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "Number of pivot points for Cb and Cr channels"
    },
    {
        ARGS_NO_KEY,  "chroma-qp-delta-in-val-cb", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "Array of input pivot points for Cb"
    },
    {
        ARGS_NO_KEY,  "chroma-qp-delta-out-val-cb", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "Array of input pivot points for Cb"
    },
    {
        ARGS_NO_KEY,  "chroma-qp-delta-in-val-cr", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "Array of input pivot points for Cr"
    },
    {
        ARGS_NO_KEY,  "chroma-qp-delta-out-val-cr", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "Array of input pivot points for Cr"
    },

    {
        ARGS_NO_KEY,  "dra-enable-flag", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "DRA enable flag"
    },
    {
        ARGS_NO_KEY,  "dra-number-ranges", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Number of DRA ranges"
    },
    {
        ARGS_NO_KEY,  "dra-range", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "Array of dra ranges"
    },
    {
        ARGS_NO_KEY,  "dra-scale", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "Array of input dra ranges"
    },
    {
        ARGS_NO_KEY,  "dra-chroma-qp-scale", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "DRA chroma qp scale value"
    },
    {
        ARGS_NO_KEY,  "dra-chroma-qp-offset", ARGS_VAL_TYPE_STRING, { .str = " "},
        0, NULL ,
        "DRA chroma qp offset"
    },
    {
        ARGS_NO_KEY,  "dra-chroma-cb-scale", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "DRA chroma cb scale"
    },
    {
        ARGS_NO_KEY,  "dra-chroma-cr-scale", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "DRA chroma cr scale"
    },
    {
        ARGS_NO_KEY,  "dra-hist-norm", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "DRA hist norm"
    },
    {
        ARGS_NO_KEY,  "rpl-extern", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Whether to input external RPL"
    },
    {
        ARGS_NO_KEY,  "inter-slice-type", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "INTER-SLICE-TYPE"
    },
    {
        ARGS_NO_KEY,  "picture-cropping-flag", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "picture crop flag"
    },
    {
        ARGS_NO_KEY,  "picture-crop-left", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "left offset of picture crop"
    },
    {
        ARGS_NO_KEY,  "picture-crop-right", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "right offset of picture crop"
    },
    {
        ARGS_NO_KEY,  "picture-crop-top", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "top offset of picture crop"
    },
    {
        ARGS_NO_KEY,  "picture-crop-bottom", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "bottom offset of picture crop"
    },
    {
        ARGS_NO_KEY,  "ref", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Number of reference pictures"
    },
    {
        ARGS_NO_KEY,  "sar", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "sar <width:height|int> possible values 1 to 16 and 255"
    },
    {
        ARGS_NO_KEY,  "sar-width", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "sar <width:height|int>"
    },
    {
        ARGS_NO_KEY,  "sar-height", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "sar <width:height|int>"
    },
    {
        ARGS_NO_KEY,  "videoformat", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        " 0-component, 1-pal, 2-ntsc, 3-secam, 4-mac. 5-unspecified"
    },
    {
        ARGS_NO_KEY,  "range", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "black level and range of luma and chroma signals as 1- full or 0- limited"
    },
    {
        ARGS_NO_KEY,  "colorprim", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "1- bt709, 2-unspecified, 3- reserved, 4- bt470m, 5- bt470bg, 6- smpte170m,\
         7- smpte240m, 8- Generic film, 9- bt2020, 10-smpte428, 11-smpte431, 12-smpte432, \
         22-EBU Tech. 3213 Default 2-unspecified"
    },
    {
        ARGS_NO_KEY,  "transfer", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "1- transfer characteristics from bt709, 2-unspecified, 3-reserved, 4-bt470m, 5-bt470bg, 6-smpte170m,\
         7-smpte240m, 8-linear, 9-log100, 10-log316, 11-iec61966-2-4, 12-bt1361e, 13-iec61966-2-1,\
         14-bt2020-10, 15-bt2020-12, 16-smpte2084, 17-smpte428, 198-arib-std-b67. Default 2-unspecified"
    },
    {
        ARGS_NO_KEY,  "matrix-coefficients", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "0-gbr, 1-bt709, 2-unspecified, 3-reserved, 4-fcc, 5-bt470bg, 6-smpte170m, 7-smpte240m, \
          8-ycgco, 9-bt2020nc, 10-bt2020c, 11-smpte2085, 12-chroma-derived-nc, 13-chroma-derived-c, 14-ictcp, 15-255 reserved}; "
    },
    {
        ARGS_NO_KEY,  "master-display", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "SMPTE ST 2086 master display color volume info SEI (HDR)\
          format: G(x,y)B(x,y)R(x,y)WP(x,y)L(max,min)"
    },
    {
        ARGS_NO_KEY,  "max-content-light-level", ARGS_VAL_TYPE_STRING, { .str = " "}, 0, NULL,
        "Specify content light level info SEI as (cll,fall) (HDR)"
    },
    {
        ARGS_NO_KEY,  "chromaloc-tf", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Chroma location for Top field - Range from 0 to 5"
    },
    {
        ARGS_NO_KEY,  "chromaloc-bf", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Chroma location for Bottom field - Range from 0 to 5"
    },
    {
        ARGS_NO_KEY,  "neutral-chroma-flag", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Value can be 0 or 1"
    },
    {
        ARGS_NO_KEY,  "frame-field-flag", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "1 indicates fields and 0 indicates frames"
    },
    {
        ARGS_NO_KEY,  "units-in-tick", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Number of units in tick, value should be > 0"
    },
    {
        ARGS_NO_KEY,  "time-scale", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Time Scale, value should be > 0"
    },
    {
        ARGS_NO_KEY,  "fixed-pic-rate-flag", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Fixed picture rate flag, default 0"
    },
    {
        ARGS_NO_KEY,  "pic-struct", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "Fixed picture rate flag, default 0"
    },
    {
        ARGS_NO_KEY,  "mv-over-pic-boundaries", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 1, NULL,
        "mvs over picture boundaries flag"
    },
    {
        ARGS_NO_KEY,  "max-bytes-per-pic-denom", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 2, NULL,
        "max bytes per picture denom, valid range 0 to 16"
    },
    {
        ARGS_NO_KEY,  "max-bits-per-cu-denom", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 1, NULL,
        "max bits per cu denom, valid range 0 to 16"
    },
    {
        ARGS_NO_KEY,  "log2-max-mv-len-hor", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 16, NULL,
        "max mv length horizontal log2, valid range 0 to 16"
    },
    {
        ARGS_NO_KEY,  "log2-max-mv-len-ver", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 16, NULL,
        "max mv length vertical log2, valid range o to 16"
    },
    {
        ARGS_NO_KEY,  "num-reorder-pics", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "# of reorder pics, valid range 0 to max_dec_pic_buffering \
         default = max_dec_pic_buffering"
    },
    {
        ARGS_NO_KEY,  "max-dec-pic-buffering", ARGS_VAL_TYPE_INTEGER, { .i = 0 }, 0, NULL,
        "max picture buffering in decoder, valid range 0 to num-reorder-pic \
         default num-reorder-pic"
    },
    {ARGS_END_KEY, "", ARGS_VAL_TYPE_NONE, { .i = 0 }, 0, NULL, ""} /* termination */
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
        if (o->val_type & ARGS_VAL_TYPE_MANDATORY)
        {
            if (o->flag == 0)
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

    if(!args->opts[idx].val)
        return XEVE_ERR;

    if( args->opts[idx].flag != 1 )
        return XEVE_ERR;

    opt = args->opts + idx;

    switch(ARGS_GET_CMD_OPT_VAL_TYPE(opt->val_type))
    {
        case ARGS_VAL_TYPE_INTEGER:
            snprintf(buf, ARGS_OPT_VALUE_MAXLEN, "%d", *((int*)(args->opts[idx].val)));

            len = strlen(buf);
            *val = (char*)malloc(len+1);
            strncpy(*val, buf, len+1);
            // *val[len-1] = '\0';

            break;
        case ARGS_VAL_TYPE_STRING:

            str = (char*)(args->opts[idx].val);
            if(!str) return XEVE_ERR;

            len = strlen(str);
            *val = (char*)malloc(len+1);
            strncpy(*val, str, len+1);

            break;
        case ARGS_VAL_TYPE_NONE:
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

    if(!args->opts[idx].val)
        return XEVE_ERR;

    opt = args->opts + idx;

    switch(ARGS_GET_CMD_OPT_VAL_TYPE(opt->val_type))
    {
        case ARGS_VAL_TYPE_INTEGER:
            ival = strtol(val, &endptr, 10);
            if (*endptr != '\0')
                return XEVE_ERR_INVALID_ARGUMENT;

            *((int*)(args->opts[idx].val)) = ival;
            args->opts[idx].flag = 1;

            break;
        case ARGS_VAL_TYPE_STRING:

            strncpy((char*)(args->opts[idx].val), val, ARGS_OPT_VALUE_MAXLEN-1);
            args->opts[idx].flag = 1;

            break;
        case ARGS_VAL_TYPE_NONE:
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
    char default_value[ARGS_OPT_DEFAULT_VALUE_MAXLEN] = { 0 };

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

    switch(ARGS_GET_CMD_OPT_VAL_TYPE(opt->val_type))
    {
        case ARGS_VAL_TYPE_INTEGER:
            strcpy(vtype, "INTEGER");
            if(opt->val != NULL) snprintf(default_value, ARGS_OPT_DEFAULT_VALUE_MAXLEN, " [%d]", *(int*)(opt->val));
            break;
        case ARGS_VAL_TYPE_STRING:
            strcpy(vtype, "STRING");
            if(opt->val != NULL) snprintf(default_value, ARGS_OPT_DEFAULT_VALUE_MAXLEN, " [%s]", strlen((char*)(opt->val)) == 0 ? "None" : (char*)(opt->val));
            break;
        case ARGS_VAL_TYPE_NONE:
        default:
            strcpy(vtype, "FLAG");
            if(opt->val != NULL) snprintf(default_value, ARGS_OPT_DEFAULT_VALUE_MAXLEN, " [%s]", *(int*)(opt->val) ? "On" : "Off");
            break;
    }

    optional = !(opt->val_type & ARGS_VAL_TYPE_MANDATORY);

    // allocate memorry for help string
    *help = (char*)malloc(ARGS_OPT_HELP_STRING_MAXLEN);

    if(opt->key != ARGS_NO_KEY)
    {
        snprintf(*help, ARGS_OPT_HELP_STRING_MAXLEN, "  -%c, --%s [%s]%s%s\n    : %s", opt->key, opt->key_long,
                vtype, (optional) ? " (optional)" : "", (optional) ? default_value : "", opt->desc);
    }
    else
    {
        snprintf(*help, ARGS_OPT_HELP_STRING_MAXLEN, "  --%s [%s]%s%s\n    : %s", opt->key_long,
                vtype, (optional) ? " (optional)" : "", (optional) ? default_value : "", opt->desc);
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

    // do some checks
    if( !args)
        return NULL;

    if(keyl)
        idx = args_search_long_key(args->opts, keyl);

    if( idx  < 0 )
        return NULL;

    // if next idx is out of bound
    if((idx + 1)> args->num_option-1) return NULL;

    opt = args->opts + (idx+1);

    return opt->key_long;
}

#if 0
int xeve_args_get_option_description(ARGS_PARSER * args, int idx, char ** help)
{
    int optional = 0;
    char vtype[32];
    ARGS_OPT * o = NULL;
    char default_value[ARGS_OPT_DEFAULT_VALUE_MAXLEN] = { 0 };

    // do some checks
    if(!args || idx<0 || idx>(args->num_option-1))
        return XEVE_ERR_INVALID_ARGUMENT;

    // allocate memorry for help string
    *help = (char*)malloc(ARGS_OPT_HELP_STRING_MAXLEN);

    o = args->opts + idx;

    switch(ARGS_GET_CMD_OPT_VAL_TYPE(o->val_type))
    {
        case ARGS_VAL_TYPE_INTEGER:
            strcpy(vtype, "INTEGER");
            if(o->val != NULL) snprintf(default_value, ARGS_OPT_DEFAULT_VALUE_MAXLEN, " [%d]", *(int*)(o->val));
            break;
        case ARGS_VAL_TYPE_STRING:
            strcpy(vtype, "STRING");
            if(o->val != NULL) snprintf(default_value, ARGS_OPT_DEFAULT_VALUE_MAXLEN, " [%s]", strlen((char*)(o->val)) == 0 ? "None" : (char*)(o->val));
            break;
        case ARGS_VAL_TYPE_NONE:
        default:
            strcpy(vtype, "FLAG");
            if(o->val != NULL) snprintf(default_value, ARGS_OPT_DEFAULT_VALUE_MAXLEN, " [%s]", *(int*)(o->val) ? "On" : "Off");
            break;
    }
    optional = !(o->val_type & ARGS_VAL_TYPE_MANDATORY);

    if(o->key != ARGS_NO_KEY)
    {
        snprintf(*help, ARGS_OPT_HELP_STRING_MAXLEN, "  -%c, --%s [%s]%s%s\n    : %s", o->key, o->key_long,
                vtype, (optional) ? " (optional)" : "", (optional) ? default_value : "", o->desc);
    }
    else
    {
        snprintf(*help, ARGS_OPT_HELP_STRING_MAXLEN, "  --%s [%s]%s%s\n    : %s", o->key_long,
                vtype, (optional) ? " (optional)" : "", (optional) ? default_value : "", o->desc);
    }

    return XEVE_OK;
}
#endif

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
    if(argv == NULL || ops->val == NULL)
    {
        return -1;
    }
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
            ops[oidx].flag = 1;
        }
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

    if(ARGS_GET_CMD_OPT_VAL_TYPE(ops[oidx].val_type) !=
       ARGS_VAL_TYPE_NONE)
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
        *((int*)ops[oidx].val) = 1;
    }
    ops[oidx].flag = 1;
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
    opts[idx].val = var;
    return 0;
}

static int args_set_storage_for_option_by_short_key(ARGS_OPT * opts, char * key, void * var)
{
    int idx;
    idx = args_search_short_arg(opts, key[0]);
    if (idx < 0) return -1;
    opts[idx].val = var;
    return 0;
}

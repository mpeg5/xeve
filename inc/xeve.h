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

#ifndef _XEVE_H_
#define _XEVE_H_

#include <xeve_exports.h>

#define XEVE_MAX_TASK_CNT               8
#define MAX_NUM_TILES_ROW               22
#define MAX_NUM_TILES_COL               20

#define MAX_QP_TABLE_SIZE               58
#define MAX_QP_TABLE_SIZE_EXT           94

/*****************************************************************************
 * return values and error code
 *****************************************************************************/
/* no more frames, but it is OK */
#define XEVE_OK_NO_MORE_FRM              (205)
/* progress success, but output is not available temporarily */
#define XEVE_OK_OUT_NOT_AVAILABLE        (204)
/* frame dimension (width or height) has been changed */
#define XEVE_OK_DIM_CHANGED              (203)
/* decoding success, but output frame has been delayed */
#define XEVE_OK_FRM_DELAYED              (202)
/* not matched CRC value */
#define XEVE_ERR_BAD_CRC                 (201)
/* CRC value presented but ignored at decoder*/
#define XEVE_WARN_CRC_IGNORED            (200)
#define XEVE_OK                          (0)
#define XEVE_ERR                         (-1) /* generic error */
#define XEVE_ERR_INVALID_ARGUMENT        (-101)
#define XEVE_ERR_OUT_OF_MEMORY           (-102)
#define XEVE_ERR_REACHED_MAX             (-103)
#define XEVE_ERR_UNSUPPORTED             (-104)
#define XEVE_ERR_UNEXPECTED              (-105)
#define XEVE_ERR_UNSUPPORTED_COLORSPACE  (-201)
#define XEVE_ERR_MALFORMED_BITSTREAM     (-202)
#define XEVE_ERR_UNKNOWN                 (-32767) /* unknown error */

/* return value checking */
#define XEVE_SUCCEEDED(ret)              ((ret) >= XEVE_OK)
#define XEVE_FAILED(ret)                 ((ret) < XEVE_OK)

/*****************************************************************************
 * color spaces
 * - value format = (endian << 14) | (bit-depth << 8) | (color format)
 * - endian (1bit): little endian = 0, big endian = 1
 * - bit-depth (6bit): 0~63
 * - color format (8bit): 0~255
 *****************************************************************************/
/* color formats */
#define XEVE_CF_UNKNOWN                 0 /* unknown color format */
#define XEVE_CF_YCBCR400                10 /* Y only */
#define XEVE_CF_YCBCR420                11 /* YCbCr 420 */
#define XEVE_CF_YCBCR422                12 /* YCBCR 422 narrow chroma*/
#define XEVE_CF_YCBCR444                13 /* YCBCR 444*/
#define XEVE_CF_YCBCR422N               XEVE_CF_YCBCR422
#define XEVE_CF_YCBCR422W               18 /* YCBCR422 wide chroma */

/* macro for color space */
#define XEVE_CS_GET_FORMAT(cs)           (((cs) >> 0) & 0xFF)
#define XEVE_CS_GET_BIT_DEPTH(cs)        (((cs) >> 8) & 0x3F)
#define XEVE_CS_GET_BYTE_DEPTH(cs)       ((XEVE_CS_GET_BIT_DEPTH(cs)+7)>>3)
#define XEVE_CS_GET_ENDIAN(cs)           (((cs) >> 14) & 0x1)
#define XEVE_CS_SET(f, bit, e)           (((e) << 14) | ((bit) << 8) | (f))
#define XEVE_CS_SET_FORMAT(cs, v)        (((cs) & ~0xFF) | ((v) << 0))
#define XEVE_CS_SET_BIT_DEPTH(cs, v)     (((cs) & ~(0x3F<<8)) | ((v) << 8))
#define XEVE_CS_SET_ENDIAN(cs, v)        (((cs) & ~(0x1<<14)) | ((v) << 14))

/* pre-defined color spaces */
#define XEVE_CS_UNKNOWN                  XEVE_CS_SET(0,0,0)
#define XEVE_CS_YCBCR400                 XEVE_CS_SET(XEVE_CF_YCBCR400, 8, 0)
#define XEVE_CS_YCBCR420                 XEVE_CS_SET(XEVE_CF_YCBCR420, 8, 0)
#define XEVE_CS_YCBCR422                 XEVE_CS_SET(XEVE_CF_YCBCR422, 8, 0)
#define XEVE_CS_YCBCR444                 XEVE_CS_SET(XEVE_CF_YCBCR444, 8, 0)
#define XEVE_CS_YCBCR400_10LE            XEVE_CS_SET(XEVE_CF_YCBCR400, 10, 0)
#define XEVE_CS_YCBCR420_10LE            XEVE_CS_SET(XEVE_CF_YCBCR420, 10, 0)
#define XEVE_CS_YCBCR422_10LE            XEVE_CS_SET(XEVE_CF_YCBCR422, 10, 0)
#define XEVE_CS_YCBCR444_10LE            XEVE_CS_SET(XEVE_CF_YCBCR444, 10, 0)
#define XEVE_CS_YCBCR400_12LE            XEVE_CS_SET(XEVE_CF_YCBCR400, 12, 0)
#define XEVE_CS_YCBCR420_12LE            XEVE_CS_SET(XEVE_CF_YCBCR420, 12, 0)
#define XEVE_CS_YCBCR400_14LE            XEVE_CS_SET(XEVE_CF_YCBCR400, 14, 0)
#define XEVE_CS_YCBCR420_14LE            XEVE_CS_SET(XEVE_CF_YCBCR420, 14, 0)

/*****************************************************************************
 * config types
 *****************************************************************************/
#define XEVE_CFG_SET_FORCE_OUT          (102)
#define XEVE_CFG_SET_FINTRA             (200)
#define XEVE_CFG_SET_QP                 (201)
#define XEVE_CFG_SET_BPS                (202)
#define XEVE_CFG_SET_VBV_SIZE           (203)
#define XEVE_CFG_SET_FPS                (204)
#define XEVE_CFG_SET_I_PERIOD           (207)
#define XEVE_CFG_SET_QP_MIN             (208)
#define XEVE_CFG_SET_QP_MAX             (209)
#define XEVE_CFG_SET_BU_SIZE            (210)
#define XEVE_CFG_SET_USE_DEBLOCK        (211)
#define XEVE_CFG_SET_DEBLOCK_A_OFFSET   (212)
#define XEVE_CFG_SET_DEBLOCK_B_OFFSET   (213)
#define XEVE_CFG_SET_USE_PIC_SIGNATURE  (301)
#define XEVE_CFG_GET_COMPLEXITY         (500)
#define XEVE_CFG_GET_SPEED              (501)
#define XEVE_CFG_GET_QP_MIN             (600)
#define XEVE_CFG_GET_QP_MAX             (601)
#define XEVE_CFG_GET_QP                 (602)
#define XEVE_CFG_GET_RCT                (603)
#define XEVE_CFG_GET_BPS                (604)
#define XEVE_CFG_GET_FPS                (605)
#define XEVE_CFG_GET_I_PERIOD           (608)
#define XEVE_CFG_GET_BU_SIZE            (609)
#define XEVE_CFG_GET_USE_DEBLOCK        (610)
#define XEVE_CFG_GET_CLOSED_GOP         (611)
#define XEVE_CFG_GET_HIERARCHICAL_GOP   (612)
#define XEVE_CFG_GET_DEBLOCK_A_OFFSET   (613)
#define XEVE_CFG_GET_DEBLOCK_B_OFFSET   (614)
#define XEVE_CFG_GET_WIDTH              (701)
#define XEVE_CFG_GET_HEIGHT             (702)
#define XEVE_CFG_GET_RECON              (703)
#define XEVE_CFG_GET_SUPPORT_PROF       (704)

/*****************************************************************************
 * NALU types
 *****************************************************************************/
#define XEVE_NONIDR_NUT                  (0)
#define XEVE_IDR_NUT                     (1)
#define XEVE_SPS_NUT                     (24)
#define XEVE_PPS_NUT                     (25)
#define XEVE_APS_NUT                     (26)
#define XEVE_FD_NUT                      (27)
#define XEVE_SEI_NUT                     (28)

/*****************************************************************************
 * slice type
 *****************************************************************************/
#define XEVE_ST_UNKNOWN                  (-1)
#define XEVE_ST_B                        (0)
#define XEVE_ST_P                        (1)
#define XEVE_ST_I                        (2)

/*****************************************************************************
 * Encoder optimization level control
 *****************************************************************************/
enum XEVE_PRESET
{
    XEVE_PRESET_FAST,
    XEVE_PRESET_MEDIUM,
    XEVE_PRESET_SLOW,
    XEVE_PRESET_PLACEBO,
    XEVE_PRESET_MAX /* number of PRESETs */
};

enum XEVE_TUNE
{
    XEVE_TUNE_NONE,
    XEVE_TUNE_ZEROLATENCY,
    XEVE_TUNE_PSNR,
    XEVE_TUNE_MAX /* number of TUNEs*/
};

/*****************************************************************************
 * type and macro for media time
 *****************************************************************************/
typedef long long                        XEVE_MTIME; /* in 100-nanosec unit */

/*****************************************************************************
 * image buffer format
 *
 *    baddr
 *     +---------------------------------------------------+ ---
 *     |                                                   |  ^
 *     |                                              |    |  |
 *     |    a                                         v    |  |
 *     |   --- +-----------------------------------+ ---   |  |
 *     |    ^  |  (x, y)                           |  y    |  |
 *     |    |  |   +---------------------------+   + ---   |  |
 *     |    |  |   |                           |   |  ^    |  |
 *     |    |  |   |            /\             |   |  |    |  |
 *     |    |  |   |           /  \            |   |  |    |  |
 *     |    |  |   |          /    \           |   |  |    |  |
 *     |       |   |  +--------------------+   |   |       |
 *     |    ah |   |   \                  /    |   |  h    |  e
 *     |       |   |    +----------------+     |   |       |
 *     |    |  |   |       |          |        |   |  |    |  |
 *     |    |  |   |      @    O   O   @       |   |  |    |  |
 *     |    |  |   |        \    ~   /         |   |  v    |  |
 *     |    |  |   +---------------------------+   | ---   |  |
 *     |    v  |                                   |       |  |
 *     |   --- +---+-------------------------------+       |  |
 *     |     ->| x |<----------- w ----------->|           |  |
 *     |       |<--------------- aw -------------->|       |  |
 *     |                                                   |  v
 *     +---------------------------------------------------+ ---
 *
 *     |<---------------------- s ------------------------>|
 *
 * - x, y, w, aw, h, ah : unit of pixel
 * - s, e : unit of byte
 *****************************************************************************/

#define XEVE_IMGB_MAX_PLANE              (4)

typedef struct _XEVE_IMGB XEVE_IMGB;
struct _XEVE_IMGB
{
    int                 cs; /* color space */
    int                 np; /* number of plane */
    /* width (in unit of pixel) */
    int                 w[XEVE_IMGB_MAX_PLANE];
    /* height (in unit of pixel) */
    int                 h[XEVE_IMGB_MAX_PLANE];
    /* X position of left top (in unit of pixel) */
    int                 x[XEVE_IMGB_MAX_PLANE];
    /* Y postion of left top (in unit of pixel) */
    int                 y[XEVE_IMGB_MAX_PLANE];
    /* buffer stride (in unit of byte) */
    int                 s[XEVE_IMGB_MAX_PLANE];
    /* buffer elevation (in unit of byte) */
    int                 e[XEVE_IMGB_MAX_PLANE];
    /* address of each plane */
    void              * a[XEVE_IMGB_MAX_PLANE];

    /* time-stamps */
    XEVE_MTIME          ts[4];

    int                 ndata[4]; /* arbitrary data, if needs */
    void              * pdata[4]; /* arbitrary adedress if needs */

    /* aligned width (in unit of pixel) */
    int                 aw[XEVE_IMGB_MAX_PLANE];
    /* aligned height (in unit of pixel) */
    int                 ah[XEVE_IMGB_MAX_PLANE];

    /* left padding size (in unit of pixel) */
    int                 padl[XEVE_IMGB_MAX_PLANE];
    /* right padding size (in unit of pixel) */
    int                 padr[XEVE_IMGB_MAX_PLANE];
    /* up padding size (in unit of pixel) */
    int                 padu[XEVE_IMGB_MAX_PLANE];
    /* bottom padding size (in unit of pixel) */
    int                 padb[XEVE_IMGB_MAX_PLANE];

    /* address of actual allocated buffer */
    void              * baddr[XEVE_IMGB_MAX_PLANE];
    /* actual allocated buffer size */
    int                 bsize[XEVE_IMGB_MAX_PLANE];

    /* life cycle management */
    int                 refcnt;
    int                 (*addref)(XEVE_IMGB * imgb);
    int                 (*getref)(XEVE_IMGB * imgb);
    int                 (*release)(XEVE_IMGB * imgb);
};

/*****************************************************************************
 * Bitstream buffer
 *****************************************************************************/
typedef struct _XEVE_BITB
{
    /* user space address indicating buffer */
    void              * addr;
    /* physical address indicating buffer, if any */
    void              * pddr;
    /* byte size of buffer memory */
    int                 bsize;
    /* byte size of bitstream in buffer */
    int                 ssize;
    /* bitstream has an error? */
    int                 err;
    /* arbitrary data, if needs */
    int                 ndata[4];
    /* arbitrary address, if needs */
    void              * pdata[4];
    /* time-stamps */
    XEVE_MTIME          ts[4];

} XEVE_BITB;

#define MAX_NUM_REF_PICS                   21
#define MAX_NUM_ACTIVE_REF_FRAME           5
#define MAX_NUM_RPLS                       32

/* rpl structure */
typedef struct _XEVE_RPL
{
    int             poc;
    int             tid;
    int             ref_pic_num;
    int             ref_pic_active_num;
    int             ref_pics[MAX_NUM_REF_PICS];
    char            pic_type;
} XEVE_RPL;

/* chromaQP table structure */
typedef struct _XEVE_CHROMA_TABLE
{
    int             chroma_qp_table_present_flag;
    int             same_qp_table_for_chroma;
    int             global_offset_flag;
    int             num_points_in_qp_table_minus1[2];
    int             delta_qp_in_val_minus1[2][MAX_QP_TABLE_SIZE];
    int             delta_qp_out_val[2][MAX_QP_TABLE_SIZE];
} XEVE_CHROMA_TABLE;

/*****************************************************************************
 * description for creating
 *****************************************************************************/
typedef struct _XEVE_CDSC_EXT XEVE_CDSC_EXT;

typedef struct _XEVE_CDSC
{
    /* width of input frame */
    int            w;
    /* height of input frame */
    int            h;
    /* frame rate (Hz) */
    int            fps;
    /* period of I-frame.
    - 0: only one I-frame at the first time.
    - 1: every frame is coded in I-frame
    */
    int            iperiod;
    /* quantization parameter */
    int            qp;
	/* quantization parameter offset for CB */
    int            qp_cb_offset;
	/* quantization parameter offset for CR */
    int            qp_cr_offset;
	/* Rate control type */
    int            rc_type;
	/* bitrate */
    int            bps;
	/* VBV buffer size for rate control*/
    int            vbv_buf_size;
    int            use_filler_flag;
    int            num_pre_analysis_frames;
    XEVE_CHROMA_TABLE chroma_qp_table_struct;
    /* color space of input image */
    int            cs;
    int            max_b_frames;
    int            disable_hgop;
    int            ref_pic_gap_length;
    /* use closed GOP sturcture
       - 0 : use open GOP (default)
       - 1 : use closed GOP */
    int            closed_gop;
    int            codec_bit_depth;
    int            profile;
    int            level;
    int            aq_mode;
    int            lookahead;
    int            cutree;
    int            constrained_intra_pred;
    int            use_deblock;
    int            threads;
    int            inter_slice_type;
    int            picture_cropping_flag;
    int            picture_crop_left_offset;
    int            picture_crop_right_offset;
    int            picture_crop_top_offset;
    int            picture_crop_bottom_offset;
    int            rdo_dbk_switch;
    int            add_qp_frame;
    int            bitstream_buf_size;
    int            preset;
    int            tune;

    XEVE_CDSC_EXT * ext;
} XEVE_CDSC;


struct _XEVE_CDSC_EXT
{
    /* enable intra-block copy feature
    - 0 : disable IBC (default)
    - 1 : enable IBC featuer */
    int            ibc_flag;
    int            ibc_search_range_x;
    int            ibc_search_range_y;
    int            ibc_hash_search_flag;
    int            ibc_hash_search_max_cand;
    int            ibc_hash_search_range_4smallblk;
    int            ibc_fast_method;
    int            toolset_idc_h;
    int            toolset_idc_l;
    int            btt;
    int            suco;
    int            framework_cb_max;
    int            framework_cb_min;
    int            framework_cu14_max;
    int            framework_tris_max;
    int            framework_tris_min;
    int            framework_suco_max;
    int            framework_suco_min;
    int            tool_amvr;
    int            tool_mmvd;
    int            tool_affine;
    int            tool_dmvr;
    int            tool_addb;
    int            tool_alf;
    int            tool_htdf;
    int            tool_admvp;
    int            tool_hmvp;
    int            tool_eipd;
    int            tool_iqt;
    int            tool_cm_init;
    int            tool_adcc;
    int            tool_rpl;
    int            tool_pocs;
    int            cu_qp_delta_area;
    int            tool_ats;
    int            deblock_aplha_offset;
    int            deblock_beta_offset;
    int            tile_uniform_spacing_flag;
    int            tile_columns;
    int            tile_rows;
    int            tile_column_width_array[20];
    int            tile_row_height_array[22];
    int            num_slice_in_pic;
    int            tile_array_in_slice[2 * 600];
    int            arbitrary_slice_flag;
    int            num_remaining_tiles_in_slice_minus1[600];
    int            loop_filter_across_tiles_enabled_flag;
    void         * dra_mapping_app;
    int            tool_dra;
    double         dra_hist_norm;
    int            dra_num_ranges;
    double         dra_scale_map_y[256][2];
    double         dra_cb_qp_scale;
    double         dra_cr_qp_scale;
    double         dra_chroma_qp_scale;
    double         dra_chroma_qp_offset;
    int            rpl_extern;
    XEVE_RPL       rpls_l0[MAX_NUM_RPLS];
    XEVE_RPL       rpls_l1[MAX_NUM_RPLS];
    int            rpls_l0_cfg_num;
    int            rpls_l1_cfg_num;
};

/*****************************************************************************
 * status
 *****************************************************************************/
typedef struct _XEVE_STAT
{
    /* encoded bitstream byte size */
    int            write;
    /* encoded sei messages byte size */
    int            sei_size;
    /* picture number increased whenever encoding a frame */
    unsigned long  fnum;
    /* nalu type */
    int            nalu_type;
    /* slice type */
    int            stype;
    /* quantization parameter used for encoding */
    int            qp;
    /* picture order count */
    int            poc;
    /* layer id */
    int            tid;
    /* number of reference pictures */
    int            refpic_num[2];
    /* list of reference pictures */
    int            refpic[2][16];

} XEVE_STAT;

/*****************************************************************************
 * API for XEVE
 *****************************************************************************/

/* XEVE instance identifier */
typedef void  * XEVE;

XEVE XEVE_EXPORT xeve_create(XEVE_CDSC * cdsc, int * err);
void XEVE_EXPORT xeve_delete(XEVE id);
int  XEVE_EXPORT xeve_push(XEVE id, XEVE_IMGB * imgb);
int  XEVE_EXPORT xeve_encode(XEVE id, XEVE_BITB * bitb, XEVE_STAT * stat);
int  XEVE_EXPORT xeve_config(XEVE id, int cfg, void * buf, int * size);

#endif /* _XEVE_H_ */


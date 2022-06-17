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

#ifdef __cplusplus

extern "C"
{
#endif

#include <xeve_exports.h>

#define XEVE_MAX_THREADS                 (8)
#define XEVE_MAX_NUM_TILES_ROW           (22)
#define XEVE_MAX_NUM_TILES_COL           (20)

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
#define XEVE_CFG_SET_KEYINT             (207)
#define XEVE_CFG_SET_QP_MIN             (208)
#define XEVE_CFG_SET_QP_MAX             (209)
#define XEVE_CFG_SET_BU_SIZE            (210)
#define XEVE_CFG_SET_USE_DEBLOCK        (211)
#define XEVE_CFG_SET_DEBLOCK_A_OFFSET   (212)
#define XEVE_CFG_SET_DEBLOCK_B_OFFSET   (213)
#define XEVE_CFG_SET_SEI_CMD            (300)
#define XEVE_CFG_SET_USE_PIC_SIGNATURE  (301)
#define XEVE_CFG_GET_COMPLEXITY         (500)
#define XEVE_CFG_GET_SPEED              (501)
#define XEVE_CFG_GET_QP_MIN             (600)
#define XEVE_CFG_GET_QP_MAX             (601)
#define XEVE_CFG_GET_QP                 (602)
#define XEVE_CFG_GET_RCT                (603)
#define XEVE_CFG_GET_BPS                (604)
#define XEVE_CFG_GET_FPS                (605)
#define XEVE_CFG_GET_KEYINT             (608)
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
 * type and macro for media time
 *****************************************************************************/
typedef long long                        XEVE_MTIME; /* in 100-nanosec unit */
#define XEVE_TS_PTS                      0
#define XEVE_TS_DTS                      1
#define XEVE_TS_NUM                      2

/*****************************************************************************
 * profiles
 *****************************************************************************/
#define XEVE_PROFILE_BASELINE            (0)
#define XEVE_PROFILE_MAIN                (1)


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
    XEVE_MTIME          ts[XEVE_TS_NUM];

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
    XEVE_MTIME          ts[XEVE_TS_NUM];

} XEVE_BITB;

#define XEVE_MAX_NUM_TILE_WIDTH                 120
#define XEVE_MAX_NUM_TILE_HEIGHT                64
#define XEVE_MAX_NUM_TILES                      (XEVE_MAX_NUM_TILE_WIDTH * XEVE_MAX_NUM_TILE_HEIGHT)

/*****************************************************************************
 * optimization level control
 *****************************************************************************/
#define XEVE_PRESET_DEFAULT                     0
#define XEVE_PRESET_FAST                        1
#define XEVE_PRESET_MEDIUM                      2
#define XEVE_PRESET_SLOW                        3
#define XEVE_PRESET_PLACEBO                     4

/*****************************************************************************
 * tuning for a specific use-case
 *****************************************************************************/
#define XEVE_TUNE_NONE                          0
#define XEVE_TUNE_ZEROLATENCY                   1
#define XEVE_TUNE_PSNR                          2

/*****************************************************************************
 * rate-control types
 *****************************************************************************/
#define XEVE_RC_CQP                             0
#define XEVE_RC_ABR                             1
#define XEVE_RC_CRF                             2

/*****************************************************************************
 * coding parameters
 *****************************************************************************/
typedef struct _XEVE_PARAM
{
    /* profile : baseline or main */
    int            profile;
    /* number of thread for parallel proessing */
    int            threads;
    /* width of input frame */
    int            w;
    /* height of input frame */
    int            h;
    /* frame rate (Hz) */
    int            fps;
    /* MAX I-frame period in frames.
    - 0: only one I-frame at the first time.
    - 1: every frame is coded in I-frame
    */
    int            keyint;
    /* color space of input image */
    int            cs;
    /* Rate control type */
    int            rc_type;
    /* quantization parameter */
    int            qp;
    /* quantization parameter offset for CB */
    int            qp_cb_offset;
    /* quantization parameter offset for CR */
    int            qp_cr_offset;
    /* bitrate (unit: kbps) */
    int            bitrate;
    /* VBV buffer size for rate control (unit: kbits) */
    int            vbv_bufsize;
    /* CRF Value */
    int            crf;
    /* number of b-frame */
    int            bframes;
    /* adaptive quantizaiton mode */
    int            aq_mode;
    /* number of look-ahead frame buffer */
    int            lookahead;
    /* use closed GOP sturcture
       - 0 : use open GOP (default)
       - 1 : use closed GOP */
    int            closed_gop;
    /* use 'Annex-B (nal_unit_length)' format */
    int            use_annexb;
    /* use filler data for tight constant bitrate */
    int            use_filler;
    /* XEVE_CHROMA_TABLE chroma_qp_table_struct */
    int            chroma_qp_table_present_flag;
    char           chroma_qp_num_points_in_table[256];
    char           chroma_qp_delta_in_val_cb[256];
    char           chroma_qp_delta_out_val_cb[256];
    char           chroma_qp_delta_in_val_cr[256];
    char           chroma_qp_delta_out_val_cr[256];
    int            disable_hgop;
    /* distance between ref pics in addition to closest ref ref pic in LD*/
    int            ref_pic_gap_length;
    /* internal codec bit-depth: EVC uses 10bit */
    int            codec_bit_depth;
    /* level indicator */
    int            level_idc;
    int            cutree;
    int            constrained_intra_pred;
    int            use_deblock;
    int            inter_slice_type;
    int            picture_cropping_flag;
    int            picture_crop_left_offset;
    int            picture_crop_right_offset;
    int            picture_crop_top_offset;
    int            picture_crop_bottom_offset;
    int            rdo_dbk_switch;
    int            qp_incread_frame;
    int            sei_cmd_info;
    int            use_pic_sign;
    int            f_ifrm;
    int            qp_max;
    int            qp_min;
    int            gop_size;
    int            force_output;
    int            use_fcst;
    int            chroma_format_idc;
    int            cs_w_shift;
    int            cs_h_shift;
    /* preset parameter */
    int            max_cu_intra;
    int            min_cu_intra;
    int            max_cu_inter;
    int            min_cu_inter;
    /* maxium number of reference frames  */
    int            ref;
    int            me_ref_num;
    int            me_algo;
    int            me_range;
    int            me_sub;
    int            me_sub_pos;
    int            me_sub_range;
    double         skip_th;             // Use it carefully. If this value is greater than zero, a huge quality drop occurs
    int            merge_num;
    int            rdoq;
    int            cabac_refine;

/*****************************************************************************
* Main Profile Parameters
*****************************************************************************/
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
    int            deblock_alpha_offset;
    int            deblock_beta_offset;
    int            loop_filter_across_tiles_enabled_flag;
    int            tool_dra;
    int            dra_enable_flag;
    int            dra_number_ranges;
    char           dra_range[256];
    char           dra_scale[256];
    char           dra_chroma_qp_scale[256];
    char           dra_chroma_qp_offset[256];
    char           dra_chroma_cb_scale[256];
    char           dra_chroma_cr_scale[256];
    char           dra_hist_norm[256];
    int            tile_uniform_spacing_flag;
    int            tile_columns;
    int            tile_rows;
    char           tile_column_width_array[XEVE_MAX_NUM_TILE_WIDTH];
    char           tile_row_height_array[XEVE_MAX_NUM_TILE_HEIGHT];
    int            num_slice_in_pic;
    char           tile_array_in_slice[XEVE_MAX_NUM_TILES];
    int            arbitrary_slice_flag;
    char           num_remaining_tiles_in_slice_minus1[XEVE_MAX_NUM_TILES >> 1];
    int            rpl_extern;
    /* max num of RPL is 32 */
    char           rpl0[32][256];
    char           rpl1[32][256];
    int            rpls_l0_cfg_num;
    int            rpls_l1_cfg_num;
    /* preset parameter */
    int            ats_intra_fast;
    int            me_fast;
    /* VUI options*/
    int  sar;
    int  sar_width, sar_height;
    int  videoformat;
    int  range;
    int  colorprim;
    int  transfer;
    int  matrix_coefficients;
    int  overscan_info_present_flag;
    int  overscan_appropriate_flag;
    int  chroma_loc_info_present_flag;
    int  chroma_sample_loc_type_top_field;
    int  chroma_sample_loc_type_bottom_field;
    int  neutral_chroma_indication_flag;
    int  field_seq_flag;
    int  timing_info_present_flag;
    int  num_units_in_tick;
    int  time_scale;
    int  fixed_pic_rate_flag;
    int  nal_hrd_parameters_present_flag;
    int  vcl_hrd_parameters_present_flag;
    int  low_delay_hrd_flag;
    int  pic_struct_present_flag;
    int  bitstream_restriction_flag;
    int  motion_vectors_over_pic_boundaries_flag;
    int  max_bytes_per_pic_denom;
    int  max_bits_per_mb_denom;
    int  log2_max_mv_length_horizontal;
    int  log2_max_mv_length_vertical;
    int  num_reorder_pics;
    int  max_dec_pic_buffering;
    int aspect_ratio_info_present_flag;
    int video_signal_type_present_flag;
    int colour_description_present_flag;

    /* SEI options*/
    int  master_display;
    int  max_cll;
    int  max_fall;
} XEVE_PARAM;

/*****************************************************************************
 * description for creating
 *****************************************************************************/
typedef struct _XEVE_CDSC_EXT XEVE_CDSC_EXT;

typedef struct _XEVE_CDSC
{
    int            max_bs_buf_size;
    XEVE_PARAM     param;
} XEVE_CDSC;

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

typedef void  * XEVE; /* XEVE instance identifier */

XEVE XEVE_EXPORT xeve_create(XEVE_CDSC * cdsc, int * err);
void XEVE_EXPORT xeve_delete(XEVE id);
int  XEVE_EXPORT xeve_push(XEVE id, XEVE_IMGB * imgb);
int  XEVE_EXPORT xeve_encode(XEVE id, XEVE_BITB * bitb, XEVE_STAT * stat);
int  XEVE_EXPORT xeve_config(XEVE id, int cfg, void * buf, int * size);
int  XEVE_EXPORT xeve_param_default(XEVE_PARAM* param);
int  XEVE_EXPORT xeve_param_ppt(XEVE_PARAM* param, int profile, int preset, int tune);
int  XEVE_EXPORT xeve_check_conf(XEVE_CDSC* cdsc);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* _XEVE_H_ */


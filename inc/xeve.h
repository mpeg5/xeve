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

/* return value checking *****************************************************/
#define XEVE_SUCCEEDED(ret)              ((ret) >= XEVE_OK)
#define XEVE_FAILED(ret)                 ((ret) < XEVE_OK)

/*****************************************************************************
 * color spaces
 *****************************************************************************/
#define XEVE_COLORSPACE_UNKNOWN          0 /* unknown color space */

/* YUV planar ****************************************************************/
#define XEVE_COLORSPACE_YUV_PLANAR_START 100

/* YUV planar 8bit */
#define XEVE_COLORSPACE_YUV400          300 /* Y 8bit */
#define XEVE_COLORSPACE_YUV420          301 /* YUV420 8bit */
#define XEVE_COLORSPACE_YUV422          302 /* YUV422 8bit narrow chroma*/
#define XEVE_COLORSPACE_YUV444          303 /* YUV444 8bit */
#define XEVE_COLORSPACE_YUV422N         XEVE_COLORSPACE_YUV422
#define XEVE_COLORSPACE_YUV422W         310 /* YUV422 8bit wide chroma */

#define XEVE_COLORSPACE_YUV400A8        400 /* Y+alpha 8bit */
#define XEVE_COLORSPACE_YUV420A8        401 /* YUV420+alpha 8bit */
#define XEVE_COLORSPACE_YUV422A8        402 /* YUV422+alpha 8bit narrow chroma*/
#define XEVE_COLORSPACE_YUV444A8        403 /* YUV444+alpha 8bit */
#define XEVE_COLORSPACE_YUV422NA8       XEVE_COLORSPACE_YUV422A8
#define XEVE_COLORSPACE_YUV422WA8       414 /* YUV422+alpha 8bit wide chroma*/

/* YUV planar 10bit */
#define XEVE_COLORSPACE_YUV400_10LE     500 /* Y 10bit little-endian */
#define XEVE_COLORSPACE_YUV400_10BE     501 /* Y 10bit big-endian */
#define XEVE_COLORSPACE_YUV420_10LE     502 /* YUV420 10bit little-endian */
#define XEVE_COLORSPACE_YUV420_10BE     503 /* YUV420 10bit big-endian */
#define XEVE_COLORSPACE_YUV422_10LE     504 /* YUV422 10bit little-endian */
#define XEVE_COLORSPACE_YUV422_10BE     505 /* YUV422 10bit big-endian */
#define XEVE_COLORSPACE_YUV444_10LE     506 /* YUV444 10bit little-endian */
#define XEVE_COLORSPACE_YUV444_10BE     507 /* YUV444 10bit big-endian */
#define XEVE_COLORSPACE_YUV400_12LE     600 /* Y 10bit little-endian */
#define XEVE_COLORSPACE_YUV420_12LE     602 /* YUV420 12bit little-endian */
#define XEVE_COLORSPACE_YUV420_12BE     603 /* YUV420 12bit big-endian */
#define XEVE_COLORSPACE_YUV400_14LE     700 /* Y 10bit little-endian */
#define XEVE_COLORSPACE_YUV420_14LE     702 /* YUV420 14bit little-endian */
#define XEVE_COLORSPACE_YUV420_14BE     703 /* YUV420 14bit big-endian */
#define XEVE_COLORSPACE_YUV400_16LE     800 /* Y 10bit little-endian */
#define XEVE_COLORSPACE_YUV420_16LE     802 /* YUV420 16bit little-endian */
#define XEVE_COLORSPACE_YUV420_16BE     803 /* YUV420 16bit big-endian */
#define XEVE_COLORSPACE_YUV_PLANAR_END  999

/* RGB pack ******************************************************************/
#define XEVE_COLORSPACE_RGB_PACK_START  2000

/* RGB pack 8bit */
#define XEVE_COLORSPACE_RGB888          2200
#define XEVE_COLORSPACE_BGR888          2201

#define XEVE_COLORSPACE_RGBA8888        2220
#define XEVE_COLORSPACE_BGRA8888        2221
#define XEVE_COLORSPACE_ARGB8888        2222
#define XEVE_COLORSPACE_ABGR8888        2223

#define XEVE_COLORSPACE_RGB_PACK_END    2999

/* macro for colorspace checking *********************************************/
#define XEVE_COLORSPACE_IS_YUV_PLANAR(cs)   \
    ((cs)>=XEVE_COLORSPACE_YUV_PLANAR_START && (cs)<=XEVE_COLORSPACE_YUV_PLANAR_END)

#define XEVE_COLORSPACE_IS_RGB_PACK(cs)   \
    ((cs)>=XEVE_COLORSPACE_RGB_PACK_START &&  (cs)<=XEVE_COLORSPACE_RGB_PACK_END)

#define BD_FROM_CS(cs)    \
    ((cs)<XEVE_COLORSPACE_YUV400_10LE) ? 8 : ((cs)<XEVE_COLORSPACE_YUV400_12LE ? 10 : ((cs)<XEVE_COLORSPACE_YUV400_14LE ? 12 : 14))
#define CS_FROM_BD_420(bd)    \
    ((bd)==8) ? XEVE_COLORSPACE_YUV420 : ((bd)==10 ? XEVE_COLORSPACE_YUV420_10LE : ((bd)==12 ? XEVE_COLORSPACE_YUV420_12LE : XEVE_COLORSPACE_YUV420_14LE))

/*****************************************************************************
 * config types
 *****************************************************************************/
#define XEVE_CFG_SET_COMPLEXITY         (100)
#define XEVE_CFG_SET_SPEED              (101)
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
 * type and macro for media time
 *****************************************************************************/
/* media time in 100-nanosec unit */
typedef long long                    XEVE_MTIME;

/*****************************************************************************
 * image buffer format
 *****************************************************************************
 baddr
    +---------------------------------------------------+ ---
    |                                                   |  ^
    |                                              |    |  |
    |    a                                         v    |  |
    |   --- +-----------------------------------+ ---   |  |
    |    ^  |  (x, y)                           |  y    |  |
    |    |  |   +---------------------------+   + ---   |  |
    |    |  |   |                           |   |  ^    |  |
    |    |  |   |                           |   |  |    |  |
    |    |  |   |                           |   |  |    |  |
    |    |  |   |                           |   |  |    |  |
    |       |   |                           |   |       |
    |    ah |   |                           |   |  h    |  e
    |       |   |                           |   |       |
    |    |  |   |                           |   |  |    |  |
    |    |  |   |                           |   |  |    |  |
    |    |  |   |                           |   |  v    |  |
    |    |  |   +---------------------------+   | ---   |  |
    |    v  |                                   |       |  |
    |   --- +---+-------------------------------+       |  |
    |     ->| x |<----------- w ----------->|           |  |
    |       |<--------------- aw -------------->|       |  |
    |                                                   |  v
    +---------------------------------------------------+ ---

    |<---------------------- s ------------------------>|

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

typedef struct _XEVED_OPL
{
    int              poc;
    char             digest[3][16];
} XEVED_OPL;

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
    int            cb_qp_offset;
    int            cr_qp_offset;
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
    /* bit depth of input video */
    int            in_bit_depth;
    /* bit depth of output video */
    int            out_bit_depth;
    int            codec_bit_depth;
    int            profile;
    int            level;
    int            use_dqp;
    int            constrained_intra_pred;
    int            use_deblock;
    int            parallel_task_cnt;
    int            inter_slice_type;
    int            picture_cropping_flag;
    int            picture_crop_left_offset;
    int            picture_crop_right_offset;
    int            picture_crop_top_offset;
    int            picture_crop_bottom_offset;
    int            rdo_dbk_switch;
    int            add_qp_frame;
    int            bitstream_buf_size;

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

XEVE xeve_create(XEVE_CDSC * cdsc, int * err);
void xeve_delete(XEVE id);
int  xeve_push(XEVE id, XEVE_IMGB * imgb);
int  xeve_encode(XEVE id, XEVE_BITB * bitb, XEVE_STAT * stat);
int  xeve_encode_sps(XEVE id, XEVE_BITB * bitb, XEVE_STAT * stat);
int  xeve_encode_pps(XEVE id, XEVE_BITB * bitb, XEVE_STAT * stat);
int  xeve_encode_aps(XEVE id, XEVE_BITB * bitb, XEVE_STAT * stat, int aps_type_id);
int  xeve_config(XEVE id, int cfg, void * buf, int * size);

#endif /* _XEVE_H_ */

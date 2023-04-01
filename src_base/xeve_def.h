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

#ifndef _XEVE_DEF_H_
#define _XEVE_DEF_H_

#include "xeve.h"
#include "xeve_port.h"

/* Profiles definitions */
#define PROFILE_IDC_BASELINE                         0
#define PROFILE_IDC_MAIN                             1

//fast algorithm
#define FAST_ALG_EXT                                 0
#if FAST_ALG_EXT
#define MODE_SAVE_LOAD_UPDATE                        1 // improve mode save load
#define ET_ME_REFIDX1                                1 // skip ME of one ref pic based on mvd of ref pic 0
#define ET_AMVP                                      1 // skip AMVP based on skip/merge cost
#define ET_BY_RDC_CHILD_SPLIT                        0 // early termination of split based on RD cost & child split (10% EncT)
#endif

#define GET_QP(qp,dqp)                             ((qp + dqp + 52) % 52)
#define GET_LUMA_QP(qp, qp_bd_offset)               (qp + 6 * qp_bd_offset)


//fast algorithm
#define ENC_ECU_DEPTH                                7 // for early CU termination
#define ENC_ECU_ADAPTIVE                             1 // for early CU termination
#define ENC_ECU_DEPTH_B                              4 // for early CU termination
#define MULTI_REF_ME_STEP                            1 // for ME speed-up
#define FAST_MERGE_THR                               1.3
#define ENC_SUCO_FAST_CONFIG                         1  /* fast config: 1(low complexity), 2(medium complexity), 4(high_complexity) */

////////////////////////////////////////////////////////////////////////////////
//                                                                            //
//                         Certain Tools Parameters                           //
//                                                                            //
////////////////////////////////////////////////////////////////////////////////
#define MAX_NUM_PPS                        64
/* Partitioning (START) */
#define INC_QT_DEPTH(qtd, smode)           (smode == SPLIT_QUAD? (qtd + 1) : qtd)
#define INC_BTT_DEPTH(bttd, smode, bound)  (bound? 0: (smode != SPLIT_QUAD? (bttd + 1) : bttd))
#define MAX_SPLIT_NUM                      6
#define SPLIT_CHECK_NUM                    6
/* Partitioning (END) */

/* CABAC (START) */
#define PROB_INIT                         (512) /* 1/2 of initialization with mps = 0 */
/* CABAC (END) */

/* Multiple Referene (START) */
#define MAX_NUM_ACTIVE_REF_FRAME_B         2  /* Maximum number of active reference frames for RA condition */
#define MAX_NUM_ACTIVE_REF_FRAME_LDB       4  /* Maximum number of active reference frames for LDB condition */
#define MVP_SCALING_PRECISION              5  /* Scaling precision for motion vector prediction (2^MVP_SCALING_PRECISION) */
/* Multiple Reference (END) */

/* MMVD (START) */
#define MMVD_BASE_MV_NUM                   4
#define MMVD_DIST_NUM                      8
#define MMVD_MAX_REFINE_NUM               (MMVD_DIST_NUM * 4)
#define MMVD_SKIP_CON_NUM                  4
#define MMVD_GRP_NUM                       3
#define MMVD_THRESHOLD                     1.5
/* MMVD (END) */

/* AMVR (START) */
#define MAX_NUM_MVR                        5
#define FAST_MVR_IDX                       2
#define SKIP_MVR_IDX                       1
#define MAX_NUM_BI                         3
/* AMVR (END)  */

/* DBF (START) */
#define DBF_LENGTH                         4
#define DBF_LENGTH_CHROMA                  2
#define DBF_ADDB_BS_INTRA_STRONG           4
#define DBF_ADDB_BS_INTRA                  3
#define DBF_ADDB_BS_CODED                  2
#define DBF_ADDB_BS_DIFF_REFS              1
#define DBF_ADDB_BS_OTHERS                 0
/* DBF (END) */

/* DMVR (START) */
#define DMVR_SUBCU_SIZE                    16
#define DMVR_ITER_COUNT                    2
#define REF_PRED_POINTS_NUM                9
#define REF_PRED_EXTENTION_PEL_COUNT       1
#define REF_PRED_POINTS_PER_LINE_NUM       3
#define REF_PRED_POINTS_LINES_NUM          3
#define DMVR_NEW_VERSION_ITER_COUNT        8
#define REF_PRED_POINTS_CROSS              5

enum SAD_POINT_INDEX
{
    SAD_NOT_AVAILABLE = -1,
    SAD_BOTTOM = 0,
    SAD_TOP,
    SAD_RIGHT,
    SAD_LEFT,
    SAD_TOP_LEFT,
    SAD_TOP_RIGHT,
    SAD_BOTTOM_LEFT,
    SAD_BOTTOM_RIGHT,
    SAD_CENTER,
    SAD_COUNT
};
/* DMVR (END) */

/* HISTORY (START) */
#define ALLOWED_CHECKED_NUM                23
#define ALLOWED_CHECKED_NUM_SMALL_CU       15
#define ALLOWED_CHECKED_AMVP_NUM           4
/* HISTORY (END) */

/* ALF (START) */
#define MAX_NUM_TLAYER                     6
#define MAX_NUM_ALFS_PER_TLAYER            6
#define ALF_LAMBDA_SCALE                   17
#define MAX_NUM_ALF_CLASSES                25
#define MAX_NUM_ALF_LUMA_COEFF             13
#define MAX_NUM_ALF_CHROMA_COEFF           7
#define MAX_ALF_FILTER_LENGTH              7
#define MAX_NUM_ALF_COEFF                 (MAX_ALF_FILTER_LENGTH * MAX_ALF_FILTER_LENGTH / 2 + 1)
/* ALF (END) */

/* AFFINE (START) */
 // AFFINE Constant
#define VER_NUM                            4
#define AFFINE_MAX_NUM_LT                  3 ///< max number of motion candidates in top-left corner
#define AFFINE_MAX_NUM_RT                  3 ///< max number of motion candidates in top-right corner
#define AFFINE_MAX_NUM_LB                  2 ///< max number of motion candidates in left-bottom corner
#define AFFINE_MAX_NUM_RB                  2 ///< max number of motion candidates in right-bottom corner
#define AFFINE_MIN_BLOCK_SIZE              4 ///< Minimum affine MC block size
#define AFF_MAX_NUM_MVP                    2 // maximum affine inter candidates
#define AFF_MAX_CAND                       5 // maximum affine merge candidates
#define AFF_MODEL_CAND                     5 // maximum affine model based candidates

// AFFINE ME configuration (non-normative)
#define AF_ITER_UNI                        7 // uni search iteration time
#define AF_ITER_BI                         5 // bi search iteration time
#define AFFINE_BI_ITER                     1

/* EIF (START) */
#define AFFINE_ADAPT_EIF_SIZE                                   8
#define EIF_SUBBLOCK_SIZE                                       4
#define EIF_NUM_ALLOWED_FETCHED_LINES_FOR_THE_FIRST_LINE        3
#define EIF_MV_PRECISION_BILINEAR                               5
#define BOUNDING_BLOCK_MARGIN                                   7
#define MEMORY_BANDWIDTH_THRESHOLD                              (8 + 2 + BOUNDING_BLOCK_MARGIN) / 8
#define MAX_MEMORY_ACCESS_BI                                    72
/* EIF (END) */

/* AFFINE (END) */

/* ALF (START) */
#define MAX_SCAN_VAL                       11
#define MAX_EXP_GOLOMB                     16
#define MAX_NUM_ALF_LUMA_COEFF             13
#define MAX_NUM_ALF_CLASSES                25
#define MAX_NUM_ALF_LUMA_COEFF             13
#define MAX_NUM_ALF_CHROMA_COEFF           7
#define MAX_ALF_FILTER_LENGTH              7
#define MAX_NUM_ALF_COEFF                 (MAX_ALF_FILTER_LENGTH * MAX_ALF_FILTER_LENGTH / 2 + 1)

#define APS_MAX_NUM                        32
#define APS_MAX_NUM_IN_BITS                5
#define APS_TYPE_ID_BITS                   3
/* ALF (END) */

/* TRANSFORM PACKAGE (START) */
#define ATS_INTRA_FAST                     1
#define ATS_INTER_INTRA_SKIP_THR           1.05
#define ATS_INTRA_Y_NZZ_THR                1
#define ATS_INTRA_IPD_THR                  1.10

#define ATS_INTER_SL_NUM                   16
#define get_ats_inter_idx(s)               (s & 0xf)
#define get_ats_inter_pos(s)               ((s>>4) & 0xf)
#define get_ats_inter_info(idx, pos)       (idx + (pos << 4))
#define is_ats_inter_horizontal(idx)       (idx == 2 || idx == 4)
#define is_ats_inter_quad_size(idx)        (idx == 3 || idx == 4)
/* TRANSFORM PACKAGE (END) */

/* ADCC (START) */
#define LOG2_RATIO_GTA                     1
#define LOG2_RATIO_GTB                     4
#define LOG2_CG_SIZE                       4
#define MLS_GRP_NUM                        1024
#define CAFLAG_NUMBER                      8
#define CBFLAG_NUMBER                      1

#define SBH_THRESHOLD                      4
#define MAX_GR_ORDER_RESIDUAL              10
#define COEF_REMAIN_BIN_REDUCTION          3
#define LAST_SIGNIFICANT_GROUPS            14

#define NUM_CTX_LAST_SIG_COEFF_LUMA        18
#define NUM_CTX_LAST_SIG_COEFF_CHROMA      3
#define NUM_CTX_LAST_SIG_COEFF             (NUM_CTX_LAST_SIG_COEFF_LUMA + NUM_CTX_LAST_SIG_COEFF_CHROMA)

#define NUM_CTX_SIG_COEFF_LUMA             39  /* number of context models for luma sig coeff flag */
#define NUM_CTX_SIG_COEFF_CHROMA           8   /* number of context models for chroma sig coeff flag */
#define NUM_CTX_SIG_COEFF_LUMA_TU          13  /* number of context models for luma sig coeff flag per TU */
#define NUM_CTX_SIG_COEFF_FLAG             (NUM_CTX_SIG_COEFF_LUMA + NUM_CTX_SIG_COEFF_CHROMA)  /* number of context models for sig coeff flag */
#define NUM_CTX_GTX_LUMA                   13
#define NUM_CTX_GTX_CHROMA                 5
#define NUM_CTX_GTX                        (NUM_CTX_GTX_LUMA + NUM_CTX_GTX_CHROMA)  /* number of context models for gtA/B flag */

#define COEF_SCAN_ZIGZAG                   0
#define COEF_SCAN_DIAG                     1
#define COEF_SCAN_DIAG_CG                  2
#define COEF_SCAN_TYPE_NUM                 3
/* ADCC (END) */

/* IBC (START) */
#define IBC_SEARCH_RANGE                     64
#define IBC_NUM_CANDIDATES                   64
#define IBC_FAST_METHOD_BUFFERBV             0X01
#define IBC_FAST_METHOD_ADAPTIVE_SEARCHRANGE 0X02
/* IBC (END) */

/* CABAC ZERO WORD (START) */
#define CABAC_ZERO_PARAM                   32
/* CABAC ZERO WORD (END) */

/* COMMON (START) */typedef int BOOL;
#define TRUE                               1
#define FALSE                              0
/* COMMON (END) */

/* For debugging (START) */
#define USE_DRAW_PARTITION_DEC             0
#define ENC_DEC_TRACE                      0
#ifndef GRAB_STAT
#define GRAB_STAT                          0
#endif
#if ENC_DEC_TRACE
#define TRACE_ENC_CU_DATA                  0 ///< Trace CU index on encoder
#define TRACE_ENC_CU_DATA_CHECK            0 ///< Trace CU index on encoder
#define MVF_TRACE                          0 ///< use for tracing MVF
#define TRACE_ENC_HISTORIC                 0
#define TRACE_COEFFS                       0 ///< Trace coefficients
#define TRACE_RDO                          0 //!< Trace only encode stream (0), only RDO (1) or all of them (2)
#define TRACE_BIN                          0 //!< trace each bin
#define TRACE_START_POC                    0 //!< POC of frame from which we start to write output tracing information
#define TRACE_COSTS                        0 //!< Trace cost information
#define TRACE_REMOVE_COUNTER               0 //!< Remove trace counter
#define TRACE_ADDITIONAL_FLAGS             0
#define TRACE_DBF                          0 //!< Trace only DBF
#define TRACE_HLS                          0 //!< Trace SPS, PPS, APS, Slice Header, etc.
#if TRACE_RDO
#define TRACE_RDO_EXCLUDE_I                0 //!< Exclude I frames
#endif
extern FILE *fp_trace;
extern int fp_trace_print;
extern int fp_trace_counter;
#if TRACE_START_POC
extern int fp_trace_started;
#endif
#if TRACE_RDO == 1
#define XEVE_TRACE_SET(A) fp_trace_print=!A
#elif TRACE_RDO == 2
#define XEVE_TRACE_SET(A)
#else
#define XEVE_TRACE_SET(A) fp_trace_print=A
#endif
#define XEVE_TRACE_STR(STR) if(fp_trace_print) { fprintf(fp_trace, STR); fflush(fp_trace); }
#define XEVE_TRACE_DOUBLE(DOU) if(fp_trace_print) { fprintf(fp_trace, "%g", DOU); fflush(fp_trace); }
#define XEVE_TRACE_INT(INT) if(fp_trace_print) { fprintf(fp_trace, "%d ", INT); fflush(fp_trace); }
#define XEVE_TRACE_INT_HEX(INT) if(fp_trace_print) { fprintf(fp_trace, "0x%x ", INT); fflush(fp_trace); }
#if TRACE_REMOVE_COUNTER
#define XEVE_TRACE_COUNTER
#else
#define XEVE_TRACE_COUNTER  XEVE_TRACE_INT(fp_trace_counter++); XEVE_TRACE_STR("\t")
#endif
#define XEVE_TRACE_MV(X, Y) if(fp_trace_print) { fprintf(fp_trace, "(%d, %d) ", X, Y); fflush(fp_trace); }
#define XEVE_TRACE_FLUSH    if(fp_trace_print) fflush(fp_trace)
#else
#define XEVE_TRACE_SET(A)
#define XEVE_TRACE_STR(str)
#define XEVE_TRACE_DOUBLE(DOU)
#define XEVE_TRACE_INT(INT)
#define XEVE_TRACE_INT_HEX(INT)
#define XEVE_TRACE_COUNTER
#define XEVE_TRACE_MV(X, Y)
#define XEVE_TRACE_FLUSH
#endif
/* For debugging (END) */
/*************Optimization************/
#define OPT_MC_BI_PAD            32
#define PRED_BI_SIZE           ((MAX_CU_SIZE + OPT_MC_BI_PAD * 2) * (MAX_CU_SIZE + OPT_MC_BI_PAD * 2))
#define PRED_MAX_I_PERIOD       100
#define PRED_MAX_REF_FRAMES     4

/********* Conditional tools definition ********/

/* number of picture order count lsb bit */
#define POC_LSB_BIT                        (8)
#define PEL2BYTE(pel,cs)                  ((pel)*(((XEVE_CS_GET_BIT_DEPTH(cs)) + 7)>>3))
#define STRIDE_IMGB2PIC(s_imgb)           ((s_imgb)>>1)

#define Y_C                                0  /* Y luma */
#define U_C                                1  /* Cb Chroma */
#define V_C                                2  /* Cr Chroma */
#define N_C                                3  /* number of color component */

#define LUMA_CH                            0
#define CHROMA_CH                          1
#define NUM_CH                             2

#define REFP_0                             0
#define REFP_1                             1
#define REFP_NUM                           2

/* X direction motion vector indicator */
#define MV_X                               0
/* Y direction motion vector indicator */
#define MV_Y                               1
/* Maximum count (dimension) of motion */
#define MV_D                               2
/* Reference index indicator */
#define REFI                               2

#define N_REF                              3  /* left, up, right */
#define NUM_NEIB                           4  /* LR: 00, 10, 01, 11*/

#define MAX_CU_LOG2                        7
#define MIN_CU_LOG2                        2
#define MAX_CU_SIZE                       (1 << MAX_CU_LOG2)
#define MIN_CU_SIZE                       (1 << MIN_CU_LOG2)
#define MAX_CU_DIM                        (MAX_CU_SIZE * MAX_CU_SIZE)
#define MIN_CU_DIM                        (MIN_CU_SIZE * MIN_CU_SIZE)
#define MAX_CU_DEPTH                       10  /* 128x128 ~ 4x4 */
#define NUM_CU_DEPTH                      (MAX_CU_DEPTH + 1)
#define NUM_CU_LOG2                       (MAX_CU_LOG2 - MIN_CU_LOG2 + 1)

#define MAX_TR_LOG2                        6  /* 64x64 */
#define MIN_TR_LOG2                        1  /* 2x2 */
#define MAX_TR_SIZE                       (1 << MAX_TR_LOG2)
#define MIN_TR_SIZE                       (1 << MIN_TR_LOG2)
#define MAX_TR_DIM                        (MAX_TR_SIZE * MAX_TR_SIZE)
#define MIN_TR_DIM                        (MIN_TR_SIZE * MIN_TR_SIZE)

#define MAX_BEF_DATA_NUM                  (1)

/* maximum CB count in a LCB */
#define MAX_CU_CNT_IN_LCU                  (MAX_CU_DIM/MIN_CU_DIM)
/* pixel position to SCB position */
#define PEL2SCU(pel)                       ((pel) >> MIN_CU_LOG2)

#define PIC_PAD_SIZE_L                     (MAX_CU_SIZE + 16)
#define PIC_PAD_SIZE_C                     (PIC_PAD_SIZE_L >> 1)

/* number of MVP candidates */
#define MAX_NUM_MVP_SMALL_CU               4
#define MAX_NUM_MVP                        6
#define NUM_SAMPLES_BLOCK                  32 // 16..64
#define ORG_MAX_NUM_MVP                    4
#define MAX_NUM_POSSIBLE_SCAND             13

/* for GOP 16 test, increase to 32 */
/* maximum reference picture count. Originally, Max. 16 */
/* for GOP 16 test, increase to 32 */

/* DPB Extra size */
#define EXTRA_FRAME                        XEVE_MAX_NUM_ACTIVE_REF_FRAME

/* maximum picture buffer size */
#define DRA_FRAME 1
#define MAX_PB_SIZE                       (XEVE_MAX_NUM_REF_PICS + EXTRA_FRAME + DRA_FRAME)

/* Neighboring block availability flag bits */
#define AVAIL_BIT_UP                       0
#define AVAIL_BIT_LE                       1
#define AVAIL_BIT_RI                       3
#define AVAIL_BIT_LO                       4
#define AVAIL_BIT_UP_LE                    5
#define AVAIL_BIT_UP_RI                    6
#define AVAIL_BIT_LO_LE                    7
#define AVAIL_BIT_LO_RI                    8
#define AVAIL_BIT_RI_UP                    9
#define AVAIL_BIT_UP_LE_LE                 10
#define AVAIL_BIT_UP_RI_RI                 11

/* Neighboring block availability flags */
#define AVAIL_UP                          (1 << AVAIL_BIT_UP)
#define AVAIL_LE                          (1 << AVAIL_BIT_LE)
#define AVAIL_RI                          (1 << AVAIL_BIT_RI)
#define AVAIL_LO                          (1 << AVAIL_BIT_LO)
#define AVAIL_UP_LE                       (1 << AVAIL_BIT_UP_LE)
#define AVAIL_UP_RI                       (1 << AVAIL_BIT_UP_RI)
#define AVAIL_LO_LE                       (1 << AVAIL_BIT_LO_LE)
#define AVAIL_LO_RI                       (1 << AVAIL_BIT_LO_RI)
#define AVAIL_RI_UP                       (1 << AVAIL_BIT_RI_UP)
#define AVAIL_UP_LE_LE                    (1 << AVAIL_BIT_UP_LE_LE)
#define AVAIL_UP_RI_RI                    (1 << AVAIL_BIT_UP_RI_RI)

/* MB availability check macro */
#define IS_AVAIL(avail, pos)            (((avail)&(pos)) == (pos))
/* MB availability set macro */
#define SET_AVAIL(avail, pos)             (avail) |= (pos)
/* MB availability remove macro */
#define REM_AVAIL(avail, pos)             (avail) &= (~(pos))
/* MB availability into bit flag */
#define GET_AVAIL_FLAG(avail, bit)      (((avail)>>(bit)) & 0x1)

/*****************************************************************************
 * slice type
 *****************************************************************************/
#define SLICE_I                            XEVE_ST_I
#define SLICE_P                            XEVE_ST_P
#define SLICE_B                            XEVE_ST_B

#define IS_INTRA_SLICE(slice_type)       ((slice_type) == SLICE_I))
#define IS_INTER_SLICE(slice_type)      (((slice_type) == SLICE_P) || ((slice_type) == SLICE_B))

/*****************************************************************************
 * prediction mode
 *****************************************************************************/
#define MODE_INTRA                         0
#define MODE_INTER                         1
#define MODE_SKIP                          2
#define MODE_DIR                           3
#define MODE_SKIP_MMVD                     4
#define MODE_DIR_MMVD                      5
#define MODE_IBC                           6

 /*****************************************************************************
 * prediction direction
 *****************************************************************************/
/* inter pred direction, look list0 side */
#define PRED_L0                            0
/* inter pred direction, look list1 side */
#define PRED_L1                            1
/* inter pred direction, look both list0, list1 side */
#define PRED_BI                            2
/* inter pred direction, look both list0, list1 side */
#define PRED_SKIP                          3
/* inter pred direction, look both list0, list1 side */
#define PRED_DIR                           4

#define PRED_SKIP_MMVD                     5
#define PRED_DIR_MMVD                      6
/* IBC pred direction, look current picture as reference */
#define PRED_IBC                           7
#define PRED_FL0_BI                        10
#define PRED_FL1_BI                        11
#define PRED_BI_REF                        12
#define ORG_PRED_NUM                       13
#define PRED_NUM                          (ORG_PRED_NUM * MAX_NUM_MVR)

#define START_NUM                         (ORG_PRED_NUM * MAX_NUM_MVR)

#define AFF_L0                            (START_NUM)          // 5  7  42
#define AFF_L1                            (START_NUM + 1)      // 6  8  43
#define AFF_BI                            (START_NUM + 2)      // 7  9  44
#define AFF_SKIP                          (START_NUM + 3)      // 8  10 45
#define AFF_DIR                           (START_NUM + 4)      // 9  11 46

#define AFF_6_L0                          (START_NUM + 5)      // 10 12 47
#define AFF_6_L1                          (START_NUM + 6)      // 11 13 48
#define AFF_6_BI                          (START_NUM + 7)      // 12 14 49

#undef PRED_NUM
#define PRED_NUM                          (START_NUM + 8)

#define LR_00                              0
#define LR_10                              1
#define LR_01                              2
#define LR_11                              3

/*****************************************************************************
 * bi-prediction type
 *****************************************************************************/
#define BI_NON                             0
#define BI_NORMAL                          1
#define BI_FL0                             2
#define BI_FL1                             3

/*****************************************************************************
 * intra prediction direction
 *****************************************************************************/
#define IPD_DC                             0
#define IPD_PLN                            1  /* Luma, Planar */
#define IPD_BI                             2  /* Luma, Bilinear */
#define IPD_HOR                            24 /* Luma, Horizontal */
#define IPD_VER                            12 /* Luma, Vertical */

#define IPD_DM_C                           0  /* Chroma, DM */
#define IPD_BI_C                           1  /* Chroma, Bilinear */
#define IPD_DC_C                           2  /* Chroma, DC */
#define IPD_HOR_C                          3  /* Chroma, Horizontal*/
#define IPD_VER_C                          4  /* Chroma, Vertical */
#define IPD_RDO_CNT                        5

#define IPD_DC_B                           0
#define IPD_HOR_B                          1 /* Luma, Horizontal */
#define IPD_VER_B                          2 /* Luma, Vertical */
#define IPD_UL_B                           3
#define IPD_UR_B                           4

#define IPD_DC_C_B                         0  /* Chroma, DC */
#define IPD_HOR_C_B                        1  /* Chroma, Horizontal*/
#define IPD_VER_C_B                        2  /* Chroma, Vertical */
#define IPD_UL_C_B                         3
#define IPD_UR_C_B                         4

#define IPD_CNT_B                          5
#define IPD_CNT                            33

#define IPD_CHROMA_CNT                     5
#define IPD_INVALID                       (-1)

#define IPD_DIA_R                          18 /* Luma, Right diagonal */ /* (IPD_VER + IPD_HOR) >> 1 */
#define IPD_DIA_L                          6  /* Luma, Left diagonal */
#define IPD_DIA_U                          30 /* Luma, up diagonal */

#define INTRA_MPM_NUM                      2
#define INTRA_PIMS_NUM                     8

#define IBC_MAX_CU_LOG2                    6 /* max block size for ibc search in unit of log2 */

/*****************************************************************************
* Transform
*****************************************************************************/
typedef enum _TRANS_TYPE
{
    DCT8, DST7, NUM_TRANS_TYPE,
} TRANS_TYPE;

#define PI                                (3.14159265358979323846)

/*****************************************************************************
 * reference index
 *****************************************************************************/
#define REFI_INVALID                      (-1)
#define REFI_IS_VALID(refi)               ((refi) >= 0)
#define SET_REFI(refi, idx0, idx1)        (refi)[REFP_0] = (idx0); (refi)[REFP_1] = (idx1)

 /*****************************************************************************
 * macros for CU map

 - [ 0: 6] : slice number (0 ~ 128)
 - [ 7:14] : reserved
 - [15:15] : 1 -> intra CU, 0 -> inter CU
 - [16:22] : QP
 - [23:23] : skip mode flag
 - [24:24] : luma cbf
 - [25:25] : dmvr_flag
 - [26:26] : IBC mode flag
 - [27:30] : reserved
 - [31:31] : 0 -> no encoded/decoded CU, 1 -> encoded/decoded CU
 *****************************************************************************/
/* set slice number to map */
#define MCU_SET_SN(m, sn)       (m)=(((m) & 0xFFFFFF80)|((sn) & 0x7F))
/* get slice number from map */
#define MCU_GET_SN(m)           (int)((m) & 0x7F)

/* set intra CU flag to map */
#define MCU_SET_IF(m)           (m)=((m)|(1<<15))
/* get intra CU flag from map */
#define MCU_GET_IF(m)           (int)(((m)>>15) & 1)
/* clear intra CU flag in map */
#define MCU_CLR_IF(m)           (m)=((m) & 0xFFFF7FFF)

/* set QP to map */
#define MCU_SET_QP(m, qp)       (m)=((m)|((qp)&0x7F)<<16)
/* get QP from map */
#define MCU_GET_QP(m)           (int)(((m)>>16)&0x7F)
/* clear QP from map */
#define MCU_CLR_QP(m)         (m)=((m) & (~((127)<<16)))

/* set skip mode flag */
#define MCU_SET_SF(m)           (m)=((m)|(1<<23))
/* get skip mode flag */
#define MCU_GET_SF(m)           (int)(((m)>>23) & 1)
/* clear skip mode flag */
#define MCU_CLR_SF(m)           (m)=((m) & (~(1<<23)))

/* set luma cbf flag */
#define MCU_SET_CBFL(m)         (m)=((m)|(1<<24))
/* get luma cbf flag */
#define MCU_GET_CBFL(m)         (int)(((m)>>24) & 1)
/* clear luma cbf flag */
#define MCU_CLR_CBFL(m)         (m)=((m) & (~(1<<24)))

/* set dmvr flag */
#define MCU_SET_DMVRF(m)         (m)=((m)|(1<<25))
/* get dmvr flag */
#define MCU_GET_DMVRF(m)         (int)(((m)>>25) & 1)
/* clear dmvr flag */
#define MCU_CLR_DMVRF(m)         (m)=((m) & (~(1<<25)))

/* set ibc mode flag */
#define MCU_SET_IBC(m)          (m)=((m)|(1<<26))
/* get ibc mode flag */
#define MCU_GET_IBC(m)          (int)(((m)>>26) & 1)
/* clear ibc mode flag */
#define MCU_CLR_IBC(m)          (m)=((m) & (~(1<<26)))

/* set encoded/decoded CU to map */
#define MCU_SET_COD(m)          (m)=((m)|(1<<31))
/* get encoded/decoded CU flag from map */
#define MCU_GET_COD(m)          (int)(((m)>>31) & 1)
/* clear encoded/decoded CU flag to map */
#define MCU_CLR_COD(m)          (m)=((m) & 0x7FFFFFFF)

/* multi bit setting: intra flag, encoded/decoded flag, slice number */
#define MCU_SET_IF_COD_SN_QP(m, i, sn, qp) \
    (m) = (((m)&0xFF807F80)|((sn)&0x7F)|((qp)<<16)|((i)<<15)|(1<<31))

#define MCU_IS_COD_NIF(m)      ((((m)>>15) & 0x10001) == 0x10000)
/*
- [8:9] : affine vertex number, 00: 1(trans); 01: 2(affine); 10: 3(affine); 11: 4(affine)
*/

/* set affine CU mode to map */
#define MCU_SET_AFF(m, v)       (m)=((m & 0xFFFFFCFF)|((v)&0x03)<<8)
/* get affine CU mode from map */
#define MCU_GET_AFF(m)          (int)(((m)>>8)&0x03)
/* clear affine CU mode to map */
#define MCU_CLR_AFF(m)          (m)=((m) & 0xFFFFFCFF)

/*****************************************************************************
* macros for affine CU map

- [ 0: 7] : log2 cu width
- [ 8:15] : log2 cu height
- [16:23] : x offset
- [24:31] : y offset
*****************************************************************************/
#define MCU_SET_AFF_LOGW(m, v)       (m)=((m & 0xFFFFFF00)|((v)&0xFF)<<0)
#define MCU_SET_AFF_LOGH(m, v)       (m)=((m & 0xFFFF00FF)|((v)&0xFF)<<8)
#define MCU_SET_AFF_XOFF(m, v)       (m)=((m & 0xFF00FFFF)|((v)&0xFF)<<16)
#define MCU_SET_AFF_YOFF(m, v)       (m)=((m & 0x00FFFFFF)|((v)&0xFF)<<24)

#define MCU_GET_AFF_LOGW(m)          (int)(((m)>>0)&0xFF)
#define MCU_GET_AFF_LOGH(m)          (int)(((m)>>8)&0xFF)
#define MCU_GET_AFF_XOFF(m)          (int)(((m)>>16)&0xFF)
#define MCU_GET_AFF_YOFF(m)          (int)(((m)>>24)&0xFF)

/* set MMVD skip flag to map */
#define MCU_SET_MMVDS(m)            (m)=((m)|(1<<2))
/* get MMVD skip flag from map */
#define MCU_GET_MMVDS(m)            (int)(((m)>>2) & 1)
/* clear MMVD skip flag in map */
#define MCU_CLR_MMVDS(m)            (m)=((m) & (~(1<<2)))

/* set log2_cuw & log2_cuh to map */
#define MCU_SET_LOGW(m, v)       (m)=((m & 0xF0FFFFFF)|((v)&0x0F)<<24)
#define MCU_SET_LOGH(m, v)       (m)=((m & 0x0FFFFFFF)|((v)&0x0F)<<28)
/* get log2_cuw & log2_cuh to map */
#define MCU_GET_LOGW(m)          (int)(((m)>>24)&0x0F)
#define MCU_GET_LOGH(m)          (int)(((m)>>28)&0x0F)

typedef u16 SBAC_CTX_MODEL;

#define NUM_CTX_MMVD_FLAG                  1
#define NUM_CTX_MMVD_GROUP_IDX            (MMVD_GRP_NUM - 1)
#define NUM_CTX_MMVD_MERGE_IDX            (MMVD_BASE_MV_NUM - 1)
#define NUM_CTX_MMVD_DIST_IDX             (MMVD_DIST_NUM - 1)
#define NUM_CTX_MMVD_DIRECTION_IDX         2
#define NUM_CTX_AFFINE_MVD_FLAG            2       /* number of context models for affine_mvd_flag_l0 and affine_mvd_flag_l1 (1st one is for affine_mvd_flag_l0 and 2nd one if for affine_mvd_flag_l1) */
#define NUM_CTX_SKIP_FLAG                  2
#define NUM_CTX_IBC_FLAG                   2
#define NUM_CTX_BTT_SPLIT_FLAG             15
#define NUM_CTX_BTT_SPLIT_DIR              5
#define NUM_CTX_BTT_SPLIT_TYPE             1
#define NUM_CTX_SUCO_FLAG                  14
#define NUM_CTX_CBF_LUMA                   1
#define NUM_CTX_CBF_CB                     1
#define NUM_CTX_CBF_CR                     1
#define NUM_CTX_CBF_ALL                    1
#define NUM_CTX_PRED_MODE                  3
#define NUM_CTX_MODE_CONS                  3
#define NUM_CTX_INTER_PRED_IDC             2       /* number of context models for inter prediction direction */
#define NUM_CTX_DIRECT_MODE_FLAG           1
#define NUM_CTX_MERGE_MODE_FLAG            1
#define NUM_CTX_REF_IDX                    2
#define NUM_CTX_MERGE_IDX                  5
#define NUM_CTX_MVP_IDX                    3
#define NUM_CTX_AMVR_IDX                   4
#define NUM_CTX_BI_PRED_IDX                2
#define NUM_CTX_MVD                        1       /* number of context models for motion vector difference */
#define NUM_CTX_INTRA_PRED_MODE            2
#define NUM_CTX_INTRA_LUMA_PRED_MPM_FLAG   1
#define NUM_CTX_INTRA_LUMA_PRED_MPM_IDX    1
#define NUM_CTX_INTRA_CHROMA_PRED_MODE     1
#define NUM_CTX_AFFINE_FLAG                2
#define NUM_CTX_AFFINE_MODE                1
#define NUM_CTX_AFFINE_MRG                 AFF_MAX_CAND
#define NUM_CTX_AFFINE_MVP_IDX            (AFF_MAX_NUM_MVP - 1)
#define NUM_CTX_CC_RUN                     24
#define NUM_CTX_CC_LAST                    2
#define NUM_CTX_CC_LEVEL                   24
#define NUM_CTX_ALF_CTB_FLAG               1
#define NUM_CTX_SPLIT_CU_FLAG              1
#define NUM_CTX_DELTA_QP                   1
#define NUM_CTX_ATS_INTRA_CU_FLAG          1
#define NUM_CTX_ATS_MODE_FLAG              1
#define NUM_CTX_ATS_INTER_FLAG             2
#define NUM_CTX_ATS_INTER_QUAD_FLAG        1
#define NUM_CTX_ATS_INTER_HOR_FLAG         3
#define NUM_CTX_ATS_INTER_POS_FLAG         1

/* context models for arithemetic coding */
typedef struct _XEVE_SBAC_CTX
{
    SBAC_CTX_MODEL   skip_flag                     [NUM_CTX_SKIP_FLAG];
    SBAC_CTX_MODEL   ibc_flag                      [NUM_CTX_IBC_FLAG];
    SBAC_CTX_MODEL   mmvd_flag                     [NUM_CTX_MMVD_FLAG];
    SBAC_CTX_MODEL   mmvd_merge_idx                [NUM_CTX_MMVD_MERGE_IDX];
    SBAC_CTX_MODEL   mmvd_distance_idx             [NUM_CTX_MMVD_DIST_IDX];
    SBAC_CTX_MODEL   mmvd_direction_idx            [NUM_CTX_MMVD_DIRECTION_IDX];
    SBAC_CTX_MODEL   mmvd_group_idx                [NUM_CTX_MMVD_GROUP_IDX];
    SBAC_CTX_MODEL   direct_mode_flag              [NUM_CTX_DIRECT_MODE_FLAG];
    SBAC_CTX_MODEL   merge_mode_flag               [NUM_CTX_MERGE_MODE_FLAG];
    SBAC_CTX_MODEL   inter_dir                     [NUM_CTX_INTER_PRED_IDC];
    SBAC_CTX_MODEL   intra_dir                     [NUM_CTX_INTRA_PRED_MODE];
    SBAC_CTX_MODEL   intra_luma_pred_mpm_flag      [NUM_CTX_INTRA_LUMA_PRED_MPM_FLAG];
    SBAC_CTX_MODEL   intra_luma_pred_mpm_idx       [NUM_CTX_INTRA_LUMA_PRED_MPM_IDX];
    SBAC_CTX_MODEL   intra_chroma_pred_mode        [NUM_CTX_INTRA_CHROMA_PRED_MODE];
    SBAC_CTX_MODEL   pred_mode                     [NUM_CTX_PRED_MODE];
    SBAC_CTX_MODEL   mode_cons                     [NUM_CTX_MODE_CONS];
    SBAC_CTX_MODEL   refi                          [NUM_CTX_REF_IDX];
    SBAC_CTX_MODEL   merge_idx                     [NUM_CTX_MERGE_IDX];
    SBAC_CTX_MODEL   mvp_idx                       [NUM_CTX_MVP_IDX];
    SBAC_CTX_MODEL   affine_mvp_idx                [NUM_CTX_AFFINE_MVP_IDX];
    SBAC_CTX_MODEL   mvr_idx                       [NUM_CTX_AMVR_IDX];
    SBAC_CTX_MODEL   bi_idx                        [NUM_CTX_BI_PRED_IDX];
    SBAC_CTX_MODEL   mvd                           [NUM_CTX_MVD];
    SBAC_CTX_MODEL   cbf_all                       [NUM_CTX_CBF_ALL];
    SBAC_CTX_MODEL   cbf_luma                      [NUM_CTX_CBF_LUMA];
    SBAC_CTX_MODEL   cbf_cb                        [NUM_CTX_CBF_CB];
    SBAC_CTX_MODEL   cbf_cr                        [NUM_CTX_CBF_CR];
    SBAC_CTX_MODEL   run                           [NUM_CTX_CC_RUN];
    SBAC_CTX_MODEL   last                          [NUM_CTX_CC_LAST];
    SBAC_CTX_MODEL   level                         [NUM_CTX_CC_LEVEL];
    SBAC_CTX_MODEL   sig_coeff_flag                [NUM_CTX_SIG_COEFF_FLAG];
    SBAC_CTX_MODEL   coeff_abs_level_greaterAB_flag[NUM_CTX_GTX];
    SBAC_CTX_MODEL   last_sig_coeff_x_prefix       [NUM_CTX_LAST_SIG_COEFF];
    SBAC_CTX_MODEL   last_sig_coeff_y_prefix       [NUM_CTX_LAST_SIG_COEFF];
    SBAC_CTX_MODEL   btt_split_flag                [NUM_CTX_BTT_SPLIT_FLAG];
    SBAC_CTX_MODEL   btt_split_dir                 [NUM_CTX_BTT_SPLIT_DIR];
    SBAC_CTX_MODEL   btt_split_type                [NUM_CTX_BTT_SPLIT_TYPE];
    SBAC_CTX_MODEL   affine_flag                   [NUM_CTX_AFFINE_FLAG];
    SBAC_CTX_MODEL   affine_mode                   [NUM_CTX_AFFINE_MODE];
    SBAC_CTX_MODEL   affine_mrg                    [NUM_CTX_AFFINE_MRG];
    SBAC_CTX_MODEL   affine_mvd_flag               [NUM_CTX_AFFINE_MVD_FLAG];
    SBAC_CTX_MODEL   suco_flag                     [NUM_CTX_SUCO_FLAG];
    SBAC_CTX_MODEL   alf_ctb_flag                  [NUM_CTX_ALF_CTB_FLAG];
    SBAC_CTX_MODEL   split_cu_flag                 [NUM_CTX_SPLIT_CU_FLAG];
    SBAC_CTX_MODEL   delta_qp                      [NUM_CTX_DELTA_QP];
    SBAC_CTX_MODEL   ats_mode                      [NUM_CTX_ATS_MODE_FLAG];
    SBAC_CTX_MODEL   ats_cu_inter_flag             [NUM_CTX_ATS_INTER_FLAG];
    SBAC_CTX_MODEL   ats_cu_inter_quad_flag        [NUM_CTX_ATS_INTER_QUAD_FLAG];
    SBAC_CTX_MODEL   ats_cu_inter_hor_flag         [NUM_CTX_ATS_INTER_HOR_FLAG];
    SBAC_CTX_MODEL   ats_cu_inter_pos_flag         [NUM_CTX_ATS_INTER_POS_FLAG];
    int              sps_cm_init_flag;

} XEVE_SBAC_CTX;

/* Maximum transform dynamic range (excluding sign bit) */
#define MAX_TX_DYNAMIC_RANGE               15
#define MAX_TX_VAL                       ((1 << MAX_TX_DYNAMIC_RANGE) - 1)
#define MIN_TX_VAL                      (-(1 << MAX_TX_DYNAMIC_RANGE))

#define QUANT_SHIFT                        14
#define QUANT_IQUANT_SHIFT                 20

/* neighbor CUs
   neighbor position:

   D     B     C

   A     X,<G>

   E          <F>
*/
#define MAX_NEB                            5
#define NEB_A                              0  /* left */
#define NEB_B                              1  /* up */
#define NEB_C                              2  /* up-right */
#define NEB_D                              3  /* up-left */
#define NEB_E                              4  /* low-left */

#define NEB_F                              5  /* co-located of low-right */
#define NEB_G                              6  /* co-located of X */
#define NEB_X                              7  /* center (current block) */
#define NEB_H                              8  /* right */
#define NEB_I                              9  /* low-right */
#define MAX_NEB2                           10

/* rpl structure */
#define XEVE_MAX_NUM_REF_PICS                   21
#define XEVE_MAX_NUM_ACTIVE_REF_FRAME           5

typedef struct _XEVE_RPL
{
    int             poc;
    int             tid;
    int             ref_pic_num;
    int             ref_pic_active_num;
    int             ref_pics[XEVE_MAX_NUM_REF_PICS];
    char            pic_type;
} XEVE_RPL;

/* picture store structure */
typedef struct _XEVE_PIC
{
    /* Address of Y buffer (include padding) */
    pel             *buf_y;
    /* Address of U buffer (include padding) */
    pel             *buf_u;
    /* Address of V buffer (include padding) */
    pel             *buf_v;
    /* Start address of Y component (except padding) */
    pel             *y;
    /* Start address of U component (except padding)  */
    pel             *u;
    /* Start address of V component (except padding)  */
    pel             *v;
    /* Stride of luma picture */
    int              s_l;
    /* Stride of chroma picture */
    int              s_c;
    /* Width of luma picture */
    int              w_l;
    /* Height of luma picture */
    int              h_l;
    /* Width of chroma picture */
    int              w_c;
    /* Height of chroma picture */
    int              h_c;
    /* padding size of luma */
    int              pad_l;
    /* padding size of chroma */
    int              pad_c;
    /* image buffer */
    XEVE_IMGB       * imgb;
    /* presentation temporal reference of this picture */
    u32              poc;
    /* 0: not used for reference buffer, reference picture type */
    u8               is_ref;
    u8               need_for_out;
    /* scalable layer id */
    u8               temporal_id;
    s16            (*map_mv)[REFP_NUM][MV_D];
    s16            (*map_unrefined_mv)[REFP_NUM][MV_D];
    s8             (*map_refi)[REFP_NUM];
    s8              *map_dqp_lah;
    u32              list_poc[XEVE_MAX_NUM_REF_PICS];
    u8               m_alfCtuEnableFlag[3][510];
    int              pic_deblock_alpha_offset;
    int              pic_deblock_beta_offset;
    int              pic_qp_u_offset;
    int              pic_qp_v_offset;
    u8               digest[N_C][16];
} XEVE_PIC;

/*****************************************************************************
 * picture buffer allocator
 *****************************************************************************/
typedef struct _PICBUF_ALLOCATOR PICBUF_ALLOCATOR;
struct _PICBUF_ALLOCATOR
{
    /* address of picture buffer allocation function */
    XEVE_PIC     *(* fn_alloc)(PICBUF_ALLOCATOR *pa, int *ret);
    /* address of picture buffer free function */
    void           (*fn_free)(PICBUF_ALLOCATOR *pa, XEVE_PIC *pic);
    /* width */
    int              w;
    /* height */
    int              h;
    /* pad size for luma */
    int              pad_l;
    /* pad size for chroma */
    int              pad_c;
    /* arbitrary data, if needs */
    int              ndata[4];
    /* arbitrary address, if needs */
    void            *pdata[4];
    int              bit_depth;
    int              chroma_format_idc;
};

/*****************************************************************************
 * picture manager
 *****************************************************************************/
typedef struct _XEVE_PM
{
    /* picture store (including reference and non-reference) */
    XEVE_PIC       * pic[MAX_PB_SIZE];
    /* address of reference pictures */
    XEVE_PIC       * pic_ref[XEVE_MAX_NUM_REF_PICS];
    /* maximum reference picture count */
    u8               max_num_ref_pics;
    /* current count of available reference pictures in PB */
    u8               cur_num_ref_pics;
    /* number of reference pictures */
    u8               num_refp[REFP_NUM];
    /* next output POC */
    u32              poc_next_output;
    /* POC increment */
    u8               poc_increase;
    /* max number of picture buffer */
    u8               max_pb_size;
    /* current picture buffer size */
    u8               cur_pb_size;
    /* address of leased picture for current decoding/encoding buffer */
    XEVE_PIC       * pic_lease;
    /* picture buffer allocator */
    PICBUF_ALLOCATOR pa;
} XEVE_PM;

/* reference picture structure */
typedef struct _XEVE_REFP
{
    /* address of reference picture */
    XEVE_PIC       * pic;
    /* POC of reference picture */
    u32              poc;
    s16            (*map_mv)[REFP_NUM][MV_D];
    s16            (*map_unrefined_mv)[REFP_NUM][MV_D];
    s8             (*map_refi)[REFP_NUM];
    u32             *list_poc;
} XEVE_REFP;

#define XEVE_MAX_QP_TABLE_SIZE           58
#define XEVE_MAX_QP_TABLE_SIZE_EXT       94

/* chromaQP table structure */
typedef struct _XEVE_CHROMA_TABLE
{
    int                chroma_qp_table_present_flag;
    int                same_qp_table_for_chroma;
    int                global_offset_flag;
    int                num_points_in_qp_table_minus1[2];
    int                delta_qp_in_val_minus1[2][XEVE_MAX_QP_TABLE_SIZE];
    int                delta_qp_out_val[2][XEVE_MAX_QP_TABLE_SIZE];
} XEVE_CHROMA_TABLE;

/*****************************************************************************
 * NALU header
 *****************************************************************************/
typedef struct _XEVE_NALU
{
    int              nal_unit_size;
    int              forbidden_zero_bit;
    int              nal_unit_type_plus1;
    int              nuh_temporal_id;
    int              nuh_reserved_zero_5bits;
    int              nuh_extension_flag;
} XEVE_NALU;

#define     EXTENDED_SAR 255
#define     NUM_CPB 32

/*****************************************************************************
* Hypothetical Reference Decoder (HRD) parameters, part of VUI
*****************************************************************************/
typedef struct _XEVE_HRD
{
    int              cpb_cnt_minus1;
    int              bit_rate_scale;
    int              cpb_size_scale;
    int              bit_rate_value_minus1[NUM_CPB];
    int              cpb_size_value_minus1[NUM_CPB];
    int              cbr_flag[NUM_CPB];
    int              initial_cpb_removal_delay_length_minus1;
    int              cpb_removal_delay_length_minus1;
    int              dpb_output_delay_length_minus1;
    int              time_offset_length;
} XEVE_HRD;

/*****************************************************************************
* video usability information (VUI) part of SPS
*****************************************************************************/
typedef struct _XEVE_VUI
{
    int              aspect_ratio_info_present_flag;
    int              aspect_ratio_idc;
    int              sar_width;
    int              sar_height;
    int              overscan_info_present_flag;
    int              overscan_appropriate_flag;
    int              video_signal_type_present_flag;
    int              video_format;
    int              video_full_range_flag;
    int              colour_description_present_flag;
    int              colour_primaries;
    int              transfer_characteristics;
    int              matrix_coefficients;
    int              chroma_loc_info_present_flag;
    int              chroma_sample_loc_type_top_field;
    int              chroma_sample_loc_type_bottom_field;
    int              neutral_chroma_indication_flag;
    int              field_seq_flag;
    int              timing_info_present_flag;
    int              num_units_in_tick;
    int              time_scale;
    int              fixed_pic_rate_flag;
    int              nal_hrd_parameters_present_flag;
    int              vcl_hrd_parameters_present_flag;
    int              low_delay_hrd_flag;
    int              pic_struct_present_flag;
    int              bitstream_restriction_flag;
    int              motion_vectors_over_pic_boundaries_flag;
    int              max_bytes_per_pic_denom;
    int              max_bits_per_mb_denom;
    int              log2_max_mv_length_horizontal;
    int              log2_max_mv_length_vertical;
    int              num_reorder_pics;
    int              max_dec_pic_buffering;
    XEVE_HRD         hrd_parameters;
} XEVE_VUI;

/*****************************************************************************
 * sequence parameter set
 *****************************************************************************/
typedef struct _XEVE_SPS
{
    int              sps_seq_parameter_set_id;
    int              profile_idc;
    int              level_idc;
    int              toolset_idc_h;
    int              toolset_idc_l;
    int              chroma_format_idc;
    u32              pic_width_in_luma_samples;
    u32              pic_height_in_luma_samples;
    int              bit_depth_luma_minus8;
    int              bit_depth_chroma_minus8;
    int              sps_btt_flag;
    int              sps_suco_flag;
    int              log2_ctu_size_minus5;
    int              log2_min_cb_size_minus2;
    int              log2_diff_ctu_max_14_cb_size;
    int              log2_diff_ctu_max_tt_cb_size;
    int              log2_diff_min_cb_min_tt_cb_size_minus2;
    int              log2_diff_ctu_size_max_suco_cb_size;
    int              log2_diff_max_suco_min_suco_cb_size;
    int              tool_amvr;
    int              tool_mmvd;
    int              tool_affine;
    int              tool_dmvr;
    int              tool_addb;
    int              tool_alf;
    int              tool_htdf;
    int              tool_admvp;
    int              tool_hmvp;
    int              tool_eipd;
    int              tool_iqt;
    int              tool_cm_init;
    int              tool_ats;
    int              tool_rpl;
    int              tool_pocs;
    int              log2_sub_gop_length;
    int              log2_ref_pic_gap_length;
    int              tool_adcc;
    int              log2_max_pic_order_cnt_lsb_minus4;
    int              sps_max_dec_pic_buffering_minus1;
    int              max_num_ref_pics;
    u32              long_term_ref_pics_flag;
    /* HLS_RPL  */
    int              rpl1_same_as_rpl0_flag;
    int              num_ref_pic_lists_in_sps0;
    XEVE_RPL         rpls_l0[XEVE_MAX_NUM_RPLS];
    int              num_ref_pic_lists_in_sps1;
    XEVE_RPL         rpls_l1[XEVE_MAX_NUM_RPLS];
    int              picture_cropping_flag;
    int              picture_crop_left_offset;
    int              picture_crop_right_offset;
    int              picture_crop_top_offset;
    int              picture_crop_bottom_offset;
    int              dquant_flag;              /*1 specifies the improved delta qp signaling processes is used*/
    XEVE_CHROMA_TABLE chroma_qp_table_struct;
    u32              ibc_flag;                   /* 1 bit : flag of enabling IBC or not */
    int              ibc_log_max_size;           /* log2 max ibc size */
    int              vui_parameters_present_flag;
    int              tool_dra;
    XEVE_VUI         vui_parameters;
} XEVE_SPS;

/*****************************************************************************
* picture parameter set
*****************************************************************************/
typedef struct _XEVE_PPS
{
    int              pps_pic_parameter_set_id;
    int              pps_seq_parameter_set_id;
    int              num_ref_idx_default_active_minus1[2];
    int              additional_lt_poc_lsb_len;
    int              rpl1_idx_present_flag;
    int              single_tile_in_pic_flag;
    int              num_tile_columns_minus1;
    int              num_tile_rows_minus1;
    int              uniform_tile_spacing_flag;
    int              tile_column_width_minus1[XEVE_MAX_NUM_TILES_ROW];
    int              tile_row_height_minus1[XEVE_MAX_NUM_TILES_COL];
    int              loop_filter_across_tiles_enabled_flag;
    int              tile_offset_lens_minus1;
    int              tile_id_len_minus1;
    int              explicit_tile_id_flag;
    int              tile_id_val[XEVE_MAX_NUM_TILES_ROW][XEVE_MAX_NUM_TILES_COL];
    int              arbitrary_slice_present_flag;
    int              constrained_intra_pred_flag;
    int              cu_qp_delta_enabled_flag;
    int              cu_qp_delta_area;
    int              pic_dra_enabled_flag;
    int              pic_dra_aps_id;
} XEVE_PPS;

/*****************************************************************************
 * slice header
 *****************************************************************************/
typedef struct _XEVE_ALF_SLICE_PARAM
{
    BOOL             is_ctb_alf_on;
    u8             * alf_ctb_flag;
    u8             * alf_ctb_chroma_flag;
    u8             * alf_ctb_chroma2_flag;
    BOOL             enable_flag[3];                                            // alf_slice_enable_flag, alf_chroma_idc
    int              luma_filter_type;                                          // filter_type_flag
    BOOL             chroma_ctb_present_flag;                                   // alf_chroma_ctb_present_flag
    short            luma_coef[MAX_NUM_ALF_CLASSES * MAX_NUM_ALF_LUMA_COEFF];   // alf_coeff_luma_delta[i][j]
    short            chroma_coef[MAX_NUM_ALF_CHROMA_COEFF];                     // alf_coeff_chroma[i]
    short            filter_coef_delta_idx[MAX_NUM_ALF_CLASSES];                // filter_coeff_delta[i]
    BOOL             filter_coef_flag[MAX_NUM_ALF_CLASSES];                     // filter_coefficient_flag[i]
    int              num_luma_filters;                                          // number_of_filters_minus1 + 1
    BOOL             coef_delta_flag;                                           // alf_coefficients_delta_flag
    BOOL             coef_delta_pred_mode_flag;                                 // coeff_delta_pred_mode_flag
    int              fixed_filter_pattern;
    int              fixed_filter_idx[MAX_NUM_ALF_CLASSES];
    u8               fixed_filter_usage_flag[MAX_NUM_ALF_CLASSES];
    int              t_layer;
    BOOL             temporal_alf_flag;
    int              prev_idx;
    int              prev_idx_comp[2];
    BOOL             reset_alf_buf_flag;
    BOOL             store2_alf_buf_flag;
    BOOL             chroma_filter_present;
} XEVE_ALF_SLICE_PARAM;

typedef struct _XEVE_SIGNALLED_ALF_PARAM
{
    BOOL             is_ctb_alf_on;
    BOOL             enable_flag[3];                                            // alf_slice_enable_flag, alf_chroma_idc
    int              luma_filter_type;                                          // filter_type_flag
    BOOL             chroma_ctb_present_flag;                                   // alf_chroma_ctb_present_flag
    short            chroma_coef[MAX_NUM_ALF_CHROMA_COEFF];                     // alf_coeff_chroma[i]
    short            filter_coef_delta_idx[MAX_NUM_ALF_CLASSES];                // filter_coeff_delta[i]
    BOOL             filter_coef_flag[MAX_NUM_ALF_CLASSES];                     // filter_coefficient_flag[i]
    int              num_luma_filters;                                          // number_of_filters_minus1 + 1
    BOOL             coef_delta_flag;                                           // alf_coefficients_delta_flag
    BOOL             coef_delta_pred_mode_flag;                                 // coeff_delta_pred_mode_flag
    int              fixed_filter_pattern;
    int              fixed_filter_idx[MAX_NUM_ALF_CLASSES];
    u8               fixed_filter_usage_flag[MAX_NUM_ALF_CLASSES];
    int              prev_idx;
} XEVE_SIGNALLED_ALF_PARAM;


typedef struct _XEVE_APS_GEN
{
    int              signal_flag;
    int              aps_type_id;          // adaptation_parameter_set_type_id
    int              aps_id;               // adaptation_parameter_set_id
    void           * aps_data;
} XEVE_APS_GEN;

typedef struct _XEVE_APS
{
    int              aps_id;               // adaptation_parameter_set_id
    int              aps_id_y;
    int              aps_id_ch;
    XEVE_ALF_SLICE_PARAM alf_aps_param;   // alf data
} XEVE_APS;

typedef enum _XEVE_SEI_PAYLOAD_TYPE
{
    BUFFERING_PERIOD = 0,
    PICTURE_TIMING = 1,
    USER_DATA_REGISTERED_ITU_T_T35 = 4,
    USER_DATA_UNREGISTERED = 5,
    RECOVERY_POINT = 6,
    MASTERING_DISPLAY_INFO = 137,
    CONTENT_LIGHT_LEVEL_INFO = 144,
    AMBIENT_VIEWING_ENVIRONMENT = 148,
} XEVE_SEI_PAYLOAD_TYPE;

typedef struct _XEVE_SEI_PAYLOAD
{
    int payload_size;
    XEVE_SEI_PAYLOAD_TYPE payload_type;
    u8* payload;
} XEVE_SEI_PAYLOAD;

typedef struct _XEVE_SEI
{
    int num_payloads;
    XEVE_SEI_PAYLOAD *payloads;
} XEVE_SEI;

typedef struct _XEVE_BUFFERING_PERIOD
{
    u32 initial_cpb_removaldelay;
    u32 initial_cpb_removal_delay_offset;
}XEVE_BUFFERING_PERIOD;

typedef struct _XEVE_PICTURE_TIMING
{
    u32       pic_struct;
    BOOL      clock_timestamp_flag;
    BOOL      nuit_field_based_flag;
    BOOL      full_timestamp_flag;
    BOOL      discontinuity_flag;
    BOOL      cnt_dropped_flag;
    u64       n_frames;
    u32       ct_type;
    u64       seconds_value;
    u64       minutes_value;
    u64       hours_value;
    BOOL      seconds_flag;
    BOOL      minutes_flag;
    BOOL      hours_flag;
    u32       cpb_removal_delay;
    u32       dpb_output_delay;
}XEVE_PICTURE_TIMING;

typedef struct _XEVE_RECOVERY_POINT
{
    int  recovery_poc_cnt;
    BOOL exact_matching_flag;
    BOOL broken_link_flag;
}XEVE_RECOVERY_POINT;

#define ISO_IEC_11578_LEN 16


typedef struct _XEVE_MASTER_DISPLAY_COLOR_V
{
    u16 display_primary_x[3];
    u16 display_primary_y[3];
    u16 white_point_x, white_point_y;
    u32 max_display_mastering_luminance;
    u32 min_display_mastering_luminance;
}XEVE_MASTER_DISPLAY_COLOR_V;

typedef struct _XEVE_CONTENT_LIGHT_LEVEL_INFO
{
    u16 max_content_light_level;
    u16 max_pic_average_light_level;
}XEVE_CONTENT_LIGHT_LEVEL_INFO;
typedef struct _XEVE_SH
{
    int              slice_pic_parameter_set_id;
    int              single_tile_in_slice_flag;
    int              first_tile_id;
    int              arbitrary_slice_flag;
    int              last_tile_id;
    int              num_remaining_tiles_in_slice_minus1;
    int              delta_tile_id_minus1[XEVE_MAX_NUM_TILES_ROW * XEVE_MAX_NUM_TILES_COL];
    int              slice_type;
    int              no_output_of_prior_pics_flag;
    int              slice_alf_enabled_flag;
    int              temporal_mvp_asigned_flag;
    int              collocated_from_list_idx;        // Specifies source (List ID) of the collocated picture, equialent of the collocated_from_l0_flag
    int              collocated_from_ref_idx;         // Specifies source (RefID_ of the collocated picture, equialent of the collocated_ref_idx
    int              collocated_mvp_source_list_idx;  // Specifies source (List ID) in collocated pic that provides MV information
    s32              poc_lsb;
    /*   HLS_RPL */
    u32              ref_pic_list_sps_flag[2];
    int              rpl_l0_idx;                            //-1 means this slice does not use RPL candidate in SPS for RPL0
    int              rpl_l1_idx;                            //-1 means this slice does not use RPL candidate in SPS for RPL1

    XEVE_RPL          rpl_l0;
    XEVE_RPL          rpl_l1;

    u32              num_ref_idx_active_override_flag;
    int              deblocking_filter_on;
    int              sh_deblock_alpha_offset;
    int              sh_deblock_beta_offset;
    int              qp;
    int              qp_u;
    int              qp_v;
    int              qp_u_offset;
    int              qp_v_offset;
    u32              entry_point_offset_minus1[XEVE_MAX_NUM_TILES_ROW * XEVE_MAX_NUM_TILES_COL];
    /*QP of previous cu in decoding order (used for dqp)*/
    u8               qp_prev_eco;
    u8               dqp;
    u8               qp_prev_mode;
    u32              alf_on;
    u32              mmvd_group_enable_flag;
    u8               ctb_alf_on;
    u16              num_ctb;
    int              aps_signaled;
    int              aps_id_y;
    int              aps_id_ch;
    XEVE_APS*         aps;
    XEVE_ALF_SLICE_PARAM alf_sh_param;
    u16              num_tiles_in_slice;
    u32              alf_chroma_idc;
    u32              ChromaAlfEnabledFlag;
    u32              ChromaAlfEnabled2Flag;
    u32              alfChromaMapSignalled;
    u32              alfChroma2MapSignalled;
    int              aps_id_ch2;
    u8               tile_order[XEVE_MAX_NUM_TILES_COL * XEVE_MAX_NUM_TILES_ROW];
} XEVE_SH;

/*****************************************************************************
* Tiles
*****************************************************************************/
typedef struct _XEVE_TILE
{
    /* tile width in CTB unit */
    u16              w_ctb;
    /* tile height in CTB unit */
    u16              h_ctb;
    /* tile size in CTB unit (= w_ctb * h_ctb) */
    u32              f_ctb;
    /* first ctb address in raster scan order */
    u16              ctba_rs_first;
    u8               qp;
    u8               qp_prev_eco[XEVE_MAX_THREADS];
} XEVE_TILE;

/*****************************************************************************/

typedef struct _XEVE_POC
{
    /* current picture order count value */
    int              poc_val;
    /* the picture order count of the previous Tid0 picture */
    u32              prev_poc_val;
    /* the decoding order count of the previous picture */
    int              prev_doc_offset;
    int              prev_idr_poc;
} XEVE_POC;

/*****************************************************************************
 * user data types
 *****************************************************************************/
#define XEVE_UD_PIC_SIGNATURE              0x10
#define XEVE_UD_END                        0xFF

typedef enum _TREE_TYPE
{
    TREE_LC = 0,
    TREE_L  = 1,
    TREE_C  = 2,
} TREE_TYPE;

typedef enum _MODE_CONS
{
    eOnlyIntra,
    eOnlyInter,
    eAll
} MODE_CONS;

typedef struct _TREE_CONS
{
    BOOL            changed;
    TREE_TYPE       tree_type;
    MODE_CONS       mode_cons;
} TREE_CONS;

typedef struct _TREE_CONS_NEW
{
    TREE_TYPE       tree_type;
    MODE_CONS       mode_cons;
} TREE_CONS_NEW;

/*****************************************************************************
 * for binary and triple tree structure
 *****************************************************************************/
typedef enum _SPLIT_MODE
{
    NO_SPLIT        = 0,
    SPLIT_BI_VER    = 1,
    SPLIT_BI_HOR    = 2,
    SPLIT_TRI_VER   = 3,
    SPLIT_TRI_HOR   = 4,
    SPLIT_QUAD      = 5,
} SPLIT_MODE;

typedef enum _SPLIT_DIR
{
    SPLIT_VER = 0,
    SPLIT_HOR = 1,
} SPLIT_DIR;

typedef enum _BLOCK_SHAPE
{
    NON_SQUARE_14,
    NON_SQUARE_12,
    SQUARE,
    NON_SQUARE_21,
    NON_SQUARE_41,
    NUM_BLOCK_SHAPE,
} BLOCK_SHAPE;

typedef enum _BLOCK_PARAMETER
{
    BLOCK_11,
    BLOCK_12,
    BLOCK_14,
    BLOCK_TT,
    NUM_BLOCK_PARAMETER,
} BLOCK_PARAMETER;

typedef enum _BLOCK_PARAMETER_IDX
{
    IDX_MAX,
    IDX_MIN,
    NUM_BLOCK_IDX,
} BLOCK_PARAMETER_IDX;

/*****************************************************************************
* history-based MV prediction buffer (slice level)
*****************************************************************************/
typedef struct _XEVE_HISTORY_BUFFER
{
    s16 history_mv_table[ALLOWED_CHECKED_NUM][REFP_NUM][MV_D];
    s8  history_refi_table[ALLOWED_CHECKED_NUM][REFP_NUM];
#if TRACE_ENC_CU_DATA
    u64 history_cu_table[ALLOWED_CHECKED_NUM];
#endif
    int currCnt;
    int m_maxCnt;
} XEVE_HISTORY_BUFFER;

typedef enum _CTX_NEV_IDX
{
    CNID_SKIP_FLAG,
    CNID_PRED_MODE,
    CNID_MODE_CONS,
    CNID_AFFN_FLAG,
    CNID_IBC_FLAG,
    NUM_CNID,

} CTX_NEV_IDX;

typedef enum _MSL_IDX
{
    MSL_SKIP,  //skip
    MSL_MERG,  //merge or direct
    MSL_LIS0,  //list 0
    MSL_LIS1,  //list 1
    MSL_BI,    //bi pred
    NUM_MODE_SL,

} MSL_IDX;

#define DMVR_PAD_LENGTH                    2
#define EXTRA_PIXELS_FOR_FILTER            7 // Maximum extraPixels required for final MC based on fiter size
#define PAD_BUFFER_STRIDE                ((MAX_CU_SIZE + EXTRA_PIXELS_FOR_FILTER + (DMVR_ITER_COUNT * 2)))

static const int NTAPS_LUMA = 8; ///< Number of taps for luma
static const int NTAPS_CHROMA = 4; ///< Number of taps for chroma

#define EIF_MV_PRECISION_INTERNAL         (2 + MAX_CU_LOG2 + 0) //2 + MAX_CU_LOG2 is MV precision in regular affine

#if EIF_MV_PRECISION_INTERNAL > 14 || EIF_MV_PRECISION_INTERNAL < 9
#error "Invalid EIF_MV_PRECISION_INTERNAL"
#endif

#if EIF_MV_PRECISION_BILINEAR > EIF_MV_PRECISION_INTERNAL
#error "EIF_MV_PRECISION_BILINEAR should be less than EIF_MV_PRECISION_INTERNAL"
#endif

#if EIF_MV_PRECISION_BILINEAR < 3
#error "EIF_MV_PRECISION_BILINEAR is to small"
#endif

#define MAX_SUB_TB_NUM 4
enum TQC_RUN {
    RUN_L = 1,
    RUN_CB = 2,
    RUN_CR = 4
};

#include "xeve_thread_pool.h"
#include "xeve_recon.h"
#include "xeve_ipred.h"
#include "xeve_picman.h"
#include "xeve_mc.h"
#if defined(__AVX2__)
#include "xeve_mc_sse.h"
#include "xeve_mc_avx.h"
#elif defined(ARM)
#include "xeve_mc_neon.h"
#endif
#include "xeve_type.h"


#endif /* _XEVE_DEF_H_ */

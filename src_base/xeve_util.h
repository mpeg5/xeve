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

#ifndef __XEVE_UTIL_H__
#define __XEVE_UTIL_H__

#include "xeve_def.h"

/* macro to determine maximum */
#define XEVE_MAX(a,b)               (((a) > (b)) ? (a) : (b))

/* macro to determine minimum */
#define XEVE_MIN(a,b)               (((a) < (b)) ? (a) : (b))

/* macro to determine median */
#define XEVE_MEDIAN(x,y,z)          ((((y) < (z))^((z) < (x))) ? (((x) < (y))^((z) < (x))) ? (y) : (x) : (z))

/* macro to absolute a value */
#define XEVE_ABS(a)                 (((a) > (0)) ? (a) : (-(a)))

/* macro to absolute a 64-bit value */
#define XEVE_ABS64(a)               (((a)^((a)>>63)) - ((a)>>63))

/* macro to absolute a 32-bit value */
#define XEVE_ABS32(a)               (((a)^((a)>>31)) - ((a)>>31))

/* macro to absolute a 16-bit value */
#define XEVE_ABS16(a)               (((a)^((a)>>15)) - ((a)>>15))

/* macro to clipping within min and max */
#define XEVE_CLIP3(min, max, val)   XEVE_MAX((min), XEVE_MIN((max), (val)))

/* macro to get a sign from a 16-bit value.
operation: if(val < 0) return 1, else return 0 */
#define XEVE_SIGN_GET(val)          ((val < 0) ? 1 : 0)

/* macro to set sign into a value.
operation: if(sign == 0) return val, else if(sign == 1) return -val */
#define XEVE_SIGN_SET(val, sign)    ((sign)? -val : val)

/* macro to get a sign from a 16-bit value.
operation: if(val < 0) return 1, else return 0 */
#define XEVE_SIGN_GET16(val)        (((val)>>15) & 1)

/* macro to set sign into a 16-bit value.
operation: if(sign == 0) return val, else if(sign == 1) return -val */
#define XEVE_SIGN_SET16(val, sign)  (((val) ^ ((s16)((sign)<<15)>>15)) + (sign))

/* macro to clipping addition */
#define XEVE_CLIP16_ADD(a,b)        (XEVE_MIN((a)+(b),0xffff))

/* macro to modulo index */
#define XEVE_MOD_IDX(num, mod)      (((num) + (mod)) % (mod))

/* change to log value */
#define XEVE_LOG2(v)                (xeve_tbl_log2[v])
#define XEVE_ALIGN_VAL(val, align)  ((((val)+(align)-1)/(align))*(align))
#define XEVE_CFI_FROM_CF(cf) ((cf == XEVE_CF_YCBCR400) ? 0 : (cf == XEVE_CF_YCBCR420) ? 1 : (cf == XEVE_CF_YCBCR422) ? 2 : 3)
#define XEVE_CF_FROM_CFI(chroma_format_idc)  ((chroma_format_idc == 0) ? XEVE_CF_YCBCR400 : (chroma_format_idc == 1) ? \
                                        XEVE_CF_YCBCR420 : (chroma_format_idc == 2) ? XEVE_CF_YCBCR422 : XEVE_CF_YCBCR444)
#define XEVE_GET_CHROMA_W_SHIFT(chroma_format_idc) ((chroma_format_idc == 0) ? 1 : (chroma_format_idc == 1) ? 1 : (chroma_format_idc == 2) ? 1 : 0)
#define XEVE_GET_CHROMA_H_SHIFT(chroma_format_idc) ((chroma_format_idc == 0) ? 1 : (chroma_format_idc == 1) ? 1 : 0)

u16  xeve_get_avail_inter(int x_scu, int y_scu, int w_scu, int h_scu, int scup, int cuw, int cuh, u32 *map_scu, u8* map_tidx);
u16  xeve_get_avail_intra(int x_scu, int y_scu, int w_scu, int h_scu, int scup, int log2_cuw, int log2_cuh, u32 *map_scu, u8* map_tidx);
XEVE_PIC* xeve_picbuf_alloc(int w, int h, int pad_l, int pad_c, int bit_depth, int *err, int chroma_format_idc);
void xeve_picbuf_free(XEVE_PIC *pic);
void xeve_picbuf_expand(XEVE_PIC *pic, int exp_l, int exp_c, int chroma_format_idc);
void xeve_poc_derivation(XEVE_SPS sps, int tid, XEVE_POC *poc);
void xeve_picbuf_rc_free(XEVE_PIC *pic);
void xeve_check_motion_availability(int scup, int cuw, int cuh, int w_scu, int h_scu, int neb_addr[MAX_NUM_POSSIBLE_SCAND], int valid_flag[MAX_NUM_POSSIBLE_SCAND], u32 *map_scu, u16 avail_lr, int num_mvp, int is_ibc, u8 * map_tidx);
int  xeve_get_default_motion(int neb_addr[MAX_NUM_POSSIBLE_SCAND], int valid_flag[MAX_NUM_POSSIBLE_SCAND], s8 cur_refi, int lidx, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], s8 *refi, s16 mv[MV_D]
                           , u32 *map_scu, s16(*map_unrefined_mv)[REFP_NUM][MV_D], int scup, int w_scu);
s8   xeve_get_first_refi(int scup, int lidx, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], int cuw, int cuh, int w_scu, int h_scu, u32 *map_scu, u8 mvr_idx, u16 avail_lr
                       , s16(*map_unrefined_mv)[REFP_NUM][MV_D], u8 * map_tidx);
void xeve_get_motion(int scup, int lidx, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D]
                   , XEVE_REFP(*refp)[REFP_NUM], int cuw, int cuh, int w_scu, u16 avail, s8 refi[MAX_NUM_MVP], s16 mvp[MAX_NUM_MVP][MV_D]);
void xeve_get_motion_skip(int slice_type, int scup, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], XEVE_REFP refp[REFP_NUM], int cuw, int cuh, int w_scu
                        , s8 refi[REFP_NUM][MAX_NUM_MVP], s16 mvp[REFP_NUM][MAX_NUM_MVP][MV_D], u16 avail_lr);
XEVE_PIC* xeve_alloc_spic_l(int w, int h);

enum
{
    SPLIT_MAX_PART_COUNT = 4
};

typedef struct _XEVE_SPLIT_STRUCT
{
    int       part_count;
    int       cud[SPLIT_MAX_PART_COUNT];
    int       width[SPLIT_MAX_PART_COUNT];
    int       height[SPLIT_MAX_PART_COUNT];
    int       log_cuw[SPLIT_MAX_PART_COUNT];
    int       log_cuh[SPLIT_MAX_PART_COUNT];
    int       x_pos[SPLIT_MAX_PART_COUNT];
    int       y_pos[SPLIT_MAX_PART_COUNT];
    int       cup[SPLIT_MAX_PART_COUNT];
    TREE_CONS tree_cons;
} XEVE_SPLIT_STRUCT;

void xeve_split_get_part_structure(int split_mode, int x0, int y0, int cuw, int cuh, int cup, int cud, int log2_culine, XEVE_SPLIT_STRUCT* split_struct);
void xeve_get_mv_dir(XEVE_REFP refp[REFP_NUM], u32 poc, int scup, int c_scu, u16 w_scu, u16 h_scu, s16 mvp[REFP_NUM][MV_D], int sps_admvp_flag);
int  xeve_get_avail_cu(int neb_scua[MAX_NEB2], u32 * map_cu, u8 * map_tidx);
int  xeve_get_split_mode(s8* split_mode, int cud, int cup, int cuw, int cuh, int lcu_s, s8(*split_mode_buf)[NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU]);
void xeve_set_split_mode(s8  split_mode, int cud, int cup, int cuw, int cuh, int lcu_s, s8(*split_mode_buf)[NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU]);
u16  xeve_check_nev_avail(int x_scu, int y_scu, int cuw, int cuh, int w_scu, int h_scu, u32 * map_scu, u8* map_tidx);
void xeve_get_ctx_some_flags(int x_scu, int y_scu, int cuw, int cuh, int w_scu, u32* map_scu, u32* map_cu_mode, u8* ctx, u8 slice_type, int sps_cm_init_flag, u8 ibc_flag, u8 ibc_log_max_size, u8* map_tidx);

/* MD5 structure */
typedef struct _XEVE_MD5
{
    u32     h[4]; /* hash state ABCD */
    u8      msg[64]; /*input buffer (nalu message) */
    u32     bits[2]; /* number of bits, modulo 2^64 (lsb first)*/
} XEVE_MD5;

/* MD5 Functions */
void xeve_md5_init(XEVE_MD5 * md5);
void xeve_md5_update(XEVE_MD5 * md5, void * buf, u32 len);
void xeve_md5_update_16(XEVE_MD5 * md5, void * buf, u32 len);
void xeve_md5_finish(XEVE_MD5 * md5, u8 digest[16]);
int  xeve_md5_imgb(XEVE_IMGB * imgb, u8 digest[N_C][16]);
int  xeve_picbuf_signature(XEVE_PIC * pic, u8 md5_out[N_C][16]);
int  xeve_atomic_inc(volatile int * pcnt);
int  xeve_atomic_dec(volatile int * pcnt);
void xeve_init_scan_sr(int *scan, int size_x, int size_y, int width, int height, int scan_type);
void xeve_init_inverse_scan_sr(u16 *scan_inv, u16 *scan_orig, int width, int height, int scan_type);

int  xeve_get_transform_shift(int log2_size, int type, int bit_depth);

BOOL check_bi_applicability(int slice_type, int cuw, int cuh, int is_sps_admvp);
void xeve_block_copy(s16 * src, int src_stride, s16 * dst, int dst_stride, int log2_copy_w, int log2_copy_h);
int  xeve_get_luma_cup(int x_scu, int y_scu, int cu_w_scu, int cu_h_scu, int w_scu);

u8   xeve_check_luma(TREE_CONS tree_cons);
u8   xeve_check_chroma(TREE_CONS tree_cons);
u8   xeve_check_all(TREE_CONS tree_cons);
u8   xeve_check_only_intra(TREE_CONS tree_cons);
u8   xeve_check_only_inter(TREE_CONS tree_cons);
u8   xeve_check_all_preds(TREE_CONS tree_cons);
TREE_CONS xeve_get_default_tree_cons();
void xeve_set_tree_mode(TREE_CONS* dest, MODE_CONS mode);
MODE_CONS xeve_get_mode_cons_by_split(SPLIT_MODE split_mode, int cuw, int cuh);
BOOL xeve_signal_mode_cons(TREE_CONS* parent, TREE_CONS* cur_split);

#define XEVE_IMGB_OPT_NONE                 (0)
XEVE_IMGB * xeve_imgb_create(int w, int h, int cs, int opt, int pad[XEVE_IMGB_MAX_PLANE], int align[XEVE_IMGB_MAX_PLANE]);
void xeve_imgb_cpy(XEVE_IMGB * dst, XEVE_IMGB * src);
void xeve_imgb_garbage_free(XEVE_IMGB * imgb);
#define XEVE_CPU_INFO_SSE2     0x7A // ((3 << 5) | 26)
#define XEVE_CPU_INFO_SSE3     0x40 // ((2 << 5) |  0)
#define XEVE_CPU_INFO_SSSE3    0x49 // ((2 << 5) |  9)
#define XEVE_CPU_INFO_SSE41    0x53 // ((2 << 5) | 19)
#define XEVE_CPU_INFO_OSXSAVE  0x5B // ((2 << 5) | 27)
#define XEVE_CPU_INFO_AVX      0x5C // ((2 << 5) | 28)
#define XEVE_CPU_INFO_AVX2     0x25 // ((1 << 5) |  5)

int  xeve_check_cpu_info();

void xeve_copy_chroma_qp_mapping_params(XEVE_CHROMA_TABLE *dst, XEVE_CHROMA_TABLE *src);
void xeve_update_core_loc_param(XEVE_CTX * ctx, XEVE_CORE * core);
void xeve_update_core_loc_param_mt(XEVE_CTX * ctx, XEVE_CORE * core);
int  xeve_mt_get_next_ctu_num(XEVE_CTX * ctx, XEVE_CORE * core, int skip_ctb_line_cnt);
int  xeve_create_cu_data(XEVE_CU_DATA *cu_data, int log2_cuw, int log2_cuh, int chroma_format_idc);
int  xeve_delete_cu_data(XEVE_CU_DATA *cu_data, int log2_cuw, int log2_cuh);
void xeve_set_tile_in_slice(XEVE_CTX * ctx);
void xeve_set_chroma_qp_tbl_loc(XEVE_CTX* ctx);

#endif /* __XEVE_UTIL_H__ */

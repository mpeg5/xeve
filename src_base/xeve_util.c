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

#include "xeve_type.h"
#include <math.h>

#define TX_SHIFT1(log2_size, bd)   ((log2_size) - 1 + bd - 8)
#define TX_SHIFT2(log2_size)   ((log2_size) + 6)

#if ENC_DEC_TRACE
FILE *fp_trace;
#if TRACE_RDO
#if TRACE_RDO_EXCLUDE_I
int fp_trace_print = 0;
#else
int fp_trace_print = 1;
#endif
#else
int fp_trace_print = 0;
#endif
int fp_trace_counter = 0;
#endif
#if TRACE_START_POC
int fp_trace_started = 0;
#endif

int xeve_atomic_inc(volatile int *pcnt)
{
    int ret;
    ret = *pcnt;
    ret++;
    *pcnt = ret;
    return ret;
}

int xeve_atomic_dec(volatile int *pcnt)
{
    int ret;
    ret = *pcnt;
    ret--;
    *pcnt = ret;
    return ret;
}

XEVE_PIC * xeve_picbuf_alloc(int w, int h, int pad_l, int pad_c, int bit_depth, int *err, int chroma_format_idc)
{
    XEVE_PIC *pic = NULL;
    XEVE_IMGB *imgb = NULL;
    int ret, opt, align[XEVE_IMGB_MAX_PLANE], pad[XEVE_IMGB_MAX_PLANE];
    int w_scu, h_scu, f_scu, size;
    int cs;

    /* allocate PIC structure */
    pic = xeve_malloc(sizeof(XEVE_PIC));
    xeve_assert_gv(pic != NULL, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    xeve_mset(pic, 0, sizeof(XEVE_PIC));

    opt = XEVE_IMGB_OPT_NONE;

    /* set align value*/
    align[0] = MIN_CU_SIZE;
    align[1] = MIN_CU_SIZE;
    align[2] = MIN_CU_SIZE;

    /* set padding value*/
    pad[0] = pad_l;
    pad[1] = pad_c;
    pad[2] = pad_c;

    cs = XEVE_CS_SET(XEVE_CF_FROM_CFI(chroma_format_idc), bit_depth, 0);
    imgb = xeve_imgb_create(w, h, cs, opt, pad, align);
    imgb->cs = XEVE_CS_SET(cs, bit_depth, 0);

    xeve_assert_gv(imgb != NULL, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

    /* set XEVE_PIC */
    pic->buf_y = imgb->baddr[0];
    pic->buf_u = imgb->baddr[1];
    pic->buf_v = imgb->baddr[2];
    pic->y     = imgb->a[0];
    pic->u     = imgb->a[1];
    pic->v     = imgb->a[2];

    pic->w_l   = imgb->w[0];
    pic->h_l   = imgb->h[0];
    pic->w_c   = imgb->w[1];
    pic->h_c   = imgb->h[1];

    pic->s_l   = STRIDE_IMGB2PIC(imgb->s[0]);
    pic->s_c   = STRIDE_IMGB2PIC(imgb->s[1]);

    pic->pad_l = pad_l;
    pic->pad_c = pad_c;

    pic->imgb  = imgb;

    /* allocate maps */
    w_scu = (pic->w_l + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
    h_scu = (pic->h_l + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
    f_scu = w_scu * h_scu;

    size = sizeof(s8) * f_scu * REFP_NUM;
    pic->map_refi = xeve_malloc_fast(size);
    xeve_assert_gv(pic->map_refi, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    xeve_mset_x64a(pic->map_refi, -1, size);

    size = sizeof(s16) * f_scu * REFP_NUM * MV_D;
    pic->map_mv = xeve_malloc_fast(size);
    xeve_assert_gv(pic->map_mv, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    xeve_mset_x64a(pic->map_mv, 0, size);

    size = sizeof(s16) * f_scu * REFP_NUM * MV_D;
    pic->map_unrefined_mv = xeve_malloc_fast(size);
    xeve_assert_gv(pic->map_unrefined_mv, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    xeve_mset_x64a(pic->map_unrefined_mv, 0, size);

    if(err)
    {
        *err = XEVE_OK;
    }
    return pic;

ERR:
    if(pic)
    {
        xeve_mfree(pic->map_mv);
        xeve_mfree(pic->map_unrefined_mv);
        xeve_mfree(pic->map_refi);
        xeve_mfree(pic->map_dqp_lah);
        xeve_mfree(pic);
    }
    if(err) *err = ret;
    return NULL;
}

void xeve_picbuf_free(XEVE_PIC *pic)
{
    XEVE_IMGB *imgb;

    if(pic)
    {
        imgb = pic->imgb;

        if(imgb)
        {
            imgb->release(imgb);

            pic->y = NULL;
            pic->u = NULL;
            pic->v = NULL;
            pic->w_l = 0;
            pic->h_l = 0;
            pic->w_c = 0;
            pic->h_c = 0;
            pic->s_l = 0;
            pic->s_c = 0;
        }
        xeve_mfree(pic->map_mv);
        xeve_mfree(pic->map_unrefined_mv);
        xeve_mfree(pic->map_refi);
        xeve_mfree(pic->map_dqp_lah);
        xeve_mfree(pic);
    }
}

static void picbuf_expand(pel *a, int s, int w, int h, int exp)
{
    int i, j;
    pel pixel;
    pel *src, *dst;

    /* left */
    src = a;
    dst = a - exp;

    for(i = 0; i < h; i++)
    {
        pixel = *src; /* get boundary pixel */
        for(j = 0; j < exp; j++)
        {
            dst[j] = pixel;
        }
        dst += s;
        src += s;
    }

    /* right */
    src = a + (w - 1);
    dst = a + w;

    for(i = 0; i < h; i++)
    {
        pixel = *src; /* get boundary pixel */
        for(j = 0; j < exp; j++)
        {
            dst[j] = pixel;
        }
        dst += s;
        src += s;
    }

    /* upper */
    src = a - exp;
    dst = a - exp - (exp * s);

    for(i = 0; i < exp; i++)
    {
        xeve_mcpy(dst, src, s*sizeof(pel));
        dst += s;
    }

    /* below */
    src = a + ((h - 1)*s) - exp;
    dst = a + ((h - 1)*s) - exp + s;

    for(i = 0; i < exp; i++)
    {
        xeve_mcpy(dst, src, s*sizeof(pel));
        dst += s;
    }
}


void xeve_picbuf_expand(XEVE_PIC *pic, int exp_l, int exp_c, int chroma_format_idc)
{
    picbuf_expand(pic->y, pic->s_l, pic->w_l, pic->h_l, exp_l);
    if(chroma_format_idc)
    {
        picbuf_expand(pic->u, pic->s_c, pic->w_c, pic->h_c, exp_c);
        picbuf_expand(pic->v, pic->s_c, pic->w_c, pic->h_c, exp_c);
    }
}

void xeve_poc_derivation(XEVE_SPS sps, int tid, XEVE_POC *poc)
{
    int sub_gop_length = (int)pow(2.0, sps.log2_sub_gop_length);
    int expected_tid = 0;
    int doc_offset, poc_offset;

    if (tid == 0)
    {
        poc->poc_val = poc->prev_poc_val + sub_gop_length;
        poc->prev_doc_offset = 0;
        poc->prev_poc_val = poc->poc_val;
        return;
    }
    doc_offset = (poc->prev_doc_offset + 1) % sub_gop_length;
    if (doc_offset == 0)
    {
        poc->prev_poc_val += sub_gop_length;
    }
    else
    {
        expected_tid = 1 + (int)log2(doc_offset);
    }
    while (tid != expected_tid)
    {
        doc_offset = (doc_offset + 1) % sub_gop_length;
        if (doc_offset == 0)
        {
            expected_tid = 0;
        }
        else
        {
            expected_tid = 1 + (int)log2(doc_offset);
        }
    }
    poc_offset = (int)(sub_gop_length * ((2.0 * doc_offset + 1) / (int)pow(2.0, tid) - 2));
    poc->poc_val = poc->prev_poc_val + poc_offset;
    poc->prev_doc_offset = doc_offset;
}

void xeve_picbuf_rc_free(XEVE_PIC *pic)
{
    XEVE_IMGB *imgb;

    if (pic)
    {
        imgb = pic->imgb;

        if (imgb)
        {
            imgb->release(imgb);

            pic->y = NULL;
            pic->u = NULL;
            pic->v = NULL;
            pic->w_l = 0;
            pic->h_l = 0;
            pic->w_c = 0;
            pic->h_c = 0;
            pic->s_l = 0;
            pic->s_c = 0;
        }

        xeve_mfree(pic);
    }
}



void xeve_check_motion_availability(int scup, int cuw, int cuh, int w_scu, int h_scu, int neb_addr[MAX_NUM_POSSIBLE_SCAND], int valid_flag[MAX_NUM_POSSIBLE_SCAND], u32* map_scu, u16 avail_lr, int num_mvp, int is_ibc, u8* map_tidx)
{
    int dx = 0;
    int dy = 0;

    int x_scu = scup % w_scu;
    int y_scu = scup / w_scu;
    int scuw = cuw >> MIN_CU_LOG2;
    int scuh = cuh >> MIN_CU_LOG2;
    xeve_mset(valid_flag, 0, 5 * sizeof(int));

    if (avail_lr == LR_11)
    {
        neb_addr[0] = scup + (scuh - 1) * w_scu - 1; // H
        neb_addr[1] = scup + (scuh - 1) * w_scu + scuw; // inverse H
        neb_addr[2] = scup - w_scu;

        if (is_ibc)
        {
            valid_flag[0] = (x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[0]]) && MCU_GET_IBC(map_scu[neb_addr[0]]) &&
                            (map_tidx[scup] == map_tidx[neb_addr[0]]));
            valid_flag[1] = (x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[1]]) && MCU_GET_IBC(map_scu[neb_addr[1]]) &&
                            (map_tidx[scup] == map_tidx[neb_addr[1]]));
            valid_flag[2] = (y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[2]]) && MCU_GET_IBC(map_scu[neb_addr[2]]) &&
                            (map_tidx[scup] == map_tidx[neb_addr[2]]));
        }
        else
        {
            valid_flag[0] = (x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[0]]) && !MCU_GET_IF(map_scu[neb_addr[0]]) && !MCU_GET_IBC(map_scu[neb_addr[0]]) &&
                            (map_tidx[scup] == map_tidx[neb_addr[0]]));
            valid_flag[1] = (x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[1]]) && !MCU_GET_IF(map_scu[neb_addr[1]]) && !MCU_GET_IBC(map_scu[neb_addr[1]]) &&
                            (map_tidx[scup] == map_tidx[neb_addr[1]]));
            valid_flag[2] = (y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[2]]) && !MCU_GET_IF(map_scu[neb_addr[2]]) && !MCU_GET_IBC(map_scu[neb_addr[2]]) &&
                            (map_tidx[scup] == map_tidx[neb_addr[2]]));
        }

        if (num_mvp == 1)
        {
            neb_addr[3] = scup - w_scu + scuw;
            neb_addr[4] = scup - w_scu - 1;

            if (is_ibc)
            {
                valid_flag[3] = (y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[3]]) && MCU_GET_IBC(map_scu[neb_addr[3]]) &&
                                (map_tidx[scup] == map_tidx[neb_addr[3]]));
                valid_flag[4] = (x_scu > 0 && y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[4]]) && MCU_GET_IBC(map_scu[neb_addr[4]]) &&
                                (map_tidx[scup] == map_tidx[neb_addr[4]]));
            }
            else
            {
                valid_flag[3] = (y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[3]]) && !MCU_GET_IF(map_scu[neb_addr[3]]) && !MCU_GET_IBC(map_scu[neb_addr[3]])&&
                                (map_tidx[scup] == map_tidx[neb_addr[3]]));
                valid_flag[4] = (x_scu > 0 && y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[4]]) && !MCU_GET_IF(map_scu[neb_addr[4]]) && !MCU_GET_IBC(map_scu[neb_addr[4]]) &&
                                (map_tidx[scup] == map_tidx[neb_addr[4]]));
            }
        }
    }
    else if (avail_lr == LR_01)
    {
        neb_addr[0] = scup + (scuh - 1) * w_scu + scuw; // inverse H
        neb_addr[1] = scup - w_scu; // inverse D
        neb_addr[2] = scup - w_scu - 1;  // inverse E

        if (is_ibc)
        {
          valid_flag[0] = (x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[0]]) && MCU_GET_IBC(map_scu[neb_addr[0]]) &&
                          (map_tidx[scup] == map_tidx[neb_addr[0]]));
          valid_flag[1] = (y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[1]]) && MCU_GET_IBC(map_scu[neb_addr[1]]) &&
                          (map_tidx[scup] == map_tidx[neb_addr[1]]));
          valid_flag[2] = (y_scu > 0 && x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[2]]) && MCU_GET_IBC(map_scu[neb_addr[2]]) &&
                          (map_tidx[scup] == map_tidx[neb_addr[2]]));
        }
        else
        {
          valid_flag[0] = (x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[0]]) && !MCU_GET_IF(map_scu[neb_addr[0]]) && !MCU_GET_IBC(map_scu[neb_addr[0]]) &&
                          (map_tidx[scup] == map_tidx[neb_addr[0]]));
          valid_flag[1] = (y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[1]]) && !MCU_GET_IF(map_scu[neb_addr[1]]) && !MCU_GET_IBC(map_scu[neb_addr[1]]) &&
                          (map_tidx[scup] == map_tidx[neb_addr[1]]));
          valid_flag[2] = (y_scu > 0 && x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[2]]) && !MCU_GET_IF(map_scu[neb_addr[2]]) && !MCU_GET_IBC(map_scu[neb_addr[2]]) &&
                          (map_tidx[scup] == map_tidx[neb_addr[2]]));
        }

        if (num_mvp == 1)
        {
            neb_addr[3] = scup + scuh * w_scu + scuw; // inverse I
            neb_addr[4] = scup - w_scu + scuw; // inverse A

            if (is_ibc)
            {
              valid_flag[3] = (y_scu + scuh < h_scu && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[3]]) && MCU_GET_IBC(map_scu[neb_addr[3]]) &&
                              (map_tidx[scup] == map_tidx[neb_addr[3]]));
              valid_flag[4] = (y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[4]]) && MCU_GET_IBC(map_scu[neb_addr[4]]) &&
                              (map_tidx[scup] == map_tidx[neb_addr[4]]));
            }
            else
            {
              valid_flag[3] = (y_scu + scuh < h_scu && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[3]]) && !MCU_GET_IF(map_scu[neb_addr[3]]) && !MCU_GET_IBC(map_scu[neb_addr[3]]) &&
                              (map_tidx[scup] == map_tidx[neb_addr[3]]));
              valid_flag[4] = (y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[4]]) && !MCU_GET_IF(map_scu[neb_addr[4]]) && !MCU_GET_IBC(map_scu[neb_addr[4]]) &&
                              (map_tidx[scup] == map_tidx[neb_addr[4]]));
            }
        }
    }
    else
    {
        neb_addr[0] = scup + (scuh - 1) * w_scu - 1; // H
        neb_addr[1] = scup - w_scu + scuw - 1; // D
        neb_addr[2] = scup - w_scu + scuw;  // E

        if (is_ibc)
        {
          valid_flag[0] = (x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[0]]) && MCU_GET_IBC(map_scu[neb_addr[0]]) &&
                          (map_tidx[scup] == map_tidx[neb_addr[0]]));
          valid_flag[1] = (y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[1]]) && MCU_GET_IBC(map_scu[neb_addr[1]]) &&
                          (map_tidx[scup] == map_tidx[neb_addr[1]]));
          valid_flag[2] = (y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[2]]) && MCU_GET_IBC(map_scu[neb_addr[2]]) &&
                          (map_tidx[scup] == map_tidx[neb_addr[2]]));
        }
        else
        {
          valid_flag[0] = (x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[0]]) && !MCU_GET_IF(map_scu[neb_addr[0]]) && !MCU_GET_IBC(map_scu[neb_addr[0]]) &&
                          (map_tidx[scup] == map_tidx[neb_addr[0]])
          );
          valid_flag[1] = (y_scu > 0 && MCU_GET_COD(map_scu[neb_addr[1]]) && !MCU_GET_IF(map_scu[neb_addr[1]]) && !MCU_GET_IBC(map_scu[neb_addr[1]]) &&
                          (map_tidx[scup] == map_tidx[neb_addr[1]]));
          valid_flag[2] = (y_scu > 0 && x_scu + scuw < w_scu && MCU_GET_COD(map_scu[neb_addr[2]]) && !MCU_GET_IF(map_scu[neb_addr[2]]) && !MCU_GET_IBC(map_scu[neb_addr[2]]) &&
                          (map_tidx[scup] == map_tidx[neb_addr[2]]));
        }

        if (num_mvp == 1)
        {
            neb_addr[3] = scup + scuh * w_scu - 1; // I
            neb_addr[4] = scup - w_scu - 1; // A

            if (is_ibc)
            {
              valid_flag[3] = (y_scu + scuh < h_scu && x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[3]]) && MCU_GET_IBC(map_scu[neb_addr[3]]) &&
                              (map_tidx[scup] == map_tidx[neb_addr[3]]));
              valid_flag[4] = (y_scu > 0 && x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[4]]) && MCU_GET_IBC(map_scu[neb_addr[4]]) &&
                              (map_tidx[scup] == map_tidx[neb_addr[4]]));
            }
            else
            {
              valid_flag[3] = (y_scu + scuh < h_scu && x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[3]]) && !MCU_GET_IF(map_scu[neb_addr[3]]) && !MCU_GET_IBC(map_scu[neb_addr[3]]) &&
                              (map_tidx[scup] == map_tidx[neb_addr[3]]));
              valid_flag[4] = (y_scu > 0 && x_scu > 0 && MCU_GET_COD(map_scu[neb_addr[4]]) && !MCU_GET_IF(map_scu[neb_addr[4]]) && !MCU_GET_IBC(map_scu[neb_addr[4]]) &&
                              (map_tidx[scup] == map_tidx[neb_addr[4]]));
            }
        }
    }
}

s8 xeve_get_first_refi(int scup, int lidx, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], int cuw, int cuh, int w_scu, int h_scu, u32 *map_scu, u8 mvr_idx, u16 avail_lr
                     , s16(*map_unrefined_mv)[REFP_NUM][MV_D], u8* map_tidx)
{
    int neb_addr[MAX_NUM_POSSIBLE_SCAND], valid_flag[MAX_NUM_POSSIBLE_SCAND];
    s8  refi = 0, default_refi;
    s16 default_mv[MV_D];

    xeve_check_motion_availability(scup, cuw, cuh, w_scu, h_scu, neb_addr, valid_flag, map_scu, avail_lr, 1, 0, map_tidx);
    xeve_get_default_motion(neb_addr, valid_flag, 0, lidx, map_refi, map_mv, &default_refi, default_mv, map_scu, map_unrefined_mv, scup, w_scu);

    assert(mvr_idx < 5);
    //neb-position is coupled with mvr index
    if (valid_flag[mvr_idx])
    {
        refi = REFI_IS_VALID(map_refi[neb_addr[mvr_idx]][lidx]) ? map_refi[neb_addr[mvr_idx]][lidx] : default_refi;
    }
    else
    {
        refi = default_refi;
    }

    return refi;
}

int xeve_get_default_motion(int neb_addr[MAX_NUM_POSSIBLE_SCAND], int valid_flag[MAX_NUM_POSSIBLE_SCAND], s8 cur_refi, int lidx, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], s8 *refi, s16 mv[MV_D]
                           , u32 *map_scu, s16(*map_unrefined_mv)[REFP_NUM][MV_D], int scup, int w_scu)
{
    int k;
    int found = 0;
    s8  tmp_refi = 0;

    *refi = 0;
    mv[MV_X] = 0;
    mv[MV_Y] = 0;

    for (k = 0; k < 2; k++)
    {
        if(valid_flag[k])
        {
            tmp_refi = REFI_IS_VALID(map_refi[neb_addr[k]][lidx]) ? map_refi[neb_addr[k]][lidx] : REFI_INVALID;
            if(tmp_refi == cur_refi)
            {
                found = 1;
                *refi = tmp_refi;
                if (MCU_GET_DMVRF(map_scu[neb_addr[k]]))
                {
                    mv[MV_X] = map_unrefined_mv[neb_addr[k]][lidx][MV_X];
                    mv[MV_Y] = map_unrefined_mv[neb_addr[k]][lidx][MV_Y];
                }
                else
                {
                mv[MV_X] = map_mv[neb_addr[k]][lidx][MV_X];
                mv[MV_Y] = map_mv[neb_addr[k]][lidx][MV_Y];
                }
                break;
            }
        }
    }

    if(!found)
    {
        for (k = 0; k < 2; k++)
        {
            if(valid_flag[k])
            {
                tmp_refi = REFI_IS_VALID(map_refi[neb_addr[k]][lidx]) ? map_refi[neb_addr[k]][lidx] : REFI_INVALID;
                if(tmp_refi != REFI_INVALID)
                {
                    found = 1;
                    *refi = tmp_refi;
                    if(MCU_GET_DMVRF(map_scu[neb_addr[k]]))
                    {
                        mv[MV_X] = map_unrefined_mv[neb_addr[k]][lidx][MV_X];
                        mv[MV_Y] = map_unrefined_mv[neb_addr[k]][lidx][MV_Y];
                    }
                    else
                    {
                        mv[MV_X] = map_mv[neb_addr[k]][lidx][MV_X];
                        mv[MV_Y] = map_mv[neb_addr[k]][lidx][MV_Y];
                    }
                    break;
                }
            }
        }
    }

    return found;
}

void xeve_get_motion(int scup, int lidx, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], XEVE_REFP(*refp)[REFP_NUM],
                    int cuw, int cuh, int w_scu, u16 avail, s8 refi[MAX_NUM_MVP], s16 mvp[MAX_NUM_MVP][MV_D])
{

    if (IS_AVAIL(avail, AVAIL_LE))
    {
        refi[0] = 0;
        mvp[0][MV_X] = map_mv[scup - 1][lidx][MV_X];
        mvp[0][MV_Y] = map_mv[scup - 1][lidx][MV_Y];
    }
    else
    {
        refi[0] = 0;
        mvp[0][MV_X] = 1;
        mvp[0][MV_Y] = 1;
    }

    if (IS_AVAIL(avail, AVAIL_UP))
    {
        refi[1] = 0;
        mvp[1][MV_X] = map_mv[scup - w_scu][lidx][MV_X];
        mvp[1][MV_Y] = map_mv[scup - w_scu][lidx][MV_Y];
    }
    else
    {
        refi[1] = 0;
        mvp[1][MV_X] = 1;
        mvp[1][MV_Y] = 1;
    }

    if (IS_AVAIL(avail, AVAIL_UP_RI))
    {
        refi[2] = 0;
        mvp[2][MV_X] = map_mv[scup - w_scu + (cuw >> MIN_CU_LOG2)][lidx][MV_X];
        mvp[2][MV_Y] = map_mv[scup - w_scu + (cuw >> MIN_CU_LOG2)][lidx][MV_Y];
    }
    else
    {
        refi[2] = 0;
        mvp[2][MV_X] = 1;
        mvp[2][MV_Y] = 1;
    }
    refi[3] = 0;
    mvp[3][MV_X] = refp[0][lidx].map_mv[scup][0][MV_X];
    mvp[3][MV_Y] = refp[0][lidx].map_mv[scup][0][MV_Y];
}

BOOL check_bi_applicability(int slice_type, int cuw, int cuh, int is_sps_admvp)
{
    BOOL is_applicable = FALSE;

    if (slice_type == SLICE_B)
    {
        if(!is_sps_admvp || cuw + cuh > 12)
        {
            is_applicable = TRUE;
        }
    }

    return is_applicable;
}

void xeve_get_motion_skip(int slice_type, int scup, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], XEVE_REFP refp[REFP_NUM], int cuw, int cuh, int w_scu
                        , s8 refi[REFP_NUM][MAX_NUM_MVP], s16 mvp[REFP_NUM][MAX_NUM_MVP][MV_D], u16 avail_lr)
{
    xeve_mset(mvp, 0, MAX_NUM_MVP * REFP_NUM * MV_D * sizeof(s16));
    xeve_mset(refi, REFI_INVALID, MAX_NUM_MVP * REFP_NUM * sizeof(s8));
    xeve_get_motion(scup, REFP_0, map_refi, map_mv, (XEVE_REFP(*)[2])refp, cuw, cuh, w_scu, avail_lr, refi[REFP_0], mvp[REFP_0]);
    if (slice_type == SLICE_B)
    {
        xeve_get_motion(scup, REFP_1, map_refi, map_mv, (XEVE_REFP(*)[2])refp, cuw, cuh, w_scu, avail_lr, refi[REFP_1], mvp[REFP_1]);
    }
}

void xeve_get_mv_dir(XEVE_REFP refp[REFP_NUM], u32 poc, int scup, int c_scu, u16 w_scu, u16 h_scu, s16 mvp[REFP_NUM][MV_D], int sps_admvp_flag)
{
    s16 mvc[MV_D];
    int dpoc_co, dpoc_L0, dpoc_L1;

    mvc[MV_X] = refp[REFP_1].map_mv[scup][0][MV_X];
    mvc[MV_Y] = refp[REFP_1].map_mv[scup][0][MV_Y];

    dpoc_co = refp[REFP_1].poc - refp[REFP_1].list_poc[0];
    dpoc_L0 = poc - refp[REFP_0].poc;
    dpoc_L1 = refp[REFP_1].poc - poc;

    if(dpoc_co == 0)
    {
        mvp[REFP_0][MV_X] = 0;
        mvp[REFP_0][MV_Y] = 0;
        mvp[REFP_1][MV_X] = 0;
        mvp[REFP_1][MV_Y] = 0;
    }
    else
    {
        mvp[REFP_0][MV_X] = dpoc_L0 * mvc[MV_X] / dpoc_co;
        mvp[REFP_0][MV_Y] = dpoc_L0 * mvc[MV_Y] / dpoc_co;
        mvp[REFP_1][MV_X] = -dpoc_L1 * mvc[MV_X] / dpoc_co;
        mvp[REFP_1][MV_Y] = -dpoc_L1 * mvc[MV_Y] / dpoc_co;
    }
}

int xeve_get_avail_cu(int neb_scua[MAX_NEB2], u32 * map_cu, u8* map_tidx)
{
    int slice_num_x;
    u16 avail_cu = 0;

    xeve_assert(neb_scua[NEB_X] >= 0);

    slice_num_x = MCU_GET_SN(map_cu[neb_scua[NEB_X]]);

    /* left */
    if(neb_scua[NEB_A] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_A]])) &&
       (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_A]]))
    {
        avail_cu |= AVAIL_LE;
    }
    /* up */
    if(neb_scua[NEB_B] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_B]])) &&
       (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_B]]))
    {
        avail_cu |= AVAIL_UP;
    }
    /* up-right */
    if(neb_scua[NEB_C] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_C]])) &&
       (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_C]]))
    {
        if(MCU_GET_COD(map_cu[neb_scua[NEB_C]]))
        {
            avail_cu |= AVAIL_UP_RI;
        }
    }
    /* up-left */
    if(neb_scua[NEB_D] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_D]])) &&
       (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_D]]))
    {
        avail_cu |= AVAIL_UP_LE;
    }
    /* low-left */
    if(neb_scua[NEB_E] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_E]])) &&
       (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_E]]))
    {
        if(MCU_GET_COD(map_cu[neb_scua[NEB_E]]))
        {
            avail_cu |= AVAIL_LO_LE;
        }
    }
    /* right */
    if(neb_scua[NEB_H] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_H]])) &&
       (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_H]]))
    {
        avail_cu |= AVAIL_RI;
    }
    /* low-right */
    if(neb_scua[NEB_I] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_I]])) &&
       (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_I]]))
    {
        if(MCU_GET_COD(map_cu[neb_scua[NEB_I]]))
        {
            avail_cu |= AVAIL_LO_RI;
        }
    }

    return avail_cu;
}

u16 xeve_get_avail_inter(int x_scu, int y_scu, int w_scu, int h_scu, int scup, int cuw, int cuh, u32 * map_scu, u8* map_tidx)
{
    u16 avail = 0;
    int scuw = cuw >> MIN_CU_LOG2;
    int scuh = cuh >> MIN_CU_LOG2;
    int curr_scup = x_scu + y_scu * w_scu;

    if(x_scu > 0 && !MCU_GET_IF(map_scu[scup - 1]) && MCU_GET_COD(map_scu[scup - 1]) &&
       (map_tidx[curr_scup] == map_tidx[scup - 1]) && !MCU_GET_IBC(map_scu[scup - 1]))
    {
        SET_AVAIL(avail, AVAIL_LE);

        if(y_scu + scuh < h_scu  && MCU_GET_COD(map_scu[scup + (scuh * w_scu) - 1]) && !MCU_GET_IF(map_scu[scup + (scuh * w_scu) - 1]) &&
           (map_tidx[curr_scup] == map_tidx[scup + (scuh * w_scu) - 1]) && !MCU_GET_IBC(map_scu[scup + (scuh * w_scu) - 1]) )
        {
            SET_AVAIL(avail, AVAIL_LO_LE);
        }
    }

    if(y_scu > 0)
    {
        if(!MCU_GET_IF(map_scu[scup - w_scu]) && (map_tidx[curr_scup] == map_tidx[scup - w_scu]) && !MCU_GET_IBC(map_scu[scup - w_scu]))
        {
            SET_AVAIL(avail, AVAIL_UP);
        }

        if(!MCU_GET_IF(map_scu[scup - w_scu + scuw - 1]) && (map_tidx[curr_scup] == map_tidx[scup - w_scu + scuw - 1]) &&
           !MCU_GET_IBC(map_scu[scup - w_scu + scuw - 1]))
        {
            SET_AVAIL(avail, AVAIL_RI_UP);
        }

        if(x_scu > 0 && !MCU_GET_IF(map_scu[scup - w_scu - 1]) && MCU_GET_COD(map_scu[scup - w_scu - 1]) && (map_tidx[curr_scup] == map_tidx[scup - w_scu - 1]) &&
           !MCU_GET_IBC(map_scu[scup - w_scu - 1]) )
        {
            SET_AVAIL(avail, AVAIL_UP_LE);
        }

        if(x_scu + scuw < w_scu  && MCU_IS_COD_NIF(map_scu[scup - w_scu + scuw]) && MCU_GET_COD(map_scu[scup - w_scu + scuw]) &&
           (map_tidx[curr_scup] == map_tidx[scup - w_scu + scuw]))
        {
            SET_AVAIL(avail, AVAIL_UP_RI);
        }
    }

    if(x_scu + scuw < w_scu && !MCU_GET_IF(map_scu[scup + scuw]) && MCU_GET_COD(map_scu[scup + scuw]) && (map_tidx[curr_scup] == map_tidx[scup + scuw]) &&
       !MCU_GET_IBC(map_scu[scup + scuw]) )
    {
        SET_AVAIL(avail, AVAIL_RI);

        if(y_scu + scuh < h_scu  && MCU_GET_COD(map_scu[scup + (scuh * w_scu) + scuw]) && !MCU_GET_IF(map_scu[scup + (scuh * w_scu) + scuw]) &&
           (map_tidx[curr_scup] == map_tidx[scup + (scuh * w_scu) + scuw]) && !MCU_GET_IBC(map_scu[scup + (scuh * w_scu) + scuw]) )
        {
            SET_AVAIL(avail, AVAIL_LO_RI);
        }
    }

    return avail;
}

u16 xeve_get_avail_intra(int x_scu, int y_scu, int w_scu, int h_scu, int scup, int log2_cuw, int log2_cuh, u32 * map_scu, u8* map_tidx)
{
    u16 avail = 0;
    int log2_scuw, log2_scuh, scuw, scuh;

    log2_scuw = log2_cuw - MIN_CU_LOG2;
    log2_scuh = log2_cuh - MIN_CU_LOG2;
    scuw = 1 << log2_scuw;
    scuh = 1 << log2_scuh;
    int curr_scup = x_scu + y_scu * w_scu;

    if(x_scu > 0 && MCU_GET_COD(map_scu[scup - 1]) && map_tidx[curr_scup] == map_tidx[scup - 1])
    {
        SET_AVAIL(avail, AVAIL_LE);

        if(y_scu + scuh + scuw - 1 < h_scu  && MCU_GET_COD(map_scu[scup + (w_scu * (scuw + scuh)) - w_scu - 1]) &&
           (map_tidx[curr_scup] == map_tidx[scup + (w_scu * (scuw + scuh)) - w_scu - 1]))
        {
            SET_AVAIL(avail, AVAIL_LO_LE);
        }
    }

    if(y_scu > 0)
    {
        if (map_tidx[scup] == map_tidx[scup - w_scu])
        {
            SET_AVAIL(avail, AVAIL_UP);
        }
        if (map_tidx[scup] == map_tidx[scup - w_scu + scuw - 1])
        {
            SET_AVAIL(avail, AVAIL_RI_UP);
        }

        if(x_scu > 0 && MCU_GET_COD(map_scu[scup - w_scu - 1]) && (map_tidx[curr_scup] == map_tidx[scup - w_scu - 1]))
        {
            SET_AVAIL(avail, AVAIL_UP_LE);
        }

        if(x_scu + scuw < w_scu  && MCU_GET_COD(map_scu[scup - w_scu + scuw]) && (map_tidx[curr_scup] == map_tidx[scup - w_scu + scuw]))
        {
            SET_AVAIL(avail, AVAIL_UP_RI);
        }
    }

    if(x_scu + scuw < w_scu && MCU_GET_COD(map_scu[scup + scuw]) && (map_tidx[curr_scup] == map_tidx[scup + scuw]))
    {
        SET_AVAIL(avail, AVAIL_RI);

        if(y_scu + scuh + scuw - 1 < h_scu  && MCU_GET_COD(map_scu[scup + (w_scu * (scuw + scuh - 1)) + scuw]) &&
           (map_tidx[curr_scup] == map_tidx[scup + (w_scu * (scuw + scuh - 1)) + scuw]))
        {
            SET_AVAIL(avail, AVAIL_LO_RI);
        }
    }

    return avail;
}

/******************************************************************************
 * alloc sub-picture only for luma
 ******************************************************************************/
XEVE_PIC * xeve_alloc_spic_l(int w, int h)
{
    XEVE_PIC * pic = NULL;
    XEVE_IMGB * imgb = NULL;
    int ret, opt, align[XEVE_IMGB_MAX_PLANE], pad[XEVE_IMGB_MAX_PLANE];
    int w_scu, h_scu, f_scu;

    /* make half-size for sub-pic allocation */
    w >>= 1;
    h >>= 1;

    /* allocate PIC structure */
    pic = xeve_malloc(sizeof(XEVE_PIC));
    xeve_assert_gv(pic != NULL, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    opt = XEVE_IMGB_OPT_NONE;

    /* set align value*/
    align[0] = MIN_CU_SIZE;
    align[1] = MIN_CU_SIZE;
    align[2] = MIN_CU_SIZE;

    /* set padding value*/
    pad[0] = 32;
    pad[1] = 0;
    pad[2] = 0;

    imgb = xeve_imgb_create(w, h, XEVE_CS_YCBCR420_10LE, opt, pad, align);
    imgb->cs = XEVE_CS_YCBCR420_10LE;

    xeve_assert_gv(imgb != NULL, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

    /* set XEVE_PIC */
    /* allocate maps */
    w_scu = (pic->w_l + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
    h_scu = (pic->h_l + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
    f_scu = w_scu * h_scu;

    /* set XEVE_PIC */
    pic->buf_y = imgb->baddr[0];
    pic->y       = imgb->a[0];
    pic->w_l   = imgb->w[0];
    pic->h_l   = imgb->h[0];
    pic->s_l   = STRIDE_IMGB2PIC(imgb->s[0]);
    pic->pad_l = pad[0];

    /* don't use chroma &*/
    pic->buf_u = NULL;
    pic->buf_v = NULL;
    pic->u       = NULL;
    pic->v       = NULL;
    pic->w_c   = 0;
    pic->s_c   = 0;
    pic->h_c   = 0;
    pic->pad_c = 0;

    pic->imgb = imgb;
    return pic;

ERR:
    if(pic) xeve_mfree(pic);
    return NULL;
}

/******************************************************************************
 * generate sub-picture
 ******************************************************************************/
void arace_gen_subpic(void * src_y, void * dst_y, int w, int h, int s_s,
    int d_s, int bit_depth)
{
    /* source bottom and top top */
    u8 * src_b, * src_t, * dst;
    int     x, k, y;

    /* top source */
    src_t = (u8 *)src_y;
    /* bottom source */
    src_b = src_t + s_s;
    dst   = (u8 *)dst_y;

    for(y = 0; y < h; y++)
    {
        for(x = 0; x < w; x++)
        {
            k =  x << 1;
            dst[x] = (src_t[k] + src_b[k] + src_t[k+1] + src_b[k+1] + 2) >> 2;
        }

        src_t += (s_s << 1);
        src_b += (s_s << 1);
        dst   += d_s;
    }
}


int xeve_picbuf_signature(XEVE_PIC *pic, u8 signature[N_C][16])
{
    return xeve_md5_imgb(pic->imgb, signature);
}

/* MD5 functions */
#define MD5FUNC(f, w, x, y, z, msg1, s,msg2 )  ( w += f(x, y, z) + msg1 + msg2,  w = w<<s | w>>(32-s),  w += x )
#define FF(x, y, z) (z ^ (x & (y ^ z)))
#define GG(x, y, z) (y ^ (z & (x ^ y)))
#define HH(x, y, z) (x ^ y ^ z)
#define II(x, y, z) (y ^ (x | ~z))

static void xeve_md5_trans(u32 *buf, u32 *msg)
{
    register u32 a, b, c, d;

    a = buf[0];
    b = buf[1];
    c = buf[2];
    d = buf[3];

    MD5FUNC(FF, a, b, c, d, msg[ 0],  7, 0xd76aa478); /* 1 */
    MD5FUNC(FF, d, a, b, c, msg[ 1], 12, 0xe8c7b756); /* 2 */
    MD5FUNC(FF, c, d, a, b, msg[ 2], 17, 0x242070db); /* 3 */
    MD5FUNC(FF, b, c, d, a, msg[ 3], 22, 0xc1bdceee); /* 4 */

    MD5FUNC(FF, a, b, c, d, msg[ 4],  7, 0xf57c0faf); /* 5 */
    MD5FUNC(FF, d, a, b, c, msg[ 5], 12, 0x4787c62a); /* 6 */
    MD5FUNC(FF, c, d, a, b, msg[ 6], 17, 0xa8304613); /* 7 */
    MD5FUNC(FF, b, c, d, a, msg[ 7], 22, 0xfd469501); /* 8 */

    MD5FUNC(FF, a, b, c, d, msg[ 8],  7, 0x698098d8); /* 9 */
    MD5FUNC(FF, d, a, b, c, msg[ 9], 12, 0x8b44f7af); /* 10 */
    MD5FUNC(FF, c, d, a, b, msg[10], 17, 0xffff5bb1); /* 11 */
    MD5FUNC(FF, b, c, d, a, msg[11], 22, 0x895cd7be); /* 12 */

    MD5FUNC(FF, a, b, c, d, msg[12],  7, 0x6b901122); /* 13 */
    MD5FUNC(FF, d, a, b, c, msg[13], 12, 0xfd987193); /* 14 */
    MD5FUNC(FF, c, d, a, b, msg[14], 17, 0xa679438e); /* 15 */
    MD5FUNC(FF, b, c, d, a, msg[15], 22, 0x49b40821); /* 16 */

    /* Round 2 */
    MD5FUNC(GG, a, b, c, d, msg[ 1],  5, 0xf61e2562); /* 17 */
    MD5FUNC(GG, d, a, b, c, msg[ 6],  9, 0xc040b340); /* 18 */
    MD5FUNC(GG, c, d, a, b, msg[11], 14, 0x265e5a51); /* 19 */
    MD5FUNC(GG, b, c, d, a, msg[ 0], 20, 0xe9b6c7aa); /* 20 */

    MD5FUNC(GG, a, b, c, d, msg[ 5],  5, 0xd62f105d); /* 21 */
    MD5FUNC(GG, d, a, b, c, msg[10],  9,  0x2441453); /* 22 */
    MD5FUNC(GG, c, d, a, b, msg[15], 14, 0xd8a1e681); /* 23 */
    MD5FUNC(GG, b, c, d, a, msg[ 4], 20, 0xe7d3fbc8); /* 24 */

    MD5FUNC(GG, a, b, c, d, msg[ 9],  5, 0x21e1cde6); /* 25 */
    MD5FUNC(GG, d, a, b, c, msg[14],  9, 0xc33707d6); /* 26 */
    MD5FUNC(GG, c, d, a, b, msg[ 3], 14, 0xf4d50d87); /* 27 */
    MD5FUNC(GG, b, c, d, a, msg[ 8], 20, 0x455a14ed); /* 28 */

    MD5FUNC(GG, a, b, c, d, msg[13],  5, 0xa9e3e905); /* 29 */
    MD5FUNC(GG, d, a, b, c, msg[ 2],  9, 0xfcefa3f8); /* 30 */
    MD5FUNC(GG, c, d, a, b, msg[ 7], 14, 0x676f02d9); /* 31 */
    MD5FUNC(GG, b, c, d, a, msg[12], 20, 0x8d2a4c8a); /* 32 */

    /* Round 3 */
    MD5FUNC(HH, a, b, c, d, msg[ 5],  4, 0xfffa3942); /* 33 */
    MD5FUNC(HH, d, a, b, c, msg[ 8], 11, 0x8771f681); /* 34 */
    MD5FUNC(HH, c, d, a, b, msg[11], 16, 0x6d9d6122); /* 35 */
    MD5FUNC(HH, b, c, d, a, msg[14], 23, 0xfde5380c); /* 36 */

    MD5FUNC(HH, a, b, c, d, msg[ 1],  4, 0xa4beea44); /* 37 */
    MD5FUNC(HH, d, a, b, c, msg[ 4], 11, 0x4bdecfa9); /* 38 */
    MD5FUNC(HH, c, d, a, b, msg[ 7], 16, 0xf6bb4b60); /* 39 */
    MD5FUNC(HH, b, c, d, a, msg[10], 23, 0xbebfbc70); /* 40 */

    MD5FUNC(HH, a, b, c, d, msg[13],  4, 0x289b7ec6); /* 41 */
    MD5FUNC(HH, d, a, b, c, msg[ 0], 11, 0xeaa127fa); /* 42 */
    MD5FUNC(HH, c, d, a, b, msg[ 3], 16, 0xd4ef3085); /* 43 */
    MD5FUNC(HH, b, c, d, a, msg[ 6], 23,  0x4881d05); /* 44 */

    MD5FUNC(HH, a, b, c, d, msg[ 9],  4, 0xd9d4d039); /* 45 */
    MD5FUNC(HH, d, a, b, c, msg[12], 11, 0xe6db99e5); /* 46 */
    MD5FUNC(HH, c, d, a, b, msg[15], 16, 0x1fa27cf8); /* 47 */
    MD5FUNC(HH, b, c, d, a, msg[ 2], 23, 0xc4ac5665); /* 48 */

    /* Round 4 */
    MD5FUNC(II, a, b, c, d, msg[ 0],  6, 0xf4292244); /* 49 */
    MD5FUNC(II, d, a, b, c, msg[ 7], 10, 0x432aff97); /* 50 */
    MD5FUNC(II, c, d, a, b, msg[14], 15, 0xab9423a7); /* 51 */
    MD5FUNC(II, b, c, d, a, msg[ 5], 21, 0xfc93a039); /* 52 */

    MD5FUNC(II, a, b, c, d, msg[12],  6, 0x655b59c3); /* 53 */
    MD5FUNC(II, d, a, b, c, msg[ 3], 10, 0x8f0ccc92); /* 54 */
    MD5FUNC(II, c, d, a, b, msg[10], 15, 0xffeff47d); /* 55 */
    MD5FUNC(II, b, c, d, a, msg[ 1], 21, 0x85845dd1); /* 56 */

    MD5FUNC(II, a, b, c, d, msg[ 8],  6, 0x6fa87e4f); /* 57 */
    MD5FUNC(II, d, a, b, c, msg[15], 10, 0xfe2ce6e0); /* 58 */
    MD5FUNC(II, c, d, a, b, msg[ 6], 15, 0xa3014314); /* 59 */
    MD5FUNC(II, b, c, d, a, msg[13], 21, 0x4e0811a1); /* 60 */

    MD5FUNC(II, a, b, c, d, msg[ 4],  6, 0xf7537e82); /* 61 */
    MD5FUNC(II, d, a, b, c, msg[11], 10, 0xbd3af235); /* 62 */
    MD5FUNC(II, c, d, a, b, msg[ 2], 15, 0x2ad7d2bb); /* 63 */
    MD5FUNC(II, b, c, d, a, msg[ 9], 21, 0xeb86d391); /* 64 */

    buf[0] += a;
    buf[1] += b;
    buf[2] += c;
    buf[3] += d;
}

void xeve_md5_init(XEVE_MD5 *md5)
{
    md5->h[0] = 0x67452301;
    md5->h[1] = 0xefcdab89;
    md5->h[2] = 0x98badcfe;
    md5->h[3] = 0x10325476;

    md5->bits[0] = 0;
    md5->bits[1] = 0;
}

void xeve_md5_update(XEVE_MD5 *md5, void *buf_t, u32 len)
{
    u8 *buf;
    u32 i, idx, part_len;

    buf = (u8*)buf_t;

    idx = (u32)((md5->bits[0] >> 3) & 0x3f);

    md5->bits[0] += (len << 3);
    if(md5->bits[0] < (len << 3))
    {
        (md5->bits[1])++;
    }

    md5->bits[1] += (len >> 29);
    part_len = 64 - idx;

    if(len >= part_len)
    {
        xeve_mcpy(md5->msg + idx, buf, part_len);
        xeve_md5_trans(md5->h, (u32 *)md5->msg);

        for(i = part_len; i + 63 < len; i += 64)
        {
            xeve_md5_trans(md5->h, (u32 *)(buf + i));
        }
        idx = 0;
    }
    else
    {
        i = 0;
    }

    if(len - i > 0)
    {
        xeve_mcpy(md5->msg + idx, buf + i, len - i);
    }
}

void xeve_md5_update_16(XEVE_MD5 *md5, void *buf_t, u32 len)
{
    u16 *buf;
    u32 i, idx, part_len, j;
    u8 t[512];

    buf = (u16 *)buf_t;
    idx = (u32)((md5->bits[0] >> 3) & 0x3f);

    len = len * 2;
    for(j = 0; j < len; j += 2)
    {
        t[j] = (u8)(*(buf));
        t[j + 1] = *(buf) >> 8;
        buf++;
    }

    md5->bits[0] += (len << 3);
    if(md5->bits[0] < (len << 3))
    {
        (md5->bits[1])++;
    }

    md5->bits[1] += (len >> 29);
    part_len = 64 - idx;

    if(len >= part_len)
    {
        xeve_mcpy(md5->msg + idx, t, part_len);
        xeve_md5_trans(md5->h, (u32 *)md5->msg);

        for(i = part_len; i + 63 < len; i += 64)
        {
            xeve_md5_trans(md5->h, (u32 *)(t + i));
        }
        idx = 0;
    }
    else
    {
        i = 0;
    }

    if(len - i > 0)
    {
        xeve_mcpy(md5->msg + idx, t + i, len - i);
    }
}

void xeve_md5_finish(XEVE_MD5 *md5, u8 digest[16])
{
    u8 *pos;
    int cnt;

    cnt = (md5->bits[0] >> 3) & 0x3F;
    pos = md5->msg + cnt;
    *pos++ = 0x80;
    cnt = 64 - 1 - cnt;

    if(cnt < 8)
    {
        xeve_mset(pos, 0, cnt);
        xeve_md5_trans(md5->h, (u32 *)md5->msg);
        xeve_mset(md5->msg, 0, 56);
    }
    else
    {
        xeve_mset(pos, 0, cnt - 8);
    }

    xeve_mcpy((md5->msg + 14 * sizeof(u32)), &md5->bits[0], sizeof(u32));
    xeve_mcpy((md5->msg + 15 * sizeof(u32)), &md5->bits[1], sizeof(u32));

    xeve_md5_trans(md5->h, (u32 *)md5->msg);
    xeve_mcpy(digest, md5->h, 16);
    xeve_mset(md5, 0, sizeof(XEVE_MD5));
}

int xeve_md5_imgb(XEVE_IMGB *imgb, u8 digest[N_C][16])
{
    XEVE_MD5 md5[N_C];
    int i, j;

    for (i = 0; i < imgb->np; i++)
    {
        xeve_md5_init(&md5[i]);

        for (j = 0; j < imgb->ah[i]; j++)
        {
            xeve_md5_update(&md5[i], ((u8 *)imgb->a[i]) + j * imgb->s[i], imgb->aw[i] * 2);
        }

        xeve_md5_finish(&md5[i], digest[i]);
    }

    return XEVE_OK;
}

static void init_scan(u16 *scan, int size_x, int size_y, int scan_type)
{
    int x, y, l, pos, num_line;

    pos = 0;
    num_line = size_x + size_y - 1;

    if(scan_type == COEF_SCAN_ZIGZAG)
    {
        /* starting point */
        scan[pos] = 0;
        pos++;

        /* loop */
        for(l = 1; l < num_line; l++)
        {
            if(l % 2) /* decreasing loop */
            {
                x = XEVE_MIN(l, size_x - 1);
                y = XEVE_MAX(0, l - (size_x - 1));

                while(x >= 0 && y < size_y)
                {
                    scan[pos] = y * size_x + x;
                    pos++;
                    x--;
                    y++;
                }
            }
            else /* increasing loop */
            {
                y = XEVE_MIN(l, size_y - 1);
                x = XEVE_MAX(0, l - (size_y - 1));
                while(y >= 0 && x < size_x)
                {
                    scan[pos] = y * size_x + x;
                    pos++;
                    x++;
                    y--;
                }
            }
        }
    }
}

int xeve_get_split_mode(s8 *split_mode, int cud, int cup, int cuw, int cuh, int lcu_s, s8(*split_mode_buf)[NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU])
{
    int ret = XEVE_OK;
    int pos = cup + (((cuh >> 1) >> MIN_CU_LOG2) * (lcu_s >> MIN_CU_LOG2) + ((cuw >> 1) >> MIN_CU_LOG2));
    int shape = SQUARE + (XEVE_LOG2(cuw) - XEVE_LOG2(cuh));

    if(cuw < 8 && cuh < 8)
    {
        *split_mode = NO_SPLIT;
        return ret;
    }

    *split_mode = split_mode_buf[cud][shape][pos];

    return ret;
}

void xeve_set_split_mode(s8 split_mode, int cud, int cup, int cuw, int cuh, int lcu_s, s8 (*split_mode_buf)[NUM_BLOCK_SHAPE][MAX_CU_CNT_IN_LCU])
{
    int pos = cup + (((cuh >> 1) >> MIN_CU_LOG2) * (lcu_s >> MIN_CU_LOG2) + ((cuw >> 1) >> MIN_CU_LOG2));
    int shape = SQUARE + (XEVE_LOG2(cuw) - XEVE_LOG2(cuh));

    if(cuw >= 8 || cuh >= 8)
        split_mode_buf[cud][shape][pos] = split_mode;
}

u16 xeve_check_nev_avail(int x_scu, int y_scu, int cuw, int cuh, int w_scu, int h_scu, u32 * map_scu, u8* map_tidx)
{
    int scup = y_scu *  w_scu + x_scu;
    int scuw = cuw >> MIN_CU_LOG2;
    u16 avail_lr = 0;
    int curr_scup = x_scu + y_scu * w_scu;

    if(x_scu > 0 && MCU_GET_COD(map_scu[scup - 1]) && (map_tidx[curr_scup] == map_tidx[scup - 1]))
    {
        avail_lr+=1;
    }

    if(x_scu + scuw < w_scu && MCU_GET_COD(map_scu[scup+scuw]) && (map_tidx[curr_scup] == map_tidx[scup + scuw]))
    {
        avail_lr+=2;
    }

    return avail_lr;
}

void xeve_get_ctx_some_flags(int x_scu, int y_scu, int cuw, int cuh, int w_scu, u32* map_scu, u32* map_cu_mode, u8* ctx, u8 slice_type
                          , int sps_cm_init_flag, u8 ibc_flag, u8 ibc_log_max_size, u8* map_tidx)
{
    int nev_info[NUM_CNID][3];
    int scun[3], avail[3];
    int scup = y_scu * w_scu + x_scu;
    int scuw = cuw >> MIN_CU_LOG2, scuh = cuh >> MIN_CU_LOG2;
    int num_pos_avail;
    int i, j;

    if ((slice_type == SLICE_I && ibc_flag == 0) || (slice_type == SLICE_I && (cuw > (1 << ibc_log_max_size) || cuh > (1 << ibc_log_max_size))))
    {
      return;
    }

    for(i = 0; i < NUM_CNID; i++)
    {
        nev_info[i][0] = nev_info[i][1] = nev_info[i][2] = 0;
        ctx[i] = 0;
    }

    scun[0] = scup - w_scu;
    scun[1] = scup - 1 + (scuh - 1)*w_scu;
    scun[2] = scup + scuw + (scuh - 1)*w_scu;
    avail[0] = y_scu == 0 ? 0 : ((map_tidx[scup] == map_tidx[scun[0]]) && MCU_GET_COD(map_scu[scun[0]]));
    avail[1] = x_scu == 0 ? 0 : ((map_tidx[scup] == map_tidx[scun[1]]) && MCU_GET_COD(map_scu[scun[1]]));
    avail[2] = x_scu + scuw >= w_scu ? 0 : ((map_tidx[scup] == map_tidx[scun[2]]) && MCU_GET_COD(map_scu[scun[2]]));
    num_pos_avail = 0;

    for (j = 0; j < 3; j++)
    {
        if (avail[j])
        {
            nev_info[CNID_SKIP_FLAG][j] = MCU_GET_SF(map_scu[scun[j]]);
            nev_info[CNID_PRED_MODE][j] = MCU_GET_IF(map_scu[scun[j]]);

            if (slice_type != SLICE_I)
            {
                nev_info[CNID_AFFN_FLAG][j] = MCU_GET_AFF(map_scu[scun[j]]);
            }

            if (ibc_flag == 1)
            {
                nev_info[CNID_IBC_FLAG][j] = MCU_GET_IBC(map_scu[scun[j]]);
            }

            num_pos_avail++;
        }
    }

    //decide ctx
    for(i = 0; i < NUM_CNID; i++)
    {
        if(num_pos_avail == 0)
        {
            ctx[i] = 0;
        }
        else
        {
            ctx[i] = nev_info[i][0] + nev_info[i][1] + nev_info[i][2];

            if(i == CNID_SKIP_FLAG)
            {
                if(sps_cm_init_flag == 1)
                {
                    ctx[i] = XEVE_MIN(ctx[i], NUM_CTX_SKIP_FLAG - 1);
                }
                else
                {
                    ctx[i] = 0;
                }
            }
            else if (i == CNID_IBC_FLAG)
            {
              if (sps_cm_init_flag == 1)
              {
                ctx[i] = XEVE_MIN(ctx[i], NUM_CTX_IBC_FLAG - 1);
              }
              else
              {
                ctx[i] = 0;
              }
            }
            else if(i == CNID_PRED_MODE)
            {
                if(sps_cm_init_flag == 1)
                {
                    ctx[i] = XEVE_MIN(ctx[i], NUM_CTX_PRED_MODE - 1);
                }
                else
                {
                    ctx[i] = 0;
                }
            }
            else if (i == CNID_MODE_CONS)
            {
                if (sps_cm_init_flag == 1)
                {
                    ctx[i] = XEVE_MIN(ctx[i], NUM_CTX_MODE_CONS - 1);
                }
                else
                {
                    ctx[i] = 0;
                }
            }
            else if(i == CNID_AFFN_FLAG)
            {
                if(sps_cm_init_flag == 1)
                {
                    ctx[i] = XEVE_MIN(ctx[i], NUM_CTX_AFFINE_FLAG - 1);
                }
                else
                {
                    ctx[i] = 0;
                }
            }
        }
    }
}

void xeve_init_scan_sr(int *scan, int size_x, int size_y, int width, int height, int scan_type)
{
    int x, y, l, pos, num_line;

    pos = 0;
    num_line = size_x + size_y - 1;
    if(scan_type == COEF_SCAN_ZIGZAG)
    {
        /* starting point */
        scan[pos] = 0;
        pos++;

        /* loop */
        for(l = 1; l < num_line; l++)
        {
            if(l % 2) /* decreasing loop */
            {
                x = XEVE_MIN(l, size_x - 1);
                y = XEVE_MAX(0, l - (size_x - 1));

                while(x >= 0 && y < size_y)
                {
                    scan[pos] = y * width + x;
                    pos++;
                    x--;
                    y++;
                }
            }
            else /* increasing loop */
            {
                y = XEVE_MIN(l, size_y - 1);
                x = XEVE_MAX(0, l - (size_y - 1));
                while(y >= 0 && x < size_x)
                {
                    scan[pos] = y * width + x;
                    pos++;
                    x++;
                    y--;
                }
            }
        }
    }
}

void xeve_init_inverse_scan_sr(u16 *scan_inv, u16 *scan_orig, int width, int height, int scan_type)
{
    int x, num_line;

    num_line = width*height;
    if ( (scan_type == COEF_SCAN_ZIGZAG) || (scan_type == COEF_SCAN_DIAG) || (scan_type == COEF_SCAN_DIAG_CG) )
    {
        for ( x = 0; x < num_line; x++)
        {
            int scan_pos = scan_orig[x];
            assert(scan_pos >= 0);
            assert(scan_pos < num_line);
            scan_inv[scan_pos] = x;
        }
    }
    else
    {
        xeve_assert(0);
        xeve_trace("Not supported scan_type\n");
    }
}

int xeve_get_transform_shift(int log2_size, int type, int bit_depth)
{
    return (type == 0) ? TX_SHIFT1(log2_size, bit_depth) : TX_SHIFT2(log2_size);
}

void xeve_split_get_part_structure(int split_mode, int x0, int y0, int cuw, int cuh, int cup, int cud, int log2_culine, XEVE_SPLIT_STRUCT* split_struct)
{
    int i;
    int log_cuw, log_cuh;
    int cup_w, cup_h;

    log_cuw = XEVE_LOG2(cuw);
    log_cuh = XEVE_LOG2(cuh);
    split_struct->x_pos[0] = x0;
    split_struct->y_pos[0] = y0;
    split_struct->cup[0] = cup;

    switch (split_mode)
    {
    case NO_SPLIT:
    {
        split_struct->width[0] = cuw;
        split_struct->height[0] = cuh;
        split_struct->log_cuw[0] = log_cuw;
        split_struct->log_cuh[0] = log_cuh;
    }
    break;

    case SPLIT_QUAD:
    {
        split_struct->part_count = 4;
        split_struct->width[0] = cuw >> 1;
        split_struct->height[0] = cuh >> 1;
        split_struct->log_cuw[0] = log_cuw - 1;
        split_struct->log_cuh[0] = log_cuh - 1;
        for (i = 1; i < split_struct->part_count; ++i)
        {
            split_struct->width[i] = split_struct->width[0];
            split_struct->height[i] = split_struct->height[0];
            split_struct->log_cuw[i] = split_struct->log_cuw[0];
            split_struct->log_cuh[i] = split_struct->log_cuh[0];
        }
        split_struct->x_pos[1] = x0 + split_struct->width[0];
        split_struct->y_pos[1] = y0;
        split_struct->x_pos[2] = x0;
        split_struct->y_pos[2] = y0 + split_struct->height[0];
        split_struct->x_pos[3] = split_struct->x_pos[1];
        split_struct->y_pos[3] = split_struct->y_pos[2];
        cup_w = (split_struct->width[0] >> MIN_CU_LOG2);
        cup_h = ((split_struct->height[0] >> MIN_CU_LOG2) << log2_culine);
        split_struct->cup[1] = cup + cup_w;
        split_struct->cup[2] = cup + cup_h;
        split_struct->cup[3] = split_struct->cup[1] + cup_h;
        split_struct->cud[0] = cud + 2;
        split_struct->cud[1] = cud + 2;
        split_struct->cud[2] = cud + 2;
        split_struct->cud[3] = cud + 2;
    }
    break;

    default:
    break;
    }
}

void xeve_block_copy(s16 * src, int src_stride, s16 * dst, int dst_stride, int log2_copy_w, int log2_copy_h)
{
    int h;
    int copy_size = (1 << log2_copy_w) * (int)sizeof(s16);
    s16 *tmp_src = src;
    s16 *tmp_dst = dst;
    for (h = 0; h < (1<< log2_copy_h); h++)
    {
        xeve_mcpy(tmp_dst, tmp_src, copy_size);
        tmp_dst += dst_stride;
        tmp_src += src_stride;
    }
}

int xeve_get_luma_cup(int x_scu, int y_scu, int cu_w_scu, int cu_h_scu, int w_scu)
{
    return (y_scu + (cu_h_scu >> 1)) * w_scu + x_scu + (cu_w_scu >> 1);
}

u8 xeve_check_luma(TREE_CONS tree_cons)
{
    return tree_cons.tree_type != TREE_C;
}

u8 xeve_check_chroma(TREE_CONS tree_cons)
{
    return tree_cons.tree_type != TREE_L;
}

u8 xeve_check_all(TREE_CONS tree_cons)
{
    return tree_cons.tree_type == TREE_LC;
}

u8 xeve_check_only_intra(TREE_CONS tree_cons)
{
    return tree_cons.mode_cons == eOnlyIntra;
}

u8 xeve_check_only_inter(TREE_CONS tree_cons)
{
    return tree_cons.mode_cons == eOnlyInter;
}

u8 xeve_check_all_preds(TREE_CONS tree_cons)
{
    return tree_cons.mode_cons == eAll;
}

TREE_CONS xeve_get_default_tree_cons()
{
    TREE_CONS ans;
    ans.changed = FALSE;
    ans.mode_cons = eAll;
    ans.tree_type = TREE_LC;
    return ans;
}

void xeve_set_tree_mode(TREE_CONS* dest, MODE_CONS mode)
{
    dest->mode_cons = mode;
    switch (mode)
    {
    case eOnlyIntra:
        dest->tree_type = TREE_L;
        break;
    default:
        dest->tree_type = TREE_LC;
        break;
    }
}

MODE_CONS xeve_get_mode_cons_by_split(SPLIT_MODE split_mode, int cuw, int cuh)
{
    int small_cuw = cuw;
    int small_cuh = cuh;
    switch (split_mode)
    {
    case SPLIT_BI_HOR:
        small_cuh >>= 1;
        break;
    case SPLIT_BI_VER:
        small_cuw >>= 1;
        break;
    case SPLIT_TRI_HOR:
        small_cuh >>= 2;
        break;
    case SPLIT_TRI_VER:
        small_cuw >>= 2;
        break;
    default:
        xeve_assert(!"For BTT only");
    }
    return (small_cuh == 4 && small_cuw == 4) ? eOnlyIntra : eAll;
}

BOOL xeve_signal_mode_cons(TREE_CONS* parent, TREE_CONS* cur_split)
{
    return parent->mode_cons == eAll && cur_split->changed;
}

static void imgb_delete(XEVE_IMGB * imgb)
{
    int i;
    xeve_assert_r(imgb);

    for(i=0; i<XEVE_IMGB_MAX_PLANE; i++)
    {
        if (imgb->baddr[i]) xeve_mfree(imgb->baddr[i]);
    }
    xeve_mfree(imgb);
}

static int imgb_addref(XEVE_IMGB * imgb)
{
    xeve_assert_rv(imgb, XEVE_ERR_INVALID_ARGUMENT);
    return xeve_atomic_inc(&imgb->refcnt);
}

static int imgb_getref(XEVE_IMGB * imgb)
{
    xeve_assert_rv(imgb, XEVE_ERR_INVALID_ARGUMENT);
    return imgb->refcnt;
}

static int imgb_release(XEVE_IMGB * imgb)
{
    int refcnt;
    xeve_assert_rv(imgb, XEVE_ERR_INVALID_ARGUMENT);
    refcnt = xeve_atomic_dec(&imgb->refcnt);
    if(refcnt == 0)
    {
        imgb_delete(imgb);
    }
    return refcnt;
}

static void imgb_cpy_shift_left_8b(XEVE_IMGB * imgb_dst, XEVE_IMGB * imgb_src, int shift)
{
    int i, j, k;

    unsigned char * s;
    short         * d;

    for (i = 0; i < imgb_dst->np; i++)
    {
        s = imgb_src->a[i];
        d = imgb_dst->a[i];

        for (j = 0; j < imgb_src->h[i]; j++)
        {
            for (k = 0; k < imgb_src->w[i]; k++)
            {
                d[k] = (short)(s[k] << shift);
            }
            s = s + imgb_src->s[i];
            d = (short*)(((unsigned char *)d) + imgb_dst->s[i]);
        }
    }
}

static void imgb_cpy_shift_right_8b(XEVE_IMGB *dst, XEVE_IMGB *src, int shift)
{
    int i, j, k, t0, add;

    short         *s;
    unsigned char *d;

    if(shift)
    add = 1 << (shift - 1);
    else
        add = 0;

    for(i = 0; i < dst->np; i++)
    {
        s = src->a[i];
        d = dst->a[i];

        for(j = 0; j < src->ah[i]; j++)
        {
            for(k = 0; k < src->aw[i]; k++)
            {
                t0 = ((s[k] + add) >> shift);
                d[k] = (unsigned char)(XEVE_CLIP3(0, 255, t0));
            }
            s = (short*)(((unsigned char *)s) + src->s[i]);
            d = d + dst->s[i];
        }
    }
}

static void imgb_cpy_plane(XEVE_IMGB * dst, XEVE_IMGB * src)
{
    int i, j;
    unsigned char *s, *d;
    int numbyte = XEVE_CS_GET_BYTE_DEPTH(src->cs);

    for(i = 0; i < src->np; i++)
    {
        s = (unsigned char*)src->a[i];
        d = (unsigned char*)dst->a[i];

        for(j = 0; j < src->ah[i]; j++)
        {
            xeve_mcpy(d, s, numbyte * src->aw[i]);
            s += src->s[i];
            d += dst->s[i];
        }
    }
}

static void imgb_cpy_shift_left(XEVE_IMGB *dst, XEVE_IMGB *src, int shift)
{
    int i, j, k;

    unsigned short * s;
    unsigned short * d;

    for (i = 0; i < dst->np; i++)
    {
        s = src->a[i];
        d = dst->a[i];

        for (j = 0; j < src->h[i]; j++)
        {
            for (k = 0; k < src->w[i]; k++)
            {
                d[k] = (unsigned short)(s[k] << shift);
            }
            s = (short*)(((unsigned char *)s) + src->s[i]);
            d = (short*)(((unsigned char *)d) + dst->s[i]);
        }
    }
}

static void imgb_cpy_shift_right(XEVE_IMGB * dst, XEVE_IMGB * src, int shift)
{

    int i, j, k, t0, add;

    int clip_min = 0;
    int clip_max = 0;

    unsigned short         * s;
    unsigned short         * d;
    if (shift)
        add = 1 << (shift - 1);
    else
        add = 0;

    clip_max = (1 << (XEVE_CS_GET_BIT_DEPTH(dst->cs))) - 1;

    for (i = 0; i < dst->np; i++)
    {
        s = src->a[i];
        d = dst->a[i];

        for (j = 0; j < src->h[i]; j++)
        {
            for (k = 0; k < src->w[i]; k++)
            {
                t0 = ((s[k] + add) >> shift);
                d[k] = (XEVE_CLIP3(clip_min, clip_max, t0));

            }
            s = (short*)(((unsigned char *)s) + src->s[i]);
            d = (short*)(((unsigned char *)d) + dst->s[i]);
        }
    }
}

void xeve_imgb_cpy(XEVE_IMGB * dst, XEVE_IMGB * src)
{
    int i, bd_src, bd_dst;
    bd_src = XEVE_CS_GET_BIT_DEPTH(src->cs);
    bd_dst = XEVE_CS_GET_BIT_DEPTH(dst->cs);

    if(src->cs == dst->cs)
    {
        imgb_cpy_plane(dst, src);
    }
    else if(bd_src == 8 && bd_dst > 8)
    {
        imgb_cpy_shift_left_8b(dst, src, bd_dst - bd_src);
    }
    else if(bd_src > 8 && bd_dst == 8)
    {
        imgb_cpy_shift_right_8b(dst, src, bd_src - bd_dst);
    }
    else if(bd_src < bd_dst)
    {
        imgb_cpy_shift_left(dst, src, bd_dst - bd_src);
    }
    else if(bd_src > bd_dst)
    {
        imgb_cpy_shift_right(dst, src, bd_src - bd_dst);
    }
    else
    {
        xeve_trace("ERROR: unsupported image copy\n");
        return;
    }
    for(i = 0; i < XEVE_IMGB_MAX_PLANE; i++)
    {
        dst->x[i] = src->x[i];
        dst->y[i] = src->y[i];
        dst->w[i] = src->w[i];
        dst->h[i] = src->h[i];
        dst->ts[i] = src->ts[i];
    }
}

XEVE_IMGB * xeve_imgb_create(int w, int h, int cs, int opt, int pad[XEVE_IMGB_MAX_PLANE], int align[XEVE_IMGB_MAX_PLANE])
{
    int i, p_size, a_size;
    XEVE_IMGB * imgb;
    int bd      = XEVE_CS_GET_BYTE_DEPTH(cs);
    int cfi     = XEVE_CFI_FROM_CF(XEVE_CS_GET_FORMAT(cs));
    int np      = (cfi == 0) ? 1 : 3;
    int w_shift = XEVE_GET_CHROMA_W_SHIFT(cfi);
    int h_shift = XEVE_GET_CHROMA_H_SHIFT(cfi);

    imgb = (XEVE_IMGB *)xeve_malloc(sizeof(XEVE_IMGB));
    xeve_assert_rv(imgb, NULL);
    xeve_mset(imgb, 0, sizeof(XEVE_IMGB));

    bd = XEVE_CS_GET_BYTE_DEPTH(cs); /* byteunit */
    cfi = XEVE_CFI_FROM_CF(XEVE_CS_GET_FORMAT(cs)); /*chroma format idc*/
    np = cfi == 0 ? 1 : 3;

    for(i = 0; i<np; i++)
    {
        imgb->w[i] = w;
        imgb->h[i] = h;
        imgb->x[i] = 0;
        imgb->y[i] = 0;

        a_size = (align != NULL)? align[i] : 0;
        p_size = (pad != NULL)? pad[i] : 0;

        imgb->aw[i] = XEVE_ALIGN_VAL(w, a_size);
        imgb->ah[i] = XEVE_ALIGN_VAL(h, a_size);

        imgb->padl[i] = imgb->padr[i]=imgb->padu[i]=imgb->padb[i]=p_size;

        imgb->s[i] = (imgb->aw[i] + imgb->padl[i] + imgb->padr[i]) * bd;
        imgb->e[i] = imgb->ah[i] + imgb->padu[i] + imgb->padb[i];

        imgb->bsize[i] = imgb->s[i]*imgb->e[i];
        imgb->baddr[i] = xeve_malloc(imgb->bsize[i]);

        imgb->a[i] = ((u8*)imgb->baddr[i]) + imgb->padu[i]*imgb->s[i] +
        imgb->padl[i]*bd;

        if(i == 0 && cfi)
        {
            if (w_shift)
            {
                w = (w + w_shift) >> w_shift;
            }
            if (h_shift)
            {
                h = (h + h_shift) >> h_shift;
            }
        }
    }
    imgb->np = np;
    imgb->addref = imgb_addref;
    imgb->getref = imgb_getref;
    imgb->release = imgb_release;
    imgb->cs = cs;
    imgb->addref(imgb);

    return imgb;
}

void xeve_imgb_garbage_free(XEVE_IMGB * imgb)
{
    int i;
    if (imgb == NULL) return;
    for(i=0; i<XEVE_IMGB_MAX_PLANE; i++)
    {
        if(imgb->a[i]) xeve_mfree(imgb->a[i]);
    }
    xeve_mfree(imgb);
}
#if X86_SSE
#if (defined(_WIN64) || defined(_WIN32)) && !defined(__GNUC__)
#include <intrin.h >
#elif defined( __GNUC__)
#ifndef _XCR_XFEATURE_ENABLED_MASK
#define _XCR_XFEATURE_ENABLED_MASK 0
#endif
static void __cpuid(int* info, int i)
{
    __asm__ __volatile__(
        "cpuid" : "=a" (info[0]), "=b" (info[1]), "=c" (info[2]), "=d" (info[3])
                : "a" (i), "c" (0));
}

static unsigned long long __xgetbv(unsigned int i)
{
    unsigned int eax, edx;
    __asm__ __volatile__(
        "xgetbv;" : "=a" (eax), "=d"(edx)
                  : "c" (i));
    return ((unsigned long long)edx << 32) | eax;
}
#endif
#define GET_CPU_INFO(A,B) ((B[((A >> 5) & 0x03)] >> (A & 0x1f)) & 1)

int xeve_check_cpu_info()
{
    int support_sse  = 0;
    int support_avx  = 0;
    int support_avx2 = 0;
    int cpu_info[4]  = { 0 };
    __cpuid(cpu_info, 0);
    int id_cnt = cpu_info[0];

    if (id_cnt >= 1)
    {
        __cpuid(cpu_info, 1);
        support_sse |= GET_CPU_INFO(XEVE_CPU_INFO_SSE41, cpu_info);
        int os_use_xsave = GET_CPU_INFO(XEVE_CPU_INFO_OSXSAVE, cpu_info);
        int cpu_support_avx = GET_CPU_INFO(XEVE_CPU_INFO_AVX, cpu_info);

        if (os_use_xsave && cpu_support_avx)
        {
            unsigned long long xcr_feature_mask = __xgetbv(_XCR_XFEATURE_ENABLED_MASK);
            support_avx = (xcr_feature_mask & 0x6) || 0;
            if (id_cnt >= 7)
            {
                __cpuid(cpu_info, 7);
                support_avx2 = support_avx && GET_CPU_INFO(XEVE_CPU_INFO_AVX2, cpu_info);
            }
        }
    }

    return (support_sse << 1) | support_avx | (support_avx2 << 2);
}
#endif

XEVE_CTX * xeve_ctx_alloc(void)
{
    XEVE_CTX * ctx;

    ctx = (XEVE_CTX*)xeve_malloc_fast(sizeof(XEVE_CTX));
    xeve_assert_rv(ctx, NULL);
    xeve_mset_x64a(ctx, 0, sizeof(XEVE_CTX));
    return ctx;
}

void xeve_ctx_free(XEVE_CTX * ctx)
{
    xeve_mfree_fast(ctx);
}


XEVE_CORE * xeve_core_alloc(int chroma_format_idc)
{
    XEVE_CORE * core;
    int i, j;

    core = (XEVE_CORE *)xeve_malloc_fast(sizeof(XEVE_CORE));

    xeve_assert_rv(core, NULL);
    xeve_mset_x64a(core, 0, sizeof(XEVE_CORE));

    for (i = 0; i < MAX_CU_LOG2; i++)
    {
        for (j = 0; j < MAX_CU_LOG2; j++)
        {
            xeve_create_cu_data(&core->cu_data_best[i][j], i, j, chroma_format_idc);
            xeve_create_cu_data(&core->cu_data_temp[i][j], i, j, chroma_format_idc);
        }
    }

    return core;
}

void xeve_core_free(XEVE_CORE * core)
{
    int i, j;

    for (i = 0; i < MAX_CU_LOG2; i++)
    {
        for (j = 0; j < MAX_CU_LOG2; j++)
        {
            xeve_delete_cu_data(&core->cu_data_best[i][j], i, j);
            xeve_delete_cu_data(&core->cu_data_temp[i][j], i, j);
        }
    }

    xeve_mfree_fast(core);
}

void xeve_copy_chroma_qp_mapping_params(XEVE_CHROMA_TABLE *dst, XEVE_CHROMA_TABLE *src)
{
    dst->chroma_qp_table_present_flag = src->chroma_qp_table_present_flag;
    dst->same_qp_table_for_chroma = src->same_qp_table_for_chroma;
    dst->global_offset_flag = src->global_offset_flag;
    dst->num_points_in_qp_table_minus1[0] = src->num_points_in_qp_table_minus1[0];
    dst->num_points_in_qp_table_minus1[1] = src->num_points_in_qp_table_minus1[1];
    xeve_mcpy(&(dst->delta_qp_in_val_minus1), &(src->delta_qp_in_val_minus1), sizeof(int) * 2 * XEVE_MAX_QP_TABLE_SIZE);
    xeve_mcpy(&(dst->delta_qp_out_val), &(src->delta_qp_out_val), sizeof(int) * 2 * XEVE_MAX_QP_TABLE_SIZE);
}

static void parse_chroma_qp_mapping_table(XEVE_CHROMA_TABLE* chroma_qp_table, XEVE_PARAM * param)
{
    memset(chroma_qp_table, 0, sizeof(XEVE_CHROMA_TABLE));
    chroma_qp_table->chroma_qp_table_present_flag = param->chroma_qp_table_present_flag;
    if (chroma_qp_table->chroma_qp_table_present_flag)
    {
        chroma_qp_table->num_points_in_qp_table_minus1[0] = atoi(strtok(param->chroma_qp_num_points_in_table, " ")) - 1;
        chroma_qp_table->num_points_in_qp_table_minus1[1] = atoi(strtok(NULL, " \r")) - 1;

        { /* input pivot points */
            chroma_qp_table->delta_qp_in_val_minus1[0][0] = atoi(strtok(param->chroma_qp_delta_in_val_cb, " "));
            int j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                chroma_qp_table->delta_qp_in_val_minus1[0][j++] = atoi(val);
            } while (1);
            if (chroma_qp_table->num_points_in_qp_table_minus1[0] + 1 == j);

            chroma_qp_table->delta_qp_in_val_minus1[1][0] = atoi(strtok(param->chroma_qp_delta_in_val_cr, " "));
            j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                chroma_qp_table->delta_qp_in_val_minus1[1][j++] = atoi(val);
            } while (1);
            assert(chroma_qp_table->num_points_in_qp_table_minus1[1] + 1 == j);
        }
        {/* output pivot points */
            chroma_qp_table->delta_qp_out_val[0][0] = atoi(strtok(param->chroma_qp_delta_out_val_cb, " "));
            int j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                chroma_qp_table->delta_qp_out_val[0][j++] = atoi(val);
            } while (1);
            assert(chroma_qp_table->num_points_in_qp_table_minus1[0] + 1 == j);

            chroma_qp_table->delta_qp_out_val[1][0] = atoi(strtok(param->chroma_qp_delta_out_val_cr, " "));
            j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                chroma_qp_table->delta_qp_out_val[1][j++] = atoi(val);
            } while (1);
            assert(chroma_qp_table->num_points_in_qp_table_minus1[1] + 1 == j);
        }
    }
}

static void parse_chroma_qp_mapping_params(XEVE_CHROMA_TABLE *dst_struct, XEVE_CHROMA_TABLE *src_struct, int bit_depth)
{
    int qp_bd_offset_c = 6 * (bit_depth - 8);
    dst_struct->chroma_qp_table_present_flag = src_struct->chroma_qp_table_present_flag;
    dst_struct->num_points_in_qp_table_minus1[0] = src_struct->num_points_in_qp_table_minus1[0];
    dst_struct->num_points_in_qp_table_minus1[1] = src_struct->num_points_in_qp_table_minus1[1];

    if (dst_struct->chroma_qp_table_present_flag)
    {
        dst_struct->same_qp_table_for_chroma = 1;
        if (src_struct->num_points_in_qp_table_minus1[0] != src_struct->num_points_in_qp_table_minus1[1])
            dst_struct->same_qp_table_for_chroma = 0;
        else
        {
            for (int i = 0; i < src_struct->num_points_in_qp_table_minus1[0]; i++)
            {
                if ((src_struct->delta_qp_in_val_minus1[0][i] != src_struct->delta_qp_in_val_minus1[1][i]) ||
                    (src_struct->delta_qp_out_val[0][i] != src_struct->delta_qp_out_val[1][i]))
                {
                    dst_struct->same_qp_table_for_chroma = 0;
                    break;
                }
            }
        }

        dst_struct->global_offset_flag = (src_struct->delta_qp_in_val_minus1[0][0] > 15 && src_struct->delta_qp_out_val[0][0] > 15) ? 1 : 0;
        if (!dst_struct->same_qp_table_for_chroma)
        {
            dst_struct->global_offset_flag = dst_struct->global_offset_flag && ((src_struct->delta_qp_in_val_minus1[1][0] > 15 && src_struct->delta_qp_out_val[1][0] > 15) ? 1 : 0);
        }

        int start_qp = (dst_struct->global_offset_flag == 1) ? 16 : -qp_bd_offset_c;
        for (int ch = 0; ch < (dst_struct->same_qp_table_for_chroma ? 1 : 2); ch++)
        {
            dst_struct->delta_qp_in_val_minus1[ch][0] = src_struct->delta_qp_in_val_minus1[ch][0] - start_qp;
            dst_struct->delta_qp_out_val[ch][0] = src_struct->delta_qp_out_val[ch][0] - start_qp - dst_struct->delta_qp_in_val_minus1[ch][0];

            for (int k = 1; k <= dst_struct->num_points_in_qp_table_minus1[ch]; k++)
            {
                dst_struct->delta_qp_in_val_minus1[ch][k] = (src_struct->delta_qp_in_val_minus1[ch][k] - src_struct->delta_qp_in_val_minus1[ch][k - 1]) - 1;
                dst_struct->delta_qp_out_val[ch][k] = (src_struct->delta_qp_out_val[ch][k] - src_struct->delta_qp_out_val[ch][k - 1]) - (dst_struct->delta_qp_in_val_minus1[ch][k] + 1);
            }
        }
    }
}

static void tbl_derived_chroma_qp_mapping(XEVE_CTX * ctx, XEVE_CHROMA_TABLE * struct_qp_c, int bit_depth)
{
    int MAX_QP = XEVE_MAX_QP_TABLE_SIZE - 1;
    int qpInVal[XEVE_MAX_QP_TABLE_SIZE_EXT] = { 0 };
    int qpOutVal[XEVE_MAX_QP_TABLE_SIZE_EXT] = { 0 };
    int qp_bd_offset_c = 6 * (bit_depth - 8);
    int startQp = (struct_qp_c->global_offset_flag == 1) ? 16 : -qp_bd_offset_c;

    for (int i = 0; i < (struct_qp_c->same_qp_table_for_chroma ? 1 : 2); i++)
    {
        qpInVal[0] = startQp + struct_qp_c->delta_qp_in_val_minus1[i][0];
        qpOutVal[0] = startQp + struct_qp_c->delta_qp_in_val_minus1[i][0] + struct_qp_c->delta_qp_out_val[i][0];
        for (int j = 1; j <= struct_qp_c->num_points_in_qp_table_minus1[i]; j++)
        {
            qpInVal[j] = qpInVal[j - 1] + struct_qp_c->delta_qp_in_val_minus1[i][j] + 1;
            qpOutVal[j] = qpOutVal[j - 1] + (struct_qp_c->delta_qp_in_val_minus1[i][j] + 1 + struct_qp_c->delta_qp_out_val[i][j]);
        }

        for (int j = 0; j <= struct_qp_c->num_points_in_qp_table_minus1[i]; j++)
        {
            assert(qpInVal[j] >= -qp_bd_offset_c && qpInVal[j]  <= MAX_QP);
            assert(qpOutVal[j] >= -qp_bd_offset_c && qpOutVal[j] <= MAX_QP);
        }

        ctx->qp_chroma_dynamic[i][qpInVal[0]] = qpOutVal[0];
        for (int k = qpInVal[0] - 1; k >= -qp_bd_offset_c; k--)
        {
            ctx->qp_chroma_dynamic[i][k] = XEVE_CLIP3(-qp_bd_offset_c, MAX_QP, ctx->qp_chroma_dynamic[i][k + 1] - 1);
        }
        for (int j = 0; j < struct_qp_c->num_points_in_qp_table_minus1[i]; j++)
        {
            int sh = (struct_qp_c->delta_qp_in_val_minus1[i][j + 1] + 1) >> 1;
            for (int k = qpInVal[j] + 1, m = 1; k <= qpInVal[j + 1]; k++, m++)
            {
                ctx->qp_chroma_dynamic[i][k] = ctx->qp_chroma_dynamic[i][qpInVal[j]]
                    + ((qpOutVal[j + 1] - qpOutVal[j]) * m + sh) / (struct_qp_c->delta_qp_in_val_minus1[i][j + 1] + 1);
            }
        }
        for (int k = qpInVal[struct_qp_c->num_points_in_qp_table_minus1[i]] + 1; k <= MAX_QP; k++)
        {
            ctx->qp_chroma_dynamic[i][k] = XEVE_CLIP3(-qp_bd_offset_c, MAX_QP, ctx->qp_chroma_dynamic[i][k - 1] + 1);
        }
    }
    if (struct_qp_c->same_qp_table_for_chroma)
    {
        xeve_mcpy(&(ctx->qp_chroma_dynamic[1][-qp_bd_offset_c]), &(ctx->qp_chroma_dynamic[0][-qp_bd_offset_c]), XEVE_MAX_QP_TABLE_SIZE_EXT * sizeof(int));
    }
}

// ChromaQP offset for U and V components
void xeve_set_chroma_qp_tbl_loc(XEVE_CTX* ctx)
{
    for (int i = 0; i < 6 * (ctx->param.codec_bit_depth - 8); i++)
    {
        ctx->qp_chroma_dynamic_ext[0][i] = i - 6 * (ctx->param.codec_bit_depth - 8);
        ctx->qp_chroma_dynamic_ext[1][i] = i - 6 * (ctx->param.codec_bit_depth - 8);
    }
    ctx->qp_chroma_dynamic[0] = &(ctx->qp_chroma_dynamic_ext[0][6 * (ctx->param.codec_bit_depth - 8)]);
    ctx->qp_chroma_dynamic[1] = &(ctx->qp_chroma_dynamic_ext[1][6 * (ctx->param.codec_bit_depth - 8)]);
}

static int parse_tile_slice_param(XEVE_CTX* ctx)
{
    XEVE_TS_INFO * ts_info = &ctx->ts_info;
    XEVE_PARAM   * param = &ctx->param;

    ts_info->tile_uniform_spacing_flag = param->tile_uniform_spacing_flag;
    ts_info->tile_columns              = param->tile_columns;
    ts_info->tile_rows                 = param->tile_rows;
    ts_info->num_slice_in_pic          = param->num_slice_in_pic;
    ts_info->arbitrary_slice_flag      = param->arbitrary_slice_flag;

    int num_tiles = ts_info->tile_columns * ts_info->tile_rows;
    if (num_tiles < ts_info->num_slice_in_pic) return XEVE_ERR;
    if (num_tiles > 1)
    {
        if (!ts_info->tile_uniform_spacing_flag)
        {
            ts_info->tile_column_width_array[0] = atoi(strtok(param->tile_column_width_array, " "));
            int j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                ts_info->tile_column_width_array[j++] = atoi(val);
            } while (1);

            ts_info->tile_row_height_array[0] = atoi(strtok(param->tile_row_height_array, " "));
            j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                ts_info->tile_row_height_array[j++] = atoi(val);
            } while (1);
        }

        if (ts_info->num_slice_in_pic == 1)
        {
            ts_info->tile_array_in_slice[0] = 0;
            ts_info->tile_array_in_slice[1] = (ts_info->tile_columns * ts_info->tile_rows) - 1;
            ts_info->num_remaining_tiles_in_slice_minus1[0] = param->num_remaining_tiles_in_slice_minus1[0];
        }
        else /* There are more than one slice in the picture */
        {
            ts_info->tile_array_in_slice[0] = atoi(strtok(param->num_remaining_tiles_in_slice_minus1, " "));
            int j = 1;
            do
            {
                char* val = strtok(NULL, " \r");
                if (!val)
                    break;
                ts_info->tile_array_in_slice[j++] = atoi(val);
            } while (1);

            if (ts_info->arbitrary_slice_flag)
            {
                ts_info->num_remaining_tiles_in_slice_minus1[0] = atoi(strtok(param->num_remaining_tiles_in_slice_minus1, " ")) - 1;
                int j = 1;
                do
                {
                    char* val = strtok(NULL, " \r");
                    if (!val)
                        break;
                    ts_info->num_remaining_tiles_in_slice_minus1[j++] = atoi(val) - 1;
                } while (1);
            }
        }
    }

    return XEVE_OK;
}

int xeve_set_init_param(XEVE_CTX * ctx, XEVE_PARAM * param)
{
    /* check input parameters */
    xeve_assert_rv(param->w > 0 && param->h > 0, XEVE_ERR_INVALID_ARGUMENT);
    xeve_assert_rv(param->qp >= MIN_QUANT && param->qp <= MAX_QUANT, XEVE_ERR_INVALID_ARGUMENT);
    xeve_assert_rv(param->keyint >= 0 ,XEVE_ERR_INVALID_ARGUMENT);
    xeve_assert_rv(param->threads <= XEVE_MAX_THREADS ,XEVE_ERR_INVALID_ARGUMENT);

    if(param->disable_hgop == 0)
    {
        xeve_assert_rv(param->bframes == 0 || param->bframes == 1 || \
                       param->bframes == 3 || param->bframes == 7 || \
                       param->bframes == 15, XEVE_ERR_INVALID_ARGUMENT);

        if(param->bframes != 0)
        {
            if(param->keyint % (param->bframes + 1) != 0)
            {
                xeve_assert_rv(0, XEVE_ERR_INVALID_ARGUMENT);
            }
        }
    }

    if (param->ref != 0)
    {
        if (param->bframes == 0)
        {
            param->me_ref_num = XEVE_MIN(5, param->ref);
        }
        else
        {
            param->me_ref_num = (param->ref > param->bframes) ? param->bframes : param->ref;
        }
    }

    if (param->ref_pic_gap_length != 0)
    {
        xeve_assert_rv(param->bframes == 0, XEVE_ERR_INVALID_ARGUMENT);
    }

    if (param->bframes == 0)
    {
        if (param->ref_pic_gap_length == 0)
        {
            param->ref_pic_gap_length = 1;
        }
        xeve_assert_rv(param->ref_pic_gap_length == 1 || param->ref_pic_gap_length == 2 || \
                       param->ref_pic_gap_length == 4 || param->ref_pic_gap_length == 8 || \
                       param->ref_pic_gap_length == 16, XEVE_ERR_INVALID_ARGUMENT);
    }

    /* set default encoding parameter */
    param->qp_max            = MAX_QUANT;
    param->qp_min            = MIN_QUANT;
    param->gop_size          = param->bframes +1;
    param->lookahead         = XEVE_MIN(XEVE_MAX((param->cutree)? param->gop_size : 0, param->lookahead), XEVE_MAX_INBUF_CNT>>1);
    param->use_fcst          = ((param->use_fcst || param->lookahead) && (param->rc_type || param->aq_mode)) ? 1 : 0;
    param->chroma_format_idc = XEVE_CFI_FROM_CF(XEVE_CS_GET_FORMAT(param->cs));
    param->cs_w_shift        = XEVE_GET_CHROMA_W_SHIFT(param->chroma_format_idc);
    param->cs_h_shift        = XEVE_GET_CHROMA_H_SHIFT(param->chroma_format_idc);


    if (param->chroma_qp_table_present_flag)
    {
        XEVE_CHROMA_TABLE tmp_qp_tbl;
        parse_chroma_qp_mapping_table(&tmp_qp_tbl, param);
        parse_chroma_qp_mapping_params(&(ctx->chroma_qp_table_struct), &tmp_qp_tbl, param->codec_bit_depth);
        tbl_derived_chroma_qp_mapping(ctx, &(ctx->chroma_qp_table_struct), param->codec_bit_depth);
    }
    else
    {
        const int * qp_chroma_ajudst = xeve_tbl_qp_chroma_ajudst;
        xeve_mcpy(&(ctx->qp_chroma_dynamic_ext[0][6 *( param->codec_bit_depth - 8)]), qp_chroma_ajudst, XEVE_MAX_QP_TABLE_SIZE * sizeof(int));
        xeve_mcpy(&(ctx->qp_chroma_dynamic_ext[1][6 * (param->codec_bit_depth - 8)]), qp_chroma_ajudst, XEVE_MAX_QP_TABLE_SIZE * sizeof(int));
    }

#if 0
    if (param->vbv_bufsize == 0)
    {
        param->vbv_bufsize = (int)((param->bitrate) * (param->vbv_msec / 1000.0));
    }
#endif

    parse_tile_slice_param(ctx);

    return XEVE_OK;
}

static void decide_normal_gop(XEVE_CTX * ctx, u32 pic_imcnt)
{
    int i_period, gop_size, pos;
    u32        pic_icnt_b;

    i_period = ctx->param.keyint;
    gop_size = ctx->param.gop_size;

    if (i_period == 0 && pic_imcnt == 0)
    {
        ctx->slice_type = SLICE_I;
        ctx->slice_depth = FRM_DEPTH_0;
        ctx->poc.poc_val = pic_imcnt;
        ctx->poc.prev_doc_offset = 0;
        ctx->poc.prev_poc_val = ctx->poc.poc_val;
        ctx->slice_ref_flag = 1;
    }
    else if ((i_period != 0) && pic_imcnt % i_period == 0)
    {
        ctx->slice_type = SLICE_I;
        ctx->slice_depth = FRM_DEPTH_0;
        ctx->poc.poc_val = pic_imcnt;
        ctx->poc.prev_doc_offset = 0;
        ctx->poc.prev_poc_val = ctx->poc.poc_val;
        ctx->slice_ref_flag = 1;
    }
    else if (pic_imcnt % gop_size == 0)
    {
        ctx->slice_type = ctx->param.inter_slice_type;
        ctx->slice_ref_flag = 1;
        ctx->slice_depth = FRM_DEPTH_1;
        ctx->poc.poc_val = pic_imcnt;
        ctx->poc.prev_doc_offset = 0;
        ctx->poc.prev_poc_val = ctx->poc.poc_val;
        ctx->slice_ref_flag = 1;
    }
    else
    {
        ctx->slice_type = ctx->param.inter_slice_type;
        if (ctx->param.disable_hgop == 0)
        {
            pos = (pic_imcnt % gop_size) - 1;

            if (ctx->sps.tool_pocs)
            {
                ctx->fn_pocs(ctx, pic_imcnt, gop_size, pos);
            }
            else
            {
                ctx->slice_depth = xeve_tbl_slice_depth[gop_size >> 2][pos];
                int tid = ctx->slice_depth - (ctx->slice_depth > 0);
                xeve_poc_derivation(ctx->sps, tid, &ctx->poc);
            }
            if (!ctx->sps.tool_pocs && gop_size >= 2)
            {
                ctx->slice_ref_flag = (ctx->slice_depth == xeve_tbl_slice_depth[gop_size >> 2][gop_size - 2] ? 0 : 1);
            }
            else
            {
                ctx->slice_ref_flag = 1;
            }
        }
        else
        {
            pos = (pic_imcnt % gop_size) - 1;
            ctx->slice_depth = FRM_DEPTH_2;
            ctx->poc.poc_val = ((pic_imcnt / gop_size) * gop_size) - gop_size + pos + 1;
            ctx->slice_ref_flag = 0;
        }
        /* find current encoding picture's(B picture) pic_icnt */
        pic_icnt_b = ctx->poc.poc_val;

        /* find pico again here */
        ctx->pico_idx = (u8)(pic_icnt_b % ctx->pico_max_cnt);
        ctx->pico = ctx->pico_buf[ctx->pico_idx];

        PIC_ORIG(ctx) = &ctx->pico->pic;
    }
}


/* slice_type / slice_depth / poc / PIC_ORIG setting */
static void decide_slice_type(XEVE_CTX * ctx)
{
    u32 pic_imcnt, pic_icnt;
    int i_period, gop_size;
    int force_cnt = 0;

    i_period = ctx->param.keyint;
    gop_size = ctx->param.gop_size;
    pic_icnt = (ctx->pic_cnt + ctx->param.bframes);
    pic_imcnt = pic_icnt;
    ctx->pico_idx = pic_icnt % ctx->pico_max_cnt;
    ctx->pico = ctx->pico_buf[ctx->pico_idx];
    PIC_ORIG(ctx) = &ctx->pico->pic;

    if(gop_size == 1)
    {
        if (i_period == 1) /* IIII... */
        {
            ctx->slice_type = SLICE_I;
            ctx->slice_depth = FRM_DEPTH_0;
            ctx->poc.poc_val = pic_icnt;
            ctx->slice_ref_flag = 0;
        }
        else /* IPPP... */
        {
            pic_imcnt = (i_period > 0) ? pic_icnt % i_period : pic_icnt;
            if (pic_imcnt == 0)
            {
                ctx->slice_type = SLICE_I;
                ctx->slice_depth = FRM_DEPTH_0;
                ctx->slice_ref_flag = 1;
            }
            else
            {
                ctx->slice_type = ctx->param.inter_slice_type;

                if (ctx->param.disable_hgop == 0)
                {
                    ctx->slice_depth = xeve_tbl_slice_depth_P[ctx->param.ref_pic_gap_length >> 2][(pic_imcnt - 1) % ctx->param.ref_pic_gap_length];
                }
                else
                {
                    ctx->slice_depth = FRM_DEPTH_1;
                }
                ctx->slice_ref_flag = 1;
            }
            ctx->poc.poc_val = (ctx->param.closed_gop && i_period > 0 && (ctx->pic_cnt % i_period) == 0 ?
                                0 : (ctx->param.closed_gop ? ctx->pic_cnt % i_period : ctx->pic_cnt));
        }
    }
    else /* include B Picture (gop_size = 2 or 4 or 8 or 16) */
    {
        if(pic_icnt == gop_size - 1) /* special case when sequence start */
        {
            ctx->slice_type = SLICE_I;
            ctx->slice_depth = FRM_DEPTH_0;
            ctx->poc.poc_val = 0;
            ctx->poc.prev_doc_offset = 0;
            ctx->poc.prev_poc_val = ctx->poc.poc_val;
            ctx->slice_ref_flag = 1;

            /* flush the first IDR picture */
            PIC_ORIG(ctx) = &ctx->pico_buf[0]->pic;
            ctx->pico = ctx->pico_buf[0];
        }
        else if(ctx->force_slice)
        {
            for(force_cnt = ctx->force_ignored_cnt; force_cnt < gop_size; force_cnt++)
            {
                pic_icnt = (ctx->pic_cnt + ctx->param.bframes + force_cnt);
                pic_imcnt = pic_icnt;

                decide_normal_gop(ctx, pic_imcnt);

                if(ctx->poc.poc_val <= (int)ctx->pic_ticnt)
                {
                    break;
                }
            }
            ctx->force_ignored_cnt = force_cnt;
        }
        else /* normal GOP case */
        {
            decide_normal_gop(ctx, pic_imcnt);
        }
    }
    if (ctx->param.disable_hgop == 0 && gop_size > 1)
    {
        ctx->nalu.nuh_temporal_id = ctx->slice_depth - (ctx->slice_depth > 0);
    }
    else
    {
        ctx->nalu.nuh_temporal_id = 0;
    }
    if (ctx->slice_type == SLICE_I && ctx->param.closed_gop)
    {
        ctx->poc.prev_idr_poc = ctx->poc.poc_val;
    }
}


int xeve_pic_prepare(XEVE_CTX * ctx, XEVE_BITB * bitb, XEVE_STAT * stat)
{
    int             ret;
    int             size;

    xeve_assert_rv(PIC_ORIG(ctx) != NULL, XEVE_ERR_UNEXPECTED);

    ctx->qp = (u8)ctx->param.qp;

    PIC_CURR(ctx) = xeve_picman_get_empty_pic(&ctx->rpm, &ret);
    xeve_assert_rv(PIC_CURR(ctx) != NULL, ret);
    ctx->map_refi = PIC_CURR(ctx)->map_refi;
    ctx->map_mv = PIC_CURR(ctx)->map_mv;
    ctx->map_unrefined_mv = PIC_CURR(ctx)->map_unrefined_mv;
    ctx->map_dqp_lah = ctx->pico->sinfo.map_qp_scu;

    PIC_MODE(ctx) = PIC_CURR(ctx);
    if(ctx->pic_dbk == NULL)
    {
        ctx->pic_dbk = xeve_pic_alloc(&ctx->rpm.pa, &ret);
        xeve_assert_rv(ctx->pic_dbk != NULL, ret);
    }

    decide_slice_type(ctx);

    ctx->lcu_cnt = ctx->f_lcu;
    ctx->slice_num = 0;

    if (ctx->tile_cnt == 1 && ctx->param.threads > 1)
    {
        for (u32 i = 0; i < ctx->f_lcu; i++)
        {
            ctx->sync_flag[i] = 0; //Reset the sync flag at the begining of each frame
        }
    }

    if(ctx->slice_type == SLICE_I) ctx->last_intra_poc = ctx->poc.poc_val;

    size = sizeof(s8) * ctx->f_scu * REFP_NUM;
    xeve_mset_x64a(ctx->map_refi, -1, size);
    size = sizeof(s16) * ctx->f_scu * REFP_NUM * MV_D;
    xeve_mset_x64a(ctx->map_mv, 0, size);
    size = sizeof(s16) * ctx->f_scu * REFP_NUM * MV_D;
    xeve_mset_x64a(ctx->map_unrefined_mv, 0, size);

    /* initialize bitstream container */
    xeve_bsw_init(&ctx->bs[0], bitb->addr, bitb->bsize, NULL);
    ctx->bs[0].pdata[1] = &ctx->sbac_enc[0];
    for (int i = 0; i < ctx->param.threads; i++)
    {
        xeve_bsw_init(&ctx->bs[i], ctx->bs[i].beg, bitb->bsize, NULL);
    }

    /* clear map */
    xeve_mset_x64a(ctx->map_scu, 0, sizeof(u32) * ctx->f_scu);
    xeve_mset_x64a(ctx->map_cu_mode, 0, sizeof(u32) * ctx->f_scu);

    xeve_set_active_pps_info(ctx);
    if (ctx->param.rc_type != 0)
    {
        ctx->qp = xeve_rc_get_qp(ctx);
    }

    return XEVE_OK;
}

int xeve_pic_finish(XEVE_CTX *ctx, XEVE_BITB *bitb, XEVE_STAT *stat)
{
    XEVE_IMGB *imgb_o, *imgb_c;
    int        ret;
    int        i, j;

    xeve_mset(stat, 0, sizeof(XEVE_STAT));

    /* adding picture sign */
    if (ctx->param.use_pic_sign)
    {
        XEVE_BSW * bs = &ctx->bs[0];
        XEVE_NALU sei_nalu;
        xeve_set_nalu(&sei_nalu, XEVE_SEI_NUT, ctx->nalu.nuh_temporal_id);

        u8* size_field = bs->cur;
        u8* cur_tmp = bs->cur;

        xeve_eco_nalu(bs, &sei_nalu);

        ret = xeve_eco_sei(ctx, bs);
        xeve_assert_rv(ret == XEVE_OK, ret);

        xeve_bsw_deinit(bs);
        stat->sei_size = (int)(bs->cur - cur_tmp);
        xeve_eco_nalu_len_update(size_field, stat->sei_size - 4);
    }

    /* expand current encoding picture, if needs */
    ctx->fn_picbuf_expand(ctx, PIC_CURR(ctx));

    /* picture buffer management */
    ret = xeve_picman_put_pic(&ctx->rpm, PIC_CURR(ctx), ctx->nalu.nal_unit_type_plus1 - 1 == XEVE_IDR_NUT,
                              ctx->poc.poc_val, ctx->nalu.nuh_temporal_id, 0, ctx->refp,
                              ctx->slice_ref_flag, ctx->sps.tool_rpl, ctx->param.ref_pic_gap_length);

    xeve_assert_rv(ret == XEVE_OK, ret);

    imgb_o = PIC_ORIG(ctx)->imgb;
    xeve_assert(imgb_o != NULL);

    imgb_c = PIC_CURR(ctx)->imgb;
    xeve_assert(imgb_c != NULL);

    /* set stat */
    stat->write = XEVE_BSW_GET_WRITE_BYTE(&ctx->bs[0]);
    stat->nalu_type = (ctx->slice_type == SLICE_I && ctx->param.closed_gop) ? XEVE_IDR_NUT : XEVE_NONIDR_NUT;
    stat->stype = ctx->slice_type;
    stat->fnum = ctx->pic_cnt;
    stat->qp = ctx->sh->qp;
    stat->poc = ctx->poc.poc_val;
    stat->tid = ctx->nalu.nuh_temporal_id;

    for(i = 0; i < 2; i++)
    {
        stat->refpic_num[i] = ctx->rpm.num_refp[i];
        for (j = 0; j < stat->refpic_num[i]; j++)
        {
            stat->refpic[i][j] = ctx->refp[j][i].poc;
        }
    }

    ctx->pic_cnt++; /* increase picture count */
    ctx->param.f_ifrm = 0; /* clear force-IDR flag */
    ctx->pico->is_used = 0;

    imgb_c->ts[0] = bitb->ts[0] = imgb_o->ts[0];
    imgb_c->ts[1] = bitb->ts[1] = imgb_o->ts[1];
    imgb_c->ts[2] = bitb->ts[2] = imgb_o->ts[2];
    imgb_c->ts[3] = bitb->ts[3] = imgb_o->ts[3];

    if (ctx->param.rc_type != 0)
    {
        ctx->rcore->real_bits = (stat->write - stat->sei_size) << 3;
    }

    if (imgb_o)
    {
        imgb_o->release(imgb_o);
    }

    return XEVE_OK;
}

void xeve_set_nalu(XEVE_NALU * nalu, int nalu_type, int nuh_temporal_id)
{
    nalu->nal_unit_size = 0;
    nalu->forbidden_zero_bit = 0;
    nalu->nal_unit_type_plus1 = nalu_type + 1;
    nalu->nuh_temporal_id = nuh_temporal_id;
    nalu->nuh_reserved_zero_5bits = 0;
    nalu->nuh_extension_flag = 0;
}

void xeve_set_vui(XEVE_CTX * ctx, XEVE_VUI * vui)
{
    vui->aspect_ratio_info_present_flag = 1;
    vui->aspect_ratio_idc = 1;
    vui->sar_width = 1;
    vui->sar_height = 1;
    vui->overscan_info_present_flag = 1;
    vui->overscan_appropriate_flag = 1;
    vui->video_signal_type_present_flag = 1;
    vui->video_format = 1;
    vui->video_full_range_flag = 1;
    vui->colour_description_present_flag = 1;
    vui->colour_primaries = 1;
    vui->transfer_characteristics = 1;
    vui->matrix_coefficients = 1;
    vui->chroma_loc_info_present_flag = 1;
    vui->chroma_sample_loc_type_top_field = 1;
    vui->chroma_sample_loc_type_bottom_field = 1;
    vui->neutral_chroma_indication_flag = 1;
    vui->field_seq_flag = 1;
    vui->timing_info_present_flag = 1;
    vui->num_units_in_tick = 1;
    vui->time_scale = 1;
    vui->fixed_pic_rate_flag = 1;
    vui->nal_hrd_parameters_present_flag = 1;
    vui->vcl_hrd_parameters_present_flag = 1;
    vui->low_delay_hrd_flag = 1;
    vui->pic_struct_present_flag = 1;
    vui->bitstream_restriction_flag = 1;
    vui->motion_vectors_over_pic_boundaries_flag = 1;
    vui->max_bytes_per_pic_denom = 1;
    vui->max_bits_per_mb_denom = 1;
    vui->log2_max_mv_length_horizontal = 1;
    vui->log2_max_mv_length_vertical = 1;
    vui->num_reorder_pics = 1;
    vui->max_dec_pic_buffering = 1;
    vui->hrd_parameters.cpb_cnt_minus1 = 1;
    vui->hrd_parameters.bit_rate_scale = 1;
    vui->hrd_parameters.cpb_size_scale = 1;
    xeve_mset(&(vui->hrd_parameters.bit_rate_value_minus1), 0, sizeof(int)*NUM_CPB);
    xeve_mset(&(vui->hrd_parameters.cpb_size_value_minus1), 0, sizeof(int)*NUM_CPB);
    xeve_mset(&(vui->hrd_parameters.cbr_flag), 0, sizeof(int)*NUM_CPB);
    vui->hrd_parameters.initial_cpb_removal_delay_length_minus1 = 1;
    vui->hrd_parameters.cpb_removal_delay_length_minus1 = 1;
    vui->hrd_parameters.dpb_output_delay_length_minus1 = 1;
    vui->hrd_parameters.time_offset_length = 1;
}

void xeve_set_sps(XEVE_CTX * ctx, XEVE_SPS * sps)
{
    xeve_mset(sps, 0, sizeof(XEVE_SPS));

    sps->profile_idc = ctx->param.profile;
    sps->level_idc = ctx->param.level_idc * 3;
    sps->pic_width_in_luma_samples = XEVE_ALIGN_VAL(ctx->param.w, 8);
    sps->pic_height_in_luma_samples = XEVE_ALIGN_VAL(ctx->param.h, 8);
    sps->toolset_idc_h = 0;
    sps->toolset_idc_l = 0;
    sps->bit_depth_luma_minus8 = ctx->param.codec_bit_depth - 8;
    sps->bit_depth_chroma_minus8 = ctx->param.codec_bit_depth - 8;
    sps->chroma_format_idc = XEVE_CFI_FROM_CF(XEVE_CS_GET_FORMAT(ctx->param.cs));
    sps->dquant_flag = 0;
    sps->log2_max_pic_order_cnt_lsb_minus4 = POC_LSB_BIT - 4;
    sps->sps_max_dec_pic_buffering_minus1 = 0; //[TBF]

    if (ctx->param.bframes > 0)
    {
        sps->max_num_ref_pics = ctx->param.me_ref_num;
    }
    else
    {
        sps->max_num_ref_pics = ctx->param.ref_pic_gap_length;;
    }

    sps->log2_sub_gop_length = (int)(log2(ctx->param.gop_size) + .5);
    sps->log2_ref_pic_gap_length = (int)(log2(ctx->param.ref_pic_gap_length) + .5);
    sps->long_term_ref_pics_flag = 0;
    sps->vui_parameters_present_flag = 0;
    xeve_set_vui(ctx, &(sps->vui_parameters));

    if (ctx->chroma_qp_table_struct.chroma_qp_table_present_flag)
    {
        xeve_copy_chroma_qp_mapping_params(&(sps->chroma_qp_table_struct), &(ctx->chroma_qp_table_struct));
    }

    sps->picture_cropping_flag = ctx->param.picture_cropping_flag;
    if (sps->picture_cropping_flag)
    {
        sps->picture_crop_left_offset = ctx->param.picture_crop_left_offset;
        sps->picture_crop_right_offset = ctx->param.picture_crop_right_offset;
        sps->picture_crop_top_offset = ctx->param.picture_crop_top_offset;
        sps->picture_crop_bottom_offset = ctx->param.picture_crop_bottom_offset;
    }
}

int xeve_set_active_pps_info(XEVE_CTX * ctx)
{
    int active_pps_id = ctx->sh->slice_pic_parameter_set_id;
    xeve_mcpy(&(ctx->pps), &(ctx->pps_array[active_pps_id]), sizeof(XEVE_PPS));

    return XEVE_OK;
}

void xeve_set_pps(XEVE_CTX * ctx, XEVE_PPS * pps)
{
    pps->loop_filter_across_tiles_enabled_flag = 0;
    pps->single_tile_in_pic_flag = 1;
    pps->constrained_intra_pred_flag = ctx->param.constrained_intra_pred;
    pps->cu_qp_delta_enabled_flag = (ctx->param.aq_mode || ctx->param.cutree);

    pps->num_ref_idx_default_active_minus1[REFP_0] = 0;
    pps->num_ref_idx_default_active_minus1[REFP_1] = 0;

    ctx->pps.pps_pic_parameter_set_id = 0;
    xeve_mcpy(&ctx->pps_array[ctx->pps.pps_pic_parameter_set_id], &ctx->pps, sizeof(XEVE_PPS));
}

void xeve_set_sh(XEVE_CTX *ctx, XEVE_SH *sh)
{
    double qp;
    int qp_l_i;
    int qp_c_i;

    const QP_ADAPT_PARAM *qp_adapt_param = ctx->param.bframes == 0 ?
        (ctx->param.keyint == 1 ? xeve_qp_adapt_param_ai : xeve_qp_adapt_param_ld) : xeve_qp_adapt_param_ra;

    sh->slice_type = ctx->slice_type;
    sh->no_output_of_prior_pics_flag = 0;
    sh->deblocking_filter_on = (ctx->param.use_deblock) ? 1 : 0;
    sh->sh_deblock_alpha_offset = ctx->param.deblock_alpha_offset;
    sh->sh_deblock_beta_offset = ctx->param.deblock_beta_offset;
    sh->single_tile_in_slice_flag = 1;
    sh->collocated_from_list_idx = (sh->slice_type == SLICE_P) ? REFP_0 : REFP_1;  // Specifies source (List ID) of the collocated picture, equialent of the collocated_from_l0_flag
    sh->collocated_from_ref_idx = 0;        // Specifies source (RefID_ of the collocated picture, equialent of the collocated_ref_idx
    sh->collocated_mvp_source_list_idx = REFP_0;  // Specifies source (List ID) in collocated pic that provides MV information (Applicability is function of NoBackwardPredFlag)

    /* set lambda */
    qp = XEVE_CLIP3(0, MAX_QUANT, (ctx->param.qp_incread_frame != 0 && (int)(ctx->poc.poc_val) >= ctx->param.qp_incread_frame) ? ctx->qp + 1.0 : ctx->qp);
    sh->dqp = ctx->param.aq_mode != 0;

    if(ctx->param.disable_hgop == 0 && ctx->param.rc_type == 0)
    {
        double dqp_offset;
        int qp_offset;

        qp += qp_adapt_param[ctx->slice_depth].qp_offset_layer;
        dqp_offset = qp * qp_adapt_param[ctx->slice_depth].qp_offset_model_scale + qp_adapt_param[ctx->slice_depth].qp_offset_model_offset + 0.5;

        qp_offset = (int)floor(XEVE_CLIP3(0.0, 3.0, dqp_offset));
        qp += qp_offset;
    }

    sh->qp   = (u8)XEVE_CLIP3(0, MAX_QUANT, qp);
    sh->qp_u_offset = ctx->param.qp_cb_offset;
    sh->qp_v_offset = ctx->param.qp_cr_offset;
    sh->qp_u = (s8)XEVE_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, sh->qp + sh->qp_u_offset);
    sh->qp_v = (s8)XEVE_CLIP3(-6 * ctx->sps.bit_depth_chroma_minus8, 57, sh->qp + sh->qp_v_offset);

    qp_l_i = sh->qp;
    ctx->lambda[0] = 0.57 * pow(2.0, (qp_l_i - 12.0) / 3.0);
    qp_c_i = ctx->qp_chroma_dynamic[0][sh->qp_u];
    ctx->dist_chroma_weight[0] = pow(2.0, (qp_l_i - qp_c_i) / 3.0);
    qp_c_i = ctx->qp_chroma_dynamic[1][sh->qp_v];
    ctx->dist_chroma_weight[1] = pow(2.0, (qp_l_i - qp_c_i) / 3.0);
    ctx->lambda[1] = ctx->lambda[0] / ctx->dist_chroma_weight[0];
    ctx->lambda[2] = ctx->lambda[0] / ctx->dist_chroma_weight[1];
    ctx->sqrt_lambda[0] = sqrt(ctx->lambda[0]);
    ctx->sqrt_lambda[1] = sqrt(ctx->lambda[1]);
    ctx->sqrt_lambda[2] = sqrt(ctx->lambda[2]);

    ctx->sh->slice_pic_parameter_set_id = 0;
}

int xeve_set_tile_info(XEVE_CTX * ctx)
{
    XEVE_TILE  * tile;
    int          size, f_tile, tidx;

    ctx->tile_cnt = ctx->ts_info.tile_columns * ctx->ts_info.tile_rows;
    f_tile = ctx->ts_info.tile_columns * ctx->ts_info.tile_rows;

    ctx->tile_to_slice_map[0] = 0;
    /* alloc tile information */
    size = sizeof(XEVE_TILE) * f_tile;
    ctx->tile = xeve_malloc(size);
    xeve_assert_rv(ctx->tile, XEVE_ERR_OUT_OF_MEMORY);
    xeve_mset(ctx->tile, 0, size);

    /* update tile information - Tile width, height, First ctb address */
    tidx = 0;
    tile = &ctx->tile[tidx];
    tile->w_ctb = ctx->w_lcu;
    tile->h_ctb = ctx->h_lcu;
    tile->f_ctb = tile->w_ctb * tile->h_ctb;

    return XEVE_OK;
}

int xeve_ready(XEVE_CTX* ctx)
{
    XEVE_CORE* core = NULL;
    int          w, h, ret, i, f_blk;
    s32          size;
    XEVE_FCST* fcst = &ctx->fcst;

    xeve_assert(ctx);
    if (ctx->core[0] == NULL)
    {

        /* set various value */
        for (int i = 0; i < ctx->param.threads; i++)
        {
            core = xeve_core_alloc(ctx->param.chroma_format_idc);
            xeve_assert_gv(core != NULL, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
            ctx->core[i] = core;
        }
    }

    xeve_init_bits_est();

    if (ctx->w == 0)
    {
        w = ctx->w = XEVE_ALIGN_VAL(ctx->param.w, 8);
        h = ctx->h = XEVE_ALIGN_VAL(ctx->param.h, 8);
        ctx->f = w * h;
        if ((ctx->w != ctx->param.w) || (ctx->h != ctx->param.h))
        {
            ctx->param.picture_cropping_flag = 1;
            ctx->param.picture_crop_left_offset = 0;
            ctx->param.picture_crop_right_offset = (ctx->w - ctx->param.w + 1) >> 1;
            ctx->param.picture_crop_top_offset = 0;
            ctx->param.picture_crop_bottom_offset = (ctx->h - ctx->param.h + 1) >> 1;
        }

        ctx->max_cuwh = 64;
        ctx->min_cuwh = 1 << 2;
        ctx->log2_min_cuwh = 2;

        ctx->log2_max_cuwh = XEVE_LOG2(ctx->max_cuwh);
        ctx->max_cud = ctx->log2_max_cuwh - MIN_CU_LOG2;
        ctx->w_lcu = (w + ctx->max_cuwh - 1) >> ctx->log2_max_cuwh;
        ctx->h_lcu = (h + ctx->max_cuwh - 1) >> ctx->log2_max_cuwh;
        ctx->f_lcu = ctx->w_lcu * ctx->h_lcu;
        ctx->w_scu = (w + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
        ctx->h_scu = (h + ((1 << MIN_CU_LOG2) - 1)) >> MIN_CU_LOG2;
        ctx->f_scu = ctx->w_scu * ctx->h_scu;
        ctx->log2_culine = ctx->log2_max_cuwh - MIN_CU_LOG2;
        ctx->log2_cudim = ctx->log2_culine << 1;
    }

    if (ctx->param.rc_type != 0 || ctx->param.lookahead != 0 || ctx->param.use_fcst != 0)
    {
        xeve_rc_create(ctx);
    }
    else
    {
        ctx->rc = NULL;
        ctx->rcore = NULL;
        ctx->qp = ctx->param.qp;
    }

    //initialize the threads to NULL
    for (int i = 0; i < XEVE_MAX_THREADS; i++)
    {
        ctx->thread_pool[i] = 0;
    }

    //get the context synchronization handle
    ctx->sync_block = get_synchronized_object();
    xeve_assert_gv(ctx->sync_block != NULL, ret, XEVE_ERR_UNKNOWN, ERR);

    if (ctx->param.threads >= 1)
    {
        ctx->tc = xeve_malloc(sizeof(THREAD_CONTROLLER));
        init_thread_controller(ctx->tc, ctx->param.threads);
        for (int i = 0; i < ctx->param.threads; i++)
        {
            ctx->thread_pool[i] = ctx->tc->create(ctx->tc, i);
            xeve_assert_gv(ctx->thread_pool[i] != NULL, ret, XEVE_ERR_UNKNOWN, ERR);
        }
    }

    size = ctx->f_lcu * sizeof(int);
    ctx->sync_flag = (volatile s32 *)xeve_malloc(size);
    xeve_assert_gv( ctx->sync_flag, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    for (int i = 0; i < (int)ctx->f_lcu; i++)
    {
        ctx->sync_flag[i] = 0;
    }

    /*  allocate CU data map*/
    if (ctx->map_cu_data == NULL)
    {
        size = sizeof(XEVE_CU_DATA) * ctx->f_lcu;
        ctx->map_cu_data = (XEVE_CU_DATA*)xeve_malloc_fast(size);
        xeve_assert_gv(ctx->map_cu_data, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset_x64a(ctx->map_cu_data, 0, size);

        for (i = 0; i < (int)ctx->f_lcu; i++)
        {
            xeve_create_cu_data(ctx->map_cu_data + i, ctx->log2_max_cuwh - MIN_CU_LOG2, ctx->log2_max_cuwh - MIN_CU_LOG2, ctx->param.chroma_format_idc);
        }
    }

    /* allocate maps */
    if (ctx->map_scu == NULL)
    {
        size = sizeof(u32) * ctx->f_scu;
        ctx->map_scu = xeve_malloc_fast(size);
        xeve_assert_gv(ctx->map_scu, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset_x64a(ctx->map_scu, 0, size);
    }

    if (ctx->map_ipm == NULL)
    {
        size = sizeof(s8) * ctx->f_scu;
        ctx->map_ipm = xeve_malloc_fast(size);
        xeve_assert_gv(ctx->map_ipm, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset(ctx->map_ipm, -1, size);
    }

    size = sizeof(s8) * ctx->f_scu;
    ctx->map_depth = xeve_malloc_fast(size);
    xeve_assert_gv(ctx->map_depth, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    xeve_mset(ctx->map_depth, -1, size);


    if (ctx->map_cu_mode == NULL)
    {
        size = sizeof(u32) * ctx->f_scu;
        ctx->map_cu_mode = xeve_malloc_fast(size);
        xeve_assert_gv(ctx->map_cu_mode, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset_x64a(ctx->map_cu_mode, 0, size);
    }

    /* initialize reference picture manager */
    ctx->pa.fn_alloc = xeve_pic_alloc;
    ctx->pa.fn_free = xeve_pic_free;
    ctx->pa.w = ctx->w;
    ctx->pa.h = ctx->h;
    ctx->pa.pad_l = PIC_PAD_SIZE_L;
    ctx->pa.pad_c = PIC_PAD_SIZE_L >> ctx->param.cs_h_shift;
    ctx->pa.bit_depth = ctx->param.codec_bit_depth;
    ctx->pic_cnt = 0;
    ctx->pic_icnt = -1;
    ctx->poc.poc_val = 0;
    ctx->pa.chroma_format_idc = ctx->param.chroma_format_idc;

    ret = xeve_picman_init(&ctx->rpm, MAX_PB_SIZE, XEVE_MAX_NUM_REF_PICS, &ctx->pa);
    xeve_assert_g(XEVE_SUCCEEDED(ret), ERR);

    if (ctx->param.gop_size == 1 && ctx->param.keyint != 1) //LD case
    {
        ctx->pico_max_cnt = 2;
    }
    else //RA case
    {
        ctx->pico_max_cnt = 1 + ((ctx->param.gop_size) << 1);
    }

    if (ctx->param.bframes)
    {
        ctx->frm_rnum = XEVE_MAX(ctx->param.bframes + 1, ctx->param.lookahead);
    }
    else
    {
        ctx->frm_rnum = 0;
    }

    ctx->qp = ctx->param.qp;
    if (ctx->param.use_fcst)
    {
        fcst->log2_fcst_blk_spic = 4; /* 16x16 in half image*/
        fcst->w_blk = (ctx->w/2 + (((1 << (fcst->log2_fcst_blk_spic + 1)) - 1))) >> (fcst->log2_fcst_blk_spic + 1);
        fcst->h_blk = (ctx->h/2 + (((1 << (fcst->log2_fcst_blk_spic + 1)) - 1))) >> (fcst->log2_fcst_blk_spic + 1);
        fcst->f_blk = fcst->w_blk * fcst->h_blk;
    }

    for (i = 0; i < ctx->pico_max_cnt; i++)
    {
        ctx->pico_buf[i] = (XEVE_PICO*)xeve_malloc(sizeof(XEVE_PICO));
        xeve_assert_gv(ctx->pico_buf[i], ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset(ctx->pico_buf[i], 0, sizeof(XEVE_PICO));

        XEVE_PICO *pico;
        pico = ctx->pico_buf[i];

        pico->spic = xeve_alloc_spic_l(ctx->w, ctx->h);
        xeve_assert_g(pico->spic != NULL, ERR);

        if (ctx->param.use_fcst)
        {
            ctx->pico_buf[i]->spic = xeve_alloc_spic_l(ctx->w, ctx->h);

            f_blk = ctx->fcst.f_blk;
            size = sizeof(u8) * f_blk;
            ctx->pico_buf[i]->sinfo.map_pdir = xeve_malloc(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.map_pdir, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

            size = sizeof(u8) * f_blk;
            ctx->pico_buf[i]->sinfo.map_pdir_bi = xeve_malloc(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.map_pdir_bi, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

            size = sizeof(s16) * f_blk * PRED_BI * MV_D;
            ctx->pico_buf[i]->sinfo.map_mv = xeve_malloc(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.map_mv, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

            size = sizeof(s16) * f_blk * PRED_BI * MV_D;
            ctx->pico_buf[i]->sinfo.map_mv_bi = xeve_malloc(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.map_mv_bi, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

            size = sizeof(s16) * f_blk * PRED_BI * MV_D;
            ctx->pico_buf[i]->sinfo.map_mv_pga = xeve_malloc(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.map_mv_pga, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

            size = sizeof(s32) * f_blk * 4;
            ctx->pico_buf[i]->sinfo.map_uni_lcost = xeve_malloc(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.map_uni_lcost, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

            size = sizeof(s32) * f_blk;
            ctx->pico_buf[i]->sinfo.map_bi_lcost = xeve_malloc(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.map_bi_lcost, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

            size = sizeof(s32) * f_blk;
            ctx->pico_buf[i]->sinfo.map_qp_blk = xeve_malloc(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.map_qp_blk, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

            size = sizeof(s8) * ctx->f_scu;
            ctx->pico_buf[i]->sinfo.map_qp_scu = xeve_malloc_fast(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.map_qp_scu, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

            size = sizeof(u16) * f_blk;
            ctx->pico_buf[i]->sinfo.transfer_cost = xeve_malloc(size);
            xeve_assert_gv(ctx->pico_buf[i]->sinfo.transfer_cost, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        }
    }

    /* alloc tile index map in SCU unit */
    if (ctx->map_tidx == NULL)
    {
        size = sizeof(u8) * ctx->f_scu;
        ctx->map_tidx = (u8*)xeve_malloc_fast(size);
        xeve_assert_gv(ctx->map_tidx, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset_x64a(ctx->map_tidx, 0, size);
    }

    if (ctx->tile == NULL)
    {
        ret = ctx->fn_set_tile_info(ctx);
        if (ret != XEVE_OK)
        {
            goto ERR;
        }
    }

    ctx->sh_array = (XEVE_SH*)xeve_malloc(sizeof(XEVE_SH) * ctx->ts_info.num_slice_in_pic);
    xeve_assert_gv(ctx->sh_array, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    xeve_mset(ctx->sh_array, 0, sizeof(XEVE_SH) * ctx->ts_info.num_slice_in_pic);
    ctx->sh = &ctx->sh_array[0];

    return XEVE_OK;
ERR:
    for (i = 0; i < (int)ctx->f_lcu; i++)
    {
        xeve_delete_cu_data(ctx->map_cu_data + i, ctx->log2_max_cuwh - MIN_CU_LOG2, ctx->log2_max_cuwh - MIN_CU_LOG2);
    }

    xeve_mfree_fast(ctx->map_cu_data);
    xeve_mfree_fast(ctx->map_ipm);
    xeve_mfree_fast(ctx->map_depth);
    xeve_mfree_fast(ctx->map_cu_mode);
    xeve_mfree_fast(ctx->sh_array);

    //free the threadpool and created thread if any
    if (ctx->sync_block)
    {
        release_synchornized_object(&ctx->sync_block);
    }

    if (ctx->param.threads >= 1)
    {
        if (ctx->tc)
        {
            //thread controller instance is present
            //terminate the created thread
            for (int i = 0; i < ctx->param.threads; i++)
            {
                if (ctx->thread_pool[i])
                {
                    //valid thread instance
                    ctx->tc->release(&ctx->thread_pool[i]);
                }
            }
            //dinitialize the tc
            dinit_thread_controller(ctx->tc);
            xeve_mfree_fast(ctx->tc);
            ctx->tc = 0;
        }
    }

    xeve_mfree_fast((void*)ctx->sync_flag);

    if (ctx->param.use_fcst)
    {
        for (i = 0; i < ctx->pico_max_cnt; i++)
        {
            xeve_picbuf_rc_free(ctx->pico_buf[i]->spic);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_pdir);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_pdir_bi);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_mv);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_mv_bi);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_mv_pga);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_uni_lcost);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_bi_lcost);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_qp_blk);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_qp_scu);
            xeve_mfree_fast(ctx->pico_buf[i]->sinfo.transfer_cost);
            xeve_mfree_fast(ctx->pico_buf[i]);
        }
    }

    if (core)
    {
        xeve_core_free(core);
    }

    if (ctx->param.rc_type != 0 || ctx->param.lookahead != 0)
    {
        xeve_rc_delete(ctx);
    }

    return ret;
}

void xeve_flush(XEVE_CTX * ctx)
{
    int i;
    xeve_assert(ctx);

    xeve_mfree_fast(ctx->map_scu);
    for(i = 0; i < (int)ctx->f_lcu; i++)
    {
        xeve_delete_cu_data(ctx->map_cu_data + i, ctx->log2_max_cuwh - MIN_CU_LOG2, ctx->log2_max_cuwh - MIN_CU_LOG2);
    }
    xeve_mfree_fast(ctx->map_cu_data);
    xeve_mfree_fast(ctx->map_ipm);
    xeve_mfree_fast(ctx->map_depth);
    xeve_mfree_fast(ctx->sh_array);

    //release the sync block
    if (ctx->sync_block)
    {
        release_synchornized_object(&ctx->sync_block);
    }

    //Release thread pool controller and created threads
    if (ctx->param.threads >= 1)
    {
        if(ctx->tc)
        {
            //thread controller instance is present
            //terminate the created thread
            for (int i = 0; i < ctx->param.threads; i++)
            {
                if(ctx->thread_pool[i])
                {
                    //valid thread instance
                    ctx->tc->release(&ctx->thread_pool[i]);
                }
            }
            //dinitialize the tc
            dinit_thread_controller(ctx->tc);
            xeve_mfree_fast(ctx->tc);
            ctx->tc = 0;
        }
    }

        xeve_mfree_fast((void*) ctx->sync_flag);

    xeve_mfree_fast(ctx->map_cu_mode);
    xeve_picbuf_free(ctx->pic_dbk);
    xeve_picman_deinit(&ctx->rpm);

    for (int i = 0; i < ctx->param.threads; i++)
    {
        xeve_core_free(ctx->core[i]);
    }

    for(i = 0; i < ctx->pico_max_cnt; i++)
    {
        xeve_picbuf_rc_free(ctx->pico_buf[i]->spic);

        xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_pdir);
        xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_pdir_bi);
        xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_mv);
        xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_mv_bi);
        xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_mv_pga);
        xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_uni_lcost);
        xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_bi_lcost);
        xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_qp_blk);
        xeve_mfree_fast(ctx->pico_buf[i]->sinfo.map_qp_scu);
        xeve_mfree_fast(ctx->pico_buf[i]->sinfo.transfer_cost);
        xeve_mfree_fast(ctx->pico_buf[i]);
    }

    for(i = 0; i < XEVE_MAX_INBUF_CNT; i++)
    {
        if(ctx->inbuf[i]) ctx->inbuf[i]->release(ctx->inbuf[i]);
    }

    if (ctx->param.rc_type != 0 ||  ctx->param.aq_mode !=0)
    {
        xeve_rc_delete(ctx);
    }
}

int xeve_picbuf_get_inbuf(XEVE_CTX * ctx, XEVE_IMGB ** imgb)
{
    int i, opt, align[XEVE_IMGB_MAX_PLANE], pad[XEVE_IMGB_MAX_PLANE];

    for(i = 0; i < XEVE_MAX_INBUF_CNT; i++)
    {
        if(ctx->inbuf[i] == NULL)
        {
            opt = XEVE_IMGB_OPT_NONE;

            /* set align value*/
            align[0] = MIN_CU_SIZE;
            align[1] = MIN_CU_SIZE;
            align[2] = MIN_CU_SIZE;

            /* no padding */
            pad[0] = 0;
            pad[1] = 0;
            pad[2] = 0;

            int cs = ctx->param.chroma_format_idc == 0 ? XEVE_CS_YCBCR400_10LE : (ctx->param.chroma_format_idc == 1 ? XEVE_CS_YCBCR420_10LE :
                (ctx->param.chroma_format_idc == 2 ? XEVE_CS_YCBCR422_10LE : XEVE_CS_YCBCR444_10LE));
            *imgb = xeve_imgb_create(ctx->w, ctx->h, cs, opt, pad, align);
            xeve_assert_rv(*imgb != NULL, XEVE_ERR_OUT_OF_MEMORY);

            ctx->inbuf[i] = *imgb;

            (*imgb)->addref(*imgb);
            return XEVE_OK;
        }
        else if(ctx->inbuf[i]->getref(ctx->inbuf[i]) == 1)
        {
            *imgb = ctx->inbuf[i];

            (*imgb)->addref(*imgb);
            return XEVE_OK;
        }
    }

    return XEVE_ERR_UNEXPECTED;
}

int xeve_header(XEVE_CTX * ctx)
{
    int ret = XEVE_OK;

    /* encode parameter sets */
    if (ctx->pic_cnt == 0 || (ctx->slice_type == SLICE_I && ctx->param.closed_gop)) /* if nalu_type is IDR */
    {
        ret = ctx->fn_encode_sps(ctx);
        xeve_assert_rv(ret == XEVE_OK, ret);

        ret = ctx->fn_encode_pps(ctx);
        xeve_assert_rv(ret == XEVE_OK, ret);
    }

    return ret;
}

void xeve_update_core_loc_param(XEVE_CTX * ctx, XEVE_CORE * core)
{
    core->x_pel = core->x_lcu << ctx->log2_max_cuwh;  // entry point's x location in pixel
    core->y_pel = core->y_lcu << ctx->log2_max_cuwh;  // entry point's y location in pixel
    core->x_scu = core->x_lcu << (MAX_CU_LOG2 - MIN_CU_LOG2); // set x_scu location
    core->y_scu = core->y_lcu << (MAX_CU_LOG2 - MIN_CU_LOG2); // set y_scu location
    core->lcu_num = core->x_lcu + core->y_lcu*ctx->w_lcu; // Init the first lcu_num in tile
}

/* updating core location parameters for CTU parallel encoding case*/
void xeve_update_core_loc_param_mt(XEVE_CTX * ctx, XEVE_CORE * core)
{
    core->x_pel = core->x_lcu << ctx->log2_max_cuwh;  // entry point's x location in pixel
    core->y_pel = core->y_lcu << ctx->log2_max_cuwh;  // entry point's y location in pixel
    core->x_scu = core->x_lcu << (MAX_CU_LOG2 - MIN_CU_LOG2); // set x_scu location
    core->y_scu = core->y_lcu << (MAX_CU_LOG2 - MIN_CU_LOG2); // set y_scu location
}

int xeve_mt_get_next_ctu_num(XEVE_CTX * ctx, XEVE_CORE * core, int skip_ctb_line_cnt)
{
    int sp_x_lcu = ctx->tile[core->tile_num].ctba_rs_first % ctx->w_lcu;
    int sp_y_lcu = ctx->tile[core->tile_num].ctba_rs_first / ctx->w_lcu;
    core->x_lcu = (core->lcu_num) % ctx->w_lcu; //entry point lcu's x location

    /* check to move next ctb line */
    core->x_lcu++;
    if (core->x_lcu == sp_x_lcu + ctx->tile[core->tile_num].w_ctb)
    {
        core->x_lcu = sp_x_lcu;
        core->y_lcu += skip_ctb_line_cnt;
    }

    core->lcu_num = core->y_lcu * ctx->w_lcu + core->x_lcu;
    /* check to exceed height of ctb line */
    if (core->y_lcu >= sp_y_lcu + ctx->tile[core->tile_num].h_ctb)
    {
        return -1;
    }

    xeve_update_core_loc_param_mt(ctx, core);

    return core->lcu_num;
}

int xeve_init_core_mt(XEVE_CTX * ctx, int tile_num, XEVE_CORE * core, int thread_cnt)
{
    ctx->fn_mode_init_mt(ctx, thread_cnt);

    /********************* Core initialization *****************************/
    ctx->core[thread_cnt]->tile_num = tile_num;
    ctx->core[thread_cnt]->qp_y = core->qp_y;
    ctx->core[thread_cnt]->qp_u = core->qp_u;
    ctx->core[thread_cnt]->qp_v = core->qp_v;
    ctx->sh->qp_prev_eco = ctx->sh->qp;
    ctx->sh->qp_prev_mode = ctx->sh->qp;
    ctx->core[thread_cnt]->dqp_data[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].prev_qp = ctx->sh->qp_prev_mode;
    ctx->core[thread_cnt]->dqp_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].curr_qp = ctx->sh->qp;
    ctx->core[thread_cnt]->dqp_curr_best[ctx->log2_max_cuwh - 2][ctx->log2_max_cuwh - 2].prev_qp = ctx->sh->qp;
    ctx->core[thread_cnt]->ctx = ctx;
    ctx->core[thread_cnt]->bs_temp.pdata[1] = &ctx->core[thread_cnt]->s_temp_run;

    return XEVE_OK;
}

int xeve_deblock_mt(void * arg)
{
    XEVE_CORE * core = (XEVE_CORE *)arg;
    XEVE_CTX * ctx = core->ctx;
    int i = core->tile_num;
    ctx->fn_deblock(ctx, PIC_MODE(ctx), i, ctx->pps.loop_filter_across_tiles_enabled_flag, core);
    return XEVE_OK;
}

int xeve_loop_filter(XEVE_CTX * ctx, XEVE_CORE * core)
{
    int ret = XEVE_OK;

    if (ctx->sh->deblocking_filter_on)
    {
#if TRACE_DBF
        XEVE_TRACE_SET(1);
#endif
        for (int is_hor_edge = 0; is_hor_edge <= 1; is_hor_edge++)
        {
            for (u32 i = 0; i < ctx->f_scu; i++)
            {
                MCU_CLR_COD(ctx->map_scu[i]);
            }

            for (ctx->slice_num = 0; ctx->slice_num < ctx->ts_info.num_slice_in_pic; ctx->slice_num++)
            {
                ctx->sh = &ctx->sh_array[ctx->slice_num];
                u16 total_tiles_in_slice = ctx->sh->num_tiles_in_slice;
                THREAD_CONTROLLER * tc;
                int res;
                int i, k = 0;
                tc = ctx->tc;
                int parallel_task = 1;
                int thread_cnt = 0, thread_cnt1 = 0;;
                int task_completed = 0;

                while (total_tiles_in_slice)
                {
                    parallel_task = 1;// (ctx->param.threads > total_tiles_in_slice) ? total_tiles_in_slice : ctx->param.threads;
                    for (thread_cnt = 0; (thread_cnt < parallel_task - 1); thread_cnt++)
                    {
                        i = ctx->sh->tile_order[thread_cnt + task_completed];
                        ctx->core[thread_cnt]->thread_cnt = thread_cnt;
                        ctx->core[thread_cnt]->tile_num = i;
                        ctx->core[thread_cnt]->deblock_is_hor = is_hor_edge;

                        tc->run(ctx->thread_pool[thread_cnt], xeve_deblock_mt, (void*)ctx->core[thread_cnt]);
                    }
                    i = ctx->sh->tile_order[thread_cnt + task_completed];
                    ctx->core[thread_cnt]->thread_cnt = thread_cnt;
                    ctx->core[thread_cnt]->tile_num = i;
                    ctx->core[thread_cnt]->deblock_is_hor = is_hor_edge;

                    xeve_deblock_mt((void*)ctx->core[thread_cnt]);
                    for (thread_cnt1 = 0; thread_cnt1 < parallel_task - 1; thread_cnt1++)
                    {
                        tc->join(ctx->thread_pool[thread_cnt1], &res);
                        if (XEVE_FAILED(res))
                        {
                            ret = res;
                        }
                    }
                    total_tiles_in_slice -= parallel_task;
                    task_completed += parallel_task;
                }
                total_tiles_in_slice = ctx->sh->num_tiles_in_slice;
            }
#if TRACE_DBF
            XEVE_TRACE_SET(0);
#endif
        }
    }

    return ret;
}

void xeve_recon(XEVE_CTX * ctx, XEVE_CORE * core, s16 *coef, pel *pred, int is_coef, int cuw, int cuh, int s_rec, pel *rec, int bit_depth)
{
    xeve_recon_blk(coef, pred, is_coef, cuw, cuh, s_rec, rec, bit_depth);
}

int xeve_enc(XEVE_CTX * ctx, XEVE_BITB * bitb, XEVE_STAT * stat)
{
    int            ret;
    int            gop_size, pic_cnt;

    pic_cnt = ctx->pic_icnt - ctx->frm_rnum;
    gop_size = ctx->param.gop_size;

    ctx->force_slice = ((ctx->pic_ticnt % gop_size >= ctx->pic_ticnt - pic_cnt + 1) && FORCE_OUT(ctx)) ? 1 : 0;

    xeve_assert_rv(bitb->addr && bitb->bsize > 0, XEVE_ERR_INVALID_ARGUMENT);

    /* initialize variables for a picture encoding */
    ret = ctx->fn_enc_pic_prepare(ctx, bitb, stat);
    xeve_assert_rv(ret == XEVE_OK, ret);

    /* encode parameter set */
    ret = ctx->fn_enc_header(ctx);
    xeve_assert_rv(ret == XEVE_OK, ret);

    /* encode one picture */
    ret = ctx->fn_enc_pic(ctx, bitb, stat);
    xeve_assert_rv(ret == XEVE_OK, ret);

    /* finishing of encoding a picture */
    ctx->fn_enc_pic_finish(ctx, bitb, stat);
    xeve_assert_rv(ret == XEVE_OK, ret);

    return XEVE_OK;
}


int xeve_push_frm(XEVE_CTX * ctx, XEVE_IMGB * img)
{
    XEVE_PIC  * pic, * spic;
    XEVE_PICO * pico;
    XEVE_IMGB * imgb;

    int ret;

    ret = ctx->fn_get_inbuf(ctx, &imgb);
    xeve_assert_rv(XEVE_OK == ret, ret);

    imgb->cs = ctx->param.cs;
    xeve_imgb_cpy(imgb, img);

    if (ctx->fn_pic_flt != NULL)
    {
        ctx->fn_pic_flt(ctx, imgb);
    }

    ctx->pic_icnt++;
    ctx->pico_idx = ctx->pic_icnt % ctx->pico_max_cnt;
    pico = ctx->pico_buf[ctx->pico_idx];
    pico->pic_icnt = ctx->pic_icnt;
    pico->is_used = 1;
    pic = &pico->pic;
    ctx->pico = pico;

    PIC_ORIG(ctx) = pic;

    /* set pushed image to current input (original) image */
    xeve_mset(pic, 0, sizeof(XEVE_PIC));

    pic->buf_y = imgb->baddr[0];
    pic->buf_u = imgb->baddr[1];
    pic->buf_v = imgb->baddr[2];

    pic->y = imgb->a[0];
    pic->u = imgb->a[1];
    pic->v = imgb->a[2];

    pic->w_l = imgb->w[0];
    pic->h_l = imgb->h[0];
    pic->w_c = imgb->w[1];
    pic->h_c = imgb->h[1];

    pic->s_l = STRIDE_IMGB2PIC(imgb->s[0]);
    pic->s_c = STRIDE_IMGB2PIC(imgb->s[1]);

    pic->imgb = imgb;
    /* generate sub-picture for RC and Forecast */
    if (ctx->param.use_fcst)
    {
        spic = pico->spic;
        xeve_gen_subpic(pic->y, spic->y, spic->w_l, spic->h_l, pic->s_l, spic->s_l, 10);

        xeve_mset(pico->sinfo.map_pdir, 0, sizeof(u8) * ctx->fcst.f_blk);
        xeve_mset(pico->sinfo.map_pdir_bi, 0, sizeof(u8) * ctx->fcst.f_blk);
        xeve_mset(pico->sinfo.map_mv, 0, sizeof(s16) * ctx->fcst.f_blk * REFP_NUM * MV_D);
        xeve_mset(pico->sinfo.map_mv_bi, 0, sizeof(s16) * ctx->fcst.f_blk * REFP_NUM * MV_D);
        xeve_mset(pico->sinfo.map_mv_pga, 0, sizeof(s16) * ctx->fcst.f_blk * REFP_NUM * MV_D);
        xeve_mset(pico->sinfo.map_uni_lcost, 0, sizeof(s32) * ctx->fcst.f_blk * 4);
        xeve_mset(pico->sinfo.map_bi_lcost, 0, sizeof(s32) * ctx->fcst.f_blk);
        xeve_mset(pico->sinfo.map_qp_blk, 0, sizeof(s32) * ctx->fcst.f_blk);
        xeve_mset(pico->sinfo.map_qp_scu, 0, sizeof(s8) * ctx->f_scu);
        xeve_mset(pico->sinfo.transfer_cost, 0, sizeof(u16) * ctx->fcst.f_blk);
        xeve_picbuf_expand(spic, spic->pad_l, spic->pad_c, ctx->sps.chroma_format_idc);
    }
    return XEVE_OK;
}


void xeve_platform_init_func(XEVE_CTX * ctx)
{
#if X86_SSE
    int check_cpu, support_sse, support_avx, support_avx2;

    check_cpu = xeve_check_cpu_info();
    support_sse  = (check_cpu >> 1) & 1;
    support_avx  = check_cpu & 1;
    support_avx2 = (check_cpu >> 2) & 1;

    if (support_avx2)
    {
        xeve_func_sad               = xeve_tbl_sad_16b_avx;
        xeve_func_ssd               = xeve_tbl_ssd_16b_sse;
        xeve_func_diff              = xeve_tbl_diff_16b_sse;
        xeve_func_satd              = xeve_tbl_satd_16b_sse;
        xeve_func_mc_l              = xeve_tbl_mc_l_avx;
        xeve_func_mc_c              = xeve_tbl_mc_c_avx;
        xeve_func_average_no_clip   = &xeve_average_16b_no_clip_sse;
        ctx->fn_itxb                = &xeve_tbl_itxb_avx;
        xeve_func_txb               = &xeve_tbl_txb_avx;
    }
    else if (support_sse)
    {
        xeve_func_sad               = xeve_tbl_sad_16b_sse;
        xeve_func_ssd               = xeve_tbl_ssd_16b_sse;
        xeve_func_diff              = xeve_tbl_diff_16b_sse;
        xeve_func_satd              = xeve_tbl_satd_16b_sse;
        xeve_func_mc_l              = xeve_tbl_mc_l_sse;
        xeve_func_mc_c              = xeve_tbl_mc_c_sse;
        xeve_func_average_no_clip   = &xeve_average_16b_no_clip_sse;
        ctx->fn_itxb                = &xeve_tbl_itxb_sse;
        xeve_func_txb               = &xeve_tbl_txb; /*to be updated*/
    }
    else
#endif
    {
        xeve_func_sad               = xeve_tbl_sad_16b;
        xeve_func_ssd               = xeve_tbl_ssd_16b;
        xeve_func_diff              = xeve_tbl_diff_16b;
        xeve_func_satd              = xeve_tbl_satd_16b;
        xeve_func_mc_l              = xeve_tbl_mc_l;
        xeve_func_mc_c              = xeve_tbl_mc_c;
        xeve_func_average_no_clip   = &xeve_average_16b_no_clip;
        ctx->fn_itxb                = &xeve_tbl_itxb;
        xeve_func_txb               = &xeve_tbl_txb;
    }
}

int xeve_platform_init(XEVE_CTX * ctx)
{
    int ret = XEVE_ERR_UNKNOWN;

    /* create mode decision */
    ret = xeve_mode_create(ctx, 0);
    xeve_assert_rv(XEVE_OK == ret, ret);

    /* create intra prediction analyzer */
    ret = xeve_pintra_create(ctx, 0);
    xeve_assert_rv(XEVE_OK == ret, ret);

    /* create inter prediction analyzer */
    if (ctx->param.profile == XEVE_PROFILE_BASELINE)
    {
        ret = xeve_pinter_create(ctx, 0);
        xeve_assert_rv(XEVE_OK == ret, ret);
    }

    ctx->fn_ready             = xeve_ready;
    ctx->fn_flush             = xeve_flush;
    ctx->fn_enc               = xeve_enc;
    ctx->fn_enc_header        = xeve_header;
    ctx->fn_enc_pic           = xeve_pic;
    ctx->fn_enc_pic_prepare   = xeve_pic_prepare;
    ctx->fn_enc_pic_finish    = xeve_pic_finish;
    ctx->fn_push              = xeve_push_frm;
    ctx->fn_deblock           = xeve_deblock;
    ctx->fn_picbuf_expand     = xeve_pic_expand;
    ctx->fn_get_inbuf         = xeve_picbuf_get_inbuf;
    ctx->fn_loop_filter       = xeve_loop_filter;
    ctx->fn_encode_sps        = xeve_encode_sps;
    ctx->fn_encode_pps        = xeve_encode_pps;
    ctx->fn_eco_sh            = xeve_eco_sh;
    ctx->fn_eco_split_mode    = xeve_eco_split_mode;
    ctx->fn_eco_sbac_reset    = xeve_sbac_reset;
    ctx->fn_eco_coef          = xeve_eco_coef;
    ctx->fn_eco_pic_signature = xeve_eco_pic_signature;
    ctx->fn_tq                = xeve_sub_block_tq;
    ctx->fn_rdoq_set_ctx_cc   = xeve_rdoq_set_ctx_cc;
    ctx->fn_itdp              = xeve_itdq;
    ctx->fn_recon             = xeve_recon;
    ctx->fn_deblock_tree      = xeve_deblock_tree;
    ctx->fn_deblock_unit      = xeve_deblock_unit;
    ctx->fn_set_tile_info     = xeve_set_tile_info;
    ctx->fn_rdo_intra_ext     = NULL;
    ctx->fn_rdo_intra_ext_c   = NULL;
    ctx->pic_dbk              = NULL;
    ctx->fn_pocs              = NULL;
    ctx->fn_pic_flt           = NULL;
    ctx->pf                   = NULL;

    xeve_platform_init_func(ctx);

    return XEVE_OK;
}

void xeve_platform_deinit(XEVE_CTX * ctx)
{
    xeve_assert(ctx->pf == NULL);

    ctx->fn_ready = NULL;
    ctx->fn_flush = NULL;
    ctx->fn_enc = NULL;
    ctx->fn_enc_pic = NULL;
    ctx->fn_enc_pic_prepare = NULL;
    ctx->fn_enc_pic_finish = NULL;
    ctx->fn_push = NULL;
    ctx->fn_deblock = NULL;
    ctx->fn_picbuf_expand = NULL;
    ctx->fn_get_inbuf = NULL;
}

int xeve_create_bs_buf(XEVE_CTX  * ctx, int max_bs_buf_size)
{
    u8 * bs_buf, *bs_buf_temp;
    if (ctx->param.threads > 1)
    {
        bs_buf = (u8 *)xeve_malloc(sizeof(u8 *) * (ctx->param.threads - 1) * max_bs_buf_size);
        for (int task_id = 1; task_id < ctx->param.threads; task_id++)
        {
            bs_buf_temp = bs_buf + ((task_id - 1) * max_bs_buf_size);
            xeve_bsw_init(&ctx->bs[task_id], bs_buf_temp, max_bs_buf_size, NULL);
            ctx->bs[task_id].pdata[1] = &ctx->sbac_enc[task_id];
        }
    }
    return XEVE_OK;
}

int xeve_delete_bs_buf(XEVE_CTX  * ctx)
{
    if (ctx->param.threads > 1)
    {
        u8 * bs_buf_temp = ctx->bs[1].beg;
        if (bs_buf_temp != NULL)
        {
            xeve_mfree(bs_buf_temp);
        }
        bs_buf_temp = NULL;
    }
    return XEVE_OK;
}


int xeve_encode_sps(XEVE_CTX * ctx)
{
    XEVE_BSW * bs = &ctx->bs[0];
    XEVE_SPS * sps = &ctx->sps;
    XEVE_NALU  nalu;

    u8* size_field = bs->cur;
    u8* cur_tmp = bs->cur;

    /* nalu header */
    xeve_set_nalu(&nalu, XEVE_SPS_NUT, 0);
    xeve_eco_nalu(bs, &nalu);

    /* sequence parameter set*/
    xeve_set_sps(ctx, &ctx->sps);
    xeve_assert_rv(xeve_eco_sps(bs, sps) == XEVE_OK, XEVE_ERR_INVALID_ARGUMENT);

    /* de-init BSW */
    xeve_bsw_deinit(bs);

    /* write the bitstream size */
    xeve_eco_nalu_len_update(size_field, (int)(bs->cur - cur_tmp) - 4);

    return XEVE_OK;
}

int xeve_encode_pps(XEVE_CTX * ctx)
{
    XEVE_BSW * bs = &ctx->bs[0];
    XEVE_SPS * sps = &ctx->sps;
    XEVE_PPS * pps = &ctx->pps;
    XEVE_NALU  nalu;
    u8       * size_field = bs->cur;
    u8       * cur_tmp = bs->cur;

    /* nalu header */
    xeve_set_nalu(&nalu, XEVE_PPS_NUT, ctx->nalu.nuh_temporal_id);
    xeve_eco_nalu(bs, &nalu);

    /* sequence parameter set*/
    xeve_set_pps(ctx, &ctx->pps);
    xeve_assert_rv(xeve_eco_pps(bs, sps, pps) == XEVE_OK, XEVE_ERR_INVALID_ARGUMENT);

    /* de-init BSW */
    xeve_bsw_deinit(bs);

    /* write the bitstream size */
    xeve_eco_nalu_len_update(size_field, (int)(bs->cur - cur_tmp) - 4);
    return XEVE_OK;
}

int xeve_check_frame_delay(XEVE_CTX * ctx)
{
    if (ctx->pic_icnt < ctx->frm_rnum)
    {
        return XEVE_OK_OUT_NOT_AVAILABLE;
    }
    return XEVE_OK;
}

int xeve_check_more_frames(XEVE_CTX * ctx)
{
    XEVE_PICO  * pico;
    int           i;

    if(FORCE_OUT(ctx))
    {
        /* pseudo xeve_push() for bumping process ****************/
        ctx->pic_icnt++;
        /**********************************************************/

        for(i=0; i<ctx->pico_max_cnt; i++)
        {
            pico = ctx->pico_buf[i];
            if(pico != NULL)
            {
                if(pico->is_used == 1)
                {
                    return XEVE_OK;
                }
            }
        }

        return XEVE_OK_NO_MORE_FRM;
    }

    return XEVE_OK;
}

int xeve_malloc_1d(void** dst, int size)
{
    int ret;
    if(*dst == NULL)
    {
        *dst = xeve_malloc_fast(size);
        xeve_assert_gv(*dst, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset(*dst, 0, size);
    }
    return XEVE_OK;
ERR:
    return ret;
}

int xeve_malloc_2d(s8*** dst, int size_1d, int size_2d, int type_size)
{
    int i;
    int ret;

    if(*dst == NULL)
    {
        *dst = xeve_malloc_fast(size_1d * sizeof(s8*));
        xeve_assert_gv(*dst, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset(*dst, 0, size_1d * sizeof(s8*));


        (*dst)[0] = xeve_malloc_fast(size_1d * size_2d * type_size);
        xeve_assert_gv((*dst)[0], ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
        xeve_mset((*dst)[0], 0, size_1d * size_2d * type_size);

        for(i = 1; i < size_1d; i++)
        {
            (*dst)[i] = (*dst)[i - 1] + size_2d * type_size;
        }
    }
    return XEVE_OK;
ERR:
    return ret;
}

int xeve_create_cu_data(XEVE_CU_DATA *cu_data, int log2_cuw, int log2_cuh, int chroma_format_idc)
{
    int i, j, ret;
    int cuw_scu, cuh_scu;
    int size_8b, size_16b, size_32b, cu_cnt, pixel_cnt;
    int w_shift = XEVE_GET_CHROMA_W_SHIFT(chroma_format_idc);
    int h_shift = XEVE_GET_CHROMA_H_SHIFT(chroma_format_idc);

    cuw_scu = 1 << log2_cuw;
    cuh_scu = 1 << log2_cuh;

    size_8b = cuw_scu * cuh_scu * sizeof(s8);
    size_16b = cuw_scu * cuh_scu * sizeof(s16);
    size_32b = cuw_scu * cuh_scu * sizeof(s32);
    cu_cnt = cuw_scu * cuh_scu;
    pixel_cnt = cu_cnt << 4;

    ret = xeve_malloc_1d((void**)&cu_data->qp_y, size_8b);
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_1d((void**)&cu_data->qp_u, size_8b);
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_1d((void**)&cu_data->qp_v, size_8b);
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_1d((void**)&cu_data->pred_mode, size_8b);
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_1d((void**)&cu_data->pred_mode_chroma, size_8b);
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_2d((s8***)&cu_data->mpm, 2, cu_cnt, sizeof(u8));
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_2d((s8***)&cu_data->ipm, 2, cu_cnt, sizeof(u8));
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_2d((s8***)&cu_data->mpm_ext, 8, cu_cnt, sizeof(u8));
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_1d((void**)&cu_data->skip_flag, size_8b);
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_1d((void**)&cu_data->ibc_flag, size_8b);
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_1d((void**)&cu_data->dmvr_flag, size_8b);
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_2d((s8***)&cu_data->refi, cu_cnt, REFP_NUM, sizeof(u8));
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_2d((s8***)&cu_data->mvp_idx, cu_cnt, REFP_NUM, sizeof(u8));
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_1d((void**)&cu_data->mvr_idx, size_8b);
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_1d((void**)&cu_data->bi_idx, size_8b);
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_1d((void**)&cu_data->mmvd_idx, size_16b);
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_1d((void**)&cu_data->mmvd_flag, size_8b);
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_1d((void**)& cu_data->ats_intra_cu, size_8b);
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_1d((void**)& cu_data->ats_mode_h, size_8b);
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_1d((void**)& cu_data->ats_mode_v, size_8b);
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_1d((void**)&cu_data->ats_inter_info, size_8b);
    xeve_assert_g(ret == XEVE_OK, ERR);

    for(i = 0; i < N_C; i++)
    {
        ret = xeve_malloc_1d((void**)&cu_data->nnz[i], size_32b);
        xeve_assert_g(ret == XEVE_OK, ERR);
    }
    for (i = 0; i < N_C; i++)
    {
        for (j = 0; j < 4; j++)
        {
            ret = xeve_malloc_1d((void**)&cu_data->nnz_sub[i][j], size_32b);
            xeve_assert_g(ret == XEVE_OK, ERR);
        }
    }
    ret = xeve_malloc_1d((void**)&cu_data->map_scu, size_32b);
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_1d((void**)&cu_data->affine_flag, size_8b);
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_1d((void**)&cu_data->map_affine, size_32b);
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_1d((void**)&cu_data->map_cu_mode, size_32b);
    xeve_assert_g(ret == XEVE_OK, ERR);
    ret = xeve_malloc_1d((void**)&cu_data->depth, size_8b);
    xeve_assert_g(ret == XEVE_OK, ERR);

    for(i = Y_C; i < U_C; i++)
    {
        ret = xeve_malloc_1d((void**)&cu_data->coef[i], (pixel_cnt) * sizeof(s16));
        xeve_assert_g(ret == XEVE_OK, ERR);
        ret = xeve_malloc_1d((void**)&cu_data->reco[i], (pixel_cnt) * sizeof(pel));
        xeve_assert_g(ret == XEVE_OK, ERR);
    }
    for(i = U_C; i < N_C; i++)
    {
        ret = xeve_malloc_1d((void**)&cu_data->coef[i], (pixel_cnt >> (w_shift + h_shift)) * sizeof(s16));
        xeve_assert_g(ret == XEVE_OK, ERR);
        ret = xeve_malloc_1d((void**)&cu_data->reco[i], (pixel_cnt >> (w_shift + h_shift)) * sizeof(pel));
        xeve_assert_g(ret == XEVE_OK, ERR);
    }

    return XEVE_OK;

ERR:
    xeve_delete_cu_data(cu_data, log2_cuw, log2_cuh);
    return ret;
}

void xeve_free_1d(void* dst)
{
    if(dst != NULL)
    {
        xeve_mfree_fast(dst);
    }
}

void xeve_free_2d(void** dst)
{
    if (dst)
    {
        if (dst[0])
        {
            xeve_mfree_fast(dst[0]);
        }
        xeve_mfree_fast(dst);
    }
}

int xeve_delete_cu_data(XEVE_CU_DATA *cu_data, int log2_cuw, int log2_cuh)
{
    int i, j;

    xeve_free_1d((void*)cu_data->qp_y);
    xeve_free_1d((void*)cu_data->qp_u);
    xeve_free_1d((void*)cu_data->qp_v);
    xeve_free_1d((void*)cu_data->pred_mode);
    xeve_free_1d((void*)cu_data->pred_mode_chroma);
    xeve_free_2d((void**)cu_data->mpm);
    xeve_free_2d((void**)cu_data->ipm);
    xeve_free_2d((void**)cu_data->mpm_ext);
    xeve_free_1d((void*)cu_data->skip_flag);
    xeve_free_1d((void*)cu_data->ibc_flag);
    xeve_free_1d((void*)cu_data->dmvr_flag);
    xeve_free_2d((void**)cu_data->refi);
    xeve_free_2d((void**)cu_data->mvp_idx);
    xeve_free_1d(cu_data->mvr_idx);
    xeve_free_1d(cu_data->bi_idx);
    xeve_free_1d((void*)cu_data->mmvd_idx);
    xeve_free_1d((void*)cu_data->mmvd_flag);

    for (i = 0; i < N_C; i++)
    {
        xeve_free_1d((void*)cu_data->nnz[i]);
    }
    for (i = 0; i < N_C; i++)
    {
        for (j = 0; j < 4; j++)
        {
            xeve_free_1d((void*)cu_data->nnz_sub[i][j]);
        }
    }
    xeve_free_1d((void*)cu_data->map_scu);
    xeve_free_1d((void*)cu_data->affine_flag);
    xeve_free_1d((void*)cu_data->map_affine);
    xeve_free_1d((void*)cu_data->ats_intra_cu);
    xeve_free_1d((void*)cu_data->ats_mode_h);
    xeve_free_1d((void*)cu_data->ats_mode_v);
    xeve_free_1d((void*)cu_data->ats_inter_info);
    xeve_free_1d((void*)cu_data->map_cu_mode);
    xeve_free_1d((void*)cu_data->depth);

    for (i = 0; i < N_C; i++)
    {
        xeve_free_1d((void*)cu_data->coef[i]);
        xeve_free_1d((void*)cu_data->reco[i]);
    }

    return XEVE_OK;
}

void xeve_set_tile_in_slice(XEVE_CTX * ctx)
{
    XEVE_SH * sh = ctx->sh;
    int      tile_cnt = 0;
    for (int i = 0; i < ctx->slice_num; i++)
    {
        tile_cnt += ctx->sh_array[i].num_tiles_in_slice;
    }

    if (ctx->ts_info.num_slice_in_pic > 1)
    {
        xeve_mset(sh->tile_order, 0, sizeof(u8) * XEVE_MAX_NUM_TILES_COL * XEVE_MAX_NUM_TILES_ROW);

        if (!ctx->ts_info.arbitrary_slice_flag)
        {
            int first_tile_in_slice, last_tile_in_slice, first_tile_col_idx, last_tile_col_idx, delta_tile_idx;
            int w_tile, w_tile_slice, h_tile_slice;

            w_tile = ctx->ts_info.tile_columns;
            first_tile_in_slice = ctx->ts_info.tile_array_in_slice[ctx->slice_num * 2];
            last_tile_in_slice = ctx->ts_info.tile_array_in_slice[ctx->slice_num * 2 + 1];

            first_tile_col_idx = first_tile_in_slice % w_tile;
            last_tile_col_idx = last_tile_in_slice % w_tile;
            delta_tile_idx = last_tile_in_slice - first_tile_in_slice;

            if (last_tile_in_slice < first_tile_in_slice)
            {
                if (first_tile_col_idx > last_tile_col_idx)
                {
                    delta_tile_idx += ctx->tile_cnt + w_tile;
                }
                else
                {
                    delta_tile_idx += ctx->tile_cnt;
                }
            }
            else if (first_tile_col_idx > last_tile_col_idx)
            {
                delta_tile_idx += w_tile;
            }

            w_tile_slice = (delta_tile_idx % w_tile) + 1; //Number of tiles in slice width
            h_tile_slice = (delta_tile_idx / w_tile) + 1;
            sh->num_tiles_in_slice = w_tile_slice * h_tile_slice;
            for (u32 k = 0; k < sh->num_tiles_in_slice; k++)
            {
                sh->tile_order[k] = ctx->tile_order[tile_cnt++];
            }
        }
        else
        {
            sh->num_tiles_in_slice = ctx->ts_info.num_remaining_tiles_in_slice_minus1[ctx->slice_num] + 2;
            int bef_tile_num = 0;
            for (int i = 0; i < ctx->slice_num; ++i)
            {
                bef_tile_num += ctx->ts_info.num_remaining_tiles_in_slice_minus1[i] + 2;
            }
            for (u32 k = 0; k < sh->num_tiles_in_slice; k++)
            {
                sh->tile_order[k] = ctx->ts_info.tile_array_in_slice[bef_tile_num + k];
            }
        }
    }
    else
    {
        if (ctx->ts_info.arbitrary_slice_flag)
        {
            sh->num_tiles_in_slice = ctx->ts_info.num_remaining_tiles_in_slice_minus1[ctx->slice_num] + 2;
            int bef_tile_num = 0;
            for (int i = 0; i < ctx->slice_num; ++i)
            {
                bef_tile_num += ctx->ts_info.num_remaining_tiles_in_slice_minus1[i] + 2;
            }
            for (u32 k = 0; k < sh->num_tiles_in_slice; k++)
            {
                sh->tile_order[k] = ctx->ts_info.tile_array_in_slice[bef_tile_num + k];
            }
        }
        else
        {
            sh->num_tiles_in_slice = 0;
            for (u32 k = 0; k < ctx->tile_cnt; k++)
            {
                sh->tile_order[sh->num_tiles_in_slice] = k;
                sh->num_tiles_in_slice++;
            }
        }
    }
}

int xeve_param_init(XEVE_PARAM* param)
{
    xeve_mset(param, 0, sizeof(XEVE_PARAM));

    param->profile                    = XEVE_PROFILE_BASELINE;
    param->qp                         = 32;
    param->crf                        = 32;
    param->bframes                    = 15;
    param->codec_bit_depth            = 10;
    param->lookahead                  = 17;
    param->use_deblock                = 1;
    param->threads                    = 1;
    param->rdo_dbk_switch             = 1;
    param->tile_rows                  = 1;
    param->tile_columns               = 1;
    param->num_slice_in_pic           = 1;

    return XEVE_OK;
}

int xeve_param_apply_ppt_baseline(XEVE_PARAM* param, int profile, int preset, int tune)
{
    if (profile != XEVE_PROFILE_BASELINE)
    {
        return XEVE_ERR;
    }

    param->profile = XEVE_PROFILE_BASELINE;

    if (preset == XEVE_PRESET_FAST)
    {
        param->max_cu_intra      = 32;
        param->min_cu_intra      = 4;
        param->max_cu_inter      = 64;
        param->min_cu_inter      = 8;
        param->me_ref_num        = 1;
        param->me_algo           = 1;
        param->me_range          = 32;
        param->me_sub            = 2;
        param->me_sub_pos        = 2;
        param->me_sub_range      = 1;
        param->skip_th           = 0;
        param->merge_num         = 2;
        param->rdoq              = 1;
        param->cabac_refine      = 1;
        param->rdo_dbk_switch    = 0;
    }
    else if (preset == XEVE_PRESET_MEDIUM)
    {
        param->max_cu_intra      = 32;
        param->min_cu_intra      = 4;
        param->max_cu_inter      = 64;
        param->min_cu_inter      = 8;
        param->me_ref_num        = 1;
        param->me_algo           = 1;
        param->me_range          = 64;
        param->me_sub            = 2;
        param->me_sub_pos        = 4;
        param->me_sub_range      = 1;
        param->skip_th           = 0;
        param->merge_num         = 3;
        param->rdoq              = 1;
        param->cabac_refine      = 1;
        param->rdo_dbk_switch    = 0;
    }
    else if (preset == XEVE_PRESET_SLOW)
    {
        param->max_cu_intra      = 32;
        param->min_cu_intra      = 4;
        param->max_cu_inter      = 64;
        param->min_cu_inter      = 8;
        param->me_ref_num        = 1;
        param->me_algo           = 1;
        param->me_range          = 128;
        param->me_sub            = 3;
        param->me_sub_pos        = 4;
        param->me_sub_range      = 2;
        param->skip_th           = 0;
        param->merge_num         = 3;
        param->rdoq              = 1;
        param->cabac_refine      = 1;
        param->rdo_dbk_switch    = 1;
    }
    else if (preset == XEVE_PRESET_PLACEBO)
    {
        param->max_cu_intra      = 64;
        param->min_cu_intra      = 4;
        param->max_cu_inter      = 64;
        param->min_cu_inter      = 4;
        param->me_ref_num        = 2;
        param->me_algo           = 2;
        param->me_range          = 384;
        param->me_sub            = 3;
        param->me_sub_pos        = 8;
        param->me_sub_range      = 3;
        param->skip_th           = 0;
        param->merge_num         = 4;
        param->rdoq              = 1;
        param->cabac_refine      = 1;
        param->rdo_dbk_switch    = 1;
    }
    else
    {
        return XEVE_ERR;
    }

    if (tune != XEVE_TUNE_NONE)
    {
        if (tune == XEVE_TUNE_ZEROLATENCY)
        {
            param->aq_mode = 1;
            param->lookahead = 0;
            param->cutree = 0;
            param->bframes = 0;
            param->me_ref_num = 1;
            param->ref_pic_gap_length = 1;
            param->use_fcst = 1;
            param->inter_slice_type = 1;
        }
        else if (tune == XEVE_TUNE_PSNR)
        {
            param->aq_mode = 0;
        }
        else
        {
            return XEVE_ERR;
        }
    }

    return XEVE_OK;
}

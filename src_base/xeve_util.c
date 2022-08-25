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
    }
    for (i = 0; i < XEVE_TS_NUM; i++)
    {
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

        a_size = (align != NULL) ? align[i] : 1; /* 0; */ // Keeping a_size as 0 will lead to division  by 0 in XEVE_ALIGN_VAL
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

#ifdef UNUSED_CURRENTLY

int xeve_get_avail_cu(int neb_scua[MAX_NEB2], u32* map_cu, u8* map_tidx)
{
    int slice_num_x;
    u16 avail_cu = 0;

    xeve_assert(neb_scua[NEB_X] >= 0);

    slice_num_x = MCU_GET_SN(map_cu[neb_scua[NEB_X]]);

    /* left */
    if (neb_scua[NEB_A] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_A]])) &&
        (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_A]]))
    {
        avail_cu |= AVAIL_LE;
    }
    /* up */
    if (neb_scua[NEB_B] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_B]])) &&
        (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_B]]))
    {
        avail_cu |= AVAIL_UP;
    }
    /* up-right */
    if (neb_scua[NEB_C] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_C]])) &&
        (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_C]]))
    {
        if (MCU_GET_COD(map_cu[neb_scua[NEB_C]]))
        {
            avail_cu |= AVAIL_UP_RI;
        }
    }
    /* up-left */
    if (neb_scua[NEB_D] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_D]])) &&
        (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_D]]))
    {
        avail_cu |= AVAIL_UP_LE;
    }
    /* low-left */
    if (neb_scua[NEB_E] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_E]])) &&
        (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_E]]))
    {
        if (MCU_GET_COD(map_cu[neb_scua[NEB_E]]))
        {
            avail_cu |= AVAIL_LO_LE;
        }
    }
    /* right */
    if (neb_scua[NEB_H] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_H]])) &&
        (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_H]]))
    {
        avail_cu |= AVAIL_RI;
    }
    /* low-right */
    if (neb_scua[NEB_I] >= 0 && (slice_num_x == MCU_GET_SN(map_cu[neb_scua[NEB_I]])) &&
        (map_tidx[neb_scua[NEB_X]] == map_tidx[neb_scua[NEB_I]]))
    {
        if (MCU_GET_COD(map_cu[neb_scua[NEB_I]]))
        {
            avail_cu |= AVAIL_LO_RI;
        }
    }

    return avail_cu;
}

s8 xeve_get_first_refi(int scup, int lidx, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], int cuw, int cuh, int w_scu, int h_scu, u32* map_scu, u8 mvr_idx, u16 avail_lr
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


/******************************************************************************
 * generate sub-picture
 ******************************************************************************/
void arace_gen_subpic(void* src_y, void* dst_y, int w, int h, int s_s,
    int d_s, int bit_depth)
{
    /* source bottom and top top */
    u8* src_b, * src_t, * dst;
    int     x, k, y;

    /* top source */
    src_t = (u8*)src_y;
    /* bottom source */
    src_b = src_t + s_s;
    dst = (u8*)dst_y;

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            k = x << 1;
            dst[x] = (src_t[k] + src_b[k] + src_t[k + 1] + src_b[k + 1] + 2) >> 2;
        }

        src_t += (s_s << 1);
        src_b += (s_s << 1);
        dst += d_s;
    }
}
#endif
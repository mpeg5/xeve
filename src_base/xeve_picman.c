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

#include "xeve_def.h"
static int picman_get_num_allocated_pics(XEVE_PM * pm)
{
    int i, cnt = 0;
    for(i = 0; i < MAX_PB_SIZE; i++) /* this is coding order */
    {
        if(pm->pic[i]) cnt++;
    }
    return cnt;
}

int xeve_picman_move_pic(XEVE_PM *pm, int from, int to)
{
    int i;
    XEVE_PIC * pic;

    pic = pm->pic[from];

    for(i = from; i < to; i++)
    {
        pm->pic[i] = pm->pic[i + 1];
    }
    pm->pic[to] = pic;

    return 0;
}

static void pic_marking(XEVE_PM * pm, int ref_pic_gap_length)
{
    int i;
    XEVE_PIC * pic;

    // mark all pics with layer id > 0 as unused for reference
    for(i = 0; i < MAX_PB_SIZE; i++) /* this is coding order */
    {
        if(pm->pic[i] && IS_REF(pm->pic[i]) &&
            (pm->pic[i]->temporal_id > 0 || (i > 0 && ref_pic_gap_length > 0 && pm->pic[i]->poc % ref_pic_gap_length != 0)))
        {
            pic = pm->pic[i];

            /* unmark for reference */
            SET_REF_UNMARK(pic);
            xeve_picman_move_pic(pm, i, MAX_PB_SIZE - 1);

            if(pm->cur_num_ref_pics > 0)
            {
                pm->cur_num_ref_pics--;
            }
            i--;
        }
    }
    while(pm->cur_num_ref_pics >= XEVE_MAX_NUM_ACTIVE_REF_FRAME) // TODO: change to signalled num ref pics
    {
        for(i = 0; i < MAX_PB_SIZE; i++) /* this is coding order */
        {
            if(pm->pic[i] && IS_REF(pm->pic[i]) )
            {
                pic = pm->pic[i];

                /* unmark for reference */
                SET_REF_UNMARK(pic);
                xeve_picman_move_pic(pm, i, MAX_PB_SIZE - 1);

                pm->cur_num_ref_pics--;

                break;
            }
        }
    }
}

static void picman_flush_pb(XEVE_PM * pm)
{
    int i;
    int max_poc = 0;
    int min_poc = INT_MAX;

    /* mark all frames unused */
    for (i = 0; i < MAX_PB_SIZE; i++)
    {
        if (pm->pic[i] && IS_REF(pm->pic[i]))
        {
            SET_REF_UNMARK(pm->pic[i]);
            xeve_picman_move_pic(pm, i, MAX_PB_SIZE - 1);
            i--;
        }
    }

    for (i = 0; i < MAX_PB_SIZE; i++)
    {
        if (pm->pic[i] && pm->pic[i]->need_for_out && pm->pic[i]->poc != 0 && pm->pic[i]->poc > max_poc)
        {
            max_poc = pm->pic[i]->poc;
        }
    }

    max_poc = max_poc == 0 ? max_poc : max_poc + 1;

    /* reorder poc in DPB */
    int reordered_min_poc = INT_MAX;
    for (i = 0; i < MAX_PB_SIZE; i++)
    {
        if (pm->pic[i] && pm->pic[i]->need_for_out && pm->pic[i]->poc != 0)
        {
            SET_REF_UNMARK(pm->pic[i]);
            pm->pic[i]->poc -= max_poc;
            if (pm->pic[i]->poc < reordered_min_poc)
            {
                reordered_min_poc = pm->pic[i]->poc;
            }
        }
    }
    pm->poc_next_output = max_poc == 0 ? 0 : reordered_min_poc;
    pm->cur_num_ref_pics = 0;
}

void xeve_picman_update_pic_ref(XEVE_PM * pm)
{
    XEVE_PIC ** pic;
    XEVE_PIC ** pic_ref;
    XEVE_PIC  * pic_t;
    int i, j, cnt;

    pic = pm->pic;
    pic_ref = pm->pic_ref;

    for(i = 0, j = 0; i < MAX_PB_SIZE; i++)
    {
        if(pic[i] && IS_REF(pic[i]))
        {
            pic_ref[j++] = pic[i];
        }
    }
    cnt = j;
    while(j < XEVE_MAX_NUM_REF_PICS) pic_ref[j++] = NULL;

    /* descending order sort based on POC */
    for(i = 0; i < cnt - 1; i++)
    {
        for(j = i + 1; j < cnt; j++)
        {
            if(pic_ref[i]->poc < pic_ref[j]->poc)
            {
                pic_t = pic_ref[i];
                pic_ref[i] = pic_ref[j];
                pic_ref[j] = pic_t;
            }
        }
    }
}

static XEVE_PIC * picman_remove_pic_from_pb(XEVE_PM * pm, int pos)
{
    int         i;
    XEVE_PIC  * pic_rem;

    pic_rem = pm->pic[pos];
    pm->pic[pos] = NULL;

    /* fill empty pic buffer */
    for(i = pos; i < MAX_PB_SIZE - 1; i++)
    {
        pm->pic[i] = pm->pic[i + 1];
    }
    pm->pic[MAX_PB_SIZE - 1] = NULL;

    pm->cur_pb_size--;

    return pic_rem;
}

static void picman_set_pic_to_pb(XEVE_PM * pm, XEVE_PIC * pic,
                                 XEVE_REFP(*refp)[REFP_NUM], int pos)
{
    int i;

    for(i = 0; i < pm->num_refp[REFP_0]; i++)
        pic->list_poc[i] = refp[i][REFP_0].poc;

    if(pos >= 0)
    {
        xeve_assert(pm->pic[pos] == NULL);
        pm->pic[pos] = pic;
    }
    else /* pos < 0 */
    {
        /* search empty pic buffer position */
        for(i = (MAX_PB_SIZE - 1); i >= 0; i--)
        {
            if(pm->pic[i] == NULL)
            {
                pm->pic[i] = pic;
                break;
            }
        }
        if(i < 0)
        {
            xeve_assert(i >= 0);
        }
    }
    pm->cur_pb_size++;
}

static int picman_get_empty_pic_from_list(XEVE_PM * pm)
{
    XEVE_IMGB * imgb;
    XEVE_PIC  * pic;
    int i;

    for(i = 0; i < MAX_PB_SIZE; i++)
    {
        pic = pm->pic[i];

        if(pic != NULL && !IS_REF(pic) && pic->need_for_out == 0)
        {
            imgb = pic->imgb;
            xeve_assert(imgb != NULL);

            /* check reference count */
            if (1 == imgb->getref(imgb))
            {
                return i; /* this is empty buffer */
            }
        }
    }
    return -1;
}

void xeve_set_refp(XEVE_REFP * refp, XEVE_PIC  * pic_ref)
{
    refp->pic      = pic_ref;
    refp->poc      = pic_ref->poc;
    refp->map_mv   = pic_ref->map_mv;
    refp->map_unrefined_mv = pic_ref->map_mv;
    refp->map_refi = pic_ref->map_refi;
    refp->list_poc = pic_ref->list_poc;
}

void xeve_copy_refp(XEVE_REFP * refp_dst, XEVE_REFP * refp_src)
{
    refp_dst->pic      = refp_src->pic;
    refp_dst->poc      = refp_src->poc;
    refp_dst->map_mv   = refp_src->map_mv;
    refp_dst->map_unrefined_mv = refp_src->map_mv;
    refp_dst->map_refi = refp_src->map_refi;
    refp_dst->list_poc = refp_src->list_poc;
}

int xeve_check_copy_refp(XEVE_REFP(*refp)[REFP_NUM], int cnt, int lidx, XEVE_REFP  * refp_src)
{
    int i;

    for(i = 0; i < cnt; i++)
    {
        if(refp[i][lidx].poc == refp_src->poc)
        {
            return -1;
        }
    }
    xeve_copy_refp(&refp[cnt][lidx], refp_src);

    return XEVE_OK;
}

int xeve_picman_refp_init(XEVE_PM *pm, int max_num_ref_pics, int slice_type, u32 poc, u8 layer_id, int last_intra, XEVE_REFP(*refp)[REFP_NUM])
{
    int i, cnt;
    if(slice_type == SLICE_I)
    {
        return XEVE_OK;
    }

    xeve_picman_update_pic_ref(pm);
    xeve_assert_rv(pm->cur_num_ref_pics > 0, XEVE_ERR_UNEXPECTED);

    for(i = 0; i < XEVE_MAX_NUM_REF_PICS; i++)
    {
        refp[i][REFP_0].pic = refp[i][REFP_1].pic = NULL;
    }
    pm->num_refp[REFP_0] = pm->num_refp[REFP_1] = 0;

    /* forward */
    if(slice_type == SLICE_P)
    {
        if(layer_id > 0)
        {
            for(i = 0, cnt = 0; i < pm->cur_num_ref_pics && cnt < max_num_ref_pics; i++)
            {
                /* if(poc >= last_intra && pm->pic_ref[i]->poc < last_intra) continue; */
                if(layer_id == 1)
                {
                    if(pm->pic_ref[i]->poc < poc && pm->pic_ref[i]->temporal_id <= layer_id)
                    {
                        xeve_set_refp(&refp[cnt][REFP_0], pm->pic_ref[i]);
                        cnt++;
                    }
                }
                else if(pm->pic_ref[i]->poc < poc && cnt == 0)
                {
                    xeve_set_refp(&refp[cnt][REFP_0], pm->pic_ref[i]);
                    cnt++;
                }
                else if(cnt != 0 && pm->pic_ref[i]->poc < poc && \
                          pm->pic_ref[i]->temporal_id <= 1)
                {
                    xeve_set_refp(&refp[cnt][REFP_0], pm->pic_ref[i]);
                    cnt++;
                }
            }
        }
        else /* layer_id == 0, non-scalable  */
        {
            for(i = 0, cnt = 0; i < pm->cur_num_ref_pics && cnt < max_num_ref_pics; i++)
            {
                if(poc >= (u32)last_intra && pm->pic_ref[i]->poc < (u32)last_intra) continue;
                if(pm->pic_ref[i]->poc < poc)
                {
                    xeve_set_refp(&refp[cnt][REFP_0], pm->pic_ref[i]);
                    cnt++;
                }
            }
        }
    }
    else /* SLICE_B */
    {
        int next_layer_id = XEVE_MAX(layer_id - 1, 0);
        for(i = 0, cnt = 0; i < pm->cur_num_ref_pics && cnt < max_num_ref_pics; i++)
        {
            if(poc >= (u32)last_intra && pm->pic_ref[i]->poc < (u32)last_intra) continue;
            if(pm->pic_ref[i]->poc < poc && pm->pic_ref[i]->temporal_id <= next_layer_id)
            {
                xeve_set_refp(&refp[cnt][REFP_0], pm->pic_ref[i]);
                cnt++;
                next_layer_id = XEVE_MAX(pm->pic_ref[i]->temporal_id - 1, 0);
            }
        }
    }

    if(cnt < max_num_ref_pics && slice_type == SLICE_B)
    {
        int next_layer_id = XEVE_MAX(layer_id - 1, 0);
        for(i = pm->cur_num_ref_pics - 1; i >= 0 && cnt < max_num_ref_pics; i--)
        {
            if(poc >= (u32)last_intra && pm->pic_ref[i]->poc < (u32)last_intra) continue;
            if(pm->pic_ref[i]->poc > poc && pm->pic_ref[i]->temporal_id <= next_layer_id)
            {
                xeve_set_refp(&refp[cnt][REFP_0], pm->pic_ref[i]);
                cnt++;
                next_layer_id = XEVE_MAX(pm->pic_ref[i]->temporal_id - 1, 0);
            }
        }
    }

    xeve_assert_rv(cnt > 0, XEVE_ERR_UNEXPECTED);
    pm->num_refp[REFP_0] = cnt;

    /* backward */
    if(slice_type == SLICE_B)
    {
        int next_layer_id = XEVE_MAX(layer_id - 1, 0);
        for(i = pm->cur_num_ref_pics - 1, cnt = 0; i >= 0 && cnt < max_num_ref_pics; i--)
        {
            if(poc >= (u32)last_intra && pm->pic_ref[i]->poc < (u32)last_intra) continue;
            if(pm->pic_ref[i]->poc > poc && pm->pic_ref[i]->temporal_id <= next_layer_id)
            {
                xeve_set_refp(&refp[cnt][REFP_1], pm->pic_ref[i]);
                cnt++;
                next_layer_id = XEVE_MAX(pm->pic_ref[i]->temporal_id - 1, 0);
            }
        }

        if(cnt < max_num_ref_pics)
        {
            next_layer_id = XEVE_MAX(layer_id - 1, 0);
            for(i = 0; i < pm->cur_num_ref_pics && cnt < max_num_ref_pics; i++)
            {

                if(poc >= (u32)last_intra && pm->pic_ref[i]->poc < (u32)last_intra) continue;
                if(pm->pic_ref[i]->poc < poc && pm->pic_ref[i]->temporal_id <= next_layer_id)
                {
                    xeve_set_refp(&refp[cnt][REFP_1], pm->pic_ref[i]);
                    cnt++;
                    next_layer_id = XEVE_MAX(pm->pic_ref[i]->temporal_id - 1, 0);
                }
            }
        }

        xeve_assert_rv(cnt > 0, XEVE_ERR_UNEXPECTED);
        pm->num_refp[REFP_1] = cnt;
    }

    if(slice_type == SLICE_B)
    {
        pm->num_refp[REFP_0] = XEVE_MIN(pm->num_refp[REFP_0], max_num_ref_pics);
        pm->num_refp[REFP_1] = XEVE_MIN(pm->num_refp[REFP_1], max_num_ref_pics);
    }

    return XEVE_OK;
}

XEVE_PIC * xeve_picman_get_empty_pic(XEVE_PM * pm, int * err)
{
    int ret;
    XEVE_PIC * pic = NULL;

    /* try to find empty picture buffer in list */
    ret = picman_get_empty_pic_from_list(pm);
    if(ret >= 0)
    {
        pic = picman_remove_pic_from_pb(pm, ret);
        goto END;
    }
    /* else if available, allocate picture buffer */
    pm->cur_pb_size = picman_get_num_allocated_pics(pm);

    if(pm->cur_pb_size < pm->max_pb_size)
    {
        /* create picture buffer */
        pic = pm->pa.fn_alloc(&pm->pa, &ret);
        xeve_assert_gv(pic != NULL, ret, XEVE_ERR_OUT_OF_MEMORY, ERR);

        goto END;
    }
    xeve_assert_gv(0, ret, XEVE_ERR_UNKNOWN, ERR);

END:
    pm->pic_lease = pic;
    if(err) *err = XEVE_OK;
    return pic;

ERR:
    if(err) *err = ret;
    return NULL;
}

int xeve_picman_put_pic(XEVE_PM * pm, XEVE_PIC * pic, int is_idr,
                        u32 poc, u8 temporal_id, int need_for_output,
                        XEVE_REFP(*refp)[REFP_NUM], int ref_pic, int tool_rpl, int ref_pic_gap_length)
{
    /* manage RPB */
    if(is_idr)
    {
        picman_flush_pb(pm);
    }
    //Perform picture marking if RPL approach is not used
    else if(tool_rpl == 0)
    {
        if (temporal_id == 0)
        {
            pic_marking(pm, ref_pic_gap_length);
        }
    }

    SET_REF_MARK(pic);

    if(!ref_pic)
    {
        SET_REF_UNMARK(pic);
    }

    pic->temporal_id = temporal_id;
    pic->poc = poc;
    pic->need_for_out = need_for_output;

    /* put picture into listed RPB */
    if(IS_REF(pic))
    {
        picman_set_pic_to_pb(pm, pic, refp, pm->cur_num_ref_pics);
        pm->cur_num_ref_pics++;
    }
    else
    {
        picman_set_pic_to_pb(pm, pic, refp, -1);
    }

    if(pm->pic_lease == pic)
    {
        pm->pic_lease = NULL;
    }

    /*PRINT_DPB(pm);*/

    return XEVE_OK;
}

XEVE_PIC * xeve_picman_out_pic(XEVE_PM * pm, int * err)
{
    XEVE_PIC ** ps;
    int i, ret, any_need_for_out = 0;

    ps = pm->pic;

    for(i = 0; i < MAX_PB_SIZE; i++)
    {
        if(ps[i] != NULL && ps[i]->need_for_out)
        {
            any_need_for_out = 1;

            if((ps[i]->poc <= pm->poc_next_output))
            {
                ps[i]->need_for_out = 0;
                pm->poc_next_output = ps[i]->poc + pm->poc_increase;

                if(err) *err = XEVE_OK;
                return ps[i];
            }
        }
    }
    if(any_need_for_out == 0)
    {
        ret = XEVE_ERR_UNEXPECTED;
    }
    else
    {
        ret = XEVE_OK_FRM_DELAYED;
    }

    if(err) *err = ret;
    return NULL;
}

int xeve_picman_deinit(XEVE_PM * pm)
{
    int i;

    /* remove allocated picture and picture store buffer */
    for(i = 0; i < MAX_PB_SIZE; i++)
    {
        if(pm->pic[i])
        {
            pm->pa.fn_free(&pm->pa, pm->pic[i]);
            pm->pic[i] = NULL;
        }
    }
    if(pm->pic_lease)
    {
        pm->pa.fn_free(&pm->pa, pm->pic_lease);
        pm->pic_lease = NULL;
    }
    return XEVE_OK;
}

int xeve_picman_init(XEVE_PM * pm, int max_pb_size, int max_num_ref_pics,
                          PICBUF_ALLOCATOR * pa)
{
    if(max_num_ref_pics > XEVE_MAX_NUM_REF_PICS || max_pb_size > MAX_PB_SIZE)
    {
        return XEVE_ERR_UNSUPPORTED;
    }
    pm->max_num_ref_pics = max_num_ref_pics;
    pm->max_pb_size = max_pb_size;
    pm->poc_increase = 1;
    pm->pic_lease = NULL;

    xeve_mcpy(&pm->pa, pa, sizeof(PICBUF_ALLOCATOR));

    return XEVE_OK;
}

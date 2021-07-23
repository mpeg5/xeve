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
#include "xeve_rc.h"
#include "xeve_fcst.h"
#include <math.h>

static const s8 tbl_small_dia_search[4][3] =
{
    { 0, -1, 3 },{ 1, 0, 0 },{ 0, 1, 1 },{ -1, 0, 2 }
};


/* weighting factor for current pic to reference pic */
static const double tbl_rpic_dist_wt[8] =
{
    1.0,  1.3,  1.4,  1.4,  1.6,  1.6,  1.6,  1.6
};


/* slice depth and reference pictures in forecast gop 16 */
/*(slice dpeth, L0, L1*/
static const int fcst_rpl_gop[4][16][3] =
{
{
{ 4, 1, -1 }, { 3, 2, -2 },
{ 4, 1, -1 }, { 2, 4, -4 },
{ 4, 1, -1 }, { 3, 2, -2 },
{ 4, 1, -1 }, { 1, 8, -8 },
{ 4, 1, -1 }, { 3, 2, -2 },
{ 4, 1, -1 }, { 2, 4, -4 },
{ 4, 1, -1 }, { 3, 2, -2 },
{ 4, 1, -1 }, {0, 16, 16}
},
{
{ 3, 1, -1 },  { 2, 2, -2 },
{ 3, 1, -1 },  { 1, 4, -4 },
{ 3, 1, -1 },  { 2, 2, -2 },
{ 3, 1, -1 },  { 0, 8,  8},
{ 0, 0, 0 },   { 0, 0, 0 },
{ 0, 0, 0 },   { 0, 0, 0 },
{ 0, 0, 0 },   { 0, 0, 0 },
{ 0, 0, 0 },   { 0, 0, 0 }
},
{
{ 2, 1, -1 },  { 1, 2, -2 },
{ 2, 1, -1 },  { 0, 4, 4 },
{ 0, 0, 0 },   { 0, 0, 0 },
{ 0, 0, 0 },   { 0, 0, 0 },
{ 0, 0, 0 },   { 0, 0, 0 },
{ 0, 0, 0 },   { 0, 0, 0 },
{ 0, 0, 0 },   { 0, 0, 0 },
{ 0, 0, 0 },   { 0, 0, 0 }
},
{
{ 1, 1, -1 },  { 0, 2, 2 },
{ 0, 0, 0 },   { 0, 0, 0 },
{ 0, 0, 0 },   { 0, 0, 0 },
{ 0, 0, 0 },   { 0, 0, 0 },
{ 0, 0, 0 },   { 0, 0, 0 },
{ 0, 0, 0 },   { 0, 0, 0 },
{ 0, 0, 0 },   { 0, 0, 0 },
{ 0, 0, 0 },   { 0, 0, 0 }
},
};

/* weighting factor for transfer cost */
static const u16 tbl_inv_qscale[41]=
{
    51, 48, 45, 43, 40, 38, 36, 34, 32, 30, 28, 27, 26, 24, 23, 21, 20, 19, 18, 17, 16,
    15, 14, 13, 13, 12, 11, 11, 10, 10, 9, 8, 8, 8, 7, 7, 6, 6, 6, 5, 5
};


s32 xeve_fcst_get_scene_type(XEVE_CTX * ctx, XEVE_PICO * pico)
{
    s32          fc_intra, fc_inter, cpx_thd, scn_thd;
    s32          i, ridx, dist_to_p, icnt_mode, stype, scene_type;

    /* init */
    stype = pico->sinfo.slice_type;
    fc_intra = pico->sinfo.uni_est_cost[INTRA];
    fc_inter = 0;
    cpx_thd = (s32)(ctx->f / ctx->rc->param->cpx_thd_resolution);
    scn_thd = (s32)(cpx_thd * ctx->rc->param->thd_sc);
    scene_type = SCENE_NORMAL;
    icnt_mode = INTER_UNI0 - 1;
    dist_to_p = 1;

    /* intra frame */
    if (stype == SLICE_I)
    {
        if (fc_intra < cpx_thd)
        {
            scene_type = SCENE_EX_LOW;
        }
        else if (fc_intra * 0.6 <= scn_thd)
        {
            scene_type = SCENE_LOW;
        }
        return scene_type;
    }

    /* get inter cost and scene threshould, dist_to_p */
    if (stype == SLICE_B)
    {
        fc_inter = pico->sinfo.bi_fcost;
        /* CHECK ME LATER!!: is it right? * (5/6) for B scene thd?? */
        scn_thd = (s32)((cpx_thd * ctx->rc->param->thd_sc * 4) / 6);
    }
    else /* SLICE_P */
    {
        fc_inter = pico->sinfo.uni_est_cost[INTER_UNI0];
        dist_to_p = ctx->param.bframes + 1;

        if (dist_to_p > 1)
        {
            if (dist_to_p > 2)
            {
                fc_inter = pico->sinfo.uni_est_cost[INTER_UNI2];
                icnt_mode = INTER_UNI2 - 1;
            }
            else
            {
                fc_inter = pico->sinfo.uni_est_cost[INTER_UNI1];
                icnt_mode = INTER_UNI1 - 1;
            }
        }

        scn_thd = (s32)(cpx_thd * (tbl_rpic_dist_wt[dist_to_p - 1] * ctx->rc->param->thd_sc));
    }

    /* get inter scene type */
    if (fc_inter * 5 < cpx_thd && fc_intra < cpx_thd * 3)
    {
        scene_type = SCENE_EX_LOW;
    }
    else if (fc_inter <= scn_thd && (fc_intra >> 1) <= scn_thd)
    {
        scene_type = SCENE_LOW;
    }
    else if (fc_inter >= (scn_thd << 2))
    {
        scene_type = SCENE_HIGH;
    }
    else if (fc_inter >= (scn_thd << 1) && pico->sinfo.icnt[icnt_mode] >= (s32)(ctx->f_lcu * 0.80))
    {
        scene_type = SCENE_HIGH;
    }

    /* if there is any scene_change in a gop, P frame is handled as scene_change */
    if (dist_to_p == ctx->param.bframes + 1)
    {
        for (i = 1; i < ctx->param.bframes + 1; i++)
        {
            ridx = XEVE_MOD_IDX(pico->pic_icnt - i, ctx->pico_max_cnt);

            if (ctx->pico_buf[ridx]->sinfo.scene_type == SCENE_HIGH)
            {
                return SCENE_HIGH;
            }
        }
    }

    return scene_type;
}

void xeve_gen_subpic(pel * src_y, pel * dst_y, int w, int h, int s_s,
    int d_s, int bit_depth)
{
    /* source bottom and top top */
    pel * src_b, * src_t;
    pel * dst;

    int      x, k, y, shift;

    /* top source */
    src_t = src_y;
    /* bottom source */
    src_b = src_t + s_s;
    dst   = dst_y;
    shift = 2; // changed to 2 from 1 to correct the averaging.

    for(y = 0; y < h; y++)
    {
        for(x = 0; x < w; x++)
        {
            k =  x << 1;
            dst[x] = (pel)((src_t[k] + src_b[k] + src_t[k+1] + src_b[k+1] + (1<<(shift-1))) >> shift);
        }
        src_t += (s_s << 1);
        src_b += (s_s << 1);
        dst += d_s;
    }
}

static u32 get_aq_blk_sum(void * pic_t, int width, int height,
        int stride)
{
    int      i, j;
    u16 * pic;
    u32   sum = 0;

    pic = (u16 *)pic_t;

    for(i=0; i < height; i++)
    {
        for(j=0; j < width; j++)
        {
            sum += pic[j];
        }
        pic += stride;
    }
    return sum;
}

static u32 get_aq_blk_ssum(void * pic_t, int width, int height,
        int stride)
{
    int      i, j;
    u16 * pic;
    u32   ssum = 0;

    pic = (u16 *)pic_t;

    for(i=0; i < height; i++)
    {
        for(j=0; j < width; j++)
        {
            ssum += (u32)pic[j] * pic[j];
        }
        pic  += stride;
    }
    return ssum;
}

u64 get_lcu_var(XEVE_CTX * ctx, void * pic, int log2_w_max, int log2_h_max,
        int x, int y, int stride)
{
    int  i, j, w, h, blk_loop_w, blk_loop_h;
    u64  sum, ssum, var = 0;
    u16 *org_16, * pic_16;
    int log2_w, log2_h;

    log2_w = LOG2_AQ_BLK_SIZE;
    log2_h = LOG2_AQ_BLK_SIZE;
    blk_loop_w = 1 << (log2_w_max- log2_w);
    blk_loop_h = 1 << (log2_h_max- log2_h);
    w = 1 << log2_w;
    h = 1 << log2_h;

    pic_16 = (u16*)pic;
    for (i = 0; i < blk_loop_h; i++)
    {
        for (j = 0; j < blk_loop_w; j++)
        {
            org_16 = pic_16 + x + (j << log2_w) + (y + (i << log2_h)) * stride;
            sum = get_aq_blk_sum(org_16, w, h, stride);
            ssum = get_aq_blk_ssum(org_16, w, h, stride);
            var += (ssum - ((sum * sum) >> (log2_w + log2_h)));
        }
    }
    return (var >> (log2_w_max - log2_w));
}



static void adaptive_quantization(XEVE_CTX * ctx)
{
    int         blk_size, blk_num, x, y, x_blk, y_blk, log2_cuwh;
    XEVE_FCST * fcst = &ctx->fcst;
    s32       * qp_offset;
    double      vald;
    int         s_l, s_c;
    u64         var;
    double      aq_bd_const;
    int         w_blk, h_blk, f_blk;
    int         sum_blk;
    s8          offset_dqp;
    int         w_shift = ctx->param.cs_w_shift;
    int         h_shift = ctx->param.cs_h_shift;

    blk_num      = 0;
    x_blk        = 0;
    y_blk        = 0;
    log2_cuwh    = fcst->log2_fcst_blk_spic +1; /* fcst block (subpic) + 1 for fullpic */
    blk_size     = 1 << log2_cuwh;
    qp_offset = ctx->pico->sinfo.map_qp_blk;

    h_blk = fcst->h_blk;
    w_blk = fcst->w_blk;
    f_blk = fcst->f_blk;

    aq_bd_const  = (ctx->sps.bit_depth_luma_minus8 + 7.2135) * 2;
    s_l = ctx->pico->pic.s_l;
    s_c = ctx->pico->pic.s_c;

    while(1)
    {
        x = x_blk << log2_cuwh;
        y = y_blk << log2_cuwh;

        if(x + blk_size >= ctx->w || y + blk_size >= ctx->h)
        {
            var = 0;
        }
        else
        {
            var  = get_lcu_var(ctx, ctx->pico->pic.buf_y, log2_cuwh,
                log2_cuwh, x, y, s_l);
            if(ctx->sps.chroma_format_idc)
            {
                var += get_lcu_var(ctx, ctx->pico->pic.buf_u, log2_cuwh - w_shift, log2_cuwh - h_shift, (x >> w_shift), (y >> h_shift), s_c);
                var += get_lcu_var(ctx, ctx->pico->pic.buf_v, log2_cuwh - w_shift, log2_cuwh - h_shift, (x >> w_shift), (y >> h_shift), s_c);
            }
        }

        vald = (int)((AQ_STR_CONST * (log2(XEVE_MAX((double)var, 1))
            - aq_bd_const)) * AQ_STRENGTH);
        qp_offset[blk_num] = (int)(XEVE_CLIP3(-5, 5, vald));

        x_blk++;

        if(x_blk == w_blk)
        {
            x_blk = 0;
            y_blk++;
        }
        blk_num++;
        if(blk_num== f_blk) break;
    }


    /* normalize dqp_map */
    sum_blk = 0;
    for (blk_num = 0; blk_num< ctx->fcst.f_blk; blk_num++)
    {
        sum_blk += qp_offset[blk_num];
    }

    offset_dqp = sum_blk / ctx->fcst.f_blk;
    for (blk_num = 0; blk_num< ctx->fcst.f_blk; blk_num++)
    {
        qp_offset[blk_num] -= offset_dqp;
    }
}


static s32 get_transfer_cost(XEVE_PICO * pico_cur, int blk_num)
{
    s32   (* map_uni_lcost)[4], * map_bi_lcost;
    u16   * transfer_in_cost;
    u8    * map_pdir;
    float  intra_cost, transfer_amount, weight;
    int    qp_offset, inv_qscale;

    /* Get transfer cost of LCU at curent blk_num = transfer_in cost from referencing piture
      stored at transfer_cost buffer at curent pic */

    transfer_in_cost = pico_cur->sinfo.transfer_cost;
    map_uni_lcost    = pico_cur->sinfo.map_uni_lcost;
    map_bi_lcost     = pico_cur->sinfo.map_bi_lcost;
    map_pdir         = pico_cur->sinfo.map_pdir_bi;
    qp_offset        = XEVE_CLIP3(-5, 5, pico_cur->sinfo.map_qp_blk[blk_num]);
    inv_qscale       = tbl_inv_qscale[((int)(qp_offset * AQ_STRENGTH) + 10) << 1];
    intra_cost       = (float)((map_uni_lcost[blk_num][INTRA] * inv_qscale) >> 8) ;
    transfer_amount  = transfer_in_cost[blk_num] + intra_cost;

    if(map_pdir[blk_num] != INTRA)
    {
        weight = (float)(map_uni_lcost[blk_num][INTRA] - (map_bi_lcost[blk_num]))
            / map_uni_lcost[blk_num][INTRA];
    }
    else
    {
        weight = 0;
    }

    return (s32)(transfer_amount * weight);
}

static void set_blk_tree_info(XEVE_CTX * ctx, s16  (*mv_t)[MV_D], int list,
                              int * blk_idx, int * area_idx)
{
    int   t0, cuwh;
    int   w_blk = ctx->fcst.w_blk;
    s16   mv[MV_D], mv_det[MV_D];

    t0     = ctx->fcst.log2_fcst_blk_spic;
    cuwh   = 1 << t0;
    mv[MV_X] = mv_t[list][MV_X];
    mv[MV_Y] = mv_t[list][MV_Y];

    /* obtain detailed mv propagating cost */
    mv_det[MV_X] = mv[MV_X] & (s16)(cuwh - 1);
    mv_det[MV_Y] = mv[MV_Y] & (s16)(cuwh - 1);

    /* obtain blk index for propagating cost */
    blk_idx[0] = (mv[MV_X] >> t0) + (mv[MV_Y] >> t0)* w_blk;
    blk_idx[1] = ((mv[MV_X] >> t0) + 1) + (mv[MV_Y] >> t0)* w_blk;
    blk_idx[2] = (mv[MV_X] >> t0) + ((mv[MV_Y] >> t0) + 1)* w_blk;
    blk_idx[3] = ((mv[MV_X] >> t0) + 1) + ((mv[MV_Y] >> t0) + 1)* w_blk;

    /* cablklate ration of blk area */
    area_idx[0] = (cuwh - mv_det[MV_X]) * (cuwh - mv_det[MV_Y]);
    area_idx[1] = (     mv_det[MV_X]) *   (cuwh - mv_det[MV_Y]);
    area_idx[2] = (cuwh - mv_det[MV_X]) * (mv_det[MV_Y]);
    area_idx[3] = mv_det[MV_X]        *    mv_det[MV_Y];

}

/* get availability of blk in blk-tree */
static void set_transfer_cost(XEVE_CTX * ctx, s16 (* mv_blk)[MV_D],
    u16 * map_transfer_cost, s32 transfer_cost, int * blk_idx,
    int * area_idx, int list)
{
    s16 * mv, w_blk, h_blk;
    int    log2_cuwh;

    mv         = mv_blk[list];
    w_blk      = ctx->fcst.w_blk;
    h_blk      = ctx->fcst.h_blk;
    log2_cuwh = ctx->fcst.log2_fcst_blk_spic;

    /* for upper left */
    if(mv[MV_X] < w_blk && mv[MV_Y] < h_blk && mv[MV_X] >= 0 && mv[MV_Y] >= 0)
    {
        map_transfer_cost[blk_idx[0]] = XEVE_CLIP16_ADD(map_transfer_cost[blk_idx[0]],
            (area_idx[0] * transfer_cost + 2048) >> (log2_cuwh * 2));
    }

    /* for upper right */
    if(mv[MV_X]+1 < w_blk && mv[MV_Y] < h_blk && mv[MV_X]+1 >= 0 && mv[MV_Y] >= 0)
    {
        map_transfer_cost[blk_idx[1]] = XEVE_CLIP16_ADD(map_transfer_cost[blk_idx[1]],
            (area_idx[1] * transfer_cost + 2048) >> (log2_cuwh * 2));
    }

    /* for bottom left */
    if(mv[MV_X] < w_blk && mv[MV_Y]+1 < h_blk && mv[MV_X] >= 0 && mv[MV_Y]+1 >= 0)
    {
        map_transfer_cost[blk_idx[2]] = XEVE_CLIP16_ADD(map_transfer_cost[blk_idx[2]],
            (area_idx[2] * transfer_cost + 2048) >> (log2_cuwh * 2));
    }

    /* for bottom right */
    if(mv[MV_X]+1 < w_blk && mv[MV_Y]+1 < h_blk && mv[MV_X]+1 >= 0 && mv[MV_Y]+1 >= 0)
    {
        map_transfer_cost[blk_idx[3]] = XEVE_CLIP16_ADD(map_transfer_cost[blk_idx[3]],
            (area_idx[3] * transfer_cost + 2048) >> (log2_cuwh * 2));
    }
}
static s32 blk_tree_transfer(XEVE_CTX * ctx, XEVE_PICO * pico_l0,
                               XEVE_PICO * pico_l1, XEVE_PICO * pico_cur)
{
    int       x, y, blk_idx[4], area_idx[4], x_blk, y_blk, blk_num, dist, log2_unit_cuwh;
    u8   * map_pdir;
    s16  (* map_mv)[REFP_NUM][MV_D];
    u16  * transfer_cost_l0, * transfer_cost_l1;
    s32     transfer_cost;
    s16     mv[REFP_NUM][MV_D], mv_blk[REFP_NUM][MV_D];

    x_blk     = 0;
    y_blk     = 0;
    blk_num   = 0;

    log2_unit_cuwh = ctx->fcst.log2_fcst_blk_spic;

    map_mv = pico_cur->sinfo.map_mv_bi;

    map_pdir         = pico_cur->sinfo.map_pdir_bi;
    transfer_cost_l0 = pico_l0->sinfo.transfer_cost;
    transfer_cost_l1 = pico_l1->sinfo.transfer_cost;

    while(1)
    {
        x = x_blk << log2_unit_cuwh;
        y = y_blk << log2_unit_cuwh;

        mv[REFP_0][MV_X] = x + (map_mv[blk_num][REFP_0][MV_X] >> 2);
        mv[REFP_0][MV_Y] = y + (map_mv[blk_num][REFP_0][MV_Y] >> 2);

        dist = pico_l1->pic_icnt- pico_cur->pic_icnt;

        mv[REFP_1][MV_X] = x + (map_mv[blk_num][REFP_1][MV_X] >> 2);
        mv[REFP_1][MV_Y] = y + (map_mv[blk_num][REFP_1][MV_Y] >> 2);

        mv_blk[REFP_0][MV_X] = mv[REFP_0][MV_X] >> log2_unit_cuwh;
        mv_blk[REFP_0][MV_Y] = mv[REFP_0][MV_Y] >> log2_unit_cuwh;
        mv_blk[REFP_1][MV_X] = mv[REFP_1][MV_X] >> log2_unit_cuwh;
        mv_blk[REFP_1][MV_Y] = mv[REFP_1][MV_Y] >> log2_unit_cuwh;

        set_blk_tree_info(ctx, mv, REFP_0, blk_idx, area_idx);

        if(map_pdir[blk_num] != INTRA)
        {
            /* Find transfer_cost */
            transfer_cost = get_transfer_cost(pico_cur, blk_num);

            if(transfer_cost > 0)
            {
                if(map_pdir[blk_num] == INTER_L0 || pico_cur->sinfo.slice_type == SLICE_P)
                {
                    set_transfer_cost(ctx, mv_blk, transfer_cost_l0, transfer_cost,
                              blk_idx, area_idx, REFP_0);
                }
                else if(map_pdir[blk_num] == INTER_L1)
                {
                    /* transfer_cost = xxx, store at L1 direction  */
                    set_blk_tree_info(ctx, mv, REFP_1, blk_idx, area_idx);
                    set_transfer_cost(ctx, mv_blk, transfer_cost_l1, transfer_cost,
                              blk_idx, area_idx, REFP_1);
                }
                else
                {
                    /* transfer_cost = xxx, store at both directions */
                    /* split cost 1/2 for each predicted direction (L0, L1) */
                    transfer_cost >>= 1;

                    /* Divide transfer_cost by blk area */
                    set_transfer_cost(ctx, mv_blk, transfer_cost_l0, transfer_cost,
                              blk_idx, area_idx, REFP_0);
                    set_blk_tree_info(ctx, mv, REFP_1, blk_idx, area_idx);
                    set_transfer_cost(ctx, mv_blk, transfer_cost_l1, transfer_cost,
                              blk_idx, area_idx, REFP_1);
                }
            }
        }
        x_blk++;

        if(x_blk == (ctx->fcst.w_blk- 1))  /* SKIP the last blk in x-direction */
        {
            x_blk = 0;
            y_blk++;
            blk_num++;
        }
        blk_num++;

        if(y_blk == (ctx->fcst.h_blk - 1)) break; /* SKIP the last blk in y-direction */
    }

    return 0;
}


static s32 blk_tree_end(XEVE_CTX * ctx, XEVE_PICO * pico)
{
    float ratio = 0;
    int   qp_offset, intra_lcost, inv_qscale, x_blk = 0, y_blk = 0, blk_num = 0;

    if (pico->sinfo.slice_depth >= FRM_DEPTH_3) return 0;


    while(1)
    {
        qp_offset   = XEVE_CLIP3(-5, 5, pico->sinfo.map_qp_blk[blk_num]);
        inv_qscale  = tbl_inv_qscale[((int)(qp_offset * AQ_STRENGTH) + 10) << 1];
        intra_lcost = (pico->sinfo.map_uni_lcost[blk_num][INTRA] * inv_qscale) >> 8;

        if(intra_lcost)
        {
            ratio   = (float)(log2(intra_lcost + pico->sinfo.transfer_cost[blk_num])
                - log2(intra_lcost));
            //pico->sinfo.map_qp_blk[blk_num] -= (int)(LCU_STRENGTH* (FRM_DEPTH_3 - (int)pico->sinfo.slice_depth)   * ratio);
            pico->sinfo.map_qp_blk[blk_num] -= (int)(LCU_STRENGTH*  ratio);
            pico->sinfo.map_qp_blk[blk_num]  = XEVE_CLIP3(-5, 5, pico->sinfo.map_qp_blk[blk_num]);
        }
        x_blk++;

        if(x_blk == (ctx->fcst.w_blk - 1)) /* SKIP the last blk in x-direction */
        {
            x_blk = 0;
            y_blk++;
            blk_num++;
        }
        blk_num++;
        if(y_blk == (ctx->fcst.h_blk - 1)) break; /* SKIP the last blk in y-direction */
    }
    return 0;
}

void fill_blk_scu(XEVE_CTX* ctx, int x_blk, int y_blk, int log2_cuwh, s8 val, s8 * qp_offset)
{
    int x_pos, y_pos, log2_size;
    int size, x, y;

    log2_size= log2_cuwh - MIN_CU_LOG2; //
    size = 1 << log2_size;
    x_pos = x_blk << log2_size;
    y_pos = y_blk << log2_size;


    for (y = y_pos; y < y_pos + size; y++)
    {
        if (y >= ctx->h_scu)break;
        for (x = x_pos; x < x_pos + size; x++)
        {
            if (x >= ctx->w_scu)break;
            qp_offset[y* ctx->w_scu + x] = (u8)val;
        }
    }
}


void fill_blk_scu_frm(XEVE_CTX* ctx,XEVE_PICO * pico, int log2_cuwh)
{

    int     blk_size, blk_num,x_blk, y_blk;
    XEVE_FCST* fcst = &ctx->fcst;
    s32 * qp_offset;
    int w_blk, h_blk, f_blk;
    s8 * qp_scu_map;

    blk_num      = 0;
    x_blk        = 0;
    y_blk        = 0;
    blk_size     = 1 << log2_cuwh;
    qp_offset  = pico->sinfo.map_qp_blk;
    qp_scu_map = pico->sinfo.map_qp_scu;

    h_blk = fcst->h_blk;
    w_blk = fcst->w_blk;
    f_blk = fcst->f_blk;

    h_blk = fcst->h_blk;
    w_blk = fcst->w_blk;
    f_blk = fcst->f_blk;

    while(1)
    {
        fill_blk_scu(ctx, x_blk, y_blk, log2_cuwh+1, qp_offset[blk_num], qp_scu_map);

        x_blk++;

        if(x_blk == w_blk)
        {
            x_blk = 0;
            y_blk++;
        }
        blk_num++;

        if(blk_num== f_blk) break;
    }

}
static void blk_tree_fixed_gop(XEVE_CTX * ctx)
{
    int          i,  bframes, pic_idx;
    XEVE_PICO  * pico, * pico_l0, * pico_l1;
    int         pic_icnt_last, depth,  gop_size, max_depth;
    int         blk_num, sum_blk = 0;
    s8          offset_dqp;
    s32        * qp_offset;

    bframes       = 0;
    pic_icnt_last = ctx->pico->pic_icnt;
    gop_size = ctx->param.bframes + 1;

    max_depth = 0;
    int offset = pic_icnt_last == gop_size ? 1 : 0;
    for (i = 0; i < gop_size + offset; i++)
    {
        pic_idx = XEVE_MOD_IDX(pic_icnt_last - i, ctx->pico_max_cnt);
        pico = ctx->pico_buf[pic_idx];
        if (pico->sinfo.slice_depth > max_depth)
        {
            max_depth = pico->sinfo.slice_depth;
        }
    }

    for (depth = max_depth; depth >= 0; depth--)
    {
        for (i = 0; i < gop_size + offset; i++)
        {
            pic_idx = XEVE_MOD_IDX(pic_icnt_last - i, ctx->pico_max_cnt);
            pico = ctx->pico_buf[pic_idx];
            if (pico->sinfo.slice_depth != depth) continue;

            pico_l0 = ctx->pico_buf[XEVE_MOD_IDX(pic_idx - pico->sinfo.ref_pic[REFP_0], ctx->pico_max_cnt)];
            pico_l1 = ctx->pico_buf[XEVE_MOD_IDX(pic_idx - pico->sinfo.ref_pic[REFP_1], ctx->pico_max_cnt)];

            blk_tree_transfer(ctx, pico_l0, pico_l1, pico);
        }
    }

    /* calcuate all qps */
    for (i = 0; i < gop_size + offset; i++)
    {
        pic_idx = XEVE_MOD_IDX(pic_icnt_last - i, ctx->pico_max_cnt);
        pico = ctx->pico_buf[pic_idx];
        if(pico->sinfo.slice_depth < max_depth) blk_tree_end(ctx, pico);
    }

    /* copy blk qp to scu map */
    for (i = 0; i < gop_size + offset; i++)
    {
        pic_idx = XEVE_MOD_IDX(pic_icnt_last - i, ctx->pico_max_cnt);
        pico = ctx->pico_buf[pic_idx];
        qp_offset = pico->sinfo.map_qp_blk;

        /* normalize */
        for (blk_num = 0; blk_num < ctx->fcst.f_blk; blk_num++)
        {
            sum_blk += qp_offset[blk_num];
        }

        offset_dqp = sum_blk / ctx->fcst.f_blk;

        for (blk_num = 0; blk_num < ctx->fcst.f_blk; blk_num++)
        {
            qp_offset[blk_num] = qp_offset[blk_num] - offset_dqp;
        }
        fill_blk_scu_frm(ctx, pico, ctx->fcst.log2_fcst_blk_spic);
    }
}


/********************* get inter and intra score*****************************/
void xeve_mc_fcst(u16 * ref_t, s32 gmv_x, s32 gmv_y, s32 s_ref, s32 s_pred
              , u16 * pred, s32 w, s32 h, s32 bi, u8 bit_depth, s32 * buf, s16* min_mv, s16* max_mv)
{
    u16 * p8u;
    u16 * p16;
    s32   i, j;
    u16 * ref;

    ref = (u16 *)ref_t;
    gmv_x >>= 2;
    gmv_x = XEVE_CLIP3(min_mv[MV_X], max_mv[MV_X], gmv_x);
    gmv_y >>= 2;
    gmv_y = XEVE_CLIP3(min_mv[MV_Y], max_mv[MV_Y], gmv_y);

    ref += gmv_y * s_ref + gmv_x;

    if (bi)
    {
        p16 = (u16 *)pred;
        for (i = 0; i<h; i++)
        {
            for (j = 0; j<w; j++)
            {
                p16[j] = (ref[j] << 4);
            }
            p16 += s_pred;
            ref += s_ref;
        }
    }
    else
    {
        p8u = (u16 *)pred;
        for (i = 0; i<h; i++)
        {
            for (j = 0; j<w; j++)
            {
                p8u[j] = ref[j];
            }
            p8u += s_pred;
            ref += s_ref;
        }
    }
}

void fcst_ipred_prepare(XEVE_PIC * spic, u16 * buf_le, u16 * buf_up, s32 cuwh, s32 x, s32 y)
{
    s32   j, log2_cuwh, avail_cnt;
    u16 * src_le = NULL;
    s32   stride = spic->s_l;
    pel * src = spic->y + x + y * stride;

    log2_cuwh = XEVE_LOG2(cuwh);
    avail_cnt = 0;

    /* Avail UP_Left */
    if (x > 0 && y > 0)
    {
        avail_cnt++;
        buf_le[0] = buf_up[0] = src[-stride - 1];
    }
    else
    {
        if (x > 0)
        {
            buf_le[0] = buf_up[0] = src[-1];
        }
        else if (y > 0)
        {
            buf_le[0] = buf_up[0] = src[-stride];
        }
        else
        {
            buf_le[0] = buf_up[0] = 512;
        }
    }

    /* Avail Left */
    if (x > 0)
    {
        avail_cnt ++;
        src_le = src - 1;
        for (j = 1; j < (cuwh + 1); j++)
        {
            buf_le[j] = *src_le;
            src_le += stride;
        }

        /* Avail Left-Below */
        if( y + cuwh * 2 < spic->h_l)
        {
            avail_cnt++;
            src_le = src - 1 + (stride << log2_cuwh);
            for (j = (cuwh + 1); j < (cuwh * 2 + 1); j++)
            {
                buf_le[j] = *src_le;
                src_le += stride;
            }
        }
        else
        {
            for (j = (cuwh + 1); j < (cuwh * 2 + 1); j++)
            {
                buf_le[j] = buf_le[cuwh];
            }
        }
    }
    else
    {
        for (j = 1; j < (cuwh * 2 + 1); j++)
        {
            buf_le[j] = buf_le[0];
        }
    }

    /* Avail Up */
    if (y > 0)
    {
        avail_cnt ++;
        xeve_mcpy(buf_up + 1, src - stride, cuwh * sizeof(pel));
        /* Avail Up-Right */
        if (x + cuwh < spic->w_l)
        {
            avail_cnt ++;
            xeve_mcpy(buf_up + cuwh + 1, src - stride + cuwh, cuwh * sizeof(pel));
        }
        else
        {
            for (j = (cuwh + 1); j < (cuwh * 2 + 1); j++)
            {
                buf_up[j] = buf_up[cuwh];
            }
        }
    }
    else
    {
        for (j = 1; j < (cuwh * 2 + 1); j++)
        {
            buf_up[j] = buf_up[0];
        }
    }

    buf_up[-1] = (buf_up[0] + buf_le[0]) >> 1;
}

static void fcst_mc_bi_avg_l(pel pred[][4096], s32 cuw, s32 cuh, s32 cuwh, pel * org_y, s32 y_s, u8 bit_depth)
{
    pel   * p0, *p1, *y, t0;
    s32     i, j;
    s32     shift = 1;

    y = org_y;
    p0 = pred[REFP_0];
    p1 = pred[REFP_1];

    for (i = 0; i < cuh; i++)
    {
        for (j = 0; j < cuw; j++)
        {
            t0 = (p0[j] + p1[j] + (1 << (shift - 1))) >> shift;
            y[j] = (pel)t0;
        }
        p0 += cuw;
        p1 += cuw;
        y += y_s;
    }
}

static s32 xeve_est_intra_cost(XEVE_CTX * ctx, s32 x0, s32 y0)
{
    s32        x, y, i, mode, cuwh, log2_cuwh, s_o;
    s32        cost, cost_best, tot_cost, intra_penalty;
    u8         temp_avil[5] = { 0 };
    pel      * org;
    XEVE_PIC * spic = ctx->pico->spic;
    pel      * pred = ctx->rcore->pred;
    pel        buf_le0[65];
    pel        buf_up0[65 + 1];

    log2_cuwh = ctx->fcst.log2_fcst_blk_spic;  // +  ctx->rc->param->intra_depth;
    cuwh           = 1 << log2_cuwh;
    s_o            = spic->s_l;
    tot_cost       = 0;
    intra_penalty  = (s32)(ctx->rc->lambda[3] * 4);

    for (i = 0; i < MAX_SUB_CNT; i++)
    {
        x = x0 + cuwh * (i % 2);
        y = y0 + cuwh * (i / 2);
        org = spic->y + x + y * s_o;

        if (x + cuwh > spic->w_l || y + cuwh > spic->h_l)
        {
            cost_best = 0;
            continue;
        }

        fcst_ipred_prepare(spic, buf_le0, (buf_up0 + 1), cuwh, x, y);
        cost_best = (s32)MAX_COST_RC;

        for (mode = 0; mode < IPD_CNT_B; mode++)
        {
            xeve_ipred(buf_le0, (buf_up0 + 1), NULL, 0, pred, mode, 1 << log2_cuwh, 1 << log2_cuwh);
            cost = xeve_sad_16b(log2_cuwh, log2_cuwh, pred, org, cuwh, s_o, ctx->param.codec_bit_depth);

            if (cost < cost_best)
            {
                cost_best = cost;
            }
        }

        tot_cost += cost_best + intra_penalty;
    }
    return tot_cost;
}

static void set_mv_bound(int x, s32 y, s32 sub_w, s32 sub_h, s16 * min_out, s16  * max_out)
{
    s16 lower_clip[MV_D], upper_clip[MV_D];
    s32 search_range_ipel;
    u8  shift = 2;

    lower_clip[MV_X] = -((PIC_PAD_SIZE_L - 16)) >> shift; /* -32 */
    lower_clip[MV_Y] = -((PIC_PAD_SIZE_L - 16)) >> shift; /* -32 */
    //upper_clip[MV_X] = sub_w - lower_clip[MV_X];          /* w + 32 */
    //upper_clip[MV_Y] = sub_h - lower_clip[MV_Y];          /* h + 32 */
    upper_clip[MV_X] = sub_w;
    upper_clip[MV_Y] = sub_h;
    search_range_ipel = SEARCH_RANGE_IPEL >> (shift - 1);

    min_out[MV_X] = XEVE_CLIP3(lower_clip[MV_X], upper_clip[MV_X], x - search_range_ipel);
    max_out[MV_X] = XEVE_CLIP3(lower_clip[MV_X], upper_clip[MV_X], x + search_range_ipel);
    min_out[MV_Y] = XEVE_CLIP3(lower_clip[MV_Y], upper_clip[MV_Y], y - search_range_ipel);
    max_out[MV_Y] = XEVE_CLIP3(lower_clip[MV_Y], upper_clip[MV_Y], y + search_range_ipel);
}

static void get_mvc_nev(s16 mvc[3][MV_D], s16(*map_mv)[REFP_NUM][MV_D], s32 position, s32 list, s32 w_lcu)
{
    s16 * pred_mv_up, *pred_mv_le, *pred_mv_ul;
    s16   pos_x, pos_y;
    s16   zero_mv[MV_D] = { 0 };

    pos_x = position % w_lcu;
    pos_y = position / w_lcu;

    if (position == 0)
    {
        mvc[0][MV_X] = mvc[0][MV_Y] = 0;
        mvc[1][MV_X] = mvc[1][MV_Y] = 0;
        mvc[2][MV_X] = mvc[2][MV_Y] = 0;
    }
    else if (position >= 1)
    {
        if (pos_x == 0)
        {
            pred_mv_ul = map_mv[-w_lcu][list];
        }
        else if (pos_y == 0)
        {
            pred_mv_ul = map_mv[-1][list];
        }
        else
        {
            pred_mv_ul = map_mv[-w_lcu - 1][list];
        }

        if (pos_x > 0)
        {
            pred_mv_le = map_mv[-1][list];
        }
        else
        {
            pred_mv_le = zero_mv;
        }

        if (pos_y > 0)
        {
            pred_mv_up = map_mv[-w_lcu][list];
        }
        else
        {
            pred_mv_up = zero_mv;
        }

        mvc[0][MV_X] = pred_mv_up[MV_X];
        mvc[0][MV_Y] = pred_mv_up[MV_Y];
        mvc[1][MV_X] = pred_mv_le[MV_X];
        mvc[1][MV_Y] = pred_mv_le[MV_Y];
        mvc[2][MV_X] = pred_mv_ul[MV_X];
        mvc[2][MV_Y] = pred_mv_ul[MV_Y];
    }
}

static void get_mvc_median(s16 * mvc, s16(*map_mv)[REFP_NUM][MV_D], s32 position, s32 list, s32 w_lcu)
{
    s16 * pred_mv_up, *pred_mv_le, *pred_mv_ul;
    s16   pos_x, pos_y;

    pos_x = position % w_lcu;
    pos_y = position / w_lcu;

    if (position == 0)
    {
        mvc[MV_X] = 0;
        mvc[MV_Y] = 0;
    }
    else
    {
        if (pos_x == 0)
        {
            pred_mv_ul = map_mv[-w_lcu][list];
        }
        else if(pos_y == 0)
        {
            pred_mv_ul = map_mv[-1][list];
        }
        else
        {
            pred_mv_ul = map_mv[-w_lcu - 1][list];
        }

        if (pos_x > 0)
        {
            pred_mv_le = map_mv[-1][list];
        }
        else
        {
            pred_mv_le = pred_mv_ul;
        }

        if (pos_y > 0)
        {
            pred_mv_up = map_mv[-w_lcu][list];
        }
        else
        {
            pred_mv_up = pred_mv_ul;
        }

        mvc[MV_X] = XEVE_MEDIAN(pred_mv_up[MV_X], pred_mv_le[MV_X], pred_mv_ul[MV_X]);
        mvc[MV_Y] = XEVE_MEDIAN(pred_mv_up[MV_Y], pred_mv_le[MV_Y], pred_mv_ul[MV_Y]);
    }
}

static s32 fcst_me_ipel(XEVE_PIC * org_pic, XEVE_PIC * ref_pic, s16 * min_mv, s16 * max_mv
                         , s32 x, s32 y, s32 log2_cuwh, s16 mvp[MV_D], u16 lambda, s16 mv[MV_D], int bit_depth)
{
    u8         mv_bits;
    s32        cost, min_cost;
    s32        total_points, pos_idx, prev_pos, org_s, ref_s;
    s32        center_x, center_y;
    pel      * org, *ref;
    s16        cmv[MV_D];
    const u8 * tbl_mv_bits = xeve_tbl_mv_bits;

    org_s = org_pic->s_l;
    ref_s = ref_pic->s_l;
    org = org_pic->y + y * org_s + x;

    prev_pos = 0;
    total_points = FIRST_SEARCH_NUM;
    pos_idx = 0;

    mv[MV_X] >>= 2;
    mv[MV_Y] >>= 2;

    cmv[MV_X] = XEVE_CLIP3(min_mv[MV_X], max_mv[MV_X], mv[MV_X]);
    cmv[MV_Y] = XEVE_CLIP3(min_mv[MV_Y], max_mv[MV_Y], mv[MV_Y]);

    mv_bits  = tbl_mv_bits[(cmv[MV_X] << 2) - mvp[MV_X]];
    mv_bits += tbl_mv_bits[(cmv[MV_Y] << 2) - mvp[MV_Y]];
    cost = lambda * mv_bits;

    ref  = ref_pic->y + cmv[MV_Y] * ref_s + cmv[MV_X];
    min_cost = xeve_sad_16b(log2_cuwh, log2_cuwh, org, ref, org_s, ref_s, bit_depth);

    while (1)
    {
        center_x = mv[MV_X];
        center_y = mv[MV_Y];

        for(int i = 0 ; i < total_points ; i++)
        {
            cmv[MV_X] = center_x + tbl_small_dia_search[pos_idx][MV_X];
            cmv[MV_Y] = center_y + tbl_small_dia_search[pos_idx][MV_Y];

            if (cmv[MV_X] >= max_mv[MV_X] || cmv[MV_X] <= min_mv[MV_X] ||
                cmv[MV_Y] >= max_mv[MV_Y] || cmv[MV_Y] <= min_mv[MV_Y])
            {
                cost = (s32)MAX_COST_RC;
            }
            else
            {
                mv_bits  = tbl_mv_bits[(cmv[MV_X] << 2) - mvp[MV_X]];
                mv_bits += tbl_mv_bits[(cmv[MV_Y] << 2) - mvp[MV_Y]];
                cost = lambda * mv_bits;

                ref  = (u16 *)ref_pic->y + cmv[MV_Y] * ref_s + cmv[MV_X];
                cost += xeve_sad_16b(log2_cuwh, log2_cuwh, org, ref, org_s, ref_s, bit_depth);
            }

            if (cost < min_cost)
            {
                mv[MV_X] = cmv[MV_X];
                mv[MV_Y] = cmv[MV_Y];
                min_cost = cost;
                prev_pos = pos_idx;
            }

            pos_idx += 1;
            pos_idx = pos_idx & 0x3;
        }

        if (center_x == mv[MV_X] && center_y == mv[MV_Y]) break;

        total_points = NEXT_SEARCH_NUM;
        pos_idx = tbl_small_dia_search[prev_pos][NEXT_POS];
    }

    mv[MV_X] <<= 2;
    mv[MV_Y] <<= 2;

    return min_cost;
}

static s32 est_inter_cost(XEVE_CTX * ctx, s32 x, s32 y, XEVE_PICO * pico_cur
                              , XEVE_PICO * pico_ref, s32 list, s32 uni_inter_mode)
{
    s32      mvp_num, pos, sub_w, sub_h, cuwh, log2_cuwh;
    s16      min_mv[MV_D], max_mv[MV_D];
    s16      (*map_mv)[REFP_NUM][MV_D], mvc[4][MV_D];
    s16      mvp[MV_D], mv[MV_D], best_mv[MV_D];
    s32      cost, min_cost;
    u16      lambda;

    sub_w  = pico_cur->spic->w_l;
    sub_h  = pico_cur->spic->h_l;
    mvp_num = 1;

    log2_cuwh = ctx->fcst.log2_fcst_blk_spic + 1;
    cuwh      = 1 << log2_cuwh;
    pos       = (x >> log2_cuwh) + (y >> log2_cuwh) * ctx->w_lcu;
    map_mv    = uni_inter_mode > 1 ? pico_cur->sinfo.map_mv_pga : pico_cur->sinfo.map_mv;
    lambda    = (u16)ctx->rc->lambda[2];

    get_mvc_median(mvc[0], &map_mv[pos], pos, list, ctx->w_lcu);

    if (XEVE_ABS((s32)(pico_cur->pic_icnt - pico_ref->pic_icnt)) != 1)
    {
        get_mvc_nev(mvc + 1, &map_mv[pos], pos, list, ctx->w_lcu);
        mvp_num = 4;
    }

    if (x + cuwh <= sub_w && y + cuwh <= sub_h)
    {
        min_cost = (s32)MAX_COST_RC;
        for (s32 i = 0; i < mvp_num; i++)
        {
            mv[MV_X] = (x << 2) + mvc[i][MV_X];
            mv[MV_Y] = (y << 2) + mvc[i][MV_Y];
            mvp[MV_X] = mv[MV_X];
            mvp[MV_Y] = mv[MV_Y];

            set_mv_bound(mvp[MV_X] >> 2, mvp[MV_Y] >> 2, sub_w, sub_h, min_mv, max_mv);
            cost = fcst_me_ipel(pico_cur->spic, pico_ref->spic, min_mv, max_mv, x, y
                                 , log2_cuwh, mvp, lambda, mv, ctx->param.codec_bit_depth);

            if (cost < min_cost)
            {
                best_mv[MV_X] = mv[MV_X];
                best_mv[MV_Y] = mv[MV_Y];
                min_cost = cost;
            }
        }
        map_mv[pos][list][MV_X] = mv[MV_X] - (x << 2);
        map_mv[pos][list][MV_Y] = mv[MV_Y] - (y << 2);
    }
    else
    {
        min_cost = 0;
        map_mv[pos][list][MV_X] = 0;
        map_mv[pos][list][MV_Y] = 0;
    }

    return min_cost;
}

void uni_direction_cost_estimation(XEVE_CTX * ctx, XEVE_PICO * pico_cur, XEVE_PICO * pico_ref
                                        , s32 is_intra_pic, s32 intra_cost_compute, s32 uni_inter_mode)
{
    s32     lcu_num = 0, x_lcu = 0, y_lcu = 0, log2_cuwh;
    s32 ( * map_lcu_cost)[4];
    u16     intra_blk_cnt = 0; /* count of intra blocks in inter picutre */
    u8    * map_pdir, ref_list;

    map_lcu_cost = pico_cur->sinfo.map_uni_lcost;
    map_pdir = pico_cur->sinfo.map_pdir;
    log2_cuwh =  ctx->fcst.log2_fcst_blk_spic + 1;

    if (intra_cost_compute) pico_cur->sinfo.uni_est_cost[INTRA] = 0;

    pico_cur->sinfo.uni_est_cost[uni_inter_mode] = 0;

    /* get fcost */
    for (lcu_num = 0; lcu_num < ctx->fcst.f_blk; lcu_num++)
    {
        if (intra_cost_compute)
        {
            map_lcu_cost[lcu_num][INTRA] = xeve_est_intra_cost(ctx, x_lcu << log2_cuwh, y_lcu << log2_cuwh) +
                                           ctx->rc->param->sub_pic_penalty;
            pico_cur->sinfo.uni_est_cost[INTRA] += map_lcu_cost[lcu_num][INTRA];
        }

        if (!is_intra_pic)
        {
            map_lcu_cost[lcu_num][uni_inter_mode] = est_inter_cost(ctx, x_lcu << log2_cuwh, y_lcu << log2_cuwh, pico_cur
                                                                      , pico_ref, REFP_0, uni_inter_mode) + ctx->rc->param->sub_pic_penalty;

            if (map_lcu_cost[lcu_num][INTRA] < map_lcu_cost[lcu_num][uni_inter_mode])
            {
                pico_cur->sinfo.uni_est_cost[uni_inter_mode] += map_lcu_cost[lcu_num][INTRA];
                /* increase intra count for inter picture */
                intra_blk_cnt++;
            }
            else
            {
                if(uni_inter_mode == INTER_UNI0) map_pdir[lcu_num] = INTER_L0;
                pico_cur->sinfo.uni_est_cost[uni_inter_mode] += map_lcu_cost[lcu_num][uni_inter_mode];
            }
        }

        x_lcu++;
        if (x_lcu == ctx->fcst.w_blk)
        {
            /* switch to the new lcu row*/
            x_lcu = 0;
            y_lcu++;
        }
    }

    /* Storing intra block count in inter frame*/
    ref_list = uni_inter_mode - 1;
    pico_cur->sinfo.icnt[ref_list] = intra_blk_cnt;

    /* weighting intra fcost */
    if (intra_cost_compute)
    {
        if (pico_cur->pic_icnt == 0)
        {
            pico_cur->sinfo.uni_est_cost[INTRA] = (s32)(pico_cur->sinfo.uni_est_cost[INTRA] >> 1);

        }
        else
        {
            pico_cur->sinfo.uni_est_cost[INTRA] = (s32)((pico_cur->sinfo.uni_est_cost[INTRA] * 3) >> 2);
        }
    }
}

static s32 fcst_me_ipel_b(XEVE_PIC * org_pic, XEVE_PIC * ref_pic_0, XEVE_PIC * ref_pic_1, s32 x, s32 y, s32 log2_cuwh, u16 lambda
                        , s16 mv_l0[MV_D], s16 mvd_L0[MV_D], s16 mv_L1[MV_D], s16 mvd_L1[MV_D], u8 bit_depth, s16* min_mv_l0, s16* max_mv_l0, s16* min_mv_l1, s16* max_mv_l1)
{
    s32        cost;
    u16        wh, mv_bits;
    pel      * org, pred[REFP_NUM][4096], bi_pred[4096];
    const u8 * tbl_mv_bits = xeve_tbl_mv_bits;

    wh = 1 << log2_cuwh;
    org = (u16 *)org_pic->y + y * org_pic->s_l + x;
    mv_bits = tbl_mv_bits[mvd_L0[MV_X]] + tbl_mv_bits[mvd_L0[MV_Y]] +
        tbl_mv_bits[mvd_L1[MV_X]] + tbl_mv_bits[mvd_L1[MV_Y]];

    /* Motion compensation for bi prediction */
    /* Obtain two prediction using L0 mv and L1 mv */
    xeve_mc_fcst(ref_pic_0->y, mv_l0[MV_X], mv_l0[MV_Y], ref_pic_0->s_l, wh, pred[REFP_0], wh, wh, 0, bit_depth, NULL, min_mv_l0, max_mv_l0);
    xeve_mc_fcst(ref_pic_1->y, mv_L1[MV_X], mv_L1[MV_Y], ref_pic_1->s_l, wh, pred[REFP_1], wh, wh, 0, bit_depth, NULL, min_mv_l1, max_mv_l1);

    /* Make bi-prediction using averaging */
    fcst_mc_bi_avg_l(pred, wh, wh, wh, bi_pred, wh, bit_depth);
    cost = xeve_sad_16b(log2_cuwh, log2_cuwh, org, bi_pred, org_pic->s_l, wh, bit_depth);

    cost += lambda * mv_bits;
    cost = (cost * 3) >> 2; /* bi-pred advantage*/

    return cost;
}

static s32 get_bi_lcost(XEVE_CTX * ctx, int x, int y, XEVE_PICO * pico_1,
                XEVE_PICO * pico_0, XEVE_PICO * pico_2, u8 * map_bdir)
{
    int      pos, sub_w, sub_h, cuwh, log2_cuwh;
    s16    min_l0[MV_D], max_l0[MV_D], mvp_l1[MV_D], mv_l1[MV_D], mvp_l0[MV_D];
    s16    min_l1[MV_D], max_l1[MV_D];
    s16    mvc_l0[MV_D], mvc_l1[MV_D], mvd_l0[MV_D], mvd_l1[MV_D], mv_l0[MV_D];
    s16 (* map_mv)[REFP_NUM][MV_D];
    s32    cost_l1,cost_l0, cost, best_cost;
    u16   lambda_p, lambda_b;

    best_cost = XEVE_INT32_MAX;
    log2_cuwh = ctx->fcst.log2_fcst_blk_spic + 1;
    cuwh      = 1 << log2_cuwh;
    pos = ((x >> log2_cuwh) + (y >> log2_cuwh)* ctx->fcst.w_blk);
    map_mv    = pico_1->sinfo.map_mv_bi;

    sub_w = pico_1->spic->w_l;
    sub_h = pico_1->spic->h_l;

    lambda_b = lambda_p = (u16)(0.57 * pow(2.0, (RC_INIT_QP - 12.0) / 3.0));

    if(x + cuwh <= sub_w && y + cuwh <= sub_h)
    {

        /* set maximum/minimum value of search range */
        get_mvc_median(mvc_l0, &map_mv[pos], pos, REFP_0, ctx->fcst.w_blk);
        set_mv_bound(x + (mvc_l0[MV_X] >> 2), y + (mvc_l0[MV_Y] >> 2), sub_w, sub_h, min_l0, max_l0);

        /* Find mvc at pos in fcst_ref */
        mv_l0[MV_X] = mvp_l0[MV_X] = (x << 2) + mvc_l0[MV_X];
        mv_l0[MV_Y] = mvp_l0[MV_Y] = (y << 2) + mvc_l0[MV_Y];

        /* L0-direction motion vector difference */
        cost_l0 = fcst_me_ipel(pico_1->spic, pico_0->spic, min_l0, max_l0, x, y,
            log2_cuwh, mvp_l0, lambda_b, mv_l0, 10);

        mvd_l0[MV_X] = mv_l0[MV_X] - mvp_l0[MV_X];
        mvd_l0[MV_Y] = mv_l0[MV_Y] - mvp_l0[MV_Y];

        if (cost_l0 < best_cost)
        {
            best_cost = cost_l0;
            *map_bdir = INTER_L0;

        }

        /* set maximum/minimum value of search range */
        get_mvc_median(mvc_l1, &map_mv[pos], pos, PRED_L1, ctx->w_lcu);
        set_mv_bound(x + (mvc_l1[MV_X] >> 2), y + (mvc_l1[MV_Y] >> 2),  sub_w, sub_h, min_l1, max_l1);

        /* Find mvc at pos in fcst_ref */
        mv_l1[MV_X] = mvp_l1[MV_X] = (x << 2) + mvc_l1[MV_X];
        mv_l1[MV_Y] = mvp_l1[MV_Y] = (y << 2) + mvc_l1[MV_Y];


        cost_l1 = fcst_me_ipel(pico_1->spic, pico_2->spic, min_l1, max_l1, x, y,
            log2_cuwh, mvp_l1, lambda_b, mv_l1, 10);

        mvd_l1[MV_X] = mv_l1[MV_X] - mvp_l1[MV_X];
        mvd_l1[MV_Y] = mv_l1[MV_Y] - mvp_l1[MV_Y];

        if (cost_l1 < best_cost)
        {
            best_cost = cost_l1;
            *map_bdir = INTER_L1;

        }

        cost = fcst_me_ipel_b(pico_1->spic, pico_0->spic,
            pico_2->spic, x, y, log2_cuwh, lambda_b, mv_l0, mvd_l0, mv_l1,
            mvd_l1, 10, min_l0, max_l0, min_l1, max_l1);

        if (cost< best_cost)
        {
            best_cost = cost;
            *map_bdir = INTER_BI;
        }

        map_mv[pos][PRED_L0][MV_X] = mv_l0[MV_X] - (x << 2);
        map_mv[pos][PRED_L0][MV_Y] = mv_l0[MV_Y] - (y << 2);
        map_mv[pos][PRED_L1][MV_X] = mv_l1[MV_X] - (x << 2);
        map_mv[pos][PRED_L1][MV_Y] = mv_l1[MV_Y] - (y << 2);
    }
    else
    {
        cost    = XEVE_INT32_MAX;
        cost_l1 = XEVE_INT32_MAX;
        cost_l0 = XEVE_INT32_MAX;
        best_cost = XEVE_INT32_MAX;

        map_mv[pos][PRED_L0][MV_X] = 0;
        map_mv[pos][PRED_L0][MV_Y] = 0;
        map_mv[pos][PRED_L1][MV_X] = 0;
        map_mv[pos][PRED_L1][MV_Y] = 0;
    }

    return best_cost;
}

void bi_direction_cost_estimation(XEVE_CTX * ctx, XEVE_PICO * pico_cur, XEVE_PICO * pico_l0, XEVE_PICO * pico_l1)
{
    s32      lcu_num = 0, x_lcu = 0, y_lcu = 0, log2_cuwh, intra_blk_cnt = 0;
    s32(*uni_lcost)[4], uni_min_cost;
    s32      * bi_lcost;

    u8   * map_pdir;

    /* get map_lcost for pictures */
    uni_lcost = pico_cur->sinfo.map_uni_lcost; /* current pic */
    bi_lcost = pico_cur->sinfo.map_bi_lcost; /* current pic */
    log2_cuwh = ctx->fcst.log2_fcst_blk_spic + 1;
    map_pdir = pico_cur->sinfo.map_pdir_bi;

    /* first init delayed_fcost */
    pico_cur->sinfo.bi_fcost = 0;

    while (1)
    {
        /*BI_estimation*/

        bi_lcost[lcu_num] = get_bi_lcost(ctx, x_lcu << log2_cuwh, y_lcu << log2_cuwh, pico_cur, pico_l0, pico_l1, &map_pdir[lcu_num]);
        if (bi_lcost[lcu_num] != XEVE_INT32_MAX)
        {
            bi_lcost[lcu_num] += ctx->rc->param->sub_pic_penalty;
        }

        uni_min_cost = XEVE_MIN(uni_lcost[lcu_num][INTRA], XEVE_MIN(uni_lcost[lcu_num][INTER_UNI0], bi_lcost[lcu_num]));
        if (uni_lcost[lcu_num][INTRA] == uni_min_cost)
        {
            map_pdir[lcu_num] = INTRA;
            intra_blk_cnt++;
        }
        pico_cur->sinfo.bi_fcost += uni_min_cost;

        lcu_num++;
        if (lcu_num == ctx->fcst.f_blk) break;

        x_lcu++;
        if (x_lcu == ctx->fcst.w_blk)
        {
            x_lcu = 0;
            y_lcu++;
        }
    }
    pico_cur->sinfo.icnt[0] = intra_blk_cnt;
    pico_cur->sinfo.bi_fcost = (pico_cur->sinfo.bi_fcost * 10) / 12; /* weighting bi-cost */
}

void set_subpic(XEVE_CTX * ctx, XEVE_PICO* pico, int is_intra_pic)
{
    int gop_idx, gop_pos, pic_icnt = pico->pic_icnt;
    int gop_size = ctx->param.bframes + 1;
    pico->sinfo.scene_type = xeve_fcst_get_scene_type(ctx, pico);

    if (is_intra_pic)
    {
        pico->sinfo.slice_type  = SLICE_I;
        pico->sinfo.slice_depth = FRM_DEPTH_0;
        pico->sinfo.ref_pic[REFP_0] = 0;
        pico->sinfo.ref_pic[REFP_1] = 0;
    }
    /* for GOP size 16, 8, 4, 2 */
    else if (gop_size == 2 || gop_size == 4 || gop_size == 8 || gop_size ==16)
    {
        gop_idx = 4 - XEVE_LOG2(gop_size);
        gop_pos = (pic_icnt-1) % gop_size;
        gop_pos = gop_pos < 0 ? 0 : gop_pos;
        pico->sinfo.slice_type  = SLICE_B;
        pico->sinfo.slice_depth = fcst_rpl_gop[gop_idx][gop_pos][0];
        pico->sinfo.ref_pic[REFP_0] = fcst_rpl_gop[gop_idx][gop_pos][1 + REFP_0];
        pico->sinfo.ref_pic[REFP_1] = fcst_rpl_gop[gop_idx][gop_pos][1 + REFP_1];
    }
    else
    {
        if (pic_icnt == 0 || pic_icnt % gop_size == 0)
        {
            pico->sinfo.slice_type = SLICE_B;
            pico->sinfo.slice_depth = FRM_DEPTH_1;
            pico->sinfo.ref_pic[REFP_0] = 1;
            pico->sinfo.ref_pic[REFP_1] = -1;
        }
        else
        {
            pico->sinfo.slice_type = SLICE_B;
            pico->sinfo.slice_depth = FRM_DEPTH_2;
            pico->sinfo.ref_pic[REFP_0] = 1;
            pico->sinfo.ref_pic[REFP_1] = -1;
        }
    }

}

void get_fcost_fixed_gop(XEVE_CTX * ctx, int is_intra_pic)
{
    XEVE_PICO* pico, * pico_ref, * pico_l0, * pico_l1;
    int           pico_ridx, pic_icnt;
    int        i, pic_icnt_last, depth, refp_l0, refp_l1, gop_size;

    pic_icnt_last = ctx->pico->pic_icnt;
    gop_size = ctx->param.bframes + 1;

    if (ctx->param.gop_size == 1 && ctx->param.keyint != 1) //LD case
    {
         pic_icnt = XEVE_MOD_IDX(ctx->pico->pic_icnt, ctx->pico_max_cnt);
         pico = ctx->pico_buf[pic_icnt];
         refp_l0 = pico->sinfo.ref_pic[REFP_0];
         pico_ridx = XEVE_MOD_IDX(pic_icnt - refp_l0, ctx->pico_max_cnt);
         pico_ref = ctx->pico_buf[pico_ridx];
         uni_direction_cost_estimation(ctx, pico, pico_ref, pico->sinfo.slice_type == SLICE_I, 1, INTER_UNI0);
    }
    else
    {
        int offset = pic_icnt_last == gop_size ? 1 : 0;
        for (depth = FRM_DEPTH_MAX; depth >= 0; depth--)
        {
            for (i = 0; i < gop_size + offset; i++)
            {

                pic_icnt = XEVE_MOD_IDX(pic_icnt_last - i, ctx->pico_max_cnt);
                pico = ctx->pico_buf[pic_icnt];

                if (pico->sinfo.slice_depth != depth) continue;
                refp_l0 = pico->sinfo.ref_pic[REFP_0];
                pico_ridx = XEVE_MOD_IDX(pic_icnt - refp_l0, ctx->pico_max_cnt);
                pico_ref = ctx->pico_buf[pico_ridx];

                uni_direction_cost_estimation(ctx, pico, pico_ref, pico->sinfo.slice_type == SLICE_I, 1, INTER_UNI0);

                /* get BI cost */
                if (B_PIC_ENABLED(ctx) && pic_icnt_last > 0 && pico->sinfo.slice_type == SLICE_B)
                {
                    refp_l0 = pico->sinfo.ref_pic[REFP_0];
                    refp_l1 = pico->sinfo.ref_pic[REFP_1];

                    pico_ridx = XEVE_MOD_IDX(pic_icnt - refp_l0, ctx->pico_max_cnt);
                    pico_l0 = ctx->pico_buf[pico_ridx];

                    pico_ridx = XEVE_MOD_IDX(pic_icnt - refp_l1, ctx->pico_max_cnt);
                    pico_l1 = ctx->pico_buf[pico_ridx];
                    bi_direction_cost_estimation(ctx, pico, pico_l0, pico_l1);

                    xeve_mset(pico->sinfo.map_mv_pga, 0, sizeof(s16) * ctx->f_lcu * REFP_NUM * MV_D);

                    /* get PGA cost */
                    pico_ridx = XEVE_MOD_IDX(((pic_icnt - 1) / ctx->param.gop_size) * ctx->param.gop_size, ctx->pico_max_cnt);
                    pico_ref = ctx->pico_buf[pico_ridx];
                    uni_direction_cost_estimation(ctx, pico, pico_ref, pico->sinfo.slice_type == SLICE_I, 0, INTER_UNI2);
                }
            }
        }
    }
}

int xeve_forecast_fixed_gop(XEVE_CTX* ctx)
{
    XEVE_PICO * pico;
    int        i_period, is_intra_pic = 0;
    int        pic_icnt;

    pico      = ctx->pico;
    pic_icnt  = ctx->pico->pic_icnt;
    i_period  = ctx->param.keyint;
    int gop_size = ctx->param.bframes + 1;

    if ((i_period == 0 && pic_icnt == 0) || (i_period > 0 && pic_icnt % i_period == 0))
    {
        is_intra_pic = 1;
    }

    /* get frame cost(complexity) for current input picture (p1, intra)*/
    set_subpic(ctx, pico, is_intra_pic);

    if (((pic_icnt % gop_size == 0) && (pic_icnt != 0) && ctx->param.use_fcst) || gop_size == 1)
    {
        get_fcost_fixed_gop(ctx, is_intra_pic);
    }

    if (ctx->param.aq_mode != 0)
    {
        adaptive_quantization(ctx);
    }

    if ((pic_icnt % gop_size == 0) && (pic_icnt != 0) && \
        (ctx->param.cutree != 0))
    {
        blk_tree_fixed_gop(ctx);
    }
    return XEVE_OK;
}


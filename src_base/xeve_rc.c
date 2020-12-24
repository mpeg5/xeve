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
#include <math.h>

XEVE_RC_PARAM tbl_rc_param =
{
    32, 0, 1, 28, 1.1, 1.13, 0.4, 1.4983, 0.95, 0.5, 0.4, 0.4, 0.6, 0.1,
    0.15, 0.3, 1.85, 26, 14, 38, 0.04, 0.5, 4, 1.0397, 4, 1.5, 1.5
};

/* weighting factor for current pic to reference pic */
static double tbl_rpic_dist_wt[8] =
{
    1.0,  1.3,  1.4,  1.4,  1.6,  1.6,  1.6,  1.6
};

/* weighting factor for transfer cost */
const u16 tbl_inv_qp_scale[41] =
{
    51, 48, 45, 43, 40, 38, 36, 34, 32, 30, 28, 27, 26, 24, 23, 21, 20, 19, 18, 17, 16,
    15, 14, 13, 13, 12, 11, 11, 10, 10, 9, 8, 8, 8, 7, 7, 6, 6, 6, 5, 5
};

const s32 tbl_ref_gop[4][32][2] =
{
    {
        { 0, 17 },{ 0, 17 },{ 0,  1 },{ 0,  2 },
        { 3,  0 },{ 3,  7 },{ 2,  1 },{ 2, 10 },
        { 0,  3 },{ 0,  4 },{ 4,  3 },{ 3,  2 },
        { 2, 11 },{11, 10 },{10,  1 },{10, 14 },
        {14,  1 },{ 1, 14 },{ 1, 17 },{ 1, 18 },
        { 1, 19 },{ 1, 20 },{20, 19 },{19, 18 },
        {19, 23 },{23, 18 },{18, 25 },{18, 26 },
        {18, 27 },{27, 26 },{26, 29 },{26, 30 }
    },
    {
        { 0,  9 },{ 0,  9 },{ 0,  1 },{ 0,  2 },
        { 2,  1 },{ 2,  3 },{ 4,  2 },{ 4,  6 },
        { 6,  1 },{ 1,  6 },{ 1,  9 },{ 1, 10 },
        {10,  9 },{10, 11 },{10,  9 },{12, 10 },
        { 0,  0 },{ 0,  0 },{ 0,  0 },{ 0,  0 },
        { 0,  0 },{ 0,  0 },{ 0,  0 },{ 0,  0 },
        { 0,  0 },{ 0,  0 },{ 0,  0 },{ 0,  0 },
        { 0,  0 },{ 0,  0 },{ 0,  0 },{ 0,  0 }
    },
    {
        { 0,  5 },{ 0,  5 },{ 0,  1 },{ 1,  0 },
        { 2,  1 },{ 1,  2 },{ 1,  3 },{ 5,  6 },
        { 0,  0 },{ 0,  0 },{ 0,  0 },{ 0,  0 },
        { 0,  0 },{ 0,  0 },{ 0,  0 },{ 0,  0 },
        { 0,  0 },{ 0,  0 },{ 0,  0 },{ 0,  0 },
        { 0,  0 },{ 0,  0 },{ 0,  0 },{ 0,  0 },
        { 0,  0 },{ 0,  0 },{ 0,  0 },{ 0,  0 },
        { 0,  0 },{ 0,  0 },{ 0,  0 },{ 0,  0 }
    },
    {
        { 0,  2 },{ 0,  2 },{ 0,  1 },{ 2,  1 },
        { 0,  0 },{ 0,  0 },{ 0,  0 },{ 0,  0 },
        { 0,  0 },{ 0,  0 },{ 0,  0 },{ 0,  0 },
        { 0,  0 },{ 0,  0 },{ 0,  0 },{ 0,  0 },
        { 0,  0 },{ 0,  0 },{ 0,  0 },{ 0,  0 },
        { 0,  0 },{ 0,  0 },{ 0,  0 },{ 0,  0 },
        { 0,  0 },{ 0,  0 },{ 0,  0 },{ 0,  0 },
        { 0,  0 },{ 0,  0 },{ 0,  0 },{ 0,  0 }
    }
};

static const s8 tbl_small_dia_search[4][3] =
{
    { 0, -1, 3 },{ 1, 0, 0 },{ 0, 1, 1 },{ -1, 0, 2 }
};

__inline static double estimate_frame_bits(XEVE_RCBE  * bit_est, double qf, s32 cpx)
{
    return (bit_est->coef * cpx + bit_est->offset) / (qf * bit_est->cnt);
}

__inline static double qp_to_qf(double qp)
{
    return 0.85 * pow(2.0, (qp - 12.0) / 6.0);
}

__inline static double qf_to_qp(double qf)
{
    return 12.0 + 3.0 * log(qf / 0.85) * 2.88538;
}

int xeve_rc_create(XEVE_CTX * ctx)
{
    /* create RC */
    ctx->rc = xeve_malloc(sizeof(XEVE_RC));
    xeve_assert_rv(ctx->rc != NULL, XEVE_ERR_OUT_OF_MEMORY);
    xeve_mset(ctx->rc, 0, sizeof(XEVE_RC));
    xeve_rc_set(ctx);

    /* create RCORE */
    ctx->rcore = xeve_malloc(sizeof(XEVE_RCORE));
    xeve_assert_rv(ctx->rcore != NULL, XEVE_ERR_OUT_OF_MEMORY);
    xeve_mset(ctx->rcore, 0, sizeof(XEVE_RCORE));
    xeve_rc_rcore_set(ctx);

    XEVE_RC_PARAM *rc_param = ctx->rc->param;
    ctx->rcore->pred = xeve_malloc(sizeof(pel) * rc_param->rc_blk_wh * rc_param->rc_blk_wh);
    xeve_mset(ctx->rcore->pred, 0, sizeof(pel) * rc_param->rc_blk_wh * rc_param->rc_blk_wh);

    return XEVE_OK;
}

int xeve_rc_delete(XEVE_CTX * ctx)
{
    xeve_mfree(ctx->rcore->pred);
    xeve_mfree(ctx->rcore);
    xeve_mfree(ctx->rc);

    return XEVE_OK;
}

int xeve_rc_rcore_set(XEVE_CTX * ctx)
{
    XEVE_PARAM * param = &ctx->param;
    XEVE_RCORE * rcore = ctx->rcore;
    XEVE_RC    * rc    = ctx->rc;

    rcore->qf_limit     = rc->param->qf_diff_lim_frm;
    rcore->offset_ip    = (6.0 * log2(rc->param->intra_rate_ratio));
    rcore->est_bits     = 0;
    rcore->scene_type   = SCENE_NORMAL;
    rcore->filler_byte  = 0;

    for (int i = 0; i < RC_NUM_SLICE_TYPE; i++)
    {
        rcore->qf_min[i] = qp_to_qf(param->qp_min);
        rcore->qf_max[i] = qp_to_qf(param->qp_max);
    }

    return XEVE_OK;
}

int xeve_rc_set(XEVE_CTX * ctx)
{
    XEVE_PARAM    * param = &ctx->param;
    XEVE_RC       * rc = ctx->rc;
    XEVE_RCORE    * rcore = ctx->rcore;
    double          max1, max2;

    /* set default value */
    rc->param       = &tbl_rc_param;
    rc->fps         = param->fps;
    rc->bitrate     = param->bps;
    rc->bpf         = rc->bitrate / rc->fps;
    rc->target_bits = rc->bpf;

    rc->k_param     = 32 * pow(ctx->w * ctx->h / 256, 0.5);    
    rc->frame_bits  = 0;
    rc->qp_cnt      = 0.01;
    rc->qp_sum      = rc->param->init_qp * rc->qp_cnt;
    rc->cpx_cnt     = 0;
    rc->cpx_sum     = 0;
    rc->bpf_decayed = 1.0;

    max1 = XEVE_MAX((ctx->f << 1), rc->bitrate);
    max2 = XEVE_MAX(rc->bitrate * rc->param->max_frm_bits_per_br, rc->bpf * 5);
    rc->max_frm_bits = XEVE_MIN(max1, max2);

    rc->vbv_enabled = param->vbv_enabled;

    for (int i = 0; i < RC_NUM_SLICE_TYPE; i++)
    {
        if (i == SLICE_I)
        {
            rc->bit_estimator[i].cnt = 1;
            rc->bit_estimator[i].coef = 0.1;
            rc->bit_estimator[i].offset = 1;
            rc->bit_estimator[i].decayed = 0.6;
        }
        else if (i == SLICE_P)
        {
            rc->bit_estimator[i].cnt = 1;
            rc->bit_estimator[i].coef = 0.5;
            rc->bit_estimator[i].offset = 1;
            rc->bit_estimator[i].decayed = 0.6;
        }
        else
        {
            rc->bit_estimator[i].cnt = 1;
            rc->bit_estimator[i].coef = 1.0;
            rc->bit_estimator[i].offset = 1;
            rc->bit_estimator[i].decayed = 0.6;
        }
        rc->prev_qf[PREV0][i] = qp_to_qf(rc->param->init_qp);
        rc->prev_qf[PREV1][i] = qp_to_qf(rc->param->init_qp);
    }

    rc->prev_st[PREV0] = SLICE_I;
    rc->prev_st[PREV1] = -1;
        
    if (rc->vbv_enabled)
    {
        rc->vbv_buf_size = param->vbv_buffer_size;
        rc->vbv_buf_fullness = 0;
        rc->bpf_decayed = 1.0 - rc->bpf / rc->vbv_buf_size;
    }
    else
    {
        rc->vbv_buf_size = 0;
    }

    rc->lambda[0] = 0.57 * pow(2.0, (rc->param->init_qp - 12.0) / 3.0);
    rc->lambda[1] = sqrt(rc->lambda[0]);
    rc->lambda[2] = sqrt(rc->lambda[1]);
    rc->lambda[3] = sqrt(rc->lambda[2]);

    return XEVE_OK;
}

static double get_vbv_qfactor_fcst(XEVE_CTX *ctx, XEVE_RCORE * rcore, s32 slice_type, double q)
{
    XEVE_RC    * rc = ctx->rc;
    XEVE_PARAM * param;
    XEVE_PICO  * pico_loop;
    XEVE_RCBE  * bit_estimator;
    s32          i, tot_cnt, over_flag, und_flag, stype, sdepth, pic_cnt, tot_loop;
    s32          exceed_maxbuf, bfrm_num;
    double       q_temp, fur_bit, fur_buf, buf_over_bottom, buf_size, buf_over_thd, buf_full;
    s32          fcost = 0;

    exceed_maxbuf = 0;
    und_flag = over_flag = 1;
    buf_size = rc->vbv_buf_size;
    rc->vbv_buf_fullness = XEVE_MAX(rc->vbv_buf_fullness, 0);
    rc->vbv_buf_fullness = XEVE_MIN(rc->vbv_buf_fullness, buf_size * 1.5);
    if (rc->vbv_buf_fullness > rc->vbv_buf_size)
    {
        q *= rc->vbv_buf_fullness / rc->vbv_buf_size;
    }
    buf_full = rc->vbv_buf_fullness;
    
    buf_over_bottom = buf_size * (rc->param->vbv_buf_of_rate_fcst);
    buf_over_bottom = XEVE_MAX(buf_over_bottom, buf_size / 2);
    

    param = &ctx->param;
    tot_loop = param->num_pre_analysis_frames - param->max_b_frames;
    bit_estimator = (slice_type != SLICE_B) ? &rc->bit_estimator[slice_type] : &rc->bit_estimator[SLICE_I + ctx->slice_depth];
    
    for (s32 loop_fcst = 0; loop_fcst < 250 && (und_flag || over_flag); loop_fcst++)
    {
        /* init future bit, future buf, cnt for current q */
        fur_bit = estimate_frame_bits(bit_estimator, q, rcore->cpx_frm);
        fur_buf = buf_full + fur_bit - rc->bpf;

        if (fur_buf > buf_over_bottom)
        {
            exceed_maxbuf = (s32)((fur_buf - buf_over_bottom) / 2);
            exceed_maxbuf = XEVE_MIN(exceed_maxbuf, (s32)(buf_size / 2));
        }

        tot_cnt = 0;
        pic_cnt = (ctx->pico->pic_icnt + 1) % ctx->pico_max_cnt;
        bfrm_num = 0;

        /* calculate fullness of future buffer */
        for (i = 1; fur_buf < buf_size && fur_buf > 0 && i < param->num_pre_analysis_frames - param->max_b_frames; i++)
        {
            pico_loop = ctx->pico_buf[pic_cnt];
            stype = pico_loop->slice_type;
            sdepth = pico_loop->slice_depth;
            q_temp = q;

            if (stype == SLICE_I)
            {
                bit_estimator = &rc->bit_estimator[SLICE_I];
                fcost = pico_loop->uni_est_cost[INTRA];
                q_temp /= ((1.0 - rc->param->intra_rate_ratio) * (bfrm_num + 1) + 1.0);
                bfrm_num = 0;
            }
            else if (stype == SLICE_P)
            {
                bit_estimator = &rc->bit_estimator[SLICE_P];
                if (ctx->param.max_b_frames > 0)
                {
                    fcost = pico_loop->uni_est_cost[INTER_UNI2];
                    q_temp /= ((1.0 - rc->param->inter_rate_ratio) * (bfrm_num + 1) + 1.0);
                    bfrm_num = 0;
                }
                else
                {
                    fcost = pico_loop->uni_est_cost[INTER_UNI0];
                }
            }
            else /* SLICE B */
            {
                sdepth = pico_loop->slice_depth;
                bit_estimator = &rc->bit_estimator[SLICE_I + sdepth];
                fcost = pico_loop->bi_fcost;
                q_temp *= ((1.0 - rc->param->inter_rate_ratio) * (sdepth) + 1.0);
                bfrm_num++;
            }

            fur_bit = estimate_frame_bits(bit_estimator, q_temp, fcost);

            if (exceed_maxbuf > 0)
            {
                fur_bit *= 1.0 + (double)(i - (tot_loop >> 1)) / tot_loop;
            }

            fur_buf += (fur_bit - rc->bpf);
            tot_cnt++;
            pic_cnt++;

            if (pic_cnt >= ctx->pico_max_cnt) pic_cnt %= ctx->pico_max_cnt;

        }

        buf_over_thd = XEVE_CLIP3(buf_over_bottom, buf_over_bottom + exceed_maxbuf, buf_full - (tot_cnt * rc->bpf) / 1.5);


        /* check future buffer overflow condition */
        if (fur_buf > buf_over_thd)
        {
            q *= 1.02;
            over_flag = 0;
            continue;
        }


        /* check future buffer underflow condition */
        if (fur_buf < XEVE_MIN((buf_size + exceed_maxbuf) * rc->param->vbv_buf_uf_rate_fcst, buf_full + (tot_cnt* rc->bpf) / 2))
        {
            q *= 0.98;
            und_flag = 0;
            continue;
        }
 }

    bit_estimator = (slice_type != SLICE_B) ? &rc->bit_estimator[slice_type] : &rc->bit_estimator[SLICE_I + ctx->slice_depth];
    
    rcore->est_bits = estimate_frame_bits(bit_estimator, q, rcore->cpx_frm);
    return q;
}

double get_qfactor_clip(XEVE_CTX *ctx, XEVE_RCORE * rcore, double qf)
{
    s32          i, i_period, stype, t0, t1, thd_distance, distance;
    double       overflow, qf_min, qf_max, accum_buf;
    double       q_model, q_avg, prev_qf_rate, q_avg_factor, t_d;
    XEVE_RC    * rc  = ctx->rc;
    XEVE_PARAM * param;
    XEVE_PICO  * pico;
    s32          fcost = 0;

    param = &ctx->param;
    accum_buf = 2 * rc->bitrate;
    i_period = param->i_period;
    stype = rcore->stype;
    overflow = 1.0;
    pico = NULL;

    if (stype == SLICE_I && i_period > 1 && rc->prev_st[0] != SLICE_I)
    {
        /* I-picture case (except all intra)*/
        q_model = qf;
        q_avg = qp_to_qf(rc->qp_sum / rc->qp_cnt);
        q_avg_factor = rc->param->intra_qf_thd * XEVE_MIN(10.0, (double)ctx->pico->pic_icnt / i_period);

        if (rcore->scene_type != SCENE_EX_LOW)
        {
            /* modeling qf is too higher than average qf */
            if ((q_avg / q_model) < 0.7)
            {
                q_avg_factor = XEVE_CLIP3(.01, .99, q_avg_factor / (q_avg / q_model));
            }
            /* get qf with weighted sum of q_avg and q_model */
            qf = (q_avg_factor)*(q_avg + (1/q_avg_factor - 1) * q_model);
        }

        prev_qf_rate = qf / rc->prev_qf[PREV0][SLICE_P];
        if (prev_qf_rate < 0.75 || prev_qf_rate > 1.5)
        {
            qf = (rc->param->prev_q_factor)* qf + (1 - rc->param->prev_q_factor) * rc->prev_qf[PREV0][SLICE_P];
        }

        if (param->num_pre_analysis_frames >= 24)
        {
            /* when encount scene change just after IDR, raise up qp to bits */
            t0 = param->fps >> 3;
            t1 = param->i_period >> 3;

            thd_distance = XEVE_MIN(t0, t1);
            distance = thd_distance;

            for (i = 1; i < thd_distance; i++)
            {
                pico = ctx->pico_buf[MOD_IDX(ctx->pico_idx + i, ctx->pico_max_cnt)];
                if (pico->scene_type == SCENE_HIGH)
                {
                    distance = i;
                    break;
                }
            }
            if (distance < thd_distance)
            {
                t_d = rcore->cpx_frm / ((ctx->f / rc->param->cpx_thd_resolution) * rc->param->thd_sc * 3);
                t_d /= (double)(distance);
                qf *= XEVE_CLIP3(1.0, 2.0, t_d);
            }
        }
    }
    else
    {
        if (rcore->scene_type == SCENE_HIGH)
        {
            /* when encount scene change just before IDR, raise up qp to bits */
            t0 = param->fps >> 3;
            t1 = param->i_period >> 3;

            thd_distance = XEVE_MIN(t0, t1);
            distance = param->i_period - (ctx->pico->pic_icnt % param->i_period);

            if (distance < thd_distance)
            {
                t_d = rcore->cpx_frm / ((ctx->f / rc->param->cpx_thd_resolution) * rc->param->thd_sc * 3);
                t_d /= (double)(distance);
                qf *= XEVE_CLIP3(1.0, 2.0, t_d);
            }

            
            for (i = 1; i < thd_distance; i++)
            {
                pico = ctx->pico_buf[MOD_IDX(ctx->pico_idx + i, ctx->pico_max_cnt)];
                if (pico->scene_type == SCENE_HIGH)
                {
                    distance = i;
                    break;
                }
            }

            if (distance < thd_distance)
            {
                t_d = rcore->cpx_frm / ((ctx->f / rc->param->cpx_thd_resolution) * rc->param->thd_sc * 3);
                if (pico->slice_type == SLICE_P)
                {
                    fcost = (ctx->param.max_b_frames > 0) ? 
                            pico->uni_est_cost[INTER_UNI2] : pico->uni_est_cost[INTER_UNI0];
                }
                else /* SLICE_B */
                {
                    fcost = pico->bi_fcost;
                }

                t_d = (double)(rcore->cpx_frm + fcost) / rcore->cpx_frm;
                t_d *= (thd_distance - 1) / distance;
                qf *= XEVE_CLIP3(1.0, 2.0, t_d);
            }
        }
    }

    if (ctx->pico->pic_icnt > 0 && stype != SLICE_B)
    {
        if (rcore->scene_type == SCENE_EX_LOW)
        {
            qf_min = qp_to_qf(rc->param->init_qp) / rcore->qf_limit;
            qf_max = qp_to_qf(rc->param->init_qp) * rcore->qf_limit;
        }
        else if (rcore->scene_type == SCENE_HIGH)
        {
            qf_min = qp_to_qf(rc->param->init_qp) / rcore->qf_limit;
            qf_max = qp_to_qf(rc->param->init_qp) * (rcore->qf_limit * 3);
        }
        else if (stype == SLICE_I)
        {
            qf_min = rc->prev_qf[PREV0][rc->prev_st[PREV0]] / (rcore->qf_limit *1.5);
            qf_max = rc->prev_qf[PREV0][rc->prev_st[PREV0]] * (rcore->qf_limit *1.5);

        }
        else /* SLICE_P */
        {
            qf_min = rc->prev_qf[PREV0][rc->prev_st[PREV0]] / rcore->qf_limit;
            qf_max = rc->prev_qf[PREV0][rc->prev_st[PREV0]] * rcore->qf_limit;

            if (overflow > 1.1 && (int)ctx->pico->pic_icnt >= param->gop_size)
            {
                qf_max *= rcore->qf_limit;
            }
            else if (overflow < 0.9)
            {
                qf_min /= rcore->qf_limit;
            }
        }
        qf = XEVE_CLIP3(qf_min, qf_max, qf);
    }
    return qf;
}
static void get_avg_dqp(XEVE_CTX *ctx, XEVE_RCORE * rcore, s32 * map_qp_offset)
{

    s32    x_lcu, y_lcu, tot_dqp;
    s32   * qp_offset;

    qp_offset = map_qp_offset;
    rcore->avg_dqp = 0;

    for (y_lcu = 0; y_lcu < ctx->h_lcu; y_lcu++)
    {
        tot_dqp = 0;
        for (x_lcu = 0; x_lcu < ctx->w_lcu; x_lcu++)
        {
            tot_dqp += qp_offset[x_lcu];
        }
        qp_offset += ctx->w_lcu;
        rcore->avg_dqp += tot_dqp / (int)ctx->w_lcu;
    }

    rcore->avg_dqp = rcore->avg_dqp / (int)ctx->h_lcu;

    qp_offset = map_qp_offset;
    for (y_lcu = 0; y_lcu < ctx->h_lcu; y_lcu++)
    {

        for (x_lcu = 0; x_lcu < ctx->w_lcu; x_lcu++)
        {
            qp_offset[x_lcu] -= rcore->avg_dqp;
        }
        qp_offset += ctx->w_lcu;
    }
}

static double get_vbv_qfactor(XEVE_CTX *ctx, XEVE_RCORE * rcore, s32 slice_type, double q)
{
    XEVE_RC   * rc = ctx->rc;
    XEVE_RCBE * bit_estimator;
    s32         stype;
    double      buf_full, bits, max_rate, buf_ratio, q_init, buf_overflow;
    double      q_rate = 1.0, buf_size;

    stype = slice_type;
    q_init = q;
    buf_size = rc->vbv_buf_size;
    buf_full = rc->vbv_buf_fullness = XEVE_MAX(rc->vbv_buf_fullness, 0);

    if (((stype == SLICE_P && ctx->param.gop_size > 1) ||
        (stype == SLICE_P && rc->prev_st[PREV0] == SLICE_P) ||
        (stype == SLICE_I && rc->prev_st[PREV0] == SLICE_I)) &&
        (buf_full / buf_size > rc->param->vbv_buf_of_rate * 1.0))
    {
        max_rate = (buf_full / buf_size > 0.85) ? 1.9 : 1.6666;
        q_rate = XEVE_CLIP3(1.01, max_rate, buf_full / (rc->param->vbv_buf_of_rate * buf_size*1.0));
    }
    else if (stype == SLICE_P && (buf_full / buf_size > rc->param->vbv_buf_of_rate * 1.2))
    {
        q_rate = XEVE_CLIP3(1.01, 1.333, buf_full / (rc->param->vbv_buf_of_rate * buf_size *1.2));
    }
    else if (rc->prev_st[PREV0] == SLICE_I && (buf_full / buf_size > rc->param->vbv_buf_of_rate * 2.0))
    {
        q_rate = XEVE_CLIP3(1.01, 1.2, buf_full / (rc->param->vbv_buf_of_rate * buf_size *2.0));
    }
    q *= q_rate;

    bit_estimator = (stype != SLICE_B) ? &rc->bit_estimator[stype] : &rc->bit_estimator[SLICE_I + ctx->slice_depth];

    /*clip2: if est bits is larger than max_frm_bit raise qf */
    bits = estimate_frame_bits(bit_estimator, q, rcore->cpx_frm);
    if (bits > rc->max_frm_bits)
    {
        double factor = 1.0;
        if (stype != SLICE_I)
        {
            factor = bits / rc->max_frm_bits;
        }
        else if (bits > rc->max_frm_bits * 1.5)
        {
            factor = bits / (rc->max_frm_bits * 1.5);
        }
        q *= factor;
        bits = estimate_frame_bits(bit_estimator, q, rcore->cpx_frm);
    }

    /* clip3: if estimated bits are more, increase the qf*/
    buf_ratio = (buf_size >= (5 * rc->bpf)) ? 2 : 1;
    if (bits > (buf_size - buf_full) / buf_ratio)
    {
        q_rate = XEVE_CLIP3(1.05, 2.5, (buf_ratio * bits) / (buf_size - buf_full));
        q *= q_rate;
        bits = estimate_frame_bits(bit_estimator, q, rcore->cpx_frm);
    }

    /* clip4: buffer overflow case */
    buf_overflow = buf_full + bits - buf_size;
    if (buf_overflow > rc->bpf)
    {
        q_rate = XEVE_MIN(buf_overflow / rc->bpf, XEVE_MAX((buf_size / 4) / bits, 1.05));
        q_rate = XEVE_CLIP3(1.0, 3.33, q_rate);
        q *= q_rate;
        bits = estimate_frame_bits(bit_estimator, q, rcore->cpx_frm);
    }

    /* clip5: if the estimated bits are less reduce the qf */
    buf_ratio = (rcore->scene_type == SCENE_EX_LOW || stype == SLICE_B) ? 1<<3 : 1<<2;
    if (bits < rc->bpf / buf_ratio)
    {
        q_rate = bits * buf_ratio / rc->bpf;
        q *= XEVE_CLIP3(0.8, 1.0, q_rate);
        bits = estimate_frame_bits(bit_estimator, q, rcore->cpx_frm);
    }

    /* clip6: buffer underflow case */
    if (stype != SLICE_I)
    {
        static s32 under_flow_cnt = 0;
        double buf_underflow;
        buf_underflow = buf_size * rc->param->vbv_buf_uf_rate - (bits + buf_full - rc->bpf);

        if (buf_underflow > 0)
        {
            double min_under_flow;
            q_rate = bits / (buf_underflow + bits);
            under_flow_cnt++;
            min_under_flow = 3.0/4 - (under_flow_cnt)*0.01;
            min_under_flow = XEVE_CLIP3(0.45, 3.0/4, min_under_flow);
            q *= XEVE_CLIP3(min_under_flow, 1.0, q_rate);
        }
        else
        {
            under_flow_cnt = 0;
        }
        bits = estimate_frame_bits(bit_estimator, q, rcore->cpx_frm);
    }

    /* clip7:restrict the estiamted bits to 30% of the buf size*/
    if (bits > buf_size * rc->param->max_vbv_rate_frm)
    {
        q *= bits / (buf_size * rc->param->max_vbv_rate_frm);
    }

    /* limit qf from inital qf (model qf) */
    q = XEVE_CLIP3(q_init / rc->param->qf_limit_vbv, q_init * rc->param->qf_limit_vbv, q);
    rcore->est_bits = estimate_frame_bits(bit_estimator, q, rcore->cpx_frm);

    return q;
}

static double get_avg_qf_b_frm_hgop(int pos, s32 * depth, s32 future_pos,
    s32 past_pos, double future_val, double past_val, double delta)
{
    double qf;
    s32    half_pos;
    
    *depth = (*depth) + 1;
    half_pos = (future_pos + past_pos + 1) >> 1;
    if (pos == half_pos || future_pos - past_pos < 2)
    {
        qf = ((past_val + future_val) / 2.0) * delta;
        return qf;
    }
    else if (pos < half_pos)
    {
        future_val = sqrt(past_val * future_val) * delta;
        qf = get_avg_qf_b_frm_hgop(pos, depth, half_pos, past_pos, future_val, past_val, delta);
    }
    else
    {
        past_val = sqrt(past_val * future_val) * delta;
        qf = get_avg_qf_b_frm_hgop(pos, depth, future_pos, half_pos, future_val, past_val, delta);
    }

    return qf;
}
static double get_qf(XEVE_CTX *ctx, XEVE_RCORE *rcore) {
    XEVE_PICO * pico;
    double      cpx, qf, cpx_rate, target_bits, min, max;
    double      delta, future_val, past_val;
    s32         pos, future_pos, past_pos, depth;
    XEVE_RC   * rc = ctx->rc;

    /* compexity rate */
    cpx_rate = (rc->param->blank_sc_cplx_ftr *  (ctx->f / rc->param->cpx_thd_resolution)) / rc->bitrate;

    /* target bits */
    target_bits = rc->target_bits;
    pico = ctx->pico;
    rcore->scene_type = pico->scene_type;
    if (ctx->param.use_dqp)
    {
        get_avg_dqp(ctx, rcore, pico->map_qp_offset);
    }

    if (ctx->slice_type == SLICE_I)
    {
        rcore->cpx_frm = pico->uni_est_cost[INTRA];
        if (ctx->param.i_period != 1)
        {
            target_bits *= rc->param->intra_rate_ratio;
        }
    }
    else if (ctx->slice_type == SLICE_P)
    {
        rcore->cpx_frm = (ctx->param.max_b_frames > 0) ? pico->uni_est_cost[INTER_UNI2] : pico->uni_est_cost[INTER_UNI0];
    }
    else /* SLICE_B */
    {
        if (pico->pic_icnt == 1)
        {
            rcore->cpx_frm = (ctx->param.max_b_frames > 0) ? pico->uni_est_cost[INTER_UNI2] : pico->uni_est_cost[INTER_UNI0];
        }
        else
        {
            rcore->cpx_frm = pico->bi_fcost;
        }
    }
    /* cpx_pow */
    if (rcore->scene_type == SCENE_EX_LOW)
    {
        cpx = rcore->cpx_frm; /* do not update - use just current complexity*/
        rcore->cpx_pow = pow(cpx, rc->param->pow_cplx);
        min = qp_to_qf(rc->param->init_qp - 4.0) * cpx_rate * (target_bits / rc->k_param);
        max = qp_to_qf(ctx->param.qp_max) * (target_bits / rc->k_param);
        rcore->cpx_pow = XEVE_CLIP3(min, max, rcore->cpx_pow);
    }
    else
    {
        rc->cpx_sum = (rc->cpx_sum * rc->param->df_cplx_sum) + rcore->cpx_frm;
        rc->cpx_cnt = (rc->cpx_cnt * rc->param->df_cplx_sum) + 1;
        cpx = rc->cpx_sum / rc->cpx_cnt;
        rcore->cpx_pow = pow(cpx, rc->param->pow_cplx);
    }

    /*** GET QF ***************************************************************/
    if (rcore->stype == SLICE_B)
    {
        delta = rc->param->inter_rate_ratio;
        future_val = rc->prev_qf[PREV0][rc->prev_st[PREV0]];
        past_val = rc->prev_qf[PREV1][rc->prev_st[PREV1]];
        depth = 0;
        if (ctx->param.use_hgop)
        {
            
            future_pos = ctx->param.gop_size;
            past_pos = 0;
            pos = (ctx->poc.poc_val + ctx->param.gop_size) % ctx->param.gop_size;
            qf = get_avg_qf_b_frm_hgop(pos, &depth, future_pos, past_pos, future_val, past_val, delta);
        }
        else
        {
            
            future_pos = rc->prev_picnt[PREV0][rc->prev_st[PREV0]];
            past_pos = rc->prev_picnt[PREV1][rc->prev_st[PREV1]];
            pos = ctx->pico->pic_icnt;
            qf = get_avg_qf_b_frm_hgop(pos, &depth, future_pos, past_pos, future_val, past_val, delta);
        }

        return qf;
    }

    qf = rc->k_param * (rcore->cpx_pow / target_bits);

    if (ctx->param.use_dqp)
    {
        qf = qp_to_qf(qf_to_qp(qf) + rcore->avg_dqp);
    }

    if (rcore->scene_type == SCENE_LOW && qf > qp_to_qf(rc->param->init_qp - 8.0)*cpx_rate)
    {
        qf *= XEVE_CLIP3(0.9, 1.0, qp_to_qf(rc->param->init_qp - 8.0) * cpx_rate / qf);
    }

    if (rcore->stype == SLICE_I && rcore->scene_type != SCENE_EX_LOW)
    {
        rc->cpx_sum -= rcore->cpx_frm * 0.5;
        rcore->cpx_pow = pow(rc->cpx_sum / rc->cpx_cnt, rc->param->pow_cplx);
    }
    else if (rcore->scene_type == SCENE_HIGH)
    {
        rc->cpx_sum -= rcore->cpx_frm * 0.4;
        rcore->cpx_pow = pow(rc->cpx_sum / rc->cpx_cnt, rc->param->pow_cplx);
    }

    return qf;
}
static void update_prev_qf(XEVE_RC * rc, s32 stype, double qf, s32 pic_icnt)
{
    /* update previouse slice type */
    if (stype != SLICE_B)
    {
        rc->prev_st[PREV1] = rc->prev_st[PREV0];
        rc->prev_st[PREV0] = stype;

        rc->prev_qf[PREV1][SLICE_I] = rc->prev_qf[PREV0][SLICE_I];
        rc->prev_qf[PREV1][SLICE_P] = rc->prev_qf[PREV0][SLICE_P];
        rc->prev_qf[PREV0][stype] = qf;

        rc->prev_picnt[PREV1][SLICE_I] = rc->prev_picnt[PREV0][SLICE_I];
        rc->prev_picnt[PREV1][SLICE_P] = rc->prev_picnt[PREV0][SLICE_P];
        rc->prev_picnt[PREV0][stype] = pic_icnt;
    }

    if (pic_icnt == 0)
    {
        rc->prev_qf[PREV0][SLICE_P] = qf * rc->param->intra_rate_ratio;
        rc->prev_qf[PREV1][SLICE_P] = qf * rc->param->intra_rate_ratio;
        rc->prev_picnt[PREV0][SLICE_P] = 0;
        rc->prev_picnt[PREV1][SLICE_P] = 0;
    }
}

static void update_bit_estimator(XEVE_RCBE * est_bits, double q, double cpx,
    double bits, XEVE_CTX *ctx)
{
    double coef, prev_coef, offset, coef_factor;


    coef_factor = 1.45;
    coef = bits * q / cpx;
    prev_coef = est_bits->coef / est_bits->cnt;
    coef = XEVE_CLIP3(prev_coef / coef_factor, prev_coef*coef_factor, coef);
    offset = bits * q - coef * cpx;

    est_bits->cnt *= est_bits->decayed;
    est_bits->cnt++;

    if (offset < 0)
    {
        offset = 0;
        coef = bits * q / cpx;
    }
        
    est_bits->coef *= est_bits->decayed;
    est_bits->offset *= est_bits->decayed;

    est_bits->offset += offset;
    est_bits->coef += coef;
}

static void update_rc_model(XEVE_RCORE *rcore, XEVE_RC * rc, s32 bits, s32 max_b_frm)
{
    double eb, bpft;
    double df1 = 0.9975, df2 = 0.9945;
    s32 stype, sdepth;
    stype = rcore->stype;
    sdepth = rcore->sdepth;

    /* update bpf_decayed */
    if (max_b_frm == 0)
    {
        eb = rcore->est_bits;

        if (eb < (bits * 0.80) || eb >(bits * 1.2))
        {
            rc->bpf_decayed *= df1;
        }
        else if (eb > (bits * 0.85) && eb < (bits * 1.15))
        {
            rc->bpf_decayed /= df2;
        }

        bpft = (rc->vbv_buf_size > 0) ? rc->bpf / rc->vbv_buf_size : rc->bpf / rc->bitrate;

        rc->bpf_decayed = XEVE_CLIP3(1.0 - bpft*1.5, 1.0 - bpft*0.1, rc->bpf_decayed);
    }
    else if (stype != SLICE_B)
    {
        eb = rcore->est_bits;

        if (eb < (bits * 0.65) || eb >(bits * 1.35))
        {
            rc->bpf_decayed *= df1;
        }
        else if (eb > (bits * 0.75) && eb < (bits * 1.25))
        {
            rc->bpf_decayed /= df2;
        }

        bpft = (rc->vbv_buf_size > 0) ? rc->bpf / rc->vbv_buf_size : rc->bpf / rc->bitrate;

        rc->bpf_decayed = XEVE_CLIP3(1.0 - bpft*1.5, 1.0 - bpft*0.1, rc->bpf_decayed);
    }

    /* update k_param */
    rc->k_param += (stype != SLICE_B) ? bits * qp_to_qf(rcore->qp) / rcore->cpx_pow : 
        bits * qp_to_qf(rcore->qp) / (rcore->cpx_pow * XEVE_MAX(rc->param->inter_rate_ratio * (sdepth - 1), 1.0));

    rc->k_param *= rc->bpf_decayed;

    /* update target bits */
    rc->target_bits += rc->bpf;
    rc->target_bits *= rc->bpf_decayed;

    /* update qp_sum */
    rc->qp_sum *= rc->param->df_qp_sum;
    rc->qp_sum += rcore->qp + ((stype == SLICE_I) ? rcore->offset_ip : 0);

    /* update qp_cnt */
    rc->qp_cnt *= rc->param->df_qp_sum;
    rc->qp_cnt++;
}

static double get_qfactor(XEVE_CTX *ctx)
{
    XEVE_RCORE * rcore = ctx->rcore;
    XEVE_RC  * rc = ctx->rc;
    double      qf, frm_qf_min, frm_qf_max;

    rcore->stype = ctx->slice_type;
    rcore->sdepth = ctx->slice_depth;

    qf = get_qf(ctx, rcore);
    qf = get_qfactor_clip(ctx, rcore, qf);

    frm_qf_min = rcore->qf_min[rcore->stype];
    frm_qf_max = rcore->qf_max[rcore->stype];
    
    if (rc->vbv_enabled && rcore->cpx_frm > 0)
    {
        /* clipping  qstep min and max before vbv cliping */
        qf = (frm_qf_min == frm_qf_max) ? frm_qf_min : XEVE_CLIP3(frm_qf_min, frm_qf_max, qf);
        qf = (ctx->param.num_pre_analysis_frames > 1) ? get_vbv_qfactor_fcst(ctx, rcore, rcore->stype, qf) : get_vbv_qfactor(ctx, rcore, rcore->stype, qf);
    }
    qf = (frm_qf_min == frm_qf_max) ? frm_qf_min : XEVE_CLIP3(frm_qf_min, frm_qf_max, qf);
    update_prev_qf(rc, rcore->stype, qf, ctx->pico->pic_icnt);
    return qf;
}


int xeve_rc_get_frame_qp(XEVE_CTX *ctx)
{
    double       qp;

    /* qp from qf */
    qp = qf_to_qp(get_qfactor(ctx));

    /* qp clip */
    qp = XEVE_CLIP3(ctx->param.qp_min, ctx->param.qp_max, qp);
    ctx->rcore->qp = qp;

    return XEVE_CLIP3(MIN_QUANT, MAX_QUANT, (int)qp);
}

void xeve_rc_update_frame(XEVE_CTX *ctx, XEVE_RC * rc, XEVE_RCORE * rcore)
{
    s32    stype = rcore->stype;


    double bits = rcore->real_bits;

    if (ctx->param.use_filler_flag) bits -= (rcore->filler_byte << 3);

    rc->frame_bits += (int)bits;

    if (rcore->scene_type != SCENE_EX_LOW)
    {
        /* update RC model */
        update_rc_model(rcore, rc, (int)bits, ctx->param.max_b_frames);

        /* update bits estimated predictor */
        (stype != SLICE_B) ? update_bit_estimator(&rc->bit_estimator[stype], 
            qp_to_qf(rcore->qp), rcore->cpx_frm, bits, ctx) : 
            update_bit_estimator(&rc->bit_estimator[SLICE_I + rcore->sdepth], 
                qp_to_qf(rcore->qp), rcore->cpx_frm, bits, ctx);
    }

    if (rc->vbv_enabled)
    {
        /* update vbv buffer */
        if (rcore->scene_type == SCENE_EX_LOW)
        {
            bits = (s32)rc->bpf;
        }
        rc->vbv_buf_fullness += (bits - rc->bpf);
    }
}


/*Cost Complexity functions*/
u32 xeve_get_aq_blk_sum(void * pic_t, s32 width, s32 height, s32 stride)
{
    s32     i, j;
    u8 * pic;
    u32  sum = 0;

    pic = (u8 *)pic_t;

    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
        {
            sum += pic[j];
        }
        pic += stride;
    }
    return sum;
}

u32 xeve_get_aq_blk_ssum(void * pic_t, s32 width, s32 height, s32 stride)
{
    s32     i, j;
    u8     * pic;
    u32    ssum = 0;

    pic = (u8 *)pic_t;

    for (i = 0; i < height; i++)
    {
        for (j = 0; j < width; j++)
        {
            ssum += (u32)pic[j] * pic[j];
        }
        pic += stride;
    }
    return ssum;
}

static u64 get_lcu_var(XEVE_CTX * ctx, void * pic, s32 log2_w, s32 log2_h, s32 x, s32 y, s32 stride)
{
    s32   i, j, w, h, max_log2, blk_loop;
    u64   sum, ssum, var = 0;
    u8  * org_8, *pic_8;
    u16 * org_16, *pic_16;

    max_log2 = (log2_w == ctx->rc->param->aq_log2_blk_size) ? ctx->log2_max_cuwh : ctx->log2_max_cuwh - 1;

    w = 1 << log2_w;
    h = 1 << log2_h;
    blk_loop = 1 << (max_log2 - log2_w);

    if (ctx->param.bit_depth == 8)
    {
        pic_8 = (u8 *)pic;
        for (i = 0; i < blk_loop; i++)
        {
            for (j = 0; j < blk_loop; j++)
            {
                org_8 = pic_8 + x + (j << log2_w) + (y + (i << log2_h)) * stride;
                sum = xeve_get_aq_blk_sum(org_8, w, h, stride);
                ssum = xeve_get_aq_blk_ssum(org_8, w, h, stride);
                var += (ssum - ((sum * sum) >> (log2_w + log2_h)));
            }
        }
    }
    else
    {
        pic_16 = (u16 *)pic;
        for (i = 0; i < blk_loop; i++)
        {
            for (j = 0; j < blk_loop; j++)
            {
                org_16 = pic_16 + x + (j << log2_w) + (y + (i << log2_h)) * stride;
                sum = xeve_get_aq_blk_sum(org_16, w, h, stride);
                ssum = xeve_get_aq_blk_ssum(org_16, w, h, stride);
                var += (ssum - ((sum * sum) >> (log2_w + log2_h)));
            }
        }
    }
    return (var >> (max_log2 - log2_w));
}


static void adaptive_quantization(XEVE_CTX * ctx)
{
    s32    * qp_offset, lcu_blk_size, lcu_num = 0, x, y, x_lcu = 0, y_lcu = 0, log2_cuwh;
    s32      s_l, s_c;
    u64      var;
    double   aq_bd_const;
    s32      aq_log2_blk_size = ctx->rc->param->aq_log2_blk_size;

    log2_cuwh = ctx->log2_max_cuwh - ctx->rc->param->lcu_depth;
    lcu_blk_size = 1 << log2_cuwh;
    qp_offset = ctx->pico->map_qp_offset;

    if (ctx->param.bit_depth == 8)
    {
        aq_bd_const = 7.2135 * 2;
        s_l = ctx->pico->pic.s_l;
        s_c = ctx->pico->pic.s_c;
    }
    else
    {
        aq_bd_const = ((ctx->param.bit_depth - 8) + 7.2135) * 2;
        s_l = ctx->pico->pic.s_l >> 1;
        s_c = ctx->pico->pic.s_c >> 1;
    }


    while (1)
    {
        x = x_lcu << log2_cuwh;
        y = y_lcu << log2_cuwh;

        if (x + lcu_blk_size >= ctx->w || y + lcu_blk_size >= ctx->h)
        {
            var = 0;
        }
        else
        {
            var = get_lcu_var(ctx, ctx->pico->pic.buf_y, aq_log2_blk_size, aq_log2_blk_size, x, y, s_l);
            var += get_lcu_var(ctx, ctx->pico->pic.buf_u, aq_log2_blk_size - 1, aq_log2_blk_size - 1, (x >> 1), (y >> 1), s_c);
            var += get_lcu_var(ctx, ctx->pico->pic.buf_v, aq_log2_blk_size - 1, aq_log2_blk_size - 1, (x >> 1), (y >> 1), s_c);
        }

        if (var == 0)
        {
            qp_offset[lcu_num] = 0;
        }
        else
        {
            qp_offset[lcu_num] = (int)((ctx->rc->param->aq_strength * (log2(XEVE_MAX((double)var, 1)) - aq_bd_const)) *
                                 ctx->rc->param->aq_mode_str);
            qp_offset[lcu_num] = XEVE_CLIP3(-5, 5, qp_offset[lcu_num]);
        }

        lcu_num++;
        if (lcu_num == ctx->f_lcu) break;

        x_lcu++;
        if (x_lcu == ctx->w_lcu)
        {
            x_lcu = 0;
            y_lcu++;
        }

    }
}

static s32 get_scene_type(XEVE_CTX * ctx, XEVE_PICO * pico)
{
    XEVE_PARAM * param;
    s32          fc_intra, fc_inter, cpx_thd, scn_thd;
    s32          i, ridx, dist_to_p, icnt_mode, stype, scene_type;

    /* init */
    param = &ctx->param;
    stype = pico->slice_type;
    fc_intra = pico->uni_est_cost[INTRA];
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
        fc_inter = pico->bi_fcost;
        /* CHECK ME LATER!!: is it right? * (5/6) for B scene thd?? */
        scn_thd = (s32)((cpx_thd * ctx->rc->param->thd_sc * 4) / 6);
    }
    else /* SLICE_P */
    {
        fc_inter = pico->uni_est_cost[INTER_UNI0];
        dist_to_p = param->max_b_frames + 1;

        if (dist_to_p > 1)
        {
            if (dist_to_p > 2)
            {
                fc_inter = pico->uni_est_cost[INTER_UNI2];
                icnt_mode = INTER_UNI2 - 1;
            }
            else
            {
                fc_inter = pico->uni_est_cost[INTER_UNI1];
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
    else if (fc_inter >= (scn_thd << 1) && pico->icnt[icnt_mode] >= (s32)(ctx->f_lcu * 0.80))
    {
        scene_type = SCENE_HIGH;
    }

    /* if there is any scene_change in a gop, P frame is handled as scene_change */
    if (dist_to_p == param->max_b_frames + 1)
    {
        for (i = 1; i<param->max_b_frames + 1; i++)
        {
            ridx = MOD_IDX(pico->pic_icnt - i, ctx->pico_max_cnt);

            if (ctx->pico_buf[ridx]->scene_type == SCENE_HIGH)
            {
                return SCENE_HIGH;
            }
        }
    }

    return scene_type;
}

/* get availability of lcu in lcu-tree */
static void set_transfer_cost(XEVE_CTX * ctx, s16(*mv_lcu)[MV_D],
    u16 * map_transfer_cost, s32 transfer_cost, s32 * lcu_idx,
    s32 * area_idx, s32 list)
{
    s16 * mv, w_lcu, h_lcu;
    s32    log2_cuwh;

    mv = mv_lcu[list];
    w_lcu = ctx->w_lcu;
    h_lcu = ctx->h_lcu;
    log2_cuwh = ctx->log2_max_cuwh;

    /* for upper left */
    if (mv[MV_X] < w_lcu && mv[MV_Y] < h_lcu && mv[MV_X] >= 0 && mv[MV_Y] >= 0)
    {
        map_transfer_cost[lcu_idx[0]] = CLIP_ADD(map_transfer_cost[lcu_idx[0]],
            (area_idx[0] * transfer_cost + 2048) >> (log2_cuwh * 2));
    }

    /* for upper right */
    if (mv[MV_X] + 1 < w_lcu && mv[MV_Y] < h_lcu && mv[MV_X] + 1 >= 0 && mv[MV_Y] >= 0)
    {
        map_transfer_cost[lcu_idx[1]] = CLIP_ADD(map_transfer_cost[lcu_idx[1]],
            (area_idx[1] * transfer_cost + 2048) >> (log2_cuwh * 2));
    }

    /* for bottom left */
    if (mv[MV_X] < w_lcu && mv[MV_Y] + 1 < h_lcu && mv[MV_X] >= 0 && mv[MV_Y] + 1 >= 0)
    {
        map_transfer_cost[lcu_idx[2]] = CLIP_ADD(map_transfer_cost[lcu_idx[2]],
            (area_idx[2] * transfer_cost + 2048) >> (log2_cuwh * 2));
    }

    /* for bottom right */
    if (mv[MV_X] + 1 < w_lcu && mv[MV_Y] + 1 < h_lcu && mv[MV_X] + 1 >= 0 && mv[MV_Y] + 1 >= 0)
    {
        map_transfer_cost[lcu_idx[3]] = CLIP_ADD(map_transfer_cost[lcu_idx[3]],
            (area_idx[3] * transfer_cost + 2048) >> (log2_cuwh * 2));
    }
}

static void set_lcu_tree_info(XEVE_CTX * ctx, s16(*mv_t)[MV_D], s32 list, s32 * lcu_idx, s32 * area_idx)
{
    s32 t0, cuwh;
    s16 mv[MV_D], mv_det[MV_D];

    t0 = ctx->log2_max_cuwh - ctx->rc->param->lcu_depth;
    cuwh = 1 << t0;

    mv[MV_X] = mv_t[list][MV_X];
    mv[MV_Y] = mv_t[list][MV_Y];

    /* obtain detailed mv propagating cost */
    mv_det[MV_X] = mv[MV_X] & (s16)(cuwh - 1);
    mv_det[MV_Y] = mv[MV_Y] & (s16)(cuwh - 1);

    /* obtain lcu index for propagating cost */
    lcu_idx[0] = (mv[MV_X] >> t0) + (mv[MV_Y] >> t0)      * ctx->w_lcu;
    lcu_idx[1] = ((mv[MV_X] >> t0) + 1) + (mv[MV_Y] >> t0)      * ctx->w_lcu;
    lcu_idx[2] = (mv[MV_X] >> t0) + ((mv[MV_Y] >> t0) + 1) * ctx->w_lcu;
    lcu_idx[3] = ((mv[MV_X] >> t0) + 1) + ((mv[MV_Y] >> t0) + 1) * ctx->w_lcu;

    /* calculate ration of lcu area */
    area_idx[0] = (cuwh - mv_det[MV_X]) * (cuwh - mv_det[MV_Y]);
    area_idx[1] = (mv_det[MV_X]) *   (cuwh - mv_det[MV_Y]);
    area_idx[2] = (cuwh - mv_det[MV_X]) * (mv_det[MV_Y]);
    area_idx[3] = mv_det[MV_X] * mv_det[MV_Y];

}

static s32 get_transfer_cost(XEVE_CTX * ctx, XEVE_PICO * pico_curr, s32 lcu_num)
{
    s32 (* map_lcu_cost_uni)[4], *map_lcu_cost_bi;
    u16  * transfer_in_cost;
    u8   * map_pdir;
    float  intra_cost, transfer_amount, weight;
    s32    qp_offset, inv_qscale;

    /* Get transfer cost of LCU at current lcu_num = transfer_in cost from referencing piture
    stored at transfer_cost buffer at current pic */
    transfer_in_cost = pico_curr->transfer_cost;
    map_lcu_cost_uni = pico_curr->map_lcu_cost_uni;
    map_lcu_cost_bi = pico_curr->map_lcu_cost_bi;
    map_pdir = pico_curr->map_pdir;
    qp_offset = XEVE_CLIP3(-5, 5, pico_curr->map_qp_offset[lcu_num]);
    inv_qscale = tbl_inv_qp_scale[((int)(qp_offset * ctx->rc->param->aq_mode_str) + 10) << 1];
    intra_cost = (float)((map_lcu_cost_uni[lcu_num][INTRA] * inv_qscale) >> 8);
    transfer_amount = transfer_in_cost[lcu_num] + intra_cost;

    if (map_pdir[lcu_num] == INTER_L0)
    {
        weight = (float)(map_lcu_cost_uni[lcu_num][INTRA] - (map_lcu_cost_uni[lcu_num][INTER_UNI0]))
               / map_lcu_cost_uni[lcu_num][INTRA];
    }
    else
    {
        weight = (float)(map_lcu_cost_uni[lcu_num][INTRA] - (map_lcu_cost_bi[lcu_num]))
               / map_lcu_cost_uni[lcu_num][INTRA];
    }

    return (s32)(transfer_amount * weight);
}

static s32 lcu_tree_transfer(XEVE_CTX * ctx, XEVE_PICO * pico_prev_nonb, XEVE_PICO * pico_curr_nonb, XEVE_PICO * pico_curr)
{
    s32    x, y, lcu_idx[4], area_idx[4], x_lcu, y_lcu, lcu_num, dist, log2_unit_cuwh;
    u8   * map_pdir;
    s16 (* map_mv)[REFP_NUM][MV_D];
    u16  * transfer_cost_l0, *transfer_cost_l1;
    s32    transfer_cost;
    s16    mv[REFP_NUM][MV_D], mv_lcu[REFP_NUM][MV_D];

    x_lcu = 0;
    y_lcu = 0;
    lcu_num = 0;

    log2_unit_cuwh = ctx->log2_max_cuwh - ctx->rc->param->lcu_depth;

    dist = pico_curr->pic_icnt - pico_prev_nonb->pic_icnt;
    if (pico_curr->slice_type == SLICE_P && dist > 1)
    {
        map_mv = pico_curr->map_mv_pga;
        /* since use map_mv_pga, we consider dist as 1 */
        dist = 1;
    }
    else
    {
        map_mv = pico_curr->map_mv;
    }

    map_pdir = pico_curr->map_pdir;
    transfer_cost_l0 = pico_prev_nonb->transfer_cost;
    transfer_cost_l1 = pico_curr_nonb->transfer_cost;

    while (1)
    {
        x = x_lcu << log2_unit_cuwh;
        y = y_lcu << log2_unit_cuwh;

        mv[REFP_0][MV_X] = x + (map_mv[lcu_num][REFP_0][MV_X] >> 2) * dist;
        mv[REFP_0][MV_Y] = y + (map_mv[lcu_num][REFP_0][MV_Y] >> 2) * dist;

        dist = (pico_curr->slice_type == SLICE_P) ? 1 : pico_curr_nonb->pic_icnt -
            pico_curr->pic_icnt;

        mv[REFP_1][MV_X] = x + (map_mv[lcu_num][REFP_1][MV_X] >> 2) * dist;
        mv[REFP_1][MV_Y] = y + (map_mv[lcu_num][REFP_1][MV_Y] >> 2) * dist;


        mv_lcu[REFP_0][MV_X] = mv[REFP_0][MV_X] >> log2_unit_cuwh;
        mv_lcu[REFP_0][MV_Y] = mv[REFP_0][MV_Y] >> log2_unit_cuwh;
        mv_lcu[REFP_1][MV_X] = mv[REFP_1][MV_X] >> log2_unit_cuwh;
        mv_lcu[REFP_1][MV_Y] = mv[REFP_1][MV_Y] >> log2_unit_cuwh;

        set_lcu_tree_info(ctx, mv, REFP_0, lcu_idx, area_idx);

        if (map_pdir[lcu_num] != INTRA)
        {
            /* Find transfer_cost */
            transfer_cost = get_transfer_cost(ctx, pico_curr, lcu_num);

            if (transfer_cost > 0)
            {
                if (map_pdir[lcu_num] == INTER_L0 || pico_curr->slice_type == SLICE_P)
                {
                    set_transfer_cost(ctx, mv_lcu, transfer_cost_l0, transfer_cost,
                        lcu_idx, area_idx, REFP_0);
                }
                else if (map_pdir[lcu_num] == INTER_L1)
                {
                    /* transfer_cost = xxx, store at L1 direction  */
                    set_lcu_tree_info(ctx, mv, REFP_1, lcu_idx, area_idx);
                    set_transfer_cost(ctx, mv_lcu, transfer_cost_l1, transfer_cost,
                        lcu_idx, area_idx, REFP_1);
                }
                else
                {
                    /* transfer_cost = xxx, store at both directions */
                    /* split cost 1/2 for each predicted direction (L0, L1) */
                    transfer_cost >>= 1;

                    /* Divide transfer_cost by lcu area */
                    set_transfer_cost(ctx, mv_lcu, transfer_cost_l0, transfer_cost,
                        lcu_idx, area_idx, REFP_0);
                    set_lcu_tree_info(ctx, mv, REFP_1, lcu_idx, area_idx);
                    set_transfer_cost(ctx, mv_lcu, transfer_cost_l1, transfer_cost,
                        lcu_idx, area_idx, REFP_1);
                }
            }
        }

        x_lcu++;

        if (x_lcu == (ctx->w_lcu - 1))  /* SKIP the last lcu in x-direction */
        {
            x_lcu = 0;
            y_lcu++;
            lcu_num++;
        }
        lcu_num++;

        if (y_lcu == (ctx->h_lcu - 1)) break; /* SKIP the last lcu in y-direction */
    }

    return 0;
}

static s32 lcu_tree_end(XEVE_CTX * ctx, XEVE_PICO * pico)
{
    float ratio;
    s32   qp_offset, intra_lcost, inv_qscale, x_lcu = 0, y_lcu = 0, lcu_num = 0;

    while (1)
    {
        qp_offset = XEVE_CLIP3(-5, 5, pico->map_qp_offset[lcu_num]);
        inv_qscale = tbl_inv_qp_scale[((int)(qp_offset * ctx->rc->param->aq_mode_str) + 10) << 1];
        intra_lcost = (pico->map_lcu_cost_uni[lcu_num][INTRA] * inv_qscale) >> 8;

        if (intra_lcost)
        {
            ratio = (float)(log2(intra_lcost + pico->transfer_cost[lcu_num])
                - log2(intra_lcost));
            pico->map_qp_offset[lcu_num] -= (int)(ctx->rc->param->lcu_tree_str * ratio);
            pico->map_qp_offset[lcu_num] = XEVE_CLIP3(-6, 6, pico->map_qp_offset[lcu_num]);

        }

        x_lcu++;

        if (x_lcu == (ctx->w_lcu - 1)) /* SKIP the last lcu in x-direction */
        {
            x_lcu = 0;
            y_lcu++;
            lcu_num++;
        }
        lcu_num++;
        if (y_lcu == (ctx->h_lcu - 1)) break; /* SKIP the last lcu in y-direction */
    }

    return 0;
}

static void lcu_tree_fixed_pga(XEVE_CTX * ctx)
{
    XEVE_PICO * pico_curr, *pico_prev_nonb, *pico_curr_nonb;
    s32          i, j, bframes, pic_idx, b_pic_idx;

    bframes = 0;
    pic_idx = ctx->pico_idx - 1;
    /* Start last picture in i-period */
    pico_curr_nonb = ctx->pico_buf[MOD_IDX(pic_idx, ctx->pico_max_cnt)];

    for (i = 1; i < ctx->param.i_period; i++) /* Find first P in backward */
    {
        pic_idx = ctx->pico_idx - i - 1;
        pico_prev_nonb = ctx->pico_buf[MOD_IDX(pic_idx, ctx->pico_max_cnt)];

        if (pico_prev_nonb->slice_type == SLICE_B)
        {
            bframes++;
            continue;
        }
        else
        {
            if (bframes > 0)
            {
                for (j = 0; j < bframes; j++)
                {
                    /* transfer logic for bframes + 1, P == 1, B == bframes + 1 */
                    b_pic_idx = pic_idx - j + bframes;
                    pico_curr = ctx->pico_buf[MOD_IDX(b_pic_idx, ctx->pico_max_cnt)];
                    lcu_tree_transfer(ctx, pico_prev_nonb, pico_curr_nonb, pico_curr);
                }
            }

            lcu_tree_transfer(ctx, pico_prev_nonb, pico_curr_nonb, pico_curr_nonb);
            lcu_tree_end(ctx, pico_curr_nonb);
            pico_curr_nonb = pico_prev_nonb; /* Change curr_nonb to prev_nonb */
            bframes = 0;
        }
    }
    pico_curr = ctx->pico_buf[MOD_IDX(ctx->pico_idx - ctx->param.i_period, ctx->pico_max_cnt)];
    lcu_tree_end(ctx, pico_curr);
}

void xeve_mc_rc(u16 * ref_t, s32 gmv_x, s32 gmv_y, s32 s_ref, s32 s_pred
              , u16 * pred, s32 w, s32 h, s32 bi, u8 bit_depth, s32 * buf)
{
    u16 * p8u;
    u16 * p16;
    s32   i, j;
    u16 * ref;

    ref = (u16 *)ref_t;
    gmv_x >>= 2;
    gmv_y >>= 2;

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

void xeve_rc_ipred_prepare(XEVE_PIC * spic, u16 * buf_le, u16 * buf_up, s32 cuwh, s32 x, s32 y)
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

void xeve_mc_bi_avg_l(pel pred[][4096], s32 cuw, s32 cuh, s32 cuwh, pel * org_y, s32 y_s, u8 bit_depth)
{
    pel   * p0, *p1, *y, t0;
    s32     i, j;
    s32     shift = 2;

    y = org_y;
    p0 = pred[REFP_0];
    p1 = pred[REFP_1];

    for (i = 0; i < cuh; i++)
    {
        for (j = 0; j < cuw; j++)
        {
            t0 = (p0[j] + p1[j] + (1 << (shift - 1))) >> shift;
            y[j] = (u16)t0;
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

    log2_cuwh      = ctx->log2_max_cuwh - (1 + ctx->rc->param->lcu_depth + ctx->rc->param->intra_depth);
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

        xeve_rc_ipred_prepare(spic, buf_le0, (buf_up0 + 1), cuwh, x, y);
        cost_best = (s32)MAX_COST_RC;

        for (mode = 0; mode < IPD_CNT_B; mode++)
        {
            xeve_ipred(buf_le0, (buf_up0 + 1), NULL, 0, pred, mode, 1 << log2_cuwh, 1 << log2_cuwh);
            cost = xeve_sad_16b(log2_cuwh, log2_cuwh, pred, org, cuwh, s_o, ctx->cdsc.codec_bit_depth);

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
    u8  shift = 1;

    lower_clip[MV_X] = -((PIC_PAD_SIZE_L - 16)) >> shift; /* -32 */
    lower_clip[MV_Y] = -((PIC_PAD_SIZE_L - 16)) >> shift; /* -32 */
    upper_clip[MV_X] = sub_w - lower_clip[MV_X];          /* w + 32 */
    upper_clip[MV_Y] = sub_h - lower_clip[MV_Y];          /* h + 32 */

    search_range_ipel = SEARCH_RANGE_IPEL >> shift;

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

static s32 xeve_rc_me_ipel(XEVE_PIC * org_pic, XEVE_PIC * ref_pic, s16 * min_mv, s16 * max_mv
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

static s32 xeve_est_inter_cost(XEVE_CTX * ctx, s32 x, s32 y, XEVE_PICO * pico_cur
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

    log2_cuwh = ctx->log2_max_cuwh - (1 + ctx->rc->param->lcu_depth);
    cuwh      = 1 << log2_cuwh;
    pos       = (x >> log2_cuwh) + (y >> log2_cuwh) * ctx->w_lcu;
    map_mv    = uni_inter_mode > 1 ? pico_cur->map_mv_pga : pico_cur->map_mv;
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
            cost = xeve_rc_me_ipel(pico_cur->spic, pico_ref->spic, min_mv, max_mv, x, y
                                 , log2_cuwh, mvp, lambda, mv, ctx->cdsc.codec_bit_depth);

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

static void uni_direction_cost_estimation(XEVE_CTX * ctx, XEVE_PICO * pico_cur, XEVE_PICO * pico_ref
                                        , s32 is_intra_pic, s32 intra_cost_compute, s32 uni_inter_mode)
{
    s32     lcu_num = 0, x_lcu = 0, y_lcu = 0, log2_cuwh;
    s32 ( * map_lcu_cost)[4];
    u16     intra_blk_cnt = 0; /* count of intra blocks in inter picutre */
    u8    * map_pdir, ref_list;

    map_lcu_cost = pico_cur->map_lcu_cost_uni;
    map_pdir = pico_cur->map_pdir;
    log2_cuwh = ctx->log2_max_cuwh - (1 + ctx->rc->param->lcu_depth);

    if (intra_cost_compute) pico_cur->uni_est_cost[INTRA] = 0;

    pico_cur->uni_est_cost[uni_inter_mode] = 0;

    /* get fcost */
    for (lcu_num = 0; lcu_num < ctx->f_lcu; lcu_num++)
    {
        if (intra_cost_compute)
        {
            map_lcu_cost[lcu_num][INTRA] = xeve_est_intra_cost(ctx, x_lcu << log2_cuwh, y_lcu << log2_cuwh) +
                                           ctx->rc->param->sub_pic_penalty;
            pico_cur->uni_est_cost[INTRA] += map_lcu_cost[lcu_num][INTRA];
        }

        if (!is_intra_pic)
        {
            map_lcu_cost[lcu_num][uni_inter_mode] = xeve_est_inter_cost(ctx, x_lcu << log2_cuwh, y_lcu << log2_cuwh, pico_cur
                                                                      , pico_ref, REFP_0, uni_inter_mode) + ctx->rc->param->sub_pic_penalty;

            if (map_lcu_cost[lcu_num][INTRA] < map_lcu_cost[lcu_num][uni_inter_mode])
            {
                pico_cur->uni_est_cost[uni_inter_mode] += map_lcu_cost[lcu_num][INTRA];
                /* increase intra count for inter picture */
                intra_blk_cnt++;
            }
            else
            {
                map_pdir[lcu_num] = INTER_L0;
                pico_cur->uni_est_cost[uni_inter_mode] += map_lcu_cost[lcu_num][uni_inter_mode];
            }
        }

        x_lcu++;
        if (x_lcu == ctx->w_lcu)
        {
            /* switch to the new lcu row*/
            x_lcu = 0; y_lcu++;
        }
    }

    /* Storing intra block count in inter frame*/
    ref_list = uni_inter_mode - 1;
    pico_cur->icnt[ref_list] = intra_blk_cnt;

    /* weighting intra fcost */
    if (intra_cost_compute)
    {
        if (pico_cur->pic_icnt == 0)
        {
            pico_cur->uni_est_cost[INTRA] = (s32)(pico_cur->uni_est_cost[INTRA] >> 1);

        }
        else
        {
            pico_cur->uni_est_cost[INTRA] = (s32)((pico_cur->uni_est_cost[INTRA] * 3) >> 2);
        }
    }
}

static s32 fcst_me_ipel_b(XEVE_PIC * org_pic, XEVE_PIC * ref_pic_0, XEVE_PIC * ref_pic_1, s32 x, s32 y, s32 log2_cuwh, u16 lambda
                        , s16 mv_l0[MV_D], s16 mvd_L0[MV_D], s16 mv_L1[MV_D], s16 mvd_L1[MV_D], u8 bit_depth)
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
    xeve_mc_rc(ref_pic_0->y, mv_l0[MV_X], mv_l0[MV_Y], ref_pic_0->s_l, wh, pred[REFP_0], wh, wh, 1, bit_depth, NULL);
    xeve_mc_rc(ref_pic_1->y, mv_L1[MV_X], mv_L1[MV_Y], ref_pic_1->s_l, wh, pred[REFP_1], wh, wh, 1, bit_depth, NULL);

    /* Make bi-prediction using averaging */
    xeve_mc_bi_avg_l(pred, wh, wh, wh, bi_pred, wh, bit_depth);
    cost = xeve_sad_16b(log2_cuwh, log2_cuwh, org, bi_pred, org_pic->s_l, wh, bit_depth);

    cost += lambda * mv_bits;

    return cost;
}

static s32 est_bi_lcost(XEVE_CTX * ctx, s32 x, s32 y, XEVE_PICO * pico_curr, XEVE_PICO * pico_prev, XEVE_PICO * pico_next)
{
    s32    pos, sub_w, sub_h, cuwh, log2_cuwh;
    s16    min[MV_D], max[MV_D], mvp_l1[MV_D], mv_l1[MV_D], mvp_l0[MV_D];
    s16    mvc_l0[MV_D], mvc_l1[MV_D], mvd_l0[MV_D], mvd_l1[MV_D], mv_l0[MV_D];
    s16 (* map_mv)[REFP_NUM][MV_D];
    s32    cost_l1, cost_l2, best_cost;
    u16    lambda_p, lambda_b;
    u8   * map_pdir;

    log2_cuwh = ctx->log2_max_cuwh - (1 + ctx->rc->param->lcu_depth);
    cuwh = 1 << log2_cuwh;
    pos = ((x >> log2_cuwh) + (y >> log2_cuwh) * ctx->w_lcu);
    map_mv = pico_curr->map_mv;
    map_pdir = pico_curr->map_pdir;

    mv_l0[MV_X] = (x << 2) + map_mv[pos][REFP_0][MV_X];
    mv_l0[MV_Y] = (y << 2) + map_mv[pos][REFP_0][MV_Y];

    mvc_l1[MV_X] = 0;
    mvc_l1[MV_Y] = 0;

    sub_w = pico_curr->spic->w_l;
    sub_h = pico_curr->spic->h_l;

    /* set maximum/minimum value of search range */
    set_mv_bound(x, y, sub_w, sub_h, min, max);

    /* Find mvc at pos in fcst_ref */
    get_mvc_median(mvc_l0, &map_mv[pos], pos, REFP_0, ctx->w_lcu);
    get_mvc_median(mvc_l1, &map_mv[pos], pos, REFP_1, ctx->w_lcu);

    /* lambda for only P-slice */
    lambda_p = (u16)ctx->sqrt_lambda[1];
    lambda_b = (u16)ctx->sqrt_lambda[0];

    /* L0-direction motion vector predictor */
    mvp_l0[MV_X] = (x << 2) + mvc_l0[MV_X];
    mvp_l0[MV_Y] = (y << 2) + mvc_l0[MV_Y];

    /* L0-direction motion vector difference */
    mvd_l0[MV_X] = mv_l0[MV_X] - mvp_l0[MV_X];
    mvd_l0[MV_Y] = mv_l0[MV_Y] - mvp_l0[MV_Y];

    /* L1-direction motion vector */
    mv_l1[MV_X] = mvp_l1[MV_X] = (x << 2) + mvc_l1[MV_X];
    mv_l1[MV_Y] = mvp_l1[MV_Y] = (y << 2) + mvc_l1[MV_Y];

    if (x + cuwh <= sub_w && y + cuwh <= sub_h)
    {
        cost_l1 = xeve_rc_me_ipel(pico_curr->spic, pico_next->spic, min, max, x, y
                                , log2_cuwh, mvp_l1, lambda_p, mv_l1, ctx->cdsc.codec_bit_depth);

        mvd_l1[MV_X] = mv_l1[MV_X] - mvp_l1[MV_X];
        mvd_l1[MV_Y] = mv_l1[MV_Y] - mvp_l1[MV_Y];
        cost_l2 = fcst_me_ipel_b(pico_curr->spic, pico_prev->spic, pico_next->spic, x, y
                               , log2_cuwh, lambda_b, mv_l0, mvd_l0, mv_l1, mvd_l1, 10);
    }
    else
    {
        cost_l2 = 0;
        cost_l1 = 0;
    }

    if (cost_l1 > cost_l2)
    {
        map_pdir[pos] = INTER_BI;
        best_cost = cost_l2;
        map_mv[pos][REFP_0][MV_X] = mv_l0[MV_X] - (x << 2);
        map_mv[pos][REFP_0][MV_Y] = mv_l0[MV_Y] - (y << 2);
        map_mv[pos][REFP_1][MV_X] = mv_l1[MV_X] - (x << 2);
        map_mv[pos][REFP_1][MV_Y] = mv_l1[MV_Y] - (y << 2);
    }
    else
    {
        map_pdir[pos] = INTER_L1;
        best_cost = cost_l1;
        map_mv[pos][REFP_1][MV_X] = mv_l1[MV_X] - (x << 2);
        map_mv[pos][REFP_1][MV_Y] = mv_l1[MV_Y] - (y << 2);
    }

    return best_cost;
}

static void bi_direction_cost_estimation(XEVE_CTX * ctx, XEVE_PICO * pico_cur, XEVE_PICO * pico_l0, XEVE_PICO * pico_l1)
{
    s32      lcu_num = 0, x_lcu = 0, y_lcu = 0, log2_cuwh;
    s32(*uni_lcost)[4];
    s32      * bi_lcost;

    /* get map_lcost for pictures */
    uni_lcost = pico_cur->map_lcu_cost_uni; /* current pic */
    bi_lcost = pico_cur->map_lcu_cost_bi; /* current pic */
    log2_cuwh = ctx->log2_max_cuwh - (1 + ctx->rc->param->lcu_depth);

    /* first init delayed_fcost */
    pico_cur->bi_fcost = 0;

    while (1)
    {
        /*BI_estimation*/
        bi_lcost[lcu_num] = est_bi_lcost(ctx, x_lcu << log2_cuwh, y_lcu << log2_cuwh, pico_cur, pico_l0, pico_l1) +
                                         ctx->rc->param->sub_pic_penalty;
        pico_cur->bi_fcost += XEVE_MIN(uni_lcost[lcu_num][INTRA], XEVE_MIN(uni_lcost[lcu_num][INTER_UNI0], bi_lcost[lcu_num]));
        
        lcu_num++;
        if (lcu_num == ctx->f_lcu) break;

        x_lcu++;
        if (x_lcu == ctx->w_lcu)
        {
            x_lcu = 0;
            y_lcu++;
        }
    }

    pico_cur->bi_fcost = (pico_cur->bi_fcost * 10) / 12; /* weighting bi-cost */

}

void get_frame_complexity(XEVE_CTX * ctx, s32 is_intra_pic)
{
    XEVE_PARAM * param;
    XEVE_PICO  * pic_orig, *pico_ref, *pico_prev_ref, *pico_future_ref, *pico_anchor;
    s32          pico_ref_idx, inp_pic_cnt;
    s32          gop_idx;

    gop_idx      = 4 - XEVE_LOG2(ctx->param.gop_size);
    param        = &ctx->param;
    pic_orig     = ctx->pico;
    inp_pic_cnt  = pic_orig->pic_icnt;
    pico_ref_idx = tbl_ref_gop[gop_idx][ctx->pico->pic_icnt % (ctx->pico_max_cnt + 1)][0];
    pico_ref     = ctx->pico_buf[pico_ref_idx];

    /*get intra, P1 cost*/
    uni_direction_cost_estimation(ctx, pic_orig, pico_ref, is_intra_pic, 1, INTER_UNI0);

    if (ctx->param.max_b_frames > 0 && inp_pic_cnt > 0)
    {
        /* get Bi cost */
        if (inp_pic_cnt > 1 && ((inp_pic_cnt - 1) % param->gop_size) != 0)
        {
            pico_anchor = pic_orig;
            pico_ref_idx = tbl_ref_gop[gop_idx][ctx->pico->pic_icnt % (ctx->pico_max_cnt + 1)][0];
            pico_prev_ref = ctx->pico_buf[pico_ref_idx];
            pico_ref_idx = tbl_ref_gop[gop_idx][ctx->pico->pic_icnt % (ctx->pico_max_cnt + 1)][1];
            pico_future_ref = ctx->pico_buf[pico_ref_idx];

            bi_direction_cost_estimation(ctx, pico_anchor, pico_prev_ref, pico_future_ref);
        }

        /* get PGA cost */
        pico_ref_idx = tbl_ref_gop[gop_idx][ctx->pico->pic_icnt % (ctx->pico_max_cnt + 1)][0];
        pico_ref = ctx->pico_buf[pico_ref_idx];

        uni_direction_cost_estimation(ctx, pic_orig, pico_ref, is_intra_pic, 0, INTER_UNI2);
    }
}


int xeve_rc_frame_est(XEVE_CTX * ctx)
{
    XEVE_PICO   * pico, *pico_non_intra;
    XEVE_PARAM  * param;
    s32           pic_icnt, intra_period, is_intra_pic = 0;

    ctx->pico_idx = (ctx->pic_icnt - ctx->frm_rnum) % ctx->pico_max_cnt;
    ctx->pico = ctx->pico_buf[ctx->pico_idx];
    pico = ctx->pico;
    pico->slice_type = ctx->slice_type;
    pic_icnt = ctx->pico->pic_icnt;
    param = &ctx->param;
    intra_period = param->i_period;
    pico_non_intra = NULL;

    if ((intra_period == 0 && pic_icnt == 0) ||
        (intra_period > 0 && ctx->pic_icnt % intra_period == 0) || pic_icnt == 0)
    {
        is_intra_pic = 1;
    }

    get_frame_complexity(ctx, is_intra_pic);

    if (pic_icnt == 0)
    {
        pico->slice_type = SLICE_I;
        pico->slice_depth = FRM_DEPTH_0;
        pico->scene_type = SCENE_NORMAL;
        return XEVE_OK;
    }

    /* Scene-type detection at every first frame of the gop*/
    if (pic_icnt % param->gop_size == 0)
    {
        if (is_intra_pic)
        {
            pico->slice_type = SLICE_I;
            pico->slice_depth = FRM_DEPTH_0;
            pico->scene_type = get_scene_type(ctx, pico);
        }
        else
        {
            pico_non_intra = ctx->pico_buf[MOD_IDX(ctx->pico_idx, ctx->pico_max_cnt)];
            pico_non_intra->slice_type = SLICE_B;
            pico_non_intra->slice_depth = FRM_DEPTH_1;
            pico_non_intra->scene_type = get_scene_type(ctx, pico_non_intra);
        }
    }

    /* Adaptive quantization in case of dqp */
    if (ctx->param.use_dqp)
    {
        adaptive_quantization(ctx);
        if ((ctx->pic_icnt % intra_period == 0) && (pic_icnt != 0))
        {
            lcu_tree_fixed_pga(ctx);
        }
    }

    return XEVE_OK;
}

void xeve_rc_gen_subpic(pel * src_y, pel * dst_y, int w, int h, int s_s, int d_s, int bit_depth)
{
    pel * src_b, *src_t;
    pel * dst;
    int   x, k, y, shift;

    src_t = src_y;
    src_b = src_t + s_s;
    dst = dst_y;
    shift = 2;

    for (y = 0; y < h; y++)
    {
        for (x = 0; x < w; x++)
        {
            k = x << 1;
            dst[x] = ((src_t[k] + src_b[k] + src_t[k + 1] + src_b[k + 1] + (1 << (shift - 1))) >> shift);
        }
        src_t += (s_s << 1);
        src_b += (s_s << 1);
        dst += d_s;
    }
}

int xeve_rc_get_qp(XEVE_CTX *ctx)
{
    int qp;
    /* start rc update after finishing first frame encoding */
    if (ctx->pic_cnt > 0)
    {
        xeve_rc_update_frame(ctx, ctx->rc, ctx->rcore);
    }
    qp = xeve_rc_get_frame_qp(ctx);
    return qp;
}
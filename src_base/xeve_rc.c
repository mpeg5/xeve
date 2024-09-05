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

// clang-format off

#define XEVE_VBV_MSEC_DEFAULT 2000 /* msec */

const static XEVE_RC_PARAM tbl_rc_param =
{
    32, 0, 1, 28, 1.3F, 1.13F, 0.4F, 1.4983F, 0.95F, 0.5F, 0.4F, 0.4F, 0.6F, 0.1F,
    0.15F, 0.3F, 1.85F, 26, 14, 38, 0.04F, 0.5F, 4, 1.0397F, 4, 1.5F, 1.5F
};

const static s32 tbl_ref_gop[4][32][2] =
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

// clang-format on

__inline static double estimate_frame_bits(XEVE_RCBE  * bit_est, double qf, s32 cpx)
{
    return (bit_est->coef * cpx + bit_est->offset) / (qf * bit_est->cnt);
}

__inline static double qp_to_qf(double qp)
{
    return 0.85 * pow(2.0, (qp - 21.0) / 8.4);
}

__inline static double qf_to_qp(double qf)
{
    return 21.0 + 4.2 * log(qf / 0.85) * 2.88538;
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

    const XEVE_RC_PARAM *rc_param = ctx->rc->param;
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
    XEVE_RCORE * rcore = ctx->rcore;
    XEVE_RC    * rc    = ctx->rc;

    rcore->qf_limit     = rc->param->qf_diff_lim_frm;
    rcore->offset_ip    = (6.0 * log2(rc->param->intra_rate_ratio));
    rcore->est_bits     = 0;
    rcore->scene_type   = SCENE_NORMAL;
    rcore->filler_byte  = 0;

    for (int i = 0; i < RC_NUM_SLICE_TYPE; i++)
    {
        rcore->qf_min[i] = qp_to_qf(ctx->param.qp_min);
        rcore->qf_max[i] = qp_to_qf(ctx->param.qp_max);
    }

    return XEVE_OK;
}

static double rc_bpf_ra[3][8][10] =
{
    { /* GOP 4 */
        { 2.21, 2.21, 0.95, 0.47 },
    },
    { /* GOP 8 */
        { 2.25, 2.25, 1.15, 1.08, 0.56, 0.56 },
    },
    { /* GOP 16 */
        { 2.70, 2.70, 2.04, 1.15, 1.05, 0.54 },
    },
};

static double rc_bpf_ld[3][10] =
{
    { 15.00, 50.00, 50.00,  0.00,  0.00 }, // LD GOP 2
    { 15.00, 30.00, 25.00, 30.00,  0.00 }, // LD GOP 4
    { 15.00, 32.25, 17.25, 25.00, 25.00 }, // LD GOP 8
};

void xeve_init_rc_bpf_tbl(XEVE_CTX * ctx)
{
    XEVE_RC       * rc = ctx->rc;

    int ld_struct = ctx->param.ref_pic_gap_length;
    int fnum_in_sec[10];
    int ngop_in_sec = ((int)((float)ctx->param.fps.num/ ctx->param.fps.den + 0.5) + ld_struct - 1) / ld_struct;

    for (int i = ld_struct; i > 0; i = i >> 1)
    {
        int idx = XEVE_LOG2(i);
        fnum_in_sec[idx] = ngop_in_sec * XEVE_MAX(1, i >> 1);
        rc_bpf_ld[rc->st_idx][idx + 1] = rc_bpf_ld[rc->st_idx][idx + 1] / fnum_in_sec[idx];
    }
}

void xeve_set_rc_bpf(XEVE_CTX * ctx)
{
    XEVE_RC       * rc = ctx->rc;

    /*
    RC_CBR_EQUAL should be deprecated
    if (param->rc_type == RC_CBR_EQUAL ||   param->iperiod == 1) // AI
    */

    if (ctx->param.keyint == 1) // AI
    {
        for (int i = 0; i < 6; i++)
        {
            rc->bpf_tid[i] = rc->bitrate / rc->fps;
        }
    }
    else if (ctx->param.bframes > 0) // RA
    {
        for (int i = 0; i < 6; i++)
        {
            rc->bpf_tid[i] = (rc->bitrate / rc->fps) * rc_bpf_ra[rc->st_idx][0][i];
        }
    }
    else // LD
    {
        for (int i = 0; i < 6; i++)
        {
            rc->bpf_tid[i] = rc->bitrate / rc->fps; // not ref_pic_gap_length relevance in LD
        }
    }
}

void xeve_init_rc(XEVE_CTX * ctx)
{
    XEVE_RC       * rc = ctx->rc;

    for (int i = 0; i < RC_NUM_SLICE_TYPE; i++)
    {
        if (i == SLICE_I - 1)
        {
            rc->bit_estimator[i].cnt = 1;
            rc->bit_estimator[i].coef = 0.1;
            rc->bit_estimator[i].offset = 1;
            rc->bit_estimator[i].decayed = 0.6;
        }
        else if (i == SLICE_P - 1)
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

        if (rc->encoding_mode != XEVE_LD)
        {
            rc->rc_model[i].k_param = (double)(20 * pow(ctx->w * ctx->h / 256.0, 0.5));
        }
        else
        {
            rc->rc_model[i].k_param = (double)(12 * pow(ctx->w * ctx->h / 256.0, 0.5)); // tuned for lower QP at the beginning in LD
        }
        rc->rc_model[i].target_bits = 0;
        rc->rc_model[i].qp_cnt = 0.01;
        rc->rc_model[i].qp_sum = rc->param->init_qp * rc->rc_model[i].qp_cnt;
        rc->rc_model[i].cpx_cnt = 0;
        rc->rc_model[i].cpx_sum = 0;
        rc->rc_model[i].bpf_decayed = 1.0;
        if (rc->vbv_buf_size)
        {
            rc->rc_model[i].bpf_decayed = 1.0 - rc->bpf / rc->vbv_buf_size;
        }
    }

    rc->prev_st[PREV0] = SLICE_I;
    rc->prev_st[PREV1] = -1;

    if (ctx->param.rc_type == RC_CRF)
    {
        xeve_assert(ctx->param.crf <= 49 && ctx->param.crf >= 10); // asserting CRF value to be between 10-49
        int log2_fcst_blk_spic = 4; /* 16x16 in half image*/
        int w_blk = (ctx->w / 2 + (((1 << (log2_fcst_blk_spic + 1)) - 1))) >> (log2_fcst_blk_spic + 1);
        int h_blk = (ctx->h / 2 + (((1 << (log2_fcst_blk_spic + 1)) - 1))) >> (log2_fcst_blk_spic + 1);
        int f_blk = w_blk * h_blk;
        rc->basecplx = f_blk * 650.0;
    }

    if (ctx->param.gop_size == 1 && ctx->param.ref_pic_gap_length != 0)
    {
        xeve_init_rc_bpf_tbl(ctx);
    }
}

int xeve_rc_set(XEVE_CTX * ctx)
{
    XEVE_RC       * rc = ctx->rc;
    XEVE_RCORE    * rcore = ctx->rcore;
    double          max1, max2;

    /* set default value */
    rc->param        = &tbl_rc_param;
    rc->fps          = (double)ctx->param.fps.num/ ctx->param.fps.den;
    rc->bitrate      = (double)(ctx->param.bitrate * 1000);
    rc->fps_idx      = (((int)rc->fps + (ctx->param.gop_size >> 1)) / ctx->param.gop_size) - 1;
    rc->prev_bpf     = 0;
    rc->frame_bits   = 0;
    rc->total_frames = 0;
    rc->prev_adpt    = 0;


    if (ctx->param.keyint == 0 && ctx->param.ref_pic_gap_length > 0) // LD Case
    {
        // Table Index to be clipped to 0, if ctx->param.ref_pic_gap_length = 1
        rc->st_idx = XEVE_MAX(0, XEVE_LOG2(ctx->param.ref_pic_gap_length) - 1);
    }
    else if (ctx->param.bframes > 0)
    {
        // Table Index to be clipped to 0, if ctx->param.bframes = 1 and ctx->param.gop_size = 2
        rc->st_idx = XEVE_MAX(0, XEVE_LOG2(ctx->param.gop_size) - 2);
    }
    else
    {
        rc->st_idx = 0;
    }

    if (ctx->param.keyint == 1) // AI
    {
        rc->encoding_mode = XEVE_AI;
    }
    else if (ctx->param.bframes > 0) // RA
    {
        rc->encoding_mode = XEVE_RA;
    }
    else // LD
    {
        rc->encoding_mode = XEVE_LD;
    }

    xeve_init_rc(ctx);
    rc->rcm = &rc->rc_model[0];

    xeve_set_rc_bpf(ctx);
    rc->bpf = rc->bpf_tid[0];

    max1 = XEVE_MAX((ctx->f << 1), rc->bitrate);
    max2 = XEVE_MAX(rc->bitrate * rc->param->max_frm_bits_per_br, rc->bpf * 5);
    rc->max_frm_bits = XEVE_MIN(max1, max2);
    rc->vbv_enabled = 1;

    if (ctx->param.vbv_bufsize > 0)
    {
        rc->vbv_buf_size = (double)(ctx->param.vbv_bufsize * 1000);
    }
    else
    {
        rc->vbv_buf_size = ((rc->bitrate) * (XEVE_VBV_MSEC_DEFAULT/1000.0));
    }
    rc->vbv_buf_fullness = 0;

    rc->lambda[0] = 0.57 * pow(2.0, (rc->param->init_qp - 12.0) / 3.0);
    rc->lambda[1] = sqrt(rc->lambda[0]);
    rc->lambda[2] = sqrt(rc->lambda[1]);
    rc->lambda[3] = sqrt(rc->lambda[2]);

    return XEVE_OK;
}

static double get_vbv_qfactor_fcst(XEVE_CTX *ctx, XEVE_RCORE * rcore, s32 slice_type, double q)
{
    XEVE_RC    * rc = ctx->rc;
    XEVE_PICO  * pico_loop;
    XEVE_RCBE  * bit_estimator;
    s32          i, tot_cnt, over_flag, und_flag, stype, sdepth, pic_cnt, tot_loop;
    s32          exceed_maxbuf, bfrm_num;
    double       q_temp, fur_bit, fur_buf, buf_over_bottom, buf_size, buf_over_thd, buf_full;
    double       q_init = q;
    double       rc_bpf;
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


    tot_loop = ctx->param.lookahead - ctx->param.bframes;
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
        rc_bpf = 0;
        pic_cnt = (ctx->pico->pic_icnt + 1) % ctx->pico_max_cnt;
        bfrm_num = 0;

        /* calculate fullness of future buffer */
        for (i = 1; fur_buf < buf_size && fur_buf > 0 && i < ctx->param.lookahead - ctx->param.bframes; i++)
        {
            pico_loop = ctx->pico_buf[pic_cnt];
            stype = pico_loop->sinfo.slice_type;
            sdepth = pico_loop->sinfo.slice_depth;
            rc_bpf += rc->bpf_tid[sdepth];
            q_temp = q;

            if (stype == SLICE_I)
            {
                bit_estimator = &rc->bit_estimator[SLICE_I];
                fcost = pico_loop->sinfo.uni_est_cost[INTRA];
                q_temp /= ((1.0 - rc->param->intra_rate_ratio) * (bfrm_num + 1) + 1.0);
                bfrm_num = 0;
            }
            else if (stype == SLICE_P || sdepth == 0)
            {
                bit_estimator = &rc->bit_estimator[SLICE_P];
                if (ctx->param.bframes > 0)
                {
                    fcost = pico_loop->sinfo.uni_est_cost[INTER_UNI2];
                    q_temp /= ((1.0 - rc->param->inter_rate_ratio) * (bfrm_num + 1) + 1.0);
                    bfrm_num = 0;
                }
                else
                {
                    fcost = pico_loop->sinfo.uni_est_cost[INTER_UNI0];
                }
            }
            else /* SLICE B */
            {
                sdepth = pico_loop->sinfo.slice_depth;
                bit_estimator = &rc->bit_estimator[SLICE_I + sdepth];
                fcost = pico_loop->sinfo.bi_fcost;
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

        buf_over_thd = XEVE_CLIP3(buf_over_bottom, buf_over_bottom + exceed_maxbuf, buf_full - (rc_bpf) / 1.5);

        /* check future buffer overflow condition */
        if (fur_buf > buf_over_thd)
        {
            q *= 1.02;
            over_flag = 0;
            continue;
        }

        /* check future buffer underflow condition */
        if (fur_buf < XEVE_MIN((buf_size + exceed_maxbuf) * rc->param->vbv_buf_uf_rate_fcst, buf_full + (rc_bpf) / 2))
        {
            q *= 0.98;
            und_flag = 0;
            continue;
        }
 }

    bit_estimator = (slice_type != SLICE_B) ? &rc->bit_estimator[slice_type] : &rc->bit_estimator[SLICE_I + ctx->slice_depth];

    rcore->est_bits = estimate_frame_bits(bit_estimator, q, rcore->cpx_frm);
    // in case of capped crf return max of initial qf and vbv qf
    return XEVE_MAX(q, q_init);
}

double get_qfactor_clip(XEVE_CTX *ctx, XEVE_RCORE * rcore, double qf)
{
    s32          i, i_period, stype, t0, t1, thd_distance, distance;
    double       overflow, qf_min, qf_max, accum_buf;
    double       q_model, q_avg, prev_qf_rate, q_avg_factor, t_d;
    XEVE_RC    * rc  = ctx->rc;
    XEVE_PICO  * pico;
    s32          fcost = 0;

    accum_buf = 2 * rc->bitrate;
    i_period = ctx->param.keyint != 0? ctx->param.keyint : MAX_INTRA_PERIOD_RC;
    stype = rcore->stype;
    overflow = 1.0;
    pico = NULL;

    if (stype == SLICE_I && i_period > 1 && rc->prev_st[0] != SLICE_I)
    {
        /* I-picture case (except all intra)*/
        q_model = qf;
        q_avg = qp_to_qf(rc->rcm->qp_sum / rc->rcm->qp_cnt);
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

        if (ctx->param.lookahead >= 24)
        {
            /* when encount scene change just after IDR, raise up qp to bits */
            t0 = (int)((float)ctx->param.fps.num/ ctx->param.fps.den + 0.5) >> 3;
            t1 = i_period >> 3;

            thd_distance = XEVE_MIN(t0, t1);
            distance = thd_distance;

            for (i = 1; i < thd_distance; i++)
            {
                pico = ctx->pico_buf[XEVE_MOD_IDX(ctx->pico_idx + i, ctx->pico_max_cnt)];
                if (pico->sinfo.scene_type == SCENE_HIGH)
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
            t0 = (int)((float)ctx->param.fps.num / ctx->param.fps.den + 0.5) >> 3;
            t1 = i_period >> 3;

            thd_distance = XEVE_MIN(t0, t1);
            distance = i_period - (ctx->pico->pic_icnt % i_period);

                if (distance < thd_distance)
                {
                    t_d = rcore->cpx_frm / ((ctx->f / rc->param->cpx_thd_resolution) * rc->param->thd_sc * 3);
                    t_d /= (double)(distance);
                    qf *= XEVE_CLIP3(1.0, 2.0, t_d);
                }

            for (i = 1; i < thd_distance; i++)
            {
                pico = ctx->pico_buf[XEVE_MOD_IDX(ctx->pico_idx + i, ctx->pico_max_cnt)];
                if (pico->sinfo.scene_type == SCENE_HIGH)
                {
                    distance = i;
                    break;
                }
            }

            if (distance < thd_distance)
            {
                t_d = rcore->cpx_frm / ((ctx->f / rc->param->cpx_thd_resolution) * rc->param->thd_sc * 3);
                if (pico->sinfo.slice_type == SLICE_P)
                {
                    fcost = (ctx->param.bframes > 0) ?
                            pico->sinfo.uni_est_cost[INTER_UNI2] : pico->sinfo.uni_est_cost[INTER_UNI0];
                }
                else /* SLICE_B */
                {
                    fcost = pico->sinfo.bi_fcost;
                }

                t_d = (double)(rcore->cpx_frm + fcost) / rcore->cpx_frm;
                if (distance != 0)
                {
                    t_d *= (thd_distance - 1) / (distance*1.0);
                }
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

            if (overflow > 1.1 && (int)ctx->pico->pic_icnt >= ctx->param.gop_size)
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

    bit_estimator = (stype != SLICE_B) ? &rc->bit_estimator[stype - 1] : &rc->bit_estimator[SLICE_I + ctx->slice_depth];
    if (bit_estimator->cnt < 1.5)
    {
        return q;
    }

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
    
    for (int i = 0; i < 20; i++)
    {
        /* clip3: if estimated bits are more, increase the qf*/
        buf_ratio = (buf_size >= (5 * rc->bpf)) ? 2 : 1;
        if (rc->scene_cut)
        {
            buf_ratio = 1;
        }
        if (bits > (buf_size - buf_full) / buf_ratio || (buf_ratio != 1 && bits > 0.9 * rc->bpf))
        {
            q_rate = XEVE_CLIP3(1.05, 2.5, (buf_ratio * bits) / (buf_size - buf_full));
            q *= q_rate;
            bits = estimate_frame_bits(bit_estimator, q, rcore->cpx_frm);
        }
    }

    /* clip7:restrict the estiamted bits to 30% of the buf size*/
    if (bits > buf_size * rc->param->max_vbv_rate_frm)
    {
        q *= bits / (buf_size * rc->param->max_vbv_rate_frm);
    }

    /* limit qf from inital qf (model qf) */
    if(ctx->param.rc_type != RC_CRF)
        q = XEVE_CLIP3(q_init / rc->param->qf_limit_vbv, q_init * rc->param->qf_limit_vbv, q);

    rcore->est_bits = estimate_frame_bits(bit_estimator, q, rcore->cpx_frm);
    
    // incase of capped crf return max of init qf and vbv qf
    return ctx->param.rc_type == RC_CRF ? XEVE_MAX(q, q_init) : q;
}

static double get_qf(XEVE_CTX *ctx, XEVE_RCORE *rcore)
{
    XEVE_PICO * pico;
    double      cpx, qf, cpx_rate, target_bits, min_cp, max_cp;
    XEVE_RC   * rc = ctx->rc;
    rc->scene_cut = 0;
    /* compexity rate */
    cpx_rate = (rc->param->blank_sc_cplx_ftr *  (ctx->f / rc->param->cpx_thd_resolution)) / rc->bitrate;

    /* update target bits */
    rc->prev_bpf = rc->bpf;
    if (ctx->pico->sinfo.icnt[0] >= 0.8 * ctx->fcst.f_blk)
    {
        // if 80% of the blocks have less intra cost then considering it as scenecut
        rc->scene_cut = 1;
    }
    rc->bpf = rc->bpf_tid[rc->scene_cut ? 0 : ctx->slice_depth];
    rc->rcm->target_bits += rc->bpf;

    /* target bits */
    target_bits = rc->rcm->target_bits;
    pico = ctx->pico;
    rcore->scene_type = SCENE_NORMAL;

    if (ctx->slice_type == SLICE_I)
    {
        rcore->cpx_frm = pico->sinfo.uni_est_cost[INTRA];
        if (ctx->param.keyint != 1)
        {
            target_bits *= rc->param->intra_rate_ratio;
        }
    }
    else if (ctx->slice_type == SLICE_P)
    {
        rcore->cpx_frm = (ctx->param.bframes > 0) ? pico->sinfo.uni_est_cost[INTER_UNI2] : pico->sinfo.uni_est_cost[INTER_UNI0];
    }
    else /* SLICE_B */
    {
        if (pico->pic_icnt == 1 || (ctx->param.gop_size == 1 && ctx->param.keyint != 1) ) //LD case
        {
            rcore->cpx_frm = ((ctx->param.bframes > 0) ? pico->sinfo.uni_est_cost[INTER_UNI2] : pico->sinfo.uni_est_cost[INTER_UNI0]) / (rc->scene_cut + 1);
        }
        else
        {
            rcore->cpx_frm = pico->sinfo.bi_fcost / (rc->scene_cut + 1);
        }
        if (rc->scene_cut)
            target_bits *= 1.3;
    }
    /* cpx_pow */
    if (rcore->scene_type == SCENE_EX_LOW)
    {
        cpx = rcore->cpx_frm; /* do not update - use just current complexity*/
        rcore->cpx_pow = pow(cpx, rc->param->pow_cplx);
        min_cp = qp_to_qf(rc->param->init_qp - 4.0) * cpx_rate * (target_bits / rc->rcm->k_param);
        max_cp = qp_to_qf(ctx->param.qp_max) * (target_bits / rc->rcm->k_param);
        rcore->cpx_pow = XEVE_CLIP3(min_cp, max_cp, rcore->cpx_pow);
    }
    else
    {
        rc->rcm->cpx_sum = (rc->rcm->cpx_sum * rc->param->df_cplx_sum) + rcore->cpx_frm;
        rc->rcm->cpx_cnt = (rc->rcm->cpx_cnt * rc->param->df_cplx_sum) + 1;
        cpx = rc->rcm->cpx_sum / rc->rcm->cpx_cnt;
        rcore->cpx_pow = pow(cpx, rc->param->pow_cplx);
    }

    if (ctx->param.rc_type == RC_CRF)
    {
        double rf_constant = (ctx->param.crf) + ((rc->scene_cut || ctx->slice_depth == 1) ? 1 : 1.1 * (ctx->slice_depth + 2.0));
        if (rc->encoding_mode == XEVE_LD)
            rf_constant = ctx->param.crf + (rc->scene_cut ? 0 : 3.0);
        double ratefactor = pow(rc->basecplx, 0.4) / qp_to_qf(rf_constant - 3.0);
        qf = rcore->cpx_pow / ratefactor;
    }
    else
    {
        // CBR
        qf = rc->rcm->k_param * (rcore->cpx_pow / target_bits);
    }

    if (rcore->scene_type == SCENE_LOW && qf > qp_to_qf(rc->param->init_qp - 8.0) * cpx_rate)
    {
        qf *= XEVE_CLIP3(0.9, 1.0, qp_to_qf(rc->param->init_qp - 8.0) * cpx_rate / qf);
    }

    if ((rcore->stype == SLICE_I && rcore->scene_type != SCENE_EX_LOW) || (rc->scene_cut))
    {
        rcore->amortize_flag = 1;
    }
    else if (rcore->scene_type == SCENE_HIGH)
    {
        rc->rcm->cpx_sum -= rcore->cpx_frm * 0.4;
        rcore->cpx_pow = pow(rc->rcm->cpx_sum / rc->rcm->cpx_cnt, rc->param->pow_cplx);
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

static void update_rc_model(XEVE_RCORE *rcore, XEVE_RC * rc, s32 bits, s32 max_b_frm, int i_period)
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

        if (eb < (bits * 0.80) || eb > (bits * 1.2))
        {
            rc->rcm->bpf_decayed *= df1;
        }
        else if (eb >= (bits * 0.85) && eb <= (bits * 1.15))
        {
            rc->rcm->bpf_decayed /= df2;
        }

        bpft = (rc->vbv_buf_size > 0) ? rc->bpf / rc->vbv_buf_size : rc->bpf / rc->bitrate;

        rc->rcm->bpf_decayed = XEVE_CLIP3(1.0 - bpft*1.5, 1.0 - bpft*0.1, rc->rcm->bpf_decayed);
    }
    else
    {
        eb = rcore->est_bits;

        if (eb < (bits * 0.65) || eb >(bits * 1.35))
        {
            rc->rcm->bpf_decayed *= df1;
        }
        else if (eb > (bits * 0.75) && eb < (bits * 1.25))
        {
            rc->rcm->bpf_decayed /= df2;
        }

        bpft = (rc->vbv_buf_size > 0) ? rc->bpf / rc->vbv_buf_size : rc->bpf / rc->bitrate;

        rc->rcm->bpf_decayed = XEVE_CLIP3(1.0 - bpft*1.5, 1.0 - bpft*0.1, rc->rcm->bpf_decayed);
    }

    if (rcore->amortize_flag) // currently tested for LD
    {
        int prev_residue_cost = rcore->residue_cost * rcore->amortized_frames;
        rcore->amortized_frames += (i_period != 0 ? i_period : 32); // distributing the bits generated by i-frames over i-period or 32 frames if iperiod not specified.
        rcore->residue_cost = (prev_residue_cost + (int)((bits * 0.85))) / rcore->amortized_frames;
        bits *= 0.15;
        rcore->amortize_flag = 0;
    }
    else if (rcore->amortized_frames > 0)
    {
        bits += rcore->residue_cost;
        rcore->amortized_frames--;
    }
    rc->rcm->k_param += bits * qp_to_qf(rcore->qp) / rcore->cpx_pow;
    

    /* update qp_sum */
    rc->rcm->qp_sum *= rc->param->df_qp_sum;
    rc->rcm->qp_sum += rcore->qp + ((stype == SLICE_I) ? rcore->offset_ip : 0);

    /* update qp_cnt */
    rc->rcm->qp_cnt *= rc->param->df_qp_sum;
    rc->rcm->qp_cnt++;
}

static double get_qfactor(XEVE_CTX *ctx)
{
    XEVE_RCORE * rcore = ctx->rcore;
    XEVE_RC    * rc = ctx->rc;
    double       qf, frm_qf_min, frm_qf_max;

    qf = get_qf(ctx, rcore);
    qf = get_qfactor_clip(ctx, rcore, qf);

    frm_qf_min = rcore->qf_min[rcore->stype];
    frm_qf_max = rcore->qf_max[rcore->stype];

    if (rc->vbv_enabled && rcore->cpx_frm > 0)
    {
        /* clipping  qstep min and max before vbv cliping */
        qf = (frm_qf_min == frm_qf_max) ? frm_qf_min : XEVE_CLIP3(frm_qf_min, frm_qf_max, qf);
        qf = (ctx->param.lookahead > 1 && rc->encoding_mode != XEVE_LD) ? get_vbv_qfactor_fcst(ctx, rcore, rcore->stype, qf) : get_vbv_qfactor(ctx, rcore, rcore->stype, qf);
    }
    qf = (frm_qf_min == frm_qf_max) ? frm_qf_min : XEVE_CLIP3(frm_qf_min, frm_qf_max, qf);
    update_prev_qf(rc, rcore->stype, qf, ctx->pico->pic_icnt);
    return qf;
}


int xeve_rc_get_frame_qp(XEVE_CTX *ctx)
{
    double qp;
    if (ctx->rc->encoding_mode != XEVE_LD)
        ctx->rc->rcm = (ctx->slice_type != SLICE_B) ? &ctx->rc->rc_model[ctx->slice_type - 1] : &ctx->rc->rc_model[SLICE_I + ctx->slice_depth];
    else
        ctx->rc->rcm = &ctx->rc->rc_model[1];

    /* qp from qf */
    qp = qf_to_qp(get_qfactor(ctx));

    /* qp clip */
    qp = XEVE_CLIP3(10, 49, qp);
    qp = XEVE_CLIP3(ctx->param.qp_min, ctx->param.qp_max, qp);
    ctx->rcore->qp = qp;

    return XEVE_CLIP3(RC_QP_MIN, RC_QP_MAX, (int)qp);
}

void xeve_rc_update_frame(XEVE_CTX *ctx, XEVE_RC * rc, XEVE_RCORE * rcore)
{
    s32    stype = rcore->stype;
    double bits = rcore->real_bits;

    if (ctx->param.use_filler) bits -= (rcore->filler_byte << 3);

    rc->frame_bits += (int)bits;

    double current_bitrate;
    rc->total_frames += 1;

    current_bitrate = rc->frame_bits * rc->fps / rc->total_frames;

    if (ctx->param.rc_type != RC_CRF && rc->total_frames > rc->fps / 2)
    {
        if (current_bitrate < rc->bitrate * 0.9)
        {
            for (int i = 0; i < 6; i++)
            {
                rc->bpf_tid[i] *= 1.02;
            }
            ctx->rc->bpf = ctx->rc->bpf_tid[ctx->slice_depth];
            rc->prev_adpt = 1;
        }
        else if (current_bitrate > rc->bitrate * 1.1)
        {
            for (int i = 0; i < 6; i++)
            {
                rc->bpf_tid[i] *= 0.98;
            }
            ctx->rc->bpf = ctx->rc->bpf_tid[ctx->slice_depth];
            rc->prev_adpt = 2;
        }
        else
        {
            if((current_bitrate > rc->bitrate && rc->prev_adpt == 1) ||
               (current_bitrate < rc->bitrate && rc->prev_adpt == 2))
            {
                xeve_set_rc_bpf(ctx);
                rc->prev_adpt = 0;
            }
        }
    }

    if (rcore->scene_type != SCENE_EX_LOW)
    {
        /* update RC model */
        if(ctx->param.rc_type != RC_CRF)
            update_rc_model(rcore, rc, (int)bits, ctx->param.bframes, ctx->param.keyint);

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
        if (ctx->param.rc_type == RC_CRF)
        {
            rc->vbv_buf_fullness += (bits - (rc->vbv_buf_size / rc->fps));
        }
        else 
        {
            rc->vbv_buf_fullness += (bits - rc->bpf);
        }
    }
}

int xeve_rc_get_qp(XEVE_CTX *ctx)
{
    int qp;
    if (ctx->pic_cnt > 0)
    {
        xeve_rc_update_frame(ctx, ctx->rc, ctx->rcore);
    }
    ctx->rcore->stype = ctx->slice_type;
    ctx->rcore->sdepth = ctx->slice_depth;
    qp = xeve_rc_get_frame_qp(ctx);

    return qp;
}
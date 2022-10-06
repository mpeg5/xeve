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

#include <math.h>
#include "xeve_type.h"
#include "xeve_param_parse.h"

XEVE xeve_create(XEVE_CDSC * cdsc, int * err)
{
    XEVE_CTX  * ctx;
    int          ret;

#if ENC_DEC_TRACE
#if TRACE_DBF
    fp_trace = fopen("enc_trace_dbf.txt", "w+");
#else
    fp_trace = fopen("enc_trace.txt", "w+");
#endif
#if TRACE_HLS
    XEVE_TRACE_SET(1);
#endif
#endif
    ctx = NULL;

    /* memory allocation for ctx and core structure */
    ctx = (XEVE_CTX*)xeve_ctx_alloc();

    /* set default value for encoding parameter */
    xeve_mcpy(&ctx->param, &(cdsc->param), sizeof(XEVE_PARAM));
    ret = xeve_set_init_param(ctx, &ctx->param);
    xeve_assert_g(ret == XEVE_OK, ERR);
    xeve_assert_g(ctx->param.profile == XEVE_PROFILE_BASELINE, ERR);

    ret = xeve_platform_init(ctx);
    xeve_assert_g(ret == XEVE_OK, ERR);

    ret = xeve_create_bs_buf(ctx, cdsc->max_bs_buf_size);
    xeve_assert_g(ret == XEVE_OK, ERR);

    xeve_init_err_scale(ctx);
    xeve_set_chroma_qp_tbl_loc(ctx);

    if(ctx->fn_ready != NULL)
    {
        ret = ctx->fn_ready(ctx);
        xeve_assert_g(ret == XEVE_OK, ERR);
    }

    /* set default value for ctx */
    ctx->magic = XEVE_MAGIC_CODE;
    ctx->id = (XEVE)ctx;
    ctx->sh->aps_signaled = -1;

    return (ctx->id);
ERR:
    if(ctx)
    {
        xeve_platform_deinit(ctx);
        xeve_delete_bs_buf(ctx);
        xeve_ctx_free(ctx);
    }
    if(err) *err = ret;
    return NULL;
}

void xeve_delete(XEVE id)
{
    XEVE_CTX * ctx;

    XEVE_ID_TO_CTX_R(id, ctx);

#if ENC_DEC_TRACE
    fclose(fp_trace);
#endif

    if(ctx->fn_flush != NULL)
    {
        ctx->fn_flush(ctx);
    }

    xeve_platform_deinit(ctx);
    xeve_delete_bs_buf(ctx);
    xeve_ctx_free(ctx);
}

int xeve_encode(XEVE id, XEVE_BITB * bitb, XEVE_STAT * stat)
{
    XEVE_CTX * ctx;

    XEVE_ID_TO_CTX_RV(id, ctx, XEVE_ERR_INVALID_ARGUMENT);
    xeve_assert_rv(ctx->fn_enc, XEVE_ERR_UNEXPECTED);

    /* bumping - check whether input pictures are remaining or not in pico_buf[] */
    if(XEVE_OK_NO_MORE_FRM == xeve_check_more_frames(ctx))
    {
        return XEVE_OK_NO_MORE_FRM;
    }
    if (!FORCE_OUT(ctx))
    {
        if (ctx->param.use_fcst)
        {
            xeve_forecast_fixed_gop(ctx);
        }
    }
    /* store input picture and return if needed */
    if(XEVE_OK_OUT_NOT_AVAILABLE == xeve_check_frame_delay(ctx))
    {
        return XEVE_OK_OUT_NOT_AVAILABLE;
    }

    /* update BSB */
    bitb->err = 0;

    return ctx->fn_enc(ctx, bitb, stat);
}

int xeve_push(XEVE id, XEVE_IMGB * img)
{
    XEVE_CTX * ctx;

    XEVE_ID_TO_CTX_RV(id, ctx, XEVE_ERR_INVALID_ARGUMENT);
    xeve_assert_rv(ctx->fn_push, XEVE_ERR_UNEXPECTED);

    return ctx->fn_push(ctx, img);
}

int xeve_config(XEVE id, int cfg, void * buf, int * size)
{
    XEVE_CTX       * ctx;
    int              t0;
    XEVE_IMGB      * imgb;

    XEVE_ID_TO_CTX_RV(id, ctx, XEVE_ERR_INVALID_ARGUMENT);

    switch(cfg)
    {
        /* set config **********************************************************/
        case XEVE_CFG_SET_FORCE_OUT:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.force_output = (t0) ? 1 : 0;
            /* store total input picture count at this time */
            ctx->pic_ticnt = ctx->pic_icnt;
            break;

        case XEVE_CFG_SET_FINTRA:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.f_ifrm = t0;
            break;
        case XEVE_CFG_SET_QP:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            xeve_assert_rv(t0 >= MIN_QUANT && t0 <= MAX_QUANT, \
                           XEVE_ERR_INVALID_ARGUMENT);
            ctx->param.qp = t0;
            break;
        case XEVE_CFG_SET_FPS:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            xeve_assert_rv(t0 > 0, XEVE_ERR_INVALID_ARGUMENT);
            ctx->param.fps = t0;
            break;
        case XEVE_CFG_SET_BPS:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            xeve_assert_rv(t0 > 0, XEVE_ERR_INVALID_ARGUMENT);
            ctx->param.bitrate = t0;
            break;
        case XEVE_CFG_SET_KEYINT:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            xeve_assert_rv(t0 >= 0, XEVE_ERR_INVALID_ARGUMENT);
            ctx->param.keyint = t0;
            break;
        case XEVE_CFG_SET_QP_MIN:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            xeve_assert_rv(t0 >= MIN_QUANT, XEVE_ERR_INVALID_ARGUMENT);
            ctx->param.qp_min = t0;
            break;
        case XEVE_CFG_SET_QP_MAX:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            xeve_assert_rv(t0 <= MAX_QUANT, XEVE_ERR_INVALID_ARGUMENT);
            ctx->param.qp_max = t0;
            break;
        case XEVE_CFG_SET_USE_DEBLOCK:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.use_deblock = t0;
            break;
        case XEVE_CFG_SET_DEBLOCK_A_OFFSET:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.deblock_alpha_offset = t0;
            break;
        case XEVE_CFG_SET_DEBLOCK_B_OFFSET:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            t0 = *((int *)buf);
            ctx->param.deblock_beta_offset = t0;
            break;
        case XEVE_CFG_SET_SEI_CMD:
            ctx->param.sei_cmd_info = (*((int *)buf)) ? 1 : 0;
            break;
        case XEVE_CFG_SET_USE_PIC_SIGNATURE:
            ctx->param.use_pic_sign = (*((int *)buf)) ? 1 : 0;
            break;

            /* get config *******************************************************/
        case XEVE_CFG_GET_QP:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.qp;
            break;
        case XEVE_CFG_GET_WIDTH:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.w;
            break;
        case XEVE_CFG_GET_HEIGHT:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.h;
            break;
        case XEVE_CFG_GET_FPS:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.fps;
            break;
        case XEVE_CFG_GET_BPS:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.bitrate;
            break;
        case XEVE_CFG_GET_KEYINT:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.keyint;
            break;
        case XEVE_CFG_GET_RECON:
            xeve_assert_rv(*size == sizeof(XEVE_IMGB**), XEVE_ERR_INVALID_ARGUMENT);
            imgb = PIC_CURR(ctx)->imgb;

            if (ctx->sps.picture_cropping_flag)
            {
                int end_comp = ctx->sps.chroma_format_idc ? N_C : Y_C;
                for (int i = 0; i < end_comp; i++)
                {
                    int cs_offset = i == Y_C ? 2 : 1;
                    imgb->x[i] = ctx->sps.picture_crop_left_offset * cs_offset;
                    imgb->y[i] = ctx->sps.picture_crop_top_offset * cs_offset;
                    imgb->h[i] = imgb->ah[i] - (ctx->sps.picture_crop_top_offset + ctx->sps.picture_crop_bottom_offset) * cs_offset;
                    imgb->w[i] = imgb->aw[i] - (ctx->sps.picture_crop_left_offset + ctx->sps.picture_crop_right_offset) * cs_offset;
                }
            }

            *((XEVE_IMGB **)buf) = imgb;
            imgb->addref(imgb);
            break;
        case XEVE_CFG_GET_USE_DEBLOCK:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.use_deblock;
            break;
        case XEVE_CFG_GET_CLOSED_GOP:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.closed_gop;
            break;
        case XEVE_CFG_GET_HIERARCHICAL_GOP:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.disable_hgop;
            break;
        case XEVE_CFG_GET_DEBLOCK_A_OFFSET:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.deblock_alpha_offset;
            break;
        case XEVE_CFG_GET_DEBLOCK_B_OFFSET:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = ctx->param.deblock_beta_offset;
            break;
        case XEVE_CFG_GET_SUPPORT_PROF:
            xeve_assert_rv(*size == sizeof(int), XEVE_ERR_INVALID_ARGUMENT);
            *((int *)buf) = XEVE_PROFILE_BASELINE;
            break;
        default:
            xeve_trace("unknown config value (%d)\n", cfg);
            xeve_assert_rv(0, XEVE_ERR_UNSUPPORTED);
    }

    return XEVE_OK;
}

int xeve_param_default(XEVE_PARAM *param)
{
    return xeve_param_init(param);
}

int xeve_param_ppt(XEVE_PARAM* param, int profile, int preset, int tune)
{
    if (preset == XEVE_PRESET_DEFAULT)
    {
        preset = XEVE_PRESET_MEDIUM;
    }
    return xeve_param_apply_ppt_baseline(param, profile, preset, tune);
}

int xeve_param_check(const XEVE_PARAM* param)
{
    int ret = 0;
    int min_block_size = 4;

    // Param check done to avoid main profile toolset inside baseline profile
    if (param->tool_amvr    == 1) { xeve_trace("AMVR cannot be on in base profile\n"); ret = -1; }
    if (param->tool_mmvd    == 1) { xeve_trace("MMVD cannot be on in base profile\n"); ret = -1; }
    if (param->tool_affine  == 1) { xeve_trace("Affine cannot be on in base profile\n"); ret = -1; }
    if (param->tool_dmvr    == 1) { xeve_trace("DMVR cannot be on in base profile\n"); ret = -1; }
    if (param->tool_admvp   == 1) { xeve_trace("ADMVP cannot be on in base profile\n"); ret = -1; }
    if (param->tool_hmvp    == 1) { xeve_trace("HMVP cannot be on in base profile\n"); ret = -1; }
    if (param->tool_addb    == 1) { xeve_trace("ADDB cannot be on in base profile\n"); ret = -1; }
    if (param->tool_alf     == 1) { xeve_trace("ALF cannot be on in base profile\n"); ret = -1; }
    if (param->tool_htdf    == 1) { xeve_trace("HTDF cannot be on in base profile\n"); ret = -1; }
    if (param->btt          == 1) { xeve_trace("BTT cannot be on in base profile\n"); ret = -1; }
    if (param->suco         == 1) { xeve_trace("SUCO cannot be on in base profile\n"); ret = -1; }
    if (param->tool_eipd    == 1) { xeve_trace("EIPD cannot be on in base profile\n"); ret = -1; }
    if (param->tool_iqt     == 1) { xeve_trace("IQT cannot be on in base profile\n"); ret = -1; }
    if (param->tool_cm_init == 1) { xeve_trace("CM_INIT cannot be on in base profile\n"); ret = -1; }
    if (param->tool_adcc    == 1) { xeve_trace("ADCC cannot be on in base profile\n"); ret = -1; }
    if (param->tool_ats     == 1) { xeve_trace("ATS_INTRA cannot be on in base profile\n"); ret = -1; }
    if (param->ibc_flag     == 1) { xeve_trace("IBC cannot be on in base profile\n"); ret = -1; }
    if (param->tool_rpl     == 1) { xeve_trace("RPL cannot be on in base profile\n"); ret = -1; }
    if (param->tool_pocs    == 1) { xeve_trace("POCS cannot be on in base profile\n"); ret = -1; }

    if (XEVE_CS_GET_FORMAT(param->cs) != XEVE_CF_YCBCR400)
    {
        int pic_m = 2;
        if ((param->w & (pic_m - 1)) != 0) { xeve_trace("Current encoder does not support odd picture width\n"); ret = -1; }
        if ((param->h & (pic_m - 1)) != 0) { xeve_trace("Current encoder does not support odd picture height\n"); ret = -1; }
    }

    return ret;
}

int xeve_param_parse(XEVE_PARAM* param, const char* name, const char* value)
{
    if (!param || !name || !value)
    {
        return XEVE_ERR_INVALID_ARGUMENT;
    }

    int ret = xeve_param_set_val(param, name, value);
    return ret;
}


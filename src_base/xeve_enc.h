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

#ifndef _XEVE_ENC_H_
#define _XEVE_ENC_H_

/* Convert XEVE into XEVE_CTX */
#define XEVE_ID_TO_CTX_R(id, ctx) \
    xeve_assert_r((id)); \
    (ctx) = (XEVE_CTX *)id; \
    xeve_assert_r((ctx)->magic == XEVE_MAGIC_CODE);

/* Convert XEVE into XEVE_CTX with return value if assert on */
#define XEVE_ID_TO_CTX_RV(id, ctx, ret) \
    xeve_assert_rv((id), (ret)); \
    (ctx) = (XEVE_CTX *)id; \
    xeve_assert_rv((ctx)->magic == XEVE_MAGIC_CODE, (ret));



XEVE_CTX * xeve_ctx_alloc(void);
void xeve_ctx_free(XEVE_CTX * ctx);
XEVE_CORE * xeve_core_alloc(int chroma_format_idc);
void xeve_core_free(XEVE_CORE * core);

int xeve_pic(XEVE_CTX * ctx, XEVE_BITB * bitb, XEVE_STAT * stat);
int  xeve_platform_init(XEVE_CTX * ctx);
void xeve_platform_deinit(XEVE_CTX * ctx);
int  xeve_pic_prepare(XEVE_CTX * ctx, XEVE_BITB * bitb, XEVE_STAT * stat);
int  xeve_pic_finish(XEVE_CTX * ctx, XEVE_BITB * bitb, XEVE_STAT * stat);
int  xeve_pic(XEVE_CTX * ctx, XEVE_BITB * bitb, XEVE_STAT * stat);
int  xeve_enc(XEVE_CTX * ctx, XEVE_BITB * bitb, XEVE_STAT * stat);
int  xeve_push_frm(XEVE_CTX * ctx, XEVE_IMGB * img);
int  xeve_ready(XEVE_CTX * ctx);
void xeve_flush(XEVE_CTX * ctx);
int  xeve_picbuf_get_inbuf(XEVE_CTX * ctx, XEVE_IMGB ** img);


void xeve_platform_init_func(XEVE_CTX * ctx);
int  xeve_platform_init(XEVE_CTX * ctx);
int  xeve_create_bs_buf(XEVE_CTX  * ctx, int max_bs_buf_size);
int  xeve_delete_bs_buf(XEVE_CTX  * ctx);
int  xeve_encode_sps(XEVE_CTX * ctx);
int  xeve_encode_pps(XEVE_CTX * ctx);
int  xeve_encode_sei(XEVE_CTX * ctx);
int  xeve_check_frame_delay(XEVE_CTX * ctx);
int  xeve_check_more_frames(XEVE_CTX * ctx);

int  xeve_set_init_param(XEVE_CTX * ctx, XEVE_PARAM * param);
int  xeve_pic_finish(XEVE_CTX *ctx, XEVE_BITB *bitb, XEVE_STAT *stat);
void xeve_set_nalu(XEVE_NALU * nalu, int nalu_type, int nuh_temporal_id);
void xeve_set_vui(XEVE_CTX * ctx, XEVE_VUI * vui);
void xeve_set_sps(XEVE_CTX * ctx, XEVE_SPS * sps);
void xeve_set_pps(XEVE_CTX * ctx, XEVE_PPS * pps);
int  xeve_set_active_pps_info(XEVE_CTX * ctx);
void xeve_set_sh(XEVE_CTX *ctx, XEVE_SH *sh);
int  xeve_set_tile_info(XEVE_CTX * ctx);
int  xeve_header(XEVE_CTX * ctx);

int  xeve_init_core_mt(XEVE_CTX * ctx, int tile_num, XEVE_CORE * core, int thread_cnt);
int  xeve_deblock_mt(void * arg);
int  xeve_loop_filter(XEVE_CTX * ctx, XEVE_CORE * core);
void xeve_recon(XEVE_CTX * ctx, XEVE_CORE * core, s16 *coef, pel *pred, int is_coef, int cuw, int cuh, int s_rec, pel *rec, int bit_depth);

int  xeve_param_apply_ppt_baseline(XEVE_PARAM* param, int profile, int preset, int tune);
int  xeve_param_init(XEVE_PARAM* param);

void xeve_param2string(XEVE_PARAM * param, char * sei_buf, int padx, int pady);
#endif /* _XEVE_ENC_H_ */


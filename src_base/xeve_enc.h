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
int  xeve_check_frame_delay(XEVE_CTX * ctx);
int  xeve_check_more_frames(XEVE_CTX * ctx);

int  xeve_param_apply_ppt_baseline(XEVE_PARAM* param, int profile, int preset, int tune);
int  xeve_param_init(XEVE_PARAM* param);


#endif /* _XEVE_ENC_H_ */


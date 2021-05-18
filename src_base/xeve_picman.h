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

#ifndef _XEVE_PICMAN_H_
#define _XEVE_PICMAN_H_

/* macros for reference picture flag */
#define IS_REF(pic)          (((pic)->is_ref) != 0)
#define SET_REF_UNMARK(pic)  (((pic)->is_ref) = 0)
#define SET_REF_MARK(pic)    (((pic)->is_ref) = 1)
#define PRINT_DPB(pm)\
    xeve_print("%s: current num_ref = %d, dpb_size = %d\n", __FUNCTION__, \
    pm->cur_num_ref_pics, picman_get_num_allocated_pics(pm));

 /*Declaration for ref pic marking and ref pic list construction functions */
int xeve_picman_refp_init(XEVE_PM *pm, int max_num_ref_pics, int slice_type, u32 poc, u8 layer_id, int last_intra, XEVE_REFP (*refp)[REFP_NUM]);
void xeve_picman_update_pic_ref(XEVE_PM * pm);
XEVE_PIC * xeve_picman_get_empty_pic(XEVE_PM *pm, int *err);
int xeve_picman_put_pic(XEVE_PM *pm, XEVE_PIC *pic, int is_idr, u32 poc, u8 layer_id, int need_for_output, XEVE_REFP (*refp)[REFP_NUM], int ref_pic, int pnpf, int ref_pic_gap_length);
XEVE_PIC * xeve_picman_out_pic(XEVE_PM *pm, int *err);
int xeve_picman_deinit(XEVE_PM *pm);
int xeve_picman_init(XEVE_PM *pm, int max_pb_size, int max_num_ref_pics, PICBUF_ALLOCATOR *pa);
void xeve_set_refp(XEVE_REFP * refp, XEVE_PIC  * pic_ref);
int xeve_picman_move_pic(XEVE_PM *pm, int from, int to);

#endif /* _XEVE_PICMAN_H_ */

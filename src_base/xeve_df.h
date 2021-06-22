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

#ifndef _XEVE_DF_H_
#define _XEVE_DF_H_

int  xeve_deblock(XEVE_CTX * ctx, XEVE_PIC * pic, int tile_idx, int filter_across_boundary, XEVE_CORE * core);
void xeve_deblock_unit(XEVE_CTX * ctx, XEVE_PIC * pic, int x, int y, int cuw, int cuh, int is_hor_edge, XEVE_CORE * core, int boundary_filtering);
void xeve_deblock_cu_hor(XEVE_PIC *pic, int x_pel, int y_pel, int cuw, int cuh, u32 *map_scu, s8 (*map_refi)[REFP_NUM], s16 (*map_mv)[REFP_NUM][MV_D]
                         , int w_scu, TREE_CONS tree_cons, u8* map_tidx, int boundary_filtering
                         , int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc, int* qp_chroma_dynamic[2]);
void xeve_deblock_cu_ver(XEVE_PIC *pic, int x_pel, int y_pel, int cuw, int cuh, u32 *map_scu, s8 (*map_refi)[REFP_NUM], s16 (*map_mv)[REFP_NUM][MV_D]
                         , int w_scu, u32 *map_cu, TREE_CONS tree_cons, u8 *map_tidx, int boundary_filtering
                         , int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc, int* qp_chroma_dynamic[2]);

void xeve_deblock_tree(XEVE_CTX * ctx, XEVE_PIC * pic, int x, int y, int cuw, int cuh, int cud, int cup, int is_hor_edge
                     , TREE_CONS tree_cons, XEVE_CORE * core, int boundary_filtering);
#endif /* _XEVE_DF_H_ */

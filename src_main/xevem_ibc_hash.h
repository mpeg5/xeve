/* The copyright in this software is being made available under the BSD
   License, included below. This software may be subject to contributor and
   other third party rights, including patent rights, and no such rights are
   granted under this license.

   Copyright (c) 2020, Samsung Electronics Co., Ltd.
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

#ifndef __XEVE_IBC_HASH__
#define __XEVE_IBC_HASH__

#include <stdint.h>
#include "xeve_def.h"
#include "xevem_type.h"

typedef struct _POSITION
{
    int x, y;
}POSITION;

typedef struct _POS_NODE
{
    struct _POS_NODE * next;
    u32 key;
    u16 x, y;
}POS_NODE;

typedef struct _HASH_KEY_NODE
{
    u32 key;
    u32 size;
    POS_NODE * pos;
    POS_NODE * pos_end;
    struct _HASH_KEY_NODE * next;
}HASH_KEY_NODE;

struct _XEVE_IBC_HASH
{
    int     pic_width;
    int     pic_height;
    int     search_range_4small_blk;
    u32     hash_table_size;
    u32     max_hash_cand;
    u32     cand_num;

    POS_NODE     ** map_pos_to_hash;
    HASH_KEY_NODE * map_hash_to_pos;
    u8            * map_hash_to_pos_used;
    POS_NODE      * cand_pos;
};

XEVE_IBC_HASH * xeve_ibc_hash_create(XEVE_CTX * ctx, int pic_width, int pic_height);
int               xeve_ibc_hash_init(XEVE_CTX * ctx, XEVE_IBC_HASH * ibc_hash, const int pic_width, const int pic_height);
void              xeve_ibc_hash_destroy(XEVE_IBC_HASH * ibc_hash);
void              xeve_ibc_hash_clear(XEVE_IBC_HASH * ibc_hash);
void              xeve_ibc_hash_insert(XEVE_IBC_HASH * ibc_hash, u32 key, u16 x, u16 y);
void              xeve_ibc_hash_rebuild(XEVE_IBC_HASH * ibc_hash, const XEVE_PIC* pic);
void              xeve_ibc_hash_build(XEVE_IBC_HASH * ibc_hash, const XEVE_PIC* pic);
BOOL              xeve_ibc_hash_match(XEVE_CTX *ctx, XEVE_IBC_HASH * ibc_hash, int cu_x, int cu_y, int log2_cuw, int log2_cuh);
u32               xeve_ibc_hash_search(XEVE_CTX *ctx, XEVE_IBC_HASH* p, int cu_x, int cu_y, int log2_cuw, int log2_cuh, s16 mvp[MV_D], s16 mv[MV_D], XEVE_CORE * core);
int               xeve_ibc_hash_hit_ratio(XEVE_CTX* ctx, XEVE_IBC_HASH* p, int cu_x, int cu_y, int log2_cuw, int log2_cuh);
HASH_KEY_NODE  *  xeve_ibc_hash_get_key_node(XEVE_IBC_HASH * ibc_hash, u32 key);
u32               xeve_ibc_hash_calc_block_key(const pel* pel, const int stride, const int width, const int height, unsigned int crc);
u32               xeve_ibc_hash_crc32_16bit(u32 crc, const pel pel);

#endif // __XEVE_IBC_HASH__

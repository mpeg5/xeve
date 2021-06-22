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

#include "xevem_ibc_hash.h"
#include "xeve_pred.h"

XEVE_IBC_HASH * xeve_ibc_hash_create(XEVE_CTX * ctx, int pic_width, int pic_height)
{
    XEVE_IBC_HASH * ibc_hash = (XEVE_IBC_HASH*)xeve_malloc(sizeof(XEVE_IBC_HASH));
    xeve_ibc_hash_init(ctx, ibc_hash, pic_width, pic_height);

    return (XEVE_IBC_HASH *)ibc_hash;
}

int xeve_ibc_hash_init(XEVE_CTX * ctx, XEVE_IBC_HASH * ibc_hash, const int pic_width, const int pic_height)
{
    int ret;
    ibc_hash->search_range_4small_blk = ctx->param.ibc_hash_search_range_4smallblk;

    ibc_hash->max_hash_cand = ctx->param.ibc_hash_search_max_cand;
    ibc_hash->cand_pos = (POS_NODE*)xeve_malloc(sizeof(POS_NODE) * ibc_hash->max_hash_cand);

    ibc_hash->pic_width = pic_width;
    ibc_hash->pic_height = pic_height;

    ibc_hash->map_pos_to_hash = (POS_NODE**)xeve_malloc(sizeof(POS_NODE*) * pic_height);
    ibc_hash->map_pos_to_hash[0] = (POS_NODE*)xeve_malloc(sizeof(POS_NODE) * pic_width  *  pic_height);
    xeve_assert_gv(ibc_hash->map_pos_to_hash[0], ret, XEVE_ERR_OUT_OF_MEMORY, ERR);
    xeve_mset(ibc_hash->map_pos_to_hash[0], 0, sizeof(POS_NODE) * pic_width  *  pic_height);
    for (int n = 1; n < pic_height; n++)
    {
        ibc_hash->map_pos_to_hash[n] = ibc_hash->map_pos_to_hash[n - 1] + pic_width;
    }

    ibc_hash->hash_table_size = pic_width * pic_height;//1 << 16;
    ibc_hash->map_hash_to_pos = (HASH_KEY_NODE *)xeve_malloc(sizeof(HASH_KEY_NODE) * ibc_hash->hash_table_size);
    xeve_mset(ibc_hash->map_hash_to_pos, 0, sizeof(HASH_KEY_NODE) * ibc_hash->hash_table_size);
    ibc_hash->map_hash_to_pos_used = (u8 *)xeve_malloc(sizeof(u8) * ibc_hash->hash_table_size);
    xeve_mset(ibc_hash->map_hash_to_pos_used, 0, sizeof(u8) * ibc_hash->hash_table_size);

    return XEVE_OK;
ERR:
    return XEVE_ERR;
}

void xeve_ibc_hash_destroy(XEVE_IBC_HASH * ibc_hash)
{
    if (ibc_hash->map_pos_to_hash != NULL)
    {
        if (ibc_hash->map_pos_to_hash[0] != NULL)
        {
            xeve_mfree(ibc_hash->map_pos_to_hash[0]);
        }
        xeve_mfree(ibc_hash->map_pos_to_hash);
    }
    ibc_hash->map_pos_to_hash = NULL;

    if (ibc_hash->map_hash_to_pos != NULL)
    {
        HASH_KEY_NODE * tmp, *cur;

        for (u32 i = 0; i < ibc_hash->hash_table_size; i++)
        {
            tmp = ibc_hash->map_hash_to_pos[i].next;
            while (tmp != NULL)
            {
                cur = tmp;
                tmp = tmp->next;
                xeve_mfree(cur);
            }
        }
        xeve_mfree(ibc_hash->map_hash_to_pos);
    }

    if (ibc_hash->map_hash_to_pos_used != NULL)
    {
        xeve_mfree(ibc_hash->map_hash_to_pos_used);
    }

    xeve_mfree(ibc_hash);
}

void xeve_ibc_hash_clear(XEVE_IBC_HASH * ibc_hash)
{
    xeve_mset(ibc_hash->map_pos_to_hash[0], 0, sizeof(POS_NODE) * ibc_hash->pic_width * ibc_hash->pic_height);
    if (ibc_hash->map_hash_to_pos != NULL)
    {
        HASH_KEY_NODE * tmp, *cur;

        for (u32 i = 0; i < ibc_hash->hash_table_size; i++)
        {
            tmp = ibc_hash->map_hash_to_pos[i].next;
            while (tmp != NULL)
            {
                cur = tmp;
                tmp = tmp->next;
                xeve_mfree(cur);
            }
        }
    }
    xeve_mset(ibc_hash->map_hash_to_pos, 0, sizeof(HASH_KEY_NODE) * ibc_hash->hash_table_size);
    xeve_mset(ibc_hash->map_hash_to_pos_used, 0, sizeof(u8) * ibc_hash->hash_table_size);
}

void xeve_ibc_hash_insert(XEVE_IBC_HASH * ibc_hash, u32 key, u16 x, u16 y)
{
    HASH_KEY_NODE ** tmp_key_node = NULL;
    u32 tmp_key = key % ibc_hash->hash_table_size;

    HASH_KEY_NODE * tmp_node = ibc_hash->map_hash_to_pos + tmp_key;
    tmp_key_node = &tmp_node;

    if (ibc_hash->map_hash_to_pos_used[tmp_key])
    {
        while (*tmp_key_node != NULL && (*tmp_key_node)->key != key)
        {
            tmp_key_node = &(*tmp_key_node)->next;
        }

        if (*tmp_key_node == NULL)
        {
            *tmp_key_node = (HASH_KEY_NODE*)xeve_malloc(sizeof(HASH_KEY_NODE));
            (*tmp_key_node)->key = key;
            (*tmp_key_node)->next = NULL;
            (*tmp_key_node)->pos = NULL;
            (*tmp_key_node)->size = 0;
        }
    }
    else
    {
        ibc_hash->map_hash_to_pos_used[tmp_key] = 1;
        (*tmp_key_node)->key = key;
    }

    (*tmp_key_node)->size++;

    POS_NODE ** tmp_pos_node = &(*tmp_key_node)->pos;
    if ((*tmp_key_node)->pos == NULL)
    {
        (*tmp_key_node)->pos = &ibc_hash->map_pos_to_hash[y][x];
        (*tmp_key_node)->pos_end = &ibc_hash->map_pos_to_hash[y][x];
    }
    else
    {
        (*tmp_key_node)->pos_end->next = &ibc_hash->map_pos_to_hash[y][x];
        (*tmp_key_node)->pos_end = (*tmp_key_node)->pos_end->next;
    }
}

void xeve_ibc_hash_build(XEVE_IBC_HASH * ibc_hash, const XEVE_PIC* pic)
{
    int y_stride = 0;
    int c_stride = 0;
    int width = 0;
    int height = 0;
    const int chroma_scaling_x = 1;
    const int chroma_scaling_y = 1;
    const int chroma_min_w = MIN_CU_SIZE >> chroma_scaling_x;
    const int chroma_min_h = MIN_CU_SIZE >> chroma_scaling_y;
    const pel* pic_y = NULL;
    const pel* pic_u = NULL;
    const pel* pic_v = NULL;

    width = pic->w_l;
    height = pic->h_l;

    y_stride = pic->s_l;
    c_stride = pic->s_c;

    POSITION pos;
    for (pos.y = 0; pos.y + MIN_CU_SIZE <= height; pos.y++)
    {
        // row pointer
        pic_y = pic->y + pos.y * y_stride;

        int chroma_y = pos.y >> chroma_scaling_y;
        pic_u = pic->u + chroma_y * c_stride;
        pic_v = pic->v + chroma_y * c_stride;

        for (pos.x = 0; pos.x + MIN_CU_SIZE <= width; pos.x++)
        {
            // 0x1FF is just an initial value
            unsigned int hash_value = 0x1FF;

            // luma part
            hash_value = xeve_ibc_hash_calc_block_key(&pic_y[pos.x], y_stride, MIN_CU_SIZE, MIN_CU_SIZE, hash_value);

            // chroma part
            int chroma_x = pos.x >> chroma_scaling_x;
            hash_value = xeve_ibc_hash_calc_block_key(&pic_u[chroma_x], c_stride, chroma_min_w, chroma_min_h, hash_value);
            hash_value = xeve_ibc_hash_calc_block_key(&pic_v[chroma_x], c_stride, chroma_min_w, chroma_min_h, hash_value);

            // hash table
            ibc_hash->map_pos_to_hash[pos.y][pos.x].key = hash_value;
            ibc_hash->map_pos_to_hash[pos.y][pos.x].x = pos.x;
            ibc_hash->map_pos_to_hash[pos.y][pos.x].y = pos.y;
            xeve_ibc_hash_insert(ibc_hash, hash_value, pos.x, pos.y);
        }
    }
}

void xeve_ibc_hash_rebuild(XEVE_IBC_HASH * ibc_hash, const XEVE_PIC* pic)
{
    xeve_ibc_hash_clear(ibc_hash);
    xeve_ibc_hash_build(ibc_hash, pic);
}

BOOL xeve_ibc_hash_match(XEVE_CTX *ctx, XEVE_IBC_HASH * ibc_hash, int cu_x, int cu_y, int log2_cuw, int log2_cuh)
{
    int cuw = (1 << log2_cuw);
    int cuh = (1 << log2_cuh);

    u32 target_block_hash = ibc_hash->map_pos_to_hash[cu_y][cu_x].key;
    HASH_KEY_NODE * temp_key_node = xeve_ibc_hash_get_key_node(ibc_hash, target_block_hash);

    ibc_hash->cand_num = 0;
    xeve_mset(ibc_hash->cand_pos, 0, sizeof(POS_NODE) * ibc_hash->max_hash_cand);

    if (temp_key_node->size > 1)
    {
        POS_NODE * temp_pos_node = temp_key_node->pos;

        while (temp_pos_node)
        {
            int offset_BR_x = temp_pos_node->x + cuw - 1;
            int offset_BR_y = temp_pos_node->y + cuh - 1;
            int offset_x_scu = PEL2SCU(offset_BR_x);
            int offset_y_scu = PEL2SCU(offset_BR_y);
            int offset_scup = (offset_y_scu * ctx->w_scu) + offset_x_scu;

            int avail_cu = MCU_GET_COD(ctx->map_scu[offset_scup]);

            BOOL whole_block_match = TRUE;
            if (cuw > MIN_CU_SIZE || cuh > MIN_CU_SIZE)
            {
                if (!avail_cu || offset_BR_x >= ibc_hash->pic_width || offset_BR_y >= ibc_hash->pic_height)
                {
                    temp_pos_node = temp_pos_node->next;
                    continue;
                }
                for (int y = 0; y < cuh && whole_block_match; y += MIN_CU_SIZE)
                {
                    for (int x = 0; x < cuw && whole_block_match; x += MIN_CU_SIZE)
                    {
                        whole_block_match &= (ibc_hash->map_pos_to_hash[cu_y + y][cu_x + x].key == ibc_hash->map_pos_to_hash[temp_pos_node->y + y][temp_pos_node->x + x].key);
                    }
                }
            }
            else
            {
                if (abs(temp_pos_node->x - cu_x) > ibc_hash->search_range_4small_blk || abs(temp_pos_node->y - cu_y) > ibc_hash->search_range_4small_blk || !avail_cu)
                {
                    temp_pos_node = temp_pos_node->next;
                    continue;
                }
            }
            if (whole_block_match)
            {
                ibc_hash->cand_pos[ibc_hash->cand_num].x = temp_pos_node->x;
                ibc_hash->cand_pos[ibc_hash->cand_num].y = temp_pos_node->y;
                ibc_hash->cand_num++;
                if (ibc_hash->cand_num > ibc_hash->max_hash_cand)
                {
                    break;
                }
            }
            temp_pos_node = temp_pos_node->next;
        }
    }

    return ibc_hash->cand_num > 0;
}

u32 xeve_ibc_hash_search(XEVE_CTX *ctx, XEVE_IBC_HASH* p, int cu_x, int cu_y, int log2_cuw
                         , int log2_cuh, s16 mvp[MV_D], s16 mv[MV_D], XEVE_CORE * core)
{
    u32 cost = 0;
    u32 min_cost = XEVE_UINT32_MAX;

    XEVEM_CTX * mctx = (XEVEM_CTX *)ctx;
    XEVE_PIBC *pi = &mctx->pibc[core->thread_cnt];
    XEVE_IBC_HASH* ibc_hash = (XEVE_IBC_HASH*)p;

    mvp[MV_X] = 0;
    mvp[MV_Y] = 0;

    mv[MV_X] = 0;
    mv[MV_Y] = 0;

    if (xeve_ibc_hash_match(ctx, ibc_hash, cu_x, cu_y, log2_cuw, log2_cuh))
    {
        const u32 max_cu_w = (1 << ctx->log2_max_cuwh);
        const int pic_width = ctx->w;
        const int pic_height = ctx->h;
        int roi_width = (1 << log2_cuw);
        int roi_height = (1 << log2_cuh);

        for (u32 idx = 0; idx < ibc_hash->max_hash_cand; idx++)
        {
            int ref_pos_LT_x_scu = PEL2SCU(ibc_hash->cand_pos[idx].x);
            int ref_pos_LT_y_scu = PEL2SCU(ibc_hash->cand_pos[idx].y);
            int ref_pos_LT_scup = (ref_pos_LT_y_scu * ctx->w_scu) + ref_pos_LT_x_scu;

            int avail_LT_cu = MCU_GET_COD(ctx->map_scu[ref_pos_LT_scup]);

            int ref_bottom_right_x = ibc_hash->cand_pos[idx].x + roi_width - 1;
            int ref_bottom_right_y = ibc_hash->cand_pos[idx].y + roi_height - 1;

            int ref_pos_BR_x_scu = PEL2SCU(ref_bottom_right_x);
            int ref_pos_BR_y_scu = PEL2SCU(ref_bottom_right_y);
            int ref_pos_BR_scup = (ref_pos_BR_y_scu * ctx->w_scu) + ref_pos_BR_x_scu;

            int avail_BR_cu = MCU_GET_COD(ctx->map_scu[ref_pos_BR_scup]);

            if (avail_LT_cu && avail_BR_cu)
            {
                s16 cand_mv[MV_D];
                cand_mv[MV_X] = ibc_hash->cand_pos[idx].x - cu_x;
                cand_mv[MV_Y] = ibc_hash->cand_pos[idx].y - cu_y;

                if (!is_bv_valid(ctx, cu_x, cu_y, roi_width, roi_height, log2_cuw, log2_cuh, pic_width, pic_height, cand_mv[0], cand_mv[1], max_cu_w, core))
                {
                    continue;
                }

                int mv_bits = get_bv_cost_bits(cand_mv[MV_X], cand_mv[MV_Y]);
                cost = GET_BV_COST(ctx, mv_bits);

                if (cost < min_cost)
                {
                    mv[0] = cand_mv[0];
                    mv[1] = cand_mv[1];
                    min_cost = cost;
                }
            }
        }
    }

    return min_cost;
}

int xeve_ibc_hash_hit_ratio(XEVE_CTX * ctx, XEVE_IBC_HASH * ibc_hash, int cu_x, int cu_y, int log2_cuw, int log2_cuh)
{
    HASH_KEY_NODE * temp_key_node;
    int pic_width = ctx->w;
    int pic_height = ctx->h;
    int roi_width = (1 << log2_cuw);
    int roi_height = (1 << log2_cuh);    
    int max_x = XEVE_MIN((int)(cu_x + roi_width), pic_width);
    int max_y = XEVE_MIN((int)(cu_y + roi_height), pic_height);
    int hit = 0, total = 0;

    for (int y = cu_y; y < max_y; y += MIN_CU_SIZE)
    {
        for (int x = cu_x; x < max_x; x += MIN_CU_SIZE)
        {
            const u32 hash = ibc_hash->map_pos_to_hash[y][x].key;
            temp_key_node = xeve_ibc_hash_get_key_node(ibc_hash, hash);
            hit += (temp_key_node->size > 1);
            total++;
        }
    }
    if (total)
    return 100 * hit / total;
    else
        return 0;
}

static const u32 crc32_table[256] = {
    0x00000000L, 0xF26B8303L, 0xE13B70F7L, 0x1350F3F4L,
    0xC79A971FL, 0x35F1141CL, 0x26A1E7E8L, 0xD4CA64EBL,
    0x8AD958CFL, 0x78B2DBCCL, 0x6BE22838L, 0x9989AB3BL,
    0x4D43CFD0L, 0xBF284CD3L, 0xAC78BF27L, 0x5E133C24L,
    0x105EC76FL, 0xE235446CL, 0xF165B798L, 0x030E349BL,
    0xD7C45070L, 0x25AFD373L, 0x36FF2087L, 0xC494A384L,
    0x9A879FA0L, 0x68EC1CA3L, 0x7BBCEF57L, 0x89D76C54L,
    0x5D1D08BFL, 0xAF768BBCL, 0xBC267848L, 0x4E4DFB4BL,
    0x20BD8EDEL, 0xD2D60DDDL, 0xC186FE29L, 0x33ED7D2AL,
    0xE72719C1L, 0x154C9AC2L, 0x061C6936L, 0xF477EA35L,
    0xAA64D611L, 0x580F5512L, 0x4B5FA6E6L, 0xB93425E5L,
    0x6DFE410EL, 0x9F95C20DL, 0x8CC531F9L, 0x7EAEB2FAL,
    0x30E349B1L, 0xC288CAB2L, 0xD1D83946L, 0x23B3BA45L,
    0xF779DEAEL, 0x05125DADL, 0x1642AE59L, 0xE4292D5AL,
    0xBA3A117EL, 0x4851927DL, 0x5B016189L, 0xA96AE28AL,
    0x7DA08661L, 0x8FCB0562L, 0x9C9BF696L, 0x6EF07595L,
    0x417B1DBCL, 0xB3109EBFL, 0xA0406D4BL, 0x522BEE48L,
    0x86E18AA3L, 0x748A09A0L, 0x67DAFA54L, 0x95B17957L,
    0xCBA24573L, 0x39C9C670L, 0x2A993584L, 0xD8F2B687L,
    0x0C38D26CL, 0xFE53516FL, 0xED03A29BL, 0x1F682198L,
    0x5125DAD3L, 0xA34E59D0L, 0xB01EAA24L, 0x42752927L,
    0x96BF4DCCL, 0x64D4CECFL, 0x77843D3BL, 0x85EFBE38L,
    0xDBFC821CL, 0x2997011FL, 0x3AC7F2EBL, 0xC8AC71E8L,
    0x1C661503L, 0xEE0D9600L, 0xFD5D65F4L, 0x0F36E6F7L,
    0x61C69362L, 0x93AD1061L, 0x80FDE395L, 0x72966096L,
    0xA65C047DL, 0x5437877EL, 0x4767748AL, 0xB50CF789L,
    0xEB1FCBADL, 0x197448AEL, 0x0A24BB5AL, 0xF84F3859L,
    0x2C855CB2L, 0xDEEEDFB1L, 0xCDBE2C45L, 0x3FD5AF46L,
    0x7198540DL, 0x83F3D70EL, 0x90A324FAL, 0x62C8A7F9L,
    0xB602C312L, 0x44694011L, 0x5739B3E5L, 0xA55230E6L,
    0xFB410CC2L, 0x092A8FC1L, 0x1A7A7C35L, 0xE811FF36L,
    0x3CDB9BDDL, 0xCEB018DEL, 0xDDE0EB2AL, 0x2F8B6829L,
    0x82F63B78L, 0x709DB87BL, 0x63CD4B8FL, 0x91A6C88CL,
    0x456CAC67L, 0xB7072F64L, 0xA457DC90L, 0x563C5F93L,
    0x082F63B7L, 0xFA44E0B4L, 0xE9141340L, 0x1B7F9043L,
    0xCFB5F4A8L, 0x3DDE77ABL, 0x2E8E845FL, 0xDCE5075CL,
    0x92A8FC17L, 0x60C37F14L, 0x73938CE0L, 0x81F80FE3L,
    0x55326B08L, 0xA759E80BL, 0xB4091BFFL, 0x466298FCL,
    0x1871A4D8L, 0xEA1A27DBL, 0xF94AD42FL, 0x0B21572CL,
    0xDFEB33C7L, 0x2D80B0C4L, 0x3ED04330L, 0xCCBBC033L,
    0xA24BB5A6L, 0x502036A5L, 0x4370C551L, 0xB11B4652L,
    0x65D122B9L, 0x97BAA1BAL, 0x84EA524EL, 0x7681D14DL,
    0x2892ED69L, 0xDAF96E6AL, 0xC9A99D9EL, 0x3BC21E9DL,
    0xEF087A76L, 0x1D63F975L, 0x0E330A81L, 0xFC588982L,
    0xB21572C9L, 0x407EF1CAL, 0x532E023EL, 0xA145813DL,
    0x758FE5D6L, 0x87E466D5L, 0x94B49521L, 0x66DF1622L,
    0x38CC2A06L, 0xCAA7A905L, 0xD9F75AF1L, 0x2B9CD9F2L,
    0xFF56BD19L, 0x0D3D3E1AL, 0x1E6DCDEEL, 0xEC064EEDL,
    0xC38D26C4L, 0x31E6A5C7L, 0x22B65633L, 0xD0DDD530L,
    0x0417B1DBL, 0xF67C32D8L, 0xE52CC12CL, 0x1747422FL,
    0x49547E0BL, 0xBB3FFD08L, 0xA86F0EFCL, 0x5A048DFFL,
    0x8ECEE914L, 0x7CA56A17L, 0x6FF599E3L, 0x9D9E1AE0L,
    0xD3D3E1ABL, 0x21B862A8L, 0x32E8915CL, 0xC083125FL,
    0x144976B4L, 0xE622F5B7L, 0xF5720643L, 0x07198540L,
    0x590AB964L, 0xAB613A67L, 0xB831C993L, 0x4A5A4A90L,
    0x9E902E7BL, 0x6CFBAD78L, 0x7FAB5E8CL, 0x8DC0DD8FL,
    0xE330A81AL, 0x115B2B19L, 0x020BD8EDL, 0xF0605BEEL,
    0x24AA3F05L, 0xD6C1BC06L, 0xC5914FF2L, 0x37FACCF1L,
    0x69E9F0D5L, 0x9B8273D6L, 0x88D28022L, 0x7AB90321L,
    0xAE7367CAL, 0x5C18E4C9L, 0x4F48173DL, 0xBD23943EL,
    0xF36E6F75L, 0x0105EC76L, 0x12551F82L, 0xE03E9C81L,
    0x34F4F86AL, 0xC69F7B69L, 0xD5CF889DL, 0x27A40B9EL,
    0x79B737BAL, 0x8BDCB4B9L, 0x988C474DL, 0x6AE7C44EL,
    0xBE2DA0A5L, 0x4C4623A6L, 0x5F16D052L, 0xAD7D5351L
};

u32 xeve_ibc_hash_crc32_16bit(u32 crc, const pel pel)
{
    const void *buf = &pel;
    const u8 *p = (const u8 *)buf;
    u8 size = 2;

    while (size--)
    {
        crc = crc32_table[(crc ^ *p++) & 0xff] ^ (crc >> 8);
    }

    return crc;
}

unsigned int xeve_ibc_hash_calc_block_key(const pel* pel, const int stride, const int width, const int height, unsigned int crc)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            crc = xeve_ibc_hash_crc32_16bit(crc, pel[x]);
        }
        pel += stride;
    }
    return crc;
}

HASH_KEY_NODE * xeve_ibc_hash_get_key_node(XEVE_IBC_HASH * ibc_hash, u32 key)
{
    u32 tmp_key = key % ibc_hash->hash_table_size;
    HASH_KEY_NODE * tmp_key_node = &ibc_hash->map_hash_to_pos[tmp_key];
    while (tmp_key_node->key != key)
    {
        tmp_key_node = tmp_key_node->next;
    }

    return tmp_key_node;
}

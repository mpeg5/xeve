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

#include "xevem_picman.h"


const XEVE_RPL pre_define_rpls[2][4][2][XEVE_MAX_NUM_RPLS] =
{
    /* Disable redordering (LD) */
    {
        /* GOP 1 */
        {
            /* L0 */
            {
                { 1, 0, 5, 5, {1, 2, 3, 4, 5}, 'B' },
                { 1, 0, 1, 1, {1,}, 'B' },
                { 2, 0, 2, 2, {1, 2,}, 'B' },
                { 3, 0, 3, 3, {1, 2, 3}, 'B' },
                { 4, 0, 4, 4, {1, 2, 3, 4}, 'B' },
                { 5, 0, 5, 5, {1, 2, 3, 4, 5}, 'B' },
            },
            /* L0 */
            {
                { 1, 0, 5, 5, {1, 2, 3, 4, 5}, 'B' },
                { 1, 0, 1, 1, {1,}, 'B' },
                { 2, 0, 2, 2, {1, 2,}, 'B' },
                { 3, 0, 3, 3, {1, 2, 3}, 'B' },
                { 4, 0, 4, 4, {1, 2, 3, 4}, 'B' },
                { 5, 0, 5, 5, {1, 2, 3, 4, 5}, 'B' },
            },
        },
        /* GOP 2 */
        {
            /* L0 */
            {
                { 1, 0, 2, 2, {1, 3}, 'B' },
                { 2, 0, 2, 2, {1, 2}, 'B' },
                { 1, 0, 1, 1, {1,}, 'B' },
                { 2, 0, 2, 2, {1, 2,}, 'B' },
            },
            /* L0 */
            {
                { 1, 0, 2, 2, {1, 3}, 'B' },
                { 2, 0, 2, 2, {1, 2}, 'B' },
                { 1, 0, 1, 1, {1,}, 'B' },
                { 2, 0, 2, 2, {1, 2,}, 'B' },
            },
        },
        /* GOP 4 */
        {
            /* L0 */
            {
                { 1, 0, 4, 4, {1, 5, 9, 13}, 'B' },
                { 2, 0, 4, 4, {1, 2, 6, 10}, 'B' },
                { 3, 0, 4, 4, {1, 3, 7, 11}, 'B' },
                { 4, 0, 4, 4, {1, 4, 8, 12}, 'B' },
                { 1, 0, 1, 1, {1,}, 'B' },
                { 2, 0, 2, 2, {1, 2,}, 'B' },
                { 3, 0, 3, 3, {1, 2, 3,}, 'B' },
                { 4, 0, 4, 4, {1, 2, 3, 4}, 'B' },
                { 1, 0, 4, 4, {1, 2, 3, 5}, 'B' },
                { 2, 0, 4, 4, {1, 2, 3, 6}, 'B' },
                { 3, 0, 4, 4, {1, 2, 3, 7}, 'B' },
                { 4, 0, 4, 4, {1, 2, 4, 8}, 'B' },
                { 1, 0, 4, 4, {1, 2, 5, 9}, 'B' },
            },
            /* L0 */
            {
                { 1, 0, 4, 4, {1, 5, 9, 13}, 'B' },
                { 2, 0, 4, 4, {1, 2, 6, 10}, 'B' },
                { 3, 0, 4, 4, {1, 3, 7, 11}, 'B' },
                { 4, 0, 4, 4, {1, 4, 8, 12}, 'B' },
                { 1, 0, 1, 1, {1,}, 'B' },
                { 2, 0, 2, 2, {1, 2,}, 'B' },
                { 3, 0, 3, 3, {1, 2, 3, }, 'B' },
                { 4, 0, 4, 4, {1, 2, 3, 4}, 'B' },
                { 1, 0, 4, 4, {1, 2, 3, 5}, 'B' },
                { 2, 0, 4, 4, {1, 2, 3, 6}, 'B' },
                { 3, 0, 4, 4, {1, 2, 3, 7}, 'B' },
                { 4, 0, 4, 4, {1, 2, 4, 8}, 'B' },
                { 1, 0, 4, 4, {1, 2, 5, 9}, 'B' },
            },
        },
        /* GOP 8 */
        {
            /* L0 */
            {
                { 1, 0, 4, 4, {1, 9, 17, 25}, 'B' },
                { 2, 0, 4, 4, {1, 2, 10, 18}, 'B' },
                { 3, 0, 4, 4, {1, 3, 11, 19}, 'B' },
                { 4, 0, 4, 4, {1, 4, 12, 20}, 'B' },
                { 5, 0, 4, 4, {1, 5, 13, 21}, 'B' },
                { 6, 0, 4, 4, {1, 6, 14, 22}, 'B' },
                { 7, 0, 4, 4, {1, 7, 15, 23}, 'B' },
                { 8, 0, 4, 4, {1, 8, 16, 24}, 'B' },
                { 1, 0, 1, 1, {1}, 'B' },
                { 2, 0, 2, 2, {1, 2}, 'B' },
                { 3, 0, 3, 3, {1, 2, 3, }, 'B' },
                { 4, 0, 4, 4, {1, 2, 3, 4}, 'B' },
                { 5, 0, 4, 4, {1, 2, 3, 5}, 'B' },
                { 6, 0, 4, 4, {1, 2, 3, 6}, 'B' },
                { 7, 0, 4, 4, {1, 2, 3, 7}, 'B' },
                { 8, 0, 4, 4, {1, 2, 3, 8}, 'B' },
                { 1, 0, 4, 4, {1, 2, 3, 9}, 'B' },
                { 2, 0, 4, 4, {1, 2, 3, 10}, 'B' },
                { 3, 0, 4, 4, {1, 2, 3, 11}, 'B' },
                { 4, 0, 4, 4, {1, 2, 4, 12}, 'B' },
                { 5, 0, 4, 4, {1, 2, 5, 13}, 'B' },
                { 6, 0, 4, 4, {1, 2, 6, 14}, 'B' },
                { 7, 0, 4, 4, {1, 2, 7, 15}, 'B' },
                { 8, 0, 4, 4, {1, 2, 8, 16}, 'B' },
                { 1, 0, 4, 4, {1, 2, 9, 17}, 'B' },
            },
            /* L1 */
            {
                { 1, 0, 4, 4, {1, 9, 17, 25}, 'B' },
                { 2, 0, 4, 4, {1, 2, 10, 18}, 'B' },
                { 3, 0, 4, 4, {1, 3, 11, 19}, 'B' },
                { 4, 0, 4, 4, {1, 4, 12, 20}, 'B' },
                { 5, 0, 4, 4, {1, 5, 13, 21}, 'B' },
                { 6, 0, 4, 4, {1, 6, 14, 22}, 'B' },
                { 7, 0, 4, 4, {1, 7, 15, 23}, 'B' },
                { 8, 0, 4, 4, {1, 8, 16, 24}, 'B' },
                { 1, 0, 1, 1, {1}, 'B' },
                { 2, 0, 2, 2, {1, 2}, 'B' },
                { 3, 0, 3, 3, {1, 2, 3}, 'B' },
                { 4, 0, 4, 4, {1, 2, 3, 4}, 'B' },
                { 5, 0, 4, 4, {1, 2, 3, 5}, 'B' },
                { 6, 0, 4, 4, {1, 2, 3, 6}, 'B' },
                { 7, 0, 4, 4, {1, 2, 3, 7}, 'B' },
                { 8, 0, 4, 4, {1, 2, 3, 8}, 'B' },
                { 1, 0, 4, 4, {1, 2, 3, 9}, 'B' },
                { 2, 0, 4, 4, {1, 2, 3, 10}, 'B' },
                { 3, 0, 4, 4, {1, 2, 3, 11}, 'B' },
                { 4, 0, 4, 4, {1, 2, 4, 12}, 'B' },
                { 5, 0, 4, 4, {1, 2, 5, 13}, 'B' },
                { 6, 0, 4, 4, {1, 2, 6, 14}, 'B' },
                { 7, 0, 4, 4, {1, 2, 7, 15}, 'B' },
                { 8, 0, 4, 4, {1, 2, 8, 16}, 'B' },
                { 1, 0, 4, 4, {1, 2, 9, 17}, 'B' },
            },
        },
    },
    /* Enable redordering (RA) */
    {
        /* GOP 4 */
        {
            /* L0 */
            {
                { 4, 0, 3, 2, {4, 8, 6, }, 'B' },
                { 2, 1, 2, 2, {2, 4,}, 'B' },
                { 1, 2, 2, 2, {1, -1,}, 'B' },
                { 3, 3, 2, 2, {1, 3,}, 'B' },
                { 4, 0, 1, 1, {4,}, 'B' },
                { 2, 1, 2, 2, {2, -2,}, 'B' },
            },
            /* L1 */
            {
                { 4, 0, 2, 2, {4, 8,}, 'B' },
                { 2, 1, 2, 2, {-2, 2,}, 'B' },
                { 1, 2, 2, 2, {-1, -3,}, 'B' },
                { 3, 3, 2, 2, {-1, 1,}, 'B' },
                { 4, 0, 1, 1, {4,}, 'B' },
                { 2, 1, 2, 2, {-2, 2,}, 'B' },
            },
        },
        /* GOP 8 */
        {
            /* L0 */
            {
                { 8, 0, 3, 2, {8, 16, 12, }, 'B' },
                { 4, 1, 2, 2, {4, 8,}, 'B' },
                { 2, 2, 2, 2, {2, 6,}, 'B' },
                { 1, 3, 2, 2, {1, -1,}, 'B' },
                { 3, 3, 2, 2, {1, -3,}, 'B' },
                { 6, 2, 2, 2, {2, 4,}, 'B' },
                { 5, 3, 2, 2, {1, 5,}, 'B' },
                { 7, 3, 3, 2, {1, 3, 7,}, 'B' },
                { 8, 0, 1, 1, {8,}, 'B' },
                { 4, 1, 2, 2, {4, -4,}, 'B' },
            },
            {
                { 8, 0, 2, 2, {8, 16,}, 'B' },
                { 4, 1, 2, 2, {-4, 4,}, 'B' },
                { 2, 2, 2, 2, {-2, -6,}, 'B' },
                { 1, 3, 3, 2, {-1, -3, -7,}, 'B' },
                { 3, 3, 2, 2, {-1, -5,}, 'B' },
                { 6, 2, 2, 2, {-2, 4,}, 'B' },
                { 5, 3, 2, 2, {-1, -3,}, 'B' },
                { 7, 3, 2, 2, {-1, 1,}, 'B' },
                { 8, 0, 2, 1, {8,}, 'B' },
                { 4, 1, 2, 2, {-4, 4,}, 'B' },
            },
        },
        /* GOP 16 */
        {
            /* L0 */
            {
                { 16, 0, 3, 2, {16, 32, 24,}, 'B' },
                { 8,  1, 2, 2, {8, 16,}, 'B' },
                { 4,  2, 2, 2, {4, 12,}, 'B' },
                { 2,  3, 2, 2, {2, 10,}, 'B' },
                { 1,  4, 2, 2, {1, -1,}, 'B' },
                { 3,  4, 2, 2, {1, 3,}, 'B' },
                { 6,  3, 2, 2, {2, 6,}, 'B' },
                { 5,  4, 2, 2, {1, 5,}, 'B' },
                { 7,  4, 3, 2, {1, 3, 7,}, 'B' },
                { 12, 2, 2, 2, {4, 12,}, 'B' },
                { 10, 3, 2, 2, {2, 10,}, 'B' },
                { 9,  4, 2, 2, {1, 9,}, 'B' },
                { 11, 4, 3, 2, {1, 3, 11,}, 'B' },
                { 13, 4, 3, 2, {1, 5, 13,}, 'B' },
                { 14, 3, 3, 2, {2, 6, 14,}, 'B' },
                { 15, 4, 4, 2, {1, 3, 7, 15}, 'B' },
                { 16, 0, 1, 1, {16,}, 'B' },
                { 8,  1, 2, 2, {8, -8,}, 'B' },
                { 4,  2, 2, 2, {4, -4,}, 'B' },
                { 2,  3, 2, 2, {2, -2,}, 'B' },
            },
            /* L1 */
            {
                { 16, 0, 2, 2, {16, 32,}, 'B' },
                { 8,  1, 2, 2, {-8, 8,}, 'B' },
                { 4,  2, 2, 2, {-4, -12,}, 'B' },
                { 2,  3, 3, 2, {-2, -6, -14,}, 'B' },
                { 1,  4, 4, 2, {-1, -3, -7, -15}, 'B' },
                { 3,  4, 3, 2, {-1, -5, -13,}, 'B' },
                { 6,  3, 2, 2, {-2, -10,}, 'B' },
                { 5,  4, 3, 2, {-1, -3, -11,}, 'B' },
                { 7,  4, 2, 2, {-1, -9,}, 'B' },
                { 12, 2, 2, 2, {-4, 4,}, 'B' },
                { 10, 3, 2, 2, {-2, -6,}, 'B' },
                { 9,  4, 3, 2, {-1, -3, -7, }, 'B' },
                { 11, 4, 2, 2, {-1, -5,}, 'B' },
                { 13, 3, 2, 2, {-1, -3,}, 'B' },
                { 14, 4, 2, 2, {-2, 2,}, 'B' },
                { 15, 4, 2, 2, {-1, 1,}, 'B' },
                { 16, 0, 1, 1, {16,}, 'B' },
                { 8,  1, 2, 2, {-8, 8,}, 'B' },
                { 4,  2, 2, 2, {-4, -12,}, 'B' },
                { 2,  3, 3, 2, {-2, -6, -14,}, 'B' },
            },
        },
    },

};


//Implementation for selecting and assigning RPL0 & RPL1 candidates in the SPS to SH
void select_assign_rpl_for_sh(XEVE_CTX *ctx, XEVE_SH *sh)
{
    //TBD: when NALU types are implemented; if the current picture is an IDR, simply return without doing the rest of the codes for this function


    /* introduce this variable for LD reason. The predefined RPL in the cfg file is made assuming GOP size is 8 for LD configuration*/
    int gopSize = (ctx->param.gop_size == 1) ? ctx->param.ref_pic_gap_length : ctx->param.gop_size;


    //Assume it the pic is in the normal GOP first. Normal GOP here means it is not the first (few) GOP in the beginning of the bitstream
    sh->rpl_l0_idx = sh->rpl_l1_idx = -1;
    sh->ref_pic_list_sps_flag[0] = sh->ref_pic_list_sps_flag[1] = 0;

    int availableRPLs = (ctx->sps.num_ref_pic_lists_in_sps0 < gopSize) ? ctx->sps.num_ref_pic_lists_in_sps0 : gopSize;
    for (int i = 0; i < availableRPLs; i++)
    {
        int pocIdx;
        if (ctx->param.keyint > 0)
        {
            pocIdx = ((ctx->poc.poc_val % ctx->param.keyint) % gopSize == 0) ? gopSize : (ctx->poc.poc_val % ctx->param.keyint) % gopSize;
        }
        else
        {
            pocIdx = (ctx->poc.poc_val % gopSize == 0) ? gopSize : ctx->poc.poc_val % gopSize;
        }

        if (pocIdx == ctx->sps.rpls_l0[i].poc)
        {
            sh->rpl_l0_idx = i;
            sh->rpl_l1_idx = sh->rpl_l0_idx;
            break;
        }
    }

    //For special case when the pic is in the first (few) GOP in the beginning of the bitstream.
    if (ctx->param.gop_size == 1)                          //For low delay configuration
    {
        if (ctx->poc.poc_val <= (ctx->sps.num_ref_pic_lists_in_sps0 - gopSize))
        {
            sh->rpl_l0_idx = ctx->poc.poc_val + gopSize - 1;
            sh->rpl_l1_idx = sh->rpl_l0_idx;
        }
    }
    else                                                 //For random access configuration
    {
        for (int i = gopSize; i < ctx->sps.num_ref_pic_lists_in_sps0; i++)
        {
            int pocIdx = ctx->param.keyint == 0 ? ctx->poc.poc_val : (ctx->poc.poc_val % ctx->param.keyint == 0) ? ctx->param.keyint : ctx->poc.poc_val % ctx->param.keyint;
            if (pocIdx == ctx->sps.rpls_l0[i].poc)
            {
                sh->rpl_l0_idx = i;
                sh->rpl_l1_idx = i;
                break;
            }
        }
    }
    if (ctx->slice_type != SLICE_I)
    {
        ctx->slice_type = ctx->param.inter_slice_type;
    }
    //Copy RPL0 from the candidate in SPS to this SH
    sh->rpl_l0.poc = ctx->poc.poc_val;
    if (sh->rpl_l0_idx != -1)
    {
    sh->rpl_l0.tid = ctx->sps.rpls_l0[sh->rpl_l0_idx].tid;
    sh->rpl_l0.ref_pic_num = ctx->sps.rpls_l0[sh->rpl_l0_idx].ref_pic_num;
    sh->rpl_l0.ref_pic_active_num = ctx->sps.rpls_l0[sh->rpl_l0_idx].ref_pic_active_num;
    for (int i = 0; i < sh->rpl_l0.ref_pic_num; i++)
        sh->rpl_l0.ref_pics[i] = ctx->sps.rpls_l0[sh->rpl_l0_idx].ref_pics[i];
    }

    //Copy RPL0 from the candidate in SPS to this SH
    sh->rpl_l1.poc = ctx->poc.poc_val;
    if (sh->rpl_l1_idx != -1)
    {
    sh->rpl_l1.tid = ctx->sps.rpls_l1[sh->rpl_l1_idx].tid;
    sh->rpl_l1.ref_pic_num = ctx->sps.rpls_l1[sh->rpl_l1_idx].ref_pic_num;
    sh->rpl_l1.ref_pic_active_num = ctx->sps.rpls_l1[sh->rpl_l1_idx].ref_pic_active_num;
    for (int i = 0; i < sh->rpl_l1.ref_pic_num; i++)
        sh->rpl_l1.ref_pics[i] = ctx->sps.rpls_l1[sh->rpl_l1_idx].ref_pics[i];
    }

    if (sh->rpl_l0_idx != -1)
    {
        sh->ref_pic_list_sps_flag[0] = 1;
    }

    if (sh->rpl_l1_idx != -1)
    {
        sh->ref_pic_list_sps_flag[1] = 1;
    }
}

//Return value 0 means all ref pic listed in the given rpl are available in the DPB
//Return value 1 means there is at least one ref pic listed in the given rpl not available in the DPB
static int check_refpic_available(int currentPOC, XEVE_PM *pm, XEVE_RPL *rpl)
{
    for (int i = 0; i < rpl->ref_pic_num; i++)
    {
        int isExistInDPB = 0;
        for (int j = 0; !isExistInDPB && j < MAX_PB_SIZE; j++)
        {
            if (pm->pic[j] && pm->pic[j]->is_ref && pm->pic[j]->poc == (currentPOC - rpl->ref_pics[i]))
                isExistInDPB = 1;
        }
        if (!isExistInDPB) //Found one ref pic missing return 1
            return 1;
    }
    return 0;
}

//Return value 0 means no explicit RPL is created. The given input parameters rpl0 and rpl1 are not modified
//Return value 1 means the given input parameters rpl0 and rpl1 are modified
static int create_explicit_rpl(XEVE_PM *pm, XEVE_SH *sh, int poc_val)
{
    XEVE_RPL *rpl0 = &sh->rpl_l0;
    XEVE_RPL *rpl1 = &sh->rpl_l1;
    if (!check_refpic_available(poc_val, pm, rpl0) && !check_refpic_available(poc_val, pm, rpl1))
    {
        return 0;
    }

    XEVE_PIC * pic = NULL;

    int isRPLChanged = 0;
    //Remove ref pic in RPL0 that is not available in the DPB
    for (int ii = 0; ii < rpl0->ref_pic_num; ii++)
    {
        int isAvailable = 0;
        for (int jj = 0; !isAvailable && jj < pm->cur_num_ref_pics; jj++)
        {
            pic = pm->pic[jj];
            if (pic && pic->is_ref && pic->poc == (poc_val - rpl0->ref_pics[ii]))
                isAvailable = 1;
            pic = NULL;
        }
        if (!isAvailable)
        {
            for (int jj = ii; jj < rpl0->ref_pic_num - 1; jj++)
                rpl0->ref_pics[jj] = rpl0->ref_pics[jj + 1];
            ii--;
            rpl0->ref_pic_num--;
            isRPLChanged = 1;
        }
    }
    if (isRPLChanged)
        sh->rpl_l0_idx = -1;

    //Remove ref pic in RPL1 that is not available in the DPB
    isRPLChanged = 0;
    for (int ii = 0; ii < rpl1->ref_pic_num; ii++)
    {
        int isAvailable = 0;
        for (int jj = 0; !isAvailable && jj < pm->cur_num_ref_pics; jj++)
        {
            pic = pm->pic[jj];
            if (pic && pic->is_ref && pic->poc == (poc_val - rpl1->ref_pics[ii]))
                isAvailable = 1;
            pic = NULL;
        }
        if (!isAvailable)
        {
            for (int jj = ii; jj < rpl1->ref_pic_num - 1; jj++)
                rpl1->ref_pics[jj] = rpl1->ref_pics[jj + 1];
            ii--;
            rpl1->ref_pic_num--;
            isRPLChanged = 1;
        }
    }
    if (isRPLChanged)
        sh->rpl_l1_idx = -1;

    /*if number of ref pic in RPL0 is less than its number of active ref pic, try to copy from RPL1*/
    if (rpl0->ref_pic_num < rpl0->ref_pic_active_num)
    {
        for (int ii = rpl0->ref_pic_num; ii < rpl0->ref_pic_active_num; ii++)
        {
            //First we need to find ref pic in RPL1 that is not already in RPL0
            int isAlreadyIncluded = 1;
            int idx = -1;
            int status = 0;
            do {
                status = 0;
                idx++;
                for (int mm = 0; mm < rpl0->ref_pic_num && idx < rpl1->ref_pic_num; mm++)
                {
                    if (rpl1->ref_pics[idx] == rpl0->ref_pics[mm])
                        status = 1;
                }
                if (!status) isAlreadyIncluded = 0;
            } while (isAlreadyIncluded && idx < rpl1->ref_pic_num);

            if (idx < rpl1->ref_pic_num)
            {
                rpl0->ref_pics[ii] = rpl1->ref_pics[idx];
                rpl0->ref_pic_num++;
            }
        }
        if (rpl0->ref_pic_num < rpl0->ref_pic_active_num) rpl0->ref_pic_active_num = rpl0->ref_pic_num;
    }

    /*same logic as above, just apply to RPL1*/
    if (rpl1->ref_pic_num < rpl1->ref_pic_active_num)
    {
        for (int ii = rpl1->ref_pic_num; ii < rpl1->ref_pic_active_num; ii++)
        {
            int isAlreadyIncluded = 1;
            int idx = -1;
            int status = 0;
            do {
                status = 0;
                idx++;
                for (int mm = 0; mm < rpl1->ref_pic_num && idx < rpl0->ref_pic_num; mm++)
                {
                    if (rpl0->ref_pics[idx] == rpl1->ref_pics[mm])
                        status = 1;
                }
                if (!status) isAlreadyIncluded = 0;
            } while (isAlreadyIncluded && idx < rpl0->ref_pic_num);

            if (idx < rpl0->ref_pic_num)
            {
                rpl1->ref_pics[ii] = rpl0->ref_pics[idx];
                rpl1->ref_pic_num++;
            }
        }
        if (rpl1->ref_pic_num < rpl1->ref_pic_active_num) rpl1->ref_pic_active_num = rpl1->ref_pic_num;
    }
    return 1;
}

int xeve_picman_refp_rpl_based_init(XEVE_PM *pm, XEVE_SH *sh, int poc_val, XEVE_REFP(*refp)[REFP_NUM])
{
    for (int i = 0; i < XEVE_MAX_NUM_REF_PICS; i++)
        refp[i][REFP_0].pic = refp[i][REFP_1].pic = NULL;
    pm->num_refp[REFP_0] = pm->num_refp[REFP_1] = 0;

    if (sh->slice_type == SLICE_I)
    {
        return XEVE_OK;
    }

    xeve_picman_update_pic_ref(pm);
    xeve_assert_rv(pm->cur_num_ref_pics > 0, XEVE_ERR_UNEXPECTED);

    //Do the L0 first
    for (int i = 0; i < sh->rpl_l0.ref_pic_active_num; i++)
    {
        int refPicPoc = poc_val - sh->rpl_l0.ref_pics[i];
        //Find the ref pic in the DPB
        int j = 0;
        while (j < pm->cur_num_ref_pics && pm->pic_ref[j]->poc != refPicPoc) j++;

        //If the ref pic is found, set it to RPL0
        if (j < pm->cur_num_ref_pics && pm->pic_ref[j]->poc == refPicPoc)
        {
            xeve_set_refp(&refp[i][REFP_0], pm->pic_ref[j]);
            pm->num_refp[REFP_0] = pm->num_refp[REFP_0] + 1;
        }
        else
            return XEVE_ERR;   //The refence picture must be available in the DPB, if not found then there is problem
    }

    if (sh->slice_type == SLICE_P) return XEVE_OK;

    //Do the L1 first
    for (int i = 0; i < sh->rpl_l1.ref_pic_active_num; i++)
    {
        int refPicPoc = poc_val - sh->rpl_l1.ref_pics[i];
        //Find the ref pic in the DPB
        int j = 0;
        while (j < pm->cur_num_ref_pics && pm->pic_ref[j]->poc != refPicPoc) j++;

        //If the ref pic is found, set it to RPL1
        if (j < pm->cur_num_ref_pics && pm->pic_ref[j]->poc == refPicPoc)
        {
            xeve_set_refp(&refp[i][REFP_1], pm->pic_ref[j]);
            pm->num_refp[REFP_1] = pm->num_refp[REFP_1] + 1;
        }
        else
            return XEVE_ERR;   //The refence picture must be available in the DPB, if not found then there is problem
    }

    return XEVE_OK;  //RPL construction completed
}

/*This is the implementation of reference picture marking based on RPL*/
int xeve_picman_refpic_marking(XEVE_PM *pm, XEVE_SH *sh, int poc_val)
{
    xeve_picman_update_pic_ref(pm);
    if (sh->slice_type != SLICE_I && poc_val != 0)
        xeve_assert_rv(pm->cur_num_ref_pics > 0, XEVE_ERR_UNEXPECTED);

    XEVE_PIC * pic;
    int numberOfPicsToCheck = pm->cur_num_ref_pics;
    for (int i = 0; i < numberOfPicsToCheck; i++)
    {
        pic = pm->pic[i];
        if (pm->pic[i] && IS_REF(pm->pic[i]))
        {
            //If the pic in the DPB is a reference picture, check if this pic is included in RPL0
            int isIncludedInRPL = 0;
            int j = 0;
            while (!isIncludedInRPL && j < sh->rpl_l0.ref_pic_num)
            {
                if (pic->poc == (poc_val - sh->rpl_l0.ref_pics[j]))  //NOTE: we need to put POC also in XEVE_PIC
                {
                    isIncludedInRPL = 1;
                }
                j++;
            }
            //Check if the pic is included in RPL1. This while loop will be executed only if the ref pic is not included in RPL0
            j = 0;
            while (!isIncludedInRPL && j < sh->rpl_l1.ref_pic_num)
            {
                if (pic->poc == (poc_val - sh->rpl_l1.ref_pics[j]))
                {
                    isIncludedInRPL = 1;
                }
                j++;
            }
            //If the ref pic is not included in either RPL0 nor RPL1, then mark it as not used for reference. move it to the end of DPB.
            if (!isIncludedInRPL)
            {
                SET_REF_UNMARK(pic);
                xeve_picman_move_pic(pm, i, MAX_PB_SIZE - 1);
                pm->cur_num_ref_pics--;
                i--;                                           //We need to decrement i here because it will be increment by i++ at for loop. We want to keep the same i here because after the move, the current ref pic at i position is the i+1 position which we still need to check.
                numberOfPicsToCheck--;                         //We also need to decrement this variable to avoid checking the moved ref picture twice.
            }
        }
    }
    return XEVE_OK;
}

int xeve_picman_rpl_refp_init(XEVE_CTX * ctx, XEVE_SH *sh)
{
    int ret = XEVE_OK;

    if (sh->slice_type != SLICE_I && ctx->poc.poc_val != 0) //TBD: change this condition to say that if this slice is not a slice in IDR picture
    {
        ret = create_explicit_rpl(&ctx->rpm, sh, ctx->poc.poc_val);
        if (ret == 1)
        {
            if (ctx->pps.rpl1_idx_present_flag)
            {
                if (sh->rpl_l0_idx == -1)
                {
                    sh->ref_pic_list_sps_flag[0] = 0;
                }
                if (sh->rpl_l1_idx == -1)
                {
                    sh->ref_pic_list_sps_flag[1] = 0;
                }
            }
            else
            {
                sh->ref_pic_list_sps_flag[0] = 0;
                sh->ref_pic_list_sps_flag[1] = 0;
            }
        }
    }

    if ((sh->rpl_l0.ref_pic_active_num - 1) == ctx->pps.num_ref_idx_default_active_minus1[REFP_0]
        && (sh->rpl_l1.ref_pic_active_num - 1) == ctx->pps.num_ref_idx_default_active_minus1[REFP_1])
    {
        sh->num_ref_idx_active_override_flag = 0;
    }
    else
    {
        sh->num_ref_idx_active_override_flag = 1;
    }

    /* reference picture marking */
    ret = xeve_picman_refpic_marking(&ctx->rpm, sh, ctx->poc.poc_val);
    xeve_assert_rv(ret == XEVE_OK, ret);

    /* reference picture lists construction */
    ret = xeve_picman_refp_rpl_based_init(&ctx->rpm, sh, ctx->poc.poc_val, ctx->refp);
    if (sh->slice_type != SLICE_I)
    {
        int delta_poc0 = (int)(ctx->poc.poc_val) - (int)(ctx->refp[0][REFP_0].poc);
        int delta_poc1 = (int)(ctx->poc.poc_val) - (int)(ctx->refp[0][REFP_1].poc);
        sh->temporal_mvp_asigned_flag = !(((delta_poc0 > 0) && (delta_poc1 > 0)) || ((delta_poc0 < 0) && (delta_poc1 < 0)));
    }

    return ret;
}
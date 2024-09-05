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

#include "xevem_df.h"

// clang-format off

#define DEFAULT_INTRA_TC_OFFSET             2
#define MAX_QP                              51
#define TCOFFSETDIV2                        0
#define BETAOFFSETDIV2                      0
#define CU_THRESH                           16


static const u8 sm_tc_table[MAX_QP + 1 + DEFAULT_INTRA_TC_OFFSET] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,2,2,2,2,3,3,3,3,4,4,4,5,5,6,6,7,8,9,10,11,13,14,16,18,20,22,24
};

static const u8 sm_beta_table[MAX_QP + 1] =
{
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,6,7,8,9,10,11,12,13,14,15,16,17,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,52,54,56,58,60,62,64
};

// clang-format on

static const u8 compare_mvs(const int mv0[2], const int mv1[2])
{
    // Return 1 if vetors difference less then 1 pixel
    return (XEVE_ABS(mv0[0] - mv1[0]) < 4) && (XEVE_ABS(mv0[1] - mv1[1]) < 4);
}

static const u8 get_index(const u8 qp, const u8 offset)
{
    return XEVE_CLIP3(0, MAX_QP, qp + offset);
}

static const u8 get_bs(u32 mcu0, u32 x0, u32 y0, u32 mcu1, u32 x1, u32 y1, u32 log2_max_cuwh, s8 *refi0
                       , s8 *refi1, s16(*mv0)[MV_D], s16(*mv1)[MV_D], XEVE_REFP(*refp)[REFP_NUM], u8 ats_present)
{
    u8 bs = DBF_ADDB_BS_OTHERS;
    u8 isIntraBlock = MCU_GET_IF(mcu0) || MCU_GET_IF(mcu1);
    int log2_cuwh = log2_max_cuwh;
    u8 sameXLCU = (x0 >> log2_cuwh) == (x1 >> log2_cuwh);
    u8 sameYLCU = (y0 >> log2_cuwh) == (y1 >> log2_cuwh);
#if TRACE_DBF
    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("Calculate BS. Input params: mcu0 = ");
    XEVE_TRACE_INT_HEX(mcu0);
    XEVE_TRACE_STR(", x0 = ");
    XEVE_TRACE_INT(x0);
    XEVE_TRACE_STR(", y0 = ");
    XEVE_TRACE_INT(y0);
    XEVE_TRACE_STR(", mcu1 = ");
    XEVE_TRACE_INT_HEX(mcu1);
    XEVE_TRACE_STR(", x1 = ");
    XEVE_TRACE_INT(x1);
    XEVE_TRACE_STR(", y1 = ");
    XEVE_TRACE_INT(y1);
    XEVE_TRACE_STR(", log2_max_cuwh = ");
    XEVE_TRACE_INT(log2_max_cuwh);
    XEVE_TRACE_STR(". isIntraBlock = ");
    XEVE_TRACE_INT(isIntraBlock ? 1 : 0);
    XEVE_TRACE_STR(". sameXLCU = ");
    XEVE_TRACE_INT(sameXLCU ? 1 : 0);
    XEVE_TRACE_STR(". sameYLCU = ");
    XEVE_TRACE_INT(sameYLCU ? 1 : 0);
    XEVE_TRACE_STR(". MCU_GET_CBFL(mcu0) = ");
    XEVE_TRACE_INT(MCU_GET_CBFL(mcu0) ? 1 : 0);
    XEVE_TRACE_STR(". MCU_GET_CBFL(mcu1) = ");
    XEVE_TRACE_INT(MCU_GET_CBFL(mcu1) ? 1 : 0);
    XEVE_TRACE_STR(". MCU_GET_IBC(mcu0) = ");
    XEVE_TRACE_INT(MCU_GET_IBC(mcu0) ? 1 : 0);
    XEVE_TRACE_STR(". MCU_GET_IBC(mcu1) = ");
    XEVE_TRACE_INT(MCU_GET_IBC(mcu1) ? 1 : 0);
#endif

    if(isIntraBlock && (!sameXLCU || !sameYLCU))
    {
        // One of the blocks is Intra and blocks lies in the different LCUs
        bs = DBF_ADDB_BS_INTRA_STRONG;
    }
    else if(isIntraBlock)
    {
        // One of the blocks is Intra
        bs = DBF_ADDB_BS_INTRA;
    }
    else if(MCU_GET_IBC(mcu0) || MCU_GET_IBC(mcu1))
    {
        bs = DBF_ADDB_BS_INTRA;
    }
    else if((MCU_GET_CBFL(mcu0) == 1 || MCU_GET_CBFL(mcu1) == 1) || ats_present)
    {
        // One of the blocks has coded residuals
        bs = DBF_ADDB_BS_CODED;
    }
    else
    {
        XEVE_PIC *refPics0[2], *refPics1[2];
        refPics0[REFP_0] = (REFI_IS_VALID(refi0[REFP_0])) ? refp[refi0[REFP_0]][REFP_0].pic : NULL;
        refPics0[REFP_1] = (REFI_IS_VALID(refi0[REFP_1])) ? refp[refi0[REFP_1]][REFP_1].pic : NULL;
        refPics1[REFP_0] = (REFI_IS_VALID(refi1[REFP_0])) ? refp[refi1[REFP_0]][REFP_0].pic : NULL;
        refPics1[REFP_1] = (REFI_IS_VALID(refi1[REFP_1])) ? refp[refi1[REFP_1]][REFP_1].pic : NULL;
        int mv0_l0[2] = {mv0[REFP_0][MV_X], mv0[REFP_0][MV_Y]};
        int mv0_l1[2] = {mv0[REFP_1][MV_X], mv0[REFP_1][MV_Y]};
        int mv1_l0[2] = {mv1[REFP_0][MV_X], mv1[REFP_0][MV_Y]};
        int mv1_l1[2] = {mv1[REFP_1][MV_X], mv1[REFP_1][MV_Y]};
#if TRACE_DBF
        XEVE_TRACE_STR(". MV info: refi0[REFP_0] = ");
        XEVE_TRACE_INT(refi0[REFP_0]);
        XEVE_TRACE_STR(", refi0[REFP_1] = ");
        XEVE_TRACE_INT(refi0[REFP_1]);
        XEVE_TRACE_STR(", refi1[REFP_0] = ");
        XEVE_TRACE_INT(refi1[REFP_0]);
        XEVE_TRACE_STR(", refi1[REFP_1] = ");
        XEVE_TRACE_INT(refi1[REFP_1]);
        XEVE_TRACE_STR("; mv0_l0 = {");
        XEVE_TRACE_INT(mv0[REFP_0][MV_X]);
        XEVE_TRACE_STR(", ");
        XEVE_TRACE_INT(mv0[REFP_0][MV_Y]);
        XEVE_TRACE_STR("}, mv0_l1 = {");
        XEVE_TRACE_INT(mv0[REFP_1][MV_X]);
        XEVE_TRACE_STR(", ");
        XEVE_TRACE_INT(mv0[REFP_1][MV_Y]);
        XEVE_TRACE_STR("}, mv1_l0 = {");
        XEVE_TRACE_INT(mv1[REFP_0][MV_X]);
        XEVE_TRACE_STR(", ");
        XEVE_TRACE_INT(mv1[REFP_0][MV_Y]);
        XEVE_TRACE_STR("}, mv1_l1 = {");
        XEVE_TRACE_INT(mv1[REFP_1][MV_X]);
        XEVE_TRACE_STR(", ");
        XEVE_TRACE_INT(mv1[REFP_1][MV_Y]);
        XEVE_TRACE_STR("}");
#endif

        if(!REFI_IS_VALID(refi0[REFP_0]))
        {
            mv0_l0[0] = mv0_l0[1] = 0;
        }

        if(!REFI_IS_VALID(refi0[REFP_1]))
        {
            mv0_l1[0] = mv0_l1[1] = 0;
        }

        if(!REFI_IS_VALID(refi1[REFP_0]))
        {
            mv1_l0[0] = mv1_l0[1] = 0;
        }

        if(!REFI_IS_VALID(refi1[REFP_1]))
        {
            mv1_l1[0] = mv1_l1[1] = 0;
        }


        if((((refPics0[REFP_0] == refPics1[REFP_0]) && (refPics0[REFP_1] == refPics1[REFP_1])))
           || ((refPics0[REFP_0] == refPics1[REFP_1]) && (refPics0[REFP_1] == refPics1[REFP_0])))
        {
            if(refPics0[REFP_0] == refPics0[REFP_1])
            {
                // Are vectors the same? Yes - 0, otherwise - 1.
                bs = (compare_mvs(mv0_l0, mv1_l0) && compare_mvs(mv0_l1, mv1_l1)
                      && compare_mvs(mv0_l0, mv1_l1) && compare_mvs(mv0_l1, mv1_l0)) ? DBF_ADDB_BS_OTHERS : DBF_ADDB_BS_DIFF_REFS;
            }
            else
            {
                if(((refPics0[REFP_0] == refPics1[REFP_0]) && (refPics0[REFP_1] == refPics1[REFP_1])))
                {
                    bs = (compare_mvs(mv0_l0, mv1_l0) && compare_mvs(mv0_l1, mv1_l1)) ? DBF_ADDB_BS_OTHERS : DBF_ADDB_BS_DIFF_REFS;
                }
                else if((refPics0[REFP_0] == refPics1[REFP_1]) && (refPics0[REFP_1] == refPics1[REFP_0]))
                {
                    bs = (compare_mvs(mv0_l0, mv1_l1) && compare_mvs(mv0_l1, mv1_l0)) ? DBF_ADDB_BS_OTHERS : DBF_ADDB_BS_DIFF_REFS;
                }
            }
        }
        else
        {
            bs = DBF_ADDB_BS_DIFF_REFS;
        }
    }
#if TRACE_DBF
    XEVE_TRACE_STR(". Answer, bs = ");
    XEVE_TRACE_INT(bs);
    XEVE_TRACE_STR(")\n");
#endif

    return bs;
}



static void deblock_get_pq(pel *buf, int offset, pel* p, pel* q, int size)
{
    // p and q has DBF_LENGTH elements
    u8 i;
    for(i = 0; i < size; ++i)
    {
        q[i] = buf[i * offset];
        p[i] = buf[(i + 1) * -offset];
    }
}

static void deblock_set_pq(pel *buf, int offset, pel* p, pel* q, int size)
{
    // p and q has DBF_LENGTH elements
    u8 i;
#if TRACE_DBF
    XEVE_TRACE_STR(" Set (P, Q): ");
#endif
    for(i = 0; i < size; ++i)
    {
        buf[i * offset] = q[i];
        buf[(i + 1) * -offset] = p[i];
#if TRACE_DBF
        if(i != 0)
        {
            XEVE_TRACE_STR(", ");
        }
        XEVE_TRACE_STR("(");
        XEVE_TRACE_INT(q[i]);
        XEVE_TRACE_STR(", ");
        XEVE_TRACE_INT(p[i]);
        XEVE_TRACE_STR(")");
#endif
    }
}

static const u8 deblock_line_apply(pel *p, pel* q, u16 alpha, u8 beta)
{
    return (XEVE_ABS(p[0] - q[0]) < alpha) && (XEVE_ABS(p[1] - p[0]) < beta) && (XEVE_ABS(q[1] - q[0]) < beta);
}

static void deblock_line_chroma_strong(pel* x, pel* y, pel* x_out)
{
    x_out[0] = (2 * x[1] + x[0] + y[1] + 2) >> 2;
}

static void deblock_line_luma_strong(pel* x, pel* y, pel* x_out)
{
    x_out[0] = (x[2] + 2 * (x[1] + x[0] + y[0]) + y[1] + 4) >> 3;
    x_out[1] = (x[2] + x[1] + x[0] + y[0] + 2) >> 2;
    x_out[2] = (2 * x[3] + 3 * x[2] + x[1] + x[0] + y[0] + 4) >> 3;
}

static void deblock_line_check(u16 alpha, u8 beta, pel *p, pel* q, u8 *ap, u8 *aq)
{
    *ap = (XEVE_ABS(p[0] - p[2]) < beta) ? 1 : 0;
    *aq = (XEVE_ABS(q[0] - q[2]) < beta) ? 1 : 0;
}

static pel deblock_line_normal_delta0(u8 c0, pel* p, pel* q)
{
    // This part of code wrote according to AdaptiveDeblocking Filter by P.List, and etc. IEEE transactions on circuits and ... Vol. 13, No. 7, 2003
    // and inconsists with code in JM 19.0
    return XEVE_CLIP3(-(pel)c0, (pel)c0, (4 * (q[0] - p[0]) + p[1] - q[1] + 4) >> 3);
}

static pel deblock_line_normal_delta1(u8 c1, pel* x, pel* y)
{
    return XEVE_CLIP3(-(pel)c1, (pel)c1, ((((x[2] + x[0] + y[0]) * 3) - 8 * x[1] - y[1])) >> 4);
}


static void deblock_scu_line_luma(pel *buf, int stride, u8 bs, u16 alpha, u8 beta, u8 c1, int bit_depth_minus8)
{
    pel p[DBF_LENGTH], q[DBF_LENGTH];
    pel p_out[DBF_LENGTH], q_out[DBF_LENGTH];

    deblock_get_pq(buf, stride, p, q, DBF_LENGTH);
    xeve_mcpy(p_out, p, DBF_LENGTH * sizeof(p[0]));
    xeve_mcpy(q_out, q, DBF_LENGTH * sizeof(q[0]));
#if TRACE_DBF
    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("Process luma line (bs = ");
    XEVE_TRACE_INT(bs);
    XEVE_TRACE_STR(", alpha = ");
    XEVE_TRACE_INT(alpha);
    XEVE_TRACE_STR(", beta = ");
    XEVE_TRACE_INT(beta);
    XEVE_TRACE_STR(", c1 = ");
    XEVE_TRACE_INT(c1);
    XEVE_TRACE_STR("). P = {");
    for(int i = 0; i < DBF_LENGTH; ++i)
    {
        if(i != 0)
        {
            XEVE_TRACE_STR(", ");
        }
        XEVE_TRACE_INT(p[i]);
    }
    XEVE_TRACE_STR("}. Q = {");
    for(int i = 0; i < DBF_LENGTH; ++i)
    {
        if(i != 0)
        {
            XEVE_TRACE_STR(", ");
        }
        XEVE_TRACE_INT(q[i]);
    }
    XEVE_TRACE_STR("}.");
#endif

    if(bs && deblock_line_apply(p, q, alpha, beta))
    {
        u8 ap, aq;
        deblock_line_check(alpha, beta, p, q, &ap, &aq);
#if TRACE_DBF
        XEVE_TRACE_STR(" Ap = ");
        XEVE_TRACE_INT(ap);
        XEVE_TRACE_STR(" Aq = ");
        XEVE_TRACE_INT(aq);
#endif
        if(bs == DBF_ADDB_BS_INTRA_STRONG)
        {
            if(ap && (XEVE_ABS(p[0] - q[0]) < ((alpha >> 2) + 2)))
            {
                deblock_line_luma_strong(p, q, p_out);
            }
            else
            {
                deblock_line_chroma_strong(p, q, p_out);
            }
            if(aq && (XEVE_ABS(p[0] - q[0]) < ((alpha >> 2) + 2)))
            {
                deblock_line_luma_strong(q, p, q_out);
            }
            else
            {
                deblock_line_chroma_strong(q, p, q_out);
            }

        }
        else
        {
            u8 c0;
            pel delta0, delta1;
            int pel_max = (1 << (bit_depth_minus8 + 8)) - 1;

            c0 = c1 + ((ap + aq) << XEVE_MAX(0, (bit_depth_minus8 + 8) - 9));

#if TRACE_DBF
            XEVE_TRACE_STR(" c1 = ");
            XEVE_TRACE_INT(c1);
            XEVE_TRACE_STR(" c0 = ");
            XEVE_TRACE_INT(c0);
#endif

            delta0 = deblock_line_normal_delta0(c0, p, q);
#if TRACE_DBF
            XEVE_TRACE_STR(" delta0 = ");
            XEVE_TRACE_INT(delta0);
#endif
            p_out[0] = XEVE_CLIP3(0, pel_max, p[0] + delta0);
            q_out[0] = XEVE_CLIP3(0, pel_max, q[0] - delta0);
            if(ap)
            {
                delta1 = deblock_line_normal_delta1(c1, p, q);
                p_out[1] = p[1] + delta1;
#if TRACE_DBF
                XEVE_TRACE_STR(" AP_delta1 = ");
                XEVE_TRACE_INT(delta1);
#endif
            }
            if(aq)
            {
                delta1 = deblock_line_normal_delta1(c1, q, p);
                q_out[1] = q[1] + delta1;
#if TRACE_DBF
                XEVE_TRACE_STR(" AQ_delta1 = ");
                XEVE_TRACE_INT(delta1);
#endif
            }
        }

        int pel_max = (1 << (bit_depth_minus8 + 8)) - 1;

        p_out[0] = XEVE_CLIP3(0, pel_max, p_out[0]);
        q_out[0] = XEVE_CLIP3(0, pel_max, q_out[0]);
        p_out[1] = XEVE_CLIP3(0, pel_max, p_out[1]);
        q_out[1] = XEVE_CLIP3(0, pel_max, q_out[1]);
        p_out[2] = XEVE_CLIP3(0, pel_max, p_out[2]);
        q_out[2] = XEVE_CLIP3(0, pel_max, q_out[2]);
        p_out[3] = XEVE_CLIP3(0, pel_max, p_out[3]);
        q_out[3] = XEVE_CLIP3(0, pel_max, q_out[3]);

        deblock_set_pq(buf, stride, p_out, q_out, DBF_LENGTH);
    }
#if TRACE_DBF
    else
    {
        XEVE_TRACE_STR("Line won't processed");
    }
    XEVE_TRACE_STR("\n");
#endif
}

static void deblock_scu_line_chroma(pel *buf, int stride, u8 bs, u16 alpha, u8 beta, u8 c0, int bit_depth_minus8)
{
    pel p[DBF_LENGTH_CHROMA], q[DBF_LENGTH_CHROMA];
    pel p_out[DBF_LENGTH_CHROMA], q_out[DBF_LENGTH_CHROMA];

    deblock_get_pq(buf, stride, p, q, DBF_LENGTH_CHROMA);
    xeve_mcpy(p_out, p, DBF_LENGTH_CHROMA * sizeof(p[0]));
    xeve_mcpy(q_out, q, DBF_LENGTH_CHROMA * sizeof(q[0]));
#if TRACE_DBF
    XEVE_TRACE_COUNTER;
    XEVE_TRACE_STR("Process chroma line (bs = ");
    XEVE_TRACE_INT(bs);
    XEVE_TRACE_STR(", alpha = ");
    XEVE_TRACE_INT(alpha);
    XEVE_TRACE_STR(", beta = ");
    XEVE_TRACE_INT(beta);
    XEVE_TRACE_STR(", c0 = ");
    XEVE_TRACE_INT(c0);
    XEVE_TRACE_STR("). P = {");
    for(int i = 0; i < DBF_LENGTH_CHROMA; ++i)
    {
        if(i != 0)
        {
            XEVE_TRACE_STR(", ");
        }
        XEVE_TRACE_INT(p[i]);
    }
    XEVE_TRACE_STR("}. Q = {");
    for(int i = 0; i < DBF_LENGTH_CHROMA; ++i)
    {
        if(i != 0)
        {
            XEVE_TRACE_STR(", ");
        }
        XEVE_TRACE_INT(q[i]);
    }
    XEVE_TRACE_STR("}.");
#endif

    if(bs && deblock_line_apply(p, q, alpha, beta))
    {
        if(bs == DBF_ADDB_BS_INTRA_STRONG)
        {
            deblock_line_chroma_strong(p, q, p_out);
            deblock_line_chroma_strong(q, p, q_out);
        }
        else
        {
            pel delta0;
            int pel_max = (1 << (bit_depth_minus8 + 8)) - 1;

            delta0 = deblock_line_normal_delta0(c0, p, q);
            p_out[0] = XEVE_CLIP3(0, pel_max, p[0] + delta0);
            q_out[0] = XEVE_CLIP3(0, pel_max, q[0] - delta0);
#if TRACE_DBF
            XEVE_TRACE_STR(" delta0 = ");
            XEVE_TRACE_INT(delta0);
#endif
        }

        int pel_max = (1 << (bit_depth_minus8 + 8)) - 1;

        p_out[0] = XEVE_CLIP3(0, pel_max, p_out[0]);
        q_out[0] = XEVE_CLIP3(0, pel_max, q_out[0]);
        p_out[1] = XEVE_CLIP3(0, pel_max, p_out[1]);
        q_out[1] = XEVE_CLIP3(0, pel_max, q_out[1]);

        deblock_set_pq(buf, stride, p_out, q_out, DBF_LENGTH_CHROMA);
    }
#if TRACE_DBF
    else
    {
        XEVE_TRACE_STR("Line won't processed");
    }
    XEVE_TRACE_STR("\n");
#endif
}

static void deblock_scu_addb_ver_luma(pel *buf, int stride, u8 bs, u16 alpha, u8 beta, u8 c1, int bit_depth_minus8)
{
    u8 i;
    pel *cur_buf = buf;
    for(i = 0; i < MIN_CU_SIZE; ++i, cur_buf += stride)
    {
        deblock_scu_line_luma(cur_buf, 1, bs, alpha, beta, c1, bit_depth_minus8);
    }
}

static void deblock_scu_addb_hor_luma(pel *buf, int stride, u8 bs, u16 alpha, u8 beta, u8 c1, int bit_depth_minus8)
{
    u8 i;
    pel *cur_buf = buf;
    for(i = 0; i < MIN_CU_SIZE; ++i, ++cur_buf)
    {
        deblock_scu_line_luma(cur_buf, stride, bs, alpha, beta, c1, bit_depth_minus8);
    }
}

static void deblock_scu_addb_ver_chroma(pel *buf, int stride, u8 bs, u16 alpha, u8 beta, u8 c0, int bit_depth_minus8)
{
    u8 i;
    pel *cur_buf = buf;
    for(i = 0; i < (MIN_CU_SIZE >> 1); ++i, cur_buf += stride)
    {
        deblock_scu_line_chroma(cur_buf, 1, bs, alpha, beta, c0, bit_depth_minus8);
    }
}

static void deblock_scu_addb_hor_chroma(pel *buf, int stride, u8 bs, u16 alpha, u8 beta, u8 c0, int bit_depth_minus8)
{
    u8 i;
    pel *cur_buf = buf;
    for(i = 0; i < (MIN_CU_SIZE >> 1); ++i, ++cur_buf)
    {
        deblock_scu_line_chroma(cur_buf, stride, bs, alpha, beta, c0, bit_depth_minus8);
    }
}

static u32* deblock_set_coded_block(u32* map_scu, int w, int h, int w_scu)
{
    int i, j;
    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            MCU_SET_COD(map_scu[j]);
        }
        map_scu += w_scu;
    }
    return map_scu;
}

static void deblock_addb_cu_hor(XEVE_PIC *pic, int x_pel, int y_pel, int cuw, int cuh, u32 *map_scu, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D]
                              , int w_scu, int log2_max_cuwh, XEVE_REFP(*refp)[REFP_NUM], int ats_inter_mode, TREE_CONS tree_cons, u8* map_tidx
                              , int boundary_filtering, u8* map_ats_inter, int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc, int* qp_chroma_dynamic[2])
{
    pel * y, *u, *v;
    int   i, t, qp, s_l, s_c;
    int   w = cuw >> MIN_CU_LOG2;
    int   h = cuh >> MIN_CU_LOG2;
    u8    indexA, indexB;
    u16   alpha;
    u8    beta;
    u8    c0, c1;
    u32 * map_scu_tmp;
    int   bitdepth_scale = (bit_depth_luma - 8);
    int   align_8_8_grid = 0;
    int   w_shift = XEVE_GET_CHROMA_W_SHIFT(chroma_format_idc);
    int   h_shift = XEVE_GET_CHROMA_H_SHIFT(chroma_format_idc);

    if(y_pel % 8 == 0)
    {
        align_8_8_grid = 1;
    }

    int  t1, t_copy; // Next row scu number
    t = (x_pel >> MIN_CU_LOG2) + (y_pel >> MIN_CU_LOG2) * w_scu;
    t_copy = t;
    t1 = (x_pel >> MIN_CU_LOG2) + ((y_pel - (1 << MIN_CU_LOG2)) >> MIN_CU_LOG2) * w_scu;

    map_scu += t;
    map_refi += t;
    map_mv += t;
    map_ats_inter += t;

    map_scu_tmp = map_scu;
    s_l = pic->s_l;
    s_c = pic->s_c;
    y = pic->y + x_pel + y_pel * s_l;
    t = (x_pel >> w_shift) + (y_pel >> h_shift) * s_c;
    u = pic->u + t;
    v = pic->v + t;

    int no_boundary = 0;
    if (y_pel > 0)
    {
        no_boundary = (map_tidx[t_copy] == map_tidx[t1]) || boundary_filtering;
    }

    if(align_8_8_grid  && y_pel > 0 && (no_boundary))
    {

        for(i = 0; i < (cuw >> MIN_CU_LOG2); ++i)
        {
#if TRACE_DBF
            XEVE_TRACE_COUNTER;
            XEVE_TRACE_STR("Start filtering hor boundary of SCU (");
            XEVE_TRACE_INT(x_pel);
            XEVE_TRACE_STR(", ");
            XEVE_TRACE_INT(y_pel);
            XEVE_TRACE_STR(") ats_inter_mode = ");
            XEVE_TRACE_INT(ats_inter_mode);
            XEVE_TRACE_STR(" tree_type = ");
            XEVE_TRACE_INT(tree_cons.tree_type);
            XEVE_TRACE_STR(" mode_cons = ");
            XEVE_TRACE_INT(tree_cons.mode_cons);
            XEVE_TRACE_STR("\n");
#endif

            t = (i << MIN_CU_LOG2);
            int cur_x_pel = x_pel + t;
            u8 current_ats = map_ats_inter[i];
            u8 neighbor_ats = map_ats_inter[i - w_scu];
            u8 ats_present = current_ats || neighbor_ats;
            u8 bs_cur = get_bs(map_scu[i], cur_x_pel, y_pel, map_scu[i - w_scu], cur_x_pel, y_pel - 1, log2_max_cuwh
                             , map_refi[i], map_refi[i - w_scu], map_mv[i], map_mv[i - w_scu], refp, ats_present);

            qp = (MCU_GET_QP(map_scu[i]) + MCU_GET_QP(map_scu[i - w_scu]) + 1) >> 1;

            indexA = get_index(qp, pic->pic_deblock_alpha_offset);            //! \todo Add offset for IndexA
            indexB = get_index(qp, pic->pic_deblock_beta_offset);            //! \todo Add offset for IndexB

            alpha = xevem_addb_alpha_tbl[indexA] << bitdepth_scale;
            beta = xevem_addb_beta_tbl[indexB] << bitdepth_scale;
            c1 = xevem_addb_clip_tbl[indexA][bs_cur] << XEVE_MAX(0, (bit_depth_luma - 9));

            if(xeve_check_luma(tree_cons))
            {
                deblock_scu_addb_hor_luma(y + t, s_l, bs_cur, alpha, beta, c1, bit_depth_luma - 8);
            }
            if(xeve_check_chroma(tree_cons) && chroma_format_idc)
            {
                t >>= w_shift;
                int qp_u = XEVE_CLIP3(-6 * (bit_depth_chroma - 8), 57, qp + pic->pic_qp_u_offset);
                indexA = get_index(qp_chroma_dynamic[0][qp_u], pic->pic_deblock_alpha_offset);
                indexB = get_index(qp_chroma_dynamic[0][qp_u], pic->pic_deblock_beta_offset);
                alpha = xevem_addb_alpha_tbl[indexA] << bitdepth_scale;
                beta = xevem_addb_beta_tbl[indexB] << bitdepth_scale;
                c1 = xevem_addb_clip_tbl[indexA][bs_cur];
                c0 = (c1 + 1) << XEVE_MAX(0, (bit_depth_chroma - 9));

                deblock_scu_addb_hor_chroma(u + t, s_c, bs_cur, alpha, beta, c0, bit_depth_chroma - 8);

                int qp_v = XEVE_CLIP3(-6 * (bit_depth_chroma - 8), 57, qp + pic->pic_qp_v_offset);
                indexA = get_index(qp_chroma_dynamic[1][qp_v], pic->pic_deblock_alpha_offset);
                indexB = get_index(qp_chroma_dynamic[1][qp_v], pic->pic_deblock_beta_offset);
                alpha = xevem_addb_alpha_tbl[indexA] << bitdepth_scale;
                beta = xevem_addb_beta_tbl[indexB] << bitdepth_scale;
                c1 = xevem_addb_clip_tbl[indexA][bs_cur];
                c0 = (c1 + 1) << XEVE_MAX(0, (bit_depth_chroma - 9));
                deblock_scu_addb_hor_chroma(v + t, s_c, bs_cur, alpha, beta, c0, bit_depth_chroma - 8);
            }
        }
    }

    map_scu = deblock_set_coded_block(map_scu_tmp, w, h, w_scu);
}

static void deblock_addb_cu_ver_yuv(XEVE_PIC *pic, int x_pel, int y_pel, int log2_max_cuwh, pel *y, pel* u, pel *v, int s_l, int s_c, int cuh, u32 *map_scu, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D], int w_scu, XEVE_REFP(*refp)[REFP_NUM], int ats_inter_mode
                                  , TREE_CONS tree_cons, u8* map_ats_inter, int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc, int* qp_chroma_dynamic[2])
{
    int i, qp;
    int h = cuh >> MIN_CU_LOG2;
    u8 indexA, indexB;
    u16 alpha;
    u8 beta;
    u8 c0, c1;
    const int bitdepth_scale = (bit_depth_luma - 8);

    for(i = 0; i < h; i++)
    {
#if TRACE_DBF
        XEVE_TRACE_COUNTER;
        XEVE_TRACE_STR("Start filtering ver boundary of SCU (");
        XEVE_TRACE_INT(x_pel);
        XEVE_TRACE_STR(", ");
        XEVE_TRACE_INT(y_pel);
        XEVE_TRACE_STR(") ats_inter_mode = ");
        XEVE_TRACE_INT(ats_inter_mode);
        XEVE_TRACE_STR(" tree_type = ");
        XEVE_TRACE_INT(tree_cons.tree_type);
        XEVE_TRACE_STR(" mode_cons = ");
        XEVE_TRACE_INT(tree_cons.mode_cons);
        XEVE_TRACE_STR("\n");
#endif
        {
            int cur_y_pel = y_pel + (i << MIN_CU_LOG2);
            u8 current_ats = map_ats_inter[0];
            u8 neighbor_ats = map_ats_inter[-1];
            u8 ats_present = current_ats || neighbor_ats;
            u8 bs_cur = get_bs(map_scu[0], x_pel, cur_y_pel, map_scu[-1], x_pel - 1, cur_y_pel, log2_max_cuwh
                               , map_refi[0], map_refi[-1], map_mv[0], map_mv[-1], refp, ats_present);

            qp = (MCU_GET_QP(map_scu[0]) + MCU_GET_QP(map_scu[-1]) + 1) >> 1;

            if(xeve_check_luma(tree_cons))
            {
                indexA = get_index(qp, pic->pic_deblock_alpha_offset);            //! \todo Add offset for IndexA
                indexB = get_index(qp, pic->pic_deblock_beta_offset);            //! \todo Add offset for IndexB

                alpha = xevem_addb_alpha_tbl[indexA] << bitdepth_scale;
                beta = xevem_addb_beta_tbl[indexB] << bitdepth_scale;
                c1 = xevem_addb_clip_tbl[indexA][bs_cur] << XEVE_MAX(0, (bit_depth_luma - 9));

                deblock_scu_addb_ver_luma(y, s_l, bs_cur, alpha, beta, c1, bit_depth_luma - 8);
            }
            if(xeve_check_chroma(tree_cons) && chroma_format_idc)
            {
                int qp_u = XEVE_CLIP3(-6 * (bit_depth_chroma - 8), 57, qp + pic->pic_qp_u_offset);

                indexA = get_index(qp_chroma_dynamic[0][qp_u], pic->pic_deblock_alpha_offset);
                indexB = get_index(qp_chroma_dynamic[0][qp_u], pic->pic_deblock_beta_offset);

                alpha = xevem_addb_alpha_tbl[indexA] << bitdepth_scale;
                beta = xevem_addb_beta_tbl[indexB] << bitdepth_scale;

                c1 = xevem_addb_clip_tbl[indexA][bs_cur];
                c0 = (c1 + 1) << XEVE_MAX(0, (bit_depth_chroma - 9));

                deblock_scu_addb_ver_chroma(u, s_c, bs_cur, alpha, beta, c0, bit_depth_chroma - 8);

                int qp_v = XEVE_CLIP3(-6 * (bit_depth_chroma - 8), 57, qp + pic->pic_qp_v_offset);
                indexA = get_index(qp_chroma_dynamic[1][qp_v], pic->pic_deblock_alpha_offset);
                indexB = get_index(qp_chroma_dynamic[1][qp_v], pic->pic_deblock_beta_offset);

                alpha = xevem_addb_alpha_tbl[indexA] << bitdepth_scale;
                beta = xevem_addb_beta_tbl[indexB] << bitdepth_scale;

                c1 = xevem_addb_clip_tbl[indexA][bs_cur];
                c0 = (c1 + 1) << XEVE_MAX(0, (bit_depth_chroma - 9));

                deblock_scu_addb_ver_chroma(v, s_c, bs_cur, alpha, beta, c0, bit_depth_chroma - 8);
            }

            y += (s_l << MIN_CU_LOG2);
            u += (s_c << (MIN_CU_LOG2 - (XEVE_GET_CHROMA_W_SHIFT(chroma_format_idc))));
            v += (s_c << (MIN_CU_LOG2 - (XEVE_GET_CHROMA_W_SHIFT(chroma_format_idc))));

            map_scu += w_scu;
            map_refi += w_scu;
            map_mv += w_scu;
            map_ats_inter += w_scu;
        }
    }

}

static void deblock_addb_cu_ver(XEVE_PIC *pic, int x_pel, int y_pel, int cuw, int cuh, u32 *map_scu, s8(*map_refi)[REFP_NUM]
                              , s16(*map_mv)[REFP_NUM][MV_D], int w_scu, int log2_max_cuwh, u32  *map_cu, XEVE_REFP(*refp)[REFP_NUM]
                              , int ats_inter_mode, TREE_CONS tree_cons, u8* map_tidx, int boundary_filtering, u8* map_ats_inter
                              , int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc, int* qp_chroma_dynamic[2])
{
    pel  * y, *u, *v;
    int    t, s_l, s_c;
    int    w = cuw >> MIN_CU_LOG2;
    int    h = cuh >> MIN_CU_LOG2;
    u32  * map_scu_tmp;
    s8  (* map_refi_tmp)[REFP_NUM];
    s16 (* map_mv_tmp)[REFP_NUM][MV_D];
    u8   * map_ats_inter_tmp;
    u32  * map_cu_tmp;
    int    align_8_8_grid = 0;
    int    w_shift = XEVE_GET_CHROMA_W_SHIFT(chroma_format_idc);
    int    h_shift = XEVE_GET_CHROMA_H_SHIFT(chroma_format_idc);

    if(x_pel % 8 == 0)
    {
        align_8_8_grid = 1;
    }

    int  t1, t2, t_copy; // Next row scu number
    t = (x_pel >> MIN_CU_LOG2) + (y_pel >> MIN_CU_LOG2) * w_scu;
    t_copy = t;

    t1 = ((x_pel - (1 << MIN_CU_LOG2)) >> MIN_CU_LOG2) + (y_pel >> MIN_CU_LOG2) * w_scu;
    t2 = ((x_pel + (w << MIN_CU_LOG2)) >> MIN_CU_LOG2) + (y_pel >> MIN_CU_LOG2) * w_scu;

    map_scu += t;
    map_refi += t;
    map_mv += t;
    map_ats_inter += t;
    map_cu += t;

    s_l = pic->s_l;
    s_c = pic->s_c;
    y = pic->y + x_pel + y_pel * s_l;
    t = (x_pel >> w_shift) + (y_pel >> h_shift) * s_c;
    u = pic->u + t;
    v = pic->v + t;

    map_scu_tmp = map_scu;
    map_refi_tmp = map_refi;
    map_mv_tmp = map_mv;
    map_ats_inter_tmp = map_ats_inter;
    map_cu_tmp = map_cu;

    /* vertical filtering */

    int no_boundary = 0;
    if (x_pel > 0)
    {
        no_boundary = (map_tidx[t_copy] == map_tidx[t1]) || boundary_filtering;
    }

    if(align_8_8_grid && x_pel > 0 && MCU_GET_COD(map_scu[-1]) && (no_boundary))
    {
        deblock_addb_cu_ver_yuv(pic, x_pel, y_pel, log2_max_cuwh, y, u, v, s_l, s_c, cuh, map_scu, map_refi, map_mv, w_scu, refp, ats_inter_mode
                              , tree_cons, map_ats_inter, bit_depth_luma, bit_depth_chroma, chroma_format_idc, qp_chroma_dynamic);
    }

    map_scu = map_scu_tmp;
    map_refi = map_refi_tmp;
    map_mv = map_mv_tmp;
    map_ats_inter = map_ats_inter_tmp;
    map_cu = map_cu_tmp;

    no_boundary = 0;
    if (x_pel + cuw < pic->w_l)
    {
        no_boundary = (map_tidx[t_copy] == map_tidx[t2]) || boundary_filtering;
    }

    if((x_pel + cuw) % 8 == 0)
    {
        align_8_8_grid = 1;
    }
    else
    {
        align_8_8_grid = 0;
    }

    if(align_8_8_grid && x_pel + cuw < pic->w_l && MCU_GET_COD(map_scu[w]) && (no_boundary))
    {
        y = pic->y + x_pel + y_pel * s_l;
        u = pic->u + t;
        v = pic->v + t;

        y += cuw;
        u += (cuw >> w_shift);
        v += (cuw >> w_shift);
        map_scu += w;
        map_refi += w;
        map_mv += w;
        map_ats_inter += w;

        deblock_addb_cu_ver_yuv(pic, x_pel + cuw, y_pel, log2_max_cuwh, y, u, v, s_l, s_c, cuh, map_scu, map_refi, map_mv, w_scu, refp, ats_inter_mode
                              , tree_cons, map_ats_inter, bit_depth_luma, bit_depth_chroma, chroma_format_idc, qp_chroma_dynamic);
    }

    map_scu = deblock_set_coded_block(map_scu_tmp, w, h, w_scu);
}


void xevem_deblock_cu_hor(XEVE_PIC *pic, int x_pel, int y_pel, int cuw, int cuh, u32 *map_scu, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D]
                        , int w_scu, int log2_max_cuwh, XEVE_REFP(*refp)[REFP_NUM], int ats_inter_mode, TREE_CONS tree_cons, u8* map_tidx
                        , int boundary_filtering, int tool_addb, u8* map_ats_inter, int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc, int* qp_chroma_dynamic[2])
{
    if(tool_addb)
    {
        deblock_addb_cu_hor(pic, x_pel, y_pel, cuw, cuh, map_scu, map_refi, map_mv, w_scu, log2_max_cuwh, refp
                          , ats_inter_mode, tree_cons, map_tidx, boundary_filtering, map_ats_inter
                          , bit_depth_luma, bit_depth_chroma, chroma_format_idc, qp_chroma_dynamic);
    }
    else
    {
        xeve_deblock_cu_hor(pic, x_pel, y_pel, cuw, cuh, map_scu, map_refi, map_mv, w_scu, tree_cons, map_tidx
                          , boundary_filtering, bit_depth_luma, bit_depth_chroma, chroma_format_idc, qp_chroma_dynamic);
    }
}

void xevem_deblock_cu_ver(XEVE_PIC *pic, int x_pel, int y_pel, int cuw, int cuh, u32 *map_scu, s8(*map_refi)[REFP_NUM], s16(*map_mv)[REFP_NUM][MV_D]
                        , int w_scu, int log2_max_cuwh, u32  *map_cu, XEVE_REFP(*refp)[REFP_NUM], int ats_inter_mode, TREE_CONS tree_cons, u8* map_tidx
                        , int boundary_filtering, int tool_addb, u8* map_ats_inter, int bit_depth_luma, int bit_depth_chroma, int chroma_format_idc, int* qp_chroma_dynamic[2])
{
    if(tool_addb)
    {
        deblock_addb_cu_ver(pic, x_pel, y_pel, cuw, cuh, map_scu, map_refi, map_mv, w_scu, log2_max_cuwh, map_cu, refp
                          , ats_inter_mode, tree_cons, map_tidx, boundary_filtering, map_ats_inter
                          , bit_depth_luma, bit_depth_chroma, chroma_format_idc, qp_chroma_dynamic);
    }
    else
    {
        xeve_deblock_cu_ver(pic, x_pel, y_pel, cuw, cuh, map_scu, map_refi, map_mv, w_scu, map_cu, tree_cons, map_tidx
                          , boundary_filtering, bit_depth_luma, bit_depth_chroma, chroma_format_idc, qp_chroma_dynamic);
    }
}

void xevem_deblock_unit(XEVE_CTX * ctx, XEVE_PIC * pic, int x, int y, int cuw, int cuh, int is_hor_edge, XEVE_CORE * core, int boundary_filtering)
{
    XEVEM_CTX *mctx = (XEVEM_CTX *)ctx;

    if(is_hor_edge)
    {
        if (cuh > MAX_TR_SIZE)
        {

            xevem_deblock_cu_hor(pic, x, y, cuw, cuh >> 1, ctx->map_scu, ctx->map_refi, ctx->map_unrefined_mv, ctx->w_scu, ctx->log2_max_cuwh, ctx->refp, 0
                               , core->tree_cons, ctx->map_tidx, boundary_filtering, ctx->sps.tool_addb, mctx->map_ats_inter
                               , ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc, ctx->qp_chroma_dynamic);
            xevem_deblock_cu_hor(pic, x, y + MAX_TR_SIZE, cuw, cuh >> 1, ctx->map_scu, ctx->map_refi, ctx->map_unrefined_mv, ctx->w_scu, ctx->log2_max_cuwh, ctx->refp, 0
                               , core->tree_cons, ctx->map_tidx, boundary_filtering, ctx->sps.tool_addb, mctx->map_ats_inter
                               , ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc, ctx->qp_chroma_dynamic);
        }
        else
        {
            xevem_deblock_cu_hor(pic, x, y, cuw, cuh, ctx->map_scu, ctx->map_refi, ctx->map_unrefined_mv, ctx->w_scu, ctx->log2_max_cuwh, ctx->refp, 0
                               , core->tree_cons, ctx->map_tidx, boundary_filtering, ctx->sps.tool_addb, mctx->map_ats_inter
                               , ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc, ctx->qp_chroma_dynamic);
        }
    }
    else
    {
        if(cuw > MAX_TR_SIZE)
        {
            xevem_deblock_cu_ver(pic, x, y, cuw >> 1, cuh, ctx->map_scu, ctx->map_refi, ctx->map_unrefined_mv, ctx->w_scu, ctx->log2_max_cuwh
                               , ctx->map_cu_mode, ctx->refp, 0, core->tree_cons, ctx->map_tidx, boundary_filtering, ctx->sps.tool_addb, mctx->map_ats_inter
                               , ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc, ctx->qp_chroma_dynamic);
            xevem_deblock_cu_ver(pic, x + MAX_TR_SIZE, y, cuw >> 1, cuh, ctx->map_scu, ctx->map_refi, ctx->map_unrefined_mv, ctx->w_scu, ctx->log2_max_cuwh
                               , ctx->map_cu_mode, ctx->refp, 0, core->tree_cons, ctx->map_tidx, boundary_filtering, ctx->sps.tool_addb, mctx->map_ats_inter
                               , ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc, ctx->qp_chroma_dynamic);
        }
        else
        {
            xevem_deblock_cu_ver(pic, x, y, cuw, cuh, ctx->map_scu, ctx->map_refi, ctx->map_unrefined_mv, ctx->w_scu, ctx->log2_max_cuwh
                               , ctx->map_cu_mode, ctx->refp, 0, core->tree_cons, ctx->map_tidx, boundary_filtering, ctx->sps.tool_addb, mctx->map_ats_inter
                               , ctx->sps.bit_depth_luma_minus8 + 8, ctx->sps.bit_depth_chroma_minus8 + 8, ctx->sps.chroma_format_idc, ctx->qp_chroma_dynamic);
        }
    }
}


void xevem_deblock_tree(XEVE_CTX * ctx, XEVE_PIC * pic, int x, int y, int cuw, int cuh, int cud, int cup, int is_hor_edge
                      , TREE_CONS tree_cons, XEVE_CORE * core, int boundary_filtering)
{
    s8  split_mode;
    int lcu_num;
    s8  suco_flag = 0;

    core->tree_cons = tree_cons;

    pic->pic_deblock_alpha_offset = ctx->sh->sh_deblock_alpha_offset;
    pic->pic_deblock_beta_offset = ctx->sh->sh_deblock_beta_offset;
    pic->pic_qp_u_offset = ctx->sh->qp_u_offset;
    pic->pic_qp_v_offset = ctx->sh->qp_v_offset;

    lcu_num = (x >> ctx->log2_max_cuwh) + (y >> ctx->log2_max_cuwh) * ctx->w_lcu;
    xeve_get_split_mode(&split_mode, cud, cup, cuw, cuh, ctx->max_cuwh, ctx->map_cu_data[lcu_num].split_mode);
    xeve_get_suco_flag(&suco_flag, cud, cup, cuw, cuh, ctx->max_cuwh, ctx->map_cu_data[lcu_num].suco_flag);

    if(split_mode != NO_SPLIT)
    {
        XEVE_SPLIT_STRUCT split_struct;
        int suco_order[SPLIT_MAX_PART_COUNT];
        xeve_split_get_part_structure_main( split_mode, x, y, cuw, cuh, cup, cud, ctx->log2_culine, &split_struct );

        xeve_split_get_suco_order(suco_flag, split_mode, suco_order);

        split_struct.tree_cons = tree_cons;

        BOOL mode_cons_changed = FALSE;

        if ( ctx->sps.tool_admvp && ctx->sps.sps_btt_flag )
        {
            split_struct.tree_cons.changed = tree_cons.mode_cons == eAll && ctx->sps.chroma_format_idc != 0 && !xeve_is_chroma_split_allowed(cuw, cuh, split_mode);
            mode_cons_changed = xeve_signal_mode_cons(&core->tree_cons , &split_struct.tree_cons);
            if (mode_cons_changed)
            {
                MODE_CONS mode = xeve_derive_mode_cons(ctx, lcu_num, cup);
                xeve_set_tree_mode(&split_struct.tree_cons, mode);
            }
        }
        else
        {
            split_struct.tree_cons = xeve_get_default_tree_cons();
        }

        for(int part_num = 0; part_num < split_struct.part_count; ++part_num)
        {
            int cur_part_num = suco_order[part_num];
            int sub_cuw = split_struct.width[cur_part_num];
            int sub_cuh = split_struct.height[cur_part_num];
            int x_pos = split_struct.x_pos[cur_part_num];
            int y_pos = split_struct.y_pos[cur_part_num];

            if(x_pos < ctx->w && y_pos < ctx->h)
            {
                xevem_deblock_tree(ctx, pic, x_pos, y_pos, sub_cuw, sub_cuh, split_struct.cud[cur_part_num], split_struct.cup[cur_part_num], is_hor_edge
                                 , split_struct.tree_cons, core, boundary_filtering);
            }

            core->tree_cons = tree_cons;
        }

        if (mode_cons_changed && !xeve_check_all(split_struct.tree_cons))
        {
            core->tree_cons = split_struct.tree_cons;
            core->tree_cons.tree_type = TREE_C;
            split_mode = NO_SPLIT;
        }
    }

    if (split_mode == NO_SPLIT)
    {
        ctx->fn_deblock_unit(ctx, pic, x, y, cuw, cuh, is_hor_edge, core, boundary_filtering);
    }

    core->tree_cons = tree_cons;
}

int xevem_deblock(XEVE_CTX * ctx, XEVE_PIC * pic, int tile_idx, int filter_across_boundary , XEVE_CORE * core)
{
    int i, j;
    int x_l, x_r, y_l, y_r, l_scu, r_scu, t_scu, b_scu;
    u32 k1;
    int scu_in_lcu_wh = 1 << (ctx->log2_max_cuwh - MIN_CU_LOG2);
    int boundary_filtering = 0;
    x_l = (ctx->tile[tile_idx].ctba_rs_first) % ctx->w_lcu; //entry point lcu's x location
    y_l = (ctx->tile[tile_idx].ctba_rs_first) / ctx->w_lcu; // entry point lcu's y location
    x_r = x_l + ctx->tile[tile_idx].w_ctb;
    y_r = y_l + ctx->tile[tile_idx].h_ctb;
    l_scu = x_l * scu_in_lcu_wh;
    r_scu = XEVE_CLIP3(0, ctx->w_scu, x_r*scu_in_lcu_wh);
    t_scu = y_l * scu_in_lcu_wh;
    b_scu = XEVE_CLIP3(0, ctx->h_scu, y_r*scu_in_lcu_wh);

    for (j = t_scu; j < b_scu; j++)
    {
        for (i = l_scu; i < r_scu; i++)
        {
            k1 = i + j * ctx->w_scu;
            MCU_CLR_COD(ctx->map_scu[k1]);

            if (!MCU_GET_DMVRF(ctx->map_scu[k1]))
            {
                ctx->map_unrefined_mv[k1][REFP_0][MV_X] = ctx->map_mv[k1][REFP_0][MV_X];
                ctx->map_unrefined_mv[k1][REFP_0][MV_Y] = ctx->map_mv[k1][REFP_0][MV_Y];
                ctx->map_unrefined_mv[k1][REFP_1][MV_X] = ctx->map_mv[k1][REFP_1][MV_X];
                ctx->map_unrefined_mv[k1][REFP_1][MV_Y] = ctx->map_mv[k1][REFP_1][MV_Y];
            }
        }
    }

    /* horizontal filtering */
    for (j = y_l; j < y_r; j++)
    {
        for (i = x_l; i < x_r; i++)
        {
            ctx->fn_deblock_tree(ctx, pic, (i << ctx->log2_max_cuwh), (j << ctx->log2_max_cuwh), ctx->max_cuwh, ctx->max_cuwh, 0, 0, core->deblock_is_hor
                               , xeve_get_default_tree_cons(), core, filter_across_boundary);
        }
    }

    return XEVE_OK;
}

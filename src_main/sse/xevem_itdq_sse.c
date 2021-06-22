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

#include <math.h>
#include "xevem_type.h"

#if X86_SSE
#define MAC_8PEL_MEM(src1, src2, m01, m02, m03, m04, mac) \
    m01 = _mm_loadu_si128((__m128i*)(src1)); \
    m02 = _mm_loadu_si128((__m128i*)(src2)); \
    \
    m03 = _mm_cvtepi16_epi32(m01); \
    m04 = _mm_cvtepi16_epi32(m02); \
    \
    m03 = _mm_mullo_epi32(m03, m04); \
    \
    m01 = _mm_srli_si128(m01, 8); \
    m02 = _mm_srli_si128(m02, 8); \
    \
    m01 = _mm_cvtepi16_epi32(m01); \
    m02 = _mm_cvtepi16_epi32(m02); \
    \
    m04 = _mm_mullo_epi32(m01, m02); \
    \
    mac = _mm_add_epi32(mac, m03); \
    mac = _mm_add_epi32(mac, m04);

#define MAC_8PEL_REG(mcoef, src2, mac) \
    mac = _mm_add_epi32(mac,  _mm_madd_epi16(mcoef, \
          _mm_cvtepi8_epi16(_mm_loadu_si128((__m128i*)(src2)))));

#define MAC_LINE(idx, w, mcoef, src2, mac, mtot, lane) \
    mac = _mm_setzero_si128(); \
    for (idx = 0; idx<((w)>>3); idx++) \
    { \
        MAC_8PEL_REG(mcoef[idx], src2 + (idx<<3), mac); \
    } \
    mac = _mm_hadd_epi32(mac, mac); \
    mac = _mm_hadd_epi32(mac, mac); \
    mtot = _mm_insert_epi32(mtot, _mm_extract_epi32(mac, 0), lane);

/* 32bit in xmm to 16bit clip with round-off */
#define ADD_SHIFT_CLIP_S32_TO_S16_4PEL(mval, madd, shift) \
    mval = _mm_srai_epi32(_mm_add_epi32(mval, madd), shift); \
    mval = _mm_packs_epi32(mval, mval);

/* top macro for inverse transforms */
#define ITX_MATRIX(coef, blk, tsize, line, shift, itm_tbl, skip_line) \
{\
    int i, j, k, h, w; \
    const s8 *itm; \
    s16 * c; \
\
    __m128i mc[8]; \
    __m128i mac, mtot=_mm_setzero_si128(), madd; \
\
    if(skip_line) \
    { \
        h = line - skip_line; \
        w = tsize; \
    } \
    else  \
    { \
        h = line; \
        w = tsize; \
    } \
\
    madd = _mm_set1_epi32(1 << (shift - 1)); \
\
    for (i = 0; i<h; i++) \
    { \
        itm = (itm_tbl); \
        c = coef + i; \
\
        for (k = 0; k<(w>>3); k++) \
        { \
            mc[k] = _mm_setr_epi16(c[0], \
                c[(1)*line], \
                c[(2)*line], \
                c[(3)*line], \
                c[(4)*line], \
                c[(5)*line], \
                c[(6)*line], \
                c[(7)*line]); \
            c += line << 3; \
        } \
\
        for (j = 0; j<(tsize>>2); j++) \
        { \
            MAC_LINE(k, w, mc, itm, mac, mtot, 0); \
            itm += tsize; \
\
            MAC_LINE(k, w, mc, itm, mac, mtot, 1); \
            itm += tsize; \
\
            MAC_LINE(k, w, mc, itm, mac, mtot, 2); \
            itm += tsize; \
\
            MAC_LINE(k, w, mc, itm, mac, mtot, 3); \
            itm += tsize; \
\
            ADD_SHIFT_CLIP_S32_TO_S16_4PEL(mtot, madd, shift); \
            _mm_storel_epi64((__m128i*)(blk + (j<<2)), mtot); \
        } \
        blk += tsize; \
    } \
}

const INV_TRANS xeve_itrans_map_tbl_sse[16][5] =
{
    { NULL, xeve_itrans_ats_intra_DCT8_B4, xeve_itrans_ats_intra_DCT8_B8_sse, xeve_itrans_ats_intra_DCT8_B16_sse, xeve_itrans_ats_intra_DCT8_B32_sse },
    { NULL, xeve_itrans_ats_intra_DST7_B4, xeve_itrans_ats_intra_DST7_B8_sse, xeve_itrans_ats_intra_DST7_B16_sse, xeve_itrans_ats_intra_DST7_B32_sse },
};


void xeve_itrans_ats_intra_DST7_B8_sse(s16 *coef, s16 *block, int shift, int line, int skip_line, int skip_line_2)
{
    ITX_MATRIX(coef, block, 8, line, shift, xevem_tbl_inv_tr[DST7][1], skip_line);

    if (skip_line)
    {
        xeve_mset(block, 0, (skip_line << 3) * sizeof(s16));
    }
}

void xeve_itrans_ats_intra_DST7_B16_sse(s16 *coef, s16 *block, int shift, int line, int skip_line, int skip_line_2)
{
    ITX_MATRIX(coef, block, 16, line, shift, xevem_tbl_inv_tr[DST7][2], skip_line);

    if (skip_line)
    {
        xeve_mset(block, 0, (skip_line << 4) * sizeof(s16));
    }
}

void xeve_itrans_ats_intra_DST7_B32_sse(s16 *coef, s16 *block, int shift, int line, int skip_line, int skip_line_2)
{
    ITX_MATRIX(coef, block, 32, line, shift, xevem_tbl_inv_tr[DST7][3], skip_line);

    if (skip_line)
    {
        xeve_mset(block, 0, (skip_line << 5) * sizeof(s16));
    }
}

void xeve_itrans_ats_intra_DCT8_B8_sse(s16 *coef, s16 *block, int shift, int line, int skip_line, int skip_line_2)
{
    ITX_MATRIX(coef, block, 8, line, shift, xevem_tbl_inv_tr[DCT8][1], skip_line);

    if (skip_line)
    {
        xeve_mset(block, 0, (skip_line << 3) * sizeof(s16));
    }
}

void xeve_itrans_ats_intra_DCT8_B16_sse(s16 *coef, s16 *block, int shift, int line, int skip_line, int skip_line_2)
{
    ITX_MATRIX(coef, block, 16, line, shift, xevem_tbl_inv_tr[DCT8][2], skip_line);

    if (skip_line)
    {
        xeve_mset(block, 0, (skip_line << 4) * sizeof(s16));
    }
}

void xeve_itrans_ats_intra_DCT8_B32_sse(s16 *coef, s16 *block, int shift, int line, int skip_line, int skip_line_2)
{
    ITX_MATRIX(coef, block, 32, line, shift, xevem_tbl_inv_tr[DCT8][3], skip_line);

    if (skip_line)
    {
        xeve_mset(block, 0, (skip_line << 5) * sizeof(s16));
    }
}

#endif /* X86_SSE */

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

#include "xeve_type.h"
#include <math.h>


const XEVE_FN_SAD  (* xeve_func_sad)[8];
const XEVE_FN_SSD  (* xeve_func_ssd)[8];
const XEVE_FN_DIFF (* xeve_func_diff)[8];
const XEVE_FN_SATD  * xeve_func_satd;

/* SAD for 16bit **************************************************************/
int sad_16b(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    u16 *s1;
    s16 *s2;

    int i, j, sad;

    s1 = (u16 *)src1;
    s2 = (s16 *)src2;

    sad = 0;

    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            sad += XEVE_ABS16((s16)s1[j] - (s16)s2[j]);          
        }
        s1 += s_src1;
        s2 += s_src2;
    }

    return (sad >> (bit_depth - 8));
}

/* index: [log2 of width][log2 of height] */
const XEVE_FN_SAD xeve_tbl_sad_16b[8][8] =
{
    /* width == 1 */
    {
        sad_16b, /* height == 1 */
        sad_16b, /* height == 2 */
        sad_16b, /* height == 4 */
        sad_16b, /* height == 8 */
        sad_16b, /* height == 16 */
        sad_16b, /* height == 32 */
        sad_16b, /* height == 64 */
        sad_16b, /* height == 128 */
    },
    /* width == 2 */
    {
        sad_16b, /* height == 1 */
        sad_16b, /* height == 2 */
        sad_16b, /* height == 4 */
        sad_16b, /* height == 8 */
        sad_16b, /* height == 16 */
        sad_16b, /* height == 32 */
        sad_16b, /* height == 64 */
        sad_16b, /* height == 128 */
    },
    /* width == 4 */
    {
        sad_16b, /* height == 1 */
        sad_16b, /* height == 2 */
        sad_16b, /* height == 4 */
        sad_16b, /* height == 8 */
        sad_16b, /* height == 16 */
        sad_16b, /* height == 32 */
        sad_16b, /* height == 64 */
        sad_16b, /* height == 128 */
    },
    /* width == 8 */
    {
        sad_16b, /* height == 1 */
        sad_16b, /* height == 2 */
        sad_16b, /* height == 4 */
        sad_16b, /* height == 8 */
        sad_16b, /* height == 16 */
        sad_16b, /* height == 32 */
        sad_16b, /* height == 64 */
        sad_16b, /* height == 128 */
    },
    /* width == 16 */
    {
        sad_16b, /* height == 1 */
        sad_16b, /* height == 2 */
        sad_16b, /* height == 4 */
        sad_16b, /* height == 8 */
        sad_16b, /* height == 16 */
        sad_16b, /* height == 32 */
        sad_16b, /* height == 64 */
        sad_16b, /* height == 128 */
    },
    /* width == 32 */
    {
        sad_16b, /* height == 1 */
        sad_16b, /* height == 2 */
        sad_16b, /* height == 4 */
        sad_16b, /* height == 8 */
        sad_16b, /* height == 16 */
        sad_16b, /* height == 32 */
        sad_16b, /* height == 64 */
        sad_16b, /* height == 128 */
    },
    /* width == 64 */
    {
        sad_16b, /* height == 1 */
        sad_16b, /* height == 2 */
        sad_16b, /* height == 4 */
        sad_16b, /* height == 8 */
        sad_16b, /* height == 16 */
        sad_16b, /* height == 32 */
        sad_16b, /* height == 64 */
        sad_16b, /* height == 128 */
    },
    /* width == 128 */
    {
        sad_16b, /* height == 1 */
        sad_16b, /* height == 2 */
        sad_16b, /* height == 4 */
        sad_16b, /* height == 8 */
        sad_16b, /* height == 16 */
        sad_16b, /* height == 32 */
        sad_16b, /* height == 64 */
        sad_16b, /* height == 128 */
    }
};

/* DIFF **********************************************************************/
void diff_16b(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int s_diff, s16 * diff, int bit_depth)
{
    s16 *s1;
    s16 *s2;

    int i, j;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            diff[j] = (s16)s1[j] - (s16)s2[j];
        }
        diff += s_diff;
        s1 += s_src1;
        s2 += s_src2;
    }
}

const XEVE_FN_DIFF xeve_tbl_diff_16b[8][8] =
{
    /* width == 1 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 2 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 4 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 8 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 16 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 32 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 64 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    },
    /* width == 128 */
    {
        diff_16b, /* height == 1 */
        diff_16b, /* height == 2 */
        diff_16b, /* height == 4 */
        diff_16b, /* height == 8 */
        diff_16b, /* height == 16 */
        diff_16b, /* height == 32 */
        diff_16b, /* height == 64 */
        diff_16b, /* height == 128 */
    }
};

/* SSD ***********************************************************************/
s64 ssd_16b(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    s16 * s1;
    s16 * s2;
    int     i, j, diff;
    s64   ssd;
    const int shift = (bit_depth - 8) << 1;

    s1 = (s16 *)src1;
    s2 = (s16 *)src2;

    ssd = 0;

    for(i = 0; i < h; i++)
    {
        for(j = 0; j < w; j++)
        {
            diff = s1[j] - s2[j];
            ssd += (diff * diff) >> shift;
        }
        s1 += s_src1;
        s2 += s_src2;
    }
    return ssd;
}

const XEVE_FN_SSD xeve_tbl_ssd_16b[8][8] =
{
    /* width == 1 */
    {
        ssd_16b, /* height == 1 */
        ssd_16b, /* height == 2 */
        ssd_16b, /* height == 4 */
        ssd_16b, /* height == 8 */
        ssd_16b, /* height == 16 */
        ssd_16b, /* height == 32 */
        ssd_16b, /* height == 64 */
        ssd_16b, /* height == 128 */
    },
    /* width == 2 */
    {
        ssd_16b, /* height == 1 */
        ssd_16b, /* height == 2 */
        ssd_16b, /* height == 4 */
        ssd_16b, /* height == 8 */
        ssd_16b, /* height == 16 */
        ssd_16b, /* height == 32 */
        ssd_16b, /* height == 64 */
        ssd_16b, /* height == 128 */
    },
    /* width == 4 */
    {
        ssd_16b, /* height == 1 */
        ssd_16b, /* height == 2 */
        ssd_16b, /* height == 4 */
        ssd_16b, /* height == 8 */
        ssd_16b, /* height == 16 */
        ssd_16b, /* height == 32 */
        ssd_16b, /* height == 64 */
        ssd_16b, /* height == 128 */
    },
    /* width == 8 */
    {
        ssd_16b, /* height == 1 */
        ssd_16b, /* height == 2 */
        ssd_16b, /* height == 4 */
        ssd_16b, /* height == 8 */
        ssd_16b, /* height == 16 */
        ssd_16b, /* height == 32 */
        ssd_16b, /* height == 64 */
        ssd_16b, /* height == 128 */
    },
    /* width == 16 */
    {
        ssd_16b, /* height == 1 */
        ssd_16b, /* height == 2 */
        ssd_16b, /* height == 4 */
        ssd_16b, /* height == 8 */
        ssd_16b, /* height == 16 */
        ssd_16b, /* height == 32 */
        ssd_16b, /* height == 64 */
        ssd_16b, /* height == 128 */
    },
    /* width == 32 */
    {
        ssd_16b, /* height == 1 */
        ssd_16b, /* height == 2 */
        ssd_16b, /* height == 4 */
        ssd_16b, /* height == 8 */
        ssd_16b, /* height == 16 */
        ssd_16b, /* height == 32 */
        ssd_16b, /* height == 64 */
        ssd_16b, /* height == 128 */
    },
    /* width == 64 */
    {
        ssd_16b, /* height == 1 */
        ssd_16b, /* height == 2 */
        ssd_16b, /* height == 4 */
        ssd_16b, /* height == 8 */
        ssd_16b, /* height == 16 */
        ssd_16b, /* height == 32 */
        ssd_16b, /* height == 64 */
        ssd_16b, /* height == 128 */
    },
    /* width == 128 */
    {
        ssd_16b, /* height == 1 */
        ssd_16b, /* height == 2 */
        ssd_16b, /* height == 4 */
        ssd_16b, /* height == 8 */
        ssd_16b, /* height == 16 */
        ssd_16b, /* height == 32 */
        ssd_16b, /* height == 64 */
        ssd_16b, /* height == 128 */
    }
};

/* SATD **********************************************************************/
int xeve_had_2x2(pel *org, pel *cur, int s_org, int s_cur, int step)
{
    int satd = 0;
    int sub[4], interm[4];
    pel *orgn = org, *curn = cur;

    sub[0] = orgn[0        ] - curn[0        ];
    sub[1] = orgn[1        ] - curn[1        ];
    sub[2] = orgn[s_org    ] - curn[0 + s_cur];
    sub[3] = orgn[s_org + 1] - curn[1 + s_cur];
    interm[0] = sub[0] + sub[2];
    interm[1] = sub[1] + sub[3];
    interm[2] = sub[0] - sub[2];
    interm[3] = sub[1] - sub[3];
    satd = (XEVE_ABS(interm[0] + interm[1]) >> 2);
    satd += XEVE_ABS(interm[0] - interm[1]);
    satd += XEVE_ABS(interm[2] + interm[3]);
    satd += XEVE_ABS(interm[2] - interm[3]);

    return satd;
}

int xeve_had_4x4(pel *org, pel *cur, int s_org, int s_cur, int step, int bit_depth)
{
int k;
    int satd = 0;
    int sub[16], interm1[16], interm2[16];
    pel * orgn = org, *curn = cur;

    for( k = 0; k < 16; k+=4 )
    {
        sub[k+0] = orgn[0] - curn[0];
        sub[k+1] = orgn[1] - curn[1];
        sub[k+2] = orgn[2] - curn[2];
        sub[k+3] = orgn[3] - curn[3];

        curn += s_cur;
        orgn += s_org;
    }

    interm1[ 0] = sub[ 0] + sub[12];
    interm1[ 1] = sub[ 1] + sub[13];
    interm1[ 2] = sub[ 2] + sub[14];
    interm1[ 3] = sub[ 3] + sub[15];
    interm1[ 4] = sub[ 4] + sub[ 8];
    interm1[ 5] = sub[ 5] + sub[ 9];
    interm1[ 6] = sub[ 6] + sub[10];
    interm1[ 7] = sub[ 7] + sub[11];
    interm1[ 8] = sub[ 4] - sub[ 8];
    interm1[ 9] = sub[ 5] - sub[ 9];
    interm1[10] = sub[ 6] - sub[10];
    interm1[11] = sub[ 7] - sub[11];
    interm1[12] = sub[ 0] - sub[12];
    interm1[13] = sub[ 1] - sub[13];
    interm1[14] = sub[ 2] - sub[14];
    interm1[15] = sub[ 3] - sub[15];

    interm2[ 0] = interm1[ 0] + interm1[ 4];
    interm2[ 1] = interm1[ 1] + interm1[ 5];
    interm2[ 2] = interm1[ 2] + interm1[ 6];
    interm2[ 3] = interm1[ 3] + interm1[ 7];
    interm2[ 4] = interm1[ 8] + interm1[12];
    interm2[ 5] = interm1[ 9] + interm1[13];
    interm2[ 6] = interm1[10] + interm1[14];
    interm2[ 7] = interm1[11] + interm1[15];
    interm2[ 8] = interm1[ 0] - interm1[ 4];
    interm2[ 9] = interm1[ 1] - interm1[ 5];
    interm2[10] = interm1[ 2] - interm1[ 6];
    interm2[11] = interm1[ 3] - interm1[ 7];
    interm2[12] = interm1[12] - interm1[ 8];
    interm2[13] = interm1[13] - interm1[ 9];
    interm2[14] = interm1[14] - interm1[10];
    interm2[15] = interm1[15] - interm1[11];

    interm1[ 0] = interm2[ 0] + interm2[ 3];
    interm1[ 1] = interm2[ 1] + interm2[ 2];
    interm1[ 2] = interm2[ 1] - interm2[ 2];
    interm1[ 3] = interm2[ 0] - interm2[ 3];
    interm1[ 4] = interm2[ 4] + interm2[ 7];
    interm1[ 5] = interm2[ 5] + interm2[ 6];
    interm1[ 6] = interm2[ 5] - interm2[ 6];
    interm1[ 7] = interm2[ 4] - interm2[ 7];
    interm1[ 8] = interm2[ 8] + interm2[11];
    interm1[ 9] = interm2[ 9] + interm2[10];
    interm1[10] = interm2[ 9] - interm2[10];
    interm1[11] = interm2[ 8] - interm2[11];
    interm1[12] = interm2[12] + interm2[15];
    interm1[13] = interm2[13] + interm2[14];
    interm1[14] = interm2[13] - interm2[14];
    interm1[15] = interm2[12] - interm2[15];

    interm2[ 0] = XEVE_ABS(interm1[ 0] + interm1[ 1]);
    interm2[ 1] = XEVE_ABS(interm1[ 0] - interm1[ 1]);
    interm2[ 2] = XEVE_ABS(interm1[ 2] + interm1[ 3]);
    interm2[ 3] = XEVE_ABS(interm1[ 3] - interm1[ 2]);
    interm2[ 4] = XEVE_ABS(interm1[ 4] + interm1[ 5]);
    interm2[ 5] = XEVE_ABS(interm1[ 4] - interm1[ 5]);
    interm2[ 6] = XEVE_ABS(interm1[ 6] + interm1[ 7]);
    interm2[ 7] = XEVE_ABS(interm1[ 7] - interm1[ 6]);
    interm2[ 8] = XEVE_ABS(interm1[ 8] + interm1[ 9]);
    interm2[ 9] = XEVE_ABS(interm1[ 8] - interm1[ 9]);
    interm2[10] = XEVE_ABS(interm1[10] + interm1[11]);
    interm2[11] = XEVE_ABS(interm1[11] - interm1[10]);
    interm2[12] = XEVE_ABS(interm1[12] + interm1[13]);
    interm2[13] = XEVE_ABS(interm1[12] - interm1[13]);
    interm2[14] = XEVE_ABS(interm1[14] + interm1[15]);
    interm2[15] = XEVE_ABS(interm1[15] - interm1[14]);

    satd = interm2[0] >> 2;
    for (k = 1; k < 16; k++)
    {
        satd += interm2[k];
    }
    satd = ((satd + 1) >> 1);

    return satd;
}

int xeve_had_8x8(pel *org, pel *cur, int s_org, int s_cur, int step, int bit_depth)
    {
    int k, i, j, jj;
    int satd = 0;
    int sub[64], interm1[8][8], interm2[8][8], interm3[8][8];
    pel * orgn = org, *curn = cur;

    for(k = 0; k < 64; k += 8)
    {
        sub[k+0] = orgn[0] - curn[0];
        sub[k+1] = orgn[1] - curn[1];
        sub[k+2] = orgn[2] - curn[2];
        sub[k+3] = orgn[3] - curn[3];
        sub[k+4] = orgn[4] - curn[4];
        sub[k+5] = orgn[5] - curn[5];
        sub[k+6] = orgn[6] - curn[6];
        sub[k+7] = orgn[7] - curn[7];

        curn += s_cur;
        orgn += s_org;
    }

    /* horizontal */
    for(j = 0; j < 8; j++)
    {
        jj = j << 3;
        interm2[j][0] = sub[jj  ] + sub[jj+4];
        interm2[j][1] = sub[jj+1] + sub[jj+5];
        interm2[j][2] = sub[jj+2] + sub[jj+6];
        interm2[j][3] = sub[jj+3] + sub[jj+7];
        interm2[j][4] = sub[jj  ] - sub[jj+4];
        interm2[j][5] = sub[jj+1] - sub[jj+5];
        interm2[j][6] = sub[jj+2] - sub[jj+6];
        interm2[j][7] = sub[jj+3] - sub[jj+7];

        interm1[j][0] = interm2[j][0] + interm2[j][2];
        interm1[j][1] = interm2[j][1] + interm2[j][3];
        interm1[j][2] = interm2[j][0] - interm2[j][2];
        interm1[j][3] = interm2[j][1] - interm2[j][3];
        interm1[j][4] = interm2[j][4] + interm2[j][6];
        interm1[j][5] = interm2[j][5] + interm2[j][7];
        interm1[j][6] = interm2[j][4] - interm2[j][6];
        interm1[j][7] = interm2[j][5] - interm2[j][7];

        interm2[j][0] = interm1[j][0] + interm1[j][1];
        interm2[j][1] = interm1[j][0] - interm1[j][1];
        interm2[j][2] = interm1[j][2] + interm1[j][3];
        interm2[j][3] = interm1[j][2] - interm1[j][3];
        interm2[j][4] = interm1[j][4] + interm1[j][5];
        interm2[j][5] = interm1[j][4] - interm1[j][5];
        interm2[j][6] = interm1[j][6] + interm1[j][7];
        interm2[j][7] = interm1[j][6] - interm1[j][7];
    }

    /* vertical */
    for(i = 0; i < 8; i++)
    {
        interm3[0][i] = interm2[0][i] + interm2[4][i];
        interm3[1][i] = interm2[1][i] + interm2[5][i];
        interm3[2][i] = interm2[2][i] + interm2[6][i];
        interm3[3][i] = interm2[3][i] + interm2[7][i];
        interm3[4][i] = interm2[0][i] - interm2[4][i];
        interm3[5][i] = interm2[1][i] - interm2[5][i];
        interm3[6][i] = interm2[2][i] - interm2[6][i];
        interm3[7][i] = interm2[3][i] - interm2[7][i];

        interm1[0][i] = interm3[0][i] + interm3[2][i];
        interm1[1][i] = interm3[1][i] + interm3[3][i];
        interm1[2][i] = interm3[0][i] - interm3[2][i];
        interm1[3][i] = interm3[1][i] - interm3[3][i];
        interm1[4][i] = interm3[4][i] + interm3[6][i];
        interm1[5][i] = interm3[5][i] + interm3[7][i];
        interm1[6][i] = interm3[4][i] - interm3[6][i];
        interm1[7][i] = interm3[5][i] - interm3[7][i];

        interm2[0][i] = XEVE_ABS(interm1[0][i] + interm1[1][i]);
        interm2[1][i] = XEVE_ABS(interm1[0][i] - interm1[1][i]);
        interm2[2][i] = XEVE_ABS(interm1[2][i] + interm1[3][i]);
        interm2[3][i] = XEVE_ABS(interm1[2][i] - interm1[3][i]);
        interm2[4][i] = XEVE_ABS(interm1[4][i] + interm1[5][i]);
        interm2[5][i] = XEVE_ABS(interm1[4][i] - interm1[5][i]);
        interm2[6][i] = XEVE_ABS(interm1[6][i] + interm1[7][i]);
        interm2[7][i] = XEVE_ABS(interm1[6][i] - interm1[7][i]);
    }

    satd = interm2[0][0]>> 2;
    for (j = 1; j < 8; j++)
    {
        satd += interm2[0][j];
    }
    for (i = 1; i < 8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            satd += interm2[i][j];
        }
    }

    satd = ((satd + 2) >> 2);

    return satd;
}

int xeve_had_16x8(pel *org, pel *cur, int s_org, int s_cur, int step, int bit_depth)
{
    int k, i, j, jj;
    int satd = 0;
    int sub[128], interm1[8][16], interm2[8][16];
    pel * orgn = org, *curn = cur;

    for(k = 0; k < 128; k += 16)
    {
        sub[k + 0] = orgn[0] - curn[0];
        sub[k + 1] = orgn[1] - curn[1];
        sub[k + 2] = orgn[2] - curn[2];
        sub[k + 3] = orgn[3] - curn[3];
        sub[k + 4] = orgn[4] - curn[4];
        sub[k + 5] = orgn[5] - curn[5];
        sub[k + 6] = orgn[6] - curn[6];
        sub[k + 7] = orgn[7] - curn[7];

        sub[k + 8]  = orgn[8]  - curn[8];
        sub[k + 9]  = orgn[9]  - curn[9];
        sub[k + 10] = orgn[10] - curn[10];
        sub[k + 11] = orgn[11] - curn[11];
        sub[k + 12] = orgn[12] - curn[12];
        sub[k + 13] = orgn[13] - curn[13];
        sub[k + 14] = orgn[14] - curn[14];
        sub[k + 15] = orgn[15] - curn[15];

        curn += s_cur;
        orgn += s_org;
    }

    for(j = 0; j < 8; j++)
    {
        jj = j << 4;

        interm2[j][0] = sub[jj] + sub[jj + 8];
        interm2[j][1] = sub[jj + 1] + sub[jj + 9];
        interm2[j][2] = sub[jj + 2] + sub[jj + 10];
        interm2[j][3] = sub[jj + 3] + sub[jj + 11];
        interm2[j][4] = sub[jj + 4] + sub[jj + 12];
        interm2[j][5] = sub[jj + 5] + sub[jj + 13];
        interm2[j][6] = sub[jj + 6] + sub[jj + 14];
        interm2[j][7] = sub[jj + 7] + sub[jj + 15];
        interm2[j][8] = sub[jj] - sub[jj + 8];
        interm2[j][9] = sub[jj + 1] - sub[jj + 9];
        interm2[j][10] = sub[jj + 2] - sub[jj + 10];
        interm2[j][11] = sub[jj + 3] - sub[jj + 11];
        interm2[j][12] = sub[jj + 4] - sub[jj + 12];
        interm2[j][13] = sub[jj + 5] - sub[jj + 13];
        interm2[j][14] = sub[jj + 6] - sub[jj + 14];
        interm2[j][15] = sub[jj + 7] - sub[jj + 15];

        interm1[j][0] = interm2[j][0] + interm2[j][4];
        interm1[j][1] = interm2[j][1] + interm2[j][5];
        interm1[j][2] = interm2[j][2] + interm2[j][6];
        interm1[j][3] = interm2[j][3] + interm2[j][7];
        interm1[j][4] = interm2[j][0] - interm2[j][4];
        interm1[j][5] = interm2[j][1] - interm2[j][5];
        interm1[j][6] = interm2[j][2] - interm2[j][6];
        interm1[j][7] = interm2[j][3] - interm2[j][7];
        interm1[j][8] = interm2[j][8] + interm2[j][12];
        interm1[j][9] = interm2[j][9] + interm2[j][13];
        interm1[j][10] = interm2[j][10] + interm2[j][14];
        interm1[j][11] = interm2[j][11] + interm2[j][15];
        interm1[j][12] = interm2[j][8] - interm2[j][12];
        interm1[j][13] = interm2[j][9] - interm2[j][13];
        interm1[j][14] = interm2[j][10] - interm2[j][14];
        interm1[j][15] = interm2[j][11] - interm2[j][15];

        interm2[j][0] = interm1[j][0] + interm1[j][2];
        interm2[j][1] = interm1[j][1] + interm1[j][3];
        interm2[j][2] = interm1[j][0] - interm1[j][2];
        interm2[j][3] = interm1[j][1] - interm1[j][3];
        interm2[j][4] = interm1[j][4] + interm1[j][6];
        interm2[j][5] = interm1[j][5] + interm1[j][7];
        interm2[j][6] = interm1[j][4] - interm1[j][6];
        interm2[j][7] = interm1[j][5] - interm1[j][7];
        interm2[j][8] = interm1[j][8] + interm1[j][10];
        interm2[j][9] = interm1[j][9] + interm1[j][11];
        interm2[j][10] = interm1[j][8] - interm1[j][10];
        interm2[j][11] = interm1[j][9] - interm1[j][11];
        interm2[j][12] = interm1[j][12] + interm1[j][14];
        interm2[j][13] = interm1[j][13] + interm1[j][15];
        interm2[j][14] = interm1[j][12] - interm1[j][14];
        interm2[j][15] = interm1[j][13] - interm1[j][15];

        interm1[j][0] = interm2[j][0] + interm2[j][1];
        interm1[j][1] = interm2[j][0] - interm2[j][1];
        interm1[j][2] = interm2[j][2] + interm2[j][3];
        interm1[j][3] = interm2[j][2] - interm2[j][3];
        interm1[j][4] = interm2[j][4] + interm2[j][5];
        interm1[j][5] = interm2[j][4] - interm2[j][5];
        interm1[j][6] = interm2[j][6] + interm2[j][7];
        interm1[j][7] = interm2[j][6] - interm2[j][7];
        interm1[j][8] = interm2[j][8] + interm2[j][9];
        interm1[j][9] = interm2[j][8] - interm2[j][9];
        interm1[j][10] = interm2[j][10] + interm2[j][11];
        interm1[j][11] = interm2[j][10] - interm2[j][11];
        interm1[j][12] = interm2[j][12] + interm2[j][13];
        interm1[j][13] = interm2[j][12] - interm2[j][13];
        interm1[j][14] = interm2[j][14] + interm2[j][15];
        interm1[j][15] = interm2[j][14] - interm2[j][15];
    }

    for(i = 0; i < 16; i++)
    {
        interm2[0][i] = interm1[0][i] + interm1[4][i];
        interm2[1][i] = interm1[1][i] + interm1[5][i];
        interm2[2][i] = interm1[2][i] + interm1[6][i];
        interm2[3][i] = interm1[3][i] + interm1[7][i];
        interm2[4][i] = interm1[0][i] - interm1[4][i];
        interm2[5][i] = interm1[1][i] - interm1[5][i];
        interm2[6][i] = interm1[2][i] - interm1[6][i];
        interm2[7][i] = interm1[3][i] - interm1[7][i];

        interm1[0][i] = interm2[0][i] + interm2[2][i];
        interm1[1][i] = interm2[1][i] + interm2[3][i];
        interm1[2][i] = interm2[0][i] - interm2[2][i];
        interm1[3][i] = interm2[1][i] - interm2[3][i];
        interm1[4][i] = interm2[4][i] + interm2[6][i];
        interm1[5][i] = interm2[5][i] + interm2[7][i];
        interm1[6][i] = interm2[4][i] - interm2[6][i];
        interm1[7][i] = interm2[5][i] - interm2[7][i];

        interm2[0][i] = XEVE_ABS(interm1[0][i] + interm1[1][i]);
        interm2[1][i] = XEVE_ABS(interm1[0][i] - interm1[1][i]);
        interm2[2][i] = XEVE_ABS(interm1[2][i] + interm1[3][i]);
        interm2[3][i] = XEVE_ABS(interm1[2][i] - interm1[3][i]);
        interm2[4][i] = XEVE_ABS(interm1[4][i] + interm1[5][i]);
        interm2[5][i] = XEVE_ABS(interm1[4][i] - interm1[5][i]);
        interm2[6][i] = XEVE_ABS(interm1[6][i] + interm1[7][i]);
        interm2[7][i] = XEVE_ABS(interm1[6][i] - interm1[7][i]);
    }

    satd = interm2[0][0] >> 2;
    for (j = 1; j < 16; j++)
    {
        satd += interm2[0][j];
    }
    for (i = 1; i < 8; i++)
    {
        for (j = 0; j < 16; j++)
        {
            satd += interm2[i][j];
        }
    }

    satd = (int)(satd / (2.0*sqrt(8.0)));

    return satd;
}

int xeve_had_8x16(pel *org, pel *cur, int s_org, int s_cur, int step, int bit_depth)
{
    int k, i, j, jj;
    int satd = 0;
    int sub[128], interm1[16][8], interm2[16][8];
    pel * curn = cur, *orgn = org;

    for(k = 0; k < 128; k += 8)
    {
        sub[k + 0] = orgn[0] - curn[0];
        sub[k + 1] = orgn[1] - curn[1];
        sub[k + 2] = orgn[2] - curn[2];
        sub[k + 3] = orgn[3] - curn[3];
        sub[k + 4] = orgn[4] - curn[4];
        sub[k + 5] = orgn[5] - curn[5];
        sub[k + 6] = orgn[6] - curn[6];
        sub[k + 7] = orgn[7] - curn[7];

        curn += s_cur;
        orgn += s_org;
    }

    for(j = 0; j < 16; j++)
    {
        jj = j << 3;

        interm2[j][0] = sub[jj] + sub[jj + 4];
        interm2[j][1] = sub[jj + 1] + sub[jj + 5];
        interm2[j][2] = sub[jj + 2] + sub[jj + 6];
        interm2[j][3] = sub[jj + 3] + sub[jj + 7];
        interm2[j][4] = sub[jj] - sub[jj + 4];
        interm2[j][5] = sub[jj + 1] - sub[jj + 5];
        interm2[j][6] = sub[jj + 2] - sub[jj + 6];
        interm2[j][7] = sub[jj + 3] - sub[jj + 7];

        interm1[j][0] = interm2[j][0] + interm2[j][2];
        interm1[j][1] = interm2[j][1] + interm2[j][3];
        interm1[j][2] = interm2[j][0] - interm2[j][2];
        interm1[j][3] = interm2[j][1] - interm2[j][3];
        interm1[j][4] = interm2[j][4] + interm2[j][6];
        interm1[j][5] = interm2[j][5] + interm2[j][7];
        interm1[j][6] = interm2[j][4] - interm2[j][6];
        interm1[j][7] = interm2[j][5] - interm2[j][7];

        interm2[j][0] = interm1[j][0] + interm1[j][1];
        interm2[j][1] = interm1[j][0] - interm1[j][1];
        interm2[j][2] = interm1[j][2] + interm1[j][3];
        interm2[j][3] = interm1[j][2] - interm1[j][3];
        interm2[j][4] = interm1[j][4] + interm1[j][5];
        interm2[j][5] = interm1[j][4] - interm1[j][5];
        interm2[j][6] = interm1[j][6] + interm1[j][7];
        interm2[j][7] = interm1[j][6] - interm1[j][7];
    }

    for(i = 0; i < 8; i++)
    {
        interm1[0][i] = interm2[0][i] + interm2[8][i];
        interm1[1][i] = interm2[1][i] + interm2[9][i];
        interm1[2][i] = interm2[2][i] + interm2[10][i];
        interm1[3][i] = interm2[3][i] + interm2[11][i];
        interm1[4][i] = interm2[4][i] + interm2[12][i];
        interm1[5][i] = interm2[5][i] + interm2[13][i];
        interm1[6][i] = interm2[6][i] + interm2[14][i];
        interm1[7][i] = interm2[7][i] + interm2[15][i];
        interm1[8][i] = interm2[0][i] - interm2[8][i];
        interm1[9][i] = interm2[1][i] - interm2[9][i];
        interm1[10][i] = interm2[2][i] - interm2[10][i];
        interm1[11][i] = interm2[3][i] - interm2[11][i];
        interm1[12][i] = interm2[4][i] - interm2[12][i];
        interm1[13][i] = interm2[5][i] - interm2[13][i];
        interm1[14][i] = interm2[6][i] - interm2[14][i];
        interm1[15][i] = interm2[7][i] - interm2[15][i];

        interm2[0][i] = interm1[0][i] + interm1[4][i];
        interm2[1][i] = interm1[1][i] + interm1[5][i];
        interm2[2][i] = interm1[2][i] + interm1[6][i];
        interm2[3][i] = interm1[3][i] + interm1[7][i];
        interm2[4][i] = interm1[0][i] - interm1[4][i];
        interm2[5][i] = interm1[1][i] - interm1[5][i];
        interm2[6][i] = interm1[2][i] - interm1[6][i];
        interm2[7][i] = interm1[3][i] - interm1[7][i];
        interm2[8][i] = interm1[8][i] + interm1[12][i];
        interm2[9][i] = interm1[9][i] + interm1[13][i];
        interm2[10][i] = interm1[10][i] + interm1[14][i];
        interm2[11][i] = interm1[11][i] + interm1[15][i];
        interm2[12][i] = interm1[8][i] - interm1[12][i];
        interm2[13][i] = interm1[9][i] - interm1[13][i];
        interm2[14][i] = interm1[10][i] - interm1[14][i];
        interm2[15][i] = interm1[11][i] - interm1[15][i];

        interm1[0][i] = interm2[0][i] + interm2[2][i];
        interm1[1][i] = interm2[1][i] + interm2[3][i];
        interm1[2][i] = interm2[0][i] - interm2[2][i];
        interm1[3][i] = interm2[1][i] - interm2[3][i];
        interm1[4][i] = interm2[4][i] + interm2[6][i];
        interm1[5][i] = interm2[5][i] + interm2[7][i];
        interm1[6][i] = interm2[4][i] - interm2[6][i];
        interm1[7][i] = interm2[5][i] - interm2[7][i];
        interm1[8][i] = interm2[8][i] + interm2[10][i];
        interm1[9][i] = interm2[9][i] + interm2[11][i];
        interm1[10][i] = interm2[8][i] - interm2[10][i];
        interm1[11][i] = interm2[9][i] - interm2[11][i];
        interm1[12][i] = interm2[12][i] + interm2[14][i];
        interm1[13][i] = interm2[13][i] + interm2[15][i];
        interm1[14][i] = interm2[12][i] - interm2[14][i];
        interm1[15][i] = interm2[13][i] - interm2[15][i];

        interm2[0][i] = XEVE_ABS(interm1[0][i] + interm1[1][i]);
        interm2[1][i] = XEVE_ABS(interm1[0][i] - interm1[1][i]);
        interm2[2][i] = XEVE_ABS(interm1[2][i] + interm1[3][i]);
        interm2[3][i] = XEVE_ABS(interm1[2][i] - interm1[3][i]);
        interm2[4][i] = XEVE_ABS(interm1[4][i] + interm1[5][i]);
        interm2[5][i] = XEVE_ABS(interm1[4][i] - interm1[5][i]);
        interm2[6][i] = XEVE_ABS(interm1[6][i] + interm1[7][i]);
        interm2[7][i] = XEVE_ABS(interm1[6][i] - interm1[7][i]);
        interm2[8][i] = XEVE_ABS(interm1[8][i] + interm1[9][i]);
        interm2[9][i] = XEVE_ABS(interm1[8][i] - interm1[9][i]);
        interm2[10][i] = XEVE_ABS(interm1[10][i] + interm1[11][i]);
        interm2[11][i] = XEVE_ABS(interm1[10][i] - interm1[11][i]);
        interm2[12][i] = XEVE_ABS(interm1[12][i] + interm1[13][i]);
        interm2[13][i] = XEVE_ABS(interm1[12][i] - interm1[13][i]);
        interm2[14][i] = XEVE_ABS(interm1[14][i] + interm1[15][i]);
        interm2[15][i] = XEVE_ABS(interm1[14][i] - interm1[15][i]);
    }

    satd = interm2[0][0] >> 2;
    for (j = 1; j < 8; j++)
    {
        satd += interm2[0][j];
    }
    for (i = 1; i < 16; i++)
    {
        for (j = 0; j < 8; j++)
        {
            satd += interm2[i][j];
        }
    }

    satd = (int)(satd / (2.0*sqrt(8.0)));

    return satd;
}

int xeve_had_8x4(pel *org, pel *cur, int s_org, int s_cur, int step, int bit_depth)
{
    int k, i, j, jj;
    int satd = 0;
    int sub[32], interm1[4][8], interm2[4][8];
    pel *orgn = org, *curn=cur;

    for(k = 0; k < 32; k += 8)
    {
        sub[k + 0] = orgn[0] - curn[0];
        sub[k + 1] = orgn[1] - curn[1];
        sub[k + 2] = orgn[2] - curn[2];
        sub[k + 3] = orgn[3] - curn[3];
        sub[k + 4] = orgn[4] - curn[4];
        sub[k + 5] = orgn[5] - curn[5];
        sub[k + 6] = orgn[6] - curn[6];
        sub[k + 7] = orgn[7] - curn[7];

        curn += s_cur;
        orgn += s_org;
    }

    for(j = 0; j < 4; j++)
    {
        jj = j << 3;

        interm2[j][0] = sub[jj] + sub[jj + 4];
        interm2[j][1] = sub[jj + 1] + sub[jj + 5];
        interm2[j][2] = sub[jj + 2] + sub[jj + 6];
        interm2[j][3] = sub[jj + 3] + sub[jj + 7];
        interm2[j][4] = sub[jj] - sub[jj + 4];
        interm2[j][5] = sub[jj + 1] - sub[jj + 5];
        interm2[j][6] = sub[jj + 2] - sub[jj + 6];
        interm2[j][7] = sub[jj + 3] - sub[jj + 7];

        interm1[j][0] = interm2[j][0] + interm2[j][2];
        interm1[j][1] = interm2[j][1] + interm2[j][3];
        interm1[j][2] = interm2[j][0] - interm2[j][2];
        interm1[j][3] = interm2[j][1] - interm2[j][3];
        interm1[j][4] = interm2[j][4] + interm2[j][6];
        interm1[j][5] = interm2[j][5] + interm2[j][7];
        interm1[j][6] = interm2[j][4] - interm2[j][6];
        interm1[j][7] = interm2[j][5] - interm2[j][7];

        interm2[j][0] = interm1[j][0] + interm1[j][1];
        interm2[j][1] = interm1[j][0] - interm1[j][1];
        interm2[j][2] = interm1[j][2] + interm1[j][3];
        interm2[j][3] = interm1[j][2] - interm1[j][3];
        interm2[j][4] = interm1[j][4] + interm1[j][5];
        interm2[j][5] = interm1[j][4] - interm1[j][5];
        interm2[j][6] = interm1[j][6] + interm1[j][7];
        interm2[j][7] = interm1[j][6] - interm1[j][7];
    }

    for(i = 0; i < 8; i++)
    {
        interm1[0][i] = interm2[0][i] + interm2[2][i];
        interm1[1][i] = interm2[1][i] + interm2[3][i];
        interm1[2][i] = interm2[0][i] - interm2[2][i];
        interm1[3][i] = interm2[1][i] - interm2[3][i];

        interm2[0][i] = XEVE_ABS(interm1[0][i] + interm1[1][i]);
        interm2[1][i] = XEVE_ABS(interm1[0][i] - interm1[1][i]);
        interm2[2][i] = XEVE_ABS(interm1[2][i] + interm1[3][i]);
        interm2[3][i] = XEVE_ABS(interm1[2][i] - interm1[3][i]);
    }

    satd = interm2[0][0] >> 2;
    for (j = 1; j < 8; j++)
    {
        satd += interm2[0][j];
    }
    for (i = 1; i < 4; i++)
    {
        for (j = 0; j < 8; j++)
        {
            satd += interm2[i][j];
        }
    }

    satd = (int)(satd / sqrt(8.0));

    return satd;
}

int xeve_had_4x8(pel *org, pel *cur, int s_org, int s_cur, int step, int bit_depth)
{
    int k, i, j, jj;
    int satd = 0;
    int sub[32], interm1[8][4], interm2[8][4];
    pel *curn =cur, *orgn = org;

    for(k = 0; k < 32; k += 4)
    {
        sub[k + 0] = orgn[0] - curn[0];
        sub[k + 1] = orgn[1] - curn[1];
        sub[k + 2] = orgn[2] - curn[2];
        sub[k + 3] = orgn[3] - curn[3];

        curn += s_cur;
        orgn += s_org;
    }

    for(j = 0; j < 8; j++)
    {
        jj = j << 2;
        interm2[j][0] = sub[jj] + sub[jj + 2];
        interm2[j][1] = sub[jj + 1] + sub[jj + 3];
        interm2[j][2] = sub[jj] - sub[jj + 2];
        interm2[j][3] = sub[jj + 1] - sub[jj + 3];

        interm1[j][0] = interm2[j][0] + interm2[j][1];
        interm1[j][1] = interm2[j][0] - interm2[j][1];
        interm1[j][2] = interm2[j][2] + interm2[j][3];
        interm1[j][3] = interm2[j][2] - interm2[j][3];
    }

    for(i = 0; i<4; i++)
    {
        interm2[0][i] = interm1[0][i] + interm1[4][i];
        interm2[1][i] = interm1[1][i] + interm1[5][i];
        interm2[2][i] = interm1[2][i] + interm1[6][i];
        interm2[3][i] = interm1[3][i] + interm1[7][i];
        interm2[4][i] = interm1[0][i] - interm1[4][i];
        interm2[5][i] = interm1[1][i] - interm1[5][i];
        interm2[6][i] = interm1[2][i] - interm1[6][i];
        interm2[7][i] = interm1[3][i] - interm1[7][i];

        interm1[0][i] = interm2[0][i] + interm2[2][i];
        interm1[1][i] = interm2[1][i] + interm2[3][i];
        interm1[2][i] = interm2[0][i] - interm2[2][i];
        interm1[3][i] = interm2[1][i] - interm2[3][i];
        interm1[4][i] = interm2[4][i] + interm2[6][i];
        interm1[5][i] = interm2[5][i] + interm2[7][i];
        interm1[6][i] = interm2[4][i] - interm2[6][i];
        interm1[7][i] = interm2[5][i] - interm2[7][i];

        interm2[0][i] = XEVE_ABS(interm1[0][i] + interm1[1][i]);
        interm2[1][i] = XEVE_ABS(interm1[0][i] - interm1[1][i]);
        interm2[2][i] = XEVE_ABS(interm1[2][i] + interm1[3][i]);
        interm2[3][i] = XEVE_ABS(interm1[2][i] - interm1[3][i]);
        interm2[4][i] = XEVE_ABS(interm1[4][i] + interm1[5][i]);
        interm2[5][i] = XEVE_ABS(interm1[4][i] - interm1[5][i]);
        interm2[6][i] = XEVE_ABS(interm1[6][i] + interm1[7][i]);
        interm2[7][i] = XEVE_ABS(interm1[6][i] - interm1[7][i]);
    }

    satd = interm2[0][0] >> 2;
    for (j = 1; j < 4; j++)
    {
        satd += interm2[0][j];
    }
    for (i = 1; i < 8; i++)
    {
        for (j = 0; j < 4; j++)
        {
            satd += interm2[i][j];
        }
    }

    satd = (int)(satd / sqrt(8.0));

    return satd;
}

int xeve_had(int w, int h, void *o, void *c, int s_org, int s_cur, int bit_depth)
{
    pel *org = o;
    pel *cur = c;
    int  x, y;
    int sum = 0;
    int step = 1;

    if(w > h && (h & 7) == 0 && (w & 15) == 0)
    {
        int  offset_org = s_org << 3;
        int  offset_cur = s_cur << 3;

        for(y = 0; y < h; y += 8)
        {
            for(x = 0; x < w; x += 16)
            {
                sum += xeve_had_16x8(&org[x], &cur[x], s_org, s_cur, step, bit_depth);
            }
            org += offset_org;
            cur += offset_cur;
        }
    }
    else if(w < h && (w & 7) == 0 && (h & 15) == 0)
    {
        int  offset_org = s_org << 4;
        int  offset_cur = s_cur << 4;

        for(y = 0; y < h; y += 16)
        {
            for(x = 0; x < w; x += 8)
            {
                sum += xeve_had_8x16(&org[x], &cur[x], s_org, s_cur, step, bit_depth);
            }
            org += offset_org;
            cur += offset_cur;
        }
    }
    else if(w > h && (h & 3) == 0 && (w & 7) == 0)
    {
        int  offset_org = s_org << 2;
        int  offset_cur = s_cur << 2;

        for(y = 0; y < h; y += 4)
        {
            for(x = 0; x < w; x += 8)
            {
                sum += xeve_had_8x4(&org[x], &cur[x], s_org, s_cur, step, bit_depth);
            }
            org += offset_org;
            cur += offset_cur;
        }
    }
    else if(w < h && (w & 3) == 0 && (h & 7) == 0)
    {
        int  offset_org = s_org << 3;
        int  offset_cur = s_cur << 3;

        for(y = 0; y < h; y += 8)
        {
            for(x = 0; x < w; x += 4)
            {
                sum += xeve_had_4x8(&org[x], &cur[x], s_org, s_cur, step, bit_depth);
            }
            org += offset_org;
            cur += offset_cur;
        }
    }
    else if((w % 8 == 0) && (h % 8 == 0))
    {
        int  offset_org = s_org << 3;
        int  offset_cur = s_cur << 3;

        for(y = 0; y < h; y += 8)
        {
            for(x = 0; x < w; x += 8)
            {
                sum += xeve_had_8x8(&org[x], &cur[x*step], s_org, s_cur, step, bit_depth);
            }
            org += offset_org;
            cur += offset_cur;
        }
    }
    else if((w % 4 == 0) && (h % 4 == 0))
    {
        int  offset_org = s_org << 2;
        int  offset_cur = s_cur << 2;

        for(y = 0; y < h; y += 4)
        {
            for(x = 0; x < w; x += 4)
            {
                sum += xeve_had_4x4(&org[x], &cur[x*step], s_org, s_cur, step, bit_depth);
            }
            org += offset_org;
            cur += offset_cur;
        }
    }
    else if((w % 2 == 0) && (h % 2 == 0) )
    {
        int  offset_org = s_org << 1;
        int  offset_cur = s_cur << 1;

        for(y = 0; y < h; y +=2)
        {
            for(x = 0; x < w; x += 2)
            {
                sum += xeve_had_2x2(&org[x], &cur[x*step], s_org, s_cur, step);
            }
            org += offset_org;
            cur += offset_cur;
        }
    }
    else
    {
        xeve_assert(0);
    }

    return (sum >> (bit_depth - 8));
}

/* index: [log2 of width][log2 of height] */
const XEVE_FN_SATD xeve_tbl_satd_16b[1] =
{
    xeve_had,
};

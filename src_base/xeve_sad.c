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

/* SAD for 16bit **************************************************************/
int sad_16b(int w, int h, void *src1, void *src2, int s_src1, int s_src2, int bit_depth)
{
    s16 *s1;
    s16 *s2;

    int i, j, sad;

    s1 = (s16 *)src1;
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
XEVE_FN_SAD xeve_tbl_sad_16b[8][8] =
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

XEVE_FN_DIFF xeve_tbl_diff_16b[8][8] =
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

XEVE_FN_SSD xeve_tbl_ssd_16b[8][8] =
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
    int diff[4], m[4];

    diff[0] = org[0        ] - cur[0        ];
    diff[1] = org[1        ] - cur[1        ];
    diff[2] = org[s_org    ] - cur[0 + s_cur];
    diff[3] = org[s_org + 1] - cur[1 + s_cur];
    m[0] = diff[0] + diff[2];
    m[1] = diff[1] + diff[3];
    m[2] = diff[0] - diff[2];
    m[3] = diff[1] - diff[3];
    satd += (XEVE_ABS(m[0] + m[1]) >> 2);
    satd += XEVE_ABS(m[0] - m[1]);
    satd += XEVE_ABS(m[2] + m[3]);
    satd += XEVE_ABS(m[2] - m[3]);

    return satd;
}

int xeve_had_4x4(pel *org, pel *cur, int s_org, int s_cur, int step, int bit_depth)
{
    int k;
    int satd = 0;
    int diff[16], m[16], d[16];

    for( k = 0; k < 16; k+=4 )
    {
        diff[k+0] = org[0] - cur[0];
        diff[k+1] = org[1] - cur[1];
        diff[k+2] = org[2] - cur[2];
        diff[k+3] = org[3] - cur[3];

        cur += s_cur;
        org += s_org;
    }

    m[ 0] = diff[ 0] + diff[12];
    m[ 1] = diff[ 1] + diff[13];
    m[ 2] = diff[ 2] + diff[14];
    m[ 3] = diff[ 3] + diff[15];
    m[ 4] = diff[ 4] + diff[ 8];
    m[ 5] = diff[ 5] + diff[ 9];
    m[ 6] = diff[ 6] + diff[10];
    m[ 7] = diff[ 7] + diff[11];
    m[ 8] = diff[ 4] - diff[ 8];
    m[ 9] = diff[ 5] - diff[ 9];
    m[10] = diff[ 6] - diff[10];
    m[11] = diff[ 7] - diff[11];
    m[12] = diff[ 0] - diff[12];
    m[13] = diff[ 1] - diff[13];
    m[14] = diff[ 2] - diff[14];
    m[15] = diff[ 3] - diff[15];

    d[ 0] = m[ 0] + m[ 4];
    d[ 1] = m[ 1] + m[ 5];
    d[ 2] = m[ 2] + m[ 6];
    d[ 3] = m[ 3] + m[ 7];
    d[ 4] = m[ 8] + m[12];
    d[ 5] = m[ 9] + m[13];
    d[ 6] = m[10] + m[14];
    d[ 7] = m[11] + m[15];
    d[ 8] = m[ 0] - m[ 4];
    d[ 9] = m[ 1] - m[ 5];
    d[10] = m[ 2] - m[ 6];
    d[11] = m[ 3] - m[ 7];
    d[12] = m[12] - m[ 8];
    d[13] = m[13] - m[ 9];
    d[14] = m[14] - m[10];
    d[15] = m[15] - m[11];

    m[ 0] = d[ 0] + d[ 3];
    m[ 1] = d[ 1] + d[ 2];
    m[ 2] = d[ 1] - d[ 2];
    m[ 3] = d[ 0] - d[ 3];
    m[ 4] = d[ 4] + d[ 7];
    m[ 5] = d[ 5] + d[ 6];
    m[ 6] = d[ 5] - d[ 6];
    m[ 7] = d[ 4] - d[ 7];
    m[ 8] = d[ 8] + d[11];
    m[ 9] = d[ 9] + d[10];
    m[10] = d[ 9] - d[10];
    m[11] = d[ 8] - d[11];
    m[12] = d[12] + d[15];
    m[13] = d[13] + d[14];
    m[14] = d[13] - d[14];
    m[15] = d[12] - d[15];

    d[ 0] = m[ 0] + m[ 1];
    d[ 1] = m[ 0] - m[ 1];
    d[ 2] = m[ 2] + m[ 3];
    d[ 3] = m[ 3] - m[ 2];
    d[ 4] = m[ 4] + m[ 5];
    d[ 5] = m[ 4] - m[ 5];
    d[ 6] = m[ 6] + m[ 7];
    d[ 7] = m[ 7] - m[ 6];
    d[ 8] = m[ 8] + m[ 9];
    d[ 9] = m[ 8] - m[ 9];
    d[10] = m[10] + m[11];
    d[11] = m[11] - m[10];
    d[12] = m[12] + m[13];
    d[13] = m[12] - m[13];
    d[14] = m[14] + m[15];
    d[15] = m[15] - m[14];

    satd += (XEVE_ABS(d[0]) >> 2);
    for (k = 1; k < 16; k++)
    {
        satd += XEVE_ABS(d[k]);
    }
    satd = ((satd + 1) >> 1);

    return satd;
}

int xeve_had_8x8(pel *org, pel *cur, int s_org, int s_cur, int step, int bit_depth)
{
    int k, i, j, jj;
    int satd = 0;
    int diff[64], m1[8][8], m2[8][8], m3[8][8];

    for(k = 0; k < 64; k += 8)
    {
        diff[k+0] = org[0] - cur[0];
        diff[k+1] = org[1] - cur[1];
        diff[k+2] = org[2] - cur[2];
        diff[k+3] = org[3] - cur[3];
        diff[k+4] = org[4] - cur[4];
        diff[k+5] = org[5] - cur[5];
        diff[k+6] = org[6] - cur[6];
        diff[k+7] = org[7] - cur[7];

        cur += s_cur;
        org += s_org;
    }

    /* horizontal */
    for(j = 0; j < 8; j++)
    {
        jj = j << 3;
        m2[j][0] = diff[jj  ] + diff[jj+4];
        m2[j][1] = diff[jj+1] + diff[jj+5];
        m2[j][2] = diff[jj+2] + diff[jj+6];
        m2[j][3] = diff[jj+3] + diff[jj+7];
        m2[j][4] = diff[jj  ] - diff[jj+4];
        m2[j][5] = diff[jj+1] - diff[jj+5];
        m2[j][6] = diff[jj+2] - diff[jj+6];
        m2[j][7] = diff[jj+3] - diff[jj+7];

        m1[j][0] = m2[j][0] + m2[j][2];
        m1[j][1] = m2[j][1] + m2[j][3];
        m1[j][2] = m2[j][0] - m2[j][2];
        m1[j][3] = m2[j][1] - m2[j][3];
        m1[j][4] = m2[j][4] + m2[j][6];
        m1[j][5] = m2[j][5] + m2[j][7];
        m1[j][6] = m2[j][4] - m2[j][6];
        m1[j][7] = m2[j][5] - m2[j][7];

        m2[j][0] = m1[j][0] + m1[j][1];
        m2[j][1] = m1[j][0] - m1[j][1];
        m2[j][2] = m1[j][2] + m1[j][3];
        m2[j][3] = m1[j][2] - m1[j][3];
        m2[j][4] = m1[j][4] + m1[j][5];
        m2[j][5] = m1[j][4] - m1[j][5];
        m2[j][6] = m1[j][6] + m1[j][7];
        m2[j][7] = m1[j][6] - m1[j][7];
    }

    /* vertical */
    for(i = 0; i < 8; i++)
    {
        m3[0][i] = m2[0][i] + m2[4][i];
        m3[1][i] = m2[1][i] + m2[5][i];
        m3[2][i] = m2[2][i] + m2[6][i];
        m3[3][i] = m2[3][i] + m2[7][i];
        m3[4][i] = m2[0][i] - m2[4][i];
        m3[5][i] = m2[1][i] - m2[5][i];
        m3[6][i] = m2[2][i] - m2[6][i];
        m3[7][i] = m2[3][i] - m2[7][i];

        m1[0][i] = m3[0][i] + m3[2][i];
        m1[1][i] = m3[1][i] + m3[3][i];
        m1[2][i] = m3[0][i] - m3[2][i];
        m1[3][i] = m3[1][i] - m3[3][i];
        m1[4][i] = m3[4][i] + m3[6][i];
        m1[5][i] = m3[5][i] + m3[7][i];
        m1[6][i] = m3[4][i] - m3[6][i];
        m1[7][i] = m3[5][i] - m3[7][i];

        m2[0][i] = m1[0][i] + m1[1][i];
        m2[1][i] = m1[0][i] - m1[1][i];
        m2[2][i] = m1[2][i] + m1[3][i];
        m2[3][i] = m1[2][i] - m1[3][i];
        m2[4][i] = m1[4][i] + m1[5][i];
        m2[5][i] = m1[4][i] - m1[5][i];
        m2[6][i] = m1[6][i] + m1[7][i];
        m2[7][i] = m1[6][i] - m1[7][i];
    }

    satd += XEVE_ABS(m2[0][0]) >> 2;
    for (j = 1; j < 8; j++)
    {
        satd += XEVE_ABS(m2[0][j]);
    }
    for (i = 1; i < 8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            satd += XEVE_ABS(m2[i][j]);
        }
    }

    satd = ((satd + 2) >> 2);

    return satd;
}

int xeve_had_16x8(pel *org, pel *cur, int s_org, int s_cur, int step, int bit_depth)
{
    int k, i, j, jj;
    int satd = 0;
    int diff[128], m1[8][16], m2[8][16];

    for(k = 0; k < 128; k += 16)
    {
        diff[k + 0] = org[0] - cur[0];
        diff[k + 1] = org[1] - cur[1];
        diff[k + 2] = org[2] - cur[2];
        diff[k + 3] = org[3] - cur[3];
        diff[k + 4] = org[4] - cur[4];
        diff[k + 5] = org[5] - cur[5];
        diff[k + 6] = org[6] - cur[6];
        diff[k + 7] = org[7] - cur[7];

        diff[k + 8]  = org[8]  - cur[8];
        diff[k + 9]  = org[9]  - cur[9];
        diff[k + 10] = org[10] - cur[10];
        diff[k + 11] = org[11] - cur[11];
        diff[k + 12] = org[12] - cur[12];
        diff[k + 13] = org[13] - cur[13];
        diff[k + 14] = org[14] - cur[14];
        diff[k + 15] = org[15] - cur[15];

        cur += s_cur;
        org += s_org;
    }

    for(j = 0; j < 8; j++)
    {
        jj = j << 4;

        m2[j][0] = diff[jj] + diff[jj + 8];
        m2[j][1] = diff[jj + 1] + diff[jj + 9];
        m2[j][2] = diff[jj + 2] + diff[jj + 10];
        m2[j][3] = diff[jj + 3] + diff[jj + 11];
        m2[j][4] = diff[jj + 4] + diff[jj + 12];
        m2[j][5] = diff[jj + 5] + diff[jj + 13];
        m2[j][6] = diff[jj + 6] + diff[jj + 14];
        m2[j][7] = diff[jj + 7] + diff[jj + 15];
        m2[j][8] = diff[jj] - diff[jj + 8];
        m2[j][9] = diff[jj + 1] - diff[jj + 9];
        m2[j][10] = diff[jj + 2] - diff[jj + 10];
        m2[j][11] = diff[jj + 3] - diff[jj + 11];
        m2[j][12] = diff[jj + 4] - diff[jj + 12];
        m2[j][13] = diff[jj + 5] - diff[jj + 13];
        m2[j][14] = diff[jj + 6] - diff[jj + 14];
        m2[j][15] = diff[jj + 7] - diff[jj + 15];

        m1[j][0] = m2[j][0] + m2[j][4];
        m1[j][1] = m2[j][1] + m2[j][5];
        m1[j][2] = m2[j][2] + m2[j][6];
        m1[j][3] = m2[j][3] + m2[j][7];
        m1[j][4] = m2[j][0] - m2[j][4];
        m1[j][5] = m2[j][1] - m2[j][5];
        m1[j][6] = m2[j][2] - m2[j][6];
        m1[j][7] = m2[j][3] - m2[j][7];
        m1[j][8] = m2[j][8] + m2[j][12];
        m1[j][9] = m2[j][9] + m2[j][13];
        m1[j][10] = m2[j][10] + m2[j][14];
        m1[j][11] = m2[j][11] + m2[j][15];
        m1[j][12] = m2[j][8] - m2[j][12];
        m1[j][13] = m2[j][9] - m2[j][13];
        m1[j][14] = m2[j][10] - m2[j][14];
        m1[j][15] = m2[j][11] - m2[j][15];

        m2[j][0] = m1[j][0] + m1[j][2];
        m2[j][1] = m1[j][1] + m1[j][3];
        m2[j][2] = m1[j][0] - m1[j][2];
        m2[j][3] = m1[j][1] - m1[j][3];
        m2[j][4] = m1[j][4] + m1[j][6];
        m2[j][5] = m1[j][5] + m1[j][7];
        m2[j][6] = m1[j][4] - m1[j][6];
        m2[j][7] = m1[j][5] - m1[j][7];
        m2[j][8] = m1[j][8] + m1[j][10];
        m2[j][9] = m1[j][9] + m1[j][11];
        m2[j][10] = m1[j][8] - m1[j][10];
        m2[j][11] = m1[j][9] - m1[j][11];
        m2[j][12] = m1[j][12] + m1[j][14];
        m2[j][13] = m1[j][13] + m1[j][15];
        m2[j][14] = m1[j][12] - m1[j][14];
        m2[j][15] = m1[j][13] - m1[j][15];

        m1[j][0] = m2[j][0] + m2[j][1];
        m1[j][1] = m2[j][0] - m2[j][1];
        m1[j][2] = m2[j][2] + m2[j][3];
        m1[j][3] = m2[j][2] - m2[j][3];
        m1[j][4] = m2[j][4] + m2[j][5];
        m1[j][5] = m2[j][4] - m2[j][5];
        m1[j][6] = m2[j][6] + m2[j][7];
        m1[j][7] = m2[j][6] - m2[j][7];
        m1[j][8] = m2[j][8] + m2[j][9];
        m1[j][9] = m2[j][8] - m2[j][9];
        m1[j][10] = m2[j][10] + m2[j][11];
        m1[j][11] = m2[j][10] - m2[j][11];
        m1[j][12] = m2[j][12] + m2[j][13];
        m1[j][13] = m2[j][12] - m2[j][13];
        m1[j][14] = m2[j][14] + m2[j][15];
        m1[j][15] = m2[j][14] - m2[j][15];
    }

    for(i = 0; i < 16; i++)
    {
        m2[0][i] = m1[0][i] + m1[4][i];
        m2[1][i] = m1[1][i] + m1[5][i];
        m2[2][i] = m1[2][i] + m1[6][i];
        m2[3][i] = m1[3][i] + m1[7][i];
        m2[4][i] = m1[0][i] - m1[4][i];
        m2[5][i] = m1[1][i] - m1[5][i];
        m2[6][i] = m1[2][i] - m1[6][i];
        m2[7][i] = m1[3][i] - m1[7][i];

        m1[0][i] = m2[0][i] + m2[2][i];
        m1[1][i] = m2[1][i] + m2[3][i];
        m1[2][i] = m2[0][i] - m2[2][i];
        m1[3][i] = m2[1][i] - m2[3][i];
        m1[4][i] = m2[4][i] + m2[6][i];
        m1[5][i] = m2[5][i] + m2[7][i];
        m1[6][i] = m2[4][i] - m2[6][i];
        m1[7][i] = m2[5][i] - m2[7][i];

        m2[0][i] = m1[0][i] + m1[1][i];
        m2[1][i] = m1[0][i] - m1[1][i];
        m2[2][i] = m1[2][i] + m1[3][i];
        m2[3][i] = m1[2][i] - m1[3][i];
        m2[4][i] = m1[4][i] + m1[5][i];
        m2[5][i] = m1[4][i] - m1[5][i];
        m2[6][i] = m1[6][i] + m1[7][i];
        m2[7][i] = m1[6][i] - m1[7][i];
    }

    satd += XEVE_ABS(m2[0][0]) >> 2;
    for (j = 1; j < 16; j++)
    {
        satd += XEVE_ABS(m2[0][j]);
    }
    for (i = 1; i < 8; i++)
    {
        for (j = 0; j < 16; j++)
        {
            satd += XEVE_ABS(m2[i][j]);
        }
    }

    satd = (int)(satd / sqrt(16.0 * 8.0) * 2.0);

    return satd;
}

int xeve_had_8x16(pel *org, pel *cur, int s_org, int s_cur, int step, int bit_depth)
{
    int k, i, j, jj;
    int satd = 0;
    int diff[128], m1[16][8], m2[16][8];

    for(k = 0; k < 128; k += 8)
    {
        diff[k + 0] = org[0] - cur[0];
        diff[k + 1] = org[1] - cur[1];
        diff[k + 2] = org[2] - cur[2];
        diff[k + 3] = org[3] - cur[3];
        diff[k + 4] = org[4] - cur[4];
        diff[k + 5] = org[5] - cur[5];
        diff[k + 6] = org[6] - cur[6];
        diff[k + 7] = org[7] - cur[7];

        cur += s_cur;
        org += s_org;
    }

    for(j = 0; j < 16; j++)
    {
        jj = j << 3;

        m2[j][0] = diff[jj] + diff[jj + 4];
        m2[j][1] = diff[jj + 1] + diff[jj + 5];
        m2[j][2] = diff[jj + 2] + diff[jj + 6];
        m2[j][3] = diff[jj + 3] + diff[jj + 7];
        m2[j][4] = diff[jj] - diff[jj + 4];
        m2[j][5] = diff[jj + 1] - diff[jj + 5];
        m2[j][6] = diff[jj + 2] - diff[jj + 6];
        m2[j][7] = diff[jj + 3] - diff[jj + 7];

        m1[j][0] = m2[j][0] + m2[j][2];
        m1[j][1] = m2[j][1] + m2[j][3];
        m1[j][2] = m2[j][0] - m2[j][2];
        m1[j][3] = m2[j][1] - m2[j][3];
        m1[j][4] = m2[j][4] + m2[j][6];
        m1[j][5] = m2[j][5] + m2[j][7];
        m1[j][6] = m2[j][4] - m2[j][6];
        m1[j][7] = m2[j][5] - m2[j][7];

        m2[j][0] = m1[j][0] + m1[j][1];
        m2[j][1] = m1[j][0] - m1[j][1];
        m2[j][2] = m1[j][2] + m1[j][3];
        m2[j][3] = m1[j][2] - m1[j][3];
        m2[j][4] = m1[j][4] + m1[j][5];
        m2[j][5] = m1[j][4] - m1[j][5];
        m2[j][6] = m1[j][6] + m1[j][7];
        m2[j][7] = m1[j][6] - m1[j][7];
    }

    for(i = 0; i < 8; i++)
    {
        m1[0][i] = m2[0][i] + m2[8][i];
        m1[1][i] = m2[1][i] + m2[9][i];
        m1[2][i] = m2[2][i] + m2[10][i];
        m1[3][i] = m2[3][i] + m2[11][i];
        m1[4][i] = m2[4][i] + m2[12][i];
        m1[5][i] = m2[5][i] + m2[13][i];
        m1[6][i] = m2[6][i] + m2[14][i];
        m1[7][i] = m2[7][i] + m2[15][i];
        m1[8][i] = m2[0][i] - m2[8][i];
        m1[9][i] = m2[1][i] - m2[9][i];
        m1[10][i] = m2[2][i] - m2[10][i];
        m1[11][i] = m2[3][i] - m2[11][i];
        m1[12][i] = m2[4][i] - m2[12][i];
        m1[13][i] = m2[5][i] - m2[13][i];
        m1[14][i] = m2[6][i] - m2[14][i];
        m1[15][i] = m2[7][i] - m2[15][i];

        m2[0][i] = m1[0][i] + m1[4][i];
        m2[1][i] = m1[1][i] + m1[5][i];
        m2[2][i] = m1[2][i] + m1[6][i];
        m2[3][i] = m1[3][i] + m1[7][i];
        m2[4][i] = m1[0][i] - m1[4][i];
        m2[5][i] = m1[1][i] - m1[5][i];
        m2[6][i] = m1[2][i] - m1[6][i];
        m2[7][i] = m1[3][i] - m1[7][i];
        m2[8][i] = m1[8][i] + m1[12][i];
        m2[9][i] = m1[9][i] + m1[13][i];
        m2[10][i] = m1[10][i] + m1[14][i];
        m2[11][i] = m1[11][i] + m1[15][i];
        m2[12][i] = m1[8][i] - m1[12][i];
        m2[13][i] = m1[9][i] - m1[13][i];
        m2[14][i] = m1[10][i] - m1[14][i];
        m2[15][i] = m1[11][i] - m1[15][i];

        m1[0][i] = m2[0][i] + m2[2][i];
        m1[1][i] = m2[1][i] + m2[3][i];
        m1[2][i] = m2[0][i] - m2[2][i];
        m1[3][i] = m2[1][i] - m2[3][i];
        m1[4][i] = m2[4][i] + m2[6][i];
        m1[5][i] = m2[5][i] + m2[7][i];
        m1[6][i] = m2[4][i] - m2[6][i];
        m1[7][i] = m2[5][i] - m2[7][i];
        m1[8][i] = m2[8][i] + m2[10][i];
        m1[9][i] = m2[9][i] + m2[11][i];
        m1[10][i] = m2[8][i] - m2[10][i];
        m1[11][i] = m2[9][i] - m2[11][i];
        m1[12][i] = m2[12][i] + m2[14][i];
        m1[13][i] = m2[13][i] + m2[15][i];
        m1[14][i] = m2[12][i] - m2[14][i];
        m1[15][i] = m2[13][i] - m2[15][i];

        m2[0][i] = m1[0][i] + m1[1][i];
        m2[1][i] = m1[0][i] - m1[1][i];
        m2[2][i] = m1[2][i] + m1[3][i];
        m2[3][i] = m1[2][i] - m1[3][i];
        m2[4][i] = m1[4][i] + m1[5][i];
        m2[5][i] = m1[4][i] - m1[5][i];
        m2[6][i] = m1[6][i] + m1[7][i];
        m2[7][i] = m1[6][i] - m1[7][i];
        m2[8][i] = m1[8][i] + m1[9][i];
        m2[9][i] = m1[8][i] - m1[9][i];
        m2[10][i] = m1[10][i] + m1[11][i];
        m2[11][i] = m1[10][i] - m1[11][i];
        m2[12][i] = m1[12][i] + m1[13][i];
        m2[13][i] = m1[12][i] - m1[13][i];
        m2[14][i] = m1[14][i] + m1[15][i];
        m2[15][i] = m1[14][i] - m1[15][i];
    }

    satd += XEVE_ABS(m2[0][0]) >> 2;
    for (j = 1; j < 8; j++)
    {
        satd += XEVE_ABS(m2[0][j]);
    }
    for (i = 1; i < 16; i++)
    {
        for (j = 0; j < 8; j++)
        {
            satd += XEVE_ABS(m2[i][j]);
        }
    }

    satd = (int)(satd / sqrt(16.0 * 8.0) * 2.0);

    return satd;
}

int xeve_had_8x4(pel *org, pel *cur, int s_org, int s_cur, int step, int bit_depth)
{
    int k, i, j, jj;
    int satd = 0;
    int diff[32], m1[4][8], m2[4][8];

    for(k = 0; k < 32; k += 8)
    {
        diff[k + 0] = org[0] - cur[0];
        diff[k + 1] = org[1] - cur[1];
        diff[k + 2] = org[2] - cur[2];
        diff[k + 3] = org[3] - cur[3];
        diff[k + 4] = org[4] - cur[4];
        diff[k + 5] = org[5] - cur[5];
        diff[k + 6] = org[6] - cur[6];
        diff[k + 7] = org[7] - cur[7];

        cur += s_cur;
        org += s_org;
    }

    for(j = 0; j < 4; j++)
    {
        jj = j << 3;

        m2[j][0] = diff[jj] + diff[jj + 4];
        m2[j][1] = diff[jj + 1] + diff[jj + 5];
        m2[j][2] = diff[jj + 2] + diff[jj + 6];
        m2[j][3] = diff[jj + 3] + diff[jj + 7];
        m2[j][4] = diff[jj] - diff[jj + 4];
        m2[j][5] = diff[jj + 1] - diff[jj + 5];
        m2[j][6] = diff[jj + 2] - diff[jj + 6];
        m2[j][7] = diff[jj + 3] - diff[jj + 7];

        m1[j][0] = m2[j][0] + m2[j][2];
        m1[j][1] = m2[j][1] + m2[j][3];
        m1[j][2] = m2[j][0] - m2[j][2];
        m1[j][3] = m2[j][1] - m2[j][3];
        m1[j][4] = m2[j][4] + m2[j][6];
        m1[j][5] = m2[j][5] + m2[j][7];
        m1[j][6] = m2[j][4] - m2[j][6];
        m1[j][7] = m2[j][5] - m2[j][7];

        m2[j][0] = m1[j][0] + m1[j][1];
        m2[j][1] = m1[j][0] - m1[j][1];
        m2[j][2] = m1[j][2] + m1[j][3];
        m2[j][3] = m1[j][2] - m1[j][3];
        m2[j][4] = m1[j][4] + m1[j][5];
        m2[j][5] = m1[j][4] - m1[j][5];
        m2[j][6] = m1[j][6] + m1[j][7];
        m2[j][7] = m1[j][6] - m1[j][7];
    }

    for(i = 0; i < 8; i++)
    {
        m1[0][i] = m2[0][i] + m2[2][i];
        m1[1][i] = m2[1][i] + m2[3][i];
        m1[2][i] = m2[0][i] - m2[2][i];
        m1[3][i] = m2[1][i] - m2[3][i];

        m2[0][i] = m1[0][i] + m1[1][i];
        m2[1][i] = m1[0][i] - m1[1][i];
        m2[2][i] = m1[2][i] + m1[3][i];
        m2[3][i] = m1[2][i] - m1[3][i];
    }

    satd += XEVE_ABS(m2[0][0]) >> 2;
    for (j = 1; j < 8; j++)
    {
        satd += XEVE_ABS(m2[0][j]);
    }
    for (i = 1; i < 4; i++)
    {
        for (j = 0; j < 8; j++)
        {
            satd += XEVE_ABS(m2[i][j]);
        }
    }

    satd = (int)(satd / sqrt(4.0 * 8.0) * 2.0);

    return satd;
}

int xeve_had_4x8(pel *org, pel *cur, int s_org, int s_cur, int step, int bit_depth)
{
    int k, i, j, jj;
    int satd = 0;
    int diff[32], m1[8][4], m2[8][4];

    for(k = 0; k < 32; k += 4)
    {
        diff[k + 0] = org[0] - cur[0];
        diff[k + 1] = org[1] - cur[1];
        diff[k + 2] = org[2] - cur[2];
        diff[k + 3] = org[3] - cur[3];

        cur += s_cur;
        org += s_org;
    }

    for(j = 0; j < 8; j++)
    {
        jj = j << 2;
        m2[j][0] = diff[jj] + diff[jj + 2];
        m2[j][1] = diff[jj + 1] + diff[jj + 3];
        m2[j][2] = diff[jj] - diff[jj + 2];
        m2[j][3] = diff[jj + 1] - diff[jj + 3];

        m1[j][0] = m2[j][0] + m2[j][1];
        m1[j][1] = m2[j][0] - m2[j][1];
        m1[j][2] = m2[j][2] + m2[j][3];
        m1[j][3] = m2[j][2] - m2[j][3];
    }

    for(i = 0; i<4; i++)
    {
        m2[0][i] = m1[0][i] + m1[4][i];
        m2[1][i] = m1[1][i] + m1[5][i];
        m2[2][i] = m1[2][i] + m1[6][i];
        m2[3][i] = m1[3][i] + m1[7][i];
        m2[4][i] = m1[0][i] - m1[4][i];
        m2[5][i] = m1[1][i] - m1[5][i];
        m2[6][i] = m1[2][i] - m1[6][i];
        m2[7][i] = m1[3][i] - m1[7][i];

        m1[0][i] = m2[0][i] + m2[2][i];
        m1[1][i] = m2[1][i] + m2[3][i];
        m1[2][i] = m2[0][i] - m2[2][i];
        m1[3][i] = m2[1][i] - m2[3][i];
        m1[4][i] = m2[4][i] + m2[6][i];
        m1[5][i] = m2[5][i] + m2[7][i];
        m1[6][i] = m2[4][i] - m2[6][i];
        m1[7][i] = m2[5][i] - m2[7][i];

        m2[0][i] = m1[0][i] + m1[1][i];
        m2[1][i] = m1[0][i] - m1[1][i];
        m2[2][i] = m1[2][i] + m1[3][i];
        m2[3][i] = m1[2][i] - m1[3][i];
        m2[4][i] = m1[4][i] + m1[5][i];
        m2[5][i] = m1[4][i] - m1[5][i];
        m2[6][i] = m1[6][i] + m1[7][i];
        m2[7][i] = m1[6][i] - m1[7][i];
    }

    satd += XEVE_ABS(m2[0][0]) >> 2;
    for (j = 1; j < 4; j++)
    {
        satd += XEVE_ABS(m2[0][j]);
    }
    for (i = 1; i < 8; i++)
    {
        for (j = 0; j < 4; j++)
        {
            satd += XEVE_ABS(m2[i][j]);
        }
    }

    satd = (int)(satd / sqrt(4.0 * 8.0) * 2.0);

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
XEVE_FN_SATD xeve_tbl_satd_16b[1] =
{
    xeve_had,
};

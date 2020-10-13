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

#include "xeve.h"
#include "xeve_img.h"

#define IMG_ALIGN(val, align)    ((((val) + (align) - 1) / (align)) * (align))

static void imgb_delete(XEVE_IMGB * imgb)
{
    int i;
    xeve_assert_r(imgb);

    for(i=0; i<XEVE_IMGB_MAX_PLANE; i++)
    {
        if (imgb->baddr[i]) xeve_mfree(imgb->baddr[i]);
    }
    xeve_mfree(imgb);
}

static int imgb_addref(XEVE_IMGB * imgb)
{
    xeve_assert_rv(imgb, XEVE_ERR_INVALID_ARGUMENT);
    return xeve_atomic_inc(&imgb->refcnt);
}

static int imgb_getref(XEVE_IMGB * imgb)
{
    xeve_assert_rv(imgb, XEVE_ERR_INVALID_ARGUMENT);
    return imgb->refcnt;
}

static int imgb_release(XEVE_IMGB * imgb)
{
    int refcnt;
    xeve_assert_rv(imgb, XEVE_ERR_INVALID_ARGUMENT);
    refcnt = xeve_atomic_dec(&imgb->refcnt);
    if(refcnt == 0)
    {
        imgb_delete(imgb);
    }
    return refcnt;
}

XEVE_IMGB * xeve_imgb_create(int w, int h, int cs, int opt, int pad[XEVE_IMGB_MAX_PLANE], int align[XEVE_IMGB_MAX_PLANE])
{
    int i, p_size, a_size, bd;
    XEVE_IMGB * imgb;

    imgb = (XEVE_IMGB *)xeve_malloc(sizeof(XEVE_IMGB));
    xeve_assert_rv(imgb, NULL);
    xeve_mset(imgb, 0, sizeof(XEVE_IMGB));

    if(cs == XEVE_COLORSPACE_YUV420 || cs == XEVE_COLORSPACE_YUV420_10LE || cs == XEVE_COLORSPACE_YUV420_12LE || cs == XEVE_COLORSPACE_YUV420_14LE)
    {
        if(cs == XEVE_COLORSPACE_YUV420) bd = 1;
        else /*if(cs == XEVE_COLORSPACE_YUV420_10LE)*/ bd = 2;

        for(i=0; i<3; i++)
        {
            imgb->w[i] = w;
            imgb->h[i] = h;
            imgb->x[i] = 0;
            imgb->y[i] = 0;

            a_size = (align != NULL)? align[i] : 0;
            p_size = (pad != NULL)? pad[i] : 0;

            imgb->aw[i] = IMG_ALIGN(w, a_size);
            imgb->ah[i] = IMG_ALIGN(h, a_size);

            imgb->padl[i] = imgb->padr[i]=imgb->padu[i]=imgb->padb[i]=p_size;

            imgb->s[i] = (imgb->aw[i] + imgb->padl[i] + imgb->padr[i]) * bd;
            imgb->e[i] = imgb->ah[i] + imgb->padu[i] + imgb->padb[i];

            imgb->bsize[i] = imgb->s[i]*imgb->e[i];
            imgb->baddr[i] = xeve_malloc(imgb->bsize[i]);

            imgb->a[i] = ((u8*)imgb->baddr[i]) + imgb->padu[i]*imgb->s[i] +
                imgb->padl[i]*bd;

            if(i == 0) { w = (w+1)>>1; h = (h+1)>>1; }
        }
        imgb->np = 3;
    }
    else
    {
        xeve_trace("unsupported color space\n");
        xeve_mfree(imgb);
        return NULL;
    }
    imgb->addref = imgb_addref;
    imgb->getref = imgb_getref;
    imgb->release = imgb_release;
    imgb->cs = cs;
    imgb->addref(imgb);

    return imgb;
}

static void imgb_conv_shift_left_8b(XEVE_IMGB * imgb_dst, XEVE_IMGB * imgb_src, int shift)
{
    int i, j, k;

    unsigned char * s;
    short         * d;

    for (i = 0; i < 3; i++)
    {
        s = imgb_src->a[i];
        d = imgb_dst->a[i];

        for (j = 0; j < imgb_src->h[i]; j++)
        {
            for (k = 0; k < imgb_src->w[i]; k++)
            {
                d[k] = (short)(s[k] << shift);
            }
            s = s + imgb_src->s[i];
            d = (short*)(((unsigned char *)d) + imgb_dst->s[i]);
        }
    }
}

static void __imgb_cpy_plane(void *src, void *dst, int bw, int h, int s_src,
    int s_dst)
{
    int i;
    unsigned char *s, *d;

    s = (unsigned char*)src;
    d = (unsigned char*)dst;

    for (i = 0; i < h; i++)
    {
        memcpy(d, s, bw);
        s += s_src;
        d += s_dst;
    }
}

static void imgb_conv_shift_left(XEVE_IMGB * imgb_dst, XEVE_IMGB * imgb_src, int shift)
{
    int i, j, k;

    unsigned short * s;
    unsigned short * d;

    for (i = 0; i < 3; i++)
    {
        s = imgb_src->a[i];
        d = imgb_dst->a[i];

        for (j = 0; j < imgb_src->h[i]; j++)
        {
            for (k = 0; k < imgb_src->w[i]; k++)
            {
                d[k] = (unsigned short)(s[k] << shift);
            }
            s = (short*)(((unsigned char *)s) + imgb_src->s[i]);
            d = (short*)(((unsigned char *)d) + imgb_dst->s[i]);
        }
    }
}

static void imgb_conv_shift_right(XEVE_IMGB * imgb_dst, XEVE_IMGB * imgb_src, int shift)
{

    int i, j, k, t0, add;

    int clip_min = 0;
    int clip_max = 0;

    unsigned short         * s;
    unsigned short         * d;
    if (shift)
        add = 1 << (shift - 1);
    else
        add = 0;

    clip_max = (1 << (BD_FROM_CS(imgb_dst->cs))) - 1;

    for (i = 0; i < 3; i++)
    {
        s = imgb_src->a[i];
        d = imgb_dst->a[i];

        for (j = 0; j < imgb_src->h[i]; j++)
        {
            for (k = 0; k < imgb_src->w[i]; k++)
            {
                t0 = ((s[k] + add) >> shift);
                d[k] = (XEVE_CLIP3(clip_min, clip_max, t0));

            }
            s = (short*)(((unsigned char *)s) + imgb_src->s[i]);
            d = (short*)(((unsigned char *)d) + imgb_dst->s[i]);
        }
    }
}

void xeve_imgb_cpy(XEVE_IMGB * dst, XEVE_IMGB * src)
{
    int i, bd;
    int src_bd, dst_bd;
    src_bd = BD_FROM_CS(src->cs);
    dst_bd = BD_FROM_CS(dst->cs);
    if(src_bd == 8)
    {
        imgb_conv_shift_left_8b(dst, src, dst_bd - src_bd);
    }
    else
    {
        if(src->cs == dst->cs)
        {
            bd = 2;
            for(i = 0; i < src->np; i++)
            {
                __imgb_cpy_plane(src->a[i], dst->a[i], bd*src->w[i], src->h[i],
                    src->s[i], dst->s[i]);
            }
        }
        else if(src->cs > dst->cs)
        {
            imgb_conv_shift_right(dst, src, src_bd - dst_bd);
        }
        else
        {
            imgb_conv_shift_left(dst, src, dst_bd - src_bd);
        }
    }

    for(i = 0; i < 4; i++)
    {
        dst->ts[i] = src->ts[i];
    }

    for (i = 0; i < 3; i++)
    {
        dst->x[i] = src->x[i];
        dst->y[i] = src->y[i];
        dst->w[i] = src->w[i];
        dst->h[i] = src->h[i];
    }
}

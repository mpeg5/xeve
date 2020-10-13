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

#ifndef _XEVEA_APP_UTIL_H_
#define _XEVEA_APP_UTIL_H_

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>

/* logging functions */
#if defined(__GNUC__)
#define logva(args...) printf(args)
#else
#define logva(args, ...) printf(args, __VA_ARGS__)
#endif

#define VERBOSE_0                  0
#define VERBOSE_1                  1
#define VERBOSE_2                  2

static int op_verbose = VERBOSE_1;

#if defined(__GNUC__)
#define logv0(args...) {if(op_verbose >= VERBOSE_0) {logva(args);}}
#define logv1(args...) {if(op_verbose >= VERBOSE_1) {logva(args);}}
#define logv2(args...) {if(op_verbose >= VERBOSE_2) {logva(args);}}
#else
#define logv0(args,...) {if(op_verbose >= VERBOSE_0){logva(args,__VA_ARGS__);}}
#define logv1(args,...) {if(op_verbose >= VERBOSE_1){logva(args,__VA_ARGS__);}}
#define logv2(args,...) {if(op_verbose >= VERBOSE_2){logva(args,__VA_ARGS__);}}
#endif

/* Clocks */
#if defined(_WIN64) || defined(_WIN32)
#include <windows.h>

#define XEVE_CLK             DWORD
#define XEVE_CLK_PER_SEC     (1000)
#define XEVE_CLK_PER_MSEC    (1)
#define XEVE_CLK_MAX         ((XEVE_CLK)(-1))
#define xeve_clk_get()       GetTickCount()

#elif __linux__ || __CYGWIN__
#include <time.h>
#include <sys/time.h>
#define XEVE_CLK             unsigned long
#define XEVE_CLK_MAX         ((XEVE_CLK)(-1))
#define XEVE_CLK_PER_SEC     (10000)
#define XEVE_CLK_PER_MSEC    (10)
static XEVE_CLK xeve_clk_get(void)
{
    XEVE_CLK clk;
    struct timeval t;
    gettimeofday(&t, NULL);
    clk = t.tv_sec*10000L + t.tv_usec/100L;
    return clk;
}

#else
#error THIS PLATFORM CANNOT SUPPORT CLOCK
#endif

#define xeve_clk_diff(t1, t2) \
    (((t2) >= (t1)) ? ((t2) - (t1)) : ((XEVE_CLK_MAX - (t1)) + (t2)))

static XEVE_CLK xeve_clk_from(XEVE_CLK from) \
{
  XEVE_CLK now = xeve_clk_get(); \
    return xeve_clk_diff(from, now); \
}

#define xeve_clk_msec(clk) \
    ((int)((clk + (XEVE_CLK_PER_MSEC/2))/XEVE_CLK_PER_MSEC))
#define xeve_clk_sec(clk)  \
    ((int)((clk + (XEVE_CLK_PER_SEC/2))/XEVE_CLK_PER_SEC))

#define XEVEA_CLIP(n,min,max) (((n)>(max))? (max) : (((n)<(min))? (min) : (n)))

static int imgb_read(FILE * fp, XEVE_IMGB * img)
{
    int f_w, f_h;
    int y_size, u_size, v_size;

    f_w = img->w[0];
    f_h = img->h[0];

    if(img->cs == XEVE_COLORSPACE_YUV420)
    {
        y_size = f_w * f_h;
        u_size = v_size = (f_w >> 1) * (f_h >> 1);

        if(fread(img->a[0], 1, y_size, fp) != (unsigned)y_size)
        {
            return -1;
        }
        if(fread(img->a[1], 1, u_size, fp) != (unsigned)u_size)
        {
            return -1;
        }
        if(fread(img->a[2], 1, v_size, fp) != (unsigned)v_size)
        {
            return -1;
        }
    }
    else if(img->cs == XEVE_COLORSPACE_YUV420_10LE || img->cs == XEVE_COLORSPACE_YUV420_12LE || img->cs == XEVE_COLORSPACE_YUV420_14LE)
    {
        y_size = f_w * f_h * sizeof(short);
        u_size = v_size = (f_w >> 1) * (f_h >> 1) * sizeof(short);
        if(fread(img->a[0], 1, y_size, fp) != (unsigned)y_size)
        {
            return -1;
        }
        if(fread(img->a[1], 1, u_size, fp) != (unsigned)u_size)
        {
            return -1;
        }
        if(fread(img->a[2], 1, v_size, fp) != (unsigned)v_size)
        {
            return -1;
        }
    }
    else
    {
        logv0("not supported color space\n");
        return -1;
    }

    return 0;
}

static int imgb_write(char * fname, XEVE_IMGB * img)
{
    unsigned char * p8;
    int             i, j, bd;
    int             cs_w_off, cs_h_off;
    FILE          * fp;

    fp = fopen(fname, "ab");
    if(fp == NULL)
    {
        logv0("cannot open file = %s\n", fname);
        return -1;
    }
    if(img->cs == XEVE_COLORSPACE_YUV420_10LE || img->cs == XEVE_COLORSPACE_YUV420_12LE || img->cs == XEVE_COLORSPACE_YUV420_14LE)
    {
        bd = 2;
        cs_w_off = 2;
        cs_h_off = 2;
    }
    else if(img->cs == XEVE_COLORSPACE_YUV420)
    {
        bd = 1;
        cs_w_off = 2;
        cs_h_off = 2;
    }
    else
    {
        logv0("cannot support the color space\n");
        return -1;
    }
    
    for (i = 0; i < 3; i++)
    {
        p8 = (unsigned char *)img->a[i] + (img->s[i] * img->y[i]) + (img->x[i] * bd);
        for (j = 0; j < img->h[i]; j++)
        {
            fwrite(p8, img->w[i] * bd, 1, fp);
            p8 += img->s[i];
        }
    }

    fclose(fp);
    return 0;
}

static void imgb_cpy_plane(void *src, void *dst, int bw, int h, int s_src, int s_dst)
{
    int i;
    unsigned char *s, *d;

    s = (unsigned char*)src;
    d = (unsigned char*)dst;

    for(i = 0; i < h; i++)
    {
        memcpy(d, s, bw);
        s += s_src;
        d += s_dst;
    }
}

static void imgb_conv_shift_left_8b(XEVE_IMGB * imgb_dst, XEVE_IMGB * imgb_src, int shift)
{
    int i, j, k;

    unsigned char * s;
    short         * d;

    for(i = 0; i < 3; i++)
    {
        s = imgb_src->a[i];
        d = imgb_dst->a[i];

        for(j = 0; j < imgb_src->ah[i]; j++)
        {
            for(k = 0; k < imgb_src->aw[i]; k++)
            {
                d[k] = (short)(s[k] << shift);
            }
            s = s + imgb_src->s[i];
            d = (short*)(((unsigned char *)d) + imgb_dst->s[i]);
        }
    }
}

static void imgb_conv_shift_right_8b(XEVE_IMGB * imgb_dst, XEVE_IMGB * imgb_src, int shift)
{
    int i, j, k, t0, add;

    short         * s;
    unsigned char * d;

    if(shift)
    add = 1 << (shift - 1);
    else
        add = 0;

    for(i = 0; i < 3; i++)
    {
        s = imgb_src->a[i];
        d = imgb_dst->a[i];

        for(j = 0; j < imgb_src->ah[i]; j++)
        {
            for(k = 0; k < imgb_src->aw[i]; k++)
            {
                t0 = ((s[k] + add) >> shift);
                d[k] = (unsigned char)(XEVEA_CLIP(t0, 0, 255));

            }
            s = (short*)(((unsigned char *)s) + imgb_src->s[i]);
            d = d + imgb_dst->s[i];
        }
    }
}

static void imgb_conv_shift_left(XEVE_IMGB * imgb_dst, XEVE_IMGB * imgb_src, int shift)
{
    int i, j, k;

    unsigned short * s;
    unsigned short * d;

    for(i = 0; i < 3; i++)
    {
        s = imgb_src->a[i];
        d = imgb_dst->a[i];

        for(j = 0; j < imgb_src->h[i]; j++)
        {
            for(k = 0; k < imgb_src->w[i]; k++)
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
    if(shift)
    add = 1 << (shift - 1);
    else
        add = 0;

    clip_max = (1 << (BD_FROM_CS(imgb_dst->cs))) - 1;

    for(i = 0; i < 3; i++)
    {
        s = imgb_src->a[i];
        d = imgb_dst->a[i];

        for(j = 0; j < imgb_src->h[i]; j++)
        {
            for(k = 0; k < imgb_src->w[i]; k++)
            {
                t0 = ((s[k] + add) >> shift);
                d[k] = (XEVEA_CLIP(t0, clip_min, clip_max));

            }
            s = (short*)(((unsigned char *)s) + imgb_src->s[i]);
            d = (short*)(((unsigned char *)d) + imgb_dst->s[i]);
        }
    }
}

static void imgb_cpy_bd(XEVE_IMGB * dst, XEVE_IMGB * src)
{
    int i, bd;

    if(src->cs == dst->cs)
    {
        if(src->cs >= XEVE_COLORSPACE_YUV400_10LE) bd = 2;
        else bd = 1;

        for(i = 0; i < src->np; i++)
        {
            imgb_cpy_plane(src->a[i], dst->a[i], bd*src->aw[i], src->ah[i], src->s[i], dst->s[i]);
        }
    }
    else if(src->cs == XEVE_COLORSPACE_YUV420)
    {
        imgb_conv_shift_left_8b(dst, src, ((dst->cs / 100) - 4) * 2); //complicated formula because of colour space macors
    }
    else if(src->cs == XEVE_COLORSPACE_YUV420_10LE)
    {
        if(dst->cs == XEVE_COLORSPACE_YUV420)
            imgb_conv_shift_right_8b(dst, src, 2);
        else
            imgb_conv_shift_left(dst, src, ((dst->cs / 100) - 5) * 2); //complicated formula because of colour space macors
    }
    else if(src->cs == XEVE_COLORSPACE_YUV420_12LE)
    {
        if(dst->cs == XEVE_COLORSPACE_YUV420)
            imgb_conv_shift_right_8b(dst, src, 4);
        else if(dst->cs == XEVE_COLORSPACE_YUV420_10LE)
            imgb_conv_shift_right(dst, src, (6 - (dst->cs / 100)) * 2);
        else
            imgb_conv_shift_left(dst, src, ((dst->cs / 100) - 6) * 2); //complicated formula because of colour space macors
    }
    else if(src->cs == XEVE_COLORSPACE_YUV420_14LE)
    {
        if(dst->cs == XEVE_COLORSPACE_YUV420)
            imgb_conv_shift_right_8b(dst, src, 6);
        else if(dst->cs == XEVE_COLORSPACE_YUV420_10LE || XEVE_COLORSPACE_YUV420_12LE)
            imgb_conv_shift_right(dst, src, (7 - (dst->cs / 100)) * 2);
        else
            imgb_conv_shift_left(dst, src, ((dst->cs / 100) - 7) * 2); //complicated formula because of colour space macors
    }
    else
    {
        logv0("ERROR: unsupported image copy\n");
        return;
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

static void imgb_free(XEVE_IMGB * imgb)
{
    int i;
    for(i = 0; i < XEVE_IMGB_MAX_PLANE; i++)
    {
        if(imgb->baddr[i]) free(imgb->baddr[i]);
    }
    free(imgb);
}

XEVE_IMGB * imgb_alloc(int w, int h, int cs)
{
    int i;
    XEVE_IMGB * imgb;

    imgb = (XEVE_IMGB *)malloc(sizeof(XEVE_IMGB));
    if(imgb == NULL)
    {
        logv0("cannot create image buffer\n");
        return NULL;
    }
    memset(imgb, 0, sizeof(XEVE_IMGB));

    if(cs == XEVE_COLORSPACE_YUV420)
    {
        for(i = 0; i < 3; i++)
        {
            imgb->w[i] = imgb->aw[i] = imgb->s[i] = w;
            imgb->h[i] = imgb->ah[i] = imgb->e[i] = h;
            imgb->bsize[i] = imgb->s[i] * imgb->e[i];

            imgb->a[i] = imgb->baddr[i] = malloc(imgb->bsize[i]);
            if(imgb->a[i] == NULL)
            {
                logv0("cannot allocate picture buffer\n");
                return NULL;
            }

            if(i == 0)
            {
                w = (w + 1) >> 1; h = (h + 1) >> 1;
            }
        }
        imgb->np = 3;
    }
    else if(cs == XEVE_COLORSPACE_YUV420_10LE || cs == XEVE_COLORSPACE_YUV420_12LE || cs == XEVE_COLORSPACE_YUV420_14LE)
    {
        for(i = 0; i < 3; i++)
        {
            imgb->w[i] = imgb->aw[i] = w;
            imgb->s[i] = w * sizeof(short);
            imgb->h[i] = imgb->ah[i] = imgb->e[i] = h;
            imgb->bsize[i] = imgb->s[i] * imgb->e[i];

            imgb->a[i] = imgb->baddr[i] = malloc(imgb->bsize[i]);
            if(imgb->a[i] == NULL)
            {
                logv0("cannot allocate picture buffer\n");
                return NULL;
            }

            if(i == 0)
            {
                w = (w + 1) >> 1; h = (h + 1) >> 1;
            }
        }
        imgb->np = 3;
    }
    else if (cs == XEVE_COLORSPACE_YUV444_10LE)
    {
        for (i = 0; i < 3; i++)
        {
            imgb->w[i] = imgb->aw[i] = w;
            imgb->s[i] = w * sizeof(float);
            imgb->h[i] = imgb->ah[i] = imgb->e[i] = h;
            imgb->bsize[i] = imgb->s[i] * imgb->e[i];

            imgb->a[i] = imgb->baddr[i] = malloc(imgb->bsize[i]);
            if (imgb->a[i] == NULL)
            {
                logv0("cannot allocate picture buffer\n");
                return NULL;
            }
        }
        imgb->np = 3;
    }
    else
    {
        logv0("unsupported color space\n");
        if(imgb)free(imgb);
        return NULL;
    }

    imgb->cs = cs;
    return imgb;
}

static int write_data(char * fname, unsigned char * data, int size)
{
    FILE * fp;

    fp = fopen(fname, "ab");
    if(fp == NULL)
    {
        logv0("cannot open an writing file=%s\n", fname);
        return -1;
    }
    fwrite(data, 1, size, fp);
    fclose(fp);
    return 0;
}


#endif /* _XEVEA_APP_UTIL_H_ */


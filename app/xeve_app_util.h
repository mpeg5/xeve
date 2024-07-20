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
#include <math.h>
#include <stdarg.h>

#define VERBOSE_NONE               0
#define VERBOSE_ERROR              1
#define VERBOSE_SIMPLE             2
#define VERBOSE_FRAME              3


/* logging functions */
static void log_msg(char * filename, int line, const char *fmt, ...)
{
    char str[1024]={'\0',};
    if(filename != NULL && line >= 0) sprintf(str, "[%s:%d] ", filename, line);
    va_list args;
    va_start(args, fmt);
    vsprintf(str + strlen(str), fmt, args);
    va_end(args);
    printf("%s", str);
}

static void log_line(char * pre)
{
    int i, len;
    char str[128]={'\0',};
    const int chars = 80;
    for(i = 0 ; i< 3; i++) {str[i] = '=';}
    str[i] = '\0';

    len = (pre == NULL)? 0: (int)strlen(pre);
    if(len > 0)
    {
        sprintf(str + 3, " %s ", pre);
        len = (int)strlen(str);
    }

    for(i = len ; i< chars; i++) {str[i] = '=';}
    str[chars] = '\0';
    printf("%s\n", str);
}


#define MAX_INP_STR_SIZE 128

#if defined(__GNUC__)
#define __FILENAME__ \
    (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#define logerr(args...) {if(op_verbose >= VERBOSE_ERROR) {log_msg(__FILENAME__, __LINE__, args);}}
#define logv2(args...) {if(op_verbose >= VERBOSE_SIMPLE) {log_msg(NULL, -1, args);}}
#define logv3(args...) {if(op_verbose >= VERBOSE_FRAME) {log_msg(NULL, -1, args);}}
#else
#define __FILENAME__ \
    (strrchr(__FILE__, '\\') ? strrchr(__FILE__, '\\') + 1 : __FILE__)
#define logerr(args,...) \
    {if(op_verbose >= VERBOSE_ERROR){log_msg(__FILENAME__, __LINE__, args, __VA_ARGS__);}}
#define logv2(args,...) \
    {if(op_verbose >= VERBOSE_SIMPLE){log_msg(NULL, -1, args,__VA_ARGS__);}}
#define logv3(args,...) \
    {if(op_verbose >= VERBOSE_FRAME){log_msg(NULL, -1, args,__VA_ARGS__);}}
#endif
#define logv2_line(pre) {if(op_verbose >= VERBOSE_SIMPLE) {log_line(pre);}}
#define logv3_line(pre) {if(op_verbose >= VERBOSE_FRAME) {log_line(pre);}}


static int op_verbose = VERBOSE_SIMPLE;

/* Clocks */
#if defined(_WIN64) || defined(_WIN32)
#include <windows.h>

#define XEVE_CLK             DWORD
#define XEVE_CLK_PER_SEC     (1000)
#define XEVE_CLK_PER_MSEC    (1)
#define XEVE_CLK_MAX         ((XEVE_CLK)(-1))
#define xeve_clk_get()       GetTickCount()

#elif __linux__ || __CYGWIN__ || __APPLE__
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

static int imgb_read(FILE * fp, XEVE_IMGB * img, int width, int height, int is_y4m)
{
    int f_w, f_h;
    int y_size, u_size, v_size;
    unsigned char * p8;

    /* handling Y4M frame header */
    char t_buf[10];
    if (is_y4m)
    {
        if (6 != fread(t_buf, 1, 6, fp)) return -1;
        if (memcmp(t_buf, "FRAME", 5))
        {
            logerr("Loss of framing in Y4M input data\n");
            return -1;
        }
        if (t_buf[5] != '\n')
        {
            logerr("Error parsing Y4M frame header\n");
            return -1;
        }
    }

    /* reading YUV format */
    int chroma_format = XEVE_CS_GET_FORMAT(img->cs);
    int bit_depth = XEVE_CS_GET_BIT_DEPTH(img->cs);
    int w_shift = (chroma_format == XEVE_CF_YCBCR420) || (chroma_format == XEVE_CF_YCBCR422) ? 1 : 0;
    int h_shift = chroma_format == XEVE_CF_YCBCR420 ? 1 : 0;

    if (bit_depth == 8)
    {
        f_w = width;
        f_h = height;
    }
    else if (bit_depth >= 10 && bit_depth <= 14)
    {
        f_w = width * sizeof(short);
        f_h = height;
    }
    else
    {
        logerr("not supported color space\n");
        return -1;
    }
               
    p8 = (unsigned char*)img->a[0];
    for (int j = 0; j < f_h; j++)
    {
        if (fread(p8, 1, f_w, fp) != (unsigned)f_w)
        {
            return -1;
        }
        p8 += img->s[0];
    }


    if (chroma_format != XEVE_CF_YCBCR400)
    {
        f_w = f_w >> w_shift;
        f_h = f_h >> h_shift;

        p8 = (unsigned char*)img->a[1];
        for (int j = 0; j < f_h; j++)
        {
            if (fread(p8, 1, f_w, fp) != (unsigned)f_w)
            {
                return -1;
            }
            p8 += img->s[1];
        }

        p8 = (unsigned char*)img->a[2];
        for (int j = 0; j < f_h; j++)
        {
            if (fread(p8, 1, f_w, fp) != (unsigned)f_w)
            {
                return -1;
            }
            p8 += img->s[2];
        }

    }
   

    return 0;
}

static int imgb_write(char * fname, XEVE_IMGB * imgb, int width, int height)
{
    unsigned char * p8;
    int             i, j, bd;
    int             cs_w_off, cs_h_off;
    FILE          * fp;

    int chroma_format = XEVE_CS_GET_FORMAT(imgb->cs);
    int bit_depth = XEVE_CS_GET_BIT_DEPTH(imgb->cs);
    int w_shift = (chroma_format == XEVE_CF_YCBCR420) || (chroma_format == XEVE_CF_YCBCR422) ? 1 : 0;
    int h_shift = chroma_format == XEVE_CF_YCBCR420 ? 1 : 0;
    fp = fopen(fname, "ab");
    if(fp == NULL)
    {
        logerr("cannot open file = %s\n", fname);
        return -1;
    }
    if(bit_depth == 8 && (chroma_format == XEVE_CF_YCBCR400 || chroma_format == XEVE_CF_YCBCR420 || chroma_format == XEVE_CF_YCBCR422 || chroma_format == XEVE_CF_YCBCR444))
    {
        bd = 1;
        cs_w_off = 2;
        cs_h_off = 2;
    }
    else if(bit_depth >= 10 && bit_depth <= 14 && (chroma_format == XEVE_CF_YCBCR400 || chroma_format == XEVE_CF_YCBCR420 || chroma_format == XEVE_CF_YCBCR422 || chroma_format == XEVE_CF_YCBCR444))
    {
        bd = 2;
        cs_w_off = 2;
        cs_h_off = 2;
    }
    else
    {
        logerr("cannot support the color space\n");
        fclose(fp);
        return -1;
    }

    for (i = 0; i < imgb->np; i++)
    {
        p8 = (unsigned char *)imgb->a[i] + (imgb->s[i] * imgb->y[i]) + (imgb->x[i] * bd);
        if (i == 1)
        {
            width = width >> w_shift;
            height = height >> h_shift;
        }
        for (j = 0; j < height; j++)
        {
            fwrite(p8, width * bd, 1, fp);
            p8 += imgb->s[i];
        }
    }

    fclose(fp);
    return 0;
}

static void imgb_cpy_plane(XEVE_IMGB * dst, XEVE_IMGB * src)
{
    int i, j;
    unsigned char *s, *d;
    int numbyte = XEVE_CS_GET_BYTE_DEPTH(src->cs);

    for(i = 0; i < src->np; i++)
    {
        s = (unsigned char*)src->a[i];
        d = (unsigned char*)dst->a[i];

        for(j = 0; j < src->ah[i]; j++)
        {
            memcpy(d, s, numbyte * src->aw[i]);
            s += src->s[i];
            d += dst->s[i];
        }
    }
}

static void imgb_cpy_shift_left_8b(XEVE_IMGB * dst, XEVE_IMGB * src, int shift)
{
    int i, j, k;

    unsigned char *s;
    short         *d;

    for(i = 0; i < dst->np; i++)
    {
        s = src->a[i];
        d = dst->a[i];

        for(j = 0; j < src->ah[i]; j++)
        {
            for(k = 0; k < src->aw[i]; k++)
            {
                d[k] = (short)(s[k] << shift);
            }
            s = s + src->s[i];
            d = (short*)(((unsigned char *)d) + dst->s[i]);
        }
    }
}

static void imgb_cpy_shift_right_8b(XEVE_IMGB *dst, XEVE_IMGB *src, int shift)
{
    int i, j, k, t0, add;

    short         *s;
    unsigned char *d;

    if(shift)
    add = 1 << (shift - 1);
    else
        add = 0;

    for(i = 0; i < dst->np; i++)
    {
        s = src->a[i];
        d = dst->a[i];

        for(j = 0; j < src->ah[i]; j++)
        {
            for(k = 0; k < src->aw[i]; k++)
            {
                t0 = ((s[k] + add) >> shift);
                d[k] = (unsigned char)(XEVEA_CLIP(t0, 0, 255));

            }
            s = (short*)(((unsigned char *)s) + src->s[i]);
            d = d + dst->s[i];
        }
    }
}

static void imgb_cpy_shift_left(XEVE_IMGB *dst, XEVE_IMGB *src, int shift)
{
    int i, j, k;

    unsigned short * s;
    unsigned short * d;

    for(i = 0; i < dst->np; i++)
    {
        s = src->a[i];
        d = dst->a[i];

        for(j = 0; j < src->h[i]; j++)
        {
            for(k = 0; k < src->w[i]; k++)
            {
                d[k] = (unsigned short)(s[k] << shift);
            }
            s = (short*)(((unsigned char *)s) + src->s[i]);
            d = (short*)(((unsigned char *)d) + dst->s[i]);
        }
    }
}

static void imgb_cpy_shift_right(XEVE_IMGB * dst, XEVE_IMGB * src, int shift)
{

    int i, j, k, t0, add;

    int clip_min = 0;
    int clip_max = 0;

    unsigned short         * s;
    unsigned short         * d;

    if(shift) add = 1 << (shift - 1);
    else add = 0;

    clip_max = (1 << (XEVE_CS_GET_BIT_DEPTH(dst->cs))) - 1;

    for(i = 0; i < dst->np; i++)
    {
        s = src->a[i];
        d = dst->a[i];

        for(j = 0; j < src->h[i]; j++)
        {
            for(k = 0; k < src->w[i]; k++)
            {
                t0 = ((s[k] + add) >> shift);
                d[k] = (XEVEA_CLIP(t0, clip_min, clip_max));

            }
            s = (short*)(((unsigned char *)s) + src->s[i]);
            d = (short*)(((unsigned char *)d) + dst->s[i]);
        }
    }
}

static void imgb_cpy(XEVE_IMGB * dst, XEVE_IMGB * src)
{
    int i, bd_src, bd_dst;
    bd_src = XEVE_CS_GET_BIT_DEPTH(src->cs);
    bd_dst = XEVE_CS_GET_BIT_DEPTH(dst->cs);

    if(src->cs == dst->cs)
    {
        imgb_cpy_plane(dst, src);
    }
    else if(bd_src == 8 && bd_dst > 8)
    {
        imgb_cpy_shift_left_8b(dst, src, bd_dst - bd_src);
    }
    else if(bd_src > 8 && bd_dst == 8)
    {
        imgb_cpy_shift_right_8b(dst, src, bd_src - bd_dst);
    }
    else if(bd_src < bd_dst)
    {
        imgb_cpy_shift_left(dst, src, bd_dst - bd_src);
    }
    else if(bd_src > bd_dst)
    {
        imgb_cpy_shift_right(dst, src, bd_src - bd_dst);
    }
    else
    {
        logerr("ERROR: unsupported image copy\n");
        return;
    }
    for(i = 0; i < XEVE_IMGB_MAX_PLANE; i++)
    {
        dst->x[i] = src->x[i];
        dst->y[i] = src->y[i];
        dst->w[i] = src->w[i];
        dst->h[i] = src->h[i];
    }
    for (i = 0; i < XEVE_TS_NUM; i++)
    {
        dst->ts[i] = src->ts[i];
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
    int i, bd;
    XEVE_IMGB * imgb;

    imgb = (XEVE_IMGB *)malloc(sizeof(XEVE_IMGB));
    if(imgb == NULL) goto ERR;
    memset(imgb, 0, sizeof(XEVE_IMGB));

    bd = XEVE_CS_GET_BYTE_DEPTH(cs); /* byte unit */

    imgb->w[0] = w;
    imgb->h[0] = h;
    switch(XEVE_CS_GET_FORMAT(cs))
    {
    case XEVE_CF_YCBCR400:
        imgb->w[1] = imgb->w[2] = w;
        imgb->h[1] = imgb->h[2] = h;
        imgb->np = 1;
        break;
    case XEVE_CF_YCBCR420:
        imgb->w[1] = imgb->w[2] = (w + 1) >> 1;
        imgb->h[1] = imgb->h[2] = (h + 1) >> 1;
        imgb->np = 3;
        break;
    case XEVE_CF_YCBCR422:
        imgb->w[1] = imgb->w[2] = (w + 1) >> 1;
        imgb->h[1] = imgb->h[2] = h;
        imgb->np = 3;
        break;
    case XEVE_CF_YCBCR444:
        imgb->w[1] = imgb->w[2] = w;
        imgb->h[1] = imgb->h[2] = h;
        imgb->np = 3;
        break;
    default:
        logv3("unsupported color format\n");
        goto ERR;
    }

    for(i = 0; i < imgb->np; i++)
    {
        imgb->aw[i] = imgb->w[i]; /* TODO: check this (no align?) */
        imgb->s[i] = imgb->aw[i] * bd;
        imgb->ah[i] = imgb->h[i]; /* TODO: check this (no align?) */
        imgb->e[i] = imgb->ah[i];
        imgb->bsize[i] = imgb->s[i] * imgb->e[i];
        imgb->a[i] = imgb->baddr[i] = malloc(imgb->bsize[i]); /* CHECK */
        if(imgb->a[i] == NULL) goto ERR;
        memset(imgb->a[i], 0, imgb->bsize[i]);
    }
    imgb->cs = cs;
    return imgb;

ERR:
    logerr("cannot create image buffer\n");
    if(imgb)
    {
        for (int i = 0; i < XEVE_IMGB_MAX_PLANE; i++)
        {
            if(imgb->a[i]) free(imgb->a[i]);
        }
        free(imgb);
    }
    return NULL;
}

#define MAX_BUMP_FRM_CNT           (16 <<2)

typedef struct _IMGB_LIST {
    XEVE_IMGB  * imgb;
    int          used;
    XEVE_MTIME   ts;
} IMGB_LIST;

static int imgb_list_alloc(IMGB_LIST *list, int w, int h, int bit_depth, int chroma_format)
{
    int i;

    memset(list, 0, sizeof(IMGB_LIST) * MAX_BUMP_FRM_CNT);

    for(i=0; i<MAX_BUMP_FRM_CNT; i++)
    {
        list[i].imgb = imgb_alloc(w, h, XEVE_CS_SET(chroma_format, bit_depth, 0));
        if(list[i].imgb == NULL) goto ERR;
    }
    return 0;

ERR:
    for(i=0; i<MAX_BUMP_FRM_CNT; i++)
    {
        if(list[i].imgb){ imgb_free(list[i].imgb); list[i].imgb = NULL; }
    }
    return -1;
}

static void imgb_list_free(IMGB_LIST *list)
{
    int i;

    for(i=0; i<MAX_BUMP_FRM_CNT; i++)
    {
        if(list[i].imgb){ imgb_free(list[i].imgb); list[i].imgb = NULL; }
    }
}

static IMGB_LIST *imgb_list_put(IMGB_LIST *list, XEVE_IMGB *imgb, XEVE_MTIME ts)
{
    int i;

    /* store original imgb for XEVE_TUNE_PSNR */
    for(i=0; i<MAX_BUMP_FRM_CNT; i++)
    {
        if(list[i].used == 0)
        {
            imgb_cpy(list[i].imgb, imgb);
            list[i].used = 1;
            list[i].ts = ts;
            return &list[i];
        }
    }
    return NULL;
}

static IMGB_LIST *imgb_list_get_empty(IMGB_LIST *list)
{
    int i;

    /* store original imgb for XEVE_TUNE_PSNR */
    for(i=0; i<MAX_BUMP_FRM_CNT; i++)
    {
        if(list[i].used == 0)
        {
            return &list[i];
        }
    }
    return NULL;
}

static int imgb_list_find_idx(IMGB_LIST *list, XEVE_MTIME ts)
{
    int i;

    for(i=0; i<MAX_BUMP_FRM_CNT; i++)
    {
        if(list[i].ts == ts && list[i].used == 1)
        {
            return i;
        }
    }
    return -1;
}

static void imgb_list_make_used(IMGB_LIST *list, XEVE_MTIME ts)
{
    list->used = 1;
    list->ts = list->imgb->ts[0] = ts;
}

static void imgb_list_make_unused(IMGB_LIST *list)
{
    list->used = 0;
}

static void imgb_list_find_and_make_unused(IMGB_LIST *list, XEVE_MTIME ts)
{
    int idx;
    idx = imgb_list_find_idx(list, ts);
    if(idx >= 0) list[idx].used = 0;
}

static IMGB_LIST * imgb_list_find(IMGB_LIST *list, XEVE_MTIME ts)
{
    int idx;
    idx = imgb_list_find_idx(list, ts);
    if (idx >= 0) return &list[idx];
    else return NULL;
}

static void find_psnr_16bit(XEVE_IMGB * org, XEVE_IMGB * rec, double psnr[3], int bit_depth)
{
    double sum[3], mse[3];
    short *o, *r;
    int i, j, k;
    int factor = 1 << (bit_depth - 8);
    factor *= factor;
    for(i = 0; i<org->np; i++)
    {
        o = (short*)org->a[i];
        r = (short*)rec->a[i];
        sum[i] = 0;
        for(j = 0; j<org->h[i]; j++)
        {
            for(k = 0; k<org->w[i]; k++)
            {
                sum[i] += (o[k] - r[k]) * (o[k] - r[k]);
            }
            o = (short*)((unsigned char *)o + org->s[i]);
            r = (short*)((unsigned char *)r + rec->s[i]);
        }
        mse[i] = sum[i] / (org->w[i] * org->h[i]);
        psnr[i] = (mse[i] == 0.0) ? 100. : fabs(10 * log10(((255 * 255 * factor) / mse[i])));
    }
}

static void find_psnr_8bit(XEVE_IMGB * org, XEVE_IMGB * rec, double psnr[3])
{
    double sum[3], mse[3];
    unsigned char *o, *r;
    int i, j, k;

    for(i=0; i<org->np; i++)
    {
        o      = (unsigned char*)org->a[i];
        r      = (unsigned char*)rec->a[i];
        sum[i] = 0;

        for(j=0; j<org->h[i]; j++)
        {
            for(k=0; k<org->w[i]; k++)
            {
                sum[i] += (o[k] - r[k]) * (o[k] - r[k]);
            }

            o += org->s[i];
            r += rec->s[i];
        }
        mse[i] = sum[i] / (org->w[i] * org->h[i]);
        psnr[i] = (mse[i]==0.0) ? 100. : fabs( 10*log10(((255*255)/mse[i])) );
    }
}

static int cal_psnr(IMGB_LIST * imgblist_inp, XEVE_IMGB * rec, XEVE_MTIME ts,
    int inp_bit_depth, int out_bit_depth, double psnr[3])
{
    int i;
    XEVE_IMGB     *img = NULL;

    /* calculate XEVE_TUNE_PSNR */
    psnr[0] = psnr[1] = psnr[2] = 0.0f;

    for(i = 0; i < MAX_BUMP_FRM_CNT; i++)
    {
        if(imgblist_inp[i].ts == ts && imgblist_inp[i].used == 1)
        {
            if(out_bit_depth == inp_bit_depth)
            {
                if(out_bit_depth == 8)
                {
                    find_psnr_8bit(imgblist_inp[i].imgb, rec, psnr);
                }
                else /* if(out_bit_depth >= 10) */
                {
                    find_psnr_16bit(imgblist_inp[i].imgb, rec, psnr, out_bit_depth);
                }
            }
            else
            {
                if (out_bit_depth == 8)
                {
                    img = imgb_alloc(rec->aw[0], rec->ah[0], XEVE_CS_SET(XEVE_CS_GET_FORMAT(rec->cs), out_bit_depth, 0));
                    imgb_cpy(img, imgblist_inp[i].imgb);
                    find_psnr_8bit(img, rec, psnr);
                    imgb_free(img);
                }
                else
                {
                    img = imgb_alloc(rec->aw[0], rec->ah[0], XEVE_CS_SET(XEVE_CS_GET_FORMAT(rec->cs), out_bit_depth, 0));
                    imgb_cpy(img, imgblist_inp[i].imgb);
                    find_psnr_16bit(img, rec, psnr, out_bit_depth);
                    imgb_free(img);
                }
            }
            return 0;
        }
    }
    return -1;
}

static int write_data(char * fname, unsigned char * data, int size)
{
    FILE * fp;

    fp = fopen(fname, "ab");
    if(fp == NULL)
    {
        logerr("cannot open an writing file=%s\n", fname);
        return -1;
    }
    fwrite(data, 1, size, fp);
    fclose(fp);
    return 0;
}


#endif /* _XEVEA_APP_UTIL_H_ */


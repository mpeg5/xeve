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

#ifndef _XEVE_BSW_H_
#define _XEVE_BSW_H_

#include "xeve_port.h"

typedef struct _XEVE_BSW XEVE_BSW;

/*! Function pointer for */
typedef int (*XEVE_BSW_FN_FLUSH)(XEVE_BSW *bs);

/*! Bitstream structure */
struct _XEVE_BSW
{
    /* buffer */
    u32                code;
    /* bits left in buffer */
    int                leftbits;
    /*! address of current writing position */
    u8               * cur;
    /*! address of bitstream buffer end */
    u8               * end;
    /*! address of bitstream buffer begin */
    u8               * beg;
    /*! size of bitstream buffer in byte */
    int                size;
    /*! address of function for flush */
    XEVE_BSW_FN_FLUSH  fn_flush;
    /*! arbitrary data, if needs */
    int                ndata[4];
    /*! arbitrary address, if needs */
    void             * pdata[4];
};

#define XEVE_BSW_IS_BYTE_ALIGN(bs)     !((bs)->leftbits & 0x7)

/* get number of byte written */
#define XEVE_BSW_GET_WRITE_BYTE(bs)    (int)((bs)->cur - (bs)->beg)

/* number of bytes to be sunk */
#define XEVE_BSW_GET_SINK_BYTE(bs)     ((32 - (bs)->leftbits + 7) >> 3)

void xeve_bsw_init(XEVE_BSW * bs, u8 * buf, int size, XEVE_BSW_FN_FLUSH fn_flush);
void xeve_bsw_init_slice(XEVE_BSW * bs, u8 * buf, int size, XEVE_BSW_FN_FLUSH fn_flush);
void xeve_bsw_deinit(XEVE_BSW * bs);
#if TRACE_HLS
#define xeve_bsw_write1(A, B) xeve_bsw_write1_trace(A, B, #B)
int xeve_bsw_write1_trace(XEVE_BSW * bs, int val, char* name);

#define xeve_bsw_write(A, B, C) xeve_bsw_write_trace(A, B, #B, C)
int xeve_bsw_write_trace(XEVE_BSW * bs, u32 val, char* name, int len);

#define xeve_bsw_write_ue(A, B) xeve_bsw_write_ue_trace(A, B, #B)
void xeve_bsw_write_ue_trace(XEVE_BSW * bs, u32 val, char* name);

#define xeve_bsw_write_se(A, B) xeve_bsw_write_se_trace(A, B, #B)
void xeve_bsw_write_se_trace(XEVE_BSW * bs, int val, char* name);
#else
int xeve_bsw_write1(XEVE_BSW * bs, int val);
int xeve_bsw_write(XEVE_BSW * bs, u32 val, int len);
void xeve_bsw_write_ue(XEVE_BSW * bs, u32 val);
void xeve_bsw_write_se(XEVE_BSW * bs, int val);
#endif

#endif /* _XEVE_BSW_H_ */

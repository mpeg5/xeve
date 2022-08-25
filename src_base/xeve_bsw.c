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

static int xeve_bsw_flush(XEVE_BSW * bs)
{
    int bytes = XEVE_BSW_GET_SINK_BYTE(bs);

    while(bytes--)
    {
        *bs->cur++ = (bs->code >> 24) & 0xFF;
        bs->code <<= 8;
    }

    bs->leftbits = 32;

    return 0;
}

void xeve_bsw_init(XEVE_BSW * bs, u8 * buf, int size, XEVE_BSW_FN_FLUSH fn_flush)
{
    bs->size = size;
    bs->beg = buf;
    bs->cur = buf;
    bs->end = buf + size - 1;
    bs->code = 0;
    bs->leftbits = 32;
    bs->fn_flush = (fn_flush == NULL ? xeve_bsw_flush : fn_flush);
}

void xeve_bsw_init_slice(XEVE_BSW * bs, u8 * buf, int size, XEVE_BSW_FN_FLUSH fn_flush)
{
    bs->size = size;
    bs->cur = buf;
    //bs->end = buf + size - 1;
    bs->code = 0;
    bs->leftbits = 32;
    bs->fn_flush = (fn_flush == NULL ? xeve_bsw_flush : fn_flush);
}

void xeve_bsw_deinit(XEVE_BSW * bs)
{
    bs->fn_flush(bs);
}

#if TRACE_HLS
void xeve_bsw_write_ue_trace(XEVE_BSW * bs, u32 val, char * name)
{
    int   len_i, len_c, info, nn;
    u32  code;

    if (name)
    {
        XEVE_TRACE_STR(name);
        XEVE_TRACE_STR(" ");
        XEVE_TRACE_INT(val);
        XEVE_TRACE_STR("\n");
    }

    nn = ((val + 1) >> 1);
    for (len_i = 0; len_i < 16 && nn != 0; len_i++)
    {
        nn >>= 1;
    }

    info = val + 1 - (1 << len_i);
    code = (1 << len_i) | ((info)& ((1 << len_i) - 1));

    len_c = (len_i << 1) + 1;

    xeve_bsw_write_trace(bs, code, 0, len_c);
}

int xeve_bsw_write_trace(XEVE_BSW * bs, u32 val, char * name, int len) /* len(1 ~ 32) */
{
    int leftbits;

    xeve_assert(bs);
    
    if (name)
    {
        XEVE_TRACE_STR(name);
        XEVE_TRACE_STR(" ");
        XEVE_TRACE_INT(val);
        XEVE_TRACE_STR("\n");
    }

    leftbits = bs->leftbits;
    val <<= (32 - len);
    bs->code |= (val >> (32 - leftbits));

    if (len < leftbits)
    {
        bs->leftbits -= len;
    }
    else
    {
        xeve_assert_rv(bs->cur + 4 <= bs->end, -1);

        bs->leftbits = 0;
        bs->fn_flush(bs);
        bs->code = (leftbits < 32 ? val << leftbits : 0);
        bs->leftbits = 32 - (len - leftbits);
    }

    return 0;
}

int xeve_bsw_write1_trace(XEVE_BSW * bs, int val, char * name)
{
    xeve_assert(bs);

    if (name)
    {
        XEVE_TRACE_STR(name);
        XEVE_TRACE_STR(" ");
        XEVE_TRACE_INT(val);
        XEVE_TRACE_STR("\n");
    }

    bs->leftbits--;
    bs->code |= ((val & 0x1) << bs->leftbits);

    if (bs->leftbits == 0)
    {
        xeve_assert_rv(bs->cur <= bs->end, -1);
        bs->fn_flush(bs);

        bs->code = 0;
        bs->leftbits = 32;
    }

    return 0;
}

void xeve_bsw_write_se_trace(XEVE_BSW * bs, int val, char * name)
{
    if (name)
    {
        XEVE_TRACE_STR(name);
        XEVE_TRACE_STR(" ");
        XEVE_TRACE_INT(val);
        XEVE_TRACE_STR("\n");
    }

    xeve_bsw_write_ue_trace(bs, val <= 0 ? (-val * 2) : (val * 2 - 1), 0);
}
#else
int xeve_bsw_write1(XEVE_BSW * bs, int val)
{
    xeve_assert(bs);

    bs->leftbits--;
    bs->code |= ((val & 0x1) << bs->leftbits);

    if (bs->leftbits == 0)
    {
        xeve_assert_rv(bs->cur <= bs->end, -1);
        bs->fn_flush(bs);

        bs->code = 0;
        bs->leftbits = 32;
    }

    return 0;
}

int xeve_bsw_write(XEVE_BSW * bs, u32 val, int len) /* len(1 ~ 32) */
{
    int leftbits;

    xeve_assert(bs);
    xeve_assert(len <= 32); // to avoid shifting by a negative value

    leftbits = bs->leftbits;
    val <<= (32 - len);
    bs->code |= (val >> (32 - leftbits));

    if(len < leftbits)
    {
        bs->leftbits -= len;
    }
    else
    {
        xeve_assert_rv(bs->cur + 4 <= bs->end, -1);

        bs->leftbits = 0;
        bs->fn_flush(bs);        
        bs->code = (leftbits < 32 ? val << leftbits : 0);
        bs->leftbits = 32 - (len - leftbits);
    }

    return 0;
}

void xeve_bsw_write_ue(XEVE_BSW * bs, u32 val)
{
    int   len_i, len_c, info, nn;
    u32  code;

    nn = ((val + 1) >> 1);
    for(len_i = 0; len_i < 16 && nn != 0; len_i++)
    {
        nn >>= 1;
    }

    info = val + 1 - (1 << len_i);
    code = (1 << len_i) | ((info)& ((1 << len_i) - 1));

    len_c = (len_i << 1) + 1;

    xeve_bsw_write(bs, code, len_c);
}

void xeve_bsw_write_se(XEVE_BSW * bs, int val)
{
    xeve_bsw_write_ue(bs, val <= 0 ? (-val * 2) : (val * 2 - 1));
}
#endif
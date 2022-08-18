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

#include <stdarg.h>
#include "xeve_port.h"

#if XEVE_TRACE
void xeve_trace0(char * filename, int line, const char *fmt, ...)
{
    char str[1024]={'\0',};
    if(filename != NULL && line >= 0) sprintf(str, "[%s:%d] ", filename, line);
    va_list args;
    va_start(args, fmt);
    vsprintf(str + strlen(str), fmt, args);
    va_end(args);
    printf("%s", str);
}

void xeve_trace_line(char * pre)
{
    char str[128]={'\0',};
    const int chars = 80;
    int len = (pre == NULL)? 0: (int)strlen(pre);
    if(len > 0)
    {
        sprintf(str, "%s ", pre);
        len = (int)strlen(str);
    }
    for(int i = len ; i< chars; i++) {str[i] = '=';}
    str[chars] = '\0';
    printf("%s\n", str);
}

#endif /* XEVE_TRACE */

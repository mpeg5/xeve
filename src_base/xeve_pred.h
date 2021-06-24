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

#ifndef _XEVE_PRED_H_
#define _XEVE_PRED_H_

#include "xeve_type.h"

/* Intra prediction */
int xeve_pintra_create(XEVE_CTX * ctx, int complexity);
int xeve_pintra_set_complexity(XEVE_CTX * ctx, int complexity);
int xeve_pintra_init_mt(XEVE_CTX * ctx, int tile_idx);
int xeve_pintra_analyze_lcu(XEVE_CTX * ctx, XEVE_CORE * core);
double xeve_pinter_analyze_cu(XEVE_CTX *ctx, XEVE_CORE *core, int x, int y, int log2_cuw, int log2_cuh, XEVE_MODE *mi, s16 coef[N_C][MAX_CU_DIM], pel *rec[N_C], int s_rec[N_C]);
double xeve_pintra_analyze_cu_simple(XEVE_CTX* ctx, XEVE_CORE* core, int x, int y, int log2_cuw, int log2_cuh, s16 coef[N_C][MAX_CU_DIM]);
int    xeve_pinter_init_lcu(XEVE_CTX *ctx, XEVE_CORE *core);

/* Inter prediction */
extern const XEVE_PRED_INTER_COMP tbl_inter_pred_comp[2];

#define BI_ITER                            4
#define MAX_FIRST_SEARCH_STEP              3
#define MAX_REFINE_SEARCH_STEP             2
#define RASTER_SEARCH_STEP                 5
#define RASTER_SEARCH_THD                  5
#define REFINE_SEARCH_THD                  0
#define BI_STEP                            5

int xeve_pinter_create(XEVE_CTX * ctx, int complexity);

#endif /* _XEVE_PRED_H_ */

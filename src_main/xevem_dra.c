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

#include "xevem_type.h"
#include <math.h>


void prec_quantize_entry_i(QUANT_PARAM_DRA *quant_param_entry, int const value, int const int_bits)
{
    int temp = (int)floor(value + 0.5);
    quant_param_entry->value = temp;
    quant_param_entry->num_frac_bits = 0;
    if (temp == 0)
    {
        quant_param_entry->num_frac_bits = 0;
        quant_param_entry->num_tot_bits = 1;
    }
    else
    {
        int est_bits = (int)ceil(log(abs(temp) + 0.0) / log(2.0));
        quant_param_entry->num_tot_bits = XEVE_MIN(est_bits, int_bits);
    }
}
void prec_quantize_entry_d(QUANT_PARAM_DRA *quant_param_entry, double const value, int const fracBits, int const int_bits)
{
    int temp = (int)floor(value*(1 << fracBits) + 0.5);
    quant_param_entry->value = temp;
    quant_param_entry->num_frac_bits = fracBits;
    if (temp == 0)
    {
        quant_param_entry->num_frac_bits = 0;
        quant_param_entry->num_tot_bits = 1;
    }
    else
    {
        int est_bits = (int)ceil(log(abs(temp) + 0.0) / log(2.0));
        quant_param_entry->num_tot_bits = XEVE_MIN(est_bits, int_bits + fracBits);
    }
}

float get_val(QUANT_PARAM_DRA *quant_param_entry)
{
    return (float)(quant_param_entry->value + 0.0) / (1 << quant_param_entry->num_frac_bits);
}

void rshift(QUANT_PARAM_DRA *value_this, int const val)
{
    int shift_val = 1 << (val - 1);
    value_this->value = (value_this->value + shift_val) >> val;
    value_this->num_frac_bits -= val;
}
void lshift(QUANT_PARAM_DRA *value_this, int const val)
{
    value_this->value <<= val;
    value_this->num_frac_bits += val;
    value_this->num_tot_bits += val;
}
QUANT_PARAM_DRA prec_plus_entry(QUANT_PARAM_DRA value_this, const QUANT_PARAM_DRA rhs)
{
    QUANT_PARAM_DRA this_prec;
    QUANT_PARAM_DRA temp = rhs;
    QUANT_PARAM_DRA tempL = value_this;
    if (value_this.num_frac_bits != rhs.num_frac_bits)
    {
        int num_frac_bits_final = XEVE_MAX(tempL.num_frac_bits, temp.num_frac_bits);
        lshift(&tempL, num_frac_bits_final - tempL.num_frac_bits);
        lshift(&temp, num_frac_bits_final - temp.num_frac_bits);

        this_prec.num_frac_bits = num_frac_bits_final;
    }
    else
    {
        this_prec.num_frac_bits = rhs.num_frac_bits;
    }
    this_prec.value = tempL.value + temp.value;
    this_prec.num_tot_bits = XEVE_MAX(tempL.num_tot_bits, rhs.num_tot_bits) + 1;
    return this_prec;
}
QUANT_PARAM_DRA prec_minus_entry(QUANT_PARAM_DRA value_this, const QUANT_PARAM_DRA rhs)
{
    QUANT_PARAM_DRA this_prec;
    QUANT_PARAM_DRA temp = rhs;
    QUANT_PARAM_DRA tempL = value_this;
    if (value_this.num_frac_bits != rhs.num_frac_bits)
    {
        int num_frac_bits_final = XEVE_MAX(tempL.num_frac_bits, temp.num_frac_bits);
        lshift(&tempL, num_frac_bits_final - value_this.num_frac_bits);
        lshift(&temp, num_frac_bits_final - temp.num_frac_bits);

        this_prec.num_frac_bits = num_frac_bits_final;
    }
    else
    {
        this_prec.num_frac_bits = rhs.num_frac_bits;
    }
    this_prec.value = tempL.value - temp.value;
    this_prec.num_tot_bits = XEVE_MAX(tempL.num_tot_bits, rhs.num_tot_bits) + 1;
    return this_prec;
}
QUANT_PARAM_DRA prec_mult_entry(QUANT_PARAM_DRA value_this, const QUANT_PARAM_DRA rhs)
{
    QUANT_PARAM_DRA this_prec;
    this_prec.value = value_this.value * rhs.value;
    if (this_prec.value == 0)
    {
        this_prec.num_tot_bits = 1;
        this_prec.num_frac_bits = 0;
    }
    else
    {
        this_prec.num_tot_bits = value_this.num_tot_bits + rhs.num_tot_bits;
        this_prec.num_frac_bits = value_this.num_frac_bits + rhs.num_frac_bits;
    }
    return this_prec;
}
QUANT_PARAM_DRA prec_divide_entry(QUANT_PARAM_DRA value_this, QUANT_PARAM_DRA const rhs)
{
    QUANT_PARAM_DRA this_prec;
    this_prec.value = (value_this.value + (rhs.value / 2)) / rhs.value;
    if (this_prec.value == 0)
    {
        this_prec.num_tot_bits = 1;
        this_prec.num_frac_bits = 0;
    }
    else
    {
        this_prec.num_tot_bits = value_this.num_tot_bits - rhs.num_tot_bits;
        this_prec.num_frac_bits = value_this.num_frac_bits - rhs.num_frac_bits;
    }
    return this_prec;
}
void set_frac_bits(QUANT_PARAM_DRA *value_this, int const nBits)
{
    if (value_this->num_frac_bits < nBits)
    {
        lshift(value_this, nBits - value_this->num_frac_bits);
    }
    else if (value_this->num_frac_bits > nBits)
    {
        rshift(value_this, value_this->num_frac_bits - nBits);
    }
    if (value_this->value == 0)
    {
        value_this->num_tot_bits = 0;
    }
    else
    {
        int est_bits = (int)ceil(log(abs(value_this->value) + 0.0) / log(2.0));
        value_this->num_tot_bits = est_bits;
    }
}

// Common functions
int xeve_get_scaled_chroma_qp2(XEVE_CTX * ctx, int comp_id, int unscaledChromaQP, int bit_depth)
{
    int qp_bd_offset_c = 6 * (bit_depth - 8);
    int qp_value = XEVE_CLIP3(-qp_bd_offset_c, XEVE_MAX_QP_TABLE_SIZE - 1, unscaledChromaQP);
    qp_value = ctx->qp_chroma_dynamic[comp_id - 1][qp_value];
    return qp_value;
}

int xeve_get_dra_range_idx_gen(DRA_CONTROL *dra_mapping, int sample, int *chromaRanges, int numRanges)
{
    int range_idx = -1;
    for (int i = 0; i < numRanges; i++)
    {
        if (sample < chromaRanges[i + 1])
        {
            range_idx = i;
            break;
        }
    }
    if (range_idx == -1)
        range_idx = numRanges - 1;

    return XEVE_MIN(range_idx, numRanges - 1);

}
int xeve_correct_local_chroma_scale(XEVE_CTX * ctx, DRA_CONTROL *dra_mapping, int int_scaleLumaDra, int chId, int bit_depth)
{
    int l_array[NUM_CHROMA_QP_OFFSET_LOG];
    xeve_mcpy(l_array, xevem_dra_chroma_qp_offset_tbl, NUM_CHROMA_QP_OFFSET_LOG * sizeof(int));
    int scale_offset = 1 << DRA_SCALE_NUMFBITS;
    int table0_shift = NUM_CHROMA_QP_SCALE_EXP >> 1;
    int out_chroma_scale = 1;

    int local_qp;
    int qp0, qp1;
    int scale_dra_int = 1;
    int qp_dra_frac = 0;
    if (dra_mapping->chroma_qp_model.dra_table_idx == 58)
    {
        return    scale_dra_int = (chId == 1) ? dra_mapping->dra_cb_scale_value : dra_mapping->dra_cr_scale_value;
    }
    else
    {
        scale_dra_int = (chId == 1) ? dra_mapping->dra_cb_scale_value * int_scaleLumaDra : dra_mapping->dra_cr_scale_value * int_scaleLumaDra;
        int local_chroma_qp_shift1 = dra_mapping->chroma_qp_model.dra_table_idx - (xeve_get_scaled_chroma_qp2(ctx, chId, dra_mapping->chroma_qp_model.dra_table_idx, bit_depth));
        int qp_dra_int = 0;
        int out_of_range = -1;
        int scale_dra_int9 = (scale_dra_int + (1 << 8)) >> 9;
        int index_scale_qp = xeve_get_dra_range_idx_gen(dra_mapping, scale_dra_int9, l_array, NUM_CHROMA_QP_OFFSET_LOG - 1);
        int interpolation_num = scale_dra_int9 - xevem_dra_chroma_qp_offset_tbl[index_scale_qp];  //delta_scale (1.2QP)  = 0.109375
        int interpolation_denom = xevem_dra_chroma_qp_offset_tbl[index_scale_qp + 1] - xevem_dra_chroma_qp_offset_tbl[index_scale_qp];  // DenomScale (2QP) = 0.232421875

        qp_dra_int = 2 * index_scale_qp - 60;  // index table == 0, associated QP == - 1

        if (interpolation_num == 0)
        {
            qp_dra_int -= 1;
            qp_dra_frac = 0;
        }
        else
        {
            qp_dra_frac = scale_offset * (interpolation_num << 1) / interpolation_denom;
            qp_dra_int += qp_dra_frac / scale_offset;  // 0
            qp_dra_frac = scale_offset - (qp_dra_frac % scale_offset);
        }
        local_qp = dra_mapping->chroma_qp_model.dra_table_idx - qp_dra_int;
        qp0 = xeve_get_scaled_chroma_qp2(ctx, chId, XEVE_CLIP3(-(6 * (bit_depth - 8)), 57, local_qp), bit_depth);
        qp1 = xeve_get_scaled_chroma_qp2(ctx, chId, XEVE_CLIP3(-(6 * (bit_depth - 8)), 57, local_qp + 1), bit_depth);

        int qp_ch_dec = (qp1 - qp0) * qp_dra_frac;
        int qp_dra_frac_adj = qp_ch_dec % (1 << 9);
        int qp_dra_int_adj = (qp_ch_dec >> 9);

        qp_dra_frac_adj = qp_dra_frac - qp_dra_frac_adj;
        int local_chroma_qp_shift2 = local_qp - qp0 - qp_dra_int_adj;

        int dra_chroma_qp_shift = local_chroma_qp_shift2 - local_chroma_qp_shift1;
        if (qp_dra_frac_adj < 0)
        {
            dra_chroma_qp_shift -= 1;
            qp_dra_frac_adj = (1 << 9) + qp_dra_frac_adj;
        }
        int dra_chroma_qp_shift_clipped = XEVE_CLIP3(-12, 12, dra_chroma_qp_shift);
        int dra_chroma_scale_shift = xevem_dra_exp_nom_v2[dra_chroma_qp_shift_clipped + table0_shift];

        int draChromaScaleShiftFrac;
        if (dra_chroma_qp_shift >= 0)
        {
            draChromaScaleShiftFrac = xevem_dra_exp_nom_v2[XEVE_CLIP3(-12, 12, dra_chroma_qp_shift + 1) + table0_shift] - xevem_dra_exp_nom_v2[dra_chroma_qp_shift_clipped + table0_shift];
        }
        else
        {
            draChromaScaleShiftFrac = xevem_dra_exp_nom_v2[dra_chroma_qp_shift_clipped + table0_shift] - xevem_dra_exp_nom_v2[XEVE_CLIP3(-12, 12, dra_chroma_qp_shift - 1) + table0_shift];
        }

        out_chroma_scale = dra_chroma_scale_shift + ((draChromaScaleShiftFrac * qp_dra_frac_adj + (1 << (DRA_SCALE_NUMFBITS - 1))) >> DRA_SCALE_NUMFBITS);
        return (scale_dra_int * out_chroma_scale + (1 << 17)) >> 18;
    }
}
void xeve_compensate_chroma_shift_table(XEVE_CTX * ctx, DRA_CONTROL *dra_mapping, int bit_depth)
{
    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        dra_mapping->chroma_dra_scales_s32[0][i] = xeve_correct_local_chroma_scale(ctx, dra_mapping, dra_mapping->dra_scales_s32[i], 1, bit_depth);
        dra_mapping->chroma_dra_scales_s32[1][i] = xeve_correct_local_chroma_scale(ctx, dra_mapping, dra_mapping->dra_scales_s32[i], 2, bit_depth);
        dra_mapping->chroma_inv_dra_scales_s32[0][i] = ( (1 << 18) + (dra_mapping->chroma_dra_scales_s32[0][i] >> 1) ) / dra_mapping->chroma_dra_scales_s32[0][i];
        dra_mapping->chroma_inv_dra_scales_s32[1][i] = ( (1 << 18) + (dra_mapping->chroma_dra_scales_s32[1][i] >> 1) ) / dra_mapping->chroma_dra_scales_s32[1][i];
    }
}
static void xeve_build_dra_luma_lut( DRA_CONTROL *dra_mapping)
{
    for (int i = 0; i < DRA_LUT_MAXSIZE; i++)
    {
        int range_idx_y = xeve_get_dra_range_idx_gen(dra_mapping, i, dra_mapping->out_ranges_s32, dra_mapping->num_ranges);
        int value = i * dra_mapping->inv_dra_scales_s32[range_idx_y];
        value = (dra_mapping->inv_dra_offsets_s32[range_idx_y] + value + (1 << 8)) >> 9;
        value = XEVE_CLIP3(0, DRA_LUT_MAXSIZE - 1, value);
        dra_mapping->xevem_luma_inv_scale_lut[i] = value;
    }
}
static void xeve_build_dra_chroma_lut(DRA_CONTROL *dra_mapping, int bit_depth)
{
    for (int i = 0; i < DRA_LUT_MAXSIZE; i++)
    {
        dra_mapping->int_chroma_scale_lut[0][i] = dra_mapping->int_chroma_scale_lut[1][i] = 1;
        dra_mapping->xevem_int_chroma_inv_scale_lut[0][i] = dra_mapping->int_chroma_scale_lut[1][i] = 1;
    }
    {
        int chroma_mult_ranges2[33 + 1];
        int chroma_mult_scale[33 + 1];
        int chroma_mult_offset[33 + 1];
        for (int ch = 0; ch < 2; ch++)
        {

            chroma_mult_ranges2[0] = dra_mapping->out_ranges_s32[0];
            chroma_mult_scale[0] = 0;
            chroma_mult_offset[0] = (int)(dra_mapping->chroma_inv_dra_scales_s32[ch][0]);
            for (int i = 1; i < dra_mapping->num_ranges + 1; i++)
            {
                chroma_mult_ranges2[i] = (int)((dra_mapping->out_ranges_s32[i - 1] + dra_mapping->out_ranges_s32[i]) / 2);
            }

            for (int i = 1; i < dra_mapping->num_ranges; i++)
            {
                int delta_range = chroma_mult_ranges2[i + 1] - chroma_mult_ranges2[i];
                chroma_mult_offset[i] = dra_mapping->chroma_inv_dra_scales_s32[ch][i - 1];
                int delta_scale = dra_mapping->chroma_inv_dra_scales_s32[ch][i] - chroma_mult_offset[i];
                chroma_mult_scale[i] = (int)(((delta_scale << bit_depth) + (delta_range >> 1)) / delta_range);
            }
            chroma_mult_scale[dra_mapping->num_ranges] = 0;
            chroma_mult_offset[dra_mapping->num_ranges] = dra_mapping->chroma_inv_dra_scales_s32[ch][dra_mapping->num_ranges - 1];

            for (int i = 0; i < DRA_LUT_MAXSIZE; i++)
            {
                int range_idx = xeve_get_dra_range_idx_gen(dra_mapping, i, chroma_mult_ranges2, dra_mapping->num_ranges + 1);
                int run_i = i - chroma_mult_ranges2[range_idx];
                int run_s = (chroma_mult_scale[range_idx] * run_i + (1 << (bit_depth - 1))) >> bit_depth;

                dra_mapping->xevem_int_chroma_inv_scale_lut[ch][i] = chroma_mult_offset[range_idx] + run_s;
            }
        }
    }
}

double xeve_get_qp2_scale_dra(int cb_qp)
{
    double scale_dra = 1.0;
    scale_dra = exp(((double)(cb_qp) / 6)*log(2.0));
    return scale_dra;
}
double xeve_get_cb_scale_dra(DRA_CHROMA_OFF_CONTROL *dra_chroma_control, int idx_qp)
{
    double scale_cb_dra = 1.0;
    double chroma_qp = dra_chroma_control->chroma_qp_scale * idx_qp + dra_chroma_control->chroma_qp_offset;
    chroma_qp = chroma_qp * dra_chroma_control->cb_qp_scale;
    int cb_qp = (int)(chroma_qp + (chroma_qp < 0 ? -0.5 : 0.5));
    cb_qp = XEVE_CLIP3(-12, 12, XEVE_MIN(0, cb_qp) + dra_chroma_control->dra_cb_qp_offset);
    cb_qp = cb_qp - dra_chroma_control->dra_cb_qp_offset;
    scale_cb_dra = xeve_get_qp2_scale_dra(cb_qp);
    scale_cb_dra = 1 / scale_cb_dra;  // chroma QP Offset is added to luma, which equialent of reduced scale factor 1/x
    return    scale_cb_dra;
}
double xeve_get_cr_scale_dra(DRA_CHROMA_OFF_CONTROL *dra_chroma_control, int idx_qp)
{
    double scale_cr_dra = 1.0;
    double chroma_qp = dra_chroma_control->chroma_qp_scale * idx_qp + dra_chroma_control->chroma_qp_offset;

    chroma_qp = chroma_qp * dra_chroma_control->cr_qp_scale;
    int crQP = (int)(chroma_qp + (chroma_qp < 0 ? -0.5 : 0.5));
    crQP = XEVE_CLIP3(-12, 12, XEVE_MIN(0, crQP) + dra_chroma_control->dra_cr_qp_offset);
    crQP = crQP - dra_chroma_control->dra_cr_qp_offset;
    scale_cr_dra = xeve_get_qp2_scale_dra(crQP);
    scale_cr_dra = 1 / scale_cr_dra;
    return    scale_cr_dra;
}
void xeve_zoom_in_range_lut(DRA_CONTROL *dra_mapping, int sdr_flag)
{
    double lum_renorm = 1.0;
    if (sdr_flag == 1)
    {
        dra_mapping->global_offset = 0;
        dra_mapping->global_end = DRA_LUT_MAXSIZE - 1;
        lum_renorm = 1.0;
    }
    if ((dra_mapping->global_offset == 0) && (dra_mapping->global_end == 0))
    {
        return;
    }

    int deltas[33] = {0};
    double scale_max = 1.7;
    lum_renorm = (double)(DRA_LUT_MAXSIZE) / (double)(DRA_LUT_MAXSIZE - (dra_mapping->global_offset + DRA_LUT_MAXSIZE - dra_mapping->global_end));

    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        deltas[i] = dra_mapping->in_ranges[i + 1] - dra_mapping->in_ranges[i];
    }
    lum_renorm = (lum_renorm > scale_max) ? scale_max : lum_renorm;

    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        deltas[i] = (int)(deltas[i] / lum_renorm + 0.5);
    }
    dra_mapping->in_ranges[0] = dra_mapping->global_offset;
    dra_mapping->dra_scales[0] = dra_mapping->dra_scales[0] * lum_renorm;
    for (int i = 1; i < dra_mapping->num_ranges; i++)
    {
        dra_mapping->in_ranges[i] = dra_mapping->in_ranges[i - 1] + deltas[i - 1];
        dra_mapping->dra_scales[i] = dra_mapping->dra_scales[i] * lum_renorm;
    }
    dra_mapping->in_ranges[dra_mapping->num_ranges] = dra_mapping->in_ranges[dra_mapping->num_ranges - 1] + deltas[dra_mapping->num_ranges - 1];

    dra_mapping->out_ranges[0] = 0;
    for (int i = 1; i < dra_mapping->num_ranges + 1; i++)
    {
        dra_mapping->out_ranges[i] = (int)(dra_mapping->out_ranges[i - 1] + dra_mapping->dra_scales[i - 1] * deltas[i - 1] + 0.5);
    }
    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        dra_mapping->dra_offets[i] = dra_mapping->in_ranges[i + 1] - dra_mapping->out_ranges[i + 1] / dra_mapping->dra_scales[i];
    }
    return;
}
void xeve_normalize_histogram_lut(DRA_CONTROL *dra_mapping, int sdr_flag, int bit_depth)
{

    if (sdr_flag == 1)
        return;

    int deltas[33];
    //-------- Normilize the scale to full range 0..1 --------//
    double scale_norm = ((int)(100.0*dra_mapping->dra_hist_norm + 0.5)) / 100.0;


    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        dra_mapping->dra_scales[i] = dra_mapping->dra_scales[i] / scale_norm;
    }

    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        deltas[i] = dra_mapping->in_ranges[i + 1] - dra_mapping->in_ranges[i];
    }

    QUANT_PARAM_DRA accum, temp1, temp2, temp3;
    QUANT_PARAM_DRA out_ranges[33];
    QUANT_PARAM_DRA dra_offsets[32];
    prec_quantize_entry_i(&(out_ranges[0]), 0, 1);
    for (int i = 1; i < dra_mapping->num_ranges + 1; i++)
    {
        prec_quantize_entry_d(&temp1, dra_mapping->dra_scales[i - 1], DRA_SCALE_NUMFBITS, 10);
        prec_quantize_entry_i(&temp2, deltas[i - 1], bit_depth + 1);

        temp3 = prec_mult_entry(temp1, temp2);
        out_ranges[i] = prec_plus_entry(out_ranges[i - 1], temp3);
    }

    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        prec_quantize_entry_d(&temp1, 1, DRA_SCALE_NUMFBITS + DRA_INVSCALE_NUMFBITS, 11);
        prec_quantize_entry_d(&temp2, dra_mapping->dra_scales[i], DRA_SCALE_NUMFBITS, 10);
        accum = prec_divide_entry(temp1, temp2);
        temp3 = prec_mult_entry(out_ranges[i + 1], accum);
        prec_quantize_entry_d(&temp1, dra_mapping->in_ranges[i + 1], 0, bit_depth);
        dra_offsets[i] = prec_minus_entry(temp1, temp3);
        set_frac_bits(&(dra_offsets[i]), DRA_OFFSET_NUMFBITS);
    }

    for (int i = 0; i < dra_mapping->num_ranges + 1; i++)
    {
        set_frac_bits(&(out_ranges[i]), 0);
        dra_mapping->out_ranges[i] = get_val(&(out_ranges[i]));
    }
    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        dra_mapping->dra_offets[i] = get_val(&(dra_offsets[i]));
    }

    return;
}
void xeve_construct_dra(DRA_CONTROL *dra_mapping, int sdr_flag, BOOL use_fixed_pt, int bit_depth)
{
    if (sdr_flag == 1)
    {
        return;
    }

    int deltas[33];
    //-------- Normilize the scale to full range 0..1 --------//
    double scale_norm = (dra_mapping->out_ranges[dra_mapping->num_ranges] - dra_mapping->out_ranges[0]) / (dra_mapping->in_ranges[dra_mapping->num_ranges] - dra_mapping->in_ranges[0]);

    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        dra_mapping->dra_scales[i] = dra_mapping->dra_scales[i] / scale_norm;
    }

    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        deltas[i] = dra_mapping->in_ranges[i + 1] - dra_mapping->in_ranges[i];
    }
    if (use_fixed_pt == TRUE)
    {
        QUANT_PARAM_DRA accum, temp1, temp2, temp3;
        QUANT_PARAM_DRA out_ranges[33];
        QUANT_PARAM_DRA dra_offsets[32];
        prec_quantize_entry_i(&(out_ranges[0]), 0, 1);
        for (int i = 1; i < dra_mapping->num_ranges + 1; i++)
        {
            prec_quantize_entry_d(&temp1, dra_mapping->dra_scales[i - 1], DRA_SCALE_NUMFBITS, 10);
            prec_quantize_entry_i(&temp2, deltas[i - 1], bit_depth + 1);
            temp3 = prec_mult_entry(temp1, temp2);
            out_ranges[i] = prec_plus_entry(out_ranges[i - 1], temp3);
        }

        for (int i = 0; i < dra_mapping->num_ranges; i++)
        {
            prec_quantize_entry_d(&temp1, 1, DRA_SCALE_NUMFBITS + DRA_INVSCALE_NUMFBITS, 11);
            prec_quantize_entry_d(&temp2, dra_mapping->dra_scales[i], DRA_SCALE_NUMFBITS, 10);
            accum = prec_divide_entry(temp1, temp2);
            temp3 = prec_mult_entry(out_ranges[i + 1], accum);
            prec_quantize_entry_d(&temp1, dra_mapping->in_ranges[i + 1], 0, bit_depth);
            dra_offsets[i] = prec_minus_entry(temp1, temp3);
            set_frac_bits(&(dra_offsets[i]), DRA_OFFSET_NUMFBITS);
        }

        for (int i = 0; i < dra_mapping->num_ranges + 1; i++)
        {
            set_frac_bits(&(out_ranges[i]), 0);
            dra_mapping->out_ranges[i] = get_val(&(out_ranges[i]));
        }
        for (int i = 0; i < dra_mapping->num_ranges; i++)
        {
            dra_mapping->dra_offets[i] = get_val(&(dra_offsets[i]));
        }
    }
    else
    {
        dra_mapping->out_ranges[0] = 0;
        for (int i = 1; i < dra_mapping->num_ranges + 1; i++)
        {
            dra_mapping->out_ranges[i] = (int)(dra_mapping->out_ranges[i - 1] + dra_mapping->dra_scales[i - 1] * deltas[i - 1] + 0.5);
            dra_mapping->out_ranges[i] = XEVE_CLIP3(0.0, (double)DRA_LUT_MAXSIZE, dra_mapping->out_ranges[i]);
        }

        for (int i = 0; i < dra_mapping->num_ranges; i++)
        {
            dra_mapping->dra_offets[i] = dra_mapping->in_ranges[i + 1] - dra_mapping->out_ranges[i + 1] / dra_mapping->dra_scales[i];
        }
    }
    return;
}
void xeve_check_equal_range_flag(DRA_CONTROL *dra_mapping)
{
    SIG_PARAM_DRA * l_signalled_dra = &(dra_mapping->signalled_dra);
    BOOL ret_val_falg = TRUE;
    for (int i = 1; i < dra_mapping->num_ranges; i++)
    {
        if (dra_mapping->in_ranges[i + 1] - dra_mapping->in_ranges[i] != dra_mapping->in_ranges[i] - dra_mapping->in_ranges[i - 1])
        { // If one
            ret_val_falg = FALSE;
            break;
        }
    }
    if (ret_val_falg == TRUE)
    {
        l_signalled_dra->equal_ranges_flag = TRUE;
        l_signalled_dra->in_ranges[0] = dra_mapping->in_ranges[0];

        int deltaVal = (int)floor((1024 - dra_mapping->in_ranges[0] + 0.0) / dra_mapping->num_ranges + 0.5);
        l_signalled_dra->delta_val = deltaVal - (dra_mapping->in_ranges[1] - dra_mapping->in_ranges[0]);
    }
    else
    {
        l_signalled_dra->equal_ranges_flag = FALSE;
        for (int i = 0; i <= dra_mapping->num_ranges; i++)
        {
            l_signalled_dra->in_ranges[i] = dra_mapping->in_ranges[i];
        }
    }
}
void xeve_quatnize_params_dra(DRA_CONTROL *dra_mapping)
{
    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        dra_mapping->dra_scales[i] = XEVE_CLIP3(0, (1 << dra_mapping->dra_descriptor1), dra_mapping->dra_scales[i]);
        dra_mapping->dra_scales_s32[i] = (int)(dra_mapping->dra_scales[i] * (1 << dra_mapping->dra_descriptor2) + 0.5);
        dra_mapping->dra_scales[i] = (double)dra_mapping->dra_scales_s32[i] / (1 << dra_mapping->dra_descriptor2);
    }

}
void xeve_set_signalled_params_dra(DRA_CONTROL *dra_mapping)
{
    dra_mapping->signalled_dra.signal_dra_flag = dra_mapping->flag_enabled;
    int num_pivot_points = dra_mapping->num_ranges + 1;
    dra_mapping->signalled_dra.dra_descriptor2 = dra_mapping->dra_descriptor2;
    dra_mapping->signalled_dra.dra_descriptor1 = dra_mapping->dra_descriptor1;
    dra_mapping->signalled_dra.dra_table_idx = dra_mapping->chroma_qp_model.dra_table_idx;

    dra_mapping->signalled_dra.num_ranges = dra_mapping->num_ranges;
    for (int i = 0; i < num_pivot_points; i++)
    {
        dra_mapping->signalled_dra.in_ranges[i] = dra_mapping->in_ranges[i];
    }
    for (int i = 0; i < num_pivot_points; i++)
    {
        dra_mapping->signalled_dra.dra_scale_value[i] = dra_mapping->dra_scales_s32[i];
    }
    assert(DRA_SCALE_NUMFBITS >= dra_mapping->dra_descriptor2);
    dra_mapping->signalled_dra.dra_cb_scale_value = dra_mapping->dra_cb_scale_value >> (DRA_SCALE_NUMFBITS - dra_mapping->dra_descriptor2);
    dra_mapping->signalled_dra.dra_cr_scale_value = dra_mapping->dra_cr_scale_value >> (DRA_SCALE_NUMFBITS - dra_mapping->dra_descriptor2);

    xeve_check_equal_range_flag(dra_mapping);
}

void xeve_construct_fwd_dra(DRA_CONTROL *dra_mapping, int bit_depth)
{

    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        dra_mapping->dra_scales[i] = (double)dra_mapping->dra_scales_s32[i] / (1 << dra_mapping->dra_descriptor2);
    }

    int deltas[33];
    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        deltas[i] = dra_mapping->in_ranges[i + 1] - dra_mapping->in_ranges[i];
    }
    {
        QUANT_PARAM_DRA accum, temp1, temp2, temp3;
        QUANT_PARAM_DRA out_ranges[33];
        QUANT_PARAM_DRA dra_offsets[32];
        prec_quantize_entry_i(&(out_ranges[0]), 0, 1);
        for (int i = 1; i < dra_mapping->num_ranges + 1; i++)
        {
            prec_quantize_entry_d(&temp1, dra_mapping->dra_scales[i - 1], DRA_SCALE_NUMFBITS, 10);
            prec_quantize_entry_i(&temp2, deltas[i - 1], bit_depth + 1);
            temp3 = prec_mult_entry(temp1, temp2);
            out_ranges[i] = prec_plus_entry(out_ranges[i - 1], temp3);
        }

        for (int i = 0; i < dra_mapping->num_ranges; i++)
        {
            prec_quantize_entry_d(&temp1, 1, DRA_SCALE_NUMFBITS + DRA_INVSCALE_NUMFBITS, 11);
            prec_quantize_entry_d(&temp2, dra_mapping->dra_scales[i], DRA_SCALE_NUMFBITS, 10);
            accum = prec_divide_entry(temp1, temp2);
            temp3 = prec_mult_entry(out_ranges[i + 1], accum);
            prec_quantize_entry_d(&temp1, dra_mapping->in_ranges[i + 1], 0, bit_depth);
            dra_offsets[i] = prec_minus_entry(temp1, temp3);
            set_frac_bits(&(dra_offsets[i]), DRA_OFFSET_NUMFBITS);
        }

        for (int i = 0; i < dra_mapping->num_ranges + 1; i++)
        {
            set_frac_bits(&(out_ranges[i]), 0);
            dra_mapping->out_ranges[i] = get_val(&(out_ranges[i]));
        }
        for (int i = 0; i < dra_mapping->num_ranges; i++)
        {
            dra_mapping->dra_offets[i] = get_val(&(dra_offsets[i]));
        }
    }
    return;
}

static void xeve_build_fwd_dra_lut_from_dec(DRA_CONTROL *dra_mapping, int bit_depth)
{
    xeve_construct_fwd_dra(dra_mapping, bit_depth);

    QUANT_PARAM_DRA accum, temp1, temp2, temp3;

    int max_in_luma_codeword = DRA_LUT_MAXSIZE - 1;
    int max_out_luma_codeword = DRA_LUT_MAXSIZE - 1;

    for (int i = 0; i < DRA_LUT_MAXSIZE; i++)
    {
        dra_mapping->luma_scale_lut[i] = 0;
    }


    QUANT_PARAM_DRA luma_scale_lut[DRA_LUT_MAXSIZE];

    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        int x = dra_mapping->in_ranges[i];
        int y = dra_mapping->in_ranges[i + 1];
        for (int j = x; j < y; j++)
        {
            prec_quantize_entry_i(&temp1, j, bit_depth);
            prec_quantize_entry_d(&temp2, dra_mapping->dra_offets[i], DRA_OFFSET_NUMFBITS, 15);
            prec_quantize_entry_d(&temp3, dra_mapping->dra_scales[i], DRA_SCALE_NUMFBITS, 10);
            accum = prec_minus_entry(temp1, temp2);
            luma_scale_lut[j] = prec_mult_entry(accum, temp3);
            set_frac_bits(&(luma_scale_lut[j]), 0);
            dra_mapping->luma_scale_lut[j] = (int)(get_val(&(luma_scale_lut[j])));
            if (dra_mapping->luma_scale_lut[j] > max_out_luma_codeword)
            {
                dra_mapping->luma_scale_lut[j] = max_out_luma_codeword;
            }
        }
    }

    for (int j = dra_mapping->in_ranges[dra_mapping->num_ranges]; j < DRA_LUT_MAXSIZE; j++)
    {
        prec_quantize_entry_i(&temp1, j, bit_depth);
        prec_quantize_entry_d(&temp2, dra_mapping->dra_offets[dra_mapping->num_ranges - 1], DRA_OFFSET_NUMFBITS, 15);
        prec_quantize_entry_d(&temp3, dra_mapping->dra_scales[dra_mapping->num_ranges - 1], DRA_SCALE_NUMFBITS, 10);
        accum = prec_minus_entry(temp1, temp2);
        luma_scale_lut[j] = prec_mult_entry(accum, temp3);
        set_frac_bits(&(luma_scale_lut[j]), 0);
        dra_mapping->luma_scale_lut[j] = (int)get_val(&(luma_scale_lut[j]));

        if (dra_mapping->luma_scale_lut[j] > max_out_luma_codeword)
        {
            dra_mapping->luma_scale_lut[j] = max_out_luma_codeword;
        }
    }

    for (int ch = 0; ch < 2; ch++)
    {
        for (int i = 0; i < DRA_LUT_MAXSIZE; i++)
        {
            int value1 = 1 << (DRA_SCALE_NUMFBITS + DRA_INVSCALE_NUMFBITS);
            int value3 = dra_mapping->xevem_int_chroma_inv_scale_lut[ch][dra_mapping->luma_scale_lut[i]];
            int temp = (int)(value1 + (value3 / 2)) / value3;
            dra_mapping->int_chroma_scale_lut[ch][i] = temp;
        }
    }
}
void xeve_init_dra(DRA_CONTROL *dra_mapping, int total_change_points, int *luma_change_points, int* qps,int bit_depth)
{
    dra_mapping->flag_enabled = TRUE;

    // Chroma handling for  WCG representations
    double scale_cb_dra = xeve_get_cb_scale_dra(&(dra_mapping->chroma_qp_model), dra_mapping->chroma_qp_model.dra_table_idx);
    double scale_cr_dra = xeve_get_cr_scale_dra(&(dra_mapping->chroma_qp_model), dra_mapping->chroma_qp_model.dra_table_idx);

    double min_bin = 1.0 / (1 << dra_mapping->dra_descriptor2);
    int sign = (scale_cb_dra < 0) ? -1 : 1;
    if (sign * scale_cb_dra < min_bin)
        scale_cb_dra = sign * min_bin;
    if (sign * scale_cb_dra > (4 - min_bin))
        scale_cb_dra = sign * (4 - min_bin);

    sign = (scale_cr_dra < 0) ? -1 : 1;
    if (sign * scale_cr_dra < min_bin)
        scale_cr_dra = sign * min_bin;
    if (sign * scale_cr_dra > (4 - min_bin))
        scale_cr_dra = sign * (4 - min_bin);

    scale_cb_dra = XEVE_CLIP3(0, 1 << dra_mapping->dra_descriptor1, scale_cb_dra);
    dra_mapping->dra_cb_scale_value = (int)(scale_cb_dra * (1 << dra_mapping->dra_descriptor2) + 0.5);
    scale_cr_dra = XEVE_CLIP3(0, 1 << dra_mapping->dra_descriptor1, scale_cr_dra);
    dra_mapping->dra_cr_scale_value = (int)(scale_cr_dra * (1 << dra_mapping->dra_descriptor2) + 0.5);

    int configID = 0; // 0: HDR, 1: SDR

    dra_mapping->global_offset = 0;
    total_change_points = dra_mapping->num_ranges + 1;
    int total_num_ranges = dra_mapping->num_ranges;
    double scales[32];
    int in_ranges[33];
    double out_ranges[33];
    int deltas[33];

    for (int i = 0; i < DRA_LUT_MAXSIZE; i++)
    {
        dra_mapping->xevem_luma_inv_scale_lut[i] = i;
        dra_mapping->luma_scale_lut[i] = i;
    };

    for (int i = 0; i < total_change_points; i++)
    {
        scales[i] = (dra_mapping->dra_scale_map.dra_scale_map_y[i][1]);
        in_ranges[i] = (int)(dra_mapping->dra_scale_map.dra_scale_map_y[i][0]);
    }


    for (int i = 0; i < total_num_ranges; i++)
    {
        deltas[i] = in_ranges[i + 1] - in_ranges[i];
    }

    out_ranges[0] = 0;
    for (int i = 1; i < total_change_points; i++)
    {
        out_ranges[i] = (int)(out_ranges[i - 1] + scales[i - 1] * deltas[i - 1] + 0.5);
    }

    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        dra_mapping->dra_scales[i] = scales[i];
        dra_mapping->in_ranges[i] = in_ranges[i];
        dra_mapping->out_ranges[i] = out_ranges[i];
    }
    dra_mapping->in_ranges[dra_mapping->num_ranges] = in_ranges[dra_mapping->num_ranges];
    dra_mapping->out_ranges[dra_mapping->num_ranges] = out_ranges[dra_mapping->num_ranges];

    xeve_construct_dra(dra_mapping, configID, TRUE, bit_depth);

    return;
}

static void xeve_get_signalled_params_dra(DRA_CONTROL *dra_mapping)
{
    dra_mapping->flag_enabled = dra_mapping->signalled_dra.signal_dra_flag;
    dra_mapping->chroma_qp_model.dra_table_idx = dra_mapping->signalled_dra.dra_table_idx;
    dra_mapping->num_ranges = dra_mapping->signalled_dra.num_ranges;
    dra_mapping->dra_descriptor2 = dra_mapping->signalled_dra.dra_descriptor2;
    dra_mapping->dra_descriptor1 = dra_mapping->signalled_dra.dra_descriptor1;

    dra_mapping->dra_cb_scale_value = dra_mapping->signalled_dra.dra_cb_scale_value;
    dra_mapping->dra_cr_scale_value = dra_mapping->signalled_dra.dra_cr_scale_value;
    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        dra_mapping->dra_scales_s32[i] = dra_mapping->signalled_dra.dra_scale_value[i];
    }
    for (int i = 0; i <= dra_mapping->num_ranges; i++)
    {
        dra_mapping->in_ranges[i] = dra_mapping->signalled_dra.in_ranges[i];
    }
}

static void xeve_construct_dra_ready(DRA_CONTROL *dra_mapping)
{
    int numFracBits = dra_mapping->dra_descriptor2;
    int NUM_MULT_BITS = DRA_SCALE_NUMFBITS + DRA_INVSCALE_NUMFBITS;
    int deltas[33];

    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        deltas[i] = dra_mapping->in_ranges[i + 1] - dra_mapping->in_ranges[i];
    }

    dra_mapping->out_ranges_s32[0] = 0;
    for (int i = 1; i < dra_mapping->num_ranges + 1; i++)
    {
        dra_mapping->out_ranges_s32[i] = dra_mapping->out_ranges_s32[i - 1] + deltas[i - 1] * dra_mapping->dra_scales_s32[i - 1];
    }

    for (int i = 0; i < dra_mapping->num_ranges; i++)
    {
        int invScale2;
        int nomin = 1 << NUM_MULT_BITS;
        invScale2 = (int) ( (nomin  + ( dra_mapping->dra_scales_s32[i] >> 1 ) ) / dra_mapping->dra_scales_s32[i] );

        int diffVal2 = dra_mapping->out_ranges_s32[i + 1] * invScale2;
        dra_mapping->inv_dra_offsets_s32[i] = ((dra_mapping->in_ranges[i + 1] << NUM_MULT_BITS) - diffVal2 + (1 << (dra_mapping->dra_descriptor2 - 1)) ) >> (dra_mapping->dra_descriptor2);
        dra_mapping->inv_dra_scales_s32[i] = invScale2;
    }

    for (int i = 0; i < dra_mapping->num_ranges + 1; i++)
    {
        dra_mapping->out_ranges_s32[i] = (dra_mapping->out_ranges_s32[i] + (1 << (numFracBits - 1))) >> numFracBits;
    }
    return;
}

static void xeve_dra_ready(XEVE_CTX * ctx, DRA_CONTROL *dra_mapping, int bit_depth)
{
    xeve_get_signalled_params_dra(dra_mapping);
    xeve_construct_dra_ready(dra_mapping);
    xeve_compensate_chroma_shift_table(ctx, dra_mapping, bit_depth);
    xeve_build_dra_luma_lut(dra_mapping);
    xeve_build_dra_chroma_lut(dra_mapping, bit_depth);
}

static void xeve_update_dra(XEVE_CTX * ctx, DRA_CONTROL *dra_mapping, int sdr_flag, int bit_depth)
{
    xeve_construct_dra(dra_mapping, sdr_flag, TRUE,  bit_depth);
    xeve_zoom_in_range_lut(dra_mapping, FALSE);
    xeve_normalize_histogram_lut(dra_mapping, 0,  bit_depth);
    xeve_quatnize_params_dra(dra_mapping);
    xeve_set_signalled_params_dra(dra_mapping);

    // Produce inverse DRA from signalled parameters
    xeve_dra_ready(ctx, dra_mapping,  bit_depth);

    // Produce forward DRA from signalled parameters
    xeve_build_fwd_dra_lut_from_dec(dra_mapping, bit_depth);

    return;
}
BOOL xeve_analyze_input_pic(XEVE_CTX * ctx, DRA_CONTROL *dra_mapping, int bit_depth)
{
    dra_mapping->global_offset = 64;
    dra_mapping->global_end = 940;
    xeve_update_dra(ctx, dra_mapping, 0,  bit_depth);
    return XEVE_OK;
}

int xeve_generate_dra_array(XEVE_CTX * ctx, SIG_PARAM_DRA * dra_control_array, DRA_CONTROL * tmp_dra_control, int num_aps, int bit_depth)
{
    for (int i = 0; i < num_aps; i++)
    {
        xeve_init_dra(tmp_dra_control, 0, NULL, NULL, bit_depth);
        xeve_analyze_input_pic(ctx, tmp_dra_control, bit_depth);
        if (tmp_dra_control->flag_enabled == 1)
        {
            xeve_mcpy(dra_control_array + i, &(tmp_dra_control->signalled_dra), sizeof(SIG_PARAM_DRA));
        }
    }
    return XEVE_OK;
}

static int xeve_construct_dra_from_array(XEVE_CTX * ctx, SIG_PARAM_DRA * dra_control_array, DRA_CONTROL * tmp_dra_control, int effective_aps_id, int bit_depth)
{
    assert(effective_aps_id >= 0 && effective_aps_id < APS_MAX_NUM);

    SIG_PARAM_DRA* pps_dra_params = dra_control_array + effective_aps_id;
    xeve_mcpy(&(tmp_dra_control->signalled_dra), pps_dra_params, sizeof(SIG_PARAM_DRA));
    xeve_dra_ready(ctx, tmp_dra_control, bit_depth);
    xeve_build_fwd_dra_lut_from_dec(tmp_dra_control, bit_depth);
    return XEVE_OK;
}

/* DRA applicaton (sample processing) functions are listed below: */
static void xeve_apply_dra_luma_plane(XEVE_IMGB * dst, XEVE_IMGB * src, DRA_CONTROL *dra_mapping, int plane_id, int backward_map)
{
    short* src_plane;
    short* dst_plane;
    short src_value, dst_value;
    int i, k, j;

    for (i = plane_id; i <= plane_id; i++)
    {
        src_plane = (short*)src->a[i];
        dst_plane = (short*)dst->a[i];
        for (j = 0; j < src->h[i]; j++)
        {
            for (k = 0; k < src->w[i]; k++)
            {
                src_value = src_plane[k];

                dst_value = dst_plane[k];
                if (backward_map == TRUE)
                    dst_value = dra_mapping->xevem_luma_inv_scale_lut[src_value];
                else
                    dst_value = dra_mapping->luma_scale_lut[src_value];
                dst_plane[k] = dst_value;
            }
            src_plane = (short*)((unsigned char *)src_plane + src->s[i]);
            dst_plane = (short*)((unsigned char *)dst_plane + dst->s[i]);
        }
    }
}
static void xeve_apply_dra_chroma_plane(XEVE_IMGB * dst, XEVE_IMGB * src, DRA_CONTROL *dra_mapping, int plane_id, int backward_map)
{
    int round_offset = 1 << (DRA_INVSCALE_NUMFBITS - 1);
    int offset_value = 0;
    int int_scale = 1;
    double scale = 0;

    short* ref_plane;
    short* src_plane;
    short* dst_plane;
    short ref_value, src_value, dst_value;
    int i, k, j;
    int c_shift = (plane_id == 0) ? 0 : 1;

    for (i = plane_id; i <= plane_id; i++)
    {
        ref_plane = (short*)src->a[0]; //luma reference
        src_plane = (short*)src->a[i];
        dst_plane = (short*)dst->a[i];

        for (j = 0; j < src->h[i]; j++)
        {
            for (k = 0; k < src->w[i]; k++)
            {
                ref_value = ref_plane[k << c_shift];
                ref_value = (ref_value < 0) ? 0 : ref_value;
                src_value = src_plane[k];
                dst_value = dst_plane[k];
                src_value = src_value - 512;
                offset_value = src_value;
                if (backward_map == TRUE)
                    int_scale = (dra_mapping->xevem_int_chroma_inv_scale_lut[i - 1][ref_value]);
                else
                    int_scale = (dra_mapping->int_chroma_scale_lut[i - 1][ref_value]);
                if (src_value < 0)
                {
                    offset_value *= -1;
                }
                offset_value = (offset_value * int_scale + round_offset) >> DRA_INVSCALE_NUMFBITS;
                if (src_value < 0)
                {
                    offset_value *= -1;
                }
                dst_value = 512 + offset_value;

                dst_plane[k] = dst_value;
            }
            ref_plane = (short*)((unsigned char *)ref_plane + (dst->s[0] << c_shift));
            src_plane = (short*)((unsigned char *)src_plane + src->s[i]);
            dst_plane = (short*)((unsigned char *)dst_plane + dst->s[i]);
        }
    }
}

/* DRA APS buffer functions are listed below: */
void xeve_reset_aps_gen_read_buffer(XEVE_APS_GEN *tmp_aps_gen_array)
{
    tmp_aps_gen_array[0].aps_type_id = 0; // ALF
    tmp_aps_gen_array[0].aps_id = -1;
    tmp_aps_gen_array[0].signal_flag = 0;

    tmp_aps_gen_array[1].aps_type_id = 1; // DRA
    tmp_aps_gen_array[1].aps_id = -1;
    tmp_aps_gen_array[1].signal_flag = 0;
}

void xeve_apply_dra_from_array(XEVE_CTX * ctx, XEVE_IMGB * dst, XEVE_IMGB * src, SIG_PARAM_DRA * dra_control_array, int dra_id, int backward_map)
{
    DRA_CONTROL dra_mapping;
    DRA_CONTROL *tmp_dra_mapping = &dra_mapping;
    int bit_depth = XEVE_CS_GET_BIT_DEPTH(src->cs);
    xeve_construct_dra_from_array(ctx, dra_control_array, tmp_dra_mapping, dra_id, bit_depth);

    if(XEVE_CFI_FROM_CF(XEVE_CS_GET_FORMAT(dst->cs)))
    {
        xeve_apply_dra_chroma_plane(dst, src, tmp_dra_mapping, 1, backward_map);
        xeve_apply_dra_chroma_plane(dst, src, tmp_dra_mapping, 2, backward_map);
    }
    xeve_apply_dra_luma_plane(dst, src, tmp_dra_mapping, 0, backward_map);
}

int xevem_set_active_dra_info(XEVE_CTX * ctx)
{
    XEVEM_CTX * mctx = (XEVEM_CTX*)ctx;

    int dra_aps_id = ctx->pps.pic_dra_aps_id;
    ctx->aps_gen_array[1].aps_id = dra_aps_id;
    ctx->aps_gen_array[1].aps_data = (void*)(&((SIG_PARAM_DRA *)mctx->dra_array)[dra_aps_id]);
    ctx->aps_gen_array[1].signal_flag = ((SIG_PARAM_DRA *)mctx->dra_array)[dra_aps_id].signal_dra_flag;  // if dra entry was already sent, signal_dra_flag is equal to 0
    xeve_assert(ctx->aps_gen_array[1].signal_flag > -1 && ctx->aps_gen_array[1].signal_flag < 2);

    return XEVE_OK;
}


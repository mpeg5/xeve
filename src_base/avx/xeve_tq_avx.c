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

#include "xeve_def.h"
#include "xeve_tq_avx.h"

#ifndef _mm256_set_m128i
#define _mm256_set_m128i(/* __m128i */ hi, /* __m128i */ lo) \
    _mm256_insertf128_si256(_mm256_castsi128_si256(lo), (hi), 0x1)
#endif // !_mm256_set_m128i

#ifndef _mm256_loadu2_m128i
#define _mm256_loadu2_m128i(/* __m128i const* */ hiaddr, \
                            /* __m128i const* */ loaddr) \
    _mm256_set_m128i(_mm_loadu_si128(hiaddr), _mm_loadu_si128(loaddr))
#endif 


// clang-format off
ALIGNED_32(static const s16 tab_dct2_2nd_shuffle_256i[][16]) = {
    // 16bit: 0-7, 3-0 7-4
    { 0x0100, 0x0302, 0x0504, 0x0706, 0x0908, 0x0B0A, 0x0D0C, 0x0F0E, 0x0706, 0x0504, 0x0302, 0x0100, 0x0F0E, 0x0D0C, 0x0B0A, 0x0908 },  // 0
    // 32bit: 3-0, 3-0
    { 0x0D0C, 0x0F0E, 0x0908, 0x0B0A, 0x0504, 0x0706, 0x0100, 0x0302, 0x0D0C, 0x0F0E, 0x0908, 0x0B0A, 0x0504, 0x0706, 0x0100, 0x0302 },  // 1
    // 32bit: 0, 3, 1, 2, 0, 3, 1, 2
    { 0x0100, 0x0302, 0x0D0C, 0x0F0E, 0x0504, 0x0706, 0x0908, 0x0B0A, 0x0100, 0x0302, 0x0D0C, 0x0F0E, 0x0504, 0x0706, 0x0908, 0x0B0A },  // 2
    // 16bit: 7-0, 7-0
    { 0x0F0E, 0x0D0C, 0x0B0A, 0x0908, 0x0706, 0x0504, 0x0302, 0x0100, 0x0F0E, 0x0D0C, 0x0B0A, 0x0908, 0x0706, 0x0504, 0x0302, 0x0100 }
};


ALIGNED_32(static const s16 tab_dct2_1st_shuffle_256i[][16]) = {
    // 16bit: 7-0, 7-0
    { 0x0F0E, 0x0D0C, 0x0B0A, 0x0908, 0x0706, 0x0504, 0x0302, 0x0100, 0x0F0E, 0x0D0C, 0x0B0A, 0x0908, 0x0706, 0x0504, 0x0302, 0x0100 },
    // 16bit: 0, 7, 1, 6, 2, 5, 3, 4, 0, 7, 1, 6, 2, 5, 3, 4
    { 0x0100, 0x0F0E, 0x0302, 0x0D0C, 0x0504, 0x0B0A, 0x0706, 0x0908, 0x0100, 0x0F0E, 0x0302, 0x0D0C, 0x0504, 0x0B0A, 0x0706, 0x0908 },
    // 16bit: 0, 3, 1, 2, 4, 7, 5, 6, 0, 3, 1, 2, 4, 7, 5, 6
    { 0x0100, 0x0706, 0x0302, 0x0504, 0x0908, 0x0F0E, 0x0B0A, 0x0D0C, 0x0100, 0x0706, 0x0302, 0x0504, 0x0908, 0x0F0E, 0x0B0A, 0x0D0C }
};
// clang-format on

static void tx_pb8b_avx(void* src_, void* dst_, int shift, int line, int step)
{
    if (line % 8 != 0 || step == 1)
    {
        tx_pb8b(src_, dst_, shift, line, step);
    }
    else
    {
        s16* src = (s16*)src_;
        s32* dst = (s32*)dst_;
        __m256i v0, v1, v2, v3, v4, v5, v6, v7;
        __m256i d0, d1, d2, d3;

        // clang-format off
        const __m256i coeff0 = _mm256_set1_epi16(64);    
        const __m256i coeff1 = _mm256_set_epi16(64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64);
        const __m256i coeff2 = _mm256_set_epi16(84, 35, -35, -84, -84, -35, 35, 84, 84, 35, -35, -84, -84, -35, 35, 84);    
        const __m256i coeff3 = _mm256_set_epi16(35, -84, 84, -35, -35, 84, -84, 35, 35, -84, 84, -35, -35, 84, -84, 35);
        const __m256i coeff4 = _mm256_set_epi16(-89, -75, -50, -18, 18, 50, 75, 89, -89, -75, -50, -18, 18, 50, 75, 89);    
        const __m256i coeff5 = _mm256_set_epi16(-75, 18, 89, 50, -50, -89, -18, 75, -75, 18, 89, 50, -50, -89, -18, 75);
        const __m256i coeff6 = _mm256_set_epi16(-50, 89, -18, -75, 75, 18, -89, 50, -50, 89, -18, -75, 75, 18, -89, 50);
        const __m256i coeff7 = _mm256_set_epi16(-18, 50, -75, 89, -89, 75, -50, 18, -18, 50, -75, 89, -89, 75, -50, 18);
        // clang-format on
        
        __m256i add = _mm256_set1_epi32(shift == 0 ? 0 : 1 << (shift - 1));

        if (line > 4)
        {

            int j;
            __m256i s0, s1, s2, s3;

            for (j = 0; j < line; j += 8)
            {
                // clang-format off
                s0 = _mm256_loadu2_m128i((const __m128i*) & src[4 * 8], (const __m128i*) & src[0]);    
                s1 = _mm256_loadu2_m128i((const __m128i*) & src[5 * 8], (const __m128i*) & src[8]);    
                s2 = _mm256_loadu2_m128i((const __m128i*) & src[6 * 8], (const __m128i*) & src[16]);
                s3 = _mm256_loadu2_m128i((const __m128i*) & src[7 * 8], (const __m128i*) & src[24]);
                
                src += 8 * 8;

                CALCU_2x8(coeff0, coeff4, d0, d1);
                CALCU_2x8(coeff2, coeff5, d2, d3);
                CALCU_2x8_ADD_SHIFT(d0, d1, d2, d3, add, shift);

                _mm256_storeu_si256((__m256i*)dst, (d0));
                _mm256_storeu_si256((__m256i*)(dst + 1 * line), (d1));
                _mm256_storeu_si256((__m256i*)(dst + 2 * line), (d2));
                _mm256_storeu_si256((__m256i*)(dst + 3 * line), (d3));

                CALCU_2x8(coeff1, coeff6, d0, d1);
                CALCU_2x8(coeff3, coeff7, d2, d3);
                CALCU_2x8_ADD_SHIFT(d0, d1, d2, d3, add, shift);

                _mm256_storeu_si256((__m256i*)(dst + 4 * line), (d0));
                _mm256_storeu_si256((__m256i*)(dst + 5 * line), (d1));
                _mm256_storeu_si256((__m256i*)(dst + 6 * line), (d2));
                _mm256_storeu_si256((__m256i*)(dst + 7 * line), (d3));

                dst += 8;
                // clang-format on
            }
        }
    }
    
}

static void tx_pb16b_avx(void* src, void* dst, int shift, int line, int step)
{
    if (line % 8 != 0 || step == 1)
    {
        tx_pb16b(src, dst, shift, line, step);
    }
    else
    {
        if (line > 4)
        {
            s16* pel_src = (s16*) src;
            s32* pel_dst = (s32*) dst;
            int j;
            __m256i s00, s01, s02, s03, s04, s05, s06, s07;
            __m256i v0, v1, v2, v3, v4, v5, v6, v7;
            __m256i d0, d1, d2, d3, d4, d5, d6, d7;
            __m256i add = _mm256_set1_epi32(shift == 0 ? 0 : 1 << (shift - 1));
            __m256i coeffs[8];
            __m256i coeffs_nxt[8];

            coeffs[0] = _mm256_set1_epi16(64);
            coeffs[1] = _mm256_set_epi16(-90, -87, -80, -70, -57, -43, -26, -9, 9, 26, 43, 57, 70, 80, 87, 90);
            coeffs[2] = _mm256_set_epi16(89, 75, 50, 18, -18, -50, -75, -89, -89, -75, -50, -18, 18, 50, 75, 89);
            coeffs[3] = _mm256_set_epi16(-87, -57, -9, 43, 80, 90, 70, 26, -26, -70, -90, -80, -43, 9, 57, 87);
            coeffs[4] = _mm256_set_epi16(84, 35, -35, -84, -84, -35, 35, 84, 84, 35, -35, -84, -84, -35, 35, 84);
            coeffs[5] = _mm256_set_epi16(-80, -9, 70, 87, 26, -57, -90, -43, 43, 90, 57, -26, -87, -70, 9, 80);
            coeffs[6] = _mm256_set_epi16(75, -18, -89, -50, 50, 89, 18, -75, -75, 18, 89, 50, -50, -89, -18, 75);
            coeffs[7] = _mm256_set_epi16(-70, 43, 87, -9, -90, -26, 80, 57, -57, -80, 26, 90, 9, -87, -43, 70);

            coeffs_nxt[0] = _mm256_set_epi16(64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64);
            coeffs_nxt[1] = _mm256_set_epi16(-57, 80, 26, -90, 9, 87, -43, -70, 70, 43, -87, -9, 90, -26, -80, 57);
            coeffs_nxt[2] = _mm256_set_epi16(50, -89, 18, 75, -75, -18, 89, -50, -50, 89, -18, -75, 75, 18, -89, 50);
            coeffs_nxt[3] = _mm256_set_epi16(-43, 90, -57, -26, 87, -70, -9, 80, -80, 9, 70, -87, 26, 57, -90, 43);
            coeffs_nxt[4] = _mm256_set_epi16(35, -84, 84, -35, -35, 84, -84, 35, 35, -84, 84, -35, -35, 84, -84, 35);
            coeffs_nxt[5] = _mm256_set_epi16(-26, 70, -90, 80, -43, -9, 57, -87, 87, -57, 9, 43, -80, 90, -70, 26);
            coeffs_nxt[6] = _mm256_set_epi16(18, -50, 75, -89, 89, -75, 50, -18, -18, 50, -75, 89, -89, 75, -50, 18);
            coeffs_nxt[7] = _mm256_set_epi16(-9, 26, -43, 57, -70, 80, -87, 90, -90, 87, -80, 70, -57, 43, -26, 9);

            for (j = 0; j < line; j += 8)
            {
                s00 = _mm256_loadu_si256((__m256i*)(pel_src));            
                s01 = _mm256_loadu_si256((__m256i*)(pel_src + 16));
                s02 = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 2));
                s03 = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 3));
                s04 = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 4));
                s05 = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 5));
                s06 = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 6));
                s07 = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 7));

                pel_src += 16 * 8;

                CALCU_LINE_1x8(coeffs[0], d0);
                CALCU_LINE_1x8(coeffs[1], d1);
                CALCU_LINE_1x8(coeffs[2], d2);
                CALCU_LINE_1x8(coeffs[3], d3);
                CALCU_LINE_1x8(coeffs[4], d4);
                CALCU_LINE_1x8(coeffs[5], d5);
                CALCU_LINE_1x8(coeffs[6], d6);
                CALCU_LINE_1x8(coeffs[7], d7);

                CALCU_LINE_1x8_ADD_SHIFT(d0, d1, d2, d3, d4, d5, d6, d7, add, shift);

                _mm256_storeu_si256((__m256i*)(pel_dst), (d0));
                _mm256_storeu_si256((__m256i*)(pel_dst + 1 * line), (d1));
                _mm256_storeu_si256((__m256i*)(pel_dst + 2 * line), (d2));
                _mm256_storeu_si256((__m256i*)(pel_dst + 3 * line), (d3));
                _mm256_storeu_si256((__m256i*)(pel_dst + 4 * line), (d4));
                _mm256_storeu_si256((__m256i*)(pel_dst + 5 * line), (d5));
                _mm256_storeu_si256((__m256i*)(pel_dst + 6 * line), (d6));
                _mm256_storeu_si256((__m256i*)(pel_dst + 7 * line), (d7));

                CALCU_LINE_1x8(coeffs_nxt[0], d0);
                CALCU_LINE_1x8(coeffs_nxt[1], d1);
                CALCU_LINE_1x8(coeffs_nxt[2], d2);
                CALCU_LINE_1x8(coeffs_nxt[3], d3);
                CALCU_LINE_1x8(coeffs_nxt[4], d4);
                CALCU_LINE_1x8(coeffs_nxt[5], d5);
                CALCU_LINE_1x8(coeffs_nxt[6], d6);
                CALCU_LINE_1x8(coeffs_nxt[7], d7);

                CALCU_LINE_1x8_ADD_SHIFT(d0, d1, d2, d3, d4, d5, d6, d7, add, shift);

                _mm256_storeu_si256((__m256i*)(pel_dst + 8 * line), (d0));
                _mm256_storeu_si256((__m256i*)(pel_dst + 9 * line), (d1));
                _mm256_storeu_si256((__m256i*)(pel_dst + 10 * line), (d2));
                _mm256_storeu_si256((__m256i*)(pel_dst + 11 * line), (d3));
                _mm256_storeu_si256((__m256i*)(pel_dst + 12 * line), (d4));
                _mm256_storeu_si256((__m256i*)(pel_dst + 13 * line), (d5));
                _mm256_storeu_si256((__m256i*)(pel_dst + 14 * line), (d6));
                _mm256_storeu_si256((__m256i*)(pel_dst + 15 * line), (d7));

                pel_dst += 8;
            }
        }
    }
    
}

static void tx_pb32b_avx(void* src, void* dst, int shift, int line, int step)
{
    if (line % 8 != 0 || step == 1)
    {
        tx_pb32b(src, dst, shift, line, step);
    }
    else
    {
        if (line > 4)
        {
            s16* pel_src = (s16*) src;
            s32* pel_dst = (s32*) dst;
            int i, j;
            __m256i s[32];
            __m256i t[16];
            __m256i tab0, tab1, tab2;
            __m256i e[16], o[16], ee[8], eo[8];
            __m256i eee[4], eeo[4];
            __m256i eeee[2], eeeo[2];
            __m256i v[18];
            __m256i dst_reg[8];
            __m256i add = _mm256_set1_epi32(shift == 0 ? 0 : 1 << (shift - 1));
            __m256i coeffs[52];


            const __m256i coeff_p64_p64 = _mm256_set_epi32(64, 64, 64, 64, 64, 64, 64, 64);    
            const __m256i coeff_p64_n64 = _mm256_set_epi32(-64, 64, -64, 64, -64, 64, -64, 64);
            const __m256i coeff_p84_p35 = _mm256_set_epi32(35, 84, 35, 84, 35, 84, 35, 84);    
            const __m256i coeff_p35_n84 = _mm256_set_epi32(-84, 35, -84, 35, -84, 35, -84, 35);

            tab0 = _mm256_loadu_si256((__m256i*)tab_dct2_2nd_shuffle_256i[3]);  
            tab1 = _mm256_loadu_si256((__m256i*)tab_dct2_2nd_shuffle_256i[1]);  
            tab2 = _mm256_loadu_si256((__m256i*)tab_dct2_2nd_shuffle_256i[2]);  

            for (j = 0; j < line; j += 8)
            {
                s[0] = _mm256_loadu_si256((__m256i*)(pel_src));                    
                s[1] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 1));          
                s[2] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 2));
                s[3] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 3));
                s[4] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 4));
                s[5] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 5));
                s[6] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 6));
                s[7] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 7));
                s[8] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 8));          
                s[9] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 9));          
                s[10] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 10));
                s[11] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 11));
                s[12] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 12));
                s[13] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 13));
                s[14] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 14));
                s[15] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 15));

                t[0] = _mm256_shuffle_epi8(s[1], tab0);                        
                t[1] = _mm256_shuffle_epi8(s[3], tab0);                        
                t[2] = _mm256_shuffle_epi8(s[5], tab0);
                t[3] = _mm256_shuffle_epi8(s[7], tab0);
                t[4] = _mm256_shuffle_epi8(s[9], tab0);
                t[5] = _mm256_shuffle_epi8(s[11], tab0);
                t[6] = _mm256_shuffle_epi8(s[13], tab0);
                t[7] = _mm256_shuffle_epi8(s[15], tab0);

                pel_src += 32 * 8;

                s[1] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[0], 1));   
                s[0] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[0]));        
                s[3] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[2], 1));   
                s[2] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[2]));        
                s[5] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[4], 1));
                s[4] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[4]));
                s[7] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[6], 1));
                s[6] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[6]));
                s[9] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[8], 1));
                s[8] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[8]));
                s[11] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[10], 1));
                s[10] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[10]));
                s[13] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[12], 1));
                s[12] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[12]));
                s[15] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[14], 1));
                s[14] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[14]));

                s[16] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[0], 1));   
                s[17] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[0]));        
                s[18] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[1], 1));   
                s[19] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[1]));        
                s[20] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[2], 1));
                s[21] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[2]));
                s[22] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[3], 1));
                s[23] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[3]));
                s[24] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[4], 1));
                s[25] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[4]));
                s[26] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[5], 1));
                s[27] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[5]));
                s[28] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[6], 1));
                s[29] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[6]));
                s[30] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[7], 1));
                s[31] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[7]));

                e[0] = _mm256_add_epi32(s[0], s[16]);                         
                o[0] = _mm256_sub_epi32(s[0], s[16]);                         
                e[1] = _mm256_add_epi32(s[1], s[17]);                         
                o[1] = _mm256_sub_epi32(s[1], s[17]);                         
                e[2] = _mm256_add_epi32(s[2], s[18]);                         
                o[2] = _mm256_sub_epi32(s[2], s[18]);                         
                e[3] = _mm256_add_epi32(s[3], s[19]);                         
                o[3] = _mm256_sub_epi32(s[3], s[19]);                         
                e[4] = _mm256_add_epi32(s[4], s[20]);
                o[4] = _mm256_sub_epi32(s[4], s[20]);
                e[5] = _mm256_add_epi32(s[5], s[21]);
                o[5] = _mm256_sub_epi32(s[5], s[21]);
                e[6] = _mm256_add_epi32(s[6], s[22]);
                o[6] = _mm256_sub_epi32(s[6], s[22]);
                e[7] = _mm256_add_epi32(s[7], s[23]);
                o[7] = _mm256_sub_epi32(s[7], s[23]);
                e[8] = _mm256_add_epi32(s[8], s[24]);                         
                o[8] = _mm256_sub_epi32(s[8], s[24]);
                e[9] = _mm256_add_epi32(s[9], s[25]);                         
                o[9] = _mm256_sub_epi32(s[9], s[25]);
                e[10] = _mm256_add_epi32(s[10], s[26]);
                o[10] = _mm256_sub_epi32(s[10], s[26]);
                e[11] = _mm256_add_epi32(s[11], s[27]);
                o[11] = _mm256_sub_epi32(s[11], s[27]);
                e[12] = _mm256_add_epi32(s[12], s[28]);
                o[12] = _mm256_sub_epi32(s[12], s[28]);
                e[13] = _mm256_add_epi32(s[13], s[29]);
                o[13] = _mm256_sub_epi32(s[13], s[29]);
                e[14] = _mm256_add_epi32(s[14], s[30]);
                o[14] = _mm256_sub_epi32(s[14], s[30]);
                e[15] = _mm256_add_epi32(s[15], s[31]);
                o[15] = _mm256_sub_epi32(s[15], s[31]);

                t[0] = _mm256_permute2x128_si256(e[0], e[8], 0x20);          
                t[1] = _mm256_permute2x128_si256(e[0], e[8], 0x31);          
                t[2] = _mm256_permute2x128_si256(e[1], e[9], 0x20);          
                t[3] = _mm256_permute2x128_si256(e[1], e[9], 0x31);          
                t[4] = _mm256_permute2x128_si256(e[2], e[10], 0x20);
                t[5] = _mm256_permute2x128_si256(e[2], e[10], 0x31);
                t[6] = _mm256_permute2x128_si256(e[3], e[11], 0x20);
                t[7] = _mm256_permute2x128_si256(e[3], e[11], 0x31);
                t[8] = _mm256_permute2x128_si256(e[4], e[12], 0x20);
                t[9] = _mm256_permute2x128_si256(e[4], e[12], 0x31);
                t[10] = _mm256_permute2x128_si256(e[5], e[13], 0x20);
                t[11] = _mm256_permute2x128_si256(e[5], e[13], 0x31);
                t[12] = _mm256_permute2x128_si256(e[6], e[14], 0x20);
                t[13] = _mm256_permute2x128_si256(e[6], e[14], 0x31);
                t[14] = _mm256_permute2x128_si256(e[7], e[15], 0x20);
                t[15] = _mm256_permute2x128_si256(e[7], e[15], 0x31);

                t[2] = _mm256_shuffle_epi8(t[2], tab1);                       
                t[3] = _mm256_shuffle_epi8(t[3], tab1);                       
                t[6] = _mm256_shuffle_epi8(t[6], tab1);
                t[7] = _mm256_shuffle_epi8(t[7], tab1);
                t[10] = _mm256_shuffle_epi8(t[10], tab1);                       
                t[11] = _mm256_shuffle_epi8(t[11], tab1);
                t[14] = _mm256_shuffle_epi8(t[14], tab1);
                t[15] = _mm256_shuffle_epi8(t[15], tab1);

                ee[0] = _mm256_add_epi32(t[0], t[3]);                         
                eo[0] = _mm256_sub_epi32(t[0], t[3]);                         
                ee[1] = _mm256_add_epi32(t[1], t[2]);                         
                eo[1] = _mm256_sub_epi32(t[1], t[2]);                         
                ee[2] = _mm256_add_epi32(t[4], t[7]);
                eo[2] = _mm256_sub_epi32(t[4], t[7]);
                ee[3] = _mm256_add_epi32(t[5], t[6]);
                eo[3] = _mm256_sub_epi32(t[5], t[6]);
                ee[4] = _mm256_add_epi32(t[8], t[11]);
                eo[4] = _mm256_sub_epi32(t[8], t[11]);
                ee[5] = _mm256_add_epi32(t[9], t[10]);
                eo[5] = _mm256_sub_epi32(t[9], t[10]);
                ee[6] = _mm256_add_epi32(t[12], t[15]);
                eo[6] = _mm256_sub_epi32(t[12], t[15]);
                ee[7] = _mm256_add_epi32(t[13], t[14]);
                eo[7] = _mm256_sub_epi32(t[13], t[14]);

                ee[1] = _mm256_shuffle_epi8(ee[1], tab1);                       
                ee[3] = _mm256_shuffle_epi8(ee[3], tab1);
                ee[5] = _mm256_shuffle_epi8(ee[5], tab1);
                ee[7] = _mm256_shuffle_epi8(ee[7], tab1);

                eee[0] = _mm256_add_epi32(ee[0], ee[1]);                        
                eeo[0] = _mm256_sub_epi32(ee[0], ee[1]);                        
                eee[1] = _mm256_add_epi32(ee[2], ee[3]);
                eeo[1] = _mm256_sub_epi32(ee[2], ee[3]);
                eee[2] = _mm256_add_epi32(ee[4], ee[5]);
                eeo[2] = _mm256_sub_epi32(ee[4], ee[5]);
                eee[3] = _mm256_add_epi32(ee[6], ee[7]);
                eeo[3] = _mm256_sub_epi32(ee[6], ee[7]);

                eee[0] = _mm256_shuffle_epi8(eee[0], tab2);                     
                eee[1] = _mm256_shuffle_epi8(eee[1], tab2);
                eee[2] = _mm256_shuffle_epi8(eee[2], tab2);
                eee[3] = _mm256_shuffle_epi8(eee[3], tab2);

                eeee[0] = _mm256_hadd_epi32(eee[0], eee[1]);                    
                eeeo[0] = _mm256_hsub_epi32(eee[0], eee[1]);
                eeee[1] = _mm256_hadd_epi32(eee[2], eee[3]);                    
                eeeo[1] = _mm256_hsub_epi32(eee[2], eee[3]);

                for (i = 0; i < 4; ++i)
                {
                    int idx = 2 * i + 1;
                    coeffs[i] = _mm256_setr_epi32(xeve_tbl_tm8[idx][0], xeve_tbl_tm8[idx][1], xeve_tbl_tm8[idx][2], xeve_tbl_tm8[idx][3], xeve_tbl_tm8[idx][0], xeve_tbl_tm8[idx][1], xeve_tbl_tm8[idx][2], xeve_tbl_tm8[idx][3]);
                }

                v[0] = _mm256_mullo_epi32(eeee[0], coeff_p64_p64);
                v[1] = _mm256_mullo_epi32(eeee[1], coeff_p64_p64);
                v[2] = _mm256_mullo_epi32(eeee[0], coeff_p64_n64);
                v[3] = _mm256_mullo_epi32(eeee[1], coeff_p64_n64);
                v[4] = _mm256_mullo_epi32(eeeo[0], coeff_p84_p35);
                v[5] = _mm256_mullo_epi32(eeeo[1], coeff_p84_p35);
                v[6] = _mm256_mullo_epi32(eeeo[0], coeff_p35_n84);
                v[7] = _mm256_mullo_epi32(eeeo[1], coeff_p35_n84);

                v[0] = _mm256_hadd_epi32(v[0], v[1]);                           
                v[2] = _mm256_hadd_epi32(v[2], v[3]);                           
                v[4] = _mm256_hadd_epi32(v[4], v[5]);                           
                v[6] = _mm256_hadd_epi32(v[6], v[7]);                           

                CALCU_2x8_ADD_SHIFT(v[0], v[2], v[4], v[6], add, shift);


                _mm256_storeu_si256((__m256i*)pel_dst, v[0]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 8 * line), v[4]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 16 * line), v[2]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 24 * line), v[6]);

                v[0] = _mm256_mullo_epi32(eeo[0], coeffs[0]);
                v[1] = _mm256_mullo_epi32(eeo[1], coeffs[0]);
                v[2] = _mm256_mullo_epi32(eeo[2], coeffs[0]);
                v[3] = _mm256_mullo_epi32(eeo[3], coeffs[0]);
                v[4] = _mm256_mullo_epi32(eeo[0], coeffs[1]);
                v[5] = _mm256_mullo_epi32(eeo[1], coeffs[1]);
                v[6] = _mm256_mullo_epi32(eeo[2], coeffs[1]);
                v[7] = _mm256_mullo_epi32(eeo[3], coeffs[1]);

                v[0] = _mm256_hadd_epi32(v[0], v[1]);
                v[2] = _mm256_hadd_epi32(v[2], v[3]);
                v[4] = _mm256_hadd_epi32(v[4], v[5]);
                v[6] = _mm256_hadd_epi32(v[6], v[7]);
                v[8] = _mm256_hadd_epi32(v[0], v[2]);               
                v[9] = _mm256_hadd_epi32(v[4], v[6]);               

                v[0] = _mm256_mullo_epi32(eeo[0], coeffs[2]);
                v[1] = _mm256_mullo_epi32(eeo[1], coeffs[2]);
                v[2] = _mm256_mullo_epi32(eeo[2], coeffs[2]);
                v[3] = _mm256_mullo_epi32(eeo[3], coeffs[2]);
                v[4] = _mm256_mullo_epi32(eeo[0], coeffs[3]);
                v[5] = _mm256_mullo_epi32(eeo[1], coeffs[3]);
                v[6] = _mm256_mullo_epi32(eeo[2], coeffs[3]);
                v[7] = _mm256_mullo_epi32(eeo[3], coeffs[3]);

                v[0] = _mm256_hadd_epi32(v[0], v[1]);
                v[2] = _mm256_hadd_epi32(v[2], v[3]);
                v[4] = _mm256_hadd_epi32(v[4], v[5]);
                v[6] = _mm256_hadd_epi32(v[6], v[7]);
                v[0] = _mm256_hadd_epi32(v[0], v[2]);               
                v[1] = _mm256_hadd_epi32(v[4], v[6]);               

                v[2] = _mm256_add_epi32(v[8], add);
                v[3] = _mm256_add_epi32(v[9], add);
                v[4] = _mm256_add_epi32(v[0], add);
                v[5] = _mm256_add_epi32(v[1], add);

                v[2] = _mm256_srai_epi32(v[2], shift);
                v[3] = _mm256_srai_epi32(v[3], shift);
                v[4] = _mm256_srai_epi32(v[4], shift);
                v[5] = _mm256_srai_epi32(v[5], shift);


                _mm256_storeu_si256((__m256i*)(pel_dst + 4 * line), v[2]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 12 * line), v[3]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 20 * line), v[4]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 28 * line), v[5]);

#define _mm256_madd_epi32_xeve(a, b, c, d) \
        _mm256_hadd_epi32(_mm256_mullo_epi32(a, b), _mm256_mullo_epi32(c, d))

                for (i = 0; i < 8; i++)
                {
                    int idx = 2 * i + 1;
                    __m256i tm_0 = _mm256_setr_epi32(xeve_tbl_tm16[idx][0], xeve_tbl_tm16[idx][1], xeve_tbl_tm16[idx][2], xeve_tbl_tm16[idx][3], xeve_tbl_tm16[idx][0], xeve_tbl_tm16[idx][1], xeve_tbl_tm16[idx][2], xeve_tbl_tm16[idx][3]);
                    __m256i tm_1 = _mm256_setr_epi32(xeve_tbl_tm16[idx][4], xeve_tbl_tm16[idx][5], xeve_tbl_tm16[idx][6], xeve_tbl_tm16[idx][7], xeve_tbl_tm16[idx][4], xeve_tbl_tm16[idx][5], xeve_tbl_tm16[idx][6], xeve_tbl_tm16[idx][7]);
                    v[0] = _mm256_madd_epi32_xeve(eo[0], tm_0, eo[1], tm_1);
                    v[2] = _mm256_madd_epi32_xeve(eo[2], tm_0, eo[3], tm_1);
                    v[4] = _mm256_madd_epi32_xeve(eo[4], tm_0, eo[5], tm_1);
                    v[6] = _mm256_madd_epi32_xeve(eo[6], tm_0, eo[7], tm_1);
                    v[0] = _mm256_hadd_epi32(v[0], v[2]);
                    v[4] = _mm256_hadd_epi32(v[4], v[6]);
                    dst_reg[i] = _mm256_hadd_epi32(v[0], v[4]);
                    dst_reg[i] = _mm256_add_epi32(dst_reg[i], add);
                    dst_reg[i] = _mm256_srai_epi32(dst_reg[i], shift);

                }

                _mm256_storeu_si256((__m256i*)(pel_dst + 2 * line), dst_reg[0]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 6 * line), dst_reg[1]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 10 * line), dst_reg[2]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 14 * line), dst_reg[3]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 18 * line), dst_reg[4]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 22 * line), dst_reg[5]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 26 * line), dst_reg[6]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 30 * line), dst_reg[7]);

#undef _mm256_madd_epi32_xeve

#define _mm256_madd1_epi32_xeve(a, b, c, d) \
        _mm256_add_epi32(_mm256_mullo_epi32(a, b), _mm256_mullo_epi32(c, d))

                for (i = 0; i < 8; ++i)
                {
                    int idx = 2 * i + 1;
                    __m256i tm_0 = _mm256_setr_epi32(xeve_tbl_tm32[idx][0], xeve_tbl_tm32[idx][1], xeve_tbl_tm32[idx][2], xeve_tbl_tm32[idx][3], xeve_tbl_tm32[idx][4], xeve_tbl_tm32[idx][5], xeve_tbl_tm32[idx][6], xeve_tbl_tm32[idx][7]);
                    __m256i tm_1 = _mm256_setr_epi32(xeve_tbl_tm32[idx][8], xeve_tbl_tm32[idx][9], xeve_tbl_tm32[idx][10], xeve_tbl_tm32[idx][11], xeve_tbl_tm32[idx][12], xeve_tbl_tm32[idx][13], xeve_tbl_tm32[idx][14], xeve_tbl_tm32[idx][15]);
                    v[0] = _mm256_madd1_epi32_xeve(o[0], tm_0, o[1], tm_1);
                    v[2] = _mm256_madd1_epi32_xeve(o[2], tm_0, o[3], tm_1);
                    v[4] = _mm256_madd1_epi32_xeve(o[4], tm_0, o[5], tm_1);
                    v[6] = _mm256_madd1_epi32_xeve(o[6], tm_0, o[7], tm_1);
                    v[8] = _mm256_madd1_epi32_xeve(o[8], tm_0, o[9], tm_1);
                    v[10] = _mm256_madd1_epi32_xeve(o[10], tm_0, o[11], tm_1);
                    v[12] = _mm256_madd1_epi32_xeve(o[12], tm_0, o[13], tm_1);
                    v[14] = _mm256_madd1_epi32_xeve(o[14], tm_0, o[15], tm_1);
                    v[0] = _mm256_hadd_epi32(v[0], v[2]);
                    v[4] = _mm256_hadd_epi32(v[4], v[6]);
                    v[8] = _mm256_hadd_epi32(v[8], v[10]);
                    v[12] = _mm256_hadd_epi32(v[12], v[14]);
                    v[0] = _mm256_hadd_epi32(v[0], v[4]);
                    v[8] = _mm256_hadd_epi32(v[8], v[12]);
                    v[2] = _mm256_permute2x128_si256(v[0], v[8], 0x20);
                    v[3] = _mm256_permute2x128_si256(v[0], v[8], 0x31);
                    dst_reg[i] = _mm256_add_epi32(v[2], v[3]);
                    dst_reg[i] = _mm256_add_epi32(dst_reg[i], add);
                    dst_reg[i] = _mm256_srai_epi32(dst_reg[i], shift);
                }

                _mm256_storeu_si256((__m256i*)(pel_dst + line), dst_reg[0]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 3 * line), dst_reg[1]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 5 * line), dst_reg[2]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 7 * line), dst_reg[3]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 9 * line), dst_reg[4]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 11 * line), dst_reg[5]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 13 * line), dst_reg[6]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 15 * line), dst_reg[7]);

                for (i = 8; i < 16; ++i)
                {
                    int idx = 2 * i + 1;
                    __m256i tm_0 = _mm256_setr_epi32(xeve_tbl_tm32[idx][0], xeve_tbl_tm32[idx][1], xeve_tbl_tm32[idx][2], xeve_tbl_tm32[idx][3], xeve_tbl_tm32[idx][4], xeve_tbl_tm32[idx][5], xeve_tbl_tm32[idx][6], xeve_tbl_tm32[idx][7]);
                    __m256i tm_1 = _mm256_setr_epi32(xeve_tbl_tm32[idx][8], xeve_tbl_tm32[idx][9], xeve_tbl_tm32[idx][10], xeve_tbl_tm32[idx][11], xeve_tbl_tm32[idx][12], xeve_tbl_tm32[idx][13], xeve_tbl_tm32[idx][14], xeve_tbl_tm32[idx][15]);
                    v[0] = _mm256_madd1_epi32_xeve(o[0], tm_0, o[1], tm_1);
                    v[2] = _mm256_madd1_epi32_xeve(o[2], tm_0, o[3], tm_1);
                    v[4] = _mm256_madd1_epi32_xeve(o[4], tm_0, o[5], tm_1);
                    v[6] = _mm256_madd1_epi32_xeve(o[6], tm_0, o[7], tm_1);
                    v[8] = _mm256_madd1_epi32_xeve(o[8], tm_0, o[9], tm_1);
                    v[10] = _mm256_madd1_epi32_xeve(o[10], tm_0, o[11], tm_1);
                    v[12] = _mm256_madd1_epi32_xeve(o[12], tm_0, o[13], tm_1);
                    v[14] = _mm256_madd1_epi32_xeve(o[14], tm_0, o[15], tm_1);
                    v[0] = _mm256_hadd_epi32(v[0], v[2]);
                    v[4] = _mm256_hadd_epi32(v[4], v[6]);
                    v[8] = _mm256_hadd_epi32(v[8], v[10]);
                    v[12] = _mm256_hadd_epi32(v[12], v[14]);
                    v[0] = _mm256_hadd_epi32(v[0], v[4]);
                    v[8] = _mm256_hadd_epi32(v[8], v[12]);
                    v[2] = _mm256_permute2x128_si256(v[0], v[8], 0x20);
                    v[3] = _mm256_permute2x128_si256(v[0], v[8], 0x31);
                    dst_reg[i - 8] = _mm256_add_epi32(v[2], v[3]);
                    dst_reg[i - 8] = _mm256_add_epi32(dst_reg[i - 8], add);
                    dst_reg[i - 8] = _mm256_srai_epi32(dst_reg[i - 8], shift);
                }

#undef _mm256_madd1_epi32_xeve

                _mm256_storeu_si256((__m256i*)(pel_dst + 17 * line), dst_reg[0]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 19 * line), dst_reg[1]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 21 * line), dst_reg[2]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 23 * line), dst_reg[3]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 25 * line), dst_reg[4]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 27 * line), dst_reg[5]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 29 * line), dst_reg[6]);
                _mm256_storeu_si256((__m256i*)(pel_dst + 31 * line), dst_reg[7]);
                pel_dst += 8;
            }
        }
    }
}

static void tx_pb64b_avx(void* src, void* dst, int shift, int line, int step)
{
    if (line % 4 != 0 || step == 1)
    {
        tx_pb64b(src, dst, shift, line, step);
    }
    else
    {   
        s16* pel_src = (s16*)src;
        s32* pel_dst = (s32*)dst;

        xeve_mset(pel_dst, 0, sizeof(s32) * MAX_TR_DIM);
        int i, j;
        __m256i s[32];
        __m256i t[16];
        __m256i tab0, tab1, tab2;
        __m256i e[16], o[16], ee[8], eo[8];
        __m256i eee[4], eeo[4];
        __m256i eeee[2], eeeo[2], eeeee, eeeeo;
        __m256i v[16];
        __m256i d0, d1, d2, d3;
        __m256i dst_reg[8];
        __m256i add = _mm256_set1_epi32(shift == 0 ? 0 : 1 << (shift - 1));
        __m256i coeffs[106];
        __m128i m0, m1, m2, m3;

        const __m256i coeff_p64_p64 = _mm256_set_epi32(64, 64, 64, 64, 64, 64, 64, 64);
        const __m256i coeff_p84_p35 = _mm256_set_epi32(35, 84, 35, 84, 35, 84, 35, 84);    

        tab0 = _mm256_loadu_si256((__m256i*)tab_dct2_2nd_shuffle_256i[3]);  
        tab1 = _mm256_loadu_si256((__m256i*)tab_dct2_2nd_shuffle_256i[1]);  
        tab2 = _mm256_loadu_si256((__m256i*)tab_dct2_2nd_shuffle_256i[2]);  
        coeffs[0] = _mm256_setr_epi32(xeve_tbl_tm8[1][0], xeve_tbl_tm8[1][1], xeve_tbl_tm8[1][2], xeve_tbl_tm8[1][3], xeve_tbl_tm8[1][0], xeve_tbl_tm8[1][1], xeve_tbl_tm8[1][2], xeve_tbl_tm8[1][3]);
        coeffs[1] = _mm256_setr_epi32(xeve_tbl_tm8[3][0], xeve_tbl_tm8[3][1], xeve_tbl_tm8[3][2], xeve_tbl_tm8[3][3], xeve_tbl_tm8[3][0], xeve_tbl_tm8[3][1], xeve_tbl_tm8[3][2], xeve_tbl_tm8[3][3]);
        coeffs[2] = _mm256_setr_epi32(xeve_tbl_tm16[1][0], xeve_tbl_tm16[1][1], xeve_tbl_tm16[1][2], xeve_tbl_tm16[1][3], xeve_tbl_tm16[1][0], xeve_tbl_tm16[1][1], xeve_tbl_tm16[1][2], xeve_tbl_tm16[1][3]);
        coeffs[3] = _mm256_setr_epi32(xeve_tbl_tm16[1][4], xeve_tbl_tm16[1][5], xeve_tbl_tm16[1][6], xeve_tbl_tm16[1][7], xeve_tbl_tm16[1][4], xeve_tbl_tm16[1][5], xeve_tbl_tm16[1][6], xeve_tbl_tm16[1][7]);
        coeffs[4] = _mm256_setr_epi32(xeve_tbl_tm16[3][0], xeve_tbl_tm16[3][1], xeve_tbl_tm16[3][2], xeve_tbl_tm16[3][3], xeve_tbl_tm16[3][0], xeve_tbl_tm16[3][1], xeve_tbl_tm16[3][2], xeve_tbl_tm16[3][3]);
        coeffs[5] = _mm256_setr_epi32(xeve_tbl_tm16[3][4], xeve_tbl_tm16[3][5], xeve_tbl_tm16[3][6], xeve_tbl_tm16[3][7], xeve_tbl_tm16[3][4], xeve_tbl_tm16[3][5], xeve_tbl_tm16[3][6], xeve_tbl_tm16[3][7]);
        coeffs[6] = _mm256_setr_epi32(xeve_tbl_tm16[5][0], xeve_tbl_tm16[5][1], xeve_tbl_tm16[5][2], xeve_tbl_tm16[5][3], xeve_tbl_tm16[5][0], xeve_tbl_tm16[5][1], xeve_tbl_tm16[5][2], xeve_tbl_tm16[5][3]);
        coeffs[7] = _mm256_setr_epi32(xeve_tbl_tm16[5][4], xeve_tbl_tm16[5][5], xeve_tbl_tm16[5][6], xeve_tbl_tm16[5][7], xeve_tbl_tm16[5][4], xeve_tbl_tm16[5][5], xeve_tbl_tm16[5][6], xeve_tbl_tm16[5][7]);
        coeffs[8] = _mm256_setr_epi32(xeve_tbl_tm16[7][0], xeve_tbl_tm16[7][1], xeve_tbl_tm16[7][2], xeve_tbl_tm16[7][3], xeve_tbl_tm16[7][0], xeve_tbl_tm16[7][1], xeve_tbl_tm16[7][2], xeve_tbl_tm16[7][3]);
        coeffs[9] = _mm256_setr_epi32(xeve_tbl_tm16[7][4], xeve_tbl_tm16[7][5], xeve_tbl_tm16[7][6], xeve_tbl_tm16[7][7], xeve_tbl_tm16[7][4], xeve_tbl_tm16[7][5], xeve_tbl_tm16[7][6], xeve_tbl_tm16[7][7]);

        for (j = 0; j < line; j += 4) {
            s[0] = _mm256_loadu_si256((__m256i*)(pel_src));
            s[1] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 1));
            s[2] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 2));
            s[3] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 3));
            s[4] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 4));
            s[5] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 5));
            s[6] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 6));
            s[7] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 7));
            s[8] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 8));
            s[9] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 9));
            s[10] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 10));
            s[11] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 11));
            s[12] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 12));
            s[13] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 13));
            s[14] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 14));
            s[15] = _mm256_loadu_si256((__m256i*)(pel_src + 16 * 15));

            t[0] = _mm256_shuffle_epi8(s[2], tab0);
            t[1] = _mm256_shuffle_epi8(s[3], tab0);
            t[2] = _mm256_shuffle_epi8(s[6], tab0);
            t[3] = _mm256_shuffle_epi8(s[7], tab0);
            t[4] = _mm256_shuffle_epi8(s[10], tab0);
            t[5] = _mm256_shuffle_epi8(s[11], tab0);
            t[6] = _mm256_shuffle_epi8(s[14], tab0);
            t[7] = _mm256_shuffle_epi8(s[15], tab0);

            s[3] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[1], 1));
            s[2] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[1]));
            s[1] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[0], 1));
            s[0] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[0]));
            s[7] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[5], 1));
            s[6] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[5]));
            s[5] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[4], 1));
            s[4] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[4]));
            s[11] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[9], 1));
            s[10] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[9]));
            s[9] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[8], 1));
            s[8] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[8]));
            s[15] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[13], 1));
            s[14] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[13]));
            s[13] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[12], 1));
            s[12] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[12]));

            s[18] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[0], 1));
            s[19] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[0]));
            s[16] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[1], 1));
            s[17] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[1]));
            s[22] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[2], 1));
            s[23] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[2]));
            s[20] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[3], 1));
            s[21] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[3]));
            s[26] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[4], 1));
            s[27] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[4]));
            s[24] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[5], 1));
            s[25] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[5]));
            s[30] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[6], 1));
            s[31] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[6]));
            s[28] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[7], 1));
            s[29] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[7]));

            pel_src += 64 * 4;

            for (i = 0; i < 16; i++) {
                e[i] = _mm256_add_epi32(s[i], s[16 + i]);               
                o[i] = _mm256_sub_epi32(s[i], s[16 + i]);
            }

            for (i = 0; i < 8; i++) {
                t[i * 2] = _mm256_permute2x128_si256(e[i], e[i + 8], 0x20);
                t[i * 2 + 1] = _mm256_permute2x128_si256(e[i], e[i + 8], 0x31);
            }

            t[4] = _mm256_shuffle_epi8(t[4], tab1);                     
            t[5] = _mm256_shuffle_epi8(t[5], tab1);                     
            t[6] = _mm256_shuffle_epi8(t[6], tab1);
            t[7] = _mm256_shuffle_epi8(t[7], tab1);
            t[12] = _mm256_shuffle_epi8(t[12], tab1);                   
            t[13] = _mm256_shuffle_epi8(t[13], tab1);                   
            t[14] = _mm256_shuffle_epi8(t[14], tab1);
            t[15] = _mm256_shuffle_epi8(t[15], tab1);

            for (i = 0; i < 8; i += 4) {
                int i2 = i * 2;
                ee[i] = _mm256_add_epi32(t[i2], t[i2 + 7]);             
                eo[i] = _mm256_sub_epi32(t[i2], t[i2 + 7]);             
                ee[i + 1] = _mm256_add_epi32(t[i2 + 1], t[i2 + 6]);     
                eo[i + 1] = _mm256_sub_epi32(t[i2 + 1], t[i2 + 6]);     
                ee[i + 2] = _mm256_add_epi32(t[i2 + 2], t[i2 + 5]);
                eo[i + 2] = _mm256_sub_epi32(t[i2 + 2], t[i2 + 5]);
                ee[i + 3] = _mm256_add_epi32(t[i2 + 3], t[i2 + 4]);
                eo[i + 3] = _mm256_sub_epi32(t[i2 + 3], t[i2 + 4]);
            }

            ee[2] = _mm256_shuffle_epi8(ee[2], tab1);                   
            ee[3] = _mm256_shuffle_epi8(ee[3], tab1);                   
            ee[6] = _mm256_shuffle_epi8(ee[6], tab1);                   
            ee[7] = _mm256_shuffle_epi8(ee[7], tab1);                   

            eee[0] = _mm256_add_epi32(ee[0], ee[3]);                    
            eeo[0] = _mm256_sub_epi32(ee[0], ee[3]);                    
            eee[1] = _mm256_add_epi32(ee[1], ee[2]);
            eeo[1] = _mm256_sub_epi32(ee[1], ee[2]);
            eee[2] = _mm256_add_epi32(ee[4], ee[7]);                    
            eeo[2] = _mm256_sub_epi32(ee[4], ee[7]);                    
            eee[3] = _mm256_add_epi32(ee[5], ee[6]);
            eeo[3] = _mm256_sub_epi32(ee[5], ee[6]);

            eee[1] = _mm256_shuffle_epi8(eee[1], tab1);                 
            eee[3] = _mm256_shuffle_epi8(eee[3], tab1);

            eeee[0] = _mm256_add_epi32(eee[0], eee[1]);                 
            eeeo[0] = _mm256_sub_epi32(eee[0], eee[1]);
            eeee[1] = _mm256_add_epi32(eee[2], eee[3]);                 
            eeeo[1] = _mm256_sub_epi32(eee[2], eee[3]);

            eeee[0] = _mm256_shuffle_epi8(eeee[0], tab2);               
            eeee[1] = _mm256_shuffle_epi8(eeee[1], tab2);

            eeeee = _mm256_hadd_epi32(eeee[0], eeee[1]);                
            eeeeo = _mm256_hsub_epi32(eeee[0], eeee[1]);

            v[0] = _mm256_mullo_epi32(eeeee, coeff_p64_p64);
            v[1] = _mm256_mullo_epi32(eeeeo, coeff_p84_p35);

            v[4] = _mm256_hadd_epi32(v[0], v[1]);                       

            v[0] = _mm256_mullo_epi32(eeeo[0], coeffs[0]);
            v[1] = _mm256_mullo_epi32(eeeo[1], coeffs[0]);
            v[2] = _mm256_mullo_epi32(eeeo[0], coeffs[1]);
            v[3] = _mm256_mullo_epi32(eeeo[1], coeffs[1]);

            v[0] = _mm256_hadd_epi32(v[0], v[1]);
            v[2] = _mm256_hadd_epi32(v[2], v[3]);
            v[0] = _mm256_hadd_epi32(v[0], v[2]);                       

            v[4] = _mm256_permute4x64_epi64(v[4], 0xd8);                
            v[0] = _mm256_permute4x64_epi64(v[0], 0xd8);                
            v[4] = _mm256_add_epi32(v[4], add);
            v[0] = _mm256_add_epi32(v[0], add);
            v[4] = _mm256_srai_epi32(v[4], shift);
            v[0] = _mm256_srai_epi32(v[0], shift);

            d0 = _mm256_packs_epi32(v[4], v[0]);                        

            m0 = _mm256_castsi256_si128(d0);
            m1 = _mm256_extracti128_si256(d0, 1);
            m2 = _mm_srli_si128(m0, 8);
            m3 = _mm_srli_si128(m1, 8);

            _mm_storeu_si128((__m128i*)(pel_dst), _mm256_castsi256_si128(v[4]));
            _mm_storeu_si128((__m128i*)(pel_dst + 8 * line), _mm256_castsi256_si128(v[0]));
            _mm_storeu_si128((__m128i*)(pel_dst + 16 * line), _mm256_extracti128_si256(v[4], 1));
            _mm_storeu_si128((__m128i*)(pel_dst + 24 * line), _mm256_extracti128_si256(v[0], 1));


#define _mm256_madd_epi32_xeve(a, b, c, d) \
        _mm256_hadd_epi32(_mm256_mullo_epi32(a, b), _mm256_mullo_epi32(c, d)); \


#define CALCU_EEO(coeff0, coeff1, dst) \
        v[0] = _mm256_madd_epi32_xeve(eeo[0], coeff0, eeo[1], coeff1); \
        v[2] = _mm256_madd_epi32_xeve(eeo[2], coeff0, eeo[3], coeff1); \
        dst = _mm256_hadd_epi32(v[0], v[2])

            CALCU_EEO(coeffs[2], coeffs[3], d0);
            CALCU_EEO(coeffs[4], coeffs[5], d1);
            CALCU_EEO(coeffs[6], coeffs[7], d2);
            CALCU_EEO(coeffs[8], coeffs[9], d3);

            d0 = _mm256_hadd_epi32(d0, d1);
            d2 = _mm256_hadd_epi32(d2, d3);

            d0 = _mm256_permute4x64_epi64(d0, 0xd8);                    
            d1 = _mm256_permute4x64_epi64(d2, 0xd8);

#undef CALCU_EEO
#undef _mm256_madd_epi32_xeve

            d0 = _mm256_add_epi32(d0, add);
            d1 = _mm256_add_epi32(d1, add);

            d0 = _mm256_srai_epi32(d0, shift);
            d1 = _mm256_srai_epi32(d1, shift);

            _mm_storeu_si128((__m128i*)(pel_dst + 4 * line), _mm256_castsi256_si128(d0));
            _mm_storeu_si128((__m128i*)(pel_dst + 12 * line), _mm256_extracti128_si256(d0, 1));
            _mm_storeu_si128((__m128i*)(pel_dst + 20 * line), _mm256_castsi256_si128(d1));
            _mm_storeu_si128((__m128i*)(pel_dst + 28 * line), _mm256_extracti128_si256(d1, 1));

#define _mm256_madd1_epi32_xeve(a, b, c, d) \
        _mm256_add_epi32(_mm256_mullo_epi32(a, b), _mm256_mullo_epi32(c, d))                    
            
            // EO
            for (i = 0; i < 8; ++i)
            {
                int idx = i * 2 + 1;
                __m256i tm_0 = _mm256_setr_epi32(xeve_tbl_tm32[idx][0], xeve_tbl_tm32[idx][1], xeve_tbl_tm32[idx][2], xeve_tbl_tm32[idx][3], xeve_tbl_tm32[idx][0], xeve_tbl_tm32[idx][1], xeve_tbl_tm32[idx][2], xeve_tbl_tm32[idx][3]);
                __m256i tm_1 = _mm256_setr_epi32(xeve_tbl_tm32[idx][4], xeve_tbl_tm32[idx][5], xeve_tbl_tm32[idx][6], xeve_tbl_tm32[idx][7], xeve_tbl_tm32[idx][4], xeve_tbl_tm32[idx][5], xeve_tbl_tm32[idx][6], xeve_tbl_tm32[idx][7]);
                __m256i tm_2 = _mm256_setr_epi32(xeve_tbl_tm32[idx][8], xeve_tbl_tm32[idx][9], xeve_tbl_tm32[idx][10], xeve_tbl_tm32[idx][11], xeve_tbl_tm32[idx][8], xeve_tbl_tm32[idx][9], xeve_tbl_tm32[idx][10], xeve_tbl_tm32[idx][11]);
                __m256i tm_3 = _mm256_setr_epi32(xeve_tbl_tm32[idx][12], xeve_tbl_tm32[idx][13], xeve_tbl_tm32[idx][14], xeve_tbl_tm32[idx][15], xeve_tbl_tm32[idx][12], xeve_tbl_tm32[idx][13], xeve_tbl_tm32[idx][14], xeve_tbl_tm32[idx][15]);
                v[0] = _mm256_madd1_epi32_xeve(eo[0], tm_0, eo[1], tm_1);
                v[2] = _mm256_madd1_epi32_xeve(eo[2], tm_2, eo[3], tm_3);
                v[4] = _mm256_madd1_epi32_xeve(eo[4], tm_0, eo[5], tm_1);
                v[6] = _mm256_madd1_epi32_xeve(eo[6], tm_2, eo[7], tm_3);
                v[0] = _mm256_add_epi32(v[0], v[2]);
                v[4] = _mm256_add_epi32(v[4], v[6]);
                dst_reg[i] = _mm256_hadd_epi32(v[0], v[4]);
            }

            d0 = _mm256_hadd_epi32(dst_reg[0], dst_reg[1]);                             
            d1 = _mm256_hadd_epi32(dst_reg[2], dst_reg[3]);
            d2 = _mm256_hadd_epi32(dst_reg[4], dst_reg[5]);
            d3 = _mm256_hadd_epi32(dst_reg[6], dst_reg[7]);

            d0 = _mm256_permute4x64_epi64(d0, 0xd8);
            d1 = _mm256_permute4x64_epi64(d1, 0xd8);
            d2 = _mm256_permute4x64_epi64(d2, 0xd8);
            d3 = _mm256_permute4x64_epi64(d3, 0xd8);

            CALCU_2x8_ADD_SHIFT(d0, d1, d2, d3, add, shift);

            _mm_storeu_si128((__m128i*)(pel_dst + 2 * line), _mm256_castsi256_si128(d0));
            _mm_storeu_si128((__m128i*)(pel_dst + 6 * line), _mm256_extracti128_si256(d0, 1));
            _mm_storeu_si128((__m128i*)(pel_dst + 10 * line), _mm256_castsi256_si128(d1));
            _mm_storeu_si128((__m128i*)(pel_dst + 14 * line), _mm256_extracti128_si256(d1, 1));
            _mm_storeu_si128((__m128i*)(pel_dst + 18 * line), _mm256_castsi256_si128(d2));
            _mm_storeu_si128((__m128i*)(pel_dst + 22 * line), _mm256_extracti128_si256(d2, 1));
            _mm_storeu_si128((__m128i*)(pel_dst + 26 * line), _mm256_castsi256_si128(d3));
            _mm_storeu_si128((__m128i*)(pel_dst + 30 * line), _mm256_extracti128_si256(d3, 1));
            

            for (i = 0; i < 8; ++i)
            {
                int idx = i * 2 + 1;
                __m256i tm_0 = _mm256_setr_epi32(xeve_tbl_tm64[idx][0], xeve_tbl_tm64[idx][1], xeve_tbl_tm64[idx][2], xeve_tbl_tm64[idx][3], xeve_tbl_tm64[idx][4], xeve_tbl_tm64[idx][5], xeve_tbl_tm64[idx][6], xeve_tbl_tm64[idx][7]);
                __m256i tm_1 = _mm256_setr_epi32(xeve_tbl_tm64[idx][8], xeve_tbl_tm64[idx][9], xeve_tbl_tm64[idx][10], xeve_tbl_tm64[idx][11], xeve_tbl_tm64[idx][12], xeve_tbl_tm64[idx][13], xeve_tbl_tm64[idx][14], xeve_tbl_tm64[idx][15]);
                __m256i tm_2 = _mm256_setr_epi32(xeve_tbl_tm64[idx][16], xeve_tbl_tm64[idx][17], xeve_tbl_tm64[idx][18], xeve_tbl_tm64[idx][19], xeve_tbl_tm64[idx][20], xeve_tbl_tm64[idx][21], xeve_tbl_tm64[idx][22], xeve_tbl_tm64[idx][23]);
                __m256i tm_3 = _mm256_setr_epi32(xeve_tbl_tm64[idx][24], xeve_tbl_tm64[idx][25], xeve_tbl_tm64[idx][26], xeve_tbl_tm64[idx][27], xeve_tbl_tm64[idx][28], xeve_tbl_tm64[idx][29], xeve_tbl_tm64[idx][30], xeve_tbl_tm64[idx][31]);
                v[0] = _mm256_madd1_epi32_xeve(o[0], tm_0, o[1], tm_1);
                v[2] = _mm256_madd1_epi32_xeve(o[2], tm_2, o[3], tm_3);
                v[4] = _mm256_madd1_epi32_xeve(o[4], tm_0, o[5], tm_1);
                v[6] = _mm256_madd1_epi32_xeve(o[6], tm_2, o[7], tm_3);
                v[8] = _mm256_madd1_epi32_xeve(o[8], tm_0, o[9], tm_1);
                v[10] = _mm256_madd1_epi32_xeve(o[10], tm_2, o[11], tm_3);
                v[12] = _mm256_madd1_epi32_xeve(o[12], tm_0, o[13], tm_1);
                v[14] = _mm256_madd1_epi32_xeve(o[14], tm_2, o[15], tm_3);
                v[0] = _mm256_add_epi32(v[0], v[2]);
                v[1] = _mm256_add_epi32(v[4], v[6]);
                v[2] = _mm256_add_epi32(v[8], v[10]);
                v[3] = _mm256_add_epi32(v[12], v[14]);
                v[0] = _mm256_hadd_epi32(v[0], v[1]);
                v[2] = _mm256_hadd_epi32(v[2], v[3]);
                dst_reg[i] = _mm256_hadd_epi32(v[0], v[2]);
            }

            t[0] = _mm256_permute2x128_si256(dst_reg[0], dst_reg[1], 0x20);
            t[1] = _mm256_permute2x128_si256(dst_reg[0], dst_reg[1], 0x31);
            t[2] = _mm256_permute2x128_si256(dst_reg[2], dst_reg[3], 0x20);
            t[3] = _mm256_permute2x128_si256(dst_reg[2], dst_reg[3], 0x31);
            t[4] = _mm256_permute2x128_si256(dst_reg[4], dst_reg[5], 0x20);
            t[5] = _mm256_permute2x128_si256(dst_reg[4], dst_reg[5], 0x31);
            t[6] = _mm256_permute2x128_si256(dst_reg[6], dst_reg[7], 0x20);
            t[7] = _mm256_permute2x128_si256(dst_reg[6], dst_reg[7], 0x31);

            d0 = _mm256_add_epi32(t[0], t[1]);
            d1 = _mm256_add_epi32(t[2], t[3]);
            d2 = _mm256_add_epi32(t[4], t[5]);
            d3 = _mm256_add_epi32(t[6], t[7]);

            CALCU_2x8_ADD_SHIFT(d0, d1, d2, d3, add, shift);

            _mm_storeu_si128((__m128i*)(pel_dst + 1 * line), _mm256_castsi256_si128(d0));
            _mm_storeu_si128((__m128i*)(pel_dst + 3 * line), _mm256_extracti128_si256(d0, 1));
            _mm_storeu_si128((__m128i*)(pel_dst + 5 * line), _mm256_castsi256_si128(d1));
            _mm_storeu_si128((__m128i*)(pel_dst + 7 * line), _mm256_extracti128_si256(d1, 1));
            _mm_storeu_si128((__m128i*)(pel_dst + 9 * line), _mm256_castsi256_si128(d2));
            _mm_storeu_si128((__m128i*)(pel_dst + 11 * line), _mm256_extracti128_si256(d2, 1));
            _mm_storeu_si128((__m128i*)(pel_dst + 13 * line), _mm256_castsi256_si128(d3));
            _mm_storeu_si128((__m128i*)(pel_dst + 15 * line), _mm256_extracti128_si256(d3, 1));

            for (i = 8; i < 16; ++i)
            {
                int idx = i * 2 + 1;
                __m256i tm_0 = _mm256_setr_epi32(xeve_tbl_tm64[idx][0], xeve_tbl_tm64[idx][1], xeve_tbl_tm64[idx][2], xeve_tbl_tm64[idx][3], xeve_tbl_tm64[idx][4], xeve_tbl_tm64[idx][5], xeve_tbl_tm64[idx][6], xeve_tbl_tm64[idx][7]);
                __m256i tm_1 = _mm256_setr_epi32(xeve_tbl_tm64[idx][8], xeve_tbl_tm64[idx][9], xeve_tbl_tm64[idx][10], xeve_tbl_tm64[idx][11], xeve_tbl_tm64[idx][12], xeve_tbl_tm64[idx][13], xeve_tbl_tm64[idx][14], xeve_tbl_tm64[idx][15]);
                __m256i tm_2 = _mm256_setr_epi32(xeve_tbl_tm64[idx][16], xeve_tbl_tm64[idx][17], xeve_tbl_tm64[idx][18], xeve_tbl_tm64[idx][19], xeve_tbl_tm64[idx][20], xeve_tbl_tm64[idx][21], xeve_tbl_tm64[idx][22], xeve_tbl_tm64[idx][23]);
                __m256i tm_3 = _mm256_setr_epi32(xeve_tbl_tm64[idx][24], xeve_tbl_tm64[idx][25], xeve_tbl_tm64[idx][26], xeve_tbl_tm64[idx][27], xeve_tbl_tm64[idx][28], xeve_tbl_tm64[idx][29], xeve_tbl_tm64[idx][30], xeve_tbl_tm64[idx][31]);
                v[0] = _mm256_madd1_epi32_xeve(o[0], tm_0, o[1], tm_1);
                v[2] = _mm256_madd1_epi32_xeve(o[2], tm_2, o[3], tm_3);
                v[4] = _mm256_madd1_epi32_xeve(o[4], tm_0, o[5], tm_1);
                v[6] = _mm256_madd1_epi32_xeve(o[6], tm_2, o[7], tm_3);
                v[8] = _mm256_madd1_epi32_xeve(o[8], tm_0, o[9], tm_1);
                v[10] = _mm256_madd1_epi32_xeve(o[10], tm_2, o[11], tm_3);
                v[12] = _mm256_madd1_epi32_xeve(o[12], tm_0, o[13], tm_1);
                v[14] = _mm256_madd1_epi32_xeve(o[14], tm_2, o[15], tm_3);
                v[0] = _mm256_add_epi32(v[0], v[2]);
                v[1] = _mm256_add_epi32(v[4], v[6]);
                v[2] = _mm256_add_epi32(v[8], v[10]);
                v[3] = _mm256_add_epi32(v[12], v[14]);
                v[0] = _mm256_hadd_epi32(v[0], v[1]);
                v[2] = _mm256_hadd_epi32(v[2], v[3]);
                dst_reg[i - 8] = _mm256_hadd_epi32(v[0], v[2]);
            }


#undef CALCU_O

            t[0] = _mm256_permute2x128_si256(dst_reg[0], dst_reg[1], 0x20);
            t[1] = _mm256_permute2x128_si256(dst_reg[0], dst_reg[1], 0x31);
            t[2] = _mm256_permute2x128_si256(dst_reg[2], dst_reg[3], 0x20);
            t[3] = _mm256_permute2x128_si256(dst_reg[2], dst_reg[3], 0x31);
            t[4] = _mm256_permute2x128_si256(dst_reg[4], dst_reg[5], 0x20);
            t[5] = _mm256_permute2x128_si256(dst_reg[4], dst_reg[5], 0x31);
            t[6] = _mm256_permute2x128_si256(dst_reg[6], dst_reg[7], 0x20);
            t[7] = _mm256_permute2x128_si256(dst_reg[6], dst_reg[7], 0x31);

            d0 = _mm256_add_epi32(t[0], t[1]);
            d1 = _mm256_add_epi32(t[2], t[3]);
            d2 = _mm256_add_epi32(t[4], t[5]);
            d3 = _mm256_add_epi32(t[6], t[7]);

            CALCU_2x8_ADD_SHIFT(d0, d1, d2, d3, add, shift)

            _mm_storeu_si128((__m128i*)(pel_dst + 17 * line), _mm256_castsi256_si128(d0));
            _mm_storeu_si128((__m128i*)(pel_dst + 19 * line), _mm256_extracti128_si256(d0, 1));
            _mm_storeu_si128((__m128i*)(pel_dst + 21 * line), _mm256_castsi256_si128(d1));
            _mm_storeu_si128((__m128i*)(pel_dst + 23 * line), _mm256_extracti128_si256(d1, 1));
            _mm_storeu_si128((__m128i*)(pel_dst + 25 * line), _mm256_castsi256_si128(d2));
            _mm_storeu_si128((__m128i*)(pel_dst + 27 * line), _mm256_extracti128_si256(d2, 1));
            _mm_storeu_si128((__m128i*)(pel_dst + 29 * line), _mm256_castsi256_si128(d3));
            _mm_storeu_si128((__m128i*)(pel_dst + 31 * line), _mm256_extracti128_si256(d3, 1));

            pel_dst += 4;
        }
    }
}

const XEVE_TXB xeve_tbl_txb_avx[MAX_TR_LOG2] =
{
    tx_pb2b,
    tx_pb4b,
    tx_pb8b_avx,
    tx_pb16b_avx,
    tx_pb32b_avx,
    tx_pb64b_avx
};
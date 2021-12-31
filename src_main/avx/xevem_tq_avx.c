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

#include "xevem_type.h"
#include "xevem_tq_avx.h"
#include "xevem_tq.h"

#define _mm256_loadu2_m128i(/* __m128i const* */ hiaddr, \
                            /* __m128i const* */ loaddr) \
    _mm256_set_m128i(_mm_loadu_si128(hiaddr), _mm_loadu_si128(loaddr))

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

static void tx_pb8_avx(s16* src, s16* dst, int shift, int line)
{
    __m256i v0, v1, v2, v3, v4, v5, v6, v7;
    __m256i d0, d1, d2, d3;
    __m256i coeff[8];
    coeff[0] = _mm256_set1_epi16(64);
    coeff[1] = _mm256_set_epi16(64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64);
    coeff[2] = _mm256_set_epi16(84, 35, -35, -84, -84, -35, 35, 84, 84, 35, -35, -84, -84, -35, 35, 84);
    coeff[3] = _mm256_set_epi16(35, -84, 84, -35, -35, 84, -84, 35, 35, -84, 84, -35, -35, 84, -84, 35);
    coeff[4] = _mm256_set_epi16(-89, -75, -50, -18, 18, 50, 75, 89, -89, -75, -50, -18, 18, 50, 75, 89);
    coeff[5] = _mm256_set_epi16(-75, 18, 89, 50, -50, -89, -18, 75, -75, 18, 89, 50, -50, -89, -18, 75);
    coeff[6] = _mm256_set_epi16(-50, 89, -18, -75, 75, 18, -89, 50, -50, 89, -18, -75, 75, 18, -89, 50);
    coeff[7] = _mm256_set_epi16(-18, 50, -75, 89, -89, 75, -50, 18, -18, 50, -75, 89, -89, 75, -50, 18);
    __m256i add = _mm256_set1_epi32(1 << (shift - 1));

    if (line > 4)
    {

        int j;
        __m256i s0, s1, s2, s3;

        for (j = 0; j < line; j += 8)
        {
            s0 = _mm256_loadu2_m128i((const __m128i*) & src[4 * 8], (const __m128i*) & src[0]);
            s1 = _mm256_loadu2_m128i((const __m128i*) & src[5 * 8], (const __m128i*) & src[8]);
            s2 = _mm256_loadu2_m128i((const __m128i*) & src[6 * 8], (const __m128i*) & src[16]);
            s3 = _mm256_loadu2_m128i((const __m128i*) & src[7 * 8], (const __m128i*) & src[24]);

            src += 8 * 8;

#define CALCU_2x8(c0, c1, d0, d1) \
            v0 = _mm256_madd_epi16(s0, c0); \
            v1 = _mm256_madd_epi16(s1, c0); \
            v2 = _mm256_madd_epi16(s2, c0); \
            v3 = _mm256_madd_epi16(s3, c0); \
            v4 = _mm256_madd_epi16(s0, c1); \
            v5 = _mm256_madd_epi16(s1, c1); \
            v6 = _mm256_madd_epi16(s2, c1); \
            v7 = _mm256_madd_epi16(s3, c1); \
            v0 = _mm256_hadd_epi32(v0, v1); \
            v2 = _mm256_hadd_epi32(v2, v3); \
            v4 = _mm256_hadd_epi32(v4, v5); \
            v6 = _mm256_hadd_epi32(v6, v7); \
            d0 = _mm256_hadd_epi32(v0, v2); \
            d1 = _mm256_hadd_epi32(v4, v6)

            CALCU_2x8(coeff[0], coeff[4], d0, d1);
            CALCU_2x8(coeff[2], coeff[5], d2, d3);

            d0 = _mm256_add_epi32(d0, add);
            d1 = _mm256_add_epi32(d1, add);
            d2 = _mm256_add_epi32(d2, add);
            d3 = _mm256_add_epi32(d3, add);

            d0 = _mm256_srai_epi32(d0, shift);
            d1 = _mm256_srai_epi32(d1, shift);
            d2 = _mm256_srai_epi32(d2, shift);
            d3 = _mm256_srai_epi32(d3, shift);

            d0 = _mm256_packs_epi32(d0, d1);
            d1 = _mm256_packs_epi32(d2, d3);

            d0 = _mm256_permute4x64_epi64(d0, 0xd8);
            d1 = _mm256_permute4x64_epi64(d1, 0xd8);

            _mm_store_si128((__m128i*)dst, _mm256_castsi256_si128(d0));
            _mm_store_si128((__m128i*)(dst + 1 * line), _mm256_extracti128_si256(d0, 1));
            _mm_store_si128((__m128i*)(dst + 2 * line), _mm256_castsi256_si128(d1));
            _mm_store_si128((__m128i*)(dst + 3 * line), _mm256_extracti128_si256(d1, 1));

            CALCU_2x8(coeff[1], coeff[6], d0, d1);
            CALCU_2x8(coeff[3], coeff[7], d2, d3);
#undef CALCU_2x8

            d0 = _mm256_add_epi32(d0, add);
            d1 = _mm256_add_epi32(d1, add);
            d2 = _mm256_add_epi32(d2, add);
            d3 = _mm256_add_epi32(d3, add);

            d0 = _mm256_srai_epi32(d0, shift);
            d1 = _mm256_srai_epi32(d1, shift);
            d2 = _mm256_srai_epi32(d2, shift);
            d3 = _mm256_srai_epi32(d3, shift);

            d0 = _mm256_packs_epi32(d0, d1);
            d1 = _mm256_packs_epi32(d2, d3);

            d0 = _mm256_permute4x64_epi64(d0, 0xd8);
            d1 = _mm256_permute4x64_epi64(d1, 0xd8);

            _mm_store_si128((__m128i*)(dst + 4 * line), _mm256_castsi256_si128(d0));
            _mm_store_si128((__m128i*)(dst + 5 * line), _mm256_extracti128_si256(d0, 1));
            _mm_store_si128((__m128i*)(dst + 6 * line), _mm256_castsi256_si128(d1));
            _mm_store_si128((__m128i*)(dst + 7 * line), _mm256_extracti128_si256(d1, 1));

            dst += 8;
        }
    }
    else if (line == 4) {
        __m256i s0, s1;

        s0 = _mm256_loadu2_m128i((const __m128i*) & src[2 * 8], (const __m128i*) & src[0]);
        s1 = _mm256_loadu2_m128i((const __m128i*) & src[3 * 8], (const __m128i*) & src[8]);

#define CALCU_2x8(c0, c1, c2, c3, d0, d1) \
            v0 = _mm256_madd_epi16(s0, c0); \
            v1 = _mm256_madd_epi16(s1, c0); \
            v2 = _mm256_madd_epi16(s0, c1); \
            v3 = _mm256_madd_epi16(s1, c1); \
            v4 = _mm256_madd_epi16(s0, c2); \
            v5 = _mm256_madd_epi16(s1, c2); \
            v6 = _mm256_madd_epi16(s0, c3); \
            v7 = _mm256_madd_epi16(s1, c3); \
            v0 = _mm256_hadd_epi32(v0, v1); \
            v2 = _mm256_hadd_epi32(v2, v3); \
            v4 = _mm256_hadd_epi32(v4, v5); \
            v6 = _mm256_hadd_epi32(v6, v7); \
            d0 = _mm256_hadd_epi32(v0, v2); \
            d1 = _mm256_hadd_epi32(v4, v6); \
            d0 = _mm256_permute4x64_epi64(d0, 0xd8); \
            d1 = _mm256_permute4x64_epi64(d1, 0xd8)

        CALCU_2x8(coeff[0], coeff[4], coeff[2], coeff[5], d0, d1);
        CALCU_2x8(coeff[1], coeff[6], coeff[3], coeff[7], d2, d3);

        d0 = _mm256_add_epi32(d0, add);
        d1 = _mm256_add_epi32(d1, add);
        d2 = _mm256_add_epi32(d2, add);
        d3 = _mm256_add_epi32(d3, add);

        d0 = _mm256_srai_epi32(d0, shift);
        d1 = _mm256_srai_epi32(d1, shift);
        d2 = _mm256_srai_epi32(d2, shift);
        d3 = _mm256_srai_epi32(d3, shift);

        d0 = _mm256_packs_epi32(d0, d1);
        d1 = _mm256_packs_epi32(d2, d3);

        d0 = _mm256_permute4x64_epi64(d0, 0xd8);
        d1 = _mm256_permute4x64_epi64(d1, 0xd8);

        _mm256_storeu_si256((__m256i*)dst, d0);
        _mm256_storeu_si256((__m256i*)(dst + 16), d1);
    }
    else
    {
        tx_pb8(src, dst, shift, line);
    }
}

static void tx_pb16_avx(s16* src, s16* dst, int shift, int line)
{
    if (line > 4)
    {
        int i, j;
        __m256i s00, s01, s02, s03, s04, s05, s06, s07;
        __m256i v0, v1, v2, v3, v4, v5, v6, v7;
        __m256i d0, d1, d2, d3, d4, d5, d6, d7;
        __m256i dst_reg[8];
        __m256i add = _mm256_set1_epi32(1 << (shift - 1));
        __m256i coeffs[8];

        for (j = 0; j < line; j += 8)
        {
            s00 = _mm256_loadu_si256((__m256i*)(src));
            s01 = _mm256_loadu_si256((__m256i*)(src + 16));
            s02 = _mm256_loadu_si256((__m256i*)(src + 16 * 2));
            s03 = _mm256_loadu_si256((__m256i*)(src + 16 * 3));
            s04 = _mm256_loadu_si256((__m256i*)(src + 16 * 4));
            s05 = _mm256_loadu_si256((__m256i*)(src + 16 * 5));
            s06 = _mm256_loadu_si256((__m256i*)(src + 16 * 6));
            s07 = _mm256_loadu_si256((__m256i*)(src + 16 * 7));

            coeffs[0] = _mm256_set1_epi16(64);
            coeffs[1] = _mm256_set_epi16(-90, -87, -80, -70, -57, -43, -26, -9, 9, 26, 43, 57, 70, 80, 87, 90);
            coeffs[2] = _mm256_set_epi16(89, 75, 50, 18, -18, -50, -75, -89, -89, -75, -50, -18, 18, 50, 75, 89);
            coeffs[3] = _mm256_set_epi16(-87, -57, -9, 43, 80, 90, 70, 26, -26, -70, -90, -80, -43, 9, 57, 87);
            coeffs[4] = _mm256_set_epi16(84, 35, -35, -84, -84, -35, 35, 84, 84, 35, -35, -84, -84, -35, 35, 84);
            coeffs[5] = _mm256_set_epi16(-80, -9, 70, 87, 26, -57, -90, -43, 43, 90, 57, -26, -87, -70, 9, 80);
            coeffs[6] = _mm256_set_epi16(75, -18, -89, -50, 50, 89, 18, -75, -75, 18, 89, 50, -50, -89, -18, 75);
            coeffs[7] = _mm256_set_epi16(-70, 43, 87, -9, -90, -26, 80, 57, -57, -80, 26, 90, 9, -87, -43, 70);

            src += 16 * 8;

            for (i = 0; i < 8; ++i)
            {
                v0 = _mm256_hadd_epi32(_mm256_madd_epi16(s00, coeffs[i]), _mm256_madd_epi16(s01, coeffs[i]));
                v2 = _mm256_hadd_epi32(_mm256_madd_epi16(s02, coeffs[i]), _mm256_madd_epi16(s03, coeffs[i]));
                v4 = _mm256_hadd_epi32(_mm256_madd_epi16(s04, coeffs[i]), _mm256_madd_epi16(s05, coeffs[i]));
                v6 = _mm256_hadd_epi32(_mm256_madd_epi16(s06, coeffs[i]), _mm256_madd_epi16(s07, coeffs[i]));
                v0 = _mm256_hadd_epi32(v0, v2);
                v4 = _mm256_hadd_epi32(v4, v6);
                v1 = _mm256_permute2x128_si256(v0, v4, 0x20);
                v2 = _mm256_permute2x128_si256(v0, v4, 0x31);
                dst_reg[i] = _mm256_add_epi32(v1, v2);
                dst_reg[i] = _mm256_add_epi32(dst_reg[i], add);
                dst_reg[i] = _mm256_srai_epi32(dst_reg[i], shift);
            }

            d0 = _mm256_packs_epi32(dst_reg[0], dst_reg[1]);
            d1 = _mm256_packs_epi32(dst_reg[2], dst_reg[3]);
            d2 = _mm256_packs_epi32(dst_reg[4], dst_reg[5]);
            d3 = _mm256_packs_epi32(dst_reg[6], dst_reg[7]);
            d0 = _mm256_permute4x64_epi64(d0, 0xd8);
            d1 = _mm256_permute4x64_epi64(d1, 0xd8);
            d2 = _mm256_permute4x64_epi64(d2, 0xd8);
            d3 = _mm256_permute4x64_epi64(d3, 0xd8);

            coeffs[0] = _mm256_set_epi16(64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64);
            coeffs[1] = _mm256_set_epi16(-57, 80, 26, -90, 9, 87, -43, -70, 70, 43, -87, -9, 90, -26, -80, 57);
            coeffs[2] = _mm256_set_epi16(50, -89, 18, 75, -75, -18, 89, -50, -50, 89, -18, -75, 75, 18, -89, 50);
            coeffs[3] = _mm256_set_epi16(-43, 90, -57, -26, 87, -70, -9, 80, -80, 9, 70, -87, 26, 57, -90, 43);
            coeffs[4] = _mm256_set_epi16(35, -84, 84, -35, -35, 84, -84, 35, 35, -84, 84, -35, -35, 84, -84, 35);
            coeffs[5] = _mm256_set_epi16(-26, 70, -90, 80, -43, -9, 57, -87, 87, -57, 9, 43, -80, 90, -70, 26);
            coeffs[6] = _mm256_set_epi16(18, -50, 75, -89, 89, -75, 50, -18, -18, 50, -75, 89, -89, 75, -50, 18);
            coeffs[7] = _mm256_set_epi16(-9, 26, -43, 57, -70, 80, -87, 90, -90, 87, -80, 70, -57, 43, -26, 9);

            _mm_store_si128((__m128i*)(dst), _mm256_castsi256_si128(d0));
            _mm_store_si128((__m128i*)(dst + 1 * line), _mm256_extracti128_si256(d0, 1));
            _mm_store_si128((__m128i*)(dst + 2 * line), _mm256_castsi256_si128(d1));
            _mm_store_si128((__m128i*)(dst + 3 * line), _mm256_extracti128_si256(d1, 1));
            _mm_store_si128((__m128i*)(dst + 4 * line), _mm256_castsi256_si128(d2));
            _mm_store_si128((__m128i*)(dst + 5 * line), _mm256_extracti128_si256(d2, 1));
            _mm_store_si128((__m128i*)(dst + 6 * line), _mm256_castsi256_si128(d3));
            _mm_store_si128((__m128i*)(dst + 7 * line), _mm256_extracti128_si256(d3, 1));

            for (i = 0; i < 8; ++i)
            {
                v0 = _mm256_hadd_epi32(_mm256_madd_epi16(s00, coeffs[i]), _mm256_madd_epi16(s01, coeffs[i]));
                v2 = _mm256_hadd_epi32(_mm256_madd_epi16(s02, coeffs[i]), _mm256_madd_epi16(s03, coeffs[i]));
                v4 = _mm256_hadd_epi32(_mm256_madd_epi16(s04, coeffs[i]), _mm256_madd_epi16(s05, coeffs[i]));
                v6 = _mm256_hadd_epi32(_mm256_madd_epi16(s06, coeffs[i]), _mm256_madd_epi16(s07, coeffs[i]));
                v0 = _mm256_hadd_epi32(v0, v2);
                v4 = _mm256_hadd_epi32(v4, v6);
                v1 = _mm256_permute2x128_si256(v0, v4, 0x20);
                v2 = _mm256_permute2x128_si256(v0, v4, 0x31);
                dst_reg[i] = _mm256_add_epi32(v1, v2);
                dst_reg[i] = _mm256_add_epi32(dst_reg[i], add);
                dst_reg[i] = _mm256_srai_epi32(dst_reg[i], shift);
            }

            d0 = _mm256_packs_epi32(dst_reg[0], dst_reg[1]);
            d1 = _mm256_packs_epi32(dst_reg[2], dst_reg[3]);
            d2 = _mm256_packs_epi32(dst_reg[4], dst_reg[5]);
            d3 = _mm256_packs_epi32(dst_reg[6], dst_reg[7]);
            d0 = _mm256_permute4x64_epi64(d0, 0xd8);
            d1 = _mm256_permute4x64_epi64(d1, 0xd8);
            d2 = _mm256_permute4x64_epi64(d2, 0xd8);
            d3 = _mm256_permute4x64_epi64(d3, 0xd8);

            _mm_store_si128((__m128i*)(dst + 8 * line), _mm256_castsi256_si128(d0));
            _mm_store_si128((__m128i*)(dst + 9 * line), _mm256_extracti128_si256(d0, 1));
            _mm_store_si128((__m128i*)(dst + 10 * line), _mm256_castsi256_si128(d1));
            _mm_store_si128((__m128i*)(dst + 11 * line), _mm256_extracti128_si256(d1, 1));
            _mm_store_si128((__m128i*)(dst + 12 * line), _mm256_castsi256_si128(d2));
            _mm_store_si128((__m128i*)(dst + 13 * line), _mm256_extracti128_si256(d2, 1));
            _mm_store_si128((__m128i*)(dst + 14 * line), _mm256_castsi256_si128(d3));
            _mm_store_si128((__m128i*)(dst + 15 * line), _mm256_extracti128_si256(d3, 1));

            dst += 8;
        }
    }
    else if (line == 4) {
        __m256i s00, s01, s02, s03;
        __m256i v0, v1, v2, v3, v4, v5, v6, v7;
        __m256i d0, d1, d2, d3;
        __m256i add = _mm256_set1_epi32(1 << (shift - 1));
        __m256i coeffs[8];
        __m256i dst_reg[8];
        s00 = _mm256_loadu_si256((__m256i*)(src));            // src[0][0-15]
        s01 = _mm256_loadu_si256((__m256i*)(src + 16));
        s02 = _mm256_loadu_si256((__m256i*)(src + 16 * 2));
        s03 = _mm256_loadu_si256((__m256i*)(src + 16 * 3));

        coeffs[0] = _mm256_set1_epi16(64);
        coeffs[1] = _mm256_set_epi16(-90, -87, -80, -70, -57, -43, -26, -9, 9, 26, 43, 57, 70, 80, 87, 90);
        coeffs[2] = _mm256_set_epi16(89, 75, 50, 18, -18, -50, -75, -89, -89, -75, -50, -18, 18, 50, 75, 89);
        coeffs[3] = _mm256_set_epi16(-87, -57, -9, 43, 80, 90, 70, 26, -26, -70, -90, -80, -43, 9, 57, 87);
        coeffs[4] = _mm256_set_epi16(84, 35, -35, -84, -84, -35, 35, 84, 84, 35, -35, -84, -84, -35, 35, 84);
        coeffs[5] = _mm256_set_epi16(-80, -9, 70, 87, 26, -57, -90, -43, 43, 90, 57, -26, -87, -70, 9, 80);
        coeffs[6] = _mm256_set_epi16(75, -18, -89, -50, 50, 89, 18, -75, -75, 18, 89, 50, -50, -89, -18, 75);
        coeffs[7] = _mm256_set_epi16(-70, 43, 87, -9, -90, -26, 80, 57, -57, -80, 26, 90, 9, -87, -43, 70);

        src += 16 * 8;

        for (int i = 0; i < 8; i += 2)
        {
            v0 = _mm256_madd_epi16(s00, coeffs[i]);
            v1 = _mm256_madd_epi16(s01, coeffs[i]);
            v2 = _mm256_madd_epi16(s02, coeffs[i]);
            v3 = _mm256_madd_epi16(s03, coeffs[i]);
            v4 = _mm256_madd_epi16(s00, coeffs[i + 1]);
            v5 = _mm256_madd_epi16(s01, coeffs[i + 1]);
            v6 = _mm256_madd_epi16(s02, coeffs[i + 1]);
            v7 = _mm256_madd_epi16(s03, coeffs[i + 1]);
            v0 = _mm256_hadd_epi32(v0, v1);
            v2 = _mm256_hadd_epi32(v2, v3);
            v4 = _mm256_hadd_epi32(v4, v5);
            v6 = _mm256_hadd_epi32(v6, v7);
            v0 = _mm256_hadd_epi32(v0, v2);
            v4 = _mm256_hadd_epi32(v4, v6);
            v1 = _mm256_permute2x128_si256(v0, v4, 0x20);
            v2 = _mm256_permute2x128_si256(v0, v4, 0x31);
            dst_reg[i] = _mm256_add_epi32(v1, v2);
        }

        d0 = _mm256_add_epi32(dst_reg[0], add);
        d1 = _mm256_add_epi32(dst_reg[2], add);
        d2 = _mm256_add_epi32(dst_reg[4], add);
        d3 = _mm256_add_epi32(dst_reg[6], add);

        d0 = _mm256_srai_epi32(d0, shift);
        d1 = _mm256_srai_epi32(d1, shift);
        d2 = _mm256_srai_epi32(d2, shift);
        d3 = _mm256_srai_epi32(d3, shift);

        d0 = _mm256_packs_epi32(d0, d1);
        d1 = _mm256_packs_epi32(d2, d3);
        d0 = _mm256_permute4x64_epi64(d0, 0xd8);
        d1 = _mm256_permute4x64_epi64(d1, 0xd8);

        coeffs[0] = _mm256_set_epi16(64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64, 64, -64, -64, 64);
        coeffs[1] = _mm256_set_epi16(-57, 80, 26, -90, 9, 87, -43, -70, 70, 43, -87, -9, 90, -26, -80, 57);
        coeffs[2] = _mm256_set_epi16(50, -89, 18, 75, -75, -18, 89, -50, -50, 89, -18, -75, 75, 18, -89, 50);
        coeffs[3] = _mm256_set_epi16(-43, 90, -57, -26, 87, -70, -9, 80, -80, 9, 70, -87, 26, 57, -90, 43);
        coeffs[4] = _mm256_set_epi16(35, -84, 84, -35, -35, 84, -84, 35, 35, -84, 84, -35, -35, 84, -84, 35);
        coeffs[5] = _mm256_set_epi16(-26, 70, -90, 80, -43, -9, 57, -87, 87, -57, 9, 43, -80, 90, -70, 26);
        coeffs[6] = _mm256_set_epi16(18, -50, 75, -89, 89, -75, 50, -18, -18, 50, -75, 89, -89, 75, -50, 18);
        coeffs[7] = _mm256_set_epi16(-9, 26, -43, 57, -70, 80, -87, 90, -90, 87, -80, 70, -57, 43, -26, 9);

        _mm256_storeu_si256((__m256i*)(dst), d0);
        _mm256_storeu_si256((__m256i*)(dst + 16), d1);

        for (int i = 0; i < 8; i += 2)
        {
            v0 = _mm256_madd_epi16(s00, coeffs[i]);
            v1 = _mm256_madd_epi16(s01, coeffs[i]);
            v2 = _mm256_madd_epi16(s02, coeffs[i]);
            v3 = _mm256_madd_epi16(s03, coeffs[i]);
            v4 = _mm256_madd_epi16(s00, coeffs[i + 1]);
            v5 = _mm256_madd_epi16(s01, coeffs[i + 1]);
            v6 = _mm256_madd_epi16(s02, coeffs[i + 1]);
            v7 = _mm256_madd_epi16(s03, coeffs[i + 1]);
            v0 = _mm256_hadd_epi32(v0, v1);
            v2 = _mm256_hadd_epi32(v2, v3);
            v4 = _mm256_hadd_epi32(v4, v5);
            v6 = _mm256_hadd_epi32(v6, v7);
            v0 = _mm256_hadd_epi32(v0, v2);
            v4 = _mm256_hadd_epi32(v4, v6);
            v1 = _mm256_permute2x128_si256(v0, v4, 0x20);
            v2 = _mm256_permute2x128_si256(v0, v4, 0x31);
            dst_reg[i] = _mm256_add_epi32(v1, v2);
        }

#undef CALCU_LINE

        d0 = _mm256_add_epi32(dst_reg[0], add);
        d1 = _mm256_add_epi32(dst_reg[2], add);
        d2 = _mm256_add_epi32(dst_reg[4], add);
        d3 = _mm256_add_epi32(dst_reg[6], add);

        d0 = _mm256_srai_epi32(d0, shift);
        d1 = _mm256_srai_epi32(d1, shift);
        d2 = _mm256_srai_epi32(d2, shift);
        d3 = _mm256_srai_epi32(d3, shift);

        d0 = _mm256_packs_epi32(d0, d1);
        d1 = _mm256_packs_epi32(d2, d3);
        d0 = _mm256_permute4x64_epi64(d0, 0xd8);
        d1 = _mm256_permute4x64_epi64(d1, 0xd8);

        _mm256_storeu_si256((__m256i*)(dst + 32), d0);
        _mm256_storeu_si256((__m256i*)(dst + 48), d1);

    }
    else
    {
        tx_pb16(src, dst, shift, line);
    }

}

static void tx_pb32_avx(s16* src, s16* dst, int shift, int line)
{
    if (line > 4)
    {
        int i, j, idx;
        __m256i s[32];
        __m256i t[16];
        __m256i tab0, tab1, tab2;
        __m256i e[16], o[16], ee[8], eo[8];
        __m256i eee[4], eeo[4];
        __m256i eeee[2], eeeo[2];
        __m256i v[18];
        __m256i d0, d1, d2, d3, d4, d5, d6, d7;
        __m256i dst_reg[8];
        __m256i add = _mm256_set1_epi32(1 << (shift - 1));
        __m256i coeffs[52];



        const __m256i coeff_p32_p32 = _mm256_set_epi32(64, 64, 64, 64, 64, 64, 64, 64);
        const __m256i coeff_p32_n32 = _mm256_set_epi32(-64, 64, -64, 64, -64, 64, -64, 64);
        const __m256i coeff_p42_p17 = _mm256_set_epi32(35, 84, 35, 84, 35, 84, 35, 84);
        const __m256i coeff_p17_n42 = _mm256_set_epi32(-84, 35, -84, 35, -84, 35, -84, 35);

        tab0 = _mm256_loadu_si256((__m256i*)tab_dct2_2nd_shuffle_256i[3]);
        tab1 = _mm256_loadu_si256((__m256i*)tab_dct2_2nd_shuffle_256i[1]);
        tab2 = _mm256_loadu_si256((__m256i*)tab_dct2_2nd_shuffle_256i[2]);

        for (j = 0; j < line; j += 8)
        {
            s[0] = _mm256_loadu_si256((__m256i*)(src));
            s[1] = _mm256_loadu_si256((__m256i*)(src + 16 * 1));
            s[2] = _mm256_loadu_si256((__m256i*)(src + 16 * 2));
            s[3] = _mm256_loadu_si256((__m256i*)(src + 16 * 3));
            s[4] = _mm256_loadu_si256((__m256i*)(src + 16 * 4));
            s[5] = _mm256_loadu_si256((__m256i*)(src + 16 * 5));
            s[6] = _mm256_loadu_si256((__m256i*)(src + 16 * 6));
            s[7] = _mm256_loadu_si256((__m256i*)(src + 16 * 7));
            s[8] = _mm256_loadu_si256((__m256i*)(src + 16 * 8));
            s[9] = _mm256_loadu_si256((__m256i*)(src + 16 * 9));
            s[10] = _mm256_loadu_si256((__m256i*)(src + 16 * 10));
            s[11] = _mm256_loadu_si256((__m256i*)(src + 16 * 11));
            s[12] = _mm256_loadu_si256((__m256i*)(src + 16 * 12));
            s[13] = _mm256_loadu_si256((__m256i*)(src + 16 * 13));
            s[14] = _mm256_loadu_si256((__m256i*)(src + 16 * 14));
            s[15] = _mm256_loadu_si256((__m256i*)(src + 16 * 15));

            t[0] = _mm256_shuffle_epi8(s[1], tab0);
            t[1] = _mm256_shuffle_epi8(s[3], tab0);
            t[2] = _mm256_shuffle_epi8(s[5], tab0);
            t[3] = _mm256_shuffle_epi8(s[7], tab0);
            t[4] = _mm256_shuffle_epi8(s[9], tab0);
            t[5] = _mm256_shuffle_epi8(s[11], tab0);
            t[6] = _mm256_shuffle_epi8(s[13], tab0);
            t[7] = _mm256_shuffle_epi8(s[15], tab0);

            src += 32 * 8;

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
                idx = 2 * i + 1;
                coeffs[i] = _mm256_setr_epi32(xeve_tbl_tm8[idx][0], xeve_tbl_tm8[idx][1], xeve_tbl_tm8[idx][2], xeve_tbl_tm8[idx][3], xeve_tbl_tm8[idx][0], xeve_tbl_tm8[idx][1], xeve_tbl_tm8[idx][2], xeve_tbl_tm8[idx][3]);
            }


            v[0] = _mm256_mullo_epi32(eeee[0], coeff_p32_p32);
            v[1] = _mm256_mullo_epi32(eeee[1], coeff_p32_p32);
            v[2] = _mm256_mullo_epi32(eeee[0], coeff_p32_n32);
            v[3] = _mm256_mullo_epi32(eeee[1], coeff_p32_n32);
            v[4] = _mm256_mullo_epi32(eeeo[0], coeff_p42_p17);
            v[5] = _mm256_mullo_epi32(eeeo[1], coeff_p42_p17);
            v[6] = _mm256_mullo_epi32(eeeo[0], coeff_p17_n42);
            v[7] = _mm256_mullo_epi32(eeeo[1], coeff_p17_n42);

            v[0] = _mm256_hadd_epi32(v[0], v[1]);
            v[2] = _mm256_hadd_epi32(v[2], v[3]);
            v[4] = _mm256_hadd_epi32(v[4], v[5]);
            v[6] = _mm256_hadd_epi32(v[6], v[7]);

            v[0] = _mm256_add_epi32(v[0], add);
            v[2] = _mm256_add_epi32(v[2], add);
            v[4] = _mm256_add_epi32(v[4], add);
            v[6] = _mm256_add_epi32(v[6], add);

            v[0] = _mm256_srai_epi32(v[0], shift);
            v[2] = _mm256_srai_epi32(v[2], shift);
            v[4] = _mm256_srai_epi32(v[4], shift);
            v[6] = _mm256_srai_epi32(v[6], shift);

            d0 = _mm256_packs_epi32(v[0], v[2]);
            d1 = _mm256_packs_epi32(v[4], v[6]);

            d0 = _mm256_permute4x64_epi64(d0, 0xd8);
            d1 = _mm256_permute4x64_epi64(d1, 0xd8);

            _mm_store_si128((__m128i*)dst, _mm256_castsi256_si128(d0));
            _mm_store_si128((__m128i*)(dst + 8 * line), _mm256_castsi256_si128(d1));
            _mm_store_si128((__m128i*)(dst + 16 * line), _mm256_extracti128_si256(d0, 1));
            _mm_store_si128((__m128i*)(dst + 24 * line), _mm256_extracti128_si256(d1, 1));

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

            d0 = _mm256_packs_epi32(v[2], v[3]);
            d1 = _mm256_packs_epi32(v[4], v[5]);

            d0 = _mm256_permute4x64_epi64(d0, 0xd8);
            d1 = _mm256_permute4x64_epi64(d1, 0xd8);

            _mm_store_si128((__m128i*)(dst + 4 * line), _mm256_castsi256_si128(d0));
            _mm_store_si128((__m128i*)(dst + 12 * line), _mm256_extracti128_si256(d0, 1));
            _mm_store_si128((__m128i*)(dst + 20 * line), _mm256_castsi256_si128(d1));
            _mm_store_si128((__m128i*)(dst + 28 * line), _mm256_extracti128_si256(d1, 1));

#define _mm256_madd_epi32(a, b, c, d) \
        _mm256_hadd_epi32(_mm256_mullo_epi32(a, b), _mm256_mullo_epi32(c, d)); \

#define CALCU_EO(coeff0, coeff1, dst) \
            v[0] = _mm256_madd_epi32(eo[0], coeff0, eo[1], coeff1); \
            v[2] = _mm256_madd_epi32(eo[2], coeff0, eo[3], coeff1); \
            v[4] = _mm256_madd_epi32(eo[4], coeff0, eo[5], coeff1); \
            v[6] = _mm256_madd_epi32(eo[6], coeff0, eo[7], coeff1); \
            v[0] = _mm256_hadd_epi32(v[0], v[2]); \
            v[4] = _mm256_hadd_epi32(v[4], v[6]); \
            dst = _mm256_hadd_epi32(v[0], v[4])

            for (i = 0; i < 8; i++)
            {
                idx = 2 * i + 1;
                __m256i tm_0 = _mm256_setr_epi32(xeve_tbl_tm16[idx][0], xeve_tbl_tm16[idx][1], xeve_tbl_tm16[idx][2], xeve_tbl_tm16[idx][3], xeve_tbl_tm16[idx][0], xeve_tbl_tm16[idx][1], xeve_tbl_tm16[idx][2], xeve_tbl_tm16[idx][3]);
                __m256i tm_1 = _mm256_setr_epi32(xeve_tbl_tm16[idx][4], xeve_tbl_tm16[idx][5], xeve_tbl_tm16[idx][6], xeve_tbl_tm16[idx][7], xeve_tbl_tm16[idx][4], xeve_tbl_tm16[idx][5], xeve_tbl_tm16[idx][6], xeve_tbl_tm16[idx][7]);
                CALCU_EO(tm_0, tm_1, dst_reg[i]);
                dst_reg[i] = _mm256_add_epi32(dst_reg[i], add);
                dst_reg[i] = _mm256_srai_epi32(dst_reg[i], shift);

            }


#undef CALCU_EO
#undef _mm256_madd_epi32

            d0 = _mm256_packs_epi32(dst_reg[0], dst_reg[1]);
            d1 = _mm256_packs_epi32(dst_reg[2], dst_reg[3]);
            d2 = _mm256_packs_epi32(dst_reg[4], dst_reg[5]);
            d3 = _mm256_packs_epi32(dst_reg[6], dst_reg[7]);
            d0 = _mm256_permute4x64_epi64(d0, 0xd8);
            d1 = _mm256_permute4x64_epi64(d1, 0xd8);
            d2 = _mm256_permute4x64_epi64(d2, 0xd8);
            d3 = _mm256_permute4x64_epi64(d3, 0xd8);

            _mm_store_si128((__m128i*)(dst + 2 * line), _mm256_castsi256_si128(d0));
            _mm_store_si128((__m128i*)(dst + 6 * line), _mm256_extracti128_si256(d0, 1));
            _mm_store_si128((__m128i*)(dst + 10 * line), _mm256_castsi256_si128(d1));
            _mm_store_si128((__m128i*)(dst + 14 * line), _mm256_extracti128_si256(d1, 1));
            _mm_store_si128((__m128i*)(dst + 18 * line), _mm256_castsi256_si128(d2));
            _mm_store_si128((__m128i*)(dst + 22 * line), _mm256_extracti128_si256(d2, 1));
            _mm_store_si128((__m128i*)(dst + 26 * line), _mm256_castsi256_si128(d3));
            _mm_store_si128((__m128i*)(dst + 30 * line), _mm256_extracti128_si256(d3, 1));


#define _mm256_madd1_epi32(a, b, c, d) \
        _mm256_add_epi32(_mm256_mullo_epi32(a, b), _mm256_mullo_epi32(c, d)); \

#define CALCU_O(coeff0, coeff1, dst) \
            v[0 ] = _mm256_madd1_epi32(o[0], coeff0, o[1], coeff1); \
            v[2 ] = _mm256_madd1_epi32(o[2], coeff0, o[3], coeff1); \
            v[4 ] = _mm256_madd1_epi32(o[4], coeff0, o[5], coeff1); \
            v[6 ] = _mm256_madd1_epi32(o[6], coeff0, o[7], coeff1); \
            v[8 ] = _mm256_madd1_epi32(o[8], coeff0, o[9], coeff1); \
            v[10] = _mm256_madd1_epi32(o[10], coeff0, o[11], coeff1); \
            v[12] = _mm256_madd1_epi32(o[12], coeff0, o[13], coeff1); \
            v[14] = _mm256_madd1_epi32(o[14], coeff0, o[15], coeff1); \
            v[0 ] = _mm256_hadd_epi32(v[0], v[2]); \
            v[4 ] = _mm256_hadd_epi32(v[4], v[6]); \
            v[8 ] = _mm256_hadd_epi32(v[8], v[10]); \
            v[12] = _mm256_hadd_epi32(v[12], v[14]); \
            v[0 ] = _mm256_hadd_epi32(v[0], v[4]); \
            v[8 ] = _mm256_hadd_epi32(v[8], v[12]); \
            v[2 ] = _mm256_permute2x128_si256(v[0], v[8], 0x20); \
            v[3 ] = _mm256_permute2x128_si256(v[0], v[8], 0x31); \
            dst = _mm256_add_epi32(v[2], v[3])

            for (i = 0; i < 8; ++i)
            {
                idx = 2 * i + 1;
                __m256i tm_0 = _mm256_setr_epi32(xeve_tbl_tm32[idx][0], xeve_tbl_tm32[idx][1], xeve_tbl_tm32[idx][2], xeve_tbl_tm32[idx][3], xeve_tbl_tm32[idx][4], xeve_tbl_tm32[idx][5], xeve_tbl_tm32[idx][6], xeve_tbl_tm32[idx][7]);
                __m256i tm_1 = _mm256_setr_epi32(xeve_tbl_tm32[idx][8], xeve_tbl_tm32[idx][9], xeve_tbl_tm32[idx][10], xeve_tbl_tm32[idx][11], xeve_tbl_tm32[idx][12], xeve_tbl_tm32[idx][13], xeve_tbl_tm32[idx][14], xeve_tbl_tm32[idx][15]);
                CALCU_O(tm_0, tm_1, dst_reg[i]);
                dst_reg[i] = _mm256_add_epi32(dst_reg[i], add);
                dst_reg[i] = _mm256_srai_epi32(dst_reg[i], shift);
            }

            d0 = _mm256_packs_epi32(dst_reg[0], dst_reg[1]);
            d1 = _mm256_packs_epi32(dst_reg[2], dst_reg[3]);
            d2 = _mm256_packs_epi32(dst_reg[4], dst_reg[5]);
            d3 = _mm256_packs_epi32(dst_reg[6], dst_reg[7]);
            d0 = _mm256_permute4x64_epi64(d0, 0xd8);
            d1 = _mm256_permute4x64_epi64(d1, 0xd8);
            d2 = _mm256_permute4x64_epi64(d2, 0xd8);
            d3 = _mm256_permute4x64_epi64(d3, 0xd8);

            _mm_store_si128((__m128i*)(dst + line), _mm256_castsi256_si128(d0));
            _mm_store_si128((__m128i*)(dst + 3 * line), _mm256_extracti128_si256(d0, 1));
            _mm_store_si128((__m128i*)(dst + 5 * line), _mm256_castsi256_si128(d1));
            _mm_store_si128((__m128i*)(dst + 7 * line), _mm256_extracti128_si256(d1, 1));
            _mm_store_si128((__m128i*)(dst + 9 * line), _mm256_castsi256_si128(d2));
            _mm_store_si128((__m128i*)(dst + 11 * line), _mm256_extracti128_si256(d2, 1));
            _mm_store_si128((__m128i*)(dst + 13 * line), _mm256_castsi256_si128(d3));
            _mm_store_si128((__m128i*)(dst + 15 * line), _mm256_extracti128_si256(d3, 1));

            for (i = 8; i < 16; ++i)
            {
                idx = 2 * i + 1;
                __m256i tm_0 = _mm256_setr_epi32(xeve_tbl_tm32[idx][0], xeve_tbl_tm32[idx][1], xeve_tbl_tm32[idx][2], xeve_tbl_tm32[idx][3], xeve_tbl_tm32[idx][4], xeve_tbl_tm32[idx][5], xeve_tbl_tm32[idx][6], xeve_tbl_tm32[idx][7]);
                __m256i tm_1 = _mm256_setr_epi32(xeve_tbl_tm32[idx][8], xeve_tbl_tm32[idx][9], xeve_tbl_tm32[idx][10], xeve_tbl_tm32[idx][11], xeve_tbl_tm32[idx][12], xeve_tbl_tm32[idx][13], xeve_tbl_tm32[idx][14], xeve_tbl_tm32[idx][15]);
                CALCU_O(tm_0, tm_1, dst_reg[i - 8]);
                dst_reg[i - 8] = _mm256_add_epi32(dst_reg[i - 8], add);
                dst_reg[i - 8] = _mm256_srai_epi32(dst_reg[i - 8], shift);
            }


#undef CALCU_O
#undef _mm256_madd1_epi32

            d0 = _mm256_packs_epi32(dst_reg[0], dst_reg[1]);
            d1 = _mm256_packs_epi32(dst_reg[2], dst_reg[3]);
            d2 = _mm256_packs_epi32(dst_reg[4], dst_reg[5]);
            d3 = _mm256_packs_epi32(dst_reg[6], dst_reg[7]);
            d0 = _mm256_permute4x64_epi64(d0, 0xd8);
            d1 = _mm256_permute4x64_epi64(d1, 0xd8);
            d2 = _mm256_permute4x64_epi64(d2, 0xd8);
            d3 = _mm256_permute4x64_epi64(d3, 0xd8);

            _mm_store_si128((__m128i*)(dst + 17 * line), _mm256_castsi256_si128(d0));
            _mm_store_si128((__m128i*)(dst + 19 * line), _mm256_extracti128_si256(d0, 1));
            _mm_store_si128((__m128i*)(dst + 21 * line), _mm256_castsi256_si128(d1));
            _mm_store_si128((__m128i*)(dst + 23 * line), _mm256_extracti128_si256(d1, 1));
            _mm_store_si128((__m128i*)(dst + 25 * line), _mm256_castsi256_si128(d2));
            _mm_store_si128((__m128i*)(dst + 27 * line), _mm256_extracti128_si256(d2, 1));
            _mm_store_si128((__m128i*)(dst + 29 * line), _mm256_castsi256_si128(d3));
            _mm_store_si128((__m128i*)(dst + 31 * line), _mm256_extracti128_si256(d3, 1));

            dst += 8;
        }
    }
    else if (line == 4) {

        int i, idx;
        __m256i s[16];
        __m256i t[8];
        __m256i tab0, tab1, tab2;
        __m256i e[8], o[8], ee[4], eo[4];
        __m256i eee[2], eeo[2];
        __m256i eeee, eeeo;
        __m256i v[8];
        __m256i d0, d1, d2, d3, d4, d5, d6, d7;
        __m256i dst_reg[8];
        __m256i add = _mm256_set1_epi32(1 << (shift - 1));
        __m256i coeffs[52];
        __m128i m0, m1, m2, m3, m4, m5, m6, m7;
        const __m256i coeff_p32_p32 = _mm256_set_epi32(64, 64, 64, 64, 64, 64, 64, 64);
        const __m256i coeff_p32_n32 = _mm256_set_epi32(-64, 64, -64, 64, -64, 64, -64, 64);
        const __m256i coeff_p42_p17 = _mm256_set_epi32(35, 84, 35, 84, 35, 84, 35, 84);
        const __m256i coeff_p17_n42 = _mm256_set_epi32(-84, 35, -84, 35, -84, 35, -84, 35);

        tab0 = _mm256_loadu_si256((__m256i*)tab_dct2_2nd_shuffle_256i[3]);
        tab1 = _mm256_loadu_si256((__m256i*)tab_dct2_2nd_shuffle_256i[1]);
        tab2 = _mm256_loadu_si256((__m256i*)tab_dct2_2nd_shuffle_256i[2]);

        s[0] = _mm256_loadu_si256((__m256i*)(src));
        s[1] = _mm256_loadu_si256((__m256i*)(src + 16 * 1));
        s[2] = _mm256_loadu_si256((__m256i*)(src + 16 * 2));
        s[3] = _mm256_loadu_si256((__m256i*)(src + 16 * 3));
        s[4] = _mm256_loadu_si256((__m256i*)(src + 16 * 4));
        s[5] = _mm256_loadu_si256((__m256i*)(src + 16 * 5));
        s[6] = _mm256_loadu_si256((__m256i*)(src + 16 * 6));
        s[7] = _mm256_loadu_si256((__m256i*)(src + 16 * 7));

        t[0] = _mm256_shuffle_epi8(s[1], tab0);
        t[1] = _mm256_shuffle_epi8(s[3], tab0);
        t[2] = _mm256_shuffle_epi8(s[5], tab0);
        t[3] = _mm256_shuffle_epi8(s[7], tab0);

        s[1] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[0], 1));
        s[0] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[0]));
        s[3] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[2], 1));
        s[2] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[2]));
        s[5] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[4], 1));
        s[4] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[4]));
        s[7] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(s[6], 1));
        s[6] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(s[6]));

        s[8] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[0], 1));
        s[9] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[0]));
        s[10] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[1], 1));
        s[11] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[1]));
        s[12] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[2], 1));
        s[13] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[2]));
        s[14] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[3], 1));
        s[15] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[3]));

        e[0] = _mm256_add_epi32(s[0], s[8]);
        o[0] = _mm256_sub_epi32(s[0], s[8]);
        e[1] = _mm256_add_epi32(s[1], s[9]);
        o[1] = _mm256_sub_epi32(s[1], s[9]);
        e[2] = _mm256_add_epi32(s[2], s[10]);
        o[2] = _mm256_sub_epi32(s[2], s[10]);
        e[3] = _mm256_add_epi32(s[3], s[11]);
        o[3] = _mm256_sub_epi32(s[3], s[11]);
        e[4] = _mm256_add_epi32(s[4], s[12]);
        o[4] = _mm256_sub_epi32(s[4], s[12]);
        e[5] = _mm256_add_epi32(s[5], s[13]);
        o[5] = _mm256_sub_epi32(s[5], s[13]);
        e[6] = _mm256_add_epi32(s[6], s[14]);
        o[6] = _mm256_sub_epi32(s[6], s[14]);
        e[7] = _mm256_add_epi32(s[7], s[15]);
        o[7] = _mm256_sub_epi32(s[7], s[15]);

        t[0] = _mm256_permute2x128_si256(e[0], e[4], 0x20);
        t[1] = _mm256_permute2x128_si256(e[0], e[4], 0x31);
        t[2] = _mm256_permute2x128_si256(e[1], e[5], 0x20);
        t[3] = _mm256_permute2x128_si256(e[1], e[5], 0x31);
        t[4] = _mm256_permute2x128_si256(e[2], e[6], 0x20);
        t[5] = _mm256_permute2x128_si256(e[2], e[6], 0x31);
        t[6] = _mm256_permute2x128_si256(e[3], e[7], 0x20);
        t[7] = _mm256_permute2x128_si256(e[3], e[7], 0x31);

        t[2] = _mm256_shuffle_epi8(t[2], tab1);
        t[3] = _mm256_shuffle_epi8(t[3], tab1);
        t[6] = _mm256_shuffle_epi8(t[6], tab1);
        t[7] = _mm256_shuffle_epi8(t[7], tab1);

        ee[0] = _mm256_add_epi32(t[0], t[3]);
        eo[0] = _mm256_sub_epi32(t[0], t[3]);
        ee[1] = _mm256_add_epi32(t[1], t[2]);
        eo[1] = _mm256_sub_epi32(t[1], t[2]);
        ee[2] = _mm256_add_epi32(t[4], t[7]);
        eo[2] = _mm256_sub_epi32(t[4], t[7]);
        ee[3] = _mm256_add_epi32(t[5], t[6]);
        eo[3] = _mm256_sub_epi32(t[5], t[6]);

        ee[1] = _mm256_shuffle_epi8(ee[1], tab1);
        ee[3] = _mm256_shuffle_epi8(ee[3], tab1);

        eee[0] = _mm256_add_epi32(ee[0], ee[1]);
        eeo[0] = _mm256_sub_epi32(ee[0], ee[1]);
        eee[1] = _mm256_add_epi32(ee[2], ee[3]);
        eeo[1] = _mm256_sub_epi32(ee[2], ee[3]);

        eee[0] = _mm256_shuffle_epi8(eee[0], tab2);
        eee[1] = _mm256_shuffle_epi8(eee[1], tab2);

        eeee = _mm256_hadd_epi32(eee[0], eee[1]);
        eeeo = _mm256_hsub_epi32(eee[0], eee[1]);

        v[0] = _mm256_mullo_epi32(eeee, coeff_p32_p32);
        v[1] = _mm256_mullo_epi32(eeee, coeff_p32_n32);
        v[2] = _mm256_mullo_epi32(eeeo, coeff_p42_p17);
        v[3] = _mm256_mullo_epi32(eeeo, coeff_p17_n42);

        v[0] = _mm256_hadd_epi32(v[0], v[1]);
        v[2] = _mm256_hadd_epi32(v[2], v[3]);

        v[0] = _mm256_permute4x64_epi64(v[0], 0xd8);
        v[2] = _mm256_permute4x64_epi64(v[2], 0xd8);

        v[0] = _mm256_add_epi32(v[0], add);
        v[2] = _mm256_add_epi32(v[2], add);

        v[0] = _mm256_srai_epi32(v[0], shift);
        v[2] = _mm256_srai_epi32(v[2], shift);

        d0 = _mm256_packs_epi32(v[0], v[2]);

        for (i = 0; i < 4; ++i)
        {
            idx = 2 * i + 1;
            coeffs[i] = _mm256_setr_epi32(xeve_tbl_tm8[idx][0], xeve_tbl_tm8[idx][1], xeve_tbl_tm8[idx][2], xeve_tbl_tm8[idx][3], xeve_tbl_tm8[idx][0], xeve_tbl_tm8[idx][1], xeve_tbl_tm8[idx][2], xeve_tbl_tm8[idx][3]);
        }

        m0 = _mm256_castsi256_si128(d0);
        m1 = _mm256_extracti128_si256(d0, 1);
        m2 = _mm_srli_si128(m0, 8);
        m3 = _mm_srli_si128(m1, 8);
        _mm_storel_epi64((__m128i*)(dst), m0);
        _mm_storel_epi64((__m128i*)(dst + 8 * line), m2);
        _mm_storel_epi64((__m128i*)(dst + 16 * line), m1);
        _mm_storel_epi64((__m128i*)(dst + 24 * line), m3);

        v[0] = _mm256_mullo_epi32(eeo[0], coeffs[0]);
        v[1] = _mm256_mullo_epi32(eeo[1], coeffs[0]);
        v[2] = _mm256_mullo_epi32(eeo[0], coeffs[1]);
        v[3] = _mm256_mullo_epi32(eeo[1], coeffs[1]);

        v[0] = _mm256_hadd_epi32(v[0], v[1]);
        v[2] = _mm256_hadd_epi32(v[2], v[3]);
        v[4] = _mm256_hadd_epi32(v[0], v[2]);

        v[0] = _mm256_mullo_epi32(eeo[0], coeffs[2]);
        v[1] = _mm256_mullo_epi32(eeo[1], coeffs[2]);
        v[2] = _mm256_mullo_epi32(eeo[0], coeffs[3]);
        v[3] = _mm256_mullo_epi32(eeo[1], coeffs[3]);

        v[0] = _mm256_hadd_epi32(v[0], v[1]);
        v[2] = _mm256_hadd_epi32(v[2], v[3]);
        v[0] = _mm256_hadd_epi32(v[0], v[2]);

        v[2] = _mm256_add_epi32(v[4], add);
        v[3] = _mm256_add_epi32(v[0], add);

        v[0] = _mm256_srai_epi32(v[2], shift);
        v[1] = _mm256_srai_epi32(v[3], shift);

        v[0] = _mm256_permute4x64_epi64(v[0], 0xd8);
        v[1] = _mm256_permute4x64_epi64(v[1], 0xd8);

        d0 = _mm256_packs_epi32(v[0], v[1]);

        m0 = _mm256_castsi256_si128(d0);
        m1 = _mm256_extracti128_si256(d0, 1);
        m2 = _mm_srli_si128(m0, 8);
        m3 = _mm_srli_si128(m1, 8);

        _mm_storel_epi64((__m128i*)(dst + 4 * line), m0);
        _mm_storel_epi64((__m128i*)(dst + 12 * line), m1);
        _mm_storel_epi64((__m128i*)(dst + 20 * line), m2);
        _mm_storel_epi64((__m128i*)(dst + 28 * line), m3);

#define _mm256_madd_epi32(a, b, c, d) \
        _mm256_hadd_epi32(_mm256_mullo_epi32(a, b), _mm256_mullo_epi32(c, d)); \

#define CALCU_EO(coeff0, coeff1, dst) \
        v[0] = _mm256_madd_epi32(eo[0], coeff0, eo[1], coeff1); \
        v[2] = _mm256_madd_epi32(eo[2], coeff0, eo[3], coeff1); \
        dst = _mm256_hadd_epi32(v[0], v[2])

        for (i = 0; i < 8; i++)
        {
            idx = 2 * i + 1;
            __m256i tm_0 = _mm256_setr_epi32(xeve_tbl_tm16[idx][0], xeve_tbl_tm16[idx][1], xeve_tbl_tm16[idx][2], xeve_tbl_tm16[idx][3], xeve_tbl_tm16[idx][0], xeve_tbl_tm16[idx][1], xeve_tbl_tm16[idx][2], xeve_tbl_tm16[idx][3]);
            __m256i tm_1 = _mm256_setr_epi32(xeve_tbl_tm16[idx][4], xeve_tbl_tm16[idx][5], xeve_tbl_tm16[idx][6], xeve_tbl_tm16[idx][7], xeve_tbl_tm16[idx][4], xeve_tbl_tm16[idx][5], xeve_tbl_tm16[idx][6], xeve_tbl_tm16[idx][7]);
            CALCU_EO(tm_0, tm_1, dst_reg[i]);

        }

        d0 = _mm256_hadd_epi32(dst_reg[0], dst_reg[1]);
        d2 = _mm256_hadd_epi32(dst_reg[2], dst_reg[3]);
        d4 = _mm256_hadd_epi32(dst_reg[4], dst_reg[5]);
        d6 = _mm256_hadd_epi32(dst_reg[6], dst_reg[7]);

        d0 = _mm256_permute4x64_epi64(d0, 0xd8);
        d1 = _mm256_permute4x64_epi64(d2, 0xd8);
        d2 = _mm256_permute4x64_epi64(d4, 0xd8);
        d3 = _mm256_permute4x64_epi64(d6, 0xd8);

#undef CALCU_EO
#undef _mm256_madd_epi32

        d0 = _mm256_add_epi32(d0, add);
        d1 = _mm256_add_epi32(d1, add);
        d2 = _mm256_add_epi32(d2, add);
        d3 = _mm256_add_epi32(d3, add);

        d0 = _mm256_srai_epi32(d0, shift);
        d1 = _mm256_srai_epi32(d1, shift);
        d2 = _mm256_srai_epi32(d2, shift);
        d3 = _mm256_srai_epi32(d3, shift);

        d0 = _mm256_packs_epi32(d0, d1);
        d1 = _mm256_packs_epi32(d2, d3);

        m0 = _mm256_castsi256_si128(d0);
        m1 = _mm256_extracti128_si256(d0, 1);
        m2 = _mm_srli_si128(m0, 8);
        m3 = _mm_srli_si128(m1, 8);
        m4 = _mm256_castsi256_si128(d1);
        m5 = _mm256_extracti128_si256(d1, 1);
        m6 = _mm_srli_si128(m4, 8);
        m7 = _mm_srli_si128(m5, 8);

        _mm_storel_epi64((__m128i*)(dst + 2 * line), m0);
        _mm_storel_epi64((__m128i*)(dst + 6 * line), m1);
        _mm_storel_epi64((__m128i*)(dst + 10 * line), m2);
        _mm_storel_epi64((__m128i*)(dst + 14 * line), m3);
        _mm_storel_epi64((__m128i*)(dst + 18 * line), m4);
        _mm_storel_epi64((__m128i*)(dst + 22 * line), m5);
        _mm_storel_epi64((__m128i*)(dst + 26 * line), m6);
        _mm_storel_epi64((__m128i*)(dst + 30 * line), m7);

#define _mm256_madd1_epi32(a, b, c, d) \
        _mm256_add_epi32(_mm256_mullo_epi32(a, b), _mm256_mullo_epi32(c, d)); \

#define CALCU_O(coeff0, coeff1, dst) \
        v[0] = _mm256_madd1_epi32(o[0], coeff0, o[1], coeff1); \
        v[2] = _mm256_madd1_epi32(o[2], coeff0, o[3], coeff1); \
        v[4] = _mm256_madd1_epi32(o[4], coeff0, o[5], coeff1); \
        v[6] = _mm256_madd1_epi32(o[6], coeff0, o[7], coeff1); \
        v[0] = _mm256_hadd_epi32(v[0], v[2]); \
        v[4] = _mm256_hadd_epi32(v[4], v[6]); \
        dst = _mm256_hadd_epi32(v[0], v[4]);                        

        for (i = 0; i < 8; ++i)
        {
            idx = 2 * i + 1;
            __m256i tm_0 = _mm256_setr_epi32(xeve_tbl_tm32[idx][0], xeve_tbl_tm32[idx][1], xeve_tbl_tm32[idx][2], xeve_tbl_tm32[idx][3], xeve_tbl_tm32[idx][4], xeve_tbl_tm32[idx][5], xeve_tbl_tm32[idx][6], xeve_tbl_tm32[idx][7]);
            __m256i tm_1 = _mm256_setr_epi32(xeve_tbl_tm32[idx][8], xeve_tbl_tm32[idx][9], xeve_tbl_tm32[idx][10], xeve_tbl_tm32[idx][11], xeve_tbl_tm32[idx][12], xeve_tbl_tm32[idx][13], xeve_tbl_tm32[idx][14], xeve_tbl_tm32[idx][15]);
            CALCU_O(tm_0, tm_1, dst_reg[i]);
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

        d0 = _mm256_add_epi32(d0, add);
        d1 = _mm256_add_epi32(d1, add);
        d2 = _mm256_add_epi32(d2, add);
        d3 = _mm256_add_epi32(d3, add);

        d0 = _mm256_srai_epi32(d0, shift);
        d1 = _mm256_srai_epi32(d1, shift);
        d2 = _mm256_srai_epi32(d2, shift);
        d3 = _mm256_srai_epi32(d3, shift);

        d0 = _mm256_packs_epi32(d0, d1);
        d1 = _mm256_packs_epi32(d2, d3);

        m0 = _mm256_castsi256_si128(d0);
        m1 = _mm256_extracti128_si256(d0, 1);
        m2 = _mm_srli_si128(m0, 8);
        m3 = _mm_srli_si128(m1, 8);
        m4 = _mm256_castsi256_si128(d1);
        m5 = _mm256_extracti128_si256(d1, 1);
        m6 = _mm_srli_si128(m4, 8);
        m7 = _mm_srli_si128(m5, 8);

        _mm_storel_epi64((__m128i*)(dst + 1 * line), m0);
        _mm_storel_epi64((__m128i*)(dst + 3 * line), m1);
        _mm_storel_epi64((__m128i*)(dst + 5 * line), m2);
        _mm_storel_epi64((__m128i*)(dst + 7 * line), m3);
        _mm_storel_epi64((__m128i*)(dst + 9 * line), m4);
        _mm_storel_epi64((__m128i*)(dst + 11 * line), m5);
        _mm_storel_epi64((__m128i*)(dst + 13 * line), m6);
        _mm_storel_epi64((__m128i*)(dst + 15 * line), m7);

        for (i = 8; i < 16; ++i)
        {
            idx = 2 * i + 1;
            __m256i tm_0 = _mm256_setr_epi32(xeve_tbl_tm32[idx][0], xeve_tbl_tm32[idx][1], xeve_tbl_tm32[idx][2], xeve_tbl_tm32[idx][3], xeve_tbl_tm32[idx][4], xeve_tbl_tm32[idx][5], xeve_tbl_tm32[idx][6], xeve_tbl_tm32[idx][7]);
            __m256i tm_1 = _mm256_setr_epi32(xeve_tbl_tm32[idx][8], xeve_tbl_tm32[idx][9], xeve_tbl_tm32[idx][10], xeve_tbl_tm32[idx][11], xeve_tbl_tm32[idx][12], xeve_tbl_tm32[idx][13], xeve_tbl_tm32[idx][14], xeve_tbl_tm32[idx][15]);
            CALCU_O(tm_0, tm_1, dst_reg[i - 8]);
        }

#undef CALCU_O
#undef _mm256_madd1_epi32

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

        d0 = _mm256_add_epi32(d0, add);
        d1 = _mm256_add_epi32(d1, add);
        d2 = _mm256_add_epi32(d2, add);
        d3 = _mm256_add_epi32(d3, add);

        d0 = _mm256_srai_epi32(d0, shift);
        d1 = _mm256_srai_epi32(d1, shift);
        d2 = _mm256_srai_epi32(d2, shift);
        d3 = _mm256_srai_epi32(d3, shift);

        d0 = _mm256_packs_epi32(d0, d1);
        d1 = _mm256_packs_epi32(d2, d3);

        m0 = _mm256_castsi256_si128(d0);
        m1 = _mm256_extracti128_si256(d0, 1);
        m2 = _mm_srli_si128(m0, 8);
        m3 = _mm_srli_si128(m1, 8);
        m4 = _mm256_castsi256_si128(d1);
        m5 = _mm256_extracti128_si256(d1, 1);
        m6 = _mm_srli_si128(m4, 8);
        m7 = _mm_srli_si128(m5, 8);

        _mm_storel_epi64((__m128i*)(dst + 17 * line), m0);
        _mm_storel_epi64((__m128i*)(dst + 19 * line), m1);
        _mm_storel_epi64((__m128i*)(dst + 21 * line), m2);
        _mm_storel_epi64((__m128i*)(dst + 23 * line), m3);
        _mm_storel_epi64((__m128i*)(dst + 25 * line), m4);
        _mm_storel_epi64((__m128i*)(dst + 27 * line), m5);
        _mm_storel_epi64((__m128i*)(dst + 29 * line), m6);
        _mm_storel_epi64((__m128i*)(dst + 31 * line), m7);
    }
    else
    {
        tx_pb32(src, dst, shift, line);
    }
}

static void tx_pb64_avx(s16* src, s16* dst, int shift, int line)
{
    xeve_mset_16b(dst, 0, MAX_TR_DIM);
    if (line % 4 == 0)
    {
        int i, j;
        __m256i inp[32];
        __m256i t[16];
        __m256i tab0, tab1, tab2;
        __m256i e[16], o[16], ee[8], eo[8];
        __m256i eee[4], eeo[4];
        __m256i eeee[2], eeeo[2], eeeee, eeeeo;
        __m256i v[16];
        __m256i d0, d1, d2, d3, d4, d5, d6, d7;
        __m256i add = _mm256_set1_epi32(1 << (shift - 1));
        __m256i coeffs[106];
        __m128i m0, m1, m2, m3, m4, m5, m6, m7;
        __m256i dst_reg[8];

        const __m256i coeff_p32_p32 = _mm256_set_epi32(64, 64, 64, 64, 64, 64, 64, 64);
        const __m256i coeff_p42_p17 = _mm256_set_epi32(35, 84, 35, 84, 35, 84, 35, 84);

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
            for (i = 0; i < 16; ++i)
            {
                inp[i] = _mm256_loadu_si256((__m256i*)(src + 16 * i));
            }
            t[0] = _mm256_shuffle_epi8(inp[2], tab0);
            t[1] = _mm256_shuffle_epi8(inp[3], tab0);
            t[2] = _mm256_shuffle_epi8(inp[6], tab0);
            t[3] = _mm256_shuffle_epi8(inp[7], tab0);
            t[4] = _mm256_shuffle_epi8(inp[10], tab0);
            t[5] = _mm256_shuffle_epi8(inp[11], tab0);
            t[6] = _mm256_shuffle_epi8(inp[14], tab0);
            t[7] = _mm256_shuffle_epi8(inp[15], tab0);

            inp[3] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(inp[1], 1));
            inp[2] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(inp[1]));
            inp[1] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(inp[0], 1));
            inp[0] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(inp[0]));
            inp[7] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(inp[5], 1));
            inp[6] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(inp[5]));
            inp[5] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(inp[4], 1));
            inp[4] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(inp[4]));
            inp[11] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(inp[9], 1));
            inp[10] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(inp[9]));
            inp[9] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(inp[8], 1));
            inp[8] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(inp[8]));
            inp[15] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(inp[13], 1));
            inp[14] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(inp[13]));
            inp[13] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(inp[12], 1));
            inp[12] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(inp[12]));

            inp[18] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[0], 1));
            inp[19] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[0]));
            inp[16] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[1], 1));
            inp[17] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[1]));
            inp[22] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[2], 1));
            inp[23] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[2]));
            inp[20] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[3], 1));
            inp[21] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[3]));
            inp[26] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[4], 1));
            inp[27] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[4]));
            inp[24] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[5], 1));
            inp[25] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[5]));
            inp[30] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[6], 1));
            inp[31] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[6]));
            inp[28] = _mm256_cvtepi16_epi32(_mm256_extracti128_si256(t[7], 1));
            inp[29] = _mm256_cvtepi16_epi32(_mm256_castsi256_si128(t[7]));

            src += 64 * 4;

            for (i = 0; i < 16; i++) {
                e[i] = _mm256_add_epi32(inp[i], inp[16 + i]);
                o[i] = _mm256_sub_epi32(inp[i], inp[16 + i]);
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

            v[0] = _mm256_mullo_epi32(eeeee, coeff_p32_p32);
            v[1] = _mm256_mullo_epi32(eeeeo, coeff_p42_p17);

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

            _mm_storel_epi64((__m128i*)(dst), m0);
            _mm_storel_epi64((__m128i*)(dst + 8 * line), m2);
            _mm_storel_epi64((__m128i*)(dst + 16 * line), m1);
            _mm_storel_epi64((__m128i*)(dst + 24 * line), m3);

#define _mm256_madd_epi32(a, b, c, d) \
        _mm256_hadd_epi32(_mm256_mullo_epi32(a, b), _mm256_mullo_epi32(c, d)); \


#define CALCU_EEO(coeff0, coeff1, dst) \
        v[0] = _mm256_madd_epi32(eeo[0], coeff0, eeo[1], coeff1); \
        v[2] = _mm256_madd_epi32(eeo[2], coeff0, eeo[3], coeff1); \
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
#undef _mm256_madd_epi32

            d0 = _mm256_add_epi32(d0, add);
            d1 = _mm256_add_epi32(d1, add);

            d0 = _mm256_srai_epi32(d0, shift);
            d1 = _mm256_srai_epi32(d1, shift);

            d0 = _mm256_packs_epi32(d0, d1);

            m0 = _mm256_castsi256_si128(d0);
            m1 = _mm256_extracti128_si256(d0, 1);
            m2 = _mm_srli_si128(m0, 8);
            m3 = _mm_srli_si128(m1, 8);

            _mm_storel_epi64((__m128i*)(dst + 4 * line), m0);
            _mm_storel_epi64((__m128i*)(dst + 12 * line), m1);
            _mm_storel_epi64((__m128i*)(dst + 20 * line), m2);
            _mm_storel_epi64((__m128i*)(dst + 28 * line), m3);

#define _mm256_madd1_epi32(a, b, c, d) \
        _mm256_add_epi32(_mm256_mullo_epi32(a, b), _mm256_mullo_epi32(c, d)); \

#define CALCU_EO(coeff0, coeff1, coeff2, coeff3, dst) \
        v[0] = _mm256_madd1_epi32(eo[0], coeff0, eo[1], coeff1); \
        v[2] = _mm256_madd1_epi32(eo[2], coeff2, eo[3], coeff3); \
        v[4] = _mm256_madd1_epi32(eo[4], coeff0, eo[5], coeff1); \
        v[6] = _mm256_madd1_epi32(eo[6], coeff2, eo[7], coeff3); \
        v[0] = _mm256_add_epi32(v[0], v[2]); \
        v[4] = _mm256_add_epi32(v[4], v[6]); \
        dst = _mm256_hadd_epi32(v[0], v[4]);

            for (i = 0; i < 8; ++i)
            {
                __m256i tm_0 = _mm256_setr_epi32(xeve_tbl_tm32[i * 2 + 1][0], xeve_tbl_tm32[i * 2 + 1][1], xeve_tbl_tm32[i * 2 + 1][2], xeve_tbl_tm32[i * 2 + 1][3], xeve_tbl_tm32[i * 2 + 1][0], xeve_tbl_tm32[i * 2 + 1][1], xeve_tbl_tm32[i * 2 + 1][2], xeve_tbl_tm32[i * 2 + 1][3]);
                __m256i tm_1 = _mm256_setr_epi32(xeve_tbl_tm32[i * 2 + 1][4], xeve_tbl_tm32[i * 2 + 1][5], xeve_tbl_tm32[i * 2 + 1][6], xeve_tbl_tm32[i * 2 + 1][7], xeve_tbl_tm32[i * 2 + 1][4], xeve_tbl_tm32[i * 2 + 1][5], xeve_tbl_tm32[i * 2 + 1][6], xeve_tbl_tm32[i * 2 + 1][7]);
                __m256i tm_2 = _mm256_setr_epi32(xeve_tbl_tm32[i * 2 + 1][8], xeve_tbl_tm32[i * 2 + 1][9], xeve_tbl_tm32[i * 2 + 1][10], xeve_tbl_tm32[i * 2 + 1][11], xeve_tbl_tm32[i * 2 + 1][8], xeve_tbl_tm32[i * 2 + 1][9], xeve_tbl_tm32[i * 2 + 1][10], xeve_tbl_tm32[i * 2 + 1][11]);
                __m256i tm_3 = _mm256_setr_epi32(xeve_tbl_tm32[i * 2 + 1][12], xeve_tbl_tm32[i * 2 + 1][13], xeve_tbl_tm32[i * 2 + 1][14], xeve_tbl_tm32[i * 2 + 1][15], xeve_tbl_tm32[i * 2 + 1][12], xeve_tbl_tm32[i * 2 + 1][13], xeve_tbl_tm32[i * 2 + 1][14], xeve_tbl_tm32[i * 2 + 1][15]);
                CALCU_EO(tm_0, tm_1, tm_2, tm_3, dst_reg[i]);
            }

#undef CALCU_EO
            d0 = _mm256_hadd_epi32(dst_reg[0], dst_reg[1]);
            d1 = _mm256_hadd_epi32(dst_reg[2], dst_reg[3]);
            d2 = _mm256_hadd_epi32(dst_reg[4], dst_reg[5]);
            d3 = _mm256_hadd_epi32(dst_reg[6], dst_reg[7]);

            d0 = _mm256_permute4x64_epi64(d0, 0xd8);
            d1 = _mm256_permute4x64_epi64(d1, 0xd8);
            d2 = _mm256_permute4x64_epi64(d2, 0xd8);
            d3 = _mm256_permute4x64_epi64(d3, 0xd8);

            d0 = _mm256_add_epi32(d0, add);
            d1 = _mm256_add_epi32(d1, add);
            d2 = _mm256_add_epi32(d2, add);
            d3 = _mm256_add_epi32(d3, add);

            d0 = _mm256_srai_epi32(d0, shift);
            d1 = _mm256_srai_epi32(d1, shift);
            d2 = _mm256_srai_epi32(d2, shift);
            d3 = _mm256_srai_epi32(d3, shift);

            d0 = _mm256_packs_epi32(d0, d1);
            d1 = _mm256_packs_epi32(d2, d3);

            m0 = _mm256_castsi256_si128(d0);
            m1 = _mm256_extracti128_si256(d0, 1);
            m2 = _mm_srli_si128(m0, 8);
            m3 = _mm_srli_si128(m1, 8);
            m4 = _mm256_castsi256_si128(d1);
            m5 = _mm256_extracti128_si256(d1, 1);
            m6 = _mm_srli_si128(m4, 8);
            m7 = _mm_srli_si128(m5, 8);

            _mm_storel_epi64((__m128i*)(dst + 2 * line), m0);
            _mm_storel_epi64((__m128i*)(dst + 6 * line), m1);
            _mm_storel_epi64((__m128i*)(dst + 10 * line), m2);
            _mm_storel_epi64((__m128i*)(dst + 14 * line), m3);
            _mm_storel_epi64((__m128i*)(dst + 18 * line), m4);
            _mm_storel_epi64((__m128i*)(dst + 22 * line), m5);
            _mm_storel_epi64((__m128i*)(dst + 26 * line), m6);
            _mm_storel_epi64((__m128i*)(dst + 30 * line), m7);

#define CALCU_O(coeff0, coeff1, coeff2, coeff3, d) \
        v[0 ] = _mm256_madd1_epi32(o[0], coeff0, o[1], coeff1);      \
        v[2 ] = _mm256_madd1_epi32(o[2], coeff2, o[3], coeff3);      \
        v[4 ] = _mm256_madd1_epi32(o[4], coeff0, o[5], coeff1);      \
        v[6 ] = _mm256_madd1_epi32(o[6], coeff2, o[7], coeff3);      \
        v[8 ] = _mm256_madd1_epi32(o[8], coeff0, o[9], coeff1);     \
        v[10] = _mm256_madd1_epi32(o[10], coeff2, o[11], coeff3);    \
        v[12] = _mm256_madd1_epi32(o[12], coeff0, o[13], coeff1);    \
        v[14] = _mm256_madd1_epi32(o[14], coeff2, o[15], coeff3);    \
        v[0 ] = _mm256_add_epi32(v[0], v[2]);      \
        v[1 ] = _mm256_add_epi32(v[4], v[6]);      \
        v[2 ] = _mm256_add_epi32(v[8], v[10]);     \
        v[3 ] = _mm256_add_epi32(v[12], v[14]);    \
        v[0 ] = _mm256_hadd_epi32(v[0], v[1]);     \
        v[2 ] = _mm256_hadd_epi32(v[2], v[3]);     \
        d = _mm256_hadd_epi32(v[0], v[2]);                        


            for (i = 0; i < 8; ++i)
            {
                __m256i tm_0 = _mm256_setr_epi32(xeve_tbl_tm64[i * 2 + 1][0], xeve_tbl_tm64[i * 2 + 1][1], xeve_tbl_tm64[i * 2 + 1][2], xeve_tbl_tm64[i * 2 + 1][3], xeve_tbl_tm64[i * 2 + 1][4], xeve_tbl_tm64[i * 2 + 1][5], xeve_tbl_tm64[i * 2 + 1][6], xeve_tbl_tm64[i * 2 + 1][7]);
                __m256i tm_1 = _mm256_setr_epi32(xeve_tbl_tm64[i * 2 + 1][8], xeve_tbl_tm64[i * 2 + 1][9], xeve_tbl_tm64[i * 2 + 1][10], xeve_tbl_tm64[i * 2 + 1][11], xeve_tbl_tm64[i * 2 + 1][12], xeve_tbl_tm64[i * 2 + 1][13], xeve_tbl_tm64[i * 2 + 1][14], xeve_tbl_tm64[i * 2 + 1][15]);
                __m256i tm_2 = _mm256_setr_epi32(xeve_tbl_tm64[i * 2 + 1][16], xeve_tbl_tm64[i * 2 + 1][17], xeve_tbl_tm64[i * 2 + 1][18], xeve_tbl_tm64[i * 2 + 1][19], xeve_tbl_tm64[i * 2 + 1][20], xeve_tbl_tm64[i * 2 + 1][21], xeve_tbl_tm64[i * 2 + 1][22], xeve_tbl_tm64[i * 2 + 1][23]);
                __m256i tm_3 = _mm256_setr_epi32(xeve_tbl_tm64[i * 2 + 1][24], xeve_tbl_tm64[i * 2 + 1][25], xeve_tbl_tm64[i * 2 + 1][26], xeve_tbl_tm64[i * 2 + 1][27], xeve_tbl_tm64[i * 2 + 1][28], xeve_tbl_tm64[i * 2 + 1][29], xeve_tbl_tm64[i * 2 + 1][30], xeve_tbl_tm64[i * 2 + 1][31]);
                CALCU_O(tm_0, tm_1, tm_2, tm_3, dst_reg[i]);
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

            d0 = _mm256_add_epi32(d0, add);
            d1 = _mm256_add_epi32(d1, add);
            d2 = _mm256_add_epi32(d2, add);
            d3 = _mm256_add_epi32(d3, add);

            d0 = _mm256_srai_epi32(d0, shift);
            d1 = _mm256_srai_epi32(d1, shift);
            d2 = _mm256_srai_epi32(d2, shift);
            d3 = _mm256_srai_epi32(d3, shift);

            d0 = _mm256_packs_epi32(d0, d1);
            d1 = _mm256_packs_epi32(d2, d3);

            m0 = _mm256_castsi256_si128(d0);
            m1 = _mm256_extracti128_si256(d0, 1);
            m2 = _mm_srli_si128(m0, 8);
            m3 = _mm_srli_si128(m1, 8);
            m4 = _mm256_castsi256_si128(d1);
            m5 = _mm256_extracti128_si256(d1, 1);
            m6 = _mm_srli_si128(m4, 8);
            m7 = _mm_srli_si128(m5, 8);

            _mm_storel_epi64((__m128i*)(dst + 1 * line), m0);
            _mm_storel_epi64((__m128i*)(dst + 3 * line), m1);
            _mm_storel_epi64((__m128i*)(dst + 5 * line), m2);
            _mm_storel_epi64((__m128i*)(dst + 7 * line), m3);
            _mm_storel_epi64((__m128i*)(dst + 9 * line), m4);
            _mm_storel_epi64((__m128i*)(dst + 11 * line), m5);
            _mm_storel_epi64((__m128i*)(dst + 13 * line), m6);
            _mm_storel_epi64((__m128i*)(dst + 15 * line), m7);

            for (i = 8; i < 16; ++i)
            {
                __m256i tm_0 = _mm256_setr_epi32(xeve_tbl_tm64[i * 2 + 1][0], xeve_tbl_tm64[i * 2 + 1][1], xeve_tbl_tm64[i * 2 + 1][2], xeve_tbl_tm64[i * 2 + 1][3], xeve_tbl_tm64[i * 2 + 1][4], xeve_tbl_tm64[i * 2 + 1][5], xeve_tbl_tm64[i * 2 + 1][6], xeve_tbl_tm64[i * 2 + 1][7]);
                __m256i tm_1 = _mm256_setr_epi32(xeve_tbl_tm64[i * 2 + 1][8], xeve_tbl_tm64[i * 2 + 1][9], xeve_tbl_tm64[i * 2 + 1][10], xeve_tbl_tm64[i * 2 + 1][11], xeve_tbl_tm64[i * 2 + 1][12], xeve_tbl_tm64[i * 2 + 1][13], xeve_tbl_tm64[i * 2 + 1][14], xeve_tbl_tm64[i * 2 + 1][15]);
                __m256i tm_2 = _mm256_setr_epi32(xeve_tbl_tm64[i * 2 + 1][16], xeve_tbl_tm64[i * 2 + 1][17], xeve_tbl_tm64[i * 2 + 1][18], xeve_tbl_tm64[i * 2 + 1][19], xeve_tbl_tm64[i * 2 + 1][20], xeve_tbl_tm64[i * 2 + 1][21], xeve_tbl_tm64[i * 2 + 1][22], xeve_tbl_tm64[i * 2 + 1][23]);
                __m256i tm_3 = _mm256_setr_epi32(xeve_tbl_tm64[i * 2 + 1][24], xeve_tbl_tm64[i * 2 + 1][25], xeve_tbl_tm64[i * 2 + 1][26], xeve_tbl_tm64[i * 2 + 1][27], xeve_tbl_tm64[i * 2 + 1][28], xeve_tbl_tm64[i * 2 + 1][29], xeve_tbl_tm64[i * 2 + 1][30], xeve_tbl_tm64[i * 2 + 1][31]);
                CALCU_O(tm_0, tm_1, tm_2, tm_3, dst_reg[i - 8]);
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

            d0 = _mm256_add_epi32(d0, add);
            d1 = _mm256_add_epi32(d1, add);
            d2 = _mm256_add_epi32(d2, add);
            d3 = _mm256_add_epi32(d3, add);

            d0 = _mm256_srai_epi32(d0, shift);
            d1 = _mm256_srai_epi32(d1, shift);
            d2 = _mm256_srai_epi32(d2, shift);
            d3 = _mm256_srai_epi32(d3, shift);

            d0 = _mm256_packs_epi32(d0, d1);
            d1 = _mm256_packs_epi32(d2, d3);

            m0 = _mm256_castsi256_si128(d0);
            m1 = _mm256_extracti128_si256(d0, 1);
            m2 = _mm_srli_si128(m0, 8);
            m3 = _mm_srli_si128(m1, 8);
            m4 = _mm256_castsi256_si128(d1);
            m5 = _mm256_extracti128_si256(d1, 1);
            m6 = _mm_srli_si128(m4, 8);
            m7 = _mm_srli_si128(m5, 8);

            _mm_storel_epi64((__m128i*)(dst + 17 * line), m0);
            _mm_storel_epi64((__m128i*)(dst + 19 * line), m1);
            _mm_storel_epi64((__m128i*)(dst + 21 * line), m2);
            _mm_storel_epi64((__m128i*)(dst + 23 * line), m3);
            _mm_storel_epi64((__m128i*)(dst + 25 * line), m4);
            _mm_storel_epi64((__m128i*)(dst + 27 * line), m5);
            _mm_storel_epi64((__m128i*)(dst + 29 * line), m6);
            _mm_storel_epi64((__m128i*)(dst + 31 * line), m7);
            dst += 4;
        }
    }
    else
    {
        tx_pb64(src, dst, shift, line);
    }
}

const XEVE_TX xeve_tbl_tx_avx[MAX_TR_LOG2] =
{
    tx_pb2,
    tx_pb4,
    tx_pb8_avx,
    tx_pb16_avx,
    tx_pb32_avx,
    tx_pb64_avx
};


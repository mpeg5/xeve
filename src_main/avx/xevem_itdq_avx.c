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
#include "xevem_itdq.h"
#include "xevem_tq_avx.h"

#define TRANSPOSE_8x4_16BIT(I0, I1, I2, I3, I4, I5, I6, I7, O0, O1, O2, O3) \
    tr0_0 = _mm_unpacklo_epi16(I0, I1); \
    tr0_1 = _mm_unpacklo_epi16(I2, I3); \
    tr0_2 = _mm_unpacklo_epi16(I4, I5); \
    tr0_3 = _mm_unpacklo_epi16(I6, I7); \
    tr1_0 = _mm_unpacklo_epi32(tr0_0, tr0_1); \
    tr1_1 = _mm_unpackhi_epi32(tr0_0, tr0_1); \
    tr1_2 = _mm_unpacklo_epi32(tr0_2, tr0_3); \
    tr1_3 = _mm_unpackhi_epi32(tr0_2, tr0_3); \
    O0 = _mm_unpacklo_epi64(tr1_0, tr1_2); \
    O1 = _mm_unpackhi_epi64(tr1_0, tr1_2); \
    O2 = _mm_unpacklo_epi64(tr1_1, tr1_3); \
    O3 = _mm_unpackhi_epi64(tr1_1, tr1_3);

// transpose 8x8: 8 x 8(32bit) --> 8 x 8(16bit)
// O0: row0, row4
// O1: row1, row5
// O2: row2, row6
// O3: row3, row7
#define TRANSPOSE_8x8_32BIT_16BIT(I0, I1, I2, I3, I4, I5, I6, I7, O0, O1, O2, O3) \
    I0 = _mm256_packs_epi32(I0, I4);    \
    I1 = _mm256_packs_epi32(I1, I5);    \
    I2 = _mm256_packs_epi32(I2, I6);    \
    I3 = _mm256_packs_epi32(I3, I7);    \
    I4 = _mm256_unpacklo_epi16(I0, I2); \
    I5 = _mm256_unpackhi_epi16(I0, I2); \
    I6 = _mm256_unpacklo_epi16(I1, I3); \
    I7 = _mm256_unpackhi_epi16(I1, I3); \
    I0 = _mm256_unpacklo_epi16(I4, I6); \
    I1 = _mm256_unpackhi_epi16(I4, I6); \
    I2 = _mm256_unpacklo_epi16(I5, I7); \
    I3 = _mm256_unpackhi_epi16(I5, I7); \
    O0 = _mm256_unpacklo_epi64(I0, I2); \
    O1 = _mm256_unpackhi_epi64(I0, I2); \
    O2 = _mm256_unpacklo_epi64(I1, I3); \
    O3 = _mm256_unpackhi_epi64(I1, I3)

// transpose 8x8: 16 x 8(32bit) --> 8 x 16(16bit)
#define TRANSPOSE_16x8_32BIT_16BIT(I00, I01, I02, I03, I04, I05, I06, I07, I08, I09, I10, I11, I12, I13, I14, I15, O0, O1, O2, O3, O4, O5, O6, O7)\
    TRANSPOSE_8x8_32BIT_16BIT(I00, I01, I02, I03, I04, I05, I06, I07, I04, I05, I06, I07); \
    TRANSPOSE_8x8_32BIT_16BIT(I08, I09, I10, I11, I12, I13, I14, I15, I12, I13, I14, I15); \
    O0 = _mm256_insertf128_si256(I04, _mm256_castsi256_si128(I12), 1);      \
    O1 = _mm256_insertf128_si256(I05, _mm256_castsi256_si128(I13), 1);      \
    O2 = _mm256_insertf128_si256(I06, _mm256_castsi256_si128(I14), 1);      \
    O3 = _mm256_insertf128_si256(I07, _mm256_castsi256_si128(I15), 1);      \
    O4 = _mm256_insertf128_si256(I12, _mm256_extracti128_si256(I04, 1), 0); \
    O5 = _mm256_insertf128_si256(I13, _mm256_extracti128_si256(I05, 1), 0); \
    O6 = _mm256_insertf128_si256(I14, _mm256_extracti128_si256(I06, 1), 0); \
    O7 = _mm256_insertf128_si256(I15, _mm256_extracti128_si256(I07, 1), 0)

#define set_vals(a,b) b, a, b, a, b, a, b, a, b, a, b, a, b, a, b, a
#define set_vals1(a,b) b, a, b, a, b, a, b, a

static inline void itx_pb2_avx(s16* src, s16* dst, int shift, int line)
{
    int j;
    int E, O;
    int add = shift == 0 ? 0 : 1 << (shift - 1);
    for (j = 0; j < line; j++)
    {
        /* E and O */
        E = src[0 * line + j] + src[1 * line + j];
        O = src[0 * line + j] - src[1 * line + j];

        dst[j * 2 + 0] = ITX_CLIP((xeve_tbl_tm2[0][0] * E + add) >> shift);
        dst[j * 2 + 1] = ITX_CLIP((xeve_tbl_tm2[1][0] * O + add) >> shift);
    }
}

static inline void itx_pb4_avx(s16* src, s16* dst, int shift, int line)
{
    int j;
    int E[2], O[2];
    int add = 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
        O[0] = xeve_tbl_tm4[1][0] * src[1 * line + j] + xeve_tbl_tm4[3][0] * src[3 * line + j];
        O[1] = xeve_tbl_tm4[1][1] * src[1 * line + j] + xeve_tbl_tm4[3][1] * src[3 * line + j];
        E[0] = xeve_tbl_tm4[0][0] * src[0 * line + j] + xeve_tbl_tm4[2][0] * src[2 * line + j];
        E[1] = xeve_tbl_tm4[0][1] * src[0 * line + j] + xeve_tbl_tm4[2][1] * src[2 * line + j];

        /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
        dst[j * 4 + 0] = ITX_CLIP((E[0] + O[0] + add) >> shift);
        dst[j * 4 + 1] = ITX_CLIP((E[1] + O[1] + add) >> shift);
        dst[j * 4 + 2] = ITX_CLIP((E[1] - O[1] + add) >> shift);
        dst[j * 4 + 3] = ITX_CLIP((E[0] - O[0] + add) >> shift);
    }
}

static inline void itx_pb8_avx(s16* src, s16* dst, int shift, int line)
{
    int j, k;
    int E[4], O[4];
    int EE[2], EO[2];
    int add = 1 << (shift - 1);

    for (j = 0; j < line; j++)
    {
        /* Utilizing symmetry properties to the maximum to minimize the number of multiplications */
        for (k = 0; k < 4; k++)
        {
            O[k] = xeve_tbl_tm8[1][k] * src[1 * line + j] + xeve_tbl_tm8[3][k] * src[3 * line + j] + xeve_tbl_tm8[5][k] * src[5 * line + j] + xeve_tbl_tm8[7][k] * src[7 * line + j];
        }

        EO[0] = xeve_tbl_tm8[2][0] * src[2 * line + j] + xeve_tbl_tm8[6][0] * src[6 * line + j];
        EO[1] = xeve_tbl_tm8[2][1] * src[2 * line + j] + xeve_tbl_tm8[6][1] * src[6 * line + j];
        EE[0] = xeve_tbl_tm8[0][0] * src[0 * line + j] + xeve_tbl_tm8[4][0] * src[4 * line + j];
        EE[1] = xeve_tbl_tm8[0][1] * src[0 * line + j] + xeve_tbl_tm8[4][1] * src[4 * line + j];

        /* Combining even and odd terms at each hierarchy levels to calculate the final spatial domain vector */
        E[0] = EE[0] + EO[0];
        E[3] = EE[0] - EO[0];
        E[1] = EE[1] + EO[1];
        E[2] = EE[1] - EO[1];

        for (k = 0; k < 4; k++)
        {
            dst[j * 8 + k] = ITX_CLIP((E[k] + O[k] + add) >> shift);
            dst[j * 8 + k + 4] = ITX_CLIP((E[3 - k] - O[3 - k] + add) >> shift);
        }
    }
}

void itx_pb16_avx(s16* src, s16* dst, int shift, int line)
{
    int bit_depth = 10;
    const __m256i p87_p90 = _mm256_set_epi16(set_vals(90, 87)); // 5701722
    const __m256i p70_p80 = _mm256_set_epi16(set_vals(80, 70)); // 4587600
    const __m256i p43_p57 = _mm256_set_epi16(set_vals(57, 43)); // 2818105
    const __m256i p09_p26 = _mm256_set_epi16(set_vals(26, 9)); // 589850
    const __m256i p57_p87 = _mm256_set_epi16(set_vals(87, 57)); // 3735639
    const __m256i n43_p09 = _mm256_set_epi16(set_vals(9, -43)); // -2818039
    const __m256i n90_n80 = _mm256_set_epi16(set_vals(-80, -90)); // -5832784
    const __m256i n26_n70 = _mm256_set_epi16(set_vals(-70, -26)); // -1638470
    const __m256i p09_p80 = _mm256_set_epi16(set_vals(80, 9)); // 589904
    const __m256i n87_n70 = _mm256_set_epi16(set_vals(-70, -87)); // -5636166
    const __m256i p57_n26 = _mm256_set_epi16(set_vals(-26, 57)); // 3801062
    const __m256i p43_p90 = _mm256_set_epi16(set_vals(90, 43)); // 2818138
    const __m256i n43_p70 = _mm256_set_epi16(set_vals(70, -43)); // -2817978
    const __m256i p09_n87 = _mm256_set_epi16(set_vals(-87, 9)); // 655273
    const __m256i p26_p90 = _mm256_set_epi16(set_vals(90, 26)); // 1704026
    const __m256i n57_n80 = _mm256_set_epi16(set_vals(-80, -57)); // -3670096
    const __m256i n80_p57 = _mm256_set_epi16(set_vals(57, -80)); // -5242823
    const __m256i p90_n26 = _mm256_set_epi16(set_vals(-26, 90)); // 5963750
    const __m256i n87_n09 = _mm256_set_epi16(set_vals(-9, -87)); // -5636105
    const __m256i p70_p43 = _mm256_set_epi16(set_vals(43, 70));
    const __m256i n90_p43 = _mm256_set_epi16(set_vals(43, -90));
    const __m256i p26_p57 = _mm256_set_epi16(set_vals(57, 26));
    const __m256i p70_n87 = _mm256_set_epi16(set_vals(-87, 70));
    const __m256i n80_p09 = _mm256_set_epi16(set_vals(9, -80));
    const __m256i n70_p26 = _mm256_set_epi16(set_vals(26, -70));
    const __m256i n80_p90 = _mm256_set_epi16(set_vals(90, -80));
    const __m256i p09_p43 = _mm256_set_epi16(set_vals(43, 9));
    const __m256i p87_n57 = _mm256_set_epi16(set_vals(-57, 87));
    const __m256i n26_p09 = _mm256_set_epi16(set_vals(9, -26));
    const __m256i n57_p43 = _mm256_set_epi16(set_vals(43, -57));
    const __m256i n80_p70 = _mm256_set_epi16(set_vals(70, -80));
    const __m256i n90_p87 = _mm256_set_epi16(set_vals(87, -90));
    const __m256i p75_p89 = _mm256_set_epi16(set_vals(89, 75));
    const __m256i p18_p50 = _mm256_set_epi16(set_vals(50, 18));
    const __m256i n18_p75 = _mm256_set_epi16(set_vals(75, -18));
    const __m256i n50_n89 = _mm256_set_epi16(set_vals(-89, -50));
    const __m256i n89_p50 = _mm256_set_epi16(set_vals(50, -89));
    const __m256i p75_p18 = _mm256_set_epi16(set_vals(18, 75));
    const __m256i n50_p18 = _mm256_set_epi16(set_vals(18, -50));
    const __m256i n89_p75 = _mm256_set_epi16(set_vals(75, -89));
    const __m256i p35_p84 = _mm256_set_epi16(set_vals(84, 35));
    const __m256i n84_p35 = _mm256_set_epi16(set_vals(35, -84));
    const __m256i p64_p64 = _mm256_set_epi16(set_vals(64, 64));
    const __m256i n64_p64 = _mm256_set_epi16(set_vals(64, -64));


    int i;
    __m256i c32_off = _mm256_set1_epi32(1 << (shift - 1));
    __m128i in00, in01, in02, in03, in04, in05, in06, in07;
    __m128i in08, in09, in10, in11, in12, in13, in14, in15;
    __m128i ss0, ss1, ss2, ss3, ss4, ss5, ss6, ss7;
    __m256i res00, res01, res02, res03, res04, res05, res06, res07;
    __m256i T_00_00, T_00_01, T_00_02, T_00_03, T_00_04, T_00_05, T_00_06, T_00_07;
    __m256i O0, O1, O2, O3, O4, O5, O6, O7;
    __m256i EO0, EO1, EO2, EO3;
    __m256i EEO0, EEO1;
    __m256i EEE0, EEE1;
    __m256i T00, T01;

    for (i = 0; i < line; i += 8) {
        in01 = _mm_loadu_si128((const __m128i*) & src[1 * line + i]);   // [17 16 15 14 13 12 11 10]
        in03 = _mm_loadu_si128((const __m128i*) & src[3 * line + i]);   // [37 36 35 34 33 32 31 30]
        in05 = _mm_loadu_si128((const __m128i*) & src[5 * line + i]);   // [57 56 55 54 53 52 51 50]
        in07 = _mm_loadu_si128((const __m128i*) & src[7 * line + i]);   // [77 76 75 74 73 72 71 70]
        in09 = _mm_loadu_si128((const __m128i*) & src[9 * line + i]);
        in11 = _mm_loadu_si128((const __m128i*) & src[11 * line + i]);
        in13 = _mm_loadu_si128((const __m128i*) & src[13 * line + i]);
        in15 = _mm_loadu_si128((const __m128i*) & src[15 * line + i]);

        ss0 = _mm_unpacklo_epi16(in01, in03);
        ss1 = _mm_unpacklo_epi16(in05, in07);
        ss2 = _mm_unpacklo_epi16(in09, in11);
        ss3 = _mm_unpacklo_epi16(in13, in15);
        ss4 = _mm_unpackhi_epi16(in01, in03);
        ss5 = _mm_unpackhi_epi16(in05, in07);
        ss6 = _mm_unpackhi_epi16(in09, in11);
        ss7 = _mm_unpackhi_epi16(in13, in15);

        T_00_00 = _mm256_set_m128i(ss4, ss0);
        T_00_01 = _mm256_set_m128i(ss5, ss1);
        T_00_02 = _mm256_set_m128i(ss6, ss2);
        T_00_03 = _mm256_set_m128i(ss7, ss3);
#define COMPUTE_ROW(c0103, c0507, c0911, c1315, row) \
    T00 = _mm256_add_epi32(_mm256_madd_epi16(T_00_00, c0103), _mm256_madd_epi16(T_00_01, c0507)); \
    T01 = _mm256_add_epi32(_mm256_madd_epi16(T_00_02, c0911), _mm256_madd_epi16(T_00_03, c1315)); \
    row = _mm256_add_epi32(T00, T01);

            COMPUTE_ROW(p87_p90, p70_p80, p43_p57, p09_p26, O0)
            COMPUTE_ROW(p57_p87, n43_p09, n90_n80, n26_n70, O1)
            COMPUTE_ROW(p09_p80, n87_n70, p57_n26, p43_p90, O2)
            COMPUTE_ROW(n43_p70, p09_n87, p26_p90, n57_n80, O3)
            COMPUTE_ROW(n80_p57, p90_n26, n87_n09, p70_p43, O4)
            COMPUTE_ROW(n90_p43, p26_p57, p70_n87, n80_p09, O5)
            COMPUTE_ROW(n70_p26, n80_p90, p09_p43, p87_n57, O6)
            COMPUTE_ROW(n26_p09, n57_p43, n80_p70, n90_p87, O7)

#undef COMPUTE_ROW

        in00 = _mm_loadu_si128((const __m128i*) & src[0 * line + i]);   // [07 06 05 04 03 02 01 00]
        in02 = _mm_loadu_si128((const __m128i*) & src[2 * line + i]);   // [27 26 25 24 23 22 21 20]
        in04 = _mm_loadu_si128((const __m128i*) & src[4 * line + i]);   // [47 46 45 44 43 42 41 40]
        in06 = _mm_loadu_si128((const __m128i*) & src[6 * line + i]);   // [67 66 65 64 63 62 61 60]
        in08 = _mm_loadu_si128((const __m128i*) & src[8 * line + i]);
        in10 = _mm_loadu_si128((const __m128i*) & src[10 * line + i]);
        in12 = _mm_loadu_si128((const __m128i*) & src[12 * line + i]);
        in14 = _mm_loadu_si128((const __m128i*) & src[14 * line + i]);

        ss0 = _mm_unpacklo_epi16(in02, in06);
        ss1 = _mm_unpacklo_epi16(in10, in14);
        ss2 = _mm_unpacklo_epi16(in04, in12);
        ss3 = _mm_unpacklo_epi16(in00, in08);
        ss4 = _mm_unpackhi_epi16(in02, in06);
        ss5 = _mm_unpackhi_epi16(in10, in14);
        ss6 = _mm_unpackhi_epi16(in04, in12);
        ss7 = _mm_unpackhi_epi16(in00, in08);

        T_00_04 = _mm256_set_m128i(ss4, ss0);
        T_00_05 = _mm256_set_m128i(ss5, ss1);
        T_00_06 = _mm256_set_m128i(ss6, ss2);
        T_00_07 = _mm256_set_m128i(ss7, ss3);

        EO0 = _mm256_add_epi32(_mm256_madd_epi16(T_00_04, p75_p89), _mm256_madd_epi16(T_00_05, p18_p50)); // EO0
        EO1 = _mm256_add_epi32(_mm256_madd_epi16(T_00_04, n18_p75), _mm256_madd_epi16(T_00_05, n50_n89)); // EO1
        EO2 = _mm256_add_epi32(_mm256_madd_epi16(T_00_04, n89_p50), _mm256_madd_epi16(T_00_05, p75_p18)); // EO2
        EO3 = _mm256_add_epi32(_mm256_madd_epi16(T_00_04, n50_p18), _mm256_madd_epi16(T_00_05, n89_p75)); // EO3

        EEO0 = _mm256_madd_epi16(T_00_06, p35_p84);
        EEO1 = _mm256_madd_epi16(T_00_06, n84_p35);
        EEE0 = _mm256_madd_epi16(T_00_07, p64_p64);
        EEE1 = _mm256_madd_epi16(T_00_07, n64_p64);

        {
            const __m256i EE0 = _mm256_add_epi32(EEE0, EEO0);       // EE0 = EEE0 + EEO0
            const __m256i EE1 = _mm256_add_epi32(EEE1, EEO1);       // EE1 = EEE1 + EEO1
            const __m256i EE3 = _mm256_sub_epi32(EEE0, EEO0);       // EE2 = EEE0 - EEO0
            const __m256i EE2 = _mm256_sub_epi32(EEE1, EEO1);       // EE3 = EEE1 - EEO1

            const __m256i E0 = _mm256_add_epi32(EE0, EO0);          // E0 = EE0 + EO0
            const __m256i E1 = _mm256_add_epi32(EE1, EO1);          // E1 = EE1 + EO1
            const __m256i E2 = _mm256_add_epi32(EE2, EO2);          // E2 = EE2 + EO2
            const __m256i E3 = _mm256_add_epi32(EE3, EO3);          // E3 = EE3 + EO3
            const __m256i E7 = _mm256_sub_epi32(EE0, EO0);          // E0 = EE0 - EO0
            const __m256i E6 = _mm256_sub_epi32(EE1, EO1);          // E1 = EE1 - EO1
            const __m256i E5 = _mm256_sub_epi32(EE2, EO2);          // E2 = EE2 - EO2
            const __m256i E4 = _mm256_sub_epi32(EE3, EO3);          // E3 = EE3 - EO3

            const __m256i T10 = _mm256_add_epi32(E0, c32_off);      // E0 + off
            const __m256i T11 = _mm256_add_epi32(E1, c32_off);      // E1 + off
            const __m256i T12 = _mm256_add_epi32(E2, c32_off);      // E2 + off
            const __m256i T13 = _mm256_add_epi32(E3, c32_off);      // E3 + off
            const __m256i T14 = _mm256_add_epi32(E4, c32_off);      // E4 + off
            const __m256i T15 = _mm256_add_epi32(E5, c32_off);      // E5 + off
            const __m256i T16 = _mm256_add_epi32(E6, c32_off);      // E6 + off
            const __m256i T17 = _mm256_add_epi32(E7, c32_off);      // E7 + off

            __m256i T20 = _mm256_add_epi32(T10, O0);                // E0 + O0 + off
            __m256i T21 = _mm256_add_epi32(T11, O1);                // E1 + O1 + off
            __m256i T22 = _mm256_add_epi32(T12, O2);                // E2 + O2 + off
            __m256i T23 = _mm256_add_epi32(T13, O3);                // E3 + O3 + off
            __m256i T24 = _mm256_add_epi32(T14, O4);                // E4
            __m256i T25 = _mm256_add_epi32(T15, O5);                // E5
            __m256i T26 = _mm256_add_epi32(T16, O6);                // E6
            __m256i T27 = _mm256_add_epi32(T17, O7);                // E7
            __m256i T2F = _mm256_sub_epi32(T10, O0);                // E0 - O0 + off
            __m256i T2E = _mm256_sub_epi32(T11, O1);                // E1 - O1 + off
            __m256i T2D = _mm256_sub_epi32(T12, O2);                // E2 - O2 + off
            __m256i T2C = _mm256_sub_epi32(T13, O3);                // E3 - O3 + off
            __m256i T2B = _mm256_sub_epi32(T14, O4);                // E4
            __m256i T2A = _mm256_sub_epi32(T15, O5);                // E5
            __m256i T29 = _mm256_sub_epi32(T16, O6);                // E6
            __m256i T28 = _mm256_sub_epi32(T17, O7);                // E7

            T20 = _mm256_srai_epi32(T20, shift);                    // [30 20 10 00]
            T21 = _mm256_srai_epi32(T21, shift);                    // [31 21 11 01]
            T22 = _mm256_srai_epi32(T22, shift);                    // [32 22 12 02]
            T23 = _mm256_srai_epi32(T23, shift);                    // [33 23 13 03]
            T24 = _mm256_srai_epi32(T24, shift);                    // [33 24 14 04]
            T25 = _mm256_srai_epi32(T25, shift);                    // [35 25 15 05]
            T26 = _mm256_srai_epi32(T26, shift);                    // [36 26 16 06]
            T27 = _mm256_srai_epi32(T27, shift);                    // [37 27 17 07]
            T28 = _mm256_srai_epi32(T28, shift);                    // [30 20 10 00] x8
            T29 = _mm256_srai_epi32(T29, shift);                    // [31 21 11 01] x9
            T2A = _mm256_srai_epi32(T2A, shift);                    // [32 22 12 02] xA
            T2B = _mm256_srai_epi32(T2B, shift);                    // [33 23 13 03] xB
            T2C = _mm256_srai_epi32(T2C, shift);                    // [33 24 14 04] xC
            T2D = _mm256_srai_epi32(T2D, shift);                    // [35 25 15 05] xD
            T2E = _mm256_srai_epi32(T2E, shift);                    // [36 26 16 06] xE
            T2F = _mm256_srai_epi32(T2F, shift);                    // [37 27 17 07] xF

            // transpose 16x8 --> 8x16
            TRANSPOSE_16x8_32BIT_16BIT(T20, T21, T22, T23, T24, T25, T26, T27,
                T28, T29, T2A, T2B, T2C, T2D, T2E, T2F, res00, res01, res02, res03, res04, res05, res06, res07);
        }
        if (bit_depth == 8) { //clip
            __m256i max_val = _mm256_set1_epi16(255);
            __m256i min_val = _mm256_set1_epi16(-256);

            res00 = _mm256_min_epi16(res00, max_val);
            res01 = _mm256_min_epi16(res01, max_val);
            res02 = _mm256_min_epi16(res02, max_val);
            res03 = _mm256_min_epi16(res03, max_val);
            res04 = _mm256_min_epi16(res04, max_val);
            res05 = _mm256_min_epi16(res05, max_val);
            res06 = _mm256_min_epi16(res06, max_val);
            res07 = _mm256_min_epi16(res07, max_val);
            res00 = _mm256_max_epi16(res00, min_val);
            res01 = _mm256_max_epi16(res01, min_val);
            res02 = _mm256_max_epi16(res02, min_val);
            res03 = _mm256_max_epi16(res03, min_val);
            res04 = _mm256_max_epi16(res04, min_val);
            res05 = _mm256_max_epi16(res05, min_val);
            res06 = _mm256_max_epi16(res06, min_val);
            res07 = _mm256_max_epi16(res07, min_val);

        }

        _mm256_storeu_si256((__m256i*) & dst[16 * 0], res00);
        _mm256_storeu_si256((__m256i*) & dst[16 * 1], res01);
        _mm256_storeu_si256((__m256i*) & dst[16 * 2], res02);
        _mm256_storeu_si256((__m256i*) & dst[16 * 3], res03);
        _mm256_storeu_si256((__m256i*) & dst[16 * 4], res04);
        _mm256_storeu_si256((__m256i*) & dst[16 * 5], res05);
        _mm256_storeu_si256((__m256i*) & dst[16 * 6], res06);
        _mm256_storeu_si256((__m256i*) & dst[16 * 7], res07);

        dst += 16 * 8; // 8 rows
    }
}


void itx_pb32_avx(s16* src, s16* dst, int shift, int line)
{
    int bit_depth = 10;

    const __m256i p90_p90 = _mm256_set1_epi32(5898330);
    const __m256i p85_p88 = _mm256_set1_epi32(5570648);
    const __m256i p78_p82 = _mm256_set1_epi32(5111890);
    const __m256i p67_p73 = _mm256_set1_epi32(4390985);
    const __m256i p54_p61 = _mm256_set1_epi32(3539005);
    const __m256i p39_p47 = _mm256_set1_epi32(2555951);
    const __m256i p22_p30 = _mm256_set1_epi32(1441822);
    const __m256i p82_p90 = _mm256_set1_epi32(5374042);
    const __m256i p47_p67 = _mm256_set1_epi32(3080259);
    const __m256i n04_p22 = _mm256_set1_epi32(-262122);
    const __m256i n54_n30 = _mm256_set1_epi32(-3473438);
    const __m256i n85_n73 = _mm256_set1_epi32(-5505097);
    const __m256i n88_n90 = _mm256_set1_epi32(-5701722);
    const __m256i n61_n78 = _mm256_set1_epi32(-3932238);
    const __m256i n13_n39 = _mm256_set1_epi32(-786471);
    const __m256i p67_p88 = _mm256_set1_epi32(4391000);
    const __m256i n13_p30 = _mm256_set1_epi32(-851938);
    const __m256i n82_n54 = _mm256_set1_epi32(-5308470);
    const __m256i n78_n90 = _mm256_set1_epi32(-5046362);
    const __m256i n04_n47 = _mm256_set1_epi32(-196655);
    const __m256i p73_p39 = _mm256_set1_epi32(4784167);
    const __m256i p85_p90 = _mm256_set1_epi32(5570650);
    const __m256i p22_p61 = _mm256_set1_epi32(1441853);
    const __m256i p47_p85 = _mm256_set1_epi32(3080277);
    const __m256i n67_n13 = _mm256_set1_epi32(-4325389);
    const __m256i n73_n90 = _mm256_set1_epi32(-4718682);
    const __m256i p39_n22 = _mm256_set1_epi32(2621418);
    const __m256i p88_p82 = _mm256_set1_epi32(5767250);
    const __m256i n04_p54 = _mm256_set1_epi32(-262090);
    const __m256i n90_n61 = _mm256_set1_epi32(-5832765);
    const __m256i n30_n78 = _mm256_set1_epi32(-1900622);
    const __m256i p22_p82 = _mm256_set1_epi32(1441874);
    const __m256i n90_n54 = _mm256_set1_epi32(-5832758);
    const __m256i p13_n61 = _mm256_set1_epi32(917443);
    const __m256i p85_p78 = _mm256_set1_epi32(5570638);
    const __m256i n47_p30 = _mm256_set1_epi32(-3080162);
    const __m256i n67_n90 = _mm256_set1_epi32(-4325466);
    const __m256i p73_p04 = _mm256_set1_epi32(4784132);
    const __m256i p39_p88 = _mm256_set1_epi32(2555992);
    const __m256i n04_p78 = _mm256_set1_epi32(-262066);
    const __m256i n73_n82 = _mm256_set1_epi32(-4718674);
    const __m256i p85_p13 = _mm256_set1_epi32(5570573);
    const __m256i n22_p67 = _mm256_set1_epi32(-1441725);
    const __m256i n61_n88 = _mm256_set1_epi32(-3932248);
    const __m256i p90_p30 = _mm256_set1_epi32(5898270);
    const __m256i n39_p54 = _mm256_set1_epi32(-2555850);
    const __m256i n47_n90 = _mm256_set1_epi32(-3014746);
    const __m256i n30_p73 = _mm256_set1_epi32(-1966007);
    const __m256i n22_n90 = _mm256_set1_epi32(-1376346);
    const __m256i p67_p78 = _mm256_set1_epi32(4390990);
    const __m256i n90_n39 = _mm256_set1_epi32(-5832743);
    const __m256i p82_n13 = _mm256_set1_epi32(5439475);
    const __m256i n47_p61 = _mm256_set1_epi32(-3080131);
    const __m256i n04_n88 = _mm256_set1_epi32(-196696);
    const __m256i p54_p85 = _mm256_set1_epi32(3539029);
    const __m256i n54_p67 = _mm256_set1_epi32(-3538877);
    const __m256i p39_n78 = _mm256_set1_epi32(2621362);
    const __m256i n22_p85 = _mm256_set1_epi32(-1441707);
    const __m256i p04_n90 = _mm256_set1_epi32(327590);
    const __m256i p13_p90 = _mm256_set1_epi32(852058);
    const __m256i n30_n88 = _mm256_set1_epi32(-1900632);
    const __m256i p47_p82 = _mm256_set1_epi32(3080274);
    const __m256i n61_n73 = _mm256_set1_epi32(-3932233);
    const __m256i n73_p61 = _mm256_set1_epi32(-4784067);
    const __m256i p82_n47 = _mm256_set1_epi32(5439441);
    const __m256i n88_p30 = _mm256_set1_epi32(-5767138);
    const __m256i p90_n13 = _mm256_set1_epi32(5963763);
    const __m256i n90_n04 = _mm256_set1_epi32(-5832708);
    const __m256i p85_p22 = _mm256_set1_epi32(5570582);
    const __m256i n78_n39 = _mm256_set1_epi32(-5046311);
    const __m256i p67_p54 = _mm256_set1_epi32(4390966);
    const __m256i n85_p54 = _mm256_set1_epi32(-5570506);
    const __m256i p88_n04 = _mm256_set1_epi32(5832700);
    const __m256i n61_n47 = _mm256_set1_epi32(-3932207);
    const __m256i p13_p82 = _mm256_set1_epi32(852050);
    const __m256i p39_n90 = _mm256_set1_epi32(2621350);
    const __m256i n78_p67 = _mm256_set1_epi32(-5111741);
    const __m256i p90_n22 = _mm256_set1_epi32(5963754);
    const __m256i n73_n30 = _mm256_set1_epi32(-4718622);
    const __m256i n90_p47 = _mm256_set1_epi32(-5898193);
    const __m256i p54_p39 = _mm256_set1_epi32(3538983);
    const __m256i p30_n90 = _mm256_set1_epi32(2031526);
    const __m256i n88_p61 = _mm256_set1_epi32(-5767107);
    const __m256i p67_p22 = _mm256_set1_epi32(4390934);
    const __m256i p13_n85 = _mm256_set1_epi32(917419);
    const __m256i n82_p73 = _mm256_set1_epi32(-5373879);
    const __m256i p78_p04 = _mm256_set1_epi32(5111812);
    const __m256i n88_p39 = _mm256_set1_epi32(-5767129);
    const __m256i n04_p73 = _mm256_set1_epi32(-262071);
    const __m256i p90_n67 = _mm256_set1_epi32(5963709);
    const __m256i n30_n47 = _mm256_set1_epi32(-1900591);
    const __m256i n78_p85 = _mm256_set1_epi32(-5111723);
    const __m256i p61_p13 = _mm256_set1_epi32(3997709);
    const __m256i p54_n90 = _mm256_set1_epi32(3604390);
    const __m256i n82_p22 = _mm256_set1_epi32(-5373930);
    const __m256i n78_p30 = _mm256_set1_epi32(-5111778);
    const __m256i n61_p90 = _mm256_set1_epi32(-3997606);
    const __m256i p54_p04 = _mm256_set1_epi32(3538948);
    const __m256i p82_n88 = _mm256_set1_epi32(5439400);
    const __m256i n22_n39 = _mm256_set1_epi32(-1376295);
    const __m256i n90_p73 = _mm256_set1_epi32(-5898167);
    const __m256i n13_p67 = _mm256_set1_epi32(-851901);
    const __m256i p85_n47 = _mm256_set1_epi32(5636049);
    const __m256i n61_p22 = _mm256_set1_epi32(-3997674);
    const __m256i n90_p85 = _mm256_set1_epi32(-5898155);
    const __m256i n39_p73 = _mm256_set1_epi32(-2555831);
    const __m256i p47_n04 = _mm256_set1_epi32(3145724);
    const __m256i p90_n78 = _mm256_set1_epi32(5963698);
    const __m256i p54_n82 = _mm256_set1_epi32(3604398);
    const __m256i n30_n13 = _mm256_set1_epi32(-1900557);
    const __m256i n88_p67 = _mm256_set1_epi32(-5767101);
    const __m256i n39_p13 = _mm256_set1_epi32(-2555891);
    const __m256i n78_p61 = _mm256_set1_epi32(-5111747);
    const __m256i n90_p88 = _mm256_set1_epi32(-5898152);
    const __m256i n73_p85 = _mm256_set1_epi32(-4784043);
    const __m256i n30_p54 = _mm256_set1_epi32(-1966026);
    const __m256i p22_p04 = _mm256_set1_epi32(1441796);
    const __m256i p67_n47 = _mm256_set1_epi32(4456401);
    const __m256i p90_n82 = _mm256_set1_epi32(5963694);
    const __m256i n30_p22 = _mm256_set1_epi32(-1966058);
    const __m256i n47_p39 = _mm256_set1_epi32(-3080153);
    const __m256i n61_p54 = _mm256_set1_epi32(-3997642);
    const __m256i n73_p67 = _mm256_set1_epi32(-4784061);
    const __m256i n82_p78 = _mm256_set1_epi32(-5373874);
    const __m256i n88_p85 = _mm256_set1_epi32(-5767083);
    const __m256i n90_p90 = _mm256_set1_epi32(-5898150);
    const __m256i p87_p90 = _mm256_set1_epi32(5701722);
    const __m256i p70_p80 = _mm256_set1_epi32(4587600);
    const __m256i p43_p57 = _mm256_set1_epi32(2818105);
    const __m256i p09_p26 = _mm256_set1_epi32(589850);
    const __m256i p57_p87 = _mm256_set1_epi32(3735639);
    const __m256i n43_p09 = _mm256_set1_epi32(-2818039);
    const __m256i n90_n80 = _mm256_set1_epi32(-5832784);
    const __m256i n26_n70 = _mm256_set1_epi32(-1638470);
    const __m256i p09_p80 = _mm256_set1_epi32(589904);
    const __m256i n87_n70 = _mm256_set1_epi32(-5636166);
    const __m256i p57_n26 = _mm256_set1_epi32(3801062);
    const __m256i p43_p90 = _mm256_set1_epi32(2818138);
    const __m256i n43_p70 = _mm256_set1_epi32(-2817978);
    const __m256i p09_n87 = _mm256_set1_epi32(655273);
    const __m256i p26_p90 = _mm256_set1_epi32(1704026);
    const __m256i n57_n80 = _mm256_set1_epi32(-3670096);
    const __m256i n80_p57 = _mm256_set1_epi32(-5242823);
    const __m256i p90_n26 = _mm256_set1_epi32(5963750);
    const __m256i n87_n09 = _mm256_set1_epi32(-5636105);
    const __m256i p70_p43 = _mm256_set1_epi32(4587563);
    const __m256i n90_p43 = _mm256_set1_epi32(-5898197);
    const __m256i p26_p57 = _mm256_set1_epi32(1703993);
    const __m256i p70_n87 = _mm256_set1_epi32(4652969);
    const __m256i n80_p09 = _mm256_set1_epi32(-5242871);
    const __m256i n70_p26 = _mm256_set1_epi32(-4587494);
    const __m256i n80_p90 = _mm256_set1_epi32(-5242790);
    const __m256i p09_p43 = _mm256_set1_epi32(589867);
    const __m256i p87_n57 = _mm256_set1_epi32(5767111);
    const __m256i n26_p09 = _mm256_set1_epi32(-1703927);
    const __m256i n57_p43 = _mm256_set1_epi32(-3735509);
    const __m256i n80_p70 = _mm256_set1_epi32(-5242810);
    const __m256i n90_p87 = _mm256_set1_epi32(-5898153);
    const __m256i p35_p84 = _mm256_set1_epi32(2293844);
    const __m256i n84_p35 = _mm256_set1_epi32(-5504989);
    const __m256i p64_p64 = _mm256_set1_epi32(4194368);
    const __m256i n64_p64 = _mm256_set1_epi32(-4194240);
    const __m256i p75_p89 = _mm256_set1_epi32(4915289);
    const __m256i p18_p50 = _mm256_set1_epi32(1179698);
    const __m256i n18_p75 = _mm256_set1_epi32(-1179573);
    const __m256i n50_n89 = _mm256_set1_epi32(-3211353);
    const __m256i n89_p50 = _mm256_set1_epi32(-5832654);
    const __m256i p75_p18 = _mm256_set1_epi32(4915218);
    const __m256i n50_p18 = _mm256_set1_epi32(-3276782);
    const __m256i n89_p75 = _mm256_set1_epi32(-5832629);
    const __m256i p04_p13 = _mm256_set1_epi32(262157);
    const __m256i n13_p04 = _mm256_set1_epi32(-851964);

    __m256i c32_off = _mm256_set1_epi32(1 << (shift - 1));

    __m128i in00, in01, in02, in03, in04, in05, in06, in07, in08, in09, in10, in11, in12, in13, in14, in15;
    __m128i in16, in17, in18, in19, in20, in21, in22, in23, in24, in25, in26, in27, in28, in29, in30, in31;
    __m128i ss00, ss01, ss02, ss03, ss04, ss05, ss06, ss07, ss08, ss09, ss10, ss11, ss12, ss13, ss14, ss15;
    __m256i res00, res01, res02, res03, res04, res05, res06, res07, res08, res09, res10, res11, res12, res13, res14, res15;
    __m256i O00, O01, O02, O03, O04, O05, O06, O07, O08, O09, O10, O11, O12, O13, O14, O15;
    __m256i EO0, EO1, EO2, EO3, EO4, EO5, EO6, EO7;
    __m256i T00, T01, T02, T03;
    int i;

    for (i = 0; i < line; i += 8) {
        in01 = _mm_loadu_si128((const __m128i*) & src[1 * line + i]);
        in03 = _mm_loadu_si128((const __m128i*) & src[3 * line + i]);
        in05 = _mm_loadu_si128((const __m128i*) & src[5 * line + i]);
        in07 = _mm_loadu_si128((const __m128i*) & src[7 * line + i]);
        in09 = _mm_loadu_si128((const __m128i*) & src[9 * line + i]);
        in11 = _mm_loadu_si128((const __m128i*) & src[11 * line + i]);
        in13 = _mm_loadu_si128((const __m128i*) & src[13 * line + i]);
        in15 = _mm_loadu_si128((const __m128i*) & src[15 * line + i]);
        in17 = _mm_loadu_si128((const __m128i*) & src[17 * line + i]);
        in19 = _mm_loadu_si128((const __m128i*) & src[19 * line + i]);
        in21 = _mm_loadu_si128((const __m128i*) & src[21 * line + i]);
        in23 = _mm_loadu_si128((const __m128i*) & src[23 * line + i]);
        in25 = _mm_loadu_si128((const __m128i*) & src[25 * line + i]);
        in27 = _mm_loadu_si128((const __m128i*) & src[27 * line + i]);
        in29 = _mm_loadu_si128((const __m128i*) & src[29 * line + i]);
        in31 = _mm_loadu_si128((const __m128i*) & src[31 * line + i]);

        ss00 = _mm_unpacklo_epi16(in01, in03);
        ss01 = _mm_unpacklo_epi16(in05, in07);
        ss02 = _mm_unpacklo_epi16(in09, in11);
        ss03 = _mm_unpacklo_epi16(in13, in15);
        ss04 = _mm_unpacklo_epi16(in17, in19);
        ss05 = _mm_unpacklo_epi16(in21, in23);
        ss06 = _mm_unpacklo_epi16(in25, in27);
        ss07 = _mm_unpacklo_epi16(in29, in31);

        ss08 = _mm_unpackhi_epi16(in01, in03);
        ss09 = _mm_unpackhi_epi16(in05, in07);
        ss10 = _mm_unpackhi_epi16(in09, in11);
        ss11 = _mm_unpackhi_epi16(in13, in15);
        ss12 = _mm_unpackhi_epi16(in17, in19);
        ss13 = _mm_unpackhi_epi16(in21, in23);
        ss14 = _mm_unpackhi_epi16(in25, in27);
        ss15 = _mm_unpackhi_epi16(in29, in31);

        {
            const __m256i T_00_00 = _mm256_set_m128i(ss08, ss00);       // [33 13 32 12 31 11 30 10]
            const __m256i T_00_01 = _mm256_set_m128i(ss09, ss01);       // [ ]
            const __m256i T_00_02 = _mm256_set_m128i(ss10, ss02);       // [ ]
            const __m256i T_00_03 = _mm256_set_m128i(ss11, ss03);       // [ ]
            const __m256i T_00_04 = _mm256_set_m128i(ss12, ss04);       // [ ]
            const __m256i T_00_05 = _mm256_set_m128i(ss13, ss05);       // [ ]
            const __m256i T_00_06 = _mm256_set_m128i(ss14, ss06);       // [ ]
            const __m256i T_00_07 = _mm256_set_m128i(ss15, ss07);       //

#define COMPUTE_ROW(c0103, c0507, c0911, c1315, c1719, c2123, c2527, c2931, row) \
    T00 = _mm256_add_epi32(_mm256_madd_epi16(T_00_00, c0103), _mm256_madd_epi16(T_00_01, c0507)); \
    T01 = _mm256_add_epi32(_mm256_madd_epi16(T_00_02, c0911), _mm256_madd_epi16(T_00_03, c1315)); \
    T02 = _mm256_add_epi32(_mm256_madd_epi16(T_00_04, c1719), _mm256_madd_epi16(T_00_05, c2123)); \
    T03 = _mm256_add_epi32(_mm256_madd_epi16(T_00_06, c2527), _mm256_madd_epi16(T_00_07, c2931)); \
    row = _mm256_add_epi32(_mm256_add_epi32(T00, T01), _mm256_add_epi32(T02, T03));

                COMPUTE_ROW(p90_p90, p85_p88, p78_p82, p67_p73, p54_p61, p39_p47, p22_p30, p04_p13, O00)
                COMPUTE_ROW(p82_p90, p47_p67, n04_p22, n54_n30, n85_n73, n88_n90, n61_n78, n13_n39, O01)
                COMPUTE_ROW(p67_p88, n13_p30, n82_n54, n78_n90, n04_n47, p73_p39, p85_p90, p22_p61, O02)
                COMPUTE_ROW(p47_p85, n67_n13, n73_n90, p39_n22, p88_p82, n04_p54, n90_n61, n30_n78, O03)
                COMPUTE_ROW(p22_p82, n90_n54, p13_n61, p85_p78, n47_p30, n67_n90, p73_p04, p39_p88, O04)
                COMPUTE_ROW(n04_p78, n73_n82, p85_p13, n22_p67, n61_n88, p90_p30, n39_p54, n47_n90, O05)
                COMPUTE_ROW(n30_p73, n22_n90, p67_p78, n90_n39, p82_n13, n47_p61, n04_n88, p54_p85, O06)
                COMPUTE_ROW(n54_p67, p39_n78, n22_p85, p04_n90, p13_p90, n30_n88, p47_p82, n61_n73, O07)
                COMPUTE_ROW(n73_p61, p82_n47, n88_p30, p90_n13, n90_n04, p85_p22, n78_n39, p67_p54, O08)
                COMPUTE_ROW(n85_p54, p88_n04, n61_n47, p13_p82, p39_n90, n78_p67, p90_n22, n73_n30, O09)
                COMPUTE_ROW(n90_p47, p54_p39, p30_n90, n88_p61, p67_p22, p13_n85, n82_p73, p78_p04, O10)
                COMPUTE_ROW(n88_p39, n04_p73, p90_n67, n30_n47, n78_p85, p61_p13, p54_n90, n82_p22, O11)
                COMPUTE_ROW(n78_p30, n61_p90, p54_p04, p82_n88, n22_n39, n90_p73, n13_p67, p85_n47, O12)
                COMPUTE_ROW(n61_p22, n90_p85, n39_p73, p47_n04, p90_n78, p54_n82, n30_n13, n88_p67, O13)
                COMPUTE_ROW(n39_p13, n78_p61, n90_p88, n73_p85, n30_p54, p22_p04, p67_n47, p90_n82, O14)
                COMPUTE_ROW(n13_p04, n30_p22, n47_p39, n61_p54, n73_p67, n82_p78, n88_p85, n90_p90, O15)
#undef COMPUTE_ROW
        }

        in00 = _mm_loadu_si128((const __m128i*) & src[0 * line + i]);
        in02 = _mm_loadu_si128((const __m128i*) & src[2 * line + i]);
        in04 = _mm_loadu_si128((const __m128i*) & src[4 * line + i]);
        in06 = _mm_loadu_si128((const __m128i*) & src[6 * line + i]);
        in08 = _mm_loadu_si128((const __m128i*) & src[8 * line + i]);
        in10 = _mm_loadu_si128((const __m128i*) & src[10 * line + i]);
        in12 = _mm_loadu_si128((const __m128i*) & src[12 * line + i]);
        in14 = _mm_loadu_si128((const __m128i*) & src[14 * line + i]);
        in16 = _mm_loadu_si128((const __m128i*) & src[16 * line + i]);
        in18 = _mm_loadu_si128((const __m128i*) & src[18 * line + i]);
        in20 = _mm_loadu_si128((const __m128i*) & src[20 * line + i]);
        in22 = _mm_loadu_si128((const __m128i*) & src[22 * line + i]);
        in24 = _mm_loadu_si128((const __m128i*) & src[24 * line + i]);
        in26 = _mm_loadu_si128((const __m128i*) & src[26 * line + i]);
        in28 = _mm_loadu_si128((const __m128i*) & src[28 * line + i]);
        in30 = _mm_loadu_si128((const __m128i*) & src[30 * line + i]);

        ss00 = _mm_unpacklo_epi16(in02, in06);
        ss01 = _mm_unpacklo_epi16(in10, in14);
        ss02 = _mm_unpacklo_epi16(in18, in22);
        ss03 = _mm_unpacklo_epi16(in26, in30);
        ss04 = _mm_unpacklo_epi16(in04, in12);
        ss05 = _mm_unpacklo_epi16(in20, in28);
        ss06 = _mm_unpacklo_epi16(in08, in24);
        ss07 = _mm_unpacklo_epi16(in00, in16);

        ss08 = _mm_unpackhi_epi16(in02, in06);
        ss09 = _mm_unpackhi_epi16(in10, in14);
        ss10 = _mm_unpackhi_epi16(in18, in22);
        ss11 = _mm_unpackhi_epi16(in26, in30);
        ss12 = _mm_unpackhi_epi16(in04, in12);
        ss13 = _mm_unpackhi_epi16(in20, in28);
        ss14 = _mm_unpackhi_epi16(in08, in24);
        ss15 = _mm_unpackhi_epi16(in00, in16);

        {
            const __m256i T_00_08 = _mm256_set_m128i(ss08, ss00);
            const __m256i T_00_09 = _mm256_set_m128i(ss09, ss01);
            const __m256i T_00_10 = _mm256_set_m128i(ss10, ss02);
            const __m256i T_00_11 = _mm256_set_m128i(ss11, ss03);
            const __m256i T_00_12 = _mm256_set_m128i(ss12, ss04);
            const __m256i T_00_13 = _mm256_set_m128i(ss13, ss05);
            const __m256i T_00_14 = _mm256_set_m128i(ss14, ss06);
            const __m256i T_00_15 = _mm256_set_m128i(ss15, ss07);

#define COMPUTE_ROW(c0206, c1014, c1822, c2630, row) \
    T00 = _mm256_add_epi32(_mm256_madd_epi16(T_00_08, c0206), _mm256_madd_epi16(T_00_09, c1014)); \
    T01 = _mm256_add_epi32(_mm256_madd_epi16(T_00_10, c1822), _mm256_madd_epi16(T_00_11, c2630)); \
    row = _mm256_add_epi32(T00, T01);
                COMPUTE_ROW(p87_p90, p70_p80, p43_p57, p09_p26, EO0)
                COMPUTE_ROW(p57_p87, n43_p09, n90_n80, n26_n70, EO1)
                COMPUTE_ROW(p09_p80, n87_n70, p57_n26, p43_p90, EO2)
                COMPUTE_ROW(n43_p70, p09_n87, p26_p90, n57_n80, EO3)
                COMPUTE_ROW(n80_p57, p90_n26, n87_n09, p70_p43, EO4)
                COMPUTE_ROW(n90_p43, p26_p57, p70_n87, n80_p09, EO5)
                COMPUTE_ROW(n70_p26, n80_p90, p09_p43, p87_n57, EO6)
                COMPUTE_ROW(n26_p09, n57_p43, n80_p70, n90_p87, EO7)

#undef COMPUTE_ROW

            {
                const __m256i EEO0 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, p75_p89), _mm256_madd_epi16(T_00_13, p18_p50));
                const __m256i EEO1 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, n18_p75), _mm256_madd_epi16(T_00_13, n50_n89));
                const __m256i EEO2 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, n89_p50), _mm256_madd_epi16(T_00_13, p75_p18));
                const __m256i EEO3 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, n50_p18), _mm256_madd_epi16(T_00_13, n89_p75));

                const __m256i EEEO0 = _mm256_madd_epi16(T_00_14, p35_p84);
                const __m256i EEEO1 = _mm256_madd_epi16(T_00_14, n84_p35);

                const __m256i EEEE0 = _mm256_madd_epi16(T_00_15, p64_p64);
                const __m256i EEEE1 = _mm256_madd_epi16(T_00_15, n64_p64);

                const __m256i EEE0 = _mm256_add_epi32(EEEE0, EEEO0);          // EEE0 = EEEE0 + EEEO0
                const __m256i EEE1 = _mm256_add_epi32(EEEE1, EEEO1);          // EEE1 = EEEE1 + EEEO1
                const __m256i EEE3 = _mm256_sub_epi32(EEEE0, EEEO0);          // EEE2 = EEEE0 - EEEO0
                const __m256i EEE2 = _mm256_sub_epi32(EEEE1, EEEO1);          // EEE3 = EEEE1 - EEEO1

                const __m256i EE0 = _mm256_add_epi32(EEE0, EEO0);          // EE0 = EEE0 + EEO0
                const __m256i EE1 = _mm256_add_epi32(EEE1, EEO1);          // EE1 = EEE1 + EEO1
                const __m256i EE2 = _mm256_add_epi32(EEE2, EEO2);          // EE2 = EEE0 + EEO0
                const __m256i EE3 = _mm256_add_epi32(EEE3, EEO3);          // EE3 = EEE1 + EEO1
                const __m256i EE7 = _mm256_sub_epi32(EEE0, EEO0);          // EE7 = EEE0 - EEO0
                const __m256i EE6 = _mm256_sub_epi32(EEE1, EEO1);          // EE6 = EEE1 - EEO1
                const __m256i EE5 = _mm256_sub_epi32(EEE2, EEO2);          // EE5 = EEE0 - EEO0
                const __m256i EE4 = _mm256_sub_epi32(EEE3, EEO3);          // EE4 = EEE1 - EEO1

                const __m256i E0 = _mm256_add_epi32(EE0, EO0);          // E0 = EE0 + EO0
                const __m256i E1 = _mm256_add_epi32(EE1, EO1);          // E1 = EE1 + EO1
                const __m256i E2 = _mm256_add_epi32(EE2, EO2);          // E2 = EE2 + EO2
                const __m256i E3 = _mm256_add_epi32(EE3, EO3);          // E3 = EE3 + EO3
                const __m256i E4 = _mm256_add_epi32(EE4, EO4);          // E4 =
                const __m256i E5 = _mm256_add_epi32(EE5, EO5);          // E5 =
                const __m256i E6 = _mm256_add_epi32(EE6, EO6);          // E6 =
                const __m256i E7 = _mm256_add_epi32(EE7, EO7);          // E7 =
                const __m256i EF = _mm256_sub_epi32(EE0, EO0);          // EF = EE0 - EO0
                const __m256i EE = _mm256_sub_epi32(EE1, EO1);          // EE = EE1 - EO1
                const __m256i ED = _mm256_sub_epi32(EE2, EO2);          // ED = EE2 - EO2
                const __m256i EC = _mm256_sub_epi32(EE3, EO3);          // EC = EE3 - EO3
                const __m256i EB = _mm256_sub_epi32(EE4, EO4);          // EB =
                const __m256i EA = _mm256_sub_epi32(EE5, EO5);          // EA =
                const __m256i E9 = _mm256_sub_epi32(EE6, EO6);          // E9 =
                const __m256i E8 = _mm256_sub_epi32(EE7, EO7);          // E8 =

                const __m256i T10 = _mm256_add_epi32(E0, c32_off);         // E0 + off
                const __m256i T11 = _mm256_add_epi32(E1, c32_off);         // E1 + off
                const __m256i T12 = _mm256_add_epi32(E2, c32_off);         // E2 + off
                const __m256i T13 = _mm256_add_epi32(E3, c32_off);         // E3 + off
                const __m256i T14 = _mm256_add_epi32(E4, c32_off);         // E4 + off
                const __m256i T15 = _mm256_add_epi32(E5, c32_off);         // E5 + off
                const __m256i T16 = _mm256_add_epi32(E6, c32_off);         // E6 + off
                const __m256i T17 = _mm256_add_epi32(E7, c32_off);         // E7 + off
                const __m256i T18 = _mm256_add_epi32(E8, c32_off);         // E8 + off
                const __m256i T19 = _mm256_add_epi32(E9, c32_off);         // E9 + off
                const __m256i T1A = _mm256_add_epi32(EA, c32_off);         // E10 + off
                const __m256i T1B = _mm256_add_epi32(EB, c32_off);         // E11 + off
                const __m256i T1C = _mm256_add_epi32(EC, c32_off);         // E12 + off
                const __m256i T1D = _mm256_add_epi32(ED, c32_off);         // E13 + off
                const __m256i T1E = _mm256_add_epi32(EE, c32_off);         // E14 + off
                const __m256i T1F = _mm256_add_epi32(EF, c32_off);         // E15 + off

                __m256i T2_00 = _mm256_add_epi32(T10, O00);          // E0 + O0 + off
                __m256i T2_01 = _mm256_add_epi32(T11, O01);          // E1 + O1 + off
                __m256i T2_02 = _mm256_add_epi32(T12, O02);          // E2 + O2 + off
                __m256i T2_03 = _mm256_add_epi32(T13, O03);          // E3 + O3 + off
                __m256i T2_04 = _mm256_add_epi32(T14, O04);          // E4
                __m256i T2_05 = _mm256_add_epi32(T15, O05);          // E5
                __m256i T2_06 = _mm256_add_epi32(T16, O06);          // E6
                __m256i T2_07 = _mm256_add_epi32(T17, O07);          // E7
                __m256i T2_08 = _mm256_add_epi32(T18, O08);          // E8
                __m256i T2_09 = _mm256_add_epi32(T19, O09);          // E9
                __m256i T2_10 = _mm256_add_epi32(T1A, O10);          // E10
                __m256i T2_11 = _mm256_add_epi32(T1B, O11);          // E11
                __m256i T2_12 = _mm256_add_epi32(T1C, O12);          // E12
                __m256i T2_13 = _mm256_add_epi32(T1D, O13);          // E13
                __m256i T2_14 = _mm256_add_epi32(T1E, O14);          // E14
                __m256i T2_15 = _mm256_add_epi32(T1F, O15);          // E15
                __m256i T2_31 = _mm256_sub_epi32(T10, O00);          // E0 - O0 + off
                __m256i T2_30 = _mm256_sub_epi32(T11, O01);          // E1 - O1 + off
                __m256i T2_29 = _mm256_sub_epi32(T12, O02);          // E2 - O2 + off
                __m256i T2_28 = _mm256_sub_epi32(T13, O03);          // E3 - O3 + off
                __m256i T2_27 = _mm256_sub_epi32(T14, O04);          // E4
                __m256i T2_26 = _mm256_sub_epi32(T15, O05);          // E5
                __m256i T2_25 = _mm256_sub_epi32(T16, O06);          // E6
                __m256i T2_24 = _mm256_sub_epi32(T17, O07);          // E7
                __m256i T2_23 = _mm256_sub_epi32(T18, O08);          //
                __m256i T2_22 = _mm256_sub_epi32(T19, O09);          //
                __m256i T2_21 = _mm256_sub_epi32(T1A, O10);          //
                __m256i T2_20 = _mm256_sub_epi32(T1B, O11);          //
                __m256i T2_19 = _mm256_sub_epi32(T1C, O12);          //
                __m256i T2_18 = _mm256_sub_epi32(T1D, O13);          //
                __m256i T2_17 = _mm256_sub_epi32(T1E, O14);          //
                __m256i T2_16 = _mm256_sub_epi32(T1F, O15);          //

                T2_00 = _mm256_srai_epi32(T2_00, shift);             // [30 20 10 00]
                T2_01 = _mm256_srai_epi32(T2_01, shift);             // [31 21 11 01]
                T2_02 = _mm256_srai_epi32(T2_02, shift);             // [32 22 12 02]
                T2_03 = _mm256_srai_epi32(T2_03, shift);             // [33 23 13 03]
                T2_04 = _mm256_srai_epi32(T2_04, shift);             // [33 24 14 04]
                T2_05 = _mm256_srai_epi32(T2_05, shift);             // [35 25 15 05]
                T2_06 = _mm256_srai_epi32(T2_06, shift);             // [36 26 16 06]
                T2_07 = _mm256_srai_epi32(T2_07, shift);             // [37 27 17 07]
                T2_08 = _mm256_srai_epi32(T2_08, shift);             // [30 20 10 00] x8
                T2_09 = _mm256_srai_epi32(T2_09, shift);             // [31 21 11 01] x9
                T2_10 = _mm256_srai_epi32(T2_10, shift);             // [32 22 12 02] xA
                T2_11 = _mm256_srai_epi32(T2_11, shift);             // [33 23 13 03] xB
                T2_12 = _mm256_srai_epi32(T2_12, shift);             // [33 24 14 04] xC
                T2_13 = _mm256_srai_epi32(T2_13, shift);             // [35 25 15 05] xD
                T2_14 = _mm256_srai_epi32(T2_14, shift);             // [36 26 16 06] xE
                T2_15 = _mm256_srai_epi32(T2_15, shift);             // [37 27 17 07] xF
                T2_16 = _mm256_srai_epi32(T2_16, shift);             // [30 20 10 00]
                T2_17 = _mm256_srai_epi32(T2_17, shift);             // [31 21 11 01]
                T2_18 = _mm256_srai_epi32(T2_18, shift);             // [32 22 12 02]
                T2_19 = _mm256_srai_epi32(T2_19, shift);             // [33 23 13 03]
                T2_20 = _mm256_srai_epi32(T2_20, shift);             // [33 24 14 04]
                T2_21 = _mm256_srai_epi32(T2_21, shift);             // [35 25 15 05]
                T2_22 = _mm256_srai_epi32(T2_22, shift);             // [36 26 16 06]
                T2_23 = _mm256_srai_epi32(T2_23, shift);             // [37 27 17 07]
                T2_24 = _mm256_srai_epi32(T2_24, shift);             // [30 20 10 00] x8
                T2_25 = _mm256_srai_epi32(T2_25, shift);             // [31 21 11 01] x9
                T2_26 = _mm256_srai_epi32(T2_26, shift);             // [32 22 12 02] xA
                T2_27 = _mm256_srai_epi32(T2_27, shift);             // [33 23 13 03] xB
                T2_28 = _mm256_srai_epi32(T2_28, shift);             // [33 24 14 04] xC
                T2_29 = _mm256_srai_epi32(T2_29, shift);             // [35 25 15 05] xD
                T2_30 = _mm256_srai_epi32(T2_30, shift);             // [36 26 16 06] xE
                T2_31 = _mm256_srai_epi32(T2_31, shift);             // [37 27 17 07] xF

                //transpose 32x8 -> 8x32.
                TRANSPOSE_16x8_32BIT_16BIT(T2_00, T2_01, T2_02, T2_03, T2_04, T2_05, T2_06, T2_07,
                    T2_08, T2_09, T2_10, T2_11, T2_12, T2_13, T2_14, T2_15, res00, res02, res04, res06, res08, res10, res12, res14);
                TRANSPOSE_16x8_32BIT_16BIT(T2_16, T2_17, T2_18, T2_19, T2_20, T2_21, T2_22, T2_23,
                    T2_24, T2_25, T2_26, T2_27, T2_28, T2_29, T2_30, T2_31, res01, res03, res05, res07, res09, res11, res13, res15);

            }
            if (bit_depth == 8) { // clip
                __m256i max_val = _mm256_set1_epi16(32767);
                __m256i min_val = _mm256_set1_epi16(-32767);

                res00 = _mm256_min_epi16(res00, max_val);
                res01 = _mm256_min_epi16(res01, max_val);
                res02 = _mm256_min_epi16(res02, max_val);
                res03 = _mm256_min_epi16(res03, max_val);
                res00 = _mm256_max_epi16(res00, min_val);
                res01 = _mm256_max_epi16(res01, min_val);
                res02 = _mm256_max_epi16(res02, min_val);
                res03 = _mm256_max_epi16(res03, min_val);
                res04 = _mm256_min_epi16(res04, max_val);
                res05 = _mm256_min_epi16(res05, max_val);
                res06 = _mm256_min_epi16(res06, max_val);
                res07 = _mm256_min_epi16(res07, max_val);
                res04 = _mm256_max_epi16(res04, min_val);
                res05 = _mm256_max_epi16(res05, min_val);
                res06 = _mm256_max_epi16(res06, min_val);
                res07 = _mm256_max_epi16(res07, min_val);

                res08 = _mm256_min_epi16(res08, max_val);
                res09 = _mm256_min_epi16(res09, max_val);
                res10 = _mm256_min_epi16(res10, max_val);
                res11 = _mm256_min_epi16(res11, max_val);
                res08 = _mm256_max_epi16(res08, min_val);
                res09 = _mm256_max_epi16(res09, min_val);
                res10 = _mm256_max_epi16(res10, min_val);
                res11 = _mm256_max_epi16(res11, min_val);
                res12 = _mm256_min_epi16(res12, max_val);
                res13 = _mm256_min_epi16(res13, max_val);
                res14 = _mm256_min_epi16(res14, max_val);
                res15 = _mm256_min_epi16(res15, max_val);
                res12 = _mm256_max_epi16(res12, min_val);
                res13 = _mm256_max_epi16(res13, min_val);
                res14 = _mm256_max_epi16(res14, min_val);
                res15 = _mm256_max_epi16(res15, min_val);

            }
        }
        _mm256_storeu_si256((__m256i*) & dst[0 * 16], res00);
        _mm256_storeu_si256((__m256i*) & dst[1 * 16], res01);
        _mm256_storeu_si256((__m256i*) & dst[2 * 16], res02);
        _mm256_storeu_si256((__m256i*) & dst[3 * 16], res03);
        _mm256_storeu_si256((__m256i*) & dst[4 * 16], res04);
        _mm256_storeu_si256((__m256i*) & dst[5 * 16], res05);
        _mm256_storeu_si256((__m256i*) & dst[6 * 16], res06);
        _mm256_storeu_si256((__m256i*) & dst[7 * 16], res07);
        _mm256_storeu_si256((__m256i*) & dst[8 * 16], res08);
        _mm256_storeu_si256((__m256i*) & dst[9 * 16], res09);
        _mm256_storeu_si256((__m256i*) & dst[10 * 16], res10);
        _mm256_storeu_si256((__m256i*) & dst[11 * 16], res11);
        _mm256_storeu_si256((__m256i*) & dst[12 * 16], res12);
        _mm256_storeu_si256((__m256i*) & dst[13 * 16], res13);
        _mm256_storeu_si256((__m256i*) & dst[14 * 16], res14);
        _mm256_storeu_si256((__m256i*) & dst[15 * 16], res15);

        dst += 8 * 32;  // 8rows
    }
}

void itx_pb64_avx(s16* src, s16* dst, int shift, int line)
{
    int i_src = line;
    int bit_depth = 10;
    // O[32] coeffs
    const __m256i n69_p62 = _mm256_set1_epi32(-4521922);
    const __m256i p74_n56 = _mm256_set1_epi32(4915144);
    const __m256i n79_p48 = _mm256_set1_epi32(-5177296);
    const __m256i p83_n41 = _mm256_set1_epi32(5504983);
    const __m256i n86_p33 = _mm256_set1_epi32(-5636063);
    const __m256i p88_n24 = _mm256_set1_epi32(5832680);
    const __m256i n90_p15 = _mm256_set1_epi32(-5898225);
    const __m256i p90_n07 = _mm256_set1_epi32(5963769);
    const __m256i n76_p59 = _mm256_set1_epi32(-4980677);
    const __m256i p87_n37 = _mm256_set1_epi32(5767131);
    const __m256i n90_p11 = _mm256_set1_epi32(-5898229);
    const __m256i p86_p15 = _mm256_set1_epi32(5636111);
    const __m256i n74_n41 = _mm256_set1_epi32(-4784169);
    const __m256i p56_p62 = _mm256_set1_epi32(3670078);
    const __m256i n33_n79 = _mm256_set1_epi32(-2097231);
    const __m256i p07_p88 = _mm256_set1_epi32(458840);
    const __m256i n83_p56 = _mm256_set1_epi32(-5439432);
    const __m256i p90_n15 = _mm256_set1_epi32(5963761);
    const __m256i n76_n28 = _mm256_set1_epi32(-4915228);
    const __m256i p45_p66 = _mm256_set1_epi32(2949186);
    const __m256i n02_n87 = _mm256_set1_epi32(-65623);
    const __m256i n41_p88 = _mm256_set1_epi32(-2686888);
    const __m256i p74_n69 = _mm256_set1_epi32(4915131);
    const __m256i n90_p33 = _mm256_set1_epi32(-5898207);
    const __m256i n87_p52 = _mm256_set1_epi32(-5701580);
    const __m256i p83_p07 = _mm256_set1_epi32(5439495);
    const __m256i n41_n62 = _mm256_set1_epi32(-2621502);
    const __m256i n20_p90 = _mm256_set1_epi32(-1310630);
    const __m256i p71_n76 = _mm256_set1_epi32(4718516);
    const __m256i n90_p28 = _mm256_set1_epi32(-5898212);
    const __m256i p69_p33 = _mm256_set1_epi32(4522017);
    const __m256i n15_n79 = _mm256_set1_epi32(-917583);
    const __m256i n90_p48 = _mm256_set1_epi32(-5898192);
    const __m256i p66_p28 = _mm256_set1_epi32(4325404);
    const __m256i p07_n84 = _mm256_set1_epi32(524204);
    const __m256i n74_p79 = _mm256_set1_epi32(-4849585);
    const __m256i p87_n15 = _mm256_set1_epi32(5767153);
    const __m256i n37_n59 = _mm256_set1_epi32(-2359355);
    const __m256i n41_p90 = _mm256_set1_epi32(-2686886);
    const __m256i p88_n56 = _mm256_set1_epi32(5832648);
    const __m256i n90_p45 = _mm256_set1_epi32(-5898195);
    const __m256i p41_p48 = _mm256_set1_epi32(2687024);
    const __m256i p52_n90 = _mm256_set1_epi32(3473318);
    const __m256i n90_p37 = _mm256_set1_epi32(-5898203);
    const __m256i p33_p56 = _mm256_set1_epi32(2162744);
    const __m256i p59_n89 = _mm256_set1_epi32(3932071);
    const __m256i n88_p28 = _mm256_set1_epi32(-5767140);
    const __m256i p24_p62 = _mm256_set1_epi32(1572926);
    const __m256i n89_p41 = _mm256_set1_epi32(-5832663);
    const __m256i p11_p66 = _mm256_set1_epi32(720962);
    const __m256i p83_n79 = _mm256_set1_epi32(5504945);
    const __m256i n59_n20 = _mm256_set1_epi32(-3801108);
    const __m256i n48_p90 = _mm256_set1_epi32(-3145638);
    const __m256i p87_n33 = _mm256_set1_epi32(5767135);
    const __m256i n02_n71 = _mm256_set1_epi32(-65607);
    const __m256i n86_p74 = _mm256_set1_epi32(-5636022);
    const __m256i n86_p37 = _mm256_set1_epi32(-5636059);
    const __m256i n20_p79 = _mm256_set1_epi32(-1310641);
    const __m256i p90_n52 = _mm256_set1_epi32(5963724);
    const __m256i p02_n69 = _mm256_set1_epi32(196539);
    const __m256i n90_p66 = _mm256_set1_epi32(-5898174);
    const __m256i p15_p56 = _mm256_set1_epi32(983096);
    const __m256i p87_n76 = _mm256_set1_epi32(5767092);
    const __m256i n33_n41 = _mm256_set1_epi32(-2097193);
    const __m256i n81_p33 = _mm256_set1_epi32(-5308383);
    const __m256i n48_p87 = _mm256_set1_epi32(-3145641);
    const __m256i p71_n15 = _mm256_set1_epi32(4718577);
    const __m256i p62_n90 = _mm256_set1_epi32(4128678);
    const __m256i n59_n02 = _mm256_set1_epi32(-3801090);
    const __m256i n74_p90 = _mm256_set1_epi32(-4849574);
    const __m256i p45_p20 = _mm256_set1_epi32(2949140);
    const __m256i p83_n86 = _mm256_set1_epi32(5504938);
    const __m256i n74_p28 = _mm256_set1_epi32(-4849636);
    const __m256i n71_p90 = _mm256_set1_epi32(-4652966);
    const __m256i p33_p24 = _mm256_set1_epi32(2162712);
    const __m256i p90_n76 = _mm256_set1_epi32(5963700);
    const __m256i p20_n69 = _mm256_set1_epi32(1376187);
    const __m256i n79_p37 = _mm256_set1_epi32(-5177307);
    const __m256i n66_p90 = _mm256_set1_epi32(-4325286);
    const __m256i p41_p15 = _mm256_set1_epi32(2686991);
    const __m256i n66_p24 = _mm256_set1_epi32(-4325352);
    const __m256i n86_p88 = _mm256_set1_epi32(-5636008);
    const __m256i n15_p59 = _mm256_set1_epi32(-982981);
    const __m256i p71_n33 = _mm256_set1_epi32(4718559);
    const __m256i p83_n90 = _mm256_set1_epi32(5504934);
    const __m256i p07_n52 = _mm256_set1_epi32(524236);
    const __m256i n76_p41 = _mm256_set1_epi32(-4980695);
    const __m256i n79_p90 = _mm256_set1_epi32(-5177254);
    const __m256i n56_p20 = _mm256_set1_epi32(-3669996);
    const __m256i n90_p81 = _mm256_set1_epi32(-5898159);
    const __m256i n59_p83 = _mm256_set1_epi32(-3866541);
    const __m256i p15_p24 = _mm256_set1_epi32(983064);
    const __m256i p79_n52 = _mm256_set1_epi32(5242828);
    const __m256i p84_n90 = _mm256_set1_epi32(5570470);
    const __m256i p28_n62 = _mm256_set1_epi32(1900482);
    const __m256i n48_p11 = _mm256_set1_epi32(-3145717);
    const __m256i n45_p15 = _mm256_set1_epi32(-2949105);
    const __m256i n84_p69 = _mm256_set1_epi32(-5504955);
    const __m256i n86_p90 = _mm256_set1_epi32(-5636006);
    const __m256i n48_p71 = _mm256_set1_epi32(-3145657);
    const __m256i p11_p20 = _mm256_set1_epi32(720916);
    const __m256i p66_n41 = _mm256_set1_epi32(4390871);
    const __m256i p90_n83 = _mm256_set1_epi32(5963693);
    const __m256i p74_n87 = _mm256_set1_epi32(4915113);
    const __m256i n33_p11 = _mm256_set1_epi32(-2162677);
    const __m256i n69_p52 = _mm256_set1_epi32(-4521932);
    const __m256i n88_p81 = _mm256_set1_epi32(-5767087);
    const __m256i n87_p90 = _mm256_set1_epi32(-5701542);
    const __m256i n66_p79 = _mm256_set1_epi32(-4325297);
    const __m256i n28_p48 = _mm256_set1_epi32(-1834960);
    const __m256i p15_p07 = _mm256_set1_epi32(983047);
    const __m256i p56_n37 = _mm256_set1_epi32(3735515);
    const __m256i n20_p07 = _mm256_set1_epi32(-1310713);
    const __m256i n66_p56 = _mm256_set1_epi32(-4325320);
    const __m256i n81_p74 = _mm256_set1_epi32(-5308342);
    const __m256i n89_p86 = _mm256_set1_epi32(-5832618);
    const __m256i n90_p90 = _mm256_set1_epi32(-5898150);
    const __m256i n83_p87 = _mm256_set1_epi32(-5439401);
    const __m256i n69_p76 = _mm256_set1_epi32(-4521908);
    const __m256i n07_p02 = _mm256_set1_epi32(-458750);
    const __m256i n15_p11 = _mm256_set1_epi32(-983029);
    const __m256i n24_p20 = _mm256_set1_epi32(-1572844);
    const __m256i n41_p37 = _mm256_set1_epi32(-2686939);
    const __m256i n48_p45 = _mm256_set1_epi32(-3145683);
    const __m256i n56_p52 = _mm256_set1_epi32(-3669964);
    const __m256i n62_p59 = _mm256_set1_epi32(-4063173);
    const __m256i p90_p90 = _mm256_set1_epi32(5898330);
    const __m256i p89_p90 = _mm256_set1_epi32(5832794);
    const __m256i p87_p88 = _mm256_set1_epi32(5701720);
    const __m256i p84_p86 = _mm256_set1_epi32(5505110);
    const __m256i p81_p83 = _mm256_set1_epi32(5308499);
    const __m256i p76_p79 = _mm256_set1_epi32(4980815);
    const __m256i p71_p74 = _mm256_set1_epi32(4653130);
    const __m256i p66_p69 = _mm256_set1_epi32(4325445);
    const __m256i p88_p90 = _mm256_set1_epi32(5767258);
    const __m256i p79_p84 = _mm256_set1_epi32(5177428);
    const __m256i p62_p71 = _mm256_set1_epi32(4063303);
    const __m256i p41_p52 = _mm256_set1_epi32(2687028);
    const __m256i p15_p28 = _mm256_set1_epi32(983068);
    const __m256i n11_p02 = _mm256_set1_epi32(-720894);
    const __m256i n37_n24 = _mm256_set1_epi32(-2359320);
    const __m256i n59_n48 = _mm256_set1_epi32(-3801136);
    const __m256i p84_p90 = _mm256_set1_epi32(5505114);
    const __m256i p59_p74 = _mm256_set1_epi32(3866698);
    const __m256i p20_p41 = _mm256_set1_epi32(1310761);
    const __m256i n24_n02 = _mm256_set1_epi32(-1507330);
    const __m256i n62_n45 = _mm256_set1_epi32(-3997741);
    const __m256i n86_n76 = _mm256_set1_epi32(-5570636);
    const __m256i n89_n90 = _mm256_set1_epi32(-5767258);
    const __m256i n71_n83 = _mm256_set1_epi32(-4587603);
    const __m256i p79_p89 = _mm256_set1_epi32(5177433);
    const __m256i p33_p59 = _mm256_set1_epi32(2162747);
    const __m256i n28_p02 = _mm256_set1_epi32(-1835006);
    const __m256i n76_n56 = _mm256_set1_epi32(-4915256);
    const __m256i n90_n88 = _mm256_set1_epi32(-5832792);
    const __m256i n62_n81 = _mm256_set1_epi32(-3997777);
    const __m256i n07_n37 = _mm256_set1_epi32(-393253);
    const __m256i p52_p24 = _mm256_set1_epi32(3407896);
    const __m256i p71_p88 = _mm256_set1_epi32(4653144);
    const __m256i p02_p41 = _mm256_set1_epi32(131113);
    const __m256i n69_n37 = _mm256_set1_epi32(-4456485);
    const __m256i n89_n87 = _mm256_set1_epi32(-5767255);
    const __m256i n45_n74 = _mm256_set1_epi32(-2883658);
    const __m256i p33_n07 = _mm256_set1_epi32(2228217);
    const __m256i p86_p66 = _mm256_set1_epi32(5636162);
    const __m256i p76_p90 = _mm256_set1_epi32(4980826);
    const __m256i p62_p87 = _mm256_set1_epi32(4063319);
    const __m256i n28_p20 = _mm256_set1_epi32(-1834988);
    const __m256i n89_n69 = _mm256_set1_epi32(-5767237);
    const __m256i n56_n84 = _mm256_set1_epi32(-3604564);
    const __m256i p37_n11 = _mm256_set1_epi32(2490357);
    const __m256i p90_p74 = _mm256_set1_epi32(5898314);
    const __m256i p48_p81 = _mm256_set1_epi32(3145809);
    const __m256i n45_p02 = _mm256_set1_epi32(-2949118);
    const __m256i p52_p86 = _mm256_set1_epi32(3407958);
    const __m256i n56_n02 = _mm256_set1_epi32(-3604482);
    const __m256i n84_n87 = _mm256_set1_epi32(-5439575);
    const __m256i p07_n48 = _mm256_set1_epi32(524240);
    const __m256i p88_p59 = _mm256_set1_epi32(5767227);
    const __m256i p45_p83 = _mm256_set1_epi32(2949203);
    const __m256i n62_n11 = _mm256_set1_epi32(-3997707);
    const __m256i n81_n89 = _mm256_set1_epi32(-5242969);
    const __m256i p41_p84 = _mm256_set1_epi32(2687060);
    const __m256i n76_n24 = _mm256_set1_epi32(-4915224);
    const __m256i n56_n89 = _mm256_set1_epi32(-3604569);
    const __m256i p66_p07 = _mm256_set1_epi32(4325383);
    const __m256i p69_p90 = _mm256_set1_epi32(4522074);
    const __m256i n52_p11 = _mm256_set1_epi32(-3407861);
    const __m256i n79_n88 = _mm256_set1_epi32(-5111896);
    const __m256i p28_p83 = _mm256_set1_epi32(1835091);
    const __m256i n88_n45 = _mm256_set1_epi32(-5701677);
    const __m256i n11_n74 = _mm256_set1_epi32(-655434);
    const __m256i p90_p59 = _mm256_set1_epi32(5898299);
    const __m256i n07_p62 = _mm256_set1_epi32(-458690);
    const __m256i n89_n71 = _mm256_set1_epi32(-5767239);
    const __m256i p24_n48 = _mm256_set1_epi32(1638352);
    const __m256i p84_p81 = _mm256_set1_epi32(5505105);
    const __m256i p15_p81 = _mm256_set1_epi32(983121);
    const __m256i n90_n62 = _mm256_set1_epi32(-5832766);
    const __m256i p37_n45 = _mm256_set1_epi32(2490323);
    const __m256i p69_p88 = _mm256_set1_epi32(4522072);
    const __m256i n76_n07 = _mm256_set1_epi32(-4915207);
    const __m256i n24_n84 = _mm256_set1_epi32(-1507412);
    const __m256i p90_p56 = _mm256_set1_epi32(5898296);
    const __m256i n28_p52 = _mm256_set1_epi32(-1834956);
    const __m256i p02_p79 = _mm256_set1_epi32(131151);
    const __m256i n81_n76 = _mm256_set1_epi32(-5242956);
    const __m256i p74_n07 = _mm256_set1_epi32(4915193);
    const __m256i p11_p83 = _mm256_set1_epi32(720979);
    const __m256i n84_n71 = _mm256_set1_epi32(-5439559);
    const __m256i p69_n15 = _mm256_set1_epi32(4587505);
    const __m256i p20_p86 = _mm256_set1_epi32(1310806);
    const __m256i n87_n66 = _mm256_set1_epi32(-5636162);
    const __m256i n11_p76 = _mm256_set1_epi32(-720820);
    const __m256i n62_n86 = _mm256_set1_epi32(-3997782);
    const __m256i p90_p33 = _mm256_set1_epi32(5898273);
    const __m256i n52_p45 = _mm256_set1_epi32(-3407827);
    const __m256i n24_n89 = _mm256_set1_epi32(-1507417);
    const __m256i p83_p69 = _mm256_set1_epi32(5439557);
    const __m256i n81_p02 = _mm256_set1_epi32(-5308414);
    const __m256i p20_n71 = _mm256_set1_epi32(1376185);
    const __m256i n24_p74 = _mm256_set1_epi32(-1572790);
    const __m256i n37_n90 = _mm256_set1_epi32(-2359386);
    const __m256i p81_p66 = _mm256_set1_epi32(5308482);
    const __m256i n88_n11 = _mm256_set1_epi32(-5701643);
    const __m256i p56_n48 = _mm256_set1_epi32(3735504);
    const __m256i p02_p86 = _mm256_set1_epi32(131158);
    const __m256i n59_n84 = _mm256_set1_epi32(-3801172);
    const __m256i p89_p45 = _mm256_set1_epi32(5832749);
    const __m256i n37_p71 = _mm256_set1_epi32(-2424761);
    const __m256i n07_n89 = _mm256_set1_epi32(-393305);
    const __m256i p48_p86 = _mm256_set1_epi32(3145814);
    const __m256i n79_n62 = _mm256_set1_epi32(-5111870);
    const __m256i p90_p24 = _mm256_set1_epi32(5898264);
    const __m256i n81_p20 = _mm256_set1_epi32(-5308396);
    const __m256i p52_n59 = _mm256_set1_epi32(3473349);
    const __m256i n11_p84 = _mm256_set1_epi32(-720812);
    const __m256i n48_p69 = _mm256_set1_epi32(-3145659);
    const __m256i p24_n83 = _mm256_set1_epi32(1638317);
    const __m256i p02_p90 = _mm256_set1_epi32(131162);
    const __m256i n28_n89 = _mm256_set1_epi32(-1769561);
    const __m256i p52_p81 = _mm256_set1_epi32(3407953);
    const __m256i n71_n66 = _mm256_set1_epi32(-4587586);
    const __m256i p84_p45 = _mm256_set1_epi32(5505069);
    const __m256i n90_n20 = _mm256_set1_epi32(-5832724);
    const __m256i n59_p66 = _mm256_set1_epi32(-3866558);
    const __m256i p52_n71 = _mm256_set1_epi32(3473337);
    const __m256i n45_p76 = _mm256_set1_epi32(-2949044);
    const __m256i p37_n81 = _mm256_set1_epi32(2490287);
    const __m256i n28_p84 = _mm256_set1_epi32(-1834924);
    const __m256i p20_n87 = _mm256_set1_epi32(1376169);
    const __m256i n11_p89 = _mm256_set1_epi32(-720807);
    const __m256i p02_n90 = _mm256_set1_epi32(196518);
    const __m256i p85_p88 = _mm256_set1_epi32(5570648);
    const __m256i p78_p82 = _mm256_set1_epi32(5111890);
    const __m256i p67_p73 = _mm256_set1_epi32(4390985);
    const __m256i p82_p90 = _mm256_set1_epi32(5374042);
    const __m256i p47_p67 = _mm256_set1_epi32(3080259);
    const __m256i n04_p22 = _mm256_set1_epi32(-262122);
    const __m256i n54_n30 = _mm256_set1_epi32(-3473438);
    const __m256i p67_p88 = _mm256_set1_epi32(4391000);
    const __m256i n13_p30 = _mm256_set1_epi32(-851938);
    const __m256i n82_n54 = _mm256_set1_epi32(-5308470);
    const __m256i n78_n90 = _mm256_set1_epi32(-5046362);
    const __m256i p47_p85 = _mm256_set1_epi32(3080277);
    const __m256i n67_n13 = _mm256_set1_epi32(-4325389);
    const __m256i n73_n90 = _mm256_set1_epi32(-4718682);
    const __m256i p39_n22 = _mm256_set1_epi32(2621418);
    const __m256i p22_p82 = _mm256_set1_epi32(1441874);
    const __m256i n90_n54 = _mm256_set1_epi32(-5832758);
    const __m256i p13_n61 = _mm256_set1_epi32(917443);
    const __m256i p85_p78 = _mm256_set1_epi32(5570638);
    const __m256i n04_p78 = _mm256_set1_epi32(-262066);
    const __m256i n73_n82 = _mm256_set1_epi32(-4718674);
    const __m256i p85_p13 = _mm256_set1_epi32(5570573);
    const __m256i n22_p67 = _mm256_set1_epi32(-1441725);
    const __m256i n30_p73 = _mm256_set1_epi32(-1966007);
    const __m256i n22_n90 = _mm256_set1_epi32(-1376346);
    const __m256i p67_p78 = _mm256_set1_epi32(4390990);
    const __m256i n90_n39 = _mm256_set1_epi32(-5832743);
    const __m256i n54_p67 = _mm256_set1_epi32(-3538877);
    const __m256i p39_n78 = _mm256_set1_epi32(2621362);
    const __m256i n22_p85 = _mm256_set1_epi32(-1441707);
    const __m256i p04_n90 = _mm256_set1_epi32(327590);
    const __m256i n73_p61 = _mm256_set1_epi32(-4784067);
    const __m256i p82_n47 = _mm256_set1_epi32(5439441);
    const __m256i n88_p30 = _mm256_set1_epi32(-5767138);
    const __m256i p90_n13 = _mm256_set1_epi32(5963763);
    const __m256i n85_p54 = _mm256_set1_epi32(-5570506);
    const __m256i p88_n04 = _mm256_set1_epi32(5832700);
    const __m256i n61_n47 = _mm256_set1_epi32(-3932207);
    const __m256i p13_p82 = _mm256_set1_epi32(852050);
    const __m256i n90_p47 = _mm256_set1_epi32(-5898193);
    const __m256i p54_p39 = _mm256_set1_epi32(3538983);
    const __m256i p30_n90 = _mm256_set1_epi32(2031526);
    const __m256i n88_p61 = _mm256_set1_epi32(-5767107);
    const __m256i n88_p39 = _mm256_set1_epi32(-5767129);
    const __m256i n04_p73 = _mm256_set1_epi32(-262071);
    const __m256i p90_n67 = _mm256_set1_epi32(5963709);
    const __m256i n30_n47 = _mm256_set1_epi32(-1900591);
    const __m256i n78_p30 = _mm256_set1_epi32(-5111778);
    const __m256i n61_p90 = _mm256_set1_epi32(-3997606);
    const __m256i p54_p04 = _mm256_set1_epi32(3538948);
    const __m256i p82_n88 = _mm256_set1_epi32(5439400);
    const __m256i n61_p22 = _mm256_set1_epi32(-3997674);
    const __m256i n90_p85 = _mm256_set1_epi32(-5898155);
    const __m256i n39_p73 = _mm256_set1_epi32(-2555831);
    const __m256i p47_n04 = _mm256_set1_epi32(3145724);
    const __m256i n39_p13 = _mm256_set1_epi32(-2555891);
    const __m256i n78_p61 = _mm256_set1_epi32(-5111747);
    const __m256i n90_p88 = _mm256_set1_epi32(-5898152);
    const __m256i n73_p85 = _mm256_set1_epi32(-4784043);
    const __m256i n13_p04 = _mm256_set1_epi32(-851964);
    const __m256i n30_p22 = _mm256_set1_epi32(-1966058);
    const __m256i n47_p39 = _mm256_set1_epi32(-3080153);
    const __m256i n61_p54 = _mm256_set1_epi32(-3997642);
    const __m256i p87_p90 = _mm256_set1_epi32(5701722);
    const __m256i p70_p80 = _mm256_set1_epi32(4587600);
    const __m256i p57_p87 = _mm256_set1_epi32(3735639);
    const __m256i n43_p09 = _mm256_set1_epi32(-2818039);
    const __m256i p09_p80 = _mm256_set1_epi32(589904);
    const __m256i n87_n70 = _mm256_set1_epi32(-5636166);
    const __m256i p90_n26 = _mm256_set1_epi32(5963750);
    const __m256i n80_p90 = _mm256_set1_epi32(-5242790);
    const __m256i n80_p57 = _mm256_set1_epi32(-5242823);
    const __m256i n43_p70 = _mm256_set1_epi32(-2817978);
    const __m256i p09_n87 = _mm256_set1_epi32(655273);
    const __m256i n90_p43 = _mm256_set1_epi32(-5898197);
    const __m256i p26_p57 = _mm256_set1_epi32(1703993);
    const __m256i n70_p26 = _mm256_set1_epi32(-4587494);
    const __m256i n26_p09 = _mm256_set1_epi32(-1703927);
    const __m256i n57_p43 = _mm256_set1_epi32(-3735509);
    const __m256i p75_p89 = _mm256_set1_epi32(4915289);
    const __m256i n18_p75 = _mm256_set1_epi32(-1179573);
    const __m256i n89_p50 = _mm256_set1_epi32(-5832654);
    const __m256i n50_p18 = _mm256_set1_epi32(-3276782);
    const __m256i p84_p64 = _mm256_set1_epi32(5505088);
    const __m256i p35_p64 = _mm256_set1_epi32(2293824);
    const __m256i n35_p64 = _mm256_set1_epi32(-2293696);
    const __m256i n84_p64 = _mm256_set1_epi32(-5504960);
    const __m256i p37_n28 = _mm256_set1_epi32(2490340);
    const __m256i n45_p33 = _mm256_set1_epi32(-2949087);
    const __m256i n33_p28 = _mm256_set1_epi32(-2162660);

    __m256i c32_off = _mm256_set1_epi32(1 << (shift - 1));

    __m128i in00, in01, in02, in03, in04, in05, in06, in07, in08, in09, in10, in11, in12, in13, in14, in15;
    __m128i in16, in17, in18, in19, in20, in21, in22, in23, in24, in25, in26, in27, in28, in29, in30, in31;
    __m128i ss00, ss01, ss02, ss03, ss04, ss05, ss06, ss07, ss08, ss09, ss10, ss11, ss12, ss13, ss14, ss15;
    __m256i res00, res01, res02, res03, res04, res05, res06, res07, res08, res09, res10, res11, res12, res13, res14, res15;
    __m256i res16, res17, res18, res19, res20, res21, res22, res23, res24, res25, res26, res27, res28, res29, res30, res31;

    int i;

    for (i = 0; i < line; i += 8) {
        in01 = _mm_loadu_si128((const __m128i*) & src[1 * i_src + i]);
        in03 = _mm_loadu_si128((const __m128i*) & src[3 * i_src + i]);
        in05 = _mm_loadu_si128((const __m128i*) & src[5 * i_src + i]);
        in07 = _mm_loadu_si128((const __m128i*) & src[7 * i_src + i]);
        in09 = _mm_loadu_si128((const __m128i*) & src[9 * i_src + i]);
        in11 = _mm_loadu_si128((const __m128i*) & src[11 * i_src + i]);
        in13 = _mm_loadu_si128((const __m128i*) & src[13 * i_src + i]);
        in15 = _mm_loadu_si128((const __m128i*) & src[15 * i_src + i]);
        in17 = _mm_loadu_si128((const __m128i*) & src[17 * i_src + i]);
        in19 = _mm_loadu_si128((const __m128i*) & src[19 * i_src + i]);
        in21 = _mm_loadu_si128((const __m128i*) & src[21 * i_src + i]);
        in23 = _mm_loadu_si128((const __m128i*) & src[23 * i_src + i]);
        in25 = _mm_loadu_si128((const __m128i*) & src[25 * i_src + i]);
        in27 = _mm_loadu_si128((const __m128i*) & src[27 * i_src + i]);
        in29 = _mm_loadu_si128((const __m128i*) & src[29 * i_src + i]);
        in31 = _mm_loadu_si128((const __m128i*) & src[31 * i_src + i]);

        ss00 = _mm_unpacklo_epi16(in01, in03);
        ss01 = _mm_unpacklo_epi16(in05, in07);
        ss02 = _mm_unpacklo_epi16(in09, in11);
        ss03 = _mm_unpacklo_epi16(in13, in15);
        ss04 = _mm_unpacklo_epi16(in17, in19);
        ss05 = _mm_unpacklo_epi16(in21, in23);
        ss06 = _mm_unpacklo_epi16(in25, in27);
        ss07 = _mm_unpacklo_epi16(in29, in31);

        ss08 = _mm_unpackhi_epi16(in01, in03);
        ss09 = _mm_unpackhi_epi16(in05, in07);
        ss10 = _mm_unpackhi_epi16(in09, in11);
        ss11 = _mm_unpackhi_epi16(in13, in15);
        ss12 = _mm_unpackhi_epi16(in17, in19);
        ss13 = _mm_unpackhi_epi16(in21, in23);
        ss14 = _mm_unpackhi_epi16(in25, in27);
        ss15 = _mm_unpackhi_epi16(in29, in31);


        {
            const __m256i T_00_00 = _mm256_set_m128i(ss08, ss00);       // [33 13 32 12 31 11 30 10]
            const __m256i T_00_01 = _mm256_set_m128i(ss09, ss01);
            const __m256i T_00_02 = _mm256_set_m128i(ss10, ss02);
            const __m256i T_00_03 = _mm256_set_m128i(ss11, ss03);
            const __m256i T_00_04 = _mm256_set_m128i(ss12, ss04);
            const __m256i T_00_05 = _mm256_set_m128i(ss13, ss05);
            const __m256i T_00_06 = _mm256_set_m128i(ss14, ss06);
            const __m256i T_00_07 = _mm256_set_m128i(ss15, ss07);

            __m256i O00, O01, O02, O03, O04, O05, O06, O07, O08, O09, O10, O11, O12, O13, O14, O15;
            __m256i O16, O17, O18, O19, O20, O21, O22, O23, O24, O25, O26, O27, O28, O29, O30, O31;
            __m256i EO00, EO01, EO02, EO03, EO04, EO05, EO06, EO07, EO08, EO09, EO10, EO11, EO12, EO13, EO14, EO15;
            {
                __m256i T1, T2, T3, T4;
#define COMPUTE_ROW(c0103, c0507, c0911, c1315, c1719, c2123, c2527, c2931, row) \
    T1 = _mm256_add_epi32(_mm256_madd_epi16(T_00_00, c0103), _mm256_madd_epi16(T_00_01, c0507)); \
    T2 = _mm256_add_epi32(_mm256_madd_epi16(T_00_02, c0911), _mm256_madd_epi16(T_00_03, c1315)); \
    T3 = _mm256_add_epi32(_mm256_madd_epi16(T_00_04, c1719), _mm256_madd_epi16(T_00_05, c2123)); \
    T4 = _mm256_add_epi32(_mm256_madd_epi16(T_00_06, c2527), _mm256_madd_epi16(T_00_07, c2931)); \
    row = _mm256_add_epi32(_mm256_add_epi32(T1, T2), _mm256_add_epi32(T3, T4));

                // O[32]
                    COMPUTE_ROW(p90_p90, p89_p90, p87_p88, p84_p86, p81_p83, p76_p79, p71_p74, p66_p69, O00)
                    COMPUTE_ROW(p88_p90, p79_p84, p62_p71, p41_p52, p15_p28, n11_p02, n37_n24, n59_n48, O01)
                    COMPUTE_ROW(p84_p90, p59_p74, p20_p41, n24_n02, n62_n45, n86_n76, n89_n90, n71_n83, O02)
                    COMPUTE_ROW(p79_p89, p33_p59, n28_p02, n76_n56, n90_n88, n62_n81, n07_n37, p52_p24, O03)
                    COMPUTE_ROW(p71_p88, p02_p41, n69_n37, n89_n87, n45_n74, p33_n07, p86_p66, p76_p90, O04)
                    COMPUTE_ROW(p62_p87, n28_p20, n89_n69, n56_n84, p37_n11, p90_p74, p48_p81, n45_p02, O05)
                    COMPUTE_ROW(p52_p86, n56_n02, n84_n87, p07_n48, p88_p59, p45_p83, n62_n11, n81_n89, O06)
                    COMPUTE_ROW(p41_p84, n76_n24, n56_n89, p66_p07, p69_p90, n52_p11, n79_n88, p37_n28, O07)
                    COMPUTE_ROW(p28_p83, n88_n45, n11_n74, p90_p59, n07_p62, n89_n71, p24_n48, p84_p81, O08)
                    COMPUTE_ROW(p15_p81, n90_n62, p37_n45, p69_p88, n76_n07, n24_n84, p90_p56, n28_p52, O09)
                    COMPUTE_ROW(p02_p79, n81_n76, p74_n07, p11_p83, n84_n71, p69_n15, p20_p86, n87_n66, O10)
                    COMPUTE_ROW(n11_p76, n62_n86, p90_p33, n52_p45, n24_n89, p83_p69, n81_p02, p20_n71, O11)
                    COMPUTE_ROW(n24_p74, n37_n90, p81_p66, n88_n11, p56_n48, p02_p86, n59_n84, p89_p45, O12)
                    COMPUTE_ROW(n37_p71, n07_n89, p48_p86, n79_n62, p90_p24, n81_p20, p52_n59, n11_p84, O13)
                    COMPUTE_ROW(n48_p69, p24_n83, p02_p90, n28_n89, p52_p81, n71_n66, p84_p45, n90_n20, O14)
                    COMPUTE_ROW(n59_p66, p52_n71, n45_p76, p37_n81, n28_p84, p20_n87, n11_p89, p02_n90, O15)

                    COMPUTE_ROW(n69_p62, p74_n56, n79_p48, p83_n41, n86_p33, p88_n24, n90_p15, p90_n07, O16)
                    COMPUTE_ROW(n76_p59, p87_n37, n90_p11, p86_p15, n74_n41, p56_p62, n33_n79, p07_p88, O17)
                    COMPUTE_ROW(n83_p56, p90_n15, n76_n28, p45_p66, n02_n87, n41_p88, p74_n69, n90_p33, O18)
                    COMPUTE_ROW(n87_p52, p83_p07, n41_n62, n20_p90, p71_n76, n90_p28, p69_p33, n15_n79, O19)
                    COMPUTE_ROW(n90_p48, p66_p28, p07_n84, n74_p79, p87_n15, n37_n59, n41_p90, p88_n56, O20)
                    COMPUTE_ROW(n90_p45, p41_p48, p52_n90, n90_p37, p33_p56, p59_n89, n88_p28, p24_p62, O21)
                    COMPUTE_ROW(n89_p41, p11_p66, p83_n79, n59_n20, n48_p90, p87_n33, n02_n71, n86_p74, O22)
                    COMPUTE_ROW(n86_p37, n20_p79, p90_n52, p02_n69, n90_p66, p15_p56, p87_n76, n33_n41, O23)
                    COMPUTE_ROW(n81_p33, n48_p87, p71_n15, p62_n90, n59_n02, n74_p90, p45_p20, p83_n86, O24)
                    COMPUTE_ROW(n74_p28, n71_p90, p33_p24, p90_n76, p20_n69, n79_p37, n66_p90, p41_p15, O25)
                    COMPUTE_ROW(n66_p24, n86_p88, n15_p59, p71_n33, p83_n90, p07_n52, n76_p41, n79_p90, O26)
                    COMPUTE_ROW(n56_p20, n90_p81, n59_p83, p15_p24, p79_n52, p84_n90, p28_n62, n48_p11, O27)
                    COMPUTE_ROW(n45_p15, n84_p69, n86_p90, n48_p71, p11_p20, p66_n41, p90_n83, p74_n87, O28)
                    COMPUTE_ROW(n33_p11, n69_p52, n88_p81, n87_p90, n66_p79, n28_p48, p15_p07, p56_n37, O29)
                    COMPUTE_ROW(n20_p07, n45_p33, n66_p56, n81_p74, n89_p86, n90_p90, n83_p87, n69_p76, O30)
                    COMPUTE_ROW(n07_p02, n15_p11, n24_p20, n33_p28, n41_p37, n48_p45, n56_p52, n62_p59, O31)


#undef COMPUTE_ROW
            }

            in00 = _mm_loadu_si128((const __m128i*) & src[0 * i_src + i]);
            in02 = _mm_loadu_si128((const __m128i*) & src[2 * i_src + i]);
            in04 = _mm_loadu_si128((const __m128i*) & src[4 * i_src + i]);
            in06 = _mm_loadu_si128((const __m128i*) & src[6 * i_src + i]);
            in08 = _mm_loadu_si128((const __m128i*) & src[8 * i_src + i]);
            in10 = _mm_loadu_si128((const __m128i*) & src[10 * i_src + i]);
            in12 = _mm_loadu_si128((const __m128i*) & src[12 * i_src + i]);
            in14 = _mm_loadu_si128((const __m128i*) & src[14 * i_src + i]);
            in16 = _mm_loadu_si128((const __m128i*) & src[16 * i_src + i]);
            in18 = _mm_loadu_si128((const __m128i*) & src[18 * i_src + i]);
            in20 = _mm_loadu_si128((const __m128i*) & src[20 * i_src + i]);
            in22 = _mm_loadu_si128((const __m128i*) & src[22 * i_src + i]);
            in24 = _mm_loadu_si128((const __m128i*) & src[24 * i_src + i]);
            in26 = _mm_loadu_si128((const __m128i*) & src[26 * i_src + i]);
            in28 = _mm_loadu_si128((const __m128i*) & src[28 * i_src + i]);
            in30 = _mm_loadu_si128((const __m128i*) & src[30 * i_src + i]);

            ss00 = _mm_unpacklo_epi16(in02, in06);
            ss01 = _mm_unpacklo_epi16(in10, in14);
            ss02 = _mm_unpacklo_epi16(in18, in22);
            ss03 = _mm_unpacklo_epi16(in26, in30);
            ss04 = _mm_unpacklo_epi16(in04, in12);
            ss05 = _mm_unpacklo_epi16(in20, in28);
            ss06 = _mm_unpacklo_epi16(in08, in24);
            ss07 = _mm_unpacklo_epi16(in00, in16);

            ss08 = _mm_unpackhi_epi16(in02, in06);
            ss09 = _mm_unpackhi_epi16(in10, in14);
            ss10 = _mm_unpackhi_epi16(in18, in22);
            ss11 = _mm_unpackhi_epi16(in26, in30);
            ss12 = _mm_unpackhi_epi16(in04, in12);
            ss13 = _mm_unpackhi_epi16(in20, in28);
            ss14 = _mm_unpackhi_epi16(in08, in24);
            ss15 = _mm_unpackhi_epi16(in00, in16);

            {
                __m256i T1, T2;
                const __m256i T_00_08 = _mm256_set_m128i(ss08, ss00);
                const __m256i T_00_09 = _mm256_set_m128i(ss09, ss01);
                const __m256i T_00_10 = _mm256_set_m128i(ss10, ss02);
                const __m256i T_00_11 = _mm256_set_m128i(ss11, ss03);
                const __m256i T_00_12 = _mm256_set_m128i(ss12, ss04);
                const __m256i T_00_13 = _mm256_set_m128i(ss13, ss05);
                const __m256i T_00_14 = _mm256_set_m128i(ss14, ss06);
                const __m256i T_00_15 = _mm256_set_m128i(ss15, ss07);

#define COMPUTE_ROW(c0206, c1014, c1822, c2630, row) \
    T1 = _mm256_add_epi32(_mm256_madd_epi16(T_00_08, c0206), _mm256_madd_epi16(T_00_09, c1014)); \
    T2 = _mm256_add_epi32(_mm256_madd_epi16(T_00_10, c1822), _mm256_madd_epi16(T_00_11, c2630)); \
    row = _mm256_add_epi32(T1, T2);

                // EO[16]
                    COMPUTE_ROW(p90_p90, p85_p88, p78_p82, p67_p73, EO00)
                    COMPUTE_ROW(p82_p90, p47_p67, n04_p22, n54_n30, EO01)
                    COMPUTE_ROW(p67_p88, n13_p30, n82_n54, n78_n90, EO02)
                    COMPUTE_ROW(p47_p85, n67_n13, n73_n90, p39_n22, EO03)
                    COMPUTE_ROW(p22_p82, n90_n54, p13_n61, p85_p78, EO04)
                    COMPUTE_ROW(n04_p78, n73_n82, p85_p13, n22_p67, EO05)
                    COMPUTE_ROW(n30_p73, n22_n90, p67_p78, n90_n39, EO06)
                    COMPUTE_ROW(n54_p67, p39_n78, n22_p85, p04_n90, EO07)
                    COMPUTE_ROW(n73_p61, p82_n47, n88_p30, p90_n13, EO08)
                    COMPUTE_ROW(n85_p54, p88_n04, n61_n47, p13_p82, EO09)
                    COMPUTE_ROW(n90_p47, p54_p39, p30_n90, n88_p61, EO10)
                    COMPUTE_ROW(n88_p39, n04_p73, p90_n67, n30_n47, EO11)
                    COMPUTE_ROW(n78_p30, n61_p90, p54_p04, p82_n88, EO12)
                    COMPUTE_ROW(n61_p22, n90_p85, n39_p73, p47_n04, EO13)
                    COMPUTE_ROW(n39_p13, n78_p61, n90_p88, n73_p85, EO14)
                    COMPUTE_ROW(n13_p04, n30_p22, n47_p39, n61_p54, EO15)


#undef COMPUTE_ROW

                {
                    // EEO[8]
                    const __m256i EEO0 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, p87_p90), _mm256_madd_epi16(T_00_13, p70_p80));
                    const __m256i EEO1 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, p57_p87), _mm256_madd_epi16(T_00_13, n43_p09));
                    const __m256i EEO2 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, p09_p80), _mm256_madd_epi16(T_00_13, n87_n70));
                    const __m256i EEO3 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, n43_p70), _mm256_madd_epi16(T_00_13, p09_n87));
                    const __m256i EEO4 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, n80_p57), _mm256_madd_epi16(T_00_13, p90_n26));
                    const __m256i EEO5 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, n90_p43), _mm256_madd_epi16(T_00_13, p26_p57));
                    const __m256i EEO6 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, n70_p26), _mm256_madd_epi16(T_00_13, n80_p90));
                    const __m256i EEO7 = _mm256_add_epi32(_mm256_madd_epi16(T_00_12, n26_p09), _mm256_madd_epi16(T_00_13, n57_p43));

                    // EEEO[4]
                    const __m256i EEEO0 = _mm256_madd_epi16(T_00_14, p75_p89);
                    const __m256i EEEO1 = _mm256_madd_epi16(T_00_14, n18_p75);
                    const __m256i EEEO2 = _mm256_madd_epi16(T_00_14, n89_p50);
                    const __m256i EEEO3 = _mm256_madd_epi16(T_00_14, n50_p18);

                    const __m256i EEEE0 = _mm256_madd_epi16(T_00_15, p84_p64);
                    const __m256i EEEE1 = _mm256_madd_epi16(T_00_15, p35_p64);
                    const __m256i EEEE2 = _mm256_madd_epi16(T_00_15, n35_p64);
                    const __m256i EEEE3 = _mm256_madd_epi16(T_00_15, n84_p64);

                    const __m256i EEE0 = _mm256_add_epi32(EEEE0, EEEO0);          // EEE0 = EEEE0 + EEEO0
                    const __m256i EEE1 = _mm256_add_epi32(EEEE1, EEEO1);          // EEE1 = EEEE1 + EEEO1
                    const __m256i EEE2 = _mm256_add_epi32(EEEE2, EEEO2);          // EEE2 = EEEE2 + EEEO2
                    const __m256i EEE3 = _mm256_add_epi32(EEEE3, EEEO3);          // EEE3 = EEEE3 + EEEO3
                    const __m256i EEE7 = _mm256_sub_epi32(EEEE0, EEEO0);          // EEE7 = EEEE0 - EEEO0
                    const __m256i EEE6 = _mm256_sub_epi32(EEEE1, EEEO1);          // EEE6 = EEEE1 - EEEO1
                    const __m256i EEE5 = _mm256_sub_epi32(EEEE2, EEEO2);          // EEE7 = EEEE2 - EEEO2
                    const __m256i EEE4 = _mm256_sub_epi32(EEEE3, EEEO3);          // EEE6 = EEEE3 - EEEO3

                    const __m256i EE00 = _mm256_add_epi32(EEE0, EEO0);          // EE0 = EEE0 + EEO0
                    const __m256i EE01 = _mm256_add_epi32(EEE1, EEO1);          // EE1 = EEE1 + EEO1
                    const __m256i EE02 = _mm256_add_epi32(EEE2, EEO2);          // EE2 = EEE2 + EEO2
                    const __m256i EE03 = _mm256_add_epi32(EEE3, EEO3);          // EE3 = EEE3 + EEO3
                    const __m256i EE04 = _mm256_add_epi32(EEE4, EEO4);          // EE4 = EEE4 + EEO4
                    const __m256i EE05 = _mm256_add_epi32(EEE5, EEO5);          // EE5 = EEE5 + EEO5
                    const __m256i EE06 = _mm256_add_epi32(EEE6, EEO6);          // EE6 = EEE6 + EEO6
                    const __m256i EE07 = _mm256_add_epi32(EEE7, EEO7);          // EE7 = EEE7 + EEO7
                    const __m256i EE15 = _mm256_sub_epi32(EEE0, EEO0);          // EE15 = EEE0 - EEO0
                    const __m256i EE14 = _mm256_sub_epi32(EEE1, EEO1);
                    const __m256i EE13 = _mm256_sub_epi32(EEE2, EEO2);
                    const __m256i EE12 = _mm256_sub_epi32(EEE3, EEO3);
                    const __m256i EE11 = _mm256_sub_epi32(EEE4, EEO4);          // EE11 = EEE4 - EEO4
                    const __m256i EE10 = _mm256_sub_epi32(EEE5, EEO5);
                    const __m256i EE09 = _mm256_sub_epi32(EEE6, EEO6);
                    const __m256i EE08 = _mm256_sub_epi32(EEE7, EEO7);

                    const __m256i E00 = _mm256_add_epi32(EE00, EO00);          // E00 = EE00 + EO00
                    const __m256i E01 = _mm256_add_epi32(EE01, EO01);          // E01 = EE01 + EO01
                    const __m256i E02 = _mm256_add_epi32(EE02, EO02);          // E02 = EE02 + EO02
                    const __m256i E03 = _mm256_add_epi32(EE03, EO03);          // E03 = EE03 + EO03
                    const __m256i E04 = _mm256_add_epi32(EE04, EO04);
                    const __m256i E05 = _mm256_add_epi32(EE05, EO05);
                    const __m256i E06 = _mm256_add_epi32(EE06, EO06);
                    const __m256i E07 = _mm256_add_epi32(EE07, EO07);
                    const __m256i E08 = _mm256_add_epi32(EE08, EO08);          // E08 = EE08 + EO08
                    const __m256i E09 = _mm256_add_epi32(EE09, EO09);
                    const __m256i E10 = _mm256_add_epi32(EE10, EO10);
                    const __m256i E11 = _mm256_add_epi32(EE11, EO11);
                    const __m256i E12 = _mm256_add_epi32(EE12, EO12);
                    const __m256i E13 = _mm256_add_epi32(EE13, EO13);
                    const __m256i E14 = _mm256_add_epi32(EE14, EO14);
                    const __m256i E15 = _mm256_add_epi32(EE15, EO15);
                    const __m256i E31 = _mm256_sub_epi32(EE00, EO00);          // E31 = EE00 - EO00
                    const __m256i E30 = _mm256_sub_epi32(EE01, EO01);          // E30 = EE01 - EO01
                    const __m256i E29 = _mm256_sub_epi32(EE02, EO02);          // E29 = EE02 - EO02
                    const __m256i E28 = _mm256_sub_epi32(EE03, EO03);          // E28 = EE03 - EO03
                    const __m256i E27 = _mm256_sub_epi32(EE04, EO04);
                    const __m256i E26 = _mm256_sub_epi32(EE05, EO05);
                    const __m256i E25 = _mm256_sub_epi32(EE06, EO06);
                    const __m256i E24 = _mm256_sub_epi32(EE07, EO07);
                    const __m256i E23 = _mm256_sub_epi32(EE08, EO08);          // E23 = EE08 - EO08
                    const __m256i E22 = _mm256_sub_epi32(EE09, EO09);
                    const __m256i E21 = _mm256_sub_epi32(EE10, EO10);
                    const __m256i E20 = _mm256_sub_epi32(EE11, EO11);
                    const __m256i E19 = _mm256_sub_epi32(EE12, EO12);
                    const __m256i E18 = _mm256_sub_epi32(EE13, EO13);
                    const __m256i E17 = _mm256_sub_epi32(EE14, EO14);
                    const __m256i E16 = _mm256_sub_epi32(EE15, EO15);

                    const __m256i T1_00 = _mm256_add_epi32(E00, c32_off);         // E0 + off
                    const __m256i T1_01 = _mm256_add_epi32(E01, c32_off);         // E1 + off
                    const __m256i T1_02 = _mm256_add_epi32(E02, c32_off);         // E2 + off
                    const __m256i T1_03 = _mm256_add_epi32(E03, c32_off);         // E3 + off
                    const __m256i T1_04 = _mm256_add_epi32(E04, c32_off);         // E4 + off
                    const __m256i T1_05 = _mm256_add_epi32(E05, c32_off);         // E5 + off
                    const __m256i T1_06 = _mm256_add_epi32(E06, c32_off);         // E6 + off
                    const __m256i T1_07 = _mm256_add_epi32(E07, c32_off);         // E7 + off
                    const __m256i T1_08 = _mm256_add_epi32(E08, c32_off);         // E8 + off
                    const __m256i T1_09 = _mm256_add_epi32(E09, c32_off);         // E9 + off
                    const __m256i T1_10 = _mm256_add_epi32(E10, c32_off);         // E10 + off
                    const __m256i T1_11 = _mm256_add_epi32(E11, c32_off);         // E11 + off
                    const __m256i T1_12 = _mm256_add_epi32(E12, c32_off);         // E12 + off
                    const __m256i T1_13 = _mm256_add_epi32(E13, c32_off);         // E13 + off
                    const __m256i T1_14 = _mm256_add_epi32(E14, c32_off);         // E14 + off
                    const __m256i T1_15 = _mm256_add_epi32(E15, c32_off);         // E15 + off
                    const __m256i T1_16 = _mm256_add_epi32(E16, c32_off);
                    const __m256i T1_17 = _mm256_add_epi32(E17, c32_off);
                    const __m256i T1_18 = _mm256_add_epi32(E18, c32_off);
                    const __m256i T1_19 = _mm256_add_epi32(E19, c32_off);
                    const __m256i T1_20 = _mm256_add_epi32(E20, c32_off);
                    const __m256i T1_21 = _mm256_add_epi32(E21, c32_off);
                    const __m256i T1_22 = _mm256_add_epi32(E22, c32_off);
                    const __m256i T1_23 = _mm256_add_epi32(E23, c32_off);
                    const __m256i T1_24 = _mm256_add_epi32(E24, c32_off);
                    const __m256i T1_25 = _mm256_add_epi32(E25, c32_off);
                    const __m256i T1_26 = _mm256_add_epi32(E26, c32_off);
                    const __m256i T1_27 = _mm256_add_epi32(E27, c32_off);
                    const __m256i T1_28 = _mm256_add_epi32(E28, c32_off);
                    const __m256i T1_29 = _mm256_add_epi32(E29, c32_off);
                    const __m256i T1_30 = _mm256_add_epi32(E30, c32_off);
                    const __m256i T1_31 = _mm256_add_epi32(E31, c32_off);

                    __m256i T2_00 = _mm256_add_epi32(T1_00, O00);          // E0 + O0 + off
                    __m256i T2_01 = _mm256_add_epi32(T1_01, O01);
                    __m256i T2_02 = _mm256_add_epi32(T1_02, O02);          // E1 + O1 + off
                    __m256i T2_03 = _mm256_add_epi32(T1_03, O03);
                    __m256i T2_04 = _mm256_add_epi32(T1_04, O04);          // E2 + O2 + off
                    __m256i T2_05 = _mm256_add_epi32(T1_05, O05);
                    __m256i T2_06 = _mm256_add_epi32(T1_06, O06);          // E3 + O3 + off
                    __m256i T2_07 = _mm256_add_epi32(T1_07, O07);
                    __m256i T2_08 = _mm256_add_epi32(T1_08, O08);          // E4
                    __m256i T2_09 = _mm256_add_epi32(T1_09, O09);
                    __m256i T2_10 = _mm256_add_epi32(T1_10, O10);          // E5
                    __m256i T2_11 = _mm256_add_epi32(T1_11, O11);
                    __m256i T2_12 = _mm256_add_epi32(T1_12, O12);          // E6
                    __m256i T2_13 = _mm256_add_epi32(T1_13, O13);
                    __m256i T2_14 = _mm256_add_epi32(T1_14, O14);          // E7
                    __m256i T2_15 = _mm256_add_epi32(T1_15, O15);
                    __m256i T2_16 = _mm256_add_epi32(T1_16, O16);          // E8
                    __m256i T2_17 = _mm256_add_epi32(T1_17, O17);
                    __m256i T2_18 = _mm256_add_epi32(T1_18, O18);          // E9
                    __m256i T2_19 = _mm256_add_epi32(T1_19, O19);
                    __m256i T2_20 = _mm256_add_epi32(T1_20, O20);          // E10
                    __m256i T2_21 = _mm256_add_epi32(T1_21, O21);
                    __m256i T2_22 = _mm256_add_epi32(T1_22, O22);          // E11
                    __m256i T2_23 = _mm256_add_epi32(T1_23, O23);
                    __m256i T2_24 = _mm256_add_epi32(T1_24, O24);          // E12
                    __m256i T2_25 = _mm256_add_epi32(T1_25, O25);
                    __m256i T2_26 = _mm256_add_epi32(T1_26, O26);          // E13
                    __m256i T2_27 = _mm256_add_epi32(T1_27, O27);
                    __m256i T2_28 = _mm256_add_epi32(T1_28, O28);          // E14
                    __m256i T2_29 = _mm256_add_epi32(T1_29, O29);
                    __m256i T2_30 = _mm256_add_epi32(T1_30, O30);          // E15
                    __m256i T2_31 = _mm256_add_epi32(T1_31, O31);
                    __m256i T2_63 = _mm256_sub_epi32(T1_00, O00);          // E00 - O00 + off
                    __m256i T2_62 = _mm256_sub_epi32(T1_01, O01);
                    __m256i T2_61 = _mm256_sub_epi32(T1_02, O02);
                    __m256i T2_60 = _mm256_sub_epi32(T1_03, O03);
                    __m256i T2_59 = _mm256_sub_epi32(T1_04, O04);
                    __m256i T2_58 = _mm256_sub_epi32(T1_05, O05);
                    __m256i T2_57 = _mm256_sub_epi32(T1_06, O06);
                    __m256i T2_56 = _mm256_sub_epi32(T1_07, O07);
                    __m256i T2_55 = _mm256_sub_epi32(T1_08, O08);
                    __m256i T2_54 = _mm256_sub_epi32(T1_09, O09);
                    __m256i T2_53 = _mm256_sub_epi32(T1_10, O10);
                    __m256i T2_52 = _mm256_sub_epi32(T1_11, O11);
                    __m256i T2_51 = _mm256_sub_epi32(T1_12, O12);
                    __m256i T2_50 = _mm256_sub_epi32(T1_13, O13);
                    __m256i T2_49 = _mm256_sub_epi32(T1_14, O14);
                    __m256i T2_48 = _mm256_sub_epi32(T1_15, O15);
                    __m256i T2_47 = _mm256_sub_epi32(T1_16, O16);
                    __m256i T2_46 = _mm256_sub_epi32(T1_17, O17);
                    __m256i T2_45 = _mm256_sub_epi32(T1_18, O18);
                    __m256i T2_44 = _mm256_sub_epi32(T1_19, O19);
                    __m256i T2_43 = _mm256_sub_epi32(T1_20, O20);
                    __m256i T2_42 = _mm256_sub_epi32(T1_21, O21);
                    __m256i T2_41 = _mm256_sub_epi32(T1_22, O22);
                    __m256i T2_40 = _mm256_sub_epi32(T1_23, O23);
                    __m256i T2_39 = _mm256_sub_epi32(T1_24, O24);
                    __m256i T2_38 = _mm256_sub_epi32(T1_25, O25);
                    __m256i T2_37 = _mm256_sub_epi32(T1_26, O26);
                    __m256i T2_36 = _mm256_sub_epi32(T1_27, O27);
                    __m256i T2_35 = _mm256_sub_epi32(T1_28, O28);
                    __m256i T2_34 = _mm256_sub_epi32(T1_29, O29);
                    __m256i T2_33 = _mm256_sub_epi32(T1_30, O30);
                    __m256i T2_32 = _mm256_sub_epi32(T1_31, O31);

                    T2_00 = _mm256_srai_epi32(T2_00, shift);             // [30 20 10 00]
                    T2_01 = _mm256_srai_epi32(T2_01, shift);             // [70 60 50 40]
                    T2_02 = _mm256_srai_epi32(T2_02, shift);             // [31 21 11 01]
                    T2_03 = _mm256_srai_epi32(T2_03, shift);             // [71 61 51 41]
                    T2_04 = _mm256_srai_epi32(T2_04, shift);             // [32 22 12 02]
                    T2_05 = _mm256_srai_epi32(T2_05, shift);             // [72 62 52 42]
                    T2_06 = _mm256_srai_epi32(T2_06, shift);             // [33 23 13 03]
                    T2_07 = _mm256_srai_epi32(T2_07, shift);             // [73 63 53 43]
                    T2_08 = _mm256_srai_epi32(T2_08, shift);             // [33 24 14 04]
                    T2_09 = _mm256_srai_epi32(T2_09, shift);             // [74 64 54 44]
                    T2_10 = _mm256_srai_epi32(T2_10, shift);             // [35 25 15 05]
                    T2_11 = _mm256_srai_epi32(T2_11, shift);             // [75 65 55 45]
                    T2_12 = _mm256_srai_epi32(T2_12, shift);             // [36 26 16 06]
                    T2_13 = _mm256_srai_epi32(T2_13, shift);             // [76 66 56 46]
                    T2_14 = _mm256_srai_epi32(T2_14, shift);             // [37 27 17 07]
                    T2_15 = _mm256_srai_epi32(T2_15, shift);             // [77 67 57 47]
                    T2_16 = _mm256_srai_epi32(T2_16, shift);             // [30 20 10 00] x8
                    T2_17 = _mm256_srai_epi32(T2_17, shift);             // [70 60 50 40]
                    T2_18 = _mm256_srai_epi32(T2_18, shift);             // [31 21 11 01] x9
                    T2_19 = _mm256_srai_epi32(T2_19, shift);             // [71 61 51 41]
                    T2_20 = _mm256_srai_epi32(T2_20, shift);             // [32 22 12 02] xA
                    T2_21 = _mm256_srai_epi32(T2_21, shift);             // [72 62 52 42]
                    T2_22 = _mm256_srai_epi32(T2_22, shift);             // [33 23 13 03] xB
                    T2_23 = _mm256_srai_epi32(T2_23, shift);             // [73 63 53 43]
                    T2_24 = _mm256_srai_epi32(T2_24, shift);             // [33 24 14 04] xC
                    T2_25 = _mm256_srai_epi32(T2_25, shift);             // [74 64 54 44]
                    T2_26 = _mm256_srai_epi32(T2_26, shift);             // [35 25 15 05] xD
                    T2_27 = _mm256_srai_epi32(T2_27, shift);             // [75 65 55 45]
                    T2_28 = _mm256_srai_epi32(T2_28, shift);             // [36 26 16 06] xE
                    T2_29 = _mm256_srai_epi32(T2_29, shift);             // [76 66 56 46]
                    T2_30 = _mm256_srai_epi32(T2_30, shift);             // [37 27 17 07] xF
                    T2_31 = _mm256_srai_epi32(T2_31, shift);             // [77 67 57 47]
                    T2_63 = _mm256_srai_epi32(T2_63, shift);
                    T2_62 = _mm256_srai_epi32(T2_62, shift);
                    T2_61 = _mm256_srai_epi32(T2_61, shift);
                    T2_60 = _mm256_srai_epi32(T2_60, shift);
                    T2_59 = _mm256_srai_epi32(T2_59, shift);
                    T2_58 = _mm256_srai_epi32(T2_58, shift);
                    T2_57 = _mm256_srai_epi32(T2_57, shift);
                    T2_56 = _mm256_srai_epi32(T2_56, shift);
                    T2_55 = _mm256_srai_epi32(T2_55, shift);
                    T2_54 = _mm256_srai_epi32(T2_54, shift);
                    T2_53 = _mm256_srai_epi32(T2_53, shift);
                    T2_52 = _mm256_srai_epi32(T2_52, shift);
                    T2_51 = _mm256_srai_epi32(T2_51, shift);
                    T2_50 = _mm256_srai_epi32(T2_50, shift);
                    T2_49 = _mm256_srai_epi32(T2_49, shift);
                    T2_48 = _mm256_srai_epi32(T2_48, shift);
                    T2_47 = _mm256_srai_epi32(T2_47, shift);
                    T2_46 = _mm256_srai_epi32(T2_46, shift);
                    T2_45 = _mm256_srai_epi32(T2_45, shift);
                    T2_44 = _mm256_srai_epi32(T2_44, shift);
                    T2_43 = _mm256_srai_epi32(T2_43, shift);
                    T2_42 = _mm256_srai_epi32(T2_42, shift);
                    T2_41 = _mm256_srai_epi32(T2_41, shift);
                    T2_40 = _mm256_srai_epi32(T2_40, shift);
                    T2_39 = _mm256_srai_epi32(T2_39, shift);
                    T2_38 = _mm256_srai_epi32(T2_38, shift);
                    T2_37 = _mm256_srai_epi32(T2_37, shift);
                    T2_36 = _mm256_srai_epi32(T2_36, shift);
                    T2_35 = _mm256_srai_epi32(T2_35, shift);
                    T2_34 = _mm256_srai_epi32(T2_34, shift);
                    T2_33 = _mm256_srai_epi32(T2_33, shift);
                    T2_32 = _mm256_srai_epi32(T2_32, shift);

                    //transpose matrix H x W: 64x8 --> 8x64
                    TRANSPOSE_16x8_32BIT_16BIT(T2_00, T2_01, T2_02, T2_03, T2_04, T2_05, T2_06, T2_07, T2_08, T2_09, T2_10, T2_11, T2_12, T2_13, T2_14, T2_15, res00, res04, res08, res12, res16, res20, res24, res28);
                    TRANSPOSE_16x8_32BIT_16BIT(T2_16, T2_17, T2_18, T2_19, T2_20, T2_21, T2_22, T2_23, T2_24, T2_25, T2_26, T2_27, T2_28, T2_29, T2_30, T2_31, res01, res05, res09, res13, res17, res21, res25, res29);
                    TRANSPOSE_16x8_32BIT_16BIT(T2_32, T2_33, T2_34, T2_35, T2_36, T2_37, T2_38, T2_39, T2_40, T2_41, T2_42, T2_43, T2_44, T2_45, T2_46, T2_47, res02, res06, res10, res14, res18, res22, res26, res30);
                    TRANSPOSE_16x8_32BIT_16BIT(T2_48, T2_49, T2_50, T2_51, T2_52, T2_53, T2_54, T2_55, T2_56, T2_57, T2_58, T2_59, T2_60, T2_61, T2_62, T2_63, res03, res07, res11, res15, res19, res23, res27, res31);

                }
                
                _mm256_storeu_si256((__m256i*) & dst[0 * 16], res00);
                _mm256_storeu_si256((__m256i*) & dst[1 * 16], res01);
                _mm256_storeu_si256((__m256i*) & dst[2 * 16], res02);
                _mm256_storeu_si256((__m256i*) & dst[3 * 16], res03);
                _mm256_storeu_si256((__m256i*) & dst[4 * 16], res04);
                _mm256_storeu_si256((__m256i*) & dst[5 * 16], res05);
                _mm256_storeu_si256((__m256i*) & dst[6 * 16], res06);
                _mm256_storeu_si256((__m256i*) & dst[7 * 16], res07);

                dst += 8 * 16;

                _mm256_storeu_si256((__m256i*) & dst[0 * 16], res08);
                _mm256_storeu_si256((__m256i*) & dst[1 * 16], res09);
                _mm256_storeu_si256((__m256i*) & dst[2 * 16], res10);
                _mm256_storeu_si256((__m256i*) & dst[3 * 16], res11);
                _mm256_storeu_si256((__m256i*) & dst[4 * 16], res12);
                _mm256_storeu_si256((__m256i*) & dst[5 * 16], res13);
                _mm256_storeu_si256((__m256i*) & dst[6 * 16], res14);
                _mm256_storeu_si256((__m256i*) & dst[7 * 16], res15);

                dst += 8 * 16;

                _mm256_storeu_si256((__m256i*) & dst[0 * 16], res16);
                _mm256_storeu_si256((__m256i*) & dst[1 * 16], res17);
                _mm256_storeu_si256((__m256i*) & dst[2 * 16], res18);
                _mm256_storeu_si256((__m256i*) & dst[3 * 16], res19);
                _mm256_storeu_si256((__m256i*) & dst[4 * 16], res20);
                _mm256_storeu_si256((__m256i*) & dst[5 * 16], res21);
                _mm256_storeu_si256((__m256i*) & dst[6 * 16], res22);
                _mm256_storeu_si256((__m256i*) & dst[7 * 16], res23);

                dst += 8 * 16;

                _mm256_storeu_si256((__m256i*) & dst[0 * 16], res24);
                _mm256_storeu_si256((__m256i*) & dst[1 * 16], res25);
                _mm256_storeu_si256((__m256i*) & dst[2 * 16], res26);
                _mm256_storeu_si256((__m256i*) & dst[3 * 16], res27);
                _mm256_storeu_si256((__m256i*) & dst[4 * 16], res28);
                _mm256_storeu_si256((__m256i*) & dst[5 * 16], res29);
                _mm256_storeu_si256((__m256i*) & dst[6 * 16], res30);
                _mm256_storeu_si256((__m256i*) & dst[7 * 16], res31);

                dst += 8 * 16;
            }
        }
    }

}


const XEVE_ITX xeve_tbl_itx_avx[MAX_TR_LOG2] =
{
    itx_pb2_avx,
    itx_pb4_avx,
    itx_pb8_avx,
    itx_pb16_avx,
    itx_pb32_avx,
    itx_pb64_avx
};
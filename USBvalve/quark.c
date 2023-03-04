/*
  USBvalve
*/

/*
   Quark reference C implementation

   Copyright (c) 2010-2014 Jean-Philippe Aumasson <jeanphilippe.aumasson@gmail.com>

   To the extent possible under law, the author(s) have dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along with
   this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
 */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include "quark.h"

#define DIGEST WIDTH

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint8_t u8;

typedef struct
{
  int pos;          /* number of bytes read into x from current block */
  u32 x[WIDTH * 8]; /* one bit stored in each word */
} hashState;


#if defined(UQUARK)
/* 17 bytes */
u8 iv[] = { 0xd8, 0xda, 0xca, 0x44, 0x41, 0x4a, 0x09, 0x97,
            0x19, 0xc8, 0x0a, 0xa3, 0xaf, 0x06, 0x56, 0x44, 0xdb };
#elif defined(DQUARK)
/* 22 bytes */
u8 iv[] = { 0xcc, 0x6c, 0x4a, 0xb7, 0xd1, 0x1f, 0xa9, 0xbd,
            0xf6, 0xee, 0xde, 0x03, 0xd8, 0x7b, 0x68, 0xf9,
            0x1b, 0xaa, 0x70, 0x6c, 0x20, 0xe9 };
#elif defined(SQUARK)
/* 32 bytes */
u8 iv[] = { 0x39, 0x72, 0x51, 0xce, 0xe1, 0xde, 0x8a, 0xa7,
            0x3e, 0xa2, 0x62, 0x50, 0xc6, 0xd7, 0xbe, 0x12,
            0x8c, 0xd3, 0xe7, 0x9d, 0xd7, 0x18, 0xc2, 0x4b,
            0x8a, 0x19, 0xd0, 0x9c, 0x24, 0x92, 0xda, 0x5d };
#elif defined(CQUARK)
/* 48 bytes */
u8 iv[] = { 0x3b, 0x45, 0x03, 0xec, 0x76, 0x62, 0xc3, 0xcb,
            0x30, 0xe0, 0x08, 0x37, 0xec, 0x8d, 0x38, 0xbb,
            0xe5, 0xff, 0x5a, 0xcd, 0x69, 0x01, 0xa2, 0x49,
            0x57, 0x50, 0xf9, 0x19, 0x8e, 0x2e, 0x3b, 0x58,
            0x52, 0xdc, 0xaa, 0x16, 0x62, 0xb7, 0xda, 0xd6,
            0x5f, 0xcb, 0x5a, 0x8a, 0x1f, 0x0d, 0x5f, 0xcc };
#endif

void showstate(u32 *x) {

  int i;
  u8 buf = 0;

  for (i = 0; i < 8 * WIDTH; ++i) {
    buf ^= (1 & x[i]) << (7 - (i % 8));

    if (((i % 8) == 7) && (i)) {
      printf("%02x", buf);
      buf = 0;
    }
  }

  printf("\n");
}


int permute_u(u32 *x) {
  /* state of 136=2x68 bits */
#define ROUNDS_U 4 * 136
#define N_LEN_U 68
#define L_LEN_U 10

  u32 *X, *Y, *L;
  u32 h;
  int i;

  X = (u32 *)malloc((N_LEN_U + ROUNDS_U) * sizeof(u32));
  Y = (u32 *)malloc((N_LEN_U + ROUNDS_U) * sizeof(u32));
  L = (u32 *)malloc((L_LEN_U + ROUNDS_U) * sizeof(u32));

  /* local copy of the state in the registers*/
  for (i = 0; i < N_LEN_U; ++i) {
    X[i] = x[i];
    Y[i] = x[i + N_LEN_U];
  }

  /* initialize the LFSR to 11..11 */
  for (i = 0; i < L_LEN_U; ++i)
    L[i] = 0xFFFFFFFF;

  /* iterate rounds */
  for (i = 0; i < ROUNDS_U; ++i) {

    /* indices up to i+59, for 8x parallelizibility*/

    /* need X[i] as linear term only, for invertibility */
    X[N_LEN_U + i] = X[i] ^ Y[i];
    X[N_LEN_U + i] ^= X[i + 9] ^ X[i + 14] ^ X[i + 21] ^ X[i + 28] ^ X[i + 33] ^ X[i + 37] ^ X[i + 45] ^ X[i + 52] ^ X[i + 55] ^ X[i + 50] ^ (X[i + 59] & X[i + 55]) ^ (X[i + 37] & X[i + 33]) ^ (X[i + 15] & X[i + 9]) ^ (X[i + 55] & X[i + 52] & X[i + 45]) ^ (X[i + 33] & X[i + 28] & X[i + 21]) ^ (X[i + 59] & X[i + 45] & X[i + 28] & X[i + 9]) ^ (X[i + 55] & X[i + 52] & X[i + 37] & X[i + 33]) ^ (X[i + 59] & X[i + 55] & X[i + 21] & X[i + 15]) ^ (X[i + 59] & X[i + 55] & X[i + 52] & X[i + 45] & X[i + 37]) ^ (X[i + 33] & X[i + 28] & X[i + 21] & X[i + 15] & X[i + 9]) ^ (X[i + 52] & X[i + 45] & X[i + 37] & X[i + 33] & X[i + 28] & X[i + 21]);

    /* need Y[i] as linear term only, for invertibility */
    Y[N_LEN_U + i] = Y[i];
    Y[N_LEN_U + i] ^= Y[i + 7] ^ Y[i + 16] ^ Y[i + 20] ^ Y[i + 30] ^ Y[i + 35] ^ Y[i + 37] ^ Y[i + 42] ^ Y[i + 51] ^ Y[i + 54] ^ Y[i + 49] ^ (Y[i + 58] & Y[i + 54]) ^ (Y[i + 37] & Y[i + 35]) ^ (Y[i + 15] & Y[i + 7]) ^ (Y[i + 54] & Y[i + 51] & Y[i + 42]) ^ (Y[i + 35] & Y[i + 30] & Y[i + 20]) ^ (Y[i + 58] & Y[i + 42] & Y[i + 30] & Y[i + 7]) ^ (Y[i + 54] & Y[i + 51] & Y[i + 37] & Y[i + 35]) ^ (Y[i + 58] & Y[i + 54] & Y[i + 20] & Y[i + 15]) ^ (Y[i + 58] & Y[i + 54] & Y[i + 51] & Y[i + 42] & Y[i + 37]) ^ (Y[i + 35] & Y[i + 30] & Y[i + 20] & Y[i + 15] & Y[i + 7]) ^ (Y[i + 51] & Y[i + 42] & Y[i + 37] & Y[i + 35] & Y[i + 30] & Y[i + 20]);

    /* need L[i] as linear term only, for invertibility */
    L[L_LEN_U + i] = L[i];
    L[L_LEN_U + i] ^= L[i + 3];

    /* compute output of the h function */
    h = X[i + 25] ^ Y[i + 59] ^ (Y[i + 3] & X[i + 55]) ^ (X[i + 46] & X[i + 55]) ^ (X[i + 55] & Y[i + 59]) ^ (Y[i + 3] & X[i + 25] & X[i + 46]) ^ (Y[i + 3] & X[i + 46] & X[i + 55]) ^ (Y[i + 3] & X[i + 46] & Y[i + 59]) ^ (X[i + 25] & X[i + 46] & Y[i + 59] & L[i]) ^ (X[i + 25] & L[i]);
    h ^= X[i + 1] ^ Y[i + 2] ^ X[i + 4] ^ Y[i + 10] ^ X[i + 31] ^ Y[i + 43] ^ X[i + 56] ^ L[i];

    /* feedback of h into the registers */
    X[N_LEN_U + i] ^= h;
    Y[N_LEN_U + i] ^= h;
  }

  /* copy final state into hashState */
  for (i = 0; i < N_LEN_U; ++i) {
    x[i] = X[ROUNDS_U + i];
    x[i + N_LEN_U] = Y[ROUNDS_U + i];
  }

  free(X);
  free(Y);
  free(L);

  return 0;
}


int permute_d(u32 *x) {
  /* state of 176=2x88 bits */
#define ROUNDS_D 4 * 176
#define N_LEN_D 88
#define L_LEN_D 10

  u32 *X, *Y, *L;
  u32 h;
  int i;

  X = (u32 *)malloc((N_LEN_D + ROUNDS_D) * sizeof(u32));
  Y = (u32 *)malloc((N_LEN_D + ROUNDS_D) * sizeof(u32));
  L = (u32 *)malloc((L_LEN_D + ROUNDS_D) * sizeof(u32));

  /* local copy of the state in the registers*/
  for (i = 0; i < N_LEN_D; ++i) {
    X[i] = x[i];
    Y[i] = x[i + N_LEN_D];
  }

  /* initialize the LFSR to 11..11 */
  for (i = 0; i < L_LEN_D; ++i)
    L[i] = 0xFFFFFFFF;

  /* iterate rounds */
  for (i = 0; i < ROUNDS_D; ++i) {

    /* need X[i] as linear term only, for invertibility */
    X[N_LEN_D + i] = X[i] ^ Y[i];
    X[N_LEN_D + i] ^= X[i + 11] ^ X[i + 18] ^ X[i + 27] ^ X[i + 36] ^ X[i + 42] ^ X[i + 47] ^ X[i + 58] ^ X[i + 67] ^ X[i + 71] ^ X[i + 64] ^ (X[i + 79] & X[i + 71]) ^ (X[i + 47] & X[i + 42]) ^ (X[i + 19] & X[i + 11]) ^ (X[i + 71] & X[i + 67] & X[i + 58]) ^ (X[i + 42] & X[i + 36] & X[i + 27]) ^ (X[i + 79] & X[i + 58] & X[i + 36] & X[i + 11]) ^ (X[i + 71] & X[i + 67] & X[i + 47] & X[i + 42]) ^ (X[i + 79] & X[i + 71] & X[i + 27] & X[i + 19]) ^ (X[i + 79] & X[i + 71] & X[i + 67] & X[i + 58] & X[i + 47]) ^ (X[i + 42] & X[i + 36] & X[i + 27] & X[i + 19] & X[i + 11]) ^ (X[i + 67] & X[i + 58] & X[i + 47] & X[i + 42] & X[i + 36] & X[i + 27]);


    /* need Y[i] as linear term only, for invertibility */
    Y[N_LEN_D + i] = Y[i];
    Y[N_LEN_D + i] ^= Y[i + 9] ^ Y[i + 20] ^ Y[i + 25] ^ Y[i + 38] ^ Y[i + 44] ^ Y[i + 47] ^ Y[i + 54] ^ Y[i + 67] ^ Y[i + 69] ^ Y[i + 63] ^ (Y[i + 78] & Y[i + 69]) ^ (Y[i + 47] & Y[i + 44]) ^ (Y[i + 19] & Y[i + 9]) ^ (Y[i + 69] & Y[i + 67] & Y[i + 54]) ^ (Y[i + 44] & Y[i + 38] & Y[i + 25]) ^ (Y[i + 78] & Y[i + 54] & Y[i + 38] & Y[i + 9]) ^ (Y[i + 69] & Y[i + 67] & Y[i + 47] & Y[i + 44]) ^ (Y[i + 78] & Y[i + 69] & Y[i + 25] & Y[i + 19]) ^ (Y[i + 78] & Y[i + 69] & Y[i + 67] & Y[i + 54] & Y[i + 47]) ^ (Y[i + 44] & Y[i + 38] & Y[i + 25] & Y[i + 19] & Y[i + 9]) ^ (Y[i + 67] & Y[i + 54] & Y[i + 47] & Y[i + 44] & Y[i + 38] & Y[i + 25]);

    /* need L[i] as linear term only, for invertibility */
    L[L_LEN_D + i] = L[i];
    L[L_LEN_D + i] ^= L[i + 3];  // linear feedback here

    /* compute output of the h function */
    h = X[i + 35] ^ Y[i + 79] ^ (Y[i + 4] & X[i + 68]) ^ (X[i + 57] & X[i + 68]) ^ (X[i + 68] & Y[i + 79]) ^ (Y[i + 4] & X[i + 35] & X[i + 57]) ^ (Y[i + 4] & X[i + 57] & X[i + 68]) ^ (Y[i + 4] & X[i + 57] & Y[i + 79]) ^ (X[i + 35] & X[i + 57] & Y[i + 79] & L[i]) ^ (X[i + 35] & L[i]);
    h ^= X[i + 1] ^ Y[i + 2] ^ X[i + 5] ^ Y[i + 12] ^ X[i + 40] ^ Y[i + 55] ^ X[i + 72] ^ L[i];
    h ^= Y[i + 24] ^ X[i + 48] ^ Y[i + 61];

    /* feedback of h into the registers */
    X[N_LEN_D + i] ^= h;
    Y[N_LEN_D + i] ^= h;
  }

  /* copy final state into hashState */
  for (i = 0; i < N_LEN_D; ++i) {
    x[i] = X[ROUNDS_D + i];
    x[i + N_LEN_D] = Y[ROUNDS_D + i];
  }

  free(X);
  free(Y);
  free(L);

  return 0;
}


int permute_s(u32 *x) {
  /* state of 256=2x128 bits */
#define ROUNDS_S 4 * 256
#define N_LEN_S 128
#define L_LEN_S 10

  u32 *X, *Y, *L;
  u32 h;
  int i;

  X = (u32 *)malloc((N_LEN_S + ROUNDS_S) * sizeof(u32));
  Y = (u32 *)malloc((N_LEN_S + ROUNDS_S) * sizeof(u32));
  L = (u32 *)malloc((L_LEN_S + ROUNDS_S) * sizeof(u32));

  /* local copy of the state in the registers*/
  for (i = 0; i < N_LEN_S; ++i) {
    X[i] = x[i];
    Y[i] = x[i + N_LEN_S];
  }

  /* initialize the LFSR to 11..11 */
  for (i = 0; i < L_LEN_S; ++i)
    L[i] = 0xFFFFFFFF;

  /* iterate rounds */
  for (i = 0; i < ROUNDS_S; ++i) {

    /* need X[i] as linear term only, for invertibility */
    X[N_LEN_S + i] = X[i] ^ Y[i];
    X[N_LEN_S + i] ^= X[i + 16] ^ X[i + 26] ^ X[i + 39] ^ X[i + 52] ^ X[i + 61] ^ X[i + 69] ^ X[i + 84] ^ X[i + 97] ^ X[i + 103] ^ X[i + 94] ^ (X[i + 111] & X[i + 103]) ^ (X[i + 69] & X[i + 61]) ^ (X[i + 28] & X[i + 16]) ^ (X[i + 103] & X[i + 97] & X[i + 84]) ^ (X[i + 61] & X[i + 52] & X[i + 39]) ^ (X[i + 111] & X[i + 84] & X[i + 52] & X[i + 16]) ^ (X[i + 103] & X[i + 97] & X[i + 69] & X[i + 61]) ^ (X[i + 111] & X[i + 103] & X[i + 39] & X[i + 28]) ^ (X[i + 111] & X[i + 103] & X[i + 97] & X[i + 84] & X[i + 69]) ^ (X[i + 61] & X[i + 52] & X[i + 39] & X[i + 28] & X[i + 16]) ^ (X[i + 97] & X[i + 84] & X[i + 69] & X[i + 61] & X[i + 52] & X[i + 39]);

    /* need Y[i] as linear term only, for invertibility */
    Y[N_LEN_S + i] = Y[i];
    Y[N_LEN_S + i] ^= Y[i + 13] ^ Y[i + 30] ^ Y[i + 37] ^ Y[i + 56] ^ Y[i + 65] ^ Y[i + 69] ^ Y[i + 79] ^ Y[i + 96] ^ Y[i + 101] ^ Y[i + 92] ^ (Y[i + 109] & Y[i + 101]) ^ (Y[i + 69] & Y[i + 65]) ^ (Y[i + 28] & Y[i + 13]) ^ (Y[i + 101] & Y[i + 96] & Y[i + 79]) ^ (Y[i + 65] & Y[i + 56] & Y[i + 37]) ^ (Y[i + 109] & Y[i + 79] & Y[i + 56] & Y[i + 13]) ^ (Y[i + 101] & Y[i + 96] & Y[i + 69] & Y[i + 65]) ^ (Y[i + 109] & Y[i + 101] & Y[i + 37] & Y[i + 28]) ^ (Y[i + 109] & Y[i + 101] & Y[i + 96] & Y[i + 79] & Y[i + 69]) ^ (Y[i + 65] & Y[i + 56] & Y[i + 37] & Y[i + 28] & Y[i + 13]) ^ (Y[i + 96] & Y[i + 79] & Y[i + 69] & Y[i + 65] & Y[i + 56] & Y[i + 37]);

    /* need L[i] as linear term only, for invertibility */
    L[L_LEN_S + i] = L[i];
    L[L_LEN_S + i] ^= L[i + 3];  // linear feedback here

    /* compute output of the h function */
    h = X[i + 47] ^ Y[i + 111] ^ (Y[i + 8] & X[i + 100]) ^ (X[i + 72] & X[i + 100]) ^ (X[i + 100] & Y[i + 111]) ^ (Y[i + 8] & X[i + 47] & X[i + 72]) ^ (Y[i + 8] & X[i + 72] & X[i + 100]) ^ (Y[i + 8] & X[i + 72] & Y[i + 111]) ^ (X[i + 47] & X[i + 72] & Y[i + 111] & L[i]) ^ (X[i + 47] & L[i]);
    h ^= X[i + 1] ^ Y[i + 3] ^ X[i + 7] ^ Y[i + 18] ^ X[i + 58] ^ Y[i + 80] ^ X[i + 105] ^ L[i];
    h ^= Y[i + 34] ^ Y[i + 71] ^ X[i + 90] ^ Y[i + 91];

    /* feedback of h into the registers */
    X[N_LEN_S + i] ^= h;
    Y[N_LEN_S + i] ^= h;
  }

  /* copy final state into hashState */
  for (i = 0; i < N_LEN_S; ++i) {
    x[i] = X[ROUNDS_S + i];
    x[i + N_LEN_S] = Y[ROUNDS_S + i];
  }

  free(X);
  free(Y);
  free(L);

  return 0;
}


int permute_c(u32 *x) {
  /* state of 384=2x192 bits */
#define ROUNDS_C 2 * 384
#define N_LEN_C 192
#define L_LEN_C 16

  u32 *X, *Y, *L;
  u32 h;
  int i;

  X = (u32 *)malloc((N_LEN_C + ROUNDS_C) * sizeof(u32));
  Y = (u32 *)malloc((N_LEN_C + ROUNDS_C) * sizeof(u32));
  L = (u32 *)malloc((L_LEN_C + ROUNDS_C) * sizeof(u32));

  /* local copy of the state in the registers*/
  for (i = 0; i < N_LEN_C; ++i) {
    X[i] = x[i];
    Y[i] = x[i + N_LEN_C];
  }

  /* initialize the LFSR to 11..11 */
  for (i = 0; i < L_LEN_C; ++i)
    L[i] = 0xFFFFFFFF;

  /* iterate rounds */
  for (i = 0; i < ROUNDS_C; ++i) {

    X[N_LEN_C + i] = X[i] ^ Y[i];
    X[N_LEN_C + i] ^= X[i + 13] ^ X[i + 34] ^ X[i + 65] ^ X[i + 77] ^ X[i + 94] ^ X[i + 109] ^ X[i + 127] ^ X[i + 145] ^ X[i + 157] ^ X[i + 140] ^ (X[i + 159] & X[i + 157]) ^ (X[i + 109] & X[i + 94]) ^ (X[i + 47] & X[i + 13]) ^ (X[i + 157] & X[i + 145] & X[i + 127]) ^ (X[i + 94] & X[i + 77] & X[i + 65]) ^ (X[i + 159] & X[i + 127] & X[i + 77] & X[i + 13]) ^ (X[i + 157] & X[i + 145] & X[i + 109] & X[i + 94]) ^ (X[i + 159] & X[i + 157] & X[i + 65] & X[i + 47]) ^ (X[i + 159] & X[i + 157] & X[i + 145] & X[i + 127] & X[i + 109]) ^ (X[i + 94] & X[i + 77] & X[i + 65] & X[i + 47] & X[i + 13]) ^ (X[i + 145] & X[i + 127] & X[i + 109] & X[i + 94] & X[i + 77] & X[i + 65]);

    Y[N_LEN_C + i] = Y[i];
    Y[N_LEN_C + i] ^= Y[i + 21] ^ Y[i + 57] ^ Y[i + 60] ^ Y[i + 94] ^ Y[i + 112] ^ Y[i + 125] ^ Y[i + 133] ^ Y[i + 152] ^ Y[i + 157] ^ Y[i + 146] ^ (Y[i + 159] & Y[i + 157]) ^ (Y[i + 125] & Y[i + 112]) ^ (Y[i + 36] & Y[i + 21]) ^ (Y[i + 157] & Y[i + 152] & Y[i + 133]) ^ (Y[i + 112] & Y[i + 94] & Y[i + 60]) ^ (Y[i + 159] & Y[i + 133] & Y[i + 94] & Y[i + 21]) ^ (Y[i + 157] & Y[i + 152] & Y[i + 125] & Y[i + 112]) ^ (Y[i + 159] & Y[i + 157] & Y[i + 60] & Y[i + 36]) ^ (Y[i + 159] & Y[i + 157] & Y[i + 152] & Y[i + 133] & Y[i + 125]) ^ (Y[i + 112] & Y[i + 94] & Y[i + 60] & Y[i + 36] & Y[i + 21]) ^ (Y[i + 152] & Y[i + 133] & Y[i + 125] & Y[i + 112] & Y[i + 94] & Y[i + 60]);

    L[L_LEN_C + i] = L[i] ^ L[i + 2] ^ L[i + 3] ^ L[i + 5];


    h = X[i + 25] ^ Y[i + 59] ^ (Y[i + 3] & X[i + 55]) ^ (X[i + 46] & X[i + 55]) ^ (X[i + 55] & Y[i + 59]) ^ (Y[i + 3] & X[i + 25] & X[i + 46]) ^ (Y[i + 3] & X[i + 46] & X[i + 55]) ^ (Y[i + 3] & X[i + 46] & Y[i + 59]) ^ (X[i + 25] & X[i + 46] & Y[i + 59] & L[i]) ^ (X[i + 25] & L[i]);
    h ^= L[i];
    h ^= X[i + 4] ^ X[i + 28] ^ X[i + 40] ^ X[i + 85] ^ X[i + 112] ^ X[i + 141] ^ X[i + 146] ^ X[i + 152];
    h ^= Y[i + 2] ^ Y[i + 33] ^ Y[i + 60] ^ Y[i + 62] ^ Y[i + 87] ^ Y[i + 99] ^ Y[i + 138] ^ Y[i + 148];

    X[N_LEN_C + i] ^= h;
    Y[N_LEN_C + i] ^= h;
  }

  /* copy final state into hashState */
  for (i = 0; i < N_LEN_C; ++i) {
    x[i] = X[ROUNDS_C + i];
    x[i + N_LEN_C] = Y[ROUNDS_C + i];
  }

  free(X);
  free(Y);
  free(L);

  return 0;
}




/* permutation of the state */
static void permute(u32 *x) {

#ifdef DEBUG
  printf("enter permute\n");
  showstate(x);
#endif

#if defined(UQUARK)
  permute_u(x);
#elif defined(DQUARK)
  permute_d(x);
#elif defined(SQUARK)
  permute_s(x);
#elif defined(CQUARK)
  permute_c(x);
#endif

#ifdef DEBUG
  printf("permute done\n");
  showstate(x);
#endif
}


/* initialization of the IV */
int init_iv(hashState *state) {
  int i;

#ifdef DEBUG
  printf("enter init\n");
#endif

  /* initialize state */
  for (i = 0; i < 8 * WIDTH; ++i)
    state->x[i] = (iv[i / 8] >> (7 - (i % 8))) & 1;

  state->pos = 0;

#ifdef DEBUG
  printf("init done\n");
  showstate(state->x);
#endif

  return 0;
}


int update(hashState *state, const u8 *data, int databytelen) {
  /* caller promises us that previous data had integral number of bytes */
  /* so state->pos is a multiple of 8 */

  int i;

#ifdef DEBUG
  printf("enter update\n");
#endif

  while (databytelen > 0) {

    /* get next byte */
    u8 u = *data;

#ifdef DEBUG
    printf("get byte %02x at pos %d\n", u, state->pos);
#endif

    /* xor state with each bit */
    for (i = 8 * state->pos; i < 8 * state->pos + 8; ++i) {
      state->x[(8 * (WIDTH - RATE)) + i] ^= (u >> (i % 8)) & 1;
    }


    data += 1;
    databytelen -= 1;
    state->pos += 1;

    if (state->pos == RATE) {
      permute(state->x);
      state->pos = 0;
    }
  }

#ifdef DEBUG
  printf("update done\n");
#endif

  return 0;
}


/* finalize (padding) and return digest */
int final(hashState *state, u8 *out) {
  int i;
  int outbytes = 0;
  u8 u;

#ifdef DEBUG
  printf("enter final\n");
#endif

  /* append '1' bit */
  state->x[8 * (WIDTH - RATE) + state->pos * 8] ^= 1;

  /* permute to obtain first final state*/
  permute(state->x);

  /* zeroize output buffer */
  for (i = 0; i < DIGEST; ++i)
    out[i] = 0;

  /* while output requested, extract RATE bytes and permute */
  while (outbytes < DIGEST) {

    /* extract one byte */
    for (i = 0; i < 8; ++i) {
      u = state->x[8 * (WIDTH - RATE) + i + 8 * (outbytes % RATE)] & 1;
      out[outbytes] ^= (u << (7 - i));
    }

#ifdef DEBUG
    printf("extracted byte %02x (%d)\n", out[outbytes], outbytes);
#endif

    outbytes += 1;

    if (outbytes == DIGEST)
      break;

    /* if RATE bytes extracted, permute again */
    if (!(outbytes % RATE)) {
      permute(state->x);
    }
  }

#ifdef DEBUG
  printf("final done\n");
#endif


  return 0;
}


int quark(u8 *out, u8 *in, u64 inlen) {
  /* inlen in bytes */

  hashState state;

  init_iv(&state);
  update(&state, in, inlen);
  final(&state, out);

  return 0;
}
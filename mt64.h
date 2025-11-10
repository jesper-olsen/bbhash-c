/* mt64.h - API for the 64-bit Mersenne Twister module.
 *
 * Copyright (C) 2025, Jesper Olsen <jesper.olsen@gmail.com>
 *
 * This file is part of a module based on the MT19937-64 implementation
 * by Makoto Matsumoto and Takuji Nishimura. The full license details
 * for the original work can be found in the mt64.c source file.
 */

#ifndef MT64_H
#define MT64_H

#include <stdint.h>

typedef struct Mt64 Mt64;

#ifdef __cplusplus
extern "C" {
#endif

// --- Constructor/Destructor API ---
Mt64 *mt64_create(uint64_t seed);
Mt64 *mt64_create_default(void);
Mt64 *mt64_create_by_array(uint64_t init_key[], uint64_t key_length);
void mt64_destroy(Mt64* state);


// --- Random Number Generation API ---
uint64_t mt64_gen_int64(Mt64* state); // [0, 2^64-1]-interval */
int64_t  mt64_gen_int63(Mt64* state); // [0, 2^63-1]-interval */
double   mt64_gen_real1(Mt64* state); // [0,1]-real-interval */
double   mt64_gen_real2(Mt64* state); // [0,1)-real-interval
double   mt64_gen_real3(Mt64* state); // (0,1)-real-interval

#ifdef __cplusplus
}
#endif

#endif // MT64_H

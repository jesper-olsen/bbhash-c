/* mt64.c - 64-bit Mersenne Twister implementation.
 *
 * Copyright (C) 2025, Jesper Olsen <jesper.olsen@gmail.com>
 * Refactored into a stateful, thread-safe module with a modern C API.
 *
 * This module is based on the original work by Makoto Matsumoto and
 * Takuji Nishimura, which is licensed under the 3-Clause BSD license.
 *
 * Original Work:
 * ----------------------------------------------------------------------
 * A C-program for MT19937-64 (2004/9/29 version).
 * Coded by Takuji Nishimura and Makoto Matsumoto.
 *
 * This is a 64-bit version of Mersenne Twister pseudorandom number
 * generator.
 *
 * Before using, initialize the state by using init_genrand64(seed)
 * or init_by_array64(init_key, key_length).
 *
 * Copyright (C) 2004, Makoto Matsumoto and Takuji Nishimura,
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   1. Redistributions of source code must retain the above copyright
 *      notice, this list of conditions and the following disclaimer.
 *
 *   2. Redistributions in binary form must reproduce the above copyright
 *      notice, this list of conditions and the following disclaimer in the
 *      documentation and/or other materials provided with the distribution.
 *
 *   3. The names of its contributors may not be used to endorse or promote
 *      products derived from this software without specific prior written
 *      permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

 * References:
 * T. Nishimura, ``Tables of 64-bit Mersenne Twisters''
 *   ACM Transactions on Modeling and
 *   Computer Simulation 10. (2000) 348--357.
 * M. Matsumoto and T. Nishimura,
 *   ``Mersenne Twister: a 623-dimensionally equidistributed
 *     uniform pseudorandom number generator''
 *   ACM Transactions on Modeling and
 *   Computer Simulation 8. (Jan. 1998) 3--30.

 * Any feedback is very welcome.
 * http://www.math.hiroshima-u.ac.jp/~m-mat/MT/emt.html
 * email: m-mat @ math.sci.hiroshima-u.ac.jp (remove spaces)
 */


#include <stdlib.h> // Required for malloc() and free()
#include "mt64.h"

#define MM 156
#define MATRIX_A 0xB5026F5AA96619E9ULL
#define UM 0xFFFFFFFF80000000ULL /* Most significant 33 bits */
#define LM 0x7FFFFFFFULL /* Least significant 31 bits */
#define MT64_NN 312

// Structure to hold the state of the generator
struct Mt64 {
    uint64_t mt[MT64_NN];
    int mti;
};

/* @brief initialises the state with a seed */
static void init_genrand64(Mt64* state, uint64_t seed)
{
    state->mt[0] = seed;
    for (state->mti = 1; state->mti < MT64_NN; state->mti++)
        state->mt[state->mti] =  (6364136223846793005ULL * (state->mt[state->mti - 1] ^ (state->mt[state->mti - 1] >> 62)) + state->mti);
}

/**
 * @brief initialises the state with an array of seeds
 *
 * @param init_key The array of seeds.
 * @param key_length  The number of elements in the array.
 * @return void
 */
static void init_by_array64(Mt64* state, uint64_t init_key[], uint64_t key_length)
{
    init_genrand64(state, 19650218ULL);
    uint64_t i = 1;
    uint64_t j = 0;
    uint64_t k = (MT64_NN > key_length ? MT64_NN : key_length);
    for (; k; k--) {
        state->mt[i] = (state->mt[i] ^ ((state->mt[i - 1] ^ (state->mt[i - 1] >> 62)) * 3935559000370003845ULL))
                       + init_key[j] + j; /* non linear */
        i++;
        j++;
        if (i >= MT64_NN) {
            state->mt[0] = state->mt[MT64_NN - 1];
            i = 1;
        }
        if (j >= key_length) j = 0;
    }
    for (k = MT64_NN - 1; k; k--) {
        state->mt[i] = (state->mt[i] ^ ((state->mt[i - 1] ^ (state->mt[i - 1] >> 62)) * 2862933555777941757ULL))
                       - i; /* non linear */
        i++;
        if (i >= MT64_NN) {
            state->mt[0] = state->mt[MT64_NN - 1];
            i = 1;
        }
    }

    state->mt[0] = 1ULL << 63; /* MSB is 1; assuring non-zero initial array */
}

Mt64 *mt64_create(uint64_t seed) {
    Mt64* state = (Mt64*)malloc(sizeof(Mt64));

    if (state == NULL) {
        return NULL; // Return NULL to indicate failure
    }

    init_genrand64(state, seed);
    return state;
}

Mt64 *mt64_create_default(void) {
    // The reference implementation and the C++ standard library use seed 5489 by default
    return mt64_create(5489ULL);
}

Mt64 *mt64_create_by_array(uint64_t init_key[], uint64_t key_length) {
    Mt64* state = (Mt64*)malloc(sizeof(Mt64));

    if (state == NULL) {
        return NULL; // Return NULL to indicate failure
    }

    init_by_array64(state, init_key, key_length);
    return state;
}


void mt64_destroy(Mt64 *state) {
    if (state != NULL) {
        free(state);
    }
}

/* generates a random number on [0, 2^64-1]-interval */
uint64_t mt64_gen_int64(Mt64 *state)
{
    int i;
    uint64_t x;
    static uint64_t mag01[2] = {0ULL, MATRIX_A};

    if (state->mti >= MT64_NN) { /* generate NN words at one time */

        for (i = 0; i < MT64_NN - MM; i++) {
            x = (state->mt[i] & UM) | (state->mt[i + 1] & LM);
            state->mt[i] = state->mt[i + MM] ^ (x >> 1) ^ mag01[(int)(x & 1ULL)];
        }
        for (; i < MT64_NN - 1; i++) {
            x = (state->mt[i] & UM) | (state->mt[i + 1] & LM);
            state->mt[i] = state->mt[i + (MM - MT64_NN)] ^ (x >> 1) ^ mag01[(int)(x & 1ULL)];
        }
        x = (state->mt[MT64_NN - 1] & UM) | (state->mt[0] & LM);
        state->mt[MT64_NN - 1] = state->mt[MM - 1] ^ (x >> 1) ^ mag01[(int)(x & 1ULL)];

        state->mti = 0;
    }

    x = state->mt[state->mti++];

    x ^= (x >> 29) & 0x5555555555555555ULL;
    x ^= (x << 17) & 0x71D67FFFEDA60000ULL;
    x ^= (x << 37) & 0xFFF7EEE000000000ULL;
    x ^= (x >> 43);

    return x;
}

/* generates a random number on [0, 2^63-1]-interval */
int64_t mt64_gen_int63(Mt64* state)
{
    return (int64_t) mt64_gen_int64(state) >> 1;
}

/* generates a random number on [0,1]-real-interval */
double mt64_gen_real1(Mt64* state)
{
    return (mt64_gen_int64(state) >> 11) * (1.0 / 9007199254740991.0);
}

/* generates a random number on [0,1)-real-interval */
double mt64_gen_real2(Mt64* state)
{
    return (mt64_gen_int64(state) >> 11) * (1.0 / 9007199254740992.0);
}

/* generates a random number on (0,1)-real-interval */
double mt64_gen_real3(Mt64* state)
{
    return ((mt64_gen_int64(state) >> 12) + 0.5) * (1.0 / 4503599627370496.0);
}

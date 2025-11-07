/* mt64_test.c - tests for 64-bit Mersenne Twister implementation.
 *
 */

#include <stdio.h>
#include <inttypes.h> // PRIu64
#include "mt64.h"

int main(void)
{
    uint64_t init[] = {0x12345ULL, 0x23456ULL, 0x34567ULL, 0x45678ULL};
    uint64_t length = sizeof(init) / sizeof(uint64_t);
    mt64* rng = mt64_create_by_array(init, length);
    printf("1000 outputs of mt64_gen_int64()\n");
    for (int i = 0; i < 1000; i++) {
        printf("%20" PRIu64 " ", mt64_gen_int64(rng));
        if (i % 5 == 4) printf("\n");
    }
    printf("\n1000 outputs of mt64_gen_real2()\n");
    for (int i = 0; i < 1000; i++) {
        printf("%10.8f ", mt64_gen_real2(rng));
        if (i % 5 == 4) printf("\n");
    }
    mt64_destroy(rng);
    return 0;
}


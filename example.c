/* example.c 
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h> // PRIu64
#include "mt64.h"
#include "dedup.h"

int main(int argc, char* argv[argc]) {
   	uint64_t rab = 100;
    uint64_t nelem = 1000000;
    uint64_t nthreads = 1;

   	if(argc !=3 ){
		printf("Usage :\n");
		printf("%s <nelem> <nthreads> \n",argv[0]);
		return EXIT_FAILURE;
	}
	
	if(argc ==3 ){
		nelem = strtoul(argv[1], NULL,0);
		nthreads = atoi(argv[2]);
	}

    size_t size = nelem+rab;
  	uint64_t *data = (uint64_t * ) calloc(size,sizeof(uint64_t));
	
    mt64* rng = mt64_create_default();
	for (size_t i = 0; i < size; i++){
		data[i] = mt64_gen_int64(rng);
	}

    size_t new_size = dedup(data, size);

    printf("found %zu duplicated items\n",size-new_size );

    for (size_t i=0; i<10; i++) {
        printf("%zu: %" PRIu64 "\n", i, data[i]);
    }

    mt64_destroy(rng);
    return 0;
}


#ifndef BBHASH_H
#define BBHASH_H

typedef struct BBHashLevel BBHashLevel;

BBHashLevel *bbhash_mphf_create(uint64_t data[], size_t nelem);
size_t bbhash_mphf_query(BBHashLevel *level, uint64_t key);
void bbhash_level_free(BBHashLevel *level);

#endif

#ifndef BBHASH_H
#define BBHASH_H

typedef struct BBHash BBHash;

BBHash *bbhash_mphf_create(const uint64_t data[], size_t unplaced, double gamma, bool verbose);
size_t bbhash_size_in_bits(const BBHash *mphf);
size_t bbhash_mphf_query(BBHash *level, uint64_t key);
void bbhash_free(BBHash *mphf);

#endif

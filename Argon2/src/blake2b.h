#ifndef BLAKE2B_H
#define BLAKE2B_H

#include <stdbool.h>

struct blake2b_state{

  	uint64_t h[8]; // state vector
  	uint8_t chunk[128]; // input buffer 128 byte
  	uint8_t *cursor; // cursor for input message 
  	size_t byteCompressed; //count
  	size_t byteToCompress; //count
  	bool flagLast; 

} blake2b_state;

void blake2b_long(void *out, size_t outlen, void *input, size_t inlen);
void blake2b_init(struct blake2b_state *state, void *m, size_t mlen, size_t outlen);
void getChunk(struct  blake2b_state *state);
void compress(struct blake2b_state *state); 
void mix(uint64_t *v, int a, int b, int c, int d, uint64_t x, uint64_t y);



/**
 * Table of permutations
 */
static const uint8_t sigma[12][16] = {
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
  { 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 },
  { 11, 8, 12, 0, 5, 2, 15, 13, 10, 14, 3, 6, 7, 1, 9, 4 },
  { 7, 9, 3, 1, 13, 12, 11, 14, 2, 6, 5, 10, 4, 0, 15, 8 },
  { 9, 0, 5, 7, 2, 4, 10, 15, 14, 1, 11, 12, 6, 8, 3, 13 },
  { 2, 12, 6, 10, 0, 11, 8, 3, 4, 13, 7, 5, 15, 14, 1, 9 },
  { 12, 5, 1, 15, 14, 13, 4, 10, 0, 7, 6, 3, 9, 2, 8, 11 },
  { 13, 11, 7, 14, 12, 1, 3, 9, 5, 0, 15, 4, 8, 6, 2, 10 },
  { 6, 15, 14, 9, 11, 3, 0, 8, 12, 2, 13, 7, 1, 4, 10, 5 },
  { 10, 2, 8, 4, 7, 6, 1, 5, 15, 11, 9, 14, 3, 12, 13, 0 },
  { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 },
  { 14, 10, 4, 8, 9, 15, 13, 6, 1, 12, 0, 2, 11, 7, 5, 3 }
};


/**
* Initialization vector
*/
static const uint64_t iv[8]={
				0x6A09E667F3BCC908, 
				0xBB67AE8584CAA73B,
				0x3C6EF372FE94F82B, 
				0xA54FF53A5F1D36F1,
				0x510E527FADE682D1,
				0x9B05688C2B3E6C1F,
				0x1F83D9ABFB41BD6B, 
				0x5BE0CD19137E2179}; 


#endif
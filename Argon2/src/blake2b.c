#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#include <stdbool.h>
#include "utils.h"
#include "blake2b.h"


// Update state->chunk with 128-byte portion of input message input
void getChunk(struct  blake2b_state *state){ 
	memcpy(state->chunk,state->cursor, 128); 
	state->cursor+=128;
	state->byteCompressed+=128;
	state->byteToCompress-=128;
}
//----------------------------------------------


/*
* Blake2b algortihm ad hoc designed for Argon2
*/


// Initialization function
void blake2b_init(struct blake2b_state *state, void *input, size_t inlen, size_t outlen){

	// Check for overflow
	if(inlen>pow(2,64)){
		fprintf(stderr, "Blake2b: input string too long\n");
		exit(-1);}

	// Initialize state vector
	for(int i=0; i<8; i++)
		state->h[i] = iv[i];
	state->h[0]=state->h[0]^0x01010000^((uint64_t)outlen);
	state->byteCompressed=0;
	state->byteToCompress=inlen;
	state->cursor=input;
	state->flagLast=inlen<128;
	
}
//----------------------------------------------


// Mixing function
void mix(uint64_t *v, int a, int b, int c, int d, uint64_t x, uint64_t y){
	
	v[a]=v[a]+v[b]+x;
	v[d]=rot_right64((v[d]^v[a]),32);

	v[c]=v[c]+v[d];
	v[b]=rot_right64((v[b]^v[c]),24);

	v[a]=v[a]+v[b]+y;
	v[d]=rot_right64((v[d]^v[a]),16);

	v[c]=v[c]+v[d];
	v[b]=rot_right64((v[b]^v[c]), 63);

}
//----------------------------------------------


// Compression function
void compress(struct blake2b_state *state){
	uint64_t v[16]; //local work vector
	uint64_t m[16]; // chunk's register

	for(int i=0; i<8; i++){
		v[i]=state->h[i];
		v[i+8]=iv[i];
	}

	v[12]=v[12]^state->byteCompressed;
	//v[13]=v[13]^t[1];

	

	if(state->flagLast)
		v[14]=~v[14];

	// Treat a 128-byte chunk as 16 8-byte words m
	for(int i=0; i<16; i++)
		memcpy(m+i,(state->chunk)+(8*i), 8); 
	
	// Twelve rounds
	for(int i=0; i<12; i++){
		mix(v, 0, 4, 8, 12, m[sigma[i][0]], m[sigma[i][1]]);
		mix(v, 1, 5, 9, 13, m[sigma[i][2]], m[sigma[i][3]]);
		mix(v, 2, 6, 10, 14, m[sigma[i][4]], m[sigma[i][5]]);
		mix(v, 3, 7, 11, 15, m[sigma[i][6]], m[sigma[i][7]]);

		mix(v, 0, 5, 10, 15, m[sigma[i][8]], m[sigma[i][9]]);
		mix(v, 1, 6, 11, 12, m[sigma[i][10]], m[sigma[i][11]]);
		mix(v, 2, 7, 8, 13, m[sigma[i][12]], m[sigma[i][13]]);
		mix(v, 3, 4, 9, 14, m[sigma[i][14]], m[sigma[i][15]]);	
	}

	// State update
	for(int i=0; i<8; i++)
		state->h[i]=state->h[i]^v[i]^v[i+8];
	
	}	
//----------------------------------------------


// All in one function
void blake2b_long(void *out, size_t outlen, void *m, size_t mlen){
	struct blake2b_state state;
	blake2b_init(&state, m, mlen, outlen);
	while(state.byteToCompress > 128){ 
		getChunk(&state);
		compress(&state);
	}
	
	// Last chunk 
	state.flagLast=true; //update flag
	memset(state.chunk, ((uint8_t) 0), 128); // final chunk copied into state.chunk padded with zeros
	memcpy(state.chunk, state.cursor, state.byteToCompress);
	state.byteCompressed+=state.byteToCompress; // update state.byte counters
	compress(&state); // last chunk compression

	// Output vector
	memcpy(out, state.h, outlen);	
}
//----------------------------------------------

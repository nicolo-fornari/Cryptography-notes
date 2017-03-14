#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "utils.h"
#include "argon.h"
#include "hash_functions.h"
#include "compression_functions.h"

/*
There are four slices
A lane is a row, q is the number of columns.
The intersection of a slice and a lane is a segment.
In a segment there are q/4 blocks!

0     q/4   q/2   3q/4   q
+-----+-----+-----+-----+
|     |     |     |#####| <-- segment 
|     |     |     |     |
|     |     |     |     |
|     |     |     |     | <-- lane 
+-----+-----+-----+-----+

*/

// Create matrices in structure Segment
void init_segment(struct Context *ctx,struct Segment *seg) {

  	// counter[i][j] used in fetch_ij_prime
	// each lane has 4 segment 
	// the counter of segment j in lane i is initialized to 1
  	seg->counter = (uint8_t **)malloc(ctx->lanes * sizeof(uint8_t *));
  	malloc_check_double_pointer(seg->counter, "Segment counter as double pointer");
  	for(int i = 0; i < ctx->lanes; i++) {
    	seg->counter[i] = (uint8_t *)malloc(4 * sizeof(uint8_t));
    	malloc_check(seg->counter[i], "Segment counter");
    	for(int j = 0; j < 4; j++)
      		seg->counter[i][j] = 1;
  	}

  	// used_pairs[i][j] tells me how many pairs I have used so far
  	// when it reaches 128 then compute_J1_J2 has to be invoked 
  	seg->used_pairs = (uint8_t **)malloc(ctx->lanes * sizeof(uint8_t *));
  	malloc_check_double_pointer(seg->used_pairs,"Segment used_pairs as double pointer");
  	for(int i = 0; i < ctx->lanes; i++) {
    	seg->used_pairs[i] = (uint8_t *)malloc(4 * sizeof(uint8_t));
    	malloc_check(seg->used_pairs[i],"Segment used_pairs");
    	for(int j = 0; j < 4; j++)
      		seg->used_pairs[i][j] = 0;
  	}

  	// couples[i][j] is a 1024 byte vector which gives me 128 couples (J1,J2) of 64 bit
  	seg->couples = (uint8_t ***)malloc(ctx->lanes * sizeof(uint8_t **));
  	malloc_check_triple_pointer(seg->couples,"Segment couples as triple pointer");
  	for(uint8_t i = 0; i < ctx->lanes; i++) {
    	seg->couples[i] = (uint8_t **)malloc(4 * sizeof(uint8_t *));
    	malloc_check_double_pointer(seg->couples[i],"Segment couples as double pointer");
    	for(int j = 0; j < 4; j++) {
    	  	seg->couples[i][j] = (uint8_t *)malloc(1024 * sizeof(uint8_t));
    	  	malloc_check(seg->couples[i][j], "Segment couples");
    	}
  	}

}

// Compute J1,J2 (each 32 bits) used by the indexing function to obtain i',j'
void compute_J1_J2(uint32_t *J1, uint32_t *J2,uint8_t ***B, struct Context *ctx, struct Segment *seg, int i, int j, int current_pass) {
	// otherwise I get -1 in B[i][j-1] so I assumed
	// it is modular even though the specific does not say so
	if (j == 0) 
		j = ctx->q-1;
	

	if (ctx->type == 0) { // Argon2d
		memcpy(J1,B[i][j-1],4);
		memcpy(J2,B[i][j-1]+4,4); // next 32 bits
	}

	else if (ctx->type == 1) { // Argon2i
		uint8_t *first_result = malloc(1024);
		uint8_t *first_input = malloc(1024);
		uint8_t *second_input = malloc(1024);
		malloc_check(first_result, "first result in fetch_ij_prime");
		malloc_check(first_input,"first input in fetch_ij_prime");
		malloc_check(second_input,"second_input in fetch_ij_prime");

		memset(first_input,0,1024);

		uint64_t r = current_pass;
		uint64_t lane_number = i;
		uint64_t slice_number = j/(ctx->q/4); // column number/semgent length in block
		uint64_t total_memory = ctx->memory_requested;
		uint64_t total_passes = ctx->number_of_passes;
		uint64_t one = 1;
		uint64_t counter = seg->counter[i][slice_number]; 

		void *cursor = memcpy(second_input,&r,8);
		cursor = memcpy(cursor+8,&lane_number,8);
		cursor = memcpy(cursor+8,&slice_number,8);
		cursor = memcpy(cursor+8,&total_memory,8);
		cursor = memcpy(cursor+8,&total_passes,8);
		cursor = memcpy(cursor+8,&one,8); // type of Argon2i
		cursor = memcpy(cursor+8,&counter,8);
		memset(cursor+8,0,968);

		// first_result= G([0..0],[r||l||s||m||t||x||1||0])
		compression_G(first_result,first_input,second_input);

		// G([0..0], G([0..0],[r||l||s||m||t||x||1||0]))
		compression_G(seg->couples[i][slice_number], first_input, first_result);

		seg->counter[i][slice_number]++;

		free(first_input);
		free(second_input);
		free(first_result);
	}

}

// Compute i',j', necessary to reference blocks
void fetch_ij_prime(uint8_t ***B, struct Context *ctx, int i, int j, int current_pass,uint32_t J1, uint32_t J2, int *i_prime, int *j_prime) {

	// otherwise I get -1 in B[i][j-1] so I assumed
	// it is modular even though the specific does not say so
	if (j == 0) 
		j = ctx-> q-1;
	
	
	// J1, J2 mapping to the reference block index
	int segmentBound = ctx->q/4;
	int currentSlice = j/segmentBound;
	int lane = J2 % ctx->lanes; // index of the lane from which the block will be taken
	int R[ctx->q]; // set of indices
	int R_cardinality = 0;
	
	if(lane < 0 ) // least positive residue
		lane = lane + ctx-> lanes;

	int h=0;
	if (current_pass == 0) {
		if (currentSlice == 0) { // take blocks already computed in the current segment, i.e. those on the left but B[i][j-1]
			lane = i;
			for (int k=0;k < j-1;k++) {
				R[h] = k;
				h++;
				R_cardinality++;
			}
		}
		else { // take blocks on the left but those in the current segment
			for (int s=0;s<currentSlice;s++) {
				for (int k=0; k < segmentBound; k++) {
					R[h] = s+k;
					h++;
					R_cardinality++;
				}
			}
		}

	}
	else {
		//int start_specs = j+1;
		int start_code = (currentSlice+1)*segmentBound;

		if (lane == i) {
			
			// take blocks in all segments on the right 
			for (int k=start_code;k<ctx->q;k++) {
				R[h] = k;
				h++;
				R_cardinality++;
			}
			
			// take blocks on the left but B[i][j-1]
			for (int k=0;k<j-1;k++) {
				R[h] = k;
				h++;
				R_cardinality++;
			}
		}

		else {
			
			// take blocks in all segments on the right 			
			for (int k=start_code;k<ctx->q;k++) {
				R[h] = k;
				h++;
				R_cardinality++;				
			}

			// take blocks in all segment on the left, but the current
			for (int k=0;k<segmentBound*currentSlice;k++) {
				R[h] = k;
				h++;
				R_cardinality++;				
			}
			// if current block is the first of the segment 
			// remove B[i][j-1]
			if (j%segmentBound == 0) {
				R_cardinality--;
			}
		}
	}

	// using non uniform distribution to extract i' j'

	int x = pow(J1,2)/pow(2,32);
	int y = (R_cardinality*x)/pow(2,32);
	int z = R_cardinality - 1 - y;

	*i_prime = lane;
	*j_prime = R[z];

}


// fills B[i][0] or B[i][1] for the first pass, 0 <= i < p
void matrix_init(uint8_t ***B, uint8_t *H0,struct Context *ctx,int column) {
	uint32_t four_bytes = 0;
	if (column == 1)
			four_bytes = 1;

	uint8_t *X = (uint8_t *)malloc(72); // 64+4+4 as H0 || 0 || i
	uint8_t *H_prime = (uint8_t *)malloc(1024);

	malloc_check(X,"H_prime_argument");
	malloc_check(H_prime, "H_prime in matrix_init");	
	
	void *cursor;

	for (uint32_t i=0;i < ctx->lanes;i++) {

		// H0 || 0 or 1 || i
		cursor = memcpy(X,H0,64);
		cursor = memcpy(cursor+64,&four_bytes,4);
		cursor = memcpy(cursor+4,&i,4);

		// H'(H0 || 0 or 1 || i)
		compute_H_prime(H_prime, X, 72,1024); 

		memcpy(B[i][column],H_prime, 1024); 
	}

	free(H_prime);
	free(X);
}

// For t=0, fills blocks in segment at lane i and slice s 
void matrix_first_pass(uint8_t ***B, struct Context *ctx,struct Segment *seg,int i, int slice) {
	int i_prime;
	int j_prime;

	int segmentBound = ctx->q/4;

	for (int j=slice*segmentBound;j<(slice+1)*segmentBound; j++) {

		if (j > 1) { // blocks in columns 0 and 1 are alredy computed by matrix_init
			uint32_t J1;
			uint32_t J2;

			if (ctx->type == 0) {
				compute_J1_J2(&J1,&J2,B,ctx,seg,i,j,0);
				fetch_ij_prime(B,ctx,i,j,0,J1,J2,&i_prime,&j_prime);
			}
			else if (ctx->type == 1) {
				int cur_pair = seg->used_pairs[i][slice];

				if (cur_pair >= 128) { // I have used all the couple J1,J2. I need to compute new ones
					compute_J1_J2(&J1,&J2,B,ctx,seg,i,j,0);
					seg->used_pairs[i][slice] = 0;
					cur_pair = 0;
				}
				
				memcpy(&J1,seg->couples[i][slice]+cur_pair*8,4);
				memcpy(&J2,seg->couples[i][slice]+cur_pair*8+4,4);
				seg->used_pairs[i][slice]++;
				
				fetch_ij_prime(B,ctx,i,j,0,J1,J2,&i_prime,&j_prime);
			}
			

			// B[i][j] = G(B[i][j-1], B[i'][j'])
			compression_G(B[i][j],B[i][j-1],B[i_prime][j_prime]);
		}		
		
	}

}

// Fill blocks in segment at lane i and slice s 
void matrix_fill_blocks(uint8_t ***B, struct Context *ctx, struct Segment *seg, int current_pass, int i, int slice) {
	int segmentBound = ctx->q/4;

	for (int j=slice*segmentBound;j<(slice+1)*segmentBound; j++) {

		int i_prime, j_prime;
		uint32_t J1;
		uint32_t J2;
		//int currentSegment = j/(ctx->q/4);

		if (ctx->type == 0) {
			compute_J1_J2(&J1,&J2,B,ctx,seg,i,j,current_pass);
			fetch_ij_prime(B,ctx,i,j,current_pass,J1,J2,&i_prime,&j_prime);
		}
		else if (ctx->type == 1) {
			int cur_pair = seg->used_pairs[i][slice];

			if (cur_pair >= 128) { // I have used all the couple J1,J2. I need to compute new ones
				compute_J1_J2(&J1,&J2,B,ctx,seg,i,j,current_pass);
				seg->used_pairs[i][slice] = 0;
				cur_pair = 0;
			}
				
			memcpy(&J1,seg->couples[i][slice]+cur_pair*8,4);
			memcpy(&J2,seg->couples[i][slice]+cur_pair*8+4,4);
			
			seg->used_pairs[i][slice]++;
			
			fetch_ij_prime(B,ctx,i,j,current_pass,J1,J2,&i_prime,&j_prime);
		}
		
		uint8_t result_of_G[1024];
		uint8_t result_of_XOR[1024];

		if (j == 0) {
			compression_G(result_of_G, B[i][ctx->q -1], B[i_prime][j_prime]);	
			xor_blocks(result_of_XOR,result_of_G, B[i][0]);
		}
		else {
			compression_G(result_of_G, B[i][j-1], B[i_prime][j_prime]);
			xor_blocks(result_of_XOR, result_of_G, B[i][j]);
		}
		memcpy(B[i][j], result_of_XOR, 1024);
	}
	
	
}
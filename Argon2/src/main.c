#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <math.h>
#include <time.h>
#include <omp.h>
#include "argon.h"
#include "core.h"
#include "compression_functions.h"
#include "hash_functions.h"
#include "utils.h"
#include "usage.h"

// Handy for usleep in case we want to see the memory filled
#define SECOND 1000000

int main(int argc, char *argv[]) {
  	struct Context ctx;
	struct Segment seg;

  // ------------------ User input ------------------- //

  	// Set default values to avoid passing parameters 
    // via CLI during development
  	set_default_values(&ctx);

  	// Assign user's parameters to variables in struct Context
    parse_options(argc, argv,&ctx);

  	// Compute the lengths of the various strings in Context
  	compute_lengths(&ctx);

  	// Check if the user has provided wrong paramters
  	check_params(&ctx);

  	// rounding down to the nearest multiple of 4p
  	ctx.memory_requested  = floor(ctx.memory_requested/(4*ctx.lanes))*4*ctx.lanes;
  	ctx.q = ctx.memory_requested / ctx.lanes; // number of columns

  	// Show the used parameters
  	show_params(&ctx);


  // ------------------ Actual Algorithm ------------------- //

  	uint8_t H0[64];
  	compute_H0(&ctx,H0);

  	// If version Argon2i is running, create matrices in structure Segment
  	if(ctx.type==1)
  		init_segment(&ctx,&seg);
  	


  	// Allocating memory in matrix B
  	uint8_t ***B = (uint8_t ***)malloc(ctx.lanes * sizeof(uint8_t **));
  	malloc_check_triple_pointer(B,"B");
  	for(uint8_t i = 0; i < ctx.lanes; i++) {
    	B[i] = (uint8_t **)malloc(ctx.q * sizeof(uint8_t *));
    	malloc_check_double_pointer(B[i],"B[i]");
    	for(int j = 0; j < ctx.q; j++) {
      		B[i][j] = (uint8_t *)malloc(1024 * sizeof(uint8_t));
      		malloc_check(B[i][j],"B[i][j]");
      		memset(B[i][j],0,1024);
    	}
    
  	}

  // ------------------ First pass (t=0) ------------------- //

 	matrix_init(B,H0,&ctx,0);
  	matrix_init(B,H0,&ctx,1);

  	// all the other columns
  	for (int s=0;s<4;s++) {
    	#pragma omp parallel num_threads(ctx.lanes)
      	{
      
      		#pragma omp for
        		for (int i=0;i<ctx.lanes;i++) 
          			// fill blocks in segment given slice s and lane i
          			matrix_first_pass(B,&ctx,&seg,i,s);
      	}  
  	}
 

  
  // ------------------ Other passes (t>0)  ---------------- //

	for (int t=1;t < ctx.number_of_passes;t++) {

    	for (int s=0;s<4;s++) { // cycle over slices
      		#pragma omp parallel num_threads(ctx.lanes)
      		{	
      
	    		#pragma omp for
        			for (int i=0;i<ctx.lanes;i++) 
          				// fill blocks in segment given slice s and lane i
          				matrix_fill_blocks(B,&ctx,&seg,t,i,s); 
      		}  
    	}    
  	}

	uint8_t B_final[1024];
    memset(B_final, ((uint8_t) 0), 1024);
    // B_final is the xor of the last column
    for (int i=0;i<ctx.lanes;i++) 
      	xor_blocks(B_final,B_final,B[i][ctx.q-1]);
    	
  	ctx.out = malloc(ctx.tau); 
  	malloc_check(ctx.out,"Final hash"); 
  	compute_H_prime(ctx.out, B_final, 1024,ctx.tau); 

  	print_hex(ctx.out,ctx.tau);
  
  	// Uncomment if you want to see the memory filled from system monitor
  	// or from other utilities
  	//usleep(3*SECOND);


   // ------------------ Free allocated memory  ---------------- //

  	for(uint8_t i = 0; i < ctx.lanes; i++) {
    	for(int j = 0; j < ctx.q; j++) {
      		free(B[i][j]);
    	}
    	free(B[i]);
  	}

  	free(B);

  	if(ctx.type==1){ //If Argon2i, Segment to be freed
		for(int i = 0; i < ctx.lanes; i++) {
	    	for(int j = 0; j < 4; j++) {
	      		free(seg.couples[i][j]);
	    	}
	    	free(seg.counter[i]);
	    	free(seg.used_pairs[i]);
	    	free(seg.couples[i]);
	  	}
	  	free(seg.counter);
	  	free(seg.used_pairs);
	  	free(seg.couples);
	}

	free(ctx.out);

	return 0;
}
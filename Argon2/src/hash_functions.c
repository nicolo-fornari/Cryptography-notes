#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "argon.h"
#include "utils.h"
#include "math.h"
#include "blake2b.h"

/* If using Blake2b from Argon repository
#include "blake2-argon/blamka-round-ref.h"
#include "blake2-argon/blake2-impl.h"
#include "blake2-argon/blake2.h"
*/


// H0 is Blake2b with digest's length of 64 bits
void compute_H0(struct Context *ctx, uint8_t *H0){
	// concatenate the inputs with their lengths and other parameters
	int lengthSum = ctx->pwdlen+ctx->saltlen+ctx->secretlen+ctx->data_arraylen+4*10;

	void * cursor;
	uint8_t *params_concatenation = (uint8_t *)malloc(lengthSum);
	malloc_check(params_concatenation,"params concatenation");

	cursor = memcpy(params_concatenation, ctx->pwd, ctx->pwdlen);
	cursor = memcpy(cursor+ctx->pwdlen, ctx->salt, ctx->saltlen);
	cursor = memcpy(cursor+ctx->saltlen, ctx->secret,ctx->secretlen);
	cursor = memcpy(cursor+ctx->secretlen, ctx->data_array, ctx->data_arraylen);
	cursor = memcpy(cursor+ctx->data_arraylen, &ctx->pwdlen, 4);
	cursor = memcpy(cursor+4,&ctx->saltlen, 4);
	cursor = memcpy(cursor+4,&ctx->secretlen, 4);
	cursor = memcpy(cursor+4,&ctx->data_arraylen,4);
	cursor = memcpy(cursor+4,&ctx->number_of_passes,4);
	cursor = memcpy(cursor+4,&ctx->memory_requested,4);
	cursor = memcpy(cursor+4,&ctx->lanes,4);
	cursor = memcpy(cursor+4,&ctx->tau,4);
	cursor = memcpy(cursor+4,&ctx->version,4);
	cursor = memcpy(cursor+4,&ctx->type,4);

	// CHECK FOR LITTLE ENDIAN
	blake2b_long(H0,64,params_concatenation,lengthSum);
	free(params_concatenation);
}

// Variable length hash function based on Blake2b
// Remark: the user's supplied length "tau" is applied only to the final block, B_final
// In the other cases tau = 1024 which is the block size
void compute_H_prime(uint8_t *H_prime, uint8_t *X, int X_len, uint32_t currentTau) {
	
	uint8_t *tau_padding_X = (uint8_t *)malloc(4+X_len);

	//malloc_check(H_64,"H_64");	
	malloc_check(tau_padding_X,"tau_padding_X");

	// computing tau || X
	void *cursor = memcpy(tau_padding_X,&currentTau,4);
	memcpy(cursor+4,X,X_len);

	if (currentTau <= 64) {
		// H' = H_tau( tau || X)
		blake2b_long(H_prime,currentTau,tau_padding_X,4+X_len);
	}

	else {
		uint32_t r = (uint32_t)ceil((float)currentTau/32)-2;
		uint8_t V1[64];
		uint8_t V2[64];

		// V1 = H_64(tau || X)
		blake2b_long(V1,64,tau_padding_X,4+X_len);

		if (r > 1) {

			// H_prime = H_prime || A1
			void *pos = memcpy(H_prime, V1,32);

			for (int i = 2; i < r+1; i++) {
				// V2 = H_64(V1)
				blake2b_long(V2,64,V1,64);
				memcpy(V1,V2,64); // V1 = V2

				// H_prime = H_prime || A_i
				pos = memcpy(pos+32,V2,32);
			}

			uint8_t *V_final = (uint8_t *)malloc(currentTau-32*r);
			malloc_check(V_final,"V_final");

			// V_(r+1) = H_(tau-32r)(V_r)
			blake2b_long(V_final,currentTau-32*r, V2,64);

			// H_prime = H_prime || V_(r+1)
			memcpy(pos+32,V_final,currentTau-32*r);
			free(V_final);
		}
		else { 
			// for r=1 -> H' = A1 || V2

			// A1 is the first 32 byte of V1
			void *pos = memcpy(H_prime, V1,32);

			// V2 = H64(V1)
			blake2b_long(V2,currentTau-32,V1,64);

			memcpy(pos+32,V2,currentTau-32);
		}

		
	}

	free(tau_padding_X);
}
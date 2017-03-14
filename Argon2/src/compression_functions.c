#include <inttypes.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "utils.h"
#include "argon.h"
#include "core.h"
#include "hash_functions.h"

// xL truncates the argument to its 32 least significant bits
uint64_t xL(uint64_t x) {
	uint32_t maxVal = 0xffffffff;
	return x & (uint64_t)maxVal;
}

// Round function based on Blake2b's one
// A,B,C,D are 8 byte
// a,b,c,d are 64 bit
void apply_G(uint8_t *A,uint8_t *B,uint8_t *C,uint8_t *D) {

	uint64_t a,b,c,d;
	
	memcpy(&a,A,8); // from A to a
	memcpy(&b,B,8); 
	memcpy(&c,C,8);
	memcpy(&d,D,8);

	a = a+b+2*xL(a)*xL(b);
	d = rot_right64((d ^ a),32);
	c = c+d+2*xL(c)*xL(d);
	b = rot_right64((b ^ c),24);
	a = a+b+2*xL(a)*xL(b);
	d = rot_right64((d ^ a),16);
	c = c+d+2*xL(c)*xL(d);
	b = rot_right64((b ^ c),63);

	memcpy(A,&a,8); // from a to A
	memcpy(B,&b,8);
	memcpy(C,&c,8);
	memcpy(D,&d,8);
}


// Permutation P
// Parameters: S0,..S7 of 16 byte each.
//
// They are viewed as a 4x4 matrix V with each entry v_i of 64 bit (or 8 bytes)
// S_i = (v_{2i+1} || v_{2i]})
//
// | v_0 v_1 v_2 v_3 |
// | v_4 v_5 v_6 v_7 |
// | . . . . . . . . |
// | . . . . . .v_15 |
//
// Then G is applied to all the columns and to the diagonals

void P(uint8_t *S0,uint8_t *S1,uint8_t *S2,uint8_t *S3,uint8_t *S4,uint8_t *S5,uint8_t *S6,uint8_t *S7) {

	uint8_t *S[8] = {S0,S1,S2,S3,S4,S5,S6,S7};
	uint8_t V[4][4][8];

	// from S0,..S7 to V
	int h;
	for (int i=0;i<4; i++) {
		for (int j=0;j<4;j++) {
			h = 4*i+j;
			if (h % 2 == 0) // h even 
				memcpy(V[i][j], S[(int)floor(h/2)]+8,8);
			else
				memcpy(V[i][j], S[(int)floor(h/2)],8);
		}
	}
	
	apply_G(V[0][0],V[1][0],V[2][0],V[3][0]); // first column
	apply_G(V[0][1],V[1][1],V[2][1],V[3][1]); // second
	apply_G(V[0][2],V[1][2],V[2][2],V[3][2]); // third
	apply_G(V[0][3],V[1][3],V[2][3],V[3][3]); // fourth

	apply_G(V[0][0],V[1][1],V[2][2],V[3][3]); // first diagonal
	apply_G(V[0][1],V[1][2],V[2][3],V[0][3]); // second upper
	apply_G(V[0][2],V[1][3],V[2][0],V[3][1]); // third
	apply_G(V[0][3],V[1][0],V[2][1],V[3][2]); // fourth

	// from V back to S0,..S7
	for (int i=0;i<4; i++) {
		for (int j=0;j<4;j++) {
			h = 4*i+j;
			if (h % 2 == 0) // h even
				memcpy(S[(int)floor(h/2)]+8,V[i][j],8); 
			else
				memcpy(S[(int)floor(h/2)],V[i][j],8);
		}
	}
}

// Compression function G
// Result = G(X,Y) where X,Y are 1024 byte blocks
// R = X ^ Y
// Then R is viewed as a matrix 8x8 with each entry of 16 byte
//
// | R_0 R_1 . . . . R_7 |
// | . . . . . . . . . . |
// | . . . . . . . . . . |
// | . . . . . . . .R_63 | 
//
//
// P is applied to all the rows,
// P(R_0 .. R_7) = Q_0 . . . Q_7 ecc
//
// then to all the columns
// P(Q_0 Q_8 Q_16 Q_24 Q_32 Q_40 Q_48 Q_56 Q_64) = (Z_0 Z_8 . .) 
//
// Finally X ^ Y ^ R

void compression_G(uint8_t *Result, uint8_t *X, uint8_t *Y) {
	
	uint8_t R_array[1024];
	xor_blocks(R_array,X,Y);

	// R = X ^ Y is 1024 bytes is viewed as 8x8 matrix of 16 bytes for entry
	uint8_t R[8][8][16];

	// from array to matrix
	int h = 0;
	for (int i=0;i<8;i++) {
		for (int j=0;j<8;j++) {
			memcpy(R[i][j],R_array+16*h,16);
			h++;
		}
	}

	// apply P to all the rows
	for (int i=0; i< 8;i++) {
		P(R[i][0],R[i][1],R[i][2],R[i][3],R[i][4],R[i][5],R[i][6],R[i][7]);
	}

	// apply P to all the columns
	for (int j=0; j< 8;j++) {
		P(R[0][j],R[1][j],R[2][j],R[3][j],R[4][j],R[5][j],R[6][j],R[7][j]);
	}

	// from matrix to array
	h=0;
	for (int i=0;i<8;i++) {
		for (int j=0;j<8;j++) {
			memcpy(R_array+16*h,R[i][j],16);
			h++;
		}
	}

	xor_blocks(Result, X, Y);
	xor_blocks(Result, Result, R_array);
}
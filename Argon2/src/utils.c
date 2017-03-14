#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "argon.h"

// Check for malloc failures

void malloc_check_triple_pointer(uint8_t ***check_me, char * name) {
    if (check_me == NULL) {
        fprintf(stderr, "%s malloc error\n", name);
        exit(-1);
    }
}

void malloc_check_double_pointer(uint8_t **check_me, char * name) {
    if (check_me == NULL) {
        fprintf(stderr, "%s malloc error\n", name);
        exit(-1);
    }
}

void malloc_check(uint8_t *check_me, char * name) {
    if (check_me == NULL) {
        fprintf(stderr, "%s malloc error\n", name);
        exit(-1);
    }
}

// Print a byte array in hexadecimal
void print_hex(uint8_t bytes[], int bytes_len) {
  for (int i=0; i < bytes_len; i++) {
    printf("%02x", bytes[i]);
  }
  printf("\n");
}

// Rotational shift to the right
uint64_t rot_right64(uint64_t w, unsigned c) {
    return (w >> c) | (w << (64 - c));
}

// Bitwise xor of 1024 bytes long blocks
void xor_blocks(uint8_t *Result, uint8_t *block1, uint8_t *block2) {
    for (int i=0; i<1024; i++) {
        Result[i] = block1[i] ^ block2[i];
    }
}


void malloc_check(uint8_t *check_me, char * name);

void malloc_check_double_pointer(uint8_t **check_me, char * name);

void malloc_check_triple_pointer(uint8_t ***check_me, char * name);

void print_hex(uint8_t bytes[], int bytes_len);

void xor_blocks(uint8_t *Result, uint8_t *block1, uint8_t *block2);

uint64_t rot_right64(uint64_t w, unsigned c);

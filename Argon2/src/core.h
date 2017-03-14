void matrix_init(uint8_t ***B, uint8_t *H0,struct Context *ctx,int column);

void compute_J1_J2(uint32_t *J1, uint32_t *J2,uint8_t ***B, struct Context *ctx, struct Segment *seg, int i, int j, int current_pass);

void fetch_ij_prime(uint8_t ***B, struct Context *ctx, int i, int j, int current_pass,uint32_t J1, uint32_t J2, int *i_prime, int *j_prime);

void matrix_first_pass(uint8_t ***B, struct Context *ctx,struct Segment *seg,int i, int slice);

void matrix_fill_blocks(uint8_t ***B, struct Context *ctx, struct Segment *seg, int current_pass, int i, int slice);

void init_segment(struct Context *ctx,struct Segment *seg);
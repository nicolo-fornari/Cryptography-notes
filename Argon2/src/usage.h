void print_usage();

void show_params(struct Context *ctx);

int parse_options(int argc, char *argv[],struct Context *ctx);

void set_default_values(struct Context *ctx);

void compute_lengths(struct Context *ctx);

void check_params(struct Context *ctx);

void print_err(char *error_type);
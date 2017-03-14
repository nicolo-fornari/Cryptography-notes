#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <getopt.h> // required with -std=c99
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include "argon.h"

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"
#define COL1  KWHT
#define COL2  KCYN


void show_params(struct Context *ctx) {
  if (ctx->verbose) {
    printf("Running Argon2 with the following parameters:\n");
    printf("\t%sPassword:%s %s\n",COL1,COL2,ctx->pwd);
    printf("\t%sSalt:%s %s\n", COL1,COL2,ctx->salt);
    printf("\t%sSecret:%s %s\n",COL1,COL2,ctx->secret);
    printf("\t%sAdditional data:%s %s\n",COL1,COL2,ctx->data_array);
    printf("\t%sNumber of passes:%s %d\n",COL1,COL2, ctx->number_of_passes);
    printf("\t%sMemory requested in Kb:%s %d\n",COL1,COL2, ctx->memory_requested);
    printf("\t%sDegree of parallelism:%s %d\n",COL1,COL2, ctx->lanes);
    printf("\t%sArgon type:%s %d\n",COL1,COL2, ctx->type);
    printf("\t%sTau:%s %d\n",COL1,COL2,ctx->tau);
    printf("%s\n",COL1);
  }
  
}

void print_usage() {
  char usage[] = 
  "Primary inputs:\n"
  "\t-P:\tPassword to hash\n"
  "\t-S:\tSalt to use\n\n"
  "Secondary inputs (optional):\n"
  "\t-k:\tSecret\n"
  "\t-D:\tAdditional data\n"
  "\t-l:\tDigest length\n"
  "\t-n:\tNumber of passes\n"
  "\t-m:\tMemory requested (Kb)\n"
  "\t-p:\tParallelism (number of lanes)\n"
  "\t-a:\tArgon type (0 for Argon2d or 1 for Argon2i)\n"
  "\t-v:\tVerbose (1 enabled by default, 0 to display only the hash)\n";

  printf("%s\n", usage);
  exit(0);
}

int parse_options(int argc, char *argv[],struct Context *ctx) {
	int c;
	while ((c = getopt (argc, argv, "P:S:D:k:n:m:l:a:p:v:h")) != -1)
    switch (c)  {
      case 'P':
      	ctx->pwd = (uint8_t *)optarg;
      	break;
      case 'S':
      	ctx->salt = (uint8_t *)optarg;
        break;
      case 'k':
      	ctx->secret = (uint8_t *)optarg;
      	break;
      case 'D':
        ctx->data_array = (uint8_t *)optarg;
        break;
      case 'n':
        ctx->number_of_passes = atoi(optarg);
        break;    
      case 'm':
        ctx->memory_requested = atoi(optarg);
        break;
      case 'p':
        ctx->lanes = atoi(optarg);
        break;
      case 'l':
        ctx->tau = atoi(optarg);
        break;
      case 'a':
        ctx->type = atoi(optarg);
        break;
      case 'v':
      ctx->verbose = atoi(optarg);
        break;
      case 'h':
        print_usage();
        break;
      case '?':
        if (optopt == 'c')
          fprintf (stderr, "Option -%c requires an argument.\n", optopt);
        else if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);
        return 1;
      default:
        abort ();
      }
      return 0;
}

// setting some default values to avoid CLI params during development
void set_default_values(struct Context *ctx) {
  ctx->tau = 32;
  ctx->pwd = (uint8_t *)"Password you want to hash";
  ctx->salt = (uint8_t *)"Salt, please use one";
  ctx->secret = (uint8_t *)"Key, for a keyed hash function";
  ctx->data_array = (uint8_t *)"Something like IP address, user agent, time stamp etc";
  ctx->number_of_passes = 3;
  ctx->memory_requested = 64;
  ctx->lanes = 8;
  ctx->version = 0x13; 
  ctx->type = 0;
  ctx->verbose = 1;
}

// I need to know these lengths in advance for memcpy and malloc
void compute_lengths(struct Context *ctx) {
  ctx->pwdlen = strlen((char *)ctx->pwd);
  ctx->saltlen = strlen((char *)ctx->salt);
  ctx->secretlen = strlen((char *)ctx->secret);
  ctx->data_arraylen = strlen((char *)ctx->data_array);
}

void print_err(char *error_type) {
  fprintf(stderr, "%s\n", error_type);
  exit(-1);
}

void check_params(struct Context *ctx) {
  if (!(ctx->lanes>0 && ctx->lanes < pow(2,24))) {
    print_err("Degree of parallelism should be greater than 0 and less than 2^24");
  }

  if (!(ctx->memory_requested>=8*ctx->lanes && ctx->memory_requested < pow(2,32))) {
    print_err("Memory should be greater than 8p and less than 2^32");
  }

  if (!(ctx->number_of_passes>0 && ctx->number_of_passes < pow(2,32))) {
    print_err("Total number of passes should be greater than 0 and less than 2^32");
  }

  if (!(ctx->saltlen>7 && ctx->saltlen < pow(2,32))) {
    print_err("Salt should be greater than 7 and less than 2^32");
  }

  if (!(ctx->tau>3 && ctx->tau < pow(2,32))) {
    print_err("Hash length should be greater than 3 and less than 2^32");
  }

  if (ctx->type != 0 && ctx->type != 1) {
    print_err("Argon type must be 0 or 1");
  }

  if (ctx->data_arraylen > pow(2,32))
    print_err("Additional data must have length less than 2^32");

  if (ctx->secretlen > 32)
    print_err("Secret must have length less than 32");
}
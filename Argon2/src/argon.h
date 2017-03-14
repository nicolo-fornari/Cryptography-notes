#include <inttypes.h>

struct Context {
    uint8_t *out;   
    uint32_t tau; // lenght of the final argon hash

    uint8_t *pwd;   
    uint32_t pwdlen; 

    uint8_t *salt;    
    uint32_t saltlen; 	

    uint8_t *secret;
    uint32_t secretlen;

    uint8_t *data_array;
    uint32_t data_arraylen;

    uint32_t number_of_passes; // t 
    uint32_t memory_requested; // m
    uint32_t lanes; 

    uint32_t version;
    uint32_t type; // 0 -> Argon2d or 1 -> Argon2i

    int q; // columns
    int verbose;
};

struct Segment {
    uint8_t **counter;
    uint8_t **used_pairs;
    uint8_t ***couples; // array of 1024 byte: 128 J1,J2

};


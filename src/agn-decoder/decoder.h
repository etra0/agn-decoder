#ifndef AGN_DECODER_DECODER_H
#define AGN_DECODER_DECODER_H

#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <zip.h>

struct context {
    FILE *model;
    char output_name[512];
    struct texture *textures;
    size_t n_textures;
};

#endif

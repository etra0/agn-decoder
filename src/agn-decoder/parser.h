#ifndef AGN_DECODER_PARSER_H
#define AGN_DECODER_PARSER_H

#include <agn-decoder/decoder.h>

// Contains decrypt key and name of the file decrypted.
struct keys {
    uint8_t *key;
    uint32_t key_len;

    uint8_t *pid;
    uint32_t pid_len;
};

// Almost straight from the source code.
unsigned char* decode(unsigned char *buffer, size_t buffer_len, unsigned char *key, size_t key_len) {
    unsigned char s[256];
    uint32_t j = 0;
    unsigned char x;
    unsigned char *buffer_result = malloc(sizeof(char) * buffer_len);

    for (unsigned int i = 0; i < 256; i++) {
        s[i] = i;
    }

    for(int i = 0; i < 256; i++) {
        j = (j + s[i] + key[i % key_len]) % 256;
        x = s[i];
        s[i] = s[j];
        s[j] = x;
    }

    uint32_t i = 0;
    j = 0;
    for (uint32_t y = 0; y < buffer_len; y++) {
        i = (i + 1) % 256;
        j = (j + s[i]) % 256;
        x = s[i];
        s[i] = s[j];
        s[j] = x;
        unsigned char result = buffer[y] ^ s[(s[i] + s[j]) % 256];
        buffer_result[y] = result;
    }

    return buffer_result;
}

void read_center_of_mass(FILE *f) {
    float center[3];
    fread(center, sizeof(center), 1, f);
    printf("center_of_mass: %f %f %f\n", center[0], center[1], center[2]);
}

uint8_t read_metatag(FILE *f) {
    uint8_t metatag;
    fread(&metatag, sizeof(metatag), 1, f);
    printf("metatag: %u\n", metatag);
    return metatag;
}

uint64_t get_file_size(FILE *f) {
    fseek(f, 0, SEEK_END);
    return ftell(f);
}

void parse_header(FILE *f, struct keys *k, uint8_t *hook, size_t hook_size) {
    char tag[4];
    fread(tag, sizeof(char), sizeof(tag), f);
    assert(strncmp(tag, "m3dm", 4) == 0);

    read_metatag(f);

    // read center of mass? 
    read_center_of_mass(f);

    read_metatag(f);
    // revision, we don't care
    fseek(f, sizeof(uint32_t), SEEK_CUR);

    read_metatag(f);
    // build date
    fseek(f, sizeof(uint64_t), SEEK_CUR);

    read_metatag(f);
    // model id
    fseek(f, sizeof(uint32_t), SEEK_CUR);

    assert(read_metatag(f) == 5);

    unsigned char *basic_key, *product_id;

    fread(&k->key_len, sizeof(uint32_t), 1, f);
    fread(&k->pid_len, sizeof(uint32_t), 1, f);

    basic_key = malloc(sizeof(char) * k->key_len);
    product_id = malloc(sizeof(char) * k->pid_len);

    fread(basic_key, sizeof(char), k->key_len, f);
    fread(product_id, sizeof(char), k->pid_len, f);

    printf("basic_key_len: %u   product_id_len: %u\n", k->key_len, k->pid_len);

    uint8_t *basic_key_decoded = decode(basic_key, k->key_len, hook, hook_size);
    uint8_t *product_id_decoded = decode(product_id, k->pid_len, hook, hook_size);

    free(basic_key); free(product_id);

    k->key = basic_key_decoded;
    k->pid = product_id_decoded;

    printf("pid_decoded: %.*s\n", 6, product_id_decoded);
}

void parse_all_mesh(struct context *ctx, struct mesh **m, uint32_t *n_meshes) {
    FILE *f = ctx->model;
    struct keys k;

    uint8_t hook[HOOK_SIZE];
    size_t hook_size = sizeof(hook);
    calculate_hook(ctx->textures, ctx->n_textures, hook);

    parse_header(f, &k, hook, hook_size);

    // Some kind of offset?
    uint32_t cmho = 128;

    uint64_t file_size = get_file_size(f);
    uint64_t rest_file = file_size - cmho;
    fseek(f, cmho, SEEK_SET);
    unsigned char *data = malloc(sizeof(char) * rest_file);
    fread(data, sizeof(char), rest_file, f);
    unsigned char *data_decoded = decode(data, rest_file, k.key, k.key_len);

    // Rewrite temp file
    FILE *temp = tmpfile();
    unsigned char *merger = malloc(sizeof(char) * cmho);
    fseek(f, 0, SEEK_SET);
    fread(merger, sizeof(char), cmho, f);
    fwrite(merger, sizeof(char), cmho, temp);
    fwrite(data_decoded, sizeof(char), rest_file, temp);

    free(data_decoded); free(merger); free(data);
    free(k.key); free(k.pid);
    fclose(f);
    ctx->model = temp;

    fseek(temp, 0, SEEK_SET);
    parse_header(temp, &k, hook, hook_size);

    read_metatag(temp);
    read_metatag(temp);

    uint32_t _n_meshes;
    fread(&_n_meshes, sizeof(uint32_t), 1, temp);
    *n_meshes = _n_meshes;
    printf("n_meshes: %d\n", _n_meshes);

    struct mesh *mesh = malloc(sizeof(struct mesh) * _n_meshes);
    *m = mesh;
    for (uint32_t i = 0; i < _n_meshes; i++) {
        parse_mesh(&mesh[i], temp);
    }

    sprintf(ctx->output_name, "%s.obj", k.pid);
    free(k.key); free(k.pid);
}

#endif

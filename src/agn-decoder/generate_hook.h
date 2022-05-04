#ifndef AGN_DECODER_GENERATE_HOOK_H
#define AGN_DECODER_GENERATE_HOOK_H

#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>

struct texture {
    FILE *f;
    size_t size;
};

static const size_t HOOK_SIZE = 128 / 8;

size_t count_char(uint8_t *arr, size_t sz, uint8_t c) {
    size_t count = 0;
    for (size_t i = 0; i < sz; i++) {
        if (arr[i] == c)
            arr[i] = 0; count++;
    }

    return count;
}

bool texture_can_ignore_path(const char *p) {
    const char *ignorable_paths[] = {
        "shadow.png", "shadow_animated.png", "model"
    };

    for (int i = 0; i < sizeof(ignorable_paths) / sizeof(char *); i++) {
        if (strcmp(p, ignorable_paths[i]) == 0)
            return true;
    }

    return false;
}

bool validate_hook(uint8_t *hook) {
    uint8_t arr[HOOK_SIZE];
    memcpy(arr, hook, sizeof(arr));
    for (int i = 0; i < HOOK_SIZE; i++) {
        if (arr[i] == 0) continue;
        size_t repeats = count_char(arr, HOOK_SIZE, arr[i]);
        float percent = repeats / HOOK_SIZE;
        if (percent > 0.3) return false;
    }

    return true;
}

void calculate_hook(const struct texture *textures, size_t n_textures, uint8_t *hook) {
    const struct texture *largest_texture = NULL;
    size_t data_size = 0;

    for (int i = 0; i < n_textures; i++) {
        if (textures[i].size > data_size) {
            data_size = textures[i].size;
            largest_texture = &textures[i];
        }
    }
    printf("Size of largest texture = %llu\n", largest_texture->size);

    assert(largest_texture != NULL);
    fseek(largest_texture->f, largest_texture->size / 2, SEEK_SET);

    while (!validate_hook(hook)) {
        size_t read = fread(hook, sizeof(uint8_t), HOOK_SIZE, largest_texture->f);
        if (read < HOOK_SIZE) {
            fseek(largest_texture->f, largest_texture->size / 2, SEEK_SET);
            fread(hook, sizeof(uint8_t), HOOK_SIZE, largest_texture->f);
            break;
        }
    }
}

void texture_free(struct texture *t) {
    fclose(t->f);
}

#endif

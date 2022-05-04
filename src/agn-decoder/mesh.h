#ifndef AGN_DECODER_MESH_H
#define AGN_DECODER_MESH_H

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define MESH_ATTRS_VERTEX_DATA_POSITION 0x01
#define MESH_ATTRS_VERTEX_DATA_NORMAL 0x02
#define MESH_ATTRS_VERTEX_DATA_TEXT_COORD 0x03
#define MESH_ATTRS_VERTEX_DATA_TANGENT4 0x04

struct vec {
    union {
        float elements[4];
        struct {
            float x, y, z, w;
        };
    };
};

struct mesh {
    char *name;
    int32_t id;
    uint32_t n_vertex;
    uint32_t n_indices;
    uint8_t attrs;

    int32_t material_ix;

    // size(3)
    struct vec *position;
    // size (3)
    struct vec *normal;
    // size(2)
    struct vec *uv;
    // size(4)
    struct vec *tangent;

    uint16_t *indices;
};

void generate_obj_file(const char *name, struct mesh *m, size_t n_meshes) {
    FILE *f = fopen(name, "w");
    uint64_t current_ix = 1;
    for (size_t i = 0; i < n_meshes; i++) {

        fprintf(f, "o %s\n", m[i].name);

        for (size_t nv = 0; nv < m[i].n_vertex; nv++) {
            // write the vertex nodes.
            fprintf(f, "v %f %f %f\n",
                    m[i].position[nv].elements[0],
                    m[i].position[nv].elements[1],
                    m[i].position[nv].elements[2]);
        }

        for (size_t nv = 0; nv < m[i].n_vertex; nv++) {
            // vertex normal
            fprintf(f, "vn %f %f %f\n",
                    m[i].normal[nv].elements[0],
                    m[i].normal[nv].elements[1],
                    m[i].normal[nv].elements[2]);
        }

        for (size_t nv = 0; nv < m[i].n_vertex; nv++) {
            // write the uvs
            fprintf(f, "vt %f %f\n",
                    m[i].uv[nv].elements[0],
                    m[i].uv[nv].elements[1]);
        }

        for (size_t n_ix = 0; n_ix < m[i].n_indices; n_ix+= 3) {
            // face indices
            fprintf(f, "f %lu %lu %lu\n", 
                    m[i].indices[n_ix]     + current_ix,
                    m[i].indices[n_ix + 1] + current_ix,
                    m[i].indices[n_ix + 2] + current_ix);
        }

        // The vertex indices are contiguous so we need to offset the faces.
        current_ix += m[i].n_vertex;
    }
    fclose(f);
}


void print_mesh(const struct mesh *m) {
    printf("id: %d\tname: %s\n", m->id, m->name);
}

void print_vec(const struct vec *v, uint32_t n_elements) {
    for (int i = 0; i < n_elements; i++) {
        printf("%f ", v->elements[i]);
    }
    printf("\n");
}

uint32_t read_weird_encoding(FILE *f) {
    int8_t b;
    uint32_t res = 0;
    uint32_t shift = 0;

    do {
        fread(&b, sizeof(int8_t), 1, f);
        res |= ((b & 0x7F) << shift);
        shift += 7;
    } while ((b & 0x80) == 0x80);

    return res;
}

// This function does allocate so it's up to you to free.
char* read_string(FILE *f) {
    uint32_t length = read_weird_encoding(f);
    char *res = malloc(sizeof(char)*length + 1);

    fread(res, sizeof(char), length, f);
    res[length] = 0;

    return res;
}

void parse_mesh(struct mesh *target, FILE *temp) {

        fread(&target->id, sizeof(uint32_t), 1, temp);
        target->name = read_string(temp);
        print_mesh(target);

        fread(&target->attrs, sizeof(target->attrs), 1, temp);
        fread(&target->n_vertex, sizeof(target->n_vertex), 1, temp);

        if (target->attrs & MESH_ATTRS_VERTEX_DATA_POSITION) {
            target->position = malloc(sizeof(struct vec) * target->n_vertex);
        }
        if (target->attrs & MESH_ATTRS_VERTEX_DATA_NORMAL) {
            target->normal = malloc(sizeof(struct vec) * target->n_vertex);
        }
        if (target->attrs & MESH_ATTRS_VERTEX_DATA_TEXT_COORD) {
            target->uv = malloc(sizeof(struct vec) * target->n_vertex);
        }
        if (target->attrs & MESH_ATTRS_VERTEX_DATA_TANGENT4) {
            target->tangent = malloc(sizeof(struct vec) * target->n_vertex);
        }

        for (uint32_t i = 0; i < target->n_vertex; i++) {
            if (target->attrs & MESH_ATTRS_VERTEX_DATA_POSITION) {
                fread(&target->position[i], sizeof(float), 3, temp);
            }

            if (target->attrs & MESH_ATTRS_VERTEX_DATA_NORMAL) {
                fread(&target->normal[i], sizeof(float), 3, temp);
            }

            if (target->attrs & MESH_ATTRS_VERTEX_DATA_TANGENT4) {
                fread(&target->tangent[i], sizeof(float), 4, temp);
            }

            if (target->attrs & MESH_ATTRS_VERTEX_DATA_TEXT_COORD) {
                fread(&target->uv[i], sizeof(float), 2, temp);
            }
        }

        fread(&target->n_indices, sizeof(target->n_indices), 1, temp);
        target->indices = malloc(sizeof(uint16_t) * target->n_indices);
        for (uint32_t i = 0; i < target->n_indices; i += 3) {
            fread(&target->indices[i], sizeof(uint16_t), 3, temp);
        }

        fread(&target->material_ix, sizeof(int32_t), 1, temp);
}

void delete_mesh(struct mesh *m) {
        if (m->attrs & MESH_ATTRS_VERTEX_DATA_POSITION) {
            free(m->position);
        }
        if (m->attrs & MESH_ATTRS_VERTEX_DATA_NORMAL) {
            free(m->normal);
        }
        if (m->attrs & MESH_ATTRS_VERTEX_DATA_TEXT_COORD) {
            free(m->uv);
        }
        if (m->attrs & MESH_ATTRS_VERTEX_DATA_TANGENT4) {
            free(m->tangent);
        }

        free(m->name);
        free(m->indices);
}

#endif

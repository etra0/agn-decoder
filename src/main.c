#include <agn-decoder/decoder.h>
#include <agn-decoder/mesh.h>
#include <agn-decoder/generate_hook.h>
#include <agn-decoder/parser.h>

void load_files(const char *file, struct context *ctx) {
    // Check the file is a valid one.
    FILE *f = fopen(file, "r");
    char tag[2];
    fread(tag, sizeof(char), 2, f);
    if (strncmp(tag, "PK", 2) != 0) {
        printf("The file is not valid!\n");
        fclose(f);
        exit(EXIT_FAILURE);
    }
    fseek(f, 0, SEEK_SET);
    fclose(f);

    struct zip_t *zip = zip_open(file, 0, 'r');
    struct texture texture_buffer[100];
    int n_textures = 0;

    uint8_t *buffer = NULL;
    size_t bufsize = 0;
    int n = zip_entries_total(zip);
    for (int i = 0; i < n; i++) {
        zip_entry_openbyindex(zip, i);
        {
            FILE *temp_file = tmpfile();
            const char *name = zip_entry_name(zip);
            zip_entry_read(zip, (void *)&buffer, &bufsize);
            fwrite(buffer, sizeof(uint8_t), bufsize, temp_file);
            fseek(temp_file, 0, SEEK_SET);
            free(buffer);

            if (!texture_can_ignore_path(name)) {
                printf("Loaded texture %s\n", name);
                texture_buffer[n_textures].size = bufsize;
                texture_buffer[n_textures++].f = temp_file;
            } else if (strcmp("model", name) == 0) {
                printf("Loaded model\n");
                ctx->model = temp_file;
            } else {
                // In this case we don't care about the shadow textures and
                // stuff. Since it's a temp file, it'll get automatically
                // deleted anyway.
                fclose(temp_file);
            }
        }
        zip_entry_close(zip);
    }

    ctx->n_textures = n_textures;
    ctx->textures = malloc(sizeof(struct texture) * n_textures);
    memcpy(ctx->textures, texture_buffer, sizeof(struct texture ) * n_textures);
    zip_close(zip);
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("You need to pass the file.\n");
        return EXIT_FAILURE;
    }

    struct context ctx;
    printf("reading %s\n", argv[1]);

    load_files(argv[1], &ctx);

    struct mesh *meshes = NULL;
    uint32_t n_meshes;
    parse_all_mesh(&ctx, &meshes, &n_meshes);
    generate_obj_file(ctx.output_name, meshes, n_meshes);
    printf("generated %s successfully!\n", ctx.output_name);

    for (uint32_t i = 0; i < n_meshes; i++) {
        delete_mesh(&meshes[i]);
    }
    free(meshes);

    for (int i = 0; i < ctx.n_textures; i++) {
        fclose(ctx.textures[i].f);
    }
    free(ctx.textures);
    return EXIT_SUCCESS;
}

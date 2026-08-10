#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libgen.h>
#include <string.h>


#define MAX_LINE 4096
#define INITIAL_CAPACITY 1000000
#define MAX_FACE_VERTS 256

typedef struct { float x, y, z; } Vec3;
typedef struct { float u, v; } Vec2;

Vec3* positions = NULL;
Vec2* texcoords = NULL;
Vec3* normals = NULL;
float* vertices = NULL;
unsigned int* indices = NULL;

int pos_count = 0, tex_count = 0, norm_count = 0;
int vert_count = 0, idx_count = 0;
int pos_cap = 0, tex_cap = 0, norm_cap = 0, vert_cap = 0, idx_cap = 0;

int total_faces = 0;
int total_triangles = 0;
int skipped_faces = 0;

void ensure_pos_cap() {
    if (pos_count >= pos_cap) {
        pos_cap = pos_cap ? pos_cap * 2 : INITIAL_CAPACITY;
        positions = realloc(positions, pos_cap * sizeof(Vec3));
    }
}
void ensure_tex_cap() {
    if (tex_count >= tex_cap) {
        tex_cap = tex_cap ? tex_cap * 2 : INITIAL_CAPACITY;
        texcoords = realloc(texcoords, tex_cap * sizeof(Vec2));
    }
}
void ensure_norm_cap() {
    if (norm_count >= norm_cap) {
        norm_cap = norm_cap ? norm_cap * 2 : INITIAL_CAPACITY;
        normals = realloc(normals, norm_cap * sizeof(Vec3));
    }
}
void ensure_vert_cap() {
    if (vert_count >= vert_cap) {
        vert_cap = vert_cap ? vert_cap * 2 : INITIAL_CAPACITY;
        vertices = realloc(vertices, vert_cap * 8 * sizeof(float));
    }
}
void ensure_idx_cap() {
    if (idx_count >= idx_cap) {
        idx_cap = idx_cap ? idx_cap * 2 : INITIAL_CAPACITY;
        indices = realloc(indices, idx_cap * sizeof(unsigned int));
    }
}

void add_vertex(float x, float y, float z, float u, float v, float nx, float ny, float nz) {
    ensure_vert_cap();
    int base = vert_count * 8;
    vertices[base] = x;
    vertices[base+1] = y;
    vertices[base+2] = z;
    vertices[base+3] = u;
    vertices[base+4] = v;
    vertices[base+5] = nx;
    vertices[base+6] = ny;
    vertices[base+7] = nz;
    vert_count++;
}

void add_index(unsigned int idx) {
    ensure_idx_cap();
    indices[idx_count++] = idx;
}

void parse_v(char* line) {
    Vec3 v;
    sscanf(line, "v %f %f %f", &v.x, &v.y, &v.z);
    ensure_pos_cap();
    positions[pos_count++] = v;
}

void parse_vt(char* line) {
    Vec2 vt;
    sscanf(line, "vt %f %f", &vt.u, &vt.v);
    ensure_tex_cap();
    texcoords[tex_count++] = vt;
}

void parse_vn(char* line) {
    Vec3 vn;
    sscanf(line, "vn %f %f %f", &vn.x, &vn.y, &vn.z);
    ensure_norm_cap();
    normals[norm_count++] = vn;
}

void parse_f(char* line) {
    total_faces++;
    int vi[MAX_FACE_VERTS], ti[MAX_FACE_VERTS], ni[MAX_FACE_VERTS];
    int count = 0;
    char* token = strtok(line, " \t");
    if (!token) return;
    token = strtok(NULL, " \t");
    while (token && count < MAX_FACE_VERTS) {
        int v, t, n;
        char* slash1 = strchr(token, '/');
        if (!slash1) {
            sscanf(token, "%d", &v);
            t = 0; n = 0;
        } else {
            *slash1 = '\0';
            v = atoi(token);
            char* rest = slash1 + 1;
            char* slash2 = strchr(rest, '/');
            if (!slash2) {
                t = atoi(rest);
                n = 0;
            } else {
                *slash2 = '\0';
                t = atoi(rest);
                n = atoi(slash2 + 1);
            }
        }
        if (v < 0) v = pos_count + v + 1;
        if (t < 0) t = tex_count + t + 1;
        if (n < 0) n = norm_count + n + 1;

        vi[count] = v;
        ti[count] = t;
        ni[count] = n;
        count++;
        token = strtok(NULL, " \t");
    }

    if (count < 3) {
        skipped_faces++;
        return;
    }

    for (int i = 1; i < count - 1; i++) {
        int tri[3] = {0, i, i+1};
        for (int j = 0; j < 3; j++) {
            int idx = tri[j];
            int pos_idx = vi[idx] - 1;
            int tex_idx = ti[idx] - 1;
            int norm_idx = ni[idx] - 1;

            float x = (pos_idx >= 0 && pos_idx < pos_count) ? positions[pos_idx].x : 0;
            float y = (pos_idx >= 0 && pos_idx < pos_count) ? positions[pos_idx].y : 0;
            float z = (pos_idx >= 0 && pos_idx < pos_count) ? positions[pos_idx].z : 0;
            float u = (tex_idx >= 0 && tex_idx < tex_count) ? texcoords[tex_idx].u : 0;
            float v = (tex_idx >= 0 && tex_idx < tex_count) ? texcoords[tex_idx].v : 0;
            float nx = (norm_idx >= 0 && norm_idx < norm_count) ? normals[norm_idx].x : 0;
            float ny = (norm_idx >= 0 && norm_idx < norm_count) ? normals[norm_idx].y : 0;
            float nz = (norm_idx >= 0 && norm_idx < norm_count) ? normals[norm_idx].z : 0;

            add_vertex(x, y, z, u, v, nx, ny, nz);
            add_index(vert_count - 1);
        }
        total_triangles++;
    }
}

int main(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s input.obj output.h\n", argv[0]);
        return 1;
    }

    FILE* in = fopen(argv[1], "r");
    if (!in) {
        perror("failed to open input file");
        return 1;
    }

    char* input_path = argv[1];
    char* base = basename(input_path);
    char* dot = strrchr(base, '.');
    int name_len = dot ? (dot - base) : strlen(base);
    char prefix[256];
    strncpy(prefix, base, name_len);
    prefix[name_len] = '\0';

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), in)) {
        size_t len = strlen(line);
        if (len > 0 && line[len-1] == '\n') line[--len] = '\0';
        if (len > 0 && line[len-1] == '\r') line[--len] = '\0';

        if (line[0] == 'v') {
            if (line[1] == ' ') {
                parse_v(line);
            } else if (line[1] == 't' && line[2] == ' ') {
                parse_vt(line);
            } else if (line[1] == 'n' && line[2] == ' ') {
                parse_vn(line);
            }
        } else if (line[0] == 'f' && line[1] == ' ') {
            parse_f(line);
        }
    }
    fclose(in);

    fprintf(stderr, "total faces: %d\n", total_faces);
    fprintf(stderr, "triangles generated: %d\n", total_triangles);
    fprintf(stderr, "skipped faces: %d\n", skipped_faces);
    fprintf(stderr, "vertices: %d, Indices: %d\n", vert_count, idx_count);

    FILE* out = fopen(argv[2], "w");
    if (!out) {
        perror("failed to open output file");
        return 1;
    }
    fprintf(out, "#pragma once\n");
    fprintf(out, "static const float model_%s_vertices[] = {\n", prefix);
    for (int i = 0; i < vert_count; i++) {
        int base = i * 8;
        fprintf(out, "    %.6ff, %.6ff, %.6ff, %.6ff, %.6ff, %.6ff, %.6ff, %.6ff,\n",
                vertices[base], vertices[base+1], vertices[base+2],
                vertices[base+3], vertices[base+4],
                vertices[base+5], vertices[base+6], vertices[base+7]);
    }
    fprintf(out, "};\n\n");

    fprintf(out, "static const unsigned int model_%s_indices[] = {\n", prefix);
    for (int i = 0; i < idx_count; i += 3) {
        fprintf(out, "    %u, %u, %u,\n", indices[i], indices[i+1], indices[i+2]);
    }
    fprintf(out, "};\n\n");

    fprintf(out, "#define MODEL_%s_VERTEX_COUNT %d\n",prefix, vert_count);
    fprintf(out, "#define MODEL_%s_INDEX_COUNT %d\n", prefix, idx_count);

    fclose(out);

    free(positions); free(texcoords); free(normals);
    free(vertices); free(indices);

    printf("convertion complete: %s\n", argv[2]);
    return 0;
}

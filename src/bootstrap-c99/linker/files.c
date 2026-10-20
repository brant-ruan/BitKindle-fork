#ifndef C1_LINK_FILES_C
#define C1_LINK_FILES_C
#include <stdio.h>
#include <stdlib.h>
#include "writer.c"

struct lk_owned_object {
    unsigned char *bytes;
    unsigned long long length;
};

/* Return 1 for arguments/invalid format, 3 for I/O, 4 for allocation.
   Only successful reads publish ownership; release once with free(bytes).
   The output record must not already own an unreleased allocation. */
int lk_read_object_file(const char *path, struct lk_owned_object *out) {
    FILE *file;
    long length;
    unsigned char *bytes;
    struct lk_object_view v;
    int bad;
    if (!path || !out)
        return 1;
    file = fopen(path, "rb");
    if (!file)
        return 3;
    if (fseek(file, 0, SEEK_END)) {
        fclose(file);
        return 3;
    }
    length = ftell(file);
    if (length < 0) {
        fclose(file);
        return 3;
    }
    if (length < 32 || length > 18563104) {
        fclose(file);
        return 1;
    }
    if (fseek(file, 0, SEEK_SET)) {
        fclose(file);
        return 3;
    }
    bytes = malloc((size_t)length);
    if (!bytes) {
        fclose(file);
        return 4;
    }
    bad = fread(bytes, 1, (size_t)length, file) != (size_t)length;
    if (!bad) {
        if (fgetc(file) != EOF || ferror(file))
            bad = 1;
    }
    if (fclose(file))
        bad = 1;
    if (bad) {
        free(bytes);
        return 3;
    }
    if (lk_object_read(bytes, (unsigned long long)length, &v)) {
        free(bytes);
        return 1;
    }
    out->bytes = bytes;
    out->length = (unsigned long long)length;
    return 0;
}

/* Validate and encode before opening the destination. I/O failures may
   leave a partial file; metadata errors never open/truncate the destination. */
int lk_write_object_file(const char *path, struct lk_section *sections, unsigned char **payloads,
                         int ns, struct lk_symbol *symbols, int ny, struct lk_fixup *fixups,
                         int nf) {
    unsigned char *bytes;
    unsigned long long length;
    FILE *file;
    int status, bad;
    if (!path)
        return 1;
    status = lk_object_write(sections, payloads, ns, symbols, ny, fixups, nf, 0, 0, &length);
    if (status)
        return status;
    bytes = malloc((size_t)length);
    if (!bytes)
        return 4;
    status =
        lk_object_write(sections, payloads, ns, symbols, ny, fixups, nf, bytes, length, &length);
    if (status) {
        free(bytes);
        return status;
    }
    file = fopen(path, "wb");
    if (!file) {
        free(bytes);
        return 3;
    }
    bad = fwrite(bytes, 1, (size_t)length, file) != (size_t)length;
    if (fclose(file))
        bad = 1;
    free(bytes);
    return bad ? 3 : 0;
}
#endif

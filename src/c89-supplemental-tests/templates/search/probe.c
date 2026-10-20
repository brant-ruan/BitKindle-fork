#include <stdlib.h>

struct Record {
    unsigned char key, id, check;
};
static struct Record records[17];
static int count, bad, calls, searching;
static unsigned char sought;

static int element(const void *p) {
    int i;
    for (i = 0; i < count; i++)
        if (p == (const void *)&records[i])
            return 1;
    return 0;
}

static int compare(const void *a, const void *b) {
    const struct Record *left = a, *right = b;
    calls++;
    if (!element(b))
        bad = 1;
    if (searching) {
        if (a != (const void *)&sought)
            bad = 1;
        return (int)sought - (int)right->key;
    }
    if (!element(a))
        bad = 1;
    return (int)left->key - (int)right->key;
}

int main(void) {
    int trial, i, key, present, seen[17];
    unsigned int state = 12345U;
    struct Record *found;
    for (count = 0; count <= 17; count++)
        for (trial = 0; trial < 32; trial++) {
            for (i = 0; i < count; i++) {
                state = state * 1664525U + 1013904223U;
                records[i].key = (unsigned char)((state >> 16) & 7U);
                records[i].id = (unsigned char)i;
                records[i].check = (unsigned char)(records[i].key ^ records[i].id ^ 85);
                seen[i] = 0;
            }
            searching = 0;
            bad = 0;
            calls = 0;
            qsort(records, (size_t)count, sizeof(struct Record), compare);
            if (bad || (!count && calls))
                return 1;
            for (i = 0; i < count; i++) {
                if (i && records[i - 1].key > records[i].key)
                    return 2;
                if (records[i].id >= count || seen[records[i].id]++)
                    return 3;
                if (records[i].check != (unsigned char)(records[i].key ^ records[i].id ^ 85))
                    return 4;
            }
            searching = 1;
            for (key = 0; key <= 8; key++) {
                present = 0;
                calls = 0;
                sought = (unsigned char)key;
                for (i = 0; i < count; i++)
                    if (records[i].key == key)
                        present = 1;
                found = bsearch(&sought, records, (size_t)count, sizeof(struct Record), compare);
                if (bad || (!count && calls))
                    return 5;
                if ((found != NULL) != present)
                    return 6;
                if (found && (!element(found) || found->key != key))
                    return 7;
            }
        }
    return 0;
}

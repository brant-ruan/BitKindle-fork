#include <stdlib.h>

static void __c1_sort_swap(unsigned char *a, unsigned char *b, size_t size) {
    unsigned char byte;
    size_t i;
    for (i = 0; i < size; i++) {
        byte = a[i];
        a[i] = b[i];
        b[i] = byte;
    }
}

static void __c1_sort_sift(unsigned char *base, size_t root, size_t count, size_t size,
                           int (*compare)(const void *, const void *)) {
    size_t child;
    /* Testing for a parent before doubling avoids overflowing the index. */
    while (root < count / 2) {
        child = root * 2 + 1;
        if (child + 1 < count && compare(base + child * size, base + (child + 1) * size) < 0)
            child++;
        if (compare(base + root * size, base + child * size) >= 0)
            return;
        __c1_sort_swap(base + root * size, base + child * size, size);
        root = child;
    }
}

void qsort(void *base, size_t count, size_t size, int (*compare)(const void *, const void *)) {
    unsigned char *bytes = base;
    size_t root, end;
    if (count < 2 || !size)
        return;
    root = count / 2;
    while (root) {
        root--;
        __c1_sort_sift(bytes, root, count, size, compare);
    }
    end = count;
    while (end > 1) {
        end--;
        __c1_sort_swap(bytes, bytes + end * size, size);
        __c1_sort_sift(bytes, 0, end, size, compare);
    }
}

void *bsearch(const void *key, const void *base, size_t count, size_t size,
              int (*compare)(const void *, const void *)) {
    const unsigned char *bytes = base, *element;
    size_t low = 0, high = count, middle;
    int order;
    while (low < high) {
        middle = low + (high - low) / 2;
        element = bytes + middle * size;
        order = compare(key, element);
        if (!order)
            return (void *)element;
        if (order < 0)
            high = middle;
        else
            low = middle + 1;
    }
    return NULL;
}

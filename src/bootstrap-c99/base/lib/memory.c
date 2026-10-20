#include <stdlib.h>
/* Windows x64 system heap, using the IAT emitted by pe_header. */
typedef void *__c1_heap_handle(void);
typedef void *__c1_heap_alloc(void *, unsigned int, size_t);
typedef void *__c1_heap_realloc(void *, unsigned int, void *, size_t);
typedef int __c1_heap_free(void *, unsigned int, void *);

void *malloc(size_t size) {
    __c1_heap_handle *heap = *(__c1_heap_handle **)0x401080;
    __c1_heap_alloc *allocate = *(__c1_heap_alloc **)0x401088;
    if (!size)
        size = 1;
    return allocate(heap(), 0, size);
}

void *calloc(size_t count, size_t size) {
    __c1_heap_handle *heap = *(__c1_heap_handle **)0x401080;
    __c1_heap_alloc *allocate = *(__c1_heap_alloc **)0x401088;
    if (size && count > (size_t)-1 / size)
        return NULL;
    size = count * size;
    if (!size)
        size = 1;
    return allocate(heap(), 8, size);
}

void free(void *block) {
    __c1_heap_handle *heap;
    __c1_heap_free *release;
    if (!block)
        return;
    heap = *(__c1_heap_handle **)0x401080;
    release = *(__c1_heap_free **)0x401098;
    release(heap(), 0, block);
}

void *realloc(void *block, size_t size) {
    __c1_heap_handle *heap;
    __c1_heap_realloc *resize;
    if (!block)
        return malloc(size);
    if (!size) {
        free(block);
        return NULL;
    }
    heap = *(__c1_heap_handle **)0x401080;
    resize = *(__c1_heap_realloc **)0x401090;
    return resize(heap(), 0, block, size);
}

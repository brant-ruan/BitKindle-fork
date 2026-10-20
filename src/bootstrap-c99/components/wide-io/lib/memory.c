#include "winimports.h"
#include <stdlib.h>
/* Windows x64 system heap, using the IAT emitted by pe_header. */
typedef void *__c1_heap_handle(void);
typedef void *__c1_heap_alloc(void *, unsigned int, size_t);
typedef void *__c1_heap_realloc(void *, unsigned int, void *, size_t);
typedef int __c1_heap_free(void *, unsigned int, void *);

void *malloc(size_t size) {
    __c1_heap_handle *heap = (__c1_heap_handle *)__c1_win_GetProcessHeap;
    __c1_heap_alloc *allocate = (__c1_heap_alloc *)__c1_win_HeapAlloc;
    if (!size)
        size = 1;
    return allocate(heap(), 0, size);
}

void *calloc(size_t count, size_t size) {
    __c1_heap_handle *heap = (__c1_heap_handle *)__c1_win_GetProcessHeap;
    __c1_heap_alloc *allocate = (__c1_heap_alloc *)__c1_win_HeapAlloc;
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
    heap = (__c1_heap_handle *)__c1_win_GetProcessHeap;
    release = (__c1_heap_free *)__c1_win_HeapFree;
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
    heap = (__c1_heap_handle *)__c1_win_GetProcessHeap;
    resize = (__c1_heap_realloc *)__c1_win_HeapReAlloc;
    return resize(heap(), 0, block, size);
}

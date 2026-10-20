#include <string.h>

void *memcpy(void *dest, const void *src, size_t count) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    size_t i;
    for (i = 0; i < count; i++)
        d[i] = s[i];
    return dest;
}

void *memmove(void *dest, const void *src, size_t count) {
    unsigned char *d = dest;
    const unsigned char *s = src;
    size_t i;
    /* Windows x64 pointers have the same width as size_t. Integer ordering
       also permits comparing disjoint objects without pointer subtraction. */
    if ((size_t)d > (size_t)s && (size_t)d - (size_t)s < count) {
        i = count;
        while (i) {
            i--;
            d[i] = s[i];
        }
    } else {
        for (i = 0; i < count; i++)
            d[i] = s[i];
    }
    return dest;
}

void *memset(void *dest, int value, size_t count) {
    unsigned char *d = dest;
    size_t i;
    for (i = 0; i < count; i++)
        d[i] = (unsigned char)value;
    return dest;
}

int memcmp(const void *left, const void *right, size_t count) {
    const unsigned char *a = left, *b = right;
    size_t i;
    for (i = 0; i < count; i++) {
        if (a[i] != b[i])
            return (int)a[i] - (int)b[i];
    }
    return 0;
}

void *memchr(const void *data, int value, size_t count) {
    const unsigned char *s = data;
    size_t i;
    for (i = 0; i < count; i++) {
        if (s[i] == (unsigned char)value)
            return (void *)(s + i);
    }
    return NULL;
}

size_t strlen(const char *text) {
    size_t n = 0;
    while (text[n])
        n++;
    return n;
}

char *strcpy(char *dest, const char *src) {
    size_t i = 0;
    do {
        dest[i] = src[i];
    } while (src[i++]);
    return dest;
}

char *strncpy(char *dest, const char *src, size_t count) {
    size_t i = 0;
    while (i < count && src[i]) {
        dest[i] = src[i];
        i++;
    }
    while (i < count)
        dest[i++] = 0;
    return dest;
}

char *strcat(char *dest, const char *src) {
    strcpy(dest + strlen(dest), src);
    return dest;
}

char *strncat(char *dest, const char *src, size_t count) {
    size_t n = strlen(dest), i = 0;
    while (i < count && src[i]) {
        dest[n + i] = src[i];
        i++;
    }
    dest[n + i] = 0;
    return dest;
}

int strcmp(const char *left, const char *right) {
    const unsigned char *a = (const unsigned char *)left;
    const unsigned char *b = (const unsigned char *)right;
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return (int)*a - (int)*b;
}

int strncmp(const char *left, const char *right, size_t count) {
    const unsigned char *a = (const unsigned char *)left;
    const unsigned char *b = (const unsigned char *)right;
    size_t i;
    for (i = 0; i < count; i++) {
        if (a[i] != b[i])
            return (int)a[i] - (int)b[i];
        if (!a[i])
            break;
    }
    return 0;
}

/* The library currently implements the C locale only. */
int strcoll(const char *left, const char *right) {
    return strcmp(left, right);
}

size_t strxfrm(char *dest, const char *src, size_t count) {
    size_t n = strlen(src), i;
    for (i = 0; i < count && i <= n; i++)
        dest[i] = src[i];
    return n;
}

char *strchr(const char *text, int value) {
    char c = (char)value;
    while (*text != c) {
        if (!*text)
            return NULL;
        text++;
    }
    return (char *)text;
}

char *strrchr(const char *text, int value) {
    char c = (char)value, *found = NULL;
    do {
        if (*text == c)
            found = (char *)text;
    } while (*text++);
    return found;
}

size_t strspn(const char *text, const char *accept) {
    size_t n = 0;
    while (text[n] && strchr(accept, text[n]))
        n++;
    return n;
}

size_t strcspn(const char *text, const char *reject) {
    size_t n = 0;
    while (text[n] && !strchr(reject, text[n]))
        n++;
    return n;
}

char *strpbrk(const char *text, const char *accept) {
    while (*text) {
        if (strchr(accept, *text))
            return (char *)text;
        text++;
    }
    return NULL;
}

char *strstr(const char *text, const char *needle) {
    size_t i;
    if (!*needle)
        return (char *)text;
    while (*text) {
        i = 0;
        while (needle[i] && text[i] && needle[i] == text[i])
            i++;
        if (!needle[i])
            return (char *)text;
        text++;
    }
    return NULL;
}

static char *__c1_token_next;

char *strtok(char *text, const char *delimiters) {
    char *start;
    if (text)
        __c1_token_next = text;
    if (!__c1_token_next)
        return NULL;
    start = __c1_token_next + strspn(__c1_token_next, delimiters);
    if (!*start) {
        __c1_token_next = NULL;
        return NULL;
    }
    __c1_token_next = start + strcspn(start, delimiters);
    if (*__c1_token_next) {
        *__c1_token_next = 0;
        __c1_token_next++;
    } else
        __c1_token_next = NULL;
    return start;
}

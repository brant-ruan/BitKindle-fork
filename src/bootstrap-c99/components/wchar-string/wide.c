#include "wchar.h"

size_t wcslen(const wchar_t *s) {
    size_t n = 0;
    while (s[n])
        n++;
    return n;
}

wchar_t *wcscpy(wchar_t *restrict d, const wchar_t *restrict s) {
    wchar_t *r = d;
    while ((*d++ = *s++) != 0)
        ;
    return r;
}

wchar_t *wcsncpy(wchar_t *restrict d, const wchar_t *restrict s, size_t n) {
    size_t i = 0;
    while (i < n && s[i]) {
        d[i] = s[i];
        i++;
    }
    while (i < n)
        d[i++] = 0;
    return d;
}

wchar_t *wcscat(wchar_t *restrict d, const wchar_t *restrict s) {
    wcscpy(d + wcslen(d), s);
    return d;
}

wchar_t *wcsncat(wchar_t *restrict d, const wchar_t *restrict s, size_t n) {
    size_t i = 0, k = wcslen(d);
    while (i < n && s[i]) {
        d[k++] = s[i++];
    }
    d[k] = 0;
    return d;
}

int wcscmp(const wchar_t *a, const wchar_t *b) {
    while (*a && *a == *b) {
        a++;
        b++;
    }
    return *a < *b ? -1 : *a > *b;
}

int wcsncmp(const wchar_t *a, const wchar_t *b, size_t n) {
    size_t i;
    for (i = 0; i < n; i++) {
        if (a[i] != b[i])
            return a[i] < b[i] ? -1 : 1;
        if (!a[i])
            break;
    }
    return 0;
}

int wcscoll(const wchar_t *a, const wchar_t *b) {
    return wcscmp(a, b);
}

size_t wcsxfrm(wchar_t *restrict d, const wchar_t *restrict s, size_t n) {
    size_t i = 0;
    while (s[i]) {
        if (i < n)
            d[i] = s[i];
        i++;
    }
    if (i < n)
        d[i] = 0;
    return i;
}

wchar_t *wcschr(const wchar_t *s, wchar_t c) {
    for (;; s++) {
        if (*s == c)
            return (wchar_t *)s;
        if (!*s)
            return NULL;
    }
}

wchar_t *wcsrchr(const wchar_t *s, wchar_t c) {
    const wchar_t *p = NULL;
    do {
        if (*s == c)
            p = s;
    } while (*s++);
    return (wchar_t *)p;
}

size_t wcscspn(const wchar_t *s, const wchar_t *reject) {
    size_t n = 0;
    while (s[n] && !wcschr(reject, s[n]))
        n++;
    return n;
}

size_t wcsspn(const wchar_t *s, const wchar_t *accept) {
    size_t n = 0;
    while (s[n] && wcschr(accept, s[n]))
        n++;
    return n;
}

wchar_t *wcspbrk(const wchar_t *s, const wchar_t *accept) {
    while (*s) {
        if (wcschr(accept, *s))
            return (wchar_t *)s;
        s++;
    }
    return NULL;
}

wchar_t *wcsstr(const wchar_t *s, const wchar_t *needle) {
    size_t i;
    if (!*needle)
        return (wchar_t *)s;
    while (*s) {
        i = 0;
        while (needle[i] && s[i] == needle[i])
            i++;
        if (!needle[i])
            return (wchar_t *)s;
        s++;
    }
    return NULL;
}

wchar_t *wcstok(wchar_t *restrict s, const wchar_t *restrict delimiters, wchar_t **restrict state) {
    wchar_t *start;
    if (!s)
        s = *state;
    if (!s)
        return NULL;
    s += wcsspn(s, delimiters);
    if (!*s) {
        *state = NULL;
        return NULL;
    }
    start = s;
    s += wcscspn(s, delimiters);
    if (*s) {
        *s = 0;
        *state = s + 1;
    } else
        *state = NULL;
    return start;
}

wchar_t *wmemchr(const wchar_t *s, wchar_t c, size_t n) {
    size_t i;
    for (i = 0; i < n; i++)
        if (s[i] == c)
            return (wchar_t *)(s + i);
    return NULL;
}

int wmemcmp(const wchar_t *a, const wchar_t *b, size_t n) {
    size_t i;
    for (i = 0; i < n; i++)
        if (a[i] != b[i])
            return a[i] < b[i] ? -1 : 1;
    return 0;
}

wchar_t *wmemcpy(wchar_t *restrict d, const wchar_t *restrict s, size_t n) {
    size_t i;
    for (i = 0; i < n; i++)
        d[i] = s[i];
    return d;
}

wchar_t *wmemmove(wchar_t *d, const wchar_t *s, size_t n) {
    size_t i;
    if ((uintptr_t)d < (uintptr_t)s) {
        for (i = 0; i < n; i++)
            d[i] = s[i];
    } else {
        for (i = n; i; i--)
            d[i - 1] = s[i - 1];
    }
    return d;
}

wchar_t *wmemset(wchar_t *d, wchar_t c, size_t n) {
    size_t i;
    for (i = 0; i < n; i++)
        d[i] = c;
    return d;
}

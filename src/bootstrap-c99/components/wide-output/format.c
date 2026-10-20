#include "wchar.h"
#include <errno.h>
#include <string.h>
#include <fenv.h>

typedef struct {
    FILE *file;
    wchar_t *text;
    size_t capacity, count;
    int error;
} __c1_wout_format_sink;

static int __c1_wout_format_room(__c1_wout_format_sink *s, size_t n) {
    if (s->error)
        return -1;
    if (n > 2147483647U - s->count) {
        errno = ERANGE;
        s->error = 1;
        return -1;
    }
    return 0;
}

static int __c1_wout_format_unit(__c1_wout_format_sink *s, wchar_t c) {
    if (s->file) {
        if (fputwc(c, s->file) == WEOF) {
            s->error = 1;
            return -1;
        }
    } else {
        if (!s->capacity || s->count >= s->capacity - 1) {
            s->error = 1;
            return -1;
        }
        s->text[s->count] = c;
    }
    s->count++;
    return 0;
}

static int __c1_wout_format_units(__c1_wout_format_sink *s, const wchar_t *p, size_t n) {
    size_t i;
    if (__c1_wout_format_room(s, n))
        return -1;
    for (i = 0; i < n; i++)
        if (__c1_wout_format_unit(s, p[i]))
            return -1;
    return 0;
}

static int __c1_wout_format_write(__c1_wout_format_sink *s, const char *p, size_t n) {
    size_t i;
    if (__c1_wout_format_room(s, n))
        return -1;
    for (i = 0; i < n; i++)
        if (__c1_wout_format_unit(s, (unsigned char)p[i]))
            return -1;
    return 0;
}

static int __c1_wout_format_pad(__c1_wout_format_sink *s, int c, size_t n) {
    size_t i, amount;
    if (__c1_wout_format_room(s, n))
        return -1;
    if (!s->file) {
        amount = s->capacity && s->count < s->capacity - 1 ? s->capacity - 1 - s->count : 0;
        if (amount > n)
            amount = n;
        for (i = 0; i < amount; i++)
            s->text[s->count + i] = (wchar_t)c;
        s->count += amount;
        if (amount < n) {
            s->error = 1;
            return -1;
        }
        return 0;
    }
    for (i = 0; i < n; i++)
        if (__c1_wout_format_unit(s, (wchar_t)c))
            return -1;
    return 0;
}

#include "decimal.c"
#include "hex.c"

static int __c1_wout_format_wide(__c1_wout_format_sink *s, const wchar_t *text, int code,
                                 int single, int precision, int width, int left) {
    size_t n = 0, padding;
    wchar_t c = (wchar_t)code;
    if (single)
        n = 1;
    else {
        if (!text)
            text = L"(null)";
        while ((precision < 0 || n < (size_t)precision) && text[n])
            n++;
    }
    padding = (size_t)width > n ? width - n : 0;
    if (!left)
        __c1_wout_format_pad(s, ' ', padding);
    if (single)
        __c1_wout_format_units(s, &c, 1);
    else
        __c1_wout_format_units(s, text, n);
    if (left)
        __c1_wout_format_pad(s, ' ', padding);
    return s->error ? -1 : 0;
}

#include "engine.c"

int vswprintf(wchar_t *text, size_t capacity, const wchar_t *format, va_list ap) {
    __c1_wout_format_sink s;
    int result;
    s.file = NULL;
    s.text = text;
    s.capacity = capacity;
    s.count = 0;
    s.error = 0;
    if (!capacity)
        return -1;
    result = __c1_wout_format_run(&s, format, ap);
    text[s.count < capacity ? s.count : capacity - 1] = 0;
    return result;
}

int vfwprintf(FILE *file, const wchar_t *format, va_list ap) {
    __c1_wout_format_sink s;
    if (__c1_file_orient(file, 1))
        return -1;
    s.file = file;
    s.text = NULL;
    s.capacity = 0;
    s.count = 0;
    s.error = 0;
    return __c1_wout_format_run(&s, format, ap);
}

int vwprintf(const wchar_t *format, va_list ap) {
    return vfwprintf(stdout, format, ap);
}

int swprintf(wchar_t *text, size_t n, const wchar_t *format, ...) {
    va_list ap;
    int r;
    va_start(ap, format);
    r = vswprintf(text, n, format, ap);
    va_end(ap);
    return r;
}

int fwprintf(FILE *file, const wchar_t *format, ...) {
    va_list ap;
    int r;
    va_start(ap, format);
    r = vfwprintf(file, format, ap);
    va_end(ap);
    return r;
}

int wprintf(const wchar_t *format, ...) {
    va_list ap;
    int r;
    va_start(ap, format);
    r = vwprintf(format, ap);
    va_end(ap);
    return r;
}

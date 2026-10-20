#include "wchar.h"
#include <errno.h>

/* C-locale directive expansions fit below 128 bytes: year fields derive from
   an int tm_year, names are fixed English text or bounded timezone names.
   Literal wide units bypass the narrow formatter and remain unchanged. */
size_t wcsftime(wchar_t *restrict out, size_t capacity, const wchar_t *restrict format,
                const struct tm *restrict value) {
    char directive[4], text[128];
    size_t used = 0, n, i;
    wchar_t c;
    int k;
    if (!capacity)
        return 0;
    if (!out || !format || !value) {
        errno = EINVAL;
        return 0;
    }
    while ((c = *format++) != 0) {
        if (c != '%') {
            if (used + 1 >= capacity) {
                out[0] = 0;
                return 0;
            }
            out[used++] = c;
            continue;
        }
        directive[0] = '%';
        k = 1;
        if (*format == 'E' || *format == 'O')
            directive[k++] = (char)*format++;
        c = *format++;
        if (!c || c > 127) {
            out[0] = 0;
            return 0;
        }
        directive[k++] = (char)c;
        directive[k] = 0;
        text[0] = 0;
        n = strftime(text, sizeof(text), directive, value);
        /* z/Z may legitimately expand to nothing when timezone information is
     unavailable. Other supported directives always produce nonempty text. */
        if (!n && !(k == 2 && (c == 'z' || c == 'Z'))) {
            out[0] = 0;
            return 0;
        }
        if (n >= capacity - used) {
            out[0] = 0;
            return 0;
        }
        for (i = 0; i < n; i++)
            out[used++] = (unsigned char)text[i];
    }
    out[used] = 0;
    return used;
}

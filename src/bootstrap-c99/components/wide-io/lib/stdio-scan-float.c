#ifndef __C1_STDIO_SCAN_FLOAT_C
#define __C1_STDIO_SCAN_FLOAT_C

typedef struct {
    __c1_scan *source;
    char *text;
    size_t used, capacity;
    int width, error;
} __c1_scan_float_buffer;

static int __c1_scan_float_peek(__c1_scan_float_buffer *b) {
    return b->width ? __c1_scan_peek(b->source) : EOF;
}

static int __c1_scan_float_take(__c1_scan_float_buffer *b) {
    size_t capacity;
    char *p;
    if (b->used + 1 >= b->capacity) {
        capacity = b->capacity ? b->capacity * 2 : 64;
        if (capacity <= b->capacity) {
            errno = ERANGE;
            b->error = 1;
            return 0;
        }
        p = realloc(b->text, capacity);
        if (!p) {
            b->error = 1;
            return 0;
        }
        b->text = p;
        b->capacity = capacity;
    }
    b->text[b->used++] = (char)__c1_scan_take(b->source);
    b->width--;
    return 1;
}

static int __c1_scan_float_word(__c1_scan_float_buffer *b, const char *word) {
    while (*word) {
        if (tolower(__c1_scan_float_peek(b)) != *word++)
            return 0;
        if (!__c1_scan_float_take(b))
            return 0;
    }
    return 1;
}

/* Consume the longest valid prefix, including an incomplete exponent; unlike
   strtod, scanf must not push such a prefix back into the input stream. */
static int __c1_scan_float_token(__c1_scan *source, int width, char **result) {
    __c1_scan_float_buffer b;
    int c, base = 10, digits = 0, n;
    b.source = source;
    b.text = NULL;
    b.used = 0;
    b.capacity = 0;
    b.width = width;
    b.error = 0;
    c = __c1_scan_float_peek(&b);
    if (c == '+' || c == '-')
        if (!__c1_scan_float_take(&b))
            goto fail;
    c = tolower(__c1_scan_float_peek(&b));
#if !defined(__C1_C89_NUMERIC)
    if (c == 'i') {
        if (!__c1_scan_float_word(&b, "inf"))
            goto fail;
        if (tolower(__c1_scan_float_peek(&b)) == 'i')
            if (!__c1_scan_float_word(&b, "inity"))
                goto fail;
        goto success;
    }
    if (c == 'n') {
        if (!__c1_scan_float_word(&b, "nan"))
            goto fail;
        if (__c1_scan_float_peek(&b) == '(') {
            if (!__c1_scan_float_take(&b))
                goto fail;
            while (1) {
                c = __c1_scan_float_peek(&b);
                if (!isalnum(c) && c != '_')
                    break;
                if (!__c1_scan_float_take(&b))
                    goto fail;
            }
            if (__c1_scan_float_peek(&b) != ')' || !__c1_scan_float_take(&b))
                goto fail;
        }
        goto success;
    }
#endif
    if (__c1_scan_float_peek(&b) == '0') {
        if (!__c1_scan_float_take(&b))
            goto fail;
        digits = 1;
        c = __c1_scan_float_peek(&b);
#if !defined(__C1_C89_NUMERIC)
        if (c == 'x' || c == 'X') {
            if (!__c1_scan_float_take(&b))
                goto fail;
            base = 16;
            digits = 0;
        }
#endif
    }
    while (1) {
        n = __c1_scan_digit(__c1_scan_float_peek(&b));
        if (n < 0 || n >= base)
            break;
        if (!__c1_scan_float_take(&b))
            goto fail;
        digits = 1;
    }
    if (__c1_scan_float_peek(&b) == '.') {
        if (!__c1_scan_float_take(&b))
            goto fail;
        while (1) {
            n = __c1_scan_digit(__c1_scan_float_peek(&b));
            if (n < 0 || n >= base)
                break;
            if (!__c1_scan_float_take(&b))
                goto fail;
            digits = 1;
        }
    }
    if (!digits)
        goto fail;
    c = tolower(__c1_scan_float_peek(&b));
    if (c == (base == 16 ? 'p' : 'e')) {
        if (!__c1_scan_float_take(&b))
            goto fail;
        c = __c1_scan_float_peek(&b);
        if (c == '+' || c == '-')
            if (!__c1_scan_float_take(&b))
                goto fail;
        digits = 0;
        while (1) {
            c = __c1_scan_float_peek(&b);
            if (c < '0' || c > '9')
                break;
            if (!__c1_scan_float_take(&b))
                goto fail;
            digits = 1;
        }
        if (!digits)
            goto fail;
    }
success:
    b.text[b.used] = 0;
    *result = b.text;
    return 1;
fail:
    free(b.text);
    *result = NULL;
    return b.error ? -1 : 0;
}

#endif

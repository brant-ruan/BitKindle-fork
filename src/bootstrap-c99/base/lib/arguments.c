#ifndef __C1_ARGUMENTS_C
#define __C1_ARGUMENTS_C

/* Self-contained startup component: only the existing kernel32 imports. */
static int __c1_parse_arguments_cp(const char *text, int *count, char ***arguments,
                                   unsigned int cp) {
    typedef int Lead(unsigned int, unsigned char);
    Lead *lead = *(Lead **)0x401118;
    typedef void *Heap(void);
    typedef void *Allocate(void *, unsigned int, unsigned long long);
    Heap *heap = *(Heap **)0x401080;
    Allocate *allocate = *(Allocate **)0x401088;
    unsigned long long length = 0;
    const char *p;
    char **v, *out;
    int n = 0, quoted = 0, slashes, i;
    if (!text || !count || !arguments)
        return 1;
    while (text[length]) {
        if (length == 2147483646)
            return 1;
        length++;
    }
    v = allocate(heap(), 0, (length + 2) * 9);
    if (!v)
        return 1;
    out = (char *)(v + length + 2);
    p = text;
    /* argv[0] treats quotes specially, but backslashes literally. */
    v[n++] = out;
    while (*p) {
        if (*p == '"') {
            quoted = !quoted;
            p++;
            continue;
        }
        if (!quoted && (*p == ' ' || *p == '\t'))
            break;
        if ((unsigned char)*p >= 128 && p[1] && lead(cp, (unsigned char)*p))
            *out++ = *p++;
        *out++ = *p++;
    }
    *out++ = 0;
    for (;;) {
        while (*p == ' ' || *p == '\t')
            p++;
        if (!*p)
            break;
        v[n++] = out;
        quoted = 0;
        while (*p) {
            if (!quoted && (*p == ' ' || *p == '\t'))
                break;
            slashes = 0;
            while (*p == '\\') {
                slashes++;
                p++;
            }
            if (*p == '"') {
                for (i = 0; i < slashes / 2; i++)
                    *out++ = '\\';
                if (slashes & 1) {
                    *out++ = '"';
                    p++;
                } else if (quoted && p[1] == '"') {
                    *out++ = '"';
                    p += 2;
                } else {
                    quoted = !quoted;
                    p++;
                }
            } else {
                for (i = 0; i < slashes; i++)
                    *out++ = '\\';
                if (!*p || (!quoted && (*p == ' ' || *p == '\t')))
                    break;
                if ((unsigned char)*p >= 128 && p[1] && lead(cp, (unsigned char)*p))
                    *out++ = *p++;
                *out++ = *p++;
            }
        }
        *out++ = 0;
    }
    v[n] = 0;
    *count = n;
    *arguments = v;
    return 0;
}

static int __c1_parse_arguments(const char *text, int *count, char ***arguments) {
    return __c1_parse_arguments_cp(text, count, arguments, 0);
}

static int __c1_get_arguments(int *count, char ***arguments) {
    typedef char *CommandLine(void);
    CommandLine *command = *(CommandLine **)0x4010d8;
    return __c1_parse_arguments(command(), count, arguments);
}
#endif

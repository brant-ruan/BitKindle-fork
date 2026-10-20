/* First C-written C1 component. Built by C0, never by an external compiler.
   Standalone driver prints one token per line for independent validation. */
int source[32768];
int names[16384];
int lengths[256];
int size;
int cursor;
int count;
int failed;
int value;
int line;

int seed(int id, int a, int b, int c) {
    int i, word, ch, n = 0;
    for (i = 0; i < 12; i = i + 1) {
        if (i < 4)
            word = a;
        else if (i < 8)
            word = b;
        else
            word = c;
        ch = (word >> ((i & 3) * 8)) & 255;
        if (ch != 0) {
            names[id * 64 + n] = ch;
            n = n + 1;
        }
    }
    lengths[id] = n;
    return 0;
}

int initialize(void) {
    seed(0, 0x746e69, 0, 0);
    seed(1, 0x75746572, 0x6e72, 0);
    seed(2, 0x6669, 0, 0);
    seed(3, 0x65736c65, 0, 0);
    seed(4, 0x6c696877, 0x65, 0);
    seed(5, 0x64696f76, 0, 0);
    seed(6, 0x6e69616d, 0, 0);
    seed(7, 0x61657262, 0x6b, 0);
    seed(8, 0x746e6f63, 0x65756e69, 0);
    seed(9, 0x726f66, 0, 0);
    seed(10, 0x30635f5f, 0x7465675f, 0x72616863);
    seed(11, 0x30635f5f, 0x7475705f, 0x72616863);
    count = 12;
    line = 1;
    return 0;
}

int peek(void) {
    if (cursor == size)
        return -1;
    return source[cursor];
}

int take(void) {
    int c = peek();
    if (c != -1) {
        cursor = cursor + 1;
        if (c == 10)
            line = line + 1;
    }
    return c;
}

int alpha(int c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

int digit(int c) {
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    return -1;
}

int intern(void) {
    int start = cursor;
    int n, i, j, equal;
    while (alpha(peek()) || (peek() >= '0' && peek() <= '9'))
        take();
    n = cursor - start;
    if (n > 63) {
        failed = 1;
        return 0;
    }
    for (i = 0; i < count; i = i + 1) {
        equal = lengths[i] == n;
        for (j = 0; j < n && equal; j = j + 1) {
            if (names[i * 64 + j] != source[start + j])
                equal = 0;
        }
        if (equal)
            return 256 + i;
    }
    if (count == 256) {
        failed = 1;
        return 0;
    }
    for (j = 0; j < n; j = j + 1)
        names[count * 64 + j] = source[start + j];
    lengths[count] = n;
    count = count + 1;
    return 255 + count;
}

int number(void) {
    int base = 10;
    int d, n = 0;
    value = 0;
    if (peek() == '0') {
        base = 8;
        if (cursor + 1 < size && (source[cursor + 1] == 'x' || source[cursor + 1] == 'X')) {
            base = 16;
            take();
            take();
        }
    }
    while (digit(peek()) >= 0) {
        d = digit(take());
        if (d >= base || value > (2147483647 - d) / base) {
            failed = 1;
            return 0;
        }
        value = value * base + d;
        n = n + 1;
    }
    if (n == 0 || alpha(peek())) {
        failed = 1;
        return 0;
    }
    return 144;
}

int character(void) {
    int c = take();
    if (c == -1 || c == 10 || c == 13 || c == 39) {
        failed = 1;
        return 0;
    }
    if (c == 92) {
        c = take();
        if (c == 'n')
            c = 10;
        else if (c == 'r')
            c = 13;
        else if (c == 't')
            c = 9;
        else if (c == '0')
            c = 0;
        else if (c != 92 && c != 39 && c != 34) {
            failed = 1;
            return 0;
        }
    }
    if (take() != 39) {
        failed = 1;
        return 0;
    }
    value = c;
    return 144;
}

int lex(void) {
    int c, d, previous;
    while (1) {
        c = peek();
        if (c == -1)
            return 0;
        if (c <= 32) {
            take();
            continue;
        }
        if (alpha(c))
            return intern();
        if (c >= '0' && c <= '9')
            return number();
        take();
        d = peek();
        if (c == '/' && d == '/') {
            while (peek() != -1 && peek() != 10)
                take();
            continue;
        }
        if (c == '/' && d == '*') {
            take();
            previous = 0;
            while (1) {
                d = take();
                if (d == -1) {
                    failed = 1;
                    return 0;
                }
                if (previous == '*' && d == '/')
                    break;
                previous = d;
            }
            continue;
        }
        if (c == 39)
            return character();
        if (c == '=' && d == '=') {
            take();
            return 128;
        }
        if (c == '!' && d == '=') {
            take();
            return 129;
        }
        if (c == '<' && d == '=') {
            take();
            return 130;
        }
        if (c == '>' && d == '=') {
            take();
            return 131;
        }
        if (c == '<' && d == '<') {
            take();
            return 132;
        }
        if (c == '>' && d == '>') {
            take();
            return 133;
        }
        if (c == '&' && d == '&') {
            take();
            return 134;
        }
        if (c == '|' && d == '|') {
            take();
            return 135;
        }
        return c;
    }
}

int print_number(int n) {
    if (n >= 10)
        print_number(n / 10);
    __c0_putchar('0' + n % 10);
    return 0;
}

int main(void) {
    int c, token, id, i;
    initialize();
    while ((c = __c0_getchar()) != -1) {
        if (size == 32768)
            return 2;
        source[size] = c;
        size = size + 1;
    }
    token = lex();
    while (token != 0 && !failed) {
        print_number(token);
        if (token == 144) {
            __c0_putchar(':');
            print_number(value);
        } else if (token >= 256) {
            id = token - 256;
            __c0_putchar(':');
            for (i = 0; i < lengths[id]; i = i + 1)
                __c0_putchar(names[id * 64 + i]);
        }
        __c0_putchar(10);
        token = lex();
    }
    return failed;
}

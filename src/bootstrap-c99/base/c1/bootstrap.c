/* C1 integer-subset compiler, written entirely in the C0-supported C subset.
   Reads C from stdin and directly emits a Windows x64 PE image on stdout. */
int source[262144];
int names[65536];
int lengths[1024];
int size;
int cursor;
int count;
int failed;
int value;
int literal_type;
int literal_high;
int string_pool[131072];
int string_used;
int literal_string_start;
int literal_string_length;
int joined_string_start;
int joined_string_length;
int line;

int trigraph(int c) {
    if (c == '=')
        return '#';
    if (c == '/')
        return 92;
    if (c == 39)
        return '^';
    if (c == '(')
        return '[';
    if (c == ')')
        return ']';
    if (c == '!')
        return '|';
    if (c == '<')
        return '{';
    if (c == '>')
        return '}';
    if (c == '-')
        return '~';
    return 0;
}

/* Physical newline mapping and trigraph replacement precede splicing.
   Separate forward passes prevent rescanning newly joined character pairs. */
int source_phases(void) {
    int input = 0, output = 0, c, replacement;
    while (input < size) {
        c = source[input];
        input = input + 1;
        if (c == 13) {
            if (input < size && source[input] == 10)
                input = input + 1;
            c = 10;
        } else if (c == '?' && input + 1 < size && source[input] == '?') {
            replacement = trigraph(source[input + 1]);
            if (replacement) {
                c = replacement;
                input = input + 2;
            }
        }
        source[output] = c;
        output = output + 1;
    }
    size = output;
    input = 0;
    output = 0;
    while (input < size) {
        c = source[input];
        input = input + 1;
        if (c == 92 && input < size && source[input] == 10)
            input = input + 1;
        else {
            source[output] = c;
            output = output + 1;
        }
    }
    size = output;
    return 0;
}

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
    seed(12, 0x6f747561, 0, 0);
    seed(13, 0x65736163, 0, 0);
    seed(14, 0x72616863, 0, 0);
    seed(15, 0x736e6f63, 0x74, 0);
    seed(16, 0x61666564, 0x746c75, 0);
    seed(17, 0x6f64, 0, 0);
    seed(18, 0x62756f64, 0x656c, 0);
    seed(19, 0x6d756e65, 0, 0);
    seed(20, 0x65747865, 0x6e72, 0);
    seed(21, 0x616f6c66, 0x74, 0);
    seed(22, 0x6f746f67, 0, 0);
    seed(23, 0x676e6f6c, 0, 0);
    seed(24, 0x69676572, 0x72657473, 0);
    seed(25, 0x726f6873, 0x74, 0);
    seed(26, 0x6e676973, 0x6465, 0);
    seed(27, 0x657a6973, 0x666f, 0);
    seed(28, 0x74617473, 0x6369, 0);
    seed(29, 0x75727473, 0x7463, 0);
    seed(30, 0x74697773, 0x6863, 0);
    seed(31, 0x65707974, 0x666564, 0);
    seed(32, 0x6f696e75, 0x6e, 0);
    seed(33, 0x69736e75, 0x64656e67, 0);
    seed(34, 0x616c6f76, 0x656c6974, 0);
    seed(35, 0x696c6e69, 0x656e, 0);
    seed(36, 0x74736572, 0x74636972, 0);
    seed(37, 0x6f6f425f, 0x6c, 0);
    seed(38, 0x6d6f435f, 0x78656c70, 0);
    seed(39, 0x616d495f, 0x616e6967, 0x7972);
    count = 40;
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
    if (count == 1024) {
        failed = 1;
        return 0;
    }
    for (j = 0; j < n; j = j + 1)
        names[count * 64 + j] = source[start + j];
    lengths[count] = n;
    count = count + 1;
    return 255 + count;
}

int word_bits(int low, int high) {
    int n = (high & 32767) * 65536 + low;
    if (high >= 32768)
        n = (-2147483647 - 1) + n;
    return n;
}

int number(void) {
    int base = 10;
    int d, n = 0, low = 0, high = 0, upper = 0, top = 0, u = 0, l = 0, c;
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
        if (d >= base) {
            failed = 1;
            return 0;
        }
        low = low * base + d;
        high = high * base + low / 65536;
        low = low & 65535;
        upper = upper * base + high / 65536;
        high = high & 65535;
        top = top * base + upper / 65536;
        upper = upper & 65535;
        if (top > 65535) {
            failed = 1;
            return 0;
        }
        n = n + 1;
    }
    if (peek() == 'u' || peek() == 'U') {
        take();
        u = 1;
    }
    if (peek() == 'l' || peek() == 'L') {
        c = take();
        l = 1;
        if (peek() == c) {
            take();
            l = 2;
        }
    }
    if (!u && (peek() == 'u' || peek() == 'U')) {
        take();
        u = 1;
    }
    if (n == 0 || alpha(peek())) {
        failed = 1;
        return 0;
    }
    value = word_bits(low, high);
    literal_high = word_bits(upper, top);
    if (l == 2 || top || upper) {
        literal_type = 10;
        if (u || top >= 32768)
            literal_type = 11;
        if (!u && base == 10 && top >= 32768) {
            failed = 1;
            return 0;
        }
        return 144;
    }
    literal_type = 1;
    if (l)
        literal_type = 3;
    if (u)
        literal_type = literal_type + 1;
    if (high >= 32768 && !u) {
        if (l || base == 10)
            literal_type = 4;
        else
            literal_type = 2;
    }
    return 144;
}

int escape(void) {
    int c = take(), n, escape_count = 0;
    if (c == 'a')
        return 7;
    if (c == 'b')
        return 8;
    if (c == 'f')
        return 12;
    if (c == 'n')
        return 10;
    if (c == 'r')
        return 13;
    if (c == 't')
        return 9;
    if (c == 'v')
        return 11;
    if (c == 92 || c == 39 || c == 34 || c == '?')
        return c;
    if (c >= '0' && c <= '7') {
        n = c - '0';
        escape_count = 1;
        while (escape_count < 3 && peek() >= '0' && peek() <= '7') {
            n = n * 8 + take() - '0';
            escape_count = escape_count + 1;
        }
        if (n > 255) {
            failed = 1;
            return 0;
        }
        return n;
    }
    if (c == 'x') {
        n = 0;
        while (digit(peek()) >= 0) {
            n = n * 16 + digit(take());
            escape_count = escape_count + 1;
            if (n > 255) {
                failed = 1;
                return 0;
            }
        }
        if (!escape_count)
            failed = 1;
        return n;
    }
    failed = 1;
    return 0;
}

int string_token(void) {
    int c;
    literal_string_start = string_used;
    while (!failed) {
        c = take();
        if (c == 34)
            break;
        if (c == -1 || c == 10 || c == 13) {
            failed = 1;
            return 0;
        }
        if (c == 92)
            c = escape();
        if (string_used == 131072) {
            failed = 1;
            return 0;
        }
        string_pool[string_used] = c;
        string_used = string_used + 1;
    }
    literal_string_length = string_used - literal_string_start;
    return 145;
}

int character(void) {
    int c = take();
    if (c == -1 || c == 10 || c == 13 || c == 39) {
        failed = 1;
        return 0;
    }
    if (c == 92)
        c = escape();
    if (take() != 39) {
        failed = 1;
        return 0;
    }
    value = c;
    literal_type = 1;
    literal_high = 0;
    return 144;
}

int lex(void) {
    int c, d, previous;
    while (1) {
        c = peek();
        if (c == -1)
            return 0;
        if (c == 32 || (c >= 9 && c <= 13)) {
            take();
            continue;
        }
        if (c < 32) {
            failed = 1;
            return 0;
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
        if (c == 34)
            return string_token();
        if (c == '<' && d == ':') {
            take();
            return '[';
        }
        if (c == ':' && d == '>') {
            take();
            return ']';
        }
        if (c == '<' && d == '%') {
            take();
            return '{';
        }
        if (c == '%' && d == '>') {
            take();
            return '}';
        }
        if (c == '%' && d == ':') {
            take();
            if (cursor + 1 < size && source[cursor] == '%' && source[cursor + 1] == ':') {
                take();
                take();
                return 172;
            }
            return '#';
        }
        if (c == '+' && d == '+') {
            take();
            return 170;
        }
        if (c == '-' && d == '-') {
            take();
            return 171;
        }
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
            if (peek() == '=') {
                take();
                return 168;
            }
            return 132;
        }
        if (c == '>' && d == '>') {
            take();
            if (peek() == '=') {
                take();
                return 169;
            }
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
        if (d == '=') {
            if (c == '+') {
                take();
                return 160;
            }
            if (c == '-') {
                take();
                return 161;
            }
            if (c == '*') {
                take();
                return 162;
            }
            if (c == '/') {
                take();
                return 163;
            }
            if (c == '%') {
                take();
                return 164;
            }
            if (c == '&') {
                take();
                return 165;
            }
            if (c == '|') {
                take();
                return 166;
            }
            if (c == '^') {
                take();
                return 167;
            }
        }
        return c;
    }
}

/* Machine code and PE image are generated here, inside the C compiler. */
int image[262656];
int position;
int token;
int label_address[1024];
int label_head[1024];
int local_offset[1024];
int local_depth[1024];
int binding_name[256];
int binding_offset[256];
int binding_depth[256];
int local_type[1024];
int binding_type[256];
int binding_bytes[256];
int local_bytes;
int frame_peak;
int frame_patch;
int initializer_depth;
int initializer_start;
int initializer_tail;
int initializer_call;
int array_base[1024];
int array_length[1024];
int array_bytes[1024];
int array_count;
int decl_kind[256];
int decl_next[256];
int decl_length[256];
int decl_count;
int decl_name;
int decl_depth;
int decl_parentheses;
int global_type[1024];
int parameter_type[131072];
int expression_type;
int expression_zero;
int expression_load;
int expression_array_bytes;
int expression_array_type;
int global_extent[1024];
int return_type;
int global_address[1024];
int symbol_flags[1024];
int arity[1024];
int function_address[1024];
int call_position[16384];
int call_symbol[16384];
int call_count;
int allocation;
int local_count;
int block_depth;
int expression_depth;
int unary_depth;
int statement_depth;
int break_head;
int continue_target;
int loop_depth;

int error(void) {
    failed = 1;
    token = 0;
    return 0;
}

int emit(int b) {
    if (failed)
        return 0;
    if (position >= 262656)
        return error();
    image[position] = b & 255;
    position = position + 1;
    return 0;
}

int sequence(int word, int n) {
    while (n > 0 && !failed) {
        emit(word);
        word = word >> 8;
        n = n - 1;
    }
    return 0;
}

int emit32(int n) {
    return sequence(n, 4);
}

int write32(int p, int n) {
    int i;
    if (p < 0 || p > 262652)
        return error();
    for (i = 0; i < 4; i = i + 1) {
        image[p + i] = n & 255;
        n = n >> 8;
    }
    return 0;
}

int read32(int p) {
    return image[p] | (image[p + 1] << 8) | (image[p + 2] << 16) | (image[p + 3] << 24);
}

int relative(int address) {
    return emit32(address - (position + 0x400e00 + 4));
}

int indirect(int address) {
    sequence(0x15ff, 2);
    return relative(address);
}

int jump(int condition) {
    int p;
    if (condition == 0)
        emit(0xe9);
    else {
        emit(0x0f);
        emit(condition);
    }
    p = position;
    emit32(0);
    return p;
}

int patch(int p, int target) {
    return write32(p, target - p - 4);
}

int call(int id) {
    if (call_count == 16384)
        return error();
    emit(0xe8);
    call_position[call_count] = position;
    call_symbol[call_count] = id;
    call_count = call_count + 1;
    return emit32(0);
}

int next(void) {
    if (failed) {
        token = 0;
        return 0;
    }
    token = lex();
    if (failed)
        token = 0;
    return token;
}

int expect(int wanted) {
    if (token != wanted)
        return error();
    next();
    return 0;
}

int name(void) {
    if (token == 262 || token == 266 || token == 267 || token >= 296)
        return token - 256;
    return error();
}

/* Labels use their own function-wide namespace. Peek without consuming the
   expression's leading identifier or retaining speculative string bytes. */
int label_ahead(void) {
    int saved_cursor = cursor, saved_line = line, saved_strings = string_used, found;
    if (!(token == 262 || token == 266 || token == 267 || token >= 296))
        return 0;
    found = lex() == ':';
    cursor = saved_cursor;
    line = saved_line;
    string_used = saved_strings;
    return found;
}

int prologue(void) {
    int start, tail, branch;
    emit(0x55);
    sequence(0xe58948, 3);
    emit(0xb8);
    frame_patch = position;
    emit32(0);
    start = position;
    emit(0x3d);
    emit32(4096);
    tail = jump(0x82);
    sequence(0xec8148, 3);
    emit32(4096);
    sequence(0x240485, 3);
    emit(0x2d);
    emit32(4096);
    branch = jump(0);
    patch(branch, start);
    patch(tail, position);
    sequence(0xc42948, 3);
    sequence(0x240485, 3);
    return 0;
}

int epilogue(void) {
    return sequence(0xc3c9, 2);
}

/* Types: void=0, int=1, unsigned int=2, long=3, unsigned long=4,
   signed char=5, unsigned char=6, short=7, unsigned short=8, char=9.
   The Windows data model uses 32-bit int/long and signed plain char. */
int type_start(int t) {
    return t == 256 || t == 261 || t == 270 || t == 279 || t == 281 || t == 282 || t == 289;
}

int specifiers(void) {
    int i = 0, ch = 0, sh = 0, lo = 0, si = 0, un = 0, vo = 0, n = 0, t;
    while (type_start(token) && !failed) {
        t = token;
        next();
        n = n + 1;
        if (t == 256)
            i = i + 1;
        if (t == 261)
            vo = vo + 1;
        if (t == 270)
            ch = ch + 1;
        if (t == 279)
            lo = lo + 1;
        if (t == 281)
            sh = sh + 1;
        if (t == 282)
            si = si + 1;
        if (t == 289)
            un = un + 1;
    }
    if (!n || i > 1 || ch > 1 || sh > 1 || lo > 2 || si > 1 || un > 1 || vo > 1)
        return error();
    if ((si && un) || (sh && lo) || (ch && (i || sh || lo)))
        return error();
    if (vo) {
        if (n != 1)
            return error();
        return 0;
    }
    if (ch) {
        if (un)
            return 6;
        if (si)
            return 5;
        return 9;
    }
    if (sh)
        return 7 + un;
    if (lo == 2)
        return 10 + un;
    if (lo)
        return 3 + un;
    return 1 + un;
}

int is_array(int t) {
    return t >= 4096 && t % 256 == 0;
}

int wide_type(int t) {
    return t == 10 || t == 11 || (t >= 16 && !is_array(t));
}

int type_size(int t) {
    if (is_array(t)) {
        t = array_bytes[(t - 4096) / 256];
        if (!t)
            return error();
        return t;
    }
    if (wide_type(t))
        return 8;
    if (t == 5 || t == 6 || t == 9)
        return 1;
    if (t == 7 || t == 8)
        return 2;
    if (t >= 1 && t <= 4)
        return 4;
    return error();
}

int pointers(int t) {
    while (token == '*' && !failed) {
        next();
        t = t + 16;
        if (t > 265)
            return error();
    }
    return t;
}

int make_pointer(int t) {
    if (t >= 4096 && t % 256 >= 240)
        return error();
    if (t < 4096 && t >= 240)
        return error();
    return t + 16;
}

int make_array(int base, int n) {
    int i, bytes = type_size(base);
    if (failed)
        return 0;
    if (n > 0x700000 / bytes)
        return error();
    for (i = 0; i < array_count; i = i + 1) {
        if (array_base[i] == base && array_length[i] == n)
            return 4096 + i * 256;
    }
    if (array_count == 1024)
        return error();
    array_base[array_count] = base;
    array_length[array_count] = n;
    array_bytes[array_count] = n * bytes;
    array_count = array_count + 1;
    return 4096 + (array_count - 1) * 256;
}

int decl_node(int kind, int n) {
    int id;
    if (decl_count == 255)
        return error();
    decl_count = decl_count + 1;
    id = decl_count;
    decl_kind[id] = kind;
    decl_length[id] = n;
    decl_next[id] = 0;
    return id;
}

/* Constructors are linked from the identifier outwards. Parentheses keep
   their inner chain before following suffixes; prefix stars come last. */
int declarator_chain(int abstract) {
    int stars = 0, head = 0, tail = 0, node, n;
    decl_depth = decl_depth + 1;
    if (decl_depth > 64)
        return error();
    while (token == '*') {
        stars = stars + 1;
        next();
    }
    if (token == '(') {
        decl_parentheses = 1;
        next();
        head = declarator_chain(abstract);
        expect(')');
        tail = head;
        while (tail && decl_next[tail])
            tail = decl_next[tail];
    } else if (token == 262 || token == 266 || token == 267 || token >= 296) {
        if (abstract == 1 || decl_name != -1)
            return error();
        decl_name = name();
        next();
    } else if (!abstract)
        return error();
    while (!failed && token == '[') {
        next();
        n = 0;
        if (token != ']') {
            if (token != 144 || literal_high || value <= 0)
                return error();
            n = value;
            next();
        }
        expect(']');
        node = decl_node(2, n);
        if (tail)
            decl_next[tail] = node;
        else
            head = node;
        tail = node;
    }
    while (stars && !failed) {
        node = decl_node(1, 0);
        if (tail)
            decl_next[tail] = node;
        else
            head = node;
        tail = node;
        stars = stars - 1;
    }
    decl_depth = decl_depth - 1;
    return head;
}

int apply_declarator(int node, int base) {
    int t;
    if (!node || failed)
        return base;
    t = apply_declarator(decl_next[node], base);
    if (decl_kind[node] == 1)
        return make_pointer(t);
    return make_array(t, decl_length[node]);
}

int declarator(int base, int abstract) {
    int head;
    decl_count = 0;
    decl_name = -1;
    decl_depth = 0;
    decl_parentheses = 0;
    head = declarator_chain(abstract);
    return apply_declarator(head, base);
}

int promoted(int t) {
    if (t >= 5 && t <= 9)
        return 1;
    return t;
}

int unsigned_type(int t) {
    return t == 2 || t == 4 || t == 11;
}

int common_type(int a, int b) {
    if (a >= 16 || b >= 16) {
        if (a == b)
            return a;
        if (a >= 16 && b >= 16 && (a == 16 || b == 16))
            return 16;
        return error();
    }
    a = promoted(a);
    b = promoted(b);
    if (!a || !b)
        return error();
    if (a == 11 || b == 11)
        return 11;
    if (a == 10 || b == 10)
        return 10;
    if (a == 4 || b == 4 || (a + b == 5))
        return 4;
    if (a == 3 || b == 3)
        return 3;
    if (a == 2 || b == 2)
        return 2;
    return 1;
}

int convert(int t) {
    if (is_array(t))
        return error();
    if (t && !expression_type)
        return error();
    if (t >= 16) {
        if (expression_type >= 16) {
            if (t != expression_type && t != 16 && expression_type != 16)
                return error();
        } else if (!expression_zero)
            return error();
        expression_type = t;
        return 0;
    }
    if (t && expression_type >= 16)
        return error();
    if (wide_type(t) && !wide_type(expression_type)) {
        if (unsigned_type(promoted(expression_type)))
            sequence(0xc089, 2);
        else
            sequence(0xc06348, 3);
    }
    if (t && !wide_type(t) && wide_type(expression_type))
        sequence(0xc089, 2);
    if (t == 5 || t == 9)
        sequence(0xc0be0f, 3);
    if (t == 6)
        sequence(0xc0b60f, 3);
    if (t == 7)
        sequence(0xc0bf0f, 3);
    if (t == 8)
        sequence(0xc0b70f, 3);
    expression_type = t;
    return 0;
}

int cast_to(int t) {
    if (t && !expression_type)
        return error();
    if (t >= 16) {
        if (!wide_type(expression_type) && !unsigned_type(promoted(expression_type)))
            sequence(0xc06348, 3);
        expression_type = t;
        return 0;
    }
    if (expression_type >= 16)
        expression_type = 11;
    convert(t);
    if (!t)
        expression_zero = 0;
    return 0;
}

int constant(int n) {
    expression_type = 1;
    expression_zero = n == 0;
    emit(0xb8);
    return emit32(n);
}

int load_opcode(int t, int modrm) {
    expression_array_bytes = 0;
    expression_array_type = 0;
    expression_load = position;
    expression_zero = 0;
    if (wide_type(t)) {
        emit(0x48);
        emit(0x8b);
    } else if (t >= 5) {
        emit(0x0f);
        if (t == 5 || t == 9)
            emit(0xbe);
        if (t == 6)
            emit(0xb6);
        if (t == 7)
            emit(0xbf);
        if (t == 8)
            emit(0xb7);
    } else
        emit(0x8b);
    emit(modrm);
    expression_type = t;
    return 0;
}

int array_value(int t) {
    expression_array_type = t;
    expression_array_bytes = type_size(t);
    expression_type = make_pointer(array_base[(t - 4096) / 256]);
    expression_zero = 0;
    return 0;
}

int indirect_value(int t) {
    sequence(0xc28948, 3);
    if (is_array(t)) {
        expression_load = position;
        array_value(t);
    } else
        load_opcode(t, 0);
    return -1;
}

int load(int id) {
    if (local_offset[id] != 0) {
        if (is_array(local_type[id])) {
            expression_load = position;
            sequence(0x858d48, 3);
            emit32(-local_offset[id]);
            array_value(local_type[id]);
            return local_offset[id];
        }
        load_opcode(local_type[id], 0x85);
        emit32(-local_offset[id]);
        return local_offset[id];
    }
    if (global_address[id] != 0) {
        if (is_array(global_type[id])) {
            expression_load = position;
            sequence(0x058d48, 3);
            relative(global_address[id]);
            array_value(global_type[id]);
            return -id - 2;
        }
        load_opcode(global_type[id], 5);
        relative(global_address[id]);
        return -id - 2;
    }
    return error();
}

int store(int lvalue, int t) {
    int n = type_size(t), opcode = 0x89;
    if (n == 1)
        opcode = 0x88;
    if (lvalue == -1)
        emit(0x59);
    if (n == 8)
        emit(0x48);
    if (n == 2)
        emit(0x66);
    emit(opcode);
    if (lvalue == -1)
        return emit(1);
    if (lvalue < -1) {
        emit(5);
        return relative(global_address[-lvalue - 2]);
    }
    emit(0x85);
    return emit32(-lvalue);
}

int boolean(int cc) {
    if (!expression_type)
        return error();
    if (wide_type(expression_type))
        emit(0x48);
    expression_type = 1;
    expression_zero = 0;
    sequence(0xc085, 2);
    emit(0x0f);
    emit(cc);
    emit(0xc0);
    return sequence(0xc0b60f, 3);
}

int comparison(int cc) {
    if (wide_type(expression_type))
        emit(0x48);
    expression_type = 1;
    sequence(0xc839, 2);
    emit(0x0f);
    emit(cc);
    emit(0xc0);
    return sequence(0xc0b60f, 3);
}

int precedence(int op) {
    if (op == '=' || (op >= 160 && op <= 169))
        return 1;
    if (op == '?')
        return 2;
    if (op == 135)
        return 3;
    if (op == 134)
        return 4;
    if (op == '|')
        return 5;
    if (op == '^')
        return 6;
    if (op == '&')
        return 7;
    if (op == 128 || op == 129)
        return 8;
    if (op == '<' || op == '>' || op == 130 || op == 131)
        return 9;
    if (op == 132 || op == 133)
        return 10;
    if (op == '+' || op == '-')
        return 11;
    if (op == '*' || op == '/' || op == '%')
        return 12;
    return 0;
}

int compound(int op) {
    if (op == 160)
        return '+';
    if (op == 161)
        return '-';
    if (op == 162)
        return '*';
    if (op == 163)
        return '/';
    if (op == 164)
        return '%';
    if (op == 165)
        return '&';
    if (op == 166)
        return '|';
    if (op == 167)
        return '^';
    if (op == 168)
        return 132;
    if (op == 169)
        return 133;
    return 0;
}

int pointer_binary(int op, int a, int b, int left_zero) {
    int n, cc = 0;
    if (op == '+' || op == '-') {
        if (op == '-' && a >= 17 && a == b) {
            n = type_size(a - 16);
            sequence(0xc18948, 3);
            emit(0x58);
            sequence(0xc82948, 3);
            sequence(0x9948, 2);
            emit(0xb9);
            emit32(n);
            sequence(0xf9f748, 3);
            expression_type = 10;
        } else if (a >= 16 && b > 0 && b < 16) {
            n = type_size(a - 16);
            if (!wide_type(b) && !unsigned_type(promoted(b)))
                sequence(0xc06348, 3);
            sequence(0xc18948, 3);
            emit(0x58);
            sequence(0xc96948, 3);
            emit32(n);
            if (op == '+')
                sequence(0xc80148, 3);
            else
                sequence(0xc82948, 3);
            expression_type = a;
        } else if (op == '+' && b >= 16 && a > 0 && a < 16) {
            n = type_size(b - 16);
            sequence(0xc18948, 3);
            emit(0x58);
            if (!wide_type(a) && !unsigned_type(promoted(a)))
                sequence(0xc06348, 3);
            sequence(0xc06948, 3);
            emit32(n);
            sequence(0xc80148, 3);
            expression_type = b;
        } else
            return error();
    } else {
        if (a < 16 && !left_zero)
            return error();
        if (b < 16 && !expression_zero)
            return error();
        if (a >= 16 && b >= 16 && a != b && a != 16 && b != 16)
            return error();
        if (op == 128)
            cc = 0x94;
        if (op == 129)
            cc = 0x95;
        if (op == '<')
            cc = 0x92;
        if (op == '>')
            cc = 0x97;
        if (op == 130)
            cc = 0x96;
        if (op == 131)
            cc = 0x93;
        if (!cc)
            return error();
        if (op != 128 && op != 129 && (a < 16 || b < 16))
            return error();
        if (op != 128 && op != 129 && a != b)
            return error();
        sequence(0xc18948, 3);
        emit(0x58);
        sequence(0xc83948, 3);
        emit(0x0f);
        emit(cc);
        emit(0xc0);
        sequence(0xc0b60f, 3);
        expression_type = 1;
    }
    expression_zero = 0;
    return 0;
}

int binary(int op, int left_type, int right_type, int left_zero) {
    int t, u, wide;
    if (left_type >= 16 || right_type >= 16)
        return pointer_binary(op, left_type, right_type, left_zero);
    t = common_type(left_type, right_type);
    expression_zero = 0;
    if (op == 132 || op == 133)
        t = promoted(left_type);
    u = unsigned_type(t);
    wide = wide_type(t);
    expression_type = right_type;
    convert(t);
    if (wide)
        sequence(0xc18948, 3);
    else
        sequence(0xc189, 2);
    emit(0x58);
    expression_type = left_type;
    convert(t);
    if (op == '/' || op == '%') {
        if (u)
            sequence(0xd231, 2);
        else {
            if (wide)
                emit(0x48);
            emit(0x99);
        }
        if (wide)
            emit(0x48);
        if (u)
            sequence(0xf1f7, 2);
        else
            sequence(0xf9f7, 2);
        if (op == '%') {
            if (wide)
                emit(0x48);
            sequence(0xd089, 2);
        }
    } else if (op == 128)
        comparison(0x94);
    else if (op == 129)
        comparison(0x95);
    else if (op == '<') {
        if (u)
            comparison(0x92);
        else
            comparison(0x9c);
    } else if (op == '>') {
        if (u)
            comparison(0x97);
        else
            comparison(0x9f);
    } else if (op == 130) {
        if (u)
            comparison(0x96);
        else
            comparison(0x9e);
    } else if (op == 131) {
        if (u)
            comparison(0x93);
        else
            comparison(0x9d);
    } else {
        if (wide)
            emit(0x48);
        if (op == '+')
            sequence(0xc801, 2);
        else if (op == '-')
            sequence(0xc829, 2);
        else if (op == '*')
            sequence(0xc1af0f, 3);
        else if (op == '&')
            sequence(0xc821, 2);
        else if (op == '|')
            sequence(0xc809, 2);
        else if (op == '^')
            sequence(0xc831, 2);
        else if (op == 132)
            sequence(0xe0d3, 2);
        else if (op == 133) {
            if (u)
                sequence(0xe8d3, 2);
            else
                sequence(0xf8d3, 2);
        } else
            error();
    }
    return 0;
}

int expression(int minimum) {
    int left, op, prec, end, alternate, left_type, middle_type, left_zero, middle_zero;
    int right_type, result_type, join;
    if (failed)
        return 0;
    expression_depth = expression_depth + 1;
    if (expression_depth > 128)
        return error();
    left = unary();
    while (!failed && precedence(token) >= minimum) {
        op = token;
        prec = precedence(op);
        left_type = expression_type;
        left_zero = expression_zero;
        if (prec == 1) {
            if (left == 0 || expression_array_type)
                return error();
            if (left == -1)
                emit(0x52);
            if (op != '=')
                emit(0x50);
            next();
            expression(1);
            if (op != '=')
                binary(compound(op), left_type, expression_type, left_zero);
            convert(left_type);
            store(left, left_type);
            expression_zero = 0;
        } else if (op == '?') {
            boolean(0x95);
            alternate = jump(0x84);
            next();
            full_expression();
            expect(':');
            middle_type = expression_type;
            middle_zero = expression_zero;
            end = jump(0);
            patch(alternate, position);
            expression(2);
            right_type = expression_type;
            if (middle_type >= 16 && expression_type > 0 && expression_type < 16 && expression_zero)
                expression_type = middle_type;
            else if (expression_type >= 16 && middle_type > 0 && middle_type < 16 && middle_zero)
                expression_type = expression_type;
            else if (middle_type || expression_type)
                expression_type = common_type(middle_type, expression_type);
            result_type = expression_type;
            expression_type = right_type;
            convert(result_type);
            join = jump(0);
            patch(end, position);
            expression_type = middle_type;
            expression_zero = middle_zero;
            convert(result_type);
            patch(join, position);
            expression_zero = 0;
        } else if (op == 134 || op == 135) {
            boolean(0x95);
            if (op == 134)
                end = jump(0x84);
            else
                end = jump(0x85);
            next();
            expression(prec + 1);
            boolean(0x95);
            patch(end, position);
        } else {
            emit(0x50);
            next();
            expression(prec + 1);
            binary(op, left_type, expression_type, left_zero);
        }
        left = 0;
        expression_array_bytes = 0;
        expression_array_type = 0;
    }
    expression_depth = expression_depth - 1;
    return left;
}

/* Commas here sequence full expressions; argument and initializer parsers
   continue to call expression(1), where comma is a delimiter. */
int full_expression(void) {
    int left = expression(1);
    while (!failed && token == ',') {
        next();
        expression(1);
        left = 0;
        expression_zero = 0;
        expression_array_bytes = 0;
        expression_array_type = 0;
    }
    return left;
}

int increment(int left, int op, int postfix) {
    int t = expression_type, step = 1;
    if (left == 0 || expression_array_type)
        return error();
    if (left == -1)
        emit(0x52);
    if (t >= 16)
        step = type_size(t - 16);
    if (postfix) {
        if (wide_type(t))
            sequence(0xc08949, 3);
        else
            sequence(0xc08941, 3);
    }
    if (wide_type(t))
        emit(0x48);
    if (op == 170)
        sequence(0xc081, 2);
    else
        sequence(0xe881, 2);
    emit32(step);
    convert(t);
    store(left, t);
    if (postfix) {
        if (wide_type(t))
            sequence(0xc0894c, 3);
        else
            sequence(0xc08944, 3);
    }
    expression_zero = 0;
    expression_array_bytes = 0;
    expression_array_type = 0;
    return 0;
}

int unary(void) {
    int op = token;
    int result, t, saved_position, saved_calls, bytes;
    if (failed)
        return 0;
    unary_depth = unary_depth + 1;
    if (unary_depth > 128)
        return error();
    if (op == 283) {
        saved_position = position;
        saved_calls = call_count;
        next();
        if (token == '(') {
            next();
            if (type_start(token)) {
                t = declarator(specifiers(), 1);
                expression_array_bytes = 0;
                expect(')');
            } else {
                full_expression();
                t = expression_type;
                expect(')');
            }
        } else {
            unary();
            t = expression_type;
        }
        bytes = expression_array_bytes;
        if (!bytes)
            bytes = type_size(t);
        position = saved_position;
        call_count = saved_calls;
        constant(bytes);
        expression_type = 11;
        expression_array_bytes = 0;
        expression_array_type = 0;
        result = 0;
    } else if (op == '&') {
        next();
        result = unary();
        if (!result)
            return error();
        position = expression_load;
        if (result == -1)
            sequence(0xd08948, 3);
        else if (result < -1) {
            sequence(0x058d48, 3);
            relative(global_address[-result - 2]);
        } else {
            sequence(0x858d48, 3);
            emit32(-result);
        }
        if (expression_array_type)
            expression_type = expression_array_type;
        expression_type = make_pointer(expression_type);
        expression_zero = 0;
        result = 0;
        expression_array_bytes = 0;
        expression_array_type = 0;
    } else if (op == '*') {
        next();
        unary();
        if (expression_type < 17)
            return error();
        t = expression_type - 16;
        result = indirect_value(t);
    } else if (op == 170 || op == 171) {
        next();
        result = unary();
        increment(result, op, 0);
        result = 0;
    } else if (op == '+' || op == '-' || op == '!' || op == '~') {
        next();
        unary();
        if (!expression_type)
            return error();
        if (expression_type >= 16 && op != '!')
            return error();
        expression_type = promoted(expression_type);
        if (op == '-') {
            if (wide_type(expression_type))
                emit(0x48);
            sequence(0xd8f7, 2);
        }
        if (op == '!')
            boolean(0x94);
        if (op == '~') {
            if (wide_type(expression_type))
                emit(0x48);
            sequence(0xd0f7, 2);
        }
        expression_zero = 0;
        expression_array_bytes = 0;
        expression_array_type = 0;
        result = 0;
    } else {
        result = primary();
        while (!failed && (token == 170 || token == 171 || token == '[')) {
            if (token == '[') {
                t = expression_type;
                emit(0x50);
                next();
                full_expression();
                expect(']');
                binary('+', t, expression_type, 0);
                if (expression_type < 17)
                    return error();
                t = expression_type - 16;
                result = indirect_value(t);
            } else {
                op = token;
                next();
                increment(result, op, 1);
                result = 0;
            }
        }
    }
    unary_depth = unary_depth - 1;
    return result;
}

int primary(void) {
    int id, result, t, n;
    if (failed)
        return 0;
    expression_array_bytes = 0;
    expression_array_type = 0;
    if (token == 145) {
        join_strings();
        return emit_string();
    }
    if (token == 144) {
        if (wide_type(literal_type)) {
            sequence(0xb848, 2);
            emit32(value);
            emit32(literal_high);
        } else
            constant(value);
        expression_type = literal_type;
        expression_zero = value == 0 && literal_high == 0;
        next();
        return 0;
    }
    if (token == '(') {
        next();
        if (type_start(token)) {
            t = declarator(specifiers(), 1);
            if (is_array(t))
                return error();
            expect(')');
            unary();
            cast_to(t);
            expression_array_bytes = 0;
            expression_array_type = 0;
            return 0;
        }
        result = full_expression();
        expect(')');
        return result;
    }
    id = name();
    next();
    if (token == '(')
        return arguments(id);
    return load(id);
}

int arguments(int id) {
    int n = 0, i, saved_stack;
    if (local_offset[id] != 0 || global_address[id] != 0)
        return error();
    expect('(');
    while (!failed && token != ')') {
        if (n == 127)
            return error();
        expression(1);
        if ((symbol_flags[id] & 4) != 0)
            convert(parameter_type[id * 128 + n]);
        else {
            if (!expression_type)
                return error();
            parameter_type[id * 128 + n] = promoted(expression_type);
        }
        emit(0x50);
        n = n + 1;
        if (token != ',')
            break;
        next();
        if (token == ')')
            return error();
    }
    expect(')');
    if ((symbol_flags[id] & 4) != 0 && arity[id] != n)
        return error();
    arity[id] = n;
    if ((symbol_flags[id] & 4) == 0)
        global_type[id] = 1;
    symbol_flags[id] = symbol_flags[id] | 6;
    if (n > 4) {
        /* R11 addresses the evaluated arguments in reverse stack order.
           A separate aligned area contains shadow space and ABI stack slots. */
        sequence(0xe38949, 3);
        sequence(0xe48348, 3);
        emit(0xf0);
        saved_stack = n * 8;
        sequence(0xec8148, 3);
        emit32(((saved_stack + 8 + 15) / 16) * 16);
        sequence(0x838d49, 3);
        emit32(n * 8);
        sequence(0x24848948, 4);
        emit32(saved_stack);
        sequence(0x8b8b49, 3);
        emit32((n - 1) * 8);
        sequence(0x938b49, 3);
        emit32((n - 2) * 8);
        sequence(0x838b4d, 3);
        emit32((n - 3) * 8);
        sequence(0x8b8b4d, 3);
        emit32((n - 4) * 8);
        for (i = 4; i < n; i = i + 1) {
            sequence(0x838b49, 3);
            emit32((n - i - 1) * 8);
            sequence(0x24848948, 4);
            emit32(i * 8);
        }
        call(id);
        sequence(0x24a48b48, 4);
        emit32(saved_stack);
    } else {
        if (n == 4)
            sequence(0x5941, 2);
        if (n >= 3)
            sequence(0x5841, 2);
        if (n >= 2)
            emit(0x5a);
        if (n >= 1)
            emit(0x59);
        sequence(0xe38949, 3);
        sequence(0xe48348, 3);
        emit(0xf0);
        sequence(0x30ec8348, 4);
        sequence(0x245c894c, 4);
        emit(0x20);
        call(id);
        sequence(0x24648b48, 4);
        emit(0x20);
    }
    expression_type = global_type[id];
    expression_array_bytes = 0;
    expression_array_type = 0;
    expression_zero = 0;
    convert(expression_type);
    return 0;
}

int local(int t, int id) {
    int bytes;
    if (failed)
        return 0;
    if (!t)
        return error();
    if (local_offset[id] != 0 && local_depth[id] == block_depth)
        return error();
    if (local_count == 256)
        return error();
    bytes = type_size(t);
    if (failed)
        return 0;
    binding_name[local_count] = id;
    binding_offset[local_count] = local_offset[id];
    binding_depth[local_count] = local_depth[id];
    binding_type[local_count] = local_type[id];
    binding_bytes[local_count] = local_bytes;
    local_bytes = local_bytes + ((bytes + 7) / 8) * 8;
    if (local_bytes > 65536)
        return error();
    if (local_bytes > frame_peak)
        frame_peak = local_bytes;
    local_count = local_count + 1;
    local_offset[id] = local_bytes;
    local_depth[id] = block_depth;
    local_type[id] = t;
    return local_offset[id];
}

int declaration(void) {
    int offset, base = specifiers(), t, id, inferred;
    while (!failed) {
        t = declarator(base, 0);
        id = decl_name;
        inferred = is_array(t) && array_length[(t - 4096) / 256] == 0;
        if (inferred)
            t = infer_string(t);
        offset = local(t, id);
        if (inferred) {
            zero_local(offset, type_size(t));
            copy_string(t, offset, 0, 0);
        } else if (token == '=') {
            next();
            if (is_array(t))
                zero_local(offset, type_size(t));
            initialize_object(t, offset, 0, 0);
        }
        if (token != ',')
            break;
        next();
    }
    return expect(';');
}

int finish_breaks(int head, int target) {
    int previous;
    while (head != 0 && !failed) {
        previous = read32(head);
        patch(head, target);
        head = previous;
    }
    return 0;
}

int restore_locals(int saved) {
    int id;
    while (local_count > saved) {
        local_count = local_count - 1;
        id = binding_name[local_count];
        local_offset[id] = binding_offset[local_count];
        local_depth[id] = binding_depth[local_count];
        local_type[id] = binding_type[local_count];
        local_bytes = binding_bytes[local_count];
    }
    return 0;
}

int statement(void) {
    if (failed)
        return 0;
    statement_depth = statement_depth + 1;
    if (statement_depth > 128)
        return error();
    statement_body();
    statement_depth = statement_depth - 1;
    return 0;
}

int scalar_test(void) {
    if (!expression_type)
        return error();
    if (wide_type(expression_type))
        emit(0x48);
    return sequence(0xc085, 2);
}

int statement_body(void) {
    int target, end, start, old_break, old_continue;
    int condition, increment, body, saved_locals, for_scope = 0;
    if (label_ahead()) {
        target = name();
        next();
        expect(':');
        if (label_address[target])
            return error();
        label_address[target] = position;
        finish_breaks(label_head[target], position);
        label_head[target] = 0;
        return statement();
    }
    if (token == 278) {
        next();
        start = name();
        next();
        expect(';');
        if (failed)
            return 0;
        target = jump(0);
        if (label_address[start])
            patch(target, label_address[start]);
        else {
            write32(target, label_head[start]);
            label_head[start] = target;
        }
        return 0;
    }
    if (token == '{') {
        saved_locals = local_count;
        block_depth = block_depth + 1;
        next();
        while (!failed && token != '}') {
            if (token == 0)
                return error();
            if (type_start(token))
                declaration();
            else
                statement();
        }
        expect('}');
        restore_locals(saved_locals);
        block_depth = block_depth - 1;
        return 0;
    }
    if (type_start(token))
        return error();
    if (token == 257) {
        next();
        if (token == ';') {
            if (return_type)
                return error();
            next();
            return epilogue();
        }
        if (!return_type)
            return error();
        full_expression();
        convert(return_type);
        expect(';');
        return epilogue();
    }
    if (token == 258) {
        next();
        expect('(');
        full_expression();
        expect(')');
        scalar_test();
        target = jump(0x84);
        statement();
        end = jump(0);
        patch(target, position);
        if (token == 259) {
            next();
            statement();
        }
        patch(end, position);
        return 0;
    }
    if (token == 273) {
        old_break = break_head;
        old_continue = continue_target;
        break_head = 0;
        loop_depth = loop_depth + 1;
        /* First entry skips the continue trampoline. Its forward jump is
           patched to the condition after the body has been generated. */
        body = jump(0);
        continue_target = position;
        condition = jump(0);
        patch(body, position);
        start = position;
        next();
        statement();
        expect(260);
        expect('(');
        patch(condition, position);
        full_expression();
        expect(')');
        expect(';');
        scalar_test();
        target = jump(0x85);
        patch(target, start);
        finish_breaks(break_head, position);
        break_head = old_break;
        continue_target = old_continue;
        loop_depth = loop_depth - 1;
        return 0;
    }
    if (token == 260 || token == 265) {
        old_break = break_head;
        old_continue = continue_target;
        break_head = 0;
        loop_depth = loop_depth + 1;
        if (token == 260) {
            start = position;
            continue_target = start;
            next();
            expect('(');
            full_expression();
            expect(')');
            scalar_test();
            end = jump(0x84);
            statement();
            target = jump(0);
            patch(target, start);
        } else {
            saved_locals = local_count;
            block_depth = block_depth + 1;
            for_scope = 1;
            next();
            expect('(');
            if (type_start(token))
                declaration();
            else {
                if (token != ';')
                    full_expression();
                expect(';');
            }
            condition = position;
            end = 0;
            if (token != ';') {
                full_expression();
                scalar_test();
                end = jump(0x84);
            }
            expect(';');
            body = jump(0);
            increment = position;
            continue_target = increment;
            if (token != ')')
                full_expression();
            expect(')');
            target = jump(0);
            patch(target, condition);
            patch(body, position);
            statement();
            target = jump(0);
            patch(target, increment);
        }
        if (end != 0)
            patch(end, position);
        finish_breaks(break_head, position);
        break_head = old_break;
        continue_target = old_continue;
        if (for_scope) {
            restore_locals(saved_locals);
            block_depth = block_depth - 1;
        }
        loop_depth = loop_depth - 1;
        return 0;
    }
    if (token == 263 || token == 264) {
        if (loop_depth == 0)
            return error();
        target = jump(0);
        if (token == 263) {
            write32(target, break_head);
            break_head = target;
        } else
            patch(target, continue_target);
        next();
        return expect(';');
    }
    if (token == ';') {
        next();
        return 0;
    }
    full_expression();
    return expect(';');
}

int initializer_address(int base, int offset, int global) {
    if (global) {
        sequence(0x0d8d48, 3);
        relative(base + offset);
    } else {
        sequence(0x8d8d48, 3);
        emit32(offset - base);
    }
    emit(0x51);
    return 0;
}

int join_strings(void) {
    joined_string_start = literal_string_start;
    joined_string_length = 0;
    if (token != 145)
        return error();
    while (!failed && token == 145) {
        joined_string_length = joined_string_length + literal_string_length;
        next();
    }
    return 0;
}

int emit_string(void) {
    int skip = jump(0), data = position, i;
    for (i = 0; i < joined_string_length && !failed; i = i + 1)
        emit(string_pool[joined_string_start + i]);
    emit(0);
    patch(skip, position);
    sequence(0x058d48, 3);
    emit32(data - position - 4);
    sequence(0xc28948, 3);
    expression_load = position;
    array_value(make_array(9, joined_string_length + 1));
    return -1;
}

int character_array(int t) {
    if (!is_array(t))
        return 0;
    t = array_base[(t - 4096) / 256];
    return t == 5 || t == 6 || t == 9;
}

int infer_string(int t) {
    int braces = 0;
    if (!character_array(t))
        return error();
    expect('=');
    if (token == '{') {
        braces = 1;
        next();
    }
    join_strings();
    if (braces) {
        if (token == ',')
            next();
        expect('}');
    }
    return make_array(array_base[(t - 4096) / 256], joined_string_length + 1);
}

int copy_string(int t, int base, int offset, int global) {
    int n = array_length[(t - 4096) / 256], bytes = joined_string_length + 1;
    if (joined_string_length > n)
        return error();
    if (bytes > n)
        bytes = n;
    emit_string();
    emit(0x56);
    emit(0x57);
    sequence(0xc68948, 3);
    if (global) {
        sequence(0x3d8d48, 3);
        relative(base + offset);
    } else {
        sequence(0xbd8d48, 3);
        emit32(offset - base);
    }
    emit(0xb9);
    emit32(bytes);
    emit(0xfc);
    sequence(0xa4f3, 2);
    emit(0x5f);
    emit(0x5e);
    return 0;
}

int zero_local(int offset, int bytes) {
    emit(0x57);
    sequence(0xbd8d48, 3);
    emit32(-offset);
    sequence(0xc031, 2);
    emit(0xb9);
    emit32(bytes);
    emit(0xfc);
    sequence(0xaaf3, 2);
    emit(0x5f);
    return 0;
}

/* Static scalar values currently accept signed numeric literals and direct
   object addresses/array designators. General constant expressions follow. */
int static_value(void) {
    int negative = 0, id, address = 0, sign = 0;
    if (token == 145) {
        join_strings();
        emit_string();
        return 0;
    }
    if (token == '+' || token == '-') {
        sign = 1;
        negative = token == '-';
        next();
    }
    if (token == 144) {
        primary();
        if (negative) {
            if (wide_type(expression_type))
                emit(0x48);
            sequence(0xd8f7, 2);
        }
        return 0;
    }
    if (sign)
        return error();
    if (token == '&') {
        address = 1;
        next();
    }
    id = name();
    next();
    if (!global_address[id])
        return error();
    if (!address && !is_array(global_type[id]))
        return error();
    sequence(0x058d48, 3);
    relative(global_address[id]);
    if (address)
        expression_type = make_pointer(global_type[id]);
    else
        expression_type = make_pointer(array_base[(global_type[id] - 4096) / 256]);
    expression_zero = 0;
    expression_array_bytes = 0;
    expression_array_type = 0;
    return 0;
}

int initialize_object(int t, int base, int offset, int global) {
    int braces = 0, i, n, element, width;
    if (failed)
        return 0;
    initializer_depth = initializer_depth + 1;
    if (initializer_depth > 64)
        return error();
    if (token == '{') {
        braces = 1;
        next();
        if (token == '}')
            return error();
    }
    if (character_array(t) && token == 145) {
        join_strings();
        copy_string(t, base, offset, global);
    } else if (is_array(t)) {
        if (!braces && initializer_depth == 1)
            return error();
        i = (t - 4096) / 256;
        n = array_length[i];
        element = array_base[i];
        width = type_size(element);
        for (i = 0; i < n && !failed; i = i + 1) {
            if (token == '}')
                break;
            initialize_object(element, base, offset + i * width, global);
            if (i + 1 < n && token == ',')
                next();
            else
                break;
        }
    } else if (token == '{') {
        initialize_object(t, base, offset, global);
    } else {
        initializer_address(base, offset, global);
        if (global)
            static_value();
        else
            expression(1);
        convert(t);
        store(-1, t);
    }
    if (braces) {
        if (token == ',')
            next();
        expect('}');
    }
    initializer_depth = initializer_depth - 1;
    return 0;
}

int global(int id, int t, int base) {
    int width, align, element, inferred;
    while (!failed) {
        if (symbol_flags[id] != 0)
            return error();
        inferred = is_array(t) && array_length[(t - 4096) / 256] == 0;
        if (inferred)
            t = infer_string(t);
        width = type_size(t);
        if (failed)
            return 0;
        element = t;
        while (is_array(element))
            element = array_base[(element - 4096) / 256];
        align = type_size(element);
        allocation = ((allocation + align - 1) / align) * align;
        if (allocation + width > 0xc00000)
            return error();
        global_address[id] = allocation;
        allocation = allocation + width;
        global_type[id] = t;
        global_extent[id] = width;
        symbol_flags[id] = 8;
        if (inferred || token == '=') {
            if (initializer_tail)
                patch(initializer_tail, position);
            else
                initializer_start = position;
            if (inferred)
                copy_string(t, global_address[id], 0, 1);
            else {
                next();
                initialize_object(t, global_address[id], 0, 1);
            }
            initializer_tail = jump(0);
        }
        if (token != ',')
            break;
        next();
        t = declarator(base, 0);
        id = decl_name;
    }
    return expect(';');
}

int parameter(int base, int function_id, int known) {
    int pt = declarator(base, 2), i;
    if (is_array(pt))
        pt = make_pointer(array_base[(pt - 4096) / 256]);
    if (local_count == 127 || !pt)
        return error();
    i = function_id * 128 + local_count;
    if (known && parameter_type[i] != pt)
        return error();
    parameter_type[i] = pt;
    if (decl_name == -1) {
        local_count = local_count + 1;
        local_bytes = local_bytes + 8;
        return 1;
    }
    local(pt, decl_name);
    return 0;
}

int function(void) {
    int id, i, n, t, base, known, unnamed = 0;
    base = specifiers();
    t = declarator(base, 0);
    id = decl_name;
    if (failed)
        return 0;
    if (token != '(')
        return global(id, t, base);
    /* Function-pointer suffixes need the next function-type stage. */
    if (is_array(t) || (decl_parentheses && t != base))
        return error();
    if (global_address[id] != 0)
        return error();
    known = (symbol_flags[id] & 4) != 0;
    if (known && global_type[id] != t)
        return error();
    global_type[id] = t;
    for (i = 0; i < count; i = i + 1) {
        local_offset[i] = 0;
        local_depth[i] = 0;
        local_type[i] = 0;
    }
    local_count = 0;
    local_bytes = 0;
    frame_peak = 0;
    block_depth = 1;
    next();
    if (token == 261) {
        next();
        if (token != ')') {
            unnamed = unnamed | parameter(0, id, known);
            if (token == ',') {
                next();
                if (token == ')')
                    return error();
            } else if (token != ')')
                return error();
        }
    }
    while (!failed && token != ')') {
        unnamed = unnamed | parameter(specifiers(), id, known);
        if (token != ',')
            break;
        next();
        if (token == ')')
            return error();
    }
    expect(')');
    n = local_count;
    if (id == 6 && n != 0)
        return error();
    if (known && arity[id] != n)
        return error();
    symbol_flags[id] = symbol_flags[id] | 4;
    arity[id] = n;
    if (token == ';') {
        next();
        return 0;
    }
    if (unnamed || (symbol_flags[id] & 1) != 0)
        return error();
    symbol_flags[id] = symbol_flags[id] | 1;
    function_address[id] = position;
    for (i = 0; i < 1024; i = i + 1) {
        label_address[i] = 0;
        label_head[i] = 0;
    }
    prologue();
    if (n >= 1) {
        emit(0x48);
        sequence(0xf84d89, 3);
    }
    if (n >= 2) {
        emit(0x48);
        sequence(0xf05589, 3);
    }
    if (n >= 3) {
        sequence(0x45894c, 3);
        emit(0xe8);
    }
    if (n >= 4) {
        sequence(0x4d894c, 3);
        emit(0xe0);
    }
    for (i = 4; i < n; i = i + 1) {
        sequence(0x858b48, 3);
        emit32(16 + i * 8);
        sequence(0x858948, 3);
        emit32(-(i + 1) * 8);
    }
    if (token != '{')
        return error();
    return_type = t;
    block_depth = 0;
    statement();
    for (i = 0; i < 1024; i = i + 1) {
        if (label_head[i])
            return error();
    }
    write32(frame_patch, ((frame_peak + 15) / 16) * 16);
    sequence(0xc031, 2);
    epilogue();
    return 0;
}

int pe_header(void) {
    int i;
    write32(0, 0x5a4d);
    write32(0x3c, 0x80);
    write32(0x80, 0x4550);
    write32(0x84, 0x28664);
    write32(0x94, 0x2300f0);
    write32(0x98, 0x20b);
    write32(0xa4, 0x701000);
    write32(0xa8, 0x1200);
    write32(0xac, 0x1000);
    write32(0xb0, 0x400000);
    write32(0xb8, 0x1000);
    write32(0xbc, 0x200);
    write32(0xc0, 6);
    write32(0xc8, 6);
    write32(0xd0, 0x801000);
    write32(0xd4, 0x200);
    write32(0xdc, 0x1000003);
    write32(0xe0, 0x100000);
    write32(0xe8, 0x1000);
    write32(0xf0, 0x100000);
    write32(0xf8, 0x1000);
    write32(0x104, 16);
    write32(0x110, 0x1000);
    write32(0x114, 40);
    write32(0x168, 0x1040);
    write32(0x16c, 40);
    write32(0x188, 0x7865742e);
    write32(0x18c, 0x74);
    write32(0x190, 0xff000);
    write32(0x194, 0x1000);
    write32(0x19c, 0x200);
    write32(0x1ac, -536870880);
    write32(0x1b0, 0x7373622e);
    write32(0x1b8, 0x701000);
    write32(0x1bc, 0x100000);
    write32(0x1d4, -1073741696);
    write32(0x200, 0x1080);
    write32(0x20c, 0x10c0);
    write32(0x210, 0x1040);
    for (i = 0; i < 4; i = i + 1) {
        write32(0x240 + i * 8, 0x10e0 + i * 16);
        write32(0x280 + i * 8, 0x10e0 + i * 16);
    }
    write32(0x2c0, 0x4e52454b);
    write32(0x2c4, 0x32334c45);
    write32(0x2c8, 0x6c6c642e);
    write32(0x2e2, 0x53746547);
    write32(0x2e6, 0x61486474);
    write32(0x2ea, 0x656c646e);
    write32(0x2f2, 0x64616552);
    write32(0x2f6, 0x656c6946);
    write32(0x302, 0x74697257);
    write32(0x306, 0x6c694665);
    write32(0x30a, 0x65);
    write32(0x312, 0x74697845);
    write32(0x316, 0x636f7250);
    write32(0x31a, 0x737365);
    position = 0x400;
    sequence(0x28ec8348, 4);
    emit(0xe8);
    initializer_call = position;
    emit32(0);
    call(6);
    sequence(0xc189, 2);
    indirect(0x401058);
    emit(0xcc);
    return 0;
}

int runtime_function(int id) {
    int first, second;
    function_address[id] = position;
    sequence(0x38ec8348, 4);
    if (id == 11) {
        sequence(0x30244c89, 4);
        sequence(0x0d88, 2);
        relative(0xc00000);
        emit(0xb9);
        emit32(-11);
    } else {
        emit(0xb9);
        emit32(-10);
    }
    indirect(0x401040);
    sequence(0xc18948, 3);
    sequence(0x158d48, 3);
    relative(0xc00000);
    sequence(0xb841, 2);
    emit32(1);
    sequence(0x0d8d4c, 3);
    relative(0xc00004);
    sequence(0x2444c748, 4);
    emit(0x20);
    emit32(0);
    if (id == 11)
        indirect(0x401050);
    else
        indirect(0x401048);
    sequence(0xc085, 2);
    first = jump(0x84);
    sequence(0x058b, 2);
    relative(0xc00004);
    sequence(0x01f883, 3);
    second = jump(0x85);
    if (id == 11) {
        sequence(0x3024448b, 4);
        emit(0x25);
        emit32(255);
    } else {
        sequence(0x05b60f, 3);
        relative(0xc00000);
    }
    sequence(0x38c48348, 4);
    emit(0xc3);
    patch(first, position);
    patch(second, position);
    constant(-1);
    sequence(0x38c48348, 4);
    emit(0xc3);
    return 0;
}

int main(void) {
    int c, i, total, id;
    initialize();
    allocation = 0x500000;
    symbol_flags[6] = 6;
    arity[6] = 0;
    global_type[6] = 1;
    symbol_flags[10] = 5;
    symbol_flags[11] = 5;
    arity[11] = 1;
    global_type[10] = 1;
    global_type[11] = 1;
    parameter_type[1408] = 1;
    while ((c = __c0_getchar()) != -1) {
        if (size == 262144)
            return 1;
        source[size] = c;
        size = size + 1;
    }
    source_phases();
    pe_header();
    next();
    while (token != 0 && !failed)
        function();
    if ((symbol_flags[6] & 1) == 0)
        error();
    if (initializer_tail)
        patch(initializer_tail, position);
    if (!initializer_start)
        initializer_start = position;
    emit(0xc3);
    patch(initializer_call, initializer_start);
    runtime_function(10);
    runtime_function(11);
    for (i = 0; i < call_count && !failed; i = i + 1) {
        id = call_symbol[i];
        if ((symbol_flags[id] & 1) == 0)
            error();
        else
            patch(call_position[i], function_address[id]);
    }
    if (failed)
        return 1;
    total = ((position + 511) / 512) * 512;
    write32(0x9c, total - 512);
    write32(0x198, total - 512);
    for (i = 0; i < total; i = i + 1) {
        if (__c0_putchar(image[i]) == -1)
            return 1;
    }
    return 0;
}

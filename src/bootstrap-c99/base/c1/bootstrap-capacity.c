/* Frozen capacity bridge: 512 KiB input, 512 KiB code buffer; larger target layout.
   Reads C from stdin and directly emits a Windows x64 PE image on stdout. */
int source[524288];
int names[65536];
int lengths[1024];
int size;
int cursor;
int count;
int failed;
int value;
int literal_type;
int literal_high;
int string_pool[65536];
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
        if (string_used == 65536) {
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
        if (c == '-' && d == '>') {
            take();
            return 173;
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
int image[524800];
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
int alias_type[1024];
int binding_alias[256];
int decl_typedef;
int decl_storage;
int register_object[1024];
int binding_register[256];
int local_static_address[1024];
int binding_static_address[256];
int unevaluated_depth;
int for_declaration;
int local_bytes;
int frame_peak;
int frame_patch;
int initializer_depth;
int initializer_pending;
int initializer_probe;
int inferred_elements;
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
int decl_result_name;
int declarator_top_function;
int definition_ready;
int definition_old;
int definition_parameter_count;
int definition_parameter_name[128];
int definition_parameter_type[128];
int function_type_count;
int function_result[1024];
int function_arity[1024];
int function_parameter[131072];
int decl_depth;
int decl_parentheses;
int declarator_active;
int global_type[1024];
int file_function_type[1024];
int parameter_type[131072];
int expression_type;
int sizeof_value;
int expression_zero;
int expression_load;
int expression_array_bytes;
int expression_array_type;
int global_extent[1024];
int return_type;
int global_address[1024];
int global_initialized[1024];
int global_tentative[1024];
int object_ref_position[16384];
int object_ref_symbol[16384];
int object_ref_count;
int symbol_flags[1024];
int global_visible[1024];
int entity_type[1024];
int enum_count;
int enum_complete[1024];
int enum_global[1024];
int enum_global_value[1024];
int tag_type[1024];
int tag_depth[1024];
int tag_count;
int tag_binding_name[1024];
int tag_binding_type[1024];
int tag_binding_depth[1024];
int parameter_count;
int parameter_offset[128];
int parameter_decl_type[128];
int return_address_offset;
int record_count;
int record_depth;
int record_size[1024];
int record_align[1024];
int record_kind[1024];
int record_open[1024];
int record_first[1024];
int record_last[1024];
int member_count;
int member_name[8192];
int member_type[8192];
int member_offset[8192];
int member_next[8192];
int expression_register;
int arity[1024];
int call_arity[16384];
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
int switch_depth;
int switch_type;
int switch_head;
int switch_default;
int case_count;
int case_next[4096];
int case_address[4096];
unsigned long long case_value[4096];

int error(void) {
    failed = 1;
    token = 0;
    return 0;
}

int emit(int b) {
    if (failed)
        return 0;
    if (position >= 524800)
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
    if (p < 0 || p > 524796)
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

int function_exists(int id) {
    return local_offset[id] == -4 ||
           (!local_offset[id] && !alias_type[id] && file_function_type[id]);
}

int function_signature(int id) {
    if (local_offset[id] == -4)
        return local_type[id];
    return file_function_type[id];
}

int function_entity_signature(int id) {
    return make_function(global_type[id], (symbol_flags[id] & 4) ? arity[id] : -1,
                         &parameter_type[id * 128]);
}

int function_reference(int id) {
    if (call_count == 16384)
        return error();
    call_position[call_count] = position;
    call_symbol[call_count] = id;
    call_arity[call_count] = -1;
    call_count++;
    return emit32(0);
}

int function_value(int t) {
    sequence(0xc28948, 3);
    expression_load = position;
    expression_type = make_pointer(t);
    expression_array_type = t;
    expression_array_bytes = 0;
    expression_zero = 0;
    expression_register = 0;
    return -1;
}

int invoke(int id, int target, int n) {
    if (id >= 0) {
        call(id);
        call_arity[call_count - 1] = n;
    } else {
        sequence(0x858b48, 3);
        emit32(-target);
        sequence(0xd0ff, 2);
    }
    return 0;
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
    return t == 256 || t == 261 || t == 270 || t == 279 || t == 281 || t == 282 || t == 289 ||
           t == 293 || t == 287 || t == 268 || t == 280 || t == 284 || t == 276 || t == 275 ||
           t == 285 || t == 288 || (t >= 256 && alias_type[t - 256]);
}

int specifiers(void) {
    int i = 0, ch = 0, sh = 0, lo = 0, si = 0, un = 0, vo = 0, bo = 0, n = 0, t, alias = 0,
        saved_storage;
    decl_typedef = 0;
    decl_storage = 0;
    while (type_start(token) && !failed) {
        if (token == 287 || token == 268 || token == 280 || token == 284 || token == 276) {
            if (decl_storage)
                return error();
            if (token == 287) {
                decl_storage = 1;
                decl_typedef = 1;
            }
            if (token == 268)
                decl_storage = 2;
            if (token == 280)
                decl_storage = 3;
            if (token == 284)
                decl_storage = 4;
            if (token == 276)
                decl_storage = 5;
            next();
            continue;
        }
        if (token == 275 || token == 285 || token == 288) {
            if (n)
                return error();
            saved_storage = decl_storage;
            if (token == 275)
                alias = enum_specifier() + 1;
            else
                alias = record_specifier() + 1;
            decl_storage = saved_storage;
            decl_typedef = decl_storage == 1;
            n++;
            continue;
        }
        if (token >= 256 && alias_type[token - 256]) {
            if (n)
                break;
            alias = alias_type[token - 256];
            next();
            n++;
            continue;
        }
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
        if (t == 293)
            bo = bo + 1;
    }
    if (alias) {
        if (n != 1)
            return error();
        return alias - 1;
    }
    if (bo) {
        if (n != 1)
            return error();
        return 12;
    }
    if (!n && decl_storage)
        return 1;
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

int is_enum(int t) {
    return t >= 0x100000 && t % 256 == 13;
}

int is_record(int t) {
    return t >= 0x200000 && t % 256 == 14;
}

int is_function(int t) {
    return t >= 0x300000 && t % 256 == 15;
}

int function_pointer(int t) {
    return is_pointer(t) && is_function(t - 16);
}

int is_pointer(int t) {
    return t >= 16 && !is_array(t) && !is_enum(t) && !is_record(t) && !is_function(t);
}

int wide_type(int t) {
    return t == 10 || t == 11 || (is_pointer(t) && !is_array(t));
}

int type_size(int t) {
    if (is_function(t))
        return error();
    if (is_record(t)) {
        t = record_size[(t - 0x20000e) / 256];
        if (!t)
            return error();
        return t;
    }
    if (is_enum(t)) {
        if (!enum_complete[(t - 0x10000d) / 256])
            return error();
        return 4;
    }
    if (is_array(t)) {
        t = array_bytes[(t - 4096) / 256];
        if (!t)
            return error();
        return t;
    }
    if (wide_type(t))
        return 8;
    if (t == 5 || t == 6 || t == 9 || t == 12)
        return 1;
    if (t == 7 || t == 8)
        return 2;
    if (t >= 1 && t <= 4)
        return 4;
    return error();
}

int type_align(int t) {
    while (is_array(t))
        t = array_base[(t - 4096) / 256];
    if (is_record(t)) {
        type_size(t);
        return record_align[(t - 0x20000e) / 256];
    }
    return type_size(t);
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
    if (n > 0x1a00000 / bytes)
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

int make_function(int result, int n, int *parameters) {
    int i, j, same;
    if (is_array(result) || is_function(result))
        return error();
    for (i = 0; i < function_type_count; i++) {
        same = function_result[i] == result && function_arity[i] == n;
        for (j = 0; j < n && same; j++)
            if (function_parameter[i * 128 + j] != parameters[j])
                same = 0;
        if (same)
            return 0x30000f + i * 256;
    }
    if (function_type_count == 1024)
        return error();
    i = function_type_count++;
    function_result[i] = result;
    function_arity[i] = n;
    for (j = 0; j < n; j++)
        function_parameter[i * 128 + j] = parameters[j];
    return 0x30000f + i * 256;
}

int function_suffix_ahead(void) {
    int saved_cursor = cursor, saved_line = line, saved_strings = string_used, t;
    t = lex();
    cursor = saved_cursor;
    line = saved_line;
    string_used = saved_strings;
    return t == ')' || type_start(t);
}

int function_suffix(int capture) {
    int saved_locals = local_count, saved_tags = tag_count, saved_bytes = local_bytes;
    int saved_peak = frame_peak, saved_block = block_depth, saved_storage = decl_storage,
        saved_alias = decl_typedef;
    int parameters[128], n = 0, t, id, storage, result, old = 0;
    block_depth++;
    expect('(');
    if (token == ')')
        n = -1;
    if (capture && token != ')' && !type_start(token)) {
        old = 1;
        while (!failed) {
            if (n == 127)
                return error();
            id = name();
            next();
            definition_parameter_name[n] = id;
            definition_parameter_type[n] = 1;
            n++;
            local(11, id);
            local_type[id] = 1;
            if (token != ',')
                break;
            next();
            if (token == ')')
                return error();
        }
    } else
        while (!failed && token != ')') {
            t = specifiers();
            storage = decl_storage;
            if (!t && !n && token == ')' && !storage)
                break;
            if (storage && storage != 3)
                return error();
            t = declarator(t, 2);
            id = decl_result_name;
            if (is_array(t))
                t = make_pointer(array_base[(t - 4096) / 256]);
            if (is_function(t))
                t = make_pointer(t);
            if (!t || n == 127)
                return error();
            if (capture) {
                definition_parameter_name[n] = id;
                definition_parameter_type[n] = t;
            }
            parameters[n++] = t;
            if (id >= 0) {
                local(11, id);
                local_type[id] = t;
                register_object[id] = storage == 3;
            }
            if (token != ',')
                break;
            next();
            if (token == ')')
                return error();
        }
    expect(')');
    result = make_function(1, old ? -1 : n, parameters);
    if (capture) {
        definition_ready = 1;
        definition_old = old;
        definition_parameter_count = n < 0 ? 0 : n;
    } else {
        restore_locals(saved_locals);
        restore_tags(saved_tags);
        local_bytes = saved_bytes;
        frame_peak = saved_peak;
        block_depth = saved_block;
    }
    decl_storage = saved_storage;
    decl_typedef = saved_alias;
    return result;
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
    if (token == '(' && (!abstract || !function_suffix_ahead())) {
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
    while (!failed && (token == '[' || token == '(')) {
        if (token == '(') {
            if (declarator_top_function && declarator_active == 1 && decl_depth == 1 && !head)
                break;
            n = function_suffix(declarator_top_function && declarator_active == 1 && !head &&
                                !definition_ready);
            node = decl_node(3, n);
        } else {
            next();
            n = 0;
            if (token != ']')
                n = constant_bound();
            expect(']');
            node = decl_node(2, n);
        }
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
    if (decl_kind[node] == 3) {
        node = (decl_length[node] - 0x30000f) / 256;
        return make_function(t, function_arity[node], &function_parameter[node * 128]);
    }
    return make_array(t, decl_length[node]);
}

int declarator(int base, int abstract) {
    int head, result, i, saved_count = decl_count, saved_name = decl_name;
    int saved_depth = decl_depth, saved_parentheses = decl_parentheses;
    int saved_kind[256], saved_next[256], saved_length[256];
    if (abstract && decl_typedef)
        return error();
    if ((abstract == 1 && decl_storage) ||
        (abstract == 2 && (decl_storage == 2 || decl_storage >= 4)))
        return error();
    declarator_active++;
    if (declarator_active > 64)
        return error();
    if (declarator_active > 1) {
        for (i = 1; i <= saved_count; i++) {
            saved_kind[i] = decl_kind[i];
            saved_next[i] = decl_next[i];
            saved_length[i] = decl_length[i];
        }
    }
    decl_count = 0;
    decl_name = -1;
    decl_depth = 0;
    decl_parentheses = 0;
    head = declarator_chain(abstract);
    result = apply_declarator(head, base);
    decl_result_name = decl_name;
    if (declarator_active > 1) {
        for (i = 1; i <= saved_count; i++) {
            decl_kind[i] = saved_kind[i];
            decl_next[i] = saved_next[i];
            decl_length[i] = saved_length[i];
        }
        decl_count = saved_count;
        decl_name = saved_name;
        decl_depth = saved_depth;
        decl_parentheses = saved_parentheses;
    }
    declarator_active--;
    return result;
}

int promoted(int t) {
    if ((t >= 5 && t <= 9) || t == 12 || is_enum(t))
        return 1;
    return t;
}

int unsigned_type(int t) {
    return t == 2 || t == 4 || t == 11;
}

int common_type(int a, int b) {
    if ((a == 16 && function_pointer(b)) || (b == 16 && function_pointer(a)))
        return error();
    if (is_record(a) || is_record(b)) {
        if (a == b)
            return a;
        return error();
    }
    if (is_pointer(a) || is_pointer(b)) {
        if (a == b)
            return a;
        if (is_pointer(a) && is_pointer(b) && (a == 16 || b == 16))
            return 16;
        if (is_pointer(a) && is_pointer(b))
            return composite_type(a, b);
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
    if (is_function(t) || is_function(expression_type))
        return error();
    if ((t == 16 && function_pointer(expression_type)) ||
        (expression_type == 16 && function_pointer(t) && !expression_zero))
        return error();
    if (is_record(t)) {
        type_size(t);
        if (expression_type != t)
            return error();
        return 0;
    }
    if (t && is_record(expression_type))
        return error();
    if (is_enum(t))
        type_size(t);
    if (is_array(t))
        return error();
    if (t && !expression_type)
        return error();
    if (t == 12) {
        boolean(0x95);
        expression_type = 12;
        return 0;
    }
    if (is_pointer(t)) {
        if (is_pointer(expression_type)) {
            if (t != 16 && expression_type != 16)
                composite_type(t, expression_type);
        } else if (!expression_zero)
            return error();
        expression_type = t;
        return 0;
    }
    if (t && is_pointer(expression_type))
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
    if (is_function(t))
        return error();
    if (is_record(t) || (t && is_record(expression_type)))
        return error();
    if (t && !expression_type)
        return error();
    if (is_pointer(t)) {
        if (!wide_type(expression_type) && !unsigned_type(promoted(expression_type)))
            sequence(0xc06348, 3);
        expression_zero = expression_zero && !is_pointer(expression_type) && t == 16;
        expression_type = t;
        return 0;
    }
    if (is_pointer(expression_type)) {
        expression_type = 11;
        expression_zero = 0;
    }
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
    } else if ((t >= 5 && t <= 9) || t == 12) {
        emit(0x0f);
        if (t == 5 || t == 9)
            emit(0xbe);
        if (t == 6 || t == 12)
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
    expression_array_bytes = array_bytes[(t - 4096) / 256];
    expression_type = make_pointer(array_base[(t - 4096) / 256]);
    expression_zero = 0;
    return 0;
}

int indirect_value(int t) {
    if (is_function(t))
        return function_value(t);
    expression_register = 0;
    sequence(0xc28948, 3);
    if (is_record(t))
        record_value(t);
    else if (is_array(t)) {
        expression_load = position;
        array_value(t);
    } else
        load_opcode(t, 0);
    return -1;
}

int object_address(int id) {
    if (local_offset[id] > 0 || local_offset[id] == -3 || alias_type[id])
        return 0;
    if (local_offset[id] == -1)
        return local_static_address[id];
    return global_address[id];
}

int object_type(int id) {
    if (local_offset[id] < 0)
        return local_type[id];
    return global_type[id];
}

int object_exists(int id) {
    if (local_offset[id] > 0 || local_offset[id] == -3 || alias_type[id])
        return 0;
    return local_offset[id] == -1 || local_offset[id] == -2 || global_visible[id];
}

int object_reference(int id) {
    if (object_address(id))
        return relative(object_address(id));
    if (object_ref_count == 16384)
        return error();
    object_ref_position[object_ref_count] = position;
    object_ref_symbol[object_ref_count] = id;
    object_ref_count++;
    return emit32(0);
}

int rewind_code(int target) {
    position = target;
    while (object_ref_count && object_ref_position[object_ref_count - 1] >= target)
        object_ref_count--;
    return 0;
}

int load(int id) {
    int start = position;
    expression_register = local_offset[id] > 0 && register_object[id];
    if (local_offset[id] > 0) {
        if (is_record(local_type[id])) {
            sequence(0x858d48, 3);
            emit32(-local_offset[id]);
            record_value(local_type[id]);
            expression_load = start;
            return local_offset[id];
        }
        if (is_array(local_type[id])) {
            if (register_object[id] && !unevaluated_depth)
                return error();
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
    if (object_exists(id)) {
        if (is_record(object_type(id))) {
            sequence(0x058d48, 3);
            object_reference(id);
            record_value(object_type(id));
            expression_load = start;
            return -id - 2;
        }
        if (is_array(object_type(id))) {
            expression_load = position;
            sequence(0x058d48, 3);
            object_reference(id);
            array_value(object_type(id));
            return -id - 2;
        }
        load_opcode(object_type(id), 5);
        object_reference(id);
        return -id - 2;
    }
    return error();
}

int store(int lvalue, int t) {
    int n = type_size(t), opcode = 0x89;
    if (is_record(t))
        return store_record(lvalue, t);
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
        return object_reference(-lvalue - 2);
    }
    emit(0x85);
    return emit32(-lvalue);
}

int boolean(int cc) {
    if (!expression_type || is_record(expression_type))
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
        if (op == '-' && (is_pointer(a) && a != 16) && (is_pointer(b) && b != 16)) {
            composite_type(a, b);
            type_size(b - 16);
            n = type_size(a - 16);
            sequence(0xc18948, 3);
            emit(0x58);
            sequence(0xc82948, 3);
            sequence(0x9948, 2);
            emit(0xb9);
            emit32(n);
            sequence(0xf9f748, 3);
            expression_type = 10;
        } else if (is_pointer(a) && b > 0 && !is_pointer(b)) {
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
        } else if (op == '+' && is_pointer(b) && a > 0 && !is_pointer(a)) {
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
        if (!is_pointer(a) && !left_zero)
            return error();
        if (!is_pointer(b) && !expression_zero)
            return error();
        if ((a == 16 && function_pointer(b) && !left_zero) ||
            (b == 16 && function_pointer(a) && !expression_zero))
            return error();
        if (is_pointer(a) && is_pointer(b) && a != 16 && b != 16)
            composite_type(a, b);
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
        if (op != 128 && op != 129 && (!is_pointer(a) || !is_pointer(b)))
            return error();
        if (op != 128 && op != 129) {
            if (function_pointer(a) || function_pointer(b))
                return error();
            composite_type(a, b);
        }
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
    if (is_record(left_type) || is_record(right_type))
        return error();
    if (is_pointer(left_type) || is_pointer(right_type))
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
            if (function_pointer(middle_type) && expression_type == 16 && expression_zero)
                expression_type = middle_type;
            else if (function_pointer(expression_type) && middle_type == 16 && middle_zero)
                expression_type = expression_type;
            else if (is_pointer(middle_type) && expression_type > 0 &&
                     !is_pointer(expression_type) && expression_zero)
                expression_type = middle_type;
            else if (is_pointer(expression_type) && middle_type > 0 && !is_pointer(middle_type) &&
                     middle_zero)
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
        expression_register = 0;
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
        expression_register = 0;
    }
    return left;
}

int increment(int left, int op, int postfix) {
    int t = expression_type, step = 1;
    if (is_record(t))
        return error();
    if (left == 0 || expression_array_type)
        return error();
    if (left == -1)
        emit(0x52);
    if (is_pointer(t))
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
    int result, t, saved_position, saved_calls, bytes, register_id;
    if (failed)
        return 0;
    unary_depth = unary_depth + 1;
    if (unary_depth > 128)
        return error();
    if (op == 283) {
        saved_position = position;
        saved_calls = call_count;
        unevaluated_depth++;
        next();
        if (token == '(') {
            next();
            if (type_start(token)) {
                t = declarator(specifiers(), 1);
                expression_array_bytes = 0;
                expression_array_type = 0;
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
        if (expression_array_type)
            t = expression_array_type;
        if (!bytes)
            bytes = type_size(t);
        sizeof_value = bytes;
        unevaluated_depth--;
        rewind_code(saved_position);
        call_count = saved_calls;
        constant(bytes);
        expression_type = 11;
        expression_array_bytes = 0;
        expression_array_type = 0;
        result = 0;
    } else if (op == '&') {
        next();
        result = unary();
        if (!result || expression_register)
            return error();
        if (result > 0) {
            for (register_id = 0; register_id < count; register_id++) {
                if (local_offset[register_id] == result && register_object[register_id])
                    return error();
            }
        }
        rewind_code(expression_load);
        if (result == -1)
            sequence(0xd08948, 3);
        else if (result < -1) {
            sequence(0x058d48, 3);
            object_reference(-result - 2);
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
        if ((!is_pointer(expression_type) || expression_type == 16))
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
        if (!expression_type || is_record(expression_type))
            return error();
        if (is_pointer(expression_type) && op != '!')
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
        while (!failed && (token == 170 || token == 171 || token == '[' || token == '.' ||
                           token == 173 || token == '(')) {
            if (token == '(') {
                if (!function_pointer(expression_type))
                    return error();
                result = arguments(-1, (expression_type - 16 - 0x30000f) / 256);
            } else if (token == '.' || token == 173)
                result = member_access(result, token == 173);
            else if (token == '[') {
                t = expression_type;
                emit(0x50);
                next();
                full_expression();
                expect(']');
                binary('+', t, expression_type, 0);
                if ((!is_pointer(expression_type) || expression_type == 16))
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
    expression_register = 0;
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
    if (alias_type[id])
        return error();
    if (enum_visible(id)) {
        constant(enum_value(id));
        next();
        return 0;
    }
    next();
    if (token == '(' &&
        (local_offset[id] == -4 || (!local_offset[id] && !(symbol_flags[id] & 8)))) {
        if (!function_exists(id))
            implicit_function(id);
        return arguments(id, -1);
    }
    if (function_exists(id)) {
        sequence(0x058d48, 3);
        function_reference(id);
        return function_value(function_signature(id));
    }
    return load(id);
}

int arguments(int id, int signature) {
    int n = 0, i, saved_stack, result, output = 0, hidden = 0, t, copy, target = 0, expected = -1;
    if (id >= 0) {
        if (!function_exists(id))
            return error();
        signature = (function_signature(id) - 0x30000f) / 256;
        result = function_result[signature];
        expected = function_arity[signature];
    } else {
        result = function_result[signature];
        expected = function_arity[signature];
        target = temporary(8, 8);
        store(target, expression_type);
    }
    if (is_record(result)) {
        output = temporary(type_size(result), 16);
        hidden = record_indirect(result);
        if (hidden) {
            sequence(0x858d48, 3);
            emit32(-output);
            emit(0x50);
        }
    }
    expect('(');
    while (!failed && token != ')') {
        if (n == 127)
            return error();
        expression(1);
        if (expected >= 0) {
            if (n >= expected)
                return error();
            convert(function_parameter[signature * 128 + n]);
        } else {
            if (!expression_type)
                return error();
            convert(promoted(expression_type));
        }
        t = expression_type;
        if (is_record(t)) {
            if (record_indirect(t)) {
                copy = temporary(type_size(t), 16);
                store_record(copy, t);
                sequence(0x858d48, 3);
                emit32(-copy);
            } else
                load_opcode(record_word(t), 0);
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
    if (expected >= 0 && expected != n)
        return error();
    if (id >= 0)
        symbol_flags[id] = symbol_flags[id] | 2;
    n += hidden;
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
        invoke(id, target, n - hidden);
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
        invoke(id, target, n - hidden);
        sequence(0x24648b48, 4);
        emit(0x20);
    }
    if (output) {
        if (!hidden)
            store(output, record_word(result));
        sequence(0x858d48, 3);
        emit32(-output);
        record_value(result);
    }
    expression_type = result;
    expression_register = 0;
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
    if ((local_offset[id] != 0 || alias_type[id]) && local_depth[id] == block_depth)
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
    binding_alias[local_count] = alias_type[id];
    alias_type[id] = 0;
    binding_register[local_count] = register_object[id];
    register_object[id] = 0;
    binding_static_address[local_count] = local_static_address[id];
    local_static_address[id] = 0;
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
    int offset, base = specifiers(), is_alias = decl_typedef, storage = decl_storage, t, id,
                inferred, original;
    if (token == ';' && (is_enum(base) || is_record(base)) && !storage) {
        if (for_declaration)
            return error();
        next();
        return 0;
    }
    if (is_alias) {
        if (for_declaration)
            return error();
        return alias_declaration(base, 0);
    }
    if (storage == 4) {
        if (for_declaration)
            return error();
        return static_declaration(base);
    }
    if (storage == 5) {
        if (for_declaration)
            return error();
        return extern_declaration(base);
    }
    while (!failed) {
        t = declarator(base, 0);
        id = decl_name;
        if (is_function(t)) {
            if (for_declaration || (token != ',' && token != ';'))
                return error();
            block_function(id, t, storage);
        } else {
            inferred = is_array(t) && array_length[(t - 4096) / 256] == 0;
            original = t;
            if (inferred)
                t = infer_initializer(t, id, 0);
            offset = local(t, id);
            if (inferred)
                local_type[id] = original;
            register_object[id] = storage == 3;
            if (token == '=') {
                next();
                if (is_array(t) || is_record(t))
                    zero_local(offset, type_size(t));
                initialize_object(t, offset, 0, 0);
            }
            local_type[id] = t;
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
        local_bytes = binding_bytes[local_count];
        if (id < 0)
            continue;
        local_offset[id] = binding_offset[local_count];
        local_depth[id] = binding_depth[local_count];
        local_type[id] = binding_type[local_count];
        alias_type[id] = binding_alias[local_count];
        register_object[id] = binding_register[local_count];
        local_static_address[id] = binding_static_address[local_count];
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
    if (!expression_type || is_record(expression_type))
        return error();
    if (wide_type(expression_type))
        emit(0x48);
    return sequence(0xc085, 2);
}

int statement_body(void) {
    int target, end, start, old_break, old_continue;
    int saved_tags;
    int condition, increment, body, saved_locals, for_scope = 0;
    if (token == 286)
        return switch_statement();
    if (token == 269 || token == 272)
        return switch_label();
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
        saved_tags = tag_count;
        block_depth = block_depth + 1;
        next();
        while (!failed && token != '}') {
            if (token == 0)
                return error();
            if (type_start(token) && !label_ahead())
                declaration();
            else
                statement();
        }
        expect('}');
        restore_locals(saved_locals);
        restore_tags(saved_tags);
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
        if (is_record(return_type)) {
            if (record_indirect(return_type)) {
                sequence(0x8d8b48, 3);
                emit32(-return_address_offset);
                emit(0x51);
                store_record(-1, return_type);
            } else
                load_opcode(record_word(return_type), 0);
        }
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
            saved_tags = tag_count;
            block_depth = block_depth + 1;
            for_scope = 1;
            next();
            expect('(');
            if (type_start(token)) {
                for_declaration = 1;
                declaration();
                for_declaration = 0;
            } else {
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
            restore_tags(saved_tags);
            block_depth = block_depth - 1;
        }
        loop_depth = loop_depth - 1;
        return 0;
    }
    if (token == 263 || token == 264) {
        if (token == 264 && loop_depth == 0)
            return error();
        if (token == 263 && loop_depth == 0 && switch_depth == 0)
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

int initializer_scalar_shape(void) {
    int closes[128], depth = 0, conditional = 0, seen = 0;
    while (!failed) {
        if (!depth && !conditional && (token == ',' || token == '}'))
            break;
        if (!token || token == ';' || token == '{')
            return error();
        if (token == '(' || token == '[') {
            if (depth == 128)
                return error();
            closes[depth++] = token == '(' ? ')' : ']';
        } else if (token == ')' || token == ']') {
            if (!depth || closes[--depth] != token)
                return error();
        } else if (token == '?')
            conditional++;
        else if (token == ':') {
            if (!conditional)
                return error();
            conditional--;
        }
        seen = 1;
        next();
    }
    if (!seen || depth || conditional)
        return error();
    return 0;
}

int initializer_shape(int t) {
    int braces = 0, i = 0, n, element, node;
    if (failed)
        return 0;
    initializer_depth++;
    if (initializer_depth > 64)
        return error();
    if (token == '{') {
        braces = 1;
        next();
        if (token == '}')
            return error();
    }
    if (character_array(t) && token == 145) {
        while (!failed && token == 145) {
            i += literal_string_length;
            next();
        }
        i++;
    } else if (is_record(t)) {
        type_size(t);
        n = (t - 0x20000e) / 256;
        node = record_first[n];
        while (node && !failed && token != '}') {
            initializer_shape(member_type[node]);
            node = record_kind[n] == 2 ? 0 : member_next[node];
            if (node && token == ',')
                next();
            else
                break;
        }
    } else if (is_array(t)) {
        if (!braces && initializer_depth == 1)
            return error();
        n = array_length[(t - 4096) / 256];
        element = array_base[(t - 4096) / 256];
        while (!failed && (!n || i < n)) {
            if (token == '}')
                break;
            initializer_shape(element);
            i++;
            if ((!n || i < n) && token == ',')
                next();
            else
                break;
        }
    } else if (token == '{')
        initializer_shape(t);
    else
        initializer_scalar_shape();
    if (braces) {
        if (token == ',')
            next();
        expect('}');
    }
    initializer_depth--;
    return i;
}

int infer_initializer(int t, int id, int global) {
    int saved_cursor = cursor, saved_line = line, saved_strings = string_used, n, element = t;
    while (is_array(element))
        element = array_base[(element - 4096) / 256];
    if (!global && is_record(element))
        return infer_aggregate_array(t, id);
    expect('=');
    n = initializer_shape(t);
    cursor = saved_cursor;
    line = saved_line;
    string_used = saved_strings;
    token = '=';
    if (failed || !n)
        return error();
    return make_array(array_base[(t - 4096) / 256], n);
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

unsigned long long ce_value;
int ce_type;
int ce_depth;

unsigned long long ce_normalize(unsigned long long x, int t) {
    if (t == 12)
        return x != 0;
    if (t == 1 || t == 3 || is_enum(t))
        return (int)x;
    if (t == 2 || t == 4)
        return (unsigned int)x;
    if (t == 5 || t == 9)
        return (signed char)x;
    if (t == 6)
        return (unsigned char)x;
    if (t == 7)
        return (short)x;
    if (t == 8)
        return (unsigned short)x;
    return x;
}

int ce_expression(int minimum, int active);

int ce_unary(int active) {
    int op = token, t, saved_position, saved_calls;
    unsigned long long sign;
    if (failed)
        return 0;
    ce_depth++;
    if (ce_depth > 128)
        return error();
    if (op == 144) {
        ce_type = literal_type;
        ce_value = ce_normalize(
            ((unsigned long long)(unsigned int)literal_high << 32) | (unsigned int)value, ce_type);
        next();
    } else if (op >= 256 && enum_visible(op - 256)) {
        ce_type = 1;
        ce_value = (long long)enum_value(op - 256);
        next();
    } else if (op == 283) {
        saved_position = position;
        saved_calls = call_count;
        unary();
        ce_value = sizeof_value;
        ce_type = 11;
        rewind_code(saved_position);
        call_count = saved_calls;
    } else if (op == '(') {
        next();
        if (type_start(token)) {
            t = declarator(specifiers(), 1);
            expect(')');
            if (!t || is_pointer(t) || is_array(t) || is_record(t) || is_function(t))
                return error();
            type_size(t);
            ce_unary(active);
            ce_value = ce_normalize(ce_value, t);
            ce_type = t;
        } else {
            ce_expression(2, active);
            expect(')');
        }
    } else if (op == '+' || op == '-' || op == '~' || op == '!') {
        next();
        ce_unary(active);
        ce_type = promoted(ce_type);
        sign = 1ULL << (type_size(ce_type) * 8 - 1);
        if (op == '-') {
            if (active && !unsigned_type(ce_type) && ce_value == ce_normalize(sign, ce_type))
                return error();
            ce_value = 0ULL - ce_value;
        }
        if (op == '~')
            ce_value = ~ce_value;
        if (op == '!') {
            ce_value = !ce_value;
            ce_type = 1;
        }
        ce_value = ce_normalize(ce_value, ce_type);
    } else
        return error();
    ce_depth--;
    return 0;
}

int ce_binary(int op, unsigned long long a, unsigned long long b, int t, int active) {
    unsigned long long result = 0, sign = 1ULL << (type_size(t) * 8 - 1), x, y, limit;
    int negative, comparison = 0;
    a = ce_normalize(a, t);
    b = ce_normalize(b, t);
    if (op == '+') {
        result = a + b;
        if (active && !unsigned_type(t) && ((~(a ^ b) & (a ^ result)) & sign))
            return error();
    } else if (op == '-') {
        result = a - b;
        if (active && !unsigned_type(t) && (((a ^ b) & (a ^ result)) & sign))
            return error();
    } else if (op == '*') {
        if (active && !unsigned_type(t)) {
            negative = ((a & sign) != 0) ^ ((b & sign) != 0);
            x = (a & sign) ? 0ULL - a : a;
            y = (b & sign) ? 0ULL - b : b;
            limit = negative ? sign : sign - 1;
            if (y && x > limit / y)
                return error();
        }
        result = a * b;
    } else if (op == '/' || op == '%') {
        if (!active)
            result = 0;
        else {
            if (!b)
                return error();
            if (!unsigned_type(t) && a == ce_normalize(sign, t) && b == ~0ULL)
                return error();
            if (unsigned_type(t)) {
                if (op == '/')
                    result = a / b;
                else
                    result = a % b;
            } else {
                if (op == '/')
                    result = (long long)a / (long long)b;
                else
                    result = (long long)a % (long long)b;
            }
        }
    } else if (op == '&')
        result = a & b;
    else if (op == '|')
        result = a | b;
    else if (op == '^')
        result = a ^ b;
    else if (op == 128) {
        result = a == b;
        comparison = 1;
    } else if (op == 129) {
        result = a != b;
        comparison = 1;
    } else {
        comparison = 1;
        if (unsigned_type(t)) {
            if (op == '<')
                result = a < b;
            if (op == '>')
                result = a > b;
            if (op == 130)
                result = a <= b;
            if (op == 131)
                result = a >= b;
        } else {
            if (op == '<')
                result = (long long)a < (long long)b;
            if (op == '>')
                result = (long long)a > (long long)b;
            if (op == 130)
                result = (long long)a <= (long long)b;
            if (op == 131)
                result = (long long)a >= (long long)b;
        }
    }
    ce_type = comparison ? 1 : t;
    ce_value = ce_normalize(result, ce_type);
    return 0;
}

int ce_expression(int minimum, int active) {
    int op, prec, left_type, middle_type, selected, width, t;
    unsigned long long left, middle, sign;
    if (failed)
        return 0;
    ce_depth++;
    if (ce_depth > 128)
        return error();
    ce_unary(active);
    while (!failed) {
        op = token;
        prec = precedence(op);
        if (prec < minimum)
            break;
        left = ce_value;
        left_type = ce_type;
        next();
        if (op == '?') {
            selected = left != 0;
            ce_expression(2, active && selected);
            middle = ce_value;
            middle_type = ce_type;
            expect(':');
            ce_expression(2, active && !selected);
            t = common_type(middle_type, ce_type);
            ce_value = ce_normalize(selected ? middle : ce_value, t);
            ce_type = t;
        } else {
            selected = active;
            if (op == 134 && !left)
                selected = 0;
            if (op == 135 && left)
                selected = 0;
            ce_expression(prec + 1, selected);
            if (op == 134 || op == 135) {
                if (op == 134)
                    ce_value = (left != 0) && (ce_value != 0);
                else
                    ce_value = (left != 0) || (ce_value != 0);
                ce_type = 1;
            } else if (op == 132 || op == 133) {
                t = promoted(left_type);
                width = type_size(t) * 8;
                left = ce_normalize(left, t);
                sign = 1ULL << (width - 1);
                if (active && ce_value >= (unsigned int)width)
                    return error();
                if (!active)
                    ce_value = 0;
                if (op == 132) {
                    if (active && !unsigned_type(t) &&
                        ((left & sign) || left > ((sign - 1) >> ce_value)))
                        return error();
                    ce_value = left << ce_value;
                } else if (unsigned_type(t))
                    ce_value = left >> ce_value;
                else
                    ce_value = (long long)left >> ce_value;
                ce_type = t;
                ce_value = ce_normalize(ce_value, t);
            } else
                ce_binary(op, left, ce_value, common_type(left_type, ce_type), active);
        }
    }
    ce_depth--;
    return 0;
}

int constant_bound(void) {
    ce_expression(2, 1);
    if (!ce_value || ce_value > 0x1a00000ULL)
        return error();
    return (int)ce_value;
}

int enum_visible(int id) {
    if (alias_type[id])
        return 0;
    if (local_offset[id])
        return local_offset[id] == -3;
    return enum_global[id];
}

int enum_value(int id) {
    if (local_offset[id] == -3)
        return local_static_address[id];
    return enum_global_value[id];
}

int enum_binding(int id, int v) {
    if (!block_depth) {
        if (global_visible[id] || file_function_type[id] || alias_type[id] || enum_global[id])
            return error();
        enum_global[id] = 1;
        enum_global_value[id] = v;
        return 0;
    }
    if ((local_offset[id] || alias_type[id]) && local_depth[id] == block_depth)
        return error();
    if (local_count == 256)
        return error();
    binding_name[local_count] = id;
    binding_offset[local_count] = local_offset[id];
    binding_depth[local_count] = local_depth[id];
    binding_type[local_count] = local_type[id];
    binding_bytes[local_count] = local_bytes;
    binding_alias[local_count] = alias_type[id];
    binding_register[local_count] = register_object[id];
    binding_static_address[local_count] = local_static_address[id];
    local_count++;
    local_offset[id] = -3;
    local_type[id] = 1;
    local_depth[id] = block_depth;
    alias_type[id] = 0;
    register_object[id] = 0;
    local_static_address[id] = v;
    return 0;
}

int restore_tags(int saved) {
    int id;
    while (tag_count > saved) {
        tag_count--;
        id = tag_binding_name[tag_count];
        tag_type[id] = tag_binding_type[tag_count];
        tag_depth[id] = tag_binding_depth[tag_count];
    }
    return 0;
}

int enum_specifier(void) {
    int tag = -1, t, id, index;
    long long v = 0;
    next();
    if (token != '{') {
        tag = name();
        next();
    }
    if (token != '{') {
        if (tag < 0 || !is_enum(tag_type[tag]))
            return error();
        return tag_type[tag];
    }
    if (enum_count == 1024)
        return error();
    index = enum_count++;
    t = 0x10000d + index * 256;
    if (tag >= 0) {
        if (tag_type[tag] && tag_depth[tag] == block_depth)
            return error();
        if (block_depth) {
            if (tag_count == 1024)
                return error();
            tag_binding_name[tag_count] = tag;
            tag_binding_type[tag_count] = tag_type[tag];
            tag_binding_depth[tag_count] = tag_depth[tag];
            tag_count++;
        }
        tag_type[tag] = t;
        tag_depth[tag] = block_depth;
    }
    next();
    if (token == '}')
        return error();
    while (!failed) {
        id = name();
        next();
        if (token == '=') {
            next();
            ce_expression(2, 1);
            if (unsigned_type(ce_type) && ce_value > 2147483647ULL)
                return error();
            v = (long long)ce_value;
        }
        if (v < -2147483647LL - 1 || v > 2147483647LL)
            return error();
        enum_binding(id, (int)v);
        v++;
        if (token != ',')
            break;
        next();
        if (token == '}')
            break;
    }
    expect('}');
    enum_complete[index] = 1;
    return t;
}

int record_specifier(void) {
    int kind = token == 285 ? 1 : 2, tag = -1, t = 0, index, base, mt, id, node, last;
    int width, align, offset = 0, total = 0, maximum = 1, create = 1;
    record_depth++;
    if (record_depth > 64)
        return error();
    next();
    if (token != '{') {
        tag = name();
        next();
    }
    if (tag >= 0 && tag_type[tag]) {
        if (token != '{' && !(token == ';' && tag_depth[tag] != block_depth))
            create = 0;
        if (tag_depth[tag] == block_depth)
            create = 0;
        if (!create) {
            t = tag_type[tag];
            if (!is_record(t))
                return error();
            index = (t - 0x20000e) / 256;
            if (record_kind[index] != kind)
                return error();
        }
    }
    if (create) {
        if (record_count == 1024)
            return error();
        index = record_count++;
        t = 0x20000e + index * 256;
        record_kind[index] = kind;
        if (tag >= 0) {
            if (block_depth) {
                if (tag_count == 1024)
                    return error();
                tag_binding_name[tag_count] = tag;
                tag_binding_type[tag_count] = tag_type[tag];
                tag_binding_depth[tag_count] = tag_depth[tag];
                tag_count++;
            }
            tag_type[tag] = t;
            tag_depth[tag] = block_depth;
        }
    }
    if (token != '{') {
        record_depth--;
        return t;
    }
    if (record_size[index] || record_open[index])
        return error();
    record_open[index] = 1;
    next();
    while (!failed && token != '}') {
        base = specifiers();
        if (decl_storage)
            return error();
        while (!failed) {
            mt = declarator(base, 0);
            id = decl_name;
            if (token == ':' || token == '=' || token == '(')
                return error();
            width = type_size(mt);
            align = type_align(mt);
            if (failed)
                return 0;
            for (node = record_first[index]; node; node = member_next[node])
                if (member_name[node] == id)
                    return error();
            if (member_count == 8191)
                return error();
            node = ++member_count;
            member_name[node] = id;
            member_type[node] = mt;
            if (align > maximum)
                maximum = align;
            if (kind == 1) {
                offset = ((total + align - 1) / align) * align;
                if (width > 0x1a00000 - offset)
                    return error();
                total = offset + width;
            } else {
                offset = 0;
                if (width > total)
                    total = width;
            }
            member_offset[node] = offset;
            last = record_last[index];
            if (last)
                member_next[last] = node;
            else
                record_first[index] = node;
            record_last[index] = node;
            if (token != ',')
                break;
            next();
        }
        expect(';');
    }
    if (!record_first[index])
        return error();
    expect('}');
    record_size[index] = ((total + maximum - 1) / maximum) * maximum;
    record_align[index] = maximum;
    record_open[index] = 0;
    record_depth--;
    return t;
}

int record_value(int t) {
    expression_type = t;
    expression_zero = 0;
    expression_load = position;
    expression_array_type = 0;
    expression_array_bytes = 0;
    return 0;
}

int find_member(int t, int id) {
    int node;
    if (!is_record(t))
        return error();
    type_size(t);
    for (node = record_first[(t - 0x20000e) / 256]; node; node = member_next[node]) {
        if (member_name[node] == id)
            return node;
    }
    return error();
}

int member_access(int left, int arrow) {
    int t = expression_type, node, reg = expression_register;
    if (arrow) {
        if (!is_pointer(t))
            return error();
        t -= 16;
        reg = 0;
    }
    next();
    node = find_member(t, name());
    next();
    if (failed)
        return 0;
    if (member_offset[node]) {
        sequence(0x0548, 2);
        emit32(member_offset[node]);
    }
    t = member_type[node];
    if (reg && is_array(t) && !unevaluated_depth)
        return error();
    indirect_value(t);
    expression_register = reg;
    return arrow || left ? -1 : 0;
}

int store_record(int left, int t) {
    int n = type_size(t);
    if (left == -1)
        emit(0x59);
    emit(0x56);
    emit(0x57);
    sequence(0xc68948, 3);
    if (left == -1)
        sequence(0xcf8948, 3);
    else if (left < -1) {
        sequence(0x3d8d48, 3);
        object_reference(-left - 2);
    } else {
        sequence(0xbd8d48, 3);
        emit32(-left);
    }
    sequence(0xf88948, 3);
    emit(0xb9);
    emit32(n);
    emit(0xfc);
    sequence(0xa4f3, 2);
    emit(0x5f);
    emit(0x5e);
    return 0;
}

int record_indirect(int t) {
    int n;
    if (!is_record(t))
        return 0;
    n = type_size(t);
    return n != 1 && n != 2 && n != 4 && n != 8;
}

int record_word(int t) {
    int n = type_size(t);
    if (n == 1)
        return 6;
    if (n == 2)
        return 8;
    if (n == 4)
        return 2;
    if (n == 8)
        return 11;
    return error();
}

int temporary(int bytes, int align) {
    if (unevaluated_depth)
        return 16;
    if (bytes > 65536 - local_bytes)
        return error();
    local_bytes = ((local_bytes + bytes + align - 1) / align) * align;
    if (local_bytes > 65536)
        return error();
    if (local_bytes > frame_peak)
        frame_peak = local_bytes;
    return local_bytes;
}

int aggregate_parameters(int n, int hidden) {
    int i, spill = temporary((n + hidden) * 8, 8), offset, t;
    return_address_offset = spill;
    for (i = 0; i < n + hidden; i++) {
        offset = spill - i * 8;
        if (i == 0)
            sequence(0x8d8948, 3);
        else if (i == 1)
            sequence(0x958948, 3);
        else if (i == 2)
            sequence(0x85894c, 3);
        else if (i == 3)
            sequence(0x8d894c, 3);
        else {
            sequence(0x858b48, 3);
            emit32(16 + i * 8);
            sequence(0x858948, 3);
        }
        emit32(-offset);
    }
    for (i = 0; i < n; i++) {
        t = parameter_decl_type[i];
        sequence(0x858b48, 3);
        emit32(-(spill - (i + hidden) * 8));
        if (record_indirect(t))
            store_record(parameter_offset[i], t);
        else {
            if (is_record(t))
                t = record_word(t);
            else {
                expression_type = promoted(t);
                convert(t);
            }
            store(parameter_offset[i], t);
        }
    }
    return 0;
}

int switch_label(void) {
    int node;
    unsigned long long value_bits;
    if (!switch_depth)
        return error();
    if (token == 272) {
        if (switch_default)
            return error();
        next();
        expect(':');
        switch_default = position;
    } else {
        next();
        ce_expression(2, 1);
        expect(':');
        value_bits = ce_normalize(ce_value, switch_type);
        for (node = switch_head; node; node = case_next[node - 1]) {
            if (case_value[node - 1] == value_bits)
                return error();
        }
        if (case_count == 4096)
            return error();
        node = case_count;
        case_count++;
        case_value[node] = value_bits;
        case_address[node] = position;
        case_next[node] = switch_head;
        switch_head = node + 1;
    }
    return statement();
}

int switch_statement(void) {
    int old_head = switch_head, old_default = switch_default, old_type = switch_type;
    int old_break = break_head, saved_cases = case_count, dispatch, end, branch, node;
    unsigned long long bits;
    next();
    expect('(');
    full_expression();
    expect(')');
    if (!expression_type || is_pointer(expression_type) || is_record(expression_type))
        return error();
    switch_type = promoted(expression_type);
    convert(switch_type);
    if (!wide_type(switch_type)) {
        if (unsigned_type(switch_type))
            sequence(0xc089, 2);
        else
            sequence(0xc06348, 3);
    }
    dispatch = jump(0);
    switch_head = 0;
    switch_default = 0;
    break_head = 0;
    switch_depth++;
    statement();
    end = jump(0);
    patch(dispatch, position);
    sequence(0xc28949, 3);
    for (node = switch_head; node && !failed; node = case_next[node - 1]) {
        bits = case_value[node - 1];
        sequence(0xb848, 2);
        emit32((int)bits);
        emit32((int)(bits >> 32));
        sequence(0xc23949, 3);
        branch = jump(0x84);
        patch(branch, case_address[node - 1]);
    }
    if (switch_default) {
        branch = jump(0);
        patch(branch, switch_default);
    }
    patch(end, position);
    finish_breaks(break_head, position);
    switch_head = old_head;
    switch_default = old_default;
    switch_type = old_type;
    break_head = old_break;
    case_count = saved_cases;
    switch_depth--;
    return 0;
}

/* Static constants keep relocatable object identity separate from the offset.
   kind: 0 integer, 1 pointer value, 2 object designator (never loaded). */
int sc_kind;
int sc_null;
int sc_type;
int sc_base;
int sc_depth;
unsigned long long sc_bits;

int sc_decay(void) {
    if (sc_kind == 2) {
        if (is_function(sc_type))
            sc_type = make_pointer(sc_type);
        else {
            if (!is_array(sc_type))
                return error();
            sc_type = make_pointer(array_base[(sc_type - 4096) / 256]);
        }
        sc_kind = 1;
    }
    return 0;
}

int sc_add(int kind, int t, int base, unsigned long long bits, int subtract) {
    int width;
    sc_decay();
    if (!kind && sc_kind == 1 && !subtract) {
        width = type_size(sc_type - 16);
        sc_bits += bits * (unsigned int)width;
        return 0;
    }
    if (kind != 1 || sc_kind)
        return error();
    width = type_size(t - 16);
    if (subtract)
        bits -= sc_bits * (unsigned int)width;
    else
        bits += sc_bits * (unsigned int)width;
    sc_kind = 1;
    sc_type = t;
    sc_base = base;
    sc_bits = bits;
    return 0;
}

int sc_expression(int minimum, int active);

int sc_unary(int active) {
    int op = token, t, id, node, start, k, b;
    unsigned long long bits, sign;
    sc_null = 0;
    if (failed)
        return 0;
    sc_depth++;
    if (sc_depth > 128)
        return error();
    if (op == '&' || op == '*' || op == '+' || op == '-' || op == '~' || op == '!') {
        next();
        sc_unary(active);
        if (op == '&') {
            if (sc_kind != 2)
                return error();
            sc_kind = 1;
            sc_type = make_pointer(sc_type);
        } else if (op == '*') {
            sc_decay();
            if (sc_kind != 1 || sc_type == 16)
                return error();
            sc_kind = 2;
            sc_type -= 16;
        } else {
            sc_decay();
            if (sc_kind)
                return error();
            sc_type = promoted(sc_type);
            sign = 1ULL << (type_size(sc_type) * 8 - 1);
            if (op == '-') {
                if (active && !unsigned_type(sc_type) && sc_bits == ce_normalize(sign, sc_type))
                    return error();
                sc_bits = 0ULL - sc_bits;
            }
            if (op == '~')
                sc_bits = ~sc_bits;
            if (op == '!') {
                sc_bits = !sc_bits;
                sc_type = 1;
            }
            sc_bits = ce_normalize(sc_bits, sc_type);
        }
        sc_depth--;
        return 0;
    }
    if (op == '(') {
        next();
        if (type_start(token)) {
            t = declarator(specifiers(), 1);
            expect(')');
            sc_unary(active);
            sc_decay();
            if (!t || is_array(t) || is_record(t) || is_function(t))
                return error();
            if (is_pointer(t)) {
                sc_null = t == 16 && !sc_kind && !sc_bits;
                if (!sc_kind)
                    sc_base = -1;
                sc_kind = 1;
            } else {
                if (sc_kind)
                    return error();
                type_size(t);
                sc_bits = ce_normalize(sc_bits, t);
            }
            sc_type = t;
            sc_depth--;
            return 0;
        }
        sc_expression(2, active);
        expect(')');
    } else if (op == 144 || op == 283 || (op >= 256 && enum_visible(op - 256))) {
        ce_unary(active);
        sc_bits = ce_value;
        sc_type = ce_type;
        sc_kind = 0;
        sc_base = -1;
    } else if (op == 145) {
        join_strings();
        start = position;
        emit_string();
        sc_kind = 2;
        sc_type = expression_array_type;
        sc_base = -1;
        sc_bits = start + 5 + 0x400e00;
    } else {
        id = name();
        next();
        sc_kind = 2;
        sc_bits = 0;
        if (function_exists(id)) {
            sc_type = function_signature(id);
            sc_base = -id - 2;
        } else {
            if (!object_exists(id))
                return error();
            sc_type = object_type(id);
            sc_base = id;
        }
    }
    while (!failed && (token == '[' || token == '.' || token == 173)) {
        op = token;
        next();
        if (op == '[') {
            sc_decay();
            k = sc_kind;
            t = sc_type;
            b = sc_base;
            bits = sc_bits;
            sc_expression(2, active);
            expect(']');
            sc_add(k, t, b, bits, 0);
            sc_kind = 2;
            sc_type -= 16;
        } else {
            if (op == 173) {
                sc_decay();
                if (sc_kind != 1)
                    return error();
                sc_kind = 2;
                sc_type -= 16;
            }
            if (sc_kind != 2)
                return error();
            node = find_member(sc_type, name());
            next();
            sc_bits += (unsigned int)member_offset[node];
            sc_type = member_type[node];
        }
    }
    sc_depth--;
    return 0;
}

int sc_expression(int minimum, int active) {
    int op, prec, k, t, b, selected, mk, mt, mb, mn, width;
    unsigned long long bits, middle, sign;
    if (failed)
        return 0;
    sc_depth++;
    if (sc_depth > 128)
        return error();
    sc_unary(active);
    while (!failed) {
        op = token;
        prec = precedence(op);
        if (prec < minimum)
            break;
        sc_decay();
        k = sc_kind;
        t = sc_type;
        b = sc_base;
        bits = sc_bits;
        next();
        if (op == '?') {
            if (k)
                return error();
            selected = bits != 0;
            sc_expression(2, active && selected);
            sc_decay();
            mk = sc_kind;
            mt = sc_type;
            mb = sc_base;
            middle = sc_bits;
            mn = sc_null;
            expect(':');
            sc_expression(2, active && !selected);
            sc_decay();
            if (function_pointer(mt) && sc_type == 16 && sc_null)
                sc_type = mt;
            else if (function_pointer(sc_type) && mt == 16 && mn)
                mt = sc_type;
            else if (mk && !sc_kind && !sc_bits)
                sc_type = mt;
            else if (!mk && !middle && sc_kind)
                mt = sc_type;
            t = common_type(mt, sc_type);
            if (selected) {
                sc_kind = mk;
                sc_base = mb;
                sc_bits = middle;
            }
            sc_type = t;
            if (is_pointer(t)) {
                sc_null = t == 16 && !sc_kind && !sc_bits;
                if (!sc_kind)
                    sc_base = -1;
                sc_kind = 1;
            } else
                sc_bits = ce_normalize(sc_bits, t);
        } else {
            selected = active;
            if (op == 134 && !bits)
                selected = 0;
            if (op == 135 && bits)
                selected = 0;
            sc_expression(prec + 1, selected);
            sc_decay();
            if (k || sc_kind) {
                if (op != '+' && op != '-')
                    return error();
                sc_add(k, t, b, bits, op == '-');
            } else if (op == 134 || op == 135) {
                if (op == 134)
                    sc_bits = (bits != 0) && (sc_bits != 0);
                else
                    sc_bits = (bits != 0) || (sc_bits != 0);
                sc_type = 1;
            } else if (op == 132 || op == 133) {
                t = promoted(t);
                width = type_size(t) * 8;
                bits = ce_normalize(bits, t);
                sign = 1ULL << (width - 1);
                if (active && sc_bits >= (unsigned int)width)
                    return error();
                if (!active)
                    sc_bits = 0;
                if (op == 132) {
                    if (active && !unsigned_type(t) &&
                        ((bits & sign) || bits > ((sign - 1) >> sc_bits)))
                        return error();
                    sc_bits = bits << sc_bits;
                } else if (unsigned_type(t))
                    sc_bits = bits >> sc_bits;
                else
                    sc_bits = (long long)bits >> sc_bits;
                sc_type = t;
                sc_bits = ce_normalize(sc_bits, t);
            } else {
                ce_binary(op, bits, sc_bits, common_type(t, sc_type), active);
                sc_type = ce_type;
                sc_bits = ce_value;
            }
        }
        sc_null = 0;
    }
    sc_depth--;
    return 0;
}

int static_value(void) {
    sc_expression(2, 1);
    sc_decay();
    if (sc_kind == 1 && sc_base < -1) {
        sequence(0x058d48, 3);
        function_reference(-sc_base - 2);
        if (sc_bits)
            return error();
    } else if (sc_kind == 1 && sc_base >= 0) {
        sequence(0x058d48, 3);
        object_reference(sc_base);
        if (sc_bits) {
            sequence(0xb948, 2);
            emit32((int)sc_bits);
            emit32((int)(sc_bits >> 32));
            sequence(0xc80148, 3);
        }
    } else if (wide_type(sc_type)) {
        sequence(0xb848, 2);
        emit32((int)sc_bits);
        emit32((int)(sc_bits >> 32));
    } else
        constant((int)sc_bits);
    expression_type = sc_type;
    expression_zero = (!sc_kind && !sc_bits) || sc_null;
    expression_array_bytes = 0;
    expression_array_type = 0;
    return 0;
}

int initialize_object(int t, int base, int offset, int global) {
    int braces = 0, i, n, element, width, node, copied = 0;
    if (failed)
        return 0;
    initializer_depth = initializer_depth + 1;
    if (initializer_depth > 64)
        return error();
    if (!initializer_pending && token == '{') {
        braces = 1;
        next();
        if (token == '}')
            return error();
    }
    if (!initializer_pending && character_array(t) && token == 145) {
        join_strings();
        copy_string(t, base, offset, global);
    } else if (is_record(t)) {
        type_size(t);
        n = (t - 0x20000e) / 256;
        if (!braces && !global && (initializer_pending || token != 145)) {
            if (!initializer_pending) {
                expression(1);
                initializer_pending = 1;
            }
            if (expression_type == t) {
                initializer_address(base, offset, global);
                convert(t);
                store(-1, t);
                initializer_pending = 0;
                copied = 1;
            }
        }
        if (!copied) {
            if (!braces && initializer_depth == 1)
                return error();
            node = record_first[n];
            while (node && !failed && (initializer_pending || token != '}')) {
                initialize_object(member_type[node], base, offset + member_offset[node], global);
                node = record_kind[n] == 2 ? 0 : member_next[node];
                if (node && token == ',')
                    next();
                else
                    break;
            }
        }
    } else if (is_array(t)) {
        if (!braces && initializer_depth == 1)
            return error();
        i = (t - 4096) / 256;
        n = array_length[i];
        element = array_base[i];
        width = type_size(element);
        i = 0;
        while ((!n && initializer_probe || i < n) && !failed) {
            if (!initializer_pending && token == '}')
                break;
            if (!n && i >= 0x1a00000 / width)
                return error();
            initialize_object(element, base, offset + i * width, global);
            i++;
            if ((!n || i < n) && token == ',')
                next();
            else
                break;
        }
        if (initializer_probe && initializer_depth == 1)
            inferred_elements = i;
    } else if (!initializer_pending && token == '{') {
        initialize_object(t, base, offset, global);
    } else {
        if (!initializer_pending) {
            if (global)
                static_value();
            else
                expression(1);
        }
        initializer_pending = 0;
        initializer_address(base, offset, global);
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

int static_declaration(int base) {
    int id, t, width, align, element, address, inferred, skip, original;
    while (!failed) {
        t = declarator(base, 0);
        id = decl_name;
        if (failed)
            return 0;
        if ((local_offset[id] || alias_type[id]) && local_depth[id] == block_depth)
            return error();
        if (local_count == 256)
            return error();
        inferred = is_array(t) && array_length[(t - 4096) / 256] == 0;
        original = t;
        if (inferred)
            t = infer_initializer(t, id, 1);
        width = type_size(t);
        if (failed)
            return 0;
        element = t;
        while (is_array(element))
            element = array_base[(element - 4096) / 256];
        align = type_align(t);
        allocation = ((allocation + align - 1) / align) * align;
        if (allocation + width > 0x2000000)
            return error();
        address = allocation;
        allocation += width;
        binding_name[local_count] = id;
        binding_offset[local_count] = local_offset[id];
        binding_depth[local_count] = local_depth[id];
        binding_type[local_count] = local_type[id];
        binding_bytes[local_count] = local_bytes;
        binding_alias[local_count] = alias_type[id];
        binding_register[local_count] = register_object[id];
        binding_static_address[local_count] = local_static_address[id];
        local_count++;
        local_offset[id] = -1;
        local_static_address[id] = address;
        local_type[id] = t;
        local_depth[id] = block_depth;
        alias_type[id] = 0;
        register_object[id] = 0;
        if (inferred)
            local_type[id] = original;
        if (inferred || token == '=') {
            skip = jump(0);
            if (initializer_tail)
                patch(initializer_tail, position);
            else
                initializer_start = position;
            next();
            initialize_object(t, address, 0, 1);
            initializer_tail = jump(0);
            patch(skip, position);
        }
        local_type[id] = t;
        if (token != ',')
            break;
        next();
    }
    return expect(';');
}

int composite_type(int a, int b) {
    int na, nb, element, i, n, parameters[128];
    if (a == b)
        return a;
    if (is_enum(a) && b == 1)
        return a;
    if (is_enum(b) && a == 1)
        return b;
    if (is_function(a) && is_function(b)) {
        a = (a - 0x30000f) / 256;
        b = (b - 0x30000f) / 256;
        element = composite_type(function_result[a], function_result[b]);
        na = function_arity[a];
        nb = function_arity[b];
        if (na >= 0 && nb >= 0 && na != nb)
            return error();
        n = na >= 0 ? na : nb;
        for (i = 0; i < n; i++) {
            if (na < 0)
                parameters[i] = composite_type(function_parameter[b * 128 + i],
                                               promoted(function_parameter[b * 128 + i]));
            else if (nb < 0)
                parameters[i] = composite_type(function_parameter[a * 128 + i],
                                               promoted(function_parameter[a * 128 + i]));
            else
                parameters[i] = composite_type(function_parameter[a * 128 + i],
                                               function_parameter[b * 128 + i]);
        }
        return make_function(element, n, parameters);
    }
    if (is_array(a) && is_array(b)) {
        na = array_length[(a - 4096) / 256];
        nb = array_length[(b - 4096) / 256];
        if (na && nb && na != nb)
            return error();
        element = composite_type(array_base[(a - 4096) / 256], array_base[(b - 4096) / 256]);
        return make_array(element, na ? na : nb);
    }
    if (is_pointer(a) && is_pointer(b) && !is_array(a) && !is_array(b))
        return make_pointer(composite_type(a - 16, b - 16));
    return error();
}

int allocate_global(int id) {
    int t = global_type[id], width = type_size(t), element = t, align;
    if (failed)
        return 0;
    while (is_array(element))
        element = array_base[(element - 4096) / 256];
    align = type_align(t);
    allocation = ((allocation + align - 1) / align) * align;
    if (allocation + width > 0x2000000)
        return error();
    global_address[id] = allocation;
    allocation += width;
    global_extent[id] = width;
    return 0;
}

int extern_declaration(int base) {
    int id, t, visible, same;
    while (!failed) {
        t = declarator(base, 0);
        id = decl_name;
        if (failed || !t)
            return error();
        if (is_function(t)) {
            if (token != ',' && token != ';')
                return error();
            block_function(id, t, 5);
        } else {
            if (token == '=' || token == '(')
                return error();
            same = (local_offset[id] || alias_type[id]) && local_depth[id] == block_depth;
            if (same && local_offset[id] != -2)
                return error();
            visible = local_offset[id] == -2 ||
                      (!local_offset[id] && !alias_type[id] && global_visible[id]);
            if ((symbol_flags[id] & 71) || ((symbol_flags[id] & 32) && !visible))
                return error();
            if (visible)
                t = composite_type(object_type(id), t);
            if (entity_type[id])
                entity_type[id] = composite_type(entity_type[id], t);
            else
                entity_type[id] = t;
            symbol_flags[id] = symbol_flags[id] | 8;
            if (!same) {
                if (local_count == 256)
                    return error();
                binding_name[local_count] = id;
                binding_offset[local_count] = local_offset[id];
                binding_depth[local_count] = local_depth[id];
                binding_type[local_count] = local_type[id];
                binding_bytes[local_count] = local_bytes;
                binding_alias[local_count] = alias_type[id];
                binding_register[local_count] = register_object[id];
                binding_static_address[local_count] = local_static_address[id];
                local_count++;
            }
            local_offset[id] = -2;
            local_type[id] = t;
            local_depth[id] = block_depth;
            alias_type[id] = 0;
            register_object[id] = 0;
            local_static_address[id] = 0;
        }
        if (token != ',')
            break;
        next();
    }
    return expect(';');
}

int finish_objects(void) {
    int id, t, p;
    for (id = 0; id < count && !failed; id++) {
        if (global_tentative[id] && !global_address[id]) {
            t = global_type[id];
            if (is_array(t) && !array_length[(t - 4096) / 256])
                global_type[id] = make_array(array_base[(t - 4096) / 256], 1);
            allocate_global(id);
        }
    }
    for (id = 0; id < object_ref_count && !failed; id++) {
        t = object_ref_symbol[id];
        p = object_ref_position[id];
        if (!global_address[t])
            return error();
        write32(p, global_address[t] - (p + 0x400e00 + 4));
    }
    return 0;
}

int merge_function_entity(int id, int t, int storage) {
    int i, signature = (t - 0x30000f) / 256, n = function_arity[signature];
    if (symbol_flags[id] & 8)
        return error();
    if (storage == 4) {
        if (symbol_flags[id] && !(symbol_flags[id] & 32))
            return error();
        symbol_flags[id] = symbol_flags[id] | 32;
    }
    if ((symbol_flags[id] & 1) && !(symbol_flags[id] & 4) && n >= 0) {
        if (arity[id] != n)
            return error();
        for (i = 0; i < n; i++)
            composite_type(parameter_type[id * 128 + i], function_parameter[signature * 128 + i]);
    }
    if (symbol_flags[id] & 69)
        t = composite_type(function_entity_signature(id), t);
    signature = (t - 0x30000f) / 256;
    n = function_arity[signature];
    global_type[id] = function_result[signature];
    if (id == 6 && (n > 0 || is_record(global_type[id])))
        return error();
    if (n >= 0) {
        symbol_flags[id] = symbol_flags[id] | 4;
        arity[id] = n;
        for (i = 0; i < n; i++)
            parameter_type[id * 128 + i] = function_parameter[signature * 128 + i];
    } else
        symbol_flags[id] = symbol_flags[id] | 64;
    return 0;
}

int expose_file_function(int id, int t) {
    if (file_function_type[id])
        t = composite_type(file_function_type[id], t);
    file_function_type[id] = t;
    return 0;
}

int typed_function_declaration(int id, int t, int storage) {
    int i;
    if (token != ',' && token != ';' && !(token == '{' && definition_ready))
        return error();
    if (definition_ready) {
        for (i = 0; i < local_count; i++)
            if (binding_name[i] == id && binding_alias[i])
                return error();
    }
    if (alias_type[id] || enum_global[id])
        return error();
    merge_function_entity(id, t, storage);
    expose_file_function(id, t);
    return 0;
}

int block_function(int id, int t, int storage) {
    int same = (local_offset[id] || alias_type[id]) && local_depth[id] == block_depth;
    int visible = function_exists(id);
    if (storage && storage != 5)
        return error();
    if (same && local_offset[id] != -4)
        return error();
    if ((symbol_flags[id] & 32) && !visible)
        return error();
    if (visible)
        t = composite_type(function_signature(id), t);
    merge_function_entity(id, t, 0);
    if (!same) {
        if (local_count == 256)
            return error();
        binding_name[local_count] = id;
        binding_offset[local_count] = local_offset[id];
        binding_depth[local_count] = local_depth[id];
        binding_type[local_count] = local_type[id];
        binding_bytes[local_count] = local_bytes;
        binding_alias[local_count] = alias_type[id];
        binding_register[local_count] = register_object[id];
        binding_static_address[local_count] = local_static_address[id];
        local_count++;
    }
    local_offset[id] = -4;
    local_type[id] = t;
    local_depth[id] = block_depth;
    alias_type[id] = 0;
    register_object[id] = 0;
    local_static_address[id] = 0;
    return 0;
}

int implicit_function(int id) {
    int t = make_function(1, -1, &parameter_type[id * 128]);
    if (block_depth)
        return block_function(id, t, 0);
    merge_function_entity(id, t, 0);
    return expose_file_function(id, t);
}

int global(int id, int t, int base, int storage) {
    int inferred, incomplete, initialized, known, original;
    while (!failed) {
        if (is_function(t))
            typed_function_declaration(id, t, storage);
        else {
            known = symbol_flags[id] & 8;
            if (alias_type[id] || enum_global[id] || (symbol_flags[id] && !known))
                return error();
            if (known) {
                if (storage == 4 && !(symbol_flags[id] & 32))
                    return error();
                if (storage == 0 && (symbol_flags[id] & 32))
                    return error();
                if (global_visible[id])
                    t = composite_type(global_type[id], t);
            }
            incomplete = (is_array(t) && !array_length[(t - 4096) / 256]) ||
                         (is_record(t) && !record_size[(t - 0x20000e) / 256]);
            inferred = is_array(t) && incomplete && token == '=';
            original = t;
            if (inferred)
                t = infer_initializer(t, id, 1);
            incomplete = (is_array(t) && !array_length[(t - 4096) / 256]) ||
                         (is_record(t) && !record_size[(t - 0x20000e) / 256]);
            initialized = inferred || token == '=';
            if (!t || (incomplete && storage == 4))
                return error();
            if (initialized && global_initialized[id])
                return error();
            if (entity_type[id])
                entity_type[id] = composite_type(entity_type[id], t);
            else
                entity_type[id] = t;
            global_type[id] = t;
            global_visible[id] = 1;
            symbol_flags[id] = symbol_flags[id] | 8;
            if (storage == 4)
                symbol_flags[id] = symbol_flags[id] | 32;
            if (storage != 5 && !initialized)
                global_tentative[id] = 1;
            if (!global_address[id] && !incomplete && (initialized || global_tentative[id]))
                allocate_global(id);
            if (initialized) {
                global_initialized[id] = 1;
                if (initializer_tail)
                    patch(initializer_tail, position);
                else
                    initializer_start = position;
                if (inferred)
                    global_type[id] = original;
                next();
                initialize_object(t, global_address[id], 0, 1);
                global_type[id] = t;
                initializer_tail = jump(0);
            }
        }
        if (token != ',')
            break;
        next();
        t = declarator(base, 0);
        id = decl_name;
    }
    return expect(';');
}

int parameter_name[128];

int parameter(int base, int function_id, int known) {
    int storage = decl_storage, pt = declarator(base, 2), i;
    if (storage && storage != 3)
        return error();
    if (is_array(pt))
        pt = make_pointer(array_base[(pt - 4096) / 256]);
    if (is_function(pt))
        pt = make_pointer(pt);
    if (parameter_count == 127 || local_count == 256 || !pt)
        return error();
    i = function_id * 128 + parameter_count;
    parameter_count++;
    parameter_decl_type[parameter_count - 1] = pt;
    parameter_name[parameter_count - 1] = decl_name;
    if (known)
        parameter_type[i] = composite_type(parameter_type[i], pt);
    else
        parameter_type[i] = pt;
    if (decl_name == -1) {
        binding_name[local_count] = -1;
        binding_bytes[local_count] = local_bytes;
        local_count = local_count + 1;
        local_bytes = local_bytes + 8;
        return 1;
    }
    local(11, decl_name);
    local_type[decl_name] = pt;
    register_object[decl_name] = storage == 3;
    return 0;
}

int alias_declaration(int base, int global_scope) {
    int id, t;
    while (!failed) {
        t = declarator(base, 0);
        id = decl_name;
        if (failed)
            return 0;
        if (global_scope) {
            if (global_visible[id] || enum_global[id] || file_function_type[id] || alias_type[id])
                return error();
            alias_type[id] = t + 1;
            symbol_flags[id] = symbol_flags[id] | 16;
        } else {
            if ((local_offset[id] || alias_type[id]) && local_depth[id] == block_depth)
                return error();
            if (local_count == 256)
                return error();
            binding_name[local_count] = id;
            binding_offset[local_count] = local_offset[id];
            binding_depth[local_count] = local_depth[id];
            binding_type[local_count] = local_type[id];
            binding_bytes[local_count] = local_bytes;
            binding_alias[local_count] = alias_type[id];
            binding_register[local_count] = register_object[id];
            register_object[id] = 0;
            binding_static_address[local_count] = local_static_address[id];
            local_static_address[id] = 0;
            local_count++;
            local_offset[id] = 0;
            local_type[id] = 0;
            local_depth[id] = block_depth;
            alias_type[id] = t + 1;
        }
        if (token != ',')
            break;
        next();
    }
    return expect(';');
}

int old_parameter_seen[128];

int old_parameters(int id, int known) {
    int i, p, base, pt, storage;
    while (!failed && token != ')') {
        if (parameter_count == 127)
            return error();
        p = name();
        next();
        i = parameter_count++;
        parameter_name[i] = p;
        parameter_decl_type[i] = 1;
        old_parameter_seen[i] = 0;
        local(11, p);
        local_type[p] = 1;
        if (token != ',')
            break;
        next();
        if (token == ')')
            return error();
    }
    expect(')');
    return old_parameter_declarations(id, known);
}

int old_parameter_declarations(int id, int known) {
    int i, p, base, pt, storage;
    while (!failed && type_start(token)) {
        base = specifiers();
        storage = decl_storage;
        if (storage && storage != 3)
            return error();
        while (!failed) {
            pt = declarator(base, 0);
            p = decl_name;
            if (is_array(pt))
                pt = make_pointer(array_base[(pt - 4096) / 256]);
            if (is_function(pt))
                pt = make_pointer(pt);
            if (!pt)
                return error();
            for (i = 0; i < parameter_count && parameter_name[i] != p; i++) {
            }
            if (i == parameter_count || old_parameter_seen[i])
                return error();
            old_parameter_seen[i] = 1;
            parameter_decl_type[i] = pt;
            local_type[p] = pt;
            register_object[p] = storage == 3;
            if (token != ',')
                break;
            next();
        }
        expect(';');
    }
    if (token != '{')
        return error();
    if (known && arity[id] != parameter_count)
        return error();
    for (i = 0; i < parameter_count; i++) {
        pt = promoted(parameter_decl_type[i]);
        if (known)
            composite_type(parameter_type[id * 128 + i], pt);
        else
            parameter_type[id * 128 + i] = pt;
    }
    return 0;
}

int function(void) {
    int id, i, n, t, base, known, storage, unnamed = 0, hidden, aggregate, unspecified, old = 0,
                                           implicit = 0, declaration_base, declaration_result;
    block_depth = 0;
    if (type_start(token))
        base = specifiers();
    else {
        base = 1;
        decl_storage = 0;
        decl_typedef = 0;
        implicit = 1;
    }
    declaration_base = base;
    storage = decl_storage;
    if (token == ';' && (is_enum(base) || is_record(base)) && !storage) {
        next();
        return 0;
    }
    if (decl_storage == 2 || decl_storage == 3)
        return error();
    if (decl_typedef)
        return alias_declaration(base, 1);
    definition_ready = 0;
    definition_old = 0;
    declarator_top_function = 1;
    t = declarator(base, 0);
    declarator_top_function = 0;
    id = decl_name;
    if (failed)
        return 0;
    if (is_function(t) && definition_ready && definition_old) {
        parameter_count = definition_parameter_count;
        for (i = 0; i < parameter_count; i++) {
            parameter_name[i] = definition_parameter_name[i];
            parameter_decl_type[i] = 1;
            old_parameter_seen[i] = 0;
        }
        old_parameter_declarations(id, (symbol_flags[id] & 4) != 0);
        for (i = 0; i < parameter_count; i++)
            definition_parameter_type[i] = parameter_decl_type[i];
    }
    if (is_function(t) && token == '{' && definition_ready) {
        n = definition_parameter_count;
        if ((symbol_flags[id] & 4) && function_arity[(t - 0x30000f) / 256] < 0 && arity[id] != n)
            return error();
        typed_function_declaration(id, t, storage);
        t = global_type[id];
        arity[id] = n;
        parameter_count = n;
        for (i = 0; i < n; i++) {
            parameter_name[i] = definition_parameter_name[i];
            parameter_decl_type[i] = definition_parameter_type[i];
            if (parameter_name[i] < 0)
                unnamed = 1;
        }
        definition_ready = 0;
        goto function_body;
    }
    if (definition_ready) {
        restore_locals(0);
        restore_tags(0);
        block_depth = 0;
        definition_ready = 0;
    }
    if (token != '(') {
        if (implicit)
            return error();
        return global(id, t, base, storage);
    }
    if (alias_type[id] || enum_global[id])
        return error();
    /* Simple definitions retain the existing parameter-list parser. */
    if (is_array(t) || is_function(t) || (decl_parentheses && t != base))
        return error();
    if (symbol_flags[id] & 8)
        return error();
    if (storage == 4) {
        if (symbol_flags[id] && !(symbol_flags[id] & 32))
            return error();
        symbol_flags[id] = symbol_flags[id] | 32;
    }
    declaration_result = t;
    known = (symbol_flags[id] & 4) != 0;
    if (symbol_flags[id] & 68)
        t = composite_type(global_type[id], t);
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
    parameter_count = 0;
    next();
    unspecified = token == ')';
    old = !unspecified && !type_start(token);
    if (old)
        old_parameters(id, known);
    else {
        while (!failed && token != ')') {
            base = specifiers();
            if (!base && token == ')' && !parameter_count && !decl_storage)
                break;
            unnamed = unnamed | parameter(base, id, known || ((symbol_flags[id] & 1) != 0));
            if (token != ',')
                break;
            next();
            if (token == ')')
                return error();
        }
        expect(')');
    }
    n = parameter_count;
    if (id == 6 && (n != 0 || is_record(t)))
        return error();
    if (old) {
        symbol_flags[id] = symbol_flags[id] | 64;
        arity[id] = n;
    } else if (unspecified) {
        if (known) {
            for (i = 0; i < arity[id]; i++)
                composite_type(parameter_type[id * 128 + i],
                               promoted(parameter_type[id * 128 + i]));
            if (token != ';' && arity[id])
                return error();
        }
        symbol_flags[id] = symbol_flags[id] | 64;
        if (!known && token != ';')
            arity[id] = 0;
    } else {
        if (known && arity[id] != n)
            return error();
        if (!known && (symbol_flags[id] & 64)) {
            for (i = 0; i < n; i++)
                composite_type(parameter_type[id * 128 + i],
                               promoted(parameter_type[id * 128 + i]));
            if ((symbol_flags[id] & 1) && arity[id] != n)
                return error();
        }
        symbol_flags[id] = symbol_flags[id] | 4;
        arity[id] = n;
    }
    expose_file_function(
        id, make_function(declaration_result, (old || unspecified) ? -1 : n, parameter_decl_type));
    if (token == ',') {
        restore_locals(0);
        restore_tags(0);
        block_depth = 0;
        next();
        t = declarator(declaration_base, 0);
        id = decl_name;
        return global(id, t, declaration_base, storage);
    }
    if (token == ';') {
        next();
        restore_locals(0);
        restore_tags(0);
        block_depth = 0;
        return 0;
    }
function_body:
    if (unnamed || (symbol_flags[id] & 1) != 0)
        return error();
    symbol_flags[id] = symbol_flags[id] | 1;
    function_address[id] = position;
    for (i = 0; i < 1024; i = i + 1) {
        label_address[i] = 0;
        label_head[i] = 0;
    }
    /* Prototype names need bindings, but only definitions need object storage. */
    local_bytes = 0;
    frame_peak = 0;
    for (i = 0; i < local_count; i++) {
        binding_bytes[i] = local_bytes;
        base = binding_name[i];
        if (base >= 0 && local_offset[base] > 0) {
            local_bytes += ((type_size(local_type[base]) + 7) / 8) * 8;
            if (local_bytes > 65536)
                return error();
            local_offset[base] = local_bytes;
        }
    }
    frame_peak = local_bytes;
    for (i = 0; i < n; i++)
        parameter_offset[i] = local_offset[parameter_name[i]];
    prologue();
    hidden = record_indirect(t);
    aggregate = hidden;
    for (i = 0; i < n; i++)
        if (record_indirect(parameter_decl_type[i]))
            aggregate = 1;
    if (aggregate)
        aggregate_parameters(n, hidden);
    else {
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
    }
    for (i = 0; i < n; i++) {
        if (!aggregate && parameter_decl_type[i] == 12) {
            sequence(0x858b, 2);
            emit32(-parameter_offset[i]);
            expression_type = 1;
            convert(12);
            store(parameter_offset[i], 12);
        }
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
    restore_locals(0);
    restore_tags(0);
    block_depth = 0;
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
    write32(0xa4, 0x1a01000);
    write32(0xa8, 0x1200);
    write32(0xac, 0x1000);
    write32(0xb0, 0x400000);
    write32(0xb8, 0x1000);
    write32(0xbc, 0x200);
    write32(0xc0, 6);
    write32(0xc8, 6);
    write32(0xd0, 0x1c01000);
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
    write32(0x190, 0x1ff000);
    write32(0x194, 0x1000);
    write32(0x19c, 0x200);
    write32(0x1ac, -536870880);
    write32(0x1b0, 0x7373622e);
    write32(0x1b8, 0x1a01000);
    write32(0x1bc, 0x200000);
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
        relative(0x2000000);
        emit(0xb9);
        emit32(-11);
    } else {
        emit(0xb9);
        emit32(-10);
    }
    indirect(0x401040);
    sequence(0xc18948, 3);
    sequence(0x158d48, 3);
    relative(0x2000000);
    sequence(0xb841, 2);
    emit32(1);
    sequence(0x0d8d4c, 3);
    relative(0x2000004);
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
    relative(0x2000004);
    sequence(0x01f883, 3);
    second = jump(0x85);
    if (id == 11) {
        sequence(0x3024448b, 4);
        emit(0x25);
        emit32(255);
    } else {
        sequence(0x05b60f, 3);
        relative(0x2000000);
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

/* Transactional type-aware pass for automatic inferred aggregate arrays.
   Interned identifier text is retained; semantic bindings and emitted code are restored. */
int initializer_backup[154624];
int initializer_state[48];

int initializer_checkpoint(int save) {
    int i;
    for (i = 0; i < 1024; i++) {
        if (save)
            initializer_backup[0 + i] = local_offset[i];
        else
            local_offset[i] = initializer_backup[0 + i];
        if (save)
            initializer_backup[1024 + i] = local_depth[i];
        else
            local_depth[i] = initializer_backup[1024 + i];
        if (save)
            initializer_backup[2048 + i] = local_type[i];
        else
            local_type[i] = initializer_backup[2048 + i];
        if (save)
            initializer_backup[3072 + i] = alias_type[i];
        else
            alias_type[i] = initializer_backup[3072 + i];
        if (save)
            initializer_backup[4096 + i] = register_object[i];
        else
            register_object[i] = initializer_backup[4096 + i];
        if (save)
            initializer_backup[5120 + i] = local_static_address[i];
        else
            local_static_address[i] = initializer_backup[5120 + i];
        if (save)
            initializer_backup[6144 + i] = global_type[i];
        else
            global_type[i] = initializer_backup[6144 + i];
        if (save)
            initializer_backup[7168 + i] = symbol_flags[i];
        else
            symbol_flags[i] = initializer_backup[7168 + i];
        if (save)
            initializer_backup[8192 + i] = global_visible[i];
        else
            global_visible[i] = initializer_backup[8192 + i];
        if (save)
            initializer_backup[9216 + i] = entity_type[i];
        else
            entity_type[i] = initializer_backup[9216 + i];
        if (save)
            initializer_backup[10240 + i] = enum_complete[i];
        else
            enum_complete[i] = initializer_backup[10240 + i];
        if (save)
            initializer_backup[11264 + i] = enum_global[i];
        else
            enum_global[i] = initializer_backup[11264 + i];
        if (save)
            initializer_backup[12288 + i] = enum_global_value[i];
        else
            enum_global_value[i] = initializer_backup[12288 + i];
        if (save)
            initializer_backup[13312 + i] = tag_type[i];
        else
            tag_type[i] = initializer_backup[13312 + i];
        if (save)
            initializer_backup[14336 + i] = tag_depth[i];
        else
            tag_depth[i] = initializer_backup[14336 + i];
        if (save)
            initializer_backup[15360 + i] = record_size[i];
        else
            record_size[i] = initializer_backup[15360 + i];
        if (save)
            initializer_backup[16384 + i] = record_align[i];
        else
            record_align[i] = initializer_backup[16384 + i];
        if (save)
            initializer_backup[17408 + i] = record_kind[i];
        else
            record_kind[i] = initializer_backup[17408 + i];
        if (save)
            initializer_backup[18432 + i] = record_open[i];
        else
            record_open[i] = initializer_backup[18432 + i];
        if (save)
            initializer_backup[19456 + i] = record_first[i];
        else
            record_first[i] = initializer_backup[19456 + i];
        if (save)
            initializer_backup[20480 + i] = record_last[i];
        else
            record_last[i] = initializer_backup[20480 + i];
        if (save)
            initializer_backup[21504 + i] = arity[i];
        else
            arity[i] = initializer_backup[21504 + i];
    }
    for (i = 0; i < 131072; i++) {
        if (save)
            initializer_backup[22528 + i] = parameter_type[i];
        else
            parameter_type[i] = initializer_backup[22528 + i];
    }
    for (i = 0; i < 1024; i++) {
        if (save)
            initializer_backup[153600 + i] = file_function_type[i];
        else
            file_function_type[i] = initializer_backup[153600 + i];
    }
    if (save)
        initializer_state[0] = cursor;
    else
        cursor = initializer_state[0];
    if (save)
        initializer_state[1] = line;
    else
        line = initializer_state[1];
    if (save)
        initializer_state[2] = string_used;
    else
        string_used = initializer_state[2];
    if (save)
        initializer_state[3] = token;
    else
        token = initializer_state[3];
    if (save)
        initializer_state[4] = value;
    else
        value = initializer_state[4];
    if (save)
        initializer_state[5] = literal_type;
    else
        literal_type = initializer_state[5];
    if (save)
        initializer_state[6] = literal_high;
    else
        literal_high = initializer_state[6];
    if (save)
        initializer_state[7] = literal_string_start;
    else
        literal_string_start = initializer_state[7];
    if (save)
        initializer_state[8] = literal_string_length;
    else
        literal_string_length = initializer_state[8];
    if (save)
        initializer_state[9] = joined_string_start;
    else
        joined_string_start = initializer_state[9];
    if (save)
        initializer_state[10] = joined_string_length;
    else
        joined_string_length = initializer_state[10];
    if (save)
        initializer_state[11] = local_count;
    else
        local_count = initializer_state[11];
    if (save)
        initializer_state[12] = tag_count;
    else
        tag_count = initializer_state[12];
    if (save)
        initializer_state[13] = array_count;
    else
        array_count = initializer_state[13];
    if (save)
        initializer_state[14] = enum_count;
    else
        enum_count = initializer_state[14];
    if (save)
        initializer_state[15] = record_count;
    else
        record_count = initializer_state[15];
    if (save)
        initializer_state[16] = member_count;
    else
        member_count = initializer_state[16];
    if (save)
        initializer_state[17] = local_bytes;
    else
        local_bytes = initializer_state[17];
    if (save)
        initializer_state[18] = frame_peak;
    else
        frame_peak = initializer_state[18];
    if (save)
        initializer_state[19] = decl_storage;
    else
        decl_storage = initializer_state[19];
    if (save)
        initializer_state[20] = decl_typedef;
    else
        decl_typedef = initializer_state[20];
    if (save)
        initializer_state[21] = decl_name;
    else
        decl_name = initializer_state[21];
    if (save)
        initializer_state[22] = decl_count;
    else
        decl_count = initializer_state[22];
    if (save)
        initializer_state[23] = decl_depth;
    else
        decl_depth = initializer_state[23];
    if (save)
        initializer_state[24] = decl_parentheses;
    else
        decl_parentheses = initializer_state[24];
    if (save)
        initializer_state[25] = declarator_active;
    else
        declarator_active = initializer_state[25];
    if (save)
        initializer_state[26] = initializer_depth;
    else
        initializer_depth = initializer_state[26];
    if (save)
        initializer_state[27] = initializer_pending;
    else
        initializer_pending = initializer_state[27];
    if (save)
        initializer_state[28] = initializer_probe;
    else
        initializer_probe = initializer_state[28];
    if (save)
        initializer_state[29] = inferred_elements;
    else
        inferred_elements = initializer_state[29];
    if (save)
        initializer_state[30] = record_depth;
    else
        record_depth = initializer_state[30];
    if (save)
        initializer_state[31] = expression_depth;
    else
        expression_depth = initializer_state[31];
    if (save)
        initializer_state[32] = unary_depth;
    else
        unary_depth = initializer_state[32];
    if (save)
        initializer_state[33] = unevaluated_depth;
    else
        unevaluated_depth = initializer_state[33];
    if (save)
        initializer_state[34] = ce_depth;
    else
        ce_depth = initializer_state[34];
    if (save)
        initializer_state[35] = ce_type;
    else
        ce_type = initializer_state[35];
    if (save)
        initializer_state[36] = expression_type;
    else
        expression_type = initializer_state[36];
    if (save)
        initializer_state[37] = sizeof_value;
    else
        sizeof_value = initializer_state[37];
    if (save)
        initializer_state[38] = expression_zero;
    else
        expression_zero = initializer_state[38];
    if (save)
        initializer_state[39] = expression_load;
    else
        expression_load = initializer_state[39];
    if (save)
        initializer_state[40] = expression_array_bytes;
    else
        expression_array_bytes = initializer_state[40];
    if (save)
        initializer_state[41] = expression_array_type;
    else
        expression_array_type = initializer_state[41];
    if (save)
        initializer_state[42] = expression_register;
    else
        expression_register = initializer_state[42];
    if (save)
        initializer_state[43] = call_count;
    else
        call_count = initializer_state[43];
    if (save)
        initializer_state[44] = object_ref_count;
    else
        object_ref_count = initializer_state[44];
    if (save)
        initializer_state[45] = position;
    else
        position = initializer_state[45];
    if (save)
        initializer_state[46] = decl_result_name;
    else
        decl_result_name = initializer_state[46];
    if (save)
        initializer_state[47] = function_type_count;
    else
        function_type_count = initializer_state[47];
    return 0;
}

int infer_aggregate_array(int t, int id) {
    int start = position, old_members = member_count, n, i;
    unsigned long long saved_value = ce_value;
    if (initializer_probe)
        return error();
    initializer_checkpoint(1);
    initializer_probe = 1;
    initializer_pending = 0;
    inferred_elements = 0;
    local_offset[id] = -2;
    local_type[id] = t;
    local_depth[id] = block_depth;
    alias_type[id] = 0;
    register_object[id] = 0;
    expect('=');
    initialize_object(t, 0, 0, 0);
    n = inferred_elements;
    for (i = old_members + 1; i <= member_count; i++)
        member_next[i] = 0;
    rewind_code(start);
    initializer_checkpoint(0);
    ce_value = saved_value;
    if (failed || !n)
        return error();
    return make_array(array_base[(t - 4096) / 256], n);
}

int main(void) {
    int c, i, total, id;
    initialize();
    allocation = 0x600000;
    symbol_flags[6] = 6;
    arity[6] = 0;
    global_type[6] = 1;
    symbol_flags[10] = 5;
    symbol_flags[11] = 5;
    arity[11] = 1;
    global_type[10] = 1;
    global_type[11] = 1;
    parameter_type[1408] = 1;
    file_function_type[6] = function_entity_signature(6);
    file_function_type[10] = function_entity_signature(10);
    file_function_type[11] = function_entity_signature(11);
    while ((c = __c0_getchar()) != -1) {
        if (size == 524288)
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
        else {
            if (call_arity[i] >= 0 && call_arity[i] != arity[id])
                error();
            patch(call_position[i], function_address[id]);
        }
    }
    finish_objects();
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

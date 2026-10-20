/* Included after the existing compiler backend. C1O1 output uses the same
   byte image buffer and stream writer as direct PE output. */
int object_symbol_map[2048];
int object_symbol_ids[2051];

int object_quad(int at, int value) {
    write32(at, value);
    return write32(at + 4, 0);
}

int object_symbol_record(int at, int name_offset, int binding, int section, int value, int size,
                         int alignment) {
    write32(at, name_offset);
    write32(at + 4, binding);
    write32(at + 8, section);
    object_quad(at + 16, value);
    object_quad(at + 24, size);
    object_quad(at + 32, alignment);
    return 0;
}

int object_fixup_record(int at, int symbol, int offset, int addend) {
    write32(at, 0);
    write32(at + 4, symbol);
    write32(at + 8, 3);
    object_quad(at + 16, offset);
    write32(at + 24, addend);
    write32(at + 28, addend < 0 ? -1 : 0);
    return 0;
}

int object_prepare(void) {
    int i, j, n = 2, refs, code_size, so = 128, fo, no, po, total, cursor, at, id, binding, section,
              value, width, align, target, ordinary;
    if (initializer_tail) {
        patch(initializer_tail, position);
        emit(0xc3);
    }
    if (position == 4096)
        emit(0xc3);
    code_size = position - 4096;
    refs = call_count + object_ref_count + address_ref_count;
    if (refs > 16384)
        return error_message("object relocation capacity exceeded");
    for (i = 0; i < count; i++) {
        object_symbol_map[i] = -1;
        if ((symbol_flags[i] & 8) || file_function_type[i] || (symbol_flags[i] & 1)) {
            object_symbol_map[i] = n;
            object_symbol_ids[n++] = i;
        }
    }
    ordinary = n;
    if (initializer_start)
        n++;
    fo = so + n * 40;
    no = fo + refs * 32;
    cursor = 11;
    for (i = 2; i < ordinary; i++)
        cursor += lengths[object_symbol_ids[i]] + 1;
    if (initializer_start)
        cursor += 6;
    po = no + cursor;
    total = po + code_size;
    if (total > 1049088)
        return error_message("object output capacity exceeded");
    for (i = 0; i < call_count; i++) {
        id = call_symbol[i];
        if (object_symbol_map[id] < 0)
            return error_message("object call has no symbol");
        if (call_arity[i] >= 0 && !arity_matches(arity[id], call_arity[i]))
            return error();
        if (call_position[i] < 4096 || call_position[i] > position - 4)
            return error();
    }
    for (i = 0; i < object_ref_count; i++)
        if (object_symbol_map[object_ref_symbol[i]] < 0 || object_ref_position[i] < 4096 ||
            object_ref_position[i] > position - 4)
            return error();
    for (i = 0; i < address_ref_count; i++) {
        target = address_ref_target[i];
        if (!((target >= 0x401e00 && target <= position + 0x400e00) ||
              (target >= 0x600000 && target <= allocation)))
            return error_message("object references an unsupported fixed runtime address");
        if (address_ref_position[i] < 4096 || address_ref_position[i] > position - 4)
            return error();
    }
    if (po > 4096) {
        for (i = code_size - 1; i >= 0; i--)
            image[po + i] = image[4096 + i];
    } else {
        for (i = 0; i < code_size; i++)
            image[po + i] = image[4096 + i];
    }
    for (i = 0; i < po; i++)
        image[i] = 0;
    write32(0, 0x314f3143);
    write32(4, 0x8664);
    write32(8, 2);
    write32(12, n);
    write32(16, refs);
    write32(20, cursor);
    write32(24, code_size);
    object_quad(32, code_size);
    object_quad(40, code_size);
    object_quad(48, 16);
    object_quad(56, 1);
    write32(72, 5);
    object_quad(80, allocation > 0x600000 ? allocation - 0x600000 : 1);
    object_quad(96, 16);
    object_quad(104, 1);
    write32(120, 3);
    object_symbol_record(so, 0, 1, 0, 0, 0, 1);
    object_symbol_record(so + 40, 6, 1, 1, 0, 0, 1);
    image[no] = '$';
    image[no + 1] = 'c';
    image[no + 2] = 'o';
    image[no + 3] = 'd';
    image[no + 4] = 'e';
    image[no + 6] = '$';
    image[no + 7] = 'b';
    image[no + 8] = 's';
    image[no + 9] = 's';
    cursor = 11;
    for (i = 2; i < ordinary; i++) {
        id = object_symbol_ids[i];
        binding = 0;
        section = -1;
        value = 0;
        width = 0;
        align = 1;
        if (symbol_flags[id] & 8) {
            if (global_address[id]) {
                width = global_extent[id];
                align = type_align(global_type[id]);
                if ((symbol_flags[id] & 32) || global_initialized[id]) {
                    binding = (symbol_flags[id] & 32) ? 1 : 2;
                    section = 1;
                    value = global_address[id] - 0x600000;
                } else
                    binding = 3;
            }
        } else if (symbol_flags[id] & 1) {
            binding = (symbol_flags[id] & 32) ? 1 : 2;
            section = 0;
            value = function_address[id] - 4096;
        }
        object_symbol_record(so + i * 40, cursor, binding, section, value, width, align);
        for (j = 0; j < lengths[id]; j++)
            image[no + cursor + j] = names[id * 64 + j];
        cursor += lengths[id] + 1;
    }
    if (initializer_start) {
        object_symbol_record(so + ordinary * 40, cursor, 1, 0, initializer_start - 4096, 0, 1);
        image[no + cursor] = '$';
        image[no + cursor + 1] = 'i';
        image[no + cursor + 2] = 'n';
        image[no + cursor + 3] = 'i';
        image[no + cursor + 4] = 't';
    }
    at = fo;
    for (i = 0; i < call_count; i++) {
        object_fixup_record(at, object_symbol_map[call_symbol[i]], call_position[i] - 4096, 0);
        at += 32;
    }
    for (i = 0; i < object_ref_count; i++) {
        object_fixup_record(at, object_symbol_map[object_ref_symbol[i]],
                            object_ref_position[i] - 4096, 0);
        at += 32;
    }
    for (i = 0; i < address_ref_count; i++) {
        target = address_ref_target[i];
        object_fixup_record(at, target >= 0x600000 ? 1 : 0, address_ref_position[i] - 4096,
                            target - (target >= 0x600000 ? 0x600000 : 0x401e00));
        at += 32;
    }
    return total;
}

int configure_object(void) {
    PP_Environment *environment = *(PP_Environment **)0x401070;
    char setting[16];
    unsigned int n = environment("C1_OUTPUT", setting, 16);
    if (!n)
        return 0;
    if (n == 6 && setting[0] == 'o' && setting[1] == 'b' && setting[2] == 'j' &&
        setting[3] == 'e' && setting[4] == 'c' && setting[5] == 't') {
        object_mode = 1;
        return 0;
    }
    return error_message("C1_OUTPUT must be object or unset");
}

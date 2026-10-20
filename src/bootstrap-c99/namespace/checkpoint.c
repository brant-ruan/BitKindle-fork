int main(void) {
    int mask;
    for (mask = 0; mask < 16; mask++) {
        literal_string_wide = mask & 1;
        joined_string_wide = (mask >> 1) & 1;
        decl_inline = (mask >> 2) & 1;
        initializer_designated = (mask >> 3) & 1;
        initializer_checkpoint(1);
        if (failed)
            return 10;
        literal_string_wide = 0;
        joined_string_wide = 1;
        decl_inline = 1;
        initializer_designated = 0;
        initializer_checkpoint(1);
        if (failed)
            return 11;
        literal_string_wide = 0;
        joined_string_wide = 0;
        decl_inline = 0;
        initializer_designated = 0;
        initializer_checkpoint(0);
        if (literal_string_wide != 0 || joined_string_wide != 1 || decl_inline != 1 ||
            initializer_designated != 0)
            return 1;
        initializer_checkpoint(0);
        if (literal_string_wide != (mask & 1) || joined_string_wide != ((mask >> 1) & 1) ||
            decl_inline != ((mask >> 2) & 1) || initializer_designated != ((mask >> 3) & 1))
            return 2;
        if (initializer_checkpoint_depth)
            return 3;
    }
    return 0;
}

inline int shared(void) {
    return 13;
}

int (*from_a(void))(void) {
    return shared;
}

int call_a(void) {
    return shared();
}

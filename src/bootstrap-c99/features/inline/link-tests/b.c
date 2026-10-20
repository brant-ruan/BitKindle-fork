inline int shared(void) {
    return 17;
}

int (*from_b(void))(void) {
    return shared;
}

int call_b(void) {
    return shared();
}

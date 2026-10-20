/* Windows x64 ABI inspection: the stub calls a function with RCX holding
   the caller's aggregate return buffer and checks its returned RAX.
   The literal lives in C1's executable .text section. Its bytes are source
   text compiled by C1; no external assembler or binary patch is used.

   push rbx; mov rbx,rcx; sub rsp,32; call rdx; cmp rax,rbx;
   setne al; movzx eax,al; add rsp,32; pop rbx; ret
*/
struct S {
    long long a, b, c;
};
volatile struct S source = {11, 22, 33};

struct S ordinary(void) {
    struct S s = {44, 55, 66};
    return s;
}

struct S qualified(void) {
    return source;
}

int main(void) {
    int (*probe)(void *, void *);
    struct S a, b;
    probe = (int (*)(void *, void *)) "\x53\x48\x89\xcb\x48\x83\xec\x20\xff\xd2\x48\x39\xd8"
                                      "\x0f\x95\xc0\x0f\xb6\xc0\x48\x83\xc4\x20\x5b\xc3";
    if (probe(&a, (void *)ordinary))
        return 1;
    if (a.a != 44 || a.b != 55 || a.c != 66)
        return 2;
    if (probe(&b, (void *)qualified))
        return 3;
    if (b.a != 11 || b.b != 22 || b.c != 33)
        return 4;
    return 0;
}

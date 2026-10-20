typedef float _Complex F;
typedef double _Complex D;
typedef long double _Complex L;

struct Box {
    int tag;
    D value;
};

extern D global_value;
extern F global_array[3];
F float_echo(F x);
D double_echo(D x);
L long_echo(L x);
D make_value(double real, double imaginary);
double imaginary(D x);
D many(int a, D b, int c, F d, L e, D f);
D varied(int count, ...);
struct Box box_echo(struct Box b);
void change(D *p);

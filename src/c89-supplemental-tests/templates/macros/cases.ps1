$cases = [ordered]@{
    recursive = @(@'
#define val 3
#define fun(a) fun(val * (a))
#undef val
#define val 2
#define alias fun
#define rec rec[0]
#define id(a) a
#define TEXT(a) #a
#define EXPAND(a) TEXT(a)
int equal(const char *a,const char *b){while(*a||*b){if(*a==' '){a++;continue;}if(*b==' '){b++;continue;}if(*a++!=*b++)return 0;}return 1;}
int main(void){return !equal(EXPAND(fun(y+1)+fun(fun(rec))%id(id(alias)(0)+id)(1)),"fun(2*(y+1))+fun(2*(fun(2*(rec[0]))))%fun(2*(0))+id(1)");}
'@, 0, $false)
    paste = @(@'
#define CAT(a,b) a##b
#define XCAT(a,b) CAT(a,b)
#define TOPBOTTOM "hello"
#define BOTTOM BOTTOM ", world"
int equal(const char *a,const char *b){while(*a&&*a==*b){a++;b++;}return *a==*b;}
int main(void){return !equal(CAT(TOP,BOTTOM),"hello")||!equal(XCAT(TOP,BOTTOM),"hello, world");}
'@, 0, $false)
    redefine = @(@'
#define VALUE (21+21)
#define VALUE /* before */ (21+21) /* after */
#define ID(a) ( a )
#define ID( a )( /* before */ a /* after */ )
int main(void){return ID(VALUE)!=42;}
'@, 0, $false)
}

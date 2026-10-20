$prefix = '#include "src/bootstrap-c99/base/lib/math-classify.c"' + [char]10 + '#include "src/bootstrap-c99/base/lib/runtime.c"' + [char]10
MathCheck 'math-quiet-comparison' ($prefix + @'
int test(float a,long double b,int order){
 if(!!isless(a,b)!=(order==0))return 1;
 if(!!islessequal(a,b)!=(order<=1))return 2;
 if(!!isgreater(a,b)!=(order==2))return 3;
 if(!!isgreaterequal(a,b)!=(order==1||order==2))return 4;
 if(!!islessgreater(a,b)!=(order==0||order==2))return 5;
 if(!!isunordered(a,b)!=(order==3))return 6;
 return 0;
}
int main(void){
 union{float f;unsigned u;}a;
 union{long double f;unsigned long long u;}b;
 fenv_t saved;unsigned control;int mode;
 fegetenv(&saved);
 for(mode=0;mode<4;mode++){
  control=0x9fe5U|((unsigned)mode<<13);__c1_fe_write(control);
  if(test(-2.0f,-1.0L,0)||test(2.0f,1.0L,2)||test(1.0f,1.0L,1))return 1;
  if(test(-0.0f,0.0L,1))return 2;
  a.u=1;b.u=0;if(test(a.f,b.f,2))return 3;
  b.f=0x1p-149L;if(test(a.f,b.f,1))return 4;
  a.u=0x7f800001;if(test(a.f,b.f,3))return 5;
  a.f=1;b.u=0x7ff0000000000001ULL;if(test(a.f,b.f,3))return 6;
  a.u=0xff800000;b.u=0xfff0000000000000ULL;if(test(a.f,b.f,1))return 7;
  b.u=0x7ff0000000000000ULL;if(test(a.f,b.f,0))return 8;
  if(__c1_fe_read()!=control)return 9;
 }
 fesetenv(&saved);return 0;
}
'@)
MathCheck 'math-comparison-evaluation' ($prefix + @'
int left_calls,right_calls;
float left(void){left_calls++;return 1.0f;}
double right(void){right_calls++;return 2.0;}
int main(void){
 if(!isless(left(),right())||left_calls!=1||right_calls!=1)return 1;
 if(!islessequal(left(),right())||left_calls!=2||right_calls!=2)return 2;
 if(isgreater(left(),right())||left_calls!=3||right_calls!=3)return 3;
 if(isgreaterequal(left(),right())||left_calls!=4||right_calls!=4)return 4;
 if(!islessgreater(left(),right())||left_calls!=5||right_calls!=5)return 5;
 if(isunordered(left(),right())||left_calls!=6||right_calls!=6)return 6;
 return 0;
}
'@)

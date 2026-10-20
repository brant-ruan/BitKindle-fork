$header = '#include "src/bootstrap-c99/base/include/float.h"' + [char]10
FloatingCheck 'float-header-values' ($header + @'
#include "src/bootstrap-c99/base/include/float.h"
#if FLT_RADIX!=2 || FLT_EVAL_METHOD!=0 || DECIMAL_DIG!=17
#error evaluation
#endif
#if FLT_MANT_DIG!=24 || FLT_DIG!=6 || FLT_MIN_EXP!=-125 || FLT_MAX_EXP!=128 || FLT_MIN_10_EXP!=-37 || FLT_MAX_10_EXP!=38
#error float
#endif
#if DBL_MANT_DIG!=53 || DBL_DIG!=15 || DBL_MIN_EXP!=-1021 || DBL_MAX_EXP!=1024 || DBL_MIN_10_EXP!=-307 || DBL_MAX_10_EXP!=308
#error double
#endif
#if LDBL_MANT_DIG!=53 || LDBL_DIG!=15 || LDBL_MIN_EXP!=-1021 || LDBL_MAX_EXP!=1024 || LDBL_MIN_10_EXP!=-307 || LDBL_MAX_10_EXP!=308
#error longdouble
#endif
float fs[]={FLT_MIN,FLT_MAX,FLT_EPSILON};
double ds[]={DBL_MIN,DBL_MAX,DBL_EPSILON};
long double ls[]={LDBL_MIN,LDBL_MAX,LDBL_EPSILON};
int main(void){
 union{float f;unsigned u;}f;
 union{double d;unsigned long long u;}d;
 union{long double d;unsigned long long u;}l;
 f.f=fs[0];if(f.u!=0x00800000)return 1;
 f.f=fs[1];if(f.u!=0x7f7fffff)return 2;
 f.f=fs[2];if(f.u!=0x34000000)return 3;
 d.d=ds[0];if(d.u!=0x0010000000000000ULL)return 4;
 d.d=ds[1];if(d.u!=0x7fefffffffffffffULL)return 5;
 d.d=ds[2];if(d.u!=0x3cb0000000000000ULL)return 6;
 l.d=ls[0];if(l.u!=0x0010000000000000ULL)return 7;
 l.d=ls[1];if(l.u!=0x7fefffffffffffffULL)return 8;
 l.d=ls[2];if(l.u!=0x3cb0000000000000ULL)return 9;
 if(sizeof(FLT_MIN)!=4||sizeof(FLT_MAX)!=4||sizeof(FLT_EPSILON)!=4)return 10;
 if(sizeof(DBL_MIN)!=8||sizeof(DBL_MAX)!=8||sizeof(DBL_EPSILON)!=8)return 11;
 if(sizeof(LDBL_MIN)!=8||sizeof(LDBL_MAX)!=8||sizeof(LDBL_EPSILON)!=8)return 12;
 return FLT_ROUNDS!=1;
}
'@)
FloatingCheck 'float-header-rounding' ($header + @'
int main(void){
 unsigned int(*get)(void)=(unsigned int(*)(void))"\x0f\xae\x5c\x24\x08\x8b\x44\x24\x08\xc3";
 void(*set)(unsigned int)=(void(*)(unsigned int))"\x89\x4c\x24\x08\x0f\xae\x54\x24\x08\xc3";
 unsigned int saved=get(),mode;
 int i,expected,result=0;
 for(i=0;i<4;i++){
  mode=(saved&~0x6000U)|((unsigned)i<<13);set(mode);
  expected=i==0?1:(i==1?3:(i==2?2:0));
  if(FLT_ROUNDS!=expected||get()!=mode)result=1;
 }
 set(saved);return result;
}
'@)
FloatingCheck 'float-header-evaluation' ($header + @'
float ff(float x){return x+FLT_EPSILON/2;}
double dd(double x){return x+DBL_EPSILON/2;}
int main(void){
 volatile float f=1.0f;
 volatile double d=1.0;
 float a=(f+FLT_EPSILON/2)-f;
 double b=(d+DBL_EPSILON/2)-d;
 return a!=0||b!=0||ff(f)!=f||dd(d)!=d||
  (FLT_MIN/2)!=0x1p-127f||(DBL_MIN/2)!=0x1p-1023;
}
'@)

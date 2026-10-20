$header = '#include "src/bootstrap-c99/base/lib/fenv.c"' + [char]10 + '#include "src/bootstrap-c99/base/include/float.h"' + [char]10
FloatingCheck 'fenv-rounding' ($header + @'
#pragma STDC FENV_ACCESS ON
int main(void){
 int modes[4]={FE_TONEAREST,FE_DOWNWARD,FE_UPWARD,FE_TOWARDZERO};
 int expected[4]={1,3,2,0},i;
 fenv_t saved;unsigned control;
 union{float f;unsigned u;}a,b;
 volatile float one=1.0f,half=0x1p-24f;
 if(fegetenv(&saved))return 1;
 for(i=0;i<4;i++){
  if(fesetround(modes[i])||fegetround()!=modes[i]||FLT_ROUNDS!=expected[i])return 2;
  a.f=one+half;b.f=-one-half;
  if(a.u!=(i==2?0x3f800001U:0x3f800000U))return 3;
  if(b.u!=(i==1?0xbf800001U:0xbf800000U))return 4;
  control=__c1_fe_read();
  if(!fesetround(12345)||__c1_fe_read()!=control)return 5;
 }
 if(fesetenv(&saved)||__c1_fe_read()!=saved.__mxcsr)return 6;
 return 0;
}
'@)
FloatingCheck 'fenv-exception-subsets' ($header + @'
int main(void){
 int mode,mask,expected;fexcept_t flags;fenv_t saved;
 fegetenv(&saved);
 for(mode=0;mode<4;mode++){
  fesetround(mode<<13);
  for(mask=0;mask<=FE_ALL_EXCEPT;mask++){
   if(mask&2)continue;
   if(feclearexcept(FE_ALL_EXCEPT)||fetestexcept(FE_ALL_EXCEPT))return 1;
   if(feraiseexcept(mask))return 2;
   expected=mask;if(mask&(FE_OVERFLOW|FE_UNDERFLOW))expected|=FE_INEXACT;
   if(fetestexcept(FE_ALL_EXCEPT)!=expected)return 3;
   if(fegetexceptflag(&flags,FE_ALL_EXCEPT))return 4;
   if(feclearexcept(FE_INVALID|FE_DIVBYZERO))return 5;
   if(fetestexcept(FE_ALL_EXCEPT)!=(expected&~(FE_INVALID|FE_DIVBYZERO)))return 6;
   if(fesetexceptflag(&flags,FE_INVALID|FE_DIVBYZERO))return 7;
   if(fetestexcept(FE_ALL_EXCEPT)!=expected||fegetround()!=(mode<<13))return 8;
  }
 }
 fesetenv(&saved);return 0;
}
'@)
FloatingCheck 'fenv-hold-update' ($header + @'
int main(void){
 fenv_t original,held,check;fexcept_t empty;
 fegetenv(&original);fesetenv(FE_DFL_ENV);
 if(fegetround()!=FE_TONEAREST||fetestexcept(FE_ALL_EXCEPT)||FLT_ROUNDS!=1)return 1;
 fegetexceptflag(&empty,FE_ALL_EXCEPT);
 fesetround(FE_DOWNWARD);feraiseexcept(FE_INEXACT);
 if(feholdexcept(&held))return 2;
 if(fetestexcept(FE_ALL_EXCEPT)||fegetround()!=FE_DOWNWARD)return 3;
 fegetenv(&check);
 if((check.__mxcsr&0x1f80U)!=0x1f80U||(check.__mxcsr&0x3fU))return 4;
 feraiseexcept(FE_INVALID);fesetround(FE_UPWARD);
 if(feupdateenv(&held)||fegetround()!=FE_DOWNWARD)return 5;
 if(fetestexcept(FE_ALL_EXCEPT)!=(FE_INVALID|FE_INEXACT))return 6;
 if(fesetexceptflag(&empty,FE_INVALID)||fetestexcept(FE_ALL_EXCEPT)!=FE_INEXACT)return 7;
 if(fesetenv(&held)||fetestexcept(FE_ALL_EXCEPT)!=FE_INEXACT)return 8;
 if(feupdateenv(FE_DFL_ENV)||fegetround()!=FE_TONEAREST||fetestexcept(FE_ALL_EXCEPT)!=FE_INEXACT)return 9;
 fesetenv(&original);return 0;
}
'@)
FloatingCheck 'fenv-real-operations' ($header + @'
#pragma STDC FENV_ACCESS ON
int main(void){
 volatile double zero=0.0,one=1.0,three=3.0,max=DBL_MAX,min=DBL_MIN;
 double result;fenv_t saved;
 feholdexcept(&saved);
 result=one/zero;if(fetestexcept(FE_ALL_EXCEPT)!=FE_DIVBYZERO)return 1;
 feclearexcept(FE_ALL_EXCEPT);
 result=zero/zero;if(fetestexcept(FE_ALL_EXCEPT)!=FE_INVALID)return 2;
 feclearexcept(FE_ALL_EXCEPT);
 result=max*three;if(fetestexcept(FE_ALL_EXCEPT)!=(FE_OVERFLOW|FE_INEXACT))return 3;
 feclearexcept(FE_ALL_EXCEPT);
 result=min*min;if(fetestexcept(FE_ALL_EXCEPT)!=(FE_UNDERFLOW|FE_INEXACT))return 4;
 feclearexcept(FE_ALL_EXCEPT);
 result=one/three;if(fetestexcept(FE_ALL_EXCEPT)!=FE_INEXACT)return 5;
 fesetenv(&saved);return 0;
}
'@)

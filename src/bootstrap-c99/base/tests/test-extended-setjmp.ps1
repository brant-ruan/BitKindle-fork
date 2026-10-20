$prefix = '#include "src/bootstrap-c99/base/lib/setjmp.c"' + [char]10
ExtendedCheck 'setjmp-constant-left' ($prefix + 'jmp_buf env;int attempts;int main(void){if(7==setjmp(env))return 0;if(attempts++)return 1;longjmp(env,7);return 2;}')
ExtendedCheck 'setjmp-wide-left-and-argument' ($prefix + @'
jmp_buf env;int calls,attempts;
unsigned long long *choose(void){calls++;return env;}
int main(void){if(~0ULL==setjmp(choose()))return calls!=1;if(attempts++)return 1;longjmp(env,-1);return 2;}
'@)
ExtendedCheck 'setjmp-stack-argument-clobber' ($prefix + @'
jmp_buf env;volatile int result;int attempts;
int many(int a,int b,int c,int d,int e,int f,int g,int h,int i){return a+b+c+d+e+f+g+h+i;}
int main(void){
 volatile int pad[4096];pad[0]=1;
 if(23==setjmp(env))return result!=45||pad[0]!=77;
 if(attempts++)return 1;
 result=many(1,2,3,4,5,6,7,8,9);pad[0]=77;longjmp(env,23);return 2;
}
'@)
ExtendedCheck 'setjmp-not-context' ($prefix + 'jmp_buf env;int calls;int main(void){if(!setjmp(env)){calls++;longjmp(env,2);}return calls!=1;}')
foreach ($value in @(0, 1, 7, -1, -2147483648, 2147483647)) {
    $expected = if ($value -eq 0) {
        1
    }
    else {
        $value
    }
    ExtendedCheck "setjmp-value-$value" ($prefix + "jmp_buf env;void jump(void){longjmp(env,$value);}int main(void){if(setjmp(env)==$expected)return 0;jump();return 1;}")
}
ExtendedCheck 'setjmp-recursive-stack' ($prefix + @'
jmp_buf target;int visits;
void descend(int n){volatile int pad[1000];pad[0]=n;visits++;if(n)descend(n-1);else longjmp(target,19);visits+=pad[0]+1000;}
int main(void){
 volatile int local=41;
 if(setjmp(target)==19){if(visits!=41||local!=73)return 1;return 0;}
 local=73;descend(40);return 2;
}
'@)
ExtendedCheck 'setjmp-nested-buffers' ($prefix + @'
jmp_buf outer,inner;int phase;
void child(void){
 if(setjmp(inner)==3){phase=2;longjmp(outer,4);}
 phase=1;longjmp(inner,3);
}
int main(void){if(setjmp(outer)==4)return phase!=2;child();return 1;}
'@)
ExtendedCheck 'setjmp-repeat' ($prefix + @'
jmp_buf env;volatile int count;
int main(void){
 switch(setjmp(env)){
 case 0:count=1;longjmp(env,2);
 case 2:count++;longjmp(env,3);
 case 3:return count!=2;
 default:return 1;
 }
}
'@)
ExtendedCheck 'setjmp-floating-environment' ($prefix + @'
#include "src/bootstrap-c99/base/lib/runtime.c"
jmp_buf env;
static fenv_t original,mode,after;
int main(void){
 fegetenv(&original);mode.__mxcsr=0x1f80U;fesetenv(&mode);
 if(setjmp(env)==5){
  fegetenv(&after);fesetenv(&original);return after.__mxcsr!=0xbfc4U;
 }
 mode.__mxcsr=0xbfc4U;fesetenv(&mode);
 longjmp(env,5);return 2;
}
'@)
$probe = @'
jmp_buf before,after;
void resume(void){longjmp(before,11);}
int main(void){
 int i;void (*poison)(void (*)(void));
 if(setjmp(before)==11){
  setjmp(after);
  for(i=2;i<10;i++)if(before[i]!=after[i])return i;
  for(i=12;i<32;i++)if(before[i]!=after[i])return i;
  return 0;
 }
 /* Complement every bit so that an omitted restore is always observable.
    Preserve RCX callback in volatile R11, then allocate ABI call home space. */
 poison=(void (*)(void (*)(void)))
  "\x49\x89\xcb"
  "\x48\x83\xf3\xff\x48\x83\xf5\xff\x48\x83\xf6\xff\x48\x83\xf7\xff"
  "\x49\x83\xf4\xff\x49\x83\xf5\xff\x49\x83\xf6\xff\x49\x83\xf7\xff"
  "\x66\x0f\x76\xc0\x66\x0f\xef\xf0\x66\x0f\xef\xf8"
  "\x66\x44\x0f\xef\xc0\x66\x44\x0f\xef\xc8\x66\x44\x0f\xef\xd0"
  "\x66\x44\x0f\xef\xd8\x66\x44\x0f\xef\xe0\x66\x44\x0f\xef\xe8"
  "\x66\x44\x0f\xef\xf0\x66\x44\x0f\xef\xf8"
  "\x48\x83\xec\x28\x41\xff\xd3\x0f\x0b";
 poison(resume);return 40;
}
'@
ExtendedCheck 'setjmp-independent-registers' ($prefix + $probe)
# Controls edit C source bytes before the custom compiler runs; no EXE patching.
$restore = [IO.File]::ReadAllText((Join-Path $PSScriptRoot '../lib/setjmp.c')).Replace('#include "setjmp.h"', '#include "src/bootstrap-c99/base/include/setjmp.h"')
ExtendedCheck 'setjmp-probe-detects-r12' ($restore.Replace('\x4c\x8b\x61\x30', '\x90\x90\x90\x90') + [char]10 + $probe) 6
ExtendedCheck 'setjmp-probe-detects-xmm8' ($restore.Replace('\xf3\x44\x0f\x6f\x81\x80\x00\x00\x00', '\x90\x90\x90\x90\x90\x90\x90\x90\x90') + [char]10 + $probe) 16
ExtendedCheck 'setjmp-probe-detects-pending-operand' ($restore.Replace('\x48\x8b\x41\x58\x49\x89\x02', '\x90\x90\x90\x90\x90\x90\x90') + [char]10 + 'jmp_buf env;int attempts;int main(void){if(7==setjmp(env))return 0;if(attempts++)return 1;longjmp(env,7);return 2;}') 1

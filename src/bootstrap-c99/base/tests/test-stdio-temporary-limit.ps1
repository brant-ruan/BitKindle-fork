$savedTmp = $env:TMP
try {
    $env:TMP = $ScratchDirectory
    Check 'tmpnam-full-promised-limit' @'
#include "src/bootstrap-c99/base/lib/runtime.c"
int main(void){char current[L_tmpnam],previous[L_tmpnam];int i;
previous[0]=0;
for(i=0;i<TMP_MAX;i++){
 if(!tmpnam(current))return 1;
 /* Fixed-width monotonic serial gives a strict ordering, proving that no
    earlier returned name repeats, without a quadratic comparison table. */
 if(strcmp(current,previous)<=0)return 2;
 strcpy(previous,current);
}
return 0;}
'@
}
finally {
    $env:TMP = $savedTmp 
}

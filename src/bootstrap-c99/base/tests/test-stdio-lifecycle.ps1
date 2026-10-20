$prefix = '#include "src/bootstrap-c99/base/lib/runtime.c"' + [char]10
Check 'exit-no-library-status' 'int main(void){return 173;}' 173
Check 'exit-main-status-handlers' ($prefix + @'
void a(void){__c0_putchar('A');}void b(void){__c0_putchar('B');}
int main(void){if(atexit(a)||atexit(b))return 1;return 37;}
'@) 37 $false '' 'BA'
Check 'exit-explicit-once' ($prefix + @'
void a(void){__c0_putchar('A');}void b(void){__c0_putchar('B');}
int main(void){if(atexit(a)||atexit(b))return 1;exit(-37);return 9;}
'@) -37 $false '' 'BA'
Check 'exit-immediate' ($prefix + @'
void a(void){__c0_putchar('A');}
int main(void){atexit(a);_Exit(53);return 9;}
'@) 53
Check 'exit-32-registrations' ($prefix + @'
int n;void a(void){n++;if(n==32)__c0_putchar('X');}
int main(void){int i;for(i=0;i<32;i++)if(atexit(a))return 1;
if(atexit(a)==0)return 2;return 0;}
'@) 0 $false '' 'X'
Check 'exit-final-stream-hook' ($prefix + @'
void a(void){__c0_putchar('A');}void last(void){__c0_putchar('Z');}
int main(void){atexit(a);__c1_exit_set_streams(last);return 7;}
'@) 7 $false '' 'AZ'
Check 'exit-empty-dispatch' ($prefix + @'
int main(void){__c1_exit_install();return 255;}
'@) 255

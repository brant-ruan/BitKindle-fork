$savedInclude = [Environment]::GetEnvironmentVariable('C1_INCLUDE', 'Process')
$savedStandard = [Environment]::GetEnvironmentVariable('C1_STANDARD', 'Process')
try {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../include')), 'Process')
    $runtime = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../lib/runtime.c'))
    $prefix = "#include <stdlib.h>`n#include <stdint.h>`n"
    $suffix = "`n#include `"$runtime`"`n"
    foreach ($mode in 'c89', 'c99') {
        [Environment]::SetEnvironmentVariable('C1_STANDARD', $mode, 'Process')
        $cases = @{
            'sizes-alignment' = 'int main(void){int n,i;unsigned char *p;for(n=1;n<4096;n++){p=malloc(n);if(!p||(uintptr_t)p%16)return 1;for(i=0;i<n;i++)p[i]=(unsigned char)i;for(i=0;i<n;i++)if(p[i]!=(unsigned char)i)return 2;free(p);}return 0;}'
            'calloc-zero' = 'int main(void){int i;unsigned char *p=calloc(127,37);if(!p||(uintptr_t)p%16)return 1;for(i=0;i<4699;i++)if(p[i])return 2;free(p);return 0;}'
            'calloc-overflow' = 'int main(void){return calloc(SIZE_MAX,2)!=NULL||calloc(2,SIZE_MAX)!=NULL||calloc(SIZE_MAX/3+1,3)!=NULL;}'
            'zero-policy' = 'int main(void){void *a=malloc(0),*b=calloc(0,SIZE_MAX),*c=calloc(SIZE_MAX,0),*d=realloc(NULL,0);if(!a||!b||!c||!d)return 1;free(a);free(b);free(c);if(realloc(d,0)!=NULL)return 2;free(NULL);return 0;}'
            'grow-shrink' = 'int main(void){unsigned char *p=malloc(128),*q;int i;if(!p)return 1;for(i=0;i<128;i++)p[i]=(unsigned char)(i+7);q=realloc(p,1048576);if(!q||(uintptr_t)q%16)return 2;for(i=0;i<128;i++)if(q[i]!=(unsigned char)(i+7))return 3;q[1048575]=99;p=realloc(q,64);if(!p||(uintptr_t)p%16)return 4;for(i=0;i<64;i++)if(p[i]!=(unsigned char)(i+7))return 5;free(p);return 0;}'
            'failure-preserves' = 'int main(void){unsigned char *p=malloc(64),*q;int i;if(!p)return 1;for(i=0;i<64;i++)p[i]=(unsigned char)i;q=realloc(p,SIZE_MAX);if(q)return 2;for(i=0;i<64;i++)if(p[i]!=(unsigned char)i)return 3;free(p);return malloc(SIZE_MAX)!=NULL;}'
            'function-pointers' = 'int main(void){void *(*a)(size_t)=malloc;void *(*c)(size_t,size_t)=calloc;void *(*r)(void *,size_t)=realloc;void (*f)(void *)=free;int n=16;void *p=a(n++),*q=c(2,n++);if(!p||!q||n!=18)return 1;p=r(p,32);if(!p)return 2;f(p);f(q);return 0;}'
            'flexible-array' = 'struct data {size_t length;int values[];};int main(void){struct data *p=malloc(sizeof(struct data)+200*sizeof(int));int i;if(!p)return 1;p->length=200;for(i=0;i<200;i++)p->values[i]=i*i;for(i=0;i<p->length;i++)if(p->values[i]!=i*i)return 2;free(p);return 0;}'
            'independent-blocks' = 'int main(void){unsigned char *p[128];int i,j;for(i=0;i<128;i++){p[i]=malloc(256);if(!p[i])return 1;for(j=0;j<256;j++)p[i][j]=(unsigned char)(i+j);}for(i=127;i>=0;i--){for(j=0;j<256;j++)if(p[i][j]!=(unsigned char)(i+j))return 2;free(p[i]);}return 0;}'
            'null-realloc' = 'int main(void){unsigned char *p=realloc(NULL,32);if(!p)return 1;p[31]=73;if(p[31]!=73)return 2;free(p);return 0;}'
        }
        foreach ($name in ($cases.Keys | Sort-Object)) {
            Check "memory-$mode-$name" ($prefix + $cases[$name] + $suffix) 
        }
    }
}
finally {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', $savedInclude, 'Process')
    [Environment]::SetEnvironmentVariable('C1_STANDARD', $savedStandard, 'Process')
}

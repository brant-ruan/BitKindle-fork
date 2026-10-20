$savedInclude = [Environment]::GetEnvironmentVariable('C1_INCLUDE', 'Process')
$savedStandard = [Environment]::GetEnvironmentVariable('C1_STANDARD', 'Process')
try {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../include')), 'Process')
    $runtime = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../lib/runtime.c'))
    $nl = [string][char]10
    $prefix = ('#include <string.h>', '#include <errno.h>', '#include <limits.h>', '#include <stdlib.h>', '') -join $nl
    $suffix = $nl + '#include "' + $runtime + '"' + $nl
    foreach ($mode in 'c89', 'c99') {
        [Environment]::SetEnvironmentVariable('C1_STANDARD', $mode, 'Process')
        $cases = @{
            'memcpy-lengths' = 'int main(void){unsigned char s[256],d[258];int n,i;for(i=0;i<256;i++)s[i]=(unsigned char)i;for(n=0;n<=256;n++){for(i=0;i<258;i++)d[i]=165;if(memcpy(d+1,s,n)!=d+1)return 1;for(i=0;i<258;i++)if(d[i]!=(i>0&&i<=n?(unsigned char)(i-1):165))return 2;}return 0;}'
            'memmove-overlaps' = 'int main(void){unsigned char a[32],before[32];int s,d,n,i,expected;for(s=0;s<32;s++)for(d=0;d<32;d++)for(n=0;n<=32-s&&n<=32-d;n++){for(i=0;i<32;i++){a[i]=(unsigned char)(i*37+11);before[i]=a[i];}if(memmove(a+d,a+s,n)!=a+d)return 1;for(i=0;i<32;i++){expected=i>=d&&i<d+n?before[s+i-d]:before[i];if(a[i]!=expected)return 2;}}return 0;}'
            'memmove-disjoint' = 'int main(void){unsigned char a[64],b[64];int i;for(i=0;i<64;i++){a[i]=(unsigned char)(i+128);b[i]=0;}if(memmove(b,a,64)!=b)return 1;for(i=0;i<64;i++)if(a[i]!=b[i])return 2;if(memmove(a,b,64)!=a)return 3;return 0;}'
            'memset-values' = 'int main(void){unsigned char a[66];int values[]={-1,0,128,256,511,INT_MIN},i,j,n;for(j=0;j<6;j++)for(n=0;n<=64;n++){for(i=0;i<66;i++)a[i]=37;if(memset(a+1,values[j],n)!=a+1)return 1;for(i=0;i<66;i++)if(a[i]!=(i>0&&i<=n?(unsigned char)values[j]:37))return 2;}return 0;}'
            'memcmp-all-byte-pairs' = 'int main(void){unsigned char a,b;int i,j,r;for(i=0;i<256;i++)for(j=0;j<256;j++){a=(unsigned char)i;b=(unsigned char)j;r=memcmp(&a,&b,1);if((r>0)!=(i>j)||(r<0)!=(i<j))return 1;}return 0;}'
            'memcmp-boundaries' = 'int main(void){unsigned char a[]={0,128,255,3},b[]={0,128,255,4};return memcmp(a,b,0)!=0||memcmp(a,b,3)!=0||memcmp(a,b,4)>=0||memcmp(b,a,4)<=0||memcmp(a,a,4)!=0;}'
            'memchr-all-bytes' = 'int main(void){unsigned char a[256];int i;for(i=0;i<256;i++)a[i]=(unsigned char)i;for(i=0;i<256;i++){if(memchr(a,i,256)!=a+i)return 1;if(memchr(a,i+256,256)!=a+i)return 2;if(memchr(a,i,i)!=NULL)return 3;}return memchr(a,0,0)!=NULL;}'
            'memchr-first' = 'int main(void){unsigned char a[]={4,7,0,7,255};return memchr(a,7,5)!=a+1||memchr(a,0,5)!=a+2||memchr(a,-1,5)!=a+4||memchr(a,9,5)!=NULL;}'
            'strlen' = 'int main(void){char a[4097];int i;for(i=0;i<4096;i++)a[i]=(char)(i%255+1);a[4096]=0;if(strlen(a)!=4096||sizeof(strlen(a))!=sizeof(size_t))return 1;a[7]=0;return strlen(a)!=7||strlen("")!=0||strlen("a\0b")!=1;}'
            'strcpy' = 'int main(void){char a[8]={1,1,1,1,1,1,1,1};const char *s="abcd";if(strcpy(a+1,s)!=a+1)return 1;if(a[0]!=1||a[1]!=97||a[4]!=100||a[5]!=0||a[6]!=1)return 2;if(strcpy(a+1,"")!=a+1)return 3;return a[1]!=0||a[2]!=98;}'
            'strncpy-counts' = 'int main(void){char a[12];int n,i;const char *s="abcd";for(n=0;n<=10;n++){for(i=0;i<12;i++)a[i]=37;if(strncpy(a+1,s,n)!=a+1)return 1;for(i=0;i<12;i++)if(a[i]!=(i>0&&i<=n?(i<=4?s[i-1]:0):37))return 2;}return 0;}'
            'strncpy-empty' = 'int main(void){char a[5]={7,7,7,7,7};strncpy(a+1,"",3);return a[0]!=7||a[1]||a[2]||a[3]||a[4]!=7;}'
            'strcat' = 'int main(void){char a[16]="ab";char b[4]="";a[15]=99;if(strcat(a,"cd")!=a)return 1;if(strcmp(a,"abcd")||a[15]!=99)return 2;if(strcat(a,"")!=a||strcmp(a,"abcd"))return 3;return strcat(b,"xyz")!=b||strcmp(b,"xyz");}'
            'strncat-counts' = 'int main(void){char a[16];int i,n,k;for(n=0;n<=6;n++){for(i=0;i<16;i++)a[i]=37;a[0]=97;a[1]=98;a[2]=0;if(strncat(a,"cdef",n)!=a)return 1;k=n<4?n:4;if(a[0]!=97||a[1]!=98||a[k+2]!=0)return 2;for(i=0;i<k;i++)if(a[i+2]!=99+i)return 3;if(a[k+3]!=37)return 4;}return 0;}'
            'strcmp-all-byte-pairs' = 'int main(void){char a[2],b[2];int i,j,r;a[1]=0;b[1]=0;for(i=0;i<256;i++)for(j=0;j<256;j++){a[0]=(char)i;b[0]=(char)j;r=strcmp(a,b);if((r>0)!=(i>j)||(r<0)!=(i<j))return 1;}return 0;}'
            'strcmp-prefixes' = 'int main(void){return strcmp("","")!=0||strcmp("a","ab")>=0||strcmp("ab","a")<=0||strcmp("abc","abc")!=0||strcmp("abc","abd")>=0||strcmp("abd","abc")<=0;}'
            'strncmp-boundaries' = 'int main(void){char a[]={97,0,3},b[]={97,0,4};return strncmp(a,b,3)!=0||strncmp("ab","ac",1)!=0||strncmp("ab","ac",2)>=0||strncmp("a","b",0)!=0||strncmp("a","ab",3)>=0;}'
            'strncmp-all-byte-pairs' = 'int main(void){char a[2],b[2];int i,j,r;a[1]=0;b[1]=0;for(i=0;i<256;i++)for(j=0;j<256;j++){a[0]=(char)i;b[0]=(char)j;r=strncmp(a,b,1);if((r>0)!=(i>j)||(r<0)!=(i<j))return 1;}return 0;}'
            'strchr' = 'int main(void){char s[]={97,98,97,(char)255,0};return strchr(s,97)!=s||strchr(s,98)!=s+1||strchr(s,0)!=s+4||strchr(s,256)!=s+4||strchr(s,-1)!=s+3||strchr(s,99)!=NULL||strchr("",1)!=NULL;}'
            'strrchr' = 'int main(void){char s[]={97,98,97,(char)255,0};return strrchr(s,97)!=s+2||strrchr(s,98)!=s+1||strrchr(s,0)!=s+4||strrchr(s,256)!=s+4||strrchr(s,-1)!=s+3||strrchr(s,99)!=NULL;}'
            'strspn' = 'int main(void){char a[]={(char)255,(char)128,97,0},set[]={(char)128,(char)255,0};return strspn("abcabcx","bca")!=6||strspn("abc","")!=0||strspn("","abc")!=0||strspn("aaaa","aa")!=4||strspn(a,set)!=2;}'
            'strcspn' = 'int main(void){char a[]={(char)255,(char)128,97,0},set[]={(char)128,0};return strcspn("abcabcx","x")!=6||strcspn("abc","")!=3||strcspn("","abc")!=0||strcspn("aaaa","aa")!=0||strcspn(a,set)!=1;}'
            'strpbrk' = 'int main(void){char a[]="abcabcx",h[]={(char)255,97,0},set[]={(char)255,0};return strpbrk(a,"xcb")!=a+1||strpbrk(a,"")!=NULL||strpbrk("","abc")!=NULL||strpbrk(a,"z")!=NULL||strpbrk(h,set)!=h;}'
            'strstr' = 'int main(void){char a[]="abababcabab",h[]={(char)255,97,0},n[]={(char)255,0};return strstr(a,"ababc")!=a+2||strstr(a,"abab")!=a||strstr(a,"")!=a||strstr(a,"abababcababx")!=NULL||strstr("","x")!=NULL||strstr(h,n)!=h;}'
            'strtok-sequence' = 'int main(void){char a[]=",,one;two,,,three;";char *p=strtok(a,",;");if(!p||strcmp(p,"one"))return 1;if(a[5]!=0)return 2;p=strtok(NULL,",;");if(!p||strcmp(p,"two"))return 3;p=strtok(NULL,",;");if(!p||strcmp(p,"three"))return 4;return strtok(NULL,",;")!=NULL||strtok(NULL,",;")!=NULL;}'
            'strtok-changing-delimiters' = 'int main(void){char a[]="a:b,c;d";char *p=strtok(a,":");if(!p||strcmp(p,"a"))return 1;p=strtok(NULL,",");if(!p||strcmp(p,"b"))return 2;p=strtok(NULL,"");if(!p||strcmp(p,"c;d"))return 3;return strtok(NULL,":")!=NULL;}'
            'strtok-reset-empty' = 'int main(void){char a[]="a,b",b[]="x:y",e[]="",d[]=",,,";if(strcmp(strtok(a,","),"a"))return 1;if(strcmp(strtok(b,":"),"x"))return 2;if(strcmp(strtok(NULL,":"),"y"))return 3;if(strtok(e,",")!=NULL||strtok(NULL,",")!=NULL)return 4;if(strtok(d,",")!=NULL)return 5;return 0;}'
            'strtok-isolated-state' = 'int main(void){char a[]="a,b",b[8];char *p=strtok(a,",");if(!p||strcmp(p,"a"))return 1;strcpy(b,"test");strerror(EDOM);strspn(b,"te");p=strtok(NULL,",");return !p||strcmp(p,"b");}'
            'strcoll-C-locale' = 'int main(void){const char *a[]={"","a","ab","b","Z","z","\200","\377"};int i,j,x,y;for(i=0;i<8;i++)for(j=0;j<8;j++){x=strcoll(a[i],a[j]);y=strcmp(a[i],a[j]);if((x>0)!=(y>0)||(x<0)!=(y<0))return 1;}return 0;}'
            'strxfrm-C-locale' = 'int main(void){char a[8],b[8];int i;for(i=0;i<8;i++){a[i]=37;b[i]=37;}if(strxfrm(NULL,"abc",0)!=3)return 1;if(strxfrm(a,"abc",8)!=3||a[0]!=97||a[1]!=98||a[2]!=99||a[3]!=0||a[4]!=37)return 2;if(strxfrm(b,"abd",8)!=3||strcmp(a,b)>=0||strcoll("abc","abd")>=0)return 3;return strxfrm(a,"",8)!=0||a[0]!=0;}'
            'strxfrm-small-buffer' = 'int main(void){char a[6];int i,n;for(n=0;n<=3;n++){for(i=0;i<6;i++)a[i]=37;if(strxfrm(a+1,"abcd",n)!=4)return 1;if(a[0]!=37)return 2;for(i=n+1;i<6;i++)if(a[i]!=37)return 3;}return 0;}'
            'errno-lvalue' = 'int main(void){int *p=&errno;if(errno!=0||sizeof(errno)!=sizeof(int))return 1;errno=EDOM;if(*p!=EDOM)return 2;*p=ERANGE;if(errno!=ERANGE)return 3;errno++;return errno!=ERANGE+1;}'
            'strerror-messages' = 'int main(void){errno=77;if(strcmp(strerror(0),"No error"))return 1;if(strcmp(strerror(EDOM),"Domain error"))return 2;if(strcmp(strerror(ERANGE),"Range error"))return 3;if(strcmp(strerror(EILSEQ),"Illegal byte sequence"))return 4;if(strcmp(strerror(INT_MIN),"Unknown error")||strcmp(strerror(INT_MAX),"Unknown error"))return 5;return errno!=77;}'
            'heap-integration' = 'int main(void){char *p=malloc(512),*q;int i;if(!p)return 1;for(i=0;i<511;i++)p[i]=(char)(i%26+97);p[511]=0;q=malloc(strlen(p)+1);if(!q)return 2;strcpy(q,p);if(strcmp(p,q))return 3;memmove(q+1,q,510);q[511]=0;if(q[0]!=q[1]||q[510]!=p[509])return 4;free(p);free(q);return 0;}'
        }
        $cases += @{
            'strchr-all-byte-values' = 'int main(void){char a[256];int i,c,k;for(i=0;i<255;i++)a[i]=(char)(i+1);a[255]=0;for(c=-256;c<512;c++){k=(unsigned char)c;k=k?k-1:255;if(strchr(a,c)!=a+k||strrchr(a,c)!=a+k)return 1;}return 0;}'
            'search-small-strings' = 'int main(void){char text[7],needle[5];int a,b,i,j,n,m,x,y,expected,ok;for(n=0;n<=6;n++)for(a=0;a<(1<<n);a++){for(i=0;i<n;i++)text[i]=(char)(97+((a>>i)&1));text[n]=0;for(m=0;m<=4;m++)for(b=0;b<(1<<m);b++){for(i=0;i<m;i++)needle[i]=(char)(97+((b>>i)&1));needle[m]=0;expected=-1;for(x=0;x<=n-m;x++){ok=1;for(y=0;y<m;y++)if(((a>>(x+y))&1)!=((b>>y)&1))ok=0;if(ok){expected=x;break;}}if(strstr(text,needle)!=(expected<0?NULL:text+expected))return 1;}}return 0;}'
            'span-search-subsets' = 'int main(void){char a[]="abccabdx",set[5];int mask,i,j,n,accept,reject,first,hit;for(mask=0;mask<16;mask++){n=0;for(i=0;i<4;i++)if(mask&(1<<i))set[n++]=(char)(97+i);set[n]=0;accept=0;while(a[accept]&&a[accept]<101&&(mask&(1<<(a[accept]-97))))accept++;reject=0;while(a[reject]&&(a[reject]>=101||!(mask&(1<<(a[reject]-97)))))reject++;first=a[reject]?reject:-1;if(strspn(a,set)!=accept||strcspn(a,set)!=reject)return 1;if(strpbrk(a,set)!=(first<0?NULL:a+first))return 2;}return 0;}'
            'strtok-high-bytes' = 'int main(void){char a[]={(char)255,97,(char)128,98,(char)255,0},d[]={(char)255,(char)128,0};char *p=strtok(a,d);if(p!=a+1||p[0]!=97||p[1]!=0)return 1;p=strtok(NULL,d);if(p!=a+3||p[0]!=98||p[1]!=0)return 2;return strtok(NULL,d)!=NULL;}'
            'strncat-empty' = 'int main(void){char a[8]="ab";a[3]=37;if(strncat(a,"",100)!=a)return 1;return a[0]!=97||a[1]!=98||a[2]!=0||a[3]!=37;}'
            'strcoll-independent-order' = 'int main(void){const char *a[]={"","a","ab","b","Z","z","\200","\377"};int rank[]={0,2,3,4,1,5,6,7};int i,j,r;for(i=0;i<8;i++)for(j=0;j<8;j++){r=strcoll(a[i],a[j]);if((r>0)!=(rank[i]>rank[j])||(r<0)!=(rank[i]<rank[j]))return 1;}return 0;}'
        }
        foreach ($name in ($cases.Keys | Sort-Object)) {
            Check "string-$mode-$name" ($prefix + $cases[$name] + $suffix) 
        }
        $macros = ('#if EDOM <= 0 || ERANGE <= 0 || EILSEQ <= 0 || EDOM == ERANGE || EDOM == EILSEQ || ERANGE == EILSEQ', '#error bad errno macros', '#endif', 'int main(void){return 0;}') -join $nl
        Check "string-$mode-errno-macros" ($prefix + $macros + $suffix)
        Check "string-$mode-prototypes-pointers" ($prefix + @'
int main(void){
void *(*a)(void *,const void *,size_t)=memcpy;
void *(*b)(void *,const void *,size_t)=memmove;
void *(*c)(void *,int,size_t)=memset;
int (*d)(const void *,const void *,size_t)=memcmp;
void *(*e)(const void *,int,size_t)=memchr;
size_t (*f)(const char *)=strlen;
char *(*g)(char *,const char *)=strcpy;
char *(*h)(char *,const char *,size_t)=strncpy;
char *(*i)(char *,const char *)=strcat;
char *(*j)(char *,const char *,size_t)=strncat;
int (*k)(const char *,const char *)=strcmp;
int (*l)(const char *,const char *,size_t)=strncmp;
int (*m)(const char *,const char *)=strcoll;
size_t (*n)(char *,const char *,size_t)=strxfrm;
char *(*o)(const char *,int)=strchr;
char *(*p)(const char *,int)=strrchr;
size_t (*q)(const char *,const char *)=strspn;
size_t (*r)(const char *,const char *)=strcspn;
char *(*s)(const char *,const char *)=strpbrk;
char *(*t)(const char *,const char *)=strstr;
char *(*u)(char *,const char *)=strtok;
char *(*v)(int)=strerror;
char x[16],y[16];int z=3;
c(x,0,16);a(x,"ab",z++);if(z!=4||f(x)!=2)return 1;
b(y,x,3);if(d(x,y,3)||e(x,98,3)!=x+1)return 2;
g(x,"a");h(y,"b",2);i(x,y);j(x,"cd",1);
if(k(x,"abc")||l(x,"abd",2)||m(x,"abc"))return 3;
if(n(y,x,16)!=3||o(x,98)!=x+1||p(x,99)!=x+2)return 4;
if(q(x,"ba")!=2||r(x,"c")!=2||s(x,"bc")!=x+1||t(x,"bc")!=x+1)return 5;
if(u(x,"b")!=x||!v(EDOM))return 6;
return 0;}
'@+ $suffix)
    }
    Check 'string-repeat-headers' ($prefix + '#include <string.h>' + $nl + '#include <errno.h>' + $nl + 'int main(void){return 0;}' + $suffix)
}
finally {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', $savedInclude, 'Process')
    [Environment]::SetEnvironmentVariable('C1_STANDARD', $savedStandard, 'Process')
}

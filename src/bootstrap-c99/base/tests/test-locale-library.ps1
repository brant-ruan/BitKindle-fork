$savedInclude = [Environment]::GetEnvironmentVariable('C1_INCLUDE', 'Process')
$savedStandard = [Environment]::GetEnvironmentVariable('C1_STANDARD', 'Process')
try {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../include')), 'Process')
    $runtime = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../lib/runtime.c'))
    $nl = [string][char]10
    $prefix = ('#include <locale.h>', '#include <stdlib.h>', '#include <limits.h>', '#include <errno.h>', '#include <string.h>', '#include <ctype.h>', '') -join $nl
    $suffix = $nl + '#include "' + $runtime + '"' + $nl
    foreach ($mode in 'c89', 'c99') {
        [Environment]::SetEnvironmentVariable('C1_STANDARD', $mode, 'Process')
        $cases = @{
            'categories' = 'int main(void){int c[]={LC_ALL,LC_COLLATE,LC_CTYPE,LC_MONETARY,LC_NUMERIC,LC_TIME};int i,j;char *p;for(i=0;i<6;i++){for(j=0;j<i;j++)if(c[i]==c[j])return 1;p=setlocale(c[i],NULL);if(!p||strcmp(p,"C"))return 2;p=setlocale(c[i],"");if(!p||strcmp(p,"C"))return 3;p=setlocale(c[i],"C");if(!p||strcmp(p,"C"))return 4;}return 0;}'
            'invalid-locale' = 'int main(void){const char *names[]={"c","C.UTF-8","en_US","zh_CN","POSIX","CC"};int i;char saved[8];strcpy(saved,setlocale(LC_ALL,NULL));for(i=0;i<6;i++){if(setlocale(LC_ALL,names[i])!=NULL)return 1;if(strcmp(setlocale(LC_ALL,NULL),saved))return 2;}return !setlocale(LC_ALL,saved)||strcmp(setlocale(LC_ALL,NULL),"C");}'
            'invalid-category' = 'int main(void){int c[]={-1,INT_MIN,INT_MAX,6};int i;for(i=0;i<4;i++)if(setlocale(c[i],"C")!=NULL||setlocale(c[i],NULL)!=NULL)return 1;return strcmp(setlocale(LC_ALL,NULL),"C");}'
            'lconv-strings' = 'int main(void){struct lconv *p=localeconv();if(!p||strcmp(p->decimal_point,"."))return 1;if(strcmp(p->thousands_sep,"")||strcmp(p->grouping,"")||strcmp(p->int_curr_symbol,"")||strcmp(p->currency_symbol,""))return 2;if(strcmp(p->mon_decimal_point,"")||strcmp(p->mon_thousands_sep,"")||strcmp(p->mon_grouping,"")||strcmp(p->positive_sign,"")||strcmp(p->negative_sign,""))return 3;return 0;}'
            'lconv-monetary' = 'int main(void){struct lconv *p=localeconv();return p->int_frac_digits!=CHAR_MAX||p->frac_digits!=CHAR_MAX||p->p_cs_precedes!=CHAR_MAX||p->p_sep_by_space!=CHAR_MAX||p->n_cs_precedes!=CHAR_MAX||p->n_sep_by_space!=CHAR_MAX||p->p_sign_posn!=CHAR_MAX||p->n_sign_posn!=CHAR_MAX;}'
            'lconv-C99-fields' = 'int main(void){struct lconv *p=localeconv();return p->int_p_cs_precedes!=CHAR_MAX||p->int_p_sep_by_space!=CHAR_MAX||p->int_n_cs_precedes!=CHAR_MAX||p->int_n_sep_by_space!=CHAR_MAX||p->int_p_sign_posn!=CHAR_MAX||p->int_n_sign_posn!=CHAR_MAX;}'
            'lconv-lifetime' = 'int main(void){struct lconv *p=localeconv();char *decimal=p->decimal_point;setlocale(LC_NUMERIC,"C");if(strcmp(localeconv()->decimal_point,".")||strcmp(decimal,"."))return 1;if(setlocale(LC_MONETARY,"bad")!=NULL)return 2;return strcmp(p->decimal_point,".")||p->frac_digits!=CHAR_MAX;}'
            'mb-limits' = 'int main(void){return MB_CUR_MAX!=1||MB_CUR_MAX>MB_LEN_MAX||sizeof(MB_CUR_MAX)!=sizeof(size_t)||sizeof(wchar_t)!=2;}'
            'single-byte-roundtrip' = 'int main(void){char a[2],out[3];wchar_t w;int i,r;for(i=0;i<256;i++){a[0]=(char)i;a[1]=0;w=999;errno=77;r=mbtowc(&w,a,1);if(r!=(i?1:0)||w!=i||errno!=77)return 1;if(mblen(a,1)!=(i?1:0))return 2;out[0]=37;out[1]=37;out[2]=37;if(wctomb(out+1,w)!=1||(unsigned char)out[1]!=i||out[0]!=37||out[2]!=37)return 3;}return 0;}'
            'all-wide-values' = 'int main(void){unsigned int i;char out[3];int r;for(i=0;i<=65535;i++){out[0]=37;out[1]=37;out[2]=37;errno=77;r=wctomb(out+1,(wchar_t)i);if(i<=255){if(r!=1||(unsigned char)out[1]!=i||errno!=77)return 1;}else if(r!=-1||errno!=EILSEQ||out[1]!=37)return 2;if(out[0]!=37||out[2]!=37)return 3;}return 0;}'
            'reset-state' = 'int main(void){wchar_t w=999;errno=77;if(mblen(NULL,0)!=0||mblen(NULL,99)!=0||mbtowc(&w,NULL,0)!=0||w!=999||mbtowc(NULL,NULL,9)!=0)return 1;if(wctomb(NULL,65535)!=0||errno!=77)return 2;return mblen("a",1)!=1||mbtowc(NULL,"a",1)!=1;}'
            'zero-count' = 'int main(void){wchar_t w=999;errno=0;if(mbtowc(&w,"a",0)!=-1||w!=999||errno!=EILSEQ)return 1;errno=0;if(mbtowc(&w,"",0)!=-1||w!=999||errno!=EILSEQ)return 2;errno=0;return mblen("a",0)!=-1||errno!=EILSEQ;}'
            'null-characters' = 'int main(void){wchar_t w=999;char a[2]={37,37};if(mbtowc(&w,"",1)!=0||w!=0||mblen("",1)!=0)return 1;if(wctomb(a,0)!=1||a[0]!=0||a[1]!=37)return 2;return 0;}'
            'mbstowcs-counts' = 'int main(void){wchar_t a[8];int n,i,expected;for(n=0;n<=6;n++){for(i=0;i<8;i++)a[i]=999;errno=77;if(mbstowcs(a+1,"abc",n)!=(n<3?n:3)||errno!=77)return 1;for(i=0;i<8;i++){expected=999;if(i>=1&&i<=n&&i<=4)expected=i==4?0:96+i;if(a[i]!=expected)return 2;}}return 0;}'
            'wcstombs-counts' = 'int main(void){const wchar_t src[]={97,98,99,0};char a[8];int n,i,expected;for(n=0;n<=6;n++){for(i=0;i<8;i++)a[i]=37;errno=77;if(wcstombs(a+1,src,n)!=(n<3?n:3)||errno!=77)return 1;for(i=0;i<8;i++){expected=37;if(i>=1&&i<=n&&i<=4)expected=i==4?0:96+i;if(a[i]!=expected)return 2;}}return 0;}'
            'string-roundtrip' = 'int main(void){char src[256],out[257];wchar_t wide[257];int i;for(i=0;i<255;i++)src[i]=(char)(i+1);src[255]=0;wide[256]=999;out[256]=37;if(mbstowcs(wide,src,257)!=255||wide[256]!=999)return 1;for(i=0;i<256;i++)if(wide[i]!=(unsigned char)src[i])return 2;if(wcstombs(out,wide,257)!=255||out[256]!=37)return 3;return memcmp(out,src,256)!=0;}'
            'wcstombs-invalid' = 'int main(void){wchar_t src[]={97,256,98,0};char a[5]={37,37,37,37,37};errno=0;if(wcstombs(a,src,5)!=(size_t)-1||errno!=EILSEQ)return 1;if(a[0]!=97||a[1]!=37||a[2]!=37)return 2;errno=77;if(wcstombs(a,src,1)!=1||errno!=77)return 3;src[0]=0;return wcstombs(a,src,5)!=0||a[0]!=0||errno!=77;}'
            'empty-strings' = 'int main(void){wchar_t w[2]={999,999},empty[]={0};char a[2]={37,37};if(mbstowcs(w,"",2)!=0||w[0]!=0||w[1]!=999)return 1;if(wcstombs(a,empty,2)!=0||a[0]!=0||a[1]!=37)return 2;w[0]=999;a[0]=37;return mbstowcs(w,"",0)!=0||w[0]!=999||wcstombs(a,empty,0)!=0||a[0]!=37;}'
            'state-independence' = 'int main(void){wchar_t bad[]={256,0},w[4];char a[4];if(wcstombs(a,bad,4)!=(size_t)-1)return 1;if(mblen("a",1)!=1||mbstowcs(w,"ab",4)!=2)return 2;if(wctomb(a,w[1])!=1||a[0]!=98)return 3;return mbtowc(w,"c",1)!=1||w[0]!=99;}'
            'locale-library-consistency' = 'int main(void){char *end;setlocale(LC_ALL,"");if(strcmp(setlocale(LC_CTYPE,NULL),"C")||!isalpha(97)||isalpha(255)||MB_CUR_MAX!=1)return 1;if(strcoll("a","b")>=0||strtol(" \t42!",&end,10)!=42||*end!=33)return 2;if(setlocale(LC_ALL,"C.UTF-8")!=NULL)return 3;return !isalpha(97)||strcoll("a","b")>=0||MB_CUR_MAX!=1;}'
            'function-pointers' = 'int main(void){char *(*a)(int,const char *)=setlocale;struct lconv *(*b)(void)=localeconv;int (*c)(const char *,size_t)=mblen;int (*d)(wchar_t *,const char *,size_t)=mbtowc;int (*e)(char *,wchar_t)=wctomb;size_t (*f)(wchar_t *,const char *,size_t)=mbstowcs;size_t (*g)(char *,const wchar_t *,size_t)=wcstombs;wchar_t w[4];char s[4];int n=1;if(!a(LC_ALL,"C")||!b()||c("a",n++)!=1||n!=2)return 1;if(d(w,"a",1)!=1||e(s,w[0])!=1||s[0]!=97)return 2;return f(w,"ab",4)!=2||g(s,w,4)!=2||strcmp(s,"ab");}'
        }
        foreach ($name in ($cases.Keys | Sort-Object)) {
            Check "locale-$mode-$name" ($prefix + $cases[$name] + $suffix) 
        }
        Check "locale-$mode-headers-repeat" ($prefix + '#include <locale.h>' + $nl + '#include <stdlib.h>' + $nl + 'int main(void){return 0;}' + $suffix)
    }
}
finally {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', $savedInclude, 'Process')
    [Environment]::SetEnvironmentVariable('C1_STANDARD', $savedStandard, 'Process')
}

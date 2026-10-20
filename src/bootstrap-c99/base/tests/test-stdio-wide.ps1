. (Join-Path $PSScriptRoot 'test-stdio-hex.ps1')
Check 'format-wide-c-locale' ($prefix + @'
int main(void){char b[64];wchar_t w[4]={65,66,67,0};
if(sprintf(b,"[%5lc][%-5.2ls]",90,w)!=14||strcmp(b,"[    Z][AB   ]"))return 1;
if(snprintf(b,3,"%ls",w)!=3||strcmp(b,"AB"))return 2;return 0;}
'@)
Check 'format-wide-bytes-and-nul' ($prefix + @'
int main(void){char b[8];wchar_t w[3]={128,255,0};
if(sprintf(b,"%ls",w)!=2||(unsigned char)b[0]!=128||(unsigned char)b[1]!=255||b[2])return 1;
memset(b,'Q',8);if(snprintf(b,8,"A%lcB",0)!=3||b[0]!='A'||b[1]||b[2]!='B'||b[3]||b[4]!='Q')return 2;
return 0;}
'@)
Check 'format-wide-encoding-errors' ($prefix + @'
int main(void){char b[8];wchar_t w[3]={65,256,0};errno=0;
if(sprintf(b,"%ls",w)!=-1||errno!=EILSEQ)return 1;
errno=0;if(sprintf(b,"%lc",256)!=-1||errno!=EILSEQ)return 2;
if(sprintf(b,"%.1ls",w)!=1||strcmp(b,"A"))return 3;
if(sprintf(b,"%.0ls",w+1)!=0||b[0])return 4;return 0;}
'@)

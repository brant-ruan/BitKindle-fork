$savedInclude = $env:C1_INCLUDE
try {
    $env:C1_INCLUDE = [IO.Path]::GetFullPath('src/bootstrap-c99/base/include')
    $prefix = "#include `"src/bootstrap-c99/base/lib/runtime.c`"`n#include `"src/bootstrap-c99/base/lib/arguments.c`"`n"
    Check 'arguments-basic' ($prefix + @'
int main(void){char **v;int n;if(__c1_parse_arguments("prog one\ttwo  three",&n,&v)||n!=4||v[n])return 1;if(strcmp(v[0],"prog")||strcmp(v[1],"one")||strcmp(v[2],"two")||strcmp(v[3],"three"))return 2;free(v);return 0;}
'@) 0
    Check 'arguments-quoted-program' ($prefix + @'
int main(void){char **v;int n;if(__c1_parse_arguments("\"C:\\Program Files\\app.exe\" \"a b\" \"\"",&n,&v)||n!=3)return 1;if(strcmp(v[0],"C:\\Program Files\\app.exe")||strcmp(v[1],"a b")||*v[2])return 2;free(v);return 0;}
'@) 0
    Check 'arguments-document-example' ($prefix + @'
int main(void){char **v;int n;if(__c1_parse_arguments("p a\\\\\\b d\"e f\"g h",&n,&v)||n!=4)return 1;if(strcmp(v[1],"a\\\\\\b")||strcmp(v[2],"de fg")||strcmp(v[3],"h"))return 2;free(v);return 0;}
'@) 0
    Check 'arguments-backslash-quotes' ($prefix + @'
int main(void){char text[40]="p a";char **v;int n,i,k;for(k=0;k<12;k++){for(i=0;i<k;i++)text[3+i]=92;text[3+k]='"';text[4+k]='b';text[5+k]=0;if(__c1_parse_arguments(text,&n,&v)||n!=2)return 1;if(*v[1]!='a')return 2;for(i=0;i<k/2;i++)if(v[1][1+i]!=92)return 3;i=1+k/2;if(k&1){if(v[1][i++]!='"')return 4;}if(v[1][i++]!='b'||v[1][i])return 5;free(v);}return 0;}
'@) 0
    Check 'arguments-quotes-inside-quotes' ($prefix + @'
int main(void){char **v;int n;if(__c1_parse_arguments("p \"a\"\"b\" x\" y",&n,&v)||n!=3)return 1;if(strcmp(v[1],"a\"b")||strcmp(v[2],"x y"))return 2;free(v);return 0;}
'@) 0
    Check 'arguments-empty-and-mutable' ($prefix + @'
int main(void){char **v;int n;if(__c1_parse_arguments("",&n,&v)||n!=1||*v[0]||v[1])return 1;free(v);if(__c1_parse_arguments("p \"\" x",&n,&v)||n!=3)return 2;v[0][0]='P';v[2][0]='X';v[1]=v[2];if(strcmp(v[1],"X"))return 3;free(v);return 0;}
'@) 0
    Check 'arguments-long-many' ($prefix + @'
int main(void){char *text=malloc(30004);char **v;int n,i;text[0]='p';text[1]=' ';for(i=0;i<15000;i++){text[2+i*2]='x';text[3+i*2]=' ';}text[30002]=0;if(__c1_parse_arguments(text,&n,&v)||n!=15001||v[n])return 1;for(i=1;i<n;i++)if(strcmp(v[i],"x"))return 2;free(v);free(text);return 0;}
'@) 0
    Check 'arguments-os-command' ($prefix + @'
int main(void){char **v;int n;if(__c1_get_arguments(&n,&v)||n<1||!v[0]||!*v[0]||v[n])return 1;if(!strstr(v[0],"arguments-os-command.exe"))return 2;free(v);return 0;}
'@) 0
    Check 'arguments-dbcs-backslash-trail' ($prefix + @'
int main(void){unsigned int cp[3]={932,936,950};unsigned char first[3]={0x95,0xb1,0xa5};char text[10]={112,32,34,0,92,34,32,120,0};int n,i;char **v;for(i=0;i<3;i++){text[3]=(char)first[i];if(__c1_parse_arguments_cp(text,&n,&v,cp[i])||n!=3)return 1;if((unsigned char)v[1][0]!=first[i]||v[1][1]!=92||v[1][2]||strcmp(v[2],"x"))return 2;free(v);}return 0;}
'@) 0
    Check 'arguments-single-byte-codepage' ($prefix + @'
int main(void){char text[10]={112,32,34,(char)0x95,92,34,32,120,0};char **v;int n;if(__c1_parse_arguments_cp(text,&n,&v,1252)||n!=2)return 1;if((unsigned char)v[1][0]!=0x95||v[1][1]!=34||v[1][2]!=32||v[1][3]!=120||v[1][4])return 2;free(v);return 0;}
'@) 0
    Check 'arguments-utf8-and-dangling-lead' ($prefix + @'
int main(void){char text[10]={112,32,34,(char)0xe8,(char)0xa1,(char)0xa8,34,0};char **v;int n;if(__c1_parse_arguments_cp(text,&n,&v,65001)||n!=2||strlen(v[1])!=3||memcmp(v[1],text+3,3))return 1;free(v);text[3]=(char)0x95;text[4]=0;if(__c1_parse_arguments_cp(text,&n,&v,932)||n!=2||strlen(v[1])!=1||(unsigned char)v[1][0]!=0x95)return 2;free(v);return 0;}
'@) 0
}
finally {
    $env:C1_INCLUDE = $savedInclude
}

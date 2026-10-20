Check 'multichar-pack-and-type' 'int main(void){return ''ab''!=0x6162||''abcd''!=0x61626364||''abcdef''!=0x63646566||sizeof(''ab'')!=sizeof(int);}' 0
Check 'multichar-escape-units' 'int main(void){return ''a\n''!=0x610a||''\101\102''!=0x4142||''a\x42''!=0x6142||''\0a''!=97;}' 0
Check 'multichar-sign-extension' 'int main(void){long long x=''\377\377\377\377'';return x!=-1||''\200abc''>=0||''\200abc''!=-2141101469;}' 0
Check 'multichar-static-enum-switch' 'enum E{A=''ab''};long long a=''\377\377\377\377'';int v[A==0x6162?1:-1];int main(void){switch(A){case ''ab'':return a!=-1;}return 2;}' 0
Check 'multichar-preprocessor' (@'
#define C 'ab'
#if C != 0x6162 || '\377\377\377\377' != -1 || '\200abc' >= 0
#error bad character evaluation
#endif
int main(void){return 0;}
'@) 0
Check 'multichar-wide-last-unit' 'int main(void){return L''ab''!=98||L''\xffff\x61''!=97||sizeof(L''ab'')!=sizeof(unsigned short);}' 0
Check 'multichar-wide-escapes' 'int main(void){return L''a\x1234''!=0x1234||L''\777b''!=98;}' 0
Check 'multichar-wide-preprocessor' (@'
#if L'ab' != 98 || L'a\x1234' != 0x1234
#error bad wide evaluation
#endif
int main(void){return 0;}
'@) 0
Check 'multichar-empty-rejected' 'int main(void){return '''';}' 0 $true
Check 'multichar-wide-empty-rejected' 'int main(void){return L'''';}' 0 $true
Check 'multichar-narrow-escape-overflow' 'int main(void){return ''a\x100'';}' 0 $true
Check 'multichar-wide-escape-overflow' 'int main(void){return L''a\x10000'';}' 0 $true


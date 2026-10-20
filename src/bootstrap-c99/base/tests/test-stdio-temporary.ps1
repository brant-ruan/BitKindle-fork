$prefix = '#include "src/bootstrap-c99/base/lib/runtime.c"' + [char]10
$dir = $ScratchDirectory.Replace('\', '/')
$prefix += ('#define RECORD "' + $dir + '/temp-record.txt"') + [char]10
$savedTmp = $env:TMP
try {
    $env:TMP = $ScratchDirectory
    Check 'tmpfile-binary-update-close' ($prefix + @'
int main(void){FILE *f,*r;char path[L_tmpnam];unsigned char a[256],b[256];int i;
f=tmpfile();if(!f||__c1_tmp_name(path,__c1_tmp_serial))return 1;
for(i=0;i<256;i++)a[i]=i;
if(fwrite(a,1,256,f)!=256||fseek(f,0,SEEK_SET)||fread(b,1,256,f)!=256||memcmp(a,b,256))return 2;
if(fclose(f))return 3;r=fopen(path,"rb");if(r){fclose(r);return 4;}return 0;}
'@)
    Check 'tmpnam-storage-and-absence' ($prefix + @'
int main(void){char a[L_tmpnam],b[L_tmpnam],saved[L_tmpnam];char *s;FILE *f;
if(tmpnam(a)!=a||tmpnam(b)!=b||!strcmp(a,b))return 1;
f=fopen(a,"rb");if(f)return 2;f=fopen(b,"rb");if(f)return 3;
s=tmpnam(NULL);if(!s)return 4;strcpy(saved,s);
f=tmpfile();if(!f||strcmp(saved,s)||fclose(f))return 5;
f=fopen(a,"wb");if(!f||fputs("mine",f)||fclose(f)||remove(a))return 6;
return 0;}
'@)
    Check 'tmpfile-collision-preserves-existing' ($prefix + @'
int main(void){FILE *f,*r;char path[L_tmpnam],b[8];
if(__c1_tmp_name(path,1))return 1;
f=fopen(path,"wb");if(!f||fputs("keep",f)||fclose(f))return 2;
f=tmpfile();if(!f||__c1_tmp_serial!=2||fclose(f))return 3;
r=fopen(path,"rb");if(!r||fread(b,1,8,r)!=4||memcmp(b,"keep",4))return 4;
return fclose(r)||remove(path);}
'@)
    Check 'tmpfile-pool-capacity' ($prefix + @'
int main(void){FILE *f[FOPEN_MAX];int i,n=FOPEN_MAX-3;
for(i=0;i<n;i++){f[i]=tmpfile();if(!f[i])return 1;}
if(tmpfile()||errno!=24)return 2;
for(i=0;i<n;i++)if(fclose(f[i]))return 3;return 0;}
'@)
    foreach ($how in 'return', 'exit', 'immediate') {
        $end = switch ($how) {
            'return' {
                'return 0;'
            } 'exit' {
                'exit(0);'
            } 'immediate' {
                '_Exit(0);'
            }
        }
        Check "tmpfile-$how-cleanup" ($prefix + @'
int main(void){FILE *f,*r;char path[L_tmpnam];
f=tmpfile();if(!f||__c1_tmp_name(path,__c1_tmp_serial)||fputs("temporary",f))return 1;
r=fopen(RECORD,"w");if(!r||fputs(path,r)||fclose(r))return 2;
'@+ $end + '}')
        Check "tmpfile-$how-observe" ($prefix + @'
int main(void){FILE *r,*f;char path[L_tmpnam];
r=fopen(RECORD,"r");if(!r||!fgets(path,L_tmpnam,r)||fclose(r))return 1;
f=fopen(path,"rb");if(f){fclose(f);return 2;}return remove(RECORD);}
'@)
    }
    $env:TMP = Join-Path $ScratchDirectory 'missing-directory'
    Check 'tmpfile-invalid-directory' ($prefix + @'
int main(void){char s[L_tmpnam];if(tmpfile()||tmpnam(s))return 1;return 0;}
'@)
    foreach ($length in 209, 210) {
        $padding = $length - $ScratchDirectory.Length - 1
        if ($padding -lt 1 -or $padding -gt 255) {
            throw 'Unexpected scratch path length for boundary fixture'
        }
        $path = Join-Path $ScratchDirectory ('d' * $padding)
        New-Item -ItemType Directory -Force -Path $path | Out-Null
        $env:TMP = $path
        if ($length -eq 209) {
            Check 'tmpnam-path-exact-limit' ($prefix + @'
int main(void){char path[L_tmpnam];FILE *f;
if(!tmpnam(path)||strlen(path)!=L_tmpnam-1)return 1;
f=tmpfile();if(!f)return 2;return fclose(f);}
'@)
        }
        else {
            Check 'tmpnam-path-over-limit' ($prefix + @'
int main(void){char path[L_tmpnam];path[0]='Q';errno=0;
if(tmpnam(path)||errno!=ERANGE||path[0]!='Q'||tmpfile())return 1;return 0;}
'@)
        }
    }
}
finally {
    $env:TMP = $savedTmp 
}

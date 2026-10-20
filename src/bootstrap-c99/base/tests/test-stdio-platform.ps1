$prefix = '#include "src/bootstrap-c99/base/lib/runtime.c"' + [char]10
$dir = $ScratchDirectory.Replace('\', '/')
Check 'platform-file-operations' ($prefix + ('#define TEST_DIRECTORY "' + $dir + '"') + [char]10 + @'
typedef void *Open(const char *,unsigned int,unsigned int,void *,unsigned int,unsigned int,void *);
typedef int Io(void *,void *,unsigned int,unsigned int *,void *);
typedef int Close(void *);
typedef int Seek(void *,long long,long long *,unsigned int);
typedef int Size(void *,long long *);
typedef int Remove(const char *);
typedef int Move(const char *,const char *,unsigned int);
typedef unsigned int Kind(void *);
typedef unsigned int Error(void);
typedef unsigned int Temp(const char *,const char *,unsigned int,char *);
typedef int Console(void *,unsigned int *);
int main(void){
 Open *open=*(Open **)0x401060;Io *read=*(Io **)0x401048,*write=*(Io **)0x401050;
 Close *close=*(Close **)0x401068,*flush=*(Close **)0x4010c0;
 Seek *seek=*(Seek **)0x4010a0;Size *size=*(Size **)0x4010a8;
 Remove *remove=*(Remove **)0x4010b0;Move *move=*(Move **)0x4010b8;
 Kind *kind=*(Kind **)0x4010c8;Error *error=*(Error **)0x4010d0;
 Temp *temp=*(Temp **)0x401108;Console *console=*(Console **)0x401110;
 char path[512],renamed[520],buffer[16];void *file;unsigned int n,mode;
 long long length,position;
 if(!temp(TEST_DIRECTORY,"c1p",0,path))return 1;
 strcpy(renamed,path);strcat(renamed,".renamed");
 file=open(path,0xc0000000U,0,0,3,0x80,0);
 if(file==(void *)-1)return 2;
 if(kind(file)!=1||console(file,&mode))return 3;
 if(!write(file,"abcdef",6,&n,0)||n!=6||!flush(file))return 4;
 if(!size(file,&length)||length!=6)return 5;
 if(!seek(file,-3,&position,2)||position!=3)return 6;
 if(!read(file,buffer,3,&n,0)||n!=3||memcmp(buffer,"def",3))return 7;
 if(!seek(file,0,&position,0)||position)return 8;
 if(!read(file,buffer,16,&n,0)||n!=6||memcmp(buffer,"abcdef",6))return 9;
 if(!read(file,buffer,1,&n,0)||n)return 10;
 if(!close(file)||!move(path,renamed,0)||!remove(renamed))return 11;
 if(remove(renamed)||error()!=2)return 12;
 return 0;
}
'@)
Check 'platform-environment-time' ($prefix + @'
typedef char *Command(void);
typedef void Time(unsigned long long *);
typedef unsigned int Zone(void *);
typedef unsigned long long Ticks(void);
typedef unsigned int Pid(void);
typedef unsigned int TempPath(unsigned int,char *);
int main(void){
 Command *command=*(Command **)0x4010d8;Time *time=*(Time **)0x4010e0;
 Zone *zone=*(Zone **)0x4010e8;Ticks *ticks=*(Ticks **)0x4010f0;
 Pid *pid=*(Pid **)0x4010f8;TempPath *temp=*(TempPath **)0x401100;
 unsigned long long stamp,a,b;unsigned int n,state;unsigned char z[192];char *path;int i;
 if(!command()||!*command()||!pid())return 1;
 time(&stamp);if(stamp<116444736000000000ULL)return 2;
 a=ticks();b=ticks();if(b<a)return 3;
 for(i=0;i<192;i++)z[i]=0xa5;
 state=zone(z);if(state>2)return 4;
 for(i=172;i<192;i++)if(z[i]!=0xa5)return 5;
 n=temp(0,0);if(!n)return 6;
 path=malloc(n+1);if(!path)return 7;
 if(!temp(n+1,path)||!path[0])return 8;
 free(path);return 0;
}
'@)
Check 'platform-existing-runtime' ($prefix + 'int main(void){char *p=malloc(32);if(!p)return 1;strcpy(p,"12.5");if(strtod(p,0)!=12.5||sqrt(4)!=2)return 2;free(p);return 0;}')

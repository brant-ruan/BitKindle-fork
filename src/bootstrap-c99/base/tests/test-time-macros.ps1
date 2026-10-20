# Uses only the self-built compiler for executables; the shell prepares and checks test text.
PpCheck 'time-defined' @('#if !defined(__DATE__) || !defined(__TIME__)', '#error missing', '#endif', 'int main(void){return 0;}')
PpCheck 'date-ifdef' @('#ifdef __DATE__', 'int main(void){return 3;}', '#else', '#error missing', '#endif') 3
PpCheck 'time-ifndef' @('#ifndef __TIME__', '#error missing', '#endif', 'int main(void){return 0;}')
PpCheck 'date-size' @('int main(void){return sizeof(__DATE__);}') 12
PpCheck 'time-size' @('int main(void){return sizeof(__TIME__);}') 9
PpCheck 'date-time-concatenation' @('int main(void){return sizeof(__DATE__ " " __TIME__);}') 21
PpCheck 'time-format' @('int main(void){char *t=__TIME__;int h=(t[0]-48)*10+t[1]-48;int m=(t[3]-48)*10+t[4]-48;int s=(t[6]-48)*10+t[7]-48;return !(t[2]==58&&t[5]==58&&h>=0&&h<24&&m>=0&&m<60&&s>=0&&s<60&&t[8]==0);}')
PpCheck 'date-repeat' @('int main(void){char *a=__DATE__,*b=__DATE__;int i=0;while(a[i]||b[i]){if(a[i]!=b[i])return 1;i++;}return 0;}')
PpCheck 'time-repeat' @('int main(void){char *a=__TIME__,*b=__TIME__;int i=0;while(a[i]||b[i]){if(a[i]!=b[i])return 1;i++;}return 0;}')
PpCheck 'date-object-macro' @('#define DATE __DATE__', 'int main(void){return sizeof(DATE);}') 12
PpCheck 'time-function-macro' @('#define TIME() __TIME__', 'int main(void){return sizeof(TIME());}') 9
PpCheck 'time-paste' @('#define CAT(a,b) a ## b', 'int main(void){return sizeof(CAT(__TI,ME__));}') 9
PpString 'date-stringize' @('#define S(x) #x') 'S(__DATE__)' '__DATE__'
PpString 'time-stringize' @('#define S(x) #x') 'S(__TIME__)' '__TIME__'
PpCheck 'time-line-control' @('#line 80 "fiction.c"', 'int main(void){return sizeof(__TIME__)+sizeof(__DATE__);}') 21
PpCheck 'date-reject-define' @('#define __DATE__ "x"', 'int main(void){return 0;}') 0 $true
PpCheck 'time-reject-define' @('#define __TIME__ "x"', 'int main(void){return 0;}') 0 $true
PpCheck 'date-reject-undef' @('#undef __DATE__', 'int main(void){return 0;}') 0 $true
PpCheck 'time-reject-undef' @('#undef __TIME__', 'int main(void){return 0;}') 0 $true
PpCheck 'time-inactive-directive' @('#if 0', '#undef __TIME__', '#define __DATE__ "x"', '#endif', 'int main(void){return 0;}')

# Exercise the production C formatter with deterministic calendar inputs.
$clockSource = [IO.File]::ReadAllText((Join-Path $PSScriptRoot '../c1/compiler.c'))
$clockBegin = $clockSource.IndexOf('int pp_clock_digits(')
$clockEnd = $clockSource.IndexOf('int pp_clock(void)', $clockBegin)
if ($clockBegin -lt 0 -or $clockEnd -lt 0) {
    throw 'Cannot locate production clock formatter'
}
$clockFunctions = $clockSource.Substring($clockBegin, $clockEnd - $clockBegin)
$clockPrefix = 'typedef void PP_LocalTime(unsigned short *);int pp_clock_ready;int pp_generated;int pp_date_start;int pp_date_length;int pp_time_start;int pp_time_length;int failed;int output[128];int error(void){failed=1;return 0;}int pp_generate(int c){output[pp_generated++]=c;return 0;}'
function CalendarCheck([string]$Name, [int]$Year, [int]$Month, [int]$Day, [int]$Hour, [int]$Minute, [int]$Second, [string]$Expected, [bool]$Invalid = $false) {
    $driver = 'int main(void){unsigned short v[8];int i;v[0]=' + $Year + ';v[1]=' + $Month + ';v[2]=0;v[3]=' + $Day + ';v[4]=' + $Hour + ';v[5]=' + $Minute + ';v[6]=' + $Second + ';v[7]=0;pp_clock_format(v);'
    if ($Invalid) {
        $driver += 'return !failed;}'
    }
    else {
        $driver += 'if(failed)return 1;for(i=0;i<pp_generated;i++)__c0_putchar(output[i]);return 0;}'
    }
    Check $Name ($clockPrefix + $clockFunctions + $driver) 0 $false '' $Expected
}
$clockMonths = @('Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec')
for ($clockMonth = 1; $clockMonth -le 12; $clockMonth++) {
    CalendarCheck "date-month-$clockMonth" 2026 $clockMonth 1 0 0 0 ('"' + $clockMonths[$clockMonth - 1] + '  1 2026""00:00:00"')
}
CalendarCheck 'date-day-nine' 2001 3 9 1 2 3 '"Mar  9 2001""01:02:03"'
CalendarCheck 'date-day-ten' 2001 3 10 10 20 30 '"Mar 10 2001""10:20:30"'
CalendarCheck 'date-leap-day' 2000 2 29 23 59 59 '"Feb 29 2000""23:59:59"'
CalendarCheck 'date-end-year' 1999 12 31 23 59 59 '"Dec 31 1999""23:59:59"'
CalendarCheck 'date-year-padding' 7 1 2 3 4 5 '"Jan  2 0007""03:04:05"'
CalendarCheck 'date-reject-month-zero' 2026 0 1 0 0 0 '' $true
CalendarCheck 'date-reject-month-range' 2026 13 1 0 0 0 '' $true
CalendarCheck 'date-reject-day-zero' 2026 1 0 0 0 0 '' $true
CalendarCheck 'time-reject-hour-range' 2026 1 1 24 0 0 '' $true
CalendarCheck 'time-reject-second-range' 2026 1 1 0 0 60 '' $true

# Compare compiler-generated strings with the host's local clock interval.
$clockInput = Join-Path $ScratchDirectory 'time-live.c'
$clockExe = Join-Path $ScratchDirectory 'time-live.exe'
$clockOutput = Join-Path $ScratchDirectory 'time-live.stdout'
[IO.File]::WriteAllText($clockInput, 'int main(void){char *s=__DATE__ " " __TIME__;while(*s)__c0_putchar(*s++);return 0;}', [Text.Encoding]::ASCII)
$clockBefore = Get-Date
if ((Native $Compiler $clockInput $clockExe (Join-Path $ScratchDirectory 'time-live.err')) -ne 0) {
    throw 'Live clock compilation failed'
}
$clockAfter = Get-Date
if ((Native $clockExe '' $clockOutput '') -ne 0) {
    throw 'Live clock executable failed'
}
$clockText = [IO.File]::ReadAllText($clockOutput)
if ($clockText -notmatch '^[A-Z][a-z]{2} [ 0-9][0-9] [0-9]{4} [0-9]{2}:[0-9]{2}:[0-9]{2}$') {
    throw "Invalid date/time format: $clockText"
}
$clockParsed = [DateTime]::ParseExact(($clockText -replace '  ', ' '), 'MMM d yyyy HH:mm:ss', [Globalization.CultureInfo]::InvariantCulture)
$clockMin = $clockBefore
$clockMax = $clockAfter
if ($clockMin -gt $clockMax) {
    $clockMin = $clockAfter
    $clockMax = $clockBefore
}
if ($clockParsed -lt $clockMin.AddSeconds(-1) -or $clockParsed -gt $clockMax.AddSeconds(1)) {
    throw "Date/time outside compilation interval: $clockText"
}
$script:passed++
'PASS C1 time-live-clock'

# The production cache receives a C clock stub whose value changes on every call.
# All test code, including the unchanged production functions, is compiled by C1.
$clockStub = 'int clock_calls;void fake_clock(unsigned short *v){int i;for(i=0;i<8;i++)v[i]=0;clock_calls++;v[0]=2026;v[1]=1;v[3]=clock_calls;v[6]=clock_calls-1;}'
$clockDriver = 'int main(void){int first_length;pp_clock_read(fake_clock);first_length=pp_generated;pp_clock_read(fake_clock);if(failed||clock_calls!=1||pp_generated!=first_length)return 1;for(int i=0;i<pp_generated;i++)__c0_putchar(output[i]);return 0;}'
Check 'time-cached-snapshot' ($clockPrefix + $clockFunctions + $clockStub + $clockDriver) 0 $false '' '"Jan  1 2026""00:00:00"'

$savedInclude = [Environment]::GetEnvironmentVariable('C1_INCLUDE', 'Process')
$savedStandard = [Environment]::GetEnvironmentVariable('C1_STANDARD', 'Process')
try {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../include')), 'Process')
    foreach ($mode in 'c89', 'c99') {
        [Environment]::SetEnvironmentVariable('C1_STANDARD', $mode, 'Process')
        foreach ($bits in 8, 16, 32, 64) {
            $bytes = $bits / 8
            foreach ($family in '', '_least', '_fast') {
                $width = if ($family -eq '_fast' -and $bits -lt 32) {
                    4
                }
                else {
                    $bytes
                }
                $suffix = if ($family) {
                    $family.Substring(1).ToUpper() + $bits
                }
                else {
                    "$bits"
                }
                $signedType = "int${family}${bits}_t"
                $unsignedType = "uint${family}${bits}_t"
                $signedMin = "INT${suffix}_MIN"
                $signedMax = "INT${suffix}_MAX"
                $unsignedMax = "UINT${suffix}_MAX"
                if ($family) {
                    $signedMin = "INT_$suffix`_MIN"
                    $signedMax = "INT_$suffix`_MAX"
                    $unsignedMax = "UINT_$suffix`_MAX"
                }
                $program = "int main(void){$signedType s=$signedMin;$unsignedType u=$unsignedMax;if(sizeof(s)!=$width || sizeof(u)!=$width)return 1;if(s>=0 || ($signedType)$signedMax<0)return 2;if(($unsignedType)(u+1)!=0)return 3;if((($unsignedType)-1)!=u)return 4;if(s!=-($signedType)$signedMax-1 || ($unsignedType)$signedMax!=((($unsignedType)1<<($width*8-1))-1))return 5;return 0;}"
                PpCheck "stdint-$mode-$family$bits-types-limits" @('#include <stdint.h>', $program)
            }
        }
        PpCheck "stdint-$mode-exact-values" @('#include <stdint.h>', '#if INT8_MIN != -128 || INT8_MAX != 127 || UINT8_MAX != 255 || INT16_MIN != -32768 || UINT16_MAX != 65535', '#error narrow limits', '#endif', '#if INT32_MIN != (-2147483647-1) || UINT32_MAX != 4294967295U || INT64_MAX != 9223372036854775807LL || UINT64_MAX != 18446744073709551615ULL', '#error wide limits', '#endif', 'int main(void){return 0;}')
        PpCheck "stdint-$mode-constant-promotions" @('#include <stdint.h>', 'int main(void){return sizeof(UINT8_C(255))==4 && -UINT8_C(255)<0 && sizeof(UINT16_C(65535))==4 && -UINT16_C(65535)<0 && sizeof(UINT32_C(1))==4 && -UINT32_C(1)>0 && sizeof(INT64_C(1))==8 && -INT64_C(1)<0 && sizeof(UINT64_C(1))==8 && -UINT64_C(1)>0;}') 1
        PpCheck "stdint-$mode-limit-promotions" @('#include <stdint.h>', 'int main(void){return -UINT8_MAX<0 && -UINT16_MAX<0 && -UINT_LEAST8_MAX<0 && -UINT_LEAST16_MAX<0 && -UINT_FAST8_MAX>0 && -UINT_FAST16_MAX>0 && sizeof(UINT64_MAX)==8;}') 1
        PpCheck "stdint-$mode-constant-values" @('#include <stdint.h>', 'int main(void){return INT8_C(127)==127 && INT16_C(32767)==32767 && INT32_C(2147483647)==2147483647 && UINT32_C(4294967295)==UINT32_MAX && INTMAX_C(9223372036854775807)==INTMAX_MAX && UINTMAX_C(18446744073709551615)==UINTMAX_MAX;}') 1
        PpCheck "stdint-$mode-constant-preprocessor" @('#include <stdint.h>', '#if INT64_C(0x7fffffffffffffff) != INT64_MAX || UINT64_C(0xffffffffffffffff) != UINT64_MAX || UINT8_C(255) != 255', '#error constants', '#endif', 'int main(void){return 0;}')
        PpCheck "stdint-$mode-pointer-roundtrip" @('#include <stdint.h>', 'int main(void){int x;void *p=&x;intptr_t s=(intptr_t)p;uintptr_t u=(uintptr_t)p;return (void*)s==p && (void*)u==p && sizeof(s)==8 && sizeof(u)==8;}') 1
        PpCheck "stdint-$mode-greatest-types" @('#include <stdint.h>', 'int main(void){intmax_t s=INTMAX_MIN;uintmax_t u=UINTMAX_MAX;return sizeof(s)==8 && sizeof(u)==8 && s<0 && u==~0ULL && INTPTR_MIN==INTMAX_MIN && INTPTR_MAX==INTMAX_MAX && UINTPTR_MAX==UINTMAX_MAX;}') 1
        PpCheck "stdint-$mode-other-limits" @('#include <stdint.h>', '#include <stddef.h>', 'int main(void){size_t n=SIZE_MAX;ptrdiff_t d=PTRDIFF_MIN;wchar_t w=WCHAR_MAX;return n==(size_t)-1 && d<0 && PTRDIFF_MAX==INT64_MAX && w==65535 && WCHAR_MIN==0 && sizeof(WCHAR_MAX)==sizeof(+w);}') 1
        PpCheck "stdint-$mode-enum-bound" @('#include <stdint.h>', 'enum E{N=INT8_C(7)};int a[UINT16_C(3)];int main(void){return N+sizeof a;}') 19
    }
    PpCheck 'stdint-repeated-header' @('#include <stdint.h>', '#include <stdint.h>', '#include <limits.h>', '#include <stddef.h>', 'int main(void){uint64_t x=UINT64_MAX;return x>0;}') 1
}
finally {
    [Environment]::SetEnvironmentVariable('C1_INCLUDE', $savedInclude, 'Process')
    [Environment]::SetEnvironmentVariable('C1_STANDARD', $savedStandard, 'Process')
}

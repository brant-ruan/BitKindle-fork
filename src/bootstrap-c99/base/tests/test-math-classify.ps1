$prefix = '#include "src/bootstrap-c99/base/lib/math-classify.c"' + [char]10 + '#include "src/bootstrap-c99/base/lib/runtime.c"' + [char]10
foreach ($v in @(@('float', 'unsigned int', 23, 127), @('double', 'unsigned long long', 52, 1023), @('long double', 'unsigned long long', 52, 1023))) {
    $t = $v[0]
    $u = $v[1]
    $p = $v[2]
    $bias = $v[3]
    $inf = [UInt64]($bias * 2 + 1) -shl $p
    $minimum = [UInt64]1 -shl $p
    $sign = if ($p -eq 23) {
        [UInt64]2147483648
    }
    else {
        [UInt64]::Parse('8000000000000000', [Globalization.NumberStyles]::HexNumber)
    }
    $vectors = @(@([UInt64]0, 2), @([UInt64]1, 3), @(($minimum - 1), 3), @($minimum, 4), @(($inf - 1), 4), @($inf, 1), @(($inf + 1), 0), @(($inf -bor ($minimum -shr 1)), 0))
    $code = "int main(void){union{$t f;$u u;}a;fenv_t saved;int mode;unsigned control;fegetenv(&saved);"
    $code += 'for(mode=0;mode<4;mode++){control=0x9fe5U|((unsigned)mode<<13);__c1_fe_write(control);'
    foreach ($negative in 0, 1) {
        foreach ($v in $vectors) {
            $bits = [UInt64]$v[0]
            if ($negative) {
                $bits = $bits -bor $sign
            }
            $class = $v[1]
            $code += 'a.u=0x' + $bits.ToString('X') + 'ULL;'
            $code += "if(fpclassify(a.f)!=$class||!!signbit(a.f)!=$negative)return 1;"
            $code += "if(!!isfinite(a.f)!=$([int]($class -gt 1))||!!isnormal(a.f)!=$([int]($class -eq 4)))return 2;"
            $code += "if(!!isnan(a.f)!=$([int]($class -eq 0))||!!isinf(a.f)!=$([int]($class -eq 1)))return 3;"
            $code += 'if(__c1_fe_read()!=control)return 4;'
        }
    }
    $code += '}fesetenv(&saved);return 0;}'
    MathCheck "math-classify-$t" ($prefix + $code)
}
MathCheck 'math-classify-evaluation' ($prefix + @'
int calls;
float value(void){calls++;return 0x1p-149f;}
int main(void){
 volatile float f=1.0f;
 if(fpclassify(value())!=FP_SUBNORMAL||calls!=1)return 1;
 if(isnormal(value())||calls!=2)return 2;
 if(!isfinite(value())||calls!=3)return 3;
 if(isnan(value())||calls!=4)return 4;
 if(isinf(value())||calls!=5)return 5;
 if(signbit(value())||calls!=6)return 6;
 if(fpclassify(f++)!=FP_NORMAL||f!=2.0f)return 7;
 if(sizeof(float_t)!=4||sizeof(double_t)!=8)return 8;
 return 0;
}
'@)

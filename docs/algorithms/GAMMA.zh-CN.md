[English](GAMMA.md) · 简体中文

# Gamma 函数的实现与验证

下文源码与测试路径均相对于 `src/bootstrap-c99/complex/`。提及的测试日志由对应套件生成，不是必需的源码输入。

lib/gamma.c 提供 binary32、binary64 和 Windows binary64 long double 的 lgamma、tgamma。不提供 signgam 扩展，它不是 C99 要求。私有 gamma-core.h 中的算术、指数和打包辅助实现来自本项目运行库内核的源码副本。

正数 log-Gamma 在 x>=64 时采用包含 B24 项的 Stirling 展开，较小参数通过递推约化。在 x=1 附近，先稳定计算 64+z 与 64 处 Stirling 表达式之差，再减去 sum(k=1..63) log1p(z/k)。差值各项均保留因子 z；直接相减两个较大的已舍入 log-Gamma 值会损失零点附近的精度。递推也保留 x=2 处的零点。负幂差值显式传播，不通过相减已舍入的幂获得。

负数参数使用反射恒等式，对 |x| 的精确小数部分求正弦，计算采用拆分的 pi 和双分量泰勒级数。log-Gamma 返回 log(abs(Gamma))，Gamma 符号由所在区间的奇偶性确定。极小参数通过一附近的递推避免反射抵消。对于极大的正数 log-Gamma 参数，乘法前缩放 x 以避免中间溢出。tgamma 对未舍入的 log-Gamma 结果求指数，并直接打包最终幅值，包括次正规值。

可精确表示的小阶乘采用独立的精确路径。极小 tgamma 参数使用倒数路径，并依据 Gamma(x)=1/x-EulerGamma+O(x) 作定向修正。这包括小于 2^-60 的二的幂，以及小于 2^-120 的一般输入。一般输入的阈值使修正量小于非精确倒数到舍入边界的距离。已有异常标志和 errno 被保留；新的极点、定义域和范围错误遵循所声明的 C99 策略。带符号零极点、负整数极点、无穷和 NaN 在有限值计算前处理。信号 NaN 保留载荷并转为安静 NaN，同时触发无效操作异常。

数学依据：

- [NIST DLMF 5.11](https://dlmf.nist.gov/5.11)：Stirling 展开。
- [NIST DLMF 5.5](https://dlmf.nist.gov/5.5)：递推与反射。
- [N1256](https://www.open-std.org/jtc1/sc22/wg14/www/docs/n1256.pdf)：第 7.12.8.3/4 节及附录 F。

make-gamma-reference.ps1 只生成 C 测试源码。它使用 Machin 公式和 BigInteger 运算构造 sqrt(pi)，通过精确有理数递推计算半整数 Gamma，并通过独立对数级数计算 log(abs(Gamma))。一、二及负极点附近的额外测试数据使用保留到二次项的局部 Gamma 展开。位移量对 float 为 2^-21，对 double 为 2^-50，因此在这些特定测试中，省略的三次项远小于结果的一个 ULP；这不证明所有舍入决策都正确。GAMMA-ORACLE.txt 记录 600 位与 700 位十进制精度下的测试数据比较。

GAMMA.txt 在四种舍入模式下报告 1,432 条输入/操作/精度记录的检查结果，每个结果均逐位比较；binary64 测试还调用 long double。三次链接执行使用两代编译器及混合代次的提供方。额外测试覆盖递推、精确阶乘、极小输入倒数的舍入方向、带符号下溢、最大输入的 log-Gamma 溢出、极点、特殊值和已有错误状态的保留。所有目标文件与可执行文件仅由已有自制编译器和链接器生成。

选定输入的精确匹配不是普遍正确舍入的证明。不宣称全范围误差上界，也不宣称穷尽负非整数输入及舍入边界情形。

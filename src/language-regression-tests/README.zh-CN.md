[English](README.md) · 简体中文

# 语言回归测试

本包仅包含源码，使用已经自举构建的两代编译器运行。覆盖 C89/C99 两种模式下的 25 个预处理表达式用例、81 对输入下的 C89 宏重扫描示例，以及选定的 C99 浮点环境、pragma 和转换行为。它补充种子验证，不证明完整 C89/C99 或附录 F 符合性。

完成所选源码快照的构建后，在发布包根目录执行：

```powershell
& src/language-regression-tests/run.ps1 -Snapshot src/bootstrap-c99 -BuildDirectory src/bootstrap-c99/build-from-seed -ScratchDirectory work/language-tests *> work/language-tests.txt
if(!$?){throw 'Language tests failed'}
& src/language-regression-tests/audit.ps1 -ScratchDirectory work/language-tests -VerificationLog work/language-tests.txt
```

使用新的测试目录。运行脚本只生成 C/PowerShell 文本。所有目标文件与可执行文件均由指定的自制编译器和链接器产生。审计检查日志、拒绝非法输入的证据、测试源码与当前模板的一致性，以及 44 对非空且逐字节相同的产物。预处理测试辅助代码取自指定源码快照；该快照的源码路径和构建来源须另行检查。

浮点测试在 C99 模式下执行，将它们纳入测试不意味着把 C99 要求视为 C89 要求。FP_CONTRACT ON 允许表达式收缩，并不要求产生融合运算结果。

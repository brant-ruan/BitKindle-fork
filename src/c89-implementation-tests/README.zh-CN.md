[English](README.md) · 简体中文

# 实现行为探针

本包包含六套源码测试，覆盖字符映射、记录布局、指针与整数转换、默认舍入、数学错误和信号处理方式。它们记录本实现的选择；测试通过不等于完整 C89 符合性。包中不提供或复制可执行文件、目标文件，所有二进制输出均由选定的自举编译器和链接器生成。在发布包根目录执行：

```powershell
$ErrorActionPreference='Stop'
& src/c89-implementation-tests/run.ps1 -Snapshot src/bootstrap-c99 -BuildDirectory src/bootstrap-c99/build-from-seed -ScratchDirectory work/implementation-reproduce *> work/implementation-reproduce.log
if(!$?){throw 'Implementation probes failed'}
& src/c89-implementation-tests/audit.ps1 -ScratchDirectory work/implementation-reproduce -VerificationLog work/implementation-reproduce.log
if(!$?){throw 'Implementation probe audit failed'}
```

使用新的测试目录。两代编译器分别执行每个套件。审计比较测试源码与当前模板，并检查 60 个执行检查点，以及 56 对相同的非空产物。布局测试的直接输出版本包含自己的清零循环，链接版本使用 C89 运行库。信号探针通过子进程退出码观察行为。

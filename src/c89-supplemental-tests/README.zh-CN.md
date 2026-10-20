[English](README.md) · 简体中文

# C89 补充验证源码

本测试包仅包含源码，面向已经自举构建的编译器、链接器和运行库，用于补充工具链的种子构建与审计，不能替代它们。包中不提供或复制可执行文件、目标文件。运行脚本只在新的测试目录中生成源码文本；所有二进制输出均由选定的自制工具生成。

完成所选源码快照的构建后，在发布包根目录执行：

```powershell
& src/c89-supplemental-tests/run.ps1 -Snapshot src/bootstrap-c99 -BuildDirectory src/bootstrap-c99/build-from-seed -ScratchDirectory work/supplemental-seed-check *> work/supplemental-seed-check.log
```

默认覆盖 210 对有序头文件组合、30 种完整头文件排列、可执行文件与目标文件模式下的递归宏/字符串化/记号拼接、C locale 的全部 256 个字节转换及边界、记录排序与查找，以及文档所述实现事实探针。测试使用两代编译器。头文件测试并未覆盖所有完整排列；这些测试本身不构成完整 C89 符合性证明。可用 `-Cases` 选择并明确报告测试子集，用于开发时的专项验证。

运行完成后，独立审计生成的测试源码、执行检查点，以及全部 276 对非空产物：

```powershell
& src/c89-supplemental-tests/audit.ps1 -ScratchDirectory work/supplemental-seed-check -VerificationLog work/supplemental-seed-check.log
```

此只读审计不能替代编译器和运行库从种子生成的来源审计。额外覆盖包括：每代编译器在直接输出和目标文件模式下的 69,634 个整数窄化输入及六个静态初始化器，以及 C89 最低翻译容量的组合探针。窄化结果验证的是本实现记录的选择，不是所有 C 实现必须遵循的规则。

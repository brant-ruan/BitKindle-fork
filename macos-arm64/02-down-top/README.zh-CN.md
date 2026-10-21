# BitKindle macOS (Darwin ARM64) - 阶梯演进自举体系 (02-down-top)

## 概述

如果说 `01-top-down` 是从**高层集成**的角度俯瞰完整的系统，那么 `02-down-top` 则是忠实还原了 BitKindle 项目的核心灵魂：**从零（0）到无穷（N）一步步攀登的自举天梯**。

每一个步骤都是一个自包含、可独立验证的里程碑，展现了从一无所有到构建出完整现代软件体系的壮丽跃迁过程。

---

## 阶梯演进目录结构

```text
02-down-top/
├── build-all.sh                   # 一键贯通阶梯演进流水线 (Step 00 ~ Step 05)
│
├── step00-seed/                   # 【阶段 0：破土萌芽】
│   ├── src/                       # seed-arm64.hex, seed_arm64.s
│   ├── decode_seed.sh             # 纯文本转二进制工具
│   ├── run.sh                     # 标准输入输出管道回显测试
│   ├── README.md & .zh-CN.md      # 阶段设计说明
│
├── step01-linker/                 # 【阶段 1：奠定基石】
│   ├── src/                       # 64 位 Mach-O 链接器完整源码 (c1link-macho)
│   ├── test_fixture/              # 测试目标文件生成工具
│   ├── build_and_test.sh          # 符号重定位、签名与执行验证
│   ├── README.md & .zh-CN.md      # Mach-O 结构与 AMFI 签名机制
│
├── step02-compiler/               # 【阶段 2：编译核心】
│   ├── src/                       # c1-arm64 递归下降编译器后端源码
│   ├── tests/                     # 循环、算术与函数调用集成测试用例
│   ├── build_and_test.sh          # 编译、生成 .c1o、链接执行
│   ├── README.md & .zh-CN.md      # AAPCS64 栈帧与代码生成说明
│
├── step03-runtime/                # 【阶段 3：人机交互】
│   ├── src/                       # 原生 Darwin 运行时生成器 (build_runtime.c)
│   ├── tests/                     # 字符串格式化与标准 I/O 测试 (hello.c)
│   ├── build_and_test.sh          # 跨模块链接并验证控制台输出
│   ├── README.md & .zh-CN.md      # Darwin svc #0x80 系统调用规范
│
├── step04-self-hosting/           # 【阶段 4：自举闭环】
│   ├── src/                       # bootstrap_core.c 自举核心
│   ├── test_self_host.sh          # Gen0 -> Stage1 -> Stage2 逐字节 SHA256 强校验
│   ├── README.md & .zh-CN.md      # Byte-for-byte 确定性编译证明
│
└── step05-applications/           # 【阶段 5：登顶之作】
    ├── src/                       # hello, args, minicurl, mario (World 1-1)
    ├── test_all.sh                # 3 款应用全功能测试套件
    └── README.md & .zh-CN.md      # 应用移植与运行指南
```

---

## 快速开始

在当前目录下执行一键阶梯演进脚本：

```bash
chmod +x build-all.sh
./build-all.sh
```

流水线将按顺序依次点亮 Step 00 至 Step 05 的自举里程碑！

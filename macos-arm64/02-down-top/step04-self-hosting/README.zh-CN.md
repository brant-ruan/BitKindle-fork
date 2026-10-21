# 阶段 4：两代自编译闭环与确定性校验 (Step 04 - Self-Hosting)

## 概述

什么是**真正意义上的自举（Self-Hosting）**？
当一个编译器由自身编译生成的二进制程序，再去编译自身的源代码时，如果生成的产物能够与上一代**逐字节完全一致（Byte-for-byte Determinism）**，则证明自举闭环在逻辑与数学意义上彻底达成。

本阶段完成了 BitKindle Darwin ARM64 自举闭环的确定性检验，建立了坚不可摧的安全信任根。

---

## 文件结构

- `src/bootstrap_core.c`: 纯 C 编写的确定性自举核心；
- `test_self_host.sh`: 两代构建生成与 SHA256 逐字节比对脚本。

---

## 核心验证流程

```text
[bootstrap_core.c]
       │
   (clang 宿主编译)
       ▼
[compiler_gen0] (第 0 代)
       │
   (运行自身代码生成)
       ▼
[compiler_stage1.c1o] (第 1 代目标文件) ──(c1link-macho 链接)──► [app_stage1]
       │
   (再次运行自身逻辑)
       ▼
[compiler_stage2.c1o] (第 2 代目标文件)
       │
   [SHA256 逐字节强比对: 7e9e070420... == 7e9e070420...]  ==> [PASS]
```

---

## 运行验证

```bash
chmod +x test_self_host.sh
./test_self_host.sh
```

[English](README.md) · 简体中文

# 技术文档导览

本目录集中介绍实现设计、目标文件格式和数学算法。语言行为与平台约定统一见 [实现约定](../src/C89-IMPLEMENTATION.zh-CN.md)，容量边界见 [限制](../src/C89-LIMITS.zh-CN.md)。

| 文档 | 内容 |
| --- | --- |
| [实现设计](DESIGN.zh-CN.md) | 初始化状态保存、头文件与运行库隔离、自举运行库及测试结构 |
| [C1O1 目标文件格式](C1O-FORMAT.zh-CN.md) | 文件布局、节与符号记录、重定位计算及校验约束 |
| [立方根](algorithms/CBRT.zh-CN.md) | 整数立方根、精确中点比较和定向舍入 |
| [融合乘加](algorithms/FMA.zh-CN.md) | 整数域精确乘加与最终一次舍入 |
| [误差函数](algorithms/ERF.zh-CN.md) | erf/erfc 分段算法、独立参考值及精度限制 |
| [Gamma 函数](algorithms/GAMMA.zh-CN.md) | 递推、反射、渐近展开及验证边界 |

入门阅读顺序：[项目说明](../README.zh-CN.md) → [自举原理](../BOOTSTRAP.zh-CN.md) → [使用手册](../USAGE.zh-CN.md)。需要实际构建时阅读 [逐阶段教程](../WALKTHROUGH.zh-CN.md)。

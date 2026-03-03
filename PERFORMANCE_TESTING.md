# SPHINCS+ 性能测试指南

本目录包含流式 SPHINCS+ 实现的性能测试程序。

## 测试程序

### 1. `test_performance` - 全面性能测试
一个全面的性能测试程序，测试所有可用的 SPHINCS+ 参数集，使用多种消息大小和迭代次数。

**功能特性：**
- 测试所有参数集（SHA2 和 SHAKE 变体）
- 多种消息大小：32、256、1024、8192、32768 字节
- 每个测试进行 100 次迭代以确保计时准确
- 每个参数集的详细指标

**测试指标：**
- 基本参数（哈希大小、树高度等）
- 尺寸指标（私钥、公钥、签名大小）
- 缓冲区使用情况分解
- 密钥生成时间
- 签名性能（平均时间和吞吐量）
- 验证性能（平均时间和吞吐量）
- 内存效率比率

**构建和运行：**
```bash
make test_performance
./build/test_performance
```

**注意：** 该程序运行时间较长（数分钟），因为它对每个参数集都进行 100 次迭代测试。

### 2. `test_performance_quick` - 快速性能测试
用于快速测试单个参数集的简化版本。

**功能特性：**
- 仅测试 SHA2-128f-simple 参数集
- 减少的消息大小：32、256、1024、8192 字节
- 每个测试 10 次迭代
- 与全面版本相同的指标

**构建和运行：**
```bash
make test_performance_quick
./build/test_performance_quick
```

**注意：** 适合快速验证和开发用途。

## 性能指标说明

### 尺寸指标
- **私钥大小：** 存储私钥所需的内存
- **公钥大小：** 存储公钥所需的内存
- **签名大小：** 生成的签名大小（因参数集而异）
- **上下文缓冲区大小：** 签名/验证上下文所需的 RAM

### 缓冲区使用情况分解
显示上下文缓冲区的分配方式：
- **主缓冲区 (TS_MAX_HASH)：** 主哈希存储
- **ADR 结构：** 用于树遍历的地址结构
- **认证路径缓冲区：** 认证路径存储
- **FORS 栈最大值：** FORS 树的最大栈使用量
- **Merkle 栈最大值：** Merkle 树的最大栈使用量
- **WOTS 数字最大值：** WOTS+ 数字存储
- **FORS 节点数组：** 用于 FORS 节点跟踪的数组

### 性能指标
- **签名时间：** 生成签名所需的时间
- **验证时间：** 验证签名所需的时间
- **吞吐量：** 数据处理速率（KB/s）

### 内存效率
- **上下文 vs 签名：** 上下文相对于签名大小的百分比
- **私钥 vs 签名：** 私钥相对于签名大小的百分比
- **公钥 vs 签名：** 公钥相对于签名大小的百分比
- **总密钥大小 vs 签名：** 密钥大小相对于签名大小的百分比

## 测试的参数集

测试程序根据编译时配置自动检测并测试可用的参数集：

### SHA2 参数集
- `sha2_128f_simple` - SHA2-128f 简单模式
- `sha2_128s_simple` - SHA2-128s 简单模式（如果 TS_SUPPORT_S）
- `sha2_192f_simple` - SHA2-192f 简单模式（如果 L3 或 L5）
- `sha2_192s_simple` - SHA2-192s 简单模式（如果 TS_SUPPORT_S 和 L3/L5）
- `sha2_256f_simple` - SHA2-256f 简单模式（如果 L5）
- `sha2_256s_simple` - SHA2-256s 简单模式（如果 TS_SUPPORT_S 和 L5）

### SHAKE 参数集
- `shake_128f_simple` - SHAKE-128f 简单模式
- `shake_128s_simple` - SHAKE-128s 简单模式（如果 TS_SUPPORT_S）
- `shake_192f_simple` - SHAKE-192f 简单模式（如果 L3 或 L5）
- `shake_192s_simple` - SHAKE-192s 简单模式（如果 TS_SUPPORT_S 和 L3/L5）
- `shake_256f_simple` - SHAKE-256f 简单模式（如果 L5）
- `shake_256s_simple` - SHAKE-256s 简单模式（如果 TS_SUPPORT_S 和 L5）

## 结果解读

### 签名性能
- 较小的值表示更快的签名速度
- 签名的计算成本较高（SHA2-128f 通常约 30ms）
- 消息大小对签名时间影响最小（主要由树遍历决定）

### 验证性能
- 比签名快得多（SHA2-128f 通常约 2ms）
- 吞吐量随消息大小增加而提高
- 对于需要多次验证的应用程序至关重要

### 内存效率
- 上下文大小相对于签名的比率越低，表示 RAM 效率越好
- 对于嵌入式和 HSM 应用程序很重要
- 流式实现最大限度地减少了内存使用

## 自定义

### 调整迭代次数
修改测试程序中的 `NUM_ITERATIONS` 常量：
```c
#define NUM_ITERATIONS 10  // 用于快速测试
#define NUM_ITERATIONS 100 // 用于准确结果
```

### 添加消息大小
修改 `message_lengths` 数组：
```c
size_t message_lengths[] = {32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};
```

### 测试特定参数集
修改 `main()` 函数以仅测试特定参数集：
```c
// 仅测试 SHA2-256s
test_parameter_set("sha2_256s_simple", &ts_ps_sha2_256s_simple);
```

## 构建选项

### 使用所有参数集构建
```bash
make clean
make ALL_PARM_SETS=1 test_performance
```

### 使用特定优化级别构建
修改 Makefile 中的 `CFLAGS`：
```makefile
CFLAGS = -Wall -Wextra -Wpedantic -fomit-frame-pointer -O2  # 更快
CFLAGS = -Wall -Wextra -Wpedantic -fomit-frame-pointer -O0  # 用于调试
```

## 故障排除

### 编译错误
- 确保所有必需的源文件都存在
- 检查测试程序中是否包含了 `internal.h`
- 验证编译器支持所需的标志

### 运行时错误
- 检查大参数集的可用 RAM
- 确保提供了随机函数（如果没有则使用确定性回退）
- 验证消息缓冲区已正确分配

### 意外结果
- 运行多次迭代以平均系统噪声
- 关闭其他应用程序以获得准确的计时
- 检查 CPU 频率缩放（禁用以获得一致的结果）

## 注意事项

- 流式实现修改了 R 值生成，使得签名与标准 SPHINCS+ 不兼容
- 所有计时测量使用 `gettimeofday()` 实现微秒精度
- 在实际测量之前执行预热运行
- 内存测量基于静态分配大小

## 进一步分析

进行更详细的分析：
1. 将输出捕获到文件：`./build/test_performance > results.txt`
2. 比较不同优化级别的结果
3. 在不同的硬件平台上测试
4. 分析流式对大消息签名的影响

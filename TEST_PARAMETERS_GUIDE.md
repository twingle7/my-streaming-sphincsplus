# 测试参数修改指南

## 📋 可修改的参数

### 1. 测试文件大小 (TEST_FILE_SIZE_MB)

**位置**：`test_real_world_memory.c` 第 14 行

```c
#define TEST_FILE_SIZE_MB 50  /* 50 MB test file */
```

**说明**：
- 控制测试文件的内存大小
- 默认值：50 MB
- 推荐范围：10 MB - 500 MB
- 注意：不要设置过大，避免测试时间过长

**修改示例**：

```c
// 小文件测试（快速）
#define TEST_FILE_SIZE_MB 10

// 中等文件测试（默认）
#define TEST_FILE_SIZE_MB 50

// 大文件测试（展示内存优势）
#define TEST_FILE_SIZE_MB 200

// 超大文件测试（展示可扩展性）
#define TEST_FILE_SIZE_MB 1000  // 1GB
```

---

### 2. 块大小 (CHUNK_SIZE)

**位置**：`test_real_world_memory.c` 第 16 行

```c
#define CHUNK_SIZE (64 * 1024)  /* 64 KB chunks */
```

**说明**：
- 流式处理时每次读取的块大小
- 默认值：64 KB
- 推荐范围：4 KB - 1 MB
- 注意：块大小会影响性能和内存使用的权衡

**修改示例**：

```c
// 小块（最小内存，但可能较慢）
#define CHUNK_SIZE (4 * 1024)      // 4 KB

// 中等块（平衡性能和内存）
#define CHUNK_SIZE (64 * 1024)     // 64 KB（默认）

// 大块（更好性能，但内存稍大）
#define CHUNK_SIZE (256 * 1024)    // 256 KB

// 超大块（接近传统方法）
#define CHUNK_SIZE (1024 * 1024)   // 1 MB
```

**块大小的影响**：

| 块大小 | 内存需求 | 性能 | 适用场景 |
|--------|---------|------|---------|
| 4 KB | 最小 | 较慢 | 内存极度受限 |
| 64 KB | 低 | 良好 | 平衡（推荐） |
| 256 KB | 中 | 快 | 一般场景 |
| 1 MB | 较高 | 最快 | 接近传统方法 |

---

### 3. 参数集 (parameter set)

**位置**：`test_real_world_memory.c` 第 228 行

```c
const struct ts_parameter_set *ps = &ts_ps_sha2_128f_simple;
```

**说明**：
- 选择 SPHINCS+ 的参数集，影响安全性和性能
- 默认值：`ts_ps_sha2_128f_simple`

**可选参数集**：

```c
// SHA2 参数集（更快，签名更小）
&ts_ps_sha2_128f_simple  // 默认，128-bit 安全，快速
&ts_ps_sha2_128s_simple  // 128-bit 安全，较小签名
&ts_ps_sha2_192f_simple  // 192-bit 安全
&ts_ps_sha2_192s_simple  // 192-bit 安全，较小签名
&ts_ps_sha2_256f_simple  // 256-bit 安全，最安全
&ts_ps_sha2_256s_simple  // 256-bit 安全，最安全且较小签名

// SHAKE 参数集（更安全，但签名更大）
&ts_ps_shake_128f_simple  // 128-bit 安全，SHAKE
&ts_ps_shake_128s_simple  // 128-bit 安全，SHAKE，较小签名
&ts_ps_shake_192f_simple  // 192-bit 安全，SHAKE
&ts_ps_shake_192s_simple  // 192-bit 安全，SHAKE，较小签名
&ts_ps_shake_256f_simple  // 256-bit 安全，SHAKE，最安全
&ts_ps_shake_256s_simple  // 256-bit 安全，SHAKE，最安全且较小签名
```

**参数集对比**：

| 参数集 | 安全性 | 签名大小 | 性能 | 推荐用途 |
|--------|--------|---------|------|---------|
| sha2_128f_simple | 128-bit | ~17 KB | 最快 | 一般应用 |
| sha2_128s_simple | 128-bit | ~8 KB | 快 | 带宽受限 |
| sha2_192f_simple | 192-bit | ~35 KB | 中等 | 高安全性 |
| sha2_256f_simple | 256-bit | ~49 KB | 较慢 | 最高安全性 |
| shake_128f_simple | 128-bit | ~17 KB | 快 | 需要抗量子 |
| shake_256f_simple | 256-bit | ~49 KB | 中等 | 最高抗量子 |

**修改示例**：

```c
// 使用更强的安全性
const struct ts_parameter_set *ps = &ts_ps_sha2_256f_simple;

// 使用更小的签名
const struct ts_parameter_set *ps = &ts_ps_sha2_128s_simple;

// 使用 SHAKE（抗量子）
const struct ts_parameter_set *ps = &ts_ps_shake_128f_simple;
```

---

## 🔧 完整修改示例

### 示例 1：快速测试（小文件）

```c
#define TEST_FILE_SIZE_MB 10   // 10 MB 文件
#define CHUNK_SIZE (64 * 1024) // 64 KB 块

// 在 main() 中
const struct ts_parameter_set *ps = &ts_ps_sha2_128f_simple;
```

**适用场景**：快速验证功能，开发调试

---

### 示例 2：标准测试（默认配置）

```c
#define TEST_FILE_SIZE_MB 50   // 50 MB 文件
#define CHUNK_SIZE (64 * 1024) // 64 KB 块

// 在 main() 中
const struct ts_parameter_set *ps = &ts_ps_sha2_128f_simple;
```

**适用场景**：标准性能测试，平衡性能和真实性

---

### 示例 3：大文件测试（展示内存优势）

```c
#define TEST_FILE_SIZE_MB 200   // 200 MB 文件
#define CHUNK_SIZE (64 * 1024)  // 64 KB 块

// 在 main() 中
const struct ts_parameter_set *ps = &ts_ps_sha2_128f_simple;
```

**适用场景**：展示流式处理在大文件上的优势

---

### 示例 4：内存受限环境测试

```c
#define TEST_FILE_SIZE_MB 100   // 100 MB 文件
#define CHUNK_SIZE (4 * 1024)   // 4 KB 块（最小内存）

// 在 main() 中
const struct ts_parameter_set *ps = &ts_ps_sha2_128s_simple;  // 更小签名
```

**适用场景**：嵌入式设备、IoT 设备等内存受限环境

---

### 示例 5：高安全性测试

```c
#define TEST_FILE_SIZE_MB 50    // 50 MB 文件
#define CHUNK_SIZE (64 * 1024)  // 64 KB 块

// 在 main() 中
const struct ts_parameter_set *ps = &ts_ps_shake_256f_simple;  // 256-bit SHAKE
```

**适用场景**：需要最高安全性和抗量子能力的场景

---

## 📝 修改步骤

1. **打开文件**
   ```bash
   nano test_real_world_memory.c
   # 或使用你喜欢的编辑器
   ```

2. **修改参数**
   - 找到对应的 `#define` 行
   - 修改数值
   - 保存文件

3. **重新编译**
   ```bash
   make test_real_world_memory
   ```

4. **运行测试**
   ```bash
   ./build/test_real_world_memory
   ```

---

## ⚠️ 注意事项

1. **文件大小限制**
   - 确保系统有足够的内存运行测试
   - 大文件测试可能需要较长时间
   - 推荐从小文件开始测试

2. **块大小选择**
   - 太小：性能下降，但内存占用更低
   - 太大：接近传统方法，失去流式优势
   - 推荐范围：4 KB - 256 KB

3. **参数集编译**
   - 某些参数集可能需要特定的编译选项
   - 如果编译失败，检查 Makefile 中的 `DFLAGS`
   - 使用 `make clean && make test_real_world_memory` 重新编译

4. **多次运行**
   - 性能测试可能需要多次运行取平均值
   - 考虑创建脚本自动运行多次测试

---

## 🚀 批量测试脚本示例

如果你想测试不同参数组合，可以创建一个脚本：

```bash
#!/bin/bash
# batch_test.sh

echo "Running batch tests..."

# 测试 1: 小文件
sed -i 's/#define TEST_FILE_SIZE_MB .*/#define TEST_FILE_SIZE_MB 10/' test_real_world_memory.c
make test_real_world_memory
./build/test_real_world_memory > results_10mb.txt

# 测试 2: 中文件
sed -i 's/#define TEST_FILE_SIZE_MB .*/#define TEST_FILE_SIZE_MB 50/' test_real_world_memory.c
make test_real_world_memory
./build/test_real_world_memory > results_50mb.txt

# 测试 3: 大文件
sed -i 's/#define TEST_FILE_SIZE_MB .*/#define TEST_FILE_SIZE_MB 200/' test_real_world_memory.c
make test_real_world_memory
./build/test_real_world_memory > results_200mb.txt

echo "Batch tests complete!"
```

---

## 📊 常用参数组合

| 场景 | 文件大小 | 块大小 | 参数集 | 说明 |
|------|---------|--------|--------|------|
| 开发调试 | 10 MB | 64 KB | sha2_128f | 快速反馈 |
| 标准测试 | 50 MB | 64 KB | sha2_128f | 默认配置 |
| 性能测试 | 100 MB | 256 KB | sha2_128f | 最大性能 |
| 内存测试 | 200 MB | 4 KB | sha2_128s | 最小内存 |
| 安全测试 | 50 MB | 64 KB | shake_256f | 最高安全 |
| IoT 设备 | 50 MB | 4 KB | sha2_128s | 低功耗 |
| 服务器 | 500 MB | 256 KB | sha2_256f | 高安全高性能 |

---

## 💡 最佳实践

1. **先小后大**：从小文件开始测试，确认功能正常后再增大文件
2. **记录结果**：将测试结果保存到文件，便于对比
3. **多次运行**：每个配置运行 3-5 次，取平均值
4. **监控资源**：使用 `top` 或 `htop` 监控内存和 CPU 使用
5. **文档化**：记录测试配置和环境，便于复现

---

## 🌏 中文版测试程序

项目中还提供了中文版的测试程序 `test_real_world_memory_cn.c`，输出信息全部为中文，方便中文用户阅读。

### 编译和运行

```bash
# 编译中文版测试程序
make test_real_world_memory_cn

# 运行测试
./build/test_real_world_memory_cn
```

### 修改中文版参数

中文版测试程序的参数修改方式与英文版完全相同：

1. 打开 `test_real_world_memory_cn.c` 文件
2. 修改相应的宏定义或参数集选择
3. 重新编译：`make test_real_world_memory_cn`
4. 运行测试：`./build/test_real_world_memory_cn`

### 输出示例

```
=================================================================
  真实场景内存节约和性能展示
=================================================================
测试文件大小: 50.00 MB
块大小: 65536 字节 (64.00 KB)

密钥对已生成
正在创建测试文件: 50.00 MB
测试文件创建成功

=================================================================
测试 1: 传统方法（main 分支 - 非流式）
=================================================================
使用: ts_init_sign() + ts_sign() - 需要将整个消息加载到内存

加载文件前的内存: 52056 KB
正在尝试将 50.00 MB 文件加载到 RAM...
加载文件后的内存: 103264 KB
文件使用的内存: 51208 KB (50.01 MB)

正在使用传统方法签名...
签名后的内存: 103320 KB
签名时间: 414082 us (414.08 ms)
签名吞吐量: 120.75 MB/s
签名大小: 17088 字节
✓ 传统方法测试完成

=================================================================
测试 2: 流式方法（incremental-double-pass 分支）
=================================================================
使用: ts_init_sign_double_pass() + ts_update_prf_msg() + ts_sign()
      分块处理消息 - 最小内存需求

流式处理前的内存: 52648 KB
第一遍: 以 65536 字节块处理 50.00 MB...
第一遍完成。使用的内存: 52720 KB
第一遍处理的总字节数: 52428800

第二遍: 以 65536 字节块处理 50.00 MB...
第二遍完成。使用的内存: 52744 KB
第二遍处理的总字节数: 52428800

正在生成签名...
签名后的内存: 52748 KB
签名时间: 33216 us (33.22 ms)
签名吞吐量: 1504.98 MB/s
签名大小: 17088 字节

块缓冲区已释放
最终内存: 52688 KB
✓ 流式方法测试完成

=================================================================
  对比总结
=================================================================

+----------------------+--------------------------+--------------------------+
| 指标                | 传统方法 (main)          | 流式方法 (double-pass)    |
+----------------------+--------------------------+--------------------------+
| 需要的内存          | 50.00 MB                  | 64.00 KB                  |
| 内存节约            | -                        | 49.94 MB (99.9%)          |
| 签名时间            | 414.08 ms                  | 33.22 ms                  |
| 时间开销            | -                        | -380.85 ms (-92.0%)       |
| 签名吞吐量          | 120.75 MB/s                | 1504.98 MB/s                |
| 签名大小            | 17088 字节                 | 17088 字节                 |
+----------------------+--------------------------+--------------------------+
```

---

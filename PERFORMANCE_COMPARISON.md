# SPHINCS+ 性能比较指南

本指南说明如何在原始版本（main分支）和流式修改版本（modify-r-generation分支）之间进行性能比较。

## 测试程序

### `test_performance_orig.c`
为原始版本（main分支）编写的性能测试程序，与修改版本的测试代码完全相同，确保测试条件一致。

**功能特性：**
- 测试单个参数集（SHA2-128f-simple）
- 消息大小：32、256、1024、8192 字节
- 每个测试 10 次迭代
- 输出详细的性能指标

## 使用方法

### 方法 1：手动测试

#### 测试原始版本（main 分支）
```bash
# 切换到 main 分支
git checkout main

# 编译并运行测试
make test_performance_orig
build/test_performance_orig
```

#### 测试修改版本（modify-r-generation 分支）
```bash
# 切换到 modify-r-generation 分支
git checkout modify-r-generation

# 编译并运行测试
make test_performance_quick
build/test_performance_quick
```

### 方法 2：使用比较脚本（推荐）

#### 比较两个分支
```bash
# 自动在两个分支之间切换并测试，生成结果文件
./compare_performance.sh --compare
```

此脚本会：
1. 自动切换到 main 分支并运行测试，保存结果到 `results_main.txt`
2. 自动切换到 modify-r-generation 分支并运行测试，保存结果到 `results_modified.txt`
3. 切换回原始分支

#### 仅测试当前分支
```bash
# 只测试当前所在的分支
./compare_performance.sh
```

## 比较结果

### 查看差异
```bash
# 比较两个版本的文本输出
diff results_main.txt results_modified.txt

# 使用并排比较（如果支持）
diff -y results_main.txt results_modified.txt | less
```

### 关键性能指标对比

#### 1. 签名性能
- **原始版本**：需要两次遍历消息（一次计算 R，一次计算消息摘要）
- **修改版本**：流式处理，只需一次遍历消息
- **预期差异**：大消息时，修改版本应该更快

#### 2. 内存使用
- **原始版本**：需要完整消息在内存中
- **修改版本**：可以分块处理大消息
- **预期差异**：大消息时，修改版本内存使用更少

#### 3. 签名大小
- **原始版本**：标准 SPHINCS+ 签名
- **修改版本**：由于 R 值生成方式改变，签名不同
- **注意**：修改版本的签名与标准 SPHINCS+ 不兼容

#### 4. 验证性能
- **原始版本**：标准 SPHINCS+ 验证
- **修改版本**：验证流程相同
- **预期差异**：验证性能应该相似

## 结果解读示例

### 签名时间对比

假设测试结果为：

```
原始版本 (8192 bytes):
  Total Time: 338,563 us
  Avg Time:   33,857 us
  Throughput: 236.29 KB/s

修改版本 (8192 bytes):
  Total Time: 338,375 us
  Avg Time:   33,838 us
  Throughput: 236.42 KB/s
```

**分析：**
- 对于 8KB 消息，两版本性能接近
- 因为消息较小，两次遍历的开销不明显

### 验证时间对比

```
原始版本 (8192 bytes):
  Total Time: 32,221 us
  Avg Time:   3,222 us
  Throughput: 2,482.85 KB/s

修改版本 (8192 bytes):
  Total Time: 18,537 us
  Avg Time:   1,854 us
  Throughput: 4,315.69 KB/s
```

**分析：**
- 修改版本验证速度更快
- 可能是由于编译优化或其他因素

## 自定义测试

### 修改消息大小
编辑 `test_performance_orig.c` 中的 `msg_sizes` 数组：

```c
size_t msg_sizes[] = {32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768};
int num_sizes = 11;
```

### 修改迭代次数
编辑测试文件中的 `NUM_ITERATIONS` 常量：

```c
#define NUM_ITERATIONS 20  // 增加迭代次数以获得更准确的结果
```

### 测试其他参数集
修改 `main()` 函数以测试不同的参数集：

```c
// 测试 SHA2-256f
test_parameter_set("sha2_256f_simple", &ts_ps_sha2_256f_simple);
```

## 性能基准测试

### 创建基准测试
```bash
# 在主分支创建基准
git checkout main
./compare_performance.sh --compare
mv results_main.txt baseline.txt

# 后续修改后可以与基准比较
diff baseline.txt results_modified.txt
```

### 自动化测试脚本
创建自定义脚本进行多次测试：

```bash
#!/bin/bash
# run_multiple_tests.sh

for i in {1..5}; do
    echo "Run $i"
    make test_performance_orig > "run_$i.txt"
done

# 分析结果
python analyze_results.py run_*.txt
```

## 故障排除

### 编译错误
```bash
# 清理并重新编译
make clean
make test_performance_orig
```

### 分支切换问题
```bash
# 确保没有未提交的更改
git status

# 如果有更改，先提交或暂存
git stash
git checkout main
```

### 结果文件不存在
```bash
# 检查是否生成了结果文件
ls -la results_*.txt

# 如果没有，手动运行测试
build/test_performance_orig > results_main.txt
```

## 注意事项

1. **测试环境一致性**
   - 在相同的硬件上运行两个版本的测试
   - 关闭其他应用程序以减少系统噪声
   - 确保编译器选项一致

2. **多次运行**
   - 每个测试运行多次以获得稳定的平均值
   - 第一次运行可能是"冷启动"，后续运行可能更快

3. **消息大小影响**
   - 小消息：两版本差异可能不明显
   - 大消息：流式版本的优势更明显

4. **签名不兼容**
   - 修改版本的签名与原始版本不兼容
   - 这是预期的行为，因为 R 值生成方式改变了

5. **验证兼容性**
   - 两版本的验证逻辑相同
   - 验证性能应该相似

## 预期结果

### 优势
- **大消息处理**：流式版本在处理大消息时应该表现更好
- **内存使用**：流式版本可以处理更大的消息而不耗尽内存
- **灵活性**：流式版本更适合嵌入式和 HSM 环境

### 劣势
- **签名不兼容**：无法与标准 SPHINCS+ 实现互操作
- **额外复杂性**：流式 API 增加了实现复杂性
- **R 值安全性**：修改后的 R 值生成可能需要额外的安全分析

## 总结

使用这些测试工具可以有效地比较原始版本和流式修改版本的性能。建议：

1. 使用 `compare_performance.sh --compare` 进行自动比较
2. 在不同大小的消息上进行测试
3. 多次运行以获得稳定的平均值
4. 关注内存使用和吞吐量等关键指标
5. 根据实际应用场景选择合适的版本

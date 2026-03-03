# 性能测试补充说明

本文档补充说明为仓库添加的性能测试和比较功能。

## 新增文件

### 测试程序

1. **`test_performance_orig.c`** - 原始版本性能测试
   - 为 main 分支设计的性能测试程序
   - 与 modify-r-generation 分支的测试代码完全相同
   - 确保两个版本的测试条件一致

2. **`compare_performance.sh`** - 自动化比较脚本
   - 自动在两个分支之间切换并运行测试
   - 生成可比较的结果文件
   - 支持单个分支测试和双分支比较模式

### 文档

1. **`PERFORMANCE_COMPARISON.md`** - 性能比较详细指南
   - 完整的使用说明
   - 结果解读指南
   - 自定义测试方法
   - 故障排除建议

### 配置

1. **`.gitignore`** - Git 忽略文件配置
   - 忽略 build 目录和编译产物
   - 忽略测试结果文件
   - 保持仓库清洁

## 快速开始

### 测试当前分支
```bash
make test_performance_orig
build/test_performance_orig
```

### 比较两个版本
```bash
./compare_performance.sh --compare
```

## 测试指标

测试程序测量以下关键性能指标：

1. **尺寸指标**
   - 私钥大小
   - 公钥大小
   - 签名大小
   - 上下文缓冲区大小

2. **性能指标**
   - 密钥生成时间
   - 签名时间（不同消息大小）
   - 验证时间（不同消息大小）
   - 吞吐量

3. **内存效率**
   - 上下文 vs 签名大小比率
   - 密钥 vs 签名大小比率

## 测试配置

当前测试配置：
- **参数集**：SHA2-128f-simple
- **消息大小**：32, 256, 1024, 8192 字节
- **迭代次数**：10 次
- **哈希函数**：SHA2-256

## 使用场景

### 1. 开发验证
在开发过程中快速验证性能变化：
```bash
./compare_performance.sh
```

### 2. 版本比较
比较原始版本和修改版本的性能差异：
```bash
./compare_performance.sh --compare
diff results_main.txt results_modified.txt
```

### 3. 性能回归测试
定期运行测试以检测性能回归：
```bash
# 创建基准
./compare_performance.sh --compare
cp results_main.txt baseline.txt

# 后续测试
./compare_performance.sh --compare
diff baseline.txt results_main.txt
```

## 分支说明

### main 分支
- 原始的 SPHINCS+ 实现
- 标准 R 值生成（依赖消息）
- 需要两次遍历消息

### modify-r-generation 分支
- 流式修改版本
- 独立的 R 值生成（不依赖消息）
- 支持流式消息处理
- 签名与标准 SPHINCS+ 不兼容

## 性能对比要点

### 预期优势（修改版本）
- 大消息处理更快
- 内存使用更少
- 支持流式处理

### 预期劣势（修改版本）
- 签名不兼容
- 实现复杂性增加
- R 值安全性需要额外分析

### 相似性能
- 验证性能
- 小消息处理
- 内存缓冲区大小

## 扩展测试

要测试更多参数集或消息大小，修改测试文件中的配置：

```c
// 测试多个参数集
#if TS_SUPPORT_SHA2
test_parameter_set("sha2_128f_simple", &ts_ps_sha2_128f_simple);
test_parameter_set("sha2_256f_simple", &ts_ps_sha2_256f_simple);
#endif

// 增加消息大小
size_t msg_sizes[] = {32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768};
```

## 故障排除

### 编译问题
```bash
make clean
make test_performance_orig
```

### 分支切换问题
```bash
git status  # 检查未提交的更改
git stash   # 暂存更改
git checkout main
```

### 权限问题
```bash
chmod +x compare_performance.sh
```

## 贡献

如果要为测试程序添加功能或修复问题，请确保：
1. 两个分支的测试逻辑保持一致
2. 测试结果格式兼容比较脚本
3. 更新相关文档

## 参考

- 原始 SPHINCS+ 规范
- tiny-sphincs 实现文档
- PERFORMANCE_COMPARISON.md - 详细比较指南

## 联系

如有问题或建议，请通过 issue tracker 反馈。

# 为什么当前测试无法显示内存节约？

## 核心问题

你的观察是完全正确的！当前的测试确实**无法显示流式处理的内存节约**。让我详细解释原因。

## 问题根源

### 1. **原始版本（main分支）的内存需求**

```c
// 原始签名函数
void ts_init_sign(struct ts_context *ctx,
                  const void *message, size_t len_message,
                  ...) {
    // 第一步：生成随机值 R
    ps->prf_msg(randomness, opt_buffer, message, len_message, ctx);

    // 第二步：计算消息摘要
    ps->hash_msg(message_hash, sizeof message_hash, randomness,
                 message, len_message, ctx);
    // ...
}
```

**内存需求：**
- 需要完整的 `message` 指针和长度
- 内部函数 `prf_msg()` 和 `hash_msg()` 都会遍历整个消息
- **必须将整个消息保存在内存中**
- 如果消息是 100MB，就需要 100MB 的内存

### 2. **流式版本（incremental-double-pass分支）的内存需求**

```c
// 流式签名函数
void ts_init_sign_double_pass(struct ts_context *ctx, ...) {
    // 只初始化上下文，不处理消息
}

void ts_update_prf_msg(const unsigned char *message_chunk, size_t len,
                       struct ts_context *ctx) {
    // 处理消息块（可以是 4KB）
    ps->prf_msg_stream(message_chunk, len, ctx);
}

void ts_finalize_prf_msg(struct ts_context *ctx) {
    // 完成第一遍
}

void ts_update_hash_msg(const unsigned char *message_chunk, size_t len,
                        struct ts_context *ctx) {
    // 第二遍处理消息块
    ps->hash_msg_stream(message_chunk, len, ctx);
}

void ts_finalize_hash_msg(struct ts_context *ctx) {
    // 完成第二遍
}
```

**内存需求：**
- 每次只需要一个消息块（例如 4KB）
- 上下文缓冲区约 1KB
- **不需要将整个消息保存在内存中**
- 即使消息是 100MB，也只需要 ~4KB + ~1KB = ~5KB 内存

## 为什么当前测试无法显示节约？

### 测试代码的问题

```c
// 我们的测试代码
unsigned char *message = malloc(MESSAGE_SIZE);  // 分配 10MB

// 测试1：原始版本
ts_init_sign(&ctx, message, MESSAGE_SIZE, ...);  // 传入整个消息

// 测试2：流式版本
ts_init_sign_double_pass(&ctx, ...);
while (remaining > 0) {
    ts_update_prf_msg(message + offset, chunk_len, &ctx);  // 仍然从已分配的 buffer 读取
    // ...
}
```

**问题：**
1. 两种测试都分配了完整的 10MB 消息缓冲区
2. 流式版本只是从这个缓冲区读取小块数据
3. **内存使用量几乎相同**（都包含 10MB 消息缓冲区）
4. `getrusage()` 测量的是进程的**最大内存使用**，不是瞬时内存
5. 10MB 缓冲区在测试开始时就已经分配，所以两种方法的最大内存都包含这 10MB

### 内存使用分析

```
当前测试的内存使用：
┌─────────────────────────────────────────┐
│  进程内存空间                           │
├─────────────────────────────────────────┤
│  10MB 消息缓冲区  ← 两种方法都有      │
│  ~1KB 上下文缓冲区                     │
│  代码段、栈等                          │
├─────────────────────────────────────────┤
│  总计: ~10MB                           │
└─────────────────────────────────────────┘
```

## 真实的内存节约场景

### 场景 1：从文件读取签名

```c
// 原始版本（无法处理大文件）
unsigned char *message = malloc(1GB);  // ❌ 内存不足
fread(message, 1, 1GB, file);
ts_init_sign(&ctx, message, 1GB, ...);

// 流式版本（可以处理任意大小的文件）
FILE *file = fopen("large_file.bin", "rb");
unsigned char chunk[4096];

ts_init_sign_double_pass(&ctx, ...);
while (fread(chunk, 1, sizeof(chunk), file) > 0) {
    ts_update_prf_msg(chunk, sizeof(chunk), &ctx);
}
ts_finalize_prf_msg(&ctx);

ts_init_hash_msg(&ctx);
rewind(file);  // 回到文件开头
while (fread(chunk, 1, sizeof(chunk), file) > 0) {
    ts_update_hash_msg(chunk, sizeof(chunk), &ctx);
}
ts_finalize_hash_msg(&ctx);
// 只需要 4KB 内存，而不是 1GB！
```

### 场景 2：网络流签名

```c
// 原始版本：必须等待整个消息到达
unsigned char *message = malloc(stream_size);
recv_all(socket, message, stream_size);  // ❌ 需要缓冲整个流
ts_init_sign(&ctx, message, stream_size, ...);

// 流式版本：边接收边处理
ts_init_sign_double_pass(&ctx, ...);
while (len = recv(socket, chunk, sizeof(chunk), 0)) {
    ts_update_prf_msg(chunk, len, &ctx);
}
ts_finalize_prf_msg(&ctx);
// 只需要少量缓冲区
```

## 如何正确测试内存节约？

### 方案 1：模拟磁盘读取（推荐）

```c
// 创建一个临时文件
FILE *temp_file = tmpfile();
fwrite(message, 1, MESSAGE_SIZE, temp_file);
rewind(temp_file);

// 测试：使用流式 API 从文件读取
ts_init_sign_double_pass(&ctx, ...);
unsigned char chunk[4096];
size_t bytes_read;
while ((bytes_read = fread(chunk, 1, sizeof(chunk), temp_file)) > 0) {
    ts_update_prf_msg(chunk, bytes_read, &ctx);
}
ts_finalize_prf_msg(&ctx);

// 此时不再需要大内存缓冲区！
```

### 方案 2：实际测量瞬时内存

使用工具如 `valgrind --tool=massif` 来测量峰值内存：

```bash
# 测试原始版本
valgrind --tool=massif ./test_original

# 测试流式版本
valgrind --tool=massif ./test_streaming

# 比较峰值内存
```

### 方案 3：模拟大消息场景

创建一个测试，尝试签名的消息大小超过可用内存：

```c
// 尝试签名 1GB 消息
#define HUGE_MESSAGE_SIZE (1024 * 1024 * 1024)  // 1GB

// 原始版本：会失败（内存不足）
unsigned char *huge_msg = malloc(HUGE_MESSAGE_SIZE);  // ❌ malloc 失败

// 流式版本：可以成功
// 从磁盘文件或网络逐块读取
```

## 测试结果对比

### 当前测试结果

| 方法 | 最大内存使用 | 说明 |
|------|-------------|------|
| 原始版本 | ~10MB | 包含完整消息缓冲区 |
| 流式版本 | ~10MB | 也包含完整消息缓冲区 |
| **内存节约** | **0MB** | 无法体现优势 ❌ |

### 真实场景下的内存使用

| 方法 | 实际需要 | 说明 |
|------|---------|------|
| 原始版本 | 消息大小 + ~1KB | 必须加载完整消息 |
| 流式版本 | 块大小 + ~1KB | 只需要小块缓冲区 |
| **内存节约** | **消息大小 - 块大小** | 巨大节约 ✅ |

对于 100MB 消息：
- 原始版本：~100MB + 1KB = ~100MB
- 流式版本：4KB + 1KB = ~5KB
- **节约：~99.995MB**

## 总结

### 当前测试的问题
1. ✅ 两种方法都分配了完整消息缓冲区
2. ✅ 内存测量工具显示的是最大内存，不是瞬时内存
3. ✅ 测试无法模拟真实的大消息场景

### 流式处理的真实优势
1. ✅ 可以处理超过可用内存的消息
2. ✅ 适合文件、网络流等连续数据源
3. ✅ 减少内存占用（消息大小 - 块大小）
4. ✅ 更好的可扩展性

### 改进建议
1. ✅ 使用文件 I/O 而非内存缓冲区
2. ✅ 使用专业内存分析工具（valgrind massif）
3. ✅ 测试超大消息场景（超过可用内存）
4. ✅ 模拟真实应用场景（文件签名、流媒体签名）

## 结论

**当前测试无法显示内存节约是预期的**，因为测试框架本身就需要分配完整消息缓冲区。要真正体现流式处理的内存节约，需要在真实场景下测试，例如：
- 从磁盘文件读取并签名
- 从网络流接收并签名
- 处理超过可用内存大小的消息

流式处理的核心价值在于**使签名操作不再受限于可用内存**，这是在当前简单测试中无法直接量化的优势。

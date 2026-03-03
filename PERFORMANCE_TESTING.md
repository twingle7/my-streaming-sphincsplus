# SPHINCS+ Performance Testing Guide

This directory contains performance testing programs for the streaming SPHINCS+ implementation.

## Testing Programs

### 1. `test_performance` - Comprehensive Performance Test
A comprehensive performance testing program that tests all available SPHINCS+ parameter sets with multiple message sizes and iterations.

**Features:**
- Tests all parameter sets (SHA2 and SHAKE variants)
- Multiple message sizes: 32, 256, 1024, 8192, 32768 bytes
- 100 iterations per test for accurate timing
- Detailed metrics for each parameter set

**Metrics Measured:**
- Basic parameters (hash size, tree heights, etc.)
- Size metrics (private key, public key, signature sizes)
- Buffer usage breakdown
- Key generation time
- Signing performance (average time and throughput)
- Verification performance (average time and throughput)
- Memory efficiency ratios

**Build and Run:**
```bash
make test_performance
./build/test_performance
```

**Note:** This program takes significant time to run (several minutes) as it tests all parameter sets with 100 iterations each.

### 2. `test_performance_quick` - Quick Performance Test
A simplified version for quick testing with a single parameter set.

**Features:**
- Tests only the SHA2-128f-simple parameter set
- Reduced message sizes: 32, 256, 1024, 8192 bytes
- 10 iterations per test
- Same metrics as the comprehensive version

**Build and Run:**
```bash
make test_performance_quick
./build/test_performance_quick
```

**Note:** This is ideal for quick verification and development purposes.

## Performance Metrics Explained

### Size Metrics
- **Private Key Size:** Storage required for the private key
- **Public Key Size:** Storage required for the public key
- **Signature Size:** Size of the generated signature (varies by parameter set)
- **Context Buffer Size:** RAM required for the signing/verification context

### Buffer Usage Breakdown
Shows how the context buffer is allocated:
- **Main buffer (TS_MAX_HASH):** Primary hash storage
- **ADR structure:** Address structure for tree traversal
- **Auth path buffer:** Authentication path storage
- **FORS stack max:** Maximum stack usage for FORS trees
- **Merkle stack max:** Maximum stack usage for Merkle trees
- **WOTS digits max:** WOTS+ digit storage
- **FORS nodes array:** Array for FORS node tracking

### Performance Metrics
- **Signing Time:** Time to generate a signature
- **Verification Time:** Time to verify a signature
- **Throughput:** Data processing rate in KB/s

### Memory Efficiency
- **Context vs Signature:** Percentage of signature size used by context
- **Private Key vs Signature:** Percentage of signature size for private key
- **Public Key vs Signature:** Percentage of signature size for public key
- **Total Key Size vs Signature:** Combined key size relative to signature

## Parameter Sets Tested

The test programs automatically detect and test available parameter sets based on compile-time configuration:

### SHA2 Parameter Sets
- `sha2_128f_simple` - SHA2-128f with simple mode
- `sha2_128s_simple` - SHA2-128s with simple mode (if TS_SUPPORT_S)
- `sha2_192f_simple` - SHA2-192f with simple mode (if L3 or L5)
- `sha2_192s_simple` - SHA2-192s with simple mode (if TS_SUPPORT_S and L3/L5)
- `sha2_256f_simple` - SHA2-256f with simple mode (if L5)
- `sha2_256s_simple` - SHA2-256s with simple mode (if TS_SUPPORT_S and L5)

### SHAKE Parameter Sets
- `shake_128f_simple` - SHAKE-128f with simple mode
- `shake_128s_simple` - SHAKE-128s with simple mode (if TS_SUPPORT_S)
- `shake_192f_simple` - SHAKE-192f with simple mode (if L3 or L5)
- `shake_192s_simple` - SHAKE-192s with simple mode (if TS_SUPPORT_S and L3/L5)
- `shake_256f_simple` - SHAKE-256f with simple mode (if L5)
- `shake_256s_simple` - SHAKE-256s with simple mode (if TS_SUPPORT_S and L5)

## Interpreting Results

### Signing Performance
- Smaller values indicate faster signing
- Signatures are computationally expensive (typically ~30ms for SHA2-128f)
- Message size has minimal impact on signing time (dominated by tree traversal)

### Verification Performance
- Much faster than signing (typically ~2ms for SHA2-128f)
- Throughput increases with message size
- Critical for applications with many verifications

### Memory Efficiency
- Lower context size relative to signature indicates better RAM efficiency
- Important for embedded and HSM applications
- The streaming implementation minimizes memory usage

## Customization

### Adjust Iterations
Modify the `NUM_ITERATIONS` constant in the test program:
```c
#define NUM_ITERATIONS 10  // For quick testing
#define NUM_ITERATIONS 100 // For accurate results
```

### Add Message Sizes
Modify the `message_lengths` array:
```c
size_t message_lengths[] = {32, 64, 128, 256, 512, 1024, 2048, 4096, 8192};
```

### Test Specific Parameter Sets
Modify the `main()` function to test only specific parameter sets:
```c
// Test only SHA2-256s
test_parameter_set("sha2_256s_simple", &ts_ps_sha2_256s_simple);
```

## Build Options

### Build with All Parameter Sets
```bash
make clean
make ALL_PARM_SETS=1 test_performance
```

### Build with Specific Optimization Level
Modify `CFLAGS` in Makefile:
```makefile
CFLAGS = -Wall -Wextra -Wpedantic -fomit-frame-pointer -O2  # Faster
CFLAGS = -Wall -Wextra -Wpedantic -fomit-frame-pointer -O0  # For debugging
```

## Troubleshooting

### Compilation Errors
- Ensure all required source files are present
- Check that `internal.h` is included in test programs
- Verify compiler supports required flags

### Runtime Errors
- Check available RAM for large parameter sets
- Ensure random function is provided (uses deterministic fallback if not)
- Verify message buffers are properly allocated

### Unexpected Results
- Run multiple iterations to average out system noise
- Close other applications for accurate timing
- Check CPU frequency scaling (disable for consistent results)

## Notes

- The streaming implementation modifies R value generation, making signatures incompatible with standard SPHINCS+
- All timing measurements use `gettimeofday()` for microsecond precision
- Warm-up runs are performed before actual measurements
- Memory measurements are based on static allocation sizes

## Further Analysis

For more detailed analysis:
1. Capture output to file: `./build/test_performance > results.txt`
2. Compare results across different optimization levels
3. Test on different hardware platforms
4. Analyze the impact of streaming on large message signing

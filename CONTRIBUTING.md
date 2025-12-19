# Contributing to DynamoRIO ChampSim Tracer

Thank you for your interest in contributing! This document provides guidelines for contributing to the project.

## Getting Started

1. **Fork the repository** on GitHub
2. **Clone your fork** locally:
   ```bash
   git clone https://github.com/YOUR_USERNAME/dynamorio_champsim_tracer.git
   cd dynamorio_champsim_tracer
   ```
3. **Set up DynamoRIO** following the README instructions
4. **Build the project** to ensure everything works:
   ```bash
   ./build.sh
   ```

## Development Workflow

### Making Changes

1. **Create a branch** for your changes:
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes** following the coding style below

3. **Test your changes**:
   ```bash
   # Build
   ./build.sh
   
   # Test with the included test program
   make -f Makefile.test
   $DYNAMORIO_HOME/bin64/drrun -c build/libchampsim_tracer.so -- ./test_program
   
   # Verify trace was created
   ls -lh champsim.trace
   ```

4. **Commit your changes**:
   ```bash
   git add .
   git commit -m "Brief description of changes"
   ```

5. **Push to your fork**:
   ```bash
   git push origin feature/your-feature-name
   ```

6. **Create a Pull Request** on GitHub

## Coding Style

### C++ Code

- Follow the existing code style in `champsim_tracer.cpp`
- Use meaningful variable names
- Add comments for complex logic
- Keep functions focused and reasonably sized

### Example

```cpp
// Good
static void
at_memory_read(app_pc addr)
{
    void *drcontext = dr_get_current_drcontext();
    per_thread_t *data = (per_thread_t *)drmgr_get_tls_field(drcontext, tls_idx);
    
    write_to_set(data->curr_instr.source_memory,
                 data->curr_instr.source_memory + NUM_INSTR_SOURCES,
                 (unsigned long long)addr);
}
```

### Documentation

- Update README.md if you add new features
- Update EXAMPLES.md if you add new usage patterns
- Add comments to explain non-obvious code

## Testing

### Minimal Testing

Before submitting a PR, ensure:

1. **Code compiles** without warnings:
   ```bash
   ./build.sh 2>&1 | grep -i warning
   # Should have no output
   ```

2. **Basic functionality works**:
   ```bash
   # Create trace
   $DYNAMORIO_HOME/bin64/drrun -c build/libchampsim_tracer.so -t 1000 -- /bin/ls
   
   # Verify size
   test -f champsim.trace && echo "OK" || echo "FAIL"
   test $(stat -c%s champsim.trace) -eq 64000 && echo "Size OK" || echo "Size mismatch"
   ```

3. **Trace format is correct**:
   ```bash
   # Each instruction should be exactly 64 bytes
   python3 << 'EOF'
   import os
   size = os.path.getsize('champsim.trace')
   assert size % 64 == 0, f"Trace size {size} is not multiple of 64"
   print(f"Trace contains {size // 64} instructions - OK")
   EOF
   ```

### Comprehensive Testing (for major changes)

For significant changes, please test on:

1. **Different architectures** (if possible):
   - x86-64
   - ARM64 (if available)
   
2. **Different programs**:
   - Simple C programs
   - C++ programs
   - System utilities (ls, grep, etc.)
   
3. **Different trace parameters**:
   - Various -s (skip) values
   - Various -t (trace) values
   - Large traces (10M+ instructions)

## What to Contribute

### High Priority

- **Bug fixes**: Always welcome!
- **Performance improvements**: Especially reducing instrumentation overhead
- **Documentation improvements**: Examples, clarifications, fixes
- **ARM/RISC-V testing**: Testing on non-x86 architectures

### Feature Ideas

- **More accurate branch taken detection**: Track BB transitions
- **SIMD register support**: Track XMM/YMM/ZMM registers
- **Instruction categorization**: Add instruction types (ALU, FP, memory, etc.)
- **Online compression**: Compress traces during generation
- **Multi-format output**: Support other trace formats
- **Sampling**: Trace only every Nth instruction for lower overhead

### Not Recommended

- Changing the trace format (breaks compatibility with ChampSim)
- Adding dependencies (keep it lightweight)
- Platform-specific code (unless absolutely necessary)

## Reporting Issues

When reporting issues, please include:

1. **DynamoRIO version**: `$DYNAMORIO_HOME/bin64/drrun -version`
2. **Platform**: OS, architecture, kernel version
3. **Command used**: Full drrun command line
4. **Expected behavior**: What you expected to happen
5. **Actual behavior**: What actually happened
6. **Error output**: Complete error messages

### Example Issue Report

```
**Environment:**
- DynamoRIO: 10.0.0
- OS: Ubuntu 22.04 LTS
- Arch: x86-64
- Kernel: 5.15.0

**Command:**
$DYNAMORIO_HOME/bin64/drrun -c build/libchampsim_tracer.so -t 10000 -- ./my_program

**Expected:**
Trace file with 10000 instructions (640000 bytes)

**Actual:**
Trace file is empty (0 bytes)

**Error Output:**
[none]

**Additional Info:**
Program runs normally without DynamoRIO.
```

## Code Review

All contributions go through code review. Here's what reviewers look for:

1. **Correctness**: Does it work as intended?
2. **Compatibility**: Does it maintain trace format compatibility?
3. **Code quality**: Is it readable and maintainable?
4. **Documentation**: Is it properly documented?
5. **Testing**: Has it been tested?

## License

By contributing, you agree that your contributions will be licensed under the Apache License 2.0, the same license as the project.

## Questions?

Feel free to open an issue for any questions about contributing!

## Thank You!

Your contributions help make this tool better for everyone in the computer architecture research community!

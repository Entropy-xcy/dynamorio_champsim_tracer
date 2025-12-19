# Architecture Support Notes

This document provides architecture-specific information for the DynamoRIO ChampSim tracer.

## Supported Architectures

### x86-32 (IA-32)
- **Status**: ✅ Fully supported
- **Tested**: Yes
- **Scratch registers**: EAX, EBX
- **Notes**: Original ChampSim target architecture

### x86-64 (AMD64)
- **Status**: ✅ Fully supported  
- **Tested**: Yes
- **Scratch registers**: RAX, RBX
- **Notes**: Most common architecture for ChampSim

### ARM32 (AArch32)
- **Status**: ✅ Supported
- **Tested**: Limited
- **Scratch registers**: R0, R1
- **Notes**: Requires DynamoRIO ARM build

### ARM64 (AArch64)
- **Status**: ✅ Supported
- **Tested**: Limited
- **Scratch registers**: X0, X1
- **Notes**: Growing in popularity for server workloads

### RISC-V 64-bit
- **Status**: ⚠️ Experimental
- **Tested**: No
- **Scratch registers**: A0, A1
- **Notes**: Depends on DynamoRIO RISC-V support (experimental)

## Building for Different Architectures

### Cross-Compilation

#### ARM32 on x86-64

```bash
# Install ARM toolchain
sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf

# Build DynamoRIO for ARM32
cd dynamorio
mkdir build-arm32 && cd build-arm32
cmake -DCMAKE_TOOLCHAIN_FILE=../make/toolchain-arm32.cmake ..
make -j$(nproc)

# Build tracer for ARM32
cd ../../
mkdir build-arm32 && cd build-arm32
cmake -DDynamoRIO_DIR=../dynamorio/build-arm32/cmake \
      -DCMAKE_TOOLCHAIN_FILE=../dynamorio/make/toolchain-arm32.cmake ..
make
```

#### ARM64 on x86-64

```bash
# Install ARM64 toolchain
sudo apt-get install gcc-aarch64-linux-gnu g++-aarch64-linux-gnu

# Build DynamoRIO for ARM64
cd dynamorio
mkdir build-arm64 && cd build-arm64
cmake -DCMAKE_TOOLCHAIN_FILE=../make/toolchain-arm64.cmake ..
make -j$(nproc)

# Build tracer for ARM64
cd ../../
mkdir build-arm64 && cd build-arm64
cmake -DDynamoRIO_DIR=../dynamorio/build-arm64/cmake \
      -DCMAKE_TOOLCHAIN_FILE=../dynamorio/make/toolchain-arm64.cmake ..
make
```

### Native Compilation

On the target architecture (e.g., on a Raspberry Pi, ARM server, or RISC-V board):

```bash
# Install DynamoRIO
# Follow https://dynamorio.org/page_building.html for your architecture

# Build tracer
mkdir build && cd build
cmake -DDynamoRIO_DIR=$DYNAMORIO_HOME/cmake ..
make
```

## Architecture-Specific Differences

### Register Encoding

Register IDs differ across architectures in the trace files:

| Architecture | Register Example | DR Reg ID | Notes |
|-------------|------------------|-----------|-------|
| x86-64 | RAX | Varies | x86 GPRs |
| ARM64 | X0 | Varies | ARM64 GPRs |
| ARM32 | R0 | Varies | ARM32 GPRs |
| RISC-V | A0 | Varies | RISC-V GPRs |

These IDs are architecture-specific but semantically correct for each architecture.

### Memory Addressing

Different architectures have different addressing modes, but the tracer captures the effective addresses uniformly across all architectures.

### Branch Behavior

Branch instruction encoding varies by architecture:
- **x86**: Many branch variants (JE, JNE, JMP, etc.)
- **ARM**: Conditional execution and branch instructions
- **RISC-V**: Simple branch instructions

All are detected and traced as branches, though the limitation of marking all branches as "taken" applies to all architectures.

## Testing on Different Architectures

### Verification Steps

1. **Build the test program**:
   ```bash
   gcc -O2 -g -o test_program test_program.c
   ```

2. **Generate a small trace**:
   ```bash
   $DYNAMORIO_HOME/bin64/drrun -c ./libchampsim_tracer.so \
     -o test.trace -t 1000 -- ./test_program
   ```

3. **Verify trace format**:
   ```bash
   # Should be exactly 64000 bytes (1000 instructions * 64 bytes)
   ls -l test.trace
   
   # Check it's a multiple of 64
   SIZE=$(stat -c%s test.trace)
   echo $((SIZE % 64))  # Should output 0
   ```

4. **Inspect trace content**:
   ```bash
   # Dump first instruction (64 bytes in hex)
   xxd -l 64 test.trace
   ```

## Performance Characteristics by Architecture

### x86-64
- Overhead: 2-15x typical
- Well-optimized in DynamoRIO

### ARM64
- Overhead: 3-20x typical
- Good DynamoRIO support

### ARM32
- Overhead: 5-25x typical
- Varies by device

### RISC-V
- Overhead: Unknown (experimental)
- Limited optimization in DynamoRIO

## Known Issues by Architecture

### All Architectures
- Branch taken detection limitation (see LIMITATIONS.md)

### ARM-Specific
- Thumb mode may have slightly different performance characteristics
- NEON/SIMD registers not currently tracked

### RISC-V-Specific
- Experimental support in DynamoRIO
- May have stability issues
- Limited testing

## Future Work

### High Priority
1. Extensive testing on ARM64 (server workloads)
2. RISC-V validation when DynamoRIO support matures

### Medium Priority
1. SIMD register tracking (NEON, SVE for ARM)
2. Architecture-specific optimizations

### Low Priority
1. Support for other architectures as DynamoRIO adds them

## Contributing Architecture Support

If you test on a new architecture:
1. Document your setup
2. Share performance characteristics
3. Report any issues
4. Contribute fixes if needed

See CONTRIBUTING.md for details.

## References

- [DynamoRIO Platform Support](https://dynamorio.org/page_platforms.html)
- [DynamoRIO Cross-Compilation](https://dynamorio.org/page_cross_compile.html)
- [ARM Developer Documentation](https://developer.arm.com/)
- [RISC-V Specification](https://riscv.org/specifications/)

# Usage Examples

This file contains practical examples of using the DynamoRIO ChampSim tracer.

## Basic Usage

### 1. Build the Test Program

```bash
make -f Makefile.test
```

### 2. Trace the Test Program

```bash
# Basic trace with defaults (1M instructions)
$DYNAMORIO_HOME/bin64/drrun -c ./build/libchampsim_tracer.so -- ./test_program

# This creates: champsim.trace
```

### 3. Trace with Custom Parameters

```bash
# Skip first 1000 instructions, trace next 5000
$DYNAMORIO_HOME/bin64/drrun -c ./build/libchampsim_tracer.so \
  -s 1000 -t 5000 -o my_trace.champsim -- ./test_program 15
```

## Advanced Examples

### Tracing System Commands

```bash
# Trace ls command
$DYNAMORIO_HOME/bin64/drrun -c ./build/libchampsim_tracer.so \
  -o traces/ls.champsim -t 100000 -- /bin/ls -la

# Trace grep
$DYNAMORIO_HOME/bin64/drrun -c ./build/libchampsim_tracer.so \
  -o traces/grep.champsim -t 200000 -- grep -r "pattern" /some/dir
```

### Tracing SPEC CPU Benchmarks

```bash
# Example with mcf from SPEC CPU 2017
$DYNAMORIO_HOME/bin64/drrun -c ./build/libchampsim_tracer.so \
  -o traces/mcf.champsim \
  -s 30000000000 \
  -t 200000000 \
  -- ./mcf_base.gcc inp.in

# Example with gcc benchmark  
$DYNAMORIO_HOME/bin64/drrun -c ./build/libchampsim_tracer.so \
  -o traces/gcc.champsim \
  -s 10000000000 \
  -t 200000000 \
  -- ./gcc_base.gcc input.c -O3
```

### Multi-threaded Programs

The tracer automatically handles multi-threaded programs:

```bash
# Trace a multi-threaded application
$DYNAMORIO_HOME/bin64/drrun -c ./build/libchampsim_tracer.so \
  -o traces/multithreaded.champsim \
  -- ./my_parallel_app --threads 4
```

## Compressing Traces

Traces compress very well with xz:

```bash
# Compress a trace (typically achieves <1 byte/instruction)
xz -9 champsim.trace

# Decompress when needed
xz -d champsim.trace.xz
```

## Analyzing Traces

### Check Trace Size

```bash
# Each instruction is 64 bytes
ls -lh champsim.trace

# Calculate number of instructions
BYTES=$(stat -f%z champsim.trace 2>/dev/null || stat -c%s champsim.trace)
INSTRUCTIONS=$((BYTES / 64))
echo "Trace contains $INSTRUCTIONS instructions"
```

### Quick Trace Inspection

```bash
# Dump first instruction in hex
xxd -l 64 champsim.trace

# Count non-zero branch instructions
# (This is an advanced example requiring a small script)
python3 << 'EOF'
import struct
import sys

with open('champsim.trace', 'rb') as f:
    branches = 0
    total = 0
    while True:
        data = f.read(64)  # Each instruction is 64 bytes
        if not data or len(data) < 64:
            break
        total += 1
        # Parse instruction: ip(8), is_branch(1), branch_taken(1), ...
        is_branch = data[8]
        if is_branch:
            branches += 1
    print(f"Total instructions: {total}")
    print(f"Branch instructions: {branches}")
    print(f"Branch percentage: {100.0 * branches / total if total > 0 else 0:.2f}%")
EOF
```

## Architecture-Specific Usage

### ARM32/ARM64

```bash
# On ARM machine or using qemu-arm
$DYNAMORIO_HOME/bin32/drrun -c ./build/libchampsim_tracer.so \
  -o traces/arm_program.champsim -- ./arm_executable

# ARM64
$DYNAMORIO_HOME/bin64/drrun -c ./build/libchampsim_tracer.so \
  -o traces/aarch64_program.champsim -- ./aarch64_executable
```

### RISC-V (Experimental)

```bash
# On RISC-V machine or emulator
$DYNAMORIO_HOME/bin64/drrun -c ./build/libchampsim_tracer.so \
  -o traces/riscv_program.champsim -- ./riscv_executable
```

## Using Traces with ChampSim

Once you have a trace, use it with ChampSim:

```bash
# In ChampSim directory
./bin/champsim --warmup_instructions=1000000 \
              --simulation_instructions=10000000 \
              /path/to/traces/my_trace.champsim
```

## Performance Tips

1. **Skip initialization code**: Use `-s` to skip the first N instructions (usually initialization)
2. **Trace representative regions**: Identify interesting program phases using profiling
3. **Compress immediately**: Compress traces right after generation to save disk space
4. **Use SSDs**: Trace generation is I/O intensive, SSDs help significantly

## Troubleshooting

### Trace file is empty

```bash
# Check that instructions were actually traced
# If your program executes fewer instructions than -s (skip), nothing will be traced
# Try reducing or removing the -s parameter
```

### Out of disk space

```bash
# Reduce the number of traced instructions with -t
# Or immediately pipe to compression:
$DYNAMORIO_HOME/bin64/drrun -c ./build/libchampsim_tracer.so \
  -o /dev/stdout -- ./program | xz -9 > trace.champsim.xz
```

### Program crashes under DynamoRIO

```bash
# Try with DynamoRIO debugging enabled
$DYNAMORIO_HOME/bin64/drrun -stderr_mask 0xc \
  -c ./build/libchampsim_tracer.so -- ./program
```

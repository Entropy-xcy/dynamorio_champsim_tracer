# Quick Start Guide

Get started with the DynamoRIO ChampSim Tracer in 5 minutes!

## Prerequisites

- Linux system (x86-64, ARM, or RISC-V)
- GCC or Clang compiler
- CMake 3.7 or later
- Git

## Step 1: Install DynamoRIO

```bash
# Download DynamoRIO (adjust version as needed)
wget https://github.com/DynamoRIO/dynamorio/releases/download/release_10.0.0/DynamoRIO-Linux-10.0.0.tar.gz
tar xzf DynamoRIO-Linux-10.0.0.tar.gz
export DYNAMORIO_HOME=$(pwd)/DynamoRIO-Linux-10.0.0
```

## Step 2: Clone and Build This Tracer

```bash
# Clone the repository
git clone https://github.com/Entropy-xcy/dynamorio_champsim_tracer.git
cd dynamorio_champsim_tracer

# Build
./build.sh
```

## Step 3: Test It!

```bash
# Build the test program
make -f Makefile.test

# Generate a trace
$DYNAMORIO_HOME/bin64/drrun -c build/libchampsim_tracer.so \
  -o my_first_trace.champsim -t 10000 -- ./test_program

# Verify the trace was created
ls -lh my_first_trace.champsim
# Should be 640000 bytes (10000 instructions × 64 bytes)
```

## Step 4: Use with ChampSim

```bash
# Clone ChampSim (if you haven't already)
git clone https://github.com/ChampSim/ChampSim.git
cd ChampSim

# Build ChampSim
./config.sh champsim_config.json
make

# Run simulation with your trace
./bin/champsim --warmup_instructions=1000 \
               --simulation_instructions=10000 \
               ../dynamorio_champsim_tracer/my_first_trace.champsim
```

## Next Steps

### Generate Real Traces

Trace a real program (e.g., `ls`):

```bash
$DYNAMORIO_HOME/bin64/drrun -c build/libchampsim_tracer.so \
  -o traces/ls.champsim -s 1000 -t 100000 -- /bin/ls -la
```

### Compress Traces

Traces compress very well:

```bash
xz -9 traces/ls.champsim
# Creates ls.champsim.xz (typically <1 byte per instruction)
```

### Trace SPEC Benchmarks

```bash
# Example with SPEC CPU benchmark
$DYNAMORIO_HOME/bin64/drrun -c build/libchampsim_tracer.so \
  -o traces/spec_mcf.champsim \
  -s 30000000000 \
  -t 200000000 \
  -- ./mcf_base.gcc inp.in
```

## Command-Line Options

| Option | Description | Default | Example |
|--------|-------------|---------|---------|
| `-o` | Output file | champsim.trace | `-o my_trace.champsim` |
| `-s` | Skip N instructions | 0 | `-s 1000000` (skip 1M) |
| `-t` | Trace N instructions | 1000000 | `-t 10000000` (trace 10M) |

## Typical Workflow

1. **Identify target program**: Choose what to trace
2. **Find interesting region**: Use profiling to find the hot portion
3. **Set skip count**: Skip initialization (`-s` option)
4. **Set trace count**: Trace representative region (`-t` option)
5. **Generate trace**: Run with DynamoRIO
6. **Compress**: Use `xz` to save space
7. **Simulate**: Run in ChampSim

## Example: Tracing a Program Phase

```bash
# Step 1: Profile to find interesting code
# (Use perf, gprof, or other profiler)

# Step 2: Generate trace skipping initialization
$DYNAMORIO_HOME/bin64/drrun -c build/libchampsim_tracer.so \
  -o program_phase.champsim \
  -s 10000000 \
  -t 50000000 \
  -- ./my_program input.dat

# Step 3: Compress immediately
xz -9 program_phase.champsim

# Step 4: Verify
xz -d program_phase.champsim.xz
SIZE=$(stat -c%s program_phase.champsim)
INSTR=$((SIZE / 64))
echo "Trace contains $INSTR instructions"
```

## Troubleshooting

### Build fails
```bash
# Make sure DYNAMORIO_HOME is set
echo $DYNAMORIO_HOME
# Should point to your DynamoRIO installation

# Try cleaning and rebuilding
rm -rf build
./build.sh
```

### Trace is empty
```bash
# Reduce or remove -s (skip) parameter
$DYNAMORIO_HOME/bin64/drrun -c build/libchampsim_tracer.so \
  -o test.trace -s 0 -t 1000 -- ./test_program
```

### Program crashes
```bash
# Enable DynamoRIO debugging
$DYNAMORIO_HOME/bin64/drrun -stderr_mask 0xc \
  -c build/libchampsim_tracer.so -- ./program
```

## Architecture-Specific Notes

### ARM64
```bash
# On ARM64 system, same commands work
$DYNAMORIO_HOME/bin64/drrun -c build/libchampsim_tracer.so \
  -o arm_trace.champsim -- ./arm_program
```

### Cross-compilation
See [ARCHITECTURE.md](ARCHITECTURE.md) for cross-compilation instructions.

## Further Reading

- **[README.md](README.md)**: Comprehensive documentation
- **[EXAMPLES.md](EXAMPLES.md)**: More usage examples
- **[LIMITATIONS.md](LIMITATIONS.md)**: Known limitations
- **[PIN_vs_DYNAMORIO.md](PIN_vs_DYNAMORIO.md)**: Comparison with PIN
- **[ARCHITECTURE.md](ARCHITECTURE.md)**: Multi-architecture support
- **[CONTRIBUTING.md](CONTRIBUTING.md)**: How to contribute

## Getting Help

- Check [LIMITATIONS.md](LIMITATIONS.md) for known issues
- Search existing issues on GitHub
- Open a new issue with details

## Summary

That's it! You now have:
- ✅ DynamoRIO installed
- ✅ Tracer built
- ✅ Test trace generated
- ✅ Ready to trace real programs

Start tracing and happy simulating! 🚀

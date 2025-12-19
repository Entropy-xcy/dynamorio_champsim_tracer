# Trace Verification Guide

This document explains how to verify that the DynamoRIO tracer produces correct and compatible traces.

## Quick Verification (Without PIN)

The DynamoRIO tracer can be verified independently:

```bash
# Run the verification script
./verify_tracer.sh
```

This script:
1. Builds the test program (Fibonacci)
2. Builds the DynamoRIO tracer
3. Generates a trace with deterministic workload
4. Validates trace format (64 bytes per instruction)
5. Checks trace content for sanity

## Full Comparison with PIN Tracer

To verify complete compatibility with the original PIN tracer:

### Step 1: Get the PIN Tracer

```bash
# Clone ChampSim repository
git clone https://github.com/ChampSim/ChampSim.git
cd ChampSim/tracer/pin

# Download PIN (version 3.22 or compatible)
wget https://software.intel.com/sites/landingpage/pintool/downloads/pin-3.22-98547-g7a303a835-gcc-linux.tar.gz
tar zxf pin-3.22-98547-g7a303a835-gcc-linux.tar.gz
export PIN_ROOT=$(pwd)/pin-3.22-98547-g7a303a835-gcc-linux

# Build PIN tracer
make
```

### Step 2: Build Test Program

Both tracers need to trace the same binary:

```bash
cd /path/to/dynamorio_champsim_tracer
make -f Makefile.test
```

### Step 3: Generate Traces

Generate traces with identical parameters:

```bash
# DynamoRIO trace
$DYNAMORIO_HOME/bin64/drrun -c build/libchampsim_tracer.so \
    -o fibonacci_dr.trace -s 0 -t 10000 -- ./test_program 10

# PIN trace
$PIN_ROOT/pin -t /path/to/ChampSim/tracer/pin/obj-intel64/champsim_tracer.so \
    -o fibonacci_pin.trace -s 0 -t 10000 -- ./test_program 10
```

**Important:** Use the same binary for both tracers!

### Step 4: Compare Traces

```bash
python3 compare_traces.py fibonacci_pin.trace fibonacci_dr.trace
```

The comparison tool checks:
- **Instruction Pointers (IP)**: Must match exactly
- **Branch Information**: is_branch and branch_taken must match
- **Memory Addresses**: Source and destination addresses must match
- **Register Count**: Number of registers used (may differ in IDs but not count)

## Expected Differences

### Register IDs

PIN and DynamoRIO use different internal register numbering:
- **PIN**: Uses PIN's register enumeration
- **DynamoRIO**: Uses DR_REG_* enumeration

**This is expected and acceptable.** The comparison tool counts registers rather than comparing IDs.

Example:
```
PIN might use:     src_regs = [1, 5, 7, 0]
DynamoRIO uses:    src_regs = [16, 20, 22, 0]
Both have 3 source registers ✓
```

### What Should Match Exactly

These fields **must match** for traces to be compatible:

1. **Number of Instructions**: Both traces should have same instruction count
2. **Instruction Pointers (IP)**: Must match exactly for each instruction
3. **Branch Behavior**: 
   - `is_branch` flag must match
   - `branch_taken` value must match for conditional branches
4. **Memory Addresses**: 
   - Source memory addresses must match
   - Destination memory addresses must match
   - Order may vary but sets should be equal

## Interpreting Results

### Perfect Match ✅
```
Comparison complete:
  Total instructions: 10000
  Mismatches: 0
  ✅ TRACES MATCH PERFECTLY!
```

### Acceptable Differences ⚠️
```
Comparison complete:
  Total instructions: 10000
  Mismatches: 15
  ❌ 15 differences found

MISMATCH at instruction 42:
  Fields differ: src_regs_count(3 vs 4)
  ...
```

Register count differences are acceptable if:
- They're consistent (e.g., DR always has +1 due to flag register)
- Memory and branch behavior still match

### Unacceptable Differences ❌
```
MISMATCH at instruction 100:
  Fields differ: IP, branch_taken
  ...
```

If you see:
- Different instruction pointers → tracers are out of sync
- Different branch_taken values → branch detection is broken
- Different memory addresses → memory tracking is incorrect

## Troubleshooting

### Traces Have Different Lengths

**Problem:** One trace has more/fewer instructions than the other

**Possible Causes:**
1. Different binaries traced (rebuild test program)
2. Different skip (-s) or trace (-t) parameters
3. One tracer crashed/exited early

**Solution:** Ensure both tracers use same binary and parameters

### Branch Taken Mismatch

**Problem:** `branch_taken` values differ for same instruction

**This should NOT happen** - it indicates a bug in branch detection.

**Debug steps:**
1. Check which branch instructions differ
2. Verify the branch resolution logic
3. Test with simpler program (e.g., single if-statement)

### Memory Address Mismatch

**Problem:** Memory addresses differ

**Possible Causes:**
1. ASLR (Address Space Layout Randomization) - but should affect both equally
2. Different execution paths (would show in IP mismatch first)
3. Bug in memory instrumentation

**Solution:**
- Disable ASLR: `setarch $(uname -m) -R ./test_program`
- Check if IPs also mismatch (indicates different execution)

## Automated Testing

Create a test suite:

```bash
#!/bin/bash
# test_suite.sh - Run multiple tests

TESTS=(
    "fibonacci:10:1000"
    "fibonacci:15:5000"
    "simple:0:500"
)

for test in "${TESTS[@]}"; do
    IFS=':' read -r prog arg count <<< "$test"
    echo "Testing: $prog($arg) with $count instructions"
    
    # Generate both traces
    # ... (generate PIN and DR traces)
    
    # Compare
    if python3 compare_traces.py pin.trace dr.trace; then
        echo "✓ $prog($arg): PASS"
    else
        echo "✗ $prog($arg): FAIL"
        exit 1
    fi
done

echo "All tests passed!"
```

## Reporting Issues

If you find incompatibilities:

1. **Reproduce** with minimal test case
2. **Capture** both traces: `pin.trace` and `dr.trace`
3. **Run comparison**: Save output of `compare_traces.py`
4. **Check binaries**: Confirm both traced the same executable
5. **Report** with:
   - Test program source
   - Comparison output
   - Trace parameters used
   - DynamoRIO and PIN versions

## Continuous Integration

For automated verification:

```yaml
# .github/workflows/verify.yml
name: Verify Tracer

on: [push, pull_request]

jobs:
  verify:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v2
      
      - name: Install DynamoRIO
        run: |
          wget https://github.com/DynamoRIO/dynamorio/releases/download/release_10.0.0/DynamoRIO-Linux-10.0.0.tar.gz
          tar xzf DynamoRIO-Linux-10.0.0.tar.gz
          echo "DYNAMORIO_HOME=$(pwd)/DynamoRIO-Linux-10.0.0" >> $GITHUB_ENV
      
      - name: Build and Verify
        run: ./verify_tracer.sh
```

## Summary

- **Quick test**: `./verify_tracer.sh` (no PIN needed)
- **Full comparison**: Generate both traces and run `compare_traces.py`
- **Register IDs differ**: Expected and acceptable
- **IP, branches, memory**: Must match exactly
- **Report issues**: Include traces and test program

The verification tools ensure the DynamoRIO tracer maintains compatibility with PIN while adding multi-architecture support.

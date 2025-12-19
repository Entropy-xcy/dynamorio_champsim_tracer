# Known Limitations

This document describes the known limitations of the DynamoRIO ChampSim tracer compared to the original PIN implementation.

## 1. Branch Taken Detection ✅ FIXED

### Previous Issue
All conditional branches were marked as "taken" in the trace, regardless of actual execution.

### Current Implementation
**Proper branch taken detection is now implemented!**

The tracer uses a two-phase approach:
1. **Setup Phase**: When a conditional branch (CBR) is encountered, the tracer saves the instruction state and the fall-through address
2. **Resolution Phase**: At the start of the next basic block, the tracer checks if execution arrived at the fall-through address:
   - If yes → branch was NOT taken (branch_taken = 0)
   - If no → branch WAS taken (branch_taken = 1)

This approach leverages DynamoRIO's basic block structure to accurately track branch outcomes.

### Impact
- ✅ **Branch prediction studies**: Now accurate
- ✅ **IPC simulation**: Accurate
- ✅ **Cache simulation**: Not affected (was already accurate)
- ✅ **Prefetcher studies**: Not affected (was already accurate)

### Implementation Details
```cpp
// Thread-local storage tracks pending branches
typedef struct {
    trace_instr_format_t curr_instr;
    trace_instr_format_t pending_cbr_instr;  // Saved CBR for resolution
    app_pc last_cbr_fallthrough;              // Fall-through address
    bool has_pending_cbr;                     // Pending flag
} per_thread_t;

// At each BB start, resolve any pending branch
resolve_pending_branch(current_bb_pc);

// For CBR instructions, save state for later resolution
at_conditional_branch_setup(cbr_pc, fallthrough_pc);
```

## 2. Register ID Encoding

### Issue
Register IDs in DynamoRIO traces differ from PIN traces.

### Impact
- Traces are not bit-identical to PIN traces
- However, they are **semantically equivalent**
- ChampSim can use either format without modification

### Why This Happens
DynamoRIO and PIN use different internal register numbering schemes. Both correctly identify the architectural registers, just with different ID numbers.

### Workaround
None needed - this is not a functional limitation.

## 3. Architecture-Specific Differences

### x86/x86-64
- ✅ Fully supported
- ✅ Accurate branch taken detection

### ARM32/AArch32
- ✅ Supported  
- ✅ Accurate branch taken detection
- ⚠️ Less tested than x86

### ARM64/AArch64
- ✅ Supported
- ✅ Accurate branch taken detection
- ⚠️ Less tested than x86

### RISC-V
- ⚠️ Experimental (depends on DynamoRIO RISC-V support)
- ✅ Accurate branch taken detection (when DynamoRIO works)
- Not extensively tested

## 4. Performance Overhead

### Issue
Instrumentation overhead is typically 2-20x slowdown, depending on the program.

### Impact
- Trace generation can take considerable time for large traces
- Not suitable for real-time tracing

### Comparison
- DynamoRIO: 2-20x typical overhead
- PIN: 5-50x typical overhead
- **DynamoRIO is generally faster**

### Workaround
- Use `-s` (skip) to avoid tracing initialization code
- Trace only representative program regions
- Use faster storage (SSD) for trace output

## 5. Multi-threaded Programs

### Issue
The current implementation traces all threads but serializes trace output.

### Impact
- All threads are traced correctly
- Thread interleaving in the trace may differ from actual execution
- Per-thread instruction counts may not be precise

### Status
- ✅ Thread-safe (uses mutexes)
- ⚠️ May have some ordering artifacts in multi-threaded workloads

### Future Enhancement
Consider adding thread ID to trace format for multi-core simulation.

## 6. System Calls and Libraries

### Issue
System calls and library code are traced if executed.

### Impact
- Traces may include library initialization code
- System calls add instructions to the trace

### Workaround
- Use `-s` (skip) option to skip initialization
- Or filter traces post-processing

## 7. Self-Modifying Code

### Issue
Self-modifying code may not be traced correctly.

### Impact
- Most programs don't use self-modifying code
- JITs and some uncommon programs may have issues

### Status
- Not extensively tested
- DynamoRIO has some support, but not guaranteed

## 8. Signal Handlers

### Issue
Signal handlers are traced as part of the execution.

### Impact
- Asynchronous signals may appear in traces
- May affect reproducibility

### Status
- Works as designed
- For deterministic traces, disable signals if possible

## Comparison Summary

| Feature | PIN Tracer | DynamoRIO Tracer | Impact |
|---------|-----------|------------------|--------|
| Branch taken accuracy | ✅ Accurate | ✅ Accurate | ✅ None |
| Register tracking | ✅ Complete | ✅ Complete | ✅ None |
| Memory tracking | ✅ Complete | ✅ Complete | ✅ None |
| Multi-arch support | ❌ x86 only | ✅ x86/ARM/RISC-V | ✅ Major advantage |
| Performance | ⚠️ Slower | ✅ Faster | ⚠️ Medium |
| Thread safety | ⚠️ Limited | ✅ Full | ✅ None |

## Severity Levels

- ✅ **No Impact**: Works as expected
- ⚠️ **Medium Impact**: Works with limitations
- ❌ **High Impact**: Significant limitation

## Reporting Issues

If you encounter a limitation not listed here, please:
1. Check if it's a DynamoRIO limitation (not specific to this tracer)
2. Open an issue with details about your use case
3. Consider contributing a fix (see CONTRIBUTING.md)

## Roadmap

### High Priority Fixes
1. ~~**Branch taken detection**: Implement proper tracking~~ ✅ COMPLETED
2. **Better testing**: More coverage on ARM/RISC-V

### Medium Priority Enhancements
1. Thread ID in trace format
2. Optional SIMD register tracking
3. Instruction categorization

### Low Priority
1. Online compression
2. Sampling mode
3. Custom trace formats

## Contributing

We welcome contributions to address these limitations! See CONTRIBUTING.md for details.

The most impactful contribution would be implementing proper branch taken detection.

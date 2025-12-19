#!/usr/bin/env python3
"""
Unit tests for trace format and comparison utilities.
"""

import struct
import tempfile
import os
import sys

def create_test_trace(filename, num_instrs=10):
    """Create a test trace file with known content."""
    with open(filename, 'wb') as f:
        for i in range(num_instrs):
            # Create a test instruction
            ip = 0x400000 + (i * 4)  # Sequential IPs
            is_branch = 1 if i % 3 == 0 else 0  # Every 3rd is a branch
            branch_taken = 1 if i % 2 == 0 else 0  # Alternating taken/not-taken
            
            # Pack the instruction (64 bytes total)
            data = struct.pack('<Q', ip)  # IP: 8 bytes
            data += bytes([is_branch, branch_taken])  # Branch info: 2 bytes
            data += bytes([1, 2])  # Dest regs: 2 bytes
            data += bytes([3, 4, 5, 0])  # Src regs: 4 bytes
            data += struct.pack('<QQ', 0x7fff0000 + i, 0)  # Dest mem: 16 bytes
            data += struct.pack('<QQQQ', 0x7fff1000 + i, 0, 0, 0)  # Src mem: 32 bytes
            
            assert len(data) == 64, f"Expected 64 bytes, got {len(data)}"
            f.write(data)

def test_trace_format():
    """Test that we can create and read trace files correctly."""
    print("Test: Trace format creation and reading")
    
    with tempfile.NamedTemporaryFile(delete=False, suffix='.trace') as tf:
        filename = tf.name
    
    try:
        # Create test trace
        create_test_trace(filename, 5)
        
        # Verify file size
        size = os.path.getsize(filename)
        expected_size = 5 * 64
        assert size == expected_size, f"Expected {expected_size} bytes, got {size}"
        
        # Read back and verify
        with open(filename, 'rb') as f:
            for i in range(5):
                data = f.read(64)
                assert len(data) == 64, f"Instruction {i}: expected 64 bytes"
                
                # Verify IP
                ip = struct.unpack('<Q', data[0:8])[0]
                expected_ip = 0x400000 + (i * 4)
                assert ip == expected_ip, f"Instruction {i}: IP mismatch"
                
                # Verify branch info
                is_branch = data[8]
                expected_branch = 1 if i % 3 == 0 else 0
                assert is_branch == expected_branch, f"Instruction {i}: branch flag mismatch"
        
        print("  ✓ PASS")
        return True
        
    finally:
        if os.path.exists(filename):
            os.unlink(filename)

def test_comparison_logic():
    """Test the comparison logic."""
    print("Test: Trace comparison logic")
    
    with tempfile.NamedTemporaryFile(delete=False, suffix='_1.trace') as tf1:
        file1 = tf1.name
    with tempfile.NamedTemporaryFile(delete=False, suffix='_2.trace') as tf2:
        file2 = tf2.name
    
    try:
        # Create identical traces
        create_test_trace(file1, 5)
        create_test_trace(file2, 5)
        
        # Import comparison function
        sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
        from compare_traces import compare_traces
        
        # Compare - should match
        result = compare_traces(file1, file2, max_diff=10, verbose=False)
        assert result == True, "Identical traces should match"
        
        print("  ✓ PASS")
        return True
        
    finally:
        for f in [file1, file2]:
            if os.path.exists(f):
                os.unlink(f)

def test_format_constants():
    """Verify trace format constants."""
    print("Test: Format constants")
    
    # From trace_instruction.h
    EXPECTED_SIZE = 64
    
    # Calculate size from structure
    calculated_size = (
        8 +   # unsigned long long ip
        1 +   # unsigned char is_branch
        1 +   # unsigned char branch_taken
        2 +   # unsigned char destination_registers[2]
        4 +   # unsigned char source_registers[4]
        16 +  # unsigned long long destination_memory[2]
        32    # unsigned long long source_memory[4]
    )
    
    assert calculated_size == EXPECTED_SIZE, \
        f"Calculated size {calculated_size} != expected {EXPECTED_SIZE}"
    
    print(f"  Trace instruction size: {EXPECTED_SIZE} bytes")
    print("  ✓ PASS")
    return True

def main():
    """Run all unit tests."""
    print("=== Trace Format Unit Tests ===\n")
    
    tests = [
        test_format_constants,
        test_trace_format,
        test_comparison_logic,
    ]
    
    passed = 0
    failed = 0
    
    for test in tests:
        try:
            if test():
                passed += 1
            else:
                failed += 1
                print(f"  ✗ FAIL")
        except Exception as e:
            failed += 1
            print(f"  ✗ FAIL: {e}")
    
    print(f"\n=== Results ===")
    print(f"Passed: {passed}/{len(tests)}")
    print(f"Failed: {failed}/{len(tests)}")
    
    return 0 if failed == 0 else 1

if __name__ == '__main__':
    sys.exit(main())

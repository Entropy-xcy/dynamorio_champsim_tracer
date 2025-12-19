/*
 *    Copyright 2023 The ChampSim Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * ChampSim Tracer for DynamoRIO
 * This tool generates ChampSim-compatible traces using DynamoRIO
 */

#include "dr_api.h"
#include "drmgr.h"
#include "drutil.h"
#include "droption.h"
#include <cstring>
#include <string>
#include "trace_instruction.h"

using trace_instr_format_t = input_instr;

/* ================================================================== */
// Global variables
/* ================================================================== */

static file_t outfile;
static void *trace_buffer_mutex;
static void *instr_count_mutex;
static uint64 instrCount = 0;

// Thread-local storage for current instruction
typedef struct {
    trace_instr_format_t curr_instr;
} per_thread_t;

static int tls_idx;

/* ===================================================================== */
// Command line options
/* ===================================================================== */
static droption_t<std::string> op_output_file
(DROPTION_SCOPE_CLIENT, "o", "champsim.trace",
 "Output file name for ChampSim tracer",
 "Specify file name for ChampSim tracer output");

static droption_t<uint64> op_skip_instructions
(DROPTION_SCOPE_CLIENT, "s", 0,
 "Skip instructions",
 "How many instructions to skip before tracing begins");

static droption_t<uint64> op_trace_instructions
(DROPTION_SCOPE_CLIENT, "t", 1000000,
 "Trace instructions",
 "How many instructions to trace");

/* ===================================================================== */
// Utility functions
/* ===================================================================== */

static void
reset_current_instruction(per_thread_t *data, app_pc pc)
{
    memset(&data->curr_instr, 0, sizeof(trace_instr_format_t));
    data->curr_instr.ip = (unsigned long long)pc;
}

static bool
should_write()
{
    // Thread-safe increment of instruction counter
    dr_mutex_lock(instr_count_mutex);
    uint64 current_count = ++instrCount;
    dr_mutex_unlock(instr_count_mutex);
    
    return (current_count > op_skip_instructions.get_value()) &&
           (current_count <= (op_trace_instructions.get_value() + op_skip_instructions.get_value()));
}

static void
write_current_instruction(per_thread_t *data)
{
    dr_mutex_lock(trace_buffer_mutex);
    dr_write_file(outfile, &data->curr_instr, sizeof(trace_instr_format_t));
    dr_mutex_unlock(trace_buffer_mutex);
}

template <typename T>
static void
write_to_set(T* begin, T* end, T value)
{
    if (value == 0) return; // Skip null values
    
    T* set_end = begin;
    while (set_end < end && *set_end != 0) {
        set_end++;
    }
    
    // Check if value already exists
    T* found = begin;
    while (found < set_end && *found != value) {
        found++;
    }
    
    // If not found and there's space, add it
    if (found == set_end && set_end < end) {
        *set_end = value;
    }
}

/* ===================================================================== */
// Instrumentation callbacks
/* ===================================================================== */

static void
at_branch(bool taken)
{
    void *drcontext = dr_get_current_drcontext();
    per_thread_t *data = (per_thread_t *)drmgr_get_tls_field(drcontext, tls_idx);
    
    data->curr_instr.is_branch = 1;
    data->curr_instr.branch_taken = taken ? 1 : 0;
}

static void
at_memory_read(app_pc addr)
{
    void *drcontext = dr_get_current_drcontext();
    per_thread_t *data = (per_thread_t *)drmgr_get_tls_field(drcontext, tls_idx);
    
    write_to_set(data->curr_instr.source_memory,
                 data->curr_instr.source_memory + NUM_INSTR_SOURCES,
                 (unsigned long long)addr);
}

static void
at_memory_write(app_pc addr)
{
    void *drcontext = dr_get_current_drcontext();
    per_thread_t *data = (per_thread_t *)drmgr_get_tls_field(drcontext, tls_idx);
    
    write_to_set(data->curr_instr.destination_memory,
                 data->curr_instr.destination_memory + NUM_INSTR_DESTINATIONS,
                 (unsigned long long)addr);
}

static void
at_instruction_start(app_pc pc)
{
    void *drcontext = dr_get_current_drcontext();
    per_thread_t *data = (per_thread_t *)drmgr_get_tls_field(drcontext, tls_idx);
    
    reset_current_instruction(data, pc);
}

static void
at_instruction_end()
{
    if (should_write()) {
        void *drcontext = dr_get_current_drcontext();
        per_thread_t *data = (per_thread_t *)drmgr_get_tls_field(drcontext, tls_idx);
        write_current_instruction(data);
    }
}

static void
at_register_read(reg_id_t reg)
{
    void *drcontext = dr_get_current_drcontext();
    per_thread_t *data = (per_thread_t *)drmgr_get_tls_field(drcontext, tls_idx);
    
    write_to_set(data->curr_instr.source_registers,
                 data->curr_instr.source_registers + NUM_INSTR_SOURCES,
                 (unsigned char)reg);
}

static void
at_register_write(reg_id_t reg)
{
    void *drcontext = dr_get_current_drcontext();
    per_thread_t *data = (per_thread_t *)drmgr_get_tls_field(drcontext, tls_idx);
    
    write_to_set(data->curr_instr.destination_registers,
                 data->curr_instr.destination_registers + NUM_INSTR_DESTINATIONS,
                 (unsigned char)reg);
}

/* ===================================================================== */
// Instruction instrumentation
/* ===================================================================== */

static dr_emit_flags_t
event_instruction(void *drcontext, void *tag, instrlist_t *bb, instr_t *instr,
                  bool for_trace, bool translating, void *user_data)
{
    // Skip non-app instructions
    if (!instr_is_app(instr))
        return DR_EMIT_DEFAULT;

    app_pc pc = instr_get_app_pc(instr);
    
    // Insert call to reset instruction at the beginning
    dr_insert_clean_call(drcontext, bb, instr, (void *)at_instruction_start,
                        false, 1, OPND_CREATE_INTPTR(pc));

    // Handle branch instructions
    // NOTE: Current implementation marks all branches as taken
    // This is a simplification - a more sophisticated version would track
    // actual branch outcomes by checking if the next basic block in the
    // trace matches the fall-through or branch target address
    if (instr_is_cbr(instr)) {
        // Conditional branch
        dr_insert_clean_call(drcontext, bb, instr, (void *)at_branch,
                           false, 1, OPND_CREATE_INT32(1));
    } else if (instr_is_ubr(instr) || instr_is_mbr(instr)) {
        // Unconditional branches are always taken
        dr_insert_clean_call(drcontext, bb, instr, (void *)at_branch,
                           false, 1, OPND_CREATE_INT32(1));
    }

    // Handle register reads and writes
    for (int i = 0; i < instr_num_srcs(instr); i++) {
        opnd_t src = instr_get_src(instr, i);
        if (opnd_is_reg(src)) {
            reg_id_t reg = opnd_get_reg(src);
            if (reg != DR_REG_NULL && reg_is_gpr(reg)) {
                dr_insert_clean_call(drcontext, bb, instr, (void *)at_register_read,
                                   false, 1, OPND_CREATE_INT32(reg));
            }
        }
    }

    for (int i = 0; i < instr_num_dsts(instr); i++) {
        opnd_t dst = instr_get_dst(instr, i);
        if (opnd_is_reg(dst)) {
            reg_id_t reg = opnd_get_reg(dst);
            if (reg != DR_REG_NULL && reg_is_gpr(reg)) {
                dr_insert_clean_call(drcontext, bb, instr, (void *)at_register_write,
                                   false, 1, OPND_CREATE_INT32(reg));
            }
        }
    }

    // Handle memory operations
    // For memory references, we need to compute the address at runtime
    // Use architecture-appropriate scratch registers
#ifdef X86
    reg_id_t scratch_reg_1 = DR_REG_XAX;
    reg_id_t scratch_reg_2 = DR_REG_XBX;
#elif defined(AARCH64)
    reg_id_t scratch_reg_1 = DR_REG_X0;
    reg_id_t scratch_reg_2 = DR_REG_X1;
#elif defined(ARM)
    reg_id_t scratch_reg_1 = DR_REG_R0;
    reg_id_t scratch_reg_2 = DR_REG_R1;
#elif defined(RISCV64)
    reg_id_t scratch_reg_1 = DR_REG_A0;
    reg_id_t scratch_reg_2 = DR_REG_A1;
#else
    // Fallback for other architectures
    reg_id_t scratch_reg_1 = DR_REG_NULL;
    reg_id_t scratch_reg_2 = DR_REG_NULL;
#endif

    if (instr_reads_memory(instr)) {
        for (int i = 0; i < instr_num_srcs(instr); i++) {
            if (opnd_is_memory_reference(instr_get_src(instr, i))) {
                // Insert code to get memory address and call our handler
                if (scratch_reg_1 != DR_REG_NULL) {
                    bool res = drutil_insert_get_mem_addr(drcontext, bb, instr, 
                                                         instr_get_src(instr, i), 
                                                         scratch_reg_1, DR_REG_NULL);
                    if (res) {
                        // scratch_reg_1 now contains the computed address, pass it as a value
                        dr_insert_clean_call(drcontext, bb, instr, (void *)at_memory_read,
                                           false, 1, opnd_create_reg(scratch_reg_1));
                    }
                }
            }
        }
    }

    if (instr_writes_memory(instr)) {
        for (int i = 0; i < instr_num_dsts(instr); i++) {
            if (opnd_is_memory_reference(instr_get_dst(instr, i))) {
                // Insert code to get memory address and call our handler
                if (scratch_reg_2 != DR_REG_NULL) {
                    bool res = drutil_insert_get_mem_addr(drcontext, bb, instr,
                                                         instr_get_dst(instr, i),
                                                         scratch_reg_2, DR_REG_NULL);
                    if (res) {
                        // scratch_reg_2 now contains the computed address, pass it as a value
                        dr_insert_clean_call(drcontext, bb, instr, (void *)at_memory_write,
                                           false, 1, opnd_create_reg(scratch_reg_2));
                    }
                }
            }
        }
    }

    // Insert call at the end of instruction to write if needed
    dr_insert_clean_call(drcontext, bb, instr, (void *)at_instruction_end,
                        false, 0);

    return DR_EMIT_DEFAULT;
}

/* ===================================================================== */
// Thread events
/* ===================================================================== */

static void
event_thread_init(void *drcontext)
{
    per_thread_t *data = (per_thread_t *)dr_thread_alloc(drcontext, sizeof(per_thread_t));
    memset(data, 0, sizeof(per_thread_t));
    drmgr_set_tls_field(drcontext, tls_idx, data);
}

static void
event_thread_exit(void *drcontext)
{
    per_thread_t *data = (per_thread_t *)drmgr_get_tls_field(drcontext, tls_idx);
    dr_thread_free(drcontext, data, sizeof(per_thread_t));
}

/* ===================================================================== */
// Initialization and cleanup
/* ===================================================================== */

static void
event_exit(void)
{
    dr_close_file(outfile);
    dr_mutex_destroy(trace_buffer_mutex);
    dr_mutex_destroy(instr_count_mutex);
    
    if (!drmgr_unregister_tls_field(tls_idx) ||
        !drmgr_unregister_thread_init_event(event_thread_init) ||
        !drmgr_unregister_thread_exit_event(event_thread_exit) ||
        !drmgr_unregister_bb_insertion_event(event_instruction))
        DR_ASSERT(false);

    drutil_exit();
    drmgr_exit();
}

DR_EXPORT void
dr_client_main(client_id_t id, int argc, const char *argv[])
{
    dr_set_client_name("DynamoRIO ChampSim Tracer",
                      "https://github.com/ChampSim/ChampSim");

    // Parse command line options
    if (!droption_parser_t::parse_argv(DROPTION_SCOPE_CLIENT, argc, argv, NULL, NULL))
        DR_ASSERT(false);

    // Initialize DynamoRIO extensions
    if (!drmgr_init() || !drutil_init())
        DR_ASSERT(false);

    // Open output file
    outfile = dr_open_file(op_output_file.get_value().c_str(),
                          DR_FILE_WRITE_OVERWRITE | DR_FILE_ALLOW_LARGE);
    DR_ASSERT(outfile != INVALID_FILE);

    // Initialize mutexes for thread-safe operations
    trace_buffer_mutex = dr_mutex_create();
    instr_count_mutex = dr_mutex_create();

    // Register thread events
    tls_idx = drmgr_register_tls_field();
    DR_ASSERT(tls_idx != -1);
    
    if (!drmgr_register_thread_init_event(event_thread_init) ||
        !drmgr_register_thread_exit_event(event_thread_exit))
        DR_ASSERT(false);

    // Register instruction event
    if (!drmgr_register_bb_insertion_event(event_instruction))
        DR_ASSERT(false);

    // Register exit event
    dr_register_exit_event(event_exit);

    dr_log(NULL, DR_LOG_ALL, 1, "ChampSim Tracer initialized\n");
    dr_log(NULL, DR_LOG_ALL, 1, "Output file: %s\n", op_output_file.get_value().c_str());
    dr_log(NULL, DR_LOG_ALL, 1, "Skip instructions: %llu\n", (unsigned long long)op_skip_instructions.get_value());
    dr_log(NULL, DR_LOG_ALL, 1, "Trace instructions: %llu\n", (unsigned long long)op_trace_instructions.get_value());
}

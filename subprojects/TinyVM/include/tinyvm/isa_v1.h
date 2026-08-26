#pragma once

#include <tinyvm/artifact_v2.h>

typedef enum {
    TV1_NOP = 0,
    TV1_CONST = 1,
    TV1_MOVE = 2,
    TV1_CONVERT = 3,
    TV1_ADD = 4,
    TV1_SUB = 5,
    TV1_MUL = 6,
    TV1_SDIV = 7,
    TV1_CMP_EQ = 8,
    TV1_CMP_NE = 9,
    TV1_CMP_LT = 10,
    TV1_CMP_LE = 11,
    TV1_CMP_GT = 12,
    TV1_CMP_GE = 13,
    TV1_JMP = 14,
    TV1_BRANCH = 15,
    TV1_RETURN = 16,
    TV1_TRAP = 17,
    TV1_HALT = 18,
    TV1_STRING_HANDLE = 19,
    TV1_STORAGE_HANDLE = 20,
    TV1_CALL_IMPORT = 21,
    TV1_OPCODE_COUNT
} TinyvmIsaV1Opcode;

typedef enum {
    TV1_TRAP_EXPLICIT = 1,
    TV1_TRAP_UNINITIALIZED_SLOT = 2,
    TV1_TRAP_TYPE_MISMATCH = 3,
    TV1_TRAP_DIVISION_BY_ZERO = 4,
    TV1_TRAP_DIVISION_OVERFLOW = 5,
    TV1_TRAP_ARITHMETIC_OVERFLOW = 6,
    TV1_TRAP_UNRESOLVED_IMPORT = 7,
    TV1_TRAP_STEP_LIMIT = 8
} TinyvmIsaV1Trap;

typedef struct {
    uint32_t carrier;
    uint64_t bits;
    bool initialized;
} TinyvmValue;

typedef struct {
    TinyvmValue *slots;
    size_t slot_count;
    uint64_t pc;
    uint64_t steps;
    uint64_t step_limit;
    bool running;
    bool returned;
    TinyvmValue result;
    uint32_t trap;
    uint64_t trap_instruction;
    const char *fault;
} TinyvmIsaV1Context;

bool tinyvm_isa_v1_validate(const TinyvmArtifactV2 *artifact,
                            char *diagnostic, size_t capacity);
bool tinyvm_isa_v1_context_init(TinyvmIsaV1Context *context,
                                size_t slot_count, uint64_t step_limit);
void tinyvm_isa_v1_context_destroy(TinyvmIsaV1Context *context);
bool tinyvm_isa_v1_run_switch(const TinyvmArtifactV2 *artifact,
                              TinyvmIsaV1Context *context);
bool tinyvm_isa_v1_run_computed(const TinyvmArtifactV2 *artifact,
                                TinyvmIsaV1Context *context);

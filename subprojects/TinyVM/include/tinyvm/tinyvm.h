#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    OP_NOP,
    OP_ADD,
    OP_SUB,
    OP_MUL,
    OP_DIV,
    OP_MOD,
    OP_AND,
    OP_OR,
    OP_XOR,
    OP_NOT,
    OP_SHL,
    OP_SHR,
    OP_CMP,
    OP_JMP,
    OP_JZ,
    OP_JNZ,
    OP_LOAD,
    OP_STORE,
    OP_MOV,
    OP_PUSH,
    OP_POP,
    OP_CTX_PUSH,
    OP_CTX_COMMIT,
    OP_CTX_ABORT,
    OP_CTX_HASCHK,
    OP_CTX_COPY,
    OP_HALT,
    OP_COUNT
} VmOpcode;

typedef enum {
    FLAG_Z = 1 << 0,
    FLAG_N = 1 << 1,
    FLAG_C = 1 << 2
} VmFlags;

typedef struct {
    int64_t opcode;
    int64_t a;
    int64_t b;
    int64_t pad;
} InstrWord;

typedef struct {
    int64_t regs[8];
    int64_t data[256];
    int64_t stack[256];
    size_t pc;
    size_t sp;
    uint64_t flags;
    bool running;
    const char *fault;
} vm_context;

void vm_init(vm_context *ctx);
bool vm_run(vm_context *ctx, const InstrWord *program, size_t length);

void op_add(vm_context *, const InstrWord *);
void op_sub(vm_context *, const InstrWord *);
void op_mul(vm_context *, const InstrWord *);
void op_div(vm_context *, const InstrWord *);
void op_mod(vm_context *, const InstrWord *);
void op_and(vm_context *, const InstrWord *);
void op_or(vm_context *, const InstrWord *);
void op_xor(vm_context *, const InstrWord *);
void op_not(vm_context *, const InstrWord *);
void op_shl(vm_context *, const InstrWord *);
void op_shr(vm_context *, const InstrWord *);
void op_cmp(vm_context *, const InstrWord *);
void op_jmp(vm_context *, const InstrWord *);
void op_jz(vm_context *, const InstrWord *);
void op_jnz(vm_context *, const InstrWord *);
void op_load(vm_context *, const InstrWord *);
void op_store(vm_context *, const InstrWord *);
void op_mov(vm_context *, const InstrWord *);
void op_push(vm_context *, const InstrWord *);
void op_pop(vm_context *, const InstrWord *);
void op_ctx_push(vm_context *, const InstrWord *);
void op_ctx_commit(vm_context *, const InstrWord *);
void op_ctx_abort(vm_context *, const InstrWord *);
void op_ctx_haschk(vm_context *, const InstrWord *);
void op_ctx_copy(vm_context *, const InstrWord *);

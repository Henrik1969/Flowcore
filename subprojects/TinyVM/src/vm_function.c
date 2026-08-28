#include <tinyvm/tinyvm.h>

typedef void (*handler_fn)(vm_context *, const InstrWord *);

static void op_nop_handler(vm_context *ctx, const InstrWord *instruction) {
    (void)ctx;
    (void)instruction;
}

static void op_halt_handler(vm_context *ctx, const InstrWord *instruction) {
    (void)instruction;
    ctx->running = false;
}

bool vm_run_function(vm_context *ctx, const InstrWord *program, size_t length) {
    static const handler_fn handlers[OP_COUNT] = {
        [OP_NOP] = op_nop_handler, [OP_ADD] = op_add, [OP_SUB] = op_sub,
        [OP_MUL] = op_mul, [OP_DIV] = op_div, [OP_MOD] = op_mod,
        [OP_AND] = op_and, [OP_OR] = op_or, [OP_XOR] = op_xor,
        [OP_NOT] = op_not, [OP_SHL] = op_shl, [OP_SHR] = op_shr,
        [OP_CMP] = op_cmp, [OP_JMP] = op_jmp, [OP_JZ] = op_jz,
        [OP_JNZ] = op_jnz, [OP_LOAD] = op_load, [OP_STORE] = op_store,
        [OP_MOV] = op_mov, [OP_PUSH] = op_push, [OP_POP] = op_pop,
        [OP_CTX_PUSH] = op_ctx_push, [OP_CTX_COMMIT] = op_ctx_commit,
        [OP_CTX_ABORT] = op_ctx_abort, [OP_CTX_HASCHK] = op_ctx_haschk,
        [OP_CTX_COPY] = op_ctx_copy, [OP_HALT] = op_halt_handler
    };

    if (ctx == NULL || program == NULL) return false;

    while (ctx->running && ctx->pc < length) {
        const InstrWord *instruction = &program[ctx->pc++];
        if (instruction->opcode < 0 || instruction->opcode >= OP_COUNT) {
            ctx->fault = "unknown opcode";
            ctx->running = false;
            break;
        }
        handlers[instruction->opcode](ctx, instruction);
    }
    if (ctx->running && ctx->pc >= length) {
        ctx->fault = "program ended without HALT";
        ctx->running = false;
    }
    return ctx->fault == NULL;
}

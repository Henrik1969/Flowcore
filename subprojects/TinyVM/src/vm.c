#include <tinyvm/tinyvm.h>

#include <string.h>

void vm_init(vm_context *ctx) {
    memset(ctx, 0, sizeof(*ctx));
    ctx->running = true;
}

bool vm_run(vm_context *ctx, const InstrWord *program, size_t length) {
    static void *dispatch[OP_COUNT] = {
        [OP_NOP] = &&do_nop, [OP_ADD] = &&do_add, [OP_SUB] = &&do_sub,
        [OP_MUL] = &&do_mul, [OP_DIV] = &&do_div, [OP_MOD] = &&do_mod,
        [OP_AND] = &&do_and, [OP_OR] = &&do_or, [OP_XOR] = &&do_xor,
        [OP_NOT] = &&do_not, [OP_SHL] = &&do_shl, [OP_SHR] = &&do_shr,
        [OP_CMP] = &&do_cmp, [OP_JMP] = &&do_jmp, [OP_JZ] = &&do_jz,
        [OP_JNZ] = &&do_jnz, [OP_LOAD] = &&do_load,
        [OP_STORE] = &&do_store, [OP_MOV] = &&do_mov,
        [OP_PUSH] = &&do_push, [OP_POP] = &&do_pop,
        [OP_CTX_PUSH] = &&do_ctx_push, [OP_CTX_COMMIT] = &&do_ctx_commit,
        [OP_CTX_ABORT] = &&do_ctx_abort, [OP_CTX_HASCHK] = &&do_ctx_haschk,
        [OP_CTX_COPY] = &&do_ctx_copy, [OP_HALT] = &&do_halt
    };

    if (ctx == NULL || program == NULL) return false;

    while (ctx->running && ctx->pc < length) {
        const InstrWord *instruction = &program[ctx->pc++];
        if (instruction->opcode < 0 || instruction->opcode >= OP_COUNT) {
            ctx->fault = "unknown opcode";
            ctx->running = false;
            break;
        }
        goto *dispatch[instruction->opcode];
do_nop: continue;
do_add: op_add(ctx, instruction); continue;
do_sub: op_sub(ctx, instruction); continue;
do_mul: op_mul(ctx, instruction); continue;
do_div: op_div(ctx, instruction); continue;
do_mod: op_mod(ctx, instruction); continue;
do_and: op_and(ctx, instruction); continue;
do_or: op_or(ctx, instruction); continue;
do_xor: op_xor(ctx, instruction); continue;
do_not: op_not(ctx, instruction); continue;
do_shl: op_shl(ctx, instruction); continue;
do_shr: op_shr(ctx, instruction); continue;
do_cmp: op_cmp(ctx, instruction); continue;
do_jmp: op_jmp(ctx, instruction); continue;
do_jz: op_jz(ctx, instruction); continue;
do_jnz: op_jnz(ctx, instruction); continue;
do_load: op_load(ctx, instruction); continue;
do_store: op_store(ctx, instruction); continue;
do_mov: op_mov(ctx, instruction); continue;
do_push: op_push(ctx, instruction); continue;
do_pop: op_pop(ctx, instruction); continue;
do_ctx_push: op_ctx_push(ctx, instruction); continue;
do_ctx_commit: op_ctx_commit(ctx, instruction); continue;
do_ctx_abort: op_ctx_abort(ctx, instruction); continue;
do_ctx_haschk: op_ctx_haschk(ctx, instruction); continue;
do_ctx_copy: op_ctx_copy(ctx, instruction); continue;
do_halt: ctx->running = false; break;
    }

    if (ctx->running && ctx->pc >= length) {
        ctx->fault = "program ended without HALT";
        ctx->running = false;
    }
    return ctx->fault == NULL;
}

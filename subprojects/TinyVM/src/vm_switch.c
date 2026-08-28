#include <tinyvm/tinyvm.h>

bool vm_run_switch(vm_context *ctx, const InstrWord *program, size_t length) {
    if (ctx == NULL || program == NULL) return false;

    while (ctx->running && ctx->pc < length) {
        const InstrWord *instruction = &program[ctx->pc++];
        switch (instruction->opcode) {
        case OP_NOP: break;
        case OP_ADD: op_add(ctx, instruction); break;
        case OP_SUB: op_sub(ctx, instruction); break;
        case OP_MUL: op_mul(ctx, instruction); break;
        case OP_DIV: op_div(ctx, instruction); break;
        case OP_MOD: op_mod(ctx, instruction); break;
        case OP_AND: op_and(ctx, instruction); break;
        case OP_OR: op_or(ctx, instruction); break;
        case OP_XOR: op_xor(ctx, instruction); break;
        case OP_NOT: op_not(ctx, instruction); break;
        case OP_SHL: op_shl(ctx, instruction); break;
        case OP_SHR: op_shr(ctx, instruction); break;
        case OP_CMP: op_cmp(ctx, instruction); break;
        case OP_JMP: op_jmp(ctx, instruction); break;
        case OP_JZ: op_jz(ctx, instruction); break;
        case OP_JNZ: op_jnz(ctx, instruction); break;
        case OP_LOAD: op_load(ctx, instruction); break;
        case OP_STORE: op_store(ctx, instruction); break;
        case OP_MOV: op_mov(ctx, instruction); break;
        case OP_PUSH: op_push(ctx, instruction); break;
        case OP_POP: op_pop(ctx, instruction); break;
        case OP_CTX_PUSH: op_ctx_push(ctx, instruction); break;
        case OP_CTX_COMMIT: op_ctx_commit(ctx, instruction); break;
        case OP_CTX_ABORT: op_ctx_abort(ctx, instruction); break;
        case OP_CTX_HASCHK: op_ctx_haschk(ctx, instruction); break;
        case OP_CTX_COPY: op_ctx_copy(ctx, instruction); break;
        case OP_HALT: ctx->running = false; break;
        default:
            ctx->fault = "unknown opcode";
            ctx->running = false;
            break;
        }
    }
    if (ctx->running && ctx->pc >= length) {
        ctx->fault = "program ended without HALT";
        ctx->running = false;
    }
    return ctx->fault == NULL;
}

#include <tinyvm/tinyvm.h>

#define R(index) (ctx->regs[(size_t)(index)])

static bool valid_register(int64_t index) {
    return index >= 0 && index < 8;
}

static bool require_registers(vm_context *ctx, const InstrWord *instruction,
                              bool needs_b) {
    if (!valid_register(instruction->a) ||
        (needs_b && !valid_register(instruction->b))) {
        ctx->fault = "register index out of range";
        ctx->running = false;
        return false;
    }
    return true;
}

#define BINARY_HANDLER(name, expression)                                      \
    void name(vm_context *ctx, const InstrWord *instruction) {                \
        if (require_registers(ctx, instruction, true)) {                      \
            expression;                                                       \
        }                                                                      \
    }

BINARY_HANDLER(op_add, R(instruction->a) += R(instruction->b))
BINARY_HANDLER(op_sub, R(instruction->a) -= R(instruction->b))
BINARY_HANDLER(op_mul, R(instruction->a) *= R(instruction->b))
BINARY_HANDLER(op_and, R(instruction->a) &= R(instruction->b))
BINARY_HANDLER(op_or, R(instruction->a) |= R(instruction->b))
BINARY_HANDLER(op_xor, R(instruction->a) ^= R(instruction->b))

void op_div(vm_context *ctx, const InstrWord *instruction) {
    if (!require_registers(ctx, instruction, true)) return;
    if (R(instruction->b) == 0) {
        ctx->fault = "division by zero";
        ctx->running = false;
        return;
    }
    R(instruction->a) /= R(instruction->b);
}

void op_mod(vm_context *ctx, const InstrWord *instruction) {
    if (!require_registers(ctx, instruction, true)) return;
    if (R(instruction->b) == 0) {
        ctx->fault = "modulo by zero";
        ctx->running = false;
        return;
    }
    R(instruction->a) %= R(instruction->b);
}

void op_not(vm_context *ctx, const InstrWord *instruction) {
    if (require_registers(ctx, instruction, false)) R(instruction->a) = ~R(instruction->a);
}

void op_shl(vm_context *ctx, const InstrWord *instruction) {
    if (require_registers(ctx, instruction, true))
        R(instruction->a) <<= ((uint64_t)R(instruction->b) & 63U);
}

void op_shr(vm_context *ctx, const InstrWord *instruction) {
    if (require_registers(ctx, instruction, true))
        R(instruction->a) >>= ((uint64_t)R(instruction->b) & 63U);
}

void op_cmp(vm_context *ctx, const InstrWord *instruction) {
    if (!require_registers(ctx, instruction, true)) return;
    ctx->flags = 0;
    if (R(instruction->a) == R(instruction->b)) ctx->flags |= FLAG_Z;
    if (R(instruction->a) < R(instruction->b)) ctx->flags |= FLAG_N;
}

static void relative_jump(vm_context *ctx, int64_t displacement) {
    const int64_t target = (int64_t)(ctx->pc - 1) + displacement;
    if (target < 0) {
        ctx->fault = "jump target out of range";
        ctx->running = false;
    } else {
        ctx->pc = (size_t)target;
    }
}

void op_jmp(vm_context *ctx, const InstrWord *instruction) { relative_jump(ctx, instruction->a); }
void op_jz(vm_context *ctx, const InstrWord *instruction) {
    if ((ctx->flags & FLAG_Z) != 0) relative_jump(ctx, instruction->a);
}
void op_jnz(vm_context *ctx, const InstrWord *instruction) {
    if ((ctx->flags & FLAG_Z) == 0) relative_jump(ctx, instruction->a);
}

void op_load(vm_context *ctx, const InstrWord *instruction) {
    if (!require_registers(ctx, instruction, true)) return;
    const int64_t address = R(instruction->b);
    if (address < 0 || address >= 256) {
        ctx->fault = "load address out of range";
        ctx->running = false;
        return;
    }
    R(instruction->a) = ctx->data[(size_t)address];
}

void op_store(vm_context *ctx, const InstrWord *instruction) {
    if (!require_registers(ctx, instruction, true)) return;
    const int64_t address = R(instruction->a);
    if (address < 0 || address >= 256) {
        ctx->fault = "store address out of range";
        ctx->running = false;
        return;
    }
    ctx->data[(size_t)address] = R(instruction->b);
}

void op_mov(vm_context *ctx, const InstrWord *instruction) {
    if (require_registers(ctx, instruction, true)) R(instruction->a) = R(instruction->b);
}

void op_push(vm_context *ctx, const InstrWord *instruction) {
    if (!require_registers(ctx, instruction, false)) return;
    if (ctx->sp >= 256) {
        ctx->fault = "stack overflow";
        ctx->running = false;
        return;
    }
    ctx->stack[ctx->sp++] = R(instruction->a);
}

void op_pop(vm_context *ctx, const InstrWord *instruction) {
    if (!require_registers(ctx, instruction, false)) return;
    if (ctx->sp == 0) {
        ctx->fault = "stack underflow";
        ctx->running = false;
        return;
    }
    R(instruction->a) = ctx->stack[--ctx->sp];
}

static void unsupported_context_operation(vm_context *ctx) {
    ctx->fault = "transaction context operation not reconstructed";
    ctx->running = false;
}

void op_ctx_push(vm_context *ctx, const InstrWord *instruction) { (void)instruction; unsupported_context_operation(ctx); }
void op_ctx_commit(vm_context *ctx, const InstrWord *instruction) { (void)instruction; unsupported_context_operation(ctx); }
void op_ctx_abort(vm_context *ctx, const InstrWord *instruction) { (void)instruction; unsupported_context_operation(ctx); }
void op_ctx_haschk(vm_context *ctx, const InstrWord *instruction) { (void)instruction; unsupported_context_operation(ctx); }
void op_ctx_copy(vm_context *ctx, const InstrWord *instruction) { (void)instruction; unsupported_context_operation(ctx); }

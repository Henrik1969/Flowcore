#include <tinyvm/tinyvm.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef bool (*runner_fn)(vm_context *, const InstrWord *, size_t);

typedef struct {
    const char *name;
    runner_fn run;
} Engine;

typedef void (*seed_fn)(vm_context *);
typedef bool (*verify_fn)(const vm_context *);

typedef struct {
    const char *name;
    const InstrWord *program;
    size_t length;
    seed_fn seed;
    const char *fault;
    verify_fn verify;
} Case;

static const Engine engines[] = {
    {"computed-goto", vm_run},
    {"switch", vm_run_switch},
    {"function-pointer", vm_run_function},
};

static void fail(const char *case_name, const char *engine, const char *reason) {
    fprintf(stderr, "FAIL case=%s engine=%s: %s\n", case_name, engine, reason);
    exit(1);
}

static bool same_fault(const char *left, const char *right) {
    if (left == NULL || right == NULL) return left == right;
    return strcmp(left, right) == 0;
}

static bool same_state(const vm_context *left, const vm_context *right) {
    return memcmp(left->regs, right->regs, sizeof(left->regs)) == 0 &&
           memcmp(left->data, right->data, sizeof(left->data)) == 0 &&
           memcmp(left->stack, right->stack, sizeof(left->stack)) == 0 &&
           left->pc == right->pc && left->sp == right->sp &&
           left->flags == right->flags && left->running == right->running &&
           same_fault(left->fault, right->fault);
}

static void seed_arithmetic(vm_context *ctx) {
    ctx->regs[0] = 20; ctx->regs[1] = 5; ctx->regs[2] = 6;
    ctx->regs[3] = 3; ctx->regs[4] = 12; ctx->regs[5] = 10;
    ctx->regs[6] = 1;
}

static bool verify_arithmetic(const vm_context *ctx) {
    return ctx->regs[0] == 2 && ctx->regs[3] == 2 && ctx->regs[4] == 14 &&
           ctx->regs[5] == 4 && ctx->regs[6] == -2 && ctx->pc == 11;
}

static void seed_comparison(vm_context *ctx) {
    ctx->regs[0] = 7; ctx->regs[1] = 7; ctx->regs[2] = 9;
}

static bool verify_equal(const vm_context *ctx) { return ctx->flags == FLAG_Z; }
static bool verify_less(const vm_context *ctx) { return ctx->flags == FLAG_N; }

static void seed_control(vm_context *ctx) {
    ctx->regs[0] = 4; ctx->regs[1] = 4; ctx->regs[2] = 1;
}

static bool verify_jump(const vm_context *ctx) { return ctx->regs[2] == 4; }

static void seed_memory(vm_context *ctx) {
    ctx->regs[0] = 7; ctx->regs[1] = 41; ctx->regs[2] = 7;
}

static bool verify_memory(const vm_context *ctx) {
    return ctx->data[7] == 41 && ctx->regs[3] == 41 && ctx->regs[4] == 41 &&
           ctx->sp == 0;
}

static void seed_zero_divisor(vm_context *ctx) { ctx->regs[0] = 10; }
static void seed_bad_load_negative(vm_context *ctx) { ctx->regs[1] = -1; }
static void seed_bad_load_high(vm_context *ctx) { ctx->regs[1] = 256; }
static void seed_bad_store(vm_context *ctx) { ctx->regs[0] = 256; }
static void seed_full_stack(vm_context *ctx) { ctx->sp = 256; }

static bool verify_fault_pc_one(const vm_context *ctx) { return ctx->pc == 1; }

#define WORDS(array) (sizeof(array) / sizeof((array)[0]))

static const InstrWord arithmetic[] = {
    {OP_ADD,0,1,0}, {OP_SUB,0,1,0}, {OP_MUL,0,1,0}, {OP_DIV,0,1,0},
    {OP_MOD,0,2,0}, {OP_AND,3,2,0}, {OP_OR,4,3,0}, {OP_XOR,5,4,0},
    {OP_NOT,6,0,0}, {OP_NOP,0,0,0}, {OP_HALT,0,0,0}
};
static const InstrWord shifts[] = {
    {OP_SHL,0,1,0}, {OP_SHR,0,2,0}, {OP_HALT,0,0,0}
};
static const InstrWord cmp_equal[] = {{OP_CMP,0,1,0},{OP_HALT,0,0,0}};
static const InstrWord cmp_less[] = {{OP_CMP,0,2,0},{OP_HALT,0,0,0}};
static const InstrWord jumps[] = {
    {OP_CMP,0,1,0}, {OP_JZ,2,0,0}, {OP_ADD,2,2,0}, {OP_JNZ,2,0,0},
    {OP_ADD,2,2,0}, {OP_JMP,2,0,0}, {OP_SUB,2,2,0}, {OP_ADD,2,2,0},
    {OP_HALT,0,0,0}
};
static const InstrWord memory_stack[] = {
    {OP_STORE,0,1,0}, {OP_LOAD,3,2,0}, {OP_MOV,4,3,0},
    {OP_PUSH,4,0,0}, {OP_POP,5,0,0}, {OP_HALT,0,0,0}
};
static const InstrWord context_push[] = {{OP_CTX_PUSH,0,0,0}};
static const InstrWord context_commit[] = {{OP_CTX_COMMIT,0,0,0}};
static const InstrWord context_abort[] = {{OP_CTX_ABORT,0,0,0}};
static const InstrWord context_haschk[] = {{OP_CTX_HASCHK,0,0,0}};
static const InstrWord context_copy[] = {{OP_CTX_COPY,0,0,0}};
static const InstrWord bad_opcode_negative[] = {{-1,0,0,0}};
static const InstrWord bad_opcode_high[] = {{OP_COUNT,0,0,0}};
static const InstrWord bad_register_a[] = {{OP_NOT,8,0,0}};
static const InstrWord bad_register_b[] = {{OP_ADD,0,-1,0}};
static const InstrWord divide_zero[] = {{OP_DIV,0,1,0}};
static const InstrWord modulo_zero[] = {{OP_MOD,0,1,0}};
static const InstrWord load_negative[] = {{OP_LOAD,0,1,0}};
static const InstrWord load_high[] = {{OP_LOAD,0,1,0}};
static const InstrWord store_high[] = {{OP_STORE,0,1,0}};
static const InstrWord stack_underflow[] = {{OP_POP,0,0,0}};
static const InstrWord stack_overflow[] = {{OP_PUSH,0,0,0}};
static const InstrWord jump_negative[] = {{OP_JMP,-1,0,0}};
static const InstrWord no_halt[] = {{OP_NOP,0,0,0}};

static const Case cases[] = {
    {"arithmetic-bitwise",arithmetic,WORDS(arithmetic),seed_arithmetic,NULL,verify_arithmetic},
    {"shifts",shifts,WORDS(shifts),seed_control,NULL,NULL},
    {"compare-equal",cmp_equal,WORDS(cmp_equal),seed_comparison,NULL,verify_equal},
    {"compare-less",cmp_less,WORDS(cmp_less),seed_comparison,NULL,verify_less},
    {"jumps",jumps,WORDS(jumps),seed_control,NULL,verify_jump},
    {"memory-stack",memory_stack,WORDS(memory_stack),seed_memory,NULL,verify_memory},
    {"ctx-push",context_push,WORDS(context_push),NULL,"transaction context operation not reconstructed",verify_fault_pc_one},
    {"ctx-commit",context_commit,WORDS(context_commit),NULL,"transaction context operation not reconstructed",verify_fault_pc_one},
    {"ctx-abort",context_abort,WORDS(context_abort),NULL,"transaction context operation not reconstructed",verify_fault_pc_one},
    {"ctx-haschk",context_haschk,WORDS(context_haschk),NULL,"transaction context operation not reconstructed",verify_fault_pc_one},
    {"ctx-copy",context_copy,WORDS(context_copy),NULL,"transaction context operation not reconstructed",verify_fault_pc_one},
    {"opcode-negative",bad_opcode_negative,WORDS(bad_opcode_negative),NULL,"unknown opcode",verify_fault_pc_one},
    {"opcode-high",bad_opcode_high,WORDS(bad_opcode_high),NULL,"unknown opcode",verify_fault_pc_one},
    {"register-a",bad_register_a,WORDS(bad_register_a),NULL,"register index out of range",verify_fault_pc_one},
    {"register-b",bad_register_b,WORDS(bad_register_b),NULL,"register index out of range",verify_fault_pc_one},
    {"divide-zero",divide_zero,WORDS(divide_zero),seed_zero_divisor,"division by zero",verify_fault_pc_one},
    {"modulo-zero",modulo_zero,WORDS(modulo_zero),seed_zero_divisor,"modulo by zero",verify_fault_pc_one},
    {"load-negative",load_negative,WORDS(load_negative),seed_bad_load_negative,"load address out of range",verify_fault_pc_one},
    {"load-high",load_high,WORDS(load_high),seed_bad_load_high,"load address out of range",verify_fault_pc_one},
    {"store-high",store_high,WORDS(store_high),seed_bad_store,"store address out of range",verify_fault_pc_one},
    {"stack-underflow",stack_underflow,WORDS(stack_underflow),NULL,"stack underflow",verify_fault_pc_one},
    {"stack-overflow",stack_overflow,WORDS(stack_overflow),seed_full_stack,"stack overflow",verify_fault_pc_one},
    {"jump-negative",jump_negative,WORDS(jump_negative),NULL,"jump target out of range",verify_fault_pc_one},
    {"missing-halt",no_halt,WORDS(no_halt),NULL,"program ended without HALT",verify_fault_pc_one},
};

static void run_case(const Case *test_case) {
    vm_context reference;
    bool reference_result = false;
    for (size_t index = 0; index < WORDS(engines); ++index) {
        vm_context actual;
        vm_init(&actual);
        if (test_case->seed != NULL) test_case->seed(&actual);
        const bool result = engines[index].run(&actual, test_case->program,
                                               test_case->length);
        const bool expected_result = test_case->fault == NULL;
        if (result != expected_result) fail(test_case->name, engines[index].name, "unexpected success/failure result");
        if (!same_fault(actual.fault, test_case->fault)) fail(test_case->name, engines[index].name, "unexpected fault");
        if (test_case->verify != NULL && !test_case->verify(&actual)) fail(test_case->name, engines[index].name, "state invariant failed");
        if (index == 0) {
            reference = actual;
            reference_result = result;
        } else if (result != reference_result || !same_state(&actual, &reference)) {
            fail(test_case->name, engines[index].name, "engine state differs from computed-goto reference");
        }
    }
}

int main(void) {
    for (size_t index = 0; index < WORDS(cases); ++index) run_case(&cases[index]);

    vm_context ctx;
    vm_init(&ctx);
    for (size_t index = 0; index < WORDS(engines); ++index) {
        if (engines[index].run(NULL, arithmetic, WORDS(arithmetic)))
            fail("null-context", engines[index].name, "accepted null context");
        if (engines[index].run(&ctx, NULL, WORDS(arithmetic)))
            fail("null-program", engines[index].name, "accepted null program");
    }

    printf("TinyVM ISA conformance: %zu cases passed across %zu engines\n",
           WORDS(cases), WORDS(engines));
    return 0;
}

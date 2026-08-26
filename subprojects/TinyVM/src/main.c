#include <tinyvm/tinyvm.h>

#include <inttypes.h>
#include <stdio.h>

int main(void) {
    const InstrWord program[] = {
        {OP_ADD, 0, 1, 0},
        {OP_PUSH, 0, 0, 0},
        {OP_POP, 2, 0, 0},
        {OP_HALT, 0, 0, 0},
    };
    vm_context ctx;
    vm_init(&ctx);
    ctx.regs[0] = 19;
    ctx.regs[1] = 23;

    if (!vm_run(&ctx, program, sizeof(program) / sizeof(program[0]))) {
        fprintf(stderr, "tinyvm fault: %s\n", ctx.fault);
        return 1;
    }
    printf("result=%" PRId64 " halted pc=%zu\n", ctx.regs[2], ctx.pc);
    return ctx.regs[2] == 42 ? 0 : 1;
}

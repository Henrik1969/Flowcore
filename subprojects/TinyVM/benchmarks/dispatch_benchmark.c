#define _POSIX_C_SOURCE 200809L

#include <tinyvm/tinyvm.h>

#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef bool (*runner_fn)(vm_context *, const InstrWord *, size_t);

typedef struct {
    const char *name;
    runner_fn run;
} Runner;

static volatile uint64_t result_sink;

static uint64_t monotonic_ns(void) {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
        perror("clock_gettime");
        exit(2);
    }
    return (uint64_t)now.tv_sec * UINT64_C(1000000000) + (uint64_t)now.tv_nsec;
}

static size_t parse_size(const char *text, const char *option) {
    char *end = NULL;
    errno = 0;
    const unsigned long long value = strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || value == 0 || value > SIZE_MAX) {
        fprintf(stderr, "invalid %s value: %s\n", option, text);
        exit(2);
    }
    return (size_t)value;
}

typedef enum { WORKLOAD_NOP, WORKLOAD_PERIODIC, WORKLOAD_SHUFFLED } Workload;

static void make_program(InstrWord *program, size_t words, Workload workload) {
    static const VmOpcode mixed_opcodes[] = {
        OP_ADD, OP_XOR, OP_MOV, OP_CMP, OP_AND, OP_OR, OP_NOT, OP_NOP
    };
    uint32_t random_state = UINT32_C(0x6d2b79f5);
    for (size_t index = 0; index + 1 < words; ++index) {
        if (workload == WORKLOAD_NOP) {
            program[index] = (InstrWord){OP_NOP, 0, 0, 0};
            continue;
        }
        size_t selection = index % 8;
        if (workload == WORKLOAD_SHUFFLED) {
            random_state = random_state * UINT32_C(1664525) + UINT32_C(1013904223);
            selection = (random_state >> 16) % 8;
        }
        switch (mixed_opcodes[selection]) {
        case OP_ADD: program[index] = (InstrWord){OP_ADD, 0, 1, 0}; break;
        case OP_XOR: program[index] = (InstrWord){OP_XOR, 2, 0, 0}; break;
        case OP_MOV: program[index] = (InstrWord){OP_MOV, 3, 2, 0}; break;
        case OP_CMP: program[index] = (InstrWord){OP_CMP, 3, 4, 0}; break;
        case OP_AND: program[index] = (InstrWord){OP_AND, 3, 5, 0}; break;
        case OP_OR: program[index] = (InstrWord){OP_OR, 2, 6, 0}; break;
        case OP_NOT: program[index] = (InstrWord){OP_NOT, 7, 0, 0}; break;
        default: program[index] = (InstrWord){OP_NOP, 0, 0, 0}; break;
        }
    }
    program[words - 1] = (InstrWord){OP_HALT, 0, 0, 0};
}

static uint64_t execute(const Runner *runner, const InstrWord *program,
                        size_t words, size_t iterations, uint64_t *checksum_out) {
    uint64_t checksum = 0;
    const uint64_t start = monotonic_ns();
    for (size_t iteration = 0; iteration < iterations; ++iteration) {
        vm_context ctx;
        vm_init(&ctx);
        ctx.regs[0] = 1;
        ctx.regs[1] = 3;
        ctx.regs[2] = 5;
        ctx.regs[3] = 7;
        ctx.regs[4] = 11;
        ctx.regs[5] = 13;
        ctx.regs[6] = 17;
        ctx.regs[7] = 19;
        if (!runner->run(&ctx, program, words)) {
            fprintf(stderr, "%s faulted: %s\n", runner->name, ctx.fault);
            exit(1);
        }
        for (size_t reg = 0; reg < 8; ++reg) checksum ^= (uint64_t)ctx.regs[reg];
        checksum += ctx.pc + (uint64_t)iteration;
    }
    const uint64_t elapsed = monotonic_ns() - start;
    result_sink ^= checksum;
    if (checksum_out != NULL) *checksum_out = checksum;
    return elapsed;
}

static void run_workload(const char *name, InstrWord *program, size_t words,
                         size_t iterations, Workload workload) {
    const Runner runners[] = {
        {"computed_goto", vm_run},
        {"switch", vm_run_switch},
        {"function_pointer", vm_run_function},
    };
    make_program(program, words, workload);
    printf("workload=%s words=%zu iterations=%zu\n", name, words, iterations);
    uint64_t reference_checksum = 0;
    for (size_t index = 0; index < sizeof(runners) / sizeof(runners[0]); ++index) {
        uint64_t checksum = 0;
        (void)execute(&runners[index], program, words, 2, NULL);
        const uint64_t elapsed = execute(&runners[index], program, words,
                                         iterations, &checksum);
        if (index == 0) {
            reference_checksum = checksum;
        } else if (checksum != reference_checksum) {
            fprintf(stderr, "%s semantic checksum mismatch in %s\n",
                    runners[index].name, name);
            exit(1);
        }
        const double operations = (double)words * (double)iterations;
        printf("  %-16s %10.3f ns/op %10.2f Mops/s elapsed=%.3f s\n",
               runners[index].name, (double)elapsed / operations,
               operations * 1000.0 / (double)elapsed,
               (double)elapsed / 1000000000.0);
    }
}

int main(int argc, char **argv) {
    size_t iterations = 5000;
    size_t words = 4096;
    for (int index = 1; index < argc; ++index) {
        if (strcmp(argv[index], "--iterations") == 0 && index + 1 < argc) {
            iterations = parse_size(argv[++index], "--iterations");
        } else if (strcmp(argv[index], "--words") == 0 && index + 1 < argc) {
            words = parse_size(argv[++index], "--words");
        } else {
            fprintf(stderr, "usage: %s [--iterations N] [--words N]\n", argv[0]);
            return 2;
        }
    }
    if (words < 2) {
        fputs("--words must be at least 2\n", stderr);
        return 2;
    }

    InstrWord *program = calloc(words, sizeof(*program));
    if (program == NULL) {
        perror("calloc");
        return 2;
    }

#if defined(__clang__)
    printf("compiler=clang-%s optimization=%s\n", __clang_version__,
#else
    printf("compiler=gcc-%s optimization=%s\n", __VERSION__,
#endif
#if defined(__OPTIMIZE__)
           "enabled");
#else
           "disabled");
#endif
    run_workload("dispatch", program, words, iterations, WORKLOAD_NOP);
    run_workload("mixed_periodic", program, words, iterations, WORKLOAD_PERIODIC);
    run_workload("mixed_shuffled", program, words, iterations, WORKLOAD_SHUFFLED);
    printf("checksum=%" PRIu64 "\n", result_sink);
    free(program);
    return 0;
}

#include "flow_terminal.h"

#include <pty.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

static int child_probe(const char *term) {
    int master = -1;
    int slave = -1;
    if (openpty(&master, &slave, NULL, NULL, NULL) != 0) return 10;
    pid_t child = fork();
    if (child < 0) return 11;
    if (child == 0) {
        close(master);
        if (dup2(slave, STDIN_FILENO) < 0 || dup2(slave, STDOUT_FILENO) < 0) _exit(20);
        close(slave);
        if (setenv("TERM", term, 1) != 0) _exit(21);
        if (flow_terminal_open() == NULL) _exit(22);
        if (flow_terminal_enter() != 0) _exit(23);
        if (flow_terminal_write("ok", 2) != 0) _exit(24);
        if (flow_terminal_present() != 0) _exit(25);
        int event = flow_terminal_read_event();
        if (flow_terminal_close() != 0) _exit(26);
        _exit(event == FLOW_TERMINAL_EVENT_UP ? 0 : 27);
    }
    close(slave);
    char output[2];
    if (read(master, output, sizeof output) != 2 || memcmp(output, "ok", 2) != 0) return 12;
    if (write(master, "\033[A", 3) != 3) return 13;
    int status = 0;
    if (waitpid(child, &status, 0) != child) return 14;
    close(master);
    return WIFEXITED(status) ? WEXITSTATUS(status) : 15;
}

static int forced_termination_probe(void) {
    int master = -1;
    int slave = -1;
    struct termios original;
    if (openpty(&master, &slave, NULL, NULL, NULL) != 0) return 30;
    if (tcgetattr(slave, &original) != 0) return 31;
    pid_t child = fork();
    if (child < 0) return 32;
    if (child == 0) {
        close(master);
        if (dup2(slave, STDIN_FILENO) < 0 || dup2(slave, STDOUT_FILENO) < 0) _exit(40);
        close(slave);
        if (flow_terminal_open() == NULL || flow_terminal_enter() != 0) _exit(41);
        if (flow_terminal_write("R", 1) != 0) _exit(42);
        for (;;) pause();
    }
    char ready;
    if (read(master, &ready, 1) != 1 || ready != 'R') return 33;
    if (kill(child, SIGKILL) != 0) return 34;
    if (waitpid(child, NULL, 0) != child) return 35;

    for (int attempt = 0; attempt != 100; ++attempt) {
        struct termios current;
        if (tcgetattr(slave, &current) == 0 &&
            (current.c_lflag & (ICANON | ECHO)) == (original.c_lflag & (ICANON | ECHO))) {
            close(master);
            close(slave);
            return 0;
        }
        usleep(10000);
    }
    return 36;
}

int main(void) {
    const char *terms[] = {"xterm-kitty", "alacritty", "xterm", "dumb", "invented-terminal"};
    for (size_t index = 0; index < sizeof terms / sizeof terms[0]; ++index) {
        int result = child_probe(terms[index]);
        if (result != 0) {
            fprintf(stderr, "projection %s failed: %d\n", terms[index], result);
            return result;
        }
    }
    int forced = forced_termination_probe();
    if (forced != 0) {
        fprintf(stderr, "forced termination restoration failed: %d\n", forced);
        return forced;
    }
    puts("FLOWTERMINAL_PROVIDER_PASS projections=5 terminfo=unused sigkill_restore=pass");
    return 0;
}

#define _DEFAULT_SOURCE

#include <errno.h>
#include <poll.h>
#include <pty.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <unistd.h>

static int contains(const char *buffer, size_t used, const char *needle) {
    size_t length = strlen(needle);
    if (length > used) return 0;
    for (size_t index = 0; index + length <= used; ++index)
        if (memcmp(buffer + index, needle, length) == 0) return 1;
    return 0;
}

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    int master = -1, slave = -1;
    if (openpty(&master, &slave, NULL, NULL, NULL) != 0) return 3;
    pid_t child = fork();
    if (child < 0) return 4;
    if (child == 0) {
        close(master);
        setsid();
        if (ioctl(slave, TIOCSCTTY, 0) != 0) _exit(119);
        if (dup2(slave, STDIN_FILENO) < 0 || dup2(slave, STDOUT_FILENO) < 0 ||
            dup2(slave, STDERR_FILENO) < 0) _exit(120);
        if (slave > STDERR_FILENO) close(slave);
        setenv("TERM", "invented-terminal", 1);
        execl(argv[1], argv[1], "alpha", "beta", "gamma", (char *)NULL);
        _exit(121);
    }
    close(slave);

    char output[8192];
    size_t used = 0;
    int sent = 0;
    for (int attempts = 0; attempts < 100 && used < sizeof output; ++attempts) {
        struct pollfd descriptor = { .fd = master, .events = POLLIN };
        int ready = poll(&descriptor, 1, 100);
        if (ready > 0 && (descriptor.revents & POLLIN)) {
            ssize_t count = read(master, output + used, sizeof output - used);
            if (count > 0) used += (size_t)count;
        }
        if (!sent && contains(output, used, "> alpha\r\n  beta\r\n  gamma")) {
            static const char down_enter[] = "\033[B\r";
            if (write(master, down_enter, sizeof down_enter - 1) < 0) return 5;
            sent = 1;
        }
        int status = 0;
        pid_t result = waitpid(child, &status, WNOHANG);
        if (result == child) {
            close(master);
            if (!WIFEXITED(status) || WEXITSTATUS(status) != 0 || !sent) return 6;
            if (!contains(output, used, "  alpha\r\n> beta\r\n  gamma")) return 7;
            if (!contains(output, used, "beta\r\n")) return 8;
            puts("FLOWSELECTION_NATIVE_PROJECTION_PASS candidates=3 selected=beta terminfo=unused");
            return 0;
        }
    }
    kill(child, SIGKILL);
    waitpid(child, NULL, 0);
    close(master);
    return 9;
}

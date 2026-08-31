#include "flow_terminal.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <termios.h>
#include <unistd.h>

struct flow_terminal_session {
    struct termios saved;
    bool open;
    bool tty_mode;
    int guardian_pipe;
    pid_t guardian;
};

static struct flow_terminal_session session;
static bool exit_handler_registered;

static void restore_at_exit(void) {
    if (session.open) (void)flow_terminal_close();
}

static int start_guardian(void) {
    int channel[2];
    if (pipe(channel) != 0) return -1;
    if (fcntl(channel[0], F_SETFD, FD_CLOEXEC) != 0 || fcntl(channel[1], F_SETFD, FD_CLOEXEC) != 0) {
        close(channel[0]);
        close(channel[1]);
        return -1;
    }
    pid_t guardian = fork();
    if (guardian < 0) {
        close(channel[0]);
        close(channel[1]);
        return -1;
    }
    if (guardian == 0) {
        const pid_t owner = getppid();
        struct pollfd watch = {.fd = channel[0], .events = POLLIN | POLLHUP};
        close(channel[1]);
        for (;;) {
            int ready = poll(&watch, 1, 100);
            if (ready > 0 || getppid() != owner) break;
            if (ready < 0 && errno != EINTR) break;
        }
        close(channel[0]);
        (void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &session.saved);
        _exit(0);
    }
    close(channel[0]);
    session.guardian_pipe = channel[1];
    session.guardian = guardian;
    return 0;
}

static int read_byte(unsigned char *byte, int timeout_ms) {
    struct pollfd descriptor = {.fd = STDIN_FILENO, .events = POLLIN};
    int ready;
    do {
        ready = poll(&descriptor, 1, timeout_ms);
    } while (ready < 0 && errno == EINTR);
    if (ready <= 0) return ready;

    ssize_t count;
    do {
        count = read(STDIN_FILENO, byte, 1);
    } while (count < 0 && errno == EINTR);
    return count == 1 ? 1 : (count == 0 ? 0 : -1);
}

void *flow_terminal_open(void) {
    if (session.open) return NULL;
    memset(&session, 0, sizeof session);
    session.guardian_pipe = -1;
    session.open = true;
    if (!exit_handler_registered) {
        if (atexit(restore_at_exit) != 0) {
            memset(&session, 0, sizeof session);
            return NULL;
        }
        exit_handler_registered = true;
    }
    return &session;
}

int flow_terminal_enter(void) {
    if (!session.open) return -1;
    if (!isatty(STDIN_FILENO)) return 0;
    if (tcgetattr(STDIN_FILENO, &session.saved) != 0) return -1;

    struct termios active = session.saved;
    active.c_lflag &= (tcflag_t)~(ICANON | ECHO);
    active.c_cc[VMIN] = 1;
    active.c_cc[VTIME] = 0;
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &active) != 0) return -1;
    session.tty_mode = true;
    if (start_guardian() != 0) {
        (void)tcsetattr(STDIN_FILENO, TCSAFLUSH, &session.saved);
        session.tty_mode = false;
        return -1;
    }
    return 0;
}

int flow_terminal_write(const char *text, int count) {
    if (!session.open || text == NULL || count < 0) return -1;
    size_t remaining = (size_t)count;
    while (remaining != 0) {
        ssize_t written = write(STDOUT_FILENO, text, remaining);
        if (written < 0 && errno == EINTR) continue;
        if (written <= 0) return -1;
        text += written;
        remaining -= (size_t)written;
    }
    return 0;
}

int flow_terminal_present(void) {
    if (!session.open) return -1;
    return isatty(STDOUT_FILENO) ? tcdrain(STDOUT_FILENO) : 0;
}

int flow_terminal_read_event(void) {
    unsigned char first;
    int status;
    if (!session.open) return FLOW_TERMINAL_EVENT_ERROR;
    status = read_byte(&first, -1);
    if (status <= 0) return status == 0 ? FLOW_TERMINAL_EVENT_EOF : FLOW_TERMINAL_EVENT_ERROR;
    if (first == '\r' || first == '\n') return FLOW_TERMINAL_EVENT_ENTER;
    if (first != 0x1b) return (int)first;

    unsigned char second;
    status = read_byte(&second, 25);
    if (status <= 0) return FLOW_TERMINAL_EVENT_ESCAPE;
    if (second != '[' && second != 'O') return FLOW_TERMINAL_EVENT_ESCAPE;

    unsigned char third;
    status = read_byte(&third, 25);
    if (status <= 0) return FLOW_TERMINAL_EVENT_ESCAPE;
    switch (third) {
        case 'A': return FLOW_TERMINAL_EVENT_UP;
        case 'B': return FLOW_TERMINAL_EVENT_DOWN;
        case 'C': return FLOW_TERMINAL_EVENT_RIGHT;
        case 'D': return FLOW_TERMINAL_EVENT_LEFT;
        default: return FLOW_TERMINAL_EVENT_ESCAPE;
    }
}

int flow_terminal_capabilities(void) {
    int capabilities = FLOW_TERMINAL_CAP_INPUT | FLOW_TERMINAL_CAP_BYTE_OUTPUT;
    if (isatty(STDIN_FILENO)) capabilities |= FLOW_TERMINAL_CAP_TTY_MODE;
    return capabilities;
}

int flow_terminal_close(void) {
    int result = 0;
    if (!session.open) return -1;
    if (session.tty_mode && session.guardian > 0) {
        close(session.guardian_pipe);
        while (waitpid(session.guardian, NULL, 0) < 0) {
            if (errno != EINTR) { result = -1; break; }
        }
    } else if (session.tty_mode && tcsetattr(STDIN_FILENO, TCSAFLUSH, &session.saved) != 0) {
        result = -1;
    }
    memset(&session, 0, sizeof session);
    return result;
}

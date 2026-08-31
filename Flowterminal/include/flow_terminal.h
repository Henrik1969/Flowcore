#ifndef FLOWCORE_FLOW_TERMINAL_H
#define FLOWCORE_FLOW_TERMINAL_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum flow_terminal_event {
    FLOW_TERMINAL_EVENT_ERROR = -1,
    FLOW_TERMINAL_EVENT_EOF = 0,
    FLOW_TERMINAL_EVENT_ENTER = 0x100,
    FLOW_TERMINAL_EVENT_ESCAPE,
    FLOW_TERMINAL_EVENT_UP,
    FLOW_TERMINAL_EVENT_DOWN,
    FLOW_TERMINAL_EVENT_RIGHT,
    FLOW_TERMINAL_EVENT_LEFT,
    FLOW_TERMINAL_EVENT_RESIZE
};

enum flow_terminal_capability {
    FLOW_TERMINAL_CAP_INPUT = 1u << 0,
    FLOW_TERMINAL_CAP_TTY_MODE = 1u << 1,
    FLOW_TERMINAL_CAP_BYTE_OUTPUT = 1u << 2
};

/* Version 1 is deliberately terminal-brand-neutral. It never reads TERM and
 * never opens a terminfo database. Unknown projections degrade to byte I/O. */
void *flow_terminal_open(void);
int flow_terminal_close(void);
int flow_terminal_enter(void);
int flow_terminal_write(const char *text, int count);
int flow_terminal_present(void);
int flow_terminal_read_event(void);
int flow_terminal_capabilities(void);

#ifdef __cplusplus
}
#endif

#endif

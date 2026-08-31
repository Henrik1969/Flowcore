#include <flow_terminal.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

enum { EVENT_ENTER = 0x100, EVENT_UP = 0x102, EVENT_DOWN = 0x103 };

int main(int argc, char **argv) {
    if (argc < 2) return 2;
    int result = dup(STDOUT_FILENO);
    int terminal = open("/dev/tty", O_RDWR);
    if (result < 0 || terminal < 0) return 2;
    if (dup2(terminal, STDIN_FILENO) < 0 || dup2(terminal, STDOUT_FILENO) < 0) return 2;
    close(terminal);
    if (flow_terminal_open() == NULL || flow_terminal_enter() != 0) return 2;

    int selected = 1;
    for (;;) {
        const char prefix[] = "cursor: ";
        flow_terminal_write(prefix, (int)(sizeof prefix - 1));
        flow_terminal_write(argv[selected], (int)strlen(argv[selected]));
        flow_terminal_write("\n", 1);
        flow_terminal_present();
        int event = flow_terminal_read_event();
        if (event == 'q' || event == 0) {
            flow_terminal_close();
            close(result);
            return 1;
        }
        if (event == EVENT_ENTER) break;
        if (event == EVENT_UP && selected > 1) --selected;
        if (event == EVENT_DOWN && selected + 1 < argc) ++selected;
    }
    if (flow_terminal_close() != 0) return 2;
    dprintf(result, "%s\n", argv[selected]);
    close(result);
    return 0;
}

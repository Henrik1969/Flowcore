#include <flow_terminal.h>

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

enum { EVENT_ENTER = 0x100, EVENT_UP = 0x102, EVENT_DOWN = 0x103 };

static int render_candidates(int argc, char **argv, int selected) {
    static const char heading[] = "Select one (Up/Down, Enter; q cancels):\n";
    if (flow_terminal_write(heading, (int)(sizeof heading - 1)) != 0) return -1;
    for (int index = 1; index < argc; ++index) {
        const char *marker = index == selected ? "> " : "  ";
        if (flow_terminal_write(marker, 2) != 0 ||
            flow_terminal_write(argv[index], (int)strlen(argv[index])) != 0 ||
            flow_terminal_write("\n", 1) != 0) return -1;
    }
    return flow_terminal_present();
}

int main(int argc, char **argv) {
    if (argc < 2) return 2;
    int result = dup(STDOUT_FILENO);
    int terminal = open("/dev/tty", O_RDWR);
    if (result < 0 || terminal < 0) return 2;
    if (dup2(terminal, STDIN_FILENO) < 0 || dup2(terminal, STDOUT_FILENO) < 0) return 2;
    close(terminal);
    if (flow_terminal_open() == NULL || flow_terminal_enter() != 0) return 2;

    int selected = 1;
    if (render_candidates(argc, argv, selected) != 0) return 2;
    for (;;) {
        int event = flow_terminal_read_event();
        if (event == 'q' || event == 0) {
            flow_terminal_close();
            close(result);
            return 1;
        }
        if (event == EVENT_ENTER) break;
        int previous = selected;
        if (event == EVENT_UP && selected > 1) --selected;
        if (event == EVENT_DOWN && selected + 1 < argc) ++selected;
        if (selected != previous && render_candidates(argc, argv, selected) != 0) {
            flow_terminal_close();
            close(result);
            return 2;
        }
    }
    if (flow_terminal_close() != 0) return 2;
    dprintf(result, "%s\n", argv[selected]);
    close(result);
    return 0;
}

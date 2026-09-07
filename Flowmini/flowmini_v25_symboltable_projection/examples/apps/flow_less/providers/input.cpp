#include <cerrno>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <vector>
#ifdef FLOW_PAGER_TERMINAL
#include <dlfcn.h>
#endif

namespace {
std::vector<std::string> lines, commands;
int page_size = 2;
const char* setting(const char* name, const char* fallback) {
    const auto* value = std::getenv(name); return value ? value : fallback;
}
int number(const char* name, int fallback) {
    const auto* value = std::getenv(name); if (!value) return fallback;
    char* end = nullptr; errno = 0; const auto result = std::strtol(value, &end, 10);
    return errno || !end || *end || result < INT_MIN || result > INT_MAX ? 0 : static_cast<int>(result);
}
std::vector<std::string> split(const std::string& input, char separator) {
    std::vector<std::string> result;
    if (input.empty()) return result;
    std::size_t start = 0;
    for (;;) {
        const auto end = input.find(separator, start);
        result.push_back(input.substr(start, end == std::string::npos ? end : end - start));
        if (end == std::string::npos) return result;
        start = end + 1;
    }
}
#ifdef FLOW_PAGER_TERMINAL
template <class Function> Function symbol(void* library, const char* name) {
    return reinterpret_cast<Function>(dlsym(library, name));
}
int terminal_input() {
    const auto count = number("FLOW_PAGER_KEY_COUNT", 1);
    if (count < 1 || count > 4096) { std::fputs("invalid terminal input batch size\n", stderr); return 1; }
    void* library = dlopen("libncursesw.so.6", RTLD_NOW | RTLD_LOCAL);
    if (!library) { std::fputs("terminal provider unavailable\n", stderr); return 1; }
    const auto init = symbol<void*(*)()>(library, "initscr");
    const auto end = symbol<int(*)()>(library, "endwin");
    const auto noecho = symbol<int(*)()>(library, "noecho");
    const auto cbreak = symbol<int(*)()>(library, "cbreak");
    const auto keypad = symbol<int(*)(void*, bool)>(library, "keypad");
    const auto refresh = symbol<int(*)(void*)>(library, "wrefresh");
    const auto getch = symbol<int(*)(void*)>(library, "wgetch");
    if (!init || !end || !noecho || !cbreak || !keypad || !refresh || !getch) { dlclose(library); return 1; }
    void* window = init();
    if (!window) { dlclose(library); return 1; }
    int status = 0;
    try {
        if (noecho() < 0 || cbreak() < 0 || keypad(window, 1) < 0 || refresh(window) < 0) status = 1;
        for (int index = 0; !status && index < count; ++index) {
            const int key = getch(window);
            if (key < 0) { status = 1; break; }
            // Transport raw terminal key codes. Flow owns their interpretation.
            commands.push_back(std::to_string(key));
        }
    } catch (...) { status = 1; }
    if (end() < 0) status = 1;
    dlclose(library);
    if (status) std::fputs("terminal input failed\n", stderr);
    return status;
}
#endif
}
extern "C" int pager_input_start() {
    try {
        lines.clear(); commands.clear(); page_size = number("FLOW_PAGER_PAGE_SIZE", 2);
#ifdef FLOW_PAGER_TERMINAL
        const auto* path = setting("FLOW_PAGER_PATH", "");
        std::ifstream input(path);
        if (!input) { std::fprintf(stderr, "unable to open text file: %s\n", path); return 1; }
        std::string line;
        while (std::getline(input, line)) {
            if (lines.size() >= 1000000) return 1;
            lines.push_back(line);
        }
        if (!input.eof()) return 1;
        return terminal_input();
#else
        lines = split(setting("FLOW_PAGER_LINES", "alpha|beta|gamma|delta|epsilon"), '|');
        commands = split(setting("FLOW_PAGER_COMMANDS", "pgdown,end"), ',');
        if (lines.size() > 1000000 || commands.size() > 4096) return 1;
        return 0;
#endif
    } catch (...) { std::fputs("input provider storage failure\n", stderr); return 1; }
}
extern "C" int pager_line_count() { return static_cast<int>(lines.size()); }
extern "C" int pager_command_count() { return static_cast<int>(commands.size()); }
extern "C" int pager_page_size() { return page_size; }
extern "C" const char* pager_line_at(int index) {
    return index >= 0 && static_cast<std::size_t>(index) < lines.size() ? lines[index].c_str() : "";
}
extern "C" const char* pager_command_at(int index) {
    return index >= 0 && static_cast<std::size_t>(index) < commands.size() ? commands[index].c_str() : "";
}

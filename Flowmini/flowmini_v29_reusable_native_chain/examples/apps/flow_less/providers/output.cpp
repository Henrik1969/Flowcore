#include <cstdio>
extern "C" int pager_write_text(const char* text) { return std::fputs(text, stdout) < 0 ? -1 : 0; }
extern "C" int pager_write_line(const char* text) { return std::printf("%s\n", text) < 0 ? -1 : 0; }
extern "C" int pager_write_integer(int value) { return std::printf("%d", value) < 0 ? -1 : 0; }
extern "C" int pager_diagnostic(const char* text) { return std::fprintf(stderr, "%s\n", text) < 0 ? -1 : 0; }
extern "C" int pager_error_text(const char* text) { return std::fputs(text, stderr) < 0 ? -1 : 0; }

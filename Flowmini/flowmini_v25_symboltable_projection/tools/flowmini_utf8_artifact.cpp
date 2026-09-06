#include "flowmini_utf8.h"

#include <iostream>
#include <iterator>
#include <string>

int main() {
    const std::string bytes{
        std::istreambuf_iterator<char>{std::cin},
        std::istreambuf_iterator<char>{}
    };
    flowmini::writeUtf8Artifact(std::cout, bytes);
    return 0;
}

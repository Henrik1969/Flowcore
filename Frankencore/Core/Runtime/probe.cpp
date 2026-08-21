#include "frankencore/runtime.hpp"

#include <iostream>

int main() {
    std::cout << frankencore::runtime::to_json(frankencore::runtime::discover());
    return 0;
}

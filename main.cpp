#include <iostream>
#include "include/code_fmt_stream.h"

int main() {
    fmt::fmtstream fmtstream{std::cout.rdbuf()};
    for (int i = 0; i < 2; ++i) {
        fmtstream << "namespace test " << fmt::begin('{')
                  << "int func() " << fmt::begin('{')
                  << "return 5;" << fmt::end('}')
                  << fmt::end('}');
    }
    return 0;
}
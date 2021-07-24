#include "binrec/tracing/trace_info.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>

using namespace binrec;

auto main(int argc, char *argv[]) -> int
{
    TraceInfo mergeTi;

    std::for_each(argv + 1, argv + argc - 1, [&mergeTi](char *arg) {
        TraceInfo ti;
        std::ifstream is{arg};
        if (!is) {
            std::cout << "Can't open file " << arg << '\n';
            exit(1);
        }
        is >> ti;
        mergeTi.add(ti);
    });

    std::ofstream os{argv[argc - 1]};
    os << mergeTi;

    return 0;
}

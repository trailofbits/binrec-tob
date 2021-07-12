#include <iostream>

namespace {
int fib(int n) {
    if (n <= 1)
        return n;
    return fib(n - 1) + fib(n - 2);
}
} // namespace

int main(int argc, char **argv) {
    if (argc >= 2) {
        int n = atoi(argv[1]);
        std::cout << "fib(" << n << ") = " << fib(n) << "\n";
    }
    return 0;
}

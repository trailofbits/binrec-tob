#include <iostream>

int fib(int n) {
    if (n< 2) return n;
    return n*fib(n-1);
    //return n > 1 ? n * fib(n - 1) : n;
}

int main(int argc, char **argv) {
    if (argc >= 2) {
        int n = atoi(argv[1]);
        std::cout << fib(n);
    }
    return 0;
}

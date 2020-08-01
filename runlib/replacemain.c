#define _GNU_SOURCE
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

typedef int (*main_t)(int, char**, char**);
typedef void (*fun_t)(void);
typedef int (*__libc_start_main_t)(
        main_t main,
        int argc,
        char **ubp_av,
        fun_t init,
        fun_t fini,
        fun_t rtld_fini,
        void *stack_end
        );
typedef __attribute__((noreturn)) void (*exit_t)(int status);

int __libc_start_main(main_t main, int argc, char **ubp_av, fun_t init, fun_t
        fini, fun_t rtld_fini, void *stack_end) {
    char *main_env;
    main_t newmain;

    if (!(main_env = getenv("NEWMAIN"))) {
        fputs("error: NEWMAIN not set in environment\n", stderr);
        exit(1);
    }

    newmain = (main_t)strtol(main_env, NULL, 16);

    __libc_start_main_t orig_libc_start_main = (__libc_start_main_t)dlsym(RTLD_NEXT, "__libc_start_main");
    (*orig_libc_start_main)(newmain, argc, ubp_av, init, fini, rtld_fini, stack_end);

    exit(1);  // this is never reached
}

void __attribute__((noreturn)) exit(int status) {
    fputs("program exited with exit()\n", stderr);
    exit_t orig_exit = (exit_t)dlsym(RTLD_NEXT, "exit");
    (*orig_exit)(status);
}

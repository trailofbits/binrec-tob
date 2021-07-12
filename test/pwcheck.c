#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PASSWORD "loltest"
#define CHECK(c, msg)                                                                                                  \
    if (!(c)) {                                                                                                        \
        puts(msg);                                                                                                     \
        return 1;                                                                                                      \
    }

int main(int argc, char **argv) {
    const char *const passwd = PASSWORD;
    unsigned passwdlen, i;

    CHECK(argc >= 2, "missing password argument");

    CHECK(strlen(argv[1]) == (passwdlen = strlen(passwd)), "wrong password length");

    for (i = 0; i != passwdlen; i++)
        CHECK(argv[1][i] == passwd[i], "incorrect password");

    puts("password is correct!");
    return 0;
}

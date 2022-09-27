#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char *line = NULL;
    size_t len = 0;
    ssize_t read;
    int running = 1;
    int lineno = 1;

    while ((read = getline(&line, &len, stdin)) != -1) {
        printf("Line %d: ", lineno);
        fwrite(line, read, 1, stdout);
        if (strcmp(line, "exit") == 0 || strcmp(line, "exit\n") == 0) {
            printf("\nexiting...\n");
            break;
        }
        ++lineno;
    }

    free(line);

    return 0;
}
/*
 Brainfuck-C ( http://github.com/kgabis/brainfuck-c )
 Copyright (c) 2012 Krzysztof Gabis

 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
*/

#include <stdio.h>

#define OP_END 0
#define OP_INC_DP 1
#define OP_DEC_DP 2
#define OP_INC_VAL 3
#define OP_DEC_VAL 4
#define OP_OUT 5
#define OP_IN 6
#define OP_JMP_FWD 7
#define OP_JMP_BCK 8

#define SUCCESS 0
#define FAILURE 1

#define PROGRAM_SIZE 14000
#define STACK_SIZE 512
#define DATA_SIZE 65535

#define STACK_PUSH(A) (STACK[SP++] = A)
#define STACK_POP() (STACK[--SP])
#define STACK_EMPTY() (SP == 0)
#define STACK_FULL() (SP == STACK_SIZE)

struct instruction_t {
    unsigned short operator;
    unsigned short operand;
};

static struct instruction_t PROGRAM[PROGRAM_SIZE];
static unsigned short STACK[STACK_SIZE];
static unsigned int SP = 0;

int compile_bf(FILE *fp) {
    unsigned short pc = 0, jmp_pc;
    int c;
    while ((c = getc(fp)) != EOF && pc < PROGRAM_SIZE) {
        switch (c) {
        case '>':
            PROGRAM[pc].operator= OP_INC_DP;
            break;
        case '<':
            PROGRAM[pc].operator= OP_DEC_DP;
            break;
        case '+':
            PROGRAM[pc].operator= OP_INC_VAL;
            break;
        case '-':
            PROGRAM[pc].operator= OP_DEC_VAL;
            break;
        case '.':
            PROGRAM[pc].operator= OP_OUT;
            break;
        case ',':
            PROGRAM[pc].operator= OP_IN;
            break;
        case '[':
            PROGRAM[pc].operator= OP_JMP_FWD;
            if (STACK_FULL()) {
                return FAILURE;
            }
            STACK_PUSH(pc);
            break;
        case ']':
            if (STACK_EMPTY()) {
                return FAILURE;
            }
            jmp_pc = STACK_POP();
            PROGRAM[pc].operator= OP_JMP_BCK;
            PROGRAM[pc].operand = jmp_pc;
            PROGRAM[jmp_pc].operand = pc;
            break;
        default:
            pc--;
            break;
        }
        pc++;
    }
    if (!STACK_EMPTY() || pc == PROGRAM_SIZE) {
        return FAILURE;
    }
    PROGRAM[pc].operator= OP_END;
    return SUCCESS;
}

int execute_bf() {
    unsigned short data[DATA_SIZE], pc = 0;
    unsigned int ptr = DATA_SIZE;
    while (--ptr) {
        data[ptr] = 0;
    }
    while (PROGRAM[pc].operator!= OP_END && ptr<DATA_SIZE) {
        switch (PROGRAM[pc].operator) {
        case OP_INC_DP:
            ptr++;
            break;
        case OP_DEC_DP:
            ptr--;
            break;
        case OP_INC_VAL:
            data[ptr]++;
            break;
        case OP_DEC_VAL:
            data[ptr]--;
            break;
        case OP_OUT:
            putchar(data[ptr]);
            break;
        case OP_IN:
            data[ptr] = (unsigned int)getchar();
            break;
        case OP_JMP_FWD:
            if (!data[ptr]) {
                pc = PROGRAM[pc].operand;
            }
            break;
        case OP_JMP_BCK:
            if (data[ptr]) {
                pc = PROGRAM[pc].operand;
            }
            break;
        default:
            return FAILURE;
        }
        pc++;
    }
    return ptr != DATA_SIZE ? SUCCESS : FAILURE;
}

int main(int argc, const char *argv[]) {
    int status;
    FILE *fp;
    if (argc != 2 || (fp = fopen(argv[1], "r")) == NULL) {
        fprintf(stderr, "Usage: %s filename\n", argv[0]);
        return FAILURE;
    }
    status = compile_bf(fp);
    fclose(fp);
    if (status == SUCCESS) {
        status = execute_bf();
    }
    if (status == FAILURE) {
        fprintf(stderr, "Error!\n");
    }
    return status;
}

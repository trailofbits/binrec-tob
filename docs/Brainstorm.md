# Motivation

Existing static binary lifting tools rely on many heuristics and assumptions. Thus, they do not correctly handle any code that breaks these assumptions; they cannot recover information beyond what can be statically known. Targets of indirect branches, inline assembly, position-independent code, and the distinction between code and data are areas where today's binary lifting tools do not correctly handle and/or heavily rely on assumptions and heuristics. We propose a novel technique which does not rely on assumptions and heuristics to lift and recover stripped binaries. We do this by lifting from program's dynamic traces, not only from the static program binary. Since we lift from actually executed traces, there is no false positive in the recovered binary. *Raw dynamic traces contain interactions with virtualization environment. Lifting these raw traces will include a giant superfluous code bases, bloating the code sizes and slowing down the program execution. To handle this, we introduce a technique to strip only essential part of the traces, eliminating the interactions to virtual environment.(FIXME: not sure about this part)* Another particular challenge to "dynamic lifting" is to improve code coverage. Since we only lift from execution traces, code paths that are not triggered by the tested input will not be recovered. To overcome this challenge, we propose a novel technique that stitches multiple traces together into a single executable binary. This way, the recovered binary can achieve both soundness and completeness when it is provided by sufficient inputs. We provide quality inputs driven by a fuzzer (and our evaluation shows that reaches 100% coverage of programs within a reasonable time, and this could be improved by fuzzing performance which we see as an orthogonal program). Our solution is faster than previous static lifting tools because (...). Our evaluation shows that the proposed solution correctly recovers PIC, inline assembly, obfuscated code, function pointer arguments, and indirect control transfers without any assumptions and heuristics.

## What do we miss now?

As a binary lifting tool, we should be able to show a compiler analysis as an application. Basically, we should be able to show what lifters can do, but rewriters cannot. This may not be easy since we don't recover local/global variables and functions yet.

# Interesting Cases

## Function pointer arguments (and callbacks)
### Examples
* An example from "Superset Disassembly: Statically Rewriting x86 Binaries Without Heuristics". If we move the location of the `gt` but do not modify this address passed to `qsort`, it will wrong the wrong code.
```c
int qt(void *, void *);

void model1(void) {
    qsort(array, 5, sizeof(char*), gt);
}
```
* A vtable example: This example shows that Binrec can handle function pointers passed to local functions without jumping to original binary. So if a code pointer from data section is passed to local function, there is no problem.
```c
//#include <memory>
//#include <string>
#include <iostream>

void execute(){
    puts("Executed");
}

void executer(void (*ex)()){
    ex();
}

struct Person {
protected:
    unsigned age;
public:
    Person( unsigned old ) : age( old ) {
        puts("Person created");
    }

    virtual ~Person() {
        puts("Person destroyed");
    }

    virtual void shout() = 0;

    void incAge() {
       ++age;
       puts("age incremented");
    }
};

struct Calm : Person {
    Calm( unsigned age ) : Person( age + 10 ) {
        puts("Calm person created");
    }

    virtual ~Calm() {
        puts("Calm person destroyed");
    }

    void shout() override {
        puts("I am calm, can't shout");
    }

};



int main() {
    Person *p = new Calm(27);
    p->shout();
    executer(execute);
    delete p;
    return 0;
}
```
#### Examples for making system calls, these will be useful I think especially if we lift library code
* Make a system call using library function
```c
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void sig_handler(int sig) {
    switch (sig) {
    case SIGSEGV:
        fprintf(stderr, "dumping everything..\n");
        exit(1);
    default:
        fprintf(stderr, "something caused a signal\n");
        exit(2);
    }
}

int main() {
    signal(SIGSEGV, sig_handler);

    int *i_ptr = NULL;
    //trigger SIGSEGV
    return *i_ptr;
}
```
* Make a system call using interrupt
```c
include <stdio.h>
#include <stdlib.h>
#include <signal.h>
//#include <unistd_32.h>

#define __NR_signal 48
typedef void (*signal_handler)(int);

//Implement signal system call with interrupt
signal_handler reg_signal(int signum, signal_handler handler) {
    signal_handler ret;
    __asm__ __volatile__
    (   
        "int $0x80"
        : "=a" (ret)
        : "0"(__NR_signal), "b"(signum), "c"(handler)
        : "cc", "edi", "esi", "memory"
    );
    return ret;
}

void sig_handler(int sig) {
    switch (sig) {
    case SIGSEGV:
        fprintf(stderr, "dumping everything..\n");
        exit(1);
    default:
        fprintf(stderr, "something caused a signal\n");
        exit(2);
    }
}

int main() {
    reg_signal(SIGSEGV, sig_handler);

    int *i_ptr = NULL;
    //trigger SIGSEGV
    return *i_ptr;
}
```
* Make a system call using vdso which uses sysenter(ring 3 to 0) and sysexit(ring 0 to 3) system calls
```c
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
//#include <unistd_32.h>

#define __NR_signal 48
typedef void (*signal_handler)(int);

//Use vdso to register a system call by calling __kernel_vsyscall.
//Address of __kernel_vsyscall is passed to user process with AT_SYSINFO elf parameter
//We make a call to %%gs:0x10 because when libc is loaded it copies the value of 
//AT_SYSINFO from the process stack to the TCB (Thread Control Block). Segment register
//%gs refers to the TCB. Offset 0x10 can be different in different systems.
signal_handler reg_signal(int signum, signal_handler handler) {
    signal_handler ret;
    __asm__ __volatile__
    (   
        "call *%%gs:0x10"
        : "=a" (ret)
        : "0"(__NR_signal), "b"(signum), "c"(handler)
        : "cc", "edi", "esi", "memory"
    );
    return ret;
}

void sig_handler(int sig) {
    switch (sig) {
    case SIGSEGV:
        fprintf(stderr, "dumping everything..\n");
        exit(1);
    default:
        fprintf(stderr, "something caused a signal\n");
        exit(2);
    }
}

int main() {
    reg_signal(SIGSEGV, sig_handler);

    int *i_ptr = NULL;
    //trigger SIGSEGV
    return *i_ptr;
}
```
* omnetpp (Put the relevant code here.)

### Solutions

1. To identify each function that uses function pointer arguments in external libraries and patch the function pointer address to be the new address (Many prioir rewriters including Stir, Reins, and SecondWrite take this approach.)
2. To create a map between old addresses and the corresponding new addresses, and rewrite every indirect control transfer to look up the new address, using the old address as a key ("Superset Disassembly" takes this approach).
3. Our solution? BinRec naturally does the solution 2 as long as the portion of binary is traced.

## Position-independent code

## Tricky inline assembly code

## Code and data distinction

## Obfuscated code

## Randomness
* Perlbench: a small part of control-flow depends on a random value.
```c
// Example from perl.c line:1207
// in the example, the value of argv[0] is random.
UV aligned = (mask < ~(UV)0) && ((PTR2UV(argv[0]) & mask) == PTR2UV(argv[0]));
```



# Comparision with other binary lifting (or rewriting) tools

* [McSema's comparison with other machine code to LLVM bitcode lifters](https://github.com/trailofbits/mcsema/blob/master/README.md#comparison-with-other-machine-code-to-llvm-bitcode-lifters)
* Superset Disassembly: this constructs literally a superset of legal disassembly, which means the disassembled code also contains illegal instructions and control/data flows. It's just constructed in a way that the illegal paths are naturally ignored when the rewritten binary executes. This, however, may not be desirable for binary lifting approaches whose goal is to generate high-level intermediate representation from the binary and to apply sophisticated compiler analysis and optimizations.

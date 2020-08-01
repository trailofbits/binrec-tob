BIN     := ab ab2 aorb hello fib args pwcheck pwcheck-vm fileptr sha2 simple \
	bf sha2-float floattest atoftest strtoftest eq-vm add-vm eq-vm-2 \
	round eq fibit fibit-vm matmul matmul-vm longjmp siglongjmp fptr qsort heap2 \
	aiethel/complex_float aiethel/complex_double aiethel/complex_numbers \
	aiethel/complex_long_double aiethel/qsort_int_array aiethel/signal_handler \
	aiethel/sigabort aiethel/sigalarm aiethel/sigsegv aiethel/sigsegv_intx80 aiethel/sigsegv_vdso \
	consecutive_calls abs eq2 aiethel/qsort

CPPBIN	:= aiethel/virtual aiethel/qsort_function_ptrs aiethel/operator_new aiethel/iostream_basics \
	aiethel/inheritance simple_class heapover uaf uas globaloob stackoob
SOURCES := $(addsuffix .c,$(BIN)) Makefile *.h
CPPSOURCES := $(addsuffix .cpp,$(CPPBIN)) Makefile *.h
WINBIN  := $(addsuffix .exe,ab ab2 aorb hello fib args fileptr)
TESTDIR := test
CFLAGS  := -m32 -Wall -Wextra -g -std=c99 -fno-stack-protector -march=pentium
CPPFLAGS:= -m32 -Wall -Wextra -g -std=c++11 -fno-stack-protector
CLANG   := ${S2EDIR}/build/llvm-native/Release/bin/clang
CLANGPP   := ${S2EDIR}/build/llvm-native/Release/bin/clang++
MYOPT   := ${S2EDIR}/build/llvm-release/Release+Asserts/bin/opt \
	-load ${S2EDIR}/build/tran-passes-release/translator/translator.so

.PHONY: all win upload clean

all: $(BIN) $(CPPBIN)
cpp: $(CPPBIN)
win:  $(WINBIN)

simple_class: CPPFLAGS += -O3 -g0
sha2 sha2-float: CFLAGS += -O3
#aiethel/qsort sha2 sha2-float: CFLAGS += -O3
add-vm: vm.c vm.h
matmul-vm: CC = $(CLANG)
eq-vm eq-vm-2 eq-vm.ll fibit-vm matmul-vm matmul-vm.ll: vm.c vm.h
eq-vm eq-vm-2 eq-vm.ll fibit-vm matmul-vm matmul-vm.ll: CFLAGS += -O1
aiethel/complex_float: LDFLAGS += -lm
aiethel/complex_long_double: LDFLAGS += -lm
aiethel/complex_double: LDFLAGS += -lm
aiethel/complex_numbers: LDFLAGS += -lm

%: %.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
%: %.cpp
	g++ $(CPPFLAGS) -o $@ $< $(LDFLAGS)

%: %.cpp
	$(CLANGPP) $(CPPFLAGS) -o $@ $< $(LDFLAGS)

%.exe: CFLAGS+=-I./cvdemo/Include/C -D_WIN32
%.exe: %.c
	i586-mingw32msvc-gcc $(CFLAGS) -Wl,-Map,$(@:.exe=.map) -o $@ $<
	@chmod -x $@

%.ll: %.c
	$(CLANG) -S -emit-llvm $(CFLAGS) -g0 -o $@ $<

%.gv: %.ll
	$(MYOPT) -disable-output -my-dot-cfg -cfg-outfile $@ -cfg-show-code $<

simple: simple.o
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)
simple.o: simple.asm
	nasm -f elf32 -o $@ $<

upload: $(SOURCES)
	@mkdir $(TESTDIR)
	@cp -t $(TESTDIR) $^
	scp -P 1234 -r $(TESTDIR) dev@localhost:
	@rm -rf $(TESTDIR)

cleanCpp:
	rm -rf $(BINCPP)
clean:
	rm -rf $(BIN) $(CPPBIN) $(WINBIN) 

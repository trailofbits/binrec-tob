LLVMBIN := ${S2EDIR}/multicompiler/bin
LLC := $(LLVMBIN)/llc
LLVM_DIS := $(LLVMBIN)/llvm-dis
CLANG := ${LLVMBIN}/clang
BIN := custom-helpers.bc replacemain.so op_helper.bc softfloat.bc replaceFuncs.o
SRC := softfloat.ll op_helper.ll custom-helpers.ll


#LLVMBIN := ${S2EDIR}/build/llvm-release/Release+Asserts/bin
#LLC := $(LLVMBIN)/llc
#LLVM_DIS := $(LLVMBIN)/llvm-dis
#CLANG := ${S2EDIR}/build/llvm-native/Release/bin/clang
#BIN := custom-helpers.bc replacemain.so op_helper.bc softfloat.bc
#SRC := softfloat.ll op_helper.ll custom-helpers.ll

INCLUDE_DIRS := \
		-I../build/qemu-release \
		-I../build/qemu-release/i386-s2e-softmmu \
		-I../build/qemu-release/linux-headers \
		-I../s2e/qemu \
		-I../s2e/qemu/fpu \
		-I../s2e/qemu/include \
		-I../s2e/qemu/linux-headers \
		-I../s2e/qemu/slirp \
		-I../s2e/qemu/target-i386 \
		-I../s2e/qemu/tcg \
		-I../s2e/qemu/tcg/i386 \
		-I/usr/include/glib-2.0 \
		-I/usr/include/libpng12 \
		-I/usr/include/p11-kit-1 \
		-I/usr/lib/x86_64-linux-gnu/glib-2.0/include \
		-Werror -fPIE -DPIE -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 \
		-D_LARGEFILE_SOURCE -Wstrict-prototypes -Wredundant-decls -Wall -Wundef \
		-Wwrite-strings -fno-strict-aliasing \
		-I/usr/include/libpng12 -Wno-initializer-overrides -DHAS_AUDIO \
		-DHAS_AUDIO_CHOICE -Wno-initializer-overrides \
		-DTARGET_PHYS_ADDR_BITS=64 -DNEED_CPU_H -pthread -Wno-unused-function \
		-march=native

.PHONY: all
all: $(BIN)

#mutator:
#	$(MAKE) -C dyninst_mutator

.PHONY: src
src: $(SRC)

%.o: %.bc
	$(LLC) $(CFLAGS) -filetype obj -o $@ $<
	#$(CLANG) -m32 -c -o $@ $<

%.s: %.bc
	$(LLC) -filetype asm -o $@ $<
	#$(CLANG) -S -m32 -c -o $@ $<

%.ll: %.bc
	$(LLVM_DIS) -o $@ $<

%.so: %.c
	$(CC) -fPIC -shared -m32 $(CFLAGS) -o $@ $< $(LDFLAGS)

%.bc: %.c
	$(CLANG) -c -emit-llvm -m32 $(CFLAGS) -o $@ $<

softfloat.bc: ../s2e/qemu/fpu/softfloat.c
	$(CLANG) -m64 -c -emit-llvm -I../build/qemu-release/i386-s2e-softmmu/ \
		-I../build/qemu-release/ -I../s2e/qemu -O0 -o $@ $<

#softfloat.bc: ../s2e/qemu/fpu/softfloat.c
#	$(CLANG) -c -emit-llvm -I../build/qemu-release/i386-s2e-softmmu/ \
#		-I../build/qemu-release/ -I../s2e/qemu -O0 -o $@ $<
		#-fpack-struct=1

op_helper.bc: ../s2e/qemu/target-i386/op_helper.c
	$(CLANG) -m64 -c -emit-llvm $(INCLUDE_DIRS) -DS2E_LLVM_LIB -g -o $@ $<
		# -fpack-struct=1

clean:
	rm -f $(BIN) $(SRC) dyninst_mutator/*.o *.o *.s

custom-helpers.bc: mysyscall.h
custom-helpers.bc: CFLAGS=$(INCLUDE_DIRS) -O3 -fno-builtin -m32
#replacemain.so: LDFLAGS=-ldl
replaceFuncs.o: CFLAGS=-m32 -fomit-frame-pointer

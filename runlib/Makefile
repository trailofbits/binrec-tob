LLC := llc
LLVM_DIS := llvm-dis
CLANG := clang
CLANGXX := clang++
BIN := custom-helpers.bc replacemain.so op_helper.bc replaceFuncs.o
SRC := op_helper.ll custom-helpers.ll
QEMU_BUILD=../build/s2e/qemu
 # TODO (hbrodin): is this the correct path for config-host.h:
 # ../s2e/build/libs2e-release/i386-s2e-softmmu/libcpu/include
#INCLUDE_DIRS := \
#		-I../binrec_rt/include \
#		-I$(QEMU_BUILD) \
#		-I$(QEMU_BUILD)/i386-s2e-softmmu \
#		-I../s2e/source/s2e/libcpu/include \
#		-I../s2e/source/s2e/libcpu/include/fpu \
#		-I../s2e/source/s2e/libcpu/include/i386 \
#		-I../s2e/source/s2e/libtcg/include \
#		-I../s2e/source/s2e/libq/include \
#		-I../s2e/build/libs2e-debug/i386-s2e-softmmu/libcpu/include \
#		-I../s2e/build/libs2e-debug/i386-s2e-softmmu/libcpu/include \
#		-I../s2e/qemu \
#		-I../s2e/qemu/fpu \
#		-I../s2e/qemu/target-i386 \
#		-I/usr/include/glib-2.0 \
#		-I/usr/lib/x86_64-linux-gnu/glib-2.0/include \
#		-fPIE -DPIE -D_GNU_SOURCE -D_FILE_OFFSET_BITS=64 \
#		-D_LARGEFILE_SOURCE -Wstrict-prototypes -Wredundant-decls -Wall -Wundef \
#		-Wwrite-strings -fno-strict-aliasing \
#		-I/usr/include/libpng12 -Wno-initializer-overrides -DHAS_AUDIO \
#		-DHAS_AUDIO_CHOICE -Wno-initializer-overrides \
#		-DTARGET_PHYS_ADDR_BITS=64 -DNEED_CPU_H -pthread -Wno-unused-function \
#		-march=native

S2ESRC := ../s2e/source/s2e
S2EGENSRC := ../s2e/build/libs2e-release/i386-s2e-softmmu

INCLUDE_DIRS := \
	-I${S2ESRC}/libcpu/include/cpu/i386 \
	-I${S2ESRC}/libtcg/include \
	-I${S2ESRC}/libcpu/include \
	-I${S2ESRC}/libcpu/include/fpu \
	-I${S2EGENSRC}/libcpu/include \
	-I${S2ESRC}/libcpu/src \
	-I${S2ESRC}/libq/include \
	-I../binrec_rt/include \
	-I/usr/include/glib-2.0 \
	-I/usr/lib/x86_64-linux-gnu/glib-2.0/include \
	-DNEED_CPU_H

#-I../s2e/source/s2e/libcpu/src \
.PHONY: all
all: $(BIN)

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

# TODO (hbrodin): Find better way of referencing the op_helper.c file.
op_helper.bc: ${S2ESRC}/libcpu/src/target-i386/op_helper.c
	$(CLANG) -m64 -c -emit-llvm $(INCLUDE_DIRS) -DS2E_LLVM_LIB -g -o $@ $<
		# -fpack-struct=1

clean:
	rm -f $(BIN) $(SRC) *.o *.s

custom-helpers.bc: custom-helpers.cpp
	$(CLANGXX) -std=c++17 -fno-rtti -fno-exceptions -c -emit-llvm -m32 $(INCLUDE_DIRS) -O3 -fno-builtin -o $@ $<

#replacemain.so: LDFLAGS=-ldl
replaceFuncs.o: CFLAGS=-m32 -fomit-frame-pointer

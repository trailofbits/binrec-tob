# This statement is copied from the S2E Makefile, it is horrible and should be
# destroyed with fire at some point
ifeq ("",$(wildcard /usr/include/x86_64-linux-gnu/c++/4.8/c++config.h))
	export CPLUS_INCLUDE_PATH=/usr/include:/usr/include/x86_64-linux-gnu:/usr/include/x86_64-linux-gnu/c++/4.8:/usr/include/c++/4.8
	export C_INCLUDE_PATH=/usr/include:/usr/include/x86_64-linux-gnu
endif

ifeq ("",$(wildcard /usr/include/i386-linux-gnu/c++/4.8/c++config.h))
	export CPLUS_INCLUDE_PATH=/usr/include:/usr/include/i386-linux-gnu:/usr/include/i386-linux-gnu/c++/4.8:/usr/include/c++/4.8
	export C_INCLUDE_PATH=/usr/include:/usr/include/i386-linux-gnu
endif

.PHONY: all dir clean cleaner cleanest tests

all: stamp-qemu-release-make passes dir/runlib dir/test

dir/%: %
	make -C $*

.PHONY: passes
passes: dir/build/tran-passes-release

build/stamps/qemu-%-quick: build/stamps/qemu-%-make
	touch $@

quick-%: build build/stamps/qemu-%-quick
	make -C build/qemu-$*/i386-s2e-softmmu

.PHONY: qemu-full
qemu-full: qemu-release qemu-debug

qemu-%: stamp-qemu-%-make

tools: stamp-tools-release-make

stamp-%: build
	make -C $< -f ../s2e/Makefile stamps/$*

.PHONY: build build_i386
build:
	@if [ "${S2EDIR}" = "" ]; then \
		echo "Error: environment variable S2EDIR not set"; \
        exit 1; \
    fi
	mkdir -p $@ && \
	cd $@ && \
	make -f ../s2e/Makefile

build_i386:
	@if [ "${S2EDIR}" = "" ]; then \
		echo "Error: environment variable S2EDIR not set"; \
        exit 1; \
    fi
	mkdir -p $@ && \
	cd $@ && \
	make -f ../s2e/Makefile_i386 && \
	make -f ../s2e/Makefile_i386 stamps/llvm-cmake-configure && \
	make -f ../s2e/Makefile_i386 stamps/llvm-cmake-make && \
	make -C llvm-release install && \
	make -C llvm-cmake install && \
	make -f ../s2e/Makefile_i386 stamps/tran-passes-release-configure && \
	make -C tran-passes-release/translator
	echo "Done"	

build/tran-passes-release:
	cd $(@D) && \
	make -f ../s2e/Makefile stamps/llvm-cmake-configure && \
	make -f ../s2e/Makefile stamps/llvm-cmake-make && \
	make -C llvm-release install && \
	make -C llvm-cmake install && \
	make -f ../s2e/Makefile stamps/tran-passes-release-configure

clean:

cleaner: clean
	rm -f *.svg *.dot

cleanest: cleaner
	rm -rf build
	make -f runlib/Makefile clean
	make -f test/Makefile clean

# dummy target to enable auto-completion for clean-% target
$(patsubst ./s2e-out-%,clean-%,$(shell find -maxdepth 1 -name 's2e-out-*' -type l)):

clean-%: s2e-out-%
	[ -e s2e-last -a "`readlink s2e-last`" = "`readlink $<`" ] && rm s2e-last
	rm -rf `readlink $<` $<

%coverage-full.txt: %ExecutionTracer.dat
	bash scripts/coverage.sh $(@D)

# phony target for quick remake
%.bblist: %
	idaq -A -OIDAPython:s2e/tools/tools/scripts/ida/extractBasicBlocks.py $< || true

%.excl: %.bblist
	echo _start frame_dummy __gmon_start__ .init_proc .__libc_start_main \
		.__gmon_start__ register_tm_clones deregister_tm_clones \
		__x86.get_pc_thunk.bx __do_global_dtors_aux \
		| tr ' ' '\n' > $@
	cut -d' ' -f3 $< | uniq | grep '@@GLIBC\|^\.' \
		| sed 's/^\(.\+\)@@GLIBC/\1\n\0/' >> $@

README.html: README.md
	markdown $< > $@

.DELETE_ON_ERROR:

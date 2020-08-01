LIBC := /lib/i386-linux-gnu/libc.so.6

all: gettypes argsizes

gettypes: CFLAGS=-m32

fnames: $(LIBC)
	nm -D $< | grep -P '[0-9a-f]{8} [tiTW] ' | cut -d' ' -f3 > $@

# TODO: make this grep multiline, manpages with multiline function signatures
# are ignored now
fsigs: fnames knownsigs
	sh getsigs.sh < $< | cat - knownsigs > $@

argsizes: fsigs parse-fsigs.py
	python parse-fsigs.py $< | grep -v error: > $@

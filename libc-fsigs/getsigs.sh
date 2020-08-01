#!/bin/bash
i=0

for f in `cat`
do
    man $f 2>/dev/null | \
    grep -o "^       [^ ].* \*\?$f *(.*);\( */\*.*\*/\)\?$" | head -1 | \
    grep -v Unused | sed 's/^ \+//'
    i=$(($i + 1))
    printf "Processed %d functions\r" $i >&2
done

echo >&2

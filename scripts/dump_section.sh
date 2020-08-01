#!/bin/bash
[ $# -eq 2 ] || { echo "usage: bash $0 FILE SECTION"; exit 1; }
file=$1
section=$2
pattern=$(sed 's/\./\\./g' <<< $section)
out=$(readelf -SW $file | sed "s/^.\+ $pattern \+[^ ]\+ \+\(.*\)$/\1/;t;d")
[ "$out" = "" ] && { echo "dump section: no such section" >&2; exit 2; }
offset=$(printf %d 0x$(cut -d' ' -f2 <<< "$out"))
size=$(printf %d 0x$(cut -d' ' -f3 <<< "$out"))
exec dd if=$file ibs=1 skip=$offset count=$size 2>/dev/null

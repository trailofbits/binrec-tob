failwith() {
    echo $* >&2
    exit 1
}
section_line() {
    pat=$(sed 's/\./\\./g' <<< $2)
    readelf -S $1 | sed "s/^.\+ $pat \+[^ ]\+ \+\(.*\)$/\1/;t;d"
}
section_field() {
    section_line $1 $2 | cut -d' ' -f$3
}
section_vaddr() {
    section_field $1 $2 1
}
section_offset() {
    section_field $1 $2 2
}
section_size() {
    section_field $1 $2 3
}
hex2dec() {
    printf %d 0x$1
}

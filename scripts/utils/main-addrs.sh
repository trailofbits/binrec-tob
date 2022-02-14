# Helper Script to print the main entry and exit points for an input binary.

#!/usr/bin/env bash
fn() {
    d=`objdump -d "${@:2}"`
    start=`grep -n "<$1>:" <<< "$d" | cut -d: -f1`
    lastlabel=`grep -n "<$1\(\..\+\)\?>:" <<< "$d" | tail -1 | cut -d: -f1`
    len=`tail -n +$(($lastlabel + 1)) <<< "$d" | grep -n '^$' | head -n1 | sed s/://`
    len=$(($len + $lastlabel - $start))
    tail -n +$start <<< "$d" | head -n $len
}

main-entry() {
    nm $1 | sed 's/^0*\([0-9a-f]\+\) T main$/\1/g;t;d'
}

main-exits() {
    returns=`fn main $1 | grep '\<ret *$' | cut -d: -f1`
    exit_stub=`objdump -d $1 | sed 's/^\([0-9a-f]\+\) <exit@plt>:$/\1/;t;d'`
    echo $returns $exit_stub | sed 's/\<0\+//g'
}

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

[ $# -ge 1 ] || { echo "usage: bash $0 BINARY"; exit 1; }
bin=$1
#log=$2
#python $DIR/cfg/main-addrs.py `main-entry $bin` `main-exits $bin` < $log
echo `main-entry $bin` e
for x in `main-exits $bin`; do echo $x x; done

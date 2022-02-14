#!/usr/bin/env bash
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG=qemu/cmd-config

usage() {
    echo "usage: bash $0 [ OPTIONS ] BIN [ ARGS... ]"
    echo
    echo "Run binary at PATH in a debian VM (requires the \"cmd\" snapshot)."
    echo "PATH should be inside test/ or test/coreutils."
    echo
    echo "OPTIONS:"
    echo "  -h,--help           Show this help message"
    echo "  --vnc               Host a VNC server as display on port 5900"
    echo "  --stdin DATA        Pass DATA on stdin"
    echo "  --sym-stdin NBYTES  Pass NBYTES symbolic bytes on stdin"
}

stdin=
sym_stdin=0

#while :
#do
#    case "$1" in
#    -h|--help)
#        usage
#        exit 0
#        ;;
#    --vnc)
#	re='^[0-9]+$'
#	if [[ $2 =~ $re ]]; then
#	    qemu_args="-vnc :$2"
#	    shift
#	else
#			qemu_args="-vnc :0"
#	fi
#        ;;
#    --stdin)
#        stdin=$2
#        sym_stdin=0
#        shift
#        ;;
#    --sym-stdin)
#        stdin=
#        sym_stdin=$2
#        shift
#        ;;
#    *)
#        break
#        ;;
#    esac
#    shift
#done
[ $# -ge 1 ] || { usage; exit 1; }
path=$3
args="${@:4}"

cd $DIR/..

bin=`basename $path`
#printf -- "$bin\n$args\n$stdin\n$sym_stdin\n" > $CONFIG
set -a
cmd_config="$bin\n$args\n$stdin\n$sym_stdin\n"
bash qemu/selectbbs.sh cmd $bin $1 $2
#bash qemu/run-vm.sh debian -sym cmd $bin $qemu_args
set +a

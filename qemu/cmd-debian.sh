#!/usr/bin/env bash
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
CONFIG=qemu/cmd-config

usage() {
    echo "usage: bash $0 [ OPTIONS ] BIN [ ARGS... ]"
    echo
    echo "Run binary at PATH in a debian VM (requires the \"cmd\" snapshot)."
    echo "PATH root should be specificed in config.debian.lua"
    echo
    echo "OPTIONS:"
    echo "  -h,--help           Show this help message"
    echo "  --vnc <N>           Host a VNC server as display on port 5900+N"
    echo "  --net               Attach tap based network interface"
    echo "  --stdin DATA        Pass DATA on stdin"
    echo "  --sym-stdin NBYTES  Pass NBYTES symbolic bytes on stdin"
    echo "  --sym-arg <N>       Replace by a symbolic argument of length N"
}

stdin=
sym_stdin=0
qemu_args=""

while :
do
    case "$1" in
    -h|--help)
        usage
        exit 0
        ;;
    --vnc)
	re='^[0-9]+$'
	if [[ $2 =~ $re ]]; then
			let "vncport=5900+$2"
			i=$2
			netstat -ntpl | grep [0-9]:$vncport -q;
			if [ $? -ne 1 ]; then
				echo "port conflict! we assign it to 100"
				let "i=100"
			fi
			echo $i
	    qemu_args=" -vnc :$i $qemu_args "
	    shift
	else
			for i in {0..40};
			do
				let "vncport=5900+i"
				echo "VNC PORT is $vncport"
				netstat -ntpl | grep [0-9]:$vncport -q;
				if [ $? -eq 1 ]; then
					break
				fi
			done 
            qemu_args=" -vnc :$i $qemu_args "
	fi
        ;;
    --net)
        net=" -net "
        ;;
    --stdin)
        stdin=$2
        sym_stdin=0
        shift
        ;;
    --sym-stdin)
        stdin=
        sym_stdin=$2
        shift
        ;;
    *)
        break
        ;;
    esac
    shift
done
#[ $# -ge 1  ] || { usage; exit 1; }
path=$1
args="${@:2}"

cd $DIR/..

#export MEM_AMOUNT=64
bin=`basename $path`
#printf -- "$bin\n$args\n$stdin\n$sym_stdin\n" > $CONFIG
#cp qemu/cmd-config s2e-last/
set -a
cmd_config="$bin\n$args\n$stdin\n$sym_stdin\n"
instance_id=$i
bash qemu/run-vm.sh debian -sym cmd $bin $net $qemu_args
set +a


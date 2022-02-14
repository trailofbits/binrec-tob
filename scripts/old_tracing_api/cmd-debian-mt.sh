#!/usr/bin/env bash
# USAGE: ./qemu/cmd-debian-mt.sh config display_port_to_start
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT=`realpath $DIR/..`

usage() {
	echo "usage: bash $0 CONFIG_FILENAME"
}
[ $# -ge 1 ] || { usage; exit 1;}

if [ "$DEBUG" = x ]; then release=debug; else release=release; fi

filename=$1
set -a
PARALLEL_LIFTING=true
i=0

if [[ "$#" -ge 2 ]]; then
    re='^[0-9]+$'
    if [[ $2 =~ $re ]]; then
        echo "Starting display port from $2"
        i=$2
    else
        echo "Port needs to be an integer"
    fi
fi

while read -r line; do
        args="$line"
        cur_tap="tap$i"
        tap_exists="$(ip tuntap show | grep $cur_tap)"
        if [[ $tap_exists == "" ]]; then
            echo "First use of $cur_tap, making tap."
            sudo ip tuntap add dev $cur_tap mode tap user $(whoami)
            sudo ip link set $cur_tap master br0
            sudo ip link set dev $cur_tap up
        fi
#   bash ./qemu/cmd-debian.sh --vnc $i $args &
    bash ./qemu/cmd-debian.sh --vnc $i --net $args & #put like this to use network
    let "i=i+1"
    sleep 2 #give some time to create folders for previous run
done < "$filename"
wait
echo "$filename 3"
set +a

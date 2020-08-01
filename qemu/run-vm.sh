#!/usr/bin/env bash
DEFAULT_MEM_AMOUNT=1024

usage() {
    echo "usage: bash $0 [-h] IMAGE_BASE [ OPTIONS ] [ ARGS... ]"
    echo
    echo "IMAGE_BASE:"
    echo "\"debian\" or \"winxp\""
    echo
    echo "OPTIONS:"
    echo "-h              show this help message"
    echo "-sym STATE [ID] run in symbolic (DBT) mode from snapshot STATE,"
    echo "                creating output dir s2e-out-ID (ID defaults to STATE)"
    echo "-raw            run in KVM raw mode with network"
    echo "                use to make changes to the base image"
    echo "-net            attach tap based network interface"
    echo "                use to get files into the vm"
    echo "-enable-kvm     run in KVM DBT mode without network"
    echo "                use to quickly boot the VM and create a KVM snapshot"
    echo "<default>       run in DBT mode without network"
    echo "                use to create snapshots for -sym with s2eget/getrun"
    echo
    echo "Any additional ARGS... are passed directly to qemu-system-i386:"
}

if [ $# -lt 1 -o "$1" = -h ]; then usage; exit 0; fi
image_base=$1
mode=dbt

DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
QEMU=$DIR/../build/qemu-release/i386-softmmu/qemu-system-i386
base_args="-cpu pentium -smp 1 -name $image_base -m ${MEM_AMOUNT-$DEFAULT_MEM_AMOUNT}"
macaddress=$(printf "52:55:00:d1:55:%02g" $instance_id)
#echo $macaddress
sym_net_args="-device e1000,netdev=net0,mac=$macaddress \
		-netdev tap,id=net0,ifname=tap${instance_id},script=no,downscript=no"
#echo "Qemu RAM: ${MEM_AMOUNT-$DEFAULT_MEM_AMOUNT}"
old_net_args=" -net nic -net user,vlan=0,hostfwd=tcp::1234-:22"

shift #remove image name
while :
do
    case "$1" in
    -h)
        usage
        exit 0
        ;;
    -sym)
        if [ $# -lt 3 ]; then usage; exit 1; fi
        mode=symbolic
        state=$2
        id=$3
        #if [ $# -ge 4 ]; then
            #id=$4
        shift 3
        #else
            #id=$state //this now depends heavily on cmd-debian
            #shift 2
        #fi
        ;;
    -raw)
        mode=raw
        shift
        ;;
    -net)
        base_args="$base_args $sym_net_args"
        shift
        ;;
    -old_net)
        mode=old_net
        shift
        ;;
    *)
        break
        ;;
    esac
done

additional_args="$@"

echo "QEMU IMPLICIT ARGS = $base_args"
set -e

case $mode in
dbt)
    image=$DIR/${image_base}.s2e
    exec $QEMU $image $base_args -net none $additional_args
    ;;
raw)
    image=$DIR/${image_base}.raw
    if [ "${image_base}" = debian ]; then
        curses_args="-nographic -curses -monitor telnet:localhost:1235,server,nowait"
    fi
    exec $QEMU $image $base_args $net_args $curses_args $additional_args
    ;;
old_net)
    image=$DIR/${image_base}.s2e
    exec $QEMU $image $base_args $old_net_args $additional_args
    ;;
symbolic)
    image=$DIR/${image_base}.s2e
    #config_file=$DIR/../plugins/config.${image_base}.lua
    config_file=config.${image_base}.lua
    exec bash $DIR/symbolic.sh $image $state $config_file $id \
        $base_args $additional_args
    ;;
esac
set +e
# After boot:
# SSH: ssh -p 1234 dev@localhost
# QEMU console: telnet localhost 1235

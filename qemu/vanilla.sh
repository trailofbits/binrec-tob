#!/usr/bin/env bash
if [ $# -lt 1 ]
then
    >&2 echo "usage: bash $0 IMAGE ARGS..."
    exit 1
fi
DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
exec $DIR/../build/qemu-release/i386-softmmu/qemu-system-i386 "$@"

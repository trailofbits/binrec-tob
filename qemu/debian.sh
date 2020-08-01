#!/usr/bin/env bash
DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
export instance_id=0
$DIR/run-vm.sh debian "$@" 

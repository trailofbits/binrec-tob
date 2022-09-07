#!/bin/bash

echo client: sleeping for 10 seconds
SLEEP=$(which sleep)
echo which sleep: $SLEEP - $?
pwd
sleep 10s
echo client: waking up $?
./input_files/client
echo client: done

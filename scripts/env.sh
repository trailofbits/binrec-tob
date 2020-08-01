#!bin/bash
export S2EDIR=`pwd`
export COMPOSE_PROJECT_NAME="$USER_$S2EDIR"
export VNCPORT=$((`id -u $USER` * 10 + 7900 + ${#S2EDIR}))
echo Use port: $VNCPORT for your binrec vnc session

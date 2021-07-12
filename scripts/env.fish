set -x S2EDIR (pwd)
set -x COMPOSE_PROJECT_NAME {$USER}_{$S2EDIR}
set -x VNCPORT (math (id -u) \* 10 + 7900 + (string length $S2EDIR))
echo Use port: $VNCPORT for your binrec vnc session

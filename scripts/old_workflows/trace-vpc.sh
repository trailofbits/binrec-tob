#/bin/bash
"$@" 2>&1 >/dev/null | grep ^vpc: | cut -d: -f2

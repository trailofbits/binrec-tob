#!/bin/sh
PNGFILE=last.png
VIEW=${IMAGE_VIEWER-eog}
rm -f $PNGFILE
python cfg.py $* | dot -T png -o $PNGFILE && $VIEW $PNGFILE 2>/dev/null

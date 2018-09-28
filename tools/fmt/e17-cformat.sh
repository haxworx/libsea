#!/bin/sh
ecrustify -l C -c fmt/ecrustify.cfg --no-backup "$@"
sed -i 's/FOREACH (/FOREACH(/g;s/FREE (/FREE(/g;s/FOREACH_SAFE (/FOREACH_SAFE(/g' "$@"
sed -i 's/^ \([^ ]\)/\1/;' "$@"

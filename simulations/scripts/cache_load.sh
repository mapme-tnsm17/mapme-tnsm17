#!/bin/bash

echo "
set term svg
set output "$2/cache_load.svg"
# * 2 because of two prefixes
# * 3 because of three consumer nodes
# / 10Mb link capacity
# / 0.5 load
# in bits/s
plot "$1/consumer-stats.out" u 1:(\$2*8192*3*2/10000000 / 0.5)  w l, "" u 1:(\$3*8192*3*2/10000000 / 0.5)  w l
" | gnuplot

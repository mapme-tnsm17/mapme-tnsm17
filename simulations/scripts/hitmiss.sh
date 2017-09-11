#!/bin/bash

echo "
set term svg

set output '$2/c1.svg'
plot '$1/c1h' u 1:4 w l, '$1/c1m' u 1:4 w l

set output '$2/b1.svg'
plot '$1/b1h' u 1:4 w l, '$1/b1m' u 1:4 w l

set output '$2/a1.svg'
plot '$1/a1h' u 1:4 w l, '$1/a1m' u 1:4 w l
" | gnuplot

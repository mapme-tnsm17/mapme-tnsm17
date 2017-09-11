#!/bin/bash

echo "
set term svg
set logscale x
set output '$2/hr_vs_pop.svg'
plot '$1/pop_hr_sort-c1.dat' u 1:2 w l, \
     '$1/pop_hr_sort-b1.dat' u 1:2 w l, \
     '$1/pop_hr_sort-a1.dat' u 1:2 w l

set output '$2/hr_vs_rank.svg'
plot '$1/pop_hr_sort-c1.dat' u 0:2 w l, \
     '$1/pop_hr_sort-b1.dat' u 0:2 w l, \
     '$1/pop_hr_sort-a1.dat' u 0:2 w l
" | gnuplot

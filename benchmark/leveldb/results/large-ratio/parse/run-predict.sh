#!/bin/bash - 
#===============================================================================
#
#         USAGE:  ./run-predict.sh 
# 
#   DESCRIPTION:  predict using throughput and plot results
# 
#        AUTHOR: Ming Chen, mchen@cs.stonybrook.edu
#
#===============================================================================

set -o nounset                          # treat unset variables as an error
set -o errexit                          # stop script if command fail
export PATH="/bin:/usr/bin:/sbin:../../../../../software"             
IFS=$' \t\n'                            # reset IFS
unset -f unalias                        # make sure unalias is not a function
\unalias -a                             # unset all aliases
ulimit -H -c 0 --                       # disable core dump
hash -r                                 # clear the command path hash

tsf=1482
tsh=6542
tlf=13238
tlh=37599

cat > plot.p <<-EOF
set size 2, 1.8
set grid ytics

set terminal postscript eps noenhanced color "Times-Roman,40"
set output "ratio_thput_predict.eps"

set yrange [0:18]
set key
set xlabel "Ratio"
set ylabel "Throughput (mb/sec)"

plot 'mris_ratio_thput.dat' using 1:6 w points pt 20 title "benchmarked Hybrid", \
    1000000.0*(x*8 + 128)/(1024*($tsf * x + $tlh)) lw 3 title "predicted Hybrid", \
	'' using 1:3 w points pt 4 title "benchmarked SSD", \
    1000000.0*(x*8 + 128)/(1024*($tsf * x + $tlf)) lw 3 title "predicted SSD", \
	'' using 1:9 w points pt 13 title "benchmarked SATA", \
    1000000.0*(x*8 + 128)/(1024*($tlf * x + $tlh)) lw 3 lc rgb "black" title "predicted SATA"
EOF

gnuplot plot.p

if uname -a | grep -q Linux; then
	evince ratio_thput_predict.eps
elif uname -a | grep -q Mac; then
	open ratio_thput_predict.eps
fi

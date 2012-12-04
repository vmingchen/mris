#!/bin/bash - 
#===============================================================================
#
#         USAGE:  ./plot.sh 
# 
#   DESCRIPTION:  plot thput
# 
#        AUTHOR: Ming Chen, mchen@cs.stonybrook.edu
#
#===============================================================================

set -o nounset                          # treat unset variables as an error
set -o errexit                          # stop script if command fail
#export PATH="/bin:/usr/bin:/sbin"             
IFS=$' \t\n'                            # reset IFS
unset -f unalias                        # make sure unalias is not a function
\unalias -a                             # unset all aliases
ulimit -H -c 0 --                       # disable core dump
hash -r                                 # clear the command path hash

function plot() {
	ylabel="$1"
	name="$2"

	cat > plot.p <<-EOF
set size 2, 1.30
set terminal postscript eps noenhanced color "Times-Roman,40"
set output "${name}.eps"

set style data linespoints

set title "Throughput of mris_facebook (${ylabel})"
set xlabel "Setup" offset 0,1.2
set ylabel "Throughput (${ylabel})" offset 3.0,0
set style histogram errorbars gap 1 lw 2

set xtics offset 0,0.5
set ytics offset 1,0
#set yrange [0:16]
set key height 4

plot "${name}.dat" using 2:3:xticlabels(1) w histogram lw 2
EOF
	gnuplot plot.p
}

plot ops/sec mris_facebook_ops
plot mb/sec mris_facebook_thput

if uname -a | grep -q Linux; then
	evince *.eps
elif uname -a | grep -q Mac; then
	open *.eps
fi

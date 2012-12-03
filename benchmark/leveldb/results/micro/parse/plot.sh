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
	workload="$1"
	extsize="$2"

	name="${workload}-${extsize}es"
	cat > plot.p <<-EOF
set size 2, 1.30
set terminal postscript eps noenhanced color "Times-Roman,40"
set output "thput-${name}.eps"

set style data linespoints

set title "Throughput of $workload"
set xlabel "Working Set Size (mb)" offset 0,1.2
set ylabel "Throughput (mb/sec)" offset 3.0,0

set xtics offset 0,0.5
set ytics offset 1,0
set yrange [0:16]
set key left bottom height 4

plot "thput-${name}.dat" using 1:2:3 w yerrorlines title 'green-2mm-${extsize}es' lw 2, \
	 "" using 1:4:5 w yerrorlines title 'green-4mm-${extsize}es' lw 2, \
	 "" using 1:6:7 w yerrorlines title 'green-6mm-${extsize}es' lw 2, \
	 "" using 1:8:9 w yerrorlines title 'green-8mm-${extsize}es' lw 2
EOF
	gnuplot plot.p
}

plot rawdevice_rr_drct 16k
plot rawdevice_rw_drct 16k
plot rawdevice_sr_drct 16k
plot rawdevice_sw_drct 16k

if uname -a | grep -q Linux; then
	evince *.eps
elif uname -a | grep -q Mac; then
	open *.eps
fi

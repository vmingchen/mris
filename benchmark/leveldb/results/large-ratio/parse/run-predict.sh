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
tlf=6542
tsh=13238
tlh=37599

function show() {
	if uname -a | grep -q Linux; then
		evince $@
	elif uname -a | grep -q Mac; then
		open $@
	fi
}

function plot_mb() {
	cat > plot.p <<-EOF
	set size 2, 1.8
	set grid ytics

	set terminal postscript eps noenhanced color "Times-Roman,40"
	set output "ratio_thput_predict.eps"

	set yrange [0:18]
	set key
	set xlabel "Ratio"
	set ylabel "Throughput (mb/sec)"

	plot 'mris_ratio_mb.dat' using 1:6 w points pt 20 title "benchmarked Hybrid", \
		1000000.0*(x*8 + 128)/(1024*($tsf * x + $tlh)) lw 3 title "predicted Hybrid", \
		'' using 1:3 w points pt 13 title "benchmarked SSD", \
		1000000.0*(x*8 + 128)/(1024*($tsf * x + $tlf)) lw 3 title "predicted SSD", \
		'' using 1:9 w points lc rgb "#483D8B" pt 4 title "benchmarked SATA", \
		1000000.0*(x*8 + 128)/(1024*($tsh * x + $tlh)) lw 3 lc rgb "gray" title "predicted SATA"
	EOF
	gnuplot plot.p
	show ratio_thput_predict.eps
}

function plot_ops() {
	cat > plot.p <<-EOF
	set size 2, 2
	set grid ytics

	set terminal postscript eps noenhanced color "Times-Roman,40"
	set output "ratio_ops_predict.eps"

	set yrange [0:900]
	set key left 
	set xlabel "Ratio"
	set ylabel "Throughput (ops/sec)"

	plot 'mris_ratio_ops.dat' using 1:6 w points pt 20 title "benchmarked Hybrid", \
		1000000.0*(x + 1)/(($tsf * x + $tlh)) lw 3 title "predicted Hybrid", \
		'' using 1:3 w points pt 13 title "benchmarked SSD", \
		1000000.0*(x + 1)/(($tsf * x + $tlf)) lw 3 title "modeled SSD", \
		'' using 1:9 w points lc rgb "#483D8B" pt 4 title "benchmarked SATA", \
		1000000.0*(x + 1)/(($tsh * x + $tlh)) lw 3 lc rgb "gray" title "modeled SATA"
	EOF
	gnuplot plot.p
	show ratio_ops_predict.eps
}

plot_ops
#plot_mb

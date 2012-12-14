#!/bin/bash - 
#===============================================================================
#
#         USAGE:  ./plot-iostat.sh 
# 
#   DESCRIPTION:  plot iostat statistics
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


function plot_iostat_rps() {
	ylabel="$1"
	name="$2"

	cat > plot.p <<-EOF
set size 2, 1.30
set grid
set terminal postscript eps noenhanced color "Times-Roman,40"
set output "${name}.eps"

set style data linespoints

set title "op/sec of mris_ratio by iostat (${ylabel})"
set xlabel "Ratio" offset 0,1.2
set ylabel "Throughput (${ylabel})" offset 3.0,0
set style histogram errorbars gap 1 lw 2

set xtics offset 0,0.5
set ytics offset 1,0
#set yrange [0:16]
set key height 4

plot "${name}.dat" using 3:4:xticlabels(1) w histogram lw 2 title "SSD-SSD", \
	"" using 8:9:xticlabels(1) w histogram lw 2 title "Hybrid-SSD", \
	"" using 10:11:xticlabels(1) w histogram lw 2 title "Hybrid-SATA", \
	"" using 15:16:xticlabels(1) w histogram lw 2 title "SATA-SATA"
EOF
#plot "${name}.dat" using 3:4:xticlabels(1) w histogram lw 2 title "SSD-SSD", \
	#"" using 5:6:xticlabels(1) w histogram lw 2 title "SSD-SATA", \
	#"" using 8:9:xticlabels(1) w histogram lw 2 title "Hybrid-SSD", \
	#"" using 10:11:xticlabels(1) w histogram lw 2 title "Hybrid-SATA", \
	#"" using 13:14:xticlabels(1) w histogram lw 2 title "SATA-SSD", \
	#"" using 15:16:xticlabels(1) w histogram lw 2 title "SATA-SATA"
	gnuplot plot.p
}

function plot_iostat() {
	ylabel="$1"
	name="$2"

	cat > plot.bargraph <<-EOF
=stackcluster;SSD;SATA
=noupperright
#title=Throughput (${ylabel}) by iostat
legendx=right
legendy=center
yformat=%g
xlabel=Ratio
ylabel=Throughput (${ylabel})

=table
EOF

	awk '(substr($1, 1, 1) != "#"){
		printf("multimulti=%s\n", $1);
		printf("%s\t%s\t%s\n", $2, $3, $5);
		printf("%s\t%s\t%s\n", $7, $8, $10);
		printf("%s\t%s\t%s\n", $12, $13, $15);
	}' $name.dat >> plot.bargraph

	bargraph.pl plot.bargraph > ${name}.eps
}

#plot_iostat_rps mb/sec mris_ratio_iostat_thput

plot_iostat mb/sec mris_ratio_iostat_thput

if uname -a | grep -q Linux; then
	evince *.eps
elif uname -a | grep -q Mac; then
	open *.eps
fi

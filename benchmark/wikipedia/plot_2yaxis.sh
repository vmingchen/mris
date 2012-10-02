#!/bin/bash - 
#===============================================================================
#
#         USAGE:  ./plot_2yaxis.sh 
# 
#   DESCRIPTION:  Plot using 2 Y-axis, one as request frequency, the other as
#   total request amount.
# 
#        AUTHOR: Ming Chen, mchen@cs.stonybrook.edu
#
#===============================================================================

set -o nounset                          # treat unset variables as an error
set -o errexit                          # stop script if command fail
export PATH="/bin:/usr/bin:/sbin"             
IFS=$' \t\n'                            # reset IFS
unset -f unalias                        # make sure unalias is not a function
\unalias -a                             # unset all aliases
ulimit -H -c 0 --                       # disable core dump
hash -r                                 # clear the command path hash

./annotate_image.py freq2size-many.dat > freq2size-many.res
./annotate_image.py traf2size-many.dat > traf2size-many.res

cat > plot.p  <<EOF
set boxwidth 0.3 absolute
set terminal postscript eps enhanced color

binwidth=20
red = "#080000"; green = "#000800"; blue = "#000008"; gray = "#888888"
set style data histogram
set output "freq+traf.eps"
set xtics nomirror rotate by -45 font ",14"
set xlabel "Image Size" font "Times-Roman, 24"

set ylabel "Request Frequency" font "Time-Roman, 24" offset -1,0
set y2tics
set y2label "Request Traffic Amount (sector)" font "Time-Roman, 24" offset -1,0
#set logscale y2 2

set boxwidth 0.9
set label font ", 24"
set key left height 1 width 12

plot "freq2size-many.res" using 3:xticlabels(1)  with histogram lw 2 axes x1y1 title "Reqeust Frequency",	\
	"traf2size-many.res" using 3:xticlabels(1)  with histogram lw 2 axes x1y2 title "Reqeust Amount"
EOF

gnuplot plot.p

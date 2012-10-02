#!/bin/bash - 
#===============================================================================
#
#         USAGE:  ./plot_many.sh 
# 
#   DESCRIPTION:  Plot image size distribution of multiple Wikipedia pagecount
#   files.
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

awk '
	(substr($1, 1, 1) != "#") { 
		freq[$1] += $2; 
	} END {
		for(sz in freq) {
			print sz, freq[sz];
		}
	}' data/image-*.out > freq2size-many.dat

# prepare data for plotting
./annotate_image.py freq2size-many.dat > freq2size-many.res

cat > freq2size.p <<EOF
set boxwidth 0.3 absolute
set terminal postscript eps enhanced color

binwidth=20
red = "#080000"; green = "#000800"; blue = "#000008"; gray = "#888888"
set style data histogram
set output "freq2size-many.eps"
set xlabel "Image Size" font "Times-Roman, 24"
set ylabel "Request Frequency" font "Time-Roman, 24" offset -1,0
set boxwidth 0.9
set label font ", 24"
set xtics nomirror rotate by -45 font ",14"
set key left height 1 width 8

plot "freq2size-many.res" using 3:xticlabels(1)  with histogram lw 2 title "Wikipedia Image Reqeust Frequency"
EOF

gnuplot freq2size.p

awk '
	(substr($1, 1, 1) != "#") { 
		freq[$1] += $2; 
	} END {
		for(sz in freq) {
			print sz, sz*freq[sz];
		}
	}' data/image-*.out > traf2size-many.dat

# prepare data for plotting
./annotate_image.py traf2size-many.dat > traf2size-many.res

cat > traf2size.p <<EOF
set boxwidth 0.3 absolute
set terminal postscript eps enhanced color

binwidth=20
red = "#080000"; green = "#000800"; blue = "#000008"; gray = "#888888"
set style data histogram
set output "traf2size-many.eps"
set xlabel "Image Size" font "Times-Roman, 24"
set ylabel "Request Traffic Amount" font "Time-Roman, 24" offset -1,0
set boxwidth 0.9
set label font ", 24"
set xtics nomirror rotate by -45 font ",14"
set key left height 1 width 8

plot "traf2size-many.res" using 3:xticlabels(1)  with histogram lw 2 title "Wikipedia Image Reqeust Traffic"
EOF

gnuplot traf2size.p

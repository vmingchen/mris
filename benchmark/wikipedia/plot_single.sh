#!/bin/bash - 
#=============================================================================
#
#         USAGE:  ./plot_single.sh 
# 
#   DESCRIPTION:  Plot image size distribution of a single Wikipedia pagecount
#   file.
# 
#        AUTHOR: Ming Chen, mchen@cs.stonybrook.edu
#
#=============================================================================

set -o nounset                          # treat unset variables as an error
set -o errexit                          # stop script if command fail
export PATH="/bin:/usr/bin:/sbin"             
IFS=$' \t\n'                            # reset IFS
unset -f unalias                        # make sure unalias is not a function
\unalias -a                             # unset all aliases
ulimit -H -c 0 --                       # disable core dump
hash -r                                 # clear the command path hash

filepath="$1"
nlen="${#filepath}"
if [ "x${filepath:$nlen-3}" = "x.gz" ]; then
	no=${filepath:$nlen-14:10}
	zcat $filepath | ./parse_raw.sh `basename $filepath` request-$no.dat image-$no.dat
else
	no=${filepath:$nlen-11:7}
	cat $filepath | ./parse_raw.sh `basename $filepath` request-$no.dat image-$no.dat
fi
echo "no: $no"

# prepare data for plotting
./annotate_image.py image-$no.dat > $no.res

cat > image.p <<EOF
set boxwidth 0.3 absolute
set terminal postscript eps enhanced color

binwidth=20
red = "#080000"; green = "#000800"; blue = "#000008"; gray = "#888888"
set style data histogram
set output "imsz-$no.eps"
set xlabel "Image Size" font "Times-Roman, 30"
set ylabel "Frequency" font "Time-Roman, 30" offset -1,0
set boxwidth 0.9
set label font ", 24"
set xtics nomirror rotate by -45 font ",14"
set key left height 1 width 8

plot "$no.res" using 3:xticlabels(1)  with histogram lw 2 title "Wikipedia $no Image Size"
EOF

gnuplot image.p

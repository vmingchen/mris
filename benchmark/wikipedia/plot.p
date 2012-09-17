# set terminal pngcairo  transparent enhanced font "arial,10" fontscale 1.0 size 500, 350 
# set output 'datastrings.2.png'
set boxwidth 0.3 absolute
set terminal postscript eps enhanced color
# set style histogram cluster gap 1

binwidth=20
red = "#080000"; green = "#000800"; blue = "#000008"; gray = "#888888"
set style data histogram
set output "size_dist.eps"
set xlabel "I/O Size" font "Times-Roman, 30"
# offset -1,0 to move ylable to the left a little
set ylabel "Throughput (mb/s)" font "Time-Roman, 30" offset -1,0
#set style histogram errorbars gap 1 lw 2
#set style histogram errorbars cluster gap 2
#set style fill solid
set boxwidth 0.9
set label font ", 24"
set xtics nomirror rotate by -45 font ",12"
#set key left height 2 width 4
set key left height 1 width 8

plot "size.dat" using 3:xticlabels(1)  with histogram lw 2 title "{/Times=20 SATA-Random-Read}"

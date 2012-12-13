set size 2, 1.30
set grid ytics
set terminal postscript eps noenhanced color "Times-Roman,40"
set output "micro_thput.eps"

set style data linespoints

set title "Throughput of MRIS write (mb/sec)"
set xlabel "Workload" offset 0,1.2
set ylabel "Throughput (mb/sec)" offset 3.0,0
set style histogram errorbars gap 1 lw 2

set xtics offset 0,0.5
set ytics offset 1,0
#set yrange [0:16]
set key left height 3

plot "micro_thput.dat" using 3:4:xticlabels(1) w histogram lw 2 title "SSD", 	"" using 6:7:xticlabels(1) w histogram lw 2 title "Hybrid", 	"" using 9:10:xticlabels(1) w histogram lw 2 title "SATA"
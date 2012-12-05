set size 2, 1.30
set grid
set terminal postscript eps noenhanced color "Times-Roman,40"
set output "mris_ratio_iostat_thput.eps"

set style data linespoints

set title "Throughput of mris_ratio by iostat (mb/sec)"
set xlabel "Ratio" offset 0,1.2
set ylabel "Throughput (mb/sec)" offset 3.0,0
set style histogram errorbars gap 1 lw 2

set xtics offset 0,0.5
set ytics offset 1,0
#set yrange [0:16]
set key height 4

plot "mris_ratio_iostat_thput.dat" using 3:4:xticlabels(1) w histogram lw 2 title "SSD-SSD", 	"" using 8:9:xticlabels(1) w histogram lw 2 title "Hybrid-SSD", 	"" using 10:11:xticlabels(1) w histogram lw 2 title "Hybrid-SATA", 	"" using 15:16:xticlabels(1) w histogram lw 2 title "SATA-SATA"

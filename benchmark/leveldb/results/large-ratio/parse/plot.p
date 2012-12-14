set size 2, 1.8
set grid ytics

set terminal postscript eps noenhanced color "Times-Roman,40"
set output "ratio_thput_predict.eps"

set yrange [0:18]
set key
set xlabel "Ratio"
set ylabel "Throughput (mb/sec)"

plot 'mris_ratio_thput.dat' using 1:6 w points pt 20 title "benchmarked Hybrid",     1000000.0*(x*8 + 128)/(1024*(1482 * x + 37599)) lw 3 title "predicted Hybrid", 	'' using 1:3 w points pt 4 title "benchmarked SSD",     1000000.0*(x*8 + 128)/(1024*(1482 * x + 13238)) lw 3 title "predicted SSD", 	'' using 1:9 w points pt 13 title "benchmarked SATA",     1000000.0*(x*8 + 128)/(1024*(13238 * x + 37599)) lw 3 lc rgb "black" title "predicted SATA"

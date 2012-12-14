set size 2, 2
set grid ytics

set terminal postscript eps noenhanced color "Times-Roman,40"
set output "ratio_ops_predict.eps"

set yrange [0:900]
set key left 
set xlabel "Ratio"
set ylabel "Throughput (ops/sec)"

plot 'mris_ratio_ops.dat' using 1:6 w points pt 20 title "benchmarked Hybrid", 		1000000.0*(x + 1)/((1482 * x + 37599)) lw 3 title "predicted Hybrid", 		'' using 1:3 w points pt 13 title "benchmarked SSD", 		1000000.0*(x + 1)/((1482 * x + 6542)) lw 3 title "modeled SSD", 		'' using 1:9 w points lc rgb "#483D8B" pt 4 title "benchmarked SATA", 		1000000.0*(x + 1)/((13238 * x + 37599)) lw 3 lc rgb "gray" title "modeled SATA"

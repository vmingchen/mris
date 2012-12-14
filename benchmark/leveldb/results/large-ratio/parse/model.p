set size 2, 1.3
set grid ytics

set terminal postscript eps noenhanced color "Times-Roman,40"
set output "ratio_ops_estimate.eps"

set yrange [0:600]
set xlabel "Ratio"
set ylabel "Throughput (ops/sec)"

set key left

plot 'ratio_ops_estimate.dat' w points pt 20  title "benchmarked ops/sec", \
    1000000.0*(x + 1)/(1482 * x + 37599) title "estimated ops/sec"

set output "ratio_thput_estimate.eps"

set yrange [0:6]
set xlabel "Ratio"
set ylabel "Throughput (mb/sec)"

plot 'ratio_thput_estimate.dat' w points pt 20  title "benchmarked mb/sec", \
    1000000.0*(x*8 + 128)/(1024*(1482 * x + 37599)) title "estimated mb/sec"

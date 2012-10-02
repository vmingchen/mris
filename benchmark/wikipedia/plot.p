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

plot "freq2size-many.res" using 3:xticlabels(1)  with histogram lw 2 axes x1y1 title "Reqeust Frequency",		"traf2size-many.res" using 3:xticlabels(1)  with histogram lw 2 axes x1y2 title "Reqeust Amount"

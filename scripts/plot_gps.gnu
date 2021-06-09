#
# gnuplot script.
#
set terminal x11 size 800, 600
#
set key autotitle columnhead
#
set title "GPS"
#
set xlabel "Time (secs)"
set ytics border nomirror
set y2tics border nomirror
#
set ylabel "Altitude (m)" textcolor rgbcolor "red"
#set y2label "Satellites" textcolor rgbcolor "blue"
set y2label "DOP" textcolor rgbcolor "blue"
#
set xrange [0:*]
set yrange [-10:*]
#set y2range [0:14]
set y2range [0:2]
#
#
plot "gps_log.tsv" using 2:3 with lines linecolor rgbcolor "green" title "Altitude" axes x1y1, "" using 2:6 with lines linecolor rgbcolor "blue" title "VDOP" axes x1y2
#
pause -1 "Hit return to continue"
#

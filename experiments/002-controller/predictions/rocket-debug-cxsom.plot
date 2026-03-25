set multiplot layout 2,1 title 'CXSOM controller debug'
plot 'predictions/rocket-debug-cxsom.dat' using 1:4 with lines title 'error', \
     '' using 1:5 with lines title 'speed'
plot 'predictions/rocket-debug-cxsom.dat' using 1:6 with lines title 'thrust'
unset multiplot

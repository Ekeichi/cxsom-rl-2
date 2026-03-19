set xlabel 'error'
set ylabel 'speed'
set zlabel 'Q'
set title  'Q(s, a = 15)'
set hidden3d
set dgrid3d 51,51
splot 'rocket-discrete-controller.dat' using 1:2:5 with lines notitle

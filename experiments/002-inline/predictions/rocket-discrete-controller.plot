set xlabel 'error'
set ylabel 'speed'
set zlabel 'thrust'
set title  'best discrete rocket controller'
set hidden3d
set dgrid3d 51,51
splot 'rocket-discrete-controller.dat' using 1:2:3 with lines notitle

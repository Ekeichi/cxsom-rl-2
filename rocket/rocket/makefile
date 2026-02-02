# This is how we compile rules.
som: xsom.cpp
	g++ -o xsom `pkg-config --cflags cxsom-builder` xsom.cpp `pkg-config --libs cxsom-builder`

test: test.cpp
	g++ -o test `pkg-config --cflags cxsom-builder` test.cpp `pkg-config --libs cxsom-builder`
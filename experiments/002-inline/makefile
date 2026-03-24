include /usr/share/cxsom/cxsom-makefile

CXSOM_HEADERS = $(wildcard *.hpp)

xsom-%: xsom-%.cpp $(CXSOM_HEADERS)
	@g++ -o $@ `pkg-config --cflags cxsom-builder` $< `pkg-config --libs cxsom-builder`

.PHONY: controller
controller: controller.cpp
	@g++ -std=c++20 -O2 -o controller controller.cpp \
		-I/usr/include/cxsom \
		-I/usr/include/rllib2 \
		-I. \
		-Ipredictions \
		`pkg-config --cflags gdyn rllib2` \
		`pkg-config --libs gdyn rllib2`

.PHONY: send-train-rules
send-train-rules: xsom-train
	@./xsom-train send `cat .cxsom-hostname-config` `cat .cxsom-port-config`

include /usr/share/cxsom/cxsom-makefile
DATA_FILE = ./predictions/rocket-orbit.dat
CXSOM_HEADERS = $(wildcard *.hpp)

xsom-%: xsom-%.cpp $(CXSOM_HEADERS)
	@g++ -o $@ `pkg-config --cflags cxsom-builder` $< `pkg-config --libs cxsom-builder`

rocket: rocket.cpp
	@g++ -std=c++20 -O2 -o rocket rocket.cpp \
	-I/usr/include/cxsom \
	-I/usr/include/rllib2 \
	-I. \
	-Ipredictions \
	`pkg-config --cflags gdyn rllib2` \
	`pkg-config --libs gdyn rllib2`

.PHONY: send-train-data
send-train-data: rocket.cpp
	@./rocket `cat .cxsom-hostname-config` `cat .cxsom-port-config`

.PHONY: send-train-rules
send-train-rules: xsom-train
	@./xsom-train send `cat .cxsom-hostname-config` `cat .cxsom-port-config`

.PHONY: show-string-% 
show-string-%:
	@python show-string.py --root-dir `cat .cxsom-rootdir-config` --target-map $* --data-file $(DATA_FILE)
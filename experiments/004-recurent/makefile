include /usr/share/cxsom/cxsom-makefile
DATA_FILE = ./data/rocket-orbit.dat
CXSOM_HEADERS = $(wildcard *.hpp)

recsom-%: recsom-%.cpp $(CXSOM_HEADERS)
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
send-train-rules: recsom-train
	@./recsom-train send `cat .cxsom-hostname-config` `cat .cxsom-port-config`

.PHONY: show-data
show-data: show-data.py
	@python3 show-data.py


.PHONY: show-training-weights
show-training-weights:
	@python show-weights.py --timeline wgt --root-dir `cat .cxsom-rootdir-config` &

.PHONY: clear-predict
clear-predict: 
	@rm -rf `cat .cxsom-rootdir-config`/predict `cat .cxsom-rootdir-config`/rlx `cat .cxsom-rootdir-config`/out

.PHONY: clear-all
clear-all: clear-predict
	@rm -rf `cat .cxsom-rootdir-config`/in  `cat .cxsom-rootdir-config`/wgt  

rocket-pred: rocket-pred.cpp
	@g++ -std=c++20 -O2 -o rocket-pred rocket-pred.cpp \
	-I/usr/include/cxsom \
	-I/usr/include/rllib2 \
	-I. \
	-Ipredictions \
	`pkg-config --cflags gdyn rllib2` \
	`pkg-config --libs gdyn rllib2`

.PHONY: send-predict-data
send-predict-data: rocket-pred
	@./rocket-pred `cat .cxsom-hostname-config` `cat .cxsom-port-config`
	@echo "ping du processeur..."
	@make --quiet cxsom-ping-processor

.PHONY: send-predict-rules 
send-predict-rules: recsom-predict
	@./recsom-predict send `cat .cxsom-hostname-config` `cat .cxsom-port-config` -- $(SAVEPOINT)


.PHONY: show-predictions
show-predictions: show-prediction.py
	@python show-prediction.py


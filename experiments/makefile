

help:
	@echo
	@echo "make help-cxsom"
	@echo
	@echo "make send-train-data NB_PASSES=3 SMOOTHING=True"
	@echo "make restart-train NB_PASSES=3 SMOOTHING=True"
	@echo
	@echo


# peut être nécessaire d'adapter le chemin en fonction de l'installation
include /usr/share/cxsom/cxsom-makefile

SAMPLES ?=
DATA_FILE = ./data/rocket-discrete-controller.dat

CXSOM_HEADERS = $(wildcard *.hpp)


xsom-%: xsom-%.cpp $(CXSOM_HEADERS)
	@g++ -o $@ `pkg-config --cflags cxsom-builder` $< `pkg-config --libs cxsom-builder`

.PHONY: send-train-rules
send-train-rules: xsom-train
	@./xsom-train send `cat .cxsom-hostname-config` `cat .cxsom-port-config`

.PHONY: send-train-data
send-train-data:  send-train-data.py
	@python send-train-data.py --root-dir `cat .cxsom-rootdir-config` --data-file $(DATA_FILE) --nb-passes $(NB_PASSES) --smoothing $(SMOOTHING)
	@echo "ping du processeur..." 
	@make --quiet cxsom-ping-processor
	@echo "Entrainement lancé." 

.PHONY: show-train-data
show-train-data: show-train-data.py
	@if [ -z "$(SAMPLES)" ]; then \
		python show-train-data.py root-dir ; \
	else \
		python show-train-data.py root-dir $(SAMPLES) ; \
	fi

.PHONY: send-predict-rules 
send-predict-rules: xsom-predict
	@./xsom-predict send `cat .cxsom-hostname-config` `cat .cxsom-port-config` -- $(SAVEPOINT)

.PHONY: send-predict-data
send-predict-data:  send-predict-data.py
	@python send-predict-data.py
	@echo "ping du processeur..." 
	@make --quiet cxsom-ping-processor

.PHONY: show-predictions
show-predictions: show-prediction.py
	@python show-prediction.py

.PHONY: clear-predict
clear-predict: 
	@rm -rf `cat .cxsom-rootdir-config`/predict `cat .cxsom-rootdir-config`/rlx `cat .cxsom-rootdir-config`/out

.PHONY: clear-all
clear-all: clear-predict
	@rm -rf `cat .cxsom-rootdir-config`/in  `cat .cxsom-rootdir-config`/wgt  

.PHONY: show-training-weights
show-training-weights:
	@python show-weights.py --timeline wgt --root-dir `cat .cxsom-rootdir-config` &

.PHONY: show-saved-weights
show-saved-weights:
	@python show-weights.py --timeline save --root-dir `cat .cxsom-rootdir-config` &

.PHONY: show-BMU
show-BMU:
	@python show-BMU.py &

.PHONY: show-string-% 
show-string-%:
	@python show-string.py --root-dir `cat .cxsom-rootdir-config` --target-map $* --data-file $(DATA_FILE)

# push sur gitlab
push-gitlab:
	@git add .
	@git commit -m "update"
	@git push


.PHONY: restart-train
restart-train:
	@make --quiet cxsom-clear-rootdir cxsom-clear-processor send-train-rules
	@make --quiet send-train-data 

# peut être nécessaire d'adapter le chemin en fonction de l'installation
include /usr/share/cxsom/cxsom-makefile

SAMPLES ?=

CXSOM_HEADERS = $(wildcard *.hpp)

	
xsom-%: xsom-%.cpp $(CXSOM_HEADERS)
	@g++ -o $@ `pkg-config --cflags cxsom-builder` $< `pkg-config --libs cxsom-builder`

.PHONY: send-train-rules
send-train-rules: xsom-train
	@./xsom-train send `cat .cxsom-hostname-config` `cat .cxsom-port-config`

.PHONY: send-train-data
send-data:  send-train-data.py
	@python send-train-data.py
	@echo "ping du processeur..." 
	@make --quiet cxsom-ping-processor
	@echo "Entrainement lancé." 

.PHONY: show-train-data
show-3d: show-train-data.py
	@if [ -z "$(SAMPLES)" ]; then \
		python show-rocket-3D.py root-dir ; \
	else \
		python show-rocket-3D.py root-dir $(SAMPLES) ; \
	fi

.PHONY: send-predict-rules 
send-predict-rules: xsom-predict
	@./xsom-predict send `cat .cxsom-hostname-config` `cat .cxsom-port-config` -- $(SAVEPOINT)

.PHONY: send-predict-data
send-predict-data:  send-predict-data.py
	@python send-predict.py
	@echo "ping du processeur..." 
	@make --quiet cxsom-ping-processor

.PHONY: show-predictions
show-predictions: show-prediction.py
	@python show-predict.py

.PHONY: clear-predict
clear-predict: 
	@rm -rf `cat .cxsom-rootdir-config`/predict `cat .cxsom-rootdir-config`/rlx `cat .cxsom-rootdir-config`/out

.PHONY: clear-all
clear-all: clear-predict
	@rm -rf `cat .cxsom-rootdir-config`/in  `cat .cxsom-rootdir-config`/wgt  

.PHONY: show-training-weights
show-training-weights:
	@python view-training-weights.py &

# push sur gitlab
push-gitlab:
	@git add .
	@git commit -m "update"
	@git push

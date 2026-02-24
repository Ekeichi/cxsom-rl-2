# peut être nécessaire d'adapter le chemin en fonction de l'installation
include /usr/share/cxsom/cxsom-makefile

SAMPLES ?=
SAVEPOINT ?=260

xsom-train: xsom-train.cpp
	@g++ -o xsom-train `pkg-config --cflags cxsom-builder` xsom-train.cpp `pkg-config --libs cxsom-builder`

xsom-predict: xsom-predict.cpp
	@g++ -o xsom-predict `pkg-config --cflags cxsom-builder` xsom-predict.cpp `pkg-config --libs cxsom-builder`


.PHONY: send-train-rules
send-train-rules: xsom-train
	@./xsom-train send `cat .cxsom-hostname-config` `cat .cxsom-port-config`

.PHONY: send-data
send-data:  send.py
	@python send.py

.PHONY: show-3d
show-3d: show-rocket-3D.py
	@if [ -z "$(SAMPLES)" ]; then \
		python show-rocket-3D.py root-dir ; \
	else \
		python show-rocket-3D.py root-dir $(SAMPLES) ; \
	fi

.PHONY: feed
feed:  feed.py
	@python feed.py

.PHONY: send-predict-rules 
send-predict-rules: xsom-predict
	@./xsom-predict send `cat .cxsom-hostname-config` `cat .cxsom-port-config` -- $(SAVEPOINT)

.PHONY: send-predict-data
send-predict-data:  send-predict.py
	@python send-predict.py
	@make --quiet cxsom-ping-processor

.PHONY: show-predictions
show-predictions: show-predict.py
	@python show-predict.py

.PHONY: clear-predict
clear-predict: 
	@rm -rf `cat .cxsom-rootdir-config`/predict `cat .cxsom-rootdir-config`/rlx `cat .cxsom-rootdir-config`/out

.PHONY: show-weights
show-weights:
	@python view-weights.py 

# push sur gitlab
push-gitlab:
	@git add .
	@git commit -m "update"
	@git push

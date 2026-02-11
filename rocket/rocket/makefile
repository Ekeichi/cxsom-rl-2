# peut être nécessaire d'adapter le chemin en fonction de l'installation
include /usr/share/cxsom/cxsom-makefile

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

.PHONY: feed
feed:  feed.py
	@python feed.py

.PHONY: send-predict-rules
send-predict-rules: xsom-predict
	@./xsom-predict send `cat .cxsom-hostname-config` `cat .cxsom-port-config`

# push sur gitlab
push-gitlab:
	@git add .
	@git commit -m "update"
	@git push

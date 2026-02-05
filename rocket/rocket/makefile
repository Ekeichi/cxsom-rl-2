# peut être nécessaire d'adapter le chemin en fonction de l'installation
include /usr/share/cxsom/cxsom-makefile

xsom: xsom.cpp
	@g++ -o xsom `pkg-config --cflags cxsom-builder` xsom.cpp `pkg-config --libs cxsom-builder`


.PHONY: send-rules
send-rules: xsom
	@./xsom send `cat .cxsom-hostname-config` `cat .cxsom-port-config`

.PHONY: send
send-data:  send.py
	@python send.py

.PHONY: feed
feed:  feed.py
	@python feed.py
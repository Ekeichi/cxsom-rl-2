# peut être nécessaire d'adapter le chemin en fonction de l'installation
include /usr/share/cxsom/cxsom-makefile

xsom: xsom.cpp
	@g++ -o xsom `pkg-config --cflags cxsom-builder` xsom.cpp `pkg-config --libs cxsom-builder`


dataset:
	@python build-dataset.py

# plus dispo (à revoir éventuellement)
# 	@dot -Tpdf architecture/3soms-archi.dot -o architecture/archi.pdf
# 	@rm -f architecture/3soms-archi.dot

.PHONY: send-rules
send-rules: xsom
	@./xsom send `cat .cxsom-hostname-config` `cat .cxsom-port-config`
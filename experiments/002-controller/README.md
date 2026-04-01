# Suivre la procédure suivante pour tester

### Permet de creer un root-dir, et de configurer les variables pour le processeur.
```
mkdir root-dir
make cxsom-set-config ROOT_DIR=./root-dir VENV=../cxsom-venv HOSTNAME=localhost PORT=10000 SKEDNET_PORT=20000 NB_THREADS=4
```

### Permet de lancer le processeur et scanner le root-dir (pour le moment vide)
```
make cxsom-launch-processor
make cxsom-scan-vars
```
# Entrainement

### Envoyer les regles d'entrainement au processeur (le root-dir se remplit)
```
make send-train-rules
```

### Envoyer les input avec les données de la fusée dans le rootdir (elles s'affichent dans le scan-vars)
```
make send-train-data NB_PASSES=X SMOOTHING=Y
```

Là, l'entrainement se lance.

### Compiler la nouvelle architecture
```
make xsom-predict
```

### Nettoyer le processeur avant prédiction
```
make cxsom-clear-processor
```

### Nettoyer le root-dir avant la prédiction
Il est primordial de reinitialiser le root-dir avant de faire les predictions. Cependant, il faut bien garder les poids sauvvegarder après l'entrainement. 
```
make clear-all
```

### Envoyer les règles de prediction au processeur
L'argument SAVEPOINT permet de choisir les poids qui seront utilisés pour faire les predictions. 
```
make send-predict-rules SAVEPOINT=xxx
```

### Envoyer les données de prediction et lancer la prediction
```
make send-predict-data
```

## Pipeline cxsom-controller
```
make cxsom-clear-processor
make clear-all
make send-predict-rules SAVEPOINT=X
make cxsom-controller
```

## Questions
- cxsom::processor::ping existe? J'ai regardé dans cxsom/cxsom/src/cxsom-processor.hpp
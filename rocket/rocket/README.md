Implementation d'une organisation de cartes auto-organisatrices pour prédire la poussée à donner à la fusée en fonction de la vitesse et de l'erreur de position.
J'essaie actuellement de comprendre comment les exemples cxsom fonctionnent pour adapter ma propre implémentation.
Le code présent permet de definir l'architecture des cartes et les relations entre elles. 

Important : veillez à bien activer le cxsom-venv en amont.

# Suivre la procédure suivante pour tester

### Permet de creer un root-dir, et de configurer les variables pour le processeur.
```
mkdir root-dir
make cxsom-set-config ROOT_DIR=./root-dir VENV=../cxsom-venv HOSTNAME=localhost PORT=10000 SKEDNET_PORT=20000 NB_THREADS=4
```

### Permet de reset le serveur et le root-dir
```
make cxsom-kill-processor
make cxsom-clear-rootdir
```

### Permet de lancer le processeur et scanner le root-dir (pour le moment vide)
```
make cxsom-launch-processor
make cxsom-scan-vars
```
## Entrainement

### Envoyer les regles au processeur (le root-dir se remplit)
```
make send-rules
```

### Envoyer les input avec les données de la fusée dans le rootdir (elles s'affichent dans le scan-vars)
```
make send-data
```

### Lancer l'entrainement
```
make feed
```
Cependant, le fichier feed.py ne permet a priori pas de lancer l'entrainement. Il doit manquer un élément.

# Prédictions
L'architecture est légérement modifiée lorsqu'on veut réaliser une prediction. En effet, la carte qui gère le thrust ne reçoit plus de valeur exterieur quand on souhaite prédire. 
Pour ce faire, nous avons décidé de creer un deuxième fichier cpp, nommé xsom-predict.cpp, qui permet de définir la nouvelle architecture de prédiction. 

### Compiler la nouvelle architecture
```
make xsom-predict
```

### Nettoyer le processeur avant prédiction
```
make cxsom-clear-processor
```


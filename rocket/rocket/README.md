Implementation d'une organisation de cartes auto-organisatrices pour prédire la poussée à donner à la fusée en fonction de la vitesse et de l'erreur de position.
J'essaie actuellement de comprendre comment les exemples cxsom fonctionnent pour adapter ma propre implémentation.
Le code présent permet de definir l'architecture des cartes et les relations entre elles. 

Important : veillez à bien activer le cxsom-venv en amont.

## Suivre la procédure suivante pour tester

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

### Envoyer les regles au processeur (le root-dir se remplit)
```
make send-rules
```

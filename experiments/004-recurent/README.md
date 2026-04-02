Permet d'entrainer une carte de facon recurrente.

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

### Permet d'envoyer les regles au processeur
```
make send-train-rules
```

### Permet d'envoyer les données au processeur
```
make send-train-data
```

### Permet de visualiser les données
```
make show-data
```

### Nettoyer le processeur avant prédiction
```
make cxsom-clear-processor
```

### Il est imperatif de nettoyer le root-dir
```
make clear-all
```

### Permet d'envoyer les regles au processeur
```
make send-predict-rules
```

### Permet d'envoyer les données au processeur
```
make send-predict-data
```

### Permet de visualiser les données
```
make show-prediction
```
# Structure de Présentation - Soutenance PFE CXSOM-RL-2

**Durée totale :** 20 minutes
**Type :** Soutenance PFE
**Sujet :** Contrôle de fusée par cartes auto-organisatrices contextuelles

---

## 🎯 Objectif de la présentation

Entrevoir comment les cartes auto-organisatrices contextuelles (CXSOM) peuvent être utilisées pour apprendre et exécuter une politique de contrôle pour un lanceur/fusée, en explorant différentes approches d'apprentissage.

---

## 📋 Plan détaillé

### 1. INTRODUCTION (2 minutes)

#### Contexte, problématique et objectif (2 min)
**À dire :**
> "Guider une fusée vers une cible est un problème classique en automatique, résolu classiquement par des PID qui nécessitent un modèle physique précis. L'objectif de ce projet : apprendre cette politique de contrôle directement à partir de données, en utilisant les cartes auto-organisatrices contextuelles (CXSOM). Nous avons testé différentes architectures progressives pour permettre le controle d'une fusée sur un seul axe."

**Points clés :**
- Variables d'état : erreur (position - cible), vitesse
- Variable de commande : poussée (0 ou max)

**Visuel suggéré :**
- Schéma fusée avec variables annotées + timeline des 4 expériences

---

### 2. FONDEMENTS THÉORIQUES (4 minutes)

#### 2.1 Self-Organizing Maps (SOM) - Rappel (1.5 min)

**À dire :**
> "Les SOM de Kohonen sont des réseaux non-supervisés qui apprennent à représenter des données en préservant leur topologie. Pour chaque entrée, on trouve le neurone le plus proche (BMU), puis on ajuste ses voisins. Des données similaires → neurones voisins sur la carte."

**Concepts clés :**
- Grille de neurones (ici : 1D, 2000 neurones par carte)
- BMU = neurone dont les poids sont les plus proches de l'entrée
- Mise à jour : BMU + voisins se rapprochent de l'entrée

**Visuel suggéré :**
- Schéma entrée → BMU → mise à jour voisinage (pas d'animation, une image suffit)

#### 2.2 CXSOM : Extension contextuelle (1.5 min)

**À dire :**
> "Une SOM classique traite une seule variable. Avec 3 variables interdépendantes (erreur, vitesse, poussée), les CXSOM créent plusieurs cartes qui communiquent entre elles via des connexions contextuelles."

**Architecture de base (3 cartes) :**
```
      Connexions externes (inputs)
            ↓     ↓     ↓
        [Error] [Speed] [Thrust]
            ↕     ↕     ↕
      Connexions contextuelles (inter-cartes)
```

**Types de connexions :**

1. **Connexions externes** (input → carte)
   - `error_input` → errorMap
   - `speed_input` → speedMap
   - `thrust_input` → thrustMap (en entraînement seulement)

2. **Connexions contextuelles** (carte ↔ carte)
   - 6 connexions bidirectionnelles entre les 3 cartes
   - Permettent à chaque carte d'utiliser le contexte des autres
   - Rayon contextuel : `r_contextual = 0.075`

**Visuel suggéré :**
- Schéma 3 cartes + connexions externes (bleu) vs. contextuelles (rouge)

#### 2.3 Application au contrôle de fusée (1 min)

**À dire :**
> "En entraînement, les 3 cartes reçoivent les données réelles. En prédiction, la carte Thrust n'a plus d'entrée externe : elle est entièrement guidée par le contexte des cartes Error et Speed."

**Visuel suggéré :**
- Diagramme flux : entraînement vs. prédiction (une slide, deux colonnes)

---

### 3. IMPLÉMENTATION : 4 EXPÉRIENCES PROGRESSIVES (8 minutes)

**Introduction de cette section (15 sec) :**
> "4 expériences progressives, de l'absence totale de temporalité à une mémoire explicite."

#### 3.1 Expérience 001 : Baseline - Données mélangées (2 min)

**Objectif :** Preuve de concept — CXSOM peut-il apprendre (error, speed) → thrust ?

**Caractéristiques :**
- 3 cartes (Error, Speed, Thrust), données **mélangées aléatoirement**
- Mapping statique : chaque pas de temps indépendant

**Résultats :**
- ✅ CXSOM apprend la relation état → action
- ❌ Pas de dynamique temporelle, ordre chronologique perdu

**Visuel suggéré :** Schéma architecture + heatmap des cartes apprises

#### 3.2 Expérience 002 : Contrôleur temps-réel (2 min)

**Objectif :** Montrer que CXSOM est utilisable en boucle de contrôle temps-réel

**Architecture système :**
```
[Simulation Fusée] ←→ [Contrôleur C++] ←→ [Processeur CXSOM (réseau)]
```

Chaque pas : simulation → état → CXSOM → thrust prédite → simulation

**Apport :**
- ✅ Faisabilité temps-réel démontrée, architecture modulaire
- ⚠️ Modèle encore entraîné sur données mélangées (001)

**Visuel suggéré :** Diagramme système + démo/screenshot simulation

#### 3.3 Expérience 003 : Inline - Données chronologiques (2 min)

**Objectif :** Conserver l'ordre temporel pour capturer implicitement les dépendances

**Innovation :** Données envoyées dans l'ordre t=0, t=1, t=2, ... (plus de mélange)

**Problème découvert :**
- La fusée converge vite → reste dans une zone étroite de l'espace d'état
- Les grandes erreurs sont sous-représentées → **biais d'exploration**
- Sans mémoire explicite, l'ordre seul ne suffit pas

**Résultats :**
- ✅ Cohérence temporelle, vidéos d'apprentissage générées
- ❌ Exploration non uniforme, pas de mémoire réelle

**Visuel suggéré :** Distribution des états visités + frames vidéo d'apprentissage

#### 3.4 Expérience 004 : Architecture récursive (2 min)

**Objectif :** Donner une mémoire temporelle explicite à chaque carte

**Innovation : connexions récursives** — chaque carte reçoit son propre BMU au pas t-1 :
```
t-1:  [ErrorMap]  [SpeedMap]  [ThrustMap]
         ↓ feedback  ↓ feedback  ↓ feedback
t  :  [ErrorMap]  [SpeedMap]  [ThrustMap]
         ↑ error     ↑ speed     ↑ thrust (entraîn. seulement)
```

En prédiction, ThrustMap est guidée par le contexte Error/Speed **et** son propre état précédent.

**Avantages :**
- ✅ Mémoire explicite → capture des trajectoires, pas seulement des états instantanés
- ✅ La décision dépend du passé (même état → actions différentes selon la trajectoire)

**Défis :** gestion du premier pas (pas de t-1), synchronisation des timelines

**Visuel suggéré :** Schéma architecture récursive + comparaison 001 vs. 004

---

### 4. RÉSULTATS ET ANALYSE (2 minutes)

**Tableau comparatif :**

| Expérience | Temporalité | Avantages | Limitations |
|------------|-------------|-----------|-------------|
| **001 - Baseline** | Aucune (mélangé) | Simple, converge bien | Pas de dynamique |
| **002 - Controller** | Aucune (utilise 001) | Temps-réel, modulaire | Idem 001 |
| **003 - Inline** | Implicite (ordre) | Données cohérentes | Exploration biaisée |
| **004 - Recursive** | Explicite (feedback) | Mémoire temporelle | Complexité accrue |

**Montrer (si disponibles) :** heatmaps des cartes apprises, trajectoires de la fusée contrôlée, vidéo d'apprentissage (003-inline/weights.mp4)

---

### 5. DISCUSSION ET PERSPECTIVES (2 minutes)

#### Contributions du projet

1. Approche bio-inspirée data-driven pour le contrôle (sans modèle physique)
2. Exploration systématique de la temporalité en 4 étapes
3. Infrastructure complète : pipeline, visualisation, interface temps-réel
4. Extension originale : architecture récursive CXSOM

#### Limitations et perspectives

**Limites honnêtes :**
- Exploration non uniforme (mode chronologique)
- Validation quantitative à compléter (pas de benchmark PID)

**Court terme :**
- Métriques de performance + comparaison PID
- Stratégies d'exploration améliorées (ε-greedy, départs aléatoires variés)

**Plus long terme :** récursion profonde (t-2, t-3), couplage RL, apprentissage en ligne

---

### 6. CONCLUSION (2 minutes)

**À dire :**
> "À travers 4 expériences progressives, ce projet montre que les CXSOM peuvent apprendre une politique de contrôle de fusée, et que l'ajout de connexions récursives est la clé pour capturer les dynamiques temporelles. L'infrastructure est complète et réutilisable, et les pistes pour aller plus loin sont nombreuses."

**Messages clés :**
1. CXSOM = approche viable pour le contrôle data-driven
2. La temporalité est cruciale → architectures adaptées
3. Architecture récursive : contribution originale du projet

**Questions ?**

---

## 🎓 Anticipation des questions du jury

### Questions techniques probables

#### Q1 : "Pourquoi CXSOM plutôt qu'un réseau de neurones classique (MLP, LSTM) ?"

**Réponse :**
- CXSOM préserve la topologie des données (interprétabilité)
- Apprentissage non-supervisé (pas besoin de labels explicites action optimale)
- Connexions contextuelles permettent interactions entre variables
- Inspiration neurobiologique (cortex sensoriel)

#### Q2 : "Combien de données d'entraînement avez-vous utilisé ?"

**Réponse :**
- Données générées par simulation (gdyn::problem::rocket)
- Episodes de 500 pas de temps
- Plusieurs passes sur les données (configurables)
- Espace d'état discret : grille (erreur × vitesse)
- Pas de limitation sur quantité de données simulées

#### Q3 : "Quel est le temps de calcul pour l'entraînement et la prédiction ?"

**Réponse :**
- Entraînement : [minutes/heures selon nombre d'itérations]
- Prédiction temps-réel : [ms par décision]
- Architecture optimisée avec caching
- Processeur CXSOM en serveur séparé

#### Q4 : "Comment gérez-vous l'exploration de l'espace d'état ?"

**Réponse :**
- **Problème identifié** dans exp. 003 : régions stables sur-représentées
- **Solutions possibles** :
  - Données avec départs aléatoires variés
  - Pondération des exemples rares
  - Stratégies d'exploration active
- **Non implémenté** dans version actuelle (perspective)

#### Q5 : "Avez-vous comparé avec un contrôleur PID classique ?"

**Réponse :**
- Pas de comparaison quantitative directe dans ce projet
- **Objectif différent** : apprentissage data-driven vs. contrôle model-based
- **Perspective** : benchmark avec PID, MPC, RL classique
- CXSOM permet adaptation sans re-tunage manuel

#### Q6 : "Comment choisissez-vous les hyperparamètres (SIGMA, ALPHA, etc.) ?"

**Réponse :**
```cpp
MAP_SIZE = 2000         // Résolution de la carte
SIGMA = 0.2             // Largeur voisinage (% de MAP_SIZE)
ALPHA = 0.1             // Taux d'apprentissage
r_external = 0.25       // Rayon connexions externes
r_contextual = 0.075    // Rayon connexions contextuelles
```
- Valeurs initiales basées sur littérature CXSOM
- Ajustements empiriques
- **Perspective** : grid search ou optimisation bayésienne

#### Q7 : "Qu'apporte l'architecture récursive concrètement ?"

**Réponse :**
- **Sans récursion** : décision basée uniquement sur état actuel (error, speed)
- **Avec récursion** : décision influencée par trajectoire passée
- Exemple : même état (e, v) peut nécessiter actions différentes selon si on arrive de gauche ou droite
- Capture des dynamiques : vitesse de variation, tendances

#### Q8 : "Le système est-il stable ? Converge-t-il toujours ?"

**Réponse :**
- Stabilité de l'apprentissage : SOM converge théoriquement
- Stabilité du contrôle : dépend de la qualité des cartes apprises
- **Tests nécessaires** : scénarios variés, perturbations, robustesse
- **Non garanti** : comme tout contrôleur appris (vs. contrôle théorique)

### Questions sur le projet

#### Q9 : "Quel est votre contribution personnelle vs. bibliothèques existantes ?"

**Réponse :**
**Utilisé (bibliothèques) :**
- CXSOM library (infrastructure de base)
- gdyn (physique fusée)
- rllib2 (types énumérables)

**Développé (contribution) :**
- Architecture 3-cartes spécifique au problème
- 4 expériences progressives (design et implémentation)
- **Architecture récursive** (extension originale)
- Pipeline complet entraînement/prédiction
- Interface contrôleur temps-réel
- Scripts de visualisation et analyse

#### Q10 : "Quelles difficultés avez-vous rencontrées ?"

**Réponse :**
**Techniques :**
- Gestion des timelines (t-1) en architecture récursive
- Synchronisation processeur/contrôleur en temps-réel
- Exploration biaisée en mode chronologique

**Conceptuelles :**
- Choix de l'architecture (combien de cartes ?)
- Comment intégrer la temporalité dans CXSOM
- Trade-off exploration vs. exploitation

**Outils :**
- Apprentissage de CXSOM library
- Debugging réseau de processeurs
- Visualisation de cartes 1D

#### Q11 : "Si vous aviez plus de temps, que feriez-vous ?"

**Priorités :**
1. **Validation quantitative** : métriques de performance, comparaisons
2. **Optimisation hyperparamètres** : recherche systématique
3. **Tests de robustesse** : perturbations, bruits, scénarios variés
4. **Architectures alternatives** : plus de cartes, récursion profonde
5. **Application réelle** : test sur système physique (si possible)

---

## 📊 Suggestions de visuels pour les slides (11 slides)

### Slide 1 : Introduction
- Schéma fusée avec variables annotées + timeline des 4 expériences

### Slide 2 : Self-Organizing Maps
- Schéma : entrée → BMU → mise à jour (pas de formule détaillée)

### Slide 3 : Architecture CXSOM
- Schéma 3 cartes avec connexions (externe bleu, contextuel rouge)

### Slide 4 : Pipeline entraînement vs. prédiction
- Deux colonnes : entraînement (3 cartes reçoivent des inputs) vs. prédiction (ThrustMap guidée par contexte)

### Slides 5-8 : Les 4 expériences (une slide chacune)
- Structure : Objectif | Caractéristique clé | Résultats (✅/❌)

### Slide 9 : Résultats comparatifs
- Tableau + visualisations (heatmaps, trajectoires, vidéo)

### Slide 10 : Perspectives
- Limites + court terme + long terme

### Slide 11 : Conclusion
- 3 messages clés + remerciements

---

## ⏱️ Timing détaillé

| Section | Durée | Slides |
|---------|-------|--------|
| Introduction | 2 min | 1 |
| Fondements théoriques | 4 min | 2-4 |
| 4 Expériences | 8 min | 5-8 |
| Résultats | 2 min | 9 |
| Perspectives | 2 min | 10 |
| Conclusion | 2 min | 11 |
| **TOTAL** | **20 min** | **11 slides** |

---

## ✅ Checklist avant la soutenance

### Préparation technique
- [ ] Tester les slides sur le vidéoprojecteur
- [ ] Préparer les démos/vidéos (format compatible)
- [ ] Screenshots de résultats prêts
- [ ] Code important imprimé ou accessible rapidement

### Préparation orale
- [ ] Répéter la présentation à voix haute (chronomètre)
- [ ] Anticiper les questions (voir section ci-dessus)
- [ ] Préparer des réponses aux points faibles du projet
- [ ] Relire le code des 4 expériences (recsom-train.cpp, etc.)

### Documents
- [ ] Rapport écrit finalisé
- [ ] Notes sur fiche (mots-clés, pas de texte intégral)
- [ ] Références bibliographiques prêtes

### Mental
- [ ] Arriver en avance
- [ ] Respirer, parler lentement
- [ ] Faire preuve d'honnêteté sur les limites
- [ ] Montrer l'enthousiasme pour le sujet

---

## 💡 Conseils généraux

1. **Raconter une histoire** : Les 4 expériences forment une progression naturelle vers la solution du problème temporel.

2. **Être pédagogue** : Le jury n'est pas forcément expert en CXSOM. Expliquer clairement les concepts.

3. **Assumer les limites** : Mieux vaut dire "je n'ai pas eu le temps de faire X, mais voici comment je le ferais" que de cacher un problème.

4. **Montrer la rigueur** : Méthodologie scientifique, expériences progressives, analyse critique.

5. **Être passionné** : Le sujet est intéressant ! Montrer que tu as pris plaisir à travailler dessus.

6. **Gérer le temps** : Si tu es en retard, sauter des détails, pas des sections entières.

7. **Interagir avec le jury** : Contact visuel, vérifier qu'ils suivent, adapter le rythme.

---

## 📚 Références utiles à mentionner

- **Kohonen, T.** : Self-Organizing Maps (1995)
- **CXSOM library** : Framework pour cartes contextuelles
- **gdyn** : github.com/HerveFrezza-Buet/gdyn
- **rllib2** : github.com/HerveFrezza-Buet/rllib2

---

**Bonne chance pour ta soutenance ! 🚀**

Le but de cette expérience est de creer une organisation qui permet à cxsom de consulter error et speed, pour renvoyer thrust en temps reelle.
Il faut donc creer une controller similaire à ce que l'on a dans predictions/rocket-test.cpp.

On distingue deux phases : l'entrainement et la prediction.

Durant la phase d'entrainement, les cartes prennent en entrée l'erreur, la vitesse et la poussée. Les données doivent être chronologiquement dans le bon ordre. 
Ainsi, si on envoie un pack de point, il faut que le pack suivant commence au point d'arret du pack précédent. 
Donc, on doit garder en mémoire le point final du premier pack, et constuire des nouvelles données à partir de ce point.

Pour la phase de prediction, on ne fournit plus que l'erreur et la vitesse. Les données d'erreur et de vitesse doivent venir de la bibliothèque qui gere la fusée. 
Le controlleur va devoir lire dans la timeline in l'erreur et la vitesse, les donner à cxsom qui va faire la relaxation des cartes pour fournir la valeur de poussée optimale.
Une fois cette poussée optimale trouvée, le controlleur va l'ecrire dans root-dir/out/predict. Le controlleur de cette fusée va lui appliquer cette nouvelle valeur de poussée à la fusée et remplir à nouveau la timeline in. Et la boucle continue. 


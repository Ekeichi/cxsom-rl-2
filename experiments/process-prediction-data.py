import sys
import traceback
import pycxsom as cx
import numpy as np

root_dir = 'root-dir'
max_points = 3000
def read_full_scalar(var_path: str) -> np.ndarray:
    """Reads all samples of a scalar CXSOM variable as a 1D numpy array."""
    return np.fromiter(
        (float(v) for _, v in cx.variable.data_range_full(var_path)),
        dtype=float
    )

real_error_path  = cx.variable.path_from(root_dir, "in", "error")
real_speed_path  = cx.variable.path_from(root_dir, "in", "speed")

pred_thrust_path = cx.variable.path_from(root_dir, "predict","thrust")

E = read_full_scalar(real_error_path)
S = read_full_scalar(real_speed_path)
pred_T = read_full_scalar(pred_thrust_path)

N = min(len(E), len(S), len(pred_T))
E = E[:N]
S = S[:N]
pred_T = pred_T[:N]

# Paramètres de la grille (doivent correspondre à my_rocket_config.hpp)
nb_errors = 51
min_error, max_error = -100.0, 100.0
nb_speeds = 51
min_speed, max_speed = -100.0, 100.0

# Construction des centres des cases pour erreur et vitesse
error_centers = min_error + (max_error - min_error) * (np.arange(nb_errors) + 0.5) / nb_errors
speed_centers = min_speed + (max_speed - min_speed) * (np.arange(nb_speeds) + 0.5) / nb_speeds

# On construit un arbre k-d pour trouver la prédiction la plus proche des centres de la grille
from scipy.spatial import cKDTree
points = np.column_stack((E, S))
tree = cKDTree(points)

output_filename = "predictions/predicted-controller.dat"

with open(output_filename, "w") as f:
    # La double boucle (erreur puis vitesse) correspond à l'ordre enum::pair<error, speed> en C++
    for e in error_centers:
        for s in speed_centers:
            # Recherche du point d'entraînement/prédiction le plus proche
            _, idx = tree.query([e, s])
            
            # Récupère la poussée correspondante (selon le SOM)
            thrust = pred_T[idx] * 15
            
            # Optionnel: On force la poussée à 0 ou 15 pour être propre envers le C++
            if thrust > 7.5:
                thrust = 15.0
            else:
                thrust = 0.0
                
            # Écriture de la ligne pour le C++:
            # Le C++ attend 5 valeurs par ligne (dont 4 sont ignorées pour juste récupérer 'thrust')
            f.write(f"{e:.4f} {s:.4f} {thrust} 0 0\n")

print(f"Fichier '{output_filename}' généré avec succès avec {nb_errors * nb_speeds} lignes.")



import sys
import pycxsom as cx
import numpy as np

if len(sys.argv) < 2:
    print(f'Usage : {sys.argv[0]} <root-dir>')
    sys.exit(0)

root_dir  = sys.argv[1]

# On remplace 'pos-ref' par 'state-ref'
with cx.variable.Realize(cx.variable.path_from(root_dir, 'calibration', 'state-ref')) as state_ref:
    state_ref[0] = .5
    
# On remplace 'rgb-ref' par 'thrust-ref' et on met une valeur Scalaire (0.5) au lieu d'un vecteur RGB
with cx.variable.Realize(cx.variable.path_from(root_dir, 'calibration', 'thrust-ref')) as thrust_ref:
    thrust_ref[0] = .5
    
# On remplace 'pos-samples' par 'state-samples'
with cx.variable.Realize(cx.variable.path_from(root_dir, 'calibration', 'state-samples')) as state_samples:
    grid_side = state_samples.datatype.side
    # Échantillonnage linéaire de 0 à 1
    state_samples[0] = np.linspace(0, 1, grid_side)
    
# On remplace 'rgb-samples' par 'thrust-samples'
with cx.variable.Realize(cx.variable.path_from(root_dir, 'calibration', 'thrust-samples')) as thrust_samples:
    # thrust-samples a une taille de side*side dans le C++, on récupère cette taille
    total_size = thrust_samples.datatype.side 
    # On remplit simplement avec un linspace de 0 à 1 (plus besoin de logique RGB complexe)
    thrust_samples[0] = np.linspace(0, 1, total_size)
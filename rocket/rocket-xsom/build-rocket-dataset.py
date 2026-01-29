import sys
import os
import numpy as np
import pycxsom as cx

def normalize_minmax(v):
    """
    Strict MinMax normalization to [0, 1] range.
    This ensures compatibility with sigma=0.075 in xsom.cpp.
    """
    v_min, v_max = v.min(), v.max()
    if v_max - v_min == 0:
        print(f"Warning: constant column detected (min=max={v_min})")
        return np.zeros_like(v)
    return (v - v_min) / (v_max - v_min)

if len(sys.argv) < 2:
    print(f"Usage : {sys.argv[0]} <root-dir>")
    sys.exit(1)

root_dir = sys.argv[1]
data_file = "data/rocket-discrete-controller.dat"

if not os.path.exists(data_file):
    print(f"Error: {data_file} not found.")
    sys.exit(1)

# Chargement et préparation
raw_data = np.loadtxt(data_file)
np.random.shuffle(raw_data)  # Important pour éviter les patterns séquentiels

# Normalisation stricte MinMax [0, 1]
errors = normalize_minmax(raw_data[:, 0])
velocities = normalize_minmax(raw_data[:, 1])
thrusts = normalize_minmax(raw_data[:, 2])

print(f"Data Loaded. Shape: {raw_data.shape}")
print(f"Error range after normalization: [{errors.min():.4f}, {errors.max():.4f}]")
print(f"Velocity range after normalization: [{velocities.min():.4f}, {velocities.max():.4f}]")
print(f"Thrust range after normalization: [{thrusts.min():.4f}, {thrusts.max():.4f}]")

# Sauvegarde des paramètres de normalisation pour dénormalisation ultérieure
norm_params = {
    'error_min': float(raw_data[:, 0].min()),
    'error_max': float(raw_data[:, 0].max()),
    'velocity_min': float(raw_data[:, 1].min()),
    'velocity_max': float(raw_data[:, 1].max()),
    'thrust_min': float(raw_data[:, 2].min()),
    'thrust_max': float(raw_data[:, 2].max())
}
np.save('data/normalization_params.npy', norm_params)
print(f"Normalization parameters saved to data/normalization_params.npy")

# Écriture simple (les fichiers existent déjà grâce au C++)
def write_var(name, data):
    path = cx.variable.path_from(root_dir, 'img', name)
    with cx.variable.Realize(path) as v:
        v[0] = data
        print(f"Written {name}")

write_var('error_data', errors)
write_var('velocity_data', velocities)
write_var('thrust_data', thrusts)
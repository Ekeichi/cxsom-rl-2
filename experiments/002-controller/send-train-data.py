import pycxsom as cx
import numpy as np
import argparse
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--data-file', help='Path to the training data file', required=True)
parser.add_argument('--root-dir', help='Path to the root directory', required=True)
parser.add_argument('--nb-passes', help='We pass all the data NB_PASSES times.', type=int, required=True)
parser.add_argument('--smoothing', help='If you want to smooth the thrust', type=lambda x: str(x).lower() in ['true', '1', 'yes'], required=True)
args = parser.parse_args()

data_file = Path(args.data_file)
root_dir = Path(args.root_dir)

raw_data = np.loadtxt(data_file)
    
def normalize_minmax(v):
    v_min, v_max = v.min(), v.max()
    if v_max - v_min == 0:
        print(f"Warning: constant column detected (min=max={v_min})")
        return np.zeros_like(v)
    return (v - v_min) / (v_max - v_min)

def smooth_grid_data(v, kernel_size=5):
    """
    Applique une moyenne mobile 2D pour transformer une frontière binaire
    (0 ou 1) en une zone de transition continue.
    On suppose que les données sont une grille carrée parfaite.
    """
    grid_size = int(np.sqrt(len(v)))
    if grid_size * grid_size != len(v):
        print(f"Warning: data length ({len(v)}) is not a perfect square square. No smoothing applied.")
        return v
    
    grid = v.reshape(grid_size, grid_size)
    smoothed = np.zeros_like(grid, dtype=float)
    offset = kernel_size // 2
    
    for i in range(grid_size):
        for j in range(grid_size):
            i_min, i_max = max(0, i - offset), min(grid_size, i + offset + 1)
            j_min, j_max = max(0, j - offset), min(grid_size, j + offset + 1)
            smoothed[i, j] = np.mean(grid[i_min:i_max, j_min:j_max])
            
    return smoothed.flatten()

error_var_path  = cx.variable.path_from(root_dir, 'in', 'error')
speed_var_path  = cx.variable.path_from(root_dir, 'in', 'speed')
thrust_var_path = cx.variable.path_from(root_dir, 'in', 'thrust')

# Lissage des données binaire pour créer une zone de transition continue (thrust)
if (args.smoothing):
    raw_data[:, 2] = smooth_grid_data(raw_data[:, 2], kernel_size=5)

with cx.variable.Realize(error_var_path) as error:
    with cx.variable.Realize(speed_var_path) as speed:
        with cx.variable.Realize(thrust_var_path) as thrust:
            for i in range(args.nb_passes):
                np.random.shuffle(raw_data)
                error_data = normalize_minmax(raw_data[:, 0])
                speed_data = normalize_minmax(raw_data[:, 1])
                thrust_data = normalize_minmax(raw_data[:, 2])
                for val in error_data:
                    error += float(val)
                for val in speed_data:
                    speed += float(val)
                for val in thrust_data:
                    thrust += float(val)

print("Dataset feeding for training complete.")

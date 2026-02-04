import sys
import pycxsom as cx
import numpy as np

data_file = 'data/rocket-discrete-controller.dat'
root_dir  = 'root-dir'

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

raw_data = np.loadtxt(data_file)
np.random.shuffle(raw_data)

error    = normalize_minmax(raw_data[:, 0])
velocity = normalize_minmax(raw_data[:, 1])
thrust   = normalize_minmax(raw_data[:, 2])

n = raw_data.shape[0] 

path = cx.variable.path_from(root_dir, 'in', 'src')
with cx.variable.Realize(path, t=cx.typing.Array(3), cache_size=3, file_size=n) as v:
    for i in range(n):
        v[i] = np.array((float(error[i]), float(velocity[i]), float(thrust[i])), dtype=float)

print("Dataset building complete.")

import os 
import sys
import pycxsom as cx
import numpy as np

data_file = 'data/rocket-discrete-controller.dat'
root_dir = 'root-dir'

raw_data = np.loadtxt(data_file)
np.random.shuffle(raw_data)

def normalize_minmax(v):
    v_min, v_max = v.min(), v.max()
    if v_max - v_min == 0:
        print(f"Warning: constant column detected (min=max={v_min})")
        return np.zeros_like(v)
    return (v - v_min) / (v_max - v_min)

error_var_path  = cx.variable.path_from(root_dir, 'in', 'predict_error')
speed_var_path  = cx.variable.path_from(root_dir, 'in', 'predict_speed')

with cx.variable.Realize(error_var_path) as error:
    with cx.variable.Realize(speed_var_path) as speed:
            error_data = normalize_minmax(raw_data[:, 0])
            speed_data = normalize_minmax(raw_data[:, 1])
            for val in error_data:
                error += float(val)
            for val in speed_data:
                speed += float(val)

print("Dataset feeding for prediction complete.")

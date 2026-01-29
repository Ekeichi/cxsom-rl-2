import sys
import time
import os
import pycxsom as cx
import numpy as np
import matplotlib.pyplot as plt

if len(sys.argv) < 2:
    print(f'Usage : {sys.argv[0]} <root-dir>')
    sys.exit(0)

root_dir = sys.argv[1]

def get_real_path(cx_path):
    if cx_path.endswith('.var'): return cx_path
    return cx_path + '.var'

# --- CONFIGURATION DES NOMS ---
thrust_real_path = cx.variable.path_from(root_dir, 'img', 'thrust_data')
# NOUVEAU NOM ICI :
pred_path_var    = cx.variable.path_from(root_dir, 'predict-out', 'predicted-thrust')
idx_path_var     = cx.variable.path_from(root_dir, 'predict-out', 'index')

thrust_file_check = get_real_path(thrust_real_path)
pred_file_check   = get_real_path(pred_path_var)

print("--- Rocket Prediction Viewer ---")

# 1. Chargement Vérité Terrain
print(f"Loading Ground Truth...")
if not os.path.exists(thrust_file_check):
    print(f"Error: {thrust_file_check} not found.")
    sys.exit(1)
try:
    with cx.variable.Realize(thrust_real_path) as v:
        real_thrust_map = v[0]
except Exception as e:
    print(f"Error reading ground truth: {e}")
    sys.exit(1)

# 2. Attente Fichier
print(f"Waiting for prediction file: {pred_file_check}")
file_ready = False
for i in range(20):
    if os.path.exists(pred_file_check):
        file_ready = True
        break
    time.sleep(1)
    print(".", end='', flush=True)
print()

if not file_ready:
    print("\nError: Prediction file not created. Did you run 'make predict'?")
    sys.exit(1)

# 3. Attente Données
print("File found. Waiting for data...")
with cx.variable.Realize(pred_path_var) as v_pred, cx.variable.Realize(idx_path_var) as v_idx:
    data_ready = False
    for i in range(20):
        r = v_pred.time_range()
        if r is not None and r[1] > 0: 
            data_ready = True
            break
        time.sleep(0.5)
        print("o", end='', flush=True)
    print()
    
    if not data_ready:
        print(f"Error: No data found in {pred_file_check} (time_range={r}).")
        print("Tip: Restart the processor (make cxsom-kill-processor && make cxsom-launch-processor)")
        sys.exit(1)

    count = r[1] + 1
    print(f"Success! Reading {count} predictions.")
    preds = np.array(v_pred[0:count])
    idxs  = np.array(v_idx[0:count])

# 4. Affichage
pixel_indices = (idxs * len(real_thrust_map)).astype(int)
pixel_indices = np.clip(pixel_indices, 0, len(real_thrust_map)-1)
targets = real_thrust_map[pixel_indices]

data = np.column_stack((targets, preds))
data = data[data[:, 0].argsort()] # Tri

def moving_average(a, n=20):
    if len(a) < n: return a
    ret = np.cumsum(a, dtype=float)
    ret[n:] = ret[n:] - ret[:-n]
    return ret[n - 1:] / n

plt.figure(figsize=(10, 6))
plt.title(f"Rocket Thrust Prediction (Test on {count} samples)")
plt.plot(data[:, 0], color='black', linewidth=2, label='Real Thrust (Target)')
plt.scatter(range(len(data)), data[:, 1], color='orange', s=5, alpha=0.3, label='Predicted Samples')

if len(data) > 50:
    smoothed = moving_average(data[:, 1], n=50)
    plt.plot(np.arange(len(smoothed)) + 25, smoothed, color='red', linewidth=2, label='Prediction Trend')

plt.xlabel('Test Samples (sorted by target thrust)')
plt.ylabel('Thrust Value (normalized)')
plt.legend()
plt.grid(True, alpha=0.3)
plt.show()
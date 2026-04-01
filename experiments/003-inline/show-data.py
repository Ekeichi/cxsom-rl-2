import pycxsom as cx
import numpy as np
import matplotlib.pyplot as plt
import argparse
from pathlib import Path

parser = argparse.ArgumentParser(description="Affiche error et speed depuis le root-dir cxsom")
parser.add_argument('--root-dir', default='root-dir', help='Chemin vers le root directory cxsom')
args = parser.parse_args()

root_dir = Path(args.root_dir)

error_path = cx.variable.path_from(root_dir, 'in', 'error')
speed_path = cx.variable.path_from(root_dir, 'in', 'speed')

# data_range_full ignore automatiquement les slots Busy (value = None)
error_data = [(at, v) for at, v in cx.variable.data_range_full(error_path) if v is not None]
speed_data = [(at, v) for at, v in cx.variable.data_range_full(speed_path) if v is not None]

if not error_data or not speed_data:
    print("Aucune donnée disponible dans le root-dir.")
    exit(0)

t_e, E = zip(*error_data)
t_s, S = zip(*speed_data)
t_e, E = np.array(t_e), np.array(E)
t_s, S = np.array(t_s), np.array(S)

print(f"Error : {len(E)} pas lus  (t={t_e[0]}..{t_e[-1]})")
print(f"Speed : {len(S)} pas lus  (t={t_s[0]}..{t_s[-1]})")

# --- Plot ---
fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(12, 6), sharex=False)
fig.suptitle("Data", fontsize=14)

ax1.plot(t_e, E, color='royalblue', linewidth=1)
ax1.axhline(0, color='gray', linestyle='--', linewidth=0.8)
ax1.set_ylabel("Error")
ax1.set_title("Error")
ax1.grid(True, alpha=0.3)

ax2.plot(t_s, S, color='tomato', linewidth=1)
ax2.axhline(0, color='gray', linestyle='--', linewidth=0.8)
ax2.set_ylabel("Speed")
ax2.set_xlabel("Time")
ax2.set_title("Speed")
ax2.grid(True, alpha=0.3)

plt.tight_layout()  
plt.show()

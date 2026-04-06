import sys
import tkinter as tk
import pycxsom as cx
import matplotlib.pyplot as plt
import numpy as np
import argparse
from pathlib import Path

MAP_SIZE = 40

def normalize_minmax(v):
    v_min, v_max = v.min(), v.max()
    if v_max - v_min == 0:
        print(f"Warning: colonne constante (min=max={v_min})")
        return np.zeros_like(v)
    return (v - v_min) / (v_max - v_min)

parser = argparse.ArgumentParser()
parser.add_argument('--target-map', help='La carte à visualiser (error, speed, thrust)', required=True)
parser.add_argument('--root-dir',   help='Chemin vers le root directory', required=True)
parser.add_argument('--data-file',  help='Chemin vers le fichier de données', required=True)
args = parser.parse_args()

root_dir  = Path(args.root_dir)
data_file = Path(args.data_file)
target_map = args.target_map

if target_map not in ['error', 'speed', 'thrust']:
    print(f"Erreur : '{target_map}' n'est pas une carte valide. Choisissez parmi: error, speed, thrust.")
    sys.exit(1)

CONTEXT = {
    'error':  ('speed', 'thrust'),
    'speed':  ('thrust', 'error'),
    'thrust': ('error', 'speed')
}

COLORS = {
    'error': 'blue',
    'speed': 'orange',
    'thrust': 'green'
}

# Chargement des données d'entrée
raw_data    = np.loadtxt(data_file)
error_data  = normalize_minmax(raw_data[:, 0])
speed_data  = normalize_minmax(raw_data[:, 1])
thrust_data = normalize_minmax(raw_data[:, 2])
min_len     = min(len(error_data), len(speed_data), len(thrust_data))
error_data  = error_data[:min_len]
speed_data  = speed_data[:min_len]
thrust_data = thrust_data[:min_len]
print(f"Chargement terminé ({min_len} points).")


def deref_pos2d(pos, weights_flat):
    """Convertit une position Pos2D (cx, cy) en valeur scalaire dans la carte cible."""
    cx_coord = float(pos[0])
    cy_coord = float(pos[1])
    i = int(np.clip(round(cx_coord * (MAP_SIZE - 1)), 0, MAP_SIZE - 1))
    j = int(np.clip(round(cy_coord * (MAP_SIZE - 1)), 0, MAP_SIZE - 1))
    return weights_flat[i * MAP_SIZE + j]


class Surface3DView(cx.tkviewer.At):
    def __init__(self, master, map_name='error', figsize=(8, 6), dpi=90):
        super().__init__(master, map_name, figsize, dpi)
        self.map_name = map_name
        self.ctx0, self.ctx1 = CONTEXT[map_name]

        # Chemins vers les poids (wgt = poids courants, save = snapshots périodiques)
        self.We_path      = cx.variable.path_from(root_dir, 'wgt', f'{self.map_name}/We-0')
        self.We_ctx0_path = cx.variable.path_from(root_dir, 'wgt', f'{self.ctx0}/We-0')
        self.We_ctx1_path = cx.variable.path_from(root_dir, 'wgt', f'{self.ctx1}/We-0')
        self.Wc0_path     = cx.variable.path_from(root_dir, 'wgt', f'{self.map_name}/Wc-0')
        self.Wc1_path     = cx.variable.path_from(root_dir, 'wgt', f'{self.map_name}/Wc-1')

    def on_draw_at(self, at):
        self.fig.clear()
        ax = self.fig.add_subplot(111, projection='3d')

        # Nuage de points des données d'entrée en fond
        ax.scatter(error_data, speed_data, thrust_data,
                   s=1, c='gray', alpha=0.05, label='Input Space', rasterized=True)

        try:
            with cx.variable.Realize(self.We_path) as We:
                Xe = np.array(We[at]).flatten()          # (MAP_SIZE*MAP_SIZE,)
            with cx.variable.Realize(self.We_ctx0_path) as We:
                Xe_ctx0 = np.array(We[at]).flatten()     # (MAP_SIZE*MAP_SIZE,)
            with cx.variable.Realize(self.We_ctx1_path) as We:
                Xe_ctx1 = np.array(We[at]).flatten()     # (MAP_SIZE*MAP_SIZE,)
            with cx.variable.Realize(self.Wc0_path) as Wc0:
                Xc0 = np.array(Wc0[at]).reshape(-1, 2)  # (MAP_SIZE*MAP_SIZE, 2) : positions Pos2D
            with cx.variable.Realize(self.Wc1_path) as Wc1:
                Xc1 = np.array(Wc1[at]).reshape(-1, 2)  # (MAP_SIZE*MAP_SIZE, 2)
        except Exception as e:
            ax.set_title(f"Pas de données pour le pas #{at}\n({e})")
            return

        n = MAP_SIZE * MAP_SIZE
        E = np.zeros(n)
        S = np.zeros(n)
        T = np.zeros(n)

        for p in range(n):
            val  = float(Xe[p])
            val0 = deref_pos2d(Xc0[p], Xe_ctx0)  # déréférencement Pos2D → valeur scalaire
            val1 = deref_pos2d(Xc1[p], Xe_ctx1)

            coords = {self.map_name: val, self.ctx0: val0, self.ctx1: val1}
            E[p] = coords['error']
            S[p] = coords['speed']
            T[p] = coords['thrust']

        # Reshape en grille 2D pour plot_surface
        E2 = E.reshape(MAP_SIZE, MAP_SIZE)
        S2 = S.reshape(MAP_SIZE, MAP_SIZE)
        T2 = T.reshape(MAP_SIZE, MAP_SIZE)

        c = COLORS.get(self.map_name, 'blue')
        ax.plot_surface(E2, S2, T2,
                        color=c, alpha=0.5,
                        linewidth=0.2, edgecolor='k',
                        label=f"Map Surface '{self.map_name}'")

        ax.set_xlabel('error')
        ax.set_ylabel('speed')
        ax.set_zlabel('thrust')
        ax.set_title(f"Surface 2D '{self.map_name}' dans l'espace (error, speed, thrust) — Pas #{at}")
        ax.legend()


# Fenêtre Tkinter
root = tk.Tk()
root.title(f"Visualisation surface 2D : {target_map}")
root.protocol('WM_DELETE_WINDOW', lambda: sys.exit(0))
root.geometry('900x700')

slider = cx.tkviewer.HistoryFromVariableSlider(
    root,
    'Timestep',
    cx.variable.path_from(root_dir, 'wgt', f'{target_map}/We-0')
)
slider.widget().pack(side=tk.TOP, fill=tk.X, padx=4, pady=2)

v_surface = Surface3DView(root, map_name=target_map)
v_surface.widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=4, pady=4)

v_surface.set_history_slider(slider)

tk.mainloop()

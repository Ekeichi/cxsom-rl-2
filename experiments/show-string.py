
import tkinter as tk
import pycxsom as cx
import matplotlib.pyplot as plt
import numpy as np
import argparse
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--target-map', help='The map we are interested in', required=True)
parser.add_argument('--root-dir', help='Path to the root directory', required=True)
args = parser.parse_args()

root_dir = Path(args.root_dir)
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

def array_of_var(path):
    # Lecture de toutes les valeurs de la variable
    return np.fromiter(
        (float(v) for _, v in cx.variable.data_range_full(path)),
        dtype=float
    )

# Chargement des données d'entrée
in_error  = array_of_var(cx.variable.path_from(root_dir, 'in', 'error'))
in_speed  = array_of_var(cx.variable.path_from(root_dir, 'in', 'speed'))
in_thrust = array_of_var(cx.variable.path_from(root_dir, 'in', 'thrust'))

# S'assurer qu'elles ont la même longueur
min_len = min(len(in_error), len(in_speed), len(in_thrust))
in_error  = in_error[:min_len]
in_speed  = in_speed[:min_len]
in_thrust = in_thrust[:min_len]
print(f"Chargement terminé ({min_len} points).")


class String3DView(cx.tkviewer.At):
    def __init__(self, master, map_name='error', figsize=(8, 6), dpi=90):
        super().__init__(master, map_name, figsize, dpi)
        self.map_name = map_name
        self.ctx0, self.ctx1 = CONTEXT[map_name]
        
        # Chemins vers les poids
        self.We_path       = cx.variable.path_from(root_dir, 'wgt', f'{self.map_name}/We-0')
        self.We_ctx0_path  = cx.variable.path_from(root_dir, 'wgt', f'{self.ctx0}/We-0')
        self.We_ctx1_path  = cx.variable.path_from(root_dir, 'wgt', f'{self.ctx1}/We-0')

        self.Wc0_path = cx.variable.path_from(root_dir, 'wgt', f'{self.map_name}/Wc-0')
        self.Wc1_path = cx.variable.path_from(root_dir, 'wgt', f'{self.map_name}/Wc-1')

    def on_draw_at(self, at):
        self.fig.clear()
        
        # Ajout d'un axe 3D
        ax = self.fig.add_subplot(111, projection='3d')
        
        # 1. Tracé des entrées en fond (nuage de points)
        ax.scatter(in_error, in_speed, in_thrust, s=2, c='gray', alpha=0.05, label='Input Space', rasterized=True)
        
        # Récupération des poids au pas de temps donné 'at'
        try:
            with cx.variable.Realize(self.We_path) as We:
                Xe = We[at]
            with cx.variable.Realize(self.We_ctx0_path) as We:
                Xe_ctx0 = We[at]
            with cx.variable.Realize(self.We_ctx1_path) as We:
                Xe_ctx1 = We[at]

            with cx.variable.Realize(self.Wc0_path) as Wc0:
                Xc0 = Wc0[at]
            with cx.variable.Realize(self.Wc1_path) as Wc1:
                Xc1 = Wc1[at]
        except Exception as e:
            ax.set_title(f"Pas de données pour le pas #{at}")
            return

        E, S, T = [], [], []

        for p in range(len(Xe)):
            val = Xe[p]
            #          val[01]     idx max de Xe    
            idx0 = int(Xc0[p] * (len(Xe_ctx0) - 1) + 0.5)
            val0 = Xe_ctx0[idx0]
            
            idx1 = int(Xc1[p] * (len(Xe_ctx1) - 1) + 0.5)
            val1 = Xe_ctx1[idx1]
            
            # Ranger les variables dans le bon axe 3D
            coords = {
                self.map_name: val,
                self.ctx0: val0,
                self.ctx1: val1
            }
            
            E.append(coords['error'])
            S.append(coords['speed'])
            T.append(coords['thrust'])

        # 2. Tracé de la corde / carte 1D en 3D
        c = COLORS.get(self.map_name, 'b')
        ax.plot(E, S, T, marker='o', markersize=4, color=c, linewidth=2, label=f"Map String '{self.map_name}'")
        
        ax.set_xlabel('error')
        ax.set_ylabel('speed')
        ax.set_zlabel('thrust')
        ax.set_title(f"3D Map '{self.map_name}' - Pas #{at}")
        
        # Legend
        ax.legend()


# Configuration de la fenêtre Tkinter principale
root = tk.Tk()
root.title(f"Visualisation 3D temporelle : {target_map}")
root.protocol('WM_DELETE_WINDOW', lambda: sys.exit(0))
root.geometry('900x700')

# Création du slider qui observe l'historique du We-0 de la carte cible
slider = cx.tkviewer.HistoryFromVariableSlider(
    root,
    'Timestep',
    cx.variable.path_from(root_dir, 'wgt', f'{target_map}/We-0')
)
slider.widget().pack(side=tk.TOP, fill=tk.X, padx=4, pady=2)

# Création de la zone de dessin (viewer 3D)
v_string = String3DView(root, map_name=target_map)
v_string.widget().pack(side=tk.TOP, fill=tk.BOTH, expand=True, padx=4, pady=4)

# Connexion du slider à la vue
v_string.set_history_slider(slider)

tk.mainloop()

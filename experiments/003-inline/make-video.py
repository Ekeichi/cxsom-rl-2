import sys
import pycxsom as cx
import numpy as np
import matplotlib.pyplot as plt
import matplotlib.animation as animation
import argparse
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('--root-dir', help='Path to the root directory', required=True)
parser.add_argument('--timeline', help='The timeline where to find the weights', required=True)
parser.add_argument('--output', help='Output video file (.mp4)', default='weights.mp4')
parser.add_argument('--frames', help='Number of frames to render', type=int, default=500)
parser.add_argument('--fps', help='Frames per second', type=int, default=30)
args = parser.parse_args()

root_dir = Path(args.root_dir)

MAP_NAMES = ['error', 'speed', 'thrust']
CONTEXT = {
    'error':  ('speed', 'thrust'),
    'speed':  ('thrust', 'error'),
    'thrust': ('error', 'speed')
}
COLORS = {
    'error': 'tab:blue',
    'speed': 'tab:orange',
    'thrust': 'tab:green'
}

# Création de la figure complète avec 3 graphes
fig, axes = plt.subplots(3, 1, figsize=(8, 10))
fig.suptitle('Évolution des Poids', fontsize=14)

lines = {}
X_axes = {}

# Initialisation de la figure et des données pour la frame 0
for i, map_name in enumerate(MAP_NAMES):
    We_path  = cx.variable.path_from(root_dir, args.timeline, map_name + '/We-0')
    Wc0_path = cx.variable.path_from(root_dir, args.timeline, map_name + '/Wc-0')
    Wc1_path = cx.variable.path_from(root_dir, args.timeline, map_name + '/Wc-1')

    # Lire les dimensions
    with cx.variable.Realize(We_path) as We:
        Xe = np.linspace(0, 1, We.datatype.shape()[0])
    with cx.variable.Realize(Wc0_path) as Wc0:
        Xc0 = np.linspace(0, 1, Wc0.datatype.shape()[0])
    with cx.variable.Realize(Wc1_path) as Wc1:
        Xc1 = np.linspace(0, 1, Wc1.datatype.shape()[0])
        
    X_axes[map_name] = (Xe, Xc0, Xc1)
    
    ax = axes[i]
    ax.set_ylim(0, 1)
    
    c_map = COLORS[map_name]
    c_ctx0 = COLORS[CONTEXT[map_name][0]]
    c_ctx1 = COLORS[CONTEXT[map_name][1]]

    # Lignes vides qui seront mises à jour
    line_We, = ax.plot([], [], c=c_map, label=f'We ({map_name})', linewidth=1.5)
    line_Wc0, = ax.plot([], [], c=c_ctx0, label=f'Wc-0 ({CONTEXT[map_name][0]})', linewidth=1.5)
    line_Wc1, = ax.plot([], [], c=c_ctx1, label=f'Wc-1 ({CONTEXT[map_name][1]})', linewidth=1.5)
    
    lines[map_name] = (line_We, line_Wc0, line_Wc1)
    ax.legend(fontsize=7, loc='upper right')
    ax.tick_params(labelsize=7)

plt.tight_layout()

# Fonction qui sera appelée pour chaque frame
def update(frame):
    print(f"Génération du pas de temps {frame}/{args.frames}...", end='\r')
    for i, map_name in enumerate(MAP_NAMES):
        We_path  = cx.variable.path_from(root_dir, args.timeline, map_name + '/We-0')
        Wc0_path = cx.variable.path_from(root_dir, args.timeline, map_name + '/Wc-0')
        Wc1_path = cx.variable.path_from(root_dir, args.timeline, map_name + '/Wc-1')

        try:
            with cx.variable.Realize(We_path) as We:
                Ye = We[frame]
            with cx.variable.Realize(Wc0_path) as Wc0:
                Yc0 = Wc0[frame]
            with cx.variable.Realize(Wc1_path) as Wc1:
                Yc1 = Wc1[frame]
        except Exception:
             # Si on dépasse la fin de la timeline, les courbes s'arrêtent
             continue

        Xe, Xc0, Xc1 = X_axes[map_name]
        l_We, l_Wc0, l_Wc1 = lines[map_name]
        
        # Mise à jour des valeurs pour la frame en cours
        l_We.set_data(Xe, Ye)
        l_Wc0.set_data(Xc0, Yc0)
        l_Wc1.set_data(Xc1, Yc1)
        axes[i].set_title(f'"{map_name}" — pas #{frame}', fontsize=9)
        
    return [l for tup in lines.values() for l in tup]

anim = animation.FuncAnimation(fig, update, frames=args.frames, blit=False)

print(f"\nCréation de la vidéo en cours... -> {args.output}")

# Attention: nécessite ffmpeg installé sur ton système.
anim.save(args.output, writer='ffmpeg', fps=args.fps)
print("\nVidéo exportée avec succès !")

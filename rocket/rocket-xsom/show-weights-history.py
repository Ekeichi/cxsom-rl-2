import sys
import pycxsom as cx
import numpy as np
import matplotlib.pyplot as plt

if len(sys.argv) < 2:
    print(f'Usage : {sys.argv[0]} <root-dir>')
    sys.exit(0)

root_dir = sys.argv[1]

def get_timeline_path(map_name, weight_name):
    # CORRECTION ICI : On concatène map_name et weight_name avec un '/'
    # La variable s'appelle "Error/We-0" dans la timeline "saved"
    full_var_name = f"{map_name}/{weight_name}"
    return cx.variable.path_from(root_dir, 'saved', full_var_name)

def plot_map_weights(map_name, weight_name, ax, title):
    path = get_timeline_path(map_name, weight_name)
    try:
        # On ouvre le fichier de sauvegarde
        with cx.variable.Realize(path) as history:
            r = history.time_range()
            # Sécurité si l'historique est vide
            if r[1] <= r[0]:
                ax.text(0.5, 0.5, "Empty History", ha='center')
                return

            # On prend 10 instantanés répartis dans le temps
            # On s'assure que les indices sont valides
            nb_steps = 10
            times = np.linspace(r[0], max(r[0], r[1]-1), nb_steps, dtype=int)
            # On dédoublonne si l'historique est court
            times = np.unique(times)
            
            for t in times:
                try:
                    Y = history[t] # Lecture des poids à l'instant t
                    X = np.linspace(0, 1, len(Y))
                    
                    # Plus l'instant est récent, plus la courbe est foncée
                    duration = max(1, r[1] - r[0])
                    alpha = 0.2 + 0.8 * (t - r[0]) / duration
                    ax.plot(X, Y, color='blue', alpha=alpha)
                except Exception:
                    pass
                
            ax.set_title(title)
            ax.set_ylim(0, 1)
            
    except Exception as e:
        print(f"Could not read {path}: {e}")
        ax.text(0.5, 0.5, f"Data not found:\n{map_name}", ha='center')

# Configuration de la fenêtre
fig, axes = plt.subplots(3, 1, figsize=(8, 12))

# 1. Carte Error (State)
plot_map_weights('Error', 'We-0', axes[0], 'Error Map - Input Weights Evolution')

# 2. Carte Velocity (State)
plot_map_weights('Velocity', 'We-0', axes[1], 'Velocity Map - Input Weights Evolution')

# 3. Carte Thrust (Action)
plot_map_weights('Thrust', 'We-0', axes[2], 'Thrust Map - Input Weights Evolution')

plt.tight_layout()
print("Displaying plots...")
plt.show()
import sys
import pycxsom as cx
import numpy as np
import matplotlib.pyplot as plt
from matplotlib.gridspec import GridSpec

root_dir = 'root-dir'
timestep = 15

if len(sys.argv) > 1:
    root_dir = sys.argv[1]
if len(sys.argv) > 2:
    timestep = int(sys.argv[2])

in_timeline  = 'in'
wgt_timeline = 'save'
out_timeline = 'out'

s = 10 # scatter point size

def make_map_paths(map_name):
    return {'We'  : cx.variable.path_from(root_dir, wgt_timeline, f'{map_name}/We-0'),
            'Wc0' : cx.variable.path_from(root_dir, wgt_timeline, f'{map_name}/Wc-0'),
            'Wc1' : cx.variable.path_from(root_dir, wgt_timeline, f'{map_name}/Wc-1'),
            'BMU' : cx.variable.path_from(root_dir, out_timeline, f'{map_name}/BMU')}

def array_of_var(path):
    return np.fromiter(
        (float(v) for _, v in cx.variable.data_range_full(path)),
        dtype=float
    )

def weights_of_var(path, at=timestep):
    with cx.variable.Realize(path) as W:
        try:
            weight = W[at]
        except:
            if W.datatype.shape()[0] > 0:
                weight = W[-1] # fallback to last weight
            else:
                weight = np.array([])
    return weight

def weight_fun_of_var(path, at=timestep):
    weight = weights_of_var(path, at)
    bmu_one_pos = len(weight) - 1 # Beware, last index is bmu == 1.
    if bmu_one_pos <= 0:
        return lambda bmu: 0.0
    return lambda bmu : weight[(int)(bmu * bmu_one_pos + .5)]

MAP_NAMES = ['error', 'speed', 'thrust']

# Wc-0 and Wc-1 mapped inputs explicitly:
CONTEXT = {
    'error':  ('speed', 'thrust'),  # Wc0 from speed, Wc1 from thrust
    'speed':  ('thrust', 'error'),  # Wc0 from thrust, Wc1 from error
    'thrust': ('error', 'speed')    # Wc0 from error, Wc1 from speed
}

COLORS = {
    'error': 'tab:blue',
    'speed': 'tab:orange',
    'thrust': 'tab:green'
}

paths = {m: make_map_paths(m) for m in MAP_NAMES}
paths['IN'] = {m: cx.variable.path_from(root_dir, in_timeline, m) for m in MAP_NAMES}

INs = {m: array_of_var(paths['IN'][m]) for m in MAP_NAMES}
BMUs = {m: array_of_var(paths[m]['BMU']) for m in MAP_NAMES}

min_len = min(*[len(INs[m]) for m in MAP_NAMES], *[len(BMUs[m]) for m in MAP_NAMES])
if min_len == 0:
    print("No data found to plot!")
    sys.exit(0)

for m in MAP_NAMES:
    INs[m] = INs[m][:min_len]
    BMUs[m] = BMUs[m][:min_len]

We = {m: weights_of_var(paths[m]['We']) for m in MAP_NAMES}
Wc0 = {m: weights_of_var(paths[m]['Wc0']) for m in MAP_NAMES}
Wc1 = {m: weights_of_var(paths[m]['Wc1']) for m in MAP_NAMES}
We_fun = {m: weight_fun_of_var(paths[m]['We']) for m in MAP_NAMES}

MAP = np.linspace(0, 1, len(We['error'])) if len(We['error']) > 0 else np.linspace(0, 1, 100)

def plot_inputs(ax):
    ax.set_title('Inputs Manifold')
    ax.set_xlabel('error')
    ax.set_ylabel('speed')
    ax.set_zlabel('thrust')
    ax.scatter(INs['error'], INs['speed'], INs['thrust'], s=s, alpha=0.5)

def plot_input_fit(ax, map_name):
    ax.set_title(f'{map_name} fit')
    ax.set_xlabel(f"{map_name} (in)")
    ax.set_ylabel(f"We(bmu)")
    IN  = INs[map_name]
    W   = We_fun[map_name]
    BMU = BMUs[map_name]
    FIN = [W(bmu) for bmu in BMU]
    ax.scatter(IN, FIN, s = s)

def plot_map_match(ax, map_name):
    c_map = COLORS[map_name]
    c_ctx0 = COLORS[CONTEXT[map_name][0]]
    c_ctx1 = COLORS[CONTEXT[map_name][1]]

    ax.set_title(f'map {map_name} weights')
    ax.set_xlabel('pos')
    ax.plot(MAP, We[map_name], c=c_map, alpha=.8, label=f'We ({map_name})', linewidth=2)
    ax.plot(MAP, Wc0[map_name], c=c_ctx0, alpha=.8, label=f'Wc0 ({CONTEXT[map_name][0]})', linewidth=2)
    ax.plot(MAP, Wc1[map_name], c=c_ctx1, alpha=.8, label=f'Wc1 ({CONTEXT[map_name][1]})', linewidth=2)
    
    A = INs[map_name]
    B0 = INs[CONTEXT[map_name][0]]
    B1 = INs[CONTEXT[map_name][1]]
    BMU = BMUs[map_name]
    alpha = .15
    ax.scatter(BMU, A, c=c_map, alpha = alpha, s = s, label = f"in {map_name}")
    ax.scatter(BMU, B0, c=c_ctx0, alpha = alpha, s = s, label = f"in {CONTEXT[map_name][0]}")
    ax.scatter(BMU, B1, c=c_ctx1, alpha = alpha, s = s, label = f"in {CONTEXT[map_name][1]}")
    ax.scatter(BMU, np.zeros_like(BMU) - .1, alpha = 0.5, color='k', s = s, label = 'BMU')
    ax.legend(fontsize='small', loc='upper left', bbox_to_anchor=(1.05, 1))

def plot_in_space(ax, map_name):
    ax.set_title(f'map {map_name} in input space')
    ax.set_xlabel('error')
    ax.set_ylabel('speed')
    ax.set_zlabel('thrust')
    I = np.argsort(BMUs[map_name])
    B_err = BMUs['error'][I]
    B_spd = BMUs['speed'][I]
    B_thr = BMUs['thrust'][I]
    
    W_err = We_fun['error']
    W_spd = We_fun['speed']
    W_thr = We_fun['thrust']
    
    # Original data points (light)
    ax.scatter(INs['error'], INs['speed'], INs['thrust'], s=s, alpha=.05, zorder=0)
    
    # Mapped 1D map in 3D space
    X = np.array([W_err(bmu) for bmu in B_err])
    Y = np.array([W_spd(bmu) for bmu in B_spd])
    Z = np.array([W_thr(bmu) for bmu in B_thr])
    
    ax.plot(X, Y, Z, c='k', alpha=.6, linewidth=2, zorder=1)
    ax.scatter(X, Y, Z, s=s*2, c='red', zorder=2)

fig = plt.figure(figsize=(10,9), constrained_layout=True)
gs = GridSpec(3, 1, figure=fig)

ax = fig.add_subplot(gs[0, 0])
plot_map_match(ax, 'error')

ax = fig.add_subplot(gs[1, 0])
plot_map_match(ax, 'speed')

ax = fig.add_subplot(gs[2, 0])
plot_map_match(ax, 'thrust')
plt.show()

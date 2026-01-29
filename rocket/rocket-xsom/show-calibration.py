import sys
import pycxsom as cx
import numpy as np
import matplotlib.pyplot as plt

if len(sys.argv) < 2:
    print(f'Usage : {sys.argv[0]} <root-dir>')
    sys.exit(0)

root_dir  = sys.argv[1]

fig = plt.figure(figsize=(10, 8))

# 1. Matching State (anciennement pos)
plt.subplot(3,1,1)
with cx.variable.Realize(cx.variable.path_from(root_dir, 'calibration', 'state-match')) as state_match:
    Y = state_match[0]
    side = len(Y)
X = np.linspace(0, 1, side)
plt.title('Matching curve for State (Error/Velocity)')
plt.xticks([])
plt.plot(X, Y)

# 2. Matching Context
plt.subplot(3,1,2)
with cx.variable.Realize(cx.variable.path_from(root_dir, 'calibration', 'ctx-match')) as ctx_match:
    Y = ctx_match[0]
    side = len(Y) # Recalcul de la taille au cas où
X = np.linspace(0, 1, side)
plt.title('Matching curve for Context')
plt.xticks([])
plt.plot(X, Y)

# 3. Matching Thrust (anciennement RGB) - C'est maintenant un plot 2D simple
plt.subplot(3,1,3)
with cx.variable.Realize(cx.variable.path_from(root_dir, 'calibration', 'thrust-match')) as thrust_match:
    Y = thrust_match[0]
    side = len(Y)
X = np.linspace(0, 1, side)
plt.title('Matching curve for Thrust')
plt.xlabel('Value (0-1)')
plt.plot(X, Y)

plt.tight_layout()
plt.show()


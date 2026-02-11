import sys
import traceback
import pycxsom as cx
import numpy as np
import matplotlib.pyplot as plt
import tkinter as tk

if len(sys.argv) < 2:
    print(f'Usage : {sys.argv[0]} <root-dir>')
    sys.exit(0)

root_dir = sys.argv[1]
max_points = int(sys.argv[2]) if len(sys.argv) >= 3 else 3000

def read_full_scalar(var_path: str) -> np.ndarray:
    """Reads all samples of a scalar CXSOM variable as a 1D numpy array."""
    return np.fromiter(
        (float(v) for _, v in cx.variable.data_range_full(var_path)),
        dtype=float
    )

# Paths (train inputs used to visualize data in 3D space)
err_path = cx.variable.path_from(root_dir, "in", "error")
spd_path = cx.variable.path_from(root_dir, "in", "speed")
thr_path = cx.variable.path_from(root_dir, "in", "thrust")

# Load full history of each variable
E = read_full_scalar(err_path)
S = read_full_scalar(spd_path)
T = read_full_scalar(thr_path)

N = min(len(E), len(S), len(T))
E, S, T = E[:N], S[:N], T[:N]

# Subsample if too many points
if N > max_points:
    indices = np.random.choice(N, size=max_points, replace=False)
    E, S, T = E[    indices], S[indices], T[indices]

class Rocket3DView(cx.tkviewer.At):
    def __init__(self, master, E, S, T, figsize=(10, 10), dpi=100):
        super().__init__(master, "Rocket inputs (error, speed, thrust)", figsize, dpi)
        self.E = E
        self.S = S
        self.T = T

    def on_draw_at(self, at):
        try:
            self.fig.clear()
            ax = self.fig.add_subplot(projection="3d")
            ax.set_xlabel("error")
            ax.set_ylabel("speed")
            ax.set_zlabel("thrust")
            ax.scatter(self.E, self.S, self.T, s=6)
            
            # IMPORTANT: force canvas draw
            self.canevas.draw_idle()
        except Exception:
            traceback.print_exc()

root = tk.Tk()
root.protocol("WM_DELETE_WINDOW", lambda: sys.exit(0))

viewer = Rocket3DView(root, E, S, T)
viewer.widget().pack(fill=tk.BOTH, side=tk.TOP)

# We force an initial draw manually by calling the method once.
root.after(100, lambda: viewer.on_draw_at(0))

tk.mainloop()
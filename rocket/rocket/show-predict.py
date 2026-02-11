import sys
import traceback
import pycxsom as cx
import numpy as np
import matplotlib.pyplot as plt
import tkinter as tk

root_dir = 'root-dir'
max_points = 3000
def read_full_scalar(var_path: str) -> np.ndarray:
    """Reads all samples of a scalar CXSOM variable as a 1D numpy array."""
    return np.fromiter(
        (float(v) for _, v in cx.variable.data_range_full(var_path)),
        dtype=float
    )

real_error_path  = cx.variable.path_from(root_dir, "in", "error")
real_speed_path  = cx.variable.path_from(root_dir, "in", "speed")
real_thrust_path = cx.variable.path_from(root_dir, "in","thrust")

pred_thrust_path = cx.variable.path_from(root_dir, "predict","thrust")

E = read_full_scalar(real_error_path)
S = read_full_scalar(real_speed_path)
real_T = read_full_scalar(real_thrust_path)
pred_T = read_full_scalar(pred_thrust_path)

N = min(len(E), len(S), len(real_T), len(pred_T))
E, S, real_T, pred_T = E[:N], S[:N], real_T[:N], pred_T[:N]

if N > max_points:
    indices = np.random.choice(N, size=max_points, replace=False)
    E = E[indices]
    S = S[indices]
    real_T = real_T[indices]
    pred_T = pred_T[indices]

# Calcul de la loss (MSE)
loss = np.mean((real_T - pred_T)**2)


class Rocket3DView(cx.tkviewer.At):
    def __init__(self, master, E, S, real_T, pred_T, loss, figsize=(10, 10), dpi=100):
        super().__init__(master, f"Rocket inputs (MSE Loss: {loss:.6f})", figsize, dpi)
        self.E = E
        self.S = S
        self.real_T = real_T
        self.pred_T = pred_T

    def on_draw_at(self, at):
        try:
            self.fig.clear()
            ax = self.fig.add_subplot(projection="3d")
            ax.set_xlabel("error")
            ax.set_ylabel("speed")
            ax.set_zlabel("thrust")
            
            # Afficher les données réelles en bleu
            ax.scatter(self.E, self.S, self.real_T, s=6, c='blue', label='Real', alpha=0.6)
            
            # Afficher les prédictions en rouge
            ax.scatter(self.E, self.S, self.pred_T, s=6, c='red', label='Predicted', alpha=0.6)
            
            # Ajouter une légende
            ax.legend()
            
            # IMPORTANT: force canvas draw
            self.canevas.draw_idle()
        except Exception:
            traceback.print_exc()

root = tk.Tk()
root.protocol("WM_DELETE_WINDOW", lambda: sys.exit(0))

viewer = Rocket3DView(root, E, S, real_T, pred_T, loss)
viewer.widget().pack(fill=tk.BOTH, side=tk.TOP)

# We force an initial draw manually by calling the method once.
root.after(100, lambda: viewer.on_draw_at(0))

tk.mainloop()


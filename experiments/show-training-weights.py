import sys
import pycxsom as cx
import numpy as np
import tkinter as tk

root_dir ="root-dir"

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

class MapView(cx.tkviewer.At):
    def __init__(self, master, map_name, figsize=(5, 3.5), dpi=90):
        super().__init__(master, map_name, figsize, dpi)
        self.map_name = map_name

        self.We_path  = cx.variable.path_from(root_dir, 'wgt', map_name + '/We-0')
        self.Wc0_path = cx.variable.path_from(root_dir, 'wgt', map_name + '/Wc-0')
        self.Wc1_path = cx.variable.path_from(root_dir, 'wgt', map_name + '/Wc-1')

        with cx.variable.Realize(self.We_path) as We:
            self.Xe = np.linspace(0, 1, We.datatype.shape()[0])
        with cx.variable.Realize(self.Wc0_path) as Wc0:
            self.Xc0 = np.linspace(0, 1, Wc0.datatype.shape()[0])
        with cx.variable.Realize(self.Wc1_path) as Wc1:
            self.Xc1 = np.linspace(0, 1, Wc1.datatype.shape()[0])

    def on_draw_at(self, at):
        self.fig.clear()
        ax = self.fig.gca()

        with cx.variable.Realize(self.We_path) as We:
            Ye = We[at]
        with cx.variable.Realize(self.Wc0_path) as Wc0:
            Yc0 = Wc0[at]
        with cx.variable.Realize(self.Wc1_path) as Wc1:
            Yc1 = Wc1[at]

        ax.set_ylim(0, 1)
        ax.set_title('"{}" — pas #{}'.format(self.map_name, at), fontsize=9)
        c_map = COLORS[self.map_name]
        c_ctx0 = COLORS[CONTEXT[self.map_name][0]]
        c_ctx1 = COLORS[CONTEXT[self.map_name][1]]

        ax.plot(self.Xe,  Ye,  c=c_map,  label=f'We ({self.map_name})',  linewidth=1.5)
        ax.plot(self.Xc0, Yc0, c=c_ctx0, label=f'Wc-0 ({CONTEXT[self.map_name][0]})', linewidth=1.5)
        ax.plot(self.Xc1, Yc1, c=c_ctx1, label=f'Wc-1 ({CONTEXT[self.map_name][1]})', linewidth=1.5)
        ax.legend(fontsize=7, loc='upper right')
        ax.tick_params(labelsize=7)


root = tk.Tk()
root.title('Visualisation des poids')
root.protocol('WM_DELETE_WINDOW', lambda: sys.exit(0))

screen_w = root.winfo_screenwidth()
screen_h = root.winfo_screenheight()
win_w    = min(screen_w, 1200)
win_h    = min(screen_h - 80, 800)
root.geometry('{}x{}'.format(win_w, win_h))

slider = cx.tkviewer.HistoryFromVariableSlider(
    root,
    'time instants',
    cx.variable.path_from(root_dir, 'wgt', 'error/We-0')
)
slider.widget().pack(side=tk.TOP, fill=tk.X, padx=4, pady=2)

row1 = tk.Frame(root)
row1.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

row2 = tk.Frame(root)
row2.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

v_error  = MapView(row1, 'error')
v_speed  = MapView(row1, 'speed')
v_thrust = MapView(row2, 'thrust')

v_error.widget().pack (side=tk.LEFT,  fill=tk.BOTH, expand=True, padx=2, pady=2)
v_speed.widget().pack (side=tk.LEFT,  fill=tk.BOTH, expand=True, padx=2, pady=2)
v_thrust.widget().pack(side=tk.LEFT,  fill=tk.BOTH, expand=True, padx=2, pady=2)

for v in (v_error, v_speed, v_thrust):
    v.set_history_slider(slider)

tk.mainloop()

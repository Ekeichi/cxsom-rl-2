import numpy as np

try:
    data = np.loadtxt('data/rocket-discrete-controller.dat')
    print("Data shape:", data.shape)
    print("Col 0 (Error) min/max:", data[:, 0].min(), data[:, 0].max())
    print("Col 1 (Velocity) min/max:", data[:, 1].min(), data[:, 1].max())
    print("Col 2 (Thrust) min/max:", data[:, 2].min(), data[:, 2].max())
    print("Col 2 (Thrust) unique values:", np.unique(data[:, 2]))
except Exception as e:
    print(e)

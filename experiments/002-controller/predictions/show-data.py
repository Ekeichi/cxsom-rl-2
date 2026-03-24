import matplotlib.pyplot as plt
import numpy as np
data = []
for line in open("rocket-orbit.dat"):
    data.append([float(elem) for elem in line.split()])

data = np.array(data)

plt.plot(data[...,0],data[...,1])
plt.show()
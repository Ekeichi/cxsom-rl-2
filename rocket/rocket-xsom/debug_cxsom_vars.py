print("Start Script")
import sys
print("Imported sys")
try:
    import pycxsom as cx
    print("Imported pycxsom")
except Exception as e:
    print(f"Failed import: {e}")
    sys.exit(1)

import numpy as np
print("Imported numpy")

if len(sys.argv) < 2:
    print(f'Usage : {sys.argv[0]} <root-dir>')
    sys.exit(0)

root_dir = sys.argv[1]
print(f"Root dir: {root_dir}")

name = "error_data"
timeline = "img"
path = cx.variable.path_from(root_dir, timeline, name)
print(f"Path: {path}")

try:
    with cx.variable.Realize(path) as v:
        print(f"Type: {v.datatype}")
        r = v.time_range()
        print(f"Time Range: {r}")
except Exception as e:
    print(f"Error accessing var: {e}")

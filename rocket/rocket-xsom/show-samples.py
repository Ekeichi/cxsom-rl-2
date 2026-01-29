import sys
import pycxsom as cx
import numpy as np
import matplotlib.pyplot as plt

if len(sys.argv) < 8:
    print(f'Usage : {sys.argv[0]} <root-dir> <w-timeline> <w-name> <h-timeline> <h-name> <rgb-timeline> <rgb-name> [frame_id]')
    sys.exit(0)

root_dir       = sys.argv[1]
w_timeline     = sys.argv[2]
w_varname      = sys.argv[3]
h_timeline     = sys.argv[4]
h_varname      = sys.argv[5]
rgb_timeline   = sys.argv[6]
rgb_varname    = sys.argv[7]
frame_id = None
if len(sys.argv) == 9:
    frame_id = int(sys.argv[8])

with cx.variable.Realize(cx.variable.path_from(root_dir, w_timeline, w_varname)) as var:
    plot_range = var.time_range()

# Helper function to extract scalar value from cxsom output
# cxsom Map1D<Scalar> returns a numpy array of shape (1,) or similar for scalars in 1D maps.
# We need to extract the float value.
def extract_scalar(gen):
    for _, value in gen:
        # If value is a numpy array or list, try to get the first element
        if hasattr(value, '__getitem__') and hasattr(value, 'size'):
             for x in value.flat:
                 yield float(x)
        else:
            yield float(value)

Error = np.fromiter(extract_scalar(cx.variable.data_range_full(cx.variable.path_from(root_dir, w_timeline  , w_varname  ))), float)
Velocity = np.fromiter(extract_scalar(cx.variable.data_range_full(cx.variable.path_from(root_dir, h_timeline  , h_varname  ))), float)
Thrust = np.fromiter(extract_scalar(cx.variable.data_range_full(cx.variable.path_from(root_dir, rgb_timeline, rgb_varname))), float)

print(f"Loaded {len(Error)} points.")
if len(Error) > 0:
    print(f"Range Error: {Error.min()}-{Error.max()}")
if len(Velocity) > 0:
    print(f"Range Velocity: {Velocity.min()}-{Velocity.max()}")
if len(Thrust) > 0:
    print(f"Range Thrust: {Thrust.min()}-{Thrust.max()}")

plt.figure(figsize=(10,10))
if frame_id is None:
    plt.title(f'Inputs in {plot_range}')
plt.xlim(0,1)
plt.ylim(0,1)
plt.xlabel('Error')
plt.ylabel('Velocity')
if len(Error) > 0 and len(Error) == len(Velocity):
    if len(Thrust) == len(Error):
        plt.scatter(Error, 1 - Velocity, c=Thrust, cmap='viridis')
    else:
        plt.scatter(Error, 1 - Velocity)
plt.colorbar(label='Thrust')
if frame_id is None:
    plt.show()
else:
    filename = 'frame-{:06d}.png'.format(frame_id)
    plt.savefig(filename, bbox_inches='tight')
    print(f'image "{filename}" generated.')

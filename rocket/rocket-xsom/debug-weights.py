import sys
import pycxsom as cx
import os

if len(sys.argv) < 2:
    print(f'Usage : {sys.argv[0]} <root-dir>')
    sys.exit(0)

root_dir = sys.argv[1]

def check_var(name, path_parts):
    full_path = cx.variable.path_from(root_dir, *path_parts)
    print(f"--- Checking {name} ---")
    print(f"Path: {full_path}")
    
    if not os.path.exists(full_path + '.var'):
        print("❌ FILE NOT FOUND on disk.")
        return

    try:
        with cx.variable.Realize(full_path) as v:
            print(f"✅ Status: Readable")
            print(f"   Type: {v.datatype}")
            tr = v.time_range()
            print(f"   Time Range: {tr}")
            if tr[1] > 0:
                print(f"   Last available iteration: {tr[1]-1}")
            else:
                print("   ⚠️  File is empty (0 records).")
    except Exception as e:
        print(f"❌ ERROR reading file: {e}")

# Vérifions un poids Scalaire (We) et un poids Position (Wc)
check_var("External Weights (Scalar)", ['saved', 'Error/We-0'])
check_var("Contextual Weights (Pos)",  ['saved', 'Error/Wc-0'])
check_var("External Weights (Scalar)", ['saved', 'Error/We-0'])

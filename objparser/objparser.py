import pywavefront
import sys
import argparse

parser = argparse.ArgumentParser()
parser.add_argument("obj", help="wavefront file")
# parser.add_argument("-c", "--color", nargs="?", const="default", help="override color")
parser.add_argument("-r", "--reflectiveness", nargs="?", const="default", help="set reflectiveness")

args = parser.parse_args()

scene = pywavefront.Wavefront(args.obj)

def printVec(v):
    return f"{v[0]} {v[1]} {v[2]}"

for name, mat in scene.materials.items():
    if mat.vertex_format != "V3F":
        print("Warning: Unknown format!")
        break

    print(f"// {name}")
    pts = list(zip(*(iter(mat.vertices),) * 3))
    tris = zip(*(iter(pts),) * 3)
    reflectiveness = 0.0
    if args.reflectiveness is not None and args.reflectiveness != "default":
        reflectiveness = float(args.reflectiveness)

    for tri in tris:
        print(f"triangle a {printVec(tri[0])} b {printVec(tri[1])} c {printVec(tri[2])} color {printVec(mat.diffuse)} specular {mat.specular[3]} color {printVec(mat.specular)} shininess {mat.shininess} opacity {mat.transparency} reflectiveness {reflectiveness}")



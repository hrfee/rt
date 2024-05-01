import csv, sys

pairs = {}
sah = {}
header = []

with open(sys.argv[1], "r") as f:
    csvf = csv.reader(f)
    for l in csvf:
        if l[0] == "Map Name":
            header = l
            continue
        
        mode = l[14]
        rt = l[10]
        bt = l[16]
        bd = l[15]

        if "SAH" in mode:
            if bd not in sah:
                sah[bd] = {"lines": [], "r": 0, "b": 0}

            sah[bd]["lines"].append(l)
            sah[bd]["r"] += float(rt)
            sah[bd]["b"] += float(bt)
        else:
            if bd not in pairs:
                pairs[bd] = {"lines": [], "r": 0, "b": 0}

            pairs[bd]["lines"].append(l)
            pairs[bd]["r"] += float(rt)
            pairs[bd]["b"] += float(bt)

print(sah)
print ("CC to render time")

for cc in sah:
    l = sah[cc]["lines"][0]
    avg = sah[cc]["r"] / len(sah[cc]["lines"])
    avgb = sah[cc]["b"] / len(sah[cc]["lines"])
    print(f"SAH & {l[15]} & {l[10]} & {l[16]}\\\\")
    print("\\midrule")

for cc in pairs:
    l = pairs[cc]["lines"][0]
    avg = pairs[cc]["r"] / len(pairs[cc]["lines"])
    avgb = pairs[cc]["b"] / len(pairs[cc]["lines"])
    print(f"Voxel & {l[15]} & {l[10]} & {l[16]}\\\\")
    print("\\midrule")


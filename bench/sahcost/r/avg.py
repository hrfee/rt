import csv, sys

pairs = {}
none = {}
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
        fp = l[18]

        if mode == "":
            if "none" not in none:
                none["none"] = {"lines": [], "r": 0}

            none["none"]["lines"].append(l)
            none["none"]["r"] += float(rt)
        else:
            print("FP", fp)
            if fp not in pairs:
                pairs[fp] = {"lines": [], "r": 0, "b": 0}

            pairs[fp]["lines"].append(l)
            pairs[fp]["r"] += float(rt)
            pairs[fp]["b"] += float(bt)

print([len(none[n]["lines"]) for n in none])
print ("CC to render time")

for cc in none:
    l = none[cc]["lines"][0]
    avg = none[cc]["r"] / len(none[cc]["lines"])
    print(f"None & N/A & {avg} & N/A\\\\")
    print("\\midrule")

for cc in pairs:
    l = pairs[cc]["lines"][0]
    avg = pairs[cc]["r"] / len(pairs[cc]["lines"])
    avgb = pairs[cc]["b"] / len(pairs[cc]["lines"])
    print(f"SAH & {cc} & {avg} & {avgb}\\\\")
    print("\\midrule")


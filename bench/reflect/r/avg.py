import csv, sys

pairs = {}

with open(sys.argv[1], "r") as f:
    csvf = csv.reader(f)
    for l in csvf:
        ray = l[4]
        time = l[10]
        if ray == "Max Ray bounce" or time == "Render time (ms)": continue

        if ray not in pairs:
            pairs[ray] = []
        pairs[ray].append(float(time))

avgs = {}

print("Max no. of ray bounces, Average render time (ms)")

for ray in pairs:
    avgs[ray] = sum(pairs[ray])/len(pairs[ray])
    print(f"{ray},{avgs[ray]}")




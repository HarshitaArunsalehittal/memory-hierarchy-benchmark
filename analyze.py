"""analyze.py - runs membench, parses the CSV, prints a table and saves plots.
Usage:  python3 analyze.py
Needs:  pip install matplotlib   (only for the plot; the table works without it)
"""
import csv, io, subprocess, sys

def human(b):
    for unit in ("B", "KB", "MB", "GB"):
        if b < 1024:
            return f"{b:g}{unit}"
        b /= 1024

def main():
    subprocess.run(["gcc", "-O2", "-o", "membench", "membench.c"], check=True)
    out = subprocess.run(["./membench"], stdout=subprocess.PIPE, universal_newlines=True, check=True).stdout
    rows = list(csv.DictReader(io.StringIO(out)))
    with open("results.csv", "w") as f:
        f.write(out)

    data = {"bandwidth": [], "latency": []}
    for r in rows:
        data[r["test"]].append((int(r["size_bytes"]), float(r["value"])))

    print(f"{'Size':>8} {'Bandwidth (GB/s)':>18} {'Latency (ns)':>14}")
    for (s, bw), (_, lat) in zip(data["bandwidth"], data["latency"]):
        print(f"{human(s):>8} {bw:>18.2f} {lat:>14.2f}")

    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt
    except ImportError:
        print("\nmatplotlib not installed; run: pip install matplotlib")
        return
    fig, ax = plt.subplots(1, 2, figsize=(11, 4))
    for a, key, label in ((ax[0], "bandwidth", "Read bandwidth (GB/s)"),
                          (ax[1], "latency", "Load latency (ns)")):
        xs, ys = zip(*data[key])
        a.plot(xs, ys, marker="o")
        a.set_xscale("log")
        a.set_xlabel("Working-set size (bytes)")
        a.set_ylabel(label)
        a.grid(True, which="both", alpha=0.3)
    fig.suptitle("Memory hierarchy: bandwidth and latency vs working-set size")
    fig.tight_layout()
    fig.savefig("memory_hierarchy.png", dpi=150)
    print("\nSaved results.csv and memory_hierarchy.png")

if __name__ == "__main__":
    sys.exit(main())

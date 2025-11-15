import csv
import os
import subprocess
from pathlib import Path

import matplotlib.pyplot as plt


WORKLOADS = [
    (10, 0),
    (8, 2),
    (6, 4),
    (4, 6),
    (2, 8),
    (0, 10),
]
POLICIES = ["lru", "mru"]


def run_benchmark(binary, policy, read_w, write_w, buffers, pages, operations):
    mix = f"{read_w}:{write_w}"
    cmd = [
        binary,
        "--mix",
        mix,
        "--policy",
        policy,
        "--buffers",
        str(buffers),
        "--pages",
        str(pages),
        "--ops",
        str(operations),
    ]
    proc = subprocess.run(
        cmd, capture_output=True, text=True, check=True, cwd=binary.parent
    )
    line = proc.stdout.strip().splitlines()[-1]
    fields = line.split(",")
    labels = [
        "policy",
        "read_weight",
        "write_weight",
        "buffers",
        "pages",
        "ops",
        "logical_reads",
        "logical_writes",
        "physical_reads",
        "physical_writes",
        "page_fixes",
        "dirty_marks",
        "elapsed_ms",
    ]
    return dict(zip(labels, fields))


def main():
    repo_root = Path(__file__).resolve().parents[1]
    tools_dir = repo_root / "toydb" / "tools"
    results_dir = repo_root / "results"
    results_dir.mkdir(exist_ok=True)

    binary_name = "pf_benchmark.exe" if os.name == "nt" else "pf_benchmark"
    binary = tools_dir / binary_name
    if not binary.exists():
        raise SystemExit(
            f"Benchmark binary {binary} not found. Build toydb/tools first."
        )

    all_rows = []
    for policy in POLICIES:
        for read_w, write_w in WORKLOADS:
            stats = run_benchmark(
                binary,
                policy,
                read_w,
                write_w,
                buffers=80,
                pages=400,
                operations=12000,
            )
            stats["read_ratio"] = (
                0 if (read_w + write_w) == 0 else (read_w / (read_w + write_w)) * 100
            )
            all_rows.append(stats)

    csv_path = results_dir / "pf_stats.csv"
    with csv_path.open("w", newline="") as fp:
        writer = csv.writer(fp)
        header = [
            "policy",
            "read_weight",
            "write_weight",
            "buffers",
            "pages",
            "ops",
            "logical_reads",
            "logical_writes",
            "physical_reads",
            "physical_writes",
            "page_fixes",
            "dirty_marks",
            "elapsed_ms",
            "read_ratio",
        ]
        writer.writerow(header)
        for row in all_rows:
            writer.writerow([row[h] for h in header])

    plt.figure(figsize=(8, 4.5))
    for policy in POLICIES:
        xs = []
        ys = []
        for row in all_rows:
            if row["policy"] != policy:
                continue
            xs.append(float(row["read_ratio"]))
            io = float(row["physical_reads"]) + float(row["physical_writes"])
            ys.append(io)
        plt.plot(xs, ys, marker="o", label=policy.upper())

    plt.xlabel("Read ratio (%)")
    plt.ylabel("Physical I/O operations")
    plt.title("PF Buffer Manager Physical I/O vs Read/Write Mix")
    plt.grid(True, linestyle="--", linewidth=0.5)
    plt.legend()
    plt.tight_layout()
    plot_path = results_dir / "pf_stats.png"
    plt.savefig(plot_path, dpi=200)
    print(f"Wrote PF statistics to {csv_path}")
    print(f"Saved plot to {plot_path}")


if __name__ == "__main__":
    main()


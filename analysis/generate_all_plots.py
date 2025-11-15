#!/usr/bin/env python3
"""
generate_all_plots.py - Generate all visualization plots from CSV files

This script runs all plotting scripts to generate visualizations for:
1. PF layer statistics (read/write mix performance)
2. Index construction metrics (three methods comparison)
3. Space utilization metrics (slotted-page vs static layouts)

Run this script after generating all CSV files to create comprehensive visualizations.
"""

import subprocess
import sys
from pathlib import Path

def main():
    repo_root = Path(__file__).resolve().parents[1]
    analysis_dir = repo_root / "analysis"
    results_dir = repo_root / "results"
    
    print("=" * 60)
    print("Generating All Visualization Plots")
    print("=" * 60)
    
    scripts = [
        ("PF Statistics Plot", "pf_stats_plot.py"),
        ("Index Metrics Plot", "index_metrics_plot.py"),
        ("Space Metrics Plot", "space_metrics_plot.py"),
    ]
    
    for name, script in scripts:
        script_path = analysis_dir / script
        if not script_path.exists():
            print(f"\nWarning: {script} not found, skipping...")
            continue
        
        print(f"\n[{name}] Running {script}...")
        try:
            result = subprocess.run(
                [sys.executable, str(script_path)],
                cwd=analysis_dir,
                check=True,
                capture_output=True,
                text=True
            )
            print(result.stdout)
            if result.stderr:
                print("Warnings:", result.stderr)
        except subprocess.CalledProcessError as e:
            print(f"Error running {script}:")
            print(e.stdout)
            print(e.stderr)
        except FileNotFoundError:
            print(f"Error: Python not found. Please run: python3 {script_path}")
    
    print("\n" + "=" * 60)
    print("Plot Generation Complete!")
    print("=" * 60)
    print(f"\nGenerated plots are saved in: {results_dir}")
    print("\nAvailable plots:")
    
    plot_files = [
        "pf_stats.png",
        "index_metrics.png",
        "index_metrics_logical.png",
        "space_metrics.png",
        "space_metrics_percent.png"
    ]
    
    for plot_file in plot_files:
        plot_path = results_dir / plot_file
        if plot_path.exists():
            print(f"  ✓ {plot_file}")
        else:
            print(f"  ✗ {plot_file} (not generated)")

if __name__ == "__main__":
    main()


#!/usr/bin/env python3
"""
index_metrics_plot.py - Generate visualizations for index construction metrics

This script reads index_metrics.csv and generates plots comparing:
- Build time for different indexing methods
- Query time for different indexing methods
- Physical I/O operations for build and query phases
- Page accesses for different methods
"""

import csv
import matplotlib.pyplot as plt
from pathlib import Path
import numpy as np

def main():
    repo_root = Path(__file__).resolve().parents[1]
    results_dir = repo_root / "results"
    csv_path = results_dir / "index_metrics.csv"
    
    if not csv_path.exists():
        print(f"Error: {csv_path} not found. Run index_benchmark first.")
        return
    
    # Read CSV data
    methods = []
    build_times = []
    query_times = []
    build_physical_io = []
    query_physical_io = []
    build_logical_reads = []
    query_logical_reads = []
    
    with csv_path.open('r') as fp:
        reader = csv.DictReader(fp)
        for row in reader:
            method = row['method']
            phase = row['phase']
            
            if phase == 'build':
                methods.append(method)
                build_times.append(float(row['elapsed_ms']))
                build_physical_io.append(int(row['physical_reads']) + int(row['physical_writes']))
                build_logical_reads.append(int(row['logical_reads']))
            elif phase == 'query':
                query_times.append(float(row['elapsed_ms']))
                query_physical_io.append(int(row['physical_reads']) + int(row['physical_writes']))
                query_logical_reads.append(int(row['logical_reads']))
    
    # Create figure with subplots
    fig, axes = plt.subplots(2, 2, figsize=(14, 10))
    fig.suptitle('Index Construction Methods Performance Comparison', fontsize=16, fontweight='bold')
    
    x_pos = np.arange(len(methods))
    colors = ['#2ecc71', '#e74c3c', '#3498db']  # Green, Red, Blue
    
    # Plot 1: Build Time Comparison
    ax1 = axes[0, 0]
    bars1 = ax1.bar(x_pos, build_times, color=colors, alpha=0.8, edgecolor='black', linewidth=1.2)
    ax1.set_xlabel('Indexing Method', fontsize=11, fontweight='bold')
    ax1.set_ylabel('Build Time (ms)', fontsize=11, fontweight='bold')
    ax1.set_title('Index Build Time Comparison', fontsize=12, fontweight='bold')
    ax1.set_xticks(x_pos)
    ax1.set_xticklabels([m.capitalize() for m in methods], fontsize=10)
    ax1.grid(True, axis='y', linestyle='--', linewidth=0.5, alpha=0.7)
    # Add value labels on bars
    for i, (bar, val) in enumerate(zip(bars1, build_times)):
        height = bar.get_height()
        ax1.text(bar.get_x() + bar.get_width()/2., height,
                f'{val:.2f}ms',
                ha='center', va='bottom', fontsize=9, fontweight='bold')
    
    # Plot 2: Query Time Comparison
    ax2 = axes[0, 1]
    bars2 = ax2.bar(x_pos, query_times, color=colors, alpha=0.8, edgecolor='black', linewidth=1.2)
    ax2.set_xlabel('Indexing Method', fontsize=11, fontweight='bold')
    ax2.set_ylabel('Query Time (ms)', fontsize=11, fontweight='bold')
    ax2.set_title('Query Time Comparison (500 queries)', fontsize=12, fontweight='bold')
    ax2.set_xticks(x_pos)
    ax2.set_xticklabels([m.capitalize() for m in methods], fontsize=10)
    ax2.grid(True, axis='y', linestyle='--', linewidth=0.5, alpha=0.7)
    # Add value labels on bars
    for i, (bar, val) in enumerate(zip(bars2, query_times)):
        height = bar.get_height()
        ax2.text(bar.get_x() + bar.get_width()/2., height,
                f'{val:.3f}ms',
                ha='center', va='bottom', fontsize=9, fontweight='bold')
    
    # Plot 3: Physical I/O Comparison (Build Phase)
    ax3 = axes[1, 0]
    bars3 = ax3.bar(x_pos, build_physical_io, color=colors, alpha=0.8, edgecolor='black', linewidth=1.2)
    ax3.set_xlabel('Indexing Method', fontsize=11, fontweight='bold')
    ax3.set_ylabel('Physical I/O Operations', fontsize=11, fontweight='bold')
    ax3.set_title('Physical I/O During Build Phase', fontsize=12, fontweight='bold')
    ax3.set_xticks(x_pos)
    ax3.set_xticklabels([m.capitalize() for m in methods], fontsize=10)
    ax3.grid(True, axis='y', linestyle='--', linewidth=0.5, alpha=0.7)
    # Add value labels on bars
    for i, (bar, val) in enumerate(zip(bars3, build_physical_io)):
        height = bar.get_height()
        ax3.text(bar.get_x() + bar.get_width()/2., height,
                f'{val:,}',
                ha='center', va='bottom', fontsize=9, fontweight='bold')
    
    # Plot 4: Physical I/O Comparison (Query Phase)
    ax4 = axes[1, 1]
    bars4 = ax4.bar(x_pos, query_physical_io, color=colors, alpha=0.8, edgecolor='black', linewidth=1.2)
    ax4.set_xlabel('Indexing Method', fontsize=11, fontweight='bold')
    ax4.set_ylabel('Physical I/O Operations', fontsize=11, fontweight='bold')
    ax4.set_title('Physical I/O During Query Phase (500 queries)', fontsize=12, fontweight='bold')
    ax4.set_xticks(x_pos)
    ax4.set_xticklabels([m.capitalize() for m in methods], fontsize=10)
    ax4.grid(True, axis='y', linestyle='--', linewidth=0.5, alpha=0.7)
    # Add value labels on bars
    for i, (bar, val) in enumerate(zip(bars4, query_physical_io)):
        height = bar.get_height()
        ax4.text(bar.get_x() + bar.get_width()/2., height,
                f'{val}',
                ha='center', va='bottom', fontsize=9, fontweight='bold')
    
    plt.tight_layout()
    plot_path = results_dir / "index_metrics.png"
    plt.savefig(plot_path, dpi=200, bbox_inches='tight')
    print(f"Saved index metrics plot to {plot_path}")
    
    # Create a second figure for logical reads comparison
    fig2, (ax5, ax6) = plt.subplots(1, 2, figsize=(14, 5))
    fig2.suptitle('Logical Reads Comparison', fontsize=16, fontweight='bold')
    
    # Logical reads - Build
    ax5.bar(x_pos, build_logical_reads, color=colors, alpha=0.8, edgecolor='black', linewidth=1.2)
    ax5.set_xlabel('Indexing Method', fontsize=11, fontweight='bold')
    ax5.set_ylabel('Logical Reads', fontsize=11, fontweight='bold')
    ax5.set_title('Logical Reads During Build Phase', fontsize=12, fontweight='bold')
    ax5.set_xticks(x_pos)
    ax5.set_xticklabels([m.capitalize() for m in methods], fontsize=10)
    ax5.grid(True, axis='y', linestyle='--', linewidth=0.5, alpha=0.7)
    for i, val in enumerate(build_logical_reads):
        ax5.text(i, val, f'{val:,}', ha='center', va='bottom', fontsize=9, fontweight='bold')
    
    # Logical reads - Query
    ax6.bar(x_pos, query_logical_reads, color=colors, alpha=0.8, edgecolor='black', linewidth=1.2)
    ax6.set_xlabel('Indexing Method', fontsize=11, fontweight='bold')
    ax6.set_ylabel('Logical Reads', fontsize=11, fontweight='bold')
    ax6.set_title('Logical Reads During Query Phase (500 queries)', fontsize=12, fontweight='bold')
    ax6.set_xticks(x_pos)
    ax6.set_xticklabels([m.capitalize() for m in methods], fontsize=10)
    ax6.grid(True, axis='y', linestyle='--', linewidth=0.5, alpha=0.7)
    for i, val in enumerate(query_logical_reads):
        ax6.text(i, val, f'{val}', ha='center', va='bottom', fontsize=9, fontweight='bold')
    
    plt.tight_layout()
    plot_path2 = results_dir / "index_metrics_logical.png"
    plt.savefig(plot_path2, dpi=200, bbox_inches='tight')
    print(f"Saved logical reads plot to {plot_path2}")

if __name__ == "__main__":
    main()


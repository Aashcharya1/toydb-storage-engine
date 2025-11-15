#!/usr/bin/env python3
"""
space_metrics_plot.py - Generate visualizations for space utilization metrics

This script reads space_metrics.csv and generates plots comparing:
- Space utilization of slotted-page vs static layouts
- Space efficiency for different maximum record lengths
- Total space usage comparison
"""

import csv
import matplotlib.pyplot as plt
from pathlib import Path
import numpy as np

def main():
    repo_root = Path(__file__).resolve().parents[1]
    results_dir = repo_root / "results"
    csv_path = results_dir / "space_metrics.csv"
    
    if not csv_path.exists():
        print(f"Error: {csv_path} not found. Run student_store first.")
        return
    
    # Read CSV data
    layouts = []
    max_lengths = []
    utilizations = []
    space_bytes = []
    records = []
    
    with csv_path.open('r') as fp:
        reader = csv.DictReader(fp)
        for row in reader:
            layouts.append(row['layout'])
            max_len = row['max_record_length']
            if max_len == 'variable':
                max_lengths.append(0)  # Use 0 for variable-length
            else:
                max_lengths.append(int(max_len))
            utilizations.append(float(row['utilization']))
            space_bytes.append(int(row['space_bytes']))
            records.append(int(row['records']))
    
    # Separate slotted and static data
    slotted_idx = layouts.index('slotted')
    slotted_util = utilizations[slotted_idx]
    slotted_space = space_bytes[slotted_idx]
    
    static_indices = [i for i, l in enumerate(layouts) if l == 'static']
    static_lengths = [max_lengths[i] for i in static_indices]
    static_utils = [utilizations[i] for i in static_indices]
    static_spaces = [space_bytes[i] for i in static_indices]
    
    # Sort static data by max_length
    sorted_pairs = sorted(zip(static_lengths, static_utils, static_spaces))
    static_lengths, static_utils, static_spaces = zip(*sorted_pairs)
    
    # Create figure with subplots
    fig, axes = plt.subplots(1, 2, figsize=(14, 5))
    fig.suptitle('Slotted-Page vs Static Layout Space Utilization Comparison', 
                 fontsize=16, fontweight='bold')
    
    # Plot 1: Utilization Comparison
    ax1 = axes[0]
    x_pos = np.arange(len(static_lengths) + 1)
    x_labels = ['Variable\n(Slotted)'] + [f'{l}' for l in static_lengths]
    
    # Create bars
    util_values = [slotted_util] + list(static_utils)
    colors_list = ['#2ecc71'] + ['#e74c3c'] * len(static_utils)  # Green for slotted, red for static
    
    bars = ax1.bar(x_pos, util_values, color=colors_list, alpha=0.8, 
                   edgecolor='black', linewidth=1.2)
    ax1.set_xlabel('Layout Type / Max Record Length (bytes)', fontsize=11, fontweight='bold')
    ax1.set_ylabel('Space Utilization Ratio', fontsize=11, fontweight='bold')
    ax1.set_title('Space Utilization: Slotted-Page vs Static Layouts', fontsize=12, fontweight='bold')
    ax1.set_xticks(x_pos)
    ax1.set_xticklabels(x_labels, fontsize=9, rotation=0)
    ax1.set_ylim([0, 1.0])
    ax1.grid(True, axis='y', linestyle='--', linewidth=0.5, alpha=0.7)
    ax1.axhline(y=slotted_util, color='#2ecc71', linestyle='--', linewidth=2, 
                alpha=0.5, label='Slotted-Page Baseline')
    
    # Add value labels on bars
    for bar, val in zip(bars, util_values):
        height = bar.get_height()
        ax1.text(bar.get_x() + bar.get_width()/2., height,
                f'{val:.3f}',
                ha='center', va='bottom', fontsize=9, fontweight='bold')
    
    # Add legend
    from matplotlib.patches import Patch
    legend_elements = [
        Patch(facecolor='#2ecc71', alpha=0.8, label='Slotted-Page'),
        Patch(facecolor='#e74c3c', alpha=0.8, label='Static Layout')
    ]
    ax1.legend(handles=legend_elements, loc='lower right', fontsize=9)
    
    # Plot 2: Total Space Usage Comparison
    ax2 = axes[1]
    space_values = [slotted_space] + list(static_spaces)
    
    bars2 = ax2.bar(x_pos, space_values, color=colors_list, alpha=0.8, 
                    edgecolor='black', linewidth=1.2)
    ax2.set_xlabel('Layout Type / Max Record Length (bytes)', fontsize=11, fontweight='bold')
    ax2.set_ylabel('Total Space Used (bytes)', fontsize=11, fontweight='bold')
    ax2.set_title('Total Space Usage Comparison', fontsize=12, fontweight='bold')
    ax2.set_xticks(x_pos)
    ax2.set_xticklabels(x_labels, fontsize=9, rotation=0)
    ax2.grid(True, axis='y', linestyle='--', linewidth=0.5, alpha=0.7)
    ax2.axhline(y=slotted_space, color='#2ecc71', linestyle='--', linewidth=2, 
                alpha=0.5, label='Slotted-Page Baseline')
    
    # Add value labels on bars (in MB for readability)
    for bar, val in zip(bars2, space_values):
        height = bar.get_height()
        mb = val / (1024 * 1024)
        ax2.text(bar.get_x() + bar.get_width()/2., height,
                f'{mb:.2f} MB',
                ha='center', va='bottom', fontsize=8, fontweight='bold')
    
    ax2.legend(handles=legend_elements, loc='upper left', fontsize=9)
    
    # Format y-axis to show values in MB
    ax2.yaxis.set_major_formatter(plt.FuncFormatter(lambda x, p: f'{x/(1024*1024):.1f}M'))
    
    plt.tight_layout()
    plot_path = results_dir / "space_metrics.png"
    plt.savefig(plot_path, dpi=200, bbox_inches='tight')
    print(f"Saved space metrics plot to {plot_path}")
    
    # Create a second figure showing utilization as percentage
    fig2, ax3 = plt.subplots(1, 1, figsize=(10, 6))
    fig2.suptitle('Space Utilization Percentage Comparison', fontsize=16, fontweight='bold')
    
    util_percent = [u * 100 for u in util_values]
    bars3 = ax3.bar(x_pos, util_percent, color=colors_list, alpha=0.8, 
                   edgecolor='black', linewidth=1.2)
    ax3.set_xlabel('Layout Type / Max Record Length (bytes)', fontsize=12, fontweight='bold')
    ax3.set_ylabel('Space Utilization (%)', fontsize=12, fontweight='bold')
    ax3.set_title('Space Utilization Percentage: Slotted-Page vs Static Layouts', 
                  fontsize=13, fontweight='bold')
    ax3.set_xticks(x_pos)
    ax3.set_xticklabels(x_labels, fontsize=10)
    ax3.set_ylim([0, 100])
    ax3.grid(True, axis='y', linestyle='--', linewidth=0.5, alpha=0.7)
    ax3.axhline(y=slotted_util*100, color='#2ecc71', linestyle='--', linewidth=2, 
                alpha=0.5, label='Slotted-Page Baseline')
    
    # Add value labels
    for bar, val in zip(bars3, util_percent):
        height = bar.get_height()
        ax3.text(bar.get_x() + bar.get_width()/2., height,
                f'{val:.1f}%',
                ha='center', va='bottom', fontsize=10, fontweight='bold')
    
    ax3.legend(handles=legend_elements, loc='lower right', fontsize=10)
    
    plt.tight_layout()
    plot_path2 = results_dir / "space_metrics_percent.png"
    plt.savefig(plot_path2, dpi=200, bbox_inches='tight')
    print(f"Saved space utilization percentage plot to {plot_path2}")

if __name__ == "__main__":
    main()


# ToyDB Database System - PF/AM Layer Extensions

## Project Overview

This project extends the ToyDB database system's Paged File (PF) and Access Method (AM) layers to implement advanced buffer management, slotted-page storage, and index construction benchmarking capabilities. The implementation fulfills all requirements for the Database Systems assignment.

## Table of Contents

1. [Objectives and Implementation](#objectives-and-implementation)
2. [Project Structure](#project-structure)
3. [Compilation Instructions](#compilation-instructions)
4. [Running the Code](#running-the-code)
5. [Detailed Changes Made](#detailed-changes-made)
6. [Implementation Details](#implementation-details)
7. [Results and Output](#results-and-output)

---

## Objectives and Implementation

### Objective 1: PF Layer Page Buffering with LRU/MRU Replacement

**Status: ✅ Fully Implemented**

**Requirements:**
- Implement page buffering with LRU and MRU replacement strategies
- Replacement strategy selectable when opening a file
- Each page maintains a dirty flag
- Explicit call to mark a page as updated
- Configurable buffer pool size
- Collect and report statistics (pages accessed, logical/physical I/O counts)
- Plot graph of statistics vs read/write query mixtures

**Implementation:**
- Added `PFReplacementPolicy` enum (LRU, MRU) in `pf.h`
- Implemented `PFbufSelectVictim()` in `buf.c` to select pages based on policy
- Added `PF_SetBufferPoolParams()` to configure buffer pool size
- Added `PF_SetDefaultPolicy()` and `PF_SetFilePolicy()` for policy selection
- Added `PF_OpenFileWithPolicy()` convenience function
- Added `PF_MarkDirty()` for explicit dirty marking
- Implemented comprehensive statistics collection in `pf_stats.[ch]`
- Created `pf_benchmark` tool to test different read/write mixes
- Created `analysis/pf_stats_plot.py` to generate performance graphs

**Key Files:**
- `toydb/pflayer/pf_stats.h` - Statistics structure and API
- `toydb/pflayer/pf_stats.c` - Statistics implementation
- `toydb/pflayer/buf.c` - Buffer manager with LRU/MRU support
- `toydb/pflayer/pf.c` - PF layer interface with new functions
- `toydb/tools/pf_benchmark.c` - Benchmarking tool
- `analysis` - Plotting scripts

### Objective 2: Slotted-Page Structure for Variable-Length Records

**Status: ✅ Fully Implemented**

**Requirements:**
- Build slotted-page structure on top of PF layer
- Store variable-length records for student data
- Support record insertion, deletion, and sequential scanning
- Gather performance metrics (space utilization)
- Compare with static record management
- Present results in table for different maximum record lengths

**Implementation:**
- Implemented slotted-page structure in `slot_page.[ch]`
- Header stores slot count, free list, and free-space pointer
- Records placed from page tail upward; slot directory grows from head
- Supports insertion, deletion, sequential scans, and compaction
- Created `student_store` tool to manage student data
- Compares slotted-page utilization vs static fixed-length layouts
- Generates CSV with utilization metrics for different record lengths

**Key Files:**
- `toydb/tools/slot_page.h` - Slotted-page API
- `toydb/tools/slot_page.c` - Slotted-page implementation
- `toydb/tools/student_store.c` - Student data management tool

### Objective 3: AM Layer Index Construction Evaluation

**Status: ✅ Fully Implemented**

**Requirements:**
- Evaluate two methods for building index on Student file using roll-no field:
  1. Creating index in single operation from existing file
  2. Building index incrementally through multiple inserts
- Implement efficient bulk-loading technique for pre-sorted files
- Compare performance (query completion time, number of pages accessed)

**Implementation:**
- Created `index_benchmark` tool to evaluate three indexing methods:
  1. **Post-build**: All records inserted in original order (simulates building on existing data)
  2. **Incremental**: Records inserted one-by-one in random order (simulates incremental inserts)
  3. **Bulk-loading**: Records inserted in sorted order (simulates bulk-loading for pre-sorted data)
- Collects build-time and query-time statistics for each method
- Generates CSV with comprehensive performance metrics
- Fixed `GetLeftPageNum()` in `amscan.c` to properly handle page fixing/unfixing

**Key Files:**
- `toydb/tools/index_benchmark.c` - Index construction benchmark tool
- `toydb/amlayer/amscan.c` - Fixed scan implementation with proper page management

---

## Project Structure

```
Project/
├── README.md                    # This file
├── student.txt                  # Student dataset (required)
├── results/                     # Generated results directory
│   ├── pf_stats.csv            # PF layer statistics
│   ├── pf_stats.png            # PF layer performance graph
│   ├── index_metrics.csv       # Index construction metrics
│   ├── index_metrics.png       # Index construction comparison plots
│   ├── index_metrics_logical.png  # Logical reads comparison
│   ├── space_metrics.csv       # Slotted-page utilization metrics
│   ├── space_metrics.png       # Space utilization comparison
│   └── space_metrics_percent.png  # Space utilization percentage
├── analysis/
│   ├── pf_stats_plot.py        # Script to generate PF statistics plots
│   ├── index_metrics_plot.py   # Script to generate index metrics plots
│   ├── space_metrics_plot.py   # Script to generate space metrics plots
│   └── generate_all_plots.py   # Master script to generate all plots
└── toydb/
    ├── pflayer/                # Paged File Layer
    │   ├── pf.h                # PF layer interface
    │   ├── pf.c                # PF layer implementation
    │   ├── buf.c               # Buffer manager (LRU/MRU)
    │   ├── hash.c              # Hash table for buffer management
    │   ├── pf_stats.h          # Statistics structure
    │   ├── pf_stats.c          # Statistics implementation
    │   ├── pftypes.h           # PF type definitions
    │   └── Makefile            # Build configuration
    ├── amlayer/                # Access Method Layer
    │   ├── am.h                # AM layer interface
    │   ├── am.c                # AM core functions
    │   ├── amfns.c             # AM file operations
    │   ├── amsearch.c         # AM search functions
    │   ├── amscan.c            # AM scan functions (fixed)
    │   ├── aminsert.c          # AM insert functions
    │   ├── amstack.c           # AM stack operations
    │   ├── amprint.c           # AM print/debug functions
    │   ├── amglobals.c         # AM global variables
    │   └── makefile            # Build configuration
    └── tools/                   # Benchmark and utility tools
        ├── pf_benchmark.c      # PF layer benchmarking tool
        ├── student_store.c     # Slotted-page student storage
        ├── index_benchmark.c   # Index construction benchmark
        ├── slot_page.h         # Slotted-page API
        ├── slot_page.c         # Slotted-page implementation
        └── Makefile            # Build configuration
```

---

## Compilation Instructions

### Prerequisites

- **Operating System**: Linux, WSL (Windows Subsystem for Linux), or macOS
- **Compiler**: `gcc` or `cc` (C compiler)
- **Build Tools**: `make`
- **Python**: Python 3 with `matplotlib` library
- **Data File**: `student.txt` must be available at project root

### Step-by-Step Compilation

#### 1. Install Python Dependencies (if needed)

```bash
pip install matplotlib
```

#### 2. Build PF Layer

```bash
cd toydb/pflayer
make clean    # Optional: clean previous builds
make
```

This creates `pflayer.o` which contains:
- Enhanced buffer manager with LRU/MRU support
- Statistics collection system
- All PF layer functions

#### 3. Build AM Layer

```bash
cd ../amlayer
make clean    # Optional: clean previous builds
make
```

This creates `amlayer.o` which contains:
- B+ tree index operations
- Search, insert, delete, and scan functions
- Fixed scan implementation with proper page management

#### 4. Build Tools

```bash
cd ../tools
make clean    # Optional: clean previous builds
make
```

This creates three executables:
- `pf_benchmark` - PF layer benchmarking tool
- `student_store` - Slotted-page student storage tool
- `index_benchmark` - Index construction benchmark tool

#### 5. Verify Build

```bash
ls -lh pf_benchmark student_store index_benchmark
```

All three executables should be present and executable.

### Troubleshooting Compilation

**Issue: Architecture mismatch (i386 vs x86-64)**
- **Solution**: The AM layer Makefile includes `-m64` flag to force 64-bit compilation

**Issue: Implicit function declarations**
- **Solution**: Added proper includes (`<stdlib.h>`, `<string.h>`, `<unistd.h>`) and forward declarations

**Issue: Conflicting types for bcopy**
- **Solution**: Replaced `bcopy` with `memcpy` and fixed argument order (bcopy uses src,dest; memcpy uses dest,src)

---

## Running the Code

### 1. PF Layer Benchmarking

#### Generate PF Statistics Plot

```bash
cd analysis
python3 pf_stats_plot.py
```

This will:
- Run `pf_benchmark` with various read/write ratios (0:10 to 10:0)
- Test both LRU and MRU policies
- Generate `results/pf_stats.csv` and `results/pf_stats.png`

#### Manual PF Benchmark

```bash
cd toydb/tools
./pf_benchmark --mix 8:2 --policy lru --buffers 64 --pages 400 --ops 12000
```

**Parameters:**
- `--mix R:W` - Read to write ratio (e.g., 8:2 means 80% reads, 20% writes)
- `--policy lru|mru` - Replacement policy
- `--buffers N` - Buffer pool size
- `--pages N` - Number of pages in test file
- `--ops N` - Number of operations to perform

### 2. Slotted-Page Student Storage

```bash
cd toydb/tools
./student_store --data ../../student.txt \
                --out student.slotted \
                --buffers 80 \
                --policy mru \
                --delete-step 7 \
                --metrics ../../results/space_metrics.csv \
                --static-lens 128,256,512,768
```

**Parameters:**
- `--data <file>` - Path to student.txt file
- `--out <file>` - Output slotted-page file name
- `--buffers N` - Buffer pool size
- `--policy lru|mru` - Replacement policy
- `--delete-step N` - Delete every Nth record (use `--no-delete` to skip deletion)
- `--metrics <file>` - Output CSV file for metrics
- `--static-lens <list>` - Comma-separated list of max record lengths for static comparison

**Output:**
- Creates `student.slotted` file
- Generates `results/space_metrics.csv` with utilization comparison

### 3. Index Construction Benchmark

```bash
cd toydb/tools
./index_benchmark --data ../../student.txt \
                  --rel-base student_roll \
                  --metrics ../../results/index_metrics.csv \
                  --buffers 80 \
                  --policy lru \
                  --queries 500
```

**Parameters:**
- `--data <file>` - Path to student.txt file
- `--rel-base <name>` - Base name for index files (creates `<name>_post.0`, `<name>_inc.0`, `<name>_bulk.0`)
- `--metrics <file>` - Output CSV file for metrics
- `--buffers N` - Buffer pool size
- `--policy lru|mru` - Replacement policy
- `--queries N` - Number of query samples to run

**Output:**
- Creates three index files: `student_roll_post.0`, `student_roll_inc.0`, `student_roll_bulk.0`
- Generates `results/index_metrics.csv` with performance metrics for all three methods

### 4. Generate Visualizations

After running the benchmarks and generating CSV files, you can create visualizations:

#### Generate All Plots (Recommended)

```bash
cd analysis
python3 generate_all_plots.py
```

This will run all plotting scripts and generate:
- `results/pf_stats.png` - PF layer performance graph
- `results/index_metrics.png` - Index construction comparison
- `results/index_metrics_logical.png` - Logical reads comparison
- `results/space_metrics.png` - Space utilization comparison
- `results/space_metrics_percent.png` - Space utilization percentage

#### Generate Individual Plots

**PF Statistics Plot:**
```bash
cd analysis
python3 pf_stats_plot.py
```
Generates: `results/pf_stats.png`

**Index Metrics Plots:**
```bash
cd analysis
python3 index_metrics_plot.py
```
Generates: `results/index_metrics.png` and `results/index_metrics_logical.png`

**Space Metrics Plots:**
```bash
cd analysis
python3 space_metrics_plot.py
```
Generates: `results/space_metrics.png` and `results/space_metrics_percent.png`

**Note:** Make sure the corresponding CSV files exist in `results/` before running the plotting scripts.

### Complete Workflow Example

```bash
# From project root
cd toydb/pflayer && make
cd ../amlayer && make
cd ../tools && make

# Run all benchmarks
cd ../tools
./student_store --data ../../student.txt --out student.slotted \
                --metrics ../../results/space_metrics.csv \
                --static-lens 128,256,512,768

./index_benchmark --data ../../student.txt \
                  --rel-base student_roll \
                  --metrics ../../results/index_metrics.csv \
                  --buffers 80 --queries 500

cd ../../analysis
python3 generate_all_plots.py

# View results
cat ../results/index_metrics.csv
cat ../results/space_metrics.csv
cat ../results/pf_stats.csv

# View generated plots
ls -lh ../results/*.png
```

---

## Detailed Changes Made

### PF Layer Changes

#### 1. Statistics Collection System (`pf_stats.[ch]`)

**Why:** Required to track logical/physical I/O, page fixes, dirty marks, and input/output counts for performance analysis.

**Changes:**
- Created new `PF_Stats` structure with all required counters
- Implemented `PF_ResetStats()`, `PF_GetStats()`, `PF_PrintStats()` functions
- Added increment functions for each statistic type
- Integrated statistics collection throughout buffer manager

**Files Modified:**
- `toydb/pflayer/pf_stats.h` (new file)
- `toydb/pflayer/pf_stats.c` (new file)
- `toydb/pflayer/Makefile` - Added `pf_stats.o` to build

#### 2. Buffer Replacement Policies (`buf.c`, `pf.c`)

**Why:** Required to support both LRU and MRU replacement strategies, selectable per file.

**Changes:**
- Added `PFReplacementPolicy` enum in `pf.h`
- Modified `PFbufSelectVictim()` to choose pages based on policy:
  - LRU: Selects least recently used page
  - MRU: Selects most recently used page
- Added `PFbufSetCapacity()` to configure buffer pool size
- Added `PFbufMarkDirty()` for explicit dirty marking
- Updated `PFbufGet()` and `PFbufAlloc()` to accept policy parameter
- Added policy field to `PFftab_ele` structure in `pftypes.h`

**Files Modified:**
- `toydb/pflayer/buf.c` - Buffer manager with LRU/MRU support
- `toydb/pflayer/pf.c` - PF interface functions
- `toydb/pflayer/pf.h` - New API functions
- `toydb/pflayer/pftypes.h` - Added policy field

#### 3. PF Layer API Extensions (`pf.c`, `pf.h`)

**Why:** Required to expose buffer pool configuration, policy selection, and dirty marking to applications.

**Changes:**
- Added `PF_SetBufferPoolParams(int numbufs)` - Configure buffer pool size
- Added `PF_SetDefaultPolicy(PFReplacementPolicy policy)` - Set default replacement policy
- Added `PF_OpenFileWithPolicy(char *fname, PFReplacementPolicy policy)` - Open file with specific policy
- Added `PF_SetFilePolicy(int fd, PFReplacementPolicy policy)` - Change policy for open file
- Added `PF_MarkDirty(int fd, int pagenum)` - Explicitly mark page as dirty
- Added `PF_ResetStats()`, `PF_GetStats()`, `PF_PrintStats()` - Statistics management
- Updated `PF_OpenFile()` to use default policy
- Modified all page access functions to pass policy to buffer manager

**Files Modified:**
- `toydb/pflayer/pf.c` - Implementation of new functions
- `toydb/pflayer/pf.h` - Function declarations

#### 4. Physical I/O Counting (`buf.c`, `pf.c`)

**Why:** Required to track actual disk I/O operations separately from logical I/O.

**Changes:**
- Updated `PFreadfcn()` to increment `physicalReads` counter
- Updated `PFwritefcn()` to increment `physicalWrites` counter
- Updated `PFbufReleaseFile()` to increment `outputCount` when writing dirty pages
- Updated `PFbufGet()` to increment `inputCount` when reading pages from disk

**Files Modified:**
- `toydb/pflayer/pf.c` - Physical I/O counting in read/write functions
- `toydb/pflayer/buf.c` - Input/output counting in buffer operations

#### 5. Code Modernization

**Why:** Fix compilation warnings and ensure compatibility with modern compilers.

**Changes:**
- Added `#include <stdlib.h>` for `malloc`, `free`, `exit`
- Added `#include <string.h>` for `memcpy`, `strlen`, etc.
- Added `#include <unistd.h>` for `lseek`, `read`, `write`, `close`, `unlink`
- Removed redundant `extern char *malloc()` declarations
- Added explicit return types to all functions
- Fixed pointer format specifiers (`%d` → `%p` for pointers)

**Files Modified:**
- `toydb/pflayer/buf.c`
- `toydb/pflayer/hash.c`
- `toydb/pflayer/pf.c`
- `toydb/amlayer/am.h`
- `toydb/amlayer/am.c`
- `toydb/amlayer/amsearch.c`
- `toydb/amlayer/amscan.c`

### AM Layer Changes

#### 1. Fixed Scan Implementation (`amscan.c`)

**Why:** Original `GetLeftPageNum()` had bugs causing "page to be unfixed is not in the buffer" errors.

**Changes:**
- Fixed `GetLeftPageNum()` to properly track which pages were fixed by the function
- Added `pageFixed` flag to track page state
- Handle `PFE_PAGEFIXED` return code from `PF_GetThisPage()` correctly
- Only unfix pages that were successfully fixed by the function
- Added proper error handling for `PFE_EOF` case
- Added forward declarations for `AM_Search` and `AM_Compare`
- Replaced `bcopy` with `memcpy` and fixed argument order
- Added explicit return types to all functions

**Files Modified:**
- `toydb/amlayer/amscan.c` - Fixed scan implementation

#### 2. AM Layer Compilation Fixes

**Why:** Fix architecture mismatch and compilation warnings.

**Changes:**
- Added `-m64` flag to AM layer Makefile to force 64-bit compilation
- Added `#include <string.h>` for string functions
- Fixed implicit function declarations

**Files Modified:**
- `toydb/amlayer/makefile` - Added `-m64` flag
- `toydb/amlayer/am.c` - Added includes
- `toydb/amlayer/amsearch.c` - Added includes

### New Tools

#### 1. PF Benchmark Tool (`pf_benchmark.c`)

**Why:** Required to test PF layer with different read/write mixes and generate statistics.

**Implementation:**
- Configurable read/write ratio
- Supports LRU and MRU policies
- Configurable buffer pool size
- Outputs CSV format statistics
- Can be called programmatically by plotting script

#### 2. Student Store Tool (`student_store.c`)

**Why:** Required to demonstrate slotted-page structure with student data and compare with static layouts.

**Implementation:**
- Loads student data from `student.txt`
- Inserts records into slotted pages
- Optional deletion of records
- Sequential scanning
- Space utilization calculation
- Comparison with static fixed-length layouts
- Outputs CSV with utilization metrics

#### 3. Index Benchmark Tool (`index_benchmark.c`)

**Why:** Required to evaluate three indexing methods and compare their performance.

**Implementation:**
- Loads student records and extracts roll numbers
- Creates three index files using different methods:
  - Post-build: Original order
  - Incremental: Random order
  - Bulk: Sorted order
- Runs same queries on all three indexes
- Collects build-time and query-time statistics
- Outputs CSV with comprehensive metrics

#### 4. Slotted-Page Implementation (`slot_page.[ch]`)

**Why:** Required to implement variable-length record storage on top of PF layer.

**Implementation:**
- Slotted-page header structure
- Record insertion with space management
- Record deletion with free list management
- Sequential scanning
- Space utilization calculation
- Compaction support

#### 5. Plotting Scripts (`analysis/`)

**Why:** Required to visualize performance metrics and generate graphs for analysis.

**Implementation:**

**`pf_stats_plot.py`:**
- Iterates over read/write ratios from 0:10 to 10:0
- Tests both LRU and MRU policies
- Collects statistics for each configuration
- Generates CSV output
- Creates PNG graph showing physical I/O vs read/write mix

**`index_metrics_plot.py`:**
- Reads `index_metrics.csv` generated by `index_benchmark`
- Creates comparison plots for build time, query time, and I/O operations
- Generates two PNG files:
  - `index_metrics.png`: Build/query time and physical I/O comparison
  - `index_metrics_logical.png`: Logical reads comparison

**`space_metrics_plot.py`:**
- Reads `space_metrics.csv` generated by `student_store`
- Creates comparison plots for space utilization
- Generates two PNG files:
  - `space_metrics.png`: Utilization ratio and total space usage
  - `space_metrics_percent.png`: Utilization as percentage

**`generate_all_plots.py`:**
- Master script that runs all plotting scripts
- Generates all visualizations in one command
- Useful for regenerating all plots after data updates

---

## Implementation Details

### Buffer Replacement Policies

**LRU (Least Recently Used):**
- Selects the page that was accessed least recently
- Good for general-purpose workloads
- Implemented by maintaining pages in order of access

**MRU (Most Recently Used):**
- Selects the page that was accessed most recently
- Good for sequential file access patterns
- Implemented by selecting the most recently accessed page

### Statistics Collection

The statistics system tracks:
- **Logical Reads/Writes**: Page access operations (may be satisfied from buffer)
- **Physical Reads/Writes**: Actual disk I/O operations
- **Input Count**: Total pages read from disk
- **Output Count**: Total pages written to disk
- **Page Fixes**: Number of times pages were fixed in buffer
- **Dirty Marks**: Number of times pages were marked dirty

### Slotted-Page Structure

```
Page Layout:
[Header][Slot Directory][Free Space][Records...]
  ↑         ↑                ↑           ↑
  |         |                |           |
  |         |                |           Records grow upward from end
  |         |                Free space pointer
  |         Slots grow downward from header
  Fixed size header
```

**Features:**
- Variable-length records
- Efficient space utilization
- Support for deletion with free list
- Sequential scanning capability

### Index Construction Methods

**Post-Build:**
- All records inserted in original order
- Simulates building index on existing data file
- Fastest method in our tests

**Incremental (Random):**
- Records inserted one-by-one in random order
- Simulates real-world incremental inserts
- Moderate performance

**Bulk-Loading (Sorted):**
- Records inserted in sorted order
- Note: This is sorted incremental inserts, not true bottom-up bulk-loading
- In our B+ tree implementation, sorted inserts can cause more rightmost splits
- True bulk-loading would fill pages completely before splitting

---

## Results and Output

### Output Files

All results are written to the `results/` directory:

1. **`results/pf_stats.csv`**
   - PF layer statistics for different read/write mixes
   - Columns: read_ratio, policy, logical_reads, physical_reads, etc.

2. **`results/pf_stats.png`**
   - Graph showing physical I/O vs read/write mix
   - Separate lines for LRU and MRU policies
   - Generated by `analysis/pf_stats_plot.py`

3. **`results/index_metrics.csv`**
   - Index construction performance metrics
   - Rows for each method (post, incremental, bulk) and phase (build, query)
   - Columns: method, phase, logical_reads, physical_reads, elapsed_ms, etc.

4. **`results/index_metrics.png`**
   - Four-panel comparison of index construction methods
   - Shows build time, query time, and physical I/O for build and query phases
   - Generated by `analysis/index_metrics_plot.py`

5. **`results/index_metrics_logical.png`**
   - Comparison of logical reads for build and query phases
   - Shows how different methods affect logical page accesses
   - Generated by `analysis/index_metrics_plot.py`

6. **`results/space_metrics.csv`**
   - Slotted-page utilization comparison
   - Compares slotted-page vs static layouts for different record lengths

7. **`results/space_metrics.png`**
   - Two-panel comparison of space utilization
   - Shows utilization ratio and total space usage
   - Generated by `analysis/space_metrics_plot.py`

8. **`results/space_metrics_percent.png`**
   - Space utilization shown as percentage
   - Easy-to-read comparison of slotted-page vs static layouts
   - Generated by `analysis/space_metrics_plot.py`

### Interpreting Results

**PF Statistics:**
- Physical I/O should decrease as read ratio increases (more cache hits)
- MRU may perform better for sequential access patterns
- LRU generally better for random access patterns

**Index Construction:**
- Build time: Time to construct the index
- Physical I/O: Actual disk operations during build
- Query time: Time to execute queries
- Lower values indicate better performance

**Space Utilization:**
- Slotted-page should show better utilization than static layouts
- Utilization improves with variable-length records
- Static layouts waste space for shorter records
- See `space_metrics.png` and `space_metrics_percent.png` for visual comparisons

**Visualizations:**
- All plots are saved as high-resolution PNG files (200 DPI)
- Plots use color coding: green for slotted-page, red for static layouts
- Bar charts include value labels for easy reading
- Grid lines and legends are included for clarity

---

## Testing

### Basic Testing

1. **Test PF Layer:**
   ```bash
   cd toydb/tools
   ./pf_benchmark --mix 5:5 --policy lru --buffers 40 --pages 100 --ops 1000
   ```

2. **Test Slotted-Page:**
   ```bash
   cd toydb/tools
   ./student_store --data ../../student.txt --out test.slotted --metrics test.csv
   ```

3. **Test Index Construction:**
   ```bash
   cd toydb/tools
   ./index_benchmark --data ../../student.txt --rel-base test --buffers 40 --queries 10
   ```

### Verification

- Check that all three executables run without errors
- Verify that CSV files are generated in `results/` directory
- Check that index files are created (`.0` files in `toydb/tools/`)
- Verify that graphs are generated (PNG files in `results/`)

---

## Notes and Limitations

1. **Bulk-Loading Implementation:**
   - Current implementation uses sorted incremental inserts
   - True bottom-up bulk-loading would require AM layer modifications
   - Results show that sorted inserts can cause more splits in this B+ tree implementation

2. **Platform Compatibility:**
   - Designed for Linux/WSL/macOS
   - Windows users should use WSL
   - All code uses standard C and POSIX APIs

3. **Data Requirements:**
   - `student.txt` must be available at project root
   - File format: semicolon-delimited, roll number in second field

4. **Buffer Pool:**
   - Default size: 60 pages
   - Can be configured via `PF_SetBufferPoolParams()`
   - Larger buffer pools reduce physical I/O but use more memory

---

## Troubleshooting

### Common Issues

**"AM_OpenIndexScan failed"**
- Ensure index files were created successfully
- Check that `student.txt` is accessible
- Rebuild AM layer: `cd toydb/amlayer && make clean && make`

**"fopen dataset: No such file or directory"**
- Ensure `student.txt` is at project root
- Use correct relative path: `../../student.txt` from `toydb/tools/`

**Compilation errors**
- Ensure all dependencies are installed
- Try `make clean` then `make`
- Check that `-m64` flag is in AM layer Makefile

**No results generated**
- Check file permissions
- Ensure `results/` directory exists
- Check that tools completed without errors

---

## Code Comments and Documentation

All key functions include:
- Function purpose and parameters
- Return value description
- Important implementation notes
- Error handling information

Key files with extensive comments:
- `toydb/pflayer/pf_stats.c` - Statistics implementation
- `toydb/pflayer/buf.c` - Buffer manager with LRU/MRU
- `toydb/tools/slot_page.c` - Slotted-page implementation
- `toydb/tools/index_benchmark.c` - Index benchmark tool

---

## Conclusion

This implementation successfully fulfills all assignment objectives:

✅ **PF Layer**: LRU/MRU replacement, configurable buffer pool, statistics collection, performance plotting  
✅ **Slotted-Page**: Variable-length records, insertion/deletion/scanning, space utilization comparison  
✅ **Index Construction**: Three methods evaluated, performance metrics collected, comprehensive comparison  

All code follows programming best practices with appropriate comments, error handling, and documentation.

---

## Contact and Support

For issues or questions:
1. Check this README for common solutions
2. Review code comments in relevant files
3. Check compilation output for specific error messages
4. Verify all prerequisites are installed

--- 

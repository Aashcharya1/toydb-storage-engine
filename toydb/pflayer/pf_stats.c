/*
 * pf_stats.c - PF Layer Statistics Collection
 * 
 * This module implements comprehensive statistics collection for the PF layer,
 * tracking logical/physical I/O operations, page fixes, dirty marks, and
 * input/output counts. Statistics are used for performance analysis and
 * benchmarking of buffer management strategies.
 */

#include <string.h>
#include "pf_stats.h"

/* Global statistics structure - tracks all PF layer performance metrics */
static PF_Stats PFstats;

/**
 * Initialize statistics system
 * Resets all counters to zero
 */
void PFStatsInit()
{
	PFStatsReset();
}

/**
 * Reset all statistics counters to zero
 * Called at the start of each benchmark run
 */
void PFStatsReset()
{
	memset(&PFstats, 0, sizeof(PFstats));
}

/**
 * Get current statistics
 * @param stats - Pointer to PF_Stats structure to fill with current values
 *                If NULL, function returns without error
 */
void PFStatsGet(stats)
PF_Stats *stats;
{
	if (stats != NULL)
		*stats = PFstats;
}

void PFStatsPrint(out)
FILE *out;
{
	PF_Stats tmp;

	if (out == NULL)
		out = stdout;
	PFStatsGet(&tmp);
	fprintf(out,"PF statistics:\n");
	fprintf(out,"  logical reads   : %lu\n",tmp.logicalReads);
	fprintf(out,"  logical writes  : %lu\n",tmp.logicalWrites);
	fprintf(out,"  physical reads  : %lu\n",tmp.physicalReads);
	fprintf(out,"  physical writes : %lu\n",tmp.physicalWrites);
	fprintf(out,"  input count     : %lu\n",tmp.inputCount);
	fprintf(out,"  output count    : %lu\n",tmp.outputCount);
	fprintf(out,"  page fixes      : %lu\n",tmp.pageFixes);
	fprintf(out,"  dirty marks     : %lu\n",tmp.dirtyMarks);
}

void PFStatsIncLogicalRead()
{
	PFstats.logicalReads++;
}

void PFStatsIncLogicalWrite()
{
	PFstats.logicalWrites++;
}

void PFStatsIncPhysicalRead()
{
	PFstats.physicalReads++;
	PFstats.inputCount++;
}

void PFStatsIncPhysicalWrite()
{
	PFstats.physicalWrites++;
	PFstats.outputCount++;
}

void PFStatsIncPageFix()
{
	PFstats.pageFixes++;
}

void PFStatsIncDirtyMark()
{
	PFstats.dirtyMarks++;
}


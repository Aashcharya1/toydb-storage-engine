#ifndef PF_STATS_H
#define PF_STATS_H

#include <stdio.h>

typedef struct PF_Stats {
	unsigned long logicalReads;
	unsigned long logicalWrites;
	unsigned long physicalReads;
	unsigned long physicalWrites;
	unsigned long inputCount;
	unsigned long outputCount;
	unsigned long pageFixes;
	unsigned long dirtyMarks;
} PF_Stats;

void PFStatsInit();
void PFStatsReset();
void PFStatsGet(/*out*/ PF_Stats *stats);
void PFStatsPrint(FILE *out);
void PFStatsIncLogicalRead();
void PFStatsIncLogicalWrite();
void PFStatsIncPhysicalRead();
void PFStatsIncPhysicalWrite();
void PFStatsIncPageFix();
void PFStatsIncDirtyMark();

#endif /* PF_STATS_H */


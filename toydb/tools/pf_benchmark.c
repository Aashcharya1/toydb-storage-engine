#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "slot_page.h"

typedef struct WorkloadSpec {
	int readWeight;
	int writeWeight;
} WorkloadSpec;

static void usage(const char *prog);
static int parseMix(const char *arg, WorkloadSpec *spec);
static const char *policyName(PFReplacementPolicy policy);

int main(int argc, char **argv)
{
	const char *fileName = "pf_bench.pf";
	int numPages = 200;
	int operations = 5000;
	int bufferSize = 40;
	PFReplacementPolicy policy = PF_REPL_LRU;
	WorkloadSpec mix = {8,2}; /* default 80/20 */
	unsigned int seed = (unsigned int)time(NULL);
	int printHeader = 0;
	int i;
	int err;
	int fd;
	char *pageBuf;
	int pageNum;
	clock_t start,end;
	PF_Stats stats;

	for (i=1; i<argc; i++){
		if (strcmp(argv[i],"--file")==0 && i+1<argc)
			fileName = argv[++i];
		else if (strcmp(argv[i],"--pages")==0 && i+1<argc)
			numPages = atoi(argv[++i]);
		else if (strcmp(argv[i],"--ops")==0 && i+1<argc)
			operations = atoi(argv[++i]);
		else if (strcmp(argv[i],"--buffers")==0 && i+1<argc)
			bufferSize = atoi(argv[++i]);
		else if (strcmp(argv[i],"--policy")==0 && i+1<argc){
			const char *p = argv[++i];
			if (strcmp(p,"mru")==0 || strcmp(p,"MRU")==0)
				policy = PF_REPL_MRU;
			else
				policy = PF_REPL_LRU;
		}
		else if (strcmp(argv[i],"--mix")==0 && i+1<argc){
			if (parseMix(argv[++i],&mix)!=0){
				fprintf(stderr,"Invalid mix specification\n");
				return 1;
			}
		}
		else if (strcmp(argv[i],"--seed")==0 && i+1<argc){
			seed = (unsigned int)strtoul(argv[++i],NULL,10);
		}
		else if (strcmp(argv[i],"--header")==0){
			printHeader = 1;
		}
		else if (strcmp(argv[i],"--help")==0){
			usage(argv[0]);
			return 0;
		}
		else {
			fprintf(stderr,"Unknown option %s\n",argv[i]);
			usage(argv[0]);
			return 1;
		}
	}

	if (numPages <= 0 || operations <= 0){
		fprintf(stderr,"pages and ops must be positive\n");
		return 1;
	}

	srand(seed);
	PF_Init();
	PF_SetBufferPoolParams(bufferSize);
	PF_SetDefaultPolicy(policy);

	PF_DestroyFile((char *)fileName);
	if ((err=PF_CreateFile((char *)fileName))!=PFE_OK){
		PF_PrintError("PF_CreateFile");
		return 1;
	}
	fd = PF_OpenFileWithPolicy((char *)fileName,policy);
	if (fd < 0){
		PF_PrintError("PF_OpenFile");
		return 1;
	}

	for (i=0; i<numPages; i++){
		err = PF_AllocPage(fd,&pageNum,&pageBuf);
		if (err != PFE_OK){
			PF_PrintError("PF_AllocPage");
			PF_CloseFile(fd);
			return 1;
		}
		memset(pageBuf,0,PF_PAGE_SIZE);
		memcpy(pageBuf,&i,sizeof(int));
		PF_UnfixPage(fd,pageNum,TRUE);
	}

	PF_ResetStats();
	start = clock();
	for (i=0; i<operations; i++){
		int isWrite;
		int pick = rand() % (mix.readWeight + mix.writeWeight);
		isWrite = (pick >= mix.readWeight);
		pageNum = rand() % numPages;
		err = PF_GetThisPage(fd,pageNum,&pageBuf);
		if (err != PFE_OK){
			PF_PrintError("PF_GetThisPage");
			PF_CloseFile(fd);
			return 1;
		}
		if (isWrite){
			int value = i;
			memcpy(pageBuf,&value,sizeof(int));
			PF_UnfixPage(fd,pageNum,TRUE);
		}
		else {
			volatile int value;
			memcpy((void *)&value,pageBuf,sizeof(int));
			PF_UnfixPage(fd,pageNum,FALSE);
		}
	}
	end = clock();
	PF_GetStats(&stats);
	PF_CloseFile(fd);

	double elapsedMs = (double)(end - start) * 1000.0 / (double)CLOCKS_PER_SEC;
	if (printHeader){
		printf("policy,read_weight,write_weight,buffers,pages,ops,logical_reads,logical_writes,physical_reads,physical_writes,input_count,output_count,page_fixes,dirty_marks,elapsed_ms\n");
	}
	printf("%s,%d,%d,%d,%d,%d,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%.3f\n",
		policyName(policy),
		mix.readWeight,
		mix.writeWeight,
		bufferSize,
		numPages,
		operations,
		stats.logicalReads,
		stats.logicalWrites,
		stats.physicalReads,
		stats.physicalWrites,
		stats.inputCount,
		stats.outputCount,
		stats.pageFixes,
		stats.dirtyMarks,
		elapsedMs);
	return 0;
}

static void usage(const char *prog)
{
	fprintf(stderr,"Usage: %s [options]\n",prog);
	fprintf(stderr,"Options:\n");
	fprintf(stderr,"  --file <name>       PF file to create (default pf_bench.pf)\n");
	fprintf(stderr,"  --pages <n>         Number of pages to initialize (default 200)\n");
	fprintf(stderr,"  --ops <n>           Operations to perform (default 5000)\n");
	fprintf(stderr,"  --buffers <n>       Buffer pool size (default 40)\n");
	fprintf(stderr,"  --policy <lru|mru>  Replacement policy (default lru)\n");
	fprintf(stderr,"  --mix R:W           Read/write weights (default 8:2)\n");
	fprintf(stderr,"  --seed <val>        RNG seed\n");
	fprintf(stderr,"  --header            Print CSV header\n");
}

static int parseMix(const char *arg, WorkloadSpec *spec)
{
	const char *sep = strchr(arg,':');
	if (!sep)
		sep = strchr(arg,'/');
	if (!sep)
		return -1;
	spec->readWeight = atoi(arg);
	spec->writeWeight = atoi(sep+1);
	if (spec->readWeight < 0 || spec->writeWeight < 0)
		return -1;
	if (spec->readWeight + spec->writeWeight == 0)
		return -1;
	return 0;
}

static const char *policyName(PFReplacementPolicy policy)
{
	return (policy == PF_REPL_MRU) ? "mru" : "lru";
}


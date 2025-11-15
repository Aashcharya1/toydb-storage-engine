/*
 * index_benchmark.c - Index Construction Benchmark Tool
 * 
 * This tool evaluates three different methods for constructing B+ tree indexes
 * on the Student file using the roll-no field as the key:
 * 
 * 1. Post-build: Creates index file, then inserts all records in original order
 *    (simulates building index on existing data file)
 * 
 * 2. Incremental: Creates index file, then inserts records one-by-one in random order
 *    (simulates real-world incremental inserts)
 * 
 * 3. Bulk-loading: Creates index file, then inserts records in sorted order
 *    (simulates efficient bulk-loading for pre-sorted data)
 * 
 * For each method, the tool:
 * - Builds the index and collects build-time statistics
 * - Runs the same set of queries and collects query-time statistics
 * - Outputs comprehensive metrics to CSV file for analysis
 * 
 * The results can be used to compare the performance characteristics of
 * different index construction strategies.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "../amlayer/am.h"
#include "../amlayer/testam.h"
#include "slot_page.h"

#ifdef _WIN32
#define strdup _strdup
#endif

/* Structure to hold a record key (roll number) and its record ID */
typedef struct {
	int roll;    /* Roll number (key for index) */
	int recId;   /* Record ID (value stored in index) */
} RecordKey;

/* Structure to hold performance metrics for a method/phase combination */
typedef struct {
	char method[16];      /* Method name: "post", "incremental", or "bulk" */
	char phase[16];      /* Phase: "build" or "query" */
	PF_Stats stats;      /* PF layer statistics */
	double elapsedMs;     /* Elapsed time in milliseconds */
} MetricRow;

static void usage(const char *prog);
static int loadRecords(const char *path, RecordKey **records, int *count);
static RecordKey *copyRecords(const RecordKey *src, int count);
static void shuffleRecords(RecordKey *records, int count);
static int compareRecord(const void *a, const void *b);
static int buildIndex(const char *relName, RecordKey *records, int count,
		      PFReplacementPolicy policy, MetricRow *row);
static int runQueries(const char *relName, int *queries, int queryCount,
		      PFReplacementPolicy policy, MetricRow *row);
static int writeMetrics(const char *path, MetricRow *rows, int rowCount);
static int *selectQueries(const RecordKey *records, int count, int queryCount);
static int equalsIgnoreCase(const char *a, const char *b);

int main(int argc, char **argv)
{
	const char *dataPath = NULL;
	const char *relBase = "student_index";
	const char *metricsPath = "../results/index_metrics.csv";
	int bufferSize = 60;
	const char *policyStr = "lru";
	PFReplacementPolicy policy = PF_REPL_LRU;
	int queryCount = 500;
	int i;
	RecordKey *records = NULL;
	int recordCount = 0;
	RecordKey *recordsPost = NULL;
	RecordKey *recordsInc = NULL;
	RecordKey *recordsBulk = NULL;
	int *queries = NULL;
	MetricRow rows[6];
	int rowIdx = 0;

	for (i=1; i<argc; i++){
		if (strcmp(argv[i],"--data")==0 && i+1<argc)
			dataPath = argv[++i];
		else if (strcmp(argv[i],"--rel-base")==0 && i+1<argc)
			relBase = argv[++i];
		else if (strcmp(argv[i],"--metrics")==0 && i+1<argc)
			metricsPath = argv[++i];
		else if (strcmp(argv[i],"--buffers")==0 && i+1<argc)
			bufferSize = atoi(argv[++i]);
		else if (strcmp(argv[i],"--policy")==0 && i+1<argc){
			policyStr = argv[++i];
		}
		else if (strcmp(argv[i],"--queries")==0 && i+1<argc){
			queryCount = atoi(argv[++i]);
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

	if (!dataPath){
		fprintf(stderr,"--data is required\n");
		usage(argv[0]);
		return 1;
	}

	if (equalsIgnoreCase(policyStr,"mru"))
		policy = PF_REPL_MRU;
	else
		policy = PF_REPL_LRU;

	if (loadRecords(dataPath,&records,&recordCount)!=0){
		fprintf(stderr,"Failed to read dataset\n");
		return 1;
	}

	if (recordCount == 0){
		fprintf(stderr,"Dataset is empty\n");
		free(records);
		return 1;
	}

	srand((unsigned int)time(NULL));

	recordsPost = copyRecords(records,recordCount);
	recordsInc = copyRecords(records,recordCount);
	recordsBulk = copyRecords(records,recordCount);
	if (!recordsPost || !recordsInc || !recordsBulk){
		fprintf(stderr,"Out of memory\n");
		free(records);
		return 1;
	}

	shuffleRecords(recordsInc,recordCount);
	qsort(recordsBulk,recordCount,sizeof(RecordKey),compareRecord);

	queries = selectQueries(records,recordCount,queryCount);
	if (!queries){
		fprintf(stderr,"Failed to sample queries\n");
		free(records);
		free(recordsPost);
		free(recordsInc);
		free(recordsBulk);
		return 1;
	}

	PF_Init();
	PF_SetBufferPoolParams(bufferSize);
	PF_SetDefaultPolicy(policy);

	char relName[256];

	/* Post build */
	snprintf(relName,sizeof(relName),"%s_post",relBase);
	if (buildIndex(relName,recordsPost,recordCount,policy,&rows[rowIdx])!=0)
		goto cleanup;
	strcpy(rows[rowIdx].method,"post");
	strcpy(rows[rowIdx].phase,"build");
	rowIdx++;
	if (runQueries(relName,queries,queryCount,policy,&rows[rowIdx])!=0)
		goto cleanup;
	strcpy(rows[rowIdx].method,"post");
	strcpy(rows[rowIdx].phase,"query");
	rowIdx++;

	/* Incremental (random order) */
	snprintf(relName,sizeof(relName),"%s_inc",relBase);
	if (buildIndex(relName,recordsInc,recordCount,policy,&rows[rowIdx])!=0)
		goto cleanup;
	strcpy(rows[rowIdx].method,"incremental");
	strcpy(rows[rowIdx].phase,"build");
	rowIdx++;
	if (runQueries(relName,queries,queryCount,policy,&rows[rowIdx])!=0)
		goto cleanup;
	strcpy(rows[rowIdx].method,"incremental");
	strcpy(rows[rowIdx].phase,"query");
	rowIdx++;

	/* Bulk (sorted order) */
	snprintf(relName,sizeof(relName),"%s_bulk",relBase);
	if (buildIndex(relName,recordsBulk,recordCount,policy,&rows[rowIdx])!=0)
		goto cleanup;
	strcpy(rows[rowIdx].method,"bulk");
	strcpy(rows[rowIdx].phase,"build");
	rowIdx++;
	if (runQueries(relName,queries,queryCount,policy,&rows[rowIdx])!=0)
		goto cleanup;
	strcpy(rows[rowIdx].method,"bulk");
	strcpy(rows[rowIdx].phase,"query");
	rowIdx++;

	if (writeMetrics(metricsPath,rows,rowIdx)!=0)
		fprintf(stderr,"Failed to write index metrics\n");

cleanup:
	free(records);
	free(recordsPost);
	free(recordsInc);
	free(recordsBulk);
	free(queries);
	return 0;
}

static void usage(const char *prog)
{
	fprintf(stderr,"Usage: %s --data student.txt [options]\n",prog);
	fprintf(stderr,"Options:\n");
	fprintf(stderr,"  --rel-base <name>      Base name for generated indexes\n");
	fprintf(stderr,"  --metrics <file>       CSV output path\n");
	fprintf(stderr,"  --buffers <n>          Buffer pool size\n");
	fprintf(stderr,"  --policy <lru|mru>     Replacement policy\n");
	fprintf(stderr,"  --queries <n>          Number of query samples\n");
}

static int loadRecords(const char *path, RecordKey **records, int *count)
{
	FILE *fp = fopen(path,"r");
	char line[4096];
	RecordKey *arr = NULL;
	int capacity = 0;
	int n = 0;

	if (!fp){
		perror("fopen dataset");
		return -1;
	}

	while (fgets(line,sizeof(line),fp)!=NULL){
		size_t len = strlen(line);
		while (len>0 && (line[len-1]=='\n' || line[len-1]=='\r'))
			line[--len] = '\0';
		if (len == 0)
			continue;
		if (!isdigit((unsigned char)line[0]))
			continue;
		char *copy = strdup(line);
		if (!copy){
			free(arr);
			fclose(fp);
			return -1;
		}
		char *token = strtok(copy,";");
		int field = 0;
		int roll = 0;
		while (token){
			if (field == 1){
				roll = atoi(token);
				break;
			}
			token = strtok(NULL,";");
			field++;
		}
		free(copy);
		if (roll == 0)
			continue;
		if (n == capacity){
			capacity = capacity ? capacity * 2 : 2048;
			RecordKey *tmp = (RecordKey *)realloc(arr,sizeof(RecordKey)*capacity);
			if (!tmp){
				free(arr);
				fclose(fp);
				return -1;
			}
			arr = tmp;
		}
		arr[n].roll = roll;
		arr[n].recId = n + 1;
		n++;
	}

	fclose(fp);
	*records = arr;
	*count = n;
	return 0;
}

static RecordKey *copyRecords(const RecordKey *src, int count)
{
	RecordKey *dst = (RecordKey *)malloc(sizeof(RecordKey)*count);
	if (!dst)
		return NULL;
	memcpy(dst,src,sizeof(RecordKey)*count);
	return dst;
}

static void shuffleRecords(RecordKey *records, int count)
{
	int i;
	for (i=count-1; i>0; i--){
		int j = rand() % (i+1);
		RecordKey tmp = records[i];
		records[i] = records[j];
		records[j] = tmp;
	}
}

/**
 * Comparison function for qsort - sorts records by roll number
 * Used to prepare records for bulk-loading method
 */
static int compareRecord(const void *a, const void *b)
{
	const RecordKey *ra = (const RecordKey *)a;
	const RecordKey *rb = (const RecordKey *)b;
	if (ra->roll < rb->roll) return -1;
	if (ra->roll > rb->roll) return 1;
	return 0;
}

/**
 * Build an index using the specified records
 * 
 * Creates a new index file and inserts all records in the order provided.
 * Collects build-time statistics including elapsed time and PF layer metrics.
 * 
 * @param relName - Base name for the index file (will create <relName>.0)
 * @param records - Array of records to insert (order matters for performance)
 * @param count - Number of records to insert
 * @param policy - Buffer replacement policy (LRU or MRU)
 * @param row - Output structure to store collected metrics
 * @return 0 on success, -1 on error
 */
static int buildIndex(const char *relName, RecordKey *records, int count,
		      PFReplacementPolicy policy, MetricRow *row)
{
	char fname[256];
	int fd;
	int i;
	int err;

	snprintf(fname,sizeof(fname),"%s.0",relName);
	PF_DestroyFile(fname);
	if ((err=AM_CreateIndex(relName,0,INT_TYPE,sizeof(int)))!=AME_OK){
		fprintf(stderr,"AM_CreateIndex failed for %s\n",relName);
		return -1;
	}
	snprintf(fname,sizeof(fname),"%s.0",relName);
	fd = PF_OpenFile(fname);
	if (fd < 0){
		PF_PrintError("PF_OpenFile");
		return -1;
	}
	PF_SetFilePolicy(fd,policy);

	PF_ResetStats();
	clock_t start = clock();
	for (i=0; i<count; i++){
		err = AM_InsertEntry(fd,INT_TYPE,sizeof(int),
				     (char *)&records[i].roll,
				     records[i].recId);
		if (err != AME_OK){
			fprintf(stderr,"AM_InsertEntry failed (%d)\n",err);
			PF_CloseFile(fd);
			return -1;
		}
	}
	clock_t end = clock();
	PF_GetStats(&row->stats);
	row->elapsedMs = (double)(end - start)*1000.0/(double)CLOCKS_PER_SEC;

	PF_CloseFile(fd);
	return 0;
}

/**
 * Run queries on an existing index and collect query-time statistics
 * 
 * Opens the index file and executes equality queries for each key in the
 * queries array. Collects query-time statistics including elapsed time and
 * PF layer metrics.
 * 
 * @param relName - Base name of the index file (will open <relName>.0)
 * @param queries - Array of roll numbers to query
 * @param queryCount - Number of queries to execute
 * @param policy - Buffer replacement policy (LRU or MRU)
 * @param row - Output structure to store collected metrics
 * @return 0 on success, -1 on error
 */
static int runQueries(const char *relName, int *queries, int queryCount,
		      PFReplacementPolicy policy, MetricRow *row)
{
	char fname[256];
	int fd;
	int i;
	int sd;
	int recId;

	snprintf(fname,sizeof(fname),"%s.0",relName);
	fd = PF_OpenFile(fname);
	if (fd < 0){
		PF_PrintError("PF_OpenFile");
		return -1;
	}
	PF_SetFilePolicy(fd,policy);

	PF_ResetStats();
	clock_t start = clock();
	for (i=0; i<queryCount; i++){
		sd = AM_OpenIndexScan(fd,INT_TYPE,sizeof(int),EQ_OP,
				      (char *)&queries[i]);
		if (sd < 0){
			fprintf(stderr,"AM_OpenIndexScan failed (AM_Errno=%d)\n",AM_Errno);
			AM_PrintError("AM_OpenIndexScan");
			PF_CloseFile(fd);
			return -1;
		}
		recId = AM_FindNextEntry(sd);
		if (recId < 0){
			fprintf(stderr,"Query key %d not found\n",queries[i]);
		}
		AM_CloseIndexScan(sd);
	}
	clock_t end = clock();
	PF_GetStats(&row->stats);
	row->elapsedMs = (double)(end - start)*1000.0/(double)CLOCKS_PER_SEC;
	PF_CloseFile(fd);
	return 0;
}

static int writeMetrics(const char *path, MetricRow *rows, int rowCount)
{
	FILE *fp = fopen(path,"w");
	int i;

	if (!fp){
		perror("fopen metrics");
		return -1;
	}

	fprintf(fp,"method,phase,logical_reads,logical_writes,physical_reads,physical_writes,page_fixes,dirty_marks,elapsed_ms\n");
	for (i=0; i<rowCount; i++){
		fprintf(fp,"%s,%s,%lu,%lu,%lu,%lu,%lu,%lu,%.3f\n",
			rows[i].method,
			rows[i].phase,
			rows[i].stats.logicalReads,
			rows[i].stats.logicalWrites,
			rows[i].stats.physicalReads,
			rows[i].stats.physicalWrites,
			rows[i].stats.pageFixes,
			rows[i].stats.dirtyMarks,
			rows[i].elapsedMs);
	}
	fclose(fp);
	return 0;
}

static int *selectQueries(const RecordKey *records, int count, int queryCount)
{
	int *keys = (int *)malloc(sizeof(int)*queryCount);
	int i;
	if (!keys)
		return NULL;
	for (i=0; i<queryCount; i++){
		int idx = rand() % count;
		keys[i] = records[idx].roll;
	}
	return keys;
}

static int equalsIgnoreCase(const char *a, const char *b)
{
	if (!a || !b)
		return 0;
	while (*a && *b){
		char ca = (char)tolower((unsigned char)*a);
		char cb = (char)tolower((unsigned char)*b);
		if (ca != cb)
			return 0;
		a++;
		b++;
	}
	return *a == '\0' && *b == '\0';
}


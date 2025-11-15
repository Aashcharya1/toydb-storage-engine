#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>
#include <time.h>
#include "slot_page.h"

#ifdef _WIN32
#define strdup _strdup
#endif

typedef struct StudentStore {
	int fd;
	int lastPage;
	long pageCount;
	long recordCount;
} StudentStore;

typedef struct MetricsConfig {
	int *staticLens;
	int staticCount;
	const char *outputPath;
} MetricsConfig;

static void usage(const char *prog);
static int parseStaticSizes(const char *arg, int **values, int *count);
static int equalsIgnoreCase(const char *a, const char *b);
static int initStore(StudentStore *store, const char *path, PFReplacementPolicy policy);
static void closeStore(StudentStore *store);
static int insertRecord(StudentStore *store, const char *data, short len);
static long deleteEvery(StudentStore *store, int step);
static long scanCount(StudentStore *store);
static int computeUsage(StudentStore *store, long *pages, long *payload);
static int writeMetrics(const MetricsConfig *cfg, long activeRecords,
			long payloadBytes, long slottedPages);

int main(int argc, char **argv)
{
	const char *dataPath = NULL;
	const char *outPath = "student.slotted";
	const char *metricsPath = "../results/space_metrics.csv";
	int bufferSize = 50;
	const char *policyStr = "lru";
	PFReplacementPolicy policy = PF_REPL_LRU;
	int deleteStep = 7;
	int *staticLens = NULL;
	int staticCount = 0;
	int i;
	int err;
	StudentStore store = { -1,-1,0,0 };
	long totalRecords = 0;
	long totalBytes = 0;
	long deletedRecords = 0;
	long activeRecords = 0;
	long slottedPages = 0;
	long payloadBytes = 0;
	long payloadBefore = 0;
	long pagesBefore = 0;
	MetricsConfig metrics;

	for (i=1; i<argc; i++){
		if (strcmp(argv[i],"--data")==0 && i+1<argc){
			dataPath = argv[++i];
		}
		else if (strcmp(argv[i],"--out")==0 && i+1<argc){
			outPath = argv[++i];
		}
		else if (strcmp(argv[i],"--buffers")==0 && i+1<argc){
			bufferSize = atoi(argv[++i]);
		}
		else if (strcmp(argv[i],"--policy")==0 && i+1<argc){
			policyStr = argv[++i];
		}
		else if (strcmp(argv[i],"--delete-step")==0 && i+1<argc){
			deleteStep = atoi(argv[++i]);
		}
		else if (strcmp(argv[i],"--metrics")==0 && i+1<argc){
			metricsPath = argv[++i];
		}
		else if (strcmp(argv[i],"--static-lens")==0 && i+1<argc){
			if (parseStaticSizes(argv[++i],&staticLens,&staticCount)!=0){
				fprintf(stderr,"invalid --static-lens argument\n");
				return 1;
			}
		}
		else if (strcmp(argv[i],"--no-delete")==0){
			deleteStep = 0;
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

	if (dataPath == NULL){
		fprintf(stderr,"--data is required\n");
		usage(argv[0]);
		return 1;
	}

	if (staticCount == 0){
		const char *defaults = "128,256,512,768";
		if (parseStaticSizes(defaults,&staticLens,&staticCount)!=0){
			fprintf(stderr,"Failed to parse default static sizes\n");
			return 1;
		}
	}

	if (equalsIgnoreCase(policyStr,"mru"))
		policy = PF_REPL_MRU;
	else
		policy = PF_REPL_LRU;

	PF_Init();
	PF_SetBufferPoolParams(bufferSize);
	PF_SetDefaultPolicy(policy);

	PF_DestroyFile((char *)outPath);
	if ((err=PF_CreateFile((char *)outPath))!=PFE_OK){
		PF_PrintError("PF_CreateFile");
		return 1;
	}

	if (initStore(&store,outPath,policy)!=PFE_OK)
		return 1;

	FILE *fp = fopen(dataPath,"r");
	if (!fp){
		perror("fopen");
		closeStore(&store);
		return 1;
	}

	char line[4096];
	while (fgets(line,sizeof(line),fp)!=NULL){
		size_t len = strlen(line);
		while (len>0 && (line[len-1]=='\n' || line[len-1]=='\r'))
			line[--len] = '\0';
		if (len == 0)
			continue;
		if (!isdigit((unsigned char)line[0]))
			continue;
		if (len >= 32760){
			fprintf(stderr,"Record too long (%zu bytes)\n",len);
			fclose(fp);
			closeStore(&store);
			return 1;
		}
		if (insertRecord(&store,line,(short)len)!=SP_OK){
			fprintf(stderr,"Failed to insert record\n");
			fclose(fp);
			closeStore(&store);
			return 1;
		}
		totalRecords++;
		totalBytes += (long)len;
	}
	fclose(fp);

	computeUsage(&store,&pagesBefore,&payloadBefore);

	if (deleteStep > 0){
		deletedRecords = deleteEvery(&store,deleteStep);
		printf("Deleted %ld records using step %d\n",deletedRecords,deleteStep);
	}

	activeRecords = scanCount(&store);
	if (activeRecords < 0){
		closeStore(&store);
		return 1;
	}

	if (computeUsage(&store,&slottedPages,&payloadBytes)!=0){
		closeStore(&store);
		return 1;
	}

	metrics.staticLens = staticLens;
	metrics.staticCount = staticCount;
	metrics.outputPath = metricsPath;
	if (writeMetrics(&metrics,activeRecords,payloadBytes,slottedPages)!=0){
		fprintf(stderr,"Failed to write metrics table\n");
	}

	printf("Loaded %ld records (%ld bytes) into %ld pages\n",
		totalRecords,totalBytes,store.pageCount);
	printf("Active records after deletion: %ld\n",activeRecords);
	printf("Slotted payload bytes: %ld, pages: %ld\n",payloadBytes,slottedPages);
	closeStore(&store);
	free(staticLens);
	return 0;
}

static void usage(const char *prog)
{
	fprintf(stderr,"Usage: %s --data student.txt [options]\n",prog);
	fprintf(stderr,"Options:\n");
	fprintf(stderr,"  --out <file>            Output PF file (default student.slotted)\n");
	fprintf(stderr,"  --buffers <n>           Buffer pool size (default 50)\n");
	fprintf(stderr,"  --policy <lru|mru>      Replacement policy (default lru)\n");
	fprintf(stderr,"  --delete-step <n>       Delete every n-th record (default 7, 0 to skip)\n");
	fprintf(stderr,"  --metrics <path>        CSV output for utilization table\n");
	fprintf(stderr,"  --static-lens <list>    Comma separated max lengths for static layout\n");
	fprintf(stderr,"  --no-delete             Skip deletion phase\n");
}

static int parseStaticSizes(const char *arg, int **values, int *count)
{
	char *copy = strdup(arg);
	char *token;
	int capacity = 0;
	int *arr = NULL;
	int n = 0;

	if (!copy)
		return -1;
	token = strtok(copy,",");
	while (token){
		int v = atoi(token);
		if (v <= 0){
			free(copy);
			free(arr);
			return -1;
		}
		if (n == capacity){
			capacity = capacity ? capacity * 2 : 4;
			arr = (int *)realloc(arr,sizeof(int)*capacity);
			if (!arr){
				free(copy);
				return -1;
			}
		}
		arr[n++] = v;
		token = strtok(NULL,",");
	}
	free(copy);
	*values = arr;
	*count = n;
	return 0;
}

static int equalsIgnoreCase(const char *a, const char *b)
{
	if (a == NULL || b == NULL)
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

static int initStore(StudentStore *store, const char *path, PFReplacementPolicy policy)
{
	int fd = PF_OpenFileWithPolicy((char *)path,policy);
	if (fd < 0){
		PF_PrintError("PF_OpenFileWithPolicy");
		return fd;
	}
	store->fd = fd;
	store->lastPage = -1;
	store->pageCount = 0;
	store->recordCount = 0;
	return PFE_OK;
}

static void closeStore(StudentStore *store)
{
	if (store->fd >= 0)
		PF_CloseFile(store->fd);
	store->fd = -1;
}

static int insertRecord(StudentStore *store, const char *data, short len)
{
	char *pageBuf;
	int pageNum;
	int err;
	int rc;

	if (store->lastPage < 0){
		err = PF_AllocPage(store->fd,&pageNum,&pageBuf);
		if (err != PFE_OK){
			PF_PrintError("PF_AllocPage");
			return err;
		}
		SP_InitPage(pageBuf);
		store->lastPage = pageNum;
		store->pageCount++;
	}
	else {
		pageNum = store->lastPage;
		err = PF_GetThisPage(store->fd,pageNum,&pageBuf);
		if (err != PFE_OK){
			PF_PrintError("PF_GetThisPage");
			return err;
		}
	}

	rc = SP_InsertRecord(pageBuf,data,len,NULL);
	if (rc == SP_OK){
		PF_UnfixPage(store->fd,pageNum,TRUE);
		store->recordCount++;
		return SP_OK;
	}

	/* page full - unfix and allocate new page */
	PF_UnfixPage(store->fd,pageNum,FALSE);
	err = PF_AllocPage(store->fd,&pageNum,&pageBuf);
	if (err != PFE_OK){
		PF_PrintError("PF_AllocPage");
		return err;
	}
	SP_InitPage(pageBuf);
	rc = SP_InsertRecord(pageBuf,data,len,NULL);
	if (rc != SP_OK){
		PF_UnfixPage(store->fd,pageNum,FALSE);
		return rc;
	}
	store->pageCount++;
	store->lastPage = pageNum;
	store->recordCount++;
	PF_UnfixPage(store->fd,pageNum,TRUE);
	return SP_OK;
}

static long deleteEvery(StudentStore *store, int step)
{
	int err;
	int pageNum;
	char *pageBuf;
	long deleted = 0;
	long counter = 0;

	err = PF_GetFirstPage(store->fd,&pageNum,&pageBuf);
	while (err == PFE_OK){
		int current = pageNum;
		short cursor = -1;
		char *dataPtr;
		short length;
		int dirty = FALSE;
		while (SP_GetNextRecord(pageBuf,&cursor,&dataPtr,&length)==SP_OK){
			counter++;
			if (step > 0 && (counter % step)==0){
				if (SP_DeleteRecord(pageBuf,cursor)==SP_OK){
					dirty = TRUE;
					deleted++;
				}
			}
		}
		PF_UnfixPage(store->fd,current,dirty);
		err = PF_GetNextPage(store->fd,&pageNum,&pageBuf);
	}
	if (err != PFE_EOF)
		PF_PrintError("PF_GetNextPage");
	return deleted;
}

static long scanCount(StudentStore *store)
{
	int err;
	int pageNum;
	char *pageBuf;
	long count = 0;

	err = PF_GetFirstPage(store->fd,&pageNum,&pageBuf);
	while (err == PFE_OK){
		int current = pageNum;
		short cursor = -1;
		while (SP_GetNextRecord(pageBuf,&cursor,NULL,NULL)==SP_OK)
			count++;
		PF_UnfixPage(store->fd,current,FALSE);
		err = PF_GetNextPage(store->fd,&pageNum,&pageBuf);
	}
	if (err != PFE_EOF){
		PF_PrintError("PF_GetNextPage");
		return -1;
	}
	return count;
}

static int computeUsage(StudentStore *store, long *pages, long *payload)
{
	int err;
	int pageNum;
	char *pageBuf;
	long totalPages = 0;
	long totalPayload = 0;

	err = PF_GetFirstPage(store->fd,&pageNum,&pageBuf);
	while (err == PFE_OK){
		int current = pageNum;
		totalPages++;
		totalPayload += SP_PageUsedBytes(pageBuf);
		PF_UnfixPage(store->fd,current,FALSE);
		err = PF_GetNextPage(store->fd,&pageNum,&pageBuf);
	}
	if (err != PFE_EOF){
		PF_PrintError("PF_GetNextPage");
		return -1;
	}
	if (pages)
		*pages = totalPages;
	if (payload)
		*payload = totalPayload;
	return 0;
}

static int writeMetrics(const MetricsConfig *cfg, long activeRecords,
			long payloadBytes, long slottedPages)
{
	FILE *fp = fopen(cfg->outputPath,"w");
	long slottedSpace = slottedPages * (long)PF_PAGE_SIZE;
	int i;

	if (!fp){
		perror("fopen metrics");
		return -1;
	}

	fprintf(fp,"layout,max_record_length,records,pages,space_bytes,payload_bytes,utilization\n");
	if (slottedPages > 0 && slottedSpace > 0){
		double util = (slottedSpace > 0) ?
			((double)payloadBytes /(double)slottedSpace) : 0.0;
		fprintf(fp,"slotted,variable,%ld,%ld,%ld,%ld,%.6f\n",
			activeRecords,slottedPages,slottedSpace,payloadBytes,util);
	}

	for (i=0; i<cfg->staticCount; i++){
		int maxLen = cfg->staticLens[i];
		if (maxLen <= 0 || maxLen > PF_PAGE_SIZE)
			continue;
		long slotsPerPage = PF_PAGE_SIZE / maxLen;
		if (slotsPerPage == 0)
			continue;
		long pagesNeeded = (activeRecords + slotsPerPage - 1)/slotsPerPage;
		long spaceBytes = pagesNeeded * (long)PF_PAGE_SIZE;
		double util = (spaceBytes > 0) ?
			((double)payloadBytes /(double)spaceBytes) : 0.0;
		fprintf(fp,"static,%d,%ld,%ld,%ld,%ld,%.6f\n",
			maxLen,activeRecords,pagesNeeded,spaceBytes,payloadBytes,util);
	}
	fclose(fp);
	return 0;
}


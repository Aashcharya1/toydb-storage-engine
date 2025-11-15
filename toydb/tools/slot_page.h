#ifndef SLOT_PAGE_H
#define SLOT_PAGE_H

#include "../pflayer/pf.h"

#define SP_OK 0
#define SP_ERR_NOSPACE -1
#define SP_ERR_INVALID_SLOT -2
#define SP_ERR_EMPTY -3

typedef struct SP_PageHeader {
	short slotCount;
	short freeListHead;
	short freePtr;
	short attrLength; /* not strictly needed but handy for debugging */
} SP_PageHeader;

typedef struct SP_RecordRef {
	int pageNum;
	short slotId;
} SP_RecordRef;

void SP_InitPage(char *pageBuf);
int SP_InsertRecord(char *pageBuf, const char *data, short length, short *slotId);
int SP_DeleteRecord(char *pageBuf, short slotId);
int SP_GetRecord(char *pageBuf, short slotId, char **dataPtr, short *length);
int SP_GetNextRecord(char *pageBuf, short *cursor, char **dataPtr, short *length);
int SP_PageFreeSpace(char *pageBuf);
int SP_PageUsedBytes(char *pageBuf);

#endif /* SLOT_PAGE_H */


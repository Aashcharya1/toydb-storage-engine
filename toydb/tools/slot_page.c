/*
 * slot_page.c - Slotted-Page Structure Implementation
 * 
 * Implements a slotted-page structure for storing variable-length records
 * on top of the PF layer. The page layout is:
 * 
 * [Header][Slot Directory][Free Space][Records...]
 * 
 * - Header: Fixed-size metadata (slot count, free list, free pointer)
 * - Slot Directory: Grows downward from header, stores offset/length for each record
 * - Records: Grow upward from page end, variable-length data
 * 
 * This structure allows efficient insertion, deletion, and scanning of
 * variable-length records while maintaining good space utilization.
 */

#include <string.h>
#include "slot_page.h"

#define SP_INVALID_SLOT -1  /* Marker for invalid slot index */

/* Internal slot entry structure - stored in slot directory */
typedef struct SP_SlotEntry {
	short offset;  /* Offset from start of page where record begins */
	short length;  /* Length of record in bytes */
} SP_SlotEntry;

/* Macros for accessing page structure */
#define SP_HEADER(page) ((SP_PageHeader *)(page))
#define SP_SLOT(page,idx) \
	((SP_SlotEntry *)((page) + sizeof(SP_PageHeader) + (idx)*sizeof(SP_SlotEntry)))
#define SP_MAX_SLOTS \
	((PF_PAGE_SIZE - sizeof(SP_PageHeader)) / sizeof(SP_SlotEntry))

/* Forward declarations for internal helper functions */
static void SP_CompactPage(char *pageBuf);
static int SP_EnsureSpace(char *pageBuf, int neededBytes);

/**
 * Initialize a slotted page
 * Sets up the page header with zero slots and free pointer at page end
 * 
 * @param pageBuf - Pointer to page buffer (must be PF_PAGE_SIZE bytes)
 */
void SP_InitPage(char *pageBuf)
{
	SP_PageHeader *header;

	memset(pageBuf, 0, PF_PAGE_SIZE);
	header = SP_HEADER(pageBuf);
	header->slotCount = 0;
	header->freeListHead = SP_INVALID_SLOT;
	header->freePtr = PF_PAGE_SIZE;  /* Start free space at page end */
	header->attrLength = 0;
}

int SP_PageFreeSpace(char *pageBuf)
{
	SP_PageHeader *header = SP_HEADER(pageBuf);
	int slotBytes = header->slotCount * sizeof(SP_SlotEntry);
	int used = sizeof(SP_PageHeader) + slotBytes;
	return header->freePtr - used;
}

int SP_PageUsedBytes(char *pageBuf)
{
	SP_PageHeader *header = SP_HEADER(pageBuf);
	int used = 0;
	int i;
	SP_SlotEntry *slot;

	for (i=0; i<header->slotCount; i++){
		slot = SP_SLOT(pageBuf,i);
		if (slot->length > 0)
			used += slot->length;
	}
	return used;
}

static int SP_ReserveSlot(char *pageBuf)
{
	SP_PageHeader *header = SP_HEADER(pageBuf);
	SP_SlotEntry *slot;
	int slotId;

	if (header->freeListHead != SP_INVALID_SLOT){
		slotId = header->freeListHead;
		slot = SP_SLOT(pageBuf,slotId);
		header->freeListHead = slot->offset;
		return slotId;
	}

	if (header->slotCount >= SP_MAX_SLOTS)
		return SP_ERR_NOSPACE;

	slotId = header->slotCount;
	header->slotCount++;
	return slotId;
}

static int SP_EnsureSpace(char *pageBuf, int neededBytes)
{
	if (SP_PageFreeSpace(pageBuf) >= neededBytes)
		return SP_OK;
	SP_CompactPage(pageBuf);
	if (SP_PageFreeSpace(pageBuf) >= neededBytes)
		return SP_OK;
	return SP_ERR_NOSPACE;
}

int SP_InsertRecord(char *pageBuf, const char *data, short length, short *slotId)
{
	SP_PageHeader *header = SP_HEADER(pageBuf);
	SP_SlotEntry *slot;
	int needSlotBytes = (header->freeListHead == SP_INVALID_SLOT) ?
				sizeof(SP_SlotEntry) : 0;
	int needed = length + needSlotBytes;
	short dest;
	int id;

	if (length <= 0)
		return SP_ERR_NOSPACE;

	if (SP_EnsureSpace(pageBuf,needed) != SP_OK)
		return SP_ERR_NOSPACE;

	dest = header->freePtr - length;
	if (dest < sizeof(SP_PageHeader))
		return SP_ERR_NOSPACE;

	memcpy(pageBuf + dest,data,length);
	header->freePtr = dest;

	id = SP_ReserveSlot(pageBuf);
	if (id < 0)
		return id;
	slot = SP_SLOT(pageBuf,id);
	slot->offset = dest;
	slot->length = length;

	if (slotId != NULL)
		*slotId = (short)id;
	return SP_OK;
}

int SP_DeleteRecord(char *pageBuf, short slotId)
{
	SP_PageHeader *header = SP_HEADER(pageBuf);
	SP_SlotEntry *slot;

	if (slotId < 0 || slotId >= header->slotCount)
		return SP_ERR_INVALID_SLOT;

	slot = SP_SLOT(pageBuf,slotId);
	if (slot->length <= 0)
		return SP_ERR_INVALID_SLOT;

	slot->length = -1;
	slot->offset = header->freeListHead;
	header->freeListHead = slotId;
	return SP_OK;
}

int SP_GetRecord(char *pageBuf, short slotId, char **dataPtr, short *length)
{
	SP_PageHeader *header = SP_HEADER(pageBuf);
	SP_SlotEntry *slot;

	if (slotId < 0 || slotId >= header->slotCount)
		return SP_ERR_INVALID_SLOT;

	slot = SP_SLOT(pageBuf,slotId);
	if (slot->length <= 0)
		return SP_ERR_INVALID_SLOT;

	if (dataPtr != NULL)
		*dataPtr = pageBuf + slot->offset;
	if (length != NULL)
		*length = slot->length;
	return SP_OK;
}

int SP_GetNextRecord(char *pageBuf, short *cursor, char **dataPtr, short *length)
{
	SP_PageHeader *header = SP_HEADER(pageBuf);
	int i;
	short start = (cursor == NULL || *cursor < 0) ? 0 : (*cursor + 1);
	SP_SlotEntry *slot;

	for (i=start; i<header->slotCount; i++){
		slot = SP_SLOT(pageBuf,i);
		if (slot->length > 0){
			if (cursor != NULL)
				*cursor = i;
			if (dataPtr != NULL)
				*dataPtr = pageBuf + slot->offset;
			if (length != NULL)
				*length = slot->length;
			return SP_OK;
		}
	}
	if (cursor != NULL)
		*cursor = -1;
	return SP_ERR_EMPTY;
}

static void SP_CompactPage(char *pageBuf)
{
	SP_PageHeader *header = SP_HEADER(pageBuf);
	int active = 0;
	short order[SP_MAX_SLOTS];
	int i,j;
	SP_SlotEntry *slot;
	short freePtr = PF_PAGE_SIZE;

	for (i=0; i<header->slotCount; i++){
		slot = SP_SLOT(pageBuf,i);
		if (slot->length > 0){
			order[active++] = i;
		}
	}

	for (i=0; i<active; i++){
		for (j=i+1; j<active; j++){
			SP_SlotEntry *slotI = SP_SLOT(pageBuf,order[i]);
			SP_SlotEntry *slotJ = SP_SLOT(pageBuf,order[j]);
			if (slotI->offset < slotJ->offset){
				short tmp = order[i];
				order[i] = order[j];
				order[j] = tmp;
			}
		}
	}

	for (i=0; i<active; i++){
		slot = SP_SLOT(pageBuf,order[i]);
		freePtr -= slot->length;
		if (freePtr < sizeof(SP_PageHeader))
			freePtr = sizeof(SP_PageHeader);
		if (slot->offset != freePtr)
			memmove(pageBuf + freePtr,pageBuf + slot->offset,slot->length);
		slot->offset = freePtr;
	}
	header->freePtr = freePtr;
}


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "block_records.h"
#include "hp_file.h"
#include "record.h"

/* Βοηθητικό macro: καλεί μια συνάρτηση του επιπέδου BF και, σε περίπτωση
   σφάλματος, εκτυπώνει την περιγραφή του και επιστρέφει HP_ERROR */
#define CALL_BF(call)         \
  {                           \
    BF_ErrorCode code = call; \
    if (code != BF_OK) {      \
      BF_PrintError(code);    \
      return HP_ERROR;        \
    }                         \
  }
//metadata blocks
static BF_Block *meta_block[BF_MAX_OPEN_FILES];
static HP_info  *meta_info[BF_MAX_OPEN_FILES];

/* Βοηθητικές δομές */
typedef struct Note {
  struct BF_Block *pin_block;
  struct HP_info *pin_info;
} Note;

/* Βοηθητικό array */
static struct Note notes[BF_MAX_OPEN_FILES];

HP_ErrorCode HP_CreateFile(const char *fileName) {
  /* TODO: δημιουργήστε το αρχείο, δεσμεύστε το πρώτο μπλοκ και αποθηκεύστε
     εκεί ένα στιγμιότυπο της HP_info */
  int fd;
  BF_Block *block;

  CALL_BF(BF_CreateFile(fileName));
  CALL_BF(BF_OpenFile(fileName, &fd));

  BF_Block_Init(&block);
  CALL_BF(BF_AllocateBlock(fd, block));

  HP_info *info = (HP_info *)BF_Block_GetData(block);
  memset(info, 0, sizeof(HP_info));
  strcpy(info->magic, "HEAPFILE");
  info->last_block = 0;             
  info->records_in_last_block = 0;
  info->records_per_block = RECORDS_PER_BLOCK;
  info->it_active = 0;

  BF_Block_SetDirty(block);
  CALL_BF(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);
  CALL_BF(BF_CloseFile(fd));
  return HP_OK;
}

HP_ErrorCode HP_OpenFile(const char *fileName, int *fileDesc) {
  /* TODO: ανοίξτε το αρχείο, διαβάστε τα μεταδεδομένα από το πρώτο μπλοκ και
     ελέγξτε ότι πρόκειται όντως για αρχείο σωρού */
  int fd;
  BF_Block *block;

  CALL_BF(BF_OpenFile(fileName, &fd));

  BF_Block_Init(&block);
  if (BF_GetBlock(fd, 0, block) != BF_OK) {
    BF_Block_Destroy(&block);
    BF_CloseFile(fd);
    return HP_ERROR;
  }

  HP_info *info = (HP_info *)BF_Block_GetData(block);

  if (strncmp(info->magic, "HEAPFILE", sizeof(info->magic)) != 0) {
    BF_UnpinBlock(block);
    BF_Block_Destroy(&block);
    BF_CloseFile(fd);
    return HP_ERROR;
  }

  info->it_active = 0;
  info->it_value  = 0;
  info->it_block  = 0;
  info->it_slot   = 0;
  BF_Block_SetDirty(block);

  meta_block[fd] = block;
  meta_info[fd]  = info;

  *fileDesc = fd;
  return HP_OK;
}

HP_ErrorCode HP_CloseFile(int fileDesc) {
  /* TODO: γράψτε στον δίσκο ό,τι εκκρεμεί και κλείστε το αρχείο */
  if (fileDesc < 0 || fileDesc >= BF_MAX_OPEN_FILES ||
      meta_block[fileDesc] == NULL)
    return HP_ERROR;

  BF_Block_SetDirty(meta_block[fileDesc]);
  CALL_BF(BF_UnpinBlock(meta_block[fileDesc]));
  BF_Block_Destroy(&meta_block[fileDesc]);
  meta_block[fileDesc] = NULL;
  meta_info[fileDesc]  = NULL;

  CALL_BF(BF_CloseFile(fileDesc));
  return HP_OK;
}

HP_ErrorCode HP_InsertEntry(int fileDesc, Record record) {
  if (fileDesc < 0 || fileDesc >= BF_MAX_OPEN_FILES || notes[fileDesc].pin_info == NULL) {
    fprintf(stderr, "Invalid fileDesc: %d\n", fileDesc);
    return HP_ERROR;
  }
  HP_info *info = notes[fileDesc].pin_info;
  int new_block_needed;
  if (info->last_block == 0 || info->records_in_last_block >= info->records_per_block) 
  {
    new_block_needed = 1;
  } else {
    new_block_needed = 0;
  }
  BF_Block *block;
  BF_Block_Init(&block);
  if (new_block_needed) {
    if (BF_AllocateBlock(fileDesc, block) != BF_OK) 
    {
      BF_PrintError(BF_ERROR);
      BF_Block_Destroy(&block);
      return HP_ERROR;
    }
    info->last_block++;
    info->records_in_last_block = 0;
  } else {
    if (BF_GetBlock(fileDesc, info->last_block, block) != BF_OK) 
    {
      BF_PrintError(BF_ERROR);
      BF_Block_Destroy(&block);
      return HP_ERROR;
    }
  }
  if (blockSetRecord(block, info->records_in_last_block, record) != 0) 
  {
    BF_PrintError(BF_ERROR);
    BF_UnpinBlock(block);
    BF_Block_Destroy(&block);  
    return HP_ERROR;
  }
  info->records_in_last_block++;
  BF_Block_SetDirty(notes[fileDesc].pin_block);
  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);
  return HP_OK;
}

HP_ErrorCode HP_PrintAllEntries(int fileDesc, char *fieldName, void *value) {
  /* TODO: διατρέξτε τα μπλοκ του αρχείου και εκτυπώστε τις εγγραφές που στο
     πεδίο fieldName έχουν τιμή value. Αν value == NULL, εκτυπώστε τις όλες. */
  return HP_ERROR;
}

HP_ErrorCode HP_GetEntry(int fileDesc, int blockid, int position, Record *record) {
  if (fileDesc < 0 || fileDesc >= BF_MAX_OPEN_FILES || notes[fileDesc].pin_info == NULL) {
    fprintf(stderr, "Invalid fileDesc: %d\n", fileDesc);
    if (record != NULL) {
      memset(record, 0, sizeof(Record));
    }
    return HP_ERROR;
  }
  if (blockid <= 0 || blockid > notes[fileDesc].pin_info->last_block) {
    fprintf(stderr, "Invalid blockid: %d\n", blockid);
    if (record != NULL) {
      memset(record, 0, sizeof(Record));
    }
    return HP_ERROR;
  }
  int block_count;
  if (blockid < notes[fileDesc].pin_info->last_block)
    block_count = notes[fileDesc].pin_info->records_per_block;
  else
    block_count = notes[fileDesc].pin_info->records_in_last_block;
  if (position < 0 || position >= block_count) {
    fprintf(stderr, "Invalid position: %d\n", position);
    if (record != NULL) {
      memset(record, 0, sizeof(Record));
    }
    return HP_ERROR;
  }
  BF_Block *block;
  BF_Block_Init(&block);
  BF_ErrorCode bf_err = BF_GetBlock(fileDesc, blockid, block);
  if (bf_err != BF_OK) 
  {
    BF_PrintError(bf_err);
    BF_Block_Destroy(&block);
    memset(record, 0, sizeof(Record));
    return HP_ERROR;
  } 
  if (blockGetRecord(block, position, record) != 0) 
  {
    fprintf(stderr, "Error retrieving record at block %d, position %d\n", blockid, position);
    BF_UnpinBlock(block);
    BF_Block_Destroy(&block);
    memset(record, 0, sizeof(Record));
    return HP_ERROR;
  }
  BF_UnpinBlock(block);
  BF_Block_Destroy(&block);
  
  return HP_OK;
}

HP_ErrorCode HP_GetNextRecordWithID(int fileDesc, Record *record, void *value) {
  if (fileDesc < 0 || fileDesc >= BF_MAX_OPEN_FILES )
  {
    fprintf(stderr, "Invalid fileDesc: %d\n", fileDesc);
    return HP_ERROR;
  }
  if (!record || !value) 
  {
    fprintf(stderr, "Invalid record or value pointer\n");
    return HP_ERROR;
  }
  int check_target = *(int *)value;
  //If no active search, we start from the beginning
  if (notes[fileDesc].pin_info->it_active == 0 || notes[fileDesc].pin_info->it_value != check_target)
  {
    notes[fileDesc].pin_info->it_active = 1;
    notes[fileDesc].pin_info->it_value = check_target;
    notes[fileDesc].pin_info->it_block = 1;
    notes[fileDesc].pin_info->it_slot = 0;
  }
  BF_Block *block;
  BF_Block_Init(&block);
  while (notes[fileDesc].pin_info->it_block <= notes[fileDesc].pin_info->last_block)
  {
    //Find how many records are in the current block
    int block_count;
    if (notes[fileDesc].pin_info->it_block < notes[fileDesc].pin_info->last_block)
      block_count = notes[fileDesc].pin_info->records_per_block;
    else
      block_count = notes[fileDesc].pin_info->records_in_last_block;
    if (BF_GetBlock(fileDesc, notes[fileDesc].pin_info->it_block, block) != BF_OK)
    {
      BF_PrintError(BF_ERROR);
      BF_Block_Destroy(&block);
      return HP_ERROR;
    }
    for (int i = notes[fileDesc].pin_info->it_slot; i < block_count; i++)
    {
      Record loc_rec;
      blockGetRecord(block, i, &loc_rec);
      if (loc_rec.id == check_target)
      {
        *record = loc_rec;
        notes[fileDesc].pin_info->it_slot = i + 1;
        BF_UnpinBlock(block);
        BF_Block_Destroy(&block);
        return HP_OK;
      }
    }
  }
  notes[fileDesc].pin_info->it_active = 0;
  BF_Block_Destroy(&block);
  return HP_EOF;

  return HP_OK;
}

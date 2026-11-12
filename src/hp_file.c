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

/* Βοηθητικές δομές για μεταδεδομένα */
typedef struct Note {
  struct BF_Block *pin_block;
  struct HP_info *pin_info;
} Note;

/* Βοηθητικό array */
static struct Note notes[BF_MAX_OPEN_FILES];

/* Βοηθητική συνάρτηση για PrintAllEntries */
static int compare_record_fields(Record *record, char *fieldName, void *value) 
{
  if (value == NULL) 
  {
    //Everything matches, so we print everything
    return 1; 
  }
  if (strcmp(fieldName, "id") == 0) 
  {
    return record->id == *(int *)value;
  } 
  else if (strcmp(fieldName, "firstname") == 0) 
  {
    return strcmp(record->firstname, (char *)value) == 0;
  } 
  else if (strcmp(fieldName, "lastname") == 0) 
  {
    return strcmp(record->lastname, (char *)value) == 0;
  } 
  else if (strcmp(fieldName, "postcode") == 0) 
  {
    return record->postcode == *(int *)value;
  } 
  else if (strcmp(fieldName, "city") == 0) 
  {
    return strcmp(record->city, (char *)value) == 0;
  }
  return 0; // Invalid field name
}

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

  notes[fd].pin_block = block;
  notes[fd].pin_info = info;

  *fileDesc = fd;
  return HP_OK;
}

HP_ErrorCode HP_CloseFile(int fileDesc) {
  /* TODO: γράψτε στον δίσκο ό,τι εκκρεμεί και κλείστε το αρχείο */
  if (fileDesc < 0 || fileDesc >= BF_MAX_OPEN_FILES ||
      notes[fileDesc].pin_info == NULL)
    return HP_ERROR;

  BF_Block_SetDirty(notes[fileDesc].pin_block);
  CALL_BF(BF_UnpinBlock(notes[fileDesc].pin_block));
  BF_Block_Destroy(&notes[fileDesc].pin_block);
  notes[fileDesc].pin_block = NULL;
  notes[fileDesc].pin_info = NULL;

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
  if (fileDesc < 0 || fileDesc >= BF_MAX_OPEN_FILES )
  {
    fprintf(stderr, "Invalid fileDesc: %d\n", fileDesc);
    return HP_ERROR;
  }
  if (value != NULL && fieldName == NULL) 
  {
    fprintf(stderr, "Don't know where to search\n");
    return HP_ERROR;
  }
  if (value != NULL && (strcmp(fieldName, "id") != 0 && strcmp(fieldName, "firstname") != 0 &&
      strcmp(fieldName, "lastname") != 0 && strcmp(fieldName, "postcode") != 0 &&
      strcmp(fieldName, "city") != 0)) 
  {
    fprintf(stderr, "Invalid field name: %s\n", fieldName);
    return HP_ERROR;
  }
  if (notes[fileDesc].pin_info == NULL) 
  {
    fprintf(stderr, "File not opened\n");
    return HP_ERROR;
  }
  BF_Block *block;
  BF_Block_Init(&block);
  for (int i = 1; i <= notes[fileDesc].pin_info->last_block; i++) 
  {
    if (BF_GetBlock(fileDesc, i, block) != BF_OK) 
    {
      BF_PrintError(BF_ERROR);
      BF_Block_Destroy(&block);
      return HP_ERROR;
    }
    int block_count;
    if (i < notes[fileDesc].pin_info->last_block)
      block_count = notes[fileDesc].pin_info->records_per_block;
    else
      block_count = notes[fileDesc].pin_info->records_in_last_block;
    for (int j = 0; j < block_count; j++)
    {
      Record record;
      if (blockGetRecord(block, j, &record) != 0)
      {
        fprintf(stderr, "Error retrieving record at block %d, position %d\n", i, j);
        BF_UnpinBlock(block);
        BF_Block_Destroy(&block);
        return HP_ERROR;
      }
      if (compare_record_fields(&record, fieldName, value))
      {
        printRecord(record);
      }
    }
    BF_UnpinBlock(block);
  }
  BF_Block_Destroy(&block);
  return HP_OK;
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
  if (notes[fileDesc].pin_info == NULL) 
  {
    fprintf(stderr, "File not opened\n");
    return HP_ERROR;
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
        BF_Block_SetDirty(notes[fileDesc].pin_block);
        BF_UnpinBlock(block);
        BF_Block_Destroy(&block);
        return HP_OK;
      }
    }
    BF_UnpinBlock(block);
    notes[fileDesc].pin_info->it_block++;
    notes[fileDesc].pin_info->it_slot = 0;
  }
  notes[fileDesc].pin_info->it_active = 0;
  BF_Block_SetDirty(notes[fileDesc].pin_block);
  BF_Block_Destroy(&block);

  return HP_EOF;
}

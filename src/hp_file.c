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
     return HP_ERROR;
}

HP_ErrorCode HP_OpenFile(const char *fileName, int *fileDesc) {
  /* TODO: ανοίξτε το αρχείο, διαβάστε τα μεταδεδομένα από το πρώτο μπλοκ και
     ελέγξτε ότι πρόκειται όντως για αρχείο σωρού */
  return HP_ERROR;
}

HP_ErrorCode HP_CloseFile(int fileDesc) {
  /* TODO: γράψτε στον δίσκο ό,τι εκκρεμεί και κλείστε το αρχείο */
  return HP_ERROR;
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
  /* TODO: υλοποιήστε τον iterator. Η κατάσταση του (πού σταμάτησε η
     προηγούμενη κλήση) πρέπει να επιβιώνει ανάμεσα στις κλήσεις. */
  return HP_ERROR;
}

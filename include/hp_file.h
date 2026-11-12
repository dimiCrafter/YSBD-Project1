#ifndef HP_FILE_H
#define HP_FILE_H

#include "record.h"

/* Κωδικοί επιστροφής των συναρτήσεων του επιπέδου σωρού */
typedef enum HP_ErrorCode {
  HP_OK,
  HP_ERROR,
  HP_EOF
} HP_ErrorCode;

/*
 * Η δομή HP_info κρατάει τα μεταδεδομένα ενός αρχείου σωρού.
 *
 * Τον σχεδιασμό της τον κάνετε εσείς. Πληροφορίες που μπορεί να χρειαστεί να
 * περιλάβετε είναι το id του τελευταίου μπλοκ του αρχείου, το μέγιστο πλήθος
 * εγγραφών ανά μπλοκ, και ό,τι άλλο κρίνετε απαραίτητο - για παράδειγμα για τη
 * διαχείριση του iterator της HP_GetNextRecordWithID.
 */
typedef struct HP_info {
    char magic[9];
    int last_block;
    int records_in_last_block;
    int records_per_block;
    int it_active;
    int it_value;
    int it_block;
    int it_slot;
} HP_info;

/* Δημιουργεί και αρχικοποιεί ένα άδειο αρχείο σωρού με όνομα fileName. Αν το
   αρχείο υπάρχει ήδη, επιστρέφεται κωδικός λάθους. */
HP_ErrorCode HP_CreateFile(
    const char *fileName        /* όνομα αρχείου */)
{
    //file descriptor and block pointer
    int fd;
    BF_Block *block;
    CALL_BF(BF_CreateFile(fileName));
    CALL_BF(BF_OpenFile(fileName, &fd));

    BF_Block_Init(&block);
    CALL_BF(BF_AllocateBlock(fd, block));
    //metadata intialization for heap file
    HP_info *info = (HP_info *)BF_Block_GetData(block);
    memset(info, 0, sizeof(HP_info));
    strcpy(info->magic, "HEAPFILE");        
    info->last_block = 0;                   
    info->records_in_last_block = 0;
    info->records_per_block = RECORDS_PER_BLOCK;
    //initializing iterator values 
    BF_Block_SetDirty(block);
    CALL_BF(BF_UnpinBlock(block));
    //free the blocks and close file
    BF_Block_Destroy(&block);
    CALL_BF(BF_CloseFile(fd));
    return HP_OK;
}

/* Ανοίγει το αρχείο fileName και διαβάζει από το πρώτο μπλοκ τα
   metadata του αρχείου σωρού. Επιστρέφει στη fileDesc τον αναγνωριστικό
   αριθμό ανοίγματος, όπως τον έδωσε το επίπεδο μπλοκ. Αν το αρχείο δεν είναι
   αρχείο σωρού, θεωρείται περίπτωση σφάλματος. */
HP_ErrorCode HP_OpenFile(
    const char *fileName,       /* όνομα αρχείου */
    int *fileDesc               /* αναγνωριστικός αριθμός ανοίγματος */){
        

    };

/* Κλείνει το αρχείο που προσδιορίζεται από τον fileDesc. */
HP_ErrorCode HP_CloseFile(
    int fileDesc                /* αναγνωριστικός αριθμός ανοίγματος */);

/* Εισάγει την εγγραφή record στο τέλος του αρχείου σωρού, μετά την τρέχουσα
   τελευταία εγγραφή. */
HP_ErrorCode HP_InsertEntry(
    int fileDesc,               /* αναγνωριστικός αριθμός ανοίγματος */
    Record record               /* η εγγραφή προς εισαγωγή */);

/* Εκτυπώνει όλες τις εγγραφές που στο πεδίο fieldName έχουν τιμή value. Το
   fieldName είναι ένα από τα πεδία που εμφανίζονται στο record.h. Αν το value
   είναι NULL, εκτυπώνονται όλες οι εγγραφές του αρχείου. */
HP_ErrorCode HP_PrintAllEntries(
    int fileDesc,               /* αναγνωριστικός αριθμός ανοίγματος */
    char *fieldName,            /* όνομα πεδίου αναζήτησης */
    void *value                 /* τιμή του πεδίου */);

/* Επιστρέφει στη record την εγγραφή που βρίσκεται στη θέση position του μπλοκ
   blockid. Αν δεν υπάρχει εγγραφή σε αυτή τη θέση, επιστρέφεται HP_ERROR. */
HP_ErrorCode HP_GetEntry(
    int fileDesc,               /* αναγνωριστικός αριθμός ανοίγματος */
    int blockid,                /* αριθμός μπλοκ */
    int position,               /* θέση της εγγραφής μέσα στο μπλοκ */
    Record *record              /* η εγγραφή που επιστρέφεται */);

/* Επιστρέφει στη record την επόμενη εγγραφή που στο πεδίο id έχει τιμή value.
   Όταν δεν υπάρχει άλλη εγγραφή, επιστρέφεται HP_ERROR. Απαιτεί κάποια μορφή
   iterator, του οποίου η κατάσταση πρέπει να είναι διαθέσιμη στα μετα-δεδομένα
   του επιπέδου HP_. Ο σχεδιασμός του μηχανισμού είναι δικός σας. */
HP_ErrorCode HP_GetNextRecordWithID(
    int fileDesc,               /* αναγνωριστικός αριθμός ανοίγματος */
    Record *record,             /* η εγγραφή που επιστρέφεται */
    void *value                 /* η τιμή του id που αναζητείται */);

#endif  // HP_FILE_H

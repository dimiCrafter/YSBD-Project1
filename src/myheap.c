#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "bf.h"
#include "hp_file.h"
#include "record.h"

/* Τα αρχεία βάσης δημιουργούνται στον κατάλογο DBs/ του project */
#define DB_FILE "DBs/myheap.db"

/*
 * O driver της ασκησης.
 *
 * Το εκτελέσιμο πρέπει να διαβάζει τις εγγραφές που αποθηκεύει από ένα αρχείο
 * κειμένου, το οποίο υποδεικνύεται με τη σημαία -i:
 *
 *     ./bin/myheap -i inputs/records.txt
 *
 * Κάθε γραμμή του αρχείου περιέχει μία εγγραφή, με τα πεδία χωρισμένα με κενά
 * και στη σειρά που έχουν στη δομή Record:
 *
 *     id firstname lastname postcode city
 *     151 Dimitra Stasinou 12376 Ioannina
 *
 * Ο driver θα πρέπει να δημιουργεί το αρχείο σωρού, να εισάγει σε αυτό τις
 * εγγραφές του αρχείου εισόδου, και να επιδεικνύει τη λειτουργία των
 * συναρτήσεων του API.
 */

int main(int argc, char *argv[]) {
  if (argc != 3 || strcmp(argv[1], "-i") != 0) 
  {
    fprintf(stderr, "Usage: %s -i <input_file.txt>\n", argv[0]);
    return EXIT_FAILURE;
  }
  FILE* input;
  input = fopen(argv[2], "r");
  if (input == NULL) {
    //Printing wich file i couldn't open
    perror(argv[2]);
    return EXIT_FAILURE;
  }
  fprintf(stdout, "Opened file %s\n", argv[2]);
  char array[100];
  int count = 0;
  Record record;
  while (fgets(array, sizeof(array), input) != NULL) {
    if(sscanf(array, "%d %24s %19s %d %19s", &record.id, record.firstname, record.lastname, &record.postcode, record.city) != 5) {
      fprintf(stderr, "Error parsing line: %s\n", array);
      continue;
    }
    count++;
    printRecord(record);
  }
  fprintf(stdout, "Number of records processed: %d\n", count);
  fclose(input);
  return EXIT_SUCCESS;
}

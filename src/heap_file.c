#include "heap_file.h"
#include "bf.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*Όπως η συνάρτηση που δόθηκε στο αρχείο bf_main
με τη διαφορά οτι γυρίζει HP_ERROR ωστε να διαχειριστεί
το λάθος η hp_main*/
#define safety(call)          \
{                             \
    BF_ErrorCode code = call; \
    if (code != BF_OK)        \
    {                         \
      BF_PrintError(code);    \
      return HP_ERROR;        \
    }                         \
}

/*Στην περίπτωση αυτής της εργασίας δεν έχει να υλοποιηθεί
κάτι για την συνάρτηση αυτή*/
HP_ErrorCode HP_Init()
{
  return HP_OK;
}

/*Δημηοργεί ενα αρχείο σωρού που ουσιαστικά είναι ενα BF_File
που στο πρώτο block περιέχει το string "HEAP FILE" ώστε να 
αναγνωρίζεται από την υλοποίηση*/
HP_ErrorCode HP_CreateIndex(const char *filename)
{
  int fd;
  BF_Block *block;
  char* data;

  BF_Block_Init(&block);
  safety(BF_CreateFile(filename));
  safety(BF_OpenFile(filename, &fd));
  safety(BF_AllocateBlock(fd, block));
  data = BF_Block_GetData(block);
  strcpy(data,"HEAP_FILE");
  BF_Block_SetDirty(block);
  safety(BF_UnpinBlock(block));
  safety(BF_CloseFile(fd));
  BF_Block_Destroy(&block);
  return HP_OK;
}

/*Εφόσον το αρχείο με όνομα fileName υπάρχει ανοίγεται από 
αυτή τη συνάρτηση και εξετάζεται αν είναι αρχείο σωρού 
(αν περιεχει δηλαδή στο πρώτο block το string "HEAP FILE").
Τέλος επιστρέφεται στον fileDesc ο file descriptor.*/
HP_ErrorCode HP_OpenFile(const char *fileName, int *fileDesc)
{
  BF_Block *block;
  char* data;
  BF_Block_Init(&block);
  safety(BF_OpenFile(fileName, fileDesc));
  safety(BF_GetBlock(*fileDesc,0,block));
  data = BF_Block_GetData(block);
  if(strcmp(data,"HEAP_FILE")!=0){fprintf(stderr, "HP_OpenFile: not a heap file!\n");return HP_ERROR;}
  safety(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);
  return HP_OK;
}

/*Απλώς κλείνει το αρχείο το οποίο είναι ανοιγμένο στον 
fileDesc και επιστρέφει ok*/
HP_ErrorCode HP_CloseFile(int fileDesc) 
{
  safety(BF_CloseFile(fileDesc));
  return HP_OK;
}

/*Εισάγει μια εγγραφή στο τέλος του αρχείου. Κάθε block
στην αρχή του έχει εναν int με τον αριθμό των εγγραφών 
που υπάρχουν ηδη. Κοιτάει πρώτα αν έχει χώρο το τευταίο 
block και εισάγει στο τέλος την εγγραφή ένώ διαφορετικά
κάνει allocate ενα καινουριο block και εισάγει την εγγραφή 
στην αρχή του.Και στις δυο περιπτώσεις ενημερώνεται 
ο αριθμός των εγγραφών που υπάρχει στην αρχή του block*/
HP_ErrorCode HP_InsertEntry(int fileDesc, Record record) 
{
  int blocks_num,entries_in_block=0;
  BF_Block *block;
  void* data;
  BF_Block_Init(&block);
  safety(BF_GetBlockCounter(fileDesc,&blocks_num));
  /*the first block only has general info for the heap
  so if there is only one block dont check if its full
  allocate a new one*/
  if(blocks_num != 1)
  {
    safety(BF_GetBlock(fileDesc,blocks_num-1,block));
    data = BF_Block_GetData(block);
    memcpy(&entries_in_block,data,sizeof(int));//first bytes of a block have the number of entries
    entries_in_block++;
    if(((sizeof(record)*entries_in_block)+sizeof(int)) <= BF_BLOCK_SIZE)//if we can fit one more entry
    {
      memcpy(data,&entries_in_block,sizeof(int));
      memcpy((data+sizeof(int)+((entries_in_block-1)*sizeof(record))),(void*)&record,sizeof(record));
      BF_Block_SetDirty(block);
      safety(BF_UnpinBlock(block));
      BF_Block_Destroy(&block);
      return HP_OK;
    }
    safety(BF_UnpinBlock(block));
  }
  //if we only have one block or the last block of the file is full
  entries_in_block=1;
  safety(BF_AllocateBlock(fileDesc, block));
  data = BF_Block_GetData(block);
  memcpy (data,&entries_in_block,sizeof(int));
  data=data+sizeof(int);
  memcpy(data,&record,sizeof(record));
  BF_Block_SetDirty(block);
  safety(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);
  return HP_OK;
}

/*Εκτυπώνει μια μια τις εγγραφές*/
HP_ErrorCode HP_PrintAllEntries(int fileDesc) 
{
  int i,j,blocks_num,entries_in_block;
  void* data;
  BF_Block *block;
  BF_Block_Init(&block);
  Record *temp_record=NULL;
  safety(BF_GetBlockCounter(fileDesc,&blocks_num))
  safety(BF_GetBlock(fileDesc,1,block));
  memcpy(&entries_in_block,data,sizeof(int));
  for(i = 1; i <blocks_num; i++)
  {
    safety(BF_GetBlock(fileDesc,i,block));
    data = BF_Block_GetData(block);
    memcpy(&entries_in_block,data,sizeof(int));
    data += sizeof(int);
    for (j = 0; j < entries_in_block; j++)
    {
      temp_record=data;
      printf("%d %s %s %s\n",temp_record->id,temp_record->name,temp_record->surname,temp_record->city);
      data += sizeof(Record);
    }
    safety(BF_UnpinBlock(block));
  }
  BF_Block_Destroy(&block);
  return HP_OK;
}

/*Φέρνει την εγγραφή με ΘΕΣΗ rowID επομένως γίνονται οι
απαραίτητοι υπολογισμοί για να βρεθεί που είναι η θέση rowId
και επιστρέφεται στο record η εγγραφη της θέσης αυτής*/
HP_ErrorCode HP_GetEntry(int fileDesc, int rowId, Record *record)
{
  rowId--;//places of the entries are computed from 0 to n
  int blocks_num;
  int entries_per_block = BF_BLOCK_SIZE/sizeof(Record);//find how many entries does one block have
  int home_block = ((rowId/entries_per_block)+1);//find in which block is the entry we are looking for (+1 because of the first info block)
  void* data;
  BF_Block *block;
  BF_Block_Init(&block);


  safety(BF_GetBlockCounter(fileDesc,&blocks_num));//checks if the entry we are looking for is out of range
  if(blocks_num < home_block){fprintf(stderr, "Entry not in the block!\n");return HP_ERROR;}

  safety(BF_GetBlock(fileDesc,home_block,block));
  data = BF_Block_GetData(block);
  
  /*example 17 entries per row
  asking for entry 18 (17 in the db)
  home_block = (17/17)+1 = 2
  place =17-(1*17)=0
  so its the first element of the second block with entries
  */
  int place = rowId-((home_block-1)*entries_per_block);
  data += sizeof(int) + (place)*sizeof(Record);
  memcpy(record,data,sizeof(record));
  safety(BF_UnpinBlock(block));
  BF_Block_Destroy(&block);
  return HP_OK;
}
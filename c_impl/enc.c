#include "openssl_aes.c"
#include <unistd.h>

#define MAXENTRIES 4096

// !!TODO WARN!! try to use something different from gets ASAP
#define gets(X) gets((X))
//#define gets(X) gets_s((X), 20)

#define printd //printf

typedef struct {
  char* name;
  char* user;
  char* pass;
  char* desc;
} PMEntry;

PMEntry columnNames = {"Name", "Username", "Password", "Description"};
PMEntry **entries;
uint32_t entryCount = 0;

EVP_CIPHER_CTX* en;
EVP_CIPHER_CTX* de;

int aes_init_w(void* key, int keylen) {
  en = EVP_CIPHER_CTX_new();
  de = EVP_CIPHER_CTX_new();
  unsigned int salt[] = {1685449954, 133742069};
  aes_init(key, keylen, (unsigned char *)&salt, en, de);
}

void aes_deinit() {
  EVP_CIPHER_CTX_free(en);
  EVP_CIPHER_CTX_free(de);
}

void* enc_buf(char* plain, int* buflen) {
  return aes_encrypt(en, (unsigned char *)plain, buflen);
}

char* dec_buf(void* cipher, int* buflen) {
  return (char *)aes_decrypt(de, cipher, buflen);
}

void* key;
uint32_t keylen = 0;

void get_entries() {
  FILE *fkey, *db;
  fkey = fopen("key.bin","rb");
  db = fopen("passdb.bin","rb");
  int justDefinedKey = 0;
  if (fkey == NULL) {
    printf("This program has been started for the first time,\n"
           "or the key used for the database could not be read.\n");
    while(keylen < 8 || keylen > 2048) {
      printf("Enter new key length: ");
      //printf("(%d) ",keylen); 
      scanf("%d", &keylen);
      printd("DEBUG: scanf\n");
      if(keylen < 8 || keylen > 2048) {
        printf("Invalid key length %d, must be within [8, 2048]\n", keylen);
      }
    }
    printd("DEBUG: got key length\n");
    int randfd = open("/dev/random", 0); // yes, this isn't portable
    key = malloc(keylen);
    int rres = read(randfd, key, keylen);          // too bad!
    printd("DEBUG: read %d bytes from /dev/random with fd %d\n", rres, randfd);
    close(randfd);

    FILE *fkeyw = fopen("key.bin","wb");
    printd("DEBUG: opening key file\n");
    fwrite(&keylen, sizeof(uint32_t), 1, fkeyw);
    printd("DEBUG: writing key\n");
    if(fwrite(key, keylen, 1, fkeyw) == 0) {
      printf("WARN: could not write key to disk.\n");
    }
    printd("DEBUG: closing key file\n");
    fclose(fkeyw);
    justDefinedKey = 1;
  } else {
    printd("DEBUG: opening key file for read\n");
    fread(&keylen, sizeof(uint32_t), 1, fkey);
    key = malloc(keylen);
    fread(key, keylen, 1, fkey);
    fclose(fkey);
  }
  printd("DEBUG: key read\n");
  entries = (PMEntry**) calloc(MAXENTRIES,sizeof(PMEntry));
  entries[0] = &columnNames;

  aes_init_w(key, keylen);
  printd("DEBUG: AES init\n");
  if (db == NULL) {
    if(!justDefinedKey)
      printf("WARN: Could not open database file.\n");
  } else {
    fread(&entryCount, sizeof(uint32_t), 1, db);
    int bufl;
    for(int i=1; i<entryCount+1; i++){
      printd("DEBUG: attempting to read entry %d\n", i-1);
      PMEntry *entry = malloc(sizeof(PMEntry));
      int j = 0;
      while(j < 4) {
        fread(&bufl, sizeof(uint32_t), 1, db);
        printd("DEBUG: entry %d has length %d\n", i-1, bufl);
        void *tbuf = calloc(1,bufl+1);
        fread(tbuf, bufl, 1, db);
        switch(j) {
          case 0:
            entry->name = (char*)tbuf;
          break;
          case 1:
            entry->user = dec_buf(tbuf, &bufl);
          break;
          case 2:
            entry->pass = dec_buf(tbuf, &bufl);
          break;
          case 3:
            entry->desc = (char*)tbuf;
          break;
        }
        j++;
      }
      entries[i] = entry;
    }
    fclose(db);
  }
  printd("DEBUG: end get_entries\n");
}

void sync_entries() {
  FILE *db;
  if ((db = fopen("passdb.bin","wb")) == NULL) {
    printf("Could not write to database, unable to open file.");
    return;
  }
  printd("DEBUG: writing entryCount = %d\n", entryCount);
  printd("DEBUG: fwrite(%d, %d, %d, %d);\n", &entryCount, sizeof(uint32_t), 1, db);
  fwrite(&entryCount, sizeof(uint32_t), 1, db);
  for(int i=1; i<entryCount+1; i++){
    PMEntry *entry = entries[i];
    printd("DEBUG: attempting to write entry %d\n", i);
    int j = 0;
    while(j < 4) {
      void *tbuf;
      int bufl;
      switch(j) {
        case 0:
          bufl = strlen(entry->name);
          tbuf = entry->name;
        break;
        case 1:
          bufl = strlen(entry->user);
          tbuf = enc_buf(entry->user, &bufl);
        break;
        case 2:
          bufl = strlen(entry->pass);
          tbuf = enc_buf(entry->pass, &bufl);
        break;
        case 3:
          bufl = strlen(entry->desc);
          tbuf = entry->desc;
        break;
      }
      printd("DEBUG: attempting to write length %d\n", bufl);
      fwrite(&bufl, sizeof(uint32_t), 1, db);
      printd("DEBUG: attempting to write subentry %d of entry %d\n", j, i-1);
      fwrite(tbuf, bufl, 1, db);
      j++;
    }
  }
}

int add_entry(PMEntry* entry) {
  if (entryCount+1 == MAXENTRIES) {
    printf("Could not add new entry - out of allocated memory.");
    return 1;
  }
  entries[entryCount+1] = entry;
  ++entryCount;
  sync_entries();
  return 0;
}

int remove_entry(int id) {
  PMEntry* temp = entries[entryCount];
  PMEntry* removed = entries[id+1];
  //free(removed);
  entries[id+1] = temp;
  --entryCount;
  sync_entries();
  return 0;
}
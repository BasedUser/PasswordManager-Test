#include "enc.c"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define false 0
#define true 1

#define MAXENTRIES 4096

typedef struct {
  char* name;
  char* user;
  char* pass;
  char* desc;
} PMEntry;

PMEntry columnNames = {"Name", "Username", "Password", "Description"};
PMEntry **entries;
uint32_t entryCount = 0;

#ifdef USEGTK
#include <gtk/gtk.h>

// since all of the widgets are hardcoded, i'm using define here as an abstraction
// the specific definition here is unsafe (prone to xml injection?) but it's basically useless until i figure out how to handle callback data
#define NAMEBUF gtk_entry_get_buffer(gtk_grid_get_child_at(newPairGrid, 0,1))
#define USERBUF gtk_entry_get_buffer(gtk_grid_get_child_at(newPairGrid, 0,2))
#define PASSBUF gtk_entry_get_buffer(gtk_grid_get_child_at(newPairGrid, 0,3))
#define REMKBUF gtk_entry_get_buffer(gtk_grid_get_child_at(newPairGrid, 0,4))

// i'm new to GTK, so i will rebuild UI as needed via setting the child of the main app window (would be better to have multiple windows? eh what the hell)
static GtkWidget *window;
static GtkWidget *mainGrid; // grid displaying all name/user/pass/remark tuples, as well as the copy and delete buttons
static GtkWidget *keyGenGrid; // grid containing the "generate new AES key with length" dialog
static GtkWidget *newPairGrid; // grid containing the "add new entry" dialog

static void pwmgr_handle_new(GtkWidget *widget, gpointer data) { // -> encrypt?
  g_print("Entry name: %s\nUsername: %s\nPassword: %s\nDescription: %s\n",
          gtk_entry_buffer_get_text(NAMEBUF),
          gtk_entry_buffer_get_text(USERBUF),
          gtk_entry_buffer_get_text(PASSBUF),
          gtk_entry_buffer_get_text(REMKBUF)
  );
}

static void pwmgr_return_main(GtkWidget *widget, gpointer data) { // called when the new user/pass pair dialog is cancelled/processed
  gtk_entry_buffer_delete_text(NAMEBUF, 0, -1); // set to ""
  gtk_entry_buffer_delete_text(USERBUF, 0, -1);
  gtk_entry_buffer_delete_text(PASSBUF, 0, -1);
  gtk_entry_buffer_delete_text(REMKBUF, 0, -1);
  // placeholder - replace with null
  gtk_window_set_child(widget, mainGrid);
}

static void pwmgr_add_entry(GtkWidget *widget, gpointer data) {
  
}

static void pwmgr_delete_entry(GtkWidget *widget, gpointer data) {
  
}

static void pwmgr_copy_password(GtkWidget *widget, gpointer data) {
  
}

static void setup_list(GtkListItemFactory *factory, GtkListItem *list_item) {
  GtkWidget *locwidg;
  int index = gtk_list_item_get_position(list_item);
  if (index > entryCount) {
    gtk_list_item_set_child (list_item, NULL);
    return;
  }
  locwidg = gtk_grid_new();
  gtk_grid_set_column_homogeneous(GTK_GRID(locwidg), true);

  GtkWidget *nameLabel = gtk_label_new(NULL);
  GtkWidget *userLabel = gtk_label_new(NULL);
  GtkWidget *descLabel = gtk_label_new(NULL);
  if (index == 0) {
    GtkWidget *copyBtn = gtk_button_new();
    GtkWidget *delBtn = gtk_button_new();
  }
  GtkWidget *copyBtn = gtk_button_new_with_label("Copy");
  GtkWidget *delBtn = gtk_button_new_with_label("×");

  gtk_grid_attach(GTK_GRID(locwidg), nameLabel, 0, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(locwidg), userLabel, 1, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(locwidg), descLabel, 2, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(locwidg), copyBtn, 3, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(locwidg), delBtn, 4, 0, 1, 1);

  gtk_list_item_set_child (list_item, locwidg);
}

static void bind_list(GtkListItemFactory *factory, GtkListItem *list_item)
{
  GtkWidget *locwidg;
  int index = gtk_list_item_get_position(list_item);
  if (index > entryCount) {
    g_print("Tried to bind item #%d, is the listview full?",index);
    return;
  }
  PMEntry *entry = entries[index];
  locwidg = gtk_list_item_get_child(list_item);
  gtk_label_set_label(gtk_grid_get_child_at(locwidg, 0, 0), entry->name);
  gtk_label_set_label(gtk_grid_get_child_at(locwidg, 1, 0), entry->user);
  gtk_label_set_label(gtk_grid_get_child_at(locwidg, 2, 0), entry->desc);
  if (index) {
    g_signal_connect_swapped(gtk_grid_get_child_at(locwidg, 2, 0), "clicked", G_CALLBACK(pwmgr_copy_password), index);
    g_signal_connect_swapped(gtk_grid_get_child_at(locwidg, 3, 0), "clicked", G_CALLBACK(pwmgr_delete_entry), index);
  }
}

static void activate_list (GtkListView *list, guint position, gpointer __data) {
  GAppInfo *app_info;

  app_info = g_list_model_get_item (G_LIST_MODEL (gtk_list_view_get_model (list)), position);
  g_app_info_launch (app_info, NULL, NULL, NULL);
  g_object_unref (app_info);
}

static void activate(GtkApplication *app, gpointer user_data) {
  // ideally, this would be dynamic length and lots of memcpying would happen
  // 4096 is (more than) enough for this test usecase
  entries = (PMEntry**) calloc(MAXENTRIES,sizeof(PMEntry));
  entries[0] = &columnNames;

  // main window
  GtkWidget *valuesListView;
  GtkWidget *goNewEntry; // add new pair button

  // new pair dialog
  GtkWidget *newEntryLabel;
  GtkWidget *nametext;
  GtkWidget *usertext;
  GtkWidget *passtext;
  GtkWidget *remarkstext;
  GtkWidget *cancel;
  GtkWidget *confirmAdd;

  // create the base window
  window = gtk_application_window_new(app);
  gtk_window_set_title(GTK_WINDOW(window), "Password Manager");
  gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);

  // main dialog
  mainGrid = gtk_grid_new();
  // gtk_window_set_child(GTK_WINDOW (window), mainGrid);
  gtk_widget_set_halign(mainGrid, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(mainGrid, GTK_ALIGN_CENTER);

  GtkScrolledWindow *scroller = gtk_scrolled_window_new();
  GtkSignalListItemFactory *factory = gtk_signal_list_item_factory_new();
  g_signal_connect(factory, "setup", setup_list, NULL);
  g_signal_connect(factory, "bind", bind_list, NULL);
  valuesListView = gtk_list_view_new(NULL, factory);

  g_signal_connect(valuesListView, "activate", G_CALLBACK(activate_list), NULL);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scroller), valuesListView);
  gtk_grid_attach(GTK_GRID(mainGrid), scroller, 0, 0, 1, 1);

  goNewEntry = gtk_button_new_with_label("+");
  gtk_grid_attach(GTK_GRID(mainGrid), goNewEntry, 0, 1, 1, 1);
  gtk_widget_set_halign(goNewEntry, GTK_ALIGN_END);
  gtk_widget_set_valign(goNewEntry, GTK_ALIGN_END);

  // new entry dialog
  newPairGrid = gtk_grid_new();

  gtk_window_set_child(GTK_WINDOW(window), newPairGrid);
  gtk_widget_set_halign(newPairGrid, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(newPairGrid, GTK_ALIGN_CENTER);

  newEntryLabel = gtk_label_new("Enter new user data");
  gtk_widget_set_halign(newEntryLabel, GTK_ALIGN_CENTER);
  gtk_grid_attach(GTK_GRID(newPairGrid), newEntryLabel, 0, 0, 3, 1);

  nametext = gtk_entry_new();
  gtk_entry_set_placeholder_text(nametext, "Entry name");
  gtk_grid_attach(GTK_GRID(newPairGrid), nametext, 0, 1, 3, 1);

  usertext = gtk_entry_new();
  gtk_entry_set_placeholder_text(usertext, "Username");
  gtk_grid_attach(GTK_GRID(newPairGrid), usertext, 0, 2, 3, 1);

  passtext = gtk_entry_new();
  gtk_entry_set_placeholder_text(passtext, "Password");
  gtk_entry_set_visibility(passtext, false);
  gtk_grid_attach(GTK_GRID(newPairGrid), passtext, 0, 3, 3, 1);

  remarkstext = gtk_entry_new();
  gtk_entry_set_placeholder_text(remarkstext, "Description");
  gtk_grid_attach(GTK_GRID(newPairGrid), remarkstext, 0, 4, 3, 1);

  cancel = gtk_button_new_with_label("Cancel");
  g_signal_connect_swapped(cancel, "clicked", G_CALLBACK(pwmgr_return_main), window);
  gtk_grid_attach(GTK_GRID(newPairGrid), cancel, 0, 5, 1, 1);

  confirmAdd = gtk_button_new_with_label("Confirm");
  g_signal_connect_swapped(confirmAdd, "clicked", G_CALLBACK(pwmgr_handle_new), NULL);
  gtk_grid_attach(GTK_GRID(newPairGrid), confirmAdd, 2, 5, 1, 1);

  gtk_widget_show(window);

} */

#else

// !!TODO WARN!! try to use something different from gets ASAP
#define gets(X) gets((X))
//#define gets(X) gets_s((X), 20)

#define printd //printf

void* key;
uint32_t keylen = 0;

void get_entries() {
  FILE *fkey, *db;
  fkey = fopen("key.bin","rb");
  db = fopen("passdb.bin","rb");
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
  if (db == NULL) {
    printf("WARN: Could not open database file.\n");
  } else {
    fread(&entryCount, sizeof(uint32_t), 1, db);
    int bufl;
    for(int i=1; i<entryCount+1; i++){
      PMEntry *entry = malloc(sizeof(PMEntry));
      int j = 0;
      while(j < 4) {
        fread(&bufl, sizeof(uint32_t), 1, db);
        void *tbuf = malloc(bufl);
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
      printd("DEBUG: attempting to write subentry %d of entry %d\n", j, i);
      fwrite(&bufl, sizeof(uint32_t), 1, db);
      printd("DEBUG: attempting to write subentry %d of entry %d\n", j, i);
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

#endif

int main(int argc, char **argv) {
#ifdef USEGTK
  GtkApplication *app;
  int status;

  app = gtk_application_new("org.eu.baseduser.passwordmanagertest", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect(app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run(G_APPLICATION (app), argc, argv);
  g_object_unref(app);

  return status;
#else
  get_entries();

  sync_entries(); // just in case
#endif
}

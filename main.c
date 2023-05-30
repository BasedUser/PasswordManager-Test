#include "enc.c"
#include <stdio.h>
#include <stdlib.h>

#define false 0
#define true 1

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

}

#endif

void print_help() {
  printf("Commands:\n"
         "h - Show this list of commands.\n"
         "l - List all registered entries. (ID, name, desc)\n"
         "p - Print a database entry completely (name, user, pass, desc).\n"
         "a - Add a user/pass entry to the database.\n"
         "d - Delete a given entry by ID.\n"
         "q - Quit this program.\n");
}

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
  print_help();
  PMEntry *entry;
  while(true) {
    char c = getchar();
    int doQuit = false;
    switch (c) {
      case 'h':
        print_help();
      break;
      case 'q':
        doQuit = true;
      break;
      case 'l':
        printf("List of entries:\n");
        for(int i=0; i < entryCount; i++) {
          printf("#%d: %s | %s\n", i, entries[i+1]->name, entries[i+1]->desc);
        }
      break;
      case 'a':
        entry = malloc(sizeof(PMEntry));
        printf("Adding a new database entry with ID #%d.\n", entryCount);
        char *name = malloc(128);
        char *user = malloc(128);
        char *pass = malloc(128);
        char *desc = malloc(256);
        printf("Entry name: "); scanf("%s", name);
        printf("User name: "); scanf("%s", user);
        printf("Password: "); scanf("%s", pass);
        printf("Description (public): "); scanf("%s", desc);
        entry->name = name;
        entry->user = user;
        entry->pass = pass;
        entry->desc = desc;
        if(add_entry(entry) == 0)
          printf("Successfully added entry #%d with name %s.\n", entryCount-1, name);
      break;
      case 'p':
        printf("ID (-1 to cancel): ");
        int id = -1;
        scanf("%d", &id);
        if(id < 0 || id >= entryCount) {
          printf("Abort.\n");
          break;
        }
        entry = entries[id+1];
        printf("Entry #%d | %s\nDescription: %s\nUsername: %s\nPassword: %s\n", id, entry->name, entry->desc, entry->user, entry->pass);
      break;
      case 'd':
        printf("ID (-1 to cancel): ");
        id = -1;
        scanf("%d", &id);
        if(id < 0 || id >= entryCount) {
          printf("Abort.\n");
          break;
        }
        remove_entry(id);
      break;
    }
    if(doQuit)
      break;
  }
  sync_entries(); // just in case
#endif
}

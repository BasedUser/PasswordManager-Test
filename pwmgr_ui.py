import gi
gi.require_version("Gtk", "4.0")
from gi.repository import Gtk, Gio, Gdk

if __name__ == "__main__":
  print("WARN: This module should not be run directly.")

entryColumns = ["Entry name", "Username", "Password", "Description"]
pwListStore = Gtk.ListStore(str, str, str, str) # NOTE: deprecated in gtk4
# pwListStore.append(["testname","testuser","testpass","testdesc"])

mainGrid = Gtk.Grid()
newEntryGrid = Gtk.Grid()

import pwmgr_db

def quit_g(self, win):
    pwmgr_db.sync_entries(pwListStore)
    win.close()

def rightclick_event(gesture, n, x, y, ctx):
    popup = ctx[0]
    gesture.set_state(Gtk.EventSequenceState.CLAIMED)
    # uncomment code below if you can't seem to rightclick anything in the menu (https://gitlab.gnome.org/GNOME/gtk/-/issues/4563)
    #copy_entry_g(ctx[1])
    # and comment the other code below likewise
    rect = Gdk.Rectangle();
    rect.x = x; rect.y = y; rect.width = 1; rect.height = 1 # Gdk.Rectangle(...) doesn't accept these, so I'm explicitly defining them
    popup.set_pointing_to(rect)
    popup.popup()

def generate_key_g(self, ctx):
    win = ctx[0]
    pwmgr_db.gen_key(int(int(ctx[1].get_string())/8)) # what
    goto_main_g(None, win)

def init_newKey(win):
    newKeyGrid = Gtk.Grid()
    newKeyGrid.set_halign(Gtk.Align.CENTER)
    newKeyGrid.set_valign(Gtk.Align.CENTER)

    infoLabel = Gtk.Label()
    infoLabel.set_text("Generating a new AES key.")
    kSizeLabel = Gtk.Label()
    kSizeLabel.set_text("Key size (bits): ")
    kSizeChoice = Gtk.DropDown.new_from_strings(["128", "192", "256"])
    kSizeChoice.set_selected(2)
    generateBtn = Gtk.Button(label="Generate")
    generateBtn.connect("clicked", generate_key_g, (win, kSizeChoice.get_selected_item()))

    newKeyGrid.attach(infoLabel, 0, 0, 3, 1)
    newKeyGrid.attach(kSizeLabel, 0, 1, 2, 1)
    newKeyGrid.attach(kSizeChoice, 2, 1, 1, 1)
    newKeyGrid.attach(generateBtn, 0, 2, 3, 1)
    return newKeyGrid

def init_mainMenu(win):
    print("Initializing main menu")
    mainGrid.set_hexpand(True)
    mainGrid.set_vexpand(True)
    mainGrid.set_halign(Gtk.Align.FILL)
    mainGrid.set_valign(Gtk.Align.FILL)

    pwTreeView = Gtk.TreeView(model=pwListStore)
    renderer = Gtk.CellRendererText()
    for i in [0, 1, 3]:
        column = Gtk.TreeViewColumn(entryColumns[i], renderer, text=i)
        pwTreeView.append_column(column)

    pwTreeView.set_hexpand(True)
    pwTreeView.set_vexpand(True)
    pwTreeView.set_halign(Gtk.Align.FILL)
    pwTreeView.set_valign(Gtk.Align.FILL)

    gesture = Gtk.GestureClick() # rightclick code, why was GTK done the way it was
    gesture.set_propagation_phase(Gtk.PropagationPhase.TARGET)
    gesture.set_button(3) # rightclick
    m = Gio.Menu() # menu bloat
    m.append("_Copy", "popup.copy")
    m.append("_Delete", "popup.delete")
    popup = Gtk.PopoverMenu()
    popup.set_parent(win)
    popup.set_has_arrow(False)
    popup.set_menu_model(m)
    actionGroup = Gio.SimpleActionGroup()
    actionGroup.add_action_entries([("copy", copy_entry_g), ("delete", delete_entry_g)], 0)
    pwTreeView.insert_action_group("popup",actionGroup)
    gesture.connect("pressed", rightclick_event, (popup, pwTreeView)) # UI
    pwTreeView.add_controller(gesture)
    mainGrid.attach(pwTreeView, 0, 0, 3, 1)

    quitBtn = Gtk.Button(label="Quit")
    quitBtn.connect("clicked", quit_g, win)
    mainGrid.attach(quitBtn, 0, 1, 1, 1)
    addNewBtn = Gtk.Button(label="+")
    addNewBtn.connect("clicked", goto_newEntry_g, win)
    mainGrid.attach(addNewBtn, 2, 1, 1, 1)
    return mainGrid

def goto_main_g(self, win):
    for i in range(1,5):
      newEntryGrid.get_child_at(0, i).set_text("")
    print("Returning to main menu")
    win.set_child(mainGrid)

def goto_newEntry_g(self, win):
    print("Going to new entry window")
    win.set_child(newEntryGrid)

def add_entry_g(self, ctx):
    win = ctx[0]
    #print(f"Entry name: {ctx[1].get_text()}\nUsername: {ctx[2].get_text()}\nPassword: {ctx[3].get_text()}\nDescription: {ctx[4].get_text()}")
    pwListStore.append([ctx[1].get_text(), ctx[2].get_text(), ctx[3].get_text(), ctx[4].get_text()])
    # eid = add_entry(ctx[1].get_text(), ctx[2].get_text(), ctx[3].get_text(), ctx[4].get_text())
    # print(f"Successfully added new entry with ID #{eid} and name {ctx[1].get_text()}.")
    goto_main_g(None, win)

def copy_entry_g(ctx):
    print("copy_entry_g")
    refSelection = ctx.get_selection()
    if refSelection != None:
        store, iter = refSelection.get_selected()
        if iter != None:
            pwd = store.get_value(iter, 2)
            Gdk.Display.get_default().get_clipboard().set(pwd)
            print(f"Copied password of {store.get_value(iter, 0)}")

def delete_entry_g(ctx):
    refSelection = ctx.get_selection()
    if refSelection != None:
        store, iter = refSelection.get_selected()
        if iter != None:
            store.remove(iter)
        else:
            print("WARN: Attempted to remove a NoneType element. What are you trying to remove?")
    print("delete_entry_g")

def init_newEntry(win):
    newEntryGrid.set_halign(Gtk.Align.CENTER)
    newEntryGrid.set_valign(Gtk.Align.CENTER)
    newLab = Gtk.Label()
    newLab.set_text("Enter new user data")
    newEntryGrid.attach(newLab, 0, 0, 3, 1);
    name = Gtk.Entry()
    name.set_placeholder_text(entryColumns[0])
    newEntryGrid.attach(name, 0, 1, 3, 1);
    user = Gtk.Entry()
    user.set_placeholder_text(entryColumns[1])
    newEntryGrid.attach(user, 0, 2, 3, 1);
    pwd = Gtk.Entry()
    pwd.set_placeholder_text(entryColumns[2])
    newEntryGrid.attach(pwd, 0, 3, 3, 1);
    desc = Gtk.Entry()
    desc.set_placeholder_text(entryColumns[3])
    newEntryGrid.attach(desc, 0, 4, 3, 1);

    cancelBtn = Gtk.Button(label="Cancel")
    cancelBtn.connect("clicked", goto_main_g, win)
    newEntryGrid.attach(cancelBtn, 0, 5, 1, 1)

    confirmBtn = Gtk.Button(label="Confirm")
    confirmBtn.connect("clicked", add_entry_g, (win, name, user, pwd, desc))
    newEntryGrid.attach(confirmBtn, 2, 5, 1, 1)
    return newEntryGrid

def on_activate(app):
    win = Gtk.ApplicationWindow(application=app, title="Password Manager")
    win.set_default_size(400,300)
    mainMenu = init_mainMenu(win)
    newEntryGrid = init_newEntry(win)
    if pwmgr_db.get_entries(pwListStore) == None:
        newKeyGrid = init_newKey(win)
        win.set_child(newKeyGrid)
    else:
        win.set_child(mainMenu)
    win.present()

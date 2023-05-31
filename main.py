import gi
gi.require_version("Gtk", "4.0")
from gi.repository import Gtk

import pwmgr_ui

app = Gtk.Application(application_id='org.eu.baseduser.passwordmanagertest')
app.connect('activate', pwmgr_ui.on_activate)
app.run(None)

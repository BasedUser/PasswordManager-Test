# PasswordManager-Test
The unsecure password manager from hell. Roughly a week of work wasted/spent on the GTK frontend, and the text end was written in 4 hours.
Employs a custom binary database.

NOTE: GTK was too confusing to work with, expect absolutely nothing from the GUI frontend. Works by default with CLI.

# Building
Run `./make.sh`. Currently works only under Linux.
For the GTK frontend, define USEGTK.

# Dependencies
This program depends on openssl and gtk4. These can be installed manually or using your package manager of choice.

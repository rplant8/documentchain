
Debian
====================
This directory contains files used to package dmsd/dms-qt
for Debian-based Linux systems. If you compile dmsd/dms-qt yourself, there are some useful files here.

## dms: URI support ##


dms-qt.desktop  (Gnome / Open Desktop)
To install:

	sudo desktop-file-install dms-qt.desktop
	sudo update-desktop-database

If you build yourself, you will either need to modify the paths in
the .desktop file or copy or symlink your dms-qt binary to `/usr/bin`
and the `../../share/pixmaps/dms128.png` to `/usr/share/pixmaps`

dms-qt.protocol (KDE)


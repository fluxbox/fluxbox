$ codeset=ISO-8859-1
$set 1 #BaseDisplay

1 %s:  X Fehler ! : %s(%d) opcodes %d/%d\n  resource 0x%lx\n
2 %s: Signal %d abgefangen\n
3 Herunterfahren...\n
4 Breche ab...core dump\n
5 BaseDisplay::BaseDisplay: Verbindung mit XServer fehlgeschlagen !\n
6 BaseDisplay::BaseDisplay: \n
7 BaseDisplay::eventLoop(): Entferne fehlerhafte Fenster aus der Ereignisschlange\n

$set 2 #Basemenu

1 Blackbox Menue

$set 3 #Configmenu

1 Einstellungen
2 Fokustyp
3 Fensterplatzierung
4 Bild-Dithering
5 Fenster undurchsichtig  bewegen
6 Vollstaendig Maximieren
7 Neue Fenster fokusieren
8 Fenster nach einem Desktopwechsel fokusieren
9 Klicken um zu Fokusieren
10 "Matschiger" Fokus
11 Automatisches Vorheben
12 Intelligente Platzierung (Reihen)
13 Intelligente Platzierung (Spalten)
14 Kaskadieren
15 Links nach Rechts
16 Rechts nach Links
17 Oben nach Unten
18 Unten nach Oben
19 Tabs verwenden
20 Icons verwenden
21 Tab-Platzierung
22 Rotiere Vertikale Tabs
23 Halbmatschiger Fokus
24 Ueber die Slit maximieren
25 Matschige Fenstergruppierung
26 Arbeitsflaechen-Warping
27 Desktop mit Mausrad wechseln

$set 4 #Icon

1 Icons

$set 5 #Image

1 BImage::render_solid: error creating pixmap\n
2 BImage::renderXImage: error creating XImage\n
3 BImage::renderXImage: unsupported visual\n
4 BImage::renderPixmap: error creating pixmap\n
5 BImageControl::BImageControl: invalid colormap size %d (%d/%d/%d) - reducing\n
6 BImageControl::BImageControl: error allocating colormap\n
7 BImageControl::BImageControl: failed to alloc color %d/%d/%d\n
8 BImageControl::~BImageControl: pixmap cache - releasing %d pixmaps\n
9 BImageControl::renderImage: cache is large, forcing cleanout\n
10 BImageControl::getColor: color parse error: '%s'\n
11 BImageControl::getColor: color alloc error: '%s'\n

$set 6 #Screen

1 BScreen::BScreen : Fehler : Ein anderer Windowmanager laeuft bereits \n
2 BScreen::BScreen: managing screen %d using visual 0x%lx, depth %d\n
3 BScreen::LoadStyle():Konnte Schriftart '%s' nicht laden !\n
4 BScreen::LoadStyle(): konnte Standardfont nicht laden !\n
5 %s: leere Menuedatei\n
6 xterm
7 Neustarten
8 Beenden
9 BScreen::parseMenuFile: [exec] error, no menu label and/or command defined\n
10 BScreen::parseMenuFile: [exit] error, no menu label defined\n
11 BScreen::parseMenuFile: [style] error, no menu label and/or filename \
defined\n
12 BScreen::parseMenuFile: [config] error, no menu label defined\n
13 BScreen::parseMenuFile: [include] error, no filename defined\n
14 BScreen::parseMenuFile: [include] error, '%s' is not a regular file\n
15 BScreen::parseMenuFile: [submenu] error, no menu label defined\n
16 BScreen::parseMenuFile: [restart] error, no menu label defined\n
17 BScreen::parseMenuFile: [reconfig] error, no menu label defined\n
18 BScreen::parseMenuFile: [stylesdir/stylesmenu] error, no directory defined\n
19 BScreen::parseMenuFile: [stylesdir/stylesmenu] error, '%s' is not a \
directory\n
20 BScreen::parseMenuFile: [stylesdir/stylesmenu] error, '%s' does not exist\n
21 BScreen::parseMenuFile: [workspaces] error, no menu label defined\n
22 0: 0000 x 0: 0000
23 X: %4d x Y: %4d
24 W: %4d x H: %4d


$set 7 #Slit

1 Slit
2 Slit-Ausrichtung
3 Slit-Platzierung

$set 8 #Toolbar

1 00:00000
2 %02d/%02d/%02d
3 %02d.%02d.%02d
4  %02d:%02d 
5 %02d:%02d %sm
6 p
7 a
8 Werkzeugleiste
9 Momentanen Arbeitsflaechennamen bearbeiten
10 Werkzeugleistenplatziung

$set 9 #Window


1 BlackboxWindow::BlackboxWindow: creating 0x%lx\n
2 BlackboxWindow::BlackboxWindow: XGetWindowAttributres failed\n
3 BlackboxWindow::BlackboxWindow: cannot find screen for root window 0x%lx\n
4 Unbenannt
5 BlackboxWindow::mapRequestEvent() for 0x%lx\n
6 BlackboxWindow::unmapNotifyEvent() for 0x%lx\n
7 BlackboxWindow::unmapnotifyEvent: reparent 0x%lx to root\n

$set 10 #Windowmenu

1 Senden an ...
2 Gruppe senden an ...
3 Aufrollen
4 Minimieren
5 Maximieren
6 Nach Vorne
7 Nach Hinten
8 Klebrig
9 Gewaltsam beenden
10 Schliessen
11 Tab

$set 11 #Workspace

1 Workspace %d

$set 12 #Workspacemenu

1 Workspaces
2 New Workspace
3 Remove Last

$set 13 #blackbox

1 Blackbox::Blackbox: no managable screens found, aborting\n
2 Blackbox::process_event: MapRequest for 0x%lx\n

$set 14 #Common

1 Ja
2 Nein

3 Ausrichtung
4 Horizontal
5 Vertikal

6 Immer oben

7 Platzierung
8 Oben links
9 Mitte links
10 Unten links
11 Oben mitte
12 Unten mitte
13 Oben rechts
14 Mitte rechts
15 Unten rechts
16 Links oben
17 Links mitte
18 Links unten
19 Rechts oben
20 Rechts mitte
21 Rechts unten
22 Oben-relative Groesse
23 Unten-relative Groesse
24 Links-relative Groesse
25 Rechts-relative Groesse
26 Automatisch Verstecken

$set 15 #main

1 error: '-rc' requires an argument\n
2 error: '-display' requires an argument\n
3 warning: could not set environment variable 'DISPLAY'\n
4 Fluxbox %s: (c) %s Henrik Kinnunen\n\n\
  -display <string>\t\tuse display connection.\n\
  -rc <string>\t\t\tuse alternate resource file.\n\
  -version\t\t\tdisplay version and exit.\n\
  -info\t\t\t\tdisplay some useful information.\n\
  -log <filename>\t\t\tlog output to file.\n\
  -help\t\t\t\tdisplay this help text and exit.\n\n
5 Compile time options:\n\
  Debugging\t\t\t%s\n\
  Interlacing:\t\t\t%s\n\
  Shape:\t\t\t%s\n\
  Slit:\t\t\t\t%s\n\
  8bpp Ordered Dithering:\t%s\n\n

$set 16 #bsetroot

1 %s: error: must specify one of: -solid, -mod, -gradient\n
2 %s 2.0: (c) 1997-2000 Brad Hughes\n\n\
  -display <string>        display connection\n\
  -mod <x> <y>             modula pattern\n\
  -foreground, -fg <color> modula foreground color\n\
  -background, -bg <color> modula background color\n\n\
  -gradient <texture>      gradient texture\n\
  -from <color>            gradient start color\n\
  -to <color>              gradient end color\n\n\
  -solid <color>           solid color\n\n\
  -help                    print this help text and exit\n


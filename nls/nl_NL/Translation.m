$ codeset=ISO-8859-1
$set 1 #BaseDisplay

1 %s:  X error: %s(%d) opcodes %d/%d\n  resource 0x%lx\n
2 %s: signal %d caught\n
3 shutting down\n
4 aborting... dumping core\n
5 BaseDisplay::BaseDisplay: connection to X server failed.\n
6 BaseDisplay::BaseDisplay: couldn't mark display connection as close-on-exec\n
7 BaseDisplay::eventLoop(): removing bad window from event queue\n

$set 2 #Basemenu

1 Fluxbox Menu

$set 3 #Configmenu

1 Instellingen
2 Focussoort
3 Vensterplaatsing
4 Plaatjes-afvlakking
5 Venster ondoorzichtig verplaatsen
6 Volledig maximaliseren
7 Nieuwe vensters focussen
8 Vensters na Werkveldwissel focussen
9 Klik om Focus te krijgen
10 Muis focus
11 Automatisch verhogen
12 Slim plaatsen (rij)
13 Slim plaatsen (kolom)
14 Stapelen
15 Links naar rechts
16 Rechts naar links
17 Boven naar Onder
18 Onder naar boven
19 Tabs gebruiken
20 Iconen gebruiken
21 Tabs Plaatsing
22 Vertikaal Tabs draaien
23 Bijna altijd muis focus
24 Over de slit maximalizeren
25 Willekeurig vensters groeperen
26 Werkveld springen
27 Werkveld met muiswiel wisselen

$set 4 #Icon

1 Iconen

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

1 BScreen::BScreen: an error occured while querying the X server.\n  \
another window manager is already running on display %s.\n
2 BScreen::BScreen: managing screen %d using visual 0x%lx, depth %d\n
3 BScreen::LoadStyle(): couldn't load font '%s'\n
4 BScreen::LoadStyle(): couldn't load default font.\n
5 %s: empty menu file\n
6 xterm
7 Restart
8 Exit
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
2 Slit-Richting
3 Slit-Plaatsing

$set 8 #Toolbar

1 00:00000
2 %02d/%02d/%02d
3 %02d.%02d.%02d
4  %02d:%02d
5 %02d:%02d %sm
6 p
7 a
8 Werkbalklijst
9 Wekveldnaam veranderen
10 Werkbalkplaatsing

$set 9 #Window


1 BlackboxWindow::BlackboxWindow: creating 0x%lx\n
2 BlackboxWindow::BlackboxWindow: XGetWindowAttributres failed\n
3 BlackboxWindow::BlackboxWindow: cannot find screen for root window 0x%lx\n
4 Unbenannt
5 BlackboxWindow::mapRequestEvent() for 0x%lx\n
6 BlackboxWindow::unmapNotifyEvent() for 0x%lx\n
7 BlackboxWindow::unmapnotifyEvent: reparent 0x%lx to root\n

$set 10 #Windowmenu

1 Stuur naar
2 Groep sturen naar
3 Oprollen
4 Minimalizeren
5 Maximalizeren
6 Bovenop leggen
7 Onderop leggen
8 Plakkerig
9 Afbreken
10 Sluiten
11 Tab

$set 11 #Workspace

1 Werkveld %d

$set 12 #Workspacemenu

1 Werkvelden
2 Nieuw werkveld
3 Verwijder laatste

$set 13 #blackbox

1 Fluxbox::Fluxbox: geen bruikbaar scherm te vinden, afbreken\n
2 Fluxbox::process_event: MapRequest for 0x%lx\n

$set 14 #Common

1 Ja
2 Nee

3 Richting
4 Horizontaal
5 Vertikaal

6 Altijd bovenop

7 Plaats
8 Boven links
9 Midden links
10 Onder links
11 Boven midden
12 Onder midden
13 Boven rechts
14 Midden rechts
15 Onder rechts
16 Links boven
17 Links midden
18 Links onder
19 Rechts boven
20 Rechts midden
21 Rechts onder
22 Boven-relatieve plaatsing
23 Onder-relatieve plaatsing
24 Links-relatieve plaatsing
25 Rechts-relatieve plaatsing
26 Automatisch verbergen

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


$ codeset=ISO-8859-1

$set 0x1 #Align

0x1 In basso al centro
0x2 In basso a sinistra
0x3 In basso a destra
0x4 Orizzontale
0x6 A sinistra in basso
0x7 A sinistra al centro
0x8 A sinistra in alto
0xb A destra in basso
0xc A destra al centro
0xd A destra in alto
0xe In alto al centro
0xf In alto a sinistra
0x10 In alto a destra
0x11 Verticale

$set 0x2 #BaseDisplay


$set 0x3 #Common


$set 0x4 #Configmenu

0x2 In primo piano automaticamente
0x4 Clicca per focalizzare
0x7 Focalizza l'ultima finestra nell'area di lavoro
0x8 Tipo di focalizzazione
0x9 Focalizza le nuove finestre
0xa Massimo ingrandimento
0xb Simulazione di colore nelle immagini
0xc Mostra il contenuto spostando le finestre
0xd Focalizzazione semiautomatica
0xe Focalizzazione automatica
0xf Cambia area di lavoro trascinando le finestre

$set 0x5 #Ewmh


$set 0x6 #FbTkError


$set 0x7 #Fluxbox


$set 0x8 #Gnome


$set 0x9 #Keys


$set 0xa #Menu

0x3 Uscita
0x4 Icone
0x7 Posizionamento
0x9 Riavvio
0xa xterm

$set 0xb #Remember


$set 0xc #Screen

0x2 W: %4d x H: %4d
0x5 X: %4d x Y: %4d
0x6 0: 0000 x 0: 0000

$set 0xd #Slit

0x4 Direzione dello slit
0x7 Posizionamento dello slit
0x8 Slit

$set 0xe #Toolbar

0x1 Cambia nome a questa area di lavoro
0xa Posizionamento della Toolbar
0xb Toolbar

$set 0xf #Window

0x1 Senza nome

$set 0x10 #Windowmenu

0x1 Chiudi
0x2 Riduci a icona
0x4 Sullo sfondo
0x5 Ingrandisci
0x6 In primo piano
0x7 Manda a ...
0x8 Riduci a barra
0x9 Su tutte le aree di lavoro

$set 0x11 #Workspace

0x1 Area di lavoro %d
0x2 Aree di lavoro
0x3 Crea una nuova
0x4 Elimina l'ultima

$set 0x12 #bsetroot

0x1 %s: errore: specicare una delle opzioni : -solid, -mod, -gradient\n
0x3 %s 2.0: (c) 1997-2000 Brad Hughes\n\n\
-display <stringa>        connessione al display\n\
-mod <x> <y>              schema\n\
-foreground, -fg <colore> colore di primo piano nello schema\n\
-background, -bg <colore> colore di sfondo nello schema\n\n\
-gradient <texture>       tipo di gradazione\n\
-from <colore>            colore di partenza dela gradazione\n\
-to <colore>              colore finale dela gradazione\n\n\
-solid <colore>           colore semplice\n\n\
-help                     mostra questo messaggio ed esci\n

$set 0x13 #main

0x1 errore: '-display' richiede un argomento\n
0xb errore: '-rc' richiede un argomento\n
0xc Fluxbox %s: (c) %s Henrik Kinnunen\n\n\
-display <stringa>\t\tusa la connessione al display.\n\
-rc <stringa>\t\t\tusa un file di configurazione alternativo.\n\
-version\t\t\tmostra la versione ed esci.\n\
-info\t\t\t\tdisplay some useful information.\n\
-log <filename>\t\t\tlog output to file.\n\
-help\t\t\t\tmostra questo messaggio di aiuto ed esci.\n\n

$set 0xd #mainWarnDisplay


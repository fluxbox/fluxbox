$set 14 #main

$ #RCRequiresArg
# error: '-rc' heeft een argument nodig\n
$ #DISPLAYRequiresArg
# error: '-display' heeft een argument nodig\n
$ #WarnDisplaySet
# warning: kan omgevingsvariabele 'DISPLAY' niet instellen.\n
$ #Usage
# Fluxbox %s: (c) %s Henrik Kinnunen\n\n\
  -display <string>\t\tkies ander scherm.\n\
  -rc <string>\t\t\tgebruik alternatieve configuratie.\n\
  -version\t\t\ttoon versienummer.\n\
  -info\t\t\t\ttoon bruikbare informatie.\n\
  -log <filename>\t\t\tlog uitvoer naar bestand.\n\
  -help\t\t\t\ttoon alleen deze helptext.\n\n
$ #CompileOptions
# Compileer opties:\n\
  Debugging\t\t\t%s\n\
  Interlacing:\t\t\t%s\n\
  Shape:\t\t\t%s\n\
  Slit:\t\t\t\t%s\n\
  8bpp Ordered Dithering:\t%s\n\n

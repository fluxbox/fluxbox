$set 14 #main

$ #RCRequiresArg
# error: '-rc' requires an argument\n
$ #DISPLAYRequiresArg
# error: '-display' requires an argument\n
$ #WarnDisplaySet
# warning: could not set environment variable 'DISPLAY'\n
$ #Usage
# Fluxbox %s: (c) %s Henrik Kinnunen\n\n\
  -display <string>\t\tuse display connection.\n\
  -rc <string>\t\t\tuse alternate resource file.\n\
  -version\t\t\tdisplay version and exit.\n\
  -info\t\t\t\tdisplay some useful information.\n\
  -log <filename>\t\t\tlog output to file.\n\
  -help\t\t\t\tdisplay this help text and exit.\n\n
$ #CompileOptions
# Compile time options:\n\
  Debugging\t\t\t%s\n\
  Interlacing:\t\t\t%s\n\
  Shape:\t\t\t%s\n\
  Slit:\t\t\t\t%s\n\
  8bpp Ordered Dithering:\t%s\n\n

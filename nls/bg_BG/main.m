$set 14 #main

$ #RCRequiresArg
# грешка: '-rc' изисква аргумент\n
$ #DISPLAYRequiresArg
# грешка: '-display' изисква аргумент\n
$ #WarnDisplaySet
# внимание: не мога да наглася променливата на средата 'DISPLAY'\n
$ #Usage
# Fluxkbox %s: (c) %s Henrik Kinnunen\n\n\
  -display <string>\t\tuse display connection.\n\
  -rc <string>\t\t\tползвай заместващ ресурсен файл.\n\
  -version\t\t\tпокажи версията и излез.\n\
  -info\t\t\t\tdisplay some useful information.\n\
  -log <filename>\t\t\tlog output to file.\n\
  -help\t\t\t\tпокажи този помощен текст и излез.\n\n
$ #CompileOptions
# Избрани възможности по време на компилацията:\n\
  Debugging\t\t\t%s\n\
  Interlacing:\t\t\t%s\n\
  Shape:\t\t\t%s\n\
  Slit:\t\t\t\t%s\n\
  8bpp Ordered Dithering:\t%s\n\n

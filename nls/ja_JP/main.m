$set 14 #main

$ #RCRequiresArg
# error: '-rc' オプションは引数を必要とします\n
$ #DISPLAYRequiresArg
# error: '-display' オプションは引数を必要とします\n
$ #WarnDisplaySet
# warning: 環境変数 'DISPLAY' を設定できませんでした\n
$ #Usage
# Fluxbox %s: (c) %s Henrik Kinnunen\n\n\
  -display <string>\t\t 指定ディスプレイに接続.\n\
  -rc <string>\t\t\t 代わりのリソースファイルを使用.\n\
  -version\t\t\t バージョン情報を表示して終了.\n\
  -info\t\t\t\tdisplay some useful information.\n\
  -log <filename>\t\t\tlog output to file.\n\
  -help\t\t\t\t このヘルプを表示して終了.\n\n
$ #CompileOptions
# コンパイル時のオプション:\n\
  Debugging\t\t\t%s\n\
  Interlacing:\t\t\t%s\n\
  Shape:\t\t\t%s\n\
  Slit:\t\t\t\t%s\n\
  8bpp Ordered Dithering:\t%s\n\n

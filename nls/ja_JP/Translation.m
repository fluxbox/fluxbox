
$set 0x1 #Align

0x1 下側-中央
0x2 下側-左
0x3 下側-右
0x4 水平(H)
0x6 左側-下
0x7 左側-中央
0x8 左側-上
0xb 右側-下
0xc 右側-中央
0xd 右側-上
0xe 上側-中央
0xf 上側-左
0x10 上側-右
0x11 垂直(V)

$set 0x2 #BaseDisplay


$set 0x3 #Common

0x2 自動的に隠す

$set 0x4 #Configmenu

0x1 アンチエイリアス
0x2 フォーカスを当てたときに最前面に移動
0x4 クリックでフォーカス
0x7 ワークスペース移動時に最後のウィンドウにフォーカス
0x8 フォーカスモデル
0x9 新規ウィンドウにフォーカス
0xa 完全最大化(タスクバー無視)
0xb 画像ディザ
0xc ウィンドウの内容を表示したまま移動
0xd カーソルオーバーでフォーカス(SemiSloppy)
0xe カーソルオーバーでフォーカス
0xf ウィンドウドラッグでワークスペースを移動

$set 0x5 #Ewmh


$set 0x6 #FbTkError


$set 0x7 #Fluxbox


$set 0x8 #Gnome


$set 0x9 #Keys


$set 0xa #Menu

0x3 終了
0x4 アイコン
0x7 配置
0x9 再起動
0xa Ｘたーむ

$set 0xb #Remember


$set 0xc #Screen

0x2 幅: %4d x 高: %4d
0x3 幅: 0000 x 高: 0000
0x5 X: %4d x Y: %4d
0x6 0: 0000 x 0: 0000

$set 0xd #Slit

0x4 Slit の方向
0x7 Slit の配置
0x8 Slit

$set 0xe #Toolbar

0x1 現在のワークスペース名を編集
0xa ツールバーの配置
0xb ツールバー

$set 0xf #Window

0x1 名前なし

$set 0x10 #Windowmenu

0x1 閉じる
0x2 アイコン化（タイトルバーに格納）
0x4 最背面に持っていく
0x5 最大化
0x6 最前面に持ってくる
0x7 送る ...
0x8 バーに格納
0x9 すべてのワークスペースで表示

$set 0x11 #Workspace

0x1 ワークスペース %d
0x2 ワークスペース
0x3 新規ワークスペース作成
0x4 末尾のワークスペースを削除

$set 0x12 #bsetroot

0x1 %s: error: 次の中から一つを選択しなければなりません: -solid, -mod, -gradient\n
0x3 %s 2.0: (c) 1997-2000 Brad Hughes\n\n\
-display <string>        指定ディスプレイに接続\n\
-mod <x> <y>             格子のパターン間隔\n\
-foreground, -fg <color> 格子模様の前景色\n\
-background, -bg <color> 格子模様の背景色\n\n\
-gradient <texture>      グラデーション（テクスチャ）\n\
-from <color>            グラデーションの開始色\n\
-to <color>              グラデーションの終了色\n\n\
-solid <color>           単色\n\n\
-help                    このヘルプを表示して終了\n

$set 0x13 #main

0x1 error: '-display' オプションは引数を必要とします\n
0xb error: '-rc' オプションは引数を必要とします\n
0xc Fluxbox %s: (c) %s Henrik Kinnunen\n\n\
-display <string>\t\t 指定ディスプレイに接続.\n\
-rc <string>\t\t\t 代わりのリソースファイルを使用.\n\
-version\t\t\t バージョン情報を表示して終了.\n\
-info\t\t\t\tdisplay some useful information.\n\
-log <filename>\t\t\tlog output to file.\n\
-help\t\t\t\t このヘルプを表示して終了.\n\n

$set 0xd #mainWarnDisplay


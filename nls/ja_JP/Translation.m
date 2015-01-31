$ codeset=eucJP

$set 1 #Align

1 下側-中央
2 下側-左
3 下側-右
4 水平(H)
6 左側-下
7 左側-中央
8 左側-上
11 右側-下
12 右側-中央
13 右側-上
14 上側-中央
15 上側-左
16 上側-右
17 垂直(V)

$set 2 #BaseDisplay

1 強制終了... コアをダンプしています\n
2 終了中\n
3 %s:      シグナル %d を受信しました\n

$set 3 #Common

1 アルファ値
2 自動的に隠す
4 コンパイル時のオプション
5 コンパイラ
6 コンパイラのバージョン
12 デフォルト
13 無効
14 エラー
15 Fluxbox のバージョン
16 ウィンドウ最大化でツールバー領域も使用
17 GIT上 のリビジョン
18 表示する
19 このスタイルでは背景オプションは指定されていません。\nマニュアルとFAQを参照してください。

$set 4 #Configmenu

1 アンチエイリアス
2 フォーカスを当てたときに最前面に移動
3 クリックで最前面に移動
4 クリックでフォーカス
7 ワークスペース移動時に最後のウィンドウにフォーカス
8 フォーカスモデル
9 新規ウィンドウにフォーカス
10 完全最大化(タスクバー無視)
11 画像ディザ
12 ウィンドウの内容を表示したまま移動
13 カーソルオーバーでフォーカス(Strict)
14 カーソルオーバーでフォーカス
15 ウィンドウドラッグでワークスペースを移動
16 強制的に疑似透過
17 メニューのアルファ値
18 透明度
19 フォーカスウィンドウのアルファ値
20 フォーカスのないウィンドウのアルファ値
21 タブのオプション
22 タブをタイトルバーに
23 外部タブの幅
24 クリックでタブにフォーカス
25 カーソルオーバーでタブにフォーカス
26 最大化オプション
27 規定のサイズ変更増分を無視
28 最大化ウィンドウの移動を禁止
29 最大化ウィンドウのサイズ変更を禁止

$set 5 #Ewmh


$set 6 #FbTkError


$set 7 #Fluxbox


$set 8 #Gnome


$set 9 #Keys


$set 10 #Menu

1 設定
2 Fluxbox デフォルトメニュー
3 終了
4 アイコン
5 レイヤー...
6 表示先のスクリーン...
7 配置
8 設定を再読み込み
9 再起動
10 警告: アンバランスな[エンコーディング]タグ

$set 11 #Remember

1 装飾
2 寸法
3 ワークスペースへジャンプ
4 レイヤー
5 記憶...
6 位置
7 閉じる時に保存
8 バーに格納
9 すべてのワークスペースで表示
11 ワークスペース
12 スクリーン
13 透明度
14 最小化
15 最大化
16 前画面表示

$set 12 #Screen

2 幅: %4d x 高: %4d
4 幅: %04d x 高: %04d

$set 13 #Slit

1 クライアント
4 スリット の方向
5 スリット のレイヤー
6 スリット を表示するスクリーン
7 スリット の配置
8 スリット
9 スリットのリストを保存

$set 14 #Toolbar

1 現在のワークスペース名を編集
2 アイコンバーの設定
3 すべてのウィンドウ
4 アイコン表示
5 なし
6 ワークスペースのウィンドウ
7 ワークスペースのウィンドウをアイコン表示
8 ツールバーのレイヤー
10 ツールバーの配置
11 ツールバー
12 ツールバーの幅(パーセント)
13 時計: 24時間表示
14 時計: 12時間表示
15 時刻表示の形式を編集
16 画像を表示
17 アイコンなし
18 ワークスペースのウィンドウをアイコンなし表示

$set 15 #Window

1 名前なし

$set 16 #Windowmenu

1 閉じる
2 アイコン化（タイトルバーに格納）
3 レイヤー
4 最背面に持っていく
5 最大化
6 最前面に持ってくる
7 送る ...
8 バーに格納
9 すべてのワークスペースで表示
12 ウィンドウのタイトルを設定

$set 17 #Workspace

1 ワークスペース %d
2 ワークスペース
3 新規ワークスペース作成
4 末尾のワークスペースを削除

$set 18 #fbsetroot

1 Error: 次の中から一つを選択しなければなりません: -solid, -mod, -gradient\n
3 -display <string>        指定ディスプレイに接続\n\
-mod <x> <y>             格子のパターン間隔\n\
-foreground, -fg <color> 格子模様の前景色\n\
-background, -bg <color> 格子模様の背景色\n\n\
-gradient <texture>      グラデーション（テクスチャ）\n\
-from <color>            グラデーションの開始色\n\
-to <color>              グラデーションの終了色\n\n\
-solid <color>           単色\n\n\
-help                    このヘルプを表示して終了\n

$set 19 #main

1 error: '-display' オプションは引数を必要とします
11 error: '-rc' オプションは引数を必要とします
13 Fluxbox %s: (c) %s Fluxbox Team\n\n\
-display <string>\t\t 指定ディスプレイに接続.\n\
-screen <all|int,int,int>\trun on specified screens only.\n\
-no-slit\t\t\tdo not provide a slit.\n\
-no-toolbar\t\t\tdo not provide a toolbar.\n\
-rc <string>\t\t\t 代わりのリソースファイルを使用.\n\
-version\t\t\t バージョン情報を表示して終了.\n\
-info\t\t\t\tdisplay some useful information.\n\
-list-commands\t\t\tlist all valid key commands.\n\
-sync\t\t\t\tsynchronize with X server for debugging.\n\
-log <filename>\t\t\tlog output to file.\n\
-help\t\t\t\t このヘルプを表示して終了.\n\n
14 warning: couldn't set environment variable 'DISPLAY'

$set 20 #layers

1 ドックの上
2 一番下のウィンドウ
3 デスクトップ
4 ドック
5 通常
6 一番上のウィンドウ

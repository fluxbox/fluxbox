$set 1 #BaseDisplay

1 %s:  X エラー: %s(%d) opcodes %d/%d\n  resource 0x%lx\n
2 %s: シグナル %d 発生\n
3 終了\n
4 中止 ... コアダンプしますe\n
5 BaseDisplay::BaseDisplay: X サーバへの接続に失敗.\n
6 BaseDisplay::BaseDisplay: couldn't mark display connection as close-on-exec\n
7 BaseDisplay::eventLoop(): イベントキューから不正なウィンドウを消します\n

$set 2 #Basemenu

1 FluxBox メニュー

$set 3 #Configmenu

1 設定オプション
2 フォーカスモデル
3 ウィンドウ配置
4 画像ディザ
5 ウィンドウの内容を表示したまま移動
6 完全最大化(タスクバー無視)
7 新規ウィンドウにフォーカス
8 ワークスペース移動時に最後のウィンドウにフォーカス
9 クリックでフォーカス
10 カーソルオーバーでフォーカス
11 フォーカスを当てたときに最前面に移動
12 賢く配置 (行:ROW)
13 賢く配置 (列:COL)
14 カスケード配置
15 左から右へ
16 右から左へ
17 上から下へ
18 下から上へ
19 タブを表示
20 アイコンの表示
21 タブの表示位置
22 タブの自動回転
23 カーソルオーバーでフォーカス(SemiSloppy)
24 Slitを越えて最大化
25 Window内にタブドロップでもタブグループに
26 Workspace Warping
27 Desktop Wheeling

$set 4 #Icon

1 アイコン

$set 5 #Image

1 BImage::render_solid: pixmap 生成エラー\n
2 BImage::renderXImage: XImage 生成エラー\n
3 BImage::renderXImage: 未サポートの視覚効果\n
4 BImage::renderPixmap: pixmap 生成エラー\n
5 BImageControl::BImageControl: 不正なカラーマップサイズ %d (%d/%d/%d) - 減色中\n
6 BImageControl::BImageControl: \n
7 BImageControl::BImageControl: カラー %d/%d/%d の確保に失敗\n
8 BImageControl::~BImageControl: pixmap キャッシュ - %d pixmap を解放\n
9 BImageControl::renderImage: キャッシュが大きすぎるので強制的に削除\n
10 BImageControl::getColor: カラー解析エラー: '%s'\n
11 BImageControl::getColor: カラー確保エラー: '%s'\n

$set 6 #Screen

1 BScreen::BScreen: X サーバ問合せ中にエラー発生.\n  \
他のウィンドゥマネージャが既に起動しています %s.\n
2 BScreen::BScreen: スクリーン %d を視覚効果 0x%lx, 色深度 %d で制御\n
3 BScreen::LoadStyle(): フォント '%s' を読み込めませんでした\n
4 BScreen::LoadStyle(): デフォルトフォントを読み込めませんでした.\n
5 %s: メニューファイルが空でした\n
6 Ｘたーむ
7 再起動
8 終了
9 BScreen::parseMenuFile: [exec] エラー, メニューラベルかコマンドが未定義です\n
10 BScreen::parseMenuFile: [exit] エラー, メニューラベル未定義です\n
11 BScreen::parseMenuFile: [style] エラー, メニューラベルかファイル名が未定義です\n
12 BScreen::parseMenuFile: [config] エラー, メニューラベルが未定義です\n
13 BScreen::parseMenuFile: [include] エラー, ファイル名が未定義です\n
14 BScreen::parseMenuFile: [include] エラー, '%s' は通常ファイルではないようです\n
15 BScreen::parseMenuFile: [submenu] エラー, メニューラベルが未定義\n
16 BScreen::parseMenuFile: [restart] エラー, メニューラベルが未定義\n
17 BScreen::parseMenuFile: [reconfig] エラー, メニューラベルが未定義\n
18 BScreen::parseMenuFile: [stylesdir/stylesmenu] エラー, ディレクトリ名が未定義\n
19 BScreen::parseMenuFile: [stylesdir/stylesmenu] エラー, '%s' はディレクトリではありません\n
20 BScreen::parseMenuFile: [stylesdir/stylesmenu] エラー, '%s' は存在しません\n
21 BScreen::parseMenuFile: [workspaces] エラー, メニューラベルが未定義\n
22 0: 0000 x 0: 0000
23 X: %4d x Y: %4d
24 幅: %4d x 高: %4d


$set 7 #Slit

1 Slit
2 Slit の方向
3 Slit の配置

$set 8 #Toolbar

1 00:00000
2 %02d/%02d/%02d
3 %02d.%02d.%02d
4  %02d:%02d 
5 %02d:%02d %sm
6 p
7 a
8 ツールバー
9 現在のワークスペース名を編集
10 ツールバーの配置

$set 9 #Window


1 BlackboxWindow::BlackboxWindow: 0x%lx を生成中\n
2 BlackboxWindow::BlackboxWindow: XGetWindowAttributres failed\n
3 BlackboxWindow::BlackboxWindow: root window 0x%lx に対するスクリーンが見つかりません\n
4 名前なし
5 BlackboxWindow::mapRequestEvent() for 0x%lx\n
6 BlackboxWindow::unmapNotifyEvent() for 0x%lx\n
7 BlackboxWindow::unmapnotifyEvent: reparent 0x%lx to root\n

$set 10 #Windowmenu

1 移動先ワークスペース ...
2 移動先グループ ...
3 バーに格納
4 アイコン化（タイトルバーに格納）
5 最大化
6 最前面に持ってくる
7 最背面に持っていく
8 すべてのワークスペースで表示
9 強制終了
10 閉じる

$set 11 #Workspace

1 ワークスペース %d

$set 12 #Workspacemenu

1 ワークスペース
2 新規ワークスペース作成
3 末尾のワークスペースを削除

$set 13 #blackbox

1 Blackbox::Blackbox: 操作可能なスクリーンがありません、中止します\n
2 Blackbox::process_event: MapRequest for 0x%lx\n

$set 14 #Common

1 はい
2 いいえ

3 方向
4 水平(H)
5 垂直(V)

6 常に最前面に

7 配置
8 上側-左
9 中央-左
10 下側-左
11 上側-中央
12 下側-中央
13 上側-右
14 中央-右
15 下側-右
16 左側-上
17 左側-中央
18 左側-下
19 右側-上
20 右側-中央
21 右側-下
22 上からの相対
23 下からの相対
24 左からの相対
25 右からの相対
26 自動的に隠す

$set 15 #main

1 error: '-rc' オプションは引数を必要とします\n
2 error: '-display' オプションは引数を必要とします\n
3 warning: 環境変数 'DISPLAY' を設定できませんでした\n
4 Fluxbox %s: (c) 2001 Henrik Kinnunen\n\n\
  -display <string>\t\t 指定ディスプレイに接続.\n\
  -rc <string>\t\t\t 代わりのリソースファイルを使用.\n\
  -version\t\t\t バージョン情報を表示して終了.\n\
  -help\t\t\t\t このヘルプを表示して終了.\n\n
5 コンパイル時のオプション:\n\
  Debugging\t\t\t%s\n\
  Interlacing:\t\t\t%s\n\
  Shape:\t\t\t%s\n\
  Slit:\t\t\t\t%s\n\
  8bpp Ordered Dithering:\t%s\n\n

$set 16 #bsetroot

1 %s: error: 次の中から一つを選択しなければなりません: -solid, -mod, -gradient\n
2 %s 2.0: (c) 1997-2000 Brad Hughes\n\n\
  -display <string>        指定ディスプレイに接続\n\
  -mod <x> <y>             格子のパターン間隔\n\
  -foreground, -fg <color> 格子模様の前景色\n\
  -background, -bg <color> 格子模様の背景色\n\n\
  -gradient <texture>      グラデーション（テクスチャ）\n\
  -from <color>            グラデーションの開始色\n\
  -to <color>              グラデーションの終了色\n\n\
  -solid <color>           単色\n\n\
  -help                    このヘルプを表示して終了\n


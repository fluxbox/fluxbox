$set 1 #BaseDisplay

1 %s:  X грешка: %s(%d) opcodes %d/%d\n  resource 0x%lx\n
2 %s: сигнал %d улових\n
3 прекратявам работата\n
4 спирам... dumping core\n
5 BaseDisplay::BaseDisplay: връзката със X сървъра е несполучлива.\n
6 BaseDisplay::BaseDisplay: не мога да маркирам връзката с екрана като close-on-exec\n
7 BaseDisplay::eventLoop(): махам лошия прозорец от опашката на събитията\n

$set 2 #Basemenu

1 Blackbox Меню

$set 3 #Configmenu

1 Конфигурация
2 Тип фокус
3 Разположение на Прозорците
4 Image Dithering
5 Непрозрачно местене на прозорците
6 Пълно Увеличаване
7 Фокусирай Новите Прозорци
8 Фокусирай Прозорец при смяна на Работните Места
9 Кликни за Фокус
10 Sloppy Фокус
11 Автоматично Повдигане
12 Интелигентно Разположение (Редици)
13 Интелигентно Разположение (Колони)
14 Каскадно Разположение
15 От Ляво на Дясно
16 От Дясно на Ляво
17 От Горе на Долу
18 От Долу на Горе
19 Ползвай Етикети
20 Ползвай Икони
21 Разположение на Етикетите
22 Завърти вертикалните Етикети
23 Полу Sloppy Фокус
24 Увеличи върху Slit -а
25 Sloppy Групиране на прозорци
26 Workspace Warping
27 Desktop Wheeling

$set 4 #Icon

1 Икони

$set 5 #Image

1 BImage::render_solid: грешка при създаването на pixmap\n
2 BImage::renderXImage: грешка при създаването на XImage\n
3 BImage::renderXImage: не подържани средства\n
4 BImage::renderPixmap: грешка при създаването на pixmap\n
5 BImageControl::BImageControl: невалиден colormap размер %d (%d/%d/%d) - намалявам\n
6 BImageControl::BImageControl: грешка при разпределяне на colormap\n
7 BImageControl::BImageControl: не мога да определя цвета %d/%d/%d\n
8 BImageControl::~BImageControl: pixmap кеш - показвам %d pixmaps\n
9 BImageControl::renderImage: кеша е голям, форсирам изчистване\n
10 BImageControl::getColor: синтактична грешка на цвета: '%s'\n
11 BImageControl::getColor: грешка определяне на цвета: '%s'\n

$set 6 #Screen

1 BScreen::BScreen: появи се грешка при проверяването на X сървъра.\n  \
друг window manager вече работи на дисплей %s.\n
2 BScreen::BScreen: ръководещия екран %d ползва visual 0x%lx, дълбочина %d\n
3 BScreen::LoadStyle(): не мога да заредя шрифт '%s'\n
4 BScreen::LoadStyle(): не мога да заредя шрифта по подразбиране.\n
5 %s: празем меню файл\n
6 xterm
7 Рестартиране
8 Изход
9 BScreen::parseMenuFile: [exec] грешка, няма етикет на менюто и/или определена команда\n
10 BScreen::parseMenuFile: [exit] грешка, няма определен етикет на менюто\n
11 BScreen::parseMenuFile: [style] грешка, няма етикет на менюто и/или име на файл \
defined\n
12 BScreen::parseMenuFile: [config] грешка, няма определен етикет на менюто\n
13 BScreen::parseMenuFile: [include] грешка, не е определено име на файл\n
14 BScreen::parseMenuFile: [include] грешка, '%s' не е правилен файл\n
15 BScreen::parseMenuFile: [submenu] грешка, няма определен етикет на менюто\n
16 BScreen::parseMenuFile: [restart] грешка, няма определен етикет на менюто\n
17 BScreen::parseMenuFile: [reconfig] грешка, няма определен етикет на менюто\n
18 BScreen::parseMenuFile: [stylesdir/stylesmenu]грешка, няма определена директория\n
19 BScreen::parseMenuFile: [stylesdir/stylesmenu] грешка, '%s' не е \
директория\n
20 BScreen::parseMenuFile: [stylesdir/stylesmenu] грешка, '%s' не съществува\n
21 BScreen::parseMenuFile: [workspaces] грешка, няма определен етикет на менюто\n
22 0: 0000 x 0: 0000
23 X: %4d x Y: %4d
24 W: %4d x H: %4d


$set 7 #Slit

1 Slit
2 Posoka na Slit -a
3 Разположение на Slit -а

$set 8 #Toolbar

1 00:00000
2 %02d/%02d/%02d
3 %02d.%02d.%02d
4  %02d:%02d 
5 %02d:%02d %sm
6 p
7 a
8 Toolbar
9 Редактирай името на текущото Работно Място
10 Разположение на Toolbar -а

$set 9 #Window


1 BlackboxWindow::BlackboxWindow: създавам 0x%lx\n
2 BlackboxWindow::BlackboxWindow: XGetWindowAttributres пропадна\n
3 BlackboxWindow::BlackboxWindow: не мога да намеря екран за коренния прозорец 0x%lx\n
4 Безиме
5 BlackboxWindow::mapRequestEvent() for 0x%lx\n
6 BlackboxWindow::unmapNotifyEvent() for 0x%lx\n
7 BlackboxWindow::unmapnotifyEvent: reparent 0x%lx to root\n

$set 10 #Windowmenu

1 Прати на ...
2 Прати Групата на ...
3 Засенчи
4 Иконизирай
5 Увеличи
6 Повдигни
7 Сниши
8 Забоди
9 Убий Клиента
10 Затвори
11 Tab

$set 11 #Workspace

1 Работно Място %d

$set 12 #Workspacemenu

1 Работни Места
2 Ново Работно Място
3 Махни Последното

$set 13 #blackbox

1 Blackbox::Blackbox: no managable screens found, aborting\n
2 Blackbox::process_event: MapRequest for 0x%lx\n

$set 14 #Common

1 Да
2 Не

3 Направление
4 Хоризонтално
5 Верткално

6 Винаги отгоре

7 Разположение
8 Горе в Ляво
9 Ляво в Средата
10 Долу в Ляво
11 Горе в Средата
12 Долу в Средата
13 Горе в Дясно
14 Дясно в Средата
15 Долу в Дясно
16 Горе в Ляво
17 Ляво в Средата
18 Долу в Ляво
19 Горе в Дясно
20 Дясно в Средата
21 Долу в Дясно
22 Горе Цялостно
23 Долу Цялостно
24 Ляво Цялостно
25 Дясно Цялостно
26 Автоматично скриване

$set 15 #main

1 грешка: '-rc' изисква аргумент\n
2 грешка: '-display' изисква аргумент\n
3 внимание: не мога да наглася променливата на средата 'DISPLAY'\n
4 Fluxkbox %s: (c) 2001 Henrik Kinnunen\n\n\
  -display <string>\t\tuse display connection.\n\
  -rc <string>\t\t\tползвай заместващ ресурсен файл.\n\
  -version\t\t\tпокажи версията и излез.\n\
  -help\t\t\t\tпокажи този помощен текст и излез.\n\n
5 Избрани възможности по време на компилацията:\n\
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


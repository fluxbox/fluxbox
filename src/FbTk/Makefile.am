noinst_LIBRARIES = libFbTk.a

if XFT
xft_SOURCE= XftFontImp.hh XftFontImp.cc
endif
if MULTIBYTE
xmb_SOURCE= XmbFontImp.hh XmbFontImp.cc
endif

libFbTk_a_SOURCES = App.hh App.cc Color.cc Color.hh Command.hh \
	EventHandler.hh EventManager.hh EventManager.cc \
	FbWindow.hh FbWindow.cc Font.cc Font.hh FontImp.hh \
	Menu.hh Menu.cc MenuTheme.hh MenuTheme.cc NotCopyable.hh \
	RefCount.hh SimpleCommand.hh SignalHandler.cc SignalHandler.hh \
	Text.hh Text.cc \
	Texture.cc Texture.hh Theme.hh Theme.cc \
	XFontImp.cc XFontImp.hh \
	Button.hh Button.cc \
	${xft_SOURCE} \
	${xmb_SOURCE}

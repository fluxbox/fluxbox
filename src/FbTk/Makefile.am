noinst_LIBRARIES = libFbTk.a

if XFT
xft_SOURCE= XftFontImp.hh XftFontImp.cc
endif
if MULTIBYTE
xmb_SOURCE= XmbFontImp.hh XmbFontImp.cc
endif

libFbTk_a_SOURCES = App.hh App.cc Color.cc Color.hh EventHandler.hh Font.cc Font.hh FontImp.hh NotCopyable.hh \
	SignalHandler.cc SignalHandler.hh  Texture.cc Texture.hh \
	XFontImp.cc XFontImp.hh \
	${xft_SOURCE} \
	${xmb_SOURCE}

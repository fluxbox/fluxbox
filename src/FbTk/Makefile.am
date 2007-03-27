noinst_LIBRARIES = libFbTk.a

AM_CPPFLAGS=@CPPFLAGS@

if XFT
xft_SOURCE= XftFontImp.hh XftFontImp.cc
endif
if MULTIBYTE
xmb_SOURCE= XmbFontImp.hh XmbFontImp.cc
endif
if XPM
xpm_SOURCE= ImageXPM.hh ImageXPM.cc
endif
if IMLIB2
imlib2_SOURCE= ImageImlib2.hh ImageImlib2.cc
endif

libFbTk_a_SOURCES = App.hh App.cc Color.cc Color.hh Command.hh \
	FileUtil.hh FileUtil.cc \
	EventHandler.hh EventManager.hh EventManager.cc \
	FbWindow.hh FbWindow.cc Font.cc Font.hh FontImp.hh \
	I18n.cc I18n.hh \
	ImageControl.hh ImageControl.cc \
	MacroCommand.hh MacroCommand.cc \
	Menu.hh Menu.cc MenuItem.hh MenuItem.cc \
	MultiButtonMenuItem.hh MultiButtonMenuItem.cc \
	MenuTheme.hh MenuTheme.cc NotCopyable.hh \
	RefCount.hh SimpleCommand.hh SignalHandler.cc SignalHandler.hh \
	Text.hh Text.cc \
	Texture.cc Texture.hh TextureRender.hh TextureRender.cc \
	Theme.hh Theme.cc ThemeItems.cc Timer.hh Timer.cc \
	XFontImp.cc XFontImp.hh \
	Button.hh Button.cc \
	TextButton.hh TextButton.cc \
	Layer.hh LayerItem.hh MultLayers.cc MultLayers.hh \
	XLayer.cc XLayer.hh XLayerItem.cc XLayerItem.hh \
	Resource.hh Resource.cc \
	StringUtil.hh StringUtil.cc \
	FbString.hh FbString.cc \
	Subject.hh Subject.cc Observer.hh Observer.cc \
	Transparent.hh Transparent.cc \
	FbPixmap.hh FbPixmap.cc \
	FbDrawable.hh FbDrawable.cc \
	XrmDatabaseHelper.hh \
	Image.hh Image.cc \
	PixmapWithMask.hh \
	Compose.hh CompareEqual.hh \
	TextBox.hh TextBox.cc \
	GContext.hh GContext.cc \
	KeyUtil.hh KeyUtil.cc \
	MenuSeparator.hh MenuSeparator.cc \
	stringstream.hh \
	TypeAhead.hh SearchResult.hh SearchResult.cc ITypeAheadable.hh \
	Select2nd.hh \
	CachedPixmap.hh CachedPixmap.cc \
	${xpm_SOURCE} \
	${xft_SOURCE} \
	${xmb_SOURCE} \
	$(imlib2_SOURCE)

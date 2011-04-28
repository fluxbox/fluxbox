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
	Accessor.hh DefaultValue.hh \
	FileUtil.hh FileUtil.cc \
	EventHandler.hh EventManager.hh EventManager.cc \
	FbWindow.hh FbWindow.cc Font.cc Font.hh FontImp.hh \
	I18n.cc I18n.hh \
	CommandParser.hh \
	RadioMenuItem.hh \
	ImageControl.hh ImageControl.cc \
	LogicCommands.hh LogicCommands.cc \
	MacroCommand.hh MacroCommand.cc \
	Menu.hh Menu.cc MenuItem.hh MenuItem.cc \
	BoolMenuItem.hh IntMenuItem.hh \
	MultiButtonMenuItem.hh MultiButtonMenuItem.cc \
	MenuTheme.hh MenuTheme.cc NotCopyable.hh \
	BorderTheme.hh BorderTheme.cc TextTheme.hh TextTheme.cc \
	RefCount.hh SimpleCommand.hh SignalHandler.cc SignalHandler.hh \
	TextUtils.hh TextUtils.cc Orientation.hh \
	Texture.cc Texture.hh TextureRender.hh TextureRender.cc \
	Shape.hh Shape.cc \
	Theme.hh Theme.cc ThemeItems.cc Timer.hh Timer.cc \
	XFontImp.cc XFontImp.hh \
	Button.hh Button.cc \
	TextButton.hh TextButton.cc \
	Container.hh Container.cc \
	MultLayers.cc MultLayers.hh \
	Layer.cc Layer.hh LayerItem.cc LayerItem.hh \
	Resource.hh Resource.cc \
	StringUtil.hh StringUtil.cc Parser.hh Parser.cc \
	RegExp.hh RegExp.cc \
	FbString.hh FbString.cc \
	AutoReloadHelper.hh AutoReloadHelper.cc \
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
	Select2nd.hh STLUtil.hh \
	CachedPixmap.hh CachedPixmap.cc \
	Slot.hh Signal.hh MemFun.hh SelectArg.hh \
	Util.hh \
	${xpm_SOURCE} \
	${xft_SOURCE} \
	${xmb_SOURCE} \
	$(imlib2_SOURCE)

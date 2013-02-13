AM_CPPFLAGS= -I$(top_srcdir)/src -include $(top_builddir)/config.h

noinst_PROGRAMS= \
	 testTexture \
	 testFont \
	 testSignals \
	 testKeys \
	 testDemandAttention \
	 testFullscreen \
	 testStringUtil \
	 testRectangleUtil

testTexture_SOURCES         = texturetest.cc
testTexture_LDFLAGS	    = $(X_LIBS) $(X11_LIBS) $(FRIBIDI_LIBS) $(XRENDER_LIBS) $(IMLIB2_LIBS) $(XPM_LIBS) $(XFT_LIBS) $(FONTCONFIG_LIBS)
testFont_SOURCES            = testFont.cc
testFont_LDFLAGS	    = $(X_LIBS) $(X11_LIBS) $(FRIBIDI_LIBS) $(XRENDER_LIBS) $(XFT_LIBS) $(FONTCONFIG_LIBS)
testSignals_SOURCES         = testSignals.cc
testSignals_LDFLAGS	    = $(X_LIBS) $(X11_LIBS) $(FRIBIDI_LIBS) $(XRENDER_LIBS)
testKeys_SOURCES            = testKeys.cc
testKeys_LDFLAGS	    = $(X_LIBS) $(X11_LIBS) $(FRIBIDI_LIBS) $(XRENDER_LIBS) $(XFT_LIBS) $(FONTCONFIG_LIBS)
testDemandAttention_SOURCES = testDemandAttention.cc
testDemandAttention_LDFLAGS = $(X_LIBS) $(X11_LIBS) $(FRIBIDI_LIBS) $(XRENDER_LIBS) $(XFT_LIBS) $(FONTCONFIG_LIBS)
#testResource_SOURCES        = Resourcetest.cc
testFullscreen_SOURCES      = fullscreentest.cc
testFullscreen_LDFLAGS	    = $(X_LIBS) $(X11_LIBS) $(FRIBIDI_LIBS) $(XRENDER_LIBS) $(XFT_LIBS) $(FONTCONFIG_LIBS)
testStringUtil_SOURCES      = StringUtiltest.cc
testRectangleUtil_SOURCES   = testRectangleUtil.cc

LDADD=../FbTk/libFbTk.a


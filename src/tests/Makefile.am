INCLUDES= -I$(top_srcdir)/src

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
testFont_SOURCES            = testFont.cc
testSignals_SOURCES         = testSignals.cc
testKeys_SOURCES            = testKeys.cc
testDemandAttention_SOURCES = testDemandAttention.cc
#testResource_SOURCES        = Resourcetest.cc
testFullscreen_SOURCES      = fullscreentest.cc
testStringUtil_SOURCES      = StringUtiltest.cc
testRectangleUtil_SOURCES   = testRectangleUtil.cc

LDADD=../FbTk/libFbTk.a


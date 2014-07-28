AM_CPPFLAGS =                                           \
	-I.                                             \
	-I$(top_srcdir)                                 \
	-DDATADIR=\""$(datadir)"\"              	\
	-DLOCALEDIR=\""$(datadir)/locale"\"		\
	$(LIBFWITUX_CFLAGS) $(WARN_CFLAGS)

noinst_LTLIBRARIES = libfwitux.la

if HAVE_GNOME_KEYRING
keyring_impl =						\
	fwitux-keyring.c	fwitux-keyring.h
endif

libfwitux_la_SOURCES =             			\
	fwitux-conf.c		fwitux-conf.h		\
	fwitux-debug.c		fwitux-debug.h		\
	fwitux-paths.c 		fwitux-paths.h		\
	fwitux-xml.c		fwitux-xml.h		\
	$(keyring_impl)

libfwitux_la_LIBADD =              			\
	$(LIBFWITUX_LIBS)

libfwituxincludedir = $(includedir)/fwitux/



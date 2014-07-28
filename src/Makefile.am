AM_CPPFLAGS=						\
	-I$(top_srcdir)                          	\
	-DPREFIX="\"$(prefix)"\"			\
	-DSYSCONFDIR=\""$(sysconfdir)"\"		\
	-DDATADIR=\""$(datadir)"\"              	\
	-DLIBDIR=\""$(libdir)"\"			\
	$(FWITUX_CFLAGS)				\
	$(ENCHANT_CFLAGS)				\
	$(WARN_CFLAGS)

bin_PROGRAMS = fwitux

if HAVE_DBUS
dbus_sources = fwitux-dbus.c fwitux-dbus.h
endif

fwitux_SOURCES = 							\
	$(dbus_sources)							\
	fwitux.h							\
	fwitux-main.c							\
	fwitux-about.c			fwitux-about.h			\
	fwitux-account-dialog.c 	fwitux-account-dialog.h		\
	fwitux-app.c			fwitux-app.h			\
	fwitux-replies-window.c	fwitux-replies-window.h	\
	fwitux-reply-list.c		fwitux-reply-list.h     \
	fwitux-geometry.c		fwitux-geometry.h		\
	fwitux-hint.c			fwitux-hint.h			\
	fwitux-label.c			fwitux-label.h			\
	fwitux-network.c		fwitux-network.h		\
	fwitux-parser.c			fwitux-parser.h			\
	fwitux-preferences.c		fwitux-preferences.h		\
	fwitux-message-dialog.c	fwitux-message-dialog.h	\
	fwitux-spell.c			fwitux-spell.h			\
	fwitux-spell-dialog.c		fwitux-spell-dialog.h		\
	fwitux-ui-utils.c		fwitux-ui-utils.h		\
	fwitux-lists-dialog.c		fwitux-lists-dialog.h		\
	fwitux-tweet-list.c		fwitux-tweet-list.h

fwitux_LDADD=						\
	$(top_builddir)/libfwitux/libfwitux.la		\
	$(FWITUX_LIBS)					\
	$(ENCHANT_LIBS)

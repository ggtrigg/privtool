# Generated automatically from Makefile.in by configure.



srcdir =	.
VPATH =		.:./liteclue:./linux

CC =		pgcc
CFLAGS =	-g  -I/usr/X11R6/include
CPPFLAGS =	 -I/usr/X11R6/include -I/opt/lib/glib/include -I/opt/include  -DHAVE_CONFIG_H -I$(srcdir)  $(LINUX_CPP)
LDFLAGS =	   -L/usr/X11R6/lib 
LDLIBS =	-lgdbm  -L/opt/lib -L/usr/X11R6/lib -lgtk -lgdk -rdynamic -lgmodule -lglib -ldl -lXext -lX11 -lm   -lSM -lICE 

INSTALL =	/bin/ginstall -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA =	${INSTALL} -m 644

# Mixmaster variables
MIXPATH =	$(HOME)/.mix
MIXFLAGS =	-DMINMIX=3 -DMAXMIX=4 -DMINMIXREL=9500 -DMINMIXPATHREL=9000

# GUI specific source files.
XVIEW_SRC =	x.c xprops.c
MOTIF_SRC =	motif.c mfolder.c mprops.c pixmapcache.c m_util.c \
		liteclue/LiteClue.c
MOTIF_CPP =	-I$(srcdir)/liteclue
GTK_SRC =	gtk.c

# OS specific source files.
LINUX_SOURCE =	linux/gettime.c linux/parsedate.c
LINUX_CPP =	-I$(srcdir)/linux

# Floppy source file(s).
FLOPPY_SRC =	floppy.c

GUI_SRC =	$(GTK_SRC)
OS_SOURCE =	$(LINUX_SOURCE)

SOURCE =	pgplib.c buffers.c messages.c main.c gui.c mail_reader.c \
		$(GUI_SRC) $(OS_SOURCE) 

OBJS =		$(notdir $(SOURCE:%.c=%.o))

privtool:	$(OBJS)
		$(LINK.c) $(LDFLAGS) -o $@ $(OBJS) $(LDLIBS)

$(DIRS):
		mkdir $@

.PHONY:		clean

clean:
		$(RM) privtool $(OBJS)

depend:		.depend

.depend:	$(SOURCE)
		$(CC) $(CFLAGS) $(CPPFLAGS) -M $(SOURCE) > .depend

include .depend

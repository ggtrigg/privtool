# Generated automatically from Makefile.in by configure.



srcdir =	.
VPATH =		.:./liteclue:./linux

CC =		pgcc -mpentium
CFLAGS =	-O6  -I/usr/X11R6/include
CPPFLAGS =	   -DHAVE_CONFIG_H -I$(srcdir) $(MOTIF_CPP) $(LINUX_CPP)
LDFLAGS =	     -L/usr/X11R6/lib 
LDLIBS =	-lgdbm -lXbae -lXm -lXpm -lXext -lXmu -lXt  -lm -lX11   -lSM -lICE 

INSTALL =	/bin/ginstall -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA =	${INSTALL} -m 644

# Mixmaster variables
MIXPATH =	$(HOME)/.mix
MIXFLAGS =	-DMINMIX=3 -DMAXMIX=4 -DMINMIXREL=9500 -DMINMIXPATHREL=9000

# GUI specific source files.
XVIEW_SRC =	x.c xprops.c
MOTIF_SRC =	motif.c mfolder.c mprops.c pixmapcache.c m_util.c
MOTIF_CPP =	-I$(srcdir)/liteclue

# OS specific source files.
LINUX_SOURCE =	linux/gettime.c linux/parsedate.c
LINUX_CPP =	-I$(srcdir)/linux

# Floppy source file(s).
FLOPPY_SRC =	floppy.c

GUI_SRC =	$(MOTIF_SRC)
OS_SOURCE =	$(LINUX_SOURCE)

SOURCE =	pgplib.c buffers.c messages.c main.c gui.c mail_reader.c \
		liteclue/LiteClue.c \
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

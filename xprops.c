/*
 *	@(#)xprops.c	1.2 6/11/96
 *
 *	(c) Copyright 1993-1996 by Mark Grant, and by other
 *	authors as appropriate. All right reserved.
 *
 *	The authors assume no liability for damages resulting from the 
 *	use of this software, even if the damage results from defects in
 *	this software. No warranty is expressed or implied.
 *
 *	This software is being distributed under the GNU Public Licence,
 *	see the file COPYING for more details.
 *
 *			- Mark Grant (mark@unicorn.com) 29/6/94
 *	
 *      Menu item for selecting Mail files added by
 *      	- Keith Paskett (keith.paskett@sdl.usu.edu) 8 Sep 1994
 *
 *	Linux compatibility changes by 
 *		- David Summers (david@actsn.fay.ar.us) 6th July 1995
 *
 *	Various changes 
 *		- Anders Baekgaard (baekgrd@ibm.net) 10th August 1995
 *      Linux icon changes by
 *              - Alan Teeder (ajteeder@dra.hmg.gb) 1 Sept 1995
 *                      changed to use .xbm files - some linux systems
 *                      corrupt normal icons.
 *      Added panels for "Privtool" & "Privtool 2" panels to Properties;
 *      Made a general-purpose function for writing to .mailrc or .privrc
 *              - Scott Cannon Jr. (scottjr@silver.cal.sdl.usu.edu)
 *                      30 May 1996
 */

#ifdef SVR4
#include <stdio.h>
#else /* !SVR4 */
#include <stdlib.h>
#include <unistd.h>
#include <sys/param.h>
#include <sys/dirent.h>
#endif /* SVR4 */
#include <ctype.h>
#include <malloc.h>
#include <dirent.h>
#include <sys/stat.h>
#ifndef linux
#include <X11/Xos.h>
#else
#include <signal.h>
#define SCO324
#endif
#ifndef MAXPATHLEN
#include <sys/param.h>
#endif /* MAXPATHLEN */
#include <xview/xview.h>
#include <xview/frame.h>
#include <xview/panel.h>
#include <xview/canvas.h>
#include <xview/sel_attrs.h>
#include <xview/textsw.h>
#include <xview/notice.h>
#include <xview/svrimage.h>
#include <xview/icon.h>
#include <xview/font.h>
#include <xview/scrollbar.h>
#include <xview/notify.h>
#include <xview/cms.h>
#include <xview/defaults.h>
#ifdef linux
#include <X11/Xos.h>
#endif

#include "def.h"
#include "mailrc.h"

/* These really shouldn't be hard-coded. But, when this is ported to
   C++ we won't have to care any more. Just stuff it up to 4k for
   buffers for now. */

#define BUFLEN	4096
#define STRLEN	256

/*global variables */

extern  char    globRCfile[MAXPATHLEN];  /* from main.c */
extern  char    globRCbak[MAXPATHLEN];   /* from main.c */
extern	Frame	main_frame;
extern	int	font_size;
extern	Panel_item mail_file_button;

extern 	LIST	mailrc;

/*local variables */

static	Panel	top_panel,alias_panel,file_panel,privtool_panel,priv2_panel;
static	Panel	display_panel;
 
static	Frame		props_frame= (Frame) XV_NULL;	 
static	Panel		panel;
static 	Panel_item	alias_list,file_list,pgpkeyxlat_list,pgpnyms_list;
static 	Panel_item	alias_text,address_text,dir_text,file_text,dir_num;
static  Panel_item      pgpshortname_text, pgpkeyname_text, pgpnym_text;
static  Panel_item      pgpdefnym_text,pgp_sec,pgp_testintv;
static  Panel_item      pgpkill_list,pgpkillu_text,pgpkills_text;

static void replace_in_file(linestart, writeBuf)
char *linestart;
char *writeBuf;
{
  int endpos;
  int writtenOut = 0;
  char *buffer, *line, *newline, c;
  FILE *fp;

  fp = fopen(globRCfile, "r");  /* read-only */
  if (!fp)
    {
      fprintf(stderr, "ERROR: Unable to open %s\n", globRCfile);
      exit(1);
    }
  fseek(fp, 0, SEEK_END);
  endpos = ftell(fp);
  buffer = (char *) malloc(sizeof(char)*(endpos + 1));
  if (!buffer)
    {
      fprintf(stderr, "ERROR: Out of memory\n");
      return;
    }
  fseek(fp, 0, SEEK_SET);
  fread(buffer, sizeof(char), endpos, fp);
  buffer[endpos] = 0;
  fclose(fp);
  unlink(globRCbak);
  if (rename(globRCfile, globRCbak))
    {
      fprintf(stderr, "ERROR: Can't rename %s to %s\n", globRCfile, globRCbak);
      free (buffer);
      return;
    }
  fp = fopen(globRCfile, "w");  /* create for writing */
  if (!fp)
    {
      fprintf(stderr, "ERROR: Can't write to %s\n", globRCfile);
      return;
    }
  line = buffer;
  while (line)
    {
      if (line != buffer)
	line++;  /* advance past newline */
      if (strncmp(line, linestart, strlen(linestart)) == 0)
	{
	  if (!writtenOut)
	    {
	      if (!fwrite(writeBuf, sizeof(char), strlen(writeBuf), fp)
		  && strlen(writeBuf) > 0)
		{
		  fclose(fp);
		  fprintf(stderr, "ERROR: Unable to write buffer to %s\n",
			  globRCfile);
		  rename(globRCbak, globRCfile);
		  free (buffer);
		  return;
		}
	      writtenOut = 1;
	    }
	}
      else
	{
	  newline = strchr(line, '\n');
	  if (newline)
	    {
	      c = *newline;
	      *newline = 0;
	      fputs(line, fp);
	      fputc('\n', fp);
	      *newline = c;
	    }
	  else
	    {
	      fputs(line, fp);
	    }
	}
      line = strchr(line, '\n');
    }
  if (!writtenOut)  /*If not written still, append to end-of-file:*/
    { 
      if (!fwrite(writeBuf, sizeof(char), strlen(writeBuf), fp)
	  && strlen(writeBuf) > 0)
	{
	  fclose(fp);
	  fprintf(stderr, "ERROR: Unable to write buffer to %s\n",
		  globRCfile);
	  rename(globRCbak, globRCfile);
	  free (buffer);
	  return;
	}
    }
  fclose(fp);
  free(buffer);
}

static 	void	reset_file (item,event)

Panel_item	item;
Event		*event;

{	
	xv_set(dir_text, 	PANEL_VALUE, "", NULL);
	xv_set(file_text,	PANEL_VALUE, "", NULL);
}

static 	void	add_file(item,event)

Panel_item	item;
Event		*event;

{	
	xv_set(file_list,
		PANEL_LIST_INSERT, 0,
		PANEL_LIST_STRING, 0, xv_get(file_text,PANEL_VALUE),
		NULL);
}

static 	void	delete_file (item,event)

Panel_item	item;
Event		*event;

{	int selected;

	selected = (int) xv_get(file_list,PANEL_LIST_FIRST_SELECTED);
        if (selected >= 0 ) {
		xv_set(file_list,
			PANEL_LIST_DELETE,	selected,
			NULL);
	}
}

static 	void	change_file (item,event)

Panel_item	item;
Event		*event;

{	int selected;
	char buf[128];

	selected = (int) xv_get(file_list,PANEL_LIST_FIRST_SELECTED);

        if (selected >= 0 ) {
		strcpy(buf, (char *) xv_get(file_text,PANEL_VALUE));
  
		xv_set(file_list,
			PANEL_LIST_DELETE,	selected,
			NULL);
		xv_set(file_list,
			PANEL_LIST_INSERT,	selected,
			PANEL_LIST_STRING,	selected,	buf,
			NULL);
	}
}

static 	void	save_file (item,event)

Panel_item	item;
Event		*event;

{	int 	n,i, pos, fsize;
	char 	buf[BUFLEN],str[STRLEN],filename[MAXPATHLEN];
	char	*p_buf,*pivot,*pivot1;
	FILE 	*privrc=NULL;

	n = (int ) xv_get(file_list,PANEL_LIST_NROWS);
	strcpy(buf,"set folder=\'");
	strcat(buf,(char *) xv_get(dir_text,PANEL_VALUE));
	strcat(buf,"\'\n");
	replace_in_file("set folder=", buf);

	strcpy(buf,"set filemenusize=\'");
	sprintf(str,"%i",(int) xv_get(dir_num,PANEL_VALUE));
	strcat(buf,str);
	strcat(buf,"\'\n");
	replace_in_file("set filemenusize=", buf);

	strcpy(buf,"set filemenu2=\'");
	n = (int ) xv_get(file_list,PANEL_LIST_NROWS);
	for (i = 0 ; i < n; i=i+1) {
	  strcat(buf,(char *) xv_get(file_list,PANEL_LIST_STRING,i));
	  strcat(buf," ");
	}
	strcat(buf,"\'\n");
	replace_in_file("set filemenu2=", buf);

	clear_list(&mailrc);
	read_mailrc();
}

static 	void 	load_file()

{	int	filemenusize,i;
	char	*str,*buf,*charp,nullstr[2];

	bzero (nullstr, 2);

	xv_set(dir_text,
		PANEL_VALUE,find_mailrc("folder"),
		NULL);
	
        charp = (char *) find_mailrc("filemenusize");
	if (charp)
	  i = sscanf (charp,"%d",&filemenusize);
	else
	  i = 0;
	
	if (i != 1) filemenusize = 10; /* default value */

	xv_set(dir_num,
		PANEL_VALUE,filemenusize,
		NULL);


        charp = (char *) find_mailrc("filemenu2");
	if (charp)
	  str = strdup(charp);
	else
	  str = nullstr;
	
	buf = strtok(str," \0");
	i=0;
	while(buf != NULL) {
		xv_set(file_list,
			PANEL_LIST_STRING, i, buf,
			NULL);
		buf = strtok(NULL," \0");
		i = i+1;
		}
}


static 	void 	render_file_panel()

{	int	height,width;

	height = 430;


	file_panel = (Panel) xv_create(props_frame,PANEL,
		XV_Y,		40,
		WIN_BORDER,	TRUE,
		XV_HEIGHT,	height,
		NULL);

	width  = xv_get(alias_panel,XV_WIDTH);



	dir_text =  xv_create(file_panel,PANEL_TEXT,
		PANEL_LABEL_STRING,"Mail File Directory:",
		PANEL_LABEL_WIDTH,font_size*11,
		PANEL_VALUE_DISPLAY_LENGTH,30,
		NULL);

	dir_num =  xv_create(file_panel,PANEL_NUMERIC_TEXT,
		PANEL_NEXT_ROW,2*font_size,
		PANEL_LABEL_STRING,"Display Up To:",
		PANEL_LABEL_WIDTH,font_size*11,
		PANEL_VALUE_DISPLAY_LENGTH,2,
		NULL);
	
	xv_create(file_panel,PANEL_MESSAGE,
		PANEL_LABEL_STRING,"Files in Menus",
		NULL);

	file_list = xv_create(file_panel,PANEL_LIST,
		PANEL_LABEL_STRING,"Move, Copy, Load Menus:",
		PANEL_LABEL_WIDTH,font_size*14,
		PANEL_LIST_DISPLAY_ROWS,10,
		PANEL_LIST_WIDTH,width/2-font_size*5,
		NULL);

	file_text =  xv_create(file_panel,PANEL_TEXT,
		PANEL_LABEL_STRING,"Permanent File:",
		PANEL_LABEL_WIDTH,font_size*14,
		PANEL_VALUE_DISPLAY_LENGTH,30,
		NULL);

	xv_create(file_panel,PANEL_BUTTON,
		PANEL_LABEL_STRING, "Add",
		PANEL_NOTIFY_PROC,add_file,
		PANEL_LABEL_WIDTH,font_size*4,
		XV_X,width/2+font_size*15,
		XV_Y,120,
		NULL);

	xv_create(file_panel,PANEL_BUTTON,
		PANEL_LABEL_STRING, "Delete",
		PANEL_NOTIFY_PROC,delete_file,
		PANEL_LABEL_WIDTH,font_size*4,
		XV_Y,120+font_size*2,
		XV_X,width/2+font_size*15,
		NULL);

	xv_create(file_panel,PANEL_BUTTON,
		PANEL_LABEL_STRING, "Change",
		PANEL_NOTIFY_PROC,change_file,
		PANEL_LABEL_WIDTH,font_size*4,
		XV_Y,120+font_size*4,
		XV_X,width/2+font_size*15,
		NULL);

	xv_create(file_panel,PANEL_BUTTON,
		PANEL_LABEL_STRING, 	"Apply",
		PANEL_NOTIFY_PROC, 	save_file,
		PANEL_NEXT_ROW,font_size*2,
		XV_X,			width/2-font_size*5,
		NULL);

	xv_create(file_panel,PANEL_BUTTON,
		PANEL_LABEL_STRING, 	"Reset",
		PANEL_NOTIFY_PROC, 	reset_file,
		NULL);

	load_file();

}

static 	void	add_alias (item,event)

Panel_item	item;
Event		*event;

{	char buf[BUFLEN];

	strcpy(buf, (char *) xv_get(alias_text,PANEL_VALUE));
        strcat(buf,"=");
	strcat(buf, (char *) xv_get(address_text,PANEL_VALUE));

	xv_set(alias_list,
		PANEL_LIST_INSERT,	0,
		PANEL_LIST_STRING,	0,	buf,
		NULL);

	xv_set(alias_list, PANEL_LIST_SORT, PANEL_FORWARD, NULL);
}


static 	void	reset_alias (item,event)

Panel_item	item;
Event		*event;

{	
	xv_set(alias_text, 	PANEL_VALUE, "", NULL);
	xv_set(address_text,	PANEL_VALUE, "", NULL);
}


static 	void	delete_alias (item,event)

Panel_item	item;
Event		*event;

{	int selected;

	selected = (int) xv_get(alias_list,PANEL_LIST_FIRST_SELECTED);
        if (selected >= 0 ) {
		xv_set(alias_list,
			PANEL_LIST_DELETE,	selected,
			NULL);
	}
}

static 	void	change_alias (item,event)

Panel_item	item;
Event		*event;

{	int selected;
	char buf[128];

	selected = (int) xv_get(alias_list,PANEL_LIST_FIRST_SELECTED);

        if (selected >= 0 ) {
		strcpy(buf, (char *) xv_get(alias_text,PANEL_VALUE));
      		strcat(buf,"=");
		strcat(buf, (char *) xv_get(address_text,PANEL_VALUE));

		xv_set(alias_list,
			PANEL_LIST_DELETE,	selected,
			NULL);
		xv_set(alias_list,
			PANEL_LIST_INSERT,	selected,
			PANEL_LIST_STRING,	selected,	buf,
			NULL);
	}
	xv_set(alias_list, PANEL_LIST_SORT, PANEL_FORWARD, NULL);
}

static 	void	save_alias (item,event)

Panel_item	item;
Event		*event;

{	int 	n,i;
	char 	buf[BUFLEN],str[STRLEN];

	buf[0] = 0;
	n = (int ) xv_get(alias_list,PANEL_LIST_NROWS);

	for (i = 0 ; i < n; i=i+1) {
	  strcpy(str,(char *) xv_get(alias_list,PANEL_LIST_STRING,i));
	  strcat(buf,"alias ");
	  strcat(buf,strtok(str,"="));
	  strcat(buf," ");
	  /* V--Spaces can separate alias addresses--V */
	  strcat(buf,strtok(NULL,"\n"));
	  strcat(buf,"\n");
	}
	replace_in_file("alias ", buf);
}

static 	void	load_alias ()


{ 	char	buf[BUFLEN],*p_buf,str[STRLEN];
	FILE 	*privrc;


	privrc = fopen (globRCfile,"r");
	if (privrc) {
		while(TRUE) {
			p_buf=fgets(buf,BUFLEN,privrc);
			if(!p_buf) break;
			strcpy(str,strtok(p_buf," "));
			if (strncmp(str,"alias",strlen(str))==0) { 
				strcpy(str,strtok(NULL," "));
				strcat(str,"=");		
				strcat(str,strtok(NULL,"\n"));
				xv_set(alias_list,
					PANEL_LIST_INSERT,	0,
					PANEL_LIST_STRING,	0,	str,
					NULL);
			}
		}
	}

	xv_set(alias_list, PANEL_LIST_SORT, PANEL_FORWARD, NULL);

	fclose(privrc);
}

static  void	render_alias_panel()

{	int		height,width;


		height = 430;
		
		alias_panel = (Panel) xv_create(props_frame,PANEL,
		XV_Y,		40,
		WIN_BORDER,	TRUE,
		XV_HEIGHT,	height,		
		NULL);


	width  = xv_get(alias_panel,XV_WIDTH);

	alias_list = xv_create(alias_panel,PANEL_LIST,
		PANEL_LABEL_STRING,"Aliases:",
		PANEL_LABEL_WIDTH,font_size*6,
		PANEL_LIST_DISPLAY_ROWS,12,
		PANEL_LIST_WIDTH,width/2+font_size*6,	
		NULL);

	xv_create(alias_panel,PANEL_BUTTON,
		PANEL_LABEL_STRING, "Add",
		PANEL_NOTIFY_PROC,add_alias,
		PANEL_LABEL_WIDTH,font_size*4,
		XV_X,width/2+font_size*16,
		XV_Y,100,
		NULL);

	xv_create(alias_panel,PANEL_BUTTON,
		PANEL_LABEL_STRING, "Delete",
		PANEL_NOTIFY_PROC,delete_alias,
		PANEL_LABEL_WIDTH,font_size*4,
		XV_X,width/2+font_size*16,
		XV_Y,100+font_size*2,
		NULL);

	xv_create(alias_panel,PANEL_BUTTON,
		PANEL_LABEL_STRING, "Change",
		PANEL_NOTIFY_PROC,change_alias,
		PANEL_LABEL_WIDTH,font_size*4,
		XV_X,width/2+font_size*16,
		XV_Y,100+font_size*4,
		NULL);

	alias_text = xv_create(alias_panel,PANEL_TEXT,
		PANEL_LABEL_STRING,"Alias:",
		PANEL_LABEL_WIDTH,font_size*6,
		PANEL_VALUE_DISPLAY_LENGTH,50,
		NULL);

	address_text = xv_create(alias_panel,PANEL_TEXT,
		PANEL_LABEL_STRING,"Addresses:",
		PANEL_LABEL_WIDTH,font_size*6,
		PANEL_VALUE_DISPLAY_LENGTH,50,
		NULL);


	xv_create(alias_panel,PANEL_BUTTON,
		PANEL_LABEL_STRING, 	"Apply",
		PANEL_NOTIFY_PROC, 	save_alias,
		PANEL_NEXT_ROW,font_size*2,
		XV_X,			width/2-font_size*5,
		NULL);

	xv_create(alias_panel,PANEL_BUTTON,
		PANEL_LABEL_STRING, 	"Reset",
		PANEL_NOTIFY_PROC, 	reset_alias,
		NULL);

		load_alias();
}

static 	void	add_pgpnym (item,event)

Panel_item	item;
Event		*event;

{	char buf[STRLEN];

	strcpy(buf, (char *) xv_get(pgpnym_text,PANEL_VALUE));

	xv_set(pgpnyms_list,
	       PANEL_LIST_INSERT,	0,
	       PANEL_LIST_STRING,	0,	buf,
	       NULL);

	xv_set(pgpnyms_list, PANEL_LIST_SORT, PANEL_FORWARD, NULL);
}

static 	void	add_pgpkeyxlat (item,event)

Panel_item	item;
Event		*event;

{	char buf[STRLEN];

	strcpy(buf, (char *) xv_get(pgpshortname_text,PANEL_VALUE));
        strcat(buf,"=");
	strcat(buf, (char *) xv_get(pgpkeyname_text,PANEL_VALUE));

	xv_set(pgpkeyxlat_list,
	       PANEL_LIST_INSERT,	0,
	       PANEL_LIST_STRING,	0,	buf,
	       NULL);

	xv_set(pgpkeyxlat_list, PANEL_LIST_SORT, PANEL_FORWARD, NULL);
}

static 	void	reset_privprops (item,event)

Panel_item	item;
Event		*event;

{	
	xv_set(pgpshortname_text, PANEL_VALUE, "", NULL);
	xv_set(pgpkeyname_text, PANEL_VALUE, "", NULL);
	xv_set(pgpnym_text, PANEL_VALUE, "", NULL);
	xv_set(pgpdefnym_text, PANEL_VALUE, "", NULL);
}

static 	void	delete_pgpnym (item,event)

Panel_item	item;
Event		*event;

{	int selected;

	selected = (int) xv_get(pgpnyms_list,PANEL_LIST_FIRST_SELECTED);
        if (selected >= 0 ) {
		xv_set(pgpnyms_list,
			PANEL_LIST_DELETE,	selected,
			NULL);
	}
}

static 	void	delete_pgpkeyxlat (item,event)

Panel_item	item;
Event		*event;

{	int selected;

	selected = (int) xv_get(pgpkeyxlat_list,PANEL_LIST_FIRST_SELECTED);
        if (selected >= 0 ) {
		xv_set(pgpkeyxlat_list,
			PANEL_LIST_DELETE,	selected,
			NULL);
	}
}

static 	void	change_pgpnym (item,event)

Panel_item	item;
Event		*event;

{	int selected;
	char buf[128];

	selected = (int) xv_get(pgpnyms_list,PANEL_LIST_FIRST_SELECTED);

        if (selected >= 0 ) {
		strcpy(buf, (char *) xv_get(pgpnym_text,PANEL_VALUE));

		xv_set(pgpnyms_list,
			PANEL_LIST_DELETE,	selected,
			NULL);
		xv_set(pgpnyms_list,
			PANEL_LIST_INSERT,	selected,
			PANEL_LIST_STRING,	selected,	buf,
			NULL);
	}
	xv_set(pgpnyms_list, PANEL_LIST_SORT, PANEL_FORWARD, NULL);
}

static 	void	change_pgpkeyxlat (item,event)

Panel_item	item;
Event		*event;

{	int selected;
	char buf[128];

	selected = (int) xv_get(pgpkeyxlat_list,PANEL_LIST_FIRST_SELECTED);

        if (selected >= 0 ) {
		strcpy(buf, (char *) xv_get(pgpshortname_text,PANEL_VALUE));
      		strcat(buf,"=");
		strcat(buf, (char *) xv_get(pgpkeyname_text,PANEL_VALUE));

		xv_set(pgpkeyxlat_list,
			PANEL_LIST_DELETE,	selected,
			NULL);
		xv_set(pgpkeyxlat_list,
			PANEL_LIST_INSERT,	selected,
			PANEL_LIST_STRING,	selected,	buf,
			NULL);
	}
	xv_set(pgpkeyxlat_list, PANEL_LIST_SORT, PANEL_FORWARD, NULL);
}

static 	void	save_privprops (item,event)

Panel_item	item;
Event		*event;

{	int 	n,i;
	char 	buf[BUFLEN],str[STRLEN],*p;

	buf[0] = 0;
	n = (int) xv_get(pgpkeyxlat_list,PANEL_LIST_NROWS);
	for (i = 0 ; i < n; i=i+1)
	  {
	    strcpy(str,(char *) xv_get(pgpkeyxlat_list,PANEL_LIST_STRING,i));
	    strcat(buf,"#@pgpkey ");
	    strcat(buf,strtok(str,"="));
	    strcat(buf,"=");
	    strcat(buf,strtok(NULL,"\n"));
	    strcat(buf,"\n");
	  }
	replace_in_file("#@pgpkey ", buf);

	buf[0] = 0;
	n = (int) xv_get(pgpnyms_list,PANEL_LIST_NROWS);
	for (i = 0; i < n; i=i+1)
	  {
	    strcpy(str,(char *) xv_get(pgpnyms_list,PANEL_LIST_STRING,i));
	    strcat(buf,"#@pseudonym ");
	    strcat(buf,strtok(str,"\n"));
	    strcat(buf,"\n");
	  }
	replace_in_file("#@pseudonym ", buf);

	buf[0] = 0;
	p = (char *) xv_get(pgpdefnym_text,PANEL_VALUE);
	if (p && strlen(p) > 0)
	  {
	    strcpy(buf,"#@defnym ");
	    strcat(buf,p);
	    strcat(buf,"\n");
	  }
	replace_in_file("#@defnym ", buf);
}

static 	void	load_privprops ()


{ 	char	buf[BUFLEN],*p_buf,str[STRLEN];
	FILE 	*privrc;

	privrc = fopen (globRCfile,"r");
	if (privrc) {
	  while(TRUE) {
	    p_buf=fgets(buf,BUFLEN,privrc);
	    if(!p_buf) break;
	    strcpy(str,strtok(p_buf," "));
	    if (strncmp(str,"#@pgpkey",8)==0) {
	      strcpy(str,strtok(NULL," ="));
	      strcat(str,"=");
	      strcat(str,strtok(NULL,"=#\n"));
	      xv_set(pgpkeyxlat_list,
		     PANEL_LIST_INSERT,	0,
		     PANEL_LIST_STRING,	0,	str,
		     NULL);
	    }
	    else if (strncmp(str,"#@pseudonym",11)==0)
	      {
		strcpy(str,strtok(NULL,"\n"));
		xv_set(pgpnyms_list,
		       PANEL_LIST_INSERT, 0,
		       PANEL_LIST_STRING, 0, str,
		       NULL);
	      }
	    else if (strncmp(str,"#@defnym",8)==0)
	      {
		strcpy(str,strtok(NULL,"\n"));
		xv_set(pgpdefnym_text,
		       PANEL_VALUE,str,
		       NULL);
	      }
	  }
	}

	xv_set(pgpkeyxlat_list, PANEL_LIST_SORT, PANEL_FORWARD, NULL);
	xv_set(pgpnyms_list, PANEL_LIST_SORT, PANEL_FORWARD, NULL);

	fclose(privrc);
}

static  void	render_privtool_panel()

{	int		height,width;


        height = 430;
		
	privtool_panel = (Panel) xv_create(props_frame,PANEL,
					   XV_Y, 40,
					   WIN_BORDER,	TRUE,
					   XV_HEIGHT,	height,
					   NULL);

	width  = xv_get(privtool_panel,XV_WIDTH);

	/* PGP Key Translations: */
	pgpkeyxlat_list = xv_create(privtool_panel,PANEL_LIST,
				    PANEL_LABEL_STRING,"PGP Key Translations:",
				    PANEL_LABEL_WIDTH,font_size*13,
				    PANEL_LIST_DISPLAY_ROWS,3,
				    PANEL_LIST_WIDTH,width/2+font_size*6,
				    NULL);

	xv_create(privtool_panel,PANEL_BUTTON,
		  PANEL_LABEL_STRING,"Add",
		  PANEL_NOTIFY_PROC,add_pgpkeyxlat,
		  PANEL_LABEL_WIDTH,font_size*4,
		  XV_X,font_size*14,
		  XV_Y,90,
		  NULL);

 	xv_create(privtool_panel,PANEL_BUTTON,
		PANEL_LABEL_STRING, "Delete", 
		PANEL_NOTIFY_PROC,delete_pgpkeyxlat, 
		PANEL_LABEL_WIDTH,font_size*4, 
		XV_X,font_size*14+font_size*10, 
		XV_Y,90, 
		NULL); 

	xv_create(privtool_panel,PANEL_BUTTON,
		PANEL_LABEL_STRING, "Change",
		PANEL_NOTIFY_PROC,change_pgpkeyxlat,
		PANEL_LABEL_WIDTH,font_size*4,
		XV_X,font_size*14+font_size*20,
		XV_Y,90,
		NULL);

	pgpshortname_text = xv_create(privtool_panel,PANEL_TEXT,
				      PANEL_LABEL_STRING,"Name:",
				      PANEL_LABEL_WIDTH,font_size*12,
				      PANEL_VALUE_DISPLAY_LENGTH,50,
				      NULL);

	pgpkeyname_text = xv_create(privtool_panel,PANEL_TEXT,
		PANEL_LABEL_STRING,"Keyname:",
		PANEL_LABEL_WIDTH,font_size*12,
		PANEL_VALUE_DISPLAY_LENGTH,50,
		NULL);

	/* Pseudonyms: */
	pgpnyms_list = xv_create(privtool_panel,PANEL_LIST,
				 PANEL_LABEL_STRING,"PGP Pseudonyms:",
				 PANEL_LABEL_WIDTH,font_size*13,
				 PANEL_LIST_DISPLAY_ROWS,3,
				 PANEL_LIST_WIDTH,width/2+font_size*6,
				 XV_X,5,
				 XV_Y,210,
				 NULL);

	xv_create(privtool_panel,PANEL_BUTTON,
		  PANEL_LABEL_STRING,"Add",
		  PANEL_NOTIFY_PROC,add_pgpnym,
		  PANEL_LABEL_WIDTH,font_size*4,
		  XV_X,font_size*14,
		  XV_Y,295,
		  NULL);

 	xv_create(privtool_panel,PANEL_BUTTON,
		  PANEL_LABEL_STRING, "Delete", 
		  PANEL_NOTIFY_PROC,delete_pgpnym, 
		  PANEL_LABEL_WIDTH,font_size*4, 
		  XV_X,font_size*14+font_size*10, 
		  XV_Y,295,
		  NULL); 

	xv_create(privtool_panel,PANEL_BUTTON,
		  PANEL_LABEL_STRING, "Change",
		  PANEL_NOTIFY_PROC,change_pgpnym,
		  PANEL_LABEL_WIDTH,font_size*4,
		  XV_X,font_size*14+font_size*20,
		  XV_Y,295,
		  NULL);

	pgpnym_text = xv_create(privtool_panel,PANEL_TEXT,
				PANEL_LABEL_STRING,"Pseudonym:",
				PANEL_LABEL_WIDTH,font_size*12,
				PANEL_VALUE_DISPLAY_LENGTH,50,
				NULL);

	pgpdefnym_text = xv_create(privtool_panel,PANEL_TEXT,
				   PANEL_LABEL_STRING,"Default Pseudonym:",
				   PANEL_LABEL_WIDTH,font_size*12,
				   PANEL_VALUE_DISPLAY_LENGTH,50,
				   NULL);

	/* Apply & Reset buttons: */
	xv_create(privtool_panel,PANEL_BUTTON,
		PANEL_LABEL_STRING, 	"Apply",
		PANEL_NOTIFY_PROC, 	save_privprops,
		PANEL_NEXT_ROW,font_size*2,
		XV_X,			width/2-font_size*5,
		NULL);

	xv_create(privtool_panel,PANEL_BUTTON,
		PANEL_LABEL_STRING, 	"Reset",
		PANEL_NOTIFY_PROC, 	reset_privprops,
		NULL);

	/* Load initial values: */
	load_privprops();
}

static 	void	add_pgpkill (item,event)

Panel_item	item;
Event		*event;

{	char buf[BUFLEN], *user, *subject;

        user = (char *) xv_get(pgpkillu_text,PANEL_VALUE);
	subject = (char *) xv_get(pgpkills_text,PANEL_VALUE);
	if (user && strlen(user))
	  {
	    strcpy(buf, "From: ");
	    strcat(buf, user);
	    xv_set(pgpkill_list,
		   PANEL_LIST_INSERT,0,
		   PANEL_LIST_STRING,0,buf,
		   NULL);
	  }
	if (subject && strlen(subject))
	  {
	    strcpy(buf, "Subject: ");
	    strcat(buf, subject);
	    xv_set(pgpkill_list,
		   PANEL_LIST_INSERT,0,
		   PANEL_LIST_STRING,0,buf,
		   NULL);
	  }

	xv_set(pgpkill_list, PANEL_LIST_SORT, PANEL_FORWARD, NULL);
}

static 	void	change_pgpkill (item,event)

Panel_item	item;
Event		*event;

{	int selected;
	char buf[128],*user,*subject;

	selected = (int) xv_get(pgpkill_list,PANEL_LIST_FIRST_SELECTED);

        if (selected >= 0 ) {
	        user = (char *) xv_get(pgpkillu_text,PANEL_VALUE);
		subject = (char *) xv_get(pgpkills_text,PANEL_VALUE);
		if (user && strlen(user))
		  {
		    strcpy(buf, "From: ");
		    strcat(buf, user);
		  }
		else if (subject && strlen(subject))
		  {
		    strcpy(buf, "Subject: ");
		    strcat(buf, subject);
		  }
		else
		  return;

		xv_set(pgpkill_list,
			PANEL_LIST_DELETE,	selected,
			NULL);
		xv_set(pgpkill_list,
			PANEL_LIST_INSERT,	selected,
			PANEL_LIST_STRING,	selected,	buf,
			NULL);
	}
	xv_set(pgpkill_list, PANEL_LIST_SORT, PANEL_FORWARD, NULL);
}

static 	void	delete_pgpkill (item,event)

Panel_item	item;
Event		*event;

{	int selected;

	selected = (int) xv_get(pgpkill_list,PANEL_LIST_FIRST_SELECTED);
        if (selected >= 0 ) {
		xv_set(pgpkill_list,
			PANEL_LIST_DELETE,	selected,
			NULL);
	}
}

static 	void	reset_priv2props (item,event)

Panel_item	item;
Event		*event;

{	
  char *p;
  int testintv;

  xv_set(pgpkillu_text, PANEL_VALUE, "", NULL);
  xv_set(pgpkills_text, PANEL_VALUE, "", NULL);
  xv_set(pgp_sec, PANEL_VALUE, security_level(), NULL);
  p = find_mailrc("testinterval");
  if (p)
    testintv = atoi(p);
  else
    testintv = 1500;
  xv_set(pgp_testintv, PANEL_VALUE, testintv, NULL);
}

static 	void	save_priv2props (item,event)

Panel_item	item;
Event		*event;

{	int 	n,i,num;
	char 	buf[BUFLEN],str[STRLEN],*p;

	buf[0] = 0;
	n = (int) xv_get(pgpkill_list,PANEL_LIST_NROWS);
	for (i = 0 ; i < n; i=i+1)
	  {
	    strcpy(str,(char *) xv_get(pgpkill_list,PANEL_LIST_STRING,i));
	    if (strncmp(strtok(str," "),"From:",5)==0)
	      strcat(buf,"#@killu ");
	    else
	      strcat(buf,"#@kills ");
	    strcat(buf,strtok(NULL,"\n "));
	    strcat(buf,"\n");
	  }
	replace_in_file("#@kill", buf);

	buf[0] = 0;
	num = (int) xv_get(pgp_sec,PANEL_VALUE);
	if (num >= 1 && num <= 3)
	  sprintf(buf, "#@security %d\n", num);
	replace_in_file("#@security ", buf);

	buf[0] = 0;
	num = (int) xv_get(pgp_testintv,PANEL_VALUE);
	if (num >= 1)
	  sprintf(buf, "testinterval='%d'\n",num);
	replace_in_file("testinterval",buf);
}

static 	void	load_priv2props ()


{ 	char	buf[BUFLEN],*p_buf,str[STRLEN];
	FILE 	*privrc;
	int     num;

	privrc = fopen (globRCfile,"r");
	if (privrc) {
	  while(TRUE) {
	    p_buf=fgets(buf,BUFLEN,privrc);
	    if(!p_buf) break;
	    strcpy(str,strtok(p_buf," ="));
	    if (strncmp(str,"#@killu",7)==0) {
	      strcpy(str,"From: ");
	      strcat(str,strtok(NULL,"\n"));
	      xv_set(pgpkill_list,
		     PANEL_LIST_INSERT,	0,
		     PANEL_LIST_STRING,	0,	str,
		     NULL);
	    }
	    else if (strncmp(str,"#@kills",7)==0)
	      {
		strcpy(str,"Subject: ");
		strcat(str,strtok(NULL,"\n"));
		xv_set(pgpkill_list,
		       PANEL_LIST_INSERT, 0,
		       PANEL_LIST_STRING, 0, str,
		       NULL);
	      }
	    else if (strncmp(str,"#@security",10)==0)
	      {
		num = atoi(strtok(NULL,"\n"));
		if (num >= 1 && num <= 3)
		  xv_set(pgp_sec,
			 PANEL_VALUE,num,
			 NULL);
	      }
	    else if (strncmp(str,"testinterval",12)==0)
	      {
		char *p;

		p = strchr(strtok(NULL,"\n"),'\'');
		num = atoi(p + 1);
		if (num >= 1)
		  xv_set(pgp_testintv,
			 PANEL_VALUE,num,
			 NULL);
	      }
	  }
	}

	xv_set(pgpkill_list, PANEL_LIST_SORT, PANEL_FORWARD, NULL);

	fclose(privrc);
}

static  void	render_priv2_panel()

{	int		height,width;
        int             testinterval;
	char            *p;


        height = 430;
		
	priv2_panel = (Panel) xv_create(props_frame,PANEL,
					XV_Y, 40,
					WIN_BORDER,	TRUE,
					XV_HEIGHT,	height,
					NULL);

	width  = xv_get(priv2_panel,XV_WIDTH);

	p = find_mailrc("testinterval");
	if (p)
	  testinterval = atoi(p);
	else
	  testinterval = 1500;

	pgp_sec =  xv_create(priv2_panel,PANEL_NUMERIC_TEXT,
			     PANEL_NEXT_ROW,2*font_size,
			     PANEL_LABEL_STRING,"Security Level:",
			     PANEL_LABEL_WIDTH,font_size*18,
			     PANEL_VALUE_DISPLAY_LENGTH,2,
			     PANEL_MIN_VALUE,1,
			     PANEL_MAX_VALUE,3,
			     PANEL_VALUE,security_level(),
			     NULL);

	pgp_testintv = xv_create(priv2_panel,PANEL_NUMERIC_TEXT,
				 PANEL_NEXT_ROW,2*font_size,
				 PANEL_LABEL_STRING,"Test Interval (seconds):",
				 PANEL_LABEL_WIDTH,font_size*18,
				 PANEL_VALUE_DISPLAY_LENGTH,5,
				 PANEL_MIN_VALUE,1,
				 PANEL_MAX_VALUE,86400,  /*24 hours max*/
				 PANEL_VALUE,testinterval,
				 NULL);

       	pgpkill_list = xv_create(priv2_panel,PANEL_LIST,
				 PANEL_LABEL_STRING,"Kill Mail Matching:",
				 PANEL_LABEL_WIDTH,font_size*13,
				 PANEL_LIST_DISPLAY_ROWS,3,
				 PANEL_LIST_WIDTH,width/2+font_size*6,
				 XV_X,5,
				 XV_Y,140,
				 NULL);

	xv_create(priv2_panel,PANEL_BUTTON,
		  PANEL_LABEL_STRING,"Add",
		  PANEL_NOTIFY_PROC,add_pgpkill,
		  PANEL_LABEL_WIDTH,font_size*4,
		  XV_X,font_size*14,
		  XV_Y,225,
		  NULL);

 	xv_create(priv2_panel,PANEL_BUTTON,
		  PANEL_LABEL_STRING, "Delete", 
		  PANEL_NOTIFY_PROC,delete_pgpkill, 
		  PANEL_LABEL_WIDTH,font_size*4,
		  XV_X,font_size*14+font_size*10,
		  XV_Y,225,
		  NULL);

	xv_create(priv2_panel,PANEL_BUTTON,
		  PANEL_LABEL_STRING, "Change",
		  PANEL_NOTIFY_PROC,change_pgpkill,
		  PANEL_LABEL_WIDTH,font_size*4,
		  XV_X,font_size*14+font_size*20,
		  XV_Y,225,
		  NULL);

	pgpkillu_text = xv_create(priv2_panel,PANEL_TEXT,
				  PANEL_LABEL_STRING,"Kill Mail From:",
				  PANEL_LABEL_WIDTH,font_size*12,
				  PANEL_VALUE_DISPLAY_LENGTH,50,
				  NULL);

	pgpkills_text = xv_create(priv2_panel,PANEL_TEXT,
				  PANEL_LABEL_STRING,"Kill Mail Subject:",
				  PANEL_LABEL_WIDTH,font_size*12,
				  PANEL_VALUE_DISPLAY_LENGTH,50,
				  NULL);

	/* Apply & Reset buttons: */
	xv_create(priv2_panel,PANEL_BUTTON,
		PANEL_LABEL_STRING, 	"Apply",
		PANEL_NOTIFY_PROC, 	save_priv2props,
		PANEL_NEXT_ROW,font_size*2,
		XV_X,			width/2-font_size*5,
		NULL);

	xv_create(priv2_panel,PANEL_BUTTON,
		PANEL_LABEL_STRING, 	"Reset",
		PANEL_NOTIFY_PROC, 	reset_priv2props,
		NULL);

	/* Load initial values: */
	load_priv2props();
}

static 	void	category_notify (item,event)

Panel_item	item;
Event		*event;

{
	switch ((int) xv_get(item,PANEL_VALUE)) {

		case 0:
			if (alias_panel == XV_NULL) render_alias_panel();
			xv_set(alias_panel,XV_SHOW,TRUE,NULL);
/*			window_fit_height(alias_panel);
			window_fit_height(panel);
			window_fit_height(props_frame); */
			break;
		case 1:
			if (file_panel == XV_NULL) render_file_panel();
			xv_set(file_panel,XV_SHOW,TRUE,NULL);
/* 			window_fit_height(file_panel);
			window_fit_height(panel);
			window_fit_height(props_frame); */
			break;
		case 2:
			if (privtool_panel == XV_NULL) render_privtool_panel();
			xv_set(privtool_panel,XV_SHOW,TRUE,NULL);
/*			window_fit_height(privtool_panel);
			window_fit_height(panel);
			window_fit_height(props_frame); */
			break;
		case 3:
		        if (priv2_panel == XV_NULL) render_priv2_panel();
			xv_set(priv2_panel,XV_SHOW,TRUE,NULL);
/*			window_fit_height(priv2_panel);
			window_fit_height(panel);
			window_fit_height(props_frame); */
			break;
	}

}

properties_proc()

{
	props_frame = (Frame) xv_create(main_frame,FRAME_CMD,
		FRAME_LABEL,"Privtool: Properties",
		FRAME_CMD_PIN_STATE,FRAME_CMD_PIN_IN,
		XV_SHOW, TRUE, 
		NULL);

	panel = (Panel ) xv_get(props_frame,FRAME_CMD_PANEL);
	
	xv_create(panel,PANEL_CHOICE_STACK,
		PANEL_LAYOUT,		PANEL_HORIZONTAL,
		PANEL_LABEL_STRING,	"Category:",
		PANEL_CHOICE_STRINGS,	
			"Alias","Mail Filing","Privtool","Privtool 2",NULL,
		PANEL_NOTIFY_PROC,	category_notify,
		NULL);

	file_panel  = XV_NULL;
	alias_panel = XV_NULL;
	privtool_panel = XV_NULL;
	priv2_panel = XV_NULL;
	
	render_alias_panel();
	window_fit_height(panel);
	window_fit_height(props_frame);
}



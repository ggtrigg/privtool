/* $Id$ 
 *
 * Copyright   : (c) 1997 by Glenn Trigg.  All Rights Reserved
 * Project     : Privtool - motif interface
 * File        : mfolder
 *
 * Author      : Glenn Trigg
 * Created     :  8 Jan 1997 
 *
 * Description : This file contains the code implementing the container
 *		 based mail folder handling.
 */

#include	<stdlib.h>
#include	<X11/X.h>
#include	<X11/Xlib.h>
#include	<X11/Intrinsic.h>
#include	<Xm/Container.h>
#include	<Xm/IconG.h>
#include	<dirent.h>
#include	<sys/stat.h>
#include	<ftw.h>
#include	<strings.h>
#include	<X11/Xmu/Editres.h>
#include	"mfolder.h"

static void		fillin_folders(Widget container);
static int		process_entry(char *file, struct stat *sb, int flag);
static Widget		findparent(Widget container, char *path);

static char		*fullfolder;
static Widget		folderwin = NULL, container;

/*----------------------------------------------------------------------*/

void
show_mail_folders(Widget parent)
{
    if(folderwin == NULL){
	folderwin = XtVaCreatePopupShell("folder",
					 topLevelShellWidgetClass,
					 parent,
					 NULL);
	XtManageChild(folderwin);

	/* Add editres protocol support */
	XtAddEventHandler(folderwin, 0, True, _XEditResCheckMessages, NULL);

	container = XmCreateContainer(folderwin, "folderC", NULL, 0);
	XtManageChild(container);

	fillin_folders(container);
    }
    XtPopup(folderwin, XtGrabNonexclusive);
} /* show_mail_folders */

/*----------------------------------------------------------------------*/

void
hide_mail_folders()
{
    printf("Hiding...\n");
    XtPopdown(folderwin);
} /* hide_mail_folders */

/*----------------------------------------------------------------------*/

static void
fillin_folders(Widget container)
{
    char		*folder;	/* Directory containing mail folders. */
    char		*homedir;
    DIR			*dirp;
    struct dirent	*de;
    Widget		icon;

    if ((folder = (char *)find_mailrc("folder"))){
	if(strchr(folder, '/') != NULL){ /* Is a specified path. */
	    fullfolder = strdup(folder);
	}else{			/* Relative to home directory. */
	    homedir = getenv("HOME");
	    fullfolder = XtMalloc(strlen(homedir) + strlen(folder) + 2);
	    strcpy(fullfolder, homedir);
	    strcat(fullfolder, "/");
	    strcat(fullfolder, folder);
	}

	ftw(fullfolder, process_entry, 10);
    }
    XtFree(fullfolder);
} /* fillin_folders */

/*----------------------------------------------------------------------*/

static int
process_entry(char *file, struct stat *sb, int flag)
{
    char		*basename, *relname, pathname[PATH_MAX];
    static Widget	dirwidget = NULL;
    Widget		icon;
    Arg			args[6];
    XmString		xname;

    if(!strcmp(fullfolder, file))
	return;

    relname = file + strlen(fullfolder) + 1;

    if((basename = strrchr(relname, '/')) == NULL){
	basename = relname;
	*pathname = '\0';
    }else{
	*basename = '\0';
	strcpy(pathname, relname);
	*basename = '/';
	basename++;
    }
    
    if(*pathname != '\0' && (dirwidget = findparent(container, pathname)))
	XtSetArg(args[0], XmNentryParent, dirwidget);

    xname = XmStringGenerate(basename, NULL, XmCHARSET_TEXT, NULL);
    XtSetArg(args[1], XmNlabelString, xname);
    icon = XmCreateIconGadget(container, (flag == FTW_D)? "directory": "file", args, 2);
    XtManageChild(icon);
    XmStringFree(xname);

    if(flag == FTW_D){
	dirwidget = icon;
    }

    return 0;
} /* process_entry */

/*----------------------------------------------------------------------*/

static Widget
findparent(Widget container, char *path)
{
    WidgetList	kids;
    int		numkids, i;
    char	*name;
    XmString	xpath = XmStringCreate(path, XmFONTLIST_DEFAULT_TAG);
    XmString	xname;

    XtVaGetValues(container, XmNnumChildren, &numkids,
		  XmNchildren, &kids, NULL);
    for(i = 0; i < numkids; i++){
	XtVaGetValues(kids[i], XmNlabelString, &xname, NULL);
	if(XmStringCompare(xname, xpath)){
	    XmStringFree(xpath);
	    return kids[i];
	}
    }

    XmStringFree(xpath);
    return NULL;
} /* findparent */

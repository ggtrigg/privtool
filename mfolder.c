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
#include	<Xm/MainW.h>
#include	<Xm/RowColumn.h>
#include	<Xm/CascadeB.h>
#include	<Xm/PushBG.h>
#include	<Xm/ScrolledW.h>
#include	<dirent.h>
#include	<sys/stat.h>
#include	<strings.h>
#include	<X11/Xmu/Editres.h>
#include	"mfolder.h"
#include	"def.h"
#include	"buffers.h"
#include	"message.h"
#include	"windows.h"
#include	"gui.h"

static void		fillin_folders(Widget container);
static void		process_dir(char *dirname);
static void		folderCB(Widget w, XtPointer clientdata, XtPointer calldata);
static void		createFolderMenubar(Widget parent);
static void		closeCB(Widget w, XtPointer clientdata, XtPointer calldata);

static char		*fullfolder;
static Widget		folderwin = NULL, container;

/*----------------------------------------------------------------------*/

void
show_mail_folders(Widget parent)
{
    Widget	mainwin;

    if(folderwin == NULL){
	folderwin = XtVaCreatePopupShell("folder",
					 topLevelShellWidgetClass,
					 parent,
					 NULL);
	XtManageChild(folderwin);

	/* Add editres protocol support */
	XtAddEventHandler(folderwin, 0, True, _XEditResCheckMessages, NULL);

	mainwin = XmCreateMainWindow(folderwin, "folderMain", NULL, 0);
	XtManageChild(mainwin);

	createFolderMenubar(mainwin);

	container = XmCreateContainer(mainwin, "folderC", NULL, 0);
	XtVaSetValues (mainwin, XmNworkWindow, container, NULL);
	XtManageChild(container);

	XtAddCallback(container, XmNdefaultActionCallback,
		      (XtCallbackProc) folderCB, NULL);

	fillin_folders(container);
    }
    XtPopup(folderwin, XtGrabNonexclusive);
} /* show_mail_folders */

/*----------------------------------------------------------------------*/

void
hide_mail_folders()
{
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

	process_dir(fullfolder);
    }
    XtFree(fullfolder);
} /* fillin_folders */

/*----------------------------------------------------------------------*/

static void
process_dir(char *dirname)
{
    char		*basename, *relname, pathname[PATH_MAX], *oldname;
    int			numkids, i;
    Widget		icon;
    WidgetList		kids;
    Arg			args[6];
    XmString		xname;
    DIR			*dirp;
    struct dirent	*de;
    struct stat		statbuf;

    if((dirp = opendir(dirname)) == NULL){
	perror(dirname);
	return;
    }

    /* Clean out any existing iconGadgets (except OutlineButton(s)) */
    XtVaGetValues(container, XmNchildren, &kids,
		  XmNnumChildren, &numkids, NULL);
    XtUnmanageChildren(kids, numkids);
    for(i = 0; i < numkids; i++){
	if(!strcmp(XtName(kids[i]), "OutlineButton"))
	    continue;
	XtDestroyWidget(kids[i]);
    }

    /* Now fill with new ones. */
    while((de = readdir(dirp)) != NULL){

	if(!strcmp(de->d_name, ".")) /* No need for this. */
	    continue;
	if(!(strcmp(de->d_name, "..") || strcmp(dirname, fullfolder)))
	    continue;		/* No '..' if already at top of folder dir. */

	xname = XmStringGenerate(de->d_name, NULL, XmCHARSET_TEXT, NULL);
	XtSetArg(args[0], XmNlabelString, xname);

	strcpy(pathname, dirname);
	strcat(pathname, "/");
	strcat(pathname, de->d_name);
	stat(pathname, &statbuf);
	XtSetArg(args[1], XmNuserData, (XtPointer)S_ISDIR(statbuf.st_mode));

	icon = XmCreateIconGadget(container,
				  (S_ISDIR(statbuf.st_mode))? "directory": "file",
				  args, 2);
	XtManageChild(icon);
	XmStringFree(xname);
    }
    closedir(dirp);

    XtVaGetValues(container, XmNuserData, &oldname, NULL);
    if(oldname != NULL){
	XtFree(oldname);
    }
    oldname = XtMalloc(strlen(dirname));
    strcpy(oldname, dirname);
    XtVaSetValues(container, XmNuserData, oldname, NULL);

    /* Do a re-layout. ??? */
    /*XmContainerRelayout(container);*/
} /* process_dir */

/*----------------------------------------------------------------------*/

static void
folderCB(Widget w, XtPointer clientdata, XtPointer calldata)
{
    Widget	selected;
    WidgetList	selobjs;
    int		numobjs, is_dir;
    XmString	xname;
    char	*name, *dir, *fullpath;

    XtVaGetValues(container, XmNselectedObjects, &selobjs,
		  XmNselectedObjectCount, &numobjs, NULL);
    if(numobjs < 1)
	return;

    XtVaGetValues(selobjs[0], XmNlabelString, &xname, NULL);
    XtVaGetValues(container, XmNuserData, &dir, NULL);

    XmStringGetLtoR(xname, XmFONTLIST_DEFAULT_TAG, &name);
    fullpath = XtMalloc(strlen(dir) + strlen(name) + 2);
    strcpy(fullpath, dir);
    strcat(fullpath, "/");
    strcat(fullpath, name);

    if(!strcmp(XtName(selobjs[0]), "file")){ /* It's a file (folder). */
	deleteAllMessages();
	load_file_proc(fullpath);
	display_message(last_message_read = messages.start);
	sync_list();
	update_message_list();
    }else{			/* It's a directory. */
	process_dir(fullpath);
    }

    /* Cleanup time. */
    XtFree(fullpath);
    XmStringFree(xname);
    XtFree(name);
} /* folderCB */

/*----------------------------------------------------------------------*/

static void
createFolderMenubar(Widget parent)
{
    Widget	menubar_, menu_, cascade_, button_;
    Arg		args[3];

    menubar_ = XmCreateMenuBar (parent, "folder_menu_bar", NULL, 0);
    XtManageChild (menubar_);
    XtVaSetValues (parent, XmNmenuBar, menubar_, NULL);

    menu_ = XmCreatePulldownMenu (menubar_, "file_pane", NULL, 0);
    XtSetArg (args[0], XmNsubMenuId, menu_);
    cascade_ = XmCreateCascadeButton (menubar_, "file", args, 1);
    XtManageChild (cascade_);

    button_ = XmCreatePushButtonGadget (menu_, "close", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		   closeCB, NULL);
} /* createFolderMenubar */

/*----------------------------------------------------------------------*/

static void
closeCB(Widget w, XtPointer clientdata, XtPointer calldata)
{
    hide_mail_folders();
} /* closeCB */

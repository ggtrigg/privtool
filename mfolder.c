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
#include	<stdio.h>
#include	<X11/X.h>
#include	<X11/Xlib.h>
#include	<X11/Intrinsic.h>
#include	<Xm/AtomMgr.h>
#include	<Xm/CascadeB.h>
#include	<Xm/Container.h>
#include	<Xm/IconG.h>
#include	<Xm/MainW.h>
#include	<Xm/Protocols.h>
#include	<Xm/PushBG.h>
#include	<Xm/RowColumn.h>
#include	<Xm/ScrolledW.h>
#include	<Xm/TransferP.h>
#include	<dirent.h>
#include	<sys/stat.h>
#include	<sys/param.h>
#include	<string.h>
#include	<X11/Xmu/Editres.h>
#include	"mfolder.h"
#include	"def.h"
#include	"buffers.h"
#include	"message.h"
#include	"windows.h"
#include	"gui.h"
#include	"debug.h"

static void		fillin_folders(Widget);
static void		process_dir(char *);
static void		folderCb(Widget, XtPointer, XtPointer);
static void		iconSelectCb(Widget, XtPointer, XtPointer);
static void		resizeCb(Widget, XtPointer, XEvent *, Boolean *);
static void		createFolderMenubar(Widget);
static void		closeCb(Widget, XtPointer, XtPointer);
static void		destinationCb(Widget, XtPointer, XtPointer);
static void		transferProc(Widget, XtPointer, XtPointer);
static void		fix_container_size(Widget);
static void		containerConvertCb(Widget, XtPointer, XtPointer);
static void		cDragProc(Widget, XtPointer, XtPointer);

static char		*fullfolder;
static Widget		folderwin = NULL, container;

/*----------------------------------------------------------------------*/

void
show_mail_folders(Widget parent)
{
    Widget	mainwin;
    Arg		args[8];
    int		i;

    if(folderwin == NULL){
	folderwin = XtVaCreatePopupShell("folder",
					 topLevelShellWidgetClass,
					 parent,
					 NULL);
	XtManageChild(folderwin);

	/* Window manager quit support */
	XmAddWMProtocolCallback(folderwin,
				XmInternAtom(XtDisplay(folderwin),
					     "WM_DELETE_WINDOW", False),
				closeCb, NULL);

	/* Add editres protocol support */
	XtAddEventHandler(folderwin, 0, True, _XEditResCheckMessages, NULL);

	mainwin = XmCreateMainWindow(folderwin, "folderMain", NULL, 0);
	XtManageChild(mainwin);

	createFolderMenubar(mainwin);

	container = XmCreateContainer(mainwin, "folderC", NULL, 0);
	XtVaSetValues (mainwin, XmNworkWindow, container, NULL);
	XtManageChild(container);

	XtAddCallback(container, XmNdefaultActionCallback,
		      (XtCallbackProc) folderCb, NULL);
	XtAddCallback(container, XmNselectionCallback,
		      (XtCallbackProc) iconSelectCb, NULL);
	XtAddCallback(container, XmNdestinationCallback,
		      (XtCallbackProc) destinationCb, NULL);
	XtAddCallback(container, XmNconvertCallback,
		      (XtCallbackProc) containerConvertCb, NULL);
	XtAddEventHandler(mainwin, StructureNotifyMask, False,
			  resizeCb, container);

	i = 0;
	XtSetArg(args[i], XmNdragProc, cDragProc); i++;
	XmDropSiteUpdate(container, args, i);

	fillin_folders(container);
    }
    set_foldwin_toggles(True);
    XtPopup(folderwin, XtGrabNonexclusive);
} /* show_mail_folders */

/*----------------------------------------------------------------------*/

void
hide_mail_folders()
{
    set_foldwin_toggles(False);
    XtPopdown(folderwin);
} /* hide_mail_folders */

/*----------------------------------------------------------------------*/

static void
fillin_folders(Widget container)
{
    char		*folder;	/* Directory containing mail folders. */
    char		*homedir;

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
} /* fillin_folders */

/*----------------------------------------------------------------------*/

static void
process_dir(char *dirname)
{
    char		pathname[MAXPATHLEN], *oldname;
    int			numkids, i;
    Widget		icon;
    WidgetList		kids;
    Arg			args[16];
    XmString		xname;
    DIR			*dirp;
    struct dirent	*de;
    struct stat		statbuf;
    /*Pixmap		pixmap;*/

    if((dirp = opendir(dirname)) == NULL){
	perror(dirname);
	return;
    }

    /* Clean out any existing iconGadgets (except OutlineButton(s)) */
    XtVaGetValues(container, XmNchildren, &kids,
		  XmNnumChildren, &numkids, NULL);
    XtUnmanageChildren(kids, numkids);
    for(i = 0; i < numkids; i++){
	if(XtClass(kids[i]) == xmIconGadgetClass)
	    XtDestroyWidget(kids[i]);
    }

    /* Now fill with new ones. */
    while((de = readdir(dirp)) != NULL){

	if(!strcmp(de->d_name, ".")) /* No need for this. */
	    continue;
	if(!(strcmp(de->d_name, "..") || strcmp(dirname, fullfolder)))
	    continue;		/* No '..' if already at top of folder dir. */

	xname = XmStringGenerate(de->d_name, NULL, XmCHARSET_TEXT, NULL);
	i = 0;
	XtSetArg(args[i], XmNlabelString, xname); i++;

	strcpy(pathname, dirname);
	strcat(pathname, "/");
	strcat(pathname, de->d_name);
	stat(pathname, &statbuf);
	XtSetArg(args[i], XmNuserData, (XtPointer)S_ISDIR(statbuf.st_mode)); i++;

	icon = XmCreateIconGadget(container,
				  (S_ISDIR(statbuf.st_mode))? "directory": "file",
				  args, 2);

#if 0
	if((pixmap = get_cached_pixmap(icon,
				       GetResourceString(icon,
							 "largeIconPixmap",
							 "LargeIconPixmap")
				       )) != 0){
	    XtVaSetValues(icon, XmNlargeIconPixmap, pixmap, NULL);
	}
#endif
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

    /* Do a re-layout. */
    fix_container_size(container);
} /* process_dir */

/*----------------------------------------------------------------------*/

static void
folderCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    XmContainerSelectCallbackStruct *cbs = (XmContainerSelectCallbackStruct *)calldata;
    XmString	xname;
    char	*name, *dir, *c, *fullpath;

    if(cbs->selected_item_count < 1)
	return;

    XtVaGetValues(cbs->selected_items[0], XmNlabelString, &xname, NULL);
    XtVaGetValues(container, XmNuserData, &dir, NULL);

    XmStringGetLtoR(xname, XmFONTLIST_DEFAULT_TAG, &name);
    fullpath = XtMalloc(strlen(dir) + strlen(name) + 2);
    if(!strcmp(name, "..")){
	if((c = strrchr(dir, '/')) != NULL){
	    *c = '\0';
	}
	else{
	    *dir = '\0';
	}
	strcpy(fullpath, dir);
    }
    else{
	strcpy(fullpath, dir);
	strcat(fullpath, "/");
	strcat(fullpath, name);
    }

    if(!strcmp(XtName(cbs->selected_items[0]), "file")){ /* It's a file (folder). */
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
} /* folderCb */

/*----------------------------------------------------------------------*/

void
iconSelectCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    XmContainerSelectCallbackStruct *cbs = (XmContainerSelectCallbackStruct *)calldata;
    XmString	xname;
    char	*name, *dir, abbrevdir[MAXPATHLEN];

    if(cbs->auto_selection_type == XmAUTO_NO_CHANGE){
	if(cbs->selected_item_count == 1 &&
	   !strcmp(XtName(cbs->selected_items[0]), "file")){
	    XtVaGetValues(cbs->selected_items[0], XmNlabelString, &xname, NULL);
	    XmStringGetLtoR(xname, XmFONTLIST_DEFAULT_TAG, &name);
	    XmStringFree(xname);
	    XtVaGetValues(container, XmNuserData, &dir, NULL);
	    if(!strncmp(dir, fullfolder, strlen(fullfolder))){
		strcpy(abbrevdir, "+");
		strcat(abbrevdir, dir + strlen(fullfolder));
	    }
	    else{
		strcpy(abbrevdir, dir);
	    }
	    strcat(abbrevdir, "/");
	    strcat(abbrevdir, name);
	    update_combo(abbrevdir);
	    XtFree(name);
	}
    }
} /* iconSelectCb */

/*----------------------------------------------------------------------*/

void
resizeCb(Widget w, XtPointer clientdata, XEvent *event, Boolean *cont)
{
    Widget	container = (Widget)clientdata;

    if(event->type == ConfigureNotify || event->type == MapNotify){
	fix_container_size(container);
    }
    *cont = True;
} /* resizeCb */

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
		   closeCb, NULL);
} /* createFolderMenubar */

/*----------------------------------------------------------------------*/

static void
closeCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    hide_mail_folders();
} /* closeCb */

/*----------------------------------------------------------------------*/

static void
fix_container_size(Widget container)
{
    Widget		swin, hscroll;
    XtWidgetGeometry	curr;
    WidgetList		cwids;
    int			numcwids, i;
    Dimension		width, height, shadowThickness, spacing, hsheight;
    Position		x, y;

    swin = XtParent(XtParent(container));
    hscroll = XtNameToWidget(swin, ".HorScrollBar");
    XtVaGetValues(hscroll, XmNheight, &hsheight, NULL);
    XtVaGetValues(swin, XmNspacing, &spacing, XmNshadowThickness,
		  &shadowThickness, NULL);
    XtQueryGeometry(swin, NULL, &curr);
    curr.width -= 2 * shadowThickness;
    curr.height -= 2 * shadowThickness;
    XtVaSetValues(container, XmNwidth, curr.width,
		  XmNheight, curr.height, NULL);

    XtVaGetValues(container, XmNchildren, &cwids,
		  XmNnumChildren, &numcwids, NULL);

    /* Find out if the window is too small to hold all icons. */
    for(i = 0; i < numcwids; i++){
	/* Skip non IconGadget's */
	if(XtClass(cwids[i]) != xmIconGadgetClass)
	    continue;
	XtVaGetValues(cwids[i], XmNx, &x, XmNy, &y,
		      XmNwidth, &width, XmNheight, &height, NULL);
	if((x + width) <= 0 || (y+height) <= 0){
	    break;
	}
    }
    if(i < numcwids){
	curr.height -= ((2 * spacing) + hsheight);
	XtVaSetValues(container, XmNwidth, curr.width + 200,
		      XmNheight, curr.height, NULL);
    }
    /*XmContainerRelayout(container);*/
} /* fix_container_size */

/*----------------------------------------------------------------------*/

static void
destinationCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    XmDestinationCallbackStruct *cbs = (XmDestinationCallbackStruct *)calldata;
    Display	*display = XtDisplay(w);
    Atom	TARGETS = XmInternAtom(display, XmSTARGETS, False);
    Atom	MOTIF_DROP = XmInternAtom(display, XmS_MOTIF_DROP, False);

    DEBUG1(("destinationCb: event = 0x%x\n", cbs->event));
    DEBUG2(("  selection = %s\n",
	    XmGetAtomName(XtDisplay(w), cbs->selection)));
    DEBUG2(("  location_data = 0x%x (%s)\n",
	    cbs->location_data, XtName(cbs->location_data)));

    /* Only process DnD transfers. */
    if(cbs->selection != MOTIF_DROP)
	return;

    XmTransferValue(cbs->transfer_id, TARGETS, (XtCallbackProc) transferProc,
		    NULL, XtLastTimestampProcessed(XtDisplay(w)));
} /* destinationCb */

/*----------------------------------------------------------------------*/

static void
transferProc(Widget w, XtPointer clientdata, XtPointer calldata)
{
    XmSelectionCallbackStruct *cbs = (XmSelectionCallbackStruct *)calldata;
    Display	*display = XtDisplay(w);
    int		i;
    Atom	TARGETS = XmInternAtom(display, XmSTARGETS, False);
    Atom	_MOTIF_DRAG_OFFSET = XmInternAtom(display,
						  XmS_MOTIF_DRAG_OFFSET,
						  False);

    DEBUG1(("transferProc: event = 0x%x\n", cbs->event));
    DEBUG2(("  selection = %s\n",
	    XmGetAtomName(XtDisplay(w), cbs->selection)));

    if(cbs->target == TARGETS && (cbs->type == XA_ATOM)){
	Atom	*targets = (Atom *)cbs->value;

	for(i = 0; i < cbs->length; i++){
	    if(targets[i] == _MOTIF_DRAG_OFFSET){
		XmTransferValue(cbs->transfer_id, _MOTIF_DRAG_OFFSET,
				(XtCallbackProc)transferProc, NULL,
				XtLastTimestampProcessed(display));
	    }
	}
    }
    else if(cbs->target == _MOTIF_DRAG_OFFSET){
	XmTransferDone(cbs->transfer_id, XmTRANSFER_DONE_DEFAULT);
    }
    else{
	XmTransferDone(cbs->transfer_id, XmTRANSFER_DONE_FAIL);
    }
} /* transferProc */

/*----------------------------------------------------------------------*/

static void
containerConvertCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    Display	*display = XtDisplay(w);
    XmConvertCallbackStruct *cbs = (XmConvertCallbackStruct *)calldata;

    Atom	PRIVTOOL_FOLDERDIR =
	XmInternAtom(display, "PRIVTOOL_FOLDERDIR", False);
    Atom	TARGETS = XmInternAtom(display, XmSTARGETS, False);
    Atom	ME_TARGETS = 
	XmInternAtom(display, XmS_MOTIF_EXPORT_TARGETS, False);
    Atom	MOTIF_DRAG_OFFSET = 
	XmInternAtom(display, XmS_MOTIF_DRAG_OFFSET, False);

    DEBUG1(("containerConvertCb target = %s\n", XmGetAtomName(display,
							 cbs->target)));

    if(cbs->target == TARGETS ||
       cbs->target == ME_TARGETS){

        Atom *targs;
        int target_count = 0;

	targs = XmeStandardTargets(w, 2, &target_count);
        targs[target_count++] = PRIVTOOL_FOLDERDIR;
        targs[target_count++] = MOTIF_DRAG_OFFSET;

        cbs->value = targs;
	cbs->length = target_count;
	cbs->format = 32;
	cbs->type = XA_ATOM;
	cbs->status = XmCONVERT_DONE;
    }
    else if(cbs->target == PRIVTOOL_FOLDERDIR){
	DEBUG2(("   converting a folderdir...\n"));
#if 0
	if(cbs->location_data != 0){ /* This is the list item to transfer */
	    for(m = messages.start; m != NULL; m = m->next){
		if(m->list_pos == (int)cbs->location_data + 1)
		    break;
	    }
	}
	else{			/* Do all selected items (somehow). */
	    for(m = messages.start; m != NULL; m = m->next){
		if(XmListPosSelected(mailslist_, m->list_pos)){
		    break;
		}
	    }
	}
	if(m != NULL){
	    if(m->decrypted){
		b = m->decrypted;
	    }
	    else{
		b = message_contents(m);
	    }
	    cbs->value = b->message;
	    cbs->length = strlen(b->message);
	    cbs->type = XA_STRING;
	    cbs->format = 8;
	    cbs->status = XmCONVERT_DONE;
	}
#endif
	cbs->status = XmCONVERT_DONE;
    }
    else{
	cbs->status = XmCONVERT_DEFAULT;
    }
}

/*----------------------------------------------------------------------*/

static void
cDragProc(Widget w, XtPointer clientdata, XtPointer calldata)
{
    XmDragProcCallbackStruct	*cbs = (XmDragProcCallbackStruct *)calldata;
    Widget	cwid;

    DEBUG3(("cDragProc\n"));

    /* This is only for _MOTIF_DRAG_OFFSET at the moment. Need to check
       the dragContext (cbs->dragContext) to see what operation we're
       (likely) doing. */
    if((cwid = XmObjectAtPoint(w, cbs->x, cbs->y)) != NULL){
	if(!strcmp(XtName(cwid), "file")){
	    cbs->dropSiteStatus = XmDROP_SITE_INVALID;
	}
	else{
	    XtVaSetValues(cwid, XmNvisualEmphasis, XmSELECTED, NULL);
	    cbs->dropSiteStatus = XmDROP_SITE_VALID;
	}
    }
    else{
	cbs->dropSiteStatus = XmDROP_SITE_VALID;
    }
} /* cDragProc */

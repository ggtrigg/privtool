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
#include	<Xm/CascadeB.h>
#include	<Xm/Container.h>
#include	<Xm/ContainerP.h>
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
static void		process_dir(Widget, char *, Widget);
static void		sort_container(Widget);
static int		widcomp(Widget, Widget);
static void		folderCb(Widget, XtPointer, XtPointer);
static void		iconSelectCb(Widget, XtPointer, XtPointer);
static void		resizeCb(Widget, XtPointer, XEvent *, Boolean *);
static void		createFolderMenubar(Widget);
static void		closeCb(Widget, XtPointer, XtPointer);
static void		fdeleteCb(Widget, XtPointer, XtPointer);
static void		destinationCb(Widget, XtPointer, XtPointer);
static void		transferProc(Widget, XtPointer, XtPointer);
static void		fix_container_size(Widget);
static void		containerConvertCb(Widget, XtPointer, XtPointer);
static void		cDragProc(Widget, XtPointer, XtPointer);
static char		*full_name(Widget);

static char		*fullfolder;
static Widget		folderwin = NULL, container;

/*----------------------------------------------------------------------*/

void
show_mail_folders(Widget parent)
{
    Arg		args[5];
    Widget	mainwin;
    int		i;

    if(folderwin == NULL){
	folderwin = XtVaCreatePopupShell("folder",
					 topLevelShellWidgetClass,
					 parent,
					 XmNdeleteResponse, XmUNMAP,
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

	i = 0;
	XtSetArg(args[i], XmNselectionPolicy, XmBROWSE_SELECT); i++;
	container = XmCreateContainer(mainwin, "folderC", args, i);
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
#if 0
	XtAddEventHandler(mainwin, StructureNotifyMask, False,
			  resizeCb, container);
#endif

	i = 0;
#if 0
	XtSetArg(args[i], XmNdragProc, cDragProc); i++;
	XmDropSiteUpdate(container, args, i);
#endif

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

	process_dir(container, fullfolder, NULL);
	/*sort_container(container);*/
    }
} /* fillin_folders */

/*----------------------------------------------------------------------*/

/* Fill the container with icon gadgets, representing the files and
   directories in the given starting directory. This descends into
   subdirectories recursively until the entire sub-tree is covered.
 */
static void
process_dir(Widget container, char *dirname, Widget entry_parent)
{
    char		pathname[MAXPATHLEN];
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

    /* Clean out any existing iconGadgets (except OutlineButton(s))
       (but only if we're at the top i.e entry_parent == NULL) */
    if(entry_parent == NULL) {
	XtVaGetValues(container, XmNchildren, &kids,
		      XmNnumChildren, &numkids, NULL);
	XtUnmanageChildren(kids, numkids);
	for(i = 0; i < numkids; i++){
	    if(XmIsIconGadget(kids[i]))
		XtDestroyWidget(kids[i]);
	}
    }

    /* Now fill with new ones. */
    while((de = readdir(dirp)) != NULL){

	if(!strcmp(de->d_name, ".")) /* No need for this. */
	    continue;
	if(!strcmp(de->d_name, ".."))
	    continue;		/* No '..'. */

	xname = XmStringGenerate(de->d_name, NULL, XmCHARSET_TEXT, NULL);
	i = 0;
	XtSetArg(args[i], XmNlabelString, xname); i++;

	strcpy(pathname, dirname);
	strcat(pathname, "/");
	strcat(pathname, de->d_name);
	stat(pathname, &statbuf);
	XtSetArg(args[i], XmNuserData, (XtPointer)S_ISDIR(statbuf.st_mode)); i++;
	XtSetArg(args[i], XmNentryParent, entry_parent); i++;

	icon = XmCreateIconGadget(container,
				  (S_ISDIR(statbuf.st_mode))? "directory": "file",
				  args, i);

	XtManageChild(icon);
	XmStringFree(xname);
	if(S_ISDIR(statbuf.st_mode)) { /* recurse into sub-directory */
	    process_dir(container, pathname, icon);
	}
    }
    closedir(dirp);
} /* process_dir */

/*----------------------------------------------------------------------*/

static void
sort_container(Widget container)
{
    int		i, numcwids;
    WidgetList	items;

    XtVaGetValues(container, XmNchildren, &items,
		  XmNnumChildren, &numcwids, NULL);
    qsort(items, numcwids, sizeof(XmContainerRec), widcomp);
    for(i = 0; i < numcwids; i++) {
	XtVaSetValues(items[i], XmNpositionIndex, i, NULL);
    }
    XmContainerReorder(container, items, numcwids);
} /* sort_container */

/*----------------------------------------------------------------------*/

static int
widcomp(Widget a, Widget b)
{
    char	*namea, *nameb;
    int		val;
    XmString	xname;

    if( ! XmIsIconGadget(a))
	return -1;

    XtVaGetValues(a, XmNlabelString, &xname, NULL);
    XmStringGetLtoR(xname, XmFONTLIST_DEFAULT_TAG, &namea);
    XmStringFree(xname);
    XtVaGetValues(b, XmNlabelString, &xname, NULL);
    XmStringGetLtoR(xname, XmFONTLIST_DEFAULT_TAG, &nameb);
    XmStringFree(xname);

    val = strcmp(namea, nameb);
    XtFree(namea);
    XtFree(nameb);

    return val;
} /* widcomp */

/*----------------------------------------------------------------------*/

static void
folderCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    XmContainerSelectCallbackStruct *cbs =
	(XmContainerSelectCallbackStruct *)calldata;
    unsigned char	ostate;
    char		*name, *fullpath;

    if(cbs->selected_item_count < 1)
	return;


    if(!strcmp(XtName(cbs->selected_items[0]), "file")){ /* It's a file (folder). */
	name = full_name(cbs->selected_items[0]);
	fullpath = XtMalloc(strlen(fullfolder) + strlen(name) + 2);
	strcpy(fullpath, "+/");
	strcat(fullpath, name);
	add_to_combo(fullpath);

	strcpy(fullpath, fullfolder);
	strcat(fullpath, "/");
	strcat(fullpath, name);
	XtFree(name);

	deleteAllMessages();
	load_file_proc(fullpath);
	display_new_message();
	sync_list();
	update_message_list();

	/* Cleanup time. */
	XtFree(fullpath);
    }else{			/* It's a directory. */
	XtVaGetValues(cbs->selected_items[0],
		      XmNoutlineState, &ostate, NULL);
	if(ostate == XmCOLLAPSED) {
	    XtVaSetValues(cbs->selected_items[0],
			  XmNoutlineState, XmEXPANDED, NULL);
	}
	else {
	    XtVaSetValues(cbs->selected_items[0],
			  XmNoutlineState, XmCOLLAPSED, NULL);
	}
    }

} /* folderCb */

/*----------------------------------------------------------------------*/

void
iconSelectCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    XmContainerSelectCallbackStruct *cbs =
	(XmContainerSelectCallbackStruct *)calldata;
    char	*name, *abbrevdir;

    if(cbs->auto_selection_type == XmAUTO_NO_CHANGE){
	if(cbs->selected_item_count == 1 &&
	   !strcmp(XtName(cbs->selected_items[0]), "file")){
	    name = full_name(cbs->selected_items[0]);

	    abbrevdir = XtMalloc(strlen(name) + 3);
	    strcpy(abbrevdir, "+/");
	    strcat(abbrevdir, name);
	    update_combo(abbrevdir);
	    XtFree(name);
	    XtFree(abbrevdir);
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

    menu_ = XmCreatePulldownMenu (menubar_, "edit_pane", NULL, 0);
    XtSetArg (args[0], XmNsubMenuId, menu_);
    cascade_ = XmCreateCascadeButton (menubar_, "edit", args, 1);
    XtManageChild (cascade_);

    button_ = XmCreatePushButtonGadget (menu_, "newdir", NULL, 0);
    XtManageChild (button_);
    button_ = XmCreatePushButtonGadget (menu_, "delete", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		   fdeleteCb, NULL);
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

    DEBUG1(("destinationCb: event = 0x%x\n", cbs->event));
    DEBUG2(("  selection = %s\n",
	    XmGetAtomName(XtDisplay(w), cbs->selection)));
    DEBUG2(("  location_data = 0x%x (%s)\n",
	    cbs->location_data, XtName(cbs->location_data)));

    XmTransferDone(cbs->transfer_id, XmTRANSFER_DONE_SUCCEED);
#if 0
    /* Only process DnD transfers. */
    if(cbs->selection != MOTIF_DROP)
	return;

    XmTransferValue(cbs->transfer_id, TARGETS, (XtCallbackProc) transferProc,
		    NULL, XtLastTimestampProcessed(XtDisplay(w)));
#endif
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

/*----------------------------------------------------------------------*/

static char *
full_name(Widget ig)
{
    int		len;
    char	*name, *fullname = NULL;
    XmString	xname;
    Widget	ep, curr;

    if( ! XmIsIconGadget(ig))
	return NULL;

    curr = ig;
    do{
	XtVaGetValues(curr, XmNlabelString, &xname,
		      XmNentryParent, &ep,
		      NULL);
	XmStringGetLtoR(xname, XmFONTLIST_DEFAULT_TAG, &name);
	XmStringFree(xname);
	if( fullname == NULL) {
	    fullname = name;
	}
	else {
	    len = strlen(name);
	    fullname = XtRealloc(fullname, strlen(fullname) + len + 2);
	    memmove(fullname + len + 1, fullname, strlen(fullname) + 1);
	    strcpy(fullname, name);
	    fullname[len] = '/';
	    XtFree(name);
	}
	curr = ep;
    } while( ep != NULL);

    return fullname;
} /* full_name */

/*----------------------------------------------------------------------*/

void
check_folder_icon(char *folder)
{
    int		numcwids, i;
    char	*ftemp, *c, *name, *basename;
    Widget	pwid, icon;
    WidgetList	cwids;
    XmString	xname;
    Arg		args[6];

    if(folderwin == NULL || folder == NULL || (strlen(folder) < 3) ||
       *folder != '+' || folder[1] != '/')
	return;

    ftemp = strdup(&folder[2]);	/* Skip the initial "+/". */

    /* Get basename of folder to ensure all intermediate directories
       are present. */
    if((basename = strrchr(ftemp, '/')) == NULL) {
	basename = ftemp;
    }
    else {
	basename++;
    }

    /* Split ftemp into path components, checking each component for
       existance of an associated IconGadget. */
    if((c = strtok(ftemp, "/")) == NULL)
	return;

    pwid = NULL;
    do {
	numcwids = XmContainerGetItemChildren(container, pwid, &cwids);
	for(i = 0; i < numcwids; i++) {
	    XtVaGetValues(cwids[i], XmNlabelString, &xname, NULL);
	    XmStringGetLtoR(xname, XmFONTLIST_DEFAULT_TAG, &name);
	    if(strcmp(name, c) == 0){
		pwid = cwids[i];
		XtFree(name);
		break;
	    }
	    XtFree(name);
	}
	XtFree(cwids);
	if(i >= numcwids) {	/* Haven't already got this component. */
	    Boolean isdir = (strcmp(c, basename) != 0);
	    xname = XmStringGenerate(c, NULL, XmCHARSET_TEXT, NULL);
	    i = 0;
	    XtSetArg(args[i], XmNlabelString, xname); i++;

	    /* TODO: How to know if this is a directory or a file? */
	    XtSetArg(args[i], XmNuserData, (XtPointer)isdir); i++;
	    XtSetArg(args[i], XmNentryParent, pwid); i++;

	    icon = XmCreateIconGadget(container, isdir? "directory": "file",
				      args, i);

	    XtManageChild(icon);
	    XmStringFree(xname);
	}
    }while((c = strtok(NULL, "/")) != NULL);
} /* check_folder_icon */

/*----------------------------------------------------------------------*/

static void
fdeleteCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    int		numitems;
    WidgetList	selwids;
    char	*fullpath, *c;

    XtVaGetValues(container, XmNselectedObjectCount, &numitems,
		  XmNselectedObjects, &selwids, NULL);

    if(numitems < 1)
	return;

    if(strcmp(XtName(selwids[0]), "file") == 0) {
	if(alert(folderwin, "reallydel", 2) != XmCR_OK)
	    return;

	c = full_name(selwids[0]);
	fullpath = XtMalloc(strlen(fullfolder) + strlen(c) + 2);
	strcpy(fullpath, fullfolder);
	strcat(fullpath, "/");
	strcat(fullpath, c);
	XtFree(c);

	unlink(fullpath);
	XtVaSetValues(container, XmNselectedObjectCount, 0, NULL);
	XtDestroyWidget(selwids[0]);
    }
    else {
	alert(folderwin, "nodeldir", 1);
    }
} /* fdeleteCb */

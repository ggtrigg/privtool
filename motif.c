/*
 *	@(#)motif.c	1.0  13/10/96
 *	$Id$
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
 *      Motif2 implementation of GUI by
 *      	- Glenn Trigg (ggt@netspace.net.au) 13 October 1996
 */

/* We define UI_MAIN so that header files can tell how to define the
   neccesary structures */

#define UI_MAIN

#include	<malloc.h>
#include	<X11/X.h>
#include	<X11/Xlib.h>
#include	<X11/Intrinsic.h>
#include	"def.h"
#include	"buffers.h"
#include	"message.h"
#include	"mailrc.h"
#include	"windows.h"
#include	"gui.h"
#include	"motif_protos.h"
#include	<Xm/AtomMgr.h>
#include	<Xm/CSText.h>
#include	<Xm/CascadeB.h>
#include	<Xm/ComboBox.h>
#include	<Xm/ContainerT.h>
#include	<Xm/DialogS.h>
#include	<Xm/Form.h>
#include	<Xm/FileSB.h>
#include	<Xm/Frame.h>
#include	<Xm/LabelG.h>
#include	<Xm/List.h>
#include	<Xm/MainW.h>
#include	<Xm/MessageB.h>
#include	<Xm/PanedW.h>
#include	<Xm/PushB.h>
#include	<Xm/PushBG.h>
#include	<Xm/RowColumn.h>
#include	<Xm/ScrolledW.h>
#include	<Xm/SeparatoG.h>
#include	<Xm/Separator.h>
#include	<Xm/Text.h>
#include	<Xm/TextF.h>
#include	<Xm/ToggleB.h>
#include	<Xm/ToggleBG.h>
#include	<Xm/TraitP.h>
#include	<Xm/TransferP.h>
#include	<X11/Xmu/Editres.h>
#include	<Xbae/Caption.h>
#include	"mfolder.h"
#include	"LiteClue.h"
#include	"pixmapcache.h"
#include	"m_util.h"
#include	"debug.h"
#include	<sys/stat.h>
#include	<fcntl.h>
#include	<sys/mman.h>

#include	"prev.xpm"
#include	"next.xpm"
#include	"delete.xpm"
#include	"undelete.xpm"
#include	"folderwin.xpm"

void			update_message_list(void);
void			set_foldwin_toggles(Boolean);
void			update_combo(char *);
int			show_deleted = 1;
int			debug_;

static Widget		toplevel_, msgarea_, msgarea2_, mailslist_;
static Widget		text_ = NULL, hdrtext_ = NULL, folder_toggle_;
static Widget		liteClue_, pp_window_ = NULL, abt_window_ = NULL;
static Widget		foldwin_[2], fold_combo_;
static XtAppContext	app_context_;
/*static XmRenderTable	render_header, render_list;*/
static char		local_pp[2048];
static COMPOSE_WINDOW	*compose_first = NULL;
static COMPOSE_WINDOW	*compose_last = NULL;
static char		attribution_string[] = " said :\n\n";
static char		begin_forward[] = "-- Begin forwarded message ---\n";
static char		end_forward[] = "\n-- End forwarded message ---\n";

static XrmOptionDescRec	options[] = {
    { "-debug", "debug", XrmoptionSepArg, NULL }
};

typedef enum {
    COMPOSE_NEW,
    COMPOSE_REPLY,
    COMPOSE_FORWARD,
    COMPOSE_RESEND
} COMPOSE_TYPE;

typedef enum {
    FOLDER_COPY,
    FOLDER_MOVE,
    FOLDER_LOAD
} FOLDER_ACTIONS;

static void		create_menubar(Widget parent);
static void		create_toolbar(Widget parent);
static void		create_file_menu(Widget parent);
static void		create_edit_menu(Widget parent);
static void		create_view_menu(Widget parent);
static void		create_fold_menu(Widget parent);
static void		create_comp_menu(Widget parent);
static void		create_help_menu(Widget parent);
static void		create_workarea (Widget parent);
static void		create_msgareas(Widget parent);
static COMPOSE_WINDOW	*setup_composeCB(Widget w, XtPointer clientdata, XtPointer calldata);
static COMPOSE_WINDOW	*compose_find_free();
static void		update_mail_list(void);
static void		show_message(Widget w, XtPointer, XmListCallbackStruct *cbs);
static void		listConvertCb(Widget w, XtPointer, XtPointer);
static void		textConvertCb(Widget w, XtPointer, XtPointer);
static void		destnCb(Widget w, XtPointer, XtPointer);
static void		transferProc(Widget w, XtPointer, XtPointer);
static void		select_message(Widget w, XtPointer, XmListCallbackStruct *cbs);
static void		next_messageCB(Widget w, XtPointer clientdata, XtPointer calldata);
static void		prev_messageCB(Widget w, XtPointer clientdata, XtPointer calldata);
static int		xioerror_handler(Display *dpy);
static void		passphrase_cb(Widget w, XtPointer clientdata, XmTextVerifyPtr tb);
static void		aboutCB(Widget w, XtPointer clientdata, XtPointer calldata);
static void		undeleteCB(Widget w, XtPointer clientdata, XtPointer calldata);
static int		alert(Widget w, char *name, int buttons);
static void		alertCB(Widget w, int *stopval, XtPointer calldata);
static void		view_foldersCB(Widget w, XtPointer clientdata, XtPointer calldata);
static void		viewAC(Widget w, XEvent *ev, String *args, Cardinal *numargs);
static void		loadNewCB(Widget w, XtPointer clientdata, XtPointer);
static void		alignCaptions(Widget parent);
static Widget		captionLabel(Widget caption);
static void		deliverCb(Widget, XtPointer, XtPointer);
static Widget		create_toolbar_button(Widget parent, char *name,
					      char *cluehelp,
					      XtCallbackProc callback,
					      XtPointer clientdata);
static Widget		create_toolbar_toggle(Widget parent, char *name,
					      char *cluehelp,
					      XtCallbackProc callback,
					      XtPointer clientdata);
static void		resizePwinCb(Widget w, XtPointer clientdata, XEvent *event, Boolean *cont);
void			populate_combo(Widget combo);
static void		folderCb(Widget w, XtPointer clientdata, XtPointer calldata);
static void		saveCb(Widget w, XtPointer clientdata, XtPointer calldata);
static void		display_new_message();
static void		insert_message(Widget w, char *m);
static void		filerCb(Widget w, XtPointer clientdata, XtPointer calldata);
static void		insert_file(Widget w, XtPointer clientdata, XtPointer calldata);

static XtActionsRec actions[] = {
    {"view", (XtActionProc)viewAC}
};

/*----------------------------------------------------------------------*/

void
bad_file_notice(int w)
{
    alert(toplevel_, "badfile", 1);
}

/*----------------------------------------------------------------------*/

void
bad_key_notice_proc()
{
    alert(toplevel_, "badkey", 1);
}

/*----------------------------------------------------------------------*/

int
bad_pass_phrase_notice(int w)
{
    int		res = alert(toplevel_, "badpp", 2);

    return(res == XmCR_OK);
}

/*----------------------------------------------------------------------*/

static int
alert(Widget w, char *name, int buttons)
{
    int			stop = 0;
    char		*fullname;
    XEvent		event;
    XmString		string;
    Widget		alert;
    Arg			args[2];

    XtSetArg(args[0], XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
    XtSetArg(args[1], XmNdialogType, XmDIALOG_ERROR);

    /* This fullname stuff is so we can do specific alert things in the
       resource file (like the message), but also allow alert general
       resources to be set (like the window title).
       (Only it doesn't work! Sigh.) */
    fullname = XtMalloc(strlen(name) + 7);
    strcpy(fullname, name);
    strcat(fullname, "_alert");
    alert = XmCreateMessageDialog(w, fullname, args, 2);

    XtFree(fullname);

    XtAddCallback(alert, XmNcancelCallback, (XtCallbackProc)alertCB, &stop);
    XtAddCallback(alert, XmNokCallback, (XtCallbackProc)alertCB, &stop);
    if(buttons < 3)
	XtUnmanageChild(XmMessageBoxGetChild(alert, XmDIALOG_HELP_BUTTON));
    if(buttons < 2)
	XtUnmanageChild(XmMessageBoxGetChild(alert, XmDIALOG_CANCEL_BUTTON));
    XtManageChild(alert);

    /* This makes this function block until the dialog is dealt with */
    while (stop == 0){
	XtAppNextEvent(app_context_, &event);
	XtDispatchEvent(&event);
    }
    XtDestroyWidget(alert);

    return stop;
} /* alert */

/*----------------------------------------------------------------------*/

static void
alertCB(Widget w, int *stopval, XtPointer calldata)
{
    int		reason = ((XmAnyCallbackStruct *)calldata)->reason;

    *stopval = reason;
#if DEBUG
    if(reason == XmCR_OK){
	printf("OK pressed\n");
    }else if(reason == XmCR_CANCEL){
	printf("Cancel pressed\n");
    }
#endif
} /* alertCB */

/*----------------------------------------------------------------------*/

void
beep_display_window()
{
    XBell(XtDisplay(toplevel_), 100);
}

/*----------------------------------------------------------------------*/

void
clear_busy()
{
}

/*----------------------------------------------------------------------*/

void
clear_display_footer(DISPLAY_WINDOW *w)
{
    set_display_footer(w, "");
}

/*----------------------------------------------------------------------*/

void
clear_display_window()
{
    XmString	empty;

    if(text_ != NULL && XtIsRealized(text_)){
	empty = XmStringGenerate("", NULL, XmCHARSET_TEXT,
				 (XmStringTag)"LIST");
	XmCSTextSetString(text_, empty);
	XmStringFree(empty);
    }
}

/*----------------------------------------------------------------------*/

void
clear_main_footer()
{
    set_main_footer("");
}

/*----------------------------------------------------------------------*/

void
clear_passphrase_string()
{
    char	*c;
    Widget	pptext;

    if(pp_window_ != NULL){
	XtVaGetValues(pp_window_, XmNuserData, &pptext, NULL);
	XmTextFieldSetString(pptext, "");
    }

    for(c = local_pp; *c != '\0'; c++){
	*c = ' ';
    }
    *local_pp = '\0';
}

/*----------------------------------------------------------------------*/

void
close_all_windows()
{
}

/*----------------------------------------------------------------------*/

void
close_deliver_window(COMPOSE_WINDOW *w)
{
    XtPopdown(w->deliver_frame);
}

/*----------------------------------------------------------------------*/

void
close_passphrase_window()
{
    if(pp_window_ != NULL)
	XtUnmanageChild(pp_window_);
}

/*----------------------------------------------------------------------*/

int
compose_windows_open()
{
    int			c = 0;
    COMPOSE_WINDOW	*w;

    w = compose_first;

    while (w) {
	if (w->in_use)
	    c++;

	w = w->next;
    }

    return c;
}

/*----------------------------------------------------------------------*/

DISPLAY_WINDOW *
create_display_window()
{
}

/*----------------------------------------------------------------------*/

void
create_passphrase_window()
{
    Widget	pptext, helpwidget;

    if(pp_window_ == NULL){
	pp_window_ = XmCreateMessageDialog(toplevel_, "pphrase", NULL, 0);
	pptext = XmCreateTextField(pp_window_, "pptext", NULL, 0);
	XtManageChild(pptext);
	XtVaSetValues(pp_window_, XmNuserData, pptext, NULL);

	/* Unmap the Help button, we don't need it here. */
	helpwidget = XtNameToWidget(pp_window_, "Help");
	XtUnmanageChild(helpwidget);

	XtAddCallback(pptext, XmNmodifyVerifyCallback,
		      (XtCallbackProc)passphrase_cb, NULL);
	XtAddCallback(pp_window_, XmNokCallback,
		      (XtCallbackProc)got_passphrase, NULL);
    }
}

/*----------------------------------------------------------------------*/

void
delete_message_proc()
{
    int			message_number;
    MESSAGE		*m = messages.start;
    XmString		tempstring;
    int			sel_pos_count;
    unsigned int	*sel_posns;

    while (m) {
	if(XmListPosSelected(mailslist_, m->list_pos)){
	    message_number = m->list_pos;
	    XmListDeleteItemsPos(mailslist_, 1, message_number);
	    if(show_deleted){
		tempstring = XmStringGenerate(m->description, NULL,
					      XmCHARSET_TEXT,
					      (XmStringTag)"STRUCK");
		XmListAddItem(mailslist_, tempstring, message_number);
		XmStringFree(tempstring);
	    }
	    delete_message(m);
	}
	m = m->next;
    }

    /* Move current selected message pointer if neccesary */
    if (last_message_read) {
	last_message_read->flags |= MESS_SELECTED;
	display_message_description (last_message_read);
	display_message(last_message_read);
	sync_list();
    }
    update_message_list();
}

/*----------------------------------------------------------------------*/

void
deleteAllMessages()
{
    XmListDeleteAllItems(mailslist_);
} /* deleteAllMessages */

/*----------------------------------------------------------------------*/

void
display_message_body(BUFFER *b)
{
    XmString	body = XmStringGenerate(b->message, NULL, XmCHARSET_TEXT,
					(XmStringTag)"LIST");

    XmCSTextReplace(text_, 0, XmCSTextGetLastPosition(text_), body);
    XmStringFree(body);
    XmCSTextSetTopCharacter(text_, 0);
}

/*----------------------------------------------------------------------*/

void
display_message_description(MESSAGE *m)
{
    int		num;
    XmString	new_string;

    if(m->flags & MESS_DELETED){
	new_string = XmStringGenerate(m->description, NULL,
				      XmCHARSET_TEXT,
				      (XmStringTag)"STRUCK");
    }else{
	new_string = XmStringGenerate(m->description, NULL,
				      XmCHARSET_TEXT,
				      (XmStringTag)"LIST");
    }
    XtVaGetValues(mailslist_, XmNitemCount, &num, NULL);
    if(num >= m->list_pos){
	XmListReplaceItemsPos(mailslist_, &new_string, 1, m->list_pos);
    }else{
	XmListAddItem(mailslist_, new_string, m->list_pos);
    }
    XmStringFree(new_string);
}

/*----------------------------------------------------------------------*/

void
display_message_sig(BUFFER *b)
{
    /* TODO */
}

/*----------------------------------------------------------------------*/

void
display_sender_info(MESSAGE *m)
{
    XmString	info;

#ifndef LONG_HDR
    info = XmStringGenerate("From:\t", NULL, XmCHARSET_TEXT,
			    (XmStringTag)"HDR_B");
    if(m->sender != NULL)
	info = XmStringConcatAndFree(info,
				     XmStringGenerate(m->sender, NULL,
						      XmCHARSET_TEXT,
						      (XmStringTag)"HDR"));
    info = XmStringConcatAndFree(info,
				 XmStringGenerate("\nSubject:\t", NULL,
						  XmCHARSET_TEXT,
						  (XmStringTag)"HDR_B"));
    if(m->subject != NULL)
	info = XmStringConcatAndFree(info,
				     XmStringGenerate(m->subject, NULL,
						      XmCHARSET_TEXT,
						      (XmStringTag)"HDR_U"));
    info = XmStringConcatAndFree(info,
				 XmStringGenerate("\nDate:\t", NULL,
						  XmCHARSET_TEXT,
						  (XmStringTag)"HDR_B"));
    if(m->date != NULL)
	info = XmStringConcatAndFree(info,
				     XmStringGenerate(m->date, NULL,
						      XmCHARSET_TEXT,
						      (XmStringTag)"HDR"));
#if 0
    info = XmStringConcatAndFree(info,
				 XmStringGenerate("\n\n", NULL,
						  XmCHARSET_TEXT,
						  (XmStringTag)"HDR"));
#endif
    XmCSTextReplace(hdrtext_, 0, XmCSTextGetLastPosition(hdrtext_), info);

    XmStringFree(info);
#else
    char	*allhdr = strdup(m->header->message), *ptr, *split;
    char	keyword[256];	/* Should be long enough? :-) */
    int		length;

    ptr = strtok(allhdr, "\n");

    if(ptr == NULL){
	free(allhdr);
	return;
    }

    info = XmStringGenerate("", NULL, XmCHARSET_TEXT,
			    (XmStringTag)"HDR");
    XmCSTextReplace(hdrtext_, 0, XmCSTextGetLastPosition(hdrtext_), info);
    XmStringFree(info);

    do{
	if((split = strchr(ptr, ':')) != NULL){
	    split++;
	    length = split - ptr;
	    strncpy(keyword, ptr, length);
	    keyword[length] = '\0';
	    info = XmStringGenerate(keyword, NULL, XmCHARSET_TEXT,
				    (XmStringTag)"HDR_B");
	    info = XmStringConcatAndFree(info,
					 XmStringGenerate(split, NULL,
							  XmCHARSET_TEXT,
							  (XmStringTag)"HDR"));
	    info = XmStringConcatAndFree(info,
					 XmStringGenerate("\n", NULL,
							  XmCHARSET_TEXT,
							  (XmStringTag)"HDR"));
	    XmCSTextInsert(hdrtext_, XmCSTextGetInsertionPosition(hdrtext_),
			   info);
	    XmStringFree(info);
	}
	else{
	    info = XmStringGenerate(ptr, NULL, XmCHARSET_TEXT,
				    (XmStringTag)"HDR");
	    info = XmStringConcatAndFree(info,
					 XmStringGenerate("\n", NULL,
							  XmCHARSET_TEXT,
							  (XmStringTag)"HDR"));
	    XmCSTextInsert(hdrtext_, XmCSTextGetInsertionPosition(hdrtext_),
			   info);
	    XmStringFree(info);
	}
    }while(ptr = strtok(NULL, "\n"));

    free(allhdr);
#endif
}

/*----------------------------------------------------------------------*/

int
dont_quit_notice_proc()
{
    int res = alert(toplevel_, "dontquit", 2);
    return(res == XmCR_OK);
}

/*----------------------------------------------------------------------*/

int
failed_save_notice_proc()
{
    int res = alert(toplevel_, "failsave", 2);
    return(res == XmCR_OK);
}

/*----------------------------------------------------------------------*/

void
hide_header_frame()
{
}

/*----------------------------------------------------------------------*/

void
lock_display_window()
{
}

/*----------------------------------------------------------------------*/

int
no_key_notice_proc(int w)
{
    int res = alert(toplevel_, "nokey", 2);
    return(res == XmCR_OK);
}

/*----------------------------------------------------------------------*/

int
no_sec_notice_proc(int w)
{
    int res = alert(toplevel_, "nosec", 2);
    return(res == XmCR_OK);
}

/*----------------------------------------------------------------------*/

void
open_passphrase_window(char *s)
{
    Widget	pptext;

    XtVaGetValues(pp_window_, XmNuserData, &pptext, NULL);
    XtVaSetValues(XtParent(pp_window_), XmNtitle, s, NULL);

    XtManageChild(pp_window_);

    if(XmIsTraversable(pptext))
	XmProcessTraversal(pptext, XmTRAVERSE_CURRENT);
}

/*----------------------------------------------------------------------*/

char *
read_bcc(COMPOSE_WINDOW *w)
{
    char	*lbl;

    XtVaGetValues(w->send_bcc, XmNvalue, &lbl, NULL);
    return lbl;
}

/*----------------------------------------------------------------------*/

char *
read_cc(COMPOSE_WINDOW *w)
{
    char	*lbl;

    XtVaGetValues(w->send_cc, XmNvalue, &lbl, NULL);
    return lbl;
}

/*----------------------------------------------------------------------*/

int
read_deliver_flags(COMPOSE_WINDOW *w)
{
    int		flags = 0;

    flags |= XmToggleButtonGadgetGetState(w->sign);
    flags |= (XmToggleButtonGadgetGetState(w->encrypt) << 1);
    flags |= (XmToggleButtonGadgetGetState(w->log) << 2);
    flags |= (XmToggleButtonGadgetGetState(w->raw) << 3);
#ifndef NO_MIXMASTER
    flags |= (XmToggleButtonGadgetGetState(w->remail) << 4);
#endif

    return flags;
}

/*----------------------------------------------------------------------*/

char *
read_extra_headerline(COMPOSE_WINDOW *w, int i)
{
    return NULL;
}

/*----------------------------------------------------------------------*/

void
read_message_to_deliver(COMPOSE_WINDOW *w, BUFFER *b)
{
    char	*buf;

    XtVaGetValues(w->text, XmNvalue, &buf, NULL);
    add_to_buffer(b, buf, strlen(buf));
}

/*----------------------------------------------------------------------*/

int
read_only_notice_proc()
{
    int res = alert(toplevel_, "readonly", 2);
    return(res == XmCR_OK);
}

/*----------------------------------------------------------------------*/

char *
read_passphrase_string()
{
    return local_pp;
}

/*----------------------------------------------------------------------*/

char *
read_recipient(COMPOSE_WINDOW *w)
{
    char	*lbl;

    XtVaGetValues(w->send_to, XmNvalue, &lbl, NULL);
    return lbl;
}

/*----------------------------------------------------------------------*/

char *
read_subject(COMPOSE_WINDOW *w)
{
    char	*lbl;

    XtVaGetValues(w->send_subject, XmNvalue, &lbl, NULL);
    return lbl;
}

/*----------------------------------------------------------------------*/

#ifndef NO_MIXMASTER
void
remail_failed_notice_proc()
{
    alert(toplevel_, "remailfail", 1);
}
#endif

/*----------------------------------------------------------------------*/

void
set_display_footer(DISPLAY_WINDOW *w, char *s)
{
    XtVaSetValues(msgarea2_, XmNvalue, s, NULL);
}

/*----------------------------------------------------------------------*/

void
set_initial_scrollbar_position()
{
    /* Not needed for motif, XmText's behave themselves! ggt :-) */
}

/*----------------------------------------------------------------------*/

void
set_main_footer(char *s)
{
    XtVaSetValues(msgarea_, XmNvalue, s, NULL);
}

/*----------------------------------------------------------------------*/

void
setup_ui(int level, int argc, char **argv)
{
    Widget	control_;
    char	*title, *dbglvl;
    XImage	*img, *shp_img;

    toplevel_ = XtVaOpenApplication(&app_context_, "Privtool", options,
				    XtNumber(options),
				    &argc, argv, NULL,
				    applicationShellWidgetClass, 0);

    if((dbglvl = GetResourceString(toplevel_, "debug", "Debug")) != NULL){
	debug_ = atoi(dbglvl);
	/*fprintf(stderr, "%s: Got debug level: %d\n", argv[0], debug_);*/
	DEBUG1(("Got debug level: %d\n", debug_));
    }

    title = (char *)malloc(strlen(prog_name) + strlen(prog_ver) + 5);
    strcpy(title, prog_name);
    strcat(title, " - ");
    strcat(title, prog_ver);
    XtVaSetValues(toplevel_, XmNtitle, title, NULL);

    /* Shell for toolbar "bubble" help */
    liteClue_ = XtVaCreatePopupShell( "LiteClue_shell",
				     xcgLiteClueWidgetClass,
				     toplevel_, NULL);

    cache_pixmap_from_data(prev_xpm, "prev.xpm");
    cache_pixmap_from_data(next_xpm, "next.xpm");
    cache_pixmap_from_data(delete_xpm, "delete.xpm");
    cache_pixmap_from_data(undelete_xpm, "undelete.xpm");
    cache_pixmap_from_data(folderwin_xpm, "folderwin.xpm");

    /* Add editres protocol support */
    XtAddEventHandler(toplevel_, 0, True, _XEditResCheckMessages, NULL);

    XtAppAddActions(app_context_, actions, XtNumber(actions));

    control_ = XmCreateMainWindow (toplevel_, "main_window", NULL, 0);
    XtManageChild (control_);

    create_menubar(control_);
    create_toolbar(control_);
    create_workarea(control_);
    create_msgareas(control_);

    last_message_read = messages.start;
    if(last_message_read)
	display_new_message();
    sync_list();
    update_message_list();
    
    XSetIOErrorHandler(xioerror_handler);

    XtRealizeWidget (toplevel_);
    XtAppMainLoop (app_context_);
}

/*----------------------------------------------------------------------*/

void
show_addkey()
{
}

/*----------------------------------------------------------------------*/

void
show_busy()
{
}

/*----------------------------------------------------------------------*/

void
show_display_window(MESSAGE *m)
{
}

/*----------------------------------------------------------------------*/

void
show_newmail_icon()
{
    XtVaSetValues(toplevel_, XmNiconPixmap, "privtool-new.xpm", NULL);
}

/*----------------------------------------------------------------------*/

void
show_normal_icon()
{
    XtVaSetValues(toplevel_, XmNiconPixmap, "privtool.xpm", NULL);
}

/*----------------------------------------------------------------------*/

void
shutdown_ui()
{
    XtDestroyWidget(toplevel_);
}

/*----------------------------------------------------------------------*/

void
update_log_item(COMPOSE_WINDOW *w)
{
}

/*----------------------------------------------------------------------*/

void
update_message_list()
{
    char	s[128];
    char	b[64];
	
    if (!deleted.number)
	sprintf (s, "%d messages", messages.number);
    else
	sprintf (s, "%d messages, %d deleted", messages.number,
		 deleted.number);

    if (messages.new) {
	sprintf (b, ", %d new", messages.new);
	strcat (s, b);
    }

    if (messages.unread) {
	sprintf (b, ", %d unread", messages.unread);
	strcat (s, b);
    }

#ifdef SHOW_ENCRYPTED
    if (messages.encrypted) {
	sprintf (b, ", %d encrypted", messages.encrypted);
	strcat (s, b);
    }
#endif
    set_main_footer(s);
}

/*----------------------------------------------------------------------*/

COMPOSE_WINDOW *
x_setup_send_window()
{
}

/*----------------------------------------------------------------------*/

static COMPOSE_WINDOW *
compose_find_free()
{
    COMPOSE_WINDOW	*w;

    w = compose_first;

    while (w) {
	if (!w->in_use)
	    return w;

	w = w->next;
    }

    return NULL;
}

/*----------------------------------------------------------------------*/

static COMPOSE_WINDOW *
setup_composeCB(Widget w, XtPointer clientdata, XtPointer calldata)
{
    int			i, width, ww;
    COMPOSE_WINDOW	*win = NULL;
    COMPOSE_TYPE	comp_type;
    BUFFER		*b;
    char		*send_to;
    Widget		msgbox, topform, form, ctrl, tbox, txt, swin, cap;
    Widget		button;

    comp_type = (COMPOSE_TYPE)clientdata;
    if (comp_type == COMPOSE_REPLY && !last_message_read)
	return;

    /* See if we have a window we can use */
    win = compose_find_free();

    if(win == NULL){
	win = XtNew(COMPOSE_WINDOW);
	win->deliver_frame = XtVaCreatePopupShell("compose",
						  topLevelShellWidgetClass,
						  toplevel_,
						  NULL);
	win->next = NULL;
	if (compose_last) {
	    compose_last->next = win;
	    win->prev = compose_last;
	    compose_last = win;
	}
	else {
	    win->prev = NULL;
	    compose_first = compose_last = win;
	}

	/* Add editres protocol support */
	XtAddEventHandler(win->deliver_frame, 0, True,
			  _XEditResCheckMessages, NULL);
	XtManageChild(win->deliver_frame);

	msgbox = XmCreateMessageBox(win->deliver_frame, "msgbox", NULL, 0);
	XtManageChild(msgbox);
	XtUnmanageChild(XtNameToWidget(msgbox, "*Help"));
	XtUnmanageChild(XtNameToWidget(msgbox, "*Message"));
	XtUnmanageChild(XtNameToWidget(msgbox, "*Symbol"));
	XtAddCallback(msgbox, XmNokCallback, deliverCb, win);
	XtAddCallback(msgbox, XmNcancelCallback, deliverCb, win);

	topform = XmCreateForm(msgbox, "topform", NULL, 0);
	XtManageChild(topform);
	ctrl = XmCreateRowColumn(topform, "ctrl", NULL, 0);
	XtManageChild(ctrl);

	cap = XtCreateManagedWidget("tocap", xbaeCaptionWidgetClass,
				    ctrl, NULL, 0);
	win->send_to = XmCreateTextField(cap, "to", NULL, 0);
	XtManageChild(win->send_to);

	cap = XtCreateManagedWidget("subjcap", xbaeCaptionWidgetClass,
				    ctrl, NULL, 0);
	win->send_subject = XmCreateTextField(cap, "subj", NULL, 0);
	XtManageChild(win->send_subject);

	cap = XtCreateManagedWidget("cccap", xbaeCaptionWidgetClass,
				    ctrl, NULL, 0);
	win->send_cc = XmCreateTextField(cap, "cc", NULL, 0);
	XtManageChild(win->send_cc);

	cap = XtCreateWidget("bcccap", xbaeCaptionWidgetClass,
			     ctrl, NULL, 0);
	if(find_mailrc("askbcc"))
	    XtManageChild(cap);
	win->send_bcc = XmCreateTextField(cap, "bcc", NULL, 0);
	if(find_mailrc("askbcc"))
	    XtManageChild(win->send_bcc);

	for (i = 1; i < MAX_EXTRA_HEADERLINES; i++) {
	    char	mailrcline[BUFSIZ];
	    char	*headerline;
	    XmString	xlabel;

	    sprintf(mailrcline, "header%d", i);

	    if ((headerline = find_mailrc(mailrcline))) {
		char	 *label;

		label = XtMalloc(strlen(headerline)+2);
			
		sprintf(label, "%s%s", headerline, ":");
		cap = XtCreateManagedWidget(mailrcline, xbaeCaptionWidgetClass,
					    ctrl, NULL, 0);
		xlabel = XmStringGenerate(label, XmFONTLIST_DEFAULT_TAG,
					  XmCHARSET_TEXT, NULL);
		XtVaSetValues(cap,
			      XmNlabelType, XmSTRING,
			      XmNlabelString, xlabel, NULL);
		XmStringFree(xlabel);
		win->extra_headers[i] = XmCreateTextField(cap, label, NULL, 0);
		XtManageChild(win->extra_headers[i]);
		XtFree(label);
	    }else{
		win->extra_headers[i] = NULL;
	    }
	}

	alignCaptions(ctrl);

	tbox = XmCreateRowColumn(topform, "tbox", NULL, 0);
	XtManageChild(tbox);

	button = XmCreatePushButton(topform, "insert", NULL, 0);
	XtManageChild(button);

	win->sign = XmCreateToggleButtonGadget(tbox, "sign", NULL, 0);
	XtManageChild(win->sign);
	win->encrypt = XmCreateToggleButtonGadget(tbox, "encrypt", NULL, 0);
	XtManageChild(win->encrypt);
	win->log = XmCreateToggleButtonGadget(tbox, "log", NULL, 0);
	XtManageChild(win->log);
	win->raw = XmCreateToggleButtonGadget(tbox, "raw", NULL, 0);
	XtManageChild(win->raw);
#ifndef NO_MIXMASTER
	win->remail = XmCreateToggleButtonGadget(tbox, "remail", NULL, 0);
	XtManageChild(win->remail);
#endif

	win->text = XmCreateScrolledText(topform, "stext", NULL, 0);
	XtAddCallback(win->text, XmNdestinationCallback,
		      (XtCallbackProc)destnCb, NULL);
	XtAddCallback(button, XmNactivateCallback,
		      (XtCallbackProc)filerCb, win->text);
	XtManageChild(win->text);
    }

    /* Now clear all fields (in case they were used before). */
    XtVaSetValues(win->send_to, XmNvalue, "", NULL);
    XtVaSetValues(win->send_cc, XmNvalue, "", NULL);
    XtVaSetValues(win->send_subject, XmNvalue, "", NULL);
    XtVaSetValues(win->send_bcc, XmNvalue, "", NULL);
    XtVaSetValues(win->text, XmNvalue, "", NULL);
    for(i = 1; i < MAX_EXTRA_HEADERLINES; i++) {
	if(win->extra_headers[i] != NULL){
	    XtVaSetValues(win->extra_headers[i], XmNvalue, "", NULL);
	}
    }

    switch(comp_type){
    case COMPOSE_NEW:
	/* Do something to make keyboard focus go to the send_to field */
	if(XmIsTraversable(win->send_to))
	    XmProcessTraversal(win->send_to, XmTRAVERSE_CURRENT);
	break;
    case COMPOSE_REPLY:
	set_reply (last_message_read);

	send_to = last_message_read->email;

	if (last_message_read->reply_to &&
		!find_mailrc("defaultusefrom")) {
		if (find_mailrc("defaultusereplyto") ||
			alert(toplevel_, "usereplyto", 2)) {
			send_to = last_message_read->reply_to;
		}
	}
	XtVaSetValues(win->send_to, XmNvalue, send_to, NULL);

	XtVaSetValues(win->send_subject, XmNvalue, "Re: ", NULL);
	XmTextFieldInsert(win->send_subject,
			  XmTextFieldGetLastPosition(win->send_subject),
			  last_message_read->subject);

	if (last_message_read->decrypted)
		b = last_message_read->decrypted;
	else
		b = message_contents(last_message_read);

	XtVaSetValues(win->text, XmNvalue, last_message_read->sender, NULL);
	XmTextSetInsertionPosition(win->text,
				   XmTextGetLastPosition(win->text));
	XmTextInsert(win->text, XmTextGetInsertionPosition(win->text),
		     attribution_string);
	XmTextSetInsertionPosition(win->text,
				   XmTextGetLastPosition(win->text));
	insert_message(win->text, b->message);
	XmTextSetInsertionPosition(win->text, 0);
	if(XmIsTraversable(win->text))
	    XmProcessTraversal(win->text, XmTRAVERSE_CURRENT);
	break;
    case COMPOSE_FORWARD:
	XtVaSetValues(win->send_subject, XmNvalue, last_message_read->subject,
		      NULL);
	XmTextFieldInsert(win->send_subject,
			  XmTextFieldGetLastPosition(win->send_subject),
			  " (fwd)");
	if (last_message_read->decrypted)
		b = last_message_read->decrypted;
	else
		b = message_contents(last_message_read);

	XmTextInsert(win->text, XmTextGetLastPosition(win->text),
		     begin_forward);
	XmTextInsert(win->text, XmTextGetLastPosition(win->text),
		     b->message);
	XmTextInsert(win->text, XmTextGetLastPosition(win->text),
		     end_forward);
	if(XmIsTraversable(win->send_to))
	    XmProcessTraversal(win->send_to, XmTRAVERSE_CURRENT);
	break;
    case COMPOSE_RESEND:
	break;
    }

    win->in_use = 1;
    XtPopup(win->deliver_frame, XtGrabNonexclusive);

    return win;
}

/*----------------------------------------------------------------------*/

static void
insert_message(Widget w, char *m)
{
    XmTextPosition	pos, newpos;
    char		*indent;

    if (!(indent = find_mailrc("indentprefix")))
	indent = "> ";

    XmTextInsert(w, (pos = XmTextGetInsertionPosition(w)), m);
    XmTextInsert(w, pos, indent);
    while(XmTextFindString(w, pos, "\n", XmTEXT_FORWARD,
			   &newpos)){
	XmTextInsert(w, newpos+1, indent);
	pos = newpos+1;
    }
} /* insert_message */

/*----------------------------------------------------------------------*/

static void
deliverCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    XmAnyCallbackStruct *cbs = (XmAnyCallbackStruct *)calldata;
    COMPOSE_WINDOW	*win = (COMPOSE_WINDOW *)clientdata;

    win->in_use = 0;
    if(cbs->reason == XmCR_OK){
	/* Do the deliver stuff */
	char		*lbl;

	XtVaGetValues(win->send_to, XmNvalue, &lbl, NULL);
	deliver_proc(win);
    }else{
	XtPopdown(win->deliver_frame);
    }
} /* deliverCb */

/*----------------------------------------------------------------------*/

static void
create_menubar(Widget parent)
{
    Widget	menubar_;

    menubar_ = XmCreateMenuBar (parent, "menu_bar", NULL, 0);
    XtManageChild (menubar_);
    XtVaSetValues (parent, XmNmenuBar, menubar_, NULL);

    create_file_menu(menubar_);
    create_edit_menu(menubar_);
    create_view_menu(menubar_);
    create_fold_menu(menubar_);
    create_comp_menu(menubar_);
    create_help_menu(menubar_);
} /* create_menubar */

/*----------------------------------------------------------------------*/

static void
create_toolbar(Widget parent)
{
    Widget	toolbar_, btn, frame;
    Pixmap	pixmap;

    frame = XmCreateFrame(parent, "toolframe", NULL, 0);
    XtManageChild(frame);
    toolbar_ = XmCreateForm(frame, "toolbar", NULL, 0);
    XtManageChild(toolbar_);
    XtVaSetValues (parent, XmNcommandWindow, frame, NULL);

    create_toolbar_button(toolbar_, "prev", "Previous message",
			  prev_messageCB, NULL);

    create_toolbar_button(toolbar_, "next", "Next message",
			  next_messageCB, NULL);

    create_toolbar_button(toolbar_, "delete", "Delete message",
			  delete_message_proc, NULL);

    create_toolbar_button(toolbar_, "undelete", "Undelete message",
			  undeleteCB, NULL);

    foldwin_[1] = create_toolbar_toggle(toolbar_, "folders", "Folder window",
				       view_foldersCB, NULL);
    fold_combo_ = XmCreateComboBox(toolbar_, "combo", NULL, 0);
    XtManageChild(fold_combo_);
    XcgLiteClueAddWidget(liteClue_, fold_combo_, "Current mail folder", 0, 0);
    populate_combo(fold_combo_);
} /* create_toolbar */

/*----------------------------------------------------------------------*/

static Widget
create_toolbar_button(Widget parent, char *name,
		      char *cluehelp, XtCallbackProc callback,
		      XtPointer clientdata)
{
    Widget	btn;
    Pixmap	pixmap;

    btn = XmCreatePushButton(parent, name, NULL, 0);
    XtManageChild(btn);
    XcgLiteClueAddWidget(liteClue_, btn, cluehelp, 0, 0);
    XtAddCallback(btn, XmNactivateCallback,
		  callback, clientdata);
    
    if((pixmap = get_cached_pixmap(btn, GetResourceString(btn,
							  "labelPixmap",
							  "LabelPixmap")
				   )) != 0){
	XtVaSetValues(btn, XmNlabelPixmap, pixmap, NULL);
    }
    return btn;
} /* create_toolbar_button */

/*----------------------------------------------------------------------*/

static Widget
create_toolbar_toggle(Widget parent, char *name,
		      char *cluehelp, XtCallbackProc callback,
		      XtPointer clientdata)
{
    Widget	btn;
    Pixmap	pixmap;

    btn = XmCreateToggleButton(parent, name, NULL, 0);
    XtManageChild(btn);
    XcgLiteClueAddWidget(liteClue_, btn, cluehelp, 0, 0);
    XtAddCallback(btn, XmNvalueChangedCallback,
		  callback, clientdata);
    
    if((pixmap = get_cached_pixmap(btn, GetResourceString(btn,
							  "labelPixmap",
							  "LabelPixmap")
				   )) != 0){
	XtVaSetValues(btn, XmNlabelPixmap, pixmap, NULL);
    }
    return btn;
} /* create_toolbar_toggle */

/*----------------------------------------------------------------------*/

static void
create_file_menu(Widget parent)
{
    Widget	menu_, cascade_, button_;
    Arg		args[2];

    menu_ = XmCreatePulldownMenu (parent, "file_pane", NULL, 0);
    XtSetArg (args[0], XmNsubMenuId, menu_);
    cascade_ = XmCreateCascadeButton (parent, "file", args, 1);
    XtManageChild (cascade_);

    button_ = XmCreatePushButtonGadget (menu_, "load", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		   loadNewCB, NULL);

    button_ = XmCreatePushButtonGadget (menu_, "save", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		   saveCb, NULL);

    button_ = XmCreatePushButtonGadget (menu_, "done", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		   done_proc, NULL);

    XtManageChild(XmCreateSeparatorGadget(menu_, "sep", NULL, 0));

    button_ = XmCreatePushButtonGadget (menu_, "qwsav", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		   save_and_quit_proc, NULL);

    button_ = XmCreatePushButtonGadget (menu_, "quit", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		   quit_proc, NULL);

} /* create_file_menu */

/*----------------------------------------------------------------------*/

static void
create_edit_menu(Widget parent)
{
    Widget	menu_, cascade_, button_;
    Arg		args[2];

    menu_ = XmCreatePulldownMenu (parent, "edit_pane", NULL, 0);
    XtSetArg (args[0], XmNsubMenuId, menu_);
    cascade_ = XmCreateCascadeButton (parent, "edit", args, 1);
    XtManageChild (cascade_);

    button_ = XmCreatePushButtonGadget (menu_, "cut", NULL, 0);
    XtManageChild (button_);

    button_ = XmCreatePushButtonGadget (menu_, "copy", NULL, 0);
    XtManageChild (button_);

    button_ = XmCreatePushButtonGadget (menu_, "delete", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		   delete_message_proc, NULL);

    button_ = XmCreatePushButtonGadget (menu_, "undelete", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		   undeleteCB, NULL);

    button_ = XmCreatePushButtonGadget (menu_, "undel_last", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		   undelete_last_proc, NULL);

    button_ = XmCreatePushButtonGadget (menu_, "clearpp", NULL, 0);
    XtManageChild (button_);
} /* create_edit_menu */

/*----------------------------------------------------------------------*/

static void
create_view_menu(Widget parent)
{
    Widget	menu_, cascade_, button_;
    Arg		args[2];

    menu_ = XmCreatePulldownMenu (parent, "view_pane", NULL, 0);
    XtSetArg (args[0], XmNsubMenuId, menu_);
    cascade_ = XmCreateCascadeButton (parent, "view", args, 1);
    XtManageChild (cascade_);

    button_ = XmCreatePushButtonGadget (menu_, "next", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		   next_messageCB, NULL);

    button_ = XmCreatePushButtonGadget (menu_, "prev", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		   prev_messageCB, NULL);

    button_ = XmCreatePushButtonGadget (menu_, "sortby", NULL, 0);
    XtManageChild (button_);

    button_ = XmCreateSeparatorGadget(menu_, "separator", NULL, 0);
    XtManageChild (button_);

    foldwin_[0] = XmCreateToggleButton(menu_, "folders", NULL, 0);
    XtManageChild (foldwin_[0]);
    XtAddCallback (foldwin_[0], XmNvalueChangedCallback,
		   view_foldersCB, NULL);
} /* create_view_menu */

/*----------------------------------------------------------------------*/

static void
create_fold_menu(Widget parent)
{
    Widget	menu_, cascade_, button_;
    Arg		args[2];

    menu_ = XmCreatePulldownMenu (parent, "fold_pane", NULL, 0);
    XtSetArg (args[0], XmNsubMenuId, menu_);
    cascade_ = XmCreateCascadeButton (parent, "folder", args, 1);
    XtManageChild (cascade_);

    button_ = XmCreatePushButtonGadget (menu_, "copy", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		   (XtCallbackProc)folderCb,
		   (XtPointer)FOLDER_COPY);

    button_ = XmCreatePushButtonGadget (menu_, "move", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		   (XtCallbackProc)folderCb,
		   (XtPointer)FOLDER_MOVE);

    button_ = XmCreatePushButtonGadget (menu_, "load", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		   (XtCallbackProc)folderCb,
		   (XtPointer)FOLDER_LOAD);
} /* create_fold_menu */

/*----------------------------------------------------------------------*/

static void
create_comp_menu(Widget parent)
{
    Widget	menu_, cascade_, button_;
    Arg		args[2];

    menu_ = XmCreatePulldownMenu (parent, "comp_pane", NULL, 0);
    XtSetArg (args[0], XmNsubMenuId, menu_);
    cascade_ = XmCreateCascadeButton (parent, "comp", args, 1);
    XtManageChild (cascade_);

    button_ = XmCreatePushButtonGadget (menu_, "new", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		   (XtCallbackProc)setup_composeCB,
		   (XtPointer)COMPOSE_NEW);

    button_ = XmCreatePushButtonGadget (menu_, "reply", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		   (XtCallbackProc)setup_composeCB,
		   (XtPointer)COMPOSE_REPLY);

    button_ = XmCreatePushButtonGadget (menu_, "forward", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		   (XtCallbackProc)setup_composeCB,
		   (XtPointer)COMPOSE_FORWARD);

    button_ = XmCreatePushButtonGadget (menu_, "resend", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		   (XtCallbackProc)setup_composeCB,
		   (XtPointer)COMPOSE_RESEND);
} /* create_comp_menu */

/*----------------------------------------------------------------------*/

static void
create_help_menu(Widget parent)
{
    Widget	menu_, cascade_, button_;
    Arg		args[2];

    menu_ = XmCreatePulldownMenu (parent, "comp_pane", NULL, 0);
    XtSetArg (args[0], XmNsubMenuId, menu_);
    cascade_ = XmCreateCascadeButton (parent, "help", args, 1);
    XtManageChild (cascade_);
    XtVaSetValues(parent, XmNmenuHelpWidget, cascade_, NULL);

    button_ = XmCreatePushButtonGadget (menu_, "about", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNactivateCallback,
		       aboutCB, NULL);
} /* create_help_menu */

/*----------------------------------------------------------------------*/

static void
create_workarea(Widget parent)
{
    Widget	swin, pwin, frame;
    MESSAGE	*m;
    int		i, l;

    frame = XmCreateFrame(parent, "frame", NULL, 0);
    XtManageChild(frame);
    pwin = XmCreatePanedWindow(frame, "panedwin", NULL, 0);
    XtVaSetValues (parent, XmNworkWindow, frame, NULL);
    XtManageChild(pwin);
    XtAddEventHandler(pwin, StructureNotifyMask, False, resizePwinCb, NULL);

    mailslist_ = XmCreateScrolledList(pwin, "slist", NULL, 0);

    XtAddCallback(mailslist_, XmNdefaultActionCallback,
		  (XtCallbackProc)show_message, NULL);
    XtAddCallback(mailslist_, XmNconvertCallback,
		  (XtCallbackProc)listConvertCb, NULL);
    XtAddCallback(mailslist_, XmNdestinationCallback,
		  (XtCallbackProc)destnCb, NULL);
    XtManageChild (mailslist_);

    hdrtext_ = XmCreateScrolledCSText(pwin, "hdrtext", NULL, 0);
    XtAddCallback(hdrtext_, XmNconvertCallback,
		  (XtCallbackProc)textConvertCb, NULL);
    XtManageChild(hdrtext_);

    text_ = XmCreateScrolledCSText(pwin, "msgtext", NULL, 0);
    XtAddCallback(text_, XmNconvertCallback,
		  (XtCallbackProc)textConvertCb, NULL);
    XtManageChild(text_);

    m = messages.start;
    i = 1;
    l = 1;
    while (m) {
	m->list_pos = l;
	m->number = i;

	if (!(m->flags & MESS_DELETED))
	    l++;
	else
	    delete_message (m);

	if (m->status == MSTAT_NONE)
	    show_newmail_icon();

	set_message_description(m);
	m = m->next;
	i++;
    }
    update_mail_list();
} /* create_workarea */

/*----------------------------------------------------------------------*/

static void
update_mail_list()
{
    MESSAGE	*m;
    XmString	tempstring;

    m = messages.start;

    while (m) {
	XmListAddItem(mailslist_,
		      tempstring = XmStringGenerate(m->description, NULL,
						    XmCHARSET_TEXT,
						    (XmStringTag)"LIST"),
		      0);
	XmStringFree(tempstring);
	m = m->next;
    }
} /* update_mail_list */

/*----------------------------------------------------------------------*/

static void
create_msgareas(Widget parent)
{
    Widget	form, frame;

    frame = XmCreateFrame(parent, "msgframe", NULL, 0);
    XtManageChild (frame);
    form = XmCreateForm(frame, "msgform", NULL, 0);
    XtManageChild (form);
    msgarea_ = XmCreateTextField(form, "msg1", NULL, 0);
    XtManageChild (msgarea_);
    msgarea2_ = XmCreateTextField(form, "msg2", NULL, 0);
    XtManageChild (msgarea2_);
    XtVaSetValues (parent, XmNmessageWindow, frame, NULL);
} /* create_msgarea */

/*----------------------------------------------------------------------*/

static void
show_message(Widget w, XtPointer none, XmListCallbackStruct *cbs)
{
    MESSAGE	*m = messages.start;

    if(cbs->reason == XmCR_DEFAULT_ACTION){
	while (m) {
	    if(m->list_pos == cbs->item_position){
		select_message_proc(m);
		sync_list();
		break;
	    }
	    m = m->next;
	}
    }
} /* show_message */

/*----------------------------------------------------------------------*/

static void
select_message(Widget w, XtPointer none, XmListCallbackStruct *cbs)
{
    MESSAGE	*m = messages.start;

    if(cbs->reason == XmCR_SINGLE_SELECT ||
       cbs->reason == XmCR_BROWSE_SELECT){
	while (m) {
	    if(m->list_pos == cbs->item_position){
		m->flags |= MESS_SELECTED;
	    }else if(m->flags | MESS_SELECTED){
		m->flags &= ~MESS_SELECTED;
	    }
	    m = m->next;
	}
    }
} /* select_message */

/*----------------------------------------------------------------------*/

/* Make sure the right list item is the one being shown, and that it is
   visible in the list.
 */
void
sync_list()
{
    XmString	new_string;
    int		topItem, numVisible;

    if(last_message_read == NULL)
	return;

    new_string = XmStringGenerate(last_message_read->description, NULL,
				  XmCHARSET_TEXT,
				  (XmStringTag)"BOLD");

    XmListReplaceItemsPos(mailslist_, &new_string, 1,
			  last_message_read->list_pos);
    XmStringFree(new_string);
    XmListSelectPos(mailslist_, last_message_read->list_pos, 0);

    XtVaGetValues(mailslist_, XmNtopItemPosition, &topItem,
		  XmNvisibleItemCount, &numVisible, NULL);
    if(last_message_read->list_pos < topItem){
	XtVaSetValues(mailslist_, XmNtopItemPosition,
		      last_message_read->list_pos, NULL);
    }else if((last_message_read->list_pos + 1) > topItem + numVisible){
	XtVaSetValues(mailslist_, XmNtopItemPosition,
		      last_message_read->list_pos - numVisible + 1, NULL);
    }
} /* sync_list */

/*----------------------------------------------------------------------*/

static void
next_messageCB(Widget w, XtPointer clientdata, XtPointer calldata)
{
    next_message_proc();
    sync_list();
} /* next_messageCB */

/*----------------------------------------------------------------------*/

static void
prev_messageCB(Widget w, XtPointer clientdata, XtPointer calldata)
{
    prev_message_proc();
    sync_list();
} /* prev_messageCB */

/*----------------------------------------------------------------------*/

static int
xioerror_handler(Display *dpy)
{
    quit_proc();
} /* initial_expose */

/*----------------------------------------------------------------------*/

/*
   This is the function called as a result of a modifyVerifyCallback
   on the TextField where the passphrase is typed. It subsitutes the
   real text typed with '*'s. The real text is saved in the static
   char array - local_pp.
 */
static void
passphrase_cb(Widget w, XtPointer clientdata, XmTextVerifyPtr tb)
{
    char	*temp_ptr = '\0', *local_mask, *c;

    /* save the bit at the end(if any) */
    if(local_pp[tb->endPos] != '\0'){
	temp_ptr = strdup(&local_pp[tb->endPos]);
    }

    /* Add in the new bit at the appropriate place */
    if(tb->text->ptr != NULL)
	strncpy(&local_pp[tb->startPos], tb->text->ptr, tb->text->length);

    /* Null the end for good measure (i.e _don't_ delete this line!) */
    local_pp[tb->startPos + tb->text->length] = '\0';

    /* Append the initial end bit if there was any */
    if(temp_ptr != NULL){
	strcat(&local_pp[tb->startPos] + tb->text->length, temp_ptr);
	free(temp_ptr);
    }

    /* Return the appropriate number of asterixes. Motif seems to free
       the string it gets, so I don't think there's a leak here.
       Purify will tell me at some stage. */
    if(tb->text->ptr != NULL){
	local_mask = strdup(tb->text->ptr);
	for(c = local_mask; *c != '\0'; c++){
	    *c = '*';
	}
	tb->text->ptr = local_mask;
    }
} /* passphrase_cb */

/*----------------------------------------------------------------------*/

static void
aboutCB(Widget w, XtPointer clientdata, XtPointer calldata)
{
    Widget	tempwidget;
    XmString	about;

    if(abt_window_ == NULL){
	abt_window_ = XmCreateMessageDialog(toplevel_, "about", NULL, 0);

	/* Unmap the Help & Cancel buttons, we don't need them here. */
	tempwidget = XtNameToWidget(abt_window_, "Help");
	XtUnmanageChild(tempwidget);
	tempwidget = XtNameToWidget(abt_window_, "Cancel");
	XtUnmanageChild(tempwidget);

	/* Set render table on the Message child widget */
	tempwidget = XtNameToWidget(abt_window_, "Message");
	/*XtVaSetValues(tempwidget, XmNrenderTable, render_header, NULL);*/
	about = XmStringGenerate(prog_name,
				 NULL, XmCHARSET_TEXT,
				 (XmStringTag)"BIG");
	about = XmStringConcatAndFree(about,
		XmStringGenerate(" - A PGP aware mailer", NULL,
				 XmCHARSET_TEXT,
				 (XmStringTag)"BIG"));
	about = XmStringConcatAndFree(about,
		XmStringGenerate("\n\nWritten (mainly) by Mark Grant ", NULL,
				 XmCHARSET_TEXT,
				 (XmStringTag)"HDR"));
	about = XmStringConcatAndFree(about,
		XmStringGenerate("(mark@unicorn.com)", NULL,
				 XmCHARSET_TEXT,
				 (XmStringTag)"BLUE"));
	about = XmStringConcatAndFree(about,
		XmStringGenerate("\nMotif interface written by Glenn Trigg ", NULL,
				 XmCHARSET_TEXT,
				 (XmStringTag)"HDR"));
	about = XmStringConcatAndFree(about,
		XmStringGenerate("(ggt@netspace.net.au)", NULL,
				 XmCHARSET_TEXT,
				 (XmStringTag)"BLUE"));
	XtVaSetValues(abt_window_, XmNmessageString, about, NULL);

	XmStringFree(about);
    }
    XtManageChild(abt_window_);
} /* aboutCB */

/*----------------------------------------------------------------------*/

static void
undeleteCB(Widget w, XtPointer clientdata, XtPointer calldata)
{
    MESSAGE		*m = messages.start;
    int			sel_pos_count;
    unsigned int	*sel_posns;

    XtVaGetValues(mailslist_, XmNselectedPositionCount, &sel_pos_count, NULL);
    sel_posns = calloc(sel_pos_count, sizeof(int));
    XtVaGetValues(mailslist_, XmNselectedPositions, &sel_posns, NULL);

    while(m){
	if(m->list_pos == *sel_posns){
	    undelete(m);
	    break;
	}
	m = m->next;
    }
} /* undeleteCB */

/*----------------------------------------------------------------------*/

static void
view_foldersCB(Widget w, XtPointer clientdata, XtPointer calldata)
{
    if(XmToggleButtonGetState(w)){
	show_mail_folders(toplevel_);
    }else{
	hide_mail_folders();
    }
} /* view_foldersCB */

/*----------------------------------------------------------------------*/

static void
viewAC(Widget w, XEvent *ev, String *args, Cardinal *numargs)
{
    int		i;

#ifdef DEBUG
    printf("In viewAC(");
    for(i = 0; i < *numargs; i++){
	if(i != 0)
	    printf(", ");
	printf("%s", args[i]);
    }
    printf(")\n");
#endif

    if(*numargs < 1)
	return;			/* Silently fail */

    if(!strcmp(args[0], "next")){
	next_messageCB(w, NULL, NULL);
    }else if(!strcmp(args[0], "prev")){
	prev_messageCB(w, NULL, NULL);
    }
} /* viewAC */

/*----------------------------------------------------------------------*/

static void
loadNewCB(Widget w, XtPointer clientdata, XtPointer calldata)
{
    deleteAllMessages();
    inbox_proc();
    display_new_message();
    sync_list();
    update_message_list();
} /* loadNewCB */

/*----------------------------------------------------------------------*/

static void
alignCaptions(Widget parent)
{
    int			i, numkids;
    WidgetList		kids;
    Dimension		length = 0;
    XtWidgetGeometry	curr;

#if 0
    if(XmeTraitGet(parent, XmQTcontainer) == NULL){
	/* This widget can't contain children, esp. Captions. */
	return;
    }
#endif

    XtVaGetValues(parent, XmNchildren, &kids,
		  XmNnumChildren, &numkids, NULL);

    for(i = 0; i < numkids; i++){
	if(XtClass(kids[i]) == xbaeCaptionWidgetClass){
	    XtQueryGeometry(captionLabel(kids[i]), NULL, &curr);
	    if(curr.width > length)
		length = curr.width;
	}
    }
    for(i = 0; i < numkids; i++){
	if(XtClass(kids[i]) == xbaeCaptionWidgetClass){
	    XtVaSetValues(captionLabel(kids[i]), XmNwidth, length, NULL);
	    /* Hack Alert!! But what else to do when there is no other way? */
	    /* This makes the Caption notice that the label has changed
	       size. */
	    ((CompositeWidgetClass)XtClass(kids[i]))->composite_class.change_managed(kids[i]);
	}
    }
} /* alignCaptions */

/*----------------------------------------------------------------------*/

static Widget
captionLabel(Widget caption)
{

    /* This is based on the assumption that the first child of a
       caption is always the XmLabel widget. */
    WidgetList	kids;

    XtVaGetValues(caption, XmNchildren, &kids, NULL);

    return kids[0];
} /* captionLabel */

/*----------------------------------------------------------------------*/

/* Make the sash the full width of the window. This looks better than the
   square that is the default. */

static void
resizePwinCb(Widget w, XtPointer clientdata, XEvent *event, Boolean *cont)
{
    int		width;

    if(event->type == ConfigureNotify || event->type == MapNotify){
	XtVaGetValues(w, XmNwidth, &width, NULL);
	XtVaSetValues(w, XmNsashWidth, width, NULL);
    }
    *cont = True;
} /* resizePwinCb */

/*----------------------------------------------------------------------*/

/* Fill the comboBox with the folders listed in the "filemenu2" mailrc
   variable. */
void
populate_combo(Widget combo)
{
    char	*maildirs, *p;
    Widget	list = XtNameToWidget(combo, "*List");
    XmString	entry;

    XmListDeleteAllItems(list);
    p = find_mailrc("filemenu2");

    if(p == NULL){
	return;
    }
    maildirs = strdup(p);

    if((p = strtok(maildirs, " \t")) == NULL){
	free(maildirs);
	return;
    }

    do{
	entry = XmStringGenerate(p, NULL, XmCHARSET_TEXT, (XmStringTag)"LIST");
	XmListAddItem(list, entry, 0);
	XmStringFree(entry);
    }while(p = strtok(NULL, " \t"));

    free(maildirs);
} /* populate_combo */

/*----------------------------------------------------------------------*/

static void
folderCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    FOLDER_ACTIONS	action = (FOLDER_ACTIONS)clientdata;
    Widget		text = XtNameToWidget(fold_combo_, "*Text");
    char		*folder, *fullname, mess[BUFSIZ];
    int			n = 0;
    MESSAGE		*m;

    XtVaGetValues(text, XmNvalue, &folder, NULL);
    if(folder == NULL || *folder == '\0')
	return;

    if(action == FOLDER_LOAD){
	deleteAllMessages();
	load_file_proc(folder);
	display_new_message();
	sync_list();
	update_message_list();
	return;
    }

    /* Take the relevant bits from gui.c, because we want to handle
       the items selected in the List rather than relying in m->selected. */

    fullname = expand_filename(folder);
    
    m = messages.start;
    while (m) {
	if(XmListPosSelected(mailslist_, m->list_pos)){
	    if (!append_message_to_file (m, fullname, FALSE)) {
		n++;
	    }
	    else if (!failed_save_notice_proc()) {
		return;
	    }
	}
	m = m->next;
    }
    if (!n)
	set_main_footer("No messages saved.");
    else {
	sprintf(mess,"%d messages %s to %s",
		n, (action == FOLDER_MOVE)? "moved": "saved", fullname);
	set_main_footer(mess);
    }

    if(action == FOLDER_MOVE){
	delete_message_proc();
    }
} /* folderCb */

/*----------------------------------------------------------------------*/

static void
saveCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    save_changes_proc();
    display_new_message();
    sync_list();
    update_message_list();
} /* saveCb */

/*----------------------------------------------------------------------*/

void
set_foldwin_toggles(Boolean set)
{
    XmToggleButtonSetState(foldwin_[0], set, False);
    XmToggleButtonSetState(foldwin_[1], set, False);
} /* set_foldwin_toggles */

/*----------------------------------------------------------------------*/

void
update_combo(char *path)
{
    Widget	text = XtNameToWidget(fold_combo_, "*Text");

    XtVaSetValues(text, XmNvalue, path, NULL);
} /* update_combo */

/*----------------------------------------------------------------------*/

static void
display_new_message()
{
    MESSAGE	*m;
    int		numVisible;

    /* Find first unread message. */
    for(m = messages.start; m != NULL; m = m->next){
	if(m->status == MSTAT_UNREAD)
	    break;
    }

    if(m == NULL){		/* No unread messages, display last. */
	display_message(last_message_read = messages.end);
    }
    else{			/* Got one so display it and position
				   list nicely. */
	display_message(last_message_read = m);
	XtVaGetValues(mailslist_, XmNvisibleItemCount, &numVisible, NULL);
	if(m->list_pos < (messages.number - numVisible)){
	    XtVaSetValues(mailslist_, XmNtopItemPosition, m->list_pos, NULL);
	}
	else{
	    XtVaSetValues(mailslist_, XmNtopItemPosition,
			  (messages.number - numVisible) + 1, NULL);
	}
    }
} /* display_new_message */

/*----------------------------------------------------------------------*/

static void
textConvertCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    XmConvertCallbackStruct *cbs = (XmConvertCallbackStruct *)calldata;

    DEBUG1(("textConvertCb target = %s\n", XmGetAtomName(XtDisplay(w),
							 cbs->target)));

    cbs->status = XmCONVERT_DEFAULT;
} /* textConvertCb */

/*----------------------------------------------------------------------*/

static void
listConvertCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    String	name, seln;
    Display	*display = XtDisplay(w);
    XmConvertCallbackStruct *cbs = (XmConvertCallbackStruct *)calldata;

    Atom	PRIVTOOL_SENDER =
	XmInternAtom(display, "PRIVTOOL_SENDER", False);
    Atom	PRIVTOOL_MESSAGE =
	XmInternAtom(display, "PRIVTOOL_MESSAGE", False);
    Atom	TARGETS = XmInternAtom(display, XmSTARGETS, False);
    Atom	ME_TARGETS = 
	XmInternAtom(display, XmS_MOTIF_EXPORT_TARGETS, False);
    MESSAGE	*m;
    BUFFER	*b;
    WidgetList	items;
    int		numitems;

    DEBUG1(("listConvertCb target = %s\n", XmGetAtomName(XtDisplay(w),
							 cbs->target)));
    if(cbs->target == TARGETS ||
       cbs->target == ME_TARGETS){

        Atom *targs;
        int target_count = 0;

	if (cbs->target == ME_TARGETS) 
	    targs = (Atom *)XtMalloc((unsigned) (2 * sizeof(Atom)));
	else
	    targs = XmeStandardTargets(w, 2, &target_count);

        targs[target_count++] = PRIVTOOL_SENDER;
        targs[target_count++] = PRIVTOOL_MESSAGE;

        cbs->value = (XtPointer) targs;
	cbs->length = target_count;
	cbs->format = 32;
	cbs->type = XA_ATOM;
	cbs->status = XmCONVERT_MERGE;
    }
    else if(cbs->target == PRIVTOOL_MESSAGE){
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
    }
    else if(cbs->target == PRIVTOOL_SENDER){
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
	    cbs->value = m->sender;
	    cbs->length = strlen(m->sender);
	    cbs->type = XA_STRING;
	    cbs->format = 8;
	    cbs->status = XmCONVERT_DONE;
	}
    }
    else{
	cbs->status = XmCONVERT_DEFAULT;
    }
} /* listConvertCb */

/*----------------------------------------------------------------------*/

static void
destnCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    XmDestinationCallbackStruct *cbs = (XmDestinationCallbackStruct *)calldata;

    Atom	TARGETS = XmInternAtom(XtDisplay(w), XmSTARGETS, False);
    DEBUG1(("destnCb  selection = %s\n", XmGetAtomName(XtDisplay(w),
						    cbs->selection)));
    if(w == mailslist_){
	DEBUG2(("   got mailslist_ destnCb\n"));
    }
    else{
	XmTransferValue(cbs->transfer_id, TARGETS,
			(XtCallbackProc) transferProc,
			NULL, XtLastTimestampProcessed(XtDisplay(w)));
    }
} /* destnCb */

/*----------------------------------------------------------------------*/

static void
transferProc(Widget w, XtPointer clientdata, XtPointer calldata)
{
    XmSelectionCallbackStruct *cbs = (XmSelectionCallbackStruct *)calldata;
    Boolean	domsg = False, dosndr = False;
    Display	*display = XtDisplay(w);
    int		i;
    Atom	*targets = (Atom *)cbs->value;

    Atom	TARGETS = XmInternAtom(display, XmSTARGETS, False);
    Atom	PRIVTOOL_SENDER = XmInternAtom(display,
						"PRIVTOOL_SENDER", False);
    Atom	PRIVTOOL_MESSAGE = XmInternAtom(display,
						"PRIVTOOL_MESSAGE", False);
    char	*buf;

    DEBUG1(("transferProc  target = %s\n", XmGetAtomName(XtDisplay(w),
							 cbs->target)));
    if((cbs->target == TARGETS) && (cbs->type == XA_ATOM)){
	for(i = 0; i < cbs->length; i++){
	    if(targets[i] == PRIVTOOL_MESSAGE){
		domsg = True;
	    }
	    if(targets[i] == PRIVTOOL_SENDER){
		dosndr = True;
	    }
	}

	if(domsg && dosndr){
	    XmTransferValue(cbs->transfer_id, PRIVTOOL_SENDER,
			    (XtCallbackProc)transferProc, NULL,
			    XtLastTimestampProcessed(display));
	}
	else{
	    DEBUG2(("  in default section.\n"));
	    /*XmTransferDone(cbs->transfer_id, XmTRANSFER_DONE_DEFAULT);*/
	}
    }
    else if((cbs->target == PRIVTOOL_SENDER) && (cbs->type == XA_STRING)){
	buf = XtMalloc(cbs->length + 1);
	strncpy(buf, cbs->value, cbs->length);
	buf[cbs->length] = '\0';
	XmTextInsert(w, XmTextGetInsertionPosition(w), buf);
	XmTextInsert(w, XmTextGetInsertionPosition(w), attribution_string);
	XtFree(buf);

	/* Have got the sender info, now get the actual message. */
	XmTransferValue(cbs->transfer_id, PRIVTOOL_MESSAGE,
			(XtCallbackProc)transferProc, NULL,
			XtLastTimestampProcessed(display));
    }
    else if((cbs->target == PRIVTOOL_MESSAGE) && (cbs->type == XA_STRING)){
	buf = XtMalloc(cbs->length + 1);
	strncpy(buf, cbs->value, cbs->length);
	buf[cbs->length] = '\0';
	insert_message(w, buf);
	XtFree(buf);
	XmTransferDone(cbs->transfer_id, XmTRANSFER_DONE_SUCCEED);
    }
} /* transferProc */

/*----------------------------------------------------------------------*/

static void
filerCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    Widget	filer;

    filer = XmCreateFileSelectionDialog(toplevel_, "filer", NULL, 0);
    XtUnmanageChild(XtNameToWidget(filer, "*Help"));
    XtAddCallback(filer, XmNokCallback,
		  (XtCallbackProc)insert_file, clientdata);
    XtAddCallback(filer, XmNcancelCallback,
		  (XtCallbackProc)insert_file, NULL);
    XtManageChild(filer);
} /* filerCb */

/*----------------------------------------------------------------------*/

static void
insert_file(Widget w, XtPointer clientdata, XtPointer calldata)
{
    XmFileSelectionBoxCallbackStruct *cbs =
	(XmFileSelectionBoxCallbackStruct *)calldata;

    Widget	text = XtNameToWidget(w, "*Text"), textbox = clientdata;
    int		fd;
    char	*name, *buf;
    struct stat	stbuf;

    XtUnmanageChild(w);
    if(cbs->reason == XmCR_OK && text != NULL){
	XtVaGetValues(text, XmNvalue, &name, NULL);
	DEBUG2(("  inserting file %s ...\n", name));
	stat(name, &stbuf);
	if(S_ISREG(stbuf.st_mode)){
	    if((fd = open(name, O_RDONLY)) == -1){
		alert(toplevel_, "fileerror", 1);
	    }
	    else{
		buf = mmap(NULL, stbuf.st_size, PROT_READ, MAP_SHARED, fd, 0L);
		if(buf == (char *)-1){
		    alert(toplevel_, "mmapfailed", 1);
		}
		else{
		    XmTextInsert(textbox, XmTextGetInsertionPosition(textbox),
				 buf);
		    munmap(buf, stbuf.st_size);
		}
		close(fd);
	    }
	}
	else{
	    alert(toplevel_, "notregfile", 1);
	    DEBUG2(("not a regular file.\n"));
	}
    }
    XtDestroyWidget(w);
} /* insert_file */

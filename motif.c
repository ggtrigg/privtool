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
#include	"windows.h"
#include	"gui.h"
#include	"motif_protos.h"
#include	<Xm/DialogS.h>
#include	<Xm/MainW.h>
#include	<Xm/RowColumn.h>
#include	<Xm/Frame.h>
#include	<Xm/Form.h>
#include	<Xm/CascadeB.h>
#include	<Xm/PanedW.h>
#include	<Xm/PushB.h>
#include	<Xm/PushBG.h>
#include	<Xm/List.h>
#include	<Xm/ScrolledW.h>
#include	<Xm/Separator.h>
#include	<Xm/TextF.h>
#include	<Xm/Text.h>
#include	<Xm/ToggleBG.h>
#include	<Xm/SeparatoG.h>
#include	<Xm/CSText.h>
#include	<Xm/MessageB.h>
#include	<X11/Xmu/Editres.h>
#include	"mfolder.h"

#include	"delete.xpm"

void			update_message_list(void);
int			show_deleted = 1;

static Widget		toplevel_, msgarea_, msgarea2_, mailslist_;
static Widget		text_ = NULL, hdrtext_;
static Widget		pp_window_ = NULL, abt_window_ = NULL;
static XtAppContext	app_context_;
static XmRenderTable	render_header, render_list;
static char		local_pp[2048];

typedef enum {
    COMPOSE_NEW,
    COMPOSE_REPLY,
    COMPOSE_FORWARD,
    COMPOSE_RESEND
} COMPOSE_TYPE;

static void		create_rendertables(Widget parent);
static void		create_menubar(Widget parent);
static void		create_toolbar(Widget parent);
static void		create_file_menu(Widget parent);
static void		create_edit_menu(Widget parent);
static void		create_view_menu(Widget parent);
static void		create_comp_menu(Widget parent);
static void		create_help_menu(Widget parent);
static void		create_workarea (Widget parent);
static void		create_msgareas(Widget parent);
static COMPOSE_WINDOW	*setup_composeCB(Widget w, XtPointer clientdata, XtPointer calldata);
static void		update_mail_list(void);
static void		show_message(Widget w, XtPointer, XmListCallbackStruct *cbs);
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
    if(text_ != NULL && XtIsRealized(text_))
	XmTextSetString(text_, "");
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
    return 0;
}

/*----------------------------------------------------------------------*/

DISPLAY_WINDOW *
create_display_window()
{
#if 0
    Widget	swin, pwin, tempw;

    if(message_window_ == NULL){
	message_window_ = XtVaCreatePopupShell("message",
					       topLevelShellWidgetClass,
					       toplevel_,
					       NULL);
	XtManageChild(message_window_);

	pwin = XmCreatePanedWindow(message_window_, "panedwin", NULL, 0);
	XtManageChild(pwin);

#if 0
	text_ = XmCreateScrolledText(pwin, "msgtext", NULL, 0);
	XtManageChild(text_);
#endif
	hdrtext_ = XmCreateScrolledCSText(pwin, "hdrtext", NULL, 0);
	XtManageChild(hdrtext_);
	tempw = XmCreateScrolledText(pwin, "sigtext", NULL, 0);
	XtManageChild(tempw);

	/*XtVaSetValues(hdrtext_, XmNrenderTable, render_header, 0);*/
    }

    XtPopup(message_window_, XtGrabNonexclusive);
#endif
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

    XtVaGetValues(mailslist_, XmNselectedPositionCount, &sel_pos_count, NULL);
    sel_posns = calloc(sel_pos_count, sizeof(int));
    XtVaGetValues(mailslist_, XmNselectedPositions, &sel_posns, NULL);

    while (m) {
	if (m->list_pos == *sel_posns) {
	    message_number = m->list_pos;
	    XmListDeleteItemsPos(mailslist_, 1, message_number);
	    if(show_deleted){
		XmListAddItem(mailslist_,
			      tempstring = XmStringGenerate(m->description, NULL,
							    XmCHARSET_TEXT,
							    (XmStringTag)"LIST_ST"),
			      message_number);
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
display_message_body(BUFFER *b)
{
    XtVaSetValues(text_, XmNvalue, b->message, NULL);
}

/*----------------------------------------------------------------------*/

void
display_message_description(MESSAGE *m)
{
    XmString	new_string;

    if(m->flags & MESS_DELETED){
	new_string = XmStringGenerate(m->description, NULL,
				      XmCHARSET_TEXT,
				      (XmStringTag)"LIST_ST");
    }else{
	new_string = XmStringGenerate(m->description, NULL,
				      XmCHARSET_TEXT,
				      (XmStringTag)"LIST");
    }
    XmListReplaceItemsPos(mailslist_, &new_string, 1, m->list_pos);
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
#if 0
    XmString	info;

    info = XmStringGenerate("From: ", NULL, XmCHARSET_TEXT,
			    (XmStringTag)"HDR_B");
    info = XmStringConcatAndFree(info,
				 XmStringGenerate(m->sender, NULL,
						  XmCHARSET_TEXT,
						  (XmStringTag)"HDR"));
    info = XmStringConcatAndFree(info,
				 XmStringGenerate("\nSubject: ", NULL,
						  XmCHARSET_TEXT,
						  (XmStringTag)"HDR_B"));
    info = XmStringConcatAndFree(info,
				 XmStringGenerate(m->subject, NULL,
						  XmCHARSET_TEXT,
						  (XmStringTag)"HDR_U"));
    info = XmStringConcatAndFree(info,
				 XmStringGenerate("\nDate", NULL,
						  XmCHARSET_TEXT,
						  (XmStringTag)"HDR_B"));
    info = XmStringConcatAndFree(info,
				 XmStringGenerate(m->date, NULL,
						  XmCHARSET_TEXT,
						  (XmStringTag)"HDR"));
    XmCSTextReplace(hdrtext_, 0, XmCSTextGetLastPosition(hdrtext_), info);

    XmStringFree(info);
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
}

/*----------------------------------------------------------------------*/

char *
read_cc(COMPOSE_WINDOW *w)
{
}

/*----------------------------------------------------------------------*/

int
read_deliver_flags(COMPOSE_WINDOW *w)
{
}

/*----------------------------------------------------------------------*/

char *
read_extra_headerline(COMPOSE_WINDOW *w, int i)
{
}

/*----------------------------------------------------------------------*/

void
read_message_to_deliver(COMPOSE_WINDOW *w, BUFFER *b)
{
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
}

/*----------------------------------------------------------------------*/

char *
read_subject(COMPOSE_WINDOW *w)
{
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
    char	*title;
    XImage	*img, *shp_img;

    toplevel_ = XtVaOpenApplication(&app_context_, "Privtool", NULL, 0,
				    &argc, argv, NULL,
				    applicationShellWidgetClass, 0);

#if 0
    XpmCreateImageFromData(XtDisplay(toplevel_),
			    &delete_xpm, &img, &shp_img, NULL);
    XmInstallImage(img, "delete.xpm");
#endif

    title = (char *)malloc(strlen(prog_name) + strlen(prog_ver) + 5);
    strcpy(title, prog_name);
    strcat(title, " - ");
    strcat(title, prog_ver);
    XtVaSetValues(toplevel_, XmNtitle, title, NULL);

    /* Add editres protocol support */
    XtAddEventHandler(toplevel_, 0, True, _XEditResCheckMessages, NULL);

    XtAppAddActions(app_context_, actions, XtNumber(actions));

    control_ = XmCreateMainWindow (toplevel_, "main_window", NULL, 0);
    XtManageChild (control_);

    create_rendertables(control_);
    create_menubar(control_);
    create_toolbar(control_);
    create_workarea(control_);
    create_msgareas(control_);

    display_message(last_message_read = messages.start);
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
setup_composeCB(Widget w, XtPointer clientdata, XtPointer calldata)
{
    COMPOSE_WINDOW	*win = NULL;

#if 0
    /* See if we have a window we can use */
    win = compose_find_free();
#endif

    if(win == NULL){
	win = XtNew(COMPOSE_WINDOW);
	win->deliver_frame = XtVaCreatePopupShell("compose",
						topLevelShellWidgetClass,
						toplevel_,
						NULL);
	XtManageChild(win->deliver_frame);
    }
    XtPopup(win->deliver_frame, XtGrabNonexclusive);

    return win;
}

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
    create_comp_menu(menubar_);
    create_help_menu(menubar_);
} /* create_menubar */

/*----------------------------------------------------------------------*/

static void
create_toolbar(Widget parent)
{
    Widget	toolbar_, btn, frame;

    frame = XmCreateFrame(parent, "toolframe", NULL, 0);
    XtManageChild(frame);
    toolbar_ = XmCreateForm(frame, "toolbar", NULL, 0);
    XtManageChild(toolbar_);
    XtVaSetValues (parent, XmNcommandWindow, frame, NULL);

    btn = XmCreatePushButtonGadget(toolbar_, "prev", NULL, 0);
    XtManageChild(btn);
    XtAddCallback (btn, XmNactivateCallback,
		       prev_messageCB, NULL);
    btn = XmCreatePushButtonGadget(toolbar_, "next", NULL, 0);
    XtManageChild(btn);
    XtAddCallback (btn, XmNactivateCallback,
		       next_messageCB, NULL);
    btn = XmCreatePushButtonGadget(toolbar_, "delete", NULL, 0);
    XtManageChild(btn);
    XtAddCallback (btn, XmNactivateCallback,
		       delete_message_proc, NULL);
    btn = XmCreatePushButtonGadget(toolbar_, "undelete", NULL, 0);
    XtManageChild(btn);
    XtAddCallback (btn, XmNactivateCallback,
		       undeleteCB, NULL);
} /* create_toolbar */

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
		   load_new_mail, NULL);

    button_ = XmCreatePushButtonGadget (menu_, "save", NULL, 0);
    XtManageChild (button_);

    button_ = XmCreatePushButtonGadget (menu_, "done", NULL, 0);
    XtManageChild (button_);

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

    button_ = XmCreateToggleButtonGadget (menu_, "folders", NULL, 0);
    XtManageChild (button_);
    XtAddCallback (button_, XmNvalueChangedCallback,
		   view_foldersCB, NULL);
} /* create_view_menu */

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

    mailslist_ = XmCreateScrolledList(pwin, "slist", NULL, 0);

    XtVaSetValues(mailslist_, XmNrenderTable, render_list, 0);
    XtAddCallback(mailslist_, XmNdefaultActionCallback,
		  (XtCallbackProc)show_message, NULL);
#if 0
    XtAddCallback(mailslist_, XmNsingleSelectionCallback,
		  (XtCallbackProc)select_message, NULL);
    XtAddCallback(mailslist_, XmNbrowseSelectionCallback,
		  (XtCallbackProc)select_message, NULL);
#endif
    XtManageChild (mailslist_);

    text_ = XmCreateScrolledText(pwin, "msgtext", NULL, 0);
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
		      tempstring = XmStringGenerate(m->description, NULL, XmCHARSET_TEXT,
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
				  (XmStringTag)"LIST_B");

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

static void
create_rendertables(Widget parent)
{
    XmRendition		rarray[5], rlist[3];

    rarray[0] = XmRenditionCreate(parent, (XmStringTag)"HDR", NULL, 0);
    rarray[1] = XmRenditionCreate(parent, (XmStringTag)"HDR_B", NULL, 0);
    rarray[2] = XmRenditionCreate(parent, (XmStringTag)"HDR_U", NULL, 0);
    rarray[3] = XmRenditionCreate(parent, (XmStringTag)"BIG", NULL, 0);
    rarray[4] = XmRenditionCreate(parent, (XmStringTag)"BLUE", NULL, 0);
    rlist[0] = XmRenditionCreate(parent, (XmStringTag)"LIST", NULL, 0);
    rlist[1] = XmRenditionCreate(parent, (XmStringTag)"LIST_ST", NULL, 0);
    rlist[2] = XmRenditionCreate(parent, (XmStringTag)"LIST_B", NULL, 0);

    /*render_header = XmRenderTableAddRenditions(NULL, rarray, 5, XmREPLACE);*/
    render_list = XmRenderTableAddRenditions(NULL, rlist, 3, XmREPLACE);
} /* create_rendertables */

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
    if(XmToggleButtonGadgetGetState(w)){
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

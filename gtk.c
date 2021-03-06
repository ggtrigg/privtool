/* $Id$ 
 *
 * Copyright   : (c) 1999 by Glenn Trigg.  All Rights Reserved
 * Project     : Privtool - Gtk+ interface
 * File        : gtk
 *
 * Author      : Glenn Trigg
 * Created     : 11 Feb 1999 
 *
 * Description : Gtk+ implementation of Privtool GUI.
 */

#define UI_MAIN

#ifdef HAVE_CONFIG_H
#include	"config.h"
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<unistd.h>
#include	<string.h>
#include	<time.h>
#include	<gtk/gtk.h>
#include	<dirent.h>
#include	<sys/stat.h>
#include	<errno.h>
#include	<fcntl.h>
#include	<sys/mman.h>

#include	"def.h"
#include	"buffers.h"
#include	"message.h"
#include	"mailrc.h"
#include	"windows.h"
#include	"gui.h"
#include	"gtk_protos.h"
#include	"pgplib.h"
#include	"mail_reader.h"
#include	"main.h"

#include	"images/privtool_empty.xpm"
#include	"images/privtool_new.xpm"
#include	"images/prev.xpm"
#include	"images/next.xpm"
#include	"images/delete.xpm"
#include	"images/undelete.xpm"
#include	"images/compose.xpm"
#include	"images/reply.xpm"
#include	"images/folderwin.xpm"
#include	"images/g_letter.xpm"
#include	"images/mini-folder.xpm"
#include	"images/mini-ofolder.xpm"
#include	"images/question.xpm"
#include	"images/exclamation.xpm"
#include	"images/exclamation2.xpm"
#include	"images/privtool-logo-lc.xpm"

/* Typedefs */
typedef enum {
    COMPOSE_NEW = 0x01,
    COMPOSE_REPLY = 0x02,
    COMPOSE_FORWARD = 0x04,
    COMPOSE_RESEND = 0x08,
    R_SENDER = 0x10,
    R_SENDER_INCLUDE = 0x20,
    R_ALL = 0x40,
    R_ALL_INCLUDE = 0x80
} COMPOSE_TYPE;

typedef enum {
    EDIT_CUT,
    EDIT_COPY,
    EDIT_PASTE,
    EDIT_QUOTE,
    EDIT_WRAP
} EDIT_OP;

/*extern int		errno;*/
extern char		*our_userid;
extern char		*current_nym(void);

/* Static functions */
static COMPOSE_WINDOW *compose_find_free(void);
static GtkWidget *create_menubar(void);
static GtkWidget *create_msglist(void);
static GtkWidget *create_toolbar(GtkWidget *);
static GtkWidget *get_pixmap(GtkWidget *, gchar **);
static char	*extract_addresses(char *);
static gint	alert_delete_cb(GtkWidget *, GdkEvent *, gpointer);
static gint	clist_sort_cmp(GtkCList *, gconstpointer, gconstpointer);
static gint	compmenu_post_cb(GtkWidget *, GdkEventButton *event);
static gint	delete_event(GtkWidget *, GdkEvent *, gpointer);
static void	about_cb(GtkWidget *, gpointer);
static void	about_close_cb(GtkWidget *, gpointer);
static void	add_key_cb(GtkWidget *, gpointer);
static void	add_signature(COMPOSE_WINDOW *);
static void	alert_cb(GtkWidget *, gpointer);
static void	clist_sort_cb(GtkWidget *, gint, gpointer);
static void	compmenu_cb(GtkWidget *, gpointer);
static void	compose_cb(GtkWidget *, gpointer);
static void	deliver_cb(GtkWidget *, gpointer);
static void	destroy (GtkWidget *, gpointer);
static void	display_new_message(void);
static void	edit_ops_cb(GtkWidget *, gpointer);
static void	filer_cb(GtkWidget *, gpointer);
static void	fillin_folders(GtkWidget *);
static void	folder_select_cb(GtkWidget *, GtkCTreeNode *, gint);
static void	hide_addkey(void);
static void	hide_attach(void);
static void	initialise_compose_win(COMPOSE_WINDOW *, COMPOSE_TYPE);
static void	insert_file_cb(GtkWidget *, gpointer);
static void	load_folder_cb(GtkWidget *, gpointer);
static void	load_new_cb(GtkWidget *, gpointer);
static void	move_to_folder_cb(GtkWidget *, gpointer);
static void	msgdrop_cb(GtkWidget *, GdkDragContext *, gint, gint, guint, gpointer);
static void	next_msg_cb(GtkWidget *, gpointer);
static void	populate_combo(void);
static void	prev_msg_cb(GtkWidget *, gpointer);
static void	print_cb(GtkWidget *, gpointer);
static void	process_dir(GtkWidget *, char *, GtkCTreeNode *);
static void	reset_deliver_flags(COMPOSE_WINDOW *);
static void	save_cb(GtkWidget *, gpointer);
static void	select_msg_cb(GtkWidget *, gint, gint, GdkEvent *);
static void	show_deleted_cb(GtkWidget *, gpointer);
static void	show_folder_win_cb(GtkWidget *, gpointer);
static void	show_fullhdr_cb(GtkWidget *, gpointer);
static void	show_undelete(gboolean);
static void	sort_cb(GtkWidget *, gpointer);
static void	textins_cb(GtkWidget *, gchar *, gint, gint *);
static void	toggle_toolbar_cb(GtkWidget *, gpointer);
static void	undelete_cb(GtkWidget *, gpointer);
static void	update_mail_list(void);

/* Static data */
static GtkWidget	*toplevel;
static COMPOSE_WINDOW	*compose_first = NULL;
static COMPOSE_WINDOW	*compose_last = NULL;
static gboolean		full_header = 0;
static char		attribution_string[] = " said :\n\n";
static char		begin_forward[] = "-- Begin forwarded message ---\n";
static char		end_forward[] = "\n-- End forwarded message ---\n";
static GtkTargetEntry	targets[] = {
    {"mailitem", GTK_TARGET_SAME_APP, 100}};

static GtkItemFactoryEntry ife[] = {
    {"/File/--", NULL, NULL, 0, "<Tearoff>"},
    {"/File/Load Inbox", "<control>L", (GtkItemFactoryCallback)load_new_cb, 0, NULL},
    {"/File/Save", "<control>S", (GtkItemFactoryCallback)save_cb, 0, NULL},
    {"/File/Print", "<control>P", (GtkItemFactoryCallback)print_cb, 0, NULL},
    {"/File/Done", NULL, (GtkItemFactoryCallback)done_proc, 0, NULL},
    {"/File/-", NULL, NULL, 0, "<Separator>"},
    {"/File/Save & Quit", "<control>Q", (GtkItemFactoryCallback)save_and_quit_proc, 0, NULL},
    {"/File/Quit", "<control>C", (GtkItemFactoryCallback)quit_proc, 0, NULL},
    {"/Edit/--", NULL, NULL, 0, "<Tearoff>"},
    /* {"/Edit/Cut", NULL, edit_ops_cb, EDIT_CUT, NULL}, */
    {"/Edit/Copy", NULL, (GtkItemFactoryCallback)edit_ops_cb, EDIT_COPY, NULL},
    {"/Edit/Delete", "<control>D", (GtkItemFactoryCallback)delete_message_proc, 0, NULL},
    {"/Edit/Undelete", "<control>U", (GtkItemFactoryCallback)undelete_cb, 0, NULL},
    {"/Edit/Undelete Last", NULL, (GtkItemFactoryCallback)undelete_last_proc, 0, NULL},
    {"/Edit/-", NULL, NULL, 0, "<Separator>"},
    {"/Edit/Add Key", NULL, (GtkItemFactoryCallback)add_key_cb, 0, NULL},
    {"/Edit/Show Attachment", NULL, NULL, 0, NULL},
    {"/Edit/Clear Passphrase", NULL, NULL, 0, NULL},
    {"/Edit/Properties", "<control>E", (GtkItemFactoryCallback)show_props, 0, NULL},
    {"/View/--", NULL, NULL, 0, "<Tearoff>"},
    {"/View/Next", "<alt>N", (GtkItemFactoryCallback)next_msg_cb, 0, NULL},
    {"/View/Previous", "<alt>P", (GtkItemFactoryCallback)prev_msg_cb, 0, NULL},
    {"/View/Sort By/Time & Date", NULL, (GtkItemFactoryCallback)sort_cb, 3, NULL},
    {"/View/Sort By/Sender", NULL, (GtkItemFactoryCallback)sort_cb, 2, NULL},
    {"/View/Sort By/Subject", NULL, (GtkItemFactoryCallback)sort_cb, 5, NULL},
    {"/View/Sort By/Size", NULL, (GtkItemFactoryCallback)sort_cb, 4, NULL},
    {"/View/Sort By/Status", NULL, (GtkItemFactoryCallback)sort_cb, 0, NULL},
    {"/View/Sort By/Message", NULL, (GtkItemFactoryCallback)sort_cb, 1, NULL},
    {"/View/-", NULL, NULL, 0, "<Separator>"},
    {"/View/Show Deleted", NULL, (GtkItemFactoryCallback)show_deleted_cb, 0, "<CheckItem>"},
    {"/View/Full Header", "<control>H", (GtkItemFactoryCallback)show_fullhdr_cb, 0, "<CheckItem>"},
    {"/View/Show Toolbar", "<control>T", (GtkItemFactoryCallback)toggle_toolbar_cb, 0, "<CheckItem>"},
    {"/View/Folders", "<control>F", (GtkItemFactoryCallback)show_folder_win_cb, 0, "<ToggleItem>"},
    {"/Folder/--", NULL, NULL, 0, "<Tearoff>"},
    {"/Folder/Copy to Folder", "<alt>C", (GtkItemFactoryCallback)move_to_folder_cb, 0, NULL},
    {"/Folder/Move to Folder", "<alt>M", (GtkItemFactoryCallback)move_to_folder_cb, 1, NULL},
    {"/Folder/Load Folder", "<alt>L", (GtkItemFactoryCallback)load_folder_cb, 0, NULL},
    {"/Folder/Load Inbox", NULL, (GtkItemFactoryCallback)load_new_cb, 0, NULL},
    {"/Compose/--", NULL, NULL, 0, "<Tearoff>"},
    {"/Compose/New", NULL, (GtkItemFactoryCallback)compose_cb, COMPOSE_NEW, NULL},
    {"/Compose/Reply/Sender", NULL, (GtkItemFactoryCallback)compose_cb,
				COMPOSE_REPLY|R_SENDER, NULL},
    {"/Compose/Reply/Sender (include)", NULL, (GtkItemFactoryCallback)compose_cb,
				COMPOSE_REPLY|R_SENDER_INCLUDE, NULL},
    {"/Compose/Reply/All", NULL, (GtkItemFactoryCallback)compose_cb,
				COMPOSE_REPLY|R_ALL, NULL},
    {"/Compose/Reply/All (include)", NULL, (GtkItemFactoryCallback)compose_cb,
				COMPOSE_REPLY|R_ALL_INCLUDE, NULL},
    {"/Compose/Forward", "<alt>F", (GtkItemFactoryCallback)compose_cb, COMPOSE_FORWARD, NULL},
    {"/Compose/Resend", "<alt>R", (GtkItemFactoryCallback)compose_cb, COMPOSE_RESEND, NULL},
    {"/Help/--", NULL, NULL, 0, "<Tearoff>"},
    {"/Help/About", "<control>A", (GtkItemFactoryCallback)about_cb, 0, NULL}
};

static GtkItemFactoryEntry comp_ife[] = {
    {"/Cut", "<control>X", (GtkItemFactoryCallback)compmenu_cb, EDIT_CUT, NULL},
    {"/Copy", "<control>C", (GtkItemFactoryCallback)compmenu_cb, EDIT_COPY, NULL},
    {"/Paste", "<control>V", (GtkItemFactoryCallback)compmenu_cb, EDIT_PASTE, NULL},
    {"/--", NULL, NULL, 0, "<Separator>"},
    {"/Quote", "<control>Q", (GtkItemFactoryCallback)compmenu_cb, EDIT_QUOTE, NULL},
    {"/Wrap Lines", "<control>W", (GtkItemFactoryCallback)compmenu_cb, EDIT_WRAP, NULL},
};

/*----------------------------------------------------------------------*/

void
setup_ui(int level, int argc, char **argv)
{
    gchar		*title, *gtkrc_path, *f;
    guint		check_interval;
    guint32		interval;
    GdkColor		delcol;
    GtkWidget		*vbox, *mbar, *tbar, *vpane, *msglist, *swin, *txt,
			*statbar;
    GtkWidget		*hb;
    GtkStyle		*cl_style;
    GtkItemFactory	*ifactory;

    show_deleted = 1;		/* Default value. */
    title = g_strconcat(prog_name, " - ", prog_ver, NULL);

    gtkrc_path = g_strconcat(g_get_home_dir(), "/.privtool/gtkrc", NULL);
    gtk_rc_add_default_file(gtkrc_path);
    g_free(gtkrc_path);

#ifdef DATADIR
    gtkrc_path = g_strconcat(DATADIR, "/gtk/privtool/gtkrc", NULL);
    gtk_rc_add_default_file(gtkrc_path);
    g_free(gtkrc_path);
#endif

    gtk_init(&argc, &argv);

    toplevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_widget_set_name(toplevel, "Privtool");
    gtk_window_set_title(GTK_WINDOW(toplevel), title);
    gtk_window_set_policy(GTK_WINDOW(toplevel), 1, 1, 1);
    g_free(title);

    gtk_signal_connect(GTK_OBJECT(toplevel), "delete_event",
		       GTK_SIGNAL_FUNC (delete_event), NULL);

    gtk_signal_connect(GTK_OBJECT(toplevel), "destroy",
		       GTK_SIGNAL_FUNC (destroy), NULL);

    vbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add(GTK_CONTAINER(toplevel), vbox);

    mbar = create_menubar();
    gtk_box_pack_start(GTK_BOX(vbox), mbar, FALSE, FALSE, 0);

    /* So that the pixmaps have a window to belong to. */
    gtk_widget_realize(toplevel);
    tbar = create_toolbar(toplevel);
    gtk_box_pack_start(GTK_BOX(vbox), tbar, FALSE, FALSE, 0);

    /* Create vertical paned window. */
    vpane = gtk_vpaned_new();
    gtk_container_set_border_width(GTK_CONTAINER(vpane), 2);

    /* List of mail header's window. */
    msglist = create_msglist();
    gtk_paned_add1(GTK_PANED(vpane), msglist);

    /* Create message text area. (In a handlebox so it can be detached.)
     */
    hb = gtk_handle_box_new();
    gtk_handle_box_set_handle_position(GTK_HANDLE_BOX(hb), GTK_POS_TOP);
    gtk_handle_box_set_snap_edge(GTK_HANDLE_BOX(hb), GTK_POS_TOP);
    gtk_container_set_border_width(GTK_CONTAINER(hb), 2);
    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(hb), swin);
    txt = gtk_text_new(NULL, NULL);
    gtk_widget_set_usize(GTK_WIDGET(txt), 550, 450);
    gtk_text_set_editable(GTK_TEXT(txt), FALSE);
    gtk_object_set_data(GTK_OBJECT(toplevel), "msgtext", (gpointer)txt);
    gtk_container_add(GTK_CONTAINER(swin), txt);
    gtk_widget_show(swin);
    gtk_widget_show(txt);
    gtk_widget_show(hb);
    gtk_paned_add2(GTK_PANED(vpane), hb);

    gtk_widget_show(vpane);
    gtk_box_pack_start(GTK_BOX(vbox), vpane, TRUE, TRUE, 0);

    /* Create statusbar for the bottom of the window. */
    statbar = gtk_statusbar_new();
    gtk_widget_show(statbar);
    gtk_object_set_data(GTK_OBJECT(toplevel), "statbar", (gpointer)statbar);
    gtk_box_pack_end(GTK_BOX(vbox), statbar, TRUE, TRUE, 0);

    gtk_widget_show(mbar);
    gtk_widget_show(tbar);
    gtk_widget_show(vbox);
    gtk_widget_show(toplevel);

    /* Set up extra clist styles & colors. */
    cl_style = gtk_style_copy(gtk_widget_get_style(msglist));
    cl_style->font = gdk_font_load("6x13bold");
    gtk_object_set_data(GTK_OBJECT(toplevel), "boldstyle", (gpointer)cl_style);

    gdk_color_parse("gray70", &delcol);
    gtk_object_set_data(GTK_OBJECT(toplevel), "delcol", (gpointer)&delcol);

    /* Create the pop-up menu for the compose window(s). */
    ifactory = gtk_item_factory_new(GTK_TYPE_MENU, "<Compose>",
				    gtk_accel_group_new());
    gtk_item_factory_create_items(ifactory,
				  sizeof(comp_ife) / sizeof(GtkItemFactoryEntry),
				  comp_ife, NULL);
    gtk_object_set_data(GTK_OBJECT(toplevel), "compmenu", ifactory->widget);

    /* General mail initialization stuff. */
    last_message_read = messages.start;
    if(last_message_read)
	display_new_message();
    sync_list();
    update_message_list();
    show_undelete(FALSE);

    /* Set up periodical mail checking. */
    f = find_mailrc("retrieveinterval");
    if (!f) {
	interval = DEFAULT_CHECK_TIME * 1000;
    }
    else {
	interval = atoi(f) * 1000;
    }
    if(interval > 0) {
	check_interval = gtk_timeout_add(interval, mail_check_cb,
					 (gpointer)interval);
	gtk_object_set_data(GTK_OBJECT(toplevel), "mailchkint",
			    (gpointer)check_interval);
    }

   gtk_main();
}

gint
mail_check_cb(gpointer data)
{
    gchar	*f;
    guint32	last_interval = (guint32)data, interval;
    guint	check_interval;

    check_for_new_mail();

    f = find_mailrc("retrieveinterval");
    if (!f) {
	interval = DEFAULT_CHECK_TIME * 1000;
    }
    else {
	interval = atoi(f) * 1000;
    }
    if((interval > 0) && (interval != last_interval)) {
	check_interval = (guint)gtk_object_get_data(GTK_OBJECT(toplevel),
						    "mailchkint");
	gtk_timeout_remove(check_interval);
	check_interval = gtk_timeout_add(interval, mail_check_cb,
					 (gpointer)interval);
	gtk_object_set_data(GTK_OBJECT(toplevel), "mailchkint",
			    (gpointer)check_interval);
    }

    return 0;
} /* mail_check_cb */

static void
about_cb(GtkWidget *widget, gpointer data)
{
    static GtkWidget	*dialog = NULL;
    gint		x, y, w, h, dialog_x, dialog_y;
    GtkWidget		*title, *message, *image, *ok_button, *hbox, *vbox;

    if( dialog == NULL ) {
	dialog = gtk_dialog_new();

	/* Create child widgets */
	hbox = gtk_hbox_new(FALSE, 4);
	vbox = gtk_vbox_new(FALSE, 4);
	title = gtk_label_new("Privtool");
	gtk_widget_set_name(title, "about_title");
	message = gtk_label_new("Written by: Mark Grant <mark@unicorn.com>\nGtk+ Interface by: Glenn Trigg <ggt@linuxfan.com>");
	gtk_widget_set_name(message, "about_text");
	gtk_label_set_justify(GTK_LABEL(message), GTK_JUSTIFY_LEFT);
	gtk_widget_realize(dialog);
	image = get_pixmap(dialog, privtool_logo_lc_xpm);
	ok_button = gtk_button_new_with_label("OK");
	gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),
				       4);

	/* Add child widgets to the dialog */
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,
			   TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), image,
			   TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(hbox), vbox,
			   TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(vbox), title,
			   FALSE, FALSE, 20);
	gtk_box_pack_start(GTK_BOX(vbox), message,
			   TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), ok_button,
			   TRUE, TRUE, 0);

	/* Show child widgets */
	gtk_widget_show(hbox);
	gtk_widget_show(vbox);
	gtk_widget_show(title);
	gtk_widget_show(message);
	gtk_widget_show(image);
	gtk_widget_show(ok_button);

	/* Add signals */
        gtk_signal_connect(GTK_OBJECT(ok_button), "clicked",
                           GTK_SIGNAL_FUNC (about_close_cb), dialog);
	gtk_widget_grab_focus(ok_button);
	gtk_window_set_transient_for(&GTK_DIALOG(dialog)->window,
				     GTK_WINDOW(toplevel));
    }
    gdk_window_get_origin(GTK_WIDGET(toplevel)->window, &x, &y);
    gdk_window_get_size(GTK_WIDGET(toplevel)->window, &w, &h);
    dialog_x = x + w/4;
    dialog_y = y + h/4;

    gtk_widget_set_uposition(GTK_WIDGET(dialog), dialog_x, dialog_y); 
    gtk_widget_show(dialog);
} /* about_cb */

static void
about_close_cb(GtkWidget *w, gpointer data)
{
    GtkWidget	*dialog = (GtkWidget *)data;

    gtk_widget_hide(dialog);
} /* about_close_cb */

static void
add_key_cb(GtkWidget *w, gpointer data)
{
    if(last_message_read != NULL){
	add_key_proc(NULL, last_message_read);
    }
} /* add_key_cb */

unsigned int
alert(GtkWidget *parent, char *message, ALERT_TYPE atype,
      unsigned int nbuttons, ...)
{
    va_list		arglist;
    unsigned int	retval;
    int			i;
    gint		x, y, w, h, dialog_x, dialog_y;
    char		*btn;
    GtkWidget		*dialog, *hbox, *label, *button, *image;

    update_random();
    dialog = gtk_dialog_new();
    hbox = gtk_hbox_new(FALSE, 4);
    label = gtk_label_new(message);

    gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_LEFT);

    switch(atype) {
    case ALERT_MESSAGE:
	image = get_pixmap(toplevel, exclamation_xpm);
	gtk_window_set_title(&GTK_DIALOG(dialog)->window, "Privtool - Message");
	break;
    case ALERT_QUESTION:
	image = get_pixmap(toplevel, question_xpm);
	gtk_window_set_title(&GTK_DIALOG(dialog)->window, "Privtool - Question");
	break;
    case ALERT_ERROR:
	image = get_pixmap(toplevel, exclamation2_xpm);
	gtk_window_set_title(&GTK_DIALOG(dialog)->window, "Privtool - Error");
	break;
    default:
	gtk_window_set_title(&GTK_DIALOG(dialog)->window, "Privtool - Alert");
	break;
    }

    gtk_box_pack_start(GTK_BOX(hbox), image, FALSE, TRUE, 20);
    gtk_box_pack_start(GTK_BOX(hbox), label, TRUE, TRUE, 20);
    gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), hbox,
		       TRUE, TRUE, 20);
    gtk_widget_show(image);
    gtk_widget_show(label);
    gtk_widget_show(hbox);

    va_start(arglist, nbuttons);

    for(i = 0; i < nbuttons; i++) {
	btn = va_arg(arglist, char *);
	button = gtk_button_new_with_label(btn);
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(dialog)->action_area),
				    button);
	gtk_object_set_user_data(GTK_OBJECT(button), (gpointer)(i+1));
	gtk_signal_connect(GTK_OBJECT(button), "clicked",
			   GTK_SIGNAL_FUNC(alert_cb), dialog);
	if(i == 0) {
	    GTK_WIDGET_SET_FLAGS(button, (GTK_CAN_DEFAULT|GTK_HAS_DEFAULT));
	    gtk_widget_grab_default(button);
	}
	gtk_widget_show(button);
    }

    va_end(arglist);

    if(parent) {
	gtk_window_set_transient_for(&GTK_DIALOG(dialog)->window,
				     GTK_WINDOW(parent));
	gdk_window_get_origin(GTK_WIDGET(parent)->window, &x, &y);
	gdk_window_get_size(GTK_WIDGET(parent)->window, &w, &h);
	dialog_x = x + w/4;
	dialog_y = y + h/4;

	gtk_widget_set_uposition(GTK_WIDGET(dialog), dialog_x, dialog_y); 
    }
    else {
	gtk_window_set_transient_for(&GTK_DIALOG(dialog)->window,
				     GTK_WINDOW(toplevel));
	gdk_window_get_origin(GTK_WIDGET(toplevel)->window, &x, &y);
	gdk_window_get_size(GTK_WIDGET(toplevel)->window, &w, &h);
	dialog_x = x + w/4;
	dialog_y = y + h/4;

	gtk_widget_set_uposition(GTK_WIDGET(dialog), dialog_x, dialog_y); 
    }

    gtk_signal_connect(GTK_OBJECT(dialog), "delete_event",
		       GTK_SIGNAL_FUNC (alert_delete_cb), dialog);
    show_busy();
    gtk_widget_show(dialog);
    gtk_grab_add(dialog);

    gtk_main();

    retval = (unsigned int)gtk_object_get_user_data(GTK_OBJECT(dialog));

    gtk_widget_destroy(dialog);
    clear_busy();

    return retval;
} /* alert */

static void
alert_cb(GtkWidget *w, gpointer data)
{
    GtkWidget	*dialog = (GtkWidget *)data;

    gtk_object_set_user_data(GTK_OBJECT(dialog),
			     gtk_object_get_user_data(GTK_OBJECT(w)));
    gtk_grab_remove(GTK_WIDGET(data));
    gtk_main_quit();
} /* alert_cb */

static gint
alert_delete_cb(GtkWidget *w, GdkEvent *event, gpointer data)
{
    GtkWidget	*dialog = (GtkWidget *)data;

    gtk_object_set_user_data(GTK_OBJECT(dialog), (gpointer)1);
    gtk_grab_remove(GTK_WIDGET(data));
    gtk_main_quit();
    return(TRUE);
} /* alert_delete_cb */

void
bad_file_notice(int w)
{
				/* TODO */
    g_warning("bad_file_notice: To be done.");
    alert(NULL, "Error: Bad message format.", ALERT_MESSAGE, 1, "OK");
}

void
bad_key_notice_proc()
{
				/* TODO */
    g_warning("bad_key_notice_proc: To be done.");
    alert(NULL, "Error: No key found in message or bad key format.",
	  ALERT_MESSAGE, 1, "OK");
}

int
bad_pass_phrase_notice(int w)
{
    int		val;

				/* TODO */
    g_warning("bad_pass_phrase_notice: To be done.");
    val = alert(NULL, "Error: Bad passphrase.", ALERT_ERROR, 2, "Retry",
		"Cancel");
    return (val == 1);
}

void
beep_display_window()
{
    gdk_beep();
}

void
clear_busy()
{
    COMPOSE_WINDOW	*win;

    gdk_window_set_cursor(toplevel->window, NULL);

    win = compose_first;

    while (win) {
	gdk_window_set_cursor(win->deliver_frame->window, NULL);
	win = win->next;
    }
}

void
clear_display_footer(DISPLAY_WINDOW *w)
{
    guint	ci;
    GtkWidget	*statbar =
	(GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel), "statbar");

    ci = gtk_statusbar_get_context_id(GTK_STATUSBAR(statbar), "display");
    gtk_statusbar_pop(GTK_STATUSBAR(statbar), ci);
}

void
clear_display_window(DISPLAY_WINDOW *w)
{
    GtkWidget	*text = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
							 "msgtext");

    /* Clear text from point onwards. */
    gtk_text_freeze(GTK_TEXT(text));
    gtk_text_forward_delete(GTK_TEXT(text),
			    gtk_text_get_length(GTK_TEXT(text)) -
			    gtk_text_get_point(GTK_TEXT(text)));
    gtk_text_thaw(GTK_TEXT(text));
}

void
clear_main_footer()
{
    guint	ci;
    GtkWidget	*statbar =
	(GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel), "statbar");

    ci = gtk_statusbar_get_context_id(GTK_STATUSBAR(statbar), "main");
    gtk_statusbar_pop(GTK_STATUSBAR(statbar), ci);
}

void
clear_passphrase_string()
{
    GtkWidget	*ppdialog =
	(GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel), "ppdialog");
    GtkWidget	*entry;
	
    if(ppdialog == NULL)
	return;

    entry = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(ppdialog));
    if(entry) {
	gtk_entry_set_text(GTK_ENTRY(entry), "");
    }
}

void
close_all_windows()
{
				/* TODO */
    g_warning("close_all_windows: To be done.");
}

void
close_deliver_window(COMPOSE_WINDOW *w)
{
    gtk_widget_hide(w->deliver_frame);
}

void
close_passphrase_window()
{
    GtkWidget	*dialog;
	
    dialog = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
					      "ppdialog");
    if(dialog) {
	gtk_widget_hide(dialog);
    }
}

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

DISPLAY_WINDOW *
create_display_window()
{
    return NULL;
}

void
create_passphrase_window()
{
    GtkWidget	*dialog = NULL, *message, *entry, *ok_button;

    if( dialog == NULL ) {
	dialog = gtk_dialog_new();

	/* Create child widgets */
	message = gtk_label_new("Enter Passphrase");
	entry = gtk_entry_new();
	ok_button = gtk_button_new_with_label("OK");
	gtk_entry_set_visibility(GTK_ENTRY(entry), FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox),
				       4);

	/* Add child widgets to the dialog */
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), message,
			   TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), entry,
			   TRUE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area), ok_button,
			   TRUE, TRUE, 0);

	/* Show child widgets */
	gtk_widget_show(message);
	gtk_widget_show(entry);
	gtk_widget_show(ok_button);

	/* Add signals */
        gtk_signal_connect(GTK_OBJECT(ok_button), "clicked",
                           GTK_SIGNAL_FUNC (got_passphrase), NULL);
        gtk_signal_connect(GTK_OBJECT(entry), "activate",
                           GTK_SIGNAL_FUNC (got_passphrase), NULL);

	gtk_object_set_data(GTK_OBJECT(toplevel), "ppdialog",
			    (gpointer)dialog);
	gtk_object_set_user_data(GTK_OBJECT(dialog), (gpointer)entry);
    }
}

void
deleteAllMessages()
{
    GtkWidget	*msglist;

    msglist = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
					       "msglist");
    gtk_clist_clear(GTK_CLIST(msglist));
}

void
delete_message_proc()
{
    int		cur_row;
    GList	*ptr;
    GtkWidget	*msglist;
    MESSAGE	*msg;

    msglist = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
					       "msglist");

    /* Delete all the selected messages. If there are none selected,
       delete the currently displayed message.
    */
    if(GTK_CLIST(msglist)->selection == NULL) {
	msg = last_message_read;
	cur_row = gtk_clist_find_row_from_data(GTK_CLIST(msglist), msg);
	delete_message(msg);
	if(show_deleted) {
	    display_message_description(msg);
	}
	else {
	    gtk_clist_remove(GTK_CLIST(msglist), cur_row);
	}
    }
    else {
	for(ptr = GTK_CLIST(msglist)->selection;
	    ptr != NULL;
	    ptr = g_list_next(ptr)) {
	    msg = (MESSAGE *)gtk_clist_get_row_data(GTK_CLIST(msglist),
						    (int)(ptr->data));
	    delete_message(msg);
	    if(show_deleted) {
		display_message_description(msg);
	    }
	    else {
		gtk_clist_remove(GTK_CLIST(msglist), (int)(ptr->data));
	    }
	}
    }

    /* Move current selected message pointer if neccesary */
    if (last_message_read) {
	last_message_read->flags |= MESS_SELECTED;
	display_message_description (last_message_read);
	display_message(last_message_read);
	sync_list();
    }
    update_message_list();
    show_undelete(TRUE);
}

void
display_message_body(BUFFER *b, DISPLAY_WINDOW *w)
{
    GtkWidget	*text = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
							 "msgtext");

    hide_addkey();
    hide_attach();
    gtk_text_freeze(GTK_TEXT(text));
    gtk_text_set_point(GTK_TEXT(text), gtk_text_get_length(GTK_TEXT(text)));

    /* Now insert this message. */
    gtk_text_insert(GTK_TEXT(text), NULL, NULL, NULL,
		    b->message, -1);

    /* Set the point back to the top of the screen. */
    gtk_text_set_point(GTK_TEXT(text), 0);
    gtk_text_thaw(GTK_TEXT(text));
}

void
display_message_description(MESSAGE *m)
{
    int		msgrow, set_style = 1;
    char	msg_stat[5];
    GdkColor	*delcol;
    GtkWidget	*msglist;
    GtkStyle	*msgstyle;

    msglist = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
					       "msglist");
    msgrow = gtk_clist_find_row_from_data(GTK_CLIST(msglist), m);
    if(m->flags & MESS_DELETED) {
	delcol = (GdkColor *)gtk_object_get_data(GTK_OBJECT(toplevel),
						 "delcol");
	gtk_clist_set_row_style(GTK_CLIST(msglist), msgrow, NULL);
	gtk_clist_set_foreground(GTK_CLIST(msglist), msgrow, delcol);
	set_style = 0;
    }
    else if(m->flags & MESS_SELECTED) {
	msgstyle = (GtkStyle *)gtk_object_get_data(GTK_OBJECT(toplevel),
						   "boldstyle");
    }
    else {
	msgstyle = NULL;	/* Reset style to clist default */
    }
    memset(msg_stat, '\0', 5);
    strncpy(msg_stat, m->description, 3);
    gtk_clist_set_text(GTK_CLIST(msglist), msgrow, 0, msg_stat);

    if(set_style) {
	gtk_clist_set_row_style(GTK_CLIST(msglist), msgrow, msgstyle);
    }
}

void
display_message_sig(BUFFER *b, DISPLAY_WINDOW *w)
{
    GtkWidget	*text;

    if(b->message == NULL)
	return;

    /* b->message is the complete signature message which may be
       multi-line. We need to figure out a decent way to display that
       without taking to much screen real-estate.  -gt 13/4/97.

       Why not just append it to the message body? Can't see anything
       wrong with that!  -gt 6/7/99.
    */

    text = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
					    "msgtext");
    gtk_text_freeze(GTK_TEXT(text));
    gtk_text_set_point(GTK_TEXT(text), gtk_text_get_length(GTK_TEXT(text)));
    gtk_text_insert(GTK_TEXT(text), NULL, NULL, NULL, "\n", 1);
    gtk_text_insert(GTK_TEXT(text), NULL, NULL, NULL,
		    b->message, -1);
    gtk_text_thaw(GTK_TEXT(text));
}

void
display_sender_info(MESSAGE *m, DISPLAY_WINDOW *w)
{
    char	*allhdr = g_strdup(m->header->message), *ptr, *ptr2, *split;
    char	colon;
    char	keyword[256];	/* Should be long enough? :-) */
    int		length, skip = 0;
    GtkWidget	*text = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
							 "msgtext");
    GtkStyle	*boldstyle;

    if(m == NULL)
	return;

    boldstyle = (GtkStyle *)gtk_object_get_data(GTK_OBJECT(toplevel),
						"boldstyle");
    ptr = strtok(allhdr, "\n");

    if(ptr == NULL){
	g_free(allhdr);
	return;
    }

    gtk_text_freeze(GTK_TEXT(text));
    gtk_text_set_point(GTK_TEXT(text), 0);

    /* Now insert this message. */
    do{
	if(((split = strchr(ptr, ':')) != NULL) && (*ptr != '\t')) {
	    ptr2 = strchr(ptr, ' ');
	    split++;
	    length = split - ptr;
	    strncpy(keyword, ptr, length);
	    keyword[length] = '\0';
	    if( ! full_header ) {
		/* Check if this header should be shown. */
		if( (colon = keyword[length-1]) == ':' )
		    keyword[length-1] = '\0';	/* Strip ':' */
		if(! retain_line(keyword) || (split > ptr2)) {
		    skip = 1;
		    continue;
		}
		keyword[length-1] = colon;
		*split = '\t';
	    }
	    skip = 0;

	    gtk_text_insert(GTK_TEXT(text), boldstyle->font, NULL, NULL,
			    keyword, -1);
	    gtk_text_insert(GTK_TEXT(text), NULL, NULL, NULL,
			    split, -1);
	    gtk_text_insert(GTK_TEXT(text), NULL, NULL, NULL,
			    "\n", -1);
	}
	else{
	    if( skip )
		continue;
	    gtk_text_insert(GTK_TEXT(text), NULL, NULL, NULL,
			    ptr, -1);
	    gtk_text_insert(GTK_TEXT(text), NULL, NULL, NULL,
			    "\n", -1);
	}
    }while((ptr = strtok(NULL, "\n")));

    gtk_text_insert(GTK_TEXT(text), NULL, NULL, NULL, "\n", 1);
    gtk_text_thaw(GTK_TEXT(text));
}

int
dont_quit_notice_proc()
{
    int		val;
    COMPOSE_WINDOW	*w;

    /* Make all the open compose window visible. */
    w = compose_first;
    while (w) {
	if (w->in_use)
	    gdk_window_raise(w->deliver_frame->window);

	w = w->next;
    }

    val = alert(NULL, "You have open compose windows. Quitting will\n\
lose the messages in those windows.\n\n\
Do you really want to quit? \n", ALERT_QUESTION, 2, "Yes", "No");

    return (val == 2);
}

int
failed_save_notice_proc()
{
				/* TODO */
    g_warning("failed_save_notice_proc: To be done.");
    return 0;
}

static void
hide_addkey()
{
    GtkWidget		*mi;
    GtkItemFactory	*ife;

    ife = (GtkItemFactory *)gtk_object_get_data(GTK_OBJECT(toplevel),
						"menufact");
    mi = gtk_item_factory_get_widget(ife, "/Edit/Add Key");
    gtk_widget_set_sensitive(mi, FALSE);
}

static void
hide_attach()
{
    GtkWidget		*mi;
    GtkItemFactory	*ife;

    ife = (GtkItemFactory *)gtk_object_get_data(GTK_OBJECT(toplevel),
						"menufact");
    mi = gtk_item_factory_get_widget(ife, "/Edit/Show Attachment");
    gtk_widget_set_sensitive(mi, FALSE);
}

void
hide_header_frame()
{
	/* This function doesn't need to do anything. */
}

void
invalid_attachment_notice_proc()
{
				/* TODO */
    g_warning("invalid_attachment_notice_proc: To be done.");
}

void
lock_display_window(DISPLAY_WINDOW *w)
{
    GtkWidget	*text = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
							 "msgtext");

    gtk_text_freeze(GTK_TEXT(text));
}

int
no_key_notice_proc(int w)
{
				/* TODO */
    g_warning("no_key_notice_proc: To be done.");
    return 0;
}

int
no_sec_notice_proc(int w)
{
				/* TODO */
    g_warning("no_sec_notice_proc: To be done.");
    return 0;
}

void
open_passphrase_window(char *s)
{
    gint	x, y, w, h, dialog_x, dialog_y;
    GtkWidget	*ppdialog =
	(GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel), "ppdialog");

    if(ppdialog) {
	gtk_window_set_transient_for(&GTK_DIALOG(ppdialog)->window,
				     GTK_WINDOW(toplevel));
	gdk_window_get_origin(GTK_WIDGET(toplevel)->window, &x, &y);
	gdk_window_get_size(GTK_WIDGET(toplevel)->window, &w, &h);
	dialog_x = x + w/4;
	dialog_y = y + h/4;

	gtk_widget_set_uposition(GTK_WIDGET(ppdialog), dialog_x, dialog_y); 
	gtk_widget_grab_focus(GTK_WIDGET(gtk_object_get_user_data(GTK_OBJECT(ppdialog))));
	gtk_widget_show(ppdialog);
    }
}

char *
read_bcc(COMPOSE_WINDOW *w)
{
    char	*lbl;

    lbl = gtk_entry_get_text(GTK_ENTRY(w->send_bcc));
    return lbl;
}

char *
read_cc(COMPOSE_WINDOW *w)
{
    char	*lbl;

    lbl = gtk_entry_get_text(GTK_ENTRY(w->send_cc));
    return lbl;
}

int
read_deliver_flags(COMPOSE_WINDOW *w)
{
    int		flags = 0;

    flags |= gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w->sign));
    flags |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w->encrypt)) << 1);
    flags |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w->log)) << 2);
    flags |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w->raw)) << 3);
#ifdef HAVE_MIXMASTER
    flags |= (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w->remail)) << 4);
#endif

    return flags;
}

char *
read_extra_headerline(COMPOSE_WINDOW *w, int i)
{
    char	*lbl;

    if(w->extra_headers[i]) {
	lbl = gtk_entry_get_text(GTK_ENTRY(w->extra_headers[i]));
	return lbl;
    }
    else
	return NULL;
}

void
read_message_to_deliver(COMPOSE_WINDOW *w, BUFFER *b)
{
    guint	length;

    length = gtk_text_get_length(GTK_TEXT(w->text));
    add_to_buffer(b, gtk_editable_get_chars(GTK_EDITABLE(w->text), 0, length),
		  length);
}

int
read_only_notice_proc()
{
				/* TODO */
    g_warning("read_only_notice_proc: To be done.");
    return 0;
}

char *
read_passphrase_string()
{
    GtkWidget	*ppdialog =
	(GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel), "ppdialog");
    GtkWidget	*entry;

    if(ppdialog == NULL)
	return NULL;

    entry = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(ppdialog));
    return gtk_entry_get_text(GTK_ENTRY(entry));
}

char *
read_recipient(COMPOSE_WINDOW *w)
{
    char	*lbl;

    lbl = gtk_entry_get_text(GTK_ENTRY(w->send_to));
    return lbl;
}

char *
read_subject(COMPOSE_WINDOW *w)
{
    char	*lbl;

    lbl = gtk_entry_get_text(GTK_ENTRY(w->send_subject));
    return lbl;
}

void
set_display_footer(DISPLAY_WINDOW *w, char *s)
{
    guint	ci;
    GtkWidget	*statbar =
	(GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel), "statbar");

    ci = gtk_statusbar_get_context_id(GTK_STATUSBAR(statbar), "display");
    gtk_statusbar_push(GTK_STATUSBAR(statbar), ci, s);
    gdk_flush();
}

void
set_initial_scrollbar_position()
{
    /* Don't need anything in here for the Gtk+ version. */
}

void
set_main_footer(char *s)
{
    guint	ci;
    GtkWidget	*statbar =
	(GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel), "statbar");

    ci = gtk_statusbar_get_context_id(GTK_STATUSBAR(statbar), "main");
    gtk_statusbar_push(GTK_STATUSBAR(statbar), ci, s);
    gdk_flush();
}

void
show_addkey(DISPLAY_WINDOW *w)
{
    GtkWidget		*mi;
    GtkItemFactory	*ife;

    ife = (GtkItemFactory *)gtk_object_get_data(GTK_OBJECT(toplevel),
						"menufact");
    mi = gtk_item_factory_get_widget(ife, "/Edit/Add Key");
    gtk_widget_set_sensitive(mi, TRUE);
}

void
show_attach(DISPLAY_WINDOW *w)
{
    GtkWidget		*mi;
    GtkItemFactory	*ife;

    ife = (GtkItemFactory *)gtk_object_get_data(GTK_OBJECT(toplevel),
						"menufact");
    mi = gtk_item_factory_get_widget(ife, "/Edit/Show Attachment");
    gtk_widget_set_sensitive(mi, TRUE);
}

void
show_busy()
{
    static GdkCursor	*busy_cursor = NULL;
    COMPOSE_WINDOW	*win;

    if( busy_cursor == NULL ) {
	busy_cursor = gdk_cursor_new(GDK_WATCH);
    }

    gdk_window_set_cursor(toplevel->window, busy_cursor);

    /* Set busy cursor on any open compose windows. */
    win = compose_first;

    while (win) {
	if (win->in_use)
	    gdk_window_set_cursor(win->deliver_frame->window, busy_cursor);

	win = win->next;
    }

    /* Flush events to make sure the cursor(s) are changed. */
    gdk_flush();
}

void
show_display_window(MESSAGE *m, DISPLAY_WINDOW *w)
{
    GtkWidget	*text = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
							 "msgtext");

    gtk_text_thaw(GTK_TEXT(text));
}

void
show_newmail_icon()
{
    static time_t	lastbeeptime = 0;
    GdkPixmap		*pixmap;
    GdkBitmap		*mask;
    GtkStyle		*style;

    /* Don't beep more than once every two seconds. */
    if(time(NULL) > (lastbeeptime + 2)) {
	lastbeeptime = time(NULL);
	beep_display_window();
    }

    style = gtk_widget_get_style(toplevel);
    /* TODO: Possible memory leak here? Need to save pixmap pointer? */
    pixmap = gdk_pixmap_create_from_xpm_d(toplevel->window,
					  &mask,
					  &style->bg[GTK_STATE_NORMAL],
					  privtool_new_xpm);
    gdk_window_set_icon(toplevel->window, NULL, pixmap, mask);
}

void
show_normal_icon()
{
    GdkPixmap	*pixmap;
    GdkBitmap	*mask;
    GtkStyle	*style;
    
    style = gtk_widget_get_style(toplevel);
    /* TODO: Possible memory leak here? Need to save pixmap pointer? */
    pixmap = gdk_pixmap_create_from_xpm_d(toplevel->window,
					  &mask,
					  &style->bg[GTK_STATE_NORMAL],
					  privtool_empty_xpm);
    gdk_window_set_icon(toplevel->window, NULL, pixmap, mask);
}

static void
show_undelete(gboolean show)
{
    GtkItemFactory	*ife;
    GtkWidget		*undeletebtn, *undeletemi, *undeleteall;

    ife = (GtkItemFactory *)gtk_object_get_data(GTK_OBJECT(toplevel),
						"menufact");
    undeleteall = gtk_item_factory_get_widget(ife, "/Edit/Undelete");
    undeletemi = gtk_item_factory_get_widget(ife, "/Edit/Undelete Last");
    undeletebtn = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
						   "undeletebtn");
    gtk_widget_set_sensitive(undeletebtn, show);
    gtk_widget_set_sensitive(undeleteall, show);
    gtk_widget_set_sensitive(undeletemi, show);
} /* show_undelete */

void
shutdown_ui()
{
    /* gtk_widget_destroy(toplevel); */
    gtk_main_quit();		/* ???? */
}

void
sync_list()
{
    gint	selrow;
    GtkWidget	*clist, *prevmi, *nextmi, *prevbtn, *nextbtn;
    GtkStyle	*msgstyle;
    GtkItemFactory *ife;

    if(last_message_read == NULL)
	return;

    clist = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
					     "msglist");
    selrow = gtk_clist_find_row_from_data(GTK_CLIST(clist), last_message_read);

    /* Make sure the current displayed message is visible and emphasised. */
    if(selrow >= 0) {
	msgstyle = (GtkStyle *)gtk_object_get_data(GTK_OBJECT(toplevel),
						   "boldstyle");
	gtk_clist_set_row_style(GTK_CLIST(clist), selrow, msgstyle);
	gtk_clist_moveto(GTK_CLIST(clist), selrow, -1,
			 0.5, 0.0);
	gtk_clist_unselect_all(GTK_CLIST(clist));
	/* gtk_clist_select_row(GTK_CLIST(clist), selrow, 0); */
    }
    
    /* Get the menu's item factory and then the previous/next message menu
       items. Also get the previous/next toolbar widgets.
    */
    ife = (GtkItemFactory *)gtk_object_get_data(GTK_OBJECT(toplevel),
						"menufact");
    prevmi = gtk_item_factory_get_widget(ife, "/View/Previous");
    nextmi = gtk_item_factory_get_widget(ife, "/View/Next");
    prevbtn = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
					       "prevbtn");
    nextbtn = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
					       "nextbtn");

    /* Set the previous or next menu & toolbar items [in]sensitive depending
       on whether the first or last message is displayed.
    */
    if(last_message_read->prev == NULL) {
	gtk_widget_set_sensitive(prevmi, FALSE);
	gtk_widget_set_sensitive(nextmi, TRUE);
	gtk_widget_set_sensitive(prevbtn, FALSE);
	gtk_widget_set_sensitive(nextbtn, TRUE);
    }
    else if(last_message_read->next == NULL) {
	gtk_widget_set_sensitive(prevmi, TRUE);
	gtk_widget_set_sensitive(nextmi, FALSE);
	gtk_widget_set_sensitive(prevbtn, TRUE);
	gtk_widget_set_sensitive(nextbtn, FALSE);
    }
    else {
	gtk_widget_set_sensitive(prevmi, TRUE);
	gtk_widget_set_sensitive(nextmi, TRUE);
	gtk_widget_set_sensitive(prevbtn, TRUE);
	gtk_widget_set_sensitive(nextbtn, TRUE);
    }
} /* sync_list */

void
update_log_item(COMPOSE_WINDOW *w, int flags)
{
				/* TODO */
    g_warning("update_log_item: To be done.");
}

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

COMPOSE_WINDOW *
x_setup_send_window()
{
    return compose_find_free();
}

/*----------------------------------------------------------------------*/

static gint
delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    GtkWidget	*fldr_t;

    if( (gint)data == 1) {
	/* gtk_widget_hide(widget); */
	fldr_t = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
						  "folderbtn");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fldr_t), FALSE);
	return(TRUE);
    }

    save_and_quit_proc();
    return (TRUE);
} /* delete_event */

static void
destroy(GtkWidget *widget, gpointer data)
{
    return;
} /* destroy */

static GtkWidget *
create_menubar(void)
{
    GtkItemFactory	*ifactory;
    GtkAccelGroup	*ag;
    GtkWidget		*mbar_tgl;

    ag = gtk_accel_group_get_default();
    ifactory = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<Main>", ag);
    gtk_item_factory_create_items(ifactory,
				  sizeof(ife) / sizeof(GtkItemFactoryEntry),
				  ife, NULL);
    gtk_widget_show(ifactory->widget);
    gtk_object_set_data(GTK_OBJECT(toplevel), "menufact", (gpointer)ifactory);

    /* Set the default state of some toggle menu items.
     */
    mbar_tgl = gtk_item_factory_get_widget(ifactory, "/View/Show Toolbar");
    gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(mbar_tgl), TRUE);
    mbar_tgl = gtk_item_factory_get_widget(ifactory, "/View/Show Deleted");
    gtk_check_menu_item_set_state(GTK_CHECK_MENU_ITEM(mbar_tgl), TRUE);
    mbar_tgl = gtk_item_factory_get_widget(ifactory, "/Edit/Add Key");
    gtk_widget_set_sensitive(mbar_tgl, FALSE);
    mbar_tgl = gtk_item_factory_get_widget(ifactory, "/Edit/Show Attachment");
    gtk_widget_set_sensitive(mbar_tgl, FALSE);

    return ifactory->widget;
} /* create_menubar */

static GtkWidget *
create_toolbar(GtkWidget *window)
{
    GtkWidget	*toolbar, *w;

    toolbar = gtk_toolbar_new(GTK_ORIENTATION_HORIZONTAL, GTK_TOOLBAR_ICONS);
    w = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Prev",
				"Previous Message", NULL,
				get_pixmap(window, prev_xpm), (GtkSignalFunc)prev_msg_cb,
				NULL);
    gtk_object_set_data(GTK_OBJECT(window), "prevbtn", (gpointer)w);
    w = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Next", "Next Message",
				NULL, get_pixmap(window, next_xpm),
				(GtkSignalFunc)next_msg_cb, NULL);
    gtk_object_set_data(GTK_OBJECT(window), "nextbtn", (gpointer)w);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Delete", "Delete Message",
			    NULL, get_pixmap(window, delete_xpm),
			    (GtkSignalFunc)delete_message_proc, NULL);
    w = gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Undelete",
				"Undelete Message",
				NULL, get_pixmap(window, undelete_xpm),
				(GtkSignalFunc)undelete_cb, NULL);
    gtk_object_set_data(GTK_OBJECT(window), "undeletebtn", (gpointer)w);
    gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Compose", "Compose new message",
			    NULL, get_pixmap(window, compose_xpm),
			    (GtkSignalFunc)compose_cb, (gpointer)COMPOSE_NEW);
    gtk_toolbar_append_item(GTK_TOOLBAR(toolbar), "Reply", "Reply to sender",
			    NULL, get_pixmap(window, reply_xpm), (GtkSignalFunc)compose_cb,
			    (gpointer)(COMPOSE_REPLY|R_SENDER_INCLUDE));
    gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
    w = gtk_toolbar_append_element(GTK_TOOLBAR(toolbar),
				   GTK_TOOLBAR_CHILD_TOGGLEBUTTON, NULL,
				   "Folders", "Folder window", NULL,
				   get_pixmap(window, folderwin_xpm),
				   (GtkSignalFunc)show_folder_win_cb, NULL);
    gtk_object_set_data(GTK_OBJECT(window), "folderbtn", (gpointer)w);

    gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));
    w = gtk_combo_new();
    gtk_widget_show(w);
    gtk_combo_set_use_arrows_always(GTK_COMBO(w), TRUE);
    gtk_toolbar_append_widget(GTK_TOOLBAR(toolbar), w, "Folders", NULL);
    gtk_object_set_data(GTK_OBJECT(window), "combo", (gpointer)w);

    gtk_toolbar_set_button_relief(GTK_TOOLBAR(toolbar), GTK_RELIEF_NONE);
    gtk_object_set_data(GTK_OBJECT(window), "toolbar", (gpointer)toolbar);

    populate_combo();

    return toolbar;
} /* create_toolbar */

static GtkWidget *
create_msglist(void)
{
    int		l = 1, i = 1;
    static gchar *titles[] = {"", "", "From", "Date", "Size", "Subject"};
    GtkWidget	*clist, *swin;
    MESSAGE	*m;

    clist = gtk_clist_new_with_titles(6, titles);
    gtk_widget_set_usize(GTK_WIDGET(clist), 0, 150);
    gtk_clist_set_reorderable(GTK_CLIST(clist), TRUE);
    gtk_clist_set_use_drag_icons(GTK_CLIST(clist), TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 0, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 1, TRUE);
    gtk_clist_set_column_width(GTK_CLIST(clist), 2, 150);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 3, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 4, TRUE);
    gtk_clist_set_column_auto_resize(GTK_CLIST(clist), 5, TRUE);
    gtk_clist_set_button_actions(GTK_CLIST(clist), 0, GTK_BUTTON_SELECTS);
    gtk_clist_set_button_actions(GTK_CLIST(clist), 1, GTK_BUTTON_DRAGS);
    gtk_clist_set_button_actions(GTK_CLIST(clist), 2, GTK_BUTTON_SELECTS);
    gtk_clist_set_compare_func(GTK_CLIST(clist), clist_sort_cmp);
    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    gtk_clist_set_selection_mode(GTK_CLIST(clist), GTK_SELECTION_EXTENDED);
#if 0
    gtk_drag_source_set(clist, GDK_BUTTON2_MASK, targets,
			1, GDK_ACTION_COPY|GDK_ACTION_MOVE);
#endif
    gtk_container_add(GTK_CONTAINER(swin), clist);
    gtk_object_set_data(GTK_OBJECT(toplevel), "msglist", (gpointer)clist);

    gtk_signal_connect(GTK_OBJECT(GTK_CLIST(clist)),
		       "click_column",
		       GTK_SIGNAL_FUNC(clist_sort_cb), NULL);
    gtk_signal_connect(GTK_OBJECT(GTK_CLIST(clist)),
		       "select_row",
		       GTK_SIGNAL_FUNC(select_msg_cb), NULL);

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
    gtk_widget_show(clist);
    gtk_widget_show(swin);

    return swin;
} /* create_msglist */

static GtkWidget *
get_pixmap(GtkWidget *window, gchar **bits)
{
    GtkWidget	*pw;
    GdkPixmap	*pixmap;
    GdkBitmap	*mask;
    GtkStyle	*style;
    
    style = gtk_widget_get_style(window);
    pixmap = gdk_pixmap_create_from_xpm_d(window->window,
					  &mask,
					  &style->bg[GTK_STATE_NORMAL],
					  bits);
    pw = gtk_pixmap_new(pixmap, mask);

    return pw;
} /* get_pixmap */

static void
update_mail_list(void)
{
    int		row;
    char	*cols[7], number[10], size[10], mess_type[5];
    MESSAGE	*m;
    GtkWidget	*clist = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
							  "msglist");

    m = messages.start;
    cols[6] = NULL;

    gtk_clist_freeze(GTK_CLIST(clist));
    gtk_clist_clear(GTK_CLIST(clist));

    while (m) {
	if(!show_deleted && (m->flags & MESS_DELETED)) {
	    m = m->next;
	    continue;
	}

	snprintf(number, 9, "%d", m->number);
	snprintf(size, 9, "%d/%d", m->lines, m->size);

	/* Set new/unread/etc. status */
	memset(mess_type, '\0', 5);
	strncpy(mess_type, m->description, 3);

	cols[0] = mess_type;
	cols[1] = number;
	cols[2] = m->sender;
	cols[3] = m->date;
	cols[4] = size;
	cols[5] = m->subject;
	row = gtk_clist_append(GTK_CLIST(clist), cols);
	gtk_clist_set_row_data(GTK_CLIST(clist), row, (gpointer)m);
	m = m->next;
    }

    gtk_clist_thaw(GTK_CLIST(clist));
} /* update_mail_list */

/* This function extracts all email addresses from the given string.
   It is used for setting the reply-to-all To: line.
   It works by finding the first '@' then working back until a suitable
   delimiter is found, and then working forward until the ending delimiter
   is found, saving that, finding the next '@' etc.
*/

static char *
extract_addresses(char *addrlist)
{
    int		n = 0, i;
    char	*atptr, *startptr, *endptr, *retptr;
    gchar	*addrs[100];	/* Is 100 enough potential addresses? */

    /* sanity check */
    if(!addrlist || (*addrlist == '\0'))
	return NULL;

    atptr = addrlist;		/* Start at the beginning... */
    endptr = addrlist;
    while((atptr = strchr(atptr, '@')) != NULL) {
	for(startptr = atptr;
	    ((*startptr != '<') &&
	     (*startptr != ' ') &&
	     (*startptr != '"') &&
	     (startptr > endptr));
	    startptr--)
	    ;
	if(startptr != addrlist)
	    startptr++;

	for(endptr = atptr;
	    ((*endptr != '\0') &&
	     (*endptr != ' ') &&
	     (*endptr != '>') &&
	     (*endptr != '"') &&
	     (*endptr != ','));
	    endptr++);

	/* Hopefully g_strndup NULL terminates ... */
	addrs[n++] = g_strndup(startptr, (endptr - startptr));
	atptr = endptr;
    }

    if(n == 0)
	return NULL;		/* No luck. */

    addrs[n] = NULL;
    retptr = g_strjoinv(", ", addrs);

    /* Now free all the strings allocated from g_strndup above. */
    for(i = 0; i < n; i++)
	g_free(addrs[i]);

    return retptr;
} /* extract_addresses */

static void
sort_cb(GtkWidget *w, gpointer data)
{
    GtkWidget	*clist;

    clist = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel), "msglist");

    clist_sort_cb(clist, (gint)data, NULL);
} /* sort_cb */

static void
clist_sort_cb(GtkWidget *clist, gint col, gpointer data)
{
    static int		last_col = -1;
    static GtkSortType	stype = GTK_SORT_ASCENDING;
    int			i;
    MESSAGE		*m, *lastm;

    if(last_col == col) {
	if(stype == GTK_SORT_ASCENDING) {
	    stype = GTK_SORT_DESCENDING;
	}
	else {
	    stype = GTK_SORT_ASCENDING;
	}
    }
    else {
	stype = GTK_SORT_ASCENDING;
    }

    gtk_clist_set_sort_type(GTK_CLIST(clist), stype);
    gtk_clist_set_sort_column(GTK_CLIST(clist), col);
    gtk_clist_sort(GTK_CLIST(clist));
    last_col = col;

    /* Fix up the message next & prev pointers & list_pos field. */
    lastm = NULL;
    for(i = 0; i < GTK_CLIST(clist)->rows; i++) {
	m = gtk_clist_get_row_data(GTK_CLIST(clist), i);
	m->list_pos = i;
	m->prev = lastm;
	if(lastm) {
	    lastm->next = m;
	}
	else {
	    messages.start = m;
	}
	lastm = m; 
    }
    m->next = NULL;
    messages.end = m;

    /* Make sure the displayed message list entry is visible. */
    sync_list();
} /* clist_sort_cb */

static gint
clist_sort_cmp(GtkCList *clist, gconstpointer ptr1, gconstpointer ptr2)
{
    long	num1, num2;
    char	*text1 = NULL;
    char	*text2 = NULL;
    struct tm	tmbuf;

    GtkCListRow *row1 = (GtkCListRow *) ptr1;
    GtkCListRow *row2 = (GtkCListRow *) ptr2;

    switch (row1->cell[clist->sort_column].type)
    {
    case GTK_CELL_TEXT:
	text1 = GTK_CELL_TEXT (row1->cell[clist->sort_column])->text;
	break;
    case GTK_CELL_PIXTEXT:
	text1 = GTK_CELL_PIXTEXT (row1->cell[clist->sort_column])->text;
	break;
    default:
	break;
    }
 
    switch (row2->cell[clist->sort_column].type)
    {
    case GTK_CELL_TEXT:
	text2 = GTK_CELL_TEXT (row2->cell[clist->sort_column])->text;
	break;
    case GTK_CELL_PIXTEXT:
	text2 = GTK_CELL_PIXTEXT (row2->cell[clist->sort_column])->text;
	break;
    default:
	break;
    }

    switch (clist->sort_column) {
    case 1:			/* Message number */
    case 4:			/* Message size */
	num1 = atol(text1);
	num2 = atol(text2);
	if( num1 < num2 ) {
	    return -1;
	}
	else if( num1 > num2 ) {
	    return 1;
	}
	return 0;
	break;
    case 3:			/* Message date */
	strptime(text1, "%a %b %d %T %Y", &tmbuf);
	num1 = mktime(&tmbuf);
	strptime(text2, "%a %b %d %T %Y", &tmbuf);
	num2 = mktime(&tmbuf);
	if( num1 < num2 ) {
	    return -1;
	}
	else if( num1 > num2 ) {
	    return 1;
	}
	return 0;
	break;
    default:
	if (!text2)
	    return (text1 != NULL);
	
	if (!text1)
	    return -1;
	return strcmp (text1, text2);
	break;
    }

    return 0;
} /* clist_sort_cmp */

static void
display_new_message()
{
    MESSAGE	*m;
    GtkWidget	*clist = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
							  "msglist");

    /* Find first unread message. */
    for(m = messages.start; m != NULL; m = m->next){
	if(m->status != MSTAT_READ)
	    break;
    }

    if(m == NULL){		/* No unread messages, display last. */
	display_message(last_message_read = messages.end);
    }
    else{			/* Got one so display it and position
				   list nicely. */
	display_message(last_message_read = m);
	gtk_clist_moveto(GTK_CLIST(clist), m->list_pos, -1,
			 0.0, 0.0);
    }
} /* display_new_message */

static void
select_msg_cb(GtkWidget *clist, gint row, gint col, GdkEvent *event)
{
    MESSAGE	*m;

    if(event && event->type == GDK_2BUTTON_PRESS) {
	m = (MESSAGE *)gtk_clist_get_row_data(GTK_CLIST(clist), row);
	select_message_proc(m);
	sync_list();
    }
} /* select_msg_cb */

static void
next_msg_cb(GtkWidget *w, gpointer data)
{
    next_message_proc();
    sync_list();
} /* next_msg_cb */

static void
prev_msg_cb(GtkWidget *w, gpointer data)
{
    prev_message_proc();
    sync_list();
} /* prev_msg_cb */

static void
show_fullhdr_cb(GtkWidget *w, gpointer data)
{
    GtkItemFactory	*ife;
    GtkWidget		*fullhdr;

    ife = (GtkItemFactory *)gtk_object_get_data(GTK_OBJECT(toplevel),
						"menufact");
    fullhdr = gtk_item_factory_get_widget(ife, "/View/Full Header");

    full_header = GTK_CHECK_MENU_ITEM(fullhdr)->active;
    if(last_message_read)
	display_message(last_message_read);
} /* show_fullhdr_cb */

static void
show_deleted_cb(GtkWidget *w, gpointer data)
{
    GtkItemFactory	*ife;
    GtkWidget		*showdel, *msglist;
    MESSAGE		*m;

    if(!m)
	return;

    msglist = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
					       "msglist");
    if(!msglist)
	return;
    ife = (GtkItemFactory *)gtk_object_get_data(GTK_OBJECT(toplevel),
						"menufact");
    showdel = gtk_item_factory_get_widget(ife, "/View/Show Deleted");

    show_deleted = GTK_CHECK_MENU_ITEM(showdel)->active;
    update_mail_list();
    gtk_clist_freeze(GTK_CLIST(msglist));
    if(show_deleted) {
	for(m = messages.start; m != NULL; m = m->next){
	    display_message_description(m);
	}
    }
    gtk_clist_thaw(GTK_CLIST(msglist));
    sync_list();
} /* show_deleted_cb */

static void
toggle_toolbar_cb(GtkWidget *w, gpointer data)
{
    GtkItemFactory	*ife;
    GtkWidget		*tbar_tgl, *toolbar;

    ife = (GtkItemFactory *)gtk_object_get_data(GTK_OBJECT(toplevel),
						"menufact");
    toolbar = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
						    "toolbar");
    if(toolbar == NULL)
	return;

    tbar_tgl = gtk_item_factory_get_widget(ife, "/View/Show Toolbar");

    if( GTK_CHECK_MENU_ITEM(tbar_tgl)->active ) {
	gtk_widget_show(toolbar);
    }
    else {
	gtk_widget_hide(toolbar);
    }
} /* toggle_toolbar_cb */

static void
save_cb(GtkWidget *w, gpointer data)
{
    deleteAllMessages();
    save_changes_proc();
    update_mail_list();
    display_new_message();
    sync_list();
    update_message_list();
    show_undelete(FALSE);
} /* save_cb */


/* Fill the comboBox with the folders listed in the "filemenu2" mailrc
   variable. */
static void
populate_combo()
{
    char	*p;
    GtkWidget	*combo;
    GList	*list = NULL;
    gchar	**strarray, *tmpstr, **ptr;

    p = find_mailrc("filemenu2");

    if((p == NULL) || (*p == '\0')){
	return;
    }

    tmpstr = g_strdelimit(p, " \t", ' ');
    strarray = g_strsplit(tmpstr, " ", 100);

    for(ptr = strarray; *ptr != NULL; ptr++) {
	if( **ptr == '\0')
	    continue;
	list = g_list_append(list, *ptr);
    }

    combo = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel), "combo");
    gtk_combo_set_popdown_strings(GTK_COMBO(combo), list);

    /* Clean up. */
    g_strfreev(strarray);
    g_list_free(list);
} /* populate_combo */

static void
load_folder_cb(GtkWidget *w, gpointer data)
{
    GtkWidget	*combo;
    gchar	*folder;

    combo = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel), "combo");
    folder = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry));

    /* add_to_combo(folder); */
    deleteAllMessages();
    load_file_proc(folder);
    update_mail_list();
    display_new_message();
    sync_list();
    update_message_list();
    show_undelete(FALSE);
} /* load_folder_cb */

static void
undelete_cb(GtkWidget *w, gpointer data)
{
    GtkWidget	*msglist;
    GList	*ptr;
    MESSAGE	*msg;

    /* Undelete all selected mail items.
     */
    msglist = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
					       "msglist");

    for(ptr = GTK_CLIST(msglist)->selection;
	ptr != NULL;
	ptr = g_list_next(ptr)) {
	msg = (MESSAGE *)gtk_clist_get_row_data(GTK_CLIST(msglist),
						(int)(ptr->data));
	undelete(msg);
    }
} /* undelete_cb */

static void
move_to_folder_cb(GtkWidget *w, gpointer data)
{
    /* Copy all selected mail items to the folder specified in the combo.
       If none are selected then move the currently displayed message.
       If the data flag is set, delete the message after copying it.
    */
    GtkWidget	*combo, *msglist;
    int		n = 0;
    GList	*ptr;
    gchar	*folder, *fullname, mess[BUFSIZ];
    MESSAGE	*msg;

    combo = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel), "combo");
    msglist = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
					       "msglist");
    folder = gtk_entry_get_text(GTK_ENTRY(GTK_COMBO(combo)->entry));

    fullname = expand_filename(folder);

    if(GTK_CLIST(msglist)->selection == NULL) {
	msg = last_message_read;
	if (!append_message_to_file (msg, fullname, FALSE)) {
	    n++;
	}
	else if (!failed_save_notice_proc()) {
	    return;
	}
    }
    else {
	for(ptr = GTK_CLIST(msglist)->selection;
	    ptr != NULL;
	    ptr = g_list_next(ptr)) {
	    msg = (MESSAGE *)gtk_clist_get_row_data(GTK_CLIST(msglist),
						    (int)(ptr->data));
	    if (!append_message_to_file (msg, fullname, FALSE)) {
		n++;
	    }
	    else if (!failed_save_notice_proc()) {
		return;
	    }
	}
    }

    if (!n)
	set_main_footer("No messages saved.");
    else {
	sprintf(mess, "%d messages %s to %s",
		n, ((int)data == 1)? "moved": "saved", fullname);
	set_main_footer(mess);
	/* check_folder_icon(folder); */
    }

    if((int)data == 1){
	delete_message_proc();
    }
}

static void
load_new_cb(GtkWidget *w, gpointer data)
{
    inbox_proc();
    update_mail_list();
    if( last_message_read ) {
	last_message_read->flags &= ~MESS_SELECTED;
	display_message_description(last_message_read);
    }
    display_new_message();
    sync_list();
    update_message_list();
    show_undelete(FALSE);
} /* load_new_cb */

static void
print_cb(GtkWidget *w, gpointer data)
{
    GtkWidget	*msglist;
    GList	*ptr;
    MESSAGE	*msg;

    /* Print all selected mail items. If there are none selected then
       print the currently displayed message.
     */
    msglist = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
					       "msglist");

    if( GTK_CLIST(msglist)->selection == NULL ) {
	last_message_read->flags |= MESS_SELECTED;
    }
    else {
	for(ptr = GTK_CLIST(msglist)->selection;
	    ptr != NULL;
	    ptr = g_list_next(ptr)) {
	    msg = (MESSAGE *)gtk_clist_get_row_data(GTK_CLIST(msglist),
						    (int)(ptr->data));
	    msg->flags |= MESS_SELECTED;
	}
    }
    print_cooked_proc();
} /* print_cb */

static void
show_folder_win_cb(GtkWidget *w, gpointer data)
{
    static GtkWidget	*fwin = NULL;
    gboolean		active;
    GtkWidget		*swin, *tree, *fldr_m, *fldr_t;
    GtkItemFactory	*ife;

    /* Get the menu toggle item and the toolbar toggle item so we can
       keep their toggled state in sync.
    */
    ife = (GtkItemFactory *)gtk_object_get_data(GTK_OBJECT(toplevel),
						"menufact");
    fldr_m = gtk_item_factory_get_widget(ife, "/View/Folders");
    fldr_t = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
					      "folderbtn");

    if(w == NULL) {
	/* Must have been the menu toggle activated. */
	active = GTK_CHECK_MENU_ITEM(fldr_m)->active;
    }
    else {
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w));
    }

    if( active ) {
	if( !fwin) {
	    fwin = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	    gtk_widget_set_usize(GTK_WIDGET(fwin), 200, 400);

	    gtk_signal_connect(GTK_OBJECT(fwin), "delete_event",
			       GTK_SIGNAL_FUNC (delete_event), (gpointer)1);

	    swin = gtk_scrolled_window_new(NULL, NULL);
	    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
					   GTK_POLICY_AUTOMATIC,
					   GTK_POLICY_AUTOMATIC);
	    gtk_container_add(GTK_CONTAINER(fwin), swin);

	    tree = gtk_ctree_new(2, 1);
	    gtk_ctree_set_spacing(GTK_CTREE(tree), 4);
	    gtk_ctree_set_show_stub(GTK_CTREE(tree), TRUE);
	    gtk_clist_set_row_height(GTK_CLIST(tree), 20);
	    gtk_clist_columns_autosize(GTK_CLIST(tree));
	    gtk_clist_set_selection_mode(GTK_CLIST(tree),
					 GTK_SELECTION_SINGLE);
	    gtk_clist_set_button_actions(GTK_CLIST(tree), 0,
					 GTK_BUTTON_SELECTS|GTK_BUTTON_EXPANDS);
	    gtk_clist_set_sort_column(GTK_CLIST(tree), 1);
	    gtk_container_add(GTK_CONTAINER(swin), tree);

	    gtk_signal_connect(GTK_OBJECT(tree), "tree_select_row",
			       GTK_SIGNAL_FUNC (folder_select_cb), NULL);

	    gtk_widget_show(GTK_WIDGET(tree));
	    gtk_widget_show(GTK_WIDGET(swin));

	    fillin_folders(tree);
	}

	gtk_widget_show(GTK_WIDGET(fwin));
	gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(fldr_m), TRUE);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fldr_t), TRUE);
    }
    else {
	if(fwin) {
	    gtk_widget_hide(GTK_WIDGET(fwin));
	    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(fldr_m), FALSE);
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(fldr_t), FALSE);
	}
    }
} /* show_folder_win_cb */


static void
fillin_folders(GtkWidget *tree)
{
    char		*folder;    /* Directory containing mail folders. */
    char		*fullfolder;

    if ((folder = (char *)find_mailrc("folder"))){
	if(strchr(folder, '/') != NULL){ /* Is a specified path. */
	    fullfolder = g_strdup(folder);
	}else{			/* Relative to home directory. */
	    fullfolder = g_strconcat(g_get_home_dir(), "/", folder, NULL);
	}

	process_dir(tree, fullfolder, NULL);
	gtk_ctree_sort_recursive(GTK_CTREE(tree), NULL);
	g_free(fullfolder);
    }
} /* fillin_folders */

/* Add nodes to the ctree, representing the files and
   directories in the given starting directory. This descends into
   subdirectories recursively until the entire sub-tree is covered.
 */
static void
process_dir(GtkWidget *tree, char *dirname, GtkCTreeNode *parent)
{
    char		*pathname;
    GtkCTreeNode	*node;
    gchar		*name[3], *tag;
    DIR			*dirp;
    struct dirent	*de;
    struct stat		statbuf;
    static GdkPixmap	*pixmap = NULL, *folder_p = NULL, *ofolder_p = NULL;
    static GdkBitmap	*mask, *folder_m, *ofolder_m;
    GtkStyle		*style;

    if((dirp = opendir(dirname)) == NULL){
	perror(dirname);
	return;
    }

    if( !pixmap ) {
	style = gtk_widget_get_style(toplevel);
	pixmap = gdk_pixmap_create_from_xpm_d(toplevel->window,
					      &mask,
					      &style->bg[GTK_STATE_NORMAL],
					      letter_xpm);
	folder_p = gdk_pixmap_create_from_xpm_d(toplevel->window,
						&folder_m,
						&style->bg[GTK_STATE_NORMAL],
						mini_folder_xpm);
	ofolder_p = gdk_pixmap_create_from_xpm_d(toplevel->window,
						 &ofolder_m,
						 &style->bg[GTK_STATE_NORMAL],
						 mini_ofolder_xpm);
    }

    name[0] = name[2] = NULL;

    /* Clean out the tree.
       (but only if we're at the top i.e parent == NULL) */
    if(parent == NULL) {
	gtk_clist_clear(GTK_CLIST(tree));
    }

    /* Now fill with new ones. */
    while((de = readdir(dirp)) != NULL){

	if(!strcmp(de->d_name, ".")) /* No need for this. */
	    continue;
	if(!strcmp(de->d_name, ".."))
	    continue;		/* No '..'. */

	pathname = g_strconcat(dirname, "/", de->d_name, NULL);
	stat(pathname, &statbuf);

	name[1] = de->d_name;
	if(S_ISDIR(statbuf.st_mode)) {
	    node = gtk_ctree_insert_node(GTK_CTREE(tree), parent, NULL,
					 name, 4, folder_p, folder_m,
					 ofolder_p, ofolder_m,
					 FALSE, FALSE);
	}
	else {
	    node = gtk_ctree_insert_node(GTK_CTREE(tree), parent, NULL,
					 name, 4, pixmap, mask,
					 NULL, NULL,
					 TRUE, FALSE);
	}
	tag = g_strconcat((parent == NULL)? "+":
			  gtk_ctree_node_get_row_data(GTK_CTREE(tree), parent),
			  "/", de->d_name, NULL);
	gtk_ctree_node_set_row_data(GTK_CTREE(tree), node, tag);

	if(S_ISDIR(statbuf.st_mode)) { /* recurse into sub-directory */
	    process_dir(tree, pathname, node);
	}
	g_free(pathname);
    }
    closedir(dirp);
} /* process_dir */

static void
folder_select_cb(GtkWidget *w, GtkCTreeNode *node, gint col)
{
    gboolean	is_leaf;
    gchar	*text;
    GtkWidget	*combo;
    GdkEvent	*event;

    gtk_ctree_get_node_info(GTK_CTREE(w), node, &text,
			    NULL, NULL, NULL, NULL, NULL, &is_leaf, NULL);
    text = gtk_ctree_node_get_row_data(GTK_CTREE(w), node);
    if(is_leaf) {
	combo = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
						 "combo");
	gtk_entry_set_text(GTK_ENTRY(GTK_COMBO(combo)->entry), text);

	/* Double click will load the folder.
	 */
	event = gtk_get_current_event();
	if(event && event->type == GDK_2BUTTON_PRESS) {
	    deleteAllMessages();
	    load_file_proc(text);
	    update_mail_list();
	    display_new_message();
	    sync_list();
	    update_message_list();
	}
    }
} /* folder_select_cb */

static void
compose_cb(GtkWidget *w, gpointer data)
{
    COMPOSE_WINDOW	*win = NULL;
    COMPOSE_TYPE	comp_type;

    comp_type = (COMPOSE_TYPE)data;

    if ((comp_type & COMPOSE_REPLY) && !last_message_read)
	return;

    /* Get a window to use. */
    if((win = setup_send_window()) == NULL)
	return;

    initialise_compose_win(win, comp_type);

    win->in_use = 1;

    gtk_widget_show(GTK_WIDGET(win->deliver_frame));
} /* compose_cb */

static COMPOSE_WINDOW *
compose_find_free()
{
    COMPOSE_WINDOW	*win;
    GtkWidget		*vbox, *hbox, *w, *align, *table, *swin;
    int			i;

    win = compose_first;

    while (win) {
	if (!win->in_use)
	    return win;

	win = win->next;
    }

    win = g_malloc( sizeof(COMPOSE_WINDOW));

    /* Add win into the list of compose windows.
     */

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

    /* Now create all the gui bits for the compose window.
     */
    win->deliver_frame = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(win->deliver_frame), "Privtool - Compose");
    gtk_container_set_border_width(GTK_CONTAINER(win->deliver_frame), 4);

    vbox = gtk_vbox_new(FALSE, 4);
    gtk_container_add(GTK_CONTAINER(win->deliver_frame), vbox);

    table = gtk_table_new(MAX_EXTRA_HEADERLINES + 4, 2, FALSE);
    /*gtk_table_set_row_spacings(GTK_TABLE(table), 4);
      gtk_table_set_col_spacings(GTK_TABLE(table), 4);*/

    align = gtk_alignment_new(1.0, 0.0, 0.0, 0.0);
    w = gtk_label_new("To:");
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_table_attach(GTK_TABLE(table), align, 0, 1, 0, 1,
		     GTK_FILL, 0, 4, 0);
    gtk_widget_show(w);
    gtk_widget_show(align);
    win->send_to = gtk_entry_new();
    gtk_table_attach_defaults(GTK_TABLE(table), win->send_to, 1, 2, 0, 1);
    gtk_widget_show(win->send_to);

    align = gtk_alignment_new(1.0, 0.0, 0.0, 0.0);
    w = gtk_label_new("Subject:");
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_table_attach(GTK_TABLE(table), align, 0, 1, 1, 2,
		     GTK_FILL, 0, 4, 0);
    gtk_widget_show(w);
    gtk_widget_show(align);
    win->send_subject = gtk_entry_new();
    gtk_table_attach_defaults(GTK_TABLE(table), win->send_subject, 1, 2, 1, 2);
    gtk_widget_show(win->send_subject);

    align = gtk_alignment_new(1.0, 0.0, 0.0, 0.0);
    w = gtk_label_new("CC:");
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_table_attach(GTK_TABLE(table), align, 0, 1, 2, 3,
		     GTK_FILL, 0, 4, 0);
    gtk_widget_show(w);
    gtk_widget_show(align);
    win->send_cc = gtk_entry_new();
    gtk_table_attach_defaults(GTK_TABLE(table), win->send_cc, 1, 2, 2, 3);
    gtk_widget_show(win->send_cc);

    align = gtk_alignment_new(1.0, 0.0, 0.0, 0.0);
    w = gtk_label_new("Bcc:");
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_table_attach(GTK_TABLE(table), align, 0, 1, 3, 4,
		     GTK_SHRINK|GTK_FILL, 0, 0, 0);
    gtk_widget_show(w);
    win->send_bcc = gtk_entry_new();
    gtk_table_attach_defaults(GTK_TABLE(table), win->send_bcc, 1, 2, 3, 4);
    if(find_mailrc("askbcc")) {
	gtk_widget_show(align);
	gtk_widget_show(win->send_bcc);
    }

    for (i = 1; i < MAX_EXTRA_HEADERLINES; i++) {
	char	mailrcline[BUFSIZ];
	char	*headerline;

	sprintf(mailrcline, "header%d", i);

	if ((headerline = find_mailrc(mailrcline))) {
	    char	 *label;

	    label = g_strconcat(headerline, ":", NULL);
	    align = gtk_alignment_new(1.0, 0.0, 0.0, 0.0);
	    w = gtk_label_new(label);
	    gtk_container_add(GTK_CONTAINER(align), w);
	    gtk_table_attach(GTK_TABLE(table), align, 0, 1, i+3, i+4,
			     GTK_SHRINK|GTK_FILL, 0, 0, 0);
	    gtk_widget_show(w);
	    gtk_widget_show(align);
	    win->extra_headers[i] = gtk_entry_new();
	    gtk_table_attach_defaults(GTK_TABLE(table), win->extra_headers[i],
				      1, 2, i+3, i+4);
	    gtk_widget_show(win->extra_headers[i]);
	    g_free(label);
	}else{
	    win->extra_headers[i] = NULL;
	}
    }

    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 4);
    gtk_widget_show(table);

    hbox = gtk_hbox_new(FALSE, 4);

    win->sign = gtk_check_button_new_with_label("Sign");
    gtk_box_pack_start(GTK_BOX(hbox), win->sign, FALSE, TRUE, 4);
    gtk_widget_show(win->sign);

    win->encrypt = gtk_check_button_new_with_label("Encrypt");
    gtk_box_pack_start(GTK_BOX(hbox), win->encrypt, FALSE, TRUE, 4);
    gtk_widget_show(win->encrypt);

    win->log = gtk_check_button_new_with_label("Log");
    gtk_box_pack_start(GTK_BOX(hbox), win->log, FALSE, TRUE, 4);
    gtk_widget_show(win->log);

    win->raw = gtk_check_button_new_with_label("Raw");
    gtk_box_pack_start(GTK_BOX(hbox), win->raw, FALSE, TRUE, 4);
    gtk_widget_show(win->raw);

#ifdef HAVE_MIXMASTER
    win->remail = gtk_check_button_new_with_label("Remail");
    gtk_box_pack_start(GTK_BOX(hbox), win->remail, FALSE, TRUE, 4);
    gtk_widget_show(win->remail);
#endif

    align = gtk_alignment_new(1.0, 0.5, 0.0, 0.0);
    w = gtk_button_new_with_label("Insert...");
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_box_pack_start(GTK_BOX(hbox), align, TRUE, TRUE, 8);
    gtk_object_set_user_data(GTK_OBJECT(w), win);
    gtk_signal_connect(GTK_OBJECT(w), "clicked",
		       GTK_SIGNAL_FUNC (filer_cb), insert_file_cb);
    gtk_widget_show(w);
    gtk_widget_show(align);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 4);
    gtk_widget_show(hbox);

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
				   GTK_POLICY_AUTOMATIC,
				   GTK_POLICY_AUTOMATIC);
    win->text = gtk_text_new(NULL, NULL);
    gtk_widget_set_usize(GTK_WIDGET(win->text), 550, 450);
    gtk_text_set_editable(GTK_TEXT(win->text), TRUE);
    gtk_container_add(GTK_CONTAINER(swin), win->text);
    gtk_box_pack_start(GTK_BOX(vbox), swin, TRUE, TRUE, 4);
    gtk_drag_dest_set(win->text, GTK_DEST_DEFAULT_ALL, targets, 1,
		      GDK_ACTION_COPY);
    gtk_signal_connect(GTK_OBJECT(win->text), "drag_drop",
		       GTK_SIGNAL_FUNC(msgdrop_cb), win);

    gtk_signal_connect_object(GTK_OBJECT(win->text), "button_press_event",
			      GTK_SIGNAL_FUNC(compmenu_post_cb),
			      GTK_OBJECT(win->text));
    gtk_signal_connect(GTK_OBJECT(win->text), "insert_text",
		       GTK_SIGNAL_FUNC(textins_cb), win);

    gtk_widget_show(win->text);
    gtk_widget_show(swin);

    w = gtk_hseparator_new();
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, TRUE, 4);
    gtk_widget_show(w);

    hbox = gtk_hbox_new(TRUE, 4);

    align = gtk_alignment_new(0.5, 0.5, 0.4, 0.4);
    w = gtk_button_new_with_label("Send");
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_box_pack_start(GTK_BOX(hbox), align, TRUE, TRUE, 4);
    gtk_widget_show(w);
    gtk_widget_show(align);
    gtk_object_set_user_data(GTK_OBJECT(w), (gpointer)0);
    gtk_signal_connect(GTK_OBJECT(w), "clicked",
		       GTK_SIGNAL_FUNC (deliver_cb), win);

    align = gtk_alignment_new(0.5, 0.5, 0.4, 0.4);
    w = gtk_button_new_with_label("Cancel");
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_box_pack_start(GTK_BOX(hbox), align, FALSE, TRUE, 4);
    gtk_widget_show(w);
    gtk_widget_show(align);
    gtk_object_set_user_data(GTK_OBJECT(w), (gpointer)1);
    gtk_signal_connect(GTK_OBJECT(w), "clicked",
		       GTK_SIGNAL_FUNC (deliver_cb), win);

    gtk_object_set_user_data(GTK_OBJECT(win->deliver_frame), (gpointer)1);
    gtk_signal_connect(GTK_OBJECT(win->deliver_frame), "delete_event",
		       GTK_SIGNAL_FUNC (deliver_cb), win);

    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, TRUE, 4);
    gtk_widget_show(hbox);

    gtk_widget_show(vbox);

    reset_deliver_flags(win);

    return win;
} /* compose_find_free */

static void
add_signature(COMPOSE_WINDOW *win)
{
    FILE	*sig_fp;
    char	*sig_name, *file, buf[BUFSIZE];
    int		l, ispipe;
	
    if (!(sig_name = find_mailrc ("sigfile")) || !*sig_name)
	return;

    if((*sig_name != '/') && (*sig_name != '|')) {
	file = g_strconcat(g_get_home_dir(), "/", sig_name, NULL);
    }
    else {
	file = g_strdup(sig_name);
    }

    if(*file == '|') {
	sig_fp = popen(file + 1, "rt");
	ispipe = 1;
    }
    else {
	sig_fp = fopen(file, "rt");
	ispipe = 0;
    }

    if(sig_fp == NULL) {
	g_free(file);
	return;
    }

    gtk_text_insert(GTK_TEXT(win->text), NULL, NULL, NULL, "\n--\n", -1);
    while(!feof(sig_fp)) {
	l = fread(buf, 1, BUFSIZE, sig_fp);
	if(l) 
	    gtk_text_insert(GTK_TEXT(win->text), NULL, NULL, NULL, buf, l);
    }
    gtk_text_set_point(GTK_TEXT(win->text), 0);

    g_free(file);
    ispipe? pclose(sig_fp): fclose(sig_fp);
} /* add_signature */

static void
reset_deliver_flags(COMPOSE_WINDOW *win)
{
    int		log, dontlog;
    char	*nym;

    /* Default to encryption and signature */
    win->deliver_flags = DELIVER_SIGN|DELIVER_ENCRYPT;

    /* Default to remail if using a nym */
    nym = current_nym();
    if (strcmp(nym, our_userid)) 
	win->deliver_flags |= DELIVER_REMAIL;

    /* Clear the bits if told to by mailrc */
    if (find_mailrc("nodefaultsign"))
	win->deliver_flags &= ~DELIVER_SIGN;
    if (find_mailrc("nodefaultencrypt"))
	win->deliver_flags &= ~DELIVER_ENCRYPT;
    if (find_mailrc("nodefaultremail"))
	win->deliver_flags &= ~DELIVER_REMAIL;

    /* Check mailrc for logging options */
    log = (find_mailrc("nodontlogmessages") != NULL);
    dontlog = (find_mailrc("dontlogmessages") != NULL);

    /* If logging enabled, set log options */
    if (log && !dontlog) {
	win->deliver_flags |= DELIVER_LOG;
    }

    if (find_mailrc("log-raw")) {
	win->deliver_flags |= DELIVER_RAW;
    }
} /* reset_deliver_flags */

static void
initialise_compose_win(COMPOSE_WINDOW *win, COMPOSE_TYPE comptype)
{
    BUFFER		*b;
    int			i;
    char		*send_to;
    gchar		**strarry, *quoted;

    /* Disable updates in the GtkText for a bit */
    gtk_text_freeze(GTK_TEXT(win->text));

    /* Now clear all fields (in case they were used before). */
    gtk_entry_set_text(GTK_ENTRY(win->send_to), "");
    gtk_entry_set_text(GTK_ENTRY(win->send_cc), "");
    gtk_entry_set_text(GTK_ENTRY(win->send_subject), "");
    gtk_entry_set_text(GTK_ENTRY(win->send_bcc), "");

    gtk_text_set_point(GTK_TEXT(win->text), 0);
    gtk_text_forward_delete(GTK_TEXT(win->text),
			    gtk_text_get_length(GTK_TEXT(win->text)));

    for(i = 1; i < MAX_EXTRA_HEADERLINES; i++) {
	if(win->extra_headers[i] != NULL){
	    gtk_entry_set_text(GTK_ENTRY(win->extra_headers[i]), "");
	}
    }

    reset_deliver_flags(win);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->sign),
				 win->deliver_flags & DELIVER_SIGN);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->encrypt),
				 win->deliver_flags & DELIVER_ENCRYPT);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->log),
				 win->deliver_flags & DELIVER_LOG);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->raw),
				 win->deliver_flags & DELIVER_RAW);
#ifdef HAVE_MIXMASTER
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(win->remail),
				 win->deliver_flags & DELIVER_REMAIL);
#endif

    switch(comptype & 0x0f){
    case COMPOSE_NEW:
	/* Do something to make keyboard focus go to the send_to field */
	gtk_widget_grab_focus(win->send_to);
	break;

    case COMPOSE_REPLY:
	set_reply (last_message_read);

	send_to = last_message_read->email;

	if (last_message_read->reply_to &&
		!find_mailrc("defaultusefrom")) {
		if (find_mailrc("defaultusereplyto") ||
			(alert(win->deliver_frame,
			       "Use the Reply-To field instead of the From field?",
			      ALERT_QUESTION, 2, "Yes", "No") == 1)) {
			send_to = last_message_read->reply_to;
		}
	}

	gtk_entry_set_text(GTK_ENTRY(win->send_to), send_to);

	if((comptype & R_ALL) || (comptype & R_ALL_INCLUDE)) {
	    send_to = extract_addresses(last_message_read->to);
	    if(send_to && *send_to != '\0') {
		gtk_entry_append_text(GTK_ENTRY(win->send_to), ", ");
		gtk_entry_append_text(GTK_ENTRY(win->send_to), send_to);
	    }
	    send_to = extract_addresses(last_message_read->cc);
	    if(send_to && *send_to != '\0') {
		gtk_entry_set_text(GTK_ENTRY(win->send_cc), send_to);
	    }
	}

	if(last_message_read->subject != NULL){
	    if(strncasecmp(last_message_read->subject, "Re:", 3))
		gtk_entry_set_text(GTK_ENTRY(win->send_subject), "Re: ");

	    gtk_entry_append_text(GTK_ENTRY(win->send_subject),
				  last_message_read->subject);
	}

	if(comptype &= (R_SENDER_INCLUDE|R_ALL_INCLUDE)){
	    if (last_message_read->decrypted)
		b = last_message_read->decrypted;
	    else
		b = message_contents(last_message_read);

	    gtk_text_insert(GTK_TEXT(win->text), NULL, NULL, NULL,
			  last_message_read->sender, -1);
	    gtk_text_insert(GTK_TEXT(win->text), NULL, NULL, NULL,
			    attribution_string, -1);
	    strarry = g_strsplit(g_strstrip(b->message), "\n", -1);
	    quoted = g_strconcat("> ", g_strjoinv("\n> ", strarry), NULL);
	    gtk_text_insert(GTK_TEXT(win->text), NULL, NULL, NULL,
			    quoted, -1/*strlen(quoted) - 2*/);
	    gtk_text_insert(GTK_TEXT(win->text), NULL, NULL, NULL, "\n", 1);
	    g_free(quoted);
	    g_strfreev(strarry);
	}
	gtk_widget_grab_focus(win->text);
	break;

    case COMPOSE_FORWARD:
	gtk_entry_set_text(GTK_ENTRY(win->send_subject),
			   last_message_read->subject);
	gtk_entry_append_text(GTK_ENTRY(win->send_subject), " (fwd)");

	if (last_message_read->decrypted)
		b = last_message_read->decrypted;
	else
		b = message_contents(last_message_read);

	gtk_text_insert(GTK_TEXT(win->text), NULL, NULL, NULL,
			begin_forward, -1);
	gtk_text_insert(GTK_TEXT(win->text), NULL, NULL, NULL,
			b->message, -1);
	gtk_text_insert(GTK_TEXT(win->text), NULL, NULL, NULL,
			end_forward, -1);

	gtk_widget_grab_focus(win->send_to);
	break;

    case COMPOSE_RESEND:
	alert(win->deliver_frame,
	      "Resend hasn't been implemented yet.\nHassle the developers if you want it.",
	      ALERT_MESSAGE, 1, "OK");
	break;
    }

    add_signature(win);
    gtk_text_thaw(GTK_TEXT(win->text));
} /* initialise_compose_win */

static void
deliver_cb(GtkWidget *w, gpointer data)
{
    COMPOSE_WINDOW	*win = (COMPOSE_WINDOW *)data;

    if((int)gtk_object_get_user_data(GTK_OBJECT(w))) {
	/* Cancel pushed. */
	if( (gtk_text_get_length(GTK_TEXT(win->text)) == 0) ||
	    alert(win->deliver_frame,
		  "Cancelling a non-empty message.\n\nReally cancel?",
		  ALERT_QUESTION, 2, "Yes", "No") == 1)
	    gtk_widget_hide(win->deliver_frame);
    }
    else {
	/* OK Pushed */
	if( (gtk_text_get_length(GTK_TEXT(win->text)) != 0) ||
	    alert(win->deliver_frame,
		  "Sending an empty message.\n\nReally send?",
		  ALERT_QUESTION, 2, "Yes", "No") == 1)
	    deliver_proc(win);
    }
    win->in_use = 0;
} /* deliver_cb */

static void
filer_cb(GtkWidget *w, gpointer data)
{
    GtkWidget	*filer;

    filer = gtk_file_selection_new("Privtool - Select File");
    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filer)->ok_button),
		       "clicked",
		       GTK_SIGNAL_FUNC (data), filer);
    gtk_signal_connect(GTK_OBJECT(GTK_FILE_SELECTION(filer)->cancel_button),
		       "clicked",
		       GTK_SIGNAL_FUNC (data), filer);
    gtk_signal_connect(GTK_OBJECT(&GTK_FILE_SELECTION(filer)->window),
		       "delete_event",
		       GTK_SIGNAL_FUNC (data), filer);
    gtk_object_set_user_data(GTK_OBJECT(filer),
			     gtk_object_get_user_data(GTK_OBJECT(w)));

    gtk_widget_show(filer);
} /* filer_cb */

static void
insert_file_cb(GtkWidget *w, gpointer data)
{
    int			fd;
    gchar		*filename, *error, *buf;
    GtkWidget		*filer;
    struct stat		stbuf;
    COMPOSE_WINDOW	*win;

    filer = (GtkWidget *)data;
    win = (COMPOSE_WINDOW *)gtk_object_get_user_data(GTK_OBJECT(filer));

    if( GTK_IS_WIDGET(w) &&
	w == (GTK_FILE_SELECTION(filer)->ok_button)) {
	filename = gtk_file_selection_get_filename(GTK_FILE_SELECTION(filer));

	stat(filename, &stbuf);
	if(S_ISREG(stbuf.st_mode)){
	    if((fd = open(filename, O_RDONLY)) == -1){
		error = g_strconcat("File could not be opened, insert failed\n",
				    strerror(errno), NULL);
		alert(win->deliver_frame, error, ALERT_ERROR, 1, "OK");
		g_free(error);
	    }
	    else{
		buf = mmap(NULL, stbuf.st_size, PROT_READ, MAP_SHARED, fd, 0L);
		if(buf == (char *)-1){
		    error = g_strconcat("File could not be mapped, insert failed\n",
					strerror(errno), NULL);
		    alert(win->deliver_frame, error, ALERT_ERROR, 1, "OK");
		    g_free(error);
		}
		else{
		    gtk_text_freeze(GTK_TEXT(win->text));
		    gtk_text_insert(GTK_TEXT(win->text), NULL, NULL, NULL,
				    buf, -1);
		    gtk_text_thaw(GTK_TEXT(win->text));
		    munmap(buf, stbuf.st_size);
		}
		close(fd);
	    }
	}
	else{
	    error = g_strconcat("The filename specified is not a regular file.\nIt cannot be inserted.\n",
				strerror(errno), NULL);
	    alert(win->deliver_frame, error, ALERT_ERROR, 1, "OK");
	    g_free(error);
	}
    }

    gtk_widget_destroy(filer);
} /* insert_file_cb */

static gint
compmenu_post_cb(GtkWidget *w, GdkEventButton *event)
{
    GtkWidget		*menu;

    if(event && (event->button == 3)) {
	menu = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel),
						"compmenu");
	gtk_object_set_user_data(GTK_OBJECT(menu), (gpointer)w);
	gtk_menu_popup(GTK_MENU(menu), 0, 0, 0, 0, event->button,
		       event->time);
	return TRUE;
    }

    return FALSE;
} /* compmenu_post_cb */

static void
compmenu_cb(GtkWidget *w, gpointer data)
{
    GdkEvent	*event = (GdkEvent *)gtk_get_current_event();
    GtkWidget	*menu, *text;

    menu = gtk_get_event_widget(event)->parent;
    text = (GtkWidget *)gtk_object_get_user_data(GTK_OBJECT(menu));

    switch((EDIT_OP)data) {
    case EDIT_CUT:
	gtk_editable_cut_clipboard(GTK_EDITABLE(text));
	break;
    case EDIT_COPY:
	gtk_editable_copy_clipboard(GTK_EDITABLE(text));
	break;
    case EDIT_PASTE:
	gtk_editable_paste_clipboard(GTK_EDITABLE(text));
	break;
    case EDIT_QUOTE:
    case EDIT_WRAP:
	break;
    }
} /* compmenu_cb */

static void
msgdrop_cb(GtkWidget *w, GdkDragContext *ctxt, gint x, gint y, guint etime,
	   gpointer data)
{
    GtkWidget	*source;

    source = gtk_drag_get_source_widget(ctxt);
    g_warning("In msgdrop_cb, the penny dropped (%d, %d)!", x, y);

    /* Quit if the drop is from an external source or from a non-clist
       widget. We only want to receive messages from the message list.
    */
    if((source == NULL) || !GTK_IS_CLIST(source)) {
	gtk_drag_finish(ctxt, FALSE, FALSE, etime);
	return;
    }

    gtk_drag_finish(ctxt, FALSE, FALSE, etime);
} /* msgdrop_cb */

static void
edit_ops_cb(GtkWidget *w, gpointer data)
{
    GtkWidget	*msg;
    EDIT_OP	op = (EDIT_OP)data;

    msg = (GtkWidget *)gtk_object_get_data(GTK_OBJECT(toplevel), "msgtext");
    if(op == EDIT_COPY) {
	gtk_editable_copy_clipboard(GTK_EDITABLE(msg));
    }
} /* edit_ops_cb */

/* Implement "wrapping as you type" functionality. Note that this only wraps
   the text already inserted into the GtkText widget, the "current" text
   has no effect.
 */

static void
textins_cb(GtkWidget *w, gchar *text, gint length, gint *position)
{
    int		nlpos, margin;
    char	*msg, *last_nl, *last_space, *prev_space, *temp;

    if(((temp = find_mailrc("wrapmargin")) == NULL) || (*temp == '\0')) {
	margin = 0;
    }
    else {
	margin = atoi(temp);
    }

    if(margin == 0)
	return;

    msg = gtk_editable_get_chars(GTK_EDITABLE(w), 0, *position);

    if((last_nl = strrchr(msg, '\n')) == NULL)
	last_nl = msg;

    nlpos = last_nl - msg;
    if(*position >= (nlpos + margin)) {
	gtk_text_freeze(GTK_TEXT(w));
	for(last_space = strchr(last_nl, ' ');
	    last_space != NULL;
	    last_space = strchr(last_space+1, ' ')) {
	    if((last_space - last_nl) >= margin) {
		gtk_text_set_point(GTK_TEXT(w), (prev_space - msg));
		gtk_text_forward_delete(GTK_TEXT(w), 1);
		gtk_text_insert(GTK_TEXT(w), NULL, NULL, NULL, "\n", 1);
		last_nl = prev_space;
	    }
	    prev_space = last_space;
	}
	nlpos = last_nl - msg;
	if(*position >= (nlpos + margin)) {
	    last_space = strrchr(last_nl, ' ');
	    if(last_space) {
		gtk_text_set_point(GTK_TEXT(w), (last_space - msg));
		gtk_text_forward_delete(GTK_TEXT(w), 1);
		gtk_text_insert(GTK_TEXT(w), NULL, NULL, NULL, "\n", 1);
	    }
	}
	gtk_text_thaw(GTK_TEXT(w));
    }
    gtk_text_set_point(GTK_TEXT(w), *position);
} /* textins_cb */

/* $Id$ 
 *
 * Copyright   : (c) 1999 by Glenn Trigg.  All Rights Reserved
 * Project     : Gtk Privtool
 * File        : gprops
 *
 * Author      : Glenn Trigg
 * Created     :  8 Jun 1999 
 *
 * Description : Properties window in Gtk
 */

#ifdef HAVE_CONFIG_H
#include	"config.h"
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<sys/param.h>
#include	<gtk/gtk.h>
#include	<gtk/gtksheet.h>
#include	<string.h>

#include	"def.h"
#include	"buffers.h"
#include	"message.h"
#include	"mailrc.h"
#include	"windows.h"
#include	"gui.h"
#include	"gtk_protos.h"
#include	"main.h"

typedef struct _mail_header_field {
    gchar	*name;
    gboolean	editable;
} MAIL_HEADER_FIELD;

/* Standard mail header field names, taken from rfc822. */
MAIL_HEADER_FIELD standard_headers[] = {
    {"To", TRUE},
    {"Subject", TRUE},
    {"Date", FALSE},
    {"From", FALSE},
    {"cc", TRUE},
    {"bcc", TRUE},
    {"Comments", TRUE},
    {"In-Reply-To", FALSE},
    {"Keywords", TRUE},
    {"Message-ID", FALSE},
    {"Received", FALSE},
    {"References", TRUE},
    {"Reply-To", FALSE},
    {"Resent-Date", FALSE},
    {"Resent-From", FALSE},
    {"Resent-Message-ID", FALSE},
    {"Resent-Reply-To", FALSE},
    {"Resent-Sender", FALSE},
    {"Resent-To", FALSE},
    {"Resent-bcc", FALSE},
    {"Resent-cc", FALSE},
    {"Return-path", FALSE},
    {"Sender", FALSE},
    {NULL, FALSE}
};

extern char		globRCfile[MAXPATHLEN];  /* from main.c */
extern LIST		alias, retain;

typedef enum {
    CANCEL,
    OK,
    APPLY
} DialogReason;

typedef enum {
    REPLY_TO,
    FROM,
    ASK
} ReplyStyle;

typedef struct {
    GtkWidget	*mail_dir;		/* folder */
    GtkWidget	*mail_spool;		/* incoming mail spool file */
    GtkWidget	*mail_menu;		/* filemenu2 */
    GtkWidget	*detached;
    GtkWidget	*aliases;
    GtkWidget	*pgpkeys;
    GtkWidget	*security_level;
    GtkWidget	*test_interval;
    GtkWidget	*check_interval;
    GtkWidget	*pseudonyms;
    GtkWidget	*kill_list;
    GtkWidget	*default_nym;
    GtkWidget	*hdr_list;
    GtkWidget	*replyto;		/* replyto */
    GtkWidget	*replystyle;		/* replyto */
    GtkWidget	*comp_sign;		/* nodefaultsign */
    GtkWidget	*comp_encrypt;		/* nodefaultencrypt */
    GtkWidget	*comp_remail;		/* nodefaultremail */
    GtkWidget	*comp_log;		/* nodontlogmessages */
    GtkWidget	*comp_lograw;		/* log-raw */
    GtkWidget	*print_cmd;		/* printmail */
    GtkWidget	*domain;		/* domain */
    GtkWidget	*indent;		/* indentprefix */
    GtkWidget	*organizn;		/* organization */
    GtkWidget	*record;		/* record */
    GtkWidget	*badbeep;		/* nobeepbadsig */
    GtkWidget	*sigfile;		/* signature file */
    GtkWidget	*wrapcol;		/* wrap column */
} PropWidgets;

static PropWidgets	propw;

static void	proptop_cb(GtkWidget *, gpointer);
static gint	propdelete_cb(GtkWidget *, GdkEvent *, gpointer);
static GtkWidget *create_mailer_page(void);
static GtkWidget *create_compose_page(void);
static GtkWidget *create_alias_page(void);
static GtkWidget *create_headers_page(void);
static GtkWidget *create_pgp_page(void);
static void	load_mailer_page(void);
static void	load_compose_page(void);
static void	load_aliases_page(void);
static void	load_pgp_page(void);
static void	cell_change_cb(GtkWidget *, gint, gint, gpointer);
static void	reply_cb(GtkWidget *, gpointer);

static GtkItemFactoryEntry reply_ife[] = {
    {"/Use Reply-To:", NULL, (GtkItemFactoryCallback)reply_cb, REPLY_TO, NULL},
    {"/Use From:", NULL, (GtkItemFactoryCallback)reply_cb, FROM, NULL},
    {"/Ask", NULL, (GtkItemFactoryCallback)reply_cb, ASK, NULL}};

/*----------------------------------------------------------------------*/

void
show_props()
{
    static GtkWidget	*proptop = NULL;
    GtkWidget		*button, *notebook;

    if(proptop == NULL) {
	proptop = gtk_dialog_new();
	gtk_window_set_title(GTK_WINDOW(proptop), "Privtool - Edit Properties");
	gtk_signal_connect(GTK_OBJECT(proptop), "delete_event",
			   GTK_SIGNAL_FUNC(propdelete_cb), proptop);
	gtk_container_set_border_width(GTK_CONTAINER(GTK_DIALOG(proptop)->vbox),
				       2);

	/* Create the notebook.
	 */
	notebook = gtk_notebook_new();
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(proptop)->vbox),
				    notebook);
	gtk_notebook_set_show_tabs(GTK_NOTEBOOK(notebook), TRUE);
	gtk_notebook_set_tab_pos(GTK_NOTEBOOK(notebook), GTK_POS_TOP);
	gtk_notebook_set_homogeneous_tabs(GTK_NOTEBOOK(notebook), TRUE);
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
				 create_mailer_page(),
				 gtk_label_new("Mailer"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
				 create_compose_page(),
				 gtk_label_new("Compose"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
				 create_alias_page(),
				 gtk_label_new("Aliases"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
				 create_headers_page(),
				 gtk_label_new("Mail Headers"));
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook),
				 create_pgp_page(),
				 gtk_label_new("Security"));
	gtk_widget_show(notebook);

	/* Add the bottom dialog buttons.
	 */
	button = gtk_button_new_with_label("OK");
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(proptop)->action_area),
				    button);
	gtk_object_set_user_data(GTK_OBJECT(button), (gpointer)OK);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", (GtkSignalFunc)proptop_cb,
			   proptop);
	gtk_widget_show(button);

	button = gtk_button_new_with_label("Apply");
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(proptop)->action_area),
				    button);
	gtk_object_set_user_data(GTK_OBJECT(button), (gpointer)APPLY);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", (GtkSignalFunc)proptop_cb,
			   proptop);
	gtk_widget_show(button);

	button = gtk_button_new_with_label("Cancel");
	gtk_box_pack_start_defaults(GTK_BOX(GTK_DIALOG(proptop)->action_area),
				    button);
	gtk_object_set_user_data(GTK_OBJECT(button), (gpointer)CANCEL);
	gtk_signal_connect(GTK_OBJECT(button), "clicked", (GtkSignalFunc)proptop_cb,
			   proptop);
	gtk_widget_show(button);
    }

    gtk_widget_show(proptop);
} /* show_props */

static void
proptop_cb(GtkWidget *w, gpointer data)
{
    GtkWidget	*window = (GtkWidget *)data, *tgl;
    GList	*hdrlist;
    int		numrows, i, row, col;
    char	*newname, *alias_name, *buf, buf2[BUFSIZ], *temp;
    FILE	*nprivrc, *privrc;
    MAILRC	*m;
    DialogReason reason;

    reason = (DialogReason)gtk_object_get_user_data(GTK_OBJECT(w));
    if(reason != CANCEL){
	newname = g_strconcat(globRCfile, ".new", NULL);

	if((nprivrc = fopen(newname, "w")) == NULL) {
	    alert(NULL, "Unable to open temporary file.", ALERT_ERROR,
		  1, "Cancel");
	    return;
	}
	if((privrc = fopen(globRCfile, "r")) == NULL) {
	    alert(NULL, "Unable to read .privrc file.", ALERT_ERROR,
		  1, "Cancel");
	    return;
	}

	/* Write all our aliases first and update in-memory list. */
	fprintf(nprivrc, "#\n# Aliases\n#\n");
	clear_aliases();
	numrows = GTK_SHEET(propw.aliases)->maxrow + 1;
	/* Be wary of the active cell, set it to
	   make sure the contents have updated. */
	gtk_sheet_get_active_cell(GTK_SHEET(propw.aliases), &row, &col);
	gtk_sheet_set_active_cell(GTK_SHEET(propw.aliases), row, col);
	for(i = 0; i < numrows; i++) {
	    alias_name = gtk_sheet_cell_get_text(GTK_SHEET(propw.aliases),
						 i, 0);
	    if((alias_name == NULL) || (*alias_name == '\0'))
		continue;		/* End of valid aliases */
	    fprintf(nprivrc, "alias %s %s\n", alias_name,
		    gtk_sheet_cell_get_text(GTK_SHEET(propw.aliases), i, 1));
	    m = new_mailrc();
	    m->name = strdup(alias_name);
	    m->value = strdup(gtk_sheet_cell_get_text(GTK_SHEET(propw.aliases),
						      i, 1));
	    add_to_list(&alias, m);
	}

	/* Followed by the retain line. */
	fprintf(nprivrc, "#\n# Retained headers\n#\n");
	fprintf(nprivrc, "retain");
	hdrlist = gtk_container_children(GTK_CONTAINER(propw.hdr_list));
	for(; hdrlist != NULL; hdrlist = g_list_next(hdrlist)) {
	    tgl = (GtkWidget *)hdrlist->data;
	    buf = (char *)gtk_widget_get_name(tgl);
	    if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(tgl))) {
		/* Print the name of the widget. */
		fprintf(nprivrc, " %s", buf);
		add_entry(&retain, buf );
	    }
	    else {
		remove_retain(buf);
	    }
	}
	fprintf(nprivrc, "\n");

	/* Followed by set values. */
	fprintf(nprivrc, "#\n# Set values\n#\n");
	buf = gtk_entry_get_text(GTK_ENTRY(propw.mail_dir));
	if(*buf != '\0') {
	    fprintf(nprivrc, "set folder='%s'\n", buf);
	    replace_mailrc("folder", buf);
	}
	else {
	    remove_mailrc("folder");
	}

	buf = gtk_entry_get_text(GTK_ENTRY(propw.mail_spool));
	if(*buf != '\0') {
	    fprintf(nprivrc, "set mailspoolfile=%s\n", buf);
	}

	temp = find_mailrc("retrieveinterval");
	buf = gtk_entry_get_text(GTK_ENTRY(propw.check_interval));
	i = temp? strcmp(temp, buf): 1;
	if(*buf != '\0') {
	    fprintf(nprivrc, "set retrieveinterval=%s\n", buf);
	    replace_mailrc("retrieveinterval", buf);
	}
	if(i) {
	    mail_check_cb(0L);
	}

	buf = gtk_entry_get_text(GTK_ENTRY(propw.print_cmd));
	if(*buf != '\0') {
	    fprintf(nprivrc, "set printmail='%s'\n", buf);
	    replace_mailrc("printmail", buf);
	}
	else {
	    remove_mailrc("printmail");
	}

	buf = gtk_entry_get_text(GTK_ENTRY(propw.replyto));
	if(*buf != '\0') {
	    fprintf(nprivrc, "set replyto='%s'\n", buf);
	    replace_mailrc("replyto", buf);
	}
	else {
	    remove_mailrc("replyto");
	}

	buf = gtk_entry_get_text(GTK_ENTRY(propw.domain));
	if(*buf != '\0') {
	    fprintf(nprivrc, "set domain='%s'\n", buf);
	    replace_mailrc("domain", buf);
	}
	else {
	    remove_mailrc("domain");
	}

	buf = gtk_entry_get_text(GTK_ENTRY(propw.indent));
	if(*buf != '\0') {
	    fprintf(nprivrc, "set indentprefix='%s'\n", buf);
	    replace_mailrc("indentprefix", buf);
	}
	else {
	    remove_mailrc("indentprefix");
	}

	buf = gtk_entry_get_text(GTK_ENTRY(propw.wrapcol));
	if(*buf != '\0') {
	    fprintf(nprivrc, "set wrapmargin=%s\n", buf);
	    replace_mailrc("wrapmargin", buf);
	}
	else {
	    remove_mailrc("wrapmargin");
	}

	buf = gtk_entry_get_text(GTK_ENTRY(propw.organizn));
	if(*buf != '\0') {
	    fprintf(nprivrc, "set organization='%s'\n", buf);
	    replace_mailrc("organization", buf);
	}
	else {
	    remove_mailrc("organization");
	}

	buf = gtk_entry_get_text(GTK_ENTRY(propw.record));
	if(*buf != '\0') {
	    fprintf(nprivrc, "set record='%s'\n", buf);
	    replace_mailrc("record", buf);
	}
	else {
	    remove_mailrc("record");
	}

	buf = gtk_entry_get_text(GTK_ENTRY(propw.sigfile));
	if(*buf != '\0') {
	    fprintf(nprivrc, "set sigfile='%s'\n", buf);
	    replace_mailrc("sigfile", buf);
	}
	else {
	    remove_mailrc("sigfile");
	}

	fprintf(nprivrc, "set filemenu2='");
	numrows = GTK_SHEET(propw.mail_menu)->maxrow + 1;
	/* Be wary of the active cell, set it to something else to
	   make sure the contents have updated. */
	gtk_sheet_get_active_cell(GTK_SHEET(propw.mail_menu), &row, &col);
	gtk_sheet_set_active_cell(GTK_SHEET(propw.mail_menu), row, col);
	for(i = 0; i < numrows; i++) {
	    buf = gtk_sheet_cell_get_text(GTK_SHEET(propw.mail_menu), i, 0);
	    if((buf == NULL) || (*buf == '\0'))
		continue;		/* End of valid items */
	    if(i > 0) fprintf(nprivrc, " ");
	    fprintf(nprivrc, "%s", buf);
	}
	fprintf(nprivrc, "'\n");
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(propw.comp_sign))) {
	    fprintf(nprivrc, "set nodefaultsign\n");
	    replace_mailrc("nodefaultsign", "");
	}
	else {
	    remove_mailrc("nodefaultsign");
	}
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(propw.comp_encrypt))) {
	    fprintf(nprivrc, "set nodefaultencrypt\n");
	    replace_mailrc("nodefaultencrypt", "");
	}
	else {
	    remove_mailrc("nodefaultencrypt");
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(propw.comp_log))) {
	    fprintf(nprivrc, "set nodontlogmessages\n");
	    replace_mailrc("nodontlogmessages", "");
	}
	else {
	    remove_mailrc("nodontlogmessages");
	}
	if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(propw.comp_lograw))) {
	    fprintf(nprivrc, "set log-raw\n");
	    replace_mailrc("log-raw", "");
	}
	else {
	    remove_mailrc("log-raw");
	}
#ifdef HAVE_MIXMASTER
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(propw.comp_remail))) {
	    fprintf(nprivrc, "set nodefaultremail\n");
	    replace_mailrc("nodefaultremail", "");
	}
	else {
	    remove_mailrc("nodefaultremail");
	}
#endif
	if(!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(propw.badbeep))) {
	    fprintf(nprivrc, "set nobeepbadsig\n");
	    replace_mailrc("nobeepbadsig", "");
	}
	else {
	    remove_mailrc("nobeepbadsig");
	}

	/* Get the reply style from the option menu. */
	switch ((ReplyStyle)gtk_object_get_user_data(
				GTK_OBJECT(propw.replystyle))) {
	case REPLY_TO:
	    fprintf(nprivrc, "set defaultusereplyto\n");
	    break;
	case FROM:
	    fprintf(nprivrc, "set defaultusefrom\n");
	    break;
	case ASK:
	    break;		/* Do nothing */
	}

	/* Now copy over any set lines from the old file that we don't
	   set here. */
	while(fgets(buf2, BUFSIZ, privrc) != NULL) {
	    if( strncmp(buf2, "set ", 4) != 0)
		continue;

	    if( (strncmp(buf2, "set folder=", 11) != 0) &&
		(strncmp(buf2, "set mailspoolfile=", 18) != 0) &&
		(strncmp(buf2, "set retrieveinterval=", 21) != 0) &&
		(strncmp(buf2, "set printmail=", 14) != 0) &&
		(strncmp(buf2, "set domain=", 11) != 0) &&
		(strncmp(buf2, "set indentprefix=", 17) != 0) &&
		(strncmp(buf2, "set organization=", 17) != 0) &&
		(strncmp(buf2, "set record=", 11) != 0) &&
		(strncmp(buf2, "set sigfile=", 12) != 0) &&
		(strncmp(buf2, "set filemenu2=", 14) != 0) &&
		(strncmp(buf2, "set wrapmargin=", 15) != 0) &&
		(strncmp(buf2, "set defaultusereplyto", 17) != 0) &&
		(strncmp(buf2, "set defaultusefrom", 17) != 0) &&
		(strncmp(buf2, "set nodefaultsign", 17) != 0) &&
		(strncmp(buf2, "set nodefaultencrypt", 20) != 0) &&
		(strncmp(buf2, "set nodontlogmessages", 21) != 0) &&
		(strncmp(buf2, "set log-raw", 11) != 0) &&
		(strncmp(buf2, "set nobeepbadsig", 16) != 0) &&
		(strncmp(buf2, "set replyto", 11) != 0) &&
		(strncmp(buf2, "set nodefaultremail", 19) != 0)
		) {
		fputs(buf2, nprivrc);
	    }
	}

	fclose(privrc);

	/* Now pgp keys. */
	fprintf(nprivrc, "#\n# PGP Settings\n#\n");
	numrows = GTK_SHEET(propw.pgpkeys)->maxrow;
	/* Be wary of the active cell, set it to something else to
	   make sure the contents have updated. */
	gtk_sheet_get_active_cell(GTK_SHEET(propw.pgpkeys), &row, &col);
	gtk_sheet_set_active_cell(GTK_SHEET(propw.pgpkeys), row, col);
	for(i = 0; i < numrows; i++) {
	    buf = gtk_sheet_cell_get_text(GTK_SHEET(propw.pgpkeys), i, 0);
	    if((buf == NULL) || (*buf == '\0'))
		continue;		/* End of valid keys */
	    fprintf(nprivrc, "#@pgpkey %s=%s\n", buf,
		    gtk_sheet_cell_get_text(GTK_SHEET(propw.pgpkeys), i, 1));
	}

	/* Default pseudonym. */
	buf = gtk_entry_get_text(GTK_ENTRY(propw.default_nym));
	if((buf != NULL) && (*buf != '\0'))
	    fprintf(nprivrc, "#@defnym %s\n", buf);

	/* Security level. */
	i = gtk_spin_button_get_value_as_int(
				GTK_SPIN_BUTTON(propw.security_level));
	fprintf(nprivrc, "#@security %d\n", i);

	/* Test interval. */
	buf = gtk_entry_get_text(GTK_ENTRY(propw.test_interval));
	if((buf != NULL) && (*buf != '\0'))
	    fprintf(nprivrc, "testinterval='%s'\n", buf);

	/* Mail kill items. */
	numrows = GTK_SHEET(propw.kill_list)->maxrow;
	/* Be wary of the active cell, set it to something else to
	   make sure the contents have updated. */
	gtk_sheet_get_active_cell(GTK_SHEET(propw.kill_list), &row, &col);
	gtk_sheet_set_active_cell(GTK_SHEET(propw.kill_list), row, col);
	for(i = 0; i < numrows; i++) {
	    buf = gtk_sheet_cell_get_text(GTK_SHEET(propw.kill_list), i, 0);
	    if((buf == NULL) || (*buf == '\0'))
		continue;
	    fprintf(nprivrc, "#@kill%s %s\n",
		    (*buf == 'F')? "u": "s",
		    gtk_sheet_cell_get_text(GTK_SHEET(propw.kill_list), i, 1));
	}

	/* Pseudonyms. */
	numrows = GTK_SHEET(propw.pseudonyms)->maxrow;
	/* Be wary of the active cell, set it to something else to
	   make sure the contents have updated. */
	gtk_sheet_get_active_cell(GTK_SHEET(propw.pseudonyms), &row, &col);
	gtk_sheet_set_active_cell(GTK_SHEET(propw.pseudonyms), row, col);
	for(i = 0; i < numrows; i++) {
	    buf = gtk_sheet_cell_get_text(GTK_SHEET(propw.pseudonyms), i, 0);
	    if((buf == NULL) || (*buf == '\0'))
		continue;
	    fprintf(nprivrc, "#@pseudonym %s\n", buf);
	}

	/* Close file. */
	fclose(nprivrc);

	/* Do some renaming of files. */
	strcpy(buf2, globRCfile);
	strcat(buf2, ".bak");
	rename(globRCfile, buf2);
	rename(newname, globRCfile);
	g_free(newname);
    }

    if((reason == OK) || (reason == CANCEL))
	gtk_widget_hide(window);

} /* proptop_cb */

static gint
propdelete_cb(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    GtkWidget	*window = (GtkWidget *)data;

    gtk_widget_hide(window);
    return(TRUE);
} /* propdelete_cb */

static GtkWidget *
create_mailer_page()
{
    GtkWidget	*vbox, *table, *align, *w, *hbox, *swin;

    vbox = gtk_vbox_new(FALSE, 0);
    table = gtk_table_new(5, 2, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 4);

    align = gtk_alignment_new(1, 0.5, 0, 0);
    w = gtk_label_new("Mail Spool File");
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_table_attach(GTK_TABLE(table), align, 0, 1, 0, 1,
		     GTK_FILL, 0, 4, 4);
    gtk_widget_show(w);
    gtk_widget_show(align);
    propw.mail_spool = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), propw.mail_spool, 1, 2, 0, 1,
		     GTK_FILL|GTK_EXPAND, 0, 4, 4);
    gtk_widget_show(propw.mail_spool);

    align = gtk_alignment_new(1, 0.5, 0, 0);
    w = gtk_label_new("Check Interval (s)");
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_table_attach(GTK_TABLE(table), align, 0, 1, 1, 2,
		     GTK_FILL, 0, 4, 4);
    gtk_widget_show(w);
    gtk_widget_show(align);
    propw.check_interval = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), propw.check_interval, 1, 2, 1, 2,
		     GTK_FILL|GTK_EXPAND, 0, 4, 4);
    gtk_widget_show(propw.check_interval);

    align = gtk_alignment_new(1, 0.5, 0, 0);
    w = gtk_label_new("Mail File Directory");
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_table_attach(GTK_TABLE(table), align, 0, 1, 2, 3,
		     GTK_FILL, 0, 4, 4);
    gtk_widget_show(w);
    gtk_widget_show(align);
    propw.mail_dir = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), propw.mail_dir, 1, 2, 2, 3,
		     GTK_FILL|GTK_EXPAND, 0, 4, 4);
    gtk_widget_show(propw.mail_dir);

    align = gtk_alignment_new(1, 0.5, 0, 0);
    w = gtk_label_new("Print Command");
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_table_attach(GTK_TABLE(table), align, 0, 1, 3, 4,
		     GTK_FILL, 0, 4, 4);
    gtk_widget_show(w);
    propw.print_cmd = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), propw.print_cmd, 1, 2, 3, 4,
		     GTK_FILL|GTK_EXPAND, 0, 4, 4);
    gtk_widget_show(align);
    gtk_widget_show(propw.print_cmd);

    align = gtk_alignment_new(1, 0.5, 0, 0);
    w = gtk_label_new("Organization");
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_table_attach(GTK_TABLE(table), align, 0, 1, 4, 5,
		     GTK_FILL, 0, 4, 4);
    gtk_widget_show(align);
    gtk_widget_show(w);
    propw.organizn = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), propw.organizn, 1, 2, 4, 5,
		     GTK_FILL|GTK_EXPAND, 0, 4, 4);
    gtk_widget_show(propw.organizn);

    hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start_defaults(GTK_BOX(vbox), hbox);

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
				   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start(GTK_BOX(hbox), swin, TRUE, TRUE, 4);
    propw.mail_menu = gtk_sheet_new(9, 1, "folders");
    gtk_container_add(GTK_CONTAINER(swin), propw.mail_menu);
    gtk_sheet_column_button_add_label(GTK_SHEET(propw.mail_menu), 0,
				      "Folder Combo Items");
    gtk_sheet_hide_row_titles(GTK_SHEET(propw.mail_menu));
    gtk_sheet_set_column_width(GTK_SHEET(propw.mail_menu), 0, 200);
    gtk_signal_connect(GTK_OBJECT(propw.mail_menu), "changed", (GtkSignalFunc)cell_change_cb,
		       NULL);
    gtk_widget_show(propw.mail_menu);
    gtk_widget_show(swin);
    gtk_widget_show(hbox);

    gtk_widget_show(table);
    gtk_widget_show(vbox);

    load_mailer_page();

    return vbox;
} /* create_mailer_page */

static GtkWidget *
create_compose_page()
{
    GtkWidget	*vbox, *table, *tbl2, *align, *w, *frame, *hbox;
    GtkItemFactory	*ifactory;

    vbox = gtk_vbox_new(FALSE, 0);
    table = gtk_table_new(5, 4, FALSE);
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, TRUE, 4);

    align = gtk_alignment_new(1, 0.5, 0, 0);
    w = gtk_label_new("Reply-To Address");
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_table_attach(GTK_TABLE(table), align, 0, 1, 0, 1,
		     GTK_FILL, 0, 4, 4);
    gtk_widget_show(w);
    gtk_widget_show(align);
    propw.replyto = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), propw.replyto, 1, 4, 0, 1,
		     GTK_FILL|GTK_EXPAND, 0, 4, 4);
    gtk_widget_show(propw.replyto);

    align = gtk_alignment_new(1, 0.5, 0, 0);
    w = gtk_label_new("Domain");
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_table_attach(GTK_TABLE(table), align, 0, 1, 1, 2,
		     GTK_FILL, 0, 4, 4);
    gtk_widget_show(w);
    propw.domain = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), propw.domain, 1, 4, 1, 2,
		     GTK_FILL|GTK_EXPAND, 0, 4, 4);
    gtk_widget_show(align);
    gtk_widget_show(propw.domain);

    align = gtk_alignment_new(1, 0.5, 0, 0);
    w = gtk_label_new("Indent Prefix");
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_table_attach(GTK_TABLE(table), align, 0, 1, 2, 3,
		     GTK_FILL, 0, 4, 4);
    gtk_widget_show(align);
    gtk_widget_show(w);
    propw.indent = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), propw.indent, 1, 2, 2, 3,
		     GTK_FILL|GTK_EXPAND, 0, 4, 4);
    gtk_widget_show(propw.indent);

    align = gtk_alignment_new(1, 0.5, 0, 0);
    w = gtk_label_new("Wrap Column");
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_table_attach(GTK_TABLE(table), align, 2, 3, 2, 3,
		     GTK_FILL, 0, 4, 4);
    gtk_widget_show(align);
    gtk_widget_show(w);
    propw.wrapcol = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), propw.wrapcol, 3, 4, 2, 3,
		     GTK_FILL|GTK_EXPAND, 0, 4, 4);
    gtk_widget_show(propw.wrapcol);

    align = gtk_alignment_new(1, 0.5, 0, 0);
    w = gtk_label_new("Log Folder Name");
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_table_attach(GTK_TABLE(table), align, 0, 1, 3, 4,
		     GTK_FILL, 0, 4, 4);
    gtk_widget_show(align);
    gtk_widget_show(w);
    propw.record = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), propw.record, 1, 4, 3, 4,
		     GTK_FILL|GTK_EXPAND, 0, 4, 4);
    gtk_widget_show(propw.record);

    align = gtk_alignment_new(1, 0.5, 0, 0);
    w = gtk_label_new("Signature File");
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_table_attach(GTK_TABLE(table), align, 0, 1, 4, 5,
		     GTK_FILL, 0, 4, 4);
    gtk_widget_show(align);
    gtk_widget_show(w);
    propw.sigfile = gtk_entry_new();
    gtk_table_attach(GTK_TABLE(table), propw.sigfile, 1, 4, 4, 5,
		     GTK_FILL|GTK_EXPAND, 0, 4, 4);
    gtk_widget_show(propw.sigfile);

    frame = gtk_frame_new("Options");
    gtk_box_pack_start(GTK_BOX(vbox), frame, FALSE, FALSE, 4);

    tbl2 = gtk_table_new(5, 2, FALSE);
    gtk_container_set_border_width(GTK_CONTAINER(frame), 4);
    gtk_container_add(GTK_CONTAINER(frame), tbl2);

    align = gtk_alignment_new(0, 0.5, 0, 0);
    propw.comp_sign = gtk_check_button_new_with_label("Sign Outgoing Mail");
    gtk_container_add(GTK_CONTAINER(align), propw.comp_sign);
    gtk_table_attach(GTK_TABLE(tbl2), align, 0, 1, 0, 1,
		     GTK_FILL, 0, 4, 4);
    gtk_widget_show(propw.comp_sign);
    gtk_widget_show(align);

    align = gtk_alignment_new(0, 0.5, 0, 0);
    propw.comp_encrypt = gtk_check_button_new_with_label("Encrypt Outgoing Mail");
    gtk_container_add(GTK_CONTAINER(align), propw.comp_encrypt);
    gtk_table_attach(GTK_TABLE(tbl2), align, 0, 1, 1, 2,
		     GTK_FILL, 0, 4, 4);
    gtk_widget_show(propw.comp_encrypt);
    gtk_widget_show(align);

    align = gtk_alignment_new(0, 0.5, 0, 0);
    propw.comp_log = gtk_check_button_new_with_label("Log Outgoing Mail");
    gtk_container_add(GTK_CONTAINER(align), propw.comp_log);
    gtk_table_attach(GTK_TABLE(tbl2), align, 0, 1, 2, 3,
		     GTK_FILL, 0, 4, 4);
    gtk_widget_show(propw.comp_log);
    gtk_widget_show(align);

    align = gtk_alignment_new(0, 0.5, 0, 0);
    propw.comp_lograw = gtk_check_button_new_with_label("Log As Ascii");
    gtk_container_add(GTK_CONTAINER(align), propw.comp_lograw);
    gtk_table_attach(GTK_TABLE(tbl2), align, 0, 1, 3, 4,
		     GTK_FILL, 0, 4, 4);
    gtk_widget_show(propw.comp_lograw);
    gtk_widget_show(align);

    align = gtk_alignment_new(0, 0.5, 0, 0);
    propw.badbeep = gtk_check_button_new_with_label("Beep on Bad Signature");
    gtk_container_add(GTK_CONTAINER(align), propw.badbeep);
    gtk_table_attach(GTK_TABLE(tbl2), align, 0, 1, 4, 5,
		     GTK_FILL, 0, 4, 4);
    gtk_widget_show(propw.badbeep);
    gtk_widget_show(align);

    hbox = gtk_hbox_new(FALSE, 4);
    w = gtk_label_new("Reply address method");
    gtk_box_pack_start_defaults(GTK_BOX(hbox), w);
    gtk_widget_show(w);
    propw.replystyle = gtk_option_menu_new();
    ifactory = gtk_item_factory_new(GTK_TYPE_MENU, "<Reply>",
				    gtk_accel_group_get_default());
    gtk_item_factory_create_items(ifactory,
				  sizeof(reply_ife) / sizeof(GtkItemFactoryEntry),
				  reply_ife, NULL);
    gtk_option_menu_set_menu(GTK_OPTION_MENU(propw.replystyle),
			     ifactory->widget);
    gtk_widget_show(ifactory->widget);
    gtk_box_pack_start_defaults(GTK_BOX(hbox), propw.replystyle);
    gtk_table_attach(GTK_TABLE(tbl2), hbox, 1, 2, 0, 1,
		     0, 0, 4, 4);
    gtk_widget_show(propw.replystyle);
    gtk_widget_show(hbox);

    gtk_widget_show(tbl2);
    gtk_widget_show(frame);

    gtk_widget_show(table);
    gtk_widget_show(vbox);

    load_compose_page();

    return vbox;
} /* create_compose_page */

static GtkWidget *
create_alias_page()
{
    GtkWidget	*swin;

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
				   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

    propw.aliases = gtk_sheet_new(14, 2, "aliases");
    gtk_container_add(GTK_CONTAINER(swin), propw.aliases);
    gtk_sheet_hide_row_titles(GTK_SHEET(propw.aliases));
    gtk_sheet_column_button_add_label(GTK_SHEET(propw.aliases), 0, "Name");
    gtk_sheet_column_button_add_label(GTK_SHEET(propw.aliases), 1, "Address");
    gtk_sheet_set_column_width(GTK_SHEET(propw.aliases), 0, 150);
    gtk_sheet_set_column_width(GTK_SHEET(propw.aliases), 1, 300);
    gtk_signal_connect(GTK_OBJECT(propw.aliases), "changed", (GtkSignalFunc)cell_change_cb,
		       NULL);

    gtk_widget_show(propw.aliases);
    gtk_widget_show(swin);

    load_aliases_page();

    return swin;
} /* create_alias_page */

static GtkWidget *
create_headers_page()
{
    GtkWidget	*frame, *swin, *w;
    MAIL_HEADER_FIELD	*mhf_ptr;

    frame = gtk_frame_new("Displayed Mail Headers");
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
				   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(frame), swin);
    propw.hdr_list = gtk_vbox_new(FALSE, 0);
    gtk_scrolled_window_add_with_viewport(GTK_SCROLLED_WINDOW(swin),
					  propw.hdr_list);

    for(mhf_ptr = standard_headers; mhf_ptr->name != NULL; mhf_ptr++) {
	w = gtk_check_button_new_with_label(mhf_ptr->name);
	gtk_widget_set_name(w, mhf_ptr->name);
	gtk_box_pack_start_defaults(GTK_BOX(propw.hdr_list), w);
	if(retain_line(mhf_ptr->name)) {
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
	}
	gtk_widget_show(w);
    }

    gtk_widget_show(propw.hdr_list);
    gtk_widget_show(swin);
    gtk_widget_show(frame);

    return frame;
} /* create_headers_page */

static GtkWidget *
create_pgp_page()
{
    GtkWidget	*vbox, *hbox, *swin, *frame, *w;
    GtkObject	*adj;

    vbox = gtk_vbox_new(FALSE, 4);
    frame = gtk_frame_new("PGP Key Translations");
    gtk_box_pack_start(GTK_BOX(vbox), frame, 1, 1, 0);
    gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_NONE);
    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
				   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(frame), swin);
    propw.pgpkeys = gtk_sheet_new(4, 2, "pgpkeytrans");
    gtk_container_add(GTK_CONTAINER(swin), propw.pgpkeys);
    gtk_sheet_hide_row_titles(GTK_SHEET(propw.pgpkeys));
    gtk_sheet_column_button_add_label(GTK_SHEET(propw.pgpkeys), 0, "Name");
    gtk_sheet_column_button_add_label(GTK_SHEET(propw.pgpkeys), 1, "Keyname");
    gtk_sheet_set_column_width(GTK_SHEET(propw.pgpkeys), 0, 100);
    gtk_sheet_set_column_width(GTK_SHEET(propw.pgpkeys), 1, 300);
    gtk_signal_connect(GTK_OBJECT(propw.pgpkeys), "changed", (GtkSignalFunc)cell_change_cb,
		       NULL);
    gtk_widget_show(propw.pgpkeys);
    gtk_widget_show(swin);
    gtk_widget_show(frame);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, 0, 1, 0);

    w = gtk_label_new("Security Level");
    gtk_box_pack_start(GTK_BOX(hbox), w, 0, 0, 0);
    gtk_widget_show(w);
    adj = gtk_adjustment_new(1.0, 1.0, 4.0, 1.0, 1.0, 1.0);
    propw.security_level = gtk_spin_button_new(GTK_ADJUSTMENT(adj), 1.0, 0);
    gtk_box_pack_start(GTK_BOX(hbox), propw.security_level, 0, 0, 0);
    gtk_widget_show(propw.security_level);
    w = gtk_label_new("Test Interval (sec)");
    gtk_box_pack_start(GTK_BOX(hbox), w, 0, 0, 8);
    gtk_widget_show(w);
    propw.test_interval = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), propw.test_interval, 1, 1, 0);
    gtk_widget_show(propw.test_interval);
    gtk_widget_show(hbox);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, 1, 1, 0);

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
				   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start_defaults(GTK_BOX(hbox), swin);
    propw.pseudonyms = gtk_sheet_new(5, 1, "pseudonyms");
    gtk_container_add(GTK_CONTAINER(swin), propw.pseudonyms);
    gtk_sheet_hide_row_titles(GTK_SHEET(propw.pseudonyms));
    gtk_sheet_column_button_add_label(GTK_SHEET(propw.pseudonyms), 0,
				      "PGP Pseudonyms");
    gtk_sheet_set_column_width(GTK_SHEET(propw.pseudonyms), 0, 180);
    gtk_signal_connect(GTK_OBJECT(propw.pseudonyms), "changed", (GtkSignalFunc)cell_change_cb,
		       NULL);
    gtk_widget_show(propw.pseudonyms);
    gtk_widget_show(swin);

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin),
				   GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_box_pack_start_defaults(GTK_BOX(hbox), swin);
    propw.kill_list = gtk_sheet_new(5, 2, "killmail");
    gtk_container_add(GTK_CONTAINER(swin), propw.kill_list);
    gtk_sheet_hide_row_titles(GTK_SHEET(propw.kill_list));
    gtk_sheet_column_button_add_label(GTK_SHEET(propw.kill_list), 0, "");
    gtk_sheet_column_button_add_label(GTK_SHEET(propw.kill_list), 1,
				      "Kill Mail Matching");
    gtk_sheet_set_column_width(GTK_SHEET(propw.kill_list), 0, 20);
    gtk_sheet_set_column_width(GTK_SHEET(propw.kill_list), 1, 180);
    gtk_signal_connect(GTK_OBJECT(propw.kill_list), "changed", (GtkSignalFunc)cell_change_cb,
		       NULL);
    gtk_widget_show(propw.kill_list);
    gtk_widget_show(swin);
    gtk_widget_show(hbox);

    hbox = gtk_hbox_new(FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, 0, 1, 0);
    w = gtk_label_new("Default Pseudonym");
    gtk_box_pack_start(GTK_BOX(hbox), w, 0, 0, 0);
    gtk_widget_show(w);
    propw.default_nym = gtk_entry_new();
    gtk_box_pack_start(GTK_BOX(hbox), propw.default_nym, 1, 1, 0);
    gtk_widget_show(propw.default_nym);
    gtk_widget_show(hbox);

    gtk_widget_show(vbox);

    load_pgp_page();

    return vbox;
} /* create_pgp_page */

static void
load_mailer_page()
{
    int		rownum = 0, numrows;
    char	*rcval, *temp, *ent;

    if( (rcval = find_mailrc("mailspoolfile")) )
	gtk_entry_set_text(GTK_ENTRY(propw.mail_spool), rcval);

    if( (rcval = find_mailrc("retrieveinterval")) )
	gtk_entry_set_text(GTK_ENTRY(propw.check_interval), rcval);
    else {
	temp = g_strdup_printf("%d", DEFAULT_CHECK_TIME);
	gtk_entry_set_text(GTK_ENTRY(propw.check_interval), temp);
	g_free(temp);
    }

    if( (rcval = find_mailrc("folder")) )
	gtk_entry_set_text(GTK_ENTRY(propw.mail_dir), rcval);

    if( (rcval = find_mailrc("filemenu2")) && (*rcval != '\0')) {
	temp = strdup(rcval);
	ent = strtok(temp, " \n\r\t");
	numrows = GTK_SHEET(propw.mail_menu)->maxrow;
	do{
	    if(rownum > numrows) {
		gtk_sheet_add_row(GTK_SHEET(propw.mail_menu),
				  rownum - numrows);
		numrows = GTK_SHEET(propw.mail_menu)->maxrow;
	    }
	    gtk_sheet_set_cell_text(GTK_SHEET(propw.mail_menu), rownum, 0, ent);
	    rownum++;
	}while((ent = strtok(NULL, " \n\r\t")));
	free(temp);
    }

    if( (rcval = find_mailrc("printmail")) ) {
	gtk_entry_set_text(GTK_ENTRY(propw.print_cmd), rcval);
    }
    if( (rcval = find_mailrc("organization")) ) {
	gtk_entry_set_text(GTK_ENTRY(propw.organizn), rcval);
    }
} /* load_mailer_page */

static void
load_compose_page()
{
    char	*rcval;

    if( (rcval = find_mailrc("replyto")) )
	gtk_entry_set_text(GTK_ENTRY(propw.replyto), rcval);

    if( (rcval = find_mailrc("record")) )
	gtk_entry_set_text(GTK_ENTRY(propw.record), rcval);

    if( (rcval = find_mailrc("sigfile")) )
	gtk_entry_set_text(GTK_ENTRY(propw.sigfile ), rcval);

    if( (rcval = find_mailrc("indentprefix")) ) {
	gtk_entry_set_text(GTK_ENTRY(propw.indent), rcval);
    }
    else {
	gtk_entry_set_text(GTK_ENTRY(propw.indent), "> ");
    }

    if( (rcval = find_mailrc("domain")) )
	gtk_entry_set_text(GTK_ENTRY(propw.domain), rcval);

    if( (rcval = find_mailrc("wrapmargin")) )
	gtk_entry_set_text(GTK_ENTRY(propw.wrapcol), rcval);

    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(propw.comp_sign),
			   !(int)find_mailrc("nodefaultsign"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(propw.comp_encrypt),
			   !(int)find_mailrc("nodefaultencrypt"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(propw.comp_log),
			   (int)find_mailrc("nodontlogmessages"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(propw.comp_lograw),
			   (int)find_mailrc("log-raw"));
#ifdef HAVE_MIXMASTER
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(propw.comp_remail),
			   !(int)find_mailrc("nodefaultremail"));
#endif
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(propw.badbeep),
			   !(int)find_mailrc("nobeepbadsig"));

    if (find_mailrc("defaultusereplyto")) {
	gtk_option_menu_set_history(GTK_OPTION_MENU(propw.replystyle), 0);
    }
    else if (find_mailrc("defaultusefrom")) {
	gtk_option_menu_set_history(GTK_OPTION_MENU(propw.replystyle), 1);
    }
    else {
	gtk_option_menu_set_history(GTK_OPTION_MENU(propw.replystyle), 2);
    }

} /* load_compose_page */

static void
load_aliases_page()
{
    MAILRC		*m;
    int			rownum = 0, numrows;

    numrows = GTK_SHEET(propw.aliases)->maxrow + 1;
    if(numrows < alias.number) {
	gtk_sheet_add_row(GTK_SHEET(propw.aliases), alias.number - numrows);
    }

    m = alias.start;

    while (m) {
	gtk_sheet_set_cell_text(GTK_SHEET(propw.aliases), rownum, 0, m->name);
	gtk_sheet_set_cell_text(GTK_SHEET(propw.aliases), rownum, 1, m->value);
	rownum++;
	m = m->next;
    }
} /* load_aliases_page */

static void
load_pgp_page()
{
    int		row = 0, psrow = 0, killrow = 0, seclev;
    char	buf[BUFSIZ], *p_buf, *tok, *name, *key;
    FILE 	*privrc;

    if((privrc = fopen(globRCfile,"r")) != NULL){
	while((p_buf = fgets(buf, BUFSIZ, privrc)) != NULL){
	    tok = strtok(p_buf, " \t=");
	    if(!tok)
		continue;
	    if(! strcmp(tok, "#@pgpkey")){
		name = strtok(NULL, "=");
		if(!name)
		    continue;
		key = strtok(NULL, " \t\n\r");
		if(!key)
		    continue;
		gtk_sheet_set_cell_text(GTK_SHEET(propw.pgpkeys), row, 0, name);
		gtk_sheet_set_cell_text(GTK_SHEET(propw.pgpkeys), row, 1, key);
		row++;
	    }
	    else if(! strcmp(tok, "#@pseudonym")){
		name = strtok(NULL, " \t\n\r");
		if(!name)
		    continue;
		gtk_sheet_set_cell_text(GTK_SHEET(propw.pseudonyms),
					psrow, 0, name);
		psrow++;
	    }
	    else if(! strcmp(tok, "#@defnym")){
		name = strtok(NULL, " \t\n\r");
		if(!name)
		    continue;
		gtk_entry_set_text(GTK_ENTRY(propw.default_nym), name);
	    }
	    else if(! strcmp(tok, "#@security")){
		name = strtok(NULL, " \t\n\r");
		if(!name)
		    continue;
		seclev = atoi(name);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(propw.security_level),
					  (gfloat)seclev);
	    }
	    else if(! strcmp(tok, "#@killu")){
		name = strtok(NULL, " \t\n\r");
		if(!name)
		    continue;
		gtk_sheet_set_cell_text(GTK_SHEET(propw.kill_list),
					killrow, 0, "F");
		gtk_sheet_set_cell_text(GTK_SHEET(propw.kill_list),
					killrow, 1, name);
		killrow++;
	    }
	    else if(! strcmp(tok, "#@kills")){
		name = strtok(NULL, " \t\n\r");
		if(!name)
		    continue;
		gtk_sheet_set_cell_text(GTK_SHEET(propw.kill_list),
					killrow, 0, "S");
		gtk_sheet_set_cell_text(GTK_SHEET(propw.kill_list),
					killrow, 1, name);
		killrow++;
	    }
	    else if(! strcmp(tok, "testinterval")){
		name = strtok(NULL, " \t\n\r");
		if(!name)
		    continue;
		if(*name == '\'')
		    name++;
		if(name[strlen(name) - 1] == '\'')
		    name[strlen(name) - 1] = '\0';
		gtk_entry_set_text(GTK_ENTRY(propw.test_interval), name);
	    }
	}
	fclose(privrc);
    }
} /* load_pgp_page */

/*----------------------------------------------------------------------*/

/* This makes sure there is always a spare row at the end of the sheet
   to save having to manually add rows when the sheet fills up.
*/
static void
cell_change_cb(GtkWidget *widget, gint row, gint col, gpointer data)
{
    if(row == GTK_SHEET(widget)->maxrow) {
	gtk_sheet_add_row(GTK_SHEET(widget), 1);
    }
} /* cell_change_cb */

static void
reply_cb(GtkWidget *w, gpointer data)
{
    gtk_object_set_user_data(GTK_OBJECT(propw.replystyle), data);
} /* reply_cb */

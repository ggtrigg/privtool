/* $Id$ 
 *
 * Copyright   : (c) 1997 by Glenn Trigg.  All Rights Reserved
 * Project     : Privtool - Motif interface
 * File        : mprops
 *
 * Author      : Glenn Trigg
 * Created     : 27 May 1997 
 *
 * Description : Code for implementing the properties window.
 */

#ifdef HAVE_CONFIG_H
#include	"config.h"
#endif

#include	<stdio.h>
#include	<stdlib.h>
#include	<X11/X.h>
#include	<X11/Xlib.h>
#include	<X11/Intrinsic.h>
#include	<X11/Xmu/Editres.h>
#include	<X11/Xcms.h>
#include	<Xm/Xm.h>
#include	<Xm/DialogS.h>
#include	<Xm/Frame.h>
#include	<Xm/Label.h>
#include	<Xm/MessageB.h>
#include	<Xm/Notebook.h>
#include	<Xm/Protocols.h>
#include	<Xm/PushB.h>
#include	<Xm/RowColumn.h>
#include	<Xm/SeparatoG.h>
#include	<Xm/SpinB.h>
#include	<Xm/TextF.h>
#include	<Xm/ToggleB.h>
#include	<Xbae/Matrix.h>
#include	<Xbae/Caption.h>
#ifndef MAXPATHLEN
#include	<sys/param.h>
#endif /* MAXPATHLEN */
#include	"def.h"
#include	"buffers.h"
#include	"message.h"
#include	"mailrc.h"
#include	"windows.h"
#include	"motif_protos.h"

extern char		globRCfile[MAXPATHLEN];  /* from main.c */
extern LIST		alias, retain;
extern MAIL_HEADER_FIELD standard_headers[];

#define IGNORED		"Ignored"
#define SHOWN		"Retained"

typedef enum {
    KILL_SUBJECT,
    KILL_USER
} KillType;

typedef struct {
    Widget	mail_dir;		/* folder */
    Widget	mail_menu;		/* filemenu2 */
    Widget	detached;
    Widget	aliases;
    Widget	pgpkeys;
    Widget	security_level;
    Widget	test_interval;
    Widget	pseudonyms;
    Widget	kill_list;
    Widget	default_nym;
    Widget	hdr_list;
    Widget	replyto;		/* replyto */
    Widget	comp_sign;		/* nodefaultsign */
    Widget	comp_encrypt;		/* nodefaultencrypt */
    Widget	comp_remail;		/* nodefaultremail */
    Widget	comp_log;		/* nodontlogmessages */
    Widget	comp_lograw;		/* log-raw */
    Widget	print_cmd;		/* printmail */
    Widget	domain;			/* domain */
    Widget	indent;			/* indentprefix */
    Widget	organizn;		/* organization */
    Widget	record;			/* record */
    Widget	badbeep;		/* nobeepbadsig */
} PropWidgets;

static PropWidgets	propw;

static Widget		propswin = NULL, killuser, killsubj;
static KillType		killtype = KILL_SUBJECT;

static void		create_gui_page(Widget);
static void		create_alias_page(Widget);
static void		create_comp_page(Widget);
static void		create_pgp_page(Widget);
static void		create_hdr_page(Widget);
static void		load_aliases();
static void		load_pgpstuff();
static void		load_guistuff();
static void		load_compstuff();
static void		load_hdrstuff();
static void		savePropsCb(Widget, XtPointer, XtPointer);
static void		closeCb(Widget, XtPointer, XtPointer);
static void		addRowCb(Widget, XtPointer, XtPointer);
static void		deleteRowCb(Widget, XtPointer, XtPointer);
static void		killToggleCb(Widget, XtPointer, XtPointer);
static void		enterCellCb(Widget, XtPointer, XtPointer);
static void		toggleCellCb(Widget, XtPointer, XtPointer);

void
show_props(Widget parent)
{
    Widget	w, notebook;

    if(propswin == NULL){
	propswin = XmCreateMessageDialog(parent, "props", NULL, 0);
	XtUnmanageChild(XtNameToWidget(propswin, "*Help"));
	w = XmCreatePushButton(propswin, "apply", NULL, 0);
	XtManageChild(w);
	XtAddCallback(w, XmNactivateCallback, savePropsCb, propswin);
	XtAddCallback(propswin, XmNokCallback, savePropsCb, propswin);
	XtAddCallback(propswin, XmNcancelCallback, savePropsCb, propswin);

	/* Window manager quit support */
	XmAddWMProtocolCallback(XtParent(propswin),
				XmInternAtom(XtDisplay(propswin),
					     "WM_DELETE_WINDOW", False),
				closeCb, NULL);

	/* Add editres protocol support */
	XtAddEventHandler(XtParent(propswin), 0, True,
			  _XEditResCheckMessages, NULL);

	notebook = XmCreateNotebook(propswin, "notebook", NULL, 0);
	XtManageChild(notebook);
	/* Trick to stop the notebook creating a page scroller */
	XtVaCreateWidget("dummy", xmSeparatorGadgetClass,
			 notebook,
			 XmNnotebookChildType, XmPAGE_SCROLLER,
			 NULL);

	create_gui_page(notebook);
	create_comp_page(notebook);
	create_alias_page(notebook);
	create_hdr_page(notebook);
	create_pgp_page(notebook);
    }
    XtManageChild(propswin);
} /* show_mail_folders */

/*----------------------------------------------------------------------*/

static void
closeCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    if(propswin)
	XtUnmanageChild(propswin);
} /* closeCb */

/*----------------------------------------------------------------------*/

static void
create_gui_page(Widget parent)
{
    Widget	w, page, ctrl, btnctrl, tglctrl, cap, frame;

    page = XmCreateRowColumn(parent, "guipage", NULL, 0);
    XtManageChild(page);
    w = XmCreatePushButton(parent, "guibtn", NULL, 0);
    XtManageChild(w);

    cap = XtVaCreateManagedWidget("dircap", xbaeCaptionWidgetClass,
				  page, NULL);
    propw.mail_dir = XmCreateTextField(cap, "directory", NULL, 0);
    XtManageChild(propw.mail_dir);

    cap = XtVaCreateManagedWidget("prncap", xbaeCaptionWidgetClass,
				  page, NULL);
    propw.print_cmd = XmCreateTextField(cap, "print", NULL, 0);
    XtManageChild(propw.print_cmd);

    cap = XtVaCreateManagedWidget("orgcap", xbaeCaptionWidgetClass,
				  page, NULL);
    propw.organizn = XmCreateTextField(cap, "organization", NULL, 0);
    XtManageChild(propw.organizn);

    alignCaptions(page);

    ctrl = XmCreateRowColumn(page, "menuctrl", NULL, 0);
    XtManageChild(ctrl);
    cap = XtVaCreateManagedWidget("menucap", xbaeCaptionWidgetClass,
				  ctrl, NULL);
    propw.mail_menu = XtVaCreateManagedWidget("menugrid",
                                 xbaeMatrixWidgetClass, cap,
                                 NULL);
    btnctrl = XmCreateRowColumn(ctrl, "btnctrl", NULL, 0);
    XtManageChild(btnctrl);
    w = XmCreatePushButton(btnctrl, "add", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, addRowCb, propw.mail_menu);
    w = XmCreatePushButton(btnctrl, "delete", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, deleteRowCb, propw.mail_menu);

    /*w = XmCreateSeparatorGadget(ctrl, "guisep", NULL, 0);
      XtManageChild(w);*/

    frame = XmCreateFrame(ctrl, "frame", NULL, 0);
    XtManageChild(frame);
    tglctrl = XmCreateRowColumn(frame, "tglctrl", NULL, 0);
    XtManageChild(tglctrl);
    w = XmCreateLabel(frame, "flabel", NULL, 0);
    XtManageChild(w);
    propw.detached = XmCreateToggleButton(tglctrl, "detached", NULL, 0);
    XtManageChild(propw.detached);

    load_guistuff();
} /* create_gui_page */

/*----------------------------------------------------------------------*/

static void
create_alias_page(Widget parent)
{
    Widget	w, page, ctrl;

    page = XmCreateRowColumn(parent, "aliaspage", NULL, 0);
    XtManageChild(page);
    w = XmCreatePushButton(parent, "aliasbtn", NULL, 0);
    XtManageChild(w);
    propw.aliases = XtVaCreateManagedWidget("aliasgrid",
					    xbaeMatrixWidgetClass, page,
					    NULL);

    ctrl = XmCreateRowColumn(page, "btn_ctrl", NULL, 0);
    XtManageChild(ctrl);
    w = XmCreatePushButton(ctrl, "add", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, addRowCb, propw.aliases);
    w = XmCreatePushButton(ctrl, "delete", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, deleteRowCb, propw.aliases);

    load_aliases();
} /* create_alias_page */

/*----------------------------------------------------------------------*/

static void
create_comp_page(Widget parent)
{
    Widget	w, page, cap, frame, tglctrl;

    page = XmCreateRowColumn(parent, "comppage", NULL, 0);
    XtManageChild(page);
    w = XmCreatePushButton(parent, "compbtn", NULL, 0);
    XtManageChild(w);

    cap = XtVaCreateManagedWidget("rplcap", xbaeCaptionWidgetClass,
				  page, NULL);
    propw.replyto = XmCreateTextField(cap, "replyto", NULL, 0);
    XtManageChild(propw.replyto);

    cap = XtVaCreateManagedWidget("dmncap", xbaeCaptionWidgetClass,
				  page, NULL);
    propw.domain = XmCreateTextField(cap, "domain", NULL, 0);
    XtManageChild(propw.domain);

    cap = XtVaCreateManagedWidget("idntcap", xbaeCaptionWidgetClass,
				  page, NULL);
    propw.indent = XmCreateTextField(cap, "indent", NULL, 0);
    XtManageChild(propw.indent);

    cap = XtVaCreateManagedWidget("rcdcap", xbaeCaptionWidgetClass,
				  page, NULL);
    propw.record = XmCreateTextField(cap, "record", NULL, 0);
    XtManageChild(propw.record);

    alignCaptions(page);

    frame = XmCreateFrame(page, "frame", NULL, 0);
    XtManageChild(frame);
    tglctrl = XmCreateRowColumn(frame, "tglctrl", NULL, 0);
    XtManageChild(tglctrl);
    w = XmCreateLabel(frame, "flabel", NULL, 0);
    XtManageChild(w);

    propw.comp_sign = XmCreateToggleButton(tglctrl, "sign", NULL, 0);
    XtManageChild(propw.comp_sign);
    propw.comp_encrypt = XmCreateToggleButton(tglctrl, "encrypt", NULL, 0);
    XtManageChild(propw.comp_encrypt);
    propw.comp_log = XmCreateToggleButton(tglctrl, "log", NULL, 0);
    XtManageChild(propw.comp_log);
    propw.comp_lograw = XmCreateToggleButton(tglctrl, "lograw", NULL, 0);
    XtManageChild(propw.comp_lograw);
#ifdef HAVE_MIXMASTER
    propw.comp_remail = XmCreateToggleButton(tglctrl, "remail", NULL, 0);
    XtManageChild(propw.comp_remail);
#endif
    propw.badbeep = XmCreateToggleButton(tglctrl, "badbeep", NULL, 0);
    XtManageChild(propw.badbeep);

    load_compstuff();
} /* create_comp_page */

/*----------------------------------------------------------------------*/

static void
create_pgp_page(Widget parent)
{
    Widget	w, page, ctrl, btnctrl, cap, spin;

    page = XmCreateRowColumn(parent, "pgppage", NULL, 0);
    XtManageChild(page);
    w = XmCreatePushButton(parent, "pgpbtn", NULL, 0);
    XtManageChild(w);

    ctrl = XmCreateRowColumn(page, "keyctrl", NULL, 0);
    XtManageChild(ctrl);
    cap = XtVaCreateManagedWidget("pgpkeycap",
			    xbaeCaptionWidgetClass, ctrl,
			    NULL);
    propw.pgpkeys = XtVaCreateManagedWidget("pgpkeygrid",
					    xbaeMatrixWidgetClass, cap,
					    NULL);
    btnctrl = XmCreateRowColumn(ctrl, "btnctrl", NULL, 0);
    XtManageChild(btnctrl);
    w = XmCreatePushButton(btnctrl, "add", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, addRowCb, propw.pgpkeys);
    w = XmCreatePushButton(btnctrl, "delete", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, deleteRowCb, propw.pgpkeys);

    ctrl = XmCreateRowColumn(page, "miscctrl", NULL, 0);
    XtManageChild(ctrl);
    cap = XtVaCreateManagedWidget("seccap",
				  xbaeCaptionWidgetClass, ctrl,
				  NULL);
    spin = XmCreateSpinBox(cap, "secspin", NULL, 0);
    XtManageChild(spin);
    propw.security_level = XmCreateTextField(spin, "security", NULL, 0);
    XtManageChild(propw.security_level);
    cap = XtVaCreateManagedWidget("intcap",
				  xbaeCaptionWidgetClass, ctrl,
				  NULL);
    propw.test_interval = XmCreateTextField(cap, "interval", NULL, 0);
    XtManageChild(propw.test_interval);

    ctrl = XmCreateRowColumn(page, "psctrl", NULL, 0);
    XtManageChild(ctrl);
    cap = XtVaCreateManagedWidget("pseudocap",
				  xbaeCaptionWidgetClass, ctrl,
				  NULL);
    propw.pseudonyms = XtVaCreateManagedWidget("pseudogrid",
					       xbaeMatrixWidgetClass, cap,
					       NULL);
    btnctrl = XmCreateRowColumn(ctrl, "btnctrl", NULL, 0);
    XtManageChild(btnctrl);
    w = XmCreatePushButton(btnctrl, "add", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, addRowCb, propw.pseudonyms);
    w = XmCreatePushButton(btnctrl, "delete", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, deleteRowCb, propw.pseudonyms);

    w = XmCreateSeparatorGadget(ctrl, "gridsep", NULL, 0);
    XtManageChild(w);

    cap = XtVaCreateManagedWidget("killcap",
			    xbaeCaptionWidgetClass, ctrl,
			    NULL);
    propw.kill_list = XtVaCreateManagedWidget("killgrid",
					      xbaeMatrixWidgetClass, cap,
					      NULL);
    XtAddCallback(propw.kill_list, XmNenterCellCallback, enterCellCb, NULL);
    btnctrl = XmCreateRowColumn(ctrl, "btnctrl", NULL, 0);
    XtManageChild(btnctrl);
    w = XmCreatePushButton(btnctrl, "add", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, addRowCb, propw.kill_list);
    w = XmCreatePushButton(btnctrl, "delete", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, deleteRowCb, propw.kill_list);
    killuser = XmCreateToggleButton(btnctrl, "from", NULL, 0);
    XtManageChild(killuser);
    XmToggleButtonSetState(killuser, (killtype == KILL_USER), False);
    XtVaSetValues(killuser, XmNuserData, propw.kill_list, NULL);
    XtAddCallback(killuser, XmNvalueChangedCallback, killToggleCb,
		  (XtPointer)KILL_USER);
    killsubj = XmCreateToggleButton(btnctrl, "subject", NULL, 0);
    XtManageChild(killsubj);
    XmToggleButtonSetState(killsubj, (killtype == KILL_SUBJECT), False);
    XtVaSetValues(killsubj, XmNuserData, propw.kill_list, NULL);
    XtAddCallback(killsubj, XmNvalueChangedCallback, killToggleCb,
		  (XtPointer)KILL_SUBJECT);

    cap = XtVaCreateManagedWidget("dpcap", xbaeCaptionWidgetClass, page, NULL);
    propw.default_nym = XmCreateTextField(cap, "default", NULL, 0);
    XtManageChild(propw.default_nym);

    load_pgpstuff();
} /* create_pgp_page */

/*----------------------------------------------------------------------*/

static void
create_hdr_page(Widget parent)
{
    char	*bgnd;
    Widget	w, page, cap;
    Colormap	cmap;
    XcmsColor	cexact, cscreen;

    page = XmCreateRowColumn(parent, "hdrpage", NULL, 0);
    XtManageChild(page);
    w = XmCreatePushButton(parent, "hdrbtn", NULL, 0);
    XtManageChild(w);

    cap = XtVaCreateManagedWidget("hdrcap",
			    xbaeCaptionWidgetClass, page,
			    NULL);
    propw.hdr_list = XtVaCreateManagedWidget("hdrgrid",
					      xbaeMatrixWidgetClass, cap,
					      NULL);
    XtAddCallback(propw.hdr_list, XmNselectCellCallback, toggleCellCb,
		  NULL);

    /* Get color for header notebook. */
    bgnd = (char *)GetResourceString(propw.hdr_list, "retainColor",
				     "Background");
    if( bgnd != NULL ) {
	XtVaGetValues(propw.hdr_list, XmNcolormap, &cmap, NULL);
	XcmsAllocNamedColor(XtDisplay(propw.hdr_list), cmap, bgnd,
			    &cexact, &cscreen, XcmsRGBFormat);
	XtVaSetValues(propw.hdr_list, XmNuserData,
		      (XtPointer)cscreen.pixel, NULL);
    }
    else {
	Pixel	bg;

	XtVaGetValues(propw.hdr_list, XmNbackground, &bg, NULL);
		XtVaSetValues(propw.hdr_list, XmNuserData,
		      (XtPointer)bg, NULL);
    }


    load_hdrstuff();
} /* create_hdr_page */

/*----------------------------------------------------------------------*/

/* Read the aliases from the in-memory list. Saves accessing disk. */
static void
load_aliases()
{
    MAILRC		*m;
    int			rownum = 0, numrows;

    numrows = XbaeMatrixNumRows(propw.aliases);
    if(numrows < alias.number) {
	XbaeMatrixAddRows(propw.aliases, numrows, NULL, NULL, NULL,
			  alias.number - numrows);
    }

    m = alias.start;

    while (m) {
	XbaeMatrixSetCell(propw.aliases, rownum, 0, m->name);
	XbaeMatrixSetCell(propw.aliases, rownum, 1, m->value);
	rownum++;
	m = m->next;
    }
} /* load_aliases */

/*----------------------------------------------------------------------*/

static void
load_pgpstuff()
{
    int		row = 0, psrow = 0, killrow = 0, temp, minval, seclev;
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
		if(row >= (temp = XbaeMatrixNumRows(propw.pgpkeys))){
		    XbaeMatrixAddRows(propw.pgpkeys, temp, NULL, NULL, NULL,
				      (row - temp + 1));
		}
		XbaeMatrixSetCell(propw.pgpkeys, row, 0, name);
		XbaeMatrixSetCell(propw.pgpkeys, row, 1, key);
		row++;
	    }
	    else if(! strcmp(tok, "#@pseudonym")){
		name = strtok(NULL, " \t\n\r");
		if(!name)
		    continue;
		if(psrow >= (temp = XbaeMatrixNumRows(propw.pseudonyms))){
		    XbaeMatrixAddRows(propw.pseudonyms, temp, NULL, NULL, NULL,
				      (psrow - temp + 1));
		}
		XbaeMatrixSetCell(propw.pseudonyms, psrow, 0, name);
		psrow++;
	    }
	    else if(! strcmp(tok, "#@defnym")){
		name = strtok(NULL, " \t\n\r");
		if(!name)
		    continue;
		XtVaSetValues(propw.default_nym, XmNvalue, name, NULL);
	    }
	    else if(! strcmp(tok, "#@security")){
		name = strtok(NULL, " \t\n\r");
		if(!name)
		    continue;
		XtVaGetValues(propw.security_level,
			      XmNminimumValue, &minval,
			      NULL);
		seclev = atoi(name);
		XtVaSetValues(propw.security_level,
			      XmNposition, seclev - minval, NULL);
	    }
	    else if(! strcmp(tok, "#@killu")){
		name = strtok(NULL, " \t\n\r");
		if(!name)
		    continue;
		if(killrow >= (temp = XbaeMatrixNumRows(propw.kill_list))){
		    XbaeMatrixAddRows(propw.kill_list, temp, NULL, NULL, NULL,
				      (killrow - temp + 1));
		}
		XbaeMatrixSetCell(propw.kill_list, killrow, 0, "F");
		XbaeMatrixSetCell(propw.kill_list, killrow, 1, name);
		XbaeMatrixSetCellUserData(propw.kill_list, killrow, 1,
					  (XtPointer)KILL_USER);
		killrow++;
	    }
	    else if(! strcmp(tok, "#@kills")){
		name = strtok(NULL, " \t\n\r");
		if(!name)
		    continue;
		if(killrow >= (temp = XbaeMatrixNumRows(propw.kill_list))){
		    XbaeMatrixAddRows(propw.kill_list, temp, NULL, NULL, NULL,
				      (killrow - temp + 1));
		}
		XbaeMatrixSetCell(propw.kill_list, killrow, 0, "S");
		XbaeMatrixSetCell(propw.kill_list, killrow, 1, name);
		XbaeMatrixSetCellUserData(propw.kill_list, killrow, 1,
					  (XtPointer)KILL_SUBJECT);
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
		XtVaSetValues(propw.test_interval, XmNvalue, name, NULL);
	    }
	}
	fclose(privrc);
    }
} /* load_pgpstuff */

/*----------------------------------------------------------------------*/

static void
load_hdrstuff()
{
    int		hdr_num, i;
    Pixel	rcolor;

    XtVaGetValues(propw.hdr_list, XmNuserData, &rcolor, NULL);

    for(hdr_num = 0; standard_headers[hdr_num].name != NULL; hdr_num++)
	;

    i = XbaeMatrixNumRows(propw.hdr_list);
    if( i < hdr_num ) {
	XbaeMatrixAddRows(propw.hdr_list, i, NULL, NULL, NULL,
			  hdr_num - i);
    }
    for( i = 0; i < hdr_num; i++ ) {
	XbaeMatrixSetCell(propw.hdr_list, i, 0,
			  standard_headers[i].name);
	if( retain_line(standard_headers[i].name) ) {
	    XbaeMatrixSetCell(propw.hdr_list, i, 1, SHOWN);
	    XbaeMatrixSetRowUserData(propw.hdr_list, i, (XtPointer)True);
	    XbaeMatrixSetRowBackgrounds(propw.hdr_list, i, &rcolor, 1);
	}
	else {
	    XbaeMatrixSetCell(propw.hdr_list, i, 1, IGNORED);
	    XbaeMatrixSetRowUserData(propw.hdr_list, i, False);
	}
    }
} /* load_hdrstuff */

/*----------------------------------------------------------------------*/

static void
toggleCellCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    XbaeMatrixSelectCellCallbackStruct *cbs =
	(XbaeMatrixSelectCellCallbackStruct *)calldata;
    Boolean	is_set;
    Pixel	rcolor, bg;

    XtVaGetValues(propw.hdr_list, XmNuserData, &rcolor, NULL);
    XtVaGetValues(propw.hdr_list, XmNbackground, &bg, NULL);

    is_set = (int)XbaeMatrixGetRowUserData(w, cbs->row);
    XbaeMatrixSetRowUserData(w, cbs->row, (XtPointer) !is_set);
    XbaeMatrixSetCell(w, cbs->row, 1, (is_set? IGNORED: SHOWN));
    XbaeMatrixSetRowBackgrounds(propw.hdr_list, cbs->row,
				(is_set? &bg: &rcolor), 1);
} /* toggleCellCb */

/*----------------------------------------------------------------------*/

static void
addRowCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    Widget	matrix = (Widget)clientdata;
    int		numrows, numcols, i, j, numvisrows;
    Dimension	fixrows, fixcols;
    Boolean	isempty;
    String	cellval;

    if(XtClass(matrix) != xbaeMatrixWidgetClass)
	return;			/* Wrong clientdata widget */

    /* Find a blank row */
    numrows = XbaeMatrixNumRows(matrix);
    numvisrows = XbaeMatrixVisibleRows(matrix);
    numcols = XbaeMatrixNumColumns(matrix);
    XtVaGetValues(matrix, XmNfixedColumns, &fixcols,
		  XmNfixedRows, &fixrows, NULL);
    for(i = fixrows; i < numrows; i++){
	isempty = False;
	for(j = fixcols; j < numcols; j++){
	    cellval = XbaeMatrixGetCell(matrix, i, j);
	    if(*cellval == '\0'){
		isempty = True;
		break;
	    }
	}
	if(isempty){
	    /* Set empty row to be editable */
	    XbaeMatrixEditCell(matrix, i, fixcols);
	    break;
	}
    }

    /* No empty rows so add one. */
    if(i == numrows){
	XbaeMatrixAddRows(matrix, numrows, NULL, NULL, NULL, 1);
	XbaeMatrixEditCell(matrix, numrows, 0);
	if(numvisrows == numrows){
	    XtVaSetValues(matrix, XmNwidth, 0, NULL);
	}
    }
} /* addRowCb */

/*----------------------------------------------------------------------*/

static void
deleteRowCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    Widget	matrix = (Widget)clientdata;
    int		numrows, numvisrows;
    int		selrow, selcol;

    if(XtClass(matrix) != xbaeMatrixWidgetClass)
	return;			/* Wrong clientdata widget */

    numrows = XbaeMatrixNumRows(matrix);
    numvisrows = XbaeMatrixVisibleRows(matrix);
    XbaeMatrixGetCurrentCell(matrix, &selrow, &selcol);
    XbaeMatrixDeleteRows(matrix, selrow, 1);

    if(numrows <= numvisrows){
	XbaeMatrixAddRows(matrix, numrows - 1, NULL, NULL, NULL, 1);
    }

    XbaeMatrixRefresh(matrix);
} /* deleteRowCb */

/*----------------------------------------------------------------------*/

static void
load_guistuff()
{
    int		rownum = 0, numrows;
    char	*rcval, *temp, *ent;

    if( rcval = find_mailrc("folder") )
	XtVaSetValues(propw.mail_dir, XmNvalue, rcval, NULL);

    if( rcval = find_mailrc("filemenu2") ) {
	temp = strdup(rcval);
	ent = strtok(temp, " \n\r\t");
	numrows = XbaeMatrixNumRows(propw.mail_menu);
	do{
	    if(rownum >= numrows) {
		XbaeMatrixAddRows(propw.mail_menu, numrows, NULL, NULL,
				  NULL, rownum - numrows + 1);
		numrows = XbaeMatrixNumRows(propw.mail_menu);
	    }
	    XbaeMatrixSetCell(propw.mail_menu, rownum, 0, ent);
	    rownum++;
	}while((ent = strtok(NULL, " \n\r\t")));
	free(temp);
    }

    if( rcval = find_mailrc("printmail") ) {
	XtVaSetValues(propw.print_cmd, XmNvalue, rcval, NULL);
    }
#if 0
    else {
	XtVaSetValues(propw.print_cmd, XmNvalue, "|lpr", NULL);
    }
#endif

    XmToggleButtonSetState(propw.detached,
			   (int)find_mailrc("detachedmessagewin"), False);
} /* load_guistuff */

/*----------------------------------------------------------------------*/

static void
load_compstuff()
{
    char	*rcval;

    if( rcval = find_mailrc("replyto") )
	XtVaSetValues(propw.replyto, XmNvalue, rcval, NULL);

    if( rcval = find_mailrc("organization") )
	XtVaSetValues(propw.organizn, XmNvalue, rcval, NULL);

    if( rcval = find_mailrc("record") )
	XtVaSetValues(propw.record, XmNvalue, rcval, NULL);

    if( rcval = find_mailrc("indentprefix") ) {
	XtVaSetValues(propw.indent, XmNvalue, rcval, NULL);
    }
    else {
	XtVaSetValues(propw.indent, XmNvalue, "> ", NULL);
    }

    if( rcval = find_mailrc("domain") )
	XtVaSetValues(propw.domain, XmNvalue, rcval, NULL);

    XmToggleButtonSetState(propw.comp_sign,
			   !(int)find_mailrc("nodefaultsign"), False);
    XmToggleButtonSetState(propw.comp_encrypt,
			   !(int)find_mailrc("nodefaultencrypt"), False);
    XmToggleButtonSetState(propw.comp_log,
			   (int)find_mailrc("nodontlogmessages"), False);
    XmToggleButtonSetState(propw.comp_lograw,
			   (int)find_mailrc("log-raw"), False);
#ifdef HAVE_MIXMASTER
    XmToggleButtonSetState(propw.comp_remail,
			   !(int)find_mailrc("nodefaultremail"), False);
#endif
    XmToggleButtonSetState(propw.badbeep,
			   !(int)find_mailrc("nobeepbadsig"), False);
} /* load_compstuff */

/*----------------------------------------------------------------------*/

static void
killToggleCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    XmToggleButtonCallbackStruct *cbs =
	(XmToggleButtonCallbackStruct *)calldata;
    Widget	matrix;
    int		row, col;

    if(cbs->set){
	killtype = (KillType)clientdata;
	XtVaGetValues(w, XmNuserData, &matrix, NULL);
	XbaeMatrixGetCurrentCell(matrix, &row, &col);
	XbaeMatrixSetCellUserData(matrix, row, col, (XtPointer)killtype);
	XbaeMatrixSetCell(matrix, row, 0, (killtype == KILL_USER)? "F": "S");
    }
} /* killToggleCb */

/*----------------------------------------------------------------------*/

static void
enterCellCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    XbaeMatrixEnterCellCallbackStruct *cbs =
	(XbaeMatrixEnterCellCallbackStruct *)calldata;
    killtype = (KillType)XbaeMatrixGetCellUserData(w, cbs->row, cbs->column);

    XmToggleButtonSetState(killuser, (killtype == KILL_USER), False);
    XmToggleButtonSetState(killsubj, (killtype == KILL_SUBJECT), False);
    XbaeMatrixSetCell(w, cbs->row, 0, (killtype == KILL_USER)? "F": "S");
} /* enterCellCb */

/*----------------------------------------------------------------------*/

static void
savePropsCb(Widget w, XtPointer clientdata, XtPointer calldata)
{
    int		numrows, i;
    char	newname[BUFSIZ], *alias_name, *buf, buf2[BUFSIZ];
    FILE	*nprivrc, *privrc;
    MAILRC	*m;
    XmAnyCallbackStruct *cbs = (XmAnyCallbackStruct *)calldata;

    if(cbs->reason != XmCR_CANCEL){
	strcpy(newname, globRCfile);
	strcat(newname, ".new");

	if((nprivrc = fopen(newname, "w")) == NULL) {
	    alert(propswin, "noprops", 1);
	    return;
	}
	if((privrc = fopen(globRCfile, "r")) == NULL) {
	    alert(propswin, "noprops", 1);
	    return;
	}

	/* Write all our aliases first and update in-memory list. */
	fprintf(nprivrc, "#\n# Aliases\n#\n");
	clear_aliases();
	XbaeMatrixCommitEdit(propw.aliases, True);
	XbaeMatrixDeselectAll(propw.aliases);
	numrows = XbaeMatrixNumRows(propw.aliases);
	for(i = 0; i < numrows; i++) {
	    alias_name = XbaeMatrixGetCell(propw.aliases, i, 0);
	    if(*alias_name == '\0')
		break;		/* End of valid aliases */
	    fprintf(nprivrc, "alias %s %s\n", alias_name,
		    XbaeMatrixGetCell(propw.aliases, i, 1));
	    m = new_mailrc();
	    m->name = strdup(alias_name);
	    m->value = strdup(XbaeMatrixGetCell(propw.aliases, i, 1));
	    add_to_list(&alias, m);
	}

	/* Followed by the retain line. */
	fprintf(nprivrc, "#\n# Retained headers\n#\n");
	fprintf(nprivrc, "retain");
	numrows = XbaeMatrixNumRows(propw.hdr_list);
	for(i = 0; i < numrows; i++) {
	    buf = XbaeMatrixGetCell(propw.hdr_list, i, 0);
	    if( (int)XbaeMatrixGetRowUserData(propw.hdr_list, i) ) {
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
	buf = XmTextFieldGetString(propw.mail_dir);
	if(*buf != '\0') {
	    fprintf(nprivrc, "set folder='%s'\n", buf);
	    replace_mailrc("folder", buf);
	}
	else {
	    remove_mailrc("folder");
	}
	XtFree(buf);
	buf = XmTextFieldGetString(propw.print_cmd);
	if(*buf != '\0') {
	    fprintf(nprivrc, "set printmail='%s'\n", buf);
	    replace_mailrc("printmail", buf);
	}
	else {
	    remove_mailrc("printmail");
	}
	XtFree(buf);
	buf = XmTextFieldGetString(propw.replyto);
	if(*buf != '\0') {
	    fprintf(nprivrc, "set replyto='%s'\n", buf);
	    replace_mailrc("replyto", buf);
	}
	else {
	    remove_mailrc("replyto");
	}
	XtFree(buf);
	buf = XmTextFieldGetString(propw.domain);
	if(*buf != '\0') {
	    fprintf(nprivrc, "set domain='%s'\n", buf);
	    replace_mailrc("domain", buf);
	}
	else {
	    remove_mailrc("domain");
	}
	XtFree(buf);
	buf = XmTextFieldGetString(propw.indent);
	if(*buf != '\0') {
	    fprintf(nprivrc, "set indentprefix='%s'\n", buf);
	    replace_mailrc("indentprefix", buf);
	}
	else {
	    remove_mailrc("indentprefix");
	}
	XtFree(buf);
	buf = XmTextFieldGetString(propw.organizn);
	if(*buf != '\0') {
	    fprintf(nprivrc, "set organization='%s'\n", buf);
	    replace_mailrc("organization", buf);
	}
	else {
	    remove_mailrc("organization");
	}
	XtFree(buf);
	buf = XmTextFieldGetString(propw.record);
	if(*buf != '\0') {
	    fprintf(nprivrc, "set record='%s'\n", buf);
	    replace_mailrc("record", buf);
	}
	else {
	    remove_mailrc("record");
	}
	XtFree(buf);

	fprintf(nprivrc, "set filemenu2='");
	XbaeMatrixCommitEdit(propw.mail_menu, True);
	XbaeMatrixDeselectAll(propw.mail_menu);
	numrows = XbaeMatrixNumRows(propw.mail_menu);
	for(i = 0; i < numrows; i++) {
	    buf = XbaeMatrixGetCell(propw.mail_menu, i, 0);
	    if(*buf == '\0')
		break;		/* End of valid items */
	    if(i > 0) fprintf(nprivrc, " ");
	    fprintf(nprivrc, "%s", buf);
	}
	fprintf(nprivrc, "'\n");
	if(XmToggleButtonGetState(propw.detached)) {
	    fprintf(nprivrc, "set detachedmessagewin\n");
	    replace_mailrc("detachedmessagewin", "");
	}
	else {
	    remove_mailrc("detachedmessagewin");
	}
	if(!XmToggleButtonGetState(propw.comp_sign)) {
	    fprintf(nprivrc, "set nodefaultsign\n");
	    replace_mailrc("nodefaultsign", "");
	}
	else {
	    remove_mailrc("nodefaultsign");
	}
	if(!XmToggleButtonGetState(propw.comp_encrypt)) {
	    fprintf(nprivrc, "set nodefaultencrypt\n");
	    replace_mailrc("nodefaultencrypt", "");
	}
	else {
	    remove_mailrc("nodefaultencrypt");
	}
	if(XmToggleButtonGetState(propw.comp_log)) {
	    fprintf(nprivrc, "set nodontlogmessages\n");
	    replace_mailrc("nodontlogmessages", "");
	}
	else {
	    remove_mailrc("nodontlogmessages");
	}
	if(XmToggleButtonGetState(propw.comp_lograw)) {
	    fprintf(nprivrc, "set log-raw\n");
	    replace_mailrc("log-raw", "");
	}
	else {
	    remove_mailrc("log-raw");
	}
#ifdef HAVE_MIXMASTER
	if(!XmToggleButtonGetState(propw.comp_remail)) {
	    fprintf(nprivrc, "set nodefaultremail\n");
	    replace_mailrc("nodefaultremail", "");
	}
	else {
	    remove_mailrc("nodefaultremail");
	}
#endif
	if(!XmToggleButtonGetState(propw.badbeep)) {
	    fprintf(nprivrc, "set nobeepbadsig\n");
	    replace_mailrc("nobeepbadsig", "");
	}
	else {
	    remove_mailrc("nobeepbadsig");
	}

	/* Now copy over any set lines from the old file that we don't
	   set here. */
	while(fgets(buf2, BUFSIZ, privrc) != NULL) {
	    if( strncmp(buf2, "set ", 4) != 0)
		continue;

	    if( (strncmp(buf2, "set folder=", 11) != 0) &&
		(strncmp(buf2, "set printmail=", 14) != 0) &&
		(strncmp(buf2, "set domain=", 11) != 0) &&
		(strncmp(buf2, "set indentprefix=", 17) != 0) &&
		(strncmp(buf2, "set organization=", 17) != 0) &&
		(strncmp(buf2, "set record=", 11) != 0) &&
		(strncmp(buf2, "set filemenu2=", 14) != 0) &&
		(strncmp(buf2, "set detachedmessagewin", 22) != 0) &&
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
	XbaeMatrixCommitEdit(propw.pgpkeys, True);
	XbaeMatrixDeselectAll(propw.pgpkeys);
	numrows = XbaeMatrixNumRows(propw.pgpkeys);
	for(i = 0; i < numrows; i++) {
	    buf = XbaeMatrixGetCell(propw.pgpkeys, i, 0);
	    if(*buf == '\0')
		break;		/* End of valid keys */
	    fprintf(nprivrc, "#@pgpkey %s=%s\n", buf,
		    XbaeMatrixGetCell(propw.pgpkeys, i, 1));
	}

	/* Default pseudonym. */
	buf = XmTextFieldGetString(propw.default_nym);
	if(*buf != '\0')
	    fprintf(nprivrc, "#@defnym %s\n", buf);
	XtFree(buf);

	/* Security level. */
	buf = XmTextFieldGetString(propw.security_level);
	if(*buf != '\0')
	    fprintf(nprivrc, "#@security %s\n", buf);
	XtFree(buf);

	/* Test interval. */
	buf = XmTextFieldGetString(propw.test_interval);
	if(*buf != '\0')
	    fprintf(nprivrc, "testinterval='%s'\n", buf);
	XtFree(buf);

	/* Mail kill items. */
	XbaeMatrixCommitEdit(propw.kill_list, True);
	XbaeMatrixDeselectAll(propw.kill_list);
	numrows = XbaeMatrixNumRows(propw.kill_list);
	for(i = 0; i < numrows; i++) {
	    buf = XbaeMatrixGetCell(propw.kill_list, i, 0);
	    if(*buf == '\0')
		break;		/* End of valid keys */
	    fprintf(nprivrc, "#@kill%s %s\n",
		    (*buf == 'F')? "u": "s",
		    XbaeMatrixGetCell(propw.kill_list, i, 1));
	}

	/* Pseudonyms. */
	XbaeMatrixCommitEdit(propw.pseudonyms, True);
	XbaeMatrixDeselectAll(propw.pseudonyms);
	numrows = XbaeMatrixNumRows(propw.pseudonyms);
	for(i = 0; i < numrows; i++) {
	    buf = XbaeMatrixGetCell(propw.pseudonyms, i, 0);
	    if(*buf == '\0')
		break;		/* End of valid keys */
	    fprintf(nprivrc, "#@pseudonym %s\n", buf);
	}

	/* Close file. */
	fclose(nprivrc);

	/* Do some renaming of files. */
	strcpy(buf2, globRCfile);
	strcat(buf2, ".bak");
	rename(globRCfile, buf2);
	rename(newname, globRCfile);

	/* Change window layout if necessary. */
	detachMW(XmToggleButtonGetState(propw.detached));
    }

    if(cbs->reason == XmCR_OK || cbs->reason == XmCR_CANCEL)
	XtUnmanageChild((Widget)clientdata);
} /* save_props */

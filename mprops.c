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

#include	<stdio.h>
#include	<stdlib.h>
#include	<X11/X.h>
#include	<X11/Xlib.h>
#include	<X11/Intrinsic.h>
#include	<Xm/Xm.h>
#include	<Xm/Protocols.h>
#include	<Xm/MessageB.h>
#include	<Xm/DialogS.h>
#include	<Xm/Notebook.h>
#include	<Xm/RowColumn.h>
#include	<Xm/PushB.h>
#include	<Xm/SeparatoG.h>
#include	<Xm/SpinB.h>
#include	<Xm/TextF.h>
#include	<Xm/ToggleB.h>
#include	<X11/Xmu/Editres.h>
#include	<Xbae/Matrix.h>
#include	<Xbae/Caption.h>
#ifndef MAXPATHLEN
#include	<sys/param.h>
#endif /* MAXPATHLEN */
#include	"def.h"
#include	"buffers.h"
#include	"message.h"
#include	"mailrc.h"

extern  char		globRCfile[MAXPATHLEN];  /* from main.c */

typedef enum {
    KILL_SUBJECT,
    KILL_USER
} KillType;

static Widget		propswin = NULL, killuser, killsubj;
static KillType		killtype = KILL_SUBJECT;

static void		create_gui_page(Widget);
static void		create_alias_page(Widget);
static void		create_mail_page(Widget);
static void		create_pgp_page(Widget);
static void		load_aliases(Widget);
static void		load_pgpstuff(Widget, Widget, Widget, Widget,
				      Widget, Widget);
static void		load_guistuff(Widget, Widget);
static void		closeCb(Widget, XtPointer, XtPointer);
static void		addRowCb(Widget, XtPointer, XtPointer);
static void		deleteRowCb(Widget, XtPointer, XtPointer);
static void		killToggleCb(Widget, XtPointer, XtPointer);
static void		enterCellCb(Widget, XtPointer, XtPointer);

void
show_props(Widget parent)
{
    Widget	w, notebook;

    if(propswin == NULL){
	propswin = XmCreateMessageDialog(parent, "props", NULL, 0);
	XtUnmanageChild(XtNameToWidget(propswin, "*Help"));
	w = XmCreatePushButton(propswin, "apply", NULL, 0);
	XtManageChild(w);

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
	create_alias_page(notebook);
	/*create_mail_page(notebook);*/
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
    Widget	w, page, ctrl, btnctrl, cap, mw, dir;

    page = XmCreateRowColumn(parent, "guipage", NULL, 0);
    XtManageChild(page);
    w = XmCreatePushButton(parent, "guibtn", NULL, 0);
    XtManageChild(w);

    cap = XtVaCreateManagedWidget("dircap", xbaeCaptionWidgetClass,
				  page, NULL);
    dir = XmCreateTextField(cap, "directory", NULL, 0);
    XtManageChild(dir);
    ctrl = XmCreateRowColumn(page, "menuctrl", NULL, 0);
    XtManageChild(ctrl);
    cap = XtVaCreateManagedWidget("menucap", xbaeCaptionWidgetClass,
				  ctrl, NULL);
    mw = XtVaCreateManagedWidget("menugrid",
                                 xbaeMatrixWidgetClass, cap,
                                 NULL);
    btnctrl = XmCreateRowColumn(ctrl, "btnctrl", NULL, 0);
    XtManageChild(btnctrl);
    w = XmCreatePushButton(btnctrl, "add", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, addRowCb, mw);
    w = XmCreatePushButton(btnctrl, "delete", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, deleteRowCb, mw);

    load_guistuff(dir, mw);
} /* create_gui_page */

/*----------------------------------------------------------------------*/

static void
create_alias_page(Widget parent)
{
    Widget	w, page, mw, ctrl;

    page = XmCreateRowColumn(parent, "aliaspage", NULL, 0);
    XtManageChild(page);
    w = XmCreatePushButton(parent, "aliasbtn", NULL, 0);
    XtManageChild(w);
    mw = XtVaCreateManagedWidget("aliasgrid",
                                 xbaeMatrixWidgetClass, page,
                                 NULL);

    ctrl = XmCreateRowColumn(page, "btn_ctrl", NULL, 0);
    XtManageChild(ctrl);
    w = XmCreatePushButton(ctrl, "add", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, addRowCb, mw);
    w = XmCreatePushButton(ctrl, "delete", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, deleteRowCb, mw);

    load_aliases(mw);
} /* create_alias_page */

/*----------------------------------------------------------------------*/

#if 0
static void
create_mail_page(Widget parent)
{
    Widget	w, page;

    page = XmCreateRowColumn(parent, "mailpage", NULL, 0);
    XtManageChild(page);
    w = XmCreatePushButton(parent, "mailbtn", NULL, 0);
    XtManageChild(w);
} /* create_mail_page */
#endif

/*----------------------------------------------------------------------*/

static void
create_pgp_page(Widget parent)
{
    Widget	w, page, ctrl, btnctrl, cap, mw1, mw2, mw3, spin;
    Widget	sec, interval;

    page = XmCreateRowColumn(parent, "pgppage", NULL, 0);
    XtManageChild(page);
    w = XmCreatePushButton(parent, "pgpbtn", NULL, 0);
    XtManageChild(w);

    ctrl = XmCreateRowColumn(page, "keyctrl", NULL, 0);
    XtManageChild(ctrl);
    cap = XtVaCreateManagedWidget("pgpkeycap",
			    xbaeCaptionWidgetClass, ctrl,
			    NULL);
    mw1 = XtVaCreateManagedWidget("pgpkeygrid",
				 xbaeMatrixWidgetClass, cap,
				 NULL);
    btnctrl = XmCreateRowColumn(ctrl, "btnctrl", NULL, 0);
    XtManageChild(btnctrl);
    w = XmCreatePushButton(btnctrl, "add", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, addRowCb, mw1);
    w = XmCreatePushButton(btnctrl, "delete", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, deleteRowCb, mw1);

    ctrl = XmCreateRowColumn(page, "miscctrl", NULL, 0);
    XtManageChild(ctrl);
    cap = XtVaCreateManagedWidget("seccap",
				  xbaeCaptionWidgetClass, ctrl,
				  NULL);
    spin = XmCreateSpinBox(cap, "secspin", NULL, 0);
    XtManageChild(spin);
    sec = XmCreateTextField(spin, "security", NULL, 0);
    XtManageChild(sec);
    cap = XtVaCreateManagedWidget("intcap",
				  xbaeCaptionWidgetClass, ctrl,
				  NULL);
    interval = XmCreateTextField(cap, "interval", NULL, 0);
    XtManageChild(interval);

    ctrl = XmCreateRowColumn(page, "psctrl", NULL, 0);
    XtManageChild(ctrl);
    cap = XtVaCreateManagedWidget("pseudocap",
				  xbaeCaptionWidgetClass, ctrl,
				  NULL);
    mw2 = XtVaCreateManagedWidget("pseudogrid",
				 xbaeMatrixWidgetClass, cap,
				 NULL);
    btnctrl = XmCreateRowColumn(ctrl, "btnctrl", NULL, 0);
    XtManageChild(btnctrl);
    w = XmCreatePushButton(btnctrl, "add", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, addRowCb, mw2);
    w = XmCreatePushButton(btnctrl, "delete", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, deleteRowCb, mw2);

    w = XmCreateSeparatorGadget(ctrl, "gridsep", NULL, 0);
    XtManageChild(w);

    cap = XtVaCreateManagedWidget("killcap",
			    xbaeCaptionWidgetClass, ctrl,
			    NULL);
    mw3 = XtVaCreateManagedWidget("killgrid",
				 xbaeMatrixWidgetClass, cap,
				 NULL);
    XtAddCallback(mw3, XmNenterCellCallback, enterCellCb, NULL);
    btnctrl = XmCreateRowColumn(ctrl, "btnctrl", NULL, 0);
    XtManageChild(btnctrl);
    w = XmCreatePushButton(btnctrl, "add", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, addRowCb, mw3);
    w = XmCreatePushButton(btnctrl, "delete", NULL, 0);
    XtManageChild(w);
    XtAddCallback(w, XmNactivateCallback, deleteRowCb, mw3);
    killuser = XmCreateToggleButton(btnctrl, "from", NULL, 0);
    XtManageChild(killuser);
    XmToggleButtonSetState(killuser, (killtype == KILL_USER), False);
    XtVaSetValues(killuser, XmNuserData, mw3, NULL);
    XtAddCallback(killuser, XmNvalueChangedCallback, killToggleCb,
		  (XtPointer)KILL_USER);
    killsubj = XmCreateToggleButton(btnctrl, "subject", NULL, 0);
    XtManageChild(killsubj);
    XmToggleButtonSetState(killsubj, (killtype == KILL_SUBJECT), False);
    XtVaSetValues(killsubj, XmNuserData, mw3, NULL);
    XtAddCallback(killsubj, XmNvalueChangedCallback, killToggleCb,
		  (XtPointer)KILL_SUBJECT);

    cap = XtVaCreateManagedWidget("dpcap", xbaeCaptionWidgetClass, page, NULL);
    w = XmCreateTextField(cap, "default", NULL, 0);
    XtManageChild(w);

    load_pgpstuff(mw1, mw2, mw3, w, sec, interval);
} /* create_pgp_page */

/*----------------------------------------------------------------------*/

static void
load_aliases(Widget w)
{
    int		rownum = 0;
    char	buf[BUFSIZ], *p_buf, *tok, *name, *addr;
    FILE 	*privrc;

    if((privrc = fopen(globRCfile,"r")) != NULL){
	while((p_buf = fgets(buf, BUFSIZ, privrc)) != NULL){
	    tok = strtok(p_buf, " \t");
	    if(!tok || strcmp(tok, "alias"))
		continue;
	    name = strtok(NULL, " \t");
	    if(name == NULL)
		continue;
	    addr = strtok(NULL, " \t\n\r");
	    XbaeMatrixSetCell(w, rownum, 0, name);
	    XbaeMatrixSetCell(w, rownum, 1, addr);
	    rownum++;
	}
	fclose(privrc);
    }
} /* load_aliases */

/*----------------------------------------------------------------------*/

static void
load_pgpstuff(Widget mat1, Widget mat2, Widget mat3, Widget defnym,
	      Widget sec, Widget interval)
{
    int		row = 0, psrow = 0, killrow = 0, temp;
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
		if(row >= (temp = XbaeMatrixNumRows(mat1))){
		    XbaeMatrixAddRows(mat1, temp, NULL, NULL, NULL,
				      (row - temp + 1));
		}
		XbaeMatrixSetCell(mat1, row, 0, name);
		XbaeMatrixSetCell(mat1, row, 1, key);
		row++;
	    }
	    else if(! strcmp(tok, "#@pseudonym")){
		name = strtok(NULL, " \t\n\r");
		if(!name)
		    continue;
		if(psrow >= (temp = XbaeMatrixNumRows(mat2))){
		    XbaeMatrixAddRows(mat2, temp, NULL, NULL, NULL,
				      (psrow - temp + 1));
		}
		XbaeMatrixSetCell(mat2, psrow, 0, name);
		psrow++;
	    }
	    else if(! strcmp(tok, "#@defnym")){
		name = strtok(NULL, " \t\n\r");
		if(!name)
		    continue;
		XtVaSetValues(defnym, XmNvalue, name, NULL);
	    }
	    else if(! strcmp(tok, "#@security")){
		name = strtok(NULL, " \t\n\r");
		if(!name)
		    continue;
		XtVaSetValues(sec, XmNvalue, name, NULL);
	    }
	    else if(! strcmp(tok, "#@killu")){
		name = strtok(NULL, " \t\n\r");
		if(!name)
		    continue;
		if(killrow >= (temp = XbaeMatrixNumRows(mat3))){
		    XbaeMatrixAddRows(mat3, temp, NULL, NULL, NULL,
				      (killrow - temp + 1));
		}
		XbaeMatrixSetCell(mat3, killrow, 0, "F");
		XbaeMatrixSetCell(mat3, killrow, 1, name);
		XbaeMatrixSetCellUserData(mat3, killrow, 1,
					  (XtPointer)KILL_USER);
		killrow++;
	    }
	    else if(! strcmp(tok, "#@kills")){
		name = strtok(NULL, " \t\n\r");
		if(!name)
		    continue;
		if(killrow >= (temp = XbaeMatrixNumRows(mat3))){
		    XbaeMatrixAddRows(mat3, temp, NULL, NULL, NULL,
				      (killrow - temp + 1));
		}
		XbaeMatrixSetCell(mat3, killrow, 0, "S");
		XbaeMatrixSetCell(mat3, killrow, 1, name);
		XbaeMatrixSetCellUserData(mat3, killrow, 1,
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
		XtVaSetValues(interval, XmNvalue, name, NULL);
	    }
	}
	fclose(privrc);
    }
} /* load_pgpstuff */

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
load_guistuff(Widget text, Widget grid)
{
    int		rownum = 0;
    char	*rcval, *temp, *ent;

    rcval = find_mailrc("folder");
    if(rcval)
	XtVaSetValues(text, XmNvalue, rcval, NULL);

    rcval = find_mailrc("filemenu2");
    if(rcval){
	temp = strdup(rcval);
	ent = strtok(temp, " \n\r\t");
	do{
	    XbaeMatrixSetCell(grid, rownum, 0, ent);
	    rownum++;
	}while((ent = strtok(NULL, " \n\r\t")));
	free(temp);
    }
} /* load_guistuff */

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

/* $Id$ 
 *
 * Copyright   : (c) 1997 by Glenn Trigg.  All Rights Reserved
 * Project     : Privtool - motif interface
 * File        : m_util
 *
 * Author      : Glenn Trigg
 * Created     : 24 Feb 1997 
 *
 * Description : 
 */

#ifdef HAVE_CONFIG_H
#include	"config.h"
#endif

#include	<stdio.h>
#include	<X11/Intrinsic.h>
#include	<X11/IntrinsicP.h>
#include	<X11/StringDefs.h>
#include	<X11/Shell.h>
#include	<X11/ShellP.h>
#include	<Xm/Xm.h>
#include	<Xm/Container.h>
#include	<Xm/IconG.h>
#include	<Xbae/Caption.h>
#include	"m_util.h"
#include	"debug.h"
#include	"pixmapcache.h"

static Widget	captionLabel(Widget caption);
static char	*GenFullName(Widget widget, int size, char *(*name_func)());
static Boolean	cvtStringToPixmap(Display *, XrmValuePtr, Cardinal *,
				  XrmValuePtr, XrmValuePtr, XtPointer *);

#define	done(type, value)					\
	{							\
	    if (toVal->addr != NULL) {				\
		if (toVal->size < sizeof(type)) {		\
		    toVal->size = sizeof(type);			\
		    return False;				\
		}						\
		*(type*)(toVal->addr) = (value);		\
	    }							\
	    else {						\
		static type static_val;				\
		static_val = (value);				\
		toVal->addr = (XPointer)&static_val;		\
	    }							\
	    toVal->size = sizeof(type);				\
	    return True;					\
	}

/*----------------------------------------------------------------------*/

char *
FullName(Widget	widget)
{
    return GenFullName(widget, 0, XtName);
} /* FullName */

/*----------------------------------------------------------------------*/

String
ClassNameFromWClass(WidgetClass class)
{
    return class->core_class.class_name;
} /* ClassNameFromWClass */

/*----------------------------------------------------------------------*/

String
ClassName(Widget widget)
{
    if (XtIsApplicationShell(widget) && widget->core.parent == NULL){
	return ((ApplicationShellWidget)widget)->application.class;
    }
    else{
	return widget->core.widget_class->core_class.class_name;
    }
} /* ClassName */

/*----------------------------------------------------------------------*/

char *
FullClassName(Widget widget)
{
    return GenFullName(widget, 0, ClassName);
} /* FullClassName */

/*----------------------------------------------------------------------*/

static char *
GenFullName(Widget widget, int size, char *(*name_func)())
{
    char	*name, *buf;
    Boolean	nodot = (size == 0);

    if (widget){
	name = name_func(widget);

	size += strlen(name) + 1;

	buf = GenFullName(XtParent(widget), size, name_func);

	strcat(buf, name);

	if (!nodot)
	    strcat(buf, ".");
    }
    else{
	buf = XtMalloc(size);
	buf[0] = '\0';
    }
    return buf;
} /* GenFullName */

/*----------------------------------------------------------------------*/

char *
GetResourceString(Widget w, char *iname, char *iclass)
{
    char		name[BUFSIZ], class[BUFSIZ];
    XrmQuark		namelist[80], classlist[80];
    Screen		*screen = XtScreenOfObject(w);
    XrmRepresentation	typeret;
    XrmValue		valret;

    sprintf(name, "%s.%s", FullName(w), iname);
    sprintf(class, "%s.%s", FullClassName(w), iclass);
    XrmStringToQuarkList(name, namelist);
    XrmStringToQuarkList(class, classlist);
    XrmQGetResource(XtScreenDatabase(screen), namelist,
		   classlist, &typeret, &valret);

    return valret.addr;
}

/*----------------------------------------------------------------------*/

void
AddConverters(Widget toplevel)
{
    XtConvertArgRec	convertArg[5];

    DEBUG1(("AddConverters\n"));

    /* Initialization of IconGadget to force the Motif converter
       to be loaded before ours. */
    XtInitializeWidgetClass(xmIconGadgetClass);

    convertArg[0].address_mode = XtWidgetBaseOffset;
    convertArg[0].address_id = 0;
    convertArg[0].size = sizeof(Widget);

    XtSetTypeConverter(XtRString, XmRDynamicPixmap,
		       (XtTypeConverter)cvtStringToPixmap,
		       convertArg, 1, XtCacheByDisplay|XtCacheRefCount, NULL);
    XtSetTypeConverter(XtRString, XmRLargeIconPixmap,
		       (XtTypeConverter)cvtStringToPixmap,
		       convertArg, 1, XtCacheByDisplay|XtCacheRefCount, NULL);
    XtSetTypeConverter(XtRString, XmRSmallIconPixmap,
		       (XtTypeConverter)cvtStringToPixmap,
		       convertArg, 1, XtCacheByDisplay|XtCacheRefCount, NULL);
} /* AddConverters */

/*----------------------------------------------------------------------*/

static Boolean
cvtStringToPixmap(Display *display, XrmValuePtr args, Cardinal *num_args,
		  XrmValuePtr fromVal, XrmValuePtr toVal,
		  XtPointer *converter_data)
{
    char	*name = (char *)fromVal->addr;
    Widget	w;
    Pixel	fg, bg;
    Pixmap	pixmap;

    DEBUG1(("In cvtStringToPixmap (converting %s)\n", name));

    if(*num_args < 1){
	XtErrorMsg("wrongParameters","cvtStringToPixmap","XtToolkitError",
		   "String to Pixmap conversion needs 1 argument.",
		   (String *)NULL, (Cardinal *)NULL);

	return False;
    }

    w = (Widget)(args[0].addr);
    XtVaGetValues(w, XmNforeground, &fg, XmNbackground, &bg,
		  NULL);
    if((pixmap = get_cached_pixmap(w, name, NULL)) == 0){
	pixmap = XmGetPixmap(XtScreen(w), name, fg, bg);
    }
    done(Pixmap, pixmap);
} /* cvtStringToPixmap */

/*----------------------------------------------------------------------*/

void
alignCaptions(Widget parent)
{
    int			i, numkids;
    WidgetList		kids;
    Dimension		length = 0;
    XtWidgetGeometry	curr;

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

Widget
parentShell(Widget w)
{
    Widget	parent, current = w;

    while((parent = XtParent(current)) != NULL) {
	if( XtIsShell(parent) )
	    return parent;
	current = parent;
    }

    return NULL;
} /* parentShell */

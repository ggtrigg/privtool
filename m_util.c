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

#include	<stdio.h>
#include	<X11/Intrinsic.h>
#include	<X11/IntrinsicP.h>
#include	<X11/StringDefs.h>
#include	<X11/Shell.h>
#include	<X11/ShellP.h>
#include	"m_util.h"
#include	"debug.h"

static char	*GenFullName(Widget widget, int size, char *(*name_func)());

/*----------------------------------------------------------------------*/

char *
FullName(Widget	widget)
{
    return GenFullName(widget, 0, XtName);
}

/*----------------------------------------------------------------------*/

String
ClassNameFromWClass(WidgetClass class)
{
    return class->core_class.class_name;
}

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
}

/*----------------------------------------------------------------------*/

char *
FullClassName(Widget widget)
{
    return GenFullName(widget, 0, ClassName);
}

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
}

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

#if 1
void
AddConverters()
{
    DEBUG1(("AddConverters\n"));

    XtSetTypeConverter(XtRString, XtRPixmap,
		       (XtTypeConverter)cvtStringToPixmap,
		       NULL, 0, XtCacheByDisplay|XtCacheRefCount, NULL);
}

void
cvtStringToPixmap()
{
    fprintf(stderr, "In cvtStringToPixmap\n");
}
#endif

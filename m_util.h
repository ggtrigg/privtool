/* $Id$ 
 *
 * Copyright   : (c) 1997 by Glenn Trigg.  All Rights Reserved
 * Project     : Privtool - motif interface
 * File        : m_util
 *
 * Author      : Glenn Trigg
 * Created     : 24 Feb 1997 
 *
 * Description : Various utilities to make life easier.
 */

#ifndef _M_UTIL_H
#define _M_UTIL_H

char		*FullName(Widget);
char		*FullClassName(Widget);
char		*ClassName(Widget);
char		*ClassNameFromWClass(WidgetClass);
char		*GetResourceString(Widget w, char *name, char *class);

#endif /* _M_UTIL_H */

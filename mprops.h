/* $Id$ 
 *
 * Copyright   : (c) 1997 by Glenn Trigg.  All Rights Reserved
 * Project     : Privtool - Motif interface
 * File        : mprops
 *
 * Author      : Glenn Trigg
 * Created     : 27 May 1997 
 *
 * Description : Public function definitions from mprops.c
 */

#ifndef _MPROPS_H
#define _MPROPS_H

typedef struct _mail_header_field {
    String	name;
    Boolean	editable;
} MAIL_HEADER_FIELD;

void show_props(Widget);

#endif /* _MPROPS_H */

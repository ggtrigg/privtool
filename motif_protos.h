/* $Id$ 
 *
 * Copyright   : (c) 1996 by Glenn Trigg.  All Rights Reserved
 * Project     : privtool motif interface
 * File        : motif_protos
 *
 * Author      : Glenn Trigg
 * Created     : 30 Nov 1996 
 *
 * Description : 
 */

#ifndef _MOTIF_PROTOS_H
#define _MOTIF_PROTOS_H

void display_message_description(MESSAGE *m);
void display_new_message();
void set_main_footer(char *s);
void set_display_footer(DISPLAY_WINDOW *w, char *s);
void sync_list(void);
void deleteAllMessages(void);
int  alert(Widget, char *, int);
void add_to_combo(char *path);
void detachMW(Boolean);

#endif /* _MOTIF_PROTOS_H */

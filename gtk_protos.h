/* $Id$ 
 *
 * Copyright   : (c) 1999 by Glenn Trigg.  All Rights Reserved
 * Project     : Privtool - Gtk interface
 * File        : gtk_protos
 *
 * Author      : Glenn Trigg
 * Created     : 12 Feb 1999 
 *
 * Description : 
 */

#ifndef _GTK_PROTOS_H
#define _GTK_PROTOS_H

typedef enum {
    ALERT_NONE,
    ALERT_MESSAGE,
    ALERT_QUESTION,
    ALERT_ERROR
} ALERT_TYPE;

unsigned int alert(GtkWidget *, char *, ALERT_TYPE, unsigned int, ...);
void	clear_busy(void);
void	delete_message_proc();
void	display_message_description(MESSAGE *);
void	set_display_footer(DISPLAY_WINDOW *, char *);
void	show_busy(void);
void	show_props(void);
void	sync_list(void);
void	update_message_list(void);

#endif /* _GTK_PROTOS_H */

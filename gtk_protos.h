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

void	delete_message_proc();
void	display_message_description(MESSAGE *);
void	set_display_footer(DISPLAY_WINDOW *, char *);
void	sync_list(void);
void	update_message_list(void);

#endif /* _GTK_PROTOS_H */

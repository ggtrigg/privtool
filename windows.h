
/*
 *	$RCSfile$	$Revision$ 
 *	$Date$
 *
 *	(c) Copyright 1993-1996 by Mark Grant, and by other
 *	authors as appropriate. All right reserved.
 *
 *	The authors assume no liability for damages resulting from the 
 *	use of this software, even if the damage results from defects in
 *	this software. No warranty is expressed or implied.
 *
 *	This software is distributed under the GNU Public Licence, see
 *	the file COPYING for more details.
 *
 *			- Mark Grant (mark@unicorn.com) 29/6/94
 *
 */

/* This structure defines a compose window */

#ifndef _WINDOWS_H
#define _WINDOWS_H

#ifdef UI_MAIN


typedef struct _compose_window {

    struct _compose_window	*next, *prev;

#ifdef HAVE_XVIEW
    Frame	deliver_frame;
    Panel	deliver_panel;
    Textsw	deliver_body_window;

    Panel_item	include_item, deliver_item, header_item;
    Panel_item	clear_item, log_item, sign_item, encrypt_item;
    Panel_item	send_to_item, send_cc_item, send_subject_item;
    Panel_item	send_bcc_item;
    Panel_item	compose_extra_headerlines[MAX_EXTRA_HEADERLINES];

#elif defined(HAVE_MOTIF)
    Widget	deliver_frame, send_to, send_cc, send_subject, menu;
    Widget	send_bcc, text, sign, encrypt, log, raw, remail;
    Widget	extra_headers[MAX_EXTRA_HEADERLINES];

#elif defined(HAVE_GTK)
    GtkWidget	*deliver_frame, *send_to, *send_cc, *send_subject, *menu;
    GtkWidget	*send_bcc, *text, *sign, *encrypt, *log, *raw, *remail;
    GtkWidget	*extra_headers[MAX_EXTRA_HEADERLINES];
#endif

    int	deliver_flags;
    int	in_use;
	
} COMPOSE_WINDOW;

typedef struct _display_window {

    struct	_display_window	*next, *prev;

#ifdef HAVE_XVIEW
    Frame	display_frame;
    Panel	display_panel;

    Panel_item	addkey_item;
    Panel_item	sender_item;
    Panel_item	date_item;
    Panel_item	decode_item;
    Textsw	sig_window;
    Textsw	body_window;
#endif

    int	number;

} DISPLAY_WINDOW;

#else
typedef	int	COMPOSE_WINDOW;
typedef int	DISPLAY_WINDOW;
#endif

/* Declare function */

COMPOSE_WINDOW	*x_setup_send_window (void);
COMPOSE_WINDOW	*setup_send_window (void);
DISPLAY_WINDOW	*create_display_window (void);

#endif /* _WINDOWS_H */

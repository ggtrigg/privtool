
/*
 *	@(#)windows.h	1.2 8/10/95
 *
 *	(c) Copyright 1993-1995 by Mark Grant, and by other
 *	authors as appropriate. All right reserved.
 *
 *	The authors assume no liability for damages resulting from the 
 *	use of this software, even if the damage results from defects in
 *	this software. No warranty is expressed or implied.
 *
 *	This software is being distributed under the GNU Public Licence,
 *	see the file COPYING for more details.
 *
 *			- Mark Grant (mark@unicorn.com) 29/6/94
 *
 */

/* This structure defines a compose window */

#ifdef UI_MAIN
typedef struct _compose_window {

	struct _compose_window	*next, *prev;

	Frame	deliver_frame;
	Panel	deliver_panel;
	Textsw	deliver_body_window;

	Panel_item	include_item, deliver_item, header_item;
	Panel_item	clear_item, log_item, sign_item, encrypt_item;
	Panel_item	send_to_item, send_cc_item, send_subject_item;
	Panel_item	send_bcc_item;
	Panel_item	compose_extra_headerlines[MAX_EXTRA_HEADERLINES];

	int	deliver_flags;
	int	in_use;
	
} COMPOSE_WINDOW;
#else
typedef	int	COMPOSE_WINDOW;
#endif

/* Declare function */

COMPOSE_WINDOW	*x_setup_send_window ();
COMPOSE_WINDOW	*setup_send_window ();


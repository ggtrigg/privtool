
/*
 *	@(#)gui.h	1.21 9/25/95
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
 *	 Added layout_compact
 *		- Anders Baekgaard (baekgrd@ibm.net) 10th August 1995
 */

/* mail_reader routines */

extern	char	*current_mail_file ();
extern  char    *expand_filename ();

/* GUI procedure definitions */

extern	void	set_message_description();
extern	void	next_message_proc();
extern	void	prev_message_proc();
extern	void	print_cooked_proc ();
extern	void	print_raw_proc ();
extern	void	quit_proc();
extern	void	save_and_quit_proc ();
extern	int	load_file_proc ();
extern  void	load_new_mail();
extern	void	got_passphrase();
extern	void	abort_passphrase_proc();
extern	void	destroy_passphrase();
extern	void	set_pgppass();
extern	void	delete_message();
extern	void	setup_display();
extern	void	decrypt_with_passphrase();
extern	void	set_flags_from_decryption();
extern	void	get_passphrase();
extern	void	deliver_proc();
extern	void	display_message();
extern	void	move_message_proc();
extern	void	copy_message_proc();
extern	void	select_message_proc();
extern	void	check_for_new_mail();
extern	void	inbox_proc ();
extern	void	save_changes_proc ();
extern	void	done_proc ();
extern	void	undelete_last_proc ();
extern	void	undelete ();
extern	void	sort_by_time ();
extern	void	sort_by_number ();
extern	void	sort_by_subject ();
extern  void	sort_by_sender ();
extern	void	sort_by_size ();
extern	void	sort_by_status ();
extern	void	add_key_proc ();
#ifdef PGPTOOLS
extern	void	reseed_random_generator ();
#endif

/* GUI variable definitions */

extern	MESSAGE	*last_message_read;
extern	MESSAGE	*message_to_decrypt;
extern	MESSAGE	*displayed_message;

extern	MESSAGE_LIST	messages;
extern	MESSAGE_LIST	deleted;
extern	void	(*callback_proc)();
extern	char	*passphrase;
extern	int	layout_compact, show_deleted;

extern	char	prog_name[];
extern	char	prog_ver[];
extern	char	default_mail_file[];

/* Host-dependant UI functions */

extern	void	setup_ui();
extern	void	shutdown_ui();
extern	void	close_passphrase_window();
extern	char	*read_passphrase_string();
extern	void	clear_passphrase_string();
extern	char	*read_file_name();
extern	char	*read_subject ();
extern	char	*read_cc ();
extern	char	*read_bcc ();
extern	char	*read_extra_headerline ();

#define DELIVER_SIGN	0x01
#define DELIVER_ENCRYPT	0x02
#define DELIVER_LOG	0x04
#define DELIVER_RAW	0x08
#define DELIVER_REMAIL	0x10

#define PGP_OPTIONS	(DELIVER_SIGN|DELIVER_ENCRYPT)

#define	ERROR_DELIVERY	0
#define ERROR_READING	1



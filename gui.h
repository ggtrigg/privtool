
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

#ifndef _GUI_H
#define _GUI_H

/* mail_reader routines */

extern	char	*current_mail_file (void);
extern  char    *expand_filename (char *);

/* GUI procedure definitions */

extern	int	load_file_proc (char *);
extern	void	abort_passphrase_proc(void);
extern	void	add_key_proc (DISPLAY_WINDOW *, MESSAGE *);
extern	void	bad_key_notice_proc(void);
extern	void	check_for_new_mail(void);
extern	void	copy_message_proc(char *);
extern	void	decrypt_with_passphrase(COMPOSE_WINDOW *);
extern	void	delete_message(MESSAGE *);
extern	void	deliver_proc(COMPOSE_WINDOW *);
extern	void	display_message(MESSAGE *);
extern	void	done_proc (void);
extern	void	get_passphrase(char *);
extern	void	got_passphrase(void);
extern	void	inbox_proc (void);
extern	void	move_message_proc(char *);
extern	void	next_message_proc(void);
extern	void	prev_message_proc(void);
extern	void	print_cooked_proc (void);
extern	void	print_raw_proc (void);
extern	void	quit_proc(void);
extern	void	save_and_quit_proc (void);
extern	void	save_attachment_proc (DISPLAY_WINDOW *, MESSAGE *);
extern	void	save_changes_proc (void);
extern	void	select_message_proc(MESSAGE *);
extern	void	set_flags_from_decryption(MESSAGE *, int);
extern	void	set_message_description(MESSAGE *);
extern	void	set_pgppass(void);
extern	void	setup_display(int, char *, int, char **);
extern	void	sort_by_number (void);
extern	void	sort_by_size (void);
extern	void	sort_by_status (void);
extern	void	sort_by_subject (void);
extern	void	sort_by_time (void);
extern	void	undelete (MESSAGE *);
extern	void	undelete_last_proc (void);
extern  int	compose_windows_open(void);
extern  int	dont_quit_notice_proc(void);
extern  int	failed_save_notice_proc(void);
extern  int	read_only_notice_proc(void);
extern  void	beep_display_window(void);
extern  void	clear_display_window(DISPLAY_WINDOW *);
extern  void	clear_main_footer(void);
extern  void	close_all_windows(void);
extern  void	create_passphrase_window(void);
extern  void	deleteAllMessages(void);
extern  void	hide_header_frame(void);
extern  void	invalid_attachment_notice_proc(void);
extern  void	load_new_mail(void);
extern  void	lock_display_window(DISPLAY_WINDOW *);
extern  void	set_initial_scrollbar_position(void);
extern  void	show_newmail_icon(void);
extern  void	show_normal_icon(void);
extern  void	sort_by_sender (void);
extern  void	update_log_item(COMPOSE_WINDOW *, int);
extern  void	set_main_footer(char *);
extern  void	clear_display_footer(DISPLAY_WINDOW *);
extern  void	display_sender_info(MESSAGE *, DISPLAY_WINDOW *);
extern  void	show_busy(void);
extern  void	clear_busy(void);
extern  void	set_display_footer(DISPLAY_WINDOW *, char *);
extern  int	bad_pass_phrase_notice(int);
extern  void	bad_file_notice(int);
extern  void	update_message_list(void);
extern  void	display_message_description(MESSAGE *);
extern  void	display_message_body(BUFFER *, DISPLAY_WINDOW *);
extern  void	display_message_sig(BUFFER *, DISPLAY_WINDOW *);
extern  void	show_addkey(DISPLAY_WINDOW *);
extern  int	buffer_contains_attachment (BUFFER *);
extern  void	show_attach(DISPLAY_WINDOW *);
extern  int	mime_decode_printable(BUFFER *, BUFFER *);
extern  void	show_display_window(MESSAGE *, DISPLAY_WINDOW *);
extern  void	open_passphrase_window(char *);
extern  void	sync_list(void);
extern  int	read_deliver_flags(COMPOSE_WINDOW *);
extern  void	read_message_to_deliver(COMPOSE_WINDOW *, BUFFER *);
extern  int	no_key_notice_proc(int);
extern  int	no_sec_notice_proc(int);
extern  char	*read_recipient(COMPOSE_WINDOW *);
extern  void	close_deliver_window(COMPOSE_WINDOW *);
extern  void	delete_message_proc(void);
extern  void	clear_display_footer(DISPLAY_WINDOW *);
extern  void	set_reply(MESSAGE *);
#ifdef PGPTOOLS
extern	void	reseed_random_generator (void);
#endif

/* GUI variable definitions */

extern	MESSAGE	*last_message_read;
extern	MESSAGE	*message_to_decrypt;
extern	MESSAGE	*displayed_message;

extern	MESSAGE_LIST	messages;
extern	MESSAGE_LIST	deleted;
extern	void	(*callback_proc)(COMPOSE_WINDOW *);
extern	char	*passphrase;
extern	int	layout_compact, show_deleted;

extern	char	prog_name[];
extern	char	prog_ver[];
extern	char	default_mail_file[];

/* Host-dependant UI functions */

extern	void	setup_ui(int, int, char **);
extern	void	shutdown_ui(void);
extern	void	close_passphrase_window(void);
extern	char	*read_passphrase_string(void);
extern	void	clear_passphrase_string(void);
extern	char	*read_file_name(COMPOSE_WINDOW *);
extern	char	*read_subject (COMPOSE_WINDOW *);
extern	char	*read_cc (COMPOSE_WINDOW *);
extern	char	*read_bcc (COMPOSE_WINDOW *);
extern	char	*read_extra_headerline (COMPOSE_WINDOW *, int);

#define DELIVER_SIGN	0x01
#define DELIVER_ENCRYPT	0x02
#define DELIVER_LOG	0x04
#define DELIVER_RAW	0x08
#define DELIVER_REMAIL	0x10

#define PGP_OPTIONS	(DELIVER_SIGN|DELIVER_ENCRYPT)

#define	ERROR_DELIVERY	0
#define ERROR_READING	1

#endif /* _GUI_H */

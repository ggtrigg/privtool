
/*
 *	$RCSfile$	$Revision$ 
 *	$Date$
 *
 *	Gui.c : This file well contain most of the user-interface code, 
 *	allowing the compilatio of privtool on different operating systems 
 *	and with UI toolkits by 'simply' replacing the lowest level of 
 *	functionality.
 *
 *	(c) Copyright 1993-1996 by Mark Grant, and by other
 *	authors as appropriate. All rights reserved.
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
 *	Linux compatibility changes by 
 *		- David Summers (david@actsn.fay.ar.us) 6th July 1995
 *
 *	Various changes 
 *		- Anders Baekgaard (baekgrd@ibm.net) 10th August 1995
 *
 *	Make subjects line up in message list
 *		- Richard Huveneers (richard@hekkihek.hacom.nl) 
 *			5th September 1995
 *
 *	Fix printing
 *		- Anthony B Gialluca (tony@hgc.edu) 11th Oct 1995
 *
 *      Fix function: expand_filename()
 *              - Scott Cannon Jr. (scottjr@silver.cal.sdl.usu.edu)
 *                30 May 1996
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef SYSV
#include <unistd.h>
#endif

#include "def.h"
#include "buffers.h"
#include "message.h"
#include "mailrc.h"
#include "windows.h"
#include "gui.h"

/* Maximum number of remailers to use */

#define MAXMIX	6

/* Local variables */

static	int	security;
static	time_t	last_displayed = 0;

static	char	*mail_args[] =

{
#if defined(__FreeBSD__) || defined(linux)
	"/usr/sbin/sendmail",
#else
	"/usr/lib/sendmail",
#endif
	"-t",
	NULL
};

static	char	*uudecode_args[] =

{
	"/usr/bin/uudecode",
	NULL,
};

static	char	*print_args[] =

{
#ifdef linux
	"/usr/bin/lpr",
#else
#ifdef __FreeBSD__
	"/usr/bin/lpr",
#else
	"/usr/ucb/lpr",
#endif
#endif
	NULL
};

#ifndef NO_MIXMASTER
static	char	*remail_args[32] =
{
	MIXEXEC,
	"-c",
#ifdef MIXMASTER_STDIN
	"stdin",
#else
	NULL,
#endif
	"-f",
	"-to"
};
#endif

/* Titles for passphrase windows */

static	char	sign_passphrase[]="Passphrase entry (Signing)";
static	char	decrypt_passphrase[]="Passphrase entry (Decryption)";

/* Global variables */

char	*passphrase;
int	show_deleted = 0;
void	(*callback_proc)();
COMPOSE_WINDOW	*callback_win;

extern	char	*our_userid;

#ifdef PGPTOOLS
static	byte	md5_pass[16];
#endif

MESSAGE	*last_message_read;
MESSAGE	*message_to_decrypt;

#ifdef COMPACT
int	layout_compact = TRUE;
#else
int	layout_compact = FALSE;
#endif

/* Program name */

#ifdef XSAFEMAIL
char	prog_name[] = "XSafeMail";
#else
char	prog_name[] = "Privtool";
#endif

char	prog_ver [] = "V0.88 BETA";

/* Set the description of the message for the message list */

void	set_message_description(m)

MESSAGE	*m;

{
	char	mess_desc[256];
	char	mess_size[32];
	char	mess_type[8];

	/* Default to spaces */

	strcpy(mess_type,"   ");

	/* Set new/unread status */

	if (m->status == MSTAT_NONE) {
		mess_type[0] = 'N';
	}
	else if (m->status == MSTAT_UNREAD) {
		mess_type[0] = 'U';
	}

	/* Check signature status */

	if (m->flags & MESS_SIGNED) {
		if (!(m->flags & MESS_VERIFIED)) {
			mess_type [1] = 's';
		}
		else if (m->flags & MESS_BAD) {
			mess_type [1] = 'X';
		}
		else {
			mess_type [1] = 'S';
		}
	}

	/* Check encryption status */

	if (m->flags & MESS_ENCRYPTED) {
		if (!(m->flags & (MESS_SIGNED|MESS_VERIFIED)))
			mess_type[1] = '?';
		if (m->flags & MESS_VERIFIED) 
			mess_type[2] = 'D';
		else
			mess_type[2] = 'E';
	}

	/* Create message size string */

	if (layout_compact) {
		sprintf(mess_desc,
#ifdef MOTIF
			"%3.3s %4d %-20.20s %17.16s %4d %s",
#else
			"%3.3s %4d %-20.20s %17.16s %4d %-64.64s",
#endif
			mess_type,m->number,m->email,m->date,
			m->lines,m->subject ? m->subject : "");
	}
	else {
		sprintf(mess_size,"%4d/%-6d",m->lines,m->size);

		if (m->subject) {
			sprintf(mess_desc,"%3.3s %-5d %-30.30s %20.20s  %s  %-64.64s",
				mess_type,m->number,m->email,m->date,
					mess_size,m->subject);
		}
		else {
			sprintf(mess_desc,"%3.3s %-5d %-30.30s %20.20s  %s ",
				mess_type,m->number,m->email,m->date,mess_size);
		}
	}

	/* Clear the old description if possible */

	if (m->description)
		free (m->description);

	/* And create the new description */

	m->description = (char *)strdup(mess_desc);
}

/* Expand a string into a buffer of email addresses, taking aliases
   into account */

#define COUNT_MAX	10

void	expand_send(b,s)

BUFFER	*b;
char	*s;

{
	char	*p,*a;
	char	address[512];
	static	int	count = 0;

	/* Just in case we get an alias loop, abort ! */

	if (count >= COUNT_MAX) {
		fprintf (stderr, "Privtool: Probable alias loop detected !\n");
		return;
	}

	/* Increment entry count */

	count++;
	p = s;

	while (*p) {

		/* Addresses can be space or comma seperated */

		a = address;
		while (*p == ' ' || *p == ',')
			p++;
		while (*p != ' ' && *p != ',' && *p) {
			*a++ = *p++;
		}

		*a = 0;
		a = find_alias (address);

		/* If we got an alias, expand it */

		if (a) {
			/* We have to duplicate the string as it will get
			   trashed lower down */

			a = strdup (a);
			expand_send (b, a);
			free_string (a);
		}
		else {
			if (b->length)
				add_to_buffer (b, " ", 1);
			add_to_buffer (b, address, strlen(address));
		}
	}

	/* Decrement entry count */

	count--;
}

static	char	**split_string (s, c)

char	*s;
int	*c;

{
	int	count = 0;
	int	i, j, k, l;
	char	**list;

	l = strlen (s);
	if (l)
		count++;

	for (i = 0; i < l; i++) {
		if (s [i] == ' ')
			count++;
	}

	*c = count;

	if (!count)
		return NULL;

	list = (char **) malloc ((count + 1) * sizeof (char *));

	j = 0;
	for (i = 0; i < count; i++) {
		for (k = j; k < l && s[k] != ' '; k++);
		s [k] = 0;
		list [i] = (char *)s + j;
		j = k + 1;
	}

	list [i] = NULL;

	return list;
}

static	char	**split_list (b, c)

BUFFER	*b;
int	*c;

{
	return split_string (b->message, c);
}

/* Display selected message */

void	display_message(m)

MESSAGE	*m;

{
	int	i;
	DISPLAY_WINDOW	*w;

	if(m == NULL)
	    return;

	/* Add time to random seed */

	update_random();
	time (&last_displayed);

	hide_header_frame();

	/* Set the icon back in case it was 'new mail' */

	show_normal_icon ();
	w = create_display_window(m);

	/* Let user know we're doing something */

	set_main_footer("Displaying Message...");
	clear_display_footer (w);

	/* Display sender and date */

	display_sender_info(m,w);
	clear_display_window(w);

	if (m->flags & MESS_DIGEST) {

	/* Put special Digest handling here */

	    m->flags &= ~(MESS_SIGNED|MESS_ENCRYPTED);

	}

	/* Process encrypted/signed messages through PGP */

	if (m->flags & (MESS_SIGNED|MESS_ENCRYPTED)) {
		if (!(m->flags & MESS_VERIFIED)) {

			show_busy ();

			/* Either create a new buffer or clear the old one */

			if (!m->decrypted)
				m->decrypted = new_buffer();
			else
				clear_buffer(m->decrypted);

			/* Ditto for signature info */

			if (!m->signature)
				m->signature = new_buffer();
			else
				clear_buffer(m->signature);

			/* If it's encrypted, we need a passphrase */

			if (m->flags & MESS_ENCRYPTED) {
				if (!passphrase) {
					message_to_decrypt = m;
					callback_proc = decrypt_with_passphrase;
					callback_win = NULL;
					clear_busy ();
					get_passphrase(decrypt_passphrase);
					clear_display_footer(w);
					clear_main_footer ();
					return;
				}
			}

			/* Keep the user informed */

			if (m->flags & MESS_ENCRYPTED)
				set_display_footer(w,"Decrypting message...");
			else
				set_display_footer(w,"Verifying signature...");

			/* Call PGP to decrypt/verify */

			i = decrypt_message(message_contents(m),
				m->decrypted,
				m->signature,passphrase,FL_ASCII,
#ifdef PGPTOOLS
				md5_pass
#else
				NULL
#endif
				);

			/* Destroy the passphrase if we got one */

			if (m->flags & MESS_ENCRYPTED) {
				destroy_passphrase(FALSE);
			}

			/* Uh-oh, bad passphrase ! */

			if (i == DEC_BAD_PHRASE) {
				destroy_passphrase(TRUE);
				clear_busy ();
				if (bad_pass_phrase_notice(ERROR_READING)) {
					callback_proc = decrypt_with_passphrase;
					callback_win = NULL;
					message_to_decrypt = m;
					get_passphrase(decrypt_passphrase);
				}
				clear_display_footer(w);
				clear_main_footer ();
				return;
			}

			if (i == DEC_BAD_FILE) {
				bad_file_notice (ERROR_READING);
				m->flags &= ~(MESS_ENCRYPTED|MESS_SIGNED);
				clear_busy ();
				goto display_plaintext;
			}

			if (i == SIG_BAD) {
				if (!find_mailrc("nobeepbadsig"))
					beep_display_window ();
			}

			/* Process the error and set flags */

			set_flags_from_decryption(m,i);
		}

		/* Handle encrypted mailing list feeds */

		if ((m->flags & MESS_CFEED) &&
			is_mail_message(m->decrypted)) {
			MESSAGE	*newm;

			/* Keep the user informed */

			set_display_footer(w,"Found encrypted mailfeed...");

			/* Replace the old message with the decrypted one */

			newm = message_from_message(m);
			replace_message_with_message(m,newm);
			last_message_read = newm;

			if (!(newm->flags & MESS_ENCRYPTED))
				messages.encrypted--;

			newm->status = MSTAT_READ;
			if (m->status == MSTAT_NONE) {
				messages.new--;
				update_message_list();
			}
			if (m->status == MSTAT_UNREAD) {
				messages.unread--;
				update_message_list();
			}

			/* Update message list */

			newm -> flags |= MESS_SELECTED;
			set_message_description(newm);
			display_message_description(newm);

			/* Then go back round the loop */

			free_message(m);
			display_message(newm);

			clear_busy ();
			return;
		}

		/* Otherwise, just display the decrypted message */

		display_message_body(m->decrypted,w);
		display_message_sig(m->signature,w);

		if (buffer_contains_key (m->decrypted))
			show_addkey (w);
		if (m->attachment_type = 
			buffer_contains_attachment (m->decrypted))
			show_attach (w);

		clear_busy ();
	}
	else {

	display_plaintext:
		/* Or display the plaintext message */

		display_message_body(message_contents (m),w);

		if (buffer_contains_key (message_contents (m)))
			show_addkey (w);
		if (m->attachment_type = 
			buffer_contains_attachment (message_contents(m)))
			show_attach (w);
	}

	/* Update status to say we read it */

	if (m->status != MSTAT_READ) {
		if (m->status == MSTAT_NONE) {
			messages.new--;
			update_message_list ();
		}
		if (m->status == MSTAT_UNREAD) {
			messages.unread--;
			update_message_list ();
		}
		m->status = MSTAT_READ;
	}

	/* Lock the display window to prevent edits, and show it */

	lock_display_window(w);
	show_display_window(m,w);

	/* Just in case anything's changed */

	set_message_description(m);
	display_message_description(m);

	/* Destroy passphrase and clear window, just in case */

	destroy_passphrase(FALSE);

	/* Clear the footer now it's displayed */

#ifndef MOTIF
	clear_display_footer(w);
#endif
	clear_main_footer ();
}

/* Selecting next or previous message */

void	select_message_proc(m)

MESSAGE	*m;

{
	if (last_message_read && m) {
		last_message_read->flags &= ~MESS_SELECTED;
		display_message_description(last_message_read);

		last_message_read = m;
		last_message_read->flags |= MESS_SELECTED;
		display_message(last_message_read);

		/* Yes, we do this twice for a reason - last_message_read
		   may have changed on the way through display_message */

		if (last_message_read) {
			last_message_read->flags |= MESS_SELECTED;
			set_message_description(last_message_read);
			display_message_description(last_message_read);
		}
	}
}

void	prev_message_proc()

{
	update_random();

	if (last_message_read) {
		MESSAGE	*m;

		m = last_message_read->prev;
		while (m && (m->flags & MESS_DELETED))
			m = m->prev;

		if (m)
			select_message_proc (m);
	}
}

void	next_message_proc()

{
	MESSAGE	*m;

	update_random();

	if (last_message_read) {
		m = last_message_read->next;
		while (m && (m->flags & MESS_DELETED))
			m = m->next;
		if (m)
			select_message_proc (m);
	}
}

void	quit_proc()

{
	MESSAGE	*m, *om;

	if (compose_windows_open()) {
		if (dont_quit_notice_proc())
			return;
	}

	close_mail_file ();
	update_random();

	/* First free the message list, so that everything gets erased */

	m = messages.start;

	while (m) {
		om = m;
		m = m->next;
		free_message (om);

		messages.number--;
	}

	/* Destroy the passphrase, just in case */

	destroy_passphrase(TRUE);

	shutdown_ui();
	close_messages ();
	close_pgplib();

	exit(1);
}

/* Save Changes and quit */

void	save_and_quit_proc ()

{
	set_main_footer ("Saving changes and quitting...");

	/* Save any changes to the file */

	if (save_changes () < 0) {
		if (failed_save_notice_proc ()) {
			clear_main_footer ();
			return;
		}
	}

	quit_proc ();
}

static	void	wipe_passphrase()

{

	/* If we don't have a passphrase, get the value of the
    	   passphrase item in case it was partially entered
   	   then aborted */

	if (!passphrase) {
		passphrase = read_passphrase_string();
	}

	/* Now destroy the passphrase in the panel item */

	if (passphrase) {
		while (*passphrase) {
			*passphrase = 0;
			passphrase++;
		}
	}

	/* Then set the value of the item to nothing, just in case */

	clear_passphrase_string();
}

void	got_passphrase()

{
	void	(*call_proc)();

	update_random();

	passphrase = read_passphrase_string();
	close_passphrase_window();

#ifdef PGPTOOLS

	/* If we have PGP Tools, we only store the MD5 */

	get_md5 (passphrase, md5_pass);

	/* And immediately wipe the passphrase. We keep the pointer
	   around until the passphrase is officially cleared */

	wipe_passphrase ();
#endif

	if (callback_proc) {
		call_proc = callback_proc;
		callback_proc = NULL;

		/* Have to do it this way as callback_proc may reset
		   the value in callback_proc on error */

		(*call_proc)(callback_win);
	}
}

/* Destroy_passphrase() is used to carefully delete all possible
   copies of the user's passphrase */

void	destroy_passphrase(force)

int	force;

{
	if (security_level() > 2 || force) {

		/* First wipe the passphrase string */

		wipe_passphrase();

#ifdef PGPTOOLS
		/* Now clear the MD5 */
		
		bzero (md5_pass, sizeof (md5_pass));
#endif
		
		/* Delete the pointer */

		passphrase = NULL;
	}

	/* And close the passphrase window if it was open */

	close_passphrase_window();
}

void	abort_passphrase_proc()

{
	update_random();

	callback_proc = NULL;
	callback_win = NULL;

	destroy_passphrase(FALSE);
	close_passphrase_window();
}

void	get_passphrase(s)

char	*s;

{
	update_random();

	create_passphrase_window();
	open_passphrase_window(s);
	clear_passphrase_string();

	passphrase = NULL;
}

void	delete_message(om)

MESSAGE	*om;

{
	/* Clear last_message_read if it's deleted */

	if (om == last_message_read) {
		MESSAGE	*m = NULL;

		if (om->next) {
			m = om->next;
			while (m && (m->flags & MESS_DELETED))
				m = m->next;
		}

		if (!m) {
			m = om->prev;
			while (m && (m->flags & MESS_DELETED))
				m = m->prev;
		}

		last_message_read = m;
	}

	/* Clear selected flag */
	if(!show_deleted){
	    om->flags &= ~MESS_SELECTED;
	    om->list_pos = (-1);
	}

	/* Update message info */

	if (om->flags & MESS_ENCRYPTED)
		messages.encrypted--;

	if (om->status == MSTAT_NONE)
		messages.new--;
	if (om->status == MSTAT_UNREAD)
		messages.unread--;

	/* And add it to the deleted list */

	add_to_deleted (om);

	messages.number --;
}

/* Return the security level */

int	security_level()

{
	return security;
}

/* Setup the display */

void	setup_display(level,phrase,argc,argv)

int	level;
char	*phrase;
int	argc;
char	**argv;

{
	security = level;
	if (security < 2)
		passphrase = phrase;

	setup_ui(level,argc,argv);
}

void	decrypt_with_passphrase()

{
	display_message(message_to_decrypt);
	sync_list();
}

void	set_flags_from_decryption(m,i)

MESSAGE	*m;
int	i;

{
	m->flags |= MESS_VERIFIED;

	switch(i) {

		/* Uh-oh, couldn't find the key */

		case SIG_NO_KEY:
		case DEC_NO_KEY:
		m->flags &= ~MESS_VERIFIED;
		break;

		/* 
		   Whoops, no signature, boys ! 
		*/

		case SIG_NONE:
		m->flags &= ~MESS_SIGNED;
		break;

		/* Uh-oh, Bad signature */

		case SIG_BAD:
		m->flags |= MESS_BAD;
		break;

		/* 
		    That's more like it ! 
		    In case it was a signed, encrypted message, set the
		    signed bit.
		*/

		case SIG_GOOD:
		m->flags |= MESS_SIGNED;
		break;
	}
}

static	char	*replying_message_id;
static	char	*replying_message_sender;

COMPOSE_WINDOW	*setup_send_window()

{
	if (replying_message_id) {
		free_string (replying_message_id);
		replying_message_id = NULL;
	}

	if (replying_message_sender) {
		free_string (replying_message_sender);
		replying_message_sender = NULL;
	}

	return x_setup_send_window ();
}

/* Set up reply variables from the message we're replying to */

void	set_reply (m)

MESSAGE	*m;

{
	if (replying_message_id) {
		free_string (replying_message_id);
		replying_message_id = NULL;
	}

	if (replying_message_sender) {
		free_string (replying_message_sender);
		replying_message_sender = NULL;
	}

	if (m) {
		if (m->message_id)
			replying_message_id = strdup (m->message_id);
		if (m->sender)
			replying_message_sender = strdup (m->sender);
	}
}

static	int	remove_duplicates (list)

char	**list;

{
	int	i, j;

	i = 0;
	while (list [i])
		i++;

	if (!i)
		return 0;

	/* Sort the list */

#ifndef linux
	qsort (list, i, sizeof (char *), strcmp);
#else
	qsort (list, i, sizeof (char *), (__compar_fn_t) strcmp);
#endif

	i = 1;
	j = 1;

	/* Loop through the sorted list */

	while (list [i]) {
		/* If they're different, move them */

		if (strcmp (list [i], list [i-1])) {
			list [j++] = list [i];
		}

		/* Otherwise, move on */

		i++;
	}

	list [j] = 0;

	return j;
}

static	add_list_to_header (b, l)

BUFFER	*b;
char	**l;

{
	int	i;
	int	w, s;

	w = 16;
	for (i = 0; l[i]; i++) {
		s = strlen (l[i]) + 1;
		if (w+s > 75) {
			add_to_buffer (b, "\n    ", 5);
			w = 5;
		}
		add_to_buffer (b, l[i], s-1);
		if (l[i+1])
			add_to_buffer (b, ",", 1);
	}
	add_to_buffer (b, "\n", 1);
}

/* Support folder specification */

char	*expand_filename (s)

char	*s;

{
	char	*folder;
	char	*home;
	static	int	filename_size = 0;
	static	char	*filename = NULL ;
	int	sz;

	folder = find_mailrc("folder");
	home = getenv ("HOME");

	if (s && *s) {

		/* If folder specified, we need to build up
		   the full pathname */

		if (folder && *folder && 
#ifndef DONT_REQUIRE_PLUS
			*s == '+'
#else
			TRUE
#endif
			) {


			/* We'll be kind to malloc here */

			sz = (strlen (s) + 
				strlen (folder) + 4 
				+ QUANTA);

			if (*folder != '/') {
				sz += strlen (home) + 1;
			}

			sz /= QUANTA;
			sz *= QUANTA;

			if (!filename) {
				filename = (char *)malloc (sz);
				filename_size = sz;
			}
			else if (sz > filename_size) {
				filename = realloc (filename, sz);
				filename_size = sz;
			}

			if (*folder == '/')
#ifdef DONT_REQUIRE_PLUS
				sprintf(filename, "%s/%s", folder, s);
#else
			  {
			    if (*s == '/')
			      strcpy(filename, s);
			    else
			      sprintf(filename, "%s/%s", folder, s);
			  }
#endif
			else
				sprintf (filename, "%s/%s/%s",
#ifdef DONT_REQUIRE_PLUS
					home, folder, s);
#else
					home, folder, s+1);
#endif

			s = filename;
		}
	}

	return s;
}

#ifndef NO_MIXMASTER
static	setup_remail_args(addrs, subject, temp)

char	*addrs, *subject;
char	*temp;

{
	int	i, j;
	int	l;
	int	c;
	FILE	*mix_fp;
	char	mix_path[1024];
	static	char	n[MAXMIX][5];
	char	used[100];

	i = 5;

#ifndef MIXMASTER_STDIN
	remail_args[2] = temp;
#endif

	/* Pass subject if neccesary */

	remail_args[i++] = addrs;

	remail_args[i++] = "-s";
	if (subject && *subject) 
		remail_args[i++] = subject;
	else
		remail_args[i++] = "None";

	/* Open type2.list */

	sprintf(mix_path, "%s/type2.list", MIXPATH);

	mix_fp = fopen(mix_path, "rt");
	if (!mix_fp)
		return FALSE;

	/* Count number of lines in type2.list */

	l = 0;
	while (!feof(mix_fp)) {
		c = fgetc(mix_fp);
		if (c == '\n')
			l++;
	}

	fclose (mix_fp);

	/* Ok, we now know how many remailers are available. We only
	   select from first 100 */

	if (!l)
		return FALSE;

	if (l > 100)
		l = 100;

	for (c = 0; c < l; c++)
	 	used[c] = FALSE;

	c = l;
	if (c > MAXMIX)
		c = MAXMIX;

	remail_args[i++] = "-l";

	/* This isn't absolutely secure, but it's not *that* important */

	do {
		do {
#ifdef PGPTOOLS
			j = our_randombyte() * 256 + our_randombyte();
#else
			j = random();
#endif
			j %= l;
		} while (used[j]);

		used[j] = TRUE;
		sprintf(n[c], "%d", j + 1);

		remail_args[i++] = n[c];
	} while (--c);

	return TRUE;
}
#endif

/* Deliver the message */

void	deliver_proc(w)

COMPOSE_WINDOW	*w;

{
	char	recipient[512];
	char	subject[256];
	char	cc[256];
	char	bcc[256];
	char	buff[256];
	BUFFER	*mail_message;
	BUFFER	*raw_message = NULL;
	BUFFER	*log_message;
	int	ret_val;
	char	**userid = NULL;
	char	**addrs = NULL;
	char	**cc_addrs = NULL;
	char	**bcc_addrs = NULL;
	int	id_count = 0;
	int	cc_count = 0;
	int	bcc_count = 0;
	char	*uid,*alias;
	char	*key_name;
	int	encrypt_flags;
	int	i, j;
	BUFFER	*full_list;
	BUFFER	*cc_list;
	BUFFER	*bcc_list;
	char	*nym;
	int	deliver_flags;

	update_random();
	show_busy ();

	copy_to_nl(read_recipient(w),recipient);
	copy_to_nl(read_subject(w),subject);
	copy_to_nl(read_cc(w),cc);
	copy_to_nl(read_bcc(w),bcc);

	deliver_flags = read_deliver_flags (w);

	full_list = new_buffer ();
	cc_list = new_buffer ();
	bcc_list = new_buffer ();

	expand_send (full_list, recipient);
	if (*cc)
		expand_send (cc_list, cc);
	if (*bcc)
		expand_send (bcc_list, bcc);

	if (!full_list->length) {
		free_buffer (full_list);
		free_buffer (cc_list);
		free_buffer (bcc_list);
		clear_busy ();
		return;
	}

	addrs = split_list (full_list, &id_count);
	if (*cc)
		cc_addrs = split_list (cc_list, &cc_count);
	if (*bcc)
		bcc_addrs = split_list (bcc_list, &bcc_count);

	if (!id_count) {
		free_buffer (full_list);
		free_buffer (cc_list);
		free_buffer (bcc_list);

		if (addrs)
			free (addrs);
		if (cc_addrs)
			free (cc_addrs);
		if (bcc_addrs)
			free (bcc_addrs);

		clear_busy ();
		return;
	}

	id_count = remove_duplicates (addrs) + 2;
	if (cc_addrs)
		cc_count = remove_duplicates (cc_addrs);
	if (bcc_addrs)
		bcc_count = remove_duplicates (bcc_addrs);

	if (*recipient) {
		BUFFER	*to_send;

		to_send = new_buffer();
		read_message_to_deliver(w, to_send);

try_again:
		/* Check for alias */

		if (deliver_flags & PGP_OPTIONS) {
			BUFFER	*encrypted;
			char	*s;

			encrypt_flags = FL_ASCII;
			if (deliver_flags & DELIVER_SIGN)
				encrypt_flags |= FL_SIGN;
			if (deliver_flags & DELIVER_ENCRYPT)
				encrypt_flags |= FL_ENCRYPT;

			if (deliver_flags & DELIVER_SIGN) {
				if (!passphrase) {
					free_buffer(to_send);
					if (addrs)
						free (addrs);
					if (cc_addrs)
						free (cc_addrs);
					if (bcc_addrs)
						free (bcc_addrs);
					free_buffer (full_list);
					free_buffer (cc_list);
					free_buffer (bcc_list);
					callback_proc = deliver_proc;
					callback_win = w;
					clear_busy ();
					get_passphrase(sign_passphrase);
					return;
				}
			}

			/* Allow for .mailrc conversion of key ids */

			userid = (char **)malloc ((id_count+cc_count +
				bcc_count) * sizeof(char *));

			i = 0;
			for (j = 0; j < id_count && addrs [j]; j++) {
				alias = find_pgpkey (addrs [j]);
				if (alias) 
					userid [i++] = alias;
				else
					userid [i++] = addrs [j];
			}

			for (j = 0; j < cc_count && cc_addrs [j]; j++) {
				alias = find_pgpkey (cc_addrs [j]);
				if (alias) 
					userid [i++] = alias;
				else
					userid [i++] = cc_addrs [j];
			}

			for (j = 0; j < bcc_count && bcc_addrs [j]; j++) {
				alias = find_pgpkey (bcc_addrs [j]);
				if (alias) 
					userid [i++] = alias;
				else
					userid [i++] = bcc_addrs [j];
			}

			userid [i] = NULL;

			/* Get our key name for the routine */

			key_name = nym = current_nym ();

			if (s = find_pgpkey (key_name))
				key_name = s;

			/* If cooked logging, encrypt for us too */

			if ((deliver_flags & DELIVER_LOG) &&
				(deliver_flags & DELIVER_ENCRYPT) &&
				!(deliver_flags & DELIVER_RAW)) {
				userid[i++] = key_name;
			}

			userid [i] = 0;

			/* Strip duplicates from list */

			(void) remove_duplicates (userid);

			encrypted = new_buffer();
			ret_val = encrypt_message(userid,to_send,
				encrypted,encrypt_flags,passphrase,
				key_name,
#ifdef PGPTOOLS
				md5_pass
#else
				NULL
#endif
				); 

			free (userid);

			if (deliver_flags & DELIVER_SIGN) {
				destroy_passphrase(FALSE);
			}

			if (ret_val == ERR_NO_KEY) {
				if (no_key_notice_proc(ERROR_DELIVERY)) {
					deliver_flags &= ~DELIVER_ENCRYPT;

					update_log_item(w, deliver_flags);
					goto try_again;
				}
				else {
				bad_encrypt_exit:
					free_buffer(encrypted);
					free_buffer(to_send);
					free_buffer (full_list);
					free_buffer (cc_list);
					free_buffer (bcc_list);
					if (addrs)
						free (addrs);
					if (cc_addrs)
						free (cc_addrs);
					if (bcc_addrs)
						free (bcc_addrs);
					clear_busy ();
					return;
				}	
			}

			if (ret_val == ERR_NO_SECRET_KEY) {
				if (no_sec_notice_proc(ERROR_DELIVERY)) {
					deliver_flags &= ~DELIVER_SIGN;

					update_log_item(w, deliver_flags);
					goto try_again;
				}
				else {
					goto bad_encrypt_exit;
				}	
			}

			if (ret_val == ERR_BAD_PHRASE) {
				free_buffer(encrypted);
				free_buffer(to_send);
				free_buffer(full_list);
				free_buffer (cc_list);
				free_buffer (bcc_list);
				if (addrs)
					free (addrs);
				if (cc_addrs)
					free (cc_addrs);
				if (bcc_addrs)
					free (bcc_addrs);
				destroy_passphrase(TRUE);
				clear_busy ();
				if (bad_pass_phrase_notice(ERROR_DELIVERY)) {
					callback_proc = deliver_proc;
					callback_win = w;
					get_passphrase(sign_passphrase);
				}
				return;
			}

			if ((deliver_flags & DELIVER_ENCRYPT) &&
				(deliver_flags & DELIVER_RAW)) {
				raw_message = to_send;
				sprintf(buff,
					"\n     [ Privtool Note : Real message was sent encrypted");
				if (deliver_flags & DELIVER_SIGN) {
					strcat(buff, " and signed");
				}
				strcat(buff, " ]\n\n");

				add_to_buffer(raw_message, buff,
					strlen(buff));
			}
			else
				free_buffer(to_send);

			to_send = encrypted;
		}

		mail_message = new_buffer();

		if (!(deliver_flags & DELIVER_REMAIL)) {
			char	*domain, *replyto, *organization;
#ifdef MAILER_LINE
			sprintf(buff, "X-Mailer: %s [%s] (%s/%s)\n",
				prog_name, prog_ver,
#if defined(linux)
				"Linux",
#elif defined(__FreeBSD__)
				"FreeBSD",
#elif defined(__sgi)
				"SGI",
#else
				"Unknown",
#endif
#ifdef MOTIF
				"Motif"
#else
				"XView"
#endif
				);
			add_to_buffer(mail_message,buff,strlen(buff));
#endif

			/* Add in-reply-to: to the header */

			if (replying_message_id &&
				replying_message_sender) {
				sprintf(buff,"In-Reply-To: %s from \"%s\"\n",
					replying_message_id,
					replying_message_sender);
				add_to_buffer (mail_message,buff,strlen(buff));
			}

			/* Allow the user to set the domain name */

			if ((domain = find_mailrc("domain")) && *domain) {
				sprintf (buff, "From: %s@%s\n", our_userid, 
					domain);
				add_to_buffer (mail_message, buff,
					strlen (buff));
			}

			/* Allow the user to specify an organization */

			if ((organization = find_mailrc("organization")) &&
				*organization) {
				sprintf (buff, "Organization: %s\n",
					organization);
				add_to_buffer (mail_message, buff,
					strlen (buff));
			}

			/* Allow the user to set the reply-to line */

			if ((replyto = find_mailrc("replyto")) && 
				*replyto) {
				sprintf (buff, "Reply-To: %s\n", replyto); 
				add_to_buffer (mail_message, buff,
					strlen (buff));
			}

			/* Add the subject to the header */

			if (*subject) {
				sprintf(buff,"Subject: %s\n",subject);
				add_to_buffer(mail_message,buff,strlen(buff));
			}

			add_to_buffer (mail_message, "To: ", 4);
			add_list_to_header (mail_message, addrs);

			if (cc_addrs) {
				add_to_buffer (mail_message, "Cc: ", 4);
				add_list_to_header (mail_message, cc_addrs);
			}

			if (bcc_addrs) {
				add_to_buffer (mail_message, "Bcc: ", 5);
				add_list_to_header (mail_message, bcc_addrs);
			}
#if 0
			if (strcmp(nym, our_userid)) {
				add_to_buffer (mail_message, "From: ", 6);
				add_to_buffer (mail_message, nym, strlen(nym));
			}
#endif

			for (i = 1; i < MAX_EXTRA_HEADERLINES; i++) {
				char* line = read_extra_headerline(w,i);

				if (line) {
					add_to_buffer(mail_message, 
						line, strlen(line));
					free(line);
				}
			}

			add_to_buffer(mail_message,"\n",1);
		}

		add_to_buffer(mail_message,to_send->message,
			to_send->length);

		/* Do we remail or send direct ? */

#ifndef NO_MIXMASTER
		if (deliver_flags & DELIVER_REMAIL) {
			char	*addr;
#ifdef MIXMASTER_STDIN
			char	*temp = NULL;
#else
			char	temp[1024];
			FILE	*mix_fp;

			sprintf(temp, "%s/temp.mix", MIXPATH);
			mix_fp = fopen(temp, "wt");

			if (!mix_fp)
				goto remail_error_exit;

			fwrite (mail_message->message, 
				mail_message->length,
				1, mix_fp);

			fclose (mix_fp);
#endif

			i = 0;
			while (addrs[i]) {
				if (setup_remail_args(addrs[i], subject, temp)) {
					run_program(remail_args[0],
						mail_message->message,
#ifdef MIXMASTER_STDIN
						mail_message->length,
#else
						0,
#endif
						remail_args,
						NULL, NULL);
				}	
				else {

remail_error_exit:
					clear_busy ();
					remail_failed_notice_proc();
					if (raw_message)
						free_buffer (raw_message);

					free_buffer(to_send);
					free_buffer(mail_message);
					free_buffer (full_list);
					free_buffer (cc_list);
					free_buffer (bcc_list);
					if (addrs)
						free (addrs);
					if (cc_addrs)
						free (cc_addrs);
					if (bcc_addrs)
						free (bcc_addrs);

#ifndef MIXMASTER_STDIN
					unlink (temp);
#endif

					return;
				}

				i++;
			}

			i = 0;
			while (cc_addrs && cc_addrs[i]) {
				if (setup_remail_args(cc_addrs[i], subject))
					run_program(remail_args[0],
						mail_message->message,
						mail_message->length,
						remail_args,
						NULL, NULL);
				else 
					goto remail_error_exit;
				i++;
			}

			i = 0;
			while (bcc_addrs && bcc_addrs[i]) {
				if (setup_remail_args(bcc_addrs[i], subject))
					run_program(remail_args[0],
					mail_message->message,
					mail_message->length,
					remail_args,
					NULL, NULL);
				else
					goto remail_error_exit;
				i++;
			}

#ifndef MIXMASTER_STDIN
			unlink (temp);
#endif

		}
		else {
#endif
			run_program(mail_args[0],mail_message->message,
				mail_message->length,mail_args,NULL, NULL);
#ifndef NO_MIXMASTER
		}
#endif

		if (deliver_flags & DELIVER_LOG)  {
			char	*record;

			record = find_mailrc("record");

			if (record && *record) {

				/* Expand filename to full path */

				record = expand_filename (record);

				log_message = to_send;
				if (deliver_flags & DELIVER_RAW) {
					if (raw_message)
						log_message = raw_message;
				}

				write_buffer_to_mail_file(log_message,recipient,
					cc,subject,record);
			}
		}

		if (raw_message)
			free_buffer (raw_message);

		free_buffer(to_send);
		free_buffer(mail_message);
		free_buffer (full_list);
		free_buffer (cc_list);
		free_buffer (bcc_list);

		if (addrs)
			free (addrs);
		if (cc_addrs)
			free (cc_addrs);
		if (bcc_addrs)
			free (bcc_addrs);

		close_deliver_window(w);
	}
	clear_busy ();
}

void	move_message_proc(s)

char	*s;

{
	MESSAGE	*m;
	char	mess[128];
	int	n = 0;

	update_random();

	if (!s)
		return;

	s = expand_filename (s);

	/* Start at the head of the list */

	m = messages.start;

	while (m) {

		if (m->flags & MESS_SELECTED) {
			if (!append_message_to_file (m,s,FALSE)) {
				n++;
			}
			else if (!failed_save_notice_proc()) {
				return;
			}
		}

		m = m->next;
	}

	if (!n)
		set_main_footer("No messages saved.");
	else {
		sprintf(mess,"%d messages moved to %s",n,s);
		set_main_footer(mess);
		delete_message_proc();
	}
}

void	copy_message_proc(s)

char	*s;

{
	MESSAGE	*m;
	int	n = 0;
	char	mess[128];

	update_random();

	if (!s)
		return;

	s = expand_filename (s);

	/* Start at the head of the list */

	m = messages.start;

	while (m) {

		if (m->flags & MESS_SELECTED) {
			if (!append_message_to_file (m,s,FALSE)) {
				n++;
			}
			else if (!failed_save_notice_proc()) {
				return;
			}
		}

		m = m->next;
	}

	if (!n)
		set_main_footer("No messages saved.");
	else {
		sprintf(mess,"%d messages saved to %s",n,s);
		set_main_footer(mess);
	}
}

void	load_new_mail()

{
	MESSAGE	*m,*last;
	int	l,i;

	update_random();

	last = messages.end;

	set_main_footer("Retrieving new mail...");
	read_new_mail();

	if (last) {
		MESSAGE	*mm;

		m = last->next;
		i = last->number;
		mm = last;

		while (mm && (mm->flags & MESS_DELETED) && ! show_deleted) {
			mm = mm->prev;
		}

		if (mm)
			l = mm->list_pos;
		else
			l = 0;
	}
	else {
		m = messages.start;
		i = l = 0;
	}

	update_message_list ();

	while (m) {
		m->number = ++i;
		if (!(m->flags & MESS_DELETED) || show_deleted)
			m->list_pos = ++l;

		set_message_description (m);
		display_message_description (m);

		m = m->next;
	}

	clear_main_footer();
}

void	check_for_new_mail()

{
	MESSAGE	*m,*last;
	static	long	test_interval = 0;
	time_t	now;

	/* If no messages displayed in testinterval seconds, then clear
	   the passphrase */

	if (!test_interval) {
		char	*f;

		f = find_mailrc("testinterval");
		if (f)
			test_interval = atoi (f);
		else
			test_interval = (-1);
	}

	if (test_interval > 0 && last_displayed > 0) {
		time (&now);
		if ((now - last_displayed) > test_interval)
			destroy_passphrase (TRUE);
	}

	/* Feed some bits to the random number generator */

	update_random();

	/* Finally, check for new mail ! */

	if (is_new_mail()) {
		show_newmail_icon ();
		load_new_mail ();
	}
}

int	load_file_proc(s)

char	*s;

{
	MESSAGE	*m,*om;
	int	i;
	int	res;

	destroy_passphrase(FALSE);

	s = expand_filename (s);

	show_busy ();
	set_main_footer ("Saving changes and loading new mail...");

	/* Save any changes to the file */

	if (save_changes () < 0) {
		clear_busy();
		if (failed_save_notice_proc ()) {
			clear_main_footer ();
			return -1;
		}
	}

	/* Free main list */

	m = messages.end;

	while (m) {
		om = m;
		m = m->prev;

		free_message(om);
	}

	deleted.start = NULL;
	deleted.end = NULL;
	deleted.number = 0;

	messages.start = NULL;
	messages.end = NULL;

	messages.number = 0;
	messages.encrypted = 0;
	messages.new = 0;
	messages.unread = 0;

	if (!s || !*s)
		res = read_mail_file(default_mail_file,TRUE);
	else
		res = read_mail_file(s,FALSE);

	m = messages.start;

	i = 1;
	while (m) {
		m->list_pos = i;
		m->number = i;
		set_message_description(m);
		display_message_description(m);

		m = m->next;

		i++;
	}

	update_message_list ();
	set_initial_scrollbar_position ();

	last_message_read = NULL;
	message_to_decrypt = NULL;

	clear_busy ();
	clear_main_footer();

	return 0;
}

void	inbox_proc()

{
	update_random();

	show_busy ();
	if (reading_file (default_mail_file)){
	    load_new_mail ();
	}
	else{
	    deleteAllMessages();
	    load_file_proc (default_mail_file);
	}

	clear_busy();
}

void	save_changes_proc()

{
	char	*s;

	update_random();

	s = current_mail_file ();
	if (s && *s) {
		load_file_proc (s);
	}
}

void	done_proc()

{
	update_random();

	show_busy ();
	save_changes_proc ();
	close_all_windows ();
	clear_busy ();
}

static	int	number_cmp (m1,m2)

MESSAGE	**m1,**m2;

{
	return (*m1)->number - (*m2)->number;
}

static	int	status_cmp (m1,m2)

MESSAGE	**m1,**m2;

{
	if ((*m1)->status == (*m2)->status)
		return (*m1)->number - (*m2)->number;

	return (*m1)->status - (*m2)->status;
}

static	int	size_cmp (m1,m2)

MESSAGE	**m1,**m2;

{
	return (*m1)->size - (*m2)->size;
}

static	int	date_cmp(m1,m2)

MESSAGE	**m1,**m2;

{
	return ((*m1)->time_secs - (*m2)->time_secs);
}

static	int	sender_cmp(m1,m2)

MESSAGE	**m1,**m2;

{
	return strcmp((*m1)->email,(*m2)->email);
}

static	int	subject_cmp(m1,m2)

MESSAGE	**m1,**m2;

{
	if ((*m1)->subject == NULL) {
		if ((*m2)->subject == NULL)
			return 0;
		return -1;
	}

	if ((*m2)->subject == NULL)
		return 1;

	return strcmp((*m1)->subject,(*m2)->subject);
}

static	int	sort_messages (proc)

int	(*proc)();

{
	MESSAGE	**m_list;
	int	n, i = 0, l = 1;
	MESSAGE	*m;

	update_random();

	show_busy ();
	n = messages.number + deleted.number;

	if (n <= 0)
		return;

	m_list = (MESSAGE **) malloc (n * sizeof (MESSAGE *));

	m = messages.start;

	while (m) {
		m_list [i++] = m;
		m = m->next;
	}

	qsort (m_list, n, sizeof (MESSAGE *), proc);

	messages.start = m_list[0];
	m_list[0]->prev = NULL;

	for (i = 0; i < n; i++) {
		if (i) {
			m_list[i]->prev = m_list[i-1];
		}
		if (i != (n-1)) {
			m_list[i]->next = m_list[i+1];
		}

		if (!(m_list[i]->flags & MESS_DELETED) || show_deleted) {
			m_list[i]->list_pos = l++;
			set_message_description (m_list[i]);
			display_message_description (m_list[i]);
		}
	}

	messages.end = m_list[n-1];
	messages.end->next = NULL;

	free (m_list);
	clear_busy ();
}

void	sort_by_time ()

{
	sort_messages (date_cmp);
}

void	sort_by_number ()

{
	sort_messages (number_cmp);
}

void	sort_by_subject ()

{
	sort_messages (subject_cmp);
}

void	sort_by_sender ()

{
	sort_messages (sender_cmp);
}

void	sort_by_size ()

{
	sort_messages (size_cmp);
}

void	sort_by_status ()

{
	sort_messages (status_cmp);
}

void	undelete_last_proc ()

{
	MESSAGE	*m, *p;
	int	l;

	update_random();

	/* Return if nothing to do */

	if (!(deleted.start))
		return;

	m = deleted.start;

	if (!m->dnext) {
		deleted.start = deleted.end = NULL;
	}
	else {
		m->dnext->dprev = NULL;
		deleted.start = m->dnext;
	}

	undelete(m);
}

void
undelete(MESSAGE *m)
{
    MESSAGE	*p;
    int		l;

    if(! (m->flags |= MESS_DELETED))
	return;

    m->dnext = m->dprev = NULL;
    m->flags &= ~MESS_DELETED;

    deleted.number--;
    messages.number++;

    if (m->status == MSTAT_NONE)
	messages.new++;
    if (m->status == MSTAT_UNREAD)
	messages.unread++;

    if (m->flags & MESS_ENCRYPTED)
	messages.encrypted++;

    p = m->prev;

    while (p && (p->flags & MESS_DELETED)) {
	p = p->prev;
    }

    if (p)
	l = p->list_pos + 1;
    else
	l = 1;

    while (m) {
	m->list_pos = l++;
	set_message_description (m);
	display_message_description (m);

	m = m->next;
	while (m && (m->flags & MESS_DELETED))
	    m = m->next;
    }

    update_message_list ();
    sync_list();
} /* undelete */

static	char	dec_mess [] = "Decrypted message reads :\n\n";
static	char	sig_mess [] = "\n\nMessage was signed :\n\n";
static	char	end_mess [] = "\n\nEnd of signature information\n";

static void print_exec(args, buff, l)

char	*args;
byte	*buff;
int	l;

{
  FILE *out, *popen();

  if( (out = popen(args,"w")) == (FILE *) NULL) {
    return;
  }
  fwrite (buff, l, 1, out);
  pclose(out);
}

static	void	print_message_proc (raw)

int	raw;

{
	MESSAGE	*m;
	BUFFER	*out;
	char	*s;
	char	mess [128];
	int	c = 0;

	update_random ();

	show_busy ();
	out = new_buffer ();

	m = messages.start;

	while (m) {
		if (!(m->flags & MESS_DELETED) && (m->flags & MESS_SELECTED)) {
			c++;

			add_to_buffer (out, m->header->message,
				m->header->length);
			add_to_buffer (out, "\n", 1);

			if ((m->flags & MESS_VERIFIED) && !raw) {
				if (m->flags & MESS_ENCRYPTED) {
					add_to_buffer (out, dec_mess,
						strlen (dec_mess));
				}
				add_to_buffer (out, m->decrypted->message,
					m->decrypted->length);
				if (m->flags & MESS_SIGNED) {
					add_to_buffer (out, sig_mess,
						strlen (sig_mess));
					add_to_buffer (out, 
						m->signature->message,
						m->signature->length);
					add_to_buffer (out, end_mess,
						strlen (end_mess));
				}
			}
			else {
				add_to_buffer (out, 
					message_contents(m)->message,
					message_contents(m)->length);
			}

			add_to_buffer (out, "\n", 1);
		}

		m = m->next;
	}

	if (out->length) {
		char	*s;

		s = find_mailrc ("printmail");

		print_exec(s,out->message,out->length);

		sprintf(mess,"%d message%s sent to %s for printing",
			c, (c == 1) ? "" : "s", s);
		set_main_footer(mess);
	}

	free_buffer (out);
	clear_busy ();
}

void	print_cooked_proc ()

{
	print_message_proc (FALSE);
}

void	print_raw_proc ()

{
	print_message_proc (TRUE);
}

void	save_attachment_proc (w, m)

DISPLAY_WINDOW	*w;
MESSAGE	*m;

{
	BUFFER	*b = NULL, *a = NULL, *mess;
	char	*s, *e;
	int	err;

	update_random ();

	if (!m)
		return;

	show_busy ();
	if (m->decrypted)
		mess = m->decrypted;
	else
		mess = message_contents (m);

	/* Now process the attachment */

	switch (m->attachment_type) {

		/* UUENCODED messages will be saved in the current
		   directory */

		case ATTACH_UUENCODE:
		s = strstr (mess->message, "begin");
		if (!s) {
		invalid_attachment:
			clear_busy ();
			invalid_attachment_notice_proc ();
			return;
		}

		e = strstr (s, "\nend");
		if (!e || (e - (char *)mess->message) > mess->length)
			goto invalid_attachment;

		/* Ok, copy it into a new buffer */

		b = new_buffer ();
		add_to_buffer (b, s, (e - s) + 5);

		err = run_program(uudecode_args[0], b->message,
				b->length, uudecode_args, NULL, NULL);

		free_buffer (b);
		if (err) {
			free_buffer (a);
			goto invalid_attachment;
		}
		break;

		default:
		goto invalid_attachment;
	}
	
	clear_busy ();

	/* Now we may have the decoded attachment */

	if (a) {
		free_buffer (a);
	}

}

void	add_key_proc (w, m)

DISPLAY_WINDOW	*w;
MESSAGE	*m;

{
	int	err;

	update_random();

	if (!m)
		return;

	set_display_footer(w, "Adding key to keyring");

	show_busy ();
	if (m->decrypted)
		err = add_key (m->decrypted);
	else
		err = add_key (message_contents(m));
	clear_busy ();

	switch (err) {

		case ADD_OK:
		set_display_footer (w, "Key added");
		break;

		case ADD_NO_KEY:
		case ADD_BAD_KEY:
		case ADD_NO_TEMP:
		bad_key_notice_proc ();
		clear_display_footer ();
		break;

		case ADD_OLD_KEY:
		set_display_footer (w, "No new keys or signatures found");
		break;
	}
}

#ifdef PGPTOOLS
void	reseed_random_generator ()

{
	show_busy();
	reseed_random();
	clear_busy();
}
#endif

/* Look for an attachment in the buffer */

int	buffer_contains_attachment (b)

BUFFER	*b;

{
	char	*begin;

	if (!b || !b->message)
		return FALSE;

	/* Look for uuencode */

	begin = strstr (b->message, "begin ");

	if (!begin)
		return FALSE;

	begin += 6;

	while (*begin && *begin != ' ') {
		if (*begin < '0' || *begin > '9')
			return FALSE;
		begin++;
	}

	if (strstr (begin, "\nend"))
		return ATTACH_UUENCODE;

	return FALSE;
}


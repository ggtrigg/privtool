/* $Id$ 
 *
 * Copyright   : (c) 1999 by Glenn Trigg.  All Rights Reserved
 * Project     : Privtool
 * File        : mail_reader
 *
 * Author      : Glenn Trigg
 * Created     : 10 Jul 1999 
 *
 * Description : 
 */

#ifndef _MAIL_READER_H
#define _MAIL_READER_H

extern int	read_mail_file(char *);
extern void	close_mail_file (void);
extern int	is_mail_message(BUFFER *);
extern MESSAGE	*message_from_message(MESSAGE *, BUFFER *);
extern int	is_mail_message(BUFFER *);
extern void	replace_message_with_message(MESSAGE *, MESSAGE *);
extern int	save_changes(void);
extern void	add_to_deleted(MESSAGE *);
extern int	write_buffer_to_mail_file(BUFFER *, char *, char *, char *,
					  char *);
extern int	append_message_to_file(MESSAGE *, char *, int);
extern void	read_new_mail(void);
extern int	is_new_mail (void);
extern int	read_mail_file(char *);
extern int	reading_file (char *);
extern int	ignore_line(char *);

#endif /* _MAIL_READER_H */

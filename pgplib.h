/* $Id$ 
 *
 * Copyright   : (c) 1999 by Glenn Trigg.  All Rights Reserved
 * Project     : Privtool
 * File        : pgplib
 *
 * Author      : Glenn Trigg
 * Created     : 10 Jul 1999 
 *
 * Description : Prototypes for functions in pgplib.c
 */

#ifndef _PGPLIB_H
#define _PGPLIB_H

extern void	destroy_passphrase(int);
extern void	init_pgplib(void);
extern void	close_pgplib(void);
extern void	update_random(void);
extern int	decrypt_message (BUFFER *, BUFFER *, BUFFER *, char *,
				 int, byte *);
extern int	buffer_contains_key (BUFFER *);
extern int	encrypt_message(char **, BUFFER *, BUFFER *, int, char *,
				char *, byte *);
extern int	run_program(char *, byte *, int, char **, char *, BUFFER *);
extern int	add_key (BUFFER *);

#endif /* _PGPLIB_H */

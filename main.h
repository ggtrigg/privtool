/* $Id$ 
 *
 * Copyright   : (c) 1999 by Glenn Trigg.  All Rights Reserved
 * Project     : Privtool
 * File        : main
 *
 * Author      : Glenn Trigg
 * Created     : 10 Jul 1999 
 *
 * Description : Prototypes for externally visible functions in main.c
 */

#ifndef _MAIN_H
#define _MAIN_H

extern void	sync_list(void);
extern void	copy_to_nl(char *, char *);
extern int	retain_line(char *);
extern int	kill_user(char *);
extern int	kill_subject (char *);
extern int	maybe_cfeed(char *);
extern void	clear_aliases(void);

#endif /* _MAIN_H */

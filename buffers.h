
/*
 * @(#)buffers.h	1.7 6/11/96
 *
 *	(c) Copyright 1993-1994 by Mark Grant. All right reserved.
 *	The author assumes no liability for damages resulting from the 
 *	use of this software, even if the damage results from defects in
 *	this software. No warranty is expressed or implied.
 *
 *	This software is being distributed under the GNU Public Licence,
 *	see the file COPYING for more details.
 *
 *			- Mark Grant (mark@unicorn.com) 29/6/94
 *
 */

typedef struct {

#ifndef SVR4
	byte	*message;
	int32	length;
	int32	size;
#else
	char	*message;
	int	length;
	int	size;
#endif
} BUFFER;

/* We allocate space in units of QUANTA bytes to try to avoid too much
   heap fragmentation */

#define QUANTA	128

extern	BUFFER	*new_buffer (void);
extern	void	add_to_buffer (BUFFER *buffer, byte *mess, int len);
extern	void	clear_buffer (BUFFER *b);
extern	void	free_buffer (BUFFER *b);
extern	void	reset_buffer (BUFFER *b);


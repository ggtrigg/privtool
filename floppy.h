/*
 *	@(#)floppy.h	1.1 7/23/95
 *
 *	(c) Copyright 1993-1995 by Mark Grant. All right reserved.
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

extern	FILE	*get_flop_file (void);
#ifdef AUTO_EJECT
extern	void	eject_floppy (void);
#endif


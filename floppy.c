/*
 *	$RCSfile$	$Revision$ 
 *	$Date$
 *
 *	(c) Copyright 1993-1996 by Mark Grant. All right reserved.
 *	The author assumes no liability for damages resulting from the 
 *	use of this software, even if the damage results from defects in
 *	this software. No warranty is expressed or implied.
 *
 *	This software is distributed under the GNU Public Licence, see
 *	the file COPYING for more details.
 *
 *			- Mark Grant (mark@unicorn.com) 29/6/94
 *	
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#ifndef linux
#include <sun/dkio.h>
#endif

#include "floppy.h"

static	FILE	*flop_file;

FILE	*get_flop_file (void)

{
	if (!flop_file) {
		flop_file = fopen (FLOP_FILE, "rb");
	}

	return flop_file;
}

close_floppy (void)

{
	if (flop_file) {
		fclose (flop_file);
		flop_file = NULL;
	}
}

#ifdef AUTO_EJECT
void	eject_floppy(void)

{
	(void) get_flop_file ();
	if (flop_file) {
		ioctl (fileno (flop_file), FDKEJECT, 0);
		close_floppy ();
	}
}
#endif


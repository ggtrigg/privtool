/* $Id$ 
 *
 * Copyright   : (c) 1997 by Glenn Trigg.  All Rights Reserved
 * Project     : Privtool - motif version
 * File        : debug
 *
 * Author      : Glenn Trigg
 * Created     :  8 Mar 1997 
 *
 * Description : 
 */

#ifndef _DEBUG_H
#define _DEBUG_H

extern int	debug_;

#define DEBUG(n, s)	if((n) <= debug_) {	\
    printf("Privtool Debug: ");		\
    printf s; }

#define DEBUG1(s)	DEBUG(1, s)
#define DEBUG2(s)	DEBUG(2, s)
#define DEBUG3(s)	DEBUG(3, s)

#endif /* _DEBUG_H */

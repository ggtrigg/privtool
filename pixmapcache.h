/* $Id$ 
 *
 * Copyright   : (c) 1997 by Glenn Trigg.  All Rights Reserved
 * Project     : Privtool - motif version
 * File        : pixmapcache
 *
 * Author      : Glenn Trigg
 * Created     : 19 Feb 1997 
 *
 * Description : 
 */

#ifndef _PIXMAPCACHE_H
#define _PIXMAPCACHE_H

void		cache_pixmap_from_data(char **data, char *name);
Pixmap		get_cached_pixmap(Widget w, char *name, Pixmap *);

#endif /* _PIXMAPCACHE_H */

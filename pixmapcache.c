/* $Id$ 
 *
 * Copyright   : (c) 1997 by Glenn Trigg.  All Rights Reserved
 * Project     : Privtool - motif version
 * File        : pixmapcache
 *
 * Author      : Glenn Trigg
 * Created     : 19 Feb 1997 
 *
 * Description : Implements a cache for included pixmaps which can
 *		 then be used for labelPixmap's etc. so separate pixmap
 *		 files need not be shipped with the executable.
 */

#include	<X11/X.h>
#include	<X11/Xlib.h>
#include	<X11/Intrinsic.h>
#include	<X11/xpm.h>
#include	"m_util.h"

typedef struct pixmap_cache_entry	PMCEntry;

struct pixmap_cache_entry
{
    char	*name;
    PMCEntry	*prev, *next;
    XpmImage	*image;
};

static PMCEntry		*first_entry = NULL, *last_entry = NULL;

/*----------------------------------------------------------------------*/

void
cache_pixmap_from_data(char **data, char *name)
{
    PMCEntry	*entry;
    XpmImage	*image = (XpmImage *)XtMalloc(sizeof(XpmImage));

    if(XpmCreateXpmImageFromData(data, image, NULL) != XpmSuccess){
	XtFree((char *)image);
	return;
    }

    entry = (PMCEntry *)XtMalloc(sizeof(PMCEntry));
    entry->name = strdup(name);
    entry->image = image;
    entry->next = NULL;

    if(last_entry == NULL){
	first_entry = last_entry = entry;
	entry->prev = NULL;
    }else{
	last_entry->next = entry;
	entry->prev = last_entry;
	last_entry = entry;
    }
}

/*----------------------------------------------------------------------*/

Pixmap
get_cached_pixmap(Widget w, char *name)
{
    int		i;
    PMCEntry	*entry;
    Pixmap	pixmap, mask;
    char	*bgnd;

    if(first_entry == NULL)
	return 0;

    bgnd = GetResourceString(w, "background", "Background");

    for(entry = first_entry; entry != NULL; entry = entry->next){
	if(!strcmp(name, entry->name)){

	    for(i = 0; i < entry->image->ncolors; i++){

		/* Set the background color (defined by color name "None"
		   to the background color of the widget. */
		if( !strcmp(entry->image->colorTable[i].c_color, "None")){
		    free(entry->image->colorTable[i].c_color);
		    entry->image->colorTable[i].c_color = strdup(bgnd);
		    break;
		}
	    }

	    XpmCreatePixmapFromXpmImage(XtDisplayOfObject(w),
					RootWindowOfScreen(XtScreenOfObject(w)),
					entry->image,
					&pixmap,
					&mask,
					NULL);
	    return pixmap;
	}
    }

    return 0;
}

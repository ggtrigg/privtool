
/*
 *	$RCSfile$	$Revision$ 
 *	$Date$
 *
 *	(c) Copyright 1993-1997 by Mark Grant, and by other
 *	authors as appropriate. All right reserved.
 *
 *	The authors assume no liability for damages resulting from the 
 *	use of this software, even if the damage results from defects in
 *	this software. No warranty is expressed or implied.
 *
 *	This software is being distributed under the GNU Public Licence,
 *	see the file COPYING for more details.
 *
 *			- Mark Grant (mark@unicorn.com) 29/6/94
 *
 *	Pass args to setup_display and store our userid in a variable
 *	rather than continually call cuserid().
 *		- Anders Baekgaard (baekgrd@ibm.net) 10th August 1995
 *
 *      Check for .privrc OR .mailrc on startup.  Add a command-line option
 *      "-f path/mailfile" to specify a default mail file.
 *              - Scott Cannon Jr. (scottjr@silver.cal.sdl.usu.edu) 5/30/96
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_GETEUID
#include <pwd.h>
#endif
#include <sys/param.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_MALLOC_H
/* malloc.h superceded by stdlib.h included above */
#include <malloc.h>
#endif
#include <signal.h>

#include "def.h"
#include "buffers.h"
#include "message.h"
#include "mailrc.h"

char	default_mail_file[MAXPATHLEN];
char    globRCfile[MAXPATHLEN];
char    globRCbak[MAXPATHLEN];

/* Currently open mail file */

FILE	*mail_fp;

/* Our user id, so that we don't have to call cuserid() all the time. */

char	*our_userid;

/* Various lists for mailrc entries */

LIST	mailrc;
LIST	alias;
LIST	pgpkey;
LIST	kills_l;
LIST	killu_l;
LIST	nym_list;
LIST	cfeed;
LIST    template_list;
LIST    template_list_fname;
LIST	ignore;
LIST	retain;

/* These store the group ids when we're running setgid mail */

#ifdef SETGID
gid_t	real_gid;
gid_t	mail_gid;
#endif

/* Static prototypes */
static void	move_to_top(LIST *l, MAILRC *m);
static MAILRC	*msearch_list(LIST *l, char *s);
static void	remove_from_list(LIST *l, char *s);
static void	replace_in_list(LIST *l, char *s, char *v);
static char	*search_list(LIST *l, char *s);
static void	add_cfeed(char *s);
static void	add_nym(char *s);
static void	add_template(char *s, char *sn);
static void	default_nym (char *s);
static void	add_ignore(char *s);
static void	add_retain(char *s);
static void	add_alias(char *s);
static void	read_contents(FILE *fp);

/* Mail spool directory */

#ifdef MAIL_SPOOL_DIR
static	char	mail_spool_dir[] = MAIL_SPOOL_DIR;
#else
static	char	mail_spool_dir[] = "/var/spool/mail/";
#endif

/* This is the nym to use */

static	char	*our_nym;

/* This is the security level -- higher is more security */

static	int	security_level = DEFAULT_SECURITY;

/* These are the entries that we should strip quotes from */

static	char	*strip_quotes[] = {

	"folder",
	"record",
	"indentprefix",
	"popuplines",
	"retrieveinterval",
	"testinterval",
	"bell",
	"flash",
	"headerlines",
	"toolcols",
	"printmail",
	"filemenusize",
	"filemenu2",
	"templates",
	"domain",
	"organization",
	"sigfile",
	"replyto",
	"normalheaders",
	NULL,

};

/* Create a mailrc entry */

MAILRC	*new_mailrc(void)

{
	MAILRC	*m;

	m = (MAILRC *) malloc(sizeof(MAILRC));

	if (m) {
		m->next = NULL;
		m->prev = NULL;
		m->name = NULL;
		m->value = NULL;
		m->flags = 0;
	}
	
	return m;
}

/* Free a mailrc entry */

void	free_mailrc(MAILRC *m)

{
	if (m) {

		if (m->name)
			free (m->name);
		if (m->value)
			free (m->value);

		free (m);
	}
}

/* Clear a list */

void	clear_list(LIST *l)

{
	MAILRC	*m,*om;

	m = l->start;

	while (m) {

		om = m;
		m = m->next;

		free_mailrc(om);
	}

	l->number = 0;
	l->start = NULL;
	l->end = NULL;
}

/* Clear the aliases list */

void clear_aliases(void)

{
	clear_list (&alias);
}

/* Add an entry to a list */
void
add_to_list(LIST *l, MAILRC *m)

{
	/* Update count of entries */

	l->number++;

	/* Add new entry */

	if (l->end) {
		l->end->next = m;
		m->prev = l->end;
		l->end = m;
	}
	else
		l->start = l->end = m;
}

/* Move a mailrc entry to the top of the list */

static	void move_to_top(LIST *l, MAILRC *m)

{
	if (l->start == m) 
		return;

	if (l->end == m)
		l->end = m->prev;

	if (m->prev)
		m->prev->next = m->next;
	if (m->next)
		m->next->prev = m->prev;
	m->prev = NULL;
	m->next = l->start;

	l->start->prev = m;
	l->start = m;
}

static	MAILRC	*msearch_list(LIST *l, char *s)

{
	MAILRC	*m;

	m = l->start;

	while (m) {
		if (!strcasecmp (m->name,s))
			return m;
		m = m->next;
	}

	return NULL;
}

static void remove_from_list(LIST *l, char *s)
{
    MAILRC	*m;

    m = l->start;

    while (m) {
	if (!strcasecmp (m->name,s)) {
	    if(m->prev)
		m->prev->next = m->next;
	    if(m->next)
		m->next->prev = m->prev;

	    l->number--;
	    free_mailrc(m);

	    return;
	}
	m = m->next;
    }
}

void print_list(LIST *l)
{
    MAILRC	*m;

    m = l->start;

    while(m) {
	fprintf(stderr, "%s: %s\n", m->name, m->value);
	m = m->next;
    }
}

static void replace_in_list(LIST *l, char *s, char *v)
{
    MAILRC	*m;

    m = l->start;

    while (m) {
	if (!strcasecmp (m->name,s)) {
	    if(m->value) {
		free(m->value);
	    }
	    m->value = strdup(v);
	    return;
	}
	m = m->next;
    }

    m = new_mailrc();

    if (m) {
	m->value = strdup(v);
	m->name = strdup(s);
	m->flags = MAILRC_PREFIXED|MAILRC_OURPREF;

	add_to_list(l, m);
    }
}

void	clear_pgpkey (void)

{
	clear_list (&pgpkey);
}

/* Add an entry to the PGP keys list */

void	add_pgpkey(char *s)

{
	char	*n,*v;
	MAILRC	*m;

	while (*s == ' ')
		s++;

	n = s;

	while (*s != ' ' && *s != '=' && *s) {
		s++;
	}

	if (!*s)
		return;

	*s = 0;
	v = s + 1;

	while (*v == ' ')
		v++;

	/* Update if necessary */

	m = msearch_list (&pgpkey, n);

	if (m) {
		free_string (m->value);
		m->value = strdup (v);
	}
	else {
		m = new_mailrc();
		m->name = strdup(n);
		m->value = strdup(v);
		m->flags = MAILRC_PREFIXED|MAILRC_OURPREF;
		add_to_list(&pgpkey,m);
	}
}

char    *search_templatename(char *s) 

{       MAILRC  *m,*n;

        m = template_list.start;
        n = template_list_fname.start ;
                
        while (m) {
                if (!strcasecmp (m->name,s))
                        return n->name;
                m = m->next;
                n = n->next ;
        }
                                                                        
        return NULL;
}

static	char	*search_list(LIST *l, char *s)

{
	MAILRC	*m;

	m = msearch_list (l, s);
	if (m)
		return m->value;
	return NULL;
}

char	*find_mailrc(char *s)

{
	return search_list(&mailrc,s);
}

void	remove_mailrc(char *s)
{
    remove_from_list(&mailrc, s);
}

void	remove_retain(char *s)
{
    remove_from_list(&retain, s);
}

void	replace_mailrc(char *s, char *v)
{
    replace_in_list(&mailrc, s, v);
}

char	*find_alias(char *s)

{
	return search_list(&alias,s);
}

char	*find_pgpkey(char *s)

{
	return search_list(&pgpkey,s);
}

int	ignore_line(char *s)

{
	return (search_list(&ignore,s) != NULL);
}

int	retain_line(char *s)

{
	return (search_list(&retain,s) != NULL);
}

int	kill_user(char *s)

{
	return (search_list(&killu_l,s) != NULL);
}

int	kill_subject (char *s)

{
	MAILRC	*m;

	m = kills_l.start;

	while (m) {
		if (strstr (s, m->name))
			return TRUE;
		m = m->next;
	}

	return FALSE;
}

int	maybe_cfeed(char *s)

{
	return (search_list(&cfeed,s) != NULL);
}

void	add_entry(LIST *l, char *s)

{
	MAILRC	*m;

	m = msearch_list (l, s);
	if (m)  {
		m->flags = MAILRC_PREFIXED|MAILRC_OURPREF;
	}
	else {
		m = new_mailrc();

		/* Have to strdup() both as clear_list() will free
		   them! */

		m->name = strdup(s);
		m->value = strdup("");
		m->flags = MAILRC_PREFIXED|MAILRC_OURPREF;

		add_to_list(l,m);
	}
}

static	void add_cfeed(char *s)

{
	add_entry(&cfeed,s);
}

void	clear_killu(void)

{
	clear_list (&killu_l);
}

void	add_killu(char *s)

{
	add_entry(&killu_l,s);
}

static	void add_nym (char *s)

{
	add_entry(&nym_list, s);
}

static  void add_template (char *s, char *sn)

{
        add_entry(&template_list, s);
        add_entry(&template_list_fname, sn);
}
                
static	void default_nym (char *s)

{
	MAILRC	*m;

	m = nym_list.start;

	while (m) {
		if (!strcasecmp (m->name,s)) {
			move_to_top (&nym_list, m);
			return;
		}
		m = m->next;
	}
}

int32	nym_count(void)

{
	return nym_list.number;
}

int32   template_count(void)

{
        return template_list.number;
}

char	*nym_name(int n)

{
	MAILRC	*m;

	if (n > nym_list.number)
		return NULL;

	m = nym_list.start;

	while (m && n) {
		m = m->next;
		n--;
	}

	return m->name;
}

char    *template_name(int n)

{
        MAILRC  *m;
        
        if (n > template_list.number)
                return NULL;
                                
        m = template_list.start;
                                        
        while (m && n) {
                m = m->next;
                n--;
        }
                                                                                        
        return m->name;
}

char	*current_nym(void)

{
	return our_nym;
}

void	set_current_nym(char *s)

{
	our_nym = s;
}

void	clear_kills(void)

{
	clear_list (&kills_l);
}

void	add_kills(char *s)

{
	add_entry(&kills_l,s);
}

static	void add_ignore(char *s)

{
	char	*n;

	n = s;
	while (*n) {

		/* Ignore spaces or quotes */

		while (*n == '\'' || *n == ' ') 
			n++;

		s = n;

		/* Find end */

		while (*n != ' ' && *n && *n != '\'')
			n++;

		if (*n) {
			*n = 0;
			n++;
		}

		if (n != s) {
			add_entry(&ignore,s);
		}
	}
}

static	void add_retain(char *s)
{
    char	*n;

    n = s;
    while (*n) {

	/* Ignore spaces or quotes */

	while (*n == '\'' || *n == ' ') 
	    n++;

	s = n;

	/* Find end */

	while (*n != ' ' && *n && *n != '\'')
	    n++;

	if (*n) {
	    *n = 0;
	    n++;
	}

	if (n != s) {
	    add_entry(&retain,s);
	}
    }
}

static	void add_alias(char *s)

{
	char	*n,*v;
	MAILRC	*m;

	while (*s == ' ')
		s++;

	n = s;

	while (*s != ' ' && *s) {
		s++;
	}

	if (!*s)
		return;

	*s = 0;
	v = s + 1;

	m = new_mailrc();

	m->name = strdup(n);
	m->value = strdup(v);

	add_to_list(&alias,m);
}

static	void	read_contents(FILE *fp)

{
	char	line[1024],*s,*sfn,*n,*v;
	int	c,i;
	int	prefixed;
	MAILRC	*m;

	rewind(fp);
	while (!feof(fp)) {

		s = line;
		prefixed = 0;

		do {
			c = getc(fp);
			if (c!= EOF && c!= '\n')
				*s++ = c;
		} while (c != EOF && c!= '\n');

		*s = 0;

		s = line;

		if (line[0] == '#') {
			if (line [1] != '-' && line[1] != '@') 
				continue;

			switch (line[1]) {

				case '-':
				prefixed = MAILRC_PREFIXED;
				break;

				case '@':
				prefixed = MAILRC_PREFIXED|MAILRC_OURPREF;
				break;

			}

			s = line+2;

			if (!strcasecmp (s, "clearaliases")) {
				clear_aliases();
				continue;
			}
			if (!strncasecmp(s,"pgpkey",6)) {
				add_pgpkey(s+6);
				continue;
			}

			if (!strncasecmp(s,"security",8)) {
				security_level = atoi(s+9);

				if (security_level > MAX_SECURITY)
					security_level = MAX_SECURITY;

				if (security_level < MIN_SECURITY)
					security_level = MIN_SECURITY;

				continue;
			}

			/* Kill subject */

			if (!strncasecmp(s,"kills",5)) {
				add_kills(s+6);
				continue;
			}

			/* Kill userid */

			if (!strncasecmp(s,"killu",5)) {
				add_killu(s+6);
				continue;
			}

			/* Include Files */
			
			if (!strncasecmp(s, "incfile", 7)) {
				sfn=s+8 ;
				while (*sfn!=' ') sfn++ ;
				*sfn='\0' ;
				sfn++;
				add_template (s+8,sfn);
				continue;
			}
			
			/* Pseudonym */

			if (!strncasecmp(s, "pseudonym", 9)) {
				add_nym (s+10);
				continue;
			}

			/* Default nym */

			if (!strncasecmp(s, "defnym", 6)) {
				default_nym (s+7);
				continue;
			}

			/* Encrypted feed */

			if (!strncasecmp(s,"cfeed",5)) {
				add_cfeed(s+6);
				continue;
			}
		}
		else {
			if (strncasecmp(line, "set",3)) {
				if (!strncasecmp(line,"alias",5)) 
					add_alias(line+5);
				if (!strncasecmp(line,"ignore",6))
					add_ignore(line+6);
				if (!strncasecmp(line,"retain",6))
					add_retain(line+6);

				continue;
			}

			s = line+3;
		}

		/* Look for the variable name */

		while (*s == ' ')
			s++;

		n = s;

		/* Scan through the name for the space */

		while (*s && *s != ' ' && *s != '=')
			s++;

		/* If no '=something', set value to null string */

		if (!*s) {
			v = "";
		}
		else {

			/* Now set v to point to the value */

			*s = 0;
			v = s + 1;

			while (*v == ' ')
				v++;
		}

		/* We want to strip quotes from some entries */

		i = 0;
		while (strip_quotes[i]) {
			if (!strcasecmp(strip_quotes[i],n)) {

				/* Strip out first quote */

				if (*v == '\'') 
					v++;

				/* Strip out final quote */

				s = v;
				while (*s)
					s++;
				if (*--s == '\'')
					*s = 0;

				/* End the loop */

				break;
			}
			i++;
		}

		/* Right, we've got n pointing to the name, and v to
		   the value, let's do it ! */

		m = new_mailrc();

		if (m) {

			m->value = strdup(v);
			m->name = strdup(n);
			m->flags = prefixed;

			add_to_list(&mailrc,m);
		}
	}
}

void	read_mailrc(void)

{
	char	*loc;
	char	path[MAXPATHLEN], path2[MAXPATHLEN];
	FILE	*mailrcf;
	  {

		strcpy(path2, ".privrc");
	  }
	/* Default to $PRIVRC */

	loc = getenv("PRIVRC");

	add_nym(our_userid);
	our_nym = nym_name (0);

	if (!loc) {

		/* If that fails, try $HOME/.privrc */

		loc = getenv("HOME");

		if (loc) {
			strcpy (path, loc);
			strcat	(path, "/.privrc");
		}
		else
			strcpy (path, ".privrc");
	}
	else
		strcpy (path, loc);

	/* See if we can open it */

	if ((mailrcf = fopen(path,"a+")) == NULL) {
	  if ((mailrcf = fopen(path2,"a+")) == NULL)
	    {
	      printf ("Can't read %s nor %s !\n", path, path2);
	      globRCfile[0] = 0;
	      globRCbak[0] = 0;
	      return;
	    }
	  else
	    strcpy(globRCfile, path2);
	}
	else
		strcpy(globRCfile, path);
	strcpy(globRCbak, globRCfile);
	strcat(globRCbak, ".bak");

	/* Read the contents here */

	read_contents(mailrcf);

	/* If retain list is empty add some defaults so that the header
	   window isn't completely empty. */
	if( retain.number == 0 ) {
	    add_entry(&retain, "From");
	    add_entry(&retain, "Subject");
	    add_entry(&retain, "Date");
	}

	/* Then close the file */

	fclose(mailrcf);
}

#ifndef HAVE_PGPTOOLS
static	char	pgp_exec [] = PGPEXEC;
#endif

main(int argc, char *argv[])

{
	char		*s;
	int		i;
#ifdef HAVE_GETEUID
	struct passwd	*pwent;
#endif
#ifdef MALLOC_TEST
	extern	void	malloc_dump();

	signal (SIGHUP, malloc_dump);
#endif

	/* Check for PGP */

#ifndef HAVE_PGPTOOLS
	if (!pgp_path()) {
		printf ("Can't find PGP at '%s', exiting !\n", pgp_exec);
	}

	srandom (time(0) + getpid());
#endif

	/* Set it here before we forget ! */

	umask (077);

	/* Get our user id */
#ifdef HAVE_GETEUID
	pwent = getpwuid(geteuid());
	our_userid = strdup(pwent->pw_name);

#elif defined(HAVE_GETLOGIN)
/* cuserid:
   a) is obselete and replaced by getpwuid.
   b) returns the effective user id rather than the real one.
      I prefer to read my mail than that of whoever privtool decides it
      belongs to at the moment */

	our_userid = getlogin();
#else  /* Last resort (?) */
	our_userid = cuserid (0);
#endif /* HAVE_GETEUID */

	/* Do all the SETGID stuff */

#ifdef SETGID
	real_gid = getgid ();
	mail_gid = getegid ();
	setegid (real_gid);
#endif

	/* Initialise pgp library */

	init_pgplib();

	/* Initialise messages.c */

	init_messages();

	read_mailrc();

	/* Check for the "-f" option on command line */
	default_mail_file[0] = 0;
	for (i = 1; i < argc; i++)
	  if (strcmp(argv[i], "-f") == 0)
	    {
	      /* Load following file as default mailbox */
	      if (i + 1 >= argc)
		{
		  fprintf(stderr, "%s: Mailbox filename missing with -f option\n", argv[0]);
		  exit(1);
		}
	      else
		strcpy(default_mail_file, argv[i + 1]);
	    }

	if (strlen(default_mail_file) < 1) {
	    if((s = find_mailrc("mailspoolfile"))) {
		strcpy (default_mail_file, s);
	    }
	    else {
		s = getenv("MAIL");

		if (s)
		    strcpy (default_mail_file, s);
		else {
		    sprintf(default_mail_file,"%s/%s", mail_spool_dir,
			    cuserid(NULL));
		}
	    }
	}

	read_mail_file(default_mail_file,TRUE);

	s = getenv("PGPPASS");

	if (security_level < 2 && !s)
		security_level = 2;

	setup_display(security_level,s,argc,argv);

	close_mail_file ();
	close_messages ();
	close_pgplib();
#ifdef MALLOC_TEST
	malloc_dump();
#endif
	exit (0);
}

/* Copy_to_nl() : Strip preceding spaces and trailing nls from a string */

void	copy_to_nl(char *from, char *to)

{
	if (from) {
		while (*from == ' ')
			from++;

		while (*from && *from != '\n')
			*to++ = *from++;
	}

	*to = 0;
}

#ifndef HAVE_PGPTOOLS
char	*pgp_path(void)

{
	static	char	pgp_path[MAXPATHLEN];
	static	int	first = TRUE;

	if (first) {
	}

	if (!access (pgp_exec, X_OK))
		return pgp_exec;
	else
		return NULL;

}
#endif

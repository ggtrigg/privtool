
/*
 * $Id$
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/param.h>
#ifdef GDBM
#include <gdbm.h>
#else
#include <fcntl.h>
#include "db.h"
#endif

#include <usuals.h>
#include <idea.h>
#include <md5.h>
#include <mpilib.h>
#include <fifo.h>
#include <pgptools.h>
#include <pgpkmgt.h>
#include <pgpmem.h>
#include <pgparmor.h>
#include <fifozip.h>

typedef struct	{
	struct	pgp_pubkey	pk;
	byte	trust;
	int	comp;
} our_pgpkey;

/* Open a PGP data file. */

static	FILE	*open_pgp_file (char *s, char *attr)

{
	char	temp[PATH_MAX];
	char	*pgppath;
	FILE	*fp;

	/* Try PGPPATH first */

	if (pgppath = getenv("PGPPATH")) {
		sprintf (temp, "%s/%s", pgppath, s);
		if (fp = fopen (temp, attr))
			return fp;
	}

	/* If not, try local. This could possibly cause security problems,
	   so you may want to take it out with -DMOST_SECURE */

#ifdef MOST_SECURE
	return	NULL;
#else
	return	fopen (s, attr);
#endif
}

#ifdef GDBM
static	char	pgptools_file[] = "pubring.dbm";
#else
static	char	pgptools_file[] = "pubring.db";
#endif

static	void	process_file(char *s)

{
	FILE		*in;
	char	out[1024], *pgppath, *q;
	char	username[2048];
#ifdef GDBM
	GDBM_FILE	outfile;
	datum	key, data, fdata, id;
#else
	DB	*outfile;
	DBT	key, data, fdata, id;
#endif
	our_pgpkey	pk;
	struct	fifo	*f, *k, *k2;
	byte	full_key [65536];
	byte	keyid [8];
	int	i;
	int	cnt = 0;

	fflush (stdout);
	if (!(in = fopen (s, "rt"))) {
		in = open_pgp_file (s, "rt");
	}
	if (!in) {
		printf("Couldn't open %s!\n", s);
		exit (1);
	}
	if (pgppath = getenv("PGPPATH")) {
		sprintf (out, "%s/%s", pgppath, pgptools_file);
	}	
	else {
		printf("PGPPATH not set!\n");
		exit (1);
	}

#ifdef GDBM
	outfile = gdbm_open (out, 512, GDBM_WRCREAT, 0600, 0);
#else
	outfile = dbopen (out, O_RDWR|O_CREAT, 0600, DB_HASH, NULL);
#endif

	if (!outfile) {
		printf("Couldn't create %s\n", out);
		exit (1);
	}

	f = fifo_file_create (in);

#ifdef GDBM
	data.dptr = &pk;
	data.dsize = sizeof (pk);
	fdata.dptr = full_key;
	id.dptr = keyid;
	id.dsize = 8;
#else
	data.data = &pk;
	data.size = sizeof (pk);
	fdata.data = full_key;
	id.data = keyid;
	id.size = 8;
#endif

	while ((k = pgpk_getnextkey(f)) != NULL) {
		if (!(++cnt%50)) {
			putchar ('.');
			fflush (stdout);
		}
		k2 = fifo_copy (k);
		pgp_get_keyid (k2, keyid);
		fifo_destroy (k2);
		k2 = fifo_copy (k);
		pgpk_extract_key(k, &pk.pk, NULL, &pk.trust, &pk.comp);
		memcpy (full_key, &pk.pk, sizeof (pk.pk));
		i = sizeof (pk.pk);
		fifo_destroy (k);
		while (!pk.comp && 
			pgpk_extract_username(k2, username, &pk.trust)) {
			/* Well, we got a username. Now look for an
			   email address */

			q = username;
			memcpy (full_key + i, username, strlen (username) + 1);
			i += strlen (username) + 1;
			full_key [i++] = pk.trust;

			while (*q && *q != '<')
				q++;
			if (*q) {
#ifdef GDBM
				key.dptr = q + 1;
#else
				key.data = q + 1;
#endif
				while (*q && *q != '>')
					q++;
				*q = 0;
#ifdef GDBM
				key.dsize = strlen (key.dptr);
				gdbm_store (outfile, key, data, GDBM_REPLACE);
#else
				key.size = strlen (key.data);
				outfile->put (outfile, &key, &data,
					0);
#endif
			}
		}
		if (!pk.comp) {
#ifdef GDBM
			fdata.dsize = i;
			gdbm_store (outfile, id, fdata, GDBM_REPLACE);
#else
			fdata.size = i;
			outfile->put (outfile, &id, &fdata, 0);
#endif
		}
		fifo_destroy (k2);
	}
#ifdef GDBM
	gdbm_close (outfile);
#else
	outfile->close (outfile);
#endif
	putchar ('\n');
}

main (int argc, char **argv)

{
	int	arg = 1;

	if (argc > 1) {
		while (argc-- > 1) {
			printf("Processing %s", argv[arg]);
			process_file (argv[arg++]);
		}
	}
	else {
		printf ("Processing pubring.pgp");
		process_file ("pubring.pgp");
	}
}



/*
 *	$Id$ 
 *
 *	pgplib.c, (mostly) by Mark Grant 1993-1997
 *
 *	Provides a basic interface to PGP and/or PGP Tools.
 *
 *	Most parts (c) Copyright 1993-1997 by Mark Grant. All rights reserved.
 *	The author assumes no liability for damages resulting from the 
 *	use of this software, even if the damage results from defects in
 *	this software. No warranty is expressed or implied.
 *
 *	Some of this code is based on the file ptd.c by Pr0duct Cypher,
 *	the rest is distributed under the GNU Public Licence; see the file 
 *	COPYING for more details. 
 *
 *			- Mark Grant (mark@unicorn.com) 29/6/94
 *
 *	Major public functions:
 *		init_pgplib () 		-- initialise library
 *		close_pgplib ()		-- shut down library
 *		encrypt_message ()	-- PGP encrypt a message
 *		decrypt_message ()	-- decrypt a PGP message
 *		update_random ()	-- update the random number seed
 *		reseed_random ()	-- generate new seed
 *		our_randombyte ()	-- generate secure random byte
 *		buffer_contains_key ()	-- search buffer for a PGP key
 *		add_key ()		-- add a key to the main PGP keyring
 *		get_md5 ()		-- calculate the MD5 of a passphrase
 */

#define PGPLIB

#include <stdio.h>
#if defined(__FreeBSD__) || defined (SYSV)
#include <signal.h>
#else
#include <vfork.h>
#endif

#include <string.h>
#if defined(__FreeBSD__) || defined (linux)
#include <stdlib.h>
#else
#include <malloc.h>
#include <limits.h>
#endif
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/timeb.h>
#include <sys/resource.h>
#include <ctype.h>
#ifdef GDBM
#include <gdbm.h>
#endif
#ifdef BDB
#include <fcntl.h>
#include <db.h>
#endif

#include "def.h"

#ifdef PGPTOOLS
#include <usuals.h>
#include <md5.h>
#include <idea.h>
#include <mpilib.h>
#include <fifo.h>
#include <pgptools.h>
#include <pgpkmgt.h>
#include <pgpmem.h>
#include <pgparmor.h>
#include <fifozip.h>
#else
#include "pgp-exit.h"
#endif

#include "buffers.h"
#ifdef USE_FLOPPY
#include "floppy.h"
#endif

/* Does SunOS have getenv() in stdlib.h? Perhaps this should be SunOS
   specific, or we should include the right SunOS header file. */

extern	char	*getenv();

#ifdef __sgi
#define vfork fork
#endif

/* BUF_SIZE is the general size of stack buffers. Should this ever be used 
   on DOS, you might want to reduce the value of BUF_SIZE, or replace the 
   stack use with a malloc(). */

#ifdef DOS
#define BUF_SIZE	512
#else
#define BUF_SIZE	4096
#endif

/* Size of MD5 output and IDEA keys: 128 bits each, or 16 bytes. These must
   be the same.*/

#define MD5_SIZE	16
#define IDEA_KEY_SIZE	16

/* Following are for storing messages from PGP or other programs */

static	BUFFER	error_messages;
static	BUFFER	stdout_messages;

/* Following strings generally useful for detecting PGP messages */

static	char	begin_key [] = "\n-----BEGIN PGP PUBLIC KEY BLOCK-----\nVersion: ";
static	char	end_key [] = "\n-----END PGP PUBLIC KEY BLOCK-----";

#ifdef SLOW_STRSTR

/* Use this routine if your machine doesn't have strstr () or it's really 
   slow like SunOS */

char	*mystrstr (char *s1, char *s2)

{
	char	*p, *q, *r;

	p = s1;
	while (*p) {

		/* If we find the first character then look for the rest
		   of the string. We won't try anything fancy here as the
		   code merely has to outperform SunOS and this is
		   already orders of magnitude faster. */

		if (*p == *s2) {
			q = p + 1;
			r = s2 + 1;

			while (*q && *r) {
				if (*r != *q)
					break;
				r++;
				q++;
			}

			/* If we reached the end of the string then we
			   found it. */

			if (!*r)
				return p;
		}

		p++;
	}

	return NULL;
}
#endif

/* All this is only needed if we're using PGP Tools rather than forking off
   a copy of PGP for encryption/decryption */

#ifdef PGPTOOLS

/* We'll set up all the strings and data we need here */

/* PGP Version number to fake -- PGP Tools is really always 2.3a */

static	char	pgp_version [] = PGPVERSION;
static	char	two_n [] = "\n\n";

/* ASCII armor lines */

static	char	begin_armour[]="-----BEGIN PGP MESSAGE-----\nVersion: ";
static	char	end_armour[]="-----END PGP MESSAGE-----\n";
static	char	begin_signed[]="-----BEGIN PGP SIGNED MESSAGE-----\n\n";
static	char	begin_signature[]="\n-----BEGIN PGP SIGNATURE-----\nVersion: ";
static	char	end_signature[]="-----END PGP SIGNATURE-----\n";

/* Copies of the messages that PGP outputs. We should really read these in
   from the language definition file, but that's a lot of hassle. Perhaps
   we should process the language file before compiling and replace these
   as appropriate. */

static	char	good_sig [] = "Good Signature from user ";
static	char	bad_sig [] = "WARNING: Bad signature, doesn't match file contents !\nBad signature from user ";
static	char	no_key [] = "WARNING: Can't check signature integrity, can't find public key\n         from user 0x";

/* Message to tell the user that we couldn't decrypt because we don't have
   the key */

static	char	rsa_failed [] = "\n\n******* DECRYPTION FAILED *******\n\nMessage can only be read by:\n\n\t";
static	char	retstring [] = "\n\t";

/* Random number generator variables */

#define RAND_SIZE	256

/* Pointer into buffer for updates */

static	int	rand_pointer = 0;
static	byte	init_rand = FALSE;

/* Random buffers */

static	byte	privseed[RAND_SIZE];
static	byte	random_seed[RAND_SIZE];

/* IDEA has weak keys of the form:

	0000,0000,0X00,0000,0000,000X,XXXX,X000

   where X is any number (Applied Cryptography, first edition, page 264). We 
   check for these, even though they're extremely rare. */

static	byte	idea_weak_key_mask [IDEA_KEY_SIZE] =

{
	0xFF, 0xFF,
	0xFF, 0xFF,
	0xF0, 0xFF,
	0xFF, 0xFF,
	0xFF, 0xFF,
	0xFF, 0xF0,
	0x00, 0x00,
	0x0F, 0xFF
};

/* PGP Tools only routines */

/* Open a PGP data file. */

static	FILE	*open_pgp_file (char *s, char *attr)

{
	char	temp[PATH_MAX];
	char	*pgppath;
	FILE	*fp;

	/* Try PGPPATH first */

	if ((pgppath = getenv("PGPPATH"))) {
		sprintf (temp, "%s/%s", pgppath, s);
		if ((fp = fopen (temp, attr)))
			return fp;
	}

	/* Then try ${HOME}/.pgp (the default for pgp itself). */

	if ((pgppath = getenv("HOME"))) {
	    sprintf (temp, "%s/.pgp/%s", pgppath, s);
	    if ((fp = fopen (temp, attr)))
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

/* Copy a fifo into a buffer. We need this to interface between the
   PGP Tools code and our own buffer code. */

static	void	fifo_to_buffer (struct fifo *f, BUFFER *b)

{
	int	n,sz;
	char	buf [BUF_SIZE];

	/* Get the fifo size */

	sz = fifo_length (f);

	/* Loop round copying fifo into buffer. We can safely ask
	   fifo_aget() for more data than the fifo contains, saving
	   a test in each loop. */

	while (sz > 0) {
		n = fifo_aget (buf, BUF_SIZE, f);
		add_to_buffer (b, buf, n);
		sz -= n;
	}
}

/* This function is called to update the PGP Tools random number seed
   each time we fill our buffer with random data. */

static	void	update_pgptools_random(void)

{
	struct	fifo	*f;

	/* Reset the pointer into the random seed buffer */

	rand_pointer = 0;

	/* Create a fifo for PGP Tools */

	f = fifo_mem_create ();

	/* We pass privseed through the first time, then random_seed (the 
	   new data) after that. That way, privseed[] will match the 
	   contents of the PGP Tools random seed, which xors together all 
	   the data passed into it. */

	if (init_rand) {
		fifo_aput (random_seed, RAND_SIZE, f);
		bzero (random_seed, RAND_SIZE);
	}
	else {
		fifo_aput (privseed, RAND_SIZE, f);
		init_rand = TRUE;
	}

	/* pgp_initrand() destroys the fifo when called */
			
	pgp_initrand (f);
}

/* 
   This is called instead of pgp_randombyte() to ensure that our random
   seed is passed through to initialise the PGP Tools generator. Note that 
   this means that you should not use any PGP Tools function with implied 
   calls to pgp_randombyte() without initialising the random number 
   generator; a line

 	(void) our_randombyte();

   will do fine if you don't want to duplicate the code.
*/

byte	our_randombyte(void)

{
	/* The first time through we need to initialise the random
	   number generator. We don't do this in init_pgplib() because
	   ideally we want to build up some real random data before we
	   use the RNG. */

	if (!init_rand) 
		update_pgptools_random ();

	/* Now call it since we know it's initialised */

	return pgp_randombyte();
}

/* Add arbitrary data to the random number seed. This is public but 
   shouldn't generally be called, particularly as it's specific to
   PGP Tools. */

void	add_to_random (byte *d, int s)

{
	int	n;

	while (s > 0) {

		/* We want to copy over either the entire buffer, or enough
		   bytes to fill the random seed. This way we save a test
		   in each inner loop. */

		n = min (s, (RAND_SIZE - rand_pointer));
		s -= n;

		while (n--) {

			/* We xor the data into privseed, and copy it into 
			   random_seed, since that will be xor-ed into the buffer
			   in the PGP Tools random number code. */

			privseed [rand_pointer] ^= *d;
			random_seed [rand_pointer++] = *d++;
		}

		/* Pass it to PGP Tools if we reached the end of the 
		   buffer. It should never be greater, but we'll check
		   just in case. */

		if (rand_pointer >= RAND_SIZE)
			update_pgptools_random();
	}
}

/* We take the contents of the privseed array, then use the IDEA cipher with
   a key based on the current time and process id to encrypt the array. This
   gives slightly more security than just using the array as read from the
   disk. */

static	void	idea_privseed(int decryp)

{
	struct	timeb	timestamp;
	int	pid;
	byte	key[IDEA_KEY_SIZE];
	word16	iv[4];		/* Don't really care what's in here, 
				   whatever's on the stack will do. Sadly,
			  	   this will probably be zero when we are
				   first called, and when we need it the
				   most */
	int	i, t;

	/* First we create an IDEA key with the current time and
	   process ID. Against a serious attacker this will probably 
	   only give eight or ten bits of real randomness. Weak keys
	   don't really matter here. */

	ftime (&timestamp);
	t = (int) timestamp.time;

	for (i = 0; i < 4; i++, t >>= 8)
		key [i] = key [i+8] = (t & 0xff);

	/* Millisecond time */

	t = (int) timestamp.millitm;

	for (i = 0; i < 2; i++, t >>= 8)
		key [i+4] = key [i+12] = (t & 0xff);

	/* Tidy up */

	bzero (&timestamp, sizeof (timestamp));
	t = 0;

	/* Finish off with the process ID */

	pid = (int) getpid();

	for (i = 0; i < 2; i++, pid >>= 8) 
		key [i+6] = key [i+14] = (pid & 0xff);

	pid = 0;

	/* Right, now we have the key, encrypt/decrypt the data */

	initcfb_idea (iv, key, decryp);
	ideacfb (privseed, RAND_SIZE);
	close_idea ();

	/* Now zero the key and iv. Close_idea() zeroed all internal
	   data. */

	bzero (key, IDEA_KEY_SIZE);
	bzero (iv, 8);
}

/* For Linux we'll add in the MD5 of some files under /proc */

#ifdef linux

/* Process a single /proc file */

static	void	add_proc_file (char *s)

{
	int	fd;
	int	sz;
	char	path[PATH_MAX];
	char	buf[BUF_SIZE];
	MD5_CTX	context;
	byte	md5[MD5_SIZE];

	/* Silently ignore the call if the file doesn't exist. Most
	   linux installations won't have all the files we look for. */
	
	sprintf(path, "/proc/%s", s);
	fd = open (path, O_RDONLY);
	if (fd < 0)
		return;

	/* Take the MD5 of the file */

	MD5Init (&context);
	do {
		sz = read (fd, buf, BUF_SIZE);
		if (sz > 0)
			MD5Update (&context, buf, sz);
	} while (sz > 0);
	MD5Final (md5, &context);

	close (fd);

	/* Add it to the random buffer and tidy up */

	add_to_random (md5, MD5_SIZE);

	bzero (md5, MD5_SIZE);
	bzero (&context, sizeof (context));
	bzero (buf, BUF_SIZE);
	bzero (path, PATH_MAX);
}

/* List of files that we're going to check. Most of these change frequently. */

static	char	*proc_files[] = {
	"apm",
	"interrupts",
	"loadavg",
	"uptime",
	"meminfo",
	"modules",
	"stat",
	"net/sockstat",
	"net/dev",
	"net/snmp",
	NULL,
};

/* Loop around processing each file */

static	void	add_proc_data (void)

{
	char	**p = proc_files;

	while (*p)
		add_proc_file (*p++);
}
#endif

/* We optionally read data from the audio device and use this as another
   source of random data. Obviously, the *worse* your sound card is, the
   better this will be, because you'll have more random noise! */

#ifdef USE_AUDIO

/* Increase the value of AUDIO_BUFF if you have a faster sound card */

#define AUDIO_BUFF	512

static	int	merge_in_audio (byte *p)

{
	byte	audio[AUDIO_BUFF];
	int	audio_fd;
	int	audio_in;
	int	audio_p;
	MD5_CTX	context;
	byte	md5[MD5_SIZE];

	/* Open the device; currently all OSes call it /dev/audio */

	audio_fd = open ("/dev/audio", O_RDONLY);

	/* Did we get it ? */
	
	if (audio_fd >= 0) {
		int	i;

		audio_p = 0;

		/* We loop over the entire privseed array */

		while (audio_p < RAND_SIZE) {

			/* Read some bytes, and make sure we get the full
			   amount (just in case the sound card is
			   configured strangely). Assume the call never
			   fails. */

			audio_in = 0;
			while (audio_in < AUDIO_BUFF) {
				audio_in += read (audio_fd, audio + audio_in, 
					AUDIO_BUFF - audio_in);
			}			

			/* Now MD5 the block */

			MD5Init (&context);
			MD5Update (&context, audio, AUDIO_BUFF);
			MD5Final (md5, &context);

			/* And XOR with the privseed data */

			for (i = 0; i < MD5_SIZE;)
				p[audio_p++] ^= md5[i++];
		}

		/* Tidy up */

		bzero (md5, MD5_SIZE);
		bzero (audio, AUDIO_BUFF);
		bzero (&context, sizeof (context));

		/* And close it so that other applications can use it */

		close (audio_fd);

		return TRUE;
	}
	else {

		/* We'll let the calling code deal with the problem, but
		   warn the user just in case. Could easily happen if
		   another application has the file open. */

		printf("Pgplib: Cannot open audio device!\n");

		return FALSE;
	}
}
#endif

/* Do what we can to reseed the random number generator */

void	reseed_random (void)

{
	int	i;

#ifdef USE_AUDIO
	/* Don't bother to zero the seed because it may contain some
	   useful random data from the stack. */

	byte	seed[RAND_SIZE];

	/* In this case we can fill random_seed with data from the
	   audio device. Currently we fail silently if we couldn't
	   read any data. Should fix this. */

	if (merge_in_audio (seed)) 
		for (i = 0; i < RAND_SIZE; i++) {
			privseed[i] ^= (random_seed[i] = seed[i]);
		}

	/* Tidy up */

	bzero (seed, RAND_SIZE);
#else

	/* Otherwise all we can really do is encrypt the privseed
	   array again */

	idea_privseed (FALSE);
	for (i = 0; i < RAND_SIZE; i++)
		random_seed[i] ^= privseed[i];
#endif

	/* Pass the new data to PGP Tools */

	update_pgptools_random();

	/* And optionally add in the contents of the /proc files */

#ifdef linux
	add_proc_data ();
#endif
}

#ifdef GDBM
static	GDBM_FILE	keyfile = 0;
#endif
#ifdef BDB
static	DB	*keyfile;
#endif

/* Find a public key, given the userid */

static	int	get_public_key (byte *id, byte *userid, struct pgp_pubkey *key, 
			byte *trust)

{
	FILE	*pkr;
	struct	fifo	*keyring = NULL;
	struct	fifo	*temp = NULL;
	int	revoked;
	int	found = FALSE;
	byte	our_uid [256];
#ifdef GDBM
	datum	data, idkey;
#endif
#ifdef BDB
	DBT	data, idkey;
#endif

	/* First check the hash table */

#ifdef USE_HASH
	if (pgp_hash_get (id, userid, key, trust))
		return TRUE;
#endif

#if defined(GDBM) || defined (BDB)
	if (!keyfile) {
		char	file[PATH_MAX];
		char	*pgpp;

		pgpp = getenv ("PGPPATH");
		if (pgpp) {
#ifdef GDBM
			sprintf (file, "%s/pubring.dbm", pgpp);
			keyfile = gdbm_open (file, 512, GDBM_READER, 
				0600, 0);
#else
			sprintf (file, "%s/pubring.db", pgpp);
			keyfile = dbopen (file, O_RDONLY, 0600,
				DB_HASH, NULL);
#endif
		}
	}
	if (keyfile) {
#ifdef GDBM
		idkey.dptr = id;
		idkey.dsize = 8;
		data = gdbm_fetch (keyfile, idkey);

		if (data.dptr) {
			memcpy (key, data.dptr, sizeof (*key));
			strcpy (userid, data.dptr + sizeof (*key));
			*trust = data.dptr [sizeof (*key) + 
				strlen (userid) + 2];
			return TRUE;
		}
#else
		idkey.data = id;
		idkey.size = 8;
		found = (keyfile->get (keyfile, &idkey, &data, 0) >= 0);

		if (found && data.data) {
			byte	*p = (byte *)data.data;

			memcpy (key, p, sizeof (*key));
			strcpy (userid, p + sizeof (*key));
			*trust = p [sizeof (*key) + strlen (userid) + 2];
			return TRUE;
		}
#endif
	}
#endif

	/* Oh well, now for the hard work */

	/* First check smallring.pgp in case it's a common key */

	pkr = open_pgp_file ("smallring.pgp", "rb");

	/* Don't do anything if the file doesn't exist, otherwise
	   search for the key. */

	if (pkr) {
		keyring = fifo_file_create (pkr);
		temp = pgpk_findkey (keyring, id, FALSE);

		if (temp) 
			found = TRUE;
		else
			fclose (pkr);
	}

	/* Now go for the pubring file if we didn't find it */

	if (!found) {
		pkr = open_pgp_file ("pubring.pgp", "rb");

		/* We don't care about smallring.pgp, but if the main
		   keyring doesn't exist then we're in trouble */

		if (!pkr)
			return FALSE;

		keyring = fifo_file_create (pkr);
		temp = pgpk_findkey (keyring, id, FALSE);

		/* Return an error if it failed. */

		if (!temp) {
			fifo_destroy (keyring);
			fclose (pkr);

			return FALSE;
		}
	}

	/* Extract the key from the fifo */

	pgpk_extract_key (temp, key, (struct pgp_seckey *)NULL, 
		(byte *)NULL, &revoked);

	/* We want the userid whether they asked for it or not */

	if (!userid)
		userid = our_uid;

	if (!pgpk_extract_username (temp, userid, trust)) 
		*userid = 0;

	/* Add it to the table */

#ifdef USE_HASH
	pgp_hash_put (id, (char *)userid, key, *trust);
#endif

	/* Tidy up */

	fifo_destroy (temp);
	fifo_destroy (keyring);
	bzero (our_uid, 256);
	fclose (pkr);

	return TRUE;
}

/* get_md5() - we use this to get the MD5 of the passphrase, to keep
   all the crypto code in one file. */

void	get_md5 (char *pass, byte *md5_val)

{
	MD5_CTX	context;

	MD5Init (&context);
	MD5Update (&context, (byte *)pass, (unsigned)strlen (pass));
	MD5Final (md5_val, &context);

	bzero (&context, sizeof (context));
}

/* Produce hex values for PGP Tools error messages. Could be a macro,
   though that would break hex_key() below. */

static	char	hex_val (int i)

{
	return (i < 10) ? ('0' + i) : ('A' + i - 10);
}

/* Convert a PGP key id to hex */

static	void	hex_key (char *p, byte *key)

{
	int	i;

	/* Key is 64 bits */

	for (i = 0; i < 8 ; i++) {
		*p++ = hex_val ((*key & 0xF0) >> 4);
		*p++ = hex_val (*key++ & 0xF);
	}
}

/* 
   Decrypt a fifo; much of this routine is based on ptd.c in the PGP Tools 
   distribution. This function destroys the input fifo (inf).
*/

static	int	decrypt_fifo (struct fifo *inf, BUFFER *decrypted, 
			BUFFER *signature, char *pass, byte *md5_pass)

{
	int	ret_val;
#define STRING_SIZE	256
	char	buf [STRING_SIZE];
	byte	userid [STRING_SIZE];
	struct	fifo	*temp = NULL;
	struct	fifo	*keyring = NULL;
	struct	fifo	*mess = NULL;
	struct	pgp_seckey	secret;
	struct	pgp_pubkey	public;
	byte	sig_type, trust;
	time_t	timestamp, etimestamp;
	byte	key [IDEA_KEY_SIZE], phrase_md5 [MD5_SIZE];
	byte	idea_key [IDEA_KEY_SIZE];
	MD5_CTX	context;
	FILE	*skr;
	byte	got_a_key = FALSE;
	byte	got_a_sig = FALSE;
	byte	got_a_rsa = FALSE;
	int	text = FALSE;
	time_t	dummy_ts;
	byte	sig_md5 [MD5_SIZE], mess_md5 [MD5_SIZE];
	byte	*s;

	/* We build up an error message in this buffer containing the
	   user ids that the message is encrypted for. This is 
	   neccesary in case we don't have any of the keys. Hence
	   it must be freed if we abort at any point. */

	BUFFER	*cant_decrypt = NULL;

	/* Assume that it will decrypt OK */

	ret_val = SIG_NONE;

	while (1) {

		/* Process each packet in turn. PGP messages contain
		   numerous different packets, some appended to each
		   other and some contained inside other packets. To
		   deal with the latter case we replace inf with a
		   fifo containing the new packet and loop back, only
		   exiting when we find either a decrypted message or
		   an invalid packet. This gives us general-purpose code
		   but makes it a little messy. */

		switch (fifo_rget (0, inf) & 0xFC) {

			case PGP_CKE:

			/* IDEA packet; if we didn't get a key
			   from the RSA packet then we need a
			   passphrase. */

			if (!got_a_key) {

				/* Do we have a passphrase? */

				if (!pass && !md5_pass) {
					fifo_destroy (inf);
					if (cant_decrypt)
						free_buffer (cant_decrypt);
					return DEC_BAD_PHRASE;
				}
	
				/* If we have a passphrase we need to
				   take the MD5 value as a key. */

				if (!md5_pass) {
					MD5Init (&context);
					MD5Update (&context, 
						(byte *)pass, 
						(unsigned)strlen (pass));
					MD5Final (idea_key, &context);

					/* Tidy up */

					bzero (&context, 
						sizeof (context));
				}
				else {
					bcopy (md5_pass,
						idea_key,
						sizeof(idea_key));
				}
			}

			/* IDEA decrypt and verify */

			mess = fifo_mem_create ();
			if (!pgp_extract_idea (inf, mess, idea_key)) {

				/* Failed, so tidy up and return
				   an error */

				fifo_destroy (inf);
				fifo_destroy (mess);
				bzero (idea_key, IDEA_KEY_SIZE);

				if (!got_a_key && got_a_rsa) {
					add_to_buffer (decrypted,
						cant_decrypt->message,
						cant_decrypt->length);

					free_buffer (cant_decrypt);
					return DEC_NO_KEY;
				}

				if (cant_decrypt)
					free_buffer (cant_decrypt);

				return DEC_BAD_PHRASE;
			}

			/* Yay ! Decrypted it */

			/* We no longer need the error message */

			if (cant_decrypt) {
				free_buffer (cant_decrypt);
				cant_decrypt = NULL;
			}

			/* And can clear the key */

			bzero (idea_key, IDEA_KEY_SIZE);

			/* Next time round the loop. Point inf
			   to the decrypted data. */

			fifo_destroy (inf);
			inf = mess;
			break;

			/* RSA block */

			case PGP_PKE:

			/* Note that we found an RSA block */

			got_a_rsa = TRUE;

			mess = fifo_copy (inf);

			/* Get the PGP key id */

			pgp_get_keyid (inf, key);

			/* Open the secret key ring */

#ifdef USE_FLOPPY
			skr = get_flop_file ();
			rewind (skr);
#else
			skr = open_pgp_file ("secring.pgp", "rb");
#endif

			if (!skr) {

				/* Can't find the keyring, so return */

				bzero (key, IDEA_KEY_SIZE);
				fifo_destroy (inf);
				if (cant_decrypt)
					free_buffer (cant_decrypt);

				return SIG_NO_KEY;
			}

			keyring = fifo_file_create (skr);

			/* Try to find the key in the keyring */

			if (!(temp = pgpk_findkey (keyring, key, FALSE))) {
				int32	length;
				byte	t;
				int	c;

				/* Ok, it's not in the secret key
				   file. */

				fifo_destroy (keyring);
#ifndef USE_FLOPPY
				fclose (skr);
#endif

				/* Add the information to the
				   failure message */

				if (!cant_decrypt) {
					cant_decrypt = new_buffer ();
					add_to_buffer (cant_decrypt,
						rsa_failed,
						strlen (rsa_failed));
				}

				/* If we can find the public key
				   then we can find the name of the
				   user this message is encrypted
				   for */

				if (get_public_key (key, userid,
					&public, &trust)) {
					add_to_buffer (cant_decrypt,
						userid, 
						strlen (userid));
					add_to_buffer (cant_decrypt,
						retstring,
						strlen (retstring));
				}
				else {

					/* Otherwise we have to use
					   the numeric id */

					strcpy (buf, "KeyID: ");
					c = strlen (buf);

					hex_key (buf + c, key);
					buf[c+16] = '\n';
					buf[c+17] = '\t';

					add_to_buffer (cant_decrypt,
						buf, c+18);
				}

				/* Tidy up */

				bzero (key, IDEA_KEY_SIZE);

				fifo_destroy (inf);
				inf = mess;

				/* Need to skip to next packet */

				pgp_examine_packet (inf, &t, 
					(word32 *)&length);
				fifo_skipn (inf, length);

				/* We don't return an error as
				   there may be another RSA
				   packet which we can decrypt */

				break;
			}

			fifo_destroy (mess);

			/* Now we need to extract the key */

			if (pgpk_extract_key (temp, &public,
				&secret, NULL, NULL)) {

				/* If it's encrypted and we don't
				   have a passphrase, then return */

				if (!pass && !md5_pass) {
					bzero (&secret, sizeof (secret));
					bzero (&public, sizeof (public));
					bzero (key, IDEA_KEY_SIZE);
					fifo_destroy (temp);
					fifo_destroy (keyring);
					fifo_destroy (inf);
#ifndef USE_FLOPPY
					fclose (skr);
#endif

					if (cant_decrypt)
						free_buffer (cant_decrypt);

					return DEC_BAD_PHRASE;
				}

				/* Calculate the MD5 of the passphrase */

				if (!md5_pass) {
					MD5Init (&context);
					MD5Update (&context, (byte *)pass, 
						(unsigned)strlen (pass));
					MD5Final (phrase_md5, &context);
				}
				else
					bcopy (md5_pass,
						phrase_md5,
						sizeof (phrase_md5));

				/* Tidy up */

				bzero (&context, sizeof (context));
				fifo_destroy (temp);

				/* Decrypt the secret key */

				if (!pgp_decrypt_sk (&secret, phrase_md5)) {

					/* Incorrect passphrase */

					fifo_destroy (keyring);
#ifndef USE_FLOPPY
					fclose (skr);
#endif

					/* Tidy up */

					bzero (phrase_md5, MD5_SIZE);
					bzero (&secret, sizeof (secret));
					bzero (&public, sizeof (public));
	
					if (cant_decrypt)
						free_buffer (cant_decrypt);

					/* Return error */

					return DEC_BAD_PHRASE;
				}

				/* Right, ready to go */

				bzero (phrase_md5, MD5_SIZE);
			}

			/* Ok, we have the key */
			
			fifo_destroy (keyring);
#ifndef USE_FLOPPY
			fclose (skr);
#endif

			if (!pgp_extract_rsa (inf, idea_key, &public,
				&secret)) {
				fifo_destroy (inf);

				bzero (idea_key, IDEA_KEY_SIZE);
				bzero (&secret, sizeof (secret));
				bzero (&public, sizeof (public));

				if (cant_decrypt)
					free_buffer (cant_decrypt);

				return DEC_BAD_FILE;
			}

			bzero (&secret, sizeof (secret));
			bzero (&public, sizeof (public));

			got_a_key = TRUE;

			break;

			/* Signature */

			case PGP_SIG:

			pgp_get_keyid (inf, key);

			if (!get_public_key (key, userid, &public,
				&trust)) {
				int32	length;
				byte	t;

				ret_val = SIG_NO_KEY;

				/* Need to skip to next packet */

				pgp_examine_packet (inf, &t, 
					(word32 *)&length);
				fifo_skipn (inf, length);

				break;
			}

			bzero (key, IDEA_KEY_SIZE);

			if (!pgp_extract_sig (inf, sig_md5, &timestamp,
				&sig_type, &public)) {

				fifo_destroy (inf);
				bzero (idea_key, IDEA_KEY_SIZE);
				bzero (sig_md5, MD5_SIZE);
				bzero (&timestamp, 4);
				bzero (&public, sizeof (public));

				if (cant_decrypt)
					free_buffer (cant_decrypt);

				return DEC_BAD_FILE;
			}

			got_a_sig = TRUE;
			bzero (&public, sizeof (public));

			break;

			case PGP_CMP:

			/* Compressed */

			temp = fifo_mem_create ();
			pgp_extract_zip (inf, temp);
			inf = temp;
			break;

			case PGP_PT:

			/* Plaintext; this will be the final layer, so 
			   we can destroy inf early on. */

			temp = fifo_mem_create ();
			pgp_extract_literal (inf, temp, &text, buf,
				&dummy_ts);
			fifo_destroy (inf);

			/* Don't need the error message any more,
			   so free it */

			if (cant_decrypt) {
				free_buffer (cant_decrypt);
				cant_decrypt = NULL;
			}

			/* Check signature if we found one */

			if (got_a_sig) {
				MD5Init (&context);
				pgp_md5 (fifo_copy (temp), &context);
				MD5Update (&context, &sig_type, 1);

				etimestamp = timestamp;
				endian (&etimestamp, 4);

				MD5Update (&context, (byte *) &etimestamp, 4);
				MD5Final (mess_md5, &context);

				bzero (&context, sizeof (context));

				/* Verify signature and create return info */

				if (memcmp (mess_md5, sig_md5, MD5_SIZE)) {
					add_to_buffer (signature, bad_sig,
						strlen (bad_sig));
					sprintf (buf, "%s\".", userid);
					add_to_buffer (signature, buf,
						strlen (buf));

					ret_val = SIG_BAD;
				}
				else {
					add_to_buffer (signature, good_sig,
						strlen (good_sig));

					sprintf(buf, "\"%s\".\nSignature made ",
						userid);
					s =(byte *) ctime (&timestamp);

					add_to_buffer (signature, buf, 
						strlen (buf));
					add_to_buffer (signature, s, 
						strlen (s) - 1);

					ret_val = SIG_GOOD;
				}

				bzero (mess_md5, MD5_SIZE);
				bzero (sig_md5, MD5_SIZE);
			}

			if (text) {
				mess = fifo_mem_create ();
				pgp_textform (temp, mess, FALSE, TRUE);
			}
			else
				mess = temp;

			fifo_to_buffer (mess, decrypted);
			fifo_destroy (mess);

			return ret_val;

			/* Any other packets are currently undefined
			   (see pgformat.doc). Tidy up and exit. */

			default:
			if (cant_decrypt)
				free_buffer (cant_decrypt);
			return DEC_BAD_FILE;
		}
	}
}
#endif

/* 
   Following should be called from user interface code whenever the
   user 'randomly' interacts with the program, to add random bits to 
   the random number seed. It takes the time it was called, 'compresses' 
   it, then xors the result into the random seed data 

   Since the 256-bytes of random data are hashed down to 16 bytes prior 
   to use, I think that it's OK to use eight bits of time per call. You 
   may want to reduce that.

   This is a simple no-op if we're not using PGP Tools, because we don't
   need it. Saves #ifdefs in the calling code.
*/

void	update_random(void)

{
#ifdef PGPTOOLS
	byte	low_byte;
	struct	timeb	timestamp;

	/* Millitm probably isn't correct, but this XOR is unlikely
	   to make things worse than using just the seconds! */

	ftime (&timestamp);

	/* Take low byte of time since 1970. */

	low_byte = (timestamp.time & 0xFF);

	/* Shift off bottom two bits of millisecond time to scale to 
	   0-250, and XOR in. The bottom bits can't be trusted anyway. */

	low_byte ^= (timestamp.millitm >> 2);

	/* Xor resulting byte into random seed. */

	add_to_random (&low_byte, 1);

	/* Clear it all just in case anyone's looking. */

	low_byte = 0;
	bzero (&timestamp, sizeof (timestamp));
#endif
}

/* This should be the first routine called, so that PGP Tools is set
   up properly before use. If we're using PGP it's a no-op. */

void	init_pgplib(void)

{
#ifdef PGPTOOLS
	FILE	*fp;

	/* Set mpilib precision */

	set_precision (MAX_UNIT_PRECISION);

	/* Init hash table */

#ifdef USE_HASH
	pgp_hash_clear ();
#endif

	/* Setup random number seeding, by reading from privseed file */

	if (fp = open_pgp_file ("privseed.bin", "rb")) {
		fread (privseed, RAND_SIZE, 1, fp);
		fclose (fp);
	}

	/* We can get a few more bits of randomness by using the
	   audio device here. We don't care too much, so ignore the
	   return code. */

#ifdef USE_AUDIO
	(void) merge_in_audio (privseed);
#endif

	/* If we didn't find the file, running it through the
	   IDEA cipher will provide us some slightly random output. This
	   will also give us a few bits if the file has been compromised. */

	idea_privseed(FALSE);

	/* We xor randseed.bin in just for luck (if we find it) */

	if (fp = open_pgp_file ("randseed.bin", "rb")) {
		byte	randseed[24];

		/* Randseed.bin is always a multiple of 24 bytes. Size
		   depends on PGP version. */

		while (fread (randseed, 24, 1, fp))
			add_to_random (randseed, 24);
		bzero (randseed, 24);
		fclose (fp);
	}

	/* Finally, add in data from the /proc files. */

#ifdef linux
	add_proc_data ();
#endif
#endif
}

/* Here we close down the code and tidy up sensitive data. */

void	close_pgplib(void)

{
#ifdef PGPTOOLS
	FILE	*fp;
	byte	old_random [RAND_SIZE];
	int	i;

	/* Close hash table */

#ifdef USE_HASH
	pgp_hash_off ();
#endif

	/* IDEA privseed array before dumping to disk */

	idea_privseed(FALSE);

	/* Possibly mix in some audio data. We only do this when 
	   MOST_SECURE is defined because it can take a couple of
	   seconds. We don't really care if the call fails. */

#ifdef MOST_SECURE
#ifdef USE_AUDIO
	(void) merge_in_audio (privseed);
#endif
#endif

	/* XOR in the old file, in case any other program has been
	   using it while we're running. Even if we start up and
	   exit in a few seconds, the output should be different
	   enough from the input for this to be safe. 

	   Just in case, with #ifdef MOST_SECURE we will only do
	   this if init_rand is set (implying that we put some
	   randomish data in the buffer). */

#ifdef MOST_SECURE
	if (init_rand)
#endif
	if ((fp = open_pgp_file ("privseed.bin", "rb"))) {
		fread (old_random, RAND_SIZE, 1, fp);
		for (i = 0; i < RAND_SIZE; i++)
			if (old_random [i] != privseed [i]) {
				privseed [i] ^= old_random [i];
			}

		fclose (fp);
	}

	/* Finally, dump privseed to privseed.bin. We ignore errors,
	   because we can't really do anything about them. */

	if ((fp = open_pgp_file ("privseed.bin", "wb"))) {
		fwrite (privseed, RAND_SIZE, 1, fp);
		fclose (fp);
	}
#endif

	/* Eject floppy disk on exit if neccesary. This isn't really 
	   useful without PGP Tools unless you link secring.pgp to
	   /dev/floppy (or whatever) */

#if defined(USE_FLOPPY) && defined(AUTO_EJECT)
	eject_floppy();
#endif

#if defined(GDBM) || defined (BDB)
	if (keyfile) {
#ifdef GDBM
		gdbm_close (keyfile);
#else
		keyfile->close (keyfile);
#endif
		keyfile = 0;
	}
#endif
}

/* Routines to deal with running other programs, such as PGP */

/* Clear all the output buffers */

static	void	clear_output(void)

{
	clear_buffer(&error_messages);
	clear_buffer(&stdout_messages);
}

/* Following routines just save us one parameter; could be made into
   macros */

/* Add a string to the stdout buffer. */

static	void	add_to_std(byte *buf, int len)

{
	add_to_buffer(&stdout_messages,buf,len);
}

/* Add a string to the error messages buffer */

static	void	add_to_error(byte *buf, int len)

{
	add_to_buffer(&error_messages,buf,len);
}

/* Run a specified program with the appropriate arguments, and pass in
   the specified data. If we're running PGP then we may also need to 
   give it a passphrase */

int	run_program(char *prog, byte *message, int msg_len,
		char **args, char *pass, BUFFER *ret)

{
	int	fd_in[2],fd_err[2],fd_out[2],pass_fd[2];
	int	from_pgp,to_pgp,pgp_error,pass_in;
	int	child_pid;
	fd_set	r_fdset,w_fdset;
#ifndef SYSV
	struct	rusage	rusage;
#endif
	int	statusp;
	int	fds_found;
	char	buf[BUF_SIZE];
	int	size;
#ifdef SYSV
	struct	timeval	t, to;

	/* We set the default timeout to 1 second */

	t.tv_sec = 1;
	t.tv_usec = 0;
#endif

	/* Clear the buffers */

	clear_output();

	/* We need some pipes to communicate with the other
	   process. We only need an input pipe if we have data
	   to pass through to the program. */

	if (message)
		pipe (fd_in);
	pipe (fd_err);
	pipe (fd_out);

	/* If we have a passphrase then we need a pipe for it */

	if (pass)
		pipe (pass_fd);

	/* Fork and execute the program */
	if (!(child_pid = vfork())) {
		char	pass_env[32];

		/* Duplicate our pipe files on stdin/stdout/stderr */

		close (0);
		close (1);
		close (2);

		if (message)
			dup2 (fd_in[0],0);
		dup2 (fd_out[1],1);
		dup2 (fd_err[1],2);

		/* We should really close all file descriptors here,
		   but we shouldn't have any sensitive files open when
		   called. XView could be a problem, but everything
		   currently works. */

		if (message)
			close (fd_in[1]);
		close (fd_out[0]);
		close (fd_err[0]);

		/* Point PGPPASSFD to the descriptor that we will be
		   writing the passphrase on, if neccesary. This is
		   more secure than putting the passphrase itself into 
		   an environment variable (try ps -e). */

		if (pass) {
			close (pass_fd[1]);
			sprintf	(pass_env,"PGPPASSFD=%d\n",pass_fd[0]);
			putenv (pass_env);
		}

		/* Exec and return an error if it fails */

		if (execvp (prog, args)<0) {
			_exit(-1);
		}

		exit(1);
	}

	/* If fork failed then return error */

	if (child_pid < 0)
		return -1;

	/* Close the other ends of the pipes */

	if (message)
	close (fd_in[0]);
	close (fd_out[1]);
	close (fd_err[1]);

	/* Make some aliases to save typing and clarify the code */

	if (message)
		to_pgp = fd_in[1];
	else
		to_pgp = (-1);
	from_pgp = fd_out[0];
	pgp_error = fd_err[0];

	/* Same for the passphrase pipe */

	if (pass) {
		pass_in = pass_fd[1];
		close (pass_fd[0]);
	}
	else
		pass_in = (-1);

	/* Now, we look around and check for input until the process
	   exits */

#ifndef SYSV
	while (!wait4(child_pid,&statusp,WNOHANG,&rusage)) {
#else
	while (!waitpid (child_pid, &statusp, WNOHANG)) {
#endif

		/* Set things up for the select call. Only include the
		   file descriptors which are still open. */

		FD_ZERO(&r_fdset);
		FD_SET(from_pgp, &r_fdset);
		FD_SET(pgp_error, &r_fdset);

		FD_ZERO(&w_fdset);
		if (to_pgp >= 0) {
			FD_SET(to_pgp,&w_fdset);
		}

		if (pass && pass_in >= 0) {
			FD_SET (pass_in, &w_fdset);
		}

		/* See which descriptors are available */

#ifndef SYSV
		fds_found = select(getdtablesize(),&r_fdset,&w_fdset,0,0);
#else
		/* SYSV select changes the timeout value! */

		to = t;
		fds_found = select(FD_SETSIZE,&r_fdset,&w_fdset,0,&to);
#endif

		/* Did we find any, or time out? */

		if (fds_found > 0) {

			/* First check for error messages and read a buffer
			   full */

			if (FD_ISSET(pgp_error,&r_fdset)) {
				size = read(pgp_error,buf,BUF_SIZE);

				/* Save it for the calling code */

				if (size > 0) {
					add_to_error(buf,size);
				}
			}

			/* Send the passphrase */

			if (pass_in >= 0 && FD_ISSET (pass_in,&w_fdset)) {
				if (*pass) {
					size = write(pass_in,pass,strlen(pass));
					pass += size;
				}

				/* If we sent the entire passphrase then
				   close the descriptor. */

				if (!*pass) {
					write (pass_in,"\n",1);
					close (pass_in);
					pass_in = (-1);

					/* To be secure, we should destroy
					   the passphrase here */
#ifdef MORE_SECURE
					destroy_passphrase(FALSE);
#endif

				}
			}

			/* If we have data to send, then send some */

			if (to_pgp >= 0 && FD_ISSET(to_pgp,&w_fdset)) {
				size = write(to_pgp,message,msg_len);

				if (size>0) {
					message += size;
					msg_len -= size;
				}

				/* Again, if we sent it all then close
				   the descriptor so the program gets
				   an EOF */

				if (msg_len <= 0) {
					close(to_pgp);
					to_pgp = (-1);
				}
			}

			/* Read some data from the program */

			if (FD_ISSET(from_pgp,&r_fdset)) {
				size = read(from_pgp,buf,BUF_SIZE);

				/* And store it away for the caller */
	
				if (size > 0) {
					add_to_std(buf,size);
				}
			}
		}
	}

	/* Just in case */

#ifdef MORE_SECURE
	if (pass)
		destroy_passphrase(FALSE);
#endif

	/* Read remaining stdout data */

	do {
		size = read (from_pgp,buf,BUF_SIZE);
		if (size > 0) {
			add_to_std(buf, size);
		}
	} while (size > 0);

	/* Close it */

	close (from_pgp);

	/* Read remaining error info */

	do {
		size = read (pgp_error,buf,BUF_SIZE);
		if (size>0) {
			add_to_error(buf, size);
		}
	} while (size > 0);

	/* Close it */

	close (pgp_error);

	/* Close remaining open files */

	if (to_pgp >= 0)
		close (to_pgp);

	if (pass_in >= 0)
		close (pass_in);

	/* Copy the stdout_message data into the ret buffer,
	   so that the caller can read it directly. Some GUI code calls
	   this function and doesn't have access to the static data
	   in this module */

	if (ret)
		add_to_buffer (ret, stdout_messages.message,
			stdout_messages.length);

	/* Finally, return the exit code */

	return statusp;
}

/* This routine runs PGP with the specified arguments. Currently it
   doesn't check that the program exists; run_program will return
   -1 if so. */

#ifndef PGPTOOLS
static	int	run_pgp(byte *message, int msg_len, char **args,
			char *pass)

{
	return run_program(pgp_path(),message,msg_len,args,pass,NULL);
}

/* Standard arguments for PGP */

static	char	*filter_argv[]={
	"pgp",
	"-f",
	"+batchmode",
	"+language=en",
	NULL
};

/* Message to tell the user that decryption failed. */

static	char	fail_string[] = "\n\n******* DECRYPTION FAILED *******\n\n";

/* Longest that we expect an output line from PGP to be. Nothing
   too horrible will happen if this is too small, you may just
   get invalid signature messages. */

#define LINE_LENGTH	1024
#endif

/* Decrypt a PGP message. */

int	decrypt_message (BUFFER *message, BUFFER *decrypted, BUFFER *signature,
		char *pass, int flags, byte *md5_pass)

{
	int	ret_val;
#ifdef PGPTOOLS
#define STRING_SIZE	256
	char	buf [STRING_SIZE];
	byte	userid [STRING_SIZE];
	int	l;
	struct	fifo	*inf = NULL;
	struct	fifo	*temp = NULL;
	struct	fifo	*sig = NULL;
	struct	fifo	*mess = NULL;
	struct	fifo	*tmess = NULL;
	struct	pgp_pubkey	public;
	byte	sig_type, trust;
	time_t	timestamp, etimestamp;
	MD5_CTX	context;
	byte	sig_md5 [MD5_SIZE], mess_md5 [MD5_SIZE];
	byte	key [IDEA_KEY_SIZE];
	byte	*s, *e;
	byte	*t, *m;

	/* Either clearsigned or ASCII-armored */

	if (flags & FL_ASCII) {

		s = message->message;
		l = message->length;

		/* Skip the white space */

		while (l && isspace(*s)) {
			l--;
			s++;
		}

		/* Now, is it signed ? */

		if (!strncmp (s, begin_signed, strlen(begin_signed) - 1)) {

			/* Yep, clearsigned; let's process it! */

			/* Start by skipping header lines */

			for (;l && *s != '\n';s++, l--);
			for (s++;l && *s != '\n'; s++, l--);
			s++;

			/* We need to extract the signature from the
			   armor. */

			temp = fifo_mem_create ();
			fifo_aput (s, l, temp);

			fifo_find ((byte *)"\n-----BEGIN PGP SIG", temp);
			fifo_find ((byte *)"Version", temp);
			fifo_find ((byte *)"\n\n", temp);

			/* Now we have the armor itself, extract the binary
			   data. */

			sig = fifo_mem_create ();
			pgp_extract_armor (temp, sig);
			fifo_destroy (temp);

			/* Ok, now we have the sig. We now need to check
			   it matches the file. */
			
			if ((fifo_rget (0, sig) & 0xFC) != PGP_SIG) {

			/* Gotos may be the tools of Satan, but this
			   saves some duplicated code because we have
			   to free off data before exiting. */

			bad_sig_bad_file:

				/* Dunno what sort of file this is, copy
				   the entire message over and return an
				   error */

				fifo_destroy (sig);
				add_to_buffer (decrypted, message->message,
					message->length);
				return DEC_BAD_FILE;
			}

			/* Look for the begin signature line */

			e = (byte *)strstr (s, begin_signature);

			if (!e)
				goto bad_sig_bad_file;

			/* Extract the key id from the signature */

			pgp_get_keyid (sig, key);

			/* And find the public key */
			
			if (!get_public_key (key, userid, &public, &trust)) {

				/* If we didn't get it, then copy the message
				   over after converting the dashes. */

				if (!strncmp("- ", s, 2))
					s+= 2;

				t = s;

				while (t < e) {
					m = (byte *)strstr (t, "\n- ");

					if (!m || m > e) {
						add_to_buffer (decrypted,
							t, (e - t));
						break;
					}
					add_to_buffer (decrypted, 
						t, (m - t) + 1);
					t = m + 3;
				}

				/* We return an error message in the signature
				   buffer, giving the id of the key in hex */

				add_to_buffer (signature, no_key,
					strlen (no_key));

				hex_key (buf, key);
				buf[16] = '.';
				add_to_buffer (signature, buf, 17);

				/* Return error */

				fifo_destroy (sig);
				return SIG_NO_KEY;
			}

			/* Now split up the signature into the appropriate
			   elements */

			if (!pgp_extract_sig (sig, sig_md5, &timestamp,
				&sig_type, &public)) 
				goto bad_sig_bad_file;

			/* Ok, we got the sig, now calculate the MD5 of the
			   message */

			fifo_destroy (sig);
			mess = fifo_mem_create ();

			/* OK, we have to loop through copying the data over,
			   and removing any '-' at the beginning of a line.
			   Blurgh! */

			if (!strncmp("- ", s, 2))
				s+= 2;

			t = s;

			while (t < e) {
				m = (byte *)strstr (t, "\n- ");

				if (!m || m > e) {
					fifo_aput (t, (e - t), mess);
					break;
				}
				fifo_aput (t, (m - t) + 1, mess);
				t = m + 3;
			}

			/* If it's a 'text' signature then we need to convert
			   to the standard text format */

			if (sig_type) {
				tmess = fifo_mem_create ();
				pgp_textform (fifo_copy (mess), tmess, TRUE, 
					TRUE);
			}

			/* Ok, copy to the decrypted message buffer */

			fifo_to_buffer (mess, decrypted);
			fifo_destroy (mess);

			etimestamp = timestamp;
			endian (&etimestamp, 4);

			/* Create the MD5 of the message */

			MD5Init (&context);
			pgp_md5 (tmess, &context);
			MD5Update (&context, &sig_type, 1);
			MD5Update (&context, (byte *) &etimestamp, 4);
			MD5Final (mess_md5, &context);

			/* Tidy up and pass the MD5 to the random number
			   generator just for luck */

			bzero (&context, sizeof (context));
			add_to_random (mess_md5, MD5_SIZE);

			/* Now verify */

			if (memcmp (mess_md5, sig_md5, MD5_SIZE)) {

				/* Return an error */

				add_to_buffer (signature, bad_sig,
					strlen (bad_sig));
				sprintf (buf, "%s\".", userid);
				add_to_buffer (signature, buf,
					strlen (buf));

				ret_val = SIG_BAD;
			}
			else {

				/* Ok, we got the right signature. Output
				   the details into the signature buffer. */

				add_to_buffer (signature, good_sig,
					strlen (good_sig));

				sprintf(buf, "\"%s\".\nSignature made ",
					userid);
				s =(byte *) ctime (&timestamp);

				add_to_buffer (signature, buf, strlen (buf));
				add_to_buffer (signature, s, strlen (s) - 1);

				ret_val = SIG_GOOD;
			}

			/* Clear everything for luck */

			bzero (&public, sizeof (public));
			bzero (key, IDEA_KEY_SIZE);
			bzero (mess_md5, MD5_SIZE);
			bzero (sig_md5, MD5_SIZE);
			bzero (userid, STRING_SIZE);

			timestamp = etimestamp = 0;

			/* And return success or error */

			return ret_val;
		}

		/* Is the entire message ASCII-armored? */

		if (!strncmp (s, begin_armour, strlen (begin_armour))) {

			/* Copy to memory temporarily */

			temp = fifo_mem_create ();
			fifo_aput (s, l, temp);

			/* Ok, it's armored, process it */

			fifo_find ((byte *)"Version", temp);
			fifo_find ((byte *)"\n\n", temp);

			/* Extract the armored file */

			inf = fifo_mem_create ();

			pgp_extract_armor (temp, inf);
			fifo_destroy (temp);
		}
	}

	/* Otherwise the message is binary so just copy it over. This will
	   also deal with the case where we fell out of the code above. */

	if (!inf) {
		inf = fifo_mem_create ();
		fifo_aput (message->message, message->length, inf);
	}

	/* Now do the hard work */
		
	return decrypt_fifo (inf, decrypted, signature, pass, 
		md5_pass);
#else

	/* If we don't have PGP Tools then this is much simpler */

	char	*s,*e;
	char	line[LINE_LENGTH+1];
	int	j,c;
	int	pgp_ret;
	int	sig_lines = 0;
	int	i;

	/* Run PGP */

	pgp_ret = run_pgp(message->message,message->length,filter_argv,pass);

	ret_val = SIG_GOOD;

	/* Interpret PGP exit codes */

	switch (pgp_ret) {
		case EXIT_OK:
		break;

		/* We can't simulate all the following, so fake it */

		case INVALID_FILE_ERROR:
		case FILE_NOT_FOUND_ERROR:
		case UNKNOWN_FILE_ERROR:
		case NO_BATCH:
		case BAD_ARG_ERROR:
		case INTERRUPT:
		case OUT_OF_MEM:
		case KEYGEN_ERROR:
		case KEYRING_ADD_ERROR:
		ret_val = DEC_BAD_FILE;

		case NONEXIST_KEY_ERROR:
		ret_val = SIG_NO_KEY;

		/* -1 means we couldn't execute PGP */

		case -1:
		ret_val = DEC_NO_PGP;
	}

	/* We check the error messages just in case we didn't pick
	   up the return code for some reason. */

	if (error_messages.message && 
		strstr((char *)error_messages.message,"WARNING")) {

		/* Ignore 'not certified' warnings, unless
		   accompanied by 'Bad signature' */

		if (!strstr((char *)error_messages.message,"not certified") ||
			strstr((char *)error_messages.message,
			"Bad signature, doesn't match"))
			ret_val = SIG_BAD;

		/* Oops, we don't have the public key to check the
		   signature! */

		if (strstr((char *)error_messages.message,
			"Can't find the right"))
			ret_val = SIG_NO_KEY;
	}

	i=0;
	j=0;

	/* Process the output, looking for the signature lines */

	do {
		c = line[j++] = error_messages.message[i++];
		if (c == '\n' || j == LINE_LENGTH || 
			i == error_messages.length) {
			line[j] = 0;

			/* Ignore warnings about low confidence */

			if (strstr(line,"ignature") && 
				!strstr(line,"onfidence") &&
				!strstr(line,"equired")) {
				add_to_buffer(signature,line,j);
				sig_lines ++;
			}
			j = 0;
		}
	} while (i<error_messages.length);

	/* If we didn't find any lines, it wasn't signed */

	if (!sig_lines)
		ret_val = SIG_NONE;

	/* Copy the decrypted message to the buffer */

	add_to_buffer(decrypted, stdout_messages.message,
		stdout_messages.length);

	/* Do we not have the secret key ? */

	if (strstr((char *)error_messages.message,"not have the secret")) {
		ret_val = DEC_NO_KEY;

		add_to_buffer (decrypted, (byte *)fail_string, 
			strlen(fail_string));

		/* Ok, let them know who the message is for! */

		s = strstr ((char *)error_messages.message, 
			"This message can only");
		if (s) {

			/* End of error message is marked with two
			   newlines */

			e = strstr (s, "\n\n");

			if (e) 
				add_to_buffer (decrypted, (byte *)s, 
					(e - s) + 1);
		}
	}

	/* Finally, check for bad passphrase */

	if (strstr((char *)error_messages.message,"Bad pass")) {
		ret_val = DEC_BAD_PHRASE;
	}

	return ret_val;
#endif
}

/* Note: user is a list of users, not just one! */

int	encrypt_message(char **user, BUFFER *message, BUFFER *encrypted,
		int flags, char *pass, char *key_name, byte *md5_pass)

{
#ifdef PGPTOOLS
	FILE	*skr;
	FILE	*pkr;
	FILE	*smkr;
	struct	fifo	*keyring;
	struct	fifo	*secret_key;
	struct	fifo	*public_key;
	struct	fifo	*signature;
	struct	fifo	*armoured;
	struct	fifo	*mess;
	struct	fifo	*tmess;
	struct	fifo	*outmess;
	struct	fifo	*smallring;
	struct	pgp_seckey	secret;
	struct	pgp_pubkey	public;
	byte	phrase_md5 [MD5_SIZE];
	byte	mess_md5[MD5_SIZE];
	byte	key[IDEA_KEY_SIZE];
	MD5_CTX	context, sig_context;
	time_t	timestamp, etimestamp;
	byte	text = 0, signature_type = 0;
	int	i;
	int	revoked;
	byte	good_key;

	/* If we're encrypting, open pubring.pgp */

	if (flags & FL_ENCRYPT) {
		smkr = open_pgp_file ("smallring.pgp", "rb");
		pkr = open_pgp_file ("pubring.pgp", "rb");
		if (!pkr)
			return ERR_NO_KEY;
	}

	/* If we're going to have to sign anything, get the secret key */

	if (flags & FL_SIGN) {

		/* Oops, no passphrase */

		if (!pass && !md5_pass) {
			if (pkr)
				fclose (pkr);
			if (smkr)
				fclose (smkr);

			return ERR_BAD_PHRASE;
		}

		/* Open the secret key file */

#ifdef USE_FLOPPY
		skr = get_flop_file ();
		rewind (skr);
#else
		skr = open_pgp_file ("secring.pgp", "rb");
#endif

		/* Check the file is there ! */

		if (!skr) {
			if (pkr)
				fclose (pkr);
			if (smkr)
				fclose (smkr);

			return ERR_NO_SECRET_KEY;
		}

		/* Read the secret key */

		keyring = fifo_file_create (skr);
		secret_key = pgpk_findkey (keyring, (byte *)key_name, 
			(int)TRUE);
		fifo_destroy (keyring);

		/* Close the keyring. If our keyring is on a floppy
		   then we keep it open. */

#ifndef USE_FLOPPY
		if (skr)
			fclose (skr);
#endif

		/* Did we get it ? */

		if (!secret_key) {
			if (pkr)
				fclose (pkr);
			if (smkr)
				fclose (smkr);

			return ERR_NO_SECRET_KEY;
		}

		/* Extract the key */

		if (pgpk_extract_key (secret_key, &public, &secret,
			NULL, NULL)) {

			/* It's encrypted, so we need the passphrase */

			/* Calculate the MD5 of the passphrase. If the
			   user passed it in, this becomes a simple
			   copy. */

			if (!md5_pass) {
				MD5Init (&context);
				MD5Update (&context, (byte *)pass, (unsigned)strlen (pass));
				MD5Final (phrase_md5, &context);
			}
			else 
				bcopy (md5_pass,
					phrase_md5,
					sizeof (phrase_md5));

			bzero (&context, sizeof (context));

			/* Decrypt the secret key */

			if (!pgp_decrypt_sk (&secret, phrase_md5)) {

				/* Uh-oh, incorrect passphrase */

				fifo_destroy (secret_key);

				bzero (phrase_md5, MD5_SIZE);
				bzero (&secret, sizeof (secret));
				bzero (&public, sizeof (public));
	
				return ERR_BAD_PHRASE;
			}

			/* Right, ready to go */

			bzero (phrase_md5, MD5_SIZE);
		}

		/* Tidy up */

		fifo_destroy (secret_key);
	}

	/* Copy input message into fifo */

	mess = fifo_mem_create ();
	fifo_aput (message->message, message->length, mess);

	/* Do encryption */

	if (flags & FL_ENCRYPT) {

		text = signature_type = 0;
		outmess = fifo_mem_create ();

		/* Do the timestamp */

		time (&timestamp);

		/* We need to ensure it's the correct endianness */

		etimestamp = timestamp;
		endian (&etimestamp, 4);

		if (flags & FL_SIGN) {
			tmess = fifo_copy (mess);

			/* Create the MD5 for the signature */

			MD5Init (&sig_context);
			pgp_md5 (tmess, &sig_context);
			MD5Update (&sig_context, &signature_type, 1);
			MD5Update (&sig_context, (byte *)&etimestamp, 4);
			MD5Final (mess_md5, &sig_context);

			/* Add it to the random seed. This may be a little 
			   dangerous, so we'll make it optional. */

#ifndef MOST_SECURE
			add_to_random (mess_md5, MD5_SIZE);
#endif

			/* Tidy up */

			bzero (&sig_context, sizeof (sig_context));

			/* Create a signature in outmess */

			pgp_create_sig (outmess, mess_md5, timestamp, 
				signature_type, &public, &secret);

			/* We don't erase the MD5 here because we'll
			   need it later for the encryption key */
		}
		else {

			/* We need an md5, but don't have one, so create
			   it from the first few bytes of the message */

			i = message->length;
			if (i > 8192)
				i = 8192;

			MD5Init (&sig_context);
			MD5Update (&sig_context, message->message, i);
			MD5Update (&sig_context, (byte *)&etimestamp, 4);
			MD5Final (mess_md5, &sig_context);

			/* Add it to the random seed */

#ifndef MOST_SECURE
			add_to_random (mess_md5, MD5_SIZE);
#endif

			bzero (&sig_context, sizeof (sig_context));
		}

		/* Create a PGP plaintext packet, using a fake name */

		pgp_create_literal (mess, outmess, text, "dev.null",
			timestamp);

		/* Compress it, destroying outmess in the process */

		tmess = fifo_mem_create ();
		pgp_create_zip (outmess, tmess);

		/* Calculate the encryption key, using mess_md5 and
		   our_randombyte (). This is the same method that PGP
		   itself uses; to crack the key you'd need to know the
		   random input *and* the MD5. If you know the MD5 you
		   probably already know the message text. */

		do {

			/* Start with values from /dev/random if we have
			   it */
#ifdef DEV_RANDOM
			int	fd;
			int	cnt = IDEA_KEY_SIZE;
			int	rdsz;
#endif

			good_key = 0;

#ifdef DEV_RANDOM
			fd = open ("/dev/random", O_RDONLY);
			if (fd >= 0) {
				while (cnt > 0 && 
					(rdsz = read (fd, key + (IDEA_KEY_SIZE - cnt), cnt)) >= 0) {
					cnt -= rdsz;
				}
				close (fd);
			}
#endif

			/* We look for a good key by checking for the
			   zero nibbles in a weak key. If any of these
			   nibbles are non-zero then we have a good key. */
			   
			for (i = 0; i < IDEA_KEY_SIZE; i++) {
				key [i] ^= our_randombyte() ^ mess_md5[i];
				good_key |= (key[i] & idea_weak_key_mask[i]);
			}

			/* If it's a weak key then we try again. This should
			   never happen (one key in 2^96 is weak). */

		} while (!good_key);

		/* Clear mess_md5 */

		bzero (mess_md5, MD5_SIZE);

		/* Uuurgh! We can't use the hash table here because we
		   have the alphanumeric user id rather than the
		   64-bit numeric user id. */

		keyring = fifo_file_create (pkr);

		if (smkr)
			smallring = fifo_file_create (smkr);

		outmess = fifo_mem_create ();

		/* This is somewhat messy as we have to create an RSA
		   packet for each user we're encrypting for. This involves
		   lots of fifo copies. Luckily, for files this is 
		   basically just a structure copy. */

		for (i = 0; user[i] != NULL; i++) {
			struct	fifo	*kring;
			int	found = FALSE;

			/* Check the small keyring first, to give
			   best performance for common keys */

			if (smkr) {
				/* Note: We have to take a copy of the 
				   keyring fifo here because
				   pgpk_findkey() would mess it up. */

				kring = fifo_copy (smallring);
				public_key = pgpk_findkey (kring, 
					(byte *)user[i], TRUE);
				fifo_destroy (kring);

				/* Did we find it? */

				if (public_key)
					found = TRUE;
			}

			/* If it's not in smallring.pgp then try
			   pubring.pgp instead. */

			if (!found) {
				kring = fifo_copy (keyring);
				public_key = pgpk_findkey (kring, 
					(byte *)user[i], TRUE);
				fifo_destroy (kring);
			}

			/* Did we find it ? */

			if (!public_key) {

				/* Oh no, no key! Tidy up and abort */

				timestamp = etimestamp = 0l;
				bzero (key, IDEA_KEY_SIZE);

				fifo_destroy (outmess);
				fifo_destroy (mess);
				fifo_destroy (tmess);
				fifo_destroy (keyring);
				if (smkr) {
					fifo_destroy (smallring);
					fclose (smkr);
				}
				fclose (pkr);

				return ERR_NO_KEY;
			}

			/* Otherwise extract the key from the keyring and
			   create an RSA packet for it */

			pgpk_extract_key (public_key, &public, 
				NULL, NULL, &revoked);

			pgp_create_rsa (outmess, key, &public);
		}

		/* Destroy keyring and close pubring.pgp */

		if (smkr) {
			fifo_destroy (smallring);
			fclose (smkr);
		}

		fifo_destroy (keyring);
		fclose (pkr);

		/* IDEA encrypt to outmess using the generated key, destroys 
		   tmess */

		pgp_create_idea (tmess, outmess, key);

		/* Erase all records */

		timestamp = etimestamp = 0;
		bzero (key, IDEA_KEY_SIZE);

		/* We now have the encrypted message in outmess, so return it */

		if (flags & FL_ASCII) {

			/* Caller wants us to ASCII-armor the message */

			armoured = fifo_mem_create ();
			pgp_create_armor (outmess, armoured, 0);
			fifo_destroy (outmess);

			/* Copy the armored data to the output buffer,
			   with appropriate begin and end lines */

			add_to_buffer (encrypted, begin_armour,
				strlen (begin_armour));
			add_to_buffer (encrypted, pgp_version, 
				strlen (pgp_version));
			add_to_buffer (encrypted, two_n, strlen (two_n));

			fifo_to_buffer (armoured, encrypted);
			add_to_buffer (encrypted, end_armour,
				strlen (end_armour));

			/* Tidy up */

			fifo_destroy (armoured);
		}
		else {

			/* Otherwise just copy over the data */

			fifo_to_buffer (outmess, encrypted);
			fifo_destroy (outmess);
		}

		/* It worked! */

		return ERR_NONE;
	}

	/* Do we want a clearsigned message? */

	else if (flags == (FL_ASCII | FL_SIGN)) {
		BUFFER	*tempb;
		byte	*t, *e, *s, *s2;
		
		/* Put begin signed message line to output */

		clear_output();
		add_to_buffer (encrypted, begin_signed, strlen(begin_signed));

		/* Set text mode flags */

		text = signature_type = 1;

		/* Create textified message in tmess */

		outmess = fifo_copy (mess);
		tmess = fifo_mem_create ();
		pgp_textform (mess, tmess, TRUE, TRUE);

		/* We have to prepend - to lines starting with F or - */

		tempb = new_buffer ();
		fifo_to_buffer (outmess, tempb);
		fifo_destroy (outmess);

		t = tempb->message;
		e = tempb->message + tempb->length;

		/* Check the first character explicitly because it has
		   no preceding \n */

		if (*t == 'F' || *t == '-')
			add_to_buffer (encrypted, "- ", 2);

		while (t < e) {

			/* Look for both types of lines and process the
			   first */

			s = (byte *)strstr (t, "\n-");
			s2 = (byte *)strstr (t, "\nF");

			if (!s || (s2 && s2 < s))
				s = s2;

			/* Did we reach the end? */

			if (!s || s >= e) {
				add_to_buffer (encrypted, t, (e - t));
				t = e;
				break;
			}
			else {
				add_to_buffer (encrypted, t, (s - t));
				add_to_buffer (encrypted, "\n- ", 3);
				t = s + 1;
			}
		}

		/* Now we can destroy the temporary copy */

		free_buffer (tempb);
		
		/* Now that the message is in encrypted, add the begin 
		   signature line */

		add_to_buffer (encrypted, begin_signature, 
			strlen (begin_signature));
		add_to_buffer (encrypted, pgp_version, strlen (pgp_version));
		add_to_buffer (encrypted, two_n, strlen (two_n));

		/* Do the timestamp */

		time (&timestamp);

		/* Ensure it's network byte order */

		etimestamp = timestamp;
		endian (&etimestamp, 4);

		/* Create the MD5 for the signature */

		MD5Init (&sig_context);

		/* Remember, this call destroys tmess, so don't do it again! */

		pgp_md5 (tmess, &sig_context);
		MD5Update (&sig_context, &signature_type, 1);
		MD5Update (&sig_context, (byte *)&etimestamp, 4);
		MD5Final (phrase_md5, &sig_context);

		/* Add it to the random seed */

		add_to_random (phrase_md5, MD5_SIZE);

		/* Create a signature packet */

		signature = fifo_mem_create();
		pgp_create_sig (signature, phrase_md5, timestamp, 
			signature_type, &public, &secret);

		/* Armour it */

		armoured = fifo_mem_create ();
		pgp_create_armor (signature, armoured, 0);	

		/* Copy armoured signature to output buffer */

		fifo_to_buffer (armoured, encrypted);

		/* Finally tag end-signature line on there */

		add_to_buffer (encrypted, end_signature, strlen(end_signature));

		/* Destroy remaining fifos */
	
		fifo_destroy (armoured);
		fifo_destroy (signature);

		/* Zero out encryption stuff */

		timestamp = 0l;
		etimestamp = 0l;

		/* YAY ! WE DID IT ! Drop through to the exit code */
	}

	else if (flags & FL_SIGN) {

		/* Here we're creating a binary, signed, unencrypted message */

		text = signature_type = 0;

		tmess = fifo_copy (mess);
		outmess = fifo_mem_create ();

		/* Create the MD5 */

		MD5Init (&sig_context);
		pgp_md5 (tmess, &sig_context);
		MD5Update (&sig_context, &signature_type, 1);
		MD5Update (&sig_context, (byte *)&etimestamp, 4);
		MD5Final (mess_md5, &sig_context);

		/* Add it to the random seed and tidy up */

		add_to_random (mess_md5, MD5_SIZE);
		bzero (&sig_context, sizeof (sig_context));

		/* Create the signature packet */

		pgp_create_sig (outmess, mess_md5, timestamp, 
			signature_type, &public, &secret);

		bzero (mess_md5, MD5_SIZE);

		/* Create a PGP plaintext packet */

		pgp_create_literal (mess, outmess, text, "dev.null",
			timestamp);
		fifo_destroy (mess);

		/* Outmess is destroyed when the zip is created */

		pgp_create_zip (outmess, tmess);

		fifo_to_buffer (tmess, encrypted);
		fifo_destroy (tmess);

		/* Drop through to tidyup and exit code */
	}

	else {
		/* Didn't ask us to do anything! Just copy over */

		fifo_destroy (mess);
		add_to_buffer (encrypted, message->message, message->length);
	}

	/* Clear it all in case we used it; some of these variables
	   *will* contain sensitive data. */

	timestamp = 0l;
	etimestamp = 0l;

	bzero (phrase_md5, MD5_SIZE);
	bzero (&secret, sizeof (secret));
	bzero (&public, sizeof (public));
	bzero (&context, sizeof (context));
	bzero (&sig_context, sizeof (sig_context));

	/* It worked */

	return ERR_NONE;
#else
	int	ret_val;
	char	args[3][32]; /* Allow some room for expansion */
	char	**argv;
	int	argv_size = 10;
	int	arg = 2;

	/* First argument will always be -f */

	strcpy(args[0],"-f");

	argv = (char **) malloc (argv_size * sizeof(char *));

	/* Are we out of memory? */

	if (!argv)
		return ERR_NO_MEM;

	/* Set up arguments */

	argv[0]="pgp";
	argv[1]=args[0];

	if (flags & FL_ENCRYPT) {
		strcat(args[0],"e");
	}
	if (flags & FL_ASCII) {
		strcat(args[0],"a");
	}
	if (flags & FL_SIGN) {
		strcat(args[0],"s");
		if (!(flags & FL_ENCRYPT)) {
			strcat(args[0],"t");
			argv[arg++] = "+clearsig=on";
		}
	}

	/* Use batchmode and set language to english when we run. */

	argv[arg++]="+batchmode";
	argv[arg++]="+language=en";

	/* We now need to add the list of users. If we overrun the
	   allocated array we'll have to extend it. */

	if (flags & FL_ENCRYPT) {
		char	**u;

		u = user;

		while (*u) {

			/* Note that we need some space for the final
			   arguments */

			if (arg > (argv_size - 5)) {
				argv_size += 10;
				argv = (char **)realloc (argv, 
					argv_size * sizeof (char *));

				/* Return if we run out of memory */

				if (!argv)
					return ERR_NO_MEM;
			}

			argv[arg++] = *u++;	
		}
	}
	else if (arg > (argv_size - 4)) {
		/* Ensure we have space. Current code always will. */
		argv_size += 10;
		argv = (char **)realloc (argv, 
			argv_size * sizeof (char *));
		if (!argv)
			return ERR_NO_MEM;
	}

	/* If signing we have to pass the key name in too */

	if (flags & FL_SIGN) {
		argv [arg++] = "-u";
		argv [arg++] = key_name;
	}

	/* End it with a null */

	argv[arg++] = NULL;

	/* Zero return value */

	ret_val = ERR_NONE;

	/* And, finally, run the program */

	if (run_pgp(message->message,message->length,argv,pass) < 0)
		ret_val = DEC_NO_PGP;

	/* Copy the output to the buffer */

	add_to_buffer(encrypted,stdout_messages.message,
		stdout_messages.length);

	/* Process the error messages */

	if (error_messages.message) {

		/* Bad passphrase */

		if (strstr((char *)error_messages.message,"Error") && 
			strstr((char *)error_messages.message,"Bad"))
			ret_val = ERR_BAD_PHRASE;

		/* No secret key */

		if (strstr((char *)error_messages.message,"Signature error")&&
			strstr((char *)error_messages.message,"Keyring file")&&
			strstr((char *)error_messages.message,"not exist")) {
			ret_val = ERR_NO_SECRET_KEY;
		}

		/* Missing public key for a user */

		if (strstr((char *)error_messages.message,"not found")) {
			ret_val = ERR_NO_KEY;
		}
	}

	/* Tidy up and return */

	free (argv);
	
	return ret_val;
#endif
}

/* Look through the buffer to see if it contains an ASCII-armored PGP
   key. */

int	buffer_contains_key (BUFFER *b)

{
	char	*s;

	/* Return if the buffer is invalid */

	if (!b->message || !b->length)
		return FALSE;

	/* Look for the begin key message, first time ignoring the
	   linefeed at the beginning of the string. */

	if (!strncmp ((char *)b->message, begin_key + 1, 
		strlen (begin_key) - 1))
		return TRUE;

	/* Now search the whole buffer */

	s = strstr ((char *)b->message, begin_key);

	/* Did we find it? At the moment we just assume it's a key and
	   don't look for the end key line. We probably should. */

	if (s && s < (char *)(b->message + b->length))
		return TRUE;

	return FALSE;
}

/* Add a PGP key to the pubring.pgp file */

int	add_key (BUFFER *m)

{
	char	*s, *e;
	char	*dir;
#ifdef PGPTOOLS
	struct	fifo	*armor;
	struct	fifo	*new_key;
	struct	fifo	*n_kr;
	struct	fifo	*o_kr;
	struct	fifo	*temp;
	struct	pgp_pubkey	pk, pkt;
	byte	trust;
	char	in_file[PATH_MAX];
	char	out_file[PATH_MAX];
	char	back_file[PATH_MAX];
	FILE	*opr;
	FILE	*npr;
#else
	FILE	*temp_fp;
	char	temp_name [PATH_MAX];
	char	*argv[5];
	int	pgp_ret;
#endif

	/* They didn't give us a key! */

	if (!m->message || !m->length)
		return ADD_NO_KEY;

	/* Look for the begin key string. As above, we first check the
	   start of the message without the linefeed. Then we scan the
	   entire message with the full string. Probably should be a
	   shared subroutine, but we need the values of s and e. */

	if (!strncmp ((char *)m->message, begin_key + 1, 
		strlen (begin_key) - 1)) {
		s = (char *)m->message;
	}
	else
		s = strstr ((char *)m->message, begin_key);

	/* Ok, we don't have a key. */

	if (!s || s >= (char *)(m->message + m->length))
		return ADD_NO_KEY;

	/* Now look for the end key line. */

	e = strstr ((char *)m->message, end_key);

	/* Again, we don't have a key. */

	if (!e || e >= (char *)(m->message + m->length))
		return ADD_NO_KEY;

#ifdef PGPTOOLS
	s += strlen (begin_key);

	/* Skip the begin key line and search for the key data */

	while (*s && *s != '\n')
		s++;

	if (*s)
		s++;
	else
		return ADD_NO_KEY;

	/* We now have s pointing at the beginning of the armor */

	armor = fifo_mem_create ();
	fifo_aput (s, (e - s) + 1, armor);

	/* Extract a binary copy of the key from the armor. */

	new_key = fifo_mem_create ();
	if (!pgp_extract_armor (armor, new_key)) {

		/* If the extraction failed then we have a bad key. */

		fifo_destroy (armor);
		fifo_destroy (new_key);
		return ADD_BAD_KEY;
	}
	fifo_destroy(armor);

	/* Now find the public key directory. */

	dir = getenv ("PGPPATH");
	if (!dir) {
		fifo_destroy (new_key);
		return ADD_NO_FILE;
	}

	/* Create key filenames */

	sprintf(in_file, "%s/pubring.pgp", dir);
	sprintf(out_file, "%s/newring.pgp", dir);
	sprintf(back_file, "%s/pubring.bak", dir);

	/* Open the input file and the temporary file. */

	opr = fopen (in_file, "rb");
	npr = fopen (out_file, "wb");
	if (!npr) {
		if (opr)
			fclose (opr);
		fifo_destroy (new_key);
		return ADD_NO_FILE;
	}

	/* Ok, we can create the new file. */

	if (opr) {
		char	userid[STRING_SIZE];

		/* Check to see if the key already exists */

		temp = fifo_copy (new_key);
		pgpk_extract_key (temp, &pk, NULL, NULL, NULL);
		fifo_destroy (temp);
		if (get_public_key (pk.keyid, userid, &pkt, &trust)) {
			/* Key already exists */

			bzero (&pk, sizeof (pk));
			bzero (&pkt, sizeof (pkt));

			fifo_destroy (new_key);
			fclose (npr);
			fclose (opr);

			return ADD_OLD_KEY;
		}

		/* Ok, the key doesn't exist. */
		
		o_kr = fifo_file_create (opr);
	}
	else
		o_kr = fifo_mem_create ();

	/* Add the key to the keyring. */

	n_kr = fifo_file_create (npr);
	pgpk_keyring_add (o_kr, n_kr, new_key);

	/* Close the keyrings. */

	fifo_destroy (n_kr);
	fclose (npr);
	if (opr)
		fclose (opr);

	/* And tidy up the fifos. */

	fifo_destroy (new_key);

	/* Now copy the files over. We rename the old file to the
	   backup file, just as PGP would. */

	unlink (back_file);
	rename (in_file, back_file);
	rename (out_file, in_file);
#else
	/* OK, we now have the key, write it to a temporary file, then
	   call PGP to add it. We would use 'pgp -f', but it doesn't
	   like adding keys that way. */

	dir = getenv ("PGPPATH");

	if (!dir) {
#ifdef MOST_SECURE
		return ADD_NO_TEMP
#else
		strcpy (temp_name, "new-keys.asc");
#endif
	}
	else
		sprintf(temp_name, "%s/new-keys.asc", dir);

	/* Open the temporary file. */

	temp_fp = fopen(temp_name, "wt");

	if (!temp_fp)
		return ADD_NO_TEMP;

	/* Write out the key */

	fwrite (s, (e - s) + strlen (end_key) + 1, 1, temp_fp);
	fclose (temp_fp);

	/* So, it's now in the temporary file, call PGP */

	argv[0] = "pgp";
	argv[1] = "-ka";
	argv[2] = "+batchmode=on";
	argv[3] = temp_name;
	argv[4] = NULL;

	pgp_ret = run_pgp (NULL, 0, argv, NULL);

	/* Having done that, unlink the temporary file */

	unlink (temp_name);

	/* Did the call work? First check specific error codes */

	switch (pgp_ret) {

		/* PGP didn't exist */

		case -1:
		return ADD_NO_PGP;

		/* Keyring addition failed */

		case KEYRING_ADD_ERROR:
		return ADD_BAD_KEY;
	}	

	/* Check for new keys */

	if (strstr((char *)stdout_messages.message,"No new keys"))
		return ADD_OLD_KEY;

	/* Check for bad key */

	if (strstr((char *)stdout_messages.message,"Keyring add error"))
		return ADD_BAD_KEY;

	/* OK, if we get here, then it should have worked... */
#endif

	return	ADD_OK;
}


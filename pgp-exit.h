
/* 
 * PGP Exit codes.
 * 
 * This comes from pgp.c in the PGP distribution - see below for
 * copyright, etc.
 *
 */
 
/*
   PGP: Pretty Good(tm) Privacy - public key cryptography for the masses.

   Synopsis:  PGP uses public-key encryption to protect E-mail. 
   Communicate securely with people you've never met, with no secure
   channels needed for prior exchange of keys.  PGP is well featured and
   fast, with sophisticated key management, digital signatures, data
   compression, and good ergonomic design.

   The original PGP version 1.0 was written by Philip Zimmermann, of
   Phil's Pretty Good(tm) Software.  Many parts of later versions of 
   PGP were developed by an international collaborative effort, 
   involving a number of contributors, including major efforts by:
   Branko Lankester <branko@hacktic.nl>
   Hal Finney <74076.1041@compuserve.com>
   Peter Gutmann <pgut1@cs.aukuni.ac.nz>
   Other contributors who ported or translated or otherwise helped include:
   Jean-loup Gailly in France
   Hugh Kennedy in Germany
   Lutz Frank in Germany
   Cor Bosman in The Netherlands
   Felipe Rodriquez Svensson in The Netherlands
   Armando Ramos in Spain
   Miguel Angel Gallardo Ortiz in Spain
   Harry Bush and Maris Gabalins in Latvia
   Zygimantas Cepaitis in Lithuania
   Alexander Smishlajev
   Peter Suchkow and Andrew Chernov in Russia
   David Vincenzetti in Italy
   ...and others.


   (c) Copyright 1990-1996 by Philip Zimmermann.  All rights reserved.
   The author assumes no liability for damages resulting from the use
   of this software, even if the damage results from defects in this
   software.  No warranty is expressed or implied.

   Note that while most PGP source modules bear Philip Zimmermann's
   copyright notice, many of them have been revised or entirely written
   by contributors who frequently failed to put their names in their
   code.  Code that has been incorporated into PGP from other authors
   was either originally published in the public domain or is used with
   permission from the various authors.

   PGP is available for free to the public under certain restrictions.
   See the PGP User's Guide (included in the release package) for
   important information about licensing, patent restrictions on
   certain algorithms, trademarks, copyrights, and export controls.


   Philip Zimmermann may be reached at:
   Boulder Software Engineering
   3021 Eleventh Street
   Boulder, Colorado 80304  USA
   (303) 541-0140  (voice or FAX)
   email:  prz@acm.org


   PGP will run on MSDOS, Sun Unix, VAX/VMS, Ultrix, Atari ST, 
   Commodore Amiga, and OS/2.  Note:  Don't try to do anything with 
   this source code without looking at the PGP User's Guide.

   PGP combines the convenience of the Rivest-Shamir-Adleman (RSA)
   public key cryptosystem with the speed of fast conventional
   cryptographic algorithms, fast message digest algorithms, data
   compression, and sophisticated key management.  And PGP performs 
   the RSA functions faster than most other software implementations.  
   PGP is RSA public key cryptography for the masses.

   Uses RSA Data Security, Inc. MD5 Message Digest Algorithm
   as a hash for signatures.  Uses the ZIP algorithm for compression.
   Uses the ETH IPES/IDEA algorithm for conventional encryption.

   PGP generally zeroes its used stack and memory areas before exiting.
   This avoids leaving sensitive information in RAM where other users
   could find it later.  The RSA library and keygen routines also
   sanitize their own stack areas.  This stack sanitizing has not been
   checked out under all the error exit conditions, when routines exit
   abnormally.  Also, we must find a way to clear the C I/O library
   file buffers, the disk buffers, and cache buffers.

   Revisions:
   Version 1.0 -  5 Jun 91
   Version 1.4 - 19 Jan 92
   Version 1.5 - 12 Feb 92
   Version 1.6 - 24 Feb 92
   Version 1.7 - 29 Mar 92
   Version 1.8 - 23 May 92
   Version 2.0 -  2 Sep 92
   Version 2.1 -  6 Dec 92
   Version 2.2 -  6 Mar 93
   Version 2.3 - 13 Jun 93
   Version 2.3a-  1 Jul 93
   Version 2.4 -  6 Nov 93
   Version 2.5 -  5 May 94
   Version 2.6 - 22 May 94
   Version 2.6.1 - 29 Aug 94
   Version 2.6.2 - 11 Oct 94
   Version 2.6.2i - 7 May 95
   Version 2.6.3(i) - 18 Jan 96

*/

#define EXIT_OK				0
#define INVALID_FILE_ERROR		1
#define FILE_NOT_FOUND_ERROR		2
#define UNKNOWN_FILE_ERROR		3
#define NO_BATCH			4
#define BAD_ARG_ERROR			5
#define INTERRUPT			6
#define OUT_OF_MEM			7

/* Keyring errors: Base value = 10 */
#define KEYGEN_ERROR			10
#define NONEXIST_KEY_ERROR		11
#define KEYRING_ADD_ERROR		12
	
#define KEYRING_EXTRACT_ERROR		13
#define KEYRING_EDIT_ERROR		14
#define KEYRING_VIEW_ERROR		15
#define KEYRING_REMOVE_ERROR		16
#define KEYRING_CHECK_ERROR		17
#define KEY_SIGNATURE_ERROR		18
#define KEYSIG_REMOVE_ERROR		19

/* Encode errors: Base value = 20 */
#define SIGNATURE_ERROR			20
#define RSA_ENCR_ERROR			21
#define ENCR_ERROR			22
#define COMPRESS_ERROR			23

/* Decode errors: Base value = 30 */
#define SIGNATURE_CHECK_ERROR		30
#define RSA_DECR_ERROR			31
#define DECR_ERROR			32
#define DECOMPRESS_ERROR		33



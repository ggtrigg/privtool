/* $Id$ 
 *
 * Copyright   : (c) 1998 by Glenn Trigg.  All Rights Reserved
 * Project     : Privtool
 * File        : pgplib5
 *
 * Author      : Glenn Trigg
 * Created     : 23 Jan 1998 
 *
 * Description : This implementation of the public functions in pgplib.c
 *		 uses the PGP 5.5 SDK to perform the necessary encryption
 *		 and key management functions.
 *
 *		 Note: Some of this code was taken from the test code of
 *		 the PGP 5.5 SDK source. This is mainly due to the lack
 *		 of programming documentation at this time.
 */

#include	<stdio.h>
#include	<string.h>
#include	<sys/types.h>
#include	<sys/wait.h>
#include	"def.h"
#include	"buffers.h"
#include	<pgpConfig.h>
#include	<pgpEncode.h>
#include	<pgpErrors.h>
#include	<pgpUtilities.h>


/* BUF_SIZE is the general size of stack buffers. Should this ever be used 
   on DOS, you might want to reduce the value of BUF_SIZE, or replace the 
   stack use with a malloc(). */

#ifdef DOS
#define BUF_SIZE	512
#else
#define BUF_SIZE	4096
#endif

/*----------------------------------------------------------------------*/

void
init_pgplib()
{
    /* Should this function really return if initialization fails? - ggt */
    PGPError		err	= kPGPError_NoErr;

    err = PGPsdkInit( );
    if ( IsPGPError( err ) ) {
	printf ("Initialization error: %d\n", err );
    }
} /* init_pgplib */

void
close_pgplib()
{
    (void)PGPsdkCleanup();
} /* close_pgplib */

/*----------------------------------------------------------------------*/

int
encrypt_message(char **user, BUFFER *message, BUFFER *encrypted,
		int flags, char *pass, char *key_name, byte *md5_pass)
{

} /* encrypt_message */

int
decrypt_message(BUFFER *message, BUFFER *decrypted, BUFFER *signature,
		char *pass, int flags, byte *md5_pass)
{

} /* decrypt_message */

/*----------------------------------------------------------------------*/

int
buffer_contains_key(BUFFER *b)
{

} /* buffer_contains_key */

int
add_key(BUFFER *m)
{

} /* add_key */

/*----------------------------------------------------------------------*/


/* Run a specified program with the appropriate arguments, and pass in
   the specified data. If we're running PGP then we may also need to 
   give it a passphrase.
   Not sure why this is in pgplib.c but it's copied straight from there to
   pgplib5.c - ggt
*/

int
run_program(char *prog, byte *message, int msg_len,
	    char **args, char *pass, BUFFER *ret)
{
    int		fd_in[2],fd_err[2],fd_out[2],pass_fd[2];
    int		from_pgp,to_pgp,pgp_error,pass_in;
    int		child_pid;
    fd_set	r_fdset,w_fdset;
#ifndef SYSV
    struct	rusage	rusage;
#endif
    int		statusp;
    int		fds_found;
    char	buf[BUF_SIZE];
    int		size;
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

#if ! (defined(SYSV) || defined(LINUX))
    while (!wait4(child_pid,&statusp,WNOHANG,&rusage))
#else
    while (!waitpid (child_pid, &statusp, WNOHANG))
#endif
    {

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
} /* run_program */

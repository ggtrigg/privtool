/*  $Revision$
**
**  Here be declarations of functions in the InterNetNews library.
*/

/* Memory allocation. */
    /* Worst-case alignment, in order to shut lint up. */
    /* =()<typedef @<ALIGNPTR>@        *ALIGNPTR;>()= */
typedef int    *ALIGNPTR;
extern ALIGNPTR        xmalloc(unsigned int);
extern ALIGNPTR        xrealloc(char *, unsigned int);

/* Headers. */
extern char    *GenerateMessageID(void);
extern char    *HeaderFind(void);
extern void    HeaderCleanFrom(void);

extern struct _DDHANDLE        *DDstart(void);
extern void            DDcheck(void);
extern char            *DDend(void);

/* NNTP functions. */
extern int     NNTPlocalopen(void);
extern int     NNTPremoteopen(void);
extern int     NNTPconnect(void);
extern int     NNTPsendarticle(void);
extern int     NNTPsendpassword(void);

/* Opening the active file on a client. */
extern FILE    *CAopen(void);
extern FILE    *CAlistopen(void);
extern void    CAclose(void);

/* Parameter retrieval. */
extern char    *GetFQDN(void);
extern char    *GetConfigValue(void);
extern char    *GetFileConfigValue(void);
extern char    *GetModeratorAddress(void);

/* Time functions. */
typedef struct _TIMEINFO {
    time_t     time;
    long       usec;
    long       tzone;
} TIMEINFO;
/*extern time_t  parsedate(void);*/
extern int     GetTimeInfo(TIMEINFO *);

/* Miscellaneous. */
extern int     getfdcount(void);
extern int     wildmat(void);
extern int     waitnb(void);
extern int     xread(void);
extern int     xwrite(void);
extern int     xwritev(void);
extern int     LockFile(void);
extern int     GetResourceUsage(void);
extern int     SetNonBlocking(void);
extern void    CloseOnExec(void);
extern void    Radix32(void);
extern char    *INNVersion(void);
extern char    *ReadInDescriptor(void);
extern char    *ReadInFile(void);
extern FILE    *xfopena(void);

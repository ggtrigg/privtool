/* At PCS use gcc mailname.c -lbsd */

#ifdef	ournix	/* { */
#include "ournix.h"
#endif		/* } */
/* JJ This file is still having countries alphabetically added
   to the comment fields, Mexico is next */
char sccsID[] =
	"@(#) mailname.c V1.8 Copyright Julian H. Stacey 2nd April 1991.\n" ;
/* For license conditions see program documentation */
/* email: stacey@freefall.cdrom.com */
/* Separate manual exists,program & source can be safely tabbed. */

/* Specify if host OS supports Symbolic links */
#ifdef scs	/* { */
#define SYMBOLIC_LINKS_AVAILABLE
#endif		/* } */
#ifdef __386BSD__	/* { */
#define SYMBOLIC_LINKS_AVAILABLE
#endif		/* } */
#ifdef svr3	/* { */
	/* I'm not sure if this is the right ifdef for SVR3 */
	/* no SYMBOLIC_LINKS_AVAILABLE */
#endif		/* } */
#ifdef svr4	/* { */
	/* I'm not sure if this is the right ifdef for SVR4 */
#define SYMBOLIC_LINKS_AVAILABLE
#endif		/* } */
#ifdef SCO_1_0	/* { */
	/* SCO 1.0 based on svr3 doesnt have sym links.
		I have no idea if this is the right ifdef,
		neither do I care, SCO is a degenerate OS */
#endif		/* } */
#ifdef SCO_2_0	/* { */
#define SYMBOLIC_LINKS_AVAILABLE
	/*  SCO 2.0 supposedly does have sym links,
		I have no idea if this is the right ifdef,
		neither do I care, SCO is a degenerate OS */
#endif		/* } */
#ifdef MSDOS	/* { */
	/* no SYMBOLIC_LINKS_AVAILABLE */
	/* just 'cos this ifdef is here dont think the prog is ported
		to MessDross */
#endif		/* } */

#include	<stdio.h>
#include	<ctype.h>
#include	<sys/types.h>
#include	<sys/stat.h>
#ifndef	PCS	/* { */
#include	<sys/time.h>
/* for scs bsd4.2 & svr4 */
#else		/* }{ */
#include	<time.h>
#if 0	/* { */
    PCS /usr/include/time.h
	------------
	/*	@(#)time.h	1.1	*/
	/*	3.0 SID #	1.2	*/
	struct	tm {	/* see ctime(3) */
		int	tm_sec ;
		int	tm_min ;
		int	tm_hour ;
		int	tm_mday ;
		int	tm_mon ;
		int	tm_year ;
		int	tm_wday ;
		int	tm_yday ;
		int	tm_isdst ;
	} ;
	extern struct tm *gmtime(), *localtime() ;
	extern char *ctime(), *asctime() ;
	extern void tzset() ;
	extern long timezone, altzone ;
	extern int daylight ;
	extern char *tzname[] ;
	------------
    PCS /usr/include/sys/time.h
	------------
	/*
	 * Copyright (c) 1982 Regents of the University of California.
	 * All rights reserved.  The Berkeley software License Agreement
	 * specifies the terms and conditions for redistribution.
	 *
	 *	@(#)time.h	6.4 (Berkeley) 6/24/85
	 */

	#ifndef _BSD_SYS_TIME_
	#define _BSD_SYS_TIME_

	/*
	 * Structure returned by gettimeofday(2) system call,
	 * and used in other calls.
	 */
	struct timeval {
		long	tv_sec ;		/* seconds */
		long	tv_usec ;	/* and microseconds */
	} ;

	struct timezone {
		int	tz_minuteswest ;	/* minutes west of Greenwich */
		int	tz_dsttime ;	/* type of dst correction */
	} ;
	#define	DST_NONE	0	/* not on dst */
	#define	DST_USA		1	/* USA style dst */
	#define	DST_AUST	2	/* Australian style dst */
	#define	DST_WET		3	/* Western European dst */
	#define	DST_MET		4	/* Middle European dst */
	#define	DST_EET		5	/* Eastern European dst */
	#define	DST_CAN		6	/* Canada */

	/*
	 * Operations on timevals.
	 *
	 * NB: timercmp does not work for >= or <=.
	 */
	#define	timerisset(tvp)		((tvp)->tv_sec || (tvp)->tv_usec)
	#define	timercmp(tvp, uvp, cmp)	\
		((tvp)->tv_sec cmp (uvp)->tv_sec || \
		 (tvp)->tv_sec == (uvp)->tv_sec && (tvp)->tv_usec cmp (uvp)->tv_usec)
	#define	timerclear(tvp)		(tvp)->tv_sec = (tvp)->tv_usec = 0

	/*
	 * Names of the interval timers, and structure
	 * defining a timer setting.
	 */
	#define	ITIMER_REAL	0
	#define	ITIMER_VIRTUAL	1
	#define	ITIMER_PROF	2

	struct	itimerval {
		struct	timeval it_interval ;	/* timer interval */
		struct	timeval it_value ;	/* current value */
	} ;

	#endif _BSD_SYS_TIME_
	------------
#endif		/* } */
#endif		/* } */

/* Notes on time.h struct tm {
	int  tm_sec ;	0-59
	int  tm_min ;	0-59
	int  tm_hour ;	0-23
	int  tm_mday ;	1-31
	int  tm_mon ;	0-11
	int  tm_year ;	year - 1970
	int  tm_wday ;	0-6, 0=sunday
	int  tm_yday ;	0-365
	int  tm_isdst ;	0=not summer
	} ;

/*	-	-	-	-	-	-	-	-	- */
/* This section tabulates the offset in worldwide timezones relative to GMT.
   It will later be broken out to make a library function.

- Purpose to allow an alogithm with reverse functionality to the Unix/C ctime()
  to understand Unix UUCP mail/news world wide date stamps.
- Summer Time (=Daylight Saving Time)
  The table as initialy prepared assumes all summer times are one hour ahead
  of the regions standard winter time, this is not not always true, I know
  some places use double summer time, but cant remeber which.
  However, as countries vary when they switch to summer time, this will never be
  embedded in this table, it is up to the ctime() caller to declare his
  timezone offset.
- Minutes:
  2nd field of table is in minutes not hours because the following have
  non standard non hour quantised timezones:
	West Canada,North coastal South America,
	Saudi, Iran, India & Ceylon, Mongolia, Central Australia (several TZs).
  - Both this table & the BSD 4.2 man description of timezone(), talk in terms
    of minutes (as in hours) around the globe, not in degrees minutes & seconds.
- Table created from the union of:
	- Timezones observed from news in period 89/91
	- Timezones extracted from libc.a with my code below,
	  results then have to be hand edited to weed out things such as
	  East Australia using EST (for summer & winter) when this is needed
	  for USA east code.
- Amendments/Corrections/Extensions: stacey@freefall.cdrom.com
- Libc.a Timezone extractor,
Cut Here	=	=	=	=	=
#ifndef pc532	/ * { * /
#include <sys/time.h>
#else		/ * }{ * /
/ * Bug in PC532 libc.a timezone() causes Illegal instruction (core dumped) * /
#include <time.h>
#endif		/ * } * /
main()	{ int i,j ;
	extern char *timezone() ;
	for (i = -12 ; i <= 11 ; i += 1 )
	for (j=0 ; j < 2 ; j++)
	printf("{\"%s\",\t %d * 60},\t\/\* %s (*1).\*\/\n",
		timezone(60 * i,j), - ( i - ((j == 0) ? 0 : 1) ),
		(j == 0) ? "Standard" : "Summer (Daylight Saving)"
		) ;
	}
	=	=	=	=	=
*/
struct tz
	{
	char *tz_name ;	/* acronym naming a timezone */
	int tz_minutes ; /* number of minutes that zone is ahead of or behind
			    GMT, examples:
	TZ String	Description				Array Content
	-0800: California 4 AM is 8 hours behind midday GMT		-8*60
	+0900: Tokyo 9 PM is 9 hours ahead of midday GMT		+9*60
	+0100: British Summer Time 1 PM is 1hr ahead of midday GMT	+1*60
	*/ } ;
struct tz tz_array[] = {
/* This structure is of indefinite length,to allow for the inevitable:
   some people will adopt different acronyms for the same timezone.
  Line order is the order the countries see Sunrise.
  Direction of world rotation:
	- West moves to East,
	- Anti-clockwise, looked at from the North Pole
	  The BBC TV news programs that showed a globe rotating the other way
	  (around maybe) 1975/1980) were WRONG
	  The modern BBC ring doughnut shaped world at least seems to rotate
	  in the correct direction (though artistic, a ring doughnut is a
	  little puzzling !)
	- Clockwise looked at from the sky at south pole
	- Japan is often referred to in Europe as the land of the rising sun,
	  (Japan sees sunrise before Europe).
  Co-ordinates in () are conventional timezone boundaries (in longitudinal
  minutes east or west of the Greenwich Meridian
  (mins being 360 around equator,not 24*60 in this case)
  Order is (TZ West,TZ Centre,TZ East)
  The meridian runs down the middle of Time zone 0,
  The Date line runs down 180 degrees (with island wiggles !).
  Timezone strings such as GMT-11:00 & GMT+03:30 are shown for convenience,
  but calculated elsewhere.
  It seems (though I'm far from sure about this), that people quote their
  timezones as anything between GMT+23 & GMT-11.
  South Central Australians & a couple of other odd places use odd (ie not
  quantized to the hour) zones,
  40 minutes offset from the hour etc, this software has no problem with that.
  Many time zone names have spaces in.
  Example GMT+0200 is appropriate for Germany in summer, +1 because it's
  1 hour ahead of the Greenwich Meridian, +1 for summer time.
  */
/* -------------------------------------------------------------------------- */
	/* (172.5E,175.25,180)	{"GMT+12:00",12 * 60},
			Possibly/Probably:	New Zealand (NZ was once 11.5 hours)
			All year for:Fiki(FIJI ?) Is.,Kiribati,Marshall Is.,
			Winter only for:
			*/
{"NZDST",13 * 60},	/* as used by system@CODEWKS.nacjack.gen.nz
					3 Oct 92 in a posting */
/* -------------------------------------------------------------------------- */
	/* (157.5E,165E,172.5E)	{"GMT+11:00",11 * 60},
			Possibly/Probably:	USSR Kamchatka Peninsula
			All year for:
			Winter only for:
			*/
/* -------------------------------------------------------------------------- */
	/* (--,--,--)		{"GMT+10:30",10 * 60 + 30},
			All year for:
			Winter only for:Lord Howe Is.
			*/
/* -------------------------------------------------------------------------- */
	/* (142.5E,150E,157.5E)	{"GMT+10:00",10 * 60},
			Possibly/Probably:	NewGuinea,Caroline Island)
			All year for:	Queensland,Guam,
			Winter only for: Victoria & New South Wales & Tasmania
					(ie not Oct-March),
			*/
{"AES",10 * 60},	/* Australian Eastern Standard */
	/* {"EST",10 * 60},	Eastern Standard Time (*1) (note scs bsd4.2
				timezone() returns
				same string for summer & winter,but I'm not
				allowing either because they clash with the
				presumably older & original USA EST ).
			*/
/* -------------------------------------------------------------------------- */
	/* (--,--,--)		{"GMT+9:30",9 * 60 + 30},
			All year for:	Australia Northern Territory,
			Winter only for: South Australia (ie not Oct-March)
			*/
/* -------------------------------------------------------------------------- */
	/* (127.5E,135E,142.5E)	{"GMT+9:00",9 * 60},
			All year for:Indonesia (Moluccas,We Irian),Japan,Korea,
			Winter only for:
			*/
/* -------------------------------------------------------------------------- */
	/* (112.5E,120E,127.5E)	{"GMT+8:00",8 * 60},
			Possibly/Probably:	Phillipines,Taiwan,China
			All year for:	West Australia,Brunei,China Peoples
					Rep (mainland),Hong Kong,Indonesia
					(Kalimantan,Sulawesi,Timor),Macau,
					Malaysia,
			Winter only for:
			*/
{"AWS",8 * 60},	/* Australian Western Standard */
{"WST",8 * 60},	/* Western Standard Time (*1).*/
/* -------------------------------------------------------------------------- */
	/* (97.5E,105E,112.5E)	{"GMT+7:00",7 * 60},
			Possibly/Probably:	Mekong Delta,Vietnam
			All year for:	Christmas Is.,Indonesia (Java,Sumatra,
				Bali),Kampuchea,Laos,
			Winter only for:
			*/
/* -------------------------------------------------------------------------- */
	/* (--,--,--)		{"GMT+6:30",6 * 60 + 30}
			All year for:	Bhutan,Burma,Cocos Is.,
			Winter only for:
			*/
/* -------------------------------------------------------------------------- */
	/* (82.5E,90E,97.5E)	{"GMT+6:00",6 * 60},
			Possibly/Probably:	East Pakistan
			All year for:	Bangladesh
			Winter only for:
			*/
/* -------------------------------------------------------------------------- */
	/* (--,--,--)		{"GMT+5:30",5 * 60 + 30},
			All year for:	India
			Winter only for:
			*/
/* -------------------------------------------------------------------------- */
	/* (67.5E,75E,82.5E)	{"GMT+5:00",5 * 60},
			All year for:Diego Garcia,Maldive Is.,
			Winter only for:
			*/
/* -------------------------------------------------------------------------- */
	/* (--,--,--)		{"GMT+4:30",4 * 60 + 30},
			All year for:	Afghanistan
			Winter only for:
			*/
/* -------------------------------------------------------------------------- */
	/* (52.5E,60E,67.5E)	{"GMT+4:00",4 * 60},
			Possibly/Probably:	Aral See (USSR),Gulf of Oman,Oman,UAE
			All year for:Mauritius,
			Winter only for:
			Summer only for:Moscow(she@elvis.sovusa.com),
			*/
/* -------------------------------------------------------------------------- */
	/* (--,--,--)		{"GMT+3:30",3 * 60 + 30},
			All year for:	Iran
			Winter only for:
			*/
/* -------------------------------------------------------------------------- */
	/* (37.5E,45E,52.5E)	{"GMT+3:00",3 * 60},
			Possibly/Probably:	Kaukasus,Kenya,Ethiopia,Somalia
			All year for:	Bahrain,Cyprus(Turkish/east),Djibouti,
					Ethiopia,Kenya,Kuwait,Madagascar,
					Mayotte,
			Winter only for:Iraq,
			Winter only for:Moscow(she@elvis.sovusa.com),
			*/
/* -------------------------------------------------------------------------- */
	/* (22.5E,30E,37.5E)	{"GMT+2:00",2 * 60},
			Possibly/Probably:	Sudan,Egypt,Rumania,Nile,South Africa
			All year for:	Botswana,Burundi,Comorro Rep.,Israel,
					Lebanon,Lesotho,Malawi
			Winter only for:Bulgaria,Cyprus(Greek/west),Egypt,
					Finland,Greece,Jordan,
			*/
{"EET",2 * 60},		/* East European Time (*1).*/
{"EET DST",3 * 60},	/* EAST European Timezone,Daylight Saving Time
				(Daylight Saving) (*1).*/
{"IDT",3 * 60},		/* Israel Daylight Saving Time, as seen in a news
			posting 2 Nov 1992 by marx@vms.huji.ac.il */
{"FST",2 * 60},		/* Finnish Standard? Time
				jarmo@ksvltd.FI
				Date: Sat, 17 Oct 92 11:32:18 FST */
/* -------------------------------------------------------------------------- */
	/* (7.5E,15E,22.5E)	{"GMT+1:00",1 * 60},
			Probably:	Central Africa,Holland,Norway,Poland,Spain,Sweden,Tunisia,Yugoslavia
			All year for:	Albania,Andorra,Angola,Benin Cameroon,
					Central African Republic,Chad,Congo
					(Brazzaville),Equatorial Guinea,Gabon,
					Lebanon,
			Winter only for:Austria,Belgium,Czechoslovakia,Denmark,
					France,Germany,Gibraltar,Hungary,Italy,
					Libya,Luxembourg,Malta,
			*/
{"MET",1 * 60},		/* Middle European Time (*1).*/
{"MES",2 * 60},		/* Middle European Summer (*1).*/
{"DST",2 * 60},		/* Deutsche Sommer T?ime - As seen from
			   Mon, 7 Sep 92 17:07:54 DST chris@chome.en.open.de */
{"MED",2 * 60},		/* Mittel Europaische D...
				Probably Summer Time
				Date: Tue, 22 Sep 92 19:16:13 MED
				jmastel@immd4.informatik.uni-erlangen.de
				*/
{"CET",1 * 60},		/* Central European Time */
{"MEZ",1 * 60},		/* Middle European Time */
{"MSZ",2 * 60},		/* Middle European Summer Time */
{"MET DST",2 * 60},	/* Middle European Timezone,Daylight Saving Time (*1).*/
/* -------------------------------------------------------------------------- */
	/* (7.5W,0,7.5E)	{"GMT",0},	Greenwich Mean Time. Note below.
			Possibly/Probably:	Portugal, West Africa,Marroco,
			All year for:	Ascension Is.,Gambia,Ghana,Guinea(Rep.),
					Guinea Bissau,Iceland,Ivory Coast,
					Liberia,Madeira,Mali,Mauritania,
			Winter only for:Algeria,Britain,Canary Is,Faroe Is.,
					Greenland East (N of 71 deg.),Ireland,
			*/
{"GMT",0},		/* Greenwich Mean Time. */
{"UTC",0},		/* Universal Coordinated Time */
{"UCT",0},		/* Universal Coordinated Time
				siglun@lotka.teorekol.lu.se
				Tue, 13 Oct 92 19:50:02 UCT	*/
{"UT",0},		/* Universal Time */
{"WET",0},		/* West European Time (*1).*/
{"WET DST",1 * 60},	/* West European Timezone,Daylight Saving Time (*1).*/
{"BST",1 * 60},		/* British Summer Time - possibly not used for email */
/* -------------------------------------------------------------------------- */
	/* (22.5W,15W,7.5W)	{"GMT-1:00",-1 * 60},
			All year for:	Cape Verde Is,
			Winter only for:Azores,Greenland (Scresby Sound),
			*/
/* -------------------------------------------------------------------------- */
	/* (37.5W,30W,22.5W)	{"GMT-2:00",-2 * 60},
			All year for:	Brazil (Oceanic Islands),
			Winter only for:
			*/
/* -------------------------------------------------------------------------- */
	/* (52.5W,45W,37.5W)	{"GMT-3:00",-3 * 60},
			Possibly/Probably:	Greenland Central,Uruguay
			All year for:	Argentina,Brazil (Eastern & Coastal),
					Guiana (French),Guyana,
			Winter only for:Greenland (Angmagssalik,W.Coast (exc
					Thule))
			*/
/* -------------------------------------------------------------------------- */
	/* (--,--,--)		{"GMT-3:30",-3 * 60 -30},
			All year for:
			Winter only for:Canada-Newfoundland,
/* -------------------------------------------------------------------------- */
	/* (67.5W,60W,52.5W)	{"GMT-4:00",-4 * 60},
			Possibly/Probably:	Paraguay,Venezuela,Guyana,Suriname,French Guyana,San Juan,Lesser Antillies,
			All year for:	Anguilla,Antigua,Barbados,Bolivia,
					Brazil (Manaos),Dominica,Dom.Rep.,
					Falkland Is.(except Port Stanley),
					Greenland (Thule Area),Grenada,
					Guadeloupe,Martinique,
			Winter only for:Bermuda,Canada Atlantic(Labrador,
					Nova Scotia),Chile (Not Oct-March),
					Falkland Is.(Port Stanley only,winter=
					not Oct-March)),
			*/
{"AST",-4 * 60},	/* Standard (*1).*/
{"ADT",-3 * 60},	/* Daylight Saving Time (*1).*/
/* -------------------------------------------------------------------------- */
	/* (82.5W,75W,67.5W)	{"GMT-5:00",-5 * 60},
			Possibly/Probably:	USA Eastern Zone,peru,Panama,haiti,Dominican Rep.
			All year for:	Brazil (Acre),Colombia,Ecuador,Haiti,
			Winter only for:Bahamas,Eastern Canada(Ontario,Quebec),
					Cayman Is.,Cuba (ie not march - sept),
					Jamaica,
			*/
{"EST",-5 * 60},	/* USA Eastern Standard Time (*1) */
{"EDT",-4 * 60},	/* USA Eastern Daylight Saving Time (*1) */
/* -------------------------------------------------------------------------- */
	/* (97.5W,90W,82.5W)	{"GMT-6:00",-6 * 60},
			Possibly/Probably:	USA Central Zone,?,Nicaragua,Mexico,
			All year for:	Belize,Costa Rica,El Salvador,
					Guatemala,Honduras (Rep.),
			Winter only for: Canada Central (Manitoba),Easter Island
			*/
{"CST",-6 * 60},	/* USA Central Standard Time (*1) */
{"CAM",-6 * 60},	/* As used by hope@huracan.cr 18 Sep 92 in a posting,
			   JJLATER no idea if this is summer or winter time */
{"CDT",-5 * 60},	/* USA Central Daylight Saving Time (*1) */
/* -------------------------------------------------------------------------- */
	/* (112.5W,105W,97.5W)	{"GMT-7:00",-7 * 60},
			All year for:
			Winter only for: Canada Mountain (Alberta,NWT)
			*/
{"MST",-7 * 60},	/* USA Mountain Standard Time (*1) */
{"MDT",-6 * 60},	/* USA Mountain Daylight Saving Time (*1) */
/* -------------------------------------------------------------------------- */
	/* (127.5W,120W,112.5W)	{"GMT-8:00",-8 * 60}, USA Pacific Zone
			All year for:
			Winter only for:Eastern Alaska (Yukon),Canada Pacific
					(Br. Columia)
			*/
{"PST",-8 * 60},	/* USA Pacific Standard Time (*1) */
{"PDT",-7 * 60},	/* USA Pacific Daylight Saving Time (*1) */
/* -------------------------------------------------------------------------- */
	/* (142.5W,135W,127.5W)	{"GMT-9:00",-9 * 60},
			All year for:
			Winter only for: Alaska (Juneau Pacific Coastline ?)
			*/
/* -------------------------------------------------------------------------- */
	/* (157.5W,150W,142.5W)	{"GMT-10:00",-10 * 60},
			All year for:	Johnston Is.,Hawaii
			Winter only for:Alaska Central (Anchorage?),Cook Is.
					(becomes -9.5 in summer),
			*/
{"HDT",-9 * 60},	/* as used by sgee@musubi.pubnet.com 21 Oct 92
				in a news posting */
/* -------------------------------------------------------------------------- */
	/* (172.5W,165W,157.5W)	{"GMT-11:00",-11 * 60},
			All year for:
			Winter only for:Alaska West (Nunivak?),
			*/
/* -------------------------------------------------------------------------- */
	/* (--,--,--)		{"GMT-11:30",-11 * 60 - 30},
			Possibly/Probably:	Samoa
			All year for:
			Winter only for:
			*/
/* -------------------------------------------------------------------------- */
{(char *)0,0* 60}
} ;

/* Notes:
Countries timezones were generally extracted from the "World Radio TV Handbook
 38th Ed. (1984) Page 72", they are listed in their winter timezone
 (Remember South of equator summer season != North of equator summer!)
GMT / Greenwich Mean Time
 Greenwich is on the River Thames, down river from London, the British Navy
 in centuries past used to set their chronometers there, while victualing
 (= taking on food & water).
 GMT was for centuries of immense significance to long distance navigators.
 GMT was long ago referenced as timezone 0.
UT / Universal Time
 The abreviated acronym by which Unix systems refer to UTC
UTC
 World Radio TV Hanbook 38th Ed. Page 16 says:
   "... UTC (Coordinated Universal Time). This term was officially adopted on
    Jan 1st 1982 by the International Telecommunications Union, replacing the
    familiar GMT ...."
  I presume the letter ordering (UTC rather than UCT) originates from the fact
  that the ITU prime language is French (like CCITT).

*/
/*	-	-	-	-	-	-	-	-	- */
typedef char	FLAG ;
#define LINE_LN 600
	/* LINE_LN: maximum scannable length of a text line in a mail,
	   I just picked a large number, if someone has a better method,
	   let me know
	*/
char	**ARGV ;
FILE	*my_err ;

#undef		USE_STRCHR
#ifdef	i386	/* { */
#define		USE_STRCHR
#endif		/* } */
#ifdef	vax	/* { */
#define		USE_STRCHR
#endif		/* } */
#ifdef	MSDOS	/* { */
#define		USE_STRCHR
#endif		/* } */
#ifdef	USE_STRCHR	/* { */
	extern	char *strchr() ;
	extern	char *strrchr() ;
#define index(s,c)	strchr(s,c)
#define rindex(s,c)	strrchr(s,c)
#else			/* }{ */
	extern	char *index() ;
	extern	char *rindex() ;
#endif			/* } */

int	err_cnt = 0 ;
char	from_hdr[] =	"From:" ;
int	from_ln ;
char	date_hdr[] =	"Date:" ;
int	date_ln ;
char	message_hdr[] =	"Message-Id:" ; /* Message-ID is also valid */
int	message_ln ;
/* Although this program is designed to analyse incoming mail,
   I added to_hdr & to_ln
   so that it'll try to analyse files
   that are ready for transmission that are stored in Sendmail syntax.
*/
char	to_hdr[] =		"To:" ;
int	to_ln ;
#define strnequ(s1,s2,ln)	(0==strncmp(s1,s2,ln))
#define strequ(s1,s2)		(0==strcmp(s1,s2))
long	file_lines ;		/* line no. in file for editor reference */
long	msg_lines ;		/* no of lines of message excluding header */

FLAG		verbose_f = 0 ;
FLAG		pretend_f = 0 ;		/* 1 - pretend to work, do all
						analysis up to the rename/link,
						even say youve done it, but
						dont actually do it! */
FLAG		no_warnings = 0 ;		/* 1 = dont issue warnings */
FLAG		head_f = 0 ;		/* 1 ==> do header only (faster) */
FLAG		beyond_header = 0 ;		/* 1 = read beyond header */
FLAG		over_write_f = 0 ;
FLAG		msg_append_f = 0 ;
FLAG		short_names_f = 0 ;	/* 1 ==> <= 14 char file names */
FLAG		numeric_0_f = 0 ;	/* 1 ==> incremental numeric,leading 0*/
FLAG		numeric_f = 0 ;		/* 1 ==> incremental numeric */
char		action_ch = 'r' ;
char		txt_warning[] = "Warning:" ;
char		txt_error[] = "Error:" ;
char		sec_str[3] ;
int	days_in_month[14] = { 31 /*dec*/,
	31,28,31,30,31,30,31,31,30,31,30,31 /*jan-dec*/, 31 /*jan*/ } ;
	/* used by adjust() */

#ifdef DEBUG	/* { */
	/* used for timec */
	char txt_dbg_ctl[]	= "%s\t%s%s%s\n\t\t\t\t%s,\n\t%s\t%s%s%s\n\t\t\t\t%s.\n";
	char txt_dbg_is[]	= "Date format order is";
	char txt_dbg_day[]	= "Day_name ";
	char txt_dbg_dm[]	= "Day_in_Month Month" ;
	char txt_dbg_such_as[]	= ", such as:";
	char txt_dbg_dm_e[]	= "\"Date: Thu, 7 Dec 89 10:19 PST\"" ;
	char txt_dbg_opp[]	= "as opposed to";
	char txt_dbg_md[]	= "Month Day_in_Month";
	char txt_dbg_md_e[]	= "\"Date: Thu Nov 23 01:57:39 1989\"";
#endif		/* } */

/* returns month number (0 to 11) or -1 if fails */
	int
month_atoi(name)
	char	*name ;
	{
	char	*p,*p2 ;
	char	**pp ;
	int	count ;
#define MONTH_LN 4
	char	tmp[MONTH_LN] ;
	static char *months[13] = { "jan","feb","mar","apr","may","jun",
				"jul","aug","sep","oct","nov","dec",(char *)0 } ;

	for (p=name,p2=tmp,count =MONTH_LN ; (*p != '\0') && (--count != 0) ;p++ )
		*p2++ = (isupper(*p)) ? tolower(*p) : *p ;
	*p2 = '\0' ;
	for (pp = months, count = 0 ; *pp != (char *)0 ; pp++,count++)
		if (strnequ(*pp,tmp,MONTH_LN -1)) return(count) ;
	return(-1) ;
	}

/* returns day number (0 = Sun, 6=Sat) or -1 if fails */
	int
day_atoi(name)
	char	*name ;
	{
	char	*p,*p2 ;
	char	**pp ;
	int	count ;
#define DAY_LN 4
	char	tmp[DAY_LN] ;
	static char *days[8] =	{ "sun","mon","tue","wed","thu","fri","sat",
				(char *)0} ;

	for (p=name,p2=tmp,count = DAY_LN ; (*p != '\0') && (--count != 0) ;p++ )
		*p2++ = (isupper(*p)) ? tolower(*p) : *p ;
	*p2 = '\0' ;
	for (pp = days, count = 0 ; *pp != (char *)0 ; pp++,count++)
		if (strnequ(*pp,tmp,DAY_LN - 1)) return(count) ;
	return(-1) ;
	}

	int
get_line(fp,where,max_length,name)
	FILE	*fp ;
	register char *where ;
	register int max_length ; /* max. size of array including null */
	char		*name ;
	{ register int ch ; char *orig ;
	orig = where ;
	file_lines++ ;
	max_length-- ; /* allow for \n */
	while (((ch = getc(fp)) != EOF) && ((char)ch != '\n'))
		{ if (--max_length <= 0 )
			{ (void) fprintf(my_err,
				"%s: %s in %s\t - Line %ld too long.\n",
				*ARGV,txt_error,name,file_lines) ;
			err_cnt++ ;
			*orig = '\0' ; where=orig ;
			break ;
			}
		*where++ = (char)ch ;
		}
	if ((char)ch == '\n') *where++ = '\n' ;
	*where = '\0' ;
	return(where - orig) ;
	}

syntax(ex)
	int	ex ;
	{
	(void) fprintf(my_err,
	 "Syntax: %s [-h] [-ls] [-m] [-nNt] [-o] [-p] [-v] [-w] [-?] file[s]\n",
		*ARGV) ;
	exit(ex) ;
	}

/* If date_element has unscanned text return 0, else complain & return 1 */
	int
is_empty_date(date_element,file_name,comment)
	char *date_element ;	/* if set to "\0", this routine may have been
				   called merely as a means of providing a
				   standard format warning diagnostic,
				   & not actually to do any real work */
	char *file_name ;
	char *comment ;
	{
#ifdef DEBUG	/* { */
	(void) fprintf(my_err,"is_empty_date(\"%s\",\"%s\"'\"%s\")\n",
		date_element,file_name,comment) ;
#endif		/* } */
	if ((*date_element != '\n') && (*date_element != '\0'))
		/* stuff remains */ return(0) ;
	if (!no_warnings)
		{
		(void) fprintf(my_err, "%s: %s problem %s.\n"
			, *ARGV, txt_warning, comment) ;
		(void) fprintf(my_err,
		"%s: %s prematurely truncated date in %s, skipping line.\n"
			, *ARGV, txt_warning, file_name) ;
		}
	return(1) ;
	}

main(argc,argv)
	int	argc ;
	char	**argv ;
	{
	/* This program generates no normal screen output, everything it
	   reports is an error, to ease error collection & conversion for
	   human intervention edit scripts, I send it all to stdout.
	   If you dont approve, just use cc -D ERRS_TO_STDERR */
	my_err = stdout ;
#ifdef	ERRS_TO_STDERR	/* { */
	my_err = stderr */
#endif		/* } */

	ARGV = argv ;
#ifdef	VSL	/* { */
#include	"../../include/vsl.h"
#endif		/* } */
	from_ln = strlen(from_hdr) ;
	to_ln = strlen(to_hdr) ;
	date_ln= strlen(date_hdr) ;
	message_ln= strlen(message_hdr) ;
	for (argc--,argv++ ; argc > 0 ; argv++ )
		{ if(**argv != '-') break /* no more parameters */ ;
		argc-- ;
		switch(*++*argv)
			{ /* option detect */
			case 'n':
				numeric_0_f = 1 ;
				numeric_f = 0 ;
				short_names_f = 0 ;
				break ;
			case 'N':
				numeric_f = 1 ;
				numeric_0_f = 0 ;
				short_names_f = 0 ;
				break ;
			case 'p':
				pretend_f = 1 ;
				break ;
			case 'v':	/* verbose mode */
				verbose_f = 1 ;
				break ;
			case 'h':	/* read header only, run faster */
				head_f = 1 ;
				break ;
			case 'w':	/* turn off warnings */
				no_warnings = 1 ;
				break ;
			case 'o':	/* overwrite target if it exists */
				over_write_f = 1 ;
				break ;
			case 'm':	/* force message id to be appended */
				msg_append_f = 1 ;
				break ;
			case 't':
				/* terminate names after 14 characters */
				short_names_f = 1 ;
				numeric_f = numeric_0_f = 0 ;
				break ;
			case 'l': /* link */
#ifdef	SYMBOLIC_LINKS_AVAILABLE /* { */
			case 's': /* symbolic link */
#endif					/* } */
				action_ch = **argv ;
				break ;
			case '?': syntax(0) ;
				break ;
			default:
				(void) fprintf(my_err,
					"%s: '%c' - Unavailable option.\n",
					*ARGV,**argv) ;
				syntax(1) ;
				break ;
			}
		} /* finished taking parameters */

	while (argc--) err_cnt += do_file(*argv++) ;
	exit(err_cnt) ;
	}

	int
do_file(initial_name)
	char	*initial_name ;
	{
	extern	int	link() ;
	extern	int	symlink() ;
	extern	int	rename() ;
	struct	tm	stamp ;
	char		new_name[LINE_LN] ;
	int		(*action_proc)() ;
	char		*action_name ;
	char		*verb_name ;
	static int	numeric_i = 0 ;
	char *p, *p2 ;
	struct stat	stat_buf ;
	char		line_in[LINE_LN] ;
	char		from_str[LINE_LN] ;
	char		message_str[LINE_LN] ;
	char		to_str[LINE_LN] ;
	char		date_str[LINE_LN] ;
	char		spare_str[LINE_LN] ;	/* tmp: person name or file (+ dir.) name */
	FILE		*fp_1 ;
	int		err_no = 0 ;
	int		rslt ;

	file_lines = msg_lines = 0L ;
	from_str[0] =  message_str[0] =  to_str[0] =  date_str[0] =
		new_name[0] = '\0' ;
	beyond_header = 0 ;

	if ( ( fp_1 = fopen(initial_name,"r")) == (FILE *)0 )
		{ (void) fprintf(my_err,"%s: %s: Cannot open %s\n",
			*ARGV,txt_error,initial_name) ;
		return(err_no+1) ;
		}
	/* Now scan for date & sender or recipient fields, either just in
	   header before blank line, or through entire file,
	   if warnings of archive required */
	while ( get_line(fp_1,line_in,LINE_LN,initial_name) )
		{
		if (beyond_header) msg_lines++ ;
		if (line_in[0] == '\n')
			{
			beyond_header = 1 ;
			if (head_f) break ;
			}
		p = line_in ;
		if (!short_names_f && strnequ(line_in,from_hdr,from_ln))
			{
			if (beyond_header)
				{ if (!no_warnings)  (void) fprintf(my_err,
					"%s: %s In %s, %s %s",
					*ARGV, txt_warning,
					initial_name, "ignoring unexpected",
					line_in) ;
				err_no++ ; continue ;
				}
			if (from_str[0] != '\0')
				{ if (!no_warnings) (void) fprintf(my_err,
					"%s: %s %s, %s %s",
					*ARGV, txt_warning, initial_name,
					"Ignoring", line_in) ;
				err_no++ ; continue ;
				}
			/* Get the actual user name from the mail name.
			See Bug Note in manual.
			*/
			if ( ((p=index(line_in,'%'))==(char *)0) &&
				((p=index(line_in,'@'))==(char *)0) &&
				((p=index(line_in,'!'))==(char *)0) )
				{
				/* Examples: From: dlr (Dave Rand)
				*/
				p = line_in + from_ln ;
				while(isspace(*p)) p++ ;
				if (*p == '\0')
					{
					if (!no_warnings) (void) fprintf(my_err,
					"%s: %s In %s, empty address %s",
					*ARGV, txt_warning,initial_name,line_in) ;
					err_no++ ; continue ;
					}
				from_str[0] = '\0' ;
				(void) strcpy(from_str,p) ;
				for ( p = from_str ;
					(!isspace(*p)) && (*p != '\0') ; ) p++ ;
				*p = '\0' ;
				if (!no_warnings) (void) fprintf(my_err,
		"%s: %s In %s, Unusual net address (possibly local ?): %s\n",
					*ARGV, txt_warning,initial_name,from_str) ;
				}
			else if ((*p == '@') || (*p == '%'))
				{
				/* Examples:
					Bruce Culbertson <culberts@hplwbc.hpl.hp.com>
					george@wombat.bungi.COM (George Scolaro)
					dave%sea375%sigma@quick.com
					dave%sea375.uucp@unido.de
				*/
				*p-- ='\0' ;
				while( (isalnum(*p) || ispunct(*p))
					&& (*p != '<')
					&& (p > line_in)) p-- ; p++ ;
				if ((p2 = rindex(p,'!')) != (char *)0 )
				/* Allow for weird addresses like
	From: sigma!sea375!dave@quick.com */
					p = p2 + 1 ;
				(void) strcpy(spare_str,p) ;
				(void) strcpy(from_str,spare_str) ;
				}
			else /* (*p == '!') */
				{
				/* Examples:
					John Connin <daver!uunet!manatee!johnc>
					decwrl!voder.nsc.com!nsc!wombat.bungi.COM!george (George Scolaro)
					sun!ucscc.UCSC.EDU!clanhlm!blank
				*/
				p = rindex(line_in,'!') ;
				(void) strcpy(from_str,++p) ;
				p = from_str ;
				while( (*p != '\0') &&
					(isalnum(*p) || ispunct(*p))
					&& (*p != '>')) p++ ;
				*p = '\0' ;
				}
			}
		else if (!short_names_f &&
			strnequ(line_in, to_hdr, to_ln))
			{
			if (beyond_header)
				{ if (!no_warnings) (void) fprintf(my_err,
					"%s: %s In %s, %s %s",
					*ARGV, txt_warning,
					initial_name, "ignoring unexpected",
					line_in) ;
				err_no++ ; continue ;
				}
			if (to_str[0] != '\0')
				{ if (!no_warnings) (void) fprintf(my_err,
					"%s: %s In %s, %s %s",
					*ARGV, initial_name,
					"Ignoring", line_in) ;
				err_no++ ; continue ;
				}
			/* Get the actual user name from the mail name.
			See Bug Note in manual.
			*/
				{
				/* Examples: To: dlr (Dave Rand) */
				p = line_in + to_ln ;
				while(isspace(*p)) p++ ;
				if (*p == '\0')
					{
					if (!no_warnings) (void) fprintf(my_err,
					"%s: %s In %s, empty address %s",
					*ARGV, txt_warning,initial_name,line_in) ;
					err_no++ ; continue ;
					}
				to_str[0] = '\0' ;
				(void) strcpy( to_str,p) ;
				for ( p = to_str ;
					(!isspace(*p)) &&
				/* Reduce "To: jack@bucket.com,jill@hill.gov"
				   To "jack@bucket.com"
				   Note as we dont have a precise date taking
				   up characters of the filename length, we
				   can afford to use full email names,
				   not just local login names
				   (as in From: field)
				 */
					(*p != ',' ) &&
					 (*p != '\0') ; ) p++ ;
				*p = '\0' ;
				}
			}
		else if (!short_names_f &&
			strnequ(line_in, message_hdr, message_ln - 2))
			/* -2 allows for "...-Id:" and "...-ID:" */
			{
			if (beyond_header)
				{ if (!no_warnings) (void) fprintf(my_err,
					"%s: %s In %s, %s %s",
					*ARGV, txt_warning,
					initial_name, "ignoring unexpected",
					line_in) ;
				err_no++ ; continue ;
				}
			if (message_str[0] != '\0')
				{ if (!no_warnings) (void) fprintf(my_err,
					"%s: %s In %s, %s %s",
					*ARGV, txt_warning, initial_name,
					"Ignoring", line_in) ;
				err_no++ ; continue ;
				}
			/* Get the actual Message-ID */
			p = line_in + message_ln + 2 ;/* 2 to space past " <" */
			message_str[0] = '\0' ;
			(void) strcpy( message_str,p) ;
			/* chop trailing '>' */
			p = message_str ;
			while ((*p != '\0' ) && ( *p != '>')) p++ ;
			*p =  '\0' ;
			}
		else if (strnequ(line_in,date_hdr,date_ln))
			{ if (beyond_header)
				{ if (!no_warnings) (void) fprintf(my_err,
					"%s: %s In %s, %s %s",
					*ARGV, txt_warning,
					initial_name, "ignoring unexpected",
					line_in) ;
				err_no++ ; continue ;
				}
			if (date_str[0] != '\0')
				{ if (!no_warnings) (void) fprintf(my_err,
					"%s: %s In %s, %s %s",
					*ARGV, txt_warning, initial_name,
					"Ignoring", line_in) ;
				err_no++ ; continue ;
				}
			/* skip "Date:" */
			while (!isspace(*p) && (*p != '\0')) p++ ;
			while (isspace(*p)) p++ ;
			if (is_empty_date(p,initial_name,"after \"Date:\""))
				{ err_no++ ; continue ; }
			if (timec(p,initial_name,&stamp)<0)
				{
				(void)is_empty_date("\0",initial_name,"after timec()") ;
				err_no++ ; continue ;
				}

			(void) sprintf(date_str,
				"%02d%02d%02d_%02d%02d_%s",
				stamp.tm_year + 70 /* from 1970 */,
				stamp.tm_mon+1,stamp.tm_mday,
				stamp.tm_hour, stamp.tm_min,sec_str ) ;
			}
		}
	(void) fclose(fp_1) ; /* finished reading file */

	if ( (to_str[0] == '\0') && (from_str[0] == '\0') )
		{
		if (date_str[0] == '\0')
			{ /* skip */
			(void) fprintf(my_err,
		  "%s: %s, Skipping %s, No \"%s\" or \"%s\" or \"%s\".\n",
				*ARGV,txt_error, initial_name, date_hdr,
				from_hdr, to_hdr ) ;
			return(err_no+1) ;
			}
		else	{
			/* warn user of something strange, even if
			   running on a 14 char filename computer that can't
			   use the from field to append to the filename */
			if (!no_warnings) (void) fprintf(my_err,
				"%s: %s: In %s, could not find a \"%s\" or a \"%s\".\n",
				*ARGV, txt_warning, initial_name, from_hdr, to_hdr ) ;
			err_no += 1 ;
			}
		}
	(void) strcpy(new_name,date_str) ;
	/*
	   There is no "Date: " field in ...
		From pete Mon Nov  2 11:56:57 1992
		Received: by muenchen.freefall.cdrom.com
		          (5.57/GUUG-V3.7) id AA28388 ; Mon, 2 Nov 92 11:56:32 +0100
		Message-Id: <9211021056.AA28388@muenchen.freefall.cdrom.com>
		From: pete
		Apparently-To: stacey
	 */
	if (!short_names_f || (date_str[0] == '\0'))
		{
		/* Preferably label with name of From: sender,
		   if not available, use name of To: intended later receiver
		*/
		if (from_str[0] != '\0')
			{
			if (date_str[0] != '\0') (void) strcat(new_name,".") ;
			(void) strcat(new_name,from_str) ;
			}
		else if (to_str[0] != '\0')
			{
			if (date_str[0] != '\0') (void) strcat(new_name,".") ;
			(void) strcat(new_name,to_str) ;
			}
		}

	/* Most networks treat upper & lower case as the same, but humans
	   are eratic in their use of upper & lower, so converge them */
	for (p=new_name ; (*p != '\0') ; p++) if (isupper(*p)) *p = tolower(*p) ;
	/* convert names like From: gribb/sps@gribb.hsr.no (Stein P. Svendsen)
		To gribb_sps */
	while ((p = index(new_name,'/')) != (char *)0) *p = '_' ;
/*
mailname: Error:: Failed to rename ./news/answers/23191 to ./news/answers/940801_1707_28.pschleck.radio/ham-radio/elmers/index-1-775759636@unomaha.edu
*/
	
	/* convert names like 
		From: "gj%pcs.dec.com@inet-gw-1.pa.dec.com" <garyj@pcs.dec.com>
		to _gj */
	while ((p = index(new_name,'"')) != (char *)0) *p = '_' ;

	/* "Pavlov's Cat" <Pavlov's.Cat@infi.net> */
	while ((p = index(new_name,'\'')) != (char *)0) *p = '_' ;

	while ((p = index(new_name,'`')) != (char *)0) *p = '_' ;

	if ((p = rindex(initial_name,'/')) != (char *)0)
		{ strcpy(spare_str,initial_name) ;
		(void) strcpy( rindex(spare_str,'/') + 1,new_name) ;
		(void) strcpy(new_name,spare_str) ;
		}
	if (msg_append_f &&
		(message_str[0] != '\0'/* if an outgoing mail, will be null*/))
		{
		(void) strcat(new_name,".") ;
		(void) strcat(new_name,message_str) ;
		}

	/* if numeric_0_f or numeric_f are asserted, we could save processor
		time earlier, but I dont have human time available to
		retro fit this better. (I added this option to provide
		simple numeric sequence numbering for other pre-existing
		mail handlers, msdos compatible simple file names etc,
		it doesnt really belongs in this program at all,
		its a bit wasteful, but hard luck */
	if (numeric_0_f) (void) sprintf(new_name,"%04d",++numeric_i) ;
	if (numeric_f) (void) sprintf(new_name,"%d",++numeric_i) ;

	if ((msg_lines == 0L) && !head_f )
		{
		/* Warn if no message.
		Note, one line mail is not considered suspicious ;
		4 or 5 people sent one line mail in the last 2 year's of
		comp.sys.nsc.32k newsgroup archives ! ).
		*/
		if (!no_warnings) (void) fprintf(my_err,
		"%s: %s: Only %ld message lines in %s (becoming %s).\n",
				*ARGV, txt_warning,msg_lines,initial_name,new_name) ;
		err_no += 1 ;
		}

	if (strequ(initial_name,new_name) )
		{ if (verbose_f) (void) printf(
			"Skipping %s, No change necessary.\n",
			new_name) ;
		return(err_no) ;
		}
	/* now check to see the target file doesnt exist,
	   dont want to zap anything,
	   a typical example is multi part FAQs posted automatically
	   within the same second, or `for loop' mail senders
	*/
	if (!over_write_f && ( (rslt = stat(new_name,&stat_buf)) == 0))
		{ /* stat has detected a pre existing target file */
		if (	!msg_append_f /* not already appended */ &&
			(message_str[0] != '\0'
			/* outgoing mail will have null*/	))
			{
			(void) fprintf(my_err,
			  "%s: %s Pre existant target %s (for old name %s),\n"
			  ,*ARGV, txt_warning,new_name,initial_name) ;
			(void) strcat(new_name,".") ;
			(void) strcat(new_name,message_str) ;
				/* note if any nasty magic chars such as space
				   appear in Message-Id, they'll appear in
				   file name
				 */
			/* this tag mailname_duplicate is for use by find(1) */
			(void) strcat(new_name,".mailname_duplicate") ;
			(void) fprintf(my_err,
				"\tso will target %s\n", new_name ) ;
			}
		if ( msg_append_f || (rslt = stat(new_name,&stat_buf)) == 0)
			{
			(void) fprintf(my_err,
			 "%s: %s Pre existant %s, so not renamed %s\n",
			*ARGV, txt_error,new_name,initial_name) ;
			return(err_no+1) ;
			}
		}
	switch (action_ch)
		{ case 'l':	action_proc = link ;
				action_name = "link" ;
				verb_name = "Linked" ;
			break ;
#ifdef	SYMBOLIC_LINKS_AVAILABLE /* { */
		case 's':	action_proc = symlink ;
				action_name = "symlink" ;
				verb_name = "Symbolically linked" ;
			break ;
#endif			/* } */
		case 'r':
		default:
				action_proc = rename ;
				action_name = "rename" ;
				verb_name = "Renamed" ;
			break ;
		}
	if (!pretend_f && (*action_proc)(initial_name,new_name))
		{ (void) fprintf(my_err,
			"%s: %s: Failed to %s %s to %s\n",
			*ARGV,txt_error,action_name,initial_name,new_name) ;
		return(err_no+1) ;
		}
	if (verbose_f) (void) printf("%s %s\tTo %s\n",verb_name,
		initial_name,new_name) ;
	return(err_no) ;
	}


/* Add an offset of (generally N*60) minutes to a time stored as
	minute, hour, day, month, year since 1st Jan. 1970, converting time in
	one timezone on the globe to time in a different timezone.
	Untouched (as I dont know what it's for): tm_isdst */
	void
adjust(minutes,tm_p)
	int		minutes ; /* offset in minutes
					 (per hour not longitudinal),
					may be +ve or -ve  */
	struct tm	*tm_p ;	/* thing to be changed */
	{
	int	hour,polarity ;

#ifdef DEBUG	/* { */
	printf("adjust %d\t",minutes); printf("asctime %s",asctime(tm_p)) ;
#endif		/* } */
	if (minutes == 0) return ;
	if (( minutes > 23 * 60 + 59) || ( minutes < -23 * 60 -59))
		{
		fprintf(my_err,
		"%s: Program %s, impossible offset %d minutes, aborting.\n",
		*ARGV,txt_error,minutes) ;
		exit(1) ;
		}
	polarity = (minutes > 0) ? 1 : -1 ;
	if (minutes < 0) minutes = -minutes ;
	hour = minutes / 60 ; minutes = minutes % 60 ;
	minutes *= polarity ; hour *= polarity ;

	/* next bit relies on tm_p->various not being unsigned, but int
	   this is true for 386BSD */
	tm_p->tm_min += minutes ; tm_p->tm_hour += hour ;

	if (tm_p->tm_min >= 60) { tm_p->tm_min -= 60 ; tm_p->tm_hour++ ; }
	else if (tm_p->tm_min <0) { tm_p->tm_min += 60 ; tm_p->tm_hour-- ; }

	if (tm_p->tm_hour >= 24 )
		{
		tm_p->tm_hour -= 24 ;
		tm_p->tm_wday += 1 ;
		tm_p->tm_mday += 1 ;
		tm_p->tm_yday += 1 ;
		}
	else if (tm_p->tm_hour <0)
		{
		tm_p->tm_hour += 24 ;
		tm_p->tm_wday -= 1 ;
		tm_p->tm_mday -= 1 ;
		tm_p->tm_yday -= 1 ;
		}

	if (tm_p->tm_wday > 6) tm_p->tm_wday = 0 ;
	else if (tm_p->tm_wday < 0) tm_p->tm_wday = 6 ;

#define LP_YR(yr)		((((yr)%4 == 0 && (yr) %100 != 0)||(yr) % 400 \
					== 0)?1:0)
#define LEAP_YEAR(yr)		LP_YR((yr)+1970)
#define DAYS_IN_MONTH(mth,yr)	(days_in_month[(mth)+1]+(((mth)!=1)?0: \
					LEAP_YEAR((yr))))
#define DAYS_IN_YEAR(yr)	(365 + LEAP_YEAR(yr))

	/* Next bit would be more complex if january or december varied
	   like february, as it we dont have to re-evaluate tm_year before
	   supplying it to DAYS_IN_MONTH in the case where moving a few hours
	   around the globe changes the year. */
	if (tm_p->tm_mday > DAYS_IN_MONTH(tm_p->tm_mon,tm_p->tm_year) )
		{ tm_p->tm_mday = 1 ; tm_p->tm_mon++ ; }
	else if (tm_p->tm_mday == 0 )
		tm_p->tm_mday = DAYS_IN_MONTH(--tm_p->tm_mon,tm_p->tm_year) ;

	if (tm_p->tm_mon == 12)
		{ tm_p->tm_mon = 0 ; tm_p->tm_year++ ; tm_p->tm_yday = 0 ; }
	else if (tm_p->tm_mon == -1)
		{
		tm_p->tm_mon = 11 ; tm_p->tm_year-- ;
		tm_p->tm_yday=DAYS_IN_YEAR(tm_p->tm_year) - 1 ; /* JJJ IS -1 CORRECT */
		}

	}

/* timec is functionaly somewhat similarity to an inverse of libc.a ctime()
	Takes a date string as found in the Date: field of news articles,
	(In the wide variety formats the world's Unixes generate)
	& converts to a struct tm field
	Return < 0 means error detected.
	Examples of acceptable input format :
		Thu, 7 Dec 89 10:19 PST
		Thu, 16 Nov 89 05:13:50 GMT
		Fri, 17 Nov 89 10:25 PST
		Sat, 18 Nov 89 1:52:38 PST
		Wed, 4 Apr 90 13:37:24 EDT
		Wed, 13 Mar 1991 21:05:38 +1000
		Thu Nov 23 01:57:39 1989
		22 Nov 89 08:34:40 PST (Wed)
		Tue,  1 Sep 1992 11:43:56 -0400
		Mon,  6 Jul 92 19:23:49 -0400 (EDT)
		Wed,  2 Sep 1992 09:04:24 -0400
		Thu,  3 Sep 1992 08:17:58 -0400
		23 Sep 92 10:36
		Known problem with elm.pl.20. fixed in pl22 :
			Sat, 17 Apr 1993 21:57:08 -40962758 (CDT)
	*/
timec(text_p,file_name,tm_p)
	char *text_p ;
	char *file_name ;
	struct	tm	*tm_p ;		/* Where to write date struct to */
	{
	char		*start_p ;
	char		timezone[LINE_LN] ;	/* big so junk wont core dump */
	char		month_str[MONTH_LN] ;
	FLAG		yr_after_f ;		/* 1 if year after hour:day */
	int		minute = 0 ;	/* offset */

	tm_p->tm_yday = 0 ; /* Not available from Date: field, and I dont bother
				to calculate it, as I dont need it, is this
				procedure is absorbed in library, write code
				to add this functionality. */
	tm_p->tm_isdst = 0 ; /* I dont use this for anthing, it is set
				here as a reminder in case the code is copied
				&/or some other functionality is required. */
	tm_p->tm_sec = 0 ; /* Not always available from Date: field */

	start_p = text_p ;
	while (isspace(*text_p)) text_p++ ;
	if (*text_p == '\0') return(-1) ;

#ifdef DEBUG	/* { */
	printf("Skip Day Name if present.\n") ;
#endif		/* } */
	if ((tm_p->tm_wday = day_atoi(text_p)) >= 0)
		{
		while ((!isspace(*text_p)) && (*text_p != '\0'))
			{
#ifdef DEBUG	/* { */
			printf("Skipping \'%c\'\n",*text_p) ;
#endif		/* } */
			text_p++ ;
			}
#ifdef DEBUG	/* { */
		printf("Skip 1 or 2 space chars after comma after Day Name.\n") ;
#endif		/* } */
		while (isspace(*text_p))
			{
#ifdef DEBUG	/* { */
			printf("Skipping \'%c\'\n",*text_p) ;
#endif		/* } */
			text_p++ ;
			}
		}
	/* JJ note tm_p->tm_wday might never attain a more useful value than
		-1 , as later code does not try again for day name */

	if (is_empty_date(text_p,file_name,
  "after skipping day name, before detecting order of day_in_month & month"
		) ) return(-1) ;
	if (!isupper(*text_p))
		{

#ifdef DEBUG	/* { */
		printf(txt_dbg_ctl, txt_dbg_is, txt_dbg_day,
			txt_dbg_dm, txt_dbg_such_as, txt_dbg_dm_e,
			txt_dbg_opp, txt_dbg_day,
			txt_dbg_md, txt_dbg_such_as, txt_dbg_md_e
			) ;
#endif		/* } */
		yr_after_f =0 ;
		if ( 2 != sscanf(text_p,"%d %s",&tm_p->tm_mday,month_str) )
		(void)is_empty_date("\0",file_name,
			"failure scanning day in month, and month name") ;
		}
	else	{
#ifdef DEBUG	/* { */
		printf(txt_dbg_ctl, txt_dbg_is, txt_dbg_day,
			txt_dbg_md, txt_dbg_such_as, txt_dbg_md_e,
			txt_dbg_opp, txt_dbg_day,
			txt_dbg_dm, txt_dbg_such_as, txt_dbg_dm_e
			) ;
#endif		/* } */
		yr_after_f =1 ;
		if ( 2 != sscanf(text_p,"%s %d",month_str,&tm_p->tm_mday) )
		(void)is_empty_date("\0",file_name,
			"failure scanning month name, and day in month") ;
		}
#ifdef DEBUG	/* { */
	printf("Checking month.\n") ;
#endif		/* } */
	if ((tm_p->tm_mon = month_atoi(month_str)) < 0)
		{
		(void)is_empty_date("\0",file_name,
			"failure checking month name") ;
		return(-1) ;
		}
#ifdef DEBUG	/* { */
	printf("Checking day in month.\n") ;
#endif		/* } */
	if ((tm_p->tm_mday <= 0) || ( tm_p->tm_mday > 31 ))
		{
		(void)is_empty_date("\0",file_name,
			"failure checking day in month") ;
		return(-1) ;
		}
#ifdef DEBUG	/* { */
	printf("Skip 1st element of month & day in month pair.\n") ;
#endif		/* } */
	while ((!isspace(*text_p)) && (*text_p != '\0')) text_p++ ;
	while (isspace(*text_p)) text_p++ ;
	if (is_empty_date(text_p,file_name,
  "after skiping 1st element of month & day in month pair, to get 2nd element"))
		return(-1) ;
#ifdef DEBUG	/* { */
	printf("Skip 2nd element of month & day in month pair.\n") ;
#endif		/* } */
	while (!isspace(*text_p) && (*text_p != '\0')) text_p++ ;
	while (isspace(*text_p)) text_p++ ;
	if (is_empty_date(text_p,file_name,
  "after skiping 2nd element of month & day in month pair, to get time of day"))
		return(-1) ;
#ifdef DEBUG	/* { */
	printf("At time of day.\n") ;
#endif		/* } */
	if (!yr_after_f)
		{
#ifdef DEBUG	/* { */
		printf("Get year.\n") ;
#endif		/* } */
		/* Date: Thu, 7 Dec 89 10:19 PST */
		if (1 != sscanf(text_p,"%d",&tm_p->tm_year))
			{
			(void)is_empty_date("\0",file_name,"getting year") ;
			return(-1) ;
			}
#ifdef DEBUG	/* { */
		printf("Skip year.\n") ;
#endif		/* } */
		while (isdigit(*text_p)) text_p++ ;
		while (isspace(*text_p)) text_p++ ;
		if (is_empty_date(text_p,file_name,
			"after skipping year")) return(-1) ;
		}

#ifdef DEBUG	/* { */
	printf("Get hour & minute.\n") ;
#endif		/* } */
	if (2 != sscanf(text_p,"%d:%d", &tm_p->tm_hour,&tm_p->tm_min))
		{
		(void)is_empty_date("\0",file_name,"getting hour & minute") ;
		return(-1) ;
		}
#ifdef DEBUG	/* { */
	printf("Check hours & minutes.\n") ;
#endif		/* } */
	if ( (tm_p->tm_hour < 0) || ( tm_p->tm_hour >= 24 ) ||
		( tm_p->tm_min < 0 ) || ( tm_p->tm_min >= 60 ))
			{
			(void)is_empty_date("\0",file_name,
				"checking hours & minutes") ;
			return(-1) ;
			}

#ifdef DEBUG	/* { */
	printf("Skipping hours & minutes.\n") ;
#endif		/* } */
	while ((*text_p != ':') && (*text_p != '\0')) text_p++ ;
	if (is_empty_date(text_p++,file_name,"after skipping hours"))
		return(-1) ;
	while (isdigit(*text_p)) text_p++ ;
	/* cant do this because seconds are optional ...
		if (is_empty_date(text_p,file_name,"after skipping minutes"))
			return(-1) ;
	*/

#ifdef DEBUG	/* { */
	printf("If seconds present, get them.\n") ;
#endif		/* } */
	(void) strcpy(sec_str,"__") ;
	if (*text_p == ':')
		{
		if ( 1 != sscanf(++text_p,"%d",&tm_p->tm_sec))
			{
			(void)is_empty_date("\0",file_name,"skipping seconds") ;
			return(-1) ;
			}
#ifdef DEBUG	/* { */
		printf("Check seconds.\n") ;
#endif		/* } */
		if (( tm_p->tm_sec < 0 ) || ( tm_p->tm_sec >= 60 ))
			{
			(void)is_empty_date("\0",file_name,
				"checking seconds value") ;
			return(-1) ;
			}
		(void) sprintf(sec_str,"%02u",tm_p->tm_sec) ;
#ifdef DEBUG	/* { */
		printf("Skip past seconds.\n") ;
#endif		/* } */
		while (isdigit(*text_p)) text_p++ ;
		}
	while (isspace(*text_p)) text_p++ ;

	/* If year occured before hour, and no time zone, & no day name,
	   we might now be at end of string, so can no longer always call
	   is_empty_date.
	*/
	if (yr_after_f)
		{
#ifdef DEBUG	/* { */
		printf("Get year.\n") ;
#endif		/* } */
		if (is_empty_date(text_p,file_name,
			"looking for year at end of string"))
			return(-1) ;
		if ( 1!= sscanf(text_p,"%d",&tm_p->tm_year))
			{
			(void)is_empty_date("\0",file_name,
				"scanning year at end of string") ;
			return(-1) ;
			}
		/* mail date may be 1991,or 91 */
#ifdef DEBUG	/* { */
		printf("Skip year.\n") ;
#endif		/* } */
		while (isdigit(*text_p)) text_p++ ;
		while (isspace(*text_p)) text_p++ ;
		}

#ifdef DEBUG	/* { */
	printf("Check year.\n") ;
#endif		/* } */
#ifdef unix	/* { */
#define YEAR_ZERO	70	/* 1970 Offset added by unix, when it was born */
#endif		/* } */
#ifdef MSDOS	/* { */
#define YEAR_ZERO	80
				/* 1980 Offset added by Msdos, when it was born,
				   (trust msdos to be awkward!) */
#endif		/* } */
	if ( ( tm_p->tm_year < 0 ) ||
		( ( tm_p->tm_year >= 150 )
			/* 150 should conceptually be 100, but changed
			   from 100 to avoid sleepless
			   nights at the imminent turn of the century!
			   (ie now in 1992 many machines call it 92,
			   my bet is some in 2000 will call it 100,
			   & others 0, (& hopefully most 2000)
			*/
		&& ( tm_p->tm_year < YEAR_ZERO ) ) )
		{
		(void)is_empty_date("\0",file_name,"check year value") ;
		return(-1) ;
		}
	if (tm_p->tm_year >= 1900 + YEAR_ZERO)
		tm_p->tm_year -= ( 1900 + YEAR_ZERO ) ;
	else tm_p->tm_year -= YEAR_ZERO ;
#ifdef DEBUG	/* { */
	printf("Year %d.\n",1900 + YEAR_ZERO + tm_p->tm_year) ;
#endif		/* } */

#ifdef DEBUG	/* { */
	printf("Get optional timezone.\n") ;
#endif		/* } */
	timezone[0] = '\0' ;
	if ((*text_p != '\n') && (*text_p != '\0'))
		{
		if ( 1 == sscanf(text_p,"%s",timezone))
			{
			minute = tz_atoi(timezone,file_name) ;
#define BAD_TZ	( -24 * 60)
			if (minute == BAD_TZ)
				{
				if (!no_warnings) fprintf(my_err,
			"%s: In %s, ignored incomprehensible timezone %s\n",
					*ARGV,file_name,timezone) ;
				}
			else	{
				adjust(-minute,tm_p) ;
				}
			}
		}
	if (timezone[0] == '\0')
		{
		if (!no_warnings) fprintf(my_err,
			"%s: %s: No timezone in %s, assuming GMT for: %s",
			*ARGV, txt_warning,file_name,start_p) ;
		/* Cant return(-1) ; that would indicate total date failure */
		}
	return(0) ; /* JJ later could make this long seconds since
			00:00 1st Jan 1970 GMT */
	}

/* Convert a time zone name or digit string to
   how many minutes ahead of GMT the local zone is.
   Examples:	IN		OUT
		-0800		-8 * 60		(California)
		GMT+4:30	Same as +4:30
		+4:30		+4*60 +30	(Afghanistan)
		EST		-5 * 60		(Eastern USA)
		WET DST		+1 * 60		(West europe summer time)
		15:07-EDT	Same as EDT
*/
	int
tz_atoi(timezone,filename)
	char	*timezone, *filename ;
	{
	register char	*p_in ;
	char		tmp_str[LINE_LN] ;
	char		chars[4]	 ;
	register char	*p_out = chars ;
	register char	ch ;
	struct tz	*tz_p ;
	extern struct tz tz_array[] ;
		/* number of minutes local is ahead (+) or behind (-) GMT */
	int	hour, minute ;
	int	east_ahead = 1 ;	/* west -1 */
#ifdef DEBUG	/* { */
	printf("tz_atoi called with (%s)\n",timezone) ;
#endif		/* } */

	for ( p_in=timezone ; isspace(*p_in) ;p_in++ ) ;
	 /* should not be called with white space */
	if (*p_in == '\0' ) return(BAD_TZ ) ;
	(void) strcpy(tmp_str,p_in) ;
	/* upper case only */
	for (p_in = tmp_str ; *p_in != '\0' ; p_in++)
		if (islower(*p_in)) *p_in = toupper(*p_in) ;
	p_in = tmp_str ;

	if (strnequ(tmp_str,"GMT",3))
		{
		p_in += 3 ;	/* convert syntax type GMT+4:30  to +4:30 */
		if (isspace(ch = *p_in) || (ch == '\0')) return(0) ;
		}
	if ((((ch = *p_in) == '+') || (ch  == '-')) && isdigit(*(p_in+1) ) )
		{
#ifdef DEBUG	/* { */
	printf("syntax type -0200 or +1300 or +01 or -1.\n");
#endif		/* } */
		if ( ch == '-') east_ahead = -1 ; ++p_in ;
		*p_out++ = *p_in++ ; /* first (possibly decade) hour digit */
		*p_out = '\0' ;
		/* second hour digit, or ':', or space */
		ch = *p_in++ ;
		if (isspace(ch) || (ch == '\0'))
			{
			hour = atoi(chars) ;
			/* chars[0] - '0' would be faster but not with ebcdic */
#ifdef DEBUG	/* { */
			printf("tz_atoi A returning %d\n",
				east_ahead * 60 * hour) ;
#endif		/* } */
			return( east_ahead * 60 * hour ) ;
			}
		else if (isdigit(ch))
			{	 /* second hour digit */
			*p_out++ = ch ; *p_out = '\0' ;
			ch = *p_in++ ;
			if (isspace(ch) || (ch == '\0'))
				{
				hour = east_ahead * atoi(chars) ;
				if ((hour > 23) || (hour < -11 ))
					return(BAD_TZ) ;
#ifdef DEBUG	/* { */
				printf("tz_atoi B returning %d\n",
					60 * hour) ;
#endif		/* } */
				return( 60 * hour) ;
				}
			}
		else if (ch != ':')  return(BAD_TZ) ;
		/* calculate hour, free chars */
		hour = atoi(chars) ;
		/* skip optional ':' after 1st or 2nd hour digit */
		if (ch == ':') ch = *p_in++ ;
		/* expect 1 or 2 minute digits, no more syntax checking */
		if (!isdigit(ch)) return(BAD_TZ) ;
		minute = atoi(p_in) ;
		if ((minute > 59) || (minute < 0)) return(BAD_TZ) ;
		minute = east_ahead * (minute + 60 * hour) ;
		if ((minute < (-12 * 60 )) || ( minute > ( 24 * 60 - 1 )))
			return(BAD_TZ) ;
#ifdef DEBUG	/* { */
		printf("tz_atoi C returning %d\n", minute) ;
#endif		/* } */
		return(minute) ;
		}
	else	{ /* Syntax type "EST" or "WET DST" or "-EDT" */
		if (*p_in == '-') p_in++ ;
		for (tz_p = tz_array ; (tz_p->tz_name != (char *)0) &&
			strncmp(tz_p->tz_name, p_in, strlen(tz_p->tz_name)) ;
			tz_p++ ) ;
		if (tz_p->tz_name != (char *)0)
			{
#ifdef DEBUG	/* { */
			printf("tz_atoi D returning %d\n",
				tz_p->tz_minutes ) ;
#endif		/* } */
			return(tz_p->tz_minutes) ;
			}
		else return(BAD_TZ) ;
		}
	printf( "%s: %s Ignoring unusual syntax in timezone of %s.\n%s\n",
		*ARGV,txt_warning,filename,timezone);
	return(0);
	}
/* end of file */

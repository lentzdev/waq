/*=============================================================================
  Wide Area Query - A project of LENTZ SOFTWARE-DEVELOPMENT & EuroBaud Software
  Design & COPYRIGHT (C) 1992 by A.G.Lentz & T.J.Caulfeild; ALL RIGHTS RESERVED

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/
#include "includes.h"


#ifdef MSDOS	/* --------------------------------------------------------- */

static char ff_dta[58];

char *ffirst (char *filespec)
{
#if defined(__COMPACT__) || defined(__LARGE__) || defined(__HUGE__)
	struct SREGS sregs;
#endif
	union  REGS  regs;

	regs.h.ah = 0x1a;
#if defined(__COMPACT__) || defined(__LARGE__) || defined(__HUGE__)
	sregs.ds  = FP_SEG(ff_dta);
	regs.x.dx = FP_OFF(ff_dta);
	intdosx(&regs,&regs,&sregs);
#else
	regs.x.dx = (word) ff_dta;
	intdos(&regs,&regs);
#endif

	regs.x.cx = 0;
	regs.h.ah = 0x4e;
#if defined(__COMPACT__) || defined(__LARGE__) || defined(__HUGE__)
	sregs.ds  = FP_SEG(filespec);
	regs.x.dx = FP_OFF(filespec);
	intdosx(&regs,&regs,&sregs);
#else
	regs.x.dx = (word) filespec;
	intdos(&regs,&regs);
#endif
	if (regs.x.cflag)
	   return (NULL);
	return (ff_dta + 0x1e);
}/*ffirst()*/


char *fnext (void)
{
#if defined(__COMPACT__) || defined(__LARGE__) || defined(__HUGE__)
	struct SREGS sregs;
#endif
	union  REGS  regs;

	regs.h.ah = 0x1a;
#if defined(__COMPACT__) || defined(__LARGE__) || defined(__HUGE__)
	sregs.ds  = FP_SEG(ff_dta);
	regs.x.dx = FP_OFF(ff_dta);
	intdosx(&regs,&regs,&sregs);
#else
	regs.x.dx = (word) ff_dta;
	intdos(&regs,&regs);
#endif

	regs.h.ah = 0x4f;
#if defined(__COMPACT__) || defined(__LARGE__) || defined(__HUGE__)
	intdosx(&regs,&regs,&sregs);
#else
	intdos(&regs,&regs);
#endif
	if (regs.x.cflag)
	   return (NULL);
	return (ff_dta + 0x1e);
}/*fnext()*/
#endif		/* --------------------------------------------------------- */


#ifdef TOS	/* --------------------------------------------------------- */

static char fname[20];		  /* name of the file found by ffirst, fnext */
static DTA  tdta;		  /* dta buffer 			     */

char *ffirst (char *name)			/* find filename	     */
{
	DTA *hisdta = Fgetdta();		/* file info stored here     */
	char *q, *p;				/* used for copying	     */
	char *temp;

	Fsetdta(&tdta);

	if (Fsfirst(name,7) != -33L) {
	   p = fname;
	   for (q = tdta.d_fname; *q; ) *p++ = *q++;
	   *p = '\0';
	   temp = fname;
	}
	else
	   temp = NULL;

	Fsetdta(hisdta);

	return (temp);
}/*ffirst()*/


char *fnext (void)			       /* find filename (see ffirst) */
{
	DTA *hisdta = Fgetdta();		/* file info stored here     */
	char *q, *p;				/* used for copying	     */
	char *temp;

	Fsetdta(&tdta);

	if (Fsnext() == 0L) {
	   p = fname;
	   for (q = tdta.d_fname; *q; ) *p++ = *q++;
	   *p = '\0';
	   temp = fname;
	}
	else
	   temp= NULL;

	Fsetdta(hisdta);

	return (temp);
}/*fnext()*/
#endif		/* --------------------------------------------------------- */


#ifdef XENIX	/* --------------------------------------------------------- */

static char	      filename[LFNMAX];
static DIR	     *dirp = NULL;
static struct direct *dp;

char *ffirst (char *filespec)
{
	char dirname[LPNMAX];

	if (dirp != NULL) {
	   closedir(dirp);
	   dirp = NULL;
	}

	splitpath(filespec,dirname,filename);
	if (strlen(dirname) > 1 && dirname[strlen(dirname) - 1] == '/')
	   dirname[strlen(dirname) - 1] = '\0';

	if (*dirname == '\0')
	   strcpy(dirname, ".");

	if ((dirp = opendir(dirname)) == NULL)
	   return (NULL);

	for (dp = readdir(dirp); dp != NULL; dp = readdir(dp))
	    if (patmat(dp->d_name,filename)) return (dp->d_name);

	closedir(dirp);
	dirp = NULL;

	return (NULL);
}/*ffirst()*/


char *fnext (void)
{
	if (dirp != NULL)  {
	   for (dp = readdir(dirp); dp != NULL; dp = readdir(dp))
	       if (patmat(dp->d_name,filename)) return (dp->d_name);
	   closedir(dirp);
	   dirp = NULL;
	}

	return (NULL);
}/*fnext()*/
#endif		/* --------------------------------------------------------- */


void unique_name (char *pathname)
{
	static char *suffix = ".000";
	register char *p;
	register int   n;

	if (ffirst(pathname)) {
	   p = pathname;
	   while (*p && *p != '.') p++;
	   for (n = 0; n < 4; n++) {
	       if (!*p) {
		  *p	 = suffix[n];
		  *(++p) = '\0';
	       }
	       else p++;
	   }

	   while (ffirst(pathname)) {
		 p = pathname + strlen(pathname) - 1;
		 if (!isdigit(*p)) *p = '0';
		 else {
		    for (n = 3; n--; ) {
			if (!isdigit(*p)) *p = '0';
			if (++(*p) <= '9') break;
			else		   *p-- = '0';
		    }/*for*/
		 }
	   }/*while*/
	}/*if(exist)*/
}/*unique_name()*/


/* end of ffind.c */

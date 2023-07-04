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


static char  *wdays[7]	 = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
static char  *months[12] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun",
			     "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };
static byte   mdays[12]  = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
#define DAY1980 (10 * 365 + 2)	/* 10 years since 1970, including 2 leapdays */
#define SEC1980 (86400L * (long) DAY1980)  /* secs per day * days since 1970 */


/*---------------------------------------------------------------------------*/
static int weekday (int year, int month, int day)
{
	long df1;

	df1 = 365L * year + 31L * month + day + 1;
	if (month > 1) {
	   df1 -= (4 * (month + 1) + 23 ) / 10;
	   year++;
	}
	df1 += (year - 1) / 4;
	df1 -= (3 * ((year - 1) / 100) + 1) / 4;	      /* friday == 1 */
	df1 += 4;

	return ((int) (df1 % 7));
}/*weekday()*/


/*---------------------------------------------------------------------------*/
long datetostamp (char *s)
{
	long t;
	int year, month, day, hour, min, sec;
	register word i;
	int days, hours;

	if (strlen(s) != 19 ||
	    sscanf(s,"%02d-%02d-%04d %02d:%02d:%02d",&day,&month,&year,&hour,&min,&sec) != 6 ||
	    day < 1 || day > 31 || month < 1 || month > 12 ||
	    year < 1992 || year > 2030 || hour < 0 || hour > 23 ||
	    min < 0 || min > 59 || sec < 0 || sec > 59)
	   return (0L);

	t = SEC1980;				/* Convert 1980 to 1970 base */
	i = year - 1980;			/* years since 1980 (= leap) */
	t += (i >> 2) * (1461L * 24L * 60L * 60L);
	t += (i & 3) * (24L * 60L * 60L * 365L);
	if (i & 3)
	   t += 24L * 3600L;
	days = 0;
	i = month - 1;					    /* Add in months */
	while (i > 0)
	      days += mdays[--i];
	days += day - 1;
	if ((month > 2) && ((year & 3) == 0))
	   days++;				   /* Currently in leap year */
	hours = days * 24 + hour;			       /* Find hours */
	t += hours * 3600L;
	t += 60L * min + sec;

	return (t);
}/*datetostamp()*/


/*---------------------------------------------------------------------------*/
char *stamptodate (long t)
{
	int year, month, day, hour, min, sec;
	static char datebuf[25];

	t -= SEC1980;					/* Start in 1980     */
	sec = (int) (t % 60);				/* Store seconds     */
	t /= 60;					/* Time in minutes   */
	min = (int) (t % 60);				/* Store minutes     */
	t /= 60;					/* Time in hours     */
	year = 1980 + ((int) ((t / (1461L * 24L)) << 2));
	t %= 1461L * 24L;
	if (t > 366 * 24) {
	   t -= 366 * 24;
	   year++;
	   year += (int) (t / (365 * 24));
	   t %= 365 * 24;
	}
	hour = (int) (t % 24);
	t /= 24;					/* Time in days      */
	t++;
	if ((year & 3) == 0) {
	   if (t > 60)
	      t--;
	   else if (t == 60) {
	      month = 1;
	      day = 29;
	      goto fini;
	   }
	}
	for (month = 0; mdays[month] < t; month++)
	    t -= mdays[month];
	day = (int) t;

fini:	sprintf(datebuf,"%3s %2d %3s %04d %02d:%02d:%02d",
		wdays[weekday(year,month,day)], day, months[month], year,
		hour, min, sec);

	return (datebuf);
}/*stamptodate()*/


/* end of stamp.c */

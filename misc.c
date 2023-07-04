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


char *strip (char *s)
{
	register char *p, *q;

	p = q = s;
	while (isspace(*p)) p++;
	while (*p) *q++ = *p++;
	*q = '\0';
	if (q > s) {
	   while (isspace(*--q));
	   *++q = '\0';
	}

	return (s);
}/*strip()*/


void splitpath (char *pathname, char *path, char *name)
{
	register char *p, *q;

	for (p = pathname; *p; p++);
	while (p != pathname && *p != ':' && *p != '\\' && *p != '/') p--;
	if (*p == ':' || *p == '\\' || *p == '/') p++;
	q = pathname;
	while (q != p) *path++ = *q++;
	*path = '\0';
	strcpy(name,p);
}/*splitpath()*/


void mergepath (char *pathname, char *path, char *name)
{
	strcpy(pathname,path);
	strcat(pathname,name);
}/*mergepath()*/


#ifndef ANSIC
char *strlwr (char *s)
{
	register char *p = s;

	while (*p) *p++ = tolower(*p);

	return (s);
}/*strlwr()*/
#endif


#ifndef MSDOS
int stricmp (const char *s1, const char *s2)
{
	for (; *s1 && tolower(*s1) == tolower(*s2); s1++, s2++);

	return (*s1 - *s2);
}/*strlwr()*/
#endif


boolean patmat (char *raw, char *pat)	     /* Pattern matching - recursive */
{					/* **** Case SENSITIVE version! **** */
	if (!*pat)
	   return (*raw ? false : true);

	if (*pat == '*') {
	   if (!*++pat)
	      return (true);
	   do if ((*raw == *pat || *pat == '?') && patmat(raw + 1,pat + 1))
		 return (true);
	   while (*raw++);
	}
	else if (*raw && (*pat == '?' || *pat == *raw))
	   return (patmat(++raw,++pat));

	return (false);
}/*patmat()*/


/* end of misc.c */

/*=============================================================================
  Wide Area Query - A project of LENTZ SOFTWARE-DEVELOPMENT & EuroBaud Software
  Design & COPYRIGHT (C) 1992 by A.G.Lentz & T.J.Caulfeild; ALL RIGHTS RESERVED
=============================================================================*/

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
#include <stdio.h>
#ifdef	ANSIC
#include <stdlib.h>
#endif
#include <stdarg.h>
#ifdef	MSDOS
#define _PORT_DEFS	    /* This piece of BC++'s dos.h has identifiers >8 */
#include <dos.h>
#endif
#ifdef	XENIX
#include <unistd.h>
#include <sys\ndir.h>
#endif
#ifdef	TOS
#include <tos.h>
#endif
#include <time.h>
#include <string.h>
#include <signal.h>
#include <ctype.h>
#include "2types.h"
#ifdef __BORLANDC__
#pragma hdrstop 	    /* Stop Borland C++ 2.0 precompiled headers here */
#endif


/* end of includes.h */

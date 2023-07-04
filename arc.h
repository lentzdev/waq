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


#ifdef MSDOS
#  define OS_KEYWORD "DOS"
#else
#  ifdef OS2
#    define OS_KEYWORD "OS2"
#  else
#    ifdef XENIX
#      define OS_KEYWORD "XENIX"
#    else
#      ifdef AMIGA
#	 define OS_KEYWORD "AMIGA"
#      else
#	 ifdef ATARI
#	   define OS_KEYWORD "ATARI"
#	 else
#	   define OS_KEYWORD "ALL"
#	 endif
#      endif
#    endif
#  endif
#endif


#define ARC_NAME   32
#define ARC_IDENT  64
#define ARC_CMD   128


typedef struct _arc_list {
	char		  name	   [ARC_NAME  + 1];
	long		  identofs;
	byte		  identlen;
	char		  identseq [ARC_IDENT + 1];
	char		  add	   [ARC_CMD   + 1];
	char		  extract  [ARC_CMD   + 1];
	struct _arc_list *next;
	struct _arc_list *prev;
} ARC_LIST;


/* end of arc.h */

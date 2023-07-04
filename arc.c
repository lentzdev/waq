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
#include "arc.h"
#include "waq.h"


static ARC_LIST *arc_first = NULL;
static ARC_LIST *arc_last  = NULL;

extern char  buffer[];
extern FILE *fp;
extern void *mycalloc (word nbytes);


#define CFG_SPACE " \t\r\n\032"
#define CFG_SEP   ", \t\r\n\032"
#define CFG_EOL   "\r\n\032"


/* ------------------------------------------------------------------------- */
void arc_cfg (void)
{
	ARC_LIST *arc_cur = NULL;
	char *p;
	byte i, n;

	if (arc_first != NULL)
	   return;

	if ((fp = fopen("compress.cfg",TREAD)) == NULL)
	   return;

	while (fgets(buffer,MAX_BUFFER,fp) != NULL) {
	      if ((p = strchr(buffer,';')) != NULL) *p = '\0';
	      p = strtok(buffer,CFG_SPACE);
again:	      if (!p) continue;
	      if (!stricmp(p,OS_KEYWORD)) {
		 p = strtok(NULL,CFG_SPACE);
		 goto again;
	      }

	      if (arc_cur == NULL) {
		 if (stricmp(p,"Archiver")) continue;
		 if ((p = strtok(NULL,CFG_SPACE)) == NULL) continue;
		 if (strlen(strip(p)) > ARC_NAME) continue;
		 arc_cur = (ARC_LIST *) mycalloc(sizeof (ARC_LIST));
		 if (arc_cur == NULL) continue;
		 strcpy(arc_cur->name,p);
		 continue;
	      }

	      if (!stricmp(p,"End")) {
		 if (arc_cur != NULL) {
		    if (arc_cur->identlen == 0 ||
			(!arc_cur->add[0] && !arc_cur->extract[0]))
		       free(arc_cur);
		    else {
		       if (arc_first == NULL)
			  arc_first = arc_last = arc_cur;
		       else {
			  arc_last->next = arc_cur;
			  arc_cur->prev = arc_last;
			  arc_last = arc_cur;
		       }
		    }
		    arc_cur = NULL;
		    continue;
		 }
	      }

	      if (!stricmp(p,"Ident")) {
		 if ((p = strtok(NULL,CFG_EOL)) == NULL) continue;
		 if ((p = strtok(p,CFG_SEP)) == NULL) continue;
		 arc_cur->identofs = atol(p);
		 if ((p = strtok(NULL,CFG_SEP)) == NULL) continue;
		 arc_cur->identlen = 0;
		 while (p[0] && p[1] && arc_cur->identlen < ARC_IDENT) {
		       i = tolower(*p++);
		       if ((i -= '0') > 9) i -= ('a' - ':');
		       n = tolower(*p++);
		       if ((n -= '0') > 9) n -= ('a' - ':');
		       if ((i & ~0x0f) || (n & ~0x0f)) {
			  arc_cur->identlen = 0;
			  break;
		       }
		       arc_cur->identseq[arc_cur->identlen++] = ((i << 4) | n);
		 }
		 if (p[0]) arc_cur->identlen = 0;
		 continue;
	      }

	      if (!stricmp(p,"Add")) {
		 if ((p = strtok(NULL,CFG_EOL)) == NULL) continue;
		 if (strlen(strip(p)) > ARC_CMD) continue;
		 strcpy(arc_cur->add,p);
		 continue;
	      }

	      if (!stricmp(p,"Extract")) {
		 if ((p = strtok(NULL,CFG_EOL)) == NULL) continue;
		 if (strlen(strip(p)) > ARC_CMD) continue;
		 strcpy(arc_cur->extract,p);
		 continue;
	      }
	}

	fclose(fp);
	fp = NULL;

	if (arc_cur != NULL)
	   free(arc_cur);

#ifdef ARC_DEBUG
for (arc_cur = arc_first; arc_cur != NULL; arc_cur = arc_cur->next) {
    printf("Archiver %s\n",arc_cur->name);
    printf("         Ident    %ld,",arc_cur->identofs);
    for (i = 0; i < arc_cur->identlen; i++)
	printf("%02x",((uint) arc_cur->identseq[i]) & 0x00ff);
    printf("\n");
    if (arc_cur->add[0])
       printf("         Add      '%s'\n",arc_cur->add);
    if (arc_cur->extract[0])
       printf("         Extract  '%s'\n",arc_cur->extract);
}
#endif
}/*arc_cfg()*/


/* ------------------------------------------------------------------------- */
char *arc_list (boolean compress)
{
	static char buf[MAX_ARC + 1];
	ARC_LIST *arc_cur;

	buf[0] = '\0';
	for (arc_cur = arc_last; arc_cur != NULL; arc_cur = arc_cur->prev) {
	    if (compress &&  !arc_cur->add[0])	   continue;
	    if (!compress && !arc_cur->extract[0]) continue;
	    if ((strlen(buf) + 1 + strlen(arc_cur->name)) > MAX_ARC) break;
	    if (buf[0]) strcat(buf,",");
	    strcat(buf,arc_cur->name);
	}

	return (buf);
}/*arc_list()*/


/* end of arc.c */

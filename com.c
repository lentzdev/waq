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
#include "waq.h"


#ifndef XENIX
static boolean	com_active = false;
#endif
#ifdef MSDOS
static int	com_port;
static boolean	fos_active = false;
#endif
extern EMU_TYPE emu_type;

#define CRLF	    "\r\n"
#define ANSI_CLS    "\033[2J"
#define ANSI_BLINK  "\033[5m"
#define VT52_CLS    "\033E"


#ifdef MSDOS
static int com_func (int func, int opt)
{
	union REGS regs;

	regs.h.ah = func;
	regs.h.al = opt;
	regs.x.dx = com_port;
	int86(0x14,&regs,&regs);
	return (regs.x.ax);
}/*com_func()*/


static int key_func (int func)
{
	union REGS regs;

	regs.h.ah = func;
	regs.x.dx = com_port;
	int86(0x16,&regs,&regs);
	if (func == 1 && (regs.x.flags & 0x40))  /* ZF on keyscan */
	   regs.x.ax = 0;
	return (regs.x.ax);
}/*key_func()*/
#endif


boolean com_init (int port)
{
#ifdef MSDOS
	if (port > 0) com_port = port - 1;
	else	      com_port = 0x00ff;
	if (com_func(4,0) == 0x1954) fos_active = true;
#endif

	if (port > 0) com_active = true;
	return (true);
}/*com_init()*/


void com_deinit (void)
{
#ifdef MSDOS
	if (fos_active) {
	   com_func(5,0);
	   fos_active = false;
	}
#endif
	if (com_active) com_active = false;
}/*com_deinit()*/


boolean com_carrier (void)
{
#ifdef MSDOS
	if (com_active)
	   if (!(com_func(3,0) & 0x0080)) return (false);
#endif

	return (true);
}/*com_carrier()*/


int com_getc (long timeout)
{
#ifdef MSDOS
	long t = time(NULL) + timeout;

	do {
	   if (fos_active)
	      if (com_func(0x0d,0) >= 0) return (com_func(0x0e,0) & 0x7f);
	   else
	      if (key_func(1)) return (key_func(0) & 0x7f);

	   if (com_active) {
	      if (!com_carrier()) break;
	      if (com_func(3,0) & 0x0100) return (com_func(2,0) & 0x7f);
	   }

	   /* could give away timeslice here */
	} while (time(NULL) < t);
#endif

#ifdef XENIX
	char c;

	alarm(timeout > 0 ? timeout : 1);
	if (read(fileno(stdin),&c,1) == 1)
	   return ((int) c & 0x7f);
#endif	      

	return (-1);
}/*com_getc()*/


void com_purge (void)
{
#ifdef XENIX
	ioctl(fileno(stdin),TCFLSH,0);
#else
	while (com_getc(0) >= 0);
#endif
}/*com_purge()*/


void com_flush (void)
{
#ifdef MSDOS
	if (com_active)
	   while (com_carrier() && !(com_func(3,0) & 0x4000));
#endif
}/*com_flush()*/


static void rem_putc (int c)
{
#ifdef XENIX
	putchar(c);
#endif
#ifdef MSDOS
	if (com_active)
	   if (com_carrier()) com_func(1,c);
#endif
}/*rem_putc()*/


static void rem_puts (char *s)
{
	while (*s) rem_putc(*s++);
}/*rem_puts()*/


#ifndef XENIX
static void loc_putc (int c)
{
#ifdef MSDOS
	if (fos_active)
	   com_func(0x13,c);
	else
#endif
	   putchar(c);
}/*loc_putc()*/


static void loc_puts (char *s)
{
	while (*s) loc_putc(*s++);
}/*loc_puts()*/
#endif


static char *avt2ansi (int c)
{
	static char buf[20];
	static char ansitab[] = "04261537"; 

	sprintf(buf, "\033[0;4%c;3%c%sm",
		     ansitab[(c >> 4) & 0x07],
		     ansitab[c & 0x07],
		     ((c >> 3) & 0x01) ? ";1" : "");
	return (buf);
}/*avt2ansi()*/


static void com_putc (int c)
{
	static int state = 0, repchar = -1;

	switch (state) {
	       case  0: switch (c) {
			       case 10: rem_puts(CRLF);
#ifndef XENIX
					loc_puts(CRLF);
#endif
					break;

			       case 12: switch (emu_type) {
					       case EMU_NONE:	com_putc('\n');
								break;
					       case EMU_FF:
					       case EMU_AVATAR: rem_putc(c);
								break;
					       case EMU_ANSI:	rem_puts(ANSI_CLS);
								rem_puts(avt2ansi(AVT_CYAN));
								break;
					       case EMU_VT52:	rem_puts(VT52_CLS);
								break;
					}
#ifndef XENIX
					if (emu_type > EMU_NONE) {
#ifdef MSDOS
					   loc_puts(ANSI_CLS);
					   if (emu_type > EMU_FF)
					      loc_puts(avt2ansi(AVT_CYAN));
#endif
					}
#endif
					break;

			       case 22: if (emu_type == EMU_AVATAR) rem_putc(c);
					state = 22;
					break;

			       case 25: if (emu_type == EMU_AVATAR) rem_putc(c);
					state = 25;
					break;

			       default: if (!isprint(c)) break;
					/* fallthrough to 7,8,13 */
			       case  7:
			       case  8:
			       case 13: rem_putc(c);
#ifndef XENIX
					loc_putc(c);
#endif
					break;
			}
			break;

	       case 22: switch (c) {
			       case 1:	if (emu_type == EMU_AVATAR) rem_putc(c);
					state = 1;
					break;

			       case 2:	switch (emu_type) {
					       case EMU_NONE:
					       case EMU_FF:	break;
					       case EMU_AVATAR: rem_putc(c);
								break;
					       case EMU_ANSI:	rem_puts(ANSI_BLINK);
								break;
					       case EMU_VT52:	/* VT52 blink */
								break;
					}
#ifndef XENIX
					if (emu_type > EMU_FF) {
#ifdef MSDOS
					   loc_puts(ANSI_BLINK);
#endif
					}
#endif
					state = 0;
					break;

			       default: state = 0;
					break;
			}
			break;

	       case  1: c &= 0x7f;
			if (c == 16) break;    /* Max' MECCA - don't ask me! */
			switch (emu_type) {
			       case EMU_NONE:
			       case EMU_FF:	break;
			       case EMU_AVATAR: rem_putc(c);
						break;
			       case EMU_ANSI:	rem_puts(avt2ansi(c));
						break;
			       case EMU_VT52:	/* VT52 colour avt2vt52() */
						break;
			}
#ifndef XENIX
			if (emu_type > EMU_FF) {
#ifdef MSDOS
			   loc_puts(avt2ansi(c));
#endif
			}
#endif
			state = 0;
			break;

	       case 25: if (emu_type == EMU_AVATAR) rem_putc(c);
			if (repchar == -1)
			   repchar = c;
			else {
			   int i;

			   if (emu_type != EMU_AVATAR)
			      for (i = 0; i < c; i++) rem_putc(repchar);

#ifndef XENIX
			   for (i = 0; i < c; i++) loc_putc(repchar);
#endif
			   state = 0;
			   repchar = -1;
			}
			break;
	}
}/*com_putc()*/


int com_printf (char *fmt, ...)
{
	char	buf[1048];
	int	cnt;
	va_list argptr;
	register char *p;

	va_start(argptr,fmt);
	cnt = vsprintf(buf,fmt,argptr);
	va_end(argptr);

	for (p = buf; *p; com_putc((int) *p++));

	return (cnt);
}/*com_printf()*/


/* end of com.c */

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
#include "md5.h"
#define WAQ_RUN
#include "waq.h"


#define VERSION "1.00"


static char	 buffer[MAX_BUFFER + 1];
static FILE	*fp = NULL;
static long	 crc;

static WAQ_LIST *script;
static byte	*code	  = NULL;
static char	*data	  = NULL;
static word	*variable = NULL;
static word	 stack[MAX_STACK];
static boolean	 errors;
static word	 ip;
static word	 sp;

#ifndef XENIX
static int	 port		    = 0;
#endif
static long	 timezone	    = 0L;
static long	 timelimit	    = 0L;
       EMU_TYPE  emu_type	    = EMU_NONE;
static char	 user[MAX_USER + 1] = "";

static WAQ_LIST *list[MAX_ITEMS];
static word	 items = 0;

static word	 withwild = 0;
static word	 nowild   = 0;


/*---------------------------------------------------------------------------*/
#define cls()	     ((emu_type > EMU_NONE) ? 1 : 0)
#define colour()     ((emu_type > EMU_FF)   ? 1 : 0)


/*---------------------------------------------------------------------------*/
static void erl_exit (int erl)
{
	if (erl != ERL_OK) {
	   if (fp != NULL) fclose(fp);
	}
	com_printf(SEQ_GREY);
	com_purge();
	com_flush();
	com_deinit();

	exit (erl);
}/*erl_exit()*/


static void run_error (char *fmt, ...)
{
	va_list argptr;

	fprintf(stderr,"Error %s.%s: ",script->filebase,EXT_COM);
	va_start(argptr,fmt);
	vfprintf(stderr,fmt,argptr);
	va_end(argptr);
	errors = true;
	fprintf(stderr,"\n");
}/*run_error()*/


static void user_break (void)
{
	fprintf(stderr,"*** User break ***\n");
	erl_exit(ERL_BREAK);
}/*user_break()*/


static void crit_on (void)
{
#ifdef XENIX
	signal(SIGHUP,SIG_IGN);
#endif
	signal(SIGINT,SIG_IGN);
}/*crit_on()*/


static void crit_off (void)
{
#ifdef XENIX
	signal(SIGHUP,SIG_DFL);
#endif
	signal(SIGINT,user_break);
}/*crit_off()*/


/*---------------------------------------------------------------------------*/
static void *myalloc (word nbytes)
{
	void *p;

	if ((p = malloc(nbytes)) == NULL) {
	   run_error("Can't allocate memory");
	   erl_exit(ERL_MEMORY);
	}

	return (p);
}/*myalloc()*/


static void *mycalloc (word nbytes)
{
	register void *p = myalloc(nbytes);

	memset((byte *) p,0,(uint) nbytes);
	return (p);
}/*myalloc()*/


/*---------------------------------------------------------------------------*/
static byte read_byte (void)
{
	byte i = (byte) getc(fp);

	crc = crc32byte(crc,i);
	script->hdr_len++;

	return (i);
}/*read_byte()*/

static void read_str (char *s, word size)
{
	fread(s,(uint) size,1,fp);
	crc = crc32block(crc,(byte *) s,size);
	script->hdr_len += size;
}/*read_str()*/

static void read_word (word *w)
{
	*w = read_byte();
	*w |= ((word) read_byte()) << 8;
}/*read_word()*/

static void read_long (long *l)
{
	*l = read_byte();
	*l |= ((long) read_byte()) << 8;
	*l |= ((long) read_byte()) << 16;
	*l |= ((long) read_byte()) << 24;
}/*read_long()*/


static void read_hdr (void)
{
	sprintf(buffer,"%s.%s",script->filebase,EXT_COM);
	if ((fp = fopen(buffer,BREAD)) == NULL) {
	   run_error("Can't open script file");
	   return;
	}

	script->hdr_len = 0;
	crc = crc32init();

	read_str  (  script->info.idstring	, LEN_IDSTR	);
	read_word ( &script->info.revision			);
	read_str  (  script->info.type_id	, LEN_WAQID + 1 );
	read_long ( &script->info.compile_stamp 		);
	read_str  (  script->info.script_name	, MAX_NAME  + 1 );
	read_str  (  script->info.script_desc	, MAX_DESC  + 1 );
	read_long ( &script->info.start_date			);
	read_long ( &script->info.end_date			);
	read_str  (  script->info.return_dest	, MAX_ADR   + 1 );
	read_str  (  script->info.arc_list	, MAX_ARC   + 1 );
	read_long ( &script->info.return_stamp			);
	read_str  (  script->info.return_orig	, MAX_ADR   + 1 );
	read_word ( &script->info.code_length			);
	read_word ( &script->info.data_length			);
	read_word ( &script->info.num_vars			);
	read_word ( &script->info.stored_vars			);
	read_long ( &script->info.file_length			);
	read_long ( &script->info.file_crc			);
	read_long ( &script->info.hdr_crc			);

	if (strcmp(script->info.idstring,IDSTRING))
	   run_error("Not WAQ format");
	else if (script->info.revision != REVISION)
	   run_error("Different format revision (%u instead of %u)",
		     script->info.revision,REVISION);
	else if (strcmp(script->info.type_id,WAQ_ID))
	   run_error("WAQ format but not a compiled script (%s instead of %s)",
		     script->info.type_id,WAQ_ID);
	else if (!crc32test(crc))
	   run_error("Header failed CRC check (%08lx instead of %08lx)",crc,script->info.hdr_crc);
	else if (!script->info.script_name[0] || !script->info.script_desc[0] ||
		 !script->info.start_date || script->info.end_date <= script->info.start_date ||
		 !script->info.return_dest[0] ||
		 script->info.return_stamp || script->info.return_orig[0] ||
		 !script->info.code_length || script->info.code_length > MAX_CODE ||
		 script->info.data_length > MAX_DATA ||
		 !script->info.num_vars || script->info.num_vars > MAX_VAR ||
		 !script->info.stored_vars || script->info.stored_vars > script->info.num_vars ||
		 script->info.file_length < (script->info.code_length + script->info.data_length))
	   run_error("Invalid data/information in WAQ header (DID pass CRC check!)");
	else {
	   fseek(fp,0L,SEEK_END);
	   if (ftell(fp) < (script->hdr_len + script->info.file_length))
	      run_error("File is shorter than length specification in header");
	}

	fclose(fp);
	fp = NULL;
}/*read_hdr()*/


/*---------------------------------------------------------------------------*/
static void add_list (char *filespec)
{
	char path[MAX_BUFFER + 1];
	word item;
	register char *p, *q;

	strlwr(filespec);
	splitpath(filespec,path,buffer);
	if ((q = strchr(&filespec[strlen(path)],'.')) != NULL)
	   *q = '\0';
	if (strchr(filespec,'*') != NULL || strchr(filespec,'?') != NULL)
	   withwild++;
	else
	   nowild++;
	sprintf(buffer,"%s.%s",filespec,EXT_COM);
	if ((p = ffirst(buffer)) == NULL) {
	   fprintf(stderr,"waqrun: No files matching '%s'\n",buffer);
	   return;
	}

	do {
	   if (items >= MAX_ITEMS)
	      break;

	   script = (WAQ_LIST *) mycalloc(sizeof (WAQ_LIST));

	   strlwr(p);
	   if ((q = strchr(p,'.')) != NULL)
	      *q = '\0';
	   mergepath(script->filebase,path,p);

	   errors = false;
	   read_hdr();

	   if (!errors && stricmp(script->info.script_name,p))
	      run_error("Script name '%s' doesn't match in filename '%s.%s'",
			script->info.script_name,script->filebase,EXT_COM);

	   if (!errors) {
	      for (item = 0; item < items; item++) {
		  if (!stricmp(script->info.script_name,list[item]->info.script_name)) {
		     run_error("Duplicate script name '%s'",list[item]->info.script_name);
		     errors = true;
		     break;
		  }
	      }
	   }

	   if (!errors && (time(NULL) + timezone) >= script->info.start_date &&
			  (time(NULL) + timezone) <= script->info.end_date)
	      list[items++] = script;
	   else
	      free(script);
	} while ((p = fnext()) != NULL);
}/*add_list()*/


/*---------------------------------------------------------------------------*/
static void read_code (void)
{
	errors = false;

	sprintf(buffer,"%s.%s",script->filebase,EXT_COM);
	if ((fp = fopen(buffer,BREAD)) == NULL) {
	   run_error("Can't open script file");
	   return;
	}

	fseek(fp,(long) script->hdr_len,SEEK_SET);

	code = (byte *) myalloc(script->info.code_length);
	if (fread(code,(uint) script->info.code_length,1,fp) != 1) {
	   run_error("Can't read script code");
	   goto fini;
	}

	if (script->info.data_length) {
	   data = (char *) myalloc(script->info.data_length);
	   if (fread(data,(uint) script->info.data_length,1,fp) != 1) {
	      run_error("Can't read script data");
	      goto fini;
	   }
	}

	crc = crc32init();
	crc = crc32block(crc, code, script->info.code_length);
	crc = crc32block(crc, (byte *) data, script->info.data_length);
	crc = crc32byte(crc, (byte) script->info.file_crc);
	crc = crc32byte(crc, (byte) (script->info.file_crc >> 8));
	crc = crc32byte(crc, (byte) (script->info.file_crc >> 16));
	crc = crc32byte(crc, (byte) (script->info.file_crc >> 24));
	if (!crc32test(crc))
	   run_error("Code/data failed CRC check (%08lx instead of %08lx)",crc,script->info.file_crc);

fini:	fclose(fp);
	fp = NULL;
}/*read_code()*/


/*---------------------------------------------------------------------------*/
static short do_get (boolean getint, short a)
{
	char buf[7];
	int  c, i;
	long val;
	long timeout, keywarn, inactive, t;

	if (a > 0) timeout = time(NULL) + a;
	else	   timeout = 0L;
	keywarn  = time(NULL) + (4 * 60);
	inactive = keywarn + 60;
	if (timeout > inactive) timeout = inactive;
	a = 0xffff;
	i = 0;
	buf[i] = '\0';

	com_purge();
	com_flush();

	for (;;) {
	    if (!com_carrier()) {
	       run_error("Carrier lost");
	       erl_exit(ERL_CARRIER);
	    }

	    t = time(NULL);

	    if (timeout && t >= timeout) {
	       com_printf("\n<timeout>");
	       break;
	    }
	    else if (keywarn > 0L) {
	       if (t >= keywarn) {
		  com_printf("\nInput timeout - please respond: %s",buf);
		  keywarn = 0L;
	       }
	    }
	    else if (t >= inactive) {
	       com_printf(SEQ_WHITE "\nInactivity timeout\n");
	       erl_exit(ERL_TIMEOUT);
	    }

	    if	    (timeout > 0L && timeout < keywarn) t = timeout  - t;
	    else if (keywarn > 0L)			t = keywarn  - t;
	    else					t = inactive - t;

	    if ((c = com_getc(t)) < 0)
	       continue;

	    if (c == '\r' || c == '\n') {
	       if (getint) {
		  if (i < 1 || (buf[0] == '-' && i < 2)) continue;
		  a = atoi(buf);
		  break;
	       }
	       else if (i == 1) {
		  a = buf[0];
		  break;
	       }
	       continue;
	    }
	    else if (c == '\b') {
	       if (i > 0) {
		  buf[--i] = '\0';
		  com_printf("\b \b");
	       }
	       continue;
	    }
	    else if (getint) {
	       if (c == '-') {
		  if (i > 0) continue;
	       }
	       else if (isdigit(c)) {
		  buf[i] = c;
		  buf[i + 1] = '\0';
		  val = atol(buf);
		  buf[i] = '\0';
		  if (val < -32768L || val > 32767L) continue;
	       }
	       else continue;
	    }
	    else {
	       if (c <= ' ' || c > 126 || i == 1) continue;
	       c = toupper(c);
	    }

	    buf[i++] = c;
	    buf[i] = '\0';
	    com_printf("%c",c);
	}

	com_printf("\n");

	return (a);
}/*do_get()*/


/*---------------------------------------------------------------------------*/
static void write_str (char *s, word size)
{
	fwrite(s,(unsigned int) size,1,fp);
}/*write_str()*/

static void write_word (word w)
{
	putc((byte) w,fp);
	putc((byte) (w >> 8),fp);
}/*write_word()*/

static void write_long (long l)
{
	putc((byte) l,fp);
	putc((byte) (l >>  8),fp);
	putc((byte) (l >> 16),fp);
	putc((byte) (l >> 24),fp);
}/*write_long()*/


static short do_end (void)
{
	com_printf(SEQ_WHITE "\n\nScript end - ");

	if (strlen(user) < 2)
	   com_printf("will not store response information (no username)\n");
	else {
	   MD5 md5;
	   long stamp = time(NULL) + timezone;
	   boolean update = false;

	   md5_init(&md5);
	   md5_update(&md5,(byte *) ID_MD5,(word) (strlen(ID_MD5) + 1));
	   md5_update(&md5,(byte *) user,(word) (strlen (user) + 1));
	   md5_result(&md5);

	   com_printf("storing response information, please wait a moment...\n");
	   crit_on();

	   sprintf(buffer,"%s.%s",script->filebase,EXT_RSP);

	   if (fp != NULL || (fp = fopen(buffer,BRDWR)) != NULL) {
	      fseek(fp,(long) script->hdr_len,SEEK_SET);
	      while (fread(buffer,(uint) LEN_MD5,1,fp) == 1) {
		    if (!strncmp(md5_result(&md5),buffer,LEN_MD5)) {
		       update = true;
		       break;
		    }
		    fseek(fp,(long) (sizeof (long) + script->info.stored_vars * sizeof (word)),SEEK_CUR);
	      }
	      fseek(fp,0L,update ? SEEK_CUR : SEEK_END);
	   }
	   else if ((fp = fopen(buffer,BWRITE)) != NULL) {
	      write_str  ( script->info.idstring      , LEN_IDSTR     );
	      write_word ( script->info.revision		      );
	      write_str  ( WAR_ID		      , LEN_WAQID + 1 );
	      write_long ( script->info.compile_stamp		      );
	      write_str  ( script->info.script_name   , MAX_NAME  + 1 );
	      write_str  ( script->info.script_desc   , MAX_DESC  + 1 );
	      write_long ( script->info.start_date		      );
	      write_long ( script->info.end_date		      );
	      write_str  ( script->info.return_dest   , MAX_ADR   + 1 );
	      write_str  ( script->info.arc_list      , MAX_ARC   + 1 );
	      write_long ( script->info.return_stamp		      );
	      write_str  ( script->info.return_orig   , MAX_ADR   + 1 );
	      write_word ( (word) 0				      );
	      write_word ( (word) 0				      );
	      write_word ( (word) 0				      );
	      write_word ( script->info.stored_vars		      );
	      write_long ( (long) 0L				      );
	      write_long ( (long) 0L				      );
	      write_long ( (long) 0L				      );
	   }
	   else {
	      run_error("Can't open/create response file '%s'",buffer);
	      com_printf("Could not write response information\n");
	   }

	   if (fp != NULL) {
	      word varidx;

	      if (update)
		 com_printf(SEQ_YELLOW "Replacing old response record\n");
	      else {
		 com_printf(SEQ_YELLOW "Appending new response record\n");
		 fwrite(md5_result(&md5),(uint) LEN_MD5,1,fp);
	      }
	      write_long(stamp);
	      for (varidx = 0; varidx < script->info.stored_vars; varidx++)
		  write_word(variable[varidx]);

	      fclose(fp);
	      fp = NULL;
	   }

	   crit_off();
	}

	return (1);
}/*do_end()*/


/*---------------------------------------------------------------------------*/
static short do_quit (void)
{
	com_printf(SEQ_WHITE "\n\nScript quit - will not store response information\n");

	return (1);
}/*do_quit()*/


/*---------------------------------------------------------------------------*/
static short do_enter (void)
{
	int  c;
	long timeout, t;

	timeout = time(NULL) + (2 * 60);

	com_purge();
	com_flush();
	for (;;) {
	    if (!com_carrier()) {
	       run_error("Carrier lost");
	       erl_exit(ERL_CARRIER);
	    }
	    t = timeout - time(NULL);
	    if (t <= 0) {
	       com_printf(SEQ_WHITE "\nInactivity timeout\n");
	       erl_exit(ERL_TIMEOUT);
	    }
	    if ((c = com_getc(t)) < 0)
	       continue;
	    if (c == '\r' || c == '\n')
	       break;
	}

	com_printf("\n");

	return (1);
}/*do_enter()*/


/*---------------------------------------------------------------------------*/
/* Call with: <0 = close, 0 = begin, >0 = next				     */
/* Returns  : <0 = error, 0 = eof  , >0 = readok			     */
static short do_read (short a)
{
	register word varidx;
	register int  c;

	if (fp == NULL) {
	   sprintf(buffer,"%s.%s",script->filebase,EXT_RSP);

	   if ((fp = fopen(buffer,BRDWR)) == NULL ||
	       fseek(fp,(long) script->hdr_len,SEEK_SET) != 0)
	      return (-1);
	}

	if (a < 0 && fp != NULL) {
	   fclose(fp);
	   fp = NULL;
	   return (0);
	}

	if (a == 0) {
	   if (fseek(fp,(long) script->hdr_len,SEEK_SET) != 0)
	      return (-1);
	}

	if (fread(buffer,(uint) (LEN_MD5 + sizeof (long)),1,fp) != 1)
	   return (0);

	for (varidx = 0; varidx < script->info.stored_vars; varidx++) {
	    if ((c = getc(fp)) == EOF) return (-1);
	    variable[varidx] = c;
	    if ((c = getc(fp)) == EOF) return (-1);
	    variable[varidx] |= c << 8;
	}

	return (1);
}/*do_read()*/


/*---------------------------------------------------------------------------*/
static void push (word w)
{
	if (sp < MAX_STACK)
	   stack[sp++] = w;
	else
	   run_error("Stack overflow (IP=%04x)",(uint) ip);
}/*push()*/


static word pop (void)
{
	if (sp > 0)
	   return (stack[--sp]);
	else {
	   run_error("Stack underflow (IP=%04x)",(uint) ip);
	   return (0);
	}
}/*pop()*/


/*---------------------------------------------------------------------------*/
static void do_run (void)
{
	short	 a;
	MNEMONIC op;
	word	 prm;

	errors = false;

	read_code();
	if (errors) {
	   com_printf(SEQ_WHITE "Load error - can not execute script\n");
	   goto fini;
	}

	com_printf(SEQ_CYAN);

	variable = (word *) mycalloc(script->info.num_vars * sizeof (word));
	ip = sp = a = 0;

	do {
	   op = code[ip++];

	   switch (op) {
		  case m_puts:
		  case m_jz: case m_jnz:
		  case m_jmp: case m_call:
		  case m_inc: case m_dec:
		  case m_ldai: case m_lda: case m_sta:
		       prm = code[ip++];
		       prm |= (code[ip++] << 8);
		       break;

		  default:
		       break;
	   }

	   if (ip > script->info.code_length) {
	      run_error("IP out of bounds");
	      continue;
	   }

	   switch (op) {
		  case m_nop:	 break;
		  case m_jz:	 if (a == 0) {
				    if (prm < script->info.code_length)
				       ip = prm;
				    else
				       run_error("JZ address out of bounds (IP=%04x)",(uint) ip - 3);
				 }
				 break;
		  case m_jnz:	 if (a != 0) {
				    if (prm < script->info.code_length)
				       ip = prm;
				    else
				       run_error("JNZ address out of bounds (IP=%04x)",(uint) ip - 3);
				 }
				 break;
		  case m_jmp:	 if (prm < script->info.code_length)
				    ip = prm;
				 else
				    run_error("JMP address out of bounds (IP=%04x)",(uint) ip - 3);
				 break;
		  case m_call:	 if (prm < script->info.code_length) {
				    push(ip);
				    ip = prm;
				 }
				 else
				    run_error("CALL address out of bounds (IP=%04x)",(uint) ip - 3);
				 break;
		  case m_return: ip = pop();
				 if (ip >= script->info.code_length)
				    run_error("RETURN address out of bounds (IP=%04x)",(uint) ip - 1);
				 break;
		  case m_inc:	 if (prm < script->info.num_vars)
				    variable[prm]++;
				 else
				    run_error("INC index out of bounds (IP=%04x)",(uint) ip - 3);
				 break;
		  case m_dec:	 if (prm < script->info.num_vars)
				    variable[prm]--;
				 else
				    run_error("DEC index out of bounds (IP=%04x)",(uint) ip - 3);
				 break;
		  case m_push:	 push(a); break;
		  case m_pop:	 a = pop(); break;
		  case m_ldai:	 a = prm; break;
		  case m_lda:	 if (prm < script->info.num_vars)
				    a = variable[prm];
				 else
				    run_error("LDA index out of bounds (IP=%04x)",(uint) ip - 3);
				 break;
		  case m_sta:	 if (prm < script->info.num_vars)
				    variable[prm] = a;
				 else
				    run_error("STA index out of bounds (IP=%04x)",(uint) ip - 3);
				 break;
		  case m_lognot: a = !a; break;
		  case m_onecom: a = ~a; break;
		  case m_neg:	 a = -a; break;
		  case m_mul:	 a = ((short) pop()) * a; break;
		  case m_div:	 if (a != 0)
				    a = ((short) pop()) / a;
				 else
				    run_error("Division by zero (IP=%04x)",(uint) ip - 1);
				 break;
		  case m_mod:	 a = ((short) pop()) %	a; break;
		  case m_add:	 a = ((short) pop()) +	a; break;
		  case m_sub:	 a = ((short) pop()) -	a; break;
		  case m_shr:	 a =	      pop()  >> a; break;
		  case m_shl:	 a =	      pop()  << a; break;
		  case m_ls:	 a = ((short) pop()) <	a; break;
		  case m_gt:	 a = ((short) pop()) >	a; break;
		  case m_lse:	 a = ((short) pop()) <= a; break;
		  case m_gte:	 a = ((short) pop()) >= a; break;
		  case m_eq:	 a = ((short) pop()) == a; break;
		  case m_ne:	 a = ((short) pop()) != a; break;
		  case m_and:	 a = pop() & a; break;
		  case m_xor:	 a = pop() ^ a; break;
		  case m_or:	 a = pop() | a; break;
		  case m_geti:	 a = do_get(true,a);  break;
		  case m_getc:	 a = do_get(false,a); break;
		  case m_puti:	 a = com_printf("%i",(int) a); break;
		  case m_putc:	 a = com_printf("%c",(int) a); break;
		  case m_puts:	 if (prm < script->info.data_length)
				    a = com_printf(&data[prm]);
				 else
				    run_error("PUTS offset out of bounds (IP=%04x)",(uint) ip - 3);
				 break;
		  case m_end:	 a = do_end();	 goto fini;
		  case m_quit:	 a = do_quit();  goto fini;
		  case m_enter:  a = do_enter(); break;
		  case m_colour: a = colour();	 break;
		  case m_cls:	 a = cls();	 break;
		  case m_read:	 a = do_read(a); break;

		  default:	 run_error("Illegal opcode %02x (IP=%04x)",(uint) op, (uint) ip - 1);
				 break;
	   }
	} while (!errors);

	com_printf(SEQ_WHITE "\n\nExecution error - aborting script\n");

fini:	if (fp != NULL) {
	   fclose(fp);
	   fp = NULL;
	}
	if (code != NULL) free(code);
	code = NULL;
	if (data != NULL) free(data);
	data = NULL;
	if (variable != NULL) free(variable);
	variable = NULL;

	enter();
}/*do_run()*/


/*---------------------------------------------------------------------------*/
static void waq_info (void)
{
	com_printf(SEQ_CLS SEQ_LGREEN "Wide Area Query - A project of LENTZ SOFTWARE-DEVELOPMENT & EuroBaud Software\n");
	com_printf(SEQ_CYAN "WAQRUN Version %s; Wide Area Query Runtime Interpreter (format revision %u)\n",VERSION,REVISION);
	com_printf("Design & COPYRIGHT (C) 1992 by A.G.Lentz & T.J.Caulfeild; ALL RIGHTS RESERVED\n");
	com_printf("\n");
}/*waq_info()*/


static void com_info (void)
{
	com_printf(SEQ_CYAN "Script name   : " SEQ_YELLOW "%s\n",
			    script->info.script_name);
	com_printf(SEQ_CYAN "Description   : " SEQ_YELLOW "%s\n",
			    script->info.script_desc);
	com_printf(SEQ_CYAN "Compile time  : " SEQ_LCYAN  "%s\n",
			    stamptodate(script->info.compile_stamp));
	com_printf(SEQ_CYAN "Start date    : " SEQ_LGREEN "%s" SEQ_CYAN " GMT/UTC\n",
			    stamptodate(script->info.start_date));
	com_printf(SEQ_CYAN "End date      : " SEQ_LGREEN "%s" SEQ_CYAN " GMT/UTC\n",
			    stamptodate(script->info.end_date));
	com_printf(SEQ_CYAN "Return address: " SEQ_LCYAN  "%s\n",
			    script->info.return_dest);
	com_printf("\n");
}/*com_info()*/


static void enter (void)
{
	com_printf(SEQ_WHITE "\nPress <enter> to continue");
	do_enter();
}/*enter()*/


/*---------------------------------------------------------------------------*/
static void sortscripts (void)
{
	register int i, j;
	register int gap;
	register WAQ_LIST *temp;

	for (gap = items >> 1; gap > 0; gap >>= 1) {
	    for (i = gap; i < items; i++) {
		for (j = i - gap;
		     j >= 0 && stricmp(list[j]->info.script_name,list[j + gap]->info.script_name) > 0;
		     j -= gap) {
		    temp = list[j];
		    list[j] = list[j + gap];
		    list[j + gap] = temp;
		}
	    }
	}
}/*sortscripts()*/


/*---------------------------------------------------------------------------*/
static void do_menu (void)
{
	uint item, first, maxitems;
	uint page, pages;
	short a;

	if (items == 0) {
	   waq_info();
	   com_printf("There are currently no scripts active\n");
	   enter();
	   return;
	}

	sortscripts();
	page  = 1;
	pages = (items + (LEN_PAGE - 1)) / LEN_PAGE;

	timelimit = (timelimit > 0L) ? (time(NULL) + (timelimit * 60)) : 0L;

again:	waq_info();

	com_printf(SEQ_YELONBLUE "Available scripts");
	if (pages > 1) com_printf(" (page %u of %u)",page,pages);
	com_printf("\n");

	if (!colour()) {
	   com_printf("-----------------");
	   if (pages > 1) com_printf("--------------");
	}
	com_printf("\n");

	first = (page - 1) * LEN_PAGE;
	maxitems = first + LEN_PAGE;
	if (maxitems > items) maxitems = items;
	maxitems -= first;
	for (item = 0; item < maxitems; item++) {
	    com_printf(SEQ_YELLOW "%c" SEQ_GREY " ... " SEQ_RED "%-8s" SEQ_GREY " - " SEQ_LCYAN "%s\n",
		       (int) ('A' + item),
		       list[first + item]->info.script_name,
		       list[first + item]->info.script_desc);
	}

/* ----------------------------------------------------------------------------|
1000 minutes left
Select [Script (A..L), Page (1..9), -)Prev, +)Next, =)Again, ?)Info, Q)uit]: ?|
*/

select: com_printf("\n");
	if (timelimit > 0L) {
	   long left = ((timelimit - time(NULL)) + 59L) / 60L;

	   if (left > 0) {
	      com_printf(SEQ_MAGENTA "%ld minutes left\n", left);
	   }
	   else {
	      com_printf(SEQ_WHITE "Sorry, no time left\n");
	      goto fini;
	   }
	}
	com_printf(SEQ_WHITE "Select" SEQ_GREY " [Script (" SEQ_YELLOW "A");
	if (maxitems > 1) com_printf("..%c", (int) ('@' + maxitems));
	com_printf(SEQ_GREY ")");
	if (pages > 1) {
	   com_printf(", Page (" SEQ_YELLOW "1..%u", SEQ_GREY ")", pages);
	   if (page > 1) {
	      com_printf(", " SEQ_YELLOW "-" SEQ_GREY);
	      if (!colour()) com_printf(")");
	      com_printf("Prev");
	   }
	   if (page < pages) {
	      com_printf(", " SEQ_YELLOW "-" SEQ_GREY);
	      if (!colour()) com_printf(")");
	      com_printf("Next");
	   }
	}
	com_printf(", " SEQ_YELLOW "=" SEQ_GREY);
	if (!colour()) com_printf(")");
	com_printf("Again, " SEQ_YELLOW "?" SEQ_GREY);
	if (!colour()) com_printf(")");
	com_printf("Info, " SEQ_YELLOW "Q" SEQ_GREY);
	if (!colour()) com_printf(")");
	com_printf("uit]: " SEQ_CYAN);

	a = do_get(false,0);

	if (a == '=')
	   goto again;
	else if (a == '-' && page > 1) {
	   page--;
	   goto again;
	}
	else if (a == '+' && page < pages) {
	   page++;
	   goto again;
	}
	else if (a == '?') {
	   boolean ask_enter = true;

	   com_printf("\n");
	   if ((fp = fopen("waq.bbs",TREAD)) == NULL)
	      com_printf(SEQ_WHITE "Information text not available\n");
	   else {
	      byte line = 0;

	      com_printf(SEQ_CLS);
	      while (fgets(buffer,MAX_BUFFER,fp)) {
		    com_printf(buffer);
		    if (++line == 23) {
more:		       com_printf("More [Y)es/N)o]: ");
		       a = do_get(false,0);
		       if (a == 'N') {
			  ask_enter = false;
			  break;
		       }
		       if (a != 'Y') {
			  com_printf("Invalid response - ");
			  goto more;
		       }
		       line = 0;
		    }
	      }
	      fclose(fp);
	      fp = NULL;
	   }
	   if (ask_enter) enter();
	   goto select;
	}
	else if (a == 'Q') {
	   com_printf("\n");
quit:	   com_printf(SEQ_WHITE "Confirm quit" SEQ_GREY " [" SEQ_YELLOW "Y" SEQ_GREY);
	   if (!colour()) com_printf(")");
	   com_printf("es/");
	   com_printf(SEQ_YELLOW "N" SEQ_GREY);
	   if (!colour()) com_printf(")");
	   com_printf("o]: " SEQ_CYAN);

	   a = do_get(false,0);

	   if (a == 'N') goto select;
	   if (a != 'Y') {
	      com_printf(SEQ_WHITE "Invalid response - ");
	      goto quit;
	   }
fini:	   com_printf(SEQ_WHITE "\nThanks for your participation!\n\n");
	   return;
	}
	else if (a >= '1' && a < ('0' + pages) && pages > 1) {
	   page = 1 + (a - '1');
	   goto again;
	}
	else if (a < 'A' || a > ('@' + maxitems)) {
	   com_printf(SEQ_WHITE "\n'%c' is not a valid response", (int) a);
	   enter();
	   goto select;
	}

	script = list[first + (a - 'A')];

info:	com_printf(SEQ_CLS SEQ_YELONBLUE "Information on selected script\n");
	if (!colour())
	   com_printf("------------------------------");
	com_printf("\n");
	com_info();
	if ((time(NULL) + timezone) > script->info.end_date) {
	   com_printf(SEQ_WHITE "Not allowed to run script - end date just passed\n");
	   goto select;
	}
run:	com_printf(SEQ_WHITE "Script" SEQ_GREY " [" SEQ_YELLOW "R" SEQ_GREY);
	if (!colour()) com_printf(")");
	com_printf("un, " SEQ_YELLOW "=" SEQ_GREY);
	if (!colour()) com_printf(")");
	com_printf("Again, " SEQ_YELLOW "Q" SEQ_GREY);
	if (!colour()) com_printf(")");
	com_printf("uit]: " SEQ_CYAN);

	a = do_get(false,0);

	if (a == 'R') {
	   com_printf("\n\n" SEQ_CLS);
	   do_run();
	   goto again;
	}
	if (a == '=') goto info;
	if (a == 'Q') goto select;
	com_printf(SEQ_WHITE "Invalid response - ");
	goto run;
}/*do_menu()*/


/*---------------------------------------------------------------------------*/
static boolean set_emu (char *parm)
{
	if	(!stricmp(parm,"NONE"))   emu_type = EMU_NONE;
	else if (!stricmp(parm,"FF"))	  emu_type = EMU_FF;
	else if (!stricmp(parm,"AVATAR")) emu_type = EMU_AVATAR;
	else if (!stricmp(parm,"ANSI"))   emu_type = EMU_ANSI;
	else if (!stricmp(parm,"VT52"))   emu_type = EMU_VT52;
	else {
	   fprintf(stderr,"waqrun: Invalid option -e type '%s'\n",parm);
	   return (false);
	}

	return (true);
}/*set_emu()*/


static void set_user (char *parm, boolean append)
{
	int i;

	for (i = append ? strlen(user) : 0; *parm && i < MAX_USER; parm++)
	    if (isalpha(*parm)) user[i++] = tolower(*parm);
	user[i] = '\0';
}/*set_user()*/


static boolean file_info (char *parm)
{
	char *p;
	int i;

	if ((p = strchr(parm,'=')) == NULL) {
	   fprintf(stderr,"waqrun: Invalid option -f syntax\n");
	   return (false);
	}
	*p++ = '\0';
	strip(parm);
	strip(p);

	if (!stricmp(parm,"DORINFO")) {
	   if ((fp = fopen(p,TREAD)) == NULL) {
	      fprintf(stderr,"waqrun: Can't open option -f DORINFO file '%s'\n",p);
	      return (false);
	   }
	   for (i = 0; i < 4; i++) fgets(buffer,MAX_BUFFER,fp);
	   sscanf(buffer,"COM%d",&port);
	   for (i = 0; i < 3; i++) fgets(buffer,MAX_BUFFER,fp);
	   set_user(buffer,false);
	   fgets(buffer,MAX_BUFFER,fp);
	   set_user(buffer,true);
	   for (i = 0; i < 2; i++) fgets(buffer,MAX_BUFFER,fp);
	   switch (atoi(buffer)) {
		  case 1:  emu_type = EMU_ANSI;   break;
		  case 2:  emu_type = EMU_AVATAR; break;
		  default: emu_type = EMU_NONE;   break;
	   }
	   for (i = 0; i < 2; i++) fgets(buffer,MAX_BUFFER,fp);
	   timelimit = atol(buffer);
	   fclose(fp);
	   fp = NULL;
	}

	else if (!stricmp(parm,"DOORSYS")) {
	   if ((fp = fopen(p,TREAD)) == NULL) {
	      fprintf(stderr,"waqrun: Can't open option -f DOORSYS file '%s'\n",p);
	      return (false);
	   }
	   fgets(buffer,MAX_BUFFER,fp);
	   sscanf(buffer,"COM%d:",&port);
	   for (i = 0; i < 9; i++) fgets(buffer,MAX_BUFFER,fp);
	   set_user(buffer,false);
	   for (i = 0; i < 9; i++) fgets(buffer,MAX_BUFFER,fp);
	   timelimit = atol(buffer);
	   fgets(buffer,MAX_BUFFER,fp);
	   strip(buffer);
	   if (!stricmp(buffer,"GR")) emu_type = EMU_ANSI;
	   else 		      emu_type = EMU_NONE;
	   fclose(fp);
	   fp = NULL;
	}

	else if (!stricmp(parm,"CALLINFO")) {
	   boolean local = false;

	   if ((fp = fopen(p,TREAD)) == NULL) {
	      fprintf(stderr,"waqrun: Can't open option -f CALLINFO file '%s'\n",p);
	      return (false);
	   }
	   fgets(buffer,MAX_BUFFER,fp);
	   set_user(buffer,false);
	   fgets(buffer,MAX_BUFFER,fp);
	   if (atoi(buffer) == 5) local = true; /* speed code 5 == local     */
	   for (i = 0; i < 3; i++) fgets(buffer,MAX_BUFFER,fp);
	   timelimit = atol(buffer);
	   fgets(buffer,MAX_BUFFER,fp);
	   strip(buffer);
	   if (!stricmp(buffer,"COLOR")) emu_type = EMU_ANSI;
	   else 			 emu_type = EMU_NONE;
	   for (i = 0; i < 22; i++) fgets(buffer,MAX_BUFFER,fp);
	   if (!stricmp(buffer,"LOCAL")) local = true;
	   fgets(buffer,MAX_BUFFER,fp);
	   sscanf(buffer,"COM%d",&port);
	   fgets(buffer,MAX_BUFFER,fp);
	   if (atoi(buffer) == 5) local = true;
	   fclose(fp);
	   fp = NULL;
	   if (local) port = 0;
	}

	else if (!stricmp(parm,"WWIV")) {
	   boolean local = false;

	   if ((fp = fopen(p,TREAD)) == NULL) {
	      fprintf(stderr,"waqrun: Can't open option -f WWIV file '%s'\n",p);
	      return (false);
	   }
	   for (i = 0; i < 2; i++) fgets(buffer,MAX_BUFFER,fp);
	   set_user(buffer,false);		/* User name		     */
	   fgets(buffer,MAX_BUFFER,fp);
	   if (isalpha(buffer[0]))		/* Real name, if applicable  */
	      set_user(buffer,false);
	   for (i = 0; i < 11; i++) fgets(buffer,MAX_BUFFER,fp);
	   switch (atoi(buffer)) {
		  case 1:  emu_type = EMU_ANSI;   break;
		  case 2:  emu_type = EMU_AVATAR; break;
		  default: emu_type = EMU_NONE;   break;
	   }
	   fgets(buffer,MAX_BUFFER,fp);
	   if (atoi(buffer) == 0) local = true; /* 0 if local, else 1	     */
	   fgets(buffer,MAX_BUFFER,fp);
	   timelimit = atol(buffer);
	   for (i = 0; i < 5; i++) fgets(buffer,MAX_BUFFER,fp);
	   sscanf(buffer,"COM%d",&port);
	   fclose(fp);
	   fp = NULL;
	   if (local) port = 0;
	}

	else {
	   fprintf(stderr,"waqrun: Invalid option -f type '%s'\n",parm);
	   return (false);
	}

	return (true);
}/*file_info()*/


/*---------------------------------------------------------------------------*/
int main (int argc, char *argv[])
{
	int	 n;
	boolean  ok = true;

	signal(SIGINT,user_break);
#ifdef XENIX
	setbuf(stdin,NULL);
	setbuf(stdout,NULL);
#endif

	if (argc < 2) goto usage;

	for (n = 1; n < argc; n++) {
	    switch (argv[n][0]) {
		   case '-':
		   case '/':
			if (withwild > 0 || nowild > 0) {
			   fprintf(stderr,"waqrun: Commandline options not allowed AFTER filespec(s)\n");
			   ok = false;
			   break;
			}
			switch (tolower(argv[n][1])) {
			       case '?':
			       case 'h': goto usage;
#ifndef XENIX
			       case 'p': port = atoi(&argv[n][2]);
					 break;
			       case 'z': { int h, mm;
					   if (sscanf(&argv[n][2],"%d:%d",&h,&mm) == 2) {
					      timezone = h * 60L;
					      if (timezone < 0) timezone -= mm;
					      else		timezone += mm;
					      timezone *= 60L;	/* mins -> secs */
					   }
					 }
					 break;
#endif
			       case 't': timelimit = atol(&argv[n][2]);
					 break;
			       case 'e': if (!set_emu(&argv[n][2]))
					    ok = false;
					 break;
			       case 'n': set_user(&argv[n][2],false);
					 break;
			       case 'f': if (!file_info(&argv[n][2]))
					    ok = false;
					 break;
			       default:  fprintf(stderr,"waqrun: Invalid commandline option '%s'",argv[n]);
					 ok = false;
					 break;
			}
			break;

		   default:
			add_list(argv[n]);
			break;
	    }
	}

	if (ok && withwild == 0) {
	   if (nowild == 0)
	      add_list("*");
	   else if (nowild == 1 && items == 0)
	      ok = false;
	}

	if (ok && !com_init(port)) {
	   fprintf(stderr,"waqrun: Can't initialize comport\n");
	   ok = false;
	}

	if (ok) {
	   if (withwild == 0 && nowild == 1) {
	      script = list[0];
	      waq_info();
	      com_info();
	      com_printf("\n");
	      do_run();
	      if (errors) ok = false;
	   }
	   else
	      do_menu();
	}

	erl_exit(ok ? ERL_OK : ERL_ERRORS);

usage:	printf("WAQRUN Version %s; Wide Area Query Runtime Interpreter (format revision %u)\n",VERSION,REVISION);
	printf("Design & COPYRIGHT (C) 1992 by A.G.Lentz & T.J.Caulfeild; ALL RIGHTS RESERVED\n");
	printf("Wide Area Query - A project of LENTZ SOFTWARE-DEVELOPMENT & EuroBaud Software\n\n");

	printf("Usage: WAQRUN [<option> ...] [<filespec>.%s ...]\n\n",EXT_COM);
	printf("Options: -? or -h          This information\n");
#ifndef XENIX
	printf("         -p<port>          Select comport to use (0 or omit for local)\n");
	printf("         -z[-]h:mm         local + timezone = GMT/UTC (Example: -z-1:00)\n");
#endif
	printf("         -t<timelimit>     User's time limit in minutes\n");
	printf("         -e<type>          Terminal emulation (NONE,FF,AVATAR,ANSI,VT52)\n");
	printf("         -n<username>      Name of user (Example: \"-nJohn Doe\")\n");
	printf("         -f<type>=<fname>  Read some/all option info from file\n");
	printf("                           (Example: -fDORINFO=\\bbs\\DORINFO1.DEF)\n");

	return (ERL_OK);
}/*main()*/


/* end of waqrun.c */

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
#define WAQ_DISASM
#include "waq.h"


#define VERSION "1.00"


static char	 buffer[MAX_BUFFER + 1];
static char	 scriptfile[MAX_BUFFER + 1];
static char	 path[MAX_BUFFER + 1];
static FILE	*fp = NULL;
static WAQ_INFO  info;
static word	 hdr_len;
static long	 crc;
static byte	*code = NULL;
static char	*data = NULL;
static word	 ip;


/*---------------------------------------------------------------------------*/
static void erl_exit (int erl)
{
	if (erl != ERL_OK) {
	   if (fp != NULL) fclose(fp);
	   deinit();
	}

	exit (erl);
}/*erl_exit()*/


static void dis_error (char *fmt, ...)
{
	va_list argptr;

	printf("Error %s: ",scriptfile);
	va_start(argptr,fmt);
	vprintf(fmt,argptr);
	va_end(argptr);
	printf("\n");
}/*dis_error()*/


static void user_break (void)
{
	printf("*** User break ***\n");
	erl_exit(ERL_BREAK);
}/*user_break()*/


/*---------------------------------------------------------------------------*/
static void *myalloc (word nbytes)
{
	void *p;

	if ((p = malloc(nbytes)) == NULL) {
	   dis_error("Can't allocate memory");
	   erl_exit(ERL_MEMORY);
	}

	return (p);
}/*myalloc()*/


/*---------------------------------------------------------------------------*/
static byte read_byte (void)
{
	int i;

	if ((i = getc(fp)) == EOF) {
	   dis_error("Can't read header");
	   erl_exit(ERL_FATAL);
	}
	crc = crc32byte(crc,(byte) i);
	hdr_len++;
	return ((byte) i);
}/*read_byte()*/

static void read_str (char *s, word size)
{
	if (fread(s,(unsigned int) size,1,fp) != 1) {
	   dis_error("Can't read header");
	   erl_exit(ERL_FATAL);
	}
	crc = crc32block(crc,(byte *) s,size);
	hdr_len += size;
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


static boolean read_com (void)
{
	hdr_len = 0;
	crc = crc32init();

	read_str  (  info.idstring	, LEN_IDSTR	);
	if (strcmp(info.idstring,IDSTRING)) {
	   dis_error("Not WAQ format");
	   return (false);
	}
	read_word ( &info.revision			);
	if (info.revision != REVISION) {
	   dis_error("Different format revision (%u instead of %u)",
		     info.revision,REVISION);
	   return (false);
	}
	read_str  (  info.type_id	, LEN_WAQID + 1 );
	if (strcmp(info.type_id,WAQ_ID)) {
	   dis_error("WAQ format but not a compiled script (%s instead of %s)",info.type_id,WAQ_ID);
	   return (false);
	}
	read_long ( &info.compile_stamp 		);
	read_str  (  info.script_name	, MAX_NAME  + 1 );
	read_str  (  info.script_desc	, MAX_DESC  + 1 );
	read_long ( &info.start_date			);
	read_long ( &info.end_date			);
	read_str  (  info.return_dest	, MAX_ADR   + 1 );
	read_str  (  info.arc_list	, MAX_ARC   + 1 );
	read_long ( &info.return_stamp			);
	read_str  (  info.return_orig	, MAX_ADR   + 1 );
	read_word ( &info.code_length			);
	read_word ( &info.data_length			);
	read_word ( &info.num_vars			);
	read_word ( &info.stored_vars			);
	read_long ( &info.file_length			);
	read_long ( &info.file_crc			);
	read_long ( &info.hdr_crc			);

	if (!crc32test(crc)) {
	   dis_error("Header failed CRC check (%08lx instead of %08lx)",crc,info.hdr_crc);
	   return (false);
	}

	if (!info.script_name[0] || !info.script_desc[0] ||
	    !info.start_date || info.end_date <= info.start_date ||
	    !info.return_dest[0] ||
	    info.return_stamp || info.return_orig[0] ||
	    !info.code_length || info.code_length > MAX_CODE ||
	    info.data_length > MAX_DATA ||
	    !info.num_vars || info.num_vars > MAX_VAR ||
	    !info.stored_vars || info.stored_vars > info.num_vars ||
	    info.file_length < (info.code_length + info.data_length)) {
	   dis_error("Invalid data/information in WAQ header (DID pass CRC check!)");
	   return (false);
	}

	fseek(fp,0L,SEEK_END);
	if (ftell(fp) < (hdr_len + info.file_length)) {
	   dis_error("File is shorter than length specification in header");
	   return (false);
	}
	fseek(fp,(long) hdr_len,SEEK_SET);

	code = (byte *) myalloc(info.code_length);
	if (fread(code,(unsigned int) info.code_length,1,fp) != 1) {
	   dis_error("Can't read script code");
	   return (false);
	}

	if (info.data_length) {
	   data = (char *) myalloc(info.data_length);
	   if (fread(data,(unsigned int) info.data_length,1,fp) != 1) {
	      dis_error("Can't read script data");
	      return (false);
	   }
	}

	crc = crc32init();
	crc = crc32block(crc, code, info.code_length);
	crc = crc32block(crc, (byte *) data, info.data_length);
	crc = crc32byte(crc, (byte) info.file_crc);
	crc = crc32byte(crc, (byte) (info.file_crc >> 8));
	crc = crc32byte(crc, (byte) (info.file_crc >> 16));
	crc = crc32byte(crc, (byte) (info.file_crc >> 24));
	if (!crc32test(crc)) {
	   dis_error("Code/data failed CRC check (%08lx instead of %08lx)",crc,info.file_crc);
	   return (false);
	}

	return (true);
}/*read_com()*/


/*---------------------------------------------------------------------------*/
static void show_info (void)
{
	printf("Format revision = %u\n",    info.revision);
	printf("Compile time    = %s\n",    stamptodate(info.compile_stamp));
	printf("Script name     = %s\n",    info.script_name);
	printf("Script desc     = %s\n",    info.script_desc);
	printf("Start date      = %s GMT/UTC\n", stamptodate(info.start_date));
	printf("End date        = %s GMT/UTC\n", stamptodate(info.end_date));
	printf("Return dest     = %s\n",    info.return_dest);
	printf("Arc list        = %s\n",    info.arc_list[0] ? info.arc_list : "<none>");
	printf("Return stamp    = <empty>\n");
	printf("Return orig     = <empty>\n");
	printf("Code length     = %u\n",    info.code_length);
	printf("Data length     = %u\n",    info.data_length);
	printf("Num vars        = %u\n",    info.num_vars);
	printf("Stored vars     = %u\n",    info.stored_vars);
	printf("File length     = hdr(%u) + %lu = %lu\n",
	       hdr_len, info.file_length, (long) info.file_length + hdr_len);
	printf("File CRC        = %08lx\n", info.file_crc);
	printf("WAQ header CRC  = %08lx\n", info.hdr_crc);
	printf("\n");
}/*show_info()*/


/*---------------------------------------------------------------------------*/
static byte get_byte (void)
{
	return (code[ip++]);
}/*get_byte()*/


static word get_word (void)
{
	byte b = get_byte();

	return ((get_byte() << 8) | b);
}/*get_word()*/


/*---------------------------------------------------------------------------*/
static void disassemble (void)
{
	MNEMONIC op;

	ip = 0;
	while (ip < info.code_length) {
	      printf("%04x",ip);
	      op = get_byte();
	      printf("  %02x",op);
	      switch (op) {
		     case m_jz: case m_jnz:
		     case m_jmp: case m_call:
		     case m_inc: case m_dec:
		     case m_ldai: case m_lda: case m_sta:
		     case m_puts:
			  printf(" %02x %02x",code[ip],code[ip + 1]);
			  break;

		     default:
			  printf("      ");
			  break;
	      }
	      printf("  ");

	      switch (op) {
		     case m_jz:
		     case m_jnz:
		     case m_jmp:
		     case m_call:
			  printf("%-6.6s %04x",mnemonic_table[op],get_word());
			  break;

		     case m_inc:
		     case m_dec:
		     case m_lda:
		     case m_sta:
			  printf("%-6.6s (%d)",mnemonic_table[op],get_word());
			  break;

		     case m_ldai:
			  printf("%-6.6s %-6d",mnemonic_table[op],get_word());
			  break;

		     case m_puts:
			  printf("%-6.6s [%04x]",mnemonic_table[op],get_word());
			  break;

		     default:
			  if (op >= m_bad) printf("???");
			  else		   printf("%s",mnemonic_table[op]);
	      }

	      printf("\n");
	}
}/*disassemble()*/


/*---------------------------------------------------------------------------*/
static void deinit (void)
{
	if (code != NULL) free(code);
	code = NULL;
	if (data != NULL) free(data);
	data = NULL;
}/*deinit()*/


/*---------------------------------------------------------------------------*/
int main (int argc, char *argv[])
{
	char	*p, *q;
	int	 n;
	boolean  ok = true, res;

	printf("WAQDIS Version %s; Wide Area Query Disassembler (format revision %u)\n",VERSION,REVISION);
	printf("Design & COPYRIGHT (C) 1992 by A.G.Lentz & T.J.Caulfeild; ALL RIGHTS RESERVED\n");
	printf("Wide Area Query - A project of LENTZ SOFTWARE-DEVELOPMENT & EuroBaud Software\n\n");

	if (argc < 2) {
	   printf("Usage: WAQDIS <filespec>.%s ...\n",EXT_COM);
	   return (ERL_OK);
	}

	signal(SIGINT,user_break);

	for (n = 1; n < argc; n++) {
	    strlwr(argv[n]);
	    splitpath(argv[n],path,buffer);
	    if ((q = strchr(&argv[n][strlen(path)],'.')) != NULL)
	       *q = '\0';
	    sprintf(buffer,"%s.%s",argv[n],EXT_COM);
	    if ((p = ffirst(buffer)) == NULL) {
	       printf("Error: No files matching '%s'",buffer);
	       ok = false;
	       continue;
	    }

	    do {
	       strlwr(p);
	       mergepath(scriptfile,path,p);

	       if ((fp = fopen(scriptfile,BREAD)) == NULL) {
		  dis_error("Can't open script file");
		  ok = false;
		  continue;
	       }

	       printf("%s:\n",scriptfile);
	       res = read_com();
	       fclose(fp);
	       fp = NULL;

	       if ((q = strchr(p,'.')) != NULL)
		  *q = '\0';
	       if (stricmp(info.script_name,p)) {
		  dis_error("Script name '%s' doesn't match in filename '%s%s.%s'",
			    info.script_name,path,p,EXT_COM);
		  ok = false;
	       }
	       else if (res) {
		  show_info();
		  disassemble();
		  deinit();
	       }
	       else
		  ok = false;
	    } while ((p = fnext()) != NULL);
	}

	return (ok ? ERL_OK : ERL_ERRORS);
}/*main()*/


/* end of waqdis.c */

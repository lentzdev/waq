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
#define IDSTRING   "Wide Area Query - COPYRIGHT (C) A.G.Lentz & T.J.Caulfeild; ALL RIGHTS RESERVED\n"
#define ID_MD5	   "Wide Area Query"
#define WAQ_ID	   "WAQ"    /* Type id strings for the binary format files   */
#define WAR_ID	   "WAR"
#define REVISION   1

#define EXT_SRC    "wsc"    /* script source				     */
#define EXT_MAP    "map"    /* script map file file created by WAQCOMP	     */
#define EXT_COM    "waq"    /* compiled script				     */
#define EXT_RSP    "war"    /* response file (one from each system)	     */

#define OFS_NULL   0xffff

#define MAX_BUFFER 256
#define MAX_LINE   1024
#define LEN_IDSTR  80
#define LEN_WAQID  3
#define LEN_MD5    32
#define MAX_USER   80
#define MAX_NAME   8
#define MAX_DESC   70
#define MAX_ADR    128
#define MAX_ARC    128
#define MAX_CODE   (20 * 1024)
#define MAX_DATA   (20 * 1024)
#define MAX_STACK  (2 * 1024)
#define MAX_VAR    512

enum { ERL_OK, ERL_TIMEOUT, ERL_CARRIER, ERL_ERRORS,
       ERL_BREAK, ERL_MEMORY, ERL_FATAL };


#ifdef MSDOS
#define BREAD  "rb"
#define TREAD  "rt"
#define BWRITE "w+b"
#define TWRITE "w+t"
#define BRDWR  "r+b"
#else
#define BREAD  "r"
#define TREAD  "r"
#define BWRITE "w+"
#define TWRITE "w+"
#define BRDWR  "r+"
#endif


enum {	m_nop	 ,
	m_jz	 , m_jnz    , m_jmp   , m_call	 , m_return ,
	m_inc	 , m_dec    , m_push  , m_pop	 , m_ldai   , m_lda  , m_sta ,
	m_lognot , m_onecom , m_neg   , m_mul	 , m_div    , m_mod  ,
	m_add	 , m_sub    , m_shr   , m_shl	 ,
	m_ls	 , m_gt     , m_lse   , m_gte	 , m_eq     , m_ne   ,
	m_and	 , m_xor    , m_or    ,
	m_geti	 , m_getc   , m_puti  , m_putc	 , m_puts   ,
	m_end	 , m_quit   , m_enter , m_colour , m_cls    , m_read ,
	m_bad
};
typedef byte MNEMONIC;


#ifdef WAQ_DISASM  /* ------------------------------------------------------ */

char *mnemonic_table[] = {
	"nop"	 ,
	"jz"	 , "jnz"    , "jmp"   , "call"	 , "return" ,
	"inc"	 , "dec"    , "push"  , "pop"	 , "ldai"   , "lda"  , "sta" ,
	"lognot" , "onecom" , "neg"   , "mul"	 , "div"    , "mod"  ,
	"add"	 , "sub"    , "shr"   , "shl"	 ,
	"ls"	 , "gt"     , "lse"   , "gte"	 , "eq"     , "ne"   ,
	"and"	 , "xor"    , "or"    ,
	"geti"	 , "getc"   , "puti"  , "putc"	 , "puts"   ,
	"end"	 , "quit"   , "enter" , "colour" , "cls"    , "read" ,
	NULL
};

#endif	/* WAQ_DISASM ------------------------------------------------------ */


#ifdef WAQ_COMPILER  /* ---------------------------------------------------- */

enum {	NONE	 ,
	INFO	 , STORE    , INTERN	,
	IF	 , ELSE     , GOTO	, CALL	       , RETURN   ,
	FOR	 , DO	    , WHILE	, CONTINUE     , BREAK	  ,
	INC	 , DEC	    , ASSIGN	,
	ADDASS	 , SUBASS   , MULASS	, DIVASS       , MODASS   ,
	SHRASS	 , SHLASS   , ANDASS	, XORASS       , ORASS	  ,
	QUESTION , COLON    , COMMA	, SEMICOLON    ,
	LBRACKET , RBRACKET , LPAREN	, RPAREN       ,
	LOGNOT	 , ONECOM   , MUL	, DIV	       , MOD	  ,
	PLUS	 , MINUS    , SHR	, SHL	       ,
	LESS	 , GREATER  , LESSEQUAL , GREATEREQUAL , EQUAL	  , NOTEQUAL ,
	AND	 , XOR	    , OR	, LOGAND       , LOGOR	  ,
	GETI	 , GETC     , PUTI	, PUTC	       , PUTS	  ,
	END	 , QUIT     , ENTER	, COLOUR       , CLS	  , READ     ,
	IDENT	 , SCONST   , CCONST	, ICONST       ,
};
typedef byte TOKEN;

char *token_table[] = {
	""	 ,
	"info"	 , "store"  , "intern"	,
	"if"	 , "else"   , "goto"	, "call"       , "return" ,
	"for"	 , "do"     , "while"	, "continue"   , "break"  ,
	"++"	 , "--"     , "="	,	       
	"+="	 , "-="     , "*="	, "/="	       , "%="	  ,
	">>="	 , "<<="    , "&="	, "^="	       , "|="	  ,
	"?"	 , ":"	    , ","	, ";"	       ,
	"{"	 , "}"	    , "("	, ")"	       ,
	"!"	 , "~"	    , "*"	, "/"	       , "%"	  ,
	"+"	 , "-"	    , ">>"	, "<<"	       ,
	"<"	 , ">"	    , "<="	, ">="	       , "=="	  , "!="     ,
	"&"	 , "^"	    , "|"	, "&&"	       , "||"	  ,
	"geti"	 , "getc"   , "puti"	, "putc"       , "puts"   ,
	"end"	 , "quit"   , "enter"	, "colour"     , "cls"	  , "read"   ,
	NULL
};


typedef struct {
	TOKEN token;			/* keyword, operator or item type    */
	union {
	      char  *ident;		/* unquoted, identifier / label name */
	      char  *sconst;		/* "quoted,  string constant         */
	      short  cconst;		/* 'quoted,  character constant      */
	      short  iconst;		/* unquoted, integer constant	     */
	}     x;
} ITEM;


enum { STS_NONE, STS_VAR, STS_REG };
typedef byte EXPR_STS;

typedef struct {
	EXPR_STS sts;
	word	 varidx;
	boolean  assigned;
} EXPR;


typedef struct _string {
	char	       *s;
	word		ofs;
	struct _string *next;
} STRING;


#define VAR_USE    0x01
#define VAR_ASSIGN 0x02

typedef struct _variable {
	char		 *s;
	word		  idx;
	byte		  flags;
	struct _variable *next;
} VARIABLE;


#define LBL_USE    0x01

typedef struct _label {
	char	      *s;
	word	       ofs;
	byte	       flags;
	struct _label *next;
} LABEL;


typedef struct _xref {
	char	      *s;
	word	       ofs;
	struct _xref *next;
} XREF;

#endif	/* WAQ_COMPILER ---------------------------------------------------- */


typedef struct {
	char  idstring[LEN_IDSTR];	/* IDSTRING define		     */
	word  revision; 		/* language/compiled format revision */
	char  type_id[LEN_WAQID + 1];	/* Type of file (xxx_ID defines)     */
	long  compile_stamp;		/* Timestamp of script compile	     */
	char  script_name[MAX_NAME + 1];/* Name of this query (base for WAR) */
	char  script_desc[MAX_DESC + 1];/* Desc. of query (for menu)	     */
	long  start_date;		/* Start date of query		     */
	long  end_date; 		/* End date of query		     */
	char  return_dest[MAX_ADR + 1]; /* Address to send .WAR file to      */
	char  arc_list[MAX_ARC + 1];	/* List of compression formats	     */
	long  return_stamp;		/* Timestamp of .WAR file sent back  */
	char  return_orig[MAX_ADR + 1]; /* Address .WAR file returned from   */
	word  code_length;		/* Length of script code in bytes    */
	word  data_length;		/* Length of string data in bytes    */
	word  num_vars; 		/* No. variables used in script      */
	word  stored_vars;		/* First N vars stored in .WAR	     */
	long  file_length;		/* Exact bytecount rest of file      */
	long  file_crc; 		/* CRC-32 of rest of file (not HDR)  */
	long  hdr_crc;			/* CRC-32 of this hdr (not hdr_crc)  */
} WAQ_INFO;


enum { EMU_NONE, EMU_FF, EMU_AVATAR, EMU_ANSI, EMU_VT52 };
typedef byte EMU_TYPE;

#define AVT_BLACK	 0
#define AVT_BLUE	 1
#define AVT_GREEN	 2
#define AVT_CYAN	 3
#define AVT_RED 	 4
#define AVT_MAGENTA	 5
#define AVT_BROWN	 6
#define AVT_GRAY	 7
#define AVT_GREY	 7
#define AVT_LBLACK	 8
#define AVT_LBLUE	 9
#define AVT_LGREEN	10
#define AVT_LCYAN	11
#define AVT_LRED	12
#define AVT_LMAGENTA	13
#define AVT_YELLOW	14
#define AVT_WHITE	15

#define SEQ_CLS 	"\014"
#define SEQ_BLACK	"\026\001\000"
#define SEQ_BLUE	"\026\001\001"
#define SEQ_GREEN	"\026\001\002"
#define SEQ_CYAN	"\026\001\003"
#define SEQ_RED 	"\026\001\004"
#define SEQ_MAGENTA	"\026\001\005"
#define SEQ_BROWN	"\026\001\006"
#define SEQ_GRAY	"\026\001\007"
#define SEQ_GREY	"\026\001\007"
#define SEQ_LBLACK	"\026\001\010"
#define SEQ_LBLUE	"\026\001\011"
#define SEQ_LGREEN	"\026\001\012"
#define SEQ_LCYAN	"\026\001\013"
#define SEQ_LRED	"\026\001\014"
#define SEQ_LMAGENTA	"\026\001\015"
#define SEQ_YELLOW	"\026\001\016"
#define SEQ_WHITE	"\026\001\017"
#define SEQ_YELONBLUE	"\026\001\036"
#define SEQ_BLINK	"\026\002"


#ifdef WAQ_RUN	/* --------------------------------------------------------- */

#define MAX_PAGES  9
#define LEN_PAGE  15
#define MAX_ITEMS (MAX_PAGES * LEN_PAGE)

typedef struct _waq_list {
	char	 filebase[MAX_BUFFER + 1];
	word	 hdr_len;
	WAQ_INFO info;
} WAQ_LIST;

#endif	/* WAQ_RUN --------------------------------------------------------- */


#ifdef ANSIC
#define __PROTO__
#endif
#include "waq.pro"
#ifdef WAQ_COMPILER
#include "waqcomp.pro"
#endif
#ifdef WAQ_DISASM
#include "waqdis.pro"
#endif
#ifdef WAQ_RUN
#include "waqrun.pro"
#endif
#ifdef ANSIC
#undef __PROTO__
#endif


/* end of waq.h */

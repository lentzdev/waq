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
#define WAQ_COMPILER
#include "waq.h"


#define VERSION "1.00"


       char	 buffer[MAX_BUFFER + 1];
static char	 linebuf[MAX_LINE + 1];
static char	 sourcefile[MAX_BUFFER + 1];
static char	 path[MAX_BUFFER + 1];
       FILE	*fp = NULL;
static word	 sourceline;
static word	 errors;
static word	 labelcount;
static ITEM	 item;
static boolean	 item_saved;
static boolean	 allow_label;
static boolean	 need_statement;
static STRING	*string   = NULL;
static VARIABLE *variable = NULL;
static LABEL	*label	  = NULL;
static XREF	*xref	  = NULL;
static WAQ_INFO  info;
static word	 hdr_len;
static byte	*code = NULL;


/*---------------------------------------------------------------------------*/
static void erl_exit (int erl)
{
	if (erl != ERL_OK && erl != ERL_ERRORS) {
	   if (fp != NULL) fclose(fp);
	   deinit();
	}

	exit (erl);
}/*erl_exit()*/


static void comp_error (char *fmt, ...)
{
	va_list argptr;

	printf("Error %s %u: ",sourcefile,sourceline);
	va_start(argptr,fmt);
	vprintf(fmt,argptr);
	va_end(argptr);
	printf("\n");
	errors++;
}/*comp_error()*/


static void comp_warning (char *fmt, ...)
{
	va_list argptr;

	printf("Warning %s %u: ",sourcefile,sourceline);
	va_start(argptr,fmt);
	vprintf(fmt,argptr);
	va_end(argptr);
	printf("\n");
}/*comp_warning()*/


static void user_break (void)
{
	printf("*** User break ***\n");
	errors++;
	erl_exit(ERL_BREAK);
}/*user_break()*/


/*---------------------------------------------------------------------------*/
static void *myalloc (word nbytes)
{
	register void *p;

	if ((p = malloc(nbytes)) == NULL) {
	   comp_error("Can't allocate memory");
	   erl_exit(ERL_MEMORY);
	}

	return (p);
}/*myalloc()*/


void *mycalloc (word nbytes)
{
	register void *p = myalloc(nbytes);

	memset((byte *) p,0,(uint) nbytes);
	return (p);
}/*myalloc()*/


static char *mydup (char *s)
{
	register char *p = (char *) myalloc((word) strlen(s) + 1);

	return (strcpy(p,s));
}/*mydup()*/


/*---------------------------------------------------------------------------*/
static void emit_byte (byte b)
{
	if (info.code_length >= MAX_CODE) {
	   comp_error("Out of code space");
	   erl_exit(ERL_FATAL);
	}
	code[info.code_length++] = b & 0xff;
}/*emit_byte()*/


static void emit_word (word w)
{
	emit_byte((byte) w);
	emit_byte((byte) (w >> 8));
}/*emit_word()*/


static void emit_short (short i)
{
	emit_byte((byte) i);
	emit_byte((byte) (i >> 8));
}/*emit_short()*/


/*---------------------------------------------------------------------------*/
static int get_char (void)
{
	register int c = getc(fp);

	if (ferror(fp)) {
	   comp_error("Can't read from input file");
	   erl_exit(ERL_FATAL);
	}
	if (c == '\n') sourceline++;
	return (c);
}/*get_char()*/


static void unget_char (register int c)
{
	if (c != EOF && ungetc(c,fp) != EOF && c == '\n') sourceline--;
}/*unget_char()*/


/*---------------------------------------------------------------------------*/
static int get_escchar (void)
{
	int c, n, i, val;

	if ((c = get_char()) == EOF) return (EOF);

	if (tolower(c) == 'x') {
	   for (val = 0, i = 0; i < 2; i++) {
	       if ((c = get_char()) == EOF) return (EOF);
	       n = tolower(c) - '0';
	       if (n > 9) n -= ('a' - ':');
	       if (n & ~0x0f) {
		  unget_char(c);
		  if (i == 0) return (EOF);
	       }
	       val <<= 4;
	       val += n;
	   }
	   return (val);
	}

	if (isdigit(c)) {
	   unget_char(c);
	   for (val = 0, i = 0; i < 3; i++) {
	       if ((c = get_char()) == EOF) return (EOF);
	       n = c - '0';
	       if (n & ~0x07) {
		  unget_char(c);
		  if (i == 0) return (EOF);
	       }
	       val <<= 3;
	       val += n;
	   }
	   return (val);
	}

	switch (c) {
	       case 'n': return ('\n'); 	/* LF  */
	       case 'r': return ('\r'); 	/* CR  */
	       case 'b': return ('\b'); 	/* BS  */
	       case 'f': return ('\f'); 	/* FF  */
	       case 'a': return ('\a'); 	/* BEL */
	       default:  return (c);
	}
}/*get_escchar()*/


#ifdef PARSE_DEBUG
static void put_escchar (int c)
{
	switch (c) {
	       case '\n': printf("\\n");  break;
	       case '\r': printf("\\r");  break;
	       case '\b': printf("\\b");  break;
	       case '\f': printf("\\f");  break;
	       case '\a': printf("\\a");  break;
	       case '\\': printf("\\\\"); break;
	       case '\'': printf("\\\'"); break;
	       case '\"': printf("\\\""); break;
	       default:   printf(isprint(c) ? "%c" : "\%03o", c);
			  break;
	}
}/*put_escchar()*/
#endif


/*---------------------------------------------------------------------------*/
static void unget_item (void)
{
	if (item_saved) {
	   comp_error("Item saved twice (compiler bug!)");
	   erl_exit(ERL_FATAL);
	}
	item_saved = true;
}/*unget_item()*/


static TOKEN get_item (void)
{
	register int c, n, i = 0;

	if (item_saved) {
	   item_saved = false;
	   return (item.token);
	}

	item.token = NONE;
	need_statement = false;

again:	while ((c = get_char()) != EOF && isspace(c));

	if (c == EOF) goto fini;

	if (c == '/') {
	   if ((c = get_char()) == EOF) goto fini;

	   if (c == '/') {
	      while ((c = get_char()) != '\n') if (c == EOF) goto fini;
	   }
	   else if (c == '*') {
	      byte commentlevel = 1;
	      word save_sourceline = sourceline;

	      do {
		 if ((c = get_char()) == EOF) break;
		 if (c == '/') {
		    if ((c = get_char()) == EOF) break;
		    if (c == '*') commentlevel++;
		    else	  unget_char(c);
		 }
		 else if (c == '*') {
		    if ((c = get_char()) == EOF) break;
		    if (c == '/') commentlevel--;
		    else	  unget_char(c);
		 }
	      } while (commentlevel > 0);

	      if (c == EOF)
		 comp_error("Unexpected end of file in comment starting on line %u",save_sourceline);
	   }
	   else
	      goto is_op;

	   goto again;
	}

	else if (c == '\'') {
	   if ((c = get_char()) == EOF ||
	       (c == '\\' && (c = get_escchar()) < 0))
	      comp_error("Illegal character 0x%02x",c);
	   else {
	      item.x.cconst = c;
	      if ((c = get_char()) == '\'')
		 item.token = CCONST;
	      else
		 comp_error("Unterminated character constant");
	   }
	}

	else if (c == '\"') {
	   word save_sourceline;

morestr:   save_sourceline = sourceline;

	   do {
	      c = get_char();

	      if (c == '\"') {
		 while ((c = get_char()) != EOF && isspace(c));
		 if (c == EOF) break;
		 if (c == '\"') goto morestr;
		 unget_char(c);
		 linebuf[i] = '\0';
		 item.token = SCONST;
		 item.x.sconst = linebuf;
		 continue;
	      }
	      else if (c == '\\' && (c = get_escchar()) < 0)
		 comp_error("Illegal character 0x%02x",c);

	      if (c == EOF) break;

	      if (i >= MAX_LINE) {
		 comp_error("String constant too long");
		 break;
	      }
	      linebuf[i++] = c;
	   } while (!item.token);

	   if (item.token != SCONST && c == EOF)
	      comp_error("Unterminated string constant starting on line %u",save_sourceline);
	}

	else if (c == '-' || isdigit(c)) {
	   long val;

	   if (c == '0') {
	      if ((c = get_char()) == EOF) goto fini;
	      if (tolower(c) == 'x') {
		 for (val = 0, i = 0; i < 4; i++) {
		     if ((c = get_char()) == EOF) goto fini;
		     n = tolower(c) - '0';
		     if (n > 9) n -= ('a' - ':');
		     if (n & ~0x0f) {
			unget_char(c);
			if (i == 0) {
			   comp_error("Illegal character 0x%02x",c);
			   goto fini;
			}
			break;
		     }
		     val <<= 4;
		     val += n;
		 }
	      }
	      else if (isdigit(c)) {
		 unget_char(c);
		 for (val = 0, i = 0; i < 3; i++) {
		     if ((c = get_char()) == EOF) return (EOF);
		     n = c - '0';
		     if (n & ~0x07) {
			unget_char(c);
			if (i == 0) {
			   comp_error("Illegal character 0x%02x",c);
			   goto fini;
			}
			break;
		     }
		     val <<= 3;
		     val += n;
		 }
	      }
	      else {
		 unget_char(c);
		 val = 0;
	      }
	   }
	   else {
	      boolean neg = false;

	      if (c == '-') {
		 if ((c = get_char()) == EOF) goto fini;
		 unget_char(c);
		 if (!isdigit(c)) {
		    c = '-';
		    goto is_op;
		 }
		 neg = true;
	      }
	      else
		 unget_char(c);

	      for (val = 0, i = 0; i < 5; i++) {
		  if ((c = get_char()) == EOF) goto fini;
		  if (!isdigit(c)) {
		     unget_char(c);
		     if (i == 0) {
			comp_error("Illegal character 0x%02x",c);
			goto fini;
		     }
		     break;
		  }
		  val *= 10;
		  val += (c - '0');
	      }

	      if (neg) val = -val;
	      if (val < -32768L || val > 32767L) {
		 comp_error("Integer constant out of range");
		 goto fini;
	      }
	   }

	   item.token = ICONST;
	   item.x.iconst = (short) val;
	}

	else if (isalpha(c) || c == '_' || c == '$') {
	   do {
	      if (isalnum(c) || c == '_' || c == '$') {
		 if (i >= MAX_LINE) {
		    comp_error("Identifier too long");
		    break;
		 }
		 linebuf[i++] = c;
	      }
	      else if (allow_label && c == ':') {
		 linebuf[i] = '\0';
		 post_label(linebuf);
#ifdef PARSE_DEBUG
printf("label\t%s\n",linebuf);
#endif
		 i = 0;
		 need_statement = true;
		 goto again;
	      }
	      else {
		 unget_char(c);
		 linebuf[i] = '\0';
		 for (i = 0; token_table[i] != NULL; i++)
		     if (!strcmp(linebuf,token_table[i])) break;
		 if (token_table[i] != NULL)
		    item.token = i;
		 else {
		    item.token = IDENT;
		    item.x.ident = linebuf;
		 }
	      }
	   } while (!item.token && (c = get_char()) != EOF);
	}

	else if (strchr("{}()?:,;+-*/%&^|~=!<>",c)) {
is_op:	   linebuf[i++] = c;
	   if (strchr("+-&|=<>",c)) {
	      if ((c = get_char()) == EOF) goto fini;
	      if (c == linebuf[0]) linebuf[i++] = c;
	      else		   unget_char(c);
	      c = linebuf[0];
	   }
	   if ((i == 1 && strchr("+-*/%&^|!",c)) || strchr("<>",c)) {
	      if ((c = get_char()) == EOF) goto fini;
	      if (c == '=') linebuf[i++] = c;
	      else	    unget_char(c);
	   }

	   linebuf[i] = '\0';
	   for (i = 0; token_table[i] != NULL; i++)
	       if (!strcmp(linebuf,token_table[i])) break;
	   if (token_table[i] != NULL)
	      item.token = i;
	   else
	      comp_error("Illegal character 0x%02x",c);
	}

	else
	   comp_error("Illegal character 0x%02x",c);

fini:	;

#ifdef PARSE_DEBUG
{ register char *p;
  switch (item.token) {
    case NONE:	 printf("none\n");
		 break;
    case IDENT:  printf("ident\t%s\n",item.x.ident);
		 break;
    case SCONST: printf("sconst\t\"");
		 for (p = item.x.sconst; *p; put_escchar(*p++));
		 printf("\"\n");
		 break;
    case CCONST: printf("cconst\t'");
		 put_escchar(item.x.cconst);
		 printf("'\n");
		 break;
    case ICONST: printf("iconst\t%d (0x%04x 0%03o)\n", (int) item.x.iconst,
			(uint) item.x.iconst, (uint) item.x.iconst);
		 break;
    default:	 printf("token\t%s\n",token_table[item.token]);
		 break;
  }
}
#endif

	allow_label = false;
	return (item.token);
}/*get_item()*/


/*---------------------------------------------------------------------------*/
static word get_label (void)
{
	return (++labelcount);
}/*get_label()*/


static char *counttolabel (word cnt)
{
	static char lblbuf[7];

	sprintf(lblbuf,"@%u",(uint) cnt);
	return (lblbuf);
}/*counttolabel()*/


/*---------------------------------------------------------------------------*/
static void post_label (char *s)
{
	LABEL *lp, *newlp;

	for (lp = label; lp != NULL; lp = lp->next) {
	    if (!strcmp(lp->s,s)) {
	       comp_error("Duplicate label '%s'",s);
	       return;
	    }
	}

	newlp = (LABEL *) mycalloc(sizeof (LABEL));
	newlp->s   = mydup(s);
	newlp->ofs = info.code_length;

	if (!label)
	   label = newlp;
	else {
	   for (lp = label; lp->next != NULL; lp = lp->next);
	   lp->next = newlp;
	}
}/*post_label()*/


/*---------------------------------------------------------------------------*/
static void add_xref (char *s)
{
	LABEL *lp;
	XREF  *xp, *newxp;

	for (lp = label; lp != NULL && strcmp(lp->s,s); lp = lp->next);
	if (lp) {
	   lp->flags |= LBL_USE;
	   emit_word(lp->ofs);
	}
	else {
	   newxp = (XREF *) mycalloc(sizeof (XREF));
	   newxp->s   = mydup(s);
	   newxp->ofs = info.code_length;

	   if (!xref)
	      xref = newxp;
	   else {
	      for (xp = xref; xp->next != NULL; xp = xp->next);
	      xp->next = newxp;
	   }
	   emit_word(OFS_NULL);
	}
}/*add_xref()*/


/*---------------------------------------------------------------------------*/
static word add_string (char *s)
{
	STRING *sp, *newsp;

	for (sp = string; sp != NULL; sp = sp->next) {
	    if (!strcmp(sp->s,s)) return (sp->ofs);
	}

	if ((info.data_length + strlen(s) + 1) > MAX_DATA) {
	   comp_error("Out of string space");
	   erl_exit(ERL_FATAL);
	}

	newsp = (STRING *) mycalloc(sizeof (STRING));
	newsp->s   = mydup(s);
	newsp->ofs = info.data_length;
	info.data_length += strlen(s) + 1;

	if (!string)
	   string = newsp;
	else {
	   for (sp = string; sp->next != NULL; sp = sp->next);
	   sp->next = newsp;
	}

	return (newsp->ofs);
}/*add_string()*/


/*---------------------------------------------------------------------------*/
static void add_variable (char *s)
{
	VARIABLE *vp, *newvp;

	for (vp = variable; vp != NULL; vp = vp->next) {
	    if (!strcmp(vp->s,s)) {
	       comp_error("Duplicate definition of variable '%s'",s);
	       return;
	    }
	}

	if (info.num_vars >= MAX_VAR) {
	   comp_error("Too many variables");
	   erl_exit(ERL_FATAL);
	}

	newvp = (VARIABLE *) mycalloc(sizeof (VARIABLE));
	newvp->s    = mydup(s);
	newvp->idx  = info.num_vars++;

	if (!variable)
	   variable = newvp;
	else {
	   for (vp = variable; vp->next != NULL; vp = vp->next);
	   vp->next = newvp;
	}
}/*add_variable()*/


static word lookup_variable (char *s)
{
	VARIABLE *vp;

	for (vp = variable; vp != NULL; vp = vp->next) {
	    if (!strcmp(vp->s,s))
	       return (vp->idx);
	}

	comp_error("Undefined variable '%s'",s);
	return (OFS_NULL);
}/*lookup_variabe()*/


static void load_variable (EXPR *expr)
{
	VARIABLE *vp;

	emit_byte(m_lda);
	emit_word(expr->varidx);
	expr->sts = STS_REG;

	for (vp = variable; vp != NULL; vp = vp->next) {
	    if (vp->idx == expr->varidx) {
	       if (!(vp->flags & VAR_ASSIGN))
		  comp_warning("Possible use of variable '%s' before definition",vp->s);
	       vp->flags |= VAR_USE;
	       break;
	    }
	}
}/*load_variable()*/


static void assign_variable (EXPR *expr, MNEMONIC op)
{
	VARIABLE *vp;

	emit_byte(op);
	emit_word(expr->varidx);
	expr->assigned = true;

	for (vp = variable; vp != NULL; vp = vp->next) {
	    if (vp->idx == expr->varidx) {
	       vp->flags |= VAR_ASSIGN;
	       break;
	    }
	}
}/*assign_variable()*/


static void store_variable (EXPR *expr)
{
	assign_variable(expr,m_sta);
}/*store_variable()*/


/*---------------------------------------------------------------------------*/
/*	info("start_date","end_date","return_dest","script_desc");	     */
static void do_info (void)
{
	if (get_item() != LPAREN) {
	   comp_error("info statement missing (");
	   unget_item();
	}

	if (get_item() != SCONST)
	   comp_error("String constant expected");
	else if (!(info.start_date = datetostamp(item.x.sconst)))
	   comp_error("Illegal start_date in info statement");

	if (get_item() != COMMA) {
	   comp_error("info statement missing ,");
	   unget_item();
	}

	if (get_item() != SCONST)
	   comp_error("String constant expected");
	else if (!(info.end_date = datetostamp(item.x.sconst)) ||
		 info.end_date <= info.start_date)
	   comp_error("Illegal end_date in info statement");

	if (get_item() != COMMA) {
	   comp_error("info statement missing ,");
	   unget_item();
	}

	if (get_item() != SCONST)
	   comp_error("String constant expected");
	else if (!item.x.sconst[0])
	   comp_error("Empty return_dest in info statement");
	else if (strlen(item.x.sconst) > MAX_ADR)
	   comp_error("return_dest too long in info statement");
	else
	   strncpy(info.return_dest,item.x.sconst,MAX_DESC + 1);

	if (get_item() != COMMA) {
	   comp_error("info statement missing ,");
	   unget_item();
	}

	if (get_item() != SCONST)
	   comp_error("String constant expected");
	else if (!item.x.sconst[0])
	   comp_error("Empty script_desc in info statement");
	else if (strlen(item.x.sconst) > MAX_DESC)
	   comp_error("script_desc too long in info statement");
	else
	   strncpy(info.script_desc,item.x.sconst,MAX_DESC + 1);

	if (get_item() != RPAREN) {
	   comp_error("info statement missing )");
	   unget_item();
	}

	if (get_item() != SEMICOLON) {
	   comp_error("info statement missing ;");
	   unget_item();
	}
}/*do_info()*/


/*---------------------------------------------------------------------------*/
static void do_declare (void)
{
	do {
	   if (get_item() == IDENT)
	      add_variable(item.x.ident);
	   else
	      comp_error("Variable name expected");
	} while (get_item() == COMMA);

	if (item.token != SEMICOLON) {
	   comp_error("Statement missing ;");
	   unget_item();
	}
}/*do_declare()*/


/*---------------------------------------------------------------------------*/
static void do_function (EXPR *expr, MNEMONIC op)
{
	if (get_item() != LPAREN) {
	   comp_error("Function call missing (");
	   unget_item();
	}

	switch (op) {
	       case m_geti:
	       case m_getc:
	       case m_puti:
	       case m_putc:
		    { EXPR putexpr;
		      if (nocomma_expr(&putexpr) == STS_VAR)
			 load_variable(&putexpr);
		      emit_byte(op);
		    }
		    break;

	       case m_puts:
		    if (get_item() != SCONST)
		       comp_error("String constant expected");
		    else {
		       emit_byte(op);
		       emit_word(add_string(item.x.sconst));
		    }
		    break;

	       case m_end:
		    { VARIABLE *vp;
		      emit_byte(op);
		      for (vp = variable; vp->idx < info.stored_vars; vp = vp->next)
			  vp->flags |= VAR_USE;
		    }
		    break;

	       case m_quit:
	       case m_enter:
	       case m_colour:
	       case m_cls:
		    emit_byte(op);
		    break;

	       case m_read:
		    { EXPR putexpr;
		      VARIABLE *vp;
		      if (nocomma_expr(&putexpr) == STS_VAR)
			 load_variable(&putexpr);
		      emit_byte(op);
		      for (vp = variable; vp->idx < info.stored_vars; vp = vp->next)
			  vp->flags |= VAR_ASSIGN;
		    }
		    break;
	}

	if (get_item() != RPAREN) {
	   comp_error("Function call missing )");
	   unget_item();
	}

	expr->sts = STS_REG;
}/*do_function()*/


/*---------------------------------------------------------------------------*/
static void prefix_op (EXPR *expr, MNEMONIC op)
{
	if (unary_expr(expr) != STS_VAR || expr->assigned)
	   comp_error("Lvalue required");
	else
	   assign_variable(expr,op);
}/*prefix_op()*/


/*---------------------------------------------------------------------------*/
static void postfix_op (EXPR *expr, MNEMONIC op)
{
	if (expr->sts != STS_VAR || expr->assigned)
	   comp_error("Lvalue required");
	else {
	   load_variable(expr);
	   assign_variable(expr,op);
	}
}/*postfix_op()*/


/*---------------------------------------------------------------------------*/
static void unary_op (EXPR *expr, MNEMONIC op)
{
	switch (unary_expr(expr)) {
	       case STS_NONE: comp_error("Expression required");
			      break;
	       case STS_VAR:  load_variable(expr);
			      /* fallthrough to STS_REG */
	       case STS_REG:  if (op) emit_byte(op);
			      break;
	}
}/*unary_op()*/


/*---------------------------------------------------------------------------*/
static EXPR_STS unary_expr (EXPR *expr)
{
	expr->sts = STS_NONE;
	expr->assigned = false;

	switch (get_item()) {
	       case IDENT:  expr->varidx = lookup_variable(item.x.ident);
			    expr->sts = STS_VAR;
			    break;
	       case CCONST: emit_byte(m_ldai);
			    emit_short(item.x.cconst);
			    expr->sts = STS_REG;
			    break;
	       case ICONST: emit_byte(m_ldai);
			    emit_short(item.x.iconst);
			    expr->sts = STS_REG;
			    break;

	       case GETI:   do_function(expr,m_geti);	break;
	       case GETC:   do_function(expr,m_getc);	break;
	       case PUTI:   do_function(expr,m_puti);	break;
	       case PUTC:   do_function(expr,m_putc);	break;
	       case PUTS:   do_function(expr,m_puts);	break;
	       case END:    do_function(expr,m_end);	break;
	       case QUIT:   do_function(expr,m_quit);	break;
	       case ENTER:  do_function(expr,m_enter);	break;
	       case COLOUR: do_function(expr,m_colour); break;
	       case CLS:    do_function(expr,m_cls);	break;
	       case READ:   do_function(expr,m_read);	break;
	       case INC:    prefix_op(expr,m_inc); break;
	       case DEC:    prefix_op(expr,m_dec); break;
	       case LOGNOT: unary_op(expr,m_lognot); break;
	       case ONECOM: unary_op(expr,m_onecom); break;
	       case PLUS:   unary_op(expr,m_nop);    break;
	       case MINUS:  unary_op(expr,m_neg);    break;
	       case LPAREN: comma_expr(expr);
			    if (get_item() != RPAREN) {
			       comp_error(") expected");
			       unget_item();
			    }
			    break;

	       default:     unget_item();
			    goto fini;
	}

	switch (get_item()) {
	       case INC: postfix_op(expr,m_inc); break;
	       case DEC: postfix_op(expr,m_dec); break;
	       default:  unget_item(); break;
	}

fini:	return (expr->sts);
}/*unary_expr()*/


/*---------------------------------------------------------------------------*/
static void mul_op (EXPR *expr)
{
	MNEMONIC op;

	for (;;) {
	    switch (get_item()) {
		   case MUL: op = m_mul; break;
		   case DIV: op = m_div; break;
		   case MOD: op = m_mod; break;
		   default:  unget_item(); return;
	    }

	    if (expr->sts == STS_VAR)
	       load_variable(expr);
	    emit_byte(m_push);

	    if (!unary_expr(expr)) {
	       comp_error("Expression required");
	       return;
	    }
	    if (expr->sts == STS_VAR)
	       load_variable(expr);
	    emit_byte(op);
	}
}/*mul_op()*/


/*---------------------------------------------------------------------------*/
static void add_op (EXPR *expr)
{
	MNEMONIC op;

	mul_op(expr);

	for (;;) {
	    switch (get_item()) {
		   case PLUS:  op = m_add; break;
		   case MINUS: op = m_sub; break;
		   default:    unget_item(); return;
	    }

	    if (expr->sts == STS_VAR)
	       load_variable(expr);
	    emit_byte(m_push);

	    if (!unary_expr(expr)) {
	       comp_error("Expression required");
	       return;
	    }
	    mul_op(expr);
	    if (expr->sts == STS_VAR)
	       load_variable(expr);
	    emit_byte(op);
	}
}/*add_op()*/


/*---------------------------------------------------------------------------*/
static void shift_op (EXPR *expr)
{
	MNEMONIC op;

	add_op(expr);

	switch (get_item()) {
	       case SHR: op = m_shr; break;
	       case SHL: op = m_shl; break;
	       default:  unget_item(); return;
	}

	if (expr->sts == STS_VAR)
	   load_variable(expr);
	emit_byte(m_push);

	if (!unary_expr(expr)) {
	   comp_error("Expression required");
	   return;
	}
	add_op(expr);
	if (expr->sts == STS_VAR)
	   load_variable(expr);
	emit_byte(op);
}/*shift_op()*/


/*---------------------------------------------------------------------------*/
static void rel_op (EXPR *expr)
{
	MNEMONIC op;

	shift_op(expr);

	switch (get_item()) {
	       case LESS:	  op = m_ls;  break;
	       case GREATER:	  op = m_gt;  break;
	       case LESSEQUAL:	  op = m_lse; break;
	       case GREATEREQUAL: op = m_gte; break;
	       default: 	  unget_item(); return;
	}

	if (expr->sts == STS_VAR)
	   load_variable(expr);
	emit_byte(m_push);

	if (!unary_expr(expr)) {
	   comp_error("Expression required");
	   return;
	}
	shift_op(expr);
	if (expr->sts == STS_VAR)
	   load_variable(expr);
	emit_byte(op);
}/*rel_op()*/


/*---------------------------------------------------------------------------*/
static void eq_op (EXPR *expr)
{
	MNEMONIC op;

	rel_op(expr);

	switch (get_item()) {
	       case EQUAL:    op = m_eq; break;
	       case NOTEQUAL: op = m_ne; break;
	       default:       unget_item(); return;
	}

	if (expr->sts == STS_VAR)
	   load_variable(expr);
	emit_byte(m_push);

	if (!unary_expr(expr)) {
	   comp_error("Expression required");
	   return;
	}
	rel_op(expr);
	if (expr->sts == STS_VAR)
	   load_variable(expr);
	emit_byte(op);
}/*eq_op()*/


/*---------------------------------------------------------------------------*/
static void and_op (EXPR *expr)
{
	eq_op(expr);

	while (get_item() == AND) {
	      if (expr->sts == STS_VAR)
		 load_variable(expr);
	      emit_byte(m_push);

	      if (!unary_expr(expr)) {
		 comp_error("Expression required");
		 return;
	      }
	      eq_op(expr);
	      if (expr->sts == STS_VAR)
		 load_variable(expr);
	      emit_byte(m_and);
	}

	unget_item();
}/*and_op()*/


/*---------------------------------------------------------------------------*/
static void xor_op (EXPR *expr)
{
	and_op(expr);

	while (get_item() == XOR) {
	      if (expr->sts == STS_VAR)
		 load_variable(expr);
	      emit_byte(m_push);

	      if (!unary_expr(expr)) {
		 comp_error("Expression required");
		 return;
	      }
	      and_op(expr);
	      if (expr->sts == STS_VAR)
		 load_variable(expr);
	      emit_byte(m_xor);
	}

	unget_item();
}/*xor_op()*/


/*---------------------------------------------------------------------------*/
static void or_op (EXPR *expr)
{
	xor_op(expr);

	while (get_item() == OR) {
	      if (expr->sts == STS_VAR)
		 load_variable(expr);
	      emit_byte(m_push);

	      if (!unary_expr(expr)) {
		 comp_error("Expression required");
		 return;
	      }
	      xor_op(expr);
	      if (expr->sts == STS_VAR)
		 load_variable(expr);
	      emit_byte(m_or);
	}

	unget_item();
}/*or_op()*/


/*---------------------------------------------------------------------------*/
static void logand_op (EXPR *expr)
{
	or_op(expr);

	if (get_item() == LOGAND) {
	   word pastand = get_label();

	   if (expr->sts == STS_VAR)
	      load_variable(expr);

	   do {
	      emit_byte(m_jz);
	      add_xref(counttolabel(pastand));

	      if (!unary_expr(expr)) {
		 comp_error("Expression required");
		 return;
	      }
	      or_op(expr);
	      if (expr->sts == STS_VAR)
		 load_variable(expr);
	   } while (get_item() == LOGAND);

	   post_label(counttolabel(pastand));
	}

	unget_item();
}/*logand_op()*/


/*---------------------------------------------------------------------------*/
static void logor_op (EXPR *expr)
{
	logand_op(expr);

	if (get_item() == LOGOR) {
	   word pastor = get_label();

	   if (expr->sts == STS_VAR)
	      load_variable(expr);

	   do {
	      emit_byte(m_jnz);
	      add_xref(counttolabel(pastor));

	      if (!unary_expr(expr)) {
		 comp_error("Expression required");
		 return;
	      }
	      logand_op(expr);
	      if (expr->sts == STS_VAR)
		 load_variable(expr);
	   } while (get_item() == LOGOR);

	   post_label(counttolabel(pastor));
	}

	unget_item();
}/*logor_op()*/


/*---------------------------------------------------------------------------*/
static void cond_op (EXPR *expr)
{
	logor_op(expr);

	if (get_item() == QUESTION) {
	   word pastif, pastelse;

	   if (expr->sts == STS_VAR)
	      load_variable(expr);
	   pastif = get_label();
	   emit_byte(m_jz);
	   add_xref(counttolabel(pastif));
	   if (nocomma_expr(expr) == STS_VAR)
	      load_variable(expr);
	   if (get_item() != COLON) {
	      comp_error(": expected");
	      unget_item();
	   }
	   pastelse = get_label();
	   emit_byte(m_jmp);
	   add_xref(counttolabel(pastelse));
	   post_label(counttolabel(pastif));
	   if (nocomma_expr(expr) == STS_VAR)
	      load_variable(expr);
	   post_label(counttolabel(pastelse));
	}
	else
	   unget_item();
}/*cond_op()*/


/*---------------------------------------------------------------------------*/
static void assign_op (EXPR *expr)
{
	MNEMONIC op;
	EXPR	 assexpr;

	switch (get_item()) {
	       case ASSIGN: op = m_nop; break;
	       case ADDASS: op = m_add; break;
	       case SUBASS: op = m_sub; break;
	       case MULASS: op = m_mul; break;
	       case DIVASS: op = m_div; break;
	       case MODASS: op = m_mod; break;
	       case SHRASS: op = m_shr; break;
	       case SHLASS: op = m_shl; break;
	       case ANDASS: op = m_and; break;
	       case XORASS: op = m_xor; break;
	       case ORASS:  op = m_or;	break;
	       default:     unget_item(); cond_op(expr); return;
	}

	if (expr->sts != STS_VAR || expr->assigned) {
	   comp_error("Lvalue required");
	   return;
	}

	if (op) {
	   load_variable(expr);
	   emit_byte(m_push);
	}
	if (nocomma_expr(&assexpr) == STS_VAR)
	   load_variable(&assexpr);
	if (op) emit_byte(op);
	store_variable(expr);
}/*assign_op()*/


/*---------------------------------------------------------------------------*/
static EXPR_STS nocomma_expr (EXPR *expr)
{
	if (unary_expr(expr))
	   assign_op(expr);
	else
	   comp_error("Expression required");

	return (expr->sts);
}/*nocomma_expr()*/


/*---------------------------------------------------------------------------*/
static EXPR_STS comma_expr (EXPR *expr)
{
	do nocomma_expr(expr);
	while (get_item() == COMMA);
	unget_item();
	return (expr->sts);
}/*comma_expr()*/


/*---------------------------------------------------------------------------*/
/*	if:	comma_expr	if/else:  comma_expr
		jz pastif		  jz pastif
		statement		  statement
	pastif: ...			  goto pastelse
				pastif:   statement
				pastelse: ...
*/
static void do_if (word loopcont, word loopbrk)
{
	word pastif, pastelse;
	EXPR expr;

	if (get_item() != LPAREN) {
	   comp_error("if statement missing (");
	   unget_item();
	}

	if (comma_expr(&expr) == STS_VAR)
	   load_variable(&expr);

	if (get_item() != RPAREN) {
	   comp_error("if statement missing )");
	   unget_item();
	}

	pastif = get_label();
	emit_byte(m_jz);
	add_xref(counttolabel(pastif));

	do_statement(loopcont,loopbrk,false);

	if (get_item() == ELSE) {
	   pastelse = get_label();
	   emit_byte(m_jmp);
	   add_xref(counttolabel(pastelse));
	   post_label(counttolabel(pastif));

	   do_statement(loopcont,loopbrk,false);

	   post_label(counttolabel(pastelse));
	}
	else {
	   unget_item();
	   post_label(counttolabel(pastif));
	}
}/*do_if()*/


/*---------------------------------------------------------------------------*/
/*	for:	  comma_expr
	looptop:  comma_expr
		  jz loopbrk
		  jmp loopcmd
	loopcont: comma_expr		; label for continue statement
		  jmp looptop
	loopcmd:  statement
		  jmp loopcont
	loopbrk:  ...			; label for break statement
*/
static void do_for (void)
{
	word looptop  = get_label(),
	     loopcmd  = get_label(),
	     loopcont = get_label(),
	     loopbrk  = get_label();
	EXPR expr;

	if (get_item() != LPAREN) {
	   comp_error("for statement missing (");
	   unget_item();
	}

	if (get_item() != SEMICOLON) {
	   unget_item();
	   comma_expr(&expr);
	   if (get_item() != SEMICOLON) {
	      comp_error("for statement missing ;");
	      unget_item();
	   }
	}

	post_label(counttolabel(looptop));
	if (get_item() != SEMICOLON) {
	   unget_item();
	   if (comma_expr(&expr) == STS_VAR)
	      load_variable(&expr);
	   emit_byte(m_jz);
	   add_xref(counttolabel(loopbrk));
	   if (get_item() != SEMICOLON) {
	      comp_error("for statement missing ;");
	      unget_item();
	   }
	}
	emit_byte(m_jmp);
	add_xref(counttolabel(loopcmd));

	post_label(counttolabel(loopcont));
	if (get_item() != RPAREN) {
	   unget_item();
	   comma_expr(&expr);
	   if (get_item() != RPAREN) {
	      comp_error("for statement missing )");
	      unget_item();
	   }
	}
	emit_byte(m_jmp);
	add_xref(counttolabel(looptop));

	post_label(counttolabel(loopcmd));
	do_statement(loopcont,loopbrk,false);
	emit_byte(m_jmp);
	add_xref(counttolabel(loopcont));

	post_label(counttolabel(loopbrk));
}/*do_for()*/


/*---------------------------------------------------------------------------*/
/*	do/while:
	looptop:  statement
	loopcont: comma_expr		; label for continue statement
		  jnz looptop
	loopbrk:  ...			; label for break statement
*/
static void do_do (void)
{
	word looptop  = get_label(),
	     loopcont = get_label(),
	     loopbrk  = get_label();
	EXPR expr;

	post_label(counttolabel(looptop));
	do_statement(loopcont,loopbrk,false);

	post_label(counttolabel(loopcont));

	if (get_item() != WHILE) {
	   comp_error("do statement must have a while");
	   unget_item();
	   return;
	}

	if (get_item() != LPAREN) {
	   comp_error("do-while statement missing (");
	   unget_item();
	}

	if (comma_expr(&expr) == STS_VAR)
	   load_variable(&expr);
	emit_byte(m_jnz);
	add_xref(counttolabel(looptop));

	if (get_item() != RPAREN) {
	   comp_error("do-while statement missing )");
	   unget_item();
	}

	if (get_item() != SEMICOLON) {
	   comp_error("do-while statement missing ;");
	   unget_item();
	}

	post_label(counttolabel(loopbrk));
}/*do_do()*/


/*---------------------------------------------------------------------------*/
/*	while:
	loopcont: comma_expr		; label for continue statement
		  jz loopbrk
		  statement
		  jmp loopcont
	loopbrk:  ...			; label for break statement
*/
static void do_while (void)
{
	word loopcont = get_label(),
	     loopbrk  = get_label();
	EXPR expr;

	post_label(counttolabel(loopcont));

	if (get_item() != LPAREN) {
	   comp_error("while statement missing (");
	   unget_item();
	}

	if (comma_expr(&expr) == STS_VAR)
	   load_variable(&expr);
	emit_byte(m_jz);
	add_xref(counttolabel(loopbrk));

	if (get_item() != RPAREN) {
	   comp_error("while statement missing )");
	   unget_item();
	}

	do_statement(loopcont,loopbrk,false);
	emit_byte(m_jmp);
	add_xref(counttolabel(loopcont));

	post_label(counttolabel(loopbrk));
}/*do_while()*/


/*---------------------------------------------------------------------------*/
static void do_goto (void)
{
	if (get_item() != IDENT)
	   comp_error("Label identifier expected");
	else {
	   emit_byte(m_jmp);
	   add_xref(item.x.ident);
	}
}/*do_goto()*/


/*---------------------------------------------------------------------------*/
static void do_call (void)
{
	if (get_item() != IDENT)
	   comp_error("Label identifier expected");
	else {
	   emit_byte(m_call);
	   add_xref(item.x.ident);
	}
}/*do_call()*/


/*---------------------------------------------------------------------------*/
static boolean do_statement (word loopcont, word loopbrk, boolean allow_rbracket)
{
again:	allow_label = true;

	switch (get_item()) {
	       case IDENT:
	       case GETI:
	       case GETC:
	       case PUTI:
	       case PUTC:
	       case PUTS:
	       case END:
	       case QUIT:
	       case ENTER:
	       case COLOUR:
	       case CLS:
	       case READ:
	       case INC:
	       case DEC:
	       case LPAREN:    unget_item();
			       { EXPR expr;
				 comma_expr(&expr);
			       }
			       break;

	       case INFO:      comp_error("info statement not allowed inside program");
			       break;

	       case STORE:
	       case INTERN:    comp_error("Variable declaration not allowed inside program");
			       break;

	       case IF:        do_if(loopcont,loopbrk);
			       goto fini;

	       case FOR:       do_for();
			       goto fini;

	       case DO:        do_do();
			       goto fini;

	       case WHILE:     do_while();
			       goto fini;

	       case CONTINUE:  if (loopcont == OFS_NULL)
				  comp_error("Misplaced continue");
			       else {
				  emit_byte(m_jmp);
				  add_xref(counttolabel(loopcont));
			       }
			       break;

	       case BREAK:     if (loopbrk == OFS_NULL)
				  comp_error("Misplaced break");
			       else {
				  emit_byte(m_jmp);
				  add_xref(counttolabel(loopbrk));
			       }
			       break;

	       case ELSE:      comp_error("Misplaced else");
			       break;

	       case GOTO:      do_goto();
			       break;

	       case CALL:      do_call();
			       break;

	       case RETURN:    emit_byte(m_return);
			       break;

	       case LBRACKET:  do_compound(loopcont,loopbrk);
			       goto fini;

	       case SEMICOLON: unget_item(); break;

	       case RBRACKET:  if (!need_statement) {
				  if (!allow_rbracket) {
				     comp_error("Unexpected }");
				     goto again;
				  }
				  return (true);
			       }
			       /* fallthrough to default */

	       default:        comp_error("Statement or expression expected");
			       break;
	}

	if (get_item() != SEMICOLON) {
	   comp_error("Statement missing ;");
	   unget_item();
	}

fini:	return (false);
}/*do_statement()*/


/*---------------------------------------------------------------------------*/
static void do_compound (word loopcont, word loopbrk)
{
	while (!do_statement(loopcont,loopbrk,true));
}/*do_compound()*/


/*---------------------------------------------------------------------------*/
static void compile (void)
{
	VARIABLE *vp;

	if (get_item() != INFO) {
	   comp_error("Expected info statement");
	   unget_item();
	}
	else
	   do_info();

	while (get_item() == STORE)
	      do_declare();
	unget_item();
	if (!info.num_vars)
	   comp_error("Expected store statement(s)");
	info.stored_vars = info.num_vars;

	while (get_item() == INTERN)
	      do_declare();

	if (item.token != LBRACKET) {
	   comp_error("{ expected");
	   unget_item();
	}
	do_compound(OFS_NULL,OFS_NULL);

	if (get_item())
	   comp_error("Statement(s) past end of program");

	if (!info.code_length)
	   comp_error("Script contains no code");

	for (vp = variable; vp; vp = vp->next) {
	    if (vp->flags & VAR_USE && !(vp->flags & VAR_ASSIGN))
	       comp_warning("Variable '%s' is used but never assigned",vp->s);
	    else if (vp->flags & VAR_ASSIGN && !(vp->flags & VAR_USE))
	       comp_warning("Variable '%s' is assigned but never used",vp->s);
	    else if (!(vp->flags & (VAR_USE | VAR_ASSIGN)))
	       comp_warning("Variable '%s' declared but never used",vp->s);
	}
}/*compile()*/


/*---------------------------------------------------------------------------*/
static void resolve_xref (void)
{
	XREF  *xp;
	LABEL *lp;

	for (xp = xref; xp; xp = xp->next) {
	    for (lp = label; lp && strcmp(lp->s,xp->s); lp = lp->next);
	    if (lp) {
	       code[xp->ofs]	 = (byte) lp->ofs;
	       code[xp->ofs + 1] = (byte) (lp->ofs >> 8);
	       lp->flags |= LBL_USE;
	    }
	    else
	       comp_error("Undefined label '%s'",xp->s);
	}

	for (lp = label; lp; lp = lp->next) {
	    if (!(lp->flags & LBL_USE))
	       comp_warning("Label '%s' never referenced",lp->s);
	}
}/*resolve_xref()*/


/*---------------------------------------------------------------------------*/
static void write_byte (byte c)
{
	c &= 0xff;
	info.hdr_crc = crc32byte(info.hdr_crc,c);
	putc(c,fp);
	hdr_len++;
}/*write_byte()*/

static void write_str (char *s, word size)
{
	info.hdr_crc = crc32block(info.hdr_crc,(byte *) s,(word) size);
	fwrite(s,(uint) size,1,fp);
	hdr_len += size;
}/*write_str()*/

static void write_word (word w)
{
	write_byte((byte) w);
	write_byte((byte) (w >> 8));
}/*write_word()*/

static void write_long (long l)
{
	write_byte((byte) l);
	write_byte((byte) (l >> 8));
	write_byte((byte) (l >> 16));
	write_byte((byte) (l >> 24));
}/*write_long()*/


static void write_com (void)
{
	STRING *sp;
	long   save_crc;

	sprintf(buffer,"%s.%s",info.script_name,EXT_COM);
	if ((fp = fopen(buffer,BWRITE)) == NULL) {
	   comp_error("Can't create output file '%s'",buffer);
	   erl_exit(ERL_FATAL);
	}

	info.file_crc = crc32init();
	info.file_crc = crc32block(info.file_crc, code, info.code_length);
	info.file_length += info.code_length;
	for (sp = string; sp; sp = sp->next) {
	    info.file_crc = crc32block(info.file_crc, (byte *) sp->s,
				       (word) strlen(sp->s) + 1);
	    info.file_length += strlen(sp->s) + 1;
	}
	info.file_crc = crc32fini(info.file_crc);

	hdr_len = 0;
	info.hdr_crc = crc32init();
	write_str  ( info.idstring	, LEN_IDSTR	);
	write_word ( info.revision			);
	write_str  ( info.type_id	, LEN_WAQID + 1 );
	write_long ( info.compile_stamp 		);
	write_str  ( info.script_name	, MAX_NAME  + 1 );
	write_str  ( info.script_desc	, MAX_DESC  + 1 );
	write_long ( info.start_date			);
	write_long ( info.end_date			);
	write_str  ( info.return_dest	, MAX_ADR   + 1 );
	write_str  ( info.arc_list	, MAX_ARC   + 1 );
	write_long ( info.return_stamp			);
	write_str  ( info.return_orig	, MAX_ADR   + 1 );
	write_word ( info.code_length			);
	write_word ( info.data_length			);
	write_word ( info.num_vars			);
	write_word ( info.stored_vars			);
	write_long ( info.file_length			);
	write_long ( info.file_crc			);

	save_crc = crc32fini(info.hdr_crc);
	write_long ( save_crc				);
	info.hdr_crc = save_crc;

	fwrite(code,(uint) info.code_length,1,fp);
	for (sp = string; sp; sp = sp->next)
	    fwrite(sp->s,(uint) strlen(sp->s) + 1,1,fp);

	fclose(fp);
	fp = NULL;
}/*write_com()*/


/*---------------------------------------------------------------------------*/
static void write_map (void)
{
	VARIABLE *vp;
	LABEL	 *lp;
	XREF	 *xp;

	sprintf(buffer,"%s.%s",info.script_name,EXT_MAP);
	if ((fp = fopen(buffer,TWRITE)) == NULL) {
	   comp_error("Can't create map file '%s'",buffer);
	   erl_exit(ERL_FATAL);
	}

	fprintf(fp,"WAQCOMP version = %s\n",	VERSION);
	fprintf(fp,"Format revision = %u\n",	(uint) info.revision);
	fprintf(fp,"Sourcefile      = %s\n",	sourcefile);
	fprintf(fp,"Compile stamp   = %s\n",	stamptodate(info.compile_stamp));
	fprintf(fp,"Script name     = %s\n",	info.script_name);
	fprintf(fp,"Script desc     = %s\n",	info.script_desc);
	fprintf(fp,"Start date      = %s GMT/UTC\n", stamptodate(info.start_date));
	fprintf(fp,"End date        = %s GMT/UTC\n", stamptodate(info.end_date));
	fprintf(fp,"Return dest     = %s\n",	info.return_dest);
	fprintf(fp,"Arc list        = %s\n",	info.arc_list[0] ? info.arc_list : "<none>");
	fprintf(fp,"Return stamp    = <empty>\n");
	fprintf(fp,"Return orig     = <empty>\n");
	fprintf(fp,"Code length     = %u\n",	(uint) info.code_length);
	fprintf(fp,"Data length     = %u\n",	(uint) info.data_length);
	fprintf(fp,"Num vars        = %u\n",	(uint) info.num_vars);
	fprintf(fp,"Stored vars     = %u\n",	(uint) info.stored_vars);
	fprintf(fp,"File length     = hdr(%u) + %lu = %lu\n",
		   (uint) hdr_len, info.file_length,
		   (long) info.file_length + hdr_len);
	fprintf(fp,"File CRC        = %08lx\n", info.file_crc);
	fprintf(fp,"WAQ header CRC  = %08lx\n", info.hdr_crc);
	fprintf(fp,"\n");

	fprintf(fp,"Stored variables:\n");
	fprintf(fp,"    Index   Identifier\n");
	for (vp = variable; vp && vp->idx < info.stored_vars; vp = vp->next)
	    fprintf(fp,"    %-4u    %s\n",(uint) vp->idx,vp->s);
	fprintf(fp,"\n");

	fprintf(fp,"Internal variables:\n");
	if (vp) {
	   fprintf(fp,"    Index   Identifier\n");
	   for ( ; vp; vp = vp->next)
	       fprintf(fp,"    %-4u    %s\n",(uint) vp->idx,vp->s);
	}
	else
	   fprintf(fp,"    NONE\n");
	fprintf(fp,"\n");

	fprintf(fp,"Labels:\n");
	if (label) {
	   fprintf(fp,"    Offset Label\n");
	   for (lp = label; lp; lp = lp->next)
	       fprintf(fp,"    %04x    %s\n",(uint) lp->ofs,lp->s);
	}
	else
	   fprintf(fp,"    NONE\n");
	fprintf(fp,"\n");

	fprintf(fp,"Forward references:\n");
	if (xref) {
	   fprintf(fp,"    Offset  Label\n");
	   for (xp = xref; xp; xp = xp->next)
	       fprintf(fp,"    %04x    %s\n",(uint) xp->ofs,xp->s);
	}
	else
	   fprintf(fp,"    NONE\n");
	fprintf(fp,"\n");

	fprintf(fp,"End of map file\n");

	fclose(fp);
	fp = NULL;
}/*write_map()*/


/*---------------------------------------------------------------------------*/
static void init (void)
{
	code = (byte *) mycalloc(MAX_CODE);

	string	 = NULL;
	variable = NULL;
	label	 = NULL;
	xref	 = NULL;

	memset(&info,0,(uint) sizeof (WAQ_INFO));
	strcpy(info.idstring,IDSTRING);
	info.revision = REVISION;
	strcpy(info.type_id,WAQ_ID);
	info.compile_stamp = time(NULL);

	errors	       = 0;
	sourceline     = 1;
	labelcount     = 0;
	item_saved     = false;
	allow_label    = false;
}/*init()*/


/*---------------------------------------------------------------------------*/
static void deinit (void)
{
	STRING	 *sp, *nextsp;
	VARIABLE *vp, *nextvp;
	LABEL	 *lp, *nextlp;
	XREF	 *xp, *nextxp;

	for (sp = string; sp != NULL; sp = nextsp) {
	    nextsp = sp->next;
	    free(sp->s);
	    free(sp);
	}
	for (vp = variable; vp != NULL; vp = nextvp) {
	    nextvp = vp->next;
	    free(vp->s);
	    free(vp);
	}
	for (lp = label; lp != NULL; lp = nextlp) {
	    nextlp = lp->next;
	    free(lp->s);
	    free(lp);
	}
	for (xp = xref; xp != NULL; xp = nextxp) {
	    nextxp = xp->next;
	    free(xp->s);
	    free(xp);
	}

	string	 = NULL;
	variable = NULL;
	label	 = NULL;
	xref	 = NULL;

	if (code != NULL) free(code);
	code = NULL;

	if (errors) {
	   sprintf(buffer,"%s.%s",info.script_name,EXT_COM);
	   if (ffirst(buffer)) remove(buffer);
	   sprintf(buffer,"%s.%s",info.script_name,EXT_MAP);
	   if (ffirst(buffer)) remove(buffer);
	}
}/*deinit()*/


/*---------------------------------------------------------------------------*/
int main (int argc, char *argv[])
{
	char	*p, *q;
	int	 n;
	boolean  ok = true;

	printf("WAQCOMP Version %s; Wide Area Query Compiler (format revision %u)\n",VERSION,REVISION);
	printf("Design & COPYRIGHT (C) 1992 by A.G.Lentz & T.J.Caulfeild; ALL RIGHTS RESERVED\n");
	printf("Wide Area Query - A project of LENTZ SOFTWARE-DEVELOPMENT & EuroBaud Software\n\n");

	if (argc < 2) {
	   printf("Usage: WAQCOMP <filespec>[.%s] ...\n",EXT_SRC);
	   return (ERL_OK);
	}

	signal(SIGINT,user_break);

	for (n = 1; n < argc; n++) {
	    strlwr(argv[n]);
	    splitpath(argv[n],path,buffer);
	    if (strchr(buffer,'.') == NULL)
	       sprintf(buffer,"%s.%s",argv[n],EXT_SRC);
	    else
	       strcpy(buffer,argv[n]);
	    if ((p = ffirst(buffer)) == NULL) {
	       printf("Error: No files matching '%s'\n",buffer);
	       ok = false;
	       continue;
	    }

	    do {
	       strlwr(p);
	       mergepath(sourcefile,path,p);
	       if ((q = strchr(p,'.')) != NULL) *q = '\0';
	       if (strlen(p) < 1 || strlen(p) > MAX_NAME) {
		  printf("Error: Invalid filename '%s'\n",sourcefile);
		  ok = false;
		  continue;
	       }

	       for (q = p; *q; q++) {
		  if (!isalnum(*q)) {
		     printf("Error: Scriptname '%s' contains invalid characters\n",p);
		     ok = false;
		     continue;
		  }
	       }

	       if ((fp = fopen(sourcefile,TREAD)) == NULL) {
		  printf("Error: Can't open source file '%s'\n",sourcefile);
		  ok = false;
		  continue;
	       }

	       init();
	       strcpy(info.script_name,p);
	       printf("%s:\n",sourcefile);
	       compile();
	       fclose(fp);
	       fp = NULL;
	       arc_cfg();
	       strcpy(info.arc_list,arc_list(false));
	       resolve_xref();

	       if (!errors) {
		  write_com();
		  write_map();
	       }
	       deinit();

	       if (errors) {
		  printf("*** %u error%s in compile ***\n",
			 errors, errors == 1 ? "" : "s");
		  ok = false;
	       }
	    } while ((p = fnext()) != NULL);
	}

	return (ok ? ERL_OK : ERL_ERRORS);
}


/* end of waqcomp.c */

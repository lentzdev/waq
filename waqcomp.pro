#ifdef __PROTO__
# define	PROTO(s) s
#else
# define	PROTO(s) ()
#endif


/* waqcomp.c */
static void erl_exit PROTO((int erl ));
static void comp_error PROTO((char *fmt , ...));
static void comp_warning PROTO((char *fmt , ...));
static void user_break PROTO((void ));
static void *myalloc PROTO((word nbytes ));
void *mycalloc PROTO((word nbytes ));
static char *mydup PROTO((char *s ));
static void emit_byte PROTO((byte b ));
static void emit_word PROTO((word w ));
static void emit_short PROTO((short i ));
static int get_char PROTO((void ));
static void unget_char PROTO((register int c ));
static int get_escchar PROTO((void ));
static void put_escchar PROTO((int c ));
static void unget_item PROTO((void ));
static TOKEN get_item PROTO((void ));
static word get_label PROTO((void ));
static char *counttolabel PROTO((word cnt ));
static void post_label PROTO((char *s ));
static void add_xref PROTO((char *s ));
static word add_string PROTO((char *s ));
static void add_variable PROTO((char *s ));
static word lookup_variable PROTO((char *s ));
static void load_variable PROTO((EXPR *expr ));
static void assign_variable PROTO((EXPR *expr , MNEMONIC op ));
static void store_variable PROTO((EXPR *expr ));
static void do_info PROTO((void ));
static void do_declare PROTO((void ));
static void do_function PROTO((EXPR *expr , MNEMONIC op ));
static void prefix_op PROTO((EXPR *expr , MNEMONIC op ));
static void postfix_op PROTO((EXPR *expr , MNEMONIC op ));
static void unary_op PROTO((EXPR *expr , MNEMONIC op ));
static EXPR_STS unary_expr PROTO((EXPR *expr ));
static void mul_op PROTO((EXPR *expr ));
static void add_op PROTO((EXPR *expr ));
static void shift_op PROTO((EXPR *expr ));
static void rel_op PROTO((EXPR *expr ));
static void eq_op PROTO((EXPR *expr ));
static void and_op PROTO((EXPR *expr ));
static void xor_op PROTO((EXPR *expr ));
static void or_op PROTO((EXPR *expr ));
static void logand_op PROTO((EXPR *expr ));
static void logor_op PROTO((EXPR *expr ));
static void cond_op PROTO((EXPR *expr ));
static void assign_op PROTO((EXPR *expr ));
static EXPR_STS nocomma_expr PROTO((EXPR *expr ));
static EXPR_STS comma_expr PROTO((EXPR *expr ));
static void do_if PROTO((word loopcont , word loopbrk ));
static void do_for PROTO((void ));
static void do_do PROTO((void ));
static void do_while PROTO((void ));
static void do_goto PROTO((void ));
static void do_call PROTO((void ));
static boolean do_statement PROTO((word loopcont , word loopbrk , boolean allow_rbracket ));
static void do_compound PROTO((word loopcont , word loopbrk ));
static void compile PROTO((void ));
static void resolve_xref PROTO((void ));
static void write_byte PROTO((byte c ));
static void write_str PROTO((char *s , word size ));
static void write_word PROTO((word w ));
static void write_long PROTO((long l ));
static void write_com PROTO((void ));
static void write_map PROTO((void ));
static void init PROTO((void ));
static void deinit PROTO((void ));
int main PROTO((int argc , char *argv []));

#undef PROTO

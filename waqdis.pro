#ifdef __PROTO__
# define	PROTO(s) s
#else
# define	PROTO(s) ()
#endif


/* waqdis.c */
static void erl_exit PROTO((int erl ));
static void dis_error PROTO((char *fmt , ...));
static void user_break PROTO((void ));
static void *myalloc PROTO((word nbytes ));
static byte read_byte PROTO((void ));
static void read_str PROTO((char *s , word size ));
static void read_word PROTO((word *w ));
static void read_long PROTO((long *l ));
static boolean read_com PROTO((void ));
static void show_info PROTO((void ));
static byte get_byte PROTO((void ));
static word get_word PROTO((void ));
static void disassemble PROTO((void ));
static void deinit PROTO((void ));
int main PROTO((int argc , char *argv []));

#undef PROTO

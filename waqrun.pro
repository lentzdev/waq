#ifdef __PROTO__
# define	PROTO(s) s
#else
# define	PROTO(s) ()
#endif


/* waqrun.c */
static void erl_exit PROTO((int erl ));
static void run_error PROTO((char *fmt , ...));
static void user_break PROTO((void ));
static void crit_on PROTO((void ));
static void crit_off PROTO((void ));
static void *myalloc PROTO((word nbytes ));
static void *mycalloc PROTO((word nbytes ));
static byte read_byte PROTO((void ));
static void read_str PROTO((char *s , word size ));
static void read_word PROTO((word *w ));
static void read_long PROTO((long *l ));
static void read_hdr PROTO((void ));
static void add_list PROTO((char *filespec ));
static void read_code PROTO((void ));
static short do_get PROTO((boolean getint , short a ));
static void write_str PROTO((char *s , word size ));
static void write_word PROTO((word w ));
static void write_long PROTO((long l ));
static short do_end PROTO((void ));
static short do_quit PROTO((void ));
static short do_enter PROTO((void ));
static short do_read PROTO((short a ));
static void push PROTO((word w ));
static word pop PROTO((void ));
static void do_run PROTO((void ));
static void waq_info PROTO((void ));
static void com_info PROTO((void ));
static void enter PROTO((void ));
static void sortscripts PROTO((void ));
static void do_menu PROTO((void ));
static boolean set_emu PROTO((char *parm ));
static void set_user PROTO((char *parm , boolean append ));
static boolean file_info PROTO((char *parm ));
int main PROTO((int argc , char *argv []));

#undef PROTO

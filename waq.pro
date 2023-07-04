#ifdef __PROTO__
# define	PROTO(s) s
#else
# define	PROTO(s) ()
#endif


/* ffind.c */
char *ffirst PROTO((char *filespec ));
char *fnext PROTO((void ));
char *ffirst PROTO((char *name ));
char *fnext PROTO((void ));
char *ffirst PROTO((char *filespec ));
char *fnext PROTO((void ));
void unique_name PROTO((char *pathname ));

/* stamp.c */
long datetostamp PROTO((char *s ));
char *stamptodate PROTO((long t ));

/* crc32.c */
long crc32block PROTO((register long crc , register byte *buf , register word len ));
long crc32byte PROTO((register long crc , register byte c ));
long crc32init PROTO((void ));
long crc32fini PROTO((long crc ));
boolean crc32test PROTO((long crc ));

/* misc.c */
char *strip PROTO((char *s ));
void splitpath PROTO((char *pathname , char *path , char *name ));
void mergepath PROTO((char *pathname , char *path , char *name ));
char *strlwr PROTO((char *s ));
int stricmp PROTO((const char *s1 , const char *s2 ));
boolean patmat PROTO((char *raw , char *pat ));

/* arc.c */
void arc_cfg PROTO((void ));
char *arc_list PROTO((boolean compress ));

/* com.c */
boolean com_init PROTO((int port ));
void com_deinit PROTO((void ));
boolean com_carrier PROTO((void ));
int com_getc PROTO((long timeout ));
void com_purge PROTO((void ));
void com_flush PROTO((void ));
int com_printf PROTO((char *fmt , ...));

#undef PROTO

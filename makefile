.RESPONSE_LINK: tlink link
MODEL	= l
CFLAGS	= -w+ -m$(MODEL) -O -G -Z -f- -d -N -H=WAQ.SYM
rem CFLAGS  = -w+ -m$(MODEL) -O -G -Z -f- -d -N -i8 -A -H=WAQ.SYM
# Word alignment -a
COPTS	= -DMSDOS -DANSIC
#COPTS	 = -DPARSE_DEBUG
LFLAGS	= /x/c/d
STARTUP = c0$(MODEL).obj
LIBS	= c$(MODEL).lib


WAQOBJS = waqcomp.obj waqdis.obj waqrun.obj
UTLSRCS = ffind.c stamp.c crc32.c misc.c arc.c com.c
UTLOBJS = ffind.obj stamp.obj crc32.obj misc.obj arc.obj com.obj
WAQCOMP = waqcomp.obj ffind.obj stamp.obj crc32.obj misc.obj arc.obj
WAQDIS	= waqdis.obj ffind.obj stamp.obj crc32.obj misc.obj
WAQRUN	= waqrun.obj ffind.obj stamp.obj crc32.obj misc.obj com.obj md5.obj


all:	waqcomp.exe waqdis.exe waqrun.exe


waqcomp.exe: $(WAQCOMP)
	     tlink $(LFLAGS) $(STARTUP) $(WAQCOMP),$*.exe,nul,$(LIBS)

waqdis.exe:  $(WAQDIS)
	     tlink $(LFLAGS) $(STARTUP) $(WAQDIS),$*.exe,nul,$(LIBS)

waqrun.exe:  $(WAQRUN)
	     tlink $(LFLAGS) $(STARTUP) $(WAQRUN),$*.exe,nul,$(LIBS)


$(WAQOBJS):  $*.c waq.h
	     mkproto -s $*.c >$*.pro
	     bcc -c $(CFLAGS) $(COPTS) $*.c

$(UTLOBJS):  $*.c includes.h
	     bcc -c $(CFLAGS) $(COPTS) $*.c

md5.obj:     $*.c md5.h
	     bcc -c $(CFLAGS) $(COPTS) $*.c

waq.pro:     $(UTLSRCS)
	     mkproto $(UTLSRCS) >$*.pro


waqrun.obj:  md5.h
com.obj:     waq.h


# end of WAQ makefile

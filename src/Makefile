# Makefile for building SDKL

# Your platform. See PLATS for possible values.
PLAT= guess

CC= gcc -std=gnu99
CFLAGS=  -O2 -DSDKL_COMPAT_5_3 $(SYSCFLAGS) $(MYCFLAGS)
LDFLAGS=  $(SYSLDFLAGS) $(MYLDFLAGS)
LIBS= $(SYSLIBS) $(MYLIBS) -lm -lreadline -ldl
AR= ar rcu
RANLIB= ranlib
RM= rm -f
UNAME= uname

SYSCFLAGS= -DSDKL_USE_EXTRAS
SYSLDFLAGS=
SYSLIBS=

MYCFLAGS=
MYLDFLAGS=
MYLIBS=
MYOBJS=

# Special flags for compiler modules; -Os reduces code size.
CMCFLAGS= 

# == END OF USER SETTINGS -- NO NEED TO CHANGE ANYTHING BELOW THIS LINE =======

PLATS= guess aix bsd c89 freebsd generic linux linux-readline macosx mingw posix solaris

SDKL_A=	libsdkl.a
CORE_O=	lapi.o lcode.o lctype.o ldebug.o ldo.o ldump.o lfunc.o lgc.o llex.o lmem.o lobject.o lopcodes.o lparser.o lstate.o lstring.o ltable.o ltm.o lundump.o lvm.o lzio.o
LIB_O=	sdklauxillary.o lbaselib.o lcorolib.o ldblib.o liolib.o lmathlib.o loadlib.o loslib.o lstrlib.o ltablib.o lutf8lib.o linit.o
BASE_O= $(CORE_O) $(LIB_O) $(MYOBJS)

SDKL_T=	sdkl
SDKL_O=	sdkl.o

SDKLC_T=	sdkl-bytes
SDKLC_O=	luac.o

ALL_O= $(BASE_O) $(SDKL_O) $(SDKLC_O)
ALL_T= $(SDKL_A) $(SDKL_T) $(SDKLC_T)
ALL_A= $(SDKL_A)

# Targets start here.
default: $(PLAT)

all:	$(ALL_T)

o:	$(ALL_O)

a:	$(ALL_A)

$(SDKL_A): $(BASE_O)
	$(AR) $@ $(BASE_O)
	$(RANLIB) $@

$(SDKL_T): $(SDKL_O) $(SDKL_A)
	$(CC) -o $@ $(LDFLAGS) $(SDKL_O) $(SDKL_A) $(LIBS)

$(SDKLC_T): $(SDKLC_O) $(SDKL_A)
	$(CC) -o $@ $(LDFLAGS) $(SDKLC_O) $(SDKL_A) $(LIBS)

test:
	./$(SDKL_T) -v

clean:
	$(RM) $(ALL_T) $(ALL_O)

depend:
	@$(CC) $(CFLAGS) -MM l*.c

echo:
	@echo "PLAT= $(PLAT)"
	@echo "CC= $(CC)"
	@echo "CFLAGS= $(CFLAGS)"
	@echo "LDFLAGS= $(SYSLDFLAGS)"
	@echo "LIBS= $(LIBS)"
	@echo "AR= $(AR)"
	@echo "RANLIB= $(RANLIB)"
	@echo "RM= $(RM)"
	@echo "UNAME= $(UNAME)"

# Convenience targets for popular platforms.
ALL= all

help:
	@echo "Do 'make PLATFORM' where PLATFORM is one of these:"
	@echo "   $(PLATS)"
	@echo "See doc/readme.html for complete instructions."

guess:
	@echo Guessing `$(UNAME)`
	@$(MAKE) `$(UNAME)`

AIX aix:
	$(MAKE) $(ALL) CC="xlc" CFLAGS="-O2 -DSDKL_USE_POSIX -DSDKL_USE_DLOPEN" SYSLIBS="-ldl" SYSLDFLAGS="-brtl -bexpall"

bsd:
	$(MAKE) $(ALL) SYSCFLAGS="-DSDKL_USE_POSIX -DSDKL_USE_DLOPEN" SYSLIBS="-Wl,-E"

c89:
	$(MAKE) $(ALL) SYSCFLAGS="-DSDKL_USE_C89" CC="gcc -std=c89"
	@echo ''
	@echo '*** C89 does not guarantee 64-bit integers for Lua.'
	@echo '*** Make sure to compile all external Lua libraries'
	@echo '*** with SDKL_USE_C89 to ensure consistency'
	@echo ''

FreeBSD NetBSD OpenBSD freebsd:
	$(MAKE) $(ALL) SYSCFLAGS="-DSDKL_USE_LINUX -DSDKL_USE_READLINE -I/usr/include/edit" SYSLIBS="-Wl,-E -ledit" CC="cc"

generic: $(ALL)

Linux linux:	linux-noreadline

linux-noreadline:
	$(MAKE) $(ALL) SYSCFLAGS="-DSDKL_USE_LINUX" SYSLIBS="-Wl,-E -ldl"

linux-readline:
	$(MAKE) $(ALL) SYSCFLAGS="-DSDKL_USE_LINUX -DSDKL_USE_READLINE" SYSLIBS="-Wl,-E -ldl -lreadline -lm"

Darwin macos macosx:
	$(MAKE) $(ALL) SYSCFLAGS="-DSDKL_USE_MACOSX -DSDKL_USE_READLINE" SYSLIBS="-lreadline"

mingw:
	$(MAKE) "SDKL_A=SDKL.dll" "SDKL_T=sdkl.exe" \
	"AR=$(CC) -shared -o" "RANLIB=strip --strip-unneeded" \
	"SYSCFLAGS=-DSDKL_BUILD_AS_DLL" "SYSLIBS=" "SYSLDFLAGS=-s" sdkl.exe
	$(MAKE) "SDKLC_T=sdkl-bytes.exe" sdkl-bytes.exe

posix:
	$(MAKE) $(ALL) SYSCFLAGS="-DSDKL_USE_POSIX"

SunOS solaris:
	$(MAKE) $(ALL) SYSCFLAGS="-DSDKL_USE_POSIX -DSDKL_USE_DLOPEN -D_REENTRANT" SYSLIBS="-ldl"

# Targets that do not create files (not all makes understand .PHONY).
.PHONY: all $(PLATS) help test clean default o a depend echo

# Compiler modules may use special flags.
llex.o:
	$(CC) $(CFLAGS) $(CMCFLAGS) -c llex.c

lparser.o:
	$(CC) $(CFLAGS) $(CMCFLAGS) -c lparser.c

lcode.o:
	$(CC) $(CFLAGS) $(CMCFLAGS) -c lcode.c

# DO NOT DELETE

lapi.o: lapi.c lprefix.h sdkl.h sdklconf.h lapi.h llimits.h lstate.h \
 lobject.h ltm.h lzio.h lmem.h ldebug.h ldo.h lfunc.h lgc.h lstring.h \
 ltable.h lundump.h lvm.h
lauxlib.o: sdklauxillary.c lprefix.h sdkl.h sdklconf.h sdklauxillary.h
lbaselib.o: lbaselib.c lprefix.h sdkl.h sdklconf.h sdklauxillary.h sdkllib.h
lcode.o: lcode.c lprefix.h sdkl.h sdklconf.h lcode.h llex.h lobject.h \
 llimits.h lzio.h lmem.h lopcodes.h lparser.h ldebug.h lstate.h ltm.h \
 ldo.h lgc.h lstring.h ltable.h lvm.h
lcorolib.o: lcorolib.c lprefix.h sdkl.h sdklconf.h sdklauxillary.h sdkllib.h
lctype.o: lctype.c lprefix.h lctype.h sdkl.h sdklconf.h llimits.h
ldblib.o: ldblib.c lprefix.h sdkl.h sdklconf.h sdklauxillary.h sdkllib.h
ldebug.o: ldebug.c lprefix.h sdkl.h sdklconf.h lapi.h llimits.h lstate.h \
 lobject.h ltm.h lzio.h lmem.h lcode.h llex.h lopcodes.h lparser.h \
 ldebug.h ldo.h lfunc.h lstring.h lgc.h ltable.h lvm.h
ldo.o: ldo.c lprefix.h sdkl.h sdklconf.h lapi.h llimits.h lstate.h \
 lobject.h ltm.h lzio.h lmem.h ldebug.h ldo.h lfunc.h lgc.h lopcodes.h \
 lparser.h lstring.h ltable.h lundump.h lvm.h
ldump.o: ldump.c lprefix.h sdkl.h sdklconf.h lobject.h llimits.h lstate.h \
 ltm.h lzio.h lmem.h lundump.h
lfunc.o: lfunc.c lprefix.h sdkl.h sdklconf.h ldebug.h lstate.h lobject.h \
 llimits.h ltm.h lzio.h lmem.h ldo.h lfunc.h lgc.h
lgc.o: lgc.c lprefix.h sdkl.h sdklconf.h ldebug.h lstate.h lobject.h \
 llimits.h ltm.h lzio.h lmem.h ldo.h lfunc.h lgc.h lstring.h ltable.h
linit.o: linit.c lprefix.h sdkl.h sdklconf.h sdkllib.h sdklauxillary.h
liolib.o: liolib.c lprefix.h sdkl.h sdklconf.h sdklauxillary.h sdkllib.h
llex.o: llex.c lprefix.h sdkl.h sdklconf.h lctype.h llimits.h ldebug.h \
 lstate.h lobject.h ltm.h lzio.h lmem.h ldo.h lgc.h llex.h lparser.h \
 lstring.h ltable.h
lmathlib.o: lmathlib.c lprefix.h sdkl.h sdklconf.h sdklauxillary.h sdkllib.h
lmem.o: lmem.c lprefix.h sdkl.h sdklconf.h ldebug.h lstate.h lobject.h \
 llimits.h ltm.h lzio.h lmem.h ldo.h lgc.h
loadlib.o: loadlib.c lprefix.h sdkl.h sdklconf.h sdklauxillary.h sdkllib.h
lobject.o: lobject.c lprefix.h sdkl.h sdklconf.h lctype.h llimits.h \
 ldebug.h lstate.h lobject.h ltm.h lzio.h lmem.h ldo.h lstring.h lgc.h \
 lvm.h
lopcodes.o: lopcodes.c lprefix.h lopcodes.h llimits.h sdkl.h sdklconf.h
loslib.o: loslib.c lprefix.h sdkl.h sdklconf.h sdklauxillary.h sdkllib.h
lparser.o: lparser.c lprefix.h sdkl.h sdklconf.h lcode.h llex.h lobject.h \
 llimits.h lzio.h lmem.h lopcodes.h lparser.h ldebug.h lstate.h ltm.h \
 ldo.h lfunc.h lstring.h lgc.h ltable.h
lstate.o: lstate.c lprefix.h sdkl.h sdklconf.h lapi.h llimits.h lstate.h \
 lobject.h ltm.h lzio.h lmem.h ldebug.h ldo.h lfunc.h lgc.h llex.h \
 lstring.h ltable.h
lstring.o: lstring.c lprefix.h sdkl.h sdklconf.h ldebug.h lstate.h \
 lobject.h llimits.h ltm.h lzio.h lmem.h ldo.h lstring.h lgc.h
lstrlib.o: lstrlib.c lprefix.h sdkl.h sdklconf.h sdklauxillary.h sdkllib.h
ltable.o: ltable.c lprefix.h sdkl.h sdklconf.h ldebug.h lstate.h lobject.h \
 llimits.h ltm.h lzio.h lmem.h ldo.h lgc.h lstring.h ltable.h lvm.h
ltablib.o: ltablib.c lprefix.h sdkl.h sdklconf.h sdklauxillary.h sdkllib.h
ltm.o: ltm.c lprefix.h sdkl.h sdklconf.h ldebug.h lstate.h lobject.h \
 llimits.h ltm.h lzio.h lmem.h ldo.h lgc.h lstring.h ltable.h lvm.h
lua.o: sdkl.c lprefix.h sdkl.h sdklconf.h sdklauxillary.h sdkllib.h
luac.o: luac.c lprefix.h sdkl.h sdklconf.h sdklauxillary.h ldebug.h lstate.h \
 lobject.h llimits.h ltm.h lzio.h lmem.h lopcodes.h lopnames.h lundump.h
lundump.o: lundump.c lprefix.h sdkl.h sdklconf.h ldebug.h lstate.h \
 lobject.h llimits.h ltm.h lzio.h lmem.h ldo.h lfunc.h lstring.h lgc.h \
 lundump.h
lutf8lib.o: lutf8lib.c lprefix.h sdkl.h sdklconf.h sdklauxillary.h sdkllib.h
lvm.o: lvm.c lprefix.h sdkl.h sdklconf.h ldebug.h lstate.h lobject.h \
 llimits.h ltm.h lzio.h lmem.h ldo.h lfunc.h lgc.h lopcodes.h lstring.h \
 ltable.h lvm.h ljumptab.h
lzio.o: lzio.c lprefix.h sdkl.h sdklconf.h llimits.h lmem.h lstate.h \
 lobject.h ltm.h lzio.h

# (end of Makefile)

# makefile for hcompress programs
# R. White, 20 April 1992
#
# Change INSTALLDIR to point to the directory where you want the programs
# installed.  (The default is your own bin directory.)  Then do "make install"
# to install the programs.  A simple "make" will compile and link the programs
# in this directory, but will not install them.
#
INSTALLDIR = ${TELHOME}/bin
#
CFLAGS = -O2
LIBABBR = hcomp
LIB = lib${LIBABBR}.a
LDFLAGS = -L. -l${LIBABBR} -lm

pass5:	install

install : hcomp hdecomp
	cp hcomp       ${INSTALLDIR}/hcomp
	cp hdecomp     ${INSTALLDIR}/hdecomp

all: hcomp hdecomp
	@echo "All programs compiled and linked."

clobber :
	-/bin/rm -f *.o hcomp hdecomp ${LIB}

# library

${LIB}: \
		${LIB}(bit_input.o)			\
		${LIB}(bit_output.o)		\
		${LIB}(decode.o)			\
		${LIB}(digitize.o)			\
		${LIB}(dodecode.o)			\
		${LIB}(doencode.o)			\
		${LIB}(encode.o)			\
		${LIB}(fitspass.o)			\
		${LIB}(fitsread.o)			\
		${LIB}(get_data.o)			\
		${LIB}(getopt.o)			\
		${LIB}(hinv.o)				\
		${LIB}(hsmooth.o)			\
		${LIB}(htrans.o)			\
		${LIB}(makefits.o)			\
		${LIB}(put_data.o)			\
		${LIB}(qread.o)				\
		${LIB}(qtree_decode.o)		\
		${LIB}(qtree_encode.o)		\
		${LIB}(qwrite.o)			\
		${LIB}(swap_bytes.o)		\
		${LIB}(undigitize.o)
	# ranlib ${LIB}
	@echo Library is up to date

.PRECIOUS: ${LIB}

# New rules for library entries

.c.a:
	${CC} -c ${CFLAGS} $<
	@ar rv ${LIB} $*.o
	@rm -f $*.o

#
# executables
#
hcomp : hcomp.o ${LIB}
	${CC} ${CFLAGS} hcomp.o -o hcomp ${LDFLAGS}
hdecomp : hdecomp.o ${LIB}
	${CC} ${CFLAGS} hdecomp.o -o hdecomp ${LDFLAGS}

# For RCS Only -- Do Not Edit
# @(#) $RCSfile: Makefile,v $ $Date: 2003/04/15 20:48:34 $ $Revision: 1.1.1.1 $ $Name:  $

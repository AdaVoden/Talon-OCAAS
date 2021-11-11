#-----------------------------------------------------------------------------
# GNU makefile for building WCSLIB.
#
# Summary of the main targets
# ---------------------------
#    all:       Build the library
#    clean:     Delete intermediate object files.
#    realclean: clean, and also delete executables and the object library.
#    test:      Compile and run the test programs.
#    show_all:  Print the values of all variables used.
#
# Notes:
#    1) The native Convex C compiler does not have the IEEE copysign()
#       function, and some versions of Ultrix have a version which returns
#       garbage.  In such cases a preprocessor macro replacement for
#       copysign() is available by defining the COPYSIGN preprocessor macro.
#
# $Id: Makefile,v 1.1.1.1 2003/04/15 20:46:59 lostairman Exp $
#-----------------------------------------------------------------------------
# C compiler and options.
#
#              CC       CFLAGS     LIBS
#             ---  --------------  ----
# GNU         gcc  -O              -lm
# SUN/ANSI    acc  -O
# SUN/K&R      cc  -O              -lm
# DEC          cc  -O  -DCOPYSIGN  -lm
# Convex       cc  -O3 -DCOPYSIGN

  CFLAGS  := -O

# Linker options.
  LDFLAGS := -s

# Extra required libraries.
  LIBS := -lm

# The remaining options are only required for compiling those test programs
# (tproj2 and tsph) which use PGPLOT to plot test grids.  You can circumvent
# this by setting PGPLOTLIB to blank.  PGPLOT is a FORTRAN plotting library
# available from astro.caltech.edu.  

# FORTRAN compiler (for linking test programs to PGPLOT).
  FC     := f77
  FFLAGS := -O

# Although written in C the plotting programs are linked to PGPLOT using the
# FORTRAN compiler since it passes the correct FORTRAN libraries to 'ld'.
# However, if you've compiled the C objects with gcc then the GNU C library
# must be added explicitly to resolve certain symbols specific to gcc.
  GCCLIB := -L/usr/local/gnu/lib/gcc-lib/sparc-sun-sunos4.1.3/2.6.0 -lgcc

# PGPLOT link list.
  PGPLOTLIB := -lpgplot -lX11 $(GCCLIB)

#-----------------------------------------------------------------------------
# You shouldn't need to change anything below here.

LIBRARY := libwcs_c.a
MODULES := proj.o \
           wcs.o  \
           sph.o  \
           wcstrig.o

ifneq "$(PGPLOTLIB)" ""
   TPROJ2 := tproj2
   TWCS   := twcs
endif

%.o : %.c
	-@ echo ""
	$(CC) $(CFLAGS) -c $<

%.o : %.f
	-@ echo ""
	$(FC) $(FFLAGS) -c $<

.PHONY : all clean lib realclean test

all : lib

lib : $(LIBRARY)

$(LIBRARY) : $(MODULES:%=$(LIBRARY)(%))
	ranlib $(LIBRARY)

clean :
	- $(RM) *.o *.i a.out core

realclean : clean
	- $(RM) tproj1 tproj2 tsph twcs $(LIBRARY)

test : tproj1 $(TPROJ2) tsph $(TWCS)
	-@ echo ""
	-@ echo "Running WCSLIB test programs:"
	-@ tproj1
        ifdef TPROJ2
	   -@ echo ""
	   -@ tproj2 < /dev/null
        endif
	-@ echo ""
	-@ tsph
        ifdef TWCS
	   -@ echo ""
	   -@ twcs
        endif

tproj1 tsph : % : %.c $(LIBRARY)
	-@ echo ""
	   $(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBRARY) $(LIBS)
	   $(RM) $@.o

tproj2 twcs : % : %.o tpgc.o tpgf.o $(LIBRARY)
	-@ echo ""
	   $(FC) $(LDFLAGS) -o $@ $< tpgc.o tpgf.o $(LIBRARY) $(PGPLOTLIB)

show_all :
	-@ echo "CC        := $(CC)"
	-@ echo "CFLAGS    := $(CFLAGS)"
	-@ echo "LDFLAGS   := $(LDFLAGS)"
	-@ echo "LIBS      := $(LIBS)"
	-@ echo "PGPLOTLIB := $(PGPLOTLIB)"
	-@ echo "LIBRARY   := $(LIBRARY)"
	-@ echo "MODULES   := $(MODULES)"
	-@ echo "TPROJ2    := $(TPROJ2)"
	-@ echo "TWCS      := $(TWCS)"

proj.o : proj.h
sph.o  : wcstrig.h
wcs.o  : wcs.h
wcstrig.o : wcstrig.h

tproj1 : proj.h
tproj2 : proj.h
tsph   : wcstrig.h
twcs   : wcs.h

proj.h : wcstrig.h
wcs.h  : proj.h


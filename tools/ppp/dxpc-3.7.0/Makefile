# Generated automatically from Makefile.in by configure.
CXX      = c++
CXXFLAGS = -O2  $(MOTIFI)  -DHAVE_SYS_SELECT_H=1 -DDXPC_ACCEPT_IS_SIZE_T=1 
LDFLAGS  =  $(MOTIFL)
LIBS     =  $(MOTIFLIBS) 

srcdir      = .
prefix      = /usr/local
exec_prefix = ${prefix}
bindir      = ${exec_prefix}/bin
man1dir     = ${prefix}/man/man1

INSTALL         = /usr/bin/install -c
INSTALL_PROGRAM = ${INSTALL}
INSTALL_DATA    = ${INSTALL} -m 644

.SUFFIXES:	.C

.C.o:
	$(CXX) -c $(CXXFLAGS) $<

TARGET=	dxpc

pass5:	install

install: dxpc
	rm -f $(TELHOME)/bin/dxpc
	cp dxpc $(TELHOME)/bin/dxpc

all: $(TARGET)

pure: pure_$(TARGET)
quantify: quant_$(TARGET)

SRCS=	main.C util.C constants.C Stats.C \
	Multiplexer.C ClientMultiplexer.C ServerMultiplexer.C \
	ClientChannel.C ServerChannel.C ReadBuffer.C \
	ProxyReadBuffer.C ClientReadBuffer.C ServerReadBuffer.C \
	EncodeBuffer.C DecodeBuffer.C WriteBuffer.C \
	IntCache.C CharCache.C PixelCache.C HuffmanCoder.C \
	ClientCache.C ServerCache.C \
	TextCompressor.C LastPixels.C SequenceNumQueue.C \
	BlockCache.C BlockCacheSet.C 

OBJS=	$(SRCS:.C=.o)

$(TARGET):	$(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

pure_$(TARGET):	$(OBJS)
	purify $(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

quant_$(TARGET):	$(OBJS) $(OBJS)
	quantify $(CXX) $(CXXFLAGS) -o $@ $(OBJS) $(LDFLAGS) $(LIBS)

depends:	
	makedepend -f Makefile $(SRCS)

install.bin:	$(TARGET)
	$(srcdir)/mkinstalldirs $(bindir)
	$(INSTALL) $(TARGET) $(bindir)/$(TARGET)

install.man:
	$(srcdir)/mkinstalldirs $(man1dir)
	$(INSTALL_DATA) $(TARGET).man $(man1dir)/$(TARGET).1

clean:
	-rm -f *~ *.o $(TARGET) pure_* quant_* *.bak core gmon.out

distclean: clean
	-rm -f config.status config.log config.cache Makefile tags

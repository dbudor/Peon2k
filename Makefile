-include config.mk

TARGETS     = peon2k.w2p peon2kdebug.w2p peon2kvision.w2p logger.w2p
LDFLAGS 	=  -Wl,--enable-stdcall-fixup -s -shared -static
NFLAGS       = -f elf -Iinc/
CC           =  mingw32-gcc
CFLAGS       = -Iinc -O2 -march=i486 -Wall -masm=intel -Wno-pointer-sign
CXXFLAGS     = -Iinc -O2 -march=i486 -Wall -masm=intel 
LIBS         = 

OBJS = peon2k.o place_check.o dump.o
        
NASM        ?= nasm

all: $(TARGETS)

.PHONY: default

%.o: %.asm
	$(NASM) $(NFLAGS) -o $@ $<

peon2k.w2p: peon2k.o place_check.o dump.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

peon2kdebug.w2p: peon2kdebug.o place_check.o dump.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

peon2kvision.w2p: peon2kvision.o place_check.o dump.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

logger.w2p: logger.o
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	$(RM) $(OBJS) $(TARGET) gen/*

peon2k.o: peon2k.cpp gen/ajmoo.h

peon2kdebug.o: peon2k.cpp gen/ajmoo.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DDEBUG -c $< -o $@

peon2kvision.o: peon2k.cpp gen/ajmoo.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DPATCH_VISION -c $< -o $@

logger.o: logger.cpp

dump.o: dump.cpp

gen/ajmoo.h: ajmoo.wav
	xxd -i $< > $@

defs.idc: defs.h
	echo "#include <idc.idc>" > $@
	echo "static main(void)" >> $@
	echo "{" >> $@
	cat $< | perl -ne 'if (/^#define (F_.*?) (0x.*?)\\s*(\/\/.*)?$$/) { printf("  MakeName(%s, \"%s\");\n", $$2, $$1); }' >> $@
	echo "}" >> $@

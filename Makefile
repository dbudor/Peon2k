-include config.mk

TARGET       = peon2k.w2p
LDFLAGS 	=  -Wl,--enable-stdcall-fixup -s -shared -static
NFLAGS       = -f elf -Iinc/
CC           =  mingw32-gcc
CFLAGS       = -Iinc -O2 -march=i486 -Wall -masm=intel -Wno-pointer-sign 
CXXFLAGS     = -Iinc -O2 -march=i486 -Wall -masm=intel
LIBS         = 

OBJS = peon2k.o place_check.o dump.o
        
NASM        ?= nasm

.PHONY: default

%.o: %.asm
	$(NASM) $(NFLAGS) -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	$(RM) $(OBJS) $(TARGET) gen/*

peon2k.o: peon2k.cpp gen/ajmoo.h

dump.o: dump.cpp

gen/ajmoo.h: ajmoo.wav
	xxd -i $< > $@

defs.idc: defs.h
	echo "#include <idc.idc>" > $@
	echo "static main(void)" >> $@
	echo "{" >> $@
	cat $< | perl -ne 'if (/^#define (F_.*?) (0x.*?)\\s*(\/\/.*)?$$/) { printf("  MakeName(%s, \"%s\");\n", $$2, $$1); }' >> $@
	echo "}" >> $@



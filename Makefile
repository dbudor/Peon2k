-include config.mk

TARGET       = peon2k.w2p
LDFLAGS 	=  -Wl,--enable-stdcall-fixup -s -shared -static
NFLAGS       = -f elf -Iinc/
CC           =  mingw32-gcc
CFLAGS       = -Iinc -O2 -march=i486 -Wall -masm=intel -Wno-pointer-sign 
CXXFLAGS     = -Iinc -O2 -march=i486 -Wall -masm=intel
LIBS         = 

OBJS = peon2k.o
        
NASM        ?= nasm

.PHONY: default

%.o: %.asm
	$(NASM) $(NFLAGS) -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LIBS)

clean:
	$(RM) $(OBJS) $(TARGET)

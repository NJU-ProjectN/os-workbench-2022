# Automatically links all .c files
SRCS = trampoline.S $(shell find . -name "*.c")
OBJS = $(addsuffix .o, $(basename $(SRCS)))

CROSS_COMPILE = x86_64-linux-gnu-

CFLAGS  = -O1 -m64 -fPIC -mno-sse -ggdb -Wall -Werror
CFLAGS += -ffreestanding -fno-builtin -nostdlib -fno-stack-protector
CFLAGS += -fno-asynchronous-unwind-tables

LDFLAGS  = -static --omagic --pic-executable --no-dynamic-linker
LDFLAGS += --gc-sections

_init: $(SRCS) ulib.h Makefile
	$(CROSS_COMPILE)gcc $(CFLAGS) -c $(SRCS)
	$(CROSS_COMPILE)ld $(LDFLAGS) -o $@ -e _start $(OBJS)
	$(CROSS_COMPILE)objcopy -S -j .text* -j.rodata* -j .data* -j .bss* --set-section-flags .bss=alloc,contents -O binary $@
	xxd -i $@ > ../kernel/src/initcode.inc

clean:
	rm -f *.o _*

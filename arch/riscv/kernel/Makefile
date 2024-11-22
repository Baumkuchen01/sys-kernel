SRC := $(wildcard *.c)
ASM_SRC := $(wildcard *.S)
OBJ := $(SRC:.c=.o) $(ASM_SRC:.S=.o)

.PHONY: all clean

all: $(OBJ)

clean:
	rm -f *.o

%.o: %.c
	$(GCC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -c $<

%.o: %.S
	$(GCC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -c $<

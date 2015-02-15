
CFLAGS=-Wall -O2

all: cpustats

cpustats:cpustats.o

clean:
	$(RM) cpustats cpustats.o 
SOURCE= main.c lwt.c lwt.h lwt_tramp.S
MYPROGRAM= lwt
MYINCLUDES= ./
CC= gcc
CFLAGS= -O3 -m32

all: $(MYPROGRAM)

$(MYPROGRAM): $(SOURCE)
	$(CC) $(CFLAGS) -I$(MYINCLUDES) $(SOURCE) -o $(MYPROGRAM)
clean:
	rm -f $(MYPROGRAM)

SOURCE= main.c lwt.c lwt.h lwt_tramp.S clist.c clist.h
MYPROGRAM= lwt
MYINCLUDES= ./
CC= gcc
CFLAGS= -ggdb3 -march=native

all: $(MYPROGRAM)

$(MYPROGRAM): $(SOURCE)
	$(CC) $(CFLAGS) -I$(MYINCLUDES) $(SOURCE) -o $(MYPROGRAM)
clean:
	rm -f $(MYPROGRAM)

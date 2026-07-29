CC		:= $(CROSS_COMPILE)gcc
INCLUDE := -I ../examples/systemcalls/ -I ./
CFLAGS	:= -g $(INCLUDE)

all: aesdsocket

aesdsocket: aesdsocket.o systemcalls.o main.o writerApp
	$(CC) main.o aesdsocket.o systemcalls.o -o aesdsocket

main.o: main.c
	$(CC) $(CFLAGS) -c main.c

aesdsocket.o: aesdsocket.c aesdsocket.h
	$(CC) $(CFLAGS) -c aesdsocket.c

systemcalls.o: ../examples/systemcalls/systemcalls.c
	$(CC) $(CFLAGS) -c ../examples/systemcalls/systemcalls.c

writerApp: writer
	install -m 744 writer $(HOME)/bin/writer

writer: writer.o
	$(CC) writer.o -o writer

writer.o: ../finder-app/writer.c
	$(CC) $(CFLAGS) -c ../finder-app/writer.c

clean: 
	rm -f *.o *.out aesdsocket writer $(HOME)/bin/writer
CC = gcc
CFLAGS = -c -Wall -ansi
LDFLAGS =
EXE = salvia_test.exe

test: jasmine.o test.o
	$(CC) $(LDFLAGS) salvia.o test.o -o $(EXE)

jasmine.o: ../salvia.c ../salvia.h
	$(CC) $(CFLAGS) ../salvia.c -o salvia.o

test.o: test.c ../salvia.h 
	$(CC) $(CFLAGS) test.c

.PHONY: clean
clean:
	rm *.o $(EXE)

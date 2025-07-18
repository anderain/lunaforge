CC = gcc
CFLAGS = -c -Wall
LDFLAGS =
EXE = khronicler.exe

all: k_utils.o kbasic.o kommon.o krt.o main.o
	$(CC) $(LDFLAGS) k_utils.o kbasic.o kommon.o krt.o main.o -o $(EXE)

k_utils.o: k_utils.c k_utils.h
	$(CC) $(CFLAGS) k_utils.c

kbasic.o: kbasic.c kbasic.h k_utils.h
	$(CC) $(CFLAGS) kbasic.c

kommon.o: kommon.c kommon.h
	$(CC) $(CFLAGS) kommon.c

krt.o: krt.c krt.h k_utils.h
	$(CC) $(CFLAGS) krt.c

main.o: main.c kbasic.h k_utils.h krt.h
	$(CC) $(CFLAGS) main.c

.PHONY: clean
clean:
	rm *.o $(EXE)
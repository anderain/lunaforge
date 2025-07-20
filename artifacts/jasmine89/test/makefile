CC = gcc
CFLAGS = -c -Wall -ansi
LDFLAGS =
SCHEMA_GENERATOR = node ../make_schema.js
EXE = jasmine_test.exe

test: jasmine_utils.o jasmine.o test.o
	$(CC) $(LDFLAGS) jasmine_utils.o jasmine.o test.o -o $(EXE)

person_schema.h: person_schema.json
	$(SCHEMA_GENERATOR) person_schema.json -o person_schema.h

jasmine_utils.o: ../jasmine_utils.c ../jasmine.h
	$(CC) $(CFLAGS) ../jasmine_utils.c -o jasmine_utils.o

jasmine.o: ../jasmine.c ../jasmine.h
	$(CC) $(CFLAGS) ../jasmine.c -o jasmine.o

test.o: test.c ../jasmine.h person_schema.h
	$(CC) $(CFLAGS) test.c

.PHONY: clean
clean:
	rm person_schema.h *.o $(EXE)
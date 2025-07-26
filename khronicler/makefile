CC          = gcc
C_FLAGS     = -c -Wall -ansi
LD_FLAGS 	=
CORE_OBJS   = k_utils.o kbasic.o kommon.o krt.o
MAIN_EXE	= khronicler.exe
TEST_EXE    = khronicler_test.exe

all: $(CORE_OBJS) main.o
	$(CC) $(LD_FLAGS) $(CORE_OBJS) main.o -o $(MAIN_EXE)

test: $(CORE_OBJS) test.o
	$(CC) $(LD_FLAGS) $(CORE_OBJS) test.o -o $(TEST_EXE)

k_utils.o: k_utils.c k_utils.h kommon.h
	$(CC) $(C_FLAGS) k_utils.c

kbasic.o: kbasic.c kbasic.h k_utils.h kommon.h
	$(CC) $(C_FLAGS) kbasic.c

kommon.o: kommon.c kommon.h
	$(CC) $(C_FLAGS) kommon.c

krt.o: krt.c krt.h k_utils.h kommon.h
	$(CC) $(C_FLAGS) krt.c

main.o: main.c kbasic.h k_utils.h krt.h kommon.h
	$(CC) $(C_FLAGS) main.c

test.o: test.c kbasic.h k_utils.h krt.h kommon.h
	$(CC) $(C_FLAGS) test.c

.PHONY: clean
clean:
	rm *.o *.kbn $(MAIN_EXE) $(TEST_EXE)
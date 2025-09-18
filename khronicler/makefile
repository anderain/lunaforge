#====================================================
# * Compiler Related Configuration
#----------------------------------------------------
# - CC			Compiler
# - C_FLAGS		Compilation flags
# - LDFLAGS		Linker flags
# - CORE_OBJS	Core object files
# - LIB_OBJS	Dependent library object files
# - MAIN_EXE	Main executable
# - TEST_EXE	Test executable
#====================================================
CC          = gcc
C_FLAGS     = -c -Wall -ansi
LD_FLAGS 	=
CORE_OBJS   = klexer.o kparser.o kompiler.o kutils.o kommon.o krt.c
MAIN_EXE	= kbasic.exe
TEST_EXE    = ktest.exe

#====================================================
# * Target: Main Program
#====================================================
all: $(CORE_OBJS) main.o test_as_utils.o
	$(CC) $(LD_FLAGS) $(CORE_OBJS) main.o test_as_utils.o -o $(MAIN_EXE)

#====================================================
# * Target: Test Program
#====================================================
test: $(CORE_OBJS) main.o test.o
	$(CC) $(LD_FLAGS) $(CORE_OBJS) test.o -o $(TEST_EXE)

#====================================================
# * Target: Core Files
#====================================================
kutils.o: kutils.c kommon.h kutils.h kalias.h
	$(CC) $(C_FLAGS) kutils.c

kparser.o: kparser.c klexer.h kommon.h kparser.h kutils.h kalias.h
	$(CC) $(C_FLAGS) kparser.c

kompiler.o: kompiler.c kompiler.h kparser.h klexer.h kommon.h kutils.h kalias.h
	$(CC) $(C_FLAGS) kompiler.c

kommon.o: kommon.c kommon.h
	$(CC) $(C_FLAGS) kommon.c

klexer.o: klexer.c klexer.h kommon.h kutils.h kalias.h
	$(CC) $(C_FLAGS) klexer.c

krt.o: krt.c krt.h kommon.h kutils.h kalias.h
	$(CC) $(C_FLAGS) krt.c

#====================================================
# * Target: Entry of main / test Program
#====================================================
main.o: main.c kbasic.h kalias.h klexer.h kparser.h kompiler.h kommon.h kutils.h krt.h
	$(CC) $(C_FLAGS) main.c

test_as_utils.o: test.c kbasic.h kalias.h klexer.h kparser.h kompiler.h kommon.h kutils.h krt.h
	$(CC) $(C_FLAGS) test.c -o test_as_utils.o

test.o: test.c kbasic.h kalias.h klexer.h kparser.h kompiler.h kommon.h kutils.h krt.h
	$(CC) $(C_FLAGS) -DIS_TEST_PROGRAM test.c

#====================================================
# * Clean
#====================================================
.PHONY: clean
clean:
	rm *.o *.kbn $(MAIN_EXE) $(TEST_EXE) test_report.html
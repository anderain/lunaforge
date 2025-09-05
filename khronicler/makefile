#====================================================
# * 目录配置
#----------------------------------------------------
# - SALVIA_FOLDER	Salvia 库的目录
#====================================================
SALVIA_FOLDER	= ../artifacts/salvia89

#====================================================
# * 编译器相关配置
#----------------------------------------------------
# - CC			编译器
# - C_FLAGS		编译参数
# - LDFLAGS		链接参数
# - CORE_OBJS	核心obj文件
# - LIB_OBJS	依赖库obj文件
# - MAIN_EXE	主程序
# - TEST_EXE	测试程序
#====================================================
CC          = gcc
C_FLAGS     = -c -Wall -ansi
LD_FLAGS 	=
CORE_OBJS   = k_utils.o kbasic.o kommon.o krt.o
LIB_OBJS	= salvia.o
MAIN_EXE	= khronicler.exe
TEST_EXE    = khronicler_test.exe

#====================================================
# * 目标：主程序
#====================================================
all: $(CORE_OBJS) $(LIB_OBJS) main.o
	$(CC) $(LD_FLAGS) $(CORE_OBJS) $(LIB_OBJS) main.o -o $(MAIN_EXE)

#====================================================
# * 目标：测试程序
#====================================================
test: $(CORE_OBJS) $(LIB_OBJS) test.o 
	$(CC) $(LD_FLAGS) $(CORE_OBJS) $(LIB_OBJS) test.o -o $(TEST_EXE)

#====================================================
# * 目标：核心文件
#====================================================
k_utils.o: k_utils.c k_utils.h kommon.h
	$(CC) $(C_FLAGS) k_utils.c

kbasic.o: kbasic.c kbasic.h k_utils.h kommon.h
	$(CC) $(C_FLAGS) kbasic.c

kommon.o: kommon.c kommon.h
	$(CC) $(C_FLAGS) kommon.c

krt.o: krt.c krt.h k_utils.h kommon.h
	$(CC) $(C_FLAGS) krt.c

#====================================================
# * 目标：主程序入口
#====================================================
main.o: main.c kbasic.h k_utils.h krt.h kommon.h
	$(CC) $(C_FLAGS) main.c

#====================================================
# * 目标：测试程序入口
#====================================================
test.o: test.c kbasic.h k_utils.h krt.h kommon.h
	$(CC) $(C_FLAGS) test.c

#====================================================
# * 目标：通用库
#====================================================
salvia.o: $(SALVIA_FOLDER)/salvia.c $(SALVIA_FOLDER)/salvia.h
	$(CC) $(C_FLAGS) $(SALVIA_FOLDER)/salvia.c -o salvia.o

#====================================================
# * 清理
#====================================================
.PHONY: clean
clean:
	rm *.o *.kbn $(MAIN_EXE) $(TEST_EXE)
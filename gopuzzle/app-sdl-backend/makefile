#====================================================
# * 目录配置
#----------------------------------------------------
# - COMMON_FOLDER 	平台无关代码的路径
# - SDL_FOLDER		SDL 头文件/静态库的路径
# - JASMINE_FOLDER	JSON 库源码路径
# - FONTS_FOLDER	Raster Fonts 路径
#====================================================
COMMON_FOLDER	= ..
SDL_FOLDER		= ../../../sdl12
JASMINE_FOLDER	= ../../artifacts/jasmine89

#====================================================
# * 编译器相关配置
#----------------------------------------------------
# - CC			编译器
# - CFLAGS		编译参数
# - LDFLAGS		链接参数
# - EXE			目标可执行程序
#====================================================
CC			= gcc
CFLAGS		= -c -Wall -I$(SDL_FOLDER)/include -ansi
LDFLAGS 	= -L$(SDL_FOLDER)/lib -lmingw32 -lSDLmain -lSDL # -mwindows
EXE			= gopuzzle.exe

#====================================================
# * 目标：可执行文件
#====================================================
all: main.o gopuzzle.o modi.o gongshu.o jasmine.o jasmine_utils.o
	$(CC) main.o gopuzzle.o modi.o gongshu.o jasmine.o jasmine_utils.o $(LDFLAGS) -o $(EXE)

#====================================================
# * 目标：平台无关代码
#====================================================
gopuzzle.o: $(COMMON_FOLDER)/gopuzzle.c $(COMMON_FOLDER)/gopuzzle.h $(COMMON_FOLDER)/modi.h $(COMMON_FOLDER)/gongshu.h
	$(CC) $(CFLAGS) $(COMMON_FOLDER)/gopuzzle.c -o gopuzzle.o

modi.o:	$(COMMON_FOLDER)/modi.c $(COMMON_FOLDER)/modi.h $(COMMON_FOLDER)/gongshu.h
	$(CC) $(CFLAGS) $(COMMON_FOLDER)/modi.c -o modi.o

#====================================================
# * 目标：平台相关封装与程序入口
#====================================================
main.o: main.c
	$(CC) $(CFLAGS) main.c

gongshu.o: gongshu.c $(COMMON_FOLDER)/gongshu.h
	$(CC) $(CFLAGS) gongshu.c

#====================================================
# * 目标：通用库
#====================================================
jasmine.o: $(JASMINE_FOLDER)/jasmine.c
	$(CC) $(CFLAGS) $(JASMINE_FOLDER)/jasmine.c -o jasmine.o

jasmine_utils.o: $(JASMINE_FOLDER)/jasmine_utils.c
	$(CC) $(CFLAGS) $(JASMINE_FOLDER)/jasmine_utils.c -o jasmine_utils.o

#====================================================
# * 清理
#====================================================
.PHONY: clean
clean:
	rm *.o $(EXE)
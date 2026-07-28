# BOOM Makefile - Terminal DOOM Clone
# Usage: make          (build)
#        make run      (build + run)
#        make clean    (remove build artifacts)

CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c99 -DNCURSES_WIDECHAR -ID:/msys64/ucrt64/include/ncursesw
LDFLAGS = -L D:/msys64/ucrt64/lib -lncursesw -lm

SRCS = boom_main.c boom_render.c boom_game.c boom_wad.c boom_audio.c
OBJS = $(SRCS:.c=.o)
TARGET = boom.exe

.PHONY: all build clean run

all: build

build: $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c boom.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: build
	./$(TARGET)

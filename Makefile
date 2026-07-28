# BOOM Makefile - Terminal DOOM Clone
# Usage: make          (build)
#        make run      (build + run)
#        make clean    (remove build artifacts)
#        make sounds   (download DOOM1.WAD and extract audio)
#        make wad      (save current level as .boomwad)

CC = gcc
CFLAGS = -O3 -Wall -Wextra -Werror -std=c99 -DNCURSES_WIDECHAR -ID:/msys64/ucrt64/include/ncursesw
LDFLAGS = -L D:/msys64/ucrt64/lib -lncursesw -lm

SRCS = boom_main.c boom_render.c boom_game.c boom_wad.c boom_audio.c
OBJS = $(SRCS:.c=.o)
TARGET = boom.exe

.PHONY: all build clean run sounds wad

all: build

build: $(OBJS)
	$(CC) $(OBJS) -o $(TARGET) $(LDFLAGS)

%.o: %.c boom.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

run: build
	./$(TARGET)

sounds:
	python download_doom_wad.py

wad: build
	./$(TARGET) --wad levels/custom.boomwad

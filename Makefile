# BOOM Makefile - Terminal DOOM Clone
# Usage: make          (build)
#        make run      (build + run)
#        make clean    (remove build artifacts)

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -std=c99
LDFLAGS ?= -lm

SRCS = boom_main.c boom_render.c boom_game.c boom_wad.c boom_audio.c
OBJS = $(SRCS:.c=.o)
TARGET = boom

# Auto-detect ncurses
NCURSES_INC := $(shell pkg-config --cflags ncursesw 2>NUL)
NCURSES_LIB := $(shell pkg-config --libs ncursesw 2>NUL)

ifeq ($(NCURSES_INC),)
  ifneq ($(wildcard D:/msys64/ucrt64/include/ncursesw/ncurses.h),)
    NCURSES_INC := -DNCURSES_WIDECHAR -ID:/msys64/ucrt64/include/ncursesw
  else ifneq ($(wildcard C:/msys64/ucrt64/include/ncursesw/ncurses.h),)
    NCURSES_INC := -DNCURSES_WIDECHAR -IC:/msys64/ucrt64/include/ncursesw
  endif
endif

ifeq ($(NCURSES_LIB),)
  NCURSES_LIB := -lncursesw
endif

ifeq ($(OS),Windows_NT)
  TARGET = boom.exe
else
  TARGET = boom
  NCURSES_INC += -DNCURSES_WIDECHAR
  NCURSES_LIB := -lncurses
endif

CFLAGS += $(NCURSES_INC)
LDFLAGS += $(NCURSES_LIB)

.PHONY: all build clean run

all: build

build: $(TARGET)
	@echo "Build complete: $(TARGET)"

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)

%.o: %.c boom.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	-del /f /q *.o boom.exe boom 2>NUL & echo Cleaned.

run: build
	./$(TARGET)

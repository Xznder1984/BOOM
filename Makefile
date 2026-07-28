# BOOM Makefile - Terminal DOOM Clone
# Usage: make build   (compile)
#        make run     (compile + run)
#        make clean   (remove build artifacts)

CC ?= gcc
CFLAGS ?= -O2 -Wall -Wextra -pedantic -std=c99
LDFLAGS ?= -lm

# Source files
SRCS = boom_main.c boom_render.c boom_game.c boom_wad.c boom_audio.c
OBJS = $(SRCS:.c=.o)
TARGET = boom

# Platform detection
UNAME_S := $(shell uname -s 2>/dev/null || echo Windows)
ifeq ($(UNAME_S),Linux)
    CFLAGS += -DNCURSES_WIDECHAR
    LDFLAGS += -lncurses
    TARGET = boom
endif
ifeq ($(UNAME_S),Darwin)
    CFLAGS += -DNCURSES_WIDECHAR
    LDFLAGS += -lncurses
    TARGET = boom
endif
ifneq (,$(findstring MINGW,$(UNAME_S)))
    NCURSES_INC := $(shell pkg-config --cflags ncursesw 2>/dev/null || echo "-DNCURSES_WIDECHAR -I$(shell cygpath -u $(MSYSTEM_PREFIX)/include/ncursesw 2>/dev/null || echo /ucrt64/include/ncursesw)")
    NCURSES_LIB := $(shell pkg-config --libs ncursesw 2>/dev/null || echo "-lncursesw")
    CFLAGS += $(NCURSES_INC)
    LDFLAGS += $(NCURSES_LIB)
    TARGET = boom.exe
endif
ifneq (,$(findstring MSYS,$(UNAME_S)))
    NCURSES_INC := $(shell pkg-config --cflags ncursesw 2>/dev/null || echo "-DNCURSES_WIDECHAR -I$(shell cygpath -u $(MSYSTEM_PREFIX)/include/ncursesw 2>/dev/null || echo /ucrt64/include/ncursesw)")
    NCURSES_LIB := $(shell pkg-config --libs ncursesw 2>/dev/null || echo "-lncursesw")
    CFLAGS += $(NCURSES_INC)
    LDFLAGS += $(NCURSES_LIB)
    TARGET = boom.exe
endif
ifeq ($(OS),Windows_NT)
    NCURSES_INC := $(shell pkg-config --cflags ncursesw 2>/dev/null || echo "-DNCURSES_WIDECHAR -Iucrt64/include/ncursesw")
    NCURSES_LIB := $(shell pkg-config --libs ncursesw 2>/dev/null || echo "-lncursesw")
    CFLAGS += $(NCURSES_INC)
    LDFLAGS += $(NCURSES_LIB)
    TARGET = boom.exe
endif

.PHONY: all build clean run install check-deps help

all: build

build: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $@ $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

%.o: %.c boom.h
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) boom boom.exe
	@echo "Cleaned."

run: build
	./$(TARGET)

install: $(TARGET)
	@echo "Installing $(TARGET) to /usr/local/bin..."
	cp $(TARGET) /usr/local/bin/ 2>/dev/null || \
		sudo cp $(TARGET) /usr/local/bin/ 2>/dev/null || \
		echo "Install failed - try: sudo make install"
	@echo "Done! Run with: boom"

check-deps:
	@echo "Checking dependencies..."
	@which gcc >/dev/null 2>&1 || which clang >/dev/null 2>&1 || \
		(echo "ERROR: No C compiler found. Install gcc or clang." && exit 1)
	@echo "C compiler: OK"
	@dpkg -l libncurses-dev >/dev/null 2>&1 || \
		brew list ncurses >/dev/null 2>&1 || \
		pkg-config --exists ncursesw 2>/dev/null || \
		(echo "ERROR: ncurses not found. Install libncurses-dev (Linux) or ncurses (macOS)." && exit 1)
	@echo "ncurses: OK"
	@echo "All dependencies satisfied!"

help:
	@echo "BOOM - Terminal DOOM Clone"
	@echo ""
	@echo "Targets:"
	@echo "  make build    Build BOOM"
	@echo "  make run      Build and run"
	@echo "  make clean    Remove build artifacts"
	@echo "  make install  Install to /usr/local/bin"
	@echo "  make check-deps  Verify build dependencies"
	@echo "  make help     Show this help"

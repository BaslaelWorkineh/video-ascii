CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -O2 -Iinclude
SRCDIR = src
INCDIR = include
BUILDDIR = build
TARGET = $(BUILDDIR)/video_ascii_player

SOURCES = $(SRCDIR)/video_sdl_main.c \
          $(SRCDIR)/video_sdl_player.c \
          $(SRCDIR)/video_processor.c \
          $(SRCDIR)/sdl_display.c \
          $(SRCDIR)/image_loader.c \
          $(SRCDIR)/image_processing.c \
          $(SRCDIR)/ascii_converter.c

OBJECTS = $(SOURCES:$(SRCDIR)/%.c=$(BUILDDIR)/%.o)

FFMPEG_FLAGS = $(shell pkg-config --cflags --libs libavformat libavcodec libswscale libavutil 2>/dev/null || echo "-lavformat -lavcodec -lswscale -lavutil")
SDL_FLAGS = $(shell pkg-config --cflags --libs sdl2 SDL2_ttf 2>/dev/null || echo "-lSDL2 -lSDL2_ttf")

all: $(TARGET)

$(TARGET): $(OBJECTS) | $(BUILDDIR)
	$(CC) $(CFLAGS) -o $@ $^ -lm $(FFMPEG_FLAGS) $(SDL_FLAGS)

$(BUILDDIR)/%.o: $(SRCDIR)/%.c | $(BUILDDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILDDIR):
	mkdir -p $(BUILDDIR)

clean:
	rm -rf $(BUILDDIR)

test: $(TARGET)
	$(TARGET) examples/tom.mp4

.PHONY: all clean test

CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra
SDL_CFLAGS := $(shell sdl2-config --cflags)
SDL_LIBS   := $(shell sdl2-config --libs)

TARGET  := wildguns

.PHONY: all clean appimage

all: $(TARGET)

$(TARGET): src/wildguns.c src/libretro.h
	$(CC) $(CFLAGS) -Isrc $(SDL_CFLAGS) src/wildguns.c -o $(TARGET) $(SDL_LIBS) -ldl -lm

appimage: $(TARGET)
	bash packaging/linux/build_appimage.sh

clean:
	rm -f $(TARGET)

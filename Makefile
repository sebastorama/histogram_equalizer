all:
	gcc bmp.c -g main.c -lm `sdl-config --cflags --libs` -lSDL_gfx -DGFX -o equalizer
all-turbo-mac:
	gcc bmp.c main.c -O3 -march=core2 -lm `sdl-config --cflags --libs` -lSDL_gfx -DGFX -o equalizer
test:
	./equalizer sample.bmp

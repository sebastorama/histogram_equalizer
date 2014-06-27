all:
	gcc bmp.c -g main.c -lm `sdl-config --cflags --libs` -lSDL_gfx -DGFX -o equalizer
test:
	./equalizer sample.bmp

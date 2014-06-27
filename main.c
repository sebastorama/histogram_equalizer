#define WIDTH 1280
#define HEIGHT 1000
#define DEPTH 32

#include "bmp.h"
#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <stdlib.h>
#include <stdbool.h>

void print_usage() {
	printf("Usage: equalizer <file>\n Where file is a windows bmp file");
}

void draw_image(BMP_pixel_matrix bmp, SDL_Surface * s, int x, int y) {
	int i,j;
	for(i = (int)bmp.height-1; i >= 0; i--) {
		for(j = 0; j < (int)bmp.width; j++) {
			pixelRGBA(s, x+j, y+ ((int)bmp.height-1-i),
					LE_TO_int(&bmp.pixels[i][j].red, 1),
					LE_TO_int(&bmp.pixels[i][j].green, 1),
					LE_TO_int(&bmp.pixels[i][j].blue, 1),
					0xFF);
		}
	}
	SDL_Flip(s);
}

void draw_histogram(SDL_Surface * s, unsigned long int * histogram, int depth, int scale, int x, int y, Uint32 color) {
	int i, j;
	unsigned long int * scaled_histogram = calloc(depth, sizeof(unsigned long int));
	unsigned long int max_hist = 0;

	for(i = 0; i < depth; i++)
		if(histogram[i] > max_hist) max_hist = histogram[i];

	for(i = 0; i < depth; i++) {
		scaled_histogram[i] = histogram[i] * scale / max_hist;
		vlineColor(s, x+i, y+scale, y+scale-scaled_histogram[i], color);
	}

	free(scaled_histogram);
	SDL_Flip(s);
}


void update_histogram(unsigned long int color, unsigned long int * histogram) {
	histogram[color]++;
}

void calculate_cdf(unsigned long int * histogram, unsigned long int * cdf, int depth) {
  int i;
  cdf[0] = histogram[0];

  for(i=1; i < depth; i++) {
    cdf[i] = cdf[i-1] + histogram[i];
  }
}

unsigned long int first_non_zero(unsigned long int * array, int size) {
  unsigned long int i = 0;

  while(array[i] == 0 && i < size) {
    i++;
  }

  return i;
}

void equalize_pixel(unsigned long int * pixel, unsigned long int * cdf, unsigned long int min, int depth, int size) {
  *pixel = ((cdf[*pixel]-cdf[min])*(depth-1))/(size-cdf[min]);
}

int main(int argc, char ** argv) {
	SDL_Surface *screen;

	if (!(screen = SDL_SetVideoMode(WIDTH, HEIGHT, DEPTH, SDL_HWSURFACE)))
	{
		SDL_Quit();
		return 1;
	}

	int i, j;

	unsigned long int * r_hist =
		calloc(256, sizeof(unsigned long int));
	unsigned long int * g_hist =
		calloc(256, sizeof(unsigned long int));
	unsigned long int * b_hist =
		calloc(256, sizeof(unsigned long int));

	unsigned long int * r_hist_eq =
		calloc(256, sizeof(unsigned long int));
	unsigned long int * g_hist_eq =
		calloc(256, sizeof(unsigned long int));
	unsigned long int * b_hist_eq =
		calloc(256, sizeof(unsigned long int));

	unsigned long int * cdf_r_hist =
		calloc(256, sizeof(unsigned long int));
	unsigned long int * cdf_g_hist =
		calloc(256, sizeof(unsigned long int));
	unsigned long int * cdf_b_hist =
		calloc(256, sizeof(unsigned long int));

	unsigned long int * cdf_r_hist_eq =
		calloc(256, sizeof(unsigned long int));
	unsigned long int * cdf_g_hist_eq =
		calloc(256, sizeof(unsigned long int));
	unsigned long int * cdf_b_hist_eq =
		calloc(256, sizeof(unsigned long int));

	unsigned long int r_scale, g_scale, b_scale;
	unsigned long int r_min, g_min, b_min;

	FILE * input_file;
	FILE * output_file;

	input_file = fopen(argv[1], "rb");

	if(argc < 2) {
		print_usage();
		exit(0);
	}


	bmp_header header = get_header(input_file);
	BMP_pixel_matrix bmp_matrix = get_pixels(input_file);

	draw_image(bmp_matrix, screen, 0, 0);

	for(i = 0; i < (int)bmp_matrix.height; i++) {
		for(j = 0; j < (int)bmp_matrix.width; j++) {
			update_histogram((unsigned long int)LE_TO_int(&bmp_matrix.pixels[i][j].red, 1), r_hist);
			update_histogram((unsigned long int)LE_TO_int(&bmp_matrix.pixels[i][j].green, 1), g_hist);
			update_histogram((unsigned long int)LE_TO_int(&bmp_matrix.pixels[i][j].blue, 1), b_hist);
		}
	}


  calculate_cdf(r_hist, cdf_r_hist, 256);
  calculate_cdf(g_hist, cdf_g_hist, 256);
  calculate_cdf(b_hist, cdf_b_hist, 256);

	draw_histogram(screen, r_hist, 256, 128, 0, bmp_matrix.height+((128+10)*0), 0xFF0000FF);
	draw_histogram(screen, g_hist, 256, 128, 0, bmp_matrix.height+((128+10)*1), 0x00FF00FF);
	draw_histogram(screen, b_hist, 256, 128, 0, bmp_matrix.height+((128+10)*2), 0x0000FFFF);

	draw_histogram(screen, cdf_r_hist, 256, 128, 0, bmp_matrix.height+((128+10)*0), 0xFFFFFF88);
	draw_histogram(screen, cdf_g_hist, 256, 128, 0, bmp_matrix.height+((128+10)*1), 0xFFFFFF88);
	draw_histogram(screen, cdf_b_hist, 256, 128, 0, bmp_matrix.height+((128+10)*2), 0xFFFFFF88);

  r_min = first_non_zero(r_hist, 256);
  g_min = first_non_zero(g_hist, 256);
  b_min = first_non_zero(b_hist, 256);

  printf("%lu %lu %lu/n", r_min, g_min, b_min);

	for(i = 0; i < (int)bmp_matrix.height; i++) {
		for(j = 0; j < (int)bmp_matrix.width; j++) {
      r_scale = (unsigned long int)LE_TO_int(&bmp_matrix.pixels[i][j].red, 1);
      g_scale = (unsigned long int)LE_TO_int(&bmp_matrix.pixels[i][j].green, 1);
      b_scale = (unsigned long int)LE_TO_int(&bmp_matrix.pixels[i][j].blue, 1);

      equalize_pixel(&r_scale, cdf_r_hist, r_min, 256, (int)bmp_matrix.height*bmp_matrix.width);
      equalize_pixel(&g_scale, cdf_g_hist, g_min, 256, (int)bmp_matrix.height*bmp_matrix.width);
      equalize_pixel(&b_scale, cdf_b_hist, b_min, 256, (int)bmp_matrix.height*bmp_matrix.width);

      bmp_matrix.pixels[i][j].red = r_scale;
      bmp_matrix.pixels[i][j].green = g_scale;
      bmp_matrix.pixels[i][j].blue = b_scale;

      update_histogram(r_scale, r_hist_eq);
      update_histogram(g_scale, g_hist_eq);
      update_histogram(b_scale, b_hist_eq);
    }
  }

  calculate_cdf(r_hist_eq, cdf_r_hist_eq, 256);
  calculate_cdf(g_hist_eq, cdf_g_hist_eq, 256);
  calculate_cdf(b_hist_eq, cdf_b_hist_eq, 256);

	draw_histogram(screen, r_hist_eq, 256, 128, bmp_matrix.width+10, bmp_matrix.height+((128+10)*0), 0xFF0000FF);
	draw_histogram(screen, g_hist_eq, 256, 128, bmp_matrix.width+10, bmp_matrix.height+((128+10)*1), 0x00FF00FF);
	draw_histogram(screen, b_hist_eq, 256, 128, bmp_matrix.width+10, bmp_matrix.height+((128+10)*2), 0x0000FFFF);

	draw_histogram(screen, cdf_r_hist_eq, 256, 128, bmp_matrix.width+10, bmp_matrix.height+((128+10)*0), 0xFFFFFF88);
	draw_histogram(screen, cdf_g_hist_eq, 256, 128, bmp_matrix.width+10, bmp_matrix.height+((128+10)*1), 0xFFFFFF88);
	draw_histogram(screen, cdf_b_hist_eq, 256, 128, bmp_matrix.width+10, bmp_matrix.height+((128+10)*2), 0xFFFFFF88);

	output_file = fopen("results.bmp", "w+b");
	write_header(header, output_file);
	write_pixels(header, bmp_matrix, output_file);

	draw_image(bmp_matrix, screen, bmp_matrix.width+20, 0);

	fclose(output_file);
	fclose(input_file);

	SDL_Event event;
	bool running = true;
	while(running) {
		while (SDL_PollEvent(&event) && event.key.keysym.sym == SDLK_q) {
			if (event.type == SDL_QUIT || event.type == SDL_KEYDOWN)
				running = false;
		}
	}
	return 0;
}

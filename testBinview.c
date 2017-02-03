#include <stdio.h>

int width = 400;
int bandHeight = 200;
int nBands = 10;

int main(int argc, char *argv[])
{
	FILE *file = fopen(argv[1], "wb");
	if(file == NULL) {
		return 1;
	}
	
	char *buff1 = malloc(width*bandHeight);
	char *buff2 = malloc(width*bandHeight);
	for(size_t i = 0; i < width*bandHeight; ++i) {
		buff1[i] = 0;
		buff2[i] = 255;
	}
	
	for(size_t i = 0; i < nBands; ++i) {
		fwrite(buff1, 1, width*bandHeight, file);
		fwrite(buff2, 1, width*bandHeight, file);
	}
	
	fclose(file);
	free(buff1);
	free(buff2);
	
	return 0;
}

/*
Chris Robertson
Parallel Programming Project 1
10-23-17
*/

#include<stdio.h>
#include<stdlib.h>
#include <algorithm>
#include <thread>
#include <cmath>

typedef struct {
	unsigned char red, green, blue;
} PPMPixel;

typedef struct {
	int x, y;
	PPMPixel *data;
} PPMImage;

#define RGB_COMPONENT_COLOR 255

static PPMImage *readPPM(const char *filename)
{
	char buff[16];
	PPMImage *img;
	FILE *fp;
	int c, rgb_comp_color;
	//open PPM file for reading
	//reading a binary file
	fopen_s(&fp, filename, "rb");
	if (!fp) {
		fprintf(stderr, "Unable to open file '%s'\n", filename);
		exit(1);
	}

	//read image format
	if (!fgets(buff, sizeof(buff), fp)) {
		perror(filename);
		exit(1);
	}

	//check the image format can be P3 or P6. P3:data is in ASCII format    P6: data is in byte format
	if (buff[0] != 'P' || buff[1] != '6') {
		fprintf(stderr, "Invalid image format (must be 'P6')\n");
		exit(1);
	}

	//alloc memory form image
	img = (PPMImage *)malloc(sizeof(PPMImage));
	if (!img) {
		fprintf(stderr, "Unable to allocate memory\n");
		exit(1);
	}

	//check for comments
	c = getc(fp);
	while (c == '#') {
		while (getc(fp) != '\n');
		c = getc(fp);
	}

	ungetc(c, fp);//last character read was out back
				  //read image size information
	if (fscanf_s(fp, "%d %d", &img->x, &img->y) != 2) {//if not reading widht and height of image means if there is no 2 values
		fprintf(stderr, "Invalid image size (error loading '%s')\n", filename);
		exit(1);
	}

	//read rgb component
	if (fscanf_s(fp, "%d", &rgb_comp_color) != 1) {
		fprintf(stderr, "Invalid rgb component (error loading '%s')\n", filename);
		exit(1);
	}

	//check rgb component depth
	if (rgb_comp_color != RGB_COMPONENT_COLOR) {
		fprintf(stderr, "'%s' does not have 8-bits components\n", filename);
		exit(1);
	}

	while (fgetc(fp) != '\n');
	//memory allocation for pixel data for all pixels
	img->data = (PPMPixel*)malloc(img->x * img->y * sizeof(PPMPixel));

	if (!img) {
		fprintf(stderr, "Unable to allocate memory\n");
		exit(1);
	}

	//read pixel data from file
	if (fread(img->data, 3 * img->x, img->y, fp) != img->y) { //3 channels, RGB
		fprintf(stderr, "Error loading image '%s'\n", filename);
		exit(1);
	}

	fclose(fp);
	return img;
}
void writePPM(const char *filename, PPMImage *img)
{
	FILE *fp;
	//open file for output
	//writing in binary format
	fopen_s(&fp, filename, "wb");
	if (!fp) {
		fprintf(stderr, "Unable to open file '%s'\n", filename);
		exit(1);
	}

	//write the header file
	//image format
	fprintf(fp, "P6\n");

	//image size
	fprintf(fp, "%d %d\n", img->x, img->y);

	// rgb component depth
	fprintf(fp, "%d\n", RGB_COMPONENT_COLOR);

	// pixel data
	fwrite(img->data, 3 * img->x, img->y, fp);
	fclose(fp);
}

void changeColorPPM(PPMImage *img)//Negating image
{
	int i;
	if (img)
	{
		img = readPPM("model.ppm");
		for (i = 0; i<img->x*img->y; i++) {
			img->data[i].red = RGB_COMPONENT_COLOR - img->data[i].red;
			img->data[i].green = RGB_COMPONENT_COLOR - img->data[i].green;
			img->data[i].blue = RGB_COMPONENT_COLOR - img->data[i].blue;
		}
		writePPM("invertedModel.ppm", img);
	}
}

void lightnessPPM(PPMImage *img)
{
	int i;
	if (img)
	{
		img = readPPM("model.ppm");
		for (i = 0; i < (img->x * img->y); i++)
		{
			unsigned char max = (img->data[i].red < img->data[i].green) ? img->data[i].green : img->data[i].red;
			max = (max < img->data[i].blue) ? img->data[i].blue : max;
			unsigned char min = (img->data[i].red < img->data[i].green) ? img->data[i].red : img->data[i].green;
			min = (min < img->data[i].blue) ? min : img->data[i].blue;
			unsigned char temp = (unsigned char)(((int)max + (int)min) / 2);
			img->data[i].red = img->data[i].green = img->data[i].blue = temp;
		}
		writePPM("lightnessModel.ppm", img);
	}
}

void averagePPM(PPMImage *img)
{
	int i;
	if (img)
	{
		img = readPPM("model.ppm");
		for (i = 0; i < (img->x * img->y); i++)
		{
			unsigned char avg = (unsigned char)(((int)img->data[i].red + (int)img->data[i].green + (int)img->data[i].blue) / 3);
			img->data[i].red = img->data[i].green = img->data[i].blue = avg;
		}
		writePPM("averageModel.ppm", img);
	}
}

void luminosityPPM(PPMImage *img)
{
	int i;
	if (img)
	{
		img = readPPM("model.ppm");
		for (i = 0; i < (img->x * img->y); i++)
		{
			unsigned char avg = (unsigned char)((0.21 * (double)img->data[i].red) + (0.72 * (double)img->data[i].green) + (0.07 * (double)img->data[i].blue));
			img->data[i].red = img->data[i].green = img->data[i].blue = avg;
		}
		writePPM("luminosityModel.ppm", img);
	}
}

void edge(PPMImage *img, PPMImage *src, int quad)
{ //Since all threads have their own quadrant, but are writing to and reading from the same full-size file, there is no reason for the threads to communicate
	//The reason there is no race case is because they are all operating on memory addresses with offsets, which is considered safe by guidelines I found on the internet
	int i,
		start;
	/*
	Quadrants are assigned as the following:
	|1|2|
	|3|4|
	*/
	if (quad == 1)
	{//Avoid edges, so starting at "[0,0]" for quadrant 1
		start = (src->x + 1);
	}
	else if (quad == 2)
	{//Starts halfway into the second line, and so on for the other quandrants
		start = (src->x * 1.5);
	}
	else if (quad == 3)
	{
		start = (src->x * (src->y * 0.5) + 1);
	}
	else if (quad == 4)
	{
		start = (src->x * (src->y * 0.5) + (src->x * 0.5));
	}

	i = start;

	for (int y = 0; y < ((src->y / 2) - 1); y++)
	{
		for (int x = 0; x < ((src->x / 2) - 1); x++)
		{
			/*
			In the 3x3 grid, the top left is accessed by [i + x - src->x -1]
			That is, "index" is always equal to the starting "x" index, and just added a whole row to go to the next line of the one-dimensional array
			x is the offset on the current line
			src->x is a whole line, basically moving up a row since it was subtracted
			-1 so we move over one index place and collect that data
			*/
			int xx = src->data[i + x - src->x - 1].red + (2 * src->data[i + x - src->x].red) + src->data[i + x - src->x + 1].red - src->data[i + x + src->x - 1].red - (2 * src->data[i + x + src->x].red) - src->data[i + x + src->x + 1].red;
			int yy = (-1 * src->data[i + x - src->x - 1].red) + src->data[i + x - src->x + 1].red - (2 * src->data[i + x - 1].red) + (2 * src->data[i + x + 1].red) - src->data[i + x + src->x - 1].red + src->data[i + x + src->x + 1].red;
			img->data[i + x].red = img->data[i + x].green = img->data[i + x].blue = (unsigned char)sqrt((xx * xx) + (yy * yy));
		}
		i += src->x;
	}
}

int main()
{
	PPMImage *invImage,
		*lightImage,
		*avgImage,
		*lumImage;

	std::thread t1(changeColorPPM, invImage);
	std::thread t2(lightnessPPM, lightImage);
	std::thread t3(averagePPM, avgImage);
	std::thread t4(luminosityPPM, lumImage);


	t1.join();
	t2.join();
	t3.join();
	t4.join();




	PPMImage *img,
		*src;
	img = readPPM("model.ppm");
	//Loading model.ppm into img ensures that all the metadata for the edge-detection file is correct, that way we don't have to re-write it.
	src = readPPM("luminosityModel.ppm");
	//This is the source for the edge file, which will not be edited in the threads.

	for (int i = 0; i < src->x - 1; i++) //three for-loops to fill in the borders of the image
	{
		img->data[i].red = img->data[i].green = img->data[i].blue = src->data[i].red;
	}

	for (int y = 1; y < src->y; y++)
	{
		img->data[src->x * y].red = img->data[src->x * y].green = img->data[src->x * y].blue = src->data[src->x * y].red;
		img->data[(src->x * y) + (src->x - 1)].red = img->data[(src->x * y) + (src->x - 1)].green = img->data[(src->x * y) + (src->x - 1)].blue = src->data[(src->x * y) + (src->x - 1)].red;
	}

	for (int i = 0; i < src->x - 1; i++)
	{
		img->data[i + (src->x * (src->y - 1))].red = img->data[i + (src->x * (src->y - 1))].green = img->data[i + (src->x * (src->y - 1))].blue = src->data[i + (src->x * (src->y - 1))].red;
	}

	std::thread th1(edge, img, src, 1);
	std::thread th2(edge, img, src, 2);
	std::thread th3(edge, img, src, 3);
	std::thread th4(edge, img, src, 4);

	th1.join();
	th2.join();
	th3.join();
	th4.join();

	writePPM("edgeModel.ppm", img);
	//I used luminosity for the edge detection file because it is usually the best, so I figured why not.
	//This can easily be modified to be performed on other algorithms if needed


	system("pause");
	return 0;
}
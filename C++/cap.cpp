#include <Xlib.h>
#include <Xutil.h>
#include <cstdint>
#include <cstring>
#include <vector>
#include <stdlib.h>

#include "ImgCvt.h"

void ImageFromDisplay(uint8_t **imgBuffer, int& Width, int& Height, int& BitsPerPixel)
{
	Display* display = XOpenDisplay(nullptr);
	Window root = DefaultRootWindow(display);

	XWindowAttributes attributes = { 0 };
	XGetWindowAttributes(display, root, &attributes);

	Width = attributes.width;
	Height = attributes.height;

	XImage* img = XGetImage(display, root, 0, 0, Width, Height, AllPlanes, ZPixmap);
	BitsPerPixel = img->bits_per_pixel;
	
	*imgBuffer = (uint8_t *)malloc(Width * Height * BitsPerPixel);

	memcpy(*imgBuffer, img->data, Width * Height * BitsPerPixel);

	XDestroyImage(img);
	XCloseDisplay(display);
}

int main() {
	uint8_t *rawBuffer;
	uint8_t *bmpBuffer;
	int width = 0;
	int height = 0;
	int bitsPerPixel;

	ImageFromDisplay(&rawBuffer, width, height, bitsPerPixel);
	bmpBuffer = (uint8_t *)malloc(width * height * 3);

	for (int i = 0; i < height; i++) {
		uint8_t *p1 = rawBuffer + (i * width * bitsPerPixel);
		uint8_t *p2 = bmpBuffer + (i * width * 3);
		for (int j = 0; j < width; j++) {
			p2[j * 3] = p1[j * 4];
			p2[j * 3 + 1] = p1[j * 4 + 1];
			p2[j * 3 + 2] = p1[j * 4 + 2];
		}
	}

	writeImage(bmpBuffer, width, height, "/mnt/screenShot.bmp");
	free(bmpBuffer);
	free(rawBuffer);
	return 0;
}
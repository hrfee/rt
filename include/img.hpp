#ifndef IMG
#define IMG

#include <cstdlib>
#include "vec.hpp"

// Images start from bottom-left.
struct Image {
    int w, h;
    Vec3 *img;
    std::uint8_t *rgbxImg;
};

Image *newImage(int w, int h);

void closeImage(Image *img);

void resizeImage(Image *img, int w, int h);

void writePixel(Image *img, int x, int y, Vec3 color);

void writePixel(Image *img, int offset, Vec3c color);

Vec3 getPixel(Image *img, int x, int y);

std::uint8_t *getPixelRGBx(Image *img, int x, int y);

void writeTestImage(Image *img);

void clearImage(Image *img);

#endif

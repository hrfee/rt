#ifndef IMG
#define IMG

#include <cstdlib>
#include "../vec/vec.hpp"

// Images start from bottom-left.
struct Image {
    int w, h;
    Vec3 *img;
    std::uint8_t *rgbxImg;
};

Image *newImage(int w, int h);

void writePixel(Image *img, int x, int y, Vec3 color);

Vec3 getPixel(Image *img, int x, int y);

void writeTestImage(Image *img);

void clear(Image *img);

#endif

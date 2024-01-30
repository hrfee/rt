#include "img.hpp"

#include <cstring>

Image *newImage(int w, int h) {
    Image *image = (Image*)malloc(sizeof(Image));
    image->w = w;
    image->h = h;
    image->img = (Vec3*)malloc(sizeof(Vec3)*w*h);
    image->rgbxImg = (std::uint8_t*)malloc(sizeof(std::uint8_t)*w*h*4);
    return image;
}

void writePixel(Image *img, int x, int y, Vec3 color) {
    img->img[(img->w * y) + x] = color;
    int idx = ((img->w * y) + x) * 4;
    img->rgbxImg[idx + 0] = std::uint8_t(color.x*255.f);
    img->rgbxImg[idx + 1] = std::uint8_t(color.y*255.f);
    img->rgbxImg[idx + 2] = std::uint8_t(color.z*255.f);
    img->rgbxImg[idx + 3] = 0;
}

Vec3 getPixel(Image *img, int x, int y) {
    return img->img[(img->w * y) + x];
}

std::uint8_t *getPixelRGBx(Image *img, int x, int y) {
    return img->rgbxImg + (4 * ((img->w * y) + x));
}

void writeTestImage(Image *img) {
    for (int y = 0; y < img->h; y++) {
        for (int x = 0; x < img->w; x++) {
            Vec3 c = { (float(y) / float(img->h)), (float(x) / float(img->w)), (float(x+y) / float(img->w + img->h)) };
            writePixel(img, x, y, c);
        }
    }
}

void clear(Image *img) {
    memset(img->img, 0, sizeof(Vec3)*img->w*img->h);
    memset(img->rgbxImg, 0, sizeof(std::uint8_t)*img->w*img->h*4);
}

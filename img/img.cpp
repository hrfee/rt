#include "img.hpp"

Image *newImage(int w, int h) {
    Image *image = (Image*)malloc(sizeof(Image));
    image->w = w;
    image->h = h;
    image->img = (Vec3*)malloc(sizeof(Vec3)*w*h);
    return image;
}

void writePixel(Image *img, int x, int y, Vec3 color) {
    img->img[(img->w * y) + x] = color;
}

Vec3 getPixel(Image *img, int x, int y) {
    return img->img[(img->w * y) + x];
}

void writeTestImage(Image *img) {
    for (int y = 0; y < img->h; y++) {
        for (int x = 0; x < img->w; x++) {
            Vec3 c = { (float(y) / float(img->h)), (float(x) / float(img->w)), (float(x+y) / float(img->w + img->h)) };
            writePixel(img, x, y, c);
        }
    }
}

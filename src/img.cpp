#include "img.hpp"

#include <cstring>
#include <cstdlib>
#include <algorithm>

Image::Image(int width, int height) {
    w = width;
    h = height;
    allocate();
    return;
}

Image::~Image() {
    free(img);
    free(rgbxImg);
}

void Image::resize(int width, int height) {
    std::printf("Resizing image to %dx%d\n", width, height);
    freeImage();
    w = width;
    h = height;
    allocate();
}

void Image::write(int x, int y, Vec3 color) {
    img[(w * y) + x] = color;
    int idx = ((w * y) + x) * 4;
    rgbxImg[idx + 0] = std::uint8_t(color.x*255.f);
    rgbxImg[idx + 1] = std::uint8_t(color.y*255.f);
    rgbxImg[idx + 2] = std::uint8_t(color.z*255.f);
    rgbxImg[idx + 3] = 255;
}

void Image::write(int offset, Vec3c color) {
    Vec3 cf = {float(255 & color.x)/255.f, float(255 & color.y)/255.f, float(255 & color.z)/255.f};
    if (cf.x < 0 || cf.x > 1 || cf.y < 0 || cf.y > 1 || cf.z < 0 || cf.z > 1) {
        std::printf("Warning: (%u,%u,%u) => (%f,%f,%f) is out of range\n", color.x, color.y, color.z, cf.x, cf.y, cf.z);
    }
    img[offset] = cf;
    int idx = offset * 4;
    rgbxImg[idx + 0] = color.x;
    rgbxImg[idx + 1] = color.y;
    rgbxImg[idx + 2] = color.z;
    rgbxImg[idx + 3] = 0;
}

Vec3 Image::get(int x, int y) {
    return img[(w * y) + x];
}

int wrap(int coord, int size) {
    int m = coord % size;
    return m >= 0 ? m : m + size;
}

// Bilinear interpolation
Vec3 Image::get(float x, float y) {
    int xs[2] = {wrap(int(x), w), wrap(int(x)+1, w)};
    int ys[2] = {wrap(int(y), h), wrap(int(y)+1, h)};
    float rx = x - float(int(x));
    float ry = y - float(int(y));

    // Top two points
    Vec3 r1 = (1.f - rx)*get(xs[0], ys[0]) + (rx)*get(xs[1], ys[0]);
    Vec3 r2 = (1.f - rx)*get(xs[0], ys[1]) + (rx)*get(xs[1], ys[1]);

    return (1.f - ry)*r1 + (ry)*r2;
}

std::uint8_t *Image::getRGBx(int x, int y) {
    return rgbxImg + (4 * ((w * y) + x));
}

void Image::writeTestImage() {
    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            Vec3 c = { (float(y) / float(h)), (float(x) / float(w)), (float(x+y) / float(w + h)) };
            write(x, y, c);
        }
    }
}

void Image::applyThinBorder(Vec3 color) {
    for (int i = 0; i < w; i++) {
        write(i, 0, color);
        write(i, h-1, color);
    }
    for (int i = 1; i < h-1; i++) {
        write(0, i, color);
        write(w-1, i, color);
    }
}

void Image::clear() {
    memset(img, 0, sizeof(Vec3)*w*h);
    memset(rgbxImg, 0, sizeof(std::uint8_t)*w*h*4);
}

#ifndef IMG
#define IMG

#include "vec.hpp"

// Images start from bottom-left.
struct Image {
    int w, h;
    Vec3 *img;
    std::uint8_t *rgbxImg;
    Image(int width, int height);
    ~Image();
    void resize(int width, int height);
    void write(int x, int y, Vec3 color);
    void write(int offset, Vec3c color);
    Vec3 get(int x, int y);
    std::uint8_t *getRGBx(int x, int y);
    void writeTestImage();
    void applyThinBorder(Vec3 color);
    void clear();
    private:
        void allocate() {
            img = (Vec3*)malloc(sizeof(Vec3)*w*h);
            rgbxImg = (std::uint8_t*)malloc(sizeof(std::uint8_t)*w*h*4);
        };
        void freeImage() {
            free(img);
            free(rgbxImg);
        };
};
#endif

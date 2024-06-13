#ifndef IMG
#define IMG

#include "vec.hpp"
#include <string>
#include <exception>

class ImgLoadException: public std::exception {
    private:
        std::string msg;
    public:
        ImgLoadException(char *fname) {
            msg = std::string("Failed to load image \"") + fname + std::string("\"");
        }
        ImgLoadException(std::string fname) {
            msg = std::string("Failed to load image \"") + fname + std::string("\"");
        }

        const char *what() const throw() {
            return msg.c_str();
        }
};

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
    Vec3 get(float x, float y); // Interpolates
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

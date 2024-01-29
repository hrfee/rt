#include "../img/img.hpp"
#include "../bmp/bmp.hpp"

int main(void) {
    Image *img = newImage(1920, 1080);
    writeTestImage(img);
   
    writeBMP(img, "/tmp/test.bmp");
}

#include "../map/map.hpp"
#include "../img/img.hpp"
#include "../bmp/bmp.hpp"
#include "../cam/cam.hpp"
#include <math.h>
#include <cstdio>

int main(void) {
    Image *img = newImage(1920, 1080);
    writeTestImage(img);
   
    writeBMP(img, "/tmp/test.bmp");

    Camera cam = newCamera(1920, 1080, 60.f*M_PI / 180.f, {0.f, 0.f, 0.f});
    cam.theta = 45.f * M_PI / 180.f;
    cam.phi = 45.f * M_PI / 180.f;
    calculateViewport(&cam);
    debugPrintCorners(&cam);
}

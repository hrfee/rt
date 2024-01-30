#include "../map/map.hpp"
#include "../img/img.hpp"
#include "../bmp/bmp.hpp"
#include "../cam/cam.hpp"
#include <math.h>
#include <cstdio>

int main(void) {
    WorldMap map = {50.f, 50.f, 50.f};
    map.cam = newCamera(1920, 1080, 60.f*M_PI / 180.f, {0.f, 0.f, 0.f});
    map.cam.theta = 0.f * M_PI / 180.f;
    map.cam.phi = 0.f * M_PI / 180.f;
    calculateViewport(&(map.cam));
    debugPrintCorners(&(map.cam));

    // A yellow sphere
    appendSphere(&map, {9.f, 0.f, 1.f}, 0.3f, {0.8f, 0.82f, 0.2f}); 
    

    Image *img = newImage(1920, 1080);
    clear(img);
    castRays(&map, img);
    // writeTestImage(img);
   
    writeBMP(img, "/tmp/test.bmp");

}

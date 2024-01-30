#include "../map/map.hpp"
#include "../img/img.hpp"
#include "../bmp/bmp.hpp"
#include "../cam/cam.hpp"
#include <math.h>
#include <cstdio>

#define TEST_W 7000
#define TEST_H 4000
#define TEST_FOV 60.f

int main(void) {
    WorldMap map = {50.f, 50.f, 50.f};
    map.cam = newCamera(TEST_W, TEST_H, TEST_FOV*M_PI / 180.f, {0.f, 0.f, 0.f});
    map.cam.theta = 0.f * M_PI / 180.f;
    map.cam.phi = 0.f * M_PI / 180.f;
    calculateViewport(&(map.cam));
    debugPrintCorners(&(map.cam));

    // A yellow sphere
    appendSphere(&map, {5.f, 0.f, 1.f}, 0.3f, {0.8f, 0.82f, 0.2f}, 0.4f); 
    appendSphere(&map, {5.2f, 0.f, -0.2f}, 0.4f, {0.25f, 0.82f, 0.82f}, 0.5f); 
    appendSphere(&map, {2.f, -0.8f, -1.4f}, 0.4f, {0.82f, 0.25f, 0.82f}, 0.1f); 
    

    Image *img = newImage(TEST_W, TEST_H);
    clear(img);
    castRays(&map, img);
    // writeTestImage(img);
   
    writeBMP(img, "/tmp/test.bmp");

}

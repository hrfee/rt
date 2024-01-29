#ifndef BMP
#define BMP

#include <string>
#include <cstdint>
#include <fstream>
#include "../img/img.hpp"

struct BMPHeader {
    char bmpSignature[2] = {'B', 'M'};
    uint32_t size;
    uint32_t reserved;
    uint32_t pixelOffset;
};

struct BMPInfoHeader {
    uint32_t headerSize = 40;
    int32_t w;
    int32_t h;
    uint16_t colorPlanes = 1;
    uint16_t colorDepth = 24; // (8 bits per channel, BGR)
    uint32_t compression = 0; // None
    uint32_t rawBitmapSize = 0; // Unneeded for now
    int32_t horizontalRes = 9999; // Doesn't matter, we aren't printing
    int32_t verticalRes = 9999;
    uint32_t colorTableEntries = 0;
    uint32_t importantColors = 0;
};

void writeBMP(Image *img, std::string fname);
#endif

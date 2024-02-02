#include "tga.hpp"

#include <cstdio>
#include <cstdint>
#include <fstream>

struct TGAHeader {
    char idLength; // Length of ID string, human-readable identifier.
    char colorMapType; // Not used, so we'll set to zero
    // Lowest three bits provide flags (4th is for run-length encoding).
    // We'll use 2, for uncompressed image with no color mapping.
    char imageType;
    // char cmapSpec[5]; // 5 bytes of nothing since we don't need it.
    char imageSpec[10]; // Dimensions & format info:
    // [0, 1]: X coordinate of the bottom-left corner
    // [2, 3]: Y coordinate
    // [4, 5]: Width
    // [6, 7]: Height
    // [8]: Bits per pixel (24 for us)
    // [9]: leftmost 4 bits are alpha channel depth, 5th is nothing, 6th is origin (0 = lower left-hand corner, 1 = upper left-hand corner), last 2 are for interleaving, which we don't do.
};

void encodeDescriptor(TGAHeader *header, int w, int h) {
    header->imageSpec[4] = 255 & w; // Lower bits of the width
    header->imageSpec[5] = 255 & (w >> 8); // Higher bits
    header->imageSpec[6] = 255 & h; // Lower bits of the height
    header->imageSpec[7] = 255 & (h>> 8); // Higher bits
    header->imageSpec[8] = 24; // bits per pixel
    header->imageSpec[9] = 0b00000000; // FIXME: check the 6th bit?
}

void writeTGA(Image *img, std::string fname) {
    TGAHeader header;
    header.idLength = 0; // FIXME: Put render info in the ID.
    header.colorMapType = 0;
    header.imageType = 0b00000010; // 2
    encodeDescriptor(&header, img->w, img->h);

    std::ofstream f(fname, std::ios_base::binary|std::ios_base::trunc);
    f.write(&(header.idLength), 1);
    f.write(&(header.colorMapType), 1);
    f.write(&(header.imageType), 1);
    f.seekp(5, std::ios_base::cur); // Skip of cmapSpec space
    f.write(&(header.imageSpec[0]), 10);
    int pxCount = img->w * img->h;
    for (int i = 0; i < pxCount; i++) {
        // BGR, not RGB!
        Vec3c px = { char(img->rgbxImg[(i*4)+2]), char(img->rgbxImg[(i*4)+1]), char(img->rgbxImg[(i*4)+0]) };
        f.write((char*)(&px), 3);
    }

    f.close();
}

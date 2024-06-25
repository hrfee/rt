#include "tga.hpp"
#include "img.hpp"

#include <iostream>
#include <cstdio>
#include <fstream>
#include <chrono>

// Rough implementation of TGA v1.0
// Source used for header format: https://en.wikipedia.org/wiki/Truevision_TGA
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
    // Only thing that matters is the 6th bit being zero.
    header->imageSpec[9] = 0b00000000; 
}

void TGA::write(Image *img, std::string fname, std::string id) {
    TGAHeader header;
    header.idLength = id.length();
    header.colorMapType = 0;
    header.imageType = 0b00000010; // 2
    encodeDescriptor(&header, img->w, img->h);

    std::ofstream f(fname, std::ios_base::binary|std::ios_base::trunc);
    // Write header
    f.write(&(header.idLength), 1);
    f.write(&(header.colorMapType), 1);
    f.write(&(header.imageType), 1);
    f.seekp(5, std::ios_base::cur); // Skip of cmapSpec space
    f.write(&(header.imageSpec[0]), 10);
    // Write ID
    f.write(id.c_str(), header.idLength);
    // Write Image
    int pxCount = img->w * img->h;
    for (int i = 0; i < pxCount; i++) {
        // BGR, not RGB!
        Vec3c px = { char(img->rgbxImg[(i*4)+2]), char(img->rgbxImg[(i*4)+1]), char(img->rgbxImg[(i*4)+0]) };
        f.write((char*)(&px), 3);
    }

    f.close();
}

Image *TGA::read(std::string fname) {
    auto start = std::chrono::high_resolution_clock::now();
    std::ifstream f(fname, std::ios_base::binary);
    if (f.fail() || f.bad()) {
        throw ImgLoadException(fname);
    }
    TGAHeader header;
    f.read(&(header.idLength), 1);
    char *id = (char*)malloc(sizeof(char)*header.idLength);
    f.read(&(header.colorMapType), 1);
    f.read(&(header.imageType), 1);
    f.seekg(5, std::ios_base::cur); // Skip of cmapSpec space
    f.read(&(header.imageSpec[0]), 10);
    f.read(id, header.idLength);

    uint w = (uint(header.imageSpec[5]) << 8) + (255 & uint(header.imageSpec[4]));
    uint h = (uint(header.imageSpec[7]) << 8) + (255 & uint(header.imageSpec[6]));
    /*std::printf("got comment: \"%s\"\n", id);
    std::printf("Got width and height %dx%d\n", w, h);
    std::printf("Got bitdepth %d\n", header.imageSpec[8]);*/
    int bitDepth = header.imageSpec[8];
    int bpc = 3;
    if (bitDepth % 8 != 0) std::printf("WARNING: Bitdepth weird: %d\n", bitDepth);
    
    // Naively, assume we only need 3 bytes of colour for rgb, and that anything further is junk
    int bytesToIgnore = (header.imageSpec[8] - 3*8)/8;

    // Also, naively deal with mono images
    if (bitDepth == 8) {
        bytesToIgnore = 0;
        bpc = 1;
    }

    Image *img = new Image(w, h);

    int pxCount = w*h;
    Vec3c pxBGR = {0,0,0};
    
    // Hopefully bytesToIgnore is less than this!
    char ignore[8];
    
    for (int i = 0; i < pxCount; i++) {
        // BGR, not RGB!
        f.read((char*)(&pxBGR), bpc);
        Vec3c px = {pxBGR.z, pxBGR.y, pxBGR.x};
        // For mono images, copy to all channels.
        if (bpc == 1) {
            px.x = px.z;
            px.y = px.z;
        }
        img->write(i, px);
        if (bytesToIgnore > 0) {
            // Seekg is really slow, Reading a 4096^2 32-bit image takes 13s with it, while
            // reading a 8192x4096 image withoout takes just 700ms.
            // f.seekg(bytesToIgnore, std::ios_base::cur);
            // Instead, read and do nothing with it.
            f.read(ignore, bytesToIgnore);
        }
    }

    f.close();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> ms = end - start;
    std::printf("Reading \"%s\" took %fms\n", fname.c_str(), ms.count());
    return img;
}

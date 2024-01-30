#include "bmp.hpp"

#include <cstdio>

void writeHeader(BMPHeader h, std::ofstream& f) {
    f.write(h.bmpSignature, 2);
    f.write((char*)(&h.size), sizeof(uint32_t));
    f.write((char*)(&h.reserved), sizeof(uint32_t));
    f.write((char*)(&h.pixelOffset), sizeof(uint32_t));
}

void writeInfoHeader(BMPInfoHeader h, std::ofstream& f) {
    f.write((char*)(&h.headerSize), sizeof(uint32_t)); 
    f.write((char*)(&h.w), sizeof(int32_t)); 
    f.write((char*)(&h.h), sizeof(int32_t)); 
    f.write((char*)(&h.colorPlanes), sizeof(uint16_t)); 
    f.write((char*)(&h.colorDepth), sizeof(uint16_t)); 
    f.write((char*)(&h.compression), sizeof(uint32_t)); 
    f.write((char*)(&h.rawBitmapSize), sizeof(uint32_t)); 
    f.write((char*)(&h.horizontalRes), sizeof(int32_t)); 
    f.write((char*)(&h.verticalRes), sizeof(int32_t)); 
    f.write((char*)(&h.colorTableEntries), sizeof(uint32_t)); 
    f.write((char*)(&h.importantColors), sizeof(uint32_t)); 
}

void writeBMP(Image *img, std::string fname) {
    BMPHeader header;
    // 54 is our (standard) header size
    header.size = 54 + (img->w*img->h*3);
    header.reserved = 0;
    header.pixelOffset = 54;

    BMPInfoHeader iHeader;
    iHeader.w = img->w;
    iHeader.h = img->h;
    // Other fields are either pre-set or don't matter.

    std::ofstream f(fname, std::ios_base::binary);

    writeHeader(header, f);
    writeInfoHeader(iHeader, f);

    int pxCount = img->w * img->h;
    for (int i = 0; i < pxCount; i++) {
        // BGR, not RGB!
        Vec3c px = { char(img->img[i].z*255.f), char(img->img[i].y*255.f), char(img->img[i].x*255.f) };
        f.write((char*)(&px), 3);
    }

    f.close();
}

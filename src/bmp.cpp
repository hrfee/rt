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

// FIXME: My BMP implementation still can't handle odd width images.
// Time to move to a simpler image format!
void writeBMP(Image *img, std::string fname) {
    BMPHeader header;
    // 54 is our (standard) header size
    header.size = 54 + (img->w*img->h*3);
    header.reserved = 0;
    header.pixelOffset = 54;

    BMPInfoHeader iHeader;
    iHeader.w = img->w;
    iHeader.h = img->h;
    // BMP requires that rows start at an address which is a multiple of 4 bytes, meaning our 24byte image might need padding on the ends of rows.
    int padding = 0;
    if (iHeader.w % 4 != 0) {
        padding = 4 - (iHeader.w % 4);
    }
    int pxCount = img->w * img->h;
    iHeader.rawBitmapSize = pxCount + (iHeader.h * padding);
    std::printf("dim %d %d pad %d\n", iHeader.w, iHeader.h, padding);
    // Other fields are either pre-set or don't matter.

    std::ofstream f(fname, std::ios_base::binary|std::ios_base::trunc);

    writeHeader(header, f);
    writeInfoHeader(iHeader, f);

    for (int i = 0; i < pxCount; i++) {
        // BGR, not RGB!
        Vec3c px = { char(img->rgbxImg[(i*4)+2]), char(img->rgbxImg[(i*4)+1]), char(img->rgbxImg[(i*4)+0]) };
        f.write((char*)(&px), 3);
        f.seekp(padding, std::ios_base::cur);
    }

    f.close();
}

#include <getopt.h>
#include <cstdio>
#include "tga.hpp"

int main(int argc, char **argv) {
    if (argc < 3) {
        std::fprintf(stderr, "Usage: %s <inputfile> <outputfile>\nTests TGA impl by reading to an internal Image, then outputting back to a TGA.\n", argv[0]);
        return 1;
    }
    Image *img = TGA::read(argv[1]);
    TGA::write(img, argv[2], "success!");
    delete img;
    std::printf("(hopefully) written to \"%s\".\n", argv[2]);
    return 0;
}


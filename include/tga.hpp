#ifndef RENDER_TGA
#define RENDER_TGA

#include <string>
#include "img.hpp"

void writeTGA(Image *img, std::string fname, std::string id);
Image *readTGA(std::string fname);
#endif

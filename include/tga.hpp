#ifndef RENDER_TGA
#define RENDER_TGA

#include <string>
#include "img.hpp"

namespace TGA {
    void write(Image *img, std::string fname, std::string id);
    Image *read(std::string fname);
};
#endif

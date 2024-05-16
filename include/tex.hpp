#ifndef TEX
#define TEX

#include "vec.hpp"
#include "img.hpp"
#include "tga.hpp"
#include <string>
#include <vector>

struct Texture {
    Image *img;
    Vec3 at(float u, float v);
    Texture(Image *image): img(image) {};
    Texture(std::string fname): img(TGA::read(fname)) {};
    ~Texture() { delete img; };
};

struct TexStore {
    std::vector<std::string> fnames;
    std::vector<Texture*> texes;
    Texture* at(int id);
    Texture* from(std::string fname);
    int id(std::string fname);
    int load(std::string fname);
    void clear();
};

#endif

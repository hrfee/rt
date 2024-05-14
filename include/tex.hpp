#ifndef TEX
#define TEX

#include "vec.hpp"
#include "img.hpp"
#include <string>
#include <vector>

struct Texture {
    Image *img;
    Vec3 at(float u, float v);
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

Texture *newTexture(Image *img);

Texture *newTexture(std::string fname);

void closeTexture(Texture *tex);

#endif

#include "tex.hpp"

#include <cstring>
#include <string>
#include <cstdlib>
#include "tga.hpp"

Texture *newTexture(Image *img) {
    Texture *tex = (Texture*)malloc(sizeof(Texture));
    tex->img = img;

    return tex;
}

Texture *newTexture(std::string fname) {
    Image *img = readTGA(fname);
    if (img == NULL) return NULL;
    return newTexture(img);
}

void closeTexture(Texture *tex) {
    free(tex->img);
    free(tex);
}

Vec3 Texture::at(float u, float v) {
    int x = std::clamp(int(u*float(img->w)), 0, img->w-1);
    int y = std::clamp(img->h-int(v*float(img->h)), 0, img->h-1);
    Vec3 c = img->get(x, y);
    return c;
}

int TexStore::id(std::string fname) {
    for (int i = 0; i < int(fnames.size()); i++) {
        if (fnames.at(i) == fname) return i;
    }
    return -1;
}

int TexStore::load(std::string fname) {
    for (int i = 0; i < int(fnames.size()); i++) {
        if (fnames.at(i) == fname) return i;
    }
    int i = fnames.size();
    Texture *tex = newTexture(fname);
    if (tex == NULL) return -1;
    texes.emplace_back(tex);
    fnames.emplace_back(fname);
    return i;
}

Texture *TexStore::from(std::string fname) {
    for (int i = 0; i < int(fnames.size()); i++) {
        if (fnames.at(i) == fname) return texes.at(i);
    }
    return NULL;
}

Texture *TexStore::at(int id) {
    return texes.at(id);
}

void TexStore::clear() {
    for (auto t: texes) {
        closeTexture(t);
    }
    texes.clear();
    fnames.clear();
}

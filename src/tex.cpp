#include "tex.hpp"

#include <cstring>
#include <string>
#include <cstdlib>
#include <algorithm>

Vec3 Texture::at(float u, float v) {
    float x = std::clamp(u*float(img->w), 0.f, float(img->w-1));
    float y = std::clamp(float(img->h)-(v*float(img->h)), 0.f, float(img->h-1));
    // return img->get(int(x), int(y));
    return img->get(x, y);
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
    Texture *tex = new Texture(fname);
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
        delete t;
    }
    texes.clear();
    fnames.clear();
}

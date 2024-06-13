#include "tex.hpp"
#include "img.hpp"
#include "tga.hpp"

#include <cstring>
#include <string>
#include <cstdlib>
// #include <algorithm>
#include <sstream>
#include <iostream>

Vec3 Texture::at(float u, float v) {
    /* float x = std::clamp(u*float(img->w), 0.f, float(img->w-1));
    float y = std::clamp(float(img->h)-(v*float(img->h)), 0.f, float(img->h-1));
    x *= scale.x;
    y *= scale.y; */

    u = u - int(u);
    if (u < 0) u += 1;
    v = v - int(v);
    if (v < 0) v += 1;

    float x = u * float(img->w);
    float y = float(img->h) - (v * float(img->h));
    // return img->get(int(x), int(y));
    return img->get(x, y);
}

int TexStore::id(std::string fname) {
    for (int i = 0; i < int(fnames.size()); i++) {
        if (fnames.at(i) == fname) return i;
    }
    return -1;
}

// Format: fname:<xscale>,<yscale>
// each <scale> indicates how many times the image should repeat on each of its axes.
// e.g. fname:2,2 would be equivalent to loading a 2x2 tiled version of the texture.
// setting a <scale> to "a" tells us to calculate the scale by preserving the image's aspect ratio.
int TexStore::load(std::string fname) {
    lastLoadFail = false;
    for (int i = 0; i < int(fnames.size()); i++) {
        if (fnames.at(i) == fname) return i;
    }
    int i = fnames.size();
    Texture *tex = NULL;
    try {
        tex = new Texture(fname);
    } catch (ImgLoadException &e) {
        lastLoadFail = true;
        std::cout << e.what() << std::endl;
        return -1;
    }
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

std::string texScaleFromFname(std::string in,  float *x, float *y) {
    *x = 1.f;
    *y = 1.f;
    std::stringstream s(in);
    std::string f;
    std::string scaleX;
    std::string scaleY;
    if (!std::getline(s, f, ':')) return f;
    std::getline(s, scaleX, ',');
    std::getline(s, scaleY);
    if (scaleX.length() != 0) {
        if (scaleX != "a") {
            *x = std::stof(scaleX);
        } else {
            *x = -1.f;
        }
    }
    if (scaleY.length() != 0) {
        if (scaleY != "a") {
            *y = std::stof(scaleY);
        } else {
            *y = -1.f;
        }
    }
    return f;
}

Texture::Texture(Image *image) {
    img = image;
    scale = {1.f, 1.f};
}

Texture::Texture(std::string fname) {
    auto fpath = texScaleFromFname(fname, &(scale(0)), &(scale(1))); 
    try {
        img = TGA::read(fpath);
    } catch (ImgLoadException &e) {
        throw;
    }
}

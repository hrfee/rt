#ifndef TEX
#define TEX

#include "vec.hpp"
#include "img.hpp"
#include <string>
#include <vector>

struct Texture {
    Image *img;
    Vec2 scale;
    Vec3 at(float u, float v);
    Texture(Image *image);
    Texture(std::string fname);
    ~Texture() { delete img; };
};

class TexStore {
    public:
        TexStore(): lastLoadFail(false), names(NULL) {};
        std::vector<std::string> fnames;
        std::vector<Texture*> texes;
        Texture* at(int id);
        Texture* from(std::string fname);
        int id(std::string fname);
        int load(std::string fname);
        void clear();
        // Stinky way to return load success/failure
        bool lastLoadFail;
        
        Texture** ptrs() { return texes.data(); };
        void genList();
        char **names;
};

#endif

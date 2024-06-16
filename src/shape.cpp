#include "shape.hpp"
#include "util.hpp"

#include <iostream>
#include <sstream>
#include <cstring>

namespace {
    const char* w_center = "center";
    const char* w_radius = "radius";
    const char* w_thickness = "thickness";
    const char* w_color = "color";
    const char* w_tex = "tex";
    const char* w_norm = "norm";
    const char w_color_hex = '#';
    const char* w_color_rgb_start = "rgb(";
    const char w_color_rgb_end = ')';
    const char* w_nolighting = "nolighting";
    const char* w_opacity = "opacity";
    const char* w_reflectiveness = "reflectiveness";
    const char* w_specular = "specular";
    // const char* w_emission = "emission";
    const char* w_brightness = "brightness";
    const char* w_shininess = "shininess";
    const char* w_a = "a";
    const char* w_b = "b";
    const char* w_c = "c";
    const char* w_plane = "plane";
    const char* w_campreset = "campreset";
    const char* w_pos = "pos";
    const char* w_phi = "phi";
    const char* w_theta = "theta";
    const char* w_fov = "fov";
}

std::string encodeCamPreset(CamPreset *p) {
    std::ostringstream fmt;
    fmt << w_campreset << " ";
    fmt << p->name << " ";
    fmt << w_pos << " ";
    fmt << p->pos.x << " " << p->pos.y << " " << p->pos.z << " ";
    fmt << w_phi << " ";
    fmt << p->phi << " ";
    fmt << w_theta << " ";
    fmt << p->theta << " ";
    fmt << w_fov << " ";
    fmt << p->fov << "";
    // fmt << std::endl;
    return fmt.str();
}

CamPreset decodeCamPreset(std::string in) {
    std::stringstream stream(in);
    CamPreset p = CamPreset();
    do {
        std::string w;
        stream >> w;
        if (w == w_campreset) {
            stream >> w;
            p.name = w;
        } else if (w == w_pos) {
            stream >> w;
            p.pos.x = std::stof(w);
            stream >> w;
            p.pos.y = std::stof(w);
            stream >> w;
            p.pos.z = std::stof(w);
        } else if (w == w_phi) {
            stream >> w;
            p.phi = std::stof(w);
        } else if (w == w_theta) {
            stream >> w;
            p.theta = std::stof(w);
        } else if (w == w_fov) {
            stream >> w;
            p.fov = std::stof(w);
        }
    } while (stream);
    return p;
}

Shape *emptyShape() {
    Shape *sh = (Shape*)alloc(sizeof(Shape));
    std::memset(sh, 0, sizeof(Shape));
    sh->shininess = -1.f; // -1 Indicates global shininess param takes precedence
    sh->opacity = 1.f;
    sh->texId = -1; // No texture
    sh->normId = -1; // No normalmap
    return sh;
}

std::string encodeShape(Shape *sh) {
    std::ostringstream fmt;
    fmt << encodeColour(sh->color) << " ";
    fmt << w_opacity << " ";
    fmt << sh->opacity << " ";
    fmt << w_reflectiveness << " ";
    fmt << sh->reflectiveness << " ";
    fmt << w_specular << " ";
    fmt << sh->specular << " ";
    fmt << w_shininess << " ";
    fmt << sh->shininess << " ";
    fmt << std::endl;
    return fmt.str();
}

Shape *decodeShape(std::string in, TexStore *tex, TexStore *norm) {
    if (tex != NULL) tex->lastLoadFail = false;
    if (norm != NULL) norm->lastLoadFail = false;
    std::stringstream stream(in);
    Shape *sh = emptyShape();
    do {
        std::string w;
        stream >> w;
        if (w == w_color) {
            sh->color = decodeColour(&stream);
        } else if (w == w_opacity) {
            stream >> w;
            sh->opacity = std::stof(w);
        } else if (w == w_reflectiveness) {
            stream >> w;
            sh->reflectiveness = std::stof(w);
        } else if (w == w_specular) {
            stream >> w;
            sh->specular = std::stof(w);
        } else if (w == w_shininess) {
            stream >> w;
            sh->shininess = std::stof(w);
        } else if (w == w_nolighting) {
            sh->noLighting = true;
        } else if (w == w_tex) {
            // FIXME: Cope with spaces in filenames
            // FIXME: "Eval" pathnames so they match, even if written differently
            stream >> w;
            if (tex != NULL) {
                sh->texId = tex->load(w);
            }
        } else if (w == w_norm) {
            // FIXME: Cope with spaces in filenames
            // FIXME: "Eval" pathnames so they match, even if written differently
            stream >> w;
            if (norm != NULL) {
                sh->normId = norm->load(w);
            }
        }
    } while (stream);
    return sh;
}

std::string encodeSphere(Shape *sh) {
    std::ostringstream fmt;
    fmt << "sphere ";
    fmt << w_center << " ";
    fmt << sh->s->center.x << " " << sh->s->center.y << " " << sh->s->center.z << " ";
    fmt << w_radius << " ";
    fmt << sh->s->radius << " ";
    fmt << w_thickness << " ";
    fmt << sh->s->thickness << " ";
    fmt << encodeShape(sh) << " ";
    fmt << std::endl;
    return fmt.str();
}

Shape *decodeSphere(std::string in, TexStore *tex, TexStore *norm) {
    std::stringstream stream(in);
    Shape *sh = decodeShape(in, tex, norm);
    sh->s = (Sphere*)alloc(sizeof(Sphere));
    sh->s->thickness = 1.f;
    do {
        std::string w;
        stream >> w;
        if (w == w_center) {
            stream >> w;
            sh->s->center.x = std::stof(w);
            stream >> w;
            sh->s->center.y = std::stof(w);
            stream >> w;
            sh->s->center.z = std::stof(w);
        } else if (w == w_radius) {
            stream >> w;
            sh->s->radius = std::stof(w);
        } else if (w == w_thickness) {
            stream >> w;
            sh->s->thickness = std::stof(w);
        }
    } while (stream);
    return sh;
}

std::string encodePointLight(PointLight *p) {
    std::ostringstream fmt;
    fmt << "plight ";
    fmt << w_center << " ";
    fmt << p->center.x << " " << p->center.y << " " << p->center.z << " ";
    fmt << encodeColour(p->color) << " ";
    fmt << w_brightness << " ";
    fmt << p->brightness << " ";
    fmt << w_specular << " ";
    fmt << p->specular << " ";
    fmt << encodeColour(p->specularColor) << " ";
    fmt << std::endl;
    return fmt.str();
}

PointLight decodePointLight(std::string in) {
    std::stringstream stream(in);
    PointLight p;
    p.specular = -1.f;
    do {
        std::string w;
        stream >> w;
        if (w == w_center) {
            stream >> w;
            p.center.x = std::stof(w);
            stream >> w;
            p.center.y = std::stof(w);
            stream >> w;
            p.center.z = std::stof(w);
        } else if (w == w_color) {
            p.color = decodeColour(&stream);
        } else if (w == w_brightness) {
            stream >> w;
            p.brightness = std::stof(w);
        } else if (w == w_specular) {
            stream >> w;
            p.specular = std::stof(w);
            stream >> w;
            if (w == w_color) {
                p.specularColor = decodeColour(&stream);
            } else {
                p.specularColor = p.color;
                stream << " " << w;
            }
        }
    } while (stream);
    if (p.specular == -1.f) p.specular = p.brightness;
    return p;
}

std::string encodeTriangle(Shape *sh) {
    std::ostringstream fmt;
    fmt << "triangle ";
    fmt << w_a << " ";
    fmt << sh->t->a.x << " " << sh->t->a.y << " " << sh->t->a.z << " ";
    fmt << w_b << " ";
    fmt << sh->t->b.x << " " << sh->t->b.y << " " << sh->t->b.z << " ";
    fmt << w_c << " ";
    fmt << sh->t->c.x << " " << sh->t->c.y << " " << sh->t->c.z << " ";
    fmt << encodeShape(sh) << " ";
    fmt << std::endl;
    return fmt.str();
}

Shape *decodeTriangle(std::string in, TexStore *tex, TexStore *norm) {
    std::stringstream stream(in);
    Shape *sh = decodeShape(in, tex, norm);
    sh->t = (Triangle*)alloc(sizeof(Triangle));
    sh->t->plane = false;

    do {
        std::string w;
        stream >> w;
        if (w == w_a) {
            stream >> w;
            sh->t->a.x = std::stof(w);
            stream >> w;
            sh->t->a.y = std::stof(w);
            stream >> w;
            sh->t->a.z = std::stof(w);
        } else if (w == w_b) {
            stream >> w;
            sh->t->b.x = std::stof(w);
            stream >> w;
            sh->t->b.y = std::stof(w);
            stream >> w;
            sh->t->b.z = std::stof(w);
        } else if (w == w_c) {
            stream >> w;
            sh->t->c.x = std::stof(w);
            stream >> w;
            sh->t->c.y = std::stof(w);
            stream >> w;
            sh->t->c.z = std::stof(w);
        } else if (w == w_plane) {
            sh->t->plane = true;
        }
    } while (stream);

    if (sh->texId != -1 || sh->normId != -1) {
        recalculateTriUVs(sh, tex, norm);
    }

    return sh;
}

void recalculateTriUVs(Shape *sh, TexStore *tex, TexStore *norm) {
    // Project onto 2D plane, any'll do for now
    float proj[3][2] = {
        {sh->t->a.x, sh->t->a.y},
        {sh->t->b.x, sh->t->b.y},
        {sh->t->c.x, sh->t->c.y}
    };
    float min[2] = {9999.f, 9999.f};
    float max[2] = {-9999.f, -9999.f};
    for (int i = 0; i < 2; i++) {
        min[i] = std::min(proj[0][i], std::min(proj[1][i], proj[2][i]));
        max[i] = std::max(proj[0][i], std::max(proj[1][i], proj[2][i]));
    }
    
    float range[2] = {max[0] - min[0], max[1] - min[1]};
    // FIXME: Only the scale factors of the TEXTURE are currently respected, and applied to the normal too!
    Texture *t = NULL;
    Vec2 scale = {1.f, 1.f};
    float iw = 0.f, ih = 0.f;
    if (sh->texId != -1) {
        t = tex->at(sh->texId);
    } else if (sh->normId) {
        t = norm->at(sh->normId);
    }
    scale = t->scale;
    iw = t->img->w;
    ih = t->img->h;

    if (scale.x == -1.f) {
        float H = range[1] / scale.y;
        float W = (H / ih) * iw;
        scale.x = range[0] / W;
    } else if (scale.y == -1.f) {
        float W = range[0] / scale.x;
        float H = (W / iw) * ih;
        scale.y = range[1] / H;
    }

    for (int i = 0; i < 3; i++) {
        sh->t->UVs[i] = {(proj[i][0] - min[0])*scale.x/range[0], (proj[i][1] - min[1])*scale.y/range[1]};
    }
}

std::string encodeAAB(Shape *sh) {
    std::ostringstream fmt;
    fmt << "box ";
    fmt << w_a << " ";
    fmt << sh->t->a.x << " " << sh->t->a.y << " " << sh->t->a.z << " ";
    fmt << w_b << " ";
    fmt << sh->t->b.x << " " << sh->t->b.y << " " << sh->t->b.z << " ";
    fmt << encodeShape(sh) << " ";
    fmt << std::endl;
    return fmt.str();
}

Shape *decodeAAB(std::string in, TexStore *tex, TexStore *norm) {
    std::stringstream stream(in);
    Shape *sh = decodeShape(in, tex, norm);
    sh->b = (AAB*)alloc(sizeof(AAB));

    do {
        std::string w;
        stream >> w;
        if (w == w_a) {
            stream >> w;
            sh->b->min.x = std::stof(w);
            stream >> w;
            sh->b->min.y = std::stof(w);
            stream >> w;
            sh->b->min.z = std::stof(w);
        } else if (w == w_b) {
            stream >> w;
            sh->b->max.x = std::stof(w);
            stream >> w;
            sh->b->max.y = std::stof(w);
            stream >> w;
            sh->b->max.z = std::stof(w);
        }
    } while (stream);

    if (sh->texId != -1 || sh->normId != -1) {
        // FIXME: AABB textures!
    }

    return sh;
}

std::string encodeColour(Vec3 c) {
    std::ostringstream fmt;
    fmt << w_color << " ";
    fmt << c.x << " " << c.y << " " << c.z;
    return fmt.str();
}

Vec3 decodeColour(std::stringstream *stream) {
    Vec3 c;
    std::string w;
    *stream >> w;
    int colorType = 0;
    if (w.at(0) == w_color_hex) {
        colorType = 1;
    } else if (w.at(0) == w_color_rgb_start[0] && w.substr(0, 4) == w_color_rgb_start) {
        colorType = 2;
    }
    switch (colorType) {
        case 1: {
            int r, g, b;
            std::sscanf(w.c_str(), "#%02x%02x%02x", &r, &g, &b);
            c.x = float(r)/255.f;
            c.y = float(g)/255.f;
            c.z = float(b)/255.f;
        }; break;
        case 2: {
            std::stringstream nextWords;
            // add back the "rgb(???"
            nextWords << w << " ";
            int pos;
            // take words until we reach the closing bracket.
            // Longest possible string will be 4 words ("r,", "g,", "b", and ")")
            for (int i = 0; i < 4; i++) {
                std::string nw;
                *stream >> nw;
                nextWords << nw << " ";
                pos = nextWords.str().find(w_color_rgb_end);
                if (pos != -1) break;
            }

            if (pos != -1) {
                std::string cvalues = nextWords.str().substr(4, pos-4);
                // Whitespace in scanf format strings mean (0 or more whitespace),
                // so any way of writing the colour should work (i.e. "123 ,456, 789")
                int r, g, b;
                std::sscanf(cvalues.c_str(), "%d , %d , %d", &r, &g, &b);
                c.x = float(r)/255.f;
                c.y = float(g)/255.f;
                c.z = float(b)/255.f;
                break;
            } else {
                // If we don't find the end of the rgb() statement,
                // dump all we took back into the main stream.
                *stream << nextWords.str();
            }
        }; [[fallthrough]];
        case 0: {
            c.x = std::stof(w);
            *stream >> w;
            c.y = std::stof(w);
            *stream >> w;
            c.z = std::stof(w);
        };
    }
    return c;
}

Bound *emptyBound() {
    Bound *bo = (Bound*)malloc(sizeof(Bound));
    std::memset(bo, 0, sizeof(Bound));
    return bo;
}

Container *emptyContainer(bool plane) {
    Container *c = (Container*)malloc(sizeof(Container));
    c->a = {0, 0, 0};
    c->b = {0, 0, 0};
    c->c = {0, 0, 0};
    c->d = {0, 0, 0};
    c->plane = plane;
    c->voxelSubdiv = 0;
    c->start = NULL;
    c->end = NULL;
    c->size = 0;
    c->splitAxis = -1;
    c->id = 0;
    return c;
}

int appendToContainer(Container *c, Bound *bo) {
    if (c->start == NULL) c->start = bo;
    if (c->end != NULL) c->end->next = bo;
    c->end = bo;
    /* if (c->plane == false) {
        for (int i = 0; i < 3; i++) {
            c->a(i) = std::min(c->a(i), bo->min(i));
            c->b(i) = std::max(c->b(i), bo->max(i));
        }
    } */
    c->size++;
    return 0;
}

int appendToContainer(Container *c, Bound bo) {
    Bound *bptr = emptyBound();
    std::memcpy(bptr, &bo, sizeof(Bound));
    return 1 + appendToContainer(c, bptr);
}

int appendToContainer(Container *c, Shape *sh) {
    auto bo = emptyBound();
    bo->s = sh;
    return 1 + appendToContainer(c, bo);
}

int appendToContainer(Container *cParent, Container *c) {
    auto bo = emptyBound(); // Alloc 
    bo->s = emptyShape(); // Alloc
    bo->s->c = c;
    return 2 + appendToContainer(cParent, bo);
}

Bound *boundByIndex(Container *c, int i) {
    int idx = 0;
    Bound *bo = c->start;
    while (bo != c->end->next) {
        if (idx == i) return bo;
        idx++;
        bo = bo->next;
    }
    return NULL;
}

int clearContainer(Container *c, bool clearChildShapes) {
    if (c == NULL) return 0;
    int freeCounter = 0;
    Bound *bo = c->start;
    Bound *end = NULL;
    if (c->end != NULL) end = c->end->next;
    int boundCounter = 0;
    int boundLimit = c->voxelSubdiv*c->voxelSubdiv*c->voxelSubdiv;
    while (bo != end) {
        Shape *current = bo->s;
        bool isContainer = current->c != NULL;
        if (isContainer) {
            freeCounter += clearContainer(current->c, clearChildShapes);
            free(current->c);
            current->c = NULL;
            freeCounter++;
        }

        if (clearChildShapes || current->debug) {
            if (current->s != NULL) {
                free(current->s);
                current->s = NULL;
                freeCounter++;
            }
            if (current->t != NULL) {
                free(current->t);
                current->t = NULL;
                freeCounter++;
            }
        }
        if (isContainer || clearChildShapes || current->debug) {
            free(current);
            freeCounter++;
        }
        Bound *next = bo->next;
        if (c->voxelSubdiv != 0) {
            next = bo + 1;
            boundCounter++;
            if (boundCounter >= boundLimit) next = NULL;
        } else {
            free(bo);
        }
        bo = next;
    }
    // Voxel containers have their "start" set to a voxelSubdiv^3 size array.
    if (c->voxelSubdiv != 0) {
        free(c->start);
    }
    c->size = 0;
    c->start = NULL;
    c->end = NULL;
    return freeCounter;
}


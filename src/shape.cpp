#include "shape.hpp"
#include "util.hpp"

#include <iostream>
#include <sstream>
#include <cstring>

namespace {
    const char* w_center = "center";
    const char* w_radius = "radius";
    const char* w_thickness = "thickness";
    const char* w_material = "material";
    const char* w_color = "color";
    const char* w_tex = "tex";
    const char* w_norm = "norm";
    const char* w_refmap = "refmap";
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

Material *emptyMaterial() {
    Material *m = (Material*)alloc(sizeof(Material));
    std::memset(m, 0, sizeof(Material));
    m->shininess = -1.f; // -1 Indicates global shininess param takes precedence
    m->opacity = 1.f;
    m->texId = -1; // No texture
    m->normId = -1; // No normalmap
    m->refId = -1; // No reflectance map
    m->name = NULL;
    m->next = NULL; // Nothing after it
    return m;
}

Shape *emptyShape(bool noMaterial) {
    Shape *sh = (Shape*)alloc(sizeof(Shape));
    std::memset(sh, 0, sizeof(Shape));
    if (!noMaterial) sh->m = emptyMaterial();
    return sh;
}

// FIXME: very out of date
std::string encodeShape(Shape *sh) {
    std::ostringstream fmt;
    fmt << encodeColour(sh->m->color) << " ";
    fmt << w_opacity << " ";
    fmt << sh->m->opacity << " ";
    fmt << w_reflectiveness << " ";
    fmt << sh->m->reflectiveness << " ";
    fmt << w_specular << " ";
    fmt << sh->m->specular << " ";
    fmt << w_shininess << " ";
    fmt << sh->m->shininess << " ";
    fmt << std::endl;
    return fmt.str();
}

Shape *Decoder::decodeShape(std::string in) {
    Shape *sh = emptyShape(true);
    if (usedMaterial != NULL) {
        sh->m = usedMaterial;
    } else {
        sh->m = decodeMaterial(in);
    }
    return sh;
}

#define NONULLMTL() if (m == NULL) { m = emptyMaterial(); newMaterial = true; }

Material *Decoder::decodeMaterial(std::string in, bool definition) {
    if (tex != NULL) tex->lastLoadFail = false;
    if (norm != NULL) norm->lastLoadFail = false;
    if (ref != NULL) ref->lastLoadFail = false;
    std::stringstream stream(in);
    Material *m = NULL;
    bool newMaterial = false;
    do {
        std::string w;
        stream >> w;
        if (w == w_material) {
            stream >> w;
            if (definition) {
                NONULLMTL();
                if (m->name != NULL) free(m->name);
                m->name = (char*)alloc(sizeof(char)*(w.size()+1));
                strncpy(m->name, w.c_str(), w.size()+1);
            } else {
                m = mat->byName(w);
                if (m == NULL) {
                    std::printf("WARNING: Couldn't resolve material \"%s\"\n", w.c_str());
                }
            }
        } else if (w == w_color) {
            NONULLMTL();
            m->color = decodeColour(&stream);
        } else if (w == w_opacity) {
            NONULLMTL();
            stream >> w;
            m->opacity = std::stof(w);
        } else if (w == w_reflectiveness) {
            NONULLMTL();
            stream >> w;
            m->reflectiveness = std::stof(w);
        } else if (w == w_specular) {
            NONULLMTL();
            stream >> w;
            m->specular = std::stof(w);
        } else if (w == w_shininess) {
            NONULLMTL();
            stream >> w;
            m->shininess = std::stof(w);
        } else if (w == w_nolighting) {
            NONULLMTL();
            m->noLighting = true;
        } else if (w == w_tex) {
            NONULLMTL();
            // FIXME: Cope with spaces in filenames
            // FIXME: "Eval" pathnames so they match, even if written differently
            stream >> w;
            if (tex != NULL) {
                m->texId = tex->load(w);
            }
        } else if (w == w_norm) {
            NONULLMTL();
            // FIXME: Cope with spaces in filenames
            // FIXME: "Eval" pathnames so they match, even if written differently
            stream >> w;
            if (norm != NULL) {
                m->normId = norm->load(w);
            }
        } else if (w == w_refmap) {
            NONULLMTL();
            // FIXME: Cope with spaces in filenames
            // FIXME: "Eval" pathnames so they match, even if written differently
            stream >> w;
            if (ref != NULL) {
                m->refId = ref->load(w);
            }
        }
    } while (stream);
    NONULLMTL();
    if (newMaterial) {
        mat->append(m);
    }
    return m;
}

std::string Decoder::encodeSphere(Shape *sh) {
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

Shape *Decoder::decodeSphere(std::string in) {
    std::stringstream stream(in);
    Shape *sh = decodeShape(in);
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

std::string Decoder::encodePointLight(PointLight *p) {
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

PointLight Decoder::decodePointLight(std::string in) {
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

std::string Decoder::encodeTriangle(Shape *sh) {
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

Shape *Decoder::decodeTriangle(std::string in) {
    std::stringstream stream(in);
    Shape *sh = decodeShape(in);
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

    if (sh->m->texId != -1 || sh->m->normId != -1 || sh->m->refId != -1) {
        recalculateTriUVs(sh);
    }

    return sh;
}

void Decoder::recalculateTriUVs(Shape *sh) {
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
    if (sh->m->texId != -1) {
        t = tex->at(sh->m->texId);
    } else if (sh->m->normId != -1) {
        t = norm->at(sh->m->normId);
    } else if (sh->m->refId != -1) {
        t = ref->at(sh->m->refId);
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

std::string Decoder::encodeAAB(Shape *sh) {
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

Shape *Decoder::decodeAAB(std::string in) {
    std::stringstream stream(in);
    Shape *sh = decodeShape(in);
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

    if (sh->m->texId != -1 || sh->m->normId != -1 || sh->m->refId != -1) {
        // FIXME: AABB texture improvements?
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
            if (current->b != NULL) {
                free(current->b);
                current->b = NULL;
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

void MaterialStore::append(Material *m) {
    if (start == NULL) start = m;
    if (end != NULL) end->next = m;
    end = m;
    count++;
}

void MaterialStore::clear() {
    if (start == NULL) return;
    Material *m = start;
    Material *e = NULL;
    if (end != NULL) e = end->next;
    while (m != e) {
        Material *n = m->next;
        free(m);
        m = n;
    }
    clearLists();
    count = 0;
    start = NULL;
    end = NULL;
}

void MaterialStore::clearLists() {
    if (names != NULL) {
        for (int i = 0; i < count; i++) {
            free(names[i]);
        }
        free(names);
        names = NULL;
    }
    if (ptrs != NULL) free(ptrs);
    ptrs = NULL;
}

void MaterialStore::genLists() {
    clearLists();
    names = (char**)alloc(count * sizeof(char*));
    ptrs = (Material**)alloc(count * sizeof(Material*));

    int i = 0;
    Material *m = start;
    Material *e = NULL;
    if (end != NULL) e = end->next;
    while (m != e) {
        std::string name;
        if (m->name != NULL) name = std::string(m->name);
        if (name.empty()) {
            name = "Material #" + std::to_string(i);
        }
        names[i] = (char*)alloc(sizeof(char)*(name.size()+1));
        strncpy(names[i], name.c_str(), name.size()+1);
        ptrs[i] = m;
        m = m->next;
        i++;
    }
}

Material *MaterialStore::byName(std::string name) {
    Material *m = start;
    Material *e = NULL;
    if (end != NULL) e = end->next;
    while (m != e) {
        if (m->name != NULL && name == std::string(m->name)) return m;
        m = m->next;
    }
    return NULL;
}

void Decoder::usingMaterial(std::string name) {
    usedMaterial = mat->byName(name);
    if (usedMaterial == NULL) {
        std::printf("WARNING: Couldn't resolve material \"%s\"\n", name.c_str());
    }
}

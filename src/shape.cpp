#include "shape.hpp"
#include "ray.hpp"
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
    const char* w_translate = "translate";
    const char* w_rotate = "rotate";
    const char* w_scale = "scale";
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
            w = collectWordOrString(stream);
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

// FIXME: very out of date
/* std::string encodeShape(Shape *sh) {
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
} */

void Decoder::decodeShape(Shape *sh, std::string in) {
    if (usedMaterial != NULL) {
        sh->material = usedMaterial;
    } else {
        sh->material = decodeMaterial(in);
    }
    std::stringstream stream(in);
    do {
        std::string w;
        stream >> w;
        if (w == w_translate) {
            stream >> w;
            sh->trans().translate.x = std::stof(w);
            stream >> w;
            sh->trans().translate.y = std::stof(w);
            stream >> w;
            sh->trans().translate.z = std::stof(w);
        } else if (w == w_rotate) {
            stream >> w;
            sh->trans().rotate.x = std::stof(w);
            stream >> w;
            sh->trans().rotate.y = std::stof(w);
            stream >> w;
            sh->trans().rotate.z = std::stof(w);
        } else if (w == w_scale) {
            stream >> w;
            sh->trans().scale = std::stof(w);
        }
    } while (stream);
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
            w = collectWordOrString(stream);
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
            w = collectWordOrString(stream);
            if (tex != NULL) {
                m->texId = tex->load(w);
            }
        } else if (w == w_norm) {
            NONULLMTL();
            w = collectWordOrString(stream);
            if (norm != NULL) {
                m->normId = norm->load(w);
            }
        } else if (w == w_refmap) {
            NONULLMTL();
            w = collectWordOrString(stream);
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

/*std::string Decoder::encodeSphere(Shape *sh) {
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
}*/

Sphere *Decoder::decodeSphere(std::string in) {
    std::stringstream stream(in);
    Sphere *sh = new Sphere();
    decodeShape(sh, in);
    do {
        std::string w;
        stream >> w;
        if (w == w_center) {
            stream >> w;
            sh->center.x = std::stof(w);
            stream >> w;
            sh->center.y = std::stof(w);
            stream >> w;
            sh->center.z = std::stof(w);
        } else if (w == w_radius) {
            stream >> w;
            sh->radius = std::stof(w);
        } else if (w == w_thickness) {
            stream >> w;
            sh->thickness = std::stof(w);
        }
    } while (stream);
    return sh;
}

/* std::string Decoder::encodePointLight(PointLight *p) {
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
} */

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

/* std::string Decoder::encodeTriangle(Shape *sh) {
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
} */

Triangle *Decoder::decodeTriangle(std::string in) {
    std::stringstream stream(in);
    Triangle *sh = new Triangle();
    decodeShape(sh, in);

    do {
        std::string w;
        stream >> w;
        if (w == w_a) {
            stream >> w;
            sh->a.x = std::stof(w);
            stream >> w;
            sh->a.y = std::stof(w);
            stream >> w;
            sh->a.z = std::stof(w);
        } else if (w == w_b) {
            stream >> w;
            sh->b.x = std::stof(w);
            stream >> w;
            sh->b.y = std::stof(w);
            stream >> w;
            sh->b.z = std::stof(w);
        } else if (w == w_c) {
            stream >> w;
            sh->c.x = std::stof(w);
            stream >> w;
            sh->c.y = std::stof(w);
            stream >> w;
            sh->c.z = std::stof(w);
        } else if (w == w_plane) {
            sh->plane = true;
        }
    } while (stream);

    // FIXME: Maybe best to do this for all tris regardless of texture,
    // since the user can now dynamically apply them.
    if (sh->mat()->hasTexture()) {
        recalculateTriUVs(sh);
    }

    return sh;
}

void Decoder::recalculateTriUVs(Triangle *tri) {
    // Project onto 2D plane, any'll do for now
    float proj[3][2] = {
        {tri->a.x, tri->a.y},
        {tri->b.x, tri->b.y},
        {tri->c.x, tri->c.y}
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
    if (tri->mat()->texId != -1) {
        t = tex->at(tri->mat()->texId);
    } else if (tri->mat()->normId != -1) {
        t = norm->at(tri->mat()->normId);
    } else if (tri->mat()->refId != -1) {
        t = ref->at(tri->mat()->refId);
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
        tri->UVs[i] = {(proj[i][0] - min[0])*scale.x/range[0], (proj[i][1] - min[1])*scale.y/range[1]};
    }
}

/* std::string Decoder::encodeAAB(Shape *sh) {
    std::ostringstream fmt;
    fmt << "box ";
    fmt << w_a << " ";
    fmt << sh->t->a.x << " " << sh->t->a.y << " " << sh->t->a.z << " ";
    fmt << w_b << " ";
    fmt << sh->t->b.x << " " << sh->t->b.y << " " << sh->t->b.z << " ";
    fmt << encodeShape(sh) << " ";
    fmt << std::endl;
    return fmt.str();
} */

AAB *Decoder::decodeAAB(std::string in) {
    std::stringstream stream(in);
    AAB *sh = new AAB();
    decodeShape(sh, in);

    do {
        std::string w;
        stream >> w;
        if (w == w_a) {
            stream >> w;
            sh->min.x = std::stof(w);
            stream >> w;
            sh->min.y = std::stof(w);
            stream >> w;
            sh->min.z = std::stof(w);
        } else if (w == w_b) {
            stream >> w;
            sh->max.x = std::stof(w);
            stream >> w;
            sh->max.y = std::stof(w);
            stream >> w;
            sh->max.z = std::stof(w);
        }
    } while (stream);

    // Potential FIXME: Texture mapping improvments for AABs
    // if (sh->mat()->hasTexture()) { }

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

int Container::append(Bound *bo) {
    if (start == NULL) start = bo;
    if (end != NULL) end->next = bo;
    end = bo;
    /* if (plane == false) {
        for (int i = 0; i < 3; i++) {
            a(i) = std::min(a(i), bo->min(i));
            b(i) = std::max(b(i), bo->max(i));
        }
    } */
    size++;
    return 0;
}

int Container::append(Bound bo) {
    Bound *bptr = emptyBound();
    std::memcpy(bptr, &bo, sizeof(Bound));
    return 1 + append(bptr);
}

int Container::append(Shape *sh) {
    auto bo = emptyBound();
    bo->s = sh;
    return 1 + append(bo);
}

int Container::append(Container *c) {
    auto bo = emptyBound(); // Alloc 
    Container *nC = new Container(c); // Alloc
    bo->s = nC;
    return 2 + append(bo);
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

int Container::clear(bool deleteShapes) {
    int freeCounter = 0;
    Bound *bo = start;
    Bound *e = NULL;
    if (end != NULL) e = end->next;
    int boundCounter = 0;
    int boundLimit = voxelSubdiv*voxelSubdiv*voxelSubdiv;
    while (bo != e) {
        Shape *current = bo->s;
        freeCounter += current->clear(deleteShapes);
        if (
            (dynamic_cast<Container*>(current) != nullptr) ||
            deleteShapes ||
            current->debug) {

            delete current;
            freeCounter ++;

        } else if (deleteShapes || current->debug) {
            delete current;
            freeCounter++;
        }

        Bound *next = bo->next;
        if (voxelSubdiv != 0) {
            next = bo + 1;
            boundCounter++;
            if (boundCounter >= boundLimit) next = NULL;
        } else {
            free(bo);
        }
        bo = next;
    }
    // Voxel containers have their "start" set to a voxelSubdiv^3 size array.
    if (voxelSubdiv != 0) {
        free(start);
    }
    size = 0;
    start = NULL;
    end = NULL;
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


void AAB::bounds(Bound *bo) {
    bo->min = min;
    bo->max = max;
    bo->centroid = centroid;
}

float AAB::intersect(Vec3 p0, Vec3 delta, Vec3 *normal, Vec2 *uv) {
    // Based on the "slab method".
    // Find the distance along (delta) you'd have to travel to hit the planes of each pair of parallel faces,
    // then select the largest distance to one of the nearer faces, and the shortest distance to one of the furthest faces.
    // If the latter is negative, we're looking in the wrong direction.
    // Where the ray actually intersects the AABB, the latter (furthest) should be greater than the former (nearest).
    // and therefore if a nearer face appears to be closer than a farther face, we do not intersect.
    
    // Pre-divide, since multiplication is faster and we use it twice
    float d[3] = {1.f/delta.x, 1.f/delta.y, 1.f/delta.z};
    float tmin = -9999.f;
    float tmax = 9999.f;
    int nminIdx = -1, nmaxIdx = -1;
    for (int i = 0; i < 3; i++) {
        float t0 = (min(i) - p0(i)) * d[i];
        float t1 = (max(i) - p0(i)) * d[i];
        float mn = std::min(t0, t1);
        if (mn >= tmin) {
            tmin = mn;
            nminIdx = i;
        }
        float mx = std::max(t0, t1);
        if (mx <= tmax) {
            tmax = mx;
            nmaxIdx = i;
        }
    }

    if (tmax < 0) return -9999.f;
    if (tmin > tmax) return -9998.f;
    if (normal == NULL) return tmin;
    *normal = {0.f, 0.f, 0.f};
    // Inside the box
    if (tmin < 0.f) {
        normal->idx(nmaxIdx) = delta(nmaxIdx) >= 0 ? -1.f : 1.f;
        tmin = tmax;
    // Outside the box
    } else {
        normal->idx(nminIdx) = delta(nminIdx) >= 0 ? -1.f : 1.f;
    }
    if (uv != NULL) {
        Vec3 hit = p0 + (tmin * delta);
        *uv = getUV(hit, *normal);
    }
    return tmin;
    // normal->idx(normIdx) = 1.f;
}

bool AAB::intersects(Vec3 p0, Vec3 delta) {
    return intersect(p0, delta, NULL) < -9990.f ? false : true;
}

Vec2 AAB::getUV(Vec3 hit, Vec3 normal) {
    int uvIdx = 0;
    Vec2 uv = {0.f, 0.f};
    for (int i = 0; i < 3; i++) {
        if (normal(i) != 0.f) continue;
        uv(uvIdx) = (hit(i) - min(i)) / (max(i) - min(i));
        uvIdx++;
    }
    return uv;
}

Shape AAB::applyTransform() {
    AAB b(&this);
    b.bakeTransform();
    return b;
}

void AAB::bakeTransform() {
    if (!transform.needed()) return;
    Mat4 m = transform.build();
    min = min * m;
    max = max * m;
    transform.reset();
};

void AAB::refract(float ri, Vec3 p0, Vec3 delta, Vec3 *p1, Vec3 *delta1) {
    *p1 = p0;
    *delta1 = delta;
    // FIXME: AAB opacity!
    std::printf("FIXME: AAB opacity!\n");
}

void Sphere::bounds(Bound *bo) {
    bo->min = center - radius;
    bo->max = center + radius;
    bo->centroid = center;
};

float Sphere::intersect(Vec3 p0, Vec3 delta, Vec3 *normal, Vec2 *uv) {
    // CG:PaP 2nd ed. in C, p. 703, eq. 15.17 is an expanded sphere equation with
    // substituted values for the camera position (x/y/z_0),
    // pixel vec from camera (delta x/y/z),
    // and normalized distance along pixel vector (t).
    Vec3 originToSphere = p0 - center;
    float a = dot(delta, delta);
    float b = 2.f * dot(delta, originToSphere);
    float c = dot(originToSphere, originToSphere) - radius*radius;
    float discrim = b*b - 4.f*a*c;
    if (discrim < 0) return -1;
    float discrimRoot = std::sqrt(discrim);
    float denom = 0.5f / a;
    float t = (-b - discrimRoot) * denom;
    if (discrim > 0 && t < 0) {
        float t_1 = (-b + discrimRoot) * denom;
        float t_ = 0.f;
        if (t < 0 || (t > t_1 && t_1 > 0)) {
            t_ = t;
            t = t_1;
            t_1 = t_;
        }
        /* if (t1 != NULL) {
            *t1 = t_1;
        } */
    }
    if (normal != NULL) {
        Vec3 hit = p0 + (t * delta);
        *normal = hit - center;
        if (uv != NULL) {
            *uv = getUV(hit);
        }
    }
    return t;
}

bool Sphere::intersects(Vec3 p0, Vec3 delta) {
    return intersect(p0, delta, NULL) >= 0;
}

Vec2 Sphere::getUV(Vec3 hit) {
    // "Move" the sphere (and the point on it) to center O.
    hit = hit - center;
    // Get polar angles from O
    float theta = std::atan2(hit.x, hit.z);
    float phi = std::acos(hit.y / radius);

    // Divide by 360deg so range is 1,
    float rU = theta / (2.f*M_PI);

    return Vec2{
        1.f - (rU + 0.5f), // and shift upwards so between 0 & 1.
        1.f - (phi / float(M_PI))
    };
}

Shape Sphere::applyTransform() {
    Sphere s(&this);
    s.bakeTransform();
    return b;
}

void Sphere::bakeTransform() {
    if (!transform.needed()) return;
    Mat4 m = transform.build();
    center = center * m;
    radius = radius * transform.scale;
    transform.reset();
}

// Refracts through the sphere as if it were solid (i.e. ignoring thickness).
void Sphere::refractSolid(float r0, float r1, Vec3 p0, Vec3 delta, Vec3 *p1, Vec3 *delta1) {
    Vec3 normal = p0 - s->center;
    bool tir = true;
    Vec3 r = Refract(r0, r1, delta, normal, &tir);
    if (tir) {
        r = delta - 2.f*dot(delta, normal) * normal;
        *delta1 = r;
        *p1 = p0 + (EPSILON * r);
        return;
    }
    tir = true;
    while (tir) {
        p0 = p0 + (EPSILON * r);
        float t = meetsSphere(p0, r, s, NULL);
        p0 = p0 + (t * r);
        Vec3 n = -1.f*(p0 - s->center);
        Vec3 r2 = Refract(r1, r0, r, n, &tir);
        if (tir) {
            r = r - 2.f*dot(r, n) * n;
        } else {
            r = r2;
        }
    }
    *p1 = p0;
    *delta1 = r;
}

void Sphere::refract(float ri, Vec3 p0, Vec3 delta, Vec3 *p1, Vec3 *delta1) {
    float hollowness = 1.f - thickness;
    if (hollowness = 1.f) {
        ri = RI_AIR;
        hollowness = 0.f;
    }

    if (hollowness = 0.f) {
        refractSolid(RI_AIR, ri, p0, delta, p1, delta1);
        return;
    }
    // Into outer sphere (RI_AIR -> RI_SPHERE)
    *p1 = p0;
    Vec3 normal = *p1 - center;
    bool tir = true;
    Vec3 r = Refract(RI_AIR, ri, delta, normal, &tir);
    if (tir) {
        // Reflect off the sphere.
        // FIXME: Put this reflection in a shared function!
        *delta1 = delta - 2.f * dot(delta, normal) * normal;
        return;
    }
    bool escaped = false;
    *delta1 = r;
    // Our smaller, inner sphere, filled with RI_AIR.
    Sphere innerSphere(&this);
    innerSphere.radius *= hollowness;
    innerSphere.thickness = 1.f;
    while (!escaped) {
        // Hit the inner sphere
        float t = innerSphere.intersect(*p1, *delta1);
        if (t < 0) { // Missing the internal sphere means we can ignore it entirely.
            refractSolid(RI_AIR, ri, p0, delta, p1, delta1);
            return;
        }
        *p1 = *p1 + (t * *delta1);
        Vec3 p2; // Temp variable just in case
        // Refract in and out of the inner sphere RI_SPHERE -> RI_AIR -> RI_SPHERE)
        innerSphere.refractSolid(ri, RI_AIR, *p1, *delta1, &p2, &r);
        *p1 = p2;
        *delta1 = r;
        // Hit the outer sphere
        t = intersect(*p1, *delta1);
        *p1 = *p1 + (t * *delta1)
        normal = -1.f * (*p1 - center);
        // Out of the outer sphere (RI_SPHERE -> RI_AIR)
        r = Refract(ri, RI_AIR, *delta1, normal, &tir);
        if (tir) {
            // If we have TIR, reflect back into the sphere, and loop this process again.
            // FIXME: Put this reflection in a shared function!
            *delta1  = *delta1 - 2.f * dot(*delta1, normal) * normal;
            continue;
        }
        *delta1 = r;
        escaped = true;
    }
}

void Triangle::bounds(Bound *bo) {
    bo->min = {9999, 9999, 9999};
    bo->max = {-9999, -9999, -9999};
    bo->centroid = {0, 0, 0};
    if (plane) return;
    for (int i = 0; i < 3; i++) {
        bo->min(i) = std::min(bo->min(i), a(i));
        bo->min(i) = std::min(bo->min(i), b(i));
        bo->min(i) = std::min(bo->min(i), c(i));
        bo->max(i) = std::max(bo->max(i), a(i));
        bo->max(i) = std::max(bo->max(i), b(i));
        bo->max(i) = std::max(bo->max(i), c(i));
    }
    bo->centroid = 0.333f * (a + b + c);
}

// Finds the normal of a triangle (abc) that points towards given vector p0
Vec3 Triangle::visibleNormal(Vec3 delta) {
    /* Vec3 norms[2] = {cross(a - c, b - c), cross(c - a, b - a)};
    float components[2] = {0, 0};
    for (int i = 0; i < 2; i++) {
        // Roughly computing component of each normal along vector delta,
        // a.k.a (n_i \dot delta)/|delta|.
        // but since we just want the lowest one (most negative), the denom. can be ignored.
        components[i] = dot(norms[i], delta);
    }
    if (components[0] <= components[1])
        return norms[0];
    return norms[1]; */
    Vec3 norm = cross(c-a, b-a);
    if (dot(norm, delta) <= 0.f) return norm;
    else return cross(a-c, b-c);
}

float Triangle::intersectsPlane(Vec3 p0, Vec3 delta, Vec3 normal) {
    // dot product of a line on the plane with the normal is zero, hence (p - t.a) \dot norm = 0, where p is a random point (x, y, z)
    // p \dot norm = t.a \dot norm
    // (p0 + t(delta)) \dot norm = t.a \dot norm
    // t(delta \dot norm) = (t.a - p0) \dot norm
    // divide and get t!
    float denom = dot(normal, delta);
    if (denom == 0) { // Plane parallel to ray, ignore
        return -1;
    }
    return dot((a - p0), normal) / denom;
}

bool Triangle::projectAndPiP(Vec3 normal, Vec3 hit) {
    // Sign doesn't matter here as our triangles aren't single-faced.
    Vec3 absNormal = {std::abs(normal.x), std::abs(normal.y), std::abs(normal.z)};
    // project orthographically as big as possible
    Vec2 projA, projB, projC, projHit;
    if (absNormal.x >= absNormal.y && absNormal.x >= absNormal.z) {
        // std::printf("projecting onto x\n");
        projA = Vec2{a.y, a.z};
        projB = Vec2{b.y, b.z};
        projC = Vec2{c.y, c.z};
        projHit = Vec2{hit.y, hit.z};
    } else if (absNormal.y >= absNormal.x && absNormal.y >= absNormal.z) {
        // std::printf("projecting onto y\n");
        projA = Vec2{a.x, a.z};
        projB = Vec2{b.x, b.z};
        projC = Vec2{c.x, c.z};
        projHit = Vec2{hit.x, hit.z};
    } else { 
        // std::printf("projecting onto z\n");
        projA = Vec2{a.x, a.y};
        projB = Vec2{b.x, b.y};
        projC = Vec2{c.x, c.y};
        projHit = Vec2{hit.x, hit.y};
    }

    return PiP(projHit, projA, projB, projC);
}

// CGPaP in C p.339 Sect. 7.12.2 "PtInPolygon":
// Cast a ray from our point in any direction.
// If the ray hits an edge once, we're inside. If twice, we're outside.
// Uses cramer's rule to find point where ray meets edge.
bool Triangle::PiP(Vec2 projHit, Vec2 projA, Vec2 projB, Vec2 projC) {
    // std::printf("tri %f %f %f %f %f %f\n", a.x, a.y, b.x, b.y, c.x, c.y);
    // std::printf("point %f %f\n", p.x, p.y);
    int collisions = 0;
    float maxX = std::fmax(std::fmax(projA.x, projB.x), projC.x);
    float minX = std::fmin(std::fmin(projA.x, projB.x), projC.x);
    float maxY = std::fmax(std::fmax(projA.y, projB.y), projC.y);
    float minY = std::fmin(std::fmin(projA.y, projB.y), projC.y);
    // Easy case: point is outside triangle's bounding box.
    // These aren't full-fledged polygons so they aren't gonna have
    // a hole in the middle.
    if (projHit.x < minX || projHit.x > maxX || projHit.y < minY || projHit.y > maxY) {
        return false;
    }
    // Cast a ray across the x axis, p + q, where q=(maxX+1, p.y) - p:
    Vec2 q = Vec2{maxX+1.f, projHit.y} - projHit;
    Vec2 edges[3][2] = {{projA, projB}, {projB, projC}, {projC, projA}};
    for (int i = 0; i < 3; i++) {
        Vec2 r = edges[i][0];
        Vec2 s = edges[i][1] - r;
        // Find where p + tq = r + us, using cramer's rule:
        Mat2 d = Mat2{
            q.x, -s.x,
            q.y, -s.y
        };
        Mat2 dt = Mat2{
            r.x - projHit.x, -s.x,
            r.y - projHit.y, -s.y
        };
        Mat2 du = Mat2{
            q.x, r.x - projHit.x,
            q.y, r.y - projHit.y
        };
        float detD = det(d);
        float t = det(dt)/detD;
        float u = det(du)/detD;
        if (detD != 0 && t >= 0.f && u >= 0.f && t <= 1.f && u <= 1.f) {
            // Vec2 g = p + (t * q);
            // Vec2 j = r + (u * s);
            // std::printf("collision on %d ((%f %f) + t(%f %f) = (%f %f) + u(%f %f))! t=%f (%f, %f), u=%f (%f, %f)\n", i, p.x, p.x, q.x, q.y, r.x, r.y, s.x, s.y, t, g.x, g.y, u, j.x, j.y);
            collisions += 1;
        }
        if (collisions == 2) break;
    }
    // if (collisions == 1) {
    //     std::printf("in triangle!\n");
    // }
    return collisions == 1;
}

// Default method: Muller-Trumbore
// Faster triangle collision algorithm which calculates barycentric coordinates to determine t.
// https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
float Triangle::intersectMT(Vec3 p0, Vec3 delta, Vec3 *bary) {
    Vec3 e1 = b - a;
    Vec3 e2 = c - a;
    Vec3 rayEdge = cross(delta, e2);
    float det = dot(e1, rayEdge);

    // Uncomment for sided-ness
    // if (det <= 0.f) return -1.f;

    float invDet = 1.f / det;
    Vec3 ap0 = p0 - a;
    // u
    bary->x = invDet * dot(ap0, rayEdge);

    if (bary->x < 0.f || bary->x > 1.f) return -1.f;

    Vec3 ap0Edge = cross(ap0, e1);
    // v
    bary->y = invDet * dot(delta, ap0Edge);

    if (bary->y < 0.f || bary->x + bary->y > 1.f) return -1.f;

    float t = invDet * dot(e2, ap0Edge);

    // w
    bary->z = 1.f - bary->x - bary->y;

    return t;
}

#define PREFER_MT

// FIXME: This new version removes the "potentialCollisions" mechanism entirely,
// where collisions that were tris farther than an already intersected object were not fully
// intersected, only their plane was, but the potential for a collision recorded and used too
// decide whether to cast another ray through a transparent object.
// While the old tri collision is mostly useless now, potentially add back?
float Triangle::intersect(Vec3 p0, Vec3 delta, Vec3 *normal, Vec2 *uv) {
    // FIXME: Normal calculation might not be necessary
    Vec3 n = visibleNormal(delta);
    if (normal != NULL) *normal = n;
#ifdef PREFER_MT
    if (!plane) {
        Vec3 bary;
        float t = intersectMT(p0, delta, &bary);
        if (uv != NULL && material != NULL && material->hasTexture()) {
            *uv = bary.z*UVs[0] + bary.x*UVs[1] + bary.y*UVs[2];
        }
        return t;
    } else {
#endif
        float t = intersectsPlane(p0, delta, n);
        if (t < 0) return t;
        Vec3 hit = p0 + (t * delta);
        if (plane || projectAndPiP(norm(n), hit)) {
            return t;
        }
        return -1;
#ifdef PREFER_MT
    }
#endif
};

bool Triangle::intersects(Vec3 p0, Vec3 delta) {
    return intersect(p0, delta, NULL, NULL) >= 0;
}

Shape Triangle::applyTransform() {
    Triangle t(&this);
    t.bakeTransform();
    return b;
}

void Triangle::bakeTransform() {
    if (!transform.needed()) return;
    Mat4 m = transform.build();
    a = a * m;
    b = b * m;
    c = c * m;
    transform.reset();
}

void Triangle::refract(float ri, Vec3 p0, Vec3 delta, Vec3 *p1, Vec3 *delta1) {
    *p1 = p0;
    *delta1 = delta;
}

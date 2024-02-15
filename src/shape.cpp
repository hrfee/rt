#include "shape.hpp"
#include "util.hpp"

#include <iostream>
#include <sstream>
#include <cstring>

namespace {
    const char* w_center = "center";
    const char* w_radius = "radius";
    const char* w_color = "color";
    const char w_color_hex = '#';
    const char* w_color_rgb_start = "rgb(";
    const char w_color_rgb_end = ')';
    const char* w_opacity = "opacity";
    const char* w_reflectiveness = "reflectiveness";
    const char* w_specular = "specular";
    // const char* w_emission = "emission";
    const char* w_brightness = "brightness";
    const char* w_shininess = "shininess";
    const char* w_a = "a";
    const char* w_b = "b";
    const char* w_c = "c";
}

Shape *emptyShape() {
    Shape *sh = (Shape*)alloc(sizeof(Shape));
    /*sh->s = NULL;
    sh->t = NULL;
    sh->c = NULL;
    sh->next = NULL;*/
    std::memset(sh, 0, sizeof(Shape));
    sh->shininess = -1.f; // -1 Indicates global shininess param takes precedence
    sh->opacity = 1.f;
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

Shape *decodeShape(std::string in) {
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
    fmt << encodeShape(sh) << " ";
    fmt << std::endl;
    return fmt.str();
}

Shape *decodeSphere(std::string in) {
    std::stringstream stream(in);
    Shape *sh = decodeShape(in);
    sh->s = (Sphere*)alloc(sizeof(Sphere));
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

Shape *decodeTriangle(std::string in) {
    std::stringstream stream(in);
    Shape *sh = decodeShape(in);
    sh->t = (Triangle*)alloc(sizeof(Triangle));
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
        }
    } while (stream);
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
        };
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

#include "shape.hpp"

#include <iostream>
#include <sstream>

namespace {
    const char* w_center = "center";
    const char* w_radius = "radius";
    const char* w_color = "color";
    const char w_color_hex = '#';
    const char* w_color_rgb_start = "rgb(";
    const char w_color_rgb_end = ')';
    const char* w_reflectiveness = "reflectiveness";
    const char* w_specular = "specular";
    // const char* w_emission = "emission";
    const char* w_brightness = "brightness";
    const char* w_shininess = "shininess";
    const char* w_a = "a";
    const char* w_b = "b";
    const char* w_c = "c";
}

std::string encodeSphere(Sphere *s) {
    std::ostringstream fmt;
    fmt << "sphere ";
    fmt << w_center << " ";
    fmt << s->center.x << " " << s->center.y << " " << s->center.z << " ";
    fmt << w_radius << " ";
    fmt << s->radius << " ";
    fmt << encodeColour(s->color) << " ";
    fmt << w_reflectiveness << " ";
    fmt << s->reflectiveness << " ";
    fmt << w_specular << " ";
    fmt << s->specular << " ";
    fmt << w_shininess << " ";
    fmt << s->shininess << " ";
    fmt << std::endl;
    return fmt.str();
}

Sphere *decodeSphere(std::string in) {
    std::stringstream stream(in);
    Sphere *s = (Sphere*)malloc(sizeof(Sphere)); // FIXME: Error check!
    s->shininess = -1.f; // -1 Indicates global shininess param takes precedence
    do {
        std::string w;
        stream >> w;
        if (w == w_center) {
            stream >> w;
            s->center.x = std::stof(w);
            stream >> w;
            s->center.y = std::stof(w);
            stream >> w;
            s->center.z = std::stof(w);
        } else if (w == w_radius) {
            stream >> w;
            s->radius = std::stof(w);
        } else if (w == w_color) {
            s->color = decodeColour(&stream);
        } else if (w == w_reflectiveness) {
            stream >> w;
            s->reflectiveness = std::stof(w);
        } else if (w == w_specular) {
            stream >> w;
            s->specular = std::stof(w);
        } else if (w == w_shininess) {
            stream >> w;
            s->shininess = std::stof(w);
        }
    } while (stream);
    return s;
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

std::string encodeTriangle(Triangle *t) {
    std::ostringstream fmt;
    fmt << "triangle ";
    fmt << w_a << " ";
    fmt << t->a.x << " " << t->a.y << " " << t->a.z << " ";
    fmt << w_b << " ";
    fmt << t->b.x << " " << t->b.y << " " << t->b.z << " ";
    fmt << w_c << " ";
    fmt << t->c.x << " " << t->c.y << " " << t->c.z << " ";
    fmt << encodeColour(t->color) << " ";
    fmt << w_reflectiveness << " ";
    fmt << t->reflectiveness << " ";
    fmt << w_specular << " ";
    fmt << t->specular << " ";
    fmt << w_shininess << " ";
    fmt << t->shininess << " ";
    fmt << std::endl;
    return fmt.str();
}

Triangle *decodeTriangle(std::string in) {
    std::stringstream stream(in);
    Triangle *t = (Triangle*)malloc(sizeof(Triangle));
    t->shininess = -1.f; // -1 Indicates global shininess param takes precedence
    do {
        std::string w;
        stream >> w;
        if (w == w_a) {
            stream >> w;
            t->a.x = std::stof(w);
            stream >> w;
            t->a.y = std::stof(w);
            stream >> w;
            t->a.z = std::stof(w);
        } else if (w == w_b) {
            stream >> w;
            t->b.x = std::stof(w);
            stream >> w;
            t->b.y = std::stof(w);
            stream >> w;
            t->b.z = std::stof(w);
        } else if (w == w_c) {
            stream >> w;
            t->c.x = std::stof(w);
            stream >> w;
            t->c.y = std::stof(w);
            stream >> w;
            t->c.z = std::stof(w);
        } else if (w == w_color) {
            t->color = decodeColour(&stream);
        } else if (w == w_reflectiveness) {
            stream >> w;
            t->reflectiveness = std::stof(w);
        } else if (w == w_specular) {
            stream >> w;
            t->specular = std::stof(w);
        } else if (w == w_shininess) {
            stream >> w;
            t->shininess = std::stof(w);
        }
    } while (stream);
    return t;
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

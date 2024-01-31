#include "shape.hpp"

#include <iostream>
#include <sstream>

namespace {
    const char* w_center = "center";
    const char* w_radius = "radius";
    const char* w_color = "color";
    const char* w_reflectiveness = "reflectiveness";
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
    fmt << w_color << " ";
    fmt << s->color.x << " " << s->color.y << " " << s->color.z << " ";
    fmt << w_reflectiveness << " ";
    fmt << s->reflectiveness;
    fmt << std::endl;
    return fmt.str();
}

Sphere decodeSphere(std::string in) {
    std::istringstream stream(in);
    Sphere s;
    do {
        std::string w;
        stream >> w;
        if (w == w_center) {
            stream >> w;
            s.center.x = std::stof(w);
            stream >> w;
            s.center.y = std::stof(w);
            stream >> w;
            s.center.z = std::stof(w);
        } else if (w == w_radius) {
            stream >> w;
            s.radius = std::stof(w);
        } else if (w == w_color) {
            stream >> w;
            s.color.x = std::stof(w);
            stream >> w;
            s.color.y = std::stof(w);
            stream >> w;
            s.color.z = std::stof(w);
        } else if (w == w_reflectiveness) {
            stream >> w;
            s.reflectiveness = std::stof(w);
        }
    } while (stream);
    return s;
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
    fmt << w_color << " ";
    fmt << t->color.x << " " << t->color.y << " " << t->color.z << " ";
    fmt << w_reflectiveness << " ";
    fmt << t->reflectiveness;
    fmt << std::endl;
    return fmt.str();
}

Triangle decodeTriangle(std::string in) {
    std::istringstream stream(in);
    Triangle t;
    do {
        std::string w;
        stream >> w;
        if (w == w_a) {
            stream >> w;
            t.a.x = std::stof(w);
            stream >> w;
            t.a.y = std::stof(w);
            stream >> w;
            t.a.z = std::stof(w);
        } else if (w == w_b) {
            stream >> w;
            t.b.x = std::stof(w);
            stream >> w;
            t.b.y = std::stof(w);
            stream >> w;
            t.b.z = std::stof(w);
        } else if (w == w_c) {
            stream >> w;
            t.c.x = std::stof(w);
            stream >> w;
            t.c.y = std::stof(w);
            stream >> w;
            t.c.z = std::stof(w);
        } else if (w == w_color) {
            stream >> w;
            t.color.x = std::stof(w);
            stream >> w;
            t.color.y = std::stof(w);
            stream >> w;
            t.color.z = std::stof(w);
        } else if (w == w_reflectiveness) {
            stream >> w;
            t.reflectiveness = std::stof(w);
        }
    } while (stream);
    return t;
}

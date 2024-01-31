#include "shape.hpp"

#include <iostream>
#include <sstream>

namespace {
    const char* w_center = "center";
    const char* w_radius = "radius";
    const char* w_color = "color";
    const char* w_reflectiveness = "reflectiveness";
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

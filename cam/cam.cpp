#include "cam.hpp"
#include "../vec/mat.hpp"

#include <vector>
#include <cmath>

Mat3 make_x_rotation(float radians) {
    float c = std::cos(radians);
    float s = std::sin(radians);
    return Mat3{
        1.f, 0.f, 0.f,
        0.f, c, -s,
        0.f, s, c
    };
}

Mat3 make_y_rotation(float radians) {
    float c = std::cos(radians);
    float s = std::sin(radians);
    return Mat3{
        c, 0.f, s,
        0.f, 1.f, 0.f,
        -s, 0.f, c
    };
}

Mat3 make_z_rotation(float radians) {
    float c = std::cos(radians);
    float s = std::sin(radians);
    return Mat3{
        c, -s, 0.f,
        s, c, 0.f,
        0.f, 0.f, 1.f
    };
}

// Given a camera object, calculate the position of the bottom-left corner of the viewport, and provide vectors for columns and rows.
// position of camera must be applied manually!
void calculateViewport(Camera *cam) {
    Vec3 frustumVec = {cam->nearFrustumDistance, 0.f, 0.f};
    float halfViewportWidth = cam->nearFrustumDistance * std::tan(cam->fov/2.f);
    float halfViewportHeight = (halfViewportWidth / float(cam->w)) * float(cam->h);
   
    // Calculate and store the bottom-left corner of the viewport
    cam->viewportCorner = frustumVec;
    cam->viewportCorner.z -= halfViewportWidth;
    cam->viewportCorner.y -= halfViewportHeight;

    Mat3 rotZ = make_z_rotation(cam->phi);
    Mat3 rotY = make_y_rotation(cam->theta);
    
    cam->viewportCorner =  rotY * (rotZ * cam->viewportCorner);
    cam->viewportCol = frustumVec;
    cam->viewportCol.y -= halfViewportHeight;
    cam->viewportCol = rotY * (rotZ * cam->viewportCol);
    cam->viewportCol = cam->viewportCol - cam->viewportCorner;
    // cam->viewportCol - cam->viewportCorner is now a vector along the viewport plane with width equivalent to cam->w/2, so scale it down to 1px:
    cam->viewportCol = 2.f * cam->viewportCol / float(cam->w);

    cam->viewportRow = frustumVec;
    cam->viewportRow.z -= halfViewportWidth;
    cam->viewportRow = rotY * (rotZ * cam->viewportRow);
    cam->viewportRow = cam->viewportRow - cam->viewportCorner;

    // and the equivalent for the row vector:
    cam->viewportRow = 2.f * cam->viewportRow / float(cam->h);
}

Camera newCamera(int w, int h, float fov, Vec3 pos) {
    Camera cam;
    cam.w = w;
    cam.h = h;
    cam.fov = fov;
    cam.position = pos;
    cam.phi = 0.f;
    cam.theta = 0.f;
    cam.nearFrustumDistance = 1.f;
    cam.farFrustumDistance = 9999.f;
    return cam;
}

void debugPrintCorners(Camera *cam) {
    std::vector<Vec3> corners;
    corners.emplace_back(cam->position);
    corners.emplace_back(cam->position + cam->viewportCorner);
    corners.emplace_back(cam->position + cam->viewportCorner + (cam->viewportRow*cam->h));
    corners.emplace_back(cam->position + cam->viewportCorner + (cam->viewportCol*cam->w));
    corners.emplace_back(cam->position + cam->viewportCorner + (cam->viewportCol*cam->w) + (cam->viewportRow*cam->h));
    char ch = 'A';
    for (Vec3 c: corners) {
        std::printf("%c = (%f, %f, %f)\n", ch, c.x, c.z, c.y);
        ch++;
    }
}

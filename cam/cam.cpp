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
void Camera::calculateViewport() {
    // Assume near frustum (viewport) is 1.f away from the camera. Makes things simpler!
    Vec3 frustumVec = {1.f, 0.f, 0.f};
    float halfViewportWidth = 1.f * std::tan(fov/2.f);
    float halfViewportHeight = (halfViewportWidth / float(w)) * float(h);
   
    // Calculate and store the bottom-left corner of the viewport
    viewportCorner = frustumVec;
    viewportCorner.z -= halfViewportWidth;
    viewportCorner.y -= halfViewportHeight;

    Mat3 rotZ = make_z_rotation(theta);
    Mat3 rotY = make_y_rotation(phi);
    
    viewportCorner =  rotY * (rotZ * viewportCorner);
    viewportCol = frustumVec;
    viewportCol.y -= halfViewportHeight;
    viewportCol = rotY * (rotZ * viewportCol);
    viewportCol = viewportCol - viewportCorner;
    // viewportCol - viewportCorner is now a vector along the viewport plane with width equivalent to w/2, so scale it down to 1px:
    viewportCol = 2.f * viewportCol / float(w);

    viewportRow = frustumVec;
    viewportRow.z -= halfViewportWidth;
    viewportRow = rotY * (rotZ * viewportRow);
    viewportRow = viewportRow - viewportCorner;

    // and the equivalent for the row vector:
    viewportRow = 2.f * viewportRow / float(h);
}

Camera::Camera(int width, int height, float fieldOfView, Vec3 pos) {
    w = width;
    h = height;
    fov = fieldOfView;
    position = pos;
    farFrustumDistance = 9999.f;
    rotate(0.f, 0.f);
}

void Camera::setDimensions(int width, int height) {
    w = width;
    h = height;
    calculateViewport();
}

void Camera::setDimensions(int width, int height, float fieldOfView) {
    w = width;
    h = height;
    fov = fieldOfView;
    calculateViewport();
}

void Camera::setFOV(float fieldOfView) {
    fov = fieldOfView;
    calculateViewport();
}

void Camera::rotateXRad(float thetaRad) {
    theta = thetaRad;
    calculateViewport();
}

void Camera::rotateX(float thetaDeg) {
    rotateXRad(thetaDeg * M_PI / 180.f);
}

void Camera::rotateYRad(float phiRad) {
    phi = phiRad;
    calculateViewport();
}

void Camera::rotateY(float phiDeg) {
    rotateYRad(phiDeg * M_PI / 180.f);
}

void Camera::rotateRad(float thetaRad, float phiRad) {
    theta = thetaRad;
    phi = phiRad;
    calculateViewport();
}

void Camera::rotate(float thetaDeg, float phiDeg) {
    rotateRad(thetaDeg * M_PI / 180.f, phiDeg * M_PI / 180.f);
}

void Camera::debugPrintCorners() {
    std::vector<Vec3> corners;
    corners.emplace_back(position);
    corners.emplace_back(position + viewportCorner);
    corners.emplace_back(position + viewportCorner + (viewportRow*h));
    corners.emplace_back(position + viewportCorner + (viewportCol*w));
    corners.emplace_back(position + viewportCorner + (viewportCol*w) + (viewportRow*h));
    char ch = 'A';
    for (Vec3 c: corners) {
        std::printf("%c = (%f, %f, %f)\n", ch, c.x, c.z, c.y);
        ch++;
    }
}

#ifndef CAM
#define CAM

#include "../vec/vec.hpp"

struct Camera {
    int w, h;
    float fov;
    float farFrustumDistance;
    float nearFrustumDistance;
    Vec3 viewportCorner;
    Vec3 viewportRow, viewportCol;
    float phi, theta; // angle of rotation, y and x.
    Vec3 position;
};

Camera newCamera(int w, int h, float fov, Vec3 pos);
void calculateViewport(Camera *cam);

void debugPrintCorners(Camera *cam);

#endif

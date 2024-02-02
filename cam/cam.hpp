#ifndef CAM
#define CAM

#include "../vec/vec.hpp"

class Camera {
    public:
        Camera(int width, int height, float fieldOfView, Vec3 pos);
        void calculateViewport();
        void debugPrintCorners();
        void setDimensions(int width, int height);
        void setDimensions(int width, int height, float fieldOfView);
        void setFOV(float fieldOfView);
        void rotateX(float thetaDeg);
        void rotateXRad(float thetaRad);
        void rotateY(float phiDeg);
        void rotateYRad(float phiRad);
        void rotate(float thetaDeg, float phiDeg);
        void rotateRad(float thetaRad, float phiRad);
        int w, h;
        float fov;
        float farFrustumDistance;
        Vec3 viewportCorner;
        Vec3 viewportRow, viewportCol;
        float phi, theta; // angle of rotation, around y and x, i.e. phi = left/right, theta = up/down.
        Vec3 position;
};

#endif

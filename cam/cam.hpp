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
        void rotateX(float thetaDeg);
        void rotateY(float phiDeg);
        void rotate(float thetaDeg, float phiDeg);
        int w, h;
        float fov;
        float farFrustumDistance;
        Vec3 viewportCorner;
        Vec3 viewportRow, viewportCol;
        float phi, theta; // angle of rotation, y and x.
        Vec3 position;
};

#endif

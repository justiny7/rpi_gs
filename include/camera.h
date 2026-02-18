#ifndef CAMERA_H
#define CAMERA_H

#include "types.h"
#include <stdint.h>

typedef struct {
    Vec3 pos;
    Vec3 target;
    Vec3 up;
    Mat4 w2c;
    float fx, fy;
    float cx, cy;
    uint32_t width, height;
} Camera;

void init_camera(Camera* c, Vec3 pos, Vec3 target, Vec3 up, uint32_t width, uint32_t height);
void project_point(Camera* c, Vec3 p, float* depth, float* u, float* v);
Mat3 quat_to_rotmat(Vec4 q);

#endif

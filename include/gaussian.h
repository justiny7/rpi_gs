#ifndef GAUSSIAN_H
#define GAUSSIAN_H

#include "types.h"
#include "camera.h"
#include <stdint.h>

// SH coefficients as static const to avoid QEMU .rodata loading issues
static const float SH_C0 = 0.28209479177387814f;
static const float SH_C1 = 0.4886025119029199f;

static const float SH_C2[] = {
    1.0925484305920792f,
    -1.0925484305920792f,
    0.31539156525252005f,
    -1.0925484305920792f,
    0.5462742152960396f
};
static const float SH_C3[] = {
    -0.5900435899266435f,
    2.890611442640554f,
    -0.4570457994644658f,
    0.3731763325901154f,
    -0.4570457994644658f,
    1.445305721320277f,
    -0.5900435899266435f
};

typedef struct {
    Vec3 pos;
    Vec3 scale;
    Vec4 rot;
    float opacity;
    Vec3 sh[16];
} Gaussian;

typedef struct {
    float screen_x, screen_y;
    float depth;
    Vec3 cov2d_inv;
    Vec3 color;
    float opacity;
} ProjectedGaussian;

Vec3 eval_sh(Vec3 pos, Vec3* sh, Vec3 cam_pos);
Mat3 compute_cov3d(Vec3 scale, Vec4 rot);
Vec3 project_cov2d(Vec3 pos, Mat3 cov3d, Mat4 w2c, float fx, float fy);
Vec3 compute_cov2d_inverse(Vec3 cov2d);
float eval_gaussian_2d(float px, float py, float cx, float cy, Vec3 cov_inv);

void precompute_gaussians(Camera* c, Gaussian* g, ProjectedGaussian* pg, uint32_t num_gaussians);

#endif

#include "types.h"
#include "math.h"

// Vec3
float vec3_len(Vec3 v) {
    return sqrtf(vec3_dot(v, v));
}
float vec3_dot(Vec3 a, Vec3 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

Vec3 vec3_add(Vec3 a, Vec3 b) {
    Vec3 res = { { a.x + b.x, a.y + b.y, a.z + b.z } };
    return res;
}
Vec3 vec3_sub(Vec3 a, Vec3 b) {
    Vec3 res = { { a.x - b.x, a.y - b.y, a.z - b.z } };
    return res;
}
Vec3 vec3_cross(Vec3 a, Vec3 b) {
    Vec3 res;
    res.x = a.y * b.z - a.z * b.y;
    res.y = a.z * b.x - a.x * b.z;
    res.z = a.x * b.y - a.y * b.x;
    return res;
}

Vec3 vec3_sadd(Vec3 v, float d) {
    Vec3 res = { { v.x + d, v.y + d, v.z + d } };
    return res;
}
Vec3 vec3_smul(Vec3 v, float d) {
    Vec3 res = { { v.x * d, v.y * d, v.z * d } };
    return res;
}
Vec3 vec3_sdiv(Vec3 v, float d) {
    Vec3 res = { { v.x / d, v.y / d, v.z / d } };
    return res;
}
Vec3 vec3_smin(Vec3 v, float d) {
    Vec3 res = { { min(v.x, d), min(v.y, d), min(v.z, d) } };
    return res;
}
Vec3 vec3_smax(Vec3 v, float d) {
    Vec3 res = { { max(v.x, d), max(v.y, d), max(v.z, d) } };
    return res;
}


// Vec4
float vec4_len(Vec4 v) {
    return sqrtf(vec4_dot(v, v));
}
float vec4_dot(Vec4 a, Vec4 b) {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

Vec4 vec4_add(Vec4 a, Vec4 b) {
    Vec4 res = { { a.x + b.x, a.y + b.y, a.z + b.z, a.w + b.w } };
    return res;
}
Vec4 vec4_sub(Vec4 a, Vec4 b) {
    Vec4 res = { { a.x - b.x, a.y - b.y, a.z - b.z, a.w - b.w } };
    return res;
}

Vec4 vec4_sadd(Vec4 v, float d) {
    Vec4 res = { { v.x + d, v.y + d, v.z + d, v.w + d } };
    return res;
}
Vec4 vec4_smul(Vec4 v, float d) {
    Vec4 res = { { v.x * d, v.y * d, v.z * d, v.w * d } };
    return res;
}
Vec4 vec4_sdiv(Vec4 v, float d) {
    Vec4 res = { { v.x / d, v.y / d, v.z / d, v.w / d } };
    return res;
}
Vec4 vec4_smin(Vec4 v, float d) {
    Vec4 res = { { min(v.x, d), min(v.y, d), min(v.z, d), min(v.w, d) } };
    return res;
}
Vec4 vec4_smax(Vec4 v, float d) {
    Vec4 res = { { max(v.x, d), max(v.y, d), max(v.z, d), max(v.w, d) } };
    return res;
}


// Mat3
Mat3 mat3_t(Mat3 m) {
    Mat3 res;
    res.m[0] = m.m[0];
    res.m[1] = m.m[3];
    res.m[2] = m.m[6];
    res.m[3] = m.m[1];
    res.m[4] = m.m[4];
    res.m[5] = m.m[7];
    res.m[6] = m.m[2];
    res.m[7] = m.m[5];
    res.m[8] = m.m[8];
    return res;
}

Mat3 mat3_mm(Mat3 a, Mat3 b) {
    Mat3 res;
    res.m[0] = a.m[0] * b.m[0] + a.m[1] * b.m[3] + a.m[2] * b.m[6];
    res.m[1] = a.m[0] * b.m[1] + a.m[1] * b.m[4] + a.m[2] * b.m[7];
    res.m[2] = a.m[0] * b.m[2] + a.m[1] * b.m[5] + a.m[2] * b.m[8];
    res.m[3] = a.m[3] * b.m[0] + a.m[4] * b.m[3] + a.m[5] * b.m[6];
    res.m[4] = a.m[3] * b.m[1] + a.m[4] * b.m[4] + a.m[5] * b.m[7];
    res.m[5] = a.m[3] * b.m[2] + a.m[4] * b.m[5] + a.m[5] * b.m[8];
    res.m[6] = a.m[6] * b.m[0] + a.m[7] * b.m[3] + a.m[8] * b.m[6];
    res.m[7] = a.m[6] * b.m[1] + a.m[7] * b.m[4] + a.m[8] * b.m[7];
    res.m[8] = a.m[6] * b.m[2] + a.m[7] * b.m[5] + a.m[8] * b.m[8];
    return res;
}


// Mat4
Mat4 mat4_mm(Mat4 a, Mat4 b) {
    Mat4 res;
    for (int i = 0; i < 16; res.m[i++] = 0.f);
    for (int k = 0; k < 4; k++) {
        for (int i = 0; i < 4; i++) {
            for (int j = 0; j < 4; j++) {
                res.rc[i][j] = a.rc[i][k] * b.rc[k][j];
            }
        }
    }
    return res;
}

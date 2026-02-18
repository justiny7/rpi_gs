#ifndef TYPES_H
#define TYPES_H

typedef union {
    struct { float x, y, z; };
    float m[3];
} Vec3;

typedef union {
    struct { float x, y, z, w; };
    float m[4];
} Vec4;

typedef union {
    float m[9];
    float rc[3][3];
    Vec3 rows[3];
} Mat3;

typedef union {
    float m[16];
    float rc[4][4];
    Vec3 rows[4];
} Mat4;


// Vec3
float vec3_len(Vec3 v);
float vec3_dot(Vec3 a, Vec3 b);

Vec3 vec3_add(Vec3 a, Vec3 b);
Vec3 vec3_sub(Vec3 a, Vec3 b);
Vec3 vec3_cross(Vec3 a, Vec3 b);

Vec3 vec3_sadd(Vec3 v, float d);
Vec3 vec3_smul(Vec3 v, float d);
Vec3 vec3_sdiv(Vec3 v, float d);
Vec3 vec3_smin(Vec3 v, float d);
Vec3 vec3_smax(Vec3 v, float d);

// Vec4
float vec4_len(Vec4 v);
float vec4_dot(Vec4 a, Vec4 b);

Vec4 vec4_add(Vec4 a, Vec4 b);
Vec4 vec4_sub(Vec4 a, Vec4 b);

Vec4 vec4_sadd(Vec4 v, float d);
Vec4 vec4_smul(Vec4 v, float d);
Vec4 vec4_sdiv(Vec4 v, float d);
Vec4 vec4_smin(Vec4 v, float d);
Vec4 vec4_smax(Vec4 v, float d);

// Mat3
Mat3 mat3_t(Mat3 m);
Mat3 mat3_mm(Mat3 a, Mat3 b);

// Mat4
Mat4 mat4_mm(Mat4 a, Mat4 b);

#endif

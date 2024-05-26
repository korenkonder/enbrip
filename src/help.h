/*
    by korenkonder
    GitHub/GitLab: korenkonder
*/

#pragma once

#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define X64 INTPTR_MAX == INT64_MAX
#define WIN WIN32 || _WIN32 || defined __CYGWIN__

#ifdef WIN
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE __attribute__((always_inline)) inline
#endif

#define free(ptr) if (ptr) { free((void*)(ptr)); (ptr) = 0; } else (ptr) = 0

#ifdef bool
#undef bool
#endif

typedef char bool;
#define true (1)
#define false (0)

#ifdef float_t
#undef float_t;
#endif
typedef float float_t;

#ifdef double_t
#undef double_t;
#endif
typedef double double_t;

#ifdef M_PI
#undef M_PI;
#endif
#define M_PI (3.14159265358979323846)

typedef struct {
    float x;
    float y;
    float z;
} vec3;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
} vec3i;

typedef struct {
    int32_t x;
    int32_t y;
    int32_t z;
    int32_t w;
} vec4i;

typedef struct {
    float x;
    float y;
    float z;
    float w;
} quat;

typedef struct {
    quat quat;
    vec3 trans;
    float time;
} quat_trans;

typedef enum {
    QUAT_TRANS_INTERP_NONE = 0,
    QUAT_TRANS_INTERP_LERP,
    QUAT_TRANS_INTERP_SLERP,
} quat_trans_interp_method;

static quat_trans quat_trans_identity = { { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, 0.0f };

extern float dot_quat(const quat* x, const quat* y);
extern float length_quat(const quat* x);
extern float length_squared_quat(const quat* x);
extern void normalize_quat(const quat* x, quat* z);
extern float lerpf(float x, float y, float blend);
extern void lerp_vec3(const vec3* x, const vec3* y, vec3* z, float blend);
extern void lerp_quat(const quat* x, const quat* y, quat* z, float blend);
extern void slerp_quat(const quat* x, const quat* y, quat* z, float blend);
extern void interp_quat_trans(const quat_trans* x, const quat_trans* y, quat_trans* z, float_t blend,
    quat_trans_interp_method quat_method, quat_trans_interp_method trans_method);

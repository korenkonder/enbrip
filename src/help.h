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

typedef struct
{
    float x;
    float y;
    float z;
} vec3;

typedef struct
{
    float x;
    float y;
    float z;
    float w;
} quat;

typedef struct
{
    quat quat;
    vec3 trans;
    float time;
} quat_trans;

static quat_trans quat_trans_identity = { { 0.0f, 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, 0.0f }, 0.0f };

extern float dot_quat(quat* x, quat* y);
extern float length_quat(quat* x);
extern float length_squared_quat(quat* x);
extern void normalize_quat(quat* x);
extern float lerpf(float x, float y, float blend);
extern void lerp_vec3(vec3* x, vec3* y, vec3* z, float blend);
extern void slerp_quat(quat* x, quat* y, quat* z, float blend);
extern void lerp_quat_trans(quat_trans* x, quat_trans* y, quat_trans* z, float blend);

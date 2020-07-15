/*
    by korenkonder
    GitHub/GitLab: korenkonder
*/

#include <math.h>
#include <stdlib.h>

typedef unsigned char bool;
typedef char sbyte;
typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;

#define FREE(ptr) if (ptr) free((void*)ptr); ptr = 0;

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
} vec4;

typedef struct
{
    vec4 quat;
    vec3 trans;
    float time;
} quat_trans_time;

typedef struct
{
    quat_trans_time qtt[2];
    vec4 quat;
    vec3 trans;
    byte flags;
    byte padding[3];
} trans_rot;

float dot_vec4(vec4* x, vec4* y);
float length_vec4(vec4* x);
float length_squared_vec4(vec4* x);
float lerpf(float x, float y, float blend);
void lerp_vec3(vec3* x, vec3* y, vec3* z, float blend);
void slerp(vec4* x, vec4* y, vec4* z, float blend);
void normalize_vec4(vec4* x);
void lerp_quat_trans_time(quat_trans_time* x, quat_trans_time* y, quat_trans_time* z, float blend);

const char* cant_allocate = "Can't allocate memory for ";

float dot_vec4(vec4* x, vec4* y)
{
    float z = x->x * y->x + x->y * y->y + x->z * y->z + x->w * y->w;
    return z;
}

float length_vec4(vec4* x)
{
    float z = x->x * x->x + x->y * x->y + x->z * x->z + x->w * x->w;
    return (float)sqrt(z);
}

float length_squared_vec4(vec4* x)
{
    float z = x->x * x->x + x->y * x->y + x->z * x->z + x->w * x->w;
    return z;
}

float lerpf(float x, float y, float blend)
{
    float b0, b1;
    b0 = blend;
    b1 = 1.0f - blend;
    return x * b1 + y * b0;
}

void lerp_vec3(vec3* x, vec3* y, vec3* z, float blend)
{
    float b0, b1;
    b0 = blend;
    b1 = 1.0f - blend;
    z->x = x->x * b1 + y->x * b0;
    z->y = x->y * b1 + y->y * b0;
    z->z = x->z * b1 + y->z * b0;
}

void slerp(vec4* x, vec4* y, vec4* z, float blend)
{
    if (length_squared_vec4(x) == 0.0f)
        if (length_squared_vec4(y) == 0.0f)
        { z->x = z->y = z->z = 0.0f; z->w = 1.0f; return; }
        else
        { z->x = y->x; z->y = y->y; z->z = y->z; z->w = y->w; return; }
    else if (length_squared_vec4(y) == 0.0f)
    { z->x = x->x; z->y = x->y; z->z = x->z; z->w = x->w; return; }

    float cosHalfAngle = dot_vec4(x, y);
    if (cosHalfAngle >= 1.0f || cosHalfAngle <= -1.0f)
    { z->x = x->x; z->y = x->y; z->z = x->z; z->w = x->w; return; }

    z->x = y->x; z->y = y->y; z->z = y->z; z->w = y->w;
    if (cosHalfAngle < 0.0f)
    { z->x = -z->x; z->y = -z->y; z->z = -z->z; z->w = -z->w; cosHalfAngle = -cosHalfAngle; }

    float blendA, blendB, halfAngle, sinHalfAngle, oneOverSinHalfAngle;
    if (cosHalfAngle < 0.99f)
    {
        halfAngle = acosf(cosHalfAngle);
        sinHalfAngle = sinf(halfAngle);
        oneOverSinHalfAngle = 1.0f / sinHalfAngle;
        blendA = sinf(halfAngle * (1.0f - blend)) * oneOverSinHalfAngle;
        blendB = sinf(halfAngle * blend) * oneOverSinHalfAngle;
    }
    else
    { blendA = 1.0f - blend; blendB = blend; }

    z->x = x->x * blendA + z->x * blendB;
    z->y = x->y * blendA + z->y * blendB;
    z->z = x->z * blendA + z->z * blendB;
    z->w = x->w * blendA + z->w * blendB;
    normalize_vec4(z);
}

void normalize_vec4(vec4* x)
{
    float length = length_vec4(x);
    if (length == 0) length = 1.0f;
    else length = 1.0f / length;
    x->x *= length;
    x->y *= length;
    x->z *= length;
    x->w *= length;
}

void lerp_quat_trans_time(quat_trans_time* x, quat_trans_time* y, quat_trans_time* z, float blend)
{
         if (blend > 1.0f) blend = 1.0f;
    else if (blend < 0.0f) blend = 0.0f;
    slerp(&x->quat, &y->quat, &z->quat, blend);
    lerp_vec3(&x->trans, &y->trans, &z->trans, blend);
    z->time = lerpf(x->time, y->time, blend);
}

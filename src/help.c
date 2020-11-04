/*
    by korenkonder
    GitHub/GitLab: korenkonder
*/

#include "help.h"

float dot_quat(quat* x, quat* y) {
    float z = x->x * y->x + x->y * y->y + x->z * y->z + x->w * y->w;
    return z;
}

float length_quat(quat* x) {
    float z = x->x * x->x + x->y * x->y + x->z * x->z + x->w * x->w;
    return sqrtf(z);
}

float length_squared_quat(quat* x) {
    float z = x->x * x->x + x->y * x->y + x->z * x->z + x->w * x->w;
    return z;
}

void normalize_quat(quat* x) {
    float length = length_quat(x);
    if (length != 0)
        length = 1.0f / length;

    x->x *= length;
    x->y *= length;
    x->z *= length;
    x->w *= length;
}

float lerpf(float x, float y, float blend) {
    float b0, b1;
    b0 = blend;
    b1 = 1.0f - blend;
    return x * b1 + y * b0;
}

void lerp_vec3(vec3* x, vec3* y, vec3* z, float blend) {
    float b0, b1;
    b0 = blend;
    b1 = 1.0f - blend;
    z->x = x->x * b1 + y->x * b0;
    z->y = x->y * b1 + y->y * b0;
    z->z = x->z * b1 + y->z * b0;
}

void slerp_quat(quat* x, quat* y, quat* z, float blend) {
    normalize_quat(x);
    normalize_quat(y);

    float dot = dot_quat(x, y);
    if (dot < 0.0f) {
        z->x = -y->x;
        z->y = -y->y;
        z->z = -y->z;
        z->w = -y->w;
        dot = -dot;
    }
    else
        *z = *y;

    const float DOT_THRESHOLD = 0.9995f;
    float s0, s1;
    if (dot <= DOT_THRESHOLD) {
        float theta_0 = acosf(dot);
        float theta = theta_0 * blend;
        float sin_theta = sinf(theta);
        float sin_theta_0 = sinf(theta_0);

        s0 = cosf(theta) - dot * sin_theta / sin_theta_0;
        s1 = sin_theta / sin_theta_0;
    }
    else {
        s0 = (1.0f - blend);
        s1 = blend;
    }
    z->x = s0 * x->x + s1 * z->x;
    z->y = s0 * x->y + s1 * z->y;
    z->z = s0 * x->z + s1 * z->z;
    z->w = s0 * x->w + s1 * z->w;
    normalize_quat(z);
}

void lerp_quat_trans(quat_trans* x, quat_trans* y, quat_trans* z, float blend)
{
    if (blend > 1.0f)
        blend = 1.0f;
    else if (blend < 0.0f)
        blend = 0.0f;

    slerp_quat(&x->quat, &y->quat, &z->quat, blend);
    lerp_vec3(&x->trans, &y->trans, &z->trans, blend);
    z->time = lerpf(x->time, y->time, blend);
}

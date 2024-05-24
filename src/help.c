/*
    by korenkonder
    GitHub/GitLab: korenkonder
*/

#include "help.h"

float dot_quat(const quat* x, const quat* y) {
    float z = x->x * y->x + x->y * y->y + x->z * y->z + x->w * y->w;
    return z;
}

float length_quat(const quat* x) {
    float z = x->x * x->x + x->y * x->y + x->z * x->z + x->w * x->w;
    return sqrtf(z);
}

float length_squared_quat(const quat* x) {
    float z = x->x * x->x + x->y * x->y + x->z * x->z + x->w * x->w;
    return z;
}

void normalize_quat(const quat* x, quat* z) {
    float length = length_quat(x);
    if (length != 0)
        length = 1.0f / length;

    z->x = x->x * length;
    z->y = x->y * length;
    z->z = x->z * length;
    z->w = x->w * length;
}

float lerpf(float x, float y, float blend) {
    float b0, b1;
    b0 = blend;
    b1 = 1.0f - blend;
    return x * b1 + y * b0;
}

void lerp_vec3(const vec3* x, const vec3* y, vec3* z, float blend) {
    float b0, b1;
    b0 = blend;
    b1 = 1.0f - blend;
    z->x = x->x * b1 + y->x * b0;
    z->y = x->y * b1 + y->y * b0;
    z->z = x->z * b1 + y->z * b0;
}

void lerp_quat(const quat* x, const quat* y, quat* z, float blend) {
    float b0, b1;
    b0 = blend;
    b1 = 1.0f - blend;
    z->x = x->x * b1 + y->x * b0;
    z->y = x->y * b1 + y->y * b0;
    z->z = x->z * b1 + y->z * b0;
    z->w = x->w * b1 + y->w * b0;
}

void slerp_quat(const quat* x, const quat* y, quat* z, float blend) {
    quat x_temp, y_temp, z_temp;
    normalize_quat(x, &x_temp);
    normalize_quat(y, &y_temp);

    float dot = dot_quat(&x_temp, &y_temp);
    if (dot < 0.0f) {
        y_temp.x = -y_temp.x;
        y_temp.y = -y_temp.y;
        y_temp.z = -y_temp.z;
        y_temp.w = -y_temp.w;
        dot = -dot;
    }

    if (1.0 - dot <= 0.08f) {
        lerp_quat(&x_temp, &y_temp, z, blend);
        return;
    }

    if (dot > 1.0f)
        dot = 1.0f;

    float_t theta = acosf(dot);
    if (theta == 0.0f) {
        *z = x_temp;
        return;
    }

    float_t st = 1.0f / sinf(theta);
    float_t s0 = sinf((1.0f - blend) * theta) * st;
    float_t s1 = sinf(theta * blend) * st;
    z_temp.x = s0 * x_temp.x + s1 * y_temp.x;
    z_temp.y = s0 * x_temp.y + s1 * y_temp.y;
    z_temp.z = s0 * x_temp.z + s1 * y_temp.z;
    z_temp.w = s0 * x_temp.w + s1 * y_temp.w;
    normalize_quat(&z_temp, z);
}

void interp_quat_trans(quat_trans* x, quat_trans* y,
    quat_trans* z, float_t blend, quat_trans_interp_method method) {
    if (blend > 1.0f)
        blend = 1.0f;
    else if (blend < 0.0f)
        blend = 0.0f;

    switch (method) {
    case QUAT_TRANS_INTERP_NONE:
        z->quat = y->quat;
        z->trans = y->trans;
        z->time = lerpf(x->time, y->time, blend);
        break;
    case QUAT_TRANS_INTERP_LERP:
        lerp_quat(&x->quat, &y->quat, &z->quat, blend);
        lerp_vec3(&x->trans, &y->trans, &z->trans, blend);
        z->time = lerpf(x->time, y->time, blend);
        break;
    case QUAT_TRANS_INTERP_SLERP:
        slerp_quat(&x->quat, &y->quat, &z->quat, blend);
        lerp_vec3(&x->trans, &y->trans, &z->trans, blend);
        z->time = lerpf(x->time, y->time, blend);
        break;
    }
}

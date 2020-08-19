/*
    by korenkonder
    GitHub/GitLab: korenkonder
*/

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

// Helper part

#define X64 INTPTR_MAX == INT64_MAX
#define WIN WIN32 || _WIN32 || defined __CYGWIN__

#ifdef WIN
#define FORCE_INLINE __forceinline
#else
#define FORCE_INLINE __attribute__((always_inline)) inline
#endif

#define free(ptr) if (ptr) free((void*)ptr); ptr = 0;

#ifdef bool
#undef bool
#endif

typedef char bool;
#define true 1
#define false 0

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

const char* is_null = "\"%s\" is null\n";
const char* cant_allocate = "Can't allocate memory for \"%s\"\n";
const char* cant_allocate_inner = "Can't allocate memory for %s\n";

FORCE_INLINE float dot_quat(quat* x, quat* y) {
    float z = x->x * y->x + x->y * y->y + x->z * y->z + x->w * y->w;
    return z;
}

FORCE_INLINE float length_quat(quat* x) {
    float z = x->x * x->x + x->y * x->y + x->z * x->z + x->w * x->w;
    return sqrtf(z);
}

FORCE_INLINE float length_squared_quat(quat* x) {
    float z = x->x * x->x + x->y * x->y + x->z * x->z + x->w * x->w;
    return z;
}

FORCE_INLINE void normalize_quat(quat* x) {
    float length = length_quat(x);
    if (length != 0)
        length = 1.0f / length;

    x->x *= length;
    x->y *= length;
    x->z *= length;
    x->w *= length;
}

FORCE_INLINE float lerpf(float x, float y, float blend) {
    float b0, b1;
    b0 = blend;
    b1 = 1.0f - blend;
    return x * b1 + y * b0;
}

FORCE_INLINE void lerp_vec3(vec3* x, vec3* y, vec3* z, float blend) {
    float b0, b1;
    b0 = blend;
    b1 = 1.0f - blend;
    z->x = x->x * b1 + y->x * b0;
    z->y = x->y * b1 + y->y * b0;
    z->z = x->z * b1 + y->z * b0;
}

void slerp_quat(quat* x, quat* y, quat* z, float blend) {
    if (length_squared_quat(x) == 0.0f) {
        if (length_squared_quat(y) == 0.0f) {
            z->x = z->y = z->z = 0.0f;
            z->w = 1.0f;
        }
        else
            *z = *y;
        return;
    }
    else if (length_squared_quat(y) == 0.0f) {
        *z = *x;
        return;
    }

    float cos_half_angle = dot_quat(x, y);
    if (cos_half_angle >= 1.0f || cos_half_angle <= -1.0f) {
        *z = *x;
        return;
    }

    *z = *y;

    if (cos_half_angle < 0.0f) {
        z->x = -z->x;
        z->y = -z->y;
        z->z = -z->z;
        z->w = -z->w;
        cos_half_angle = -cos_half_angle;
    }

    float b[2], half_angle, sin_half_angle, one_over_sin_half_angle;
    if (cos_half_angle < 0.99f) {
        half_angle = acosf(cos_half_angle);
        sin_half_angle = sinf(half_angle);
        one_over_sin_half_angle = 1.0f / sin_half_angle;
        b[0] = sinf(half_angle * (1.0f - blend)) * one_over_sin_half_angle;
        b[1] = sinf(half_angle * blend) * one_over_sin_half_angle;
    }
    else {
        b[0] = 1.0f - blend;
        b[1] = blend;
    }

    z->x = x->x * b[0] + z->x * b[1];
    z->y = x->y * b[0] + z->y * b[1];
    z->z = x->z * b[0] + z->z * b[1];
    z->w = x->w * b[0] + z->w * b[1];
    normalize_quat(z);
}

FORCE_INLINE void lerp_quat_trans(quat_trans* x, quat_trans* y, quat_trans* z, float blend)
{
    if (blend > 1.0f)
        blend = 1.0f;
    else if (blend < 0.0f)
        blend = 0.0f;

    slerp_quat(&x->quat, &y->quat, &z->quat, blend);
    lerp_vec3(&x->trans, &y->trans, &z->trans, blend);
    z->time = lerpf(x->time, y->time, blend);
}

// Enbaya part

typedef struct {
    quat_trans qt[2];
    quat quat;
    vec3 trans;
    uint8_t flags;
    uint8_t padding[3];
} enb_track;

typedef struct {
    uint32_t signature;                   // 0x00
    uint32_t track_count;                 // 0x04
    float scale;                          // 0x08
    float duration;                       // 0x0C
    uint32_t samples;                     // 0x10
    uint32_t track_data_init_mode_length; // 0x14
    uint32_t track_data_init_i8_length;   // 0x18
    uint32_t track_data_init_i16_length;  // 0x1C
    uint32_t track_data_init_i32_length;  // 0x20
    uint32_t track_data_mode_length;      // 0x24
    uint32_t track_data_mode2_length;     // 0x28
    uint32_t track_data_i8_length;        // 0x2C
    uint32_t track_data_i16_length;       // 0x30
    uint32_t track_data_i32_length;       // 0x44
    uint32_t params_mode_length;          // 0x38
    uint32_t params_u8_length;            // 0x3C
    uint32_t params_u16_length;           // 0x40
    uint32_t params_u32_length;           // 0x44
    uint32_t track_flags_length;          // 0x48
    uint32_t unknown;                     // In runtime becomes pointer to data after this uint32_t
} enb_head;

typedef struct {
    int32_t current_sample;             // 0x00
    float current_sample_time;          // 0x04
    float previous_sample_time;         // 0x08
    enb_head* data_header;              // 0x0C
    enb_track* track_data;              // 0x10
    uint32_t data_length;               // 0x14
    uint32_t unknown[2];                // 0x18
    float requested_time;               // 0x20
    float seconds_per_sample;           // 0x24
    uint32_t next_params_change;        // 0x28
    uint32_t prev_params_change;        // 0x2C
    uint8_t* track_flags;               // 0x30
    uint8_t* track_data_init_mode;      // 0x34
    int8_t* track_data_init_i8;         // 0x38
    int16_t* track_data_init_i16;       // 0x3C
    int32_t* track_data_init_i32;       // 0x40
    uint8_t track_data_init_counter;    // 0x44
    uint8_t padding[3];                 // 0x45
    uint8_t* track_data_mode;           // 0x48
    uint8_t* track_data_mode2;          // 0x4C
    int8_t* track_data_i8;              // 0x50
    int16_t* track_data_i16;            // 0x54
    int32_t* track_data_i32;            // 0x58
    uint8_t track_data_mode_counter;    // 0x5C
    uint8_t track_data_mode2_counter;   // 0x5D
    uint8_t padding2[2];                // 0x5E
    uint8_t* params_mode;               // 0x60
    uint8_t* params_u8;                 // 0x64
    uint16_t* params_u16;               // 0x68
    uint32_t* params_u32;               // 0x6C
    uint8_t params_counter;             // 0x70
    uint8_t padding3[3];                // 0x71
    uint8_t* orig_track_data_init_mode; // 0x74
    int8_t* orig_track_data_init_i8;    // 0x78
    int16_t* orig_track_data_init_i16;  // 0x7C
    int32_t* orig_track_data_init_i32;  // 0x80
    uint8_t* orig_track_data_mode;      // 0x84
    uint8_t* orig_track_data_mode2;     // 0x88
    int8_t* orig_track_data_i8;         // 0x8C
    int16_t* orig_track_data_i16;       // 0x90
    int32_t* orig_track_data_i32;       // 0x94
    uint8_t* orig_params_mode;          // 0x98
    uint8_t* orig_params_u8;            // 0x9C
    uint16_t* orig_params_u16;          // 0xA0
    uint32_t* orig_params_u32;          // 0xA4
    uint8_t track_mode_selector;        // 0xA8
    uint8_t track_data_selector;        // 0xA9
    uint8_t padding4[6];                // 0xAA
} enb_play_head;

extern int32_t enb_process(uint8_t* data_in, uint8_t** data_out,
    size_t* data_out_len, float* duration, float* fps, size_t* frames);
extern int32_t enb_initialize(uint8_t* data, enb_play_head** play_head);
extern void enb_free(enb_play_head** play_head);
extern void enb_get_track_data(enb_play_head* play_head, size_t track, quat_trans* data);
extern void enb_set_time(enb_play_head* play_head, float time);

const uint8_t shift_table_1[] = { 6, 4, 2, 0 }; // 0x08BF1CE8, 0x08BF2160, 0x08BF210
const uint8_t shift_table_2[] = { 4, 0 };       // 0x08BF1CF8
const int8_t value_table_1[] = { 0, 1, 0, -1 }; // 0x08BB3FC0
const int8_t value_table_2[] = { 0, 8, 2, 3, 4, 5, 6, 7, -8, -7, -6, -5, -4, -3, -2, -9 }; // 0x08BB3FD0

static void enb_init(enb_play_head* play_head, enb_head* head);
static void enb_copy_pointers(enb_play_head* play_head);
static void enb_get_track_unscaled_init(enb_play_head* play_head);
static void enb_get_track_unscaled_forward(enb_play_head* play_head);
static void enb_get_track_unscaled_backward(enb_play_head* play_head);
static void enb_calc_params_init(enb_play_head* play_head);
static void enb_calc_params_forward(enb_play_head* play_head);
static void enb_calc_params_backward(enb_play_head* play_head);
static void enb_calc_track_init(enb_play_head* play_head);
static void enb_calc_track(enb_play_head* play_head, float time, bool forward);

int32_t enb_process(uint8_t* data_in, uint8_t** data_out,
    size_t* data_out_len, float* duration, float* fps, size_t* frames) {
    enb_play_head* play_head;
    enb_head* head;
    quat_trans* qt_data;
    size_t i, j;
    int32_t code;

    if (!data_in)
        return -1;
    else if (!data_out)
        return -2;
    else if (!data_out_len)
        return -3;
    else if (!duration)
        return -4;
    else if (!fps)
        return -5;
    else if (!frames)
        return -6;

    code = enb_initialize(data_in, &play_head);
    if (code)
    {
        free(*data_out);
        return code - 0x10;
    }

    head = (enb_head*)data_in;
    *duration = head->duration;

    if (*fps > 600.0f)
        *fps = 600.0f;
    else if (*fps < (float)head->samples)
        *fps = (float)head->samples;

    float frames_float = *duration * *fps;
    *frames = (size_t)frames_float + (fmodf(frames_float, 1.0f) >= 0.5f) + 1;
    if (*frames > 0x7FFFFFFFU)
        return -7;

    *data_out_len = sizeof(quat_trans) * head->track_count * *frames + 0x10;
    *data_out = (uint8_t*)malloc(*data_out_len);

    if (!*data_out)
        return -8;

    memset((void*)*data_out, 0, *data_out_len);

    ((uint32_t*)*data_out)[0] = head->track_count;
    ((uint32_t*)*data_out)[1] = *(uint32_t*)frames;
    ((float*)*data_out)[2] = *fps;
    ((float*)*data_out)[3] = *duration;

    qt_data = (quat_trans*)(*data_out + 0x10);
    for (i = 0; i < *frames; i++) {
        enb_set_time(play_head, (float)i / *fps);

        for (j = 0; j < head->track_count; j++, qt_data++)
            enb_get_track_data(play_head, j, qt_data);
    }
    enb_free(&play_head);
    return 0;
}

int32_t enb_initialize(uint8_t* data, enb_play_head** play_head) {
    if (!data)
        return -1;
    else if (!play_head)
        return -2;
    *play_head = 0;

    enb_head* head = (enb_head*)data;
    if (head->signature != 0x100A9DA4 && head->signature != 0x100AAD74)
        return -3;

    enb_play_head* ph = (enb_play_head*)malloc(sizeof(enb_play_head));
    if (!ph)
        return -4;

    memset((void*)ph, 0, sizeof(enb_play_head));

    ph->data_header = head;
    enb_init(ph, head);

    ph->track_data = (enb_track*)malloc(sizeof(enb_track) * head->track_count);

    if (!ph->track_data) {
        free(ph);
        return -5;
    }

    memset((void*)ph->track_data, 0, sizeof(enb_track) * head->track_count);

    *play_head = ph;
    return 0;
}

void enb_free(enb_play_head** play_head) {
    if (!play_head || !*play_head)
        return;

    free((*play_head)->track_data);
    free(*play_head);
    *play_head = (enb_play_head*)0;
}

void enb_get_track_data(enb_play_head* play_head, size_t track, quat_trans* data) {
    if (!data)
        return;
    else if (!play_head || play_head->data_header->track_count < track) {
        *data = quat_trans_identity;
        return;
    }

    if (play_head->track_mode_selector) {
        quat_trans* qt1 = play_head->track_data[track].qt + (play_head->track_data_selector & 0x1);
        quat_trans* qt2 = play_head->track_data[track].qt + ((play_head->track_data_selector & 0x1) ^ 1);
        float blend = (play_head->requested_time - play_head->previous_sample_time)
            / (play_head->current_sample_time - play_head->previous_sample_time);
        lerp_quat_trans(qt1, qt2, data, blend);
    }
    else
        *data = *play_head->track_data[track].qt;
}

static void enb_init(enb_play_head* play_head, enb_head* head) { // 0x08A08050 in ULJM05681
    uint8_t* data;
    uint32_t temp;

    data = (uint8_t*)head;
    play_head->current_sample = -1;
    play_head->current_sample_time = -1.0f;
    play_head->previous_sample_time = -1.0f;
    play_head->requested_time = -1.0f;
    play_head->seconds_per_sample = 1.0f / (float)head->samples;
    play_head->track_mode_selector = 0;

    temp = 0x50;
    play_head->orig_track_data_init_i32 = (int32_t*)(data + temp);

    temp += head->track_data_init_i32_length;
    play_head->orig_track_data_i32 = (int32_t*)(data + temp);

    temp += head->track_data_i32_length;
    play_head->orig_params_u32 = (uint32_t*)(data + temp);

    temp += head->params_u32_length;
    play_head->orig_track_data_init_i16 = (int16_t*)(data + temp);

    temp += head->track_data_init_i16_length;
    play_head->orig_track_data_i16 = (int16_t*)(data + temp);

    temp += head->track_data_i16_length;
    play_head->orig_params_u16 = (uint16_t*)(data + temp);

    temp += head->params_u16_length;
    play_head->orig_track_data_init_mode = data + temp;

    temp += head->track_data_init_mode_length;
    play_head->orig_track_data_init_i8 = (int8_t*)(data + temp);

    temp += head->track_data_init_i8_length;
    play_head->orig_track_data_mode = data + temp;

    temp += head->track_data_mode_length;
    play_head->orig_track_data_mode2 = data + temp;

    temp += head->track_data_mode2_length;
    play_head->orig_track_data_i8 = (int8_t*)(data + temp);

    temp += head->track_data_i8_length;
    play_head->orig_params_mode = data + temp;

    temp += head->params_mode_length;
    play_head->orig_params_u8 = data + temp;

    temp += head->params_u8_length;
    play_head->track_flags = data + temp;

    temp += head->track_flags_length;
    play_head->data_length = temp;

    enb_copy_pointers(play_head);
}

static void enb_copy_pointers(enb_play_head* play_head) { // 0x08A07FD0 in ULJM05681
    play_head->track_data_init_mode = play_head->orig_track_data_init_mode;
    play_head->track_data_init_i8 = play_head->orig_track_data_init_i8;
    play_head->track_data_init_i16 = play_head->orig_track_data_init_i16;
    play_head->track_data_init_i32 = play_head->orig_track_data_init_i32;
    play_head->track_data_init_counter = 0;

    play_head->track_data_mode = play_head->orig_track_data_mode;
    play_head->track_data_mode2 = play_head->orig_track_data_mode2;
    play_head->track_data_i8 = play_head->orig_track_data_i8;
    play_head->track_data_i16 = play_head->orig_track_data_i16;
    play_head->track_data_i32 = play_head->orig_track_data_i32;
    play_head->track_data_mode_counter = 0;
    play_head->track_data_mode2_counter = 0;

    play_head->params_mode = play_head->orig_params_mode;
    play_head->params_u8 = play_head->orig_params_u8;
    play_head->params_u16 = play_head->orig_params_u16;
    play_head->params_u32 = play_head->orig_params_u32;
    play_head->params_counter = 0;
}

void enb_set_time(enb_play_head* play_head, float time) { // 0x08A0876C in ULJM05681
    float requested_time;
    float sps; // seconds per sample
    uint32_t mode;

    if (time == play_head->requested_time)
        return;

    requested_time = play_head->requested_time;
    sps = play_head->seconds_per_sample;

    if ((requested_time == -1.0f) || (0.000001f > time) || (requested_time - time > time)
        || ((sps <= requested_time) && (sps > time))) {
        play_head->current_sample = 0;
        play_head->current_sample_time = 0.0f;
        play_head->previous_sample_time = 0.0f;
        play_head->track_mode_selector = 0;

        enb_copy_pointers(play_head);
        enb_calc_params_init(play_head);
        enb_get_track_unscaled_init(play_head);
        enb_calc_track_init(play_head);
    }

    play_head->requested_time = time;
    if (time < 0.000001f)
        return;

    while ((time > play_head->current_sample_time)
        && (play_head->data_header->duration - play_head->current_sample_time > 0.00001f)) {
        if (play_head->track_mode_selector == 2) {
            if (play_head->params_counter == 4) {
                play_head->params_counter = 0;
                play_head->params_mode++;
            }

            mode = *play_head->params_mode >> shift_table_1[play_head->params_counter++];
            mode &= 0x3;

            switch (mode) {
            case 1:
                play_head->params_u8++;
                break;
            case 2:
                play_head->params_u16++;
                break;
            case 3:
                play_head->params_u32++;
                break;
            }
            play_head->track_mode_selector = 1;
        }
        else if (play_head->current_sample > 0) {
            enb_calc_params_forward(play_head);
            play_head->track_mode_selector = 1;
        }

        enb_get_track_unscaled_forward(play_head);
        requested_time = ++play_head->current_sample * sps;

        if (play_head->data_header->duration <= requested_time)
            requested_time = play_head->data_header->duration;

        enb_calc_track(play_head, requested_time, true);
        play_head->current_sample_time = play_head->current_sample * sps;
        play_head->previous_sample_time = (play_head->current_sample - 1) * sps;
    }

    while (time < play_head->previous_sample_time) {
        if (play_head->track_mode_selector == 1) {
            if (--play_head->params_counter == 0xFF) {
                play_head->params_counter = 3;
                play_head->params_mode--;
            }

            mode = *play_head->params_mode >> shift_table_1[play_head->params_counter];
            mode &= 0x3;

            switch (mode) {
            case 1:
                play_head->params_u8--;
                break;
            case 2:
                play_head->params_u16--;
                break;
            case 3:
                play_head->params_u32--;
                break;
            }
            play_head->track_mode_selector = 2;
        }
        else
            enb_calc_params_backward(play_head);

        play_head->current_sample--;
        enb_get_track_unscaled_backward(play_head);
        play_head->current_sample_time = play_head->current_sample * sps;
        play_head->previous_sample_time = (play_head->current_sample - 1) * sps;
        enb_calc_track(play_head, play_head->previous_sample_time, false);
    }
}

static void enb_get_track_unscaled_init(enb_play_head* play_head) { // 0x08A08D3C in ULJM05681
    uint32_t i, j, mode;
    int32_t val;

    enb_track* track_data = play_head->track_data;

    for (i = 0; i < play_head->data_header->track_count; i++, track_data++) {
        for (j = 0; j < 7; j++) {
            if (play_head->track_data_init_counter == 4) {
                play_head->track_data_init_counter = 0;
                play_head->track_data_init_mode++;
            }

            mode = *play_head->track_data_init_mode >> shift_table_1[play_head->track_data_init_counter++];
            mode &= 0x3;

            val = 0;
            switch (mode) {
            case 1:
                val = *play_head->track_data_init_i8++;
                break;
            case 2:
                val = *play_head->track_data_init_i16++;
                break;
            case 3:
                val = *play_head->track_data_init_i32++;
                break;
            }

            switch (j) {
            case 0:
                track_data->quat.x = (float)val;
                break;
            case 1:
                track_data->quat.y = (float)val;
                break;
            case 2:
                track_data->quat.z = (float)val;
                break;
            case 3:
                track_data->quat.w = (float)val;
                break;
            case 4:
                track_data->trans.x = (float)val;
                break;
            case 5:
                track_data->trans.y = (float)val;
                break;
            case 6:
                track_data->trans.z = (float)val;
                break;
            }
        }
    }
    play_head->current_sample = 0;
}

static void enb_get_track_unscaled_forward(enb_play_head* play_head) { // 0x08A08E7C in ULJM05681
    uint32_t i, j;
    int32_t val;

    enb_track* track_data = play_head->track_data;

    for (i = 0; i < play_head->data_header->track_count; i++, track_data++) {
        if (track_data->flags == 0)
            continue;

        for (j = 0; j < 7; j++) {
            if ((track_data->flags & (1 << j)) == 0)
                continue;

            if (play_head->track_data_mode_counter == 4) {
                play_head->track_data_mode_counter = 0;
                play_head->track_data_mode++;
            }

            val = *play_head->track_data_mode >> shift_table_1[play_head->track_data_mode_counter++];
            val &= 0x3;

            if (val == 2) {
                if (play_head->track_data_mode2_counter == 2) {
                    play_head->track_data_mode2_counter = 0;
                    play_head->track_data_mode2++;
                }

                val = *play_head->track_data_mode2 >> shift_table_2[play_head->track_data_mode2_counter++];
                val &= 0xF;

                if (val == 0) {
                    val = *play_head->track_data_i8;
                    play_head->track_data_i8++;
                    if (val == 0) {
                        val = *play_head->track_data_i16++;
                        if (val == 0)
                            val = *play_head->track_data_i32++;
                    }
                    else if ((val > 0) && (val < 9))
                        val += 0x7f;
                    else if ((val > -9) && (val < 0))
                        val -= 0x80;
                }
                else
                    val = value_table_2[val];
            }
            else
                val = value_table_1[val];

            switch (j) {
            case 0:
                track_data->quat.x += val;
                break;
            case 1:
                track_data->quat.y += val;
                break;
            case 2:
                track_data->quat.z += val;
                break;
            case 3:
                track_data->quat.w += val;
                break;
            case 4:
                track_data->trans.x += val;
                break;
            case 5:
                track_data->trans.y += val;
                break;
            case 6:
                track_data->trans.z += val;
                break;
            }
        }
    }
}

static void enb_get_track_unscaled_backward(enb_play_head* play_head) { // 0x08A090A0 in ULJM05681
    uint32_t i, j;
    int32_t val;

    enb_track* track_data = play_head->track_data;

    track_data += (size_t)play_head->data_header->track_count - 1;
    for (i = play_head->data_header->track_count - 1; i >= 0; i--, track_data--) {
        if (track_data->flags == 0)
            continue;

        for (j = 6; j >= 0; j--) {
            if ((track_data->flags & (1 << j)) == 0)
                continue;

            if (--play_head->track_data_mode_counter == 0xFF) {
                play_head->track_data_mode_counter = 3;
                play_head->track_data_mode--;
            }

            val = *play_head->track_data_mode >> shift_table_1[play_head->track_data_mode_counter];
            val &= 0x3;

            if (val == 2) {
                if (--play_head->track_data_mode2_counter == 0xFF) {
                    play_head->track_data_mode2_counter = 1;
                    play_head->track_data_mode2--;
                }

                val = *play_head->track_data_mode2 >> shift_table_2[play_head->track_data_mode2_counter];
                val &= 0xF;

                if (val == 0) {
                    val = *--play_head->track_data_i8;
                    if (val == 0) {
                        val = *--play_head->track_data_i16;
                        if (val == 0)
                            val = *--play_head->track_data_i32;
                    }
                    else if ((val > 0) && (val < 9))
                        val += 0x7f;
                    else if ((val > -9) && (val < 0))
                        val -= 0x80;
                }
                else
                    val = value_table_2[val];
            }
            else
                val = value_table_1[val];

            switch (j) {
            case 0:
                track_data->quat.x -= val;
                break;
            case 1:
                track_data->quat.y -= val;
                break;
            case 2:
                track_data->quat.z -= val;
                break;
            case 3:
                track_data->quat.w -= val;
                break;
            case 4:
                track_data->trans.x -= val;
                break;
            case 5:
                track_data->trans.y -= val;
                break;
            case 6:
                track_data->trans.z -= val;
                break;
            }
        }
    }
}

static void enb_calc_params_init(enb_play_head* play_head) { // 0x08A0931C in ULJM05681
    uint32_t mode, val;

    if (play_head->params_counter == 4) {
        play_head->params_counter = 0;
        play_head->params_mode++;
    }

    mode = *play_head->params_mode >> shift_table_1[play_head->params_counter++];
    mode &= 0x3;

    val = 0;
    switch (mode) {
    case 1:
        val = *play_head->params_u8++;
        break;
    case 2:
        val = *play_head->params_u16++;
        break;
    case 3:
        val = *play_head->params_u32++;
        break;
    }
    play_head->next_params_change = val;
    play_head->prev_params_change = 0;
}

static void enb_calc_params_forward(enb_play_head* play_head) { // 0x08A09404 in ULJM05681
    uint32_t i, j, mode, temp, track_params_count, val;

    enb_track* track_data = play_head->track_data;

    track_params_count = play_head->data_header->track_count * 7;
    i = 0;
    while (i < track_params_count) {
        j = play_head->next_params_change;
        if (j == 0) {
            track_data[i / 7].flags ^= 1 << (i % 7);
            if (play_head->params_counter == 4) {
                play_head->params_counter = 0;
                play_head->params_mode++;
            }

            mode = *play_head->params_mode >> shift_table_1[play_head->params_counter++];
            mode &= 0x3;

            val = 0;
            switch (mode) {
            case 1:
                val = *play_head->params_u8++;
                break;
            case 2:
                val = *play_head->params_u16++;
                break;
            case 3:
                val = *play_head->params_u32++;
                break;
            }
            play_head->next_params_change = val;
            play_head->prev_params_change = 0;
            i++;
        }
        else {
            temp = j < (uint32_t)(track_params_count - i) ? j : (uint32_t)(track_params_count - i);
            i += (int32_t)temp;
            play_head->next_params_change -= temp;
            play_head->prev_params_change += temp;
        }
    }
}

static void enb_calc_params_backward(enb_play_head* play_head) { // 0x08A0968C in ULJM05681
    uint32_t i, j, mode, temp, track_params_count, val;

    enb_track* track_data = play_head->track_data;

    track_params_count = play_head->data_header->track_count * 7;
    i = track_params_count - 1;
    while (i != 0xFFFFFFFFU) {
        j = play_head->prev_params_change;
        if (j == 0) {
            track_data[i / 7].flags ^= 1 << (i % 7);
            if (--play_head->params_counter == 0xFF) {
                play_head->params_counter = 3;
                play_head->params_mode--;
            }

            mode = *play_head->params_mode >> shift_table_1[play_head->params_counter];
            mode &= 0x3;

            val = 0;
            switch (mode) {
            case 1:
                val = *--play_head->params_u8;
                break;
            case 2:
                val = *--play_head->params_u16;
                break;
            case 3:
                val = *--play_head->params_u32;
                break;
            }
            play_head->next_params_change = 0;
            play_head->prev_params_change = val;
            i--;
        }
        else {
            temp = j < (uint32_t)(i + 1) ? j : (uint32_t)(i + 1);
            i -= (int32_t)temp;
            play_head->next_params_change += temp;
            play_head->prev_params_change -= temp;
        }
    }
}

static void enb_calc_track_init(enb_play_head* play_head) { // 0x08A086CC in ULJM05681
    uint32_t i;
    uint8_t* track_flags;
    quat C010, C100, C200; // PSP VFPU registers
    vec3 C020, C110; // PSP VFPU registers
    float S030; // PSP VFPU register

    enb_track* track_data = play_head->track_data;

    track_flags = play_head->track_flags;

    C200.x = C200.y = C200.z = C200.w = 0.0f;
    S030 = play_head->data_header->scale;
    for (i = 0; i < play_head->data_header->track_count; i++) {
        C010 = track_data->quat;
        C020 = track_data->trans;

        C100.x = C010.x * S030;
        C100.y = C010.y * S030;
        C100.z = C010.z * S030;
        C100.w = C010.w * S030;

        C110.x = C020.x * S030;
        C110.y = C020.y * S030;
        C110.z = C020.z * S030;

        normalize_quat(&C100);

        track_data->qt[0].quat = C100;
        track_data->qt[0].trans = C110;
        track_data->qt[0].time = 0.0f;
        track_data->qt[1].quat = C100;
        track_data->qt[1].trans = C110;
        track_data->qt[1].time = 0.0f;

        track_data->quat = C200;
        (&track_data->quat)[1] = C200;
        track_data->flags = *track_flags++;

        track_data++;
    }
    play_head->track_data_selector = 0;
}

static void enb_calc_track(enb_play_head* play_head, float time, bool forward) { // 0x08A085D8 in ULJM05681
    uint8_t s0, s1;
    uint32_t i;
    quat C010, C100, C120; // PSP VFPU registers
    vec3 C020, C110, C130; // PSP VFPU registers
    float S030; // PSP VFPU register

    enb_track* track_data = play_head->track_data;

    if (forward) {
        s1 = play_head->track_data_selector & 0x1;
        play_head->track_data_selector = s0 = s1 ^ 1;
    }
    else {
        s0 = play_head->track_data_selector & 0x1;
        play_head->track_data_selector = s1 = s0 ^ 1;
    }

    S030 = forward ? play_head->data_header->scale : -play_head->data_header->scale;
    for (i = 0; i < play_head->data_header->track_count; i++, track_data++) {
        C010 = track_data->quat;
        C020 = track_data->trans;

        C120 = track_data->qt[s0].quat;
        C130 = track_data->qt[s0].trans;

        C100.x = C010.x * S030 + C120.x;
        C100.y = C010.y * S030 + C120.y;
        C100.z = C010.z * S030 + C120.z;
        C100.w = C010.w * S030 + C120.w;

        C110.x = C020.x * S030 + C130.x;
        C110.y = C020.y * S030 + C130.y;
        C110.z = C020.z * S030 + C130.z;

        normalize_quat(&C100);

        track_data->qt[s1].quat = C100;
        track_data->qt[s1].trans = C110;
        track_data->qt[s1].time = time;
    }
}

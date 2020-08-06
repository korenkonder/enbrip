/*
    by korenkonder
    GitHub/GitLab: korenkonder
*/

#include <math.h>
#include <stdlib.h>

// Helper part

typedef unsigned char bool;
typedef char sbyte;
typedef unsigned char byte;
typedef unsigned short ushort;
typedef unsigned int uint;

#define true 1
#define false 0

#define FREE(ptr) if (ptr) free((void*)ptr); ptr = 0;

typedef struct _vec3
{
    float x;
    float y;
    float z;
} vec3;

typedef struct _vec4
{
    float x;
    float y;
    float z;
    float w;
} vec4;

typedef struct _quat_trans_time
{
    vec4 quat;
    vec3 trans;
    float time;
} quat_trans_time;

typedef struct _track
{
    quat_trans_time qtt[2];
    vec4 quat;
    vec3 trans;
    byte flags;
    byte padding[3];
} track;

const char* is_null = "\"%s\" is null\n";
const char* cant_allocate = "Can't allocate memory for \"%s\"\n";
const char* cant_allocate_inner = "Can't allocate memory for %s\n";

static float dot_vec4(vec4* x, vec4* y)
{
    float z = x->x * y->x + x->y * y->y + x->z * y->z + x->w * y->w;
    return z;
}

static float length_vec4(vec4* x)
{
    float z = x->x * x->x + x->y * x->y + x->z * x->z + x->w * x->w;
    return (float)sqrt(z);
}

static float length_squared_vec4(vec4* x)
{
    float z = x->x * x->x + x->y * x->y + x->z * x->z + x->w * x->w;
    return z;
}

static void normalize_vec4(vec4* x)
{
    float length = length_vec4(x);
    if (length != 0)
        length = 1.0f / length;

    x->x *= length;
    x->y *= length;
    x->z *= length;
    x->w *= length;
}

static float lerpf(float x, float y, float blend)
{
    float b0, b1;
    b0 = blend;
    b1 = 1.0f - blend;
    return x * b1 + y * b0;
}

static void lerp_vec3(vec3* x, vec3* y, vec3* z, float blend)
{
    float b0, b1;
    b0 = blend;
    b1 = 1.0f - blend;
    z->x = x->x * b1 + y->x * b0;
    z->y = x->y * b1 + y->y * b0;
    z->z = x->z * b1 + y->z * b0;
}

static void slerp(vec4* x, vec4* y, vec4* z, float blend)
{
    if (length_squared_vec4(x) == 0.0f)
    {
        if (length_squared_vec4(y) == 0.0f)
        {
            z->x = z->y = z->z = 0.0f;
            z->w = 1.0f;
        }
        else
        {
            *z = *y;
        }
        return;
    }
    else if (length_squared_vec4(y) == 0.0f)
    {
        *z = *x;
        return;
    }

    float cosHalfAngle = dot_vec4(x, y);
    if (cosHalfAngle >= 1.0f || cosHalfAngle <= -1.0f)
    {
        *z = *x;
        return;
    }

    *z = *y;

    if (cosHalfAngle < 0.0f)
    {
        z->x = -z->x;
        z->y = -z->y;
        z->z = -z->z;
        z->w = -z->w;
        cosHalfAngle = -cosHalfAngle;
    }

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
    {
        blendA = 1.0f - blend;
        blendB = blend;
    }

    z->x = x->x * blendA + z->x * blendB;
    z->y = x->y * blendA + z->y * blendB;
    z->z = x->z * blendA + z->z * blendB;
    z->w = x->w * blendA + z->w * blendB;
    normalize_vec4(z);
}

static void lerp_quat_trans_time(quat_trans_time* x, quat_trans_time* y, quat_trans_time* z, float blend)
{
    if (blend > 1.0f)
        blend = 1.0f;
    else if (blend < 0.0f)
        blend = 0.0f;

    slerp(&x->quat, &y->quat, &z->quat, blend);
    lerp_vec3(&x->trans, &y->trans, &z->trans, blend);
    z->time = lerpf(x->time, y->time, blend);
}

// Enbaya part

typedef struct _enb_head
{
    int signature;                      // 0x00
    int track_count;                    // 0x04
    float scale;                        // 0x08
    float duration;                     // 0x0C
    int samples;                        // 0x10
    int track_data_init_mode_length;    // 0x14
    int track_data_init_sbyte_length;   // 0x18
    int track_data_init_short_length;   // 0x1C
    int track_data_init_int_length;     // 0x20
    int track_data_mode_length;         // 0x24
    int track_data_mode2_length;        // 0x28
    int track_data_sbyte_length;        // 0x2C
    int track_data_short_length;        // 0x30
    int track_data_int_length;          // 0x44
    int params_mode_length;             // 0x38
    int params_byte_length;             // 0x3C
    int params_ushort_length;           // 0x40
    int params_uint_length;             // 0x44
    int track_flags_length;             // 0x48
    int unknown;                        // In runtime becomes pointer to data after this int
} enb_head;

typedef struct _enb_play_head
{
    int current_sample;                 // 0x00
    float current_sample_time;          // 0x04
    float previous_sample_time;         // 0x08
    enb_head* data_header;              // 0x0C
    track* track_data;                  // 0x10
    int data_length;                    // 0x14
    int unknown[2];                     // 0x18
    float requested_time;               // 0x20
    float seconds_per_sample;           // 0x24
    uint next_params_change;            // 0x28
    uint prev_params_change;            // 0x2C
    byte* track_flags;                  // 0x30
    byte* track_data_init_mode;         // 0x34
    sbyte* track_data_init_sbyte;       // 0x38
    short* track_data_init_short;       // 0x3C
    int* track_data_init_int;           // 0x40
    byte track_data_init_counter;       // 0x44
    byte padding[3];                    // 0x45
    byte* track_data_mode;              // 0x48
    byte* track_data_mode2;             // 0x4C
    sbyte* track_data_sbyte;            // 0x50
    short* track_data_short;            // 0x54
    int* track_data_int;                // 0x58
    byte track_data_mode_counter;       // 0x5C
    byte track_data_mode2_counter;      // 0x5D
    byte padding2[2];                   // 0x5E
    byte* params_mode;                  // 0x60
    byte* params_byte;                  // 0x64
    ushort* params_ushort;              // 0x68
    uint* params_uint;                  // 0x6C
    byte params_counter;                // 0x70
    byte padding3[3];                   // 0x71
    byte* orig_track_data_init_mode;    // 0x74
    sbyte* orig_track_data_init_sbyte;  // 0x78
    short* orig_track_data_init_short;  // 0x7C
    int* orig_track_data_init_int;      // 0x80
    byte* orig_track_data_mode;         // 0x84
    byte* orig_track_data_mode2;        // 0x88
    sbyte* orig_track_data_sbyte;       // 0x8C
    short* orig_track_data_short;       // 0x90
    int* orig_track_data_int;           // 0x94
    byte* orig_params_mode;             // 0x98
    byte* orig_params_byte;             // 0x9C
    ushort* orig_params_ushort;         // 0xA0
    uint* orig_params_uint;             // 0xA4
    byte track_mode_selector;           // 0xA8
    byte track_data_selector;           // 0xA9
    byte padding4[6];                   // 0xAA
} enb_play_head;

__declspec(dllexport) int enb_process(byte* data_in, byte** data_out, int* data_out_len, float* duration, float* fps, int* frames);
static void enb_init(enb_play_head* play_head, enb_head* head);
static void enb_copy_pointers(enb_play_head* play_head);
static void enb_set_time(enb_play_head* play_head, float time);
static void enb_get_track_unscaled_init(enb_play_head* play_head);
static void enb_get_track_unscaled_forward(enb_play_head* play_head);
static void enb_get_track_unscaled_backward(enb_play_head* play_head);
static void enb_calc_params_init(enb_play_head* play_head);
static void enb_calc_params_forward(enb_play_head* play_head);
static void enb_calc_params_backward(enb_play_head* play_head);
static void enb_calc_track_init(enb_play_head* play_head);
static void enb_calc_track(enb_play_head* play_head, float time, bool forward);

const byte shift_table_1[] = { 6, 4, 2, 0 }; // 0x08BF1CE8, 0x08BF2160, 0x08BF210
const byte shift_table_2[] = { 4, 0 };       // 0x08BF1CF8
const int value_table_1[] = { 0, 1, 0, -1 }; // 0x08BB3FC0
const int value_table_2[] = { 0, 8, 2, 3, 4, 5, 6, 7, -8, -7, -6, -5, -4, -3, -2, -9 }; // 0x08BB3FD0

#define Return(message, val, err_code) { FREE(track_data); printf(message, val); return err_code; }

__declspec(dllexport) int enb_process(byte* data_in, byte** data_out, int* data_out_len, float* duration, float* fps, int* frames)
{
    enb_play_head play_head;
    enb_head* head;
    track* track_data;
    quat_trans_time* qtt_data;
    quat_trans_time qt1, qt2;
    int i, j;
    float blend;

    track_data = 0;

    if (!data_in)
        Return(is_null, "is_null", -1)
    else if (!data_out)
        Return(is_null, "data_out", -2)
    else if (!data_out_len)
        Return(is_null, "data_out_len", -3)
    else if (!duration)
        Return(is_null, "duration", -4)
    else if (!fps)
        Return(is_null, "fps", -5)
    else if (!frames)
        Return(is_null, "frames", -6)

    head = (enb_head*)data_in;
    memset((void*)&play_head, 0, sizeof(enb_play_head));

    if (head->signature != 0x100A9DA4 && head->signature != 0x100AAD74)
        Return("Invalid signature 0x%X\n", head->signature, -7)

    enb_init(&play_head, head);
    *duration = head->duration;

    if (*fps > 600.0f)
        *fps = 600.0f;
    else if (*fps < (float)head->samples)
        *fps = (float)head->samples;

    track_data = (track*)malloc(sizeof(track) * head->track_count);

    if (!track_data)
        Return(cant_allocate_inner, "tracks data", -8)

    memset((void*)track_data, 0, sizeof(track) * head->track_count);

    blend = *duration * *fps;
    *frames = (int)blend + (fmod(blend, 1.0f) >= 0.5f) + 1;
    *data_out_len = sizeof(quat_trans_time) * head->track_count * *frames + 0x10;
    *data_out = (byte*)malloc(*data_out_len);

    if (!*data_out)
        Return(cant_allocate_inner, "output data", -9)
    memset((void*)*data_out, 0, *data_out_len);

    ((int*)*data_out)[0] = head->track_count;
    ((int*)*data_out)[1] = *frames;
    ((float*)*data_out)[2] = *fps;
    ((float*)*data_out)[3] = *duration;

    play_head.data_header = head;
    play_head.track_data = track_data;
    qtt_data = (quat_trans_time*)(*data_out + 0x10);
    for (i = 0; i < *frames; i++)
    {
        enb_set_time(&play_head, (float)i / *fps);

        for (j = 0; j < head->track_count; j++, qtt_data++)
        {
            if (play_head.track_mode_selector)
            {
                qt1 = track_data[j].qtt[play_head.track_data_selector];
                qt2 = track_data[j].qtt[play_head.track_data_selector ^ 1];
                blend = ((float)i / *fps - play_head.previous_sample_time)
                    / (play_head.current_sample_time - play_head.previous_sample_time);
                lerp_quat_trans_time(&qt1, &qt2, qtt_data, blend);
            }
            else
                *qtt_data = track_data[j].qtt[0];
        }
    }

    FREE(track_data);

    return 0;
}

static void enb_init(enb_play_head* play_head, enb_head* head) // 0x08A08050 in ULJM05681
{
    byte* data;
    int temp;

    data = (byte*)head;
    play_head->current_sample = -1;
    play_head->current_sample_time = -1.0f;
    play_head->previous_sample_time = -1.0f;
    play_head->requested_time = -1.0f;
    play_head->seconds_per_sample = 1.0f / (float)head->samples;
    play_head->track_mode_selector = 0;

    temp = 0x50;
    play_head->orig_track_data_init_int = (int*)(data + temp);

    temp += head->track_data_init_int_length;
    play_head->orig_track_data_int = (int*)(data + temp);

    temp += head->track_data_int_length;
    play_head->orig_params_uint = (uint*)(data + temp);

    temp += head->params_uint_length;
    play_head->orig_track_data_init_short = (short*)(data + temp);

    temp += head->track_data_init_short_length;
    play_head->orig_track_data_short = (short*)(data + temp);

    temp += head->track_data_short_length;
    play_head->orig_params_ushort = (ushort*)(data + temp);

    temp += head->params_ushort_length;
    play_head->orig_track_data_init_mode = data + temp;

    temp += head->track_data_init_mode_length;
    play_head->orig_track_data_init_sbyte = (sbyte*)(data + temp);

    temp += head->track_data_init_sbyte_length;
    play_head->orig_track_data_mode = data + temp;

    temp += head->track_data_mode_length;
    play_head->orig_track_data_mode2 = data + temp;

    temp += head->track_data_mode2_length;
    play_head->orig_track_data_sbyte = (sbyte*)(data + temp);

    temp += head->track_data_sbyte_length;
    play_head->orig_params_mode = data + temp;

    temp += head->params_mode_length;
    play_head->orig_params_byte = data + temp;

    temp += head->params_byte_length;
    play_head->track_flags = data + temp;

    temp += head->track_flags_length;
    play_head->data_length = temp;

    enb_copy_pointers(play_head);
}

static void enb_copy_pointers(enb_play_head* play_head) // 0x08A07FD0 in ULJM05681
{
    play_head->track_data_init_mode = play_head->orig_track_data_init_mode;
    play_head->track_data_init_sbyte = play_head->orig_track_data_init_sbyte;
    play_head->track_data_init_short = play_head->orig_track_data_init_short;
    play_head->track_data_init_int = play_head->orig_track_data_init_int;
    play_head->track_data_init_counter = 0;

    play_head->track_data_mode = play_head->orig_track_data_mode;
    play_head->track_data_mode2 = play_head->orig_track_data_mode2;
    play_head->track_data_sbyte = play_head->orig_track_data_sbyte;
    play_head->track_data_short = play_head->orig_track_data_short;
    play_head->track_data_int = play_head->orig_track_data_int;
    play_head->track_data_mode_counter = 0;
    play_head->track_data_mode2_counter = 0;

    play_head->params_mode = play_head->orig_params_mode;
    play_head->params_byte = play_head->orig_params_byte;
    play_head->params_ushort = play_head->orig_params_ushort;
    play_head->params_uint = play_head->orig_params_uint;
    play_head->params_counter = 0;
}

static void enb_set_time(enb_play_head* play_head, float time) // 0x08A0876C in ULJM05681
{
    float requested_time;
    float sps; // seconds per sample
    int mode;

    if (time == play_head->requested_time) return;

    requested_time = play_head->requested_time;
    sps = play_head->seconds_per_sample;

    if ((requested_time == -1.0f) || (0.000001f > time) || (requested_time - time > time)
        || ((sps <= requested_time) && (sps > time)))
    {
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
    if (time < 0.000001f) return;

    while ((time > play_head->current_sample_time)
        && (play_head->data_header->duration - play_head->current_sample_time > 0.00001f))
    {
        if (play_head->track_mode_selector == 2)
        {
            if (play_head->params_counter == 4)
            {
                play_head->params_counter = 0;
                play_head->params_mode++;
            }

            mode = *play_head->params_mode >> shift_table_1[play_head->params_counter++];
            mode &= 0x3;

            switch (mode)
            {
                case 1:
                    play_head->params_byte++;
                    break;
                case 2:
                    play_head->params_ushort++;
                    break;
                case 3:
                    play_head->params_uint++;
                    break;
            }
            play_head->track_mode_selector = 1;
        }
        else if (play_head->current_sample > 0)
        {
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

    while (time < play_head->previous_sample_time)
    {
        if (play_head->track_mode_selector == 1)
        {
            if (--play_head->params_counter == 0xFF)
            {
                play_head->params_counter = 3;
                play_head->params_mode--;
            }

            mode = *play_head->params_mode >> shift_table_1[play_head->params_counter];
            mode &= 0x3;

            switch (mode)
            {
                case 1:
                    play_head->params_byte--;
                    break;
                case 2:
                    play_head->params_ushort--;
                    break;
                case 3:
                    play_head->params_uint--;
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
    return;
}

static void enb_get_track_unscaled_init(enb_play_head* play_head) // 0x08A08D3C in ULJM05681
{
    int i, j, mode, val;

    track* track_data = play_head->track_data;

    for (i = 0; i < play_head->data_header->track_count; i++, track_data++)
    {
        for (j = 0; j < 7; j++)
        {
            if (play_head->track_data_init_counter == 4)
            {
                play_head->track_data_init_counter = 0;
                play_head->track_data_init_mode++;
            }

            mode = *play_head->track_data_init_mode >> shift_table_1[play_head->track_data_init_counter++];
            mode &= 0x3;

            val = 0;
            switch (mode)
            {
                case 1:
                    val = *play_head->track_data_init_sbyte++;
                    break;
                case 2:
                    val = *play_head->track_data_init_short++;
                    break;
                case 3:
                    val = *play_head->track_data_init_int++;
                    break;
            }

            switch (j)
            {
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

static void enb_get_track_unscaled_forward(enb_play_head* play_head) // 0x08A08E7C in ULJM05681
{
    int j;
    int i;
    int val;

    track* track_data = play_head->track_data;

    for (i = 0; i < play_head->data_header->track_count; i++, track_data++)
    {
        if (track_data->flags == 0)
            continue;

        for (j = 0; j < 7; j++)
        {
            if ((track_data->flags & (1 << j)) == 0)
                continue;

            if (play_head->track_data_mode_counter == 4)
            {
                play_head->track_data_mode_counter = 0;
                play_head->track_data_mode++;
            }

            val = *play_head->track_data_mode >> shift_table_1[play_head->track_data_mode_counter++];
            val &= 0x3;

            if (val == 2)
            {
                if (play_head->track_data_mode2_counter == 2)
                {
                    play_head->track_data_mode2_counter = 0;
                    play_head->track_data_mode2++;
                }

                val = *play_head->track_data_mode2 >> shift_table_2[play_head->track_data_mode2_counter++];
                val &= 0xF;

                if (val == 0)
                {
                    val = *play_head->track_data_sbyte;
                    play_head->track_data_sbyte++;
                    if (val == 0)
                    {
                        val = *play_head->track_data_short++;
                        if (val == 0)
                            val = *play_head->track_data_int++;
                    }
                    else if ((val > 0) && (val < 9))
                        val += 0x7f;
                    else if ((val > -9) && (val < 0))
                        val -= 0x80;
                }
                else val = value_table_2[val];
            }
            else val = value_table_1[val];

            switch (j)
            {
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

static void enb_get_track_unscaled_backward(enb_play_head* play_head) // 0x08A090A0 in ULJM05681
{
    int j;
    int val;
    int i;

    track* track_data = play_head->track_data;

    track_data += play_head->data_header->track_count - 1;
    for (i = play_head->data_header->track_count - 1; i >= 0; i--, track_data--)
    {
        if (track_data->flags == 0)
            continue;

        for (j = 6; j >= 0; j--)
        {
            if ((track_data->flags & (1 << j)) == 0)
                continue;

            if (--play_head->track_data_mode_counter == 0xFF)
            {
                play_head->track_data_mode_counter = 3;
                play_head->track_data_mode--;
            }

            val = *play_head->track_data_mode >> shift_table_1[play_head->track_data_mode_counter];
            val &= 0x3;

            if (val == 2)
            {
                if (--play_head->track_data_mode2_counter == 0xFF)
                {
                    play_head->track_data_mode2_counter = 1;
                    play_head->track_data_mode2--;
                }

                val = *play_head->track_data_mode2 >> shift_table_2[play_head->track_data_mode2_counter];
                val &= 0xF;

                if (val == 0)
                {
                    val = *--play_head->track_data_sbyte;
                    if (val == 0)
                    {
                        val = *--play_head->track_data_short;
                        if (val == 0)
                            val = *--play_head->track_data_int;
                    }
                    else if ((val > 0) && (val < 9))
                        val += 0x7f;
                    else if ((val > -9) && (val < 0))
                        val -= 0x80;
                }
                else val = value_table_2[val];
            }
            else val = value_table_1[val];

            switch (j)
            {
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

static void enb_calc_params_init(enb_play_head* play_head) // 0x08A0931C in ULJM05681
{
    uint val;
    int mode;

    if (play_head->params_counter == 4)
    {
        play_head->params_counter = 0;
        play_head->params_mode++;
    }

    mode = *play_head->params_mode >> shift_table_1[play_head->params_counter++];
    mode &= 0x3;

    val = 0;
    switch (mode)
    {
        case 1:
            val = *play_head->params_byte++;
            break;
        case 2:
            val = *play_head->params_ushort++;
            break;
        case 3:
            val = *play_head->params_uint++;
            break;
    }
    play_head->next_params_change = val;
    play_head->prev_params_change = 0;
}

static void enb_calc_params_forward(enb_play_head* play_head) // 0x08A09404 in ULJM05681
{
    int i, mode, track_params_count;
    uint j, temp, val;

    track* track_data = play_head->track_data;

    track_params_count = play_head->data_header->track_count * 7;
    i = 0;
    while (i < track_params_count)
    {
        j = play_head->next_params_change;
        if (j == 0)
        {
            track_data[i / 7].flags ^= 1 << (i % 7);
            if (play_head->params_counter == 4)
            {
                play_head->params_counter = 0;
                play_head->params_mode++;
            }

            mode = *play_head->params_mode >> shift_table_1[play_head->params_counter++];
            mode &= 0x3;

            val = 0;
            switch (mode)
            {
                case 1:
                    val = *play_head->params_byte++;
                    break;
                case 2:
                    val = *play_head->params_ushort++;
                    break;
                case 3:
                    val = *play_head->params_uint++;
                    break;
            }
            play_head->next_params_change = val;
            play_head->prev_params_change = 0;
            i++;
        }
        else
        {
            temp = j < (uint)(track_params_count - i) ? j : (uint)(track_params_count - i);
            i += (int)temp;
            play_head->next_params_change -= temp;
            play_head->prev_params_change += temp;
        }
    }
}

static void enb_calc_params_backward(enb_play_head* play_head) // 0x08A0968C in ULJM05681
{
    int i, mode, track_params_count;
    uint j, temp, val;

    track* track_data = play_head->track_data;

    track_params_count = play_head->data_header->track_count * 7;
    i = track_params_count - 1;
    while (i >= 0)
    {
        j = play_head->prev_params_change;
        if (j == 0)
        {
            track_data[i / 7].flags ^= 1 << (i % 7);
            if (--play_head->params_counter == 0xFF)
            {
                play_head->params_counter = 3;
                play_head->params_mode--;
            }

            mode = *play_head->params_mode >> shift_table_1[play_head->params_counter];
            mode &= 0x3;

            val = 0;
            switch (mode)
            {
                case 1:
                    val = *--play_head->params_byte;
                    break;
                case 2:
                    val = *--play_head->params_ushort;
                    break;
                case 3:
                    val = *--play_head->params_uint;
                    break;
            }
            play_head->next_params_change = 0;
            play_head->prev_params_change = val;
            i--;
        }
        else
        {
            temp = j < (uint)(i + 1) ? j : (uint)(i + 1);
            i -= (int)temp;
            play_head->next_params_change += temp;
            play_head->prev_params_change -= temp;
        }
    }
}

static void enb_calc_track_init(enb_play_head* play_head) // 0x08A086CC in ULJM05681
{
    int i;
    byte* track_flags;
    vec4 C010, C100, C200;
    vec3 C020, C110;
    float S030;

    track* track_data = play_head->track_data;

    track_flags = play_head->track_flags;

    C200.x = C200.y = C200.z = C200.w = 0.0f;
    S030 = play_head->data_header->scale;
    for (i = 0; i < play_head->data_header->track_count; i++)
    {
        C010 = track_data->quat;
        C020 = track_data->trans;

        C100.x = C010.x * S030;
        C100.y = C010.y * S030;
        C100.z = C010.z * S030;
        C100.w = C010.w * S030;

        C110.x = C020.x * S030;
        C110.y = C020.y * S030;
        C110.z = C020.z * S030;

        normalize_vec4(&C100);

        track_data->qtt[0].quat = C100;
        track_data->qtt[0].trans = C110;
        track_data->qtt[0].time = 0.0f;
        track_data->qtt[1].quat = C100;
        track_data->qtt[1].trans = C110;
        track_data->qtt[1].time = 0.0f;

        track_data->quat = C200;
        track_data->trans.x = C200.x;
        track_data->trans.y = C200.y;
        track_data->trans.z = C200.z;
        track_data->flags = *track_flags++;

        track_data++;
    }
    play_head->track_data_selector = 0;
}

static void enb_calc_track(enb_play_head* play_head, float time, bool forward) // 0x08A085D8 in ULJM05681
{
    int i, s0, s1;
    vec4 C010, C100, C120;
    vec3 C020, C110, C130;
    float S030;

    track* track_data = play_head->track_data;

    if (forward)
    {
        s1 = play_head->track_data_selector;
        s0 = play_head->track_data_selector ^= 1;
    }
    else
    {
        s0 = play_head->track_data_selector;
        s1 = play_head->track_data_selector ^= 1;
    }

    S030 = forward ? play_head->data_header->scale : -play_head->data_header->scale;
    for (i = 0; i < play_head->data_header->track_count; i++, track_data++)
    {
        C010 = track_data->quat;
        C020 = track_data->trans;

        C120 = track_data->qtt[s0].quat;
        C130 = track_data->qtt[s0].trans;

        C100.x = C010.x * S030 + C120.x;
        C100.y = C010.y * S030 + C120.y;
        C100.z = C010.z * S030 + C120.z;
        C100.w = C010.w * S030 + C120.w;

        C110.x = C020.x * S030 + C130.x;
        C110.y = C020.y * S030 + C130.y;
        C110.z = C020.z * S030 + C130.z;

        normalize_vec4(&C100);

        track_data->qtt[s1].quat = C100;
        track_data->qtt[s1].trans = C110;
        track_data->qtt[s1].time = time;
    }
}

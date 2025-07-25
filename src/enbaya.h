/*
    by korenkonder
    GitHub/GitLab: korenkonder
*/

#pragma once

#include "help.h"

typedef struct {
    quat_trans qt[2];
    quat quat;
    vec3 trans;
    uint8_t flags;
    uint8_t padding[3];
} enb_track;

typedef struct  __attribute__((aligned(4))) {
    uint32_t signature;                                     // 0x00
    uint32_t track_count;                                   // 0x04
    float_t quantization_error;                             // 0x08
    float_t duration;                                       // 0x0C
    uint32_t sample_rate;                                   // 0x10
    uint32_t track_data_init_i2_length;                     // 0x14
    uint32_t track_data_init_i8_length;                     // 0x18
    uint32_t track_data_init_i16_length;                    // 0x1C
    uint32_t track_data_init_i32_length;                    // 0x20
    uint32_t track_data_i2_length;                          // 0x24
    uint32_t track_data_i4_length;                          // 0x28
    uint32_t track_data_i8_length;                          // 0x2C
    uint32_t track_data_i16_length;                         // 0x30
    uint32_t track_data_i32_length;                         // 0x44
    uint32_t state_data_u2_length;                          // 0x38
    uint32_t state_data_u8_length;                          // 0x3C
    uint32_t state_data_u16_length;                         // 0x40
    uint32_t state_data_u32_length;                         // 0x44
    uint32_t track_flags_length;                            // 0x48
    uint32_t data;                                          // In runtime becomes pointer to data after this uint32_t
} enb_anim_stream;

typedef struct {
    int32_t next_step;                                      // 0x00
    int32_t prev_step;                                      // 0x04
} enb_anim_state;

typedef struct {
    const uint8_t* i2;                                      // 0x00
    const int8_t* i8;                                       // 0x04
    const int16_t* i16;                                     // 0x08
    const int32_t* i32;                                     // 0x0C
    int8_t i2_counter;                                      // 0x10
} enb_anim_track_data_init_decoder;

typedef struct {
    const uint8_t* i2;                                      // 0x00
    const uint8_t* i4;                                      // 0x04
    const int8_t* i8;                                       // 0x08
    const int16_t* i16;                                     // 0x0C
    const int32_t* i32;                                     // 0x10
    int8_t i2_counter;                                      // 0x14
    int8_t i4_counter;                                      // 0x18
} enb_anim_track_data_decoder;

typedef struct {
    const uint8_t* u2;                                      // 0x00
    const uint8_t* u8;                                      // 0x04
    const uint16_t* u16;                                    // 0x08
    const uint32_t* u32;                                    // 0x0C
    int8_t u2_counter;                                      // 0x10
} enb_anim_state_data_decoder;

typedef struct {
    const uint8_t* i2;                                      // 0x00
    const int8_t* i8;                                       // 0x04
    const int16_t* i16;                                     // 0x08
    const int32_t* i32;                                     // 0x0C
} enb_anim_track_data_init;

typedef struct {
    const uint8_t* i2;                                      // 0x00
    const uint8_t* i4;                                      // 0x04
    const int8_t* i8;                                       // 0x08
    const int16_t* i16;                                     // 0x0C
    const int32_t* i32;                                     // 0x10
} enb_anim_track_data;

typedef struct {
    const uint8_t* u2;                                      // 0x00
    const uint8_t* u8;                                      // 0x04
    const uint16_t* u16;                                    // 0x08
    const uint32_t* u32;                                    // 0x0C
} enb_anim_state_data;

typedef struct __attribute__((aligned(8))) {
    uint32_t current_sample;                                // 0x00
    float_t current_sample_time;                            // 0x04
    float_t previous_sample_time;                           // 0x08
    enb_anim_stream* stream;                                // 0x0C
    enb_track* track;                                       // 0x10
    uint32_t data_length;                                   // 0x14
    uint32_t fast_cache_decoding_state;                     // 0x18
} enb_anim_context_data;

typedef struct __attribute__((aligned(8))) {
    enb_anim_context_data data;                             // 0x00
    float_t requested_time;                                 // 0x20
    float_t seconds_per_sample;                             // 0x24
    enb_anim_state state;                                   // 0x28
    const uint8_t* track_flags;                             // 0x30
    enb_anim_track_data_init_decoder track_data_init_dec;   // 0x34
    enb_anim_track_data_decoder track_data_dec;             // 0x48
    enb_anim_state_data_decoder state_data_dec;             // 0x60
    enb_anim_track_data_init track_data_init;               // 0x74
    enb_anim_track_data track_data;                         // 0x84
    enb_anim_state_data state_data;                         // 0x98
    uint8_t track_direction;                                // 0xA8
    uint8_t track_selector;                                 // 0xA9
} enb_anim_context;

extern int32_t enb_process(uint8_t* data_in, uint8_t** data_out, size_t* data_out_len, float_t* duration,
    float_t* fps, int32_t* frames, quat_trans_interp_method quat_method, quat_trans_interp_method trans_method);
extern int32_t enb_initialize(uint8_t* data, enb_anim_context** anim_ctx);
extern void enb_free(enb_anim_context** anim_ctx);
extern void enb_get_component_values(enb_anim_context* anim_ctx, float_t time, int32_t track_id,
    quat_trans* data, quat_trans_interp_method quat_method, quat_trans_interp_method trans_method);
extern int32_t enb_encode_data(quat_trans* track_data, int32_t* track_data_count, int32_t num_tracks,
    int32_t num_components, float_t duration, int32_t sample_rate, float_t quantization_error,
    quat_trans_interp_method quat_method, quat_trans_interp_method trans_method,
    uint8_t** data_out, size_t* data_out_len);

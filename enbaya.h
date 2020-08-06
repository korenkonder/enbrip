/*
    by korenkonder
    GitHub/GitLab: korenkonder
*/

#include "help.h"

typedef struct
{
      int x00; // signature
      int x04; // track count
    float x08; // scale
    float x0C; // duration
      int x10; // samples
      int x14;
      int x18;
      int x1C;
      int x20;
      int x24;
      int x28;
      int x2C;
      int x30;
      int x34;
      int x38;
      int x3C;
      int x40;
      int x44;
      int x48;
      int x4C; // unknown. In runtime it becomes pointer to data
} enb_head;

typedef struct
{
       int  x00;    // current sample
     float  x04;    // current sample time
     float  x08;    // previous sample time
     enb_head* x0C; // data pointer
    trans_rot* x10; // trans rot pointer
       int  x14;    // length of data
       int  x18;    // unknown
       int  x1C;    // unknown
     float  x20;    // requested time
     float  x24;    // seconds per sample
      uint  x28;    // distance to next params change
      uint  x2C;    // distance to previous params change
      byte* x30;    // trans rot flags pointer
      byte* x34;    // unscaled trans rot init mode pointer
     sbyte* x38;    // unscaled trans rot init sbyte pointer
     short* x3C;    // unscaled trans rot init short pointer
       int* x40;    // unscaled trans rot init int pointer
      byte  x44;    // unscaled trans rot init counter
      byte  x45[3]; // padding
      byte* x48;    // unscaled trans rot mode 0 pointer
      byte* x4C;    // unscaled trans rot mode 1 pointer
     sbyte* x50;    // unscaled trans rot sbyte pointer
     short* x54;    // unscaled trans rot short pointer
       int* x58;    // unscaled trans rot int pointer
      byte  x5C;    // unscaled trans rot mode 0 counter
      byte  x5D;    // unscaled trans rot mode 1 counter
      byte  x5E[2]; // padding
      byte* x60;    // params mode pointer
      byte* x64;    // params byte pointer
    ushort* x68;    // params ushort pointer
      uint* x6C;    // params uint pointer
      byte  x70;    // params counter
      byte  x71[3]; // padding
      byte* x74;    // unscaled trans rot init mode pointer
     sbyte* x78;    // unscaled trans rot init sbyte pointer
     short* x7C;    // unscaled trans rot init short pointer
       int* x80;    // unscaled trans rot init int pointer
      byte* x84;    // unscaled trans rot mode 0 pointer
      byte* x88;    // unscaled trans rot mode 1 pointer
     sbyte* x8C;    // unscaled trans rot sbyte pointer
     short* x90;    // unscaled trans rot short pointer
       int* x94;    // unscaled trans rot int pointer
      byte* x98;    // params mode pointer
      byte* x9C;    // params byte pointer
    ushort* xA0;    // params ushort pointer
      uint* xA4;    // params uint pointer
      byte  xA8;    // params mode
      byte  xA9;    // trans rot selector
      byte  xAA[6]; // padding
} enb_play_head;

int enb_process(byte* data_in, byte** data_out, int* data_out_len, float* duration, float* fps, int* frames);
void enb_init(enb_play_head* play_head, enb_head* head);
void enb_copy_pointers(enb_play_head* play_head);
void enb_set_time(enb_play_head* play_head, float time);
void enb_get_trans_rot_unscaled_init(enb_play_head* play_head);
void enb_get_trans_rot_unscaled_forward(enb_play_head* play_head);
void enb_get_trans_rot_unscaled_backward(enb_play_head* play_head);
void enb_calc_params_init(enb_play_head* play_head);
void enb_calc_params_forward(enb_play_head* play_head);
void enb_calc_params_backward(enb_play_head* play_head);
void enb_calc_trans_rot_init(enb_play_head* play_head);
void enb_calc_trans_rot(enb_play_head* play_head, float time, bool forward);

const byte shift_table_1[] = { 6, 4, 2,  0 }; // 0x08BF1CE8, 0x08BF2160, 0x08BF210
const byte shift_table_2[] = { 4, 0 };        // 0x08BF1CF8
const  int value_table_1[] = { 0, 1, 0, -1 }; // 0x08BB3FC0
const  int value_table_2[] = { 0, 8, 2, 3, 4, 5, 6, 7, -8, -7, -6, -5, -4, -3, -2, -9 }; // 0x08BB3FD0

int enb_process(byte* data_in, byte** data_out, int* data_out_len, float* duration, float* fps, int* frames)
{
    enb_play_head play_head;
    enb_head* head;
    trans_rot* trans_rot_data;
    quat_trans_time* qtt_data;
    quat_trans_time qt1, qt2;
    int i, j;
    float blend;

    head = (enb_head*)data_in;
    memset((void*)&play_head, 0, sizeof(enb_play_head));
    
    if (head->x00 != 0x100A9DA4 && head->x00 != 0x100AAD74)
    { printf("Invalid signature 0x%X\n", head->x00); return -1; }

    enb_init(&play_head, head);
    *duration = head->x0C;
    if (*fps > 600.0f) *fps = 600.0f;
    else if (*fps < (float)head->x10) *fps = (float)head->x10;

    trans_rot_data = (trans_rot*)malloc(sizeof(trans_rot) * head->x04);
    if (!trans_rot_data) { printf(cant_allocate); printf("\"trans_rot\"\n"); return -2; }
    memset((void*)trans_rot_data, 0, sizeof(trans_rot) * head->x04);

    *frames = (int)roundf(*duration * *fps) + 1;
    *data_out_len = sizeof(quat_trans_time) * head->x04 * *frames + 0x10;
    *data_out = (byte*)malloc(*data_out_len);
    if (!*data_out) { printf(cant_allocate); printf("\"data_out\"\n"); return -3; }
    memset((void*)*data_out, 0, *data_out_len);

    ((  int*)*data_out)[0] = head->x04;
    ((  int*)*data_out)[1] = *frames;
    ((float*)*data_out)[2] = *fps;
    ((float*)*data_out)[3] = *duration;
    
    play_head.x0C = head;
    play_head.x10 = trans_rot_data;
    qtt_data = (quat_trans_time*)(*data_out + 0x10);
    for (i = 0; i < *frames; i++)
    {
        enb_set_time(&play_head, (float)i / *fps);

        for (j = 0; j < head->x04; j++, qtt_data++)
        {
            if (((play_head.xA8 != 2) && !play_head.x04) || ((play_head.xA8 != 2) && !play_head.x08))
            {
                *qtt_data = trans_rot_data[j].qtt[0];
                continue;
            }

            qt1 = trans_rot_data[j].qtt[play_head.xA9];
            qt2 = trans_rot_data[j].qtt[play_head.xA9 ^ 1];
            blend = ((float)i / *fps - play_head.x08) / (play_head.x04 - play_head.x08);
            lerp_quat_trans_time(&qt1, &qt2, qtt_data, blend);
            continue;
        }
    }

    return 0;
}

void enb_init(enb_play_head* play_head, enb_head* head) // 0x08A08050 in ULJM05681
{
    byte* data;
    int temp;

    data = (byte*)head;
    play_head->x00 = -1;
    play_head->x04 = -1.0f;
    play_head->x08 = -1.0f;
    play_head->x20 = -1.0f;
    play_head->x24 =  1.0f / (float)head->x10;
    play_head->xA8 =  0;
    
    temp =       0x50; play_head->x80 = (   int*)(data + temp);
    temp += head->x20; play_head->x94 = (   int*)(data + temp);
    temp += head->x34; play_head->xA4 = (  uint*)(data + temp);
    temp += head->x44; play_head->x7C = ( short*)(data + temp);
    temp += head->x1C; play_head->x90 = ( short*)(data + temp);
    temp += head->x30; play_head->xA0 = (ushort*)(data + temp);
    temp += head->x40; play_head->x74 =           data + temp ;
    temp += head->x14; play_head->x78 = ( sbyte*)(data + temp);
    temp += head->x18; play_head->x84 =           data + temp ;
    temp += head->x24; play_head->x88 =           data + temp ;
    temp += head->x28; play_head->x8C = ( sbyte*)(data + temp);
    temp += head->x2C; play_head->x98 =           data + temp ;
    temp += head->x38; play_head->x9C =           data + temp ;
    temp += head->x3C; play_head->x30 =           data + temp ;
    temp += head->x48;
    play_head->x14 = temp + 0x50;

    enb_copy_pointers(play_head);
}

void enb_copy_pointers(enb_play_head* play_head) // 0x08A07FD0 in ULJM05681
{
    play_head->x34 = play_head->x74;
    play_head->x38 = play_head->x78;
    play_head->x3C = play_head->x7C;
    play_head->x40 = play_head->x80;
    play_head->x44 = 0;
    play_head->x48 = play_head->x84;
    play_head->x4C = play_head->x88;
    play_head->x50 = play_head->x8C;
    play_head->x54 = play_head->x90;
    play_head->x58 = play_head->x94;
    play_head->x5C = 0;
    play_head->x5D = 0;
    play_head->x60 = play_head->x98;
    play_head->x64 = play_head->x9C;
    play_head->x68 = play_head->xA0;
    play_head->x6C = play_head->xA4;
    play_head->x70 = 0;
}

void enb_set_time(enb_play_head* play_head, float time) // 0x08A0876C in ULJM05681
{
    float currTime;
    float sps; // samples per second
    int mode;

    if (time == play_head->x20) return;

    currTime = play_head->x20;
    sps = play_head->x24;

    if ((currTime == -1.0f) || (0.000001f > time) || (currTime - time > time)
        || ((sps <= currTime) && (sps > time)))
    {
        play_head->x00 = 0;
        play_head->x04 = 0.0f;
        play_head->x08 = 0.0f;
        enb_copy_pointers(play_head);
        play_head->xA8 = 0;
        enb_calc_params_init(play_head);
        enb_get_trans_rot_unscaled_init(play_head);
        enb_calc_trans_rot_init(play_head);
    }

    play_head->x20 = time;
    if (time < 0.000001f) return;

    while ((time > play_head->x04) && (play_head->x0C->x0C - play_head->x04 > 0.00001f))
    {
        if (play_head->xA8 == 2)
        {
            if (play_head->x70 == 4)
            {
                play_head->x70 = 0;
                play_head->x60++;
            }

            mode = (*play_head->x60 >> shift_table_1[play_head->x70]) & 0x03;
            play_head->x70++;
            switch (mode)
            {
                case 1: play_head->x64++; break;
                case 2: play_head->x68++; break;
                case 3: play_head->x6C++; break;
            }
            play_head->xA8 = 1;
        }
        else if (play_head->x00 > 0)
        {
            enb_calc_params_forward(play_head);
            play_head->xA8 = 1;
        }
        play_head->x00++;
        enb_get_trans_rot_unscaled_forward(play_head);
        currTime = play_head->x00 * sps;
        if (play_head->x0C->x0C <= currTime) currTime = play_head->x0C->x0C;
        enb_calc_trans_rot(play_head, currTime, 1);
        play_head->x04 =  play_head->x00      * sps;
        play_head->x08 = (play_head->x00 - 1) * sps;
    }

    while (time < play_head->x08)
    {
        if (play_head->xA8 == 1)
        {
            play_head->x70--;
            if (play_head->x70 == 0xFF)
            {
                play_head->x70 = 3;
                play_head->x60--;
            }

            mode = (*play_head->x60 >> shift_table_1[play_head->x70]) & 0x03;
            switch (mode)
            {
                case 1: play_head->x64--; break;
                case 2: play_head->x68--; break;
                case 3: play_head->x6C--; break;
            }
            play_head->xA8 = 2;
        }
        else
            enb_calc_params_backward(play_head);
        play_head->x00--;
        enb_get_trans_rot_unscaled_backward(play_head);
        play_head->x04 =  play_head->x00      * sps;
        play_head->x08 = (play_head->x00 - 1) * sps;
        enb_calc_trans_rot(play_head, play_head->x08, 0);
    }
    return;
}

void enb_get_trans_rot_unscaled_init(enb_play_head* play_head) // 0x08A08D3C in ULJM05681
{
    int i, j, mode, val;

    trans_rot* trans_rot_data = play_head->x10;

    for (i = 0; i < play_head->x0C->x04; i++, trans_rot_data++)
    {
        for (j = 0; j < 7; j++)
        {
            if (play_head->x44 == 4)
            {
                play_head->x44 = 0;
                play_head->x34++;
            }

            mode = (*play_head->x34 >> shift_table_1[play_head->x44]) & 0x3;
            play_head->x44++;
            val = 0;
            switch (mode)
            {
                case 1: val = *play_head->x38++; break;
                case 2: val = *play_head->x3C++; break;
                case 3: val = *play_head->x40++; break;
            }

            switch (j)
            {
                case 0: trans_rot_data->quat.x = (float)val; break;
                case 1: trans_rot_data->quat.y = (float)val; break;
                case 2: trans_rot_data->quat.z = (float)val; break;
                case 3: trans_rot_data->quat.w = (float)val; break;
                case 4: trans_rot_data->trans.x = (float)val; break;
                case 5: trans_rot_data->trans.y = (float)val; break;
                case 6: trans_rot_data->trans.z = (float)val; break;
            }
        }
    }
    play_head->x00 = 0;
}

void enb_get_trans_rot_unscaled_forward(enb_play_head* play_head) // 0x08A08E7C in ULJM05681
{
    int j;
    int i;
    int val;

    trans_rot* trans_rot_data = play_head->x10;

    for (i = 0; i < play_head->x0C->x04; i++, trans_rot_data++)
    {
        if (trans_rot_data->flags == 0) continue;

        for (j = 0; j < 7; j++)
        {
            if ((trans_rot_data->flags & (1 << j)) == 0) continue;

            if (play_head->x5C == 4)
            {
                play_head->x5C = 0;
                play_head->x48++;
            }

            val = (*play_head->x48 >> shift_table_1[play_head->x5C]) & 0x3;
            play_head->x5C++;
            if (val == 2)
            {
                if (play_head->x5D == 2)
                {
                    play_head->x5D = 0;
                    play_head->x4C++;
                }

                val = (*play_head->x4C >> shift_table_2[play_head->x5D]) & 0xF;
                play_head->x5D++;
                if (val == 0)
                {
                    val = *play_head->x50;
                    play_head->x50++;
                    if (val == 0)
                    {
                        val = *play_head->x54++;
                        if (val == 0)
                            val = *play_head->x58++;
                    }
                    else if ((val >  0) && (val < 9)) val += 0x7f;
                    else if ((val > -9) && (val < 0)) val -= 0x80;
                }
                else val = value_table_2[val];
            }
            else val = value_table_1[val];

            switch (j)
            {
                case 0: trans_rot_data->quat.x += val; break;
                case 1: trans_rot_data->quat.y += val; break;
                case 2: trans_rot_data->quat.z += val; break;
                case 3: trans_rot_data->quat.w += val; break;
                case 4: trans_rot_data->trans.x += val; break;
                case 5: trans_rot_data->trans.y += val; break;
                case 6: trans_rot_data->trans.z += val; break;
            }
        }
    }
}

void enb_get_trans_rot_unscaled_backward(enb_play_head* play_head) // 0x08A090A0 in ULJM05681
{
    int j;
    int val;
    int i;

    trans_rot* trans_rot_data = play_head->x10;

    trans_rot_data += (size_t)(play_head->x0C->x04 - 1);
    for (i = play_head->x0C->x04 - 1; i >= 0; i--, trans_rot_data--)
    {
        if (trans_rot_data->flags == 0) continue;

        for (j = 6; j >= 0; j--)
        {
            if ((trans_rot_data->flags & (1 << j)) == 0) continue;

            play_head->x5C--;
            if (play_head->x5C == 0xFF)
            {
                play_head->x5C = 3;
                play_head->x48--;
            }

            val = (*play_head->x48 >> shift_table_1[play_head->x5C]) & 0x03;
            if (val == 2)
            {
                play_head->x5D--;
                if (play_head->x5D == 0xFF)
                {
                    play_head->x5D = 1;
                    play_head->x4C--;
                }

                val = *play_head->x4C >> shift_table_2[play_head->x5D];
                val &= 0x0F;

                if (val == 0)
                {
                    val = *--play_head->x50;
                    if (val == 0)
                    {
                        val = *--play_head->x54;
                        if (val == 0)
                            val = *--play_head->x58;
                    }
                    else if ((val >  0) && (val < 9)) val += 0x7f;
                    else if ((val > -9) && (val < 0)) val -= 0x80;
                }
                else val = value_table_2[val];
            }
            else val = value_table_1[val];

            switch (j)
            {
                case 0: trans_rot_data->quat.x -= val; break;
                case 1: trans_rot_data->quat.y -= val; break;
                case 2: trans_rot_data->quat.z -= val; break;
                case 3: trans_rot_data->quat.w -= val; break;
                case 4: trans_rot_data->trans.x -= val; break;
                case 5: trans_rot_data->trans.y -= val; break;
                case 6: trans_rot_data->trans.z -= val; break;
            }
        }
    }
}

void enb_calc_params_init(enb_play_head* play_head) // 0x08A0931C in ULJM05681
{
    uint val;
    int mode;

    if (play_head->x70 == 4)
    {
        play_head->x70 = 0;
        play_head->x60++;
    }

    mode = (*play_head->x60 >> shift_table_1[play_head->x70]) & 0x3;
    play_head->x70++;
    val = 0;
    switch (mode)
    {
        case 1: val = *play_head->x64++; break;
        case 2: val = *play_head->x68++; break;
        case 3: val = *play_head->x6C++; break;
    }
    play_head->x28 = val;
    play_head->x2C = 0;
}

void enb_calc_params_forward(enb_play_head* play_head) // 0x08A09404 in ULJM05681
{
    int i, mode, track;
    uint j, temp, val;

    trans_rot* trans_rot_data = play_head->x10;

    track = play_head->x0C->x04 * 7;
    i = 0;
    while (i < track)
    {
        j = play_head->x28;
        if (j == 0)
        {
            trans_rot_data[i / 7].flags ^= 1 << (i % 7);
            if (play_head->x70 == 4)
            {
                play_head->x70 = 0;
                play_head->x60++;
            }

            mode = (*play_head->x60 >> shift_table_1[play_head->x70]) & 0x3;
            play_head->x70++;
            val = 0;
            switch (mode)
            {
                case 1: val = *play_head->x64; play_head->x64++; break;
                case 2: val = *play_head->x68; play_head->x68++; break;
                case 3: val = *play_head->x6C; play_head->x6C++; break;
            }
            play_head->x28 = val;
            play_head->x2C = 0;
            i++;
        }
        else
        {
            temp = j < (uint)(track - i) ? j : (uint)(track - i);
            i += (int)temp;
            play_head->x28 -= temp;
            play_head->x2C += temp;
        }
    }
}

void enb_calc_params_backward(enb_play_head* play_head) // 0x08A0968C in ULJM05681
{
    int i, mode, tracks;
    uint j, temp, val;

    trans_rot* trans_rot_data = play_head->x10;

    tracks = play_head->x0C->x04 * 7;
    i = tracks - 1;
    while (i >= 0)
    {
        j = play_head->x2C;
        if (j == 0)
        {
            trans_rot_data[i / 7].flags ^= 1 << (i % 7);
            play_head->x70--;
            if (play_head->x70 == 0xFF)
            {
                play_head->x70 = 3;
                play_head->x60--;
            }

            mode = (*play_head->x60 >> shift_table_1[play_head->x70]) & 0x3;
            val = 0;
            switch (mode)
            {
                case 1: val = *--play_head->x64; break;
                case 2: val = *--play_head->x68; break;
                case 3: val = *--play_head->x6C; break;
            }
            play_head->x28 = 0;
            play_head->x2C = val;
            i--;
        }
        else
        {
            temp = j < (uint)(i + 1) ? j : (uint)(i + 1);
            i -= (int)temp;
            play_head->x28 += temp;
            play_head->x2C -= temp;
        }
    }
}

void enb_calc_trans_rot_init(enb_play_head* play_head) // 0x08A086CC in ULJM05681
{
    int i;
    byte* flags;
    vec4 C010, C100, C200;
    vec3 C020, C110;
    float S030;

    trans_rot* trans_rot_data = play_head->x10;

    flags = play_head->x30;

    C200.x = C200.y = C200.z = C200.w = 0.0f;
    S030 = play_head->x0C->x08;
    for (i = 0; i < play_head->x0C->x04; i++)
    {
        C010 = trans_rot_data->quat; C020 = trans_rot_data->trans;

        C100.x = C010.x * S030;
        C100.y = C010.y * S030;
        C100.z = C010.z * S030;
        C100.w = C010.w * S030;
        C110.x = C020.x * S030;
        C110.y = C020.y * S030;
        C110.z = C020.z * S030;

        normalize_vec4(&C100);

        trans_rot_data->qtt[0].quat = C100; trans_rot_data->qtt[0].trans = C110; trans_rot_data->qtt[0].time = 0.0f;
        trans_rot_data->qtt[1].quat = C100; trans_rot_data->qtt[1].trans = C110; trans_rot_data->qtt[1].time = 0.0f;
        trans_rot_data->quat  = C200;
        trans_rot_data->trans.x = C200.x;
        trans_rot_data->trans.y = C200.y;
        trans_rot_data->trans.z = C200.z;
        trans_rot_data->flags = *flags++;

        trans_rot_data++;
    }
    play_head->xA9 = 0;
}

void enb_calc_trans_rot(enb_play_head* play_head, float time, bool forward) // 0x08A085D8 in ULJM05681
{
    int i, s0, s1;
    vec4 C010, C100, C120;
    vec3 C020, C110, C130;
    float S030;

    trans_rot* trans_rot_data = play_head->x10;

    if (forward) { s1 = play_head->xA9; s0 = play_head->xA9 ^= 1; }
    else         { s0 = play_head->xA9; s1 = play_head->xA9 ^= 1; }

    S030 = forward ? play_head->x0C->x08 : -play_head->x0C->x08;
    for (i = 0; i < play_head->x0C->x04; i++, trans_rot_data++)
    {
        C010 = trans_rot_data->quat; C020 = trans_rot_data->trans;
        C120 = trans_rot_data->qtt[s0].quat; C130 = trans_rot_data->qtt[s0].trans;

        C100.x = C010.x * S030 + C120.x;
        C100.y = C010.y * S030 + C120.y;
        C100.z = C010.z * S030 + C120.z;
        C100.w = C010.w * S030 + C120.w;
        C110.x = C020.x * S030 + C130.x;
        C110.y = C020.y * S030 + C130.y;
        C110.z = C020.z * S030 + C130.z;

        normalize_vec4(&C100);

        trans_rot_data->qtt[s1].quat  = C100;
        trans_rot_data->qtt[s1].trans = C110;
        trans_rot_data->qtt[s1].time  = time;
    }
}

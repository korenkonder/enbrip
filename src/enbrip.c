/*
    by korenkonder
    GitHub/GitLab: korenkonder
*/

#include "enbrip.h"

int main(int argc, char** argv) {
    FILE* file_in, * file_out;
    char* file_in_name, * file_out_name, * p;
    uint8_t* file_in_data, * file_out_data;
    int32_t code, frames;
    size_t file_in_len, file_in_name_len, file_out_len, file_out_name_len;
    float duration, fps;
    int32_t method;

    file_in = file_out = (FILE*)0;
    file_in_name = file_out_name = p = (char*)0;
    file_in_data = file_out_data = (uint8_t*)0;
    code = frames = 0;
    file_in_len = file_in_name_len = file_out_len = file_out_name_len = 0;
    duration = fps = 0.0f;
    method = QUAT_TRANS_INTERP_NONE;

    if (argc < 2 || argc > 4) {
        printf("Usage: enbrip <Enbaya file> [fps] [interpolation method]\n");
        printf("\nInterpolation method:\n  0: None\n  1: Lerp\n  2: Slerp\n");
        printf("\nDefault fps: 30.0\nDefault interpolation method: 2 (Slerp)\n");
        return -1;
    }

    if (argc > 2)
        fps = (float)atof(argv[2]);
    else
        fps = 30.0f;

    if (argc > 3) {
        method = atoi(argv[3]);
        if (method < QUAT_TRANS_INTERP_NONE || method > QUAT_TRANS_INTERP_SLERP)
            method = QUAT_TRANS_INTERP_SLERP;
    }
    else
        method = QUAT_TRANS_INTERP_SLERP;

    file_in_name_len = (int)strlen(argv[1]);
    p = strrchr(argv[1], '.');

    if (p)
        file_out_name_len = (int)(p - argv[1]);
    else
        file_out_name_len = 0;

    file_in_name  = (char*)malloc(file_in_name_len + 1);
    file_out_name = (char*)malloc(file_out_name_len + 6);

    if (!file_in_name)
        exit(cant_allocate, "file_in", -2)
    if (!file_out_name)
        exit(cant_allocate, "file_out", -3)

    memcpy(file_in_name, argv[1], file_in_name_len + 1);
    memcpy(file_out_name, argv[1], file_out_name_len);
    memcpy(file_out_name + file_out_name_len, ".rtrd", 6);

    if (fopen_s(&file_in, file_in_name, "rb") || !file_in)
        exit("Can't open file \"%s\" for read", file_in_name, -4)

    fseek(file_in, 0, SEEK_END);
    file_in_len = ftell(file_in);
    fseek(file_in, 0, SEEK_SET);

    file_in_data = (uint8_t*)malloc(file_in_len);
    if (!file_in_data)
        exit(cant_allocate, "file_in_data", -5)

    if (fread(file_in_data, 1, file_in_len, file_in) != file_in_len)
        if (fclose(file_in))
            exit("Can't read entire file \"%s\"and close it\n", file_in_name, -6)
        else
            exit("Can't read entire file \"%s\"\n", file_in_name, -7)

    if (fclose(file_in))
        exit("Can't close input file \"%s\"\n", file_in_name, -8)

    code = enb_process(file_in_data, &file_out_data, &file_out_len, &duration, &fps, &frames, (quat_trans_interp_method)method);
    if (code) {
        code -= 100;
        goto End;
    }

    if (fopen_s(&file_out, file_out_name, "wb") || !file_out)
        exit("Can't open file \"%s\" for write\n", file_out_name, -9)

    if (fwrite(file_out_data, 1, file_out_len, file_out) != file_out_len)
        if (fclose(file_out))
            exit("Can't write entire file \"%s\" and close it\n", file_out_name, -10)
        else
            exit("Can't write entire file \"%s\"\n", file_out_name, -11)

    if (fclose(file_out))
        exit("Can't close output file \"%s\"\n", file_out_name, -12)

    printf("Processed \"%s\" to \"%s\"\n", file_in_name, file_out_name);
    printf("Duration: %f; FPS: %f; Frames: %d\n", duration, fps, frames);
    code = 0;

End:
    free(file_in_name);
    free(file_out_name);
    free(file_in_data);
    free(file_out_data);
    file_in = 0;
    file_out = 0;
    p = 0;
    return code;
}

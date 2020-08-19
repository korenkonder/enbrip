/*
    by korenkonder
    GitHub/GitLab: korenkonder
*/

#include <stdio.h>
#include <string.h>
#include "enbaya.h"

#define Exit(message, val, err_code) { printf(message, val); code = err_code; goto End; }

int main(int argc, char** argv)
{
    FILE* file_in, * file_out;
    char* file_in_name, * file_out_name, * p;
    uint8_t* file_in_data, * file_out_data;
    int32_t code;
    size_t file_in_len, file_in_name_len, file_out_len, file_out_name_len, frames;
    float duration, fps;

    file_in = file_out = (FILE*)0;
    file_in_name = file_out_name = p = (char*)0;
    file_in_data = file_out_data = (uint8_t*)0;
    code = 0;
    file_in_len = file_in_name_len = file_out_len = file_out_name_len = frames = 0;
    duration = fps = 0.0f;

    if (argc != 2 && argc != 3)
    {
        printf("Usage: enbrip <Enbaya file> [fps]\nDefault fps: 30.0\n");
        return -1;
    }

    if (argc == 2)
        fps = 30.0f;
    else
        fps = (float)atof(argv[2]);

    file_in_name_len = (int)strlen(argv[1]);
    p = strrchr(argv[1], '.');

    if (p)
        file_out_name_len = (int)(p - argv[1]);
    else
        file_out_name_len = 0;

    file_in_name  = (char*)malloc(file_in_name_len + 1);
    file_out_name = (char*)malloc(file_out_name_len + 6);

    if (!file_in_name)
        Exit(cant_allocate, "file_in", -2)
    if (!file_out_name)
        Exit(cant_allocate, "file_out", -3)

    memcpy(file_in_name, argv[1], file_in_name_len + 1);
    memcpy(file_out_name, argv[1], file_out_name_len);
    memcpy(file_out_name + file_out_name_len, ".rtrd", 6);

    if (fopen_s(&file_in, file_in_name, "rb") || !file_in)
        Exit("Can't open file \"%s\" for read", file_in_name, -4)

    fseek(file_in, 0, SEEK_END);
    file_in_len = ftell(file_in);
    fseek(file_in, 0, SEEK_SET);

    file_in_data = (uint8_t*)malloc(file_in_len);
    if (!file_in_data)
        Exit(cant_allocate, "file_in_data", -5)

    if (fread(file_in_data, 1, file_in_len, file_in) != file_in_len)
        if (fclose(file_in))
            Exit("Can't read entire file \"%s\"and close it\n", file_in_name, -6)
        else
            Exit("Can't read entire file \"%s\"\n", file_in_name, -7)

    if (fclose(file_in))
        Exit("Can't close input file \"%s\"\n", file_in_name, -8)

    code = enb_process(file_in_data, &file_out_data, &file_out_len, &duration, &fps, &frames);
    if (code)
    {
        code -= 100;
        goto End;
    }

    if (fopen_s(&file_out, file_out_name, "wb") || !file_out)
        Exit("Can't open file \"%s\" for write\n", file_out_name, -9)

    if (fwrite(file_out_data, 1, file_out_len, file_out) != file_out_len)
        if (fclose(file_out))
            Exit("Can't write entire file \"%s\" and close it\n", file_out_name, -10)
        else
            Exit("Can't write entire file \"%s\"\n", file_out_name, -11)

    if (fclose(file_out))
        Exit("Can't close output file \"%s\"\n", file_out_name, -12)

    printf("Processed \"%s\" to \"%s\"\n", file_in_name, file_out_name);
    printf("Duration: %f; FPS: %f; Frames: %d\n", duration, fps, frames);
    code = 0;

End:
    free(file_in_name)
    free(file_out_name)
    free(file_in_data)
    free(file_out_data)
    file_in = 0;
    file_out = 0;
    p = 0;
    return code;
}

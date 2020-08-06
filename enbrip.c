/*
    by korenkonder
    GitHub/GitLab: korenkonder
*/

#include <stdio.h>
#include <string.h>
#include "enbaya.h"

#define Exit(message, err_code) { printf(message); code = err_code; goto End; }
#define ExitFormat(message, val, err_code) { printf(message, val); code = err_code; goto End; }

int main(int argc, char** argv)
{
    FILE* file_in, * file_out;
    char* file_in_name, * file_out_name, * p;
    byte* file_in_data, * file_out_data;
    int code, file_in_len, file_in_name_len, file_out_len, file_out_name_len, frames;
    float duration, fps;

    file_in = file_out = 0;
    file_in_name = file_out_name = p = 0;
    file_in_data = 0;
    file_out_data = 0;
    code = file_in_len = file_in_name_len = file_out_len = file_out_name_len = frames = 0;
    duration = fps = 0.0;

    if (argc != 2 && argc != 3)
        Exit("Usage: enbrip <Enbaya file> [fps]\nDefault fps: 30.0\n", -1)

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

    file_in_name  = (char*)malloc((size_t)file_in_name_len + 1);
    file_out_name = (char*)malloc((size_t)file_out_name_len + 5);

    if (!file_in_name)
        ExitFormat(cant_allocate, "file_in", -2)
    if (!file_out_name)
        ExitFormat(cant_allocate, "file_out", -3)

    memcpy(file_in_name, argv[1], (size_t)file_in_name_len + 1);
    memcpy(file_out_name, argv[1], file_out_name_len);
    memcpy(file_out_name + file_out_name_len, ".rtrd", 6);

    file_in = fopen(file_in_name, "rb");
    if (!file_in)
        ExitFormat("Can't open file \"%s\" for read", file_in_name, -4)

    fseek(file_in, 0, SEEK_END);
    file_in_len = ftell(file_in);
    fseek(file_in, 0, SEEK_SET);

    file_in_data = (byte*)malloc(file_in_len);
    if (!file_in_data)
        ExitFormat(cant_allocate, "file_in_data", -5)

    if (fread(file_in_data, 1, file_in_len, file_in) != file_in_len)
        ExitFormat("Can't read entire file \"%s\"\n", file_in_name, -6)

    if (fclose(file_in))
        ExitFormat("Can't close input file \"%s\"\n", file_in_name, -7)

    code = enb_process(file_in_data, &file_out_data, &file_out_len, &duration, &fps, &frames);
    if (code)
    {
        code -= 100;
        goto End;
    }

    file_out = fopen(file_out_name, "wb");
    if (!file_out)
        ExitFormat("Can't open file \"%s\" for write\n", file_out_name, -8)

    if (fwrite(file_out_data, 1, file_out_len, file_out) != file_out_len)
        ExitFormat("Can't write entire file \"%s\"\n", file_in_name, -9)

    if (fclose(file_out))
        ExitFormat("Can't close output file \"%s\"\n", file_out_name, -10)

    printf("Processed \"%s\" to \"%s\"\n", file_in_name, file_out_name);
    printf("Duration: %f; FPS: %f; Frames: %d\n", duration, fps, frames);
    code = 0;

End:
    FREE(file_in_name);
    FREE(file_out_name);
    FREE(file_in_data);
    file_in = 0;
    file_out = 0;
    p = 0;
    return code;
}

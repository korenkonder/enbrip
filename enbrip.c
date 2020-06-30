/*
    by korenkonder
    GitHub/GitLab: korenkonder
*/

#include <stdio.h>
#include <string.h>
#include "enbaya.h"

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

    if (argc != 2 && argc != 3) { printf("Usage: enbrip <Enbaya file> [fps]\nDefault fps: 30.0\n"); code = -1; goto End; }

    if (argc == 2) fps = 30.0f;
    else fps = (float)atof(argv[2]);

    file_in_name_len = (int)strlen(argv[1]);
    p = strrchr(argv[1], '.');
    if (p) file_out_name_len = (int)(p - argv[1]);
    else file_out_name_len = 0;

    file_in_name  = (char*)malloc((size_t)file_in_name_len + 1);
    file_out_name = (char*)malloc((size_t)file_out_name_len + 5);

         if (!file_in_name ) { printf(cant_allocate); printf("\"file_in\"\n" ); code = -2; goto End; }
    else if (!file_out_name) { printf(cant_allocate); printf("\"file_out\"\n"); code = -3; goto End; }

    memcpy(file_in_name, argv[1], (size_t)file_in_name_len + 1);
    memcpy(file_out_name, argv[1], file_out_name_len);
    memcpy(file_out_name + file_out_name_len, ".rtrd", 6);

    file_in = fopen(file_in_name, "rb");
    if (!file_in) { printf("Can't open file \"%s\" for read\n", file_in_name); code = -4; goto End; }
    
    fseek(file_in, 0, SEEK_END);
    file_in_len = ftell(file_in);
    fseek(file_in, 0, SEEK_SET);

    file_in_data = (byte*)malloc(file_in_len);
    if (!file_in_data) { printf(cant_allocate); printf("\"file_in_data\"\n"); code = -5; goto End; }

    if (fread(file_in_data, 1, file_in_len, file_in) != file_in_len)
    { printf("Can't read entire file \"%s\"\n", file_in_name); code = -6; goto End; }

    if (fclose(file_in)) { printf("Can't close input file \"%s\"\n", file_in_name); code = -7; goto End; }
    
    code = enb_process(file_in_data, &file_out_data, &file_out_len, &duration, &fps, &frames);
    if (code) { code -= 100; goto End; }

    file_out = fopen(file_out_name, "wb");
    if (!file_out) { printf("Can't open file \"%s\" for write\n", file_out_name); code = -8; goto End; }
    
    if (fwrite(file_out_data, 1, file_out_len, file_out) != file_out_len)
    { printf("Can't write entire file \"%s\"\n", file_in_name); code = -9; goto End; }

    if (fclose(file_out)) { printf("Can't close output file \"%s\"\n", file_out_name); code = -10; goto End; }
    
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

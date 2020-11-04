/*
    by korenkonder
    GitHub/GitLab: korenkonder
*/

#pragma once

#include <stdio.h>
#include <string.h>
#include "enbaya.h"

#define exit(message, val, err_code) { printf(message, val); code = err_code; goto End; }

static const char* is_null = "\"%s\" is null\n";
static const char* cant_allocate = "Can't allocate memory for \"%s\"\n";
static const char* cant_allocate_inner = "Can't allocate memory for %s\n";

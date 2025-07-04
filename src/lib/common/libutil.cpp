/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include "libutil.h"
#include <stdio.h>
#include <stdarg.h>

#ifdef _DEBUG
static int s_log_level = kLogAll;
#else
static int s_log_level = kLogError;
#endif
static FILE* s_log_file = nullptr;

void xse_log(int level, const char* format, ...)
{
    if (level > s_log_level) return;
    va_list list;
    va_start(list, format);
    vfprintf(s_log_file ? s_log_file : stdout, format, list);
}

void xse_log_level(int level)
{
    s_log_level = level;
}

void xse_log_file(const char* fname)
{
    if (s_log_file) fclose(s_log_file);
    s_log_file = fopen(fname, "w");
}

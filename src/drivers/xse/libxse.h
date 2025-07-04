/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#ifndef __LIBXSE_H__
#define __LIBXSE_H__

#include "xse_def.h"

#define XSE_DEBUG 10

#if XSE_DEBUG > 0
#define xse_log printk
#else
#define xse_log printk
#endif

#define xse_err pr_err

#endif

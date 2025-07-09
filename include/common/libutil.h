/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#ifndef _LIBUTIL_H__
#define _LIBUTIL_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    kErrorSuccess = 0,
    kErrorUnknownException = -1,
    kErrorInvalidArgument = -2,
    kErrorNotInitialized = -3,
    kErrorUnavailableChannel = -4,
    kErrorInvalidAddress = -5,
    kErrorInvalidData = -6,
    kErrorDeviceOpenFailed = -7,
    kErrorIoctlFailed = -8,
    kErrorSeekFailed = -9,
    kErrorWriteRegFailed = -10,
    kErrorReadRegFailed = -11,
    kErrorInitializationFailed = -12,
    kErrorAlreadyInitialized = -13,
    kErrorRelationOverflow = -14,
    kErrorUnimplemented = -15,
    kErrorConnectFailed = -16,
    kErrorListenFailed = -17,
    kErrorAcceptFailed = -18,
    kErrorDisconnectFailed = -19,
    kErrorInvalidOperation = -20,
} xse_status_t;

typedef enum {
    kLogFatal = 0,
    kLogError,
    kLogWarn,
    kLogInfo,
    kLogDebug,
    kLogAll,
} xse_log_level_t;

void xse_log(int level, const char* format, ...);
void xse_log_level(int level);
void xse_log_file(const char* fname);

#ifdef __cplusplus
}
#endif

#endif // _LIBUTIL_H__

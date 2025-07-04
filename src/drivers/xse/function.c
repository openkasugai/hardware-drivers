/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the GPL-2.0  License, see LICENSE for details.
* SPDX-License-Identifier: GPL-2.0-or-later
*************************************************/

#include "libxse.h"
#include "device_info.h"
#include "function.h"

static const struct file_operations s_function_ops = {
    .owner = THIS_MODULE,
    .open = xse_device_file_open,
    .release = xse_device_file_close,
    .llseek = xse_device_file_seek,
    .read = xse_device_read,
    .write = xse_device_write,
};

int xse_function_init(struct xse_pci_dev* xse_pdev, struct xse_device_info* info)
{
    int ret = 0;
    int pos = 0;
    char name[MAX_NAME_SIZE];

    if (!info) return 0;
    if (info->offset == DEVICE_INFO_OFFSET) {
        release_device_info(info);
        return 0;
    }

    while (pos < MAX_NAME_SIZE && info->name[pos] != '/') {
        name[pos] = info->name[pos];
        pos++;
    }
    name[pos] = '\0';

    ret = xse_cdev_init(xse_pdev, &xse_pdev->custom_functions[xse_pdev->function_num], name,
                        DEVICE_MINOR_FUNCTION + xse_pdev->function_num, info, &s_function_ops, 0, 1);
    if (ret) return ret;

    xse_pdev->function_num++;

    return 0;
}

void xse_function_exit(struct xse_pci_dev* xse_pdev)
{
    int function_num;
    int id;
    function_num = xse_pdev->function_num;
    for (id = 0; id<function_num; id++) {
        struct xse_cdev* cdev = &xse_pdev->custom_functions[id];
        xse_cdev_exit(cdev);
        if (cdev->module_info_owner) {
            struct xse_device_info *info = cdev->module_info;
            kfree(info);
        }
    }
}

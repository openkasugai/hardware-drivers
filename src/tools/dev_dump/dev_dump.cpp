/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <libutil.h>
#include <string>
#include <cstdint>

int help(char* exec)
{
    std::string exec_str(exec);
    auto pos = exec_str.rfind("/");
    if (pos != std::string::npos) {
        exec_str = exec_str.substr(pos+1);
    }
    printf("usage: %s <options>\n", exec_str.c_str());
    printf("\t-d <dev name>        : device file\n");
    printf("\t-a <address offset>  : eg) 0x100, 256\n");
    printf("\t-n <bytes>           : multiply of 4\n");
    printf("\t-h                   : show this message.\n");
    return -1;
}

xse_status_t print_regs(int fd, int offset, int bytes)
{
    int ret = lseek(fd, offset, SEEK_SET);
    printf("seek: %d\n", ret);
    for (int i=0; i<(bytes & ~3); i+=4) {
        uint32_t val;
        if (read(fd, &val, 4) == 4) {
            printf("[%4x]: %08x\n", offset + i, val);
        } else {
            return kErrorReadRegFailed;
        }
    }
    return kErrorSuccess;
}

xse_status_t write_reg(int fd, int offset, uint32_t data)
{
    int ret = lseek(fd, offset, SEEK_SET);
    printf("seek: %d\n", ret);
    if (ret == offset) {
        if (write(fd, &data, 4) == 4) return kErrorSuccess;
    }
    return kErrorWriteRegFailed;
}

int main(int argc, char** argv)
{
    int pos = 1;
    int bytes = 4;
    int offset = 0;
    uint32_t wdata = 0;
    bool is_write = false;
    std::string dev_name;

    while (pos < argc) {
        std::string arg(argv[pos++]);
        if (arg == "-d" && pos < argc) {
            dev_name = argv[pos++];
        } else if (arg == "-a" && pos < argc) {
            offset = std::stol(argv[pos++], NULL, 0);
        } else if (arg == "-n" && pos < argc) {
            bytes = std::stol(argv[pos++]);
        } else if (arg == "-w" && pos < argc) {
            wdata = std::stoul(argv[pos++], NULL, 0);
            is_write = true;
        } else if (arg == "-h") {
            return help(argv[0]);
        } else {
            printf("unknown option: %s\n", arg.c_str());
            return help(argv[0]);
        }
    }

    int fd = open(dev_name.c_str(), O_RDWR);
    if (fd < 0) {
        printf("Cannot open device file %s\n", dev_name.c_str());
        return -2;
    }
    xse_status_t stat;
    if (!is_write) {
        stat = print_regs(fd, offset, bytes);
    } else {
        stat = write_reg(fd, offset, wdata);
    }

    close(fd);
    if (stat) {
        printf("error: %d\n", stat);
    }
    return (int)stat;
}

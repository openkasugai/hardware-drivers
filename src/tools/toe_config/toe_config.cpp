/*************************************************
* Copyright 2025 NTT Corporation
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <toe_network_ioctl.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <libutil.h>
#include <string>

int help(char* exec)
{
    std::string exec_str(exec);
    auto pos = exec_str.rfind("/");
    if (pos != std::string::npos) {
        exec_str = exec_str.substr(pos+1);
    }
    printf("usage: %s <options>\n", exec_str.c_str());
    printf("\t-i <ip address>      : eg) 192.168.100.101\n");
    printf("\t-m <subnet mask>     : default is 255.255.255.0\n");
    printf("\t-g <default gateway> : eg) 192.168.100.1\n");
    printf("\t-p                   : print current info\n");
    printf("\t-d <dev id>          : fpga device index (default is 0)\n");
    printf("\t-n <instance id>     : instance index in the device (default is 0)\n");
    printf("\t-h                   : show this message.\n");
    return -1;
}

xse_status_t convert_str2int(const std::string& str, uint32_t& val)
{
    static const std::string delimiter(".");
    std::string cur(str);
    val = 0;
    int shift = 0;
    while (!cur.empty()) {
        std::string digit;
        size_t pos = cur.find(delimiter);
        if (pos != std::string::npos) {
            digit = cur.substr(0, pos);
            cur = cur.substr(pos + 1);
        } else {
            digit = cur;
            cur.clear();
        }
        try {
            int cur_val = std::stoul(digit, nullptr, 0);
            val = val | (cur_val << shift);
            shift += 8;
        } catch (...) {
            return kErrorInvalidArgument;
        }
    }
    printf("%s: %08x\n", str.c_str(), val);
    return kErrorSuccess;
}

xse_status_t toe_config(int fd, const std::string& ip, const std::string& mask, const std::string& gateway)
{
    toe_network_ioctl_config_t info;
    xse_status_t ret;
    ret = convert_str2int(ip, info.ip);
    if (ret) return ret;
    ret = convert_str2int(mask, info.subnet_mask);
    if (ret) return ret;
    ret = convert_str2int(gateway, info.default_gateway);
    if (ret) return ret;

    int stat = ioctl(fd, XSE_TOE_NETWORK_CONFIG, &info);
    printf("ioctl: %d\n", stat);
    if (stat == -EINVAL) return kErrorInvalidArgument;
    if (stat == -EPERM) return kErrorInvalidOperation;
    if (stat < 0) return kErrorUnknownException;
    return kErrorSuccess;
}

xse_status_t print_toe_config(int fd)
{
    for (int i=0; i<24; i+=4) {
        lseek(fd, SEEK_SET, i);
        uint32_t val;
        if (read(fd, &val, 4) == 4) {
            printf("[%4x]: %08x\n", i, val);
        }
    }
    return kErrorSuccess;
}

int main(int argc, char** argv)
{
    int pos = 1;
    int dev_id = 0;
    int instance_id = 0;
    bool is_print = false;
    std::string dev_name("/dev/xse");
    std::string toe_dev_name("_toe_network_");
    std::string ip, gateway;
    std::string mask("255.255.255.0");

    while (pos < argc) {
        std::string arg(argv[pos++]);
        if (arg == "-i" && pos < argc) {
            ip = argv[pos++];
        } else if (arg == "-m" && pos < argc) {
            mask = argv[pos++];
        } else if (arg == "-g" && pos < argc) {
            gateway = argv[pos++];
        } else if (arg == "-d" && pos < argc) {
            dev_id = std::stol(argv[pos++]);
        } else if (arg == "-n" && pos < argc) {
            instance_id = std::stol(argv[pos++]);
        } else if (arg == "-p") {
            is_print = true;
        } else if (arg == "-h") {
            return help(argv[0]);
        } else {
            printf("unknown option: %s\n", arg.c_str());
            return help(argv[0]);
        }
    }

    dev_name += std::to_string(dev_id) + toe_dev_name + std::to_string(instance_id);

    int fd = open(dev_name.c_str(), O_RDWR);
    if (fd < 0) {
        printf("Cannot open device file %s\n", dev_name.c_str());
        return -2;
    }
    xse_status_t stat;
    if (is_print) {
        stat = print_toe_config(fd);
    } else {
        stat = toe_config(fd, ip, mask, gateway);
    }

    close(fd);
    return (int)stat;
}

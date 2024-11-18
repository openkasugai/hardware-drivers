/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <libshmem_socket.h>
#include <liblogging.h>

#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <errno.h>


// LogLibFpga
#define LIBSHMEM_SOCKET "[SOCK] "
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBSHMEM LIBSHMEM_SOCKET


/**
 * static global variable: listen for shmem controller
 */
static int fd_listen = -1;


int fpga_shmem_get_fd_server(
  struct sockaddr_in *data,
  unsigned short port,
  const char *addr
) {
  llf_dbg("%s(data(%#llx), port(%d), addr(%s)\n", __func__, (uint64_t)data, port, addr);

  int ret;
  struct sockaddr_in server;
  socklen_t len;

  // socket
  if (fd_listen < 0) {
    fd_listen = socket(AF_INET, SOCK_STREAM, 0);
    if (fd_listen < 0) {
      int err = errno;
      llf_err(err, "socket() error\n");
      return -FAILURE_INITIALIZE;
    }

    // set reuse port setting
    bool flag = true;
    setsockopt(
      fd_listen,
      SOL_SOCKET,
      SO_REUSEADDR,
      (const char *)&flag,
      sizeof(flag));

    // bind
    server.sin_family = AF_INET;
    server.sin_port = htons(port);
    server.sin_addr.s_addr = inet_addr(addr);
    ret = bind(fd_listen, (struct sockaddr*)&server, sizeof(server));
    if (ret < 0) {
      int err = errno;
      llf_err(err, " bind  (%s:%hu) failure[%d]\n", addr, port, fd_listen);
      ret = -FAILURE_BIND;
      goto failed1;
    }
    llf_dbg(" bind  (%s:%hu) success[%d]\n", addr, port, fd_listen);

    // listen
    ret = listen(fd_listen, 16);
    if (ret < 0) {
      int err = errno;
      llf_err(err, " listen(%s:%hu) error[%d]\n", addr, port, fd_listen);
      ret = -FAILURE_ESTABLISH;
      goto failed1;
    }
    llf_dbg(" listen(%s:%hu) success[%d]\n", addr, port, fd_listen);
  }

  // accept
  len = sizeof(*data);
  int fd_accept = accept(fd_listen, (struct sockaddr*)data, &len);
  if (fd_accept < 0) {
    int err = errno;
    llf_err(err, " accept(%s:%hu) error[%d]\n", addr, port, fd_listen);
    ret = -FAILURE_ESTABLISH;
    goto failed1;
  }
  llf_dbg(" accept(%s:%hu) success[%d:%d]\n", addr, port, fd_listen, fd_accept);

  return fd_accept;

failed1:
  close(fd_listen);
  fd_listen = -1;

  return ret;
}


// cppcheck-suppress unusedFunction
void fpga_shmem_put_fd_server(void) {
  close(fd_listen);
  fd_listen = -1;
}


int fpga_shmem_get_fd_client(
  struct sockaddr_in *data,
  unsigned short port,
  const char *addr
) {
  llf_dbg("%s(data(%#llx), port(%d), addr(%s)\n", __func__, (uint64_t)data, port, addr);

  int ret;

  // socket
  int fd_connect = socket(AF_INET, SOCK_STREAM, 0);
  if (fd_connect < 0) {
    int err = errno;
    llf_err(err, "socket() error\n");
    return -FAILURE_INITIALIZE;
  }

  // connect
  data->sin_family = AF_INET;
  data->sin_port = htons(port);
  data->sin_addr.s_addr = inet_addr(addr);
  ret = connect(fd_connect, (struct sockaddr*)data, sizeof(*data));
  if (ret < 0) {
    int err = errno;
    llf_err(err, " connect(%s:%hu) error[%d]\n", addr, port, fd_connect);
    goto failed;
  }
  llf_dbg(" connect(%s:%hu) success[%d]\n", addr, port, fd_connect);

  return fd_connect;

failed:
  close(fd_connect);
  return -1;
}


static int __fpga_shmem_send(
  int fd,
  void *data,
  socklen_t len,
  int flag
) {
  int ret;

  // send header
  ret = send(fd, &len, sizeof(len), flag);
  if (ret <= 0) {
    int err = errno;
    llf_err(err, "  send header error\n");
    return -1;
  }
  llf_dbg("  send header(%dbyte)\n", ret);

  if (len == 0) {
//    llf_dbg(" header shows data len is %dbyte, so no need to send data\n", len);
    return 0;
  }

  // send data
  ret = send(fd, data, len, flag);
  if (ret <= 0) {
    int err = errno;
    llf_err(err, "  send data error\n");
    return -1;
  }
  llf_dbg("  send data  (%dbyte)\n", ret);

  return 0;
}


int fpga_shmem_send(
  int fd,
  void *data,
  socklen_t len
) {
  llf_dbg("[%s][%d]\n", __func__, fd);
  return __fpga_shmem_send(fd, data, len, 0);
}


static int __fpga_shmem_recv(
  int fd,
  void **pdata,
  socklen_t *plen,
  int flag
) {
  void *data;
  socklen_t len;
  int ret;

  // recv header
  ret = recv(fd, &len, sizeof(len), flag);
  if (ret == 0) {
    llf_dbg("  recv header failed...(connection lost)\n");
    return 1;
  } else if (ret < 0) {
    int err = errno;
    llf_err(err, "  recv header failed\n");
    return -1;
  }
  llf_dbg("  recv header(%dbyte)\n", ret);

  if (plen)
    *plen = len;
  if (len == 0) {
//    llf_dbg(" header shows data len is %dbyte, so no need to recv data\n", len);
    return 0;
  }
  data = *pdata = malloc(len);
  if (!*pdata) {
    llf_err(FAILURE_MEMORY_ALLOC, "  recv malloc failed\n");
    return -FAILURE_MEMORY_ALLOC;
  }

  // recv data
  ret = recv(fd, data, len, flag);
  if (ret == 0) {
    llf_dbg("  recv data failed...(connection lost)\n");
    return -1;
  } else if (ret < 0) {
    int err = errno;
    llf_err(err, "  recv data failed\n");
    return -1;
  }
  llf_dbg("  recv data  (%dbyte)\n", ret);

  return 0;
}


int fpga_shmem_recv(
  int fd,
  void **data,
  socklen_t *plen
) {
  llf_dbg("[%s][%d]\n", __func__, fd);

  return __fpga_shmem_recv(fd, data, plen, 0);
}

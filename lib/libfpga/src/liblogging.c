/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/

#include <liblogging.h>

#include <stdarg.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>


// LogLibFpga
#undef FPGA_LOGGER_LIBNAME
#define FPGA_LOGGER_LIBNAME LIBLOGGING


/**
 * @enum FPGA_LOGGER_TIMESTAMP
 * @brief Enumeration for status of timestamp
 */
enum FPGA_LOGGER_TIMESTAMP {
  FPGA_LOGGER_TIMESTAMP_INVALID,  /**< invalid value */
  FPGA_LOGGER_TIMESTAMP_ON,       /**< Enabled timestamp(default) */
  FPGA_LOGGER_TIMESTAMP_OFF,      /**< Disabled timestamp */
};

/**
 * @enum FPGA_LOGGER_STDOUT
 * @brief Enumeration for status of output target
 */
enum FPGA_LOGGER_STDOUT {
  FPGA_LOGGER_STDOUT_INVALID, /**< invalid value */
  FPGA_LOGGER_STDOUT_ON,      /**< Set output only stdout */
  FPGA_LOGGER_STDOUT_OFF,     /**< Set output file too(default) */
};

/**
 * @enum FPGA_LOGGER_CREATE_FILE
 * @brief Enumeration for status of file create
 */
enum FPGA_LOGGER_FILE {
  FPGA_LOGGER_FILE_INVALID, /**< invalid value */
  FPGA_LOGGER_FILE_CLOSED,  /**< log file is not opening */
  FPGA_LOGGER_FILE_OPENED,  /**< log file is opening */
  FPGA_LOGGER_FILE_REOPEN,  /**< set flag to close opening the log file and create a new log file */
};


/**
 * static global variable: long options for libfpga_log_parse_args()
 */
static const struct option
libfpga_log_long_options[] = {
  { "lib-loglevel", required_argument, NULL, 'l' },
  { "set-timestamp", no_argument , NULL, 'p' },
  { "quit-timestamp", no_argument , NULL, 't' },
  { "set-output-stdout", no_argument , NULL, 's' },
  { "set-output-file", no_argument , NULL, 'f' },
  { NULL, 0, 0, 0 },
};

/**
 * static global variable: short options for libfpga_log_parse_args()
 */
static const char
libfpga_log_short_options[] = {
  "l:ptsf"
};

/**
 * static global variable: libfpga's output loglevel
 */
static int libfpga_glevel = LIBFPGA_LOG_ERROR;

/**
 * static global variable: libfpga's timestamp's status
 */
static int libfpga_gtime = FPGA_LOGGER_TIMESTAMP_ON;

/**
 * static global variable: libfpga's output's status
 */
static int libfpga_stdout = FPGA_LOGGER_STDOUT_OFF;

/**
 * static global variable: libfpga's output loglevel
 */
static int libfpga_flag_create_file = FPGA_LOGGER_FILE_CLOSED;


void libfpga_log_set_level(
  int level
) {
  libfpga_glevel = level;
}


int libfpga_log_get_level(void) {
  return libfpga_glevel;
}


void libfpga_log_set_timestamp(void) {
  libfpga_gtime = FPGA_LOGGER_TIMESTAMP_ON;
}


void libfpga_log_quit_timestamp(void) {
  libfpga_gtime = FPGA_LOGGER_TIMESTAMP_OFF;
}


int libfpga_log_get_timestamp(void) {
  return libfpga_gtime == FPGA_LOGGER_TIMESTAMP_ON;
}


void libfpga_log_set_output_stdout(void) {
  libfpga_stdout = FPGA_LOGGER_STDOUT_ON;
}


void libfpga_log_quit_output_stdout(void) {
  libfpga_stdout = FPGA_LOGGER_STDOUT_OFF;
}


int libfpga_log_get_output_stdout(void) {
  return libfpga_stdout == FPGA_LOGGER_STDOUT_ON;
}


int libfpga_log_parse_args(
  int argc,
  char **argv
) {
  int opt, ret;
  char **argvopt;
  int option_index;
  char *prgname = argv[0];
  const int old_optind = optind;
  const int old_optopt = optopt;
  char * const old_optarg = optarg;

  argvopt = argv;
  optind = 1;
  opterr = 0;

  while ((opt = getopt_long(argc, argvopt, libfpga_log_short_options,
                libfpga_log_long_options, &option_index)) != EOF) {
    switch (opt) {
    case 'l': {
      libfpga_log_set_level(atoi(optarg));
      break;
    }
    case 'p': {
      libfpga_log_set_timestamp();
      break;
    }
    case 't': {
      libfpga_log_quit_timestamp();
      break;
    }
    case 's': {
      libfpga_log_set_output_stdout();
      break;
    }
    case 'f': {
      libfpga_log_quit_output_stdout();
      break;
    }
    default:
      llf_err(INVALID_ARGUMENT, "Invalid operation: unable to parse option[%s].\n", argvopt[optind - 1]);
      ret = -INVALID_ARGUMENT;
      goto out;
    }
  }

  if (optind >= 0)
    argv[optind - 1] = prgname;
  ret = optind - 1;

out:
  optind = old_optind;
  optopt = old_optopt;
  optarg = old_optarg;

  return ret;
}


void libfpga_log_reset_output_file(void) {
  libfpga_flag_create_file = FPGA_LOGGER_FILE_REOPEN;
}


/**
 * @brief print log
 * @remarks close() of the log file is not explicitly done.
 */
static int __log_libfpga(
  int level,
  FILE **file
) {
  FILE *fp = NULL;
  static FILE *fp_logfile = NULL;
  time_t t;
  char date[32];

  if (libfpga_stdout == FPGA_LOGGER_STDOUT_ON) {
    // output : only stdout
    fp = stdout;
  } else {
    // output : log file
    if (libfpga_flag_create_file == FPGA_LOGGER_FILE_REOPEN) {
      if (fp_logfile) {
        fclose(fp_logfile);
        fp_logfile = NULL;
      }
      libfpga_flag_create_file = FPGA_LOGGER_FILE_CLOSED;
    }
    if (libfpga_flag_create_file == FPGA_LOGGER_FILE_CLOSED) {
      // The logfile is not opened, so create logfile
      char filename[64];
      time(&t);
      strftime(date, sizeof(date), "%m%d-%H%M%S", localtime(&t));
      sprintf(filename, "%s%s.log", LOGFILE, date);  // NOLINT
      int index = 1;

      // When the same name log file is already exist, add suffix `(%d)`
      while ((fp_logfile = fopen(filename, "r"))) {
        fclose(fp_logfile);
        fp_logfile = NULL;
        sprintf(filename, "%s%s(%d).log", LOGFILE, date, index++);  // NOLINT
      }

      // Create log file
      fp_logfile = fopen(filename, "a");
      libfpga_flag_create_file = FPGA_LOGGER_FILE_OPENED;
      if (fp_logfile == NULL) {
        printf("file open error[%d]\n", errno);
        fflush(stdout);
        return -1;
      }
      fp = fp_logfile;
      printf("logfile = %s\n", filename);
      fflush(stdout);
      fprintf(fp, "log start...%s\n", date);
    } else {
      if (fp_logfile == NULL) {
        return -1;
      }
      fp = fp_logfile;
    }
  }

  // print timestamp
  if (libfpga_gtime == FPGA_LOGGER_TIMESTAMP_ON) {
    time(&t);
    strftime(date, sizeof(date), "%H:%M:%S", localtime(&t));
    fprintf(fp, "%s", date);
  }

  // print loglevel
  switch (level) {
  case LIBFPGA_LOG_PRINT:
    break;
  case LIBFPGA_LOG_ERROR:
    fprintf(fp, "[error] ");
    break;
  case LIBFPGA_LOG_WARN:
    fprintf(fp, "[warn]  ");
    break;
  case LIBFPGA_LOG_INFO:
    fprintf(fp, "[info]  ");
    break;
  case LIBFPGA_LOG_DEBUG:
    fprintf(fp, "[debug] ");
    break;
  default:
    fprintf(fp, "[?????] ");
    break;
  }

  // return fp through argument
  *file = fp;

  return 0;
}


int log_libfpga(
  int level,
  const char *format,
  ...
) {
  // Check if argument's loglevel is larger than setting loglevel(`libfpga_glevel`)
  if (level > libfpga_glevel) {
    return 0;
  }

  va_list args;
  FILE *fp;
  int ret;

  // Create log file if need, and print prefix(timestamp,loglevel) into the target output
  ret = __log_libfpga(level, &fp);
  if (ret < 0)
    return ret;

  // Print log into the target output
  va_start(args, format);
  vfprintf(fp, format, args);
  va_end(args);
  fflush(fp);

  // Print log into stdout without timestamp and loglevel
  // when argument's log level is `LIBFPGA_LOG_PRINT` and libfpga_log_set_output_stdout() is not called
  if ((level == LIBFPGA_LOG_PRINT) && (libfpga_stdout == FPGA_LOGGER_STDOUT_OFF)) {
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    fflush(stdout);
  }

  return 0;
}


int log_libfpga_cmdline_arg(
  int level,
  int argc,
  char **argv,
  const char *format,
  ...
) {
  // Check if argument's loglevel is larger than setting loglevel(`libfpga_glevel`)
  if (level > libfpga_glevel) {
    return 0;
  }

  va_list args;
  FILE *fp;
  int ret;

  // Create log file if need, and print prefix(timestamp,loglevel) into the target output
  ret = __log_libfpga(level, &fp);
  if (ret < 0)
    return ret;

  // Print log into the target output
  va_start(args, format);
  vfprintf(fp, format, args);
  va_end(args);

  // Print argc and argv into the target output
  fprintf(fp, "(argc(%d)", argc);
  for (int i = 0; i < argc; i++) {
    fprintf(fp, ", argv[%d](%s)", i, argv[i]);
  }
  fprintf(fp, ")\n");
  fflush(fp);

  // Print log into stdout without timestamp and loglevel
  // when argument's log level is `LIBFPGA_LOG_PRINT` and libfpga_log_set_output_stdout() is not called
  if (level == LIBFPGA_LOG_PRINT && (libfpga_stdout == FPGA_LOGGER_STDOUT_OFF)) {
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("(argc(%d)", argc);
    for (int i = 0; i < argc; i++) {
      printf(", argv[%d](%s)", i, argv[i]);
    }
    printf(")\n");
    fflush(stdout);
  }

  return 0;
}

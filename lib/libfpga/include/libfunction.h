/*************************************************
* Copyright 2024 NTT Corporation, FUJITSU LIMITED
* Licensed under the 3-Clause BSD License, see LICENSE for details.
* SPDX-License-Identifier: BSD-3-Clause
*************************************************/
/**
 * @file libfunction.h
 * @brief Header file for interface APIs with function setting libraries
 */

#ifndef LIBFPGA_INCLUDE_LIBFUNCTION_H_
#define LIBFPGA_INCLUDE_LIBFUNCTION_H_

#include <libfpgactl.h>

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif


/**
 * Example of json parameter format of the last element.
 * @param[in] param
 *   String of parameter name. Need double quotation as json format.
 * @param[in] format
 *   Same as printf. e.g.) c,s,d,ld,u,lu,f,04d
 *
 * LIBFUNCTION_PARAM_FMT0(width, s)
~~~{.json}
width:%s
~~~
 * LIBFUNCTION_PARAM_FMT0("width", s)
~~~{.json}
"width":%s
~~~
 */
#define LIBFUNCTION_PARAM_FMT0(param, fmt) #param ":%" #fmt

/**
 * Example of json parameter format of the element excluding the last.
 * @param[in] param
 *   String of parameter name. Need double quotation as json format.
 * @param[in] format
 *   Same as printf. e.g.) c,s,d,ld,u,lu,f,04d
 *
 * @li Invalid case : LIBFUNCTION_PARAM_FMT1(width, s) will be as follows:
~~~{.json}
width:%s,
~~~
 * @li Valid case : LIBFUNCTION_PARAM_FMT1("width", s) will be as follows:
~~~{.json}
"width":%s,
~~~
 */
#define LIBFUNCTION_PARAM_FMT1(param, fmt) LIBFUNCTION_PARAM_FMT0(param, fmt) ","


/**
 * @struct fpga_function_ops_t
 * @brief Struct for operation function to control function modules
 * @var fpga_function_ops_t::name
 *      function name, used for matching function
 *      @sa fpga_function_config()
 *      @sa fpga_function_load()
 *      @sa fpga_function_unload()
 *      @sa fpga_function_register()
 *      @sa fpga_function_unregister()
 * @var fpga_function_ops_t::init
 *      initialize function, called by fpga_function_init()@n
 *      This function is expected to be called once at first
 * @var fpga_function_ops_t::set
 *      setter function, called by fpga_function_set()@n
 *      This function is expected to be called as many times as you like
 *       after init() and before finish()
 * @var fpga_function_ops_t::get
 *      getter function, called by fpga_function_get()
 *      This function is expected to be called as many times as you like
 *       after init() and before finish()
 * @var fpga_function_ops_t::finish
 *      finalize function, called by fpga_function_finish()
 *      This function is expected to be called at the last after init()
 */
typedef struct fpga_function_ops {
  const char *name;
  int (*init)(uint32_t, uint32_t, const char*);
  int (*set)(uint32_t, uint32_t, const char*);
  int (*get)(uint32_t, uint32_t, char**);
  int (*finish)(uint32_t, uint32_t, const char*);
} fpga_function_ops_t;


/**
 * @struct fpga_func_err_prot_t
 * @brief Struct for Protocol error Information Structure
 * @var fpga_func_err_prot_t::prot_ch
 *      channel protocol error
 * @var fpga_func_err_prot_t::prot_len
 *      length protocol error
 * @var fpga_func_err_prot_t::prot_sof
 *      SOF protocol error
 * @var fpga_func_err_prot_t::prot_eof
 *      EOF protocol error
 * @var fpga_func_err_prot_t::prot_reqresp
 *      number of req/resp protocol error
 * @var fpga_func_err_prot_t::prot_datanum
 *      number of data protocol error
 * @var fpga_func_err_prot_t::prot_req_outstanding
 *      number of request outstanding protocol error
 * @var fpga_func_err_prot_t::prot_resp_outstanding
 *      number of response outstanding protocol error
 * @var fpga_func_err_prot_t::prot_max_datanum
 *      data maximum number error
 * @var fpga_func_err_prot_t::prot_reqlen
 *      req.length > 0 error
 * @var fpga_func_err_prot_t::prot_reqresplen
 *      req.length == resp.length error
 */
typedef struct fpga_func_err_prot {
  uint8_t prot_ch;
  uint8_t prot_len;
  uint8_t prot_sof;
  uint8_t prot_eof;
  uint8_t prot_reqresp;
  uint8_t prot_datanum;
  uint8_t prot_req_outstanding;
  uint8_t prot_resp_outstanding;
  uint8_t prot_max_datanum;
  uint8_t prot_reqlen;
  uint8_t prot_reqresplen;
} fpga_func_err_prot_t;


/**
 * @brief API which corresponds FPGA's module to function type
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] func_type
 *   Target module's function name@n
 *   hyphen('-') in `func_type` will be converted to under score('_') in library
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `lane` is too large, `func_type` is null
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 * @retval -INVALID_DATA
 *   e.g.) Not found function name(name converted hyphen to under score from `func_type`) in shared libraries list
 *
 * @details
 *   Search function operation mathcing function name with
 *    function name(name converted hyphen to under score from `func_type`)
 *    from function operations list.@n
 *   When you want to delete the correspondence, you need to call fpga_function_finish(),
 *    or call this API with ""(i.e. length is 0).@n
 *   When matching function operation is found, set it to function operation tables
 *    with using `dev_id`, `lane_id`.@n
 *   The functions which can be registered in default is as follows:
 *    @li filter_resize
 *   Whether these functions are registered in defalut or not depends on Makefile at building.@n
 *   If there were the same name's operations, the least recently one will be used.
 *   (normaly please do not register the same name's operations)
 */
int fpga_function_config(
        uint32_t dev_id,
        uint32_t lane,
        const char *func_type);

/**
 * @brief API which get function type of the target FPGA's module
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] func_type
 *   pointer variable to get the target module's function name
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `lane` is too large, `func_type` is null
 * @retval -INVALID_DATA
 *   e.g.) Not found function configuration data at the index(`dev_id` and `lane`)
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 *
 * @details
 *   Get configuration data's function name from `dev_id` and `lane`.
 *   The name will return with memory allocated by this library through `func_type`,
 *    so please explicitly free through free().@n
 *   The value of `*func_type` is undefined when this API fails.@n
 *   Please keep in mind that `func_type` will NOT be got from FPGA but from the table set by fpga_function_config().
 */
int fpga_function_get_config_name(
        uint32_t dev_id,
        uint32_t lane,
        char **func_type);

/**
 * @brief API which call initialize function of target module
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] json_txt
 *   e.g.) parameters
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -INVALID_DATA
 *   e.g.) The data of function operations table is `NULL`
 * @return Depends on each operations' fpga_function_ops_t::init()
 *
 * @details
 *   This API is interface of init() function in the function operations,
 *    init() will execute setting for`func_type` set by fpga_function_config()
 *    to the target module once at first.
 *   Get initialize function from function operations table by `dev_id` and `lane`,
 *    and call it with original arguments(`dev_id`, `lane`, `json_txt`).@n
 *   This API only check `dev_id` and `lane`, and call function,
 *    so the `retval` depends on each function operations' init().@n
 *   This API does not check `json_txt` because init() may not need parameter and allow `NULL`.
 */
int fpga_function_init(
        uint32_t dev_id,
        uint32_t lane,
        const char *json_txt);

/**
 * @brief API which call setter function of target module
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] json_txt
 *   e.g.) parameters
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -INVALID_DATA
 *   e.g.) The data of function operations table is `NULL`
 * @return Depends on each operations' fpga_function_ops_t::set()
 *
 * @details
 *   This API is interface of set() function in the function operations,
 *    set() will execute setting for `func_type` set by fpga_function_config()
 *    to the target module as many times as you like
 *    after fpga_function_init() and before fpga_function_finish().
 *   Get setter function from function operations table by `dev_id` and `lane`,
 *    and call it with original arguments(`dev_id`, `lane`, `json_txt`).@n
 *   This API only check `dev_id` and `lane`, and call function,
 *    so the `retval` depends on each function operations' set().@n
 *   This API does not check `json_txt` because set() may not need parameter and allow `NULL`.
 */
int fpga_function_set(
        uint32_t dev_id,
        uint32_t lane,
        const char *json_txt);

/**
 * @brief API which call getter function of target module
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[out] json_txt
 *   e.g.) parameters' json text
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -INVALID_DATA
 *   e.g.) The data of function operations table is `NULL`
 * @return Depends on each operations' fpga_function_ops_t::get()
 *
 * @details
 *   This API is interface of get() function in the function operations,
 *    get() will get data for `func_type` set by fpga_function_config()
 *    from the target module and free as many times as you like
 *    after fpga_function_init() and before fpga_function_finish().
 *   Get getter function from function operations table by `dev_id` and `lane`,
 *    and call it with original arguments(`dev_id`, `lane`, `json_txt`).@n
 *   This API only check `dev_id` and `lane`, and call function,
 *    so the `retval` depends on each function operations' get().@n
 *   `*json_txt` will return with memory allocated by this library and store in this library.
 *   Please be carefull that the memory will be freed()
 *    when fpga_function_finish() or fpga_function_get() is called with the same `dev_id` and `lane`.
 */
int fpga_function_get(
        uint32_t dev_id,
        uint32_t lane,
        char **json_txt);

/**
 * @brief API which call finalize function of target module
 * @param[in] dev_id
 *   FPGA's device id got by fpga_dev_init()
 * @param[in] lane
 *   Target lane of FPGA's module
 * @param[in] json_txt
 *   e.g.) parameters
 * @retval -INVALID_ARGUMENT
 *   e.g.) `dev_id` is invalid, `lane` is too large
 * @retval -INVALID_DATA
 *   e.g.) The data of function operations table is `NULL`
 * @return Depends on each operations' fpga_function_ops_t::finish()
 *
 * @details
 *   This API is interface of finish() function in the function operations,
 *    finish() will finalize for `func_type` set by fpga_function_config()
 *    to the target module at the last after fpga_function_init().
 *   Get finalize function from function operations table by `dev_id` and `lane`,
 *    and call it with original arguments(`dev_id`, `lane`, `json_txt`).@n
 *   This API only check `dev_id` and `lane`, and call function,
 *    so the `retval` depends on each function operations' finish().@n
 *   This API does not check `json_txt` because finish() may not need parameter and allow `NULL`.@n
 *   After calling this API, the correspondence between target module and function operations list will be removed,
 *    so if you need to set again as the same function type, please call fpga_function_config() again.
 */
int fpga_function_finish(
        uint32_t dev_id,
        uint32_t lane,
        const char *json_txt);

/**
 * @brief API which register function operations into the function operations list
 * @param[in] ops
 *   Target function operations
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `ops` is null, `ops->name` is null
 * @retval -FULL_ELEMENT
 *   e.g.) function operation list has no free region
 * @retval -ALREADY_ASSIGNED
 *   e.g.) function operation list has the same function operation
 *   
 * @details
 *   Set operations to be settable by fpga_function_config()
 *   (i.e. set into the function operations list).@n
 *   This API can use only global variable in the library.
 */
int fpga_function_register(
        const fpga_function_ops_t *ops);

/**
 * @brief API which unregister function operations from the function operations list
 * @param[in] name
 *   Target function operations' name
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `ops` is null, `ops->name` is null
 * @retval -INVALID_DATA
 *   e.g.) Not found mathcing data in the function operations list
 *   
 * @details
 *   Set NULL to the function operations list at the index whose name(`fpga_function_ops_t::name`) is the same
 *    with the argument(`name`).@n
 *   This API does not remove the operations from the function operations table.
 */
int fpga_function_unregister(
        const char *name);

/**
 * @brief API which register function operations into function operations list by using shared library
 * @param[in] library_name
 *   Target function's name used as library name
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `library_name` is null, `library_name` >= FILENAME_MAX
 * @retval -FULL_ELEMENT
 *   e.g.) function operation list has no free region
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 * @retval -FAILURE_OPEN
 *   e.g.) Mathing file name is not exist
 * @retval -INVALID_DATA
 *   e.g.) Failed to get a symbol
 * @return
 *   See also fpga_function_register_***() in the target library
 *
 * @details
 *   Search the target shared library from `library_name` and open it,
 *    get a symbol for the register function and call it.@n
 *   The way to search shared library is the same as dlopen().@n
 *   And this registers shared lib's name and handle into shared libs table.@n
 *   The target library's name rules are as follows:
 *    1. libfunction_<library_name>.so
 *    2. `library_name` must NOT include characters which means specific meanings in C language
 *       ('.', '*', ' ',...).
 *   The register function's rules are as follows:
 *    1. name : fpga_function_register_<library_name>
 *    2. retval : 0:success/else:failure
 *    3. arguments : void
 *    4. remarks : `func_type` of fpga_function_config should be same as `library_name`
 *   By converting `library_name` from hyphen to under score automatically in this library,
 *    `library_name` will allow '-' as valid character,
 *     on the other hand the register function will not allow '-'. 
 */
int fpga_function_load(
        const char *library_name);

/**
 * @brief API which unregister function operations from function operations list
 * @param[in] library_name
 *   Target function's name used as library name
 * @retval 0
 *   Success
 * @retval -INVALID_ARGUMENT
 *   e.g.) `library_name` is null, `library_name` >= FILENAME_MAX
 * @retval -FAILURE_MEMORY_ALLOC
 *   Failed to allocate memory
 * @retval -INVALID_DATA
 *   e.g.) Matching data is not found
 * @return
 *   @sa fpga_function_unregister()
 *
 * @details
 *   Search target shared library from shared libs table and close it, free temporary memory.@n
 *   If there are still target library's operations in the operations table,
 *    free fpga_function_get()'s memory and remove them without calling finish(),
 *    so please call finish() by user explicitly.@n
 *   If there are still target library's operations in the operations list, remove them.
 */
int fpga_function_unload(
        const char *library_name);

#ifdef __cplusplus
}
#endif

#endif  // LIBFPGA_INCLUDE_LIBFUNCTION_H_

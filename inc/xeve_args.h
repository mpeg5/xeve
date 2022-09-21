/* Copyright (c) 2022, Samsung Electronics Co., Ltd.
   All Rights Reserved. */
/*
   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   - Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   - Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

   - Neither the name of the copyright owner, nor the names of its contributors
   may be used to endorse or promote products derived from this software
   without specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
   ARE DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
   LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
   CONSEQUENTIAL DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
   SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
   INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
   CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
   ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
   POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef _XEVE_ARGS_H_
#define _XEVE_ARGS_H_

#ifdef __cplusplus

extern "C"
{
#endif

#include <xeve_exports.h>
#include <xeve.h>

#define ARGS_OPT_VALUE_MAXLEN         (256)

typedef struct _ARGS_PARSER ARGS_PARSER; // opaque struct

ARGS_PARSER * XEVE_EXPORT xeve_args_create(void);

/**
 * @brief Do cleanup.
 * The function must be called when ARGS_PARSER object is no more needed
 */
void XEVE_EXPORT xeve_args_release(ARGS_PARSER * args);

/**
 * @brief Initialize codec options
 * Initializes memory needed to store option values and creates a mapping between these values and codec params.
 * The function must be call right next to xeve_args_create is called.
 * After the function is called, setting a value for any valid option will affect corresponding codec param value.
 *
 * @param args
 * @param param codec params
 * @retval XEVE_OK if OK
 * @retval XEVE error code if Error
 */
int XEVE_EXPORT xeve_args_init(ARGS_PARSER * args, XEVE_PARAM* param);

/**
 * @brief Parse application input arguments
 *
 * @param args
 * @param argc number of command line argumnets (strings in argv)
 * @param argv table containing command line arguments (strings)
 * @param errstr
 * @return int
 */
int XEVE_EXPORT xeve_args_parse(ARGS_PARSER * args, int argc, const char* argv[], char ** errstr);

/**
 * @brief Returns string value for an option of a given key value
 *
 * @note Keep in mind that, there are a lot of options that have numeric values.
 * @note It's the user who is in charge of conversion to integer type if it is needed.
 * @param args
 * @param [in] keyl key value for an given option
 * @param [out] val string value for option of a given index
 *
 * @retval XEVE_ERR Option not initialized or set
 * @retval XEVE_ERR_INVALID_ARGUMENT if the option of a given key not exists or one of the arguments is invalid
 * @retval XEVE_OK Ok
 *
 * @attention The function allocates memory for string so user is in charge of releasing memory when no more needed
 */
int XEVE_EXPORT xeve_args_get_option_val(ARGS_PARSER * args, char* keyl,  char** val);

/**
 * @brief Set string value for an option of a given key value
 *
 * @note Keep in mind that, there are a lot of options that have numeric values.
 * @note It's the user who is in charge of conversion from integer type to string if it is needed.
 *
 * @param args
 * @param idx keyl key value for an given option
 * @param val string value for option of a given key
 * @retval XEVE_ERROR Option not initialized or set
 * @retval XEVE_ERR_INVALID_ARGUMENT if the option of a given key not exists or one of the arguments is invalid
 * @retval XEVE_OK Ok
 */
int XEVE_EXPORT xeve_args_set_option_val(ARGS_PARSER * args, char* keyl,  char* val);

/**
 * @brief Returns string containig help for an option of a given index
 *
 * @param [in] args
 * @param [in] keyl key value for an given option
 * @param [out] help String contating description for option of given option or NULL string if the option of given long name exists
 * @retval NULL No option of given long name exists
 *
 * @attention The function allocates memory for string so user is in charge of releasing memory when no more needed
 */
int XEVE_EXPORT xeve_args_get_option_description(ARGS_PARSER * args, char* keyl, char ** help);

///////////////////////////////////////////////////////////////////////////////
// HELPERS
///////////////////////////////////////////////////////////////////////////////

// The following functions must be deleted in the final implementation
// pomysl nad zmiana nazwy albo usun
/**
 * @brief Check whether all mandatory options have been set
 *
 * @param args
 * @param err_arg
 * @retval XEVE_ERROR Not all mandatory options have been set
 * @retval XEVE_OK Ok
 */
int XEVE_EXPORT xeve_args_check_mandatory(ARGS_PARSER * args, char ** err_arg);

/* iterator */
/**
 * @brief Return the next key value in the options container
 *
 * The function lets the user iterate through the options container and returns
 * the next key value after the one passed as an argument.
 * If the passed key value is NULL returns the first key value from the options container.
 *
 * @param args
 * @param key string containg key value or NULL
 *
 * @retval NULL if there is no next key in the options container
 * @retval key value Ok
 */
char* XEVE_EXPORT xeve_args_next_key(ARGS_PARSER * args, char* key);

#ifdef __cplusplus
extern "C"
{
#endif

#endif /*_XEVE_ARGS_H_ */



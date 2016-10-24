/*!
 * \copyright
 * Copyright 2013-2014 NVIDIA Corporation.  All rights reserved.
 *
 * NOTICE TO LICENSEE:
 *
 * This source code and/or documentation ("Licensed Deliverables") are
 * subject to the applicable NVIDIA license agreement that governs the
 * use of the Licensed Deliverables.
 */

#ifndef __HELPER_H__

#define __HELPER_H__

#include <stdio.h>

#include <time.h>

#define PRINT_WITH_TIMESTAMP(fmt_string,...)                           \
    do {                                                               \
        const time_t c_time = time(0);                                 \
        char * time_str = ctime(&c_time);                              \
                                                                       \
        fprintf(stdout, "[%.*s] " fmt_string,                          \
                (int)strlen(time_str)-1 , time_str, ##__VA_ARGS__);    \
        fflush(stdout);                                                \
    } while(0)

#define PRINT(fmt_string,...)                       \
    do {                                            \
        fprintf(stdout, fmt_string, ##__VA_ARGS__); \
        fflush(stdout);                             \
    } while (0)

#define PRINT_ERROR(fmt_string,...)                           \
    do {                                                      \
        fprintf(stderr, "Error: " fmt_string, ##__VA_ARGS__); \
        fflush(stderr);                                       \
    } while (0)

#define PRINT_ERROR_AND_EXIT(fmt_string,...)                  \
    do {                                                      \
        fprintf(stderr, "Error: " fmt_string, ##__VA_ARGS__); \
        fflush(stderr);                                       \
        exit(-1);                                             \
    } while(0)

#define EXIT_IF_FAILED(con_exp)                               \
    do {                                                      \
        if (!(con_exp)) {                                     \
            exit(-1);                                         \
        }                                                     \
    } while(0)

#endif

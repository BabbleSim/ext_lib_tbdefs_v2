/**
 * Copyright 2017-2018 Oticon A/S
 * SPDX-License-Identifier: MIT
 *
 * This file is a derivative of a work licensed to Oticon A/S under the
 * SPDX-License-Identifier: MIT
 * by Paul Emmanuel Wad, with Copyright of 2016-2018
 */

#ifndef TB_DEFS_UNIT_TEST_UTILS_H
#define TB_DEFS_UNIT_TEST_UTILS_H

// This file contains utilities needed for testing tb_defs.h.

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

//////////////////////////////////////////////////////////////////////////////////////////////////
// BabbleSim replacements

typedef uint64_t bs_time_t;
#define TIME_NEVER UINT64_MAX
#define BASE_TRACE_ERROR 1
#define BASE_TRACE_TIME_PROVIDED 1

#define bs_trace_print(_type, _file, _line, _verbosity, _time_type, _time, _fmt, ...) \
    { \
        char strbuf[20]; \
        fprintf(stderr, "%s: ERROR at line %u: " _fmt, bs_time_to_str(strbuf, (_time)), _line, ##__VA_ARGS__); \
        exit(1); \
    }

#define bs_trace_raw_time(_verbosity, _fmt, ...) \
    { \
        char strbuf[20]; \
        printf("%s: " _fmt, bs_time_to_str(strbuf, tm_get_hw_time()), ##__VA_ARGS__); \
    }

char *bs_time_to_str(char *dest, bs_time_t time);
bs_time_t tm_get_hw_time(void);
void bst_ticker_set_next_tick_absolute(bs_time_t t);
void bst_ticker_set_next_tick_delta(bs_time_t d);

//////////////////////////////////////////////////////////////////////////////////////////////////
// Unit testing stuff

typedef void (*tb_defs_unit_test_tick_handler_t)(bs_time_t t);
typedef void (*tb_defs_unit_test_event_handler_t)(void);

void tb_defs_unit_test_schedule_special_event_delta(bs_time_t d, tb_defs_unit_test_event_handler_t event_handler);
void tb_defs_unit_test_scheduler(tb_defs_unit_test_tick_handler_t tick_handler);

#endif // #ifndef TB_DEFS_UNIT_TEST_UTILS_H

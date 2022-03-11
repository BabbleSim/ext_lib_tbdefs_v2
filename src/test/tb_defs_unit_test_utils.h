/**
 * Copyright 2017-2022 Oticon A/S
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

//////////////////////////////////////////////////////////////////////////////////////////////////
// BabbleSim replacements

typedef uint64_t bs_time_t;
#define TIME_NEVER UINT64_MAX
#define BASE_TRACE_ERROR 1
#define BASE_TRACE_TIME_PROVIDED 1

#define bs_trace_print(_type, _file, _line, _verbosity, _time_type, _time, _fmt, ...) \
    tb_defs_unit_test_fatal_error(_line, _time, _fmt, ##__VA_ARGS__)

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
void tb_defs_unit_test_fatal_error(unsigned int caller_line, bs_time_t time, const char *format, ...);
void tb_defs_unit_test_expect_fatal_error(char *error_msg);
void _tb_defs_unit_test_check_no_pending_fatal_error(unsigned int caller_line);
#define tb_defs_unit_test_check_no_pending_fatal_error() \
    _tb_defs_unit_test_check_no_pending_fatal_error(__LINE__)

#endif // #ifndef TB_DEFS_UNIT_TEST_UTILS_H

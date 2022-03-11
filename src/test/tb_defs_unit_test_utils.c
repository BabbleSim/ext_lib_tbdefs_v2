/**
 * Copyright 2017-2022 Oticon A/S
 * SPDX-License-Identifier: MIT
 *
 * This file is a derivative of a work licensed to Oticon A/S under the
 * SPDX-License-Identifier: MIT
 * by Paul Emmanuel Wad, with Copyright of 2016-2018
 */

// This file contains utilities needed for testing tb_defs.h.

#include <string.h>
#include <stdarg.h>
#include "tb_defs_unit_test_utils.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// BabbleSim replacements

static bs_time_t now = 0;
static bs_time_t next_tick_time = TIME_NEVER;

char *bs_time_to_str(char *dest, bs_time_t time)
{
    unsigned hour;
    unsigned minute;
    unsigned second;
    unsigned us;

    hour   = ( time/3600/1000000 ) % 100;
    minute = ( time/60/1000000 ) % 60;
    second = ( time/1000000 ) % 60;
    us     = time % 1000000;

    sprintf(dest,"%02u:%02u:%02u.%06u",hour, minute, second,us);
    return dest;
}

bs_time_t tm_get_hw_time(void)
{
    return now;
}

void bst_ticker_set_next_tick_absolute(bs_time_t t)
{
    //printf("%10llu: bst_ticker_set_next_tick_absolute called with param %llu\n", now, t);
    next_tick_time = t >= now ? t : TIME_NEVER;
}

void bst_ticker_set_next_tick_delta(bs_time_t d)
{
    //printf("%10llu: bst_ticker_set_next_tick_delta called with param %llu\n", now, d);
    next_tick_time = now + d;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Unit testing stuff

static bs_time_t tb_defs_unit_test_special_event_time = TIME_NEVER;
static tb_defs_unit_test_event_handler_t tb_defs_unit_test_event_handler = NULL;
static char *tb_defs_unit_test_expected_fatal_error = NULL;

void tb_defs_unit_test_schedule_special_event_delta(bs_time_t d, tb_defs_unit_test_event_handler_t event_handler)
{
    tb_defs_unit_test_special_event_time = now + d;
    tb_defs_unit_test_event_handler = event_handler;
}

void tb_defs_unit_test_scheduler(tb_defs_unit_test_tick_handler_t tick_handler)
{
    // Repeatedly handle next event until no new event has been scheduled
    while (next_tick_time != TIME_NEVER || tb_defs_unit_test_special_event_time != TIME_NEVER)
    {
        // Handle whichever event comes next
        if (tb_defs_unit_test_special_event_time < next_tick_time)
        {
            // Special event happens before next tick, so handle special event now
            now = tb_defs_unit_test_special_event_time;
            tb_defs_unit_test_special_event_time = TIME_NEVER;
            tb_defs_unit_test_event_handler();
        }
        else
        {
            // Handle next tick
            now = next_tick_time;
            next_tick_time = TIME_NEVER;
            tick_handler(now);
        }
    }
}

void tb_defs_unit_test_fatal_error(unsigned int caller_line, bs_time_t time, const char *format, ...)
{
    char strbuf[1024];
    fprintf(stderr, "%s: ", bs_time_to_str(strbuf, time));
    va_list variable_args;
    va_start(variable_args, format);
    vsprintf(strbuf, format, variable_args);
    va_end(variable_args);
    if (tb_defs_unit_test_expected_fatal_error)
    {
        if (strcmp(strbuf, tb_defs_unit_test_expected_fatal_error) == 0)
        {
            fprintf(stderr, "Error (as expected) at line %u: %s", caller_line, strbuf);
            tb_defs_unit_test_expected_fatal_error = NULL;
        }
        else
        {
            fprintf(stderr, "ERROR: Unexpected error at line %u: %sExpected error was: %s", caller_line, strbuf,
                tb_defs_unit_test_expected_fatal_error);
            exit(1);
        }
    }
    else
    {
        fprintf(stderr, "ERROR at line %u: %s", caller_line, strbuf);
        exit(1);
    }
}

void tb_defs_unit_test_expect_fatal_error(char *error_msg)
{
    tb_defs_unit_test_expected_fatal_error = error_msg;
}

void _tb_defs_unit_test_check_no_pending_fatal_error(unsigned int caller_line)
{
    if (tb_defs_unit_test_expected_fatal_error)
    {
        tb_defs_unit_test_expected_fatal_error = NULL;
        tb_defs_unit_test_fatal_error(caller_line, now, "Expected fatal error still pending!\n");
    }
}

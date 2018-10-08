/**
 * Copyright 2017-2018 Oticon A/S
 * SPDX-License-Identifier: MIT
 *
 * This file is a derivative of a work licensed to Oticon A/S under the
 * SPDX-License-Identifier: MIT
 * by Paul Emmanuel Wad, with Copyright of 2016-2018
 */

// This file contains utilities needed for testing tb_defs.h.

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

/**
 * Copyright 2017-2018 Oticon A/S
 * SPDX-License-Identifier: MIT
 *
 * This file is a derivative of a work licensed to Oticon A/S under the
 * SPDX-License-Identifier: MIT
 * by Paul Emmanuel Wad, with Copyright of 2016-2018
 */

// The purpose of this minimal test bench is to reveal possible errors/warnings that might occur
// when as few tb_defs.h macros as possible are present in the test bench file.

#include "tb_defs_unit_test_utils.h"
#include "tb_defs.h"

#undef TB_PRINT_PREFIX
#define TB_PRINT_PREFIX "Test device: "

TB_GLOBALS

void test_init()
{
    bst_ticker_set_next_tick_absolute(0);
}

void test_tick(bs_time_t HW_device_time)
{
    TB_BEGIN

    TB_TEST_STEP("Test started");

    TB_END
}

int main()
{
    test_init();
    tb_defs_unit_test_scheduler(test_tick);
    TB_TEST_STEP("Test ended");
    return 0;
}

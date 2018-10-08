/**
 * Copyright 2017-2018 Oticon A/S
 * SPDX-License-Identifier: MIT
 *
 * This file is a derivative of a work licensed to Oticon A/S under the
 * SPDX-License-Identifier: MIT
 * by Paul Emmanuel Wad, with Copyright of 2016-2018
 */

// This file contains sub-test functions used by tb_defs_unit_test_main.c.

#include "tb_defs_unit_test_utils.h"
#include "tb_defs.h"

#undef TB_PRINT_PREFIX
#define TB_PRINT_PREFIX "Test funcs: "

static int i;

void test_sub_sub_func_in_other_file(TB_CONTEXT_PARAM)
{
    TB_BEGIN

    TB_TEST_STEP("Sub-sub func in other file test");
    TB_CHECKPOINT(10100);
    TB_WAIT(1e6);
    TB_CHECKPOINT(10101);

    TB_END
}

void test_sub_func_in_other_file(TB_CONTEXT_PARAM, int n, bool event)
{
    TB_BEGIN

    TB_TEST_STEP("Sub func in other file test");
    TB_CHECKPOINT(10000);
    TB_FOR(i = 0, i < n, i++)
        TB_WAIT_COND_W_DEADLINE_DELTA(event, 2e6);
        TB_CHECKPOINT(10001);
        TB_IF(i == 1)
            TB_CHECKPOINT(10002);
            TB_CALL(test_sub_sub_func_in_other_file)
            TB_CHECKPOINT(10003);
            TB_RETURN;
            TB_CHECKPOINT(-99);
        TB_ENDIF
        TB_CHECKPOINT(10004);
    TB_ENDFOR
    TB_CHECKPOINT(10005);

    TB_END
}

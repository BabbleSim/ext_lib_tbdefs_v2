/**
 * Copyright 2017-2018 Oticon A/S
 * SPDX-License-Identifier: MIT
 *
 * This file is a derivative of a work licensed to Oticon A/S under the
 * SPDX-License-Identifier: MIT
 * by Paul Emmanuel Wad, with Copyright of 2016-2018
 */

// The purpose of this test bench is to test that all macros in tb_defs.h work as expected.

#include "tb_defs_unit_test_utils.h"
#include "tb_defs.h"
#include "tb_defs_unit_test_sub_funcs.h"

#undef TB_PRINT_PREFIX
#define TB_PRINT_PREFIX "Test device: "

TB_GLOBALS

void test_tick(bs_time_t HW_device_time);

void test_init()
{
    bst_ticker_set_next_tick_absolute(0);
}

static int i, j;
static bool event1 = false;

void event1_handler(void)
{
    bs_trace_raw_time(3, TB_PRINT_PREFIX "Event1 occurred\n");
    event1 = true;
    TB_SIGNAL_EVENT(test_tick);
}

void test_sub_func_in_same_file(TB_CONTEXT_PARAM)
{
    TB_BEGIN
    TB_TEST_STEP("Sub func same file test");
    TB_CHECKPOINT(500);
    TB_WAIT(1e6);
    TB_CHECKPOINT(501);
    TB_RETURN
    TB_CHECKPOINT(502);
    TB_END
}

void test_tick(bs_time_t HW_device_time)
{
    TB_CHECKPOINT_SEQ(
        // WAIT test
        {0,0}, {1e6,1}, {1e6,2}, {5e6,3},

        // WAIT_UNTIL test
        {5e6,10}, {10e6,11},

        // WAIT_COND test
        {11e6,12}, {11e6,13}, {11.5e6,14},

        // FOR/ENDFOR/IF/ELSIF/ELSE/ENDIF test
        {20e6,20}, {21e6,24}, {21e6,21}, {23e6,24}, {23e6,22}, {26e6,24}, {26e6,23}, {30e6,23}, {34e6,24},

        // WHILE/ENDWHILE test
        {40e6,30}, {40e6,31}, {41e6,31}, {42e6,32}, {43e6,30}, {43e6,31}, {44e6,32}, {45e6,30}, {45e6,32}, {46e6,33},

        // REPEAT/UNTIL test
        {50e6,35}, {50e6,36}, {51e6,36}, {52e6,36}, {53e6,37}, {54e6,35}, {54e6,36}, {55e6,36}, {56e6,37}, {57e6,35}, {57e6,36}, {58e6,37}, {59e6,38},

        // BREAK/CONTINUE test
        {60e6,40}, {61e6,41}, {62e6,42}, {62e6,41}, {63e6,43},
        {63e6,44}, {64e6,45},
        {64e6,40}, {65e6,1041}, {66e6,1042}, {66e6,1041}, {67e6,1043},
        {67e6,44}, {68e6,46}, {68e6,48},
        {68e6,50}, {68e6,51}, {68e6,52}, {68e6,53}, {68e6,54}, {68e6,55},

        // WAIT_COND_W_DEADLINE[_DELTA] test
        {121e6,60}, {122e6,61}, {122.5e6,62}, {123e6,63},

        // CALL/RETURN test
        {130e6,500}, {131e6,501}, {131e6,70},
        {131e6,500}, {132e6,501}, {132e6,71},
        {132e6,10000}, {134e6,10001}, {134e6,10004}, {134e6,10005}, {134e6,72},
        {134e6,10000}, {136e6,10001}, {136e6,10004}, {137e6,10001}, {137e6,10002}, {137e6,10100}, {138e6,10101}, {138e6,10003}, {138e6,73},

        // END
        {900e6,-2},
        {900e6,-1},
    );

    TB_BEGIN

    TB_TEST_STEP("WAIT test");
    TB_CHECKPOINT(0);
    TB_WAIT(1e6);
    TB_CHECKPOINT(1);
    TB_WAIT(0);
    TB_CHECKPOINT(2);
    TB_WAIT(4e6);
    TB_CHECKPOINT(3);

    TB_TEST_STEP("WAIT_UNTIL test");
    TB_WAIT_UNTIL(5e6);
    TB_CHECKPOINT(10);
    TB_WAIT_UNTIL(10e6);
    TB_CHECKPOINT(11);

    TB_TEST_STEP("WAIT_COND test");
    // Set an event to occur during the following WAIT before the WAIT_COND that waits for the event;
    // check that the occurrence of the event does not shorten the WAIT
    tb_defs_unit_test_schedule_special_event_delta(0.5e6, event1_handler);
    event1 = false;
    TB_WAIT(1e6);
    TB_CHECKPOINT(12);
    // TB_WAIT_COND when event has already occurred (should not wait)
    TB_WAIT_COND(event1);
    TB_CHECKPOINT(13);
    // Set an event to occur during the following WAIT_COND
    tb_defs_unit_test_schedule_special_event_delta(0.5e6, event1_handler);
    event1 = false;
    // TB_WAIT_COND when event hasn't yet occurred (should end when event occurs)
    TB_WAIT_COND(event1);
    TB_CHECKPOINT(14);

    TB_WAIT_UNTIL(20e6);
    TB_TEST_STEP("FOR/ENDFOR/IF/ELSIF/ELSE/ENDIF test");
    TB_FOR(i = 47, i <= 50, i++)
        TB_IF(i == 47)
            TB_CHECKPOINT(20);
            TB_WAIT(1e6);
        TB_ELSIF(i == 48)
            // IF inside IF
            TB_IF(false)
                TB_CHECKPOINT(-99);
                TB_WAIT(2e6);
            TB_ELSIF(false)
                TB_CHECKPOINT(-99);
                TB_WAIT(2e6);
            TB_ELSE
                TB_CHECKPOINT(21);
                TB_WAIT(2e6);
            TB_ENDIF
        TB_ELSIF(i == 49)
            TB_CHECKPOINT(22);
            TB_WAIT(3e6);
            // FOR with no iterations
            TB_FOR(j = 0, j < 0, j++)
                TB_CHECKPOINT(-99);
                TB_WAIT(3e6);
            TB_ENDFOR
        TB_ELSE
            // FOR inside FOR
            TB_FOR(j = 100, j < 102, j++)
                TB_CHECKPOINT(23);
                TB_WAIT(4e6);
            TB_ENDFOR
        TB_ENDIF    
        TB_CHECKPOINT(24);
    TB_ENDFOR

    TB_WAIT_UNTIL(40e6);
    TB_TEST_STEP("WHILE/ENDWHILE test");
    i = 100;
    TB_WHILE(i < 103)
        TB_CHECKPOINT(30);
        j = i;
        // WHILE inside WHILE
        TB_WHILE(j < 102)
            TB_CHECKPOINT(31);
            TB_WAIT(1e6);
            j++;
        TB_ENDWHILE
        TB_CHECKPOINT(32);
        TB_WAIT(1e6);
        i++;
    TB_ENDWHILE
    TB_CHECKPOINT(33);

    TB_WAIT_UNTIL(50e6);
    TB_TEST_STEP("REPEAT/UNTIL test");
    i = 100;
    TB_REPEAT
        TB_CHECKPOINT(35);
        j = i;
        // REPEAT inside REPEAT
        TB_REPEAT
            TB_CHECKPOINT(36);
            TB_WAIT(1e6);
            j++;
        TB_UNTIL(j == 103)
        TB_CHECKPOINT(37);
        TB_WAIT(1e6);
        i++;
    TB_UNTIL(i == 103)
    TB_CHECKPOINT(38);

    TB_WAIT_UNTIL(60e6);
    TB_TEST_STEP("BREAK/CONTINUE test");
    TB_FOR(i = 0, i < 5, i++)
        TB_CHECKPOINT(40);
        TB_WAIT(1e6);
        j = 10;
        TB_IF(i == 0)
            TB_WHILE(true)
                TB_CHECKPOINT(41);
                TB_WAIT(1e6);
                j++;
                TB_IF(j == 11)
                    TB_CHECKPOINT(42);
                    // CONTINUE WHILE inside FOR
                    TB_CONTINUE;
                    TB_CHECKPOINT(-99);
                TB_ELSIF(j == 12)
                    TB_CHECKPOINT(43);
                    // BREAK WHILE inside FOR
                    TB_BREAK;
                    TB_CHECKPOINT(-99);
                TB_ENDIF
                TB_CHECKPOINT(-99);
                TB_WAIT(1e6);
            TB_ENDWHILE
        TB_ELSE
            TB_REPEAT
                TB_CHECKPOINT(1041);
                TB_WAIT(1e6);
                j++;
                TB_IF(j == 11)
                    TB_CHECKPOINT(1042);
                    // CONTINUE REPEAT inside FOR
                    TB_CONTINUE;
                    TB_CHECKPOINT(-99);
                TB_ELSIF(j == 12)
                    TB_CHECKPOINT(1043);
                    // BREAK REPEAT inside FOR
                    TB_BREAK;
                    TB_CHECKPOINT(-99);
                TB_ENDIF
                TB_CHECKPOINT(-99);
                TB_WAIT(1e6);
            TB_UNTIL(false)
        TB_ENDIF
        TB_CHECKPOINT(44);
        TB_WAIT(1e6);
        TB_IF(i == 0)
            TB_CHECKPOINT(45);
            // CONTINUE FOR
            TB_CONTINUE;
            TB_CHECKPOINT(-99);
        TB_ELSIF(i == 1)
            TB_CHECKPOINT(46);
            // BREAK FOR
            TB_BREAK;
            TB_CHECKPOINT(-99);
        TB_ENDIF
        TB_CHECKPOINT(-99);
        TB_WAIT(1e6);
    TB_ENDFOR
    TB_CHECKPOINT(48);
    // Make sure that CONTINUE doesn't loop without checking loop condition
    TB_FOR(i = 0, i < 1, i++)
        TB_CHECKPOINT(50);
        j = 0;
        TB_WHILE(j < 1)
            TB_CHECKPOINT(51);
            TB_REPEAT
                TB_CHECKPOINT(52);
                TB_CONTINUE;
                TB_CHECKPOINT(-99);
            TB_UNTIL(true)
            TB_CHECKPOINT(53);
            j++;
            TB_CONTINUE;
            TB_CHECKPOINT(-99);
        TB_ENDWHILE
        TB_CHECKPOINT(54);
        TB_CONTINUE;
        TB_CHECKPOINT(-99);
    TB_ENDFOR
    TB_CHECKPOINT(55);

    TB_WAIT_UNTIL(120e6);
    TB_TEST_STEP("WAIT_COND_W_DEADLINE[_DELTA] test");
    event1 = false;
    TB_WAIT_COND_W_DEADLINE(event1, 121e6);
    TB_CHECKPOINT(60);
    event1 = false;
    TB_WAIT_COND_W_DEADLINE_DELTA(event1, 1e6);
    TB_CHECKPOINT(61);
    // Set an event to occur during the following WAIT_COND_W_DEADLINE
    tb_defs_unit_test_schedule_special_event_delta(0.5e6, event1_handler);
    event1 = false;
    TB_WAIT_COND_W_DEADLINE(event1, 125e6);
    TB_CHECKPOINT(62);
    // Set an event to occur during the following WAIT_COND_W_DEADLINE_DELTA
    tb_defs_unit_test_schedule_special_event_delta(0.5e6, event1_handler);
    event1 = false;
    TB_WAIT_COND_W_DEADLINE_DELTA(event1, 5e6);
    TB_CHECKPOINT(63);

    TB_WAIT_UNTIL(130e6);
    TB_TEST_STEP("CALL test");
    TB_CALL(test_sub_func_in_same_file);
    TB_CHECKPOINT(70);
    TB_CALL(test_sub_func_in_same_file);
    TB_CHECKPOINT(71);
    // Set an event to occur during the WAIT_COND_W_DEADLINE_DELTA in the last FOR loop iteration in the last call
    // to test_sub_func_in_other_file below (should shorten that wait while letting the preceding waits run to their
    // deadlines)
    tb_defs_unit_test_schedule_special_event_delta(5e6, event1_handler);
    event1 = false;
    TB_CALL(test_sub_func_in_other_file, 1, event1);
    TB_CHECKPOINT(72);
    TB_CALL(test_sub_func_in_other_file, 3, event1);
    TB_CHECKPOINT(73);

    TB_WAIT_UNTIL(900e6);
    TB_TEST_STEP("Final");
    TB_CHECKPOINT(-2);
    TB_END
}

int main()
{
    test_init();
    tb_defs_unit_test_scheduler(test_tick);
    TB_CHECKPOINT(-1);
    TB_TEST_STEP("Test ended - all OK!");
    return 0;
}

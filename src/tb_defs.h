/**
 * Copyright 2017-2018 Oticon A/S
 * SPDX-License-Identifier: MIT
 *
 * This file is a derivative of a work licensed to Oticon A/S under the
 * SPDX-License-Identifier: MIT
 * by Paul Emmanuel Wad, with Copyright of 2016-2018
 */

#ifndef TB_DEFS_H
#define TB_DEFS_H

// This file provides several definitions that ease the writing - and reading - of an BabbleSim test bench.
//
// These definitions make it possible to write the test bench inside a time tick handler as if the whole thing were a
// simple sequential program without the complexity of repeatedly exiting and reentering the tick handler whenever
// delays are needed. Here's an example of a test program that does something, waits 1 ms, and does something more:
//
// TB_GLOBALS
//
// void test_tick()
// {
//     TB_BEGIN
//     do_something;
//     TB_WAIT(1e3)
//     do_something_more;
//     TB_END
// }
//
// In this example, the TB_WAIT macro hides the complexity of scheduling a new tick event after 1 ms, exiting the
// tick handler, and continuing execution of the next statement when the tick handler is called again.
//
// Macros are also provided to facilitate conditional execution (TB_IF/ELSIF/ELSE/ENDIF), looping (TB_FOR/ENDFOR/WHILE/
// ENDWHILE/REPEAT/UNTIL/BREAK/CONTINUE), and calling (TB_CALL/RETURN) of test bench code that includes TB_WAITs.
//
// Furthermore, TB_WAIT_UNTIL waits until an absolute time, and TB_WAIT_COND waits for a condition to become true as a
// result of one or more (non-time-tick) events (the test bench must ensure that event handlers for these events set
// the appropriate variables which are used in the condition, and call the time tick handler via TB_SIGNAL_EVENT).
// Two other variants, TB_WAIT_COND_W_DEADLINE and TB_WAIT_COND_W_DEADLINE_DELTA, wait for either a condition to become
// true OR a certain absolute/relative time to occur/elapse, whichever happens first.
//
// Usage examples can be found in the tb_defs_unit_test_main.c file which tests all these definitions.
//
// IMPORTANT: Always use statically allocated (e.g. file level) variables to store information that needs to survive
//            the execution of TB_WAIT and the other macros. These macros close and open code blocks or exit and enter
//            the tick handler function, so local stack variables will obviously not be preserved!

#include <stdbool.h>
// Include the following header files in the c file before including this header file.
//#include "bs_types.h"
//#include "bs_tracing.h"
//#include "bs_utils.h"
//#include "bs_string.h"
//#include "time_machine.h"

//////////////////////////////////////////////////////////////////////////////////////////////////
// Private definitions

typedef struct
{
    bs_time_t time;
    int val;
} tb_checkpoint_t;

typedef struct
{
    bool is_waiting_for_cond;
    bool non_time_event_occurred;
    bs_time_t waiting_deadline;
    bool is_func_done;
    const tb_checkpoint_t *checkpoints;
    int nbr_checkpoints;
    int checkpoint_idx;
} tb_context_t;

typedef enum
{
    TB_BLK_TYPE_IF,

    TB_BLK_TYPE_LOOP_TYPE_MARKER, // Marks the start of the loop types in this enum
    TB_BLK_TYPE_WHILE,
    TB_BLK_TYPE_FOR,
    TB_BLK_TYPE_REPEAT,
    TB_BLK_TYPE_LOOP_TYPE_ENDMARKER, // Marks the end of the loop types in this enum
} tb_blk_type_t;

#define TB_BLK_TYPE_IS_LOOP(_blk_type) \
    ((_blk_type) > TB_BLK_TYPE_LOOP_TYPE_MARKER && (_blk_type) < TB_BLK_TYPE_LOOP_TYPE_ENDMARKER)

typedef struct
{
    int first_line;
    tb_blk_type_t blk_type;
} tb_blk_info_t;

#define TB_MAX_BLK_LEVELS 64

// Special values for tb_next_line when next line is unknown
#define TB_END_LINE             -1 // Go to line following ENDIF/ENDFOR/ENDWHILE/UNTIL
#define TB_ELSE_OR_ENDIF_LINE   -2 // Go to line following ELSE OR ENDIF - whichever comes first
#define TB_LOOP_COND_LINE       -3 // Go to evaluation of loop condition (skip loop iteration expression)
#define TB_LOOP_ITER_LINE       -4 // Go to end of loop before evaluation of iteration expression and loop condition,
                                   // i.e. point immediately before ENDFOR/ENDWHILE/UNTIL

//////////////////////////////////////////////////////////////////////////////////////////////////
// Public definitions for use in test benches

// TB_GLOBALS defines needed globals. Must be instantiated once per test bench at file level. Is not necessary in a
// file containing only sub-test functions and no time tick handler.
#define TB_GLOBALS \
    static tb_context_t tb_context = { \
        .is_waiting_for_cond = false, \
        .non_time_event_occurred = false, \
        .waiting_deadline = TIME_NEVER, \
        .is_func_done = false, \
        .checkpoints = NULL, \
        .nbr_checkpoints = 0, \
        .checkpoint_idx = 0 \
    }; \
    static tb_context_t *tb_context_ptr = &tb_context;

// TB_PRINT_PREFIX defines a string to be prepended to all printed messages. Optionally #undef this in the test bench
// and #define it to some meaningful string. Or #define it before including this header file.
#ifndef TB_PRINT_PREFIX
#define TB_PRINT_PREFIX ""
#endif

// TB_ASSERT checks that the specified condition is true, and if not, prints the specified printf-style formatted error
// message, and terminates the test with status failed.
#define TB_ASSERT(_cond, _fmt_str, ...) \
        if (!(_cond)) bs_trace_print(BS_TRACE_ERROR, __FILE__, __LINE__, 0, BS_TRACE_TIME_PROVIDED, \
            tm_get_hw_time(), TB_PRINT_PREFIX "TB_ASSERT failed: " _fmt_str "\n", ##__VA_ARGS__);

// TB_CHECKPOINT_SEQ defines a list of time/value pairs to be used later by TB_CHECKPOINT statements.
// Must be put inside the time tick handler before TB_BEGIN, if checkpoints are used. Should normally NOT be used in
// sub-test functions, as those inherit the calling function's TB_CHECKPOINT_SEQ.
// Example: TB_CHECKPOINT_SEQ({0,1}, {1e6,2}, {2e6, 3})
#define TB_CHECKPOINT_SEQ(...) \
    static const tb_checkpoint_t tb_checkpoints[] = {__VA_ARGS__}; \
    tb_context_ptr->checkpoints = tb_checkpoints; \
    tb_context_ptr->nbr_checkpoints = sizeof(tb_checkpoints)/sizeof(tb_checkpoints[0]);

// TB_CHECKPOINT checks that the current time and specified value match the current checkpoint item in the
// TB_CHECKPOINT_SEQ.
// Example: Given the TB_CHECKPOINT_SEQ example above, TB_CHECKPOINT should be called 3 times at times 0, 1e6, and 2e6
// with parameters 1, 2, and 3 respectively. Otherwise the test will fail.
#define TB_CHECKPOINT(_val) \
        { \
            char tb_strbuf[20]; \
            TB_ASSERT(tb_context_ptr->checkpoints != NULL, "TB_CHECKPOINT without TB_CHECKPOINT_SEQ!"); \
            TB_ASSERT(tb_context_ptr->checkpoint_idx < tb_context_ptr->nbr_checkpoints, \
                "More TB_CHECKPOINTs than items in TB_CHECKPOINT_SEQ!"); \
            const tb_checkpoint_t *chkpnt_ptr = &tb_context_ptr->checkpoints[tb_context_ptr->checkpoint_idx]; \
            TB_ASSERT(chkpnt_ptr->time == tm_get_hw_time() && chkpnt_ptr->val == (_val), \
                "TB_CHECKPOINT != TB_CHECKPOINT_SEQ[%d]: actual value=%d, expected value=%d, expected time=%s", \
                tb_context_ptr->checkpoint_idx, (_val), chkpnt_ptr->val, \
                bs_time_to_str(tb_strbuf, chkpnt_ptr->time)); \
            tb_context_ptr->checkpoint_idx++; \
        }

// TB_SIGNAL_EVENT signals to the time tick handler that a non-time-tick event has occurred. Event handlers should use
// this macro.
#define TB_SIGNAL_EVENT(_tick_handler) \
    { \
        tb_context_ptr->non_time_event_occurred = true; \
        (_tick_handler)(tm_get_hw_time()); \
    } \

// TB_BEGIN starts the (sub-)test sequence. Should be the first statement in the tick handler or sub-test function
// (except for TB_CHECKPOINT_SEQ if used).
#define TB_BEGIN \
    tb_context_ptr->is_func_done = false; \
    if (tb_context_ptr->non_time_event_occurred) \
    { \
        tb_context_ptr->non_time_event_occurred = false; \
        if (!tb_context_ptr->is_waiting_for_cond) \
            return; \
    } \
    tb_blk_info_t tb_blk_info[TB_MAX_BLK_LEVELS] __attribute__ ((__unused__)); \
    int tb_cur_blk_level = 0; \
    int tb_next_blk_level __attribute__ ((__unused__)) = 0; \
    static int tb_next_line = 0; \
    if (tb_next_line == 0) \
    {

// TB_TEST_STEP prints the test step title (as well as the time and line number). Can be used any number of times in
// a test.
#define TB_TEST_STEP(_fmt, ...) \
        bs_trace_raw_time(3, TB_PRINT_PREFIX "### Test step: " _fmt " (line %d)\n", ##__VA_ARGS__, __LINE__);

// TB_WAIT_UNTIL waits until the specified absolute time point.
#define TB_WAIT_UNTIL(_time) \
        { \
            char tb_strbuf[20]; \
            TB_ASSERT(_time >= tm_get_hw_time(), "TB_WAIT_UNTIL time %s is in the past!", \
                bs_time_to_str(tb_strbuf, (_time))); \
        } \
        bst_ticker_set_next_tick_absolute(_time); \
        tb_next_line = __LINE__; \
        return; \
    } \
    if (tb_next_line == __LINE__) \
    {

// TB_WAIT waits for the specified delay to elapse.
#define TB_WAIT(_delay) \
        bst_ticker_set_next_tick_delta(_delay); \
        tb_next_line = __LINE__; \
        return; \
    } \
    if (tb_next_line == __LINE__) \
    {

// TB_WAIT_COND waits for the specified condition to occur.
#define TB_WAIT_COND(_cond) \
        tb_context_ptr->is_waiting_for_cond = true; \
        tb_next_line = __LINE__; \
    } \
    if (tb_next_line == __LINE__) \
    { \
        if (!(_cond)) \
            return; \
        tb_context_ptr->is_waiting_for_cond = false;

// TB_WAIT_COND_W_DEADLINE waits for the specified condition to occur, or until the specified absolute time point,
// whichever happens first.
#define TB_WAIT_COND_W_DEADLINE(_cond, _time) \
        bst_ticker_set_next_tick_absolute(_time); \
        tb_context_ptr->waiting_deadline = _time; \
        tb_context_ptr->is_waiting_for_cond = true; \
        tb_next_line = __LINE__; \
    } \
    if (tb_next_line == __LINE__) \
    { \
        if (!(_cond) && (tm_get_hw_time() < tb_context_ptr->waiting_deadline)) \
            return; \
        tb_context_ptr->waiting_deadline = TIME_NEVER; \
        bst_ticker_set_next_tick_absolute(TIME_NEVER); \
        tb_context_ptr->is_waiting_for_cond = false;

// TB_WAIT_COND_W_DEADLINE_DELTA waits for the specified condition to occur, or for the specified delay to elapse,
// whichever happens first.
#define TB_WAIT_COND_W_DEADLINE_DELTA(_cond, _delay) \
        bst_ticker_set_next_tick_delta(_delay); \
        tb_context_ptr->waiting_deadline = _delay + tm_get_hw_time(); \
        tb_context_ptr->is_waiting_for_cond = true; \
        tb_next_line = __LINE__; \
    } \
    if (tb_next_line == __LINE__) \
    { \
        if (!(_cond) && (tm_get_hw_time() < tb_context_ptr->waiting_deadline)) \
            return; \
        tb_context_ptr->waiting_deadline = TIME_NEVER; \
        bst_ticker_set_next_tick_absolute(TIME_NEVER); \
        tb_context_ptr->is_waiting_for_cond = false;

// TB_IF and TB_ENDIF delimit a block of statements which are only executed if the specified condition is true.
// TB_IF/TB_ENDIF blocks can be nested.
#define TB_IF(_cond) \
        tb_next_line = __LINE__; \
    } \
    TB_ASSERT(++tb_cur_blk_level < TB_MAX_BLK_LEVELS, "Too many nested blocks!"); \
    tb_blk_info[tb_cur_blk_level].blk_type = TB_BLK_TYPE_IF; \
    if (tb_next_line == __LINE__ && !(_cond)) \
    { \
        tb_next_line = TB_ELSE_OR_ENDIF_LINE; \
        tb_next_blk_level = tb_cur_blk_level - 1; \
    } \
    if (tb_next_line == __LINE__) \
    {

// TB_ELSE is only allowed within a TB_IF/TB_ENDIF block, and causes the following statements to be executed only if
// the associated TB_IF condition is false.
#define TB_ELSE \
        tb_next_line = __LINE__; \
    } \
    TB_ASSERT(tb_cur_blk_level > 0 && tb_blk_info[tb_cur_blk_level].blk_type == TB_BLK_TYPE_IF, \
        "TB_ELSE with no matching TB_IF!"); \
    if (tb_next_line == __LINE__) \
    { \
        tb_next_line = TB_END_LINE; \
        tb_next_blk_level = tb_cur_blk_level - 1; \
    } \
    else if (tb_next_line == TB_ELSE_OR_ENDIF_LINE && tb_next_blk_level == tb_cur_blk_level - 1) \
    {

// TB_ELSIF is equivalent to a TB_ELSE followed by a TB_IF, but this TB_IF shares the same TB_ENDIF as the original
// TB_IF associated with the TB_ELSE. Example: TB_IF() ... TB_ELSIF() ... TB_ELSIF() ... TB_ELSE ... TB_ENDIF.
#define TB_ELSIF(_cond) \
        tb_next_line = __LINE__; \
    } \
    TB_ASSERT(tb_cur_blk_level > 0 && tb_blk_info[tb_cur_blk_level].blk_type == TB_BLK_TYPE_IF, \
        "TB_ELSIF with no matching TB_IF!"); \
    if (tb_next_line == __LINE__) \
    { \
        tb_next_line = TB_END_LINE; \
        tb_next_blk_level = tb_cur_blk_level - 1; \
    } \
    else if (tb_next_line == TB_ELSE_OR_ENDIF_LINE && tb_next_blk_level == tb_cur_blk_level - 1 && (_cond)) \
    {

// TB_IF and TB_ENDIF delimit a block of statements which are only executed if the TB_IF condition is true.
#define TB_ENDIF \
        tb_next_line = __LINE__; \
    } \
    TB_ASSERT(tb_cur_blk_level > 0 && tb_blk_info[tb_cur_blk_level--].blk_type == TB_BLK_TYPE_IF, \
        "TB_ENDIF with no matching TB_IF!"); \
    if (tb_next_line == __LINE__ || \
        ((tb_next_line == TB_END_LINE || tb_next_line == TB_ELSE_OR_ENDIF_LINE) && tb_next_blk_level == tb_cur_blk_level)) \
    {

// TB_WHILE and TB_ENDWHILE delimit a block of statements which are repeatedly executed as long as the specified
// condition is true. If the condition is initially false, the block of statements is not executed at all.
// TB_WHILE/TB_ENDWHILE blocks can be nested.
#define TB_WHILE(_cond) \
        tb_next_line = __LINE__; \
    } \
    TB_ASSERT(++tb_cur_blk_level < TB_MAX_BLK_LEVELS, "Too many nested blocks!"); \
    tb_blk_info[tb_cur_blk_level].blk_type = TB_BLK_TYPE_WHILE; \
    tb_blk_info[tb_cur_blk_level].first_line = __LINE__; \
    if (tb_next_line == __LINE__ && !(_cond)) \
    { \
        tb_next_line = TB_END_LINE; \
        tb_next_blk_level = tb_cur_blk_level - 1; \
    } \
    if (tb_next_line == __LINE__) \
    {

// TB_WHILE and TB_ENDWHILE delimit a block of statements which are repeatedly executed as long as the TB_WHILE
// condition is true.
#define TB_ENDWHILE \
        tb_next_line = __LINE__; \
    } \
    TB_ASSERT(tb_cur_blk_level > 0 && tb_blk_info[tb_cur_blk_level--].blk_type == TB_BLK_TYPE_WHILE, \
        "TB_ENDWHILE with no matching TB_WHILE!"); \
    if (tb_next_line == __LINE__ || (tb_next_line == TB_LOOP_ITER_LINE && tb_next_blk_level == tb_cur_blk_level)) \
    { \
        bst_ticker_set_next_tick_delta(0); \
        tb_next_line = tb_blk_info[tb_cur_blk_level + 1].first_line; \
        return; \
    } \
    if (tb_next_line == TB_END_LINE && tb_next_blk_level == tb_cur_blk_level) \
    {

// TB_FOR and TB_ENDFOR delimit a block of statements which are repeatedly executed as long as the specified condition
// is true. If the condition is initially false, the block of statements is not executed at all. Additionally, TB_FOR
// executes the specified initialization expression before the first evaluation of the condition, and executes the
// specified iteration expression after each loop iteration (before the condition is evaluated again).
// Example of loop with 10 iterations: TB_FOR(i = 0, i < 10, i++) ... TB_ENDFOR.
// (Remember to use variables that will survive the exiting and reentering of the time tick handler).
// TB_FOR/TB_ENDFOR blocks can be nested.
#define TB_FOR(_init_expr, _cond, _iter_expr) \
        (_init_expr); \
        tb_next_line = TB_LOOP_COND_LINE; \
    } \
    TB_ASSERT(++tb_cur_blk_level < TB_MAX_BLK_LEVELS, "Too many nested blocks!"); \
    tb_blk_info[tb_cur_blk_level].blk_type = TB_BLK_TYPE_FOR; \
    tb_blk_info[tb_cur_blk_level].first_line = __LINE__; \
    if (tb_next_line == __LINE__) \
    { \
        (_iter_expr); \
    } \
    if (tb_next_line == TB_LOOP_COND_LINE) \
    { \
        tb_next_line = __LINE__; \
    } \
    if (tb_next_line == __LINE__ && !(_cond)) \
    { \
        tb_next_line = TB_END_LINE; \
        tb_next_blk_level = tb_cur_blk_level - 1; \
    } \
    if (tb_next_line == __LINE__) \
    {

// TB_FOR and TB_ENDFOR delimit a block of statements which are repeatedly executed as long as the TB_FOR condition is
// true.
#define TB_ENDFOR \
        tb_next_line = __LINE__; \
    } \
    TB_ASSERT(tb_cur_blk_level > 0 && tb_blk_info[tb_cur_blk_level--].blk_type == TB_BLK_TYPE_FOR, \
        "TB_ENDFOR with no matching TB_FOR!"); \
    if (tb_next_line == __LINE__ || (tb_next_line == TB_LOOP_ITER_LINE && tb_next_blk_level == tb_cur_blk_level)) \
    { \
        bst_ticker_set_next_tick_delta(0); \
        tb_next_line = tb_blk_info[tb_cur_blk_level + 1].first_line; \
        return; \
    } \
    if (tb_next_line == TB_END_LINE && tb_next_blk_level == tb_cur_blk_level) \
    {

// TB_REPEAT and TB_UNTIL delimit a block of statements which are repeatedly executed until the TB_UNTIL condition is
// true. The block of statements will be executed at least once, as the condition is checked at the end of the block.
// TB_REPEAT/TB_UNTIL blocks can be nested.
#define TB_REPEAT \
        tb_next_line = __LINE__; \
    } \
    TB_ASSERT(++tb_cur_blk_level < TB_MAX_BLK_LEVELS, "Too many nested blocks!"); \
    tb_blk_info[tb_cur_blk_level].blk_type = TB_BLK_TYPE_REPEAT; \
    tb_blk_info[tb_cur_blk_level].first_line = __LINE__; \
    if (tb_next_line == __LINE__) \
    {

// TB_REPEAT and TB_UNTIL delimit a block of statements which are repeatedly executed until the specified condition is
// true.
#define TB_UNTIL(_cond) \
        tb_next_line = __LINE__; \
    } \
    TB_ASSERT(tb_cur_blk_level > 0 && tb_blk_info[tb_cur_blk_level--].blk_type == TB_BLK_TYPE_REPEAT, \
        "TB_UNTIL with no matching TB_REPEAT!"); \
    if (tb_next_line == __LINE__ || (tb_next_line == TB_LOOP_ITER_LINE && tb_next_blk_level == tb_cur_blk_level)) \
    { \
        if (!(_cond)) \
        { \
            bst_ticker_set_next_tick_delta(0); \
            tb_next_line = tb_blk_info[tb_cur_blk_level + 1].first_line; \
            return; \
        } \
        tb_next_line = __LINE__; \
    } \
    if (tb_next_line == __LINE__ || (tb_next_line == TB_END_LINE && tb_next_blk_level == tb_cur_blk_level)) \
    {

// TB_BREAK breaks out of a surrounding TB_WHILE, TB_FOR, or TB_REPEAT loop, and continues execution of the statements
// following the end of the loop.
#define TB_BREAK \
        tb_next_line = __LINE__; \
    } \
    { \
        int i; \
        for (i = tb_cur_blk_level; i > 0 && !TB_BLK_TYPE_IS_LOOP(tb_blk_info[i].blk_type); i--); \
        TB_ASSERT(i > 0, "TB_BREAK not inside loop!"); \
        if (tb_next_line == __LINE__) \
        { \
            tb_next_line = TB_END_LINE; \
            tb_next_blk_level = i - 1; \
        } \
    } \
    if (false) \
    {

// TB_CONTINUE jumps to the end of a surrounding TB_WHILE, TB_FOR, or TB_REPEAT loop, and proceeds with the next
// loop iteration if any. Statements between the TB_CONTINUE and the end of the loop are skipped.
#define TB_CONTINUE \
        tb_next_line = __LINE__; \
    } \
    { \
        int i; \
        for (i = tb_cur_blk_level; i > 0 && !TB_BLK_TYPE_IS_LOOP(tb_blk_info[i].blk_type); i--); \
        TB_ASSERT(i > 0, "TB_CONTINUE not inside loop!"); \
        if (tb_next_line == __LINE__) \
        { \
            tb_next_line = TB_LOOP_ITER_LINE; \
            tb_next_blk_level = i - 1; \
        } \
    } \
    if (false) \
    {

// TB_CALL calls the specified function (defined in the same or a different file) containing a sub-test sequence
// delimited by its own TB_BEGIN and TB_END. Any arguments following the function name in the TB_CALL argument list
// are passed to the called function. The function definition must have TB_CONTEXT_PARAM as its first parameter,
// optionally followed by any user defined parameters. When the sub-test sequence in the called function completes,
// execution continues with the statement following the TB_CALL. TB_CALLs can be nested.
#define TB_CALL(_func, ...) \
        tb_next_line = __LINE__; \
    } \
    if (tb_next_line == __LINE__) \
    { \
        (_func)(tb_context_ptr, ##__VA_ARGS__); \
        if (!tb_context_ptr->is_func_done) \
            return; \
        tb_context_ptr->is_func_done = false;

// TB_RETURN ends the current sub-test sequence and returns control to the calling function (the one that issued the
// TB_CALL). If TB_RETURN is executed in the top level test sequence, the sequence ends (no new time tick is
// scheduled). A (sub-)test sequence that does not encounter a TB_RETURN, ends/returns at TB_END.
#define TB_RETURN \
        tb_context_ptr->is_func_done = true; \
        tb_next_line = 0; \
        return;

// TB_CONTEXT_PARAM must be specified as the first parameter when defining a function that is to be called by a
// TB_CALL.
#define TB_CONTEXT_PARAM \
    tb_context_t *tb_context_ptr

// TB_END ends the (sub-)test sequence. Should be the last statement in the tick handler or sub-test function.
#define TB_END \
        tb_context_ptr->is_func_done = true; \
        tb_next_line = 0; \
    } \
    TB_ASSERT(tb_cur_blk_level == 0, "TB_END inside block!");

#endif // #ifndef TB_DEFS_H

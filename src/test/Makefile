# Copyright 2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

CC:=gcc
WARNINGS:=-Wall -Wundef
INCLUDE_DIRS:=-I..
CFLAGS:=${WARNINGS} -std=c99 ${INCLUDE_DIRS}
vpath %.h ..

HEADERS:=tb_defs.h tb_defs_unit_test_utils.h tb_defs_unit_test_sub_funcs.h

.PHONY: all compile run clean

all: compile run clean

%.o: %.c $(HEADERS)
	${CC} ${CFLAGS} -c $< -o $@

EXES:=

tb_defs_unit_test_main: tb_defs_unit_test_main.o tb_defs_unit_test_sub_funcs.o tb_defs_unit_test_utils.o
	${CC} ${CFLAGS} $^ -o $@
EXES+=tb_defs_unit_test_main

tb_defs_unit_test_minimal: tb_defs_unit_test_minimal.o tb_defs_unit_test_utils.o 
	${CC} ${CFLAGS} $^ -o $@
EXES+=tb_defs_unit_test_minimal

compile: $(EXES)

define TEST_RECIPE =
	@echo
	@echo "### Running test $t"
	@./$t
	@
endef

run: $(EXES)
	$(foreach t,$^,$(TEST_RECIPE))

clean:
	@-rm -f ${EXES} *.o

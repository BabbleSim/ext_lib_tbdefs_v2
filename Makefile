# Copyright 2017-2018 Oticon A/S
# SPDX-License-Identifier: Apache-2.0

CC:=gcc

.PHONY: all compile test clean install

all: compile

compile:
#	$(info Hint: Run "make test" to build and run tb_defs unit tests)

test:
	@$(MAKE) -C src/test run clean

clean:
	@$(MAKE) -C test clean

install:

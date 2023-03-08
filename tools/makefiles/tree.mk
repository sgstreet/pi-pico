#
# Copyright (C) 2017 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# tree.mk
#
# Created on: Mar 16, 2017
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

where-am-i := ${CURDIR}/$(lastword $(subst $(lastword ${MAKEFILE_LIST}),,${MAKEFILE_LIST}))

SUBDIRS ?= $(subst ${CURDIR}/,,$(shell find ${CURDIR} -mindepth 2 -maxdepth 2 -name "*.mk" -and -not -name "subdir.mk" -printf "%h "))

${SUBDIRS}:
	@echo "ENTERING $@"
	+${MAKE} --no-print-directory -C $@ -f $@.mk ${MAKECMDGOALS}
.PHONY: ${SUBDIRS}

target:

all: target
	
clean: target

install: target

#$(info SUBDIR=${SUBDIRS})


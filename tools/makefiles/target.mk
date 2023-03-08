#
# Copyright (C) 2017 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# target.mk
#
# Created on: Mar 16, 2017
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

.SUFFIXES:

BUILD_PATH ?= $(subst ${PROJECT_ROOT},${BUILD_ROOT},${CURDIR})
SUBDIRS ?= $(subst ${CURDIR}/,,$(shell find ${CURDIR} -mindepth 2 -maxdepth 2 -name "*.mk" -and -not -name "subdir.mk" -printf "%h "))

#$(info PROJECT_ROOT=${PROJECT_ROOT})
#$(info BUILD_ROOT=${BUILD_ROOT})
#$(info CURDIR=${CURDIR})
#$(info BUILD_PATH=${BUILD_PATH})
#$(info SUBDIRS=${SUBDIRS})

${SUBDIRS}:
	@echo "ENTERING $@"
	+${MAKE} --no-print-directory -C $@ -f $@.mk ${MAKECMDGOALS}
.PHONY: ${SUBDIRS}

${BUILD_PATH}: ${SUBDIRS}
	+[ -d $@ ] || mkdir -p $@
	+${MAKE} --no-print-directory -C $@ -f ${CURDIR}/$(lastword $(subst /, ,${CURDIR})).mk SOURCE_DIR=${CURDIR} ${MAKECMDGOALS}
.PHONY: ${BUILD_PATH}

target:

all: target
	
clean: target

install: target

Makefile : ;
%.mk :: ;

% :: ${BUILD_PATH} ; @:

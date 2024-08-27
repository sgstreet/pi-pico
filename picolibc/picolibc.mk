#
# Copyright (C) 2017 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# template-project.mk
#
# Created on: Mar 16, 2017
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

ifeq ($(findstring ${BUILD_ROOT},${CURDIR}),)
include ${PROJECT_ROOT}/tools/makefiles/target.mk
else

GIT_REPOSITORY := https://github.com/picolibc/picolibc.git
GIT_TAG := 1.8.6

${CURDIR}/README.md:
	@echo "CLONING ${GIT_REPOSITORY}@${GIT_TAG}"
	git clone ${GIT_REPOSITORY} --quiet ${CURDIR}
	git switch --quiet --detach ${GIT_TAG}
	
${CURDIR}/build/.gitignore: ${CURDIR}/README.md
	@echo "CONFIGURING ${CURDIR}"
	+[ -d $(dir ${@}) ] || mkdir -p $(dir ${@})
	cd $(dir ${@}) && meson setup --cross-file ${SOURCE_DIR}/cross-cortex-m0-none-eabi.txt -Dc_args=-funwind-tables -Dc_args=-mpoke-function-name -Dmultilib=false -Dprefix=${CURDIR} -Dincludedir=arm-none-eabi/include -Dlibdir=arm-none-eabi/lib -Dspecsdir=arm-none-eabi/lib ${CURDIR}

${CURDIR}/arm-none-eabi/lib/picolibc.specs: ${CURDIR}/build/.gitignore
	@echo "BUILDING ${CURDIR}"
	cd ${CURDIR}/build && ninja --quiet install
 	
all: ${CURDIR}/arm-none-eabi/lib/picolibc.specs

clean:
	@echo "CLEANING ${CURDIR}"
	cd ${CURDIR}/build && ninja clean
	rm -rf ${CURDIR}/build

picolibc: all

endif

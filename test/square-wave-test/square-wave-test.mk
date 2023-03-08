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

EXTRA_DEPS := ${PIO} ${SOURCE_DIR}/square-wave-test.mk ${PROJECT_ROOT}/ldscripts/sections.ld ${PROJECT_ROOT}/ldscripts/memory.ld ${PROJECT_ROOT}/ldscripts/regions-sram.ld ${PROJECT_ROOT}/ldscripts/regions-flash.ld
EXTRA_CLEAN := ${PIO} ${INSTALL_ROOT}/square-wave-test.bin ${INSTALL_ROOT}/square-wave-test.elf ${INSTALL_ROOT}/square-wave-test.uf2

EXTRA_OBJ_DEPS := ${CURDIR}/squarewave.pio.h

TARGET_OBJ_LIBS := bootstrap
TARGET_OBJ_LIBS += board board/${BOARD_TYPE}
TARGET_OBJ_LIBS += diag diag/board
TARGET_OBJ_LIBS += sys
TARGET_OBJ_LIBS += cmsis/device/rp2040
TARGET_OBJ_LIBS += init

include ${PROJECT_ROOT}/tools/makefiles/project.mk

CPPFLAGS += -DPICO_NO_HARDWARE -I${CURDIR}
LDSCRIPTS := -L${PROJECT_ROOT}/ldscripts -T memory.ld -T regions-flash.ld -T sections.ld

all: ${INSTALL_ROOT}/square-wave-test.bin ${INSTALL_ROOT}/square-wave-test.elf ${INSTALL_ROOT}/square-wave-test.uf2

${INSTALL_ROOT}/square-wave-test.uf2: ${CURDIR}/square-wave-test.uf2
	@echo "INSTALLING ${@}"
	$(INSTALL) -m 660 -D ${<} ${@}

${INSTALL_ROOT}/square-wave-test.elf: ${CURDIR}/square-wave-test.elf
	@echo "INSTALLING ${@}"
	$(INSTALL) -m 660 -D ${<} ${@}

${INSTALL_ROOT}/square-wave-test.bin: ${CURDIR}/square-wave-test.bin
	@echo "INSTALLING ${@}"
	$(INSTALL) -m 660 -D ${<} ${@}

endif

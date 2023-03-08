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

EXTRA_DEPS := ${SOURCE_DIR}/rtos-cmsis-validation.mk ${PROJECT_ROOT}/ldscripts/sections.ld ${PROJECT_ROOT}/ldscripts/memory.ld ${PROJECT_ROOT}/ldscripts/regions-sram.ld ${PROJECT_ROOT}/ldscripts/regions-flash.ld
EXTRA_CLEAN := ${INSTALL_ROOT}/rtos-cmsis-validation.bin ${INSTALL_ROOT}/rtos-cmsis-validation.elf ${INSTALL_ROOT}/rtos-cmsis-validation.uf2

TARGET_OBJ_LIBS := bootstrap
TARGET_OBJ_LIBS += board board/${BOARD_TYPE}
TARGET_OBJ_LIBS += diag diag/board
TARGET_OBJ_LIBS += sys
TARGET_OBJ_LIBS += cmsis/device/rp2040
TARGET_OBJ_LIBS += init
TARGET_OBJ_LIBS += hal/hal-${CHIP_TYPE}
TARGET_OBJ_LIBS += rtos/rtos-toolkit rtos/rtos-toolkit/cmsis-rtos2

include ${PROJECT_ROOT}/tools/makefiles/project.mk

CPPFLAGS += -DTF_OUTPUT=1
CPPFLAGS += -I ${PROJECT_ROOT}/include/cmsis -I ${PROJECT_ROOT}/include/cmsis/rtos2 
LDSCRIPTS := -L${PROJECT_ROOT}/ldscripts -T memory.ld -T regions-flash.ld -T sections.ld

all: ${INSTALL_ROOT}/rtos-cmsis-validation.bin ${INSTALL_ROOT}/rtos-cmsis-validation.elf ${INSTALL_ROOT}/rtos-cmsis-validation.uf2

${INSTALL_ROOT}/rtos-cmsis-validation.uf2: ${CURDIR}/rtos-cmsis-validation.uf2
	@echo "INSTALLING ${@}"
	$(INSTALL) -m 660 -D ${<} ${@}

${INSTALL_ROOT}/rtos-cmsis-validation.elf: ${CURDIR}/rtos-cmsis-validation.elf
	@echo "INSTALLING ${@}"
	$(INSTALL) -m 660 -D ${<} ${@}

${INSTALL_ROOT}/rtos-cmsis-validation.bin: ${CURDIR}/rtos-cmsis-validation.bin
	@echo "INSTALLING ${@}"
	$(INSTALL) -m 660 -D ${<} ${@}

endif

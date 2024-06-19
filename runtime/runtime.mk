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

TARGET_OBJ_LIBS += runtime/hardware-divider
TARGET_OBJ_LIBS += runtime/pico-divider
TARGET_OBJ_LIBS += runtime/pico-platform
TARGET_OBJ_LIBS += runtime/pico-float
TARGET_OBJ_LIBS += runtime/pico-double
TARGET_OBJ_LIBS += runtime/pico-bootrom
TARGET_OBJ_LIBS += runtime/pico-bit-ops
TARGET_OBJ_LIBS += runtime/pico-int64-ops
TARGET_OBJ_LIBS += runtime/pico-mem-ops

include ${PROJECT_ROOT}/tools/makefiles/project.mk

CPPFLAGS += -I${SOURCE_DIR}/include
CFLAGS += -Wno-missing-declarations

all: ${CURDIR}/libruntime.a

endif

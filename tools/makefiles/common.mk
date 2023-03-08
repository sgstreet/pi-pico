#
# Copyright (C) 2017 Red Rocket Computing, LLC
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# commom.mk
#
# Created on: Mar 16, 2017
#     Author: Stephen Street (stephen@redrocketcomputing.com)
#

CC := ${CROSS_COMPILE}gcc
CXX := ${CROSS_COMPILE}g++
LD := ${CROSS_COMPILE}g++
AR := ${CROSS_COMPILE}ar
AS := ${CROSS_COMPILE}as
OBJCOPY := ${CROSS_COMPILE}objcopy
OBJDUMP := ${CROSS_COMPILE}objdump
SIZE := ${CROSS_COMPILE}size
NM := ${CROSS_COMPILE}nm
INSTALL := install
INSTALL_STRIP := install --strip-program=${CROSS_COMPILE}strip -s

CPPFLAGS := 
ARFLAGS := cr
ASFLAGS := ${CROSS_FLAGS} 

CFLAGS := ${CROSS_FLAGS} -pipe -feliminate-unused-debug-types -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wunused -Wuninitialized -Wmissing-declarations -Werror -std=gnu11 -funwind-tables
CXXFLAGS := ${CROSS_FLAGS} -pipe -feliminate-unused-debug-types -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wunused -Wuninitialized -Wmissing-declarations -Werror -std=gnu++20

LDSCRIPTS :=
LDFLAGS := ${CROSS_FLAGS} -Wl,-O1 -Wl,--hash-style=gnu
LOADLIBES := 
LDLIBS :=

-include ${PROJECT_ROOT}/tools/makefiles/${ARCH_CROSS}.mk
-include ${PROJECT_ROOT}/tools/makefiles/${BUILD_TYPE}.mk
